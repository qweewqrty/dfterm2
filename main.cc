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

using namespace dfterm;
using namespace trankesbel;
using namespace boost;
using namespace std;

/* Every message goes here. */
namespace dfterm {
SP<Logger> admin_logger;
SP<LoggerReader> admin_messages_reader;
};

/* Flush admin messages */
void dfterm::flush_messages()
{
    bool msg;
    do
    {
        UnicodeString us = admin_messages_reader->getLogMessage(&msg);
        if (!msg) break;

        string utf8_str;
        us.toUTF8String(utf8_str);
        cout << utf8_str << endl;
    } while(msg);
}

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
    admin_logger = SP<Logger>(new Logger);
    admin_messages_reader = admin_logger->createReader();

    sockets_initialize socket_initialization;

    string port("8000");
    string address("0.0.0.0");

    string database_file("dfterm2_database.sqlite3");

    SocketAddress listen_address;
    bool succeeded_resolve = false;
    string error_message;

    unsigned int ticks_per_second = 20;

    function3<void, bool, SocketAddress, string> resolve_binding = 
    bind(resolve_success, &succeeded_resolve, &listen_address, &error_message, _1, _2, _3);

    int i1;
    for (i1 = 1; i1 < argc; i1++)
    {
        if (!strcmp(argv[i1], "--port") || !strcmp(argv[i1], "-p") && i1 < argc-1)
            port = argv[++i1];
        else if (!strcmp(argv[i1], "--database") || !strcmp(argv[i1], "-db") && i1 < argc-1)
            database_file = argv[++i1];
        else if (!strcmp(argv[i1], "--address") || !strcmp(argv[i1], "-l") && i1 < argc-1)
            address = argv[++i1];
        else if (!strcmp(argv[i1], "--version") || !strcmp(argv[i1], "-v"))
        {
            cout << "This is dfterm2, (c) 2010 Mikko Juola" << endl;
            cout << "This version was compiled " << __DATE__ << " " << __TIME__ << endl;
            cout << "This program is distributed under a BSD-style license. See license.txt" << endl << endl;
            cout << "This dfterm2 version supports IPv4 and IPv6." << endl;
            return 0;
        }
        else if (!strcmp(argv[i1], "--tickspersecond") || !strcmp(argv[i1], "-t") && i1 < argc-1)
        {
            ticks_per_second = (unsigned int) strtol(argv[++i1], NULL, 10);
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
            cout << "--tickspersecond (ticks per second)" << endl;
            cout << "-t (ticks per second) Set number of ticks per second. Defaults to 20." << endl << endl;;
            cout << "--database (database file)" << endl;
            cout << "-db (database file)   Set the database file used. By default dfterm2 will try to look for" << endl;
            cout << "                      environment variable DFTERM2_DB for the file location. If there is no" << endl;
            cout << "                      such variable, it will try opening dfterm2_database.sqlite3 instead." << endl;
            cout << "--help"  << endl;;
            cout << "-h"  << endl;
            cout << "-?                    Show this help." << endl;
            cout << endl;
            cout << "Examples:" << endl << endl;
            cout << "Set 10 ticks per second and listen on port 8001:" << endl;
            cout << "dfterm2 --port 8001 -t 10" << endl;
            cout << "Listen on an IPv6 localhost:" << endl;
            cout << "dfterm2 --address ::1" << endl << endl;
            cout << "Take it easy!!" << endl;
            cout << endl;
            return 0;
        }
    }
    if (ticks_per_second == 0)
    {
        cout << "Cannot use 0 ticks per second. Making that 1 tick per second." << endl;
        ticks_per_second = 1;
    }

    SocketAddress::resolve(address, port, resolve_binding, true);
    if (!succeeded_resolve)
    {
        flush_messages();
        cerr << "Resolving [" << address << "]:" << port << " failed. Check your listening address settings." << endl;
        return -1;
    }

    SP<State> state = State::createState();
    state->setTicksPerSecond(ticks_per_second);

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

