#pragma once
#include "Example.h"
#include <RatUI/Widget/ButtonWidget.h>
#include <RatUI/Widget/PanelWidget.h>
#include <RatUI/Widget/SliderWidget.h>
#include <RatUI/Widget/TextWidget.h>
#include <cmath>
#include <format>

/**
 * @brief Buttons, sliders and text, restyled live by switching between four themes.
 */
class ThemeShowcase : public Example
{
public:
    explicit ThemeShowcase( ITextMetrics& a_TextMetrics )
        : Example( a_TextMetrics )
    {
        // Every widget shares m_ActiveTheme, so copying another theme into it restyles the whole scene.
        m_Scene.DefaultTheme = m_ActiveTheme;

        PanelWidget* root = m_Scene.CreateRootWidget<PanelWidget>();
        root->GetLayout()
            .LayoutType( ELayoutType::Vertical )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Flex )
            .Padding( Edges::All( 16_u ) )
            .Spacing( 12_u )
            .FocusScope( true );

        AddText( root, "Theme Showcase", 30_u );
        m_Status = AddText( root, "", 16_u );

        PanelWidget* themeButtons = AddPanel( root, ELayoutType::Horizontal );
        themeButtons->GetLayout().HeightMode( ESizing::Content ).FocusScope( true );
        for ( size i = 0; i < Size( m_Themes ); ++i )
            AddButton( themeButtons, m_Themes[i].Name, 130_u, [this, i]( ButtonBaseWidget& ) { ApplyTheme( i ); } );

        PanelWidget* columns = AddPanel( root, ELayoutType::Horizontal );
        columns->GetLayout().FocusScope( true );

        // Left column: sliders.
        PanelWidget* controls = AddPanel( columns, ELayoutType::Vertical );
        controls->GetLayout().FlexGrow( 1.f ).FocusScope( true );

        AddText( controls, "Controls", 20_u );
        AddSlider( controls, "Master Volume",   0.65f );
        AddSlider( controls, "Accent Strength", 0.30f );
        AddSlider( controls, "Vertical Mix",    0.45f, EOrient::Vertical );

        // Right column: text wrapping, typewriter reveal, overflow fade and a button.
        PanelWidget* preview = AddPanel( columns, ELayoutType::Vertical );
        preview->GetLayout().FlexGrow( 1.f ).FocusScope( true );

        AddText( preview, "Preview", 20_u );

        AddText( preview, "This panel uses the active theme for panel fills, text color, button states, and slider visuals.",
                 16_u, TextWrap::WrapWord() )->GetLayout().WidthMode( ESizing::Flex );

        m_Typewriter = AddText( preview,
            "My game dialogue uses the visible glyphs system. This only affects the rendering of the text, not the layout "
            "or shaped text data, which is useful for text revealed over time, such as in a dialogue system.",
            16_u, TextWrap::WrapWord() );
        m_Typewriter->GetLayout().FixedWidth( 280_u ).FlexHeight();

        AddText( preview, "This is an example of a long text string that will exceed its box and fade out instead of being cut off.",
                 16_u, TextWrap::WrapWord(), ETextOverflow::Fade )->GetLayout().FixedWidth( 100_u ).FixedHeight( 100_u );

        AddButton( preview, "Preview Button", 280_u, [this]( ButtonBaseWidget& )
        {
            m_Status->SetText( Text{ "Theme applied: " + m_Themes[m_ThemeIndex].Name + " (preview button clicked)" } );
        } );

        ApplyTheme( 0 );
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

    void ApplyTheme( size a_Index )
    {
        m_ThemeIndex   = a_Index;
        *m_ActiveTheme = *m_Themes[a_Index].Style;
        m_Status->SetText( Text{ "Theme applied: " + m_Themes[a_Index].Name + " (use the buttons below to switch styles)" } );
    }

    // -------------------------------------------------------------------------
    // Widget helpers
    // -------------------------------------------------------------------------

    PanelWidget* AddPanel( IWidget* a_Parent, ELayoutType a_Type )
    {
        PanelWidget* panel = m_Scene.CreateWidget<PanelWidget>( a_Parent->GetLayoutID() );
        panel->GetLayout()
            .LayoutType( a_Type )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Flex )
            .Padding( Edges::All( 12_u ) )
            .Spacing( 10_u );
        return panel;
    }

    TextWidget* AddText( IWidget* a_Parent, String a_String, Unit a_Size,
                         TextWrap a_Wrap = TextWrap::NoWrap(), ETextOverflow a_Overflow = ETextOverflow::Clip )
    {
        TextLayoutStyle style{};
        style.Font     = Fonts::Roboto;
        style.Size     = a_Size;
        style.Wrap     = a_Wrap;
        style.Overflow = a_Overflow;

        TextWidget* text = m_Scene.CreateWidget<TextWidget>( a_Parent->GetLayoutID(), Text{ std::move( a_String ) }, style );
        text->GetLayout().HeightMode( ESizing::Content );
        return text;
    }

    ButtonWidget* AddButton( IWidget* a_Parent, String a_Label, Unit a_Width, ButtonWidget::OnClickCallback a_OnClick )
    {
        ButtonWidget* button = m_Scene.CreateWidget<ButtonWidget>( a_Parent->GetLayoutID(), std::move( a_OnClick ) );
        button->GetLayout()
            .FixedWidth( a_Width )
            .FixedHeight( 38_u )
            .ChildAlign( EAlign::Center );

        // The label must not steal clicks from the button.
        AddText( button, std::move( a_Label ), 16_u )->GetLayout().Visibility( EVisibility::HitTestInvisible );
        return button;
    }

    /** @brief A card with a slider and a label showing its value. */
    void AddSlider( IWidget* a_Parent, String a_Label, f32 a_Value, EOrient a_Orientation = EOrient::Horizontal )
    {
        const bool vertical = a_Orientation == EOrient::Vertical;

        PanelWidget* card = AddPanel( a_Parent, ELayoutType::Vertical );
        card->GetLayout()
            .FixedHeight( vertical ? 180_u : 86_u )
            .Padding( Edges::All( 8_u ) )
            .Spacing( 6_u );

        TextWidget* label = AddText( card, "", 14_u );

        SliderWidget* slider = m_Scene.CreateWidget<SliderWidget>( card->GetLayoutID(), 0.f, 1.f, a_Value );
        slider->Orientation = a_Orientation;
        if ( vertical )
            slider->GetLayout().FixedWidth( 36_u ).FixedHeight( 120_u );
        else
            slider->GetLayout().WidthMode( ESizing::Flex ).FixedHeight( 28_u );

        slider->Value.Subscribe( [label, a_Label]( const f32& a_Current )
        {
            label->SetText( Text{ std::format( "{}: {}%", a_Label, static_cast<int>( std::round( a_Current * 100.f ) ) ) } );
        } );
    }

    // -------------------------------------------------------------------------
    // Themes
    // -------------------------------------------------------------------------

    static Shared<Theme> MakeDarkTheme()
    {
        Shared<Theme> theme = MakeShared<Theme>( *Themes::Dark() );
        theme->SetFont( ThemeKey::Font::Default, Fonts::Roboto );
        theme->SetColors( {
            { ThemeKey::Color::SliderThumbHover,   Colors::LightBlue  },
            { ThemeKey::Color::SliderThumbPressed, Colors::AccentBlue },
            { ThemeKey::Color::SliderTrackFill,    Colors::AccentBlue }
        } );
        theme->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = Colors::White } );
        return theme;
    }

    static Shared<Theme> MakeLightTheme()
    {
        Shared<Theme> theme = MakeShared<Theme>( *Themes::Dark() );
        theme->SetFont( ThemeKey::Font::Default, Fonts::Roboto );
        theme->SetColors( {
            { ThemeKey::Color::FocusOutline,       Colors::DarkBlue   },
            { ThemeKey::Color::SliderTrack,        Colors::Silver     },
            { ThemeKey::Color::SliderTrackFill,    Colors::DarkBlue   },
            { ThemeKey::Color::SliderThumb,        Colors::AccentBlue },
            { ThemeKey::Color::SliderThumbHover,   Colors::Blue       },
            { ThemeKey::Color::SliderThumbPressed, Colors::DarkBlue   }
        } );
        theme->SetBrushes( {
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
            { ThemeKey::Color::FocusOutline,       Colors::AccentRose                  },
            { ThemeKey::Color::SliderTrack,        FromColorF32( 0.10f, 0.10f, 0.20f ) },
            { ThemeKey::Color::SliderTrackFill,    Colors::AccentRose                  },
            { ThemeKey::Color::SliderThumb,        Colors::AccentSky                   },
            { ThemeKey::Color::SliderThumbHover,   Colors::LightCyan                   },
            { ThemeKey::Color::SliderThumbPressed, Colors::AccentRose                  }
        } );
        theme->SetRadii( {
            { ThemeKey::Radii::Panel,       CornerRadius::All( 12_u ) },
            { ThemeKey::Radii::Button,      CornerRadius::All( 10_u ) },
            { ThemeKey::Radii::SliderTrack, CornerRadius::All(  5_u ) },
            { ThemeKey::Radii::SliderThumb, CornerRadius::All(  8_u ) }
        } );
        theme->SetBrushes( {
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

        theme->SetColors( {
            { ThemeKey::Color::SliderTrackFill,    FromColorF32( 0.1f, 0.5f, 0.1f ) },
            { ThemeKey::Color::SliderThumb,        FromColorF32( 0.9f, 0.9f, 0.9f ) },
            { ThemeKey::Color::SliderThumbHover,   FromColorF32( 0.8f, 0.8f, 0.8f ) },
            { ThemeKey::Color::SliderThumbPressed, FromColorF32( 1.f,  1.f,  1.f  ) }
        } );
        return theme;
    }
};
