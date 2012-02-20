#ifndef termemu_h
#define termemu_h

/*
Things to do:
- Symbols are stored as UChar32, one of those per character. The library
  also assumes this will hold any unicode character. This could be Bad.
  (maybe move to ICU and store UChar32 instead? like rest of Trankesbel)
  The way it's coded now was due to a misunderstanding how wchars work.

  The library assumes UTF-8 everywhere.
*/

#include <string>
#include <vector>
#include <stdio.h>
#include "cpp_regexes.h"
#include <stddef.h>
#include <unicode/ustring.h>

class TerminalTile
{
    private:
        UChar32 Symbol;
        unsigned int ForegroundColor;
        unsigned int BackgroundColor;
        bool Inverse;
        bool Bold;

    public:
        TerminalTile()
        {
            Symbol = BackgroundColor = 0;
            Inverse = false;
            Bold = false;
            ForegroundColor = 9;
        };

        TerminalTile(UChar32 s, unsigned int f, unsigned int b, bool inverse = false, bool bold = false)
        {
            Symbol = s;
            ForegroundColor = f;
            BackgroundColor = b;
            Inverse = inverse;
            Bold = bold;
        };

        UChar32 getSymbol() const { return Symbol; };
        unsigned int getForegroundColor() const { return ForegroundColor; };
        unsigned int getBackgroundColor() const { return BackgroundColor; };
        bool getInverse() const { return Inverse; };
        bool getBold() const { return Bold; };

        void setSymbol(UChar32 s) { Symbol = s; };
        void setForegroundColor(unsigned int f) { ForegroundColor = f; };
        void setBackgroundColor(unsigned int b) { BackgroundColor = b; };
        void setInverse(bool i) { Inverse = i; };
        void setBold(bool b) { Bold = b; };

        bool operator==(const TerminalTile &t) const
        {
            if (Symbol == t.Symbol &&
                ForegroundColor == t.ForegroundColor &&
                BackgroundColor == t.BackgroundColor &&
                Inverse == t.Inverse &&
                Bold == t.Bold)
                return true;
            return false;
        };
        bool operator!=(const TerminalTile &t) const
        {
            return !((*this) == t);
        }
        bool operator<(const TerminalTile &t) const
        {
            if (Symbol != t.Symbol)
                return Symbol < t.Symbol;
            if (ForegroundColor != t.ForegroundColor)
                return ForegroundColor < t.ForegroundColor;
            if (BackgroundColor != t.BackgroundColor)
                return BackgroundColor < t.BackgroundColor;
            if (!Inverse && t.Inverse)
                return true;
            if (Inverse && !t.Inverse)
                return false;
            if (!Bold && t.Bold)
                return true;
            if (Bold && !t.Bold)
                return false;
            return false;
        }
};

/* Terminal has width and height.
* The leftmost, top character has coordinates of (0, 0)
* The rightmost, bottom character has coordinates of (width-1, height-1)
* This is sligthly different for vt102 control sequences, which start counting from 1.
*
* Emulation is not 100%. It is enough for nethack but many other programs
* don't work properly.
*/
class Terminal
{
    private:
        std::vector<TerminalTile> Tiles;

        unsigned int Width, Height;

        std::string ReceiveBuffer;
        std::string buffered_str;

        bool WrapAround;

        bool red_blue_swap;

        // Current color settings
        unsigned int ForegroundColor, BackgroundColor;
        // Inverse and bold
        bool Inverse, Bold;
        // Visible cursor
        bool VisibleCursor;

        // Default values for above
        unsigned int DefaultForegroundColor, DefaultBackgroundColor;
        bool DefaultInverse, DefaultBold;

        // Scroll region
        unsigned int TopScrolling, BottomScrolling;

        // Cursor position
        unsigned int CursorX, CursorY;
        // Saved cursor position
        unsigned int SavedCursorX, SavedCursorY;

        // When matching sequences, this stores the number values in control sequence
        std::vector<unsigned int> Numbers;
        unsigned int SequenceCharacter;

        // Returns the control sequence string matches or -1 if it doesn't match.
        int matchesControlSequence(const char* str, unsigned int maxlen, int &advance, bool &atleast_one_partial);

        // Adds a single character to terminal
        void addCharacter(UChar32 c);

        // Scrolls the terminal up.
        void scrollUp(unsigned int lines);
        // Scrolls the terminal down
        void scrollDown(unsigned int lines);

        void eraseAll();
        void eraseBelow();
        void eraseAbove();
        void eraseLineBefore();
        void eraseLineAfter();
        void eraseLine();
        void eraseCharacters(int numcharacters);

        void insertLines(unsigned int numlines);

        void deleteCharacters(unsigned int numcharacters);
        void deleteLines(unsigned int numlines);

    public:
        Terminal(unsigned int width, unsigned int height);
        Terminal();

        // Set red/blue flipping on. If set, red and blue channels
        // are swapped. Red blue swapping is only present in
        // updateCycle/restrictedUpdateCycle functions.
        void setRedBlueSwap(bool enable);
        bool getRedBlueSwap() const;

        // Puts a character in the terminal
        void setTile(int x, int y, const TerminalTile &t);
        // Fills a rectangular area with copies of given tile
        void fillRectangle(int x, int y, int w, int h, const TerminalTile &t);
        // Prints a string starting from the given position and going to right.
        // Does not use cursor position or affect it in any way.
        // If the string can't fit on one line, the rest of it is put nowhere.
        void printString(const char* str, int x, int y, unsigned int ForegroundColor, unsigned int BackgroundColor, bool Inverse, bool Bold);
        // The C++ string version of the above
        void printString(std::string str, int x, int y, unsigned int ForegroundColor, unsigned int BackgroundColor, bool Inverse, bool Bold)
        { printString(str.c_str(), x, y, ForegroundColor, BackgroundColor, Inverse, Bold); };
        // Sets attributes. Works identically to printString, except that it won't touch symbols. n is the number
        // of squares to change.
        void setAttributes(int n, int x, int y, unsigned int ForegroundColor, unsigned int BackgroundColor, bool Inverse, bool Bold);

        // Copies terminal data to this class. If delim is not null, only copies
        // when the target symbol is in that group (requires same size for terminals)
        void copy(Terminal* t, const char* character_group = NULL);
        // C++ string version of copy
        void copy(Terminal* t, std::string character_group)
        { copy(t, character_group.c_str()); };

        // Copies terminal data from another terminal to this class but does not resize or modify current cursor attributes or position.
        // The terminals MUST be the same size or bad things can happen.
        void copyPreserve(const Terminal* t);
        void copyPreserve(const Terminal* t, const char* character_group);
        void copyPreserve(const Terminal* t, std::string character_group)
        { copyPreserve(t, character_group.c_str()); }

        int getCursorX() const { return CursorX; };
        int getCursorY() const { return CursorY; };
        int getNumberOfRows() const { return Height; };
        int getHeight() const { return Height; };
        int getWidth() const { return Width; };
        TerminalTile getTile(int x, int y) const { return Tiles[x + y * Width]; };

        void setCursorY(unsigned int y)
        {
            CursorY = (y >= Height) ? (Height-1) : y;
        }
        void setCursorX(unsigned int x)
        {
            CursorX = (x >= Width) ? (Width-1) : x;
        }
        void setCursorXY(unsigned int x, unsigned int y)
        {
            CursorX = (x >= Width) ? (Width-1) : x;
            CursorY = (y >= Height) ? (Height-1) : y;
        }
        bool isCursorVisible()
        {
            return VisibleCursor;
        }
        void setCursorVisibility(bool vis) { VisibleCursor = vis; };

        // The state of terminal tiles after resize is undefined.
        void resize(unsigned int width, unsigned int height);

        // Using string data, updates terminal.
        void feedString(const char* str, unsigned int length);
        // C++ string version of feedString
        void feedString(std::string str)
        { feedString(str.c_str(), str.size()); };

        // Attempts to match regex at every position.
        // Set x and y to 0 and 0 when calling for the first time.
        // The call sets them to the position where match occurs.
        // You can then call the method again for next match.
        // Returns false when there are no more matches.
        bool matchRe(Regex &re, unsigned int &x, unsigned int &y);

        // Returns a line in terminal
        std::string getLine(unsigned int y) const;

        // Returns a string that can be fed to another terminal to show output of this terminal.
        std::string updateCycle() const;

        // Like Terminal::updateCycle() but only updates at those positions that are different
        // from the given terminal 'source'.
        // If delim_characters is not NULL, it also doesn't write at characters
        // that have those symbols.
        std::string restrictedUpdateCycle(const Terminal* source, const char* delim_characters) const;
        // Same as above, but with a reference to string that'll be used.
        void restrictedUpdateCycle(const Terminal* source, const char* delim_characters, std::string *result) const;

        // Overlays another terminal on top of this. If a tile in source terminal is equal to source_delim,
        // that square is not drawn.
        void overlay(const Terminal* source, const TerminalTile &source_delim);
};

#endif

