#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <windows.h>

extern "C"
{

char GetKeyState_patch[6];
ptrdiff_t GetKeyState_addr;
char GetAsyncKeyState_patch[6];
ptrdiff_t GetAsyncKeyState_addr;
char DefWindowProc_patch[6];
ptrdiff_t DefWindowProc_addr;

volatile int shift_state = 0;
volatile int control_state = 0;
volatile int alt_state = 0;

HANDLE me_process;

void patch_function(ptrdiff_t p1, ptrdiff_t p2, char* p3);

void restore_old_function(ptrdiff_t patched_function_addr, char* patch)
{
    WriteProcessMemory(me_process, (void*) patched_function_addr, patch, 6, NULL); 
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
            SendMessage(hwnd, WM_KEYDOWN, wparam, 0);
            SendMessage(hwnd, WM_KEYUP, wparam, 0);
        }
    }
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

    char buf[6];

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

    /* Hook GetAsyncKeyState */
    HMODULE module = GetModuleHandle("User32");
    LPCVOID gaks = (LPCVOID) GetProcAddress(module, "GetKeyState");
    HMODULE module3 = GetModuleHandle("User32");
    LPCVOID gaks2 = (LPCVOID) GetProcAddress(module3, "GetAsyncKeyState");
    HMODULE module2 = GetModuleHandle("Ntdll");
    LPCVOID dwp = (LPCVOID) GetProcAddress(module, "DefWindowProcA");
    GetKeyState_addr = (ptrdiff_t) gaks;
    DefWindowProc_addr = (ptrdiff_t) dwp;
    GetAsyncKeyState_addr = (ptrdiff_t) gaks2;
    patch_function(GetKeyState_addr, (ptrdiff_t) hooked_GetKeyState, GetKeyState_patch);
    patch_function(GetAsyncKeyState_addr, (ptrdiff_t) hooked_GetAsyncKeyState, GetAsyncKeyState_patch);
    patch_function(DefWindowProc_addr, (ptrdiff_t) hooked_DefWindowProc, DefWindowProc_patch);

    return TRUE;
}

}

