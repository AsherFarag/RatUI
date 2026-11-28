#pragma once
#include "Example.h"
#include <RatUI/Widget/WidgetBuilder.h>
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
    using Super = PanelWidget;

    void OnPaint( const PaintEvent& a_Event ) override
    {
        a_Event.Drawer.AddBrushRect( GetLayout().Layout.SceneRect, SolidBrush{ FromColorF32( 1.f, 0.294f, 0.404f, 0.35f ) },
                                     Colors::Transparent, 0_u, CornerRadius::None() );
        PaintChildren( a_Event );
    }
};

/** @brief A panel that cuts off what its children draw outside it, for content that can outgrow a card in a small window. */
class ClippedPanel : public PanelWidget
{
public:
    using Super = PanelWidget;

    void OnPaint( const PaintEvent& a_Event ) override
    {
        a_Event.Drawer.PushClipRect( GetLayout().Layout.SceneRect.Expanded( 1_u ) ); // Room for its own border.
        PanelWidget::OnPaint( a_Event );
        a_Event.Drawer.PopClipRect();
    }
};

/**
 * @brief The showcase's own factories, composed with the standard ones (see ShowcaseSceneBuilder below).
 * This is how you add your own ui.Something() calls: a mixin templated on the final builder type B.
 */
template<typename B>
struct ShowcaseFactories
{
    /** @brief A panel with the theme's fill that fills its parent's width. With a_Clip, its children are clipped to it. */
    WidgetBuilder<PanelWidget> Box( bool a_Clip = false ) const
    {
        WidgetBuilder<PanelWidget> panel = a_Clip ? WidgetBuilder<PanelWidget>( MakeWidget<ClippedPanel>( Ui() ) ) : Ui().Panel();
        return panel.FlexWidth().ContentHeight();
    }

    NodeBuilder Row( Unit a_Spacing ) const    { return Ui().HBox().FlexWidth().ContentHeight().Spacing( a_Spacing ); }
    NodeBuilder Column( Unit a_Spacing ) const { return Ui().VBox().FlexWidth().ContentHeight().Spacing( a_Spacing ); }

    WidgetBuilder<TextWidget> SizedText( String a_String, Unit a_Size, TextWrap a_Wrap = TextWrap::NoWrap(),
                                    ETextOverflow a_Overflow = ETextOverflow::Clip ) const
    {
        return Ui().Text( std::move( a_String ) ).Font( Fonts::Roboto ).Size( a_Size ).Wrap( a_Wrap ).Overflow( a_Overflow ).ContentHeight();
    }

    /** @brief Text as wide as its parent, so one too long for it wraps there instead of running past it. */
    WidgetBuilder<TextWidget> FitText( String a_String, Unit a_Size, TextWrap a_Wrap = TextWrap::WrapWord(),
                                       ETextOverflow a_Overflow = ETextOverflow::Clip ) const
    {
        return SizedText( std::move( a_String ), a_Size, a_Wrap, a_Overflow ).FlexWidth();
    }

    /** @brief One line of text as wide as its parent, ending in "..." when it is too long for it. */
    WidgetBuilder<TextWidget> Label( String a_String, Unit a_Size ) const
    {
        return FitText( std::move( a_String ), a_Size, TextWrap::NoWrap(), ETextOverflow::Ellipsis );
    }

    WidgetBuilder<InputTextWidget> Field( WidgetBuilder<InputTextWidget> a_Input ) const
    {
        return a_Input.FlexWidth().ContentHeight().Padding( 8_u );
    }

private:
    const B& Ui() const { return static_cast<const B&>( *this ); }
};

using ShowcaseSceneBuilder = BasicSceneBuilder<StandardFactories, ShowcaseFactories>;

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

        const ShowcaseSceneBuilder& ui = m_Ui;

        // The backdrop keeps the whole window, so its tint fills the unsafe bands; the page is laid out clear of them.
        ui.Root( ui.MakeWidget<UnsafeAreaBackdrop>().Flex().SafeAreaEdges( ESafeAreaEdges::All )(
            ui.Panel().Theme( m_Screen ).Vertical().Flex().Padding( 20_u ).Spacing( 16_u )(
                Header(),
                ui.Row( 16_u ).FlexHeight()( ControlsCard(), TextCard(), ScrollCard() ),
                StatusBar()
            )
        ) );

        ApplyTheme( 0 );
        SetSafeAreaEnabled( false );
        SetStatus( "Ready. Click, drag, scroll or type anywhere." );
    }

    void Update( f32 a_DeltaSeconds ) override
    {
        // Reveal the dialogue a glyph at a time, restarting every 10 seconds.
        m_Time = std::fmod( m_Time + a_DeltaSeconds, 10.f );
        m_Typewriter->VisibleGlyphs = static_cast<u32>( 400.f * m_Time / 10.f );

        // The key hint shows only while the status bar has room for it beside the status.
        const Unit barWidth = m_KeyHint->GetParentWidget()->GetLayout().Layout.SceneRect.Width();
        m_KeyHint->GetLayout().Visibility( barWidth >= 700_u ? EVisibility::Visible : EVisibility::Collapsed );
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
    Shared<Theme> m_Screen{ MakeShared<Theme>() }; ///< The page itself: square and borderless, so it fills the window to its corners.
    Shared<Theme> m_Muted { MakeShared<Theme>() }; ///< Text drawn in the muted color.
    Shared<Theme> m_Hint  { MakeShared<Theme>() }; ///< Muted text aligned to the right of its box.
    Shared<Theme> m_Accent{ MakeShared<Theme>() }; ///< Buttons and labels in the accent color.
    Shared<Theme> m_Label      { MakeShared<Theme>() }; ///< m_Base with centred text, for a label that fills its button.
    Shared<Theme> m_AccentLabel{ MakeShared<Theme>() }; ///< m_Accent with centred text.

    ShowcaseSceneBuilder m_Ui{ m_Scene };

    Array<ToggleButton> m_ThemeButtons;
    ToggleButton        m_SafeAreaButton;
    size                m_ThemeIndex{ 0 };
    bool                m_SafeAreaEnabled{ false };
    int                 m_Clicks{ 0 };

    TextWidget* m_Status{ nullptr };
    TextWidget* m_KeyHint{ nullptr };
    TextWidget* m_Typewriter{ nullptr };
    f32         m_Time{ 0.f };

    // -------------------------------------------------------------------------
    // Sections
    // -------------------------------------------------------------------------

    NodeBuilder Header()
    {
        const ShowcaseSceneBuilder& ui = m_Ui;

        // In their own row, so the title keeps the full width and the buttons share theirs in a narrow window.
        NodeBuilder buttons = ui.Row( 8_u );
        for ( size i = 0; i < Size( m_Themes ); ++i )
        {
            PushBack( m_ThemeButtons, ToggleButton{} );
            buttons( Toggle( m_Themes[i].Name, 110_u, [this, i] { ApplyTheme( i ); }, Back( m_ThemeButtons ) ) );
        }
        buttons( Toggle( "Safe Area", 120_u, [this] { SetSafeAreaEnabled( !m_SafeAreaEnabled ); }, m_SafeAreaButton ) );

        return ui.Column( 10_u )(
            ui.Column( 2_u )(
                ui.FitText( "RatUI Showcase", 28_u ),
                Muted( ui.FitText( "Every widget below restyles live when you switch themes.", 14_u ) )
            ),
            buttons
        );
    }

    WidgetBuilder<PanelWidget> ControlsCard()
    {
        const ShowcaseSceneBuilder& ui = m_Ui;

        TextWidget* counter = nullptr;
        WidgetBuilder<ButtonWidget> click = Button( "Click me", {}, &counter ).OnClick( [this, counter]( ButtonBaseWidget& )
        {
            ++m_Clicks;
            counter->SetText( Text{ std::format( "Clicked {}x", m_Clicks ) } );
            SetStatus( std::format( "Button clicked {} time{}", m_Clicks, m_Clicks == 1 ? "" : "s" ) );
        } );
        WidgetBuilder<ButtonWidget> reset = Button( "Reset", [this, counter]( ButtonBaseWidget& )
        {
            m_Clicks = 0;
            counter->SetText( Text{ "Click me" } );
            SetStatus( "Counter reset" );
        } );

        return Card( "Controls", "Drag them, or press Enter on one to adjust it with the arrows." )(
            LabeledSlider( "Master Volume",   0.65f ),
            LabeledSlider( "Accent Strength", 0.30f ),
            ui.Row( 10_u )(
                LabeledSlider( "Bass",   0.45f, EOrient::Vertical ),
                LabeledSlider( "Mid",    0.70f, EOrient::Vertical ),
                LabeledSlider( "Treble", 0.25f, EOrient::Vertical )
            ),
            ui.Row( 10_u )( click, reset )
        );
    }

    WidgetBuilder<PanelWidget> TextCard()
    {
        const ShowcaseSceneBuilder& ui = m_Ui;

        return Card( "Text", "Wrapping, reveal, overflow and editing." )(
            Muted( ui.FitText( "Word wrap", 13_u ) ),
            ui.FitText( "Text wraps at word boundaries to fit its box, and reflows as the window is resized.", 16_u ),

            Muted( ui.FitText( "Typewriter reveal", 13_u ) ),
            ui.FitText( "Dialogue can be revealed a glyph at a time. Only the drawing changes; the layout and shaped text stay "
                        "the same, so nothing jumps around while it types.", 16_u ).Bind( m_Typewriter ),

            Muted( ui.FitText( "Overflow fade", 13_u ) ),
            ui.FitText( "This line is far too long for its box, so instead of being cut off mid-glyph it fades out softly "
                        "at the edge, which reads much better than a hard clip.", 16_u, TextWrap::NoWrap(), ETextOverflow::Fade ),

            Muted( ui.FitText( "Text input", 13_u ) ),
            ui.Field( ui.InputText() )
                .Placeholder( "Type here and press Enter" )
                .OnSubmit( [this]( const String& a_Text ) { SetStatus( "Submitted: " + a_Text ); } ),
            ui.Field( ui.TextArea() ).Text( "Multi-line field.\nEnter adds a line." )
        );
    }

    WidgetBuilder<PanelWidget> ScrollCard()
    {
        const ShowcaseSceneBuilder& ui = m_Ui;

        // Room around the rows for their focus rings, which the list clips; more on the right to clear the scrollbar.
        NodeBuilder rows = ui.Column( 6_u ).Padding( Edges{ .T = 4_u, .R = 10_u, .B = 4_u, .L = 4_u } );

        static constexpr const char* c_Items[] = { "Rusty Sword", "Cheese Wheel", "Lantern", "Rope", "Map Fragment",
                                                   "Iron Key", "Healing Herb", "Old Boot", "Silver Ring", "Torch",
                                                   "Lockpick", "Bread Crust", "Compass", "Feather", "Bone Dice",
                                                   "Copper Coin", "Spyglass", "Candle", "Whistle", "Crown" };
        for ( size i = 0; i < std::size( c_Items ); ++i )
        {
            rows( ListRow( c_Items[i], std::format( "x{}", ( i * 7 ) % 12 + 1 ) ) );
            if ( i == 4 )
                rows( LabeledSlider( "Click me, then scroll", 0.5f ) );
        }
        rows( ListRow( "A wide row that scrolls the list sideways", "->" ).FixedWidth( 640_u ) );

        return Card( "Scroll List", "Wheel, drag the bars, or Tab through the rows." )(
            ui.Box().Theme( m_Page ).Vertical().FlexHeight().Padding( 6_u )(
                ui.ScrollBox().With( [this]( ScrollContainerWidget& a_Scroll )
                {
                    a_Scroll.OnScroll = [this]( ScrollContainerWidget&, const ScrollEvent& a_Event )
                    {
                        // Also called when only the range changes (e.g. the list is resized); the status is about where it scrolled to.
                        if ( a_Event.OffsetDelta[0] == 0_u && a_Event.OffsetDelta[1] == 0_u )
                            return;

                        SetStatus( std::format( "Scrolled to {}, {}", static_cast<int>( a_Event.Offset[0].ToFloat() ),
                                                static_cast<int>( a_Event.Offset[1].ToFloat() ) ) );
                    };
                    a_Scroll.ScrollPadding = Edges::All( 4_u ); // A focused row keeps clear of the list's edges, which clip its focus ring.
                } )( rows )
            )
        );
    }

    WidgetBuilder<PanelWidget> StatusBar()
    {
        const ShowcaseSceneBuilder& ui = m_Ui;

        return ui.Box().Horizontal().Padding( Edges::Axis( 14_u, 10_u ) ).Spacing( 12_u ).ChildAlign( EAlign::CenterLeft )(
            ui.Label( "", 14_u ).Bind( m_Status ),
            ui.Label( "Tab / arrows: move focus  |  Enter: press  |  F1: layout bounds", 13_u ).Theme( m_Hint ).Bind( m_KeyHint )
        );
    }

    // -------------------------------------------------------------------------
    // Themed pieces
    // -------------------------------------------------------------------------

    template<typename W>
    WidgetBuilder<W> Muted( WidgetBuilder<W> a_Builder ) const { return a_Builder.Theme( m_Muted ); }

    /** @brief A titled card that fills its share of the row. */
    WidgetBuilder<PanelWidget> Card( String a_Title, String a_Caption )
    {
        const ShowcaseSceneBuilder& ui = m_Ui;
        return ui.Box( true ).Vertical().FlexHeight().Grow( 1.f ).Padding( 16_u ).Spacing( 10_u )(
            ui.FitText( std::move( a_Title ), 20_u ),
            Muted( ui.FitText( std::move( a_Caption ), 13_u ) )
        );
    }

    WidgetBuilder<ButtonWidget> Button( String a_Label, ButtonWidget::OnClickCallback a_OnClick, TextWidget** a_OutLabel = nullptr )
    {
        const ShowcaseSceneBuilder& ui = m_Ui;
        TextWidget* label = nullptr;
        WidgetBuilder<ButtonWidget> button = ui.Button( std::move( a_OnClick ) ).FlexWidth().FixedHeight( 36_u ).Padding( Edges::Axis( 6_u, 0_u ) )(
            ui.Label( std::move( a_Label ), 15_u ).Theme( m_Label ).Bind( label )
        );
        if ( a_OutLabel )
            *a_OutLabel = label;
        return button;
    }

    /** @brief A header button, at most a_Width wide and never narrower than its label, that shows the accent color while selected. */
    WidgetBuilder<ButtonWidget> Toggle( String a_Label, Unit a_Width, std::function<void()> a_OnClick, ToggleButton& a_Out )
    {
        return Button( std::move( a_Label ), [a_OnClick]( ButtonBaseWidget& ) { a_OnClick(); }, &a_Out.Label )
            .MinWidth( 84_u ).MaxWidth( a_Width ).Bind( a_Out.Button );
    }

    /** @brief A list row: a name on the left and a muted detail on the right. */
    WidgetBuilder<ButtonWidget> ListRow( String a_Name, String a_Detail )
    {
        const ShowcaseSceneBuilder& ui = m_Ui;
        return ui.Button( [this, a_Name]( ButtonBaseWidget& ) { SetStatus( "Picked " + a_Name ); } )
            .Horizontal().FlexWidth().FixedHeight( 34_u ).Padding( Edges::Axis( 12_u, 0_u ) ).ChildAlign( EAlign::CenterLeft )(
                ui.Label( std::move( a_Name ), 15_u ),
                Muted( ui.SizedText( std::move( a_Detail ), 13_u ) )
            );
    }

    /** @brief An inset card with a slider, its name and its value. */
    WidgetBuilder<PanelWidget> LabeledSlider( String a_Label, f32 a_Value, EOrient a_Orientation = EOrient::Horizontal )
    {
        const ShowcaseSceneBuilder& ui = m_Ui;
        const bool vertical = a_Orientation == EOrient::Vertical;

        // A vertical slider is narrow, so its name and value are stacked and centred, in a card with less padding.
        WidgetBuilder<TextWidget> name = ui.Label( a_Label, 14_u );
        if ( vertical )
            name.Theme( m_Label );

        TextWidget* value = nullptr;
        NodeBuilder header = vertical ? ui.Column( 2_u ).ChildAlign( EAlign::TopCenter ) : ui.Row( 6_u );
        header( name, Muted( ui.SizedText( "", 14_u ) ).Bind( value ) );

        WidgetBuilder<SliderWidget> slider = ui.Slider( 0.f, 1.f, a_Value, a_Orientation ).Step( 0.05f ); // Snaps to 5%; the arrow keys move one step.
        if ( vertical )
            slider.FixedSize( 32_u, 110_u );
        else
            slider.FlexWidth().FixedHeight( 26_u );

        slider.OnValueChanged( [this, value, a_Label]( const f32& a_Current )
        {
            const int percent = static_cast<int>( std::round( a_Current * 100.f ) );
            value->SetText( Text{ std::format( "{}%", percent ) } );
            if ( m_Status )
                SetStatus( std::format( "{}: {}%", a_Label, percent ) );
        } );

        WidgetBuilder<PanelWidget> card = ui.Box( true ).Theme( m_Page ).Vertical().Padding( vertical ? 6_u : 10_u ).Spacing( 8_u );
        if ( vertical )
            card.ChildAlign( EAlign::TopCenter );
        return card( header, slider );
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

        *m_Screen = *m_Page;
        m_Screen->SetRadius( ThemeKey::Radii::Panel, CornerRadius::None() );
        m_Screen->SetMetric( ThemeKey::Metric::PanelBorderThickness, 0_u );

        *m_Muted = *theme.Style;
        m_Muted->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = theme.Muted } );

        *m_Hint = *theme.Style;
        m_Hint->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .Align = ETextAlign::Right, .FillColor = theme.Muted } );

        *m_Accent = *theme.Style;
        m_Accent->SetBrushes( {
            { ThemeKey::Brush::ButtonNormal,  SolidBrush{ theme.Accent } },
            { ThemeKey::Brush::ButtonHover,   SolidBrush{ Lerp( theme.Accent, theme.OnAccent, 0.15f ) } },
            { ThemeKey::Brush::ButtonPressed, SolidBrush{ Lerp( theme.Accent, Colors::Black, 0.25f ) } }
        } );
        m_Accent->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = theme.OnAccent } );

        MakeCentred( *m_Label, *m_Base );
        MakeCentred( *m_AccentLabel, *m_Accent );

        for ( size i = 0; i < Size( m_ThemeButtons ); ++i )
            SetSelected( m_ThemeButtons[i], i == a_Index );
        SetSelected( m_SafeAreaButton, m_SafeAreaEnabled );

        SetStatus( "Theme: " + theme.Name );
    }

    void SetSelected( ToggleButton& a_Toggle, bool a_Selected )
    {
        a_Toggle.Button->Theme = a_Selected ? m_Accent : m_Base;
        a_Toggle.Label->Theme  = a_Selected ? m_AccentLabel : m_Label;
    }

    /** @brief Makes a_Out a copy of a_From whose text is centred in its box. */
    static void MakeCentred( Theme& a_Out, const Theme& a_From )
    {
        TextRenderStyle style = a_From.GetTextStyle( ThemeKey::TextStyle::Default );
        style.Align           = ETextAlign::Center;
        a_Out                 = a_From;
        a_Out.SetTextStyle( ThemeKey::TextStyle::Default, style );
    }

    static Color Lerp( Color a_From, Color a_To, f32 a_T )
    {
        const auto mix = [&]( int i ) { return ( static_cast<f32>( a_From[i] ) * ( 1.f - a_T ) + static_cast<f32>( a_To[i] ) * a_T ) / 255.f; };
        return FromColorF32( mix( 0 ), mix( 1 ), mix( 2 ), mix( 3 ) );
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
