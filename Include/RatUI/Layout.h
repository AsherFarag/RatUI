#pragma once

/** 
 * @file Layout.h
 * @brief This file contains layout-related type definitions and utilities for RatUI.
 * The layout engine uses a two-pass approach where the first pass calculates the desired size of each element based on its content and constraints,
 * and the second pass determines the final position and size of each element based on the available space and alignment settings.
 */

#include "Core.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace RatUI
{
    /** 
     * @brief Enumerates the possible alignment options for UI elements.
     */
    enum EAlignment : u16
    {
        // Horizontal alignment flags

        AlignLeft   = 1 << 0,
        AlignHCenter = 1 << 1,
        AlignRight  = 1 << 2,

        // Vertical alignment flags

        AlignTop    = 1 << 3,
        AlignVCenter = 1 << 4,
        AlignBottom = 1 << 5,

        // Combined alignment presets

        AlignTopLeft     = AlignTop    | AlignLeft,
        AlignTopCenter   = AlignTop    | AlignHCenter,
        AlignTopRight    = AlignTop    | AlignRight,

        AlignCenterLeft  = AlignVCenter | AlignLeft,
        AlignCenter      = AlignVCenter | AlignHCenter,
        AlignCenterRight = AlignVCenter | AlignRight,

        AlignBottomLeft  = AlignBottom | AlignLeft,
        AlignBottomCenter= AlignBottom | AlignHCenter,
        AlignBottomRight = AlignBottom | AlignRight
    };

    /**
     * @brief Enumerates the possible layout directions for container elements.
     */
    enum class ELayoutDirection : u8
    {
        Horizontal, ///< Child elements are laid out from left to right.
        Vertical,   ///< Child elements are laid out from top to bottom.
        Stack,      ///< Child elements are stacked on top of each other, all occupying the same space.
        Grid        ///< Child elements are arranged in a grid pattern, filling rows first and then columns.
    };

    /**
     * @brief Represents the size constraints for a UI element, including minimum and maximum sizes.
     */
    struct Constraints
    {
        Vec2f MinSize{ 0.0f, 0.0f };
        Vec2f MaxSize{ std::numeric_limits<f32>::max(), std::numeric_limits<f32>::max() };

        static constexpr Constraints Unbounded() { return {}; }
        static constexpr Constraints Fixed( Vec2f a_Size ) { return { a_Size, a_Size }; }
        static constexpr Constraints AtLeast( Vec2f a_Min ) { return { a_Min, { std::numeric_limits<f32>::max(), std::numeric_limits<f32>::max() } }; }
        static constexpr Constraints AtMost ( Vec2f a_Max ) { return { { 0.0f, 0.0f }, a_Max }; }
    };

    /**
     * @brief Represents the input parameters for the layout process of a UI element,
     * including constraints, padding, margin, size hints, flex weight, alignment, and layout direction.
     */
    struct LayoutInput
    {
        Constraints      Constraints{ Constraints::Unbounded() }; ///< The minimum and maximum size constraints for the element.
        Vec4f            Padding{ 0.0f };           ///< The padding to apply inside the element's bounds, reducing the space available for content.
        Vec4f            Margin{ 0.0f };            ///< The margin to apply outside the element's bounds, increasing the space between this element and others.
        Vec2f            SizeHint{ 0.0f, 0.0f };    ///< Explicit desired size. Zero means derived from content.
        f32              FlexWeight{ 0.0f };        ///< For flex/box layouts, determines how much extra space this element should take relative to its siblings.
        EAlignment       Alignment{ AlignTopLeft }; ///< The alignment of the element's content within its bounds, using the EAlignment flags.
        ELayoutDirection LayoutDirection{ ELayoutDirection::Horizontal }; ///< The layout direction for child elements, if this element is a container.
    };
    
    /**
     * @brief Represents the output of the measure pass of the layout process, containing the desired size of the element.
     */
    struct MeasureOutput
    {
        Vec2f DesiredSize{ 0.0f, 0.0f }; ///< The size that the element desires based on its content and constraints.
    };

    /**
     * @brief Represents the output of the arrange pass of the layout process, containing the final position and size of the element.
     */
    struct ArrangeOutput
    {
        Rectf FinalRect{ { 0.0f, 0.0f }, { 0.0f, 0.0f } }; ///< The final position and size of the element after arrangement.
    };

    // Vec4f side ordering used by layout helpers: Left, Top, Right, Bottom.

    constexpr f32 Clamp( f32 a_Value, f32 a_Min, f32 a_Max )
    {
        return ( a_Value < a_Min ) ? a_Min : ( ( a_Value > a_Max ) ? a_Max : a_Value );
    }

    constexpr Vec2f Clamp( Vec2f a_Value, Vec2f a_Min, Vec2f a_Max )
    {
        return {
            Clamp( a_Value[ 0 ], a_Min[ 0 ], a_Max[ 0 ] ),
            Clamp( a_Value[ 1 ], a_Min[ 1 ], a_Max[ 1 ] )
        };
    }

    constexpr f32 HorizontalInsets( const Vec4f& a_Insets )
    {
        return a_Insets[ 0 ] + a_Insets[ 2 ];
    }

    constexpr f32 VerticalInsets( const Vec4f& a_Insets )
    {
        return a_Insets[ 1 ] + a_Insets[ 3 ];
    }

    constexpr Vec2f InsetSize( const Vec4f& a_Insets )
    {
        return { HorizontalInsets( a_Insets ), VerticalInsets( a_Insets ) };
    }

    constexpr Constraints IntersectConstraints( const Constraints& a_Left, const Constraints& a_Right )
    {
        const Vec2f MinSize{
            ( a_Left.MinSize[ 0 ] > a_Right.MinSize[ 0 ] ) ? a_Left.MinSize[ 0 ] : a_Right.MinSize[ 0 ],
            ( a_Left.MinSize[ 1 ] > a_Right.MinSize[ 1 ] ) ? a_Left.MinSize[ 1 ] : a_Right.MinSize[ 1 ]
        };

        const Vec2f MaxSize{
            ( a_Left.MaxSize[ 0 ] < a_Right.MaxSize[ 0 ] ) ? a_Left.MaxSize[ 0 ] : a_Right.MaxSize[ 0 ],
            ( a_Left.MaxSize[ 1 ] < a_Right.MaxSize[ 1 ] ) ? a_Left.MaxSize[ 1 ] : a_Right.MaxSize[ 1 ]
        };

        return { MinSize, MaxSize };
    }

    constexpr Constraints DeflateConstraints( const Constraints& a_Constraints, const Vec4f& a_Insets )
    {
        const Vec2f Inset = InsetSize( a_Insets );
        const Vec2f MinSize{
            ( a_Constraints.MinSize[ 0 ] > Inset[ 0 ] ) ? ( a_Constraints.MinSize[ 0 ] - Inset[ 0 ] ) : 0.0f,
            ( a_Constraints.MinSize[ 1 ] > Inset[ 1 ] ) ? ( a_Constraints.MinSize[ 1 ] - Inset[ 1 ] ) : 0.0f
        };
        const Vec2f MaxSize{
            ( a_Constraints.MaxSize[ 0 ] > Inset[ 0 ] ) ? ( a_Constraints.MaxSize[ 0 ] - Inset[ 0 ] ) : 0.0f,
            ( a_Constraints.MaxSize[ 1 ] > Inset[ 1 ] ) ? ( a_Constraints.MaxSize[ 1 ] - Inset[ 1 ] ) : 0.0f
        };
        return { MinSize, MaxSize };
    }

    constexpr Rectf DeflateRect( const Rectf& a_Rect, const Vec4f& a_Insets )
    {
        const f32 Left = a_Rect.Left() + a_Insets[ 0 ];
        const f32 Top = a_Rect.Top() + a_Insets[ 1 ];
        const f32 Right = a_Rect.Right() - a_Insets[ 2 ];
        const f32 Bottom = a_Rect.Bottom() - a_Insets[ 3 ];

        const f32 ValidRight = ( Right < Left ) ? Left : Right;
        const f32 ValidBottom = ( Bottom < Top ) ? Top : Bottom;

        return Rectf::FromMinMax( { Left, Top }, { ValidRight, ValidBottom } );
    }

    constexpr Rectf MakeRectMinSize( Vec2f a_Min, Vec2f a_Size )
    {
        return Rectf::FromMinMax( a_Min, a_Min + a_Size );
    }

    constexpr Rectf AlignRect( const Rectf& a_Bounds, Vec2f a_Size, EAlignment a_Alignment )
    {
        const Vec2f BoundsSize = a_Bounds.Size();
        const Vec2f ClampedSize{
            ( a_Size[ 0 ] > BoundsSize[ 0 ] ) ? BoundsSize[ 0 ] : a_Size[ 0 ],
            ( a_Size[ 1 ] > BoundsSize[ 1 ] ) ? BoundsSize[ 1 ] : a_Size[ 1 ]
        };

        f32 X = a_Bounds.Left();
        f32 Y = a_Bounds.Top();

        if ( a_Alignment & AlignHCenter )
        {
            X = a_Bounds.Left() + ( BoundsSize[ 0 ] - ClampedSize[ 0 ] ) * 0.5f;
        }
        else if ( a_Alignment & AlignRight )
        {
            X = a_Bounds.Right() - ClampedSize[ 0 ];
        }

        if ( a_Alignment & AlignVCenter )
        {
            Y = a_Bounds.Top() + ( BoundsSize[ 1 ] - ClampedSize[ 1 ] ) * 0.5f;
        }
        else if ( a_Alignment & AlignBottom )
        {
            Y = a_Bounds.Bottom() - ClampedSize[ 1 ];
        }

        return MakeRectMinSize( { X, Y }, ClampedSize );
    }

    constexpr Vec2f ApplySizeHint( Vec2f a_Size, const Vec2f& a_SizeHint )
    {
        if ( a_SizeHint[ 0 ] > 0.0f ) a_Size[ 0 ] = a_SizeHint[ 0 ];
        if ( a_SizeHint[ 1 ] > 0.0f ) a_Size[ 1 ] = a_SizeHint[ 1 ];
        return a_Size;
    }

    constexpr Vec2f ClampToConstraints( Vec2f a_Size, const Constraints& a_Constraints )
    {
        return Clamp( a_Size, a_Constraints.MinSize, a_Constraints.MaxSize );
    }

} // namespace RatUI