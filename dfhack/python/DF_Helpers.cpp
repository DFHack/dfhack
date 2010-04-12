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
//#include "DF_Imports.cpp"
#include "DFTypes.h"

using namespace DFHack;

#include "modules/Creatures.h"

#define DICTADD(d, name, item) PyDict_SetItemString(d, name, item); Py_DECREF(item)

static PyObject* BuildMatglossPair(DFHack::t_matglossPair& matgloss)
{
	return Py_BuildValue("ii", matgloss.type, matgloss.index);
}

// static PyObject* BuildTreeDesc(DFHack::t_tree_desc& tree)
// {
	// return Py_BuildValue("OO", BuildMatglossPair(tree.material), Py_BuildValue("III", tree.x, tree.y, tree.z));
// }

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

//PyDict_SetItem and PyDict_SetItemString don't steal references, so this had to get a bit more complicated...
static PyObject* BuildNote(DFHack::t_note& note)
{
	PyObject* noteDict = PyDict_New();
	PyObject* temp;
	
	temp = PyString_FromFormat("%c", note.symbol);
	DICTADD(noteDict, "symbol", temp);
	
	temp = Py_BuildValue("II", note.foreground, note.background);
	DICTADD(noteDict, "fore_back", temp);
	
	if(note.name[0])
		temp = PyString_FromString(note.name);
	else
		PyString_FromString("");
	
	DICTADD(noteDict, "name", temp);
	
	temp = Py_BuildValue("III", note.x, note.y, note.z);
	DICTADD(noteDict, "position", temp);
	
	return noteDict;
}

//same here...reference counting is kind of a pain, assuming I'm even doing it right...
static PyObject* BuildName(DFHack::t_name& name)
{
	PyObject* nameDict;
	PyObject *wordList, *speechList;
	PyObject* temp;
	int wordCount = 7;
	
	nameDict = PyDict_New();
	
	if(name.first_name[0])
		temp = PyString_FromString(name.first_name);
	else
		temp = PyString_FromString("");
	
	DICTADD(nameDict, "first_name", temp);
	
	if(name.nickname[0])
		temp = PyString_FromString(name.nickname);
	else
		temp = PyString_FromString("");
	
	DICTADD(nameDict, "nickname", temp);
	
	temp = PyInt_FromLong(name.language);
	DICTADD(nameDict, "language", temp);
	
	temp = PyBool_FromLong((int)name.has_name);
	DICTADD(nameDict, "has_name", temp);
	
	wordList = PyList_New(wordCount);
	speechList = PyList_New(wordCount);
	
	for(int i = 0; i < wordCount; i++)
	{
		PyList_SET_ITEM(wordList, i, PyInt_FromLong(name.words[i]));
		PyList_SET_ITEM(speechList, i, PyInt_FromLong(name.parts_of_speech[i]));
	}
	
	DICTADD(nameDict, "words", wordList);
	DICTADD(nameDict, "parts_of_speech", speechList);
	
	return nameDict;
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