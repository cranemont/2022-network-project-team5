#include <string>
#include "ns3/log.h"
#include "ns3/string.h"
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
#include "ns3/vpn-client.h"
#include "ns3/vpn-aes.h" // for using aes cryption
#include "ns3/vpn-header.h"

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
                        MakeIpv4MaskChecker ())
				.AddAttribute ("CipherKey",
						"Private Cipher Key",
						StringValue("12345678901234567890123456789012"),
						MakeStringAccessor ( &VPNClient::m_cipherKey),
						MakeStringChecker())
						;
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

	void VPNClient::SetCipherKey (std::string cipherKey){
		NS_LOG_FUNCTION (this << cipherKey);
		m_cipherKey = cipherKey;
	}

    bool VPNClient::SendPacket (Ptr<Packet> packet, const Address& src, const Address& dst, uint16_t protocolNumber) {
        NS_LOG_DEBUG ("Send to : " << m_serverAddress << ": " << *packet << "with size " << packet->GetSize());
		//std::string packetstring = packet->ToString();
		//NS_LOG_DEBUG ("Packet to String : " << packetstring);
		VpnHeader crypthdr;
		std::string plainText = "62531124552322311567ABD150BBFFCC";
		//std::string cipherKey = "2B7E151628AED2A6ABF7158809CF4F3C";
		crypthdr.EncryptInput(plainText, m_cipherKey, false);
		packet->AddHeader(crypthdr);
		NS_LOG_DEBUG ("Send to : encrypted -> " << crypthdr.GetEncrypted());
		NS_LOG_DEBUG ("Send to : originwas -> " << crypthdr.GetSentOrigin());
		
		//Ptr<Packet> encrypted_packet = packet->Copy();
		//packet->AddAtEnd(encrypted_packet);
		//std::string packetstring = packet->ToString();
		//NS_LOG_DEBUG("Packet to String:" << packetstring);
		//Buffer payLoadContent = encrypted_packet->GetPayLoadContent();

		//uint32_t encrypted_packet_size = encrypted_packet->GetSerializedSize();
		//uint32_t packet_size = packet->GetSerializedSize();

		//NS_LOG_DEBUG("1:: " << packet_size << " 2:: " << encrypted_packet_size);
		//Ptr<Packet> test = encrypted_packet->Copy();
		//encrypted_packet->AddAtEnd(test);


		//NS_LOG_DEBUG("3:: " << encrypted_packet->GetSerializedSize() << " 4::  " << test->GetSerializedSize());
		//uint32_t data = payLoadContent.ReadU32();
		//NS_LOG_DEBUG ("PayLoadContent has : " << data.toString());

		//uint32_t ser_size = payLoadContent.GetSerializedSize();
		//uint8_t *ser = new uint8_t[ser_size];
		//uint32_t *p = reinterpret_cast<uint32_t *> (ser);
		//uint32_t isDone = payLoadContent.Serialize(ser, ser_size);
		//uint32_t isDone = payLoadContent.Serialize(reinterpret_cast<uint8_t *> (p), ser_size);
		//NS_LOG_DEBUG ("PayLoadContent Serializing done well -> 1 / something wrong -> 0  : " << isDone << "with ser_size " << ser_size);
		//NS_LOG_DEBUG (*p);


		//NS_LOG_DEBUG ("Packet's PayLoadContent Serialized is : ");
		//uint32_t* just = reinterpret_cast<uint32_t *>(ser);
		//for(uint32_t i = 0 ; i < ser_size; i++){
			//NS_LOG_DEBUG("just values: " << just[i]);
		//}

		//std::string cipherKey = "2B7E151628AED2A6ABF7158809CF4F3C";
		//aes128.encryption(str_converted_payLoadC, cipherKey, false);
		//NS_LOG_DEBUG(static_cast<uint32_t>test);
		//NS_LOG_DEBUG (reinterpret_cast<uint32_t *>(ser));
		//NS_LOG_DEBUG (*(reinterpret_cast<uint32_t *>(ser)+1));
		//NS_LOG_DEBUG (*ser);
		//NS_LOG_DEBUG ("hi");
		
		//uint32_t* just = reinterpret_cast<uint32_t *>(ser);
		//for(uint32_t i = 0; i < ser_size; i++){
			//NS_LOG_DEBUG("just values: " << just[i]);  // just[i] & (uint32_t)ser[i] is same
			//uint32_t temp = (uint32_t)ser[i];
			//NS_LOG_DEBUG("just values : " << temp );
		//}
		//for(uint32_t i = 0; i < ser_size /sizeof(uint32_t); i++){
			//NS_LOG_DEBUG("32 values : " << just[i]);
		//}

        // send encrypted packet to VPN server
        m_clientSocket->SendTo (packet, 0, InetSocketAddress (m_serverAddress, m_serverPort));
        return true;
    }

    void VPNClient::ReceivePacket (Ptr<Socket> socket) {
        Ptr<Packet> packet = socket->Recv (65535, 0);
		VpnHeader crypthdr;
		packet->RemoveHeader(crypthdr);
		NS_LOG_DEBUG ("Received " << *packet << "with decrypt message");
        //std::string cipherKey = "2B7E151628AED2A6ABF7158809CF4F3C";
		NS_LOG_DEBUG("Received : received encrypted -> " << crypthdr.GetEncrypted());
		NS_LOG_DEBUG("Received : received originwas -> " << crypthdr.GetSentOrigin());
		NS_LOG_DEBUG("Received : received decrypted -> " << crypthdr.DecryptInput(m_cipherKey, false));

		if(!crypthdr.GetSentOrigin().compare(crypthdr.DecryptInput(m_cipherKey,false))){
			NS_LOG_DEBUG("Received : now really get correctly packet");
	        // receive packet
	        m_clientTap->Receive (packet, 0x0800, m_clientTap->GetAddress (), m_clientTap->GetAddress (), NetDevice::PACKET_HOST);
		}
		else{
			NS_LOG_DEBUG("Received : wrong ,, do you sniffing?");
		}
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
