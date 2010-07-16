#ifndef slot_terminal_hpp
#define slot_terminal_hpp

#include "interface.hpp"
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
        boost::recursive_mutex game_terminal_mutex;

        std::map<std::string, UnicodeString> parameters;

        std::deque<trankesbel::KeyPress> input_queue;
        void flushInput(Pty* program_pty);

        trankesbel::ui32 terminal_w, terminal_h;

        volatile bool close_thread;

        static void static_thread_function(TerminalGlue* self);
        void thread_function();

        static void pushEscapeSequence(trankesbel::KeyCode special_key, std::string &input_buf);
    public:
        TerminalGlue();
        ~TerminalGlue();

        void setParameter(std::string key, UnicodeString value);

        void getSize(trankesbel::ui32* width, trankesbel::ui32* height);
        bool isAlive();
        void unloadToWindow(SP<trankesbel::Interface2DWindow> target_window);
        void feedInput(const trankesbel::KeyPress &kp);
};

}; /* namespace */

#endif

