#include "interface.hpp"
#include "interface_ncurses.hpp"
#include <algorithm>
#include "unicode/ustring.h"
#include "unicode/normlzr.h"
#include <iostream>
#include <sstream>
#include "nanoclock.hpp"

using namespace trankesbel;
using namespace std;
using namespace boost;

bool InterfaceCurses::ncurses_initialized = false;

InterfaceCurses::InterfaceCurses() : Interface()
{
    #ifndef NO_CURSES
    screen = (WINDOW*) 0;
    #endif
    this_class_initialized_ncurses = false;

    terminal_w = 80;
    terminal_h = 24;

    focused_window = 0xFFFFFFFF;
    initNCursesKeyMappings();

    termemu_mode = false;
    escape_down = false;
    update_key_queue = false;

    window_update = false;
    window_lock = false;

    fullscreen = false;

    key_queue_last_check = 0;

    resetDefaultKeys();
}

InterfaceCurses::InterfaceCurses(bool termemu_mode)
{
    #ifndef NO_CURSES
    screen = (WINDOW*) 0;
    #endif
    this_class_initialized_ncurses = false;

    terminal_w = 80;
    terminal_h = 24;

    window_update = false;
    window_lock = false;

    focused_window = 0xFFFFFFFF;
    escape_down = false;
    initNCursesKeyMappings();

    this->termemu_mode = termemu_mode;

    fullscreen = false;

    key_queue_last_check = 0;

    resetDefaultKeys();
}

InterfaceCurses::~InterfaceCurses()
{
    shutdown(); 
}

// stupid inefficient small line reader function used
// to read lines from keymappings file
static string fline(FILE* f)
{
    assert(f);
    string result;
    char c;

    while(!feof(f) && !ferror(f))
    {
        c = fgetc(f);
        if (c == '\n')
            break;
        if (c != '\r' && c != '\n')
            result += c;
    }
    return result;
}

// another stupid function to parse a key definition from a line
static void parse_key_definition(const string &line,
                                 string* key,
                                 string* symbol,
                                 string* shift_mod,
                                 string* alt_mod,
                                 string* ctrl_mod)
{
    const char* cstr = line.data();
    size_t len = line.size();

    string* strbuf[] = { key, symbol, shift_mod, alt_mod, ctrl_mod };
    size_t strbuf_i = 0;

    size_t i1 = 0;
    for (i1 = 0; i1 < len; ++i1)
    {
        if (cstr[i1] != ' ')
            (*strbuf[strbuf_i]) += cstr[i1];
        else
        {
            ++strbuf_i;
            if (strbuf_i > 4)
                return;
            while(cstr[i1] == ' ' && i1 < len) { ++i1; };
            --i1;
        }
    }
}

// Maps a string to a KeyCode, where string
// is a description of the key. (e.g. Alt5, Up, Return)
static KeyCode mapStrKey(const string &s, bool* special_key)
{
    assert(special_key);
#define r(a, b) if (s == a) { (*special_key) = true; return b; };
r("InvalidKey", InvalidKey);
r("Break", Break);
r("ADown", ADown); 
r("AUp", AUp); 
r("ALeft", ALeft); 
r("ARight", ARight);
r("Home", Home);
r("Backspace", Backspace);
r("F0", F0);
r("F1", F1);
r("F2", F2);
r("F3", F3);
r("F4", F4);
r("F5", F5);
r("F6", F6);
r("F7", F7);
r("F8", F8);
r("F9", F9);
r("F10", F10);
r("F11", F11);
r("F12", F12); 
r("DeleteLine", DeleteLine);
r("InsertLine", InsertLine);
r("DeleteChar", DeleteChar);
r("InsertChar", InsertChar);
r("ExitInsert", ExitInsert);
r("Clear", Clear);
r("ClearEoS", ClearEoS);
r("ClearEoL", ClearEoL);
r("SF", SF);
r("SR", SR);
r("PgDown", PgDown);
r("PgUp", PgUp);
r("SetTab", SetTab);
r("ClearTab", ClearTab);
r("ClearAllTab", ClearAllTab);
r("Enter", Enter);
r("SReset", SReset);
r("HReset", HReset);
r("Print", Print);
r("HomeDown", HomeDown);
r("A1", A1);
r("A3", A3);
r("B2", B2);
r("C1", C1);
r("C3", C3);
r("BTab", BTab);
r("Beginning", Beginning);
r("Cancel", Cancel);
r("Close", Close);
r("Command", Command);
r("Copy", Copy);
r("Create", Create);
r("End", End);
r("Exit", Exit);
r("Find", Find);
r("Help", Help);
r("Mark", Mark);
r("Message", Message);
r("Mouse", Mouse);
r("Move", Move);
r("Next", Next);
r("Open", Open);
r("Options", Options);
r("Previous", Previous);
r("Redo", Redo);
r("Reference", Reference);
r("Refresh", Refresh);
r("Replace", Replace);
r("Resize", Resize);
r("Restart", Restart);
r("Resume", Resume);
r("Save", Save);
r("SBeg", SBeg);
r("SCancel", SCancel);
r("SCommand", SCommand);
r("SCopy", SCopy);
r("SCreate", SCreate);
r("SDC", SDC);
r("SDL", SDL);
r("Select", Select);
r("SEnd", SEnd);
r("SEol", SEol);
r("SExit", SExit);
r("SFind", SFind);
r("SHelp", SHelp);
r("SHome", SHome);
r("SLeft", SLeft);
r("SMessage", SMessage);
r("SMove", SMove);
r("SNext", SNext);
r("SOptions", SOptions);
r("SPrevious", SPrevious);
r("SPrint", SPrint);
r("SRedo", SRedo);
r("SReplace", SReplace);
r("SRight", SRight);
r("SResume", SResume);
r("SSave", SSave);
r("SSuspend", SSuspend);
r("SUndo", SUndo);
r("Suspend", Suspend);
r("Undo", Undo);
r("CtrlUp", CtrlUp);
r("CtrlDown", CtrlDown);
r("CtrlRight", CtrlRight);
r("CtrlLeft", CtrlLeft);
r("Alt0", Alt0);
r("Alt1", Alt1);
r("Alt2", Alt2);
r("Alt3", Alt3);
r("Alt4", Alt4);
r("Alt5", Alt5);
r("Alt6", Alt6);
r("Alt7", Alt7);
r("Alt8", Alt8);
r("Alt9", Alt9);
r("AltUp", AltUp);
r("AltDown", AltDown);
r("AltRight", AltRight);
r("AltLeft", AltLeft);
#undef r
(*special_key) = false;
if (s.size() == 1)
    return (KeyCode) s.at(0);
return (KeyCode) strtol(s.c_str(), NULL, 10);
}

void InterfaceCurses::resetDefaultKeys()
{
    lock_key = KeyPress((KeyCode) 'L', false, false, true, false);
    fullscreen_key = KeyPress((KeyCode) 'F', false, false, true, false);
    hide_key = KeyPress((KeyCode) 'X', false, false, true, false);
    border_key = KeyPress((KeyCode) 'R', false, false, true, false);

    focus_up_key = KeyPress(AltUp, true);
    focus_down_key = KeyPress(AltDown, true);
    focus_left_key = KeyPress(AltLeft, true);
    focus_right_key = KeyPress(AltRight, true);

    focus_number_key[1] = KeyPress(Alt2, true);
    focus_number_key[2] = KeyPress(Alt3, true);
    focus_number_key[3] = KeyPress(Alt4, true);
    focus_number_key[4] = KeyPress(Alt5, true);
    focus_number_key[5] = KeyPress(Alt6, true);
    focus_number_key[6] = KeyPress(Alt7, true);
    focus_number_key[7] = KeyPress(Alt8, true);
    focus_number_key[8] = KeyPress(Alt9, true);
    focus_number_key[9] = KeyPress(InvalidKey, true);
    focus_number_key[0] = KeyPress(Alt1, true);

    // HACK: This is just a quick conf file reader so that
    // people can redefine their keys in dfterm2
    // The keys should be defined from dfterm2 project,
    // not here
    FILE* f = fopen("interface_keymappings.conf", "rb");
    if (!f)
        return;
    while(!feof(f) && !ferror(f))
    {
        string line = fline(f);
        string key, symbol, shift_mod, alt_mod, ctrl_mod;
        parse_key_definition(line, &key, &symbol, &shift_mod, &alt_mod, &ctrl_mod);
        
        bool special_key;
        KeyCode kc = mapStrKey(symbol, &special_key);
        KeyPress kp;
        kp.setKeyCode(kc, special_key);
        if (shift_mod == "on" ||
            shift_mod == "1" ||
            shift_mod == "yes" ||
            shift_mod == "true")
            kp.setShiftDown(true);
        if (alt_mod == "on" ||
            alt_mod == "1" ||
            alt_mod == "yes" ||
            alt_mod == "true")
            kp.setAltDown(true);
        if (ctrl_mod == "on" ||
            ctrl_mod == "1" ||
            ctrl_mod == "yes" ||
            ctrl_mod == "true")
            kp.setCtrlDown(true);
        if (key == "focus1")
            focus_number_key[0] = kp;
        else if (key == "focus2")
            focus_number_key[1] = kp;
        else if (key == "focus3")
            focus_number_key[2] = kp;
        else if (key == "focus4")
            focus_number_key[3] = kp;
        else if (key == "focus5")
            focus_number_key[4] = kp;
        else if (key == "focus6")
            focus_number_key[5] = kp;
        else if (key == "focus7")
            focus_number_key[6] = kp;
        else if (key == "focus8")
            focus_number_key[7] = kp;
        else if (key == "focus9")
            focus_number_key[8] = kp;
        else if (key == "focus_up")
            focus_up_key = kp;
        else if (key == "focus_down")
            focus_down_key = kp;
        else if (key == "focus_left")
            focus_left_key = kp;
        else if (key == "focus_right")
            focus_right_key = kp;
        else if (key == "lock_key")
            lock_key = kp;
        else if (key == "fullscreen_key")
            fullscreen_key = kp;
        else if (key == "hide_key")
            hide_key = kp;
        else if (key == "border_key")
            border_key = kp;
    }
    fclose(f);
}

void InterfaceCurses::shutdown()
{
    if (this_class_initialized_ncurses && ncurses_initialized)
    {
        #ifndef NO_CURSES
        endwin();
        #endif

        ncurses_initialized = false;
        this_class_initialized_ncurses = false;
    }

    #ifndef NO_CURSES
    screen = (WINDOW*) 0;
    #endif
    elements.clear();
}

bool InterfaceCurses::initialize()
{
    if (ncurses_initialized) 
    {
        initialization_error = "ncurses already initialized in this program.";
        return false;
    };

    /* Haxy hax, fix colors */ 
    screen_terminal.setCursorVisibility(false);

    #ifdef NO_CURSES
    if (!termemu_mode)
    {
        initialization_error = "curses mode requested when no curses are available.";
        return false;
    }
    #endif

    #ifndef NO_CURSES
    if (!termemu_mode)
    {
        screen = initscr();
        if (!screen)
        {
            initialization_error = "initscr() returned NULL.";
            return false;
        }

        start_color();
        /* Initialize the default colors */
        int i1;
        
        /* A stupid hack to make colors with PDCurses, for some reason red and blue
           colors flip with it. */
        #ifndef _WIN32
        short colors[] = { COLOR_BLACK, COLOR_RED, COLOR_GREEN, 
                           COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA, 
                           COLOR_CYAN, COLOR_WHITE };
        #else
        short colors[8];
        if (!termemu_mode)
        {
            colors[0] = COLOR_BLACK;
            colors[1] = COLOR_BLUE;
            colors[2] = COLOR_GREEN;
            colors[3] = COLOR_CYAN;
            colors[4] = COLOR_RED;
            colors[5] = COLOR_MAGENTA;
            colors[6] = COLOR_YELLOW;
            colors[7] = COLOR_WHITE;
        }
        else
        {
            colors[0] = COLOR_BLACK;
            colors[1] = COLOR_RED;
            colors[2] = COLOR_GREEN;
            colors[3] = COLOR_YELLOW;
            colors[4] = COLOR_BLUE;
            colors[5] = COLOR_MAGENTA;
            colors[6] = COLOR_CYAN;
            colors[7] = COLOR_WHITE;
        }
        #endif
        for (i1 = 0; i1 < 64; ++i1)
            init_pair(i1, colors[i1%8], colors[i1/8]); 

        cbreak();
        noecho();
        nodelay(screen, 1);

        nonl();
        intrflush(screen, FALSE);
        keypad(screen, TRUE);

        /* hide cursor */
        curs_set(0);

        ncurses_initialized = true;
        this_class_initialized_ncurses = true;

        getmaxyx(stdscr, terminal_h, terminal_w);
    }
    #endif

    if (termemu_mode)
    {
        terminal_w = 80;
        terminal_h = 24;

        screen_terminal.resize(terminal_w, terminal_h);
    }

    return true;
}

void InterfaceCurses::refresh()
{
    lock_guard<recursive_mutex> lock(class_mutex);
    purgeDeadWindows();

    ui32 old_terminal_w, old_terminal_h;

#ifndef NO_CURSES
    if (!termemu_mode)
    {
        old_terminal_w = terminal_w;
        old_terminal_h = terminal_h;
        getmaxyx(stdscr, terminal_h, terminal_w);
    }
#endif
    if (termemu_mode)
    {
        old_terminal_w = screen_terminal.getWidth();
        old_terminal_h = screen_terminal.getHeight();
    }

    /* Size changed or otherwise need update? */
    if (old_terminal_w != terminal_w || old_terminal_h != terminal_h || window_update)
    {
        window_update = false;

        /* Clear the screen */
        #ifndef NO_CURSES
        if (!termemu_mode)
            clear();
        #endif
        if (termemu_mode)
        {
            terminal_w = old_terminal_w;
            terminal_h = old_terminal_h;
            screen_terminal.fillRectangle(0, 0, screen_terminal.getWidth(), screen_terminal.getHeight(), TerminalTile((UChar32) ' ', 7, 0, false, false));
        }

        /* Replace all windows. 
           We do this by removing all windows first, then
           adding them back. */
        vector<Rectangle> rectangles_copy = rectangles;
        vector<Rectangle> rectangles_minimum_copy = rectangles_minimum;
        vector<AbstractWindow> rectangles_window_copy = rectangles_window;
        vector<bool> rectangles_refresh_copy = rectangles_refresh;

        rectangles.clear();
        rectangles_minimum.clear();
        rectangles_window.clear();
        rectangles_refresh.clear();
        clear_rectangles.clear();

        AbstractWindow focus;
        if (focused_window < rectangles_copy.size())
            focus = rectangles_window_copy[focused_window];

        ui32 i1, len = rectangles_copy.size();
        for (i1 = 0; i1 < len; ++i1)
        {
            ui32 x, y, w, h;
            x = y = 0;
            w = rectangles_minimum_copy[i1].right - rectangles_minimum_copy[i1].left + 1;
            h = rectangles_minimum_copy[i1].bottom - rectangles_minimum_copy[i1].top + 1;
            requestWindowSize(rectangles_window_copy[i1], &w, &h, &x, &y, true);
            rectangles_window_copy[i1].resizeEvent(Rectangle(x, y, x+w-1, y+h-1));
        }
        len = rectangles.size();
        if (focused_window < len)
            for (i1 = 0; i1 < len; ++i1)
                if (rectangles_window[i1] == focus)
                {
                    focused_window = i1;
                    break;
                }

        /* Check for fullscreen */
        if (fullscreen && focused_window < rectangles.size())
        {
            rectangles[focused_window].left = 0;
            rectangles[focused_window].right = screen_terminal.getWidth()-1;
            rectangles[focused_window].top = 0;
            rectangles[focused_window].bottom = screen_terminal.getHeight()-1;
            rectangles_window[focused_window].setHidden(false); // hidden window can't be fullscreen
            rectangles_window[focused_window].resizeEvent(Rectangle(rectangles[focused_window]));
        }

        /* to ensure we don't get into update loop */
        window_update = false;
    }

    ui32 i1, len = clear_rectangles.size();
    for (i1 = 0; i1 < len; ++i1)
    {
        #ifndef NO_CURSES
        if (!termemu_mode)
        {
            attroff(A_BOLD);
            attr_t attributes;
            attr_get(&attributes, (short*) 0, (void*) 0);
            
            Rectangle &r = clear_rectangles[i1];
            ui32 i2, i3;
            cchar_t character;
            setcchar(&character, L" ", attributes, White, NULL);
            for (i2 = r.left; i2 <= r.right; ++i2)
                for (i3 = r.top; i3 <= r.bottom; ++i3)
                    mvadd_wch(i3, i2, &character);
        } 
        #endif
        if (termemu_mode)
        {
            screen_terminal.fillRectangle(clear_rectangles[i1].left, clear_rectangles[i1].top,
                                          clear_rectangles[i1].right - clear_rectangles[i1].left + 1,
                                          clear_rectangles[i1].bottom - clear_rectangles[i1].top + 1,
                                          TerminalTile(' ', 7, 0, false, false));
        }
    }
    clear_rectangles.clear();

    Rectangle refresh_this(0, 0, 0, 0);
    bool refresh_this_ok = false;

    len = rectangles.size();
    if (fullscreen && focused_window < len && !rectangles_window[focused_window].getHidden())
    {
        rectangles_window[focused_window].refreshEvent(focused_window);
        rectangles_refresh[focused_window] = false;
        refresh_this_ok = true;
        refresh_this = refresh_this.extend(rectangles[focused_window]);
    }
    
    if (!fullscreen || focused_window >= len)
    for (i1 = 0; i1 < len; ++i1)
    {
        if (rectangles_window[i1].getHidden())
            continue;

        if ((rectangles_refresh[i1] && rectangles[i1].right >= rectangles[i1].left && rectangles[i1].bottom >= rectangles[i1].top) ||
             (refresh_this_ok && rectangles[i1].overlaps(refresh_this)))
        {
            rectangles_window[i1].refreshEvent(i1);
            rectangles_refresh[i1] = false;
            refresh_this_ok = true;
            refresh_this = refresh_this.extend(rectangles[i1]);
        }
    }

    #ifndef NO_CURSES
    if (!termemu_mode)
        ::refresh();
    #endif
}

bool InterfaceCurses::isInFullscreen()
{
    lock_guard<recursive_mutex> lock(class_mutex);
    return fullscreen;
}

void InterfaceCurses::drawBorders(ui32 x, ui32 y, ui32 w, ui32 h, ui32 color, bool bold, UnicodeString title, ui32 number)
{
    lock_guard<recursive_mutex> lock(class_mutex);
    if (w == 0 || h == 0) return;

    /* Draw a nice border around the window */
    #ifndef NO_CURSES
    if (!termemu_mode)
    {
        ui32 i1;
        if (bold) attron(A_BOLD); else attroff(A_BOLD);
        color_set(color, (void*) 0);

        /* Left and right edges */
        if (h >= 1 && w >= 1)
        for (i1 = 1; i1 < h-1; ++i1)
        {
            mvadd_wch(i1 + y, x, WACS_VLINE);
            mvadd_wch(i1 + y, x+w-1, WACS_VLINE);
        }
        /* Top and bottom edges */
        if (h >= 1 && w >= 1)
        for (i1 = 1; i1 < w-1; ++i1)
        {
            mvadd_wch(y, x + i1, WACS_HLINE);
            mvadd_wch(y+h-1, x + i1, WACS_HLINE);
        }
        /* Corners */
        mvadd_wch(y, x, WACS_ULCORNER);
        mvadd_wch(y+h-1, x, WACS_LLCORNER);
        mvadd_wch(y+h-1, x+w-1, WACS_LRCORNER);
        mvadd_wch(y, x+w-1, WACS_URCORNER);

        /* Title, if it fits */
        UnicodeString tt = title;
        if (number >= 0 && number <= 8)
        {
            stringstream ss;
            if (tt.countChar32() > 0)
                ss << " (Alt+" << number+1 << ")";
            else
                ss << "(Alt+" << number+1 << ")";
            tt.append(UnicodeString::fromUTF8(ss.str()));
        }
        ui32 title_length = tt.countChar32();
        if (title_length > w - 2)
            tt = UnicodeString(tt, 0, w - 2);

        string str = TO_UTF8(tt);

        mvaddstr(y, x+1, str.c_str());
    }
    #endif
    if (termemu_mode)
    {
        ui32 i1;
        ui32 tw = screen_terminal.getWidth();
        ui32 th = screen_terminal.getHeight();
        /* Left and right edges */
        if (h >= 1 && w >= 1)
        for (i1 = 1; i1 < h-1; ++i1)
        {
            if (i1 + y < th)
            {
                if (x < tw)
                    screen_terminal.setTile(x, i1 + y, TerminalTile(0x2551, color, 0, false, bold));
                if (x+w-1 < tw)
                    screen_terminal.setTile(x+w-1, i1 + y, TerminalTile(0x2551, color, 0, false, bold));
            }
        }
        /* Top and bottom edges */
        if (w >= 1 && h >= 1)
        for (i1 = 1; i1 < w-1; ++i1)
        {
            if (i1 + x < tw)
            {
                if (y < th)
                    screen_terminal.setTile(x + i1, y, TerminalTile(0x2550, color, 0, false, bold));
                if (y+h-1 < th)
                    screen_terminal.setTile(x + i1, y+h-1, TerminalTile(0x2550, color, 0, false, bold));
            }
        }
        /* Corners */
        if (w >= 1 && h >= 1)
        if (x < tw)
        {
            if (y < th)
                screen_terminal.setTile(x, y, TerminalTile(0x2554, color, 0, false, bold));
            if (y+h-1 < th)
                screen_terminal.setTile(x, y+h-1, TerminalTile(0x255A, color, 0, false, bold));
        }
        if (w >= 1 && h >= 1)
        if (x+w-1 < tw)
        {
            if (y < th)
                screen_terminal.setTile(x+w-1, y, TerminalTile(0x2557, color, 0, false, bold));
            if (y+h-1 < th)
                screen_terminal.setTile(x+w-1, y+h-1, TerminalTile(0x255D, color, 0, false, bold));
        }

        /* Title, if it fits */
        UnicodeString tt = title;
        if (number >= 0 && number <= 8)
        {
            stringstream ss;
            if (tt.countChar32() > 0)
                ss << " (Alt+" << number+1 << ")";
            else
                ss << "(Alt+" << number+1 << ")";
            tt.append(UnicodeString::fromUTF8(ss.str()));
        }
        ui32 title_length = tt.countChar32();
        if (title_length > w - 2)
            tt = UnicodeString(tt, 0, w - 2);

        string str = TO_UTF8(tt);

        screen_terminal.printString(str, x+1, y, color, 0, false, bold);
    }
}

bool InterfaceCurses::hasFocus(AbstractWindow who) const
{
    ui32 i1, len = rectangles.size();
    for (i1 = 0; i1 < len; ++i1)
    {
        if (rectangles_window[i1] == who)
        {
            if (focused_window == i1) return true;
            return false;
        }
    }
    return false;
}

UnicodeString InterfaceCurses::getInitializationError()
{
    return initialization_error;
}

UnicodeString InterfaceCurses::getName() const
{
    return UnicodeString("ncurses");
}

void InterfaceCurses::mapElementIdentifier(ui32 identifier, const void* data, size_t datasize)
{
    lock_guard<recursive_mutex> lock(class_mutex);

    if (elements.size() < identifier+1)
        elements.resize(identifier+1);

    if (datasize != sizeof(CursesElement))
        return;

    const CursesElement* ce = (const CursesElement*) data;
    elements[identifier] = (*ce);
}

SP<Interface2DWindow> InterfaceCurses::createInterface2DWindow()
{
    Interface2DWindowCurses* window = new Interface2DWindowCurses;
    SP<Interface2DWindowCurses> sp_window(window);
    WP<Interface2DWindowCurses> wp_window(sp_window);
    window->setInterface(this, wp_window);
    window_update = true;

    return SP<Interface2DWindow>(sp_window);
}

SP<InterfaceElementWindow> InterfaceCurses::createInterfaceElementWindow()
{
    InterfaceElementWindowCurses* window = new InterfaceElementWindowCurses;
    SP<InterfaceElementWindowCurses> sp_window(window);
    WP<InterfaceElementWindowCurses> wp_window(sp_window);
    window->setInterface(this, wp_window);
    window_update = true;

    return SP<InterfaceElementWindow>(sp_window);
}

void InterfaceCurses::requestRefresh(AbstractWindow aw)
{
    lock_guard<recursive_mutex> lock(class_mutex);

    /* Find the window from our window list. */
    ui32 i1, len = rectangles.size();
    for (i1 = 0; i1 < len; ++i1)
    {
        if (rectangles_window[i1] == aw)
        {
            rectangles_refresh[i1] = true;
            return;
        }
    }

}

void InterfaceCurses::insertWindow(ui32 target, ui32 from)
{
    ui32 i1;
    if (from > target)
        for (i1 = from; i1 > target; --i1)
            swapWindows(i1, i1-1); 
    if (from < target)
        for (i1 = from; i1 < target; ++i1)
            swapWindows(i1, i1+1);
}

void InterfaceCurses::swapWindows(ui32 index1, ui32 index2)
{
    if (index1 >= rectangles.size() ||  index2 >= rectangles.size()) return;
    if (index1 == index2) return;

    Rectangle temp_r = rectangles[index1];
    Rectangle temp_r_min = rectangles_minimum[index1];
    AbstractWindow temp_aw = rectangles_window[index1];
    bool temp_b = rectangles_refresh[index1];

    rectangles[index1] = rectangles[index2];
    rectangles_minimum[index1] = rectangles_minimum[index2];
    rectangles_window[index1] = rectangles_window[index2];
    rectangles_refresh[index1] = rectangles_refresh[index2];

    rectangles[index2] = temp_r;
    rectangles_minimum[index2] = temp_r_min;
    rectangles_window[index2] = temp_aw;
    rectangles_refresh[index2] = temp_b;

    if (focused_window == index1) focused_window = index2;
    else if (focused_window == index2) focused_window = index1;
}

bool InterfaceCurses::requestWindowSize(AbstractWindow who, ui32* width, ui32* height, ui32* x, ui32* y, bool force)
{
    purgeDeadWindows();

    window_update = true;

    lock_guard<recursive_mutex> lock(class_mutex);
    bool set_focus = false;
    ui32 old_index = 0xFFFFFFFF;
    /* Remove the window from rectangles list, if it's already there. */
    ui32 i1, i2, i3, i4, len = rectangles.size();
    for (i1 = 0; i1 < len; ++i1)
    {
        if (rectangles_window[i1] == who)
        {
             if (focused_window == i1)
             {
                 set_focus = true;
                 focused_window = 0xFFFFFFFF;
             }
             else if (focused_window > i1)
                 --focused_window;

             clear_rectangles.push_back(rectangles[i1]);
             for (i2 = 0; i2 < len; ++i2)
             {
                 if (i2 == i1) continue;
                 for (i3 = rectangles[i1].left; i3 <= rectangles[i1].right; ++i3)
                     for (i4 = rectangles[i1].top; i4 <= rectangles[i1].bottom; ++i4)
                         if (rectangles[i2].inside(i3, i4))
                         {
                             rectangles_refresh[i2] = true;
                             i4 = rectangles[i1].bottom;
                             i3 = rectangles[i1].right;
                             break;
                         }
             }
             rectangles.erase(rectangles.begin() + i1);
             rectangles_minimum.erase(rectangles_minimum.begin() + i1);
             rectangles_window.erase(rectangles_window.begin() + i1);
             rectangles_refresh.erase(rectangles_refresh.begin() + i1);
             old_index = i1;
             for (i2 = i1; i2 < len; ++i2)
                 rectangles_refresh[i2] = true;
        }
    }

    ui32 &w = (*width);
    ui32 &h = (*height);
    ui32 min_w = w;
    ui32 min_h = h;

    /* We can always grant requests of 0-sized windows. */
    if (w == 0 || h == 0) 
    {
        rectangles_minimum.push_back(Rectangle(0, 0, w, h));
        rectangles.push_back(Rectangle(1, 1, 0, 0)); /* Invalid rectangle, causes refreshes not to happen */
        rectangles_window.push_back(who);
        rectangles_refresh.push_back(false);
        w = 0;
        h = 0;
        (*x) = 1;
        (*y) = 1;
        if (set_focus) focused_window = rectangles.size()-1;
        if (old_index != 0xFFFFFFFF)
            insertWindow(old_index, rectangles.size()-1);
        return true;
    }
    if (w > terminal_w) w = terminal_w;
    if (h > terminal_h) h = terminal_h;

    /* Is this the first window? Then just put it left-top corner. */
    if (rectangles.size() == 0)
    {
        Rectangle r(0, 0, w-1, h-1);
        rectangles_minimum.push_back(Rectangle(0, 0, min_w-1, min_h-1));
        rectangles.push_back(r);
        rectangles_window.push_back(who);
        rectangles_refresh.push_back(true);
        (*x) = 0;
        (*y) = 0;
        if (set_focus) focused_window = rectangles.size()-1;
        if (old_index != 0xFFFFFFFF)
            insertWindow(old_index, rectangles.size()-1);
        return true;
    }
    /* We need to find an empty area from the screen for the new window.
       We'll do it the slowest possible way.
       Travel through every position in the screen, and then test if rectangle
       overlaps with any other rectangle on the screen at that position. 
      
       If the slowness becomes an issue, this needs to be rewritten with a
       more efficient algorithm. This code shouldn't be used very often in program.
       (Perhaps keep track of rectangles /not/ under any windows?)
       */
    set<ui32> skip_rectangles;
    /* Put hidden windows in skip_rectangles */
    len = rectangles.size();
    for (i1 = 0; i1 < len; ++i1)
        if (rectangles_window[i1].getHidden())
            skip_rectangles.insert(i1);

    while (1)
    {
        len = rectangles.size();
        for (i1 = 0; i1 < terminal_w; ++i1)
            for (i2 = 0; i2 < terminal_h; ++i2)
            {
                Rectangle r2(i1, i2, i1+w-1, i2+h-1);
                if (r2.right >= terminal_w || r2.bottom >= terminal_h) continue;

                /* Now test the overlaps. */
                bool overlap_ok = true;
                for (i3 = 0; i3 < len; ++i3)
                    if (skip_rectangles.find(i3) == skip_rectangles.end() && r2.overlaps(rectangles[i3]))
                    {
                        overlap_ok = false;
                        break;
                    }
                if (!overlap_ok) continue;

                /* There's free space here so we can place the window here. */
                rectangles_minimum.push_back(Rectangle(i1, i2, i1+min_w-1, i2+min_h-1));
                rectangles.push_back(r2);
                rectangles_window.push_back(who);
                rectangles_refresh.push_back(true);
                (*x) = i1;
                (*y) = i2;
                if (set_focus) focused_window = rectangles.size()-1;
                if (old_index != 0xFFFFFFFF)
                    insertWindow(old_index, rectangles.size()-1);
                return true;
            }
        if (skip_rectangles.size() == rectangles.size())
            break;
        /* Take the largest rectangle, act like it's not there, and try to shove the window there again. */
        vector<Rectangle> sort_rectangles = rectangles;
        bool repeat_again = true;
        while(repeat_again)
        {
            repeat_again = false;
            for (ui32 i1 = 0; i1 < sort_rectangles.size(); ++i1)
            {
                if (rectangles_window[i1].getHidden())
                {
                    sort_rectangles.erase(sort_rectangles.begin() + i1);
                    repeat_again = true;
                    break;
                }
            }
        }
        sort(sort_rectangles.begin(), sort_rectangles.end());
        skip_rectangles.insert(sort_rectangles.size()-skip_rectangles.size()-1); 
    }; /* while(1) */

    /* If we get to be this desperate getting the window on the screen, then let's just make it 0-sized. */
     rectangles_minimum.push_back(Rectangle(0, 0, w, h));
     rectangles.push_back(Rectangle(1, 1, 0, 0)); /* Invalid rectangle, causes refreshes not to happen */
     rectangles_window.push_back(who);
     rectangles_refresh.push_back(false);
     w = 0;
     h = 0;
     (*x) = 1;
     (*y) = 1;
    if (set_focus) focused_window = rectangles.size()-1;

    if (old_index != 0xFFFFFFFF)
        insertWindow(old_index, rectangles.size()-1);
    return true;
}

void InterfaceCurses::initNCursesKeyMappings()
{
    #ifndef NO_CURSES
    keymappings[Break] = KEY_BREAK;     
    keymappings[ADown] = KEY_DOWN;
    keymappings[AUp] = KEY_UP;
    keymappings[ALeft] = KEY_LEFT;
    keymappings[ARight] = KEY_RIGHT;
    keymappings[Home] = KEY_HOME; 
    keymappings[Backspace] = KEY_BACKSPACE;
    keymappings[F0] = KEY_F0;
    keymappings[F1] = KEY_F0+1;
    keymappings[F2] = KEY_F0+2;
    keymappings[F3] = KEY_F0+3;
    keymappings[F4] = KEY_F0+4;
    keymappings[F5] = KEY_F0+5;
    keymappings[F6] = KEY_F0+6;
    keymappings[F7] = KEY_F0+7;
    keymappings[F8] = KEY_F0+8;
    keymappings[F9] = KEY_F0+9;
    keymappings[F10] = KEY_F0+10;
    keymappings[F11] = KEY_F0+11;
    keymappings[F12] = KEY_F0+12; 
    keymappings[DeleteLine] = KEY_DL;
    keymappings[InsertLine] = KEY_IL;
    keymappings[DeleteChar] = KEY_DC;
    keymappings[InsertChar] = KEY_IC;
    keymappings[ExitInsert] = KEY_EIC;
    keymappings[Clear] = KEY_CLEAR;
    keymappings[ClearEoS] = KEY_EOS;
    keymappings[ClearEoL] = KEY_EOL;
    keymappings[SF] = KEY_SF;
    keymappings[SR] = KEY_SR;
    keymappings[PgDown] = KEY_NPAGE;
    keymappings[PgUp] = KEY_PPAGE;
    keymappings[SetTab] = KEY_STAB;
    keymappings[ClearTab] = KEY_CTAB;
    keymappings[ClearAllTab] = KEY_CATAB;
    keymappings[Enter] = KEY_ENTER;
    keymappings[SReset] = KEY_SRESET;
    keymappings[HReset] = KEY_RESET;
    keymappings[Print] = KEY_PRINT;
    keymappings[HomeDown] = KEY_LL;
    keymappings[A1] = KEY_A1;
    keymappings[A3] = KEY_A3;
    keymappings[B2] = KEY_B2;
    keymappings[C1] = KEY_C1;
    keymappings[C3] = KEY_C3;
    keymappings[BTab] = KEY_BTAB;
    keymappings[Beginning] = KEY_BEG;
    keymappings[Cancel] = KEY_CANCEL;
    keymappings[Close] = KEY_CLOSE;
    keymappings[Command] = KEY_COMMAND;
    keymappings[Copy] = KEY_COPY;
    keymappings[Create] = KEY_CREATE;
    keymappings[End] = KEY_END;
    keymappings[Exit] = KEY_EXIT;
    keymappings[Find] = KEY_FIND;
    keymappings[Help] = KEY_HELP;
    keymappings[Mark] = KEY_MARK;
    keymappings[Message] = KEY_MESSAGE;
    keymappings[Mouse] = KEY_MOUSE;
    keymappings[Move] = KEY_MOVE;
    keymappings[Next] = KEY_NEXT;
    keymappings[Open] = KEY_OPEN;
    keymappings[Options] = KEY_OPTIONS;
    keymappings[Previous] = KEY_PREVIOUS;
    keymappings[Redo] = KEY_REDO;
    keymappings[Reference] = KEY_REFERENCE;
    keymappings[Refresh] = KEY_REFRESH;
    keymappings[Replace] = KEY_REPLACE;
    keymappings[Resize] = KEY_RESIZE;
    keymappings[Restart] = KEY_RESTART;
    keymappings[Resume] = KEY_RESUME;
    keymappings[Save] = KEY_SAVE;
    keymappings[SBeg] = KEY_SBEG;
    keymappings[SCancel] = KEY_SCANCEL;
    keymappings[SCommand] = KEY_SCOMMAND;
    keymappings[SCopy] = KEY_SCOPY;
    keymappings[SCreate] = KEY_SCREATE;
    keymappings[SDC] = KEY_SDC;
    keymappings[SDL] = KEY_SDL;
    keymappings[Select] = KEY_SELECT;
    keymappings[SEnd] = KEY_SEND;
    keymappings[SEol] = KEY_SEOL;
    keymappings[SExit] = KEY_SEXIT;
    keymappings[SFind] = KEY_SFIND;
    keymappings[SHelp] = KEY_SHELP;
    keymappings[SHome] = KEY_SHOME;
    keymappings[SLeft] = KEY_SIC;
    keymappings[SMessage] = KEY_SMESSAGE;
    keymappings[SMove] = KEY_SMOVE;
    keymappings[SNext] = KEY_SNEXT;
    keymappings[SOptions] = KEY_SOPTIONS;
    keymappings[SPrevious] = KEY_SPREVIOUS;
    keymappings[SPrint] = KEY_SPRINT;
    keymappings[SRedo] = KEY_SREDO;
    keymappings[SReplace] = KEY_SREPLACE;
    keymappings[SRight] = KEY_SRIGHT;
    keymappings[SResume] = KEY_SRSUME;
    keymappings[SSave] = KEY_SSAVE;
    keymappings[SSuspend] = KEY_SSUSPEND;
    keymappings[SUndo] = KEY_SUNDO;
    keymappings[Suspend] = KEY_SUSPEND;
    keymappings[Undo] = KEY_UNDO;
    
    #ifdef PDCURSES
    keymappings[Alt0] = ALT_0;
    keymappings[Alt1] = ALT_1;
    keymappings[Alt2] = ALT_2;
    keymappings[Alt3] = ALT_3;
    keymappings[Alt4] = ALT_4;
    keymappings[Alt5] = ALT_5;
    keymappings[Alt6] = ALT_6;
    keymappings[Alt7] = ALT_7;
    keymappings[Alt8] = ALT_8;
    keymappings[Alt9] = ALT_9;
    keymappings[AltUp] = ALT_UP;
    keymappings[AltDown] = ALT_DOWN;
    keymappings[AltRight] = ALT_RIGHT;
    keymappings[AltLeft] = ALT_LEFT;
    #endif
    #endif

    ctrl_keycodes.insert(CtrlUp);
    ctrl_keycodes.insert(CtrlDown);
    ctrl_keycodes.insert(CtrlRight);
    ctrl_keycodes.insert(CtrlDown);

    keysequence_keymappings["\x1b\x5b\x41"] = AUp;
    keysequence_keymappings["\x1b\x5b\x42"] = ADown;
    keysequence_keymappings["\x1b\x5b\x43"] = ARight;
    keysequence_keymappings["\x1b\x5b\x44"] = ALeft;
    keysequence_keymappings["\x1b\x4f\x41"] = CtrlUp;
    keysequence_keymappings["\x1b\x4f\x42"] = CtrlDown;
    keysequence_keymappings["\x1b\x4f\x43"] = CtrlRight;
    keysequence_keymappings["\x1b\x4f\x44"] = CtrlLeft;

    keysequence_keymappings["\x1b\x5b\x31\x31\x7e"] = F1;
    keysequence_keymappings["\x1b\x5b\x31\x32\x7e"] = F2;
    keysequence_keymappings["\x1b\x5b\x31\x33\x7e"] = F3;
    keysequence_keymappings["\x1b\x5b\x31\x34\x7e"] = F4;
    keysequence_keymappings["\x1b\x5b\x31\x35\x7e"] = F5;
    keysequence_keymappings["\x1b\x5b\x31\x37\x7e"] = F6;
    keysequence_keymappings["\x1b\x5b\x31\x38\x7e"] = F7;
    keysequence_keymappings["\x1b\x5b\x31\x39\x7e"] = F8;
    keysequence_keymappings["\x1b\x5b\x32\x30\x7e"] = F9;
    keysequence_keymappings["\x1b\x5b\x32\x31\x7e"] = F10;
    keysequence_keymappings["\x1b\x5b\x32\x32\x7e"] = F11;
    keysequence_keymappings["\x1b\x5b\x32\x33\x7e"] = F12;

    keysequence_keymappings["\x1b\x5b\x31\x7e"] = Home;
    keysequence_keymappings["\x1b\x5b\x32\x7e"] = InsertChar;

    keysequence_keymappings["\x1b\x5b\x33\x7e"] = DeleteChar;

    keysequence_keymappings["\x1b\x5b\x34\x7e"] = End;
    keysequence_keymappings["\x1b\x5b\x36\x7e"] = PgDown;   // pgdown
    keysequence_keymappings["\x1b\x5b\x35\x7e"] = PgUp;  // pgup

    /* Konsole sends these */
    keysequence_keymappings["\x1b[1;3A"] = AltUp; 
    keysequence_keymappings["\x1b[1;3B"] = AltDown;   /* The 3 denotes alt key. */
    keysequence_keymappings["\x1b[1;3C"] = AltRight;  /* 5 is ctrl key and 7 is */
    keysequence_keymappings["\x1b[1;3D"] = AltLeft;   /* alt and ctrl at the same time */
    keysequence_keymappings["\x1b[1;5A"] = CtrlUp;
    keysequence_keymappings["\x1b[1;5B"] = CtrlDown;
    keysequence_keymappings["\x1b[1;5C"] = CtrlRight;
    keysequence_keymappings["\x1b[1;5D"] = CtrlLeft;
    keysequence_keymappings["\x1b[1;7A"] = AltUp;
    keysequence_keymappings["\x1b[1;7B"] = AltDown;
    keysequence_keymappings["\x1b[1;7C"] = AltRight;
    keysequence_keymappings["\x1b[1;7D"] = AltLeft;

    keysequence_keymappings["\x1b\x4fP"] = F1;
    keysequence_keymappings["\x1b\x4fQ"] = F2;
    keysequence_keymappings["\x1b\x4fR"] = F3;
    keysequence_keymappings["\x1b\x4fS"] = F4;

    keysequence_keymappings["\x1b\x5b" "F"] = End;
    keysequence_keymappings["\x1b\x5b" "H"] = Home;
}

KeyCode InterfaceCurses::mapNCursesKeyToKeyCode(wint_t nkey)
{
    UnorderedMap<KeyCode, int>::iterator i1;
    for (i1 = keymappings.begin(); i1 != keymappings.end(); ++i1)
        if ((int) i1->second == (int) nkey)
            return i1->first;

    return InvalidKey; 
}

void InterfaceCurses::parseKeyQueue(bool no_wait)
{
    key_queue2.clear();
    key_queue2.reserve(key_queue.size());

    key_queue.insert(key_queue.begin(), partial_key_queue.begin(), partial_key_queue.end());
    partial_key_queue.clear();

    ui32 i1, len = key_queue.size();
    UnorderedMap<string, KeyCode>::iterator i2;
    for (i1 = 0; i1 < len; ++i1)
    {
        if (key_queue[i1].isSpecialKey() == true) 
        {
            key_queue2.push_back(key_queue[i1]);
            continue;
        }

        /* Some keys have special key codes for them with key combinations. */
        if (key_queue[i1].isAltDown())
        {
            char c = (char) key_queue[i1].getKeyCode();
            if (c == '0') key_queue[i1].setKeyCode(Alt0, true);
            else if (c == '1') key_queue[i1].setKeyCode(Alt1, true);
            else if (c == '2') key_queue[i1].setKeyCode(Alt2, true);
            else if (c == '3') key_queue[i1].setKeyCode(Alt3, true);
            else if (c == '4') key_queue[i1].setKeyCode(Alt4, true);
            else if (c == '5') key_queue[i1].setKeyCode(Alt5, true);
            else if (c == '6') key_queue[i1].setKeyCode(Alt6, true);
            else if (c == '7') key_queue[i1].setKeyCode(Alt7, true);
            else if (c == '8') key_queue[i1].setKeyCode(Alt8, true);
            else if (c == '9') key_queue[i1].setKeyCode(Alt9, true);
            else
                c = 0;

            if (c != 0)
            {
                key_queue2.push_back(key_queue[i1]);
                continue;
            }
        }

        /* Turn all '\r' to '\n' and all '\r\n' to '\n' */
        if (key_queue[i1].getKeyCode() == (KeyCode) '\r')
        {
            if (i1 == len - 1)
            {
                if (no_wait)
                    key_queue[i1].setKeyCode((KeyCode) '\n', false);
                else
                {
                    partial_key_queue.push_back(key_queue[i1]);
                    break;
                }
            }
            else if (key_queue[i1+1].isSpecialKey() == false && key_queue[i1+1].getKeyCode() == (KeyCode) '\n')
                continue;
            else
                key_queue[i1].setKeyCode((KeyCode) '\n', false);
        }
        /* And then turn all '\n' to Enter */
        if (key_queue[i1].getKeyCode() == (KeyCode) '\n')
        {
            key_queue2.push_back(KeyPress(Enter, true, key_queue[i1].isAltDown(), key_queue[i1].isCtrlDown(), key_queue[i1].isShiftDown()));
            continue;
        }
        /* Turn keys 1-31 to CTRL+key combination (except tab and some others) */
        if ((ui32) key_queue[i1].getKeyCode() >= 1 && (ui32) key_queue[i1].getKeyCode() <= 31 && key_queue[i1].getKeyCode() != 27)
        {
            if (key_queue[i1].getKeyCode() != 9 && key_queue[i1].getKeyCode() != 13 && key_queue[i1].getKeyCode() != 10)
            {
                key_queue[i1].setKeyCode((KeyCode) ((ui32) key_queue[i1].getKeyCode() - 1 + 'A'), false);
                key_queue[i1].setCtrlDown(true);
                key_queue2.push_back(key_queue[i1]);
                continue;
            }
        }

        /* Check if following characters are valid UTF-8 */
        ui8 keyc1 = (ui8) key_queue[i1].getKeyCode();
        if ((keyc1 & 0xC0) == 0xC0 && !(keyc1 & 0x20)) /* two-byte utf-8 sequence */
        {
            if (i1 == len-1)
            {
                if (no_wait)
                    break;

                partial_key_queue.push_back(key_queue[i1]);
                break;
            }

            char utf8[] = { keyc1, (ui8) key_queue[i1+1].getKeyCode() };
            UnicodeString us = UnicodeString::fromUTF8(string(utf8, 2));
            key_queue2.push_back(KeyPress((KeyCode) us.char32At(0), false, key_queue[i1].isAltDown(), key_queue[i1].isCtrlDown(), key_queue[i1].isShiftDown()));
            ++i1;
            continue;
        }
        if ((keyc1 & 0xE0) == 0xE0 && !(keyc1 & 0x10)) /* three-byte utf-8 sequence */
        {
            if (i1 == len-2)
            {
                if (no_wait)
                    break;
                partial_key_queue.push_back(key_queue[i1]);
                partial_key_queue.push_back(key_queue[i1+1]);
                break;
            }
            else if (i1 == len-1)
            {
                if (no_wait)
                    break;
                partial_key_queue.push_back(key_queue[i1]);
                break;
            }

            char utf8[] = { keyc1, (ui8) key_queue[i1+1].getKeyCode(),
                                   (ui8) key_queue[i1+2].getKeyCode() };
            UnicodeString us = UnicodeString::fromUTF8(string(utf8, 3));
            key_queue2.push_back(KeyPress((KeyCode) us.char32At(0), false, key_queue[i1].isAltDown(), key_queue[i1].isCtrlDown(), key_queue[i1].isShiftDown()));
            i1 += 2;
            continue;
        }
        if ((keyc1 & 0xF0) == 0xF0 && !(keyc1 & 0x08)) /* four-byte utf-8 sequence */
        {
            if (i1 == len-3)
            {
                if (no_wait)
                    break;
                partial_key_queue.push_back(key_queue[i1]);
                partial_key_queue.push_back(key_queue[i1+1]);
                partial_key_queue.push_back(key_queue[i1+2]);
                break;
            }
            else if (i1 == len-2)
            {
                if (no_wait)
                    break;
                partial_key_queue.push_back(key_queue[i1]);
                partial_key_queue.push_back(key_queue[i1+1]);
                break;
            }
            else if (i1 == len-1)
            {
                if (no_wait)
                    break;
                partial_key_queue.push_back(key_queue[i1]);
                break;
            }

            char utf8[] = { keyc1, (ui8) key_queue[i1+1].getKeyCode(),
                                   (ui8) key_queue[i1+2].getKeyCode(),
                                   (ui8) key_queue[i1+3].getKeyCode() };
            UnicodeString us = UnicodeString::fromUTF8(string(utf8, 4));
            key_queue2.push_back(KeyPress((KeyCode) us.char32At(0), false, key_queue[i1].isAltDown(), key_queue[i1].isCtrlDown(), key_queue[i1].isShiftDown()));
            i1 += 3;
            continue;
        }

        bool push_normally = true;
        bool break_away = false;
        for (i2 = keysequence_keymappings.begin();
             i2 != keysequence_keymappings.end(); ++i2)
        {
            ui32 i3, len2 = i2->first.size();
            for (i3 = 0; i3 < len2; ++i3)
            {
                if (i2->first.c_str()[i3] != (char) key_queue[i1 + i3].getKeyCode())
                    break;
                if (i1 + i3 == len-1 && i3 != len2-1)
                {
                    if (no_wait)
                        break;
                    for (ui32 i4 = 0; i4 < len2 && i1+i4 < len; ++i4)
                        partial_key_queue.push_back(key_queue[i1 + i4]);
                    break_away = true;
                    break;
                }
            }
            if (break_away) break;

            if (i3 == len2)
            {
                KeyCode kc = i2->second;

                /* Turn some keys into their alt+combination equivalents. */
                if (key_queue[i1].isAltDown())
                {
                    if (kc == ALeft) kc = AltLeft;
                    if (kc == ARight) kc = AltRight;
                    if (kc == AUp) kc = AltUp;
                    if (kc == ADown) kc = AltDown;
                }

                key_queue2.push_back(KeyPress(kc, true, key_queue[i1].isAltDown(), key_queue[i1].isCtrlDown(), key_queue[i1].isShiftDown()));
                push_normally = false;
                i1 += len2 - 1;
                break;
            }
        }
        if (break_away) break;

        if (push_normally)
        {
            /* Turn escape character (27) to alt+combination,
               if it's not the last character. */
            if (key_queue[i1].getKeyCode() == (KeyCode) 27)
            {
                if (i1 == len-1 && !no_wait)
                {
                    partial_key_queue.push_back(key_queue[i1]);
                    break;
                }
                if (i1 == len-1 && no_wait)
                {
                    key_queue2.push_back(key_queue[i1]);
                    continue;
                }
                key_queue[i1+1].setAltDown(true);
                continue;
            }

            key_queue2.push_back(key_queue[i1]);
        }
    }

    key_queue.swap(key_queue2);
}

KeyPress InterfaceCurses::nextQueuePress()
{
    if (!key_queue.empty() || !partial_key_queue.empty())
    {
        bool no_wait = false;
        if (nanoclock() - key_queue_last_check > 50000000LL)
            no_wait = true;
        if (update_key_queue)
            parseKeyQueue(no_wait);
        update_key_queue = false;
        if (!partial_key_queue.empty()) update_key_queue = true;

        if (key_queue.empty()) return KeyPress();

        KeyPress kp = *key_queue.begin();
        key_queue.erase(key_queue.begin());

        return kp;
    }

    return KeyPress();
}

KeyPress InterfaceCurses::getKeyPress()
{
    lock_guard<recursive_mutex> lock(class_mutex);

    if (!key_queue.empty() || !partial_key_queue.empty()) return nextQueuePress();

    #ifndef NO_CURSES
    if (!termemu_mode)
    {
        while(1)
        {
            wint_t wch = 0;
            int result = get_wch(&wch);
            if (result == ERR) break;
            if (result == KEY_CODE_YES)
                pushKeyPress(KeyPress(mapNCursesKeyToKeyCode(wch), true));
            else
                pushKeyPress(KeyPress((KeyCode) wch, false));
        }

        if (!key_queue.empty() || !partial_key_queue.empty()) return nextQueuePress();
    }
    #endif
    if (termemu_mode)
    {
        /* no op, must use pushKeyPress to get input. */
    }

    return KeyPress();
}

void InterfaceCurses::pushKeyPress(const KeyPress &kp)
{
    key_queue_last_check = nanoclock();
    key_queue.push_back(kp);
    update_key_queue = true;
}

ui32 InterfaceCurses::getWindowUnder(ui32 x, ui32 y) const
{
    ui32 i1, len = rectangles.size();
    for (i1 = 0; i1 < len; ++i1)
    {
        if (x >= rectangles[i1].left && x <= rectangles[i1].right &&
            y >= rectangles[i1].top && y <= rectangles[i1].bottom)
            return i1;
    }

    return 0xFFFFFFFF;
}

void InterfaceCurses::requestFocus(AbstractWindow who)
{
    ui32 i1, len = rectangles.size();
    for (i1 = 0; i1 < len; ++i1)
    {
        if (rectangles_window[i1] == who)
        {
            if (focused_window != i1)
            {
                if (focused_window < len)
                    rectangles_refresh[focused_window] = true;
                focused_window = i1;
                rectangles_refresh[focused_window] = true;
            }
            return;
        }
    }
}

void InterfaceCurses::handleChangeWindowFocusByDirection(const KeyPress &kp)
{
    if (focused_window >= rectangles.size()) return;

    /* Handle changing focused window by direction */
    /* To find the window the focus goes after pressing
        alt+arrow, we take the position at top-left corner
        of the currently focused window, then move it
        to up/left/right/down (wherever arrow goes) until
        we hit some other window. First match gets to be focused.
        Out of bounds means no window in that direction. */
    i32 x, y;
    x = (i32) rectangles[focused_window].left;
    y = (i32) rectangles[focused_window].top;
    i32 x_increment, y_increment;
    x_increment = y_increment = 0;
    if (kp == focus_up_key)
        y_increment = -1;
    else if (kp == focus_down_key)
        y_increment = 1;
    else if (kp == focus_left_key)
        x_increment = -1;
    else if (kp == focus_right_key)
        x_increment = 1;
    else
        return;

    if (x_increment || y_increment)
        for (; y >= 0 && x >= 0 && x < (i32) terminal_w && y < (i32) terminal_h; x += x_increment, y += y_increment) 
        {
            ui32 new_window = getWindowUnder(x, y);
            if (new_window < rectangles.size() && new_window != focused_window)
            {
                if (focused_window < rectangles.size())
                    rectangles_refresh[focused_window] = true;
                rectangles_refresh[new_window] = true;
                focused_window = new_window;
                fullscreen = false;
                break;
            }
        }
}

void InterfaceCurses::purgeDeadWindows()
{
    ui32 i1, i2, i3, i4, len = rectangles.size();
    for (i1 = 0; i1 < len; ++i1)
    {
        if (!rectangles_window[i1].isAlive())
        {
            clear_rectangles.push_back(rectangles[i1]);
            for (i2 = 0; i2 < len; ++i2)
            {
                if (i2 == i1) continue;
                for (i3 = rectangles[i1].left; i3 <= rectangles[i1].right; ++i3)
                    for (i4 = rectangles[i1].top; i4 <= rectangles[i1].bottom; ++i4)
                        if (rectangles[i2].inside(i3, i4))
                        {
                            rectangles_refresh[i2] = true;
                            i4 = rectangles[i1].bottom;
                            i3 = rectangles[i1].right;
                            break;
                        }
            }
            rectangles.erase(rectangles.begin() + i1);
            rectangles_refresh.erase(rectangles_refresh.begin() + i1);
            rectangles_window.erase(rectangles_window.begin() + i1);
            rectangles_minimum.erase(rectangles_minimum.begin() + i1);
            for (i2 = i1; i2 < len; ++i2)
                rectangles_refresh[i2] = true;
            if (focused_window > i1) --focused_window;
            else if (focused_window == i1) focused_window = 0xFFFFFFFF;
            --i1;
            len = rectangles.size();
            continue;
        }
    }
}

void InterfaceCurses::cycle()
{
    lock_guard<recursive_mutex> lock(class_mutex);
    KeyPress kp;

    while(true)
    {
        kp = getKeyPress();
        if (kp.getKeyCode() == 0) break;

        /* Lock the window with alt+l */
        if (kp == lock_key && focused_window < rectangles.size())
        {
            rectangles_refresh[focused_window] = true;
            window_lock = (window_lock ? false : true);
            continue;
        }
        if (window_lock && focused_window < rectangles.size())
        {
            rectangles_window[focused_window].inputEvent(kp);
            continue;
        }
        if (focused_window >= rectangles.size()) window_lock = false;

        /* remove border with ctrl+r */
        if (kp == border_key && focused_window < rectangles.size())
        {
            rectangles_refresh[focused_window] = true;
            rectangles_window[focused_window].setBorder(rectangles_window[focused_window].getBorder() ? false : true);
            continue;
        }
        /* Hide with ctrl+x */
        if (kp == hide_key && focused_window < rectangles.size())
        {
            window_update = true;
            rectangles_refresh[focused_window] = true;
            rectangles_window[focused_window].setHidden(rectangles_window[focused_window].getHidden() ? false : true);
            clear_rectangles.push_back(rectangles[focused_window]);
            continue;
        }

        /* Fullscreen with ctrl+f */
        if (kp == fullscreen_key)
        {
            window_update = true;
            fullscreen = fullscreen ? false : true;
            continue;
        }

        ui32 i1;
        bool do_continue = false;
        for (i1 = 0; i1 < 10; i1++)
            if (kp == focus_number_key[i1])
            {
                if (i1 < rectangles.size())
                {
                    if (focused_window < rectangles.size())
                        rectangles_refresh[focused_window] = true;
                    rectangles_refresh[i1] = true;
                    focused_window = i1;
                    fullscreen = false;
                    do_continue = true;
                }
                break;
            }
        if (do_continue)
            continue;

        handleChangeWindowFocusByDirection(kp);
        if (focused_window < rectangles.size() && !rectangles_window[focused_window].getHidden())
        {
            if (escape_down)
                rectangles_window[focused_window].inputEvent(KeyPress((KeyCode) 27, false));
            rectangles_window[focused_window].inputEvent(kp);
        }
        escape_down = false;
    };

    if (focused_window >= rectangles.size() && rectangles.size() > 0)
    {
        focused_window = 0;
        rectangles_refresh[0] = true;
    }
}

Interface2DWindowCurses::Interface2DWindowCurses() : Interface2DWindow()
{
    interface = (InterfaceCurses*) 0;
    minimum_w = minimum_h = 0;
    w = h = 0;
    x = y = 0;
    border = true;

    is_hidden = false;

    max_keypresses = 10;
}

Interface2DWindowCurses::~Interface2DWindowCurses()
{
    w = h = 0;
}

KeyPress Interface2DWindowCurses::getKeyPress()
{
    if (!keypresses.empty())
    {
        KeyPress result = keypresses.front();
        keypresses.pop_front();
        return result;
    }

    return KeyPress();
}

void Interface2DWindowCurses::gainFocus()
{
    interface->requestFocus(self);
}

void Interface2DWindowCurses::resizeEvent(const Rectangle &r)
{
    bool size_changed = false;
    if (w != r.right - r.left + 1) size_changed = true;
    if (h != r.bottom - r.top + 1) size_changed = true;

    int border_i = 0;
    if (border) border_i = 1;

    w = r.right - r.left + 1;
    h = r.bottom - r.top + 1;
    x = r.left;
    y = r.top;

    if (elements_vector.size() != (w - 2*border_i) * (h - 2*border_i))
        elements_vector.resize((w - 2*border_i) * (h - 2*border_i), CursesElement('X', White, Black, false));

    if (size_changed && resize_callback) resize_callback(w-2*border_i, h-2*border_i);
}

void Interface2DWindowCurses::setMinimumSize(ui32 width, ui32 height)
{
    if (interface->isInFullscreen()) return;

    if (border)
    {
        width += 2;
        height += 2;
    }
    if (minimum_w == width && minimum_h == height) return;

    minimum_w = width;
    minimum_h = height;
    interface->requestWindowSize(self, &width, &height, &x, &y, true);
    w = width;
    h = height;
}

ui32 Interface2DWindowCurses::getWidth()
{
    ui32 width, throw_away;
    getSize(&width, &throw_away);
    return width;
}

ui32 Interface2DWindowCurses::getHeight()
{
    ui32 throw_away, height;
    getSize(&throw_away, &height);
    return height;
}

void Interface2DWindowCurses::getSize(ui32* width, ui32* height)
{
    ui32 border_i = 0;
    if (border) border_i = 1;

    if (w < 2*border_i || h < 2*border_i)
    {
        (*width) = 0;
        (*height) = 0;
        return;
    }

    (*width) = w-2*border_i;
    (*height) = h-2*border_i;
}

void Interface2DWindowCurses::setTitle(UnicodeString sp)
{
    title = sp;
    interface->requestRefresh(self);
}

UnicodeString Interface2DWindowCurses::getTitle() const
{
    return title;
}

string Interface2DWindowCurses::getTitleUTF8() const
{
    return TO_UTF8(title);
}

void Interface2DWindowCurses::setBorder(bool b)
{
    border = b;
}

bool Interface2DWindowCurses::getBorder() const
{
    return border;
}

void Interface2DWindowCurses::setHidden(bool h)
{
    is_hidden = h;
}

bool Interface2DWindowCurses::getHidden() const
{
    return is_hidden;
}

void Interface2DWindowCurses::refreshEvent(ui32 number)
{
    if (w == 0 || h == 0) return;
    /* We are using not re-entrant ncurses here so we just output directly to screen without consulting InterfaceCurses. 
       Lock mutexes and draw! */
    lock_guard<recursive_mutex> lock(interface->getMutex());

    bool termemu_mode = interface->getTermemuMode();

    if (border)
    {
        if (interface->hasFocus(self))
        {
            if (interface->hasLock())
                interface->drawBorders(x, y, w, h, Red, true, title, number);
            else
                interface->drawBorders(x, y, w, h, Blue, true, title, number);
        }
        else
            interface->drawBorders(x, y, w, h, White, false, title, number);
    }

    ui32 border_i = 0;
    if (border) border_i = 1;

    ui32 i1, i2;
    #ifndef NO_CURSES
    if (!termemu_mode && w >= 2*border_i && h >= 2*border_i)
        for (i1 = 0; i1 < w - 2*border_i; ++i1)
        {
            for (i2 = 0; i2 < h - 2*border_i; ++i2)
            {
                /* This process of converting a 32-bit unicode symbol to a cchar_t is incredibly
                   bothersome as can be probably seen from following code. It's probably also
                   a little slow.
                   TODO: Do something about this! */

                CursesElement &ce = elements_vector[i1 + i2 * (w-2*border_i)];
                UChar32 character32 = (UChar32) ce.Symbol;
                cchar_t character;
                attr_t attributes;
                wchar_t wcharacter[5]; /* A single character shouldn't be able to be more than 5 wide char types. */

                UChar ucharacter[3];
                UErrorCode uerror = U_ZERO_ERROR;
                ::int32_t ucharacter_written;
                u_strFromUTF32(ucharacter, 3, &ucharacter_written, &character32, 
                               1, &uerror);
                if (uerror != U_ZERO_ERROR)
                    continue;
                u_strToWCS(wcharacter, 5, NULL, ucharacter, ucharacter_written, &uerror);
                if (uerror != U_ZERO_ERROR)
                    continue;
                if (ce.Bold) attron(A_BOLD); else attroff(A_BOLD);
                attr_get(&attributes, (short*) 0, (void*) 0);

                setcchar(&character, wcharacter, attributes, ce.Foreground + ce.Background * 8, NULL);
                mvadd_wch(i2 + y + 1*border_i, i1 + x + 1*border_i, &character);
            }
        }
    #endif
    if (termemu_mode && w >= 2*border_i && h >= 2*border_i)
    {
        if (elements_vector.size() != (w - 2*border_i) * (h - 2*border_i))
            elements_vector.resize((w - 2*border_i) * (h - 2*border_i), CursesElement('X', White, Black, false));

        Terminal* screen_terminal = interface->getScreenTerminal();
        for (i1 = 0; i1 < w - 2*border_i; ++i1)
            for (i2 = 0; i2 < h - 2*border_i; ++i2)
            {
                CursesElement &ce = elements_vector[i1 + i2 * (w-2*border_i)];
                UChar32 character32 = (UChar32) ce.Symbol;

                screen_terminal->setTile(i1 + x + 1*border_i, i2 + y + 1*border_i, TerminalTile(character32, ce.Foreground, ce.Background, false, ce.Bold));
            }
    }
}

void Interface2DWindowCurses::inputEvent(const KeyPress &kp)
{
    if (input_callback) 
        input_callback(kp);
    else if (keypresses.size() < max_keypresses)
        keypresses.push_back(kp);
}

void Interface2DWindowCurses::setInputCallback(function1<void, const KeyPress &> input_callback)
{
    this->input_callback = input_callback;
}

void Interface2DWindowCurses::setResizeCallback(boost::function2<void, ui32, ui32> resize_callback)
{
    this->resize_callback = resize_callback;
}

void Interface2DWindowCurses::setScreenDisplayFill(ui32 element_index, ui32 width, ui32 height, ui32 sx, ui32 sy)
{
    ui32 border_i = 0;
    if (border) border_i = 1;

    if (w < 2*border_i || h < 2*border_i) return;

    if (elements_vector.size() != (w - 2*border_i) * (h - 2*border_i))
        elements_vector.resize((w - 2*border_i) * (h - 2*border_i), CursesElement('X', White, Black, false));

    if (width >= w - 2*border_i - sx)
        width = w - 2*border_i - sx;
    if (height >= h - 2*border_i - sy)
        height = h - 2*border_i - sy;

    ui32 i1, i2;
    for (i1 = 0; i1 < width; ++i1)
        for (i2 = 0; i2 < height; ++i2)
            elements_vector[(i1 + sx) + (i2 + sy) * (w-2*border_i)] = interface->getCursesElement(element_index);
}

void Interface2DWindowCurses::setScreenDisplayFillNewElement(const void* element, size_t element_size, ui32 width, ui32 height, ui32 sx, ui32 sy)
{
    ui32 border_i = 0;
    if (border) border_i = 1;

    if (w < 2*border_i || h < 2*border_i) return;

    if (elements_vector.size() != (w - 2*border_i) * (h - 2*border_i))
        elements_vector.resize((w - 2*border_i) * (h - 2*border_i), CursesElement('X', White, Black, false));

    if (width >= w - 2*border_i - sx)
        width = w - 2*border_i - sx;
    if (height >= h - 2*border_i - sy)
        height = h - 2*border_i - sy;

    ui32 i1, i2;
    for (i1 = 0; i1 < width; ++i1)
        for (i2 = 0; i2 < height; ++i2)
            elements_vector[(i1 + sx) + (i2 + sy) * (w-2*border_i)] = *((const CursesElement*) element);
}

void Interface2DWindowCurses::setScreenDisplayNewElements(const void* elements, size_t element_size,
                                                          ui32 element_pitch, ui32 width, ui32 height, ui32 sx, ui32 sy)
{
    ui32 border_i = 0;
    if (border) border_i = 1;

    if (w < 2*border_i || h < 2*border_i) return;

    if (elements_vector.size() != (w - 2*border_i) * (h - 2*border_i))
        elements_vector.resize((w - 2*border_i) * (h - 2*border_i), CursesElement('X', White, Black, false));

    if (width >= w - 2*border_i - sx)
        width = w - 2*border_i - sx;
    if (height >= h - 2*border_i - sy)
        height = h - 2*border_i - sy;

    ui32 i1, i2;
    for (i1 = 0; i1 < width; ++i1)
        for (i2 = 0; i2 < height; ++i2)
            elements_vector[(i1 + sx) + (i2 + sy) * (w-2*border_i)] = ((const CursesElement*) elements)[i1 + i2 * element_pitch];

    interface->requestRefresh(self);
}

void Interface2DWindowCurses::setScreenDisplay(const ui32* elements, ui32 element_pitch, ui32 width, ui32 height, ui32 sx, ui32 sy)
{
    ui32 border_i = 0;
    if (border) border_i = 1;

    if (w < 2*border_i || h < 2*border_i) return;

    if (elements_vector.size() != (w - 2*border_i) * (h - 2*border_i))
        elements_vector.resize((w - 2*border_i) * (h - 2*border_i), CursesElement('X', White, Black, false));

    if (width >= w - 2*border_i - sx)
        width = w - 2*border_i - sx;
    if (height >= h - 2*border_i - sy)
        height = h - 2*border_i - sy;

    ui32 i1, i2;
    for (i1 = 0; i1 < width; ++i1)
        for (i2 = 0; i2 < height; ++i2)
            elements_vector[(i1 + sx) + (i2 + sy) * (w-2*border_i)] = interface->getCursesElement(elements[i1 + i2 * element_pitch]);

    interface->requestRefresh(self);
}

void Interface2DWindowCurses::setInterface(InterfaceCurses* ic, WP<Interface2DWindowCurses> s)
{
    self = s;
    interface = ic;
}

InterfaceElementWindowCurses::InterfaceElementWindowCurses()
{
    last_selected_item = 0xFFFFFFFF;
    selectable_list_elements = false;
    selected_list_element = 0xFFFFFFFF;
    first_shown_element = 0;
    interface = (InterfaceCurses*) 0;
    x = y = w = h = 0;
    cursor = 0;
    cursor_left_offset = 0;
    border = true;
    is_hidden = false;
}

InterfaceElementWindowCurses::~InterfaceElementWindowCurses()
{
    w = h = 0;
}

void InterfaceElementWindowCurses::setBorder(bool b)
{
    border = b;
}

bool InterfaceElementWindowCurses::getBorder() const
{
    return border;
}

void InterfaceElementWindowCurses::setHidden(bool h)
{
    is_hidden = h;
}

bool InterfaceElementWindowCurses::getHidden() const
{
    return is_hidden;
}

void InterfaceElementWindowCurses::gainFocus()
{
    interface->requestFocus(self);
}

void InterfaceElementWindowCurses::refreshEvent(ui32 number)
{
    lock_guard<recursive_mutex> lock(interface->getMutex());
    bool termemu_mode = interface->getTermemuMode();

    int border_i = 0;
    if (border) border_i = 1;

    if (border)
    {
        if (interface->hasFocus(self))
        {
            if (interface->hasLock())
                interface->drawBorders(x, y, w, h, Red, true, title, number);
            else
                interface->drawBorders(x, y, w, h, Blue, true, title, number);
        }
        else
            interface->drawBorders(x, y, w, h, White, false, title, number);
    }

    if (termemu_mode)
    {
        Terminal* screen_terminal = interface->getScreenTerminal();
        screen_terminal->fillRectangle(x+1*border_i,y+1*border_i,w-2*border_i,h-2*border_i, TerminalTile(' ', 7, 0, false, false));

        /* No point trying to render 0 elements. */
        if (list_elements.size() == 0) return;
        
        /* Don't try anything if we don't have at least 4x3 */
        if (border && (w < 4 || h < 3)) return;
        if (!border && (w < 2 || h < 1)) return;

        /* Make sure the listing starting position is not "off the list" so that
           we can always see at least one element, if there's any. */
        if (first_shown_element >= list_elements.size())
            first_shown_element = 0;

        vector<line_info> lines;
        if (border)
            makeListLines(w-4, h-2, lines);
        else
            makeListLines(w-2, h, lines);

        ui32 i1, lines_len = lines.size();
        for (i1 = 0; i1 < lines_len; ++i1)
        {
            unsigned int fore_c = (unsigned int) White;
            unsigned int back_c = (unsigned int) Black;
            bool bold = false;

            if (lines[i1].selection) 
            {
                back_c = (unsigned int) Blue;
                bold = true;
            }

            if (lines[i1].us.countChar32() < (i32) w-(1+2*border_i))
                lines[i1].us.padTrailing(w-(1+2*border_i));

            screen_terminal->printString(TO_UTF8(lines[i1].us), x+1+border_i, y+border_i+i1, fore_c, back_c, false, bold);
            screen_terminal->setTile(x+border_i, y+border_i+i1, TerminalTile(' ', fore_c, back_c, false, bold));

            if (list_elements[lines[i1].index].getEditable() && selected_list_element == lines[i1].index && lines[i1].cursor_x >= 0 && lines[i1].cursor_x < (i32) w-(1+2*border_i))
                screen_terminal->setAttributes(1, x+1+border_i+lines[i1].cursor_x, y+border_i+i1, back_c, fore_c, false, bold);
        }
        return;
    } // if (termemu_mode)
    #ifndef NO_CURSES
    else
    {
        for (ui32 i2 = y+1*border_i; i2 <= h-2*border_i; ++i2)
            for (ui32 i1 = x+1*border_i; i1 <= w-2*border_i; ++i1)
                mvaddch(i2, i1, ' ');

        /* No point trying to render 0 elements. */
        if (list_elements.size() == 0) return;
        
        /* Don't try anything if we don't have at least 4x3 */
        if (border && (w < 4 || h < 3)) return;
        if (!border && (w < 2 || h < 1)) return;

        /* Make sure the listing starting position is not "off the list" so that
           we can always see at least one element, if there's any. */
        if (first_shown_element >= list_elements.size())
            first_shown_element = 0;

        vector<line_info> lines;
        if (border)
            makeListLines(w-4, h-2, lines);
        else
            makeListLines(w-2, h, lines);

        const size_t wch_buf_size = w*2+1;
        wchar_t* wch_buf = new wchar_t[wch_buf_size];
        wch_buf[wch_buf_size-1] = 0;

        ui32 i1, lines_len = lines.size();
        for (i1 = 0; i1 < lines_len; ++i1)
        {
            unsigned int fore_c = (unsigned int) White;
            unsigned int back_c = (unsigned int) Black;
            bool bold = false;

            if (lines[i1].selection) 
            {
                back_c = (unsigned int) Blue;
                bold = true;
            }

            if (lines[i1].us.countChar32() < (i32) w-(1+border_i*2))
                lines[i1].us.padTrailing(w-(1+border_i*2));

            size_t linelen = wch_buf_size-1;
            TO_WCHAR_STRING(lines[i1].us, wch_buf, &linelen);

            if (bold)
                attron(A_BOLD);
            else
                attroff(A_BOLD);

            color_set(fore_c + back_c * 8, (void*) 0);
            mvaddwstr(y+border_i+i1, x+1+border_i, wch_buf);
            mvaddwstr(y+border_i+i1, x+border_i, L" ");

            if (list_elements[lines[i1].index].getEditable() && selected_list_element == lines[i1].index && lines[i1].cursor_x >= 0 && lines[i1].cursor_x < (i32) w-(1+2*border_i))
            {
                color_set(back_c + fore_c * 8, (void*) 0);
                short colorpair;
                attr_t attributes;
                attr_get(&attributes, &colorpair, NULL);

                mvchgat(y+border_i+i1, x+1+border_i+lines[i1].cursor_x, 1, attributes, colorpair, NULL);
            }
        }

        delete[] wch_buf;
        return;
    }
    #endif

}

ui32 InterfaceElementWindowCurses::makeListLines(ui32 width, ui32 height, vector<line_info> &lines, bool do_selection_check)
{
    lines.clear();

    /* Should we tune first_shown_element so that selection can be seen? */
    if (do_selection_check && selected_list_element < list_elements.size())
    {
        /* If selection is above the first shown element in the list,
         * just set the first shown element to selected element and leave it
         * at that. */
        if (selected_list_element < first_shown_element)
        {
            first_shown_element = selected_list_element;
            do_selection_check = false;
        }
        /* otherwise, we need to traverse through items to find the first
           element to show. This is because of possible line wrapping, so we can't
           just calculate it in one line. */
    }
    else
        do_selection_check = false;

    ui32 first_element = first_shown_element;
    /* Make lines as they should appear in the window, filling
     * line_info structs */
    ui32 i1, cursor_y;
    bool final_trim = false;
    for (i1 = first_element, cursor_y = 0; cursor_y < height || do_selection_check; ++i1, ++cursor_y)
    {
        while (cursor_y > height && lines.size() > 0 && do_selection_check)
        {
            lines.erase(lines.begin());
            --cursor_y;

            if (lines.size() == 0) break;

            ui32 i2, len = lines.size();
            for (i2 = 0; i2 < len; ++i2)
                if (lines[i2].first_in_index)
                {
                    first_shown_element = lines[i2].index;
                    break;
                }
            if (i2 >= len)
                first_shown_element = lines[0].index;
        };

        if (final_trim) break;

        if (i1 >= list_elements.size()) break;
        UnicodeString line = list_elements[i1].getDescription() + list_elements[i1].getProcessedText();
        i32 line_len = line.countChar32();

        if (line_len <= (i32) width) /* if line fits completely inside the box, put it there and get on with it. */
        {
            line_info li;
            li.us = line;
            li.index = i1;
            li.first_in_index = true;
            if (i1 == selected_list_element)
            {
                li.selection = true;
                li.cursor_x = cursor;
                if (do_selection_check)
                    final_trim = true;
            }
            lines.push_back(li);
            continue;
        }

        bool dont_line_wrap = false;
        if (selected_list_element == i1 && list_elements[i1].getEditable())
            dont_line_wrap = true;

        if (dont_line_wrap)
        {
            i32 cursor_offset = 0;
            if (cursor < width)
                line.remove(width);
            else
            {
                line.remove(0, cursor - width);
                line.remove(width);
                cursor_offset = cursor - width;
            }

            line_info li;
            li.us = line;
            li.first_in_index = true;
            li.index = i1;
            if (i1 == selected_list_element)
            {
                li.selection = true;
                if (do_selection_check)
                    final_trim = true;
            }
            li.cursor_x = cursor - cursor_offset;
            lines.push_back(li);
            continue;
        }

        /* If we got here, we need to wrap the line in some way. */
        UnicodeString first, second;
        second = line;
        bool first_bool = true;
        ui32 cursor_offset = 0;

        while (second.countChar32() > (i32) width && (cursor_y < height || do_selection_check))
        {
            /* First we try wrapping it "nicely", from a whitespace. */
            i32 whitespace_location = -1;
            whitespace_location = second.lastIndexOf(' ', 0, width);
            if (whitespace_location == -1) whitespace_location = second.indexOf(9); /* 9 == tab */
            if (whitespace_location == -1) whitespace_location = second.indexOf('\n');
            if (whitespace_location == -1) whitespace_location = second.indexOf('\r');

            line_info li;
            if (whitespace_location != -1 && whitespace_location != 0 && whitespace_location <= (i32) width)
            {
                second.extract(0, whitespace_location, first);
                second.remove(0, whitespace_location);
                if (cursor >= cursor_offset && cursor < cursor_offset+width)
                    li.cursor_x = cursor - cursor_offset;
                cursor_offset += whitespace_location;
            }
            else /* if we can't wrap from whitespace, just wrap from the middle */
            {
                second.extract(0, width, first);
                second.remove(0, width);
                if (cursor >= cursor_offset && cursor < cursor_offset+width)
                    li.cursor_x = cursor - cursor_offset;
                cursor_offset += width;
            }

            li.us = first;
            li.first_in_index = first_bool;
            first_bool = false;
            li.index = i1;
            if (i1 == selected_list_element)
                li.selection = true;
            if (cursor >= cursor_offset && cursor < cursor_offset+width)
                li.cursor_x = cursor - cursor_offset;
            lines.push_back(li);
            ++cursor_y;
        };

        if (cursor_y < height || do_selection_check)
        {
            line_info li;
            li.us = second;
            li.first_in_index = false;
            li.index = i1;
            if (i1 == selected_list_element)
                li.selection = true;
            if (cursor >= cursor_offset && cursor < cursor_offset+width)
                li.cursor_x = cursor - cursor_offset;
            lines.push_back(li);

            if (do_selection_check && i1 == selected_list_element)
                final_trim = true;
        }
    }

    if (do_selection_check)
        return makeListLines(width, height, lines, false);

    return cursor_y;
}

void InterfaceElementWindowCurses::resizeEvent(const Rectangle &r)
{
    w = r.right - r.left + 1;
    h = r.bottom - r.top + 1;
    x = r.left;
    y = r.top;
    interface->requestRefresh(self);
}

void InterfaceElementWindowCurses::checkCursor()
{
    if (selected_list_element >= list_elements.size()) return;

    ui32 old_cursor = cursor;

    UnicodeString &us = list_elements[selected_list_element].getTextRef();
    UnicodeString &us2 = list_elements[selected_list_element].getDescriptionRef();
    if (cursor < (ui32) us2.countChar32()) cursor = us2.countChar32();
    if (cursor > (ui32) us.countChar32() + (ui32) us2.countChar32()) cursor = us.countChar32() + us2.countChar32();
    ui32 dummy = 0;
    if (input_callback_function) input_callback_function(&dummy, &cursor);
    if (cursor < (ui32) us2.countChar32()) cursor = us2.countChar32();
    if (cursor > (ui32) us.countChar32() + (ui32) us2.countChar32()) cursor = us.countChar32() + us2.countChar32();

    if (old_cursor != cursor) interface->requestRefresh(self);
}

void InterfaceElementWindowCurses::inputEvent(const KeyPress &kp)
{
    KeyCode keycode = kp.getKeyCode();
    bool special_key = kp.isSpecialKey();

    i32 i1;
    if (keycode == AUp && special_key && selected_list_element > 0)
    {
        cursor_left_offset = 0;
        ui32 selection = selected_list_element-1;
        for (i1 = (i32) selection; i1 >= 0; --i1)
            if (list_elements[i1].getSelectable() == true)
            {
                selected_list_element = i1;
                interface->requestRefresh(self);
                checkCursor();
                return;
            }
        return;
    }
    if (keycode == ADown && special_key && selected_list_element < list_elements.size() - 1)
    {
        cursor_left_offset = 0;
        ui32 selection = selected_list_element+1;
        i32 list_elements_size = (i32) list_elements.size();
        for (i1 = (i32) selection; i1 < (i32) list_elements_size; ++i1)
            if (list_elements[i1].getSelectable() == true)
            {
                selected_list_element = i1;
                interface->requestRefresh(self);
                checkCursor();
                return;
            }
        return;
    }
    if (keycode == Home && special_key && selected_list_element > 0)
    {
        ui32 selection = selected_list_element-1;
        for (i1 = (i32) selection; i1 >= 0; --i1)
            if (list_elements[i1].getSelectable() == true)
                selected_list_element = i1;
        interface->requestRefresh(self);
        cursor = 0;
        checkCursor();
        return;
    }
    if (keycode == End && special_key && selected_list_element < list_elements.size()-1)
    {
        ui32 selection = selected_list_element+1;
        for (i1 = (i32) selection; i1 < (i32) list_elements.size(); ++i1)
            if (list_elements[i1].getSelectable() == true)
                selected_list_element = i1;
        interface->requestRefresh(self);
        cursor = 0xFFFFFFFF;
        checkCursor();
        return;
    }
    if (keycode == PgDown && special_key && selected_list_element < list_elements.size()-1)
    {
        ui32 selection = selected_list_element+h-2;
        if (selection >= 0 || selection < list_elements.size())
            for (i1 = (i32) selection; i1 < (i32) list_elements.size(); ++i1)
                if (list_elements[i1].getSelectable() == true)
                {
                    selected_list_element = i1;
                    interface->requestRefresh(self);
                    checkCursor();
                    return;
                }
        return;
    }
    if (keycode == PgUp && special_key && selected_list_element > 0)
    {
        ui32 selection = selected_list_element-(h-2);
        if (selection >= 0 || selection < list_elements.size())
            for (i1 = (i32) selection; i1 >= 0; --i1)
                if (list_elements[i1].getSelectable() == true)
                {
                    selected_list_element = i1;
                    interface->requestRefresh(self);
                    checkCursor();
                    return;
                }
        return;
    }

    ui32 old_cursor = cursor;
    ui32 dummy;
    checkCursor();

    UnicodeString &us = list_elements[selected_list_element].getTextRef();
    UnicodeString &us2 = list_elements[selected_list_element].getDescriptionRef();
    if (keycode == ALeft && cursor > (ui32) us2.countChar32()) 
    { 
        --cursor; 
        if (!list_elements[selected_list_element].getEditable() && cursor_left_offset > 0)
            --cursor_left_offset;
        bool ok = true;
        dummy = 0;
        if (input_callback_function) ok = input_callback_function(&dummy, &cursor);
        if (!ok) cursor = old_cursor;
        else
            interface->requestRefresh(self); 
    };
    if (keycode == ARight && cursor < (ui32) us.countChar32() + us2.countChar32()) 
    {
        ++cursor;
        if (!list_elements[selected_list_element].getEditable())
            ++cursor_left_offset;
        bool ok = true;
        dummy = 0;
        if (input_callback_function) ok = input_callback_function(&dummy, &cursor);
        if (!ok) cursor = old_cursor;
        else
            interface->requestRefresh(self); 
    };
    if (selected_list_element < list_elements.size() && cursor > (ui32) us.countChar32() + (ui32) us2.countChar32()) cursor = us.countChar32() + us2.countChar32();
    
    if (selected_list_element < list_elements.size())
    {
        if (keycode == Enter && list_elements[selected_list_element].getSelectable())
        {
            last_selected_item = selected_list_element;
            if (callback_function) callback_function(selected_list_element);
        }
        else if (keycode == Enter && list_elements[selected_list_element].getEditable())
        {
            if (callback_function) callback_function(selected_list_element);
            dummy = 0;
            if (input_callback_function) input_callback_function(&dummy, &cursor);
        }
        else if (!special_key && list_elements[selected_list_element].getEditable())
        {
            bool add_char = true;
            if (input_callback_function)
            {
                ui32 keycode_ui32 = (ui32) keycode;
                add_char = input_callback_function(&keycode_ui32, &cursor);
            }
            if (add_char && keycode)
            {
                /* Note that we are getting reference to the list element here. */
                if (cursor > (ui32) us.countChar32() + (ui32) us2.countChar32()) cursor = us.countChar32() + us2.countChar32();
                if (keycode >= 32 && keycode != 127)
                    us.insert( (cursor++) - us2.countChar32(), (UChar32) keycode);
                else if (keycode == 127 && cursor > (ui32) us2.countChar32()) /* backspace */
                {
                    us.remove(cursor-1-us2.countChar32(), 1);
                    --cursor;
                }
                else if (keycode == 8 && cursor < (ui32) us.countChar32() + (ui32) us2.countChar32())
                    us.remove(cursor-us2.countChar32(), 1);
                interface->requestRefresh(self);
            };
        }
    }
}

void InterfaceElementWindowCurses::setInterface(InterfaceCurses* ic, WP<InterfaceElementWindowCurses> s)
{
    interface = ic;
    self = s;
}

void InterfaceElementWindowCurses::refreshWindowSize()
{
    int border_i = 0;
    if (border) border_i = 1;

    /* Calculate how much space we need. */
    ui32 max_w = 0, max_h = 0;

    ui32 terminal_w, terminal_h;
    interface->getSize(&terminal_w, &terminal_h);

    /* Do we have a list? */
    if (list_elements.size() > 0)
    {
        /* Yes we have. */
        /* The size should include the border, the list text and padding between the border and text */
        /* | Blaa blaa |   <-- Like this */

        /* So let's find the longest list element text. */
        vector<ListElement>::iterator i1, list_elements_end = list_elements.end();
        for (i1 = list_elements.begin(); i1 != list_elements_end; ++i1)
        {
            w = (*i1).getProcessedText().countChar32() + 2+2*border_i + (*i1).getDescription().countChar32();
            if (w > max_w) max_w = w;
        }
        

        /* Then two height units plus the number of list items for height */
        ui32 h = list_elements.size() + 2*border_i;
        if (h > max_h) max_h = h;
    }

    if (hint == "chat" || hint == "wide")
    {
        /* Everyone wants a wide nice chat window. */
        max_w = 200;
        if (hint == "chat") max_h = 12;
    }
    if (hint == "nicklist")
    {
        if (max_w > 20)
            max_w = 20;
    }

    if (max_h >= terminal_h / 2 && hint != "nicklist") /* Nick lists are allowed to be tall. */
        max_h = terminal_h / 2;

    max_h = max_h ? max_h : 1;
    max_w = max_w ? max_w : 1;

    ui32 w = max_w;
    ui32 h = max_h;
    ui32 x = 0;
    ui32 y = 0;

    if (w != this->w || h != this->h)
        interface->requestWindowSize(self, &w, &h, &x, &y, true);

    this->w = w;
    this->h = h;
    this->x = x;
    this->y = y;
}

ui32 InterfaceElementWindowCurses::addListElement(UnicodeString text, UnicodeString description, string data, bool selectable, bool editable)
{
    ListElement le(text, description, data, selectable, editable, false);
    list_elements.push_back(le);

    refreshWindowSize();
    return list_elements.size() - 1;
}

void InterfaceElementWindowCurses::modifyListElementDescription(ui32 index, UnicodeString description)
{
    if (index < list_elements.size())
    {
        list_elements[index].setDescription(description);
        checkCursor();
    }
    refreshWindowSize();
}

void InterfaceElementWindowCurses::modifyListElementStars(ui32 index, bool stars)
{
    if (index < list_elements.size())
        list_elements[index].setStars(stars);
}

bool InterfaceElementWindowCurses::getListElementStars(ui32 index) const
{
    if (index < list_elements.size())
        return list_elements[index].getStars();
    return false;
}

UnicodeString InterfaceElementWindowCurses::getListElementDescription(ui32 index) const
{
    if (index < list_elements.size())
        return list_elements[index].getDescription();
    return UnicodeString();
}

void InterfaceElementWindowCurses::modifyListElementText(ui32 index, UnicodeString text)
{
    if (index < list_elements.size())
    {
        list_elements[index].setText(text);
        checkCursor();
    }
    refreshWindowSize();
}

void InterfaceElementWindowCurses::modifyListElementData(ui32 index, string data)
{
    if (index < list_elements.size())
        list_elements[index].setData(data);
}

void InterfaceElementWindowCurses::modifyListElementSelectable(ui32 index, bool selectable)
{
    if (index < list_elements.size())
    {
        list_elements[index].setSelectable(selectable);
        if (selected_list_element == index && selectable == false)
            selected_list_element = 0xFFFFFFFF;
    }
}

bool InterfaceElementWindowCurses::getListElementSelectable(ui32 index) const
{
    if (index < list_elements.size())
        return list_elements[index].getSelectable();
    return false;
}

bool InterfaceElementWindowCurses::getListElementEditable(ui32 index) const
{
    if (index < list_elements.size())
        return list_elements[index].getEditable();
    return false;
}

void InterfaceElementWindowCurses::modifyListElementEditable(ui32 index, bool editable)
{
    if (index < list_elements.size())
        list_elements[index].setEditable(editable);
}

void InterfaceElementWindowCurses::modifyListElement(ui32 index, UnicodeString text, string data, bool selectable, bool editable)
{
    if (index < list_elements.size())
    {
        list_elements[index].setText(text);
        list_elements[index].setData(data);
        list_elements[index].setSelectable(selectable);
        list_elements[index].setEditable(editable);
        if (selected_list_element == index && selectable == false)
            selected_list_element = 0xFFFFFFFF;
        refreshWindowSize();
    }
}

ui32 InterfaceElementWindowCurses::getListSelectionIndex() const
{
    return selected_list_element;
}

ui32 InterfaceElementWindowCurses::getListDataIndex(const std::string &data) const
{
    ui32 result = 0;
    std::vector<ListElement>::const_iterator i1, list_elements_end = list_elements.end();
    for (i1 = list_elements.begin(); i1 != list_elements_end; ++i1, ++result)
        if (i1->getDataRef() == data) 
            return result;
    return 0xffffffff;
}

void InterfaceElementWindowCurses::modifyListSelectionIndex(ui32 selection_index)
{
    if (selection_index >= list_elements.size() ||
        !list_elements[selection_index].getSelectable())
    {
        selected_list_element = 0xFFFFFFFF;
        return;
    }

    selected_list_element = selection_index;
    interface->requestRefresh(self);
    checkCursor();
    cursor_left_offset = 0;
}

void InterfaceElementWindowCurses::deleteListElement(ui32 index)
{
    if (index >= list_elements.size()) return;

    list_elements.erase(list_elements.begin() + index);
    if (selected_list_element == index) selected_list_element = 0xFFFFFFFF;
    else if (selected_list_element > index) --selected_list_element;
    refreshWindowSize();
}

void InterfaceElementWindowCurses::deleteAllListElements()
{
    list_elements.clear();
    selected_list_element = 0xFFFFFFFF;
    refreshWindowSize();
}

ui32 InterfaceElementWindowCurses::getNumberOfListElements() const
{
    return list_elements.size();
}

UnicodeString InterfaceElementWindowCurses::getListElement(ui32 index) const
{
    if (index >= list_elements.size()) return UnicodeString();
    return list_elements[index].getText();
}

string InterfaceElementWindowCurses::getListElementData(ui32 index) const
{
    if (index >= list_elements.size()) return string();
    return list_elements[index].getData();
}

void InterfaceElementWindowCurses::setTitle(UnicodeString sp)
{
    title = sp;
    interface->requestRefresh(self);
}

UnicodeString InterfaceElementWindowCurses::getTitle() const
{
    return title;
}

string InterfaceElementWindowCurses::getTitleUTF8() const
{
    return TO_UTF8(title);
}

void InterfaceElementWindowCurses::setListCallback(function1<bool, ui32> callback_function)
{
    this->callback_function = callback_function;
}

void InterfaceElementWindowCurses::setInputCallback(function2<bool, ui32*, ui32*> callback_function)
{
    input_callback_function = callback_function;
}

void InterfaceElementWindowCurses::setHint(string hint)
{
    this->hint = hint;
};

void AbstractWindow::resizeEvent(const Rectangle &g)
{
    SP<Interface2DWindowCurses> window_2d_sp = window_2d.lock();
    if (window_2d_sp) window_2d_sp->resizeEvent(g);
    SP<InterfaceElementWindowCurses> window_element_sp = window_element.lock();
    if (window_element_sp) window_element_sp->resizeEvent(g);
}

void AbstractWindow::refreshEvent(ui32 number)
{
    SP<Interface2DWindowCurses> window_2d_sp = window_2d.lock();
    if (window_2d_sp) window_2d_sp->refreshEvent(number);
    SP<InterfaceElementWindowCurses> window_element_sp = window_element.lock();
    if (window_element_sp) window_element_sp->refreshEvent(number);
}

void AbstractWindow::inputEvent(const KeyPress &kp)
{
    SP<Interface2DWindowCurses> window_2d_sp = window_2d.lock();
    if (window_2d_sp) window_2d_sp->inputEvent(kp);
    SP<InterfaceElementWindowCurses> window_element_sp = window_element.lock();
    if (window_element_sp) window_element_sp->inputEvent(kp);
}

bool AbstractWindow::operator==(const AbstractWindow &aw) const
{
    if (window_2d.expired() && window_element.expired() &&
        aw.window_2d.expired() && aw.window_element.expired())
        return true;
    if (!window_2d.expired() && !aw.window_2d.expired())
    {
        SP<Interface2DWindowCurses> window_2d_sp = window_2d.lock();
        SP<Interface2DWindowCurses> aw_window_2d_sp = aw.window_2d.lock();
        if (window_2d_sp == aw_window_2d_sp) return true;
    }
    if (!window_element.expired() && !aw.window_element.expired())
    {
        SP<InterfaceElementWindowCurses> window_element_sp = window_element.lock();
        SP<InterfaceElementWindowCurses> aw_window_element_sp = aw.window_element.lock();
        if (window_element_sp == aw_window_element_sp) return true;
    }
    return false;
};

UnicodeString AbstractWindow::getTitle() const
{
    SP<Interface2DWindowCurses> window_2d_sp = window_2d.lock();
    if (window_2d_sp) return window_2d_sp->getTitle();
    SP<InterfaceElementWindowCurses> window_element_sp = window_element.lock();
    if (window_element_sp) return window_element_sp->getTitle();
    return UnicodeString();
}

void AbstractWindow::setTitle(UnicodeString us)
{
    SP<Interface2DWindowCurses> window_2d_sp = window_2d.lock();
    if (window_2d_sp) window_2d_sp->setTitle(us);
    SP<InterfaceElementWindowCurses> window_element_sp = window_element.lock();
    if (window_element_sp) window_element_sp->setTitle(us);
}

void AbstractWindow::setBorder(bool border)
{
    SP<Interface2DWindowCurses> c = window_2d.lock();
    if (c) c->setBorder(border);
    SP<InterfaceElementWindowCurses> d = window_element.lock();
    if (d) d->setBorder(border);
}

bool AbstractWindow::getBorder() const
{
    SP<Interface2DWindowCurses> c = window_2d.lock();
    if (c) return c->getBorder();
    SP<InterfaceElementWindowCurses> d = window_element.lock();
    if (d) return d->getBorder();

    //assert(false);
    return false;
}

void AbstractWindow::setHidden(bool border)
{
    SP<Interface2DWindowCurses> c = window_2d.lock();
    if (c) c->setHidden(border);
    SP<InterfaceElementWindowCurses> d = window_element.lock();
    if (d) d->setHidden(border);
}

bool AbstractWindow::getHidden() const
{
    SP<Interface2DWindowCurses> c = window_2d.lock();
    if (c) return c->getHidden();
    SP<InterfaceElementWindowCurses> d = window_element.lock();
    if (d) return d->getHidden();

    //assert(false);
    return false;
}

