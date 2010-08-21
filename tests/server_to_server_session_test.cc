/*
   Test that connecting with the session class is working.
   Assumes 127.0.0.1:23456 is a free port and can be listened on. 
*/

#include "server_to_server.hpp"
#include "nanoclock.hpp"
#include <iostream>

using namespace dfterm;
using namespace std;
using namespace trankesbel;

int main(int argc, char* argv[])
{
try {
    bool success = false;

    cout << "Resolving address." << endl;
    Socket s;
    SocketAddress sa = SocketAddress::resolvePlainUTF8("127.0.0.1", "23456", &success, NULL);
    if (!success)
    {
        cout << "Resolving 127.0.0.1:23456 failed." << endl;
        return 1;
    }

    if (!s.listen(sa))
    {
        cout << "Listening on 127.0.0.1:23456 failed. " << s.getError() << endl;
        return 1;
    }

    ServerToServerConfigurationPair pair;
    pair.setTargetUTF8("127.0.0.1", "23456");
    pair.setServerTimeout(10000000000ULL);

    SP<ServerToServerSession> session = ServerToServerSession::create(pair);
    cout << "Readyness status at start (undeterministic due to threads, can be 1 or 0): " << session->isConnectionReady() << endl;
    nanowait(1000000000ULL); /* 1 second */
    if (!session->isConnectionReady())
    {
        cout << "Connection did not get ready in 1 second to localhost. (failure)" << endl;
        return 1;
    }

    cout << "Trying incorrect port next." << endl;

    /* Now, try a session with incorrect port, should not get ready. */
    ServerToServerConfigurationPair pair2 = pair;
    pair2.setTargetUTF8("127.0.0.1", "23457");
    SP<ServerToServerSession> session2 = ServerToServerSession::create(pair2);
    cout << "Readyness status at start (undeterministic due to threads, can be 1 or 0): " << session2->isConnectionReady() << endl;
    nanowait(1000000000ULL); /* 1 second */
    if (session2->isConnectionReady())
    {
        cout << "Connection got ready in 1 second to localhost. (failure)" << endl;
        return 1;
    }
}
catch (const std::exception &e)
{
    cout << "Standard exception: " << e.what() << endl;
    return 1;
}
    cout << "Everything ok." << endl;
    return 0;
}

