#include <wchar.h>
#include <iostream>
#include <cstdio>
#include <cstring>
#include "logger.hpp"
#include "unicode/unistr.h"
#include "unicode/ustring.h"

using namespace dfterm;
using namespace std;
using namespace boost;
using namespace trankesbel;

/* Every message goes here. */
namespace dfterm {
SP<Logger> admin_logger;
SP<LoggerReader> admin_messages_reader;
};

void dfterm::initialize_logger()
{
    if (admin_logger) return;

    admin_logger = SP<Logger>(new Logger);
    admin_messages_reader = admin_logger->createReader();
}

/* Flush admin messages */
void dfterm::flush_messages()
{
    if (!admin_logger) return;
    if (!admin_messages_reader) return;

    bool msg;
    do
    {
        UnicodeString us = admin_messages_reader->getLogMessage(&msg);
        if (!msg) break;

        int32_t src_len = us.length();
        UChar* us_str = us.getBuffer(src_len);
        int32_t dest_len = 0;
        UErrorCode uerror = U_ZERO_ERROR;
        u_strToWCS(NULL, 0, &dest_len, us_str, src_len, &uerror);
        if (U_FAILURE(uerror) && uerror != U_BUFFER_OVERFLOW_ERROR)
        {
            cout << "Failure in converting log message to wchar_t. (pre-flighting) " << u_errorName(uerror) << endl;
            us.releaseBuffer();
            continue;
        }
        uerror = U_ZERO_ERROR;

        wchar_t* dest_str = new wchar_t[dest_len+1];
        memset(dest_str, 0, sizeof(wchar_t) * (dest_len+1));
        u_strToWCS(dest_str, dest_len, &dest_len, us_str, src_len, &uerror);
        us.releaseBuffer();

        if (U_FAILURE(uerror))
        {
            cout << "Failure in converting log message to wchar_t." << u_errorName(uerror) << endl;
            delete[] dest_str;
            continue;
        }

        /* wcout not supported by mingw, so we use wprintf */
        wprintf(L"%ls\n", dest_str);
        fflush(stdout);
        delete[] dest_str;
    } while(msg);
}

void Logger::logMessage(const UnicodeString &message)
{
    lock_guard<recursive_mutex> lock(logmutex);
    vector<WP<LoggerReader> >::iterator i1;
    for (i1 = readers.begin(); i1 != readers.end(); i1++)
    {
        SP<LoggerReader> reader = (*i1).lock();
        if (!reader)
            continue;
        reader->logMessage(message);
    }
    /* Remove null readers */
    for (i1 = readers.begin(); i1 != readers.end(); i1++)
    {
        SP<LoggerReader> reader = (*i1).lock();
        if (!reader)
        {
            readers.erase(i1);
            i1 = readers.begin() - 1;
            continue;
        }
    }
}

void Logger::logMessageUTF8(string message)
{
    logMessage(UnicodeString::fromUTF8(message));
}

void Logger::logMessageUTF8(const stringstream &message)
{
    logMessage(UnicodeString::fromUTF8(message.str()));
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

