#pragma once
#include "IDemoScene.h"

class FeatureSandboxScene : public IDemoScene
{
public:
    FeatureSandboxScene( FontHandle a_Font, ITextMetrics* a_TextMetrics ) : IDemoScene( a_TextMetrics ), DefaultFont( a_Font ) {}
    ~FeatureSandboxScene() override = default;

    FontHandle DefaultFont;
    WidgetID MainContentArea;
    f32 Time = 0.f;

    void Init() override
    {
        // Root container - deep app background
        WidgetID trueRoot = m_Scene.CreateRootWidget<RectWidget>( Colorsu8::Surface900, "AppBackground" );
        LayoutNode* trueRootNode = m_Scene.Layouts.Get( m_Scene.GetWidget( trueRoot )->GetLayoutID() );
        trueRootNode->Style.LayoutType = ELayoutType::Vertical;
        trueRootNode->Style.Spacing = 10_u;
        trueRootNode->Style.Padding = Edges::Uniform( 10_u );
        trueRootNode->Style.WidthMode = ESizingMode::Flex;
        trueRootNode->Style.HeightMode = ESizingMode::Flex;
    
        // Main panel - slightly lighter surface
        WidgetID root = m_Scene.CreateWidget<RectWidget>( trueRoot, Colorsu8::Surface800, "MainPanel" );
        LayoutNode* rootNode = m_Scene.Layouts.Get( m_Scene.GetWidget( root )->GetLayoutID() );
        rootNode->Style.LayoutType = ELayoutType::Vertical;
        rootNode->Style.Spacing = 10_u;
        rootNode->Style.Padding = Edges::Uniform( 10_u );
        rootNode->Style.WidthMode = ESizingMode::Flex;
        rootNode->Style.HeightMode = ESizingMode::Flex;
        rootNode->Style.IsFocusScope = true;
    
        // ---------------- HEADER BAR (was Red) ----------------
        WidgetID headerBar = m_Scene.CreateWidget<RectWidget>( root, Colorsu8::AccentBlue, "HeaderBar" );
        auto* headerBarNode = m_Scene.Layouts.Get( m_Scene.GetWidget( headerBar )->GetLayoutID() );
    
        headerBarNode->Style.FixedHeight = 100_u;
        headerBarNode->Style.WidthMode = ESizingMode::Flex;
        headerBarNode->Style.HeightMode = ESizingMode::Fixed;
        headerBarNode->Style.Margin = Edges::Uniform( 10_u );
    
        // ---------------- CONTENT ROW (was HBox) ----------------
        WidgetID contentRow = m_Scene.CreateWidget<RectWidget>( root, Colorsu8::Surface700, "ContentRow" );
        auto* contentRowNode = m_Scene.Layouts.Get( m_Scene.GetWidget( contentRow )->GetLayoutID() );
    
        contentRowNode->Style.LayoutType = ELayoutType::Horizontal;
        contentRowNode->Style.Spacing = 0_u;
        contentRowNode->Style.Padding = Edges::Uniform( 10_u );
        contentRowNode->Style.HeightMode = ESizingMode::Fixed;
        contentRowNode->Style.FixedHeight = 150_u;
        contentRowNode->Style.WidthMode = ESizingMode::Flex;
        contentRowNode->Style.Spacing = 20_u;
        contentRowNode->Style.IsFocusScope = true;
    
        // ---------------- MAIN CONTENT AREA ----------------
        {
            MainContentArea = m_Scene.CreateWidget<RectWidget>( contentRow, Colorsu8::Surface600, "MainContentArea" );
            auto* mainContentNode = m_Scene.Layouts.Get( m_Scene.GetWidget( MainContentArea )->GetLayoutID() );
            
            mainContentNode->Style.WidthMode = ESizingMode::Flex;
            mainContentNode->Style.HeightMode = ESizingMode::Flex;
            mainContentNode->Style.FlexGrow = 1.f;
            mainContentNode->Style.PercentWidth = 0.5f;
            mainContentNode->Style.Margin = Edges::Uniform( 10_u );

            TextLayoutStyle layStyle;
            layStyle.Font = DefaultFont;
            layStyle.Size = 16_u;
            layStyle.Wrap = TextWrap::NoWrap();
            layStyle.Overflow = ETextOverflow::Ellipsis;

            TextRenderStyle textStyle;
            textStyle.Color = Colorsu8::AccentRose;

            // Add some text to the footer bar
            WidgetID footerText = m_Scene.CreateWidget<TextWidget>( MainContentArea, 
                "This is the main content area. It can contain the primary information or controls for the application.\n"
                "Hello ifahjkfhaiofhoajfojaofjasojdioajodjaodjoas", 
                layStyle, textStyle
            );
            auto* footerTextNode = m_Scene.Layouts.Get( m_Scene.GetWidget( footerText )->GetLayoutID() );

            footerTextNode->Style.WidthMode = ESizingMode::Flex;
			footerTextNode->Style.FixedWidth = 100_u; // Intentionally small to demonstrate text overflow handling
            footerTextNode->Style.HeightMode = ESizingMode::Content;
            footerTextNode->Style.Padding = Edges::Uniform( 10_u );
        }
    
        // ---------------- SIDEBAR PANEL ----------------
        {
            WidgetID sidebarPanel = m_Scene.CreateWidget<RectWidget>( contentRow, Colorsu8::Surface600, "SidebarPanel" );
            auto* sidebarNode = m_Scene.Layouts.Get( m_Scene.GetWidget( sidebarPanel )->GetLayoutID() );
    
            sidebarNode->Style.LayoutType = ELayoutType::Vertical;
            sidebarNode->Style.Spacing = 10_u;
            sidebarNode->Style.Padding = Edges::Uniform( 10_u );
            sidebarNode->Style.HeightMode = ESizingMode::Flex;
            sidebarNode->Style.WidthMode = ESizingMode::Fixed;
            sidebarNode->Style.FixedWidth = 100_u;
            sidebarNode->Style.ChildAlign = EAlignment::Center;
            sidebarNode->Style.IsFocusScope = true;
    
            // ---------------- STATUS INDICATOR ----------------
            WidgetID statusIndicator = m_Scene.CreateWidget<RectWidget>( sidebarPanel, Colorsu8::AccentEmerald, "StatusIndicator" );
            auto* statusNode = m_Scene.Layouts.Get( m_Scene.GetWidget( statusIndicator )->GetLayoutID() );
    
            statusNode->Style.WidthMode = ESizingMode::Flex;
            statusNode->Style.FixedWidth = 40_u;
            statusNode->Style.HeightMode = ESizingMode::Flex;
            statusNode->Style.FixedHeight = 60_u;
    
            // ---------------- NOTIFICATION DOT ----------------
            WidgetID notificationDot = m_Scene.CreateWidget<RectWidget>( sidebarPanel, Colorsu8::AccentRose, "NotificationDot" );
            auto* notifNode = m_Scene.Layouts.Get( m_Scene.GetWidget( notificationDot )->GetLayoutID() );
    
            notifNode->Style.WidthMode = ESizingMode::Flex;
            notifNode->Style.FixedWidth = 20_u;
            notifNode->Style.HeightMode = ESizingMode::Flex;
            notifNode->Style.FixedHeight = 20_u;
            notifNode->Style.Padding = Edges::Uniform( 10_u );
        }
    
        // ---------------- SECONDARY CONTENT AREA ----------------
        {
            WidgetID secondaryContent = m_Scene.CreateWidget<RectWidget>( contentRow, Colorsu8::Surface600, "SecondaryContentArea" );
            auto* secondaryNode = m_Scene.Layouts.Get( m_Scene.GetWidget( secondaryContent )->GetLayoutID() );
            
            secondaryNode->Style.WidthMode = ESizingMode::Flex;
            secondaryNode->Style.HeightMode = ESizingMode::Flex;
            secondaryNode->Style.PercentWidth = 0.5f;
            secondaryNode->Style.FlexGrow = 1.f;

            // Long wrapping Text
            TextLayoutStyle layStyle;
            layStyle.Font = DefaultFont;
            layStyle.Size = 16_u;
            layStyle.Wrap = TextWrap::WrapWord();
            WidgetID longText = m_Scene.CreateWidget<TextWidget>( secondaryContent, 
                "This is the secondary content area. It can contain supplementary information or controls that support the main content.\n"
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.", 
                layStyle
            );
            auto* longTextNode = m_Scene.Layouts.Get( m_Scene.GetWidget( longText )->GetLayoutID() );
            longTextNode->Style.WidthMode = ESizingMode::Flex;
            longTextNode->Style.HeightMode = ESizingMode::Content;
            longTextNode->Style.Padding = Edges::Uniform( 10_u );
        }
        
    
        // ---------------- FOOTER BAR ----------------
        {
            WidgetID footerBar = m_Scene.CreateWidget<RectWidget>( root, Colorsu8::Surface700, "FooterBar" );
            auto* footerNode = m_Scene.Layouts.Get( m_Scene.GetWidget( footerBar )->GetLayoutID() );

            footerNode->Style.FixedHeight = 120_u;
            footerNode->Style.WidthMode = ESizingMode::Flex;
            footerNode->Style.HeightMode = ESizingMode::Fixed;

            TextLayoutStyle layStyle;
            layStyle.Font = DefaultFont;
            layStyle.Size = 16_u;
            layStyle.Wrap = TextWrap::WrapWord();

            // Add some text to the footer bar
            WidgetID footerText = m_Scene.CreateWidget<TextWidget>( footerBar, 
                "This is the footer bar. It can contain status messages, controls, or other information.\n"
                "VAVAVAVA\n"
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
                "0123456789\n", layStyle
            );
            auto* footerTextNode = m_Scene.Layouts.Get( m_Scene.GetWidget( footerText )->GetLayoutID() );

            footerTextNode->Style.WidthMode = ESizingMode::Content;
            footerTextNode->Style.HeightMode = ESizingMode::Content;
            footerTextNode->Style.Padding = Edges::Uniform( 10_u );
        }
    
        // ---------------- ACCENT SWATCH ROW ----------------
        WidgetID accentSwatchRow = m_Scene.CreateWidget<RectWidget>( root, Colorsu8::Surface800, "AccentSwatchRow" );
        auto* swatchRowNode = m_Scene.Layouts.Get( m_Scene.GetWidget( accentSwatchRow )->GetLayoutID() );
        swatchRowNode->Style.LayoutType = ELayoutType::Horizontal;
        swatchRowNode->Style.Spacing = 20_u;
        swatchRowNode->Style.Padding = Edges::Uniform( 10_u );
        swatchRowNode->Style.HeightMode = ESizingMode::Fixed;
        swatchRowNode->Style.FixedHeight = 200_u;
        swatchRowNode->Style.WidthMode = ESizingMode::Flex;
        swatchRowNode->Style.IsFocusScope = true;
    
        // Five accent swatches: blue, purple, violet, emerald, rose
        constexpr Coloru8 accentSwatches[5] = {
            Colorsu8::AccentBlue,
            Colorsu8::AccentPurple,
            Colorsu8::AccentViolet,
            Colorsu8::AccentEmerald,
            Colorsu8::AccentRose,
        };
    
        // Draw colored circles with increasing radius for each accent color
        for ( int i = 0; i < 5; ++i )
        {
            Unit radius = 30_u + i * 10_u;
            WidgetID swatch = m_Scene.CreateWidget<CircleWidget>( accentSwatchRow, radius, accentSwatches[i] );
            auto* swatchNode = m_Scene.Layouts.Get( m_Scene.GetWidget( swatch )->GetLayoutID() );
            swatchNode->Style.WidthMode = ESizingMode::Fixed;
            swatchNode->Style.FixedWidth = radius * 2.f;
            swatchNode->Style.HeightMode = ESizingMode::Fixed;
            swatchNode->Style.FixedHeight = radius * 2.f;
        }
    
        // Draw colored circle borders in reverse order on top of the swatches to create a layered effect
        for ( int i = 3; i >= 0; --i )
        {
            Unit radius = 30_u + i * 10_u;
            WidgetID swatch = m_Scene.CreateWidget<CircleWidget>( accentSwatchRow, radius, accentSwatches[i], false  );
            auto* swatchNode = m_Scene.Layouts.Get( m_Scene.GetWidget( swatch )->GetLayoutID() );
            swatchNode->Style.WidthMode = ESizingMode::Fixed;
            swatchNode->Style.FixedWidth = radius * 2.f;
            swatchNode->Style.HeightMode = ESizingMode::Fixed;
            swatchNode->Style.FixedHeight = radius * 2.f;
        }
    }

    void OnInputEvent( const InputEvent& a_Event ) override
    {
        if ( a_Event.Device == EDeviceID::Keyboard )
        {
            const ButtonEvent& btnEvent = Get<ButtonEvent>( a_Event.Payload );

            ENavAction navAction = ENavAction::None;
            switch ( btnEvent.Button )
            {
                case EButtonID::KeyUp:    navAction = ENavAction::MoveUp; break;
                case EButtonID::KeyDown:  navAction = ENavAction::MoveDown; break;
                case EButtonID::KeyLeft:  navAction = ENavAction::MoveLeft; break;
                case EButtonID::KeyRight: navAction = ENavAction::MoveRight; break;
                case EButtonID::KeyEnter: navAction = ENavAction::Activate; break;
                case EButtonID::KeyEscape: navAction = ENavAction::Cancel; break;
                default: break; // Unsupported key
            }

            if ( btnEvent.Pressed && navAction != ENavAction::None )
            {
                m_Scene.Navigate( navAction );
            }

            return;
        }

        m_Scene.DispatchInputEvent( a_Event );
    }

    void Update( f32 a_DeltaTime ) override
    {
        Time += a_DeltaTime;

        // Animate green widget's width with a sine wave
        if ( auto* mainContentAreaNode = m_Scene.Layouts.Get( m_Scene.GetWidget( MainContentArea )->GetLayoutID() ) )
        {
			mainContentAreaNode->Style.FlexGrow = 0.51f + 0.5f * std::sin( Time ); // FlexGrow oscillates between 0.01 and 1
            mainContentAreaNode->Layout.IsDirty = true; // Mark layout dirty to trigger recalculation
		}
    }

    void Render( DrawList& a_DrawList ) override
    {
        m_Scene.Render( a_DrawList );
    }

    void Shutdown() override
    {
        //m_Scene.DestroyWidget( m_Scene.RootWidget );
    }
};