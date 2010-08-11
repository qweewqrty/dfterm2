#ifndef configuration_db_hpp
#define configuration_db_hpp

#include <cstdlib>
#include <cstdio>
#include "types.hpp"
#include "sqlite3.h"
#include "configuration_primitives.hpp"
#include "logger.hpp"
#include "sockets.hpp"

namespace dfterm
{
        
/* Escapes a string so that it can be used as an SQL string in its statements. */
/* Used internally. Throws null characters away. */
data1D escape_sql_string(const data1D &str);
    
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
        int motdCallback(UnicodeString* us, void* v_self, int argc, char** argv, char** colname);
        int userDataCallback(SP<User>* user, void* v_self, int argc, char** argv, char** colname);
        int userListDataCallback(std::vector<SP<User> >* user_list, void* v_self, int argc, char** argv, char** colname);
        int maximumSlotsCallback(trankesbel::ui32* maximum, void* v_self, int argc, char** argv, char** colname);
        int allowedAndForbiddenSocketAddressRangesCallback(trankesbel::SocketAddressRange* allowed, trankesbel::SocketAddressRange* forbidden, void* v_self, int argc, char** argv, char** colname);

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
        SP<User> loadUserData(const ID& id);
        void saveUserData(User* user);
        void saveUserData(SP<User> user) { saveUserData(user.get()); };

        void saveSlotProfileData(SlotProfile* slotprofile);
        void saveSlotProfileData(SP<SlotProfile> slotprofile) { saveSlotProfileData(slotprofile.get()); };

        std::vector<UnicodeString> loadSlotProfileNames();

        void deleteSlotProfileData(const UnicodeString &name);
        void deleteSlotProfileDataUTF8(const std::string &name) { deleteSlotProfileData(UnicodeString::fromUTF8(name)); };
        SP<SlotProfile> loadSlotProfileData(const UnicodeString &name);
        SP<SlotProfile> loadSlotProfileDataUTF8(const std::string &name) { return loadSlotProfileData(UnicodeString::fromUTF8(name)); };
        
        void saveMOTD(UnicodeString motd);
        void saveMOTDUTF8(std::string motd_utf8);
        UnicodeString loadMOTD();
        std::string loadMOTDUTF8();

        void saveAllowedAndForbiddenSocketAddressRanges(const trankesbel::SocketAddressRange &allowed, const trankesbel::SocketAddressRange &forbidden);
        void loadAllowedAndForbiddenSocketAddressRanges(trankesbel::SocketAddressRange* allowed, trankesbel::SocketAddressRange* forbidden);

        trankesbel::ui32 loadMaximumNumberOfSlots();
        void saveMaximumNumberOfSlots(trankesbel::ui32 maximum);
};

}

#endif
