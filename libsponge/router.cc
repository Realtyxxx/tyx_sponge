#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    DUMMY_CODE(route_prefix, prefix_length, next_hop, interface_num);
    // Your code here.
    // size_t routeI;
    // string prefix_ip = Address::from_ipv4_numeric(route_prefix).ip();
    // for (routeI = 0; routeI < _route_table.size(); ++routeI) {
    //     auto [_route_prefix, _prefix_length, _next_hop, _interface_num] = _route_table[routeI][0];
    //     string this_ip = Address::from_ipv4_numeric(_route_prefix).ip();
    //     uint8_t comp_len = min(prefix_length, _prefix_length);
    //     if (this_ip.substr(0, comp_len) != prefix_ip.substr(0, comp_len)) {
    //         continue;
    //     }
    //     if (prefix_length < _prefix_length) /* 如果这是更先一步的先放前面 */
    //         _route_table[routeI].push_front({route_prefix, prefix_length, next_hop, interface_num});
    //     else
    //         _route_table[routeI].push_back({route_prefix, prefix_length, next_hop, interface_num});
    //     return; /* 能走到这一步的肯定就是能匹配成功了, 大胆点还是直接return */
    // }
    // if (routeI == _route_table.size()) /* 没有一个相似的 */ {
    // _route_table.push_back(deque<tuple<uint32_t, uint8_t, optional<Address>, size_t>>());
    // _route_table[routeI].push_back({route_prefix, prefix_length, next_hop, interface_num});
    // }
    _route_table.push_back({route_prefix, prefix_length, next_hop, interface_num});
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    DUMMY_CODE(dgram);
    // Your code here.
    /*
    string ip = Address::from_ipv4_numeric(dgram.header().dst).ip();
    cout << ip << endl;
    for (size_t i = 0; i < _route_table.size(); ++i) {
        auto [_route_prefix, _prefix_length, _next_hop, _interface_num] = _route_table[i][0];
        string _ip = Address::from_ipv4_numeric(_route_prefix).ip();
        if (ip.substr(0, _prefix_length) == _ip.substr(0, _prefix_length)) {
            uint8_t max_p_len = _prefix_length;
            size_t max_i_n = _interface_num;
            optional<Address> to_next_hop = _next_hop;
            for (size_t j = 1; j < _route_table[i].size(); ++j) {
                auto [_r_p, _p_l, _n_h, _i_n] = _route_table[i][j];
                if (_p_l < max_p_len)
                    continue;
                string this_ip = Address::from_ipv4_numeric(_route_prefix).ip();
                if (ip.substr(0, _p_l) == this_ip.substr(0, _p_l)) {
                    max_p_len = _p_l;
                    max_i_n = _i_n;
                    to_next_hop = _n_h;
                }
            }
            dgram.header().ttl--;
            if (dgram.header().ttl <= 0)
                return;
            interface(max_i_n).send_datagram(dgram, to_next_hop.value());
        }
    }
     */
    uint32_t ip = dgram.header().dst;
    // cout << Address::from_ipv4_numeric(dgram.header().dst).ip() << endl;
    uint8_t max_p_len = 0;
    int32_t ret = -1;
    for (size_t i = 0; i < _route_table.size(); ++i) {
        auto [_r_p, _p_l, _n_h, _i_n] = _route_table[i];
        if (_p_l < max_p_len)
            continue;

        string this_ip = Address::from_ipv4_numeric(_r_p).ip();
        // cout << this_ip << endl;

        if (comp(ip, _r_p, _p_l)) {
            max_p_len = _p_l;
            ret = i;
        }
    }
    if (dgram.header().ttl <= 1)
        return;
    dgram.header().ttl--;
    if (ret < 0)
        return;
    auto [_r_p, _p_l, _n_h, _i_n] = _route_table[ret];
    // if (_n_h.has_value())
    //     cout << "final : " << _n_h.value().ip() << endl;
    interface(_i_n).send_datagram(dgram,
                                  (_n_h.has_value() ? _n_h.value() : Address::from_ipv4_numeric(dgram.header().dst)));
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
/*
bool Router::comp(uint32_t x, uint32_t y, uint8_t len) {
    string s1;
    string s2;
    int i;
    for (i = 1; i <= len; ++i) {
        s1 += bool(x & (1 << (32 - i))) + '0';
        s2 += bool(y & (1 << (32 - i))) + '0';
    }

    cout << "s1: " << s1 << endl;
    cout << "s2: " << s2 << endl;
    return s1 == s2;
}
*/

bool Router::comp(uint32_t x, uint32_t y, uint8_t len) {
    char c1;
    char c2;
    int i;
    for (i = 1; i <= len; ++i) {
        c1 = bool(x & (1 << (32 - i))) + '0';
        c2 = bool(y & (1 << (32 - i))) + '0';
        if (c1 != c2)
            return false;
    }
    return true;
}

// TODO:
// 感觉这个路由表可以用更加复杂的结构构成，如果存储了足够庞大数量的路由表的话，然后跟着自己对于存储他的这个思路感觉到了
// 最终还是觉得先写个for循环查找最长匹配的

// HACK:
// 出现了错误
//[x]: 比较前缀时候竟然直接用长度 + 字符串，还是需要进行特殊处理的比较的
//[x]: 对于unsign 类型的处理不够敏感
//[x]: 当路由表中没有next_hop时候忽然就不知道该怎么处理，实际上就是将目的地址放进去，让它激活arp协议