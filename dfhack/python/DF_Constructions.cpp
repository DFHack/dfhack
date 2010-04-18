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

#ifndef __DFCONSTRUCTIONS__
#define __DFCONSTRUCTIONS__

#include "Python.h"
#include "modules/Constructions.h"
#include "DF_Helpers.cpp"

using namespace DFHack;

static PyObject* BuildConstruction(DFHack::t_construction& construction)
{
	PyObject* t_dict;
	PyObject* temp;
	
	t_dict = PyDict_New();
	
	temp = PyTuple_Pack(3, construction.x, construction.y, construction.z);
	DICTADD(t_dict, "position", temp);
	
	temp = PyInt_FromLong(construction.form);
	DICTADD(t_dict, "form", temp);
	
	temp = PyInt_FromLong(construction.unk_8);
	DICTADD(t_dict, "unk_8", temp);
	
	temp = PyInt_FromLong(construction.mat_type);
	DICTADD(t_dict, "mat_type", temp);
	
	temp = PyInt_FromLong(construction.mat_idx);
	DICTADD(t_dict, "mat_idx", temp);
	
	temp = PyInt_FromLong(construction.unk3);
	DICTADD(t_dict, "unk3", temp);
	
	temp = PyInt_FromLong(construction.unk4);
	DICTADD(t_dict, "unk4", temp);
	
	temp = PyInt_FromLong(construction.unk5);
	DICTADD(t_dict, "unk5", temp);
	
	temp = PyInt_FromLong(construction.unk6);
	DICTADD(t_dict, "unk6", temp);
	
	temp = PyInt_FromLong(construction.origin);
	DICTADD(t_dict, "origin", temp);
	
	return t_dict;
}

struct DF_Construction
{
	PyObject_HEAD
	DFHack::Constructions* c_Ptr;
};

static PyObject* DF_Construction_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	DF_Construction* self;
	
	self = (DF_Construction*)type->tp_alloc(type, 0);
	
	if(self != NULL)
		self->c_Ptr = NULL;
	
	return (PyObject*)self;
}

static int DF_Construction_init(DF_Construction* self, PyObject* args, PyObject* kwds)
{
	return 0;
}

static void DF_Construction_dealloc(DF_Construction* self)
{
	PySys_WriteStdout("construction dealloc\n");
	
	if(self != NULL)
	{
		PySys_WriteStdout("construction not NULL\n");
		
		if(self->c_Ptr != NULL)
		{
			PySys_WriteStdout("c_Ptr = %i\n", (int)self->c_Ptr);
			
			delete self->c_Ptr;
			
			PySys_WriteStdout("c_Ptr deleted\n");
			
			self->c_Ptr = NULL;
		}
		
		self->ob_type->tp_free((PyObject*)self);
	}
	
	PySys_WriteStdout("construction dealloc done\n");
}

// Type methods

static PyObject* DF_Construction_Start(DF_Construction* self, PyObject* args)
{
	uint32_t numConstructions = 0;
	
	if(self->c_Ptr != NULL)
	{
		if(self->c_Ptr->Start(numConstructions))
			return PyInt_FromLong(numConstructions);
		else
			return PyInt_FromLong(-1);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Construction_Finish(DF_Construction* self, PyObject* args)
{
	if(self->c_Ptr != NULL)
	{
		if(self->c_Ptr->Finish())
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Construction_Read(DF_Construction* self, PyObject* args)
{
	uint32_t index = 0;
	t_construction construction;
	
	if(self->c_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "I", &index))
			return NULL;
		
		if(self->c_Ptr->Read(index, construction))
			return BuildConstruction(construction);
	}
	
	Py_RETURN_NONE;
}

static PyMethodDef DF_Construction_methods[] =
{
    {"Start", (PyCFunction)DF_Construction_Start, METH_NOARGS, ""},
    {"Finish", (PyCFunction)DF_Construction_Finish, METH_NOARGS, ""},
    {"Read", (PyCFunction)DF_Construction_Read, METH_VARARGS, ""},
    {NULL}  // Sentinel
};

static PyTypeObject DF_Construction_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pydfhack.Construction",             /*tp_name*/
    sizeof(DF_Construction), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DF_Construction_dealloc,                         /*tp_dealloc*/
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
    "pydfhack Construction objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    DF_Construction_methods,             /* tp_methods */
    0,                      /* tp_members */
    0,      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)DF_Construction_init,      /* tp_init */
    0,                         /* tp_alloc */
    DF_Construction_new,                 /* tp_new */
};

#endif