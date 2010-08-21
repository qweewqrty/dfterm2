#ifndef server_to_server_hpp
#define server_to_server_hpp

#include <string>
#include "sockets.hpp"

namespace dfterm
{

/* Defines a link to another dfterm2 server. 
   This class stores just the information needed.
   ServerToServerLinkSession then maintains the
   runtime information. */
class ServerToServerConfigurationPair
{
    private:
        std::string remotehostname;
        std::string remoteport;
        trankesbel::ui64 server_timeout;

    public:
        ServerToServerConfigurationPair();

        /* Sets the remote server host. The address will be resolved
           with SocketAddress::resolveUTF8() in a ServerToServerSession. */
        void setTargetUTF8(const std::string &remotehost, const std::string &port);
        /* Returns the hostname of the remote server. 
           This is an empty string if not set before. */
        std::string getTargetHostnameUTF8() const;
        /* Returns the port of the remote server.
           This is "0" if not set before. */
        std::string getTargetPortUTF8() const;

        /* This is the number of nanoseconds to wait until trying again to connect
           to another server. It defaults to 120000000000. (120 seconds) */
        void setServerTimeout(trankesbel::ui64 nanoseconds);
        /* And gets the timeout */
        trankesbel::ui64 getServerTimeout() const;
};


/* Maintains a server-to-server session. Set configuration with
   ServerToServerConfigurationPair. */
class ServerToServerSession
{
    private:
        ServerToServerConfigurationPair pair;
        SP<trankesbel::Socket> server_socket;
        trankesbel::SocketAddress server_address;
        bool server_address_resolved;

        ServerToServerSession() { assert(false); };

        /* No copies */
        ServerToServerSession(const ServerToServerSession &stss) { assert(false); };
        ServerToServerSession& operator=(const ServerToServerSession &stss) { assert(false); return (*this); };

    public:
        ServerToServerSession(const ServerToServerConfigurationPair &pair);
};


}

#endif

