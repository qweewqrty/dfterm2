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
        bool active;
        bool admin;

    public:
        User() { active = true; admin = false; };
        User(UnicodeString us) { name = us; active = true; admin = false; };
        User(string us) { name = UnicodeString::fromUTF8(us); };

        UnicodeString getName() const { return name; };
        void setName(UnicodeString us) { name = us; };

        bool isActive() const { return active; };
        void kill() { active = false; };

        bool isAdmin() const { return admin; };
        void setAdmin(bool admin_status) { admin = admin_status; };
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

enum Menu { MainMenu, AdminMainMenu };

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
        void enterAdminMainMenu();

        bool admin;
        SP<User> user;

        bool menuSelectFunction(ui32 index);

    public:
        ConfigurationInterface();
        ConfigurationInterface(SP<Interface> interface);
        ~ConfigurationInterface();

        /* Set interface to use from this */
        void setInterface(SP<Interface> interface);

        /* Sets the user for this interface. Calling this will also
         * call enableAdmin/disableAdmin according to user admin status. */
        void setUser(SP<User> user);
        /* And gets the user. */
        SP<User> getUser();

        /* Allow admin stuff to be controlled from this interface. 
         * Better do some authentication on the user before calling this... 
         * Also causes current user to drop to the main menu. */
        void enableAdmin();
        /* Revoke admin stuff. Drops the user to the main menu. */
        void disableAdmin();

        /* Generic user window, configuration and stuff is handled through this. */
        SP<InterfaceElementWindow> getUserWindow();
};

class SlotProfile
{
    public:
};

};

#endif

