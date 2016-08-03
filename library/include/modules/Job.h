/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

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
#include "Types.h"

#include <ostream>

#include "DataDefs.h"
#include "df/job_item_ref.h"
#include "df/item_type.h"

namespace df
{
    struct job;
    struct job_item;
    struct job_item_filter;
    struct building;
    struct unit;
}

namespace DFHack
{
    namespace Job {
        // Duplicate the job structure. It is not linked into any DF lists.
        DFHACK_EXPORT df::job *cloneJobStruct(df::job *job, bool keepEverything=false);

        // Delete a cloned structure.
        DFHACK_EXPORT void deleteJobStruct(df::job *job, bool keptEverything=false);

        DFHACK_EXPORT void printItemDetails(color_ostream &out, df::job_item *item, int idx);
        DFHACK_EXPORT void printJobDetails(color_ostream &out, df::job *job);

        DFHACK_EXPORT df::general_ref *getGeneralRef(df::job *job, df::general_ref_type type);
        DFHACK_EXPORT df::specific_ref *getSpecificRef(df::job *job, df::specific_ref_type type);

        DFHACK_EXPORT df::building *getHolder(df::job *job);
        DFHACK_EXPORT df::unit *getWorker(df::job *job);

        DFHACK_EXPORT void setJobCooldown(df::building *workshop, df::unit *worker, int cooldown = 100);
        DFHACK_EXPORT bool removeWorker(df::job *job, int cooldown = 100);

        // Instruct the game to check and assign workers
        DFHACK_EXPORT void checkBuildingsNow();
        DFHACK_EXPORT void checkDesignationsNow();

        DFHACK_EXPORT bool linkIntoWorld(df::job *job, bool new_id = true);

        // Flag this job's posting as "dead" and set its posting_index to -1
        // If remove_all is true, flag all postings pointing to this job
        // Returns true if any postings were removed
        DFHACK_EXPORT bool removePostings(df::job *job, bool remove_all = false);

        // lists jobs with ids >= *id_var, and sets *id_var = *job_next_id;
        DFHACK_EXPORT bool listNewlyCreated(std::vector<df::job*> *pvec, int *id_var);

        DFHACK_EXPORT bool attachJobItem(df::job *job, df::item *item,
                                         df::job_item_ref::T_role role,
                                         int filter_idx = -1, int insert_idx = -1);

        DFHACK_EXPORT bool isSuitableItem(df::job_item *item, df::item_type itype, int isubtype);
        DFHACK_EXPORT bool isSuitableMaterial(df::job_item *item, int mat_type, int mat_index);
        DFHACK_EXPORT std::string getName(df::job *job);
    }

    DFHACK_EXPORT bool operator== (const df::job_item &a, const df::job_item &b);
    DFHACK_EXPORT bool operator== (const df::job &a, const df::job &b);
}
#endif

