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

#include "DFExport.h"
#include <string>
#include <sstream>
#include <exception>

namespace DFHack
{
    namespace Error
    {
        /*
         * our wrapper for the C++ exception. used to differentiate
         * the whole array of DFHack exceptions from the rest
         */
        class DFHACK_EXPORT All : public std::exception{};
        class DFHACK_EXPORT NoProcess : public All
        {
        public:
            virtual const char* what() const throw()
            {
                return "couldn't find a suitable process";
            }
        };

        class DFHACK_EXPORT CantAttach : public All
        {
        public:
            virtual const char* what() const throw()
            {
                return "couldn't attach to process";
            }
        };

        class DFHACK_EXPORT NoMapLoaded : public All
        {
        public:
            virtual const char* what() const throw()
            {
                return "no map has been loaded in the dwarf fortress process";
            }
        };

        class DFHACK_EXPORT BadMapDimensions : public All
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
        class DFHACK_EXPORT MissingMemoryDefinition : public All
        {
        public:
            MissingMemoryDefinition(const char* _type, const std::string _key) : type(_type), key(_key)
            {
                std::stringstream s;
                s << "memory object not declared: type=" << type << " key=" << key;
                full = s.str();
            }
            // Used by functios using integer keys, such as getTrait
            MissingMemoryDefinition(const char* _type, uint32_t _key) : type(_type)
            {
                std::stringstream s1;
                s1 << _key;
                key = s1.str();
                
                std::stringstream s;
                s << "memory object not declared: type=" << type << " key=" << key;
                full = s.str();
            }
            virtual ~MissingMemoryDefinition() throw(){};

            std::string full;
            const std::string type;
            std::string key;

            virtual const char* what() const throw()
            {
                return full.c_str();
            }
        };

        // a call to DFHack::mem_info::get* failed
        class DFHACK_EXPORT UnsetMemoryDefinition : public All
        {
        public:
            UnsetMemoryDefinition(const char* _type, const std::string _key) : type(_type), key(_key)
            {
                std::stringstream s;
                s << "memory object not set: type " << type << " key " << key;
                full = s.str();
            }
            // Used by functios using integer keys, such as getTrait
            UnsetMemoryDefinition(const char* _type, uint32_t _key) : type(_type)
            {
                std::stringstream s1;
                s1 << _key;
                key = s1.str();

                std::stringstream s;
                s << "memory object not set: type " << type << " key " << key;
                full = s.str();
            }
            virtual ~UnsetMemoryDefinition() throw(){};

            std::string full;
            const std::string type;
            std::string key;

            virtual const char* what() const throw()
            {
                return full.c_str();
            }
        };

        // Syntax errors and whatnot, the xml cant be read
        class DFHACK_EXPORT MemoryXmlParse : public All
        {
        public:
            MemoryXmlParse(const char* _desc, int _id, int _row, int _col)
            :desc(_desc), id(_id), row(_row), col(_col)
            {
                std::stringstream s;
                s << "error " << id << ": " << desc << ", at row " << row << " col " << col;
                full = s.str();
            }
                
                
            std::string full;
            const std::string desc;
            const int id;
            const int row;
            const int col;
            
            virtual ~MemoryXmlParse() throw(){};

            virtual const char* what() const throw()
            {
                return full.c_str();
            }
        };

        class DFHACK_EXPORT MemoryXmlBadAttribute : public All
        {
        public:
            MemoryXmlBadAttribute(const char* _attr) : attr(_attr)
            {
                std::stringstream s;
                s << "attribute is either missing or invalid: " << attr;
                full = s.str();
            }
            std::string full;
            std::string attr;
            
            virtual ~MemoryXmlBadAttribute() throw(){};

            virtual const char* what() const throw()
            {
                return full.c_str();
            }
        };

        class DFHACK_EXPORT MemoryXmlNoRoot : public All
        {
        public:
            MemoryXmlNoRoot() {}
            
            virtual ~MemoryXmlNoRoot() throw(){};

            virtual const char* what() const throw()
            {
                return "no pElem found";
            }
        };

        class DFHACK_EXPORT MemoryXmlNoDFExtractor : public All
        {
        public:
            MemoryXmlNoDFExtractor(const char* _name) : name(_name)
            {
                std::stringstream s;
                s << "DFExtractor != " << name;
                full = s.str();
            }
            virtual ~MemoryXmlNoDFExtractor() throw(){};

            std::string name;
            std::string full;

            virtual const char* what() const throw()
            {
                return full.c_str();
            }
        };

        class DFHACK_EXPORT MemoryXmlUnderspecifiedEntry : public All
        {
        public:
            MemoryXmlUnderspecifiedEntry(const char * _where) : where(_where)
            {
                std::stringstream s;
                s << "underspecified MemInfo entry, each entry needs to set both the name attribute and have a value. parent: " << where;
                full = s.str();
            }
            virtual ~MemoryXmlUnderspecifiedEntry() throw(){};
            std::string where;
            std::string full;
            virtual const char* what() const throw()
            {
                return full.c_str();
            }
        };

        class DFHACK_EXPORT MemoryXmlUnknownType : public All
        {
        public:
            MemoryXmlUnknownType(const char* _type) : type(_type)
            {
                std::stringstream s;
                s << "unknown MemInfo type: " << type;
                full = s.str();
            }
            virtual ~MemoryXmlUnknownType() throw(){};

            std::string type;
            std::string full;

            virtual const char* what() const throw()
            {
                return full.c_str();
            }
        };
        
        class DFHACK_EXPORT SHMServerDisappeared : public All
        {
        public:
            SHMServerDisappeared(){}
            virtual ~SHMServerDisappeared() throw(){};
            virtual const char* what() const throw()
            {
                return "The server process has disappeared";
            }
        };
        class DFHACK_EXPORT SHMLockingError : public All
        {
        public:
            SHMLockingError(const char* _type) : type(_type)
            {
                std::stringstream s;
                s << "SHM locking error: " << type;
                full = s.str();
            }
            virtual ~SHMLockingError() throw(){};
            
            std::string type;
            std::string full;
            
            virtual const char* what() const throw()
            {
                return full.c_str();
            }
        };
        class DFHACK_EXPORT MemoryAccessDenied : public All
        {
        public:
            MemoryAccessDenied() {}
            virtual ~MemoryAccessDenied() throw(){};
            virtual const char* what() const throw()
            {
                return "SHM ACCESS DENIED";
            }
        };
        class DFHACK_EXPORT SHMVersionMismatch : public All
        {
        public:
            SHMVersionMismatch() {}
            virtual ~SHMVersionMismatch() throw(){};
            virtual const char* what() const throw()
            {
                return "SHM VERSION MISMATCH";
            }
        };
        class DFHACK_EXPORT SHMAttachFailure : public All
        {
        public:
            SHMAttachFailure() {}
            virtual ~SHMAttachFailure() throw(){};
            virtual const char* what() const throw()
            {
                return "SHM ATTACH FAILURE";
            }
        };
        class DFHACK_EXPORT ModuleNotInitialized : public All
        {
        public:
            ModuleNotInitialized() {}
            virtual ~ModuleNotInitialized() throw(){};
            virtual const char* what() const throw()
            {
                return "Programmer error: module not initialized!";
            }
        };
        
    }
}

#endif // ERROR_H_INCLUDED
