/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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

#include "Pragma.h"
#include "Export.h"
#include "VersionInfo.h"
#include "MemAccess.h"

#include <string.h>
#include <vector>

namespace DFHack
{
    template <class T>
    class DFHACK_EXPORT DfVector
    {
        private:
            std::vector<T> * real_vec;
        public:
            DfVector(uint32_t address)
            {
                real_vec = (std::vector<T> *) address;
            };
            ~DfVector()
            {
            };
            // get offset of the specified index
            inline const T& operator[] (uint32_t index)
            {
                // FIXME: vector out of bounds exception
                //assert(index < size);
                return real_vec->at(index);
            };
            // get offset of the specified index
            inline const T& at (uint32_t index)
            {
                //assert(index < size);
                return real_vec->at(index);
            };
            // update value at index
            bool set(uint32_t index, T value)
            {
                if (index >= real_vec->size())
                    return false;
                real_vec->at(index) = value;
                return true;
            }
            // remove value
            bool remove(uint32_t index)
            {
                if (index >= real_vec->size())
                    return false;
                // Remove the item
                real_vec->erase(real_vec->begin() + index);
                return true;
            }
            // get vector size
            inline uint32_t size ()
            {
                return real_vec->size();
            };
            // get vector start
            inline const T * start ()
            {
                return real_vec->data();
            };
    };
}
#endif // DFVECTOR_H_INCLUDED
