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
#include "Core.h"

#include "Debug.h"
#include "DebugManager.h"

#include <chrono>
#include <iomanip>
#include <thread>

namespace DFHack {

DBG_DECLARE(core,debug);

void DebugManager::registerCategory(DebugCategory& cat)
{
    DEBUG(debug) << "register DebugCategory '" << cat.category() << "' from '" << cat.plugin() << "'\n";
    std::lock_guard<std::mutex> guard(access_mutex_);
    push_back(&cat);
    categorySignal.emit(CAT_ADD, cat);
}

void DebugManager::unregisterCategory(DebugCategory& cat)
{
    DEBUG(debug) << "unregister DebugCategory '" << cat.category() << "' from '" << cat.plugin() << "'\n";
    std::lock_guard<std::mutex> guard(access_mutex_);
    auto iter = std::find(begin(), end(), &cat);
    std::swap(*iter, back());
    pop_back();
    categorySignal.emit(CAT_REMOVE, cat);
}

DebugCategory::DebugCategory(DebugCategory::cstring_ref plugin,
        DebugCategory::cstring_ref category,
        DebugCategory::level l) :
    allowed_{l},
    plugin_{plugin},
    category_{category}
{
    // Make sure Core lives at least as long any DebugCategory to
    // allow debug prints until all Debugcategories has been destructed
    Core::getInstance();
    DebugManager::getInstance().registerCategory(*this);
}

DebugCategory::~DebugCategory()
{
    DebugManager::getInstance().unregisterCategory(*this);
}

static color_value selectColor(const DebugCategory::level msgLevel)
{
    switch(msgLevel) {
    case DebugCategory::LTRACE:
        return COLOR_GREY;
    case DebugCategory::LDEBUG:
        return COLOR_LIGHTBLUE;
    case DebugCategory::LINFO:
        return COLOR_CYAN;
    case DebugCategory::LWARNING:
        return COLOR_YELLOW;
    case DebugCategory::LERROR:
        return COLOR_LIGHTRED;
    }
    return COLOR_WHITE;
}

namespace {
static std::atomic<uint32_t> nextId{0};
static thread_local uint32_t thread_id{nextId.fetch_add(1)};
}

void DebugCategory::beginLine(const DebugCategory::level msgLevel,
        color_ostream& out) const
{
    out.color(selectColor(msgLevel));
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    // Output time in format %02H:%02M:%02S.%03ms
    out << std::put_time(std::localtime(&now_c),"%T.")
        << std::setfill('0') << std::setw(3) << ms.count()
        // Thread id is allocated in the thread creation order to a thread_local
        // variable
        << ":t" << thread_id
        // Output plugin and category names to make it easier to locate where
        // the message is coming. It would be easy replaces these with __FILE__
        // and __LINE__ passed from the macro if that would be preferred prefix.
        << ':' << plugin_ << ':' << category_ << ": ";
}

color_ostream_proxy DebugCategory::getStream( const DebugCategory::level msgLevel ) const
{
    color_ostream_proxy rv(Core::getInstance().getConsole());
    beginLine(msgLevel, rv);
    return rv;
}

color_ostream_proxy DebugCategory::getStream( const DebugCategory::level msgLevel,
        color_ostream& out) const
{
    color_ostream_proxy rv(out);
    beginLine(msgLevel, rv);
    return rv;
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
    auto& manager = DebugManager::getInstance();
    manager.categorySignal.emit(DebugManager::CAT_MODIFIED, *this);
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
        std::atomic<DebugCategory::level> test;
        if (test.is_lock_free())
            return;
        std::cerr << __FILE__ << ':' << __LINE__
            << ": error: std::atomic<DebugCategory::level> should be lock free. Your compiler reports the atomic requires runtime locks. Either you are using a very old CPU or we need to change code to use integer atomic type." << std::endl;
        std::abort();
    }
} failIfEnumAtomicIsNotLockFree;
#endif

}
