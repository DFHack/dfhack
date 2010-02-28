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

#ifndef ERROR_H_INCLUDED
#define ERROR_H_INCLUDED

#include <string>
#include <sstream>
#include <exception>

namespace DFHack
{
    namespace Error
    {
        class NoProcess : public exception
        {
        public:
            virtual const char* what() const throw()
            {
                return "couldn't find a suitable process";
            }
        };

        class CantAttach : public exception
        {
        public:
            virtual const char* what() const throw()
            {
                return "couldn't attach to process";
            }
        };

        class NoMapLoaded : public exception
        {
        public:
            virtual const char* what() const throw()
            {
                return "no map has been loaded in the dwarf fortress process";
            }
        };

        class BadMapDimensions : public exception
        {
        public:
            BadMapDimensions(uint32_t& _x, uint32_t& _y) : x(_x), y(_y) {}
            const uint32_t x;
            const uint32_t y;

            virtual const char* what() const throw()
            {
                return "both x and y needs to be between 0 and 48";
            }
        };

        // a call to DFHack::mem_info::get* failed
        class MissingMemoryDefinition : public exception
        {
        public:
            MissingMemoryDefinition(const char* _type, const char* _key) : type(_type), key(_key) {}
            // Used by functios using integer keys, such as getTrait
            MissingMemoryDefinition(const char* _type, uint32_t _key) : type(_type)
            {
                // FIXME fancy hex printer goes here
                std::stringstream s;
                s << _key;
                key = s.str();
            }

            // (perhaps it should be an enum, but this is intended for easy printing/logging)
            // type can be any of the following:
            //
            // address
            // offset
            // hexvalue
            // string
            // profession
            // job
            // skill
            // trait
            // traitname
            // labor
            const std::string type;
            std::string key;

            virtual const char* what() const throw()
            {
                return "memory definition missing";
            }
        };
    }
}

#endif // ERROR_H_INCLUDED
