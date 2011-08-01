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
#include "dfhack/Pragma.h"
#include "dfhack/Export.h"
#include <stdint.h>
#include <string.h>
#include <sstream>
//#include <ostream>
namespace DFHack
{
    template <typename T = int>
    class BitArray
    {
    public:
        BitArray()
        {
            bits = NULL;
            size = 0;
        }
        ~BitArray()
        {
            if(bits)
                delete [] bits;
        }
        void clear_all ( void )
        {
            if(bits)
                memset(bits, 0, size);
        }
        void set (T index, bool value = true)
        {
            if(!value)
            {
                clear(index);
                return;
            }
            uint32_t byte = index / 8;
            if(byte < size)
            {
                uint8_t bit = 1 << (index % 8);
                bits[byte] |= bit;
            }
        }
        void clear (T index)
        {
            uint32_t byte = index / 8;
            if(byte < size)
            {
                uint8_t bit = 1 << (index % 8);
                bits[byte] &= ~bit;
            }
        }
        void toggle (T index)
        {
            uint32_t byte = index / 8;
            if(byte < size)
            {
                uint8_t bit = 1 << (index % 8);
                bits[byte] ^= bit;
            }
        }
        bool is_set (T index)
        {
            uint32_t byte = index / 8;
            if(byte < size)
            {
                uint8_t bit = 1 << (index % 8);
                return bit & bits[byte];
            }
            else return false;
        }
        /// WARNING: this can truncate long bit arrays
        operator uint32_t ()
        {
            if(!bits)
                return 0;
            if(size >= 4)
                return  *(uint32_t *)bits;
            uint32_t target = 0;
            memcpy (&target, bits,size);
            return target;
        }
        /// WARNING: this can be truncated / only overwrite part of the data
        bool operator =(uint32_t data)
        {
            if(!bits)
                return false;
            if (size >= 4)
            {
                (*(uint32_t *)bits) = data;
                return true;
            }
            memcpy(bits, &data, size);
            return true;
        }
        friend std::ostream& operator<< (std::ostream &out, BitArray <T> &ba)
        {
            std::stringstream sstr;
            for (int i = 0; i < ba.size * 8; i++)
            {
                if(ba.is_set((T)i))
                    sstr << "1 ";
                else
                    sstr << "0 ";
            }
            out << sstr.str();
            return out;
        }
    //private:
        uint8_t * bits;
        uint32_t size;
    };
}