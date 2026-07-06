#pragma once
#include "../Core.h"
#include "../Layout/Layout.h"
#include "../Renderer/RenderTransform.h"

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
    RATUI_NODISCARD constexpr T Lerp( const T& a_A, const T& a_B, f32 a_Time );

    template<typename T> requires
        requires( T a, T b, f32 t ) { { a + ( b - a ) * t } -> std::convertible_to<T>; }
	RATUI_NODISCARD constexpr inline
	T Lerp( const T& a_A, const T& a_B, f32 a_Time )
	{
		return a_A + ( a_B - a_A ) * a_Time;
	}

    template<>
    RATUI_NODISCARD constexpr inline 
    Color Lerp(const Color& a_A, const Color& a_B, f32 a_Time)
    {
        return Color{
            static_cast<u8>(a_A[0] + (a_B[0] - a_A[0]) * a_Time),
            static_cast<u8>(a_A[1] + (a_B[1] - a_A[1]) * a_Time),
            static_cast<u8>(a_A[2] + (a_B[2] - a_A[2]) * a_Time),
            static_cast<u8>(a_A[3] + (a_B[3] - a_A[3]) * a_Time),
        };
    }

    template<>
    RATUI_NODISCARD constexpr inline 
    CornerRounding Lerp(const CornerRounding& a_A, const CornerRounding& a_B, f32 a_Time)
    {
        return {
            Lerp(a_A.TopLeft,     a_B.TopLeft,     a_Time),
            Lerp(a_A.TopRight,    a_B.TopRight,    a_Time),
            Lerp(a_A.BottomLeft,  a_B.BottomLeft,  a_Time),
            Lerp(a_A.BottomRight, a_B.BottomRight, a_Time),
        };
    }

    template<typename Scalar, size N>
    RATUI_NODISCARD constexpr inline
    Vec<Scalar, N> Lerp( Vec<Scalar, N> a_A, Vec<Scalar, N> a_B, f32 a_Time )
    {
		Vec<Scalar, N> result;
		for ( size i = 0; i < N; ++i )
			result[i] = Lerp( a_A[i], a_B[i], a_Time );
		return result;
    }

    template<typename Scalar, size N>
    RATUI_NODISCARD constexpr inline 
    Mat<Scalar, N> Lerp( const Mat<Scalar, N>& a_A, const Mat<Scalar, N>& a_B, f32 a_Time )
    {
        Mat<Scalar, N> result;
        for ( size i = 0; i < N; ++i )
            for ( size j = 0; j < N; ++j )
                result[i][j] = Lerp( a_A[i][j], a_B[i][j], a_Time );
        return result;
    }

    template<>
    RATUI_NODISCARD constexpr inline 
    RenderTransform Lerp( const RenderTransform& a_A, const RenderTransform& a_B, f32 a_Time )
    {
        // TODO: Should probably use matrix lerp but might be too slow
        return RenderTransform{
            .Translation = Lerp(a_A.Translation, a_B.Translation, a_Time),
            .Scale       = Lerp(a_A.Scale,       a_B.Scale,       a_Time),
            .Pivot       = Lerp(a_A.Pivot,       a_B.Pivot,       a_Time),
            .SkewX       = Lerp(a_A.SkewX,       a_B.SkewX,       a_Time),
            .SkewY       = Lerp(a_A.SkewY,       a_B.SkewY,       a_Time),
            .Rotation    = Lerp(a_A.Rotation,    a_B.Rotation,    a_Time),
        };
    }
} // namespace RatUI::Animation