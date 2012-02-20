/*
 Implementation of the Telnet protocol.

 There's is not much to it, but this implementation
 supports client terminal size information (NAWS).

 Does *not* handle connections or sockets. They need to be
 provided by something else.

 These classes speak of packets. These don't really exist
 in TCP or telnet, but they are used here to refer to 
 chunks sent through the connection. Reason is that we may
 want to later cancel sending the packet, for connection throttling reasons.
 This way we can distinguish from the data we may or may not want to send
 through the connection.
*/

#ifndef telnet_hpp
#define telnet_hpp

#include <map>
#include <string>
#include "types.hpp"

namespace trankesbel {

/* A packet */
class TelnetPacket
{
    private:
        std::string packet_data;
        size_t sent_data;

    public:
        TelnetPacket()
        { sent_data = 0; };
        TelnetPacket(std::string data)
        { packet_data = data; sent_data = 0; };
        TelnetPacket(const void* data, size_t datasize)
        { packet_data = std::string((const char*) data, datasize); sent_data = 0; };

        /* Returns remaining data as a new string. */
        std::string getRemainingData() const
        {
            return packet_data.substr(sent_data);
        }
        ui32 getDataLength() const { return packet_data.size() - sent_data; };

        /* Returns true if there is no more data in this packet to send. */
        bool empty() const
        {
            if (packet_data.size() == 0) return true;
            if (sent_data >= packet_data.size()) return true;
            return false;
        }
        /* Inform this packet class that some bytes were used from
           getRemainingData(). */
        void addUsedBytes(size_t used_bytes)
        {
            sent_data += used_bytes;
            if (sent_data > packet_data.size()) sent_data = packet_data.size();
        }
        /* Returns true if no data have been used from this packet yet. */
        bool isUnTouched() const
        {
            return (sent_data == 0);
        }
};

/* A session for telnet connection. An abstract class.
   You need to derive and implement readRawData and writeRawData. 
   (Typically you'd call something like send/recv on them, but
    read the semantics carefully, they are a little different from them.) */
class TelnetSession
{
    private:
        /* For NAWS. If client terminal is reporting its size, these
           fields will be filled. */
        bool terminal_size_known;
        ui32 terminal_w, terminal_h;

        /* Packets we are sending. The first number is an index number
           and the order in which packets are sent. */
        std::map<ui32, TelnetPacket> packets;
        /* Running index number for packets */
        ui32 packet_index_number;

        /* Receive bufffer */
        std::string recv_buffer;

        /* Is the connection closed? */
        bool closed;
        
        void sendPendingData();

    protected:
        /* Reads data. Called by cycle(). Put the data you read
           to the buffer pointed by 'data'.
           Size parameter is both input and output. You should 
           put the number of bytes you wrote to data buffer to it.
           Return true if connection is still alive, false if the
           connection is terminated. If you just want to signal
           there's no more data at this time, supply 0 for size and
           return true. */
        virtual bool readRawData(void* data, size_t* size) = 0;
        /* Writes data back to connection. Called when changing parameters or
           sending information. Return false, if connection has been
           terminated, true if successful. If data could not be sent at 
           this time, supply 0 for size and return true. */
        virtual bool writeRawData(const void* data, size_t* size) = 0;

    public:
        TelnetSession();
        virtual ~TelnetSession();

        /* Makes the session do handshaking routines.
           In current implementation, this includes setting echo off and
           setting one-character mode. */
        void handShake();

        /* Returns true if session has been closed. */
        bool isClosed() const;

        /* Cycles packages. */
        void cycle();

        /* Sends a packet through connection. 
           The packet should be 'pure', in sense that
           they must be 'untouched', see TelnetPacket::isUnTouched().
           Returns index number that can be used to refer to the
           packet later (if you want to remove it from send queue). */
        ui32 sendPacket(const TelnetPacket &tp);
        ui32 sendPacket(const void* data, size_t datasize)
        { return sendPacket(TelnetPacket(data, datasize)); };

        /* Returns true, if a packet is still cancellable in the queue.
           Returns false, if no such packet or the packet is in process
           to be sent. */
        bool isPacketCancellable(ui32 packet_index) const;
        /* Cancels a packet. Returns what isPacketCancellable would
           return. */
        bool cancelPacket(ui32 packet_index);

        /* Receive data from remote side. Returns false
           if you should not expect more data from this session anymore
           (connection was closed or something). 
           Set the size of your buffer to 'datasize', this will
           be then filled with bytes read from remote side. */
        bool receive(void* data, size_t* datasize);

        /* If client terminal is using NAWS (terminal size information), then
           isTerminalSizeKnown() returns true. The terminal size can then
           be read from following functions. If terminal size is not known,
           the returned size is always 80x24. */
        bool isTerminalSizeKnown() const { return terminal_size_known; };
        ui32 getTerminalWidth() const { return terminal_w; };
        ui32 getTerminalHeight() const { return terminal_h; };
        void getTerminalSize(ui32 *w, ui32 *h) const
        {
            (*w) = terminal_w;
            (*h) = terminal_h;
        }
};

}

#endif

