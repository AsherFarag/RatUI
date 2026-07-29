#pragma once
#include "IDemoScene.h"
#include <RatUI/Widget/SliderWidget.h>
#include <RatUI/Widget/InputTextWidget.h>
#include <RatUI/Widget/ScrollContainerWidget.h>
#include <RatUI/Widget/WidgetBuilder.h>
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

        PanelWidget* root = m_Scene.CreateRootWidget<PanelWidget>();
        root->Theme = m_ActiveTheme;
        root->GetLayout()
            .LayoutType( ELayoutType::Vertical )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Flex )
            .Padding( Edges::All( 16_u ) )
            .Spacing( 12_u )
            .FocusScope( true );

        TextWidget* title = m_Scene.CreateWidget<TextWidget>( root->GetLayoutID(), MakeText( "Theme Showcase" ), MakeTextLayout( 30_u, ETextOverflow::Clip ) );
        title->Theme = m_ActiveTheme;
        title->GetLayout().HeightMode( ESizing::Content );

        m_StatusText = m_Scene.CreateWidget<TextWidget>( root->GetLayoutID(), MakeText( "" ), MakeTextLayout( 16_u, ETextOverflow::Clip ) );
        m_StatusText->Theme = m_ActiveTheme;
        m_StatusText->GetLayout().HeightMode( ESizing::Content );

        PanelWidget* themeButtonRow = m_Scene.CreateWidget<PanelWidget>( root->GetLayoutID() );
		themeButtonRow->Theme = m_ActiveTheme;
        themeButtonRow->GetLayout()
            .LayoutType( ELayoutType::Horizontal )
            .WidthMode( ESizing::Flex )
            .FixedHeight( 54_u )
            .Padding( Edges::All( 8_u ) )
            .Spacing( 8_u )
            .FocusScope( true );

        CreateThemeButton( themeButtonRow, "Dark", 0 );
        CreateThemeButton( themeButtonRow, "Light", 1 );
        CreateThemeButton( themeButtonRow, "Neon", 2 );
        CreateThemeButton( themeButtonRow, "Minecraft", 3 );

        PanelWidget* contentRow = m_Scene.CreateWidget<PanelWidget>( root->GetLayoutID() );
        contentRow->Theme = m_ActiveTheme;
        contentRow->GetLayout()
            .LayoutType( ELayoutType::Horizontal )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Flex )
            .Padding( Edges::All( 12_u ) )
            .Spacing( 12_u )
            .FocusScope( true );

        PanelWidget* controlsPanel = m_Scene.CreateWidget<PanelWidget>( contentRow->GetLayoutID() );
        controlsPanel->Theme = m_ActiveTheme;
        controlsPanel->GetLayout()
            .LayoutType( ELayoutType::Vertical )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Flex )
            .Padding( Edges::All( 12_u ) )
            .Spacing( 10_u )
            .FlexGrow( 1.f )
            .FocusScope( true );

        PanelWidget* previewPanel = m_Scene.CreateWidget<PanelWidget>( contentRow->GetLayoutID() );
        previewPanel->Theme = m_ActiveTheme;
        previewPanel->GetLayout()
            .LayoutType( ELayoutType::Vertical )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Flex )
            .Padding( Edges::All( 12_u ) )
            .Spacing( 10_u )
            .FlexGrow( 1.f )
            .FocusScope( true );

        TextWidget* controlsTitle = m_Scene.CreateWidget<TextWidget>( controlsPanel->GetLayoutID(), MakeText( "Controls" ), MakeTextLayout( 20_u, ETextOverflow::Clip ) );
        controlsTitle->Theme = m_ActiveTheme;
        controlsTitle->GetLayout().HeightMode( ESizing::Content );

        CreateSliderCard( controlsPanel, "Master Volume",    0.65f, false );
        CreateSliderCard( controlsPanel, "Accent Strength",  0.30f, false );
        CreateSliderCard( controlsPanel, "Vertical Mix",     0.45f, true  );

		// 2x2 Grid of sliders
        {
			PanelWidget* buttonGrid = m_Scene.CreateWidget<PanelWidget>( controlsPanel->GetLayoutID() );
			buttonGrid->Theme = m_ActiveTheme;
			buttonGrid->GetLayout()
				.LayoutType( ELayoutType::Grid )
				.WidthMode( ESizing::Content )
				.HeightMode( ESizing::Content )
				.SizeConstraints( Constraints{}.AtLeast( { 600_u, 75_u } ) )
				.Padding( Edges::All( 8_u ) )
				.Spacing( 8_u )
				.GridColumns( 2 )
				.GridRows( 2 );

			for ( u32 i = 0; i < 4; ++i )
			{
				CreateSliderCard( buttonGrid, std::format( "Slider {}", i + 1 ), 0.5f, false );
			}
        }

        TextWidget* previewTitle = m_Scene.CreateWidget<TextWidget>( previewPanel->GetLayoutID(), MakeText( "Preview" ), MakeTextLayout( 20_u, ETextOverflow::Clip ) );
        previewTitle->Theme = m_ActiveTheme;
        previewTitle->GetLayout().HeightMode( ESizing::Content );

        PanelWidget* previewCard = m_Scene.CreateWidget<PanelWidget>( previewPanel->GetLayoutID() );
        previewCard->Theme = m_ActiveTheme;
        previewCard->GetLayout()
            .LayoutType( ELayoutType::Vertical )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Flex )
            .Padding( Edges::All( 10_u ) )
            .Spacing( 8_u );

        TextWidget* previewText = m_Scene.CreateWidget<TextWidget>(
            previewCard->GetLayoutID(),
            MakeText( "This panel uses the active theme for panel fills, text color, button states, and slider visuals." ),
            MakeTextLayout( 16_u, ETextOverflow::Fade, TextWrap::WrapWord() ) );
        previewText->Theme = m_ActiveTheme;
        previewText->GetLayout()
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Content );

        m_DynamicVisibleGlyphsText = m_Scene.CreateWidget<TextWidget>(
            previewCard->GetLayoutID(),
            MakeText( "My game dialogue uses visible glyphs system. "
                      "This only affects the rendering of the text, not the layout or shaped text data. "
                      "This is useful when you have text that is being revealed over time, such as in a dialogue system, and you want to show only the visible glyphs without affecting the layout of the text. "
            ),
            MakeTextLayout( 16_u, ETextOverflow::Clip, TextWrap::WrapWord() ) );
		m_DynamicVisibleGlyphsText->Theme = m_ActiveTheme;
        m_DynamicVisibleGlyphsText->GetLayout()
            .FixedWidth( 280_u )
            .FlexHeight();

        // Add TextWidget with vertical overflow to demonstrate text overflow handling in the active theme.
        // This will intentionally overflow to show the fade effect.
        {
            TextWidget* overflowText = m_Scene.CreateWidget<TextWidget>(
                previewCard->GetLayoutID(),
                MakeText( "This is an example of a long text string that will exceed the width of the container and demonstrate how the active theme handles text overflow with a fade effect." ),
                MakeTextLayout( 16_u, ETextOverflow::Fade, TextWrap::WrapWord() ) );
            overflowText->Theme = m_ActiveTheme;
            overflowText->GetLayout()
                .WidthMode( ESizing::Fixed )
                .FixedWidth( 100_u )
                .HeightMode( ESizing::Fixed )
                .FixedHeight( 100_u );
        }

        ButtonWidget* actionButton = m_Scene.CreateWidget<ButtonWidget>( previewCard->GetLayoutID(),
			[this]( ButtonBaseWidget& )
            {
                if ( m_StatusText )
                    m_StatusText->SetText( { "Theme applied: " + m_ThemeNames[m_ActiveThemeIndex] + " (preview action clicked)" } );
            } );
        actionButton->Theme = m_ActiveTheme;
        actionButton->GetLayout()
            .WidthMode( ESizing::Fixed )
            .FixedWidth( 280_u )
            .HeightMode( ESizing::Fixed )
            .FixedHeight( 40_u )
            .ChildAlign( EAlign::Center );

        TextWidget* actionButtonText = m_Scene.CreateWidget<TextWidget>( actionButton->GetLayoutID(), MakeText( "Preview Button" ), MakeTextLayout( 16_u, ETextOverflow::Clip ) );
        actionButtonText->Theme = m_ActiveTheme;
        actionButtonText->GetLayout().Visibility( EVisibility::HitTestInvisible );

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
        }

        m_Scene.DispatchInputEvent( a_Event );
    }

	void Render( DrawList& a_DrawList, f32 a_DeltaSeconds ) override
    {
        static f32 timeAccumulator = 0.f;
		timeAccumulator += a_DeltaSeconds;

		m_DynamicVisibleGlyphsText->VisibleGlyphs = 500 * ( std::fmodf( timeAccumulator, 10.f ) / 10.f );

        m_Scene.Render( a_DrawList, a_DeltaSeconds );
    }

private:
    FontHandle    DefaultFont;
    Shared<Theme> m_Themes[4];
    Shared<Theme> m_ActiveTheme;
    size          m_ActiveThemeIndex{ 0 };

    std::array<String, 4> m_ThemeNames{ "Dark", "Light", "Neon", "Minecraft" };
    TextWidget* m_StatusText{ nullptr };
	TextWidget* m_DynamicVisibleGlyphsText{ nullptr };

    TextLayoutStyle MakeTextLayout( Unit a_Size, ETextOverflow a_Overflow, TextWrap a_Wrap = TextWrap::NoWrap() ) const
    {
        TextLayoutStyle style{};
        style.Font     = DefaultFont;
        style.Size     = a_Size;
        style.Overflow = a_Overflow;
        style.Wrap     = a_Wrap;
        return style;
    }

    void BuildThemes()
    {
        // Dark
        m_Themes[0] = MakeShared<Theme>( *Themes::Dark() );
        m_Themes[0]->SetColors( {
            { ThemeKey::Color::SliderThumbHover,    Colors::LightBlue  },
            { ThemeKey::Color::SliderThumbPressed,  Colors::AccentBlue },
            { ThemeKey::Color::SliderTrackFill,     Colors::AccentBlue }
        } );
        m_Themes[0]->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = Colors::White } );

        // Light
        m_Themes[1] = MakeShared<Theme>( *Themes::Dark() );
        m_Themes[1]->SetColors( {
            { ThemeKey::Color::FocusOutline,        Colors::DarkBlue   },
            { ThemeKey::Color::SliderTrack,         Colors::Silver     },
            { ThemeKey::Color::SliderTrackFill,     Colors::DarkBlue   },
            { ThemeKey::Color::SliderThumb,         Colors::AccentBlue },
            { ThemeKey::Color::SliderThumbHover,    Colors::Blue       },
            { ThemeKey::Color::SliderThumbPressed,  Colors::DarkBlue   }
        } );
        m_Themes[1]->SetBrushes( {
            { ThemeKey::Brush::PanelNormal,   SolidBrush{ Colors::LightGray  } },
            { ThemeKey::Brush::ButtonNormal,  SolidBrush{ Colors::White      } },
            { ThemeKey::Brush::ButtonHover,   SolidBrush{ Colors::PowderBlue } },
            { ThemeKey::Brush::ButtonPressed, SolidBrush{ Colors::LightBlue  } },
        } );
        m_Themes[1]->SetTextStyle( ThemeKey::TextStyle::Default, TextRenderStyle{ .FillColor = Colors::Surface900 } );

        // Neon
        m_Themes[2] = MakeShared<Theme>( *Themes::Dark() );
        m_Themes[2]->SetColors( {
            { ThemeKey::Color::FocusOutline,        Colors::AccentRose                      },
            { ThemeKey::Color::SliderTrack,         FromColorF32( 0.10f, 0.10f, 0.20f )    },
            { ThemeKey::Color::SliderTrackFill,     Colors::AccentRose                      },
            { ThemeKey::Color::SliderThumb,         Colors::AccentSky                       },
            { ThemeKey::Color::SliderThumbHover,    Colors::LightCyan                       },
            { ThemeKey::Color::SliderThumbPressed,  Colors::AccentRose                      }
        } );
        m_Themes[2]->SetRadii( {
            { ThemeKey::Radii::Panel,        CornerRadius::All( 12_u ) },
            { ThemeKey::Radii::Button,       CornerRadius::All( 10_u ) },
            { ThemeKey::Radii::SliderTrack,  CornerRadius::All(  5_u ) },
            { ThemeKey::Radii::SliderThumb,  CornerRadius::All(  8_u ) }
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
        for ( const auto& [key, value] : m_Themes[3]->GetRadii() )
            m_Themes[3]->SetRadius( key, CornerRadius::None() ); // Override all roundings to sharp corners.

        m_Themes[3]->SetColors( {
            { ThemeKey::Color::SliderTrackFill,    FromColorF32( 0.1f, 0.5f, 0.1f ) },
            { ThemeKey::Color::SliderThumb,        FromColorF32( 0.9f, 0.9f, 0.9f ) },
            { ThemeKey::Color::SliderThumbHover,   FromColorF32( 0.8f, 0.8f, 0.8f ) },
            { ThemeKey::Color::SliderThumbPressed, FromColorF32( 1.f,  1.f,  1.f  ) }
        } );

        m_ActiveTheme = MakeShared<Theme>( *m_Themes[0] );
    }

    void ApplyTheme( size a_ThemeIndex )
    {
        m_ActiveThemeIndex = a_ThemeIndex % 4;
        *m_ActiveTheme = *m_Themes[m_ActiveThemeIndex];

        UpdateStatusText();
    }

    void CreateThemeButton( PanelWidget* a_Parent, const String& a_Label, size a_ThemeIndex )
    {
        Builder<ButtonWidget> builder( m_Scene, a_Parent->GetLayoutID(),
        [this, a_ThemeIndex]( ButtonBaseWidget& )
        {
            ApplyTheme( a_ThemeIndex );
        } );

        builder
            .WithTheme( m_ActiveTheme )
			.WithLayout( LayoutStyle{
                .ChildAlign = EAlign::Center,
                .WidthMode  = ESizing::Fixed,
                .HeightMode = ESizing::Flex, 
                .FixedWidth = 130_u }
		    ).AddChild<TextWidget>( 
                [this, a_Label]( Builder<TextWidget>& b )
                {
                    b.WithTheme( m_ActiveTheme )
                        .WithLayout( LayoutStyle{ .Visibility = EVisibility::HitTestInvisible } );
                }, MakeText( a_Label ), MakeTextLayout( 16_u, ETextOverflow::Clip ) 
            );
    }

    void CreateSliderCard( PanelWidget* a_Parent, const String& a_Label, f32 a_Value, bool a_Vertical )
    {
        PanelWidget* card = m_Scene.CreateWidget<PanelWidget>( a_Parent->GetLayoutID() );
        card->Theme = m_ActiveTheme;
        card->GetLayout()
            .LayoutType( ELayoutType::Vertical )
            .WidthMode( ESizing::Flex )
            .FixedHeight( a_Vertical ? 180_u : 86_u )
            .Padding( Edges::All( 8_u ) )
            .Spacing( 6_u );

        TextWidget* valueText = m_Scene.CreateWidget<TextWidget>( card->GetLayoutID(), MakeText( "" ), MakeTextLayout( 14_u, ETextOverflow::Clip ) );
        valueText->Theme = m_ActiveTheme;
        valueText->GetLayout().HeightMode(ESizing::Content);

        SliderWidget* sliderWidget = m_Scene.CreateWidget<SliderWidget>( card->GetLayoutID(), 0.f, 1.f, a_Value );
        sliderWidget->Theme = m_ActiveTheme;
		LayoutNode& sliderNode = sliderWidget->GetLayout();
        sliderNode
            .WidthMode( ESizing::Flex )
            .FixedHeight( a_Vertical ? 120_u : 28_u );

        if ( a_Vertical )
        {
            sliderWidget->Orientation = EOrient::Vertical;
            sliderNode.FixedWidth( 36_u );
        }

        sliderWidget->Value.Subscribe( [this, valueText, a_Label]( const f32& a_CurrentValue )
        {
            const int percent = static_cast<int>( std::round( a_CurrentValue * 100.f ) );
            valueText->SetText( { std::format( "{}: {}%", a_Label, percent ) } );
        } );
    }

    void UpdateStatusText()
    {
        if ( m_StatusText )
            m_StatusText->SetText( { "Theme applied: " + m_ThemeNames[m_ActiveThemeIndex] + " (use the buttons below to switch styles)" } );
    }
};
