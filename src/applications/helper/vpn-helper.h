#ifndef VPN_HELPER_H
#define VPN_HELPER_H

#include <stdint.h>
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/vpn-client.h"
#include "ns3/vpn-server.h"

namespace ns3 {
    class Node;

    // class VPNServerHelper {
    // public:
    //     VPNServerHelper (uint16_t port);

    //     void SetAttribute (std::string name, const AttributeValue &value);

    //     ApplicationContainer Install (Ptr<Node> node) const;

    // private:
    //     ObjectFactory m_factory;
    // };

    class VPNClientHelper {
    public:
        VPNClientHelper (Ipv4Address serverIp, uint16_t serverPort);
        VPNClientHelper (Ipv4Address serverIp, uint16_t serverPort, uint16_t clientPort);
        VPNClientHelper (Ipv4Address serverIp, Ipv4Address clientIp, uint16_t serverPort, uint16_t clientPort); // TEMPORARY for test
        VPNClientHelper (Ipv4Address serverIp);

        void SetAttribute (std::string name, const AttributeValue &value);

        ApplicationContainer Install (Ptr<Node> node) const;

    private:
        ObjectFactory m_factory;
    };
}

#endif /* VPN_HELPER_H */