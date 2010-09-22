#include "client.hpp"
#include "sockets.hpp"
#include <cstring>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include "nanoclock.hpp"
#include "cp437_to_unicode.hpp"
#include "logger.hpp"
#include "state.hpp"

#include "lua.hpp"

#include <clocale>

using namespace dfterm;
using namespace trankesbel;
using namespace boost;
using namespace std;

static int getenv_unicode(lua_State* l)
{
    const char* str = luaL_checkstring(l, 1);
    #ifdef _WIN32
    wchar_t* wc = (wchar_t*) 0;
    size_t wc_size = 0;
    TO_WCHAR_STRING(string(str), wc, &wc_size);
    if (wc_size == 0)
        luaL_error(l, "Environment variable name string is corrupted.");
    wc = new wchar_t[wc_size+1];
    memset(wc, 0, sizeof(wchar_t)+1);
    TO_WCHAR_STRING(string(str), wc, &wc_size);

    wchar_t* result = _wgetenv(wc);
    delete[] wc;
    if (!result)
    {
        lua_pushnil(l);
        return 1;
    }
    string result2 = TO_UTF8(result);
    lua_pushlstring(l, result2.c_str(), result2.size());
    return 1;
    #else
    char* result = getenv(str);
    if (!result)
    {
        lua_pushnil(l);
        return 1;
    }
    lua_pushstring(l, result);
    return 1;
    #endif
}

/* Read from lua script what to use as database and logfile */
static bool readConfigurationFile(const std::string &conffile, std::string* logfile, std::string* database)
{
    wchar_t* wc = (wchar_t*) 0;
    size_t wc_size = 0;
    TO_WCHAR_STRING(conffile, wc, &wc_size);
    if (wc_size == 0)
    {
        cout << "Configuration file name is corrupted. Can't read configuration." << endl;
        return false;
    }
    wc = new wchar_t[wc_size+1];
    memset(wc, 0, (wc_size+1) * sizeof(wchar_t));
    TO_WCHAR_STRING(conffile, wc, &wc_size);

    #ifdef _WIN32
    FILE* f = _wfopen(wc, L"rb");
    #else
    FILE* f = fopen(conffile.c_str(), "rb");
    #endif
    if (!f)
    {
        int err = errno;
        wprintf(L"Could not open configuration file %ls, errno %d.\n", wc, err);
        delete[] wc;
        return false;
    }
    delete[] wc;

    fseek(f, 0, SEEK_END);
    ui64 size = ftell(f);
    if (size > CONFIGURATION_FILE_MAX_SIZE)
    {
        cout << "Configuration file is too large. (Over " << CONFIGURATION_FILE_MAX_SIZE << " bytes)" << endl;
        return false;
    }
    fseek(f, 0, SEEK_SET);

    char* buf = new char[size];
    fread(buf, size, 1, f);
    if (ferror(f))
    {
        cout << "Error while reading configuration file, error code " << ferror(f) << endl;
        delete[] buf;
        return false;
    }

    lua_State* l = luaL_newstate();
    if (!l)
    {
        cout << "Could not create a lua state when reading configuration file." << endl;
        delete[] buf;
        return false;
    }
    luaL_openlibs(l);

    /* Replace os.getenv in lua with a version that takes unicode. */
    lua_getfield(l, LUA_GLOBALSINDEX, "os");
    lua_pushcfunction(l, getenv_unicode);
    lua_setfield(l, -2, "getenv");
    lua_pop(l, 1);

    int result = luaL_loadbuffer(l, buf, size, "conffile");
    if (result != 0)
    {
        delete[] buf;
        cout << "Could not load configuration file. ";
        if (result != LUA_ERRMEM)
            cout << lua_tostring(l, -1) << endl;
        else
            cout << "Memory error." << endl;
        return false;
    }

    delete[] buf;
    result = lua_pcall(l, 0, 0, 0);
    if (result != 0)
    {
        cout << "Could not run configuration file. ";
        if (result != LUA_ERRMEM)
            cout << lua_tostring(l, -1) << endl;
        else
            cout << "Memory error." << endl;
        return false;
    }
    cout << "Ran through configuration file." << endl;

    bool set_database = false;
    bool set_logfile = false;

    lua_getfield(l, LUA_GLOBALSINDEX, "database");
    if (lua_isstring(l, -1))
    {
        cout << "Database: " << lua_tostring(l, -1) << endl;
        set_database = true;
        (*database) = lua_tostring(l, -1);
        lua_pop(l, 1);
    }
    else
        cout << "Database not set." << endl;
    lua_getfield(l, LUA_GLOBALSINDEX, "logfile");
    if (lua_isstring(l, -1))
    {
        cout << "Logfile: " << lua_tostring(l, -1) << endl;
        set_logfile = true;
        (*logfile) = lua_tostring(l, -1);
        lua_pop(l, 1);
    }
    else
        cout << "Logfile not set." << endl;

    lua_close(l);

    return true;
}

void resolve_success(bool *success_p,
                     SocketAddress *sa, 
                     string* error_message_p, 
                     bool success, 
                     SocketAddress new_addr, 
                     string error_message)
{
    (*sa) = new_addr;
    (*success_p) = success;
    (*error_message_p) = error_message;
};

class sockets_initialize
{
    public:
        sockets_initialize()
        { initializeSockets(); };
        ~sockets_initialize()
        { shutdownSockets(); };
};

#ifdef _WIN32
WCHAR local_directory[60000];
size_t local_directory_size;
#endif

int main(int argc, char* argv[])
{
#ifdef _WIN32
    local_directory_size = 0;
    memset(local_directory, 0, 60000 * sizeof(WCHAR));
    local_directory_size = GetCurrentDirectoryW(59999, local_directory);
#endif

    log_file = TO_UNICODESTRING(string("dfterm2.log"));
    setlocale(LC_ALL, "");

    sockets_initialize socket_initialization;

    string port("8000");
    string address("0.0.0.0");

    string httpport("8080");
    string httpaddress("0.0.0.0");
    string httpconnectaddress("255.255.255.255");

    string flashpolicyport("8081");
    string flashpolicyaddress("0.0.0.0");

    bool use_http_service = false;

    string database_file("dfterm2_database.sqlite3");

    SocketAddress listen_address, http_listen_address, flash_policy_listen_address;
    bool succeeded_resolve = false;
    string error_message;

    function3<void, bool, SocketAddress, string> resolve_binding = 
    boost::bind(resolve_success, &succeeded_resolve, &listen_address, &error_message, _1, _2, _3);
    function3<void, bool, SocketAddress, string> http_resolve_binding = 
    boost::bind(resolve_success, &succeeded_resolve, &http_listen_address, &error_message, _1, _2, _3);
    function3<void, bool, SocketAddress, string> flash_resolve_binding = 
    boost::bind(resolve_success, &succeeded_resolve, &flash_policy_listen_address, &error_message, _1, _2, _3);

    bool create_app_dir = false;

    bool use_luaconf_database = true;
    bool use_luaconf_logfile = true;
    string conffile("dfterm2.conf");

    int i1;
    for (i1 = 1; i1 < argc; ++i1)
    {
        if ((!strcmp(argv[i1], "--create-appdir")))
            create_app_dir = true;
        if ((!strcmp(argv[i1], "--conf")))
            conffile = argv[++i1];
        else if ((!strcmp(argv[i1], "--port") || !strcmp(argv[i1], "-p")) && i1 < argc-1)
            port = argv[++i1];
        else if ((!strcmp(argv[i1], "--httpport") || !strcmp(argv[i1], "-hp")) && i1 < argc-1)
            httpport = argv[++i1];
        else if ((!strcmp(argv[i1], "--database") || !strcmp(argv[i1], "-db")) && i1 < argc-1)
        {
            database_file = argv[++i1];
            use_luaconf_database = false;
        }
        else if ((!strcmp(argv[i1], "--address") || !strcmp(argv[i1], "-a")) && i1 < argc-1)
            address = argv[++i1];
        else if ((!strcmp(argv[i1], "--httpaddress") || !strcmp(argv[i1], "-ha")) && i1 < argc-1)
            httpaddress = argv[++i1];
        else if ((!strcmp(argv[i1], "--httpconnectaddress") || !strcmp(argv[i1], "-hca")) && i1 < argc-1)
            httpconnectaddress = argv[++i1];
        else if ((!strcmp(argv[i1], "--flashpolicyaddress") || !strcmp(argv[i1], "-fpa")) && i1 < argc-1)
            flashpolicyaddress = argv[++i1];
        else if ((!strcmp(argv[i1], "--flashpolicyport") || !strcmp(argv[i1], "-fpp")) && i1 < argc-1)
            flashpolicyport = argv[++i1];
        else if (!strcmp(argv[i1], "--http"))
            use_http_service = true;
        else if (!strcmp(argv[i1], "--version") || !strcmp(argv[i1], "-v"))
        {
            cout << "This is dfterm2, (c) 2010 Mikko Juola" << endl;
            cout << "This version was compiled " << __DATE__ << " " << __TIME__ << endl;
            cout << "This program is distributed under a BSD-style license. See license.txt" << endl << endl;
            cout << "This dfterm2 version supports IPv4 and IPv6." << endl;
            return 0;
        }
        else if (!strcmp(argv[i1], "--disablelog"))
        {
            log_file = UnicodeString();
        }
        else if (!strcmp(argv[i1], "--logfile") && i1 < argc-1)
        {
            use_luaconf_logfile = false;
            log_file = TO_UNICODESTRING(string(argv[i1+1]));
            ++i1;
        }
        else if (!strcmp(argv[i1], "--help") || !strcmp(argv[i1], "-h") || !strcmp(argv[i1], "-?"))
        {
            cout << "Usage: " << endl;
            cout << "dfterm2 [parameters]" << endl;
            cout << "Where \'parameters\' is one or more of the following:" << endl << endl;
            cout << "--port (port number)" << endl;
            cout << "--p (port number)     Set the port where dfterm2 will listen. Defaults to 8000." << endl << endl;
            cout << "--address (address)" << endl;
            cout << "-a (address)          Set the address on which dfterm2 will listen on. Defaults to 0.0.0.0." << endl << endl;
            cout << "--httpport (port number)" << endl;
            cout << "-hp (port number)     Set the port where dfterm2 will listen for HTTP connections. Defaults to 8080." << endl << endl;
            cout << "--httpaddress (address)" << endl;
            cout << "--ha (address)        Set the address on which dfterm2 will listen for HTTP connections. Defaults to 0.0.0.0." << endl << endl;
            cout << "--httpconnectaddress (address)" << endl;
            cout << "-hca (address)" << endl;
            cout << "                      Set the address reported in the flash program for dfterm2 server. You should" << endl;
            cout << "                      set this to your IP as it appears to outside world. Defaults to 255.255.255.255" << endl;
            cout << "--flashpolicyaddress (address)" << endl;
            cout << "-fpa (address)        Set the address from where flash policy file is served. Defaults to 0.0.0.0" << endl;
            cout << "                      Flash policy serving is turned on whenever HTTP service is turned on." << endl;
            cout << "--flashpolicyport (port)" << endl;
            cout << "-fpp (port)           Set the port from where flash policy file is served. Defaults to 8081" << endl;
            cout << "--http                Enable HTTP service." << endl;
            cout << "--version" << endl;
            cout << "-v                    Show version information and exit." << endl << endl;
            cout << "--logfile (log file)  Specify where dfterm2 saves its log. Defaults to dfterm2.log" << endl;
            cout << "--disablelog          Do not log to a file." << endl;
            cout << "--database (database file)" << endl;
            cout << "-db (database file)   Set the database file used. By default dfterm2 will try to look for" << endl;
            cout << "                      dfterm2_database.sqlite3 as database." << endl;
            cout << "--conf (conf file)    Set the configuration file to be used. Defaults to dfterm2.conf." << endl;
            cout << "                      Configuration file is a lua script, that should set variables" << endl;
            cout << "                      \"database\" and \"logfile\" to global environment pointing to the files" << endl;
            cout << "                      to be used. Configuration file is superseded by --database and --log-file" << endl;
            cout << "                      settings. The strings are interpreted as UTF-8." << endl;
            #ifdef _WIN32
            cout << "--create-appdir       Creates %APPDIR%\\dfterm2 on start-up, if the" << endl;
            cout << "                      directory does not exist. Windows only." << endl;
            #endif
            cout << "--help"  << endl;;
            cout << "-h"  << endl;
            cout << "-?                    Show this help." << endl;
            cout << endl;
            cout << "Examples:" << endl << endl;
            cout << "Listen on port 8001:" << endl;
            cout << "dfterm2 --port 8001" << endl;
            cout << "Listen on an IPv6 localhost:" << endl;
            cout << "dfterm2 --address ::1" << endl;
            cout << "Use another database instead of the default one: " << endl;
            cout << "dfterm2 --db my_great_dfterm2_database.sqlite3" << endl << endl;
            cout << "Take it easy!!" << endl;
            cout << endl;
            return 0;
        }
    }

    if (!use_luaconf_logfile && !use_luaconf_database)
        cout << "Will not run configuration file, because both log file and database were specified on command line." << endl;
    else
    {
        string logfile2 = TO_UTF8(log_file), database2 = database_file;
        if (readConfigurationFile(conffile, &logfile2, &database2))
        {
            if (use_luaconf_logfile)
                log_file = TO_UNICODESTRING(logfile2);
            else
                cout << "Will not use configuration specified log file " << logfile2 << " because log file was specified on command line." << endl;
            if (use_luaconf_database)
                database_file = database2;
            else
                cout << "Will not use configuration specified database file " << database2 << " because database file was specified on command line." << endl;
        }
    }

    #ifdef _WIN32
    if (create_app_dir)
    {
        WCHAR AppData[] = L"APPDATA";
        WCHAR result_str[32868]; /* Max size for env variable is 32767,
                                but leave some extra space there,
                                because we append to it. */
        memset(result_str, 0, 32868 * sizeof(WCHAR));

        DWORD result = GetEnvironmentVariableW(AppData, result_str, 32767);
        if (!result)
        { LOG(Error, "Could not fetch environment variable APPDATA with GetLastError() == " << GetLastError()); }
        else
        {
            wcscpy(&result_str[result], L"\\dfterm2");

            BOOL success = CreateDirectoryW(result_str, NULL);
            if (success)
            { LOG(Note, "Created directory " << TO_UTF8(result_str, result+8)); }
            else
            {
                DWORD errorcode = GetLastError();
                if (errorcode == ERROR_ALREADY_EXISTS)
                { LOG(Note, "Will not create " << TO_UTF8(result_str, result+8) << " because the directory already exists."); }
                else
                { LOG(Error, "Could not create directory " << TO_UTF8(result_str, result+8) << " with GetLastError() == " << GetLastError()); };
            }
        }
    }
    #endif

    if (log_file.length() > 0)
        cout << "Logging to file " << TO_UTF8(log_file) << endl;

    LOG(Note, "Starting up dfterm2.");

    SocketAddress::resolve(address, port, resolve_binding, true);
    if (!succeeded_resolve)
    {
        flush_messages();
        cerr << "Resolving [" << address << "]:" << port << " failed. Check your listening address settings." << endl;
        return -1;
    }

    if (use_http_service)
    {
        SocketAddress::resolve(httpaddress, httpport, http_resolve_binding, true);
        if (!succeeded_resolve)
        {
            flush_messages();
            cerr << "Resolving [" << httpaddress << "]:" << httpport << " failed. Check your listening address settings for HTTP." << endl;
            return -1;
        }
        SocketAddress::resolve(flashpolicyaddress, flashpolicyport, flash_resolve_binding, true);
        if (!succeeded_resolve)
        {
            flush_messages();
            cerr << "Resolving [" << flashpolicyaddress << "]:" << flashpolicyport << " failed. Check your listening address settings for flash policy." << endl;
            return -1;
        }
    }

    SP<State> state = State::createState();
    if (!state->setDatabaseUTF8(database_file))
    {
        cerr << "Failed to set database." << endl;
        flush_messages();
        return -1;
    }
    LOG(Note, "Using database " << database_file);

    if (!state->addTelnetService(listen_address))
    {
        flush_messages();
        cerr << "Could not add a telnet service. " << endl;
        return -1;
    }
    if (use_http_service && !state->addHTTPService(http_listen_address, flash_policy_listen_address, httpconnectaddress))
    {
        flush_messages();
        cerr << "Could not add an HTTP service. " << endl;
        return -1;
    }

    state->loop();
    return 0;
}

