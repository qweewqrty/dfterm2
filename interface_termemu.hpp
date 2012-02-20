#ifndef interface_termemu_hpp
#define interface_termemu_hpp

#include "interface_ncurses.hpp"

namespace trankesbel {

class InterfaceTermemu : public InterfaceCurses
{
    public:
        InterfaceTermemu() : InterfaceCurses(true)
        { };

        const Terminal& getTerminal() const { return screen_terminal; };
        Terminal& getTerminal() { return screen_terminal; };
        void setTerminalSize(ui32 w, ui32 h)
        { screen_terminal.resize(w, h); screen_terminal.feedString("\x1b[2J", 4); };
};

};

#endif

