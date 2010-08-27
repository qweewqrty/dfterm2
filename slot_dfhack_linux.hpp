#ifndef slot_dfhack_linux_hpp
#define slot_dfhack_linux_hpp

// HACK:
// stdint.h and DFHack's DFstdint_win.h conflict.
// This macro prevents standard windows stdint.h definitions
#define _MSC_STDINT_H_
#include <stdint.h>

#include "DFHack.h"

#include "slot.hpp"

#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "types.hpp"
#include <map>
#include <vector>
#include "interface.hpp"
#include "termemu.h"
#include "interface_ncurses.hpp"
#include "dfscreendataformat.hpp"
#include "lockedresource.hpp"

namespace dfterm
{

class DFHackSlotLinux : public Slot
{
    private:
        bool alive;
        boost::recursive_mutex glue_mutex;
        SP<boost::thread> glue_thread;

        DFHack::ContextManager* df_contextmanager;
        DFHack::Context* df_context;
        DFHack::Position* df_position_module;

        bool isDFClosed();

        trankesbel::ui64 ticks_per_second;

        trankesbel::ui32 df_w, df_h;

        /* Input deque */
        std::deque<trankesbel::KeyPress> input_queue;

        /* Terminal for DF screen. */
        Terminal df_terminal;

        LockedResource<Terminal> df_cache_terminal;

        /* Cycle thread watches this variable and closes when it sees it's true. */
        volatile bool close_thread;

        static void static_thread_function(DFHackSlotLinux* self);
        void thread_function();

        /* Data format in DF memory */
        DFScreenDataFormat data_format;

        void updateDFWindowTerminal();

        /* Reads current DF window size from memory. */
        void updateWindowSizeFromDFMemory();

        /* Detects DF version and fills in pointer paths for them. */
        bool detectDFVersion();

        /* Injects a DLL to the DF process.  */
        bool injectDLL(std::string dllname);

        bool dont_take_running_process;

        std::map<std::string, UnicodeString> parameters;


    public:
        DFHackSlotLinux();
        DFHackSlotLinux(bool dummy); /* Create with this constructor to
                                        not let DFGlue grab a running
                                        process. */
        ~DFHackSlotLinux();

        void setParameter(std::string str, UnicodeString str2) { boost::lock_guard<boost::recursive_mutex> lock(glue_mutex); parameters[str] = str2; };
        void getSize(trankesbel::ui32* width, trankesbel::ui32* height);
        bool isAlive();
        void unloadToWindow(SP<trankesbel::Interface2DWindow> target_window);
        void feedInput(const trankesbel::KeyPress &kp);
};

}

#endif

