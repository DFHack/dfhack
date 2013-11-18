/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "Internal.h"

#include <string>
#include <vector>
#include <map>
using namespace std;

#define DFHACK_RANDOM_CPP

#include "modules/Random.h"
#include "VersionInfo.h"
#include "MemAccess.h"
#include "Types.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "Error.h"
#include "VTableInterpose.h"

#include <cmath>

using namespace DFHack;
using namespace df::enums;

using namespace DFHack::Random;

//public domain RNG stuff by Michael Brundage
//modified to be compatible with the version in DF

#define MT_IA           397
#define MT_IB           (MT_LEN - MT_IA)
#define UPPER_MASK      0x80000000
#define LOWER_MASK      0x7FFFFFFF
#define MATRIX_A        0x9908B0DF
#define TWIST(b,i,j)    ((b)[i] & UPPER_MASK) | ((b)[j] & LOWER_MASK)
#define MAGIC(s)        (((s)&1)*MATRIX_A)

void MersenneRNG::twist()
{
    uint32_t *b = mt_buffer;
    uint32_t s;
    unsigned i;

    i = 0;
    for (; i < MT_IB; i++) {
        s = TWIST(b, i, i+1);
        b[i] = b[i + MT_IA] ^ (s >> 1) ^ MAGIC(s);
    }
    for (; i < MT_LEN-1; i++) {
        s = TWIST(b, i, i+1);
        b[i] = b[i - MT_IB] ^ (s >> 1) ^ MAGIC(s);
    }

    s = TWIST(b, MT_LEN-1, 0);
    b[MT_LEN-1] = b[MT_IA-1] ^ (s >> 1) ^ MAGIC(s);

    mt_index = 0;
}

void MersenneRNG::prefill(unsigned step, int twist_cnt)
{
    for(unsigned i=step;i<MT_LEN;i++)
    {
        //2010: better init line from wikipedia, ultimate source unknown
        mt_buffer[i]=1812433253UL * (mt_buffer[i-step] ^ (mt_buffer[i-step]>>30)) + i;
    }

    mt_index = 0;

    for(int j=0;j<twist_cnt;j++)
        twist();
}

void MersenneRNG::init()
{
    init(Core::getInstance().p->getTickCount(), 20);
}

void MersenneRNG::init(const uint32_t *pseed, unsigned cnt, int twist_cnt)
{
    memcpy(mt_buffer, pseed, cnt*sizeof(uint32_t));

    prefill(cnt, twist_cnt);
}

int32_t MersenneRNG::df_trandom(uint32_t max)
{
    if(max<=1)return 0;
    uint32_t seed=random();
    seed=seed%2147483647LU;
    seed=seed/((2147483647LU/max)+1);

    return((int32_t)seed);
}

int32_t MersenneRNG::df_loadtrandom(uint32_t max)
{
    uint32_t seed=random();
    seed=seed%max;

    return((int32_t)seed);
}

template<class T>
void MersenneRNG::unitvector(T *p, int size)
{
    for (;;)
    {
        T rsqr = 0;
        for (int i = 0; i < size; i++)
        {
            p[i] = (T)unitrandom();
            rsqr += p[i]*p[i];
        }

        if (rsqr > 0 && rsqr <= 1)
        {
            rsqr = std::sqrt(rsqr);
            for (int i = 0; i < size; i++)
                p[i] /= rsqr;
            break;
        }
    }
}

template void MersenneRNG::unitvector<float>(float *p, int size);
template void MersenneRNG::unitvector<double>(double *p, int size);

#include "modules/PerlinNoise.inc"

template class DFHACK_EXPORT PerlinNoise<float, 1>;
template class DFHACK_EXPORT PerlinNoise<float, 2>;
template class DFHACK_EXPORT PerlinNoise<float, 3>;
