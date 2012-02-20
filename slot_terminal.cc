#include "slot_terminal.hpp"
#include "pty.h"
#include "types.hpp"
#include <unistd.h>
#include "nanoclock.hpp"
#include "interface.hpp"
#include "interface_ncurses.hpp"
#include <iostream>

#include "slot_linux_common.hpp"

#include "bsd_pty.h"

#ifdef __WIN32
#error "Terminal (pty) glue does not work on Windows."
#endif

using namespace dfterm;
using namespace boost;
using namespace std;
using namespace trankesbel;

TerminalGlue::TerminalGlue() : Slot()
{
    close_thread = false;
    alive = true;

    terminal_w = 80;
    terminal_h = 25;
    old_w = old_h = 0;

    try_resize_again = true;

    glue_thread = SP<thread>(new thread(static_thread_function, this));
    if (glue_thread->get_id() == thread::id())
        alive = false;
}

TerminalGlue::~TerminalGlue()
{
    unique_lock<recursive_mutex> lock(glue_mutex);
    close_thread = true;
    lock.unlock();

    if (glue_thread)
        glue_thread->join();
}

bool TerminalGlue::isAlive()
{
    lock_guard<recursive_mutex> lock(glue_mutex);
    bool alive_bool = alive;
    return alive_bool;
}

void TerminalGlue::static_thread_function(TerminalGlue* self)
{
    assert(self);
    self->thread_function();
}

void TerminalGlue::pushEscapeSequence(KeyCode special_key, string &input_buf)
{
    switch(special_key)
    {
        case Enter:      input_buf.append("\r"); break;
        case AUp:        input_buf.append("\x1b\x4f\x41"); break;
        case ADown:      input_buf.append("\x1b\x4f\x42"); break;
        case ARight:     input_buf.append("\x1b\x4f\x43"); break;
        case ALeft:      input_buf.append("\x1b\x4f\x44"); break;

        case AltUp:      input_buf.append("\x1b\x1b\x4f\x41"); break;
        case AltDown:    input_buf.append("\x1b\x1b\x4f\x42"); break;
        case AltRight:   input_buf.append("\x1b\x1b\x4f\x43"); break;
        case AltLeft:    input_buf.append("\x1b\x1b\x4f\x44"); break;

        case CtrlUp:     input_buf.append("\x1b\x4f\x41"); break;
        case CtrlDown:   input_buf.append("\x1b\x4f\x42"); break;
        case CtrlRight:  input_buf.append("\x1b\x4f\x43"); break;
        case CtrlLeft:   input_buf.append("\x1b\x4f\x44"); break;
        case F1:         input_buf.append("\x1b\x4fP"); break;
        case F2:         input_buf.append("\x1b\x4fQ"); break;
        case F3:         input_buf.append("\x1b\x4fR"); break;
        case F4:         input_buf.append("\x1b\x4fS"); break;
        case F5:         input_buf.append("\x1b\x5b\x31\x35\x7e"); break;
        case F6:         input_buf.append("\x1b\x5b\x31\x37\x7e"); break;
        case F7:         input_buf.append("\x1b\x5b\x31\x38\x7e"); break;
        case F8:         input_buf.append("\x1b\x5b\x31\x39\x7e"); break;
        case F9:         input_buf.append("\x1b\x5b\x32\x30\x7e"); break;
        case F10:        input_buf.append("\x1b\x5b\x32\x31\x7e"); break;
        case F11:        input_buf.append("\x1b\x5b\x32\x32\x7e"); break;
        case F12:        input_buf.append("\x1b\x5b\x32\x33\x7e"); break;
        case Home:       input_buf.append("\x1b\x5b\x31\x7e"); break;
        case InsertChar: input_buf.append("\x1b\x5b\x32\x7e"); break;
        case DeleteChar: input_buf.append("\x1b\x5b\x33\x7e"); break;
        case End:        input_buf.append("\x1b\x5b\x34\x7e"); break;
        case PgDown:     input_buf.append("\x1b\x5b\x36\x7e"); break;
        case PgUp:       input_buf.append("\x1b\x5b\x35\x7e"); break;
        default: break;
    }
}

void TerminalGlue::flushInput(Pty* program_pty)
{
    assert(program_pty);

    lock_guard<recursive_mutex> lock(glue_mutex);
    if (input_queue.size() == 0) return;

    string input_buf;
    input_buf.reserve(input_queue.size());

    while(input_queue.size() > 0)
    {
        KeyPress kp = input_queue.front();

        ui32 keycode = (ui32) input_queue.front().getKeyCode();
        bool special_key = input_queue.front().isSpecialKey();

        input_queue.pop_front();

        if (kp.isAltDown())
            input_buf.push_back('\x1b');
            
        if (special_key) { pushEscapeSequence((KeyCode) keycode, input_buf); continue; }

        if (keycode == '\n')
        {
            input_buf.append("\r");
            continue;
        }

        if (kp.isCtrlDown() && keycode >= 'A' && keycode < 'Z')
        {
            input_buf.push_back(keycode - 'A' + 1);
            continue;
        }

        /* ought to be enough to encode a unicode code point in
         * all locales we care about. */
        char localized_key[10];
        memset(localized_key, 0, 10);
        UErrorCode errorcode = U_ZERO_ERROR;

        UnicodeString us((UChar32) keycode);
        us.extract(localized_key, 9, NULL, errorcode);
        localized_key[9] = 0;

        if (U_FAILURE(errorcode))
            continue;

        ui8 k = (ui8) keycode;
        for (size_t i1 = 0; localized_key[i1]; ++i1)
            input_buf.push_back(localized_key[i1]);
    }


    if (input_buf.size() > 0)
        program_pty->feed(input_buf.c_str(), input_buf.size());
}

void TerminalGlue::thread_function()
{
    Pty program_pty;
    int terminated = 0;

    pid_t pid;
    if (!waitAndLaunchProcess(&pid, &glue_mutex, &close_thread, &parameters, &program_pty, &terminal_w, &terminal_h))
    {
        lock_guard<recursive_mutex> lock(glue_mutex);
        alive = false;
        return;
    }

    unique_lock<recursive_mutex> lock3(game_terminal_mutex);
    game_terminal.resize(terminal_w, terminal_h);
    lock3.unlock();

    /* There is a situation where, where we need to wait on two things at once:
     * 1. Data read directly from pty (blocking POSIX poll())
     * 2. Data from TerminalGlue::feedInput (blocking boost condition variable wait)
     *
     * As far as I know, I'd need one additional thread or two to be able to wait on both.
     * So for now, let's go with ticking system like on slot_dfglue.cc. We tick, say,
     * 60 times per second and then check the status of those both. */
    while (!program_pty.isClosed() && !close_thread)
    {
        unique_lock<recursive_mutex> ulock(glue_mutex);
        /* New input? */
        flushInput(&program_pty);
        /* New output? */
        while (program_pty.poll())
        {
            string read_buffer;
            while(program_pty.poll())
            {
                char buf[1000];
                int data = program_pty.fetch(buf, 1000);
                if (data <= 0)
                {
                    program_pty.terminate();
                    terminated = 1;
                    break;
                }
                read_buffer.append(buf, data);
            }
            if (terminated)
                break;

            UErrorCode errorcode = U_ZERO_ERROR;
            UnicodeString converted(read_buffer.data(), read_buffer.size(),
                                    NULL, errorcode);

            if (U_FAILURE(errorcode))
            {
                printf("%d\n", errorcode);
                converted = UnicodeString();
            }

            read_buffer.clear();
            converted.toUTF8String(read_buffer);

            unique_lock<recursive_mutex> lock2(game_terminal_mutex);
            game_terminal.feedString(read_buffer.data(), read_buffer.size());
            lock2.unlock();
            SP<State> s = state.lock();
            if (s)
            {
                SP<Slot> self_sp = self.lock();
                ulock.unlock();
                s->signalSlotData(self_sp);
                ulock.lock();
            }
        }

        ulock.unlock();
        nanowait(1000000000LL / 60);
    }

    unique_lock<recursive_mutex> ulock2(glue_mutex);
    alive = false;
}

void TerminalGlue::setParameter(string key, UnicodeString value)
{
    lock_guard<recursive_mutex> lock(glue_mutex);
    if (key == "path" || key == "work" || key == "w" || key == "h" || key.substr(0, 3) == "arg")
        parameters[key] = value;
}

void TerminalGlue::feedInput(const KeyPress &kp)
{
    lock_guard<recursive_mutex> lock(glue_mutex);
    input_queue.push_back(kp);
}

void TerminalGlue::getSize(ui32* width, ui32* height)
{
    assert(width && height);

    lock_guard<recursive_mutex> lock(glue_mutex);
    (*width) = terminal_w;
    (*height) = terminal_h;
}

void TerminalGlue::unloadToWindow(SP<Interface2DWindow> target_window)
{
    assert(target_window);

    lock_guard<recursive_mutex> lock(game_terminal_mutex);
    ui32 t_w = min(terminal_w, (ui32) game_terminal.getWidth());
    ui32 t_h = min(terminal_h, (ui32) game_terminal.getHeight());

    /*
    Disable this functionality for now.
    if (old_w != t_w || old_h != t_h)
        try_resize_again = true;
    */
    try_resize_again = true;
    old_w = t_w;
    old_h = t_h;

    if (try_resize_again)
    {
        target_window->setMinimumSize(t_w, t_h);
        try_resize_again = false;
    }

    ui32 actual_window_w, actual_window_h;
    target_window->getSize(&actual_window_w, &actual_window_h);

    ui32 game_offset_x, game_offset_y;
    game_offset_x = game_offset_y = 0;
    if (actual_window_w > t_w)
        game_offset_x = (actual_window_w - t_w) / 2;
    if (actual_window_h > t_h)
        game_offset_y = (actual_window_h - t_h) / 2;

    CursesElement* elements = new CursesElement[actual_window_w * actual_window_h];
    ui32 i1, i2;
    ui32 cursor_x = game_terminal.getCursorX();
    ui32 cursor_y = game_terminal.getCursorY();
    for (i1 = 0; i1 < actual_window_w; ++i1)
        for (i2 = 0; i2 < actual_window_h; ++i2)
        {
            TerminalTile t;
            
            if (i1 < game_offset_x || i2 < game_offset_y || (i1 - game_offset_x) >= t_w || (i2 - game_offset_y) >= t_h)
                t = TerminalTile(' ', 7, 0, false, false);
            else
                t = game_terminal.getTile(i1 - game_offset_x, i2 - game_offset_y);

            ui32 symbol = t.getSymbol();
            ui32 fore_c = t.getForegroundColor();
            ui32 back_c = t.getBackgroundColor();
            if (fore_c == 9) fore_c = 7;
            if (back_c == 9) back_c = 0;
            ui32 temp;
            if (cursor_x == i1 - game_offset_x && cursor_y == i2 - game_offset_y && game_terminal.isCursorVisible())
            {
                temp = fore_c;
                fore_c = back_c;
                back_c = temp;
            }
            // Flip colors in case of inversed element
            if (t.getInverse())
            {
                temp = fore_c;
                fore_c = back_c;
                back_c = temp;
            }
            elements[i1 + i2 * actual_window_w] = CursesElement(symbol, (Color) fore_c, (Color) back_c, t.getBold());
        }
    target_window->setScreenDisplayNewElements(elements, sizeof(CursesElement), actual_window_w, actual_window_w, actual_window_h, 0, 0);
    delete[] elements;
}

