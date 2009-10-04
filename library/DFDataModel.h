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

#ifndef DATAMODEL_H_INCLUDED
#define DATAMODEL_H_INCLUDED

class DfVector;

// let's go pure virtual.
class DataModel
{
    public:
    // read a string
    virtual const string readSTLString (uint32_t offset) = 0;
    // read a vector from memory
    virtual DfVector readVector (uint32_t offset, uint32_t item_size) = 0;
};

class DMWindows40d : public DataModel
{
    virtual const string readSTLString (uint32_t offset);
    // read a vector from memory
    virtual DfVector readVector (uint32_t offset, uint32_t item_size);
};

class DMLinux40d : public DataModel
{
    virtual const string readSTLString (uint32_t offset);
    // read a vector from memory
    virtual DfVector readVector (uint32_t offset, uint32_t item_size);
};

#endif // DATAMODEL_H_INCLUDED
