#pragma once
#include "Theme.h"
#include "Scene.h"
#include "FocusRing.h"
#include "IWidget.h"

namespace RatUI
{
    /**
     * @brief A filled, optionally bordered and rounded box that lays out its children.
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
        FocusRingStyle FocusRing{};                          ///< Drawn while the widget has focus (theme: FocusOutline keys)

        // --------------------------------------------------------------------
        // IWidget Overrides
        // --------------------------------------------------------------------

        // A panel is only background and layout (ENavRole::None), so navigation passes through it to the widgets inside.
        // With SetNavRole( ENavRole::Group ) it is a group: navigation stops on it, Activate enters it and Cancel leaves it.
        bool IsInteractable() const override { return false; }

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
                    FocusRing.UpdateFromTheme( Theme );
                }
            }

            a_Event.Drawer.AddBrushRect( rect, FillBrush, BorderColor, BorderThickness, Radius );

            PaintChildren( a_Event );

            // Focus ring after the children, so they don't cover it.
            if ( scene.IsFocusVisible( GetLayoutID() ) )
                FocusRing.Draw( a_Event.Drawer, rect, Radius );
        }
    };

} // namespace RatUI