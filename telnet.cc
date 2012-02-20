#include "telnet.hpp"
#include <cstring>

using namespace trankesbel;
using namespace std;

TelnetSession::TelnetSession()
{
    terminal_size_known = false;
    terminal_w = 80;
    terminal_h = 24;
    packet_index_number = 1;
    closed = false;
}

TelnetSession::~TelnetSession()
{
}

void TelnetSession::cycle()
{
    if (closed) return;

    /* First reading */
    size_t read_size;
    char buf[1000];
    bool result;

    do
    {
        read_size = 1000;
        result = readRawData((void*) buf, &read_size);
        if (read_size > 0)
        {
            /* Process telnet commands */
            /* This code conveniently assumes telnet commands are not
               broken up at packet boundary. */
            ui32 i1;
            for (i1 = 0; i1 < read_size; ++i1)
            {
                // Telnet command?
                if ((unsigned char) buf[i1] == 255 && i1 < read_size-1)
                {
                    if ((unsigned char) buf[i1+1] == 255) // Double IAC == just 255
                    {
                        ++i1;
                        recv_buffer.push_back((char) 255);
                        continue;
                    }
                    else
                    {
                        // option command? then it should be a three-byte sequence that we'll happily ignore
                        if ((unsigned char) buf[i1+1] >= 251 && (unsigned char) buf[i1+1] <= 254 && i1 < read_size-2)
                        {
                            i1 += 2;
                            continue;
                        }
                        else if (i1 < read_size-2)
                        {
                            // Subcommand? We are only performing NAWS
                            if ((unsigned char) buf[i1+1] == 250 && (unsigned char) buf[i1+2] == 31)
                            {
                                ui32 po = i1+3;
                                if (po >= read_size) continue;

                                unsigned char w1 = buf[po++];
                                if (w1 == 255)
                                    ++po;
                                if (po >= read_size) continue;
                                unsigned char w2 = buf[po++];
                                if (w2 == 255)
                                    ++po;
                                if (po >= read_size) continue;
                                unsigned char h1 = buf[po++];
                                if (h1 == 255)
                                    ++po;
                                if (po >= read_size) continue;
                                unsigned char h2 = buf[po++];
                                if (h2 == 255)
                                    ++po;
                                if (po+1 >= read_size) continue;

                                // Next should IAC and SE
                                if ((unsigned char) buf[po] != 255 || (unsigned char) buf[po+1] != 240)
                                    continue;

                                // The numbers are in big endian.
                                short w = (w1 << 8) + w2;
                                short h = (h1 << 8) + h2;
                                
                                terminal_size_known = true;
                                terminal_w = w;
                                terminal_h = h;

                                i1 = po+1;
                                continue;
                            }
                            // Then it's a 2-byte command
                            ++i1;
                            continue;
                        }
                    }
                }
                else
                    recv_buffer.push_back(buf[i1]);
            }
        }
    } while (result && read_size > 0);

    if (!result) { closed = true; return; };

    sendPendingData();
}

void TelnetSession::sendPendingData()
{
    size_t got_bytes = 0;

    do
    {
        /* Collect at least 1000 bytes, unless queue is shorter. */
        string sbuf;
        map<ui32, TelnetPacket>::iterator i1, packets_end = packets.end();
        map<ui32, size_t> used_packets;  /* index to number of bytes used */
        for (i1 = packets.begin(); i1 != packets_end; ++i1)
        {
            sbuf.append(i1->second.getRemainingData());
            used_packets[i1->first] = i1->second.getDataLength();
            if (sbuf.size() >= 1000)
                break;
        }
        if (sbuf.size() == 0) return;

        size_t bufsize = sbuf.size();
        size_t result = writeRawData((void*) sbuf.c_str(), &bufsize);
        got_bytes = bufsize;
        if (!result) closed = true;

        map<ui32, size_t>::iterator i2, used_packets_end = used_packets.end();

        if (bufsize == sbuf.size())
        {
            for (i2 = used_packets.begin(); i2 != used_packets_end; ++i2)
                packets.erase(i2->first);
            return;
        }

        for (i2 = used_packets.begin(); i2 != used_packets_end && bufsize > 0; ++i2)
        {
            if (i2->second <= bufsize)
            {
                packets.erase(i2->first);
                bufsize -= i2->second;
                continue;
            }

            TelnetPacket &tp = packets[i2->first];
            tp.addUsedBytes(bufsize);
            break;
        }

    } while(got_bytes > 0);
}

void TelnetSession::handShake()
{
    /* IAC WILL ECHO - IAC WILL SUPPRESS_GO_AHEAD - IAC WONT LINEMODE - IAC DO NAWS */
    /* and \x1b[2J to clear the terminal. */
    string handshake_material("\xff\xfb\x01\xff\xfb\x03"
                              "\xff\xfc\x22\xff\xfd\x1f"
                              "\x1b[2J");
    sendPacket(TelnetPacket(handshake_material));
}

ui32 TelnetSession::sendPacket(const TelnetPacket &tp)
{
    if (tp.empty() || !tp.isUnTouched()) return 0;
    ui32 old_index = packet_index_number;
    packets[packet_index_number++] = tp;
    if (packet_index_number == 0) packet_index_number = 1;
    return old_index;
}

bool TelnetSession::isPacketCancellable(ui32 packet_index) const
{
    map<ui32, TelnetPacket>::const_iterator i1;
    i1 = packets.find(packet_index);
    if (i1 == packets.end()) return false;

    const TelnetPacket &tp = i1->second;
    if (tp.empty()) return false;
    if (tp.isUnTouched()) return true;

    return false;
}

bool TelnetSession::cancelPacket(ui32 packet_index)
{
    map<ui32, TelnetPacket>::iterator i1;
    i1 = packets.find(packet_index);
    if (i1 == packets.end()) return false;

    const TelnetPacket &tp = i1->second;
    if (tp.empty()) return false;
    if (tp.isUnTouched())
    {
        packets.erase(i1);
        return true;
    }

    return false;
}

bool TelnetSession::receive(void* data, size_t* datasize)
{
    const char* recv_buf_cstr = recv_buffer.c_str();
    size_t read_bytes = min(*datasize, recv_buffer.size());
    (*datasize) = read_bytes;
    memcpy(data, recv_buf_cstr, read_bytes);

    recv_buffer = recv_buffer.substr(read_bytes);
    if (recv_buffer.size() > 0) return true;
    if (closed) return false;
    return true;
}

