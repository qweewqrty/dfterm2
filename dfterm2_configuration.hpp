/* Lump all configuration handling (reading from sqlite database)
 * and window handling to this file. */

#ifndef dfterm2_configuration_hpp
#define dfterm2_configuration_hpp

#include "interface.hpp"

using namespace trankesbel;

namespace dfterm {

class User
{
    private:
        UnicodeString name;

    public:
        User();
        User(UnicodeString us) { name = us; };
        User(string us) { name = UnicodeString::fromUTF8(us); };

        UnicodeString getName() const { return name; };
};


/* A class that handles access to the actual database file on disk. */
class ConfigurationDatabase
{
    private:
        ConfigurationDatabase(const ConfigurationDatabase &) { };
        ConfigurationDatabase& operator=(const ConfigurationDatabase &) { };

    public:
        ~ConfigurationDatabase();

        bool open(UnicodeString filename);

        void setString(UnicodeString key, UnicodeString us);
        UnicodeString getString(UnicodeString key);
};

enum Menu { MainMenu };

/* Handles windows for easy online editing of configuration */
class ConfigurationInterface
{
    private:
        /* No copies */
        ConfigurationInterface(const ConfigurationInterface &ci) { };
        ConfigurationInterface& operator=(const ConfigurationInterface &ci) { };

        SP<Interface> interface;
        SP<InterfaceElementWindow> window;

        Menu current_menu;
        void enterMainMenu();

    public:
        ConfigurationInterface();
        ConfigurationInterface(SP<Interface> interface);
        ~ConfigurationInterface();

        /* Set interface to use from this */
        void setInterface(SP<Interface> interface);

        /* Generic user window, configuration and stuff is handled through this. */
        SP<InterfaceElementWindow> getUserWindow();
};

class SlotProfile
{
    public:
};

};

#endif

