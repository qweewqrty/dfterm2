#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <windows.h>
#include <psapi.h>
#include <sstream>
#include <stdint.h>
#include <map>
#include <set>
#include <deque>

using namespace std;

bool set_buffer_address = false;
ptrdiff_t buffer_address = 0;

bool hooked_erase = false;

ptrdiff_t dwarfort_base = 0;

extern "C"
{

char GetKeyState_patch[6];
ptrdiff_t GetKeyState_addr;
char GetKeyboardState_patch[6];
ptrdiff_t GetKeyboardState_addr;
char GetAsyncKeyState_patch[6];
ptrdiff_t GetAsyncKeyState_addr;
char DefWindowProc_patch[6];
ptrdiff_t DefWindowProc_addr;
char SDLNumJoysticks_patch[6];
ptrdiff_t SDLNumJoysticks_addr;

char erasescreen_patch[6];
ptrdiff_t erasescreen_addr;

volatile int shift_state = 0;
volatile int control_state = 0;
volatile int alt_state = 0;

HANDLE me_process;
set<HWND> hwnds;

struct KeySym
{
    unsigned char scancode;
    uint32_t keycode;
    uint32_t mod;
    uint16_t unicode;
};

const int KEYDOWN = 2;
const int KEYUP = 3;

const int RELEASED = 0;
const int PRESSED = 1;

const int SDLK_KP0 = 256;
const int SDLK_KP1 = 257;
const int SDLK_KP2 = 258;
const int SDLK_KP3 = 259;
const int SDLK_KP4 = 260;
const int SDLK_KP5 = 261;
const int SDLK_KP6 = 262;
const int SDLK_KP7 = 263;
const int SDLK_KP8 = 264;
const int SDLK_KP9 = 265;
const int SDLK_KP_PERIOD = 266;
const int SDLK_KP_DIVIDE = 267;
const int SDLK_KP_MULTIPLY = 268;
const int SDLK_KP_MINUS = 269;
const int SDLK_KP_PLUS = 270;
const int SDLK_KP_ENTER = 271;
const int SDLK_KP_EQUALS = 272;
const int SDLK_UP = 273;
const int SDLK_DOWN = 274;
const int SDLK_RIGHT = 275;
const int SDLK_LEFT = 276;
const int SDLK_INSERT = 277;
const int SDLK_HOME = 278;
const int SDLK_END = 279;
const int SDLK_PAGEUP = 280;
const int SDLK_PAGEDOWN = 281;
const int SDLK_F1 = 282;
const int SDLK_F2 = 283;
const int SDLK_F3 = 284;
const int SDLK_F4 = 285;
const int SDLK_F5 = 286;
const int SDLK_F6 = 287;
const int SDLK_F7 = 288;
const int SDLK_F8 = 289;
const int SDLK_F9 = 290;
const int SDLK_F10 = 291;
const int SDLK_F11 = 292;
const int SDLK_F12 = 293;
const int SDLK_F13 = 294;
const int SDLK_F14 = 295;
const int SDLK_F15 = 296;
const int SDLK_RSHIFT = 303;
const int SDLK_LSHIFT = 304;
const int SDLK_RCTRL = 305;
const int SDLK_LCTRL = 306;
const int SDLK_RALT	= 307;
const int SDLK_LALT = 308;

struct KeyboardEvent
{
    unsigned char type;
    unsigned char state;
    KeySym keysym;

    bool is_windows_key;
    HWND hwnd;
    UINT msg;
    WPARAM wparam;
    LPARAM lparam;
    char dummy[2000]; /* To pad the structure size a little */
    
    KeyboardEvent()
    {
        is_windows_key = false;
    }
    KeyboardEvent(const KeyboardEvent &ke)
    {
        (*this) = ke;
    }
    KeyboardEvent& operator=(const KeyboardEvent &ke)
    {
        if (this == &ke) return (*this);

        type = ke.type;
        state = ke.state;
        keysym = ke.keysym;
        is_windows_key = ke.is_windows_key;
        msg = ke.msg;
        wparam = ke.wparam;
        lparam = ke.lparam;
        memcpy(dummy, ke.dummy, 2000);
    }
};

map<DWORD, uint32_t> vk_to_keycode;

int (__cdecl *SDL_PushEvent)(KeyboardEvent* ke);

void initialize_vk_to_keycode_map()
{
    /* Letters are their ASCII equivalents,
       uppercase in MSDN virtual key codes,
       lowercase in SDL virtual key codes. */
    uint32_t i1;
    for (i1 = 'A'; i1 <= 'Z'; ++i1)
        vk_to_keycode[i1] = i1 - 'A' + 'a';

    /* Same for numbers */
    for (i1 = '0'; i1 <= '9'; i1++)
        vk_to_keycode[i1] = i1;

    vk_to_keycode[VK_PRIOR] = SDLK_PAGEUP;
    vk_to_keycode[VK_NEXT] = SDLK_PAGEDOWN;
    vk_to_keycode[VK_LEFT] = SDLK_LEFT;
    vk_to_keycode[VK_RIGHT] = SDLK_RIGHT;
    vk_to_keycode[VK_UP] = SDLK_UP;
    vk_to_keycode[VK_DOWN] = SDLK_DOWN;

    vk_to_keycode[VK_F1] = SDLK_F1;
    vk_to_keycode[VK_F2] = SDLK_F2;
    vk_to_keycode[VK_F3] = SDLK_F3;
    vk_to_keycode[VK_F4] = SDLK_F4;
    vk_to_keycode[VK_F5] = SDLK_F5;
    vk_to_keycode[VK_F6] = SDLK_F6;
    vk_to_keycode[VK_F7] = SDLK_F7;
    vk_to_keycode[VK_F8] = SDLK_F8;
    vk_to_keycode[VK_F9] = SDLK_F9;
    vk_to_keycode[VK_F10] = SDLK_F10;
    vk_to_keycode[VK_F11] = SDLK_F11;
    vk_to_keycode[VK_F12] = SDLK_F12;

    vk_to_keycode[VK_RETURN] = '\r';
    vk_to_keycode[VK_TAB] = 9;
    vk_to_keycode[VK_PAUSE] = 19;
    vk_to_keycode[VK_ESCAPE] = 27;
    vk_to_keycode[VK_SPACE] = ' ';
}

void patch_function(ptrdiff_t p1, ptrdiff_t p2, char* p3);

void restore_old_function(ptrdiff_t patched_function_addr, char* patch)
{
    WriteProcessMemory(me_process, (void*) patched_function_addr, patch, 6, NULL); 
}

unsigned char buffer[500*500];

ptrdiff_t last_read_address = 0;

deque<KeyboardEvent> events;
HANDLE events_mutex;

int WINAPI hooked_SDLNumJoysticks()
{
    /*DWORD ownership = WaitForSingleObject(events_mutex, INFINITE);
    while (!events.empty())
    {
        KeyboardEvent ke = events.front();
        events.pop_front();

        if (ke.is_windows_key == false)
            SDL_PushEvent(&ke);
        else
            SendMessage(ke.hwnd, ke.msg, ke.wparam, ke.lparam);
    }
    ReleaseMutex(events_mutex);*/
    if (set_buffer_address && buffer_address == 3117)
    {
        unsigned int w = 1, h = 1;
        ReadProcessMemory(me_process, (void*) (0x71107C+dwarfort_base), &w, sizeof(unsigned int), NULL);
        ReadProcessMemory(me_process, (void*) (0x71107C+dwarfort_base+sizeof(int)), &h, sizeof(unsigned int), NULL);

        ptrdiff_t final_address = 0x00710D1C+dwarfort_base;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        ReadProcessMemory(me_process, (void*) last_read_address, buffer, w*h*sizeof(int), NULL);
        last_read_address = final_address;
    }
    if (set_buffer_address && buffer_address == 3116)
    {
        unsigned int w = 1, h = 1;
        ReadProcessMemory(me_process, (void*) (0x13DE914+dwarfort_base), &w, sizeof(unsigned int), NULL);
        ReadProcessMemory(me_process, (void*) (0x13DE914+dwarfort_base+sizeof(int)), &h, sizeof(unsigned int), NULL);
        
        ptrdiff_t final_address = 0x01082694+dwarfort_base;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        final_address += 0x1C;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        ReadProcessMemory(me_process, (void*) last_read_address, buffer, w*h*sizeof(int), NULL);
        last_read_address = final_address;
    }
    if (set_buffer_address && buffer_address == 3114)
    {
        unsigned int w = 1, h = 1;
        ReadProcessMemory(me_process, (void*) (0x13DD8C4+dwarfort_base), &w, sizeof(unsigned int), NULL);
        ReadProcessMemory(me_process, (void*) (0x13DD8C4+dwarfort_base+sizeof(int)), &h, sizeof(unsigned int), NULL);
        
        ptrdiff_t final_address = 0x01081694+dwarfort_base;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        final_address += 0x4;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        ReadProcessMemory(me_process, (void*) last_read_address, buffer, w*h*sizeof(int), NULL);
        last_read_address = final_address;
    }
    if (set_buffer_address && buffer_address == 3113)
    {
        unsigned int w = 1, h = 1;
        ReadProcessMemory(me_process, (void*) (0x13DC8BC+dwarfort_base), &w, sizeof(unsigned int), NULL);
        ReadProcessMemory(me_process, (void*) (0x13DC8BC+dwarfort_base+sizeof(int)), &h, sizeof(unsigned int), NULL);

        ptrdiff_t final_address = 0x01080694+dwarfort_base;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        final_address += 0x1C;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        ReadProcessMemory(me_process, (void*) last_read_address, buffer, w*h*sizeof(int), NULL);
        last_read_address = final_address;
    }
    if (set_buffer_address && buffer_address == 3112)
    {
        unsigned int w = 1, h = 1;
        ReadProcessMemory(me_process, (void*) (0x142015C+dwarfort_base), &w, sizeof(unsigned int), NULL);
        ReadProcessMemory(me_process, (void*) (0x142015C+dwarfort_base+sizeof(int)), &h, sizeof(unsigned int), NULL);

        ptrdiff_t final_address = 0x010BEB44+dwarfort_base;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        final_address += 0x1C;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        ReadProcessMemory(me_process, (void*) last_read_address, buffer, w*h*sizeof(int), NULL);
        last_read_address = final_address;
    }
    if (set_buffer_address && buffer_address == 3111)
    {
        unsigned int w = 1, h = 1;
        ReadProcessMemory(me_process, (void*) (0x142015C+dwarfort_base), &w, sizeof(unsigned int), NULL);
        ReadProcessMemory(me_process, (void*) (0x142015C+dwarfort_base+sizeof(int)), &h, sizeof(unsigned int), NULL);

        ptrdiff_t final_address = 0x014313D0+dwarfort_base;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        ReadProcessMemory(me_process, (void*) last_read_address, buffer, w*h*sizeof(int), NULL);
        last_read_address = final_address;
    }
    if (set_buffer_address && (buffer_address == 3109 || buffer_address == 3110))
    {
        unsigned int w = 1, h = 1;
        ReadProcessMemory(me_process, (void*) (0x1419144+dwarfort_base), &w, sizeof(unsigned int), NULL);
        ReadProcessMemory(me_process, (void*) (0x1419144+dwarfort_base+sizeof(int)), &h, sizeof(unsigned int), NULL);

        ptrdiff_t final_address = 0x010B7B44+dwarfort_base;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        final_address += 0x4;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        ReadProcessMemory(me_process, (void*) last_read_address, buffer, w*h*sizeof(int), NULL);
        last_read_address = final_address;
    }
    else if (set_buffer_address && buffer_address == 3108)
    {
        unsigned int w = 1, h = 1;
        ReadProcessMemory(me_process, (void*) (0x140C11C+dwarfort_base), &w, sizeof(int), NULL);
        ReadProcessMemory(me_process, (void*) (0x140C11C+dwarfort_base+sizeof(int)), &h, sizeof(int), NULL);

        ptrdiff_t final_address = 0x0141D390+dwarfort_base;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        ReadProcessMemory(me_process, (void*) last_read_address, buffer, w*h*sizeof(int), NULL);
        last_read_address = final_address;
    }
    else if (set_buffer_address && buffer_address == 3106)
    {
        unsigned int w = 1, h = 1;
        ReadProcessMemory(me_process, (void*) (0x140B11C+dwarfort_base), &w, sizeof(int), NULL);
        ReadProcessMemory(me_process, (void*) (0x140B11C+dwarfort_base+sizeof(int)), &h, sizeof(int), NULL);

        ptrdiff_t final_address = 0x0141C390+dwarfort_base;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        ReadProcessMemory(me_process, (void*) last_read_address, buffer, w*h*sizeof(int), NULL);
        last_read_address = final_address;
    }

    /* Don't patch SDLNumJoystics back */
    restore_old_function(SDLNumJoysticks_addr, SDLNumJoysticks_patch);
    int result = ((int (*)()) SDLNumJoysticks_addr)();
    patch_function(SDLNumJoysticks_addr, (ptrdiff_t) hooked_SDLNumJoysticks, NULL);
    return 0;
}

LRESULT WINAPI hooked_DefWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    hwnds.insert(hwnd);

    restore_old_function(DefWindowProc_addr, DefWindowProc_patch);
    LRESULT result = DefWindowProc(hwnd, msg, wparam, lparam);
    patch_function(DefWindowProc_addr, (ptrdiff_t) hooked_DefWindowProc, NULL);
    if (msg == WM_USER)
    {
        if (lparam == 0)
        {
            if (wparam & 0x1)
                shift_state = 1;
            else
                shift_state = 0;
        }
        else if (lparam == 1)
        {
            if (wparam & 0x1)
                control_state = 1;
            else
                control_state = 0;
        }
        else if (lparam == 2)
        {
            if (wparam & 0x1)
                alt_state = 1;
            else
                alt_state = 0;
        }
        else if ((lparam & 0x0000ffff) == 3)
        {
            if (alt_state)
                SendMessage(hwnd, WM_KEYDOWN, VK_LMENU, 0);
            if (control_state)
                SendMessage(hwnd, WM_KEYDOWN, VK_LCONTROL, 0);
            if (shift_state)
                SendMessage(hwnd, WM_KEYDOWN, VK_LSHIFT, 0);

            SendMessage(hwnd, WM_KEYDOWN, wparam, 0);
            SendMessage(hwnd, WM_KEYUP, wparam, 0xC0000001);

            if (alt_state)
                SendMessage(hwnd, WM_KEYUP, VK_LMENU, 0xC0000001);
            if (control_state)
                SendMessage(hwnd, WM_KEYUP, VK_LCONTROL, 0xC0000001);
            if (shift_state)
                SendMessage(hwnd, WM_KEYUP, VK_LSHIFT, 0xC0000001);
        }
        else if (lparam == 4)
        {
            buffer_address = (ptrdiff_t) wparam;
            set_buffer_address = true;
            if (!hooked_erase)
            {
                HMODULE df_module = GetModuleHandleW(NULL);
                if (df_module)
                {
                    MODULEINFO mi;
                    if (GetModuleInformation(me_process, df_module, &mi, sizeof(mi)))
                    {
                        dwarfort_base = (ptrdiff_t) mi.lpBaseOfDll;
                        if (wparam == 3106)
                            erasescreen_addr = 0x002fa600 + (ptrdiff_t) mi.lpBaseOfDll;
                        else if (wparam == 3108)
                            erasescreen_addr = 0x0015DE50 + (ptrdiff_t) mi.lpBaseOfDll;
                        else
                            return result;
                        //patch_function(erasescreen_addr, (ptrdiff_t) fake_graphics_31_06__erasescreen, erasescreen_patch);
                        hooked_erase = true;
                    };
                }
            }
        }
    }
    return result;
}

BOOL WINAPI hooked_GetKeyboardState(PBYTE lpKeyState)
{
    restore_old_function(GetKeyboardState_addr, GetKeyboardState_patch);
    BOOL result = GetKeyboardState(lpKeyState);
    patch_function(GetKeyboardState_addr, (ptrdiff_t) hooked_GetKeyboardState, NULL);

    if (shift_state)
        lpKeyState[VK_SHIFT] = 0x80;
    if (control_state)
        lpKeyState[VK_CONTROL] = 0x80;
    if (alt_state)
        lpKeyState[VK_MENU] = 0x80;
    return result;
}

SHORT WINAPI hooked_GetKeyState(int vKey)
{
    restore_old_function(GetKeyState_addr, GetKeyState_patch);
    SHORT result = GetKeyState(vKey);
    patch_function(GetKeyState_addr, (ptrdiff_t) hooked_GetKeyState, NULL); 
    if (vKey == VK_SHIFT && shift_state)
        return shift_state << 7;
    if (vKey == VK_CONTROL && control_state)
        return control_state << 7;
    if (vKey == VK_MENU && alt_state)
        return alt_state << 7;
    return result;
}

SHORT WINAPI hooked_GetAsyncKeyState(int vKey)
{
    restore_old_function(GetAsyncKeyState_addr, GetAsyncKeyState_patch);
    SHORT result = GetAsyncKeyState(vKey);
    patch_function(GetAsyncKeyState_addr, (ptrdiff_t) hooked_GetAsyncKeyState, NULL); 
    if (vKey == VK_SHIFT && shift_state)
        return shift_state << 7;
    if (vKey == VK_CONTROL && control_state)
        return control_state << 7;
    if (vKey == VK_MENU && alt_state)
        return alt_state << 7;
    return result;
}

void patch_function(ptrdiff_t patched_function_addr, ptrdiff_t inject_function_addr, char* old_patch)
{
    char* n = (char*) 0;
    if (old_patch)
        memcpy(old_patch, (void*) patched_function_addr, 6);

    unsigned char buf[6];

    buf[0] = 0x68; /* push (32-bit) */
    buf[1] = (char) ((ptrdiff_t) inject_function_addr & 0x000000ff);
    buf[2] = (char) (((ptrdiff_t) inject_function_addr & 0x0000ff00) >> 8);
    buf[3] = (char) (((ptrdiff_t) inject_function_addr & 0x00ff0000) >> 16);
    buf[4] = (char) (((ptrdiff_t) inject_function_addr & 0xff000000) >> 24);
    buf[5] = 0xC3; /* ret */

    if (!WriteProcessMemory(me_process, (void*) patched_function_addr, buf, 6, NULL))
    {
    }
}

BOOL WINAPI DllMain(HINSTANCE hi, DWORD reason, LPVOID reserved)
{
    if (reason != DLL_PROCESS_ATTACH)
        return FALSE;

    initialize_vk_to_keycode_map();
    events_mutex = CreateMutex(NULL, FALSE, NULL);

    me_process = GetCurrentProcess();

    /* Hook GetAsyncKeyState and friends */
    HMODULE module = GetModuleHandleW(L"User32");
    LPCVOID gaks = (LPCVOID) GetProcAddress(module, "GetKeyState");
    HMODULE module4 = GetModuleHandleW(L"User32");
    LPCVOID gaks4 = (LPCVOID) GetProcAddress(module4, "GetKeyboardState");
    HMODULE module3 = GetModuleHandleW(L"User32");
    LPCVOID gaks2 = (LPCVOID) GetProcAddress(module3, "GetAsyncKeyState");
    HMODULE module2 = GetModuleHandleW(L"Ntdll");
    LPCVOID dwp = (LPCVOID) GetProcAddress(module, "DefWindowProcA");
    
    HMODULE module5 = GetModuleHandleW(L"SDL");
    LPCVOID gaks5 = GetProcAddress(module5, "SDL_NumJoysticks");
    SDL_PushEvent = (int (__cdecl*)(KeyboardEvent*)) GetProcAddress(module5, "SDL_PushEvent");

    GetKeyState_addr = (ptrdiff_t) gaks;
    DefWindowProc_addr = (ptrdiff_t) dwp;
    GetAsyncKeyState_addr = (ptrdiff_t) gaks2;
    GetKeyboardState_addr = (ptrdiff_t) gaks4;
    SDLNumJoysticks_addr = (ptrdiff_t) gaks5;
    patch_function(GetKeyState_addr, (ptrdiff_t) hooked_GetKeyState, GetKeyState_patch);
    patch_function(GetKeyboardState_addr, (ptrdiff_t) hooked_GetKeyboardState, GetKeyboardState_patch);
    patch_function(GetAsyncKeyState_addr, (ptrdiff_t) hooked_GetAsyncKeyState, GetAsyncKeyState_patch);
    patch_function(DefWindowProc_addr, (ptrdiff_t) hooked_DefWindowProc, DefWindowProc_patch);
    patch_function(SDLNumJoysticks_addr, (ptrdiff_t) hooked_SDLNumJoysticks, SDLNumJoysticks_patch);


    return TRUE;
}

}

