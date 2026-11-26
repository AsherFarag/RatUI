#pragma once
#include "Scene.h"
#include "IWidget.h"
#include "SliderWidget.h"

namespace RatUI 
{
    /**
	 * @brief Enumeration for scrollbar visibility modes in the ScrollContainerWidget.
	 * These modes determine when the vertical and horizontal scrollbars are displayed based on the content size relative to the container's bounds.
     */
    enum class EScrollbarMode : u8
    {
        Auto,  ///< The scrollbar is visible only when the content exceeds the container's bounds in the corresponding direction.
        Never, ///< The scrollbar is never visible, and the wheel doesn't scroll that axis. SetScrollOffset still does.
        Always ///< The scrollbar is always visible, regardless of the content size relative to the container's bounds.
    };

    /**
     * @brief A container widget that provides a clipping region for its children, allowing for scrollable content.
     * This widget supports vertical, horizontal, or bidirectional scrolling based on the layout and content size.
     *
     * The content node is an EOverflow::Scroll layout node: layout moves the children by the scroll offset and clamps
     * it, so hit-testing, navigation and painting use the children's FinalRects as they are. Focus moved by keyboard
     * or gamepad is scrolled into view by the scene (Scene::ScrollIntoView).
     *
     * Layout structure:
     *
     *   ScrollContainerWidget  [Vertical]
     *   +-- m_ContentRowID     [Horizontal, Flex height]
     *   |   +-- m_ContentNodeID  [Flex width, Flex height]  <- user children go here
     *   |   +-- VScrollbar       [Fixed width=16, Flex height]
     *   +-- HScrollbar           [Flex width, Fixed height=16]
     */
    class ScrollContainerWidget : public IWidget
    {
    public:
        static constexpr Unit c_ScrollbarSize = 16_u;
        static constexpr Unit c_WheelStep     = 40_u; ///< Distance scrolled per mouse wheel notch.

        /**
         * @brief Called with the new offset when the scroll offset changes. Changes made by layout (the content
         * shrinking, focus scrolled into view) are reported after the next layout.
         */
        Callback<ScrollContainerWidget&, Vec2<Unit>> OnScroll;

        /**
         * @brief Retrieves the internal content node ID where user-added child widgets are parented.
         * This is where children should be added to ensure they are properly clipped and scrolled within the container.
         */
        RATUI_NODISCARD NodeID      GetContentNodeID() const { return m_ContentNodeID; }
        RATUI_NODISCARD LayoutNode* GetContentNode() { return GetScene().GetLayoutNode( m_ContentNodeID ); }
        RATUI_NODISCARD const LayoutNode* GetContentNode() const { return GetScene().GetLayoutNode( m_ContentNodeID ); }

        RATUI_NODISCARD EScrollbarMode GetVScrollbarMode() const { return m_VScrollbarMode; }

		void SetVScrollbarMode( EScrollbarMode a_Mode ) 
        {
            if ( m_VScrollbarMode == a_Mode ) return;
			m_VScrollbarMode = a_Mode;

			if ( a_Mode != EScrollbarMode::Auto )
			{
				ShowScrollbar( m_VScrollbarID, a_Mode == EScrollbarMode::Never );
			}
        }

        RATUI_NODISCARD EScrollbarMode GetHScrollbarMode() const { return m_HScrollbarMode; }

        void SetHScrollbarMode( EScrollbarMode a_Mode )
        {
            if ( m_HScrollbarMode == a_Mode ) return;
            m_HScrollbarMode = a_Mode;

            if ( a_Mode != EScrollbarMode::Auto )
            {
				ShowScrollbar( m_HScrollbarID, a_Mode == EScrollbarMode::Never );
            }
        }

        // =====================================================================
        // IWidget overrides
        // =====================================================================

        void OnConstruct() override
        {
            Scene& scene = GetScene();
            // Root node: vertical layout so the h-scrollbar row sits below the content row.
            {
                GetLayout()
                    .LayoutType( ELayoutType::Vertical )
                    .WidthMode( ESizing::Flex )
                    .HeightMode( ESizing::Flex );
            }

            EnsureContentRow();
            EnsureScrollbars();
        }


        Reply OnPointerScroll( const PointerEvent& a_Event ) override
        {
            const Vec2<Unit> before = GetScrollOffset();
            const f32 dx = m_HScrollbarMode == EScrollbarMode::Never ? 0.f : a_Event.ScrollDelta[0].ToFloat();
            const f32 dy = m_VScrollbarMode == EScrollbarMode::Never ? 0.f : a_Event.ScrollDelta[1].ToFloat();
            SetScrollOffset( { before[0] + c_WheelStep * dx, before[1] - c_WheelStep * dy } );

            // Unmoved at either end, the scroll passes on to an enclosing scroll container.
            const Vec2<Unit> after = GetScrollOffset();
            const bool moved = after[0] != before[0] || after[1] != before[1];
            return moved ? Reply::Handled() : Reply::Unhandled();
        }

        bool OnArranged() override
        {
            const Vec2<Unit> max = GetMaxScrollOffset();
            bool relayout = UpdateAutoScrollbar( m_HScrollbarID, m_HScrollbarMode, max[0] );
            relayout     |= UpdateAutoScrollbar( m_VScrollbarID, m_VScrollbarMode, max[1] );

            SyncScrollbars();

            // OnScroll is user code, which may change the scene: not in the middle of layout.
            const Vec2<Unit> offset = GetScrollOffset();
            if ( offset[0] != m_NotifiedOffset[0] || offset[1] != m_NotifiedOffset[1] )
                GetScene().NotifyAfterLayout( GetLayoutID() );
            return relayout;
        }

        void OnLayoutDone() override { NotifyScroll(); }

        void OnPaint( const PaintEvent& a_Event ) override
        {
            Scene& scene = GetScene();

            // Layout already placed the content where it scrolled to; it only needs clipping to the viewport.
            if ( LayoutNode* contentNode = GetContentNode() )
            {
                a_Event.Drawer.PushClipRect( contentNode->Layout.FinalRect );
                contentNode->ForEachChild( [&]( LayoutNode& child )
                {
                    if ( child.Widget )
                        child.Widget->Paint( a_Event );
                } );
                a_Event.Drawer.PopClipRect();
            }

            // Draw scrollbars if needed
            if ( IWidget* vScrollbar = scene.GetWidget( m_VScrollbarID ) )
            {
                vScrollbar->Paint( a_Event );
            }

            if ( IWidget* hScrollbar = scene.GetWidget( m_HScrollbarID ) )
            {
                hScrollbar->Paint( a_Event );
            }
        }

        // =====================================================================
        // Scroll offset access
        // =====================================================================

        RATUI_NODISCARD Vec2<Unit> GetScrollOffset() const
        {
            const LayoutNode* contentNode = GetContentNode();
            return contentNode ? contentNode->Layout.ScrollOffset : Vec2<Unit>{ 0_u, 0_u };
        }

        /** @brief How far the content extends past the viewport, as of the last layout. */
        RATUI_NODISCARD Vec2<Unit> GetMaxScrollOffset() const
        {
            const LayoutNode* contentNode = GetContentNode();
            return contentNode ? contentNode->Layout.MaxScrollOffset : Vec2<Unit>{ 0_u, 0_u };
        }

        /**
         * @brief Scrolls to @p a_Offset, clamped to the content's extent.
         * Content added since the last layout counts too: the next layout clamps the request again.
         */
        void SetScrollOffset( Vec2<Unit> a_Offset )
        {
            if ( LayoutNode* contentNode = GetContentNode() )
                ScrollLayoutNode( *contentNode, a_Offset );

            SyncScrollbars();
            NotifyScroll();
        }

    protected:
        EScrollbarMode m_VScrollbarMode{ EScrollbarMode::Auto };
        EScrollbarMode m_HScrollbarMode{ EScrollbarMode::Auto };
        Vec2<Unit>     m_NotifiedOffset{ 0_u, 0_u }; ///< The offset OnScroll last reported.
        bool           m_SyncingScrollbars{ false };

        void NotifyScroll()
        {
            const Vec2<Unit> offset = GetScrollOffset();
            if ( offset[0] == m_NotifiedOffset[0] && offset[1] == m_NotifiedOffset[1] )
                return;

            m_NotifiedOffset = offset;
            if ( OnScroll ) OnScroll( *this, offset );
        }

        // Internal layout node IDs
        NodeID   m_ContentRowID{ c_InvalidNodeID };  ///< Horizontal row containing content + v-scrollbar.
        NodeID   m_ContentNodeID{ c_InvalidNodeID }; ///< Flex node that user children are parented to.

        // Scrollbar widget IDs
        NodeID m_VScrollbarID{ c_InvalidNodeID };
        NodeID m_HScrollbarID{ c_InvalidNodeID };

		void ShowScrollbar( NodeID a_ScrollbarID, bool a_Hidden = false )
		{
			if ( IWidget* scrollbar = GetScene().GetWidget( a_ScrollbarID ) )
			{
				if ( LayoutNode* scrollbarNode = GetScene().GetLayoutNode( scrollbar->GetLayoutID() ) )
				{
					scrollbarNode->Style.Visibility = a_Hidden ? EVisibility::Collapsed : EVisibility::Visible;
					scrollbarNode->MarkDirty();
				}
			}
		}

        // =====================================================================
        // Setup helpers
        // =====================================================================

        /**
         * @brief Creates the horizontal content row node and the content node inside it.
         *
         * Content row:  Horizontal layout, Flex in both axes (fills root minus h-scrollbar).
         * Content node: Flex in both axes (fills row minus v-scrollbar).
         */
        void EnsureContentRow()
        {
            if ( m_ContentRowID != c_InvalidNodeID )
                return;

            Scene& scene = GetScene();
            LayoutNode& selfNode = GetLayout();

            // --- Content row ---
            LayoutNode& rowNode = scene.CreateLayoutNode( {}, GetLayoutID() );
            m_ContentRowID = rowNode.ID;
            rowNode
                .LayoutType( ELayoutType::Horizontal )
                .WidthMode( ESizing::Flex )
                .HeightMode( ESizing::Flex );   

            // --- Content node (user children go here) ---
            LayoutNode& contentNode = scene.CreateLayoutNode( {}, rowNode.ID );
            m_ContentNodeID = contentNode.ID;
            contentNode
                .WidthMode( ESizing::Flex )
                .HeightMode( ESizing::Flex )
                .LayoutType( ELayoutType::Vertical )
                .Overflow( EOverflow::Scroll );
        }

        /**
         * @brief Creates both scrollbar widgets and inserts them at the correct positions.
         *
         * V-scrollbar: appended to the content row
         * H-scrollbar: appended to the root node  
         */
        void EnsureScrollbars()
        {
            Scene& scene = GetScene();
            LayoutNode& selfNode = GetLayout();
            LayoutNode& rowNode = *scene.GetLayoutNode( m_ContentRowID );

            const auto setUpScrollbarStyle = [&]( SliderWidget& a_Slider, EOrient a_Orientation )
            {
				a_Slider.Orientation = a_Orientation;
                a_Slider.SetNavRole( ENavRole::None ); // For the pointer: navigation moves through the content instead.
                a_Slider.Min = 0.f;
                a_Slider.Max = 1.f;
                
				// Slider track should be the same size as the scrollbar, so set the thickness to fill the entire scrollbar in the non-scrolling axis.
                a_Slider.ShowTrackFill = false;
                a_Slider.ScaleThumbOnTrackAxis = true;

                // Values are in content units, so the wheel over a bar scrolls as far as over the content.
                a_Slider.ScrollStep = c_WheelStep.ToFloat();

                const u32 axis = a_Orientation == EOrient::Horizontal ? 0 : 1;
                a_Slider.Value.Subscribe( [this, axis]( const f32& a_Value )
                {
                    // Matching the bar to the offset must not replace a pending, not yet clamped, request.
                    if ( m_SyncingScrollbars )
                        return;

                    Vec2<Unit> offset = GetScrollOffset();
                    offset[axis] = Unit( a_Value );
                    SetScrollOffset( offset );
                }, false );
            };

            // --- Vertical scrollbar ---
            {
                SliderWidget* vScroll = scene.CreateWidget<SliderWidget>( m_ContentRowID );
				m_VScrollbarID        = vScroll->GetLayoutID();
                LayoutNode& vNode     = vScroll->GetLayout();

                vNode
                    .FixedWidth( c_ScrollbarSize )
                    .HeightMode( ESizing::Flex );

				setUpScrollbarStyle( *vScroll, EOrient::Vertical );
            }

            // --- Horizontal scrollbar ---
            {
                SliderWidget* hScroll = scene.CreateWidget<SliderWidget>( GetLayoutID() );
                m_HScrollbarID		  = hScroll->GetLayoutID();
                LayoutNode& hNode     = hScroll->GetLayout();

                hNode
                    .WidthMode( ESizing::Flex )
                    .FixedHeight( c_ScrollbarSize );

				setUpScrollbarStyle( *hScroll, EOrient::Horizontal );
            }
        }

        /** @brief Matches the scrollbars' range, thumb and value to the content and scroll offset. */
        void SyncScrollbars()
        {
            Scene& scene = GetScene();
            const LayoutNode* contentNode = GetContentNode();
            if ( !contentNode )
                return;

            const Rect<Unit>& viewport  = contentNode->Layout.FinalRect;
            const Vec2<Unit>  offset    = contentNode->Layout.ScrollOffset;
            const Vec2<Unit>  maxOffset = contentNode->Layout.MaxScrollOffset;

            m_SyncingScrollbars = true;
            const auto sync = [&]( NodeID a_ScrollbarID, u32 a_Axis )
            {
                SliderWidget* bar = scene.GetWidget<SliderWidget>( a_ScrollbarID );
                if ( !bar )
                    return;

                const f32 viewportSize = viewport.Size[a_Axis].ToFloat();
                const f32 contentSize  = viewportSize + maxOffset[a_Axis].ToFloat();
                bar->Min = 0.f;
                bar->Max = maxOffset[a_Axis].ToFloat();
                bar->ThumbScale = contentSize <= 0.f ? 1.f : std::clamp( viewportSize / contentSize, 0.f, 1.f );
                bar->Value.Set( offset[a_Axis].ToFloat() );
            };

            sync( m_HScrollbarID, 0 );
            sync( m_VScrollbarID, 1 );
            m_SyncingScrollbars = false;
        }

        /** @brief Shows an Auto scrollbar only while the content overflows on its axis. Returns whether it changed. */
        bool UpdateAutoScrollbar( NodeID a_ScrollbarID, EScrollbarMode a_Mode, Unit a_MaxOffset )
        {
            if ( a_Mode != EScrollbarMode::Auto )
                return false;

            // Showing a bar only shrinks the viewport and hiding one only grows it, so this settles without flicker.
            const LayoutNode* bar = GetScene().GetLayoutNode( a_ScrollbarID );
            if ( !bar )
                return false;

            const bool hidden    = bar->Style.Visibility == EVisibility::Collapsed;
            const bool overflows = a_MaxOffset > 0_u;
            if ( hidden != overflows )
                return false;

            ShowScrollbar( a_ScrollbarID, !overflows );
            return true;
        }
    };

} // namespace RatUI
