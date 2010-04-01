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

#ifndef __MATGLOSS__
#define __MATGLOSS__

#include <Python.h>
#include <stdio.h>
#include <string.h>
#include "DFTypes.h"

using namespace DFHack;

// MatGloss

struct MatGloss
{
	PyObject_HEAD
	DFHack::t_matgloss native;
};

static PyObject* MatGloss_new(PyTypeObject* type, PyObject* arg, PyObject* kwds)
{
	MatGloss* self;
	
	self = (MatGloss*)type->tp_alloc(type, 0);
	
	self->native->id[0] = '\0';
	self->native->name[0] = '\0';
	
	return (PyObject*)self;
}

static void MatGloss_dealloc(MatGloss* self)
{
	self->ob_type->tp_free((PyObject*)self);
}

static PyObject* MatGloss_GetID(MatGloss* self, void* closure)
{
	return PyString_FromString(self->native->id);
}

static int MatGloss_SetID(MatGloss* self, PyObject* args, void* closure)
{
	char* temp;
	
	if(!PyArg_ParseTuple(args, "s", &temp))
		return -1;
	
	strncpy(self->id, temp, 127);
	self->native->id[127] = '\0';
	
	return 0;
}

static PyObject* MatGloss_GetName(MatGloss* self, PyObject* args, void* closure)
{
	return PyString_FromString(self->native->name);
}

static PyObject* MatGloss_SetName(MatGloss* self, PyObject* args, void* closure)
{
	char* temp;
	
	if(!PyArg_ParseTuple(args, "s", &temp))
		return -1;
	
	strncpy(self->native->name, temp, 127);
	self->native->name[127] = '\0';
	
	return 0;
}

static PyGetSetDef MatGloss_getSetters[] = 
{
	{"id", (getter)MatGloss_GetID, (setter)MatGloss_SetID, "id", NULL},
	{"name", (getter)MatGloss_GetName, (setter)MatGloss_SetName, "name", NULL},
	{NULL}
};

// MatGlossPlant

struct MatGlossPlant
{
	PyObject_HEAD
	DFHack::t_matglossPlant native;
};

static PyObject* MatGlossPlant_new(PyTypeObject* type, PyObject* arg, PyObject* kwds)
{
	MatGlossPlant* self;
	
	self = (MatGlossPlant*)type->tp_alloc(type, 0);
	
	self->native->id[0] = '\0';
	self->native->name[0] = '\0';
	self->native->drink_name[127] = '\0';
	self->native->food_name[127] = '\0';
	self->native->extract_name[127] = '\0';
	
	return (PyObject*)self;
}

static void MatGlossPlant_dealloc(MatGlossPlant* self)
{
	self->ob_type->tp_free((PyObject*)self);
}

static PyObject* MatGlossPlant_GetID(MatGlossPlant* self, void* closure)
{
	return PyString_FromString(self->native->id);
}

static int MatGlossPlant_SetID(MatGlossPlant* self, PyObject* args, void* closure)
{
	char* temp;
	
	if(!PyArg_ParseTuple(args, "s", &temp))
		return -1;
	
	strncpy(self->native->id, temp, 127);
	self->native->id[127] = '\0';
	
	return 0;
}

static PyObject* MatGlossPlant_GetName(MatGlossPlant* self, PyObject* args, void* closure)
{
	return PyString_FromString(self->native->name);
}

static PyObject* MatGlossPlant_SetName(MatGlossPlant* self, PyObject* args, void* closure)
{
	char* temp;
	
	if(!PyArg_ParseTuple(args, "s", &temp))
		return -1;
	
	strncpy(self->native->name, temp, 127);
	self->native->name[127] = '\0';
	
	return 0;
}

static PyObject* MatGlossPlant_GetDrinkName(MatGlossPlant* self, PyObject* args, void* closure)
{
	return PyString_FromString(self->native->drink_name);
}

static PyObject* MatGlossPlant_SetDrinkName(MatGlossPlant* self, PyObject* args, void* closure)
{
	char* temp;
	
	if(!PyArg_ParseTuple(args, "s", &temp))
		return -1;
	
	strncpy(self->native->drink_name, temp, 127);
	self->native->drink_name[127] = '\0';
	
	return 0;
}

static PyObject* MatGlossPlant_GetFoodName(MatGlossPlant* self, PyObject* args, void* closure)
{
	return PyString_FromString(self->native->food_name);
}

static PyObject* MatGlossPlant_SetFoodName(MatGlossPlant* self, PyObject* args, void* closure)
{
	char* temp;
	
	if(!PyArg_ParseTuple(args, "s", &temp))
		return -1;
	
	strncpy(self->native->food_name, temp, 127);
	self->native->food_name[127] = '\0';
	
	return 0;
}

static PyObject* MatGlossPlant_GetExtractName(MatGlossPlant* self, PyObject* args, void* closure)
{
	return PyString_FromString(self->native->extract_name);
}

static PyObject* MatGlossPlant_SetExtractName(MatGlossPlant* self, PyObject* args, void* closure)
{
	char* temp;
	
	if(!PyArg_ParseTuple(args, "s", &temp))
		return -1;
	
	strncpy(self->native->extract_name, temp, 127);
	self->native->extract_name[127] = '\0';
	
	return 0;
}

static PyGetSetDef MatGlossPlant_getSetters[] = 
{
	{"id", (getter)MatGlossPlant_GetID, (setter)MatGlossPlant_SetID, "id", NULL},
	{"name", (getter)MatGlossPlant_GetName, (setter)MatGlossPlant_SetName, "name", NULL},
	{"drink_name", (getter)MatGlossPlant_GetDrinkName, (setter)MatGlossPlant_SetDrinkName, "drink_name", NULL},
	{"food_name", (getter)MatGlossPlant_GetFoodName, (setter)MatGlossPlant_SetFoodName, "food_name", NULL},
	{"extract_name", (getter)MatGlossPlant_GetExtractName, (setter)MatGlossPlant_SetExtractName, "extract_name", NULL},
	{NULL}
};

#endif