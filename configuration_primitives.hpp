#ifndef configuration_primitives_hpp
#define configuration_primitives_hpp

namespace dfterm {
class SlotProfile;
class User;
class UserGroup;
};

#include "types.hpp"
#include <unicode/unistr.h>
#include "id.hpp"
#include "hash.hpp"
#include <set>

namespace dfterm
{

class User
{
    private:
        UnicodeString name;
        data1D password_hash_sha512;
        data1D password_salt;
        bool active;
        bool admin;
        ID id;

    public:
        User() { active = true; admin = false; };
        User(UnicodeString us) { name = us; active = true; admin = false; };
        User(const std::string &us) { name = UnicodeString::fromUTF8(us); };

        data1D getPasswordHash() const { return password_hash_sha512; };
        void setPasswordHash(data1D hash) { password_hash_sha512 = hash; };

        data1D getPasswordSalt() const { return password_salt; };
        void setPasswordSalt(data1D salt) { password_salt = salt; };
        
        ID getID() const { return id; };
        const ID& getIDRef() const { return id; };
        void setID(const ID& id) { this->id = id; };

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
        std::set<ID> has_user;

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
        bool hasUser(const ID &user_id) const 
        { 
            if (has_nobody) return false;
            if (has_anybody) return true;
            return has_user.find(user_id) != has_user.end(); 
        };

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
        bool toggleUser(const ID &user_id)
        {
            std::set<ID>::iterator i1 = has_user.find(user_id);
            if (i1 != has_user.end())
            {
                has_user.erase(i1);
                if (has_user.empty() && !has_anybody && !has_nobody) setNobody();
                return false;
            }
            else
            {
                has_user.insert(user_id);
                if (has_nobody) has_nobody = false;
                return true;
            }
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
        void setUser(const ID &user_id)
        {
            has_user.insert(user_id);
        }
        void unsetUser(const ID &user_id)
        {
            has_user.erase(user_id);
        }
};

class SlotProfile
{
    private:
        UnicodeString name;              /* name of the slot profile */
        trankesbel::ui32 w, h;           /* width and height of the game inside slot */
        UnicodeString path;              /* path to game executable */
        UnicodeString working_path;      /* working directory for the game. */
        trankesbel::ui32 slot_type;      /* slot type (should be type SlotType, is ui32 for header dependency reasons) */
        UserGroup allowed_watchers;      /* who may watch */
        UserGroup allowed_launchers;     /* who may launch */
        UserGroup allowed_players;       /* who may play */
        UserGroup allowed_closers;       /* who may force close the game */
        UserGroup forbidden_watchers;    /* who may not watch */
        UserGroup forbidden_launchers;   /* who may not launch */
        UserGroup forbidden_players;     /* who may not play */
        UserGroup forbidden_closers;     /* who may not force close the game */
        trankesbel::ui32 max_slots;      /* maximum number of slots to create */
        
        ID id;                           /* Identify the slot profile with this. */

    public:
        SlotProfile()
        {
            w = 80; h = 25;
            slot_type = 0;
            max_slots = 1;
            allowed_watchers.setAnybody();
            allowed_launchers.setAnybody();
            allowed_players.setAnybody();
            allowed_closers.setAnybody();
            forbidden_players.setNobody();
            forbidden_launchers.setNobody();
            forbidden_watchers.setNobody();
            forbidden_closers.setNobody();
        }

        /* All these functions are just simple getter/setter pairs */
        void setName(const UnicodeString &name) { this->name = name; };
        void setNameUTF8(const std::string &name) { this->name = UnicodeString::fromUTF8(name); };
        UnicodeString getName() const { return name; };
        std::string getNameUTF8() const { return TO_UTF8(name); };
        
        ID getID() const { return id; };
        const ID& getIDRef() const { return id; };
        void setID(const ID &id) { this->id = id; };

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

        /* Use like this: setSlotType((ui32) t); where t is of SlotType enum. */
        void setSlotType(trankesbel::ui32 t) { slot_type = t; };
        trankesbel::ui32 getSlotType() const { return slot_type; };

        void setMaxSlots(trankesbel::ui32 slots) { max_slots = slots; };
        trankesbel::ui32 getMaxSlots() const { return max_slots; };

        UserGroup getAllowedWatchers() const { return allowed_watchers; };
        UserGroup getAllowedLaunchers() const { return allowed_launchers; };
        UserGroup getAllowedPlayers() const { return allowed_players; };
        UserGroup getAllowedClosers() const { return allowed_closers; };
        UserGroup getForbiddenWatchers() const { return forbidden_watchers; };
        UserGroup getForbiddenLaunchers() const { return forbidden_launchers; };
        UserGroup getForbiddenPlayers() const { return forbidden_players; };
        UserGroup getForbiddenClosers() const { return forbidden_closers; };
        
        void setAllowedWatchers(UserGroup gr) { allowed_watchers = gr; };
        void setAllowedLaunchers(UserGroup gr) { allowed_launchers = gr; };
        void setAllowedPlayers(UserGroup gr) { allowed_players = gr; };
        void setAllowedClosers(UserGroup gr) { allowed_closers = gr; };
        void setForbiddenWatchers(UserGroup gr) { forbidden_watchers = gr; };
        void setForbiddenLaunchers(UserGroup gr) { forbidden_launchers = gr; };
        void setForbiddenPlayers(UserGroup gr) { forbidden_players = gr; };
        void setForbiddenClosers(UserGroup gr) { forbidden_closers = gr; };
};
    
};

#endif
