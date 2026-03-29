#pragma once

#include "../Core.h"
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>
#include <concepts>
#include <numbers>

// TODO: Might make more sense to have a Vec and Mat traits class with the specified operations but this is probably good enough for now. 

/**
 * If you want to use your own math library, define RATUI_MATH_IMPL and the following macros before including this header:
 * - RATUI_VEC_IMPL<T, Dim>: A template for a vector type with scalar components, where T is the scalar type and Dim is the dimensionality (e.g., 2, 3, or 4).
 * - RATUI_MATRIX_IMPL<T, Dim>: A template for a square matrix type with scalar components, where T is the scalar type and Dim is the dimensionality (e.g., 2, 3, or 4).
 * - RATUI_COLOR_IMPL: A type representing a color (e.g., RGBA), which should be constructible from 4 scalar components (e.g., r, g, b, a).
 * 
 * Required for RATUI_VEC_IMPL & RATUI_COLOR_IMPL:
 * - operator[](size_t)
 * - operator+(Vec, Vec)
 * - operator-(Vec, Vec)
 * - operator*(Vec, Scalar)
 * - operator/(Vec, Scalar)
 * - operator*(Scalar, Vec)
 * - operator/(Scalar, Vec)
 * - constructible from individual scalar components (e.g., Vec3(float x, float y, float z))
 * 
 * Required for RATUI_MATRIX_IMPL:
 * - operator[](size_t) returning a column vector
 * 
 * @note If you're using GLM, you'll need to do:
 * #define RATUI_VEC_IMPL(T, Dim) glm::vec<Dim, T>
 * #define RATUI_MATRIX_IMPL(T, Dim) glm::mat<Dim, Dim, T>
 */
#ifndef RATUI_MATH_IMPL

    #define RATUI_BUILTIN_MATH

    #include "../Extern/nicemath.h"
    #define RATUI_VEC_IMPL ::nm::vec
    #define RATUI_MATRIX_IMPL ::nm::mat
    #define RATUI_COLOR_IMPL RATUI_VEC_IMPL<RatUI::f32, 4>

#endif // RATUI_MATH_IMPL

namespace RatUI
{
    template<typename T>
    inline constexpr T Pi = std::numbers::pi_v<T>;

    template<typename T>
    constexpr T DegToRad( T a_Degrees ) { return a_Degrees * ( Pi<T> / static_cast<T>( 180 ) ); }

    template<typename T>
    constexpr T RadToDeg( T a_Radians ) { return a_Radians * ( static_cast<T>( 180 ) / Pi<T> ); }

    // === Vector Types ===

    template<typename T, size Dim> requires std::is_arithmetic_v<T>
    using Vec = RATUI_VEC_IMPL<T, Dim>;

    template<typename T> using Vec2 = Vec<T, 2>;
    template<typename T> using Vec3 = Vec<T, 3>;
    template<typename T> using Vec4 = Vec<T, 4>;

    using Vec2f = Vec<f32, 2>;
    using Vec3f = Vec<f32, 3>;
    using Vec4f = Vec<f32, 4>;
    using Vec2i = Vec<i32, 2>;
    using Vec3i = Vec<i32, 3>;
    using Vec4i = Vec<i32, 4>;
    using Vec2u = Vec<u32, 2>;
    using Vec3u = Vec<u32, 3>;
    using Vec4u = Vec<u32, 4>;

    // === Matrix Types ===

    template<typename T, size Dim> requires std::is_arithmetic_v<T>
    using Mat = RATUI_MATRIX_IMPL<T, Dim>;

    template<typename T> using Mat2 = Mat<T, 2>;
    template<typename T> using Mat3 = Mat<T, 3>;
    template<typename T> using Mat4 = Mat<T, 4>;

    using Mat2f = Mat<f32, 2>;
    using Mat3f = Mat<f32, 3>;
    using Mat4f = Mat<f32, 4>;
    using Mat2i = Mat<i32, 2>;
    using Mat3i = Mat<i32, 3>;
    using Mat4i = Mat<i32, 4>;
    using Mat2u = Mat<u32, 2>;
    using Mat3u = Mat<u32, 3>;
    using Mat4u = Mat<u32, 4>;

    template<typename T> requires requires { T::identity(); }
    constexpr T c_Identity = T::identity();


    // === Color ===

    using Colorf = RATUI_COLOR_IMPL;

    /** @brief Creates a color from f32 RGBA components in the range [0, 1]. */
    constexpr Colorf MakeColorF32( f32 a_Red, f32 a_Green, f32 a_Blue, f32 a_Alpha = 1.f );

    /** @brief Creates a color from u8 RGBA components in the range [0, 255]. */
    constexpr Colorf MakeColorU8( u8 a_Red, u8 a_Green, u8 a_Blue, u8 a_Alpha = 255 );

#ifdef RATUI_BUILTIN_MATH

    inline constexpr Colorf MakeColorF32( f32 a_Red, f32 a_Green, f32 a_Blue, f32 a_Alpha )
    {
        // Built-in Color is Vec4<f32> with RGBA components in [0, 1], so we can just construct it directly
        return Colorf{ a_Red, a_Green, a_Blue, a_Alpha };
    }

    inline constexpr Colorf MakeColorU8( u8 a_Red, u8 a_Green, u8 a_Blue, u8 a_Alpha )
    {
        // Built-in Color is Vec4<f32> with RGBA components in [0, 1], so we need to convert from [0, 255] to [0, 1]
        return Colorf{
            static_cast<f32>(a_Red) / 255.f,
            static_cast<f32>(a_Green) / 255.f,
            static_cast<f32>(a_Blue) / 255.f,
            static_cast<f32>(a_Alpha) / 255.f
        };
    }

#endif // RATUI_BUILTIN_MATH

    // === Rectangles ===

    /**
     * @brief A simple axis-aligned rectangle structure defined by its origin (top-left) and size.
     * @tparam T The type of the rectangle's coordinates and dimensions (e.g., float, int).
     */
    template<typename T> requires std::is_arithmetic_v<T>
    struct Rect
    {
        using ValueType = T;
        using Vec2Type = Vec<T, 2>;

        Vec2<T> Origin{ static_cast<T>( 0 ), static_cast<T>( 0 ) };
        Vec2<T> Size{ static_cast<T>( 0 ), static_cast<T>( 0 ) };

        constexpr T Width() const { return Size[ 0 ]; }
        constexpr T Height() const { return Size[ 1 ]; }
        constexpr T Top() const { return Origin[ 1 ]; }
        constexpr T Bottom() const { return Origin[ 1 ] + Size[ 1 ]; }
        constexpr T Left() const { return Origin[ 0 ]; }
        constexpr T Right() const { return Origin[ 0 ] + Size[ 0 ]; }

        constexpr Vec2<T> TopLeft() const { return Origin; }
        constexpr Vec2<T> TopRight() const { return Vec2<T>{ Origin[ 0 ] + Size[ 0 ], Origin[ 1 ] }; }
        constexpr Vec2<T> BottomLeft() const { return Vec2<T>{ Origin[ 0 ], Origin[ 1 ] + Size[ 1 ] }; }
        constexpr Vec2<T> BottomRight() const { return Origin + Size; }
		constexpr Vec2<T> Center() const { return Origin + Size * static_cast<T>( 0.5 ); }

        constexpr Vec2<T> Min() const { return TopLeft(); }
        constexpr Vec2<T> Max() const { return BottomRight(); }

        constexpr bool Intersects( const Rect<T>& a_Other ) const
        {
            return Left() < a_Other.Right() && Right() > a_Other.Left() &&
                   Top() < a_Other.Bottom() && Bottom() > a_Other.Top();
        }

        constexpr Rect<T> Expanded( T a_Amount ) const
        {
            return {
                Vec2<T>{ Origin[ 0 ] - a_Amount, Origin[ 1 ] - a_Amount },
                Vec2<T>{ Size[ 0 ] + static_cast<T>( 2 ) * a_Amount, Size[ 1 ] + static_cast<T>( 2 ) * a_Amount }
            };
        }

        constexpr Rect<T> Intersection( const Rect<T>& a_Other ) const
        {
            if ( !Intersects( a_Other ) )
                return Rect<T>{};

            Vec2f newMin{
                std::max( Left(), a_Other.Left() ),
                std::max( Top(), a_Other.Top() )
			};

            Vec2f newMax{
                std::min( Right(), a_Other.Right() ),
                std::min( Bottom(), a_Other.Bottom() )
            };

            return FromMinMax( newMin, newMax );
        }

        constexpr bool Contains( Vec2<T> a_Point ) const
        {
            return a_Point[ 0 ] >= Left() && a_Point[ 0 ] <= Right() &&
                   a_Point[ 1 ] >= Top() && a_Point[ 1 ] <= Bottom();
        }

        static constexpr Rect FromMinMax( Vec2<T> a_Min, Vec2<T> a_Max )
        {
            return { a_Min, a_Max - a_Min };
        }

        static constexpr Rect FromCenter( Vec2<T> a_Center, Vec2<T> a_Size )
        {
            Vec2<T> halfSize = a_Size * static_cast<T>( 0.5 );
            return { a_Center - halfSize, a_Size };
        }

        static constexpr Rect Infinite()
        {
            return {
                Vec2<T>{ Limits<T>::lowest(), Limits<T>::lowest() },
                Vec2<T>{ Limits<T>::max(), Limits<T>::max() }
            };
        }
    };

    using Rectf = Rect<f32>;
    using Recti = Rect<i32>;
    using Rectu = Rect<u32>;

    namespace Colors
    {
        // - Common Colors

        inline constexpr Colorf Red         = MakeColorF32( 1.f, 0.f, 0.f );
        inline constexpr Colorf Yellow      = MakeColorF32( 1.f, 1.f, 0.f );
        inline constexpr Colorf Green       = MakeColorF32( 0.f, 1.f, 0.f );
        inline constexpr Colorf Cyan        = MakeColorF32( 0.f, 1.f, 1.f );
        inline constexpr Colorf Blue        = MakeColorF32( 0.f, 0.f, 1.f );
        inline constexpr Colorf Magenta     = MakeColorF32( 1.f, 0.f, 1.f );
        inline constexpr Colorf White       = MakeColorF32( 1.f, 1.f, 1.f, 1.f );
        inline constexpr Colorf Gray        = MakeColorF32( 0.5f, 0.5f, 0.5f, 1.f );
        inline constexpr Colorf Black       = MakeColorF32( 0.f, 0.f, 0.f, 1.f );
        inline constexpr Colorf Transparent = MakeColorF32( 0.f, 0.f, 0.f, 0.f );

        // - Light Colors

        inline constexpr Colorf LightRed     = MakeColorF32( 1.f, 0.5f, 0.5f, 1.f );
        inline constexpr Colorf LightYellow  = MakeColorF32( 1.f, 1.f, 0.5f, 1.f );
        inline constexpr Colorf LightGreen   = MakeColorF32( 0.5f, 1.f, 0.5f, 1.f );
        inline constexpr Colorf LightCyan    = MakeColorF32( 0.5f, 1.f, 1.f, 1.f );
        inline constexpr Colorf LightBlue    = MakeColorF32( 0.5f, 0.5f, 1.f, 1.f );
        inline constexpr Colorf LightMagenta = MakeColorF32( 1.f, 0.5f, 1.f, 1.f );
        inline constexpr Colorf LightGray    = MakeColorF32( 0.75f, 0.75f, 0.75f, 1.f );

        // - Dark Colors

        inline constexpr Colorf DarkRed     = MakeColorF32( 0.5f, 0.f, 0.f, 1.f );
        inline constexpr Colorf DarkYellow  = MakeColorF32( 0.5f, 0.5f, 0.f, 1.f );
        inline constexpr Colorf DarkGreen   = MakeColorF32( 0.f, 0.5f, 0.f, 1.f );
        inline constexpr Colorf DarkCyan    = MakeColorF32( 0.f, 0.5f, 0.5f, 1.f );
        inline constexpr Colorf DarkBlue    = MakeColorF32( 0.f, 0.f, 0.5f, 1.f );
        inline constexpr Colorf DarkMagenta = MakeColorF32( 0.5f, 0.f, 0.5f, 1.f );
        inline constexpr Colorf DarkGray    = MakeColorF32( 0.25f, 0.25f, 0.25f, 1.f );

        // - Pretty Colors

        inline constexpr Colorf Orange    = MakeColorF32( 1.f, 0.65f, 0.f, 1.f );
        inline constexpr Colorf Pink      = MakeColorF32( 1.f, 0.75f, 0.8f, 1.f );
        inline constexpr Colorf Purple    = MakeColorF32( 0.5f, 0.f, 0.5f, 1.f );
        inline constexpr Colorf Teal      = MakeColorF32( 0.f, 0.5f, 0.5f, 1.f );
        inline constexpr Colorf Lime      = MakeColorF32( 0.75f, 1.f, 0.f, 1.f );
        inline constexpr Colorf Indigo    = MakeColorF32( 0.29f, 0.f, 0.51f, 1.f );
        inline constexpr Colorf Violet    = MakeColorF32( 0.93f, 0.51f, 0.93f, 1.f );
        inline constexpr Colorf Brown     = MakeColorF32( 0.65f, 0.16f, 0.16f, 1.f );
        inline constexpr Colorf Maroon    = MakeColorF32( 0.5f, 0.f, 0.f, 1.f );
        inline constexpr Colorf Olive     = MakeColorF32( 0.5f, 0.5f, 0.f, 1.f );
        inline constexpr Colorf Navy      = MakeColorF32( 0.f, 0.f, 0.5f, 1.f );
        inline constexpr Colorf Silver    = MakeColorF32( 0.75f, 0.75f, 0.75f, 1.f );
        inline constexpr Colorf Gold      = MakeColorF32( 1.f, 0.84f, 0.f, 1.f );
        inline constexpr Colorf Salmon    = MakeColorF32( 0.98f, 0.5f, 0.45f, 1.f );
        inline constexpr Colorf Coral     = MakeColorF32( 1.f, 0.5f, 0.31f, 1.f );
        inline constexpr Colorf Turquoise = MakeColorF32( 0.25f, 0.88f, 0.82f, 1.f );
        inline constexpr Colorf PowderBlue= MakeColorF32( 0.69f, 0.88f, 0.9f, 1.f );
        inline constexpr Colorf LightPink = MakeColorF32( 1.f, 0.71f, 0.76f, 1.f );

    } // namespace Colors

} // namespace RatUI
