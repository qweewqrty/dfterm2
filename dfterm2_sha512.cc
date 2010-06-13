/* A program for testing SHA512 correctness used in dfterm2 */

#include "types.hpp"
#include "dfterm2_configuration.hpp"
#include <iostream>

using namespace dfterm;
using namespace std;

namespace dfterm {
SP<Logger> admin_logger;
SP<LoggerReader> admin_messages_reader;
};

void dfterm::flush_messages()
{
    bool msg;
    do
    {
        UnicodeString us = admin_messages_reader->getLogMessage(&msg);
        if (!msg) break;

        string utf8_str;
        us.toUTF8String(utf8_str);
        cout << utf8_str << endl;
        cout.flush();
    } while(msg);
}

int main(int argc, char* argv[])
{
    if (argc < 2) return 0;

    admin_logger = SP<Logger>(new Logger);
    admin_messages_reader = admin_logger->createReader();

    data1D result = hash_data(argv[1]);
    cout << result << endl;
    
    return 0;
}

