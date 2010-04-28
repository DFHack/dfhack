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

#include "DFTypes.h"
#include "modules/Translation.h"
#include "DF_Helpers.cpp"

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
			
			PySys_WriteStdout("tran_Ptr = 0x%x\n", self->tran_Ptr);
			
			delete self->tran_Ptr;
			
			PySys_WriteStdout("tran_Ptr deleted\n");
			
			self->tran_Ptr = NULL;
		}
		
		self->ob_type->tp_free((PyObject*)self);
	}
	
	PySys_WriteStdout("translate dealloc done\n");
}

// Type methods

static PyObject* DF_Translate_Start(DF_Translate* self, PyObject* args)
{
	DFHack::Dicts* t_dicts;
	std::vector<std::string> wordVec;
	PyObject *list, *tempDict;
	
	if(self->tran_Ptr != NULL)
	{
		if(self->tran_Ptr->Start())
		{
			Py_CLEAR(self->dict);
			
			t_dicts = self->tran_Ptr->getDicts();
			
			DFHack::DFDict & translations = t_dicts->translations;
			DFHack::DFDict & foreign_languages = t_dicts->foreign_languages;
			
			self->dict = PyDict_New();
			tempDict = PyDict_New();
			
			for(uint32_t i = 0; i < translations.size(); i++)
			{
				uint32_t size = translations[i].size();
				uint32_t j = 0;
				
				list = PyList_New(size);
				
				for(; j < size; j++)
					PyList_SET_ITEM(list, j, PyString_FromString(translations[i][j].c_str()));
				
				PyObject* key = PyInt_FromLong(j);
				PyDict_SetItem(tempDict, key, list);
				
				Py_DECREF(key);
				Py_DECREF(list);
			}
			
			PyDict_SetItemString(self->dict, "translations", tempDict);
			
			Py_DECREF(tempDict);
			
			tempDict = PyDict_New();
			
			for(uint32_t i = 0; i < foreign_languages.size(); i++)
			{
				uint32_t size = foreign_languages[i].size();
				uint32_t j = 0;
				
				list = PyList_New(size);
				
				for(; j < size; j++)
					PyList_SET_ITEM(list, j, PyString_FromString(foreign_languages[i][j].c_str()));
				
				PyObject* key = PyInt_FromLong(j);
				PyDict_SetItem(tempDict, key, list);
				
				Py_DECREF(key);
				Py_DECREF(list);
			}
			
			PyDict_SetItemString(self->dict, "foreign_languages", tempDict);
			
			Py_DECREF(tempDict);
			
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
		{
			if(self->dict != NULL)
				PyDict_Clear(self->dict);
			
			Py_CLEAR(self->dict);
			
			Py_RETURN_TRUE;
		}
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
		
		if(PyObject_IsInstance(nameObj, Name_type) != 1)
		{
			PyErr_SetString(PyExc_TypeError, "argument 1 must be a Name object");
			return NULL;
		}
		
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

// Getters/Setters

static PyObject* DF_Translate_getDicts(DF_Translate* self, PyObject* args)
{	
	if(self->tran_Ptr != NULL)
	{
		if(self->dict != NULL)
			return self->dict;
	}
	
	Py_RETURN_NONE;
}

static PyGetSetDef DF_Translate_getterSetters[] =
{
    {"dictionaries", (getter)DF_Translate_getDicts, NULL, "dictionaries", NULL},
    {NULL}  // Sentinel
};

static PyTypeObject DF_Translate_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pydfhack._TranslationManager",             /*tp_name*/
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
    "pydfhack TranslationManager object",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    DF_Translate_methods,             /* tp_methods */
    0,                      /* tp_members */
    DF_Translate_getterSetters,      /* tp_getset */
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
