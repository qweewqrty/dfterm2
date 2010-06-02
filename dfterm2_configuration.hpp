/* Lump all configuration handling (reading from sqlite database)
 * and window handling to this file. */

#ifndef dfterm2_configuration_hpp
#define dfterm2_configuration_hpp

#include "interface.hpp"
#include "sqlite3.h"

using namespace trankesbel;

namespace dfterm {

class User
{
    private:
        UnicodeString name;
        data1D password_hash_sha512;
        bool active;
        bool admin;

    public:
        User() { active = true; admin = false; };
        User(UnicodeString us) { name = us; active = true; admin = false; };
        User(string us) { name = UnicodeString::fromUTF8(us); };

        data1D getPasswordHash() const { return password_hash_sha512; };
        void setPasswordHash(data1D hash) { password_hash_sha512 = hash; };

        UnicodeString getName() const { return name; };
        void setName(UnicodeString us) { name = us; };

        bool isActive() const { return active; };
        void kill() { active = false; };

        bool isAdmin() const { return admin; };
        void setAdmin(bool admin_status) { admin = admin_status; };
};


/* A class that handles access to the actual database file on disk. 
 * Note that this class does not do input sanitization. Beware of SQL injection! */
class ConfigurationDatabase
{
    private:
        ConfigurationDatabase(const ConfigurationDatabase &) { };
        ConfigurationDatabase& operator=(const ConfigurationDatabase &) { };

        sqlite3* db;
        int userDataCallback(string* name, string* password_hash, void* v_self, int argc, char** argv, char** colname);

    public:
        ConfigurationDatabase();
        ~ConfigurationDatabase();

        bool open(UnicodeString filename);
        bool openUTF8(string filename)
        { return open(UnicodeString::fromUTF8(filename)); };

        SP<User> loadUserData(UnicodeString name);
        void saveUserData(User* user);
        void saveUserData(SP<User> user) { saveUserData(user.get()); };
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

