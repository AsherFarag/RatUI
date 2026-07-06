#pragma once

/**
 * @file Math.h
 * @brief Provides vector, matrix, color, and rectangle types, along with common mathematical operations and constants.
 * 
 * To use your own math library, create a file named RatUIMathImpl.h that defines the following:
 * @code
 * // Example RatUIMathImpl.h to use GLM as the math library for RatUI
 * namespace RatUI
 * {
 *   struct MathTraits
 *   {
 *    template<typename T, size Dim>
 *    using Vec = glm::vec<Dim, T>;
 * 
 *    template<typename T, size Dim>
 *    using Mat = glm::mat<Dim, Dim, T>;
 * 
 *    etc... (see MathTraits in this file for the full list of required types and members)
 *   };
 * }
 * @endcode
 * 
 * Since vector members can be named differently across libraries (e.g., x/y/z/w vs X/Y/Z/W vs x()/y()/z()/w()), 
 * RatUI expects the following minimal interface for vector types:
 * - operator[](size_t)
 * - operator+(Vec, Vec)
 * - operator-(Vec, Vec)
 * - operator*(Vec, Scalar)
 * - operator/(Vec, Scalar)
 * - operator*(Scalar, Vec)
 * - operator/(Scalar, Vec)
 * - constructible from individual scalar components (e.g., Vec3(x, y, z))
 * 
 * TODO: Math types can be tricky to access (row or column) so create a clean method and clean up all uses of them across the codebase.
 * Required for matrices:
 * - operator[](size_t) returning a column vector
 */

#include "Types.h"
#include <cmath>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <concepts>
#include <numbers>

#if defined(__has_include)
    #if __has_include("RatUIMathImpl.h")
        #include "RatUIMathImpl.h"
        #define RATUI_MATH_IMPL
    #elif __has_include(<RatUIMathImpl.h>)
        #include <RatUIMathImpl.h>
        #define RATUI_MATH_IMPL
    #endif
#endif

#ifndef RATUI_MATH_IMPL

#include "../Extern/nicemath.h"

namespace nm
{
    template<typename S, size_t N>
    constexpr vec<S, N> operator*(const vec<S, N>& a_Left, float a_Scalar)
    {
        return vec<S, N>(a_Left) * S(a_Scalar);
    }
}

namespace RatUI
{
    /**
     * @brief Defines the mathematical types and traits used by RatUI. 
     * By default, it uses the vec and mat types from the nicemath library, 
     * but users can provide their own implementation by defining a custom MathTraits struct in a RatUIMathImpl.h file.
     */
    struct MathTraits
    {
        template<typename T, size Dim>
        using Vec = ::nm::vec<T, Dim>;

        template<typename T, size Dim>
        using Mat = ::nm::mat<T, Dim>;

        using Color = Vec<u8, 4>;

        static constexpr bool c_ColumnMajor = true;

        template<typename T>
        static constexpr T c_Identity = T::identity(); 
    };
} // namespace RatUI

#endif // RATUI_MATH_IMPL

namespace RatUI
{
    template<typename T>
    inline constexpr T Pi = T{ 3.141592653589793 };

    template<typename T>
    constexpr T DegToRad( T a_Degrees ) { return a_Degrees * ( Pi<T> / static_cast<T>( 180 ) ); }

    template<typename T>
    constexpr T RadToDeg( T a_Radians ) { return a_Radians * ( static_cast<T>( 180 ) / Pi<T> ); }

    /**
     * @brief Checks if two floating-point numbers are approximately equal within a specified epsilon tolerance.
     * This function accounts for the relative scale of the numbers and handles special cases like NaN and infinity.
     * @tparam T A floating-point type (e.g., float, double).
     * @param a_Left The first floating-point number to compare.
     * @param a_Right The second floating-point number to compare.
     * @param a_Epsilon The tolerance for comparison. Defaults to the machine epsilon for type T.
     * @return true if the numbers are approximately equal within the specified tolerance, false otherwise.
     */
    template<std::floating_point T>
    constexpr bool IsApproxEqual( T a_Left, T a_Right, T a_Epsilon = std::numeric_limits<T>::epsilon() )
    {
        // Handle NaN
        if ( std::isnan( a_Left ) || std::isnan( a_Right ) ) 
            return false;

        // Handle infinities: only equal if both are infinite and have the same sign
        if ( std::isinf( a_Left ) || std::isinf( a_Right ) )
            return std::isinf( a_Left ) && 
                   std::isinf( a_Right ) && 
                   ( std::signbit( a_Left ) == std::signbit( a_Right ) );

        return std::abs( a_Left - a_Right ) <= a_Epsilon * std::max( std::abs( a_Left ), std::abs( a_Right ) );
    }

    template<typename T>
    constexpr T c_Identity = MathTraits::template c_Identity<T>;

    // === Vector Types ===

    template<typename T, size Dim>
    using Vec = MathTraits::template Vec<T, Dim>;

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

    template<typename T, size Dim>
    using Mat = MathTraits::template Mat<T, Dim>;

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

    // === Color ===

    using Color = MathTraits::Color;

    /** @brief Creates a color from f32 RGBA components in the range [0, 1]. */
    constexpr Color FromColorF32( f32 a_Red, f32 a_Green, f32 a_Blue, f32 a_Alpha = 1.f )
    {
        return Color{
            static_cast<u8>( a_Red   * 255.f ),
            static_cast<u8>( a_Green * 255.f ),
            static_cast<u8>( a_Blue  * 255.f ),
            static_cast<u8>( a_Alpha * 255.f )
        };
    }

	/** @brief Creates a color from a Vec4f RGBA color, where each component is in the range [0, 1]. */
    constexpr Color FromColorF32( Vec4f a_Color )
    {
        return FromColorF32( a_Color[ 0 ], a_Color[ 1 ], a_Color[ 2 ], a_Color[ 3 ] );
	}

	/** @brief Converts a Color to a Vec4f with RGBA components in the range [0, 1]. */
    constexpr Vec4f ToColorF32( Color a_Color )
    {
        return Vec4f{
            static_cast<f32>( a_Color[ 0 ] ) / 255.f,
            static_cast<f32>( a_Color[ 1 ] ) / 255.f,
            static_cast<f32>( a_Color[ 2 ] ) / 255.f,
            static_cast<f32>( a_Color[ 3 ] ) / 255.f
        };
	}

    // === Rectangles ===

    /**
     * @brief A simple axis-aligned rectangle structure defined by its origin (top-left) and size.
     * @tparam T The type of the rectangle's coordinates and dimensions (e.g., float, int).
     */
    template<typename T>
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
		constexpr Vec2<T> Center() const { return Origin + Size / static_cast<T>( 2 ); }

        constexpr Vec2<T> Min() const { return TopLeft(); }
        constexpr Vec2<T> Max() const { return BottomRight(); }

        constexpr bool Intersects( const Rect<T>& a_Other ) const
        {
            return Left() < a_Other.Right() && Right() > a_Other.Left() &&
                   Top() < a_Other.Bottom() && Bottom() > a_Other.Top();
        }

        constexpr bool operator==( const Rect<T>& a_Other ) const
        {
            return Origin == a_Other.Origin && Size == a_Other.Size;
		}

        constexpr bool IsInfinite() const
        {
            return Size[ 0 ] >= Limits<T>::max() && Size[ 1 ] >= Limits<T>::max();
		}

        constexpr Rect<T> Expanded( T a_Amount ) const
        {
            return {
                Vec2<T>{ Origin[ 0 ] - a_Amount, Origin[ 1 ] - a_Amount },
                Vec2<T>{ Size[0] + static_cast<T>( 2 ) * a_Amount, Size[1] + static_cast<T>( 2 ) * a_Amount }
            };
        }

        constexpr Rect<T> Intersection( const Rect<T>& a_Other ) const
        {
            if ( !Intersects( a_Other ) )
                return Rect<T>{};

            Vec2<T> newMin{
                std::max( Left(), a_Other.Left() ),
                std::max( Top(), a_Other.Top() )
			};

            Vec2<T> newMax{
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

        template<typename U> 
        constexpr Rect<U> Cast() const
        {
            return {
                Vec2<U>{ static_cast<U>( Origin[ 0 ] ), static_cast<U>( Origin[ 1 ] ) },
                Vec2<U>{ static_cast<U>( Size[ 0 ] ), static_cast<U>( Size[ 1 ] ) }
            };
		}
    };

    using Rectf = Rect<f32>;
    using Recti = Rect<i32>;
    using Rectu = Rect<u32>;
    using Rectu16 = Rect<u16>;

    namespace Math 
    {
        template<typename T>
        constexpr T Sq( const T& a_Value ) 
        { 
            return a_Value * a_Value; 
        }

        template<typename T>
        constexpr T Clamp( const T& a_Value, const T& a_Min, const T& a_Max )
        {
            return std::max( a_Min, std::min( a_Value, a_Max ) );
        }

        template<typename T>
        constexpr T Lerp( const T& a_Start, const T& a_End, f32 a_T )
        {
            return a_Start + ( a_End - a_Start ) * a_T;
        }

		template<typename Scalar, auto Dim>
        constexpr Scalar Dot( const Vec<Scalar, Dim>& a_Left, const Vec<Scalar, Dim>& a_Right )
        {
            Scalar result = static_cast<Scalar>( 0 );
            for ( auto i = 0; i < Dim; ++i )
                result += a_Left[ i ] * a_Right[ i ];
            return result;
		}

		template<typename Scalar, auto Dim>
        constexpr Scalar LengthSq( const Vec<Scalar, Dim>& a_Vec )
        {
            return Dot( a_Vec, a_Vec );
        }

		template<typename Scalar, auto Dim>
        constexpr Scalar Length(const Vec<Scalar, Dim>& a_Vec)
        {
            return std::sqrt( LengthSq( a_Vec ) );
		}

	} // namespace Math

    namespace Colors
    {
        // - Common Colors

        static constexpr Color Red         = FromColorF32( 1.f, 0.f, 0.f );          // #FF0000
        static constexpr Color Yellow      = FromColorF32( 1.f, 1.f, 0.f );          // #FFFF00
        static constexpr Color Green       = FromColorF32( 0.f, 1.f, 0.f );          // #00FF00
        static constexpr Color Cyan        = FromColorF32( 0.f, 1.f, 1.f );          // #00FFFF
        static constexpr Color Blue        = FromColorF32( 0.f, 0.f, 1.f );          // #0000FF
        static constexpr Color Magenta     = FromColorF32( 1.f, 0.f, 1.f );          // #FF00FF
        static constexpr Color White       = FromColorF32( 1.f, 1.f, 1.f );          // #FFFFFF
        static constexpr Color Gray        = FromColorF32( 0.5f, 0.5f, 0.5f );       // #808080
        static constexpr Color Black       = FromColorF32( 0.f, 0.f, 0.f );          // #000000
        static constexpr Color Transparent = FromColorF32( 0.f, 0.f, 0.f, 0.f );     // #00000000

        // - Light Colors

        static constexpr Color LightRed     = FromColorF32( 1.f, 0.5f, 0.5f );    // #FF8080
        static constexpr Color LightYellow  = FromColorF32( 1.f, 1.f, 0.5f );     // #FFFF80
        static constexpr Color LightGreen   = FromColorF32( 0.5f, 1.f, 0.5f );    // #80FF80
        static constexpr Color LightCyan    = FromColorF32( 0.5f, 1.f, 1.f );     // #80FFFF
        static constexpr Color LightBlue    = FromColorF32( 0.5f, 0.5f, 1.f );    // #8080FF
        static constexpr Color LightMagenta = FromColorF32( 1.f, 0.5f, 1.f );     // #FF80FF
        static constexpr Color LightGray    = FromColorF32( 0.75f, 0.75f, 0.75f );// #BFBFBF

        // - Dark Colors

        static constexpr Color DarkRed     = FromColorF32( 0.5f, 0.f, 0.f );      // #800000
        static constexpr Color DarkYellow  = FromColorF32( 0.5f, 0.5f, 0.f );     // #808000
        static constexpr Color DarkGreen   = FromColorF32( 0.f, 0.5f, 0.f );      // #008000
        static constexpr Color DarkCyan    = FromColorF32( 0.f, 0.5f, 0.5f );     // #008080
        static constexpr Color DarkBlue    = FromColorF32( 0.f, 0.f, 0.5f );      // #000080
        static constexpr Color DarkMagenta = FromColorF32( 0.5f, 0.f, 0.5f );     // #800080
        static constexpr Color DarkGray    = FromColorF32( 0.25f, 0.25f, 0.25f ); // #404040

        // - Pretty Colors

        static constexpr Color Orange    = FromColorF32( 1.f, 0.65f, 0.f );     // #FFA500
        static constexpr Color Pink      = FromColorF32( 1.f, 0.75f, 0.8f );    // #FFC0CB
        static constexpr Color Purple    = FromColorF32( 0.5f, 0.f, 0.5f );     // #800080
        static constexpr Color Teal      = FromColorF32( 0.f, 0.5f, 0.5f );     // #008080
        static constexpr Color Lime      = FromColorF32( 0.75f, 1.f, 0.f );     // #ecffec
        static constexpr Color Indigo    = FromColorF32( 0.29f, 0.f, 0.51f );   // #4B0082
        static constexpr Color Violet    = FromColorF32( 0.93f, 0.51f, 0.93f ); // #DDA0DD
        static constexpr Color Brown     = FromColorF32( 0.65f, 0.16f, 0.16f ); // #A52A2A
        static constexpr Color Maroon    = FromColorF32( 0.5f, 0.f, 0.f );      // #800000
        static constexpr Color Olive     = FromColorF32( 0.5f, 0.5f, 0.f );     // #808000
        static constexpr Color Navy      = FromColorF32( 0.f, 0.f, 0.5f );      // #000080
        static constexpr Color Silver    = FromColorF32( 0.75f, 0.75f, 0.75f ); // #C0C0C0
        static constexpr Color Gold      = FromColorF32( 1.f, 0.84f, 0.f );     // #FFD700
        static constexpr Color Salmon    = FromColorF32( 0.98f, 0.5f, 0.45f );  // #FA8072
        static constexpr Color Coral     = FromColorF32( 1.f, 0.5f, 0.31f );    // #FF7F50
        static constexpr Color Turquoise = FromColorF32( 0.25f, 0.88f, 0.82f ); // #40E0D0
        static constexpr Color PowderBlue= FromColorF32( 0.69f, 0.88f, 0.9f );  // #B0E0E6
        static constexpr Color LightPink = FromColorF32( 1.f, 0.71f, 0.76f );   // #FADADD

        // - UI Surface Colors (dark theme base)
        static constexpr Color Surface900   = FromColorF32( 0.067f, 0.067f, 0.078f ); // #111113 - deepest bg
        static constexpr Color Surface800   = FromColorF32( 0.110f, 0.110f, 0.133f ); // #1C1C22 - panel bg
        static constexpr Color Surface700   = FromColorF32( 0.157f, 0.157f, 0.188f ); // #282830 - card bg
        static constexpr Color Surface600   = FromColorF32( 0.208f, 0.208f, 0.247f ); // #35353F - elevated card
        static constexpr Color Surface500   = FromColorF32( 0.275f, 0.275f, 0.322f ); // #464652 - border/divider

        // - UI Accent Colors
        static constexpr Color AccentBlue       = FromColorF32( 0.239f, 0.510f, 1.000f ); // #3D82FF - primary action
        static constexpr Color AccentBlueDim    = FromColorF32( 0.149f, 0.337f, 0.714f ); // #2656B6 - hover state
        static constexpr Color AccentPurple     = FromColorF32( 0.498f, 0.357f, 1.000f ); // #7F5BFF - secondary accent
        static constexpr Color AccentViolet     = FromColorF32( 0.686f, 0.404f, 1.000f ); // #AF67FF - highlight
        static constexpr Color AccentEmerald    = FromColorF32( 0.098f, 0.780f, 0.522f ); // #19C785 - success
        static constexpr Color AccentAmber      = FromColorF32( 1.000f, 0.718f, 0.137f ); // #FFB723 - warning
        static constexpr Color AccentRose       = FromColorF32( 1.000f, 0.294f, 0.404f ); // #FF4B67 - error/danger
        static constexpr Color AccentSky        = FromColorF32( 0.220f, 0.780f, 1.000f ); // #38C7FF - info

        // - UI Text Colors
        static constexpr Color TextPrimary      = FromColorF32( 0.929f, 0.929f, 0.961f ); // #EDECF5 - primary text
        static constexpr Color TextSecondary    = FromColorF32( 0.588f, 0.588f, 0.647f ); // #9696A5 - muted text
        static constexpr Color TextDisabled     = FromColorF32( 0.357f, 0.357f, 0.400f ); // #5B5B66 - disabled text
    };

} // namespace RatUI
