/* Stolen from old dfterm.  */

#ifndef pty_h
#define pty_h

#include <stdlib.h>

// Full-duplex simple pty interface based on forkpty().
class Pty
{
    private:
        // No copies
        Pty(const Pty &p) { abort(); };
        Pty& operator=(const Pty &p) { abort(); return (*this); };
        
        int MasterPty;
        pid_t ChildPid;
        
    public:
        Pty();
        // Destructor forcibly closes the pty.
        ~Pty();
        
        // Creates a pty, executes a process (see execvp) and attaches it to the pty.
        bool launchExecutable(int term_w, int term_h, const char* executable, char* argv[], char* const env[] = (char**) 0, const char* workingdirectory = (char*) 0);
        // Forcibly closes the pty.
        void terminate();
        
        int getMasterPty() const { return MasterPty; };
        
        // Return true if pty has been closed.
        bool isClosed();
        
        // Returns if there's some data waiting to be read from master pty.
        // (You can also use select on the handle from getMasterPty).
        // If pty is in an erroneus state or closed, always returns true.
        bool poll();
        
        // Writes data to pty. Returns the number of characters written.
        // Returns 0 if pty has been closed and -1 if pty is in an erroneus state.
        int feed(const char* data, size_t length);
        // Reads at most 'n' bytes of data from pty. If no data is waiting, blocks.
        // Returns the number of bytes read or 0 if pty has been closed and -1
        // if pty is in an erroneus state.
        int fetch(char* data, size_t n);

        // Returns the pid of the process. Return value is undefined if pty is not active.
        pid_t getPID() const;
        
        // Returns the character needed for CTRL+character. Returns 0 if character
        // is invalid in some way. 0 may also be a valid result for some keys...
        static unsigned char controlCharacter(unsigned char c)
        {
            // Lowercase
            if (c >= 'A' && c <= 'Z')
            {
                c = c - 'A' + 1;
                return c;
            }
            else if (c >= 'a' && c <= 'z')
            {
                c = c - 'a' + 1;
                return c;
            }
            
            if (c == '@') c = 0;
            else if (c == '`') c = 0;
            else if (c == '[') c = 27;
            else if (c == '{') c = 27;
            else if (c == '\\') c = 28;
            else if (c == '|') c = 28;
            else if (c == ']') c = 29;
            else if (c == '}') c = 29;
            else if (c == '^') c = 30;
            else if (c == '~') c = 30;
            else if (c == '_') c = 31;
            else if (c == '?') c = 127;
            else return 0;
            
            return c;
        }
};

#endif

