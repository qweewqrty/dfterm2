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

    current_menu = MainMenu;
    int slot_index = window->addListElement("Disconnect", "disconnect", true, false); 
    window->modifyListSelection(slot_index);
}

void ConfigurationInterface::enterAdminMainMenu()
{
    if (!admin) return;

    window->deleteAllListElements();

    current_menu = AdminMainMenu;

    int slot_index = window->addListElement("Configure slots", "slots", true, false); 
    window->addListElement("Disconnect", "disconnect", true, false); 
    window->addListElement("Shutdown server", "shutdown", true, false);
    window->modifyListSelection(slot_index);
}

bool ConfigurationInterface::menuSelectFunction(ui32 index)
{
    data1D selection = window->getListElementData(index);
    if (selection == "disconnect")
        if (user)
            user->kill();
    return false;
};


