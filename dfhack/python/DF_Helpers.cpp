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
#include "DF_Imports.cpp"
#include "DFTypes.h"

using namespace DFHack;

static PyObject* BuildMatglossPair(DFHack::t_matglossPair& matgloss)
{
	return Py_BuildValue("ii", matgloss.type, matgloss.index);
}

static PyObject* BuildTreeDesc(DFHack::t_tree_desc& tree)
{
	return Py_BuildValue("OO", BuildMatglossPair(tree.material), Py_BuildValue("III", tree.x, tree.y, tree.z));
}

static PyObject* BuildSkill(DFHack::t_skill& skill)
{
	return Py_BuildValue("III", skill.id, skill.experience, skill.rating);
}

static PyObject* BuildJob(DFHack::t_job& job)
{
	return Py_BuildValue("Oi", PyBool_FromLong((int)job.active), job.jobId);
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
	PyObject* noteDict = PyDict_New();
	
	PyDict_SetItemString(noteDict, "symbol", PyString_FromFormat("%c", note.symbol));
	PyDict_SetItemString(noteDict, "fore_back", Py_BuildValue("II", note.foreground, note.background));
	
	if(note.name[0])
		PyDict_SetItemString(noteDict, "name", PyString_FromString(note.name));
	else
		PyDict_SetItemString(noteDict, "name", PyString_FromString(""));
		
	PyDict_SetItemString(noteDict, "position", Py_BuildValue("III", note.x, note.y, note.z));
	
	return noteDict;
}

static PyObject* BuildName(DFHack::t_name& name)
{
	PyObject* nameDict;
	PyObject *wordList, *speechList;
	
	nameDict = PyDict_New();
	
	if(name.first_name[0])
		PyDict_SetItemString(nameDict, "first_name", PyString_FromString(name.first_name));
	else
		PyDict_SetItemString(nameDict, "first_name", PyString_FromString(""));
	
	if(name.nickname[0])
		PyDict_SetItemString(nameDict, "nickname", PyString_FromString(name.nickname));
	else
		PyDict_SetItemString(nameDict, "nickname", PyString_FromString(""));
	
	PyDict_SetItemString(nameDict, "language", PyInt_FromLong(name.language));
	PyDict_SetItemString(nameDict, "has_name", PyBool_FromLong((int)name.has_name));
	
	wordList = PyList_New(7);
	speechList = PyList_New(7);
	
	for(int i = 0; i < 7; i++)
	{
		PyList_SetItem(wordList, i, PyInt_FromLong(name.words[i]));
		PyList_SetItem(wordList, i, PyInt_FromLong(name.parts_of_speech[i]));
	}
	
	PyDict_SetItemString(nameDict, "words", wordList);
	PyDict_SetItemString(nameDict, "parts_of_speech", speechList);
	
	return nameDict;
}

static PyObject* BuildSettlement(DFHack::t_settlement& settlement)
{
	PyObject* setDict;
	PyObject *local_pos1, *local_pos2;
	
	setDict = PyDict_New();
	
	PyDict_SetItemString(setDict, "origin", PyInt_FromLong(settlement.origin));
	PyDict_SetItemString(setDict, "name", BuildName(settlement.name));
	PyDict_SetItemString(setDict, "world_pos", Py_BuildValue("ii", settlement.world_x, settlement.world_y));
	
	local_pos1 = Py_BuildValue("ii", settlement.local_x1, settlement.local_y1);
	local_pos2 = Py_BuildValue("ii", settlement.local_x2, settlement.local_y2);
	
	PyDict_SetItemString(setDict, "local_pos", Py_BuildValue("OO", local_pos1, local_pos2));
	
	return setDict;
}

#endif