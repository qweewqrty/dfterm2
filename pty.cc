#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <iostream>
#include <errno.h>
#include "pty.h"
#include "bsd_pty.h"
#include <termios.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "logger.hpp"

Pty::Pty()
{
    MasterPty = -1;
    ChildPid = -1;
};

Pty::~Pty()
{
    terminate();
};

void Pty::terminate()
{
    if (ChildPid > 0)
    {
        int status;
        
        if (waitpid(ChildPid, &status, WNOHANG) == 0)
        {
            kill(ChildPid, SIGKILL);
            waitpid(ChildPid, &status, 0);
        }
        
        ChildPid = -1;
    };
    
    if (MasterPty >= 0)
        close(MasterPty);
    MasterPty = -1;
};

bool Pty::launchExecutable(int term_width, int term_height, const char* executable, char* argv[], char* const env[], const char* workingdirectory)
{
    if (MasterPty >= 0)
        return false;
    
    struct winsize ws;
    struct termios to;
    
    ioctl(0, TIOCGWINSZ, &ws);
    tcgetattr(0, &to);
    ws.ws_row = term_height;
    ws.ws_col = term_width;
    
    ChildPid = forkpty(&MasterPty, NULL, &to, &ws);
    if (ChildPid < 0)
        return false;
    if (ChildPid == 0)
    {
        setsid();
        
        ws.ws_row = term_height;
        ws.ws_col = term_width;
        ioctl(0, TIOCSWINSZ, &ws);
        
        if (workingdirectory)
            chdir(workingdirectory);
        
        if (env)
            execve(executable, argv, env);
        else
            execv(executable, argv);
        abort();
    };
    
    return true;
};

bool Pty::isClosed()
{
    if (MasterPty < 0)
        return true;
    return false;
}

bool Pty::poll()
{
    if (MasterPty < 0)
        return true;
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    fd_set ptyset;
    FD_ZERO(&ptyset);
    FD_SET(MasterPty, &ptyset);
    
    int result = 0;
    do
    {
        result = select(FD_SETSIZE, &ptyset, NULL, NULL, &tv);
        if (result < 0 && errno != EINTR)
        {
            terminate();
            return true;
        }
        else if (result < 0 && errno == EINTR)
            continue;
        break;
    } while(true);

    if (result == 0)
        return false;
    
    return true;
}

int Pty::feed(const char* data, size_t len)
{
    if (MasterPty < 0)
        return -1;
        
    int result = write(MasterPty, data, len);
    if (result < 0)
    {
        terminate();
        return -1;
    }
    else if (result == 0)
    {
        terminate();
        return 0;
    }
    return result;
};

int Pty::fetch(char* data, size_t n)
{
    if (MasterPty < 0)
        return -1;
    
    int result = read(MasterPty, data, n);
    if (result < 0)
    {
        terminate();
        return -1;
    }
    else if (result == 0)
    {
        terminate();
        return 0;
    }
    
    return result;
}

pid_t Pty::getPID() const
{
    return ChildPid;
}

