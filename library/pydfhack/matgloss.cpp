#ifndef __MATGLOSS__
#define __MATGLOSS__

#include <Python.h>
#include <stdio.h>
#include <string.h>
#include "DFTypes.h"

struct MatGloss
{
	PyObject_HEAD
	t_matgloss native;
};

static PyObject* MatGloss_new(PyTypeObject* type, PyObject* arg, PyObject* kwds)
{
	MatGloss* self;
	
	self = (MatGloss*)type->tp_alloc(type, 0);
	
	self->id[0] = '\0';
	self->name[0] = '\0';
}

static int MatGloss_init(MatGloss* self, PyObject* args, PyObject* kwds)
{
	return 0;
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
	self->id[127] = '\0';
	
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
	
	strncpy(self->name, temp, 127);
	self->name[127] = '\0';
	
	return 0;
}

static PyGetSetDef MatGloss_getSetters[] = 
{
	{"id", (getter)MatGloss_GetID, (setter)MatGloss_SetID, "id", NULL},
	{"name", (getter)MatGloss_GetName, (setter)MatGloss_SetName, "name", NULL},
	{NULL}
};

#endif