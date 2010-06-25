#ifndef slot_terminal_hpp
#define slot_terminal_hpp

#include "slot.hpp"
#include "types.hpp"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <string>
#include <deque>
#include "termemu.h"
#include "pty.h"

namespace dfterm
{

class TerminalGlue : public Slot
{
    private:
        bool alive;
        boost::recursive_mutex glue_mutex;
        SP<boost::thread> glue_thread;

        Terminal game_terminal;
        recursive_mutex game_terminal_mutex;

        std::map<string, UnicodeString> parameters;

        std::deque<pair<ui32, bool> > input_queue;
        void flushInput(Pty* program_pty);

        ui32 terminal_w, terminal_h;

        volatile bool close_thread;

        static void static_thread_function(TerminalGlue* self);
        void thread_function();

        static void pushEscapeSequence(KeyCode special_key, string &input_buf);
    public:
        TerminalGlue();
        ~TerminalGlue();

        void setParameter(std::string key, UnicodeString value);

        void getSize(ui32* width, ui32* height);
        bool isAlive();
        void unloadToWindow(SP<Interface2DWindow> target_window);
        void feedInput(ui32 keycode, bool special_key);
};

}; /* namespace */

#endif

