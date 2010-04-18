#ifndef DFHACK_GLOBAL
#define DFHACK_GLOBAL
#include "Export.h"
namespace DFHack
{
    class Process;
    /*
    * Currently attached process and its handle
    */
    extern DFHACK_EXPORT Process * g_pProcess; ///< current process. non-NULL when picked
}
#endif