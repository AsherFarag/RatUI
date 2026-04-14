#pragma once
#include "IDemoScene.h"

/**
 * @brief Demonstrates all text rendering features supported by RatUI's TextWidget / TextStyle.
 *
 * Sections
 * --------
 *  1. Font Sizes          - 8 px … 40 px
 *  2. Text Colours        - full accent + semantic palette
 *  3. Text Alignment      - Left / Center / Right
 *  4. Text Wrapping       - NoWrap+Ellipsis, WrapWord, WrapChar
 *  5. Text Overflow       - Clip, Ellipsis, MaxLines
 *  6. Text Transform      - None / Uppercase / Lowercase / Capitalize
 *  7. Letter Spacing      - -1 px ... +5 px
 *  8. Line Height         - auto / tight / normal / loose
 *  9. Text Decoration     - Underline, Strikethrough, combined
 * 10. Combined Styles     - multiple properties at once
 * 11. Animated            - container width oscillates -> Ellipsis reflows live
 */
class DynamicTextScene : public IDemoScene
{
public:
    DynamicTextScene( FontHandle a_Font, ITextMetrics* a_TextMetrics )
        : IDemoScene( a_TextMetrics )
        , m_Font( a_Font )
    {}

    ~DynamicTextScene() override = default;

private:

    struct TextStyle
    {
        TextLayoutStyle Layout{};
        TextRenderStyle Render{};
    };

    FontHandle m_Font;
    f32        m_Time{ 0.f };
    WidgetID   m_AnimContainer{};

    // -------------------------------------------------------------------------
    // Convenience accessors
    // -------------------------------------------------------------------------

    LayoutNode* Node( WidgetID id )
    {
        return m_Scene.Layouts.Get( m_Scene.GetWidget( id )->GetLayoutID() );
    }

    // -------------------------------------------------------------------------
    // Style factories
    // -------------------------------------------------------------------------

    TextStyle MakeStyle( f32 sz = 14.f ) const
    {
        TextStyle s;
        s.Layout.Font    = m_Font;
        s.Layout.Size    = sz;
        s.Render.Color   = Colorsu8::TextPrimary;
        s.Layout.Wrap    = TextWrap::NoWrap();
        s.Layout.Overflow = ETextOverflow::Clip;
        return s;
    }

    TextStyle SectionHeadingStyle() const
    {
        TextStyle s = MakeStyle( 9.f );
        s.Render.Color      = Colorsu8::TextSecondary;
        s.Layout.Transform  = ETextTransform::Uppercase;
        return s;
    }

    TextStyle RowLabelStyle() const
    {
        TextStyle s = MakeStyle( 10.f );
        s.Render.Color = Colorsu8::TextDisabled;
        return s;
    }

    // -------------------------------------------------------------------------
    // Layout primitives
    // -------------------------------------------------------------------------

    /// Adds a TextWidget child. Returns its WidgetID.
    WidgetID AddText(
        WidgetID    parent,
        const char* text,
        TextStyle   style,
        ESizingMode wMode = ESizingMode::Flex,
        f32         fixedW = 0.f )
    {
        WidgetID w = m_Scene.CreateWidget<TextWidget>( parent, text, style.Layout, style.Render );
        auto* n = Node( w );
        n->Style.WidthMode  = wMode;
        n->Style.FixedWidth = fixedW;
        n->Style.HeightMode = ESizingMode::Content;
        return w;
    }

    /// Vertical card with heading + divider. Returns card WidgetID.
    WidgetID BeginCard( WidgetID parent, const char* title )
    {
        WidgetID card = m_Scene.CreateWidget<RectWidget>(
            parent, Colorsu8::Surface700, "Card",
            CornerRounding::Uniform( 6_deg )
        );
        {
            auto* n = Node( card );
            n->Style.LayoutType = ELayoutType::Vertical;
            n->Style.Padding    = Edges{ 12.f };
            n->Style.Spacing    = 5.f;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Content;
        }

        AddText( card, title, SectionHeadingStyle(), ESizingMode::Flex );

        WidgetID div = m_Scene.CreateWidget<RectWidget>(
            card, Colorsu8::Surface500, "Div", CornerRounding::None()
        );
        {
            auto* n = Node( div );
            n->Style.WidthMode   = ESizingMode::Flex;
            n->Style.HeightMode  = ESizingMode::Fixed;
            n->Style.FixedHeight = 1.f;
        }

        return card;
    }

    /// Two-column horizontal container inside a vertical parent.
    std::pair<WidgetID, WidgetID> MakeCols( WidgetID parent )
    {
        WidgetID row = m_Scene.CreateWidget<RectWidget>(
            parent, Colorsu8::Surface900, "ColRow"
        );
        {
            auto* n = Node( row );
            n->Style.LayoutType = ELayoutType::Horizontal;
            n->Style.Spacing    = 12.f;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Content;
        }

        auto MakeCol = [&]() -> WidgetID
        {
            WidgetID c = m_Scene.CreateWidget<RectWidget>( row, Colorsu8::Surface900, "Col" );
            auto* n = Node( c );
            n->Style.LayoutType = ELayoutType::Vertical;
            n->Style.Spacing    = 12.f;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.FlexGrow   = 1.f;
            n->Style.HeightMode = ESizingMode::Content;
            return c;
        };

        return { MakeCol(), MakeCol() };
    }

    /// Horizontal row: 112 px label | demo text.  Returns demo text WidgetID.
    WidgetID AddRow(
        WidgetID    card,
        const char* rowLabel,
        const char* demoText,
        TextStyle   style,
        ESizingMode textWMode = ESizingMode::Flex,
        f32         textFixedW = 0.f )
    {
        WidgetID row = m_Scene.CreateWidget<RectWidget>(
            card, Colorsu8::Surface800, "Row"
        );
        {
            auto* n = Node( row );
            n->Style.LayoutType = ELayoutType::Horizontal;
            n->Style.Spacing    = 8.f;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Content;
        }

        // Label column (fixed width)
        TextStyle lblStyle = RowLabelStyle();
        WidgetID lbl = m_Scene.CreateWidget<TextWidget>( row, rowLabel, lblStyle.Layout, lblStyle.Render );
        {
            auto* n = Node( lbl );
            n->Style.Padding    = Edges{ 4.f };
            n->Style.WidthMode  = ESizingMode::Fixed;
            n->Style.FixedWidth = 112.f;
            n->Style.HeightMode = ESizingMode::Content;
        }

        return AddText( row, demoText, style, textWMode, textFixedW );
    }

    // =========================================================================
    // Section builders
    // =========================================================================

    void BuildFontSizes( WidgetID parent )
    {
        WidgetID card = BeginCard( parent, "Font Sizes" );

        constexpr struct { f32 sz; const char* label; } kSizes[] = {
            {  8.f, "8 px - caption"  },
            { 11.f, "11 px - small"   },
            { 13.f, "13 px - ui"      },
            { 16.f, "16 px - body"    },
            { 20.f, "20 px - lead"    },
            { 28.f, "28 px - h3"      },
            { 40.f, "40 px - h1"      },
        };
        for ( auto& e : kSizes )
            AddRow( card, e.label, "The quick brown fox", MakeStyle( e.sz ) );
    }

    void BuildColors( WidgetID parent )
    {
        WidgetID card = BeginCard( parent, "Text Colours" );

        constexpr struct { Coloru8 col; const char* label; } kCols[] = {
            { Colorsu8::TextPrimary,   "Primary"   },
            { Colorsu8::TextSecondary, "Secondary" },
            { Colorsu8::TextDisabled,  "Disabled"  },
            { Colorsu8::AccentBlue,    "Blue"      },
            { Colorsu8::AccentEmerald, "Emerald"   },
            { Colorsu8::AccentRose,    "Rose"      },
            { Colorsu8::AccentAmber,   "Amber"     },
            { Colorsu8::AccentViolet,  "Violet"    },
            { Colorsu8::AccentSky,     "Sky"       },
            { Colorsu8::AccentPurple,  "Purple"    },
        };
        for ( auto& e : kCols )
        {
            TextStyle ts = MakeStyle( 14.f );
            ts.Render.Color = e.col;
            AddRow( card, e.label, "Aa  The quick brown fox", ts );
        }
    }

    void BuildAlignment( WidgetID parent )
    {
        WidgetID card = BeginCard( parent, "Text Alignment" );
        static const char* kSent = "Pack my box with five dozen liquor jugs.";

        constexpr struct { ETextAlign align; const char* label; } kAligns[] = {
            { ETextAlign::Left,   "Left"   },
            { ETextAlign::Center, "Center" },
            { ETextAlign::Right,  "Right"  },
        };

        for ( auto& e : kAligns )
        {
            TextStyle ts = MakeStyle( 14.f );
            ts.Render.Align = e.align;
            ts.Layout.Wrap = TextWrap::WrapWord();
            AddRow( card, e.label, kSent, ts, ESizingMode::Flex );
        }
    }

    void BuildWrapping( WidgetID parent )
    {
        WidgetID card = BeginCard( parent, "Text Wrapping" );
        static const char* kLong =
            "Sphinx of black quartz, judge my vow. "
            "The five boxing wizards jump quickly. "
            "Amazingly few discotheques provide jukeboxes.";

        {
            TextStyle ts = MakeStyle( 13.f );
            ts.Layout.Wrap = TextWrap::NoWrap();
            ts.Layout.Overflow = ETextOverflow::Ellipsis;
            AddRow( card, "NoWrap+Ellipsis", kLong, ts, ESizingMode::Flex );
        }
        {
            TextStyle ts = MakeStyle( 13.f );
            ts.Layout.Wrap = TextWrap::WrapWord();
            AddRow( card, "WrapWord", kLong, ts, ESizingMode::Flex );
        }
        {
            TextStyle ts = MakeStyle( 13.f );
            ts.Layout.Wrap = TextWrap::WrapChar();
            AddRow( card, "WrapChar", kLong, ts, ESizingMode::Flex );
        }
    }

    void BuildOverflow( WidgetID parent )
    {
        WidgetID card = BeginCard( parent, "Text Overflow  (fixed 260 px box)" );
        static const char* kLong =
            "The five boxing wizards jump quickly over the lazy dog. "
            "Pack my box with five dozen liquor jugs.";

        auto add = [&]( const char* label, TextWrap wrap, ETextOverflow overflow, u16 maxLines )
        {
            TextStyle ts = MakeStyle( 13.f );
            ts.Layout.Wrap = wrap;
            ts.Layout.Overflow = overflow;
            ts.Layout.MaxLines = maxLines;
            AddRow( card, label, kLong, ts, ESizingMode::Fixed, 260.f );
        };

        add( "Clip",         TextWrap::NoWrap(),   ETextOverflow::Clip,     0 );
        add( "Fade",         TextWrap::NoWrap(),   ETextOverflow::Fade,     0 );
        add( "Ellipsis",     TextWrap::NoWrap(),   ETextOverflow::Ellipsis, 0 );
        add( "MaxLines = 2", TextWrap::WrapWord(), ETextOverflow::Ellipsis, 2 );
        add( "MaxLines = 1", TextWrap::WrapWord(), ETextOverflow::Ellipsis, 1 );
    }

    void BuildTransform( WidgetID parent )
    {
        WidgetID card = BeginCard( parent, "Text Transform" );
        static const char* kRaw = "the Quick Brown Fox JUMPS Over The lazy Dog";

        constexpr struct { ETextTransform xf; const char* label; } kXforms[] = {
            { ETextTransform::None,       "None"       },
            { ETextTransform::Uppercase,  "Uppercase"  },
            { ETextTransform::Lowercase,  "Lowercase"  },
            { ETextTransform::Capitalize, "Capitalize" },
        };
        for ( auto& e : kXforms )
        {
            TextStyle ts = MakeStyle( 14.f );
            ts.Layout.Transform = e.xf;
            AddRow( card, e.label, kRaw, ts );
        }
    }

    void BuildLetterSpacing( WidgetID parent )
    {
        WidgetID card = BeginCard( parent, "Letter Spacing" );

        constexpr struct { f32 sp; const char* label; } kSpacings[] = {
            { -1.0f, "-1 px  tight"  },
            {  0.0f, " 0 px  normal" },
            {  1.0f, "+1 px"         },
            {  2.5f, "+2.5 px"       },
            {  5.0f, "+5 px  wide"   },
        };
        for ( auto& e : kSpacings )
        {
            TextStyle ts = MakeStyle( 14.f );
            ts.Layout.LetterSpacing = e.sp;
            ts.Layout.Wrap = TextWrap::NoWrap();
            ts.Layout.Overflow = ETextOverflow::Clip;
            AddRow( card, e.label, "LOREM IPSUM SIT AMET", ts );
        }
    }

    void BuildLineHeight( WidgetID parent )
    {
        WidgetID card = BeginCard( parent, "Line Height  (WrapWord)" );
        static const char* kPara =
            "Kernels of corn gleam gold.\n"
            "Waves break; seagulls call out.\n"
            "Dusk paints the harbour crimson.";

        constexpr struct { f32 lh; const char* label; } kLH[] = {
            {  0.f, "Auto"          },
            { 12.f, "12 px  tight"  },
            { 20.f, "20 px  normal" },
            { 30.f, "30 px  loose"  },
        };

        for ( auto& e : kLH )
        {
            TextStyle ts = MakeStyle( 13.f );
            ts.Layout.LineHeight = e.lh;
            ts.Layout.Wrap = TextWrap::WrapWord();
            AddRow( card, e.label, kPara, ts, ESizingMode::Flex );
        }
    }

    void BuildDecoration( WidgetID parent )
    {
        WidgetID card = BeginCard( parent, "Text Decoration" );
        static const char* kSamp = "The quick brown fox jumps over the lazy dog";

        struct { bool ul; bool st; const char* label; } kDec[] = {
            { false, false, "None"          },
            { true,  false, "Underline"     },
            { false, true,  "Strikethrough" },
            { true,  true,  "Both"          },
        };
        for ( auto& e : kDec )
        {
            TextStyle ts = MakeStyle( 14.f );
            ts.Render.Underline     = e.ul;
            ts.Render.Strikethrough = e.st;
            AddRow( card, e.label, kSamp, ts );
        }
    }

    void BuildCombined( WidgetID parent )
    {
        WidgetID card = BeginCard( parent, "Combined Styles" );
        static const char* kLong =
            "Amazingly few discotheques provide jukeboxes. "
            "How vexingly quick daft zebras jump!";

        // Violet underline + WrapWord
        {
            TextStyle ts             = MakeStyle( 14.f );
            ts.Render.Color          = Colorsu8::AccentViolet;
            ts.Render.Underline      = true;
            ts.Render.FadePercentage = 0.5f; // 50% faded underline
            ts.Layout.Wrap           = TextWrap::NoWrap();
            ts.Layout.Overflow       = ETextOverflow::Fade;
            AddRow( card, "Fade+UL", kLong, ts, ESizingMode::Fixed, 200.f );
        }
        // Rose strikethrough + uppercase + WrapWord
        {
            TextStyle ts             = MakeStyle( 14.f );
            ts.Render.Color          = Colorsu8::AccentRose;
            ts.Render.Strikethrough  = true;
            ts.Layout.Transform      = ETextTransform::Uppercase;
            ts.Layout.Wrap           = TextWrap::NoWrap();
            ts.Layout.Overflow       = ETextOverflow::Fade;
            ts.Render.FadePercentage = 0.5f; // 50% faded strikethrough
            AddRow( card, "Fade+ST+UC", kLong, ts, ESizingMode::Fixed, 200.f );
        }
        // Amber wide spacing + capitalize + ellipsis
        {
            TextStyle ts            = MakeStyle( 13.f );
            ts.Render.Color         = Colorsu8::AccentAmber;
            ts.Layout.LetterSpacing = 3.f;
            ts.Layout.Transform     = ETextTransform::Capitalize;
            ts.Layout.Wrap          = TextWrap::NoWrap();
            ts.Layout.Overflow      = ETextOverflow::Ellipsis;
            AddRow( card, "Wide+Cap", "the quick brown fox jumps over", ts, ESizingMode::Flex );
        }
        // Sky center-aligned large display
        {
            TextStyle ts         = MakeStyle( 22.f );
            ts.Render.Color      = Colorsu8::AccentSky;
            ts.Render.Align      = ETextAlign::Center;
            ts.Layout.Wrap       = TextWrap::WrapWord();
            ts.Layout.LineHeight = 30.f;
            AddRow( card, "Center 22px", "The beauty of typography lives in every detail.", ts, ESizingMode::Flex );
        }
    }

    void BuildAnimated( WidgetID parent )
    {
        WidgetID card = BeginCard(
            parent,
            "Animated  —  container width oscillates, Ellipsis reflows live"
        );

        // Row container
        WidgetID row = m_Scene.CreateWidget<RectWidget>( card, Colorsu8::Transparent, "ARow" );
        {
            auto* n = Node( row );
            n->Style.LayoutType = ELayoutType::Horizontal;
            n->Style.Spacing    = 8.f;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Content;
        }

        // Label
		const TextStyle lblStyle = RowLabelStyle();
		WidgetID lbl = m_Scene.CreateWidget<TextWidget>( row, "Ellipsis", lblStyle.Layout, lblStyle.Render );
        {
            auto* n = Node( lbl );
            n->Style.Padding = Edges{ 4.f };
            n->Style.WidthMode  = ESizingMode::Fixed;
            n->Style.FixedWidth = 112.f;
            n->Style.HeightMode = ESizingMode::Content;
        }

        // Animated box
        m_AnimContainer = m_Scene.CreateWidget<RectWidget>(
            row, Colorsu8::Surface600, "AnimBox",
            CornerRounding::Uniform( 4_deg )
        );
        {
            auto* n = Node( m_AnimContainer );
            n->Style.WidthMode  = ESizingMode::Fixed;
            n->Style.FixedWidth = 400.f;
            n->Style.HeightMode = ESizingMode::Content;
            n->Style.LayoutType = ELayoutType::Horizontal;
            n->Style.Padding    = Edges{ 4.f };
            n->Style.Spacing    = 4.f;
        }

        TextStyle ts       = MakeStyle( 13.f );
        ts.Render.Color    = Colorsu8::AccentBlue;
        ts.Layout.Wrap     = TextWrap::NoWrap();
        ts.Layout.Overflow = ETextOverflow::Ellipsis;

        {
            WidgetID txt = m_Scene.CreateWidget<TextWidget>(
            m_AnimContainer,
            "The quick brown fox jumps over the lazy dog.",
			ts.Layout, ts.Render
            );
            {
                auto* n = Node( txt );
                n->Style.WidthMode  = ESizingMode::Flex;
                n->Style.HeightMode = ESizingMode::Content;
                n->Style.FlexGrow = 1.f;
            }
        }
        
        {
            ts.Render.Color    = Colorsu8::AccentEmerald;
            WidgetID txt = m_Scene.CreateWidget<TextWidget>(
            m_AnimContainer,
            "Amazingly few discotheques provide jukeboxes.",
			ts.Layout, ts.Render
            );
            {
                auto* n = Node( txt );
                n->Style.WidthMode  = ESizingMode::Flex;
                n->Style.HeightMode = ESizingMode::Content;
                n->Style.FlexGrow = 2.f;
            }
        }

    }

    // =========================================================================
public:

    void Init() override
    {
        // Root
        WidgetID root = m_Scene.CreateRootWidget<RectWidget>(
            Colorsu8::Surface900, "Root", CornerRounding::None()
        );
        {
            auto* n = Node( root );
            n->Style.LayoutType = ELayoutType::Vertical;
            n->Style.Spacing    = 0.f;
            n->Style.Padding    = Edges{ 0.f };
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Flex;
        }

        // Title bar
        {
            WidgetID bar = m_Scene.CreateWidget<RectWidget>(
                root, Colorsu8::Surface800, "TitleBar", CornerRounding::None()
            );
            auto* n = Node( bar );
            n->Style.LayoutType = ELayoutType::Vertical;
            n->Style.Padding    = Edges{ 14.f, 28.f, 10.f, 28.f };
            n->Style.Spacing    = 4.f;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Content;

            {
                TextStyle ts = MakeStyle( 22.f );
                ts.Render.Color = Colorsu8::TextPrimary;
                AddText( bar, "Text Feature Showcase", ts, ESizingMode::Flex );
            }
            {
                TextStyle ts = MakeStyle( 11.f );
                ts.Render.Color   = Colorsu8::TextSecondary;
                ts.Layout.Wrap = TextWrap::NoWrap();
                ts.Layout.Overflow = ETextOverflow::Clip;
                AddText( bar,
                    "Sizes  |  Colours  |  Alignment  |  Wrapping  |  Overflow  |"
                    "  Transform  |  Spacing  |  LineHeight  |  Decoration  |  Animation",
                    ts, ESizingMode::Flex );
            }
        }

        // Scrollable / content column
        WidgetID content = m_Scene.CreateWidget<RectWidget>(
            root, Colorsu8::Surface900, "Content"
        );
        {
            auto* n = Node( content );
            n->Style.LayoutType = ELayoutType::Vertical;
            n->Style.Padding    = Edges{ 16.f, 24.f, 24.f, 24.f };
            n->Style.Spacing    = 12.f;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Content;
        }

        // Layout: 2-column grid
        { auto [l, r] = MakeCols( content ); BuildFontSizes( l );      BuildColors( r );        }
        { auto [l, r] = MakeCols( content ); BuildAlignment( l );      BuildTransform( r );     }
        { auto [l, r] = MakeCols( content ); BuildWrapping( l );       BuildOverflow( r );      }
        { auto [l, r] = MakeCols( content ); BuildLetterSpacing( l );  BuildLineHeight( r );    }
        { auto [l, r] = MakeCols( content ); BuildDecoration( l );     BuildCombined( r );      }

        BuildAnimated( content );
    }

    void OnInputEvent( const InputEvent& a_Event ) override
    {
        m_Scene.DispatchInputEvent( a_Event );
    }

    void Update( f32 a_DeltaTime ) override
    {
        m_Time += a_DeltaTime;

        // Oscillate animated container width: 80 px -> 500 px
        if ( IWidget* w = m_Scene.GetWidget( m_AnimContainer ) )
        {
            if ( LayoutNode* n = m_Scene.Layouts.Get( w->GetLayoutID() ) )
            {
                const f32 t = ( std::sin( m_Time * 0.6f ) + 1.f ) * 0.5f;
                n->Style.FixedWidth = 80.f + t * 720.f;
                n->Layout.IsDirty   = true;
            }
        }
    }

    void Render( DrawList& a_DrawList ) override
    {
        m_Scene.Render( a_DrawList );
    }

    void Shutdown() override {}
};
