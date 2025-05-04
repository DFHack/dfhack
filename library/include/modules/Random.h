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
 * \defgroup grp_random Random: Random number and noise generation
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
        /* No constructor or destructor - safe to treat as data */

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
    extern template void DFHACK_IMPORT MersenneRNG::unitvector<float>(float *p, int size);
    extern template void DFHACK_IMPORT MersenneRNG::unitvector<double>(double *p, int size);
#endif

    // Standard Splitmix64 RNG, as used by Dwarf Fortress's "hash_rngst" class
    class SplitmixRNG
    {
        uint64_t state;

    public:
        SplitmixRNG(uint64_t seed) {
            init(seed);
        }

        void init(uint64_t seed) {
            state = seed;
        }

        uint64_t next() {
            state += 0x9e3779b97f4a7c15;
            uint64_t z = state;
            z ^= z >> 30;
            z *= 0xbf58476d1ce4e5b9;
            z ^= z >> 27;
            z *= 0x94d049bb133111eb;
            z ^= z >> 31;
            return z;
        }

        int32_t df_trandom(uint32_t max) {
            uint32_t val = next() >> 32;
            return (int32_t)(val % max);
        }
    };

    /*
     * Classical Perlin noise function in template form.
     * http://mrl.nyu.edu/~perlin/doc/oscar.html#noise
     *
     * Using an improved hash function from:
     * http://www.cs.utah.edu/~aek/research/noise.pdf
     */

    template<class T, unsigned VSIZE, unsigned BITS = 8, class IDXT = uint8_t>
    class PerlinNoise
    {
        // Size of randomness tables
        static const unsigned TSIZE = 1<<BITS;

        T gradients[TSIZE][VSIZE];
        IDXT idxmap[VSIZE][TSIZE];

        // Templates used to unwind and inline recursion and loops
        struct Temp {
            T r0, s;
            unsigned b0, b1;
        };
        template<unsigned mask, int i>
        struct Impl {
            static inline void setup(PerlinNoise<T,VSIZE,BITS,IDXT> *self, const T *pv, Temp *pt);
            static inline T eval(PerlinNoise<T,VSIZE,BITS,IDXT> *self, Temp *pt, unsigned idx, T *pq);
        };

    public:
        /* No constructor or destructor - safe to treat as data */

        void init(MersenneRNG &rng);

        T eval(const T coords[VSIZE]);
    };

#ifndef DFHACK_RANDOM_CPP
    extern template class DFHACK_IMPORT PerlinNoise<float, 1>;
    extern template class DFHACK_IMPORT PerlinNoise<float, 2>;
    extern template class DFHACK_IMPORT PerlinNoise<float, 3>;
#endif

    template<class T, unsigned BITS = 8, class IDXT = uint8_t>
    class PerlinNoise1D : public PerlinNoise<T, 1, BITS, IDXT>
    {
    public:
        T operator() (T x) { return this->eval(&x); }
    };

    template<class T, unsigned BITS = 8, class IDXT = uint8_t>
    class PerlinNoise2D : public PerlinNoise<T, 2, BITS, IDXT>
    {
    public:
        T operator() (T x, T y) {
            T tmp[2] = { x, y };
            return this->eval(tmp);
        }
    };

    template<class T, unsigned BITS = 8, class IDXT = uint8_t>
    class PerlinNoise3D : public PerlinNoise<T, 3, BITS, IDXT>
    {
    public:
        T operator() (T x, T y, T z) {
            T tmp[3] = { x, y, z };
            return this->eval(tmp);
        }
    };
}
}
#endif
