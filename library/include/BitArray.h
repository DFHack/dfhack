/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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
#include "Pragma.h"
#include "Export.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <exception>
//#include <ostream>
namespace DFHack
{
    template <typename T = int>
    class BitArray
    {
    public:
        BitArray() : bits(NULL), size(0) {}
        BitArray(const BitArray<T> &other) : bits(NULL), size(0)
        {
            *this = other;
        }
        ~BitArray()
        {
            free(bits);
        }

        explicit BitArray(T last) : bits(NULL), size(0) {
            extend(last);
        }
        explicit BitArray(unsigned bytes) : bits(NULL), size(0) {
            resize(bytes);
        }

        void clear_all ( void )
        {
            if(bits)
                memset(bits, 0, size);
        }
        void resize (unsigned newsize)
        {
            if (newsize == size)
                return;
            uint8_t* mem = (uint8_t *) realloc(bits, newsize);
            if(!mem && newsize != 0)
                throw std::bad_alloc();
            bits = mem;
            if (newsize > size)
                memset(bits+size, 0, newsize-size);
            size = newsize;
        }
        BitArray<T> &operator= (const BitArray<T> &other)
        {
            resize(other.size);
            memcpy(bits, other.bits, size);
            return *this;
        }
        void extend (T index)
        {
            unsigned newsize = (index / 8) + 1;
            if (newsize > size)
                resize(newsize);
        }
        void set (T index, bool value = true)
        {
            if(!value)
            {
                clear(index);
                return;
            }
            uint32_t byte = index / 8;
            extend(index);
            //if(byte < size)
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
            extend(index);
            //if(byte < size)
            {
                uint8_t bit = 1 << (index % 8);
                bits[byte] ^= bit;
            }
        }
        bool is_set (T index) const
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
        uint32_t as_int ()
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
        uint8_t * bits;
        uint32_t size;
    };

    template <typename T = int>
    class DfArray
    {
        T *m_data;
        unsigned short m_size;
    public:
        DfArray() : m_data(NULL), m_size(0) {}
        ~DfArray() { free(m_data); }

        DfArray(const DfArray<T> &other) : m_data(NULL), m_size(0)
        {
            resize(other.m_size);
            memcpy(m_data, other.m_data,m_size*sizeof(T));
        }

        typedef T value_type;

        T *data() { return m_data; }
        const T *data() const { return m_data; }
        unsigned size() const { return m_size; }

        T *begin() { return m_data; }
        T *end() { return m_data+m_size; }

        T& operator[] (unsigned i) { return m_data[i]; }
        const T& operator[] (unsigned i) const { return m_data[i]; }

        void resize(unsigned new_size)
        {
            if (new_size == m_size)
                return;
            if(!m_data)
            {
                m_data = (T*) malloc(sizeof(T)*new_size);
            }
            else
            {
                T* mem = (T*) realloc(m_data, sizeof(T)*new_size);
                if(!mem && new_size != 0)
                    throw std::bad_alloc();
                m_data = mem;
            }
            if (new_size > m_size)
                memset(m_data+sizeof(T)*m_size, 0, sizeof(T)*(new_size - m_size));
            m_size = new_size;
        }

        DfArray &operator= (const DfArray<T> &other)
        {
            resize(other.size());
            memcpy(data(), other.data(), sizeof(T)*size());
            return *this;
        }

        void erase(T *ptr) {
            memmove(ptr, ptr+1, sizeof(T)*(m_size - (ptr - m_data))); m_size--;
        }
        void insert(T *ptr, const T &item) {
            int idx = ptr - m_data;
            resize(m_size+1);
            memmove(m_data + idx + 1, m_data + idx, sizeof(T)*(m_size - idx - 1));
            m_data[idx] = item;
        }
    };
}
