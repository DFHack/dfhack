/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf, doomchild

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

#ifndef __DFIMPORTS__
#define __DFIMPORTS__

#include "Python.h"

static PyObject* FlagsModule = NULL;
static PyObject* CreatureFlags1_type = NULL;
static PyObject* CreatureFlags2_type = NULL;
static PyObject* DesignationFlags_type = NULL;
static PyObject* OccupancyFlags_type = NULL;
static PyObject* ItemFlags_type = NULL;
static PyObject* BlockFlags_type = NULL;

static PyObject* TypesModule = NULL;
static PyObject* Note_type = NULL;
static PyObject* Construction_type = NULL;
static PyObject* Name_type = NULL;
static PyObject* MapBlock40d_type = NULL;
static PyObject* Vein_type = NULL;
static PyObject* FrozenLiquidVein_type = NULL;
static PyObject* SpatterVein_type = NULL;

static void DoImports()
{
	if(TypesModule == NULL)
	{
		FlagsModule = PyImport_ImportModule("pydfhackflags");
				
		CreatureFlags1_type = PyObject_GetAttrString(FlagsModule, "CreatureFlags1");
		CreatureFlags2_type = PyObject_GetAttrString(FlagsModule, "CreatureFlags2");
		DesignationFlags_type = PyObject_GetAttrString(FlagsModule, "DesignationFlags");
		OccupancyFlags_type = PyObject_GetAttrString(FlagsModule, "OccupancyFlags");
		ItemFlags_type = PyObject_GetAttrString(FlagsModule, "ItemFlags");
		BlockFlags_type = PyObject_GetAttrString(FlagsModule, "BlockFlags");
		
		TypesModule = PyImport_ImportModule("pydftypes");

		Note_type = PyObject_GetAttrString(TypesModule, "Note");
		Construction_type = PyObject_GetAttrString(TypesModule, "Construction");
		Name_type = PyObject_GetAttrString(TypesModule, "Name");
		MapBlock40d_type = PyObject_GetAttrString(TypesModule, "MapBlock40d");
		Vein_type = PyObject_GetAttrString(TypesModule, "Vein");
		FrozenLiquidVein_type = PyObject_GetAttrString(TypesModule, "FrozenLiquidVein");
		SpatterVein_type = PyObject_GetAttrString(TypesModule, "SpatterVein");
	}
}

#endif