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
#include "Error.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <iterator>

namespace DFHack
{
    template <typename T = int>
    class BitArray
    {
    private:
        // note that these are mandated by the implementation of flagarrayst in DF code, and must be exactly as below
        using buffer_type = unsigned char;
        using size_type = int32_t;

        buffer_type* _bits;
        size_type _size;

        void resize(size_type newsize, const BitArray<T>* replacement)
        {
            if (newsize == _size)
                return;

            if (newsize == 0)
            {
                delete[] _bits;
                _bits = nullptr;
                _size = 0;
                return;
            }

            buffer_type* old_data = _bits;

            _bits = new buffer_type[newsize];

            buffer_type* copysrc = replacement ? replacement->_bits : old_data;
            size_type copysize = replacement ? replacement->_size : _size;

            if (copysrc)
                std::memcpy(_bits, copysrc, std::min(copysize, newsize));

            if (newsize > _size)
                std::memset(_bits + _size, 0, newsize - _size);

            delete[] old_data;

            _size = newsize;
        }

        void extend(T index)
        {
            size_type newsize = (index / 8) + 1;
            if (newsize > _size)
                resize(newsize);
        }

    public:
        BitArray() : _bits(nullptr), _size(0) {}
        BitArray(const BitArray<T> &other) : _bits(nullptr), _size(0)
        {
            resize(other._size, &other);
        }
        ~BitArray()
        {
            delete [] _bits;
        }

        explicit BitArray(T last) : _bits(nullptr), _size(0) {
            extend(last);
        }
        explicit BitArray(unsigned bytes) : _bits(nullptr), _size(0) {
            resize(bytes);
        }

        size_type size() const { return _size; }
        buffer_type* bits() const { return _bits; }

        void resize( size_type newsize )
        {
            resize(newsize, nullptr);
        }

        void clear_all ( void )
        {
            if(_bits)
                memset(_bits, 0, _size);
        }

        BitArray<T>& operator= (const BitArray<T>& other)
        {
            resize(other._size, &other);
            return *this;
        }

        void set (T index, bool value = true)
        {
            if(!value)
            {
                clear(index);
                return;
            }
            size_type byte = index / 8;
            extend(index);
            uint8_t bit = 1 << (index % 8);
            _bits[byte] |= bit;
        }

        void clear (T index)
        {
            size_type byte = index / 8;
            if(byte < _size)
            {
                uint8_t bit = 1 << (index % 8);
                _bits[byte] &= ~bit;
            }
        }

        void toggle (T index)
        {
            size_type byte = index / 8;
            extend(index);
            uint8_t bit = 1 << (index % 8);
            _bits[byte] ^= bit;
        }

        bool is_set (T index) const
        {
            size_type byte = index / 8;
            if(byte < _size)
            {
                uint8_t bit = 1 << (index % 8);
                return bit & _bits[byte];
            }
            else return false;
        }

        /// WARNING: this can truncate long bit arrays
        template <typename I = uint32_t>
        I as_int () const
        {
            if(!_bits)
                return 0;
            if (_size >= sizeof(I))
                // FIXME (C++23): should be std::start_lifetime_as
                return *reinterpret_cast<I*>(_bits);
            I target = 0;
            std::memcpy(&target, _bits, _size);
            return target;
        }

        /// WARNING: this can be truncated / only overwrite part of the data
        template <typename I = uint32_t>
        bool operator =(I data)
        {
            if(!_bits)
                return false;
            if (_size >= sizeof(I))
            {
                *reinterpret_cast<I*>(_bits) = data;
                return true;
            }
            std::memcpy(_bits, &data, _size);
            return true;
        }

        friend std::ostream& operator<< (std::ostream &out, BitArray <T> &ba)
        {
            std::stringstream sstr;
            for (int i = 0; i < ba._size * 8; i++)
            {
                if(ba.is_set((T)i))
                    sstr << "1 ";
                else
                    sstr << "0 ";
            }
            out << sstr.str();
            return out;
        }
    };

    template <typename T = int>
    class DfArray
    {
    private:
        T *m_data;
        unsigned short m_size;

        void resize(unsigned short new_size, const DfArray<T>* replacement)
        {
            if (new_size == m_size)
                return;

            T* old_data = m_data;

            m_data = (T*) new T[new_size];

            T* copysrc = replacement ? replacement->m_data : old_data;
            unsigned short copysize = replacement ? replacement->m_size : m_size;

            if (copysrc)
                std::memcpy(m_data, copysrc, sizeof(T) * std::min(copysize, new_size));

            if (new_size > m_size)
                std::memset(m_data + m_size, 0, sizeof(T) * (new_size - m_size));

            delete[] old_data;

            m_size = new_size;
        }
    public:
        DfArray() : m_data(nullptr), m_size(0) {}
        ~DfArray() { delete[] m_data; }

        DfArray(const DfArray<T> &other) : m_data(nullptr), m_size(0)
        {
            resize(other.m_size, &other);
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
            resize(new_size, nullptr);
        }

        DfArray &operator= (const DfArray<T> &other)
        {
            resize(other.size(), &other);
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

    template <typename L, typename I>
    struct DfLinkedList
    {
        class const_iterator;

        class iterator
        {
            L *root;
            L *cur;
            friend struct DfLinkedList<L, I>;
            friend class const_iterator;
            iterator(L *root, L *cur) : root(root), cur(cur) {}
        public:
            using difference_type = std::ptrdiff_t;
            using value_type = I *;
            using pointer = I **;
            using reference = I *&;
            using iterator_category = std::bidirectional_iterator_tag;

            iterator() : root(nullptr), cur(nullptr) {}
            iterator(const iterator & other) : root(other.root), cur(other.cur) {}

            iterator & operator++()
            {
                CHECK_NULL_POINTER(root);
                CHECK_NULL_POINTER(cur);

                cur = cur->next;
                return *this;
            }
            iterator & operator--()
            {
                CHECK_NULL_POINTER(root);

                if (!cur)
                {
                    // find end() - 1
                    for (cur = root->next; cur && cur->next; cur = cur->next)
                    {
                    }
                    return *this;
                }

                CHECK_NULL_POINTER(cur);
                CHECK_NULL_POINTER(cur->prev);

                cur = cur->prev;
                return *this;
            }
            iterator operator++(int)
            {
                iterator copy(*this);
                ++*this;
                return copy;
            }
            iterator operator--(int)
            {
                iterator copy(*this);
                --*this;
                return copy;
            }
            iterator & operator=(const iterator & other)
            {
                root = other.root;
                cur = other.cur;
                return *this;
            }

            I *& operator*()
            {
                CHECK_NULL_POINTER(root);
                CHECK_NULL_POINTER(cur);

                return cur->item;
            }

            I *& operator*() const
            {
                CHECK_NULL_POINTER(root);
                CHECK_NULL_POINTER(cur);

                return cur->item;
            }

            I * operator->()
            {
                CHECK_NULL_POINTER(root);
                CHECK_NULL_POINTER(cur);

                return cur->item;
            }

            I * operator->() const
            {
                CHECK_NULL_POINTER(root);
                CHECK_NULL_POINTER(cur);

                return cur->item;
            }

            operator const_iterator() const
            {
                return const_iterator(*this);
            }
            bool operator==(const iterator & other) const
            {
                return root == other.root && cur == other.cur;
            }
            bool operator!=(const iterator & other) const
            {
                return !(*this == other);
            }
        };
        class const_iterator
        {
            iterator iter;
            friend struct DfLinkedList<L, I>;
        public:
            using difference_type = std::ptrdiff_t;
            using value_type = I *;
            using pointer = I *const *;
            using reference = I *const &;
            using iterator_category = std::bidirectional_iterator_tag;

            const_iterator() : iter(nullptr) {}
            const_iterator(const iterator & iter) : iter(iter) {}
            const_iterator(const const_iterator & other) : iter(other.iter) {}

            const_iterator & operator++()
            {
                ++iter;
                return *this;
            }
            const_iterator & operator--()
            {
                --iter;
                return *this;
            }
            const_iterator operator++(int)
            {
                const_iterator copy(*this);
                ++iter;
                return copy;
            }
            const_iterator operator--(int)
            {
                const_iterator copy(*this);
                --iter;
                return copy;
            }
            const_iterator & operator=(const const_iterator & other)
            {
                iter = other.iter;
                return *this;
            }
            I *const & operator*() const
            {
                return *iter;
            }
            bool operator==(const const_iterator & other) const
            {
                return iter == other.iter;
            }
            bool operator!=(const const_iterator & other) const
            {
                return iter != other.iter;
            }
        };

        using value_type = I *;
        using reference_type = I *&;
        using difference_type = void;
        using size_type = size_t;

        bool empty() const
        {
            return static_cast<const L *>(this)->next == nullptr;
        }
        size_t size() const
        {
            size_t n = 0;
            for (auto It = this->cbegin(); It != this->cend(); ++It)
            {
               n++;
            }
            return n;
        }

        iterator begin()
        {
            return iterator(static_cast<L *>(this), static_cast<L *>(this)->next);
        }
        const_iterator begin() const
        {
            return const_iterator(const_cast<DfLinkedList<L, I> *>(this)->begin());
        }
        const_iterator cbegin() const
        {
            return begin();
        }
        iterator end()
        {
            return iterator(static_cast<L *>(this), nullptr);
        }
        const_iterator end() const
        {
            return const_iterator(const_cast<DfLinkedList<L, I> *>(this)->end());
        }
        const_iterator cend() const
        {
            return end();
        }

        /**
         * This erases the cell at the location of the iterator, but not the item pointed to by that cell.
         * When using this with lists that own the items being pointed to (e.g. world->jobs.list), it is the
         * responsibility of the caller to avoid memory leaks.
         */
        iterator erase(const_iterator pos)
        {
            auto root = static_cast<L *>(this);
            CHECK_INVALID_ARGUMENT(pos.iter.root == root);
            CHECK_NULL_POINTER(pos.iter.cur);

            auto link = pos.iter.cur;
            auto next = link->next;

            if (link->prev)
            {
                link->prev->next = link->next;
            }
            else
            {
                root->next = link->next;
            }

            if (link->next)
            {
                link->next->prev = link->prev;
            }

            if (link->item)
                link->item->dfhack_set_list_link(nullptr);

            delete link;

            return iterator(root, next);
        }

        iterator insert(const_iterator pos, I *const & item)
        {
            auto root = static_cast<L *>(this);
            CHECK_INVALID_ARGUMENT(pos.iter.root == root);

            auto link = pos.iter.cur;
            if (!link || !link->prev)
            {
                if (!link && root->next)
                {
                    pos--;
                    return insert_after(pos, item);
                }

                CHECK_INVALID_ARGUMENT(root->next == link);
                push_front(item);
                return begin();
            }

            auto newlink = new L();
            newlink->prev = link->prev;
            newlink->next = link;
            link->prev = newlink;
            if (newlink->prev)
            {
                newlink->prev->next = newlink;
            }
            else if (link == root->next)
            {
                root->next = newlink;
            }
            newlink->item = item;
            item->dfhack_set_list_link(newlink);

            return iterator(root, newlink);
        }

        iterator insert_after(const_iterator pos, I *const & item)
        {
            auto root = static_cast<L *>(this);
            CHECK_INVALID_ARGUMENT(pos.iter.root == root);
            CHECK_INVALID_ARGUMENT(pos.iter.cur);

            auto link = pos.iter.cur;
            auto next = link->next;
            auto newlink = new L();
            newlink->prev = link;
            newlink->next = next;
            link->next = newlink;
            if (next)
            {
                next->prev = newlink;
            }
            newlink->item = item;
            item->dfhack_set_list_link(newlink);
            return iterator(root, newlink);
        }

        void push_front(I *const & item)
        {
            auto root = static_cast<L *>(this);
            auto link = new L();
            link->prev = root;
            if (root->next)
            {
                root->next->prev = link;
                link->next = root->next;
            }
            link->item = item;
            item->dfhack_set_list_link(link);
            root->next = link;
        }

        static_assert(std::bidirectional_iterator<iterator>);
        static_assert(std::bidirectional_iterator<const_iterator>);
    };

    template<typename T, typename O, typename I>
    struct DfOtherVectors
    {
        std::vector<I *> & operator[](O other_id)
        {
            CHECK_INVALID_ARGUMENT(size_t(other_id) < sizeof(T) / sizeof(std::vector<I *>));

            auto vectors = reinterpret_cast<std::vector<I *> *>(this);
            return vectors[other_id];
        }
    };
}
