#pragma once

#include "../Core.h"
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

#ifndef RATUI_VEC_IMPL
    #include "../Extern/nicemath.h"
    #define RATUI_VEC_IMPL ::nm::vec
#endif // Default to Vec template if no custom vector implementation is provided.

#ifndef RATUI_MATRIX_IMPL
    #include "../Extern/nicemath.h"
    #define RATUI_MATRIX_IMPL ::nm::mat
#endif // Default to Mat template if no custom matrix implementation is provided.

#ifndef RATUI_COLOR_IMPL
    #define RATUI_COLOR_IMPL RATUI_VEC_IMPL<RatUI::f32, 4>
#endif // Default to Vec<f32, 4> if no custom Color implementation is provided.

namespace RatUI
{
    template<typename T>
    inline constexpr T Pi = static_cast<T>( 3.14159265358979323846 );

    template<typename T>
    constexpr T DegToRad( T a_Degrees ) { return a_Degrees * ( Pi<T> / static_cast<T>( 180 ) ); }

    template<typename T>
    constexpr T RadToDeg( T a_Radians ) { return a_Radians * ( static_cast<T>( 180 ) / Pi<T> ); }

    // === Vector Types ===

    template<typename T, size Dim>
    using Vec = RATUI_VEC_IMPL<T, Dim>;

    template<typename T> using Vec2 = Vec<T, 2>;
    template<typename T> using Vec3 = Vec<T, 3>;
    template<typename T> using Vec4 = Vec<T, 4>;

    using Vec2f = Vec<f32, 2>;
    using Vec2i = Vec<i32, 2>;
    using Vec2u = Vec<u32, 2>;
    using Vec3f = Vec<f32, 3>;
    using Vec3i = Vec<i32, 3>;
    using Vec3u = Vec<u32, 3>;
    using Vec4f = Vec<f32, 4>;
    using Vec4i = Vec<i32, 4>;
    using Vec4u = Vec<u32, 4>;

    // === Matrix Types ===

    template<typename T, size Dim>
    using Mat = RATUI_MATRIX_IMPL<T, Dim>;

    template<typename T> using Mat2 = Mat<T, 2>;
    template<typename T> using Mat3 = Mat<T, 3>;
    template<typename T> using Mat4 = Mat<T, 4>;

    using Mat2f = Mat<f32, 2>;
    using Mat3f = Mat<f32, 3>;
    using Mat4f = Mat<f32, 4>;

    // === Color Type ===

    using Color = RATUI_COLOR_IMPL;

    /**
     * @brief A simple axis-aligned rectangle structure defined by its center and half-extents.
     * @tparam T The type of the rectangle's coordinates and dimensions (e.g., float, int).
     */
    template<typename T>
    struct Rect
    {
        using ValueType = T;
        using Vec2Type = Vec<T, 2>;

        Vec2<T> Center{ static_cast<T>( 0 ), static_cast<T>( 0 ) };
        Vec2<T> HalfExtents{ static_cast<T>( 0 ), static_cast<T>( 0 ) };

        static constexpr Rect FromMinMax( Vec2<T> a_Min, Vec2<T> a_Max )
        {
            const Vec2<T> Size = a_Max - a_Min;
            return { a_Min + ( Size * static_cast<T>( 0.5 ) ), Size * static_cast<T>( 0.5 ) };
        }

        constexpr T Top() const { return Center[ 1 ] - HalfExtents[ 1 ]; }
        constexpr T Bottom() const { return Center[ 1 ] + HalfExtents[ 1 ]; }
        constexpr T Left() const { return Center[ 0 ] - HalfExtents[ 0 ]; }
        constexpr T Right() const { return Center[ 0 ] + HalfExtents[ 0 ]; }

        constexpr Vec2<T> TopLeft() const { return Center - HalfExtents; }
        constexpr Vec2<T> TopRight() const { return Vec2<T>{ Center[ 0 ] + HalfExtents[ 0 ], Center[ 1 ] - HalfExtents[ 1 ] }; }
        constexpr Vec2<T> BottomLeft() const { return Vec2<T>{ Center[ 0 ] - HalfExtents[ 0 ], Center[ 1 ] + HalfExtents[ 1 ] }; }
        constexpr Vec2<T> BottomRight() const { return Center + HalfExtents; }

        constexpr Vec2<T> Min() const { return TopLeft(); }
        constexpr Vec2<T> Max() const { return BottomRight(); }

        constexpr Vec2<T> Size() const { return HalfExtents * static_cast<T>( 2 ); }

        constexpr bool Intersects( const Rect<T>& a_Other ) const
        {
            return Left() <= a_Other.Right() && Right() >= a_Other.Left() &&
                   Top() <= a_Other.Bottom() && Bottom() >= a_Other.Top();
        }

        constexpr bool Contains( Vec2<T> a_Point ) const
        {
            Vec2<T> TopLeft = this->TopLeft();
            Vec2<T> BottomRight = this->BottomRight();
            return ( a_Point[ 0 ] >= TopLeft[ 0 ] && a_Point[ 0 ] <= BottomRight[ 0 ] ) &&
                   ( a_Point[ 1 ] >= TopLeft[ 1 ] && a_Point[ 1 ] <= BottomRight[ 1 ] );
        }

        constexpr Rect<T> Intersection( const Rect<T>& a_Other ) const
        {
            const Vec2<T> NewMin{ ( Left() > a_Other.Left() ) ? Left() : a_Other.Left(),
                                   ( Top() > a_Other.Top() ) ? Top() : a_Other.Top() };
            const Vec2<T> NewMax{ ( Right() < a_Other.Right() ) ? Right() : a_Other.Right(),
                                   ( Bottom() < a_Other.Bottom() ) ? Bottom() : a_Other.Bottom() };

            if ( NewMax[ 0 ] < NewMin[ 0 ] || NewMax[ 1 ] < NewMin[ 1 ] )
            {
                return {};
            }

            return Rect<T>::FromMinMax( NewMin, NewMax );
        }
    };

    using Rectf = Rect<f32>;
    using Recti = Rect<i32>;
    using Rectu = Rect<u32>;

} // namespace RatUI
