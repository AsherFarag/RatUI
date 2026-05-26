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

            if ( a_Scene.GetFocusedWidget() == GetID() )
            {
                a_DrawList.AddRect( rect,
                {
                    .FillColor = fill,
                    .BorderColor = Colors::White,
                    .BorderThickness = 2_u,
                    .Rounding = Rounding
                } );
            }
            else
            {
                a_DrawList.AddRect( rect,
                {
                    .FillColor = fill,
                    .Rounding = Rounding
                } );
            }

            a_DrawList.PushClipRect( rect );
            a_Scene.ForEachChildWidget( GetLayoutID(), [&]( IWidget& a_Child )
            {
                a_Child.OnPaint( a_Scene, a_DrawList );
            } );
            a_DrawList.PopClipRect();
        }
    };

} // namespace RatUI