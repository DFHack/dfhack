// SPDX-License-Identifier: Zlib
#include <condition_variable>
#include <mutex>
#include <utility>

#include "RenderThreadTask.h"

namespace DFHack {

struct RenderThreadTask::State {
    enum class Phase { pending, running, cancelling, finished };
    mutable std::mutex mutex;
    std::condition_variable changed;
    std::function<void()> callback;
    Phase phase = Phase::pending;
    bool succeeded = false;
};

RenderThreadTask::RenderThreadTask(std::function<void()> callback) : state(std::make_unique<State>()) {
    state->callback = std::move(callback);
}

RenderThreadTask::~RenderThreadTask() = default;

bool RenderThreadTask::cancel() {
    std::function<void()> discarded;
    {
        std::lock_guard lock(state->mutex);
        if (state->phase != State::Phase::pending)
            return false;
        state->phase = State::Phase::cancelling;
        discarded = std::move(state->callback);
        state->callback = {};
    }
    // Captures can have nontrivial destructors. Destroy them outside the lock,
    // and do not publish completion while any plugin-owned destructor remains.
    discarded = {};
    {
        std::lock_guard lock(state->mutex);
        state->phase = State::Phase::finished;
    }
    state->changed.notify_all();
    return true;
}

void RenderThreadTask::run() {
    std::function<void()> callback;
    {
        std::lock_guard lock(state->mutex);
        if (state->phase != State::Phase::pending)
            return;
        state->phase = State::Phase::running;
        callback = std::move(state->callback);
        state->callback = {};
    }
    bool success = false;
    try {
        callback();
        success = true;
    } catch (...) {
        // Do not retain an exception_ptr: an exception's destructor might also
        // reside in the plugin. The caller can inspect succeeded() instead.
    }
    callback = {};
    {
        std::lock_guard lock(state->mutex);
        state->succeeded = success;
        state->phase = State::Phase::finished;
    }
    state->changed.notify_all();
}

bool RenderThreadTask::finished() const {
    std::lock_guard lock(state->mutex);
    return state->phase == State::Phase::finished;
}

bool RenderThreadTask::succeeded() const {
    std::lock_guard lock(state->mutex);
    return state->phase == State::Phase::finished && state->succeeded;
}

bool RenderThreadTask::waitFor(std::chrono::milliseconds timeout) const {
    std::unique_lock lock(state->mutex);
    return state->changed.wait_for(lock, timeout, [this] { return state->phase == State::Phase::finished; });
}

std::shared_ptr<RenderThreadTask> queueOnRenderThread(std::function<void()> callback) {
    auto task = std::shared_ptr<RenderThreadTask>(new RenderThreadTask(std::move(callback)));
    // This wrapper's std::function manager is compiled into core, not into the
    // caller's module. A cancelled/finished task can safely remain in the queue.
    runOnRenderThread([task] { task->run(); });
    return task;
}

} // namespace DFHack
