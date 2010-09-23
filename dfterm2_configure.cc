/* Simple configuration script for dfterm2 databases. */

#include "configuration_db.hpp"
#include <string>
#include <iostream>
#include <unicode/unistr.h>
#include "types.hpp"
#include <openssl/rand.h>
#include <time.h>
#include <unicode/uclean.h>
#include <cstring>
#include "sockets.hpp"
#include "lua_configuration.hpp"

#include <clocale>

using namespace std;
using namespace dfterm;
using namespace trankesbel;

enum Action { Nothing, ListUsers, AddUser, RemoveUser, UserInfo, SetMotd, GetMotd, RemoveAddressRestrictions };

void seedRNG()
{
    int rng_counter = 100000;
    /* Seed random number generator a bit */
    while(!RAND_status() && rng_counter > 0)
    {
        --rng_counter;

        #ifdef __WIN32
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>

        RAND_screen();
        DWORD t = GetTickCount();
        RAND_add((void*) &t, sizeof(t), sizeof(t) / 2);
        #else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        RAND_add((void*) &tv, sizeof(tv), sizeof(tv) / 4);
        #endif
    }
}

void makeRandomBytes(unsigned char* output, int output_size)
{
    if (!RAND_bytes(output, output_size))
    {
        seedRNG();
        if (!RAND_bytes(output, output_size))
            RAND_pseudo_bytes(output, output_size);
    }
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");
    
    seedRNG();
    if (RAND_status() == 0)
    {
        cout << "Could not seed random number generator with sufficient entropy." << endl;
        cout << "Are you on a weird system or something?" << endl;
        return -1;
    }

    string database_file("dfterm2_database.sqlite3");

    Action action = Nothing;

    UnicodeString username;
    UnicodeString password;
    UnicodeString motd;

    bool make_admin = false;

    bool use_luaconf_database = true;
    string conffile("dfterm2.conf");

    int i1;
    for (i1 = 1; i1 < argc; ++i1)
    {
        if ((!strcmp(argv[i1], "--database") || !strcmp(argv[i1], "--db")) && i1 < argc-1)
        {
            database_file = argv[++i1];
            use_luaconf_database = false;
        }
        else if (!strcmp(argv[i1], "--conffile"))
            conffile = argv[++i1];
        else if (!strcmp(argv[i1], "--adduser") && i1 < argc - 2)
        {
            username = UnicodeString::fromUTF8(string(argv[i1+1]));
            password = UnicodeString::fromUTF8(string(argv[i1+2]));
            if (i1 < argc - 3 && !strcmp(argv[i1+3], "admin"))
            {
                make_admin = true;
                ++i1;
            }
                
            action = AddUser;
            i1 += 2;
        }
        else if (!strcmp(argv[i1], "--removeuser") && i1 < argc - 1)
        {
            username = UnicodeString::fromUTF8(string(argv[i1+1]));
            action = RemoveUser;
            ++i1;
        }
        else if (!strcmp(argv[i1], "--userinfo") && i1 < argc - 1)
        {
            username = UnicodeString::fromUTF8(string(argv[i1+1]));
            action = UserInfo;
            ++i1;
        }
        else if (!strcmp(argv[i1], "--setmotd") && i1 < argc - 1)
        {
            motd = UnicodeString::fromUTF8(string(argv[i1+1]));
            action = SetMotd;
            ++i1;
        }
        else if (!strcmp(argv[i1], "--motd") || !strcmp(argv[i1], "--getmotd"))
        {
            action = GetMotd;
        }
        else if (!strcmp(argv[i1], "--removeaddressrestrictions"))
        {
            action = RemoveAddressRestrictions;
        }
        else if (!strcmp(argv[i1], "--help") || !strcmp(argv[i1], "-h"))
        {
            cout << "Usage: " << endl;
            cout << "--database (database file)" << endl;
            cout << "-db (database file)   Set the database file used. By default dfterm2 will try to look for" << endl;
            cout << "                      dfterm2_database.sqlite3" << endl;
            cout << "--removeaddressrestrictions" << endl;
            cout << "                      Removes all restrictions on addresses and lets everyone to connect it." << endl;
            cout << "                      You can use this if you accidentally firewalled yourself out." << endl;
            cout << "--adduser (name) (password) [admin]" << endl;
            cout << "                      Add a new user to database. The password will be hashed with SHA512." << endl;
            cout << "                      If admin is specified, makes this user an admin." << endl;
            cout << "                      For security purposes, you probably should change the password from" << endl; 
            cout << "                      inside dfterm2 after you log in for the first time." << endl;
            cout << "--removeuser (name)" << endl;
            cout << "                      Removes a user from the database." << endl;
            cout << "--setmotd (motd)      Sets Message of the Day." << endl;
            cout << "--getmotd             Print out current Message of the Day." << endl;
            cout << "--motd                Print out current Message of the Day." << endl;
            cout << "--userinfo (name)" << endl;
            cout << "                      Shows user information." << endl;
            cout << "--conffile (configuration file)" << endl;
            cout << "                      Use given configuration file to look for what database to use." << endl;
            cout << "                      Defaults to dfterm2.conf. --database argument takes preference over this" << endl;
            cout << "                      argument." << endl;
            cout << endl;
            cout << "Examples:" << endl;
            cout << "  Adding an administrator: " << endl;
            cout << "    dfterm2_configure --adduser Adeon s3cr3t_p4ssw0rd admin" << endl;
            cout << "  Setting a MotD: " << endl;
            cout << "    dfterm2_configure --setmotd \"Hello. This is Adeon's b3stest server on da earth. Close DF before disconnecting, if at all possible." << endl; 
            return 0;
        }
    }

    if (!use_luaconf_database)
        cout << "Will not use configuration file because database was specified on command line." << endl;
    else
    {
        string database2, logfile2;
        bool result = readConfigurationFile(conffile, &logfile2, &database2);
        if (result)
            database_file = database2;
    }

    cout << "Selecting database file " << database_file << endl;

    SP<ConfigurationDatabase> cdb(new ConfigurationDatabase);
    OpenStatus os = cdb->openUTF8(database_file);
    if (os == Failure)
    {
        cout << "Failed." << endl;
        return -1;
    }
    if (os == OkCreatedNewDatabase)
        cout << "Created a new database." << endl;

    User user;
    SP<User> user_sp;
    switch(action)
    {
        case RemoveAddressRestrictions:
        {
        SocketAddressRange empty_allowed, empty_forbidden;
        bool default_allowance = true;

        cdb->saveAllowedAndForbiddenSocketAddressRanges(default_allowance, empty_allowed, empty_forbidden);
        }
        break;
        case GetMotd:
        {
        string motd = cdb->loadMOTDUTF8();
        cout << motd << endl;
        }
        return 0;
        break;
        case SetMotd:
        cdb->saveMOTD(motd);
        cout << "MotD set." << endl;
        return 0;
        case RemoveUser:
        cout << "Removing user." << endl;
        user_sp = cdb->loadUserData(username);
        if (!user_sp)
        {
            cout << "Cannot do that. No such user." << endl;
            return 0;
        }

        cdb->deleteUserData(username);
        return 0;
        case AddUser:
        cout << "Adding user." << endl;
        /* Check if user of this name already exists. */
        user_sp = cdb->loadUserData(username);
        if (user_sp)
        {
            cout << "Cannot do that. This user already exists." << endl;
            return 0;
        }

        user.setName(username);
        if (make_admin)
            user.setAdmin(true);

        {
            unsigned char random_salt[8];
            makeRandomBytes(random_salt, 8);
            user.setPasswordSalt(bytes_to_hex(data1D((char*) random_salt, 8)));

            string password_utf8;
            password.toUTF8String(password_utf8);
            user.setPassword(password_utf8);
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
            string password_salt = user_sp->getPasswordSalt();
            cout << "Password salt: " << password_salt << endl;
            cout << "ID: " << user_sp->getID().serialize() << endl;
            string admin("Yes");
            if (!user_sp->isAdmin()) admin = "No";
            cout << "Administrator: " << admin << endl;
        }
        return 0;
        default:
        case Nothing:
        cout << "Nothing to do." << endl;
        return 0;
    }

    return 0;
}

