#include "slot.hpp"

#ifdef _WIN32
#include "slot_dfglue.hpp"

#ifndef NO_DFHACK
    #include "slot_dfhack.hpp"
#endif

#else
#include "slot_terminal.hpp"
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
    if (i1 >= num_slots) return SP<Slot>();

    SP<Slot> result;

    switch((SlotType) i1)
    {
        #ifdef __WIN32
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
        #endif

        default:
            return SP<Slot>();
    };

    result->setSelf(result);
    return result;
}

