/**
Copyright © 2018 Pauli <suokkos@gmail.com>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
   not claim that you wrote the original software. If you use this
   software in a product, an acknowledgment in the product
   documentation would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
   must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
   distribution.
 */
#define _POSIX_C_SOURCE 200809L
#include "Core.h"

#include "Debug.h"
#include "DebugManager.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <thread>

#ifdef _MSC_VER
static tm* localtime_r(const time_t* time, tm* result)
{
    localtime_s(result, time);
    return result;
}
#endif

namespace DFHack {
DBG_DECLARE(core,debug);

void DebugManager::registerCategory(DebugCategory& cat)
{
    DEBUG(debug) << "register DebugCategory '" << cat.category()
        << "' from '" << cat.plugin()
        << "' allowed " << cat.allowed() << std::endl;
    std::lock_guard<std::mutex> guard(access_mutex_);
    push_back(&cat);
    categorySignal(CAT_ADD, cat);
}

void DebugManager::unregisterCategory(DebugCategory& cat)
{
    DEBUG(debug) << "unregister DebugCategory '" << cat.category()
        << "' from '" << cat.plugin()
        << "' allowed " << cat.allowed() << std::endl;
    std::lock_guard<std::mutex> guard(access_mutex_);
    auto iter = std::find(begin(), end(), &cat);
    std::swap(*iter, back());
    pop_back();
    categorySignal(CAT_REMOVE, cat);
}

DebugRegisterBase::DebugRegisterBase(DebugCategory* cat)
{
    // Make sure Core lives at least as long any DebugCategory to
    // allow debug prints until all Debugcategories has been destructed
    Core::getInstance();
    DebugManager::getInstance().registerCategory(*cat);
}

void DebugRegisterBase::unregister(DebugCategory* cat)
{
    DebugManager::getInstance().unregisterCategory(*cat);
}

static color_value selectColor(const DebugCategory::level msgLevel)
{
    switch(msgLevel) {
    case DebugCategory::LTRACE:
        return COLOR_BROWN;
    case DebugCategory::LDEBUG:
        return COLOR_LIGHTBLUE;
    case DebugCategory::LWARNING:
        return COLOR_YELLOW;
    case DebugCategory::LERROR:
        return COLOR_LIGHTRED;
    case DebugCategory::LINFO:
    default:
        return COLOR_RESET;
    }
}

#if __GNUC__
// Allow gcc to optimize tls access. It also makes sure initialized is done as
// early as possible. The early initialization helps to give threads same ids as
// gdb shows.
#define EXEC_ATTR __attribute__((tls_model("initial-exec")))
#else
#define EXEC_ATTR
#endif

namespace {
static std::atomic<uint32_t> nextId{0};
static EXEC_ATTR thread_local uint32_t thread_id{nextId.fetch_add(1)+1};
}

DebugCategory::ostream_proxy_prefix::ostream_proxy_prefix(
        const DebugCategory& cat,
        color_ostream& target,
        const DebugCategory::level msgLevel) :
    color_ostream_proxy(target)
{
    DebugManager &dm = DebugManager::getInstance();
    const DebugManager::HeaderConfig &config = dm.getHeaderConfig();

    color(selectColor(msgLevel));

    bool has_header = false;
    if (config.timestamp) {
        has_header = true;
        auto now = std::chrono::system_clock::now();
        tm local{};
        //! \todo c++ 2020 will have std::chrono::to_stream(fmt, system_clock::now())
        //! but none implements it yet.
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        // Output time in format %02H:%02M:%02S.%03ms
#if __GNUC__ < 5
        // Fallback for gcc 4
        char buffer[32];
        size_t sz = strftime(buffer, sizeof(buffer)/sizeof(buffer[0]),
                             "%T", localtime_r(&now_c, &local));
        *this << (sz > 0 ? buffer : "HH:MM:SS");
#else
        *this << std::put_time(localtime_r(&now_c, &local),"%T");
#endif
        if (config.timestamp_ms) {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) % 1000;
            *this << '.' << std::setfill('0') << std::setw(3) << ms.count();
        }
        *this << ':';
    }
    if (config.thread_id) {
        has_header = true;
        // Thread id is allocated in the thread creation order to a thread_local
        // variable
        *this << 't' << thread_id << ':';
    }
    if (config.plugin) {
        has_header = true;
        *this << cat.plugin() << ':';
    }
    if (config.category) {
        has_header = true;
        *this << cat.category() << ':';
    }
    // It would be easy to pass __FILE__ and __LINE__ from the logging macros
    // and include that information as well, if we want to.

    if (has_header) {
        *this << ' ';
    }
}


DebugCategory::level DebugCategory::allowed() const noexcept
{
    return allowed_.load(std::memory_order_relaxed);
}

void DebugCategory::allowed(DebugCategory::level value) noexcept
{
    level old = allowed_.exchange(value, std::memory_order_relaxed);
    if (old == value)
        return;
    TRACE(debug) << "modify DebugCategory '" << category()
        << "' from '" << plugin()
        << "' allowed " << value << std::endl;
    auto& manager = DebugManager::getInstance();
    manager.categorySignal(DebugManager::CAT_MODIFIED, *this);
}

DebugCategory::cstring_ref DebugCategory::category() const noexcept
{
    return category_;
}

DebugCategory::cstring_ref DebugCategory::plugin() const noexcept
{
    return plugin_;
}

#if __cplusplus < 201703L && __cpp_lib_atomic_is_always_lock_free < 201603
//! C++17 has std::atomic::is_always_lock_free for static_assert. Older
//! standards only provide runtime checks if an atomic type is lock free
struct failIfEnumAtomicIsNotLockFree {
    failIfEnumAtomicIsNotLockFree() {
        std::atomic<DebugCategory::level> test(DebugCategory::LINFO);
        if (test.is_lock_free())
            return;
        std::cerr << __FILE__ << ':' << __LINE__
            << ": error: std::atomic<DebugCategory::level> should be lock free. Your compiler reports the atomic requires runtime locks. Either you are using a very old CPU or we need to change code to use integer atomic type." << std::endl;
        std::abort();
    }
} failIfEnumAtomicIsNotLockFree;
#endif

}
