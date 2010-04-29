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

#ifndef __DFCREATURES__
#define __DFCREATURES__

#include "Python.h"
#include "stdio.h"
#include <vector>
#include "integers.h"
#include "DFTypes.h"
#include "modules/Creatures.h"
#include "DF_CreatureType.cpp"

using namespace DFHack;

struct DF_CreatureManager
{
	PyObject_HEAD
	DFHack::Creatures* creature_Ptr;
};

// API type Allocation, Deallocation, and Initialization

static PyObject* DF_CreatureManager_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	DF_CreatureManager* self;
	
	self = (DF_CreatureManager*)type->tp_alloc(type, 0);
	
	if(self != NULL)
		self->creature_Ptr = NULL;
	
	return (PyObject*)self;
}

static int DF_CreatureManager_init(DF_CreatureManager* self, PyObject* args, PyObject* kwds)
{
	return 0;
}

static void DF_CreatureManager_dealloc(DF_CreatureManager* self)
{
	PySys_WriteStdout("creature manager dealloc\n");
	
	if(self != NULL)
	{
		PySys_WriteStdout("creature manager not NULL\n");
		
		if(self->creature_Ptr != NULL)
		{
			PySys_WriteStdout("creature_Ptr = 0x%x\n", self->creature_Ptr);
			
			delete self->creature_Ptr;
			
			PySys_WriteStdout("creature_Ptr deleted\n");
			
			self->creature_Ptr = NULL;
		}
		
		self->ob_type->tp_free((PyObject*)self);
	}
	
	PySys_WriteStdout("creature manager dealloc done\n");
}

// Type methods

static PyObject* DF_CreatureManager_Start(DF_CreatureManager* self, PyObject* args)
{
	uint32_t numCreatures = 0;
	
	if(self->creature_Ptr != NULL)
	{		
		if(self->creature_Ptr->Start(numCreatures))
			return PyInt_FromLong(numCreatures);
		else
			return PyInt_FromLong(-1);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_CreatureManager_Finish(DF_CreatureManager* self, PyObject* args)
{
	if(self->creature_Ptr != NULL)
	{
		if(self->creature_Ptr->Finish())
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_CreatureManager_ReadCreature(DF_CreatureManager* self, PyObject* args)
{
	uint32_t index;
	DFHack::t_creature furball;
	
	if(self->creature_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "I", &index))
			return NULL;
		
		if(self->creature_Ptr->ReadCreature(index, furball))
		{
			return BuildCreature(furball);
		}
		else
		{
			Py_RETURN_FALSE;
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_CreatureManager_ReadCreatureInBox(DF_CreatureManager* self, PyObject* args)
{
	int32_t index;
	uint32_t x1, y1, z1, x2, y2, z2;
	DFHack::t_creature furball;
	
	if(self->creature_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "iIIIIII", &index, &x1, &y1, &z1, &x2, &y2, &z2))
			return NULL;
		
		if(self->creature_Ptr->ReadCreatureInBox(index, furball, x1, y1, z1, x2, y2, z2) >= 0)
			return BuildCreature(furball);
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_CreatureManager_GetDwarfRaceIndex(DF_CreatureManager* self, PyObject* args)
{
	if(self->creature_Ptr != NULL)
	{
		return PyInt_FromLong(self->creature_Ptr->GetDwarfRaceIndex());
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_CreatureManager_GetDwarfCivId(DF_CreatureManager* self, PyObject* args)
{
	if(self->creature_Ptr != NULL)
	{
		return PyInt_FromLong(self->creature_Ptr->GetDwarfCivId());
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_CreatureManager_WriteLabors(DF_CreatureManager* self, PyObject* args)
{
	int32_t index;
	PyObject* laborList;
	
	
	if(self->creature_Ptr != NULL)
	{
		uint8_t laborArray[NUM_CREATURE_LABORS];
		
		if(!PyArg_ParseTuple(args, "iO", &index, &laborList))
			return NULL;
		
		if(!PyList_Check(laborList))
		{
			PyErr_SetString(PyExc_TypeError, "argument 2 must be a list");
			return NULL;
		}
		
		if(PyList_Size(laborList) < NUM_CREATURE_LABORS)
		{
			char errBuff[50];
			
			sprintf(errBuff, "list must contain at least %u entries", NUM_CREATURE_LABORS);
			
			PyErr_SetString(PyExc_StandardError, errBuff);
			return NULL;
		}
		
		for(int i = 0; i < NUM_CREATURE_LABORS; i++)
			laborArray[i] = (uint8_t)PyInt_AsLong(PyList_GET_ITEM(laborList, i));
		
		if(self->creature_Ptr->WriteLabors(index, laborArray))
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyMethodDef DF_CreatureManager_methods[] =
{
    {"Start", (PyCFunction)DF_CreatureManager_Start, METH_NOARGS, ""},
    {"Finish", (PyCFunction)DF_CreatureManager_Finish, METH_NOARGS, ""},
    {"Read_Creature", (PyCFunction)DF_CreatureManager_ReadCreature, METH_VARARGS, ""},
    {"Read_Creature_In_Box", (PyCFunction)DF_CreatureManager_ReadCreatureInBox, METH_VARARGS, ""},
	{"Write_Labors", (PyCFunction)DF_CreatureManager_WriteLabors, METH_VARARGS, ""},
	{"Get_Dwarf_Race_Index", (PyCFunction)DF_CreatureManager_GetDwarfRaceIndex, METH_NOARGS, ""},
	{"Get_Dwarf_Civ_id", (PyCFunction)DF_CreatureManager_GetDwarfCivId, METH_NOARGS, ""},
    {NULL}  // Sentinel
};

static PyTypeObject DF_CreatureManager_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pydfhack.CreatureManager",             /*tp_name*/
    sizeof(DF_CreatureManager), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DF_CreatureManager_dealloc,                         /*tp_dealloc*/
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
    "pydfhack CreatureManager objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    DF_CreatureManager_methods,             /* tp_methods */
    0,                      /* tp_members */
    0,      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)DF_CreatureManager_init,      /* tp_init */
    0,                         /* tp_alloc */
    DF_CreatureManager_new,                 /* tp_new */
};

#endif
