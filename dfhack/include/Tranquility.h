#ifndef DFHACK_TRANQUILITY
#define DFHACK_TRANQUILITY

// This is here to keep MSVC from spamming the build output with nonsense
// Call it public domain.

#ifdef _MSC_VER
    // don't spew nonsense
    #pragma warning( disable: 4251 )
    // don't display bogus 'deprecation' and 'unsafe' warnings
    #pragma warning( disable: 4996 )
    // disable stupid
    #pragma warning( disable: 4800 )
    // disable more stupid
    #pragma warning( disable: 4068 )
#endif

#endif
