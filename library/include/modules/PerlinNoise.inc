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
    return t * t * (3 - 2*t);
}

template<class T>
inline T lerp(T s, T a, T b)
{
    return a + s * (b-a);
}

// Templates used to force unrolling and inlining of the loops

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
template<unsigned mask>
struct PerlinNoise<T,VSIZE,BITS,IDXT>::Impl<mask,0> {
    typedef typename PerlinNoise<T,VSIZE,BITS,IDXT>::Temp Temp;
    static inline T dot(T *pa, T *pb);
    static inline void setup(const T *pv, Temp *pt);
    static inline T eval(PerlinNoise<T,VSIZE,BITS,IDXT> *self, Temp *pt, unsigned idx, T *pq);
};

// Dot product of VSIZE vectors pointed by pa, pb

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
template<unsigned mask>
inline T PerlinNoise<T,VSIZE,BITS,IDXT>::Impl<mask,0>::dot(T *pa, T *pb)
{
    return pa[0] * pb[0];
}

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
template<unsigned mask, unsigned i>
inline T PerlinNoise<T,VSIZE,BITS,IDXT>::Impl<mask,i>::dot(T *pa, T *pb)
{
    return Impl<mask,i-1>::dot(pa, pb) + pa[i] * pb[i];
}

// Initialization of the temporaries from input coordinates

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
template<unsigned mask>
inline void PerlinNoise<T,VSIZE,BITS,IDXT>::Impl<mask,0>::setup(const T *pv, Temp *pt)
{
    T t = std::floor(*pv);
    pt->s = s_curve(pt->r0 = *pv - t);
    pt->b0 = unsigned(int32_t(t)) & mask;
}

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
template<unsigned mask, unsigned i>
inline void PerlinNoise<T,VSIZE,BITS,IDXT>::Impl<mask,i>::setup(const T *pv, Temp *pt)
{
    Impl<mask,i-1>::setup(pv,pt);
    Impl<mask,0>::setup(pv+i,pt+i);
}

// Main recursion. Uses tables from self and pt.
// Recursion changes current index idx, and current offset vector pq.

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
template<unsigned mask>
inline T PerlinNoise<T,VSIZE,BITS,IDXT>::Impl<mask, 0>::eval(
    PerlinNoise<T,VSIZE,BITS,IDXT> *self, Temp *pt, unsigned idx, T *pq
) {
    pq[0] = pt[0].r0;
    idx += pt[0].b0;
    T u = Impl<mask,VSIZE-1>::dot(pq, self->gradients[idx]);

    pq[0] -= 1;
    idx += 1;
    T v = Impl<mask,VSIZE-1>::dot(pq, self->gradients[idx]);

    return lerp(pt[0].s, u, v);
}

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
template<unsigned mask, unsigned i>
inline T PerlinNoise<T,VSIZE,BITS,IDXT>::Impl<mask,i>::eval(
    PerlinNoise<T,VSIZE,BITS,IDXT> *self, Temp *pt, unsigned idx, T *pq
) {
    pq[i] = pt[i].r0;
    idx += pt[i].b0;
    T u = Impl<mask,i-1>::eval(self, pt, self->idxmap[idx], pq);

    pq[i] -= 1;
    idx += 1;
    T v = Impl<mask,i-1>::eval(self, pt, self->idxmap[idx], pq);

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

    // Random permutation table
    for (unsigned i = 0; i < TSIZE; i++)
        idxmap[i] = i;

    rng.permute(idxmap, TSIZE);

    // Extended part of the table to avoid doing bitwise ops
    for (unsigned i = TSIZE; i < TSIZE_EXT; i++)
    {
        for (unsigned j = 0; j < VSIZE; j++)
            gradients[i][j] = gradients[i-TSIZE][j];

        idxmap[i] = idxmap[i-TSIZE];
    }
}

template<class T, unsigned VSIZE, unsigned BITS, class IDXT>
T PerlinNoise<T,VSIZE,BITS,IDXT>::eval(const T coords[VSIZE])
{
    // Precomputed properties from the coordinates
    Temp tmp[VSIZE];
    // Temporary used to build the current offset vector
    T q[VSIZE];

    Impl<TSIZE-1,VSIZE-1>::setup(coords, tmp);

    return Impl<TSIZE-1,VSIZE-1>::eval(this, tmp, 0, q);
}

}} // namespace
