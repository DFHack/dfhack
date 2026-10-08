/*
 * DFHack crash reporting for Windows.
 *
 * Dwarf Fortress installs its own top-level unhandled exception filter,
 * which produces crashlog.txt (a native stack trace with module/offset
 * annotations). That trace is enough to identify that a crash happened
 * inside the Lua VM, but not what Lua code was running.
 *
 * This module chains an exception filter ahead of DF's (and additionally
 * registers a vectored continue handler in case our filter is itself
 * replaced later). When a fatal exception occurs and the Lua VM was
 * involved - lua53.dll on the native stack, or a DFHack Lua call active
 * on the faulting thread - we write a Lua stack traceback to stderr.log
 * before chaining to DF's filter so that its crashlog is still produced.
 *
 * Everything in the crash path runs in the context of a crashed process,
 * so it uses bare Win32 IO only and is wrapped in SEH so that a secondary
 * fault while reporting simply falls through to DF's handler.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>

#include "Core.h"
#include "Crashlog.h"
#include "DFHackVersion.h"

#include "lua.h"
#include "lauxlib.h"
#include "lstate.h"

namespace {

std::atomic<bool> handler_active{false};
LPTOP_LEVEL_EXCEPTION_FILTER previous_filter = nullptr;
PVOID continue_handler = nullptr;

// image bounds of interest, resolved once at init (before anyone could crash)
struct ImageRange { const char *name; uintptr_t base, end; };
ImageRange df_image = {"Dwarf Fortress.exe", 0, 0};
ImageRange dfhack_image = {"dfhack.dll", 0, 0};
ImageRange lua_image = {"lua53.dll", 0, 0};

void cache_image_range(ImageRange &range, const wchar_t *module_name)
{
    HMODULE m = GetModuleHandleW(module_name);
    if (!m)
        return;
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(m);
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(
        reinterpret_cast<char*>(m) + dos->e_lfanew);
    range.base = reinterpret_cast<uintptr_t>(m);
    range.end = range.base + nt->OptionalHeader.SizeOfImage;
}

const char *image_name_for(uintptr_t addr)
{
    for (const auto &r : {df_image, dfhack_image, lua_image})
        if (r.base && addr >= r.base && addr < r.end)
            return r.name;
    return nullptr;
}

// Minimal output: append to stderr.log with raw Win32 IO so we depend on
// as little process state as possible.
void log_write(HANDLE log, const char *fmt, ...)
{
    char buf[8192];
    va_list args;
    va_start(args, fmt);
    int len = _vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    va_end(args);
    if (len == 0)
        return;
    if (len < 0 || len >= (int)sizeof(buf))
        len = (int)sizeof(buf) - 1;  // truncated output is still worth having
    buf[sizeof(buf) - 1] = '\0';
    DWORD written;
    WriteFile(log, buf, len, &written, nullptr);
}

void log_write_str(HANDLE log, const char *str)
{
    DWORD written;
    WriteFile(log, str, (DWORD)strlen(str), &written, nullptr);
}

// Walk the faulting thread's stack looking for instruction pointers inside
// lua53.dll. Uses RtlVirtualUnwind (driven by .pdata unwind info), which does
// not need dbghelp or symbol files.
bool lua_frames_on_stack(EXCEPTION_POINTERS *ep)
{
    CONTEXT ctx = *ep->ContextRecord;
    for (int i = 0; i < 200; i++)
    {
        if (ctx.Rip >= lua_image.base && ctx.Rip < lua_image.end)
            return true;
        if (!ctx.Rip || (ctx.Rip & 7))
            break;

        DWORD64 image_base = 0;
        PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(ctx.Rip, &image_base, nullptr);
        if (fn)
        {
            PVOID handler_data = nullptr;
            DWORD64 establisher = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, ctx.Rip, fn,
                             &ctx, &handler_data, &establisher, nullptr);
        }
        else
        {
            // leaf function: return address is at [rsp]
            ctx.Rip = *reinterpret_cast<DWORD64*>(ctx.Rsp);
            ctx.Rsp += 8;
        }
    }
    return false;
}

bool state_has_active_call(lua_State *L)
{
    return L && L->ci != &L->base_ci;
}

void write_lua_traceback(HANDLE log, lua_State *L)
{
    if (!L)
        return;

    // Mirror of struct lua_extra_state in depends/lua/include/dfhack_llimits.h:
    // the first extraspace slot holds a pointer to the state's recursive
    // critical section. If the faulting thread holds it already (e.g. the
    // crash is inside a Lua call on this thread), TryEnterCriticalSection
    // recurses and succeeds. If another thread is inside the VM, we note it
    // and walk the state anyway - it is read-only and SEH-guarded, and an
    // approximate trace is better than none.
    bool locked = false;
    CRITICAL_SECTION *cs = nullptr;
    __try
    {
        cs = *reinterpret_cast<CRITICAL_SECTION**>(lua_getextraspace(L));
        if (cs)
            locked = TryEnterCriticalSection(cs);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        cs = nullptr;
    }

    __try
    {
        if (!locked)
            log_write(log, "(could not acquire Lua state lock; "
                           "traceback may be inaccurate)\n");
        if (!state_has_active_call(L))
        {
            log_write(log, "main lua_State %p: no active call\n", (void*)L);
        }
        else
        {
            luaL_traceback(L, L, "crash while Lua was executing", 0);
            const char *tb = lua_tostring(L, -1);
            if (tb)
            {
                log_write(log, "lua_State %p traceback:\n", (void*)L);
                log_write_str(log, tb);
                log_write(log, "\n");
            }
            else
            {
                log_write(log, "lua_State %p: traceback not a string\n", (void*)L);
            }
            lua_pop(L, 1);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        log_write(log, "(lua traceback attempt faulted)\n");
    }

    if (locked)
        LeaveCriticalSection(cs);
}

void report_crash(EXCEPTION_POINTERS *ep)
{
    HANDLE log = CreateFileA("stderr.log", FILE_APPEND_DATA,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                             OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (log == INVALID_HANDLE_VALUE)
        return;

    EXCEPTION_RECORD *er = ep->ExceptionRecord;
    const char *fault_image = image_name_for(
        reinterpret_cast<uintptr_t>(er->ExceptionAddress));
    bool in_lua = lua_frames_on_stack(ep);

    lua_State *active = DFHack::Crashlog::active_state();
    lua_State *main_state = nullptr;
    __try
    {
        if (!DFHack::Core::noInstance())
            main_state = DFHack::Core::getInstance().getLuaState(true);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}

    log_write(log,
        "\n===== DFHack crash report =====\n"
        "DFHack %s, DF %s\n"
        "Exception 0x%08lx at %p (module: %s), thread %lu\n",
        DFHack::Version::dfhack_version(), DFHack::Version::df_version(),
        er->ExceptionCode, er->ExceptionAddress,
        fault_image ? fault_image : "?",
        GetCurrentThreadId());

    if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        er->NumberParameters >= 2)
        log_write(log, "access violation %s address %p\n",
            er->ExceptionInformation[0] ? "writing" : "reading",
            (void*)er->ExceptionInformation[1]);

    log_write(log,
        "lua53.dll on native stack: %s\n"
        "thread-local active lua_State: %p\n",
        in_lua ? "yes" : "no",
        (void*)active);

    if (in_lua || active || state_has_active_call(main_state))
    {
        write_lua_traceback(log, active ? active : main_state);
        if (active && main_state && active != main_state &&
            state_has_active_call(main_state))
            write_lua_traceback(log, main_state);
    }
    else
    {
        log_write(log, "no Lua involvement detected\n");
    }

    log_write(log, "===== end DFHack crash report =====\n\n");
    CloseHandle(log);
}

LONG WINAPI dfhack_unhandled_exception(EXCEPTION_POINTERS *ep)
{
    if (!handler_active.exchange(true))
    {
        __try
        {
            report_crash(ep);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    if (previous_filter)
        return previous_filter(ep);
    return EXCEPTION_CONTINUE_SEARCH;
}

// Runs only if the exception was never handled and DF's own filter (or
// whoever replaced us) declined to handle it either. Produces the same
// report; the handler_active flag prevents double-reporting.
LONG WINAPI dfhack_vectored_continue(EXCEPTION_POINTERS *ep)
{
    if (!handler_active.exchange(true))
    {
        __try
        {
            report_crash(ep);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void chain_top_level_filter()
{
    LPTOP_LEVEL_EXCEPTION_FILTER prev =
        SetUnhandledExceptionFilter(dfhack_unhandled_exception);
    if (prev && prev != dfhack_unhandled_exception)
        previous_filter = prev;
}

}

namespace DFHack {

void dfhack_crashlog_init()
{
    cache_image_range(df_image, nullptr);
    cache_image_range(dfhack_image, L"dfhack.dll");
    cache_image_range(lua_image, L"lua53.dll");

    // Install our top-level filter, chaining to whoever was there before
    // (normally DF's own crash handler). This function is also called again
    // on the first update tick so that a filter DF installs later in
    // startup still ends up behind us in the chain.
    chain_top_level_filter();

    // Fallback path: runs after the top-level filter if the exception was
    // never handled, catching the case where our filter was overwritten.
    if (!continue_handler)
        continue_handler = AddVectoredContinueHandler(0, dfhack_vectored_continue);
}

void dfhack_crashlog_shutdown()
{
    // Probe the current top-level filter by installing ours and looking at
    // what was there. If ours was current, the chain tail is what should be
    // restored; otherwise put back whatever we just displaced.
    LPTOP_LEVEL_EXCEPTION_FILTER cur =
        SetUnhandledExceptionFilter(dfhack_unhandled_exception);
    SetUnhandledExceptionFilter(cur == dfhack_unhandled_exception
                                ? previous_filter : cur);
    if (continue_handler)
    {
        RemoveVectoredContinueHandler(continue_handler);
        continue_handler = nullptr;
    }
}

}
