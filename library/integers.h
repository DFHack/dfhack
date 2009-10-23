/*
This file is a wrapper for stdint.h

Visual Studio doesn't have stdint.h

stdint.h is part of the C99 standard. It's ancient and simply should be there. FAIL

You can turn off the include by defining SKIP_DFHACK_STDINT
*/
#ifndef SKIP_DFHACK_STDINT
    #ifndef _MSC_VER
        #include <stdint.h>
    #else
        #include "stdint_win.h"
    #endif
#endif