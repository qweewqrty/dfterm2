#ifndef nanoclock_hpp
#define nanoclock_hpp

#include <stdint.h>

namespace dfterm
{

/* "Nanoclock" functions, named so because they
   take a uint64_t type values for times, that are represented
   in nanoseconds. Note that their actual resolution may not be even
   close to nanoseconds, but they shouldn't have too much error beside
   a few milliseconds. */

/* Waits for the given amount of nanoseconds. */
void nanowait(uint64_t nanoseconds);
/* Gets current time in nanoseconds. */
uint64_t nanoclock();

}

#endif
