#pragma once
#include "Scene.h"
#include "IWidget.h"

namespace RatUI
{
    /**
     * @brief
     */
    class PanelWidget : public IWidget
    {
    public:
        Color          NormalColor{ Colors::Surface700 };
        Color          HoverColor{ Colors::Surface600 };
        Color          PressedColor{ Colors::Surface500 };
        Color          FocusOutlineColor{ Colors::White };
        CornerRounding Rounding{ CornerRounding::Uniform( 8_u ) };
        Unit           BorderThickness{ 1_u };

        void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
        {
            const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
            if ( !node || !Visibility::IsRendered( node->Layout.Visibility ) )
                return;

            const Rect<Unit>& rect = node->Layout.FinalRect;
            const Color fill = NormalColor;

            a_DrawList.AddRect( fill, rect, Rounding );

            if ( a_Scene.GetFocusedWidget() == GetID() )
                a_DrawList.AddRectBorder( FocusOutlineColor, rect.Expanded( 2_u ), Rounding + 2_u, 2_u );

            a_DrawList.PushClipRect( rect );
            a_Scene.ForEachChildWidget( GetID(), [&]( IWidget& a_Child )
            {
                a_Child.OnPaint( a_Scene, a_DrawList );
            } );
            a_DrawList.PopClipRect();
        }
    };

} // namespace RatUI