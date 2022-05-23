#include "ns3/vpn-header.h"
#include "ns3/vpn-aes.h"
#include "ns3/log.h"

namespace ns3
{
  NS_LOG_COMPONENT_DEFINE("VpnHeader");
  NS_OBJECT_ENSURE_REGISTERED(VpnHeader);
  TypeId VpnHeader::GetTypeId(void)
  {
    static TypeId tid = TypeId("ns3::VpnHeader")
                            .SetParent<Header>()
                            .AddConstructor<VpnHeader>();
    return tid;
  }

  std::string VpnHeader::EncryptInput(const std::string &input, const std::string &cipherKey, bool verbose)
  {
    m_sentOrigin = input;
    AES aes128(128, MODE::ECB);
    m_encrypted = aes128.encryption(input, cipherKey, verbose);
    return m_encrypted;
  }

  std::string VpnHeader::DecryptInput(const std::string &cipherKey, bool verbose)
  {
    AES aes128(128, MODE::ECB);
    return aes128.decryption(VpnHeader::GetEncrypted(), cipherKey, verbose);
  }

  TypeId VpnHeader::GetInstanceTypeId(void) const
  {
    return GetTypeId();
  }

  std::string VpnHeader::GetSentOrigin(void) const
  {
    return m_sentOrigin;
  }

  std::string VpnHeader::GetEncrypted(void) const
  {
    return m_encrypted;
  }

  void VpnHeader::Serialize(Buffer::Iterator start) const
  {
    const uint8_t *convert = reinterpret_cast<const uint8_t *>(m_sentOrigin.c_str());
    NS_LOG_DEBUG("While Serialize Origin -> " << m_sentOrigin);
    for (int i = 0; i < 32; i++)
    {
      start.WriteU8(convert[i]);
    }

    const uint8_t *convert2 = reinterpret_cast<const uint8_t *>(m_encrypted.c_str());
    for (int i = 0; i < 32; i++)
    {
      start.WriteU8(convert2[i]);
    }
    NS_LOG_FUNCTION(this);
  }

  uint32_t VpnHeader::GetSerializedSize(void) const
  {
    return 64;
  }

  uint32_t VpnHeader::Deserialize(Buffer::Iterator start)
  {
    Buffer::Iterator i = start;

    std::ostringstream ss;
    for (int j = 0; j < 32; j++)
    {
      ss << i.ReadU8();
    }
    // ss << i.ReadNtohU64();
    m_sentOrigin = ss.str();

    std::ostringstream ss2;
    for (int j = 0; j < 32; j++)
    {
      ss2 << i.ReadU8();
    }
    // ss2<< i.ReadNtohU64();
    m_encrypted = ss2.str();

    NS_LOG_FUNCTION(this);

    return 64;
  }

  void VpnHeader::Print(std::ostream &os) const
  {
    os << "m_encrypted =" << m_encrypted << "\n";
  }
}