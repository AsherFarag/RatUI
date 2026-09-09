#pragma once
#include "../Core.h"
#include "../Layout/Layout.h"
#include "../Renderer/RenderTransform.h"

namespace RatUI
{
    template<typename T> requires
        requires( T a, T b, f32 t ) { { a + ( b - a ) * t } -> std::convertible_to<T>; }
	RATUI_NODISCARD constexpr inline
	T Lerp( const T& a_A, const T& a_B, f32 a_Time )
	{
		return a_A + ( a_B - a_A ) * a_Time;
	}

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

    RATUI_NODISCARD constexpr inline 
    CornerRadius Lerp(const CornerRadius& a_A, const CornerRadius& a_B, f32 a_Time)
    {
        return {
            Lerp(a_A.TL,     a_B.TL,     a_Time),
            Lerp(a_A.TR,    a_B.TR,    a_Time),
            Lerp(a_A.BL,  a_B.BL,  a_Time),
            Lerp(a_A.BR, a_B.BR, a_Time),
        };
    }

    template<typename Scalar, unsigned N>
    RATUI_NODISCARD constexpr inline
    Vec<Scalar, N> Lerp( Vec<Scalar, N> a_A, Vec<Scalar, N> a_B, f32 a_Time )
    {
		Vec<Scalar, N> result;
		for ( unsigned i = 0; i < N; ++i )
			result[i] = Lerp( a_A[i], a_B[i], a_Time );
		return result;
    }

    template<typename Scalar, unsigned N>
    RATUI_NODISCARD constexpr inline 
    Mat<Scalar, N> Lerp( const Mat<Scalar, N>& a_A, const Mat<Scalar, N>& a_B, f32 a_Time )
    {
        Mat<Scalar, N> result;
        for ( unsigned i = 0; i < N; ++i )
            for ( unsigned j = 0; j < N; ++j )
                result[i][j] = Lerp( a_A[i][j], a_B[i][j], a_Time );
        return result;
    }

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