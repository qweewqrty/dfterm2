#include "slot.hpp"

#ifdef __WIN32
#include "slot_dfglue.hpp"
#else
#include "slot_terminal.hpp"
#endif

using namespace dfterm;
using namespace std;

SP<Slot> Slot::createSlot(string slottype)
{
    ui32 num_slots = (ui32) InvalidSlotType;
    ui32 i1;
    SlotType t = InvalidSlotType;
    for (i1 = 0; i1 < num_slots; i1++)
        if (slottype == SlotNames[i1])
            break;
    if (i1 >= num_slots) return SP<Slot>();

    switch((SlotType) i1)
    {
        #ifdef __win32
        case DFGrab:
            return SP<Slot>(new DFGlue);
        break;
        case DFLaunch:
            return SP<Slot>(new DFGlue);
        break;
        #else
        case TerminalLaunch:
            return SP<Slot>(new TerminalGlue);
        break;
        #endif

        default:
            return SP<Slot>();
    };
    return SP<Slot>();
}

