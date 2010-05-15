#ifndef __WIN32
#error "dfglue is only for Windows. For UNIX/Linux/BSD/Some other use pty instead."
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <iostream>
#include "slot_dfglue.hpp"
#include "nanoclock.hpp"
#include "interface_ncurses.hpp"
#include "cp437_to_unicode.hpp"

using namespace dfterm;
using namespace boost;
using namespace std;
using namespace trankesbel;

/* A simple checksum function. */
static ui32 checksum(const char* buf, size_t buflen)
{
    ui32 a = 1, b = buflen;
    size_t i1;
    for (i1 = 0; i1 < buflen; i1++)
    {
        a += buf[i1];
        b += (buflen - i1) * (ui32) buf[i1];
    }

    a %= 65521;
    b %= 65521;

    return a + b * 65536;
}

static ui32 checksum(string str)
{ return checksum(str.c_str(), str.size()); };

DFGlue::DFGlue() : Slot()
{
    df_handle = INVALID_HANDLE_VALUE;
    df_window = (HWND) INVALID_HANDLE_VALUE;
    close_thread = false;
    data_format = Unknown;

    df_w = 80;
    df_h = 25;

    ticks_per_second = 20;

    alive = true;

    initVkeyMappings();

    // Create a thread that will launch or grab the DF process.
    glue_thread = SP<thread>(new thread(static_thread_function, this));
    if (glue_thread->get_id() == thread::id())
        alive = false;
};

DFGlue::~DFGlue()
{
    unique_lock<recursive_mutex> lock(glue_mutex);
    close_thread = true;
    lock.unlock();

    if (glue_thread)
        glue_thread->join();
    if (df_handle != INVALID_HANDLE_VALUE) CloseHandle(df_handle);
}

bool DFGlue::isAlive()
{
    lock_guard<recursive_mutex> alive_lock(glue_mutex);
    bool alive_bool = alive;
    return alive_bool;
}

void DFGlue::static_thread_function(DFGlue* self)
{
    self->thread_function();
}

void DFGlue::initVkeyMappings()
{
    vkey_mappings[AUp] = VK_UP;
    vkey_mappings[ADown] = VK_DOWN;
    vkey_mappings[ALeft] = VK_LEFT;
    vkey_mappings[ARight] = VK_RIGHT;
    vkey_mappings[CtrlUp] = VK_UP;
    vkey_mappings[CtrlDown] = VK_DOWN;
    vkey_mappings[CtrlLeft] = VK_LEFT;
    vkey_mappings[CtrlRight] = VK_RIGHT;
    vkey_mappings[F1] = VK_F1;
    vkey_mappings[F2] = VK_F2;
    vkey_mappings[F3] = VK_F3;
    vkey_mappings[F4] = VK_F4;
    vkey_mappings[F5] = VK_F5;
    vkey_mappings[F6] = VK_F6;
    vkey_mappings[F7] = VK_F7;
    vkey_mappings[F8] = VK_F8;
    vkey_mappings[F9] = VK_F9;
    vkey_mappings[F10] = VK_F10;
    vkey_mappings[F11] = VK_F11;
    vkey_mappings[F12] = VK_F12;

    vkey_mappings[Home] = VK_HOME;
    vkey_mappings[End] = VK_END;
    vkey_mappings[PgUp] = VK_NEXT;
    vkey_mappings[PgDown] = VK_PRIOR;

    vkey_mappings[InsertChar] = VK_INSERT;
    vkey_mappings[DeleteChar] = VK_DELETE;
}

void DFGlue::thread_function()
{
    // Handle and window for process goes here. 
    HANDLE df_process = INVALID_HANDLE_VALUE;
    HWND df_window = (HWND) INVALID_HANDLE_VALUE;

    // This function should find the window and process for us.
    bool found_process = findDFProcess(&df_process, &df_window);
    if (!found_process)
    {
        lock_guard<recursive_mutex> alive_lock2(glue_mutex);
        alive = false;
        return;
    }

    unique_lock<recursive_mutex> alive_lock(glue_mutex);
    this->df_window = df_window;
    this->df_handle = df_process;
    bool version = detectDFVersion();
    if (!version)
    {
        CloseHandle(df_process);
        this->df_handle = INVALID_HANDLE_VALUE;
        this->df_window = (HWND) INVALID_HANDLE_VALUE;
        alive = false;
        alive_lock.unlock();
        return;
    }
    injectDLL("libdfterm_injection_glue.dll");
    alive_lock.unlock();

    while(1)
    {
        unique_lock<recursive_mutex> update_mutex(glue_mutex);

        if (close_thread) break;
        updateWindowSizeFromDFMemory();
        
        bool esc_down = false;
        while(input_queue.size() > 0)
        {
            bool esc_down_now = esc_down;
            bool ctrl_down_now = false;
            bool shift_down_now = false;
            esc_down = false;

            DWORD vkey;
            ui32 keycode = input_queue.front().first;
            bool special_key = input_queue.front().second;
            input_queue.pop_front();

            if (!special_key)
            {
                if (keycode == 27 && input_queue.size() > 0)
                {
                    esc_down = true;
                    continue;
                }
                else if (keycode == 27)
                    vkey = VK_ESCAPE;
                else
                {
                    SHORT keyscan = VkKeyScan(keycode);
                    vkey = (DWORD) (keyscan & 0xFF);
                    if ((keyscan & 0x00ff) != -1 && (keyscan & 0xff00))
                        shift_down_now = true;
                    if (keycode == 127) // backspace
                        vkey = VK_BACK;
                }
            }
            else if (special_key)
            {
                map<KeyCode, DWORD>::iterator i1 = vkey_mappings.find((KeyCode) keycode);
                if (i1 == vkey_mappings.end()) continue;

                vkey = i1->second;
                
                KeyCode kc = (KeyCode) keycode;
                if (kc == CtrlUp || kc == CtrlDown || kc == CtrlRight || kc == CtrlLeft)
                    ctrl_down_now = true;
            }

            if (shift_down_now)
                PostMessage(df_window, WM_USER, 1, 0);
            if (ctrl_down_now)
                PostMessage(df_window, WM_USER, 1, 1);
            if (esc_down_now)
                PostMessage(df_window, WM_USER, 1, 2);
            PostMessage(df_window, WM_USER, vkey, 3);
            if (shift_down_now)
                PostMessage(df_window, WM_USER, 0, 0);
            if (ctrl_down_now)
                PostMessage(df_window, WM_USER, 0, 1);
            if (esc_down_now)
                PostMessage(df_window, WM_USER, 0, 2);
        }

        update_mutex.unlock();
        updateDFWindowTerminal();

        nanowait(1000000000LL / ticks_per_second);
    }
    alive = false;
}

void DFGlue::buildColorFromFloats(float32 r, float32 g, float32 b, Color* color, bool* bold)
{
    (*color) = Black;
    (*bold) = false;

    Color result = Black;
    bool bold_result = false;

    if (r > 0.1) result = Red;
    if (g > 0.1)
    {
        if (result == Red) result = Yellow;
        else result = Green;
    }
    if (b > 0.1)
    {
        if (result == Red) result = Magenta;
        else if (result == Green) result = Cyan;
        else if (result == Yellow) result = White;
        else result = Blue;
    }
    if (r >= 0.9 || g >= 0.9 || b >= 0.9)
        bold_result = true;
    if (result == White && r < 0.75)
    {
        result = Black;
        bold_result = true;
    }

    (*color) = result;
    (*bold) = bold_result;
}

void DFGlue::updateDFWindowTerminal()
{
    if (data_format == DistinctFloatingPointVarying && df_w < 256 && df_h < 256)
    {
        ptrdiff_t symbol_address = symbol_pp.getFinalAddress(df_handle);
        ptrdiff_t red_address = red_pp.getFinalAddress(df_handle);
        ptrdiff_t green_address = green_pp.getFinalAddress(df_handle);
        ptrdiff_t blue_address = blue_pp.getFinalAddress(df_handle);
        ptrdiff_t red_b_address = red_b_pp.getFinalAddress(df_handle);
        ptrdiff_t green_b_address = green_b_pp.getFinalAddress(df_handle);
        ptrdiff_t blue_b_address = blue_b_pp.getFinalAddress(df_handle);

        ui32 symbol_buf[256*256];
        float32 red_buf[256*256];
        float32 green_buf[256*256];
        float32 blue_buf[256*256];
        float32 red_b_buf[256*256];
        float32 green_b_buf[256*256];
        float32 blue_b_buf[256*256];

        ReadProcessMemory(df_handle, (LPCVOID) symbol_address, symbol_buf, df_w*df_h*4, NULL);
        ReadProcessMemory(df_handle, (LPCVOID) red_address, red_buf, df_w*df_h*4, NULL);
        ReadProcessMemory(df_handle, (LPCVOID) blue_address, blue_buf, df_w*df_h*4, NULL);
        ReadProcessMemory(df_handle, (LPCVOID) green_address, green_buf, df_w*df_h*4, NULL);
        ReadProcessMemory(df_handle, (LPCVOID) red_b_address, red_b_buf, df_w*df_h*4, NULL);
        ReadProcessMemory(df_handle, (LPCVOID) blue_b_address, blue_b_buf, df_w*df_h*4, NULL);
        ReadProcessMemory(df_handle, (LPCVOID) green_b_address, green_b_buf, df_w*df_h*4, NULL);

        lock_guard<recursive_mutex> lock(glue_mutex);
        if (df_terminal.getWidth() != df_w || df_terminal.getHeight() != df_h)
            df_terminal.resize(df_w, df_h);
        ui32 i1, i2;
        for (i2 = 0; i2 < df_h; i2++)
            for (i1 = 0; i1 < df_w; i1++)
            {
                ui32 offset = i1 * df_h + i2;
                Color f_color, b_color;
                bool f_bold = false;
                bool b_bold = false;
                buildColorFromFloats(blue_buf[offset], green_buf[offset], red_buf[offset], &f_color, &f_bold);
                buildColorFromFloats(blue_b_buf[offset], green_b_buf[offset], red_b_buf[offset], &b_color, &b_bold);
                df_terminal.setTile(i1, i2, TerminalTile(symbol_buf[i1 * df_h + i2], f_color, b_color, false, f_bold));
            }
    };
}

class enumDFWindow_struct
{
    public:
    DWORD process_id;
    HWND window;
    enumDFWindow_struct()
    {
        process_id = 0;
        window = (HWND) INVALID_HANDLE_VALUE;
    };
};

// Callback for finding the DF window
BOOL CALLBACK DFGlue::enumDFWindow(HWND hwnd, LPARAM strct)
{
    enumDFWindow_struct* edw = (enumDFWindow_struct*) strct;

    DWORD window_pid;
    GetWindowThreadProcessId(hwnd, &window_pid);
    if (edw->process_id == window_pid)
    {
        edw->window = hwnd;
        return FALSE;
    }

    return TRUE;
}

bool DFGlue::findDFWindow(HWND* df_window, DWORD pid)
{
    (*df_window) = (HWND) 0;

    // Find the DF window
    enumDFWindow_struct edw;
    edw.window = NULL;
    edw.process_id = pid;
    EnumWindows(enumDFWindow, (LPARAM) &edw);
    if (!edw.window)
        return false;

    (*df_window) = edw.window;
    return true;
}


bool DFGlue::findDFProcess(HANDLE* df_process, HWND* df_window)
{
    DWORD processes[1000], num_processes;
    EnumProcesses(processes, sizeof(DWORD)*1000, &num_processes);

    num_processes /= sizeof(DWORD);

    HANDLE df_handle = NULL;

    // Check if Dwarf Fortress is running
    int i1;
    for (i1 = 0; i1 < (int) num_processes; i1++)
    {
        HANDLE h_p = OpenProcess(PROCESS_CREATE_THREAD|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, processes[i1]);
        if (!h_p)
            continue;

        HMODULE h_mod;
        DWORD cb_needed;
        TCHAR process_name[MAX_PATH];

        if (EnumProcessModules(h_p, &h_mod, sizeof(h_mod), &cb_needed))
        {
            GetModuleBaseName(h_p, h_mod, process_name, sizeof(process_name) / sizeof(TCHAR));

            if (string(process_name) == string("dwarfort.exe"))
            {
                df_handle = h_p;
                break;
            }
        }

        CloseHandle(h_p);
    }

    if (!df_handle)
        return false;

    findDFWindow(df_window, processes[i1]);
    if (!df_window)
    {
        CloseHandle(df_handle);
        return false;
    }

    (*df_process) = df_handle;
    return true;
}

void DFGlue::getSize(ui32* width, ui32* height)
{
    lock_guard<recursive_mutex> alive_lock(glue_mutex);
    ui32 w = 80, h = 25;
    (*width) = w;
    (*height) = h;
}

void DFGlue::updateWindowSizeFromDFMemory()
{
    lock_guard<recursive_mutex> alive_lock(glue_mutex);
    if (df_handle == INVALID_HANDLE_VALUE) return;

    ui32 w = 0, h = 0;
    ptrdiff_t size_address = size_pp.getFinalAddress(df_handle);
    ReadProcessMemory(df_handle, (LPCVOID) size_address, &w, 4, NULL);
    ReadProcessMemory(df_handle, (LPCVOID) (size_address + 4), &h, 4, NULL);

    /* Limit the size. If this condition here ever becomes true, it probably means we are reading
       the size information from wrong place and getting weird results. */
    if (!w || !h || w > 255 || h > 255)
    {
        w = 80;
        h = 25;
    }

    df_w = w;
    df_h = h;
}

bool DFGlue::detectDFVersion()
{
    lock_guard<recursive_mutex> alive_lock(glue_mutex);
    if (!df_handle) return false;

    /* Find the DF executable, open it, and calculate checksum for it. */
    char image_file_name[1001];
    memset(image_file_name, 0, 1001);
    GetModuleFileNameEx(df_handle, NULL, image_file_name, 1000);
    FILE* f = fopen(image_file_name, "rb");
    if (!f)
        return false;
    /* We read 100000 bytes from the executable */
    char buf[100000];
    memset(buf, 0, 100000);
    fread(buf, 100000, 1, f);
    fclose(f);
    uint32_t csum = checksum(buf, 100000);

    PointerPath af;
    PointerPath sz;

    switch(csum)
    {
    case 0x9404d33d:  /* DF 0.31.03 */
    af.pushAddress(0x0106FE7C, "dwarfort.exe");
    af.pushAddress(0);
    af.pushAddress(0);
    af.pushAddress(0xC);
    sz.pushAddress(0x013F6B00, "dwarfort.exe");
    red_pp.reset();
    red_pp.pushAddress(0x0106FE7C, "dwarfort.exe");
    red_pp.pushAddress(0);
    red_pp.pushAddress(0);
    blue_pp = red_pp;
    green_pp = red_pp;
    red_b_pp = red_pp;
    green_b_pp = red_pp;
    blue_b_pp = red_pp;
    red_pp.pushAddress(0x18);
    blue_pp.pushAddress(0x10);
    green_pp.pushAddress(0x14);
    red_b_pp.pushAddress(0x24);
    green_b_pp.pushAddress(0x20);
    blue_b_pp.pushAddress(0x1c);

    data_format = DistinctFloatingPointVarying;
    break;
    case 0xa0b99a67:  /* DF v0.31.02 */
    af.pushAddress(0x0106EE7C, "dwarfort.exe");
    af.pushAddress(0);
    af.pushAddress(0);
    af.pushAddress(0xC);
    sz.pushAddress(0x013F5AC0, "dwarfort.exe");

    red_pp.reset();
    red_pp.pushAddress(0x0106EE7C, "dwarfort.exe");
    red_pp.pushAddress(0);
    red_pp.pushAddress(0);
    blue_pp = red_pp;
    green_pp = red_pp;
    red_b_pp = red_pp;
    green_b_pp = red_pp;
    blue_b_pp = red_pp;
    red_pp.pushAddress(0x18);
    blue_pp.pushAddress(0x10);
    green_pp.pushAddress(0x14);
    red_b_pp.pushAddress(0x24);
    green_b_pp.pushAddress(0x20);
    blue_b_pp.pushAddress(0x1c);

    data_format = DistinctFloatingPointVarying;
    break;
    case 0xdf6285d3: /* DF v.0.31.01 */
    af.pushAddress(0x0106EE7C, "dwarfort.exe");
    af.pushAddress(0);
    af.pushAddress(0);
    af.pushAddress(0xC);
    sz.pushAddress(0x013F5AC0, "dwarfort.exe");

    red_pp.reset();
    red_pp.pushAddress(0x0106EE7C, "dwarfort.exe");
    red_pp.pushAddress(0);
    red_pp.pushAddress(0);
    blue_pp = red_pp;
    green_pp = red_pp;
    red_b_pp = red_pp;
    green_b_pp = red_pp;
    blue_b_pp = red_pp;
    red_pp.pushAddress(0x18);
    blue_pp.pushAddress(0x10);
    green_pp.pushAddress(0x14);
    red_b_pp.pushAddress(0x24);
    green_b_pp.pushAddress(0x20);
    blue_b_pp.pushAddress(0x1c);

    data_format = DistinctFloatingPointVarying;
    break;
    case 0x0b6bf445:  /* DF 40d19.2 */
    af.pushAddress(0x00F32B68, "dwarfort.exe");
    af.pushAddress(0);
    sz.pushAddress(0x0129BF28, "dwarfort.exe");
    data_format = PackedVarying;
    break;
    case 0xb548e1b3: /* DF 40d19 */
    af.pushAddress(0x00F31B68, "dwarfort.exe");
    af.pushAddress(0);
    sz.pushAddress(0x0129AF28, "dwarfort.exe");
    data_format = PackedVarying;
    break;
    case 0x04b75394: /* DF 40d18 */
    af.pushAddress(0x0101D240, "dwarfort.exe");
    sz.pushAddress(0x01417EE8, "dwarfort.exe");
    data_format = Packed256x256;
    break;
    default:
    return false;
    }

    size_pp = sz;
    symbol_pp = af;

    return true;
}

void DFGlue::unloadToWindow(SP<Interface2DWindow> target_window)
{
    if (!target_window) return;
    if (!alive) return;

    lock_guard<recursive_mutex> alive_lock(glue_mutex);
    target_window->setMinimumSize(df_w, df_h);

    CursesElement elements[256*256];
    ui32 i1, i2;
    for (i1 = 0; i1 < df_w; i1++)
        for (i2 = 0; i2 < df_h; i2++)
        {
            const TerminalTile &t = df_terminal.getTile(i1, i2);
            ui32 symbol = t.getSymbol();
            if (symbol > 255) symbol = symbol % 256;
            elements[i1 + i2 * 256] = CursesElement(mapCharacter(symbol), (Color) t.getForegroundColor(), (Color) t.getBackgroundColor(), t.getBold());
        }
    target_window->setScreenDisplayNewElements(elements, sizeof(CursesElement), 256, df_w, df_h, 0, 0);
}

bool DFGlue::injectDLL(string dllname)
{
    if (df_handle == INVALID_HANDLE_VALUE) return false;

    /* Allocate memory for the dll string in the target process */
    LPVOID address = VirtualAllocEx(df_handle, NULL, dllname.size()+1, MEM_COMMIT, PAGE_READWRITE);
    if (!address)
        return false;
    if (!WriteProcessMemory(df_handle, address, 
                            (LPCVOID) dllname.c_str(), dllname.size(), NULL))
        return false;

    HMODULE k32 = GetModuleHandle("Kernel32");
    FARPROC loadlibrary_address = GetProcAddress(k32, "LoadLibraryA");

    HANDLE remote_thread = CreateRemoteThread(df_handle, NULL, 0, (LPTHREAD_START_ROUTINE) loadlibrary_address, address, 0, NULL);
    if (!remote_thread)
        return false;

    return true;
}

void DFGlue::feedInput(ui32 keycode, bool special_key)
{
    lock_guard<recursive_mutex> alive_lock(glue_mutex);
    input_queue.push_back(pair<ui32, bool>(keycode, special_key));
}

