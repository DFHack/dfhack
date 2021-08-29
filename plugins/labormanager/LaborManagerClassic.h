#pragma once

#include "AutoLaborManager.h"

class LaborManagerClassic : public AutoLaborManager
{
    friend class JobLaborMappper;

public:
    LaborManagerClassic();
    ~LaborManagerClassic() = default;

    void process(color_ostream* o);
    void cleanup_state();
    void init_state();
    command_result do_command(color_ostream& out, std::vector<std::string>& parameters) override;
    
private:
    class impl;
    std::unique_ptr<impl> i;
};
