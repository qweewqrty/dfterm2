#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <windows.h>
#include <psapi.h>
#include <sstream>

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

void patch_function(ptrdiff_t p1, ptrdiff_t p2, char* p3);

void restore_old_function(ptrdiff_t patched_function_addr, char* patch)
{
    WriteProcessMemory(me_process, (void*) patched_function_addr, patch, 6, NULL); 
}

unsigned char buffer[500*500];

ptrdiff_t last_read_address = 0;

int WINAPI hooked_SDLNumJoysticks()
{
    if (set_buffer_address && buffer_address == 3108)
    {
        unsigned int w, h;
        ReadProcessMemory(me_process, (void*) (0x140C11C+dwarfort_base), &w, sizeof(int), NULL);
        ReadProcessMemory(me_process, (void*) (0x140C11C+dwarfort_base+sizeof(int)), &h, sizeof(int), NULL);

        ptrdiff_t final_address = 0x0141D390+dwarfort_base;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        ReadProcessMemory(me_process, (void*) last_read_address, buffer, w*h*sizeof(int), NULL);
        last_read_address = final_address;
    }
    else if (set_buffer_address && buffer_address == 3106)
    {
        unsigned int w, h;
        ReadProcessMemory(me_process, (void*) (0x140B11C+dwarfort_base), &w, sizeof(int), NULL);
        ReadProcessMemory(me_process, (void*) (0x140B11C+dwarfort_base+sizeof(int)), &h, sizeof(int), NULL);

        ptrdiff_t final_address = 0x0141C390+dwarfort_base;
        ReadProcessMemory(me_process, (void*) final_address, (void*) &final_address, sizeof(ptrdiff_t), NULL);
        ReadProcessMemory(me_process, (void*) last_read_address, buffer, w*h*sizeof(int), NULL);
        last_read_address = final_address;
    }

    restore_old_function(SDLNumJoysticks_addr, SDLNumJoysticks_patch);
    int result = ((int (*)()) SDLNumJoysticks_addr)();
    patch_function(SDLNumJoysticks_addr, (ptrdiff_t) hooked_SDLNumJoysticks, NULL);
    return result;
}

LRESULT WINAPI hooked_DefWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
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
        else if (lparam == 3)
        {
            SendMessage(hwnd, WM_KEYDOWN, wparam, 0x0);
            SendMessage(hwnd, WM_KEYUP, wparam, 0xC0000001);
        }
        else if (lparam == 4)
        {
            buffer_address = (ptrdiff_t) wparam;
            set_buffer_address = true;
            if (!hooked_erase)
            {
                HMODULE df_module = GetModuleHandleW(L"Dwarf Fortress.exe");
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

