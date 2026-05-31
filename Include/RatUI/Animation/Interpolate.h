#pragma once
#include "../Core.h"
#include "../Layout/Layout.h"

namespace RatUI
{
    /**
     * @brief Linearly interpolates between two values of type T based on the given time parameter.
     * @tparam T The type of values to interpolate. Must support addition, subtraction, and multiplication by a float.
     * @param a_A The starting value (corresponding to time = 0).
     * @param a_B The ending value (corresponding to time = 1).
     * @param a_Time The interpolation parameter, typically in the range [0, 1]. Values outside this range will extrapolate.
     * @return The interpolated value between a_A and a_B based on a_Time.
     */
    template<typename T>
    constexpr T Lerp( const T& a_A, const T& a_B, f32 a_Time )
    {
        return a_A + (a_B - a_A) * a_Time;
    }

    template<>
    constexpr inline Color Lerp(const Color& a_A, const Color& a_B, f32 a_Time)
    {
        return Color{
            static_cast<u8>(a_A[0] + (a_B[0] - a_A[0]) * a_Time),
            static_cast<u8>(a_A[1] + (a_B[1] - a_A[1]) * a_Time),
            static_cast<u8>(a_A[2] + (a_B[2] - a_A[2]) * a_Time),
            static_cast<u8>(a_A[3] + (a_B[3] - a_A[3]) * a_Time),
        };
    }

    template<>
    constexpr inline CornerRounding Lerp(const CornerRounding& a_A, const CornerRounding& a_B, f32 a_Time)
    {
        return {
            Lerp(a_A.TopLeft,     a_B.TopLeft,     a_Time),
            Lerp(a_A.TopRight,    a_B.TopRight,    a_Time),
            Lerp(a_A.BottomLeft,  a_B.BottomLeft,  a_Time),
            Lerp(a_A.BottomRight, a_B.BottomRight, a_Time),
        };
    }
} // namespace RatUI::Animation