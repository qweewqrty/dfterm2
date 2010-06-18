#ifndef slot_dfglue_hpp
#define slot_dfglue_hpp

#ifndef __WIN32
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

using namespace boost;
using namespace trankesbel;
using namespace std;

namespace dfterm
{

enum SearchCompare { Equal, NotEqual, Greater, GreaterOrEqual, Less, LessOrEqual, Any };

template<class T> bool inline max_comp(const T& a, const T& b)
{
    return (a > b) ? a : b;
};

/* Copied from old dfterm. Contains information on how to find some
   memory location. */
class SearchAddress
{
    private:
        map<ptrdiff_t, pair<ui8, SearchCompare> > values8;
        map<ptrdiff_t, pair<ui16, SearchCompare> > values16;
        map<ptrdiff_t, pair<ui32, SearchCompare> > values32;

    public:
        void reset() { values8.clear(); values16.clear(); values32.clear(); };
        void pushByte(ptrdiff_t offset, ui8 byte, SearchCompare comparetype = Equal)
        { values8[offset] = pair<ui8, SearchCompare>(byte, comparetype); };
        void pushShort(ptrdiff_t offset, ui16 short16, SearchCompare comparetype = Equal)
        { values16[offset] = pair<ui16, SearchCompare>(short16, comparetype); };
        void pushInt(ptrdiff_t offset, ui32 integer32, SearchCompare comparetype = Equal)
        { values32[offset] = pair<ui32, SearchCompare>(integer32, comparetype); };

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
            map<ptrdiff_t, pair<ui8, SearchCompare> >::iterator i8;
            map<ptrdiff_t, pair<ui16, SearchCompare> >::iterator i16;
            map<ptrdiff_t, pair<ui32, SearchCompare> >::iterator i32;

            for (i8 = values8.begin(); i8 != values8.end(); i8++)
                if (!compareValue(i8->second.first, c[i8->first], i8->second.second))
                    return false;
            for (i16 = values16.begin(); i16 != values16.end(); i16++)
                if (!compareValue(i16->second.first, *((ui16*) &c[i16->first]), i16->second.second))
                    return false;
            for (i32 = values32.begin(); i32 != values32.end(); i32++)
                if (!compareValue(i32->second.first, *((ui32*) &c[i32->first]), i32->second.second))
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
        vector<ptrdiff_t> address;
        vector<string> module;
        vector<pair<SearchAddress, ptrdiff_t> > search_address;

        map<string, pair<ptrdiff_t, ptrdiff_t> > module_addresses;

        bool needs_search;

        HANDLE last_used_module;
        DWORD num_modules;
        HMODULE modules[1000];

        void enumModules(HANDLE df_handle);

        ptrdiff_t searchAddress(HANDLE df_handle, SearchAddress sa, ptrdiff_t baseaddress, ptrdiff_t size);

    public:
        PointerPath();

        void reset();
        void pushAddress(ptrdiff_t offset, string module = string(""));

        void pushSearchAddress(SearchAddress sa, string module, ptrdiff_t end_offset);

        vector<ptrdiff_t> &getAddressList() { return address; };
        vector<string> &getModuleList() { return module; };
        vector<pair<SearchAddress, ptrdiff_t> > &getSearchAddressList() { return search_address; };

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
        recursive_mutex glue_mutex;
        SP<thread> glue_thread;

        bool isDFClosed();

        ui64 ticks_per_second;

        ui32 df_w, df_h;

        /* Input deque */
        deque<pair<ui32, bool> > input_queue;

        HANDLE df_handle;
        vector<HWND> df_windows;

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
        static void buildColorFromFloats(float32 r, float32 g, float32 b, Color* color, bool* bold);

        void updateDFWindowTerminal();

        /* Launches a new DF process and fills in pointers. */
        /* Returns false if no can do. */
        bool launchDFProcess(HANDLE* df_process, vector<HWND>* df_window);
        /* This finds a running DF process for us. */
        static bool findDFProcess(HANDLE* df_process, vector<HWND>* df_window);
        /* Find the window for DF process. Needs process id */
        static bool findDFWindow(vector<HWND>* df_window, DWORD pid);
        /* A callback for enumerating windows */
        static BOOL enumDFWindow(HWND hwnd, LPARAM strct);

        /* Reads current DF window size from memory. */
        void updateWindowSizeFromDFMemory();

        /* Detects DF version and fills in pointer paths for them. */
        bool detectDFVersion();

        /* Injects a DLL to the DF process.  */
        bool injectDLL(string dllname);

        /* Maps trankesbel (special) key codes to windows virtual keys. */
        map<KeyCode, DWORD> vkey_mappings;
        /* Initialize aformentioned map */
        void initVkeyMappings();

        bool dont_take_running_process;

        map<string, UnicodeString> parameters;

    public:
        DFGlue();
        DFGlue(bool dummy); /* Create with this constructor to
                               not let DFGlue grab a running
                               process. */
        ~DFGlue();

        void setParameter(string str, UnicodeString str2) { lock_guard<recursive_mutex> lock(glue_mutex); parameters[str] = str2; };
        void getSize(ui32* width, ui32* height);
        bool isAlive();
        void unloadToWindow(SP<Interface2DWindow> target_window);
        void feedInput(ui32 keycode, bool special_key);
};

}

#endif

