#pragma once

/*  The following tests optimise behaviour on little-endian
    machines, where there is no need to reverse the byte order
    of 32 bit words in the MD5 computation.  By default,
    HIGHFIRST is defined, which indicates we're running on a
    big-endian (most significant byte first) machine, on which
    the byteReverse function in md5.c must be invoked. However,
    byteReverse is coded in such a way that it is an identity
    function when run on a little-endian machine, so calling it
    on such a platform causes no harm apart from wasting time.
    If the platform is known to be little-endian, we speed
    things up by undefining HIGHFIRST, which defines
    byteReverse as a null macro.  Doing things in this manner
    insures we work on new platforms regardless of their byte
    order.  */

#define HIGHFIRST

#ifdef __i386__
#undef HIGHFIRST
#endif

#include <stdint.h>

struct MD5Context
{
    uint32_t buf[4];
    uint32_t bits[2];
    unsigned char in[64];
};

extern void MD5Init( MD5Context *ctx);
extern void MD5Update( MD5Context *ctx, unsigned char *buf, unsigned len);
extern void MD5Final(unsigned char digest[16], MD5Context *ctx);
extern void MD5Transform(uint32_t buf[4], uint32_t in[16]);
