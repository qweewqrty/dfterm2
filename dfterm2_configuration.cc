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
}

ConfigurationInterface::~ConfigurationInterface()
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

    int slot_index = window->addListElement("Configure slots", "slots", true, false); 
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
    window->addListElement("80",                          "Width:                     ", "newslot_width", true, false);
    window->addListElement("25",                          "Height:                    ", "newslot_height", true, false);
    window->addListElement("5",                           "Maximum slots:             ", "newslot_maxslots", true, true);
    window->addListElement(                               "Create slot profile", "newslot_create", false, true);

    window->modifyListSelectionIndex(slot_index);
}

bool ConfigurationInterface::menuSelectFunction(ui32 index)
{
    data1D selection = window->getListElementData(index);
    if (selection == "mainmenu")
    {
        enterMainMenu();
    }
    else if (selection == "new_slotprofile")
    {
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
    return false;
};

bool ConfigurationInterface::shouldShutdown() const
{ return shutdown; };


