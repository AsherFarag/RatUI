#pragma once
#include "Theme.h"
#include "../Renderer/DrawList.h"

namespace RatUI
{
    /**
     * @brief The outline drawn around a focused widget. Widgets with a theme fill it from the
     * FocusOutline keys (ThemeKey::Color::FocusOutline, Metric::FocusOutlineThickness, Radii::FocusOutline).
     */
    struct FocusRingStyle
    {
        Color                  OutlineColor{ Colors::AccentBlue };
        Unit                   Thickness{ 2_u };
        Optional<CornerRadius> Radius{}; ///< Unset: follow the widget's own corner radius.

        /** @brief Reads the FocusOutline values from a theme, keeping the current values for keys it doesn't set. */
        void UpdateFromTheme( const ThemeHandle& a_Theme )
        {
            OutlineColor = a_Theme.GetColor( ThemeKey::Color::FocusOutline, OutlineColor );
            Thickness    = a_Theme.GetMetric( ThemeKey::Metric::FocusOutlineThickness, Thickness );
            if ( const CornerRadius* radius = a_Theme.TryGetRadius( ThemeKey::Radii::FocusOutline ) )
                Radius = *radius;
        }

        /** @brief Draws the ring along the inside of a_Rect. */
        void Draw( DrawList& a_Drawer, const Rect<Unit>& a_Rect, const CornerRadius& a_WidgetRadius ) const
        {
            a_Drawer.AddRect( a_Rect,
            {
                .FillColor       = Colors::Transparent,
                .BorderColor     = OutlineColor,
                .BorderThickness = Thickness,
                .Radius          = Radius.value_or( a_WidgetRadius )
            } );
        }
    };

} // namespace RatUI
