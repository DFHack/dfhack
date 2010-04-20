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
#include "DFTypes.h"
#include "DF_Imports.cpp"

using namespace DFHack;

#include "modules/Creatures.h"

#define DICTADD(d, name, item) PyDict_SetItemString(d, name, item); Py_DECREF(item)
#define OBJSET(o, name, item) PyObject_SetAttrString(o, name, item); Py_DECREF(item)

static PyObject* BuildMatglossPair(DFHack::t_matglossPair& matgloss)
{
	return Py_BuildValue("ii", matgloss.type, matgloss.index);
}

static DFHack::t_matglossPair ReverseBuildMatglossPair(PyObject* mObj)
{
	DFHack::t_matglossPair mPair;
	
	mPair.type = (int16_t)PyInt_AsLong(PyTuple_GetItem(mObj, 0));
	mPair.index = (int32_t)PyInt_AsLong(PyTuple_GetItem(mObj, 1));
	
	return mPair;
}

static PyObject* BuildSkill(DFHack::t_skill& skill)
{
	return Py_BuildValue("III", skill.id, skill.experience, skill.rating);
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
	return Py_BuildValue("IIIIIII", at.level, at.field_4, at.field_8, at.field_C, at.leveldiff, at.field_14, at.field_18);
}

static PyObject* BuildItemType(DFHack::t_itemType& item)
{
	PyObject *id, *name;
	
	if(item.id[0])
		id = PyString_FromString(item.id);
	else
		id = PyString_FromString("");
	
	if(item.name[0])
		name = PyString_FromString(item.name);
	else
		name = PyString_FromString("");
		
	return Py_BuildValue("OO", id, name);
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
	PyObject *args, *name, *position;
	
	if(note.name[0])
		name = PyString_FromString(note.name);
	else
		name = PyString_FromString("");
	
	position = Py_BuildValue("III", note.x, note.y, note.z);
	
	args = Py_BuildValue("cIIsO", note.symbol, note.foreground, note.background, name, position);
	
	noteObj = PyObject_CallObject(Note_type, args);
	
	return noteObj;
}

static PyObject* BuildName(DFHack::t_name& name)
{
	PyObject* nameObj;
	PyObject *wordList, *speechList;
	PyObject* temp;
	int wordCount = 7;
	
	nameObj = PyObject_CallObject(Name_type, NULL);
	
	if(name.first_name[0])
		temp = PyString_FromString(name.first_name);
	else
		temp = PyString_FromString("");
	
	OBJSET(nameObj, "first_name", temp);
	
	if(name.nickname[0])
		temp = PyString_FromString(name.nickname);
	else
		temp = PyString_FromString("");
	
	OBJSET(nameObj, "nickname", temp);
	
	temp = PyInt_FromLong(name.language);
	OBJSET(nameObj, "language", temp);
	
	temp = PyBool_FromLong((int)name.has_name);
	OBJSET(nameObj, "has_name", temp);
	
	wordList = PyList_New(wordCount);
	speechList = PyList_New(wordCount);
	
	for(int i = 0; i < wordCount; i++)
	{
		PyList_SET_ITEM(wordList, i, PyInt_FromLong(name.words[i]));
		PyList_SET_ITEM(speechList, i, PyInt_FromLong(name.parts_of_speech[i]));
	}
	
	OBJSET(nameObj, "words", wordList);
	OBJSET(nameObj, "parts_of_speech", speechList);
	
	return nameObj;
}

static DFHack::t_name ReverseBuildName(PyObject* nameObj)
{
	PyObject *temp, *listTemp;
	int boolTemp, arrLength;
	Py_ssize_t listLength, strLength;
	char* strTemp;
	DFHack::t_name name;
	
	temp = PyObject_GetAttrString(nameObj, "language");
	name.language = (uint32_t)PyInt_AsLong(temp);
	
	temp = PyObject_GetAttrString(nameObj, "has_name");
	
	boolTemp = (int)PyInt_AsLong(temp);
	
	if(boolTemp != 0)
		name.has_name = true;
	else
		name.has_name = false;
	
	//I seriously doubt the name arrays will change length, but why take chances?
	listTemp = PyObject_GetAttrString(nameObj, "words");
	
	arrLength = sizeof(name.words) / sizeof(uint32_t);
	listLength = PyList_Size(listTemp);
	
	if(listLength < arrLength)
		arrLength = listLength;
	
	for(int i = 0; i < arrLength; i++)
		name.words[i] = (uint32_t)PyInt_AsLong(PyList_GetItem(listTemp, i));
	
	listTemp = PyObject_GetAttrString(nameObj, "parts_of_speech");
	
	arrLength = sizeof(name.parts_of_speech) / sizeof(uint16_t);
	listLength = PyList_Size(listTemp);
	
	if(listLength < arrLength)
		arrLength = listLength;
	
	for(int i = 0; i < arrLength; i++)
		name.parts_of_speech[i] = (uint16_t)PyInt_AsLong(PyList_GetItem(listTemp, i));
	
	temp = PyObject_GetAttrString(nameObj, "first_name");
	strLength = PyString_Size(temp);
	strTemp = PyString_AsString(temp);
	
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
	PyObject* setDict;
	PyObject *local_pos1, *local_pos2;
	PyObject* temp;
	
	setDict = PyDict_New();
	
	temp = PyInt_FromLong(settlement.origin);
	DICTADD(setDict, "origin", temp);
	
	temp = BuildName(settlement.name);
	DICTADD(setDict, "name", temp);
	
	temp = Py_BuildValue("ii", settlement.world_x, settlement.world_y);
	DICTADD(setDict, "world_pos", temp);
	
	local_pos1 = Py_BuildValue("ii", settlement.local_x1, settlement.local_y1);
	local_pos2 = Py_BuildValue("ii", settlement.local_x2, settlement.local_y2);
	
	temp = Py_BuildValue("OO", local_pos1, local_pos2);
	DICTADD(setDict, "local_pos", temp);
	
	return setDict;
}

static PyObject* BuildSoul(DFHack::t_soul& soul)
{
	PyObject *soulDict, *skillList, *temp;
	
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
	
	return soulDict;
}

#endif