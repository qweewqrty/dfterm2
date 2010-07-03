#ifndef dfterm2_limits_hpp
#define dfterm2_limits_hpp

#include "types.hpp"

namespace dfterm
{
/* Various limits on stuff. */
/* At the moment they are rather low, because dfterm2
   is not in use on any large servers */

/* How many slots can be running at a time, at max. 
   It will not be possible to configure dfterm2 to have
   larger value than this. */
const trankesbel::ui32 MAX_SLOTS = 30;

/* Maximum number of slot profiles */
const trankesbel::ui32 MAX_SLOT_PROFILES = 30;

/* Maximum number of registered users */
const trankesbel::ui32 MAX_REGISTERED_USERS = 50;

/* Maximum number of connections at a time */
const trankesbel::ui32 MAX_CONNECTIONS = 50;

};

#endif
