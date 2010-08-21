/*
   Test to check ServerToServerConfigurationPair
   methods do as they advertise.
*/

#include <string>
#include <iostream>
#include "server_to_server.hpp"

using namespace dfterm;
using namespace std;

int main(int argc, char* argv[])
{
    ServerToServerConfigurationPair pair;

    cout << "Checking that port is \"0\" at start." << endl;
    if (pair.getTargetPortUTF8() != "0")
        return 1;

    cout << "Checking that hostname is \"\" at start." << endl;
    if (pair.getTargetHostnameUTF8() != "")
        return 1;
    
    pair.setTargetUTF8("apina ja gorilla", "123456789");

    cout << "Checking that hostname is \"apina ja gorilla\" after set." << endl;
    if (pair.getTargetHostnameUTF8() != "apina ja gorilla")
        return 1;

    cout << "Checking that port is \"123456789\" after set." << endl;
    if (pair.getTargetPortUTF8() != "123456789")
        return 1;

    cout << "Checking that timeout is 120000000000 nanoseconds at start." << endl;
    if (pair.getServerTimeout() != 120000000000ULL)
        return 1;

    cout << "Checking that timeout is 12345678987654321 nanoseconds after set." << endl;
    pair.setServerTimeout(12345678987654321ULL);
    if (pair.getServerTimeout() != 12345678987654321ULL)
        return 1;

    cout << "Everything ok." << endl;

    return 0;
}

