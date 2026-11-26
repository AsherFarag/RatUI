#pragma once
#include "Example.h"
#include <RatUI/Widget/ButtonWidget.h>
#include <RatUI/Widget/InputTextWidget.h>
#include <RatUI/Widget/PanelWidget.h>
#include <RatUI/Widget/ScrollContainerWidget.h>
#include <RatUI/Widget/SliderWidget.h>
#include <RatUI/Widget/TextWidget.h>
#include <cmath>
#include <format>
#include <functional>

/**
 * @brief A panel filled with a warning tint, whatever the theme. As ThemeShowcase's root it clears the safe area, so its
 * fill shows in the unsafe bands around the content when a safe area is set.
 */
class UnsafeAreaBackdrop : public PanelWidget
{
public:
    void OnPaint( const PaintEvent& a_Event ) override
    {
        a_Event.Drawer.AddBrushRect( GetLayout().Layout.FinalRect, SolidBrush{ FromColorF32( 1.f, 0.294f, 0.404f, 0.35f ) },
                                     Colors::Transparent, 0_u, CornerRadius::None() );
        PaintChildren( a_Event );
    }
};

/**
 * @brief A tour of the widgets, restyled live by switching between four themes.
 *
 * A header with the theme switcher, three cards (controls, text, a scroll list) and a status bar. The Safe Area button
 * simulates a phone's notch, rounded corners and home bar.
 */
class ThemeShowcase : public Example
{
public:
    explicit ThemeShowcase( ITextMetrics& a_TextMetrics )
        : Example( a_TextMetrics )
    {
        // Every widget shares m_Base, so copying another theme into it restyles the whole scene. The variants are
        // copies of it with a few keys changed, rebuilt on every theme switch.
        m_Scene.DefaultTheme = m_Base;

        UnsafeAreaBackdrop* screen = m_Scene.CreateRootWidget<UnsafeAreaBackdrop>();
        screen->GetLayout()
            .LayoutType( ELayoutType::Overlay )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Flex )
            .SafeAreaEdges( ESafeAreaEdges::All );

        PanelWidget* page = m_Scene.CreateWidget<PanelWidget>( screen->GetLayoutID() );
        page->Theme = m_Page;
        page->GetLayout()
            .LayoutType( ELayoutType::Vertical )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Flex )
            .Padding( Edges::All( 20_u ) )
            .Spacing( 16_u );

        BuildHeader( page );

        PanelWidget* body = AddBox( page, ELayoutType::Horizontal, 16_u );
        body->GetLayout().HeightMode( ESizing::Flex );
        BuildControlsCard( body );
        BuildTextCard( body );
        BuildScrollCard( body );

        BuildStatusBar( page );

        ApplyTheme( 0 );
        SetSafeAreaEnabled( false );
        SetStatus( "Ready. Click, drag, scroll or type anywhere." );
    }

    void Update( f32 a_DeltaSeconds ) override
    {
        // Reveal the dialogue a glyph at a time, restarting every 10 seconds.
        m_Time = std::fmod( m_Time + a_DeltaSeconds, 10.f );
        m_Typewriter->VisibleGlyphs = static_cast<u32>( 400.f * m_Time / 10.f );
    }

private:
    /** @brief A theme plus the showcase's own colors for it. */
    struct NamedTheme
    {
        String        Name;
        Shared<Theme> Style;
        Color         Page;   ///< Behind the cards, and inside inset wells.
        Color         Muted;  ///< Captions and secondary text.
        Color         Accent; ///< The selected theme button.
        Color         OnAccent;
    };

    struct ToggleButton
    {
        ButtonWidget* Button{ nullptr };
        TextWidget*   Label{ nullptr };
    };

    Array<NamedTheme> m_Themes{ MakeDarkTheme(), MakeLightTheme(), MakeNeonTheme(), MakeMinecraftTheme() };

    Shared<Theme> m_Base  { MakeShared<Theme>( *m_Themes[0].Style ) };
    Shared<Theme> m_Page  { MakeShared<Theme>() }; ///< Panels drawn in the page color.
    Shared<Theme> m_Muted { MakeShared<Theme>() }; ///< Text drawn in the muted color.
    Shared<Theme> m_Accent{ MakeShared<Theme>() }; ///< Buttons and labels in the accent color.

    Array<ToggleButton> m_ThemeButtons;
    ToggleButton        m_SafeAreaButton;
    size                m_ThemeIndex{ 0 };
    bool                m_SafeAreaEnabled{ false };
    int                 m_Clicks{ 0 };

    TextWidget* m_Status{ nullptr };
    TextWidget* m_Typewriter{ nullptr };
    f32         m_Time{ 0.f };

    // -------------------------------------------------------------------------
    // Sections
    // -------------------------------------------------------------------------

    void BuildHeader( IWidget* a_Parent )
    {
        PanelWidget* header = AddBox( a_Parent, ELayoutType::Horizontal, 8_u );
        header->GetLayout().ChildAlign( EAlign::CenterLeft );

        PanelWidget* titles = AddBox( header, ELayoutType::Vertical, 2_u );
        AddText( titles, "RatUI Showcase", 28_u );
        AddText( titles, "Every widget below restyles live when you switch themes.", 14_u, true );

        for ( size i = 0; i < Size( m_Themes ); ++i )
            PushBack( m_ThemeButtons, AddToggle( header, m_Themes[i].Name, 110_u, [this, i] { ApplyTheme( i ); } ) );

        m_SafeAreaButton = AddToggle( header, "Safe Area", 120_u, [this] { SetSafeAreaEnabled( !m_SafeAreaEnabled ); } );
    }

    void BuildControlsCard( IWidget* a_Parent )
    {
        PanelWidget* card = AddCard( a_Parent, "Controls", "Drag them, or press Enter on one to adjust it with the arrows." );

        AddSlider( card, "Master Volume",   0.65f );
        AddSlider( card, "Accent Strength", 0.30f );

        PanelWidget* mixer = AddBox( card, ELayoutType::Horizontal, 10_u );
        AddSlider( mixer, "Bass",   0.45f, EOrient::Vertical );
        AddSlider( mixer, "Mid",    0.70f, EOrient::Vertical );
        AddSlider( mixer, "Treble", 0.25f, EOrient::Vertical );

        PanelWidget* buttons = AddBox( card, ELayoutType::Horizontal, 10_u );
        TextWidget*   counter = nullptr;
        ButtonWidget* click   = AddButton( buttons, "Click me", {}, &counter );
        click->OnClick = [this, counter]( ButtonBaseWidget& )
        {
            ++m_Clicks;
            counter->SetText( Text{ std::format( "Clicked {}x", m_Clicks ) } );
            SetStatus( std::format( "Button clicked {} time{}", m_Clicks, m_Clicks == 1 ? "" : "s" ) );
        };
        AddButton( buttons, "Reset", [this, counter]( ButtonBaseWidget& )
        {
            m_Clicks = 0;
            counter->SetText( Text{ "Click me" } );
            SetStatus( "Counter reset" );
        } );
    }

    void BuildTextCard( IWidget* a_Parent )
    {
        PanelWidget* card = AddCard( a_Parent, "Text", "Wrapping, reveal, overflow and editing." );

        AddText( card, "Word wrap", 13_u, true );
        AddText( card, "Text wraps at word boundaries to fit its box, and reflows as the window is resized.", 16_u, false,
                 TextWrap::WrapWord() )->GetLayout().WidthMode( ESizing::Flex );

        AddText( card, "Typewriter reveal", 13_u, true );
        m_Typewriter = AddText( card,
            "Dialogue can be revealed a glyph at a time. Only the drawing changes; the layout and shaped text stay the "
            "same, so nothing jumps around while it types.",
            16_u, false, TextWrap::WrapWord() );
        m_Typewriter->GetLayout().WidthMode( ESizing::Flex );

        AddText( card, "Overflow fade", 13_u, true );
        AddText( card, "This line is far too long for its box, so instead of being cut off mid-glyph it fades out softly "
                       "at the edge, which reads much better than a hard clip.",
                 16_u, false, TextWrap::NoWrap(), ETextOverflow::Fade )->GetLayout().WidthMode( ESizing::Flex );

        AddText( card, "Text input", 13_u, true );
        InputTextWidget* name = AddInput( card, MakeUnique<SingleLineTextEditPolicy>() );
        name->SetPlaceholder( "Type here and press Enter" );
        name->OnSubmit = [this]( const String& a_Text ) { SetStatus( "Submitted: " + a_Text ); };

        AddInput( card, MakeUnique<MultiLineTextEditPolicy>() )->SetText( "Multi-line field.\nEnter adds a line." );
    }

    void BuildScrollCard( IWidget* a_Parent )
    {
        PanelWidget* card = AddCard( a_Parent, "Scroll List", "Wheel, drag the bars, or Tab through the rows." );

        PanelWidget* well = AddPanel( card, ELayoutType::Vertical, 0_u, Edges::All( 6_u ) );
        well->Theme = m_Page;
        well->GetLayout().HeightMode( ESizing::Flex );

        ScrollContainerWidget* scroll = m_Scene.CreateWidget<ScrollContainerWidget>( well->GetLayoutID() );
        scroll->OnScroll = [this]( ScrollContainerWidget&, Vec2<Unit> a_Offset )
        {
            SetStatus( std::format( "Scrolled to {}, {}", static_cast<int>( a_Offset[0].ToFloat() ), static_cast<int>( a_Offset[1].ToFloat() ) ) );
        };

        // Room around the rows for their focus rings, which the list clips; more on the right to clear the scrollbar.
        PanelWidget* rows = m_Scene.CreateWidget<PanelWidget>( scroll->GetContentNodeID() );
        MakeInvisible( *rows );
        rows->GetLayout()
            .LayoutType( ELayoutType::Vertical )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Content )
            .Padding( Edges{ .T = 4_u, .R = 10_u, .B = 4_u, .L = 4_u } )
            .Spacing( 6_u );

        static constexpr const char* c_Items[] = { "Rusty Sword", "Cheese Wheel", "Lantern", "Rope", "Map Fragment",
                                                   "Iron Key", "Healing Herb", "Old Boot", "Silver Ring", "Torch",
                                                   "Lockpick", "Bread Crust", "Compass", "Feather", "Bone Dice",
                                                   "Copper Coin", "Spyglass", "Candle", "Whistle", "Crown" };
        for ( size i = 0; i < std::size( c_Items ); ++i )
        {
            AddListRow( rows, c_Items[i], std::format( "x{}", ( i * 7 ) % 12 + 1 ) );
            if ( i == 4 )
                AddSlider( rows, "Click me, then scroll", 0.5f );
        }

        ButtonWidget* wide = AddListRow( rows, "A wide row that scrolls the list sideways", "->" );
        wide->GetLayout().FixedWidth( 640_u );
    }

    void BuildStatusBar( IWidget* a_Parent )
    {
        PanelWidget* bar = AddPanel( a_Parent, ELayoutType::Horizontal, 12_u, Edges::Axis( 14_u, 10_u ) );
        bar->GetLayout().ChildAlign( EAlign::CenterLeft );

        m_Status = AddText( bar, "", 14_u );
        m_Status->GetLayout().WidthMode( ESizing::Flex );
        AddText( bar, "Tab / arrows: move focus  |  Enter: press  |  F1: layout bounds", 13_u, true );
    }

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    void SetStatus( String a_Text ) { m_Status->SetText( Text{ std::move( a_Text ) } ); }

    /** @brief Toggles a phone-like safe area: a notch on top, a home bar at the bottom and rounded corners at the sides. */
    void SetSafeAreaEnabled( bool a_Enabled )
    {
        m_SafeAreaEnabled = a_Enabled;
        m_Scene.SetSafeArea( a_Enabled ? Edges{ .T = 44_u, .R = 24_u, .B = 34_u, .L = 24_u } : Edges{} );
        SetSelected( m_SafeAreaButton, a_Enabled );
        SetStatus( a_Enabled ? "Safe area on: the tinted bands are the unsafe screen edges" : "Safe area off" );
    }

    void ApplyTheme( size a_Index )
    {
        const NamedTheme& theme = m_Themes[a_Index];
        m_ThemeIndex = a_Index;
        *m_Base      = *theme.Style;

        *m_Page = *theme.Style;
        m_Page->SetBrush( ThemeKey::Brush::PanelNormal, SolidBrush{ theme.Page } );

        *m_Muted = *theme.Style;
        m_Muted->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = theme.Muted } );

        *m_Accent = *theme.Style;
        m_Accent->SetBrushes( {
            { ThemeKey::Brush::ButtonNormal,  SolidBrush{ theme.Accent } },
            { ThemeKey::Brush::ButtonHover,   SolidBrush{ Lerp( theme.Accent, theme.OnAccent, 0.15f ) } },
            { ThemeKey::Brush::ButtonPressed, SolidBrush{ Lerp( theme.Accent, Colors::Black, 0.25f ) } }
        } );
        m_Accent->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = theme.OnAccent } );

        for ( size i = 0; i < Size( m_ThemeButtons ); ++i )
            SetSelected( m_ThemeButtons[i], i == a_Index );
        SetSelected( m_SafeAreaButton, m_SafeAreaEnabled );

        SetStatus( "Theme: " + theme.Name );
    }

    void SetSelected( ToggleButton& a_Toggle, bool a_Selected )
    {
        const Shared<Theme>& theme = a_Selected ? m_Accent : m_Base;
        a_Toggle.Button->Theme = theme;
        a_Toggle.Label->Theme  = theme;
    }

    static Color Lerp( Color a_From, Color a_To, f32 a_T )
    {
        const auto mix = [&]( int i ) { return ( static_cast<f32>( a_From[i] ) * ( 1.f - a_T ) + static_cast<f32>( a_To[i] ) * a_T ) / 255.f; };
        return FromColorF32( mix( 0 ), mix( 1 ), mix( 2 ), mix( 3 ) );
    }

    // -------------------------------------------------------------------------
    // Widget helpers
    // -------------------------------------------------------------------------

    /** @brief A panel with the theme's fill. */
    PanelWidget* AddPanel( IWidget* a_Parent, ELayoutType a_Type, Unit a_Spacing, Edges a_Padding )
    {
        PanelWidget* panel = m_Scene.CreateWidget<PanelWidget>( a_Parent->GetLayoutID() );
        panel->GetLayout()
            .LayoutType( a_Type )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Content )
            .Padding( a_Padding )
            .Spacing( a_Spacing );
        return panel;
    }

    /** @brief An invisible panel, only for layout. */
    PanelWidget* AddBox( IWidget* a_Parent, ELayoutType a_Type, Unit a_Spacing )
    {
        PanelWidget* box = AddPanel( a_Parent, a_Type, a_Spacing, Edges{} );
        MakeInvisible( *box );
        return box;
    }

    static void MakeInvisible( PanelWidget& a_Panel )
    {
        a_Panel.Theme.Reset();
        a_Panel.FillBrush       = SolidBrush{ Colors::Transparent };
        a_Panel.BorderThickness = 0_u;
    }

    /** @brief A titled card that fills its share of the row. */
    PanelWidget* AddCard( IWidget* a_Parent, String a_Title, String a_Caption )
    {
        PanelWidget* card = AddPanel( a_Parent, ELayoutType::Vertical, 10_u, Edges::All( 16_u ) );
        card->GetLayout().HeightMode( ESizing::Flex ).FlexGrow( 1.f );

        AddText( card, std::move( a_Title ), 20_u );
        AddText( card, std::move( a_Caption ), 13_u, true );
        return card;
    }

    TextWidget* AddText( IWidget* a_Parent, String a_String, Unit a_Size, bool a_Muted = false,
                         TextWrap a_Wrap = TextWrap::NoWrap(), ETextOverflow a_Overflow = ETextOverflow::Clip )
    {
        TextLayoutStyle style{};
        style.Font     = Fonts::Roboto;
        style.Size     = a_Size;
        style.Wrap     = a_Wrap;
        style.Overflow = a_Overflow;

        TextWidget* text = m_Scene.CreateWidget<TextWidget>( a_Parent->GetLayoutID(), Text{ std::move( a_String ) }, style );
        if ( a_Muted )
            text->Theme = m_Muted;
        text->GetLayout().HeightMode( ESizing::Content );
        return text;
    }

    InputTextWidget* AddInput( IWidget* a_Parent, Unique<ITextEditPolicy> a_Policy )
    {
        InputTextWidget* input = m_Scene.CreateWidget<InputTextWidget>( a_Parent->GetLayoutID(), std::move( a_Policy ) );
        input->GetLayout()
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Content )
            .Padding( Edges::All( 8_u ) );
        return input;
    }

    ButtonWidget* AddButton( IWidget* a_Parent, String a_Label, ButtonWidget::OnClickCallback a_OnClick,
                             TextWidget** a_OutLabel = nullptr )
    {
        ButtonWidget* button = m_Scene.CreateWidget<ButtonWidget>( a_Parent->GetLayoutID(), std::move( a_OnClick ) );
        button->GetLayout()
            .WidthMode( ESizing::Flex )
            .FixedHeight( 36_u )
            .ChildAlign( EAlign::Center );

        // The label must not steal clicks from the button.
        TextWidget* label = AddText( button, std::move( a_Label ), 15_u );
        label->GetLayout().Visibility( EVisibility::HitTestInvisible );
        if ( a_OutLabel )
            *a_OutLabel = label;
        return button;
    }

    /** @brief A header button that shows the accent color while selected. */
    ToggleButton AddToggle( IWidget* a_Parent, String a_Label, Unit a_Width, std::function<void()> a_OnClick )
    {
        ToggleButton toggle;
        toggle.Button = AddButton( a_Parent, std::move( a_Label ), [a_OnClick]( ButtonBaseWidget& ) { a_OnClick(); }, &toggle.Label );
        toggle.Button->GetLayout().FixedWidth( a_Width );
        return toggle;
    }

    /** @brief A list row: a name on the left and a muted detail on the right. */
    ButtonWidget* AddListRow( IWidget* a_Parent, String a_Name, String a_Detail )
    {
        ButtonWidget* row = m_Scene.CreateWidget<ButtonWidget>( a_Parent->GetLayoutID(), [this, a_Name]( ButtonBaseWidget& )
        {
            SetStatus( "Picked " + a_Name );
        } );
        row->GetLayout()
            .LayoutType( ELayoutType::Horizontal )
            .WidthMode( ESizing::Flex )
            .FixedHeight( 34_u )
            .Padding( Edges::Axis( 12_u, 0_u ) )
            .ChildAlign( EAlign::CenterLeft );

        TextWidget* name = AddText( row, std::move( a_Name ), 15_u );
        name->GetLayout().WidthMode( ESizing::Flex ).Visibility( EVisibility::HitTestInvisible );
        AddText( row, std::move( a_Detail ), 13_u, true )->GetLayout().Visibility( EVisibility::HitTestInvisible );
        return row;
    }

    /** @brief An inset card with a slider, its name and its value. */
    void AddSlider( IWidget* a_Parent, String a_Label, f32 a_Value, EOrient a_Orientation = EOrient::Horizontal )
    {
        const bool vertical = a_Orientation == EOrient::Vertical;

        PanelWidget* card = AddPanel( a_Parent, ELayoutType::Vertical, 8_u, Edges::All( 10_u ) );
        card->Theme = m_Page;
        if ( vertical )
            card->GetLayout().ChildAlign( EAlign::TopCenter );

        PanelWidget* header = AddBox( card, ELayoutType::Horizontal, 6_u );
        AddText( header, a_Label, 14_u )->GetLayout().WidthMode( ESizing::Flex );
        TextWidget* value = AddText( header, "", 14_u, true );

        SliderWidget* slider = m_Scene.CreateWidget<SliderWidget>( card->GetLayoutID(), 0.f, 1.f, a_Value );
        slider->Orientation = a_Orientation;
        slider->Step        = 0.05f; // Snaps to 5%; the arrow keys move one step.
        if ( vertical )
            slider->GetLayout().FixedWidth( 32_u ).FixedHeight( 110_u );
        else
            slider->GetLayout().WidthMode( ESizing::Flex ).FixedHeight( 26_u );

        slider->Value.Subscribe( [this, value, a_Label]( const f32& a_Current )
        {
            const int percent = static_cast<int>( std::round( a_Current * 100.f ) );
            value->SetText( Text{ std::format( "{}%", percent ) } );
            if ( m_Status )
                SetStatus( std::format( "{}: {}%", a_Label, percent ) );
        } );
    }

    // -------------------------------------------------------------------------
    // Themes
    // -------------------------------------------------------------------------

    static NamedTheme MakeDarkTheme()
    {
        Shared<Theme> theme = MakeShared<Theme>( *Themes::Dark() );
        theme->SetFont( ThemeKey::Font::Default, Fonts::Roboto );
        theme->SetColors( { { ThemeKey::Color::PanelBorder, Colors::Surface600 } } );
        theme->SetBrushes( {
            { ThemeKey::Brush::PanelNormal,        SolidBrush{ Colors::Surface800 } },
            { ThemeKey::Brush::ButtonNormal,       SolidBrush{ Colors::Surface600 } },
            { ThemeKey::Brush::ButtonHover,        SolidBrush{ Colors::Surface500 } },
            { ThemeKey::Brush::ButtonPressed,      SolidBrush{ Colors::Surface700 } },
            { ThemeKey::Brush::SliderThumbHover,   SolidBrush{ Colors::LightBlue  } },
            { ThemeKey::Brush::SliderThumbPressed, SolidBrush{ Colors::AccentBlue } },
            { ThemeKey::Brush::SliderTrackFill,    SolidBrush{ Colors::AccentBlue } }
        } );
        theme->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = Colors::White } );
        return { "Dark", theme, Colors::Surface900, FromColorF32( 0.60f, 0.61f, 0.68f ), Colors::AccentBlue, Colors::White };
    }

    static NamedTheme MakeLightTheme()
    {
        Shared<Theme> theme = MakeShared<Theme>( *Themes::Dark() );
        theme->SetFont( ThemeKey::Font::Default, Fonts::Roboto );
        theme->SetColors( {
            { ThemeKey::Color::FocusOutline, Colors::DarkBlue },
            { ThemeKey::Color::PanelBorder,  FromColorF32( 0.82f, 0.83f, 0.87f ) }
        } );
        theme->SetBrushes( {
            { ThemeKey::Brush::SliderTrack,        SolidBrush{ FromColorF32( 0.80f, 0.82f, 0.87f ) } },
            { ThemeKey::Brush::SliderTrackFill,    SolidBrush{ Colors::AccentBlue } },
            { ThemeKey::Brush::SliderThumb,        SolidBrush{ Colors::AccentBlue } },
            { ThemeKey::Brush::SliderThumbHover,   SolidBrush{ Colors::Blue       } },
            { ThemeKey::Brush::SliderThumbPressed, SolidBrush{ Colors::DarkBlue   } },
            { ThemeKey::Brush::PanelNormal,   SolidBrush{ FromColorF32( 0.99f, 0.99f, 1.00f ) } },
            { ThemeKey::Brush::ButtonNormal,  SolidBrush{ FromColorF32( 0.91f, 0.92f, 0.95f ) } },
            { ThemeKey::Brush::ButtonHover,   SolidBrush{ FromColorF32( 0.84f, 0.88f, 0.96f ) } },
            { ThemeKey::Brush::ButtonPressed, SolidBrush{ FromColorF32( 0.76f, 0.82f, 0.94f ) } }
        } );
        theme->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = Colors::Surface900 } );
        return { "Light", theme, FromColorF32( 0.93f, 0.94f, 0.96f ), FromColorF32( 0.40f, 0.42f, 0.48f ), Colors::AccentBlue, Colors::White };
    }

    static NamedTheme MakeNeonTheme()
    {
        Shared<Theme> theme = MakeShared<Theme>( *Themes::Dark() );
        theme->SetFont( ThemeKey::Font::Default, Fonts::Roboto );
        theme->SetColors( {
            { ThemeKey::Color::FocusOutline, Colors::AccentRose },
            { ThemeKey::Color::PanelBorder,  FromColorF32( 0.30f, 0.10f, 0.40f ) }
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
            { ThemeKey::Brush::PanelNormal,   SolidBrush{ FromColorF32( 0.08f, 0.04f, 0.12f ) } },
            { ThemeKey::Brush::ButtonNormal,  SolidBrush{ FromColorF32( 0.20f, 0.05f, 0.28f ) } },
            { ThemeKey::Brush::ButtonHover,   SolidBrush{ FromColorF32( 0.30f, 0.08f, 0.45f ) } },
            { ThemeKey::Brush::ButtonPressed, SolidBrush{ FromColorF32( 0.12f, 0.45f, 0.42f ) } }
        } );
        theme->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = Colors::AccentSky } );
        return { "Neon", theme, FromColorF32( 0.03f, 0.01f, 0.05f ), FromColorF32( 0.62f, 0.45f, 0.80f ), Colors::AccentRose,
                 FromColorF32( 0.06f, 0.02f, 0.08f ) };
    }

    static NamedTheme MakeMinecraftTheme()
    {
        NamedTheme named = MakeDarkTheme();
        named.Name   = "Minecraft";
        named.Page   = FromColorF32( 0.07f, 0.07f, 0.07f );
        named.Accent = FromColorF32( 0.20f, 0.55f, 0.20f );

        Shared<Theme>& theme = named.Style;
        theme->SetFont( ThemeKey::Font::Default, Fonts::Minecraft );
        for ( const auto& [key, value] : theme->GetRadii() )
            theme->SetRadius( key, CornerRadius::None() ); // Sharp corners everywhere.

        theme->SetBrushes( {
            { ThemeKey::Brush::PanelNormal,        SolidBrush{ FromColorF32( 0.20f, 0.20f, 0.20f ) } },
            { ThemeKey::Brush::ButtonNormal,       SolidBrush{ FromColorF32( 0.42f, 0.42f, 0.42f ) } },
            { ThemeKey::Brush::ButtonHover,        SolidBrush{ FromColorF32( 0.50f, 0.52f, 0.62f ) } },
            { ThemeKey::Brush::ButtonPressed,      SolidBrush{ FromColorF32( 0.30f, 0.30f, 0.30f ) } },
            { ThemeKey::Brush::SliderTrackFill,    SolidBrush{ FromColorF32( 0.1f, 0.5f, 0.1f ) } },
            { ThemeKey::Brush::SliderThumb,        SolidBrush{ FromColorF32( 0.9f, 0.9f, 0.9f ) } },
            { ThemeKey::Brush::SliderThumbHover,   SolidBrush{ FromColorF32( 0.8f, 0.8f, 0.8f ) } },
            { ThemeKey::Brush::SliderThumbPressed, SolidBrush{ FromColorF32( 1.f,  1.f,  1.f  ) } }
        } );
        theme->SetColors( { { ThemeKey::Color::PanelBorder, Colors::Black } } );
        return named;
    }
};
