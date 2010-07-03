#include <boost/bind.hpp>
#include <iostream>
#include "configuration_interface.hpp"
#include <algorithm>
#include <cstdio>
#include "logger.hpp"

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
    window->addListElement("Force close running slot", "forceclose", true, false);
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
    vector<WP<Slot> >::iterator i1;
    for (i1 = slots.begin(); i1 != slots.end(); i1++)
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
    if (!st)
    {
        if (user)
            { LOG(Error, "ConfigurationInterface::enterLaunchSlotsMenu(), null state in menu, user " << user->getNameUTF8()); }
        else
            { LOG(Error, "ConfigurationInterface::enterLaunchSlotsMenu(), null state in menu and the user is null too."); }
        slot_index = window->addListElementUTF8("No slots available at the moment", "mainmenu", true, false);
        window->modifyListSelectionIndex(slot_index);
        return;
    }

    slot_index = window->addListElementUTF8("Back to main menu", "mainmenu", true, false);

    vector<WP<SlotProfile> > slotprofiles = st->getSlotProfiles();
    vector<WP<SlotProfile> >::iterator i1;
    for (i1 = slotprofiles.begin(); i1 != slotprofiles.end(); i1++)
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
    if (st)
    {
        vector<WP<SlotProfile> > slotprofiles = st->getSlotProfiles();
        vector<WP<SlotProfile> >::iterator i1;
        for (i1 = slotprofiles.begin(); i1 != slotprofiles.end(); i1++)
        {
            SP<SlotProfile> slot_profile = (*i1).lock();
            if (!slot_profile) continue;

            window->addListElement(slot_profile->getName(), string("editslotprofile_") + slot_profile->getIDRef().serialize(), true, false);
        }
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
    if (!window) return;

    ui32 index;
    string data;
    for (index = 0; (data = window->getListElementData(index)).size() > 0; index++)
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
    if (!window) return;

    ui32 index;
    string data;
    for (index = 0; (data = window->getListElementData(index)).size() > 0; index++)
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
                    if (st != DFGrab || st != DFLaunch) st = DFLaunch;
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
    if (!auxiliary_window) return false;

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
    if (auxiliary_window) return false;

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
        if (!st)
        {
            if (user)
            { LOG(Error, "User " << user->getNameUTF8() << " attempted to configure maximum number of slots but state is null."); }
            else
            { LOG(Error, "Null user attempted to configure maximum number of slots but state is null."); };
        }
        else
        {
            ui32 max_slots = strtol(window->getListElementUTF8(index).c_str(), 0, 10);
            stringstream ss;
            ss << max_slots;
            window->modifyListElementTextUTF8(index, ss.str());

            st->setMaximumNumberOfSlots(max_slots);
        }
    }
    else if (selection == "disconnect")
    {
        if (user)
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
        if (!st)
        {
            if (user)
            { LOG(Error, "User " << user->getNameUTF8() << " attempted to force close a slot but state is null."); }
            else
            { LOG(Error, "Null user attempted to force close a slot but state is null."); };
        }
        else
            st->forceCloseSlotOfUser(user);
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
            if (st)
                st->setMOTD(window->getListElement(0)); /* assuming MotD text is in index 0 */
            else
            {
                if (user)
                { LOG(Error, "User " << user->getNameUTF8() << " attempted to set MotD but state is null."); }
                else
                { LOG(Error, "Null user attempted to set MotD but state is null."); }
            }
        }
        enterMainMenu();
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
        checkSlotProfileMenu();
        if (edit_slotprofile.getName().countChar32() == 0) /* Require a name for the slot */
        {
            if (user)
                { LOG(Note, "User " << user->getNameUTF8() << " attempted to create a slot profile with an empty name."); }
            else
                { LOG(Note, "Null user attempted to create a slot profile with an empty name."); }
            window->modifyListSelectionIndex(1); /* HACK: Assuming the name of the slot profile is in index number 1. */
        }
        else
        {
            SP<State> st = state.lock();
            if (!st)
                { LOG(Error, "Could not delete slot profile from slot profile menu, because state is null. Oopsies. Profile name " << edit_slotprofile.getNameUTF8()); }
            else
            {
                configuration_database->deleteSlotProfileData(edit_slotprofile_sp_target->getName());
                st->deleteSlotProfile(edit_slotprofile_sp_target);
                edit_slotprofile_sp_target = SP<SlotProfile>();
                enterSlotsMenu();
            }
        }
    }
    else if (selection == "newslot_create" || selection == "newslot_save")
    {
        checkSlotProfileMenu();
        if (edit_slotprofile.getName().countChar32() == 0) /* Require a name for the slot */
        {
            if (user)
                { LOG(Note, "User " << user->getNameUTF8() << " attempted to create a slot profile with an empty name."); }
            else
                { LOG(Note, "Null user attempted to create a slot profile with an empty name."); }

            window->modifyListSelectionIndex(1); /* HACK: Assuming the name of the slot profile is in index number 1. */
        }
        else
        {
            SP<State> st = state.lock();
            if (!st)
            {
                if (user)
                    { LOG(Error, "User " << user->getNameUTF8() << " could not save slot profile from slot profile menu. State is null. Oopsies. Slot profile name " << edit_slotprofile.getNameUTF8()); }
                else
                    { LOG(Error, "Null user could not save slot profile from slot profile menu. State is null. Oopsies. Slot profile name " << edit_slotprofile.getNameUTF8()); }
            }
            else
            {
                if (selection == "newslot_create" && st->hasSlotProfile(edit_slotprofile.getID()))
                {
                    if (user)
                        { LOG(Note, "User " << user->getNameUTF8() << " attempted to create a slot profile with an ID that already exists. Slot profile name " << edit_slotprofile.getNameUTF8()); }
                    else
                        { LOG(Note, "Null user attempted to create a slot profile with an ID that already exists. Slot profile name " << edit_slotprofile.getNameUTF8()); }
                    window->modifyListSelectionIndex(1); /* HACK: Assuming the name of the slot profile is in index number 1. */
                    window->modifyListElementTextUTF8(1, window->getListElementUTF8(1) + string("_"));
                }
                else
                {
                    if (selection == "newslot_create")
                    {
                        SP<SlotProfile> slotp(new SlotProfile(edit_slotprofile));
                        if (!st->addSlotProfile(slotp))
                        {
                            configuration_database->saveSlotProfileData(slotp);
                            edit_slotprofile_sp_target = SP<SlotProfile>();

                            if (user)
                                { LOG(Note, "User " << user->getNameUTF8() << " created a new slot profile with the name " << edit_slotprofile.getNameUTF8()); }
                            else
                                { LOG(Note, "Null user created a new slot profile with the name " << edit_slotprofile.getNameUTF8()); }
                            enterSlotsMenu();
                        }
                    }
                    else
                    {
                        configuration_database->deleteSlotProfileData(edit_slotprofile_sp_target->getName());
                        st->updateSlotProfile(edit_slotprofile_sp_target, edit_slotprofile);
                        configuration_database->saveSlotProfileData(edit_slotprofile_sp_target);

                        edit_slotprofile_sp_target = SP<SlotProfile>();

                        if (user)
                            { LOG(Note, "User " << user->getNameUTF8() << " edited and saved slotprofile " << edit_slotprofile.getNameUTF8()); }

                        enterSlotsMenu();
                    }
                }
            }

            return false;
        }
    }
    /* editing slots */
    else if (!selection.compare(0, min(selection.size(), (size_t) 16), "editslotprofile_", 16))
    {
        ID slot_id = ID::getUnSerialized(selection.substr(16));
        SP<State> st = state.lock();
        if (!st)
        {
            if (user)
                { LOG(Error, "User " << user->getNameUTF8() << " requested slot profile edit but state is null. Oopsies."); }
            else
                { LOG(Error, "Null user requested slot profile edit but state is null. Oopsies."); }
        }
        else
        {
            WP<SlotProfile> wp_sp = st->getSlotProfile(slot_id);
            SP<SlotProfile> sp = wp_sp.lock();
            if (!sp)
            {
                if (user)
                    { LOG(Error, "User " << user->getNameUTF8() << " requested edit from interface with name " << sp->getNameUTF8() << " but there's no such slot profile."); }
                else
                    { LOG(Error, "Null user requested edit from interface with name " << sp->getNameUTF8() << " but there's no such slot profile."); }
            }
            else
            {
                edit_slotprofile = (*sp);
                edit_slotprofile_sp_target = sp;
                enterEditSlotProfileMenu();
            }
        }
    }
    /* launching slots */
    else if (!selection.compare(0, min(selection.size(), (size_t) 11), "launchslot_", 11))
    {
        ID slot_id = ID::getUnSerialized(selection.substr(11));
        SP<State> st = state.lock();
        if (!st)
        {
            if (user)
                { LOG(Error, "User " << user->getNameUTF8() << " requested slot launch from interface but state is null. Oops. Slot ID " << slot_id.serialize()); }
            else
                { LOG(Error, "Null user requested slot launch from interface but state is null. Oops. Slot profile ID " << slot_id.serialize() ); }
        }
        else
        {
            if (st->launchSlot(slot_id, user));
                enterMainMenu();
        }
    }
    /* join slots */
    else if (!selection.compare(0, min(selection.size(), (size_t) 9), "joinslot_", 9))
    {
        ID slot_id = ID::getUnSerialized(selection.substr(9));
        SP<State> st = state.lock();
        if (!st)
        {
            if (user)
                { LOG(Error, "User " << user->getNameUTF8() << " requested slot join from interface but state is null. Oopsies. Slot ID " << slot_id.serialize()); }
            else
                { LOG(Error, "Null user requested slot join from interface but state is null. Oopsies. Slot profile name " << slot_id.serialize()); }
        }
        else
            if (st->setUserToSlot(user, slot_id))
                enterMainMenu();
    }
    else if (selection == "join_none")
    {
        SP<State> st = state.lock();
        if (!st)
        {
            if (user)
                { LOG(Error, "User " << user->getNameUTF8() << " requested slot join none from interface but state is null. Oopsies."); }
            else
                { LOG(Error, "Null user requested slot join none from interface but state is null. Oopsies."); }
        }
        else
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
    if (!configuration_database) return;
    if (!auxiliary_window) return;

    auxiliary_window->deleteAllListElements(); 

    auxiliary_window->setTitle("Select users");
    int ok = auxiliary_window->addListElementUTF8("Ok", "usergroup_users_ok", true, false);

    vector<SP<User> > users = configuration_database->loadAllUserData();
    ui32 i1, len = users.size();
    for (i1 = 0; i1 < len; i1++)
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
    if (!auxiliary_window) return;

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
    for (index = 0; (data = auxiliary_window->getListElementData(index)).size() > 0; index++)
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


