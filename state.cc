#include "state.hpp"
#include <iostream>
#include <sstream>
#include "nanoclock.hpp"
#include "logger.hpp"

#include "dfterm2_limits.hpp"

using namespace dfterm;
using namespace std;
using namespace boost;
using namespace trankesbel;

static bool state_initialized = false;

static ui64 running_counter = 1;

State::State()
{
    maximum_slots = 0xffffffff;
    state_initialized = true;
    global_chat = SP<Logger>(new Logger);
    close = false;
    
    default_address_allowance = true;
    
    stringstream ss;
    ss << "Welcome. This is a dfterm2 server. Take off your shoes and wipe your nose. "
    "Do good and no evil. ";
    MOTD = TO_UNICODESTRING(ss.str());
};

State::~State()
{
    state_initialized = false;
};

void State::saveUser(SP<User> user)
{
    assert(configuration);
    configuration->saveUserData(user);
}

void State::saveUser(const ID& user_id)
{
    assert(configuration);

    SP<User> user = getUser(user_id);
    if (!user)
    { LOG(Note, "saveUser() called with user ID but there is no such user. (" << user_id.serialize() << ")."); }

    configuration->saveUserData(user);
}

void State::getAllUsers(vector<SP<User> >* users)
{
    assert(users);
    assert(configuration);

    vector<SP<User > > &u = (*users);
    u.clear();

    u = configuration->loadAllUserData();
}

LockedObject<vector<SP<Client> > > State::getAllClients()
{
    return clients.lock();
}

SP<Client> State::getClient(const ID& id)
{
    LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
    vector<SP<Client> > &cli = *lo_clients.get();

    vector<SP<Client> >::iterator i1, cli_end = cli.end();
    for (i1 = cli.begin(); i1 != cli_end; ++i1)
        if ( (*i1) && (*i1)->getIDRef() == id)
            return (*i1);
    lo_clients.release();
    
    return SP<Client>();
}

SP<User> State::getUser(const ID& id)
{
    LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
    vector<SP<Client> > &cli = *lo_clients.get();

    vector<SP<Client> >::iterator i1, cli_end = cli.end();
    for (i1 = cli.begin(); i1 != cli_end; ++i1)
        if ( (*i1) && (*i1)->getUser()->getID() == id)
            return (*i1)->getUser();
    lo_clients.release();

    if (configuration)
        return configuration->loadUserData(id);
    
    return SP<User>();
}

void State::setMaximumNumberOfSlots(trankesbel::ui32 max_slots)
{
    assert(configuration);

    maximum_slots = max_slots;
    if (maximum_slots >= MAX_SLOTS)
        maximum_slots = MAX_SLOTS;
}

trankesbel::ui32 State::getMaximumNumberOfSlots() const
{
    return maximum_slots;
}

bool State::forceCloseSlotOfUser(SP<User> user)
{
    assert(user);
    
    /* Find the slot and the corresponding slot profile this user is watching */
    
    SP<Slot> slot;
    LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
    vector<SP<Client> > &cli = *lo_clients.get();
    vector<SP<Client> >::iterator i1, cli_end = cli.end();
    for (i1 = cli.begin(); i1 != cli_end; ++i1)
        if ( (*i1) && (*i1)->getUser()->getID() == user->getID() )
        {
            slot = (*i1)->getSlot().lock();
            notifyClient(*i1);
            break;
        }
    lo_clients.release();

    if (!slot) return false;
    
    SP<SlotProfile> sp = slot->getSlotProfile().lock();
    if (!sp) return false;
    
    /* Check that user is allowed to force-close this slot */
    if (!isAllowedForceCloser(user, slot)) return false;

    /* Find the slot from slot list */
    bool found_slot = false;
    vector<SP<Slot> >::iterator i2, slots_end = slots.end();
    for (i2 = slots.begin(); i2 != slots_end; ++i2)
        if ((*i2) == slot)
        {
            slots.erase(i2);
            found_slot = true;
            break;
        }

    if (!found_slot) return false;

    char time_c[51];
    time_c[50] = 0;
    time_t timet = time(0);
    #ifdef _WIN32
    struct tm* timem = localtime(&timet);
    strftime(time_c, 50, "%H:%M:%S ", timem);
    #else
    struct tm timem;
    localtime_r(&timet, &timem);
    strftime(time_c, 50, "%H:%M:%S ", &timem);
    #endif
    stringstream ss;
    ss << time_c << " " << user->getNameUTF8() << " has force closed slot " << slot->getNameUTF8();
    global_chat->logMessageUTF8(ss.str());
    
    return true;
}

void State::setMOTD(UnicodeString motd)
{
    assert(configuration);
    this->MOTD = motd;

    configuration->saveMOTD(motd);
}

UnicodeString State::getMOTD()
{
    return MOTD;
}

WP<SlotProfile> State::getSlotProfile(const ID &id)
{
    lock_guard<recursive_mutex> lock(slotprofiles_mutex);

    vector<SP<SlotProfile> >::iterator i1, slotprofiles_end = slotprofiles.end();
    for (i1 = slotprofiles.begin(); i1 != slotprofiles_end; ++i1)
        if ((*i1) && (*i1)->getIDRef() == id)
            return (*i1);
    return WP<SlotProfile>();
}
    
WP<Slot> State::getSlot(const ID &id)
{
    lock_guard<recursive_mutex> lock(slots_mutex);

    vector<SP<Slot> >::iterator i1, slots_end = slots.end();
    for (i1 = slots.begin(); i1 != slots_end; ++i1)
    {
        if ((*i1))
        {
            if ((*i1)->getIDRef() == id)
                return (*i1);
        }
    }
     
    return WP<Slot>();
}

vector<WP<Slot> > State::getSlots()
{
    lock_guard<recursive_mutex> lock(slots_mutex);

    vector<WP<Slot> > wp_slots;
    vector<SP<Slot> >::iterator i1, slots_end = slots.end();
    for (i1 = slots.begin(); i1 != slots_end; ++i1)
        wp_slots.push_back(*i1);
    return wp_slots;
}

vector<WP<SlotProfile> > State::getSlotProfiles()
{
    lock_guard<recursive_mutex> lock(slotprofiles_mutex);

    vector<WP<SlotProfile> > wp_slots;
    vector<SP<SlotProfile> >::iterator i1, slotprofiles_end = slotprofiles.end();
    for (i1 = slotprofiles.begin(); i1 != slotprofiles_end; ++i1)
        wp_slots.push_back(*i1);
    return wp_slots;
}

SP<State> State::createState()
{
    assert(!state_initialized);

    SP<State> newstate(new State);
    newstate->self = newstate;

    return newstate;
};

bool State::setDatabase(UnicodeString database_file)
{
    return setDatabaseUTF8(TO_UTF8(database_file));
}

bool State::setDatabaseUTF8(string database_file)
{
    unique_lock<recursive_mutex> clock(configuration_mutex);

    /* Configuration */
    configuration = SP<ConfigurationDatabase>(new ConfigurationDatabase);
    OpenStatus d_result = configuration->openUTF8(database_file);
    if (d_result == Failure)
    {
        LOG(Error, "Failed to open database file " << database_file);
        return false;
    }
    if (d_result == OkCreatedNewDatabase)
    {
        LOG(Note, "Created a new database from scratch. You should add an admin account to configure dfterm2.");
        LOG(Note, "You need to use the command line tool dfterm2_configure for that. Close dfterm2 and then");
        LOG(Note, "add an account like this: ");
        LOG(Note, "dfterm2_configure --adduser (user name) (password) admin");
        LOG(Note, "For example:");
        LOG(Note, "dfterm2_configure --adduser Adeon s3cr3t_p4ssw0rd admin");
        LOG(Note, "This will create a new admin account for you.");
        LOG(Note, "If you are not using the default database (if you don't know then you are using it), use");
        LOG(Note, "the --database switch to modify the correct database.");
        return true;
    }
    clock.unlock();

    unique_lock<recursive_mutex> lock(slots_mutex);
    slots.clear();
    lock.unlock();

    maximum_slots = configuration->loadMaximumNumberOfSlots();
    LOG(Note, "Maximum number of slots is " << maximum_slots);
    if (maximum_slots >= MAX_SLOTS)
        maximum_slots = MAX_SLOTS;

    lock_guard<recursive_mutex> lock2(slotprofiles_mutex);
    slotprofiles.clear();

    vector<UnicodeString> profile_list = configuration->loadSlotProfileNames();
    vector<UnicodeString>::iterator i1, profile_list_end = profile_list.end();
    for (i1 = profile_list.begin(); i1 != profile_list_end; ++i1)
    {
        SP<SlotProfile> sp = configuration->loadSlotProfileData(*i1);
        if (!sp) continue;

        addSlotProfile(sp);
    }
    
    MOTD = configuration->loadMOTD();

    return true;
}

bool State::addHTTPService(SocketAddress address, SocketAddress flashpolicy_address, const string &httpconnectaddress)
{
    if (listening_sockets.begin() == listening_sockets.end())
    {
        LOG(Error, "Cannot start an HTTP service without a Telnet service.");
        return false;
    }
    SP<Socket> ls = *listening_sockets.begin();
    if (!ls || !ls->active())
    {
        LOG(Error, "Cannot start an HTTP service without a Telnet service.");
        return false;
    }

    ui16 telnet_port = ls->getAddress().getPort();

    SP<Socket> s(new Socket);
    bool result = s->listen(address);
    if (!result)
    {
        LOG(Error, "Listening as HTTP service at address " << address.getHumanReadablePlainUTF8() << " failed. " << s->getError());
        return false;
    }
    LOG(Note, "HTTP service started on address " << address.getHumanReadablePlainUTF8());

    /* We need the Telnet service to make HTTP service work */

    /* Add flash policy file */
    stringstream flashpolicy;
    flashpolicy << "<?xml version=\"1.0\"?>" << endl;
    flashpolicy << "<!DOCTYPE cross-domain-policy SYSTEM \"/xml/dtds/cross-domain-policy.dtd\">" << endl;
    flashpolicy << "<cross-domain-policy>" << endl;
    flashpolicy << "<site-control permitted-cross-domain-policies=\"master-only\"/>" << endl;
    flashpolicy << "<allow-access-from domain=\"*\" to-ports=\"" << telnet_port << "\" />" << endl;
    flashpolicy << "</cross-domain-policy>" << endl;
    http_server.serveContentUTF8(flashpolicy.str(), "text/x-cross-domain-policy", "/crossdomain.xml");

    stringstream ss;
    ss << telnet_port;

    map<string, string> replacors;
    replacors["##LOCALADDRESS##"] = httpconnectaddress;
    replacors["##LOCALPORT##"] = ss.str();
    stringstream ss2;
    ss2 << flashpolicy_address.getPort();
    replacors["##FLASHPOLICYPORT##"] = ss2.str();

    map<string, string> empty;

    http_server.serveFileUTF8("soiled/soiled.swf", "application/x-shockwave-flash", "/soiled.swf", empty);
    http_server.serveFileUTF8("soiled/soiled.html", "text/html", "/", replacors);
    http_server.serveFileUTF8("soiled/soiled.txt", "text/plain; charset=UTF-8", "/soiled.txt", empty);
    http_server.serveFileUTF8("soiled/beep.mp3", "audio/mp3", "/beep.mp3", empty);
    http_server.serveFileUTF8("soiled/AC_OETags.js", "application/javascript", "/AC_OETags.js", empty);

    http_server.addListeningSocket(s);

    socketevents.addSocket(s);

    SP<Socket> s2(new Socket);
    result = s2->listen(flashpolicy_address);
    if (!result)
    {
        LOG(Error, "Listening as flashpolicy.xml service at address " << flashpolicy_address.getHumanReadablePlainUTF8() << " failed. " << s->getError());
        return true; // intentionally returns non-error value
    }
    LOG(Note, "flashpolicy.xml service started on address " << flashpolicy_address.getHumanReadablePlainUTF8());

    http_server.addPlainListeningSocket(s2, "/crossdomain.xml");
    socketevents.addSocket(s2);

    return true;
}

bool State::addTelnetService(SocketAddress address)
{
    SP<Socket> s(new Socket);
    bool result = s->listen(address);
    if (!result)
    {
        LOG(Error, "Listening as telnet service at address " << address.getHumanReadablePlainUTF8() << " failed. " << s->getError());
        return false;
    }
    LOG(Note, "Telnet service started on address " << address.getHumanReadablePlainUTF8());
    listening_sockets.insert(s);

    socketevents.addSocket(s);

    return true;
}

void State::destroyClientAndUser(const ID& id, SP<Client> exclude)
{
    SP<User> u = getUser(id);
    if (!u)
    {
        SP<Client> c = getClient(id);
        if (c == exclude) return;

        u = c->getUser();
    }

    destroyClient(id, exclude);
    if (configuration && u && u->getNameUTF8().size() > 0)
        configuration->deleteUserData(u->getName());
}

void State::destroyClient(const ID &user_id, SP<Client> exclude)
{
    bool update_nicklists = false;

    LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
    vector<SP<Client> > &cli = *lo_clients.get();

    LockedObject<vector<WP<Client> > > lo_weak_clients = clients_weak.lock();
    vector<WP<Client> > &weak_cli = *lo_weak_clients.get();

    size_t i1, len = cli.size();
    for (i1 = 0; i1 < len; ++i1)
    {
        if (cli[i1] == exclude) continue;

        if (cli[i1] && (cli[i1]->getUser()->getIDRef() == user_id || cli[i1]->getIDRef() == user_id))
        {
            cli.erase(cli.begin() + i1);
            weak_cli.erase(weak_cli.begin() + i1);
            LOG(Note, "Disconnected connection for user " << cli[i1]->getUser()->getNameUTF8());
            update_nicklists = true;
            break;
        }
    }

    if (!update_nicklists) return;

    len = cli.size();
    for (i1 = 0; i1 < len; ++i1)
        if (cli[i1])
            cli[i1]->updateClientNicklist(&cli);
}

bool State::addSlotProfile(SP<SlotProfile> sp)
{
    assert(sp);

    if (slotprofiles.size() >= MAX_SLOT_PROFILES)
    {
        LOG(Error, "Attempted to create a slot profile but compile-time maximum number of slot profiles has been reached.");
        return false;
    }
    slotprofiles.push_back(sp);
    return true;
};

void State::deleteSlotProfile(SP<SlotProfile> slotprofile)
{
    assert(slotprofile);

    size_t i1, len = slots.size();
    for (i1 = 0; i1 < len; ++i1)
    {
        if (!slots[i1]) continue;

        SP<SlotProfile> sp = slots[i1]->getSlotProfile().lock();
        if (!sp || slotprofile != sp) continue;

        slots.erase(slots.begin() + i1);
        --len;
        --i1;
    };

    len = slotprofiles.size();
    for (i1 = 0; i1 < len; ++i1)
    {
        if (slotprofiles[i1] == slotprofile)
        {
            slotprofiles.erase(slotprofiles.begin() + i1);
            --i1;
            --len;
        }
    }
}

void State::updateSlotProfile(SP<SlotProfile> target, const SlotProfile &source)
{
    assert(target);

    (*target.get()) = source;

    size_t i1, len = slots.size();
    for (i1 = 0; i1 < len; ++i1)
    {
        if (!slots[i1]) continue;

        SP<SlotProfile> slotprofile = slots[i1]->getSlotProfile().lock();
        if (!slotprofile) continue;

        LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
        vector<SP<Client> > &cli = *lo_clients.get();

        size_t i2, len2 = cli.size();
        for (i2 = 0; i2 < len2; ++i2)
        {
            if (!cli[i2]) continue;

            if (cli[i2]->getSlot().lock() != slots[i1])
                continue;

            SP<User> user = cli[i2]->getUser();
            if (!user) continue;

            if (!isAllowedWatcher(user, slots[i1]))
                cli[i2]->setSlot(SP<Slot>());
        }
    }
}

bool State::hasSlotProfile(const ID& id)
{
    vector<SP<SlotProfile> >::iterator i1, slotprofiles_end = slotprofiles.end();
    for (i1 = slotprofiles.begin(); i1 != slotprofiles_end; ++i1)
        if ((*i1)->getIDRef() == id) return true;
    return false;
}

bool State::isAllowedForceCloser(SP<User> closer, SP<Slot> slot)
{
    bool not_allowed_by_being_launcher = false;
    SP<SlotProfile> sp_slotprofile = slot->getSlotProfile().lock();
    if (!sp_slotprofile)
    {
        LOG(Error, "State::isAllowedForceCloser(), no slot profile associated with slot " << slot->getNameUTF8());
        return false;
    }
    
    UserGroup allowed_closers = sp_slotprofile->getAllowedClosers();
    UserGroup forbidden_closers = sp_slotprofile->getForbiddenClosers();
    
    SP<User> launcher = slot->getLauncher().lock();
    if (launcher && launcher == closer)
    {
        if (forbidden_closers.hasLauncher())
            return false;
        if (!allowed_closers.hasLauncher())
            not_allowed_by_being_launcher = true;
    }
    if (forbidden_closers.hasUser(closer->getIDRef()))
        return false;
    if (!allowed_closers.hasUser(closer->getIDRef()) && (not_allowed_by_being_launcher || launcher != closer))
        return false;
    return true;
}

bool State::isAllowedLauncher(SP<User> launcher, SP<SlotProfile> slot_profile)
{
    UserGroup allowed_launchers = slot_profile->getAllowedLaunchers();
    UserGroup forbidden_launchers = slot_profile->getForbiddenLaunchers();
    
    if (forbidden_launchers.hasUser(launcher->getIDRef()))
        return false;
    if (!allowed_launchers.hasUser(launcher->getIDRef()) && !allowed_launchers.hasLauncher())
        return false;
    return true;
}

bool State::isAllowedPlayer(SP<User> user, SP<Slot> slot)
{
    bool not_allowed_by_being_launcher = false;
    SP<SlotProfile> sp_slotprofile = slot->getSlotProfile().lock();
    if (!sp_slotprofile)
    {
        LOG(Error, "State::isAllowedPlayer(), no slot profile associated with slot " << slot->getNameUTF8());
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
    if (forbidden_players.hasUser(user->getIDRef()))
        return false;
    if (!allowed_players.hasUser(user->getIDRef()) && (not_allowed_by_being_launcher || launcher != user))
        return false;
    return true;
};

bool State::isAllowedWatcher(SP<User> user, SP<Slot> slot)
{
    bool not_allowed_by_being_launcher = false;
    SP<SlotProfile> sp_slotprofile = slot->getSlotProfile().lock();
    if (!sp_slotprofile)
    {
        LOG(Error, "State::isAllowedWatcher(), no slot profile associated with slot " << slot->getNameUTF8());
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
    if (forbidden_watchers.hasUser(user->getIDRef()))
        return false;
    if (!allowed_watchers.hasUser(user->getIDRef()) && (not_allowed_by_being_launcher || launcher != user))
        return false;
    return true;
};

bool State::setUserToSlot(SP<User> user, const ID &slot_id)
{
    assert(user);

    /* Find the user from client list */
    LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
    vector<SP<Client> > &cli = *lo_clients.get();

    SP<Client> client;
    vector<SP<Client> >::iterator i1, cli_end = cli.end();
    for (i1 = cli.begin(); i1 != cli_end; ++i1)
        if ( (*i1)->getUser() == user)
        {
            client = (*i1);
            break;
        }

    if (!client)
    {
        LOG(Error, "User " << user->getNameUTF8() << " attempted to watch slot with ID " << slot_id.serialize() << " but there is no associated client connected.");
        return false;
    }

    client->setSlot(SP<Slot>());

    WP<Slot> slot = getSlot(slot_id);
    SP<Slot> sp_slot = slot.lock();
    if (!sp_slot)
    {
        LOG(Error, "Slot join requested by " << user->getNameUTF8() << " from interface but no such slot is in state. Slot ID " << slot_id.serialize());
        return false;
    }

    SP<SlotProfile> sp_slotprofile = sp_slot->getSlotProfile().lock();
    if (!sp_slotprofile)
    {
        LOG(Error, "Slot join requested by " << user->getNameUTF8() << " from interface but the slot has no slot profile associated with it. Slot name " << slot_id.serialize());
        return false;
    }

    /* Check if this client is allowed to watch this. */
    if (!isAllowedWatcher(user, sp_slot))
    {
        LOG(Error, "Slot join requested by " << user->getNameUTF8() << " but they are not allowed to do that. Slot ID " << slot_id.serialize());
        return false;
    }

    client->setSlot(sp_slot);
    LOG(Note, "User " << user->getNameUTF8() << " is now watching slot " << sp_slot->getNameUTF8());
    return true;
}

bool State::launchSlotNoCheck(SP<SlotProfile> slot_profile, SP<User> launcher)
{
    assert(slot_profile && launcher);

    LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
    vector<SP<Client> > &cli = *lo_clients.get();
    SP<Client> client;

    vector<SP<Client> >::iterator i2, cli_end = cli.end();
    for (i2 = cli.begin(); i2 != cli_end; ++i2)
        if ( (*i2) && (*i2)->getUser()->getIDRef() == launcher->getIDRef())
            client = (*i2);

    lo_clients.release();

    lock_guard<recursive_mutex> lock(slotprofiles_mutex);
    lock_guard<recursive_mutex> lock2(slots_mutex);

    /* Check for slot limits */
    if (slots.size() >= MAX_SLOTS) /* hard compile-time limit */
    {
        LOG(Error, "User " << launcher->getNameUTF8() << " attempted to launch a slot from slot profile " << slot_profile->getNameUTF8() << " but maximum number of compile-time slots has been reached.");
        if (client) client->sendPrivateChatMessageUTF8("Maximum number of slots has been reached.");
        if (client) notifyClient(client);
        return false;
    }
    else if (slots.size() >= maximum_slots) /* soft configurable limit */
    {
        LOG(Error, "User " << launcher->getNameUTF8() << " attempted to launch a slot from slot profile " << slot_profile->getNameUTF8() << " but maximum number of slots has been reached.");
        if (client) client->sendPrivateChatMessageUTF8("Maximum number of slots has been reached.");
        if (client) notifyClient(client);
        return false;
    }

    if (!isAllowedLauncher(launcher, slot_profile))
    {
        LOG(Error, "User " << launcher->getNameUTF8() << " attempted to launch a slot from slot profile " << slot_profile->getNameUTF8() << " but they are not allowed to do that.");
        if (client) client->sendPrivateChatMessageUTF8("You are not allowed to launch from this slot profile.");
        if (client) notifyClient(client);
        return false;
    }

    /* Check that there are not too many slots of this slot profile */
    ui32 num_slots = 0;
    vector<SP<Slot> >::iterator i1, slots_end = slots.end();
    for (i1 = slots.begin(); i1 != slots_end; ++i1)
        if ((*i1) && (*i1)->getSlotProfile().lock() == slot_profile)
            ++num_slots;
    if (num_slots >= slot_profile->getMaxSlots())
    {
        LOG(Error, "User " << launcher->getNameUTF8() << " attempted to launch a slot but maximum number of slots of this slot profile has been reached. Slot profile name " << slot_profile->getNameUTF8());
        if (client) client->sendPrivateChatMessageUTF8("Maximum number of slots of this slot profile have already been launched.");
        if (client) notifyClient(client);
        return false;
    }

    stringstream rcs;
    rcs << running_counter;
    ++running_counter;

    SP<Slot> slot = Slot::createSlot((SlotType) slot_profile->getSlotType());
    if (!slot)
    {
        LOG(Error, "Slot::createSlot() failed with slot profile " << slot_profile->getNameUTF8());
        if (client) client->sendPrivateChatMessageUTF8("Internal error while trying to launch a slot.");
        if (client) notifyClient(client);
        return false;
    }

    slot->setState(self);
    slot->setSlotProfile(slot_profile);
    slot->setLauncher(launcher);
    string name_utf8 = slot_profile->getNameUTF8() + string(" - ") + launcher->getNameUTF8() + string(":") + rcs.str();
    slot->setNameUTF8(name_utf8);
    slot->setParameter("path", slot_profile->getExecutable());
    slot->setParameter("work", slot_profile->getWorkingPath());

    stringstream ss_w, ss_h;
    ss_w << slot_profile->getWidth();
    ss_h << slot_profile->getHeight();

    slot->setParameter("w", UnicodeString::fromUTF8(ss_w.str()));
    slot->setParameter("h", UnicodeString::fromUTF8(ss_h.str()));

    LOG(Note, "Launched a slot from slot profile " << slot_profile->getNameUTF8());

    slots.push_back(slot);

    /* Put the user to watch the just launched slot */
    setUserToSlot(launcher, slot->getIDRef());
    
    char time_c[51];
    time_c[50] = 0;
    time_t timet = time(0);
    #ifdef _WIN32
    struct tm* timem = localtime(&timet);
    strftime(time_c, 50, "%H:%M:%S ", timem);
    #else
    struct tm timem;
    localtime_r(&timet, &timem);
    strftime(time_c, 50, "%H:%M:%S ", &timem);
    #endif
    stringstream ss;
    ss << time_c << " " << launcher->getNameUTF8() << " has launched slot " << slot->getNameUTF8();
    global_chat->logMessageUTF8(ss.str());

    return true;
}

bool State::launchSlot(SP<SlotProfile> slot_profile, SP<User> launcher)
{
    lock_guard<recursive_mutex> lock(slotprofiles_mutex);

    assert(slot_profile && launcher);

    vector<SP<SlotProfile> >::iterator i1, slotprofiles_end = slotprofiles.end();
    for (i1 = slotprofiles.begin(); i1 != slotprofiles_end; ++i1)
        if ((*i1) == slot_profile)
            return launchSlotNoCheck(*i1, launcher);
    LOG(Error, "User " << launcher->getNameUTF8() << " attempted to launch a slot profile that does not exist in slot profile list.");

    return false;
}

bool State::launchSlot(const ID &slot_id, SP<User> launcher)
{
    assert(launcher);

    vector<SP<SlotProfile> >::iterator i1, slotprofiles_end = slotprofiles.end();
    for (i1 = slotprofiles.begin(); i1 != slotprofiles_end; ++i1)
        if ((*i1) && (*i1)->getIDRef() == slot_id)
            return launchSlotNoCheck(*i1, launcher);
    string utf8_name = slot_id.serialize();
    if (launcher)
        { LOG(Error, "User " << launcher->getNameUTF8() << " attempted to launch a slot profile with ID " << utf8_name << " that does not exist in slot profile list."); }
    else
        { LOG(Error, "Null user attempted to launch a slot profile with ID " << utf8_name << " that does not exist in slot profile list."); }
    return false;
}

void State::notifyAllClients()
{
    LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
    vector<SP<Client> > &cli = *lo_clients.get();

    vector<SP<Client> >::iterator i1, cli_end = cli.end();
    for (i1 = cli.begin(); i1 != cli_end; ++i1)
        if (*i1 && (*i1)->getSocket())
            socketevents.forceEvent((*i1)->getSocket());
}

void State::notifyClient(SP<Client> client)
{
    assert(client);

    LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
    vector<SP<Client> > &cli = *lo_clients.get();

    vector<SP<Client> >::iterator i1, cli_end = cli.end();
    for (i1 = cli.begin(); i1 != cli_end; ++i1)
        if ((*i1) == client)
        {
            socketevents.forceEvent(client->getSocket());
            return;
        }

}

void State::notifyClient(SP<User> user)
{
    assert(user);

    LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
    vector<SP<Client> > &cli = *lo_clients.get();

    vector<SP<Client> >::iterator i1, cli_end = cli.end();
    for (i1 = cli.begin(); i1 != cli_end; ++i1)
        if ((*i1) && (*i1)->getUser()->getIDRef() == user->getIDRef())
        {
            socketevents.forceEvent((*i1)->getSocket());
            return;
        }
}

void State::notifyClient(SP<Socket> socket)
{
    assert(socket);
    socketevents.forceEvent(socket);
}

void State::delayedNotifyClient(SP<Client> c, ui64 nanoseconds)
{
    assert(c);

    LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
    vector<SP<Client> > &cli = *lo_clients.get();

    /* Check that this client is ours */
    vector<SP<Client> >::iterator i1, cli_end = cli.end();
    for (i1 = cli.begin(); i1 != cli_end; ++i1)
        if ((*i1) && (*i1) == c)
            break;

    if (i1 == cli_end) return;

    pending_delayed_notifications[nanoclock() + nanoseconds] = c;
}

void State::pruneInactiveSlots()
{
    lock_guard<recursive_mutex> lock(slots_mutex);

    /* Prune inactive slots */
    size_t i2, len = slots.size();
    for (i2 = 0; i2 < len; ++i2)
    {
        if (!slots[i2] || !slots[i2]->isAlive())
        {
            if (!slots[i2])
                { LOG(Note, "Removed a null slot from slot list."); }
            else
                { LOG(Note, "Removed slot " << slots[i2]->getNameUTF8() << " from slot list."); }

            char time_c[51];
            time_c[50] = 0;
            time_t timet = time(0);
            #ifdef _WIN32
            struct tm* timem = localtime(&timet);
            strftime(time_c, 50, "%H:%M:%S ", timem);
            #else
            struct tm timem;
            localtime_r(&timet, &timem);
            strftime(time_c, 50, "%H:%M:%S ", &timem);
            #endif
            stringstream ss;
            if (slots[i2])
            {
                ss << time_c << " Slot " << slots[i2]->getNameUTF8() << " has closed.";
                global_chat->logMessageUTF8(ss.str());
            }
                
            LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
            vector<SP<Client> > &cli = *lo_clients.get();
            vector<SP<Client> >::iterator i1, cli_end = cli.end();
            for (i1 = cli.begin(); i1 != cli_end; ++i1)
            {
                if (!(*i1)) continue;

                if ((*i1)->getSlot().lock() == slots[i2])
                {
                    (*i1)->setSlot(SP<Slot>());
                    notifyClient((*i1)->getSocket());
                }
            }
            lo_clients.release();

            slots.erase(slots.begin() + i2);
            --len;
            --i2;
            continue;
        }
    }
}

void State::pruneInactiveClients()
{
    LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
    vector<SP<Client> > &cli = *lo_clients.get();
    LockedObject<vector<WP<Client> > > lo_weak_clients = clients_weak.lock();
    vector<WP<Client> > &weak_cli = *lo_weak_clients.get();

    bool changes = false;

    /* Prune inactive clients */
    size_t i2, len = cli.size();
    for (i2 = 0; i2 < len; ++i2)
    {
        if (!cli[i2]->isActive())
        {
            char time_c[51];
            time_c[50] = 0;
            time_t timet = time(0);
            #ifdef __WIN32
            struct tm* timem = localtime(&timet);;
            strftime(time_c, 50, "%H:%M:%S ", timem);
            #else
            struct tm timem;
            localtime_r(&timet, &timem);
            strftime(time_c, 50, "%H:%M:%S ", &timem);
            #endif

            stringstream ss;
            if (cli[i2]->getUser()->getNameUTF8().size() > 0)
            {
                ss << time_c << " " << cli[i2]->getUser()->getNameUTF8() << " has disconnected from the server.";
                global_chat->logMessageUTF8(ss.str());
            }

            cli.erase(cli.begin() + i2);
            weak_cli.erase(weak_cli.begin() + i2);
            --len;
            --i2;
            LOG(Note, "Removed an inactive client from memory.");
            changes = true;
            continue;
        }
    }

    if (!changes) return;

    len = cli.size();
    for (i2 = 0; i2 < len; ++i2)
        cli[i2]->updateClientNicklist(&cli);
    notifyAllClients();
}

bool State::checkAddressAllowance(SP<Client> client)
{
    assert(client);

    SP<Socket> s = client->getSocket();
    if (!s) return false;
    if (!s->active()) return false;

    SocketAddress sa = s->getAddress();

    vector<SocketAddressRange>::const_iterator i1, forbidden_addresses_end = forbidden_addresses.end();
    for (i1 = forbidden_addresses.begin(); i1 != forbidden_addresses_end; ++i1)
        if (i1->testAddress(sa))
        {
            SP<User> u = client->getUser();
            if (!u || u->getNameUTF8().empty())
            { LOG(Note, "Disconnected connection from " << sa.getHumanReadablePlainUTF8() << " because it matched a forbidden address."); }
            else
            { LOG(Note, "Disconnected connection from " << sa.getHumanReadablePlainUTF8() << " because it matched a forbidden address. User was \"" << u->getNameUTF8() << "\"."); };

            s->close();
            return true;
        }
    
    if (default_address_allowance) 
        return false;
    
    vector<SocketAddressRange>::const_iterator allowed_addresses_end = allowed_addresses.end();
    for (i1 = allowed_addresses.begin(); i1 != allowed_addresses_end; ++i1)
        if (i1->testAddress(sa))
            return false;
    
    SP<User> u = client->getUser();
    if (!u || u->getNameUTF8().empty())
    { LOG(Note, "Disconnected connection from " << sa.getHumanReadablePlainUTF8() << " because it did not match an allowed address."); }
    else
    { LOG(Note, "Disconnected connection from " << sa.getHumanReadablePlainUTF8() << " because it did not match an allowed address. User was \"" << u->getNameUTF8() << "\"."); };

    s->close();
    return true;
}

void State::client_signal_function(WP<Client> client, SP<Socket> from_where)
{
    SP<Client> sp_cli = client.lock();
    if (!sp_cli || !from_where || !from_where->active()) return;

    sp_cli->cycle();
    if (sp_cli->shouldShutdown()) close = true;
}

bool State::new_connection(SP<Socket> listening_socket)
{
    assert(listening_socket);

    /* Check for incoming connections. */
    SP<Socket> new_connection(new Socket);
    bool got_connection = listening_socket->accept(new_connection.get());
    if (got_connection)
    {
        new_connection->startAsynchronousReverseResolve();

        SP<Client> new_client = Client::createClient(new_connection);
        new_client->setState(self);
        
        /* Do early check of banned/allowed addresses. Hostname is probably not resolved yet at this point though. */
        if (checkAddressAllowance(new_client))
            return true;

        new_client->setConfigurationDatabase(configuration);
        new_client->setGlobalChatLogger(global_chat);

        LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
        vector<SP<Client> > &cli = *lo_clients.get();
        LockedObject<vector<WP<Client> > > lo_weak_clients = clients_weak.lock();
        vector<WP<Client> > &weak_cli = *lo_weak_clients.get();

        if (cli.size() >= MAX_CONNECTIONS)
        {
            pruneInactiveClients();
            if (cli.size() >= MAX_CONNECTIONS)
            {
                new_connection->send("Maximum number of connections reached.\n");
                LOG(Note, "New connection from " << new_connection->getAddress().getHumanReadablePlainUTF8() << " but disconnected because maximum number of connections has been reached.");
                return true;
            }
        }

        cli.push_back(new_client);
        weak_cli.push_back(new_client);
        
        LOG(Note, "New connection from " << new_connection->getAddress().getHumanReadablePlainUTF8());
        socketevents.addSocket(new_connection);
        socketevents.forceEvent(new_connection);
        
        new_client->sendPrivateChatMessage(MOTD);
        
        size_t i1, len = cli.size();
        for (i1 = 0; i1 < len; ++i1)
            if (cli[i1])
                cli[i1]->updateClientNicklist(&cli);
        return true;
    }

    return false;
}

void State::loop()
{
    unique_lock<recursive_mutex> lock(cycle_mutex);
    close = false;
    while(!close)
    {
        ui64 next_event_time = 5000000000LL; // 5 seconds
        if (!pending_delayed_notifications.empty())
        {
            next_event_time = pending_delayed_notifications.begin()->first - nanoclock();
            if (next_event_time > 5000000000LL)
                next_event_time = 0;
        }

        pruneInactiveClients();
        pruneInactiveSlots();
        flush_messages();
        cycle_mutex.unlock();
        SP<Socket> s = socketevents.getEvent(next_event_time);
        cycle_mutex.lock();
        if (!s) 
        {
            if (pending_delayed_notifications.empty()) continue;

            if (nanoclock() > pending_delayed_notifications.begin()->first)
            {
                SP<Client> c = pending_delayed_notifications.begin()->second.lock();
                pending_delayed_notifications.erase(pending_delayed_notifications.begin());
                if (!c || !c->getSocket()) continue;
                s = c->getSocket();
            }

            if (!s) continue;
        }

        /* Test if it's a listening socket */
        set<SP<Socket> >::iterator i2 = listening_sockets.find(s);
        if (i2 != listening_sockets.end())
        {
            while (new_connection(s)) { };
            continue;
        }
        LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
        vector<SP<Client> > cli = *lo_clients.get();

        bool got_socket = false;

        vector<SP<Client> >::iterator i1, cli_end = cli.end();
        for (i1 = cli.begin(); i1 != cli_end; ++i1)
        {
            if (!(*i1)) continue;
            if ((*i1)->getSocket() == s)
            {
                (*i1)->cycle();
                if ((*i1)->shouldShutdown()) close = true;
                got_socket = true;
                break;
            }
        }

        if (!got_socket)
        {
            while(true)
            {
                SP<Socket> s = http_server.cycle();
                if (!s)
                    break;
                socketevents.addSocket(s);
            }
        };

    }
}

void State::signalSlotData(SP<Slot> who)
{
    assert(who);

    lock_guard<recursive_mutex> lock(cycle_mutex);
    LockedObject<vector<SP<Client> > > lo_clients = clients.lock();
    vector<SP<Client> > &cli = *lo_clients.get();

    vector<SP<Client> >::iterator i1, cli_end = cli.end();
    for (i1 = cli.begin(); i1 != cli_end; ++i1)
    {
        if (!(*i1)) continue;
        if ((*i1)->getSlot().lock() == who)
        {
            (*i1)->cycle();
            if ((*i1)->shouldShutdown()) close = true;
        }
    }
}

