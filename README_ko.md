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

> 내용 추가 필요

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

### IP forwarding

### VPN Header Encrypting/decrypting