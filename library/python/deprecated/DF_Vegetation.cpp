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

#ifndef __DFVEGETATION__
#define __DFVEGETATION__

#include "Python.h"
#include "DFIntegers.h"
#include "modules/Vegetation.h"
#include "DF_Helpers.cpp"

using namespace DFHack;

static PyObject* BuildTree(DFHack::t_tree& tree)
{
	PyObject* t_Obj;
	PyObject* args;
	
	args = Py_BuildValue("iiOi", tree.type, tree.material, BuildPosition3D(tree.x, tree.y, tree.z), tree.address);
	
	t_Obj = PyObject_CallObject(Tree_type, args);
	
	Py_DECREF(args);
	
	return t_Obj;
}

struct DF_Vegetation
{
	PyObject_HEAD
	DFHack::Vegetation* veg_Ptr;
};

static PyObject* DF_Vegetation_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	DF_Vegetation* self;
	
	self = (DF_Vegetation*)type->tp_alloc(type, 0);
	
	if(self != NULL)
		self->veg_Ptr = NULL;
	
	return (PyObject*)self;
}

static int DF_Vegetation_init(DF_Vegetation* self, PyObject* args, PyObject* kwds)
{
	return 0;
}

static void DF_Vegetation_dealloc(DF_Vegetation* self)
{
	PySys_WriteStdout("vegetation dealloc\n");
	
	if(self != NULL)
	{
		PySys_WriteStdout("vegetation not NULL\n");
		
		if(self->veg_Ptr != NULL)
		{
			PySys_WriteStdout("veg_Ptr = 0x%x\n", self->veg_Ptr);
			
			delete self->veg_Ptr;
			
			PySys_WriteStdout("veg_Ptr deleted\n");
			
			self->veg_Ptr = NULL;
		}
		
		self->ob_type->tp_free((PyObject*)self);
	}
	
	PySys_WriteStdout("vegetation dealloc done\n");
}

// Type methods

static PyObject* DF_Vegetation_Start(DF_Vegetation* self, PyObject* args)
{
	uint32_t numTrees = 0;
	
	if(self->veg_Ptr != NULL)
	{
		if(self->veg_Ptr->Start(numTrees))
			return PyInt_FromLong(numTrees);
		else
			return PyInt_FromLong(-1);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Vegetation_Finish(DF_Vegetation* self, PyObject* args)
{
	if(self->veg_Ptr != NULL)
	{
		if(self->veg_Ptr->Finish())
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Vegetation_Read(DF_Vegetation* self, PyObject* args)
{
	uint32_t index = 0;
	t_tree tree;
	
	if(self->veg_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "I", &index))
			return NULL;
		
		if(self->veg_Ptr->Read(index, tree))
			return BuildTree(tree);
	}
	
	Py_RETURN_NONE;
}

static PyMethodDef DF_Vegetation_methods[] =
{
    {"Start", (PyCFunction)DF_Vegetation_Start, METH_NOARGS, ""},
    {"Finish", (PyCFunction)DF_Vegetation_Finish, METH_NOARGS, ""},
    {"Read", (PyCFunction)DF_Vegetation_Read, METH_VARARGS, ""},
    {NULL}  // Sentinel
};

static PyTypeObject DF_Vegetation_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pydfhack.Vegetation",             /*tp_name*/
    sizeof(DF_Vegetation), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DF_Vegetation_dealloc,                         /*tp_dealloc*/
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
    "pydfhack Vegetation objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    DF_Vegetation_methods,             /* tp_methods */
    0,                      /* tp_members */
    0,      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)DF_Vegetation_init,      /* tp_init */
    0,                         /* tp_alloc */
    DF_Vegetation_new,                 /* tp_new */
};

#endif
