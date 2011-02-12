#ifndef DFHACK_TRANQUILITY
#define DFHACK_TRANQUILITY

// This is here to keep MSVC from spamming the build output with nonsense
// Call it public domain.

#ifdef _MSC_VER
    // don't spew nonsense
    #pragma warning( disable: 4251 )
    // don't display bogus 'deprecation' and 'unsafe' warnings.
    // See the idiocy: http://msdn.microsoft.com/en-us/magazine/cc163794.aspx
    #define _CRT_SECURE_NO_DEPRECATE
    #define _SCL_SECURE_NO_DEPRECATE
    #pragma warning( disable: 4996 )
    // Let me demonstrate:
    /**
     * [peterix@peterix dfhack]$ man wcscpy_s
     * No manual entry for wcscpy_s
     * 
     * Proprietary extensions.
     */
    // disable stupid
    #pragma warning( disable: 4800 )
    // disable more stupid
    #pragma warning( disable: 4068 )
#endif

#endif
