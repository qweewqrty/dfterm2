#include "logger.hpp"

using namespace dfterm;
using namespace std;
using namespace boost;
using namespace trankesbel;

void Logger::logMessage(const UnicodeString &message)
{
    lock_guard<recursive_mutex> lock(logmutex);
    vector<WP<LoggerReader> >::iterator i1;
    for (i1 = readers.begin(); i1 != readers.end(); i1++)
    {
        SP<LoggerReader> reader = (*i1).lock();
        if (!reader)
        {
            readers.erase(i1);
            i1 = readers.begin() - 1;
            continue;
        }
        reader->logMessage(message);
    }
}

void Logger::logMessageUTF8(string message)
{
    logMessage(UnicodeString::fromUTF8(message));
}

void Logger::logMessageUTF8(const char* message, size_t len)
{
    logMessage(UnicodeString::fromUTF8(string(message, len)));
};

void Logger::logMessageUTF8(const char* message)
{
    logMessage(UnicodeString::fromUTF8(string(message)));
}

SP<LoggerReader> Logger::createReader()
{
    SP<LoggerReader> lr(new LoggerReader);
    readers.push_back(lr);
    return lr;
}

void LoggerReader::logMessage(UnicodeString message)
{
    lock_guard<recursive_mutex> lock(logmutex);
    messages.push(message);
}

UnicodeString LoggerReader::getLogMessage(bool* got_message)
{
    lock_guard<recursive_mutex> lock(logmutex);
    (*got_message) = false;
    if (messages.empty()) return UnicodeString("");
    (*got_message) = true;
    UnicodeString us = messages.front();
    messages.pop();
    return us;
};

