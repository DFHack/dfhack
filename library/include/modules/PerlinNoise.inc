#pragma once

namespace DFHack {
namespace Random {

/*
 * A good explanation:
 * http://webstaff.itn.liu.se/~stegu/TNM022-2005/perlinnoiselinks/perlin-noise-math-faq.html
 */

// Interpolation functions

template<class T>
inline T s_curve(T t)
{
    // Classical function
    //return t * t * (3 - 2*t);

    // 2002 version from http://mrl.nyu.edu/~perlin/paper445.pdf
    return t * t * t * (t * (t * 6 - 15) + 10);
}

template<class T>
inline T lerp(T s, T a, T b)
{
    return a + s * (b-a);
}

// Dot product of VSIZE vectors pointed by pa, pb

template<class T, unsigned i>
struct DotProduct {
    static inline T eval(T *pa, T *pb);
};
template<class T>
struct DotProduct<T,0> {
    static inline T eval(T *pa, T *pb) { return pa[0]*pb[0]; }
};
template<class T, unsigned i>
inline T DotProduct<T,i>::eval(T *pa, T *pb) {
    return DotProduct<T,i-1>::eval(pa, pb) + pa[i]*pb[i];
}

// Templates used to force unrolling and inlining of the loops

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
template<unsigned mask>
struct PerlinNoise<T,VSIZE,BITS,IDXT>::Impl<mask,-1> {
    typedef typename PerlinNoise<T,VSIZE,BITS,IDXT>::Temp Temp;
    static inline void setup(PerlinNoise<T,VSIZE,BITS,IDXT> *, const T *, Temp *) {}
    static inline T eval(PerlinNoise<T,VSIZE,BITS,IDXT> *self, Temp *pt, unsigned idx, T *pq);
};

// Initialization of the temporaries from input coordinates

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
template<unsigned mask, int i>
inline void PerlinNoise<T,VSIZE,BITS,IDXT>::Impl<mask,i>::setup(
    PerlinNoise<T,VSIZE,BITS,IDXT> *self, const T *pv, Temp *pt
) {
    Impl<mask,i-1>::setup(self, pv, pt);

    int32_t t = int32_t(pv[i]);
    t -= (pv[i]<t);
    pt[i].s = s_curve(pt[i].r0 = pv[i] - t);

    unsigned b = unsigned(int32_t(t));
    pt[i].b0 = self->idxmap[i][b & mask];
    pt[i].b1 = self->idxmap[i][(b+1) & mask];
}

// Main recursion. Uses tables from self and pt.
// Recursion changes current index idx, and current offset vector pq.

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
template<unsigned mask>
inline T PerlinNoise<T,VSIZE,BITS,IDXT>::Impl<mask, -1>::eval(
    PerlinNoise<T,VSIZE,BITS,IDXT> *self, Temp *pt, unsigned idx, T *pq
) {
    return DotProduct<T,VSIZE-1>::eval(pq, self->gradients[idx]);
}

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
template<unsigned mask, int i>
inline T PerlinNoise<T,VSIZE,BITS,IDXT>::Impl<mask,i>::eval(
    PerlinNoise<T,VSIZE,BITS,IDXT> *self, Temp *pt, unsigned idx, T *pq
) {
    pq[i] = pt[i].r0;
    T u = Impl<mask,i-1>::eval(self, pt, idx ^ pt[i].b0, pq);

    pq[i] -= 1;
    T v = Impl<mask,i-1>::eval(self, pt, idx ^ pt[i].b1, pq);

    return lerp(pt[i].s, u, v);
}

// Actual methods of the object

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
void PerlinNoise<T,VSIZE,BITS,IDXT>::init(MersenneRNG &rng)
{
    STATIC_ASSERT(VSIZE > 0 && BITS <= 8*sizeof(IDXT));

    // Random unit gradient vectors
    for (unsigned i = 0; i < TSIZE; i++)
        rng.unitvector(gradients[i], VSIZE);

    // Random permutation tables
    for (unsigned j = 0; j < VSIZE; j++)
    {
        for (unsigned i = 0; i < TSIZE; i++)
            idxmap[j][i] = i;

        rng.permute(idxmap[j], TSIZE);
    }
}

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
T PerlinNoise<T,VSIZE,BITS,IDXT>::eval(const T coords[VSIZE])
{
    // Precomputed properties from the coordinates
    Temp tmp[VSIZE];
    // Temporary used to build the current offset vector
    T q[VSIZE];

    Impl<TSIZE-1,VSIZE-1>::setup(this, coords, tmp);

    return Impl<TSIZE-1,VSIZE-1>::eval(this, tmp, 0, q);
}

}} // namespace
