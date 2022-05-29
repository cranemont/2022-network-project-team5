# Simple VPN Application

[한글 버전](./README_ko.md)

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

> Content for servers needed

#### Example

```cpp
VPNHelper vpnServer ("10.1.3.2", 1194);
ApplicationContainer app = vpnServer.Install (nodes.Get(2));

app.Start (Seconds (0.));
app.Stop (Seconds (11.));
```

## Tested topology

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

after virtual socket removes header
-------------Payload--------------
| private IP | TCP/UDP | Payload | <- packet given to receive callback
----------------------------------
send to VirtualNetDevice::Recv()

---------------------
| TCP/UDP | Payload |
---------------------

after real socket removes header
-----------
| Payload |
-----------
```

### IP forwarding

### VPN Header Encrypting/decrypting