#include "ns3/vpn-application.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/packet.h"
#include "ns3/packet-metadata.h"

/**
 *
 *
    n0: 10.1.1.1 /          /          / 11.0.0.100(V)
    n1:          / 10.1.2.1 /          / 11.0.0.101(V) /
    n2: 10.1.1.2 / 10.1.2.2 / 10.1.3.1
    n3:          /          / 10.1.3.2 / 12.0.0.1(V)   / 11.0.0.1
    n4:          /          /          /               / 11.0.0.2
    n5:          /          /          /               / 11.0.0.3
    n6:          /          /          /               / 11.0.0.4


    n0 -> n3 (Private, Encryption)
    n1 -> n3 (Public)

    << Network topology >>
    (OnOff)
        n0
          \ 5Mb/s, 10us
           \
            \          50Mb/s, 10us
            n2 -------------------------n3
            /                           | CSMA, 100Mb/s, 100ns
           /                            |_______
          / 5Mb/s, 10us                |   |   |
        n1                             n4  n5  n6
    (OnOff)                          (sink)
**/

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PerformanceTest");

static void
RxTime (std::string context, Ptr<const Packet> p, const Address &a)
{
  static double bytes0=0, bytes1=0;
    static double time0=1, time1=1;
    static double period0=0, period1=0;
    double now = Simulator::Now().GetSeconds();

    if (context == "VPN") {
        bytes0 += p->GetSize();
        
        double period = now - time0;
        time0 = now;

        period0 += period;
        if (period0 < 0.1) return;

        NS_LOG_UNCOND("VPN\t" << now << "\t" << bytes0*8/1000000/period0);
        bytes0 = 0;
        period0 = 0;
    }
    else {
        bytes1 += p->GetSize();

        double period = now - time1;
        time1 = now;
        
        period1 += period;
        if (period1 < 0.1) return;

        NS_LOG_UNCOND("NORMAL\t" << now << "\t" << bytes1*8/1000000/period1);
        bytes1 = 0;
        period1 = 0;
    }
}

int main(int argc, char *argv[])
{
    Ptr<Node> n0 = CreateObject<Node>();
    Ptr<Node> n1 = CreateObject<Node>();
    Ptr<Node> n2 = CreateObject<Node>();
    Ptr<Node> n3 = CreateObject<Node>();
    NodeContainer n0n2, n1n2, n2n3, n0n1n2;
    
    n0n2 = NodeContainer(n0, n2);
    n1n2 = NodeContainer(n1, n2);
    n2n3 = NodeContainer(n2, n3);
    n0n1n2 = NodeContainer(n0, n1, n2);

    NodeContainer csmaNodes;
    csmaNodes.Add(n3);
    csmaNodes.Create(3);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("50Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("10us"));

    NetDeviceContainer p2pDevices1, p2pDevices2, p2pDevices3;
    p2pDevices1 = pointToPoint.Install(n0n2);
    p2pDevices2 = pointToPoint.Install(n1n2);
    p2pDevices3 = pointToPoint.Install(n2n3);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("100ns"));

    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install(csmaNodes);

    InternetStackHelper stack;
    stack.Install(n0n1n2);
    stack.Install(csmaNodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces1;
    p2pInterfaces1 = address.Assign(p2pDevices1);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces2;
    p2pInterfaces2 = address.Assign(p2pDevices2);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces3;
    p2pInterfaces3 = address.Assign(p2pDevices3);

    address.SetBase("11.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = address.Assign(csmaDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    VPNHelper
        vpn1("10.1.3.2", "11.0.0.100", 50000, 50000),
        vpn2("12.0.0.1", 50000);

    ApplicationContainer vpnApp1, vpnApp2;
    vpnApp1 = vpn1.Install(n0);
    vpnApp2 = vpn2.Install(n3);

    vpnApp1.Start(Seconds(1.0));
    vpnApp1.Stop(Seconds(10.0));

    vpnApp2.Start(Seconds(1.0));
    vpnApp2.Stop(Seconds(10.0));

    OnOffHelper client("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address("11.0.0.2"), 9)));
    client.SetAttribute("PacketSize", UintegerValue(50000));
    client.SetAttribute("OnTime",	StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    client.SetAttribute("OffTime",	StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    client.SetAttribute("DataRate", DataRateValue(5000000));

    OnOffHelper client2("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address("11.0.0.2"), 10)));
    client2.SetAttribute("PacketSize", UintegerValue(50000));
    client2.SetAttribute("OnTime",	StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    client2.SetAttribute("OffTime",	StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    client.SetAttribute("DataRate", DataRateValue(5000000));

    ApplicationContainer clientApp = client.Install(n0);
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(10.0));

    ApplicationContainer clientApp2 = client2.Install(n1);
    clientApp2.Start(Seconds(1.0));
    clientApp2.Stop(Seconds(10.0));

    PacketSinkHelper server("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), 9)));
    ApplicationContainer serverApp = server.Install(csmaNodes.Get(1));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    PacketSinkHelper server2("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), 10)));
    ApplicationContainer serverApp2 = server2.Install(csmaNodes.Get(1));
    serverApp2.Start(Seconds(1.0));
    serverApp2.Stop(Seconds(10.0));

    serverApp.Get(0)->TraceConnect("Rx", "VPN", MakeCallback(&RxTime));
    serverApp2.Get(0)->TraceConnect("Rx", "NORMAL", MakeCallback(&RxTime));

    Simulator::Run();
    Simulator::Stop(Seconds(11.0));
    Simulator::Destroy();
    return 0;
}