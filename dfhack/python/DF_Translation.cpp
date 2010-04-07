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

#ifndef __DFTRANSLATION__
#define __DFTRANSLATION__

#include "Python.h"
#include "modules/Translation.h"

using namespace

struct DF_Translation
{
	PyObject_HEAD
	DFHack::Translation* trans_Ptr;
};

// API type Allocation, Deallocation, and Initialization

static PyObject* DF_Translation_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	DF_Translation* self;
	
	self = (DF_Translation*)type->tp_alloc(type, 0);
	
	if(self != NULL)
		self->trans_Ptr = NULL;
	
	return (PyObject*)self;
}

static int DF_Translation_init(DF_Translation* self, PyObject* args, PyObject* kwds)
{
	return 0;
}

static void DF_Translation_dealloc(DF_Translation* self)
{
	if(self != NULL)
	{		
		if(self->trans_Ptr != NULL)
		{
			delete self->trans_Ptr;
			
			self->trans_Ptr = NULL;
		}
		
		self->ob_type->tp_free((PyObject*)self);
	}
}

// Type methods

static PyObject* DF_Translation_Start(DF_Translation* self, PyObject* args)
{
	if(self->trans_Ptr != NULL)
	{
		if(self->trans_Ptr->Start())
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Translation_Finish(DF_Translation* self, PyObject* args)
{
	if(self->trans_Ptr != NULL)
	{
		if(self->trans_Ptr->Finish())
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Translation_TranslateName(DF_Translation* self, PyObject* args)
{
}

#endif