#include "logger.hpp"
#include "minimal_http_server.hpp"
#include <iostream>
#include <cstdio>

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

    if (!plain_document.empty())
    {
        if (waiting_for_request)
        {
            map<string, Document>::iterator i1 = served_content->find(plain_document);
            if (i1 == served_content->end())
            {
                s->close();
                return;
            }

            waiting_for_request = false;
            response_string = i1->second.document;
        }
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
            map<string, Document>::iterator i1 = served_content->find(document);
            if (i1 != served_content->end())
            {
                LOG(Note, "A connection from " << s->getAddress().getHumanReadablePlainUTF8() << " " << get_or_head << " requested page \"" << document << "\" (200)");

                stringstream response;
                response << "HTTP/1.1 200 OK\r\n";
                response << "Server: Dfterm2 minimal HTTP server\r\n";
                response << "Connection: close\r\n";
                response << "Content-Type: " << i1->second.contenttype << "\r\n\r\n";
                if (get)
                    response << i1->second.document;

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
        else
        {
            s->close();
            return;
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
            hc->setServedContent(&served_content);
            hc->setSocket(http_client);
            http_clients.insert(hc);

            LOG(Note, "Got HTTP connection from " << http_client->getAddress().getHumanReadablePlainUTF8());
            return http_client;
        };

        SP<Socket> plain_http_client(new Socket);
        set<pair<SP<Socket>, string> >::iterator i2, plain_listening_sockets_end = plain_listening_sockets.end();
        for (i2 = plain_listening_sockets.begin(); i2 != plain_listening_sockets_end; ++i2)
        {
            if (!(i2->first) || !(i2->first)->active())
                continue;

            bool got_connection = (i2->first)->accept(http_client.get());
            if (!got_connection) continue;

            if (http_clients.size() >= MAX_HTTP_CONNECTIONS)
            {
                repeat = true;
                pruneUnusedSockets();
                if (http_clients.size() >= MAX_HTTP_CONNECTIONS)
                {
                    LOG(Note, "Got plain connection from " << http_client->getAddress().getHumanReadablePlainUTF8() << " but closed it immediately, as maximum number (" << MAX_HTTP_CONNECTIONS << ") of HTTP connections has been reached.");
                    http_client->close();
                    break;
                }
            }

            SP<HTTPClient> hc(new HTTPClient);
            hc->setServedContent(&served_content);
            hc->setPlain(i2->second);
            hc->setSocket(http_client);
            http_clients.insert(hc);

            LOG(Note, "Got plain connection from " << http_client->getAddress().getHumanReadablePlainUTF8());
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
    assert(listening_socket && listening_socket->active());
    listening_sockets.insert(listening_socket);
}

void HTTPServer::addPlainListeningSocket(SP<Socket> listening_socket, const string &serviceaddress)
{
    assert(listening_socket && listening_socket->active());
    plain_listening_sockets.insert(pair<SP<Socket>, string>(listening_socket, serviceaddress));
}

void HTTPServer::serveContentUTF8(string buffer, string contenttype, string serviceaddress)
{
    assert(!serviceaddress.empty());
    served_content[serviceaddress] = Document(buffer, contenttype);
}

void HTTPServer::serveFileUTF8(string filename, string contenttype, string serviceaddress, const map<string, string> &replacors)
{
    wchar_t* wc = (wchar_t*) 0;
    size_t wc_size = 0;
    TO_WCHAR_STRING(filename, wc, &wc_size);
    if (wc_size == 0)
    {
        LOG(Error, "Tried to serve a file from disk in HTTP server but filename \"" << filename << "\" is malformed.");
        return;
    }

    wc = new wchar_t[wc_size+1];
    memset(wc, 0, (wc_size+1) * sizeof(wchar_t));

    TO_WCHAR_STRING(filename, wc, &wc_size);

    #ifdef _WIN32
    FILE* f = _wfopen(wc, L"rb");
    #else
    FILE* f = fopen(TO_UTF8(filename).c_str(), "rb");
    #endif
    delete[] wc;

    if (!f)
    {
        LOG(Error, "Attempted to serve file \"" << filename << "\" but opening it for reading failed.");
        return;
    }

    // Calculate the size of the file
    
    
    #ifdef _WIN32
    __int64 fsize = 0;
    _fseeki64(f, 0, SEEK_END);
    fsize = _ftelli64(f);
    if (fsize == -1)
    #else
    off64_t fsize = 0;
    fseeko64(f, 0, SEEK_END);
    fsize = ftello64(f);
    if (fsize < (off64_t) 0)
    #endif
    {
        LOG(Error, "Attempted to serve file \"" << filename << "\" but calculating file size failed.");
        fclose(f);
        return;
    }
    #ifdef _WIN32
    _fseeki64(f, 0, SEEK_SET);
    #else
    fseeko64(f, 0, SEEK_SET);
    #endif

    if ((ui64) fsize > MAX_HTTP_FILE_SIZE)
    {
        LOG(Error, "Attempted to serve file \"" << filename << "\" but it is larger (" << fsize << " bytes) than the compile time limit (" << MAX_HTTP_FILE_SIZE << " bytes) on file sizes.");
        fclose(f);
        return;
    }

    char* buf = new char[fsize];
    size_t result = fread(buf, 1, fsize, f);
    if (result != (ui64) fsize)
    {
        LOG(Error, "Attempted to serve file \"" << filename << "\" but there was a read error.");
        delete[] buf;
        return;
    }
    fclose(f);

    string document_copy = string(buf, fsize);
    map<string, string>::const_iterator i1, replacors_end = replacors.end();
    for (i1 = replacors.begin(); i1 != replacors_end; ++i1)
    {
        while(true)
        {
            size_t index = document_copy.find(i1->first);
            if (index == string::npos) break;

            document_copy.erase(index, i1->first.size());
            document_copy.insert(index, i1->second);
        }
    }

    served_content[serviceaddress] = Document(document_copy, contenttype);

    delete[] buf;
}
