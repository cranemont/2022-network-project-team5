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
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"

/**
 *
 *
    n0: 10.1.1.1 / 10.1.2.3 /               /
    n1: 10.1.1.2 /          / 12.0.0.1(V)   / 11.0.0.1
    n2:          /          /               / 11.0.0.2
    n3:          /          /               / 11.0.0.3
    n4:          /          /               / 11.0.0.4
    n5:          / 10.1.2.1 / 11.0.0.100(V) /
    n6:          / 10.1.2.2 /               /

    n5 -> n1 (Private)
    n6 -> n1 (Public)


    << Network topology >>

      Wifi 10.1.2.0
     (V)           AP
     *      *      *
     |      |      |      10.1.1.0      (V)           (sink)
    n5     n6     n0 ------------------ n1   n2   n3   n4
       (sniffer)       point-to-point    |    |    |    |
    (OnOff)                              ================
                                           LAN 11.0.0.0
**/

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VPNWifiTest");

static void
Throughput (Ptr<const Packet> p, const Address &addr)
{
    static double bytes0=0, bytes1=0;
    static double time0=1, time1=1;
    static double period0=0, period1=0;
    double now = Simulator::Now().GetSeconds();

    if (InetSocketAddress::ConvertFrom(addr).GetIpv4() == "11.0.0.1") {
        bytes0 += p->GetSize();
        
        double period = now - time0;
        time0 = now;

        period0 += period;
        if (period0 < 0.1) return;

        NS_LOG_UNCOND("0\t" << now << "\t" << bytes0*8/1000000/period0);
        bytes0 = 0;
        period0 = 0;
    }
    else if (InetSocketAddress::ConvertFrom(addr).GetIpv4() == "10.1.2.2") {
        bytes1 += p->GetSize();

        double period = now - time1;
        time1 = now;
        
        period1 += period;
        if (period1 < 0.1) return;

        NS_LOG_UNCOND("1\t" << now << "\t" << bytes1*8/1000000/period1);
        bytes1 = 0;
        period1 = 0;
    }
}

int
main(int argc, char *argv[])
{
    bool verbose = false;
    bool tracing = true;
    uint32_t nCsma = 3;
    uint32_t nWifi = 2;

    CommandLine cmd;
    cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);

    cmd.Parse(argc,argv);

    // The underlying restriction of 18 is due to the grid position
    // allocator's configuration; the grid layout will exceed the
    // bounding box if more than 18 nodes are provided.
    if (nWifi > 18)
    {
        std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
        return 1;
    }

    if (verbose)
    {
        LogComponentEnable("VPNWifiTest", LOG_LEVEL_DEBUG);
        LogComponentEnable("VPNApplication", LOG_LEVEL_DEBUG);
        LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
        LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
        LogComponentEnable("VpnHeader", LOG_LEVEL_ALL);
    }

    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("10us"));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(1));
    csmaNodes.Create(nCsma);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("100ns"));

    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install(csmaNodes);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWifi);
    NodeContainer wifiApNode = p2pNodes.Get(0);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211n_5GHZ);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", 
                                 "DataMode", StringValue("HtMcs7"), 
                                 "ControlMode", StringValue("HtMcs0"));

    WifiMacHelper mac;
    Ssid ssid = Ssid("Team5Project");
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid),
                "BeaconInterval", TimeValue(MicroSeconds(102400)), 
                "BeaconGeneration", BooleanValue(true));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, mac, wifiApNode);

    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(wifiStaNodes);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);

    InternetStackHelper stack;
    stack.Install(csmaNodes);
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    address.Assign(p2pDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    address.Assign(staDevices);
    address.Assign(apDevices);

    address.SetBase("11.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = address.Assign(csmaDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    VPNHelper
        vpnServer("12.0.0.1", 50000),
        vpnClient("10.1.1.2", "11.0.0.100", 50000, 50000);

    ApplicationContainer vpnServerApp, vpnClientApp;
    vpnServerApp = vpnServer.Install(p2pNodes.Get(1));
    vpnClientApp = vpnClient.Install(wifiStaNodes.Get(0));

    vpnServerApp.Start(Seconds(1.0));
    vpnServerApp.Stop(Seconds(16.0));

    vpnClientApp.Start(Seconds(1.0));
    vpnClientApp.Stop(Seconds(16.0));

    OnOffHelper client("ns3::UdpSocketFactory", Address(InetSocketAddress(csmaInterfaces.GetAddress(nCsma), 9)));
	client.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
	client.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    client.SetAttribute("DataRate", DataRateValue(5000000));

    ApplicationContainer clientApps;
    clientApps.Add(client.Install(wifiStaNodes.Get(0)));
    clientApps.Add(client.Install(wifiStaNodes.Get(1)));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(16.0));

    PacketSinkHelper server("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), 9)));
    ApplicationContainer serverApps = server.Install(csmaNodes.Get(nCsma));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(16.0));

    serverApps.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&Throughput));

    Simulator::Stop(Seconds(16.0));

    if (tracing)
    {
        pointToPoint.EnablePcapAll("vpn-wifi-test-p2p");
        phy.EnablePcap("vpn-wifi-test-ap", apDevices.Get(0));
        csma.EnablePcap("vpn-wifi-test-csma", csmaDevices.Get(0), true);
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}