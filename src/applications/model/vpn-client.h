#ifndef VPN_CLIENT_H
#define VPN_CLIENT_H

#include <stdint.h>
#include "ns3/address.h"
#include "ns3/net-device.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/ptr.h"
#include "ns3/application.h"
#include "ns3/ipv4-address.h"
#include "ns3/virtual-net-device.h"

namespace ns3 {
    class Socket;
    class Packet;
    class Node;
    class Ipv4;
    class VirtualNetDevice;

    class VPNClient : public Application {
    public:
        static TypeId GetTypeId ();

        VPNClient();
        virtual ~VPNClient();

        void SetServer (Ipv4Address addr, uint16_t port);
        void SetServer (Ipv4Address addr);
		void SetCipherKey(std::string cipherKey);

        bool SendPacket (Ptr<Packet> packet, const Address& src, const Address& dst, uint16_t ptorocolNumber);
        void ReceivePacket (Ptr<Socket> socket);
    
    protected:
        virtual void DoDispose (void);

    private:
        virtual void StartApplication (void);
        virtual void StopApplication (void);

        Ipv4Address m_serverAddress; // IP address of server
        uint16_t m_serverPort; // port for server
        Ipv4Mask m_serverMask; // VPN mask

        Ptr<Node> m_clientNode; // this node
        Ptr<Socket> m_clientSocket; // client socket
        Ipv4Address m_clientVPNAddress; // VPN address of client
        uint16_t m_clientPort; // client port
        uint16_t m_clientInterface; // client interface number in Ipv4
        Ptr<VirtualNetDevice> m_clientTap; // client TAP device

        std::string m_cipherKey;// key
    };
}

#endif /* VPN_CLIENT_H */
