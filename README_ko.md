# Simple VPN Application

[English Version](./README.md)

## 사용법

### 클라이언트

VPN 클라이언트 앱은 `VPNHelper`를 사용하여 만들 수 있습니다. `VPNHelper`의 생성자는 아래와 같습니다.

```cpp
VPNHelper::VPNHelper (Ipv4Address serverIp, Ipv4Address clientIp, uint32_t serverPort, uint32_t clientPort, std::string cipherKey);
VPNHelper::VPNHelper (Ipv4Address serverIp, Ipv4Address clientIp, uint32_t serverPort, uint32_t clientPort);
```

`cipherKey`가 주어지지 않으면 기본값이 사용됩니다. 더 적은 매개변수를 사용하는 생성자가 존재하고, 이를 사용하여 생성한 경우 나중에 `VPNHelper::SetAttribute(std::string, const AttributeValue&)`를 사용하여 값을 지정할 수 있습니다.

attribute은 총 6가지가 있습니다.

|이름|설명|타입|기본값|
|:-:|-|:-:|:-:|
|`ServerAddress`|VPN 서버의 공인 IP|`Ipv4Address`||
|`ClientAddress`|VPN 클라이언트의 사설 IP|`Ipv4Address`||
|`ServerPort`|VPN 서버의 공인 포트|`uint16_t`|`1194`|
|`ClientPort`|VPN 클라이언트의 공인 포트|`uint16_t`|`50000`|
|`ServerMask`|사설 네트워크의 IP 마스크|`Ipv4Mask`|`255.255.255.0`|
|`CipherKey`|패킷 암호화/복호화를 위한 키|`std::string`|`12345678901234567890123456789012`|

모든 attribute을 설정했다면, `VPNHelper::Install(Ptr<Node>)`를 사용하여 클라이언트 앱을 만들 수 있습니다.

#### 예시

```cpp
VPNHelper vpnClient ("10.1.1.1", "11.0.0.2", 1194, 50000);
ApplicationContainer app = vpnClient.Install (nodes.Get(0));

app.Start (Seconds (1.));
app.Stop (Seconds (10.));
```

### 서버

VPN 서버는 VPN 클라이언트와 동일한 `VPNApplication`을 사용합니다.
모든 `VPNApplication`이 설치된 Node는 `VirtualNetDevice`를 가지며, `VirtualNetDevice`의 IP주소는 `VPNHelper`의 `ClientAddress` attribute 값으로 설정됩니다. 수신한 패킷의 목적지 IP주소가 `VirtualNetDevice`의 IP주소와 다른 경우에는 [IP forwarding](#IP-forwarding)을 통해 목적지로 보내고, 동일한 경우에만 [`VirtualNetDevice::Receive ()`](#패킷-수신-콜백)를 통해 패킷을 수신합니다.

VPN 서버 앱은 클라이언트 앱과 동일하게 `VPNHelper`를 사용하여 만들 수 있습니다. VPN 서버를 위한 `VPNHelper`의 생성자는 다음과 같습니다.

```cpp
VPNHelper::VPNHelper(Ipv4Address clientIp, uint16_t clientPort);
VPNHelper::VPNHelper(Ipv4Address clientIp, uint16_t clientPort, std::string cipherKey);
```

#### 예시

```cpp
VPNHelper vpnServer ("10.1.3.2", 1194);
ApplicationContainer app = vpnServer.Install (nodes.Get(2));

app.Start (Seconds (0.));
app.Stop (Seconds (11.));
```

## 테스트 환경

```
         P2P          P2P
       10 Mbps      10 Mbps
        10 us        10 us
   n0 --------- n1 --------- n2
(Onoff) ===== (Sink)              [Public]
(Onoff) ================== (Sink) [Private]

-IP Address-
n0: 10.1.1.1 /          / 11.0.0.2(V)
n1: 10.1.1.2 / 10.2.1.1 /
n2:          / 10.2.1.2 / 11.0.0.1(V)
```

```
(OnOff)
    n0
      \ 10Mb/s, 10us
       \
        \          10Mb/s, 10us
        n2 -------------------------n3
        /                           | CSMA, 100Mb/s, 100ns
       /                            |_______
      / 10Mb/s, 10us                |   |   |
    n1                             n4  n5  n6
(OnOff)                          (sink)

-IP Address-
n0: 10.1.1.1 /          /          / 11.0.0.100(V)
n1:          / 10.1.2.1 /          / 11.0.0.101(V) /
n2: 10.1.1.2 / 10.1.2.2 / 10.1.3.1
n3:          /          / 10.1.3.2 / 12.0.0.1(V)   / 11.0.0.1
n4:          /          /          /               / 11.0.0.2
n5:          /          /          /               / 11.0.0.3
n6:          /          /          /               / 11.0.0.4
```

## 구현 방법

### IP 터널링

ns3의 [`virtual-net-device.cc`](https://www.nsnam.org/doxygen/examples_2virtual-net-device_8cc_source.html) 예시 파일을 참조하여 만들었습니다. 해당 파일에서 IP 터널링을 구현한 `Tunnel` 클래스를 추출하여 사용했습니다. 이 파일은 IP 터널링의 예시 코드였는데, 여기서는 하나의 정적인 object가 세 노드의 터널링 과정을 모두 관리했습니다. 그래서 이 `Tunnel` 클래스를 각 노드에 설치하여 각각 알아서 작동할 수 있게끔 어플리케이션으로 만들었습니다.

#### 시작 시

1. 이 노드에 `VirtualNetDevice` 오브젝트를 하나 생성합니다.
1. `VirtualNetDevice`에 `ClientIP`로 정의된 주소를 할당합니다.
1. VPN 패킷을 받기 위한 `Socket`을 생성합니다.
1. 이 노드의 공인 IP와 `ClientPort`로 정의된 Inet 주소를 `Socket`에 할당합니다.
1. `VirtualNetDevice`의 send와 `Socket`의 receive에 콜백을 추가합니다.

#### 패킷 전송 콜백

1. 원래 보내려던 패킷을 매개변수를 통해 받습니다.
1. `Socket::SendTo ()` 함수를 호출합니다.

이 콜백에서는 L3 헤더(사설 IP)까지 쌓여진 패킷을 매개변수로 받습니다. `Socket::SendTo ()` 함수에 이 패킷을 매개변수로 호출하여 새로 만든 `Socket`의 Inet 주소를 기반으로 하는 새로운 L4 헤더(UDP)와 L3 헤더(공인 IP)로 다시 쌉니다. 또한, `Socket::SendTo ()` 함수를 호출하면서 VPN 서버의 주소를 목적지로 하기 때문에 이 패킷은 VPN 서버로 보내집니다. 원래 보내려던 패킷이 새로운 패킷의 페이로드가 되고, 원래 목적지가 아닌 VPN 서버로 패킷이 보내진다고 생각할 수 있습니다.

```
원래의 데이터
-----------
| Payload |
-----------

---------------------
| TCP/UDP | Payload |
---------------------

다른 어플리케이션이 패킷을 만든 후
----------------------------------
| private IP | TCP/UDP | Payload | <- 매개변수로 주어진 패킷
----------------------------------
Socket::SendTo () 호출

----------------------------------------
|     |------------Payload-------------|
| UDP | private IP | TCP/UDP | Payload |
|     |--------------------------------|
----------------------------------------

Socket::SendTo ()가 완료된 후
----------------------------------------------------
|           |     |------------Payload-------------|
| public IP | UDP | private IP | TCP/UDP | Payload |
|           |     |--------------------------------|
----------------------------------------------------
```

#### 패킷 수신 콜백

1. `Socket::Recv ()`를 호출하여 패킷을 읽어들입니다.
1. `VirtualNetDevice::Receive ()` 함수를 호출합니다.

수신 콜백에서는 이미 패킷의 헤더가 모두 제거된 상태로 들어옵니다. 즉, 페이로드만 남아있는 상태입니다. 이 상태에서 `VirtualNetDevice::Receive ()`를 호출하여 패킷의 헤더를 제거하는 과정을 한 번 더 진행합니다. 페이로드는 전송 시 다른 헤더로 감쌌던 L3 헤더까지 있던 패킷이었기 때문에 헤더 제거 과정을 한 번 더 진행할 수 있습니다. 이 과정에서 올바른 NetDevice로 패킷이 전달되며 원래 패킷의 내용이 처리됩니다.

```
수신한 패킷
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

Socket이 헤더를 제거
-------------Payload--------------
| private IP | TCP/UDP | Payload | <- packet given to receive callback
----------------------------------
VirtualNetDevice::Recv() 호출

---------------------
| TCP/UDP | Payload |
---------------------

실제 Socket이 헤더를 제거한 뒤
-----------
| Payload |
-----------
```

#### IP forwarding

[패킷 수신 콜백](#패킷-수신-콜백)의 1번 과정에서 수신한 패킷의 목적지 Address가 `VirtualNetDevice`에 할당된 IP주소와 다르다면 해당 패킷은 해당 Node에 설치된 `VPNApplication`을 위한 패킷이 아님을 의미합니다. 이 경우 `VirtualNetDevice::Receive ()`함수를 호출하지 않고 `Packet::RemoveHeader ()`를 사용하여 IP header 및 TCP/UDP header를 제거한 뒤 `Socket::SendTo ()`함수를 호출하여 목적지 Address로 패킷을 forwarding합니다.

```
수신한 패킷
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

Socket이 헤더를 제거
-------------Payload--------------
| private IP | TCP/UDP | Payload | <- packet given to receive callback
----------------------------------

IpHeader, TcpHeader/UdpHeader 제거
Packet->RemoveHeader(IpHeader)
Packet->RemoveHeader(TcpHeader/UdpHeader)
-----------
| Payload |
-----------

Socket::SendTo ()가 완료된 후
|------------------------------------|
| Destination IP | TCP/UDP | Payload |
|------------------------------------|
```

### VPN Header Encryption/decryption

#### VPNHeader 정의의 목적
 VPN 상의 가입자 및 서버 간의 통신에 기 정의된 평문을 암호화 및 복호화를 수행하는 헤더를 사용하여 정보보호의 6가지 목표 중 하나인 기밀성을 보장하기 위함입니다.

#### 사용한 암호 알고리즘 소개
 구현에 이용된 암호 알고리즘은 AES 알고리즘을 사용하였습니다. AES는, 
 
 1. 데이터 암호화 표준인 DES 보다 상대적으로 공격에 안정성을 갖고 있다고 알려져 있습니다.
 2. 암호화 및 복호화 대상인 평문은 128비트 단위의 크기를 가져야 하며, 암호화 키의 길이는 128, 192, 256 비트의 세 가지 종류가 있습니다.
 3. 공개키 암호방식에 비해 연산량이 가볍다는 점과 상대적으로 암복호화 과정이 단순하다는 장점이 있는 상용관용암호방식입니다.
 4. 평문과 키가 동일한 경우 같은 암호문을 출력할 수 있다는 문제점을 방지하기 위해 블록암호모드를 사용하며 ECB, CBC 모드 중 선택하여 사용할 수 있도록 반영되었습니다.
 5. Nr 라운드 수는 암호화 키의 길이에 따라 10, 12, 14로 정해지게 되고, 마지막 라운드에서 MixColumn은 생략한 형태를 띱니다.
 6. 아래 그림에 표현한 SubByte는 S-box를 의미하고, ShiftRows 단계에서 행들을 전치시키며 MixColumn은 열을 다항식의 형태로 표현한 후 특정 다항식을 곱하는 단계입니다.
 7. 복호화 때에는 곱해주었던 특정 다항식의 역다항식을 이용, 마찬가지로 역-SubByte, 역-ShitRows 과정을 수행합니다.
 
 ```
 |==========================|
 |---- 평문 M(128 비트) ----|
 |-------    ::    ---------|
 |-------    VV    ---------|  
 |--------- Ex-OR ----------|  <- 0 라운드 키
 |-------    ::    ---------|
 |-------    VV    ---------|  
 |-------- SubByte ---------|
 |-------- ShiftRows--------|
 |-------- MixColumn--------|
 |-------    ::    ---------|
 |-------    VV    ---------|  
 |--------- Ex-OR ----------|  <- 1 라운드 키
 |--------------------------|
 |-------    ...  ----------|
 |-------    ...  ----------|
 |-------    ::    ---------|
 |-------    VV    ---------|  <- .. 라운드
 |-------    ...  ----------|
 |-------    ...  ----------|
 |-------- SubByte ---------|
 |-------- ShiftRows--------|
 |-------    ::    ---------|
 |-------    VV    ---------|  
 |--------- Ex-OR ----------|  <- Nr 라운드 키
 |-------    ::    ---------|
 |-------    VV    ---------|  
 |-------- 암호문 C --------|
 |==========================|
 ```

#### VPN 헤더에서의 암/복호화
아래 핵심 두 메소드를 통해 수행합니다.
```cpp
std::string EncryptInput(const std::string &input, const std::string &cipherKey, bool verbose);
std::string DecryptInput(const std::string &cipherKey, bool verbose);
```
각 메소드는 `vpn-aes.h`에 정의된 `AES` 클래스를 참조하여 암호화 알고리즘을 수행하게 됩니다. 

##### 암/복호화 예시
VPN 헤더를 사용하는 `VPNApplication` layer에서,

송신자 측
```cpp
VpnHeader crypthdr;
std::string plainText = "62531124552322311567ABD150BBFFCC";
crypthdr.EncryptInput(plainText, m_cipherKey, false);
```

수신자 측
```cpp
VpnHeader crypthdr;
packet->RemoveHeader(crypthdr);
crypthdr.DecryptInput(m_cipherKey, false)
```
#### VPN 헤더의 핵심 멤버(함수 및 변수) 구조
기본값으로 32바이트 길이의 평문을 사용한다고 가정하였고, 직/역직렬화 수행 간 바이트로 변환 및 추출할 `private` 변수들은 각각 `m_sentOrigin`, `m_encrypted` 이므로 `GetSerializedSize`는 지정한 평문의 32바이트 길이의 2배값이 됩니다. 

|지정자|이름|설명|
|:-:|-|-|
|`public`|`EncryptInput`|평문의 암호화 함수|
|`public`|`DecryptInput`|암호문의 복호화 함수|
|`public`|`GetSerializedSize`|직/역직렬화 시 필요 공간 정의 함수|
|`public`|`Serialize`|직렬화 수행 함수|
|`public`|`Deserialize`|역직렬화 수행 함수|
|`public`|`GetSendOrigin`|평문 반환 함수|
|`public`|`GetEncrypted`|암호문 반환 함수|
|`private`|`m_sentOrigin`|암호화 이전 평문 변수|
|`private`|`m_encrypted`|암호문 변수|

또한 직/역직렬화를 수행하는 `Serialize` 함수 및 `Deserialize` 함수 역시 평문의 길이를 조정하게 된다면 해당 길이에 맞춘 수정이 필요합니다.

##### 예시
16바이트 길이의 평문(128비트의 최소길이)로 수정하는 경우
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
