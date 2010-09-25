/* DFHack slot type. Code initially copied from slot_dfglue.cc */

#include "slot_dfhack_linux.hpp"

#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <iostream>
#include "nanoclock.hpp"
#include "interface_ncurses.hpp"
#include "cp437_to_unicode.hpp"
#include <algorithm>
#include <cstring>

#include "slot_linux_common.hpp"

#include "pty.h"

#include "state.hpp"

using namespace dfterm;
using namespace boost;
using namespace std;
using namespace trankesbel;

/* A simple checksum function. */
static ui32 checksum(const char* buf, size_t buflen)
{
    assert(!buflen || (buflen && buf));

    ui32 a = 1, b = buflen;
    size_t i1;
    for (i1 = 0; i1 < buflen; ++i1)
    {
        a += buf[i1];
        b += (buflen - i1) * (ui32) buf[i1];
    }

    a %= 65521;
    b %= 65521;

    return a + b * 65536;
}

static ui32 checksum(string str)
{ return checksum(str.c_str(), str.size()); };

DFHackSlotLinux::DFHackSlotLinux() : Slot()
{
    df_terminal.setCursorVisibility(false);
    close_thread = false;
    data_format = Unknown;

    dont_take_running_process = false;

    df_w = 80;
    df_h = 25;

    ticks_per_second = 30;

    alive = true;

    df_context = (DFHack::Context*) 0;
    df_position_module = (DFHack::Position*) 0;
    df_contextmanager = new DFHack::ContextManager("Memory.xml");

    // Create a thread that will launch or grab the DF process.
    glue_thread = SP<thread>(new thread(static_thread_function, this));
    if (glue_thread->get_id() == thread::id())
        alive = false;
};

DFHackSlotLinux::DFHackSlotLinux(bool dummy) : Slot()
{
    df_terminal.setCursorVisibility(false);

    close_thread = false;
    data_format = Unknown;

    dont_take_running_process = true;

    df_w = 80;
    df_h = 25;

    ticks_per_second = 20;

    alive = true;

    df_context = (DFHack::Context*) 0;
    df_contextmanager = new DFHack::ContextManager("Memory.xml");

    // Create a thread that will launch or grab the DF process.
    glue_thread = SP<thread>(new thread(static_thread_function, this));
    if (glue_thread->get_id() == thread::id())
        alive = false;
}

DFHackSlotLinux::~DFHackSlotLinux()
{
    if (glue_thread)
        glue_thread->interrupt();

    unique_lock<recursive_mutex> lock(glue_mutex);
    close_thread = true;
    lock.unlock();

    if (glue_thread)
        glue_thread->join();

    if (df_contextmanager) delete df_contextmanager;
}

bool DFHackSlotLinux::isAlive()
{
    unique_lock<recursive_mutex> alive_try_lock(glue_mutex, try_to_lock_t());
    if (!alive_try_lock.owns_lock()) return true;

    bool alive_bool = alive;
    return alive_bool;
}

bool DFHackSlotLinux::launchDFProcess(Pty* program_pty, pid_t *pid)
{
    assert(program_pty);
    assert(pid);

    ui32 w1, w2;

    bool result = dfterm::waitAndLaunchProcess(pid, &glue_mutex, &close_thread, &parameters, program_pty, &w1, &w2);
    return result;
}

void DFHackSlotLinux::static_thread_function(DFHackSlotLinux* self)
{
    assert(self);
    self->thread_function();
}

void DFHackSlotLinux::thread_function()
{
    assert(!df_context);
    assert(df_contextmanager);

    bool poll_pty = false;
    Pty program_pty;

    if (!dont_take_running_process)
    {
        try
        {
            df_contextmanager->Refresh(NULL);
            df_context = df_contextmanager->getSingleContext();
            df_context->Attach();
            df_position_module = df_context->getPosition();
        }
        catch (std::exception &e)
        {
            LOG(Error, "Cannot get a working context to DF with dfhack. \"" << e.what() << "\"");
            alive = false;
            return;
        }

        DFHack::Process* p = df_context->getProcess();
        assert(p);
    }
    else
    // Launch a new DF process
    {
        pid_t pid;
        if (!launchDFProcess(&program_pty, &pid))
        {
            alive = false;
            return;
        }
        try
        {
            this_thread::sleep(posix_time::time_duration(posix_time::microseconds(2000000LL)));
        }
        catch (const thread_interrupted &ti)
        {
            lock_guard<recursive_mutex> lock(glue_mutex);
            alive = false;
            return;
        }
        poll_pty = true;

        try
        {
            df_contextmanager->Refresh(NULL);
            df_context = df_contextmanager->getSingleContextWithPID((int) pid);
            df_context->Attach();
            df_position_module = df_context->getPosition();
        }
        catch (std::exception &e)
        {
            LOG(Error, "Cannot get a working contect to DF with dfhack. \"" << e.what() << "\"");
            alive = false;
            return;
        }

        DFHack::Process* p = df_context->getProcess();
        assert(p);
    }

    assert(df_context->isAttached());

    unique_lock<recursive_mutex> alive_lock(glue_mutex);
    df_context->ForceResume();

    try
    {
        this_thread::sleep(posix_time::time_duration(posix_time::microseconds(2000000LL)));
    }
    catch (const thread_interrupted &ti)
    {
        alive = false;
        alive_lock.unlock();
        return;
    }

    alive_lock.unlock();

    try
    {
        while(!isDFClosed() && df_context)
        {
            this_thread::interruption_point();

            unique_lock<recursive_mutex> update_mutex(glue_mutex);

            if (close_thread) break;
            updateWindowSizeFromDFMemory();
        
            bool esc_down = false;
            while(input_queue.size() > 0)
            {
                input_queue.pop_front();
            }

            update_mutex.unlock();

            updateDFWindowTerminal();

            {
            SP<State> s = state.lock();
            SP<Slot> self_sp = self.lock();
            if (s && self_sp)
                s->signalSlotData(self_sp);
            }

            this_thread::sleep(posix_time::time_duration(posix_time::microseconds(1000000LL / ticks_per_second)));

            if (poll_pty)
            {
                while(program_pty.poll())
                {
                    char dummy[5000];
                    if (program_pty.fetch(dummy, 5000) == -1)
                        break;
                }
            }
                
        }
    }
    catch (const thread_interrupted &ti)
    {
        
    }
    alive = false;
}


void DFHackSlotLinux::updateDFWindowTerminal()
{
    lock_guard<recursive_mutex> alive_mutex_lock(glue_mutex);

    assert(df_context);

    if (df_terminal.getWidth() != df_w || df_terminal.getHeight() != df_h)
        df_terminal.resize(df_w, df_h);

    if (df_w > 256 || df_h > 256)
        return; /* I'd like to log this message but it would most likely spam the log
                   when this happens. */

    DFHack::t_screen screendata[256*256];

    assert(df_position_module);
    try
    {
        df_context->Suspend();
        if (!df_position_module->getScreenTiles(df_w, df_h, screendata))
        {
            LOG(Error, "Cannot read screen data with DFHack from DF for some reason. (missing screen_tiles_pointer?)");
            return;
        }
        df_context->ForceResume();
    }
    catch(std::exception &e)
    {
        LOG(Error, "Catched an exception from DFHack while reading screen tiles: " << e.what());
        return;
    }

    for (ui32 i1 = 0; i1 < df_h; ++i1)
        for (ui32 i2 = 0; i2 < df_w; ++i2)
        {
            ui32 offset = i2 + i1*df_w;

            Color f_color = (Color) screendata[offset].foreground;
            Color b_color = (Color) screendata[offset].background;

            if (f_color == Red) f_color = Blue;
            else if (f_color == Blue) f_color = Red;
            else if (f_color == Yellow) f_color = Cyan;
            else if (f_color == Cyan) f_color = Yellow;
            if (b_color == Red) b_color = Blue;
            else if (b_color == Blue) b_color = Red;
            else if (b_color == Yellow) b_color = Cyan;
            else if (b_color == Cyan) b_color = Yellow;

            df_terminal.setTile(i2, i1, TerminalTile(screendata[offset].symbol, 
                                                     f_color, 
                                                     b_color,
                                                     false, 
                                                     screendata[offset].bright));
        }

    LockedObject<Terminal> term = df_cache_terminal.lock();
    term->copyPreserve(&df_terminal);
}

void DFHackSlotLinux::getSize(ui32* width, ui32* height)
{
    assert(width && height);

    lock_guard<recursive_mutex> alive_lock(glue_mutex);
    ui32 w = 80, h = 25;
    (*width) = w;
    (*height) = h;
}

void DFHackSlotLinux::updateWindowSizeFromDFMemory()
{
    lock_guard<recursive_mutex> alive_lock(glue_mutex);

    assert(df_position_module);

    ::int32_t width, height;
    try
    {
        df_context->Suspend();
        df_position_module->getWindowSize(width, height);
        df_context->ForceResume();
    }
    catch (std::exception &e)
    {
        LOG(Error, "Catched an exception from DFHack while reading window size: " << e.what());
        return;
    }

    // Some protection against bogus offsets
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    if (width > 300) width = 300;
    if (height > 300) height = 300;

    df_w = width;
    df_h = height;

    LockedObject<Terminal> t = df_cache_terminal.lock();
    if (t->getWidth() != df_w || t->getHeight() != df_h)
        t->resize(df_w, df_h);
}

void DFHackSlotLinux::unloadToWindow(SP<Interface2DWindow> target_window)
{
    assert(target_window);

    LockedObject<Terminal> term = df_cache_terminal.lock();
    ui32 t_w = term->getWidth(), t_h = term->getHeight();

    ui32 actual_window_w, actual_window_h;
    t_w = min(t_w, (ui32) 256);
    t_h = min(t_h, (ui32) 256);
    target_window->setMinimumSize(t_w, t_h);
    target_window->getSize(&actual_window_w, &actual_window_h);

    ui32 game_offset_x = 0;
    ui32 game_offset_y = 0;
    if (actual_window_w > t_w)
        game_offset_x = (actual_window_w - t_w) / 2;
    if (actual_window_h > t_h)
        game_offset_y = (actual_window_h - t_h) / 2;

    CursesElement* elements = new CursesElement[actual_window_w * actual_window_h];
    ui32 i1, i2;
    for (i1 = 0; i1 < actual_window_w; ++i1)
        for (i2 = 0; i2 < actual_window_h; ++i2)
        {
            TerminalTile t;
            if (i1 < game_offset_x || i2 < game_offset_y || (i1 - game_offset_x) >= t_w || (i2 - game_offset_y) >= t_h)
                t = TerminalTile(' ', 7, 0, false, false);
            else
                t = term->getTile(i1 - game_offset_x, i2 - game_offset_y);

            ui32 symbol = t.getSymbol();
            if (symbol > 255) symbol = symbol % 256;
            elements[i1 + i2 * actual_window_w] = CursesElement(mapCharacter(symbol), (Color) t.getForegroundColor(), (Color) t.getBackgroundColor(), t.getBold());
        }
    target_window->setScreenDisplayNewElements(elements, sizeof(CursesElement), actual_window_w, actual_window_w, actual_window_h, 0, 0);
    delete[] elements;
}

void DFHackSlotLinux::feedInput(const KeyPress &kp)
{
    lock_guard<recursive_mutex> alive_lock(glue_mutex);
    input_queue.push_back(kp);
}

bool DFHackSlotLinux::isDFClosed()
{
    assert(df_contextmanager);

    DFHack::BadContexts bc;
    df_contextmanager->Refresh(&bc);
    if (bc.Contains(df_context))
    {
        df_context = (DFHack::Context*) 0;
        return true;
    }
    
    return false;
};

