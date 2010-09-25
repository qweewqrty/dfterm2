#include "slot.hpp"

#ifdef _WIN32
#include "slot_dfglue.hpp"

#ifndef NO_DFHACK
    #include "slot_dfhack.hpp"
#endif

#else
#include "slot_terminal.hpp"

#ifndef NO_DFHACK
    #include "slot_dfhack_linux.hpp"
#endif

#endif

using namespace dfterm;
using namespace std;
using namespace trankesbel;

SP<Slot> Slot::createSlot(string slottype)
{
    ui32 num_slots = (ui32) InvalidSlotType;
    ui32 i1;
    for (i1 = 0; i1 < num_slots; ++i1)
        if (slottype == SlotNames[i1])
            break;
    if (i1 >= num_slots) 
    {
        LOG(Error, "Tried to create a slot with unknown slot type name \"" << slottype << "\"");
        return SP<Slot>();
    }

    SP<Slot> result;

    switch((SlotType) i1)
    {
        #ifdef _WIN32
        case DFGrab:
            result = SP<Slot>(new DFGlue);
        break;
        case DFLaunch:
            result = SP<Slot>(new DFGlue(true));
        break;
        #ifndef NO_DFHACK
        case DFGrabHackSlot:
            result = SP<Slot>(new DFHackSlot);
        break;
        #endif

        #else
        case TerminalLaunch:
            result = SP<Slot>(new TerminalGlue);
        break;
        #ifndef NO_DFHACK
        case DFGrabHackSlotLinux:
            result = SP<Slot>(new DFHackSlotLinux);
        break;
        case LaunchDFHackSlotLinux:
            result = SP<Slot>(new DFHackSlotLinux(true));
        break;
        #endif

        #endif

        default:
            LOG(Error, "Tried to create a slot with unknown slot type number " << (int) i1);
            return SP<Slot>();
    };

    result->setSelf(result);
    return result;
}

