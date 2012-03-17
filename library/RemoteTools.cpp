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

#include "DataDefs.h"
#include "df/material.h"
#include "df/matter_state.h"
#include "df/inorganic_raw.h"
#include "df/creature_raw.h"
#include "df/plant_raw.h"
#include "df/historical_figure.h"

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

void DFHack::describeMaterial(BasicMaterialInfo *info, df::material *mat,
                              const BasicMaterialInfoMask *mask)
{
    info->set_token(mat->id);

    if (mask && mask->flags())
        flagarray_to_string(info->mutable_flags(), mat->flags);

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
            flagarray_to_string(info->mutable_inorganic_flags(), mat.inorganic->flags);
        break;

    case MaterialInfo::Creature:
        info->set_subtype(mat.subtype);
        if (mat.figure)
        {
            info->set_hfig_id(mat.index);
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

static void listMaterial(ListMaterialsRes *out, int type, int index, const BasicMaterialInfoMask *mask)
{
    MaterialInfo info(type, index);
    if (info.isValid())
        describeMaterial(out->add_value(), info, mask);
}

static command_result ListMaterials(color_ostream &stream,
                                    const ListMaterialsRq *in, ListMaterialsRes *out)
{
    CoreSuspender suspend;

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

CoreService::CoreService() {
    suspend_depth = 0;

    // These 2 methods must be first, so that they get id 0 and 1
    addMethod("BindMethod", &CoreService::BindMethod);
    addMethod("RunCommand", &CoreService::RunCommand);

    // Add others here:
    addMethod("CoreSuspend", &CoreService::CoreSuspend);
    addMethod("CoreResume", &CoreService::CoreResume);

    addFunction("ListMaterials", ListMaterials);
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
