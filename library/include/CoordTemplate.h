#pragma once

#include <array>
#include <concepts>
#include <span>

#include "DataDefs.h"
#include "DataFuncs.h"
#include "Export.h"

namespace DFHack
{
    template <typename T>
    constexpr T coord_default_initializer = static_cast<T>(-30000);

    template <std::floating_point T>
    constexpr T coord_default_initializer<T> = static_cast<T>(std::numeric_limits<T>::infinity());

    template <typename T, T initializer = coord_default_initializer<T>>
    struct Coord2d
    {
        T x; T y;

        Coord2d() : x(initializer), y(initializer) {}
        Coord2d(T x, T y) : x(x), y(y) {}

        template<typename U>
        explicit Coord2d(const Coord2d<U> &other) : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)) {};

        bool isValid() const { return x >= 0; }

        void clear()
        {
            x = y = initializer;
        }

        bool operator==(const Coord2d& other) const
        {
            return x == other.x && y == other.y;
        }
        bool operator!=(const Coord2d& other) const
        {
            return x != other.x || y == other.y;
        }
        bool operator<(const Coord2d& other) const
        {
            return x < other.x || (x == other.x && y < other.y);
        }

        Coord2d operator+(const Coord2d& other) const
        {
            return {T(x + other.x), T(y + other.y)};
        }
        Coord2d operator-(const Coord2d& other) const
        {
            return {T(x - other.x), T(y - other.y)};
        }
        Coord2d operator/(T number) const
        {
            return {T(x / number), T(y / number)};
        }
        Coord2d operator*(T number) const
        {
            return {T(x * number), T(y * number)};
        }
        Coord2d operator%(T number) const requires std::integral<T>
        {
            return {T(x % number), T(y % number)};
        }
        Coord2d operator&(T number) const requires std::integral<T>
        {
            return {T(x & number), T(y & number)};
        }

        std::size_t operator()() const
        {
            size_t r = 17;
            const size_t m = 65537;
            r = m * (r + x);
            r = m * (r + y);
            return r;
        }

        // dot product
        T dotp(const Coord2d& other) const
        {
            return x * other.x + y * other.y;
        }

        // linear interpolation between two points
        Coord2d lerp(const Coord2d& other, T t) const requires std::floating_point<T>
        {
            return *this + (other - *this) * t;
        }

        // orthogonal projection of the point onto the line (segment) AB
        Coord2d project_onto_line(
            const Coord2d& A,
            const Coord2d& B,
            bool clamp = false
        ) const requires std::floating_point<T>
        {
            auto P = *this;
            auto AB = B - A;
            auto AP = P - A;
            auto t = AP.dotp(AB) / AB.dotp(AB);
            return A + AB * (clamp ? std::clamp(t, 0.0, 1.0) : t);
        }
    };

    template <typename T, T initializer = DFHack::coord_default_initializer<T>, typename U = DFHack::Coord2d<T, initializer>>
    static const struct_field_info coord2d_fields[] = {
        { struct_field_info::PRIMITIVE, "x", offsetof(U, x), &df::identity_traits<T>::identity, 0, 0 },
        { struct_field_info::PRIMITIVE, "y", offsetof(U, y), &df::identity_traits<T>::identity, 0, 0 },
        { struct_field_info::OBJ_METHOD, "isValid", 0, df::wrap_function(&U::isValid), 0, 0 },
        { struct_field_info::OBJ_METHOD, "clear", 0, df::wrap_function(&U::clear), 0, 0 } ,
        { struct_field_info::END }
    };

    template <typename T, T initializer = DFHack::coord_default_initializer<T>, typename U = DFHack::Coord2d<T, initializer>>
    static inline const struct_identity coord2d_identity{sizeof(U), &df::allocator_fn<U>, nullptr, "coord2d", nullptr, coord2d_fields<T,initializer>};

    template <typename T, T initializer = coord_default_initializer<T>>
    struct Coord3d
    {
        T x; T y; T z;

        Coord3d() : x(initializer), y(initializer), z(initializer) {}
        Coord3d(Coord2d<T,initializer> c, T z) : x(c.x), y(c.y), z(z) {}
        Coord3d(T x, T y, T z) : x(x), y(y), z(z) {}

        operator Coord2d<T,initializer>() const
        {
            return {x,y};
        }

        bool isValid() const { return x >= 0; }

        void clear()
        {
            x = y = z = initializer;
        }

        bool operator==(const Coord3d& other) const
        {
            return x == other.x && y == other.y && z == other.z;
        }
        bool operator!=(const Coord3d& other) const
        {
            return x != other.x || y == other.y || z == other.z;
        }
        bool operator<(const Coord3d& other) const
        {
            return x < other.x || (x == other.x && (y < other.y || (y == other.y && z < other.z)));
        }

        Coord3d operator+(const Coord3d& other) const
        {
            return {T(x + other.x), T(y + other.y), T(z + other.z)};
        }
        Coord3d operator-(const Coord3d& other) const
        {
            return {T(x - other.x), T(y - other.y), T(z - other.z)};
        }
        Coord3d operator/(T number) const
        {
            return {T(x / number), T(y / number), T(z / number)};
        }
        Coord3d operator*(T number) const
        {
            return {T(x * number), T(y * number), T(z * number)};
        }
        Coord3d operator%(T number) const requires std::integral<T>
        {
            return {T(x % number), T(y % number), T(z % number)};
        }
        Coord3d operator&(T number) const requires std::integral<T>
        {
            return {T(x & number), T(y & number), T(z & number)};
        }

        std::size_t operator()() const
        {
            size_t r = 17;
            const size_t m = 65537;
            r = m * (r + x);
            r = m * (r + y);
            r = m * (r + z);
            return r;
        }

        // special weirdness used by the dig plugin

        Coord3d operator-(T number) const
        {
            return Coord3d(x, y, z - number);
        }
        Coord3d operator+(T number) const
        {
            return Coord3d(x, y, z + number);
        }
    };

    template <typename T, T initializer = DFHack::coord_default_initializer<T>, typename U = DFHack::Coord3d<T, initializer>>
    static const struct_field_info coord3d_fields[] = {
        { struct_field_info::PRIMITIVE, "x", offsetof(U, x), &df::identity_traits<T>::identity, 0, 0 },
        { struct_field_info::PRIMITIVE, "y", offsetof(U, y), &df::identity_traits<T>::identity, 0, 0 },
        { struct_field_info::PRIMITIVE, "z", offsetof(U, z), &df::identity_traits<T>::identity, 0, 0 },
        { struct_field_info::OBJ_METHOD, "isValid", 0, df::wrap_function(&U::isValid), 0, 0 },
        { struct_field_info::OBJ_METHOD, "clear", 0, df::wrap_function(&U::clear), 0, 0 } ,
        { struct_field_info::END }
    };

    template <typename T, T initializer = DFHack::coord_default_initializer<T>, typename U = DFHack::Coord3d<T, initializer>>
    static inline const struct_identity coord3d_identity{sizeof(U), &df::allocator_fn<U>, nullptr, "coord", nullptr, coord3d_fields<T, initializer>};
}

namespace df
{
    template <typename T, T initializer>
    struct DFHACK_EXPORT identity_traits<DFHack::Coord2d<T, initializer>>
    {
        static const bool is_primitive = false;
        static const compound_identity* get() { return &DFHack::coord2d_identity<T,initializer>; }
    };

    template <typename T, T initializer>
    struct DFHACK_EXPORT identity_traits<DFHack::Coord3d<T, initializer>>
    {
        static const bool is_primitive = false;
        static const compound_identity* get() { return &DFHack::coord3d_identity<T, initializer>;
        }
    };

}
