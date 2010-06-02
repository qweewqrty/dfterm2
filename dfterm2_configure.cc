/* Simple configuration script for dfterm2 databases. */

#include "dfterm2_configuration.hpp"
#include <string>
#include <iostream>
#include <unistr.h>
#include "types.hpp"

using namespace std;
using namespace dfterm;

enum Action { Nothing, ListUsers, AddUser, RemoveUser, UserInfo };

int main(int argc, char* argv[])
{
    string database_file("dfterm2_database.sqlite3");

    Action action = Nothing;

    UnicodeString username;
    UnicodeString password;

    int i1;
    for (i1 = 1; i1 < argc; i1++)
    {
        if (!strcmp(argv[i1], "--database") || !strcmp(argv[i1], "--db") && i1 < argc-1)
            database_file = argv[++i1];
        else if (!strcmp(argv[i1], "--adduser") && i1 < argc - 2)
        {
            username = UnicodeString::fromUTF8(string(argv[i1+1]));
            password = UnicodeString::fromUTF8(string(argv[i1+2]));
            action = AddUser;
            i1 += 2;
        }
        else if (!strcmp(argv[i1], "--removeuser") && i1 < argc - 1)
        {
            username = UnicodeString::fromUTF8(string(argv[i1+1]));
            action = RemoveUser;
            i1++;
        }
        else if (!strcmp(argv[i1], "--userinfo") && i1 < argc - 1)
        {
            username = UnicodeString::fromUTF8(string(argv[i1+1]));
            action = UserInfo;
            i1++;
        }
        else if (!strcmp(argv[i1], "--listusers"))
            action = ListUsers;
        else if (!strcmp(argv[i1], "--help") || !strcmp(argv[i1], "-h"))
        {
            cout << "Usage: " << endl;
            cout << "--database (database file)" << endl;
            cout << "-db (database file)   Set the database file used. By default dfterm2 will try to look for" << endl;
            cout << "                      dfterm2_database.sqlite3" << endl;
            cout << "--adduser (name) (hash)" << endl;
            cout << "                      Add a new user to database. The password will be hashed with SHA512." << endl;
            cout << "--removeuser (name)" << endl;
            cout << "                      Removes a user from the database." << endl;
            cout << "--userinfo (name)" << endl;
            cout << "                      Shows user information." << endl;
            cout << "--listusers" << endl;
            cout << "                      Lists all users in the database." << endl;
            cout << endl;
            return 0;
        }
    }

    cout << "Selecting database file " << database_file << endl;

    SP<ConfigurationDatabase> cdb(new ConfigurationDatabase);
    if (!cdb->openUTF8(database_file))
    {
        cout << "Failed." << endl;
        return -1;
    }

    User user;
    SP<User> user_sp;
    switch(action)
    {
        case AddUser:
        cout << "Adding user." << endl;
        user.setName(username);
        {
            string password_utf8;
            password.toUTF8String(password_utf8);
            user.setPasswordHash(password_utf8);
        }
        cdb->saveUserData(&user);
        return 0;
        case UserInfo:
        cout << "Checking user info." << endl;
        user_sp = cdb->loadUserData(username);
        if (!user_sp) cout << "Could not read user information." << endl;
        else
        {
            string name_utf8;
            user_sp->getName().toUTF8String(name_utf8);
            cout << "Name: " << name_utf8 << endl;
            string password_utf8 = user_sp->getPasswordHash();
            cout << "Password (hash): " << password_utf8 << endl;
        }
        return 0;
        default:
        case Nothing:
        cout << "Nothing to do." << endl;
        return 0;
    }

    return 0;
}

