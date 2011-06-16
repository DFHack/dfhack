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

#ifndef MODULE_FACTORY_H_INCLUDED
#define MODULE_FACTORY_H_INCLUDED

namespace DFHack
{
    class Module;
    class DFContextShared;
    Module* createCreatures(DFContextShared * d);
    Module* createEngravings(DFContextShared * d);
    Module* createGui(DFContextShared * d);
    Module* createWorld(DFContextShared * d);
    Module* createMaterials(DFContextShared * d);
    Module* createItems(DFContextShared * d);
    Module* createTranslation(DFContextShared * d);
    Module* createVegetation(DFContextShared * d);
    Module* createBuildings(DFContextShared * d);
    Module* createConstructions(DFContextShared * d);
    Module* createMaps(DFContextShared * d);
}
#endif