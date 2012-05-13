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

#include "Export.h"
#include "Pragma.h"
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

        class DFHACK_EXPORT NullPointer : public All {
            const char *varname_;
        public:
            NullPointer(const char *varname_ = NULL) : varname_(varname_) {}
            const char *varname() const { return varname_; }
            virtual const char *what() const throw();
        };

#define CHECK_NULL_POINTER(var) \
    { if (var == NULL) throw DFHack::Error::NullPointer(#var); }

        class DFHACK_EXPORT InvalidArgument : public All {
            const char *expr_;
        public:
            InvalidArgument(const char *expr_ = NULL) : expr_(expr_) {}
            const char *expr() const { return expr_; }
            virtual const char *what() const throw();
        };

#define CHECK_INVALID_ARGUMENT(expr) \
    { if (!(expr)) throw DFHack::Error::InvalidArgument(#expr); }


        class DFHACK_EXPORT AllSymbols : public All{};
        // Syntax errors and whatnot, the xml can't be read
        class DFHACK_EXPORT SymbolsXmlParse : public AllSymbols
        {
        public:
            SymbolsXmlParse(const char* _desc, int _id, int _row, int _col)
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
            virtual ~SymbolsXmlParse() throw(){};
            virtual const char* what() const throw()
            {
                return full.c_str();
            }
        };

        class DFHACK_EXPORT SymbolsXmlBadAttribute : public All
        {
        public:
            SymbolsXmlBadAttribute(const char* _attr) : attr(_attr)
            {
                std::stringstream s;
                s << "attribute is either missing or invalid: " << attr;
                full = s.str();
            }
            std::string full;
            std::string attr;
            virtual ~SymbolsXmlBadAttribute() throw(){};
            virtual const char* what() const throw()
            {
                return full.c_str();
            }
        };

        class DFHACK_EXPORT SymbolsXmlNoRoot : public All
        {
        public:
            SymbolsXmlNoRoot() {}
            virtual ~SymbolsXmlNoRoot() throw(){};
            virtual const char* what() const throw()
            {
                return "Symbol file is missing root element.";
            }
        };

        class DFHACK_EXPORT SymbolsXmlUnderspecifiedEntry : public All
        {
        public:
            SymbolsXmlUnderspecifiedEntry(const char * _where) : where(_where)
            {
                std::stringstream s;
                s << "Underspecified symbol file entry, each entry needs to set both the name attribute and have a value. parent: " << where;
                full = s.str();
            }
            virtual ~SymbolsXmlUnderspecifiedEntry() throw(){};
            std::string where;
            std::string full;
            virtual const char* what() const throw()
            {
                return full.c_str();
            }
        };
    }
}

