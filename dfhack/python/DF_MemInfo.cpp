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

#ifndef __DF_MEMINFO__
#define __DF_MEMINFO__

#include "Python.h"
#include <string>

using namespace std;

#include "Export.h"
#include "DFCommonInternal.h"
#include "DFMemInfo.h"

using namespace DFHack;

struct DF_MemInfo
{
	PyObject_HEAD
	DFHack::memory_info* mem_Ptr;
};

// MemInfo type Allocation, Deallocation, and Initialization

static PyObject* DF_MemInfo_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	DF_MemInfo* self;
	
	self = (DF_MemInfo*)type->tp_alloc(type, 0);
	
	if(self != NULL)
		self->mem_Ptr = NULL;
	
	return (PyObject*)self;
}

static int DF_MemInfo_init(DF_MemInfo* self, PyObject* args, PyObject* kwds)
{
	return 0;
}

static void DF_MemInfo_dealloc(DF_MemInfo* self)
{
	if(self != NULL)
	{
		if(self->mem_Ptr != NULL)
		{
			delete self->mem_Ptr;
			
			self->mem_Ptr = NULL;
		}
		
		self->ob_type->tp_free((PyObject*)self);
	}
}

// Setters/Getters

static PyObject* DF_MemInfo_getBase(DF_MemInfo* self)
{
	if(self->mem_Ptr != NULL)
	{
		return PyInt_FromLong(self->mem_Ptr->getBase());
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_setBase(DF_MemInfo* self, PyObject* args)
{
	PyObject* arg = NULL;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "O", arg))
		{
			PyErr_SetString(PyExc_ValueError, "Bad argument list");
			return NULL;
		}
		
		if(PyString_Check(arg))
			self->mem_Ptr->setBase(std::string(PyString_AsString(arg)));
		else if(PyInt_Check(arg))
			self->mem_Ptr->setBase((uint32_t)PyInt_AsUnsignedLongMask(arg));
		else
		{
			PyErr_SetString(PyExc_ValueError, "Argument must be string or integer");
			return NULL;
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_getVersion(DF_MemInfo* self)
{
	if(self->mem_Ptr != NULL)
	{
		return PyString_FromString(self->mem_Ptr->getVersion().c_str());
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_setVersion(DF_MemInfo* self, PyObject* args)
{
	const char* v = NULL;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "s", &v))
			return NULL;
		
		self->mem_Ptr->setVersion(v);
	}
	
	Py_RETURN_NONE;
}

static PyGetSetDef DF_MemInfo_getterSetters[] =
{
    {"base", (getter)DF_MemInfo_getBase, (setter)DF_MemInfo_setBase, "base", NULL},
	{"version", (getter)DF_MemInfo_getVersion, (setter)DF_MemInfo_setVersion, "version", NULL},
    {NULL}  // Sentinel
};

// Member methods

static PyObject* DF_MemInfo_GetOffset(DF_MemInfo* self, PyObject* args)
{
	const char* s = NULL;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "s", &s))
			return NULL;
		
		return PyInt_FromLong(self->mem_Ptr->getOffset(s));
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_SetOffset(DF_MemInfo* self, PyObject* args)
{
	// const char* s = NULL;
	// const char* n = NULL;
	// int32_t i;
	// PyObject* obj;
	
	// if(self->mem_Ptr != NULL)
	// {
		// if(!PyArg_ParseTuple(args, "sO", &s, &obj))
			// return NULL;
		
		// if(PyString_Check(obj))
		// {
			// n = PyString_AsString(obj);
			// self->mem_Ptr->setOffset(s, n);
		// }
		// else if(PyInt_Check(obj))
		// {
			// i = (int32_t)PyInt_AsLong(obj);
			// self->mem_Ptr->setOffset(s, i);
		// }
	// }
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_GetAddress(DF_MemInfo* self, PyObject* args)
{
	const char* s = NULL;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "s", &s))
			return NULL;
		
		return PyInt_FromLong(self->mem_Ptr->getAddress(s));
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_SetAddress(DF_MemInfo* self, PyObject* args)
{
	// const char* s = NULL;
	// const char* n = NULL;
	// int32_t i;
	// PyObject* obj;
	
	// if(self->mem_Ptr != NULL)
	// {
		// if(!PyArg_ParseTuple(args, "sO", &s, &obj))
			// return NULL;
		
		// if(PyString_Check(obj))
		// {
			// n = PyString_AsString(obj);
			// self->mem_Ptr->setAddress(s, n);
		// }
		// else if(PyInt_Check(obj))
		// {
			// i = (int32_t)PyInt_AsLong(obj);
			// self->mem_Ptr->setAddress(s, i);
		// }
	// }
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_GetHexValue(DF_MemInfo* self, PyObject* args)
{
	const char* s = NULL;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "s", &s))
			return NULL;
		
		return PyInt_FromLong(self->mem_Ptr->getHexValue(s));
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_SetHexValue(DF_MemInfo* self, PyObject* args)
{
	// const char* s = NULL;
	// const char* n = NULL;
	// int32_t i;
	// PyObject* obj;
	
	// if(self->mem_Ptr != NULL)
	// {
		// if(!PyArg_ParseTuple(args, "sO", &s, &obj))
			// return NULL;
		
		// if(PyString_Check(obj))
		// {
			// n = PyString_AsString(obj);
			// self->mem_Ptr->setHexValue(s, n);
		// }
		// else if(PyInt_Check(obj))
		// {
			// i = (int32_t)PyInt_AsLong(obj);
			// self->mem_Ptr->setHexValue(s, i);
		// }
	// }
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_GetString(DF_MemInfo* self, PyObject* args)
{
	const char* s = NULL;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "s", &s))
			return NULL;
		
		return PyString_FromString(self->mem_Ptr->getString(s).c_str());
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_SetString(DF_MemInfo* self, PyObject* args)
{
	// const char* s = NULL;
	// const char* n = NULL;
	
	// if(self->mem_Ptr != NULL)
	// {
		// if(!PyArg_ParseTuple(args, "ss", &s, &n))
			// return NULL;
		
		// self->mem_Ptr->setString(s, n);
	// }
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_GetProfession(DF_MemInfo* self, PyObject* args)
{
	uint32_t s;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "I", &s))
			return NULL;
		
		return PyString_FromString(self->mem_Ptr->getProfession(s).c_str());
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_SetProfession(DF_MemInfo* self, PyObject* args)
{
	const char* s = NULL;
	const char* n = NULL;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "ss", &s, &n))
			return NULL;
		
		self->mem_Ptr->setProfession(s, n);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_GetJob(DF_MemInfo* self, PyObject* args)
{
	uint32_t s;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "I", &s))
			return NULL;
		
		return PyString_FromString(self->mem_Ptr->getJob(s).c_str());
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_SetJob(DF_MemInfo* self, PyObject* args)
{
	const char* s = NULL;
	const char* n = NULL;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "ss", &s, &n))
			return NULL;
		
		self->mem_Ptr->setJob(s, n);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_GetSkill(DF_MemInfo* self, PyObject* args)
{
	uint32_t s;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "I", &s))
			return NULL;
		
		return PyString_FromString(self->mem_Ptr->getSkill(s).c_str());
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_SetSkill(DF_MemInfo* self, PyObject* args)
{
	const char* s = NULL;
	const char* n = NULL;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "ss", &s, &n))
			return NULL;
		
		self->mem_Ptr->setSkill(s, n);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_GetTrait(DF_MemInfo* self, PyObject* args)
{
	uint32_t s, n;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "II", &s, &n))
			return NULL;
		
		return PyString_FromString(self->mem_Ptr->getTrait(s, n).c_str());
	}
	
	Py_RETURN_NONE;
}

// I have no idea what any of the strings are, so I can't really put meaningful names - doomchild
static PyObject* DF_MemInfo_SetTrait(DF_MemInfo* self, PyObject* args)
{
	const char* a = NULL;
	const char* b = NULL;
	const char* c = NULL;
	const char* d = NULL;
	const char* e = NULL;
	const char* f = NULL;
	const char* g = NULL;
	const char* h = NULL;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "ssssssss", &a, &b, &c, &d, &e, &f, &g, &h))
			return NULL;
		
		self->mem_Ptr->setTrait(a, b, c, d, e, f, g, h);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_GetTraitName(DF_MemInfo* self, PyObject* args)
{
	uint32_t s;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "I", &s))
			return NULL;
		
		return PyString_FromString(self->mem_Ptr->getTraitName(s).c_str());
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_GetLabor(DF_MemInfo* self, PyObject* args)
{
	uint32_t s;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "i", &s))
			return NULL;
		
		return PyString_FromString(self->mem_Ptr->getLabor(s).c_str());
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_SetLabor(DF_MemInfo* self, PyObject* args)
{
	const char* s = NULL;
	const char* n = NULL;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "ss", &s, &n))
			return NULL;
		
		self->mem_Ptr->setLabor(s, n);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_RebaseAddresses(DF_MemInfo* self, PyObject* args)
{
	int32_t base;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "i", &base))
			return NULL;
		
		self->mem_Ptr->RebaseAddresses(base);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_RebaseAll(DF_MemInfo* self, PyObject* args)
{
	int32_t base;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "i", &base))
			return NULL;
		
		self->mem_Ptr->RebaseAll(base);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_RebaseVTable(DF_MemInfo* self, PyObject* args)
{
	int32_t offset;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "i", &offset))
			return NULL;
		
		self->mem_Ptr->RebaseVTable(offset);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_ResolveObjectToClassID(DF_MemInfo* self, PyObject* args)
{
	uint32_t address;
	int32_t classID;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "I", &address))
			return NULL;
		
		if(self->mem_Ptr->resolveObjectToClassID(address, classID))
			return PyInt_FromLong(classID);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_ResolveClassNameToClassID(DF_MemInfo* self, PyObject* args)
{
	// const char* name;
	// int32_t classID;
	
	// if(self->mem_Ptr != NULL)
	// {
		// if(!PyArg_ParseTuple(args, "s", &name))
			// return NULL;
		
		// if(self->mem_Ptr->resolveClassnameToClassID(name, classID))
			// return PyInt_FromLong(classID);
	// }
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_ResolveClassNameToVPtr(DF_MemInfo* self, PyObject* args)
{
	const char* n;
	uint32_t vPtr;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "s", &n))
			return NULL;
		
		std::string name(n);
		
		if(self->mem_Ptr->resolveClassnameToVPtr(name, vPtr))
			return PyInt_FromLong(vPtr);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_MemInfo_ResolveClassIDToClassName(DF_MemInfo* self, PyObject* args)
{
	int32_t id;
	string name;
	
	if(self->mem_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "i", &id))
			return NULL;
		
		if(self->mem_Ptr->resolveClassIDToClassname(id, name))
			return PyString_FromString(name.c_str());
	}
	
	Py_RETURN_NONE;
}

static PyMethodDef DF_MemInfo_methods[] =
{
	{"Rebase_Addresses", (PyCFunction)DF_MemInfo_RebaseAddresses, METH_VARARGS, ""},
	{"Rebase_All", (PyCFunction)DF_MemInfo_RebaseAll, METH_VARARGS, ""},
	{"Rebase_VTable", (PyCFunction)DF_MemInfo_RebaseVTable, METH_VARARGS, ""},
	{"Get_Offset", (PyCFunction)DF_MemInfo_GetOffset, METH_VARARGS, ""},
	{"Set_Offset", (PyCFunction)DF_MemInfo_SetOffset, METH_VARARGS, ""},
	{"Get_Address", (PyCFunction)DF_MemInfo_GetAddress, METH_VARARGS, ""},
	{"Set_Address", (PyCFunction)DF_MemInfo_SetAddress, METH_VARARGS, ""},
	{"Get_HexValue", (PyCFunction)DF_MemInfo_GetHexValue, METH_VARARGS, ""},
	{"Set_HexValue", (PyCFunction)DF_MemInfo_SetHexValue, METH_VARARGS, ""},
	{"Get_String", (PyCFunction)DF_MemInfo_GetString, METH_VARARGS, ""},
	{"Set_String", (PyCFunction)DF_MemInfo_SetString, METH_VARARGS, ""},
	{"Get_Profession", (PyCFunction)DF_MemInfo_GetProfession, METH_VARARGS, ""},
	{"Set_Profession", (PyCFunction)DF_MemInfo_SetProfession, METH_VARARGS, ""},
	{"Get_Job", (PyCFunction)DF_MemInfo_GetJob, METH_VARARGS, ""},
	{"Set_Job", (PyCFunction)DF_MemInfo_SetJob, METH_VARARGS, ""},
	{"Get_Skill", (PyCFunction)DF_MemInfo_GetSkill, METH_VARARGS, ""},
	{"Set_Skill", (PyCFunction)DF_MemInfo_SetSkill, METH_VARARGS, ""},
	{"Get_Trait", (PyCFunction)DF_MemInfo_GetTrait, METH_VARARGS, ""},
	{"Set_Trait", (PyCFunction)DF_MemInfo_SetTrait, METH_VARARGS, ""},
	{"Get_Trait_Name", (PyCFunction)DF_MemInfo_GetTraitName, METH_VARARGS, ""},
	{"Get_Labor", (PyCFunction)DF_MemInfo_GetLabor, METH_VARARGS, ""},
	{"Set_Labor", (PyCFunction)DF_MemInfo_SetLabor, METH_VARARGS, ""},
	{"Resolve_Object_To_Class_ID", (PyCFunction)DF_MemInfo_ResolveObjectToClassID, METH_VARARGS, ""},
	{"Resolve_Classname_To_Class_ID", (PyCFunction)DF_MemInfo_ResolveClassNameToClassID, METH_VARARGS, ""},
	{"Resolve_Classname_To_VPtr", (PyCFunction)DF_MemInfo_ResolveClassNameToVPtr, METH_VARARGS, ""},
	{"Resolve_Class_ID_To_Classname", (PyCFunction)DF_MemInfo_ResolveClassIDToClassName, METH_VARARGS, ""},
	{NULL}	//Sentinel
};

static PyTypeObject DF_MemInfo_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pydfhack.DF_MemInfo",             /*tp_name*/
    sizeof(DF_MemInfo), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DF_MemInfo_dealloc,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "pydfhack DF_MemInfo objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    DF_MemInfo_methods,             /* tp_methods */
    0,                      /* tp_members */
    DF_MemInfo_getterSetters,      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)DF_MemInfo_init,      /* tp_init */
    0,                         /* tp_alloc */
    DF_MemInfo_new,                 /* tp_new */
};

#endif