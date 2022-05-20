#include "ns3/vpn-client.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/packet.h"
#include "ns3/packet-metadata.h"

//
// n0 --- n1 --- n2
//
// n0: 10.1.1.1 /          / 11.0.0.2(V)
// n1: 10.1.1.2 / 10.2.1.1 /
// n2:          / 10.2.1.2 / 11.0.0.1(V)
//
//
// n0 -> n1 (Public)
// n0 -> n2 (Private)
//

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VPNTest");

int main(int argc, char *argv[]) {
    LogComponentEnable("VPNTest", LOG_LEVEL_ALL);
    LogComponentEnable("VPNClientApplication", LOG_LEVEL_DEBUG);
	LogComponentEnable("VpnHeader", LOG_LEVEL_ALL);
    //LogComponentEnable("PacketSink", LOG_LEVEL_ALL);
    //LogComponentEnable("Packet", LOG_LEVEL_FUNCTION);
	//LogComponentEnable("Buffer", LOG_LEVEL_FUNCTION);

	Packet init;
	init.EnablePrinting();
	init.EnableChecking();

	PacketMetadata::Enable();

    NodeContainer nodes, n0n1, n1n2;
    nodes.Create(3);
    n0n1 = NodeContainer(nodes.Get(0), nodes.Get(1));
    n1n2 = NodeContainer(nodes.Get(1), nodes.Get(2));

    PointToPointHelper ptp;
    ptp.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    ptp.SetChannelAttribute("Delay", StringValue("10us"));

    NetDeviceContainer d1, d2;
    d1 = ptp.Install(n0n1);
    d2 = ptp.Install(n1n2);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper a;
    a.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer in0 = a.Assign(d1);

    a.SetBase("10.2.1.0", "255.255.255.0");
    Ipv4InterfaceContainer in1 = a.Assign(d2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    VPNClientHelper vpn1("10.2.1.2", "11.0.0.2", 50000, 50000), vpn2("10.1.1.1", "11.0.0.1", 50000, 50000);
    ApplicationContainer vpnApp1 = vpn1.Install(n0n1.Get(0)), vpnApp2 = vpn2.Install(n1n2.Get(1));
    vpnApp1.Start(Seconds(1.0));
    vpnApp1.Stop(Seconds(10.0));
    vpnApp2.Start(Seconds(1.0));
    vpnApp2.Stop(Seconds(10.0));

    OnOffHelper client("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address ("11.0.0.1"), 9)));
    client.SetConstantRate (DataRate ("1kb/s"));

    OnOffHelper client2("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address ("10.1.1.2"), 9)));
    client2.SetConstantRate (DataRate ("1kb/s"));

    ApplicationContainer clientApp = client.Install(n0n1.Get(0));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(10.0));

    ApplicationContainer clientApp2 = client2.Install(n0n1.Get(0));
    clientApp2.Start(Seconds(1.0));
    clientApp2.Stop(Seconds(10.0));

    PacketSinkHelper server("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny(), 9)));
    ApplicationContainer serverApp = server.Install(n1n2.Get(1));

    PacketSinkHelper server2("ns3::UdpSocketFactory", Address (InetSocketAddress (Ipv4Address::GetAny(), 9)));
    ApplicationContainer serverApp2 = server2.Install(n0n1.Get(1));

    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));
    serverApp2.Start(Seconds(1.0));
    serverApp2.Stop(Seconds(10.0));

    ptp.EnablePcapAll("test1");

    Simulator::Run();
    Simulator::Stop(Seconds(11.0));
    Simulator::Destroy();
    return 0;
}
