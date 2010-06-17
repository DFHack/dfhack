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

#ifndef __DFBUILDINGS__
#define __DFBUILDINGS__

#include "Python.h"
#include <map>
#include <string>
#include "DFIntegers.h"

using namespace std;

#include "DFTypes.h"
#include "modules/Buildings.h"
#include "DF_Helpers.cpp"

using namespace DFHack;

static PyObject* BuildBuilding(DFHack::t_building& building)
{
	PyObject* t_dict;
	PyObject* temp;
	
	t_dict = PyDict_New();
	
	temp = Py_BuildValue("(si)(si)(si)(sO)(s((ii)(ii)i))", \
		"origin", building.origin, \
		"vtable", building.vtable, \
		"type", building.type, \
		"material", BuildMatglossPair(building.material), \
		"bounds", Py_BuildValue("(ii)(ii)i", building.x1, building.y1, building.x2, building.y2, building.z));
	
	PyDict_MergeFromSeq2(t_dict, temp, 0);
	
	return t_dict;
}

static DFHack::t_building ReverseBuildBuilding(PyObject* bDict)
{
	PyObject* temp;
	uint32_t x1, y1, x2, y2, z;
	DFHack::t_building building;
	
	building.origin = (uint32_t)PyInt_AsLong(PyDict_GetItemString(bDict, "origin"));
	building.vtable = (uint32_t)PyInt_AsLong(PyDict_GetItemString(bDict, "vtable"));
	building.material = ReverseBuildMatglossPair(PyDict_GetItemString(bDict, "material"));
	building.type = (uint32_t)PyInt_AsLong(PyDict_GetItemString(bDict, "type"));
	
	temp = PyDict_GetItemString(bDict, "bounds");
	
	PyArg_ParseTuple(temp, "(ii)(ii)i", &x1, &y1, &x2, &y2, &z);
	
	building.x1 = x1;
	building.y1 = y1;
	building.x2 = x2;
	building.y2 = y2;
	building.z = z;
	
	return building;
}

struct DF_Building
{
	PyObject_HEAD
	DFHack::Buildings* b_Ptr;
};

static PyObject* DF_Building_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	DF_Building* self;
	
	self = (DF_Building*)type->tp_alloc(type, 0);
	
	if(self != NULL)
		self->b_Ptr = NULL;
	
	return (PyObject*)self;
}

static int DF_Building_init(DF_Building* self, PyObject* args, PyObject* kwds)
{
	return 0;
}

static void DF_Building_dealloc(DF_Building* self)
{
	PySys_WriteStdout("building dealloc\n");
	
	if(self != NULL)
	{
		PySys_WriteStdout("building not NULL\n");
		
		if(self->b_Ptr != NULL)
		{
			PySys_WriteStdout("b_Ptr = 0x%x\n", self->b_Ptr);
			
			delete self->b_Ptr;
			
			PySys_WriteStdout("b_Ptr deleted\n");
			
			self->b_Ptr = NULL;
		}
		
		self->ob_type->tp_free((PyObject*)self);
	}
	
	PySys_WriteStdout("building dealloc done\n");
}

// Type methods

static PyObject* DF_Building_Start(DF_Building* self, PyObject* args)
{
	uint32_t numBuildings = 0;
	
	if(self->b_Ptr != NULL)
	{
		if(self->b_Ptr->Start(numBuildings))
			return PyInt_FromLong(numBuildings);
		else
			return PyInt_FromLong(-1);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Building_Finish(DF_Building* self, PyObject* args)
{
	if(self->b_Ptr != NULL)
	{
		if(self->b_Ptr->Finish())
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Building_Read(DF_Building* self, PyObject* args)
{
	uint32_t index = 0;
	t_building building;
	
	if(self->b_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "I", &index))
			return NULL;
		
		if(self->b_Ptr->Read(index, building))
			return BuildBuilding(building);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Building_ReadCustomWorkshopTypes(DF_Building* self, PyObject* args)
{
	PyObject* bDict;
	std::map<uint32_t, string> bTypes;
	std::map<uint32_t, string>::iterator bIter;
	
	if(self->b_Ptr != NULL)
	{
		if(self->b_Ptr->ReadCustomWorkshopTypes(bTypes))
		{
			bDict = PyDict_New();
			
			for(bIter = bTypes.begin(); bIter != bTypes.end(); bIter++)
			{
				PyObject* temp = Py_BuildValue("is", (*bIter).first, (*bIter).second.c_str());
				
				PyDict_MergeFromSeq2(bDict, temp, 1);
			}
			
			return bDict;
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Building_GetCustomWorkshopType(DF_Building* self, PyObject* args)
{
	DFHack::t_building building;
	
	if(self->b_Ptr != NULL)
	{
		building = ReverseBuildBuilding(args);
		
		return PyInt_FromLong(self->b_Ptr->GetCustomWorkshopType(building));
	}
	
	Py_RETURN_NONE;
}

static PyMethodDef DF_Building_methods[] =
{
    {"Start", (PyCFunction)DF_Building_Start, METH_NOARGS, ""},
    {"Finish", (PyCFunction)DF_Building_Finish, METH_NOARGS, ""},
    {"Read", (PyCFunction)DF_Building_Read, METH_VARARGS, ""},
	{"Read_Custom_Workshop_Types", (PyCFunction)DF_Building_ReadCustomWorkshopTypes, METH_NOARGS, ""},
	{"Get_Custom_Workshop_Type", (PyCFunction)DF_Building_GetCustomWorkshopType, METH_VARARGS, ""},
    {NULL}  // Sentinel
};

static PyTypeObject DF_Building_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pydfhack.Building",             /*tp_name*/
    sizeof(DF_Building), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DF_Building_dealloc,                         /*tp_dealloc*/
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
    "pydfhack Building objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    DF_Building_methods,             /* tp_methods */
    0,                      /* tp_members */
    0,      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)DF_Building_init,      /* tp_init */
    0,                         /* tp_alloc */
    DF_Building_new,                 /* tp_new */
};

#endif
