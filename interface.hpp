#ifndef interface_hpp
#define interface_hpp

#include "types.hpp"
#include <boost/function.hpp>
#include "unicode/unistr.h"

namespace trankesbel {

class Interface2DWindow;
class InterfaceElementWindow;
class KeyPress;

/* The interface. The class should implement basic functions such as drawing
   an element on the screen, and complex things such as menus and buttons.
   It should also handle input. And it should also be ready to work multithreaded,
   for things like a menu on top of screen while game logic runs on the background
   just minding its own business. */
class Interface
{
    private:
    /* No copies */
    Interface(const Interface &i) { };
    Interface& operator=(const Interface &i) { return (*this); };

    public:
    
    Interface() { };
    virtual ~Interface() { };

    /* These should initialize and shutdown the interface as needed.
       Return value of false means initialization failed and true means it succeeded.
       The reason for error can be read from getInitializationError() in case of failure. Not thread-safe. */
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    /* Refreshes the screen to reflect updates to windows. Thread safe but has race conditions. */
    virtual void refresh() = 0;

    /* Returns the error that occured during initialization. Thread-safe. */
    virtual UnicodeString getInitializationError() = 0;

    /* Returns the name of the implementation. E.g. "ncurses" when it's text. Thread-safe. */
    virtual UnicodeString getName() const = 0;

    /* Maps an element identifier number to a representation. The representation
       depends on the implementation of the interface. Note that implementation
       may allocate memory as (N * X), where N is the highest identifier number
       and X is the size of an element representation. Thread safe in the sense
       that it won't burn and crash at simultaneous access but at the same identifier
       number there's a race condition at which representation gets used. */
    virtual void mapElementIdentifier(ui32 identifier, const void* representation, size_t representation_size) = 0;

    /* Creates a 2D interface window to the screen. Thread-safe. */
    virtual SP<Interface2DWindow> createInterface2DWindow() = 0;
    /* Creates a 2D element window to the screen. Threaf-safe. */
    virtual SP<InterfaceElementWindow> createInterfaceElementWindow() = 0;

    /* Returns a key code for a key press. Thread-safe in the sense
       it won't crash and burn if multiple threads call it at the same time.

       You should probably use events instead of reading the key directly.
    */
    virtual KeyPress getKeyPress() = 0;

    /* Pushes a key press to queue.
       You can use this to use alternative sources of input outside the interface
       (e.g. telnet -connection) 
       The pushed keys are returned in the order they were pushed. (i.e. first-in first-out).
       
       Certain backends may mangle the key press sequence, in order to collapse
       certain escape sequences (termemu). */
    virtual void pushKeyPress(const KeyPress &kc) = 0;

    /* Processes all key presses and sends events to windows. */
    virtual void cycle() = 0;
};

/* Window defines an area of the interface (the game screen, HUD, stats etc.).
   Interface itself defines where these are drawn on the actual display.
   There are two types of interface windows, 2D-windows and element windows.
   2D-windows only show "tiles", which may be symbols and colors (traditional roguelike map) or
   actual graphical tiles (compare to crawl/nethack tiles).
   Element windows concentrate on showing item lists, menus, status displays, etc.
   They don't have a predefined size, the interface decides how big they are, where they are
   displayed etc.

   Element windows are not supposed to be a fully featured UI for everything.
   For the moment we are targetting simple interfaces for roguelike games. For this reason,
   there are no advanced positioning stuff, nested lists or other exotic stuff.

   */

/* An element window. */
class InterfaceElementWindow
{
    public:
    InterfaceElementWindow() { };
    virtual ~InterfaceElementWindow() { };

    /* Sets a title for the window. */
    virtual void setTitle(UnicodeString title) = 0;
    /* Same as above, but assumes a UTF-8 encoded standard string. */
    virtual void setTitleUTF8(std::string title) = 0;
    /* Gets the title currently used for the window. */
    virtual UnicodeString getTitle() const = 0;
    /* Same as above, only in UTF8 */
    virtual std::string getTitleUTF8() const = 0;

    /* Tells window to gain focus. */
    virtual void gainFocus() = 0;

    /* Sets a hint on how this window is used.
     * The hint is a string and may be totally ignored.
     * Currently defined hints:
     *   "chat"     The window is meant for lines of text, 
     *              common for chat boxes. These are wide and short.
     *   "nicklist" The window is meant for displaying a list
     *              of words (usually nicks). Tend to be tall and
     *              narrow.
     *   "wide"     The window will try to be as wide as possible.
     *   "default"  Use default behaviour. This is how all newly created
     *              windows are. Any unknown hint value will be also treated
     *              like it was "default".
     * Of course, no guarantees are made how these will actually look like. */
    virtual void setHint(std::string hint) = 0;
    virtual void setHint(UnicodeString hint) = 0;
    virtual void setHint(const char* hint) = 0;
    virtual void setHint(const char* hint, size_t hint_size) = 0;

    /* Adds a list element to the window. Initially the window does not have a list;
       adding an element may visually create one. Returns an index to the element. 
       If 'selectable' is set to true, it will be possible to select the item from
       the list using arrow keys. Returns an index to the list element. 
       'data' is an user-defined string to associate with item (note that it's
       not of type UnicodeString, it's meant to be plain 8-bit byte sequence).

       If 'editable' is set to true, it is possible for the user to edit the text.
       Set the editable callbacks, if you want to control what can be entered.

       Description is used to describe what the list element represents. It can be empty.

       In future revisions the list elements may be separate objects themselves, rather
       than plain strings. */
    virtual ui32 addListElement(UnicodeString text, std::string data, bool selectable, bool editable = false) = 0;
    virtual ui32 addListElement(UnicodeString text, UnicodeString description, std::string data, bool selectable, bool editable = false) = 0;
    /* Same as above, but assumes a UTF-8 encoded standard string(s). */
    virtual ui32 addListElementUTF8(std::string text, std::string data, bool selectable, bool editable = false) = 0;
    virtual ui32 addListElementUTF8(std::string text, std::string description, std::string data, bool selectable, bool editable = false) = 0;

    /* Return whether or not to use stars instead of characters
       in list element. Useful for password fields. 
       Setting stars for non-existant item is a NOP, and
       getting it is always false for non-existant items. */
    virtual void modifyListElementStars(ui32 index, bool use_stars) = 0;
    virtual bool getListElementStars(ui32 index) const = 0;

    /* Set a description for a list element.
     * Useful for editable list elements, that are edited for configuration.
     * Modifying is a NOP when used with an invalid index. Getting the description
     * returns an empty string if you use an invalid index.
     */
    virtual void modifyListElementDescription(ui32 index, UnicodeString description) = 0;
    virtual void modifyListElementDescriptionUTF8(ui32 index, std::string description) = 0;
    virtual UnicodeString getListElementDescription(ui32 index) const = 0;

    /* Deletes a list element created with addListElement(). Note that
       the index numbers of list elements above the index are shifted down. */
    virtual void deleteListElement(ui32 index) = 0;

    /* Deletes all list elements. */
    virtual void deleteAllListElements() = 0;

    /* Returns the number of list elements currently in window. */
    virtual ui32 getNumberOfListElements() const = 0;

    /* Returns the string pointed by list element index. If index is invalid,
       it returns an empty string. */
    virtual UnicodeString getListElement(ui32 index) const = 0;
    /* Same as above, but in UTF-8 encoded string. */
    virtual std::string getListElementUTF8(ui32 index) const = 0;

    /* Returns the data associated with list element index. If index is invalid,
       it returns an empty string. */
    virtual std::string getListElementData(ui32 index) const = 0;

    /* Modifies the text of a list element. Fails silently on invalid index. */
    virtual void modifyListElementText(ui32 index, UnicodeString text) = 0;
    /* Same as above, but assumes a UTF-8 encoded standard string. */
    virtual void modifyListElementTextUTF8(ui32 index, std::string text) = 0;

    /* Modifies the data of a list element. Fails silently on invalid index. */
    virtual void modifyListElementData(ui32 index, std::string data) = 0;

    /* Modifies if the list element is selectable. If a currently selected item is
       set as not selectable, the selection is resetted to nothing selected. */
    virtual void modifyListElementSelectable(ui32 index, bool selectable) = 0;
    /* Returns if the list element is selectable. If index is invalid, return value is
       false (as in with not selectable items). */
    virtual bool getListElementSelectable(ui32 index) const = 0;

    /* Modifies if the list element is editable. */
    virtual void modifyListElementEditable(ui32 index, bool editable) = 0;
    /* Returns if the list element is editable. If index is invalid, return value
       is false (as in with not editable items). */
    virtual bool getListElementEditable(ui32 index) const = 0;

    /* Modifies both text and data of a list element. Fails silently on invalid index. 
       If a currently selected item is set as not selectable, the selection is 
       resetted to nothing selected. */
    virtual void modifyListElement(ui32 index, UnicodeString text, std::string data, bool selectable, bool editable = false) = 0;
    /* Same as above, but assuming a UTF-8 encoded standard string. */
    virtual void modifyListElementUTF8(ui32 index, std::string text, std::string data, bool selectable, bool editable = false) = 0;

    /* Puts the selection to specified index in list. If index is invalid or item is not selectable,
       the selection is resetted to nothing selected. */
    virtual void modifyListSelectionIndex(ui32 selection_index) = 0;
    /* Returns the current index of selected item. */
    virtual ui32 getListSelectionIndex() const = 0;
    /* Returns index to an element that has the data string equal to
       'data' argument. */
    virtual ui32 getListDataIndex(const std::string &data) const = 0;

    /* Sets a callback when user selects an item from the list. 
       The argument will be the index of the selected item.*/
    virtual void setListCallback(boost::function1<bool, ui32> callback_function) = 0;
    /* Returns last list element selected. Subsequent calls return 0xffffffff.
       This is an alternative to setting callbacks. */
    virtual ui32 getLastSelectedItem() = 0;

    /* Sets callback when user inputs a letter to an editable list element.
       You can edit given keycode to modify how the interface
       interprets the key press. If you return false, then it's as the key press
       was ignored. The first argument in the callback is the keycode. 
       The second argument is the position where the character
       would be inserted or the position where the cursor would move if the keycode is 0. 
       Keycodes 127 and 8 will delete a letter. 
       Useful for accepting only
       numeric input or maybe something else. Keycode 0 will be ignored.
       Cursor position changes are also reported, but with keycodes of 0.
       You can use this to disallow cursor position to move to some
       areas of the string.
       (see Interface::pushKeyPress and Interface::getKeyPress) */
    virtual void setInputCallback(boost::function2<bool, ui32*, ui32*> callback_function) = 0;
};

/* This is the 2D interface window class. */
class Interface2DWindow
{
    public:
    Interface2DWindow() { };
    virtual ~Interface2DWindow() { };

    /* Sets a title for the window. */
    virtual void setTitle(UnicodeString title) = 0;
    /* Sets a title for the window, assuming UTF-8 string. */
    virtual void setTitleUTF8(std::string title) = 0;
    /* Gets the title currently used for the window. */
    virtual UnicodeString getTitle() const = 0;
    /* Same as above, only in UTF8 */
    virtual std::string getTitleUTF8() const = 0;

    /* Tells window to gain focus. */
    virtual void gainFocus() = 0;
    
    /* Use this to set how much space you want for the 2D-display, at the very least.
       The interface may give you more. If there's absolutely no space,
       the size will be smaller and can be queried with getSize(). */
    virtual void setMinimumSize(ui32 width, ui32 height) = 0;

    /* Returns the size that has been allocated for the window to width and 
       height. Note that adding more interface windows (of any type) to the 
       screen may change the size for individual windows. */
    virtual void getSize(ui32* width, ui32* height) = 0;
    /* Same as Interface2DWindow::getSize, but individually for width and height. */
    virtual ui32 getWidth() = 0;
    virtual ui32 getHeight() = 0;

    /* Sets a callback function that gets called whenever input is entered to this window. */
    virtual void setInputCallback(boost::function1<void, const KeyPress&> input_callback) = 0;
    /* Sets a callback function that gets called whenever the size of the window is changed.
       The parameters are width and height (as in what can be used in setScreenDisplayX function). */
    virtual void setResizeCallback(boost::function2<void, ui32, ui32> resize_callback) = 0;

    /* If you have not set an input callback, you can query keys through this function instead.
       Keys are queued to undefined extent, and when queue is empty, returns a null key. (KeyCode == 0). */
    virtual KeyPress getKeyPress() = 0;

    /* Defines the things that should be displayed on the screen right now as an array of 32-bit integers.
       The elements are assumed to be in an array, where element_pitch defines how many elements you need to
       go to reach the next row. The first element will be drawn from top and left at given position of the window,
       going to the far bottom-right corner if the element array is big enough. If the window is smaller
       than the element array, the extra data is discarded. */
    virtual void setScreenDisplay(const ui32* elements, ui32 element_pitch, ui32 width, ui32 height, ui32 x = 0, ui32 y = 0) = 0;
    /* Same as above, but directly with implementation-dependent elements. The elements won't be registered
       as new ones to interface. See Interface::mapElementIdentifier(). */
    virtual void setScreenDisplayNewElements(const void* elements, size_t element_size,
                                             ui32 element_pitch, ui32 width, ui32 height, ui32 x = 0, ui32 y = 0) = 0;
    /* Fills all the elements with a single element. */
    virtual void setScreenDisplayFill(ui32 element_index, ui32 width, ui32 height, ui32 x = 0, ui32 y = 0) = 0;
    /* And directly with an element. */
    virtual void setScreenDisplayFillNewElement(const void* element, size_t element_size, ui32 width, ui32 height, ui32 x = 0, ui32 y = 0) = 0;
};

/* Some special key codes. Convert the value from GetKeyPress to this enum type to use them,
   when special_key was set to true in that call. Most of these correspond to ncurses key codes (but their values
   are not the same!) 
   Don't change the order. Some code does things like if (x >= ADown && x <= ARight) */
enum KeyCode {
   InvalidKey = 0,
   Break,                /* break key */
   ADown, AUp, ALeft, ARight, /* arrow keys */
   Home,                 /* the home key */
   Backspace,
   F0,
   F1,
   F2,
   F3,
   F4,
   F5,
   F6,
   F7,
   F8,
   F9,
   F10,
   F11,
   F12,  /* the function keys */
   DeleteLine, /* line delete */
   InsertLine, /* line insert */
   DeleteChar, /* delete character */
   InsertChar, /* insert character or insert mode */
   ExitInsert, /* exit insert char mode */
   Clear, /* clear screen */
   ClearEoS, /* clear to end of screen */
   ClearEoL, /* clear to end of line */
   SF, /* scroll 1 line forward */
   SR, /* scroll 1 line backward */
   PgDown, /* next page */
   PgUp, /* previous page */
   SetTab, /* set tab */
   ClearTab, /* clear tab */
   ClearAllTab, /* clear all tabs */
   Enter, /* enter (or return) */
   SReset, /* soft reset */
   HReset, /* hard reset */
   Print, /* print/copy */
   HomeDown, /* home down */
   A1, /* upper left of keypad */
   A3, /* upper right of keypad */
   B2, /* center of keypad */
   C1, /* lower left of keypad */
   C3, /* lower right of keypad */
   BTab, /* back tab key */
   Beginning, /* beginning key */
   Cancel,
   Close,
   Command,
   Copy,
   Create,
   End,
   Exit,
   Find,
   Help,
   Mark,
   Message,
   Mouse,
   Move,
   Next,
   Open,
   Options,
   Previous,
   Redo,
   Reference,
   Refresh,
   Replace,
   Resize,
   Restart,
   Resume,
   Save,
   SBeg,
   SCancel,
   SCommand,
   SCopy,
   SCreate,
   SDC,
   SDL,
   Select,
   SEnd,
   SEol,
   SExit,
   SFind,
   SHelp,
   SHome,
   SLeft,
   SMessage,
   SMove,
   SNext,
   SOptions,
   SPrevious,
   SPrint,
   SRedo,
   SReplace,
   SRight,
   SResume,
   SSave,
   SSuspend,
   SUndo,
   Suspend,
   Undo,

   /* Termemu -backend. */
   CtrlUp,
   CtrlDown,
   CtrlRight,
   CtrlLeft,
   
   /* These exist only on Windows with PDCurses */
   Alt0,
   Alt1,
   Alt2,
   Alt3,
   Alt4,
   Alt5,
   Alt6,
   Alt7,
   Alt8,
   Alt9,
   AltUp,
   AltDown,
   AltRight,
   AltLeft,
};

/* Represents a key press. */
class KeyPress
{
    private:
        KeyCode keycode;
        bool special_key;

        bool alt_down;
        bool ctrl_down;
        bool shift_down;

    public:
        KeyPress()
        {
            keycode = (KeyCode) 0;
            special_key = false;
            alt_down = ctrl_down = shift_down = false;
        };
        /* Constructs and immediately calls setKeyCode with parameters */
        KeyPress(const KeyCode &kc, bool special_key);
        /* And this one also calls setAltDown+setCtrlDown+setShiftDown */
        KeyPress(const KeyCode &kc, bool special_key, bool alt_down, bool ctrl_down, bool shift_down);

        /* Modifiers are automatically set if special_key is true. */
        void setKeyCode(const KeyCode &kc, bool special_key);

        bool isSpecialKey() const { return special_key; };
        KeyCode getKeyCode() const { return keycode; };

        void setAltDown(bool down) { alt_down = down; };
        void setCtrlDown(bool down) { ctrl_down = down; };
        void setShiftDown(bool down) { shift_down = down; };

        bool isAltDown() const { return alt_down; };
        bool isCtrlDown() const { return ctrl_down; };
        bool isShiftDown() const { return shift_down; };

        bool operator==(const KeyPress &kp) const
        {
            if (keycode == kp.keycode &&
                special_key == kp.special_key &&
                alt_down == kp.alt_down &&
                ctrl_down == kp.ctrl_down &&
                shift_down == kp.shift_down)
                return true;
            return false;
        }
        bool operator<(const KeyPress &kp) const
        {
            if (keycode < kp.keycode) return true;
            if (keycode > kp.keycode) return false;

            if (!special_key && kp.special_key) return true;
            if (special_key && !kp.special_key) return false;
            if (!alt_down && kp.alt_down) return true;
            if (alt_down && !kp.alt_down) return false;
            if (!ctrl_down && kp.ctrl_down) return true;
            if (ctrl_down && !kp.ctrl_down) return false;
            if (!shift_down && kp.shift_down) return true;
            if (shift_down && !kp.shift_down) return false;

            return false;
        }
};


};

#endif
