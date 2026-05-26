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
        Callback<ScrollContainerWidget&, Vec2<Unit>> OnScroll; ///< Callback invoked when the scroll offset changes, providing the new offset.

        EScrollbarMode VScrollbarMode{ EScrollbarMode::Auto };
        EScrollbarMode HScrollbarMode{ EScrollbarMode::Auto };

        static constexpr Unit c_ScrollbarSize = 16_u;

        // =====================================================================
        // IWidget overrides
        // =====================================================================

        void OnConstruct( Scene& a_Scene ) override
        {
            // Root node: vertical layout so the h-scrollbar row sits below the content row.
            {
                LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
                node->Style.LayoutType = ELayoutType::Vertical;
                node->Style.WidthMode  = ESizingMode::Flex;
                node->Style.HeightMode = ESizingMode::Flex;
            }

            EnsureContentRow( a_Scene );
            EnsureScrollbars( a_Scene );
        }

        /**
         * @brief Reparents @p a_ChildNode under the internal content node instead of
         *        directly under this widget's root node.
         */
        void AddChild( Scene& a_Scene, LayoutNode* a_ChildNode )
        {
            a_Scene.Layouts.Get( m_ContentNodeID )->PushBackChild( *a_ChildNode );
        }

        bool IsFocusable( Scene& a_Scene ) const override { return true; }

        void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
        {
            const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
            if ( !node || !Visibility::IsRendered( node->Layout.Visibility ) )
                return;

            UpdateScrollMetrics( a_Scene );

            // Draw content with clipping and translation based on scroll offset
            {
                const Rect<Unit> contentRect = a_Scene.Layouts.Get( m_ContentNodeID )->Layout.FinalRect;
                a_DrawList.PushClipRect( contentRect );

                const bool hasTranslation = !IsApproxEqual( m_ScrollOffset[0].ToFloat(), 0.f ) ||
                    !IsApproxEqual( m_ScrollOffset[1].ToFloat(), 0.f );

                if ( hasTranslation )
                {
                    const Mat3<Unit> translation = Mat3<Unit>::from_columns(
                          Vec3<Unit>{ 1_u, 0_u, 0_u },
                          Vec3<Unit>{ 0_u, 1_u, 0_u },
                          Vec3<Unit>{ -m_ScrollOffset[0], -m_ScrollOffset[1], 1_u } );
                    a_DrawList.PushTransform( translation );
                }

                a_Scene.ForEachChildWidget( m_ContentNodeID, [&]( IWidget& a_Child )
                {
                    a_Child.OnPaint( a_Scene, a_DrawList );
                } );

                if ( hasTranslation )
                    a_DrawList.PopTransform();

                a_DrawList.PopClipRect();
            }

            // Draw scrollbars if needed
            if ( IWidget* vScrollbar = a_Scene.GetWidget( m_VScrollbarID ) )
            {
                vScrollbar->OnPaint( a_Scene, a_DrawList );
            }

            if ( IWidget* hScrollbar = a_Scene.GetWidget( m_HScrollbarID ) )
            {
                hScrollbar->OnPaint( a_Scene, a_DrawList );
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
        Vec2<Unit> m_ScrollOffset{ 0_u, 0_u };
        Vec2<Unit> m_MaxScrollOffset{ 0_u, 0_u };

        // Internal layout node IDs
        NodeID   m_ContentRowID{ c_InvalidNodeID };  ///< Horizontal row containing content + v-scrollbar.
        NodeID   m_ContentNodeID{ c_InvalidNodeID }; ///< Flex node that user children are parented to.

        // Scrollbar widget IDs
        WidgetID m_VScrollbarID{ c_InvalidWidgetID };
        WidgetID m_HScrollbarID{ c_InvalidWidgetID };

        // =====================================================================
        // Setup helpers
        // =====================================================================

        /**
         * @brief Creates the horizontal content row node and the content node inside it.
         *
         * Content row:  Horizontal layout, Flex in both axes (fills root minus h-scrollbar).
         * Content node: Flex in both axes (fills row minus v-scrollbar).
         */
        void EnsureContentRow( Scene& a_Scene )
        {
            if ( m_ContentRowID != c_InvalidNodeID )
                return;

            LayoutNode* selfNode = a_Scene.Layouts.Get( GetLayoutID() );

            // --- Content row ---
            m_ContentRowID = a_Scene.Layouts.Allocate( LayoutNode{} );
            LayoutNode* rowNode = a_Scene.Layouts.Get( m_ContentRowID );
            selfNode->PushBackChild( *rowNode );

            rowNode->Style.LayoutType = ELayoutType::Horizontal;
            rowNode->Style.WidthMode = ESizingMode::Flex;
            rowNode->Style.HeightMode = ESizingMode::Flex;

            // --- Content node (user children go here) ---
            m_ContentNodeID = a_Scene.Layouts.Allocate( LayoutNode{} );
            LayoutNode* contentNode = a_Scene.Layouts.Get( m_ContentNodeID );
            rowNode->PushBackChild( *contentNode );

            contentNode->Style.WidthMode = ESizingMode::Flex;
            contentNode->Style.HeightMode = ESizingMode::Flex;
            contentNode->Style.LayoutType = ELayoutType::Vertical;
        }

        /**
         * @brief Creates both scrollbar widgets and inserts them at the correct positions.
         *
         * V-scrollbar: appended to the content row  � Fixed width, Flex height.
         * H-scrollbar: appended to the root node    � Flex width, Fixed height.
         */
        void EnsureScrollbars( Scene& a_Scene )
        {
            LayoutNode* selfNode = a_Scene.Layouts.Get( GetLayoutID() );
            LayoutNode* rowNode = a_Scene.Layouts.Get( m_ContentRowID );

            const auto setUpScrollbarStyle = [&]( SliderWidget& a_Slider, EOrientation a_Orientation )
            {
				a_Slider.Orientation = a_Orientation;
                a_Slider.Min = 0.f;
                a_Slider.Max = 1.f;
                
				// Slider track should be the same size as the scrollbar, so set the thickness to fill the entire scrollbar in the non-scrolling axis.
                a_Slider.ShowTrackFill = false;
				a_Slider.TrackThickness = c_ScrollbarSize;
				a_Slider.ThumbRounding = CornerRounding::Uniform( c_ScrollbarSize / 2 );
				a_Slider.TrackRounding = {};
                a_Slider.ScaleThumbOnTrackAxis = true;
                a_Slider.MinThumbSizeOnTrackAxis = 12_u;

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
				m_VScrollbarID        = a_Scene.CreateWidget<SliderWidget>( m_ContentRowID );
                SliderWidget* vScroll = a_Scene.GetWidget<SliderWidget>( m_VScrollbarID );
                LayoutNode* vNode     = a_Scene.Layouts.Get( vScroll->GetLayoutID() );

                vNode->Style.WidthMode  = ESizingMode::Fixed;
                vNode->Style.FixedWidth = c_ScrollbarSize;
                vNode->Style.HeightMode = ESizingMode::Flex;

				setUpScrollbarStyle( *vScroll, EOrientation::Vertical );
            }

            // --- Horizontal scrollbar ---
            {
                m_HScrollbarID        = a_Scene.CreateWidget<SliderWidget>( GetID() );
                SliderWidget* hScroll = a_Scene.GetWidget<SliderWidget>( m_HScrollbarID );
                LayoutNode* hNode     = a_Scene.Layouts.Get( hScroll->GetLayoutID() );

                hNode->Style.WidthMode   = ESizingMode::Flex;
                hNode->Style.HeightMode  = ESizingMode::Fixed;
                hNode->Style.FixedHeight = c_ScrollbarSize;

				setUpScrollbarStyle( *hScroll, EOrientation::Horizontal );
            }
        }

        void UpdateScrollMetrics( Scene& a_Scene )
        {
            const LayoutNode* contentNode = a_Scene.Layouts.Get( m_ContentNodeID );
            if ( !contentNode )
                return;

            const Rect<Unit>& viewportRect = contentNode->Layout.FinalRect;

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

            if ( SliderWidget* hScroll = a_Scene.GetWidget<SliderWidget>( m_HScrollbarID ) )
            {
                hScroll->Min = 0.f;
                hScroll->Max = maxX;
                hScroll->Value.Set( clampedX );
                const f32 contentWidth = viewportRect.Width().ToFloat() + maxX;
                hScroll->ThumbScale = contentWidth <= 0.f ? 1.f : std::clamp( viewportRect.Width().ToFloat() / contentWidth, 0.f, 1.f );
            }

            if ( SliderWidget* vScroll = a_Scene.GetWidget<SliderWidget>( m_VScrollbarID ) )
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