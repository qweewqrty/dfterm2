#include "logger.hpp"
#include "minimal_http_server.hpp"
#include <iostream>

using namespace dfterm;
using namespace std;
using namespace trankesbel;

HTTPServer::HTTPServer()
{
    prune_counter = 0;
}

void HTTPServer::pruneUnusedSockets()
{
    set<SP<Socket> >::iterator i1, listening_sockets_end;
    bool repeat = true;
    while (repeat)
    {
        repeat = false;
        listening_sockets_end = listening_sockets.end();
        for (i1 = listening_sockets.begin(); i1 != listening_sockets_end; ++i1)
        {
            if (!(*i1) || !(*i1)->active())
            {
                listening_sockets.erase(i1);
                LOG(Note, "Pruned an inactive listening socket for HTTP from memory.");
                repeat = true;
                break;
            }
        }
    };

    repeat = true;
    set<SP<HTTPClient> >::iterator i2, http_clients_end;
    while(repeat)
    {
        repeat = false;
        http_clients_end = http_clients.end();
        for (i2 = http_clients.begin(); i2 != http_clients_end; ++i2)
        {
            if (!(*i2) || !(*i2)->getSocket() || !(*i2)->getSocket()->active())
            {
                LOG(Note, "Pruned an inactive HTTP connection from memory.");
                http_clients.erase(i2);
                repeat = true;
                break;
            }
        }
    };
}

void HTTPClient::cycle()
{
    if (!s || !s->active()) return;

    if (nanoclock() - creation_time >= max_nanoseconds_on_request)
    {
        LOG(Note, "Disconnecting HTTP connection from " << s->getAddress().getHumanReadablePlainUTF8() << " due to reaching maximum connection time.");
        s->close();
        return;
    }

    if (waiting_for_request)
    {
        size_t result = 1;
        while(result)
        {
            char buf[1000];
            result = s->recv(buf, 1000);
            if (result == 0) break;

            request_string.append(buf, result);
            if (request_string.size() > max_bytes_on_request)
            {
                LOG(Note, "Disconnecting HTTP connection from " << s->getAddress().getHumanReadablePlainUTF8() << " due to reaching maximum byte limit (" << max_bytes_on_request << ") on HTTP request.");
                s->close();
                return;
            }
        }


        /* Check if request string is complete */
        size_t index = request_string.find("\r\n\r\n", 0, 4);
        if (index == string::npos)
            return;

        Regex get_method("^GET ([^ \r\n]+) HTTP/1\\.1\r\n");
        bool get = get_method.execute(request_string);
        bool head = false;
        string document;
        if (get)
            document = get_method.getMatch(1);
        else
        {
            Regex head_method("^HEAD ([^ \r\n+) HTTP/1\\.1\r\n");
            head = head_method.execute(request_string);
            if (head)
                document = head_method.getMatch(1);
        }
        string get_or_head("GET");
        if (!get && head) get_or_head = "HEAD";

        if (get || head)
        {
            if (document == string("/"))
            {
                LOG(Note, "A connection from " << s->getAddress().getHumanReadablePlainUTF8() << " " << get_or_head << " requested page \"" << document << "\" (200)");

                stringstream response;
                response << "HTTP/1.1 200 OK\r\n";
                response << "Server: Dfterm2 minimal HTTP server\r\n";
                response << "Connection: close\r\n";
                response << "Content-Type: text/html; charset=UTF-8\r\n\r\n";
                if (get)
                    response << "Go to hell\r\n";

                response_string = response.str();
                waiting_for_request = false;
            }
            else
            {
                LOG(Note, "A connection from " << s->getAddress().getHumanReadablePlainUTF8() << " " << get_or_head << " requested page \"" << document << "\" (404)");

                stringstream response;
                response << "HTTP/1.1 404 Not Found\r\n";
                response << "Server: Dfterm2 minimal HTTP server\r\n";
                response << "Connection: close\r\n";
                response << "Content-Type: text/html; charset=UTF-8\r\n\r\n";
                if (get)
                    response << "<html><body>404</body></html>\r\n";

                response_string = response.str();
                waiting_for_request = false;
            }
        }
    }

    if (!waiting_for_request)
    {
        /* Read and ignore all data */
        char dummybuf[1000];
        size_t result = 1;
        while(result)
        { result = s->recv(dummybuf, 1000); };

        if (response_string.size() == 0)
        {
            s->close();
            return;
        }

        const char* buf;
        size_t len;

        result = 1;
        while (result)
        {
            buf = response_string.data();
            len = response_string.size();

            result = s->send(buf, len);
            if (result > 0)
                response_string.erase(0, result);
        
            if (result == len)
            {
                s->close();
                return;
            }

        }
    }
}

SP<Socket> HTTPServer::cycle()
{
    ++prune_counter;
    if (prune_counter > 200)
    {
        pruneUnusedSockets();
        prune_counter = 0;
    }

    bool repeat = true;

    while(repeat)
    {
        repeat = false;
        /* Check listening sockets */
        SP<Socket> http_client(new Socket);
        set<SP<Socket> >::iterator i1, listening_sockets_end = listening_sockets.end();
        for (i1 = listening_sockets.begin(); i1 != listening_sockets_end; ++i1)
        {
            if (!(*i1) || !(*i1)->active())
                continue;

            bool got_connection = (*i1)->accept(http_client.get());
            if (!got_connection) continue;

            if (http_clients.size() >= MAX_HTTP_CONNECTIONS)
            {
                repeat = true;
                pruneUnusedSockets();
                if (http_clients.size() >= MAX_HTTP_CONNECTIONS)
                {
                    LOG(Note, "Got HTTP connection from " << http_client->getAddress().getHumanReadablePlainUTF8() << " but closed it immediately, as maximum number (" << MAX_HTTP_CONNECTIONS << ") of HTTP connections has been reached.");
                    http_client->close();
                    break;
                }
            }

            SP<HTTPClient> hc(new HTTPClient);
            hc->setSocket(http_client);
            http_clients.insert(hc);

            LOG(Note, "Got HTTP connection from " << http_client->getAddress().getHumanReadablePlainUTF8());
            return http_client;
        };
    }

    /* Check clients */
    set<SP<HTTPClient> >::iterator i2, http_clients_end = http_clients.end();
    for (i2 = http_clients.begin(); i2 != http_clients_end; ++i2)
    {
        SP<HTTPClient> hc = (*i2);
        hc->cycle();
    }

    return SP<Socket>();
}

void HTTPServer::addListeningSocket(SP<Socket> listening_socket)
{
    if (!listening_socket || !listening_socket->active())
        return;

    listening_sockets.insert(listening_socket);
}
