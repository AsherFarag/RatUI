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
     * @brief Represents the visibility state of a UI element, which can affect both rendering and layout.
     * @note Similar to Unreal Engine's EVisibility.
     */
    struct Visibility
    {
        enum EType : u8
        {
            Visible,  
            Hidden,   
            Collapsed,
        };

        EType Value{ Visible };

        constexpr bool IsVisible() const { return Value == Visible; }
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
        Constraints      SizeConstraints{ Constraints::Unbounded() }; ///< The minimum and maximum size constraints for the element.
        Vec4f            Padding{ 0.0f };           ///< The padding to apply inside the element's bounds, reducing the space available for content.
        Vec4f            Margin{ 0.0f };            ///< The margin to apply outside the element's bounds, increasing the space between this element and others.
        Vec2f            SizeHint{ 0.0f, 0.0f };    ///< Explicit desired size. Zero means derived from content.
        f32              FlexWeight{ 0.0f };        ///< For flex/box layouts, determines how much extra space this element should take relative to its siblings.
        EAlignment       Alignment{ AlignTopLeft }; ///< The alignment of the element's content within its bounds, using the EAlignment flags.
        ELayoutDirection LayoutDirection{ ELayoutDirection::Horizontal }; ///< The layout direction for child elements, if this element is a container.
    };

    struct Geometry
    {
        Vec2f LocalPosition{ 0.0f, 0.0f }; ///< Position of the element relative to its parent.
        Vec2f LocalSize{ 0.0f, 0.0f }; ///< Size of the element in its local coordinate space.
        Vec2f AbsoluteScale{ 1.0f, 1.0f }; ///< Cumulative scale from the root to this element.
        Vec2f AbsolutePosition{ 0.0f, 0.0f }; ///< Cumulative position from the root to this element.
    };

} // namespace RatUI