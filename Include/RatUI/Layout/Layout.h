#pragma once
#include "../Core.h"
#include "NodeID.h"
#include <bitset>
#include <concepts>

namespace RatUI
{
    /**
	 * @brief Orientation of a thing, either horizontal or vertical.
     */
    enum class EOrient : u8
    {
        Horizontal,
        Vertical
    };

    /** 
     * @brief Alignment flags for positioning UI elements within their parent containers. 
     * These can be combined to specify both horizontal and vertical alignment.
     */
    enum class EAlign : u8
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
    RATUI_ENUM_ENABLE_BITMASK_OPERATORS( EAlign, u8 )

    /**
     * @brief The screen edges whose safe area (Scene::SetSafeArea) a node keeps its content clear of.
     * The safe area passes down the tree: a node opting into an edge adds what is left of it to its padding and
     * clears it for its descendants, so nested nodes don't inset twice. The inset doesn't depend on where the node
     * sits, so opt in on the containers that touch the screen's edges, such as a HUD root or a top or bottom bar.
     */
    enum class ESafeAreaEdges : u8
    {
        None   = 0,
        Top    = 1 << 0,
        Right  = 1 << 1,
        Bottom = 1 << 2,
        Left   = 1 << 3,

        Horizontal = Left | Right,
        Vertical   = Top | Bottom,
        All        = Horizontal | Vertical,
    };
    RATUI_ENUM_ENABLE_BITMASK_OPERATORS( ESafeAreaEdges, u8 )

    /** 
     * @brief Positioning modes define how a UI element is positioned relative to its parent container. 
     */
    enum class EPositioning : u8
    {
        /**
		 * @brief Positioned in the parent's stack/flex layout, 
         * based on the parent's layout type and the element's alignment.
         */
        Flow,
        /**
         * Positioned based on its Anchor relative to the parent's padded inner rect, outside of flow layout.
         * The element's Margin insets that rect before anchoring, Percent sizes are relative to it, and
         * SizeConstraints apply (including on stretched axes, where the clamped element is placed by its Pivot).
         * Flex sizing is treated as Content - use a stretch anchor (Min != Max) to fill instead.
         * SelfAlign and the parent's ChildAlign are ignored; use the Pivot.
         */
        Anchored,
    };

    /** 
     * @brief Sizing modes define how the size of a UI element is determined during layout. 
     */
    enum class ESizing : u8
    {
        Content, ///< Size is determined by the content of the element.
        Fixed,   ///< Size is fixed to the specified dimensions.
		Percent, ///< Size is a percentage of the parent container's size.
        Flex,    ///< Size is determined by available space and the element's FlexGrow weight relative to siblings.
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

        RATUI_NODISCARD static constexpr Anchor TopLeft()      { return { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } }; }
        RATUI_NODISCARD static constexpr Anchor TopCenter()    { return { { 0.5f, 0.0f }, { 0.5f, 0.0f }, { 0.5f, 0.0f } }; }
        RATUI_NODISCARD static constexpr Anchor TopRight()     { return { { 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f } }; }
        RATUI_NODISCARD static constexpr Anchor CenterLeft()   { return { { 0.0f, 0.5f }, { 0.0f, 0.5f }, { 0.0f, 0.5f } }; }
        RATUI_NODISCARD static constexpr Anchor Center()       { return { { 0.5f, 0.5f }, { 0.5f, 0.5f }, { 0.5f, 0.5f } }; }
        RATUI_NODISCARD static constexpr Anchor CenterRight()  { return { { 1.0f, 0.5f }, { 1.0f, 0.5f }, { 1.0f, 0.5f } }; }
        RATUI_NODISCARD static constexpr Anchor BottomLeft()   { return { { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f } }; }
        RATUI_NODISCARD static constexpr Anchor BottomCenter() { return { { 0.5f, 1.0f }, { 0.5f, 1.0f }, { 0.5f, 1.0f } }; }
        RATUI_NODISCARD static constexpr Anchor BottomRight()  { return { { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f } }; }

        RATUI_NODISCARD static constexpr Anchor StretchAll()   { return { { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.5f, 0.5f } }; }
        RATUI_NODISCARD static constexpr Anchor StretchTop()   { return { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 0.0f } }; }
        RATUI_NODISCARD static constexpr Anchor StretchBottom(){ return { { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.5f, 1.0f } }; }
        RATUI_NODISCARD static constexpr Anchor StretchLeft()  { return { { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.5f } }; }
        RATUI_NODISCARD static constexpr Anchor StretchRight() { return { { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.5f } }; }
    };

    /**
	 * @brief Whether a UI element is drawn and whether it takes space in layout.
     * Whether the pointer can hit it is separate (LayoutStyle::HitTestable), so showing an element never changes how it takes input.
     */
	enum class EVisibility : u8
    {
        Visible,   ///< Drawn, and takes its space in layout.
        Hidden,    ///< Not drawn or hit-tested, but still takes its space.
        Collapsed, ///< Not drawn or hit-tested, and takes no space.
    };

    namespace Visibility
    {
        /** @brief Whether the element takes space in layout (it isn't Collapsed). */
        constexpr bool AffectsLayout( EVisibility a_Visibility ) { return a_Visibility != EVisibility::Collapsed; }

        /** @brief Whether the element is drawn. */
        constexpr bool IsRendered( EVisibility a_Visibility ) { return a_Visibility == EVisibility::Visible; }

        /** @brief The visibility a child ends up with under its parent: the more hidden of the two. */
        RATUI_NODISCARD constexpr EVisibility Apply( EVisibility a_Parent, EVisibility a_Child )
        {
            return a_Parent > a_Child ? a_Parent : a_Child;
        }
    }

    /**
     * @brief Represents the edge insets for a UI element, defining the spacing around its content or between it and other elements.
     * The Edges struct provides utility functions for calculating total horizontal and vertical insets, as well as applying the insets to a given rectangle.
     */
    struct Edges
    {
        Unit T{}, R{}, B{}, L{};

        /** @brief Calculates the total horizontal inset. */
        RATUI_NODISCARD constexpr Unit Horizontal() const { return L + R; }

        /** @brief Calculates the total vertical inset. */
        RATUI_NODISCARD constexpr Unit Vertical() const { return T + B; }

        /** @brief Calculates the total inset as a vector. */
        RATUI_NODISCARD constexpr Vec2<Unit> Total() const { return { Horizontal(), Vertical() }; }

        /**
         * @brief Applies the edge insets to a given rectangle, returning a new rectangle that is reduced by the specified insets.
         * Insets larger than the rectangle leave it zero-sized at the inset min edge rather than inside out.
         */
        RATUI_NODISCARD constexpr Rect<Unit> Apply( const Rect<Unit>& a_Rect ) const
        {
            const Vec2<Unit> min = a_Rect.Min() + Vec2<Unit>{ L, T };
            const Vec2<Unit> max = a_Rect.Max() - Vec2<Unit>{ R, B };
			return Rect<Unit>::FromMinMax( min, Vec2<Unit>{ std::max( min[0], max[0] ), std::max( min[1], max[1] ) } );
        }

        /** @brief Initializes all edges to the same value. */
		RATUI_NODISCARD static constexpr Edges All( Unit a_Value ) { return { a_Value, a_Value, a_Value, a_Value }; }

        /** @brief Initializes horizontal and vertical edges separately. */
		RATUI_NODISCARD static constexpr Edges Axis( Unit a_Horizontal, Unit a_Vertical ) { return { a_Vertical, a_Horizontal, a_Vertical, a_Horizontal }; }

		RATUI_NODISCARD constexpr bool operator==( const Edges& ) const = default;

		RATUI_NODISCARD constexpr Edges operator+( const Edges& a_Other ) const { return { .T = T + a_Other.T, .R = R + a_Other.R, .B = B + a_Other.B, .L = L + a_Other.L }; }
		RATUI_NODISCARD constexpr Edges operator-( const Edges& a_Other ) const { return { .T = T - a_Other.T, .R = R - a_Other.R, .B = B - a_Other.B, .L = L - a_Other.L }; }

		RATUI_NODISCARD constexpr Edges operator+( Unit a_Value ) const { return { .T = T + a_Value, .R = R + a_Value, .B = B + a_Value, .L = L + a_Value }; }
		RATUI_NODISCARD constexpr Edges operator-( Unit a_Value ) const { return { .T = T - a_Value, .R = R - a_Value, .B = B - a_Value, .L = L - a_Value }; }
		RATUI_NODISCARD constexpr Edges operator*( f32 a_Scalar ) const { return { .T = T * a_Scalar, .R = R * a_Scalar, .B = B * a_Scalar, .L = L * a_Scalar }; }
		RATUI_NODISCARD constexpr Edges operator/( f32 a_Scalar ) const { return { .T = T / a_Scalar, .R = R / a_Scalar, .B = B / a_Scalar, .L = L / a_Scalar }; }
    };

    /**
     * @brief Represents the size constraints for a UI element, including minimum and maximum sizes.
     */
    struct Constraints
    {
        Vec2<Unit> Min{ 0_u, 0_u };
        Vec2<Unit> Max{ Limits<Unit>::max(), Limits<Unit>::max() };

        /** @brief Creates unbounded constraints (i.e., no minimum or maximum size limits). */
        RATUI_NODISCARD static constexpr Constraints Unbounded() { return {}; }

        /** @brief Creates fixed size constraints. */
        RATUI_NODISCARD static constexpr Constraints Fixed( Vec2<Unit> a_Size ) { return { a_Size, a_Size }; }

        /** @brief Creates minimum size constraints with no maximum limit. */
        RATUI_NODISCARD static constexpr Constraints AtLeast( Vec2<Unit> a_Min ) { return { .Min = a_Min, .Max = { Limits<Unit>::max(), Limits<Unit>::max() } }; }

        /** @brief Creates maximum size constraints with no minimum limit. */
        RATUI_NODISCARD static constexpr Constraints AtMost ( Vec2<Unit> a_Max ) { return { .Min = { 0_u, 0_u }, .Max = a_Max }; }
    };

    // TODO: THis shouldnt be in Layout.h but I dont know a better spot yet
    /**
     * @brief Represents the radius of each corner of a rectangle, allowing for asymmetric rounding.
     */
    struct CornerRadius
    {
        Unit TL{ 0_u }, TR{ 0_u };
        Unit BR{ 0_u }, BL{ 0_u };

        RATUI_NODISCARD static constexpr CornerRadius None() { return {}; }
		RATUI_NODISCARD static constexpr CornerRadius All( Unit a_Value ) { return { .TL = a_Value, .TR = a_Value, .BR = a_Value, .BL = a_Value }; }
		RATUI_NODISCARD static constexpr CornerRadius Vertical( Unit a_Top, Unit a_Bottom ) { return { .TL = a_Top, .TR = a_Top, .BR = a_Bottom, .BL = a_Bottom }; }
		RATUI_NODISCARD static constexpr CornerRadius Horizontal( Unit a_Left, Unit a_Right ) { return { .TL = a_Left, .TR = a_Right, .BR = a_Right, .BL = a_Left }; }

        RATUI_NODISCARD constexpr CornerRadius operator+( Unit a_Amount ) const { return { .TL = TL + a_Amount, .TR = TR + a_Amount, .BR = BR + a_Amount, .BL = BL + a_Amount }; }
		RATUI_NODISCARD constexpr CornerRadius operator-( Unit a_Amount ) const { return { .TL = TL - a_Amount, .TR = TR - a_Amount, .BR = BR - a_Amount, .BL = BL - a_Amount }; }
		RATUI_NODISCARD constexpr CornerRadius operator*( f32 a_Scalar )  const { return { .TL = TL * a_Scalar, .TR = TR * a_Scalar, .BR = BR * a_Scalar, .BL = BL * a_Scalar }; }
		RATUI_NODISCARD constexpr CornerRadius operator/( f32 a_Scalar )  const { return { .TL = TL / a_Scalar, .TR = TR / a_Scalar, .BR = BR / a_Scalar, .BL = BL / a_Scalar }; }

		CornerRadius& operator+=( Unit a_Amount )
		{
			TL += a_Amount; TR += a_Amount; BR += a_Amount; BL += a_Amount;
			return *this;
		}

		CornerRadius& operator-=( Unit a_Amount )
		{
			TL -= a_Amount; TR -= a_Amount; BR -= a_Amount; BL -= a_Amount;
			return *this;
		}

        CornerRadius& operator*=( f32 a_Scalar )
        {
			TL *= a_Scalar; TR *= a_Scalar; BR *= a_Scalar; BL *= a_Scalar;
			return *this;
        }

        CornerRadius& operator/=( f32 a_Scalar )
        {
            TL /= a_Scalar; TR /= a_Scalar; BR /= a_Scalar; BL /= a_Scalar;
            return *this;
        }
    };

    /**
     * @brief Represents the input parameters for the layout process of a UI element.
     * If any of these values change, the computed layout is invalid and must be recalculated.
     */
    struct LayoutStyle
    {
        // - Layout properties
        Unit        Spacing{ 0_u };                     ///< The spacing to apply between child elements in a container.
        ELayoutType LayoutType{ ELayoutType::Overlay }; ///< The layout type to use for arranging child elements (if this element is a container).
        EAlign      ChildAlign{ EAlign::TopLeft };      ///< Default alignment for child elements within this container.
		EVisibility Visibility{ EVisibility::Visible }; ///< Whether the element is drawn and whether it takes space.
		bool        HitTestable{ true };                ///< False: this node and everything below it ignore the pointer. Independent of Visibility.
		ESafeAreaEdges SafeAreaEdges{ ESafeAreaEdges::None }; ///< Edges whose remaining safe area is added to Padding for this node's content.

        /**
         * @note For grid layouts, at least one of GridColumns or GridRows must be set to a non-zero value. 
         * The layout engine will auto-calculate the other dimension based on the number of children and the specified dimension.
         * Defaults to 1 row, auto columns.
         */
        u16 GridColumns{ 0 }; ///< For grid layouts, the number of columns to arrange child elements into. 0 will auto-calculate.
        u16 GridRows{ 1 };    ///< For grid layouts, the number of rows to arrange child elements into. 0 will auto-calculate.

        // - Positioning properties
        Edges  Padding{};        ///< The padding to apply around the content of the element, in pixels.
        Edges  Margin{};         ///< The margin to apply around the element itself, in pixels.
        Anchor PositionAnchor{}; ///< The anchor points for the element, used when PositionMode is set to Anchored.
        EPositioning PositionMode{ EPositioning::Flow }; ///< The positioning mode for the element, 
                                                         ///< determining how it is positioned relative to its parent container.

        // - Alignment properties
        EAlign SelfAlign{ EAlign::Inherit }; ///< Overrides parent's ChildAlign for this element. Only applicable when PositionMode is Flow.

        // - Sizing properties
        ESizing WidthMode{ ESizing::Content };  ///< The sizing mode for the width of the element.
        ESizing HeightMode{ ESizing::Content }; ///< The sizing mode for the height of the element.
        Unit FixedWidth{ 0_u };  ///< The fixed width to use when WidthMode is set to Fixed.
        Unit FixedHeight{ 0_u }; ///< The fixed height to use when HeightMode is set to Fixed.

        // TODO: Not a fan of this api (flex and percent) and it can be confusing. Look into CSS more for ideas

        /**
         * @note PercentWidth/PercentHeight are only meaningful when the parent axis is Fixed or Flex.
         * Inside a Content-sized parent, Flex children fall back to Content (zero intrinsic size) and PercentWidth/Height is ignored.
         */
        f32 PercentWidth{ 0.0f };  ///< The percentage of the available width to use when WidthMode is set to Percent.
        f32 PercentHeight{ 0.0f }; ///< The percentage of the available height to use when HeightMode is set to Percent.

        // TODO: Should be flex width and flex height?
        f32 FlexGrow{ 0.0f };          ///< Determines how much of the remaining space the element should occupy relative to its siblings.
        Constraints SizeConstraints{}; ///< The size constraints to consider when laying out the element.
    };

    /**
     * @brief Cached values for linear layouts (ELayoutType::Vertical/Horizontal).
     * Generated by the Measure step in the layout engine, 
     * and used during the Arrange step to avoid recalculating them.
     */
	struct LinearAggregate
    {
        Unit TotalFixed{ 0_u };
        Unit FlexMarginSpace{ 0_u };
        f32  TotalGrow{ 0.f };
        u32  NumFlow{ 0 };
    };

    /**
     * @brief Represents the output of the layout process for a UI element, including its final position and size, desired size, and visibility state.
     * Calculated and cached during the Measure and Arrange steps of the layout process.
     */
    struct LayoutResult
    {
        Rect<Unit> ParentRect{};         ///< Position and size in the parent's space, before any ContentOffset. Filled by the Arrange step.
        Rect<Unit> SceneRect{};          ///< Where the element is drawn and hit, in the scene's space. Filled by ComposeLayout, after Arrange.
        Vec2<Unit> ContentSize{};        ///< How far the element's content extends from its top-left: its children, what overflows them (unless they clip, LayoutNode::ClipsContent), and its end padding. Filled by the Arrange step.
        Vec2<Unit> DesiredSize{};        ///< The desired size of the element based on its content and constraints. Filled by the Measure step.
		Vec2<Unit> LastAvailableSize{};  ///< The last available size passed to the Measure step. Used to determine if a re-measure is necessary.
		Edges      LastSafeArea{};       ///< The safe area that reached this node in the last Measure step. Used to determine if a re-measure is necessary.

		LinearAggregate CachedLinear{};  ///< Cached aggregate values for linear layouts, used to optimize layout calculations.

        EVisibility Visibility{ EVisibility::Visible };       ///< Style.Visibility combined with the ancestors'.
        bool        HitTestable{ true };        ///< Whether the pointer can hit this node: it and its ancestors are hit-testable and it is drawn.
        bool        IsDirty{ true };            ///< Whether the layout needs to be recalculated. Set to true when properties affecting layout are changed.
		bool        IsDescendantDirty{ true };  ///< Whether any descendant elements are dirty and require layout recalculation.
		bool        IsHitTestDirty{ false };    ///< On a root: where its nodes are, or which can be hit, changed (LayoutNode::MarkHitTestDirty). Whoever tracks what is under the pointer clears it.
    };

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
        const NodeID ID;              ///< The pool ID of this node, this is set automatically by the scene.
		LayoutStyle Style{};          ///< @warning Any changes to this struct should be followed by a call to MarkDirty() to ensure the layout is recalculated. 
                                      ///< Or use the factory-style setters below which automatically mark the node as dirty.
        LayoutResult Layout{};        ///< The cached layout result for this widget computed during the layout process.
        Unique<class IWidget> Widget;

        /**
         * @brief Moves this node's children by this much from where layout put them, e.g. to scroll or slide them.
         * It only changes where they are drawn and hit (their SceneRects), so layout isn't run. Set it with
         * SetContentOffset, which updates them at once.
         */
        Vec2<Unit> ContentOffset{ 0_u, 0_u };

        bool NotifyGeometry{ false };       ///< A change to this node's size or ContentSize is reported to the nearest widget (IWidget::OnGeometryChanged). Set by the widget that wants it.
        bool ClipsContent{ false };        ///< What overflows this node is cut off (by the widget painting it), so it doesn't count towards its parent's ContentSize.

        LayoutNode( NodeID a_ID = {}, LayoutStyle a_Style = {} ); // Defined in Layout.cpp, IWidget is incomplete here.
        ~LayoutNode();
        LayoutNode( const LayoutNode& ) = delete;
        LayoutNode( LayoutNode&& ) = delete;
        LayoutNode& operator=( const LayoutNode& ) = delete;
        LayoutNode& operator=( LayoutNode&& ) = delete;

        /** @brief Factory-style setter methods for layout properties - automatically marks the node as dirty when a property is changed. */

        LayoutNode& Spacing( Unit a_Spacing )              { Style.Spacing = a_Spacing; MarkDirty(); return *this; }
        LayoutNode& LayoutType( ELayoutType a_LayoutType ) { Style.LayoutType = a_LayoutType; MarkDirty(); return *this; }
        LayoutNode& ChildAlign( EAlign a_ChildAlign )      { Style.ChildAlign = a_ChildAlign; MarkDirty(); return *this; }
        /** @brief Only entering or leaving Collapsed runs layout again; Visible and Hidden take the same space. */
        LayoutNode& Visibility( EVisibility a_Visibility );
        /** @brief Sets whether the pointer can hit this node and everything below it. Never runs layout. */
        LayoutNode& HitTestable( bool a_HitTestable );
        LayoutNode& SafeAreaEdges( ESafeAreaEdges a_Edges ) { Style.SafeAreaEdges = a_Edges; MarkDirty(); return *this; }

        LayoutNode& GridColumns( u16 a_GridColumns )  { Style.GridColumns = a_GridColumns; MarkDirty(); return *this; }
        LayoutNode& GridRows( u16 a_GridRows )        { Style.GridRows = a_GridRows; MarkDirty(); return *this; }
        LayoutNode& Padding( Edges a_Padding )        { Style.Padding = a_Padding; MarkDirty(); return *this; }
        LayoutNode& Margin( Edges a_Margin )          { Style.Margin = a_Margin; MarkDirty(); return *this; }
        LayoutNode& Anchor( struct Anchor a_Anchor )  { Style.PositionAnchor = a_Anchor; MarkDirty(); return *this; }

        LayoutNode& PositionMode( EPositioning a_PositionMode ) { Style.PositionMode = a_PositionMode; MarkDirty(); return *this; }
        LayoutNode& SelfAlign( EAlign a_SelfAlign )             { Style.SelfAlign = a_SelfAlign; MarkDirty(); return *this; }
        LayoutNode& WidthMode( ESizing a_WidthMode )            { Style.WidthMode = a_WidthMode; MarkDirty(); return *this; }
        LayoutNode& HeightMode( ESizing a_HeightMode )          { Style.HeightMode = a_HeightMode; MarkDirty(); return *this; }

        LayoutNode& FixedWidth( Unit a_FixedWidth )      { Style.WidthMode = ESizing::Fixed; Style.FixedWidth = a_FixedWidth; MarkDirty(); return *this; }
        LayoutNode& FixedHeight( Unit a_FixedHeight )    { Style.HeightMode = ESizing::Fixed; Style.FixedHeight = a_FixedHeight; MarkDirty(); return *this; }
        LayoutNode& PercentWidth( f32 a_PercentWidth )   { Style.WidthMode = ESizing::Percent; Style.PercentWidth = a_PercentWidth; MarkDirty(); return *this; }
        LayoutNode& PercentHeight( f32 a_PercentHeight ) { Style.HeightMode = ESizing::Percent; Style.PercentHeight = a_PercentHeight; MarkDirty(); return *this; }
		LayoutNode& FlexWidth()                          { Style.WidthMode = ESizing::Flex; MarkDirty(); return *this; }
        LayoutNode& FlexHeight()                         { Style.HeightMode = ESizing::Flex; MarkDirty(); return *this; }
        LayoutNode& FlexGrow( f32 a_FlexGrow )           { Style.FlexGrow = a_FlexGrow; MarkDirty(); return *this; }

        LayoutNode& SizeConstraints( Constraints a_SizeConstraints ) { Style.SizeConstraints = a_SizeConstraints; MarkDirty(); return *this; }

        /** @brief Hierachy access */

        LayoutNode* Parent()      const { return m_Parent; }
        LayoutNode* FirstChild()  const { return m_FirstChild; }
        LayoutNode* LastChild()   const { return m_LastChild; }
        LayoutNode* PrevSibling() const { return m_PrevSibling; }
        LayoutNode* NextSibling() const { return m_NextSibling; }

        /** @brief Sets Layout.Visibility and Layout.HitTestable from this node's style and its parent's resolved values. */
        void ResolveVisibility();

        /** @brief Marks this node as dirty, indicating that its layout needs to be recalculated. Also marks all ancestor nodes as having a dirty descendant. */
        void MarkDirty();

        /** @brief Marks this node's root as hit-test dirty: nodes moved, or which of them the pointer can hit changed. */
        void MarkHitTestDirty();

        /** @brief Marks this node and all its descendants as dirty, indicating that their layout needs to be recalculated. */
        void MarkDescendantDirty();

        /** @brief Detaches this node from its parent, updating the linked list pointers of siblings and parent accordingly. */
        void DetachFromParent();

        /** @brief Returns the number of child nodes this node has. */
        u32 ChildCount() const { return m_ChildCount; }

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

        template<std::invocable<const LayoutNode&> Func>
        void ForEachChild( Func&& a_Func ) const;

        template<std::invocable<LayoutNode&> Func>
        void ForEachChildReverse( Func&& a_Func );

        template<std::invocable<const LayoutNode&> Func>
        void ForEachChildReverse( Func&& a_Func ) const;

        /**
         * @brief Applies the given function to this widget and all of its descendant widgets in a depth-first manner.
         * @tparam Func The type of the function to apply to each descendant widget. It must be invocable with a LayoutNode reference.
         * @param a_Func A callable that takes a LayoutNode reference.
         */
        template<std::invocable<LayoutNode&> Func>
        void ForEachDescendant( Func&& a_Func );

        template<std::invocable<const LayoutNode&> Func>
        void ForEachDescendant( Func&& a_Func ) const;

    protected:
        LayoutNode* m_Parent{ nullptr };
        LayoutNode* m_FirstChild{ nullptr };
        LayoutNode* m_LastChild{ nullptr };
        LayoutNode* m_PrevSibling{ nullptr };
        LayoutNode* m_NextSibling{ nullptr };
        u32         m_ChildCount{ 0 };
    };

    // === Inline Implementations ===

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
    void LayoutNode::ForEachChildReverse( Func&& a_Func )
    {
        for ( LayoutNode* child = m_LastChild; child != nullptr; child = child->m_PrevSibling )
            std::forward<Func>(a_Func)(*child);
    }

    template<std::invocable<const LayoutNode&> Func>
    void LayoutNode::ForEachChildReverse( Func&& a_Func ) const
    {
        for ( const LayoutNode* child = m_LastChild; child != nullptr; child = child->m_PrevSibling )
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

    template<std::invocable<const LayoutNode&> Func>
    void LayoutNode::ForEachDescendant( Func&& a_Func ) const
    {
        ForEachChild( [&]( const LayoutNode& child )
        {
            std::forward<Func>( a_Func )( child );
            child.ForEachDescendant( std::forward<Func>( a_Func ) );
        } );
    }

    /**
     * @brief A versioned, bucket-based pool allocator for LayoutNodes providing stable pointers and safe handle validation via NodeID.
     * Grows by allocating new buckets on the heap - existing bucket pointers and item references are never invalidated.
     */
    class NodePool
    {
    public:
        /** @brief Checks if the given NodeID is valid (i.e., refers to an allocated item in the pool). */
        RATUI_NODISCARD bool IsValid( NodeID a_ID ) const
        {
            const Bucket* bucket = BucketAt( Traits::Index( a_ID ) / c_ItemsPerBucket );
            return bucket && bucket->IsValid( a_ID );
        }

        /** @brief Retrieves a pointer to the item associated with the given NodeID, or nullptr if the ID is invalid. */
        RATUI_NODISCARD LayoutNode* Get( NodeID a_ID )
        {
            Bucket* bucket = BucketAt( Traits::Index( a_ID ) / c_ItemsPerBucket );
            if ( !bucket || !bucket->IsValid( a_ID ) ) return nullptr;
            return bucket->Get( Traits::Index( a_ID ) );
        }

        /** @brief Retrieves a const pointer to the item associated with the given NodeID, or nullptr if the ID is invalid. */
        RATUI_NODISCARD const LayoutNode* Get( NodeID a_ID ) const
        {
            const Bucket* bucket = BucketAt( Traits::Index( a_ID ) / c_ItemsPerBucket );
            if ( !bucket || !bucket->IsValid( a_ID ) ) return nullptr;
            return bucket->Get( Traits::Index( a_ID ) );
        }

        /** @brief Allocates a new item in the pool with the given arguments, returning a NodeID that can be used to access it. */
        template<typename... Args>
        RATUI_NODISCARD NodeID Allocate( Args&&... a_Args )
        {
            if ( Empty( FreeList ) )
                Grow();

            // Only pop once construction succeeded, so a throwing constructor doesn't leak the slot.
            u32 index = Back( FreeList );
            NodeID id = BucketForItem( index )->Allocate( index, std::forward<Args>( a_Args )... );
            PopBack( FreeList );
            return id;
        }

        /** @brief Deallocates the item associated with the given NodeID, making its slot available for future allocations. */
        void Deallocate( NodeID a_ID )
        {
            Bucket* bucket = BucketAt( Traits::Index( a_ID ) / c_ItemsPerBucket );
            if ( !bucket || !bucket->IsValid( a_ID ) ) return;
            bucket->Deallocate( Traits::Index( a_ID ) );
			PushBack( FreeList, static_cast<u32>( Traits::Index( a_ID ) ) );
        }

        /** @brief Destroys all items and frees every slot. Buckets and their versions are kept so previously issued NodeIDs stay stale. */
        void Clear()
        {
            ::RatUI::Clear( FreeList );
            for ( u32 b = static_cast<u32>( Size( Buckets ) ); b-- > 0; )
            {
                Bucket& bucket = *Buckets[ b ];
                for ( u32 i = c_ItemsPerBucket; i-- > 0; )
                {
                    if ( bucket.Occupancy.test( i ) )
                        bucket.Deallocate( b * c_ItemsPerBucket + i );

                    PushBack( FreeList, b * c_ItemsPerBucket + i ); // Reverse so lowest index is popped first
                }
            }
        }

        template<std::invocable<LayoutNode&> Func>
        void ForEach( Func&& a_Func ) { ForEachImpl( *this, a_Func ); }

        template<std::invocable<const LayoutNode&> Func>
        void ForEach( Func&& a_Func ) const { ForEachImpl( *this, a_Func ); }

    private:
        using Traits = NodeIDTraits<NodeID>;

        static constexpr u32 c_ItemsPerBucket = 4096;

        static constexpr u32 NextVersion( u32 a_Version )
        {
            return ( a_Version + 1 ) & Traits::VersionMask;
        }

        /**
         * @brief A bucket containing a fixed number of items, along with versioning and occupancy tracking for safe allocation and deallocation.
         */
        struct Bucket
        {
            alignas(LayoutNode) FixedArray<byte, c_ItemsPerBucket * sizeof(LayoutNode)> RawData{};
            FixedArray<u32, c_ItemsPerBucket> Versions{};
            std::bitset<   c_ItemsPerBucket> Occupancy{};

            RATUI_NODISCARD LayoutNode* GetItems() { return reinterpret_cast<LayoutNode*>( Data( RawData ) ); }
            RATUI_NODISCARD const LayoutNode* GetItems() const { return reinterpret_cast<const LayoutNode*>( Data( RawData ) ); }

            RATUI_NODISCARD bool IsOccupied( u32 a_LocalIndex ) const
            {
                return Occupancy.test( a_LocalIndex );
            }

            RATUI_NODISCARD bool IsValid( NodeID a_ID ) const
            {
                u32 local = Traits::Index( a_ID ) % c_ItemsPerBucket;
                return IsOccupied( local ) && Versions[ local ] == Traits::Version( a_ID );
            }

            template<typename... Args>
            RATUI_NODISCARD NodeID Allocate( u32 a_GlobalIndex, Args&&... a_Args )
            {
                u32 local = a_GlobalIndex % c_ItemsPerBucket;
                const NodeID id = Traits::Construct( a_GlobalIndex, NextVersion( Versions[ local ] ) );
                std::construct_at( GetItems() + local, id, std::forward<Args>( a_Args )... );
                Versions[ local ] = Traits::Version( id );
                Occupancy.set( local );
                return id;
            }

            void Deallocate( u32 a_GlobalIndex )
            {
                u32 local = a_GlobalIndex % c_ItemsPerBucket;
                std::destroy_at( GetItems() + local );
                Versions[ local ] = NextVersion( Versions[ local ] ); // invalidate existing NodeIDs
                Occupancy.reset( local );
            }


            RATUI_NODISCARD LayoutNode*       Get( u32 a_GlobalIndex )       { return GetItems() + ( a_GlobalIndex % c_ItemsPerBucket ); }
            RATUI_NODISCARD const LayoutNode* Get( u32 a_GlobalIndex ) const { return GetItems() + ( a_GlobalIndex % c_ItemsPerBucket ); }

            Bucket() = default;
            ~Bucket()
            {
                // Destroy any occupied items in the bucket
                for ( u32 i = 0; i < c_ItemsPerBucket; ++i )
                {
                    if ( Occupancy.test( i ) )
                        std::destroy_at( GetItems() + i );
                }
            }
        };

        Array<Unique<Bucket>> Buckets{};
        Array<u32>            FreeList{};

        template<typename Self, typename Func>
        static void ForEachImpl( Self& a_Self, Func& a_Func )
        {
            for ( const auto& bucket : a_Self.Buckets )
            {
                if ( bucket == nullptr )
                    continue;

                for ( u32 i = 0; i < c_ItemsPerBucket; ++i )
                {
                    if ( bucket->Occupancy.test( i ) )
                        a_Func( bucket->GetItems()[ i ] );
                }
            }
        }

        Bucket* BucketAt( u32 a_BucketIndex )
        {
            if ( a_BucketIndex >= Size( Buckets ) ) return nullptr;
            return Buckets[ a_BucketIndex ].get();
        }

        const Bucket* BucketAt( u32 a_BucketIndex ) const
        {
            if ( a_BucketIndex >= Size( Buckets ) ) return nullptr;
            return Buckets[ a_BucketIndex ].get();
        }

        Bucket*       BucketForItem( u32 a_ItemIndex )       { return BucketAt( a_ItemIndex / c_ItemsPerBucket ); }
        const Bucket* BucketForItem( u32 a_ItemIndex ) const { return BucketAt( a_ItemIndex / c_ItemsPerBucket ); }

        void Grow()
        {
            u32 newBucketIndex = static_cast<u32>( Size( Buckets ) );
            EmplaceBack( Buckets, MakeUnique<Bucket>() );

            // Push in reverse so lowest index is popped first
            Reserve( FreeList, Size( FreeList ) + c_ItemsPerBucket );
            for ( u32 i = c_ItemsPerBucket; i-- > 0; )
                PushBack( FreeList, newBucketIndex * c_ItemsPerBucket + i );
        }
    };

} // namespace RatUI
