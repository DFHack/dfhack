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

#ifndef __DFPOSITION__
#define __DFPOSITION__

#include "Python.h"
#include "modules/Position.h"

using namespace DFHack;

struct DF_Position
{
	PyObject_HEAD
	DFHack::Position* pos_Ptr;
};

// API type Allocation, Deallocation, and Initialization

static PyObject* DF_Position_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	DF_Position* self;
	
	self = (DF_Position*)type->tp_alloc(type, 0);
	
	if(self != NULL)
		self->pos_Ptr = NULL;
	
	return (PyObject*)self;
}

static int DF_Position_init(DF_Position* self, PyObject* args, PyObject* kwds)
{
	return 0;
}

static void DF_Position_dealloc(DF_Position* self)
{
	if(self != NULL)
	{
		if(self->pos_Ptr != NULL)
		{
			delete self->pos_Ptr;
			
			self->pos_Ptr = NULL;
		}
		
		self->ob_type->tp_free((PyObject*)self);
	}
}

// Getters/Setters

static PyObject* DF_Position_getViewCoords(DF_Position* self, void* closure)
{
	int32_t x, y, z;
	
	if(self->pos_Ptr != NULL)
	{
		if(self->pos_Ptr->getViewCoords(x, y, z))
			return Py_BuildValue("iii", x, y, z);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Position_setViewCoords(DF_Position* self, PyObject* args)
{
	int32_t x, y, z;
	
	if(self->pos_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "iii", &x, &y, &z))
			return NULL;
		
		if(self->pos_Ptr->setViewCoords(x, y, z))
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Position_getCursorCoords(DF_Position* self, void* closure)
{
	int32_t x, y, z;
	
	if(self->pos_Ptr != NULL)
	{
		if(self->pos_Ptr->getCursorCoords(x, y, z))
			return Py_BuildValue("iii", x, y, z);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Position_setCursorCoords(DF_Position* self, PyObject* args)
{
	int32_t x, y, z;
	
	if(self->pos_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "iii", &x, &y, &z))
			return NULL;
		
		if(self->pos_Ptr->setCursorCoords(x, y, z))
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Position_getWindowSize(DF_Position* self, void* closure)
{
	int32_t x, y;
	
	if(self->pos_Ptr != NULL)
	{
		if(self->pos_Ptr->getWindowSize(x, y))
			return Py_BuildValue("ii", x, y);
	}
	
	Py_RETURN_NONE;
}

static PyGetSetDef DF_Position_getterSetters[] =
{
    {"view_coords", (getter)DF_Position_getViewCoords, (setter)DF_Position_setViewCoords, "view_coords", NULL},
    {"cursor_coords", (getter)DF_Position_getCursorCoords, (setter)DF_Position_setCursorCoords, "cursor_coords", NULL},
	{"window_size", (getter)DF_Position_getWindowSize, NULL, "window_size", NULL},
    {NULL}  // Sentinel
};

static PyTypeObject DF_Position_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pydfhack.Position",             /*tp_name*/
    sizeof(DF_Position), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DF_Position_dealloc,                         /*tp_dealloc*/
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
    "pydfhack Position objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0,             /* tp_methods */
    0,                      /* tp_members */
    DF_Position_getterSetters,      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)DF_Position_init,      /* tp_init */
    0,                         /* tp_alloc */
    DF_Position_new,                 /* tp_new */
};

#endif