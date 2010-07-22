#include <boost/bind.hpp>
#include <iostream>
#include "configuration_interface.hpp"
#include <algorithm>
#include <cstdio>
#include "logger.hpp"

/*
TODO:
This whole interface thing is a sort of mess, compared
to other code. There's some duplicate log message stuff
that irritates me but I can't be bothered to fix it, 
when it works.

 It should not directly interface with ConfigurationDatabase.
 Only the State.
*/

using namespace dfterm;
using namespace boost;
using namespace std;
using namespace trankesbel;

ConfigurationInterface::ConfigurationInterface()
{
    current_menu = MainMenu;
    admin = false;
    shutdown = false;

    true_if_ok = false;
}

ConfigurationInterface::~ConfigurationInterface()
{
}

void ConfigurationInterface::setClient(WP<Client> client)
{
    this->client = client;
}

WP<Client> ConfigurationInterface::getClient()
{
    return client;
};


void ConfigurationInterface::setConfigurationDatabase(SP<ConfigurationDatabase> c_database)
{
    configuration_database = c_database;
}

SP<ConfigurationDatabase> ConfigurationInterface::getConfigurationDatabase()
{
    return configuration_database;
}

void ConfigurationInterface::cycle()
{
}

ConfigurationInterface::ConfigurationInterface(SP<Interface> interface)
{
    setInterface(interface);
}

void ConfigurationInterface::setInterface(SP<Interface> interface)
{
    this->interface = interface;
}

SP<InterfaceElementWindow> ConfigurationInterface::getUserWindow()
{
    if (!window && !interface) return SP<InterfaceElementWindow>();
    if (!window && interface) 
    {
        window = interface->createInterfaceElementWindow();
        window->setListCallback(boost::bind(&ConfigurationInterface::menuSelectFunction, this, _1));
        enterMainMenu();
    }
    if (!window) return SP<InterfaceElementWindow>();

    return window;
}

void ConfigurationInterface::setUser(SP<User> user)
{
    this->user = user;
    if (user->isAdmin()) enableAdmin();
    else disableAdmin();
}

void ConfigurationInterface::enableAdmin()
{
    admin = true;
    enterAdminMainMenu();
}

void ConfigurationInterface::disableAdmin()
{
    admin = false;
    enterMainMenu();
}

void ConfigurationInterface::enterMainMenu()
{
    edit_slotprofile_sp_target = SP<SlotProfile>();

    if (admin)
    {
        enterAdminMainMenu();
        return;
    }

    window->deleteAllListElements();
    window->setHint("default");
    window->setTitle("Main menu");

    current_menu = MainMenu;
    ui32 slot_index = window->addListElement("Launch a new game", "launchgame", true, false);
    window->addListElement("Join a running game", "joingame", true, false);
    window->addListElement("Force close running slot", "forceclose", true, false);
    window->addListElement("Change your password", "setuserpassword", true, false);
    window->addListElement("Disconnect", "disconnect", true, false);
    window->modifyListSelectionIndex(slot_index);
}

void ConfigurationInterface::enterAdminMainMenu()
{
    if (!admin) return;

    window->deleteAllListElements();
    window->setHint("default");
    window->setTitle("Main menu");

    current_menu = AdminMainMenu;

    ui32 slot_index = window->addListElement("Launch a new game", "launchgame", true, false);
    window->addListElement("Join a running game", "joingame", true, false);
    window->addListElement("Configure slots", "slots", true, false);
    window->addListElement("Set MotD", "motd", true, false);
    window->addListElement("Manage users", "manage_users", true, false);
    window->addListElement("Force close running slot", "forceclose", true, false);
    window->addListElement("Change your password", "setuserpassword", true, false);
    window->addListElement("Disconnect", "disconnect", true, false); 
    window->addListElement("Shutdown server", "shutdown", true, false);
    window->modifyListSelectionIndex(slot_index);
}

void ConfigurationInterface::enterJoinSlotsMenu()
{
    window->deleteAllListElements();
    window->setHint("default");
    window->setTitle("Select a game to join");

    current_menu = JoinSlotsMenu;

    ui32 slot_index = 0;
    SP<State> st = state.lock();
    if (!st)
    {
        if (user)
            { LOG(Error, "ConfigurationInterface::enterJoinSlotsMenu(), null state in menu, user " << user->getNameUTF8()); }
        else
            { LOG(Error, "ConfigurationInterface::enterJoinSlotsMenu(), null state in menu and user is null too."); }

        slot_index = window->addListElementUTF8("No slots available at the moment", "mainmenu", true, false);
        window->modifyListSelectionIndex(slot_index);
        return;
    }

    slot_index = window->addListElementUTF8("Back to main menu", "mainmenu", true, false);

    window->addListElementUTF8("None", "join_none", true, false);

    vector<WP<Slot> > slots = st->getSlots();
    vector<WP<Slot> >::iterator i1, slots_end = slots.end();
    for (i1 = slots.begin(); i1 != slots_end; ++i1)
    {
        SP<Slot> sp = (*i1).lock();
        if (!sp) continue;
        if (!st->isAllowedWatcher(user, sp)) continue;

        data1D data_str("joinslot_");
        data_str += sp->getIDRef().serialize();
        window->addListElement(sp->getName(), data_str, true, false);
    }

    window->modifyListSelectionIndex(slot_index);
}

void ConfigurationInterface::enterLaunchSlotsMenu()
{
    window->deleteAllListElements();
    window->setHint("default");
    window->setTitle("Select a game to launch");

    current_menu = LaunchSlotsMenu;

    ui32 slot_index = 0;
    SP<State> st = state.lock();
    assert(st && user);

    slot_index = window->addListElementUTF8("Back to main menu", "mainmenu", true, false);

    vector<WP<SlotProfile> > slotprofiles = st->getSlotProfiles();
    vector<WP<SlotProfile> >::iterator i1, slotprofiles_end = slotprofiles.end();
    for (i1 = slotprofiles.begin(); i1 != slotprofiles.end(); ++i1)
    {
        SP<SlotProfile> sp = (*i1).lock();
        if (!sp) continue;
        if (!st->isAllowedLauncher(user, sp)) continue;

        data1D data_str("launchslot_");
        data_str += sp->getID().serialize();
        window->addListElement(sp->getName(), data_str, true, false);
    }

    window->modifyListSelectionIndex(slot_index);
}

void ConfigurationInterface::enterSetPasswordMenu(const ID &user_id, bool admin_menu)
{
    user_target = user_id;

    old_password_index = 0xffffffff;
    password_index = 0xffffffff;
    retype_password_index = 0xffffffff;

    bool admin_password_change = false;
    if (admin && admin_menu)
        admin_password_change = true;

    SP<State> st = state.lock();
    assert(st && user);
    if (!admin && user->getIDRef() != user_id)
    {
        LOG(Error, "Non-admin user tried to set a password for a user that is not themselves.");
        return;
    }

    int back_index = 0;

    window->deleteAllListElements();
    window->setHint("wide");
    window->setTitle("Set password");

    if (admin_password_change)
    {
        back_index = window->addListElementUTF8("Back to user accounts menu", "showaccounts", true, false);
        password_index = window->addListElementUTF8("", "Password: ", "password", true, true);
        retype_password_index = window->addListElementUTF8("", "Retype password: ", "retype_password", true, true);
        window->addListElementUTF8("Save", "savepassword", true, false);
    }
    else
    {
        back_index = window->addListElementUTF8("Back to main menu", "mainmenu", true, false);
        old_password_index = window->addListElementUTF8("", "Old password: ", "old_password", true, true);
        password_index = window->addListElementUTF8("", "Password: ", "password", true, true);
        retype_password_index = window->addListElementUTF8("", "Retype password: ", "retype_password", true, true);
        window->addListElementUTF8("Save", "savepassword", true, false);
    }

    window->modifyListSelectionIndex(back_index);
}

void ConfigurationInterface::enterShowClientInformationMenu(SP<Client> c)
{
    assert(c);
    if (!admin) return;

    window->deleteAllListElements();
    window->setHint("default");
    window->setTitle("Connection information");

    SP<User> u = c->getUser();
    SP<Socket> s = c->getSocket();
    
    int first_index = window->addListElementUTF8("Back to manage users menu", "manage_users", true, false);
    window->modifyListSelectionIndex(first_index);
    window->addListElementUTF8("Force disconnect", "forcedisconnect", true, false);

    string userstring = string("User: \"");
    if (u)
        userstring += u->getNameUTF8() + string("\"");
    else
        userstring += "(Unidentified)";

    window->addListElementUTF8(userstring, "", false, false);

    string ip_address, hostname;
    if (s && s->active())
    {
        SocketAddress sa = s->getAddress();
        ip_address = string("IP-address: ") + sa.getHumanReadablePlainUTF8();
        hostname = sa.getHostnameUTF8();
    }
    else
    {
        ip_address = string("IP-address: No connection");
    }

    if (hostname.size() == 0)
        hostname = string("Host name: (Unknown)");
    else
        hostname = string("Host name: \"") + hostname + string("\"");

    window->addListElementUTF8(ip_address, "", false, false);
    window->addListElementUTF8(hostname, "", false, false);

    client_target = c->getID();
}

void ConfigurationInterface::enterShowAccountsMenu()
{
    if (!admin) return;

    window->deleteAllListElements();
    window->setHint("chat");
    window->setTitle("User list");

    int back_index = window->addListElement("Back to connection list", "manage_users", true, false);
    window->modifyListSelectionIndex(back_index);

    SP<State> st = state.lock();
    assert(st);

    vector<SP<User> > users;
    st->getAllUsers(&users);

    vector<SP<User> >::iterator i1, users_end = users.end();
    for (i1 = users.begin(); i1 != users_end; ++i1)
    {
        if ( !(*i1) ) continue;

        string userstring = (*i1)->getNameUTF8();
        if (userstring.size() == 0)
            userstring = string("(Anonymous)");
        else
            userstring = string("\"") + userstring + string("\"");

        string datastring = "user_" + (*i1)->getIDRef().serialize();
        window->addListElementUTF8(userstring, datastring, true, false);
    }
}

void ConfigurationInterface::enterManageAccountMenu(const ID &user_id)
{
    if (!admin) return;

    window->deleteAllListElements();
    window->setHint("chat");
    window->setTitle("User information");

    int back_index = window->addListElement("Back to manage user accounts menu", "showaccounts", true, false);
    window->modifyListSelectionIndex(back_index);

    SP<State> st = state.lock();
    assert(st);

    SP<User> user = st->getUser(user_id);
    if (!user)
    {
        LOG(Error, "Attempt to show user information for user ID " << user_id.serialize() << " but no such user exists.");
        window->addListElement("This user does not exist.", "", true, true);
        return;
    }

    window->addListElementUTF8(string("Name: \"") + user->getNameUTF8() + string("\""), "", true, false);
    window->addListElementUTF8(string("ID: ") + user->getIDRef().serialize(), "", true, false);
    window->addListElementUTF8(string("Password hash: ") + user->getPasswordHash(), "", true, false);
    window->addListElementUTF8(string("Password salt: ") + user->getPasswordSalt(), "", true, false);
    window->addListElementUTF8("Set new password", "setpassword", true, false);
    window->addListElementUTF8("Delete user", "deleteuser", true, false);
}

void ConfigurationInterface::enterManageUsersMenu()
{
    if (!admin) return;

    window->deleteAllListElements();
    window->setHint("chat");
    window->setTitle("Connection list");

    current_menu = ManageUsersMenu;

    int back_index = window->addListElement("Back to main menu", "mainmenu", true, false);
    window->modifyListSelectionIndex(back_index);
    window->addListElement("Show user accounts", "showaccounts", true, false);

    SP<State> st = state.lock();
    assert(st);

    LockedObject<vector<SP<Client> > > lo_clients = st->getAllClients();
    vector<SP<Client> > &clients = *lo_clients.get();

    vector<SP<Client> >::iterator i1, clients_end = clients.end();
    for (i1 = clients.begin(); i1 != clients_end; ++i1)
    {
        if ( !(*i1) ) continue;
        if ( !(*i1)->isActive() ) continue;

        string userstring;

        if ( !(*i1)->getUser() || (*i1)->getUser()->getName().length() == 0)
            userstring = "(Unidentified)";
        else
            userstring = string("\"") + (*i1)->getUser()->getNameUTF8() + string("\"");

        string datastring = string("client_") + (*i1)->getIDRef().serialize();

        window->addListElementUTF8(userstring, datastring, true, false);
    }
}

void ConfigurationInterface::enterMotdMenu()
{
    if (!admin) return;
    
    window->deleteAllListElements();
    window->setHint("chat");
    window->setTitle("Message of the Day");
    
    current_menu = MotdMenu;
    
    int motd_index = window->addListElement("", "motd_text", true, true);
    if (configuration_database)
        window->modifyListElementText(motd_index, configuration_database->loadMOTD());
    
    window->addListElement("Cancel", "mainmenu", true, false);
    window->addListElement("Set MotD", "motd_set", true, false);
    window->modifyListSelectionIndex(motd_index);
}

void ConfigurationInterface::enterSlotsMenu()
{
    if (!admin) return;

    window->deleteAllListElements();
    window->setHint("wide");
    window->setTitle("Slot configuration");

    current_menu = SlotsMenu;

    int slot_index = window->addListElement("Back to main menu", "mainmenu", true, false);
    window->addListElement("Add a new slot profile", "new_slotprofile", true, false);
    window->addListElement("", "Maximum number of slots running at a time: ", "max_slots", true, true);
    window->modifyListSelectionIndex(slot_index);

    SP<State> st = state.lock();
    assert(st);
    vector<WP<SlotProfile> > slotprofiles = st->getSlotProfiles();
    vector<WP<SlotProfile> >::iterator i1, slotprofiles_end = slotprofiles.end();
    for (i1 = slotprofiles.begin(); i1 != slotprofiles_end; ++i1)
    {
        SP<SlotProfile> slot_profile = (*i1).lock();
        if (!slot_profile) continue;

        window->addListElement(slot_profile->getName(), string("editslotprofile_") + slot_profile->getIDRef().serialize(), true, false);
    }

    checkSlotsMenu(true);
}

void ConfigurationInterface::enterEditSlotProfileMenu()
{
    if (!admin) return;
    
    window->deleteAllListElements();
    window->setHint("wide");
    window->setTitle("Editing a new slot profile");

    current_menu = SlotsMenu;

    int slot_index = window->addListElement("Back to slot profile menu", "slots", true, false);
    window->addListElement("",                            "Slot profile name:         ", "newslot_name", true, true);
    window->addListElement("launch in a pty+vt102/ANSI",  "Method of screen scraping: ", "newslot_method", true, false);
    window->addListElement("",                            "Game executable path:      ", "newslot_executable", true, true);
    window->addListElement("",                            "Game working directory:    ", "newslot_workingdir", true, true);
    window->addListElement("Anybody",                     "Allowed watchers:          ", "newslot_watchers", true, false);
    window->addListElement("Anybody",                     "Allowed launchers:         ", "newslot_launchers", true, false);
    window->addListElement("Anybody",                     "Allowed players:           ", "newslot_players", true, false);
    window->addListElement("Anybody",                     "Allowed force closers:     ", "newslot_closers", true, false);
    window->addListElement("Nobody",                      "Forbidden watchers:        ", "newslot_watchers_forbidden", true, false);
    window->addListElement("Nobody",                      "Forbidden launchers:       ", "newslot_launchers_forbidden", true, false);
    window->addListElement("Nobody",                      "Forbidden players:         ", "newslot_players_forbidden", true, false);
    window->addListElement("Nobody",                      "Forbidden force closers:   ", "newslot_closers_forbidden", true, false);
    window->addListElement("80",                          "Width:                     ", "newslot_width", true, true);
    window->addListElement("25",                          "Height:                    ", "newslot_height", true, true);
    window->addListElement("1",                           "Maximum slots:             ", "newslot_maxslots", true, true);
    window->addListElement(                               "Save slot profile",        "newslot_save", true, false);
    window->addListElement(                               "Delete slot profile",      "newslot_delete", true, false);

    window->modifyListSelectionIndex(slot_index);

    checkSlotProfileMenu(true);
}

void ConfigurationInterface::enterNewSlotProfileMenu()
{
    if (!admin) return;
    
    window->deleteAllListElements();
    window->setHint("wide");
    window->setTitle("Creating a new slot profile");

    current_menu = SlotsMenu;

    int slot_index = window->addListElement("Back to slot profile menu", "slots", true, false);
    window->addListElement("",                            "Slot profile name:         ", "newslot_name", true, true);
    window->addListElement("launch in a pty+vt102/ANSI",  "Method of screen scraping: ", "newslot_method", true, false);
    window->addListElement("",                            "Game executable path:      ", "newslot_executable", true, true);
    window->addListElement("",                            "Game working directory:    ", "newslot_workingdir", true, true);
    window->addListElement("Anybody",                     "Allowed watchers:          ", "newslot_watchers", true, false);
    window->addListElement("Anybody",                     "Allowed launchers:         ", "newslot_launchers", true, false);
    window->addListElement("Anybody",                     "Allowed players:           ", "newslot_players", true, false);
    window->addListElement("Anybody",                     "Allowed force closers:     ", "newslot_closers", true, false);
    window->addListElement("Nobody",                      "Forbidden watchers:        ", "newslot_watchers_forbidden", true, false);
    window->addListElement("Nobody",                      "Forbidden launchers:       ", "newslot_launchers_forbidden", true, false);
    window->addListElement("Nobody",                      "Forbidden players:         ", "newslot_players_forbidden", true, false);
    window->addListElement("Anybody",                     "Forbidden force closers:   ", "newslot_closers_forbidden", true, false);
    window->addListElement("80",                          "Width:                     ", "newslot_width", true, true);
    window->addListElement("25",                          "Height:                    ", "newslot_height", true, true);
    window->addListElement("1",                           "Maximum slots:             ", "newslot_maxslots", true, true);
    window->addListElement(                               "Create slot profile",         "newslot_create", true, false);

    window->modifyListSelectionIndex(slot_index);

    checkSlotProfileMenu();
}

void ConfigurationInterface::checkSlotsMenu(bool no_read)
{
    assert(window);

    ui32 index;
    string data;
    for (index = 0; (data = window->getListElementData(index)).size() > 0; ++index)
    {
        if (data == "max_slots")
        {
            SP<State> st = state.lock();
            if (st)
            {
                ui32 max_slots = 0xffffffff;
                if (!no_read)
                    max_slots = strtol(window->getListElementUTF8(index).c_str(), 0, 10);
                else
                    max_slots = st->getMaximumNumberOfSlots();

                stringstream ss;
                ss << max_slots;
                window->modifyListElementTextUTF8(index, ss.str());

                st->setMaximumNumberOfSlots(max_slots);
            }
        }
    }
}

void ConfigurationInterface::checkSlotProfileMenu(bool no_read)
{
    assert(window);

    ui32 index;
    string data;
    for (index = 0; (data = window->getListElementData(index)).size() > 0; ++index)
    {
        UserGroup ug;
        if (data == "newslot_watchers")
            ug = edit_slotprofile.getAllowedWatchers();
        else if (data == "newslot_launchers")
            ug = edit_slotprofile.getAllowedLaunchers();
        else if (data == "newslot_players")
            ug = edit_slotprofile.getAllowedPlayers();
        else if (data == "newslot_closers")
            ug = edit_slotprofile.getAllowedClosers();
        else if (data == "newslot_watchers_forbidden")
            ug = edit_slotprofile.getForbiddenWatchers();
        else if (data == "newslot_launchers_forbidden")
            ug = edit_slotprofile.getForbiddenLaunchers();
        else if (data == "newslot_players_forbidden")
            ug = edit_slotprofile.getForbiddenPlayers();
        else if (data == "newslot_closers_forbidden")
            ug = edit_slotprofile.getForbiddenClosers();
        else
        {
            char number[50];
            number[49] = 0;
            if (data == "newslot_width")
            {
                if (!no_read)
                {
                    edit_slotprofile.setWidth(strtol(window->getListElementUTF8(index).c_str(), (char**) 0, 10));
                    if (edit_slotprofile.getWidth() < 1) edit_slotprofile.setWidth(1);
                    if (edit_slotprofile.getWidth() > 300) edit_slotprofile.setWidth(300);
                }

                sprintf(number, "%d", edit_slotprofile.getWidth());
                window->modifyListElementTextUTF8(index, number);
            }
            else if (data == "newslot_height")
            {
                if (!no_read)
                {
                    edit_slotprofile.setHeight(strtol(window->getListElementUTF8(index).c_str(), (char**) 0, 10));
                    if (edit_slotprofile.getHeight() < 1) edit_slotprofile.setHeight(1);
                    if (edit_slotprofile.getHeight() > 300) edit_slotprofile.setHeight(300);
                }

                sprintf(number, "%d", edit_slotprofile.getHeight());
                window->modifyListElementTextUTF8(index, number);
            }
            else if (data == "newslot_workingdir")
            {
                if (!no_read) edit_slotprofile.setWorkingPath(window->getListElement(index));
                window->modifyListElementText(index, edit_slotprofile.getWorkingPath());
            }
            else if (data == "newslot_executable")
            {
                if (!no_read) edit_slotprofile.setExecutable(window->getListElement(index));
                window->modifyListElementText(index, edit_slotprofile.getExecutable());
            }
            else if (data == "newslot_method")
            {
                SlotType st = (SlotType) edit_slotprofile.getSlotType();
                if (!no_read)
                {
                    #ifdef _WIN32
                    if (st != DFGrab && st != DFLaunch) st = DFLaunch;
                    #else
                    if (st != TerminalLaunch) st = TerminalLaunch;
                    #endif
                    edit_slotprofile.setSlotType(st);
                }
                
                if (st == DFGrab)
                    window->modifyListElementTextUTF8(index, "Grab a running DF instance (win32)");
                else if (st == DFLaunch)
                    window->modifyListElementTextUTF8(index, "Launch a new DF instance (win32)");
                else if (st == TerminalLaunch)
                    window->modifyListElementTextUTF8(index, "Launch a new DF instance (pty+vt102)");
                else 
                    window->modifyListElementTextUTF8(index, "Unknown slot type");
            }
            else if (data == "newslot_name")
            {
                if (!no_read) edit_slotprofile.setName(window->getListElement(index));
                window->modifyListElementText(index, edit_slotprofile.getName());
            }
            else if (data == "newslot_maxslots")
            {
                if (!no_read)
                {
                    edit_slotprofile.setMaxSlots(strtol(window->getListElementUTF8(index).c_str(), (char**) 0, 10));
                    if (edit_slotprofile.getMaxSlots() < 1) edit_slotprofile.setMaxSlots(1);
                }

                sprintf(number, "%d", edit_slotprofile.getMaxSlots());
                window->modifyListElementTextUTF8(index, number);
            }

            continue;
        }

        if (ug.hasNobody())
            window->modifyListElementTextUTF8(index, "Nobody");
        else if (ug.hasAnybody())
            window->modifyListElementTextUTF8(index, "Anybody");
        else if (ug.hasAnySpecificUser())
            window->modifyListElementTextUTF8(index, "Specific users");
        else if (ug.hasLauncher())
            window->modifyListElementTextUTF8(index, "Launcher");
        else
            window->modifyListElementTextUTF8(index, "What?"); /* should not happen. */
    }
}

bool ConfigurationInterface::auxiliaryMenuSelectFunction(ui32 index)
{
    assert(auxiliary_window);

    data1D selection = auxiliary_window->getListElementData(index);
    if (selection == "usergroup_nobody")
        edit_usergroup.setNobody();   /* toggling doesn't seem to make much sense for "nobody" */
    else if (selection == "usergroup_anybody")
        edit_usergroup.toggleAnybody();
    else if (selection == "usergroup_launcher")
        edit_usergroup.toggleLauncher();
    else if (selection == "usergroup_users" && !edit_usergroup.hasAnybody())
        auxiliaryEnterSpecificUsersWindow();
    else if (selection == "usergroup_users_ok")
        auxiliaryEnterUsergroupWindow();
    else if (selection == "usergroup_ok")
    {
        true_if_ok = true;
        window->gainFocus();
        auxiliary_window = SP<InterfaceElementWindow>();
        if (edit_slotprofile_target == "watchers")
            edit_slotprofile.setAllowedWatchers(edit_usergroup);
        else if (edit_slotprofile_target == "launchers")
            edit_slotprofile.setAllowedLaunchers(edit_usergroup);
        else if (edit_slotprofile_target == "players")
            edit_slotprofile.setAllowedPlayers(edit_usergroup);
        else if (edit_slotprofile_target == "closers")
            edit_slotprofile.setAllowedClosers(edit_usergroup);
        else if (edit_slotprofile_target == "forbidden_watchers")
            edit_slotprofile.setForbiddenWatchers(edit_usergroup);
        else if (edit_slotprofile_target == "forbidden_launchers")
            edit_slotprofile.setForbiddenLaunchers(edit_usergroup);
        else if (edit_slotprofile_target == "forbidden_players")
            edit_slotprofile.setForbiddenPlayers(edit_usergroup);
        else if (edit_slotprofile_target == "forbidden_closers")
            edit_slotprofile.setForbiddenClosers(edit_usergroup);
        checkSlotProfileMenu();
    }
    else if (selection == "usergroup_cancel")
    {
        true_if_ok = false;
        window->gainFocus();
        auxiliary_window = SP<InterfaceElementWindow>();
    }
    else if (!selection.compare(0, min(selection.size(), (size_t) 11), "userselect_", 11))
    {
        ID user_id = ID::getUnSerialized(selection.substr(11));
        edit_usergroup.toggleUser(user_id);
    }

    checkAuxiliaryWindowUsergroupSelections();

    return false;
}

bool ConfigurationInterface::menuSelectFunction(ui32 index)
{
    assert(window);

    checkSlotsMenu();

    data1D selection = window->getListElementData(index);
    if (selection == "mainmenu")
    {
        enterMainMenu();
    }
    else if (selection == "new_slotprofile")
    {
        edit_slotprofile = SlotProfile();
        enterNewSlotProfileMenu();
    }
    else if (selection == "max_slots")
    {
        SP<State> st = state.lock();
        assert(st && user);

        ui32 max_slots = strtol(window->getListElementUTF8(index).c_str(), 0, 10);
        stringstream ss;
        ss << max_slots;
        window->modifyListElementTextUTF8(index, ss.str());

        st->setMaximumNumberOfSlots(max_slots);
    }
    else if (selection == "disconnect")
    {
        assert(user);
        user->kill();
        return false;
    }
    else if (selection == "shutdown")
    {
        shutdown = true;
        return false;
    }
    else if (selection == "slots")
    {
        enterSlotsMenu();
        return false;
    }
    /* To the game menu */
    else if (selection == "launchgame")
    {
        enterLaunchSlotsMenu();
    }
    else if (selection == "joingame")
    {
        enterJoinSlotsMenu();
    }
    else if (selection == "forceclose")
    {
        SP<State> st = state.lock();
        assert(st && user);
        st->forceCloseSlotOfUser(user);
    }
    else if (selection == "forcedisconnect")
    {
        SP<State> st = state.lock();
        assert(st && user);
        st->destroyClient(client_target);
        enterManageUsersMenu();
    }
    /* in show user accounts menu */
    else if (!selection.compare(0, min(selection.size(), (size_t) 5), "user_", 5))
    {
        user_target = ID::getUnSerialized(selection.substr(5));
        enterManageAccountMenu(user_target);
    }
    else if (selection == "savepassword")
    {
        SP<State> st = state.lock();
        assert(st && user);

        SP<User> user_sp = st->getUser(user_target);
        if (!user_sp)
        {
            LOG(Error, "User " << user->getNameUTF8() << " attempted to set a password for user ID " << user_target.serialize() << " but there is no such user.");
        }
        else
        {
            bool old_password_ok = false;
            if (old_password_index == -1) old_password_ok = true;


            if (!old_password_ok)
            {
                UnicodeString old_password = window->getListElement(old_password_index);
                old_password_ok = user_sp->verifyPassword(old_password);

                if (!old_password_ok)
                {
                    window->modifyListElementTextUTF8(old_password_index, "");
                    window->modifyListSelectionIndex(old_password_index);
                }
            }

            if (old_password_ok)
            {
                string password = window->getListElementUTF8(password_index);
                string retype_password = window->getListElementUTF8(retype_password_index);

                if (password != retype_password)
                {
                    window->modifyListElementTextUTF8(password_index, "");
                    window->modifyListElementTextUTF8(retype_password_index, "");
                    window->modifyListSelectionIndex(password_index);
                }
                else
                {
                    user_sp->setPassword(password);
                    st->saveUser(user_sp);
                    if (old_password_index != -1)
                        enterMainMenu();
                    else
                        enterManageAccountMenu(user_target);
                }
            }
        }
    }
    else if (selection == "setpassword")
    {
        enterSetPasswordMenu(user_target, true);
    }
    else if (selection == "setuserpassword")
    {
        assert(user);
        user_target = user->getIDRef();
        enterSetPasswordMenu(user_target, false);
    }
    else if (selection == "deleteuser")
    {
        SP<State> st = state.lock();
        assert(st && user);
        st->destroyClientAndUser(user_target);
        enterShowAccountsMenu();
    }
    else if (selection == "manage_users")
    {
        enterManageUsersMenu();
    }
    else if (selection == "showaccounts")
    {
        enterShowAccountsMenu();
    }
    else if (!selection.compare(0, min(selection.size(), (size_t) 7), "client_", 7))
    {
        ID client_id = ID::getUnSerialized(selection.substr(7));
        SP<State> st = state.lock();
        assert(st && user);
        LOG(Error, "User " << user->getNameUTF8() << " attempted to view connection information for client ID " << client_id.serialize() << " but state is null.");
        SP<Client> client = st->getClient(client_id);
        if (!client)
        {
            LOG(Error, "User " << user->getNameUTF8() << " attempted to view connection information for client ID " << client_id.serialize() << " but no such client is in state.");
        }
        else
            enterShowClientInformationMenu(client);
    }
    /* Motd */
    else if (selection == "motd")
    {
        enterMotdMenu();
    }
    else if (selection == "motd_text")
    {
        window->modifyListSelectionIndex(window->getListSelectionIndex()+1);
    }
    else if (selection == "motd_set")
    {
        if (admin)
        {
            SP<State> st = state.lock();
            assert(st);
            st->setMOTD(window->getListElement(0)); /* assuming MotD text is in index 0 */
        }
        enterMainMenu();
    }
    else if (selection == "newslot_method")
    {
        #ifdef _WIN32
        if ((SlotType) edit_slotprofile.getSlotType() == DFGrab)
            edit_slotprofile.setSlotType((ui32) DFLaunch);
        else
            edit_slotprofile.setSlotType((ui32) DFGrab);
        #endif
    }
    /* The new slot profile menu */
    else if (selection == "newslot_watchers")
    {
        edit_usergroup = edit_slotprofile.getAllowedWatchers();
        edit_slotprofile_target = "watchers";
        auxiliaryEnterUsergroupWindow();
    }
    else if (selection == "newslot_launchers")
    {
        edit_usergroup = edit_slotprofile.getAllowedLaunchers();
        edit_slotprofile_target = "launchers";
        auxiliaryEnterUsergroupWindow();
    }
    else if (selection == "newslot_players")
    {
        edit_usergroup = edit_slotprofile.getAllowedPlayers();
        edit_slotprofile_target = "players";
        auxiliaryEnterUsergroupWindow();
    }
    else if (selection == "newslot_closers")
    {
        edit_usergroup = edit_slotprofile.getAllowedClosers();
        edit_slotprofile_target = "closers";
        auxiliaryEnterUsergroupWindow();
    }
    else if (selection == "newslot_watchers_forbidden")
    {
        edit_usergroup = edit_slotprofile.getForbiddenWatchers();
        edit_slotprofile_target = "forbidden_watchers";
        auxiliaryEnterUsergroupWindow();
    }
    else if (selection == "newslot_launchers_forbidden")
    {
        edit_usergroup = edit_slotprofile.getForbiddenLaunchers();
        edit_slotprofile_target = "forbidden_launchers";
        auxiliaryEnterUsergroupWindow();
    }
    else if (selection == "newslot_players_forbidden")
    {
        edit_usergroup = edit_slotprofile.getForbiddenPlayers();
        edit_slotprofile_target = "forbidden_players";
        auxiliaryEnterUsergroupWindow();
    }
    else if (selection == "newslot_closers_forbidden")
    {
        edit_usergroup = edit_slotprofile.getForbiddenClosers();
        edit_slotprofile_target = "forbidden_closers";
        auxiliaryEnterUsergroupWindow();
    }
    else if (selection == "newslot_delete")
    {
        assert(user);
        checkSlotProfileMenu();
        if (edit_slotprofile.getName().countChar32() == 0) /* Require a name for the slot */
        {
            LOG(Note, "User " << user->getNameUTF8() << " attempted to create a slot profile with an empty name.");
            window->modifyListSelectionIndex(1); /* HACK: Assuming the name of the slot profile is in index number 1. */
        }
        else
        {
            SP<State> st = state.lock();
            assert(st);

            configuration_database->deleteSlotProfileData(edit_slotprofile_sp_target->getName());
            st->deleteSlotProfile(edit_slotprofile_sp_target);
            edit_slotprofile_sp_target = SP<SlotProfile>();
            enterSlotsMenu();
        }
    }
    else if (selection == "newslot_create" || selection == "newslot_save")
    {
        assert(user);
        checkSlotProfileMenu();
        if (edit_slotprofile.getName().countChar32() == 0) /* Require a name for the slot */
        {
            LOG(Note, "User " << user->getNameUTF8() << " attempted to create a slot profile with an empty name.");
            window->modifyListSelectionIndex(1); /* HACK: Assuming the name of the slot profile is in index number 1. */
        }
        else
        {
            SP<State> st = state.lock();
            assert(st);

            if (selection == "newslot_create")
            {
                SP<SlotProfile> slotp(new SlotProfile(edit_slotprofile));
                if (st->addSlotProfile(slotp))
                {
                    configuration_database->saveSlotProfileData(slotp);
                    edit_slotprofile_sp_target = SP<SlotProfile>();

                    LOG(Note, "User " << user->getNameUTF8() << " created a new slot profile with the name " << edit_slotprofile.getNameUTF8());
                    enterSlotsMenu();
                }
            }
            else
            {
                configuration_database->deleteSlotProfileData(edit_slotprofile_sp_target->getName());
                st->updateSlotProfile(edit_slotprofile_sp_target, edit_slotprofile);
                configuration_database->saveSlotProfileData(edit_slotprofile_sp_target);

                edit_slotprofile_sp_target = SP<SlotProfile>();

                LOG(Note, "User " << user->getNameUTF8() << " edited and saved slotprofile " << edit_slotprofile.getNameUTF8());

                enterSlotsMenu();
            }

            return false;
        }
    }
    /* editing slots */
    else if (!selection.compare(0, min(selection.size(), (size_t) 16), "editslotprofile_", 16))
    {
        ID slot_id = ID::getUnSerialized(selection.substr(16));
        SP<State> st = state.lock();
        assert(st && user);
        WP<SlotProfile> wp_sp = st->getSlotProfile(slot_id);
        SP<SlotProfile> sp = wp_sp.lock();
        if (!sp)
        {
            LOG(Error, "User " << user->getNameUTF8() << " requested edit from interface with name " << sp->getNameUTF8() << " but there's no such slot profile.");
        }
        else
        {
            edit_slotprofile = (*sp);
            edit_slotprofile_sp_target = sp;
            enterEditSlotProfileMenu();
        }
    }
    /* launching slots */
    else if (!selection.compare(0, min(selection.size(), (size_t) 11), "launchslot_", 11))
    {
        ID slot_id = ID::getUnSerialized(selection.substr(11));
        SP<State> st = state.lock();
        assert(st && user);

        if (st->launchSlot(slot_id, user))
            enterMainMenu();
    }
    /* join slots */
    else if (!selection.compare(0, min(selection.size(), (size_t) 9), "joinslot_", 9))
    {
        ID slot_id = ID::getUnSerialized(selection.substr(9));
        SP<State> st = state.lock();
        assert(st && user);

        if (st->setUserToSlot(user, slot_id))
            enterMainMenu();
    }
    else if (selection == "join_none")
    {
        SP<State> st = state.lock();
        assert(st && user);
        st->setUserToSlot(user, ID());
    }

    checkSlotProfileMenu();
    return false;
};

void ConfigurationInterface::auxiliaryEnterUsergroupWindow()
{
    if (!auxiliary_window) auxiliary_window = interface->createInterfaceElementWindow();

    auxiliary_window->setHint("wide");
    auxiliary_window->deleteAllListElements();

    auxiliary_window->setTitle("Select user group");
    auxiliary_window->addListElement("Nobody", "usergroup_nobody", true, false);
    auxiliary_window->addListElement("Anybody", "usergroup_anybody", true, false);
    auxiliary_window->addListElement("Launcher", "usergroup_launcher", true, false);
    auxiliary_window->addListElement("Specific users", "usergroup_users", true, false);
    int ok = auxiliary_window->addListElement("Ok", "usergroup_ok", true, false);
    auxiliary_window->addListElement("Cancel", "usergroup_cancel", true, false);

    auxiliary_window->modifyListSelectionIndex(ok);
    auxiliary_window->setListCallback(boost::bind(&ConfigurationInterface::auxiliaryMenuSelectFunction, this, _1));

    checkAuxiliaryWindowUsergroupSelections();

    interface->cycle();
    auxiliary_window->gainFocus();
}

void ConfigurationInterface::auxiliaryEnterSpecificUsersWindow()
{
    assert(configuration_database && auxiliary_window);

    auxiliary_window->deleteAllListElements(); 

    auxiliary_window->setTitle("Select users");
    int ok = auxiliary_window->addListElementUTF8("Ok", "usergroup_users_ok", true, false);

    vector<SP<User> > users = configuration_database->loadAllUserData();
    ui32 i1, len = users.size();
    for (i1 = 0; i1 < len; ++i1)
    {
        if (!users[i1]) continue;
        string element = string("\"") + users[i1]->getNameUTF8() + string("\"");
        if (edit_usergroup.hasUser(users[i1]->getIDRef())) element.push_back('*');

        string user_str = string("userselect_") + users[i1]->getID().serialize();
        auxiliary_window->addListElementUTF8(element.c_str(), user_str.c_str(), true, false);
    }

    auxiliary_window->modifyListSelectionIndex(ok);
    auxiliary_window->setListCallback(boost::bind(&ConfigurationInterface::auxiliaryMenuSelectFunction, this, _1));

    checkAuxiliaryWindowUsergroupSelections();

    interface->cycle();
    auxiliary_window->gainFocus();
}

void ConfigurationInterface::checkAuxiliaryWindowUsergroupSelections()
{
    assert(auxiliary_window);

    /* This function iteraters over all list elements in auxiliary window, and updates
     * the texts to reflect what users the user has selected for a user group.
     * Se we can have the list looking like:
     * | Nobody          |
     * | Anybody*        |
     * | Launcher*       |
     * | Specific users* |
     * | Ok              |
     * | Cancel          |
     * (where * denotes selected users) */
    ui32 index;
    string data;
    for (index = 0; (data = auxiliary_window->getListElementData(index)).size() > 0; ++index)
    {
        if (data == "usergroup_nobody")
        {
            if (edit_usergroup.hasNobody())
                auxiliary_window->modifyListElementTextUTF8(index, "Nobody*");
            else
                auxiliary_window->modifyListElementTextUTF8(index, "Nobody");
        }
        else if (data == "usergroup_anybody")
        {
            if (edit_usergroup.hasAnybody())
                auxiliary_window->modifyListElementTextUTF8(index, "Anybody*");
            else
                auxiliary_window->modifyListElementTextUTF8(index, "Anybody");
        }
        else if (data == "usergroup_launcher")
        {
            if (edit_usergroup.hasLauncher())
                auxiliary_window->modifyListElementTextUTF8(index, "Launcher*");
            else
                auxiliary_window->modifyListElementTextUTF8(index, "Launcher");
        }
        else if (data == "usergroup_users")
        {
            if (edit_usergroup.hasAnybody())
                auxiliary_window->modifyListElementTextUTF8(index, "(Specific users)");
            else if (edit_usergroup.hasAnySpecificUser())
                auxiliary_window->modifyListElementTextUTF8(index, "Specific users*");
            else
                auxiliary_window->modifyListElementTextUTF8(index, "Specific users");
        }
        else if (!data.compare(0, min(data.size(), (size_t) 11), "userselect_", 11))
        {
            ID user_id = ID::getUnSerialized(data.substr(11));
            SP<State> st = state.lock();
            if (!st)
            { LOG(Error, "Tried to toggle user " << user_id.serialize() << " in user group selection window but state is null."); }
            else
            {
                SP<User> user = st->getUser(user_id);
                if (!user)
                { LOG(Error, "Tried to toggle user " << user_id.serialize() << " but such user can't be found."); }
                else
                {
                    string username = user->getNameUTF8();
                    if (edit_usergroup.hasUser(user_id))
                        auxiliary_window->modifyListElementTextUTF8(index, string("\"") + username + string("\"*"));
                    else
                        auxiliary_window->modifyListElementTextUTF8(index, string("\"") + username + string("\""));
                }
            }
        }
    }
}

bool ConfigurationInterface::shouldShutdown() const
{ return shutdown; };


