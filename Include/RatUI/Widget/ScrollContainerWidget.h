#pragma once
#include "Scene.h"
#include "IWidget.h"

namespace RatUI
{
    struct ScrollBar
    {
    };

    /**
     * @brief A container widget that provides a clipping region for its children, allowing for scrollable content.
     * This widget supports vertical, horizontal, or bidirectional scrolling based on the layout and content size.
     */
    class ScrollContainerWidget : public IWidget
    {
    public:
        virtual ~ScrollContainerWidget() = default;

        bool IsFocusable( Scene& a_Scene ) const override { return true; }

        void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
        {
            const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
            if ( !node || !Visibility::IsRendered( node->Layout.Visibility ) )
                return;

            const Rect<Unit>& rect = node->Layout.FinalRect;

            a_DrawList.PushClipRect( rect );
            a_Scene.ForEachChildWidget( GetID(), [&]( IWidget& a_Child )
            {
                a_Child.OnPaint( a_Scene, a_DrawList );
            } );
            a_DrawList.PopClipRect();
        }

    protected:
        Vec2<Unit> m_ScrollOffset{ 0_u, 0_u };
        ScrollBar  m_VerticalScrollBar;
        ScrollBar  m_HorizontalScrollBar;
    };

} // namespace RatUI