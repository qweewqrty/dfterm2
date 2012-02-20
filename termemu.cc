#include <string>
#include <sstream>
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "termemu.h"
#include "utf8.h"

// Hack to make this compile on MSVC++
#ifdef _WIN32
#define snprintf _snprintf
#endif

Terminal::Terminal()
{
    VisibleCursor = true;
    WrapAround = false;
    
    CursorX = 0;
    CursorY = 0;
    SavedCursorX = 0;
    SavedCursorY = 0;
    ForegroundColor = 7;
    BackgroundColor = 0;
    Bold = false;
    Inverse = false;
    
    DefaultForegroundColor = 7;
    DefaultBackgroundColor = 0;
    DefaultBold = false;
    DefaultInverse = false;
    
    Width = 80;
    Height = 24;

    red_blue_swap = false;
    
    resize(Width, Height);
}

Terminal::Terminal(unsigned int w, unsigned int h)
{
    WrapAround = false;

    CursorX = 0;
    CursorY = 0;
    SavedCursorX = 0;
    SavedCursorY = 0;
    ForegroundColor = 7;
    BackgroundColor = 0;
    Bold = false;
    Inverse = false;
    
    DefaultForegroundColor = 7;
    DefaultBackgroundColor = 0;
    DefaultBold = false;
    DefaultInverse = false;
    
    red_blue_swap = false;

    resize(w, h);
};

void Terminal::resize(unsigned int w, unsigned int h)
{
    Tiles.resize(w * h, TerminalTile(' ', 7, 0, false, false));

    Width = w;
    Height = h;
    
    TopScrolling = 0;
    BottomScrolling = h-1;
    if (CursorY >= h)
        CursorY = h;
    if (CursorX >= w)
        CursorX = w;
};

/* TODO: add the names for these terminal sequences */
const char* controlseqs[] = 
    { 
    "\x1b[N%@",                  // 0
    "\x1b[N%A",                  // 1
    "\x1b[N%B",                  // 2
    "\x1b[N%C",                  // 3
    "\x1b[N%D",                  // 4
    "\x1b[N%E",                  // 5
    "\x1b[N%F",                  // 6
    "\x1b[N%G",                  // 7
    "\x1b[N%;N%H",               // 8
    "\x1b[N%J",                  // 9
    "\x1b[?N%J",                 // 10
    "\x1b[N%L",                  // 11
    "\x1b[N%M",                  // 12
    "\x1b[N%P",                  // 13
    "\x1b[N%S",                  // 14
    "\x1b[N%T",                  // 15
    "\x1b[N%K",                  // 16
    "\x1b[?N%K",                 // 17
    "\x1b[N%;N%;N%;N%;N%T",      // 18
    "\x1b[N%X",                  // 19
    "\x1b[N%Z",                  // 20
    "\x1b[N%`",                  // 21
    "\x1b[N%b",                  // 22
    "\x1b[n%m",                  // 23
    "\x1b[H",                    // 24
    "\x1b[?n%l",                 // 25
    "\x1b[?n%h",                 // 26
    "\x1b[?n%r",                 // 27
    "\x1b>",                     // 28
    "\x1b(C%",                   // 29
    "\x1b[N%d",                  // 30
    "\x1b[N%;N%r",               // 31
    "\x1b]N%;s%\x1b\\",          // 32
    "\x1b]N%;s%\x07",            // 33
    "\x1bM",                     // 34 (reverse index)
    "\x1b[n%h",                  // 35 (SM)
    "\x1b" "7",                  // 36 (DECSC)
    "\x1b" "8",                  // 37 (DECRC)
    "\x1b[N%;N%f",               // 38 (HVP)
    "\x1b[N%c",                  // 39 (Primary DA)
    "\x1b#8",                    // 40 (DECALN)
    "\x1b" "D",                  // 41 (index)
    "\x1b" "E",                  // 42 (next line)
    0 };

const char* controlseq_names[] =
    {
    "ICH",
    "CUU",
    "CUD",
    "CUF",
    "CUB",
    "CNL",
    "CPL",
    "CHA",
    "CUP",
    "ED",
    "DECSED",
    "IL",
    "DL",
    "DCH",
    "SU",
    "SD",
    "EL",
    "DECSEL",
    "(mousetracking)",
    "ECH",
    "CBT",
    "HPA",
    "REP",
    "SGR",
    "CUP (without parameters)",
    "DECRST",
    "DECSET",
    "(restore DEC private modes)",
    "DECPNM",
    "Designate G0",
    "VPA",
    "(set scrolling region)",
    "OSC Ps ; Pt ST",
    "OSC Ps ; Pt BEL",
    "RI",
    "SM",
    "DECSC",
    "DECRC",
    "HVP",
    "Primary DA",
    "DECALN",
    "IND",
    "NEL",
    0
    }; 

int Terminal::matchesControlSequence(const char* str, unsigned int maxlen, int &advance, bool &atleast_one_partial)
{
    #define MAX_NUMBERS 10

    advance = 1;
    unsigned int numbers[MAX_NUMBERS];
    unsigned int number1 = 0;
    atleast_one_partial = false;
    int partial_matches = 0;
    bool continue_after_while = false;
    
    unsigned int i1, i2, i3;
    for (i1 = 0; controlseqs[i1]; i1++)
    {
        const char* seq = controlseqs[i1];
        bool not_match = false;
       
        number1 = 0;
        
        for (i2 = 0, i3 = 0; i3 < maxlen && seq[i2] && str[i3]; i2++, i3++)
        {
            if (seq[i2] == 'C' && seq[i2+1] == '%') // Character
            {
                i2++;
                SequenceCharacter = str[i3];
                continue;
            };
            
            if (seq[i2] == 'n' && seq[i2+1] == '%') // Optionally many numbers
            {
                ++i2;
                continue_after_while = false;
                
                while(1)
                {
                    unsigned int read_number = 0;
                    unsigned int old_i3 = i3;
                    
                    for (; str[i3] >= '0' && str[i3] <= '9'; i3++)
                    {
                        read_number *= 10;
                        read_number += (str[i3] - '0');
                    }
                    
                    if (old_i3 != i3)
                    {
                        numbers[number1++] = read_number;
                        if (number1 >= MAX_NUMBERS)
                        {
                            continue_after_while = true;
                            break;
                        }
                    }
                    
                    if (str[i3] == ';')
                    {
                        ++i3;
                        continue;
                    };
                    
                    --i3;
                    
                    continue_after_while = true;
                    break;
                }; 
                
                if (continue_after_while)
                    continue;
            };
            
            if (not_match == true)
                continue;
            
            if (seq[i2] == 's' && seq[i2+1] == '%') // Optional string
            {
                i2++;
                
                if (str[i3] < ' ' || str[i3] > 255)
                {
                    i3--;
                    continue;
                };

                /* Currently discard strings */
               
                // Match number
                for (; str[i3] >= ' ' && str[i3] <= 255; i3++)
                { }

                i3--;

                continue;
            }
            
            if (seq[i2] == 'N' && seq[i2+1] == '%') // Optional number
            {
                i2++;
                
                unsigned int read_number = 0;
                unsigned int old_i3 = i3;
                
                for (; str[i3] >= '0' && str[i3] <= '9'; i3++)
                {
                    read_number *= 10;
                    read_number += (str[i3] - '0');
                }
                
                if (old_i3 != i3)
                    numbers[number1++] = read_number;

                --i3;
                continue;
            };
            
            if (seq[i2] != str[i3])
            {
                not_match = true;
                break;
            };
        };
        
        if (not_match == true)
        {
            partial_matches--;
            continue;
        };
        if (seq[i2] != 0)
        {
            atleast_one_partial = true;
            continue;
        };
        
        Numbers.resize(number1);
        for (i2 = 0; i2 < number1; ++i2)
            Numbers[i2] = numbers[i2];
        advance = i3;
        
        return i1;
    };
    
    partial_matches = i1 + partial_matches;
    
    return -1;
};

void Terminal::eraseAbove()
{
    unsigned int i1, i2;
    for (i1 = 0; i1 < CursorY; i1++)
    {
        for (i2 = 0; i2 < Width; i2++)
            Tiles[i2 + i1 * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);
    };
            
    for (i1 = 0; i1 < CursorX; i1++)
        Tiles[i1 + CursorY * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);
};

void Terminal::eraseBelow()
{
    unsigned int i1, i2;
    for (i1 = CursorY+1; i1 < Height; i1++)
    {
        for (i2 = 0; i2 < Width; i2++)
            Tiles[i2 + i1 * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);
    };
            
    for (i1 = CursorX; i1 < Width; i1++)
        Tiles[i1 + CursorY * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);
};

void Terminal::eraseAll()
{
    unsigned int i1, i2;
    for (i1 = 0; i1 < Height; i1++)
    {
        for (i2 = 0; i2 < Width; i2++)
            Tiles[i2 + i1 * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);
    };
};

void Terminal::eraseLineBefore()
{
    unsigned int i1;
    for (i1 = 0; i1 < CursorX; i1++)
        Tiles[i1 + CursorY * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);
};

void Terminal::eraseLineAfter()
{
    unsigned int i1;
    for (i1 = CursorX; i1 < Width; i1++)
        Tiles[i1 + CursorY * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);
};

void Terminal::eraseLine()
{
    unsigned int i1;
    for (i1 = 0; i1 < Width; i1++)
        Tiles[i1 + CursorY * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);
};

void Terminal::insertLines(unsigned int numlines)
{
    unsigned int i1, i2, i3;
    for (i1 = 0; i1 < numlines; i1++)
        for (i2 = BottomScrolling; i2 > CursorY; i2--)
            for (i3 = 0; i3 < Width; i3++)
                Tiles[i3 + i2 * Width] = Tiles[i3 + (i2-1) * Width];
    for (i1 = 0; i1 < Width; i1++)
        Tiles[i1 + CursorY * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);
}

void Terminal::deleteLines(unsigned int numlines)
{
    unsigned int i1, i2, i3;
    for (i1 = 0; i1 < numlines; i1++)
        for (i2 = CursorY; i2 < BottomScrolling; i2++)
            for (i3 = 0; i3 < Width; i3++)
                Tiles[i3 + i2 * Width] = Tiles[i3 + (i2+1) * Width];
    for (i1 = 0; i1 < Width; i1++)
        Tiles[i1 + (Height-1) * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);
}

void Terminal::eraseCharacters(int numcharacters)
{
    unsigned int i1;
    for (i1 = CursorX; i1 < CursorX + numcharacters; i1++)
        Tiles[i1 + CursorY * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);
}

void Terminal::deleteCharacters(unsigned int numcharacters)
{
    unsigned int i1, i2;
    for (i2 = 0; i2 < numcharacters; i2++)
        for (i1 = CursorX; i1 < Width-1; i1++)
            Tiles[i1 + CursorY * Width] = Tiles[i1 + 1 + CursorY * Width];
    Tiles[Width-1 + CursorY * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);
};

void Terminal::feedString(const char* str, unsigned int len)
{
    buffered_str = buffered_str + std::string(str, len);
    
    len = buffered_str.size();
    const char *bstr = buffered_str.c_str();
    
    unsigned int i1;
    for (i1 = 0; i1 < len; i1++)
        if (!bstr[i1])
            len = bstr[i1];

    for (i1 = 0; i1 < len; i1++)
    {
        int advance = 1;
        
        bool atleast_one_partial;
        int control_seq = matchesControlSequence(&bstr[i1], len - i1, advance, atleast_one_partial);
        if (control_seq == -1)
        {
            if (atleast_one_partial)
                break;
            
            if (bstr[i1] == 0)
                continue;
            
            int cursor = 0;
            unsigned int symbol = fetchUTF8Unicode(&bstr[i1], cursor);
            if (symbol == 0 && (signed int) i1 >= (signed int) len-4) // Partial utf-8 string
                break;
            
            addCharacter(symbol);
            /*if (symbol < 32)
                std::cout << "Character: " << (long) symbol << std::endl;*/
            i1 += cursor-1;
            continue;
        };
        
        i1 += advance - 1;

        /*
        std::cout << "Control sequence: " << controlseq_names[control_seq] << std::endl;
        std::cout << "Numerical parameters: ";*/
        std::vector<unsigned int>::iterator i2;
        /*
        for (i2 = Numbers.begin(); i2 != Numbers.end(); ++i2)
            std::cout << (*i2) << " ";
        std::cout << std::endl;*/
        
        int mode, lines, characters, row;
        switch(control_seq)
        {
            case 0:    // Insert blank characters
            break;
            case 1:    // Move cursor up
                if (Numbers.size() > 0)
                {
                    if (CursorY >= Numbers[0] && Numbers[0] != 0)
                        CursorY = CursorY - Numbers[0];
                    else
                        CursorY = 0;
                }
                else if (CursorY > 0)
                    CursorY--;
                    
                if (CursorY < 0)
                    CursorY = 0;
            break;
            case 2:    // Move cursor down
                if (Numbers.size() > 0 && Numbers[0] != 0)
                    CursorY = CursorY + Numbers[0];
                else
                    CursorY++;
                    
                if (CursorY >= Height-1)
                    CursorY = Height-1;
            break;
            case 3:    // Forward
                if (Numbers.size() > 0 && Numbers[0] != 0)
                    CursorX = CursorX + Numbers[0];
                else
                    CursorX++;
            break;
            case 4:    // Backward
                if (Numbers.size() > 0 && Numbers[0] != 0)
                {
                    if (CursorX >= Numbers[0])
                        CursorX = CursorX - Numbers[0];
                    else
                        CursorX = 0;
                }
                else if (CursorX > 0)
                    CursorX--;
                    
                if (CursorX < 0)
                    CursorX = 0;
            break;
            case 5:    // Next lines
                if (Numbers.size() > 0 && Numbers[0] != 0)
                    CursorY = CursorY + Numbers[0];
                else
                    CursorY++;
                    
                if (CursorY >= Height-1)
                    CursorY = Height-1;
            break;
            case 6:    // Previous lines
                if (Numbers.size() > 0 && Numbers[0] != 0)
                    CursorY = CursorY - Numbers[0];
                else if (CursorY > 0)
                    CursorY--;
                    
                if (CursorY < 0)
                    CursorY = 0;
            break;
            case 7:    // Column
            case 21:
                if (Numbers.size() > 0)
                    CursorX = Numbers[0]-1;
                else
                    CursorX = 0;
            break;
            case 8:    // Absolute position
            case 38:
                if (Numbers.size() > 1)
                    CursorX = Numbers[1]-1;
                else
                    CursorX = 0;
                
                if (Numbers.size() > 0)
                    CursorY = Numbers[0]-1;
                else
                    CursorY = 0;
                
                if (CursorX < 0) CursorX = 0;
                if (CursorX >= Width-1) CursorX = Width-1;
                if (CursorY < 0) CursorY = 0;
                if (CursorY >= Height-1) CursorY = Height-1;
            break;
            case 9:    // Clear terminal
                mode = 0;
                if (Numbers.size() > 0)
                    mode = Numbers[0];
                
                if (mode == 0)
                    eraseBelow();
                else if (mode == 1)
                    eraseAbove();
                else if (mode == 2)
                    eraseAll();
            break;
            case 11:
                if (Numbers.size() > 0)
                    insertLines(Numbers[0]);
                else
                    insertLines(1);
            break;
            case 12:
                if (Numbers.size() > 0)
                    deleteLines(Numbers[0]);
                else
                    deleteLines(1);
            break;
            case 13:
                if (Numbers.size() > 0)
                    deleteCharacters(Numbers[0]);
                else
                    deleteCharacters(1);
            break;
            case 14:
                lines = 0;
                if (Numbers.size() > 0)
                    lines = Numbers[0];
                if (lines <= 0)
                    lines = 1;
                
                scrollUp(lines);
            break;
            case 15:
                lines = 0;
                if (Numbers.size() > 0)
                    lines = Numbers[0];
                if (lines <= 0)
                    lines = 1;
                
                scrollDown(lines);
            break;
            case 16:
            case 17:
                mode = 0;
                if (Numbers.size() > 0)
                    mode = Numbers[0];
                
                if (mode == 0)
                    eraseLineAfter();
                else if (mode == 1)
                    eraseLineBefore();
                else if (mode == 2)
                    eraseLine();
            break;
            case 19:
                characters = 1;
                if (Numbers.size() > 0)
                    characters = Numbers[0];
                    
                eraseCharacters(characters);
            break;
            case 20:
                CursorX = CursorX - (CursorX % 8);
            break;
            case 23: // Attribute change
                if (Numbers.size() == 0)
                {
                    ForegroundColor = 7;
                    BackgroundColor = 0;
                    Bold = false;
                    Inverse = false;
                }
                
                for (i2 = Numbers.begin(); i2 != Numbers.end(); i2++)
                {
                    switch (*i2)
                    {
                        case 0: // Default attributes
                            ForegroundColor = 7;
                            BackgroundColor = 0;
                            Bold = false;
                            Inverse = false;
                        break;
                        case 1:
                            Bold = true;
                        break;
                        case 7:
                            Inverse = true;
                        break;
                        case 27:
                            Inverse = false;
                        break;
                        case 22:
                            Bold = false;
                        break;
                        case 30:
                        case 31:
                        case 32:
                        case 33:
                        case 34:
                        case 35:
                        case 36:
                        case 37:
                        case 38:
                        case 39:
                            ForegroundColor = (*i2) - 30;
                        break;
                        case 40:
                        case 41:
                        case 42:
                        case 43:
                        case 44:
                        case 45:
                        case 46:
                        case 47:
                        case 48:
                        case 49:
                            BackgroundColor = (*i2) - 40;
                        break;
                    };
                };
            break;
            case 24:
                CursorX = 0;
                CursorY = 0;
            break;
            case 25:
                for (i2 = Numbers.begin(); i2 != Numbers.end(); ++i2)
                {
                    switch (*i2)
                    {
                        case 25: // Hide cursor
                            VisibleCursor = false;
                        break;
                        case 7: // Wraparound mode unset
                            WrapAround = false;
                        break;
                        case 1049: // Go to normal screen buffer and restore cursor
                            CursorX = SavedCursorX;
                            CursorY = SavedCursorY;
                            if (CursorX >= Width) CursorX = Width-1;
                            if (CursorY >= Height) CursorY = Height-1;
                        break;
                    }
                }
            break;
            case 26:
                for (i2 = Numbers.begin(); i2 != Numbers.end(); ++i2)
                {
                    switch (*i2)
                    {
                        case 25: // Show cursor
                            VisibleCursor = true;
                        break;
                        case 7: // Wraparound mode set
                            WrapAround = true;
                        break;
                        case 1049: // Clear alternate screen buffer and save cursor
                                   // (we don't implement alternate screen buffer)
                            SavedCursorX = CursorX;
                            SavedCursorY = CursorY;
                        break;
                    }
                }
            break;
            case 30:
                row = 0;
                if (Numbers.size() > 0)
                    row = Numbers[0] - 1;
                
                CursorY = row;
            break;
            case 31:
                if (Numbers.size() > 0)
                    TopScrolling = Numbers[0]-1;
                else
                    TopScrolling = 0;
                
                if (Numbers.size() > 1)
                    BottomScrolling = Numbers[1]-1;
                else
                    BottomScrolling = Height-1;
                
            break;
            case 32:
            case 33:
            break;
            case 34: // reverse index
                if (CursorY == TopScrolling)
                    scrollDown(1);
                else
                    CursorY--;
            break;
            case 35: // (SM), ignored for now
            break;
            case 36: // Save cursor
                SavedCursorX = CursorX;
                SavedCursorY = CursorY;
            break;
            case 37: // Restore cursor
                CursorX = SavedCursorX;
                CursorY = SavedCursorY;
                if (CursorX >= Width) CursorX = Width-1;
                if (CursorY >= Height) CursorY = Height-1;
            break;
            case 40: // DECALN, fill screen with Es
            {
                unsigned int x, y;
                for (y = 0; y < Height; ++y)
                {
                    unsigned int offset = y * Width;
                    for (x = 0; x < Width; ++x)
                        Tiles[x + offset] = TerminalTile('E', 7, 0, false, false);
                }
            }
            break;
            case 41: // index
            case 42:
                if (CursorY == BottomScrolling)
                    scrollUp(1);
                else
                    CursorY++;
            break;
        };
    };
    
    buffered_str = buffered_str.substr(i1);
};

void Terminal::copy(Terminal* t, const char* character_group)
{
    Numbers = t->Numbers;
    SequenceCharacter = t->SequenceCharacter;
    
    VisibleCursor = t->VisibleCursor;

    red_blue_swap = t->red_blue_swap;
    
    TopScrolling = t->TopScrolling;
    BottomScrolling = t->BottomScrolling;
    
    ForegroundColor = t->ForegroundColor;
    BackgroundColor = t->BackgroundColor;
    DefaultForegroundColor = t->DefaultForegroundColor;
    DefaultBackgroundColor = t->DefaultBackgroundColor;
    DefaultBold = t->DefaultBold;
    DefaultInverse = t->DefaultInverse;
    
    Inverse = t->Inverse;
    Bold = t->Bold;
    
    ReceiveBuffer = t->ReceiveBuffer;
    buffered_str = t->buffered_str;
    
    if (!character_group || Width != t->Width || Height != t->Height)
        Tiles = t->Tiles;
    else
    {
        unsigned int i1, i2, len = Tiles.size();
        for (i1 = 0; i1 < len; i1++)
        {
            char target_c = Tiles[i1].getSymbol();
            for (i2 = 0; character_group[i2]; i2++)
                if (target_c == character_group[i2])
                    break;
            if (!character_group[i2])
                continue;
            
            Tiles[i1] = t->Tiles[i1];
        }
    }
    
    Width = t->Width;
    Height = t->Height;
    
    CursorX = t->CursorX;
    CursorY = t->CursorY;
}

void Terminal::copyPreserve(const Terminal* t)
{
    assert(t->Width == Width);
    assert(t->Height == Height);
   
    Tiles = t->Tiles;
}

void Terminal::copyPreserve(const Terminal* t, const char* character_group)
{
    assert(t->Width == Width);
    assert(t->Height == Height);

    assert(character_group);

    unsigned int i1, i2, i3;

    for (i2 = 0; i2 < (unsigned int) Height; ++i2)
    {
        unsigned int offset = i2 * Width;
        for (i1 = 0; i1 < (unsigned int) Width; ++i1)
        {
            char target_c = Tiles[i1 + offset].getSymbol();
            for (i3 = 0; character_group[i3]; ++i3)
                if (target_c == character_group[i3])
                    break;
            if (!character_group[i3])
                continue;

            Tiles[i1 + offset] = t->Tiles[i1 + offset];
        }
    }
}

void Terminal::scrollDown(unsigned int lines)
{
    unsigned int i1, i2;
    for (i1 = BottomScrolling; i1 > TopScrolling && i1 <= BottomScrolling; i1--)
    {
        for (i2 = 0; i2 < Width; i2++)
            Tiles[i2 + i1 * Width] = Tiles[i2 + (i1-1) * Width];
    }
    
    for (i2 = 0; i2 < Width; i2++)
        Tiles[i2 + TopScrolling * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);

    if (lines > 1)
        scrollDown(lines-1);
};

void Terminal::scrollUp(unsigned int lines)
{
    unsigned int i1, i2;
    for (i1 = TopScrolling; i1 < BottomScrolling; i1++)
    {
        for (i2 = 0; i2 < Width; i2++)
            Tiles[i2 + i1 * Width] = Tiles[i2 + (i1+1) * Width];
    }
    
    for (i2 = 0; i2 < Width; i2++)
        Tiles[i2 + BottomScrolling * Width] = TerminalTile(' ', ForegroundColor, BackgroundColor, Inverse, Bold);

    if (lines > 1)
        scrollUp(lines-1);
};

void Terminal::addCharacter(UChar32 c)
{
    if (c == '\r')
    {
        CursorX = 0;
        return;
    };
    
    if (c == '\n')
    {
        CursorY++;
        if (CursorY > BottomScrolling)
        {
            CursorY = BottomScrolling;
            scrollUp(1);
        }
        return;
    };
    
    if (c == 9)
    {
        if (CursorX % 8 > 0)
            CursorX = CursorX + (8 - CursorX % 8);
        else
            CursorX += 8;
        
        if (CursorX > Width-1)
        {
            CursorX -= Width;
            CursorY++;
            
            if (CursorY > BottomScrolling)
            {
                CursorY = BottomScrolling;
                scrollUp(1);
            };
        };
        
        return;
    };
    if (c == 7)
        return;
    if (c == 8)
    {
        if (CursorX > 0)
            CursorX--;
        return;
    };

    while (CursorX >= Width)
    {
        CursorX -= Width;
        CursorY++;
        if (CursorY > BottomScrolling)
        {
            CursorY = BottomScrolling;
            scrollUp(1);
        };
    };
    
    Tiles[CursorX + CursorY * Width] = TerminalTile(c, ForegroundColor, BackgroundColor, Inverse, Bold);
    
    CursorX++;
};

static void char_to_ss(char* &ss, const char &source, size_t &ss_cursor, size_t &ss_size)
{
    if (ss_cursor == ss_size)
    {
        char* newbuf = new char[ss_size + 8192];
        memcpy(newbuf, ss, ss_cursor);
        delete[] ss;
        ss = newbuf;
        ss_size += 8192;
    }
    ss[ss_cursor++] = source;
}

static void add_to_ss(char* &ss, const char* source, const size_t &source_size, size_t &ss_cursor, size_t &ss_size)
{
    if (ss_cursor + source_size > ss_size)
    {
        char* newbuf = new char[ss_size + 8192];
        memcpy(newbuf, ss, ss_cursor);
        delete[] ss;
        ss = newbuf;
        ss_size += 8192;
    }
    memcpy(&ss[ss_cursor], source, source_size);
    ss_cursor += source_size;
}

std::string Terminal::restrictedUpdateCycle(const Terminal* source, const char* delim_characters) const
{
    std::string result;
    restrictedUpdateCycle(source, delim_characters, &result);
    return result;
}

void Terminal::restrictedUpdateCycle(const Terminal* source, const char* delim_characters, std::string *str) const
{
    assert(str);
    std::string &result = *str;

    if (source)
    {
        assert(source->Width == Width);
        assert(source->Height == Height);
    }

    char* ss = new char[8192];
    size_t ss_size = 8192;
    size_t ss_cursor = 14;
    memcpy(ss, "\x1b[0m\x1b[40m\x1b[30m", 14);
    
    unsigned int CurrentForegroundColor = 0;
    unsigned int CurrentBackgroundColor = 0;
    bool CurrentBold = false;
    bool CurrentInverse = false;
    
    bool SomethingWritten = false;
    
    unsigned int CurrentRealCursorY = Height;
    unsigned int CurrentRealCursorX = Width;
    
    unsigned int i1, i2;
    for (i1 = 0; i1 < Height; i1++)
    {
        unsigned int offset = i1 * Width;
        for (i2 = 0; i2 < Width; i2++)
        {
            const TerminalTile &t = Tiles[i2 + offset];
            if (source && i1 < source->Height && i2 < source->Width)
            {
                const TerminalTile &s = source->Tiles[i2 + i1 * source->Width];
                
                if (s.getSymbol() == t.getSymbol() &&
                    s.getForegroundColor() == t.getForegroundColor() &&
                    s.getBackgroundColor() == t.getBackgroundColor() &&
                    s.getBold() == t.getBold() &&
                    s.getInverse() == t.getInverse())
                    continue;
                if (delim_characters)
                {
                    unsigned int i3;
                    for (i3 = 0; delim_characters[i3]; i3++)
                        if (t.getSymbol() == (UChar32) delim_characters[i3])
                            break;
                    if (delim_characters[i3])
                        continue;
                }
            }
            
            if (CurrentRealCursorY != i1 || CurrentRealCursorX != i2)
            {
                char line[50];
                size_t len = snprintf(line, 50, "\x1b[%d;%dH", i1+1, i2+1);

                add_to_ss(ss, line, len, ss_cursor, ss_size);
                CurrentRealCursorY = i1;
                CurrentRealCursorX = i2;
            };
            
            SomethingWritten = true;
            
            UChar32 symbol = t.getSymbol();
            unsigned int t_fore = t.getForegroundColor();
            unsigned int t_back = t.getBackgroundColor();
            if (t_fore > 9) t_fore = 0;
            if (t_back > 9) t_back = 0;
            if (t_fore == 9) t_fore = 7;
            if (t_back == 9) t_back = 0;
            if (VisibleCursor && i1 == CursorY && i2 == CursorX)
            {
                unsigned int temp = t_fore;
                t_fore = t_back;
                t_back = temp;
            }

            bool t_bold = t.getBold();
            bool t_inve = t.getInverse();

            if (red_blue_swap)
            {
                if (t_fore == 1) t_fore = 4;
                else if (t_fore == 3) t_fore = 6;
                else if (t_fore == 4) t_fore = 1;
                else if (t_fore == 6) t_fore = 3;
                if (t_back == 1) t_back = 4;
                else if (t_back == 3) t_back = 6;
                else if (t_back == 4) t_back = 1;
                else if (t_back == 6) t_back = 3;
            }
           
            #define MAX_ATTRIBUTES 4
            int new_attributes[MAX_ATTRIBUTES];
            int new_attributes_size = 0;
            
            if (t_fore != CurrentForegroundColor)
            {
                new_attributes[new_attributes_size++] = t_fore+30;
                CurrentForegroundColor = t_fore;
            };
            if (t_back != CurrentBackgroundColor)
            {
                new_attributes[new_attributes_size++] = t_back+40;
                CurrentBackgroundColor = t_back;
            };
            if (t_bold != CurrentBold)
            {
                if (t_bold)
                    new_attributes[new_attributes_size++] = 1;
                else
                    new_attributes[new_attributes_size++] = 22;
                    
                CurrentBold = t_bold;
            };
            if (t_inve != CurrentInverse)
            {
                if (t_inve)
                    new_attributes[new_attributes_size++] = 7;
                else
                    new_attributes[new_attributes_size++] = 27;
                    
                CurrentInverse = t_inve;
            };
            
            if (new_attributes_size > 0)
            {
                add_to_ss(ss, "\x1b[", 2, ss_cursor, ss_size);
                
                int i3;
                for (i3 = 0; i3 < new_attributes_size; i3++)
                {
                    char num[20];
                    int len = snprintf(num, 20, "%d", new_attributes[i3]);
                    add_to_ss(ss, num, len, ss_cursor, ss_size);
                    if (i3 < new_attributes_size - 1)
                        char_to_ss(ss, ';', ss_cursor, ss_size);
                    else
                        char_to_ss(ss, 'm', ss_cursor, ss_size);
                };
            };
            
            if (symbol < 32)
                symbol = ' ';
            
            if (symbol < 128 && symbol >= 32)
                char_to_ss(ss, symbol, ss_cursor, ss_size);
            else if (symbol >= 128)
            {
                char s[4];
                int c = getUTF8Str(symbol, s);
                if (c)
                    add_to_ss(ss, s, c, ss_cursor, ss_size);
            }
            else
                char_to_ss(ss, symbol, ss_cursor, ss_size);

            CurrentRealCursorX++;
        };
    };
    
    if (source)
    {
        if (CursorY != source->CursorY || CursorX != source->CursorX || SomethingWritten)
        {
            char line[50];
            size_t len = snprintf(line, 50, "\x1b[%d;%dH", CursorY+1, CursorX+1);
            add_to_ss(ss, line, len, ss_cursor, ss_size);
            SomethingWritten = true;
        }
    }
    else
    {
        char line[50];
        size_t len = snprintf(line, 50, "\x1b[%d;%dH", CursorY+1, CursorX+1);
        add_to_ss(ss, line, len, ss_cursor, ss_size);
        SomethingWritten = true;
    }
  
    if (!SomethingWritten)
    {
        delete[] ss;
        return;
    }

    result.assign(ss, ss_cursor);
    delete[] ss;
}

std::string Terminal::updateCycle() const
{
    return restrictedUpdateCycle(NULL, NULL);
};

std::string Terminal::getLine(unsigned int y) const
{
    if (y >= Height) return std::string("");
    
    char* result = (char*) malloc(Width+1);
    result[Width] = 0;
    
    unsigned int i1;
    for (i1 = 0; i1 < Width; i1++)
        result[i1] = Tiles[i1 + y * Width].getSymbol();
    
    std::string stdresult(result);
    free(result);
    
    return stdresult;
};

bool Terminal::matchRe(Regex &re, unsigned int &x, unsigned int &y)
{
    unsigned int start_x = x, start_y = y;
    
    unsigned int i1, i2;
    for (i1 = start_y; i1 < Height; i1++)
    {
        std::string line = getLine(i1);
        const char* cstr = line.c_str();
        
        for (i2 = start_x; i2 < Width; i2++)
        {
            bool matches = re.execute(&cstr[i2]);
            if (!matches)
                break;
            
            x = re.getMatchStartIndex(0) + i2;
            y = i1;
            return true;
        };
        
        start_x = 0;
    };
    
    return false;
};

void Terminal::setTile(int x, int y, const TerminalTile &t)
{
    Tiles[x + y * Width] = t;
}

void Terminal::fillRectangle(int x, int y, int w, int h, const TerminalTile &t)
{
    int i1, i2;
    if (x < 0)
    {
        w += x;
        x = 0;
    }
    if (y < 0)
    {
        h += y;
        y = 0;
    }
    if (w <= 0 || h <= 0) return;
    
    int x_right = x + w;
    int y_bottom = y + h;
    if (x_right > (int) Width) x_right = Width;
    if (y_bottom > (int) Height) y_bottom = Height;
    
    for (i1 = y; i1 < y_bottom; i1++)
    {
        int loop_w = i1 * Width;
        for (i2 = x; i2 < x_right; i2++)
            Tiles[i2 + loop_w] = t;
    }
}

void Terminal::overlay(const Terminal* source, const TerminalTile &source_delim)
{
    int i1, i2;
    int w = (Width > source->Width) ? source->Width : Width;
    int h = (Height > source->Height) ? source->Height : Height;

    for (i1 = 0; i1 < w; i1++)
        for (i2 = 0; i2 < h; i2++)
            if (source->Tiles[i1 + i2 * source->Width] != source_delim)
                Tiles[i1 + i2 * Width] = source->Tiles[i1 + i2 * source->Width];
}

void Terminal::setAttributes(int n, int x, int y, unsigned int ForegroundColor, unsigned int BackgroundColor, bool Inverse, bool Bold)
{
    if (y >= (int) Height) return;

    while (n > 0)
    {
        n--;
        if (x >= (int) Width) break;

        TerminalTile &t = Tiles[x + y * Width];
        t.setForegroundColor(ForegroundColor);
        t.setBackgroundColor(BackgroundColor);
        t.setInverse(Inverse);
        t.setBold(Bold);
        x++;
    }
}

void Terminal::printString(const char* str, int x, int y, unsigned int ForegroundColor, unsigned int BackgroundColor, bool Inverse, bool Bold)
{
    if (y >= (int) Height) return;

    int cursor = 0, old_cursor = 0;
    unsigned int symbol = 0;
    while ( (symbol = fetchUTF8Unicode(&str[cursor], cursor)))
    {
        if (cursor == old_cursor) break;
        old_cursor = cursor;

        if (x >= (int) Width) break;

        TerminalTile &t = Tiles[x + y * Width];
        t.setSymbol(symbol);
        t.setForegroundColor(ForegroundColor);
        t.setBackgroundColor(BackgroundColor);
        t.setInverse(Inverse);
        t.setBold(Bold);
        x++;
    }
};

void Terminal::setRedBlueSwap(bool enable)
{
    red_blue_swap = enable;
}

bool Terminal::getRedBlueSwap() const
{
    return red_blue_swap;
}
