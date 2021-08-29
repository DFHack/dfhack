#pragma once

#include <Console.h>
#include <PluginManager.h>

#include <vector>

using namespace DFHack;

class AutoLaborManager {

public:
    AutoLaborManager() {}
    virtual ~AutoLaborManager() {}

    virtual void cleanup_state() {}
    virtual void init_state() {}

    virtual void process(color_ostream* out) = 0;

    virtual command_result do_command(color_ostream& out, std::vector<std::string>& parameters);
protected:
    DFHack::color_ostream* out;
};

