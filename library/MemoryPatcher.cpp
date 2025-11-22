#include "Core.h"
#include "MemoryPatcher.h"

namespace DFHack
{
    MemoryPatcher::MemoryPatcher(Process* p_) : p(p_)
    {
        if (!p)
            p = Core::getInstance().p.get();
    }

    MemoryPatcher::~MemoryPatcher()
    {
        close();
    }

    bool MemoryPatcher::verifyAccess(void* target, size_t count, bool write)
    {
        auto* sptr = (uint8_t*)target;
        uint8_t* eptr = sptr + count;

        // Find the valid memory ranges
        if (ranges.empty())
            p->getMemRanges(ranges);

        // Find the ranges that this area spans
        unsigned start = 0;
        while (start < ranges.size() && ranges[start].end <= sptr)
            start++;
        if (start >= ranges.size() || ranges[start].start > sptr)
            return false;

        unsigned end = start + 1;
        while (end < ranges.size() && ranges[end].start < eptr)
        {
            if (ranges[end].start != ranges[end - 1].end)
                return false;
            end++;
        }
        if (ranges[end - 1].end < eptr)
            return false;

        // Verify current permissions
        for (unsigned i = start; i < end; i++)
            if (!ranges[i].valid || !(ranges[i].read || ranges[i].execute) || ranges[i].shared)
                return false;

        // Apply writable permissions & update
        for (unsigned i = start; i < end; i++)
        {
            auto& perms = ranges[i];
            if ((perms.write || !write) && perms.read)
                continue;

            save.push_back(perms);
            perms.write = perms.read = true;
            if (!p->setPermissions(perms, perms))
                return false;
        }

        return true;
    }

    bool MemoryPatcher::write(void* target, const void* src, size_t size)
    {
        if (!makeWritable(target, size))
            return false;

        memmove(target, src, size);

        p->flushCache(target, size);
        return true;
    }

    void MemoryPatcher::close()
    {
        for (size_t i = 0; i < save.size(); i++)
            p->setPermissions(save[i], save[i]);

        save.clear();
        ranges.clear();
    };
}
