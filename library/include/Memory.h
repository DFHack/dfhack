// Default base address
#if defined(_WIN32)
    #ifdef DFHACK64
        #define DEFAULT_BASE_ADDR 0x140000000
    #else
        #define DEFAULT_BASE_ADDR 0x400000
    #endif
#elif defined(_DARWIN)
    #ifdef DFHACK64
        #define DEFAULT_BASE_ADDR 0x100000000
    #else
        #define DEFAULT_BASE_ADDR 0x1000
    #endif
#elif defined(_LINUX)
    #ifdef DFHACK64
        #define DEFAULT_BASE_ADDR 0x400000
    #else
        #define DEFAULT_BASE_ADDR 0x8048000
    #endif
#else
    #error Unknown OS
#endif
