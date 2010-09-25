#ifndef slot_linux_common_hpp
#define slot_linux_common_hpp

#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <map>
#include "pty.h"
#include "types.hpp"

namespace dfterm
{

bool waitAndLaunchProcess(pid_t* pid,
                          boost::recursive_mutex* glue_mutex,
                          volatile bool* close_thread,
                          std::map<std::string, UnicodeString>* parameters_p,
                          Pty* program_pty_p,
                          trankesbel::ui32* terminal_w,
                          trankesbel::ui32* terminal_h);

}

#endif

