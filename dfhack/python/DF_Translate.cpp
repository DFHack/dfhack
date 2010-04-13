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

#ifndef __DFTRANSLATE__
#define __DFTRANSLATE__

#include "Python.h"
#include <vector>
#include <string>

using namespace std;

#include "modules/Translation.h"

using namespace DFHack;

struct DF_Translate
{
	PyObject_HEAD
	PyObject* dict;
	DFHack::Translation* tran_Ptr;
};

// API type Allocation, Deallocation, and Initialization

static PyObject* DF_Translate_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	DF_Translate* self;
	
	self = (DF_Translate*)type->tp_alloc(type, 0);
	
	if(self != NULL)
		self->tran_Ptr = NULL;
	
	return (PyObject*)self;
}

static int DF_Translate_init(DF_Translate* self, PyObject* args, PyObject* kwds)
{
	return 0;
}

static void DF_Translate_dealloc(DF_Translate* self)
{
	PySys_WriteStdout("translate dealloc\n");
	
	if(self != NULL)
	{
		PySys_WriteStdout("translate not NULL\n");
		
		if(self->tran_Ptr != NULL)
		{
			Py_XDECREF(self->dict);
			
			PySys_WriteStdout("tran_Ptr = %i\n", (int)self->tran_Ptr);
			
			delete self->tran_Ptr;
			
			PySys_WriteStdout("tran_Ptr deleted\n");
			
			self->tran_Ptr = NULL;
		}
		
		self->ob_type->tp_free((PyObject*)self);
	}
	
	PySys_WriteStdout("translate dealloc done\n");
}

// Type methods

static PyObject* DF_Translate_GetDicts(DF_Translate* self, PyObject* args)
{
	PyObject* dict;
	Dicts* t_dicts;
	
	if(self->tran_Ptr != NULL)
	{
		t_dicts = self->tran_Ptr->getDicts();
	}
}

static PyObject* DF_Translate_Start(DF_Translate* self, PyObject* args)
{
	Dicts* t_dicts;
	
	if(self->tran_Ptr != NULL)
	{
		if(self->tran_Ptr->Start())
		{
			t_dicts = self->tran_Ptr->getDicts();
			
			Py_RETURN_TRUE;
		}
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Translate_Finish(DF_Translate* self, PyObject* args)
{
	if(self->tran_Ptr != NULL)
	{
		if(self->tran_Ptr->Finish())
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Translate_TranslateName(DF_Translate* self, PyObject* args)
{
	PyObject *nameObj, *retString;
	DFHack::t_name name;
	int inEnglish = 1;
	
	if(self->tran_Ptr != NULL)
	{
		if(PyArg_ParseTuple(args, "O|i", &nameObj, &inEnglish))
			return NULL;
		
		name = ReverseBuildName(nameObj);
		
		std::string nameStr = self->tran_Ptr->TranslateName(name, (bool)inEnglish);
		
		retString = PyString_FromString(nameStr.c_str());
		
		return retString;
	}
	
	Py_RETURN_NONE;
}



static PyMethodDef DF_Translate_methods[] =
{
	{"Start", (PyCFunction)DF_Translate_Start, METH_NOARGS, ""},
	{"Finish", (PyCFunction)DF_Translate_Finish, METH_NOARGS, ""},
	{"Translate_Name", (PyCFunction)DF_Translate_TranslateName, METH_VARARGS, ""},
	{NULL}	//Sentinel
};

static PyTypeObject DF_Translate_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pydfhack.Translate",             /*tp_name*/
    sizeof(DF_Translate), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DF_Translate_dealloc,                         /*tp_dealloc*/
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
    "pydfhack Translate objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    DF_Translate_methods,             /* tp_methods */
    0,                      /* tp_members */
    0,      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)DF_Translate_init,      /* tp_init */
    0,                         /* tp_alloc */
    DF_Translate_new,                 /* tp_new */
};

#endif