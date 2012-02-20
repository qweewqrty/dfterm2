#ifndef nanoclock_hpp
#define nanoclock_hpp

#include "types.hpp"

namespace trankesbel
{

/* "Nanoclock" functions, named so because they
   take a uint64_t type values for times, that are represented
   in nanoseconds. Note that their actual resolution may not be even
   close to nanoseconds, but they shouldn't have too much error beside
   a few milliseconds. */

/* Waits for the given amount of nanoseconds. */
void nanowait(ui64 nanoseconds);
/* Gets current time in nanoseconds. */
ui64 nanoclock();

}

#endif
