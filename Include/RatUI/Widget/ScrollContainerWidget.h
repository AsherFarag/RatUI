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
        Never, ///< The scrollbar is never visible, even if the content exceeds the container's bounds.
        Always ///< The scrollbar is always visible, regardless of the content size relative to the container's bounds.
    };

    /**
     * @brief A container widget that provides a clipping region for its children, allowing for scrollable content.
     * This widget supports vertical, horizontal, or bidirectional scrolling based on the layout and content size.
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

        Callback<ScrollContainerWidget&, Vec2<Unit>> OnScroll; ///< Callback invoked when the scroll offset changes, providing the new offset.

        /**
         * @brief Retrieves the internal content node ID where user-added child widgets are parented.
         * This is where children should be added to ensure they are properly clipped and scrolled within the container.
         */
        RATUI_NODISCARD NodeID      GetContentNodeID() const { return m_ContentNodeID; }
        RATUI_NODISCARD LayoutNode* GetContentNode() { return GetScene().GetLayoutNode( m_ContentNodeID ); }

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
                    .WidthMode( ESizingMode::Flex )
                    .HeightMode( ESizingMode::Flex );
            }

            EnsureContentRow();
            EnsureScrollbars();
        }

        bool IsFocusable() const override { return true; }

        void OnPaint( const PaintEvent& a_Event ) override
        {
            Scene& scene = GetScene();
            const LayoutNode& node = GetLayout();
            UpdateScrollMetrics();

            // Draw content with clipping and translation based on scroll offset
            {
                const Rect<Unit> contentRect = GetContentNode()->Layout.FinalRect;
                a_Event.Drawer.PushClipRect( contentRect );

                const bool hasTranslation = !IsApproxEqual( m_ScrollOffset[0].ToFloat(), 0.f ) ||
                    !IsApproxEqual( m_ScrollOffset[1].ToFloat(), 0.f );

                if ( hasTranslation )
                {
                    const Mat3<Unit> translation = Mat3<Unit>::from_columns(
                          Vec3<Unit>{ 1_u, 0_u, 0_u },
                          Vec3<Unit>{ 0_u, 1_u, 0_u },
                          Vec3<Unit>{ -m_ScrollOffset[0], -m_ScrollOffset[1], 1_u } );
                    a_Event.Drawer.PushTransform( translation );
                }

                if ( LayoutNode* contentNode = scene.GetLayoutNode( m_ContentNodeID ) )
                {
                    contentNode->ForEachChild( [&]( LayoutNode& child )
                    {
                        if ( child.Widget )
                            child.Widget->Paint( a_Event );
                    } );
                }

                if ( hasTranslation )
                    a_Event.Drawer.PopTransform();

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

        Vec2<Unit> GetScrollOffset() const { return m_ScrollOffset; }

        void SetScrollOffset( Vec2<Unit> a_Offset )
        {
            m_ScrollOffset = a_Offset;
            if ( OnScroll ) OnScroll( *this, m_ScrollOffset );
        }

    protected:
        EScrollbarMode m_VScrollbarMode{ EScrollbarMode::Auto };
        EScrollbarMode m_HScrollbarMode{ EScrollbarMode::Auto };
        Vec2<Unit> m_ScrollOffset{ 0_u, 0_u };
        Vec2<Unit> m_MaxScrollOffset{ 0_u, 0_u };

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
                .WidthMode( ESizingMode::Flex )
                .HeightMode( ESizingMode::Flex );   

            // --- Content node (user children go here) ---
            LayoutNode& contentNode = scene.CreateLayoutNode( {}, rowNode.ID );
            m_ContentNodeID = contentNode.ID;
            contentNode
                .WidthMode( ESizingMode::Flex )
                .HeightMode( ESizingMode::Flex )
                .LayoutType( ELayoutType::Vertical );
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

            const auto setUpScrollbarStyle = [&]( SliderWidget& a_Slider, EOrientation a_Orientation )
            {
				a_Slider.Orientation = a_Orientation;
                a_Slider.Min = 0.f;
                a_Slider.Max = 1.f;
                
				// Slider track should be the same size as the scrollbar, so set the thickness to fill the entire scrollbar in the non-scrolling axis.
                a_Slider.ShowTrackFill = false;
                a_Slider.ScaleThumbOnTrackAxis = true;

				// Subscribe to value changes to update scroll offset and invoke OnScroll callback

				if ( a_Orientation == EOrientation::Vertical )
				{
					a_Slider.Value.Subscribe( [this]( const f32& a_Value )
					{
						m_ScrollOffset[1] = Unit( a_Value );
						if ( OnScroll ) OnScroll( *this, m_ScrollOffset );
					} );
				}
				else
				{
					a_Slider.Value.Subscribe( [this]( const f32& a_Value )
					{
						m_ScrollOffset[0] = Unit( a_Value );
						if ( OnScroll ) OnScroll( *this, m_ScrollOffset );
					} );
				}
            };

            // --- Vertical scrollbar ---
            {
                SliderWidget* vScroll = scene.CreateWidget<SliderWidget>( m_ContentRowID );
				m_VScrollbarID        = vScroll->GetLayoutID();
                LayoutNode& vNode     = vScroll->GetLayout();

                vNode
                    .FixedWidth( c_ScrollbarSize )
                    .HeightMode( ESizingMode::Flex );

				setUpScrollbarStyle( *vScroll, EOrientation::Vertical );
            }

            // --- Horizontal scrollbar ---
            {
                SliderWidget* hScroll = scene.CreateWidget<SliderWidget>( GetLayoutID() );
                m_HScrollbarID		  = hScroll->GetLayoutID();
                LayoutNode& hNode     = hScroll->GetLayout();

                hNode
                    .WidthMode( ESizingMode::Flex )
                    .FixedHeight( c_ScrollbarSize );

				setUpScrollbarStyle( *hScroll, EOrientation::Horizontal );
            }
        }

        void UpdateScrollMetrics()
        {
            Scene& scene = GetScene();
            const LayoutNode* contentNode = GetContentNode();
            if ( !contentNode )
                return;

            const Rect<Unit> viewportRect = contentNode->Layout.FinalRect;

            Unit maxRight = viewportRect.Right();
            Unit maxBottom = viewportRect.Bottom();

            const auto AccumulateBounds = [&]( auto& Self, const LayoutNode& a_Node ) -> void
            {
                if ( !Visibility::AffectsLayout( a_Node.Layout.Visibility ) )
                    return;

                const Rect<Unit>& rect = a_Node.Layout.FinalRect;
                maxRight = std::max( maxRight, rect.Right() );
                maxBottom = std::max( maxBottom, rect.Bottom() );

                a_Node.ForEachChild( [&]( const LayoutNode& a_Child )
                {
                    Self( Self, a_Child );
                } );
            };

            contentNode->ForEachChild( [&]( const LayoutNode& a_Child )
            {
                AccumulateBounds( AccumulateBounds, a_Child );
            } );

            m_MaxScrollOffset[0] = std::max( 0_u, maxRight - viewportRect.Right() );
            m_MaxScrollOffset[1] = std::max( 0_u, maxBottom - viewportRect.Bottom() );

            const f32 maxX = m_MaxScrollOffset[0].ToFloat();
            const f32 maxY = m_MaxScrollOffset[1].ToFloat();

            const f32 clampedX = std::clamp( m_ScrollOffset[0].ToFloat(), 0.f, maxX );
            const f32 clampedY = std::clamp( m_ScrollOffset[1].ToFloat(), 0.f, maxY );
            m_ScrollOffset = { Unit( clampedX ), Unit( clampedY ) };

            if ( SliderWidget* hScroll = scene.GetWidget<SliderWidget>( m_HScrollbarID ) )
            {
                hScroll->Min = 0.f;
                hScroll->Max = maxX;
                hScroll->Value.Set( clampedX );
                const f32 contentWidth = viewportRect.Width().ToFloat() + maxX;
                hScroll->ThumbScale = contentWidth <= 0.f ? 1.f : std::clamp( viewportRect.Width().ToFloat() / contentWidth, 0.f, 1.f );
            }

            if ( SliderWidget* vScroll = scene.GetWidget<SliderWidget>( m_VScrollbarID ) )
            {
                vScroll->Min = 0.f;
                vScroll->Max = maxY;
                vScroll->Value.Set( clampedY );
                const f32 contentHeight = viewportRect.Height().ToFloat() + maxY;
                vScroll->ThumbScale = contentHeight <= 0.f ? 1.f : std::clamp( viewportRect.Height().ToFloat() / contentHeight, 0.f, 1.f );
            }
        }
    };

} // namespace RatUI
