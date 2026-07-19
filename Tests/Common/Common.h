#pragma once

/**
 * @file Common.h
 * @brief Shared test utilities and helpers for RatUI tests.
 */

#include <RatUI/RatUI.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <format>

namespace RatUI
{
    // This is a stub implementation of GetRendererFromTexture for testing purposes.
    RATUI_NODISCARD inline IRenderer* GetRendererFromTexture( const TextureHandle& )
    {
		return nullptr;
    }
}

// Approximate float comparison tolerance used throughout the test suite.
inline constexpr float k_FloatEpsilon = 1e-5f;

inline float ToFloat( RatUI::Unit a_Value ) { return a_Value.ToFloat(); }
inline RatUI::Unit ToUnit( float a_Value ) { return RatUI::Unit{ a_Value }; }

inline RatUI::Vec2<RatUI::Unit> ToUnitVec2( const RatUI::Vec2f& a_Value )
{
    return { ToUnit( a_Value[ 0 ] ), ToUnit( a_Value[ 1 ] ) };
}

inline RatUI::Rect<RatUI::Unit> ToUnitRect( const RatUI::Rectf& a_Value )
{
    return { ToUnitVec2( a_Value.Origin ), ToUnitVec2( a_Value.Size ) };
}

inline RatUI::Vec2f ToFloatVec2( const RatUI::Vec2<RatUI::Unit>& a_Value )
{
    return { a_Value[ 0 ].ToFloat(), a_Value[ 1 ].ToFloat() };
}

inline RatUI::Rectf ToFloatRect( const RatUI::Rect<RatUI::Unit>& a_Value )
{
    return { ToFloatVec2( a_Value.Origin ), ToFloatVec2( a_Value.Size ) };
}

inline RatUI::Vec2f MeasureLayoutNode( RatUI::LayoutNode& a_Node, RatUI::Vec2f a_AvailableSize )
{
    return ToFloatVec2( RatUI::MeasureLayoutNode( a_Node, ToUnitVec2( a_AvailableSize ) ) );
}

inline void ArrangeLayoutNode( RatUI::LayoutNode& a_Node, RatUI::Rectf a_AllocatedRect )
{
    RatUI::ArrangeLayoutNode( a_Node, ToUnitRect( a_AllocatedRect ) );
}

inline RatUI::Rectf AlignRect( RatUI::Vec2f a_ContentSize, RatUI::Rectf a_Container, RatUI::EAlignment a_Align )
{
    RatUI::Vec2f offset{ 0.0f, 0.0f };

    if ( RatUI::HasFlag( a_Align, RatUI::EAlignment::HCenter ) ) offset[ 0 ] = ( a_Container.Size[ 0 ] - a_ContentSize[ 0 ] ) * 0.5f;
    else if ( RatUI::HasFlag( a_Align, RatUI::EAlignment::Right ) ) offset[ 0 ] = a_Container.Size[ 0 ] - a_ContentSize[ 0 ];

    if ( RatUI::HasFlag( a_Align, RatUI::EAlignment::VCenter ) ) offset[ 1 ] = ( a_Container.Size[ 1 ] - a_ContentSize[ 1 ] ) * 0.5f;
    else if ( RatUI::HasFlag( a_Align, RatUI::EAlignment::Bottom ) ) offset[ 1 ] = a_Container.Size[ 1 ] - a_ContentSize[ 1 ];

    return {
        .Origin = { a_Container.Origin[ 0 ] + offset[ 0 ], a_Container.Origin[ 1 ] + offset[ 1 ] },
        .Size = a_ContentSize
    };
}

inline float AlignCrossAxis( float a_ChildSize, float a_ParentPos, float a_ParentSize, RatUI::EAlignment a_Align, bool a_IsMainAxisHorizontal )
{
    const bool center = a_IsMainAxisHorizontal
        ? RatUI::HasFlag( a_Align, RatUI::EAlignment::VCenter )
        : RatUI::HasFlag( a_Align, RatUI::EAlignment::HCenter );

    const bool end = a_IsMainAxisHorizontal
        ? RatUI::HasFlag( a_Align, RatUI::EAlignment::Bottom )
        : RatUI::HasFlag( a_Align, RatUI::EAlignment::Right );

    if ( center ) return a_ParentPos + ( a_ParentSize - a_ChildSize ) * 0.5f;
    if ( end ) return a_ParentPos + a_ParentSize - a_ChildSize;
    return a_ParentPos;
}

/** @brief Checks that two Vec2f values are approximately equal component-wise. */
inline void RequireApproxEqual( const RatUI::Vec2f& a_Lhs, const RatUI::Vec2f& a_Rhs )
{
    REQUIRE( a_Lhs[ 0 ] == Catch::Approx( a_Rhs[ 0 ] ).epsilon( k_FloatEpsilon ) );
    REQUIRE( a_Lhs[ 1 ] == Catch::Approx( a_Rhs[ 1 ] ).epsilon( k_FloatEpsilon ) );
}

inline void RequireApproxEqual( const RatUI::Vec2<RatUI::Unit>& a_Lhs, const RatUI::Vec2f& a_Rhs )
{
    RequireApproxEqual( ToFloatVec2( a_Lhs ), a_Rhs );
}

inline void RequireApproxEqual( const RatUI::Vec2f& a_Lhs, const RatUI::Vec2<RatUI::Unit>& a_Rhs )
{
    RequireApproxEqual( a_Lhs, ToFloatVec2( a_Rhs ) );
}

/** @brief Checks that two Vec3f values are approximately equal component-wise. */
inline void RequireApproxEqual( const RatUI::Vec3f& a_Lhs, const RatUI::Vec3f& a_Rhs )
{
    REQUIRE( a_Lhs[ 0 ] == Catch::Approx( a_Rhs[ 0 ] ).epsilon( k_FloatEpsilon ) );
    REQUIRE( a_Lhs[ 1 ] == Catch::Approx( a_Rhs[ 1 ] ).epsilon( k_FloatEpsilon ) );
    REQUIRE( a_Lhs[ 2 ] == Catch::Approx( a_Rhs[ 2 ] ).epsilon( k_FloatEpsilon ) );
}

namespace RatUI
{
    inline bool operator==( Unit a_Lhs, float a_Rhs ) { return a_Lhs.ToFloat() == a_Rhs; }
    inline bool operator==( float a_Lhs, Unit a_Rhs ) { return a_Lhs == a_Rhs.ToFloat(); }
    inline bool operator==( Unit a_Lhs, const Catch::Approx& a_Rhs ) { return a_Rhs == a_Lhs.ToFloat(); }
    inline bool operator==( const Catch::Approx& a_Lhs, Unit a_Rhs ) { return a_Lhs == a_Rhs.ToFloat(); }
}

namespace std
{
    template<typename _Tag>
    struct formatter<RatUI::UnitBase<_Tag>, char> : formatter<float, char>
    {
        auto format( RatUI::UnitBase<_Tag> a_Value, format_context& a_Ctx ) const
        {
            return formatter<float, char>::format( a_Value.ToFloat(), a_Ctx );
        }
    };
}
