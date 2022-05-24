#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/udp-header.h"
#include "ns3/mac48-address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/net-device.h"
#include "ns3/virtual-net-device.h"
#include "ns3/socket.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/application.h"
#include "vpn-application.h"
#include "ns3/vpn-aes.h" // for using aes cryption
#include "ns3/vpn-header.h"
#include "ns3/string.h"

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("VPNApplication");

    TypeId VPNApplication::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::VPNApplication")
                                .SetParent<Application>()
                                .SetGroupName("Applications")
                                .AddConstructor<VPNApplication>()
                                .AddAttribute("ServerAddress",
                                              "Server IP",
                                              Ipv4AddressValue(),
                                              MakeIpv4AddressAccessor(&VPNApplication::m_serverAddress),
                                              MakeIpv4AddressChecker())
                                .AddAttribute("ClientAddress",                                              // TEMPORARY for test
                                              "Client VPN IP",                                              //
                                              Ipv4AddressValue(),                                           //
                                              MakeIpv4AddressAccessor(&VPNApplication::m_clientVPNAddress), //
                                              MakeIpv4AddressChecker())                                     //
                                .AddAttribute("ServerPort",
                                              "Server Port",
                                              UintegerValue(1194),
                                              MakeUintegerAccessor(&VPNApplication::m_serverPort),
                                              MakeUintegerChecker<uint16_t>())
                                .AddAttribute("ClientPort",
                                              "Client Port",
                                              UintegerValue(50000),
                                              MakeUintegerAccessor(&VPNApplication::m_clientPort),
                                              MakeUintegerChecker<uint16_t>())
                                .AddAttribute("ServerMask",
                                              "Server Mask",
                                              Ipv4MaskValue("255.255.255.0"),
                                              MakeIpv4MaskAccessor(&VPNApplication::m_serverMask),
                                              MakeIpv4MaskChecker())
                                .AddAttribute("CipherKey",
                                              "Private Cipher Key",
                                              StringValue("12345678901234567890123456789012"),
                                              MakeStringAccessor(&VPNApplication::m_cipherKey),
                                              MakeStringChecker());
        return tid;
    }

    VPNApplication::VPNApplication()
    {
        NS_LOG_FUNCTION(this);
    }

    VPNApplication::~VPNApplication()
    {
        NS_LOG_FUNCTION(this);
    }

    // set server info
    void VPNApplication::SetServer(Ipv4Address addr, uint16_t port)
    {
        NS_LOG_FUNCTION(this << addr << port);
        m_serverAddress = addr;
        m_serverPort = port;
    }

    void VPNApplication::SetServer(Ipv4Address addr)
    {
        NS_LOG_FUNCTION(this << addr);
        m_serverAddress = addr;
    }

    void VPNApplication::SetCipherKey(std::string cipherKey)
    {
        NS_LOG_FUNCTION(this << cipherKey);
        m_cipherKey = cipherKey;
    }
    bool VPNApplication::SendPacket(Ptr<Packet> packet, const Address &src, const Address &dst, uint16_t protocolNumber)
    {
        NS_LOG_DEBUG("\nSend packet from VPN client " << m_clientVPNAddress << " -> " << m_serverAddress);
        ///// encrypt *packet
        NS_LOG_DEBUG("Send to : " << m_serverAddress << ": " << *packet << "with size " << packet->GetSize());

        VpnHeader crypthdr;
        std::string plainText = "62531124552322311567ABD150BBFFCC";

        crypthdr.EncryptInput(plainText, m_cipherKey, false);
        packet->AddHeader(crypthdr);
        NS_LOG_DEBUG("Send to : encrypted -> " << crypthdr.GetEncrypted());
        NS_LOG_DEBUG("Send to : originwas -> " << crypthdr.GetSentOrigin());

        // send encrypted packet to VPN server
        m_clientSocket->SendTo(packet, 0, InetSocketAddress(m_serverAddress, m_serverPort));
        return true;
    }

    void VPNApplication::ReceivePacket(Ptr<Socket> socket)
    {
        Ptr<Packet> packet = socket->Recv(65535, 0);
        NS_LOG_DEBUG("\nVPN server received");
        ///// decrypt *packet
        VpnHeader crypthdr;
        packet->RemoveHeader(crypthdr);
        NS_LOG_DEBUG("Received " << *packet << "with decrypt message");
        NS_LOG_DEBUG("Received : received encrypted -> " << crypthdr.GetEncrypted());
        NS_LOG_DEBUG("Received : received originwas -> " << crypthdr.GetSentOrigin());
        NS_LOG_DEBUG("Received : received decrypted -> " << crypthdr.DecryptInput(m_cipherKey, false));

        // forward decrypted packet
        Ptr<Packet> copy = packet->Copy();
        Ipv4Header ipHeader;
        UdpHeader udpHeader;
        copy->RemoveHeader(ipHeader);
        copy->RemoveHeader(udpHeader);
        Ipv4Address destinationIPAddress = ipHeader.GetDestination();
        uint16_t destinationPort = udpHeader.GetDestinationPort();

        NS_LOG_DEBUG("\nVPN server received");
        NS_LOG_DEBUG("VPN client address: " << m_clientVPNAddress);
        NS_LOG_DEBUG("Source IP: " << ipHeader.GetSource());
        NS_LOG_DEBUG("Destination IP: " << destinationIPAddress);
        NS_LOG_DEBUG("Source Port: " << udpHeader.GetSourcePort());
        NS_LOG_DEBUG("Destination Port: " << destinationPort);
        NS_LOG_DEBUG("Size: " << copy->GetSize());

        if (m_clientVPNAddress == destinationIPAddress && !crypthdr.GetSentOrigin().compare(crypthdr.DecryptInput(m_cipherKey, false)))
        {
            // Destiation of the packet is this VPN Client. Receive packet
            m_clientTap->Receive(packet, 0x0800, m_clientTap->GetAddress(), m_clientTap->GetAddress(), NetDevice::PACKET_HOST);
        }
        else
        {
            // Not for this VPN Client. Forwarding to destination
            packet->RemoveHeader(ipHeader);
            packet->RemoveHeader(udpHeader);
            NS_LOG_DEBUG("\nNot for this VPN Client. Forwarding...\n");
            socket->SendTo(packet, 0, InetSocketAddress(destinationIPAddress, destinationPort));
        }
    }

    void VPNApplication::DoDispose()
    {
        NS_LOG_FUNCTION(this);
        Application::DoDispose();
    }

    void VPNApplication::StartApplication(void)
    {
        // key exchange

        // get client IP
        // m_clientVPNAddress = ;

        // create nic for VPN
        m_clientNode = GetNode();
        m_clientTap = CreateObject<VirtualNetDevice>();
        m_clientTap->SetAddress(Mac48Address::Allocate());
        m_clientNode->AddDevice(m_clientTap);

        // create and bind socket
        m_clientSocket = Socket::CreateSocket(m_clientNode, TypeId::LookupByName("ns3::UdpSocketFactory"));
        m_clientSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_clientPort));
        m_clientSocket->SetRecvCallback(MakeCallback(&VPNApplication::ReceivePacket, this));

        // enable send packet from NIC
        m_clientTap->SetSendCallback(MakeCallback(&VPNApplication::SendPacket, this));

        // add interface
        Ptr<Ipv4> ipv4 = m_clientNode->GetObject<Ipv4>();
        m_clientInterface = ipv4->AddInterface(m_clientTap);
        ipv4->AddAddress(m_clientInterface, Ipv4InterfaceAddress(m_clientVPNAddress, m_serverMask));
        ipv4->SetUp(m_clientInterface);
    }

    void VPNApplication::StopApplication(void)
    {
        // send client exit to server

        // remove interface
        Ptr<Ipv4> ipv4 = m_clientNode->GetObject<Ipv4>();
        ipv4->SetDown(m_clientInterface);
        ipv4->RemoveAddress(m_clientInterface, m_clientVPNAddress);

        // disable packet sending from NIC
        // m_clientTap->SendSendCallback (MakeNullCallback ());

        // unbind socket
        m_clientSocket->ShutdownRecv();
        m_clientSocket->Close();
    }
}