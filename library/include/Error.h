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

#include <exception>
#include <string>

#include "Export.h"
#include "MiscUtils.h"

namespace DFHack
{
    namespace Error
    {
        /*
         * our wrapper for the C++ exception. used to differentiate
         * the whole array of DFHack exceptions from the rest
         */
#ifdef _MSC_VER
#pragma push
/**
 * C4275 - The warning officially is non dll-interface class 'std::exception' used as base for
 * dll-interface class
 *
 * Basically, it's saying that you might have an ABI problem if you mismatch compilers. We don't
 * care since we build all of DFhack at once against whatever Toady is using
 */
#pragma warning(disable: 4275)
#endif
    class DFHACK_EXPORT All : public std::exception
    {
    public:
        const std::string full;
        All(const std::string &full)
            :full(full)
        {}
        virtual const char *what() const noexcept
        {
            return full.c_str();
        }
        virtual ~All() noexcept {}
    };
#ifdef _MSC_VER
#pragma pop
#endif
        class DFHACK_EXPORT NullPointer : public All {
        public:
            const char *const varname;
            NullPointer(const char *varname = NULL, const char *func = NULL);
        };

#define CHECK_NULL_POINTER(var) \
    { if (var == NULL) throw DFHack::Error::NullPointer(#var, DFHACK_FUNCTION_SIG); }

        class DFHACK_EXPORT InvalidArgument : public All {
        public:
            const char *const expr;
            InvalidArgument(const char *expr = NULL, const char *func = NULL);
        };

#define CHECK_INVALID_ARGUMENT(expr) \
    { if (!(expr)) throw DFHack::Error::InvalidArgument(#expr, DFHACK_FUNCTION_SIG); }

        class DFHACK_EXPORT VTableMissing : public All {
        public:
            const char *const name;
            VTableMissing(const char *name = NULL);
        };


        class DFHACK_EXPORT AllSymbols : public All
        {
        public:
            AllSymbols(const std::string &full)
                :All(full)
            {}
        };
        // Syntax errors and whatnot, the xml can't be read
        class DFHACK_EXPORT SymbolsXmlParse : public AllSymbols
        {
        public:
            SymbolsXmlParse(const char* desc, int id, int row, int col);
            const std::string desc;
            const int id;
            const int row;
            const int col;
        };

        class DFHACK_EXPORT SymbolsXmlBadAttribute : public AllSymbols
        {
        public:
            SymbolsXmlBadAttribute(const char* attr);
            std::string attr;
        };

        class DFHACK_EXPORT SymbolsXmlNoRoot : public AllSymbols
        {
        public:
            SymbolsXmlNoRoot();
        };

        class DFHACK_EXPORT SymbolsXmlUnderspecifiedEntry : public AllSymbols
        {
        public:
            SymbolsXmlUnderspecifiedEntry(const char *where);
            std::string where;
        };
    }
}
