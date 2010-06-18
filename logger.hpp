#ifndef logger_hpp
#define logger_hpp

#include <string>
#include <sstream>
#include "types.hpp"
#include <boost/thread/recursive_mutex.hpp>
#include <vector>
#include <queue>
#include <unicode/unistr.h>
#include <time.h>

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

/* This is a generic logger macro.
   When you need to log something, write this:

   LOG( notability, stringstream )

   where notability is one of these:
   NOTE        - Just some note, normal operation log.
                 E.g. something like "User Gabriel logged out.".
   ERROR       - When it's possible there's a malfunction or
                 something.
                 E.g. "Could not save slot profile to database."
   FATAL       - Error that's grave enough that dfterm2 has to close.
                 E.g. "Out of memory."

   Stringstream can be used a bit like C++ streams.
   E.g.

   LOG(NOTE, "This is a logger test string number " << 1234 << " and there is " << str_animal << " in my pants.")

   Have all your messages in UTF-8. Timestamp will be added to every log message.
*/

extern SP<Logger> admin_logger; 
extern void flush_messages();
extern void initialize_logger();

enum Notability { Note, Error, Fatal };


/* This is the most monstrous macro I have ever written. Please don't send me to hell. */
#ifdef __WIN32
#define LOG(notability, stringstream_msg) ({ std::stringstream ss; \
                                        dfterm::initialize_logger(); \
                                        wchar_t msg[1000]; \
                                        UChar msg_uchar[1000]; \
                                        msg_uchar[999] = 0; \
                                        msg[999] = 0; \
                                        time_t timet = time(0); \
                                        struct tm* timem = localtime(&timet); /* on windows localtime() is thread-safe */ \
                                        if (wcsftime(msg, 999, L"%Y-%m-%d %H:%M:%S", timem) == 0) \
                                        { \
                                            admin_logger->logMessageUTF8("Error while trying to make a log message. wcsftime() returned 0."); \
                                        } else { \
                                        int32_t msg_len = 0; \
                                        UErrorCode uerror = U_ZERO_ERROR; \
                                        u_strFromWCS(msg_uchar, 999, &msg_len, msg, -1, &uerror); \
                                        if (U_FAILURE(uerror)) \
                                        { \
                                            admin_logger->logMessageUTF8("Error while trying to make a log message."); \
                                        } else { \
                                        UnicodeString msg_us(msg_uchar, msg_len); \
                                        string msg_utf8; \
                                        msg_us.toUTF8String(msg_utf8); \
                                        ss << msg_utf8 << " "; \
                                        if (notability == dfterm::Note) \
                                            ss << "Note: "; \
                                        else if (notability == dfterm::Error) \
                                            ss << "Error: "; \
                                        else if (notability == dfterm::Fatal) \
                                            ss << "FATAL: "; \
                                        ss << stringstream_msg; \
                                        admin_logger->logMessageUTF8(ss.str()); \
                                        } } \
                                       })
#else
#define LOG(notability, stringstream_msg) ({ std::stringstream ss; \
                                        dfterm::initialize_logger(); \
                                        wchar_t msg[1000]; \
                                        UChar msg_uchar[1000]; \
                                        msg_uchar[999] = 0; \
                                        msg[999] = 0; \
                                        time_t timet = time(0); \
                                        struct tm timem; \
                                        localtime_r(&timet, &timem); /* assuming call always succeeds */ \
                                        if (wcsftime(msg, 999, L"%Y-%m-%d %H:%M:%S", &timem) == 0) \
                                        { \
                                            admin_logger->logMessageUTF8("Error while trying to make a log message. wcsftime() returned 0."); \
                                        } else { \
                                        int32_t msg_len = 0; \
                                        UErrorCode uerror = U_ZERO_ERROR; \
                                        u_strFromWCS(msg_uchar, 999, &msg_len, msg, -1, &uerror); \
                                        if (U_FAILURE(uerror)) \
                                        { \
                                            admin_logger->logMessageUTF8("Error while trying to make a log message."); \
                                        } else { \
                                        UnicodeString msg_us(msg_uchar, msg_len); \
                                        string msg_utf8; \
                                        msg_us.toUTF8String(msg_utf8); \
                                        ss << msg_utf8 << " "; \
                                        if (notability == dfterm::Note) \
                                            ss << "Note: "; \
                                        else if (notability == dfterm::Error) \
                                            ss << "Error: "; \
                                        else if (notability == dfterm::Fatal) \
                                            ss << "FATAL: "; \
                                        ss << stringstream_msg; \
                                        admin_logger->logMessageUTF8(ss.str()); \
                                        } } \
                                       })
#endif

} /* namespace dfterm */

#endif

