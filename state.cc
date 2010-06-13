#include "state.hpp"
#include <iostream>
#include <sstream>
#include "nanoclock.hpp"

using namespace dfterm;
using namespace std;

static bool state_initialized = false;

static ui64 running_counter = 1;

State::State()
{
    state_initialized = true;
    global_chat = SP<Logger>(new Logger);
    ticks_per_second = 20;
};

State::~State()
{
    state_initialized = false;
};
    
WP<Slot> State::getSlot(UnicodeString name)
{
    vector<SP<Slot> >::iterator i1;
    for (i1 = slots.begin(); i1 != slots.end(); i1++)
        if ((*i1) && (*i1)->getName() == name)
            return (*i1);
    return WP<Slot>();
}

vector<WP<Slot> > State::getSlots()
{
    vector<WP<Slot> > wp_slots;
    vector<SP<Slot> >::iterator i1;
    for (i1 = slots.begin(); i1 != slots.end(); i1++)
        wp_slots.push_back(*i1);
    return wp_slots;
}

vector<WP<SlotProfile> > State::getSlotProfiles()
{
    vector<WP<SlotProfile> > wp_slots;
    vector<SP<SlotProfile> >::iterator i1;
    for (i1 = slotprofiles.begin(); i1 != slotprofiles.end(); i1++)
        wp_slots.push_back(*i1);
    return wp_slots;
}

SP<State> State::createState()
{
    if (state_initialized) return SP<State>();
    SP<State> newstate(new State);
    newstate->self = newstate;

    return newstate;
};

bool State::setDatabase(UnicodeString database_file)
{
    string r;
    database_file.toUTF8String(r);
    return setDatabaseUTF8(r);
}

bool State::setDatabaseUTF8(string database_file)
{
    /* Configuration */
    configuration = SP<ConfigurationDatabase>(new ConfigurationDatabase);
    OpenStatus d_result = configuration->openUTF8(database_file);
    if (d_result == Failure)
    {
        stringstream ss;
        ss << "Failed to open database file " << database_file;
        admin_logger->logMessageUTF8(ss);
        return false;
    }
    if (d_result == OkCreatedNewDatabase)
    {
        admin_logger->logMessageUTF8("Created a new database from scratch. You should add an admin account to configure dfterm2.");
        admin_logger->logMessageUTF8("You need to use the command line tool dfterm2_configure for that. Close dfterm2 and then");
        admin_logger->logMessageUTF8("add an account like this: ");
        admin_logger->logMessageUTF8("dfterm2_configure --adduser (user name) (password) admin");
        admin_logger->logMessageUTF8("For example:");
        admin_logger->logMessageUTF8("dfterm2_configure --adduser Adeon s3cr3t_p4ssw0rd admin");
        admin_logger->logMessageUTF8("This will create a new admin account for you.");
        admin_logger->logMessageUTF8("If you are not using the default database (if you don't know then you are using it), use");
        admin_logger->logMessageUTF8("the --database switch to modify the correct database.");
        return false;
    }

    return true;
}

bool State::addTelnetService(SocketAddress address)
{
    stringstream ss;
    SP<Socket> s(new Socket);
    bool result = s->listen(address);
    if (!result)
    {
        ss << "Listening failed. " << s->getError();
        admin_logger->logMessageUTF8(ss);
        return false;
    }
    admin_logger->logMessageUTF8("Telnet service started. ");
    listening_sockets.insert(s);

    return true;
}

void State::setTicksPerSecond(uint64_t ticks_per_second)
{
    this->ticks_per_second = ticks_per_second;
}

void State::addSlotProfile(SP<SlotProfile> sp)
{
    slotprofiles.push_back(sp);
};

bool State::hasSlotProfile(UnicodeString name)
{
    if (name.countChar32() == 0) return true;

    vector<SP<SlotProfile> >::iterator i1;
    for (i1 = slotprofiles.begin(); i1 != slotprofiles.end(); i1++)
        if ((*i1)->getName() == name) return true;
    return false;
}

bool State::isAllowedLauncher(SP<User> launcher, SP<SlotProfile> slot_profile)
{
    UserGroup allowed_launchers = slot_profile->getAllowedLaunchers();
    UserGroup forbidden_launchers = slot_profile->getForbiddenLaunchers();
    
    if (forbidden_launchers.hasUser(launcher->getName()))
        return false;
    if (!allowed_launchers.hasUser(launcher->getName()) && !allowed_launchers.hasLauncher())
        return false;
    return true;
}

bool State::isAllowedPlayer(SP<User> user, SP<Slot> slot)
{
    bool not_allowed_by_being_launcher = false;
    SP<SlotProfile> sp_slotprofile = slot->getSlotProfile().lock();
    if (!sp_slotprofile)
    {
        stringstream ss;
        ss << "State::isAllowedPlayer(), not slot profile associated with slot " << slot->getNameUTF8();
        admin_logger->logMessageUTF8(ss.str());
        return false;
    }

    UserGroup allowed_players = sp_slotprofile->getAllowedPlayers();
    UserGroup forbidden_players = sp_slotprofile->getForbiddenPlayers();
    SP<User> launcher = slot->getLauncher().lock();
    if (launcher && launcher == user)
    {
        if (forbidden_players.hasLauncher())
            return false;
        if (!allowed_players.hasLauncher())
            not_allowed_by_being_launcher = true;
    }
    if (forbidden_players.hasUser(user->getName()))
        return false;
    if (!allowed_players.hasUser(user->getName()) && (not_allowed_by_being_launcher || launcher != user))
        return false;
    return true;
};

bool State::isAllowedWatcher(SP<User> user, SP<Slot> slot)
{
    bool not_allowed_by_being_launcher = false;
    SP<SlotProfile> sp_slotprofile = slot->getSlotProfile().lock();
    if (!sp_slotprofile)
    {
        stringstream ss;
        ss << "State::isAllowedWatcher(), not slot profile associated with slot " << slot->getNameUTF8();
        admin_logger->logMessageUTF8(ss.str());
        return false;
    }

    UserGroup allowed_watchers = sp_slotprofile->getAllowedWatchers();
    UserGroup forbidden_watchers = sp_slotprofile->getForbiddenWatchers();
    SP<User> launcher = slot->getLauncher().lock();
    if (launcher && launcher == user)
    {
        if (forbidden_watchers.hasLauncher())
            return false;
        if (!allowed_watchers.hasLauncher())
            not_allowed_by_being_launcher = true;
    }
    if (forbidden_watchers.hasUser(user->getName()))
        return false;
    if (!allowed_watchers.hasUser(user->getName()) && (not_allowed_by_being_launcher || launcher != user))
        return false;
    return true;
};

bool State::setUserToSlot(SP<User> user, UnicodeString slot_name)
{
    /* Find the user from client list */
    SP<Client> client;
    vector<SP<Client> >::iterator i1;
    for (i1 = clients.begin(); i1 != clients.end(); i1++)
        if ( (*i1)->getUser() == user)
        {
            client = (*i1);
            break;
        }

    string slot_name_utf8;
    slot_name.toUTF8String(slot_name_utf8);

    if (!client)
    {
        stringstream ss;
        ss << "User " << user->getNameUTF8() << " attempted to watch a slot " << slot_name_utf8 << " but there is no associated client connected.";
        admin_logger->logMessageUTF8(ss.str());
        return false;
    }

    client->setSlot(SP<Slot>());

    WP<Slot> slot = getSlot(slot_name);
    SP<Slot> sp_slot = slot.lock();
    if (!sp_slot)
    {
        stringstream ss;
        ss << "Slot join requested by " << user->getNameUTF8() << " from interface but no such slot is in state. " << slot_name_utf8;
        admin_logger->logMessageUTF8(ss.str());
        return false;
    }

    SP<SlotProfile> sp_slotprofile = sp_slot->getSlotProfile().lock();
    if (!sp_slotprofile)
    {
        stringstream ss;
        ss << "Slot join requested by " << user->getNameUTF8() << " from interface but the slot has no slot profile associated with it. " << slot_name_utf8;
        admin_logger->logMessageUTF8(ss.str());
        return false;
    }

    /* Check if this client is allowed to watch this. */
    if (!isAllowedWatcher(user, sp_slot))
    {
        stringstream ss;
        ss << "Slot join requested by " << user->getNameUTF8() << " but they are not allowed to do that. " << slot_name_utf8;
        admin_logger->logMessageUTF8(ss.str());
        return false;
    }

    client->setSlot(sp_slot);
    stringstream ss;
    ss << "User " << user->getNameUTF8() << " is now watching slot " << slot_name_utf8;
    admin_logger->logMessageUTF8(ss.str());
    return true;
}

bool State::launchSlotNoCheck(SP<SlotProfile> slot_profile, SP<User> launcher)
{
    if (!launcher) launcher = SP<User>(new User);

    if (!isAllowedLauncher(launcher, slot_profile))
    {
        stringstream ss;
        ss << "User " << launcher->getNameUTF8() << " attempted to launch a slot but they are not allowed to do that. " << slot_profile->getNameUTF8();
        admin_logger->logMessageUTF8(ss.str());
        return false;
    }

    stringstream rcs;
    rcs << running_counter;
    running_counter++;

    SP<Slot> slot = Slot::createSlot(slot_profile->getSlotType());
    slot->setSlotProfile(slot_profile);
    slot->setLauncher(launcher);
    slot->setNameUTF8(slot_profile->getNameUTF8() + string(" - ") + launcher->getNameUTF8() + string(":") + rcs.str());
    if (!slot)
    {
        admin_logger->logMessageUTF8(string("Slot::createSlot() failed with slot profile ") + slot_profile->getNameUTF8());
        return false;
    }
    slot->setParameter("path", slot_profile->getExecutable());
    slot->setParameter("work", slot_profile->getWorkingPath());

    stringstream ss_w, ss_h;
    ss_w << slot_profile->getWidth();
    ss_h << slot_profile->getHeight();

    slot->setParameter("w", UnicodeString::fromUTF8(ss_w.str()));
    slot->setParameter("h", UnicodeString::fromUTF8(ss_h.str()));

    admin_logger->logMessageUTF8(string("Launched a slot from slot profile ") + slot_profile->getNameUTF8());

    slots.push_back(slot);
    return true;
}

bool State::launchSlot(SP<SlotProfile> slot_profile, SP<User> launcher)
{
    if (!slot_profile)
    {
        admin_logger->logMessageUTF8("Attempted to launch a null slot profile.");
        return false;
    }

    vector<SP<SlotProfile> >::iterator i1;
    for (i1 = slotprofiles.begin(); i1 != slotprofiles.end(); i1++)
        if ((*i1) == slot_profile)
            return launchSlotNoCheck(*i1, launcher);
    admin_logger->logMessageUTF8("Attempted to launch a slot profile that does not exist in slot profile list.");
    return false;
}

bool State::launchSlot(UnicodeString slot_profile_name, SP<User> launcher)
{
    vector<SP<SlotProfile> >::iterator i1;
    for (i1 = slotprofiles.begin(); i1 != slotprofiles.end(); i1++)
        if ((*i1) && (*i1)->getName() == slot_profile_name)
            return launchSlotNoCheck(*i1, launcher);
    string utf8_name;
    slot_profile_name.toUTF8String(utf8_name);
    admin_logger->logMessageUTF8(string("Attempted to launch a slot with name ") + utf8_name + string(" that does not exist in slot profile list."));
    return false;
}

void State::loop()
{
    /* Use these for timing ticks */
    uint64_t start_time;
    const uint64_t tick_time = 1000000000 / ticks_per_second;

    bool close = false;

    while(listening_sockets.size() > 0 && !close)
    {
        start_time = nanoclock();

        bool update_nicklists = false;
        /* Prune inactive clients */
        unsigned int i2, len = clients.size();
        for (i2 = 0; i2 < len; i2++)
        {
            if (!clients[i2]->isActive())
            {
                clients.erase(clients.begin() + i2);
                clients_weak.erase(clients_weak.begin() + i2);
                len--;
                i2--;
                admin_logger->logMessageUTF8("Pruned an inactive connection.");
                update_nicklists = true;
                continue;
            }
        }

        /* Check for incoming connections. */
        SP<Socket> new_connection(new Socket);
        set<SP<Socket> >::iterator listener;
        for (listener = listening_sockets.begin(); listener != listening_sockets.end(); listener++)
        {
            bool got_connection = (*listener)->accept(new_connection.get());
            if (got_connection)
            {
                SP<Client> new_client = Client::createClient(new_connection);
                new_client->setState(self);
                new_client->setConfigurationDatabase(configuration);
                new_client->setGlobalChatLogger(global_chat);
                clients.push_back(new_client);
                clients_weak.push_back(new_client);
                new_client->setClientVector(&clients_weak);
                admin_logger->logMessageUTF8("Got new connection.");
                update_nicklists = true;
            }
        }

        /* Read and write from and to connections */
        len = clients.size();
        for (i2 = 0; i2 < len; i2++)
        {
            if (update_nicklists) clients[i2]->updateClients();
            clients[i2]->cycle();
            if (clients[i2]->shouldShutdown()) close = true;
        }

        /* Ticky wait. */
        uint64_t end_time = nanoclock();
        if (end_time - start_time < tick_time)
            nanowait( tick_time - (end_time - start_time) );
        flush_messages();
    }
    flush_messages();
}

