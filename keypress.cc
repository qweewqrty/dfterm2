#include "interface.hpp"

using namespace trankesbel;

KeyPress::KeyPress(const KeyCode &kc, bool special_key)
{
    alt_down = ctrl_down = shift_down = false;
    setKeyCode(kc, special_key);
}

KeyPress::KeyPress(const KeyCode &kc, bool special_key, bool alt_down, bool ctrl_down, bool shift_down)
{
    setKeyCode(kc, special_key);
    setAltDown(alt_down);
    setCtrlDown(ctrl_down);
    setShiftDown(shift_down);
}

void KeyPress::setKeyCode(const KeyCode &kc, bool special_key)
{
    this->special_key = special_key;

    keycode = kc;
    if (!special_key)
        return;

    alt_down = ctrl_down = shift_down = false;
    if (kc >= Alt0 && kc <= AltLeft)
    {
        alt_down = true;
        return;
    }

    if (kc == CtrlUp || kc == CtrlDown || kc == CtrlLeft || kc == CtrlRight)
    {
        ctrl_down = true;
        return;
    }

    if (kc >= SBeg && kc <= SUndo)
    {
        shift_down = true;
        return;
    }
}
