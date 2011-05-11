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

#pragma once

#ifndef DFVECTOR_H_INCLUDED
#define DFVECTOR_H_INCLUDED

#include "DFPragma.h"
#include "DFExport.h"
#include "VersionInfo.h"
#include "DFProcess.h"

#include <string.h>

namespace DFHack
{
    template <class T>
    class DFHACK_EXPORT DfVector
    {
        private:
            Process *_p;
            uint32_t _address;
            t_vecTriplet t;
            uint32_t _size;// vector size
            
            T * data; // cached data

            bool isMetadataInSync()
            {
                t_vecTriplet t2;
                _p->readSTLVector(_address,t2);
                return (t2.start == t.start || t2.end == t.end || t2.alloc_end == t.alloc_end);
            }
        public:
            DfVector(Process *p, uint32_t address) : _p(p), _address(address)
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
            inline const T& operator[] (uint32_t index)
            {
                // FIXME: vector out of bounds exception
                //assert(index < size);
                return data[index];
            };
            // get offset of the specified index
            inline const T& at (uint32_t index)
            {
                //assert(index < size);
                return data[index];
            };
            // update value at index
            bool set(uint32_t index, T value)
            {
                if (index >= _size)
                    return false;
                data[index] = value;
                _p->write(t.start + sizeof(T)*index, sizeof(T), (uint8_t *)&data[index]);
                return true;
            }
            // remove value
            bool remove(uint32_t index)
            {
                if (index >= _size || !isMetadataInSync())
                    return false;
                // Remove the item
                _size--;
                t.end -= sizeof(T);
                int tail = (_size-index)*sizeof(T);
                memmove(&data[index], &data[index+1], tail);
                // Write back the data
                if (tail)
                    _p->write(t.start + sizeof(T)*index, tail, (uint8_t *)&data[index]);
                _p->writeSTLVector(_address,t);
                return true;
            }
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
