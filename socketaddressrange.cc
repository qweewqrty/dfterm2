#include "sockets.hpp"
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include "marshal.hpp"

using namespace trankesbel;
using namespace std;

bool SocketAddressRange::operator==(const SocketAddressRange &sar) const
{
    if (sar.individual_socket_addresses.size() != individual_socket_addresses.size()) return false;

    set<SocketAddress>::const_iterator i1, individual_socket_addresses_end = individual_socket_addresses.end();
    for (i1 = individual_socket_addresses.begin(); i1 != individual_socket_addresses_end; ++i1)
        if (sar.individual_socket_addresses.find(*i1) == sar.individual_socket_addresses.end()) 
            return false;

    size_t i2, i3, ipv4_size = IPv4_ranges.size();
    size_t sar_ipv4_size = sar.IPv4_ranges.size();

    if (ipv4_size != sar_ipv4_size) return false;

    assert( !(ipv4_size % 4) );
    assert( !(sar_ipv4_size % 4) );

    for (i2 = 0; i2 < ipv4_size; i2 += 4)
    {
        bool index_ok = false;

        for (i3 = 0; i3 < sar_ipv4_size; i3 += 4)
        {
            if (IPv4_ranges[i2] == sar.IPv4_ranges[i3] &&
                IPv4_ranges[i2+1] == sar.IPv4_ranges[i3+1] &&
                IPv4_ranges[i2+2] == sar.IPv4_ranges[i3+2] &&
                IPv4_ranges[i2+3] == sar.IPv4_ranges[i3+3])
            {
                index_ok = true;
                break;
            }
        }

        if (!index_ok) return false;
    }

    size_t ipv6_size = IPv6_ranges.size();
    size_t sar_ipv6_size = sar.IPv6_ranges.size();

    if (ipv6_size != sar_ipv6_size) return false;

    assert( !(ipv6_size % 8) );
    assert( !(sar_ipv6_size % 8) );

    for (i2 = 0; i2 < ipv6_size; i2 += 8)
    {
        bool index_ok = false;

        for (i3 = 0; i3 < sar_ipv6_size; i3 += 8)
        {
            if (IPv6_ranges[i2] == sar.IPv6_ranges[i3] &&
                IPv6_ranges[i2+1] == sar.IPv6_ranges[i3+1] &&
                IPv6_ranges[i2+2] == sar.IPv6_ranges[i3+2] &&
                IPv6_ranges[i2+3] == sar.IPv6_ranges[i3+3] &&
                IPv6_ranges[i2+4] == sar.IPv6_ranges[i3+4] &&
                IPv6_ranges[i2+5] == sar.IPv6_ranges[i3+5] &&
                IPv6_ranges[i2+6] == sar.IPv6_ranges[i3+6] &&
                IPv6_ranges[i2+7] == sar.IPv6_ranges[i3+7])
            {
                index_ok = true;
                break;
            }
        }

        if (!index_ok) return false;
    }

    size_t host_size = hostname_regexes.size();
    size_t sar_host_size = sar.hostname_regexes.size();
    if (host_size != sar_host_size) return false;

    for (i2 = 0; i2 < host_size; ++i2)
    {
        bool index_ok = false;

        for (i3 = 0; i3 < sar_host_size; ++i3)
            if (hostname_regexes[i2].first == sar.hostname_regexes[i3].first)
            {
                index_ok = true;
                break;
            }

        if (!index_ok) return false;
    }

    return true;
}

string SocketAddressRange::serialize() const
{
    /* When we serialize, we write the size
       of next item stack in a 64-bit integer, then
       serialize the items and stack them after it. */

    /* 1. Individual socket addresses
       2. IPv4 ranges
       3. IPv6 ranges
       4. Hostnames */

    stringstream ss;

    size_t individual_socket_addresses_size = individual_socket_addresses.size();
    ss << marshalUnsignedInteger64(individual_socket_addresses_size);

    set<SocketAddress>::iterator i1, individual_socket_addresses_end = individual_socket_addresses.end();
    for (i1 = individual_socket_addresses.begin(); i1 != individual_socket_addresses_end; ++i1)
    {
        std::string serialized_address = (*i1).serialize();
        ss << marshalUnsignedInteger64(serialized_address.size()) << serialized_address;
    }

    ss << marshalUnsignedInteger64(IPv4_ranges.size());
    size_t i2, IPv4_ranges_size = IPv4_ranges.size();
    for (i2 = 0; i2 < IPv4_ranges_size; ++i2)
    {
        string serialized = IPv4_ranges[i2].serialize();
        ss << marshalUnsignedInteger64(serialized.size()) << serialized;
    }

    ss << marshalUnsignedInteger64(IPv6_ranges.size());
    size_t IPv6_ranges_size = IPv6_ranges.size();
    for (i2 = 0; i2 < IPv6_ranges_size; ++i2)
    {
        string serialized = IPv6_ranges[i2].serialize();
        ss << marshalUnsignedInteger64(serialized.size()) << serialized;
    }

    ss << marshalUnsignedInteger64(hostname_regexes.size());
    size_t hostname_regexes_size = hostname_regexes.size();
    for (i2 = 0; i2 < hostname_regexes_size; ++i2)
        ss << marshalString(hostname_regexes[i2].first);

    return ss.str();
}

bool SocketAddressRange::unSerialize(const std::string &s)
{
    clear();

    size_t offset = 0;
    size_t consumed = 0;

    size_t individual_socket_addresses_size = unMarshalUnsignedInteger64(s, offset, &consumed);
    if (!consumed) return false;
    offset += consumed;

    for (size_t i1 = 0; i1 < individual_socket_addresses_size; ++i1)
    {
        size_t address_size = unMarshalUnsignedInteger64(s, offset, &consumed);
        if (!consumed) return false;
        offset += consumed;

        SocketAddress sa;
        bool result = sa.unSerialize(s.substr(offset, address_size));
        if (!result) return false;
        offset += address_size;

        individual_socket_addresses.insert(sa);
    }

    size_t ipv4_range_size = unMarshalUnsignedInteger64(s, offset, &consumed);
    if (!consumed) return false;
    offset += consumed;

    if (ipv4_range_size % 4) return false;

    IPv4_ranges.reserve(ipv4_range_size);
    for (size_t i1 = 0; i1 < ipv4_range_size; ++i1)
    {
        size_t range_size = unMarshalUnsignedInteger64(s, offset, &consumed);
        if (!consumed) return false;
        offset += consumed;

        NumberRange nr;
        if (!nr.unSerialize(s.substr(offset, range_size))) return false;
        offset += range_size;

        IPv4_ranges.push_back(nr);
    }

    size_t ipv6_range_size = unMarshalUnsignedInteger64(s, offset, &consumed);
    if (!consumed) return false;
    offset += consumed;

    if (ipv6_range_size % 8) return false;

    IPv6_ranges.reserve(ipv6_range_size);
    for (size_t i1 = 0; i1 < ipv6_range_size; ++i1)
    {
        size_t range_size = unMarshalUnsignedInteger64(s, offset, &consumed);
        if (!consumed) return false;
        offset += consumed;

        NumberRange nr;
        if (!nr.unSerialize(s.substr(offset, range_size))) return false;
        offset += range_size;

        IPv6_ranges.push_back(nr);
    }

    size_t hostname_regexes_size = unMarshalUnsignedInteger64(s, offset, &consumed);
    if (!consumed) return false;
    offset += consumed;

    hostname_regexes.reserve(hostname_regexes_size);
    for (size_t i1 = 0; i1 < hostname_regexes_size; ++i1)
    {
        string sa = unMarshalString(s, offset, &consumed);
        if (!consumed) return false;
        offset += consumed;

        addHostnameRegexUTF8(sa);
    }

    return true;
}

void SocketAddressRange::clear()
{
    individual_socket_addresses.clear();
    IPv4_ranges.clear();
    IPv6_ranges.clear();
    hostname_regexes.clear();
}

void SocketAddressRange::addSocketAddress(const SocketAddress &sa)
{
    SocketAddress sa_copy = sa;
    sa_copy.setPort(0);
    sa_copy.setHostnameUTF8("");
    individual_socket_addresses.insert(sa_copy);
}


void SocketAddressRange::deleteSocketAddress(const SocketAddress &sa)
{
    SocketAddress sa_copy = sa;
    sa_copy.setPort(0);
    sa_copy.setHostnameUTF8("");
    individual_socket_addresses.erase(sa_copy);
}

void SocketAddressRange::addIPv4Range(const NumberRange &a,
                                      const NumberRange &b,
                                      const NumberRange &c,
                                      const NumberRange &d)
{

/* I think there really should be a private struct for specifying IPv4 and IPv6
   address ranges respectively. But I can't be bothered, right, right?? */

    IPv4_ranges.push_back(a);
    IPv4_ranges.push_back(b);
    IPv4_ranges.push_back(c);
    IPv4_ranges.push_back(d);
}

void SocketAddressRange::addIPv6Range(const NumberRange &a,
                                      const NumberRange &b,
                                      const NumberRange &c,
                                      const NumberRange &d,
                                      const NumberRange &e,
                                      const NumberRange &f,
                                      const NumberRange &g,
                                      const NumberRange &h)
                                      
{
    IPv6_ranges.push_back(a);
    IPv6_ranges.push_back(b);
    IPv6_ranges.push_back(c);
    IPv6_ranges.push_back(d);
    IPv6_ranges.push_back(e);
    IPv6_ranges.push_back(f);
    IPv6_ranges.push_back(g);
    IPv6_ranges.push_back(h);
}

void SocketAddressRange::addHostnameRegexUTF8(const std::string &regex_string)
{
    pair<string, Regex> p(regex_string, Regex(regex_string));
    if (p.second.isInvalid()) return;

    hostname_regexes.push_back(p);
}

void SocketAddressRange::deleteHostnameRegexUTF8(const std::string &regex_string)
{
    vector<pair<string, Regex> >::iterator i1, hostname_regexes_end = hostname_regexes.end();
    for (i1 = hostname_regexes.begin(); i1 != hostname_regexes_end; ++i1)
    {
        if (i1->first != regex_string)
            continue;

        hostname_regexes.erase(i1);
        return;
    }
}

bool SocketAddressRange::testAddress(const SocketAddress &sa) const
{
    SocketAddress sa_copy = sa;
    sa_copy.setPort(0);
    sa_copy.setHostnameUTF8("");

    set<SocketAddress>::const_iterator i1 = individual_socket_addresses.find(sa_copy);
    if (i1 != individual_socket_addresses.end())
        return true;

    switch (sa.getIPProtocol())
    {
        case IPv4:
        {
            struct sockaddr_in sai;
            sa.getRawIPv4Sockaddr(&sai);

            ui32 addr = ntohl(sai.sin_addr.s_addr);
            ui32 a = (addr & 0xff000000) >> 24;
            ui32 b = (addr & 0x00ff0000) >> 16;
            ui32 c = (addr & 0x0000ff00) >> 8;
            ui32 d = (addr & 0x000000ff);

            ui32 i1, len = IPv4_ranges.size();
            assert( (len % 4) == 0);

            for (i1 = 0; i1 < len; i1 += 4)
            {
                if (!IPv4_ranges[i1].isInRange(a)) continue;
                if (!IPv4_ranges[i1+1].isInRange(b)) continue;
                if (!IPv4_ranges[i1+2].isInRange(c)) continue;
                if (!IPv4_ranges[i1+3].isInRange(d)) continue;

                return true;
            }
        }
        break;
        case IPv6:
        {
            struct sockaddr_in6 sai6;
            sa.getRawIPv6Sockaddr(&sai6);

            ui16 a, b, c, d, e, f, g, h;
            
            const char* buf = (const char*) &sai6.sin6_addr;

            a = ((ui16) buf[0] << 8) + buf[1];
            b = ((ui16) buf[2] << 8) + buf[3];
            c = ((ui16) buf[4] << 8) + buf[5];
            d = ((ui16) buf[6] << 8) + buf[7];
            e = ((ui16) buf[8] << 8) + buf[9];
            f = ((ui16) buf[10] << 8) + buf[11];
            g = ((ui16) buf[12] << 8) + buf[13];
            h = ((ui16) buf[14] << 8) + buf[15];

            ui32 i1, len = IPv6_ranges.size();
            assert( (len % 8) == 0);

            for (i1 = 0; i1 < len; i1 += 8)
            {
                if (!IPv6_ranges[i1].isInRange(a)) continue;
                if (!IPv6_ranges[i1+1].isInRange(b)) continue;
                if (!IPv6_ranges[i1+2].isInRange(c)) continue;
                if (!IPv6_ranges[i1+3].isInRange(d)) continue;
                if (!IPv6_ranges[i1+4].isInRange(e)) continue;
                if (!IPv6_ranges[i1+5].isInRange(f)) continue;
                if (!IPv6_ranges[i1+6].isInRange(g)) continue;
                if (!IPv6_ranges[i1+7].isInRange(h)) continue;

                return true;
            }
        }
        break;
        default:
        assert(false);
        break;
    }

    /* Finally, test hostnames. */
    string hostname = sa.getHostnameUTF8();
    string address_name = sa.getHumanReadablePlainUTF8WithoutPort();
    vector<pair<string, Regex> >::const_iterator i2, hostname_regexes_end = hostname_regexes.end();
    for (i2 = hostname_regexes.begin(); i2 != hostname_regexes_end; ++i2)
    {
        Regex r = i2->second;
        if (r.execute(hostname))
            return true;
        if (r.execute(address_name))
            return true;
    }

    return false;
};

void SocketAddressRange::getSocketAddressesToVector(vector<SocketAddress> &vec) const
{
    vec.clear();
    vec.reserve(individual_socket_addresses.size());
    
    set<SocketAddress>::const_iterator i1, individual_socket_addresses_end = individual_socket_addresses.end();
    for (i1 = individual_socket_addresses.begin(); i1 != individual_socket_addresses_end; ++i1)
        vec.push_back(*i1);
}

void SocketAddressRange::getIPv4RangesToVector(vector<NumberRange> &vec) const
{
    vec = IPv4_ranges;
}

void SocketAddressRange::getIPv6RangesToVector(vector<NumberRange> &vec) const
{
    vec = IPv6_ranges;
}

void SocketAddressRange::getHostnameRegexesUTF8ToVector(vector<string> &str) const
{
    str.clear();
    str.reserve(hostname_regexes.size());
    
    vector<pair<string, Regex> >::const_iterator i1, hostname_regexes_end = hostname_regexes.end();
    for (i1 = hostname_regexes.begin(); i1 != hostname_regexes_end; ++i1)
        str.push_back(i1->first);
}

