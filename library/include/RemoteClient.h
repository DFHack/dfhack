/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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

#pragma once
#include "Pragma.h"
#include "Export.h"
#include "ColorText.h"

#include "ActiveSocket.h"

#include "CoreProtocol.pb.h"

namespace  DFHack
{
    using dfproto::CoreVoidReply;

    enum command_result
    {
        CR_WOULD_BREAK = -2,
        CR_NOT_IMPLEMENTED = -1,
        CR_OK = 0,
        CR_FAILURE = 1,
        CR_WRONG_USAGE = 2
    };

    enum DFHackReplyCode : int16_t {
        RPC_REPLY_RESULT = -1,
        RPC_REPLY_FAIL = -2,
        RPC_REPLY_TEXT = -3,
        RPC_REQUEST_QUIT = -4
    };

    struct RPCHandshakeHeader {
        char magic[8];
        int version;

        static const char REQUEST_MAGIC[9];
        static const char RESPONSE_MAGIC[9];
    };

    struct RPCMessageHeader {
        int16_t id;
        int32_t size;
    };

    class DFHACK_EXPORT RemoteClient;

    class DFHACK_EXPORT RPCFunctionBase {
    public:
        typedef ::google::protobuf::MessageLite message_type;

        const message_type *const p_in_template;
        const message_type *const p_out_template;

        message_type *make_in() const {
            return p_in_template->New();
        }

        message_type *in() {
            if (!p_in) p_in = make_in();
            return p_in;
        }

        message_type *make_out() const {
            return p_out_template->New();
        }

        message_type *out() {
            if (!p_out) p_out = make_out();
            return p_out;
        }

        void reset(bool free = false);

    protected:
        RPCFunctionBase(const message_type *in, const message_type *out)
            : p_in_template(in), p_out_template(out), p_in(NULL), p_out(NULL)
        {}
        ~RPCFunctionBase() { delete p_in; delete p_out; }

        message_type *p_in, *p_out;
    };

    class DFHACK_EXPORT RemoteFunctionBase : public RPCFunctionBase {
    public:
        bool bind(color_ostream &out,
                  RemoteClient *client, const std::string &name,
                  const std::string &proto = std::string());
        bool isValid() { return (p_client != NULL); }

    protected:
        friend class RemoteClient;

        RemoteFunctionBase(const message_type *in, const message_type *out)
            : RPCFunctionBase(in, out), p_client(NULL), id(-1)
        {}

        command_result execute(color_ostream &out, const message_type *input, message_type *output);

        std::string name, proto;
        RemoteClient *p_client;
        int16_t id;
    };

    template<typename In, typename Out>
    class RemoteFunction : public RemoteFunctionBase {
    public:
        In *make_in() const { return static_cast<In*>(RemoteFunctionBase::make_in()); }
        In *in() { return static_cast<In*>(RemoteFunctionBase::in()); }
        Out *make_out() const { return static_cast<Out*>(RemoteFunctionBase::make_out()); }
        Out *out() { return static_cast<Out*>(RemoteFunctionBase::out()); }

        RemoteFunction() : RemoteFunctionBase(&In::default_instance(), &Out::default_instance()) {}

        command_result execute(color_ostream &out) {
            return RemoteFunctionBase::execute(out, this->in(), this->out());
        }
        command_result operator() (color_ostream &out, const In *input, Out *output) {
            return RemoteFunctionBase::execute(out, input, output);
        }
    };

    bool readFullBuffer(CSimpleSocket &socket, void *buf, int size);
    bool sendRemoteMessage(CSimpleSocket &socket, int16_t id, const ::google::protobuf::MessageLite *msg);

    class DFHACK_EXPORT RemoteClient
    {
        friend class RemoteFunctionBase;

        bool bind(color_ostream &out, RemoteFunctionBase *function,
                  const std::string &name, const std::string &proto);

    public:
        RemoteClient();
        ~RemoteClient();

        static int GetDefaultPort();

        bool connect(int port = -1);
        void disconnect();

    private:
        bool active;
        CActiveSocket socket;

        RemoteFunction<dfproto::CoreBindRequest,dfproto::CoreBindReply> bind_call;
    };
}
