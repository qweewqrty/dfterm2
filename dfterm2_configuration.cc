#include "dfterm2_configuration.hpp"

using namespace dfterm;

ConfigurationInterface::ConfigurationInterface()
{
    current_menu = MainMenu;
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
        enterMainMenu();
    }
    if (!window) return SP<InterfaceElementWindow>();

    return window;
}

void ConfigurationInterface::enterMainMenu()
{
    window->deleteAllListElements();
    current_menu = MainMenu;

    int slot_index = window->addListElement("Configure slots", "slots", true, false); 
    window->addListElement("Shutdown server", "shutdown", true, false);
    window->modifyListSelection(slot_index);
}

