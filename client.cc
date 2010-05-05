#include "client.hpp"
#include <iostream>
#include <algorithm>

using namespace std; 
using namespace trankesbel;
using namespace dfterm;

ClientTelnetSession::ClientTelnetSession() : TelnetSession()
{
};

ClientTelnetSession::~ClientTelnetSession()
{
};

void ClientTelnetSession::setClient(WP<Client> client)
{
    this->client = client;
}

bool ClientTelnetSession::writeRawData(const void* data, size_t* size)
{
    size_t max_size = (*size);

    (*size) = 0;
    SP<Client> spc = client.lock();
    if (!spc) return false;

    SP<Socket> s = spc->getSocket();
    if (!s) return false;

    if (!s->active()) return false;
    if (max_size == 0) return true;

    size_t result = s->send(data, max_size);
    if (result > 0)
    {
        (*size) = result;
        return true;
    }
    if (!s->active()) return false;

    return true;
}

bool ClientTelnetSession::readRawData(void* data, size_t* size)
{
    size_t max_size = (*size);

    (*size) = 0;
    SP<Client> spc = client.lock();
    if (!spc) return false;

    SP<Socket> s = spc->getSocket();
    if (!s) return false;

    if (!s->active()) return false;
    if (max_size == 0) return true;

    size_t result = s->recv(data, max_size);
    if (result > 0)
    {
        (*size) = result;
        return true;
    }
    if (!s->active()) return false;

    return true;
}

Client::Client(SP<Socket> client_socket)
{
    this->client_socket = client_socket;
    packet_pending = false;
    packet_pending_index = 0;
    do_full_redraw = false;

    game_window = interface.createInterface2DWindow();
    nicklist_window = interface.createInterfaceElementWindow();
    chat_window = interface.createInterfaceElementWindow();
    game_window->setMinimumSize(40, 10);
    chat_window->addListElementUTF8("Apina", "kapina", true);
    chat_window->setDesiredWidth(60);
    chat_window->setDesiredHeight(5);
    nicklist_window->addListElementUTF8("Korilla", "morilla", true);

    interface.initialize();
}

Client::~Client()
{
}

bool Client::isActive() const
{
    return client_socket->active();
}

void Client::cycle()
{
    if (!isActive()) return;

    ts.cycle();

    /* Check if client has resized their terminal, adjust
       if necessary. */
    do_full_redraw = false;
    ui32 w, h;
    ts.getTerminalSize(&w, &h);
    if (w != buffer_terminal.getWidth() || h != buffer_terminal.getHeight())
    {
        /* Do not allow larger than 300x300 terminals. */
        w = (w > 300) ? 300 : w;
        h = (h > 300) ? 300 : h;

        buffer_terminal.resize(w, h);
        buffer_terminal.feedString("\x1b[2J", 4);
        do_full_redraw = true;

        ts.sendPacket("\x1b[2J", 4);

        interface.setTerminalSize(w, h);
    }
    interface.refresh();
    interface.cycle();

    const Terminal& client_t = interface.getTerminal();

    if (packet_pending && ts.isPacketCancellable(packet_pending_index))
        ts.cancelPacket(packet_pending_index);
    else if (deltas.size() > 0)
    {
        buffer_terminal.copyPreserve(&client_t);
        deltas.clear();
        packet_pending = false;
        packet_pending_index = 0;
    }

    if (!do_full_redraw)
        deltas = client_t.restrictedUpdateCycle(&buffer_terminal); 
    else
        deltas = client_t.updateCycle();
    if (deltas.size() > 0)
    {
        packet_pending_index = ts.sendPacket(deltas.c_str(), deltas.size());
        packet_pending = true;
        /* If full redraw, throw away the packet indices so that
           we can't discard this package */
        if (do_full_redraw)
        {
            packet_pending = false;
            packet_pending_index = 0;
            deltas.clear();
        }
    }

    ts.cycle();

    char buf[500];
    size_t buf_size = 500;
    do
    {
        bool active = ts.receive((void*) buf, &buf_size);
        if (buf_size == 0) break;

        size_t i1;
        for (i1 = 0; i1 < buf_size; i1++)
            interface.pushKeyPress((ui32) buf[i1], false);
    }
    while(buf_size == 500);
}

