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


DfVector::DfVector(Process * p, uint32_t address, uint32_t _item_size)
{
    uint32_t triplet[3];
    item_size = _item_size;
    memory_info * mem = p->getDescriptor();
    uint32_t offs =  mem->getOffset("hacked_vector_triplet");
    
    p->read(address + offs, sizeof(triplet), (uint8_t *) &triplet);
    start = triplet[0];
    uint32_t byte_size = triplet[1] - triplet[0];
    size = byte_size / item_size;
    data = (uint8_t *) new char[byte_size];
    g_pProcess->read(start,byte_size, (uint8_t *)data);
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
