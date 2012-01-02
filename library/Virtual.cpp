#include "Internal.h"

#include <string>
#include <vector>
#include <map>

#include "MemAccess.h"
#include "Core.h"
#include "Virtual.h"
using namespace DFHack;

std::string t_virtual::getClassName() const
{
    Core & c = Core::getInstance();
    return c.p->readClassName(vptr);
}