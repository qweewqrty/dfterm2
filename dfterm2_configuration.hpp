/* Lump all configuration handling (reading from sqlite database)
 * and window handling to this file. */

#ifndef dfterm2_configuration_hpp
#define dfterm2_configuration_hpp

namespace dfterm {
class ConfigurationDatabase;
class SlotProfile;
class ConfigurationInterface;
class User;
};

#include "interface.hpp"
#include "sqlite3.h"
#include "slot.hpp"
#include <set>
#include <vector>
#include "state.hpp"

using namespace trankesbel;
using namespace std;

namespace dfterm {

/* Hash function (SHA512). This uses OpenSSL as backend. */

/* This one hashes the given data chunk and returns an ASCII data chunk
 * containing the hash in hex. */
data1D hash_data(data1D chunk);
/* Turns a byte chunk into an ASCII hex-encoded chunk */
data1D bytes_to_hex(data1D bytes);
/* Escapes a string so that it can be used as an SQL string in its statements. */
/* Used internally. Throws null characters away. */
data1D escape_sql_string(data1D str);

class UserGroup;
class User;

class User
{
    private:
        UnicodeString name;
        data1D password_hash_sha512;
        data1D password_salt;
        bool active;
        bool admin;

    public:
        User() { active = true; admin = false; };
        User(UnicodeString us) { name = us; active = true; admin = false; };
        User(string us) { name = UnicodeString::fromUTF8(us); };

        data1D getPasswordHash() const { return password_hash_sha512; };
        void setPasswordHash(data1D hash) { password_hash_sha512 = hash; };

        data1D getPasswordSalt() const { return password_salt; };
        void setPasswordSalt(data1D salt) { password_salt = salt; };

        /* This one hashes the password and then calls setPasswordHash. */
        /* UnicodeString will be first converted to UTF-8 */
        /* If you use salt, set it before calling this function. */
        void setPassword(UnicodeString password)
        {
            data1D password_utf8;
            password.toUTF8String(password_utf8);
            setPassword(password_utf8);
        }
        void setPassword(data1D password)
        {
            setPasswordHash(hash_data(password + password_salt));
        }
        /* Returns true if password is correct. */
        bool verifyPassword(UnicodeString password)
        {
            data1D password_utf8;
            password.toUTF8String(password_utf8);
            return verifyPassword(password_utf8);
        }
        bool verifyPassword(data1D password)
        {
            return (hash_data(password + password_salt) == password_hash_sha512);
        }

        UnicodeString getName() const { return name; };
        string getNameUTF8() const
        {
            string r;
            name.toUTF8String(r);
            return r;
        }

        void setName(UnicodeString us) { name = us; };
        void setNameUTF8(string us)
        {
            setName(UnicodeString::fromUTF8(us));
        }

        bool isActive() const { return active; };
        void kill() { active = false; };

        bool isAdmin() const { return admin; };
        void setAdmin(bool admin_status) { admin = admin_status; };
};

class UserGroup
{
    private:
        bool has_nobody;
        bool has_anybody;
        bool has_launcher;
        set<UnicodeString> has_user;

    public:
        UserGroup()
        {
            has_nobody = true;
            has_anybody = false;
            has_launcher = false;
        }

        bool hasAnySpecificUser() const { return ((!has_user.empty()) || has_anybody); };

        bool hasNobody() const { return has_nobody; };
        bool hasAnybody() const { return has_anybody; };
        bool hasLauncher() const { return has_launcher; };
        bool hasUser(UnicodeString username) const 
        { 
            if (has_nobody) return false;
            if (has_anybody) return true;
            return has_user.find(username) != has_user.end(); 
        };
        bool hasUserUTF8(string username) const
        {
            return hasUser(UnicodeString::fromUTF8(username));
        }

        bool toggleAnybody()
        {
            if (!has_anybody)
                setAnybody();
            else
                setNobody();
            return hasAnybody();
        }
        bool toggleLauncher()
        {
            if (has_launcher) unsetLauncher();
            else setLauncher();
            return hasLauncher();
        }
        bool toggleUser(UnicodeString username)
        {
            set<UnicodeString>::iterator i1 = has_user.find(username);
            if (i1 != has_user.end())
            {
                has_user.erase(i1);
                if (has_user.empty() && !has_anybody && !has_nobody) setNobody();
                return false;
            }
            else
            {
                has_user.insert(username);
                if (has_nobody) has_nobody = false;
                return true;
            }
        }
        bool toggleUserUTF8(string username)
        {
            return toggleUser(UnicodeString::fromUTF8(username));
        }

        void setNobody()
        {
            has_nobody = true;
            has_anybody = false;
            has_launcher = false;
            has_user.clear();
        }
        void setAnybody()
        {
            has_nobody = false;
            has_anybody = true;
            has_launcher = true;
            has_user.clear();
        }
        void setLauncher()
        {
            has_launcher = true;
            has_nobody = false;
        }
        void unsetLauncher()
        {
            if (has_anybody)
                has_launcher = true;
            else
            {
                has_launcher = false;
                if (has_user.empty())
                    has_nobody = true;
            }
        }
        void setUser(UnicodeString username)
        {
            has_user.insert(username);
        }
        void unsetUser(UnicodeString username)
        {
            has_user.erase(username);
        }
};

class SlotProfile
{
    private:
        UnicodeString name;              /* name of the slot profile */
        ui32 w, h;                       /* width and height of the game inside slot */
        UnicodeString path;              /* path to game executable */
        UnicodeString working_path;      /* working directory for the game. */
        SlotType slot_type;              /* slot type */
        UserGroup allowed_watchers;      /* who may watch */
        UserGroup allowed_launchers;     /* who may launch */
        UserGroup allowed_players;       /* who may play */
        UserGroup forbidden_watchers;    /* who may not watch */
        UserGroup forbidden_launchers;   /* who may not launch */
        UserGroup forbidden_players;     /* who may not play */
        ui32 max_slots;                  /* maximum number of slots to create */

    public:
        SlotProfile()
        {
            w = 80; h = 25;
            #ifdef __WIN32
            slot_type = DFLaunch;
            #else
            slot_type = TerminalLaunch;
            #endif
            max_slots = 1;
            allowed_watchers.setAnybody();
            allowed_launchers.setAnybody();
            allowed_players.setAnybody();
            forbidden_players.setNobody();
            forbidden_launchers.setNobody();
            forbidden_watchers.setNobody();
        }

        /* All these functions are just simple getter/setter pairs */
        void setName(UnicodeString name) { this->name = name; };
        void setNameUTF8(string name) { this->name = UnicodeString::fromUTF8(name); };
        UnicodeString getName() const { return name; };
        string getNameUTF8() const { string r; name.toUTF8String(r); return r; };

        void setWidth(ui32 w) { this->w = w; };
        ui32 getWidth() const { return w; };
        void setHeight(ui32 h) { this->h = h; };
        ui32 getHeight() const { return h; };
        void setSize(ui32 w, ui32 h)
        { setWidth(w); setHeight(h); };
        void getSize(ui32* w, ui32* h)
        { (*w) = getWidth(); (*h) = getHeight(); };

        void setExecutable(UnicodeString executable) { path = executable; };
        void setExecutableUTF8(string executable) { path = UnicodeString::fromUTF8(executable); };
        UnicodeString getExecutable() const { return path; };
        string getExecutableUTF8() const { string r; path.toUTF8String(r); return r; };

        void setWorkingPath(UnicodeString executable_path) { working_path = executable_path; };
        void setWorkingPathUTF8(string executable_path) { working_path = UnicodeString::fromUTF8(executable_path); };
        UnicodeString getWorkingPath() const { return working_path; };
        string getWorkingPathUTF8() const { string r; working_path.toUTF8String(r); return r; };

        void setSlotType(SlotType t) { slot_type = t; };
        SlotType getSlotType() const { return slot_type; };

        void setMaxSlots(ui32 slots) { max_slots = slots; };
        ui32 getMaxSlots() const { return max_slots; };

        UserGroup getAllowedWatchers() const { return allowed_watchers; };
        UserGroup getAllowedLaunchers() const { return allowed_launchers; };
        UserGroup getAllowedPlayers() const { return allowed_players; };
        UserGroup getForbiddenWatchers() const { return forbidden_watchers; };
        UserGroup getForbiddenLaunchers() const { return forbidden_launchers; };
        UserGroup getForbiddenPlayers() const { return forbidden_players; };
        
        void setAllowedWatchers(UserGroup gr) { allowed_watchers = gr; };
        void setAllowedLaunchers(UserGroup gr) { allowed_launchers = gr; };
        void setAllowedPlayers(UserGroup gr) { allowed_players = gr; };
        void setForbiddenWatchers(UserGroup gr) { forbidden_watchers = gr; };
        void setForbiddenLaunchers(UserGroup gr) { forbidden_launchers = gr; };
        void setForbiddenPlayers(UserGroup gr) { forbidden_players = gr; };
};

enum OpenStatus { Failure, Ok, OkCreatedNewDatabase };

/* A class that handles access to the actual database file on disk. 
 * The class does some input sanitizing so you can't do SQL injection with the methods. */
class ConfigurationDatabase
{
    private:
        ConfigurationDatabase(const ConfigurationDatabase &) { };
        ConfigurationDatabase& operator=(const ConfigurationDatabase &) { };

        sqlite3* db;
        int userDataCallback(string* name, string* password_hash, string* password_salt, bool* admin, void* v_self, int argc, char** argv, char** colname);
        int userListDataCallback(vector<SP<User> >* user_list, void* v_self, int argc, char** argv, char** colname);

    public:
        ConfigurationDatabase();
        ~ConfigurationDatabase();

        OpenStatus open(UnicodeString filename);
        OpenStatus openUTF8(string filename)
        { return open(UnicodeString::fromUTF8(filename)); };

        void deleteUserData(UnicodeString name);

        vector<SP<User> > loadAllUserData();
        SP<User> loadUserData(UnicodeString name);
        void saveUserData(User* user);
        void saveUserData(SP<User> user) { saveUserData(user.get()); };

        /*void saveSlotProfileData(SlotProfile* slotprofile);
        void saveSlotProfileData(SP<SlotProfile> slotprofile) { saveSlotProfileData(slotprofile.get()); };

        SP<SlotProfile> loadSlotProfileData(*/
};

enum Menu { /* These are menus for all of us! */
            MainMenu,
            LaunchSlotsMenu,
            JoinSlotsMenu,

            /* And the following are admin-only menus. */
            AdminMainMenu, 
            SlotsMenu };

/* Handles windows for easy online editing of configuration */
class ConfigurationInterface
{
    private:
        /* No copies */
        ConfigurationInterface(const ConfigurationInterface &ci) { };
        ConfigurationInterface& operator=(const ConfigurationInterface &ci) { };

        SP<Interface> interface;
        SP<InterfaceElementWindow> window;
        SP<InterfaceElementWindow> auxiliary_window;

        SP<ConfigurationDatabase> configuration_database;

        Menu current_menu;
        void enterMainMenu();
        void enterAdminMainMenu();
        void enterSlotsMenu();
        void enterNewSlotProfileMenu();
        void enterLaunchSlotsMenu();
        void enterJoinSlotsMenu();
        void checkSlotProfileMenu();

        void auxiliaryEnterUsergroupWindow();
        void auxiliaryEnterSpecificUsersWindow();
        void checkAuxiliaryWindowUsergroupSelections();

        /* When defining user groups, this is the currently edited user group. */
        UserGroup edit_usergroup;
        /* And this is currently edited slot profile */
        SlotProfile edit_slotprofile;
        /* And this is where the currently edited slot profile should be copied
         * when done with it. ("watchers", "launchers", "etc.") */
        string edit_slotprofile_target;

        /* When in a menu that has "ok" and "cancel", this is set to what was selected. */
        bool true_if_ok;

        bool admin;
        SP<User> user;
        WP<Client> client;

        bool menuSelectFunction(ui32 index);
        bool auxiliaryMenuSelectFunction(ui32 index);

        bool shutdown;

        WP<State> state;

    public:
        ConfigurationInterface();
        ConfigurationInterface(SP<Interface> interface);
        ~ConfigurationInterface();

        /* Set/get the configuration database */
        void setConfigurationDatabase(SP<ConfigurationDatabase> c_database);
        SP<ConfigurationDatabase> getConfigurationDatabase();

        /* Set/get dfterm2 state class */
        void setState(WP<State> state) { this->state = state; };
        void setState(SP<State> state) { setState(WP<State>(state)); };
        WP<State> getState() { return state; };

        /* Some menus change over time, without interaction.
         * This function has to be called to fix'em. */
        void cycle();

        /* Set interface to use from this */
        void setInterface(SP<Interface> interface);

        /* Sets the user for this interface. Calling this will also
         * call enableAdmin/disableAdmin according to user admin status. */
        void setUser(SP<User> user);
        /* And gets the user. */
        SP<User> getUser();

        /* Sets the client for this interface. */
        void setClient(WP<Client> client);
        /* And gets it */
        WP<Client> getClient();

        /* Returns true if shutdown of the entire server has been requested. */
        bool shouldShutdown() const;

        /* Allow admin stuff to be controlled from this interface. 
         * Better do some authentication on the user before calling this... 
         * Also causes current user to drop to the main menu. */
        void enableAdmin();
        /* Revoke admin stuff. Drops the user to the main menu. */
        void disableAdmin();

        /* Generic user window, configuration and stuff is handled through this. */
        SP<InterfaceElementWindow> getUserWindow();
};


};

#endif

