#include "server_to_server.hpp"
#include "logger.hpp"

using namespace dfterm;
using namespace trankesbel;
using namespace std;
using namespace boost;


SP<ServerToServerSession> ServerToServerSession::create(const ServerToServerConfigurationPair &pair,
                                                        function1<void, SP<Socket> > callback_function)
{
    SP<ServerToServerSession> result(new ServerToServerSession);
    result->self = result;
    result->callback_function = callback_function;
    result->construct(pair);
    return result;
}

ServerToServerSession::~ServerToServerSession()
{
    unique_lock<recursive_mutex> lock(session_mutex);

    if (server_socket)
    {
        server_socket->close();
        server_socket = SP<Socket>();
    }

    if (session_thread)
    {
        session_thread->interrupt();
        session_thread->detach();
        session_thread = SP<thread>();
    }

    lock.unlock();
}

void ServerToServerSession::construct(const ServerToServerConfigurationPair &pair)
{
    lock_guard<recursive_mutex> lock(session_mutex);

    this->pair = pair;
    broken = false;
    connection_ready = false;

    /* Launch thread */
    try
    {
        session_thread = SP<thread>(new thread(static_server_to_server_session, this));
    }
    catch (boost::thread_resource_error tre)
    {
        broken = true;
        return;
    }
}

void ServerToServerSession::static_server_to_server_session(ServerToServerSession* self)
{
    try
    {
        self->server_to_server_session();
    }
    catch(thread_interrupted ti)
    {
    };
}

void ServerToServerSession::server_to_server_session()
{
    SP<ServerToServerSession> stss = self.lock();
    assert(stss);

    this_thread::interruption_point();

    unique_lock<recursive_mutex> lock(session_mutex);
    string hostname = pair.getTargetHostnameUTF8();
    string port = pair.getTargetPortUTF8();
    ui64 server_timeout = pair.getServerTimeout();
    lock.unlock();

    SocketAddress sa;
    while(true)
    {
        this_thread::interruption_point();
        bool success;
        string errormsg;

        sa = SocketAddress::resolveUTF8(hostname, port, &success, &errormsg);
        if (!success)
        {
            LOG(Error, "Resolving hostname \"" << hostname << "\" with port \"" << port << "\" failed with error \"" << errormsg << "\". Trying again in " << server_timeout << " nanoseconds.");

            this_thread::sleep(posix_time::time_duration(posix_time::microseconds(server_timeout / 1000ULL)));
            continue;
        }

        break;
    }

    SP<Socket> connection;
    while(!connection || !connection->active())
    {
        this_thread::interruption_point();

        connection = SP<Socket>(new Socket);
        if (!connection->connect(sa))
        {
            LOG(Error, "Connecting to hostname \"" << hostname << "\" to port \"" << port << "\" failed with error \"" << connection->getError() << "\". Trying again in " << server_timeout << " nanoseconds.");
            this_thread::sleep(posix_time::time_duration(posix_time::microseconds(server_timeout / 1000ULL)));
            continue;
        }
    }

    LOG(Note, "Successfully connected server-to-server session to hostname \"" << hostname << "\" to port \"" << port << "\".");
    lock.lock();
    assert(connection);
    connection_ready = true;
    server_socket = connection;
    server_address = sa;
    function1<void, SP<Socket> > cf = callback_function;
    lock.unlock();

    if (cf) cf(connection);
}

bool ServerToServerSession::isBroken()
{
    lock_guard<recursive_mutex> lock(session_mutex);
    return broken;
}

bool ServerToServerSession::isConnectionReady()
{
    lock_guard<recursive_mutex> lock(session_mutex);
    return connection_ready;
}

SP<Socket> ServerToServerSession::getSocket()
{
    lock_guard<recursive_mutex> lock(session_mutex);
    if (broken) return SP<Socket>();
    if (!connection_ready) return SP<Socket>();

    return server_socket;
}

