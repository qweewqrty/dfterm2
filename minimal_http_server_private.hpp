/* Private classes for minimal_http_server */

#ifndef minimal_http_server_private_hpp
#define minimal_http_server_private_hpp

#include <sstream>
#include "nanoclock.hpp"
#include "sockets.hpp"

namespace dfterm
{

/* This is the maximum number of bytes that we will accept
   before two newlines in HTTP request. */
const size_t max_bytes_on_request = 5000;
/* This is the longest lifespan of a connection, in nanoseconds.
   (120000000000 = 120 seconds = 2 minutes) */
const trankesbel::ui64 max_nanoseconds_on_request = 120000000000LL;

class Document
{
    public:
        std::string document;    /* The whole document is in this string */
        std::string contenttype; /* And the contenttype for it */

        Document() { };

        Document(const std::string &d, const std::string &ct)
        {
            document = d;
            contenttype = ct;
        }
};

class HTTPClient
{
    private:
        SP<trankesbel::Socket> s;
        std::string request_string;
        std::string response_string;
        std::string plain_document;
        trankesbel::ui64 creation_time;
        std::map<std::string, Document>* served_content;

        bool waiting_for_request;

        /* No copies */
        HTTPClient(const HTTPClient &hc) { };
        HTTPClient& operator=(const HTTPClient &hc) { return (*this); };

    public:
        HTTPClient()
        {
            creation_time = trankesbel::nanoclock();
            waiting_for_request = true;
        }

        void setPlain(const std::string &p)
        { plain_document = p; };

        void setServedContent(std::map<std::string, Document>* sc)
        { served_content = sc; };

        void setSocket(SP<trankesbel::Socket> sock) { s = sock; };
        SP<trankesbel::Socket> getSocket() { return s; };

        void cycle();
};

}

#endif
