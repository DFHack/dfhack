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

#include "RemoteServer.h"
#include "PluginManager.h"
#include "MiscUtils.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

#include <memory>

using namespace DFHack;

#include "tinythread.h"
using namespace tthread;

using dfproto::CoreTextNotification;
using dfproto::CoreTextFragment;
using google::protobuf::MessageLite;

CoreService::CoreService() {
    // This must be the first method, so that it gets id 0
    addMethod("BindMethod", &CoreService::BindMethod);

    addMethod("RunCommand", &CoreService::RunCommand);
}

command_result CoreService::BindMethod(color_ostream &stream,
                                       const dfproto::CoreBindRequest *in,
                                       dfproto::CoreBindReply *out)
{
    ServerFunctionBase *fn = connection()->findFunction(in->plugin(), in->method());

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
                                       const dfproto::CoreRunCommandRequest *in,
                                       CoreVoidReply*)
{
    std::string cmd = in->command();
    std::vector<std::string> args;
    for (int i = 0; i < in->arguments_size(); i++)
        args.push_back(in->arguments(i));

    return Core::getInstance().plug_mgr->InvokeCommand(stream, cmd, args, false);
}

RPCService::RPCService()
{
    owner = NULL;
    holder = NULL;
}

RPCService::~RPCService()
{
    for (size_t i = 0; i < functions.size(); i++)
        delete functions[i];
}

ServerFunctionBase *RPCService::getFunction(const std::string &name)
{
    assert(owner);
    return lookup[name];
}

void RPCService::finalize(ServerConnection *owner, std::vector<ServerFunctionBase*> *ftable)
{
    this->owner = owner;

    for (size_t i = 0; i < functions.size(); i++)
    {
        auto fn = functions[i];

        fn->id = (int16_t)ftable->size();
        ftable->push_back(fn);

        lookup[fn->name] = fn;
    }
}

ServerConnection::ServerConnection(CActiveSocket *socket)
    : socket(socket), stream(this)
{
    in_error = false;

    core_service = new CoreService();
    core_service->finalize(this, &functions);

    thread = new tthread::thread(threadFn, (void*)this);
}

ServerConnection::~ServerConnection()
{
    in_error = true;
    socket->Close();
    delete socket;

    for (auto it = plugin_services.begin(); it != plugin_services.end(); ++it)
        delete it->second;

    delete core_service;
}

ServerFunctionBase *ServerConnection::findFunction(const std::string &plugin, const std::string &name)
{
    if (plugin.empty())
        return core_service->getFunction(name);
    else
        return NULL; // todo: add plugin api support
}

void ServerConnection::connection_ostream::flush_proxy()
{
    if (owner->in_error)
    {
        buffer.clear();
        return;
    }

    if (buffer.empty())
        return;

    CoreTextNotification msg;

    for (auto it = buffer.begin(); it != buffer.end(); ++it)
    {
        auto frag = msg.add_fragments();
        frag->set_text(it->second);
        if (it->first >= 0)
            frag->set_color(CoreTextFragment::Color(it->first));
    }

    buffer.clear();

    if (!sendRemoteMessage(*owner->socket, RPC_REPLY_TEXT, &msg))
    {
        owner->in_error = true;
        Core::printerr("Error writing text into client socket.\n");
    }
}

void ServerConnection::threadFn(void *arg)
{
    ServerConnection *me = (ServerConnection*)arg;
    color_ostream_proxy out(Core::getInstance().getConsole());

    /* Handshake */

    {
        RPCHandshakeHeader header;

        if (!readFullBuffer(*me->socket, &header, sizeof(header)))
        {
            out << "In RPC server: could not read handshake header." << endl;
            delete me;
            return;
        }

        if (memcmp(header.magic, RPCHandshakeHeader::REQUEST_MAGIC, sizeof(header.magic)) ||
            header.version != 1)
        {
            out << "In RPC server: invalid handshake header." << endl;
            delete me;
            return;
        }

        memcpy(header.magic, RPCHandshakeHeader::RESPONSE_MAGIC, sizeof(header.magic));
        header.version = 1;

        if (me->socket->Send((uint8*)&header, sizeof(header)) != sizeof(header))
        {
            out << "In RPC server: could not send handshake response." << endl;
            delete me;
            return;
        }
    }

    /* Processing */

    std::cerr << "Client connection established." << endl;

    while (!me->in_error) {
        RPCMessageHeader header;

        if (!readFullBuffer(*me->socket, &header, sizeof(header)))
        {
            out.printerr("In RPC server: I/O error in receive header.\n");
            break;
        }

        if (header.id == RPC_REQUEST_QUIT)
            break;

        if (header.size < 0 || header.size > 2*1048576)
        {
            out.printerr("In RPC server: invalid received size %d.\n", header.size);
            break;
        }

        std::auto_ptr<uint8_t> buf(new uint8_t[header.size]);

        if (!readFullBuffer(*me->socket, buf.get(), header.size))
        {
            out.printerr("In RPC server: I/O error in receive %d bytes of data.\n", header.size);
            break;
        }

        //out.print("Handling %d:%d\n", header.id, header.size);

        ServerFunctionBase *fn = vector_get(me->functions, header.id);
        MessageLite *reply = NULL;
        command_result res = CR_FAILURE;

        if (!fn)
        {
            me->stream.printerr("RPC call of invalid id %d\n", header.id);
        }
        else
        {
            if (!fn->in()->ParseFromArray(buf.get(), header.size))
            {
                me->stream.printerr("In call to %s: could not decode input args.\n", fn->name);
            }
            else
            {
                reply = fn->out();
                res = fn->execute(me->stream);
            }
        }

        if (me->in_error)
            break;

        me->stream.flush();

        //out.print("Answer %d:%d\n", res, reply);

        if (res == CR_OK && reply)
        {
            if (!sendRemoteMessage(*me->socket, RPC_REPLY_RESULT, reply))
            {
                out.printerr("In RPC server: I/O error in send result.\n");
                break;
            }
        }
        else
        {
            header.id = RPC_REPLY_FAIL;
            header.size = res;

            if (me->socket->Send((uint8_t*)&header, sizeof(header)) != sizeof(header))
            {
                out.printerr("In RPC server: I/O error in send failure code.\n");
                break;
            }
        }
    }

    std::cerr << "Shutting down client connection." << endl;

    delete me;
}

ServerMain::ServerMain()
{
    thread = NULL;
}

ServerMain::~ServerMain()
{
    socket.Close();
}

bool ServerMain::listen(int port)
{
    if (thread)
        return true;

    socket.Initialize();

    if (!socket.Listen((const uint8 *)"127.0.0.1", port))
        return false;

    thread = new tthread::thread(threadFn, this);
    return true;
}

void ServerMain::threadFn(void *arg)
{
    ServerMain *me = (ServerMain*)arg;
    CActiveSocket *client;

    while ((client = me->socket.Accept()) != NULL)
    {
        new ServerConnection(client);
    }
}
