#include "ns3/header.h"
#include "ns3/simulator.h"

namespace ns3
{

  class VpnHeader : public Header
  {
  public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    virtual void Print(std::ostream &os) const;
    std::string EncryptInput(const std::string &input, const std::string &cipherKey, bool verbose);
    std::string DecryptInput(const std::string &cipherKey, bool verbose);
    virtual uint32_t GetSerializedSize(void) const;
    virtual void Serialize(Buffer::Iterator start) const;
    virtual uint32_t Deserialize(Buffer::Iterator start);
    std::string GetSentOrigin(void) const;
    std::string GetEncrypted(void) const;

  private:
    std::string m_sentOrigin;
    std::string m_encrypted;
  };

}