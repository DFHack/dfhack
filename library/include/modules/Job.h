/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#pragma once
#ifndef CL_MOD_JOB
#define CL_MOD_JOB

#include "Export.h"
#include "Module.h"
#include <ostream>

namespace df
{
    struct job;
    struct job_item;
    struct job_item_filter;
    struct building;
}

namespace DFHack
{
    // Duplicate the job structure. It is not linked into any DF lists.
    DFHACK_EXPORT df::job *cloneJobStruct(df::job *job);

    // Delete a cloned structure.
    DFHACK_EXPORT void deleteJobStruct(df::job *job);

    DFHACK_EXPORT bool operator== (const df::job_item &a, const df::job_item &b);
    DFHACK_EXPORT bool operator== (const df::job &a, const df::job &b);

    DFHACK_EXPORT void printJobDetails(Core *c, df::job *job);

    DFHACK_EXPORT df::building *getJobHolder(df::job *job);

    DFHACK_EXPORT bool linkJobIntoWorld(df::job *job, bool new_id = true);
}
#endif

