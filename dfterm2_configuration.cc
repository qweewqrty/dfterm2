#include <boost/bind.hpp>
#include <iostream>
#include "dfterm2_configuration.hpp"

using namespace dfterm;
using namespace boost;
using namespace std;

ConfigurationInterface::ConfigurationInterface()
{
    current_menu = MainMenu;
    admin = false;
    shutdown = false;

    true_if_ok = false;
    destroy_auxiliary_window = false;
}

ConfigurationInterface::~ConfigurationInterface()
{
}

void ConfigurationInterface::cycle()
{
    if (destroy_auxiliary_window) auxiliary_window = SP<InterfaceElementWindow>();
    destroy_auxiliary_window = false;
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
        window->setListCallback(bind(&ConfigurationInterface::menuSelectFunction, this, _1));
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
    if (admin)
    {
        enterAdminMainMenu();
        return;
    }

    window->deleteAllListElements();
    window->setHint("default");
    window->setTitle("Main menu");

    current_menu = MainMenu;
    int slot_index = window->addListElement("Disconnect", "disconnect", true, false); 
    window->modifyListSelectionIndex(slot_index);
}

void ConfigurationInterface::enterAdminMainMenu()
{
    if (!admin) return;

    window->deleteAllListElements();
    window->setHint("default");
    window->setTitle("Main menu");

    current_menu = AdminMainMenu;

    ui32 slot_index = window->addListElement("Configure slots", "slots", true, false); 
    window->addListElement("Disconnect", "disconnect", true, false); 
    window->addListElement("Shutdown server", "shutdown", true, false);
    window->modifyListSelectionIndex(slot_index);
}

void ConfigurationInterface::enterSlotsMenu()
{
    if (!admin) return;

    window->deleteAllListElements();
    window->setHint("default");
    window->setTitle("Slot configuration");

    current_menu = SlotsMenu;

    int slot_index = window->addListElement("Back to main menu", "mainmenu", true, false);
    window->addListElement("Add a new slot profile", "new_slotprofile", true, false);
    window->modifyListSelectionIndex(slot_index);
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
    window->addListElement("anyone",                      "Allowed watchers:          ", "newslot_watchers", true, false);
    window->addListElement("anyone",                      "Allowed launchers:         ", "newslot_launchers", true, false);
    window->addListElement("anyone",                      "Allowed players:           ", "newslot_players", true, false);
    window->addListElement("nobody",                      "Forbidden watchers:        ", "newslot_watchers_forbidden", true, false);
    window->addListElement("nobody",                      "Forbidden launchers:       ", "newslot_launchers_forbidden", true, false);
    window->addListElement("nobody",                      "Forbidden players:         ", "newslot_players_forbidden", true, false);
    window->addListElement("80",                          "Width:                     ", "newslot_width", true, true);
    window->addListElement("25",                          "Height:                    ", "newslot_height", true, true);
    window->addListElement("1",                           "Maximum slots:             ", "newslot_maxslots", true, true);
    window->addListElement(                               "Create slot profile",         "newslot_create", false, true);

    window->modifyListSelectionIndex(slot_index);
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
    else if (selection == "usergroup_ok")
    {
        true_if_ok = true;
        window->gainFocus();
        auxiliary_window = SP<InterfaceElementWindow>();
    }
    else if (selection == "usergroup_cancel")
    {
        true_if_ok = false;
        window->gainFocus();
        auxiliary_window = SP<InterfaceElementWindow>();
    }

    return false;
}

bool ConfigurationInterface::menuSelectFunction(ui32 index)
{
    if (auxiliary_window) return false;

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
    /* The new slot profile menu */
    else if (selection == "newslot_watchers")
    {
        edit_usergroup = edit_slotprofile.getAllowedWatchers();
        auxiliaryEnterUsergroupWindow();
    }
    return false;
};

void ConfigurationInterface::auxiliaryEnterUsergroupWindow()
{
    destroy_auxiliary_window = false;
    auxiliary_window = interface->createInterfaceElementWindow();
    auxiliary_window->setHint("wide");

    auxiliary_window->setTitle("Select user group");
    int nobody = auxiliary_window->addListElement("Nobody", "usergroup_nobody", true, false);
    int anybody = auxiliary_window->addListElement("Anybody", "usergroup_anybody", true, false);
    int launcher = auxiliary_window->addListElement("Launcher", "usergroup_launcher", true, false);
    int players = auxiliary_window->addListElement("Specific users", "usergroup_players", true, false);
    int ok = auxiliary_window->addListElement("Ok", "usergroup_ok", true, false);
    int cancel = auxiliary_window->addListElement("Cancel", "usergroup_cancel", true, false);

    auxiliary_window->modifyListSelectionIndex(ok);
    auxiliary_window->setListCallback(bind(&ConfigurationInterface::auxiliaryMenuSelectFunction, this, _1));

    checkAuxiliaryWindowUsergroupSelections();

    interface->cycle();
    auxiliary_window->gainFocus();
}

void ConfigurationInterface::checkAuxiliaryWindowUsergroupSelections()
{
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
        if (data == "usergroup_anybody")
        {
            if (edit_usergroup.hasAnybody())
                auxiliary_window->modifyListElementTextUTF8(index, "Anybody*");
            else
                auxiliary_window->modifyListElementTextUTF8(index, "Anybody");
        }
        if (data == "usergroup_launcher")
        {
            if (edit_usergroup.hasLauncher())
                auxiliary_window->modifyListElementTextUTF8(index, "Launcher*");
            else
                auxiliary_window->modifyListElementTextUTF8(index, "Launcher");
        }
        if (data == "usergroup_players")
        {
            if (edit_usergroup.hasAnySpecificUser())
                auxiliary_window->modifyListElementTextUTF8(index, "Specific users*");
            else
                auxiliary_window->modifyListElementTextUTF8(index, "Specific users");
        }
    }
}

bool ConfigurationInterface::shouldShutdown() const
{ return shutdown; };


