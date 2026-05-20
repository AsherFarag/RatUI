#pragma once
#include "Scene.h"
#include "IWidget.h"

namespace RatUI
{
	enum class EScrollbarMode : u8
    {
		Auto,  ///< The scrollbar is visible only when the content exceeds the container's bounds in the corresponding direction.
		Never, ///< The scrollbar is never visible, even if the content exceeds the container's bounds.
		Always ///< The scrollbar is always visible, regardless of the content size relative to the container's bounds.
    };

    /**
     * @brief A container widget that provides a clipping region for its children, allowing for scrollable content.
     * This widget supports vertical, horizontal, or bidirectional scrolling based on the layout and content size.
     */
    class ScrollContainerWidget : public IWidget
    {
    public:
		Callback<ScrollContainerWidget&, Vec2<Unit>> OnScroll; ///< Callback invoked when the scroll offset changes, providing the new offset.

		EScrollbarMode VScrollbarMode = EScrollbarMode::Auto;
        EScrollbarMode HScrollbarMode = EScrollbarMode::Auto;

        bool IsFocusable( Scene& a_Scene ) const override { return true; }

        void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
        {
            const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
            if ( !node || !Visibility::IsRendered( node->Layout.Visibility ) )
                return;

            //UpdateScrollMetrics( a_Scene );

            const Rect<Unit>& rect = node->Layout.FinalRect;

            a_DrawList.PushClipRect( rect );

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

            a_Scene.ForEachChildWidget( GetID(), [&]( IWidget& a_Child )
            {
                a_Child.OnPaint( a_Scene, a_DrawList );
            } );

            if ( hasTranslation )
                a_DrawList.PopTransform();

            a_DrawList.PopClipRect();
        }

    protected:
        Vec2<Unit> m_ScrollOffset{ 0_u, 0_u };

    };

} // namespace RatUI