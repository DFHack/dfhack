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

#ifndef __DFMATERIAL__
#define __DFMATERIAL__

#include "Python.h"
#include <vector>

using namespace std;

#include "modules/Materials.h"

using namespace DFHack;

struct DF_Material
{
	PyObject_HEAD
	DFHack::Materials* mat_Ptr;
};

// Helpers

static PyObject* BuildMatgloss(t_matgloss matgloss)
{
	PyObject* matDict;
	
	matDict = PyDict_New();
	
	PyDict_SetItemString(matDict, "id", PyString_FromString(matgloss.id));
	PyDict_SetItemString(matDict, "fore", PyInt_FromLong(matgloss.fore));
	PyDict_SetItemString(matDict, "back", PyInt_FromLong(matgloss.back));
	PyDict_SetItemString(matDict, "bright", PyInt_FromLong(matgloss.bright));
	PyDict_SetItemString(matDict, "name", PyString_FromString(matgloss.name));
	
	return matDict;
}

static PyObject* BuildMatglossPlant(t_matglossPlant matgloss)
{
	PyObject* matDict;
	
	matDict = PyDict_New();
	
	PyDict_SetItemString(matDict, "id", PyString_FromString(matgloss.id));
	PyDict_SetItemString(matDict, "fore", PyInt_FromLong(matgloss.fore));
	PyDict_SetItemString(matDict, "back", PyInt_FromLong(matgloss.back));
	PyDict_SetItemString(matDict, "bright", PyInt_FromLong(matgloss.bright));
	PyDict_SetItemString(matDict, "name", PyString_FromString(matgloss.name));
	PyDict_SetItemString(matDict, "drink_name", PyString_FromString(matgloss.drink_name));
	PyDict_SetItemString(matDict, "food_name", PyString_FromString(matgloss.food_name));
	PyDict_SetItemString(matDict, "extract_name", PyString_FromString(matgloss.extract_name));
	
	return matDict;
}

static PyObject* BuildMatglossList(std::vector<t_matgloss> & matVec)
{
	PyObject* matList;
	std::vector<t_matgloss>::iterator matIter;
	
	matList = PyList_New(0);
	
	for(matIter = matVec.begin(); matIter != matVec.end(); matIter++)
	{
		PyObject* matgloss = BuildMatgloss(*matIter);
		
		PyList_Append(matList, matgloss);
	}
	
	return matList;
}

// API type Allocation, Deallocation, and Initialization

static PyObject* DF_Material_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	DF_Material* self;
	
	self = (DF_Material*)type->tp_alloc(type, 0);
	
	if(self != NULL)
		self->mat_Ptr = NULL;
	
	return (PyObject*)self;
}

static int DF_Material_init(DF_Material* self, PyObject* args, PyObject* kwds)
{
	return 0;
}

static void DF_Material_dealloc(DF_Material* self)
{
	if(self != NULL)
	{
		if(self->mat_Ptr != NULL)
		{
			delete self->mat_Ptr;
			
			self->mat_Ptr = NULL;
		}
		
		self->ob_type->tp_free((PyObject*)self);
	}
}

// Type methods

static PyObject* DF_Material_ReadInorganicMaterials(DF_Material* self, PyObject* args)
{
	if(self->mat_Ptr != NULL)
	{
		std::vector<DFHack::t_matgloss> matVec;
		
		if(self->mat_Ptr->ReadInorganicMaterials(matVec))
		{
			return BuildMatglossList(matVec);
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Material_ReadOrganicMaterials(DF_Material* self, PyObject* args)
{
	if(self->mat_Ptr != NULL)
	{
		std::vector<DFHack::t_matgloss> matVec;
		
		if(self->mat_Ptr->ReadOrganicMaterials(matVec))
		{
			return BuildMatglossList(matVec);
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Material_ReadWoodMaterials(DF_Material* self, PyObject* args)
{
	if(self->mat_Ptr != NULL)
	{
		std::vector<DFHack::t_matgloss> matVec;
		
		if(self->mat_Ptr->ReadWoodMaterials(matVec))
		{
			return BuildMatglossList(matVec);
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Material_ReadPlantMaterials(DF_Material* self, PyObject* args)
{
	if(self->mat_Ptr != NULL)
	{
		std::vector<DFHack::t_matgloss> matVec;
		
		if(self->mat_Ptr->ReadPlantMaterials(matVec))
		{
			return BuildMatglossList(matVec);
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Material_ReadCreatureTypes(DF_Material* self, PyObject* args)
{
	if(self->mat_Ptr != NULL)
	{
		std::vector<DFHack::t_matgloss> matVec;
		
		if(self->mat_Ptr->ReadCreatureTypes(matVec))
		{
			return BuildMatglossList(matVec);
		}
	}
	
	Py_RETURN_NONE;
}

static PyMethodDef DF_Material_methods[] =
{
	{"Read_Inorganic_Materials", (PyCFunction)DF_Material_ReadInorganicMaterials, METH_NOARGS, ""},
	{"Read_Organic_Materials", (PyCFunction)DF_Material_ReadOrganicMaterials, METH_NOARGS, ""},
	{"Read_Wood_Materials", (PyCFunction)DF_Material_ReadWoodMaterials, METH_NOARGS, ""},
	{"Read_Plant_Materials", (PyCFunction)DF_Material_ReadPlantMaterials, METH_NOARGS, ""},
	{"Read_Creature_Types", (PyCFunction)DF_Material_ReadCreatureTypes, METH_NOARGS, ""},
	{NULL}	//Sentinel
};

static PyTypeObject DF_Material_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pydfhack.Material",             /*tp_name*/
    sizeof(DF_Material), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DF_Material_dealloc,                         /*tp_dealloc*/
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
    "pydfhack Material objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    DF_Material_methods,             /* tp_methods */
    0,                      /* tp_members */
    0,      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)DF_Material_init,      /* tp_init */
    0,                         /* tp_alloc */
    DF_Material_new,                 /* tp_new */
};

#endif