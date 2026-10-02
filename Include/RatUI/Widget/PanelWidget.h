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
        CornerRadius Radius{ CornerRadius::None() };         ///< The corner rounding

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
                    Radius = Theme.GetRadius( ThemeKey::Radii::Panel, Radius );
                }
            }

            if ( Holds<SolidBrush>( FillBrush ) )
            {
                const SolidBrush& solid = Get<SolidBrush>( FillBrush );
                a_Event.Drawer.AddRect( rect, 
                {
                    .FillColor = solid.Fill,
                    .BorderColor = BorderColor,
                    .BorderThickness = BorderThickness,
                    .Radius = Radius
                } );
            }
            else if ( Holds<TextureBrush>( FillBrush ) )
            {
                const TextureBrush& texture = Get<TextureBrush>( FillBrush );
                a_Event.Drawer.AddRect( rect, 
                {
                    .FillColor = texture.Tint,
                    .BorderColor = BorderColor,
                    .BorderThickness = BorderThickness,
                    .Radius = Radius,
                    .Texture = texture.Texture
                } );
            }
            else if ( Holds<NineSliceBrush>( FillBrush ) )
            {
                const NineSliceBrush& nineSlice = Get<NineSliceBrush>( FillBrush );
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
                    .Radius = Radius
                } );
            }

            PaintChildren( a_Event );
        }
    };

} // namespace RatUI