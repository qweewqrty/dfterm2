#include "server_to_server.hpp"

using namespace dfterm;
using namespace trankesbel;
using namespace std;;

ServerToServerConfigurationPair::ServerToServerConfigurationPair()
{
    remoteport = "0";
    server_timeout = 120000000000ULL;
}

void ServerToServerConfigurationPair::setTargetUTF8(const std::string &remotehost, const std::string &port)
{
    remotehostname = remotehost;
    remoteport = port;
}

std::string ServerToServerConfigurationPair::getTargetHostnameUTF8() const
{
    return remotehostname;
}

std::string ServerToServerConfigurationPair::getTargetPortUTF8() const
{
    return remoteport;
}

void ServerToServerConfigurationPair::setServerTimeout(ui64 nanoseconds)
{
    server_timeout = nanoseconds;
}

ui64 ServerToServerConfigurationPair::getServerTimeout() const
{
    return server_timeout;
}

void ServerToServerConfigurationPair::setNameUTF8(const std::string &name)
{
    this->name = name;
}

std::string ServerToServerConfigurationPair::getNameUTF8() const
{
    return name;
}

void ServerToServerConfigurationPair::getNameUTF8(std::string* name) const
{
    assert(name);
    (*name) = this->name;
}

