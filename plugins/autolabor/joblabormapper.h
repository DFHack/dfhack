#pragma once

#include <map>
#include <string>
#include <vector>


#include "df/job.h"
#include "df/job_type.h"
#include "df/unit_labor.h"

using namespace DFHack;
using namespace df::enums;


class jlfunc;

class JobLaborMapper {

private:
    std::map<df::job_type, jlfunc*> job_to_labor_table;
    std::map<df::unit_labor, jlfunc*> jlf_cache;

    jlfunc* jlf_const(df::unit_labor l);

public:
    ~JobLaborMapper();
    JobLaborMapper();

    df::unit_labor find_job_labor(df::job* j);

    // appends names of job_type enum items with no entry in the labor
    // table; used by the job coverage self-test
    void job_coverage(std::vector<std::string> &missing);


};
