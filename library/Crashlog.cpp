// Dl_info and the SA_SIGINFO handler form need the GNU/POSIX extensions.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "DFHackVersion.h"
#include "Crashlog.h"

#include <csignal>
#include <cstring>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <thread>

#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <execinfo.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>
#include <pthread.h>
#include <dlfcn.h>

#include "Core.h"
#include "lua.h"
#include "lauxlib.h"
#include "lstate.h"

const int BT_ENTRY_MAX = 25;
struct CrashInfo {
    int backtrace_entries = 0;
    void* backtrace[BT_ENTRY_MAX];
    int signal = 0;
    int sig_code = 0;
    void* fault_address = nullptr;
    // The lua_State the faulting thread was executing in, if it was inside
    // a DFHack Lua entry point when it crashed.
    lua_State* active_lua_state = nullptr;
    pid_t thread_id = 0;
};

CrashInfo crash_info;

std::atomic<bool> crashed = false;
std::atomic<bool> crashlog_ready = false;
std::atomic<bool> shutdown = false;

// Load base of liblua53, resolved at init so the crashlog thread can
// recognize Lua frames without depending on anything else being intact.
void* lua_image_base = nullptr;

// Use eventfd for async-signal safe waiting
int crashlog_complete = -1;

void flag_set(std::atomic_bool &atom) {
    atom.store(true);
    atom.notify_all();
}
void flag_wait(std::atomic_bool &atom) {
    atom.wait(false);
}

void signal_crashlog_complete() {
    if (crashlog_complete == -1)
        return;
    uint64_t v = 1;
    [[maybe_unused]] auto _ = write(crashlog_complete, &v, sizeof(v));
}

std::thread crashlog_thread;

// Force this method to be inlined so that it doesn't create a stack frame
[[gnu::always_inline]] inline void handle_signal_internal(int sig, siginfo_t *si) {
    if (shutdown.load() || crashed.exchange(true) || crashlog_ready.load()) {
        // Ensure the signal handler doesn't try to write a crashlog
        // whilst the crashlog thread is unavailable.
        std::quick_exit(1);
    }
    crash_info.signal = sig;
    if (si) {
        crash_info.sig_code = si->si_code;
        crash_info.fault_address = si->si_addr;
    }
    crash_info.thread_id = (pid_t)syscall(SYS_gettid);
    crash_info.active_lua_state = DFHack::Crashlog::active_state();
    crash_info.backtrace_entries = backtrace(crash_info.backtrace, BT_ENTRY_MAX);

    // Signal saving of crashlog
    flag_set(crashlog_ready);
    // Wait for completion via eventfd read, if fd isn't valid, bail
    if (crashlog_complete != -1) {
        [[maybe_unused]] uint64_t v;
        [[maybe_unused]] auto _ = read(crashlog_complete, &v, sizeof(v));
    }
    std::quick_exit(1);
}

extern "C" void dfhack_crashlog_handle_signal(int sig, siginfo_t *si, void *) {
    handle_signal_internal(sig, si);
}

void dfhack_crashlog_handle_terminate() {
    handle_signal_internal(0, nullptr);
}

std::string signal_name(int sig) {
    switch (sig) {
        case SIGINT:
            return "SIGINT";
        case SIGILL:
            return "SIGILL";
        case SIGABRT:
            return "SIGABRT";
        case SIGFPE:
            return "SIGFPE";
        case SIGSEGV:
            return "SIGSEGV";
        case SIGBUS:
            return "SIGBUS";
        case SIGTERM:
            return "SIGTERM";
    }
    return "";
}

std::string signal_code_name(int sig, int code) {
    // si_code values are only unique within their signal, so key on both.
    switch (sig) {
        case SIGSEGV:
            switch (code) {
                case SEGV_MAPERR: return "SEGV_MAPERR";
                case SEGV_ACCERR: return "SEGV_ACCERR";
                case SEGV_BNDERR: return "SEGV_BNDERR";
            }
            break;
        case SIGBUS:
            switch (code) {
                case BUS_ADRALN: return "BUS_ADRALN";
                case BUS_ADRERR: return "BUS_ADRERR";
            }
            break;
        case SIGILL:
            switch (code) {
                case ILL_ILLOPC: return "ILL_ILLOPC";
                case ILL_PRVOPC: return "ILL_PRVOPC";
            }
            break;
        case SIGFPE:
            switch (code) {
                case FPE_INTDIV: return "FPE_INTDIV";
                case FPE_FLTDIV: return "FPE_FLTDIV";
            }
            break;
    }
    return "";
}

std::filesystem::path get_crashlog_path() {
    std::time_t time = std::time(nullptr);
    std::tm* tm = std::localtime(&time);

    std::string timestamp = "unknown";
    if (tm) {
        char stamp[64];
        std::size_t out = strftime(&stamp[0], 63, "%Y-%m-%d-%H-%M-%S", tm);
        if (out != 0)
            timestamp = stamp;
    }

    std::filesystem::path dir = "crashlog";
    std::error_code err;
    std::filesystem::create_directories(dir, err);

    std::filesystem::path log_path = dir / ("crash_" + timestamp + ".txt");
    return log_path;
}

// ---------------------------------------------------------------------------
// Lua crash diagnostics
//
// These run on the dedicated crashlog thread in normal (non-signal) context,
// after the native backtrace has already been written and flushed. The Lua
// state may itself be corrupt - that is often *why* we crashed - so the
// inspection is bounded by a fault guard: a segfault while tracing only
// loses the Lua section of the report.
// ---------------------------------------------------------------------------

volatile sig_atomic_t lua_guard_armed = 0;
sigjmp_buf lua_guard_jmp;
pid_t crashlog_thread_id = 0;
const int guarded_signals[3] = {SIGSEGV, SIGBUS, SIGILL};
struct sigaction saved_actions[3];

// Runs as a temporary signal disposition while the Lua state is inspected.
// Faults on the crashlog thread jump back to the guard; a fault on any other
// thread re-raises with the default disposition so the process still dies
// with its original signal.
extern "C" void lua_fault_guard(int sig, siginfo_t *, void *) {
    if (lua_guard_armed && (pid_t)syscall(SYS_gettid) == crashlog_thread_id)
        siglongjmp(lua_guard_jmp, sig);
    signal(sig, SIG_DFL);
    raise(sig);
}

bool in_lua_image(void *addr) {
    Dl_info di;
    if (!dladdr(addr, &di))
        return false;
    if (lua_image_base && di.dli_fbase == lua_image_base)
        return true;
    if (di.dli_fname) {
        const char *base = std::strrchr(di.dli_fname, '/');
        base = base ? base + 1 : di.dli_fname;
        if (std::strstr(base, "lua53") || std::strstr(base, "liblua"))
            return true;
    }
    return false;
}

bool state_has_active_call(lua_State *L) {
    return L && L->ci != &L->base_ci;
}

// Emits the Lua section of the crashlog. Must only be called between
// arming and disarming the fault guard.
void inspect_lua_state(std::ofstream &crashlog, lua_State *L) {
    if (!L)
        return;

    // Best-effort lock. The faulting thread is frozen inside its signal
    // handler and usually still holds the state mutex, so trylock typically
    // fails and we walk the state read-only - its only writer is suspended
    // anyway. If we do acquire it, no other thread is inside the VM.
    // Mirror of struct lua_extra_state in depends/lua/include/dfhack_llimits.h:
    // the extraspace slot holds a pointer to the state's recursive mutex.
    bool locked = false;
    auto *mutex = *reinterpret_cast<pthread_mutex_t**>(lua_getextraspace(L));
    if (mutex)
        locked = pthread_mutex_trylock(mutex) == 0;
    if (!locked)
        crashlog << "could not acquire Lua state lock; "
                    "the following is a best-effort snapshot\n";

    if (!state_has_active_call(L)) {
        crashlog << "lua_State " << (const void*)L << ": no active call\n";
    } else {
        // A corrupted CallInfo chain (e.g. a cycle) would send
        // luaL_traceback into an unbounded walk, which the fault guard
        // cannot interrupt. Bound the chain first.
        const CallInfo *ci = L->ci;
        int depth = 0;
        while (ci && ci != &L->base_ci && ++depth <= 65536)
            ci = ci->previous;
        if (ci != &L->base_ci) {
            crashlog << "lua_State " << (const void*)L
                     << ": corrupted CallInfo chain; not safe to trace\n";
        } else {
            crashlog << "lua_State " << (const void*)L << " traceback:\n";
            luaL_traceback(L, L, "crash while Lua was executing", 0);
            const char *tb = lua_tostring(L, -1);
            crashlog << (tb ? tb : "(traceback unavailable)") << "\n";
            lua_pop(L, 1);
        }
    }

    if (locked)
        pthread_mutex_unlock(mutex);
}

// Writes the Lua diagnostics section, guarded against faults in corrupted
// interpreter metadata. Fault guard handlers are installed for the duration
// of the inspection and the original dispositions are restored afterwards.
void write_lua_context(std::ofstream &crashlog) {
    bool lua_on_stack = false;
    for (int i = 0; i < crash_info.backtrace_entries; i++) {
        if (in_lua_image(crash_info.backtrace[i])) {
            lua_on_stack = true;
            break;
        }
    }

    crashlog << "\nliblua53 on native stack: " << (lua_on_stack ? "yes" : "no") << "\n";
    crashlog << "thread-local active lua_State: "
             << (const void*)crash_info.active_lua_state << "\n";

    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = lua_fault_guard;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    for (int i = 0; i < 3; i++)
        sigaction(guarded_signals[i], &sa, &saved_actions[i]);
    crashlog_thread_id = (pid_t)syscall(SYS_gettid);

    // Everything that dereferences the Lua state or Core singleton lives
    // inside the guard: either may be pointing at corrupt memory.
    int fault = sigsetjmp(lua_guard_jmp, 1);
    if (fault == 0) {
        lua_guard_armed = 1;

        lua_State *active = crash_info.active_lua_state;
        lua_State *main_state = nullptr;
        if (!DFHack::Core::noInstance())
            main_state = DFHack::Core::getInstance().getLuaState(true);

        if (!lua_on_stack && !active && !state_has_active_call(main_state)) {
            crashlog << "no Lua involvement detected\n";
        } else {
            inspect_lua_state(crashlog, active ? active : main_state);
            if (active && main_state && active != main_state &&
                state_has_active_call(main_state))
                inspect_lua_state(crashlog, main_state);
        }
        lua_guard_armed = 0;
    } else {
        crashlog << "(inspecting the Lua state faulted: "
                 << signal_name(fault) << ")\n";
    }

    for (int i = 0; i < 3; i++)
        sigaction(guarded_signals[i], &saved_actions[i], nullptr);
}

void dfhack_save_crashlog() {
    char** backtrace_strings = backtrace_symbols(crash_info.backtrace, crash_info.backtrace_entries);
    try {
        std::filesystem::path crashlog_path = get_crashlog_path();
        std::ofstream crashlog(crashlog_path);

        crashlog << "Dwarf Fortress Linux has crashed!" << "\n";
        crashlog << "Dwarf Fortress Version " << DFHack::Version::df_version() << "\n";
        crashlog << "DFHack Version " << DFHack::Version::dfhack_version() << "\n\n";

        std::string signal = signal_name(crash_info.signal);
        if (!signal.empty()) {
            crashlog << "Signal " << signal;
            std::string code = signal_code_name(crash_info.signal, crash_info.sig_code);
            if (!code.empty())
                crashlog << " (" << code << ")";
            else if (crash_info.sig_code)
                crashlog << " (code " << crash_info.sig_code << ")";
            // si_addr is only meaningful for faults (a null deref reads as
            // (nil), which is itself diagnostic).
            if (crash_info.signal == SIGSEGV || crash_info.signal == SIGBUS ||
                crash_info.signal == SIGILL || crash_info.signal == SIGFPE)
                crashlog << " at address " << crash_info.fault_address;
            crashlog << " on thread " << crash_info.thread_id << "\n";
        }

        if (crash_info.backtrace_entries >= 2 && backtrace_strings != nullptr) {
            // Skip the first backtrace entry as it will always be dfhack_crashlog_handle_(signal|terminate)
            for (int i = 1; i < crash_info.backtrace_entries; i++) {
                crashlog << i - 1 << "> " << backtrace_strings[i] << "\n";
            }
        } else {
            // Make it clear if no relevant backtrace was able to be obtained
            crashlog << "Failed to obtain relevant backtrace\n";
        }

        // Make sure the native report is on disk before touching the Lua
        // state, which may itself be corrupt.
        crashlog.flush();
        write_lua_context(crashlog);
    } catch (...) {}

    free(backtrace_strings);
}

void dfhack_crashlog_thread() {
    // Wait for activation signal
    flag_wait(crashlog_ready);
    if (shutdown.load()) // Shutting down gracefully, end thread.
        return;

    dfhack_save_crashlog();
    signal_crashlog_complete();
    std::quick_exit(1);
}

std::terminate_handler term_handler = nullptr;

const int desired_signals[4] = {SIGSEGV,SIGBUS,SIGILL,SIGABRT};
namespace DFHack {
    void dfhack_crashlog_init() {
        // Initialize eventfd flag
        crashlog_complete = eventfd(0, EFD_CLOEXEC);

        crashlog_thread = std::thread(dfhack_crashlog_thread);

        struct sigaction sa;
        std::memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = dfhack_crashlog_handle_signal;
        sa.sa_flags = SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
        for (int signal : desired_signals) {
            sigaction(signal, &sa, nullptr);
        }
        term_handler = std::set_terminate(dfhack_crashlog_handle_terminate);

        // Symbols used by the signal handler and the crashlog thread must be
        // resolved ahead of time - lazy binding cannot run in the crash path.
        // backtrace is AsyncSignal-Unsafe due to dynamic loading of libgcc_s.
        // Using it here ensures it is loaded before use in the signal handler.
        [[maybe_unused]] int _ = backtrace(crash_info.backtrace, 1);
        [[maybe_unused]] lua_State *s = DFHack::Crashlog::active_state();
        [[maybe_unused]] pid_t t = (pid_t)syscall(SYS_gettid);
        Dl_info di;
        if (dladdr((void*)&lua_close, &di))
            lua_image_base = di.dli_fbase;
    }

    void dfhack_crashlog_shutdown() {
        shutdown.exchange(true);
        for (int signal : desired_signals) {
            std::signal(signal, SIG_DFL);
        }
        std::set_terminate(term_handler);

        // Shutdown the crashlog thread.
        flag_set(crashlog_ready);
        crashlog_thread.join();

        // If the signal handler is somehow running whilst here, let it terminate
        signal_crashlog_complete();
        if (crashlog_complete != -1)
            close(crashlog_complete); // Close fd
        return;
    }
}
