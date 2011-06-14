#pragma once

#include "dfhack/Pragma.h"
#include "dfhack/Export.h"
namespace DFHack
{
    class VersionInfoFactory;
    class Process;
    class Context;
    // Core is a singleton. Why? Because it is closely tied to SDL calls. It tracks the global state of DF.
    // There should never be more than one instance
    // Better than tracking some weird variables all over the place.
    class DFHACK_EXPORT Core
    {
    public:
        static Core& getInstance()
        {
            // FIXME: add critical section for thread safety here.
            static Core instance;
            return instance;
        }
        int Update   (void);
        int Shutdown (void);
    private:
        Core();
        Core(Core const&);              // Don't Implement
        void operator=(Core const&);    // Don't implement
        bool errorstate;
        // legacy mess.
        DFHack::VersionInfoFactory * vif;
        DFHack::Process * p;
        DFHack::Context * c;
    };
}