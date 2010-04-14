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

#include "Python.h"
//#include "DF_Imports.cpp"
#include "DF_MemInfo.cpp"
#include "DF_Material.cpp"
#include "DF_CreatureType.cpp"
#include "DF_CreatureManager.cpp"
#include "DF_Translate.cpp"
#include "DF_Vegetation.cpp"
#include "DF_Buildings.cpp"
#include "DF_API.cpp"

#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif

extern "C"
{
void ReadRaw(DF_API* self, const uint32_t offset, const uint32_t size, uint8_t* target)
{
	if(self != NULL && self->api_Ptr != NULL)
	{
		self->api_Ptr->ReadRaw(offset, size, target);
	}
}

void WriteRaw(DF_API* self, const uint32_t offset, const uint32_t size, uint8_t* source)
{
	if(self != NULL && self->api_Ptr != NULL)
	{
		self->api_Ptr->WriteRaw(offset, size, source);
	}
}
};

static PyMethodDef module_methods[] = 
{
	{NULL}		//Sentinel
};

PyMODINIT_FUNC initpydfhack(void)
{
	PyObject* module;
	
	if(PyType_Ready(&DF_API_type) < 0)
		return;
	
	if(PyType_Ready(&DF_MemInfo_type) < 0)
		return;
	
	if(PyType_Ready(&DF_Position_type) < 0)
		return;
		
	if(PyType_Ready(&DF_Material_type) < 0)
		return;
	
	if(PyType_Ready(&DF_Creature_Base_type) < 0)
		return;
	
	if(PyType_Ready(&DF_CreatureManager_type) < 0)
		return;
	
	if(PyType_Ready(&DF_Translate_type) < 0)
		return;
	
	if(PyType_Ready(&DF_Vegetation_type) < 0)
		return;
	
	if(PyType_Ready(&DF_Building_type) < 0)
		return;
	
	module = Py_InitModule3("pydfhack", module_methods, "pydfhack extension module");
	
	Py_INCREF(&DF_API_type);
	Py_INCREF(&DF_MemInfo_type);
	Py_INCREF(&DF_Position_type);
	Py_INCREF(&DF_Material_type);
	Py_INCREF(&DF_Creature_Base_type);
	Py_INCREF(&DF_CreatureManager_type);
	Py_INCREF(&DF_Translate_type);
	Py_INCREF(&DF_Vegetation_type);
	
	PyModule_AddObject(module, "API", (PyObject*)&DF_API_type);
	PyModule_AddObject(module, "MemInfo", (PyObject*)&DF_MemInfo_type);
	PyModule_AddObject(module, "Position", (PyObject*)&DF_Position_type);
	PyModule_AddObject(module, "Materials", (PyObject*)&DF_Material_type);
	PyModule_AddObject(module, "Creature_Base", (PyObject*)&DF_Creature_Base_type);
	PyModule_AddObject(module, "CreatureManager", (PyObject*)&DF_CreatureManager_type);
	PyModule_AddObject(module, "Translate", (PyObject*)&DF_Translate_type);
	PyModule_AddObject(module, "Vegetation", (PyObject*)&DF_Vegetation_type);
	PyModule_AddObject(module, "Building", (PyObject*)&DF_Building_type);
	
	//DoImports();
}