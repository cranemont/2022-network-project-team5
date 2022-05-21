#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/uinteger.h"
#include "ns3/vpn-helper.h"
#include "ns3/vpn-application.h"

namespace ns3
{
    VPNHelper::VPNHelper(Ipv4Address clientIp, uint16_t clientPort)
    {
        m_factory.SetTypeId(VPNApplication::GetTypeId());
        SetAttribute("ClientAddress", Ipv4AddressValue(clientIp));
        SetAttribute("ClientPort", UintegerValue(clientPort));
    }

    VPNHelper::VPNHelper(Ipv4Address serverIp, uint16_t serverPort, uint16_t clientPort)
    {
        m_factory.SetTypeId(VPNApplication::GetTypeId());
        SetAttribute("ServerAddress", Ipv4AddressValue(serverIp));
        if (serverPort != 0)
            SetAttribute("ServerPort", UintegerValue(serverPort));
        SetAttribute("ClientPort", UintegerValue(clientPort));
    }

    VPNHelper::VPNHelper(Ipv4Address serverIp, Ipv4Address clientIp, uint16_t serverPort, uint16_t clientPort)
    { // TEMPORARY for test
        m_factory.SetTypeId(VPNApplication::GetTypeId());
        SetAttribute("ServerAddress", Ipv4AddressValue(serverIp));
        SetAttribute("ClientAddress", Ipv4AddressValue(clientIp));
        if (serverPort != 0)
            SetAttribute("ServerPort", UintegerValue(serverPort));
        SetAttribute("ClientPort", UintegerValue(clientPort));
    }

    VPNHelper::VPNHelper(Ipv4Address serverIp)
    {
        m_factory.SetTypeId(VPNApplication::GetTypeId());
        SetAttribute("ServerAddress", Ipv4AddressValue(serverIp));
    }

    void VPNHelper::SetAttribute(std::string name, const AttributeValue &value)
    {
        m_factory.Set(name, value);
    }

    ApplicationContainer VPNHelper::Install(Ptr<Node> node) const
    {
        Ptr<Application> app = m_factory.Create<VPNApplication>();
        node->AddApplication(app);
        return app;
    }
}