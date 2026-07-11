#pragma once

#include <functional>
#include <bit>
#include "df/texture_fullid.h"

template<>
struct std::hash<df::texture_fullid>
{
    size_t operator()(df::texture_fullid const &t) const noexcept
    {
// for some reason, bay12 used different hash methods on windows vs linux
#ifdef WIN32
        size_t h=std::hash<int>{}(t.texpos);
        auto u_hash=std::hash<uint64_t>{};
        h^=u_hash(std::bit_cast<uint64_t>(std::make_pair(t.r, t.g)));
        h^=u_hash(std::bit_cast<uint64_t>(std::make_pair(t.b, t.br)))<<1;
        h^=u_hash(std::bit_cast<uint64_t>(std::make_pair(t.bg, t.bb)))<<2;
        h^=std::hash<uint32_t>{}(t.flag.whole);
        return h;
#else
        size_t h=std::hash<float>{}(t.texpos);
        auto u_hash=std::hash<uint64_t>{};
        h^=u_hash(t.r);
        h^=u_hash(t.g)<<1;
        h^=u_hash(t.b)<<2;
        h^=u_hash(t.br)<<3;
        h^=u_hash(t.bg)<<4;
        h^=u_hash(t.bb)<<5;
        h^=std::hash<uint32_t>{}(t.flag.whole)<<6;
        return h;
#endif
    }
};
