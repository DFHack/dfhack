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

#ifndef __DFHELPERS__
#define __DFHELPERS__

#include "Python.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

using namespace std;

#include "DFTypes.h"
#include "DF_Imports.cpp"

using namespace DFHack;

#include "modules/Materials.h"
#include "modules/Creatures.h"

#define DICTADD(d, name, item) PyDict_SetItemString(d, name, item); Py_DECREF(item)
#define OBJSET(o, name, item) PyObject_SetAttrString(o, name, item); Py_DECREF(item)

static PyObject* BuildTileColor(uint16_t fore, uint16_t back, uint16_t bright)
{
	PyObject *tObj, *args;
	
	args = Py_BuildValue("iii", fore, back, bright);
	
	tObj = PyObject_CallObject(TileColor_type, args);
	
	Py_DECREF(args);
	
	return tObj;
}

static PyObject* BuildPosition2D(uint16_t x, uint16_t y)
{
	PyObject *posObj, *args;
	
	args = Py_BuildValue("ii", x, y);
	
	posObj = PyObject_CallObject(Position2D_type, args);
	
	Py_DECREF(args);
	
	return posObj;
}

static PyObject* BuildPosition3D(uint16_t x, uint16_t y, uint16_t z)
{
	PyObject *posObj, *args;
	
	args = Py_BuildValue("iii", x, y, z);
	
	posObj = PyObject_CallObject(Position3D_type, args);
	
	Py_DECREF(args);
	
	return posObj;
}

static PyObject* BuildMatglossPair(DFHack::t_matglossPair& matgloss)
{
	return Py_BuildValue("ii", matgloss.type, matgloss.index);
}

static DFHack::t_matglossPair ReverseBuildMatglossPair(PyObject* mObj)
{
	DFHack::t_matglossPair mPair;
	PyObject* temp;
	
	temp = PyTuple_GetItem(mObj, 0);
	
	mPair.type = (int16_t)PyInt_AsLong(temp);
	
	Py_DECREF(temp);
	
	temp = PyTuple_GetItem(mObj, 1);
	
	mPair.index = (int32_t)PyInt_AsLong(temp);
	
	Py_DECREF(temp);
	
	return mPair;
}

static PyObject* BuildSkill(DFHack::t_skill& skill)
{
	PyObject *args, *skillObj;
	
	args = Py_BuildValue("III", skill.id, skill.experience, skill.rating);
	
	skillObj = PyObject_CallObject(Skill_type, args);
	
	Py_DECREF(args);
	
	return skillObj;
}

static PyObject* BuildSkillList(DFHack::t_skill (&skills)[256], uint8_t numSkills)
{
	PyObject* list = PyList_New(numSkills);
	
	for(int i = 0; i < numSkills; i++)
		PyList_SET_ITEM(list, i, BuildSkill(skills[i]));
	
	return list;
}

static PyObject* BuildJob(DFHack::t_job& job)
{
	return Py_BuildValue("Oi", PyBool_FromLong((int)job.active), job.jobId);
}

static PyObject* BuildAttribute(DFHack::t_attrib& at)
{
	PyObject *args, *attrObj;
	
	args = Py_BuildValue("IIIIIII", at.level, at.field_4, at.field_8, at.field_C, at.leveldiff, at.field_14, at.field_18);
	
	attrObj = PyObject_CallObject(Attribute_type, args);
	
	Py_DECREF(args);
	
	return attrObj;
}

static PyObject* BuildItemType(DFHack::t_itemType& item)
{
	return Py_BuildValue("ss", item.id, item.name);
}

static PyObject* BuildLike(DFHack::t_like& like)
{
	PyObject* item;
	
	item = Py_BuildValue("iii", like.type, like.itemClass, like.itemIndex);
	
	return Py_BuildValue("OOO", item, BuildMatglossPair(like.material), PyBool_FromLong((int)like.active));
}

static PyObject* BuildNote(DFHack::t_note& note)
{
	PyObject* noteObj;
	PyObject *args, *position;
	
	args = Py_BuildValue("III", note.x, note.y, note.z);
	
	position = PyObject_CallObject(Position3D_type, args);
	
	Py_DECREF(args);
	
	args = Py_BuildValue("cIIsO", note.symbol, note.foreground, note.background, note.name, position);
	
	noteObj = PyObject_CallObject(Note_type, args);
	
	Py_DECREF(args);
	
	return noteObj;
}

static int NAME_WORD_COUNT = 7;

static PyObject* BuildName(DFHack::t_name& name)
{
	PyObject* nameObj;
	PyObject *wordList, *speechList, *args;
	
	wordList = PyList_New(NAME_WORD_COUNT);
	speechList = PyList_New(NAME_WORD_COUNT);
	
	for(int i = 0; i < NAME_WORD_COUNT; i++)
	{
		PyList_SET_ITEM(wordList, i, PyInt_FromLong(name.words[i]));
		PyList_SET_ITEM(speechList, i, PyInt_FromLong(name.parts_of_speech[i]));
	}
	
	args = Py_BuildValue("ssiOOO", name.first_name, name.nickname, name.language, \
									PyBool_FromLong((int)name.has_name), wordList, speechList);
	
	nameObj = PyObject_CallObject(Name_type, args);
	
	Py_DECREF(args);
	
	return nameObj;
}

static DFHack::t_name ReverseBuildName(PyObject* nameObj)
{
	PyObject *temp, *listTemp;
	int boolTemp;
	Py_ssize_t strLength;
	char* strTemp;
	DFHack::t_name name;
	
	temp = PyObject_GetAttrString(nameObj, "language");
	name.language = (uint32_t)PyInt_AsLong(temp);
	
	temp = PyObject_GetAttrString(nameObj, "has_name");
	
	boolTemp = (int)PyInt_AsLong(temp);
	
	Py_DECREF(temp);
	
	if(boolTemp != 0)
		name.has_name = true;
	else
		name.has_name = false;
	
	listTemp = PyObject_GetAttrString(nameObj, "words");
	
	for(int i = 0; i < NAME_WORD_COUNT; i++)
		name.words[i] = (uint32_t)PyInt_AsLong(PyList_GetItem(listTemp, i));
	
	Py_DECREF(listTemp);
	
	listTemp = PyObject_GetAttrString(nameObj, "parts_of_speech");
	
	for(int i = 0; i < NAME_WORD_COUNT; i++)
		name.parts_of_speech[i] = (uint16_t)PyInt_AsLong(PyList_GetItem(listTemp, i));
	
	Py_DECREF(listTemp);
	
	temp = PyObject_GetAttrString(nameObj, "first_name");
	strLength = PyString_Size(temp);
	strTemp = PyString_AsString(temp);
	
	Py_DECREF(temp);
	
	if(strLength > 128)
	{
		strncpy(name.first_name, strTemp, 127);
		name.first_name[127] = '\0';
	}
	else
	{
		strncpy(name.first_name, strTemp, strLength);
		name.first_name[strLength] = '\0';
	}
	
	temp = PyObject_GetAttrString(nameObj, "nickname");
	strLength = PyString_Size(temp);
	strTemp = PyString_AsString(temp);
	
	Py_DECREF(temp);
	
	if(strLength > 128)
	{
		strncpy(name.nickname, strTemp, 127);
		name.nickname[127] = '\0';
	}
	else
	{
		strncpy(name.nickname, strTemp, strLength);
		name.nickname[strLength] = '\0';
	}
	
	return name;
}

static PyObject* BuildSettlement(DFHack::t_settlement& settlement)
{
	PyObject* setObj;
	PyObject *world_pos, *local_pos, *args;
	
	args = Py_BuildValue("ii", settlement.world_x, settlement.world_y);
	
	world_pos = PyObject_CallObject(Position2D_type, args);
	
	Py_DECREF(args);
	
	args = Py_BuildValue("iiii", settlement.local_x1, settlement.local_y1, settlement.local_x2, settlement.local_y2);
	
	local_pos = PyObject_CallObject(Rectangle_type, args);
	
	Py_DECREF(args);
	
	args = Py_BuildValue("iOOO", settlement.origin, BuildName(settlement.name), world_pos, local_pos);
	
	setObj = PyObject_CallObject(Settlement_type, args);
	
	Py_DECREF(args);
	
	return setObj;
}

static PyObject* BuildSoul(DFHack::t_soul& soul)
{
	PyObject *soulDict, *skillList, *temp, *emptyArgs;
	PyObject* soulObj;
	
	emptyArgs = Py_BuildValue("()");
	
	soulDict = PyDict_New();
	
	skillList = BuildSkillList(soul.skills, soul.numSkills);
	DICTADD(soulDict, "skills", skillList);
	
	temp = BuildAttribute(soul.analytical_ability);
	DICTADD(soulDict, "analytical_ability", temp);
	
	temp = BuildAttribute(soul.focus);
	DICTADD(soulDict, "focus", temp);
	
	temp = BuildAttribute(soul.willpower);
	DICTADD(soulDict, "willpower", temp);
	
	temp = BuildAttribute(soul.creativity);
	DICTADD(soulDict, "creativity", temp);
	
	temp = BuildAttribute(soul.intuition);
	DICTADD(soulDict, "intuition", temp);
	
	temp = BuildAttribute(soul.patience);
	DICTADD(soulDict, "patience", temp);
	
	temp = BuildAttribute(soul.memory);
	DICTADD(soulDict, "memory", temp);
	
	temp = BuildAttribute(soul.linguistic_ability);
	DICTADD(soulDict, "linguistic_ability", temp);
	
	temp = BuildAttribute(soul.spatial_sense);
	DICTADD(soulDict, "spatial_sense", temp);
	
	temp = BuildAttribute(soul.musicality);
	DICTADD(soulDict, "musicality", temp);
	
	temp = BuildAttribute(soul.kinesthetic_sense);
	DICTADD(soulDict, "kinesthetic_sense", temp);
	
	temp = BuildAttribute(soul.empathy);
	DICTADD(soulDict, "empathy", temp);
	
	temp = BuildAttribute(soul.social_awareness);	
	DICTADD(soulDict, "social_awareness", temp);
	
	soulObj = PyObject_Call(Soul_type, emptyArgs, soulDict);
	
	Py_DECREF(emptyArgs);
	
	return soulObj;
}

#endif
