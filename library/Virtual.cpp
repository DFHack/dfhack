#include "Internal.h"

#include <string>
#include <vector>
#include <map>

#include "dfhack/Process.h"
#include "dfhack/Core.h"
#include "dfhack/Virtual.h"
using namespace DFHack;

std::string t_virtual::getClassName() const
{
    Core & c = Core::getInstance();
    return c.p->readClassName(vptr);
}