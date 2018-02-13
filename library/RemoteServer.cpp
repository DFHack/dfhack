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

#include "RemoteServer.h"
#include "RemoteTools.h"

#include "PassiveSocket.h"
#include "PluginManager.h"
#include "MiscUtils.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

#include <memory>

using namespace DFHack;

#include "tinythread.h"
using namespace tthread;

#include "jsoncpp.h"

using dfproto::CoreTextNotification;
using dfproto::CoreTextFragment;
using google::protobuf::MessageLite;

bool readFullBuffer(CSimpleSocket *socket, void *buf, int size);
bool sendRemoteMessage(CSimpleSocket *socket, int16_t id,
                        const ::google::protobuf::MessageLite *msg, bool size_ready);


RPCService::RPCService()
{
    owner = NULL;
    holder = NULL;
}

RPCService::~RPCService()
{
    if (holder)
        holder->detach_connection(this);

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
    thread->detach();
}

ServerConnection::~ServerConnection()
{
    in_error = true;
    socket->Close();
    delete socket;
    delete thread;

    for (auto it = plugin_services.begin(); it != plugin_services.end(); ++it)
        delete it->second;

    delete core_service;
}

ServerFunctionBase *ServerConnection::findFunction(color_ostream &out, const std::string &plugin, const std::string &name)
{
    RPCService *svc;

    if (plugin.empty())
        svc = core_service;
    else
    {
        svc = plugin_services[plugin];

        if (!svc)
        {
            Plugin *plug = Core::getInstance().plug_mgr->getPluginByName(plugin);
            if (!plug)
            {
                out.printerr("No such plugin: %s\n", plugin.c_str());
                return NULL;
            }

            svc = plug->rpc_connect(out);
            if (!svc)
            {
                out.printerr("Plugin %s doesn't export any RPC methods.\n", plugin.c_str());
                return NULL;
            }

            svc->finalize(this, &functions);
            plugin_services[plugin] = svc;
        }
    }

    return svc->getFunction(name);
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

    if (!sendRemoteMessage(owner->socket, RPC_REPLY_TEXT, &msg, false))
    {
        owner->in_error = true;
        Core::printerr("Error writing text into client socket.\n");
    }
}

void ServerConnection::threadFn(void *arg)
{
    ServerConnection *me = (ServerConnection*)arg;

    me->threadFn();

    delete me;
}

void ServerConnection::threadFn()
{
    color_ostream_proxy out(Core::getInstance().getConsole());

    /* Handshake */

    {
        RPCHandshakeHeader header;

        if (!readFullBuffer(socket, &header, sizeof(header)))
        {
            out << "In RPC server: could not read handshake header." << endl;
            return;
        }

        if (memcmp(header.magic, RPCHandshakeHeader::REQUEST_MAGIC, sizeof(header.magic)) ||
            header.version < 1 || header.version > 255)
        {
            out << "In RPC server: invalid handshake header." << endl;
            return;
        }

        memcpy(header.magic, RPCHandshakeHeader::RESPONSE_MAGIC, sizeof(header.magic));
        header.version = 1;

        if (socket->Send((uint8*)&header, sizeof(header)) != sizeof(header))
        {
            out << "In RPC server: could not send handshake response." << endl;
            return;
        }
    }

    /* Processing */

    std::cerr << "Client connection established." << endl;

    while (!in_error) {
        // Read the message
        RPCMessageHeader header;

        if (!readFullBuffer(socket, &header, sizeof(header)))
        {
            out.printerr("In RPC server: I/O error in receive header.\n");
            break;
        }

        if ((DFHack::DFHackReplyCode)header.id == RPC_REQUEST_QUIT)
            break;

        if (header.size < 0 || header.size > RPCMessageHeader::MAX_MESSAGE_SIZE)
        {
            out.printerr("In RPC server: invalid received size %d.\n", header.size);
            break;
        }

        std::unique_ptr<uint8_t[]> buf(new uint8_t[header.size]);

        if (!readFullBuffer(socket, buf.get(), header.size))
        {
            out.printerr("In RPC server: I/O error in receive %d bytes of data.\n", header.size);
            break;
        }

        //out.print("Handling %d:%d\n", header.id, header.size);

        // Find and call the function
        int in_size = header.size;

        ServerFunctionBase *fn = vector_get(functions, header.id);
        MessageLite *reply = NULL;
        command_result res = CR_FAILURE;

        if (!fn)
        {
            stream.printerr("RPC call of invalid id %d\n", header.id);
        }
        else
        {
            if (((fn->flags & SF_ALLOW_REMOTE) != SF_ALLOW_REMOTE) && strcmp(socket->GetClientAddr(), "127.0.0.1") != 0)
            {
                stream.printerr("In call to %s: forbidden host: %s\n", fn->name, socket->GetClientAddr());
            }
            else if (!fn->in()->ParseFromArray(buf.get(), header.size))
            {
                stream.printerr("In call to %s: could not decode input args.\n", fn->name);
            }
            else
            {
                buf.reset();

                reply = fn->out();

                if (fn->flags & SF_DONT_SUSPEND)
                {
                    res = fn->execute(stream);
                }
                else
                {
                    CoreSuspender suspend;
                    res = fn->execute(stream);
                }
            }
        }

        // Flush all text output
        if (in_error)
            break;

        //out.print("Answer %d:%d\n", res, reply);

        // Send reply
        int out_size = (reply ? reply->ByteSize() : 0);

        if (out_size > RPCMessageHeader::MAX_MESSAGE_SIZE)
        {
            stream.printerr("In call to %s: reply too large: %d.\n",
                                (fn ? fn->name : "UNKNOWN"), out_size);
            res = CR_LINK_FAILURE;
        }

        stream.flush();

        if (res == CR_OK && reply)
        {
            if (!sendRemoteMessage(socket, RPC_REPLY_RESULT, reply, true))
            {
                out.printerr("In RPC server: I/O error in send result.\n");
                break;
            }
        }
        else
        {
            header.id = RPC_REPLY_FAIL;
            header.size = res;

            if (socket->Send((uint8_t*)&header, sizeof(header)) != sizeof(header))
            {
                out.printerr("In RPC server: I/O error in send failure code.\n");
                break;
            }
        }

        // Cleanup
        if (fn)
        {
            fn->reset((fn->flags & SF_CALLED_ONCE) ||
                      (out_size > 128*1024 || in_size > 32*1024));
        }
    }

    std::cerr << "Shutting down client connection." << endl;
}

ServerMain::ServerMain()
{
    socket = new CPassiveSocket();
    thread = NULL;
}

ServerMain::~ServerMain()
{
    socket->Close();
    delete socket;
    delete thread;
}

bool ServerMain::listen(int port)
{
    if (thread)
        return true;

    socket->Initialize();

    std::string filename("dfhack-config/remote-server.json");

    Json::Value configJson;

    std::ifstream inFile(filename, std::ios_base::in);

    bool allow_remote = false;

    if (inFile.is_open())
    {
        inFile >> configJson;
        inFile.close();

        allow_remote = configJson.get("allow_remote", "false").asBool();
        port = configJson.get("port", port).asInt();
    }

    configJson["allow_remote"] = allow_remote;
    configJson["port"] = port;

    std::ofstream outFile(filename, std::ios_base::trunc);

    if (outFile.is_open())
    {
        outFile << configJson;
        outFile.close();
    }

    if (allow_remote)
    {
        if (!socket->Listen(NULL, port))
            return false;
    }
    else
    {
        if (!socket->Listen("127.0.0.1", port))
            return false;
    }

    thread = new tthread::thread(threadFn, this);
    thread->detach();
    return true;
}

void ServerMain::threadFn(void *arg)
{
    ServerMain *me = (ServerMain*)arg;
    CActiveSocket *client;

    while ((client = me->socket->Accept()) != NULL)
    {
        new ServerConnection(client);
    }
}
