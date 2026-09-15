#ifndef HASHUTIL_H
#define HASHUTIL_H

// some utility classes for dealing with hashes

namespace
{
    // borrowed from Boost
    template <class T>
    inline void hash_combine(std::size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b97f4a7c15ull + (seed << 12) + (seed >> 4);
    }

    template <class... Args>
    inline std::size_t hash_value(const Args&... args)
    {
        std::size_t seed = 0;
        (hash_combine(seed, args), ...);
        return seed;
    }

    template<typename ... Bases>
    struct overload : Bases ...
    {
        using is_transparent = void;
        using Bases::operator() ...;
    };

    struct char_pointer_hash
    {
        auto operator()(const char* ptr) const noexcept
        {
            return std::hash<std::string_view>{}(ptr);
        }
    };

    using transparent_string_hash = overload<
        std::hash<std::string>,
        std::hash<std::string_view>,
        char_pointer_hash
    >;
}

#endif
