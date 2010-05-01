#ifndef client_hpp
#define client_hpp

#include "interface_termemu.hpp"

namespace dfterm
{

using namespace trankesbel;

class Client
{
    private:
        TelnetSession ts;
        Socket client_socket;
        InterfaceTermemu interface;

        /* No copies */
        Client(const Client &c) { };
        Client& operator=(const Client &c) { return (*this); };

        /* No default constructor */

    public:
        /* Create a client and associate a socket with it. */
        Client(Socket client_socket);
}

}

#endif

