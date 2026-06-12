#include <RatUI/Widget/Theme.h>

namespace RatUI::Themes
{

    const Shared<const Theme> &Dark()
    {
        static Shared<const Theme> darkTheme = []()
        {
            auto theme = std::make_shared<Theme>();

            theme->SetColors( {
                { ThemeKey::Color::FocusOutline, Colors::White },
                
                // Slider
                { ThemeKey::Color::SliderThumb, Colors::AccentBlue },
                { ThemeKey::Color::SliderTrack, Colors::Surface600 },
                { ThemeKey::Color::SliderTrackFill, Colors::AccentBlue }
            } );

            theme->SetRoundings( {
                { ThemeKey::Rounding::Panel, CornerRounding::Uniform( 8_u ) },
                { ThemeKey::Rounding::Button, CornerRounding::Uniform( 6_u ) },
                { ThemeKey::Rounding::SliderTrack, CornerRounding::Uniform( 2_u ) },
                { ThemeKey::Rounding::SliderThumb, CornerRounding::Uniform( 4_u ) }
            } );

            theme->SetMetrics( {
                { ThemeKey::Metric::PanelBorderThickness, 1_u },
                { ThemeKey::Metric::ButtonBorderThickness, 1_u }
            } );

            theme->SetBrushes( {
                // Panel
                { ThemeKey::Brush::PanelNormal,        SolidBrush{ Colors::Surface700 } },

                // Button
                { ThemeKey::Brush::ButtonNormal,       SolidBrush{ Colors::Surface600 } },
                { ThemeKey::Brush::ButtonHover,        SolidBrush{ Colors::Surface500 } },
                { ThemeKey::Brush::ButtonPressed,      SolidBrush{ Colors::White } },
            } );

            // TODO
            //theme->SetTextStyles( {
            //    { ThemeKey::TextStyle::Default, TextRenderStyle{ .Font = nullptr, .FontSize = 16_u } }
            //} );

            return Shared<const Theme>{ std::move( theme ) };
        }();

        return darkTheme;
    }

} // namespace RatUI::Themes