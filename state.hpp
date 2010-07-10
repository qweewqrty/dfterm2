#ifndef state_hpp
#define state_hpp

namespace dfterm {
class State;
};

#include "types.hpp"
#include <set>
#include "sockets.hpp"
#include "logger.hpp"
#include "client.hpp"
#include "configuration_interface.hpp"
#include "configuration_primitives.hpp"
#include <sstream>
#include "lockedresource.hpp"

namespace dfterm {

/* Holds the state of the dfterm2 process.
 * All clients, slots etc. */
class State
{
    private:
        State();

        /* No copies */
        State(const State& s) { };
        State& operator=(const State &s) { return (*this); };

        /* Self */
        WP<State> self;
        
        /* The MOTD */
        UnicodeString MOTD;

        /* Maximum slots */
        trankesbel::ui32 maximum_slots;

        std::set<SP<trankesbel::Socket> > listening_sockets;

        /* This is the list of connected clients. */
        LockedResource<std::vector<SP<Client> > > clients;
        /* And weak pointers to them. */
        LockedResource<std::vector<WP<Client> > > clients_weak;
        
        /* The global chat logger */
        SP<Logger> global_chat;

        /* Database */
        SP<ConfigurationDatabase> configuration;
        boost::recursive_mutex configuration_mutex; /* configuration itself is thread-safe, but setting it is not */

        /* Running slots */
        std::vector<SP<Slot> > slots;
        /* And a mutex to them */
        boost::recursive_mutex slots_mutex;

        /* Current slot profiles */
        std::vector<SP<SlotProfile> > slotprofiles;
        boost::recursive_mutex slotprofiles_mutex;

        bool launchSlotNoCheck(SP<SlotProfile> slot_profile, SP<User> launcher);

        void pruneInactiveSlots();
        void pruneInactiveClients();

        bool close;

        trankesbel::SocketEvents socketevents;

        void new_connection(SP<trankesbel::Socket> listening_socket);
        void client_signal_function(WP<Client> client, SP<trankesbel::Socket> from_where);

        /* this mutex locked in loop(), to ensure no outside code is called from this class
           in different threads. */
        boost::recursive_mutex cycle_mutex;

    public:
        /* Creates a new state. There can be only one, so trying to create another of this class in the same process is going to return null. */
        static SP<State> createState();
        ~State();

        /* Disconnects a user with the given nickname. Unless it corresponds to the client 'exclude' */
        void destroyClient(const ID &id, SP<Client> exclude = SP<Client>());

        /* Notify client or user to do I/O */
        void notifyClient(SP<Client> client);
        void notifyClient(SP<User> user);
        void notifyClient(SP<trankesbel::Socket> socket);
        void notifyAllClients();

        /* Called by slots to inform the state that slot has new data on it. */
        void signalSlotData(SP<Slot> who);

        /* Returns all the slots that are running. */
        std::vector<WP<Slot> > getSlots();
        /* Returns currently available slot profiles. */
        std::vector<WP<SlotProfile> > getSlotProfiles();

        /* Launches a slot. Returns bool if that can't be done (and logs to admin_logger) */
        /* If called with a slot profile, that slot profile must be in the list of slot profiles
         * this state knows about. This method checks for allowed launchers and returns false if it's not allowed for this user. */
        bool launchSlot(SP<SlotProfile> slot_profile, SP<User> launcher);
        bool launchSlot(const ID &id, SP<User> launcher);

        /* Makes given user watch a slot with the given name. */
        bool setUserToSlot(SP<User> user, const ID& id);

        /* Checks if given user is allowed to watch given slot. */
        bool isAllowedWatcher(SP<User> user, SP<Slot> slot);
        /* Checks if given user is allowed to launch given slot profile. */
        bool isAllowedLauncher(SP<User> user, SP<SlotProfile> slot_profile);
        /* Checks if given user is allowed to play in a given slot */
        bool isAllowedPlayer(SP<User> user, SP<Slot> slot);
        /* Checks if given user is allowed to force close a slot */
        bool isAllowedForceCloser(SP<User> closer, SP<Slot> slot);

        /* Returns if a slot by given name exists and returns true if it does.
         * Also returns true for empty string. */
        bool hasSlotProfile(const ID &id);

        /* Updates a slot profile. It checks for currently watching/playing users
         * and kicks them out according to if something was forbidden in edit. */
        void updateSlotProfile(SP<SlotProfile> target, const SlotProfile &source);

        /* Deletes a slot profile. */
        void deleteSlotProfile(SP<SlotProfile> slotprofile);

        /* Returns a slot of given name, if there is one. */
        WP<Slot> getSlot(const ID &id);

        /* Returns a slot profile of given name, if there is one. */
        WP<SlotProfile> getSlotProfile(const ID &id);
        
        /* Adds a new slot profile to state */
        bool addSlotProfile(SP<SlotProfile> sp);

        /* Sets ticks per second. */
        void setTicksPerSecond(uint64_t ticks_per_second);

        /* Sets configuration database to use. */
        bool setDatabase(UnicodeString database_file);
        bool setDatabaseUTF8(std::string database_file);

        /* Add a new telnet port to listen to. */
        bool addTelnetService(trankesbel::SocketAddress address);

        /* Sets and gets the maximum number of slots that can run at a time. */
        void setMaximumNumberOfSlots(trankesbel::ui32 max_slots);
        trankesbel::ui32 getMaximumNumberOfSlots() const;
        
        /* Set/Get motd */
        void setMOTD(UnicodeString motd);
        UnicodeString getMOTD();
        
        /* Gets the user that corresponds to the given id */
        SP<User> getUser(const ID& id);

        /* Gets all users or clients */
        void getAllUsers(std::vector<SP<User> >* users);
        LockedObject<std::vector<SP<Client> > > getAllClients();
        
        /* Force closes slot user is currently watching. 
           Returns true if that succeeded and false if it did not. */
        bool forceCloseSlotOfUser(SP<User> user);

        /* Runs until admin tells it to stop. */
        void loop();
};


}; /* namespace */

#endif

