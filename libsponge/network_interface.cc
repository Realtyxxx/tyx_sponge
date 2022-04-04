#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "network_interface.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    if (_arp_table.count(next_hop.ipv4_numeric()) != 0) {                    /* arp 表里有他, */
        if (_arp_table[next_hop.ipv4_numeric()].first == ETHERNET_BROADCAST) /* 如果是正在广播的 */
            return;
        EthernetFrame frame;
        frame.header().type = EthernetHeader::TYPE_IPv4;
        frame.header().dst = _arp_table[next_hop.ipv4_numeric()].first;
        frame.header().src = _ethernet_address;
        frame.payload().append(dgram.serialize());
        _frames_out.push(frame);
    } else { /* arp 表里没有 */
        // TODO: 直接在arp表里存下他，不过他的地址设置为广播地址，这样他可以和有效的地址一样的去被tick调用检查
        _arp_table[next_hop.ipv4_numeric()] = {ETHERNET_BROADCAST, 0};
        EthernetFrame _arp_frame;
        _arp_frame.header() = {ETHERNET_BROADCAST, _ethernet_address, EthernetHeader::TYPE_ARP};
        // _arp_frame.header().dst = ETHERNET_BROADCAST;
        // _arp_frame.header().src = _ethernet_address;
        // _arp_frame.header().type = EthernetHeader::TYPE_ARP;
        ARPMessage _arpm;
        _arpm.sender_ethernet_address = _ethernet_address;
        _arpm.sender_ip_address = _ip_address.ipv4_numeric();
        _arpm.target_ip_address = next_hop.ipv4_numeric();
        _arpm.target_ethernet_address = {};
        _arpm.opcode = ARPMessage::OPCODE_REQUEST;
        _arp_frame.payload().append(_arpm.serialize());
        _frames_out.push(_arp_frame);
        _waiting.push({next_hop, dgram});
    }
    DUMMY_CODE(dgram, next_hop, next_hop_ip);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    DUMMY_CODE(frame);
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgram;
        if (dgram.parse(frame.payload()) != ParseResult::NoError || frame.header().dst != _ethernet_address) {
            _arp_table[dgram.header().src] = {frame.header().src, 0};
            return {};
        }
        _arp_table[dgram.header().src] = {frame.header().src, 0};
        return dgram;
    } else { /* 是 arp */
        ARPMessage _arpM;
        if (_arpM.parse(frame.payload()) != ParseResult::NoError) {
            return {};
        }
        _arp_table[_arpM.sender_ip_address] = {_arpM.sender_ethernet_address, 0};
        if (_arpM.opcode == ARPMessage::OPCODE_REPLY) {
            // TODO: 完成收到回复后的工作, 拿出没发出的帧发出，更新arp表, 删掉记录正在查找的表段
            queue<pair<Address, InternetDatagram>> tmp;
            while (!_waiting.empty()) {
                auto [ip, dgram] = _waiting.front();
                _waiting.pop();
                if (ip.ipv4_numeric() == _arpM.sender_ip_address) {
                    send_datagram(dgram, ip);
                } else {
                    tmp.push({ip, dgram});
                }
            }
            swap(_waiting, tmp);
        } else {
            // TODO: 完成收到请求后的工作, 如果目标是自身那么回应他，否则丢掉，更新arp表
            if (_ip_address.ipv4_numeric() == _arpM.target_ip_address) { /* 他找的就是我，我需要给他回应 */
                ARPMessage reply;
                reply.sender_ethernet_address = _ethernet_address;
                reply.sender_ip_address = _ip_address.ipv4_numeric();
                reply.target_ethernet_address = _arpM.sender_ethernet_address;
                reply.target_ip_address = _arpM.sender_ip_address;
                reply.opcode = ARPMessage::OPCODE_REPLY;
                EthernetFrame reply_frame;
                reply_frame.header().src = _ethernet_address;
                reply_frame.header().dst = _arpM.sender_ethernet_address;
                reply_frame.header().type = EthernetHeader::TYPE_ARP;
                reply_frame.payload().append(reply.serialize());
                frames_out().push(reply_frame);
            }
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    DUMMY_CODE(ms_since_last_tick);
    /* 处理过期 */
    for (auto it = _arp_table.begin(); it != _arp_table.end();) {
        auto &[e_adr, time] = it->second;
        time += ms_since_last_tick;
        if (e_adr != ETHERNET_BROADCAST) {
            // 删除arp过期条目
            if (time > 30000) {
                it = _arp_table.erase(it);
            } else {
                ++it;
            }
        } else {
            // arp 请求的国旗
            if (time > 5000) {
                time = 0;
                ARPMessage _arpm;
                _arpm.sender_ethernet_address = _ethernet_address;
                _arpm.sender_ip_address = _ip_address.ipv4_numeric();
                _arpm.target_ethernet_address = {};
                _arpm.target_ip_address = it->first;
                _arpm.opcode = ARPMessage::OPCODE_REQUEST;
                EthernetFrame arp_request;
                arp_request.header() = {/* dst */ ETHERNET_BROADCAST,
                                        /* src */ _ethernet_address,
                                        /* type */ EthernetHeader::TYPE_ARP};
                arp_request.payload().append(_arpm.serialize());
                _frames_out.push(arp_request);
                ++it;
            } else {
                ++it;
            }
        }
    }
}
