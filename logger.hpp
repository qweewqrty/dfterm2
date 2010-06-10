#ifndef logger_hpp
#define logger_hpp

#include <string>
#include <sstream>
#include "types.hpp"
#include <boost/thread/recursive_mutex.hpp>
#include <vector>
#include <queue>
#include <unicode/unistr.h>

using namespace std;
using namespace boost;

namespace dfterm
{

class LoggerReader;

/* Class that collects lines and sends them out to callbacks.
   It is thread-safe to be written to by multiple threads.
   Users of loggers (that read from it), create a read handle
   from it. */
class Logger
{
    private:
        recursive_mutex logmutex;
        vector<WP<LoggerReader> > readers;

    public:
        /* Logs a unicode message. */
        void logMessage(const UnicodeString &message);
        /* Logs a UTF-8 encoded message. (standard string) */
        void logMessageUTF8(string message);
        /* Logs a UTF-8 encoded message. (string stream) */
        void logMessageUTF8(const stringstream &message);
        /* Logs a UTF-8 encoded message. (C-string) */
        void logMessageUTF8(const char* message, size_t len);
        /* logs a UTF-8 encoded message. (C-string, null terminated) */
        void logMessageUTF8(const char* message);

        /* Create a reader for this logger. */
        SP<LoggerReader> createReader();
};

/* Users that read the log information will get a smart pointer
   for this class from Logger class. It has a single method,
   getLogMessage(), from which you get log lines. */
class LoggerReader
{
    friend class Logger;
    private:
        recursive_mutex logmutex;
        queue<UnicodeString> messages;
        void logMessage(UnicodeString message);

    public:
        /* Returns next message from log. It sets 'got_message'
           to false if there was no string in the log buffer, and to true
           if a new string was returned. An empty string is returned in the former case. */
        UnicodeString getLogMessage(bool* got_message);
};

}

#endif

