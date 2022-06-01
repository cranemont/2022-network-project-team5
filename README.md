# Simple VPN Application

[한글 버전](./README_ko.md)

This project aims to implement simple VPN communication in an ns-3 environment.<br/>
In order to implement critical security and private network access through IP changes in VPNs, the focus is on implementing encryption and tunneling.

## How to use

### Client

VPN client application can be created by using `VPNHelper`. Constructors of helper are provided as below.

```cpp
VPNHelper::VPNHelper (Ipv4Address serverIp, Ipv4Address clientIp, uint32_t serverPort, uint32_t clientPort, std::string cipherKey);
VPNHelper::VPNHelper (Ipv4Address serverIp, Ipv4Address clientIp, uint32_t serverPort, uint32_t clientPort);
```

If cipher key is not given, default value will be used. There are other constructors with less parameters and you can assign those attributes with `VPNHelper::SetAttribute(std::string, const AttributeValue&)` later.

There are six modifiable attributes.

|Attribute Name|Description|Type|Default Value|
|:-:|-|:-:|:-:|
|`ServerAddress`|public IP of VPN server|`Ipv4Address`||
|`ClientAddress`|private IP of VPN client|`Ipv4Address`||
|`ServerPort`|public port of VPN server|`uint16_t`|`1194`|
|`ClientPort`|public port of VPN client|`uint16_t`|`50000`|
|`ServerMask`|server mask of private network|`Ipv4Mask`|`255.255.255.0`|
|`CipherKey`|key for encrypting/decrypting packets|`std::string`|`12345678901234567890123456789012`|

After setting all attributes, you can create a client application by `VPNHelper::Install(Ptr<Node>)`.

#### Example

```cpp
VPNHelper vpnClient ("10.1.1.1", "11.0.0.2", 1194, 50000);
ApplicationContainer app = vpnClient.Install (nodes.Get(0));

app.Start (Seconds (1.));
app.Stop (Seconds (10.));
```

### Server

The VPN server uses the same `VPNAplication` as the VPN client.
All nodes with `VPNAplication` installed have `VirtualNetDevice`, and the IP address of `VirtualNetDevice` is set to the `ClientAddress` attribute value of `VPNHelper`. If the destination IP address of the received packet is different from the IP address of `VirtualNetDevice`, it is sent to the destination via [IP forwarding](#IP-forwarding) and received via [`VirtualNetDevice::Receive()`](#Packet-Receive-Callback) only if the same.

VPN server apps can be created using 'VPNHelper' just like client apps. Constructors of 'VPNHelper' for VPN server apps are provided as below

```cpp
VPNHelper::VPNHelper(Ipv4Address clientIp, uint16_t clientPort);
VPNHelper::VPNHelper(Ipv4Address clientIp, uint16_t clientPort, std::string cipherKey);
```

#### Example

```cpp
VPNHelper vpnServer ("10.1.3.2", 1194);
ApplicationContainer app = vpnServer.Install (nodes.Get(2));

app.Start (Seconds (0.));
app.Stop (Seconds (11.));
```

## How it works

### IP tunneling

Referenced [`virtual-net-device.cc`](https://www.nsnam.org/doxygen/examples_2virtual-net-device_8cc_source.html) example file from ns3. From the source file, extracted `Tunnel` part which simulated IP tunneling part. This file showed an example of tunneling IP. However there was a static class managed all three nodes' send callback and receive callback, not a class which can be allocated to each node. So we made this static class to application to allocate to each node and work individually.

#### On start

1. Create a `VirtualNetDevice` object for this node.
1. Allocate address defined by `ClientIP` to `VirtualNetDevice`.
1. Create a `Socket` for receiving VPN packets.
1. Allocate an inet address (public IP, `ClientPort`) to `Socket`.
1. Hook send event of `VirtualNetDevice` and receive event of `Socket`.

#### Packet Send (Send event callback)

1. Get original packet to be sent, given by parameter.
1. Call `Socket::SendTo ()` function.

In the send event callback, packet is made up by L3 header(private IP). By calling `Socket::SendTo ()` function with original packet, L4 header(UDP) and L3 header(public IP) will be re-attached according to address information of `Socket` we created. Also, since we set destination address to VPN server when calling `Socket::SendTo ()`, packet will be sent to the VPN server. We can think that original packet became payload of newly created packet and this new packet will be sent to VPN server, not the original destination.

```
original data
-----------
| Payload |
-----------

---------------------
| TCP/UDP | Payload |
---------------------

after application creates a packet
----------------------------------
| private IP | TCP/UDP | Payload | <- packet given to callback
----------------------------------
send to Socket::SendTo ()

----------------------------------------
|     |------------Payload-------------|
| UDP | private IP | TCP/UDP | Payload |
|     |--------------------------------|
----------------------------------------

after Socket::SendTo ()
----------------------------------------------------
|           |     |------------Payload-------------|
| public IP | UDP | private IP | TCP/UDP | Payload |
|           |     |--------------------------------|
----------------------------------------------------
```

#### Packet Receive (Receive event callback)

1. Get packet by calling `Socket::Recv ()`.
1. Call `VirtualNetDevice::Receive ()` function.

In the receive event callback, headers(public network info) of packet are already removed. This means there is only payload left. By calling `VirtualNetDevice::Receive ()`, the procedure of removing headers will be done again. Since this payload is originally a packet with real payload, L4 header and L3 header, procedure of removing headers can be done again. At this procedure, proper NetDevice according to private IP and port will receive this packet and do the right thing.

```
original packet
----------------------------------------------------
|           |     |------------Payload-------------|
| public IP | UDP | private IP | TCP/UDP | Payload |
|           |     |--------------------------------|
----------------------------------------------------

----------------------------------------
|     |------------Payload-------------|
| UDP | private IP | TCP/UDP | Payload |
|     |--------------------------------|
----------------------------------------

after real socket removes headers
-------------Payload--------------
| private IP | TCP/UDP | Payload | <- packet given to receive callback
----------------------------------
send to VirtualNetDevice::Recv()

---------------------
| TCP/UDP | Payload |
---------------------

after virtual socket removes headers
-----------
| Payload |
-----------
```

#### IP forwarding

If the destination address of a packet received in step 1 of [Packet Receive](#packet-receive-receive-event-callback) is different from the IP address assigned to `VirtualNetDevice`, it means that the packet is not for the `VPNApplication` installed on this node. In this case, `VirtualNetDevice::Receive()` function is not being called. It removes the IP header and TCP/UDP header using `Packet::RemoveHeader()` and then call the function `Socket:SendTo()` to forward the packet to the destination address.

```
original packet
----------------------------------------------------
|           |     |------------Payload-------------|
| public IP | UDP | private IP | TCP/UDP | Payload |
|           |     |--------------------------------|
----------------------------------------------------

----------------------------------------
|     |------------Payload-------------|
| UDP | private IP | TCP/UDP | Payload |
|     |--------------------------------|
----------------------------------------

after real socket removes headers
-------------Payload--------------
| private IP | TCP/UDP | Payload | <- packet given to receive callback
----------------------------------

remove IpHeader and TcpHeader/UdpHeader
Packet->RemoveHeader(IpHeader)
Packet->RemoveHeader(TcpHeader/UdpHeader)
-----------
| Payload |
-----------

after Socket::SendTo ()
|------------------------------------|
| Destination IP | TCP/UDP | Payload |
|------------------------------------|
```

### VPN Header Encryption/decryption

#### Purpose of VPN Header Definition 
 To ensure confidentiality, one of the six objectives of information protection, using headers that encrypt and decrypt predefined plaintexts for communication between subscribers and servers on the VPN.

#### Introducing the cipher-algorithm we used
 The cryptographic algorithm used in the implementation used the AES algorithm. In AES, 
 
 1. It is known to be more reliable in attack than DES, the standard for data encryption.
 2. The plaintext to be encrypted and decrypted must be 128 bits in size, and there are three types of encryption keys: 128, 192, and 256 bits in length.
 3. Compared to the public key encryption method, it has the advantage of lighter computation and relatively simple encryption process.
 4. Block cipher mode is used to prevent the problem that the same ciphertext can be output if the plain text and the key are the same, and it is reflected so that it can be used by selecting between ECB and CBC modes.
 5. The number of Nr rounds is determined as 10, 12, and 14 depending on the length of the encryption key, and the MixColumn in the last round is omitted.
 6. SubByte in the figure below means S-box, transpose rows in ShiftRows step, and MixColumn is a step in which columns are expressed in the form of polynomials and then multiplied by a specific polynomial.
 7. While decrypting, the inverse polynomial of the specific polynomial multiplied is used, and the inverse-SubByte and inverse-ShitRows processes are performed similarly.
 
 ```
 |==========================|
 |-- PlainText M(128 bit) --|
 |-------    ::    ---------|
 |-------    VV    ---------|  
 |--------- Ex-OR ----------|  <- 0 round key
 |-------    ::    ---------|
 |-------    VV    ---------|  
 |-------- SubByte ---------|
 |-------- ShiftRows--------|
 |-------- MixColumn--------|
 |-------    ::    ---------|
 |-------    VV    ---------|  
 |--------- Ex-OR ----------|  <- 1 round key
 |--------------------------|
 |-------    ...  ----------|
 |-------    ...  ----------|
 |-------    ::    ---------|
 |-------    VV    ---------|  <- .. round
 |-------    ...  ----------|
 |-------    ...  ----------|
 |-------- SubByte ---------|
 |-------- ShiftRows--------|
 |-------    ::    ---------|
 |-------    VV    ---------|  
 |--------- Ex-OR ----------|  <- Nr round key
 |-------    ::    ---------|
 |-------    VV    ---------|  
 |-------  CipherText ------|
 |==========================|
 ```

#### Encryption/decryption in VPN header
Use the two key methods below to do so.
```cpp
std::string EncryptInput(const std::string &input, const std::string &cipherKey, bool verbose);
std::string DecryptInput(const std::string &cipherKey, bool verbose);
```
Each method performs an encryption algorithm by referring to the `AES` class defined in `vpn-aes.h`.

##### Examples of encryption/decryption
In the `VPNAplication` layer that uses the VPN header,

the sender's side
```cpp
VpnHeader crypthdr;
std::string plainText = "62531124552322311567ABD150BBFFCC";
crypthdr.EncryptInput(plainText, m_cipherKey, false);
```

the receiver's side
```cpp
VpnHeader crypthdr;
packet->RemoveHeader(crypthdr);
crypthdr.DecryptInput(m_cipherKey, false)
```
#### Core members (functions and variables) structure of VPN headers
Suppose you use a 32-byte-long plaintext by default, and the `private` variables to convert and extract to bytes between serial and reverse serialization are `m_sentOrigin` and `m_encrypted`, so `GetSerializedSize` is twice the 32-byte length of the specified plaintext.

|access specifier|name|info|
|:-:|-|-|
|`public`|`EncryptInput`|Encryption function of plaintext|
|`public`|`DecryptInput`|Decryption function of ciphertext|
|`public`|`GetSerializedSize`|Required space definition function for serialization|
|`public`|`Serialize`|Serialization Performance Function|
|`public`|`Deserialize`|Deserialization Performance Function|
|`public`|`GetSendOrigin`|Plain text return function|
|`public`|`GetEncrypted`|Cipher text return function|
|`private`|`m_sentOrigin`|Plain text member variable|
|`private`|`m_encrypted`|Cipher text member variable|

In addition, if you adjust the length of the plaintext, the `Serialize` and `Deserialize` functions that perform serialization also need to be modified to that length.

##### Example
Modifying to 16-byte length plaintext (minimum length of 128 bits)
```cpp
void VpnHeader::Serialize(Buffer::Iterator start) const
  {
    ...
    for (int i = 0; i < 16; i++){ start.WriteU8(convert[i]); }  // 32 -> 16
    ...
  }
uint32_t VpnHeader::GetSerializedSize(void) const
  {
    return 32;  // 32 -> 16
  }

  uint32_t VpnHeader::Deserialize(Buffer::Iterator start)
  {
    ...
    for (int j = 0; j < 16; j++){ ss << i.ReadU8(); } // 32 -> 16
    ...
    
    return 32;  // 64 -> 32
  }
```

## How to Test

#### Sniffing Test
```
          point-to-point
 (V)  10.1.1.0      10.1.2.0  (V)           (sink)
  n0 ---------- n1 ---------- n2   n3   n4   n5
(OnOff)      (sniffer)         |    |    |    |
                               ================
                                 LAN 11.0.0.0

    n0 -> n2 -> n5 (Private, Encryption)
       n1
    (sniffer)

-IP Address-
n0: 10.1.1.1 /          / 11.0.0.100(V) /
n1: 10.1.1.2 / 10.1.2.1 /               /
n2:          / 10.1.2.2 / 12.0.0.1(V)   / 11.0.0.1
n3:          /          /               / 11.0.0.2
n4:          /          /               / 11.0.0.3
n5:          /          /               / 11.0.0.4
```
Simulates a situation in which the intermediate node, n1, attempts to sniff when communicating from n0 to n5 over a VPN.<br/>
On n1, use PromiscSniffer Trace source to sniff all packets passing by, and if the sniffing is successful, print out the address of the source and destination in debug mode.

<img src="https://user-images.githubusercontent.com/58473522/171424934-2bdd6348-16ea-49d9-9061-b79bca7bf9f6.png" width="700"/>

#### p2p link performance Test
```
(OnOff)
    n0
      \ 5Mbps, 10us
       \
        \          50Mbps, 10us
        n2 -------------------------n3
        /                           | CSMA, 100Mbps, 100ns
       /                            |_______
      / 5Mbps, 10us                 |   |   |
    n1                             n4  n5  n6
(OnOff)                          (sink)

    n0 -> n3 -> n4 (Private, Encryption)
    n1    ->    n4 (Public)

-IP Address-
n0: 10.1.1.1 /          /          / 11.0.0.100(V) /
n1:          / 10.1.2.1 /          /               /
n2: 10.1.1.2 / 10.1.2.2 / 10.1.3.1 /               /
n3:          /          / 10.1.3.2 / 12.0.0.1(V)   / 11.0.0.1
n4:          /          /          /               / 11.0.0.2
n5:          /          /          /               / 11.0.0.3
n6:          /          /          /               / 11.0.0.4
```

#### wifi performance Test
```
Wifi 10.1.2.0
(V)       AP
 *    *    *
 |    |    |      10.1.1.0      (V)           (sink)
n5   n6   n0 ------------------ n1   n2   n3   n4
(OnOff)        point-to-point    |    |    |    |
                10Mbps, 10us     ================
                                   LAN 11.0.0.0
                                  100Mbps, 100ns

    n5 -> n1 -> n4 (Private, Encryption)
    n6    ->    n4 (Public)

-IP Address-
n0: 10.1.1.1 / 10.1.2.3 /               /
n1: 10.1.1.2 /          / 12.0.0.1(V)   / 11.0.0.1
n2:          /          /               / 11.0.0.2
n3:          /          /               / 11.0.0.3
n4:          /          /               / 11.0.0.4
n5:          / 10.1.2.1 / 11.0.0.100(V) /
n6:          / 10.1.2.2 /               /
```
Measure VPN communication throughput in wifi communication situations.<br/>
Constantly fix the DataRate value of OnOffApplication to 5Mbps and compare it with normal communication.

![image](https://user-images.githubusercontent.com/58473522/171427002-0065b2df-b277-4362-bc69-832355d2430f.png)

## License
This project is open under the GPLv2 license.<br/>
You can see the full license at [LICENSE](https://github.com/nsnam/ns-3-dev-git/blob/master/LICENSE).
