#include "types.hpp"
#include "slot_linux_common.hpp"
#include <vector>
#include <cstring>
#include "logger.hpp"

using namespace dfterm;
using namespace boost;
using namespace trankesbel;
using namespace std;

/* Fetches "w", "h", "path" and "work"
   from slot settings and then launches a process.
   Returns true if that succeeded and false if it didn't.
   Times out after 60 seconds. */
bool dfterm::waitAndLaunchProcess(pid_t* pid_p,
                                  recursive_mutex* glue_mutex_p, 
                                  volatile bool* close_thread_p, 
                                  std::map<std::string, UnicodeString>* parameters_p,
                                  Pty* program_pty_p,
                                  ui32* terminal_w,
                                  ui32* terminal_h)
{
    assert(glue_mutex_p);
    assert(close_thread_p);
    assert(parameters_p);
    assert(program_pty_p);

    recursive_mutex &glue_mutex = *glue_mutex_p;
    volatile bool& close_thread = *close_thread_p;
    std::map<std::string, UnicodeString>& parameters = *parameters_p;
    Pty& program_pty = *program_pty_p;

    /* Wait until "path", "work", "w" and "h" are set. 60 seconds. */
    ui8 counter = 60;
    map<string, UnicodeString>::iterator i1;

    vector<string> arguments;
    string next_arg("arg0");
    ui32 num_arg = 0;

    while (counter > 0)
    {
        unique_lock<recursive_mutex> lock(glue_mutex);
        if (close_thread)
        {
            lock.unlock();
            return false;
        }

        // Collect arguments to 'arguments' vector
        while (parameters.find(next_arg) != parameters.end())
        {
            arguments.push_back(TO_UTF8(parameters[next_arg]));
            stringstream next_arg_ss;
            next_arg_ss << "arg" << (++num_arg);
            next_arg = next_arg_ss.str();
        }

        if (parameters.find("path") != parameters.end() &&
            parameters.find("work") != parameters.end() &&
            parameters.find("w") != parameters.end() &&
            parameters.find("h") != parameters.end())
            break;
        lock.unlock();
        --counter;
        sleep(1);
    }
    if (counter == 0)
        return false;

    unique_lock<recursive_mutex> ulock(glue_mutex);
    UnicodeString path = parameters["path"];
    UnicodeString work_dir = parameters["work"];
    string path_str = TO_UTF8(path);
    string work_dir_str = TO_UTF8(work_dir);

    /* Get width and height from parameters, default to 80x25 */
    *terminal_w = 80;
    *terminal_h = 25;
    i1 = parameters.find("w");
    if (i1 != parameters.end())
    {
        string n = TO_UTF8(i1->second);
        *terminal_w = (ui32) strtol(n.c_str(), NULL, 10);
        if (*terminal_w == 0) *terminal_w = 1;
        if (*terminal_w > 500) *terminal_w = 500;
    }
    i1 = parameters.find("h");
    if (i1 != parameters.end())
    {
        string n = TO_UTF8(i1->second);
        *terminal_h = (ui32) strtol(n.c_str(), NULL, 10);
        if (*terminal_h == 0) *terminal_h = 1;
        if (*terminal_h > 500) *terminal_h = 500;
    }

    char **argv;
    argv = new char*[arguments.size() + 2];
    memset(argv, 0, sizeof(char*) * (arguments.size() + 2));
    argv[0] = new char[path_str.size()+1];
    argv[0][path_str.size()] = 0;
    memcpy(argv[0], path_str.c_str(), path_str.size());

    stringstream logmessage;

    logmessage << "Launching a program \"" << path_str.c_str() << "\" in a pty, with terminal size " << *terminal_w << "x" << *terminal_h << " and with following arguments: ";
    for (ui32 i1 = 0; i1 < arguments.size(); ++i1)
    {
        argv[i1+1] = new char[arguments[i1].size()+1];
        argv[i1+1][arguments[i1].size()] = 0;
        memcpy(argv[i1+1], arguments[i1].c_str(), arguments[i1].size());
        logmessage << "\"" << argv[i1+1] << "\"";
        if (i1 < arguments.size() - 1)
            logmessage << " ";
    }
    LOG(Note, logmessage.str());

    bool result = program_pty.launchExecutable(*terminal_w, *terminal_h, path_str.c_str(), argv, (char**) 0, work_dir_str.c_str());
    for (ui32 i1 = 0; i1 < arguments.size()+1; ++i1)
    {
        delete[] argv[i1];
    }
    delete[] argv;

    if (result)
        (*pid_p) = program_pty.getPID();

    return result;
}

