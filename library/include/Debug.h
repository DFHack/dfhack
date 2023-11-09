/**
Copyright © 2018 Pauli <suokkos\gmail.com>

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

#pragma once

#include "ColorText.h"

#include <atomic>
#include "Core.h"

namespace DFHack {

/*! \file Debug.h
 * Light weight wrappers for runtime debug output filtering. The idea is to add
 * as little as possible code compared to debug output without filtering. The
 * effect is archived using #TRACE, #DEBUG, #INFO, #WARN and #ERR macros. They
 * "return" color_ostream object or reference that can be then used normally for
 * either printf or stream style debug output.
 *
 * Internally macros do inline filtering check which allows compiler to have a
 * fast path when output is disabled. However, if output is enabled then
 * parameters are evaluated and the printing function is called. The macro setup
 * code will also print a standard header for each log message, including
 * timestamp, plugin name, and category name.
 *
 * \code{.cpp}
 * #include "Debug.h"
 * DBG_DECLARE(myplugin,init);
 *
 * DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
 * {
 *     command_result rv = CR_OK;
 *     DEBUG(init, out).print("initializing\n")
 *     if ((rv = initWork()) != CR_OK) {
 *         ERR(init, out) << "initWork failed with "
 *              << rv << " error code" << std::endl;
 *         return rv;
 *     }
 *     return rv
 * }
 * \endcode
 *
 * The debug print filtering levels can be changed using a debugger. The
 * following gdb example will automatically setup core/init and core/render to
 * trace level when SDL_init is called.
 *
 * \code{.unparsed}
 * break SDL_init
 * commands
 * silent
 * p DFHack::debug::core::debug_init.allowed_ = 0
 * p DFHack::debug::core::debug_render.allowed_ = 0
 * c
 * end
 * \endcode
 *
 */

#ifndef __has_cpp_attribute
#define __has_cpp_attribute(x) 0
#endif

/*!
 * \defgroup debug_branch_prediction Branch prediction helper macros
 * Helper macro tells compiler that debug output branch is unlikely and should
 * be optimized to cold section of the function.
 * \{
 */
#if __cplusplus >= 202000L || __has_cpp_attribute(likely)
// c++20 will have standard branch prediction hint attributes
#define likely(x) (x) [[likely]]
#define unlikely(x) (x) [[unlikely]]
#elif defined(__GNUC__)
// gcc has builtin functions that give hints to the branch prediction
#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif
//! \}

#ifdef NDEBUG
/*!
 * This is here so we can reduce minimum compiled in debug levels if NDEBUG is
 * defined if we want to. If LTRACE slows down the binary, we can change it to
 * LDEBUG (but no lower than that so users can usefully increase logging levels
 *for bug reports).
 */
#define DBG_FILTER DFHack::DebugCategory::LTRACE
#else
//! Set default compiled in debug levels to include all prints
#define DBG_FILTER DFHack::DebugCategory::LTRACE
#endif

/*!
 * DebugCategory is used to enable and disable debug messages in runtime.
 * Declaration and definition are handled by #DBG_DECLARE and #DBG_DEFINE
 * macros. Runtime filtering support is handled by #TRACE, #DEBUG, #INFO, #WARN
 * and #ERR macros.
 */
class DFHACK_EXPORT DebugCategory final {
public:
    //! type helper to maybe make it easier to convert to std::string_view when
    //! c++17 can be required.
    using cstring = const char*;
    using cstring_ref = const char*;
    /*!
     * Debug level enum for message filtering
     */
    enum level {
        LTRACE = 0,
        LDEBUG = 1,
        LINFO = 2,
        LWARNING = 3,
        LERROR = 4,
    };

    /*!
     * \param plugin the name of plugin the category belongs to
     * \param category the name of category
     * \param defaultLevel optional default filtering level for the category
     */
    constexpr DebugCategory(cstring_ref plugin,
            cstring_ref category,
            level defaultLevel = LWARNING) noexcept :
        plugin_{plugin},
        category_{category},
        allowed_{defaultLevel}
    {}


    DebugCategory(const DebugCategory&) = delete;
    DebugCategory(DebugCategory&&) = delete;
    DebugCategory& operator=(DebugCategory) = delete;
    DebugCategory& operator=(DebugCategory&&) = delete;

    /*!
     * Used by debug macros to check if message should be printed.
     *
     * It is defined in the header to allow compiler inline it and make disabled
     * state a fast path without function calls.
     *
     * \param msgLevel the debug message level the following print belongs to
     * \return boolean with true indicating that message should be printed
     */
    bool isEnabled(const level msgLevel) const noexcept {
        const uint32_t intLevel = static_cast<uint32_t>(msgLevel);
        // Compile time filtering to allow compiling out debug checks prints
        // from binary.
        return static_cast<uint32_t>(DBG_FILTER) <= intLevel &&
            // Runtime filtering for debug messages
            static_cast<uint32_t>(allowed_.load(std::memory_order_relaxed)) <= intLevel;
    }

    struct DFHACK_EXPORT ostream_proxy_prefix : public color_ostream_proxy {
        ostream_proxy_prefix(const DebugCategory& cat,
                color_ostream& target,
                DebugCategory::level level);
        ~ostream_proxy_prefix() {
            flush();
        }
    };

    /*!
     * Fetch a steam object proxy object for output. It also adds standard
     * message components like time and plugin and category names to the line.
     *
     * User must make sure that the line is terminated with a line end.
     *
     * \param msgLevel Specifies the level which next debug message belongs
     * \return color_ostream_proxy that can be used to print the message
     * \sa DFHack::Core::getConsole()
     */
    ostream_proxy_prefix getStream(const level msgLevel) const
    {
        return {*this,Core::getInstance().getConsole(),msgLevel};
    }
    /*!
     * Add standard message components to existing output stream object to begin
     * a new message line to an shared buffered object.
     *
     * \param msgLevel Specifies the level which next debug message belongs
     * \param target An output stream object where a debug output is printed
     * \return color_ostream reference that was passed as second parameter
     */
    ostream_proxy_prefix getStream(const level msgLevel, color_ostream& target) const
    {
        return {*this,target,msgLevel};
    }

    /*!
     * \brief Allow management code to set a new filtering level
     * Caller must have locked DebugManager::access_mutex_.
     */
    void allowed(level value) noexcept;
    //! Query current filtering level
    level allowed() const noexcept;
    //! Query plugin name
    cstring_ref plugin() const noexcept;
    //! Query category name
    cstring_ref category() const noexcept;
private:

    cstring plugin_;
    cstring category_;
    std::atomic<level> allowed_;
#if __cplusplus >= 201703L || __cpp_lib_atomic_is_always_lock_free >= 201603
    static_assert(std::atomic<level>::is_always_lock_free,
            "std::atomic<level> should be lock free. You are using a very old CPU or code needs to use std::atomic<int>");
#endif
};

/**
 * Handle actual registering wrong template parameter generated pointer
 * calculation.
 */
class DFHACK_EXPORT DebugRegisterBase {
protected:
    DebugRegisterBase(DebugCategory* category);
    void unregister(DebugCategory* category);
};

/**
 * Register DebugCategory to DebugManager
 */
template<DebugCategory* category>
class DebugRegister final : public DebugRegisterBase {
public:
    DebugRegister() :
        DebugRegisterBase{category}
    {}
    ~DebugRegister() {
        unregister(category);
    }
};

#define DBG_NAME(category) debug_ ## category


/*!
 * Declares a debug category. There must be only one declaration per category.
 * Declaration should be in same plugin where it is used. If same category name
 * is used in core and multiple plugins they all are changed with same command
 * unless user explicitly specifies a plugin name.
 *
 * Must be used in one translation unit only.
 *
 * \param plugin the name of plugin where debug category is used
 * \param category the name of category
 * \param level the initial DebugCategory::level filtering level.
 */
#define DBG_DECLARE(plugin,category, ...)                                   \
    namespace debug { namespace plugin {                                    \
        DebugCategory DBG_NAME(category){#plugin,#category,__VA_ARGS__};    \
        DebugRegister<&DBG_NAME(category)> register_ ## category;           \
    } }                                                                     \
    using debug::plugin::DBG_NAME(category)

/*!
 * Can be used to access a shared DBG_DECLARE category. But may not be used from
 * static initializer because translation unit order is undefined.
 *
 * Can be used in shared headers to gain access to one definition from
 * DBG_DECLARE.
 * \param plugin The plugin name that must match DBG_DECLARE
 * \param category The category name that must matnch DBG_DECLARE
 */
#define DBG_EXTERN(plugin,category)                                         \
    namespace debug { namespace plugin {                                    \
        extern DFHack::DebugCategory DBG_NAME(category);                    \
    } }                                                                     \
    using debug::plugin::DBG_NAME(category)

#define DBG_PRINT(category,pred,level,...)                                  \
    if pred(!DFHack::DBG_NAME(category).isEnabled(level))                   \
        ; /* nop fast path when debug category is disabled */               \
    else /* else to allow macro use in if-else branches */                  \
        DFHack::DBG_NAME(category).getStream(level, ## __VA_ARGS__)         \
/* end of DBG_PRINT */

/*!
 * Open a line for trace level debug output if enabled
 *
 * Preferred category for inside loop debug messages or callbacks/methods called
 * multiple times per second. Good example would be render or onUpdate methods.
 *
 * \param category the debug category
 * \param optional the optional second parameter is an existing
 *                 color_ostream_proxy object
 * \return color_ostream object that can be used for stream output
 */
#define TRACE(category, ...) DBG_PRINT(category, likely, \
        DFHack::DebugCategory::LTRACE, ## __VA_ARGS__)

/*!
 * Open a line for debug level debug output if enabled
 *
 * Preferred place to use it would be commonly called functions that don't fall
 * into trace category.
 *
 * \param category the debug category
 * \param optional the optional second parameter is an existing
 *                 color_ostream_proxy object
 * \return color_ostream object that can be used for stream output
 */
#define DEBUG(category, ...) DBG_PRINT(category, likely, \
        DFHack::DebugCategory::LDEBUG, ## __VA_ARGS__)

/*!
 * Open a line for info level debug output if enabled
 *
 * Important debug messages when some rarely changed state changes. Example
 * would be when a debug category filtering level changes.
 *
 * \param category the debug category
 * \param optional the optional second parameter is an existing
 *                 color_ostream_proxy object
 * \return color_ostream object that can be used for stream output
 */
#define INFO(category, ...) DBG_PRINT(category, likely, \
        DFHack::DebugCategory::LINFO, ## __VA_ARGS__)

/*!
 * Open a line for warning level debug output if enabled
 *
 * Warning category is for recoverable errors. This generally signals that
 * something unusual happened but there is code handling the error which should
 * allow df continue running without issues.
 *
 * \param category the debug category
 * \param optional the optional second parameter is an existing
 *                 color_ostream_proxy object
 * \return color_ostream object that can be used for stream output
 */
#define WARN(category, ...) DBG_PRINT(category, unlikely, \
        DFHack::DebugCategory::LWARNING, ## __VA_ARGS__)

/*!
 * Open a line for error level error output if enabled
 *
 * Errors should be printed only for cases where plugin or dfhack can't recover
 * from reported error and it requires manual handling from the user.
 *
 * \param category the debug category
 * \param optional the optional second parameter is an existing
 *                 color_ostream_proxy object
 * \return color_ostream object that can be used for stream output
 */
#define ERR(category, ...) DBG_PRINT(category, unlikely, \
        DFHack::DebugCategory::LERROR, ## __VA_ARGS__)

}
