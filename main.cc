#include "client.hpp"
#include "sockets.hpp"
#include <cstring>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <iostream>

using namespace dfterm;
using namespace trankesbel;
using namespace boost;
using namespace std;

void resolve_success(bool *success_p,
                     SocketAddress *sa, 
                     string* error_message_p, 
                     bool success, 
                     SocketAddress new_addr, 
                     string error_message)
{
    (*sa) = new_addr;
    (*success_p) = success;
    (*error_message_p) = error_message;
};

class sockets_initialize
{
    public:
        sockets_initialize()
        { initializeSockets(); };
        ~sockets_initialize()
        { shutdownSockets(); };
};

int main(int argc, char* argv[])
{
    sockets_initialize socket_initialization;

    string port("8000");
    string address("0.0.0.0");

    SocketAddress listen_address;
    bool succeeded_resolve = false;
    string error_message;

    function3<void, bool, SocketAddress, string> resolve_binding = 
    bind(resolve_success, &succeeded_resolve, &listen_address, &error_message, _1, _2, _3);

    int i1;
    for (i1 = 1; i1 < argc; i1++)
    {
        if (!strcmp(argv[i1], "--port") || !strcmp(argv[i1], "-p") && i1 < argc-1)
            port = argv[++i1];
        else if (!strcmp(argv[i1], "--address") || !strcmp(argv[i1], "-l") && i1 < argc-1)
            address = argv[++i1];
        else if (!strcmp(argv[i1], "--version") || !strcmp(argv[i1], "-v"))
        {
            cout << "This is dfterm2, (c) 2010 Mikko Juola" << endl;
            cout << "This version was compiled " << __DATE__ << " " << __TIME__ << endl;
            cout << "This program is distributed under a BSD-style license. See license.txt" << endl << endl;
            return 0;
        }
    }

    SocketAddress::resolve(address, port, resolve_binding, true);
    if (!succeeded_resolve)
    {
        cerr << "Resolving " << address << ":" << port << " failed. Check your listening address settings." << endl;
        return -1;
    }

    Socket listening_socket;
}

