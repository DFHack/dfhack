// SPDX-License-Identifier: Zlib
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include "RenderThreadTask.h"

// Only the transport is substituted. The task/cancellation/fence implementation
// is library/RenderThreadTask.cpp, compiled as a separate translation unit.
static std::vector<std::function<void()>> queue;
namespace DFHack {
void runOnRenderThread(std::function<void()> callback) {
    queue.push_back(std::move(callback));
}
} // namespace DFHack

using namespace std::chrono_literals;

struct CallbackResource {
    std::atomic_bool &destroyed;
    explicit CallbackResource(std::atomic_bool &flag) : destroyed(flag) {}
    ~CallbackResource() {
        destroyed = true;
    }
};

int main() {
    std::atomic_bool destroyed{false};
    int calls = 0;
    auto resource = std::make_shared<CallbackResource>(destroyed);
    auto cancelled = DFHack::queueOnRenderThread([resource, &calls] { ++calls; });
    resource.reset();
    assert(!cancelled->finished());
    assert(cancelled->cancel());
    assert(cancelled->finished() && destroyed && !cancelled->succeeded());
    // The core queue wrapper is still alive. Executing/destroying it must not
    // invoke any cancelled plugin code or retain its captured resource.
    queue.front()();
    queue.clear();
    assert(calls == 0);

    destroyed = false;
    resource = std::make_shared<CallbackResource>(destroyed);
    auto finished = DFHack::queueOnRenderThread([resource, &calls] { ++calls; });
    resource.reset();
    queue.front()();
    assert(finished->finished() && finished->succeeded() && destroyed && calls == 1);
    queue.clear();

    std::mutex mutex;
    std::condition_variable changed;
    bool entered = false, release = false;
    auto running = DFHack::queueOnRenderThread([&] {
        std::unique_lock lock(mutex);
        entered = true;
        changed.notify_one();
        changed.wait(lock, [&] { return release; });
    });
    std::thread render([] { queue.front()(); });
    {
        std::unique_lock lock(mutex);
        changed.wait(lock, [&] { return entered; });
    }
    assert(!running->cancel());
    assert(!running->waitFor(1ms));
    {
        std::lock_guard lock(mutex);
        release = true;
    }
    changed.notify_one();
    assert(running->waitFor(1000ms) && running->succeeded());
    render.join();
    queue.clear();

    // Publishing completion before capture destruction is a use-after-unload
    // bug even when callback() already returned. Block a capture's destructor
    // to verify that the fence covers that interval too.
    struct BlockingDestructor {
        std::mutex &mutex;
        std::condition_variable &changed;
        bool &entered, &release;
        ~BlockingDestructor() {
            std::unique_lock lock(mutex);
            entered = true;
            changed.notify_one();
            changed.wait(lock, [&] { return release; });
        }
    };
    entered = release = false;
    auto blocking =
        std::shared_ptr<BlockingDestructor>(new BlockingDestructor{mutex, changed, entered, release});
    auto destruction = DFHack::queueOnRenderThread([blocking] {});
    blocking.reset();
    std::thread render_destructor([] { queue.front()(); });
    {
        std::unique_lock lock(mutex);
        changed.wait(lock, [&] { return entered; });
    }
    assert(!destruction->finished() && !destruction->waitFor(1ms));
    {
        std::lock_guard lock(mutex);
        release = true;
    }
    changed.notify_one();
    assert(destruction->waitFor(1000ms) && destruction->succeeded());
    render_destructor.join();
    queue.clear();

    entered = release = false;
    blocking = std::shared_ptr<BlockingDestructor>(new BlockingDestructor{mutex, changed, entered, release});
    auto cancelling = DFHack::queueOnRenderThread([blocking] {});
    blocking.reset();
    std::thread cancel([&] { assert(cancelling->cancel()); });
    {
        std::unique_lock lock(mutex);
        changed.wait(lock, [&] { return entered; });
    }
    queue.front()(); // cancellation owns the payload; the queue must skip it
    assert(!cancelling->finished());
    {
        std::lock_guard lock(mutex);
        release = true;
    }
    changed.notify_one();
    cancel.join();
    assert(cancelling->finished() && !cancelling->succeeded());
    queue.clear();

    auto failed = DFHack::queueOnRenderThread([] { throw std::runtime_error("expected test failure"); });
    queue.front()();
    assert(failed->finished() && !failed->succeeded());
    queue.clear();
    std::cout << "PASS: pending cancellation, capture destruction, running-task wait, "
                 "post-cancel queue safety, completion fence, and exception cleanup\n";
}
