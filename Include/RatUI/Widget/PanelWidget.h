#pragma once
#include "Theme.h"
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
        PanelWidget() = default;

        PanelWidget( Shared<const Theme> a_Theme )
        {
            m_Theme = std::move( a_Theme );
        }

        void SetTheme( Shared<const Theme> a_Theme )
        {
            m_Theme = std::move( a_Theme );
        }

        void OnPaint( DrawList& a_DrawList ) override
        {
            Scene& scene = GetScene();
            const LayoutNode* node = scene.Layouts.Get( GetLayoutID() );
            if ( !node || !Visibility::IsRendered( node->Layout.Visibility ) )
                return;

            const Rect<Unit>& rect = node->Layout.FinalRect;
            const Color fill = GetThemeColor( ThemeKey::Color::PanelNormal, Colors::Surface700 );
            const Color focusOutlineColor = GetThemeColor( ThemeKey::Color::PanelFocusOutline, Colors::White );
            const CornerRounding rounding = GetThemeRounding( ThemeKey::Rounding::Panel, CornerRounding::Uniform( 8_u ) );
            const Unit borderThickness = GetThemeMetric( ThemeKey::Metric::PanelBorderThickness, 1_u );

            if ( scene.GetFocusedWidget() == GetID() )
            {
                a_DrawList.AddRect( rect,
                {
                    .FillColor = fill,
                    .BorderColor = focusOutlineColor,
                    .BorderThickness = borderThickness,
                    .Rounding = rounding
                } );
            }
            else
            {
                a_DrawList.AddRect( rect,
                {
                    .FillColor = fill,
                    .Rounding = rounding
                } );
            }

            a_DrawList.PushClipRect( rect );
            scene.ForEachChildWidget( GetLayoutID(), [&]( IWidget& a_Child )
            {
                a_Child.OnPaint( a_DrawList );
            } );
            a_DrawList.PopClipRect();
        }

    private:
        Shared<const Theme> m_Theme;

        Color GetThemeColor( ThemeID a_ID, Color a_Default ) const
        {
            return m_Theme ? m_Theme->GetColor( a_ID, a_Default ) : a_Default;
        }

        CornerRounding GetThemeRounding( ThemeID a_ID, CornerRounding a_Default ) const
        {
            return m_Theme ? m_Theme->GetRounding( a_ID, a_Default ) : a_Default;
        }

        Unit GetThemeMetric( ThemeID a_ID, Unit a_Default ) const
        {
            return m_Theme ? m_Theme->GetMetric( a_ID, a_Default ) : a_Default;
        }
    };

} // namespace RatUI