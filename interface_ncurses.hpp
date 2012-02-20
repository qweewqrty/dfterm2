/*

Implements the ncurses interface. Most of interface is documented in interface.hpp

It can also work without curses with the use of termemu library (included in
trankesbel), if curses is not present or vt102/utf8 output is wanted for some
other reason (e.g. telnet server).

*/

#ifndef interface_ncurses_hpp
#define interface_ncurses_hpp

#include "types.hpp"
#include "interface.hpp"
#include <string>
#include <unicode/unistr.h>
#include <vector>
#include <queue>
#include <deque>
#include <set>
#include <algorithm>

#ifndef NO_CURSES
#define PDC_WIDE
#include <curses.h>
#endif

#include "termemu.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>

namespace trankesbel {

class Interface2DWindowCurses;
class InterfaceElementWindowCurses;

#ifndef NO_CURSES
enum Color { Black = COLOR_BLACK, 
             Red = COLOR_RED, 
             Green = COLOR_GREEN,
             Yellow = COLOR_YELLOW,
             Blue = COLOR_BLUE,
             Magenta = COLOR_MAGENTA,
             Cyan = COLOR_CYAN,
             White = COLOR_WHITE };
#else
enum Color { Black = 0,
             Red = 1,
             Green = 2,
             Yellow = 3,
             Blue = 4,
             Magenta = 5,
             Cyan = 6,
             White = 7 };
#endif

/* A rectangle. It has fields left, top, right and bottom that tell the boundaries of the rectangle.
   The right and bottom edges are counted as inside the rectangle. */
class Rectangle
{
    public:
        ui32 left, top, right, bottom;

        Rectangle()
        {
            left = top = right = bottom = 0;
        }
        Rectangle(ui32 l, ui32 t, ui32 r, ui32 b)
        {
            left = l;
            top = t;
            right = r;
            bottom = b;
        }
        /* Returns a rectangle that has both of the rectangles inside it. */
        Rectangle extend(const Rectangle &r2) const
        {
            Rectangle r;
            r.left = (left < r2.left) ? left : r2.left;
            r.right = (right > r2.right) ? right : r2.right;
            r.top = (top < r2.top) ? top : r2.top;
            r.bottom = (bottom > r2.bottom) ? bottom : r2.bottom;
            return r;
        };
        /* Returns true if this rectangle overlaps with another. */
        bool overlaps(const Rectangle &r) const
        {
            /* I think there's a slightly faster way to do this,
               but this test just tests if rectangle A is completely
               outside rectangle B from all directions. */
           if (left > r.right) return false;
           if (right < r.left) return false;
           if (top > r.bottom) return false;
           if (bottom < r.top) return false;
           return true;
        }
        /* Returns true if position is inside this rectangle. */
        bool inside(ui32 x, ui32 y) const
        {
            if (x >= left && y >= top && x <= right && y <= bottom) return true;
            return false;
        }
        /* Compares the rectangles by size. */
        bool operator<(const Rectangle &r) const
        {
            ui32 size = (right - left + 1) * (bottom - top + 1);
            ui32 size2 = (r.right - r.left + 1) * (r.bottom - r.top + 1);

            if (size != size2) return (size < size2);

            if (left != r.left) return (left < r.left);
            if (right != r.right) return (right < r.right);
            if (top != r.top) return (top < r.top);
            return (bottom < r.bottom);
        }
        bool operator==(const Rectangle &r) const
        {
            if (left == r.left && right == r.right && top == r.top && bottom == r.bottom) return true;
            return false;
        }
};

class Interface2DWindowCurses; 

/* This is the element used for the curses interface.
   It has a symbol, foreground and background colors
   and the bold attribute. */
class CursesElement
{
    public:
        ui32 Symbol; // in unicode
        Color Foreground; // the foreground color
        Color Background; // the background color
        bool Bold; // is it bold?

        CursesElement()
        {
            Symbol = ' ';
            Foreground = White;
            Background = Black;
            Bold = false;
        }
        CursesElement(ui32 symbol, Color foreground, Color background, bool bold)
        {
            Symbol = symbol;
            Foreground = foreground;
            Background = background;
            Bold = bold;
        }
};


/* Class that abstracts 2D-windows and element windows.
   I should have written a common base class and derive from that,
   but what's done is done...until I get less lazy and fix things.
   So for now, we'll use this glue. */
class AbstractWindow
{
    private:
    WP<Interface2DWindowCurses> window_2d;
    WP<InterfaceElementWindowCurses> window_element;
    bool is_2d;

    public:
    AbstractWindow() { is_2d = false; };

    void setBorder(bool border);
    bool getBorder() const;

    void setHidden(bool hidden);
    bool getHidden() const;

    void setWindow(WP<Interface2DWindowCurses> i2wc)
    {
        is_2d = true;
        window_2d = i2wc;
        window_element = WP<InterfaceElementWindowCurses>();
    }
    void setWindow(WP<InterfaceElementWindowCurses> iewc)
    {
        is_2d = false;
        window_element = iewc;
        window_2d = WP<Interface2DWindowCurses>();
    }
    WP<Interface2DWindowCurses> getWindow2D() const { return window_2d; };
    WP<InterfaceElementWindowCurses> getWindowElement() const { return window_element; };

    bool isAlive() const
    {
        SP<Interface2DWindowCurses> c = window_2d.lock();
        if (c) return true;
        SP<InterfaceElementWindowCurses> d = window_element.lock();
        if (d) return true;
        return false;
    }

    void resizeEvent(const Rectangle &g);
    void refreshEvent(ui32 number);
    void inputEvent(const KeyPress &kp);

    UnicodeString getTitle() const;
    void setTitle(UnicodeString us);

    bool operator==(const AbstractWindow &aw) const;
};

class InterfaceCurses : public Interface
{
    protected:
    Terminal screen_terminal;

    private:
    static bool ncurses_initialized;
    bool this_class_initialized_ncurses;

    bool update_key_queue;

    UnicodeString initialization_error;

    #ifndef NO_CURSES
    WINDOW* screen;
    #endif

    /* If termemu should be used instead of curses */
    bool termemu_mode;

    std::vector<CursesElement> elements;

    /* Current size of the terminal, assumed 80x24 by default. */
    ui32 terminal_w;
    ui32 terminal_h;

    /* If the currently focused window should be enlarged to fullscreen */
    bool fullscreen;

    /* Allocated rectangles of the terminal window. */
    std::vector<Rectangle> rectangles;
    /* The minimum size they asked. */
    std::vector<Rectangle> rectangles_minimum;
    /* And the windows they refer to */
    std::vector<AbstractWindow> rectangles_window;
    /* And if they need refreshing at next refresh() call. */
    std::vector<bool> rectangles_refresh;
    /* (Maybe should have bundled those in an object instead of using 4 different vectors...
        well it's not critical) */

    /* Swaps two window indices. */
    void swapWindows(ui32 index1, ui32 index2);
    /* Take the window from 'from', insert it at 'target', shift other windows. */
    void insertWindow(ui32 target, ui32 from);

    /* And these are rectangles that need clearing at next refresh() call. */
    std::vector<Rectangle> clear_rectangles;

    /* Currently focused window (gets input events) */
    ui32 focused_window;

    /* Mutex to allow multi-threaded access to functions where they are defined safe. */
    boost::recursive_mutex class_mutex;

    UnorderedMap<KeyCode, int> keymappings;
    void initNCursesKeyMappings();
    KeyCode mapNCursesKeyToKeyCode(wint_t nkey);

    void handleChangeWindowFocusByDirection(const KeyPress &kp);
    UnorderedMap<std::string, KeyCode> keysequence_keymappings; /* Escape sequences that map to certain key codes */
    UnorderedSet<KeyCode> ctrl_keycodes; /* Key codes that have CTRL pressed down */

    void parseKeyQueue(bool no_wait);
    KeyPress nextQueuePress();

    /* Returns index of window under given position. */
    ui32 getWindowUnder(ui32 x, ui32 y) const;

    /* Key queue for pushKeyPress/getKeyPress */
    std::vector<KeyPress> key_queue;
    std::vector<KeyPress> key_queue2; /* used for swapping and lessening copying stuff */
    std::vector<KeyPress> partial_key_queue;

    /* Last time key_queue was parsed.
       Used for waiting after ESC character and incomplete
       escape sequences. */
    ui64 key_queue_last_check;
    
    /* If last processed key was 27 (hence, if escape/alt is down) */
    bool escape_down;

    /* Set to true, all windows will have their position and size updated at next cycle. */
    bool window_update;

    /* Set to true, currently focused window will be "locked", and all keys will be
     * directly sent to it. Alt+l commonly used. */
    bool window_lock;

    /* Key configuration */

    KeyPress lock_key;             /* The key that locks a window. CTRL+L */
    KeyPress fullscreen_key;       /* The key that makes window fullscreen. CTRL+F */
    KeyPress border_key;           /* The key that makes window border go away. CTRL+R */
    KeyPress hide_key;             /* The key that makes window to hide itself. CTRL+X */
    KeyPress focus_up_key;         /* The key that focuses window above the current one. ALT+up */
    KeyPress focus_down_key;       /* The key that focuses window below the current one. ALT+down */
    KeyPress focus_left_key;       /* The key that focuses window right of the current one. ALT+left */
    KeyPress focus_right_key;      /* The key that focuses window left of the current one. ALT+right */
    KeyPress focus_number_key[10]; /* The keys that focus windows by index */

    void resetDefaultKeys(); /* initializes above key codes to their default values. */
    void purgeDeadWindows();

    protected:
    InterfaceCurses(bool termemu_mode);

    public:

    boost::recursive_mutex& getMutex() { return class_mutex; };
    
    InterfaceCurses();
    ~InterfaceCurses();

    bool initialize();
    void shutdown();

    bool getTermemuMode() const { return termemu_mode; };
    Terminal* getScreenTerminal() { return &screen_terminal; };

    void getSize(ui32* w, ui32* h) const
    {
        (*w) = terminal_w;
        (*h) = terminal_h;
    }

    bool isInFullscreen();

    void refresh();
    void drawBorders(ui32 x, ui32 y, ui32 w, ui32 h, ui32 color, bool bold, UnicodeString title, ui32 number);

    UnicodeString getName() const;
    UnicodeString getInitializationError();

    void mapElementIdentifier(ui32 identifier, const void* representation, size_t representation_size);

    void mapElementIdentifier(ui32 identifier, const CursesElement* representation) // SWIG bait
    {
        mapElementIdentifier(identifier, (const void*) representation, sizeof(CursesElement));
    }

    SP<Interface2DWindow> createInterface2DWindow();
    SP<InterfaceElementWindow> createInterfaceElementWindow();

    /* Used by windows to request window sizes for them */
    bool requestWindowSize(AbstractWindow aw, ui32* width, ui32* height, ui32* x, ui32* y, bool force = false);
    bool requestWindowSize(WP<Interface2DWindowCurses> i2wd, ui32* width, ui32* height, ui32* x, ui32* y, bool force = false)
    {
        AbstractWindow aw;
        aw.setWindow(i2wd);
        return requestWindowSize(aw, width, height, x, y, force);
    }
    bool requestWindowSize(WP<InterfaceElementWindowCurses> iewd, ui32* width, ui32* height, ui32* x, ui32* y, bool force = false)
    {
        AbstractWindow aw;
        aw.setWindow(iewd);
        return requestWindowSize(aw, width, height, x, y, force);
    }
    /* Requests interface to call refresh function on the window at next refresh. */
    void requestRefresh(AbstractWindow aw);
    void requestRefresh(WP<Interface2DWindowCurses> i2wd)
    {
        AbstractWindow aw;
        aw.setWindow(i2wd);
        requestRefresh(aw);
    }
    void requestRefresh(WP<InterfaceElementWindowCurses> iewd)
    {
        AbstractWindow aw;
        aw.setWindow(iewd);
        requestRefresh(aw);
    }

    const CursesElement& getCursesElement(ui32 index) const
    {
        return elements[index];
    };

    KeyPress getKeyPress();
    void pushKeyPress(const KeyPress &kp);

    void cycle();

    void requestFocus(AbstractWindow who);
    void requestFocus(WP<Interface2DWindowCurses> i2wd)
    {
        AbstractWindow aw;
        aw.setWindow(i2wd);
        requestFocus(aw);
    }
    void requestFocus(WP<InterfaceElementWindowCurses> iewd)
    {
        AbstractWindow aw;
        aw.setWindow(iewd);
        requestFocus(aw);
    }

    bool hasLock() const { return window_lock; };

    bool hasFocus(AbstractWindow who) const;
    bool hasFocus(WP<Interface2DWindowCurses> i2wd) const
    {
        AbstractWindow aw;
        aw.setWindow(i2wd);
        return hasFocus(aw);
    }
    bool hasFocus(WP<InterfaceElementWindowCurses> iewd) const
    {
        AbstractWindow aw;
        aw.setWindow(iewd);
        return hasFocus(aw);
    }
};

/* Represents an item in a list in element windows that have lists. */
class ListElement
{
    private:
    UnicodeString text;
    UnicodeString description;
    std::string data;
    bool selectable, editable, stars;

    public:
    ListElement() { selectable = false; editable = false; stars = false; };
    ListElement(UnicodeString t, UnicodeString desc, std::string d, bool s, bool e, bool st) 
    {
        text = t;
        data = d;
        description = desc;
        selectable = s;
        editable = e;
        stars = st;
    }

    void setText(UnicodeString t) { text = t; };

    UnicodeString getText() const { return text; };
    UnicodeString getProcessedText() const
    {
        if (!stars) return text;
        UnicodeString copy;
        copy.padTrailing(text.countChar32(), (UChar) '*');
        return copy;
    }
    UnicodeString& getTextRef() { return text; };
    const UnicodeString& getTextRef() const { return text; };

    void setDescription(UnicodeString d) { description = d; };
    UnicodeString getDescription() const { return description; };
    UnicodeString& getDescriptionRef() { return description; };
    const UnicodeString& getDescriptionRef() const { return description; };

    void setData(std::string d) { data = d; };
    std::string getData() const { return data; };
    std::string& getDataRef() { return data; };
    const std::string& getDataRef() const { return data; };

    void setSelectable(bool s) { selectable = s; };
    bool getSelectable() const { return selectable; };
    void setEditable(bool e) { editable = e; };
    bool getEditable() const { return editable; };

    void setStars(bool stars) { this->stars = stars; };
    bool getStars() const { return stars; };
};

/* Used when building lines to draw on the window, internal */
struct line_info
{
    UnicodeString us;
    bool selection;      /* should the line be with green background (selected) */
    i32 cursor_x;        /* set to where cursor is in this line or -1 if it's not on this line */
    ui32 index;          /* the list element index this line belongs to. */
    bool first_in_index; /* True if this line is where a list element. */

    line_info()
    {
        selection = false;
        cursor_x = -1;
        first_in_index = false;
        index = 0xffffffff;
    }
};

class InterfaceElementWindowCurses : public InterfaceElementWindow
{
    private:
    InterfaceCurses* interface;
    bool selectable_list_elements;
    std::vector<ListElement> list_elements;
    ui32 selected_list_element;
    ui32 first_shown_element; /* The element that is at the top of the list, if there's a list. */

    ui32 cursor; /* When editing, this is where the cursor is. */
    ui32 cursor_left_offset; /* And this is how many characters are left
                                out from left when editing a list element.
                                (For scrolling when inputting a longer string that fits on the line) */

    boost::function1<bool, ui32> callback_function;
    boost::function2<bool, ui32*, ui32*> input_callback_function;

    WP<InterfaceElementWindowCurses> self;

    ui32 w, h, x, y;

    ui32 makeListLines(ui32 width, ui32 height, std::vector<line_info> &lines, bool do_selection_check = true);

    UnicodeString title;
    std::string hint;

    ui32 last_selected_item;

    bool border;

    bool is_hidden;

    void checkCursor();

    public:
    InterfaceElementWindowCurses();
    ~InterfaceElementWindowCurses();

    void setTitle(UnicodeString title);
    void setTitleUTF8(std::string title) 
    { setTitle(UnicodeString::fromUTF8(title)); };
    UnicodeString getTitle() const;
    std::string getTitleUTF8() const;

    void gainFocus();

    void resizeEvent(const Rectangle &g);
    void refreshEvent(ui32 number);

    void setInterface(InterfaceCurses* ic, WP<InterfaceElementWindowCurses> s);
    void refreshWindowSize();

    void setHint(std::string hint);
    void setHint(UnicodeString hint) 
    {
        setHint(TO_UTF8(hint)); 
    };
    void setHint(const char* hint) { setHint(std::string(hint)); };
    void setHint(const char* hint, size_t hint_size) { setHint(std::string(hint, hint_size)); };

    ui32 addListElement(UnicodeString text, UnicodeString description, std::string data, bool selectable, bool editable);

    ui32 addListElement(UnicodeString text, std::string data, bool selectable, bool editable)
    { return addListElement(text, UnicodeString(), data, selectable, editable); };
    ui32 addListElementUTF8(std::string text, std::string data, bool selectable, bool editable)
    { return addListElement(UnicodeString::fromUTF8(text), UnicodeString(), data, selectable, editable); };
    ui32 addListElementUTF8(std::string text, std::string description, std::string data, bool selectable, bool editable)
    { return addListElement(UnicodeString::fromUTF8(text), UnicodeString::fromUTF8(description), data, selectable, editable); };

    void modifyListElementStars(ui32 index, bool stars);
    bool getListElementStars(ui32 index) const;

    void modifyListElementDescription(ui32 index, UnicodeString description);
    void modifyListElementDescriptionUTF8(ui32 index, std::string description)
    { return modifyListElementDescription(index, UnicodeString::fromUTF8(description)); };
    UnicodeString getListElementDescription(ui32 index) const;

    void modifyListElementText(ui32 index, UnicodeString text);
    void modifyListElementTextUTF8(ui32 index, std::string text)
    { modifyListElementText(index, UnicodeString::fromUTF8(text)); };
    void modifyListElementData(ui32 index, std::string data);
    void modifyListElementSelectable(ui32 index, bool selectable);
    bool getListElementSelectable(ui32 index) const;
    void modifyListElementEditable(ui32 index, bool editable);
    bool getListElementEditable(ui32 index) const;
    void modifyListElement(ui32 index, UnicodeString text, std::string data, bool selectable, bool editable);
    void modifyListElementUTF8(ui32 index, std::string text, std::string data, bool selectable, bool editable)
    { modifyListElement(index, UnicodeString::fromUTF8(text), data, selectable, editable); };
    void modifyListSelectionIndex(ui32 selection_index);
    ui32 getListSelectionIndex() const;
    ui32 getListDataIndex(const std::string &data) const;
    void deleteListElement(ui32 index);
    void deleteAllListElements();
    ui32 getNumberOfListElements() const;
    UnicodeString getListElement(ui32 index) const;
    std::string getListElementUTF8(ui32 index) const
    {
        return TO_UTF8(getListElement(index));
    };
    std::string getListElementData(ui32 index) const;
    void setListCallback(boost::function1<bool, ui32> callback_function);
    ui32 getLastSelectedItem()
    { ui32 result = last_selected_item; last_selected_item = 0xffffffff; return result; };
    void setInputCallback(boost::function2<bool, ui32*, ui32*> callback_function);

    void setBorder(bool border);
    bool getBorder() const;
    void setHidden(bool hidden);
    bool getHidden() const;

    void inputEvent(const KeyPress &kp);
};


class Interface2DWindowCurses : public Interface2DWindow
{
    private:
    InterfaceCurses* interface;
    WP<Interface2DWindowCurses> self;
    ui32 minimum_w, minimum_h;  /* These are what the user requests from setMinimumSize */
    ui32 w, h;                  /* The actual size used */
    ui32 x, y;                  /* The top-left corner of the window. */
    std::vector<CursesElement> elements_vector;

    std::deque<KeyPress> keypresses;
    ui32 max_keypresses;

    bool border;

    bool is_hidden;

    UnicodeString title;

    boost::function1<void, const KeyPress&> input_callback;
    boost::function2<void, ui32, ui32> resize_callback;

    public:
    Interface2DWindowCurses();
    ~Interface2DWindowCurses();

    void setTitle(UnicodeString title);
    void setTitleUTF8(std::string title)
    { setTitle(UnicodeString::fromUTF8(title)); };
    UnicodeString getTitle() const;
    std::string getTitleUTF8() const;

    void gainFocus();

    void resizeEvent(const Rectangle &g);
    void refreshEvent(ui32 number);

    void setMinimumSize(ui32 width, ui32 height);
    void getSize(ui32* width, ui32* height);
    ui32 getWidth();
    ui32 getHeight();

    void setScreenDisplay(const ui32* elements, ui32 element_pitch, ui32 width, ui32 height, ui32 sx, ui32 sy);
    void setScreenDisplayNewElements(const void* elements, size_t element_size,
                                     ui32 element_pitch, ui32 width, ui32 height, ui32 x = 0, ui32 y = 0);
    void setScreenDisplayFill(ui32 element_index, ui32 width, ui32 height, ui32 x = 0, ui32 y = 0);
    void setScreenDisplayFillNewElement(const void* element, size_t element_size, ui32 width, ui32 height, ui32 sx = 0, ui32 sy = 0);

    void setInterface(InterfaceCurses* ic, WP<Interface2DWindowCurses> s);

    void inputEvent(const KeyPress &kp);

    void setBorder(bool border);
    bool getBorder() const;
    void setHidden(bool hidden);
    bool getHidden() const;

    void setInputCallback(boost::function1<void, const KeyPress&> input_callback);
    void setResizeCallback(boost::function2<void, ui32, ui32> resize_callback);
    KeyPress getKeyPress();
};

};

#endif

