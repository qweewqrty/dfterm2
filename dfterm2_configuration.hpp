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

namespace dfterm {

/* Hash function (SHA512). This uses OpenSSL as backend. */

/* This one hashes the given data chunk and returns an ASCII data chunk
 * containing the hash in hex. */
data1D hash_data(const data1D &chunk);
/* Turns a byte chunk into an ASCII hex-encoded chunk */
data1D bytes_to_hex(const data1D &bytes);
/* Escapes a string so that it can be used as an SQL string in its statements. */
/* Used internally. Throws null characters away. */
data1D escape_sql_string(const data1D &str);

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
        User(const std::string &us) { name = UnicodeString::fromUTF8(us); };

        data1D getPasswordHash() const { return password_hash_sha512; };
        void setPasswordHash(data1D hash) { password_hash_sha512 = hash; };

        data1D getPasswordSalt() const { return password_salt; };
        void setPasswordSalt(data1D salt) { password_salt = salt; };

        /* This one hashes the password and then calls setPasswordHash. */
        /* UnicodeString will be first converted to UTF-8 */
        /* If you use salt, set it before calling this function. */
        void setPassword(const UnicodeString &password)
        {
            setPassword(TO_UTF8(password));
        }
        void setPassword(const data1D &password)
        {
            setPasswordHash(hash_data(password + password_salt));
        }
        /* Returns true if password is correct. */
        bool verifyPassword(const UnicodeString &password)
        {
            return verifyPassword(TO_UTF8(password));
        }
        bool verifyPassword(const data1D &password)
        {
            return (hash_data(password + password_salt) == password_hash_sha512);
        }

        UnicodeString getName() const { return name; };
        std::string getNameUTF8() const
        {
			return TO_UTF8(name);
        }

        void setName(const UnicodeString &us) { name = us; };
        void setNameUTF8(const std::string &us)
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
        std::set<UnicodeString> has_user;

    public:
        UserGroup()
        {
            has_nobody = true;
            has_anybody = false;
            has_launcher = false;
        }

        data1D serialize() const;
        static UserGroup unSerialize(data1D data);

        bool hasAnySpecificUser() const { return ((!has_user.empty()) || has_anybody); };

        bool hasNobody() const { return has_nobody; };
        bool hasAnybody() const { return has_anybody; };
        bool hasLauncher() const { return has_launcher; };
        bool hasUser(const UnicodeString &username) const 
        { 
            if (has_nobody) return false;
            if (has_anybody) return true;
            return has_user.find(username) != has_user.end(); 
        };
        bool hasUserUTF8(const std::string &username) const
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
        bool toggleUser(const UnicodeString &username)
        {
            std::set<UnicodeString>::iterator i1 = has_user.find(username);
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
        bool toggleUserUTF8(const std::string &username)
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
        void setUser(const UnicodeString &username)
        {
            has_user.insert(username);
        }
        void unsetUser(const UnicodeString &username)
        {
            has_user.erase(username);
        }
};

class SlotProfile
{
    private:
        UnicodeString name;              /* name of the slot profile */
        trankesbel::ui32 w, h;                       /* width and height of the game inside slot */
        UnicodeString path;              /* path to game executable */
        UnicodeString working_path;      /* working directory for the game. */
        SlotType slot_type;              /* slot type */
        UserGroup allowed_watchers;      /* who may watch */
        UserGroup allowed_launchers;     /* who may launch */
        UserGroup allowed_players;       /* who may play */
        UserGroup forbidden_watchers;    /* who may not watch */
        UserGroup forbidden_launchers;   /* who may not launch */
        UserGroup forbidden_players;     /* who may not play */
        trankesbel::ui32 max_slots;                  /* maximum number of slots to create */

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
        void setName(const UnicodeString &name) { this->name = name; };
        void setNameUTF8(const std::string &name) { this->name = UnicodeString::fromUTF8(name); };
        UnicodeString getName() const { return name; };
        std::string getNameUTF8() const { return TO_UTF8(name); };

        void setWidth(trankesbel::ui32 w) { this->w = w; };
        trankesbel::ui32 getWidth() const { return w; };
        void setHeight(trankesbel::ui32 h) { this->h = h; };
        trankesbel::ui32 getHeight() const { return h; };
        void setSize(trankesbel::ui32 w, trankesbel::ui32 h)
        { setWidth(w); setHeight(h); };
        void getSize(trankesbel::ui32* w, trankesbel::ui32* h)
        { (*w) = getWidth(); (*h) = getHeight(); };

        void setExecutable(const UnicodeString &executable) { path = executable; };
        void setExecutableUTF8(const std::string &executable) { path = UnicodeString::fromUTF8(executable); };
        UnicodeString getExecutable() const { return path; };
        std::string getExecutableUTF8() const { return TO_UTF8(path); };

        void setWorkingPath(const UnicodeString &executable_path) { working_path = executable_path; };
        void setWorkingPathUTF8(const std::string &executable_path) { working_path = UnicodeString::fromUTF8(executable_path); };
        UnicodeString getWorkingPath() const { return working_path; };
        std::string getWorkingPathUTF8() const { return TO_UTF8(working_path); };

        void setSlotType(SlotType t) { slot_type = t; };
        SlotType getSlotType() const { return slot_type; };

        void setMaxSlots(trankesbel::ui32 slots) { max_slots = slots; };
        trankesbel::ui32 getMaxSlots() const { return max_slots; };

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
        ConfigurationDatabase& operator=(const ConfigurationDatabase &) { return (*this); };

        sqlite3* db;
        int slotprofileNameListDataCallback(std::vector<UnicodeString>* name_list, void* v_self, int argc, char** argv, char** colname);
        int slotprofileDataCallback(SlotProfile* sp, void* v_self, int argc, char** argv, char** colname);
        int userDataCallback(std::string* name, std::string* password_hash, std::string* password_salt, bool* admin, void* v_self, int argc, char** argv, char** colname);
        int userListDataCallback(std::vector<SP<User> >* user_list, void* v_self, int argc, char** argv, char** colname);

    public:
        ConfigurationDatabase();
        ~ConfigurationDatabase();

        OpenStatus open(const UnicodeString &filename);
        OpenStatus openUTF8(const std::string &filename)
        { 
			UnicodeString us = UnicodeString::fromUTF8(filename);
			OpenStatus os = open(us);
			return os;
		};

        void deleteUserData(const UnicodeString &name);

        std::vector<SP<User> > loadAllUserData();
        SP<User> loadUserData(const UnicodeString &name);
        void saveUserData(User* user);
        void saveUserData(SP<User> user) { saveUserData(user.get()); };

        void saveSlotProfileData(SlotProfile* slotprofile);
        void saveSlotProfileData(SP<SlotProfile> slotprofile) { saveSlotProfileData(slotprofile.get()); };

        std::vector<UnicodeString> loadSlotProfileNames();

        void deleteSlotProfileData(const UnicodeString &name);
        void deleteSlotProfileDataUTF8(const std::string &name) { deleteSlotProfileData(UnicodeString::fromUTF8(name)); };
        SP<SlotProfile> loadSlotProfileData(const UnicodeString &name);
        SP<SlotProfile> loadSlotProfileDataUTF8(const std::string &name) { return loadSlotProfileData(UnicodeString::fromUTF8(name)); };
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
        ConfigurationInterface& operator=(const ConfigurationInterface &ci) { return (*this); };

        SP<trankesbel::Interface> interface;
        SP<trankesbel::InterfaceElementWindow> window;
        SP<trankesbel::InterfaceElementWindow> auxiliary_window;

        SP<ConfigurationDatabase> configuration_database;

        Menu current_menu;
        void enterMainMenu();
        void enterAdminMainMenu();
        void enterSlotsMenu();
        void enterNewSlotProfileMenu();
        void enterEditSlotProfileMenu();
        void enterLaunchSlotsMenu();
        void enterJoinSlotsMenu();
        void checkSlotProfileMenu(bool no_read = false);

        void auxiliaryEnterUsergroupWindow();
        void auxiliaryEnterSpecificUsersWindow();
        void checkAuxiliaryWindowUsergroupSelections();

        /* When defining user groups, this is the currently edited user group. */
        UserGroup edit_usergroup;
        /* And this is currently edited slot profile */
        SlotProfile edit_slotprofile;
        /* This is there slot profile should be saved after done with it.
         * Used when editing an existing slot profile. */
        SP<SlotProfile> edit_slotprofile_sp_target;

        /* And this is where the currently edited user group should be copied
         * when done with it. ("watchers", "launchers", "etc.") */
        std::string edit_slotprofile_target;

        /* When in a menu that has "ok" and "cancel", this is set to what was selected. */
        bool true_if_ok;

        bool admin;
        SP<User> user;
        WP<Client> client;

        bool menuSelectFunction(trankesbel::ui32 index);
        bool auxiliaryMenuSelectFunction(trankesbel::ui32 index);

        bool shutdown;

        WP<State> state;

    public:
        ConfigurationInterface();
        ConfigurationInterface(SP<trankesbel::Interface> interface);
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
        void setInterface(SP<trankesbel::Interface> interface);

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
        SP<trankesbel::InterfaceElementWindow> getUserWindow();
};


};

#endif

