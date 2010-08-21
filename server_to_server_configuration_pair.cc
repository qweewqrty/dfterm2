#include "server_to_server.hpp"

using namespace dfterm;
using namespace trankesbel;
using namespace std;;

ServerToServerConfigurationPair::ServerToServerConfigurationPair()
{
    remoteport = "0";
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

