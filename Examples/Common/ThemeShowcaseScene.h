#pragma once
#include "IDemoScene.h"
#include <RatUI/Widget/SliderWidget.h>
#include <RatUI/Widget/InputTextWidget.h>
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
        GetNode( root )->Style
            .SetLayoutType( ELayoutType::Vertical )
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Flex )
            .SetPadding( Edges::Uniform( 16_u ) )
            .SetSpacing( 12_u )
            .SetFocusScope( true );

        TextWidget* title = m_Scene.CreateWidget<TextWidget>( root->GetLayoutID(), MakeText( "Theme Showcase" ), MakeTextLayout( 30_u, ETextOverflow::Clip ) );
        title->Theme = m_ActiveTheme;
        GetNode( title )->Style.HeightMode = ESizingMode::Content;

        m_StatusText = m_Scene.CreateWidget<TextWidget>( root->GetLayoutID(), MakeText( "" ), MakeTextLayout( 16_u, ETextOverflow::Clip ) );
        m_StatusText->Theme = m_ActiveTheme;
        GetNode( m_StatusText )->Style.HeightMode = ESizingMode::Content;

        PanelWidget* themeButtonRow = m_Scene.CreateWidget<PanelWidget>( root->GetLayoutID() );
		themeButtonRow->Theme = m_ActiveTheme;
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

        PanelWidget* contentRow = m_Scene.CreateWidget<PanelWidget>( root->GetLayoutID() );
        contentRow->Theme = m_ActiveTheme;
        GetNode( contentRow )->Style
            .SetLayoutType( ELayoutType::Horizontal )
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Flex )
            .SetPadding( Edges::Uniform( 12_u ) )
            .SetSpacing( 12_u )
            .SetFocusScope( true );

        PanelWidget* controlsPanel = m_Scene.CreateWidget<PanelWidget>( contentRow->GetLayoutID() );
        controlsPanel->Theme = m_ActiveTheme;
        GetNode( controlsPanel )->Style
            .SetLayoutType( ELayoutType::Vertical )
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Flex )
            .SetPadding( Edges::Uniform( 12_u ) )
            .SetSpacing( 10_u )
            .SetFlexGrow( 1.f )
            .SetFocusScope( true );

        PanelWidget* previewPanel = m_Scene.CreateWidget<PanelWidget>( contentRow->GetLayoutID() );
        previewPanel->Theme = m_ActiveTheme;
        GetNode( previewPanel )->Style
            .SetLayoutType( ELayoutType::Vertical )
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Flex )
            .SetPadding( Edges::Uniform( 12_u ) )
            .SetSpacing( 10_u )
            .SetFlexGrow( 1.f )
            .SetFocusScope( true );

        TextWidget* controlsTitle = m_Scene.CreateWidget<TextWidget>( controlsPanel->GetLayoutID(), MakeText( "Controls" ), MakeTextLayout( 20_u, ETextOverflow::Clip ) );
        controlsTitle->Theme = m_ActiveTheme;
        GetNode( controlsTitle )->Style.HeightMode = ESizingMode::Content;

        CreateSliderCard( controlsPanel, "Master Volume",    0.65f, false );
        CreateSliderCard( controlsPanel, "Accent Strength",  0.30f, false );
        CreateSliderCard( controlsPanel, "Vertical Mix",     0.45f, true  );

        TextWidget* previewTitle = m_Scene.CreateWidget<TextWidget>( previewPanel->GetLayoutID(), MakeText( "Preview" ), MakeTextLayout( 20_u, ETextOverflow::Clip ) );
        previewTitle->Theme = m_ActiveTheme;
        GetNode( previewTitle )->Style.HeightMode = ESizingMode::Content;

        PanelWidget* previewCard = m_Scene.CreateWidget<PanelWidget>( previewPanel->GetLayoutID() );
        previewCard->Theme = m_ActiveTheme;
        GetNode( previewCard )->Style
            .SetLayoutType( ELayoutType::Vertical )
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Flex )
            .SetPadding( Edges::Uniform( 10_u ) )
            .SetSpacing( 8_u );

        TextWidget* previewText = m_Scene.CreateWidget<TextWidget>(
            previewCard->GetLayoutID(),
            MakeText( "This panel uses the active theme for panel fills, text color, button states, and slider visuals." ),
            MakeTextLayout( 16_u, ETextOverflow::Fade, TextWrap::WrapWord() ) );
        previewText->Theme = m_ActiveTheme;
        GetNode( previewText )->Style
            .SetWidthMode( ESizingMode::Flex )
            .SetHeightMode( ESizingMode::Content );

        // Add TextWidget with vertical overflow to demonstrate text overflow handling in the active theme.
        // This will intentionally overflow to show the fade effect.
        {
            TextWidget* overflowText = m_Scene.CreateWidget<TextWidget>(
                previewCard->GetLayoutID(),
                MakeText( "This is an example of a long text string that will exceed the width of the container and demonstrate how the active theme handles text overflow with a fade effect." ),
                MakeTextLayout( 16_u, ETextOverflow::Fade, TextWrap::WrapWord() ) );
            overflowText->Theme = m_ActiveTheme;
            GetNode( overflowText )->Style
                .SetWidthMode( ESizingMode::Fixed )
                .SetFixedWidth( 100_u )
                .SetHeightMode( ESizingMode::Fixed )
                .SetFixedHeight( 100_u );
        }

        ButtonWidget* actionButton = m_Scene.CreateWidget<ButtonWidget>( previewCard->GetLayoutID(),
			[this]( ButtonBaseWidget& )
            {
                if ( m_StatusText )
                    m_StatusText->SetText( { "Theme applied: " + m_ThemeNames[m_ActiveThemeIndex] + " (preview action clicked)" } );
            } );
        actionButton->Theme = m_ActiveTheme;
        GetNode( actionButton )->Style
            .SetWidthMode( ESizingMode::Fixed )
            .SetFixedWidth( 280_u )
            .SetHeightMode( ESizingMode::Fixed )
            .SetFixedHeight( 40_u )
            .SetChildAlign( EAlignment::Center );

        TextWidget* actionButtonText = m_Scene.CreateWidget<TextWidget>( actionButton->GetLayoutID(), MakeText( "Preview Button" ), MakeTextLayout( 16_u, ETextOverflow::Clip ) );
        actionButtonText->Theme = m_ActiveTheme;
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
        }

        m_Scene.DispatchInputEvent( a_Event );
    }

	void Render( DrawList& a_DrawList, f32 a_DeltaSeconds ) override
    {
        m_Scene.Render( a_DrawList, a_DeltaSeconds );
    }

private:
    FontHandle    DefaultFont;
    Shared<Theme> m_Themes[4];
    Shared<Theme> m_ActiveTheme;
    size          m_ActiveThemeIndex{ 0 };

    std::array<String, 4> m_ThemeNames{ "Dark", "Light", "Neon", "Minecraft" };
    TextWidget* m_StatusText{ nullptr }; ///< Cached pointer — valid for the lifetime of the scene.

    TextLayoutStyle MakeTextLayout( Unit a_Size, ETextOverflow a_Overflow, TextWrap a_Wrap = TextWrap::NoWrap() ) const
    {
        TextLayoutStyle style{};
        style.Font     = DefaultFont;
        style.Size     = a_Size;
        style.Overflow = a_Overflow;
        style.Wrap     = a_Wrap;
        return style;
    }

    LayoutNode* GetNode( IWidget* a_Widget )
    {
        return a_Widget ? m_Scene.Layouts.Get( a_Widget->GetLayoutID() ) : nullptr;
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
        m_Themes[2]->SetRoundings( {
            { ThemeKey::Rounding::Panel,        CornerRounding::Uniform( 12_u ) },
            { ThemeKey::Rounding::Button,       CornerRounding::Uniform( 10_u ) },
            { ThemeKey::Rounding::SliderTrack,  CornerRounding::Uniform(  5_u ) },
            { ThemeKey::Rounding::SliderThumb,  CornerRounding::Uniform(  8_u ) }
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
            m_Themes[3]->SetRounding( key, CornerRounding::None() ); // Override all roundings to sharp corners.

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
            .WithLayout( LayoutStyle{}
                .WithFixedWidth( 130_u )
                .SetHeightMode( ESizingMode::Flex )
                .SetChildAlign( EAlignment::Center )
		    ).AddChild<TextWidget>( 
                [this, a_Label]( Builder<TextWidget>& b )
                {
                    b.WithTheme( m_ActiveTheme )
                     .WithLayout( LayoutStyle{}.SetVisibility( EVisibility::HitTestInvisible ) );
                }, MakeText( a_Label ), MakeTextLayout( 16_u, ETextOverflow::Clip ) 
            );
    }

    void CreateSliderCard( PanelWidget* a_Parent, const String& a_Label, f32 a_Value, bool a_Vertical )
    {
        PanelWidget* card = m_Scene.CreateWidget<PanelWidget>( a_Parent->GetLayoutID() );
        card->Theme = m_ActiveTheme;
        LayoutNode* cardNode = GetNode( card );
        cardNode->Style.LayoutType  = ELayoutType::Vertical;
        cardNode->Style.WidthMode   = ESizingMode::Flex;
        cardNode->Style.HeightMode  = ESizingMode::Fixed;
        cardNode->Style.FixedHeight = a_Vertical ? 180_u : 86_u;
        cardNode->Style.Padding     = Edges::Uniform( 8_u );
        cardNode->Style.Spacing     = 6_u;

        TextWidget* valueText = m_Scene.CreateWidget<TextWidget>( card->GetLayoutID(), MakeText( "" ), MakeTextLayout( 14_u, ETextOverflow::Clip ) );
        valueText->Theme = m_ActiveTheme;
        GetNode( valueText )->Style.HeightMode = ESizingMode::Content;

        SliderWidget* sliderWidget = m_Scene.CreateWidget<SliderWidget>( card->GetLayoutID(), 0.f, 1.f, a_Value );
        sliderWidget->Theme = m_ActiveTheme;
        LayoutNode* sliderNode = GetNode( sliderWidget );
        sliderNode->Style.WidthMode   = ESizingMode::Flex;
        sliderNode->Style.HeightMode  = ESizingMode::Fixed;
        sliderNode->Style.FixedHeight = a_Vertical ? 120_u : 28_u;

        if ( a_Vertical )
        {
            sliderWidget->Orientation     = EOrientation::Vertical;
            sliderNode->Style.FixedWidth  = 36_u;
            sliderNode->Style.WidthMode   = ESizingMode::Fixed;
        }

        sliderWidget->Value.Subscribe( [this, valueText, a_Label]( const f32& a_CurrentValue )
        {
            const int percent = static_cast<int>( std::round( a_CurrentValue * 100.f ) );
            valueText->SetText( { a_Label + ": " + std::to_string( percent ) + "%" } );
        } );
    }

    void UpdateStatusText()
    {
        if ( m_StatusText )
            m_StatusText->SetText( { "Theme applied: " + m_ThemeNames[m_ActiveThemeIndex] + " (use the buttons below to switch styles)" } );
    }
};
