#include "ns3/vpn-application.h" 
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/csma-module.h"
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


    << Network topology >>

      Wifi 10.1.2.0
     (V)           AP
     *      *      *
     |      |      |      10.1.1.0      (V)           (sink)
    n5     n6     n0 ------------------ n1   n2   n3   n4
       (sniffer)       point-to-point    |    |    |    |
  (OnOff)                                ================
                                           LAN 11.0.0.0
**/

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SniffingWifiTest");

static void
Sniffer (Ptr<const Packet> p)
{
    Ptr<Packet> copy = p->Copy();

    NS_LOG_DEBUG("\n>> SNIFFING START");
    VpnHeader vpnHeader;
    copy->RemoveHeader(vpnHeader);
    NS_LOG_DEBUG("Sniffing : encrypted data -> " << vpnHeader.GetEncrypted());
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
    bool verbose = true;
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
        LogComponentEnable("SniffingWifiTest", LOG_LEVEL_DEBUG);
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
    
    p2pDevices.Get(0)->TraceConnectWithoutContext("PromiscSniffer", MakeCallback(&Sniffer));
    // apDevices.Get(0)->GetObject<WifiNetDevice>()->GetMac()->TraceConnectWithoutContext("MacPromiscRx", MakeCallback(&Sniffer));

    VPNHelper
        vpnServer("12.0.0.1", 50000),
        vpnClient("10.1.1.2", "11.0.0.100", 50000, 50000);

    ApplicationContainer vpnServerApp, vpnClientApp;
    vpnServerApp = vpnServer.Install(p2pNodes.Get(1));
    vpnClientApp = vpnClient.Install(wifiStaNodes.Get(0));

    vpnServerApp.Start(Seconds(1.0));
    vpnServerApp.Stop(Seconds(10.0));

    vpnClientApp.Start(Seconds(1.0));
    vpnClientApp.Stop(Seconds(10.0));

    OnOffHelper client("ns3::UdpSocketFactory", Address(InetSocketAddress(csmaInterfaces.GetAddress(nCsma), 9)));
    client.SetConstantRate(DataRate("1kb/s"));

    ApplicationContainer clientApps = client.Install(wifiStaNodes.Get(0));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(10.0));

    PacketSinkHelper server("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), 9)));
    ApplicationContainer serverApps = server.Install(csmaNodes.Get(nCsma));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    Simulator::Stop(Seconds(11.0));

    if (tracing)
    {
        pointToPoint.EnablePcapAll("sniffing-wifi-test-p2p");
        phy.EnablePcap("sniffing-wifi-test-ap", apDevices.Get(0));
        csma.EnablePcap("sniffing-wifi-test-csma", csmaDevices.Get(0), true);
    }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}