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

#include "RemoteClient.h"
#include <ActiveSocket.h>
#include "MiscUtils.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

#include <memory>

using namespace DFHack;

#include "tinythread.h"
using namespace tthread;

using dfproto::CoreTextNotification;

using google::protobuf::MessageLite;

const char RPCHandshakeHeader::REQUEST_MAGIC[9] = "DFHack?\n";
const char RPCHandshakeHeader::RESPONSE_MAGIC[9] = "DFHack!\n";

void color_ostream_proxy::decode(CoreTextNotification *data)
{
    flush_proxy();

    int cnt = data->fragments_size();
    if (cnt > 0) {
        target->begin_batch();

        for (int i = 0; i < cnt; i++)
        {
            auto &frag = data->fragments(i);

            color_value color = frag.has_color() ? color_value(frag.color()) : COLOR_RESET;
            target->add_text(color, frag.text());
        }

        target->end_batch();
    }
}

RemoteClient::RemoteClient(color_ostream *default_output)
    : p_default_output(default_output)
{
    active = false;
    socket = new CActiveSocket();
    suspend_ready = false;

    if (!p_default_output)
    {
        delete_output = true;
        p_default_output = new color_ostream_wrapper(std::cout);
    }
    else
        delete_output = false;
}

RemoteClient::~RemoteClient()
{
    disconnect();
    delete socket;

    if (delete_output)
        delete p_default_output;
}

bool readFullBuffer(CSimpleSocket *socket, void *buf, int size)
{
    if (!socket->IsSocketValid())
        return false;

    char *ptr = (char*)buf;
    while (size > 0) {
        int cnt = socket->Receive(size);
        if (cnt <= 0)
            return false;
        memcpy(ptr, socket->GetData(), cnt);
        ptr += cnt;
        size -= cnt;
    }

    return true;
}

int RemoteClient::GetDefaultPort()
{
    const char *port = getenv("DFHACK_PORT");
    if (!port) port = "0";

    int portval = atoi(port);
    if (portval <= 0)
        return 5000;
    else
        return portval;
}

bool RemoteClient::connect(int port)
{
    assert(!active);

    if (port <= 0)
        port = GetDefaultPort();

    if (!socket->Initialize())
    {
        default_output().printerr("Socket init failed.\n");
        return false;
    }

    if (!socket->Open("localhost", port))
    {
        default_output().printerr("Could not connect to localhost: %d\n", port);
        return false;
    }

    active = true;

    RPCHandshakeHeader header;
    memcpy(header.magic, RPCHandshakeHeader::REQUEST_MAGIC, sizeof(header.magic));
    header.version = 1;

    if (socket->Send((uint8*)&header, sizeof(header)) != sizeof(header))
    {
        default_output().printerr("Could not send handshake header.\n");
        socket->Close();
        return active = false;
    }

    if (!readFullBuffer(socket, &header, sizeof(header)))
    {
        default_output().printerr("Could not read handshake header.\n");
        socket->Close();
        return active = false;
    }

    if (memcmp(header.magic, RPCHandshakeHeader::RESPONSE_MAGIC, sizeof(header.magic)) ||
        header.version != 1)
    {
        default_output().printerr("Invalid handshake response.\n");
        socket->Close();
        return active = false;
    }

    bind_call.name = "BindMethod";
    bind_call.p_client = this;
    bind_call.id = 0;

    runcmd_call.name = "RunCommand";
    runcmd_call.p_client = this;
    runcmd_call.id = 1;

    return true;
}

void RemoteClient::disconnect()
{
    if (active && socket->IsSocketValid())
    {
        RPCMessageHeader header;
        header.id = RPC_REQUEST_QUIT;
        header.size = 0;
        if (socket->Send((uint8_t*)&header, sizeof(header)) != sizeof(header))
            default_output().printerr("Could not send the disconnect message.\n");
    }

    socket->Close();
}

bool RemoteClient::bind(color_ostream &out, RemoteFunctionBase *function,
                        const std::string &name, const std::string &proto)
{
    if (!active || !socket->IsSocketValid())
        return false;

    bind_call.reset();

    {
        auto in = bind_call.in();

        in->set_method(name);
        if (!proto.empty())
            in->set_plugin(proto);
        in->set_input_msg(function->p_in_template->GetTypeName());
        in->set_output_msg(function->p_out_template->GetTypeName());
    }

    if (bind_call(out) != CR_OK)
        return false;

    function->id = bind_call.out()->assigned_id();

    return true;
}

command_result RemoteClient::run_command(color_ostream &out, const std::string &cmd,
                                         const std::vector<std::string> &args)
{
    if (!active || !socket->IsSocketValid())
    {
        out.printerr("In RunCommand: client connection not valid.\n");
        return CR_FAILURE;
    }

    runcmd_call.reset();

    runcmd_call.in()->set_command(cmd);
    for (size_t i = 0; i < args.size(); i++)
        runcmd_call.in()->add_arguments(args[i]);

    return runcmd_call(out);
}

int RemoteClient::suspend_game()
{
    if (!active)
        return -1;

    if (!suspend_ready) {
        suspend_ready = true;

        suspend_call.bind(this, "CoreSuspend");
        resume_call.bind(this, "CoreResume");
    }

    if (suspend_call(default_output()) == CR_OK)
        return suspend_call.out()->value();
    else
        return -1;
}

int RemoteClient::resume_game()
{
    if (!suspend_ready)
        return -1;

    if (resume_call(default_output()) == CR_OK)
        return resume_call.out()->value();
    else
        return -1;
}

void RPCFunctionBase::reset(bool free)
{
    if (free)
    {
        delete p_in;
        delete p_out;
        p_in = p_out = NULL;
    }
    else
    {
        if (p_in)
            p_in->Clear();
        if (p_out)
            p_out->Clear();
    }
}

bool RemoteFunctionBase::bind(color_ostream &out, RemoteClient *client,
                              const std::string &name, const std::string &proto)
{
    if (isValid())
    {
        if (p_client == client && this->name == name && this->proto == proto)
            return true;

        out.printerr("Function already bound to %s::%s\n",
                     this->proto.c_str(), this->name.c_str());
        return false;
    }

    this->name = name;
    this->proto = proto;
    this->p_client = client;

    return client->bind(out, this, name, proto);
}

bool sendRemoteMessage(CSimpleSocket *socket, int16_t id, const MessageLite *msg, bool size_ready)
{
    int size = size_ready ? msg->GetCachedSize() : msg->ByteSize();
    int fullsz = size + sizeof(RPCMessageHeader);

    uint8_t *data = new uint8_t[fullsz];
    RPCMessageHeader *hdr = (RPCMessageHeader*)data;

    hdr->id = id;
    hdr->size = size;

    uint8_t *pstart = data + sizeof(RPCMessageHeader);
    uint8_t *pend = msg->SerializeWithCachedSizesToArray(pstart);
    assert((pend - pstart) == size);

    int got = socket->Send(data, fullsz);
    delete[] data;
    return (got == fullsz);
}

command_result RemoteFunctionBase::execute(color_ostream &out,
                                           const message_type *input, message_type *output)
{
    if (!isValid())
    {
        out.printerr("Calling an unbound RPC function %s::%s.\n",
                     this->proto.c_str(), this->name.c_str());
        return CR_NOT_IMPLEMENTED;
    }

    if (!p_client->socket->IsSocketValid())
    {
        out.printerr("In call to %s::%s: invalid socket.\n",
                     this->proto.c_str(), this->name.c_str());
        return CR_LINK_FAILURE;
    }

    int send_size = input->ByteSize();

    if (send_size > RPCMessageHeader::MAX_MESSAGE_SIZE)
    {
        out.printerr("In call to %s::%s: message too large: %d.\n",
                     this->proto.c_str(), this->name.c_str(), send_size);
        return CR_LINK_FAILURE;
    }

    if (!sendRemoteMessage(p_client->socket, id, input, true))
    {
        out.printerr("In call to %s::%s: I/O error in send.\n",
                     this->proto.c_str(), this->name.c_str());
        return CR_LINK_FAILURE;
    }

    color_ostream_proxy text_decoder(out);
    CoreTextNotification text_data;

    output->Clear();

    for (;;) {
        RPCMessageHeader header;

        if (!readFullBuffer(p_client->socket, &header, sizeof(header)))
        {
            out.printerr("In call to %s::%s: I/O error in receive header.\n",
                         this->proto.c_str(), this->name.c_str());
            return CR_LINK_FAILURE;
        }

        //out.print("Received %d:%d\n", header.id, header.size);

        if ((DFHack::DFHackReplyCode)header.id == RPC_REPLY_FAIL)
            return header.size == CR_OK ? CR_FAILURE : command_result(header.size);

        if (header.size < 0 || header.size > RPCMessageHeader::MAX_MESSAGE_SIZE)
        {
            out.printerr("In call to %s::%s: invalid received size %d.\n",
                         this->proto.c_str(), this->name.c_str(), header.size);
            return CR_LINK_FAILURE;
        }

        uint8_t *buf = new uint8_t[header.size];

        if (!readFullBuffer(p_client->socket, buf, header.size))
        {
            out.printerr("In call to %s::%s: I/O error in receive %d bytes of data.\n",
                         this->proto.c_str(), this->name.c_str(), header.size);
            return CR_LINK_FAILURE;
        }

        switch (header.id) {
        case RPC_REPLY_RESULT:
            if (!output->ParseFromArray(buf, header.size))
            {
                out.printerr("In call to %s::%s: error parsing received result.\n",
                             this->proto.c_str(), this->name.c_str());
                delete[] buf;
                return CR_LINK_FAILURE;
            }

            delete[] buf;
            return CR_OK;

        case RPC_REPLY_TEXT:
            text_data.Clear();
            if (text_data.ParseFromArray(buf, header.size))
                text_decoder.decode(&text_data);
            else
                out.printerr("In call to %s::%s: received invalid text data.\n",
                             this->proto.c_str(), this->name.c_str());
            break;

        default:
            break;
        }
        delete[] buf;
    }
}
