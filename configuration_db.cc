#include "configuration_db.hpp"
#include <string>
#include <cstdio>
#include <iostream>
#include <boost/bind.hpp>
#include <openssl/sha.h>
#include "hash.hpp"
#include "sqlite3.h"
#include <vector>
#include "configuration_primitives.hpp"
#include <boost/function.hpp>
#include "slot.hpp"
#include "sockets.hpp"

using namespace dfterm;
using namespace std;
using namespace boost;
using namespace trankesbel;

ConfigurationDatabase::ConfigurationDatabase()
{
    db = (sqlite3*) 0;
}

ConfigurationDatabase::~ConfigurationDatabase()
{
    if (db)
        sqlite3_close(db);
    db = (sqlite3*) 0;
}

OpenStatus ConfigurationDatabase::open(const UnicodeString &filename)
{
    if (db) sqlite3_close(db);
    db = (sqlite3*) 0;

    string filename_utf8 = TO_UTF8(filename);

    /* Test if the database exists on the disk. */
    bool database_exists = true;
    int result = sqlite3_open_v2(filename_utf8.c_str(), &db, SQLITE_OPEN_READONLY, (const char*) 0);
    if (result)
        database_exists = false;
    else
        sqlite3_close(db);
    db = (sqlite3*) 0;

    result = sqlite3_open(filename_utf8.c_str(), &db);
    if (result)
        return Failure;

    /* Create tables. These calls fail if they already exist (which is not bad). */
    result = sqlite3_exec(db, "CREATE TABLE Users(Name TEXT, ID TEXT, PasswordSHA512 TEXT, PasswordSalt TEXT, Admin TEXT);", 0, 0, 0);
    result = sqlite3_exec(db, "CREATE TABLE Slotprofiles(Name TEXT, ID TEXT, Width TEXT, Height TEXT, Path TEXT, WorkingPath TEXT, SlotType TEXT, AllowedWatchers TEXT, AllowedLaunchers TEXT, AllowedPlayers TEXT, AllowedClosers TEXT, ForbiddenWatchers TEXT, ForbiddenLaunchers TEXT, ForbiddenPlayers TEXT, ForbiddenClosers TEXT, MaxSlots TEXT);", 0, 0, 0);
    result = sqlite3_exec(db, "CREATE TABLE MOTD(Content TEXT);", 0, 0, 0);
    result = sqlite3_exec(db, "CREATE TABLE GlobalSettings(Key TEXT, Value TEXT);", 0, 0, 0);
    result = sqlite3_exec(db, "CREATE TABLE AllowedAndForbiddenSocketAddressRanges(Allowed TEXT, Forbidden TEXT, DefaultAllowance TEXT);", 0, 0, 0);

    /* Create an admin user, if database was created. */
    if (!database_exists)
        return OkCreatedNewDatabase;

    return Ok;
}

int ConfigurationDatabase::maximumSlotsCallback(ui32* maximum, void* v_self, int argc, char** argv, char** colname)
{
    assert(maximum && v_self);

    ui32 value = 0xffffffff;
    bool correct_key = false;

    int i;
    for (i = 0; i < argc; ++i)
    {
        if (!argv[i]) continue;
        
        if (!strcmp(colname[i], "Value"))
        {
            value = strtol(argv[i], 0, 10);
            break;
        }
        if (!strcmp(colname[i], "Key") && !strcmp(argv[i], "MaximumSlots"))
            correct_key = true;
    }

    if (correct_key)
        (*maximum) = value;

    return 0;
}

int ConfigurationDatabase::slotprofileNameListDataCallback(vector<UnicodeString>* name_list, void* v_self, int argc, char** argv, char** colname)
{
    assert(name_list && v_self);

    int i;
    for (i = 0; i < argc; ++i)
    {
        if (!argv[i]) continue;
        
        if (!strcmp(colname[i], "Name"))
        {
            string name_utf8 = argv[i];
            (*name_list).push_back(UnicodeString::fromUTF8(name_utf8));
        }
    }

    return 0;
}

int ConfigurationDatabase::slotprofileDataCallback(SlotProfile* sp, void* v_self, int argc, char** argv, char** colname)
{
    assert(sp && v_self);

    int i;
    for (i = 0; i < argc; ++i)
    {
        if (!argv[i]) continue;

        if (!strcmp(colname[i], "Name"))
            sp->setNameUTF8(argv[i]);
        else if (!strcmp(colname[i], "ID"))
            sp->setID(ID::getUnSerialized(string(argv[i])));
        else if (!strcmp(colname[i], "Width"))
            sp->setWidth(strtol(argv[i], NULL, 10));
        else if (!strcmp(colname[i], "Height"))
            sp->setHeight(strtol(argv[i], NULL, 10));
        else if (!strcmp(colname[i], "Path"))
            sp->setExecutable(argv[i]);
        else if (!strcmp(colname[i], "WorkingPath"))
            sp->setWorkingPath(argv[i]);
        else if (!strcmp(colname[i], "SlotType"))
        {
            SlotType st = InvalidSlotType;
            for (st = (SlotType) 0; st != InvalidSlotType; st = (SlotType) ((size_t) st + 1))
                if (!strcmp(argv[i], SlotNames[(size_t) st].c_str()))
                    break;
            sp->setSlotType(st);
        }
        else if (!strcmp(colname[i], "AllowedWatchers"))
            sp->setAllowedWatchers(UserGroup::unSerialize(argv[i]));
        else if (!strcmp(colname[i], "AllowedLaunchers"))
            sp->setAllowedLaunchers(UserGroup::unSerialize(argv[i]));
        else if (!strcmp(colname[i], "AllowedPlayers"))
            sp->setAllowedPlayers(UserGroup::unSerialize(argv[i]));
        else if (!strcmp(colname[i], "AllowedClosers"))
            sp->setAllowedClosers(UserGroup::unSerialize(argv[i]));
        else if (!strcmp(colname[i], "ForbiddenWatchers"))
            sp->setForbiddenWatchers(UserGroup::unSerialize(argv[i]));
        else if (!strcmp(colname[i], "ForbiddenLaunchers"))
            sp->setForbiddenLaunchers(UserGroup::unSerialize(argv[i]));
        else if (!strcmp(colname[i], "ForbiddenPlayers"))
            sp->setForbiddenPlayers(UserGroup::unSerialize(argv[i]));
        else if (!strcmp(colname[i], "ForbiddenClosers"))
            sp->setForbiddenClosers(UserGroup::unSerialize(argv[i]));
        else if (!strcmp(colname[i], "MaxSlots"))
            sp->setMaxSlots(strtol(argv[i], NULL, 10));
    }

    return 0;
};

int ConfigurationDatabase::userDataCallback(SP<User>* user, void* v_self, int argc, char** argv, char** colname)
{   
    assert(user && (*user));

    string password_salt, password_hash;
    int i;
    for (i = 0; i < argc; ++i)
    {
        if (!argv[i]) continue;

        if (!strcmp(colname[i], "Name"))
            (*user)->setNameUTF8(string(argv[i]));
        else if (!strcmp(colname[i], "PasswordSHA512"))
            password_hash = argv[i];
        else if (!strcmp(colname[i], "PasswordSalt"))
            password_salt = argv[i];
        else if (!strcmp(colname[i], "ID"))
        {
            ID id;
            id.unSerialize(string(argv[i]));
            (*user)->setID(id);
        }
        else if (!strcmp(colname[i], "Admin"))
            if (!strcmp(argv[i], "Yes"))
                (*user)->setAdmin(true);
    }
    (*user)->setPasswordSalt(password_salt);
    (*user)->setPasswordHash(password_hash);

    return 0;
};

int ConfigurationDatabase::userListDataCallback(vector<SP<User> >* user_list, void* v_self, int argc, char** argv, char** colname)
{
    assert(user_list && v_self);

    string name, password_hash, password_salt;
    ID id;
    bool admin = false;

    int i;
    for (i = 0; i < argc; ++i)
    {
        if (!argv[i]) continue;

        if (!strcmp(colname[i], "Name"))
            name = argv[i];
        else if (!strcmp(colname[i], "PasswordSHA512"))
            password_hash = argv[i];
        else if (!strcmp(colname[i], "PasswordSalt"))
            password_salt = argv[i];
        else if (!strcmp(colname[i], "ID"))
            id.unSerialize(string(argv[i]));
        else if (!strcmp(colname[i], "Admin"))
            if (!strcmp(argv[i], "Yes"))
                admin = true;
    }

    if (name.size() <= 0) return 0;

    SP<User> user(new User);
    user->setPasswordSalt(password_salt);
    user->setPasswordHash(password_hash);
    user->setID(id);
    user->setName(UnicodeString::fromUTF8(name));
    user->setAdmin(admin);

    (*user_list).push_back(user);
    return 0;
};

int ConfigurationDatabase::motdCallback(UnicodeString* us, void* v_self, int argc, char** argv, char** colname)
{
    assert(us && v_self);

    int i;
    for (i = 0; i < argc; ++i)
    {
        if (!argv[i]) continue;

        if (!strcmp(colname[i], "Content"))
            (*us) = TO_UNICODESTRING(string(argv[i]));
    }
    
    return 0;
}

int ConfigurationDatabase::allowedAndForbiddenSocketAddressRangesCallback(bool* default_allowance, SocketAddressRange* allowed, SocketAddressRange* forbidden, void* v_self, int argc, char** argv, char** colname)
{
    assert(allowed);
    assert(forbidden);
    assert(default_allowance);

    int i;
    for (i = 0; i < argc; ++i)
    {
        if (!argv[i]) continue;

        if (!strcmp(colname[i], "Allowed"))
        {
            bool result = allowed->unSerialize(hex_to_bytes(argv[i]));
            if (!result)
            {
                LOG(Error, "The socket address range for allowed connections in database is malformed.");
                return 1;
            }
        }
        else if (!strcmp(colname[i], "Forbidden"))
        {
            bool result = forbidden->unSerialize(hex_to_bytes(argv[i]));
            if (!result)
            {
                LOG(Error, "The socket address range for allowed connections in database is malformed.");
                return 1;
            }
        }
        else if (!strcmp(colname[i], "DefaultAllowance"))
        {
            if (!strcmp(argv[i], "Yes"))
                (*default_allowance) = true;
            else if (!strcmp(argv[i], "No"))
                (*default_allowance) = false;
            else
            {
                LOG(Error, "The default allowance field in database for address restrictions is malformed. Defaulting it to allow everyone.");
                (*default_allowance) = true;
            }
        }
    }

    return 0;
}

static int c_callback(void* a, int b, char** c, char** d)
{
    function4<int, void*, int, char**, char**>* sql_callback_function = 
    (function4<int, void*, int, char**, char**>*) a;

    return (*sql_callback_function)((void*) 0x1, b, c, d);
}

void ConfigurationDatabase::saveMOTD(UnicodeString motd)
{
    saveMOTDUTF8(TO_UTF8(motd));
}

void ConfigurationDatabase::saveMOTDUTF8(string motd_utf8)
{
    assert(db);
    
    string motd_escaped = escape_sql_string(motd_utf8);
    
    char* errormsg = (char*) 0;
    
    string statement;
    statement = string("DELETE FROM MOTD;");
    int result = sqlite3_exec(db, statement.c_str(), 0, 0, &errormsg);
    if (result != SQLITE_OK) /* Yes, next one is a warning and not an error */
    { LOG(Error, "Error while executing SQL statement \"" << statement << "\": " << errormsg); };
    
    if (errormsg) sqlite3_free(errormsg);
    errormsg = (char*) 0;
    
    statement = string("INSERT INTO MOTD(Content) VALUES(\'") + motd_escaped + string("\');");
    result = sqlite3_exec(db, statement.c_str(), 0, 0, &errormsg);
    if (result != SQLITE_OK)
    { LOG(Error, "Error while executing SQL statement \"" << statement << "\": " << errormsg); };
    
    if (errormsg) sqlite3_free(errormsg);
}

string ConfigurationDatabase::loadMOTDUTF8()
{
    return TO_UTF8(loadMOTD());
}

UnicodeString ConfigurationDatabase::loadMOTD()
{
    assert(db);
    
    UnicodeString motd;
    
    function4<int, void*, int, char**, char**> sql_callback_function;
    sql_callback_function = boost::bind(&ConfigurationDatabase::motdCallback, this, &motd, _1, _2, _3, _4);
    
    string statement = string("SELECT Content FROM MOTD;");
    char* errormsg = (char*) 0;
    int result = sqlite3_exec(db, statement.c_str(), c_callback, (void*) &sql_callback_function, &errormsg);
    if (result != SQLITE_OK)
    { LOG(Error, "Error while executing SQL statement \"" << statement << "\": " << errormsg); };
    
    if (errormsg) sqlite3_free(errormsg);
    
    return motd;
}

vector<SP<User> > ConfigurationDatabase::loadAllUserData()
{
    assert(db);

    vector<SP<User> > result_users;

    function4<int, void*, int, char**, char**> sql_callback_function;
    sql_callback_function = boost::bind(&ConfigurationDatabase::userListDataCallback, this, &result_users, _1, _2, _3, _4);

    string statement;
    statement = string("SELECT Name, ID, PasswordSHA512, PasswordSalt, Admin FROM Users;");
    int result = sqlite3_exec(db, statement.c_str(), c_callback, (void*) &sql_callback_function, 0);
    if (result != SQLITE_OK) return vector<SP<User> >();

    return result_users;
}

void ConfigurationDatabase::deleteSlotProfileData(const UnicodeString &name)
{
    assert(db);

    string name_utf8 = TO_UTF8(name);
    if (escape_sql_string(name_utf8).size() < 1) return;

    string statement = string("DELETE FROM Slotprofiles WHERE Name = \'") + escape_sql_string(name_utf8) + string("\';"); 
    sqlite3_exec(db, statement.c_str(), 0, 0, 0);
}

void ConfigurationDatabase::deleteUserData(const UnicodeString &name)
{
    assert(db);

    string name_utf8 = TO_UTF8(name);
    if (escape_sql_string(name_utf8).size() < 1) return;

    string statement = string("DELETE FROM Users WHERE Name = \'") + escape_sql_string(name_utf8) + string("\';"); 
    sqlite3_exec(db, statement.c_str(), 0, 0, 0);
}

SP<User> ConfigurationDatabase::loadUserData(const ID& id)
{
    assert(db);

    string id_str = escape_sql_string(id.serialize());
    
    SP<User> r_result(new User);
    function4<int, void*, int, char**, char**> sql_callback_function;
    sql_callback_function = boost::bind(&ConfigurationDatabase::userDataCallback, this, &r_result, _1, _2, _3, _4);

    string statement;
    statement = string("SELECT Name, ID, PasswordSHA512, PasswordSalt, Admin FROM Users WHERE ID = \'") + id_str + string("\';");
    int result = sqlite3_exec(db, statement.c_str(), c_callback, (void*) &sql_callback_function, 0);
    if (result != SQLITE_OK) return SP<User>();
    if (r_result->getNameUTF8().size() == 0) return SP<User>();

    return r_result;
}

SP<User> ConfigurationDatabase::loadUserData(const UnicodeString &name)
{
    assert(db);

    string name_utf8 = TO_UTF8(name);
    if (escape_sql_string(name_utf8).size() < 1) return SP<User>();
    
    SP<User> r_result(new User);
    function4<int, void*, int, char**, char**> sql_callback_function;
    sql_callback_function = boost::bind(&ConfigurationDatabase::userDataCallback, this, &r_result, _1, _2, _3, _4);

    string statement;
    statement = string("SELECT Name, ID, PasswordSHA512, PasswordSalt, Admin FROM Users WHERE Name = \'") + escape_sql_string(name_utf8) + string("\';");
    int result = sqlite3_exec(db, statement.c_str(), c_callback, (void*) &sql_callback_function, 0);
    if (result != SQLITE_OK) return SP<User>();
    if (r_result->getNameUTF8().size() == 0) return SP<User>();

    return r_result;
}

void ConfigurationDatabase::saveUserData(User* user)
{
    assert(db && user);

    string name_utf8 = TO_UTF8(user->getName());
    if (escape_sql_string(name_utf8).size() < 1) return;

    string statement;
    statement = string("DELETE FROM Users WHERE Name = \'") + escape_sql_string(name_utf8) + string("\';"); 
    int result = sqlite3_exec(db, statement.c_str(), 0, 0, 0);

    string admin_str("No");
    if (user->isAdmin()) admin_str = "Yes";
        
    statement = string("INSERT INTO Users(Name, ID, PasswordSHA512, PasswordSalt, Admin) VALUES(\'") + escape_sql_string(name_utf8) + string("\', \'") + escape_sql_string(user->getID().serialize()) + string("\', \'") + escape_sql_string(user->getPasswordHash()) + string("\', \'") + escape_sql_string(user->getPasswordSalt()) + string("\', \'") + admin_str + string("\');");
    result = sqlite3_exec(db, statement.c_str(), 0, 0, 0);
};

void ConfigurationDatabase::saveSlotProfileData(SlotProfile* slotprofile)
{
    assert(db && slotprofile);

    string name = slotprofile->getNameUTF8();

    string statement;
    statement = string("DELETE FROM Slotprofiles WHERE NAME = \'") + escape_sql_string(name) + string("\';");
    int result = sqlite3_exec(db, statement.c_str(), 0, 0, 0);

    stringstream ss;
    ss << "INSERT INTO Slotprofiles(Name, ID, Width, Height, Path, WorkingPath, SlotType, AllowedWatchers, AllowedLaunchers, AllowedPlayers, AllowedClosers, ForbiddenWatchers, ForbiddenLaunchers, ForbiddenPlayers, ForbiddenClosers, MaxSlots) VALUES(\'" << 
    escape_sql_string(slotprofile->getNameUTF8()) << "\',\'" << 
    escape_sql_string(slotprofile->getID().serialize()) << "\',\'" <<
    slotprofile->getWidth() << "\',\'" <<
    slotprofile->getHeight() << "\',\'" <<
    escape_sql_string(slotprofile->getExecutableUTF8()) << "\',\'" <<
    escape_sql_string(slotprofile->getWorkingPathUTF8()) << "\',\'" <<
    escape_sql_string(SlotNames[(size_t) slotprofile->getSlotType()]) << "\',\'" <<
    escape_sql_string(slotprofile->getAllowedWatchers().serialize()) << "\',\'" <<
    escape_sql_string(slotprofile->getAllowedLaunchers().serialize()) << "\',\'" <<
    escape_sql_string(slotprofile->getAllowedPlayers().serialize()) << "\',\'" <<
    escape_sql_string(slotprofile->getAllowedClosers().serialize()) << "\',\'" <<
    escape_sql_string(slotprofile->getForbiddenWatchers().serialize()) << "\',\'" <<
    escape_sql_string(slotprofile->getForbiddenLaunchers().serialize()) << "\',\'" <<
    escape_sql_string(slotprofile->getForbiddenPlayers().serialize()) << "\',\'" <<
    escape_sql_string(slotprofile->getForbiddenClosers().serialize()) << "\',\'" <<
    slotprofile->getMaxSlots() << "\');";

    char* errormsg = (char*) 0;
    result = sqlite3_exec(db, ss.str().c_str(), 0, 0, &errormsg);
    if (result != SQLITE_OK)
        LOG(Error, "Error while executing SQL statement \"" << ss.str() << "\": " << errormsg);
    if (errormsg) sqlite3_free(errormsg);
};

vector<UnicodeString> ConfigurationDatabase::loadSlotProfileNames()
{
    assert(db);

    vector<UnicodeString> name_list;

    function4<int, void*, int, char**, char**> sql_callback_function;
    sql_callback_function = boost::bind(&ConfigurationDatabase::slotprofileNameListDataCallback, this, &name_list, _1, _2, _3, _4);

    char* errormsg = (char*) 0;

    string statement;
    statement = string("SELECT Name FROM Slotprofiles;");
    int result = sqlite3_exec(db, statement.c_str(), c_callback, (void*) &sql_callback_function, &errormsg);
    if (result != SQLITE_OK)
    {
        LOG(Error, "Error while executing SQL statement \"" << statement << "\": " << errormsg);
        if (errormsg) sqlite3_free(errormsg);
        return name_list;
    }
    if (errormsg) sqlite3_free(errormsg);

    return name_list;
}

SP<SlotProfile> ConfigurationDatabase::loadSlotProfileData(const UnicodeString &name)
{
    assert(db);

    string name_utf8 = TO_UTF8(name);

    SlotProfile sp;

    function4<int, void*, int, char**, char**> sql_callback_function;
    sql_callback_function = boost::bind(&ConfigurationDatabase::slotprofileDataCallback, this, &sp, _1, _2, _3, _4);

    char* errormsg = (char*) 0;

    string statement;
    statement = string("SELECT Name, ID, Width, Height, Path, WorkingPath, SlotType, AllowedWatchers, AllowedLaunchers, AllowedPlayers, AllowedClosers, ForbiddenWatchers, ForbiddenLaunchers, ForbiddenPlayers, ForbiddenClosers, MaxSlots FROM Slotprofiles WHERE Name = \'") + escape_sql_string(name_utf8) + string("\';");
    int result = sqlite3_exec(db, statement.c_str(), c_callback, (void*) &sql_callback_function, &errormsg);
    if (result != SQLITE_OK)
    {
        LOG(Error, "Error while executing SQL statement \"" << statement << "\": " << errormsg);
        if (errormsg) sqlite3_free(errormsg);
        return SP<SlotProfile>();
    }
    if (errormsg) sqlite3_free(errormsg);

    return SP<SlotProfile>(new SlotProfile(sp));
};

void ConfigurationDatabase::saveMaximumNumberOfSlots(ui32 maximum)
{
    assert(db);

    string statement = string("DELETE FROM GlobalSettings WHERE Key = \'MaximumSlots\';");
    int result = sqlite3_exec(db, statement.c_str(), 0, 0, 0);

    char* errormsg = (char*) 0;

    stringstream ss;
    ss << maximum;
    statement = string("INSERT INTO GlobalSettings(Key, Value) VALUES(\'MaximumSlots\', \'") + escape_sql_string(ss.str()) + string("\');");
    result = sqlite3_exec(db, statement.c_str(), 0, 0, &errormsg);
    if (result != SQLITE_OK)
    { LOG(Error, "Error while executing SQL statement \"" << statement << "\": " << errormsg); };

    if (errormsg) sqlite3_free(errormsg);
}

ui32 ConfigurationDatabase::loadMaximumNumberOfSlots()
{
    assert(db);

    ui32 maximum = 0xffffffff;
    function4<int, void*, int, char**, char**> sql_callback_function;
    sql_callback_function = boost::bind(&ConfigurationDatabase::maximumSlotsCallback, this, &maximum, _1, _2, _3, _4);

    string statement = string("SELECT Key, Value FROM GlobalSettings WHERE Key = \'MaximumSlots\';");
    char* errormsg = (char*) 0;
    int result = sqlite3_exec(db, statement.c_str(), c_callback, (void*) &sql_callback_function, &errormsg);
    if (result != SQLITE_OK)
    {
        LOG(Error, "Error while executing SQL statement \"" << statement << "\": " << errormsg);
        if (errormsg) sqlite3_free(errormsg);
        return 0xffffffff;
    }
    if (errormsg) sqlite3_free(errormsg);

    return maximum;
}

void ConfigurationDatabase::saveAllowedAndForbiddenSocketAddressRanges(bool default_allowance, const SocketAddressRange &allowed, const SocketAddressRange &forbidden)
{
    assert(db);

    char* errormsg = (char*) 0;

    string allowed_str = allowed.serialize();
    string forbidden_str = forbidden.serialize();

    string statement = string("DELETE FROM AllowedAndForbiddenSocketAddressRanges;");
    int result = sqlite3_exec(db, statement.c_str(), 0, 0, &errormsg);
    if (result != SQLITE_OK)
    { LOG(Error, "Error while executing SQL statement \"" << statement << "\": " << errormsg); };
    if (errormsg) sqlite3_free(errormsg);

    string default_allowance_str("Yes");
    if (!default_allowance)
        default_allowance_str = "No";

    stringstream ss;
    ss << "INSERT INTO AllowedAndForbiddenSocketAddressRanges(Allowed, Forbidden, DefaultAllowance) VALUES(\'" << bytes_to_hex(allowed_str) << "\', \'" << bytes_to_hex(forbidden_str) << "\', \'" << default_allowance_str << "\');";
    result = sqlite3_exec(db, ss.str().c_str(), 0, 0, &errormsg);
    if (result != SQLITE_OK)
    { LOG(Error, "Error while executing SQL statement \"" << statement << "\": " << errormsg); };

    if (errormsg) sqlite3_free(errormsg);
}

void ConfigurationDatabase::loadAllowedAndForbiddenSocketAddressRanges(bool* default_allowance, SocketAddressRange* allowed, SocketAddressRange* forbidden)
{
    assert(allowed);
    assert(forbidden);
    assert(db);
    assert(default_allowance);

    SocketAddressRange allowed_f, forbidden_f;
    bool default_allowance_f = true;

    function4<int, void*, int, char**, char**> sql_callback_function;
    sql_callback_function = boost::bind(&ConfigurationDatabase::allowedAndForbiddenSocketAddressRangesCallback, this, &default_allowance_f, &allowed_f, &forbidden_f, _1, _2, _3, _4);

    string statement = string("SELECT Allowed, Forbidden, DefaultAllowance FROM AllowedAndForbiddenSocketAddressRanges;");
    char* errormsg = (char*) 0;
    int result = sqlite3_exec(db, statement.c_str(), c_callback, (void*) &sql_callback_function, &errormsg);
    if (result != SQLITE_OK)
    {
        LOG(Error, "Error while executing SQL statement \"" << statement << "\": " << errormsg);
        if (errormsg) sqlite3_free(errormsg);
        return;
    }
    if (errormsg) sqlite3_free(errormsg);

    (*allowed) = allowed_f;
    (*forbidden) = forbidden_f;
    (*default_allowance) = default_allowance_f;
}


data1D dfterm::escape_sql_string(const data1D &str)
{
    data1D result;
    result.reserve(str.size());
    size_t str_size = str.size();

    size_t i1;
    for (i1 = 0; i1 < str_size; ++i1)
    {
        if (str[i1] == '\'')
            result.append("\'\'");
        else if (str[i1] == 0)
            continue;
        else
            result.push_back(str[i1]);
    }

    return result;
}

