/* Simple configuration script for dfterm2 databases. */

#include "dfterm2_configuration.hpp"
#include <string>
#include <iostream>
#include <unistr.h>
#include "types.hpp"
#include <openssl/rand.h>
#include <sys/time.h>

using namespace std;
using namespace dfterm;

enum Action { Nothing, ListUsers, AddUser, RemoveUser, UserInfo };

void seedRNG()
{
    int rng_counter = 100000;
    /* Seed random number generator a bit */
    while(!RAND_status() && rng_counter > 0)
    {
        rng_counter--;

        #ifdef __WIN32
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>

        RAND_screen();
        DWORD t = GetTickCount();
        RAND_ADD((void*) &t, sizeof(t), sizeof(t) / 2);
        #endif

        struct timeval tv;
        gettimeofday(&tv, NULL);
        RAND_add((void*) &tv, sizeof(tv), sizeof(tv) / 4);
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

    bool make_admin = false;

    int i1;
    for (i1 = 1; i1 < argc; i1++)
    {
        if (!strcmp(argv[i1], "--database") || !strcmp(argv[i1], "--db") && i1 < argc-1)
            database_file = argv[++i1];
        else if (!strcmp(argv[i1], "--adduser") && i1 < argc - 2)
        {
            username = UnicodeString::fromUTF8(string(argv[i1+1]));
            password = UnicodeString::fromUTF8(string(argv[i1+2]));
            if (i1 < argc - 3 && !strcmp(argv[i1+3], "admin"))
            {
                make_admin = true;
                i1++;
            }
                
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
            cout << "--adduser (name) (password) [admin]" << endl;
            cout << "                      Add a new user to database. The password will be hashed with SHA512." << endl;
            cout << "                      If admin is specified, makes this user an admin." << endl;
            cout << "                      For security purposes, you probably should change the password from" << endl; 
            cout << "                      inside dfterm2 after you log in for the first time." << endl;
            cout << "--removeuser (name)" << endl;
            cout << "                      Removes a user from the database." << endl;
            cout << "--userinfo (name)" << endl;
            cout << "                      Shows user information." << endl;
            cout << "--listusers" << endl;
            cout << "                      Lists all users in the database." << endl;
            cout << endl;
            cout << "Examples:" << endl;
            cout << "  Adding an administrator: " << endl;
            cout << "  dfterm2_configure --adduser Adeon s3cr3t_p4ssw0rd admin" << endl;
            return 0;
        }
    }

    cout << "Selecting database file " << database_file << endl;

    SP<ConfigurationDatabase> cdb(new ConfigurationDatabase);
    if (cdb->openUTF8(database_file) == Failure)
    {
        cout << "Failed." << endl;
        return -1;
    }

    User user;
    SP<User> user_sp;
    switch(action)
    {
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

