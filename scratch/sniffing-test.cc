 #include "ns3/vpn-application.h" 
 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/udp-client-server-helper.h"
 #include "ns3/csma-module.h"


 //(OnOff)
 // 10Mb/s, 10us
 // n0 --- n1 --- n2
 //     (sniffer) | CSMA, 100Mb/s, 100ns
 //               |_______
 //               |   |   |
 //               n3  n4  n5
 //             (sink) 
 //
 // n0: 10.1.1.1 /          / 11.0.0.2(V)
 // n1: 10.1.1.2 / 10.2.1.1 /
 // n2:          /          / 10.2.1.2 / 12.0.0.1(V)   / 11.0.0.1
 // n3:          /          /          /               / 11.0.0.2
 // n4:          /          /          /               / 11.0.0.3
 // n5:          /          /          /               / 11.0.0.4
 //
 //
 // n0 -> n3 (Private)
 // 
 //

 using namespace ns3;

 NS_LOG_COMPONENT_DEFINE("SniffingTest");

static void
Sniffer (Ptr<const Packet> p)
{
    Ptr<Packet> copy = p->Copy();

    NS_LOG_DEBUG("SNIFFING START");
    VpnHeader vpnHeader;
    copy->RemoveHeader(vpnHeader);
    NS_LOG_DEBUG("Sniffing : data -> " << vpnHeader.DecryptInput("test", false));
    
    Ipv4Header ipHeader;
    copy->RemoveHeader(ipHeader);
    if(ipHeader.GetProtocol() == 6){
        TcpHeader tcpHeader;
        copy->RemoveHeader(tcpHeader);
    }else if(ipHeader.GetProtocol() == 17){
        UdpHeader udpHeader;
        copy->RemoveHeader(udpHeader);    
    }

    NS_LOG_DEBUG("SNIFFING END");
}
 int main(int argc, char *argv[]) {
     LogComponentEnable("SniffingTest", LOG_LEVEL_DEBUG);
     LogComponentEnable("VPNApplication", LOG_LEVEL_DEBUG);
     LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
     LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
     LogComponentEnable("VpnHeader", LOG_LEVEL_ALL);

     Ptr<Node> n0 = CreateObject<Node>();
     Ptr<Node> n1 = CreateObject<Node>();
     Ptr<Node> n2 = CreateObject<Node>();

     NodeContainer n0n1, n1n2;
     n0n1 = NodeContainer(n0, n1);
     n1n2 = NodeContainer(n1, n2);

     NodeContainer csmaNodes;
     csmaNodes.Add(n2);
     csmaNodes.Create(3);

     PointToPointHelper ptp;
     ptp.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
     ptp.SetChannelAttribute("Delay", StringValue("10us"));

     NetDeviceContainer d1, d2;
     d1 = ptp.Install(n0n1);
     d2 = ptp.Install(n1n2);

     CsmaHelper csma;
     csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
     csma.SetChannelAttribute("Delay", StringValue("100ns"));

     NetDeviceContainer csmaDevices;
     csmaDevices = csma.Install(csmaNodes);

     InternetStackHelper stack;
     stack.Install(n0n1);
     stack.Install(csmaNodes);

     Ipv4AddressHelper a;
     a.SetBase("10.1.1.0", "255.255.255.0");
     Ipv4InterfaceContainer in0 = a.Assign(d1);

     a.SetBase("10.2.1.0", "255.255.255.0");
     Ipv4InterfaceContainer in1 = a.Assign(d2);

     a.SetBase("11.0.0.0", "255.255.255.0");
     Ipv4InterfaceContainer csmaInterfaces;
     csmaInterfaces = a.Assign(csmaDevices);

     Ipv4GlobalRoutingHelper::PopulateRoutingTables();
     n1->GetDevice(0)->GetObject<PointToPointNetDevice>()->TraceConnectWithoutContext("PromiscSniffer", MakeCallback(&Sniffer));

     VPNHelper
         vpn1("10.2.1.2", "11.0.0.100", 50000, 50000),
         vpn2("12.0.0.1", 50000);

     ApplicationContainer vpnApp1;
     vpnApp1 = vpn1.Install(n0);

     vpnApp1.Start(Seconds(1.0));
     vpnApp1.Stop(Seconds(10.0));

     ApplicationContainer vpnApp2;
     vpnApp2 = vpn2.Install(n2);

     vpnApp2.Start(Seconds(1.0));
     vpnApp2.Stop(Seconds(10.0));

     OnOffHelper client("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address("11.0.0.2"), 9)));
     client.SetConstantRate(DataRate("1kb/s"));

     ApplicationContainer clientApp = client.Install(n0);
     clientApp.Start(Seconds(1.0));
     clientApp.Stop(Seconds(10.0));

     PacketSinkHelper server("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), 9)));
     ApplicationContainer serverApp = server.Install(csmaNodes.Get(1));
     serverApp.Start(Seconds(1.0));
     serverApp.Stop(Seconds(10.0));

     ptp.EnablePcapAll("sniffing_test");

     Simulator::Run();
     Simulator::Stop(Seconds(11.0));
     Simulator::Destroy();
     return 0;
 }