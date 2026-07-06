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

        // --------------------------------------------------------------------
        // Render Properties
        // --------------------------------------------------------------------

        Brush FillBrush{ SolidBrush{ Colors::Surface700 } }; ///< The brush used to fill the panel's background
        Color BorderColor{ Colors::Transparent };            ///< The color of the panel's border
        Unit  BorderThickness{ 0_u };                        ///< The thickness of the panel's border
        CornerRounding Rounding{ CornerRounding::None() };   ///< The corner rounding

        // --------------------------------------------------------------------
        // IWidget Overrides
        // --------------------------------------------------------------------

        bool IsInteractable()       const override { return false; }
		bool IsFocusable()          const override { return true; }
        bool IsNavigationBoundary() const override { return true; }

        void OnPaint( const PaintEvent& a_Event ) override
        {
			Scene& scene = GetScene();
            const LayoutNode& node = GetLayout();
            const Rect<Unit>& rect = node.Layout.FinalRect;

            if constexpr ( HasMixin<ThemeMixin> )
            {
                if ( Theme.Update() )
                {
                    FillBrush = Theme.GetBrush( ThemeKey::Brush::PanelNormal, FillBrush );
                    BorderColor = Theme.GetColor( ThemeKey::Color::PanelBorder, BorderColor );
                    BorderThickness = Theme.GetMetric( ThemeKey::Metric::PanelBorderThickness, BorderThickness );
                    Rounding = Theme.GetRounding( ThemeKey::Rounding::Panel, Rounding );
                }
            }

            if ( std::holds_alternative<SolidBrush>( FillBrush ) )
            {
                const SolidBrush& solid = std::get<SolidBrush>( FillBrush );
                a_Event.Drawer.AddRect( rect, 
                {
                    .FillColor = solid.Fill,
                    .BorderColor = BorderColor,
                    .BorderThickness = BorderThickness,
                    .Rounding = Rounding
                } );
            }
            else if ( std::holds_alternative<TextureBrush>( FillBrush ) )
            {
                const TextureBrush& texture = std::get<TextureBrush>( FillBrush );
                a_Event.Drawer.AddRect( rect, 
                {
                    .FillColor = texture.Tint,
                    .BorderColor = BorderColor,
                    .BorderThickness = BorderThickness,
                    .Rounding = Rounding,
                    .Texture = texture.Texture
                } );
            }
            else if ( std::holds_alternative<NineSliceBrush>( FillBrush ) )
            {
                const NineSliceBrush& nineSlice = std::get<NineSliceBrush>( FillBrush );
                a_Event.Drawer.AddSlicedRect( rect, 
                {
                    .Texture = nineSlice.Texture,
                    .Slice = nineSlice.Slice,
                    .Tint = nineSlice.Tint
                } );
            }

            if ( scene.GetFocusedNode() == GetLayoutID() )
            {
                a_Event.Drawer.AddRect( rect,
                {
					.FillColor = Colors::Transparent,
					.BorderColor = BorderColor,
					.BorderThickness = BorderThickness,
                    .Rounding = Rounding
                } );
            }

            PaintChildren( a_Event );
        }
    };

} // namespace RatUI