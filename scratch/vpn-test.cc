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


    n0 -> n1 (Public)
    n0 -> n2 (Private)

    << Network topology >>
    (OnOff)
        n0
          \ 10Mb/s, 10us
           \
            \          10Mb/s, 10us
            n2 -------------------------n3
            /                           | CSMA, 100Mb/s, 100ns
           /                            |_______
          / 10Mb/s, 10us                |   |   |
        n1                             n4  n5  n6
    (OnOff)                          (sink)
**/

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VPNTest");

int main(int argc, char *argv[])
{
    LogComponentEnable("VPNTest", LOG_LEVEL_DEBUG);
    LogComponentEnable("VPNApplication", LOG_LEVEL_DEBUG);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    LogComponentEnable("VpnHeader", LOG_LEVEL_ALL);

    Packet init;
    init.EnablePrinting();
    init.EnableChecking();

    PacketMetadata::Enable();

    NodeContainer p2pNodes, n0n2, n1n2, n2n3, n0n1n2;
    p2pNodes.Create(4);
    n0n2 = NodeContainer(p2pNodes.Get(0), p2pNodes.Get(2));
    n1n2 = NodeContainer(p2pNodes.Get(1), p2pNodes.Get(2));
    n2n3 = NodeContainer(p2pNodes.Get(2), p2pNodes.Get(3));
    n0n1n2 = NodeContainer(p2pNodes.Get(0), p2pNodes.Get(1), p2pNodes.Get(2));

    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(3));
    csmaNodes.Create(3);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
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
        vpn2("10.1.3.2", "11.0.0.101", 50000, 50000),
        vpn3("12.0.0.1", 50000);

    ApplicationContainer vpnApp1, vpnApp2, vpnApp3;
    vpnApp1 = vpn1.Install(p2pNodes.Get(0));
    vpnApp2 = vpn2.Install(p2pNodes.Get(1));
    vpnApp3 = vpn3.Install(p2pNodes.Get(3));

    vpnApp1.Start(Seconds(1.0));
    vpnApp1.Stop(Seconds(10.0));

    vpnApp2.Start(Seconds(1.0));
    vpnApp2.Stop(Seconds(10.0));

    vpnApp3.Start(Seconds(1.0));
    vpnApp3.Stop(Seconds(10.0));

    OnOffHelper client("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address("11.0.0.2"), 9)));
    client.SetConstantRate(DataRate("1kb/s"));

    OnOffHelper client2("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address("11.0.0.2"), 9)));
    client2.SetConstantRate(DataRate("1kb/s"));

    ApplicationContainer clientApp = client.Install(p2pNodes.Get(0));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(10.0));

    ApplicationContainer clientApp2 = client2.Install(p2pNodes.Get(1));
    clientApp2.Start(Seconds(1.0));
    clientApp2.Stop(Seconds(10.0));

    PacketSinkHelper server("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), 9)));
    ApplicationContainer serverApp = server.Install(csmaNodes.Get(1));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    pointToPoint.EnablePcapAll("vpn-test-p2p");
    csma.EnablePcapAll("vpn-test-csma");

    Simulator::Run();
    Simulator::Stop(Seconds(11.0));
    Simulator::Destroy();
    return 0;
}