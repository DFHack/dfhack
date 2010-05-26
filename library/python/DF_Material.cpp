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
#include <string>
#include "DFIntegers.h"

using namespace std;

#include "DFTypes.h"
#include "modules/Materials.h"
#include "DF_Imports.cpp"
#include "DF_Helpers.cpp"

using namespace DFHack;

struct DF_Material
{
	PyObject_HEAD
	DFHack::Materials* mat_Ptr;
};

// Helpers

static PyObject* BuildMatgloss(t_matgloss& matgloss)
{
	PyObject* matObj;
	PyObject* args;
	
	args = Py_BuildValue("siiis", matgloss.id, matgloss.fore, matgloss.back, matgloss.bright, matgloss.name);
	
	matObj = PyObject_CallObject(Matgloss_type, args);
	
	Py_DECREF(args);
	
	return matObj;
}

static PyObject* BuildMatglossPlant(t_matglossPlant& matgloss)
{
	PyObject* matDict;
	
	matDict = PyDict_New();
	
	if(matgloss.id[0])
		PyDict_SetItemString(matDict, "id", PyString_FromString(matgloss.id));
	else
		PyDict_SetItemString(matDict, "id", PyString_FromString(""));
		
	PyDict_SetItemString(matDict, "fore", PyInt_FromLong(matgloss.fore));
	PyDict_SetItemString(matDict, "back", PyInt_FromLong(matgloss.back));
	PyDict_SetItemString(matDict, "bright", PyInt_FromLong(matgloss.bright));
	
	if(matgloss.name[0])
		PyDict_SetItemString(matDict, "name", PyString_FromString(matgloss.name));
	else
		PyDict_SetItemString(matDict, "name", PyString_FromString(""));
	
	if(matgloss.drink_name[0])
		PyDict_SetItemString(matDict, "drink_name", PyString_FromString(matgloss.drink_name));
	else
		PyDict_SetItemString(matDict, "drink_name", PyString_FromString(""));
	
	if(matgloss.food_name[0])
		PyDict_SetItemString(matDict, "food_name", PyString_FromString(matgloss.food_name));
	else
		PyDict_SetItemString(matDict, "food_name", PyString_FromString(""));
	
	if(matgloss.extract_name[0])
		PyDict_SetItemString(matDict, "extract_name", PyString_FromString(matgloss.extract_name));
	else
		PyDict_SetItemString(matDict, "extract_name", PyString_FromString(""));
	
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
		
		Py_DECREF(matgloss);
	}
	
	return matList;
}

static PyObject* BuildDescriptorColor(t_descriptor_color& color)
{
	PyObject* descObj;
	PyObject* args;
	
	args = Py_BuildValue("sfffs", color.id, color.r, color.v, color.b, color.name);
	
	descObj = PyObject_CallObject(DescriptorColor_type, args);
	
	Py_DECREF(args);
	
	return descObj;
}

static PyObject* BuildDescriptorColorList(std::vector<t_descriptor_color>& colors)
{
	PyObject* colorList;
	std::vector<t_descriptor_color>::iterator colorIter;
	
	colorList = PyList_New(0);
	
	for(colorIter = colors.begin(); colorIter != colors.end(); colorIter++)
	{
		PyObject* color = BuildDescriptorColor(*colorIter);
		
		PyList_Append(colorList, color);
		
		Py_DECREF(colorList);
	}
	
	return colorList;
}

static PyObject* BuildCreatureCaste(t_creaturecaste& caste)
{
	PyObject* casteObj;
	PyObject* args;
	
	args = Py_BuildValue("ssss", caste.rawname, caste.singular, caste.plural, caste.adjective);
	
	casteObj = PyObject_CallObject(CreatureCaste_type, args);
	
	Py_DECREF(args);
	
	return casteObj;
}

static PyObject* BuildCreatureCasteList(std::vector<t_creaturecaste>& castes)
{
	PyObject* casteList;
	std::vector<t_creaturecaste>::iterator casteIter;
	
	casteList = PyList_New(0);
	
	for(casteIter = castes.begin(); casteIter != castes.end(); casteIter++)
	{
		PyObject* caste = BuildCreatureCaste(*casteIter);
		
		PyList_Append(casteList, caste);
		
		Py_DECREF(caste);
	}
	
	return casteList;
}

static PyObject* BuildCreatureTypeEx(t_creaturetype& creature)
{
	PyObject* cObj;
	PyObject* args;
	
	args = Py_BuildValue("sOiO", creature.rawname, BuildCreatureCasteList(creature.castes), creature.tile_character, \
									BuildTileColor(creature.tilecolor.fore, creature.tilecolor.back, creature.tilecolor.bright));
	
	cObj = PyObject_CallObject(CreatureTypeEx_type, args);
	
	Py_DECREF(args);
	
	return cObj;
}

static PyObject* BuildCreatureTypeExList(std::vector<t_creaturetype>& creatures)
{
	PyObject* creatureList;
	std::vector<t_creaturetype>::iterator creatureIter;
	
	creatureList = PyList_New(0);
	
	for(creatureIter = creatures.begin(); creatureIter != creatures.end(); creatureIter++)
	{
		PyObject* creature = BuildCreatureTypeEx(*creatureIter);
		
		PyList_Append(creatureList, creature);
		
		Py_DECREF(creature);
	}
	
	return creatureList;
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
	PySys_WriteStdout("material dealloc\n");
	
	if(self != NULL)
	{
		PySys_WriteStdout("material not NULL\n");
		
		if(self->mat_Ptr != NULL)
		{
			PySys_WriteStdout("mat_Ptr = 0x%x\n", self->mat_Ptr);
			
			delete self->mat_Ptr;
			
			PySys_WriteStdout("mat_Ptr deleted\n");
			
			self->mat_Ptr = NULL;
		}
		
		self->ob_type->tp_free((PyObject*)self);
	}
	
	PySys_WriteStdout("material dealloc done\n");
}

// Type methods

static PyObject* DF_Material_ReadInorganicMaterials(DF_Material* self, PyObject* args)
{
	if(self->mat_Ptr != NULL)
	{
		if(self->mat_Ptr->ReadInorganicMaterials())
		{
			return BuildMatglossList(self->mat_Ptr->inorganic);
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Material_ReadOrganicMaterials(DF_Material* self, PyObject* args)
{
	if(self->mat_Ptr != NULL)
	{
		if(self->mat_Ptr->ReadOrganicMaterials())
		{
			return BuildMatglossList(self->mat_Ptr->organic);
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Material_ReadWoodMaterials(DF_Material* self, PyObject* args)
{
	if(self->mat_Ptr != NULL)
	{
		if(self->mat_Ptr->ReadWoodMaterials())
		{
			return BuildMatglossList(self->mat_Ptr->tree);
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Material_ReadPlantMaterials(DF_Material* self, PyObject* args)
{
	if(self->mat_Ptr != NULL)
	{
		if(self->mat_Ptr->ReadPlantMaterials())
		{
			return BuildMatglossList(self->mat_Ptr->plant);
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Material_ReadCreatureTypes(DF_Material* self, PyObject* args)
{
	if(self->mat_Ptr != NULL)
	{
		if(self->mat_Ptr->ReadCreatureTypes())
		{
			return BuildMatglossList(self->mat_Ptr->race);
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Material_ReadCreatureTypesEx(DF_Material* self, PyObject* args)
{
	if(self->mat_Ptr != NULL)
	{
		if(self->mat_Ptr->ReadCreatureTypesEx())
		{
			return BuildCreatureTypeExList(self->mat_Ptr->raceEx);
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Material_ReadDescriptorColors(DF_Material* self, PyObject* args)
{
	if(self->mat_Ptr != NULL)
	{
		if(self->mat_Ptr->ReadDescriptorColors())
		{
			return BuildDescriptorColorList(self->mat_Ptr->color);
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
	{"Read_Creature_Types_Ex", (PyCFunction)DF_Material_ReadCreatureTypesEx, METH_NOARGS, ""},
	{"Read_Descriptor_Colors", (PyCFunction)DF_Material_ReadDescriptorColors, METH_NOARGS, ""},
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
