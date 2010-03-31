#ifndef __UNIONBASE__
#define __UNIONBASE__

#include <Python.h>

struct UnionBase
{
	PyObject_HEAD
	unsigned int whole;
};

static PyObject* UnionBase_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	UnionBase* self;
	
	self = (UnionBase*)type->tp_alloc(type, 0);
	
	if(self != NULL)
		self->whole = 0;
	else
		return NULL;
	
	return (PyObject*)self;
}

static int UnionBase_init(UnionBase* self, PyObject* args, PyObject* kwds)
{
	if(!PyArg_ParseTuple(args, "|I", &self->whole))
		return -1;
	
	return 0;
}

static void UnionBase_dealloc(UnionBase* self)
{
	self->ob_type->tp_free((PyObject*)self);
}

static PyObject* UnionBase_GetBitMask(UnionBase* self, PyObject* args)
{
	unsigned int mask;
	
	if(!PyArg_ParseTuple(args, "I", &mask))
		Py_RETURN_NONE;
	
	if((self->whole & mask) != 0)
		Py_RETURN_TRUE;
	
	Py_RETURN_FALSE;
}

static PyObject* UnionBase_SetBitMask(UnionBase* self, PyObject* args)
{
	unsigned int mask;
	int on;
	
	if(!PyArg_ParseTuple(args, "Ii", &mask, &on))
		return NULL;
		
	if(on)
		self->whole |= mask;
	else
		self->whole &= ~mask;
	
	Py_RETURN_TRUE;
}

static PyObject* UnionBase_GetSingleBit(UnionBase* self, PyObject* args)
{
	unsigned int mask;
	
	if(!PyArg_ParseTuple(args, "I", &mask))
		Py_RETURN_NONE;
	
	if((self->whole & (1 << mask)) != 0)
		Py_RETURN_TRUE;
	
	Py_RETURN_FALSE;
}

static PyObject* UnionBase_SetSingleBit(UnionBase* self, PyObject* args)
{
	unsigned int mask;
	int on;
	
	if(!PyArg_ParseTuple(args, "Ii", &mask, &on))
		return NULL;
	
	if(on)
		self->whole |= (1 << mask);
	else
		self->whole &= ~(1 << mask);
	
	Py_RETURN_TRUE;
}

static PyObject* UnionBase_int(PyObject* self)
{
	return PyInt_FromLong(((UnionBase*)self)->whole);
}

static PyObject* UnionBase_long(PyObject* self)
{
	return PyInt_FromLong(((UnionBase*)self)->whole);
}

static PyObject* UnionBase_hex(PyObject* self)
{
	return PyNumber_ToBase(PyInt_FromLong(((UnionBase*)self)->whole), 16);
}

static PyMethodDef UnionBase_methods[] = 
{
	{"_get_mask_bit", (PyCFunction)UnionBase_GetSingleBit, METH_VARARGS, "Get mask bit"},
	{"_set_mask_bit", (PyCFunction)UnionBase_SetSingleBit, METH_VARARGS, "Set mask bit"},
	{"_get_mask", (PyCFunction)UnionBase_GetBitMask, METH_VARARGS, ""},
	{"_set_mask", (PyCFunction)UnionBase_SetBitMask, METH_VARARGS, ""},
	{NULL}		//Sentinel
};

static PyNumberMethods UnionBase_as_number[] = 
{
	0,               			// binaryfunc nb_add;         /* __add__ */
    0,               			// binaryfunc nb_subtract;    /* __sub__ */
    0,               			// binaryfunc nb_multiply;    /* __mul__ */
    0,               			// binaryfunc nb_divide;      /* __div__ */
    0,               			// binaryfunc nb_remainder;   /* __mod__ */
    0,            				// binaryfunc nb_divmod;      /* __divmod__ */
    0,               			// ternaryfunc nb_power;      /* __pow__ */
    0,               			// unaryfunc nb_negative;     /* __neg__ */
    0,               			// unaryfunc nb_positive;     /* __pos__ */
    0,               			// unaryfunc nb_absolute;     /* __abs__ */
    0,           				// inquiry nb_nonzero;        /* __nonzero__ */
    0,            				// unaryfunc nb_invert;       /* __invert__ */
    0,            				// binaryfunc nb_lshift;      /* __lshift__ */
    0,           				// binaryfunc nb_rshift;      /* __rshift__ */
    0,               			// binaryfunc nb_and;         /* __and__ */
    0,               			// binaryfunc nb_xor;         /* __xor__ */
    0,                			// binaryfunc nb_or;          /* __or__ */
    0,            				// coercion nb_coerce;        /* __coerce__ */
    UnionBase_int,              // unaryfunc nb_int;          /* __int__ */
    UnionBase_long,             // unaryfunc nb_long;         /* __long__ */
    0,             				// unaryfunc nb_float;        /* __float__ */
    0,               			// unaryfunc nb_oct;          /* __oct__ */
    UnionBase_hex,              // unaryfunc nb_hex;          /* __hex__ */
};

static PyTypeObject UnionBase_type =
{
	PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pydfhack.UnionBase",             /*tp_name*/
    sizeof(UnionBase), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)UnionBase_dealloc,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    UnionBase_as_number,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "pydfhack UnionBase objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    UnionBase_methods,             /* tp_methods */
    0,                     /* tp_members */
    0,      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)UnionBase_init,      /* tp_init */
    0,                         /* tp_alloc */
    UnionBase_new,                 /* tp_new */
};

#endif