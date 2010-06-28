#ifndef slot_hpp
#define slot_hpp

namespace dfterm
{
    class Slot;

    /* Slot types. */
    enum SlotType { DFGrab =   0,        /* Grab a running DF instance from local */
                    DFLaunch = 1,        /* Launch a DF process and use that. */
                    TerminalLaunch = 2,  /* Launch a DF (or any other terminal) process and use that. */
                    InvalidSlotType = 3, /* Have this at the last slot. Other code use this as the last type. */ };
    /* Slots DFLaunch and TerminalLaunch need parameters "path" and "work" to be set in the first 60 seconds
     * they were created or slot goes dead. DFGrab needs no parameters. "path" is the path to the DF executable
     * and "work" is the path to DF work directory (so DF can find its files. */

};

#include "types.hpp"
#include <string>
#include "interface.hpp"
#include "configuration_interface.hpp"
#include "state.hpp"

namespace dfterm
{

/* And their human-readable names */
const std::string SlotNames[] = { "Grab a running DF instance.",
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

        UnicodeString name;
        WP<SlotProfile> slotprofile;
        WP<User> user;
        WP<User> last_player;
        
        ID id;

        void setSelf(WP<Slot> s) { self = s; };

    protected:
        Slot() { };

        WP<State> state;
        WP<Slot> self;

    public:
        virtual ~Slot() { };
        
        const ID& getIDRef() const { return id; };
        ID getID() const { return id; };
        void setID(const ID& id) { this->id = id; };

        /* Creates a slot of given slottype.
           If it fails, returns a null pointer. */
        static SP<Slot> createSlot(std::string slottype);
        static SP<Slot> createSlot(SlotType st) { return createSlot(SlotNames[(size_t) st]); };

        /* Sets/gets the slot profile used to create this slot. */
        void setSlotProfile(WP<SlotProfile> sp) { slotprofile = sp; };
        WP<SlotProfile> getSlotProfile() { return slotprofile; };

        /* Sets who last fed input to this slot. */
        void setLastUser(WP<User> user) { this->last_player = user; };
        WP<User> getLastUser() { return last_player; };

        /* Sets/gets the launcher of this slot. */
        void setLauncher(WP<User> user) { this->user = user; };
        WP<User> getLauncher() { return user; };

        /* Sets/gets the state this slot belongs to. */
        void setState(WP<State> state) { this->state = state; };
        WP<State> getState() { return state; };

        /* Gives/retrieves a name of the slot. */
        void setName(UnicodeString name) { this->name = name; };
        void setNameUTF8(std::string name) { this->name = UnicodeString::fromUTF8(name); };
        UnicodeString getName() const { return name; };
        std::string getNameUTF8() const { return TO_UTF8(name); };

        /* Sets a generic parameter for slot. (See SlotTypes enum for explanations) 
         * TODO: Maybe roll some RTTI-like system to get real methods like setWorkingDirectory()
         * and setDFExecutablePath() instead of a lousy interface like this to set those. */
        virtual void setParameter(std::string key, UnicodeString value) = 0;

        /* Returns false if slot is dead and should be thrown away. 
           Slot can die if, for example, the DF session in it closed or could not be started. */
        virtual bool isAlive() = 0;

        /* Returns the current size of the slot (in pointers). 
           If slot is empty, sets them to 80x25. */
        virtual void getSize(trankesbel::ui32* width, trankesbel::ui32* height) = 0;

        /* Unloads current screen to target window.
           This may include a resize to the window. */
        virtual void unloadToWindow(SP<trankesbel::Interface2DWindow> target_window) = 0;

        /* Sends input to the slot. */
        virtual void feedInput(trankesbel::ui32 keycode, bool special_key) = 0;
};

/* Lists slot types. */
class SlotEnumerator
{
    private:
        trankesbel::ui32 slot;

    public:
        SlotEnumerator() { slot = 0; };
        std::string getSlotType() 
        {
            if (slot >= (trankesbel::ui32) InvalidSlotType) return std::string("");
            return SlotNames[slot++]; 
        };
};

};

#endif

