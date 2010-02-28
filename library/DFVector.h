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

#ifndef DFVECTOR_H_INCLUDED
#define DFVECTOR_H_INCLUDED

#include "Tranquility.h"
#include "Export.h"

namespace DFHack
{
    class DFHACK_EXPORT DfVector
    {
        private:
            // starting offset
            uint32_t start;
            // vector size
            uint32_t size;
            uint32_t item_size;
            uint8_t* data;
            
        public:
            DfVector();
            DfVector(uint32_t _start, uint32_t _size, uint32_t _item_size);
            ~DfVector();
            // get offset of the specified index
            inline void* operator[] (uint32_t index)
            {
                // FIXME: vector out of bounds exception
                //assert(index < size);
                return data + index*item_size;
            };
            // get offset of the specified index
            inline void* at (uint32_t index)
            {
                //assert(index < size);
                return data + index*item_size;
            };
            // get vector size
            inline uint32_t  getSize () 
            {
                return size;
            };
    };
}
#endif // DFVECTOR_H_INCLUDED
