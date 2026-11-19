#pragma once
#include "../Text/Text.h"
#include "../Text/TextEdit.h"
#include "../Text/TextLayout.h"
#include "Theme.h"
#include "Scene.h"
#include "FocusRing.h"
#include "IWidget.h"

namespace RatUI
{
    /**
     * @brief An editable text field. Click it (or navigate to it) to focus, then type.
     * Editing behaviour (single-line, multi-line, numeric) comes from the ITextEditPolicy.
     *
     * Typed characters arrive as TextInputEvents; editing keys (Backspace, arrows, Ctrl+Z...)
     * arrive as ButtonEvents. While focused, the widget receives keys before navigation and
     * only lets through keys its policy leaves unhandled (e.g. Up/Down in a single-line field).
     */
    class InputTextWidget : public IWidget
    {
    public:
        using TextBuffer = TextEditModel::TextBuffer;

        Unique<ITextEditPolicy> Policy;
        Callback<const String&> OnChanged; ///< Called with the new text (UTF-8) whenever it changes.
        Callback<const String&> OnSubmit;  ///< Called with the text (UTF-8) when the policy commits (Enter in single-line mode).

        // --------------------------------------------------------------------
        // Render Properties
        // --------------------------------------------------------------------

        TextRenderStyle RenderStyle{};
        Brush        FillBrush{ SolidBrush{ Colors::Surface800 } };
        Color        BorderColor{ Colors::Surface500 };
        Unit         BorderThickness{ 1_u };
        CornerRadius Radius{ CornerRadius::None() };
        Optional<Color> CaretColor{};                                   ///< Unset: use the text colour.
        Optional<Color> PlaceholderColor{};                             ///< Unset: the text colour at 60% opacity. Themed by ThemeKey::Color::TextPlaceholder.
        Color        SelectionColor{ Color{ 61, 130, 255, 96 } }; ///< AccentBlue, translucent. Themed by ThemeKey::Color::TextSelection.
        Unit         CaretWidth{ 1.5_u };
        Unit         MinWidth{ 120_u };
        FocusRingStyle FocusRing{};                                  ///< Drawn while the field has focus (theme: FocusOutline keys).

        explicit InputTextWidget( Unique<ITextEditPolicy> a_Policy = MakeUnique<SingleLineTextEditPolicy>() )
            : Policy( std::move( a_Policy ) )
        {
            // Preserve spaces and newlines so the caret and the rendered text agree.
            // Shaping uses unlimited width, so there are no soft wraps.
            m_LayoutStyle.Wrap = TextWrap::PreWrap();
        }

        // --------------------------------------------------------------------
        // Text
        // --------------------------------------------------------------------

        const TextEditModel& GetModel() const noexcept { return m_Model; }

        /** @brief Returns the current text, UTF-8 encoded. */
        const String& GetText() const noexcept { return m_Utf8; }

        /** @brief Replaces the text (UTF-8), clearing undo history and moving the caret to the end. Line breaks become '\n'. */
        void SetText( StringView a_Text )
        {
            TextBuffer buffer = DecodeEditText( a_Text );

            const u32 length = static_cast<u32>( buffer.size() );
            m_Model.SetTextBuffer( std::move( buffer ) );
            m_Model.SetCaret( length );
            OnTextChanged();
        }

        const TextLayoutStyle& GetLayoutStyle() const noexcept { return m_LayoutStyle; }

        /** @brief Replaces the layout style. Wrap is forced to TextWrap::PreWrap() so whitespace is kept. */
        void SetLayoutStyle( const TextLayoutStyle& a_Style )
        {
            m_LayoutStyle      = a_Style;
            m_LayoutStyle.Wrap = TextWrap::PreWrap();
            InvalidateFont();
        }

        /** @brief Hint text (UTF-8) shown greyed out while the field is empty, e.g. "Leave empty for a random seed". */
        const String& GetPlaceholder() const noexcept { return m_Placeholder; }

        void SetPlaceholder( StringView a_Text )
        {
            if ( m_Placeholder == a_Text )
                return;

            m_Placeholder       = String( a_Text );
            m_ShapedPlaceholder = NullOpt;
            m_PlaceholderShaped = false;
            if ( m_Scene )
                GetLayout().MarkDirty(); // A content-sized field fits its placeholder.
        }

        bool HasFocus() const { return GetScene().GetFocusedNode() == GetLayoutID(); }

        // --------------------------------------------------------------------
        // IWidget overrides
        // --------------------------------------------------------------------

        bool IsInteractable() const override { return true; }
        bool IsFocusable()    const override { return true; }
        bool WantsTextInput() const override { return HasFocus(); }

        Reply OnButtonPressed( const ButtonEvent& a_Event ) override
        {
            if ( IsPointerButton( a_Event.Button ) )
            {
                if ( a_Event.Button != EButtonID::MouseLeft )
                    return Reply::Unhandled();

                // TODO: Place the caret under the pointer instead of at the end.
                if ( !HasFocus() )
                    m_Model.SetCaret( static_cast<u32>( m_Model.GetTextBuffer().size() ) );
                return Reply::Handled().SetFocus( GetLayoutID() );
            }

            if ( !Policy )
                return Reply::Unhandled();

            const TextEditOutcome outcome = Policy->HandleKey( m_Model, a_Event );
            if ( !outcome.Handled )
                return PassesThrough( a_Event.Button ) ? Reply::Unhandled() : Reply::Handled();

            if ( outcome.Changed )
                OnTextChanged();

            // Holding Enter shouldn't submit again with every key repeat.
            if ( outcome.Commited && !a_Event.Repeat )
                if ( OnSubmit ) OnSubmit( m_Utf8 );

            if ( outcome.Canceled )
                return Reply::Handled().ClearFocus();

            return Reply::Handled();
        }

        Reply OnButtonReleased( const ButtonEvent& a_Event ) override
        {
            // Releases of keys this field took on press are routed back here by the Scene;
            // there is nothing to do on release.
            return a_Event.Button == EButtonID::MouseLeft ? Reply::Handled() : Reply::Unhandled();
        }

        Reply OnTextInput( const TextInputEvent& a_Event ) override
        {
            if ( !Policy )
                return Reply::Unhandled();

            // No modifier filtering here: platforms don't send text for shortcuts like Ctrl+A,
            // and Alt/Option (or AltGr) combinations that do produce text should be typed.
            const TextEditOutcome outcome = Policy->HandleText( m_Model, a_Event.Text );
            if ( outcome.Changed )
                OnTextChanged();

            return Reply::Handled();
        }

        Vec2<Unit> OnMeasureContent( const LayoutNode&, Vec2<Unit>, const LayoutContext& ) override
        {
            HandleThemeUpdate();
            EnsureShaped();

            // Measure each line including trailing spaces (ShapedText::MaxWidth drops them),
            // so the caret after trailing spaces stays inside the field.
            // Line widths are cached by content, so an edit only reshapes the line it changed.
            const TextBuffer& text  = m_Model.GetTextBuffer();
            Unit              width = 0_u;
            u32               lines = 1;
            LineWidthMap      current;
            for ( u32 lineStart = 0;; )
            {
                u32 lineEnd = lineStart;
                while ( lineEnd < text.size() && text[lineEnd] != U'\n' )
                    ++lineEnd;

                width = std::max( width, MeasureLine( lineStart, lineEnd, current ) );
                if ( lineEnd >= text.size() )
                    break;

                lineStart = lineEnd + 1;
                ++lines;
            }
            m_LineWidths = std::move( current ); // Drop lines that no longer exist.

            // A content-sized field fits its placeholder, whether or not it is showing, so the
            // field doesn't change size when the first character is typed.
            if ( const ShapedText* placeholder = GetShapedPlaceholder() )
            {
                width = std::max( width, placeholder->MaxWidth );
                lines = std::max( lines, placeholder->LineCount() );
            }

            // Content size only: the layout engine adds the padding.
            return Vec2<Unit>{ std::max( width + CaretWidth, MinWidth ), m_LineHeight * static_cast<f32>( lines ) };
        }

    protected:

        void OnPaint( const PaintEvent& a_Event ) override
        {
            HandleThemeUpdate();
            EnsureShaped();

            const LayoutNode& node     = GetLayout();
            const Rect<Unit>& rect     = node.Layout.FinalRect;
            const Rect<Unit>  textRect = GetTextRect( node.Style.Padding.Apply( rect ) );
            const bool        focused  = HasFocus();

            a_Event.Drawer.AddBrushRect( rect, FillBrush, BorderColor, BorderThickness, Radius );
            if ( focused )
                FocusRing.Draw( a_Event.Drawer, rect, Radius );

            a_Event.Drawer.PushClipRect( node.Style.Padding.Apply( rect ) );

            // Selection highlight, one rect per line touched.
            if ( focused && m_Model.HasSelection() )
            {
                const TextBuffer& text  = m_Model.GetTextBuffer();
                const u32         start = m_Model.SelectionStart();
                const u32         end   = m_Model.SelectionEnd();

                u32 lineStart = 0, lineIndex = 0;
                while ( lineStart <= text.size() )
                {
                    u32 lineEnd = lineStart;
                    while ( lineEnd < text.size() && text[lineEnd] != U'\n' )
                        ++lineEnd;

                    if ( lineEnd >= start && lineStart <= end )
                    {
                        const u32  from = std::max( start, lineStart );
                        const u32  to   = std::min( end, lineEnd );
                        const Unit x0   = MeasureRange( lineStart, from );
                        // Show a sliver for selected newlines so empty selected lines are visible.
                        const Unit x1   = MeasureRange( lineStart, to ) + ( end > lineEnd ? Unit{ 4.f } : 0_u );

                        a_Event.Drawer.AddRect(
                            Rect<Unit>{ Vec2<Unit>{ textRect.Origin[0] + x0, textRect.Origin[1] + m_LineHeight * static_cast<f32>( lineIndex ) },
                                        Vec2<Unit>{ x1 - x0, m_LineHeight } },
                            { .FillColor = SelectionColor } );
                    }

                    if ( lineEnd >= end || lineEnd >= text.size() )
                        break;

                    lineStart = lineEnd + 1;
                    ++lineIndex;
                }
            }

            // Line boxes start at textRect; move the glyphs so they sit in the middle of them.
            Rect<Unit> glyphRect = textRect;
            glyphRect.Origin[1] += m_GlyphOffset;

            if ( m_ShapedText )
            {
                a_Event.Drawer.AddText( *m_ShapedText, GetDrawStyle( RenderStyle.FillColor ), glyphRect );
            }
            else if ( m_Model.GetTextBuffer().empty() )
            {
                if ( const ShapedText* placeholder = GetShapedPlaceholder() )
                    a_Event.Drawer.AddText( *placeholder, GetDrawStyle( GetPlaceholderColor() ), glyphRect );
            }

            if ( focused )
            {
                const TextBuffer& text  = m_Model.GetTextBuffer();
                const u32         caret = m_Model.Caret();

                u32 lineStart = 0, lineIndex = 0;
                for ( u32 i = 0; i < caret; ++i )
                {
                    if ( text[i] == U'\n' )
                    {
                        lineStart = i + 1;
                        ++lineIndex;
                    }
                }

                const Unit x = MeasureRange( lineStart, caret );
                a_Event.Drawer.AddRect(
                    Rect<Unit>{ Vec2<Unit>{ textRect.Origin[0] + x, textRect.Origin[1] + m_LineHeight * static_cast<f32>( lineIndex ) },
                                Vec2<Unit>{ CaretWidth, m_LineHeight } },
                    { .FillColor = CaretColor.value_or( RenderStyle.FillColor ) } );
            }

            a_Event.Drawer.PopClipRect();
        }

        /**
         * @brief The area the lines are laid out in: the padded rect, with a single-line field's line
         * centred vertically when the field is taller than its text. Multi-line text stays at the top.
         */
        Rect<Unit> GetTextRect( Rect<Unit> a_Padded ) const
        {
            if ( Policy && !Policy->MultiLine() )
            {
                const Unit spare = a_Padded.Size[1] - m_LineHeight;
                if ( spare > 0_u )
                    a_Padded.Origin[1] += spare * 0.5f;
            }
            return a_Padded;
        }

        /**
         * @brief RenderStyle as the field draws it: left/top aligned, because the caret and selection are
         * measured from the left edge and the top of each line, and without the overflow fade, since
         * text past the edge is clipped (a fade would dim the characters next to the caret).
         */
        TextRenderStyle GetDrawStyle( Color a_Fill ) const
        {
            TextRenderStyle style = RenderStyle;
            style.FillColor       = a_Fill;
            style.Align           = ETextAlign::Left;
            style.Baseline        = ETextBaseline::Top;
            style.FadePercentage  = 0.f;
            return style;
        }

        Color GetPlaceholderColor() const
        {
            if ( PlaceholderColor )
                return *PlaceholderColor;

            Color color = RenderStyle.FillColor;
            color[3]    = static_cast<u8>( color[3] * 0.6f );
            return color;
        }

        /** @brief The shaped placeholder, or null if there is none (or no text metrics yet). */
        const ShapedText* GetShapedPlaceholder()
        {
            if ( Empty( m_Placeholder ) )
                return nullptr;

            if ( !m_PlaceholderShaped )
            {
                m_ShapedPlaceholder = Shape( m_Placeholder );
                m_PlaceholderShaped = HasValue( m_ShapedPlaceholder ); // Retried until text metrics exist.
            }
            return m_ShapedPlaceholder ? &*m_ShapedPlaceholder : nullptr;
        }

        /** @brief Keys the widget lets fall through to navigation when its policy doesn't use them; it keeps all others. */
        static constexpr bool PassesThrough( EButtonID a_Button )
        {
            switch ( a_Button )
            {
                case EButtonID::KeyUp:
                case EButtonID::KeyDown:
                case EButtonID::KeyTab:
                case EButtonID::Unknown:
                    return true;
                default:
                    return false;
            }
        }

        void OnTextChanged()
        {
            m_Utf8 = Unicode::EncodeUTF8( m_Model.GetTextBuffer().data(), m_Model.GetTextBuffer().size() );
            InvalidateShaped();
            if ( OnChanged ) OnChanged( m_Utf8 );
        }

        /** @brief The text changed: reshape it. Per-line widths are kept (they are keyed by content). */
        void InvalidateShaped()
        {
            m_ShapedText = NullOpt;
            m_Shaped     = false;
            ::RatUI::Clear( m_PrefixWidths );
            if ( m_Scene )
                GetLayout().MarkDirty();
        }

        /** @brief The font or style changed: every cached measurement is stale. */
        void InvalidateFont()
        {
            m_ShapedPlaceholder = NullOpt;
            m_PlaceholderShaped = false;
            m_LineHeightValid   = false;
            m_SentinelWidth   = Unit{ -1.f };
            ::RatUI::Clear( m_LineWidths );
            InvalidateShaped();
        }

        void HandleThemeUpdate()
        {
            if constexpr ( HasMixin<ThemeMixin> )
            {
                if ( !Theme.Update() )
                    return;

                RenderStyle     = Theme.GetTextStyle( ThemeKey::TextStyle::Default, RenderStyle );
                FillBrush       = Theme.GetBrush( ThemeKey::Brush::ButtonNormal, FillBrush );
                BorderColor     = Theme.GetColor( ThemeKey::Color::ButtonBorder, BorderColor );
                BorderThickness = std::max( 1_u, Theme.GetMetric( ThemeKey::Metric::ButtonBorderThickness, BorderThickness ) );
                Radius          = Theme.GetRadius( ThemeKey::Radii::Button, Radius );
                FocusRing.UpdateFromTheme( Theme );
                SelectionColor  = Theme.GetColor( ThemeKey::Color::TextSelection, SelectionColor );
                if ( const Color* placeholder = Theme.TryGetColor( ThemeKey::Color::TextPlaceholder ) )
                    PlaceholderColor = *placeholder;

                if ( const FontHandle* font = Theme.TryGetFont( ThemeKey::Font::Default ) )
                {
                    if ( m_LayoutStyle.Font != *font )
                    {
                        m_LayoutStyle.Font = *font;
                        InvalidateFont();
                    }
                }
            }
        }

        /** @brief Shapes the whole buffer (and the line height) if it changed since the last call. */
        void EnsureShaped()
        {
            if ( m_Shaped || !m_Scene )
                return;

            ITextMetrics* metrics = GetScene().TextMetrics;
            if ( !metrics )
                return;

            m_Shaped = true;

            // Line metrics come from shaping one glyph, so an empty field still has a height.
            // It only depends on the font, so it is not redone on every edit.
            if ( !m_LineHeightValid )
            {
                // Not marked valid on failure (e.g. the font isn't loaded yet), so it is retried.
                if ( Optional<ShapedText> probe = Shape( c_Sentinel ) ) // A visible glyph: whitespace alone may not shape.
                {
                    // Shaped text puts its first baseline at Ascender, which includes GlyphPadding.
                    // Shift the glyphs up by that padding, then down by half of any line gap, so the
                    // ink of each line sits in the middle of its LineHeight box.
                    const Unit descent = Unit{ std::abs( probe->Descender.ToFloat() ) };
                    const Unit ascent  = probe->Ascender - probe->GlyphPadding;
                    m_LineHeight       = probe->LineHeight;
                    m_GlyphOffset      = ( probe->LineHeight - ( ascent + descent ) ) * 0.5f - probe->GlyphPadding;
                    m_LineHeightValid  = true;
                }
                else
                {
                    m_Shaped = false; // Try again next measure/paint.
                    return;
                }
            }

            if ( !Empty( m_Utf8 ) )
                m_ShapedText = Shape( m_Utf8 );
        }

        Optional<ShapedText> Shape( StringView a_Text ) const
        {
            ITextMetrics* metrics = m_Scene ? m_Scene->TextMetrics : nullptr;
            if ( !metrics )
                return NullOpt;

            Optional<PreparedText> prepared = metrics->Prepare( a_Text, m_LayoutStyle );
            if ( !prepared )
                return NullOpt;

            return metrics->Shape( *prepared, m_LayoutStyle );
        }

        /** @brief Width of the codepoints [a_Begin, a_End) of one line, cached until the text changes. */
        Unit MeasureRange( u32 a_Begin, u32 a_End )
        {
            if ( a_End <= a_Begin )
                return 0_u;

            const u64 key = ( static_cast<u64>( a_Begin ) << 32 ) | a_End;
            if ( const Unit* cached = FindValue( m_PrefixWidths, key ) )
                return *cached;

            const Optional<Unit> width = MeasureCodepoints( a_Begin, a_End );
            if ( width )
                Emplace( m_PrefixWidths, key, *width ); // Not cached without text metrics, so it is measured once they exist.
            return width.value_or( 0_u );
        }

        using LineWidthMap = HashMap<TextBuffer, Unit>;

        /** @brief Width of the line [a_Begin, a_End), looked up by its content; records it in a_Current. */
        Unit MeasureLine( u32 a_Begin, u32 a_End, LineWidthMap& a_Current )
        {
            TextBuffer line( m_Model.GetTextBuffer().data() + a_Begin, a_End - a_Begin );
            if ( const Unit* cached = FindValue( a_Current, line ) )
                return *cached;

            Optional<Unit> width;
            if ( const Unit* known = FindValue( m_LineWidths, line ) )
                width = *known;
            else
                width = MeasureCodepoints( a_Begin, a_End );

            if ( width )
                Emplace( a_Current, std::move( line ), *width );
            return width.value_or( 0_u );
        }

        /** @brief Shapes the codepoints [a_Begin, a_End), or NullOpt without text metrics. */
        Optional<Unit> MeasureCodepoints( u32 a_Begin, u32 a_End )
        {
            if ( a_End <= a_Begin )
                return 0_u;

            // Shaped line widths leave out trailing whitespace, so measure with a sentinel glyph
            // appended and subtract the sentinel's own width.
            const String utf8 = Unicode::EncodeUTF8( m_Model.GetTextBuffer().data() + a_Begin, a_End - a_Begin ) + c_Sentinel;
            Optional<ShapedText> shaped = Shape( utf8 );
            if ( !shaped )
                return NullOpt;

            return std::max( 0_u, shaped->MaxWidth - SentinelWidth() );
        }

        static constexpr const char* c_Sentinel = "|";

        Unit SentinelWidth()
        {
            if ( m_SentinelWidth < 0_u )
            {
                Optional<ShapedText> shaped = Shape( c_Sentinel );
                if ( !shaped )
                    return 0_u; // No text metrics yet; measure again later.
                m_SentinelWidth = shaped->MaxWidth;
            }
            return m_SentinelWidth;
        }

    protected:
        TextEditModel          m_Model;
        String                 m_Utf8;           ///< UTF-8 copy of the model's buffer, for shaping and callbacks.
        TextLayoutStyle        m_LayoutStyle{};
        Optional<ShapedText>   m_ShapedText;
        bool                   m_Shaped{ false };
        Unit                   m_LineHeight{ 20_u };
        bool                   m_LineHeightValid{ false };
        Unit                   m_GlyphOffset{ 0_u };   ///< Vertical shift that puts each line's glyphs in the middle of its line box (see EnsureShaped).
        String                 m_Placeholder;
        Optional<ShapedText>   m_ShapedPlaceholder;
        bool                   m_PlaceholderShaped{ false };
        Unit                   m_SentinelWidth{ -1.f };
        HashMap<u64, Unit>             m_PrefixWidths; ///< (begin << 32 | end) -> width; cleared on every edit.
        LineWidthMap           m_LineWidths;     ///< Line content -> width; kept across edits, cleared on font change.
    };

} // namespace RatUI
