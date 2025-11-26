#include "Error.h"
#include "Format.h"
#include "MiscUtils.h"

#include <string>

using namespace DFHack::Error;

inline std::string safe_str(const char *s)
{
    return s ? s : "(NULL)";
}

NullPointer::NullPointer(const char *varname, const char *func)
    :All("In " + safe_str(func) + ": NULL pointer: " + safe_str(varname)),
    varname(varname)
{}

InvalidArgument::InvalidArgument(const char *expr, const char *func)
    :All("In " + safe_str(func) + ": Invalid argument; expected: " + safe_str(expr)),
    expr(expr)
{}

VTableMissing::VTableMissing(const char *name)
    :All("Missing vtable address: " + safe_str(name)),
    name(name)
{}

SymbolsXmlParse::SymbolsXmlParse(const char* desc, int id, int row, int col)
    :AllSymbols(fmt::format("error {}: {}, at row {} col {}", id, desc, row, col)),
    desc(safe_str(desc)), id(id), row(row), col(col)
{}

SymbolsXmlBadAttribute::SymbolsXmlBadAttribute(const char *attr)
    :AllSymbols("attribute is either missing or invalid: " + safe_str(attr)),
    attr(safe_str(attr))
{}

SymbolsXmlNoRoot::SymbolsXmlNoRoot()
    :AllSymbols("no root element")
{}

SymbolsXmlUnderspecifiedEntry::SymbolsXmlUnderspecifiedEntry(const char *where)
    :AllSymbols("Underspecified symbol file entry, each entry needs to set both the name attribute and have a value. parent: " + safe_str(where)),
    where(safe_str(where))
{}
