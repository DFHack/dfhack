
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

#ifndef MODULE_H_INCLUDED
#define MODULE_H_INCLUDED

#include "DFExport.h"
namespace DFHack
{
    class Context;
    class DFHACK_EXPORT Module
    {
        public:
        virtual ~Module(){};
        virtual bool Start(){return true;};// default start...
        virtual bool Finish() = 0;// everything should have a Finish()
        // should Context call Finish when Resume is called?
        virtual bool OnResume()
        {
            Finish();
            return true;
        };
        // Finish when map change is detected?
        // TODO: implement
        virtual bool OnMapChange()
        {
            return false;
        };
    };
}
#endif //MODULE_H_INCLUDED

