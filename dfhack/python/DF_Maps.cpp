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

#ifndef __DFMAPS__
#define __DFMAPS__

#include "Python.h"
#include <vector>
#include <string>

using namespace std;

#include "DFTypes.h"
#include "modules/Maps.h"
#include "DF_Imports.cpp"
#include "DF_Helpers.cpp"

using namespace DFHack;

static PyObject* BuildVein(DFHack::t_vein& v)
{
	PyObject* vObj;
	PyObject* temp;
	
	vObj = PyObject_Call(Vein_type, NULL, NULL);
	
	temp = PyInt_FromLong(v.vtable);
	OBJSET(vObj, "vtable", temp);
	
	temp = PyInt_FromLong(v.type);
	OBJSET(vObj, "type", temp);
	
	temp = PyInt_FromLong(v.flags);
	OBJSET(vObj, "flags", temp);
	
	temp = PyInt_FromLong(v.address_of);
	OBJSET(vObj, "address", temp);
	
	temp = PyList_New(16);
	
	for(int i = 0; i < 16; i++)
		PyList_SET_ITEM(temp, i, PyInt_FromLong(v.assignment[i]));
	
	OBJSET(vObj, "assignment", temp);
	
	return vObj;
}

static PyObject* BuildFrozenLiquidVein(DFHack::t_frozenliquidvein& v)
{
	PyObject* vObj;
	PyObject *temp, *list;
	
	vObj = PyObject_Call(FrozenLiquidVein_type, NULL, NULL);
	
	temp = PyInt_FromLong(v.vtable);
	OBJSET(vObj, "vtable", temp);
	
	temp = PyInt_FromLong(v.address_of);
	OBJSET(vObj, "address", temp);
	
	list = PyList_New(16);
	
	for(int i = 0; i < 16; i++)
	{
		temp = PyList_New(16);
		
		for(int j = 0; j < 16; j++)
			PyList_SET_ITEM(temp, j, PyInt_FromLong(v.tiles[i][j]));
		
		PyList_SET_ITEM(list, i, temp);
	}
	
	OBJSET(vObj, "tiles", list);
	
	return vObj;
}

static PyObject* BuildSpatterVein(DFHack::t_spattervein& v)
{
	PyObject* vObj;
	PyObject *temp, *list;
	
	vObj = PyObject_Call(SpatterVein_type, NULL, NULL);
	
	temp = PyInt_FromLong(v.vtable);
	OBJSET(vObj, "vtable", temp);
	
	temp = PyInt_FromLong(v.address_of);
	OBJSET(vObj, "address", temp);
	
	temp = PyInt_FromLong(v.mat1);
	OBJSET(vObj, "mat1", temp);
	
	temp = PyInt_FromLong(v.unk1);
	OBJSET(vObj, "unk1", temp);
	
	temp = PyInt_FromLong(v.mat2);
	OBJSET(vObj, "mat2", temp);
	
	temp = PyInt_FromLong(v.mat3);
	OBJSET(vObj, "mat3", temp);
	
	list = PyList_New(16);
	
	for(int i = 0; i < 16; i++)
	{
		temp = PyList_New(16);
		
		for(int j = 0; j < 16; j++)
			PyList_SET_ITEM(temp, j, PyInt_FromLong(v.intensity[i][j]));
		
		PyList_SET_ITEM(list, i, temp);
	}
	
	OBJSET(vObj, "intensity", list);
	
	return vObj;
}

static PyObject* BuildVeinDict(std::vector<t_vein>& veins, std::vector<t_frozenliquidvein>& ices, std::vector<t_spattervein>& splatter)
{
	PyObject* vDict;
	PyObject* vList;
	int veinSize;
	
	vDict = PyDict_New();
	
	veinSize = veins.size();
	
	if(veinSize <= 0)
	{
		Py_INCREF(Py_None);
		DICTADD(vDict, "veins", Py_None);
	}
	else
	{
		vList = PyList_New(veinSize);
		
		for(int i = 0; i < veinSize; i++)
			PyList_SET_ITEM(vList, i, BuildVein(veins[i]));
		
		DICTADD(vDict, "veins", vList);
	}
	
	veinSize = ices.size();
	
	if(veinSize <= 0)
	{
		Py_INCREF(Py_None);
		DICTADD(vDict, "ices", Py_None);
	}
	else
	{
		vList = PyList_New(veinSize);
		
		for(int i = 0; i < veinSize; i++)
			PyList_SET_ITEM(vList, i, BuildFrozenLiquidVein(ices[i]));
		
		DICTADD(vDict, "ices", vList);
	}
	
	veinSize = splatter.size();
	
	if(veinSize <= 0)
	{
		Py_INCREF(Py_None);
		DICTADD(vDict, "spatter", Py_None);
	}
	else
	{
		vList = PyList_New(veinSize);
		
		for(int i = 0; i < veinSize; i++)
			PyList_SET_ITEM(vList, i, BuildSpatterVein(splatter[i]));
		
		DICTADD(vDict, "spatter", vList);
	}
	
	return vDict;
}

static PyObject* BuildTileTypes40d(DFHack::tiletypes40d& types)
{
	PyObject *list, *temp;
	
	list = PyList_New(16);
	
	for(int i = 0; i < 16; i++)
	{
		temp = PyList_New(16);
		
		for(int j = 0; j < 16; j++)
			PyList_SET_ITEM(temp, j, PyInt_FromLong(types[i][j]));
		
		PyList_SET_ITEM(list, i, temp);
	}
	
	return list;
}

static void ReverseBuildTileTypes40d(PyObject* list, DFHack::tiletypes40d t_types)
{
	PyObject* innerList;
	
	for(int i = 0; i < 16; i++)
	{
		innerList = PyList_GetItem(list, i);
		
		for(int j = 0; j < 16; j++)
			t_types[i][j] = (uint16_t)PyInt_AsLong(PyList_GetItem(innerList, j));
	}
}

static PyObject* BuildOccupancies40d(DFHack::occupancies40d& occ)
{
	PyObject *list, *temp, *args;
	
	list = PyList_New(16);
	
	for(int i = 0; i < 16; i++)
	{
		temp = PyList_New(16);
		
		for(int j = 0; j < 16; j++)
		{
			args = Py_BuildValue("(I)", occ[i][j].whole);
			
			PyList_SET_ITEM(temp, j, PyObject_Call(OccupancyFlags_type, args, NULL));
			
			Py_DECREF(args);
		}
		
		PyList_SET_ITEM(list, i, temp);
	}
	
	return list;
}

static void ReverseBuildOccupancies40d(PyObject* list, DFHack::occupancies40d occ)
{
	PyObject* innerList;
	
	for(int i = 0; i < 16; i++)
	{
		innerList = PyList_GetItem(list, i);
		
		for(int j = 0; j < 16; j++)
			occ[i][j].whole = (uint32_t)PyInt_AsLong(PyList_GetItem(innerList, j));
	}
}

static PyObject* BuildDesignations40d(DFHack::designations40d& des)
{
	PyObject *list, *temp, *args;
	
	list = PyList_New(16);
	
	for(int i = 0; i < 16; i++)
	{
		temp = PyList_New(16);
		
		for(int j = 0; j < 16; j++)
		{
			args = Py_BuildValue("(I)", des[i][j].whole);
			
			PyList_SET_ITEM(temp, j, PyObject_Call(DesignationFlags_type, args, NULL));
			
			Py_DECREF(args);
		}
		
		PyList_SET_ITEM(list, i, temp);
	}
	
	return list;
}

static void ReverseBuildDesignations40d(PyObject* list, DFHack::designations40d& des)
{
	PyObject* innerList;
	
	for(int i = 0; i < 16; i++)
	{
		innerList = PyList_GET_ITEM(list, i);
		
		for(int j = 0; j < 16; j++)
			des[i][j].whole = (uint32_t)PyInt_AS_LONG(PyList_GET_ITEM(innerList, j));
	}
}

static PyObject* BuildBiomeIndices40d(DFHack::biome_indices40d& idx)
{
	PyObject *list;
	
	list = PyList_New(16);
	
	for(int i = 0; i < 16; i++)
		PyList_SET_ITEM(list, i, PyInt_FromLong(idx[i]));
	
	return list;
}

static void ReverseBuildBiomeIndices40d(PyObject* list, DFHack::biome_indices40d bio)
{
	for(int i = 0; i < 16; i++)
		bio[i] = (uint8_t)PyInt_AsLong(PyList_GetItem(list, i));
}

static PyObject* BuildMapBlock40d(DFHack::mapblock40d& block)
{
	PyObject *b_Obj;
	PyObject *temp, *args;
	
	b_Obj = PyObject_Call(MapBlock40d_type, NULL, NULL);
	
	temp = BuildTileTypes40d(block.tiletypes);
	OBJSET(b_Obj, "tiletypes", temp);
	
	temp = BuildDesignations40d(block.designation);
	OBJSET(b_Obj, "designation", temp);
	
	temp = BuildOccupancies40d(block.occupancy);
	OBJSET(b_Obj, "occupancy", temp);
	
	temp = BuildBiomeIndices40d(block.biome_indices);
	OBJSET(b_Obj, "biome_indices", temp);
	
	temp = PyInt_FromLong(block.origin);
	OBJSET(b_Obj, "origin", temp);
	
	args = Py_BuildValue("(I)", block.blockflags.whole);
	temp = PyObject_CallObject(BlockFlags_type, args);
	
	temp = PyInt_FromLong(block.blockflags.whole);
	OBJSET(b_Obj, "blockflags", temp);
	
	return b_Obj;
}

struct DF_Map
{
	PyObject_HEAD
	DFHack::Maps* m_Ptr;
};

static PyObject* DF_Map_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	DF_Map* self;
	
	self = (DF_Map*)type->tp_alloc(type, 0);
	
	if(self != NULL)
		self->m_Ptr = NULL;
	
	return (PyObject*)self;
}

static int DF_Map_init(DF_Map* self, PyObject* args, PyObject* kwds)
{
	return 0;
}

static void DF_Map_dealloc(DF_Map* self)
{
	PySys_WriteStdout("map dealloc\n");
	
	if(self != NULL)
	{
		PySys_WriteStdout("map not NULL\n");
		
		if(self->m_Ptr != NULL)
		{
			PySys_WriteStdout("m_Ptr = %i\n", (int)self->m_Ptr);
			
			delete self->m_Ptr;
			
			PySys_WriteStdout("m_Ptr deleted\n");
			
			self->m_Ptr = NULL;
		}
		
		self->ob_type->tp_free((PyObject*)self);
	}
	
	PySys_WriteStdout("map dealloc done\n");
}

// Type methods

static PyObject* DF_Map_Start(DF_Map* self, PyObject* args)
{
	if(self->m_Ptr != NULL)
	{
		if(self->m_Ptr->Start())
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_Finish(DF_Map* self, PyObject* args)
{
	if(self->m_Ptr != NULL)
	{
		if(self->m_Ptr->Finish())
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_ReadGeology(DF_Map* self, PyObject* args)
{
	PyObject *list, *temp;
	int outerSize, innerSize;
	
	if(self->m_Ptr != NULL)
	{
		std::vector< std::vector<uint16_t> > geoVec;
		
		if(self->m_Ptr->ReadGeology(geoVec))
		{
			outerSize = geoVec.size();
			
			list = PyList_New(outerSize);
			
			for(int i = 0; i < outerSize; i++)
			{
				std::vector<uint16_t> innerVec = geoVec[i];
				
				innerSize = innerVec.size();
				
				temp = PyList_New(innerSize);
				
				for(int j = 0; j < innerSize; j++)
					PyList_SET_ITEM(temp, j, PyInt_FromLong(innerVec[i]));
				
				PyList_SET_ITEM(list, i, temp);
			}
			
			return list;
		}
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_IsValidBlock(DF_Map* self, PyObject* args)
{
	uint32_t x, y, z;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "III", &x, &y, &z))
			return NULL;
		
		if(self->m_Ptr->isValidBlock(x, y, z))
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_GetBlockPtr(DF_Map* self, PyObject* args)
{
	uint32_t x, y, z;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "III", &x, &y, &z))
			return NULL;
		
		return PyInt_FromLong(self->m_Ptr->getBlockPtr(x, y, z));
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_ReadBlock40d(DF_Map* self, PyObject* args)
{
	uint32_t x, y, z;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "III", &x, &y, &z))
			return NULL;
		
		mapblock40d mapblock;
		
		if(self->m_Ptr->ReadBlock40d(x, y, z, &mapblock))
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_ReadTileTypes(DF_Map* self, PyObject* args)
{
	uint32_t x, y, z;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "III", &x, &y, &z))
			return NULL;
		
		tiletypes40d t_types;
		
		if(self->m_Ptr->ReadTileTypes(x, y, z, &t_types))
			return BuildTileTypes40d(t_types);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_WriteTileTypes(DF_Map* self, PyObject* args)
{
	PyObject* tileList;
	uint32_t x, y, z;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "IIIO", &x, &y, &z, &tileList))
			return NULL;
		
		tiletypes40d t_types;
		
		ReverseBuildTileTypes40d(tileList, t_types);
		
		if(self->m_Ptr->WriteTileTypes(x, y, z, &t_types))
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_ReadDesignations(DF_Map* self, PyObject* args)
{
	uint32_t x, y, z;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "III", &x, &y, &z))
			return NULL;
		
		DFHack::designations40d des;
		
		if(self->m_Ptr->ReadDesignations(x, y, z, &des))
			return BuildDesignations40d(des);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_WriteDesignations(DF_Map* self, PyObject* args)
{
	PyObject* desList;
	uint32_t x, y, z;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "IIIO", &x, &y, &z, &desList))
			return NULL;
		
		DFHack::designations40d writeDes;
		
		ReverseBuildDesignations40d(desList, writeDes);
		
		if(self->m_Ptr->WriteDesignations(x, y, z, &writeDes))
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_ReadOccupancy(DF_Map* self, PyObject* args)
{
	uint32_t x, y, z;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "III", &x, &y, &z))
			return NULL;
		
		occupancies40d occ;
		
		if(self->m_Ptr->ReadOccupancy(x, y, z, &occ))
			return BuildOccupancies40d(occ);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_WriteOccupancy(DF_Map* self, PyObject* args)
{
	PyObject* occList;
	uint32_t x, y, z;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "IIIO", &x, &y, &z, &occList))
			return NULL;
		
		occupancies40d occ;
		
		ReverseBuildOccupancies40d(occList, occ);
		
		if(self->m_Ptr->WriteOccupancy(x, y, z, &occ))
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_ReadDirtyBit(DF_Map* self, PyObject* args)
{
	uint32_t x, y, z;
	bool bit = false;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "III", &x, &y, &z))
			return NULL;
		
		self->m_Ptr->ReadDirtyBit(x, y, z, bit);
		
		if(bit)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_WriteDirtyBit(DF_Map* self, PyObject* args)
{
	uint32_t x, y, z, b;
	bool bit;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "IIIi", &x, &y, &z, &b))
			return NULL;
		
		if(b != 0)
			bit = true;
		else
			bit = false;
		
		if(self->m_Ptr->WriteDirtyBit(x, y, z, bit))
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_ReadBlockFlags(DF_Map* self, PyObject* args)
{
	uint32_t x, y, z;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "III", &x, &y, &z))
			return NULL;
		
		t_blockflags flags;
		
		if(self->m_Ptr->ReadBlockFlags(x, y, z, flags))
			return PyInt_FromLong(flags.whole);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_WriteBlockFlags(DF_Map* self, PyObject* args)
{
	uint32_t x, y, z, whole;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "IIII", &x, &y, &z, &whole))
			return NULL;
		
		t_blockflags blockFlags;
		
		blockFlags.whole = whole;
		
		if(self->m_Ptr->WriteBlockFlags(x, y, z, blockFlags))
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_ReadRegionOffsets(DF_Map* self, PyObject* args)
{
	uint32_t x, y, z;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "III", &x, &y, &z))
			return NULL;
		
		biome_indices40d biome;
		
		if(self->m_Ptr->ReadRegionOffsets(x, y, z, &biome))
			return BuildBiomeIndices40d(biome);
	}
	
	Py_RETURN_NONE;
}

static PyObject* DF_Map_ReadVeins(DF_Map* self, PyObject* args, PyObject* kwds)
{
	uint32_t x, y, z;
	
	if(self->m_Ptr != NULL)
	{
		if(!PyArg_ParseTuple(args, "III", &x, &y, &z))
			return NULL;
		
		std::vector<t_vein> veins;
		std::vector<t_frozenliquidvein> ices;
		std::vector<t_spattervein> splatter;
		
		if(self->m_Ptr->ReadVeins(x, y, z, &veins, &ices, &splatter))
		{
			return BuildVeinDict(veins, ices, splatter);
		}
	}
	
	Py_RETURN_NONE;
}

static PyMethodDef DF_Map_methods[] =
{
	{"Start", (PyCFunction)DF_Map_Start, METH_NOARGS, ""},
	{"Finish", (PyCFunction)DF_Map_Finish, METH_NOARGS, ""},
	{"Read_Geology", (PyCFunction)DF_Map_ReadGeology, METH_NOARGS, ""},
	{"Is_Valid_Block", (PyCFunction)DF_Map_IsValidBlock, METH_VARARGS, ""},
	{"Read_Block_40d", (PyCFunction)DF_Map_ReadBlock40d, METH_VARARGS, ""},
	{"Read_Tile_Types", (PyCFunction)DF_Map_ReadTileTypes, METH_VARARGS, ""},
	{"Write_Tile_Types", (PyCFunction)DF_Map_WriteTileTypes, METH_VARARGS, ""},
	{"Read_Designations", (PyCFunction)DF_Map_ReadDesignations, METH_VARARGS, ""},
	{"Write_Designations", (PyCFunction)DF_Map_WriteDesignations, METH_VARARGS, ""},
	{"Read_Occupancy", (PyCFunction)DF_Map_ReadOccupancy, METH_VARARGS, ""},
	{"Write_Occupancy", (PyCFunction)DF_Map_WriteOccupancy, METH_VARARGS, ""},
	{"Read_Dirty_Bit", (PyCFunction)DF_Map_ReadDirtyBit, METH_VARARGS, ""},
	{"Write_Dirty_Bit", (PyCFunction)DF_Map_WriteDirtyBit, METH_VARARGS, ""},
	{"Read_Block_Flags", (PyCFunction)DF_Map_ReadBlockFlags, METH_VARARGS, ""},
	{"Write_Block_Flags", (PyCFunction)DF_Map_WriteBlockFlags, METH_VARARGS, ""},
	{"Read_Region_Offsets", (PyCFunction)DF_Map_ReadRegionOffsets, METH_VARARGS, ""},
	{NULL}	//Sentinel
};

// Getter/Setter

static PyObject* DF_Map_getSize(DF_Map* self, void* closure)
{
	uint32_t x, y, z;
	
	if(self->m_Ptr != NULL)
	{
		self->m_Ptr->getSize(x, y, z);
		
		return Py_BuildValue("III", x, y, z);
	}
	
	Py_RETURN_NONE;
}

static PyGetSetDef DF_Map_getterSetters[] =
{
    {"size", (getter)DF_Map_getSize, NULL, "size", NULL},
    {NULL}  // Sentinel
};

static PyTypeObject DF_Map_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pydfhack._MapManager",             /*tp_name*/
    sizeof(DF_Map), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DF_Map_dealloc,                         /*tp_dealloc*/
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
    "pydfhack MapManager object",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    DF_Map_methods,             /* tp_methods */
    0,                      /* tp_members */
    DF_Map_getterSetters,      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)DF_Map_init,      /* tp_init */
    0,                         /* tp_alloc */
    DF_Map_new,                 /* tp_new */
};

#endif