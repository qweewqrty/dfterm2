#ifndef server_to_server_hpp
#define server_to_server_hpp

#include <string>
#include "sockets.hpp"

namespace dfterm
{

/* Defines a link to another dfterm2 server. 
   This class stores just the information needed.
   ServerToServerLinkSession then maintains the
   runtime information. */
class ServerToServerConfigurationPair
{
    private:
        std::string remotehostname;
        std::string remoteport;
        std::string name;
        trankesbel::ui64 server_timeout;

    public:
        ServerToServerConfigurationPair();

        std::string serialize() const;
        void unSerialize(const std::string &stscp);

        /* Sets the remote server host. The address will be resolved
           with SocketAddress::resolveUTF8() in a ServerToServerSession. */
        void setTargetUTF8(const std::string &remotehost, const std::string &port);
        /* Returns the hostname of the remote server. 
           This is an empty string if not set before. */
        std::string getTargetHostnameUTF8() const;
        /* Returns the port of the remote server.
           This is "0" if not set before. */
        std::string getTargetPortUTF8() const;

        /* Set and get the name of the link. */
        void setNameUTF8(const std::string &name);
        std::string getNameUTF8() const;
        void getNameUTF8(std::string* name) const;

        /* This is the number of nanoseconds to wait until trying again to connect
           to another server. It defaults to 120000000000. (120 seconds) */
        void setServerTimeout(trankesbel::ui64 nanoseconds);
        /* And gets the timeout */
        trankesbel::ui64 getServerTimeout() const;

        bool operator<(const ServerToServerConfigurationPair &c_pair) const
        {
            if (name != c_pair.name) return name < c_pair.name;
            if (server_timeout != c_pair.server_timeout) return server_timeout < c_pair.server_timeout;
            if (remotehostname != c_pair.remotehostname) return remotehostname < c_pair.remotehostname;
            return remoteport < c_pair.remoteport;
        }
};


/* Maintains a server-to-server session. Set configuration with
   ServerToServerConfigurationPair. 
   
   You cannot update ServerToServerConfigurationPair in a session
   once it has been created. */
class ServerToServerSession
{
    private:
        ServerToServerConfigurationPair pair;
        SP<trankesbel::Socket> server_socket;
        trankesbel::SocketAddress server_address;

        /* This class internally uses threads, so we need a mutex. */
        boost::recursive_mutex session_mutex;

        /* The function that will be called when socket goes ready. */
        boost::function1<void, SP<trankesbel::Socket> > callback_function;

        /* This thread handles (re)connecting and resolving. */
        SP<boost::thread> session_thread;
        /* And this is the function for it */
        static void static_server_to_server_session(ServerToServerSession* self);
        void server_to_server_session();

        bool broken; /* Used when thread creation fails or some other unrecoverable error occurs. */
        bool connection_ready; /* Set to true, when sending and receiving data through the session is ok. */

        WP<ServerToServerSession> self;

        ServerToServerSession() { };

        /* No copies */
        ServerToServerSession(const ServerToServerSession &stss) { assert(false); };
        ServerToServerSession& operator=(const ServerToServerSession &stss) { assert(false); return (*this); };

        void construct(const ServerToServerConfigurationPair &pair);
    public:

        /* Creates a new session. 
           You can set a callback here that will be called
           when the server session becomes ready. The
           argument will be the socket object associated with the 
           session. (So you can drop it to SocketEvents when called).
           Note that the callback is called from another thread.
         */
        static SP<ServerToServerSession> create(const ServerToServerConfigurationPair &pair,
                                                boost::function1<void, SP<trankesbel::Socket> > callback_function);
        static SP<ServerToServerSession> create(const ServerToServerConfigurationPair &pair)
        { return create(pair, boost::function1<void, SP<trankesbel::Socket> >()); };

        /* Destructor */
        ~ServerToServerSession();

        /* Returns true if some unrecoverable error has occursed in the session. 
           Currently happens only if has not been possible to create a thread for
           resolving address. */
        bool isBroken();

        /* Returns true if connection has been established. */
        bool isConnectionReady();

        /* Returns the socket class used by session. You can put it in SocketEvents
           to know when to call ServerToServerSession::read() or ServerToServerSession::write().
           Returns a null reference if session is not ready. */
        SP<trankesbel::Socket> getSocket();

        void cycle();
};


}

#endif

