#include "Python.h"
#include "UnionBase.cpp"
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
	
	if(PyType_Ready(&UnionBase_type) < 0)
		return;
	
	if(PyType_Ready(&DF_API_type) < 0)
		return;
	
	module = Py_InitModule3("pydfhack", module_methods, "pydfhack extension module");
	
	Py_INCREF(&UnionBase_type);
	Py_INCREF(&DF_API_type);
	
	PyModule_AddObject(module, "UnionBase", (PyObject*)&UnionBase_type);
	PyModule_AddObject(module, "API", (PyObject*)&DF_API_type);
}