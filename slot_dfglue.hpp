#ifndef slot_dfglue_hpp
#define slot_dfglue_hpp

#ifndef _WIN32
#error "DF glue is only for Windows."
#endif

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

namespace dfterm
{

enum SearchCompare { Equal, NotEqual, Greater, GreaterOrEqual, Less, LessOrEqual, Any };

template<class T> T inline max_comp(const T& a, const T& b)
{
    return (a > b) ? a : b;
};

/* Copied from old dfterm. Contains information on how to find some
   memory location. */
class SearchAddress
{
    private:
        std::map<ptrdiff_t, std::pair<trankesbel::ui8, SearchCompare> > values8;
        std::map<ptrdiff_t, std::pair<trankesbel::ui16, SearchCompare> > values16;
        std::map<ptrdiff_t, std::pair<trankesbel::ui32, SearchCompare> > values32;

    public:
        void reset() { values8.clear(); values16.clear(); values32.clear(); };
        void pushByte(ptrdiff_t offset, trankesbel::ui8 byte, SearchCompare comparetype = Equal)
        { values8[offset] = std::pair<trankesbel::ui8, SearchCompare>(byte, comparetype); };
        void pushShort(ptrdiff_t offset, trankesbel::ui16 short16, SearchCompare comparetype = Equal)
        { values16[offset] = std::pair<trankesbel::ui16, SearchCompare>(short16, comparetype); };
        void pushInt(ptrdiff_t offset, trankesbel::ui32 integer32, SearchCompare comparetype = Equal)
        { values32[offset] = std::pair<trankesbel::ui32, SearchCompare>(integer32, comparetype); };

        template<class T> bool compareValue(T value, T value2, SearchCompare comparetype)
        {
            switch(comparetype)
            {
                case Equal:
                    return (value == value2);
                case NotEqual:
                    return (value != value2);
                case Greater:
                    return (value > value2);
                case GreaterOrEqual:
                    return (value >= value2);
                case Less:
                    return (value < value2);
                case LessOrEqual:
                    return (value <= value2);
                default:
                    return true;
            }

            return false;
        }

        ptrdiff_t getSize() const
        {
            ptrdiff_t maxsize = 0;
            if (values8.size() > 0)
                maxsize = (ptrdiff_t) ((1 + values8.rend()->first) - (values8.begin()->first));
            if (values16.size() > 0)
                maxsize = max_comp((ptrdiff_t) ((2 + values16.rend()->first) - (values16.begin()->first)), maxsize);
            if (values32.size() > 0)
                maxsize = max_comp((ptrdiff_t) ((4 + values32.rend()->first) - (values32.begin()->first)), maxsize);
            return maxsize;
        }

        bool compareMemory(void* data)
        {
            unsigned char* c = (unsigned char*) data;
            std::map<ptrdiff_t, std::pair<trankesbel::ui8, SearchCompare> >::iterator i8;
            std::map<ptrdiff_t, std::pair<trankesbel::ui16, SearchCompare> >::iterator i16;
            std::map<ptrdiff_t, std::pair<trankesbel::ui32, SearchCompare> >::iterator i32;

            for (i8 = values8.begin(); i8 != values8.end(); i8++)
                if (!compareValue(i8->second.first, c[i8->first], i8->second.second))
                    return false;
            for (i16 = values16.begin(); i16 != values16.end(); i16++)
                if (!compareValue(i16->second.first, *((trankesbel::ui16*) &c[i16->first]), i16->second.second))
                    return false;
            for (i32 = values32.begin(); i32 != values32.end(); i32++)
                if (!compareValue(i32->second.first, *((trankesbel::ui32*) &c[i32->first]), i32->second.second))
                    return false;

            return true;
        }
};

/* Copied from old dfterm and renamed to PointerPath. Stores information
   on how to find an address in Win32 memory environment. Very platform -specific. 
   
   You push addresses to define a pointer path. See examples from dfglue source. */
class PointerPath
{
    private:
        std::vector<ptrdiff_t> address;
        std::vector<std::string> module;
        std::vector<std::pair<SearchAddress, ptrdiff_t> > search_address;

        bool reported_error;
        bool reported_path;

        std::map<std::string, std::pair<ptrdiff_t, ptrdiff_t> > module_addresses;

        bool needs_search;

        HANDLE last_used_module;
        DWORD num_modules;
        HMODULE modules[1000];

        void enumModules(HANDLE df_handle);

        ptrdiff_t searchAddress(HANDLE df_handle, SearchAddress sa, ptrdiff_t baseaddress, ptrdiff_t size);

    public:
        PointerPath();

        void reset();
        void pushAddress(ptrdiff_t offset, std::string module = std::string(""));

        void pushSearchAddress(SearchAddress sa, std::string module, ptrdiff_t end_offset);

        std::vector<ptrdiff_t> &getAddressList() { return address; };
        std::vector<std::string> &getModuleList() { return module; };
        std::vector<std::pair<SearchAddress, ptrdiff_t> > &getSearchAddressList() { return search_address; };

        ptrdiff_t getFinalAddress(HANDLE df_handle);
};

enum DFScreenDataFormat { 
                          DistinctFloatingPointVarying,   /* Varying size for 32-bit floating point buffers. */
                          PackedVarying,                  /* Varying size for 32-bit integer buffer. */
                          Packed256x256,                  /* Same as above, but limited to 256x256 buffer. */
                          Unknown };

class DFGlue : public Slot
{
    private:
        bool alive;
        boost::recursive_mutex glue_mutex;
        SP<boost::thread> glue_thread;

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

        static void static_thread_function(DFGlue* self);
        void thread_function();

        /* For 32-bit floating point data format for screen data, there's
           different buffers for symbols, colors and their backgrounds.
           And these are pointers paths for them. 
           
           The symbol pointer path is used for all data when utilizing the "packed"
           format found in many 40dXX versions and expected new 0.31.04+ versions. */
        PointerPath symbol_pp;
        PointerPath red_pp, blue_pp, green_pp;
        PointerPath red_b_pp, blue_b_pp, green_b_pp;

        /* Pointer path for size information */
        PointerPath size_pp;

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
        DFGlue();
        DFGlue(bool dummy); /* Create with this constructor to
                               not let DFGlue grab a running
                               process. */
        ~DFGlue();

        void setParameter(std::string str, UnicodeString str2) { boost::lock_guard<boost::recursive_mutex> lock(glue_mutex); parameters[str] = str2; };
        void getSize(trankesbel::ui32* width, trankesbel::ui32* height);
        bool isAlive();
        void unloadToWindow(SP<trankesbel::Interface2DWindow> target_window);
        void feedInput(const trankesbel::KeyPress &kp);
};

}

#endif

