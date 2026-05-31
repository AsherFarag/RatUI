#pragma once
#include "../Core.h"

namespace RatUI
{

    /**
     * @brief f(Time) -> Time
     */
    struct EaseLinear
    {
        constexpr f32 operator()( f32 a_Time ) const noexcept { return a_Time; }
    };

    /**
     * @brief f(Time) -> Time^2
     */
    struct EaseInQuad
    {
        constexpr f32 operator()( f32 a_Time ) const noexcept { return a_Time * a_Time; }
    };

    /**
     * @brief f(Time) -> Time * (2 - Time)
     */
    struct EaseOutQuad
    {
        constexpr f32 operator()( f32 a_Time ) const noexcept { return a_Time * ( 2.f - a_Time ); }
    };

    /**
     * @brief f(Time) -> 2 * Time^2 if Time < 0.5, otherwise -1 + (4 - 2 * Time) * Time
     */
    struct EaseInOutQuad
    {
        constexpr f32 operator()( f32 a_Time ) const noexcept
        {
            return a_Time < 0.5f 
                   ? 2.f * a_Time * a_Time 
                   : -1.f + ( 4.f - 2.f * a_Time ) * a_Time;
        }
    };

    /**
     * @brief f(Time) -> Time^3
     */
    struct EaseInCubic
    {
        constexpr f32 operator()( f32 a_Time ) const noexcept { return a_Time * a_Time * a_Time; }
    };

    /**
     * @brief f(Time) -> 1 - (1 - Time)^3
     */
    struct EaseOutCubic
    {
        constexpr f32 operator()( f32 a_Time ) const noexcept
        { 
            const f32 u = 1.f - a_Time; 
            return 1.f - u * u * u; 
        }
    };

    /**
     * @brief f(Time) -> 4 * Time^3 if Time < 0.5, otherwise 1 - (-2 * Time + 2)^3 / 2
     */
    struct EaseInOutCubic
    {
        constexpr f32 operator()( f32 a_Time ) const noexcept
        {
            const f32 u = -2.f * a_Time + 2.f;     
            return a_Time < 0.5f
                   ? 4.f * a_Time * a_Time * a_Time
                   : 1.f - ( u * u * u ) * 0.5f;
        }
    };

    /**
     * @brief f(Time) -> 1 + c3 * (Time - 1)^3 + c1 * (Time - 1)^2
     */
    struct EaseOutBack
    {
        constexpr f32 operator()( f32 a_Time ) const noexcept
        {
            constexpr f32 c1 = 1.70158f, c3 = c1 + 1.f;
            const f32 u = a_Time - 1.f;
            return 1.f + c3 * u * u * u + c1 * u * u;
        }
    };

    /**
     * @brief f(Time) -> 1 + c4 * 2^(-10 * Time) * sin((Time * 10 - 0.75) * c4) where c4 = (2 * pi) / 3
     */
    struct EaseOutElastic
    {
        f32 operator()( f32 a_Time ) const noexcept
        {
            // Overshooting spring easing function based on the formula from https://easings.net/#easeOutElastic
            constexpr f32 c4 = ( 2.f * Math::Pi<f32> ) / 3.f;

            if ( a_Time == 0.f ) return 0.f;
            if ( a_Time == 1.f ) return 1.f;
            return 1.f + c4 * std::pow( 2.f, -10.f * a_Time ) 
                            * std::sin( ( a_Time * 10.f - 0.75f ) * c4 );
        }
    };

    /**
     * @brief A variant of common easing functions.
     * Easing functions specify the rate of change of a parameter over time.
     * 
     * Easing functions take in a normalized time value (0 to 1) and output a transformed time value 
     * (also typically between 0 and 1) that can be used to create animation effects.
     */
    struct Easing
    {
        Variant<
            EaseLinear,
            EaseInQuad,
            EaseOutQuad,
            EaseInOutQuad,
            EaseInCubic,
            EaseOutCubic,
            EaseInOutCubic,
            EaseOutBack,
            EaseOutElastic
        > Function;

        constexpr f32 Evaluate( f32 a_Time ) const noexcept
        {
            return std::visit( [&]( const auto& a_Func ) { return a_Func( a_Time ); }, Function );
        }

        constexpr f32 operator()( f32 a_Time ) const noexcept   
        { 
            return Evaluate( a_Time ); 
        }
    };

} // namespace RatUI