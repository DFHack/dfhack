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

#pragma once
#ifndef CL_MOD_RANDOM
#define CL_MOD_RANDOM
/**
 * \defgroup grp_translation Translation: DF word tables and name translation/reading
 * @ingroup grp_modules
 */

#include "Export.h"
#include "Module.h"
#include "Types.h"

#include "DataDefs.h"

namespace DFHack
{
namespace Random
{
    class DFHACK_EXPORT MersenneRNG
    {
        static const unsigned MT_LEN = 624;

        unsigned mt_index;
        uint32_t mt_buffer[MT_LEN];

        void twist();
        void prefill(unsigned step, int twist_cnt);

    public:
        void init(const uint32_t *pseed, unsigned cnt, int twist_cnt = 1);

        void init(); // uses time
        void init(uint32_t seed, int twist_cnt = 1) { init(&seed, 1, twist_cnt); }

        // [0, 2^32)
        uint32_t random() {
            if (mt_index >= MT_LEN) twist();
            return mt_buffer[mt_index++];
        }
        // [0, limit)
        uint32_t random(uint32_t limit) {
            return uint32_t(uint64_t(random())*limit >> 32);
        }
        // (0, 1)
        double drandom0() {
            return (double(random())+1)/4294967297.0;
        }
        // [0, 1)
        double drandom() {
            return double(random())/4294967296.0;
        }
        // [0, 1]
        double drandom1() {
            return double(random())/4294967295.0;
        }
        // [-1, 1]
        double unitrandom() {
            return drandom1()*2.0 - 1.0;
        }

        // Two exact replicas of functions in DF code
        int32_t df_trandom(uint32_t max=2147483647LU);
        int32_t df_loadtrandom(uint32_t max=2147483647LU);

        template<class T>
        void unitvector(T *p, int size);

        template<class T>
        void permute(T *p, int size) {
            while(size > 1)
            {
                int j = random(size--);
                T c = p[j]; p[j] = p[size]; p[size] = c;
            }
        }
    };

#ifndef DFHACK_RANDOM_CPP
    extern template void MersenneRNG::unitvector<float>(float *p, int size);
    extern template void MersenneRNG::unitvector<double>(double *p, int size);
#endif

}
}
#endif
