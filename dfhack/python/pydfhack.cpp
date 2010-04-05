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
//#include "UnionBase.cpp"
#include "DF_API.cpp"

#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif

static PyMethodDef module_methods[] = 
{
	{NULL}		//Sentinel
};

PyMODINIT_FUNC initpydfhack(void)
{
	PyObject* module;
	
	// if(PyType_Ready(&UnionBase_type) < 0)
		// return;
	
	if(PyType_Ready(&DF_API_type) < 0)
		return;
	
	module = Py_InitModule3("pydfhack", module_methods, "pydfhack extension module");
	
	//Py_INCREF(&UnionBase_type);
	Py_INCREF(&DF_API_type);
	
	//PyModule_AddObject(module, "UnionBase", (PyObject*)&UnionBase_type);
	PyModule_AddObject(module, "API", (PyObject*)&DF_API_type);
}