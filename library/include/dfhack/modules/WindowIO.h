/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#ifndef KEYS_H_INCLUDED
#define KEYS_H_INCLUDED

#include "dfhack/DFPragma.h"
#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"

namespace DFHack
{

class Process;
    
enum t_special
{
    ENTER,
    SPACE,
    BACK_SPACE,
    TAB,
    CAPS_LOCK,
    LEFT_SHIFT,
    RIGHT_SHIFT,
    LEFT_CONTROL,
    RIGHT_CONTROL,
    ALT,
    WAIT,
    ESCAPE,
    UP,
    DOWN,
    LEFT,
    RIGHT,
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
    F12,
    PAGE_UP,
    PAGE_DOWN,
    INSERT,
    DFK_DELETE, // stupid windows fails here
    HOME,
    END,
    KEYPAD_DIVIDE,
    KEYPAD_MULTIPLY,
    KEYPAD_SUBTRACT,
    KEYPAD_ADD,
    KEYPAD_ENTER,
    KEYPAD_0,
    KEYPAD_1,
    KEYPAD_2,
    KEYPAD_3,
    KEYPAD_4,
    KEYPAD_5,
    KEYPAD_6,
    KEYPAD_7,
    KEYPAD_8,
    KEYPAD_9,
    KEYPAD_DECIMAL_POINT,
    NUM_SPECIALS
};
class DFContextShared;
class DFHACK_EXPORT WindowIO : public Module
{
    class Private;
    private:
        Private * d;
    public:
        bool Finish(){return true;};
        WindowIO(DFHack::DFContextShared * d);
        ~WindowIO();
        void TypeStr (const char *input, int delay = 0, bool useShift = false);
        void TypeSpecial (t_special command, int count = 1, int delay = 0);
};

}
#endif // KEYS_H_INCLUDED
