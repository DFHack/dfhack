#include "Error.h"
#include "MiscUtils.h"

using namespace DFHack::Error;

inline const char *safe_str(const char *s)
{
    return s ? s : "(NULL)";
}

NullPointer::NullPointer(const char *varname)
    :All(stl_concat("NULL pointer: ", safe_str(varname))),
    varname(varname)
{}

InvalidArgument::InvalidArgument(const char *expr)
    :All(stl_concat("Invalid argument; expected: ", safe_str(expr))),
    expr(expr)
{}

VTableMissing::VTableMissing(const char *name)
    :All(stl_concat("Missing vtable address: ", safe_str(name))),
    name(name)
{}

SymbolsXmlParse::SymbolsXmlParse(const char* desc, int id, int row, int col)
    :AllSymbols(stl_concat("error ", id, ": ", desc, ", at row ", row, " col ", col)),
    desc(safe_str(desc)), id(id), row(row), col(col)
{}

SymbolsXmlBadAttribute::SymbolsXmlBadAttribute(const char *attr)
    :AllSymbols(stl_concat("attribute is either missing or invalid: ", safe_str(attr))),
    attr(safe_str(attr))
{}

SymbolsXmlNoRoot::SymbolsXmlNoRoot()
    :AllSymbols("no root element")
{}

SymbolsXmlUnderspecifiedEntry::SymbolsXmlUnderspecifiedEntry(const char *where)
    :AllSymbols(stl_concat("Underspecified symbol file entry, each entry needs to set both the name attribute and have a value. parent: ",
        safe_str(where))),
    where(safe_str(where))
{}
