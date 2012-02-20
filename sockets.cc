#define _WIN32_WINNT 0x0501

#include "sockets.hpp"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include "types.hpp"
#include <boost/thread/recursive_mutex.hpp>
#include "unicode/unistr.h"

#ifdef _WIN32
    #ifndef __WIN32
    #define __WIN32
    #endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#endif

using namespace trankesbel;
using namespace std;
using namespace boost;

#ifdef _WIN32
static bool winsock_initialized = false;
static recursive_mutex winsock_initialized_mutex;

static void second_wait(int seconds)
{
    Sleep(seconds * 1000);
}

static void test_initialize_winsock()
{
    lock_guard<recursive_mutex> lock(winsock_initialized_mutex);

    if (!winsock_initialized)
    {
        WSAData wd;
        if (WSAStartup(MAKEWORD(2, 2), &wd) != 0)
        {
            cerr << "WinSock initialization failed!" << endl;
            terminate();
            return;
        }
        winsock_initialized = true;
    }
}

static void test_winsock_shutdown()
{
    lock_guard<recursive_mutex> lock(winsock_initialized_mutex);

    if (winsock_initialized)
    {
        WSACleanup();
        winsock_initialized = false;
    }
}
#else
static void second_wait(int seconds)
{
    sleep(seconds);
}
static void test_winsock_shutdown() { };
static void test_initialize_winsock() { };
#endif

SocketAddress::SocketAddress()
{
    ip_protocol = IPv4;
    memset(&sa4, 0, sizeof(sa4));
    memset(&sa6, 0, sizeof(sa6));

    sa4.sin_family = PF_INET;
    sa6.sin6_family = PF_INET6;
}

SocketAddress::~SocketAddress()
{
    lock_guard<recursive_mutex> hostname_lock(hostname_mutex);
}

SocketAddress::SocketAddress(const SocketAddress &sa)
{
    memcpy(&sa4, &sa.sa4, sizeof(sa4));
    memcpy(&sa6, &sa.sa6, sizeof(sa6));
    ip_protocol = sa.ip_protocol;

    SocketAddress* foreign = const_cast<SocketAddress*>(&sa);

    lock_guard<recursive_mutex> hostname_lock(foreign->hostname_mutex);
    hostname = sa.hostname;
}

SocketAddress& SocketAddress::operator=(const SocketAddress &sa)
{
    if (this == &sa) return (*this);

    memcpy(&sa4, &sa.sa4, sizeof(sa4));
    memcpy(&sa6, &sa.sa6, sizeof(sa6));
    ip_protocol = sa.ip_protocol;

    SocketAddress* foreign = const_cast<SocketAddress*>(&sa);

    lock_guard<recursive_mutex> hostname_own_lock(hostname_mutex);
    lock_guard<recursive_mutex> hostname_lock(foreign->hostname_mutex);
    hostname = sa.hostname;

    return (*this);
}

SocketAddress::SocketAddress(const struct sockaddr_in* sai)
{
    ip_protocol = IPv4;
    memcpy(&sa4, sai, sizeof(sa4));
    memset(&sa6, 0, sizeof(sa6));

    sa4.sin_family = PF_INET;
    sa6.sin6_family = PF_INET6;
}

SocketAddress::SocketAddress(const struct sockaddr_in6* sai6)
{
    ip_protocol = IPv6;
    memset(&sa4, 0, sizeof(sa4));
    memcpy(&sa6, sai6, sizeof(sa6));

    sa4.sin_family = PF_INET;
    sa6.sin6_family = PF_INET6;
}

bool SocketAddress::operator==(const SocketAddress &sa) const
{
    if (ip_protocol != sa.ip_protocol) return false;

    if (ip_protocol == IPv4)
        if (memcmp(&sa.sa4, &sa4, sizeof(sa4))) 
            return false;
    if (ip_protocol == IPv6)
        if (memcmp(&sa.sa6, &sa6, sizeof(sa6)))
            return false;

    if (sa.hostname != hostname)
        return false;

    return true;
}

bool SocketAddress::operator<(const SocketAddress &sa) const
{
    if (ip_protocol == IPv4 && sa.ip_protocol == IPv6) return true;
    if (ip_protocol == IPv6 && sa.ip_protocol == IPv4) return false;

    char* b1, *b2;
    size_t b_size;
    if (ip_protocol == IPv4)
    {
        b1 = (char*) &sa4;
        b2 = (char*) &sa.sa4;
        b_size = sizeof(sa4);
    }
    else
    {
        b1 = (char*) &sa6;
        b2 = (char*) &sa.sa6;
        b_size = sizeof(sa6);
    }

    size_t i1;
    for (i1 = 0; i1 < b_size; ++i1)
    {
        if (b1[i1] == b2[i1]) continue;

        if (b1[i1] < b2[i1]) return true;

        return false;
    }

    return hostname < sa.hostname;
}

string SocketAddress::serialize() const
{
    stringstream ss;

    /* First byte is either 4 or 6, for IPvX versions */
    if (ip_protocol == IPv4)
    {
        ss << "4";
        ui16 p = getPort();
        ss << (unsigned char) (p >> 8);
        ss << (unsigned char) (p & 0x00ff);

        ui32 address = ntohl(sa4.sin_addr.s_addr);
        ss << (unsigned char) ( (address & 0xff000000) >> 24 );
        ss << (unsigned char) ( (address & 0x00ff0000) >> 16 );
        ss << (unsigned char) ( (address & 0x0000ff00) >> 8 );
        ss << (unsigned char) ( (address & 0x000000ff) );

        ss << hostname;

        return ss.str(); 
    }

    ss << "6";
    ui16 p = getPort();
    ss << (unsigned char) (p >> 8);
    ss << (unsigned char) (p & 0x00ff);

    const char* buf = (const char*) &sa6.sin6_addr;
    for (ui32 i1 = 0; i1 < 16; i1++)
        ss << buf[i1];

    ss << hostname;

    return ss.str();
}

bool SocketAddress::unSerialize(const std::string &s)
{
    const char* c = s.c_str();
    size_t len = s.size();

    if (c[0] == '4') /* IPv4 address */
    {
        if (len < 7) return false; /* Invalid data */

        ip_protocol = IPv4;

        ui16 p = 0;
        p = (ui16) c[1] << 8;
        p |= (ui16) c[2];

        ui32 address = 0;
        address = (ui32) c[3] << 24;
        address |= (ui32) c[4] << 16;
        address |= (ui32) c[5] << 8;
        address |= (ui32) c[6];

        setPort(p);
        sa4.sin_addr.s_addr = htonl(address);

        hostname = s.substr(7);

        memset(&sa6, 0, sizeof(sa6));
        return true;
    }
    if (c[0] == '6') /* IPv6 address */
    {
        if (len < 19) return false; /* invalid data */
          
        ip_protocol = IPv6;

        ui16 p = 0;
        p = (ui16) c[1] << 8;
        p |= (ui16) c[2];
        
        setPort(p);
        
        for (ui32 i1 = 3; i1 < 19; i1++)
            ((char*) &sa6.sin6_addr)[i1-3] = c[i1];
        
        hostname = s.substr(19);
        memset(&sa4, 0, sizeof(sa4));
        return true;
    }

    return false;
}

struct address_info_s
{
   struct sockaddr_in sa4;
   struct sockaddr_in6 sa6;
   function3<void, bool, string, string> signal_function;
   ProtocolFamily fam;

   address_info_s() { };
};

#ifdef _WIN32
static int getSocketError() { return WSAGetLastError(); };
static int getEWOULDBLOCK() { return WSAEWOULDBLOCK; };
static void resetSocketError() { WSASetLastError(0); };

static string getErrorStringSocketsError(int error_code)
{
    LPWSTR buffer_string = (LPWSTR) 0;
    if (!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, NULL, error_code, 0, (LPWSTR) &buffer_string, 1, 0))
        return string("");
    
    if (!buffer_string)
        return string("");

    return TO_UTF8(buffer_string);
#else
static int getSocketError() { return errno; };
static int getEWOULDBLOCK() { return EWOULDBLOCK; };
static void resetSocketError() { errno = 0; };

static string getErrorStringSocketsError(int error_code)
{
    char node_buf[501];
    node_buf[500] = 0;
    node_buf[0] = 0;
#ifdef _GNU_SOURCE
    /* GNU strerror_r */
    return strerror_r(error_code, node_buf, 500);
#else
    strerror_r(error_code, node_buf, 500);
    return string(node_buf);
#endif

#endif
}

static string getErrorStringEAI(int errorcode)
{
    char node_buf[501];
    node_buf[500] = 0;

    switch(errorcode)
    {
        case EAI_AGAIN:
            return "Could not resolve the name before timeout. (EAI_AGAIN)";
        break;
        case EAI_BADFLAGS:
            return "Bad flags. (EAI_BADFLAGS)";
        break;
        case EAI_FAIL:
            return "Non-recoverable error occured. (EAI_FAIL)";
        break;
        case EAI_FAMILY:
            return "Unknown address family. (EAI_FAMILY)";
        break;
        case EAI_MEMORY:
            return "Memory allocation failure. (EAI_MEMORY)";
        break;
        case EAI_NONAME:
            return "Name doesn't resolve to asked format. (EAI_NONAME)";
        break;
#ifndef _WIN32
        case EAI_OVERFLOW:
            return "Argument overflow. (EAI_OVERFLOW)";
        break;
        case EAI_SYSTEM:
            return getErrorStringSocketsError(errno);
            node_buf[0] = 0;
#ifdef _GNU_SOURCE
            return strerror_r(errno, node_buf, 500);
#else
            strerror_r(errno, node_buf, 500);
            return string(node_buf);
#endif
        break;
#endif
        default:
#ifdef _WIN32
            return getErrorStringSocketsError(WSAGetLastError());
#else
            return "Unknown error.";
#endif
        break;
    };
}

void SocketAddress::getHumanReadable_thread(void* address_info)
{
    SP<struct address_info_s> ais((struct address_info_s*) address_info);
    char node_buf[501];
    char service_buf[501];
    /* It should not be possible to reach 500th byte with getnameinfo(),
       but I felt like having some insurance. */
    node_buf[500] = 0;
    service_buf[500] = 0;
    int result;

    int max_wait = 20;
    do
    {
        if (ais->fam == IPv4)
            result = getnameinfo((struct sockaddr*) &ais->sa4, sizeof(ais->sa4), node_buf, 500, service_buf, 500, NI_NUMERICHOST|NI_NUMERICSERV);
        else
            result = getnameinfo((struct sockaddr*) &ais->sa6, sizeof(ais->sa6), node_buf, 500, service_buf, 500, NI_NUMERICHOST|NI_NUMERICSERV);

        if (result == EAI_AGAIN)
            second_wait(1);
        else
            break;
    }
    while( (max_wait--) > 0);

    if (!result)
    {
        ais->signal_function(true, string(node_buf), string(service_buf));
        return;
    }

    /* gai_strerror is not thread-safe and we don't want to be windows-specific,
       therefore, we must use our own error messages. */
    ais->signal_function(false, getErrorStringEAI(result), "");
}

string SocketAddress::getHumanReadablePlainUTF8_both(bool with_port) const
{
    char node_buf[501];
    char service_buf[501];
    node_buf[500] = 0;
    service_buf[500] = 0;
    int result;

    address_info_s ais;
    memcpy(&ais.sa4, &sa4, sizeof(sa4));
    memcpy(&ais.sa6, &sa6, sizeof(sa6));
    ais.fam = ip_protocol;

    if (ais.fam == IPv4)
        result = getnameinfo((struct sockaddr*) &ais.sa4, sizeof(ais.sa4), node_buf, 500, service_buf, 500, NI_NUMERICHOST|NI_NUMERICSERV);
    else
        result = getnameinfo((struct sockaddr*) &ais.sa6, sizeof(ais.sa6), node_buf, 500, service_buf, 500, NI_NUMERICHOST|NI_NUMERICSERV);
    if (result != 0) return string("");

    if (with_port)
    {
        if (ais.fam == IPv4)
        {
            stringstream ss;
            ss << node_buf << ":" << service_buf;
            return ss.str();
        }
        else
        {
            stringstream ss;
            ss << "[" << node_buf << "]:" << service_buf;
            return ss.str();
        }
    }

    return string(node_buf);
}

void SocketAddress::getHumanReadable(function3<void, bool, string, string> humanreadable_signal, bool block)
{
    if (!humanreadable_signal) return;

    address_info_s* ais = new address_info_s;
    memset(ais, 0, sizeof(*ais));

    memcpy(&ais->sa4, &sa4, sizeof(sa4));
    memcpy(&ais->sa6, &sa6, sizeof(sa6));
    ais->signal_function = humanreadable_signal;
    ais->fam = ip_protocol;

    thread thread_lookup(getHumanReadable_thread, (void*) ais);
    if (block)
        thread_lookup.join();
    else
        thread_lookup.detach();
}

void SocketAddress::resolve_thread(string address_string, string service_string, function3<void, bool, SocketAddress, string> readysignal)
{
    struct addrinfo *ai = (struct addrinfo*) 0;
    int result = getaddrinfo(address_string.c_str(), service_string.c_str(), NULL, &ai);
    if (result)
    {
        if (ai) freeaddrinfo(ai);
        readysignal(false, SocketAddress(), getErrorStringEAI(result));
        return;
    }

    struct addrinfo *i1;
    for (i1 = ai; i1; i1 = i1->ai_next)
    {
        if (i1->ai_family != PF_INET &&
            i1->ai_family != PF_INET6)
            continue;

        if (i1->ai_addr)
        {
            if (i1->ai_family == PF_INET)
            {
                SocketAddress sa;
                sa.ip_protocol = IPv4;
                memcpy(&sa.sa4, i1->ai_addr, sizeof(sa.sa4));
                sa.sa4.sin_family = PF_INET;
                readysignal(true, sa, "");
                freeaddrinfo(ai);
                return;
            }
            if (i1->ai_family == PF_INET6)
            {
                SocketAddress sa;
                sa.ip_protocol = IPv6;
                memcpy(&sa.sa6, i1->ai_addr, sizeof(sa.sa6));
                sa.sa6.sin6_family = PF_INET6;
                readysignal(true, sa, "");
                freeaddrinfo(ai);
                return;
            }
        }
    };

    freeaddrinfo(ai);
    readysignal(false, SocketAddress(), "Could not find supported address for asked name.");
}

static void resolve_callback(bool success, SocketAddress sa, string errormsg, bool* success2, SocketAddress* sa2, string* errormsg2)
{
    (*success2) = success;
    (*errormsg2) = errormsg;
    (*sa2) = sa;
}

SocketAddress SocketAddress::resolvePlainUTF8(std::string address_string, std::string service_string, bool* success, std::string* errormsg)
{
    struct addrinfo *ai = (struct addrinfo*) 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICHOST;

    int result = getaddrinfo(address_string.c_str(), service_string.c_str(), &hints, &ai);
    if (result)
    {
        if (ai) freeaddrinfo(ai);
        if (success) (*success) = false;
        if (errormsg) (*errormsg) = getErrorStringEAI(result);

        return SocketAddress();
    }

    struct addrinfo *i1;
    for (i1 = ai; i1; i1 = i1->ai_next)
    {
        if (i1->ai_family != PF_INET &&
            i1->ai_family != PF_INET6)
            continue;

        if (i1->ai_addr)
        {
            if (i1->ai_family == PF_INET)
            {
                SocketAddress sa;
                sa.ip_protocol = IPv4;
                memcpy(&sa.sa4, i1->ai_addr, sizeof(sa.sa4));
                sa.sa4.sin_family = PF_INET;
                if (success) (*success) = true;

                freeaddrinfo(ai);
                return sa;
            }
            if (i1->ai_family == PF_INET6)
            {
                SocketAddress sa;
                sa.ip_protocol = IPv6;
                memcpy(&sa.sa6, i1->ai_addr, sizeof(sa.sa6));
                sa.sa6.sin6_family = PF_INET6;
                if (success) (*success) = true;

                freeaddrinfo(ai);
                return sa;
            }
        }
    };

    freeaddrinfo(ai);

    if (success) (*success) = false;
    if (errormsg) (*errormsg) = "Could not find supported address for asked name.";

    return SocketAddress();
}

SocketAddress SocketAddress::resolveUTF8(string address_string, string service_string, bool* success2, string* errormsg2)
{
    string result_str;
    bool success = false;
    SocketAddress sa;

    function3<void, bool, SocketAddress, string> callback;
    callback = boost::bind(resolve_callback, _1, _2, _3, &success, &sa, &result_str);

    resolve(address_string, service_string, callback, true);

    if (errormsg2) (*errormsg2) = result_str;
    if (success2) (*success2) = success;

    return sa;
}

void SocketAddress::resolve(string address_string, string service_string, function3<void, bool, SocketAddress, string> readysignal, bool block)
{
    thread socketaddress_creation(resolve_thread, address_string, service_string, readysignal);
    if (block)
        socketaddress_creation.join();
    else
        socketaddress_creation.detach();
}

ui16 SocketAddress::getPort() const
{
    if (ip_protocol == IPv4)
        return ntohs(sa4.sin_port);
    else
        return ntohs(sa6.sin6_port);
}

void SocketAddress::setPort(ui16 port)
{
    sa4.sin_port = htons(port);
    sa6.sin6_port = htons(port);
}

void SocketAddress::setHostnameUTF8(const string &hostname)
{
    lock_guard<recursive_mutex> hostname_lock(hostname_mutex);
    this->hostname = hostname;
}

string SocketAddress::getHostnameUTF8() const
{
    unique_lock<recursive_mutex> hostname_lock(*const_cast<recursive_mutex*>(&hostname_mutex));
    string h = hostname;
    hostname_lock.unlock();

    return h;
}

ProtocolFamily SocketAddress::getIPProtocol() const
{
    return ip_protocol;
}

void SocketAddress::getRawIPv4Sockaddr(struct sockaddr_in *sai) const
{
    memcpy(sai, &sa4, sizeof(sa4));
}

void SocketAddress::getRawIPv6Sockaddr(struct sockaddr_in6 *sai) const
{
    memcpy(sai, &sa6, sizeof(sa6));
}

Socket::Socket()
{
    socket_desc = INVALID_SOCKET;
    listening_socket = false;
}

Socket::~Socket()
{
    unique_lock<recursive_mutex> lock(socket_mutex);
    close();
    lock.unlock();
}

Socket::Socket(SOCKET new_socket_desc, const SocketAddress &sa)
{
    socket_desc = new_socket_desc;
    listening_socket = false;
    socket_addr = SP<SocketAddress>(new SocketAddress(sa));
}

SOCKET Socket::getRawSocket()
{
    lock_guard<recursive_mutex> lock(socket_mutex);
    return socket_desc;
}

bool Socket::createSocket(const SocketAddress &sa, bool also_bind)
{
    lock_guard<recursive_mutex> lock(socket_mutex);
    socket_desc = INVALID_SOCKET;
    if (sa.ip_protocol == IPv4)
        socket_desc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    else
        socket_desc = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (socket_desc == INVALID_SOCKET)
    {
        socket_error = string("socket() failure. ") + getErrorStringSocketsError(getSocketError());
        return false;
    }

    if (also_bind)
    {
        int result;
        const int flag = 1;
        if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
        {
            socket_error = string("setsockopt() failure. ") + getErrorStringSocketsError(getSocketError());
            closesocket(socket_desc);
            socket_desc = INVALID_SOCKET;
            return false;
        }
        if (sa.ip_protocol == IPv4) 
            result = ::bind(socket_desc, (struct sockaddr*) &sa.sa4, sizeof(sa.sa4));
        else
            result = ::bind(socket_desc, (struct sockaddr*) &sa.sa6, sizeof(sa.sa6));

        if (result)
        {
            socket_error = string("bind() failure. ") + getErrorStringSocketsError(getSocketError());
            closesocket(socket_desc);
            socket_desc = INVALID_SOCKET;
            return false;
        }
    }

    return true;
}

void Socket::connectAsynchronous_thread(SocketAddress sa, function1<void, bool> callback_function)
{
    int sdesc;
    sdesc = INVALID_SOCKET;
    if (sa.ip_protocol == IPv4)
        sdesc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    else
        sdesc = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (sdesc == INVALID_SOCKET)
    {
        {
            lock_guard<recursive_mutex> lock(socket_mutex);
            socket_error = string("socket() failure. ") + getErrorStringSocketsError(getSocketError());
        }

        if (callback_function) callback_function(false);
        return;
    }

    int result = -1;
    if (sa.ip_protocol == IPv4)
        result = ::connect(sdesc, (struct sockaddr*) &sa.sa4, sizeof(sa.sa4));
    else
        result = ::connect(sdesc, (struct sockaddr*) &sa.sa6, sizeof(sa.sa6));
    
    try
    {
        this_thread::interruption_point();
        if (callback_function)
        {
            if (result != INVALID_SOCKET)
            {
                lock_guard<recursive_mutex> lock(socket_mutex);
                socket_desc = result;
                #ifndef _WIN32
                    int flags = fcntl(socket_desc, F_GETFL, 0);
                    fcntl(socket_desc, F_SETFL, flags | O_NONBLOCK);
                #else
                    unsigned long non_blocking_mode = 1;
                    ioctlsocket(socket_desc, FIONBIO, &non_blocking_mode);
                #endif

                socket_addr = SP<SocketAddress>(new SocketAddress(sa));
                listening_socket = false;
                callback_function(true);

                async_connect_thread = SP<boost::thread>();
                return;
            }
            else
                callback_function(false);
        }
    }
    catch (boost::thread_interrupted ti)
    {
        if (result != INVALID_SOCKET)
            closesocket(result);
        return;
    }

    lock_guard<recursive_mutex> lock(socket_mutex);
    async_connect_thread = SP<boost::thread>();
}

bool Socket::connectAsynchronous(const SocketAddress &sa, function1<void, bool> callback_function)
{
    lock_guard<recursive_mutex> lock(socket_mutex);
    if (async_connect_thread)
    {
        socket_error = "Asynchronous connecting already in progress.";
        return false;
    }
    if (socket_desc != INVALID_SOCKET)
    {
        socket_error = "Socket is active; cannot start asynchronous connecting.";
        return false;
    }

    async_connect_thread = SP<thread>(new thread(boost::bind(&Socket::connectAsynchronous_thread, this, sa, callback_function)));
    return true;
}

bool Socket::connect(const SocketAddress &sa)
{
    lock_guard<recursive_mutex> lock(socket_mutex);
    if (socket_desc != INVALID_SOCKET)
    {
        socket_error = "Socket is active; can't start connecting.";
        return false;
    }
    if (async_connect_thread)
    {
        socket_error = "Asynchronous connecting in progress; can't start connecting.";
        return false;
    }

    listening_socket = false;

    if (!createSocket(sa, false))
        return false;

    int result = -1;
    if (sa.ip_protocol == IPv4)
        result = ::connect(socket_desc, (struct sockaddr*) &sa.sa4, sizeof(sa.sa4));
    else
        result = ::connect(socket_desc, (struct sockaddr*) &sa.sa6, sizeof(sa.sa6));

    if (result == -1)
    {
        socket_error = string("connect() failure. ") + getErrorStringSocketsError(getSocketError());
        closesocket(socket_desc);
        socket_desc = INVALID_SOCKET;
        return false;
    }

#ifndef _WIN32
    int flags = fcntl(socket_desc, F_GETFL, 0);
    fcntl(socket_desc, F_SETFL, flags | O_NONBLOCK);
#else
    unsigned long non_blocking_mode = 1;
    ioctlsocket(socket_desc, FIONBIO, &non_blocking_mode);
#endif

    socket_addr = SP<SocketAddress>(new SocketAddress(sa));
    return true;
}

bool Socket::listen(const SocketAddress &sa)
{
    lock_guard<recursive_mutex> lock(socket_mutex);
    if (socket_desc != INVALID_SOCKET)
    {
        socket_error = "Socket is active; can't start listening.";
        return false;
    }
    if (async_connect_thread)
    {
        socket_error = "Asynchronous connecting in progress; can't start connecting.";
        return false;
    }

    listening_socket = false;

    if (!createSocket(sa, true))
        return false;

    int result = ::listen(socket_desc, 10);
    if (result)
    {
        socket_error = string("listen() failure. ") + getErrorStringSocketsError(getSocketError());
        closesocket(socket_desc);
        socket_desc = INVALID_SOCKET;
        return false;
    }

#ifndef _WIN32
    int flags = fcntl(socket_desc, F_GETFL, 0);
    fcntl(socket_desc, F_SETFL, flags | O_NONBLOCK);
#else
    unsigned long non_blocking_mode = 1;
    ioctlsocket(socket_desc, FIONBIO, &non_blocking_mode);
#endif

    socket_addr = SP<SocketAddress>(new SocketAddress(sa));
    listening_socket = true;
    return true;
}

void Socket::close()
{
    if (async_connect_thread)
    {
        async_connect_thread->interrupt();
        async_connect_thread->join();
        async_connect_thread = SP<boost::thread>();
    }

    lock_guard<recursive_mutex> lock(socket_mutex);

    if (socket_desc != INVALID_SOCKET)
        closesocket(socket_desc);
    socket_desc = INVALID_SOCKET;
    listening_socket = false;
    socket_addr = SP<SocketAddress>();
}

bool Socket::accept(Socket* accept_socket)
{
    lock_guard<recursive_mutex> lock(socket_mutex);
    if (socket_desc == INVALID_SOCKET) return false;
    if (accept_socket->socket_desc != INVALID_SOCKET) return false;
    if (!accept_socket) return false;
    if (!listening_socket) return false;

    SOCKET result;
    result = ::accept(socket_desc, NULL, NULL);
    if (result == INVALID_SOCKET)
    {
        if (getSocketError() == getEWOULDBLOCK()) return false;
        close();
        listening_socket = false;
        return false;
    }
    int new_socket_desc = result;

    sockaddr_max_t sa_struct;
    memset(&sa_struct, 0, sizeof(sockaddr_max_t));
    socklen_t sa_struct_len = sizeof(sa_struct);
    result = getpeername(new_socket_desc, (struct sockaddr*) &sa_struct, &sa_struct_len);
    if (result)
    {
        ::closesocket(new_socket_desc);
        return false;
    }

    SocketAddress sa;
    struct sockaddr* sa2 = (struct sockaddr*) &sa_struct;
    if (sa2->sa_family == PF_INET)
        sa = SocketAddress((struct sockaddr_in*) &sa_struct);
    else if (sa2->sa_family == PF_INET6)
        sa = SocketAddress((struct sockaddr_in6*) &sa_struct);
    else
    {
        /* Some unknown address family? hrrrr... */
        ::closesocket(new_socket_desc);
        return false;
    }

    accept_socket->socket_desc = new_socket_desc;
    accept_socket->socket_addr = SP<SocketAddress>(new SocketAddress(sa));
    accept_socket->listening_socket = false;

#ifndef _WIN32
    int flags = fcntl(new_socket_desc, F_GETFL, 0);
    fcntl(new_socket_desc, F_SETFL, flags | O_NONBLOCK);
#else
    unsigned long non_blocking_mode = 1;
    ioctlsocket(new_socket_desc, FIONBIO, &non_blocking_mode);
#endif
    return true;
}

SocketAddress Socket::getAddress()
{
    lock_guard<recursive_mutex> lock(socket_mutex);
    if (!socket_addr) return SocketAddress();
    return *socket_addr.get();
}

static void set_async_socketaddress_hostname(string address_string, SP<SocketAddress> address)
{
    if (!address) return;

    /* For now, use regexes to throw away the port part
       of the address string. */
    Regex IPv4_regex("^([0-9.]+):(.+)$");
    if (IPv4_regex.execute(address_string))
        address_string = IPv4_regex.getMatch(1);
    else
    {
        Regex IPv6_regex("^\\[([0-9a-fA-F:]+)\\]:(.+)$");
        if (IPv6_regex.execute(address_string))
            address_string = IPv6_regex.getMatch(1);
        else
            return;
    }

    bool success = false;
    string hostname = reverseResolveUTF8(address_string, &success);
    if (!success) return;

    address->setHostnameUTF8(hostname);
}

void Socket::startAsynchronousReverseResolve()
{
    lock_guard<recursive_mutex> lock(socket_mutex);
    string addr = socket_addr->getHumanReadablePlainUTF8();
    thread t(set_async_socketaddress_hostname, addr, socket_addr);
    t.detach();
}

bool Socket::active()
{
    lock_guard<recursive_mutex> lock(socket_mutex);
    return (socket_desc != INVALID_SOCKET);
}

size_t Socket::send(const void* data, size_t datalen)
{
    lock_guard<recursive_mutex> lock(socket_mutex);
    if (socket_desc == INVALID_SOCKET) return 0;
    if (listening_socket) return 0;

    resetSocketError();
    int result = ::send(socket_desc, (const char*) data, datalen, 0);
    i32 er = getSocketError();
    if (result == 0 && er != getEWOULDBLOCK())
    {
        close();
        return 0;
    }
    if (result == -1)
    {
        if (getSocketError() == getEWOULDBLOCK()) return 0;
        close();
        return 0;
    }

    return result;
}

size_t Socket::recv(void* data, size_t datalen)
{
    lock_guard<recursive_mutex> lock(socket_mutex);
    if (socket_desc == INVALID_SOCKET) return 0;
    if (listening_socket) return 0;

    resetSocketError();
    int result = ::recv(socket_desc, (char*) data, datalen, 0);
    i32 er = getSocketError();
    if (result == 0)
    {
        if (er != getEWOULDBLOCK())
            close();
        return 0;
    }
    if (result == -1)
    {
        if (er == getEWOULDBLOCK()) return 0;
        close();
        return 0;
    }

    return result;
}

string Socket::recv(size_t datalen)
{
    lock_guard<recursive_mutex> lock(socket_mutex);
    char* datachunk = new char[datalen];
    size_t result = recv((void*) datachunk, datalen);
    string result_str(datachunk, result);
    delete[] datachunk;
    return result_str;
}

string Socket::getError()
{
    lock_guard<recursive_mutex> lock(socket_mutex);
    return socket_error;
}

void trankesbel::initializeSockets()
{
    test_initialize_winsock();
}

void trankesbel::shutdownSockets()
{
    test_winsock_shutdown();
}


static void reverse_resolve_function(const SocketAddress &sa, function2<void, bool, string> signal_func)
{
    if (!signal_func) return;

    struct sockaddr_in sa4;
    struct sockaddr_in6 sa6;
    bool use_ipv4 = false;

    if (sa.getIPProtocol() == IPv4)
    {
        sa.getRawIPv4Sockaddr(&sa4);
        use_ipv4 = true;
    }
    else
        sa.getRawIPv6Sockaddr(&sa6);


    int max_wait = 20;

    char node_name[2001], service_name[2001];
    node_name[2000] = 0;
    service_name[2000] = 0;

    int result = 0;
    do
    {
        if (use_ipv4)
            result = getnameinfo((struct sockaddr*) &sa4, sizeof(sa4), node_name, 2000, service_name, 2000, NI_NAMEREQD);
        else
            result = getnameinfo((struct sockaddr*) &sa6, sizeof(sa6), node_name, 2000, service_name, 2000, NI_NAMEREQD);

        if (result == EAI_AGAIN)
            second_wait(1);
        else
            break;
    } while ( (max_wait--) > 0);

    if (!result)
        signal_func(true, string(node_name));
    else
        signal_func(false, getErrorStringEAI(result));
}

void trankesbel::reverseResolveUTF8(const SocketAddress &sa, function2<void, bool, string> func, bool block)
{
    thread reverse_resolve_thread(reverse_resolve_function, sa, func);
    if (block)
        reverse_resolve_thread.join();
    else
        reverse_resolve_thread.detach();
}

static void reverse_resolve_proxy(bool success, SocketAddress sa, string errormsg, function2<void, bool, string> func)
{
    if (!success)
        func(false, errormsg);

    reverse_resolve_function(sa, func);
}

void trankesbel::reverseResolveUTF8(string address, function2<void, bool, string> func, bool block)
{
    if (block)
    {
        bool success;
        string result_msg;

        SocketAddress sa = SocketAddress::resolveUTF8(address, "1", &success, &result_msg);
        if (!success)
        {
            func(false, result_msg);
            return;
        }

        reverseResolveUTF8(sa, func, true);
        return;
    }

    function3<void, bool, SocketAddress, string> callback;
    callback = boost::bind(reverse_resolve_proxy, _1, _2, _3, func);

    SocketAddress::resolve(address, "1", callback, false);
}

static void reverse_resolve_callback(bool success, string host_or_error, bool* success2, string* host_or_error2)
{
    (*success2) = success;
    (*host_or_error2) = host_or_error;
}

string trankesbel::reverseResolveUTF8(string address, bool* success2)
{
    bool success = false;
    string errormsg;

    function2<void, bool, string> callback;
    callback = boost::bind(reverse_resolve_callback, _1, _2, &success, &errormsg);

    reverseResolveUTF8(address, callback, true);

    if (success2) (*success2) = success;

    return errormsg;
}

std::string trankesbel::reverseResolveUTF8(const SocketAddress &sa, bool* success2)
{
    bool success = false;
    string errormsg;

    function2<void, bool, string> callback;
    callback = boost::bind(reverse_resolve_callback, _1, _2, &success, &errormsg);

    reverseResolveUTF8(sa, callback, true);

    if (success2) (*success2) = success;

    return errormsg;
}

