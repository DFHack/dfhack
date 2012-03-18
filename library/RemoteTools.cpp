/*
https://github.com/peterix/dfhack
Copyright (c) 2011 Petr Mrázek <peterix@gmail.com>

A thread-safe logging console with a line editor for windows.

Based on linenoise win32 port,
copyright 2010, Jon Griffiths <jon_p_griffiths at yahoo dot com>.
All rights reserved.
Based on linenoise, copyright 2010, Salvatore Sanfilippo <antirez at gmail dot com>.
The original linenoise can be found at: http://github.com/antirez/linenoise

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of Redis nor the names of its contributors may be used
    to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <istream>
#include <string>
#include <stdint.h>

#include "RemoteTools.h"
#include "PluginManager.h"
#include "MiscUtils.h"

#include "modules/Materials.h"
#include "modules/Translation.h"
#include "modules/Units.h"

#include "DataDefs.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/unit_soul.h"
#include "df/unit_skill.h"
#include "df/material.h"
#include "df/matter_state.h"
#include "df/inorganic_raw.h"
#include "df/creature_raw.h"
#include "df/plant_raw.h"
#include "df/historical_figure.h"
#include "df/historical_entity.h"
#include "df/squad.h"
#include "df/squad_position.h"

#include "BasicApi.pb.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

#include <memory>

using namespace DFHack;
using namespace df::enums;
using namespace dfproto;

using google::protobuf::MessageLite;

void DFHack::strVectorToRepeatedField(RepeatedPtrField<std::string> *pf,
                                      const std::vector<std::string> &vec)
{
    for (size_t i = 0; i < vec.size(); ++i)
        *pf->Add() = vec[i];
}

void DFHack::describeEnum(RepeatedPtrField<EnumItemName> *pf, int base,
                          int size, const char* const *names)
{
    for (int i = 0; i < size; i++)
    {
        auto item = pf->Add();
        item->set_value(base+i);
        const char *key = names[i];
        if (key)
            item->set_name(key);
    }
}

void DFHack::describeBitfield(RepeatedPtrField<EnumItemName> *pf,
                              int size, const bitfield_item_info *items)
{
    for (int i = 0; i < size; i++)
    {
        auto item = pf->Add();
        item->set_value(i);
        const char *key = items[i].name;
        if (key)
            item->set_name(key);
        if (items[i].size > 1)
        {
            item->set_bit_size(items[i].size);
            i += items[i].size-1;
        }
    }
}

void DFHack::describeMaterial(BasicMaterialInfo *info, df::material *mat,
                              const BasicMaterialInfoMask *mask)
{
    info->set_token(mat->id);

    if (mask && mask->flags())
        flagarray_to_ints(info->mutable_flags(), mat->flags);

    if (!mat->prefix.empty())
        info->set_name_prefix(mat->prefix);

    if (!mask || mask->states_size() == 0)
    {
        df::matter_state state = matter_state::Solid;
        int temp = (mask && mask->has_temperature()) ? mask->temperature() : 10015;

        if (temp >= mat->heat.melting_point)
            state = matter_state::Liquid;
        if (temp >= mat->heat.boiling_point)
            state = matter_state::Gas;

        info->add_state_color(mat->state_color[state]);
        info->add_state_name(mat->state_name[state]);
        info->add_state_adj(mat->state_adj[state]);
    }
    else
    {
        for (int i = 0; i < mask->states_size(); i++)
        {
            info->add_state_color(mat->state_color[i]);
            info->add_state_name(mat->state_name[i]);
            info->add_state_adj(mat->state_adj[i]);
        }
    }

    if (mask && mask->reaction())
    {
        for (size_t i = 0; i < mat->reaction_class.size(); i++)
            info->add_reaction_class(*mat->reaction_class[i]);

        for (size_t i = 0; i < mat->reaction_product.id.size(); i++)
        {
            auto ptr = info->add_reaction_product();
            ptr->set_id(*mat->reaction_product.id[i]);
            ptr->set_type(mat->reaction_product.material.mat_type[i]);
            ptr->set_index(mat->reaction_product.material.mat_index[i]);
        }
    }
}

void DFHack::describeMaterial(BasicMaterialInfo *info, const MaterialInfo &mat,
                              const BasicMaterialInfoMask *mask)
{
    assert(mat.isValid());

    info->set_type(mat.type);
    info->set_index(mat.index);

    describeMaterial(info, mat.material, mask);

    switch (mat.mode) {
    case MaterialInfo::Inorganic:
        info->set_token(mat.inorganic->id);
        if (mask && mask->flags())
            flagarray_to_ints(info->mutable_inorganic_flags(), mat.inorganic->flags);
        break;

    case MaterialInfo::Creature:
        info->set_subtype(mat.subtype);
        if (mat.figure)
        {
            info->set_histfig_id(mat.index);
            info->set_creature_id(mat.figure->race);
        }
        else
            info->set_creature_id(mat.index);
        break;

    case MaterialInfo::Plant:
        info->set_plant_id(mat.index);
        break;
    }
}

void DFHack::describeName(NameInfo *info, df::language_name *name)
{
    if (!name->first_name.empty())
        info->set_first_name(name->first_name);
    if (!name->nickname.empty())
        info->set_nickname(name->nickname);

    if (name->language >= 0)
        info->set_language_id(name->language);

    std::string lname = Translation::TranslateName(name, false, true);
    if (!lname.empty())
        info->set_last_name(lname);

    lname = Translation::TranslateName(name, true, true);
    if (!lname.empty())
        info->set_english_name(lname);
}

void DFHack::describeUnit(BasicUnitInfo *info, df::unit *unit,
                          const BasicUnitInfoMask *mask)
{
    info->set_unit_id(unit->id);

    info->set_pos_x(unit->pos.x);
    info->set_pos_y(unit->pos.y);
    info->set_pos_z(unit->pos.z);

    auto name = Units::GetVisibleName(unit);
    if (name->has_name)
        describeName(info->mutable_name(), name);

    info->set_flags1(unit->flags1.whole);
    info->set_flags2(unit->flags2.whole);
    info->set_flags3(unit->flags3.whole);

    info->set_race(unit->race);
    info->set_caste(unit->caste);

    if (unit->sex >= 0)
        info->set_gender(unit->sex);
    if (unit->civ_id >= 0)
        info->set_civ_id(unit->civ_id);
    if (unit->hist_figure_id >= 0)
        info->set_histfig_id(unit->hist_figure_id);

    if (mask && mask->labors())
    {
        for (int i = 0; i < sizeof(unit->status.labors)/sizeof(bool); i++)
            if (unit->status.labors[i])
                info->add_labors(i);
    }

    if (mask && mask->skills() && unit->status.current_soul)
    {
        auto &vec = unit->status.current_soul->skills;

        for (size_t i = 0; i < vec.size(); i++)
        {
            auto skill = vec[i];
            auto item = info->add_skills();
            item->set_id(skill->id);
            item->set_level(skill->rating);
            item->set_experience(skill->experience);
        }
    }
}

static command_result ListEnums(color_ostream &stream,
                                const EmptyMessage *, ListEnumsOut *out)
{
#define ENUM(name) describe_enum<df::name>(out->mutable_##name());
#define BITFIELD(name) describe_bitfield<df::name>(out->mutable_##name());

    ENUM(material_flags);
    ENUM(inorganic_flags);

    BITFIELD(unit_flags1);
    BITFIELD(unit_flags2);
    BITFIELD(unit_flags3);

    ENUM(unit_labor);
    ENUM(job_skill);

#undef ENUM
#undef BITFIELD
}

static void listMaterial(ListMaterialsOut *out, int type, int index, const BasicMaterialInfoMask *mask)
{
    MaterialInfo info(type, index);
    if (info.isValid())
        describeMaterial(out->add_value(), info, mask);
}

static command_result ListMaterials(color_ostream &stream,
                                    const ListMaterialsIn *in, ListMaterialsOut *out)
{
    auto mask = in->has_mask() ? &in->mask() : NULL;

    for (int i = 0; i < in->id_list_size(); i++)
    {
        auto &elt = in->id_list(i);
        listMaterial(out, elt.type(), elt.index(), mask);
    }

    if (in->builtin())
    {
        for (int i = 0; i < MaterialInfo::NUM_BUILTIN; i++)
            listMaterial(out, i, -1, mask);
    }

    if (in->inorganic())
    {
        auto &vec = df::inorganic_raw::get_vector();
        for (size_t i = 0; i < vec.size(); i++)
            listMaterial(out, 0, i, mask);
    }

    if (in->creatures())
    {
        auto &vec = df::creature_raw::get_vector();
        for (size_t i = 0; i < vec.size(); i++)
        {
            auto praw = vec[i];

            for (size_t j = 0; j < praw->material.size(); j++)
                listMaterial(out, MaterialInfo::CREATURE_BASE+j, i, mask);
        }
    }

    if (in->plants())
    {
        auto &vec = df::plant_raw::get_vector();
        for (size_t i = 0; i < vec.size(); i++)
        {
            auto praw = vec[i];

            for (size_t j = 0; j < praw->material.size(); j++)
                listMaterial(out, MaterialInfo::PLANT_BASE+j, i, mask);
        }
    }

    return out->value_size() ? CR_OK : CR_NOT_FOUND;
}

static command_result ListUnits(color_ostream &stream,
                                const ListUnitsIn *in, ListUnitsOut *out)
{
    auto mask = in->has_mask() ? &in->mask() : NULL;

    if (in->id_list_size() > 0)
    {
        for (int i = 0; i < in->id_list_size(); i++)
        {
            auto unit = df::unit::find(in->id_list(i));
            if (unit)
                describeUnit(out->add_value(), unit, mask);
        }
    }
    else
    {
        auto &vec = df::unit::get_vector();

        for (size_t i = 0; i < vec.size(); i++)
        {
            auto unit = vec[i];

            if (in->has_race() && unit->race != in->race())
                continue;
            if (in->civ_id() && unit->civ_id != in->civ_id())
                continue;

            describeUnit(out->add_value(), unit, mask);
        }
    }

    return out->value_size() ? CR_OK : CR_NOT_FOUND;
}

static command_result ListSquads(color_ostream &stream,
                                 const ListSquadsIn *in, ListSquadsOut *out)
{
    auto entity = df::historical_entity::find(df::global::ui->group_id);
    if (!entity)
        return CR_NOT_FOUND;

    for (size_t i = 0; i < entity->squads.size(); i++)
    {
        auto squad = df::squad::find(entity->squads[i]);
        if (!squad)
            continue;

        auto item = out->add_value();
        item->set_squad_id(squad->id);

        if (squad->name.has_name)
            describeName(item->mutable_name(), &squad->name);
        if (!squad->alias.empty())
            item->set_alias(squad->alias);

        for (size_t j = 0; j < squad->positions.size(); j++)
            item->add_members(squad->positions[j]->occupant);
    }

    return CR_OK;
}

CoreService::CoreService() {
    suspend_depth = 0;

    // These 2 methods must be first, so that they get id 0 and 1
    addMethod("BindMethod", &CoreService::BindMethod, SF_DONT_SUSPEND);
    addMethod("RunCommand", &CoreService::RunCommand, SF_DONT_SUSPEND);

    // Add others here:
    addMethod("CoreSuspend", &CoreService::CoreSuspend, SF_DONT_SUSPEND);
    addMethod("CoreResume", &CoreService::CoreResume, SF_DONT_SUSPEND);

    // Functions:
    addFunction("ListEnums", ListEnums, SF_CALLED_ONCE | SF_DONT_SUSPEND);
    addFunction("ListMaterials", ListMaterials, SF_CALLED_ONCE);
    addFunction("ListUnits", ListUnits);
    addFunction("ListSquads", ListSquads);
}

CoreService::~CoreService()
{
    while (suspend_depth-- > 0)
        Core::getInstance().Resume();
}

command_result CoreService::BindMethod(color_ostream &stream,
                                       const dfproto::CoreBindRequest *in,
                                       dfproto::CoreBindReply *out)
{
    ServerFunctionBase *fn = connection()->findFunction(stream, in->plugin(), in->method());

    if (!fn)
    {
        stream.printerr("RPC method not found: %s::%s\n",
                        in->plugin().c_str(), in->method().c_str());
        return CR_FAILURE;
    }

    if (fn->p_in_template->GetTypeName() != in->input_msg() ||
        fn->p_out_template->GetTypeName() != in->output_msg())
    {
        stream.printerr("Requested wrong signature for RPC method: %s::%s\n",
                        in->plugin().c_str(), in->method().c_str());
        return CR_FAILURE;
    }

    out->set_assigned_id(fn->getId());
    return CR_OK;
}

command_result CoreService::RunCommand(color_ostream &stream,
                                       const dfproto::CoreRunCommandRequest *in)
{
    std::string cmd = in->command();
    std::vector<std::string> args;
    for (int i = 0; i < in->arguments_size(); i++)
        args.push_back(in->arguments(i));

    return Core::getInstance().plug_mgr->InvokeCommand(stream, cmd, args);
}

command_result CoreService::CoreSuspend(color_ostream &stream, const EmptyMessage*, IntMessage *cnt)
{
    Core::getInstance().Suspend();
    cnt->set_value(++suspend_depth);
    return CR_OK;
}

command_result CoreService::CoreResume(color_ostream &stream, const EmptyMessage*, IntMessage *cnt)
{
    if (suspend_depth <= 0)
        return CR_WRONG_USAGE;

    Core::getInstance().Resume();
    cnt->set_value(--suspend_depth);
    return CR_OK;
}
