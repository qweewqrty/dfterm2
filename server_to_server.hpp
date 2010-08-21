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
};

}

#endif

