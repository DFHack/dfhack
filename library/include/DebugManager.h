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

#pragma once

#include "Export.h"
#include "Signal.hpp"

#include <mutex>
#include <vector>

namespace DFHack {

/*! \file DebugManager.h
 * Expose an simple interface to runtime debug output filtering. The management
 * interface is separated from output interface because output is required in
 * many places while management is expected to be required only in a few places.
 */

class DebugCategory;

/*!
 * \brief Container holding all registered runtime debug categories
 * Singleton DebugManager is a minor extension to std::vector that allows signal
 * callbacks to be attached from plotinfo code that manages.
 *
 * To avoid parallel plugin unload causing issues access to DebugManager must be
 * protected by mutex. The access mutex will be taken when
 * DFHack::DebugCategory::~DebugCategory performs unregister calls to
 * DFHack::DebugManager. The mutex will protect from memory disappearing while
 * plotinfo code is accessing or changing the runtime state.
 *
 * Signal emitting happens from a locked contexts. Taking the
 * DFHack::DebugManager::access_mutex_ in a signal callback will results to a
 * deadlock.
 *
 * The interface is extremely simple but enough to implement persistent filter
 * states and runtime configuration code in a plugin.
 */
class DFHACK_EXPORT DebugManager : public std::vector<DebugCategory*> {
public:
    friend class DebugRegisterBase;

    //! access_mutex_ protects all readers and writers to DFHack::DebugManager
    std::mutex access_mutex_;

    //! Different signals that all will be routed to
    //! DebugManager::categorySignal
    enum signalType {
        CAT_ADD,
        CAT_REMOVE,
        CAT_MODIFIED,
    };

    //! Log message header configuration, controlled via the debug plugin
    struct HeaderConfig {
        bool timestamp = false;    // Hour, minute, and seconds
        bool timestamp_ms = false; // only used if timestamp == true
        bool thread_id = false;
        bool plugin = false;
        bool category = false;
    };

    //! type to help access signal features like Connection and BlockGuard
    using categorySignal_t = Signal<void (signalType, DebugCategory&)>;

    /*!
     * Signal object where callbacks can be connected. Connecting to a class
     * method can use a lambda wrapper to the capture object pointer and correctly
     * call required method.
     *
     * Signal is internally serialized allowing multiple threads call it
     * freely.
     */
    categorySignal_t categorySignal;

    //! Get the singleton object
    static DebugManager& getInstance() {
        static DebugManager instance;
        return instance;
    }

    // don't bother to protect these with mutexes. we don't want to pay the
    // performance cost of locking on every log message, and the consequences
    // of race conditions are minimal.
    const HeaderConfig & getHeaderConfig() const {
        return headerConfig;
    }
    void setHeaderConfig(const HeaderConfig &config) {
        headerConfig = config;
    }

    //! Prevent copies
    DebugManager(const DebugManager&) = delete;
    //! Prevent copies
    DebugManager(DebugManager&&) = delete;
    //! Prevent copies
    DebugManager& operator=(DebugManager) = delete;
    //! Prevent copies
    DebugManager& operator=(DebugManager&&) = delete;
protected:
    DebugManager() = default;

    HeaderConfig headerConfig;

    //! Helper for automatic category registering and signaling
    void registerCategory(DebugCategory &);
    //! Helper for automatic category unregistering and signaling
    void unregisterCategory(DebugCategory &);
private:
};
}
