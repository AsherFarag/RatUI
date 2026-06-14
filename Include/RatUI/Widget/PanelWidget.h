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

        PanelWidget( ThemeHandle a_Theme )
            : m_Theme( std::move( a_Theme ) )
        {}

        void SetTheme( ThemeHandle a_Theme )
        {
            m_Theme = std::move( a_Theme );
        }

        bool IsInteractable()       const override { return false; }
		bool IsFocusable()          const override { return true; }
        bool IsNavigationBoundary() const override { return true; }

        void OnPaint( DrawList& a_DrawList ) override
        {
            Scene& scene = GetScene();
            const LayoutNode* node = scene.Layouts.Get( GetLayoutID() );
            if ( !node || !Visibility::IsRendered( node->Layout.Visibility ) )
                return;

            const Rect<Unit>& rect = node->Layout.FinalRect;

            const Brush& panelBrush = m_Theme.GetBrush( ThemeKey::Brush::PanelNormal, SolidBrush{ Colors::Surface700 } );
            if ( std::holds_alternative<SolidBrush>( panelBrush ) )
            {
                const SolidBrush& solid = std::get<SolidBrush>( panelBrush );
                a_DrawList.AddRect( rect, 
                {
                    .FillColor = solid.Fill,
                    .BorderColor = m_Theme.GetColor( ThemeKey::Color::PanelBorder, Colors::Transparent ),
                    .BorderThickness = m_Theme.GetMetric( ThemeKey::Metric::PanelBorderThickness, 0_u ),
                    .Rounding = m_Theme.GetRounding( ThemeKey::Rounding::Panel, CornerRounding::None() )
                } );
            }
            else if ( std::holds_alternative<TextureBrush>( panelBrush ) )
            {
                const TextureBrush& texture = std::get<TextureBrush>( panelBrush );
                a_DrawList.AddRect( rect, 
                {
                    .FillColor = texture.Tint,
                    .BorderColor = m_Theme.GetColor( ThemeKey::Color::PanelBorder, Colors::Transparent ),
                    .BorderThickness = m_Theme.GetMetric( ThemeKey::Metric::PanelBorderThickness, 0_u ),
                    .Rounding = m_Theme.GetRounding( ThemeKey::Rounding::Panel, CornerRounding::None() ),
                    .Texture = texture.Texture
                } );
            }
            else if ( std::holds_alternative<NineSliceBrush>( panelBrush ) )
            {
                const NineSliceBrush& nineSlice = std::get<NineSliceBrush>( panelBrush );
                a_DrawList.AddSlicedRect( rect, 
                {
                    .Texture = nineSlice.Texture,
                    .Slice = nineSlice.Slice,
                    .Tint = nineSlice.Tint
                } );
            }

            if ( scene.GetFocusedNode() == GetLayoutID() )
            {
                a_DrawList.AddRect( rect,
                {
					.FillColor = Colors::Transparent,
					.BorderColor = m_Theme.GetColor( ThemeKey::Color::FocusOutline, Colors::White ),
					.BorderThickness = m_Theme.GetMetric( ThemeKey::Metric::FocusOutlineThickness, 2_u ),
                    .Rounding = m_Theme.GetRounding( ThemeKey::Rounding::FocusOutline, CornerRounding::None() )
                } );
            }

            a_DrawList.PushClipRect( rect );
            scene.ForEachChildWidget( GetLayoutID(), [&]( IWidget& a_Child )
            {
                a_Child.OnPaint( a_DrawList );
            } );
            a_DrawList.PopClipRect();
        }

    protected:
        ThemeHandle m_Theme;
    };

} // namespace RatUI