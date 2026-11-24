// SPDX-License-Identifier: Zlib
#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include "Export.h"

namespace DFHack {

// Fire-and-forget work must not refer to an unloadable plugin. Use the task
// handle below when callback code or captured objects belong to a plugin.
DFHACK_EXPORT void runOnRenderThread(std::function<void()> callback);

class DFHACK_EXPORT RenderThreadTask;
DFHACK_EXPORT std::shared_ptr<RenderThreadTask> queueOnRenderThread(std::function<void()> callback);

// The implementation, queue wrapper, and callback destruction live in core.
// A finished task contains no callable or capture whose code lives in a plugin.
class DFHACK_EXPORT RenderThreadTask {
public:
    ~RenderThreadTask();
    bool cancel();
    bool finished() const;
    bool succeeded() const;
    // Never wait on the render thread or while holding a core suspend context:
    // the render thread may itself be waiting for simulation to advance.
    bool waitFor(std::chrono::milliseconds timeout) const;

private:
    struct State;
    std::unique_ptr<State> state;
    explicit RenderThreadTask(std::function<void()> callback);
    void run();
    friend DFHACK_EXPORT std::shared_ptr<RenderThreadTask>
    queueOnRenderThread(std::function<void()> callback);
};

} // namespace DFHack
