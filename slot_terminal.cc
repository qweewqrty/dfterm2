#include "slot_terminal.hpp"
#include "pty.h"
#include "types.hpp"
#include <unistd.h>
#include "nanoclock.hpp"
#include "interface.hpp"
#include "interface_ncurses.hpp"
#include <iostream>

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
    self->thread_function();
}

void TerminalGlue::pushEscapeSequence(KeyCode special_key, string &input_buf)
{
    switch(special_key)
    {
        case AUp:        input_buf.append("\x1b\x4f\x41"); break;
        case ADown:      input_buf.append("\x1b\x4f\x42"); break;
        case ARight:     input_buf.append("\x1b\x4f\x43"); break;
        case ALeft:      input_buf.append("\x1b\x4f\x44"); break;
        case CtrlUp:     input_buf.append("\x1b\x4f\x41"); break;
        case CtrlDown:   input_buf.append("\x1b\x4f\x42"); break;
        case CtrlRight:  input_buf.append("\x1b\x4f\x43"); break;
        case CtrlLeft:   input_buf.append("\x1b\x4f\x44"); break;
        case F1:         input_buf.append("\x1b\x5b\x31\x31\x7e"); break;
        case F2:         input_buf.append("\x1b\x5b\x31\x32\x7e"); break;
        case F3:         input_buf.append("\x1b\x5b\x31\x33\x7e"); break;
        case F4:         input_buf.append("\x1b\x5b\x31\x34\x7e"); break;
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
    lock_guard<recursive_mutex> lock(glue_mutex);
    if (input_queue.size() == 0) return;

    string input_buf;
    input_buf.reserve(input_queue.size());

    while(input_queue.size() > 0)
    {
        ui32 keycode = input_queue.front().first;
        bool special_key = input_queue.front().second;
        input_queue.pop_front();

        if (special_key) { pushEscapeSequence((KeyCode) keycode, input_buf); continue; }

        if (keycode == '\n')
        {
            input_buf.append("\r");
            continue;
        }

        if (keycode > 127)
        {
            UChar32 uni32 = (UChar32) keycode;
            UChar uni16[] = { 0, 0, 0 };
            UErrorCode uerror = U_ZERO_ERROR;
            u_strFromUTF32(uni16, 2, NULL, &uni32, 1, &uerror);
            if (U_FAILURE(uerror)) continue;

            char uni8[] = { 0, 0, 0, 0, 0 };

            u_strToUTF8(uni8, 4, NULL, uni16, -1, &uerror);
            if (U_FAILURE(uerror)) continue;

            input_buf.append(uni8);
            continue;
        }

        ui8 k = (ui8) keycode;
        input_buf.push_back(k);
    }

    if (input_buf.size() > 0)
        program_pty->feed(input_buf.c_str(), input_buf.size());
}

void TerminalGlue::thread_function()
{
    /* Wait until "path", "work", "w" and "h" are set. 60 seconds. */
    ui8 counter = 60;
    map<string, UnicodeString>::iterator i1;
    while (counter > 0)
    {
        unique_lock<recursive_mutex> lock(glue_mutex);
        if (close_thread)
        {
            lock.unlock();
            return;
        }

        if (parameters.find("path") != parameters.end() &&
            parameters.find("work") != parameters.end() &&
            parameters.find("w") != parameters.end() &&
            parameters.find("h") != parameters.end())
            break;
        lock.unlock();
        counter--;
        sleep(1);
    }
    if (counter == 0)
    {
        lock_guard<recursive_mutex> lock(glue_mutex);
        alive = false;
        return;
    }

    unique_lock<recursive_mutex> ulock(glue_mutex);
    Pty program_pty;
    UnicodeString path = parameters["path"];
    UnicodeString work_dir = parameters["work"];
    string path_str = TO_UTF8(path);
    string work_dir_str = TO_UTF8(work_dir);

    /* Get width and height from parameters, default to 80x25 */
    terminal_w = 80;
    terminal_h = 25;
    i1 = parameters.find("w");
    if (i1 != parameters.end())
    {
        string n = TO_UTF8(i1->second);
        terminal_w = (ui32) strtol(n.c_str(), NULL, 10);
        if (terminal_w == 0) terminal_w = 1;
        if (terminal_w > 500) terminal_w = 500;
    }
    i1 = parameters.find("h");
    if (i1 != parameters.end())
    {
        string n = TO_UTF8(i1->second);
        terminal_h = (ui32) strtol(n.c_str(), NULL, 10);
        if (terminal_h == 0) terminal_h = 1;
        if (terminal_h > 500) terminal_h = 500;
    }

    char *argv[] = { const_cast<char*>(path_str.c_str()), 0 };

    bool result = program_pty.launchExecutable(terminal_w, terminal_h, path_str.c_str(), argv, (char**) 0, work_dir_str.c_str());
    if (!result)
    {
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
     * 30 times per second and then check the status of those both. */
    ulock.unlock();
    while (!program_pty.isClosed() && !close_thread)
    {
        unique_lock<recursive_mutex> ulock(glue_mutex);
        /* New input? */
        flushInput(&program_pty);
        /* New output? */
        while (program_pty.poll())
        {
            char buf[1000];
            int data = program_pty.fetch(buf, 1000);
            if (data <= 0)
            {
                program_pty.terminate();
                break;
            }

            unique_lock<recursive_mutex> lock2(game_terminal_mutex);
            game_terminal.feedString(buf, data);
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
        nanowait(1000000000LL / 30);
    }

    unique_lock<recursive_mutex> ulock2(glue_mutex);
    alive = false;
}

void TerminalGlue::setParameter(string key, UnicodeString value)
{
    lock_guard<recursive_mutex> lock(glue_mutex);
    if (key == "path" || key == "work" || key == "w" || key == "h")
        parameters[key] = value;
}

void TerminalGlue::feedInput(ui32 keycode, bool special_key)
{
    lock_guard<recursive_mutex> lock(glue_mutex);
    input_queue.push_back(pair<ui32, bool>(keycode, special_key));
}

void TerminalGlue::getSize(ui32* width, ui32* height)
{
    lock_guard<recursive_mutex> lock(glue_mutex);
    (*width) = terminal_w;
    (*height) = terminal_h;
}

void TerminalGlue::unloadToWindow(SP<Interface2DWindow> target_window)
{
    if (!target_window) return;

    lock_guard<recursive_mutex> lock(game_terminal_mutex);

    target_window->setMinimumSize(terminal_w, terminal_h);
    CursesElement* elements = new CursesElement[terminal_w * terminal_h];
    ui32 i1, i2;
    ui32 cursor_x = game_terminal.getCursorX();
    ui32 cursor_y = game_terminal.getCursorY();
    for (i1 = 0; i1 < terminal_w; i1++)
        for (i2 = 0; i2 < terminal_h; i2++)
        {
            const TerminalTile &t = game_terminal.getTile(i1, i2);
            ui32 symbol = t.getSymbol();
            ui32 fore_c = t.getForegroundColor();
            ui32 back_c = t.getBackgroundColor();
            if (fore_c == 9) fore_c = 7;
            if (back_c == 9) back_c = 0;
            ui32 temp;
            if (cursor_x == i1 && cursor_y == i2)
            {
                temp = fore_c;
                fore_c = back_c;
                back_c = temp;
            }
            elements[i1 + i2 * terminal_w] = CursesElement(symbol, (Color) fore_c, (Color) back_c, t.getBold());
        }
    target_window->setScreenDisplayNewElements(elements, sizeof(CursesElement), terminal_w, terminal_w, terminal_h, 0, 0);
    delete[] elements;
}

