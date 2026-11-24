/*
 * luadebugger: remote debugging server for DFHack Lua.
 *
 * Hosts a Debug Adapter Protocol (DAP) endpoint on a loopback TCP socket so
 * that external tools (VSCode, nvim-dap, the bundled CLI client) can drive a
 * Lua debug session inside the running game.
 *
 * All socket I/O happens on dedicated threads. The Lua debuggee (the
 * plugins.luadebugger module) never touches the socket directly; it exchanges
 * whole DAP message bodies (JSON) with this plugin through two condition
 * variable mailboxes. This keeps the simulation thread free of socket calls
 * and satisfies the rule that the lua_State may only be accessed under
 * CoreSuspender on the simulation thread.
 *
 * While the debuggee is stopped at a breakpoint, the simulation thread blocks
 * inside debugger_recv() waiting on the inbound mailbox, which is exactly the
 * "paused" behavior a debugger needs -- and requires no cooperation from DF's
 * render/event loop.
 */

#include "Console.h"
#include "Core.h"
#include "Debug.h"
#include "PluginManager.h"
#include "PluginLua.h"
#include "LuaTools.h"

#include <ActiveSocket.h>
#include <PassiveSocket.h>
#include <SimpleSocket.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace DFHack;

DFHACK_PLUGIN("luadebugger");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

namespace DFHack {
    DBG_DECLARE(luadebugger, status, DebugCategory::LINFO);
    DBG_DECLARE(luadebugger, io, DebugCategory::LINFO);
}

static constexpr int32_t DEFAULT_PORT = 43030;

namespace {

struct Mailbox {
    std::mutex m;
    std::condition_variable cv;
    std::deque<std::string> queue;
};

class DebugServer {
public:
    bool start(color_ostream &out, int32_t port);
    void stop();

    bool is_listening() const { return listening.load(); }
    int32_t get_port() const { return bound_port.load(); }
    bool has_client() const { return client_connected.load(); }

    // Consumed by plugin_onupdate on the simulation thread.
    bool take_poll_flag() { return needs_poll.exchange(false); }

    // Lua-facing mailbox API. Only safe to call from the simulation thread.
    std::string recv(int32_t timeout_ms);
    bool send(const std::string &msg);

private:
    void accept_loop();
    void client_io_loop(std::unique_ptr<CActiveSocket> sock, uint32_t id);
    void client_disconnected(uint32_t id);

    Mailbox inbound;   // complete DAP message bodies, socket -> lua
    Mailbox outbound;  // complete DAP message bodies, lua -> socket

    std::atomic<bool> needs_poll{false};     // asks onupdate to call Lua poll()
    std::atomic<bool> client_connected{false};
    std::atomic<uint32_t> client_id{0};      // increments per accepted client
    std::atomic<bool> stopping{false};
    std::atomic<bool> listening{false};
    std::atomic<int32_t> bound_port{0};

    CPassiveSocket listener;
    std::thread accept_thread;
    std::thread io_thread;
};

DebugServer server;

// ---------------------------------------------------------------------------
// DAP framing: "Content-Length: <n>\r\n\r\n<json body>"
// ---------------------------------------------------------------------------

struct DapFramer {
    std::string buf;
    size_t body_start = std::string::npos;
    int32_t content_length = -1;

    // Append received bytes; push each complete message body to `out`.
    void feed(const uint8_t *data, size_t len, std::deque<std::string> &out) {
        buf.append(reinterpret_cast<const char *>(data), len);
        for (;;) {
            if (content_length < 0) {
                size_t hdr_end = buf.find("\r\n\r\n");
                if (hdr_end == std::string::npos)
                    return;
                std::string header = buf.substr(0, hdr_end);
                body_start = hdr_end + 4;
                size_t pos = header.find("Content-Length:");
                if (pos == std::string::npos) {
                    // malformed; resync after this header block
                    buf.erase(0, body_start);
                    body_start = std::string::npos;
                    continue;
                }
                content_length = (std::max)(0, std::atoi(header.c_str() + pos + 15));
            }
            if (buf.size() < body_start + size_t(content_length))
                return;
            out.emplace_back(buf.substr(body_start, content_length));
            buf.erase(0, body_start + content_length);
            body_start = std::string::npos;
            content_length = -1;
        }
    }

    static std::string frame(const std::string &body) {
        return "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
    }
};

// ---------------------------------------------------------------------------
// server lifecycle
// ---------------------------------------------------------------------------

bool DebugServer::start(color_ostream &out, int32_t port) {
    if (listening.load())
        return true;

    stopping = false;
    if (!listener.Initialize()) {
        out.printerr("luadebugger: failed to initialize socket\n");
        return false;
    }
    // The debug channel grants full code execution in the Lua context; it must
    // never be reachable from other hosts.
    if (!listener.Listen("127.0.0.1", port)) {
        out.printerr("luadebugger: failed to bind 127.0.0.1:{}: {}\n",
                     port, listener.DescribeError());
        listener.Close();
        return false;
    }
    listener.SetNonblocking();
    bound_port = port;
    listening = true;
    accept_thread = std::thread(&DebugServer::accept_loop, this);
    out.print("luadebugger: listening for debug clients on 127.0.0.1:{}\n", port);
    return true;
}

void DebugServer::accept_loop() {
    while (!stopping.load()) {
        std::unique_ptr<CActiveSocket> sock{listener.Accept()};
        if (!sock) {
            auto err = listener.GetSocketError();
            if (err == CSimpleSocket::SocketInvalidSocket ||
                err == CSimpleSocket::SocketProtocolError)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        if (client_connected.load()) {
            // Only a single client session is supported; drop extras.
            WARN(io).print("luadebugger: rejecting extra debug client\n");
            sock->Close();
            continue;
        }

        sock->SetBlocking();
        sock->SetReceiveTimeout(0, 50000);
        sock->SetSendTimeout(0, 50000);
        uint32_t id = ++client_id;
        client_connected = true;
        needs_poll = true;
        if (io_thread.joinable())
            io_thread.join();
        io_thread = std::thread(&DebugServer::client_io_loop, this,
                                std::move(sock), id);
        DEBUG(status).print("luadebugger: debug client connected\n");
    }
}

void DebugServer::client_io_loop(std::unique_ptr<CActiveSocket> sock, uint32_t id) {
    DapFramer framer;
    bool alive = true;

    while (alive && !stopping.load()) {
        int32_t n = sock->Receive(64 * 1024);
        if (n > 0) {
            {
                std::lock_guard<std::mutex> lk(inbound.m);
                framer.feed(sock->GetData(), n, inbound.queue);
            }
            inbound.cv.notify_all();
            needs_poll = true;
        }
        else if (n < 0) {
            auto err = sock->GetSocketError();
            if (err != CSimpleSocket::SocketTimedout &&
                err != CSimpleSocket::SocketEwouldblock)
                break;
        }
        else {
            break; // orderly close by peer
        }

        // Drain outbound mailbox.
        for (;;) {
            std::string msg;
            {
                std::lock_guard<std::mutex> lk(outbound.m);
                if (outbound.queue.empty())
                    break;
                msg = std::move(outbound.queue.front());
                outbound.queue.pop_front();
            }
            std::string framed = DapFramer::frame(msg);
            if (sock->Send(reinterpret_cast<const uint8_t *>(framed.data()),
                           framed.size()) <= 0) {
                alive = false;
                break;
            }
        }
    }

    sock->Close();
    client_disconnected(id);
}

void DebugServer::client_disconnected(uint32_t id) {
    // a newer client may already have been accepted; only reset state that
    // still belongs to this connection
    if (client_id.load() != id)
        return;
    client_connected = false;
    needs_poll = true;
    {
        std::lock_guard<std::mutex> lk(inbound.m);
        inbound.queue.clear();
    }
    {
        std::lock_guard<std::mutex> lk(outbound.m);
        outbound.queue.clear();
    }
    inbound.cv.notify_all();
    DEBUG(status).print("luadebugger: debug client disconnected\n");
}

void DebugServer::stop() {
    stopping = true;
    listener.Close();
    inbound.cv.notify_all();
    outbound.cv.notify_all();
    if (accept_thread.joinable())
        accept_thread.join();
    if (io_thread.joinable())
        io_thread.join();
    {
        std::lock_guard<std::mutex> lk(inbound.m);
        inbound.queue.clear();
    }
    {
        std::lock_guard<std::mutex> lk(outbound.m);
        outbound.queue.clear();
    }
    client_connected = false;
    listening = false;
}

// ---------------------------------------------------------------------------
// mailbox access from the simulation thread (exposed to Lua)
// ---------------------------------------------------------------------------

std::string DebugServer::recv(int32_t timeout_ms) {
    std::unique_lock<std::mutex> lk(inbound.m);
    auto ready = [this]() {
        return !inbound.queue.empty() || !client_connected.load() || stopping.load();
    };
    if (timeout_ms < 0)
        inbound.cv.wait(lk, ready);
    else
        inbound.cv.wait_for(lk, std::chrono::milliseconds(timeout_ms), ready);
    if (inbound.queue.empty())
        return "";
    std::string msg = std::move(inbound.queue.front());
    inbound.queue.pop_front();
    // if messages are left over (e.g. a burst larger than one poll() drain),
    // make sure the sim thread gets asked to drain again
    if (!inbound.queue.empty())
        needs_poll = true;
    return msg;
}

bool DebugServer::send(const std::string &msg) {
    if (!client_connected.load())
        return false;
    {
        std::lock_guard<std::mutex> lk(outbound.m);
        outbound.queue.push_back(msg);
    }
    return true;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Lua-visible functions (injected into the plugins.luadebugger module env)
// ---------------------------------------------------------------------------

static std::string debugger_recv(int32_t timeout_ms) {
    return server.recv(timeout_ms);
}
static bool debugger_send(std::string msg) {
    return server.send(msg);
}
static bool debugger_connected() {
    return server.has_client();
}
static int32_t debugger_port() {
    return server.get_port();
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(debugger_recv),
    DFHACK_LUA_FUNCTION(debugger_send),
    DFHACK_LUA_FUNCTION(debugger_connected),
    DFHACK_LUA_FUNCTION(debugger_port),
    DFHACK_LUA_END
};

// ---------------------------------------------------------------------------
// plugin glue
// ---------------------------------------------------------------------------

static command_result luadebugger_cmd(color_ostream &out, std::vector<std::string> &params);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "luadebugger",
        "Remote Lua debugging server (DAP over TCP).",
        luadebugger_cmd));
    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (enable == is_enabled)
        return CR_OK;
    if (enable) {
        if (!server.start(out, server.get_port() > 0 ? server.get_port() : DEFAULT_PORT))
            return CR_FAILURE;
    } else {
        server.stop();
    }
    is_enabled = enable;
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (server.take_poll_flag()) {
        Lua::CallLuaModuleFunction(out, "plugins.luadebugger", "poll");
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    server.stop();
    return CR_OK;
}

static command_result luadebugger_cmd(color_ostream &out, std::vector<std::string> &params) {
    if (params.empty() || params[0] == "status") {
        out.print("luadebugger: {}{}\n",
                  server.is_listening() ? "listening" : "not listening",
                  server.is_listening()
                      ? " on 127.0.0.1:" + std::to_string(server.get_port()) : "");
        out.print("luadebugger: client {}\n",
                  server.has_client() ? "connected" : "not connected");
        return CR_OK;
    }
    if (params[0] == "stop") {
        server.stop();
        is_enabled = false;
        return CR_OK;
    }
    if (params[0] == "start") {
        int32_t port = DEFAULT_PORT;
        if (params.size() > 1)
            port = std::atoi(params[1].c_str());
        if (port <= 0 || port > 65535)
            return CR_WRONG_USAGE;
        if (server.start(out, port)) {
            is_enabled = true;
            return CR_OK;
        }
        return CR_FAILURE;
    }
    return CR_WRONG_USAGE;
}
