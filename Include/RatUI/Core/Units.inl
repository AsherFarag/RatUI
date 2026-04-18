#pragma once
#include "../Core.h"

namespace RatUI
{
    /**
     * @brief Unit template for type-safe representation of various units of measurement in RatUI.
     * 
     */
    template<typename _Tag>
    struct UnitBase
    {
        using ValueType = f32;

        ValueType Value;

        constexpr UnitBase() : Value( static_cast<ValueType>( 0 ) ) {}
        constexpr explicit UnitBase( ValueType a_Value ) : Value( a_Value ) {}

        constexpr UnitBase  operator+ ( UnitBase a_Other )       const { return UnitBase{ Value + a_Other.Value }; }
        constexpr UnitBase  operator- ( UnitBase a_Other )       const { return UnitBase{ Value - a_Other.Value }; }
        constexpr UnitBase  operator* ( ValueType a_Scalar )     const { return UnitBase{ Value * a_Scalar }; }
        constexpr UnitBase  operator/ ( ValueType a_Scalar )     const { return UnitBase{ Value / a_Scalar }; }
        constexpr UnitBase& operator+=( UnitBase a_Other )             { Value += a_Other.Value; return *this; }
        constexpr UnitBase& operator-=( UnitBase a_Other )             { Value -= a_Other.Value; return *this; }
        constexpr UnitBase& operator*=( ValueType a_Scalar )           { Value *= a_Scalar; return *this; }
        constexpr UnitBase& operator/=( ValueType a_Scalar )           { Value /= a_Scalar; return *this; }

		constexpr bool operator== ( UnitBase a_Other ) const { return Value == a_Other.Value; }
		constexpr auto operator<=>( UnitBase a_Other ) const { return Value <=> a_Other.Value; }

        constexpr explicit operator ValueType() const { return Value; }
        constexpr f32 ToFloat()                 const { return Value; }
    };

    template <typename _Tag> 
    constexpr inline UnitBase<_Tag> operator*( f32 a_Scalar, UnitBase<_Tag> a_Unit ) { return a_Unit * a_Scalar; }

    /**
     * @brief Normalized font-space unit (EM space).
     * Represents measurements in the font’s design coordinate system.
     * Typically normalized such that: 1.0 = 1 EM (font units / unitsPerEM)
     *
     * Used for glyph metrics such as advance, bearing, and glyph bounds.
     *
     * @note Independent of DPI, UI scaling, and rendering resolution.
     */
    using FontUnit = UnitBase<struct FontUnitTag>;

    /**
     * @brief Resolution-independent layout unit.
     * Represents logical UI space used for layout (positions, sizes, margins).
     * It is independent of framebuffer resolution and font metrics.
     * It is converted to Pixel space using the current DPI scale (and optional UI scale factor).
     * @note This is NOT a physical unit. It does not correspond to screen pixels directly.
     */
    using Unit = UnitBase<struct UnitTag>;

    /**
     * @brief Physical screen pixels used for rendering.
     * Represents final raster-space coordinates after all scaling.
     * Derived from Unit using DPI scaling (and optional UI scaling).
     * Stored as float to preserve subpixel precision for high-quality rendering.
     */
    using Pixel = UnitBase<struct PixelTag>;

    // - Unit Conversion chain:
    //      FontUnit --(font metrics)--> Unit --(DPI/UI scale)--> Pixel

    /** @brief Converts a FontUnit to Unit space using the font's unitsPerEM metric. */
    inline constexpr FontUnit ToFontUnit( Unit a_Unit, f32 a_UnitsPerEM )
    {
        return FontUnit{ a_Unit.ToFloat() * a_UnitsPerEM };
    }

    /** @brief Converts a FontUnit to Unit space using the font's unitsPerEM metric. */
    inline constexpr Unit ToUnit( FontUnit a_FontUnit, f32 a_UnitsPerEM )
    {
        return Unit{ a_FontUnit.ToFloat() / a_UnitsPerEM };
    }

    /** @brief Converts a Pixel back to Unit space using the specified DPI scale. */
    inline constexpr Unit ToUnit( Pixel a_Pixel, f32 a_DPIScale )
    {
        return Unit{ a_Pixel.ToFloat() / a_DPIScale };
    }

    /** @brief Converts a Unit to Pixel space using the specified DPI scale. */
    inline constexpr Pixel ToPixel( Unit a_Unit, f32 a_DPIScale )
    {
        return Pixel{ a_Unit.ToFloat() * a_DPIScale };
    }

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

        template<typename U>
		constexpr Radians( Radians<U> a_Other ) : Value( static_cast<T>( a_Other.Value ) ) {}

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

    template<typename T>
    struct Degrees
    {
        using ValueType = T;

        T Value; ///< The value of the angle in degrees.

        // === Constructors ===

        constexpr Degrees() : Value(static_cast<T>( 0 )) {}
        constexpr explicit Degrees( T a_Value ) : Value( a_Value ) {}
        constexpr explicit Degrees( Radians<T> a_Radians ) : Value( RadToDeg( a_Radians.Value ) ) {}

		template<typename U>
		constexpr Degrees( Degrees<U> a_Other ) : Value( static_cast<T>( a_Other.Value ) ) {}

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
    using Radiansf = Radians<f32>;

    /** 
     * @brief A namespace containing literals for RatUI units.
     */
    namespace Literals
    {
        constexpr Radians<f64> operator"" _rad( long double a_Value )        { return Radians<f64>{ static_cast<f64>( a_Value ) }; }
        constexpr Degrees<f64> operator"" _deg( long double a_Value )        { return Degrees<f64>{ static_cast<f64>( a_Value ) }; }
        constexpr Radians<f64> operator"" _rad( unsigned long long a_Value ) { return Radians<f64>{ static_cast<f64>( a_Value ) }; }
        constexpr Degrees<f64> operator"" _deg( unsigned long long a_Value ) { return Degrees<f64>{ static_cast<f64>( a_Value ) }; }

        constexpr FontUnit     operator"" _fu ( long double a_Value )        { return FontUnit{ static_cast<f32>( a_Value ) }; }
        constexpr FontUnit     operator"" _fu ( unsigned long long a_Value ) { return FontUnit{ static_cast<f32>( a_Value ) }; }
        constexpr Unit         operator"" _u  ( long double a_Value )        { return Unit{  static_cast<f32>( a_Value ) }; }
        constexpr Unit         operator"" _u  ( unsigned long long a_Value ) { return Unit{  static_cast<f32>( a_Value ) }; }
        constexpr Pixel        operator"" _px ( unsigned long long a_Value ) { return Pixel{ static_cast<f32>( a_Value ) }; }
        constexpr Pixel        operator"" _px ( long double a_Value )        { return Pixel{ static_cast<f32>( a_Value ) }; }
    }

    using namespace Literals;

    // =========================================================
    // Inline Implementations
    // =========================================================

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