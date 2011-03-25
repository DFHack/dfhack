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

#include "DFPragma.h"
#include "DFExport.h"
#include "VersionInfo.h"
#include "DFProcess.h"
namespace DFHack
{
    template <class T>
    class DFHACK_EXPORT DfVector
    {
        private:
            t_vecTriplet t;
            uint32_t _size;// vector size
            
            T * data; // cached data
        public:
            DfVector(Process * p, uint32_t address)
            {
                p->readSTLVector(address,t);
                uint32_t byte_size = t.end - t.start;
                _size = byte_size / sizeof(T);
                data = new T[_size];
                p->read(t.start,byte_size, (uint8_t *)data);
            };
            DfVector()
            {
                data = 0;
            };
            ~DfVector()
            {
                if(data)
                    delete [] data;
            };
            // get offset of the specified index
            inline T& operator[] (uint32_t index)
            {
                // FIXME: vector out of bounds exception
                //assert(index < size);
                return data[index];
            };
            // get offset of the specified index
            inline T& at (uint32_t index)
            {
                //assert(index < size);
                return data[index];
            };
            // get vector size
            inline uint32_t size ()
            {
                return _size;
            };
            // get vector start
            inline uint32_t start ()
            {
                return t.start;
            };
            // get vector end
            inline uint32_t end ()
            {
                return t.end;
            };
            // get vector start
            inline const uint32_t alloc_end ()
            {
                return t.alloc_end;
            };
    };
}
#endif // DFVECTOR_H_INCLUDED
