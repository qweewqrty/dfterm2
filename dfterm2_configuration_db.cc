#include "sqlite3.h"
#include "dfterm2_configuration.hpp"
#include <string>
#include <cstdio>
#include <iostream>
#include <boost/bind.hpp>

using namespace dfterm;
using namespace std;
using namespace boost;

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

bool ConfigurationDatabase::open(UnicodeString filename)
{
    if (db) sqlite3_close(db);
    db = (sqlite3*) 0;

    string filename_utf8;
    filename.toUTF8String(filename_utf8);

    int result = sqlite3_open(filename_utf8.c_str(), &db);
    if (result)
        return false;

    /* Create tables. These calls fail if they already exist. */
    result = sqlite3_exec(db, "CREATE TABLE Users(Name TEXT, PasswordSHA512 TEXT);", 0, 0, 0); 

    return true;
}

int ConfigurationDatabase::userDataCallback(string* name, string* password_hash, void* v_self, int argc, char** argv, char** colname)
{
    (*name).clear();
    (*password_hash).clear();

    int i;
    for (i = 0; i < argc; i++)
    {
        if (!argv[i]) continue;

        if (!strcmp(colname[i], "Name"))
            (*name) = argv[i];
        else if (!strcmp(colname[i], "PasswordSHA512"))
            (*password_hash) = argv[i];
    }
};

static int c_callback(void* a, int b, char** c, char** d)
{
    function4<int, void*, int, char**, char**>* sql_callback_function = 
    (function4<int, void*, int, char**, char**>*) a;

    return (*sql_callback_function)((void*) 0, b, c, d);
}

SP<User> ConfigurationDatabase::loadUserData(UnicodeString name)
{
    if (!db) return SP<User>();

    string name_utf8;
    name.toUTF8String(name_utf8);

    string r_name, r_password_hash;
    function4<int, void*, int, char**, char**> sql_callback_function;
    sql_callback_function = bind(&ConfigurationDatabase::userDataCallback, this, &r_name, &r_password_hash, _1, _2, _3, _4);

    string statement;
    statement = string("SELECT Name, PasswordSHA512 FROM Users WHERE Name = \'") + name_utf8 + string("\';");
    int result = sqlite3_exec(db, statement.c_str(), c_callback, (void*) &sql_callback_function, 0);
    if (result != SQLITE_OK) return SP<User>();

    SP<User> user(new User);
    user->setPasswordHash(r_password_hash);
    user->setName(UnicodeString::fromUTF8(r_name));

    return user;
}

void ConfigurationDatabase::saveUserData(User* user)
{
    if (!db) return;
    if (!user) return;

    string name_utf8;
    user->getName().toUTF8String(name_utf8);

    string statement;
    statement = string("DELETE FROM Users WHERE Name = \'") + name_utf8 + string("\';"); 
    int result = sqlite3_exec(db, statement.c_str(), 0, 0, 0);
        
    statement = string("INSERT INTO Users(Name, PasswordSHA512) VALUES(\'") + name_utf8 + string("\', \'") + user->getPasswordHash() + string("\');");
    result = sqlite3_exec(db, statement.c_str(), 0, 0, 0);
};

