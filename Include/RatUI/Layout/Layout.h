#pragma once

/** 
 * @file Layout.h
 * @brief This file contains layout-related type definitions and utilities for RatUI.
 * The layout engine uses a two-pass approach where the first pass calculates the desired size of each element based on its content and constraints,
 * and the second pass determines the final position and size of each element based on the available space and alignment settings.
 */

#include "../Core.h"

namespace RatUI
{
    /** 
     * @brief Alignment flags for positioning UI elements within their parent containers. 
     * These can be combined to specify both horizontal and vertical alignment.
     */
    enum EAlignment : u8
    {
        Inherit = 0, ///< Inherit alignment from parent container. Only applicable when PositionMode is Flow.

        // - Horizontal

        Left    = 1 << 0,
        HCenter = 1 << 1,
        Right   = 1 << 2,

        // - Vertical

        Top     = 1 << 3,
        VCenter = 1 << 4,
        Bottom  = 1 << 5,

        // - Presets

        TopLeft     = Top | Left,
        TopCenter   = Top | HCenter,
        TopRight    = Top | Right,

        CenterLeft  = VCenter | Left,
        Center      = VCenter | HCenter,
        CenterRight = VCenter | Right,

        BottomLeft  = Bottom | Left,
        BottomCenter= Bottom | HCenter,
        BottomRight = Bottom | Right,

        HStretch    = Left | Right,
        VStretch    = Top  | Bottom,
        StretchFill = HStretch | VStretch, 
    };

    /** 
     * @brief Positioning modes define how a UI element is positioned relative to its parent container. 
     */
    enum class EPositioningMode : u8
    {
        Flow,     ///< Participates in parent's stack/flex layout.
        Anchored, ///< Positioned based on its Anchor relative to the parent.
    };

    /** 
     * @brief Sizing modes define how the size of a UI element is determined during layout. 
     */
    enum class ESizingMode : u8
    {
        Content, ///< Size is determined by the content of the element.
        Fill,    ///< Size fills the available space in the parent container.
        Fixed    ///< Size is explicitly specified and does not change based on content or available space.
    };

    /**
     * @brief Wrap modes define how child elements are arranged when they exceed the available space in a container.
     */
    enum class EWrapMode : u8
    {
        NoWrap, ///< Child elements are laid out in a single line and may overflow the container if they exceed its size.
        Wrap    ///< Child elements wrap to the next line when they exceed the container's size, similar to text wrapping.
    };

    /** 
     * @brief Enumerates the possible axes for layout operations.
     */
    enum class EAxis : u8
    {
        Horizontal,
        Vertical
    };

    /**
     * @brief Defines how child elements are arranged and sized within a container.
     */
    enum class ELayoutType : u8
    {
        Horizontal, ///< Child elements are laid out from left to right.
        Vertical,   ///< Child elements are laid out from top to bottom.
        Overlay,    ///< Child elements are stacked on top of each other, all occupying the same space.
        Grid        ///< Child elements are arranged in a grid pattern, filling rows first and then columns.
    };

    /**
     * @brief Represents the anchor points for a UI element, defining how it is positioned and scaled relative to its parent container.
     * The Min and Max anchors define the normalized position of the element's corners relative to the parent (0.0 to 1.0),
     * while the Pivot defines the point around which the element rotates and scales.
     */
    struct Anchor
    {
        Vec2f Min{ 0.0f, 0.0f };    ///< The minimum anchor point (top-left corner). (Values are normalized 0.0 to 1.0)
        Vec2f Max{ 0.0f, 0.0f };    ///< The maximum anchor point (bottom-right corner). (Values are normalized 0.0 to 1.0)
        Vec2f Pivot{ 0.0f, 0.0f };  ///< The pivot point for rotation and scaling, relative to the element's size. (Values are normalized 0.0 to 1.0)
        Vec2f Offset{ 0.0f, 0.0f }; ///< The pixel offset from the anchored position, allowing for fine-tuning of the element's position. (Values are in pixels)

        static constexpr Anchor TopLeft()      { return { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } }; }
        static constexpr Anchor TopCenter()    { return { { 0.5f, 0.0f }, { 0.5f, 0.0f }, { 0.5f, 0.0f } }; }
        static constexpr Anchor TopRight()     { return { { 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f } }; }
        static constexpr Anchor CenterLeft()   { return { { 0.0f, 0.5f }, { 0.0f, 0.5f }, { 0.0f, 0.5f } }; }
        static constexpr Anchor Center()       { return { { 0.5f, 0.5f }, { 0.5f, 0.5f }, { 0.5f, 0.5f } }; }
        static constexpr Anchor CenterRight()  { return { { 1.0f, 0.5f }, { 1.0f, 0.5f }, { 1.0f, 0.5f } }; }
        static constexpr Anchor BottomLeft()   { return { { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f } }; }
        static constexpr Anchor BottomCenter() { return { { 0.5f, 1.0f }, { 0.5f, 1.0f }, { 0.5f, 1.0f } }; }
        static constexpr Anchor BottomRight()  { return { { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f } }; }

        static constexpr Anchor StretchAll()   { return { { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.5f, 0.5f } }; }
        static constexpr Anchor StretchTop()   { return { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 0.0f } }; }
        static constexpr Anchor StretchBottom(){ return { { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.5f, 1.0f } }; }
        static constexpr Anchor StretchLeft()  { return { { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.5f } }; }
        static constexpr Anchor StretchRight() { return { { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.5f } }; }
    };

    /**
     * @brief Represents the visibility state of a UI element, which can affect both rendering and layout.
     * @note Similar to Unreal Engine's EVisibility.
     */
    struct Visibility
    {
        enum EType : u8
        {
            Visible,   ///< The element is visible and participates in layout.
            Hidden,    ///< The element is not visible but still occupies space in the layout.
            Collapsed, ///< The element is not visible and does not occupy any space in the layout.
        };

        EType Value{ Visible };

        /** @brief Returns true if the element should be considered in layout calculations (i.e., it is not Collapsed). */
        constexpr bool AffectsLayout() const { return Value != Collapsed; }

        /** @brief Returns true if the element should be rendered (i.e., it is Visible). */
        constexpr bool IsRendered() const { return Value == Visible; }
    };

    /**
     * @brief Represents the edge insets for a UI element, defining the spacing around its content or between it and other elements.
     * The Edges struct provides utility functions for calculating total horizontal and vertical insets, as well as applying the insets to a given rectangle.
     */
    struct Edges
    {
        f32 Top{ 0.0f };
        f32 Right{ 0.0f };
        f32 Bottom{ 0.0f };
        f32 Left{ 0.0f };

        /** @brief Calculates the total horizontal inset. */
        constexpr f32 Horizontal() const { return Left + Right; }

        /** @brief Calculates the total vertical inset. */
        constexpr f32 Vertical() const { return Top + Bottom; }

        /** @brief Calculates the total inset as a vector. */
        constexpr Vec2f Total() const { return { Horizontal(), Vertical() }; }

        /** @brief Applies the edge insets to a given rectangle, returning a new rectangle that is reduced by the specified insets. */
        constexpr Rectf Apply( Rectf a_Rect ) const
        {
			return a_Rect.FromMinMax( a_Rect.Min() + Vec2f{ Left, Top }, a_Rect.Max() - Vec2f{ Right, Bottom } );
        }

        /** @brief Initializes all edges to the same value. */
        static constexpr Edges Uniform( f32 a_Value ) { return { a_Value, a_Value, a_Value, a_Value }; }

        /** @brief Initializes horizontal and vertical edges separately. */
        static constexpr Edges Symmetric( f32 a_Horizontal, f32 a_Vertical ) { return { a_Vertical, a_Horizontal, a_Vertical, a_Horizontal }; }

        /** @brief Initializes each edge individually. */
        static constexpr Edges Asymmetric( f32 a_Top, f32 a_Right, f32 a_Bottom, f32 a_Left ) { return { a_Top, a_Right, a_Bottom, a_Left }; }
    };

    /**
     * @brief Represents the size constraints for a UI element, including minimum and maximum sizes.
     */
    struct Constraints
    {
        Vec2f MinSize{ 0.0f, 0.0f };
        Vec2f MaxSize{ Limits<f32>::max(), Limits<f32>::max() };

        /** @brief Creates unbounded constraints (i.e., no minimum or maximum size limits). */
        static constexpr Constraints Unbounded() { return {}; }

        /** @brief Creates fixed size constraints. */
        static constexpr Constraints Fixed( Vec2f a_Size ) { return { a_Size, a_Size }; }

        /** @brief Creates minimum size constraints with no maximum limit. */
        static constexpr Constraints AtLeast( Vec2f a_Min ) { return { a_Min, { Limits<f32>::max(), Limits<f32>::max() } }; }

        /** @brief Creates maximum size constraints with no minimum limit. */
        static constexpr Constraints AtMost ( Vec2f a_Max ) { return { { 0.0f, 0.0f }, a_Max }; }
    };

    /**
     * @brief Represents the input parameters for the layout process of a UI element.
     */
    struct LayoutStyle
    {
        // Layout properties
        f32         Spacing{ 0.0f };                    ///< The spacing to apply between child elements in a container, in pixels.
        ELayoutType LayoutType{ ELayoutType::Overlay }; ///< The layout type to use for arranging child elements (if this element is a container).
        EAlignment  ChildAlign{ EAlignment::TopLeft };  ///< Default alignment for child elements within this container.
        EAxis       Axis{ EAxis::Horizontal };          ///< The primary axis for layout operations, used by certain layout types to determine the direction of child arrangement.
        EWrapMode   WrapMode{ EWrapMode::NoWrap };      ///< The wrap mode to use when child elements exceed the available space in a container.

        // Positioning properties
        Edges Padding; ///< The padding to apply around the content of the element, in pixels.
        Edges Margin;  ///< The margin to apply around the element itself, in pixels.
        Anchor Anchor; ///< The anchor points for the element, used when PositionMode is set to Anchored.
        EPositioningMode PositionMode{ EPositioningMode::Flow }; ///< The positioning mode for the element, determining how it is positioned relative to its parent container.

        // Alignment properties
        EAlignment SelfAlign{ EAlignment::Inherit }; ///< Overrides parent's ChildAlign for this element. Only applicable when PositionMode is Flow.

        // Sizing properties
        ESizingMode WidthMode{ ESizingMode::Content };  ///< The sizing mode for the width of the element.
        ESizingMode HeightMode{ ESizingMode::Content }; ///< The sizing mode for the height of the element.
        f32 FixedWidth{ 0.0f };       ///< The fixed width to use when WidthMode is set to Fixed.
        f32 FixedHeight{ 0.0f };      ///< The fixed height to use when HeightMode is set to Fixed.
        f32 PercentWidth{ 0.0f };     ///< The percentage of the available width to use when WidthMode is set to Fill.
        f32 PercentHeight{ 0.0f };    ///< The percentage of the available height to use when HeightMode is set to Fill.
        f32 FlexGrow{ 0.0f };         ///< Determines how much of the remaining space the element should occupy relative to its siblings.
        Constraints  SizeConstraints; ///< The size constraints to consider when laying out the element.
    };

    /**
     * @brief Represents the output of the layout process for a UI element, including its final position and size, desired size, and visibility state.
     * Calculated and cached during the Measure and Arrange steps of the layout process. 
     */
    struct LayoutResult
    {
        Rectf FinalRect{};    ///< The final position and size of the element after layout, in absolute coordinates. Filled by the Arrange step.
        Vec2f DesiredSize{};  ///< The desired size of the element based on its content and constraints. Filled by the Measure step.
        bool IsDirty{ true }; ///< Whether the layout needs to be recalculated. Set to true when properties affecting layout are changed.
        Visibility Visibility{}; ///< The visibility state of the element, which can affect both rendering and layout.
    };

} // namespace RatUI