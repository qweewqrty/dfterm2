#include "slot_dfglue.hpp"
#ifndef _WIN32
#error "Windows-only source file."
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <iostream>
#include "logger.hpp"

using namespace dfterm;
using namespace std;
using namespace trankesbel;
using namespace std;

PointerPath::PointerPath()
{
    last_used_module = (HANDLE) 0;
    num_modules = 0;
    reported_error = false;
    needs_search = true;
}

void PointerPath::reset()
{
    address.clear();
    module.clear();
    search_address.clear();
    needs_search = true;
}

void PointerPath::pushAddress(ptrdiff_t offset, string module)
{
    address.push_back(offset);
    this->module.push_back(module);
    search_address.push_back(pair<SearchAddress, ptrdiff_t>(SearchAddress(), 0));
    needs_search = true;
}

void PointerPath::pushSearchAddress(SearchAddress sa, string module, ptrdiff_t end_offset)
{
    address.push_back(0);
    this->module.push_back(module);
    search_address.push_back(pair<SearchAddress, ptrdiff_t>(sa, end_offset));
    needs_search = true;
}

void PointerPath::enumModules(HANDLE df_handle)
{
    last_used_module = df_handle;

    num_modules = 0;
    EnumProcessModules(df_handle, modules, 1000 * sizeof(HMODULE), &num_modules);
    num_modules /= sizeof(HMODULE);

    module_addresses.clear();
    int i1;
    WCHAR namebuf[1000];
    namebuf[999] = 0;

    for (i1 = 0; i1 < num_modules; i1++)
    {
        MODULEINFO mi;

        int length = GetModuleBaseNameW(df_handle, modules[i1], namebuf, 999);
        if (!GetModuleInformation(df_handle, modules[i1], &mi, sizeof(mi)))
            continue;

        if (length > 0)
            module_addresses.insert(pair<string, pair<ptrdiff_t, ptrdiff_t> >(TO_UTF8(namebuf, length), pair<ptrdiff_t, ptrdiff_t>((ptrdiff_t) mi.lpBaseOfDll, (ptrdiff_t) mi.SizeOfImage)));
    }
}

ptrdiff_t PointerPath::searchAddress(HANDLE df_handle, SearchAddress sa, ptrdiff_t baseaddress, ptrdiff_t size)
{
    ptrdiff_t search_size = sa.getSize();
    if (!search_size) return 0;

    ptrdiff_t i1, i2;
    char buf[4096];
    for (i1 = baseaddress; i1 < baseaddress+size; i1 += 4096)
    {
        ReadProcessMemory(df_handle, (LPCVOID) i1, buf, 4096, NULL);

        for (i2 = 0; i2 < 4096 - search_size + 1; i2 += 4)
        {
            if (sa.compareMemory(&buf[i2]))
                return i1 + i2;
        }
    }

    return 0;
}

ptrdiff_t PointerPath::getFinalAddress(HANDLE df_handle)
{
    if (df_handle != last_used_module)
        enumModules(df_handle);

    ptrdiff_t fa = 0;
    ptrdiff_t module_offset;
    map<string, pair<ptrdiff_t, ptrdiff_t> >::iterator i2;

    int i1, len = address.size();
    for (i1 = 0; i1 < len; i1++)
    {
        if (needs_search && search_address[i1].first.getSize() > 0 && module[i1].size() > 0)
        {
            if ( (i2 = module_addresses.find(module[i1])) != module_addresses.end())
            {
                address[i1] = searchAddress(df_handle, search_address[i1].first, i2->second.first, i2->second.second) + search_address[i1].second;
            }
            module_offset = 0;
            module_addresses.erase(module[i1]);
        }
        else if ( (i2 = module_addresses.find(module[i1])) != module_addresses.end())
            module_offset = i2->second.first;
        else
            module_offset = 0;

        if (i1 == 0)
        {
            fa = address[i1] + module_offset;
            continue;
        }

        ptrdiff_t fa_target = 0;
        if (!ReadProcessMemory(df_handle, (LPCVOID) (fa + address[i1] + module_offset), &fa_target, sizeof(ptrdiff_t), NULL))
        {
            DWORD err = GetLastError();

            /* If we have a wrong address or something, it's likely this error will repeat, repeat and repeat.
               Therefore, we limit it to once per class instance. */
            if (!reported_error)
            { LOG(Error, "Pointerpathing: ReadProcessMemory() failed in address " << (LPCVOID) (fa + address[i1] + module_offset) << " with GetLastError() == " << err << ". (This error will be reported only once for this PointerPath -class.)"); }
            reported_error = true;
        }
        else if (!reported_path)
        { LOG(Note, "Pointerpathing: Read process memory from address " << (LPCVOID) (fa + address[i1] + module_offset) << ". Got address " << (LPCVOID) fa_target << ". (Path will be reported only once per PointerPath -class)"); }

        fa = fa_target;
    }
    needs_search = false;
    reported_path = true;
    return fa;
}

