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
    {
        BuildThemes();
        ApplyTheme( 0 );

        WidgetID root = m_Scene.CreateRootWidget<PanelWidget>( m_ActiveTheme );
        GetNode( root )->Style
            .SetLayoutType( ELayoutType::Vertical )
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Flex )
            .SetPadding( Edges::Uniform( 16_u ) )
            .SetSpacing( 12_u )
            .SetFocusScope( true );

        WidgetID title = m_Scene.CreateWidget<TextWidget>( root, m_ActiveTheme, MakeText( "Theme Showcase" ), MakeTextLayout( 30_u, ETextOverflow::Clip ) );
        GetNode( title )->Style.HeightMode = ESizingMode::Content;

        m_StatusText = m_Scene.CreateWidget<TextWidget>( root, m_ActiveTheme, MakeText( "" ), MakeTextLayout( 16_u, ETextOverflow::Clip ) );
        GetNode( m_StatusText )->Style.HeightMode = ESizingMode::Content;

        WidgetID themeButtonRow = m_Scene.CreateWidget<PanelWidget>( root, m_ActiveTheme );
        GetNode( themeButtonRow )->Style
            .SetLayoutType( ELayoutType::Horizontal )
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Fixed )
            .SetFixedHeight( 54_u )
            .SetPadding( Edges::Uniform( 8_u ) )
            .SetSpacing( 8_u )
            .SetFocusScope( true );

        CreateThemeButton( themeButtonRow, "Dark", 0 );
        CreateThemeButton( themeButtonRow, "Light", 1 );
        CreateThemeButton( themeButtonRow, "Neon", 2 );
		CreateThemeButton( themeButtonRow, "Minecraft", 3 );

        WidgetID contentRow = m_Scene.CreateWidget<PanelWidget>( root, m_ActiveTheme );
        GetNode( contentRow )->Style
            .SetLayoutType( ELayoutType::Horizontal )
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Flex )
            .SetPadding( Edges::Uniform( 12_u ) )
            .SetSpacing( 12_u )
            .SetFocusScope( true );

        WidgetID controlsPanel = m_Scene.CreateWidget<PanelWidget>( contentRow, m_ActiveTheme );
        GetNode( controlsPanel )->Style
            .SetLayoutType( ELayoutType::Vertical )
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Flex )
            .SetPadding( Edges::Uniform( 12_u ) )
            .SetSpacing( 10_u )
            .SetFlexGrow( 1.f )
            .SetFocusScope( true );

        WidgetID previewPanel = m_Scene.CreateWidget<PanelWidget>( contentRow, m_ActiveTheme );
        GetNode( previewPanel )->Style
			.SetLayoutType( ELayoutType::Vertical )
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Flex )
            .SetPadding( Edges::Uniform( 12_u ) )
            .SetSpacing( 10_u )
            .SetFlexGrow( 1.f )
            .SetFocusScope( true );

        WidgetID controlsTitle = m_Scene.CreateWidget<TextWidget>( controlsPanel, m_ActiveTheme, MakeText( "Controls" ), MakeTextLayout( 20_u, ETextOverflow::Clip ) );
        GetNode( controlsTitle )->Style.HeightMode = ESizingMode::Content;

        CreateSliderCard( controlsPanel, "Master Volume", 0.65f, false );
        CreateSliderCard( controlsPanel, "Accent Strength", 0.30f, false );
        CreateSliderCard( controlsPanel, "Vertical Mix", 0.45f, true );

        WidgetID previewTitle = m_Scene.CreateWidget<TextWidget>( previewPanel, m_ActiveTheme, MakeText( "Preview" ), MakeTextLayout( 20_u, ETextOverflow::Clip ) );
        GetNode( previewTitle )->Style.HeightMode = ESizingMode::Content;

        WidgetID previewCard = m_Scene.CreateWidget<PanelWidget>( previewPanel, m_ActiveTheme );
        GetNode( previewCard )->Style
            .SetLayoutType( ELayoutType::Vertical )
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Flex )
            .SetPadding( Edges::Uniform( 10_u ) )
            .SetSpacing( 8_u );

        WidgetID previewText = m_Scene.CreateWidget<TextWidget>(
            previewCard,
            m_ActiveTheme,
            MakeText( "This panel uses the active theme for panel fills, text color, button states, and slider visuals." ),
            MakeTextLayout( 16_u, ETextOverflow::Fade, TextWrap::WrapWord() ) );
        GetNode( previewText )->Style
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Content );

		// Add TextWidget with vertical overflow to demonstrate text overflow handling in the active theme. This will intentionally overflow to show the fade effect.
        {
            WidgetID overflowText = m_Scene.CreateWidget<TextWidget>(
                previewCard,
                m_ActiveTheme,
                MakeText( "This is an example of a long text string that will exceed the width of the container and demonstrate how the active theme handles text overflow with a fade effect." ),
                MakeTextLayout( 16_u, ETextOverflow::Fade, TextWrap::WrapWord() ) );
            GetNode( overflowText )->Style
                .SetWidthMode( ESizingMode::Fixed )
                .SetFixedWidth( 100_u )
                .SetHeightMode( ESizingMode::Fixed )
                .SetFixedHeight( 100_u );
        }

        WidgetID actionButton = m_Scene.CreateWidget<ButtonWidget>( previewCard, m_ActiveTheme,
            [this]( Scene&, WidgetID )
            {
                if ( auto* status = m_Scene.GetWidget<TextWidget>( m_StatusText ) )
                {
                    status->SetText( { "Theme applied: " + m_ThemeNames[m_ActiveThemeIndex] + " (preview action clicked)" } );
                }
            } );
        GetNode( actionButton )->Style
            .SetWidthMode( ESizingMode::Fixed )
            .SetFixedWidth( 280_u )
            .SetHeightMode( ESizingMode::Fixed )
            .SetFixedHeight( 40_u )
            .SetChildAlign( EAlignment::Center );

        WidgetID actionButtonText = m_Scene.CreateWidget<TextWidget>( actionButton, m_ActiveTheme, MakeText( "Preview Button" ), MakeTextLayout( 16_u, ETextOverflow::Clip ) );
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
            { ThemeKey::Color::SliderTrack, Colors::Silver },
            { ThemeKey::Color::SliderTrackFill, Colors::DarkBlue },
            { ThemeKey::Color::SliderThumb, Colors::AccentBlue },
            { ThemeKey::Color::SliderThumbHover, Colors::Blue },
            { ThemeKey::Color::SliderThumbPressed, Colors::DarkBlue }
        } );
		m_Themes[1]->SetBrushes( {
            { ThemeKey::Brush::PanelNormal,   SolidBrush{ Colors::LightGray } },
            { ThemeKey::Brush::ButtonNormal,  SolidBrush{ Colors::White } },
            { ThemeKey::Brush::ButtonHover,   SolidBrush{ Colors::PowderBlue } },
            { ThemeKey::Brush::ButtonPressed, SolidBrush{ Colors::LightBlue } },
		} );
        m_Themes[1]->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = Colors::Surface900 } );

		// Neon
        m_Themes[2] = MakeShared<Theme>( *Themes::Dark() );
        m_Themes[2]->SetColors( {
            { ThemeKey::Color::FocusOutline, Colors::AccentRose },
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
        m_Themes[2]->SetBrushes( {
            { ThemeKey::Brush::PanelNormal,   SolidBrush{ FromColorF32( 0.07f, 0.03f, 0.10f ) } },
            { ThemeKey::Brush::ButtonNormal,  SolidBrush{ FromColorF32( 0.20f, 0.05f, 0.28f ) } },
            { ThemeKey::Brush::ButtonHover,   SolidBrush{ FromColorF32( 0.30f, 0.08f, 0.45f ) } },
            { ThemeKey::Brush::ButtonPressed, SolidBrush{ FromColorF32( 0.12f, 0.45f, 0.42f ) } },
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

        m_ActiveTheme = MakeShared<Theme>( *m_Themes[0] );
    }

    void ApplyTheme( size a_ThemeIndex )
    {
        m_ActiveThemeIndex = a_ThemeIndex % 4;
        *m_ActiveTheme = *m_Themes[m_ActiveThemeIndex];

        UpdateStatusText();
    }

    void CreateThemeButton( WidgetID a_Parent, const String& a_Label, size a_ThemeIndex )
    {
        WidgetID button = m_Scene.CreateWidget<ButtonWidget>( a_Parent, m_ActiveTheme,
            [this, a_ThemeIndex]( Scene&, WidgetID )
            {
                ApplyTheme( a_ThemeIndex );
            } );

        LayoutNode* buttonNode = GetNode( button );
        buttonNode->Style.WidthMode = ESizingMode::Fixed;
        buttonNode->Style.FixedWidth = 130_u;
        buttonNode->Style.HeightMode = ESizingMode::Flex;
        buttonNode->Style.ChildAlign = EAlignment::Center;

        WidgetID text = m_Scene.CreateWidget<TextWidget>( button, m_ActiveTheme, MakeText( a_Label ), MakeTextLayout( 16_u, ETextOverflow::Clip ) );
        GetNode( text )->Style.Visibility = EVisibility::HitTestInvisible;
    }

    void CreateSliderCard( WidgetID a_Parent, const String& a_Label, f32 a_Value, bool a_Vertical )
    {
        WidgetID card = m_Scene.CreateWidget<PanelWidget>( a_Parent, m_ActiveTheme );
        LayoutNode* cardNode = GetNode( card );
        cardNode->Style.LayoutType = ELayoutType::Vertical;
        cardNode->Style.WidthMode = ESizingMode::Flex;
        cardNode->Style.HeightMode = ESizingMode::Fixed;
        cardNode->Style.FixedHeight = a_Vertical ? 180_u : 86_u;
        cardNode->Style.Padding = Edges::Uniform( 8_u );
        cardNode->Style.Spacing = 6_u;

        WidgetID valueText = m_Scene.CreateWidget<TextWidget>( card, m_ActiveTheme, MakeText( "" ), MakeTextLayout( 14_u, ETextOverflow::Clip ) );
        GetNode( valueText )->Style.HeightMode = ESizingMode::Content;

        WidgetID slider = m_Scene.CreateWidget<SliderWidget>( card, m_ActiveTheme, 0.f, 1.f, a_Value );
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
                    text->SetText( { a_Label + ": " + std::to_string( percent ) + "%" } );
                }
            } );
        }
    }

    void UpdateStatusText()
    {
        if ( auto* status = m_Scene.GetWidget<TextWidget>( m_StatusText ) )
            status->SetText( { "Theme applied: " + m_ThemeNames[m_ActiveThemeIndex] + " (use the buttons below to switch styles)" } );
    }
};
