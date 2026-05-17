#pragma once

/** 
 * @file Layout.h
 * @brief This file contains layout-related type definitions and utilities for RatUI.
 * The layout engine uses a two-pass approach where the first pass calculates the desired size of each element based on its content and constraints,
 * and the second pass determines the final position and size of each element based on the available space and alignment settings.
 */

#include "../Core.h"
#include <concepts>

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
        Fixed,   ///< Size is fixed to the specified dimensions.
		Percent, ///< Size is a percentage of the parent container's size.
        Flex,    ///< Size is determined by available space and the element's FlexGrow weight relative to siblings.
    };

    /**
     * @brief Wrap modes define how child elements are arranged when they exceed the available space in a container.
	 * @note Wrapping behavior is only applicable to certain layout types (e.g., Horizontal and Vertical) and is ignored for others (e.g., Overlay).
	 * ELayoutType::Horizontal with EWrapMode::Wrap will wrap child elements to the next line when they exceed the container's width,
     * while ELayoutType::Vertical with EWrapMode::Wrap will wrap child elements to the next column when they exceed the container's height.
     */
    enum class EWrapMode : u8
    {
        NoWrap, ///< Child elements are laid out in a single line and may overflow the container if they exceed its size.
        Wrap    ///< Child elements wrap to the next line when they exceed the container's size, similar to text wrapping.
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
        Vec2f      Min{ 0.0f, 0.0f };    ///< The minimum anchor point (top-left corner). (Values are normalized 0.0 to 1.0)
        Vec2f      Max{ 0.0f, 0.0f };    ///< The maximum anchor point (bottom-right corner). (Values are normalized 0.0 to 1.0)
        Vec2f      Pivot{ 0.0f, 0.0f };  ///< The pivot point for rotation and scaling, relative to the element's size. (Values are normalized 0.0 to 1.0)
        Vec2<Unit> Offset{ 0_u, 0_u };   ///< The offset from the anchored position, allowing for fine-tuning of the element's position. (Values are in pixels)

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

        /** @brief Returns true if the element should be hit-testable (i.e., it is Visible). */
        constexpr bool IsHitTestable() const { return Value == Visible; }
    };

    /**
     * @brief Represents the edge insets for a UI element, defining the spacing around its content or between it and other elements.
     * The Edges struct provides utility functions for calculating total horizontal and vertical insets, as well as applying the insets to a given rectangle.
     */
    struct Edges
    {
        Unit Top;
        Unit Right;
        Unit Bottom;
        Unit Left;

        /** @brief Calculates the total horizontal inset. */
        constexpr Unit Horizontal() const { return Left + Right; }

        /** @brief Calculates the total vertical inset. */
        constexpr Unit Vertical() const { return Top + Bottom; }

        /** @brief Calculates the total inset as a vector. */
        constexpr Vec2<Unit> Total() const { return { Horizontal(), Vertical() }; }

        /** @brief Applies the edge insets to a given rectangle, returning a new rectangle that is reduced by the specified insets. */
        constexpr Rect<Unit> Apply( const Rect<Unit>& a_Rect ) const
        {
			return Rect<Unit>::FromMinMax( a_Rect.Min() + Vec2<Unit>{ Left, Top },
                                           a_Rect.Max() - Vec2<Unit>{ Right, Bottom } );
        }

        /** @brief Combines two Edges by adding their respective values together. */
        constexpr Edges operator+( const Edges& a_Other ) const
        {
            return { Top + a_Other.Top, Right + a_Other.Right, Bottom + a_Other.Bottom, Left + a_Other.Left };
		}

        /** @brief Scales the edge insets by a scalar value, multiplying each edge by the specified factor. */
        constexpr Edges operator*( f32 a_Scalar ) const
        {
            return { Top * a_Scalar, Right * a_Scalar, Bottom * a_Scalar, Left * a_Scalar };
		}

        /** @brief Scales the edge insets by a scalar value, dividing each edge by the specified factor. */
        constexpr Edges operator/( f32 a_Scalar ) const
        {
            return { Top / a_Scalar, Right / a_Scalar, Bottom / a_Scalar, Left / a_Scalar };
        }

		/** @brief Initializes all edges to the same value. */
		constexpr Edges( Unit a_UniformValue = 0_u ) : Top( a_UniformValue ), Right( a_UniformValue ), Bottom( a_UniformValue ), Left( a_UniformValue ) {}

		/** @brief Initializes horizontal and vertical edges separately. */
		constexpr Edges( Unit a_Horizontal, Unit a_Vertical ) : Top( a_Vertical ), Right( a_Horizontal ), Bottom( a_Vertical ), Left( a_Horizontal ) {}

		/** @brief Initializes each edge individually. */
		constexpr Edges( Unit a_Top, Unit a_Right, Unit a_Bottom, Unit a_Left ) : Top( a_Top ), Right( a_Right ), Bottom( a_Bottom ), Left( a_Left ) {}

        /** @brief Initializes all edges to the same value. */
        static constexpr Edges Uniform( Unit a_Value ) { return { a_Value }; }

        /** @brief Initializes horizontal and vertical edges separately. */
        static constexpr Edges Symmetric( Unit a_Horizontal, Unit a_Vertical ) { return { a_Horizontal, a_Vertical }; }

        /** @brief Initializes each edge individually. */
        static constexpr Edges Asymmetric( Unit a_Top, Unit a_Right, Unit a_Bottom, Unit a_Left ) { return { a_Top, a_Right, a_Bottom, a_Left }; }
    };

    /**
     * @brief Represents the size constraints for a UI element, including minimum and maximum sizes.
     */
    struct Constraints
    {
        Vec2<Unit> MinSize{ 0_u, 0_u };
        Vec2<Unit> MaxSize{ Limits<Unit>::max(), Limits<Unit>::max() };

        /** @brief Creates unbounded constraints (i.e., no minimum or maximum size limits). */
        static constexpr Constraints Unbounded() { return {}; }

        /** @brief Creates fixed size constraints. */
        static constexpr Constraints Fixed( Vec2<Unit> a_Size ) { return { a_Size, a_Size }; }

        /** @brief Creates minimum size constraints with no maximum limit. */
        static constexpr Constraints AtLeast( Vec2<Unit> a_Min ) { return { a_Min, { Limits<Unit>::max(), Limits<Unit>::max() } }; }

        /** @brief Creates maximum size constraints with no minimum limit. */
        static constexpr Constraints AtMost ( Vec2<Unit> a_Max ) { return { { 0_u, 0_u }, a_Max }; }
    };

    /**
     * @brief Represents the input parameters for the layout process of a UI element.
     */
    struct LayoutStyle
    {
        // - Layout properties
        Unit        Spacing{ 0_u };                     ///< The spacing to apply between child elements in a container.
        ELayoutType LayoutType{ ELayoutType::Overlay }; ///< The layout type to use for arranging child elements (if this element is a container).
        EAlignment  ChildAlign{ EAlignment::TopLeft };  ///< Default alignment for child elements within this container.
        EWrapMode   WrapMode{ EWrapMode::NoWrap };      ///< The wrap mode to use when child elements exceed the available space in a container.
		bool        IsFocusScope{ false };              ///< Whether this element should be considered a focus scope for navigation.

        /**
         * @note For grid layouts, at least one of GridColumns or GridRows must be set to a non-zero value. 
         * The layout engine will auto-calculate the other dimension based on the number of children and the specified dimension.
         */
        u16         GridColumns{ 0 };                   ///< For grid layouts, the number of columns to arrange child elements into. 0 will auto-calculate.
        u16         GridRows{ 1 };                      ///< For grid layouts, the number of rows to arrange child elements into. 0 will auto-calculate.

        // - Positioning properties
        Edges Padding{};        ///< The padding to apply around the content of the element, in pixels.
        Edges Margin{};         ///< The margin to apply around the element itself, in pixels.
        struct Anchor Anchor{}; ///< The anchor points for the element, used when PositionMode is set to Anchored.
        EPositioningMode PositionMode{ EPositioningMode::Flow }; ///< The positioning mode for the element, determining how it is positioned relative to its parent container.

        // - Alignment properties
        EAlignment SelfAlign{ EAlignment::Inherit }; ///< Overrides parent's ChildAlign for this element. Only applicable when PositionMode is Flow.

        // - Sizing properties
        ESizingMode WidthMode{ ESizingMode::Content };  ///< The sizing mode for the width of the element.
        ESizingMode HeightMode{ ESizingMode::Content }; ///< The sizing mode for the height of the element.
        Unit FixedWidth{ 0_u };  ///< The fixed width to use when WidthMode is set to Fixed.
        Unit FixedHeight{ 0_u }; ///< The fixed height to use when HeightMode is set to Fixed.
        
        /**
         * @note PercentWidth/PercentHeight are only meaningful when the parent axis is Fixed or Flex.
         * Inside a Content-sized parent, Flex children fall back to Content (zero intrinsic size) and PercentWidth/Height is ignored.
         */
        f32 PercentWidth{ 0.0f };      ///< The percentage of the available width to use when WidthMode is set to Fill.
        f32 PercentHeight{ 0.0f };     ///< The percentage of the available height to use when HeightMode is set to Fill.

        f32 FlexGrow{ 0.0f };          ///< Determines how much of the remaining space the element should occupy relative to its siblings.
        Constraints SizeConstraints{}; ///< The size constraints to consider when laying out the element.
    };

    /**
     * @brief Represents the output of the layout process for a UI element, including its final position and size, desired size, and visibility state.
     * Calculated and cached during the Measure and Arrange steps of the layout process.
     */
    struct LayoutResult
    {
        Rect<Unit> FinalRect{};          ///< The final position and size of the element after layout, in absolute coordinates. Filled by the Arrange step.
        Vec2<Unit> DesiredSize{};        ///< The desired size of the element based on its content and constraints. Filled by the Measure step.

		// TODO: This works as a hot fix for text support but it doesnt work with text wrapping etc.
        // Figure out a clean solution without coupling the layout engine to the widgets.
		Vec2<Unit> IntrinsicSize{};      ///< The natural content size set by the user before layout. (e.g., the size of an image or text block).

        struct Visibility Visibility {}; ///< The visibility state of the element, which can affect both rendering and layout.
        bool IsDirty{ true };            ///< Whether the layout needs to be recalculated. Set to true when properties affecting layout are changed.
		bool IsDescendantDirty{ true };  ///< Whether any descendant elements are dirty and require layout recalculation.
    };

    using WidgetID = PoolID;
    using NodeID = PoolID;

    /**
     * @brief Represents a UI element in the RatUI layout system, containing layout styles, results, and hierarchical relationships with other nodes.
     * The hierarchy is represented as a doubly-linked list of children for efficient insertion and removal.
     * This avoids the overhead of dynamic arrays for child management, with minimal traversal costs as LayoutNodes are stored in pools.
	 * @warning LayoutNodes require either pointer stability from their storage (e.g., pool allocator) or careful management to ensure that pointers to child nodes remain valid.
     * @warning LayoutNodes do not own their children and are not responsible for their lifetime. 
     * It is the caller's responsibility to ensure that child nodes remain valid as long as they are part of the layout hierarchy.
     */
    struct LayoutNode
    {
        LayoutStyle Style{};   ///< The layout style properties that define how this widget should be sized and positioned.
        LayoutResult Layout{}; ///< The cached layout result for this widget computed during the layout process.
        u32 NumChildren{ 0 };

        union
        {
			PoolID WidgetID;          ///< The ID of the widget associated with this layout node.
			void* UserData = nullptr; ///< Incase LayoutNode is not being used with the IWidget system, this can store arbitrary user data.
        };

        LayoutNode* Parent()      const { return m_Parent; }
        LayoutNode* FirstChild()  const { return m_FirstChild; }
        LayoutNode* LastChild()   const { return m_LastChild; }
        LayoutNode* PrevSibling() const { return m_PrevSibling; }
        LayoutNode* NextSibling() const { return m_NextSibling; }

        /** @brief Detaches this node from its parent, updating the linked list pointers of siblings and parent accordingly. */
        void DetachFromParent();

        /** @brief Attaches the given child node to the end of this node's children. */
        void PushBackChild( LayoutNode& a_Child );

        /** @brief Attaches the given child node to the front of this node's children. */
        void PushFrontChild( LayoutNode& a_Child );

        /**
         * @brief Inserts the given child node after the specified sibling node in this node's children.
         * @param a_Child The child node to insert.
         * @param a_Sibling The sibling node after which the child should be inserted. This sibling must already be a child of this node.
         */
        void InsertChildAfter( LayoutNode& a_Child, LayoutNode& a_Sibling );

        /**
         * @brief Inserts the given child node before the specified sibling node in this node's children.
         * @param a_Child The child node to insert.
         * @param a_Sibling The sibling node before which the child should be inserted. This sibling must already be a child of this node.
         */
        void InsertChildBefore( LayoutNode& a_Child, LayoutNode& a_Sibling );

        /**
         * @brief Applies the given function to each child widget of this widget.
         * @tparam Func The type of the function to apply to each child widget. It must be invocable with a LayoutNode reference.
         * @param a_Func A callable that takes a LayoutNode reference. It will be invoked for each child widget of this widget.
         */
        template<std::invocable<LayoutNode&> Func>
        void ForEachChild( Func&& a_Func );

        /**
         * @brief Applies the given function to each child widget of this widget (const version).
         * @tparam Func The type of the function to apply to each child widget. It must be invocable with a const LayoutNode reference.
         * @param a_Func A callable that takes a const LayoutNode reference. It will be invoked for each child widget of this widget.
         */
        template<std::invocable<const LayoutNode&> Func>
        void ForEachChild( Func&& a_Func ) const;

        /**
         * @brief Applies the given function to this widget and all of its descendant widgets in a depth-first manner.
         * @tparam Func The type of the function to apply to each descendant widget. It must be invocable with a LayoutNode reference.
         * @param a_Func A callable that takes a LayoutNode reference.
         */
        template<std::invocable<LayoutNode&> Func>
        void ForEachDescendant( Func&& a_Func );

    private:
        LayoutNode* m_Parent{ nullptr };
        LayoutNode* m_FirstChild{ nullptr };
        LayoutNode* m_LastChild{ nullptr };
        LayoutNode* m_PrevSibling{ nullptr };
        LayoutNode* m_NextSibling{ nullptr };
    };

    // === Inline Implementations ===

    inline void LayoutNode::DetachFromParent()
    {
        if ( !m_Parent ) return; // Not attached to any parent

        // Link siblings together, bypassing this node
        if ( m_PrevSibling ) m_PrevSibling->m_NextSibling = m_NextSibling;
        else                 m_Parent->m_FirstChild = m_NextSibling;

        if ( m_NextSibling ) m_NextSibling->m_PrevSibling = m_PrevSibling;
        else                 m_Parent->m_LastChild = m_PrevSibling;

        m_Parent->NumChildren--;
        m_Parent = nullptr;
        m_PrevSibling = nullptr;
        m_NextSibling = nullptr;
    }

    inline void LayoutNode::PushBackChild( LayoutNode& a_Child )
    {
        a_Child.DetachFromParent(); // Ensure child is not currently attached to another parent

        a_Child.m_Parent = this;
        a_Child.m_NextSibling = nullptr;

        if ( m_LastChild )
        {
            m_LastChild->m_NextSibling = &a_Child;
            a_Child.m_PrevSibling = m_LastChild;
        }
        else
        {
            m_FirstChild = &a_Child;
            a_Child.m_PrevSibling = nullptr;
        }

        m_LastChild = &a_Child;
        ++NumChildren;
    }

    inline void LayoutNode::PushFrontChild( LayoutNode& a_Child )
    {
        a_Child.DetachFromParent(); // Ensure child is not currently attached to another parent

        a_Child.m_Parent = this;
        a_Child.m_PrevSibling = nullptr;

        if ( m_FirstChild )
        {
            m_FirstChild->m_PrevSibling = &a_Child;
            a_Child.m_NextSibling = m_FirstChild;
        }
        else
        {
            m_LastChild = &a_Child;
            a_Child.m_NextSibling = nullptr;
        }

        m_FirstChild = &a_Child;
        ++NumChildren;
    }

    inline void LayoutNode::InsertChildAfter( LayoutNode& a_Child, LayoutNode& a_Sibling )
    {
        if ( !a_Sibling.m_Parent || a_Sibling.m_Parent != this )
        {
            RATUI_USER_ASSERT( false, "Sibling node is not a child of this parent" );
            return;
        }

        a_Child.DetachFromParent(); // Ensure child is not currently attached to another parent

        a_Child.m_Parent = this;
        a_Child.m_PrevSibling = &a_Sibling;
        a_Child.m_NextSibling = a_Sibling.m_NextSibling;

        if ( a_Sibling.m_NextSibling )
            a_Sibling.m_NextSibling->m_PrevSibling = &a_Child;
        else
            m_LastChild = &a_Child;

        a_Sibling.m_NextSibling = &a_Child;
        ++NumChildren;
    }

    inline void LayoutNode::InsertChildBefore( LayoutNode& a_Child, LayoutNode& a_Sibling )
    {
        if ( !a_Sibling.m_Parent || a_Sibling.m_Parent != this )
        {
            RATUI_USER_ASSERT( false, "Sibling node is not a child of this parent" );
            return;
        }

        a_Child.DetachFromParent(); // Ensure child is not currently attached to another parent

        a_Child.m_Parent = this;
        a_Child.m_NextSibling = &a_Sibling;
        a_Child.m_PrevSibling = a_Sibling.m_PrevSibling;

        if ( a_Sibling.m_PrevSibling )
            a_Sibling.m_PrevSibling->m_NextSibling = &a_Child;
        else
            m_FirstChild = &a_Child;

        a_Sibling.m_PrevSibling = &a_Child;
        ++NumChildren;
    }

    template<std::invocable<LayoutNode&> Func>
    void LayoutNode::ForEachChild( Func&& a_Func )
    {
        for ( LayoutNode* child = m_FirstChild; child != nullptr; child = child->m_NextSibling )
            std::forward<Func>(a_Func)(*child);
    }

    template<std::invocable<const LayoutNode&> Func>
    void LayoutNode::ForEachChild( Func&& a_Func ) const
    {
        for ( const LayoutNode* child = m_FirstChild; child != nullptr; child = child->m_NextSibling )
            std::forward<Func>(a_Func)(*child);
    }

    template<std::invocable<LayoutNode&> Func>
    void LayoutNode::ForEachDescendant( Func&& a_Func )
    {
        ForEachChild( [&]( LayoutNode& child )
        {
            std::forward<Func>(a_Func)( child );
            child.ForEachDescendant( std::forward<Func>( a_Func ) );
        });
    }

} // namespace RatUI