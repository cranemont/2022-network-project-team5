#include "ns3/vpn-application.h" 
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/csma-module.h"

/**
 *
 *
    n0: 10.1.1.1 /          / 11.0.0.100(V) /
    n1: 10.1.1.2 / 10.1.2.1 /               /
    n2:          / 10.1.2.2 / 12.0.0.1(V)   / 11.0.0.1
    n3:          /          /               / 11.0.0.2
    n4:          /          /               / 11.0.0.3
    n5:          /          /               / 11.0.0.4

    n0 -> n2 -> n5 (Private, Encryption)
       n1
    (sniffer)


    << Network topology >>

            point-to-point
   (V)  10.1.1.0      10.1.2.0  (V)           (sink)
    n0 ---------- n1 ---------- n2   n3   n4   n5
  (OnOff)      (sniffer)         |    |    |    |
                                 ================
                                   LAN 11.0.0.0
**/

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SniffingTest");

static void
Sniffer (Ptr<const Packet> p)
{
    Ptr<Packet> copy = p->Copy();

    NS_LOG_DEBUG("\n>> SNIFFING START");
    VpnHeader vpnHeader;
    copy->RemoveHeader(vpnHeader);
    NS_LOG_DEBUG("Sniffing : decrypted data -> " << vpnHeader.DecryptInput("00000000000000000000000000000000", false));

    if (vpnHeader.DecryptInput("00000000000000000000000000000000", false) != vpnHeader.GetSentOrigin())
    {
        NS_LOG_DEBUG(">> SNIFFING FAIL");
        return;
    }
    
    Ipv4Header ipHeader;
    copy->RemoveHeader(ipHeader);
    if (ipHeader.GetProtocol() == 6)
    {
        TcpHeader tcpHeader;
        copy->RemoveHeader(tcpHeader);
        NS_LOG_DEBUG("Source IP: " << ipHeader.GetSource());
        NS_LOG_DEBUG("Destination IP: " << ipHeader.GetDestination());
        NS_LOG_DEBUG("Source Port: " << tcpHeader.GetSourcePort());
        NS_LOG_DEBUG("Destination Port: " << tcpHeader.GetDestinationPort());
        NS_LOG_DEBUG("Size: " << copy->GetSize());
    }
    else if (ipHeader.GetProtocol() == 17)
    {
        UdpHeader udpHeader;
        copy->RemoveHeader(udpHeader);
        NS_LOG_DEBUG("Source IP: " << ipHeader.GetSource());
        NS_LOG_DEBUG("Destination IP: " << ipHeader.GetDestination());
        NS_LOG_DEBUG("Source Port: " << udpHeader.GetSourcePort());
        NS_LOG_DEBUG("Destination Port: " << udpHeader.GetDestinationPort());
        NS_LOG_DEBUG("Size: " << copy->GetSize());
    }

    NS_LOG_DEBUG(">> SNIFFING SUCCESS");
}

int
main(int argc, char *argv[])
{
    uint32_t nCsma = 3;

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
    csmaNodes.Create(nCsma);

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
    a.Assign(d1);

    a.SetBase("10.1.2.0", "255.255.255.0");
    a.Assign(d2);

    a.SetBase("11.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = a.Assign(csmaDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    n1->GetDevice(0)->GetObject<PointToPointNetDevice>()->TraceConnectWithoutContext("PromiscSniffer", MakeCallback(&Sniffer));

    VPNHelper
        vpnClient("10.1.2.2", "11.0.0.100", 50000, 50000),
        vpnServer("12.0.0.1", 50000);

    ApplicationContainer vpnClientApp;
    vpnClientApp = vpnClient.Install(n0);

    vpnClientApp.Start(Seconds(1.0));
    vpnClientApp.Stop(Seconds(10.0));

    ApplicationContainer vpnServerApp;
    vpnServerApp = vpnServer.Install(n2);

    vpnServerApp.Start(Seconds(1.0));
    vpnServerApp.Stop(Seconds(10.0));

    OnOffHelper client("ns3::UdpSocketFactory", Address(InetSocketAddress(csmaInterfaces.GetAddress(nCsma), 9)));
    client.SetConstantRate(DataRate("1kb/s"));

    ApplicationContainer clientApp = client.Install(n0);
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(10.0));

    PacketSinkHelper server("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), 9)));
    ApplicationContainer serverApp = server.Install(csmaNodes.Get(nCsma));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    ptp.EnablePcapAll("sniffing_test");

    Simulator::Run();
    Simulator::Stop(Seconds(11.0));
    Simulator::Destroy();
    return 0;
}