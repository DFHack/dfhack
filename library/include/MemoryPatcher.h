#pragma once

#include <vector>
#include <cstddef>
#include "MemAccess.h"

namespace DFHack {

    class Process;

    class MemoryPatcher {
    public:
        explicit MemoryPatcher(Process *p_ = nullptr);
        ~MemoryPatcher();

        // Ensure the target memory region is accessible and optionally writable.
        // If `write` is true this will attempt to make the pages writable.
        bool verifyAccess(void *target, size_t count, bool write);

        // Write `size` bytes from `src` to `target`. Returns true on success.
        bool write(void *target, const void *src, size_t size);

        // Restore any modified permissions and clear internal state.
        void close();

        bool makeWritable(void* target, size_t size)
        {
            return verifyAccess(target, size, true);
        }
    private:
        Process* p;
        std::vector<t_memrange> ranges, save;
    };

}
