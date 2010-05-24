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
enum SlotType { DFGrab =   0,        /* Grab a running DF instance from local */
                DFLaunch = 1,        /* Launch a DF process and use that. */
                TerminalLaunch = 2,  /* Launch a DF (or any other terminal) process and use that. */
                InvalidSlotType = 3, /* Have this at the last slot. Other code use this as the last type. */ };
/* Slots DFLaunch and TerminalLaunch need parameters "path" and "work" to be set in the first 60 seconds
 * they were created or slot goes dead. DFGrab needs no parameters. "path" is the path to the DF executable
 * and "work" is the path to DF work directory (so DF can find its files. */

/* And their human-readable names */
const string SlotNames[] = { "Grab a running DF instance.",
                             "Launch a new DF instance.",
                             "Launch a new terminal program instance." };

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
        static SP<Slot> createSlot(SlotType st) { return createSlot(SlotNames[(size_t) st]); };

        /* Sets a generic parameter for slot. (See SlotTypes enum for explanations) 
         * TODO: Maybe roll some RTTI-like system to get real methods like setWorkingDirectory()
         * and setDFExecutablePath() instead of a lousy interface like this to set those. */
        virtual void setParameter(string key, UnicodeString value) = 0;

        /* Returns false if slot is dead and should be thrown away. 
           Slot can die if, for example, the DF session in it closed or could not be started. */
        virtual bool isAlive() = 0;

        /* Returns the current size of the slot (in pointers). 
           If slot is empty, sets them to 80x25. */
        virtual void getSize(ui32* width, ui32* height) = 0;

        /* Unloads current screen to target window.
           This may include a resize to the window. */
        virtual void unloadToWindow(SP<Interface2DWindow> target_window) = 0;

        /* Sends input to the slot. */
        virtual void feedInput(ui32 keycode, bool special_key) = 0;
};

/* Lists slot types. */
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

