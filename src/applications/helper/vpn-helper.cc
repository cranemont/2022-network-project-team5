#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/uinteger.h"
#include "ns3/vpn-helper.h"
#include "ns3/vpn-client.h"
#include "ns3/string.h"

// #include "ns3/vpn-server.h"

namespace ns3 {
    // VPNServerHelper::VPNServerHelper (uint16_t port) {
    //     m_factory.SetTypeId (VPNServer::GetTypeId ());
    //     SetAttribute ("Port", UintegerValue (port));
    // }

    // VPNServerHelper::SetAttribute (std::string name, const AttributeValue &value) {
    //     m_factory.Set (name, value);
    // }

    // ApplicationContainer VPNServerHelper::Install (Ptr<Node> node) const {
    //     Ptr<Application> app = m_factory.Create<VPNServer> ();
    //     node->AddApplication (app);
    //     return app;
    // }

    VPNClientHelper::VPNClientHelper (Ipv4Address serverIp, uint16_t serverPort) {
        m_factory.SetTypeId (VPNClient::GetTypeId ());
        SetAttribute ("ServerAddress", Ipv4AddressValue (serverIp));
        SetAttribute ("ServerPort", UintegerValue (serverPort));
    }

    VPNClientHelper::VPNClientHelper (Ipv4Address serverIp, uint16_t serverPort, uint16_t clientPort) {
        m_factory.SetTypeId (VPNClient::GetTypeId ());
        SetAttribute ("ServerAddress", Ipv4AddressValue (serverIp));
        if (serverPort != 0)
            SetAttribute ("ServerPort", UintegerValue (serverPort));
        SetAttribute ("ClientPort", UintegerValue (clientPort));
    }

    VPNClientHelper::VPNClientHelper (Ipv4Address serverIp, Ipv4Address clientIp, uint16_t serverPort, uint16_t clientPort) { // TEMPORARY for test
        m_factory.SetTypeId (VPNClient::GetTypeId ());
        SetAttribute ("ServerAddress", Ipv4AddressValue (serverIp));
        SetAttribute ("ClientAddress", Ipv4AddressValue (clientIp));
        if (serverPort != 0)
            SetAttribute ("ServerPort", UintegerValue (serverPort));
        SetAttribute ("ClientPort", UintegerValue (clientPort));
    }

	VPNClientHelper::VPNClientHelper (Ipv4Address serverIp, Ipv4Address clientIp, uint16_t serverPort, uint16_t clientPort, std::string cipherKey) { // TEMPORARY for test
        m_factory.SetTypeId (VPNClient::GetTypeId ());
        SetAttribute ("ServerAddress", Ipv4AddressValue (serverIp));
        SetAttribute ("ClientAddress", Ipv4AddressValue (clientIp));
        if (serverPort != 0)
            SetAttribute ("ServerPort", UintegerValue (serverPort));
        SetAttribute ("ClientPort", UintegerValue (clientPort));
		SetAttribute ("CipherKey", StringValue(cipherKey)); 
    }

    VPNClientHelper::VPNClientHelper (Ipv4Address serverIp) {
        m_factory.SetTypeId (VPNClient::GetTypeId ());
        SetAttribute ("ServerAddress", Ipv4AddressValue (serverIp));
    }

    void VPNClientHelper::SetAttribute (std::string name, const AttributeValue &value) {
        m_factory.Set (name, value);
    }

    ApplicationContainer VPNClientHelper::Install (Ptr<Node> node) const {
        Ptr<Application> app = m_factory.Create<VPNClient> ();
        node->AddApplication (app);
        return app;
    }
}
