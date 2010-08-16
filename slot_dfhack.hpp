#ifndef slot_dfhack_hpp
#define slot_dfhack_hpp

#ifndef _WIN32
#error "DFHack slot glue is only for Windows."
#endif

// HACK:
// stdint.h and DFHack's DFstdint_win.h conflict.
// This macro prevents standard windows stdint.h definitions
#define _MSC_STDINT_H_
#include <stdint.h>

#include "DFHack.h"

#include "slot.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "types.hpp"
#include <map>
#include <vector>
#include "interface.hpp"
#include "termemu.h"
#include "interface_ncurses.hpp"
#include "dfscreendataformat.hpp"

namespace dfterm
{

class DFHackSlot : public Slot
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

        HANDLE df_handle;
        std::vector<HWND> df_windows;

        /* Terminal for DF screen. */
        Terminal df_terminal;

        /* Cycle thread watches this variable and closes when it sees it's true. */
        volatile bool close_thread;

        static void static_thread_function(DFHackSlot* self);
        void thread_function();

        /* Data format in DF memory */
        DFScreenDataFormat data_format;

        /* Turns rgb floating point values to a terminal color. */
        static void buildColorFromFloats(trankesbel::float32 r, trankesbel::float32 g, trankesbel::float32 b, trankesbel::Color* color, bool* bold);

        void updateDFWindowTerminal();

        /* Launches a new DF process and fills in pointers. */
        /* Returns false if no can do. */
        bool launchDFProcess(HANDLE* df_process, std::vector<HWND>* df_window);
        /* This finds a running DF process for us. */
        static bool findDFProcess(HANDLE* df_process, std::vector<HWND>* df_window);
        /* Find the window for DF process. Needs process id */
        static bool findDFWindow(std::vector<HWND>* df_window, DWORD pid);
        /* A callback for enumerating windows */
        static BOOL CALLBACK enumDFWindow(HWND hwnd, LPARAM strct);

        /* Reads current DF window size from memory. */
        void updateWindowSizeFromDFMemory();

        /* Detects DF version and fills in pointer paths for them. */
        bool detectDFVersion();

        /* Injects a DLL to the DF process.  */
        bool injectDLL(std::string dllname);

        /* Maps trankesbel (special) key codes to windows virtual keys. */
        std::map<trankesbel::KeyCode, DWORD> vkey_mappings;
        /* And these are just normal keys that need to be mapped to somewhere else for greater portability
           (such as number keys to numpad) */
        std::map<trankesbel::KeyCode, DWORD> fixed_mappings;
        /* Initialize aformentioned map */
        void initVkeyMappings();

        bool dont_take_running_process;

        std::map<std::string, UnicodeString> parameters;


        /* Like ReadProcessMemory(), but logs a message if it fails. */
        void LoggerReadProcessMemory(HANDLE handle, const void* address, void* target, SIZE_T size, SIZE_T* read_size);
        bool reported_memory_error;

    public:
        DFHackSlot();
        DFHackSlot(bool dummy); /* Create with this constructor to
                               not let DFGlue grab a running
                               process. */
        ~DFHackSlot();

        void setParameter(std::string str, UnicodeString str2) { boost::lock_guard<boost::recursive_mutex> lock(glue_mutex); parameters[str] = str2; };
        void getSize(trankesbel::ui32* width, trankesbel::ui32* height);
        bool isAlive();
        void unloadToWindow(SP<trankesbel::Interface2DWindow> target_window);
        void feedInput(const trankesbel::KeyPress &kp);
};

}

#endif

