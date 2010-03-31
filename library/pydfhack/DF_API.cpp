#ifndef __DFAPI__
#define __DFAPI__

#include "Python.h"
#include <string>
#include <vector>
#include "DFTypes.h"
#include "DFHackAPI.h"
#include "UnionBase.cpp"

using namespace std;
using namespace DFHack;

struct DF_API
{
	PyObject_HEAD
	DFHack::API* api_Ptr;
};

// API type Allocation, Deallocation, and Initialization

static PyObject* DF_API_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	DF_API* self;
	
	self = (DF_API*)type->tp_alloc(type, 0);
	
	if(self != NULL)
		self->api_Ptr = NULL;
	
	return (PyObject*)self;
}

static int DF_API_init(DF_API* self, PyObject* args, PyObject* kwds)
{
	const char* memFileString = NULL;
	
	if(self->api_Ptr == NULL)
	{
		if(!PyArg_ParseTuple(args, "s", &memFileString))
			return -1;
		
		if(memFileString)
			self->api_Ptr = new DFHack::API(std::string(memFileString));
		else
			return -1;
	}
	
	return 0;
}

static void DF_API_dealloc(DF_API* self)
{
	if(self != NULL)
	{
		if(self->api_Ptr != NULL)
		{
			delete self->api_Ptr;
			
			self->api_Ptr = NULL;
		}
		
		self->ob_type->tp_free((PyObject*)self);
	}
}

// Accessors

static PyObject* DF_API_getIsAttached(DF_API* self, void* closure)
{
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->isAttached())
                Py_RETURN_TRUE;
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to read attached flag");
		Py_RETURN_FALSE;
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_getIsSuspended(DF_API* self, void* closure)
{
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->isSuspended())
                Py_RETURN_TRUE;
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to read suspension flag");
		Py_RETURN_FALSE;
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_getIsPaused(DF_API* self, void* closure)
{
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->ReadPauseState())
                Py_RETURN_TRUE;
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to read pause state");
		Py_RETURN_FALSE;
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_getMenuState(DF_API* self, void* closure)
{
    uint32_t menuState = 0;
    
    try
    {
        if(self->api_Ptr != NULL)
        {
            menuState = self->api_Ptr->ReadMenuState();
            
            return PyInt_FromLong(menuState);
        }
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to read menu state");
		Py_RETURN_NONE;
    }
    
    Py_RETURN_NONE;
}

static PyObject* DF_API_getViewCoords(DF_API* self, void* closure)
{
    int32_t x, y, z;
    bool success;
    
    try
    {
        if(self->api_Ptr != NULL)
        {
            success = self->api_Ptr->getViewCoords(x, y, z);
            
            if(success)
            {
                return Py_BuildValue("iii", x, y, z);
            }
            else
                Py_RETURN_NONE;
        }
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to get view coordinates");
		Py_RETURN_NONE;
    }
    
    Py_RETURN_NONE;
}

static PyObject* DF_API_getSize(DF_API* self, void* closure)
{
	uint32_t x, y, z;
	
	try
    {
        if(self->api_Ptr != NULL)
        {
            self->api_Ptr->getSize(x, y, z);
            return Py_BuildValue("III", x, y, z);
        }
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to get view coordinates");
		Py_RETURN_NONE;
    }
    
    Py_RETURN_NONE;
}

static PyGetSetDef DF_API_getterSetters[] =
{
    {"is_attached", (getter)DF_API_getIsAttached, NULL, "is_attached", NULL},
    {"is_suspended", (getter)DF_API_getIsSuspended, NULL, "is_suspended", NULL},
    {"is_paused", (getter)DF_API_getIsPaused, NULL, "is_paused", NULL},
    {"menu_state", (getter)DF_API_getMenuState, NULL, "menu_state", NULL},
    {"view_coords", (getter)DF_API_getViewCoords, NULL, "view_coords", NULL},
	{"map_size", (getter)DF_API_getSize, NULL, "max_size", NULL},
    {NULL}  // Sentinel
};

// API type methods

static PyObject* DF_API_Attach(DF_API* self)
{
    if(self->api_Ptr != NULL)
        if(self->api_Ptr->Attach())
            Py_RETURN_TRUE;
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_Detach(DF_API* self)
{
    if(self->api_Ptr != NULL)
        if(self->api_Ptr->Detach())
            Py_RETURN_TRUE;
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_Suspend(DF_API* self)
{
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->Suspend())
                Py_RETURN_TRUE;
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to suspend");
		Py_RETURN_FALSE;
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_Resume(DF_API* self)
{
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->Resume())
                Py_RETURN_TRUE;
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to resume");
		Py_RETURN_FALSE;
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_AsyncSuspend(DF_API* self)
{
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->AsyncSuspend())
                Py_RETURN_TRUE;
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to asynchronously suspend");
		Py_RETURN_FALSE;
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_ForceResume(DF_API* self)
{
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->ForceResume())
                Py_RETURN_TRUE;
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to force resume");
		Py_RETURN_FALSE;
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_InitMap(DF_API* self)
{
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->InitMap())
                Py_RETURN_TRUE;
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to initialize map");
		Py_RETURN_FALSE;
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_DestroyMap(DF_API* self)
{
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->DestroyMap())
                Py_RETURN_TRUE;
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to destroy map");
		Py_RETURN_FALSE;
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_InitReadConstructions(DF_API* self)
{
    uint32_t numConstructions = 0;
    
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->InitReadConstructions(numConstructions))
                return PyInt_FromLong(numConstructions);
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to read constructions");
    }
    
    Py_RETURN_NONE;
}

static PyObject* DF_API_FinishReadConstructions(DF_API* self)
{    
    try
    {
        if(self->api_Ptr != NULL)
        {
            self->api_Ptr->FinishReadConstructions();
            Py_RETURN_TRUE;
        }
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to finish reading constructions");
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_InitReadBuildings(DF_API* self)
{
    uint32_t numBuildings = 0;
    
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->InitReadBuildings(numBuildings))
                return PyInt_FromLong(numBuildings);
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to read buildings");
    }
    
    Py_RETURN_NONE;
}

static PyObject* DF_API_FinishReadBuildings(DF_API* self)
{    
    try
    {
        if(self->api_Ptr != NULL)
        {
            self->api_Ptr->FinishReadBuildings();
            Py_RETURN_TRUE;
        }
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to finish reading buildings");
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_InitReadEffects(DF_API* self)
{
    uint32_t numEffects = 0;
    
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->InitReadEffects(numEffects))
                return PyInt_FromLong(numEffects);
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to read effects");
    }
    
    Py_RETURN_NONE;
}

static PyObject* DF_API_FinishReadEffects(DF_API* self)
{    
    try
    {
        if(self->api_Ptr != NULL)
        {
            self->api_Ptr->FinishReadEffects();
            Py_RETURN_TRUE;
        }
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to finish reading effects");
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_InitReadVegetation(DF_API* self)
{
    uint32_t numVegetation = 0;
    
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->InitReadVegetation(numVegetation))
                return PyInt_FromLong(numVegetation);
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to read vegetation");
    }
    
    Py_RETURN_NONE;
}

static PyObject* DF_API_FinishReadVegetation(DF_API* self)
{    
    try
    {
        if(self->api_Ptr != NULL)
        {
            self->api_Ptr->FinishReadVegetation();
            Py_RETURN_TRUE;
        }
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to finish reading vegetation");
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_InitReadCreatures(DF_API* self)
{
    uint32_t numCreatures = 0;
    
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->InitReadCreatures(numCreatures))
                return PyInt_FromLong(numCreatures);
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to read creatures");
    }
    
    Py_RETURN_NONE;
}

static PyObject* DF_API_FinishReadCreatures(DF_API* self)
{    
    try
    {
        if(self->api_Ptr != NULL)
        {
            self->api_Ptr->FinishReadCreatures();
            Py_RETURN_TRUE;
        }
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to finish reading creatures");
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_InitReadNotes(DF_API* self)
{
    uint32_t numNotes = 0;
    
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->InitReadNotes(numNotes))
                return PyInt_FromLong(numNotes);
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to read notes");
    }
    
    Py_RETURN_NONE;
}

static PyObject* DF_API_FinishReadNotes(DF_API* self)
{    
    try
    {
        if(self->api_Ptr != NULL)
        {
            self->api_Ptr->FinishReadNotes();
            Py_RETURN_TRUE;
        }
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to finish reading notes");
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_InitReadSettlements(DF_API* self)
{
    uint32_t numSettlements = 0;
    
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->InitReadSettlements(numSettlements))
                return PyInt_FromLong(numSettlements);
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to read settlements");
    }
    
    Py_RETURN_NONE;
}

static PyObject* DF_API_FinishReadSettlements(DF_API* self)
{    
    try
    {
        if(self->api_Ptr != NULL)
        {
            self->api_Ptr->FinishReadSettlements();
            Py_RETURN_TRUE;
        }
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to finish reading settlements");
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_InitReadItems(DF_API* self)
{
    uint32_t numItems = 0;
    
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->InitReadItems(numItems))
                return PyInt_FromLong(numItems);
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to read items");
    }
    
    Py_RETURN_NONE;
}

static PyObject* DF_API_FinishReadItems(DF_API* self)
{    
    try
    {
        if(self->api_Ptr != NULL)
        {
            self->api_Ptr->FinishReadItems();
            Py_RETURN_TRUE;
        }
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to finish reading items");
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_InitReadHotkeys(DF_API* self)
{
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->InitReadHotkeys())
                Py_RETURN_TRUE;
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to initialize hotkey reader");
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_InitViewSize(DF_API* self)
{
    try
    {
        if(self->api_Ptr != NULL)
            if(self->api_Ptr->InitViewSize())
                Py_RETURN_TRUE;
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ValueError, "Error trying to initialize view size");
    }
    
    Py_RETURN_FALSE;
}

static PyObject* DF_API_IsValidBlock(DF_API* self, PyObject* args)
{
	uint32_t blockx, blocky, blockz;
	bool valid;
	
	if(self->api_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "III", &blockx, &blocky, &blockz))
			Py_RETURN_NONE;
		
		valid = self->api_Ptr->isValidBlock(blockx, blocky, blockz);
		
		if(valid)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_FALSE;
}

static PyObject* DF_API_ReadDesignations(DF_API* self, PyObject* args)
{
	uint32_t blockx, blocky, blockz;
	DFHack::designations40d designations;
	PyObject* list = NULL;
	
	if(self->api_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "III", &blockx, &blocky, &blockz))
			Py_RETURN_NONE;
		
		self->api_Ptr->ReadDesignations(blockx, blocky, blockz, &designations);
		
		list = PyList_New(16);
		
		for(int i = 0; i < 16; i++)
		{
			PyObject* innerList = PyList_New(16);
			
			for(int j = 0; j < 16; j++)
			{
				PyList_SetItem(innerList, j, PyInt_FromLong(designations[i][j].whole));
			}
			
			PyList_SetItem(list, i, innerList);
		}
	}
	
	return list;
}

static PyObject* DF_API_WriteDesignations(DF_API* self, PyObject* args)
{
	uint32_t blockx, blocky, blockz;
	DFHack::designations40d designations;
	PyObject* list;
	
	if(self->api_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "IIIO", &blockx, &blocky, &blockz, &list))
			Py_RETURN_NONE;
		
		for(int i = 0; i < 16; i++)
		{
			PyObject* innerList = PyList_GetItem(list, i);
			
			for(int j = 0; j < 16; j++)
			{
				UnionBase* obj = (UnionBase*)PyList_GetItem(innerList, j);
				
				designations[i][j].whole = obj->whole;
			}
		}
		
		self->api_Ptr->WriteDesignations(blockx, blocky, blockz, &designations);
		
		Py_RETURN_TRUE;
	}
	
	Py_RETURN_FALSE;
}

static PyObject* DF_API_ReadOccupancy(DF_API* self, PyObject* args)
{
	uint32_t blockx, blocky, blockz;
	DFHack::occupancies40d occupancies;
	PyObject* list = NULL;
	
	if(self->api_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "III", &blockx, &blocky, &blockz))
			Py_RETURN_NONE;
		
		self->api_Ptr->ReadOccupancy(blockx, blocky, blockz, &occupancies);
		
		list = PyList_New(16);
		
		for(int i = 0; i < 16; i++)
		{
			PyObject* innerList = PyList_New(16);
			
			for(int j = 0; j < 16; j++)
			{
				PyList_SetItem(innerList, j, PyInt_FromLong(occupancies[i][j].whole));
			}
			
			PyList_SetItem(list, i, innerList);
		}
	}
	
	return list;
}

static PyObject* DF_API_WriteOccupancy(DF_API* self, PyObject* args)
{
	uint32_t blockx, blocky, blockz;
	DFHack::occupancies40d occupancies;
	PyObject* list;
	
	if(self->api_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "IIIO", &blockx, &blocky, &blockz, &list))
			Py_RETURN_NONE;
		
		for(int i = 0; i < 16; i++)
		{
			PyObject* innerList = PyList_GetItem(list, i);
			
			for(int j = 0; j < 16; j++)
			{
				UnionBase* obj = (UnionBase*)PyList_GetItem(innerList, j);
				
				occupancies[i][j].whole = obj->whole;
			}
		}
		
		self->api_Ptr->WriteOccupancy(blockx, blocky, blockz, &occupancies);
		
		Py_RETURN_TRUE;
	}
	
	Py_RETURN_FALSE;
}

static PyObject* DF_API_ReadDirtyBit(DF_API* self, PyObject* args)
{
	uint32_t blockx, blocky, blockz;
	bool dirty;
	
	if(self->api_Ptr == NULL)
		return NULL;
	else
	{
		if(!PyArg_ParseTuple(args, "III", &blockx, &blocky, &blockz))
			return NULL;
		
		self->api_Ptr->ReadDirtyBit(blockx, blocky, blockz, dirty);
		
		if(dirty)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
}

static PyObject* DF_API_WriteDirtyBit(DF_API* self, PyObject* args)
{
	uint32_t blockx, blocky, blockz;
	int dirtyFlag;
	
	if(self->api_Ptr == NULL)
		return NULL;
	else
	{
		if(!PyArg_ParseTuple(args, "IIIi", &blockx, &blocky, &blockz, &dirtyFlag))
			return NULL;
		
		self->api_Ptr->WriteDirtyBit(blockx, blocky, blockz, dirtyFlag);
		
		Py_RETURN_TRUE;
	}
}

static PyMethodDef DF_API_methods[] =
{
    {"Attach", (PyCFunction)DF_API_Attach, METH_NOARGS, "Attach to the DF process"},
    {"Detach", (PyCFunction)DF_API_Detach, METH_NOARGS, "Detach from the DF process"},
    {"Suspend", (PyCFunction)DF_API_Suspend, METH_NOARGS, "Suspend the DF process"},
    {"Resume", (PyCFunction)DF_API_Resume, METH_NOARGS, "Resume the DF process"},
    {"Async_Suspend", (PyCFunction)DF_API_AsyncSuspend, METH_NOARGS, "Asynchronously suspend the DF process"},
    {"Force_Resume", (PyCFunction)DF_API_ForceResume, METH_NOARGS, "Force the DF process to resume"},
    {"Init_Map", (PyCFunction)DF_API_InitMap, METH_NOARGS, "Initialize the DFHack map reader"},
    {"Destroy_Map", (PyCFunction)DF_API_DestroyMap, METH_NOARGS, "Shut down the DFHack map reader"},
    {"Init_Read_Constructions", (PyCFunction)DF_API_InitReadConstructions, METH_NOARGS, "Initialize construction reader"},
    {"Finish_Read_Constructions", (PyCFunction)DF_API_FinishReadConstructions, METH_NOARGS, "Shut down construction reader"},
    {"Init_Read_Buildings", (PyCFunction)DF_API_InitReadBuildings, METH_NOARGS, "Initialize building reader"},
    {"Finish_Read_Buildings", (PyCFunction)DF_API_FinishReadBuildings, METH_NOARGS, "Shut down building reader"},
    {"Init_Read_Effects", (PyCFunction)DF_API_InitReadEffects, METH_NOARGS, "Initialize effect reader"},
    {"Finish_Read_Effects", (PyCFunction)DF_API_FinishReadEffects, METH_NOARGS, "Shut down effect reader"},
    {"Init_Read_Vegetation", (PyCFunction)DF_API_InitReadVegetation, METH_NOARGS, "Initialize vegetation reader"},
    {"Finish_Read_Vegetation", (PyCFunction)DF_API_FinishReadVegetation, METH_NOARGS, "Shut down vegetation reader"},
    {"Init_Read_Creatures", (PyCFunction)DF_API_InitReadCreatures, METH_NOARGS, "Initialize creature reader"},
    {"Finish_Read_Creatures", (PyCFunction)DF_API_FinishReadCreatures, METH_NOARGS, "Shut down creature reader"},
    {"Init_Read_Notes", (PyCFunction)DF_API_InitReadNotes, METH_NOARGS, "Initialize note reader"},
    {"Finish_Read_Notes", (PyCFunction)DF_API_FinishReadNotes, METH_NOARGS, "Shut down note reader"},
    {"Init_Read_Settlements", (PyCFunction)DF_API_InitReadSettlements, METH_NOARGS, "Initialize settlement reader"},
    {"Finish_Read_Settlements", (PyCFunction)DF_API_FinishReadSettlements, METH_NOARGS, "Shut down settlement reader"},
    {"Init_Read_Items", (PyCFunction)DF_API_InitReadItems, METH_NOARGS, "Initialize item reader"},
    {"Finish_Read_Items", (PyCFunction)DF_API_FinishReadItems, METH_NOARGS, "Shut down item reader"},
    {"Init_Read_Hotkeys", (PyCFunction)DF_API_InitReadHotkeys, METH_NOARGS, "Initialize hotkey reader"},
    {"Init_View_Size", (PyCFunction)DF_API_InitViewSize, METH_NOARGS, "Initialize view size reader"},
	{"Is_Valid_Block", (PyCFunction)DF_API_IsValidBlock, METH_VARARGS, ""},
	{"Read_Designations", (PyCFunction)DF_API_ReadDesignations, METH_VARARGS, "Read a block's designations"},	
	{"Write_Designations", (PyCFunction)DF_API_WriteDesignations, METH_VARARGS, ""},
	{"Read_Occupancy", (PyCFunction)DF_API_ReadOccupancy, METH_VARARGS, ""},
	{"Write_Occupancy", (PyCFunction)DF_API_WriteOccupancy, METH_VARARGS, ""},
	{"Read_Dirty_Bit", (PyCFunction)DF_API_ReadDirtyBit, METH_VARARGS, ""},
	{"Write_Dirty_Bit", (PyCFunction)DF_API_WriteDirtyBit, METH_VARARGS, ""},
    {NULL}  // Sentinel
};

static PyTypeObject DF_API_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pydfhack.API",             /*tp_name*/
    sizeof(DF_API), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DF_API_dealloc,                         /*tp_dealloc*/
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
    "pydfhack API objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    DF_API_methods,             /* tp_methods */
    0,                      /* tp_members */
    DF_API_getterSetters,      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)DF_API_init,      /* tp_init */
    0,                         /* tp_alloc */
    DF_API_new,                 /* tp_new */
};

#endif