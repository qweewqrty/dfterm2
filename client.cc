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

    identified = false;
    clients = (vector<WP<Client> >*) 0;

    interface.initialize();

    identify_window = interface.createInterfaceElementWindow();
    identify_window->setDesiredWidth(30);
    identify_window->setDesiredHeight(1);
    identify_window->setTitleUTF8("Enter your nickname for this session");
    ui32 index = identify_window->addListElementUTF8("", "", true, true);
    identify_window->modifyListSelection(index);
    identify_window->setListCallback(bind(&Client::identifySelectFunction, this, _1));

    max_chat_history = 500;

    /* Hide cursor */
    ts.sendPacket("\x1b[?25l", 6);
}

Client::~Client()
{
}

bool Client::isActive() const
{
    return client_socket->active();
}

void Client::cycleChat()
{
    if (!global_chat) return;
    if (!global_chat_reader) return;

    while(1)
    {
        bool message;
        UnicodeString us = global_chat_reader->getLogMessage(&message);
        if (!message) break;

        UnicodeString chat_str = chat_window->getListElement(chat_window_input_index);
        chat_window->deleteListElement(chat_window_input_index);
        chat_window_input_index = 0xFFFFFFFF;

        chat_window->addListElement(us, "", true, false);

        /* Remove first item from list if maximum chat history is reached */
        ui32 list_elements = chat_window->getNumberOfListElements();
        if (list_elements > max_chat_history)
            chat_window->deleteListElement(0);

        chat_window_input_index = chat_window->addListElement(chat_str, "chat", true, true);
        chat_window->modifyListSelection(chat_window_input_index);
    }
}

void Client::cycle()
{
    if (!isActive()) return;
    if (identified) identify_window = SP<InterfaceElementWindow>();

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
        buffer_terminal.copyPreserve(&last_client_terminal);
        deltas.clear();
        packet_pending = false;
        packet_pending_index = 0;
    }

    if (last_client_terminal.getWidth() != client_t.getWidth() ||
        last_client_terminal.getHeight() != client_t.getHeight())
        last_client_terminal.resize(client_t.getWidth(), client_t.getHeight());
    last_client_terminal.copyPreserve(&client_t);

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
            buffer_terminal.copyPreserve(&client_t);
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
            interface.pushKeyPress((ui32) ((unsigned char) buf[i1]), false);
    }
    while(buf_size == 500);

    cycleChat();
}

void Client::setGlobalChatLogger(SP<Logger> global_chat)
{
    this->global_chat = global_chat;
    global_chat_reader = global_chat->createReader();
};

SP<Logger> Client::getGlobalChatLogger() const
{
    return global_chat;
}

bool Client::identifySelectFunction(ui32 index)
{
    /* Some restrictions on name. */
    UnicodeString suggested_name = identify_window->getListElement(index);
    /* Remove leading and trailing whitespace */
    suggested_name.trim();
    /* Reject nicknames with embedded whitespace */
    i32 pos = suggested_name.indexOf(' ');
    if (pos != -1)
    {
        suggested_name.findAndReplace(' ', '_');
        identify_window->modifyListElementText(index, suggested_name);
        return false;
    }

    ui32 str_len = suggested_name.countChar32();
    if (str_len == 0) return false; /* no empty nicks! */
    if (str_len > 20)
    {
        /* no overly long nicks either! */
        suggested_name.truncate(20);
        identify_window->modifyListElementText(index, suggested_name);
        return false; 
    }

    /* The string has no whitespace, is at most 20 characters long but does have at least 1 character.
       This is an acceptable nickname. */
    nickname = suggested_name;
    clientIdentified();
    return true;
}

bool Client::chatSelectFunction(ui32 index)
{
    UnicodeString chat_message = chat_window->getListElement(chat_window_input_index);
    chat_message.remove(0, 6);
    chat_window->modifyListElementText(chat_window_input_index, "Chat> ");
    chat_window->modifyListSelection(chat_window_input_index);

    UnicodeString prefix;
    prefix = UnicodeString::fromUTF8("<") +
             nickname +
             UnicodeString::fromUTF8("> ");
    global_chat->logMessage(prefix + chat_message);
    return false;
};

/* Used as a callback function for element window input. */
bool Client::chatRestrictFunction(ui32* keycode, ui32* cursor)
{
    /* Don't allow cursor to move before 6 characters. */
    if ((*cursor) < 6) { (*cursor) = 6; return false; };
    if ((*keycode) == 0) return true;

    /* Restrict minimum length to 6 characters. (length of "Chat> ") */
    UnicodeString us = chat_window->getListElement(chat_window_input_index);
    if (us.countChar32() <= 6 && ((*keycode) == 8 || (*keycode) == 127)) return false;
       
    return true;
}

void Client::updateNicklistWindowForAll()
{
    vector<WP<Client> >::iterator i1;
    for (i1 = clients->begin(); i1 != clients->end(); i1++)
    {
        SP<Client> c = (*i1).lock();
        if (c)
            c->updateNicklistWindow();
    }
}

void Client::updateNicklistWindow()
{
    if (!nicklist_window) return;

    nicklist_window->deleteAllListElements();
    if (!clients) return;

    vector<UnicodeString> nicks;
    nicks.reserve(clients->size());

    vector<WP<Client> >::iterator i1;
    for (i1 = clients->begin(); i1 != clients->end(); i1++)
    {
        SP<Client> c = (*i1).lock();
        if (!c)
        {
            clients->erase(i1);
            i1--;
            continue;
        };
        UnicodeString us = c->nickname;
        if (us.countChar32() == 0) continue;

        nicks.push_back(c->nickname);
    }
    sort(nicks.begin(), nicks.end());

    vector<UnicodeString>::iterator i2;
    for (i2 = nicks.begin(); i2 != nicks.end(); i2++)
        nicklist_window->addListElement((*i2), "", false);
}

void Client::clientIdentified()
{
    game_window = interface.createInterface2DWindow();
    nicklist_window = interface.createInterfaceElementWindow();
    chat_window = interface.createInterfaceElementWindow();

    /* Set callbacks for chat window */
    function2<bool, ui32*, ui32*> bound_function = bind(&Client::chatRestrictFunction, this, _1, _2);
    chat_window->setInputCallback(bound_function);
    function1<bool, ui32> bound_select_callback = bind(&Client::chatSelectFunction, this, _1);
    chat_window->setListCallback(bound_select_callback);

    /* 10x10 is small, and a good default. */
    game_window->setMinimumSize(10, 10);
    chat_window->setDesiredWidth(40);
    chat_window->setDesiredHeight(5);
    chat_window_input_index = chat_window->addListElementUTF8("Chat> ", "chat", true, true);
    chat_window->modifyListSelection(chat_window_input_index);

    identified = true;

    updateNicklistWindowForAll();
}

void Client::setClientVector(vector<WP<Client> >* clients)
{
    this->clients = clients;
}

