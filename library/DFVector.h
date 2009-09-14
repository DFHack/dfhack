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

class DfVector
{
private:
    // starting offset
    uint32_t start;
    // vector size
    uint32_t size;
    // vector item size
    uint32_t item_size;
    
public:
    DfVector(uint32_t _start, uint32_t _size, uint32_t _item_size):
        start(_start),size(_size),item_size(_item_size) {};
    DfVector(const DfVector & vec)
    {
        start = vec.start;
        size = vec.size;
        item_size = vec.item_size;
    };
    DfVector(){};
    // get offset of the specified index
    inline uint32_t operator[](uint32_t index)
    {
        assert(index < size);
        return start + index*item_size;
    };
    // get vector size
    inline uint32_t getSize()
    {
        return size;
    };
    // read item_size bytes from the right offset
    inline void read (uint32_t index, uint8_t *target)
    {
        assert(index < size);
        Mread (start + index*item_size, item_size, target);
    };
};

#endif // DFVECTOR_H_INCLUDED
