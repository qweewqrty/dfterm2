#include "nanoclock.hpp"
#ifndef __WIN32
#include <sys/time.h>
#include <time.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>
#include <time.h>
#endif

using namespace dfterm;

void dfterm::nanowait(uint64_t nanoseconds)
{
    #ifndef __WIN32
    struct timespec ts;
    ts.tv_sec = nanoseconds / (uint64_t) 1000000000;
    ts.tv_nsec = nanoseconds % (uint64_t) 1000000000;

    struct timespec remaining;
    while (nanosleep(&ts, &remaining) != 0)
    {
        ts.tv_sec = remaining.tv_sec;
        ts.tv_nsec = remaining.tv_nsec;
    }
    #else
    /* According to microsoft docs, Sleep()
       rarely has better resolution than a few milliseconds.
       Should work just fine here, I hope... */
    Sleep( (DWORD) ((uint64_t) nanoseconds / (uint64_t) 1000000) );
    #endif
}

uint64_t dfterm::nanoclock()
{
    #ifndef __WIN32
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (uint64_t) tv.tv_sec * (uint64_t) 1000000000 + 
           (uint64_t) tv.tv_usec * (uint64_t) 1000;
    #else
    LARGE_INTEGER li, fr;
    QueryPerformanceCounter(&li);
    if (QueryPerformanceFrequency(&fr) == 0)
    {
        /* The value will wrap way before 64-bit integer max is reached.
           For the user it may suddenly look like a LOT of time
           has passed when the wrapping happens. But at least it
           will work without the high-resolution clock...for some time. */
        DWORD t = GetTickCount();
        return t * 1000000LL;
    }

    LONGLONG li64 = li.QuadPart, fr64 = fr.QuadPart;
    LONGLONG result = ((LONGLONG) li64 / fr64) * 1000000000LL +
               (((LONGLONG) li64 % fr64) * 1000000000LL) / fr64;
    return (uint64_t) result;
    #endif
}


