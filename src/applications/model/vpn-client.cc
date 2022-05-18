#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/mac48-address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/net-device.h"
#include "ns3/virtual-net-device.h"
#include "ns3/socket.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/application.h"
#include "vpn-client.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE("VPNClientApplication");

    TypeId VPNClient::GetTypeId (void) {
        static TypeId tid = TypeId ("ns3::VPNClient")
                .SetParent<Application> ()
                .SetGroupName ("Applications")
                .AddConstructor<VPNClient> ()
                .AddAttribute ("ServerAddress",
                        "Server IP",
                        Ipv4AddressValue(),
                        MakeIpv4AddressAccessor (&VPNClient::m_serverAddress),
                        MakeIpv4AddressChecker ())
                .AddAttribute ("ClientAddress",                                     // TEMPORARY for test
                        "Client VPN IP",                                            //
                        Ipv4AddressValue (),                                        //
                        MakeIpv4AddressAccessor (&VPNClient::m_clientVPNAddress),   //
                        MakeIpv4AddressChecker ())                                  //
                .AddAttribute ("ServerPort",
                        "Server Port",
                        UintegerValue(1194),
                        MakeUintegerAccessor (&VPNClient::m_serverPort),
                        MakeUintegerChecker<uint16_t> ())
                .AddAttribute ("ClientPort",
                        "Client Port",
                        UintegerValue(50000),
                        MakeUintegerAccessor (&VPNClient::m_clientPort),
                        MakeUintegerChecker<uint16_t> ())
                .AddAttribute ("ServerMask",
                        "Server Mask",
                        Ipv4MaskValue("255.255.255.0"),
                        MakeIpv4MaskAccessor (&VPNClient::m_serverMask),
                        MakeIpv4MaskChecker ());
        return tid;
    }

    VPNClient::VPNClient () {
        NS_LOG_FUNCTION (this);
    }

    VPNClient::~VPNClient () {
        NS_LOG_FUNCTION (this);
    }

    // set server info
    void VPNClient::SetServer (Ipv4Address addr, uint16_t port) {
        NS_LOG_FUNCTION (this << addr << port);
        m_serverAddress = addr;
        m_serverPort = port;
    }
    
    void VPNClient::SetServer (Ipv4Address addr) {
        NS_LOG_FUNCTION (this << addr);
        m_serverAddress = addr;
    }

    bool VPNClient::SendPacket (Ptr<Packet> packet, const Address& src, const Address& dst, uint16_t protocolNumber) {
        NS_LOG_DEBUG ("Send to " << m_serverAddress << ": " << *packet);
        ///// encrypt *packet

        // send encrypted packet to VPN server
        m_clientSocket->SendTo (packet, 0, InetSocketAddress (m_serverAddress, m_serverPort));
        return true;
    }

    void VPNClient::ReceivePacket (Ptr<Socket> socket) {
        Ptr<Packet> packet = socket->Recv (65535, 0);
        ///// decrypt *packet

        NS_LOG_DEBUG ("Received " << *packet);
        // receive packet
        m_clientTap->Receive (packet, 0x0800, m_clientTap->GetAddress (), m_clientTap->GetAddress (), NetDevice::PACKET_HOST);
    }

    void VPNClient::DoDispose () {
        NS_LOG_FUNCTION (this);
        Application::DoDispose ();
    }

    void VPNClient::StartApplication (void) {
        // key exchange

        // get client IP
        // m_clientVPNAddress = ;

        // create nic for VPN
        m_clientNode = GetNode ();
        m_clientTap = CreateObject<VirtualNetDevice> ();
        m_clientTap->SetAddress (Mac48Address::Allocate ());
        m_clientNode->AddDevice (m_clientTap);

        // create and bind socket
        m_clientSocket = Socket::CreateSocket (m_clientNode, TypeId::LookupByName ("ns3::UdpSocketFactory"));
        m_clientSocket->Bind (InetSocketAddress (Ipv4Address::GetAny(), m_clientPort));
        m_clientSocket->SetRecvCallback (MakeCallback (&VPNClient::ReceivePacket, this));

        // enable send packet from NIC
        m_clientTap->SetSendCallback (MakeCallback (&VPNClient::SendPacket, this));

        // add interface
        Ptr<Ipv4> ipv4 = m_clientNode->GetObject<Ipv4> ();
        m_clientInterface = ipv4->AddInterface (m_clientTap);
        ipv4->AddAddress (m_clientInterface, Ipv4InterfaceAddress (m_clientVPNAddress, m_serverMask));
        ipv4->SetUp (m_clientInterface);
    }

    void VPNClient::StopApplication (void) {
        // send client exit to server

        // remove interface
        Ptr<Ipv4> ipv4 = m_clientNode->GetObject<Ipv4> ();
        ipv4->SetDown (m_clientInterface);
        ipv4->RemoveAddress (m_clientInterface, m_clientVPNAddress);

        // disable packet sending from NIC
        // m_clientTap->SendSendCallback (MakeNullCallback ());
        
        // unbind socket
        m_clientSocket->ShutdownRecv ();
        m_clientSocket->Close ();
    }
}