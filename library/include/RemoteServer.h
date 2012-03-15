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
#include "RemoteClient.h"
#include "Core.h"

#include "PassiveSocket.h"

namespace  DFHack
{
    class DFHACK_EXPORT ServerConnection;
    class DFHACK_EXPORT RPCService;

    class DFHACK_EXPORT ServerFunctionBase : public RPCFunctionBase {
    public:
        const char *const name;

        virtual command_result execute(color_ostream &stream) = 0;

        int16_t getId() { return id; }

    protected:
        friend class RPCService;

        ServerFunctionBase(const message_type *in, const message_type *out, RPCService *owner, const char *name)
            : RPCFunctionBase(in, out), name(name), owner(owner), id(-1)
        {}

        RPCService *owner;
        int16_t id;
    };

    template<typename In, typename Out>
    class ServerFunction : public ServerFunctionBase {
    public:
        typedef command_result (*function_type)(color_ostream &out, const In *input, Out *output);

        In *in() { return static_cast<In*>(RPCFunctionBase::in()); }
        Out *out() { return static_cast<Out*>(RPCFunctionBase::out()); }

        ServerFunction(RPCService *owner, const char *name, function_type fptr)
            : ServerFunctionBase(&In::default_instance(), &Out::default_instance(), owner, name),
              fptr(fptr) {}

        virtual command_result execute(color_ostream &stream) { return fptr(stream, in(), out()); }

    private:
        function_type fptr;
    };

    template<typename In>
    class VoidServerFunction : public ServerFunctionBase {
    public:
        typedef command_result (*function_type)(color_ostream &out, const In *input);

        In *in() { return static_cast<In*>(RPCFunctionBase::in()); }

        VoidServerFunction(RPCService *owner, const char *name, function_type fptr)
            : ServerFunctionBase(&In::default_instance(), &EmptyMessage::default_instance(), owner, name),
              fptr(fptr) {}

        virtual command_result execute(color_ostream &stream) { return fptr(stream, in()); }

    private:
        function_type fptr;
    };

    template<typename Svc, typename In, typename Out>
    class ServerMethod : public ServerFunctionBase {
    public:
        typedef command_result (Svc::*function_type)(color_ostream &out, const In *input, Out *output);

        In *in() { return static_cast<In*>(RPCFunctionBase::in()); }
        Out *out() { return static_cast<Out*>(RPCFunctionBase::out()); }

        ServerMethod(RPCService *owner, const char *name, function_type fptr)
            : ServerFunctionBase(&In::default_instance(), &Out::default_instance(), owner, name),
              fptr(fptr) {}

        virtual command_result execute(color_ostream &stream) {
            return (static_cast<Svc*>(owner)->*fptr)(stream, in(), out());
        }

    private:
        function_type fptr;
    };

    template<typename Svc, typename In>
    class VoidServerMethod : public ServerFunctionBase {
    public:
        typedef command_result (Svc::*function_type)(color_ostream &out, const In *input);

        In *in() { return static_cast<In*>(RPCFunctionBase::in()); }

        VoidServerMethod(RPCService *owner, const char *name, function_type fptr)
            : ServerFunctionBase(&In::default_instance(), &EmptyMessage::default_instance(), owner, name),
              fptr(fptr) {}

        virtual command_result execute(color_ostream &stream) {
            return (static_cast<Svc*>(owner)->*fptr)(stream, in());
        }

    private:
        function_type fptr;
    };

    class Plugin;

    class DFHACK_EXPORT RPCService {
        friend class ServerConnection;

        std::vector<ServerFunctionBase*> functions;
        std::map<std::string, ServerFunctionBase*> lookup;
        ServerConnection *owner;

        Plugin *holder;

        void finalize(ServerConnection *owner, std::vector<ServerFunctionBase*> *ftable);

    public:
        RPCService();
        virtual ~RPCService();

        ServerFunctionBase *getFunction(const std::string &name);

        template<typename In, typename Out>
        void addFunction(
            const char *name,
            command_result (*fptr)(color_ostream &out, const In *input, Out *output)
        ) {
            assert(!owner);
            functions.push_back(new ServerFunction<In,Out>(this, name, fptr));
        }

        template<typename In>
        void addFunction(
            const char *name,
            command_result (*fptr)(color_ostream &out, const In *input)
        ) {
            assert(!owner);
            functions.push_back(new VoidServerFunction<In>(this, name, fptr));
        }

    protected:
        ServerConnection *connection() { return owner; }

        template<typename Svc, typename In, typename Out>
        void addMethod(
            const char *name,
            command_result (Svc::*fptr)(color_ostream &out, const In *input, Out *output)
        ) {
            assert(!owner);
            functions.push_back(new ServerMethod<Svc,In,Out>(this, name, fptr));
        }

        template<typename Svc, typename In>
        void addMethod(
            const char *name,
            command_result (Svc::*fptr)(color_ostream &out, const In *input)
        ) {
            assert(!owner);
            functions.push_back(new VoidServerMethod<Svc,In>(this, name, fptr));
        }
    };

    class CoreService : public RPCService {
    public:
        CoreService();

        command_result BindMethod(color_ostream &stream,
                                  const dfproto::CoreBindRequest *in,
                                  dfproto::CoreBindReply *out);
        command_result RunCommand(color_ostream &stream,
                                  const dfproto::CoreRunCommandRequest *in);
    };

    class DFHACK_EXPORT ServerConnection {
        class connection_ostream : public buffered_color_ostream {
            ServerConnection *owner;

        protected:
            virtual void flush_proxy();

        public:
            connection_ostream(ServerConnection *owner) : owner(owner) {}
        };

        bool in_error;
        CActiveSocket *socket;
        connection_ostream stream;

        std::vector<ServerFunctionBase*> functions;

        CoreService *core_service;
        std::map<std::string, RPCService*> plugin_services;

        tthread::thread *thread;
        static void threadFn(void *);

    public:
        ServerConnection(CActiveSocket *socket);
        ~ServerConnection();

        ServerFunctionBase *findFunction(const std::string &plugin, const std::string &name);
    };

    class DFHACK_EXPORT ServerMain {
        CPassiveSocket socket;

        tthread::thread *thread;
        static void threadFn(void *);
    public:
        ServerMain();
        ~ServerMain();

        bool listen(int port);
    };
}
