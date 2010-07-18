#ifndef minimal_http_server_hpp
#define minimal_http_server_hpp

#include <set>
#include "dfterm2_limits.hpp"
#include "types.hpp"
#include "sockets.hpp"
#include "minimal_http_server_private.hpp"

namespace dfterm {

/* This class hosts a HTTP server. It's very minimal.
   It will only serve files that have been explicitly
   defined to be served. */
class HTTPServer
{
    private:
        std::set<SP<trankesbel::Socket> > listening_sockets;
        std::set<SP<HTTPClient> > http_clients;
        int prune_counter;

        void pruneUnusedSockets();

        /* No copies */
        HTTPServer(const HTTPServer &hs) { };
        HTTPServer& operator=(const HTTPServer &hs) { return (*this); };

    public:
        HTTPServer();

        /* Checks for new connections, sends data over network etc. etc. 
           It can return a socket that should be added to a SocketEvents class,
           and then this function should be called whenever any of the sockets
           is ready. These sockets are client connections to the HTTP server.

           You should call this until the return value is a null reference. 
           Listening sockets added with HTTPServer::addListeningSocket are never
           returned through this function. */
        SP<trankesbel::Socket> cycle();

        /* Gives a listening socket on which HTTPServer will listen for HTTP connections. */
        void addListeningSocket(SP<trankesbel::Socket> listening_socket);
};

};

#endif
