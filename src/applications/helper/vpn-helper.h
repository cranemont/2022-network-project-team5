#ifndef VPN_HELPER_H
#define VPN_HELPER_H

#include <stdint.h>
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/vpn-application.h"

namespace ns3
{
    class Node;
    class VPNHelper
    {
    public:
        VPNHelper(Ipv4Address clientIp, uint16_t clientPort);
        VPNHelper(Ipv4Address serverIp, uint16_t serverPort, uint16_t clientPort);
        VPNHelper(Ipv4Address serverIp, Ipv4Address clientIp, uint16_t serverPort, uint16_t clientPort);
        VPNHelper(Ipv4Address serverIp);

        void SetAttribute(std::string name, const AttributeValue &value);

        ApplicationContainer Install(Ptr<Node> node) const;

    private:
        ObjectFactory m_factory;
    };
}

#endif /* VPN_HELPER_H */