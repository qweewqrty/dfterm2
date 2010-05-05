#ifndef client_hpp
#define client_hpp

#include "interface_termemu.hpp"
#include "termemu.h"
#include "sockets.hpp"
#include "telnet.hpp"

namespace dfterm
{

using namespace trankesbel;

class Client;

/* Required by TelnetSession */
class ClientTelnetSession : public TelnetSession
{
    private:
        WP<Client> client;

    public:
        ClientTelnetSession();
        ~ClientTelnetSession();

        bool readRawData(void* data, size_t* size);
        bool writeRawData(const void* data, size_t* size);

        void setClient(WP<Client> client);
};

class Client
{
    private:
        ClientTelnetSession ts;
        SP<Socket> client_socket;
        InterfaceTermemu interface;
        Terminal buffer_terminal;

        /* Nicks go in this window */
        SP<InterfaceElementWindow> nicklist_window;
        /* And chat to this window */
        SP<InterfaceElementWindow> chat_window;
        /* And the game screen to this one */
        SP<Interface2DWindow> game_window;

        bool do_full_redraw;
        bool packet_pending;
        ui32 packet_pending_index;
        string deltas;

        WP<Client> self;

        /* No copies */
        Client(const Client &c) { };
        Client& operator=(const Client &c) { return (*this); };

        /* No default constructor */
        Client() { };

        /* Create a client and associate a socket with it. */
        Client(SP<Socket> client_socket);

        void setSelf(WP<Client> c) { self = c; ts.setClient(self); };

    public:
        static SP<Client> createClient(SP<Socket> client_socket)
        {
            SP<Client> c(new Client(client_socket));
            c->setSelf(WP<Client>(c));
            c->ts.handShake();
            return c;
        }
        /* Destructor */
        ~Client();

        /* Returns the socket the client is using */
        SP<Socket> getSocket() { return client_socket; };

        /* Returns true if client connection is active. */
        bool isActive() const;

        /* Cycle the client connection */
        void cycle();
};

}

#endif

