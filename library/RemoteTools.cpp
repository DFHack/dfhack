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

#include <cstdio>
#include <cstdlib>
#include <sstream>

#include <memory>

using namespace DFHack;

using dfproto::CoreTextNotification;
using dfproto::CoreTextFragment;
using google::protobuf::MessageLite;

void DFHack::strVectorToRepeatedField(RepeatedPtrField<std::string> *pf,
                                      const std::vector<std::string> &vec)
{
    for (size_t i = 0; i < vec.size(); ++i)
        *pf->Add() = vec[i];
}

CoreService::CoreService() {
    suspend_depth = 0;

    // These 2 methods must be first, so that they get id 0 and 1
    addMethod("BindMethod", &CoreService::BindMethod);
    addMethod("RunCommand", &CoreService::RunCommand);

    // Add others here:
    addMethod("CoreSuspend", &CoreService::CoreSuspend);
    addMethod("CoreResume", &CoreService::CoreResume);
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
