#pragma once
#include "IDemoScene.h"
#include <RatUI/Widget/SliderWidget.h>

#include <array>
#include <cmath>

class ThemeShowcaseScene : public IDemoScene
{
public:
    ThemeShowcaseScene( FontHandle a_Font, ITextMetrics* a_TextMetrics )
        : IDemoScene( a_TextMetrics )
        , DefaultFont( a_Font )
    {}

    ~ThemeShowcaseScene() override = default;

    void Init() override
    {
        BuildThemes();
        ApplyTheme( 0 );

        WidgetID root = TrackThemedWidget( m_Scene.CreateRootWidget<PanelWidget>( m_ActiveTheme ) );
        LayoutNode* rootNode = GetNode( root );
        rootNode->Style.LayoutType = ELayoutType::Vertical;
        rootNode->Style.WidthMode = ESizingMode::Flex;
        rootNode->Style.HeightMode = ESizingMode::Flex;
        rootNode->Style.Padding = Edges::Uniform( 16_u );
        rootNode->Style.Spacing = 12_u;
        rootNode->Style.IsFocusScope = true;

        WidgetID title = TrackThemedWidget( m_Scene.CreateWidget<TextWidget>( root, m_ActiveTheme, "Theme Showcase", MakeTextLayout( 30_u, ETextOverflow::Clip ) ) );
        GetNode( title )->Style.HeightMode = ESizingMode::Content;

        m_StatusText = TrackThemedWidget( m_Scene.CreateWidget<TextWidget>( root, m_ActiveTheme, "", MakeTextLayout( 16_u, ETextOverflow::Clip ) ) );
        GetNode( m_StatusText )->Style.HeightMode = ESizingMode::Content;

        WidgetID themeButtonRow = TrackThemedWidget( m_Scene.CreateWidget<PanelWidget>( root, m_ActiveTheme ) );
        LayoutNode* buttonRowNode = GetNode( themeButtonRow );
        buttonRowNode->Style.LayoutType = ELayoutType::Horizontal;
        buttonRowNode->Style.WidthMode = ESizingMode::Flex;
        buttonRowNode->Style.HeightMode = ESizingMode::Fixed;
        buttonRowNode->Style.FixedHeight = 54_u;
        buttonRowNode->Style.Padding = Edges::Uniform( 8_u );
        buttonRowNode->Style.Spacing = 8_u;
        buttonRowNode->Style.IsFocusScope = true;

        CreateThemeButton( themeButtonRow, "Dark", 0 );
        CreateThemeButton( themeButtonRow, "Light", 1 );
        CreateThemeButton( themeButtonRow, "Neon", 2 );
		CreateThemeButton( themeButtonRow, "Minecraft", 3 );

        WidgetID contentRow = TrackThemedWidget( m_Scene.CreateWidget<PanelWidget>( root, m_ActiveTheme ) );
        LayoutNode* contentNode = GetNode( contentRow );
        contentNode->Style.LayoutType = ELayoutType::Horizontal;
        contentNode->Style.WidthMode = ESizingMode::Flex;
        contentNode->Style.HeightMode = ESizingMode::Flex;
        contentNode->Style.Padding = Edges::Uniform( 12_u );
        contentNode->Style.Spacing = 12_u;
        contentNode->Style.IsFocusScope = true;

        WidgetID controlsPanel = TrackThemedWidget( m_Scene.CreateWidget<PanelWidget>( contentRow, m_ActiveTheme ) );
        LayoutNode* controlsNode = GetNode( controlsPanel );
        controlsNode->Style.LayoutType = ELayoutType::Vertical;
        controlsNode->Style.WidthMode = ESizingMode::Flex;
        controlsNode->Style.HeightMode = ESizingMode::Flex;
        controlsNode->Style.Padding = Edges::Uniform( 12_u );
        controlsNode->Style.Spacing = 10_u;
        controlsNode->Style.FlexGrow = 1.f;
		controlsNode->Style.IsFocusScope = true;

        WidgetID previewPanel = TrackThemedWidget( m_Scene.CreateWidget<PanelWidget>( contentRow, m_ActiveTheme ) );
        GetNode( previewPanel )->Style
			.SetLayoutType( ELayoutType::Vertical )
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Flex )
            .SetPadding( Edges::Uniform( 12_u ) )
            .SetSpacing( 10_u )
            .SetFlexGrow( 1.f )
            .SetFocusScope( true );

        WidgetID controlsTitle = TrackThemedWidget( m_Scene.CreateWidget<TextWidget>( controlsPanel, m_ActiveTheme, "Controls", MakeTextLayout( 20_u, ETextOverflow::Clip ) ) );
        GetNode( controlsTitle )->Style.HeightMode = ESizingMode::Content;

        CreateSliderCard( controlsPanel, "Master Volume", 0.65f, false );
        CreateSliderCard( controlsPanel, "Accent Strength", 0.30f, false );
        CreateSliderCard( controlsPanel, "Vertical Mix", 0.45f, true );

        WidgetID previewTitle = TrackThemedWidget( m_Scene.CreateWidget<TextWidget>( previewPanel, m_ActiveTheme, "Preview", MakeTextLayout( 20_u, ETextOverflow::Clip ) ) );
        GetNode( previewTitle )->Style.HeightMode = ESizingMode::Content;

        WidgetID previewCard = TrackThemedWidget( m_Scene.CreateWidget<PanelWidget>( previewPanel, m_ActiveTheme ) );
        LayoutNode* previewCardNode = GetNode( previewCard );
        previewCardNode->Style.LayoutType = ELayoutType::Vertical;
        previewCardNode->Style.WidthMode = ESizingMode::Flex;
        previewCardNode->Style.HeightMode = ESizingMode::Flex;
        previewCardNode->Style.Padding = Edges::Uniform( 10_u );
        previewCardNode->Style.Spacing = 8_u;

        WidgetID previewText = TrackThemedWidget( m_Scene.CreateWidget<TextWidget>(
            previewCard,
            m_ActiveTheme,
            "This panel uses the active theme for panel fills, text color, button states, and slider visuals.",
            MakeTextLayout( 16_u, ETextOverflow::Fade, TextWrap::WrapWord() ) ) );
        LayoutNode* previewTextNode = GetNode( previewText );
        previewTextNode->Style.WidthMode = ESizingMode::Flex;
        previewTextNode->Style.HeightMode = ESizingMode::Content;

        WidgetID actionButton = TrackThemedWidget( m_Scene.CreateWidget<ButtonWidget>( previewCard, m_ActiveTheme,
            [this]( Scene&, WidgetID )
            {
                if ( auto* status = m_Scene.GetWidget<TextWidget>( m_StatusText ) )
                {
                    status->SetText( "Theme applied: " + m_ThemeNames[m_ActiveThemeIndex] + " (preview action clicked)" );
                }
            } ) );
        LayoutNode* actionButtonNode = GetNode( actionButton );
        actionButtonNode->Style.WidthMode = ESizingMode::Fixed;
        actionButtonNode->Style.FixedWidth = 280_u;
        actionButtonNode->Style.HeightMode = ESizingMode::Fixed;
        actionButtonNode->Style.FixedHeight = 40_u;
        actionButtonNode->Style.ChildAlign = EAlignment::Center;

        WidgetID actionButtonText = TrackThemedWidget( m_Scene.CreateWidget<TextWidget>( actionButton, m_ActiveTheme, "Preview Button", MakeTextLayout( 16_u, ETextOverflow::Clip ) ) );
        GetNode( actionButtonText )->Style.Visibility = EVisibility::HitTestInvisible;

        UpdateStatusText();
    }

    void OnInputEvent( const InputEvent& a_Event ) override
    {
        if ( a_Event.Device == EDeviceID::Keyboard )
        {
            const ButtonEvent& btnEvent = Get<ButtonEvent>( a_Event.Payload );

            switch ( btnEvent.Button )
            {
                case EButtonID::KeyUp:     if ( btnEvent.Pressed ) m_Scene.Navigate( ENavAction::MoveUp ); break;
                case EButtonID::KeyDown:   if ( btnEvent.Pressed ) m_Scene.Navigate( ENavAction::MoveDown ); break;
                case EButtonID::KeyLeft:   if ( btnEvent.Pressed ) m_Scene.Navigate( ENavAction::MoveLeft ); break;
                case EButtonID::KeyRight:  if ( btnEvent.Pressed ) m_Scene.Navigate( ENavAction::MoveRight ); break;
                case EButtonID::KeyEscape: if ( btnEvent.Pressed ) m_Scene.Navigate( ENavAction::Cancel ); break;
                case EButtonID::KeyEnter:
                    if ( btnEvent.Pressed ) m_Scene.Navigate( ENavAction::ActivatePressed );
                    else if ( btnEvent.Released ) m_Scene.Navigate( ENavAction::ActivateReleased );
                    break;
                default: break;
            }
            return;
        }

        m_Scene.DispatchInputEvent( a_Event );
    }

    void Render( DrawList& a_DrawList ) override
    {
        m_Scene.Render( a_DrawList );
    }

private:
    FontHandle DefaultFont;
    Shared<Theme> m_Themes[4];
    Shared<Theme> m_ActiveTheme;
    size m_ActiveThemeIndex{ 0 };

	std::array<String, 4> m_ThemeNames{ "Dark", "Light", "Neon", "Minecraft" };
    Array<WidgetID> m_ThemedWidgets;
    WidgetID m_StatusText{ c_InvalidWidgetID };

    TextLayoutStyle MakeTextLayout( Unit a_Size, ETextOverflow a_Overflow, TextWrap a_Wrap = TextWrap::NoWrap() ) const
    {
        TextLayoutStyle style{};
        style.Font = DefaultFont;
        style.Size = a_Size;
        style.Overflow = a_Overflow;
        style.Wrap = a_Wrap;
        return style;
    }

    LayoutNode* GetNode( WidgetID a_WidgetID )
    {
        IWidget* widget = m_Scene.GetWidget( a_WidgetID );
        return widget ? m_Scene.Layouts.Get( widget->GetLayoutID() ) : nullptr;
    }

    WidgetID TrackThemedWidget( WidgetID a_WidgetID )
    {
        if ( a_WidgetID != c_InvalidWidgetID )
            m_ThemedWidgets.push_back( a_WidgetID );
        return a_WidgetID;
    }

    void BuildThemes()
    {
		// Dark
        m_Themes[0] = MakeShared<Theme>( *Themes::Dark() );
        m_Themes[0]->SetColors( {
            { ThemeKey::Color::SliderThumbHover, Colors::LightBlue },
            { ThemeKey::Color::SliderThumbPressed, Colors::AccentBlue },
            { ThemeKey::Color::SliderTrackFill, Colors::AccentBlue }
        } );
        m_Themes[0]->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = Colors::White } );

		// Light
        m_Themes[1] = MakeShared<Theme>( *Themes::Dark() );
        m_Themes[1]->SetColors( {
            { ThemeKey::Color::FocusOutline, Colors::DarkBlue },
            { ThemeKey::Color::PanelNormal, Colors::LightGray },
            { ThemeKey::Color::ButtonNormal, Colors::White },
            { ThemeKey::Color::ButtonHover, Colors::PowderBlue },
            { ThemeKey::Color::ButtonPressed, Colors::LightBlue },
            { ThemeKey::Color::SliderTrack, Colors::Silver },
            { ThemeKey::Color::SliderTrackFill, Colors::DarkBlue },
            { ThemeKey::Color::SliderThumb, Colors::AccentBlue },
            { ThemeKey::Color::SliderThumbHover, Colors::Blue },
            { ThemeKey::Color::SliderThumbPressed, Colors::DarkBlue }
        } );
        m_Themes[1]->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = Colors::Surface900 } );

		// Neon
        m_Themes[2] = MakeShared<Theme>( *Themes::Dark() );
        m_Themes[2]->SetColors( {
            { ThemeKey::Color::FocusOutline, Colors::AccentRose },
            { ThemeKey::Color::PanelNormal, FromColorF32( 0.07f, 0.03f, 0.10f ) },
            { ThemeKey::Color::ButtonNormal, FromColorF32( 0.20f, 0.05f, 0.28f ) },
            { ThemeKey::Color::ButtonHover, FromColorF32( 0.30f, 0.08f, 0.45f ) },
            { ThemeKey::Color::ButtonPressed, FromColorF32( 0.12f, 0.45f, 0.42f ) },
            { ThemeKey::Color::SliderTrack, FromColorF32( 0.10f, 0.10f, 0.20f ) },
            { ThemeKey::Color::SliderTrackFill, Colors::AccentRose },
            { ThemeKey::Color::SliderThumb, Colors::AccentSky },
            { ThemeKey::Color::SliderThumbHover, Colors::LightCyan },
            { ThemeKey::Color::SliderThumbPressed, Colors::AccentRose }
        } );
        m_Themes[2]->SetRoundings( {
            { ThemeKey::Rounding::Panel, CornerRounding::Uniform( 12_u ) },
            { ThemeKey::Rounding::Button, CornerRounding::Uniform( 10_u ) },
            { ThemeKey::Rounding::SliderTrack, CornerRounding::Uniform( 5_u ) },
            { ThemeKey::Rounding::SliderThumb, CornerRounding::Uniform( 8_u ) }
        } );
        m_Themes[2]->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = Colors::AccentSky } );

		for ( auto& theme : m_Themes )
		{
			if ( theme )
				theme->SetFont( ThemeKey::Font::Default, DefaultFont );
		}

        // Minecraft
		m_Themes[3] = MakeShared<Theme>( *m_Themes[0] );
        m_Themes[3]->SetFont( ThemeKey::Font::Default, FontHandle{ 2 } );
        for ( const auto& [key, value] : m_Themes[3]->GetRoundings() )
        {
			m_Themes[3]->SetRounding( key, CornerRounding::None() ); // override all roundings to be 0 (sharp corners)
        }

        // Set slider fill to green and thumbs to gray white like minecraft bedrock
		m_Themes[3]->SetColors( {
            { ThemeKey::Color::SliderTrackFill, FromColorF32( 0.1f, 0.5f, 0.1f ) },
			{ ThemeKey::Color::SliderThumb, FromColorF32( 0.9f, 0.9f, 0.9f ) },
			{ ThemeKey::Color::SliderThumbHover, FromColorF32( 0.8f, 0.8f, 0.8f ) },
			{ ThemeKey::Color::SliderThumbPressed, FromColorF32( 1.f, 1.f, 1.f ) }
		} );
    }

    void ApplyTheme( size a_ThemeIndex )
    {
        m_ActiveThemeIndex = a_ThemeIndex % 4;
        m_ActiveTheme = m_Themes[m_ActiveThemeIndex];

        for ( WidgetID id : m_ThemedWidgets )
        {
            if ( auto* panel = m_Scene.GetWidget<PanelWidget>( id ) )
                panel->SetTheme( m_ActiveTheme );
            else if ( auto* button = m_Scene.GetWidget<ButtonWidget>( id ) )
                button->SetTheme( m_ActiveTheme );
            else if ( auto* slider = m_Scene.GetWidget<SliderWidget>( id ) )
                slider->SetTheme( m_ActiveTheme );
            else if ( auto* text = m_Scene.GetWidget<TextWidget>( id ) )
                text->SetTheme( m_ActiveTheme );
        }

        UpdateStatusText();
    }

    void CreateThemeButton( WidgetID a_Parent, const String& a_Label, size a_ThemeIndex )
    {
        WidgetID button = TrackThemedWidget( m_Scene.CreateWidget<ButtonWidget>( a_Parent, m_ActiveTheme,
            [this, a_ThemeIndex]( Scene&, WidgetID )
            {
                ApplyTheme( a_ThemeIndex );
            } ) );

        LayoutNode* buttonNode = GetNode( button );
        buttonNode->Style.WidthMode = ESizingMode::Fixed;
        buttonNode->Style.FixedWidth = 130_u;
        buttonNode->Style.HeightMode = ESizingMode::Flex;
        buttonNode->Style.ChildAlign = EAlignment::Center;

        WidgetID text = TrackThemedWidget( m_Scene.CreateWidget<TextWidget>( button, m_ActiveTheme, a_Label, MakeTextLayout( 16_u, ETextOverflow::Clip ) ) );
        GetNode( text )->Style.Visibility = EVisibility::HitTestInvisible;
    }

    void CreateSliderCard( WidgetID a_Parent, const String& a_Label, f32 a_Value, bool a_Vertical )
    {
        WidgetID card = TrackThemedWidget( m_Scene.CreateWidget<PanelWidget>( a_Parent, m_ActiveTheme ) );
        LayoutNode* cardNode = GetNode( card );
        cardNode->Style.LayoutType = ELayoutType::Vertical;
        cardNode->Style.WidthMode = ESizingMode::Flex;
        cardNode->Style.HeightMode = ESizingMode::Fixed;
        cardNode->Style.FixedHeight = a_Vertical ? 180_u : 86_u;
        cardNode->Style.Padding = Edges::Uniform( 8_u );
        cardNode->Style.Spacing = 6_u;

        WidgetID valueText = TrackThemedWidget( m_Scene.CreateWidget<TextWidget>( card, m_ActiveTheme, "", MakeTextLayout( 14_u, ETextOverflow::Clip ) ) );
        GetNode( valueText )->Style.HeightMode = ESizingMode::Content;

        WidgetID slider = TrackThemedWidget( m_Scene.CreateWidget<SliderWidget>( card, m_ActiveTheme, 0.f, 1.f, a_Value ) );
        SliderWidget* sliderWidget = m_Scene.GetWidget<SliderWidget>( slider );
        LayoutNode* sliderNode = GetNode( slider );
        sliderNode->Style.WidthMode = ESizingMode::Flex;
        sliderNode->Style.HeightMode = ESizingMode::Fixed;
        sliderNode->Style.FixedHeight = a_Vertical ? 120_u : 28_u;

        if ( a_Vertical && sliderWidget )
        {
            sliderWidget->Orientation = EOrientation::Vertical;
            sliderNode->Style.FixedWidth = 36_u;
            sliderNode->Style.WidthMode = ESizingMode::Fixed;
        }

        if ( sliderWidget )
        {
            sliderWidget->Value.Subscribe( [this, valueText, a_Label]( const f32& a_CurrentValue )
            {
                if ( auto* text = m_Scene.GetWidget<TextWidget>( valueText ) )
                {
                    const int percent = static_cast<int>( std::round( a_CurrentValue * 100.f ) );
                    text->SetText( a_Label + ": " + std::to_string( percent ) + "%" );
                }
            } );
        }
    }

    void UpdateStatusText()
    {
        if ( auto* status = m_Scene.GetWidget<TextWidget>( m_StatusText ) )
            status->SetText( "Theme applied: " + m_ThemeNames[m_ActiveThemeIndex] + " (use the buttons below to switch styles)" );
    }
};
