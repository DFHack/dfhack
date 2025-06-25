#pragma once

/**
  * Split off from Core.h to keep unnecessary class definitions out of compilation units that do not need them
  *
  */

namespace DFHack
{

    enum command_result
    {
        CR_LINK_FAILURE = -3,    // RPC call failed due to I/O or protocol error
        CR_NEEDS_CONSOLE = -2,   // Attempt to call interactive command without console
        CR_NOT_IMPLEMENTED = -1, // Command not implemented, or plugin not loaded
        CR_OK = 0,               // Success
        CR_FAILURE = 1,          // Failure
        CR_WRONG_USAGE = 2,      // Wrong arguments or ui state
        CR_NOT_FOUND = 3         // Target object not found (for RPC mainly)
    };

    enum state_change_event
    {
        SC_UNKNOWN = -1,
        SC_WORLD_LOADED = 0,
        SC_WORLD_UNLOADED = 1,
        SC_MAP_LOADED = 2,
        SC_MAP_UNLOADED = 3,
        SC_VIEWSCREEN_CHANGED = 4,
        SC_CORE_INITIALIZED = 5,
        SC_BEGIN_UNLOAD = 6,
        SC_PAUSED = 7,
        SC_UNPAUSED = 8,
        SC_NEW_MAP_AVAILABLE = 9
    };

}
