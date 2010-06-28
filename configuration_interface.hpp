/* Lump all configuration handling (reading from sqlite database)
 * and window handling to this file. */

#ifndef dfterm2_configuration_hpp
#define dfterm2_configuration_hpp

namespace dfterm {
class ConfigurationDatabase;
class ConfigurationInterface;
};

#include "interface.hpp"
#include "slot.hpp"
#include <set>
#include <vector>
#include "state.hpp"
#include "hash.hpp"
#include "configuration_db.hpp"
#include "configuration_primitives.hpp"

namespace dfterm {

enum Menu { /* These are menus for all of us! */
            MainMenu,
            LaunchSlotsMenu,
            JoinSlotsMenu,

            /* And the following are admin-only menus. */
            AdminMainMenu, 
            MotdMenu,
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
        void enterMotdMenu();
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

