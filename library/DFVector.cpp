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

#include "Tranquility.h"
#include "DFCommonInternal.h"

using namespace DFHack;

DfVector::DfVector(uint32_t _start, uint32_t _size, uint32_t _item_size): start(_start),size(_size),item_size(_item_size)
{
    data = (uint8_t *) new char[size * item_size];
    g_pProcess->read(start,size*item_size, (uint8_t *)data);
};
DfVector::DfVector()
{
    data = 0;
};
            
DfVector::~DfVector()
{
    if(data)
        delete [] data;
};
