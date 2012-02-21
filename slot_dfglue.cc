#ifndef _WIN32
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
#include <algorithm>
#include <cstring>

#include "state.hpp"

using namespace dfterm;
using namespace boost;
using namespace std;
using namespace trankesbel;

static void PostMessage(const vector<HWND> &windows, UINT msg, WPARAM wparam, LPARAM lparam)
{
    vector<HWND>::const_iterator i1, windows_end = windows.end();
    for (i1 = windows.begin(); i1 != windows_end; ++i1)
        PostMessage(*i1, msg, wparam, lparam);
}

static void SendMessage(const vector<HWND> &windows, UINT msg, WPARAM wparam, LPARAM lparam)
{
    vector<HWND>::const_iterator i1, windows_end = windows.end();
    for (i1 = windows.begin(); i1 != windows_end; ++i1)
        SendMessage(*i1, msg, wparam, lparam);
}

/* A simple checksum function. */
static ui32 checksum(const char* buf, size_t buflen)
{
    assert(!buflen || (buflen && buf));

    ui32 a = 1, b = buflen;
    size_t i1;
    for (i1 = 0; i1 < buflen; ++i1)
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
    df_version = 0;

    df_terminal.setCursorVisibility(false);
    reported_memory_error = false;

    df_handle = INVALID_HANDLE_VALUE;
    close_thread = false;
    data_format = Unknown;

    dont_take_running_process = false;

    use_address_settings = false;
    found_base = false;

    df_w = 80;
    df_h = 25;

    ticks_per_second = 30;

    alive = true;

    initVkeyMappings();

    // Create a thread that will launch or grab the DF process.
    glue_thread = SP<thread>(new thread(static_thread_function, this));
    if (glue_thread->get_id() == thread::id())
        alive = false;
};

DFGlue::DFGlue(bool dummy) : Slot()
{
    df_version = 0;

    df_terminal.setCursorVisibility(false);
    reported_memory_error = false;

    df_handle = INVALID_HANDLE_VALUE;
    close_thread = false;
    data_format = Unknown;

    dont_take_running_process = true;

    df_w = 80;
    df_h = 25;

    ticks_per_second = 20;

    alive = true;

    initVkeyMappings();

    // Create a thread that will launch or grab the DF process.
    glue_thread = SP<thread>(new thread(static_thread_function, this));
    if (glue_thread->get_id() == thread::id())
        alive = false;
}

DFGlue::~DFGlue()
{
    if (glue_thread)
        glue_thread->interrupt();

    unique_lock<recursive_mutex> lock(glue_mutex);
    close_thread = true;
    if (df_handle != INVALID_HANDLE_VALUE && dont_take_running_process) TerminateProcess(df_handle, 1);
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
    assert(self);
    self->thread_function();
}

void DFGlue::initVkeyMappings()
{
    fixed_mappings[(KeyCode) '0'] = VK_NUMPAD0;
    fixed_mappings[(KeyCode) '1'] = VK_NUMPAD1;
    fixed_mappings[(KeyCode) '2'] = VK_NUMPAD2;
    fixed_mappings[(KeyCode) '3'] = VK_NUMPAD3;
    fixed_mappings[(KeyCode) '4'] = VK_NUMPAD4;
    fixed_mappings[(KeyCode) '5'] = VK_NUMPAD5;
    fixed_mappings[(KeyCode) '6'] = VK_NUMPAD6;
    fixed_mappings[(KeyCode) '7'] = VK_NUMPAD7;
    fixed_mappings[(KeyCode) '8'] = VK_NUMPAD8;
    fixed_mappings[(KeyCode) '9'] = VK_NUMPAD9;
    fixed_mappings[(KeyCode) '/'] = VK_DIVIDE;
    fixed_mappings[(KeyCode) '*'] = VK_MULTIPLY;
    fixed_mappings[(KeyCode) '+'] = VK_ADD;
    fixed_mappings[(KeyCode) '-'] = VK_SUBTRACT;

    vkey_mappings[AUp] = VK_UP;
    vkey_mappings[ADown] = VK_DOWN;
    vkey_mappings[ALeft] = VK_LEFT;
    vkey_mappings[ARight] = VK_RIGHT;
    vkey_mappings[AltUp] = VK_UP;
    vkey_mappings[AltDown] = VK_DOWN;
    vkey_mappings[AltLeft] = VK_LEFT;
    vkey_mappings[AltRight] = VK_RIGHT;
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
    vkey_mappings[PgUp] = VK_PRIOR;
    vkey_mappings[PgDown] = VK_NEXT;

    vkey_mappings[InsertChar] = VK_INSERT;
    vkey_mappings[DeleteChar] = VK_DELETE;

    vkey_mappings[Enter] = VK_RETURN;
}

extern WCHAR local_directory[60000];
extern size_t local_directory_size;

void DFGlue::thread_function()
{
    // Handle and window for process goes here. 
    HANDLE df_process = INVALID_HANDLE_VALUE;
    vector<HWND> df_windows;

    if (!dont_take_running_process)
    {
        // This function should find the window and process for us.
        bool found_process = findDFProcess(&df_process, &df_windows);
        if (!found_process)
        {
            lock_guard<recursive_mutex> alive_lock2(glue_mutex);
            alive = false;
            return;
        }
    }
    else
    // Launch a new DF process
    {
        if (!launchDFProcess(&df_process, &df_windows))
        {
            alive = false;
            return;
        }
    }

    unique_lock<recursive_mutex> alive_lock(glue_mutex);
    this->df_windows = df_windows;
    this->df_handle = df_process;

    bool version = detectDFVersion();
    if (!version)
    {
        if (df_process != INVALID_HANDLE_VALUE && dont_take_running_process) TerminateProcess(df_process, 1);

        LOG(Error, "Can't launch slot because of unknown version.");
        CloseHandle(df_process);
        this->df_handle = INVALID_HANDLE_VALUE;
        this->df_windows.clear();;
        alive = false;
        alive_lock.unlock();
        return;
    }

    string injection_glue_dll = TO_UTF8(local_directory, local_directory_size) + string("\\dfterm_injection_glue.dll");
    string msvcr100dll = TO_UTF8(local_directory, local_directory_size) + string("\\msvcr100.dll");
    string msvcp100dll = TO_UTF8(local_directory, local_directory_size) + string("\\msvcr100.dll");
    LOG(Note, "Injecting " << msvcr100dll << " and " << msvcp100dll << " to process and waiting one second.");
    injectDLL(msvcr100dll);
    injectDLL(msvcp100dll);
    try
    {
        this_thread::sleep(posix_time::time_duration(posix_time::microseconds(1000000LL)));
    }
    catch (const thread_interrupted &ti)
    {
        if (df_process != INVALID_HANDLE_VALUE && dont_take_running_process) TerminateProcess(df_process, 1);

        CloseHandle(df_process);
        this->df_handle = INVALID_HANDLE_VALUE;
        this->df_windows.clear();;
        alive = false;
        alive_lock.unlock();
        return;
    }

    LOG(Note, "Injecting " << injection_glue_dll << " to process.");
    injectDLL(injection_glue_dll);
    LOG(Note, "Waiting 2 seconds for injection to land.");

    if (df_version != 0 && !use_address_settings)
        SendMessage(df_windows, WM_USER, df_version, 4);
    else if (use_address_settings)
    {
        SendMessage(df_windows, 
                    WM_USER, 
                    address_settings.size_address, 5);
        SendMessage(df_windows, 
                    WM_USER, 
                    address_settings.screendata_address, 6);
    }

    try
    {
        this_thread::sleep(posix_time::time_duration(posix_time::microseconds(2000000LL)));
    }
    catch (const thread_interrupted &ti)
    {
        if (df_process != INVALID_HANDLE_VALUE && dont_take_running_process) TerminateProcess(df_process, 1);

        CloseHandle(df_process);
        this->df_handle = INVALID_HANDLE_VALUE;
        this->df_windows.clear();;
        alive = false;
        alive_lock.unlock();
        return;
    }

    alive_lock.unlock();
    try
    {
        while(!isDFClosed())
        {
            this_thread::interruption_point();
            if (use_address_settings && !found_base)
            {
                tryFindBase();
                this_thread::sleep(posix_time::time_duration(posix_time::microseconds(500000LL)));
            }

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
                ui32 keycode = input_queue.front().getKeyCode();
                bool special_key = input_queue.front().isSpecialKey();
                if (input_queue.front().isAltDown()) esc_down_now = true;
                if (input_queue.front().isShiftDown()) shift_down_now = true;
                if (input_queue.front().isCtrlDown()) ctrl_down_now = true;

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
                    else if (keycode == '\n')
                        vkey = VK_RETURN;
                    else
                    {
                        map<KeyCode, DWORD>::iterator i1 = fixed_mappings.find((KeyCode) keycode);
                        if (i1 != fixed_mappings.end())
                            vkey = i1->second;
                        else
                        {
                            SHORT keyscan = VkKeyScanW((WCHAR) keycode);
                            vkey = (DWORD) (keyscan & 0xFF);
                            if ((keyscan & 0x00ff) != -1 && (keyscan & 0xff00))
                                shift_down_now = true;
                            if (keycode == 127) // backspace
                                vkey = VK_BACK;
                        }
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
                    PostMessage(df_windows, WM_USER, 1, 0);
                if (ctrl_down_now)
                    PostMessage(df_windows, WM_USER, 1, 1);
                if (esc_down_now)
                    PostMessage(df_windows, WM_USER, 1, 2);
                PostMessage(df_windows, WM_USER, vkey, 3 + (keycode << 16));
                if (shift_down_now)
                    PostMessage(df_windows, WM_USER, 0, 0);
                if (ctrl_down_now)
                    PostMessage(df_windows, WM_USER, 0, 1);
                if (esc_down_now)
                    PostMessage(df_windows, WM_USER, 0, 2);
            }

            update_mutex.unlock();
            updateDFWindowTerminal();

            {
            SP<State> s = state.lock();
            SP<Slot> self_sp = self.lock();
            if (s && self_sp)
                s->signalSlotData(self_sp);
            }

            this_thread::sleep(posix_time::time_duration(posix_time::microseconds(1000000LL / ticks_per_second)));
        }
    }
    catch (const thread_interrupted &ti)
    {
        
    }
    alive = false;
}

void DFGlue::buildColorFromFloats(float32 r, float32 g, float32 b, Color* color, bool* bold)
{
    assert(color && bold);

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
    if (data_format == PackedVarying && df_w < 256 && df_h < 256)
    {
        ptrdiff_t symbol_address = symbol_pp.getFinalAddress(df_handle);

        ui32 packed_buf[256*256];
        LoggerReadProcessMemory(df_handle, (LPCVOID) symbol_address, packed_buf, df_w*df_h*4, NULL);

        lock_guard<recursive_mutex> lock(glue_mutex);
        if (df_terminal.getWidth() != df_w || df_terminal.getHeight() != df_h)
            df_terminal.resize(df_w, df_h);
        ui32 i1, i2;
        for (i2 = 0; i2 < df_h; ++i2)
            for (i1 = 0; i1 < df_w; ++i1)
            {
                ui32 offset = i1 * df_h + i2;
                Color f_color, b_color;
                bool f_bold = false;
                bool b_bold = false;

                ui8 symbol = (ui8) ((ui32) (packed_buf[i1 * df_h + i2] & 0x000000FF));
                f_color = (Color) ((ui32) (packed_buf[i1 * df_h + i2] & 0x0000FF00) >> 8);
                b_color = (Color) ((ui32) (packed_buf[i1 * df_h + i2] & 0x00FF0000) >> 16);
                f_bold = (Color) ((ui32) (packed_buf[i1 * df_h + i2] & 0xFF000000) >> 24);

                if (f_color == Red) f_color = Blue;
                else if (f_color == Blue) f_color = Red;
                else if (f_color == Yellow) f_color = Cyan;
                else if (f_color == Cyan) f_color = Yellow;
                if (b_color == Red) b_color = Blue;
                else if (b_color == Blue) b_color = Red;
                else if (b_color == Yellow) b_color = Cyan;
                else if (b_color == Cyan) b_color = Yellow;

                df_terminal.setTile(i1, i2, TerminalTile(symbol, f_color, b_color, false, f_bold));
            }
    }
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

        LoggerReadProcessMemory(df_handle, (LPCVOID) symbol_address, symbol_buf, df_w*df_h*4, NULL);
        LoggerReadProcessMemory(df_handle, (LPCVOID) red_address, red_buf, df_w*df_h*4, NULL);
        LoggerReadProcessMemory(df_handle, (LPCVOID) blue_address, blue_buf, df_w*df_h*4, NULL);
        LoggerReadProcessMemory(df_handle, (LPCVOID) green_address, green_buf, df_w*df_h*4, NULL);
        LoggerReadProcessMemory(df_handle, (LPCVOID) red_b_address, red_b_buf, df_w*df_h*4, NULL);
        LoggerReadProcessMemory(df_handle, (LPCVOID) blue_b_address, blue_b_buf, df_w*df_h*4, NULL);
        LoggerReadProcessMemory(df_handle, (LPCVOID) green_b_address, green_b_buf, df_w*df_h*4, NULL);

        lock_guard<recursive_mutex> lock(glue_mutex);
        if (df_terminal.getWidth() != df_w || df_terminal.getHeight() != df_h)
            df_terminal.resize(df_w, df_h);
        ui32 i1, i2;
        for (i2 = 0; i2 < df_h; ++i2)
            for (i1 = 0; i1 < df_w; ++i1)
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
    vector<HWND> window;
    enumDFWindow_struct()
    {
        process_id = 0;
    };
};

// Callback for finding the DF window
BOOL CALLBACK DFGlue::enumDFWindow(HWND hwnd, LPARAM strct)
{
    enumDFWindow_struct* edw = (enumDFWindow_struct*) strct;

    DWORD window_pid;
    GetWindowThreadProcessId(hwnd, &window_pid);
    if (edw->process_id == window_pid)
        edw->window.push_back(hwnd);;

    return TRUE;
}

bool DFGlue::findDFWindow(vector<HWND>* df_windows, DWORD pid)
{
    assert(df_windows);
    (*df_windows).clear();

    // Find the DF window
    enumDFWindow_struct edw;
    edw.process_id = pid;
    EnumWindows(enumDFWindow, (LPARAM) &edw);
    if (edw.window.empty())
        return false;

    (*df_windows) = edw.window;
    return true;
}

bool DFGlue::launchDFProcess(HANDLE* df_process, vector<HWND>* df_windows)
{
    assert(df_process && df_windows);

    /* Wait until "path" and "work" are set for 60 seconds. */
    /* Copied the code from slot_terminal.cc */

    ui8 counter = 60;
    map<string, UnicodeString>::iterator i1;
    while (counter > 0)
    {
        unique_lock<recursive_mutex> lock(glue_mutex);
        if (close_thread)
        {
            lock.unlock();
            return false;
        }

        if (parameters.find("path") != parameters.end() &&
            parameters.find("work") != parameters.end())
            break;
        lock.unlock();
        --counter;
        try
        {
            this_thread::sleep(posix_time::time_duration(posix_time::microseconds(1000000LL)));
        }
        catch (const thread_interrupted &ti)
        {
            return false;
        }
    }
    if (counter == 0)
    {
        lock_guard<recursive_mutex> lock(glue_mutex);
        alive = false;
        return false;
    }

    unique_lock<recursive_mutex> ulock(glue_mutex);
    UnicodeString path = parameters["path"];
    UnicodeString work_dir = parameters["work"];
    ulock.unlock();

    wchar_t path_cstr[MAX_PATH+1], working_path[MAX_PATH+1];
    memset(path_cstr, 0, (MAX_PATH+1)*sizeof(wchar_t));
    memset(working_path, 0, (MAX_PATH+1)*sizeof(wchar_t));
    size_t path_size = MAX_PATH;
    size_t work_size = MAX_PATH;
    TO_WCHAR_STRING(path, path_cstr, &path_size);
    TO_WCHAR_STRING(work_dir, working_path, &work_size);
    
    STARTUPINFO sup;
    memset(&sup, 0, sizeof(STARTUPINFO));
    sup.cb = sizeof(sup);

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    if (!CreateProcessW(NULL, path_cstr, NULL, NULL, FALSE, 0, NULL, working_path, &sup, &pi))
    {
        LOG(Error, "SLOT/DFGlue: CreateProcess() failed with GetLastError() == " << GetLastError());
        return false;
    }

    unique_lock<recursive_mutex> lock(glue_mutex);
    df_terminal.printString("Take it easy!! Game is being launched and attached in 20 seconds.", 1, 1, 7, 0, false, false);
    lock.unlock();

    /* Sleep for 18 seconds before attaching */
    try
    {
        this_thread::sleep(posix_time::time_duration(posix_time::microseconds(18000000LL)));
    }
    catch (const thread_interrupted &ti)
    {
        if (pi.hThread != INVALID_HANDLE_VALUE) CloseHandle(pi.hThread);
        if (pi.hProcess) { UINT dummy = 0; TerminateProcess(pi.hProcess, dummy); };
        CloseHandle(pi.hProcess);
        return false;
    }

    (*df_process) = pi.hProcess;
    if (pi.hThread != INVALID_HANDLE_VALUE) CloseHandle(pi.hThread);

    findDFWindow(df_windows, pi.dwProcessId);
    if ((*df_windows).empty())
    {
        CloseHandle(*df_process);
        return false;
    }

    return true;
};

bool DFGlue::findDFProcess(HANDLE* df_process, vector<HWND>* df_windows)
{
    assert(df_process && df_windows);

    DWORD processes[1000], num_processes;
    EnumProcesses(processes, sizeof(DWORD)*1000, &num_processes);

    num_processes /= sizeof(DWORD);

    HANDLE df_handle = NULL;

    // Check if Dwarf Fortress is running
    int i1;
    for (i1 = 0; i1 < (int) num_processes; ++i1)
    {
        HANDLE h_p = OpenProcess(PROCESS_CREATE_THREAD|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, processes[i1]);
        if (!h_p)
            continue;

        HMODULE h_mod;
        DWORD cb_needed;
        WCHAR process_name[MAX_PATH+1];
        process_name[MAX_PATH] = 0;

        if (EnumProcessModules(h_p, &h_mod, sizeof(h_mod), &cb_needed))
        {
            GetModuleBaseNameW(h_p, h_mod, process_name, sizeof(process_name) / sizeof(TCHAR));

            if (TO_UTF8(process_name) == string("dwarfort.exe") ||
                TO_UTF8(process_name) == string("Dwarf Fortress.exe"))
            {
                df_handle = h_p;
                break;
            }
        }

        CloseHandle(h_p);
    }

    if (!df_handle)
        return false;

    findDFWindow(df_windows, processes[i1]);
    if ((*df_windows).empty())
    {
        CloseHandle(df_handle);
        return false;
    }

    (*df_process) = df_handle;
    return true;
}

void DFGlue::getSize(ui32* width, ui32* height)
{
    assert(width && height);

    lock_guard<recursive_mutex> alive_lock(glue_mutex);
    ui32 w = 80, h = 25;
    (*width) = w;
    (*height) = h;
}

void DFGlue::LoggerReadProcessMemory(HANDLE handle, const void* address, void* target, SIZE_T size, SIZE_T* read_size)
{
    int result = ReadProcessMemory(handle, address, target, size, read_size);
    if (!result && !reported_memory_error)
    {
        reported_memory_error = true;
        int err = GetLastError();
        LOG(Error, "ReadProcessMemory() failed for address " << address << " with GetLastError() == " << err << ". (This error will only be reported once per slot)");
    }
}

void DFGlue::tryFindBase()
{
    WCHAR namebuf[1000];
    namebuf[999] = 0;
    DWORD num_modules;
    HMODULE modules[1000];
    char buf[4096];

    EnumProcessModules(df_handle, modules, 
                       1000 * sizeof(HMODULE), &num_modules);

    for (size_t i1 = 0; i1 < num_modules; ++i1)
    {
        MODULEINFO mi;
        int length = 
            GetModuleBaseNameW(df_handle, modules[i1], namebuf, 999);
        if (!GetModuleInformation(df_handle, modules[i1], &mi,
                    sizeof(mi)))
            continue;

        if (length <= 0)
            continue;

        if (wcscmp(namebuf, L"dfterm_injection_glue.dll"))
            continue;

        for (ptrdiff_t i2 = (ptrdiff_t) mi.lpBaseOfDll;
             i2 < mi.SizeOfImage; i2 += 4096)
        {
            memset(buf, 0, 4096);
            ReadProcessMemory(df_handle, (LPCVOID) i2, 
                              buf, 4096, NULL);

            for (size_t i3 = 0; i3 < 4096; i3++)
            {
                if (!memcmp(&buf[i3], "DFTERM2BASE_", 12))
                {
                    found_base = true;
                    size_pp.pushAddress(address_settings.size_address, 
                                   utf8_image_base_name);
                    symbol_pp.pushAddress((i2+i3+12) - (ptrdiff_t) mi.lpBaseOfDll,
                                   "dfterm_injection_glue.dll");
                    return;
                }
            }
        }
    }
}

void DFGlue::updateWindowSizeFromDFMemory()
{
    lock_guard<recursive_mutex> alive_lock(glue_mutex);
    if (df_handle == INVALID_HANDLE_VALUE) return;

    ui32 w = 0, h = 0;
    ptrdiff_t size_address = size_pp.getFinalAddress(df_handle);
    LoggerReadProcessMemory(df_handle, (LPCVOID) size_address, &w, 4, NULL);
    LoggerReadProcessMemory(df_handle, (LPCVOID) (size_address + 4), &h, 4, NULL);

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
    WCHAR image_file_name[1001];
    memset(image_file_name, 0, 1001);
    WCHAR image_base_name[1001];
    memset(image_base_name, 0, 1001);

    GetModuleFileNameExW(df_handle, NULL, image_file_name, 1000);
    WCHAR* last_p = wcsrchr(image_file_name, L'\\');
    if (!last_p)
        memcpy(image_base_name, image_file_name, 1000 * sizeof(WCHAR));
    else
        wcscpy(image_base_name, last_p + 1);
    utf8_image_base_name = TO_UTF8(image_base_name);

    FILE* f = _wfopen(image_file_name, L"rb");
    if (!f)
        return false;
    /* We read 100000 bytes from the executable */
    char buf[100000];
    memset(buf, 0, 100000);
    fread(buf, 100000, 1, f);
    fclose(f);
    ::uint32_t csum = checksum(buf, 100000);

    {
        SP<State> s = state.lock();
        const vector<AddressSettings32> settings = 
            s->getAddressSettings();

        for (size_t i1 = 0; i1 < settings.size(); ++i1)
        {
            if (settings[i1].checksum == csum)
            {
                use_address_settings = true;
                address_settings = settings[i1];
                LOG(Note, "Configuration file checksum " <<
                          (void*) csum <<
                          "match "
                          "with " << settings[i1].name);
                return true;
            }
        }
    }

    

    PointerPath af;
    PointerPath sz;

    /* NOTE: this isn't the only place where you add new entries for versions.
             You also need to modify dfterm_injection_glue.cc */
    switch(csum)
    {
    case 0xfbff0fc9: /* DF 0.31.25 (SDL) */
    af.pushAddress(0x000060C0, "dfterm_injection_glue.dll");
    sz.pushAddress(0x0073507C, utf8_image_base_name);
    data_format = PackedVarying;
    df_version = 3125;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.25 SDL version)");
    break;
    case 0x19b25b44: /* DF 0.31.19 (SDL) */
    af.pushAddress(0x000060C0, "dfterm_injection_glue.dll");
    sz.pushAddress(0x0074B07C, utf8_image_base_name);
    data_format = PackedVarying;
    df_version = 3119;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.19 SDL version)");
    break;
    case 0xf3527029: /* DF 0.31.18 (SDL) */
    af.pushAddress(0x000060C0, "dfterm_injection_glue.dll");
    sz.pushAddress(0x0071307C, utf8_image_base_name);
    data_format = PackedVarying;
    df_version = 3118;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.18 SDL version)");
    break;
    case 0xbeed6089: /* DF 0.31.17 (SDL) */
    af.pushAddress(0x000060C0, "dfterm_injection_glue.dll");
    sz.pushAddress(0x0071107C, utf8_image_base_name);
    data_format = PackedVarying;
    df_version = 3117;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.17 SDL version)");
    break;
    case 0x92589853: /* DF 0.31.16 (SDL) */
    af.pushAddress(0x000060C0, "dfterm_injection_glue.dll");
    sz.pushAddress(0x013DE914, utf8_image_base_name);
    data_format = PackedVarying;
    df_version = 3116;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.16 SDL version)");
    break;
    case 0x887f99c7: /* DF 0.31.14 (SDL) */
    af.pushAddress(0x000060C0, "dfterm_injection_glue.dll");
    sz.pushAddress(0x013DD8C4, utf8_image_base_name);
    data_format = PackedVarying;
    df_version = 3114;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.14 SDL version)");
    break;
    case 0xad6215b4: /* DF 0.31.13 (SDL) */
    af.pushAddress(0x000060C0, "dfterm_injection_glue.dll");
    sz.pushAddress(0x013DC8BC, utf8_image_base_name);
    data_format = PackedVarying;
    df_version = 3113;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.13 SDL version)");
    break;
    case 0xc6b98be5: /* DF 0.31.12 (SDL) */
    af.pushAddress(0x000060C0, "dfterm_injection_glue.dll");
    sz.pushAddress(0x0142015C, utf8_image_base_name);
    data_format = PackedVarying;
    df_version = 3112;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.12 SDL version)");
    break;
    case 0xc61cac7e: /* DF 0.31.11 (SDL) */
    af.pushAddress(0x000060C0, "dfterm_injection_glue.dll");
    sz.pushAddress(0x0142015C, utf8_image_base_name);
    data_format = PackedVarying;
    df_version = 3111;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.11 SDL version)");
    break;
    case 0xdb942094: /* DF 0.31.10 (SDL) */
    af.pushAddress(0x000060C0, "dfterm_injection_glue.dll");
    sz.pushAddress(0x01419144, utf8_image_base_name);
    data_format = PackedVarying;
    df_version = 3110;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.10 SDL version)");
    break;
    case 0xc58b306c:  /* DF 0.31.09 (SDL) */
    af.pushAddress(0x000060C0, "dfterm_injection_glue.dll");
    sz.pushAddress(0x01419144, utf8_image_base_name);
    data_format = PackedVarying;
    df_version = 3109;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.09 SDL version)");
    break;
    case 0xc4fe6f50:  /* DF 0.31.08 (SDL) */
    af.pushAddress(0x000060C0, "dfterm_injection_glue.dll");
    sz.pushAddress(0x140C11C, utf8_image_base_name);
    data_format = PackedVarying;
    df_version = 3108;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.08 SDL version)");
    break;
    case 0xf6afb6c9:  /* DF 0.31.06 (SDL) */
    af.pushAddress(0x000060C0, "dfterm_injection_glue.dll");
    sz.pushAddress(0x0140B11C, utf8_image_base_name);
    data_format = PackedVarying;
    df_version = 3106;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.06 SDL version)");
    break;
    case 0x9404d33d:  /* DF 0.31.03 */
    af.pushAddress(0x0106FE7C, utf8_image_base_name);
    af.pushAddress(0);
    af.pushAddress(0);
    af.pushAddress(0xC);
    sz.pushAddress(0x013F6B00, utf8_image_base_name);
    red_pp.reset();
    red_pp.pushAddress(0x0106FE7C, utf8_image_base_name);
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
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.03)");
    break;
    case 0xa0b99a67:  /* DF v0.31.02 */
    af.pushAddress(0x0106EE7C, utf8_image_base_name);
    af.pushAddress(0);
    af.pushAddress(0);
    af.pushAddress(0xC);
    sz.pushAddress(0x013F5AC0, utf8_image_base_name);

    red_pp.reset();
    red_pp.pushAddress(0x0106EE7C, utf8_image_base_name);
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
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.02)");
    break;
    case 0xdf6285d3: /* DF v.0.31.01 */
    af.pushAddress(0x0106EE7C, utf8_image_base_name);
    af.pushAddress(0);
    af.pushAddress(0);
    af.pushAddress(0xC);
    sz.pushAddress(0x013F5AC0, utf8_image_base_name);

    red_pp.reset();
    red_pp.pushAddress(0x0106EE7C, utf8_image_base_name);
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
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 0.31.01)");
    break;
    case 0x0b6bf445:  /* DF 40d19.2 */
    af.pushAddress(0x00F32B68, utf8_image_base_name);
    af.pushAddress(0);
    sz.pushAddress(0x0129BF28, utf8_image_base_name);
    data_format = PackedVarying;

    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 40d19.2)");
    break;
    case 0xb548e1b3: /* DF 40d19 */
    af.pushAddress(0x00F31B68, utf8_image_base_name);
    af.pushAddress(0);
    sz.pushAddress(0x0129AF28, utf8_image_base_name);
    data_format = PackedVarying;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 40d19)");
    break;
    case 0x04b75394: /* DF 40d18 */
    af.pushAddress(0x0101D240, utf8_image_base_name);
    sz.pushAddress(0x01417EE8, utf8_image_base_name);
    data_format = Packed256x256;
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (DF 40d18)");
    break;
    default:
    LOG(Note, "Dwarf Fortress executable checksum calculated to " << (void*) csum << " (Unknown, can't show the screen)");
    return false;
    }

    size_pp = sz;
    symbol_pp = af;

    return true;
}

void DFGlue::unloadToWindow(SP<Interface2DWindow> target_window)
{
    assert(alive && target_window);

    lock_guard<recursive_mutex> alive_lock(glue_mutex);
    ui32 t_w, t_h;
    ui32 actual_window_w, actual_window_h;
    t_w = min(df_w, (ui32) df_terminal.getWidth());
    t_h = min(df_h, (ui32) df_terminal.getHeight());
    t_w = min(t_w, (ui32) 256);
    t_h = min(t_h, (ui32) 256);
    target_window->setMinimumSize(t_w, t_h);
    target_window->getSize(&actual_window_w, &actual_window_h);

    ui32 game_offset_x = 0;
    ui32 game_offset_y = 0;
    if (actual_window_w > t_w)
        game_offset_x = (actual_window_w - t_w) / 2;
    if (actual_window_h > t_h)
        game_offset_y = (actual_window_h - t_h) / 2;

    CursesElement* elements = new CursesElement[actual_window_w * actual_window_h];
    ui32 i1, i2;
    for (i1 = 0; i1 < actual_window_w; ++i1)
        for (i2 = 0; i2 < actual_window_h; ++i2)
        {
            TerminalTile t;
            if (i1 < game_offset_x || i2 < game_offset_y || (i1 - game_offset_x) >= t_w || (i2 - game_offset_y) >= t_h)
                t = TerminalTile(' ', 7, 0, false, false);
            else
                t = df_terminal.getTile(i1 - game_offset_x, i2 - game_offset_y);

            ui32 symbol = t.getSymbol();
            if (symbol > 255) symbol = symbol % 256;
            elements[i1 + i2 * actual_window_w] = CursesElement(mapCharacter(symbol), (Color) t.getForegroundColor(), (Color) t.getBackgroundColor(), t.getBold());
        }
    target_window->setScreenDisplayNewElements(elements, sizeof(CursesElement), actual_window_w, actual_window_w, actual_window_h, 0, 0);
    delete[] elements;
}

bool DFGlue::injectDLL(string dllname)
{
    assert(!dllname.empty());
    assert(df_handle != INVALID_HANDLE_VALUE);

    /* Allocate memory for the dll string in the target process */
    LPVOID address = VirtualAllocEx(df_handle, NULL, dllname.size()+1, MEM_COMMIT, PAGE_READWRITE);
    if (!address)
        return false;
    if (!WriteProcessMemory(df_handle, address, 
                            (LPCVOID) dllname.c_str(), dllname.size(), NULL))
        return false;

    HMODULE k32 = GetModuleHandleW(L"Kernel32");
    FARPROC loadlibrary_address = GetProcAddress(k32, "LoadLibraryA");

    HANDLE remote_thread = CreateRemoteThread(df_handle, NULL, 0, (LPTHREAD_START_ROUTINE) loadlibrary_address, address, 0, NULL);
    if (!remote_thread)
        return false;

    return true;
}

void DFGlue::feedInput(const KeyPress &kp)
{
    lock_guard<recursive_mutex> alive_lock(glue_mutex);
    input_queue.push_back(kp);
}

bool DFGlue::isDFClosed()
{
    lock_guard<recursive_mutex> alive_lock(glue_mutex);
    if (df_handle == INVALID_HANDLE_VALUE) return true;

    DWORD exitcode;
    if (GetExitCodeProcess(df_handle, &exitcode))
    {
        if (exitcode != STILL_ACTIVE)
        {
            CloseHandle(df_handle);
            df_handle = INVALID_HANDLE_VALUE;
            LOG(Note, "Game process has closed with exit code " << exitcode << " in slot " << getNameUTF8());
            return true;
        }
    }

    return false;
};

