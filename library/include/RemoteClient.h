/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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
#include "Core.h"

class CPassiveSocket;
class CActiveSocket;
class CSimpleSocket;

#include "CoreProtocol.pb.h"

namespace  DFHack
{
    using dfproto::EmptyMessage;
    using dfproto::IntMessage;
    using dfproto::StringMessage;

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
        static const int MAX_MESSAGE_SIZE = 64*1048576;

        int16_t id;
        int32_t size;
    };

    /* Protocol description:
     *
     * 1. Handshake
     *
     *   Client initiates connection by sending the handshake
     *   request header. The server responds with the response
     *   magic. Currently both versions must be 1.
     *
     * 2. Interaction
     *
     *   Requests are done by exchanging messages between the
     *   client and the server. Messages consist of a serialized
     *   protobuf message preceeded by RPCMessageHeader. The size
     *   field specifies the length of the protobuf part.
     *
     *   NOTE: As a special exception, RPC_REPLY_FAIL uses the size
     *         field to hold the error code directly.
     *
     *   Every callable function is assigned a non-negative id by
     *   the server. Id 0 is reserved for BindMethod, which can be
     *   used to request any other id by function name. Id 1 is
     *   RunCommand, used to call console commands remotely.
     *
     *   The client initiates every call by sending a message with
     *   appropriate function id and input arguments. The server
     *   responds with zero or more RPC_REPLY_TEXT:CoreTextNotification
     *   messages, followed by RPC_REPLY_RESULT containing the output
     *   of the function if it succeeded, or RPC_REPLY_FAIL with the
     *   error code if it did not.
     *
     * 3. Disconnect
     *
     *   The client terminates the connection by sending an
     *   RPC_REQUEST_QUIT header with zero size and immediately
     *   closing the socket.
     */

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
        bool bind(RemoteClient *client, const std::string &name,
                  const std::string &plugin = std::string());
        bool bind(color_ostream &out,
                  RemoteClient *client, const std::string &name,
                  const std::string &plugin = std::string());

        bool isValid() { return (id >= 0); }

    protected:
        friend class RemoteClient;

        RemoteFunctionBase(const message_type *in, const message_type *out)
            : RPCFunctionBase(in, out), p_client(NULL), id(-1)
        {}

        inline color_ostream &default_ostream();
        command_result execute(color_ostream &out, const message_type *input, message_type *output);

        std::string name, plugin;
        RemoteClient *p_client;
        int16_t id;
    };

    template<typename In, typename Out = EmptyMessage>
    class RemoteFunction : public RemoteFunctionBase {
    public:
        In *make_in() const { return static_cast<In*>(RemoteFunctionBase::make_in()); }
        In *in() { return static_cast<In*>(RemoteFunctionBase::in()); }
        Out *make_out() const { return static_cast<Out*>(RemoteFunctionBase::make_out()); }
        Out *out() { return static_cast<Out*>(RemoteFunctionBase::out()); }

        RemoteFunction() : RemoteFunctionBase(&In::default_instance(), &Out::default_instance()) {}

        command_result operator() () {
            return p_client ? RemoteFunctionBase::execute(default_ostream(), in(), out())
                            : CR_NOT_IMPLEMENTED;
        }
        command_result operator() (color_ostream &stream) {
            return RemoteFunctionBase::execute(stream, in(), out());
        }
        command_result operator() (const In *input, Out *output) {
            return p_client ? RemoteFunctionBase::execute(default_ostream(), input, output)
                            : CR_NOT_IMPLEMENTED;
        }
        command_result operator() (color_ostream &stream, const In *input, Out *output) {
            return RemoteFunctionBase::execute(stream, input, output);
        }
    };

    template<typename In>
    class RemoteFunction<In,EmptyMessage> : public RemoteFunctionBase {
    public:
        In *make_in() const { return static_cast<In*>(RemoteFunctionBase::make_in()); }
        In *in() { return static_cast<In*>(RemoteFunctionBase::in()); }

        RemoteFunction() : RemoteFunctionBase(&In::default_instance(), &EmptyMessage::default_instance()) {}

        command_result operator() () {
            return p_client ? RemoteFunctionBase::execute(default_ostream(), in(), out())
                            : CR_NOT_IMPLEMENTED;
        }
        command_result operator() (color_ostream &stream) {
            return RemoteFunctionBase::execute(stream, in(), out());
        }
        command_result operator() (const In *input) {
            return p_client ? RemoteFunctionBase::execute(default_ostream(), input, out())
                            : CR_NOT_IMPLEMENTED;
        }
        command_result operator() (color_ostream &stream, const In *input) {
            return RemoteFunctionBase::execute(stream, input, out());
        }
    };

    class DFHACK_EXPORT RemoteClient
    {
        friend class RemoteFunctionBase;

        bool bind(color_ostream &out, RemoteFunctionBase *function,
                  const std::string &name, const std::string &plugin);

    public:
        RemoteClient(color_ostream *default_output = NULL);
        ~RemoteClient();

        static constexpr int DEFAULT_PORT = 5000;
        static int GetDefaultPort();

        color_ostream &default_output() { return *p_default_output; };

        bool connect(int port = -1);
        void disconnect();

        command_result run_command(const std::string &cmd, const std::vector<std::string> &args) {
            return run_command(default_output(), cmd, args);
        }
        command_result run_command(color_ostream &out, const std::string &cmd,
                                   const std::vector<std::string> &args);

        // For executing multiple calls in rapid succession.
        // Best used via RemoteSuspender.
        int suspend_game();
        int resume_game();

    private:
        bool active, delete_output;
        CActiveSocket *socket;
        color_ostream *p_default_output;

        RemoteFunction<dfproto::CoreBindRequest,dfproto::CoreBindReply> bind_call;
        RemoteFunction<dfproto::CoreRunCommandRequest> runcmd_call;

        bool suspend_ready;
        RemoteFunction<EmptyMessage, IntMessage> suspend_call, resume_call;
    };

    inline color_ostream &RemoteFunctionBase::default_ostream() {
        return p_client->default_output();
    }

    inline bool RemoteFunctionBase::bind(RemoteClient *client, const std::string &name,
                                         const std::string &plugin) {
        return bind(client->default_output(), client, name, plugin);
    }

    class RemoteSuspender {
        RemoteClient *client;
    public:
        RemoteSuspender(RemoteClient *client) : client(client) {
            if (!client || client->suspend_game() <= 0) client = NULL;
        }
        ~RemoteSuspender() {
            if (client) client->resume_game();
        }
    };
}
