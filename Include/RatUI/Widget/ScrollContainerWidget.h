#pragma once
#include "Scene.h"
#include "IWidget.h"
#include "SliderWidget.h"

namespace RatUI
{
    /** @brief When a scrollbar is shown. */
    enum class EScrollbarMode : u8
    {
        Auto,  ///< Shown only while the content overflows on its axis.
        Never, ///< Never shown, and the wheel doesn't scroll that axis. SetScrollOffset still does.
        Always ///< Always shown.
    };

    /** @brief Whether the scrollbars take space from the content. */
    enum class EScrollbarPlacement : u8
    {
        Gutter, ///< Each bar has a strip of its own beside the content, kept whether or not the bar is showing, so content never shifts.
        Overlay ///< Bars draw over the edge of the content and take no space.
    };

    /** @brief What moved a scroll container. */
    enum class EScrollCause : u8
    {
        User,         ///< The wheel or a scrollbar drag.
        Programmatic, ///< SetScrollOffset, ScrollToEnd, or a widget brought into view (e.g. keyboard focus).
        Layout,       ///< Layout changed the content or the viewport: the offset was clamped, or only the range changed.
    };

    /** @brief A change to a scroll container's offset or to how far it can scroll. */
    struct ScrollEvent
    {
        Vec2<Unit>   Offset{};
        Vec2<Unit>   OffsetDelta{};
        Vec2<Unit>   MaxOffset{};      ///< How far the content extends past the viewport.
        Vec2<Unit>   MaxOffsetDelta{}; ///< Non-zero when the content or the viewport changed size.
        EScrollCause Cause{ EScrollCause::Programmatic };
    };

    /**
     * @brief A viewport onto content larger than itself, with scrollbars.
     *
     * Scrolling moves the content node's children (LayoutNode::ContentOffset) without running layout, so hit-testing,
     * navigation and painting use the children's SceneRects as they are. A descendant brought into view
     * (Scene::BringIntoView, e.g. by keyboard or gamepad focus) is scrolled to.
     *
     * Layout structure:
     *
     *   ScrollContainerWidget  [Overlay]
     *   +-- content node       [Flex width, Flex height]  <- children go here (GetContentNodeID)
     *   +-- VScrollbar         [right edge, Flex height]
     *   +-- HScrollbar         [bottom edge, Flex width]
     *
     * It scrolls vertically by default; SetHScrollbarMode turns on the horizontal axis.
     */
    class ScrollContainerWidget : public IWidget
    {
    public:
        static constexpr Unit c_ScrollbarSize = 16_u;
        static constexpr Unit c_WheelStep     = 40_u; ///< Distance scrolled per mouse wheel notch.

        /** @brief Called whenever the offset or the scrollable range changes, whatever the cause. Never during layout. */
        Callback<ScrollContainerWidget&, const ScrollEvent&> OnScroll;

        /**
         * @brief Keeps the view at the end of an axis while content is added, as long as it was at the end (e.g. a chat
         * or combat log). Scrolling away from the end releases it; scrolling back catches it again.
         */
        bool StickToEndX{ false };
        bool StickToEndY{ false };

        /** @brief The node the children are parented to (IWidget::GetContentNodeID). Scene::CreateWidget puts them there. */
        RATUI_NODISCARD LayoutNode* GetContentNode() { return GetScene().GetLayoutNode( m_ContentNodeID ); }
        RATUI_NODISCARD const LayoutNode* GetContentNode() const { return GetScene().GetLayoutNode( m_ContentNodeID ); }

        RATUI_NODISCARD EScrollbarMode GetVScrollbarMode() const { return m_VScrollbarMode; }
        RATUI_NODISCARD EScrollbarMode GetHScrollbarMode() const { return m_HScrollbarMode; }
        RATUI_NODISCARD EScrollbarPlacement GetScrollbarPlacement() const { return m_Placement; }

        void SetVScrollbarMode( EScrollbarMode a_Mode )
        {
            m_VScrollbarMode = a_Mode;
            UpdateBarLayout();
        }

        void SetHScrollbarMode( EScrollbarMode a_Mode )
        {
            m_HScrollbarMode = a_Mode;
            UpdateBarLayout();
        }

        void SetScrollbarPlacement( EScrollbarPlacement a_Placement )
        {
            m_Placement = a_Placement;
            UpdateBarLayout();
        }

        // =====================================================================
        // Scrolling
        // =====================================================================

        /** @brief The offset the content is scrolled by. */
        RATUI_NODISCARD Vec2<Unit> GetScrollOffset() const { return m_Offset; }

        /** @brief How far the content extends past the viewport, as of the last layout. */
        RATUI_NODISCARD Vec2<Unit> GetMaxScrollOffset() const
        {
            const LayoutNode* content = GetContentNode();
            return content ? MaxOffset( content->Layout.ContentSize, content->Layout.ParentRect.Size ) : Vec2<Unit>{ 0_u, 0_u };
        }

        /**
         * @brief Scrolls to @p a_Offset, clamped to the content's extent. Content added since the last layout counts
         * too: the offset asked for is kept until the next layout is done, then clamped again.
         */
        void SetScrollOffset( Vec2<Unit> a_Offset ) { ScrollTo( a_Offset, EScrollCause::Programmatic ); }

        /** @brief Scrolls to the end of an axis, including content added since the last layout. */
        void ScrollToEnd( EOrient a_Axis = EOrient::Vertical )
        {
            Vec2<Unit> offset = m_Offset;
            offset[a_Axis == EOrient::Horizontal ? 0 : 1] = Limits<Unit>::max();
            SetScrollOffset( offset );
        }

        // =====================================================================
        // IWidget overrides
        // =====================================================================

        void OnConstruct() override
        {
            Scene& scene = GetScene();
            GetLayout()
                .LayoutType( ELayoutType::Overlay )
                .WidthMode( ESizing::Flex )
                .HeightMode( ESizing::Flex );

            LayoutNode& content = scene.CreateLayoutNode( {}, GetLayoutID() );
            m_ContentNodeID        = content.ID;
            content.NotifyGeometry = true;
            content.ClipsContent   = true;
            content
                .WidthMode( ESizing::Flex )
                .HeightMode( ESizing::Flex )
                .LayoutType( ELayoutType::Vertical );

            // After the content, so the bars are drawn and hit above it.
            m_VScrollbarID = CreateScrollbar( EOrient::Vertical );
            m_HScrollbarID = CreateScrollbar( EOrient::Horizontal );
            UpdateBarLayout();

            // Last, so the container's own parts above aren't parented to the content.
            m_ContentID = m_ContentNodeID;
        }

        Reply OnPointerScroll( const PointerEvent& a_Event ) override
        {
            const Vec2<Unit> before = m_Offset;
            const f32 dx = m_HScrollbarMode == EScrollbarMode::Never ? 0.f : a_Event.ScrollDelta[0].ToFloat();
            const f32 dy = m_VScrollbarMode == EScrollbarMode::Never ? 0.f : a_Event.ScrollDelta[1].ToFloat();
            ScrollTo( { before[0] + c_WheelStep * dx, before[1] - c_WheelStep * dy }, EScrollCause::User );

            // Unmoved at either end, the scroll passes on to an enclosing scroll container.
            const bool moved = m_Offset[0] != before[0] || m_Offset[1] != before[1];
            return moved ? Reply::Handled() : Reply::Unhandled();
        }

        void OnGeometryChanged( const LayoutNode& a_Node, const GeometryEvent& a_Event ) override
        {
            if ( a_Node.ID != m_ContentNodeID )
                return;

            const Vec2<Unit> max         = MaxOffset( a_Event.ContentSize, a_Event.Size );
            const Vec2<Unit> previousMax = MaxOffset( a_Event.PreviousContentSize, a_Event.PreviousSize );

            // Only the layout an offset was asked ahead of clamps it again; any other leaves the view where it is.
            Vec2<Unit> target = GetScene().GetLayoutCount() == m_RequestLayout ? m_Request : m_Offset;
            if ( StickToEndX && m_Offset[0] >= previousMax[0] ) target[0] = max[0];
            if ( StickToEndY && m_Offset[1] >= previousMax[1] ) target[1] = max[1];

            Apply( target, max, previousMax, EScrollCause::Layout );
            SyncScrollbars( max ); // The thumb's size follows the viewport, also when the range is unchanged.
            UpdateBarVisibility( max );
        }

        void OnBringIntoView( const LayoutNode& a_Target ) override
        {
            const LayoutNode* content = GetContentNode();
            if ( !content )
                return;

            // The least movement that shows it; when it is larger than the viewport, its start.
            const Rect<Unit>& viewport = content->Layout.SceneRect;
            const Rect<Unit>& rect     = a_Target.Layout.SceneRect;
            Vec2<Unit>        offset   = m_Offset;
            for ( u32 axis = 0; axis < 2; ++axis )
            {
                const Unit before = rect.Origin[axis] - viewport.Origin[axis];
                const Unit after  = before + rect.Size[axis] - viewport.Size[axis];
                if ( before < 0_u )
                    offset[axis] = offset[axis] + before;
                else if ( after > 0_u )
                    offset[axis] = offset[axis] + std::min( after, before );
            }
            ScrollTo( offset, EScrollCause::Programmatic );
        }

        void OnPaint( const PaintEvent& a_Event ) override
        {
            Scene& scene = GetScene();

            if ( LayoutNode* content = GetContentNode() )
            {
                a_Event.Drawer.PushClipRect( content->Layout.SceneRect );
                PaintChildren( a_Event, *content, &content->Layout.SceneRect );
                a_Event.Drawer.PopClipRect();
            }

            if ( IWidget* vScrollbar = scene.GetWidget( m_VScrollbarID ) )
                vScrollbar->Paint( a_Event );

            if ( IWidget* hScrollbar = scene.GetWidget( m_HScrollbarID ) )
                hScrollbar->Paint( a_Event );
        }

    protected:
        EScrollbarMode      m_VScrollbarMode{ EScrollbarMode::Auto };
        EScrollbarMode      m_HScrollbarMode{ EScrollbarMode::Never }; // Most content scrolls one way: no strip is kept for a bar it never needs.
        EScrollbarPlacement m_Placement{ EScrollbarPlacement::Gutter };

        Vec2<Unit> m_Request{ 0_u, 0_u }; ///< The offset last asked for, which may be past the content's end.
        Vec2<Unit> m_Offset{ 0_u, 0_u };  ///< The offset applied.
        u64        m_RequestLayout{ 0 };  ///< The one layout (Scene::GetLayoutCount) that clamps m_Request again.
        bool       m_SyncingScrollbars{ false };

        NodeID m_ContentNodeID{ c_InvalidNodeID };
        NodeID m_VScrollbarID{ c_InvalidNodeID };
        NodeID m_HScrollbarID{ c_InvalidNodeID };

        static Vec2<Unit> MaxOffset( Vec2<Unit> a_ContentSize, Vec2<Unit> a_ViewportSize )
        {
            const Vec2<Unit> overflow = a_ContentSize - a_ViewportSize;
            return { std::max( 0_u, overflow[0] ), std::max( 0_u, overflow[1] ) };
        }

        void ScrollTo( Vec2<Unit> a_Offset, EScrollCause a_Cause )
        {
            const Scene& scene = GetScene();
            m_Request = { std::max( a_Offset[0], 0_u ), std::max( a_Offset[1], 0_u ) };

            // Content added since the last layout is laid out by the pending one, which clamps the offset again.
            m_RequestLayout = scene.GetLayoutCount() + ( scene.IsLayoutPending() ? 1 : 0 );

            const Vec2<Unit> max = GetMaxScrollOffset();
            Apply( m_Request, max, max, a_Cause );
        }

        /** @brief Moves the content and the bars to a_Target, clamped to a_Max, and reports what changed. */
        void Apply( Vec2<Unit> a_Target, Vec2<Unit> a_Max, Vec2<Unit> a_PreviousMax, EScrollCause a_Cause )
        {
            const Vec2<Unit>   offset{ std::clamp( a_Target[0], 0_u, a_Max[0] ), std::clamp( a_Target[1], 0_u, a_Max[1] ) };
            const ScrollEvent  event{ .Offset = offset, .OffsetDelta = offset - m_Offset, .MaxOffset = a_Max, .MaxOffsetDelta = a_Max - a_PreviousMax, .Cause = a_Cause };
            const Vec2<Unit>   none{ 0_u, 0_u };
            if ( event.OffsetDelta == none && event.MaxOffsetDelta == none )
                return;

            m_Offset = offset;
            GetScene().SetContentOffset( m_ContentNodeID, { -offset[0], -offset[1] } );
            SyncScrollbars( a_Max );

            if ( OnScroll )
                OnScroll( *this, event );
        }

        NodeID CreateScrollbar( EOrient a_Orientation )
        {
            SliderWidget* bar = GetScene().CreateWidget<SliderWidget>( GetLayoutID() );
            bar->Orientation = a_Orientation;
            bar->SetFocusPolicy( EFocusPolicy::None ); // Dragging a bar leaves focus where it is, and navigation moves through the content.
            bar->ShowTrackFill         = false;
            bar->ScaleThumbOnTrackAxis = true;

            if ( a_Orientation == EOrient::Vertical )
                bar->GetLayout().FixedWidth( c_ScrollbarSize ).HeightMode( ESizing::Flex ).SelfAlign( EAlign::TopRight );
            else
                bar->GetLayout().WidthMode( ESizing::Flex ).FixedHeight( c_ScrollbarSize ).SelfAlign( EAlign::BottomLeft );

            const u32 axis = a_Orientation == EOrient::Horizontal ? 0 : 1;
            bar->Value.Subscribe( [this, axis]( const f32& a_Value )
            {
                // Matching the bar to the offset isn't a drag.
                if ( m_SyncingScrollbars )
                    return;

                Vec2<Unit> offset = m_Offset;
                offset[axis]      = Unit( a_Value );
                ScrollTo( offset, EScrollCause::User );
            }, false );

            return bar->GetLayoutID();
        }

        /** @brief Applies the modes and the placement: which bars take part in layout, and the gutters the content leaves for them. */
        void UpdateBarLayout()
        {
            Scene&      scene   = GetScene();
            LayoutNode* content = GetContentNode();
            LayoutNode* vBar    = scene.GetLayoutNode( m_VScrollbarID );
            LayoutNode* hBar    = scene.GetLayoutNode( m_HScrollbarID );
            if ( !content || !vBar || !hBar )
                return;

            const bool vertical   = m_VScrollbarMode != EScrollbarMode::Never;
            const bool horizontal = m_HScrollbarMode != EScrollbarMode::Never;
            const bool gutter     = m_Placement == EScrollbarPlacement::Gutter;

            // The gutters depend on the settings alone, never on whether the content overflows, so layout can't feed back into them.
            content->Margin( Edges{ .R = vertical && gutter ? c_ScrollbarSize : 0_u, .B = horizontal && gutter ? c_ScrollbarSize : 0_u } );

            // The horizontal bar stops short of the vertical bar's strip.
            hBar->Margin( Edges{ .R = vertical ? c_ScrollbarSize : 0_u } );

            UpdateBarVisibility( GetMaxScrollOffset() );
        }

        void UpdateBarVisibility( Vec2<Unit> a_Max )
        {
            const auto apply = [this]( NodeID a_BarID, EScrollbarMode a_Mode, Unit a_Max )
            {
                LayoutNode* bar = GetScene().GetLayoutNode( a_BarID );
                if ( !bar )
                    return;

                // Auto only switches between Visible and Hidden, which take the same space: no layout.
                const EVisibility visibility = a_Mode == EScrollbarMode::Never ? EVisibility::Collapsed
                                             : a_Mode == EScrollbarMode::Always || a_Max > 0_u ? EVisibility::Visible
                                             : EVisibility::Hidden;
                if ( bar->Style.Visibility != visibility )
                    bar->Visibility( visibility );
            };

            apply( m_HScrollbarID, m_HScrollbarMode, a_Max[0] );
            apply( m_VScrollbarID, m_VScrollbarMode, a_Max[1] );
        }

        /** @brief Matches the scrollbars' range, thumb and value to the content and scroll offset. */
        void SyncScrollbars( Vec2<Unit> a_Max )
        {
            const LayoutNode* content = GetContentNode();
            if ( !content )
                return;

            m_SyncingScrollbars = true;
            const auto sync = [&]( NodeID a_ScrollbarID, u32 a_Axis )
            {
                // The container made the bars itself, so their type is known.
                SliderWidget* bar = static_cast<SliderWidget*>( GetScene().GetWidget( a_ScrollbarID ) );
                if ( !bar )
                    return;

                const f32 viewportSize = content->Layout.ParentRect.Size[a_Axis].ToFloat();
                const f32 contentSize  = viewportSize + a_Max[a_Axis].ToFloat();
                bar->Min = 0.f;
                bar->Max = a_Max[a_Axis].ToFloat();
                bar->ThumbScale = contentSize <= 0.f ? 1.f : std::clamp( viewportSize / contentSize, 0.f, 1.f );
                bar->Value.Set( m_Offset[a_Axis].ToFloat() );
            };

            sync( m_HScrollbarID, 0 );
            sync( m_VScrollbarID, 1 );
            m_SyncingScrollbars = false;
        }
    };

} // namespace RatUI
