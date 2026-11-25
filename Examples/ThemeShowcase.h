#pragma once
#include "Example.h"
#include <RatUI/Widget/WidgetBuilder.h>
#include <cmath>
#include <format>

/**
 * @brief A panel filled with a warning tint, whatever the theme. As ThemeShowcase's root it clears the safe area, so its
 * fill shows in the unsafe bands around the content when a safe area is set.
 */
class UnsafeAreaBackdrop : public PanelWidget
{
public:
    using Super = PanelWidget;

    bool IsFocusable() const override { return false; }

    void OnPaint( const PaintEvent& a_Event ) override
    {
        a_Event.Drawer.AddBrushRect( GetLayout().Layout.FinalRect, SolidBrush{ FromColorF32( 1.f, 0.294f, 0.404f, 0.35f ) },
                                     Colors::Transparent, 0_u, CornerRadius::None() );
        PaintChildren( a_Event );
    }
};

/**
 * @brief The showcase's own factories, composed with the standard ones (see ShowcaseSceneBuilder below).
 * This is how you add your own ui.Something() calls: a mixin templated on the final builder type B.
 */
template<typename B>
struct ShowcaseFactories
{
    WidgetBuilder<PanelWidget> Section() const
    {
        return Ui().Panel().Flex().Padding( 12_u ).Spacing( 10_u );
    }

    WidgetBuilder<TextWidget> Label( String a_String, Unit a_Size ) const
    {
        return Ui().Text( std::move( a_String ) ).Size( a_Size ).NoWrap();
    }

    /** @brief A button that grows up to a_MaxWidth and shrinks on narrow screens. */
    WidgetBuilder<ButtonWidget> WideButton( String a_Label, Unit a_MaxWidth, ButtonWidget::OnClickCallback a_OnClick,
                                            TextWidget** a_OutLabel = nullptr ) const
    {
        TextWidget* label = nullptr;
        WidgetBuilder<ButtonWidget> button = Ui().Button( std::move( a_OnClick ) ).FlexWidth().FixedHeight( 38_u ).MaxWidth( a_MaxWidth )(
            Ui().Text( std::move( a_Label ) ).NoWrap().Visibility( EVisibility::HitTestInvisible ).Bind( label )
        );
        if ( a_OutLabel )
            *a_OutLabel = label;
        return button;
    }

    /** @brief Sizes a text field: fills the column, up to 280 wide. */
    WidgetBuilder<InputTextWidget> Field( WidgetBuilder<InputTextWidget> a_Input ) const
    {
        return a_Input.FlexWidth().ContentHeight().Padding( 6_u ).MaxWidth( 280_u );
    }

    /** @brief A card with a slider and a label showing its value. */
    WidgetBuilder<PanelWidget> LabeledSlider( String a_Label, f32 a_Value, EOrient a_Orientation = EOrient::Horizontal ) const
    {
        const bool vertical = a_Orientation == EOrient::Vertical;

        WidgetBuilder<SliderWidget> slider = Ui().Slider( 0.f, 1.f, a_Value, a_Orientation );
        if ( vertical )
            slider.FixedSize( 36_u, 120_u );
        else
            slider.FlexWidth().FixedHeight( 28_u );

        TextWidget* label = nullptr;
        WidgetBuilder<PanelWidget> card = Section().Vertical().FixedHeight( vertical ? 180_u : 86_u ).Padding( 8_u ).Spacing( 6_u )(
            Label( "", 14_u ).Bind( label ),
            slider
        );

        slider.OnValueChanged( [label, a_Label]( const f32& a_Current )
        {
            label->SetText( Text{ std::format( "{}: {}%", a_Label, static_cast<int>( std::round( a_Current * 100.f ) ) ) } );
        } );
        return card;
    }

private:
    const B& Ui() const { return static_cast<const B&>( *this ); }
};

using ShowcaseSceneBuilder = BasicSceneBuilder<StandardFactories, ShowcaseFactories>;

/**
 * @brief Buttons, sliders and text, restyled live by switching between four themes.
 * The Safe Area button simulates a phone's notch, rounded corners and home bar.
 */
class ThemeShowcase : public Example
{
public:
    explicit ThemeShowcase( ITextMetrics& a_TextMetrics )
        : Example( a_TextMetrics )
    {
        // Every widget shares m_ActiveTheme, so copying another theme into it restyles the whole scene.
        m_Scene.DefaultTheme = m_ActiveTheme;

        ShowcaseSceneBuilder ui{ m_Scene };

        WidgetBuilder<PanelWidget> themeButtons = ui.Section().Horizontal().ContentHeight().FocusScope();
        for ( size i = 0; i < Size( m_Themes ); ++i )
            themeButtons( ui.WideButton( m_Themes[i].Name, 130_u, [this, i]( ButtonBaseWidget& ) { ApplyTheme( i ); } ) );
        themeButtons( ui.WideButton( "", 170_u, [this]( ButtonBaseWidget& ) { SetSafeAreaEnabled( !m_SafeAreaEnabled ); }, &m_SafeAreaLabel ) );

        // The backdrop keeps the whole window, so its tint fills the unsafe bands; the content is laid out clear of them.
        ui.Root( ui.MakeWidget<UnsafeAreaBackdrop>().Flex().SafeAreaEdges( ESafeAreaEdges::All )( ui.Panel().Vertical().Flex().Padding( 16_u ).Spacing( 12_u ).FocusScope()(
            ui.Label( "Theme Showcase", 30_u ),
            ui.Text( "" ).Size( 16_u ).FlexWidth().Bind( m_Status ),
            themeButtons,
            ui.Section().Horizontal().FocusScope()(
                // Left column: sliders.
                ui.Section().Vertical().Grow( 1.f ).FocusScope()(
                    ui.Label( "Controls", 20_u ),
                    ui.LabeledSlider( "Master Volume",   0.65f ),
                    ui.LabeledSlider( "Accent Strength", 0.30f ),
                    ui.LabeledSlider( "Vertical Mix",    0.45f, EOrient::Vertical )
                ),
                // Right column: text wrapping, typewriter reveal, overflow fade and a button.
                ui.Section().Vertical().Grow( 1.f ).FocusScope()(
                    ui.Label( "Preview", 20_u ),
                    ui.Text( "This panel uses the active theme for panel fills, text color, button states, and slider visuals." )
                        .Size( 16_u ).FlexWidth(),
                    ui.Text( "My game dialogue uses the visible glyphs system. This only affects the rendering of the text, not the layout "
                             "or shaped text data, which is useful for text revealed over time, such as in a dialogue system." )
                        .Size( 16_u ).FlexWidth().FlexHeight().MaxWidth( 280_u ).Bind( m_Typewriter ),
                    ui.Text( "This is an example of a long text string that will exceed its box and fade out instead of being cut off." )
                        .Size( 16_u ).Overflow( ETextOverflow::Fade ).FixedSize( 100_u, 100_u ),
                    ui.WideButton( "Preview Button", 280_u, [this]( ButtonBaseWidget& )
                    {
                        m_Status->SetText( Text{ "Theme applied: " + m_Themes[m_ThemeIndex].Name + " (preview button clicked)" } );
                    } ),

                    // Text input: Enter in the single-line field submits to the status line.
                    ui.Label( "Text Input", 20_u ),
                    ui.Field( ui.InputText() )
                        .Placeholder( "Type here and press Enter" )
                        .OnSubmit( [this]( const String& a_Text ) { m_Status->SetText( Text{ "Submitted: " + a_Text } ); } ),
                    ui.Field( ui.TextArea() ).Text( "Multi-line field.\nEnter adds a line." )
                )
            )
        ) ) );

        ApplyTheme( 0 );
        SetSafeAreaEnabled( false );
    }

    void Update( f32 a_DeltaSeconds ) override
    {
        // Reveal the dialogue a glyph at a time, restarting every 10 seconds.
        m_Time = std::fmod( m_Time + a_DeltaSeconds, 10.f );
        m_Typewriter->VisibleGlyphs = static_cast<u32>( 500.f * m_Time / 10.f );
    }

private:
    struct NamedTheme
    {
        String        Name;
        Shared<Theme> Style;
    };

    Array<NamedTheme> m_Themes{ { "Dark", MakeDarkTheme() }, { "Light", MakeLightTheme() }, { "Neon", MakeNeonTheme() }, { "Minecraft", MakeMinecraftTheme() } };
    Shared<Theme>     m_ActiveTheme{ MakeShared<Theme>( *m_Themes[0].Style ) };
    size              m_ThemeIndex{ 0 };
    TextWidget*       m_Status{ nullptr };
    TextWidget*       m_Typewriter{ nullptr };
    f32               m_Time{ 0.f };
    TextWidget*       m_SafeAreaLabel{ nullptr };
    bool              m_SafeAreaEnabled{ false };

    /** @brief Toggles a phone-like safe area: a notch on top, a home bar at the bottom and rounded corners at the sides. */
    void SetSafeAreaEnabled( bool a_Enabled )
    {
        m_SafeAreaEnabled = a_Enabled;
        m_Scene.SetSafeArea( a_Enabled ? Edges{ .T = 44_u, .R = 24_u, .B = 34_u, .L = 24_u } : Edges{} );
        m_SafeAreaLabel->SetText( Text{ a_Enabled ? "Safe Area: On" : "Safe Area: Off" } );
    }

    void ApplyTheme( size a_Index )
    {
        m_ThemeIndex   = a_Index;
        *m_ActiveTheme = *m_Themes[a_Index].Style;
        m_Status->SetText( Text{ "Theme applied: " + m_Themes[a_Index].Name + " (use the buttons below to switch styles)" } );
    }

    // -------------------------------------------------------------------------
    // Themes
    // -------------------------------------------------------------------------

    static Shared<Theme> MakeDarkTheme()
    {
        Shared<Theme> theme = MakeShared<Theme>( *Themes::Dark() );
        theme->SetFont( ThemeKey::Font::Default, Fonts::Roboto );
        theme->SetBrushes( {
            { ThemeKey::Brush::SliderThumbHover,   SolidBrush{ Colors::LightBlue  } },
            { ThemeKey::Brush::SliderThumbPressed, SolidBrush{ Colors::AccentBlue } },
            { ThemeKey::Brush::SliderTrackFill,    SolidBrush{ Colors::AccentBlue } }
        } );
        theme->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = Colors::White } );
        return theme;
    }

    static Shared<Theme> MakeLightTheme()
    {
        Shared<Theme> theme = MakeShared<Theme>( *Themes::Dark() );
        theme->SetFont( ThemeKey::Font::Default, Fonts::Roboto );
        theme->SetColors( {
            { ThemeKey::Color::FocusOutline,       Colors::DarkBlue   }
        } );
        theme->SetBrushes( {
            { ThemeKey::Brush::SliderTrack,        SolidBrush{ Colors::Silver     } },
            { ThemeKey::Brush::SliderTrackFill,    SolidBrush{ Colors::DarkBlue   } },
            { ThemeKey::Brush::SliderThumb,        SolidBrush{ Colors::AccentBlue } },
            { ThemeKey::Brush::SliderThumbHover,   SolidBrush{ Colors::Blue       } },
            { ThemeKey::Brush::SliderThumbPressed, SolidBrush{ Colors::DarkBlue   } },
            { ThemeKey::Brush::PanelNormal,   SolidBrush{ Colors::LightGray  } },
            { ThemeKey::Brush::ButtonNormal,  SolidBrush{ Colors::White      } },
            { ThemeKey::Brush::ButtonHover,   SolidBrush{ Colors::PowderBlue } },
            { ThemeKey::Brush::ButtonPressed, SolidBrush{ Colors::LightBlue  } }
        } );
        theme->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = Colors::Surface900 } );
        return theme;
    }

    static Shared<Theme> MakeNeonTheme()
    {
        Shared<Theme> theme = MakeShared<Theme>( *Themes::Dark() );
        theme->SetFont( ThemeKey::Font::Default, Fonts::Roboto );
        theme->SetColors( {
            { ThemeKey::Color::FocusOutline,       Colors::AccentRose }
        } );
        theme->SetRadii( {
            { ThemeKey::Radii::Panel,       CornerRadius::All( 12_u ) },
            { ThemeKey::Radii::Button,      CornerRadius::All( 10_u ) },
            { ThemeKey::Radii::SliderTrack, CornerRadius::All(  5_u ) },
            { ThemeKey::Radii::SliderThumb, CornerRadius::All(  8_u ) }
        } );
        theme->SetBrushes( {
            { ThemeKey::Brush::SliderTrack,        SolidBrush{ FromColorF32( 0.10f, 0.10f, 0.20f ) } },
            { ThemeKey::Brush::SliderTrackFill,    SolidBrush{ Colors::AccentRose } },
            { ThemeKey::Brush::SliderThumb,        SolidBrush{ Colors::AccentSky } },
            { ThemeKey::Brush::SliderThumbHover,   SolidBrush{ Colors::LightCyan } },
            { ThemeKey::Brush::SliderThumbPressed, SolidBrush{ Colors::AccentRose } },
            { ThemeKey::Brush::PanelNormal,   SolidBrush{ FromColorF32( 0.07f, 0.03f, 0.10f ) } },
            { ThemeKey::Brush::ButtonNormal,  SolidBrush{ FromColorF32( 0.20f, 0.05f, 0.28f ) } },
            { ThemeKey::Brush::ButtonHover,   SolidBrush{ FromColorF32( 0.30f, 0.08f, 0.45f ) } },
            { ThemeKey::Brush::ButtonPressed, SolidBrush{ FromColorF32( 0.12f, 0.45f, 0.42f ) } }
        } );
        theme->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = Colors::AccentSky } );
        return theme;
    }

    static Shared<Theme> MakeMinecraftTheme()
    {
        Shared<Theme> theme = MakeDarkTheme();
        theme->SetFont( ThemeKey::Font::Default, Fonts::Minecraft );
        for ( const auto& [key, value] : theme->GetRadii() )
            theme->SetRadius( key, CornerRadius::None() ); // Sharp corners everywhere.

        theme->SetBrushes( {
            { ThemeKey::Brush::SliderTrackFill,    SolidBrush{ FromColorF32( 0.1f, 0.5f, 0.1f ) } },
            { ThemeKey::Brush::SliderThumb,        SolidBrush{ FromColorF32( 0.9f, 0.9f, 0.9f ) } },
            { ThemeKey::Brush::SliderThumbHover,   SolidBrush{ FromColorF32( 0.8f, 0.8f, 0.8f ) } },
            { ThemeKey::Brush::SliderThumbPressed, SolidBrush{ FromColorF32( 1.f,  1.f,  1.f  ) } }
        } );
        return theme;
    }
};
