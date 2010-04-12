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

#ifndef __DFCREATURETYPE__
#define __DFCREATURETYPE__

#include "Python.h"
#include "structmember.h"
//#include "DF_Imports.cpp"
#include "DF_Helpers.cpp"
#include "modules/Creatures.h"

using namespace DFHack;

struct DF_Creature_Base
{
	PyObject_HEAD
	
	// simple type stuff
	uint32_t origin;
	uint32_t race;
	uint8_t profession;
	uint16_t mood;
	uint32_t happiness;
	uint32_t c_id;
	int32_t squad_leader_id;
	uint8_t sex;
	uint32_t pregnancy_timer;
	uint32_t flags1, flags2;
	
	PyObject* custom_profession;
	
	// composites
	PyObject* position;
	PyObject *name, *artifact_name;
	PyObject* current_job;
	PyObject *strength, *agility, *toughness, *endurance, *recuperation, *disease_resistance;
	PyObject* defaultSoul;
	
	// lists
	PyObject* labor_list;
};

// API type Allocation, Deallocation, and Initialization

static PyObject* DF_Creature_Base_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	DF_Creature_Base* self;
	
	self = (DF_Creature_Base*)type->tp_alloc(type, 0);
	
	if(self != NULL)
	{
		self->origin = 0;
		self->profession = 0;
		self->mood = 0;
		self->happiness = 0;
		self->c_id = 0;
		self->squad_leader_id = 0;
		self->sex = 0;
		self->pregnancy_timer = 0;
		self->flags1 = 0;
		self->flags2 = 0;
		
		self->custom_profession = PyString_FromString("");
		self->name = PyString_FromString("");
		self->artifact_name = PyString_FromString("");
		
		self->strength = NULL;
		self->agility = NULL;
		self->toughness = NULL;
		self->endurance = NULL;
		self->recuperation = NULL;
		self->disease_resistance = NULL;
		
		self->defaultSoul = NULL;
		
		self->labor_list = NULL;
	}
	
	return (PyObject*)self;
}

static int DF_Creature_Base_init(DF_Creature_Base* self, PyObject* args, PyObject* kwd)
{
	return 0;
}

static void DF_Creature_Base_dealloc(DF_Creature_Base* self)
{
	if(self != NULL)
	{
		Py_XDECREF(self->position);

		Py_XDECREF(self->custom_profession);	
		Py_XDECREF(self->name);
		Py_XDECREF(self->artifact_name);
		Py_XDECREF(self->current_job);
		
		Py_XDECREF(self->strength);
		Py_XDECREF(self->agility);
		Py_XDECREF(self->toughness);
		Py_XDECREF(self->endurance);
		Py_XDECREF(self->recuperation);
		Py_XDECREF(self->disease_resistance);
		
		Py_XDECREF(self->defaultSoul);
		
		Py_XDECREF(self->labor_list);
		
		self->ob_type->tp_free((PyObject*)self);
	}
}

static PyMemberDef DF_Creature_Base_members[] =
{
	{"origin", T_UINT, offsetof(DF_Creature_Base, origin), 0, ""},
	{"_flags1", T_UINT, offsetof(DF_Creature_Base, flags1), 0, ""},
	{"_flags2", T_UINT, offsetof(DF_Creature_Base, flags2), 0, ""},
	{"name", T_OBJECT_EX, offsetof(DF_Creature_Base, name), 0, ""},
	{"artifact_name", T_OBJECT_EX, offsetof(DF_Creature_Base, artifact_name), 0, ""},
	{"profession", T_INT, offsetof(DF_Creature_Base, profession), 0, ""},
	{"custom_profession", T_OBJECT_EX, offsetof(DF_Creature_Base, custom_profession), 0, ""},
	{"happiness", T_SHORT, offsetof(DF_Creature_Base, happiness), 0, ""},
	{"mood", T_SHORT, offsetof(DF_Creature_Base, mood), 0, ""},
	{"squad_leader_id", T_INT, offsetof(DF_Creature_Base, squad_leader_id), 0, ""},
	{"sex", T_UBYTE, offsetof(DF_Creature_Base, sex), 0, ""},
	{"pregnancy_timer", T_UINT, offsetof(DF_Creature_Base, pregnancy_timer), 0, ""},
	{NULL}	//Sentinel
};

static PyGetSetDef DF_Creature_Base_getterSetters[] =
{
	{NULL}	//Sentinel
};

static PyMethodDef DF_Creature_Base_methods[] =
{
	{NULL}	//Sentinel
};

static PyTypeObject DF_Creature_Base_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pydfhack.Creature_Base",             /*tp_name*/
    sizeof(DF_Creature_Base), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DF_Creature_Base_dealloc,                         /*tp_dealloc*/
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
    "pydfhack CreatureBase objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    DF_Creature_Base_methods,             /* tp_methods */
    DF_Creature_Base_members,                      /* tp_members */
    DF_Creature_Base_getterSetters,      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)DF_Creature_Base_init,      /* tp_init */
    0,                         /* tp_alloc */
    DF_Creature_Base_new,                 /* tp_new */
};

static PyObject* BuildCreature(DFHack::t_creature& creature)
{
	DF_Creature_Base* obj;
	
	obj = (DF_Creature_Base*)PyObject_Call((PyObject*)&DF_Creature_Base_type, NULL, NULL);
	
	if(obj != NULL)
	{
		obj->position = Py_BuildValue("III", creature.x, creature.y, creature.z);
		obj->profession = creature.profession;
		obj->mood = creature.mood;
		obj->happiness = creature.happiness;
		obj->c_id = creature.id;
		obj->agility = BuildAttribute(creature.agility);
		obj->strength = BuildAttribute(creature.strength);
		obj->toughness = BuildAttribute(creature.toughness);
		obj->endurance = BuildAttribute(creature.endurance);
		obj->recuperation = BuildAttribute(creature.recuperation);
		obj->disease_resistance = BuildAttribute(creature.disease_resistance);
		obj->squad_leader_id = creature.squad_leader_id;
		obj->sex = creature.sex;
		obj->pregnancy_timer = creature.pregnancy_timer;
		
		if(creature.custom_profession[0])
			obj->custom_profession = PyString_FromString(creature.custom_profession);
		
		obj->flags1 = creature.flags1.whole;
		obj->flags2 = creature.flags2.whole;
		
		obj->current_job = BuildJob(creature.current_job);
		obj->name = BuildName(creature.name);
		obj->artifact_name = BuildName(creature.artifact_name);
		
		obj->labor_list = PyList_New(NUM_CREATURE_LABORS);
		
		for(int i = 0; i < NUM_CREATURE_LABORS; i++)
			PyList_SET_ITEM(obj->labor_list, i, PyInt_FromLong(creature.labors[i]));
		
		return (PyObject*)obj;
	}
	
	Py_RETURN_NONE;
}

#endif