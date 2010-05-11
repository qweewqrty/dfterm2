#ifndef slot_hpp
#define slot_hpp

#include "types.hpp"
#include <string>
#include "interface.hpp"

using namespace std;
using namespace trankesbel;

namespace dfterm
{

/* Slot types. */
enum SlotType { DFGrab =   0,    /* Grab a running DF instance from local */
                DFLaunch = 1,    /* Launch a DF process and use that. */
                InvalidSlotType = 2,  /* Have this at the last slot. Other code use this as the last type. */ };

/* And their names */
const string SlotNames[] = { "Grab a running DF instance.",
                             "Launch a new DF instance." };

/* A slot. Slots display game window.
   Slot implements DF program finding and grabbing or pty reading. */
class Slot
{
    private:
        // No copies or default constructors
        Slot(const Slot &s) { };
        Slot& operator==(const Slot &s) { return (*this); };
    protected:
        Slot() { };

    public:
        virtual ~Slot() { };

        /* Creates a slot of given slottype.
           If it fails, returns a null pointer. */
        static SP<Slot> createSlot(string slottype);

        /* Returns false if slot is dead and should be thrown away. 
           Slot can die if, for example, the DF session in it closed or could not be started. */
        virtual bool isAlive() = 0;

        /* Returns the current size of the slot (in pointers). 
           If slot is empty, sets them to 80x25. */
        virtual void getSize(ui32* width, ui32* height) = 0;

        /* Unloads current screen to target window.
           This may include a resize to the window. */
        virtual void unloadToWindow(SP<Interface2DWindow> target_window) = 0;
};

class SlotEnumerator
{
    private:
        ui32 slot;

    public:
        SlotEnumerator() { slot = 0; };
        string getSlotType() 
        {
            if (slot >= (ui32) InvalidSlotType) return string("");
            return SlotNames[slot++]; 
        };
};

};

#endif

