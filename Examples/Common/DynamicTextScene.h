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

    TextStyle MakeStyle( Unit sz = 14_u ) const
    {
        TextStyle s;
        s.Layout.Font    = m_Font;
        s.Layout.Size    = sz;
        s.Render.FillColor   = Colors::TextPrimary;
        s.Layout.Wrap    = TextWrap::NoWrap();
        s.Layout.Overflow = ETextOverflow::Clip;
        return s;
    }

    TextStyle SectionHeadingStyle() const
    {
        TextStyle s = MakeStyle( 9_u );
        s.Render.FillColor      = Colors::TextSecondary;
        s.Layout.Transform  = ETextTransform::Uppercase;
        return s;
    }

    TextStyle RowLabelStyle() const
    {
        TextStyle s = MakeStyle( 10_u );
        s.Render.FillColor = Colors::TextDisabled;
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
        Unit        fixedW = 0_u )
    {
        WidgetID w = m_Scene.CreateWidget<TextWidget>( parent, Shared<Theme>{}, text, style.Layout, style.Render );
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
            parent, Colors::Surface700, "Card",
            CornerRounding::Uniform( 6_u )
        );
        {
            auto* n = Node( card );
            n->Style.LayoutType = ELayoutType::Vertical;
            n->Style.Padding    = Edges::Uniform( 12_u );
            n->Style.Spacing    = 5_u;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Content;
        }

        AddText( card, title, SectionHeadingStyle(), ESizingMode::Flex );

        WidgetID div = m_Scene.CreateWidget<RectWidget>(
            card, Colors::Surface500, "Div", CornerRounding::None()
        );
        {
            auto* n = Node( div );
            n->Style.WidthMode   = ESizingMode::Flex;
            n->Style.HeightMode  = ESizingMode::Fixed;
            n->Style.FixedHeight = 1_u;
        }

        return card;
    }

    struct ColumnSet3
    {
        WidgetID A{};
        WidgetID B{};
        WidgetID C{};
    };

    /// Three-column horizontal container inside a vertical parent.
    ColumnSet3 MakeCols( WidgetID parent )
    {
        WidgetID row = m_Scene.CreateWidget<RectWidget>(
            parent, Colors::Surface900, "ColRow"
        );
        {
            auto* n = Node( row );
            n->Style.LayoutType = ELayoutType::Horizontal;
            n->Style.Spacing    = 12_u;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Content;
        }

        auto MakeCol = [&]() -> WidgetID
        {
            WidgetID c = m_Scene.CreateWidget<RectWidget>( row, Colors::Surface900, "Col" );
            auto* n = Node( c );
            n->Style.LayoutType = ELayoutType::Vertical;
            n->Style.Spacing    = 12_u;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.FlexGrow   = 1.f;
            n->Style.HeightMode = ESizingMode::Content;
            return c;
        };

        return { MakeCol(), MakeCol(), MakeCol() };
    }

    /// Horizontal row: 112 px label | demo text.  Returns demo text WidgetID.
    WidgetID AddRow(
        WidgetID    card,
        const char* rowLabel,
        const char* demoText,
        TextStyle   style,
        ESizingMode textWMode = ESizingMode::Flex,
        Unit        textFixedW = 0_u )
    {
        WidgetID row = m_Scene.CreateWidget<RectWidget>(
            card, Colors::Surface800, "Row"
        );
        {
            auto* n = Node( row );
            n->Style.LayoutType = ELayoutType::Horizontal;
            n->Style.Spacing    = 8_u;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Content;
        }

        // Label column (fixed width)
        TextStyle lblStyle = RowLabelStyle();
        WidgetID lbl = m_Scene.CreateWidget<TextWidget>( row, Shared<Theme>{}, rowLabel, lblStyle.Layout, lblStyle.Render );
        {
            auto* n = Node( lbl );
            n->Style.Padding    = Edges::Uniform( 4_u );
            n->Style.WidthMode  = ESizingMode::Fixed;
            n->Style.FixedWidth = 112_u;
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

        constexpr struct { Unit sz; const char* label; } kSizes[] = {
            {  8_u, "8 px - caption"  },
            { 11_u, "11 px - small"   },
            { 13_u, "13 px - ui"      },
            { 16_u, "16 px - body"    },
            { 20_u, "20 px - lead"    },
            { 28_u, "28 px - h3"      },
            { 40_u, "40 px - h1"      },
        };
        for ( auto& e : kSizes )
            AddRow( card, e.label, "The quick brown fox", MakeStyle( e.sz ) );
    }

    void BuildColors( WidgetID parent )
    {
        WidgetID card = BeginCard( parent, "Text Colours" );

        constexpr struct { Color col; const char* label; } kCols[] = {
            { Colors::TextPrimary,   "Primary"   },
            { Colors::TextSecondary, "Secondary" },
            { Colors::TextDisabled,  "Disabled"  },
            { Colors::AccentBlue,    "Blue"      },
            { Colors::AccentEmerald, "Emerald"   },
            { Colors::AccentRose,    "Rose"      },
            { Colors::AccentAmber,   "Amber"     },
            { Colors::AccentViolet,  "Violet"    },
            { Colors::AccentSky,     "Sky"       },
            { Colors::AccentPurple,  "Purple"    },
        };
        for ( auto& e : kCols )
        {
            TextStyle ts = MakeStyle( 14_u );
            ts.Render.FillColor = e.col;
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
            TextStyle ts = MakeStyle( 14_u );
            ts.Render.Align = e.align;
            ts.Layout.Wrap = TextWrap::WrapWord();
            AddRow( card, e.label, kSent, ts, ESizingMode::Flex );
        }

        constexpr struct { ETextBaseline baseline; const char* label; } kBaselines[] = {
            { ETextBaseline::Alphabetic, "Alphabetic" },
            { ETextBaseline::Top,        "Top"        },
            { ETextBaseline::Middle,     "Middle"     },
            { ETextBaseline::Bottom,     "Bottom"     },
            { ETextBaseline::Hanging,    "Hanging"    },
        };

        WidgetID row = m_Scene.CreateWidget<RectWidget>( card, Colors::Surface800, "BaselineRow" );
        {
            auto* n = Node( row );
            n->Style.LayoutType = ELayoutType::Horizontal;
            n->Style.Spacing    = 8_u;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Content;
        }

        TextStyle lblStyle = RowLabelStyle();
        WidgetID lbl = m_Scene.CreateWidget<TextWidget>( row, Shared<Theme>{}, "Baseline", lblStyle.Layout, lblStyle.Render );
        {
            auto* n = Node( lbl );
            n->Style.Padding    = Edges::Uniform( 4_u );
            n->Style.WidthMode  = ESizingMode::Fixed;
            n->Style.FixedWidth = 112_u;
            n->Style.HeightMode = ESizingMode::Content;
        }

        WidgetID box = m_Scene.CreateWidget<RectWidget>(
            row, Colors::Transparent, "BaselineBox", CornerRounding::Uniform( 4_u )
        );
        {
            auto* n = Node( box );
            n->Style.Padding    = Edges::Uniform( 4_u );
            n->Style.LayoutType = ELayoutType::Horizontal;
            n->Style.Spacing    = 4_u;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Fixed;
            n->Style.FixedHeight = 40_u;
        }

        TextStyle ts = MakeStyle( 14_u );
        ts.Render.FillColor    = Colors::AccentSky;
        ts.Render.FadePercentage = 0.0f;
        ts.Layout.Wrap     = TextWrap::NoWrap();
        ts.Layout.Overflow = ETextOverflow::Clip;

        for ( const auto& e : kBaselines )
        {
            ts.Render.Baseline = e.baseline;

            WidgetID txt = m_Scene.CreateWidget<TextWidget>( box, Shared<Theme>{}, e.label, ts.Layout, ts.Render );
            {
                auto* n = Node( txt );
                n->Style.WidthMode  = ESizingMode::Content;
                n->Style.HeightMode = ESizingMode::Flex;
            }
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
            TextStyle ts = MakeStyle( 13_u );
            ts.Layout.Wrap = TextWrap::NoWrap();
            ts.Layout.Overflow = ETextOverflow::Ellipsis;
            AddRow( card, "NoWrap+Ellipsis", kLong, ts, ESizingMode::Flex );
        }
        {
            TextStyle ts = MakeStyle( 13_u );
            ts.Layout.Wrap = TextWrap::WrapWord();
            AddRow( card, "WrapWord", kLong, ts, ESizingMode::Flex );
        }
        {
            TextStyle ts = MakeStyle( 13_u );
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
            TextStyle ts = MakeStyle( 13_u );
            ts.Layout.Wrap = wrap;
            ts.Layout.Overflow = overflow;
            ts.Layout.MaxLines = maxLines;
            AddRow( card, label, kLong, ts, ESizingMode::Fixed, 260_u );
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
            TextStyle ts = MakeStyle( 14_u );
            ts.Layout.Transform = e.xf;
            AddRow( card, e.label, kRaw, ts );
        }
    }

    void BuildLetterSpacing( WidgetID parent )
    {
        WidgetID card = BeginCard( parent, "Letter & Word Spacing" );

        // Letter spacing demo
        constexpr struct { Unit sp; const char* label; } kLetterSpacing[] = {
            { -1_u,   "-1 px  tight"  },
            {  0_u,   " 0 px  normal" },
            {  1_u,   "+1 px"         },
            {  2.5_u, "+2.5 px"       },
            {  5.0_u, "+5 px  wide"   },
        };
        for ( auto& e : kLetterSpacing )
        {
            TextStyle ts = MakeStyle( 14_u );
            ts.Layout.LetterSpacing = e.sp;
            ts.Layout.Wrap = TextWrap::NoWrap();
            ts.Layout.Overflow = ETextOverflow::Clip;
            AddRow( card, e.label, "LOREM IPSUM SIT AMET", ts );
        }
        
        // Word spacing demo
        constexpr struct { Unit sp; const char* label; } kWordSpacing[] = {
            {  0_u,   " 0 px  normal" },
            {  5.0_u, "+5 px  wide"   },
            {  15_u,  "+15 px extra wide" },
        };
        for ( auto& e : kWordSpacing )
        {
            TextStyle ts = MakeStyle( 14_u );
            ts.Layout.WordSpacing = e.sp;
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

        constexpr struct { Unit lh; const char* label; } kLH[] = {
            {  0_u, "Auto"          },
            { 12_u, "12 px  tight"  },
            { 20_u, "20 px  normal" },
            { 30_u, "30 px  loose"  },
        };

        for ( auto& e : kLH )
        {
            TextStyle ts = MakeStyle( 13_u );
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
            TextStyle ts = MakeStyle( 14_u );
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
            TextStyle ts             = MakeStyle( 14_u );
            ts.Render.FillColor       = Colors::AccentViolet;
            ts.Render.Underline      = true;
            ts.Render.FadePercentage = 0.5f; // 50% faded underline
            ts.Layout.Wrap           = TextWrap::NoWrap();
            ts.Layout.Overflow       = ETextOverflow::Fade;
            AddRow( card, "Fade+UL", kLong, ts, ESizingMode::Fixed, 200_u );
        }
        // Rose strikethrough + uppercase + WrapWord
        {
            TextStyle ts             = MakeStyle( 14_u );
            ts.Render.FillColor       = Colors::AccentRose;
            ts.Render.Strikethrough  = true;
            ts.Layout.Transform      = ETextTransform::Uppercase;
            ts.Layout.Wrap           = TextWrap::NoWrap();
            ts.Layout.Overflow       = ETextOverflow::Fade;
            ts.Render.FadePercentage = 0.5f; // 50% faded strikethrough
            AddRow( card, "Fade+ST+UC", kLong, ts, ESizingMode::Fixed, 200_u );
        }
        // Amber wide spacing + capitalize + ellipsis
        {
            TextStyle ts            = MakeStyle( 13_u );
            ts.Render.FillColor      = Colors::AccentAmber;
            ts.Layout.LetterSpacing = 3_u;
            ts.Layout.Transform     = ETextTransform::Capitalize;
            ts.Layout.Wrap          = TextWrap::NoWrap();
            ts.Layout.Overflow      = ETextOverflow::Ellipsis;
            AddRow( card, "Wide+Cap", "the quick brown fox jumps over", ts, ESizingMode::Flex );
        }
        // Sky center-aligned large display
        {
            TextStyle ts         = MakeStyle( 22_u );
            ts.Render.FillColor   = Colors::AccentSky;
            ts.Render.Align      = ETextAlign::Center;
            ts.Layout.Wrap       = TextWrap::WrapWord();
            ts.Layout.LineHeight = 30_u;
            AddRow( card, "Center 22px", "The beauty of typography lives in every detail.", ts, ESizingMode::Flex );
        }
    }

    void BuildRenderStyles( WidgetID parent )
    {
        WidgetID card = BeginCard( parent, "Render Styles" );
        const Unit fontSize = 48_u;
        auto text = "I am fancy!";

        // Outline
        {
            TextStyle ts = MakeStyle( fontSize );
            ts.Render.Outline = true;
            ts.Render.OutlineWidth = 0.3f;
            ts.Render.OutlineColor = Colors::White;
            ts.Render.FillColor = Colors::Black;
            AddRow( card, "Outline", text, ts );
        }

        // Glow
        {
            TextStyle ts = MakeStyle( fontSize );
            ts.Render.Glow = true;
            ts.Render.GlowColor = Colors::White;
            ts.Render.FillColor = Colors::Black;
            AddRow( card, "Glow", text, ts );
        }

        // Shadow
        {
            TextStyle ts = MakeStyle( fontSize );
            ts.Render.Shadow = true;
            ts.Render.ShadowColor = Colors::White;
            ts.Render.ShadowSoftness = 5.f;
            ts.Render.FillColor = Colors::Black;
            AddRow( card, "Shadow", text, ts );
        }

        // Combined: outline + glow + shadow
        {
            TextStyle ts = MakeStyle( fontSize );

            ts.Render.Outline = true;
            ts.Render.OutlineColor = Colors::Red;

            ts.Render.Glow = true;
            ts.Render.GlowColor = Colors::Green;

            ts.Render.Shadow = true;
            ts.Render.ShadowColor = Colors::Blue;
            ts.Render.ShadowOffset = Vec2f{ 8, 8 };

            AddRow( card, "Outline, Glow, Shadow", text, ts );
        }
    }

    void BuildAnimated( WidgetID parent )
    {
        WidgetID card = BeginCard(
            parent,
            "Animated  —  container width oscillates, Ellipsis reflows live"
        );

        // Row container
        WidgetID row = m_Scene.CreateWidget<RectWidget>( card, Colors::Surface800, "ARow" );
        {
            auto* n = Node( row );
            n->Style.LayoutType = ELayoutType::Horizontal;
            n->Style.Spacing    = 8_u;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Content;
        }

        // Label
        const TextStyle lblStyle = RowLabelStyle();
        WidgetID lbl = m_Scene.CreateWidget<TextWidget>( row, Shared<Theme>{}, "Ellipsis", lblStyle.Layout, lblStyle.Render );
        {
            auto* n = Node( lbl );
            n->Style.Padding = Edges::Uniform( 4_u );
            n->Style.WidthMode  = ESizingMode::Fixed;
            n->Style.FixedWidth = 112_u;
            n->Style.HeightMode = ESizingMode::Content;
        }

        // Animated box
        {
            // We wrap the animated container in a flex parent because percent widths are based on the parent's content width, not the available width.

            WidgetID container = m_Scene.CreateWidget<RectWidget>( row, Colors::Transparent, "ARow" );
            {
                auto* n = Node( container );
                n->Style.WidthMode  = ESizingMode::Flex;
                n->Style.HeightMode = ESizingMode::Content;
            }

            m_AnimContainer = m_Scene.CreateWidget<RectWidget>(
                container, Colors::Surface600, "AnimBox",
                CornerRounding::Uniform( 6_u )
            );
            {
                auto* n = Node( m_AnimContainer );
                n->Style.WidthMode  = ESizingMode::Percent;
                n->Style.HeightMode = ESizingMode::Content;
                n->Style.LayoutType = ELayoutType::Horizontal;
                n->Style.Padding    = Edges::Uniform( 4_u );
                n->Style.Spacing    = 4_u;
            }
        }
        

        TextStyle ts       = MakeStyle( 13_u );
        ts.Render.FillColor    = Colors::AccentBlue;
        ts.Layout.Wrap     = TextWrap::NoWrap();
        ts.Layout.Overflow = ETextOverflow::Ellipsis;

        {
            WidgetID txt = m_Scene.CreateWidget<TextWidget>(
            m_AnimContainer,
            Shared<Theme>{},
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
            ts.Render.FillColor    = Colors::AccentEmerald;
            WidgetID txt = m_Scene.CreateWidget<TextWidget>(
            m_AnimContainer,
            Shared<Theme>{},
            "Amazingly few discotheques provide jukeboxes.",
			ts.Layout, ts.Render
            );
            {
                auto* n = Node( txt );
                n->Style.WidthMode  = ESizingMode::Flex;
                n->Style.HeightMode = ESizingMode::Content;
                n->Style.FlexGrow = 1.f;
            }
        }

    }

    // =========================================================================
public:

    void Init() override
    {
        // Root
        WidgetID root = m_Scene.CreateRootWidget<RectWidget>(
            Colors::Surface900, "Root", CornerRounding::None()
        );
        {
            auto* n = Node( root );
            n->Style.LayoutType = ELayoutType::Vertical;
            n->Style.Spacing    = 0_u;
            n->Style.Padding    = Edges::Uniform( 0_u );
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Flex;
        }

        // Title bar
        {
            WidgetID bar = m_Scene.CreateWidget<RectWidget>(
                root, Colors::Surface800, "TitleBar", CornerRounding::None()
            );
            auto* n = Node( bar );
            n->Style.LayoutType = ELayoutType::Vertical;
            n->Style.Padding    = Edges{ 14_u, 28_u, 10_u, 28_u };
            n->Style.Spacing    = 4_u;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Content;

            {
                TextStyle ts = MakeStyle( 22_u );
                ts.Render.FillColor = Colors::TextPrimary;
                AddText( bar, "Text Feature Showcase", ts, ESizingMode::Flex );
            }
            {
                TextStyle ts = MakeStyle( 11_u );
                ts.Render.FillColor   = Colors::TextSecondary;
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
            root, Colors::Surface900, "Content"
        );
        {
            auto* n = Node( content );
            n->Style.LayoutType = ELayoutType::Vertical;
            n->Style.Padding    = Edges{ 16_u, 24_u, 24_u, 24_u };
            n->Style.Spacing    = 12_u;
            n->Style.WidthMode  = ESizingMode::Flex;
            n->Style.HeightMode = ESizingMode::Content;
        }

        // Layout: single 3-column grid
        {
            auto [l, m, r] = MakeCols( content );
            
            // Left column
            BuildFontSizes( l );
            BuildWrapping( l );
            BuildLetterSpacing( l );
            BuildCombined( l );
            
            // Middle column
            BuildColors( m );
            BuildOverflow( m );
            BuildRenderStyles( m );
            
            // Right column
            BuildLineHeight( r );
            BuildTransform( r );
            BuildDecoration( r );
            BuildAlignment( r );
            BuildAnimated( r );
        }
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
                n->Style.PercentWidth = Math::Lerp( 0.1f, 1.f, t );
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
