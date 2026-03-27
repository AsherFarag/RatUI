#pragma once
#include "../Core.h"

namespace RatUI
{
    /** 
     * @brief A unit representing a pixel, similar to CSS pixels.
     * @note _px literals for this type are defined in the Literals namespace, e.g., 16_px.
     */
    struct Pixel
    {
        using ValueType = u32;

        u32 Value; ///< The value of the pixel measurement.

        // === Constructors ===

        constexpr Pixel() : Value( 0 ) {}
        constexpr explicit Pixel( u32 a_Value ) : Value( a_Value ) {}

        // === Comparison Operators ===

        constexpr auto operator<=>( const Pixel& ) const = default;
        constexpr bool operator==( const Pixel& ) const = default;

        // === Arithmetic Operators ===

        constexpr Pixel operator-() const { return Pixel{ static_cast<u32>( -static_cast<i32>( Value ) ) }; }
        constexpr Pixel operator+( Pixel a_Other ) const { return Pixel{ Value + a_Other.Value }; }
        constexpr Pixel operator-( Pixel a_Other ) const { return Pixel{ Value - a_Other.Value }; }
        constexpr Pixel operator*( f32 a_Scalar ) const { return Pixel{ static_cast<u32>( Value * a_Scalar ) }; }
        constexpr Pixel operator/( f32 a_Scalar ) const { return Pixel{ static_cast<u32>( Value / a_Scalar ) }; }
        constexpr Pixel& operator+=( Pixel a_Other ) { Value += a_Other.Value; return *this; }
        constexpr Pixel& operator-=( Pixel a_Other ) { Value -= a_Other.Value; return *this; }
        constexpr Pixel& operator*=( f32 a_Scalar ) { Value = static_cast<u32>( Value * a_Scalar ); return *this; }
        constexpr Pixel& operator/=( f32 a_Scalar ) { Value = static_cast<u32>( Value / a_Scalar ); return *this; }
    };

    constexpr Pixel operator*( f32 a_Scalar, Pixel a_Pixel ) { return a_Pixel * a_Scalar; }

    template<typename T>
    struct Degrees;

    /** 
     * @brief A unit representing radians.
     * @note _rad literals for this type are defined in the Literals namespace, e.g., 3.14159_rad.
     */
    template<typename T>
    struct Radians
    {
        using ValueType = T;

        T Value; ///< The value of the angle in radians.

        // === Constructors ===

        constexpr Radians() : Value(static_cast<T>( 0 )) {}
        constexpr explicit Radians( T a_Value ) : Value( a_Value ) {}
        constexpr explicit Radians( Degrees<T> a_Degrees );

        // === Conversion Operators ===

        constexpr explicit operator Degrees<T>() const;

        // === Comparison Operators ===

        constexpr auto operator<=>( const Radians& ) const = default;
        constexpr bool operator==( const Radians& ) const = default;

        // === Arithmetic Operators ===

        constexpr Radians operator-() const { return Radians{ -Value }; }
        constexpr Radians operator+( Radians a_Other ) const { return Radians{ Value + a_Other.Value }; }
        constexpr Radians operator-( Radians a_Other ) const { return Radians{ Value - a_Other.Value }; }
        constexpr Radians operator*( T a_Scalar ) const { return Radians{ Value * a_Scalar }; }
        constexpr Radians operator/( T a_Scalar ) const { return Radians{ Value / a_Scalar }; }
        constexpr Radians& operator+=( Radians a_Other ) { Value += a_Other.Value; return *this; }
        constexpr Radians& operator-=( Radians a_Other ) { Value -= a_Other.Value; return *this; }
        constexpr Radians& operator*=( T a_Scalar ) { Value *= a_Scalar; return *this; }
        constexpr Radians& operator/=( T a_Scalar ) { Value /= a_Scalar; return *this; }
    };

    template<typename T>
    constexpr Radians<T> operator*( T a_Scalar, Radians<T> a_Radians ) { return a_Radians * a_Scalar; }

    using Radiansf = Radians<f32>;

    template<typename T>
    struct Degrees
    {
        using ValueType = T;

        T Value; ///< The value of the angle in degrees.

        // === Constructors ===

        constexpr Degrees() : Value(static_cast<T>( 0 )) {}
        constexpr explicit Degrees( T a_Value ) : Value( a_Value ) {}
        constexpr explicit Degrees( Radians<T> a_Radians ) : Value( RadToDeg( a_Radians.Value ) ) {}

        // === Conversion Operators ===

        constexpr explicit operator Radians<T>() const { return Radians<T>{ DegToRad( Value ) }; }

        // === Comparison Operators ===

        constexpr auto operator<=>( const Degrees& ) const = default;
        constexpr bool operator==( const Degrees& ) const = default;

        // === Arithmetic Operators ===

        constexpr Degrees operator-() const { return Degrees{ -Value }; }
        constexpr Degrees operator+( Degrees a_Other ) const { return Degrees{ Value + a_Other.Value }; }
        constexpr Degrees operator-( Degrees a_Other ) const { return Degrees{ Value - a_Other.Value }; }
        constexpr Degrees operator*( T a_Scalar ) const { return Degrees{ Value * a_Scalar }; }
        constexpr Degrees operator/( T a_Scalar ) const { return Degrees{ Value / a_Scalar }; }
        constexpr Degrees& operator+=( Degrees a_Other ) { Value += a_Other.Value; return *this; }
        constexpr Degrees& operator-=( Degrees a_Other ) { Value -= a_Other.Value; return *this; }
        constexpr Degrees& operator*=( T a_Scalar ) { Value *= a_Scalar; return *this; }
        constexpr Degrees& operator/=( T a_Scalar ) { Value /= a_Scalar; return *this; }
    };

    template<typename T>
    constexpr Degrees<T> operator*( T a_Scalar, Degrees<T> a_Degrees ) { return a_Degrees * a_Scalar; }

    using Degreesf = Degrees<f32>;

    /** 
     * @brief A namespace containing user-defined literals for RatUI units.
     * @note Write 'using namespace RatUI::Literals;' to bring these literals into scope.
     */
    namespace Literals
    {
        constexpr Radians<f64> operator"" _rad( long double a_Value ) { return Radians<f64>{ static_cast<f64>( a_Value ) }; }
        constexpr Degrees<f64> operator"" _deg( long double a_Value ) { return Degrees<f64>{ static_cast<f64>( a_Value ) }; }
        constexpr Radians<f64> operator"" _rad( unsigned long long a_Value ) { return Radians<f64>{ static_cast<f64>( a_Value ) }; }
        constexpr Degrees<f64> operator"" _deg( unsigned long long a_Value ) { return Degrees<f64>{ static_cast<f64>( a_Value ) }; }
        constexpr Pixel operator"" _px( unsigned long long a_Value ) { return Pixel{ static_cast<u32>( a_Value ) }; }
    }
    using namespace Literals; // Bring literals into RatUI namespace for convenience.

    // === Radians Impl ===

    template<typename T>
    constexpr Radians<T>::Radians( Degrees<T> a_Degrees )
     : Value( DegToRad( a_Degrees.Value ) )
    {
    }

    template<typename T>
    constexpr Radians<T>::operator Degrees<T>() const
    {
        return Degrees<T>{ RadToDeg( Value ) };
    }
}