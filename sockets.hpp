/* Socket support. 
   At the moment we support only listening for connections, because
   trankesbel doesn't need to connect to anywhere. We shall add that
   later if need arises, right? Also TCP-only.
   */

#ifndef sockets_hpp
#define sockets_hpp

#include "cpp_regexes.h"
#include <cstdlib>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <string>
#include <set>
#include <vector>
#include "types.hpp"
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#else
    #ifndef __WIN32
    #define __WIN32
    #endif
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "marshal.hpp"

namespace trankesbel {

/* Some glue to port WIN32/BSD sockets */
#ifndef __WIN32
typedef int SOCKET;
template<class T> inline int closesocket(T a) { return ::close(a); };
const int INVALID_SOCKET = -1;
#endif

enum ProtocolFamily { IPv4, IPv6 };

typedef union sockaddr_max
{
    struct sockaddr sa;
    struct sockaddr_in sa_in;
    struct sockaddr_in6 sa_in6;
    struct sockaddr_storage sa_stor;
} sockaddr_max_t;

class Socket;

/* A socket address that points to somewhere. 
   By default it points to IPv4 address 0.0.0.0 */
class SocketAddress
{
    friend class Socket;

    private:
        /* Versions for IPv4 and IPv6 addresses */
        struct sockaddr_in sa4;
        struct sockaddr_in6 sa6;

        ProtocolFamily ip_protocol;

        std::string hostname;
        boost::recursive_mutex hostname_mutex;

        static void getHumanReadable_thread(void* address_info);
        static void resolve_thread(std::string address_string, std::string service_string, boost::function3<void, bool, SocketAddress, std::string> readysignal);

        SocketAddress(const struct sockaddr_in* sai);
        SocketAddress(const struct sockaddr_in6* sai6);

        std::string getHumanReadablePlainUTF8_both(bool with_port) const;

    public:
        SocketAddress();

        ~SocketAddress();
        SocketAddress& operator=(const SocketAddress &sa);
        SocketAddress(const SocketAddress &sa);

        /* Returns the address in a form that can be saved on disk or over network.
           In unserializing, returns false if address is somehow invalid (and current address
           is not touched). If 'consumed' is not null, puts the number of bytes consumed there. */
        std::string serialize() const;
        bool unSerialize(const std::string &s);

        /* Copies raw IPv4 sockaddr_in structure to sai */
        void getRawIPv4Sockaddr(struct sockaddr_in* sai) const;
        /* Copies raw IPv6 sockaddr_in6 structure to sai6 */
        void getRawIPv6Sockaddr(struct sockaddr_in6* sai) const;

        /* Returns human readable form of the address. The call is asynchronous (unless you set block to true),
           and result is then 'signalled' to the given callback function.
           It creates a thread for this purpose so thread-safety issues are relevant. 
           The first bool argument tells if converting the address was succeful.
           If it's true, the string contains the address in human readable form.
           If it's false, the string contains error message. The second
           string is undefined on error, otherwise it's the service name of
           address (port number). 

           If block is true, the call won't return until it has sent the signal. */
        void getHumanReadable(boost::function3<void, bool, std::string, std::string> humanreadable_signal, bool block);
        /* Returns human readable form of the address. Never blocks and never resolves the address to anything. 
           The address will be in plain IPv4 or IPv6 form. */
        std::string getHumanReadablePlainUTF8() const { return getHumanReadablePlainUTF8_both(true); };
        std::string getHumanReadablePlainUTF8WithoutPort() const { return getHumanReadablePlainUTF8_both(false); };
        

        /* Resolves an internet address to a new class. The call is asynchronous (unless you set block to true).
           Just like getHumanReadable(), same thread issues apply.
           The signal function is called when resolving is ready, with the boolean argument as true
           if resolving was successful and false if it was not. 

           The third argument to ready signal function is the error message, if resolving fails.
           
           It can resolve hostnames and IP addresses, supports both IPv4 and IPv6. If a hostname
           can correspond to many hosts, the first is selected. Service string refers to port number.

           If block is true, the call won't return until it has sent the signal. 
           If you resolve an IP address, the call won't really block but same semantics apply. */
        static void resolve(std::string address_string, std::string service_string, boost::function3<void, bool, SocketAddress, std::string> readysignal, bool block);
        /* Resolves an address and returns a class instance. Will block if it needs to resolve a hostname. 
           The boolean will be set to either true or false, depending on if the call succeeded. */
        static SocketAddress resolveUTF8(std::string address_string, std::string service_string, bool* success, std::string* errormsg);
        /* Resolves the address immediately (i.e. never blocks). If the address is not in IP-address
           form, the call will most likely fail. */
        static SocketAddress resolvePlainUTF8(std::string address_string, std::string service_string, bool* success, std::string* errormsg);

        /* Gets/sets the port this socket address points to. */
        ui16 getPort() const;
        void setPort(ui16 port);

        /* Sets the hostname this address points to. 
           This hostname is used in SocketAddressRange -class to
           match hostnames. You probably want to use reverse resolve
           to get the hostname. 
           
           For convenience, these functions are thread-safe between
           themselves. (I.e. they still have a race condition
           but they won't crash if you set the name in one thread
           and get in other) */ 
        void setHostnameUTF8(const std::string &hostname);
        std::string getHostnameUTF8() const;

        /* Gets the IP family of the socket address. Returns either IPv4 or IPv6. */
        ProtocolFamily getIPProtocol() const;

        /* Operator < allows SocketAddress to be used in associative containers */
        bool operator<(const SocketAddress &sa) const;
        bool operator==(const SocketAddress &sa) const;
        bool operator!=(const SocketAddress &sa) const
        { return !((*this) == sa); };
};

/* Reverse resolves an IP address to hostnames. The call is asynchronous and calls
   the given function when ready. If 'block' is true, blocks until the function
   has been called. The first boolean argument in the callback indicates whether the resolve
   was successful or not. The vector will contain the hostnames. 
   The address will first be converted to a SocketAddress structure with SocketAddress:resolve,
   if you use the string version of this function. */
void reverseResolveUTF8(std::string address, boost::function2<void, bool, std::string> func, bool block);
void reverseResolveUTF8(const SocketAddress &sa, boost::function2<void, bool, std::string> func, bool block);
/* Same as above, but always blocks and returns the addresses instead of calling a callback. 
   If the resolving fails, returns an error message and 'success', if not NULL, is set to false (otherwise true). */
std::string reverseResolveUTF8(std::string address, bool* success);
std::string reverseResolveUTF8(const SocketAddress &sa, bool* success);

class NumberRange
{
    private:
        ui32 start, end;

    public:
        NumberRange()
        {
            /* Default to no range 
               (i.e. no number is in this range) */
            start = 1;
            end = 0;
        }
        NumberRange(ui32 n)
        {
            start = n;
            end = n;
        }
        NumberRange(ui32 s, ui32 e)
        {
            start = s;
            end = e;
        }
        static NumberRange getAllRange()
        {
            return NumberRange(0, 0xffffffff);
        }

        bool isInRange(ui32 n) const { return (n >= start) && (n <= end); };
        bool isNoRange() const { return (start > end); };
        
        ui32 getStart() const { return start; };
        ui32 getEnd() const { return end; };
        void setStart(ui32 s) { start = s; };
        void setEnd(ui32 e) { end = e; };

        void setRange(ui32 s, ui32 e) { start = s; end = e; };
        void getRange(ui32* s, ui32* e) { (*s) = start; (*e) = end; };

        std::string serialize() const
        {
            return marshalUnsignedInteger32(start) + marshalUnsignedInteger32(end);
        }
        bool unSerialize(const std::string &s)
        {
            size_t consumed = 0;
            start = unMarshalUnsignedInteger32(s, consumed, &consumed);
            if (!consumed) return false;
            end = unMarshalUnsignedInteger32(s, consumed, &consumed);
            if (!consumed) return false;

            return true;
        }

        bool operator==(const NumberRange &nr) const
        {
            return (start == nr.start) && (end == nr.end);
        }
        bool operator!=(const NumberRange &nr) const
        { return !( (*this) == nr ); };
};

/* Defines some address set based on IP-addresses
 * or hostnames. Useful for, say, testing addresses against
 * a banned address set. */
class SocketAddressRange
{
    private:
        std::set<SocketAddress> individual_socket_addresses;
        std::vector<NumberRange> IPv4_ranges;  // .size() % 4 == 0
        std::vector<NumberRange> IPv6_ranges;  // .size() % 8 == 0
        std::vector<std::pair<std::string, Regex> > hostname_regexes;


    public:
        /* Clears all rules from the range. After this call,
           no address can match. */
        void clear();

        /* Adds individual socket address to the list. 
           Note that port values in address are ignored. */
        void addSocketAddress(const SocketAddress &sa);
        /* And delets one. Is a NOP if the address was not individually
           added before. */
        void deleteSocketAddress(const SocketAddress &sa);

        /* Adds IPv4 address range to the list.
         * The address that gets used (according to parameters) is:
         *   a.b.c.d
         * NumberRange classes to define numbers.
         * Example that defines address 192.168.0.*:
         * SocketAddressRange sar;
         * sar.addIPv4Range(192, 168, 0, NumberRange(0, 255));
         *
         * If you use invalid numbers, the result is undefined.
         * Don't do that.
         */
        void addIPv4Range(const NumberRange &a,
                          const NumberRange &b,
                          const NumberRange &c,
                          const NumberRange &d);

        /* Adds an IPv6 address range to the list. Works the same way as
           addIPv4Range, except that the form is:
           a:b:c:d:e:f:g:h
        */
        void addIPv6Range(const NumberRange &a,
                          const NumberRange &b,
                          const NumberRange &c,
                          const NumberRange &d,
                          const NumberRange &e,
                          const NumberRange &f,
                          const NumberRange &g,
                          const NumberRange &h);

        /* Adds a hostname regex that will match host address.
           It will try matching directly to IP addresses and hostnames. 
           You need to resolve hostname addresses to SocketAddress
           if you want this to work in the intended way.
           
           e.g.:
           127.0.0.1
           2001:db8:85a3::8a2e:370:7334
           turtlesarefood.com

           */
        void addHostnameRegexUTF8(const std::string &regex_string);
        void addHostnameRegexUTF8(const char* regex_string, size_t regex_string_size)
        { addHostnameRegexUTF8(std::string(regex_string, regex_string_size)); };
        void addHostnameRegex(const UnicodeString &us) { addHostnameRegexUTF8(TO_UTF8(us)); };
        /* Deletes a regex. */
        void deleteHostnameRegexUTF8(const std::string &regex_string);
        void deleteHostnameRegex(const UnicodeString &us) { deleteHostnameRegexUTF8(TO_UTF8(us)); };

        /* Tests an address against this range. Returns true
           if there's a match and false if there is not. 
           The port number in socket address is ignored. */
        bool testAddress(const SocketAddress &sa) const;
        
        /* Adds all individual addresses to the given vector. 
           The vector will be cleared first. */
        void getSocketAddressesToVector(std::vector<SocketAddress> &vec) const;
        /* Adds all IPv4 number ranges to the given vector. 
           The vector will be cleared first.
           The size of the vector will be divisible by 4.
           One IPv4 range is in 4 elements. E.g.:
           
           vector<NumberRange> nr;
           NumberRange a, b, c, d;
           sar.addIPv4Range(a, b, c, d);
           sar.getIPv4RangesToVector(nr);
           nr[0] == a, nr[1] == b, nr[2] == c, nr[3] == d */
        void getIPv4RangesToVector(std::vector<NumberRange> &vec) const;
        /* Same as getIPv4RangesToVector but with IPv6 addresses
           The size of the vector will be divisible by 8. */
        void getIPv6RangesToVector(std::vector<NumberRange> &vec) const;
        /* Gets all hostname regexes and puts them into a vector. 
           The vector will be cleared first. */
        void getHostnameRegexesUTF8ToVector(std::vector<std::string> &str) const;

        /* Serialized the range. */
        std::string serialize() const;
        /* And unserialized it. Returns false, if this was not successful. */
        bool unSerialize(const std::string &s);

        bool operator==(const SocketAddressRange &sar) const;
        bool operator!=(const SocketAddressRange &sar) const
        { return !( (*this) == sar); };
};

/* Socket class. This wraps the most important functions
   of a socket to a C++ class. Supposed to be a portable
   wrapping for Winsock/BSD sockets and alike.
   
   All functions are non-blocking. */
class Socket
{
    private:
        SOCKET socket_desc;
        std::string socket_error;
        SP<SocketAddress> socket_addr;
        bool listening_socket;

        /* Mutex for all operations. Needed when
           connecting asynchronously. */
        boost::recursive_mutex socket_mutex;
        /* Thread for asynchronous connection */
        SP<boost::thread> async_connect_thread;

        /* Thread object when connecting asynchronously. */

        /* Creates and binds the socket descriptor, if it can. */
        bool createSocket(const SocketAddress &sa, bool also_bind);

        /* thread function for asynchronous connections */
        void connectAsynchronous_thread(SocketAddress sa, boost::function1<void, bool> callback_function);

        /* No copies */
        Socket(const Socket &s) { };
        Socket& operator=(const Socket &s) { return (*this); };

        Socket(SOCKET socket_desc_new, const SocketAddress &sa);

    public:
        /* The default constructor & destructor. */
        Socket();
        ~Socket();

        /* Starts an asynchronous request to reverse resolve the
           address of this socket. At some point in future,
           the hostname /may/ be set to the address you get
           from Socket::getAddress().getHostname(). */
        void startAsynchronousReverseResolve();

        /* Returns true if socket is active. */
        bool active();

        /* Bind the socket to given address and start listening. 
           Returns true if listening was successful, false, if it was not.
           Sets error string (Socket::getError()) if there's an error. */
        bool listen(const SocketAddress &sa);

        /* Try to connect to given address. The call blocks. 
           Returns either true or false, depending on success. 
           Error message on failure can be fetched from
           Socket::getError() */
        bool connect(const SocketAddress &sa);
        /* Asynchronous Socket::connect(). 
           Calls the given callback when
           connection is ready, or call otherwise fails.
           The boolean argument tells whether the connection failed or succeded, where
           true means success.
           Error message can be read from Socket::getError().
           The callback will be called from another thread. 

           This call itself always returns true, unless there's already asynchronous connection
           in process, in which case it returns false. */
        bool connectAsynchronous(const SocketAddress &sa, boost::function1<void, bool> callback_function);

        /* Closes the socket down. */
        void close();

        /* Returns the address this socket is bound or connected to. */
        SocketAddress getAddress();

        /* Accepts a connection from a listening socket.
           Returns false, if there are no pending connections at the moment or
           if socket is invalid or not listening. Does not set error string in any case. 
           The socket pointed by 'accept_socket' gets to be the connection socket.
           It has to be a closed socket or this method will always return false. 
           Socket may close if there's an error, you should check with Socket::active(). */
        bool accept(Socket* accept_socket);

        /* Send data through socket. Returns the number of bytes written, which may be less
           than requested amount and even 0.
           If there's an error or connection otherwise closes, socket closes down and you 
           can see that from Socket::active().
           Does not set error string in any case. */
        size_t send(const void* data, size_t datalen);
        size_t send(std::string data) { return send((const void*) data.c_str(), data.size()); };

        /* Receive data from socket. Returns the number of bytes read. Does not block.
           If there's an error or connection otherwise closes, socket closes down and you
           can see that from Socket::actve(). The data is put in the buffer pointed by 'data'. 
           Does not set error string in any case. */
        size_t recv(void* data, size_t datalen);
        std::string recv(size_t datalen);

        /* Returns the raw socket file descriptor. */
        SOCKET getRawSocket();

        /* Returns a human-readable error message. */
        std::string getError();
};

/* Socket events class. Use this to wait for events on a bunch of sockets.
   The purpose of this class is to efficiently scale to handling a lot of sockets. 
   NOT thread-safe. */
class SocketEvents
{
    private:
        #ifndef __WIN32
        int epoll_desc;

        WP<Socket> _getEvent(uint64_t timeout_nanoseconds, bool ignore_forced_events);

        #else
        /* Normal c-style array for easy interfacing with winsock */
        WSAEVENT* event_objects;
        WP<Socket>* event_sockets;
        size_t event_size;
        size_t event_size_allocated;
        #endif

        std::map<SOCKET, WP<Socket> > target_sockets;

        int prune_counter;
        void pruneTargetSockets();

        std::set<WP<Socket> > forced_events;

        /* No copies */
        SocketEvents(const SocketEvents &se) { };
        SocketEvents& operator=(const SocketEvents &se) { return (*this); };

    public:
        SocketEvents();
        ~SocketEvents();

        /* Adds a socket to check for events. */
        void addSocket(WP<Socket> socket);

        /* Makes given socket to immediately generate an event at next getEvent() call. */
        /* Sockets not added with addSocket are just ignored. */
        /* You need to call this in the same thread as with getEvent(), i.e. this call
         * is not thread-safe. */
        void forceEvent(SP<Socket> socket);

        /* Returns a socket where an event occured. Blocks if no events are occuring
         * May indicate socket is now ready for writing or reading or maybe neither. 
         * If there are no sockets in the class, returns null reference. 
         * Also returns null reference if timeout is reached. */
        SP<Socket> getEvent(uint64_t timeout_nanoseconds);
};

void initializeSockets();
void shutdownSockets();

};

#endif

