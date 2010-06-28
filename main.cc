#include "client.hpp"
#include "sockets.hpp"
#include <cstring>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include "nanoclock.hpp"
#include "cp437_to_unicode.hpp"
#include "logger.hpp"
#include "state.hpp"

#include <clocale>

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
    setlocale(LC_ALL, "");
    
    LOG(Note, "Starting up dfterm2.");

    sockets_initialize socket_initialization;

    string port("8000");
    string address("0.0.0.0");

    string database_file("dfterm2_database.sqlite3");

    SocketAddress listen_address;
    bool succeeded_resolve = false;
    string error_message;

    function3<void, bool, SocketAddress, string> resolve_binding = 
    boost::bind(resolve_success, &succeeded_resolve, &listen_address, &error_message, _1, _2, _3);

    int i1;
    for (i1 = 1; i1 < argc; i1++)
    {
        if ((!strcmp(argv[i1], "--port") || !strcmp(argv[i1], "-p")) && i1 < argc-1)
            port = argv[++i1];
        else if ((!strcmp(argv[i1], "--database") || !strcmp(argv[i1], "-db")) && i1 < argc-1)
            database_file = argv[++i1];
        else if ((!strcmp(argv[i1], "--address") || !strcmp(argv[i1], "-a")) && i1 < argc-1)
            address = argv[++i1];
        else if (!strcmp(argv[i1], "--version") || !strcmp(argv[i1], "-v"))
        {
            cout << "This is dfterm2, (c) 2010 Mikko Juola" << endl;
            cout << "This version was compiled " << __DATE__ << " " << __TIME__ << endl;
            cout << "This program is distributed under a BSD-style license. See license.txt" << endl << endl;
            cout << "This dfterm2 version supports IPv4 and IPv6." << endl;
            return 0;
        }
        else if (!strcmp(argv[i1], "--help") || !strcmp(argv[i1], "-h") || !strcmp(argv[i1], "-?"))
        {
            cout << "Usage: " << endl;
            cout << "dfterm2 [parameters]" << endl;
            cout << "Where \'parameters\' is one or more of the following:" << endl << endl;
            cout << "--port (port number)" << endl;
            cout << "--p (port number)     Set the port where dfterm2 will listen. Defaults to 8000." << endl << endl;
            cout << "--address (address)" << endl;
            cout << "-a (address)          Set the address on which dfterm2 will listen to. Defaults to 0.0.0.0." << endl << endl;
            cout << "--version" << endl;
            cout << "-v                    Show version information and exit." << endl << endl;
            cout << "--database (database file)" << endl;
            cout << "-db (database file)   Set the database file used. By default dfterm2 will try to look for" << endl;
            cout << "                      dfterm2_database.sqlite3. as database." << endl;
            cout << "--help"  << endl;;
            cout << "-h"  << endl;
            cout << "-?                    Show this help." << endl;
            cout << endl;
            cout << "Examples:" << endl << endl;
            cout << "Listen on port 8001:" << endl;
            cout << "dfterm2 --port 8001" << endl;
            cout << "Listen on an IPv6 localhost:" << endl;
            cout << "dfterm2 --address ::1" << endl;
            cout << "Use another database instead of the default one: " << endl;
            cout << "dfterm2 --db my_great_dfterm2_database.sqlite3" << endl << endl;
            cout << "Take it easy!!" << endl;
            cout << endl;
            return 0;
        }
    }

    SocketAddress::resolve(address, port, resolve_binding, true);
    if (!succeeded_resolve)
    {
        flush_messages();
        cerr << "Resolving [" << address << "]:" << port << " failed. Check your listening address settings." << endl;
        return -1;
    }

    SP<State> state = State::createState();
    if (!state->setDatabaseUTF8(database_file))
    {
        cerr << "Failed to set database." << endl;
        flush_messages();
        return -1;
    }

    if (!state->addTelnetService(listen_address))
    {
        flush_messages();
        cerr << "Could not add a telnet service. " << endl;
        return -1;
    }

    state->loop();
    return 0;
}

