#ifndef minimal_http_server_hpp
#define minimal_http_server_hpp

#include <set>
#include <map>
#include "dfterm2_limits.hpp"
#include "types.hpp"
#include "sockets.hpp"
#include "minimal_http_server_private.hpp"

namespace dfterm {

/* This class hosts a HTTP server. It's very minimal.
   It will only serve files that have been explicitly
   defined to be served. It also has a plain serving interface,
   that does not use HTTP but just immediately sends content on connect and then disconnects. */
class HTTPServer
{
    private:
        std::set<SP<trankesbel::Socket> > listening_sockets;
        std::set<std::pair<SP<trankesbel::Socket>, std::string> > plain_listening_sockets;

        std::set<SP<HTTPClient> > http_clients;
        std::map<std::string, Document> served_content; /* key = document address, value = document */
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
        /* Adds a listening socket that will, on client connect, immediately just serve the file in serviceaddress.
           If the serviceaddress is invalid on this call, the listening socket will not be added. */
        void addPlainListeningSocket(SP<trankesbel::Socket> listening_socket, const std::string &serviceaddress);

        /* Tells the server to serve given buffer as content for given request address.
           For serviceaddress http://1.2.3.4/ this would be "/" and for http://1.2.3.4/monkey this would be "/monkey" 
           The contenttype is given after Content-type: (goes here), so you can use values like "text/html; charset=UTF-8" or
           something else. Don't use \r\n in it. */
        void serveContentUTF8(std::string buffer, std::string contenttype, std::string serviceaddress);
        /* Same as above but loads the content from file. Note that the file is loaded into memory
           before returning so it is not a good idea to use this on large files. 
           If you fill in replacors, any string that matches the key in the map is replaced by the value. */
        void serveFileUTF8(std::string filename, std::string contenttype, std::string serviceaddress, const std::map<std::string, std::string> &replacors = std::map<std::string, std::string>());
};

};

#endif
