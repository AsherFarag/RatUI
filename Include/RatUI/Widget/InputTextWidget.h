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
     * @brief An editable text field. Editing behaviour (single-line, multi-line, numeric) comes from the ITextEditPolicy.
     *
     * It is edited while engaged (see IWidget::CanEngage): navigation stops on the field without editing it, and
     * Activate (Enter, A) or a click starts editing. While editing, typed characters arrive as TextInputEvents and
     * editing keys (Backspace, arrows, Ctrl+Z...) as ButtonEvents, before navigation. Cancel (Escape) stops editing, as
     * does a commit (Enter in a single-line field) after OnSubmit; Tab stops and moves on.
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
            : IWidget( { .Focus = EFocusPolicy::All, .Interactable = true, .Engageable = true } ), Policy( std::move( a_Policy ) )
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

        /** @brief Whether the field is being edited: it is engaged, so it takes typed text. */
        bool IsEditing() const { return IsEngaged(); }

        // --------------------------------------------------------------------
        // IWidget overrides
        // --------------------------------------------------------------------

        Optional<Rect<Unit>> GetTextInputRect() const override
        {
            return IsEditing() ? Optional<Rect<Unit>>{ GetCaretRect() } : NullOpt;
        }

        Reply OnButtonPressed( const ButtonEvent& a_Event ) override
        {
            if ( a_Event.Button == EButtonID::MouseLeft )
            {
                // TODO: Place the caret under the pointer instead of at the end.
                if ( !IsEditing() )
                    m_Model.SetCaret( static_cast<u32>( m_Model.GetTextBuffer().size() ) );
                return Reply::Handled().Engage( GetLayoutID() );
            }

            if ( !IsEditing() || !Policy )
                return Reply::Unhandled();

            const TextEditOutcome outcome = Policy->HandleKey( m_Model, a_Event );
            if ( !outcome.Handled )
                return PassesThrough( a_Event.Button ) ? Reply::Unhandled() : Reply::Handled();

            if ( outcome.Changed )
                OnTextChanged();

            Reply reply = Reply::Handled();

            // Holding Enter shouldn't submit again with every key repeat.
            if ( outcome.Commited && !a_Event.Repeat )
            {
                if ( OnSubmit ) OnSubmit( m_Utf8 );
                if ( IsEditing() ) // OnSubmit may have moved focus on.
                    reply.Disengage();
            }

            if ( outcome.Canceled )
                reply.Disengage();

            return reply;
        }

        Reply OnButtonReleased( const ButtonEvent& a_Event ) override
        {
            return a_Event.Button == EButtonID::MouseLeft ? Reply::Handled() : Reply::Unhandled();
        }

        Reply OnTextInput( const TextInputEvent& a_Event ) override
        {
            if ( !IsEditing() || !Policy )
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

            // Lines are measured with their trailing spaces (ShapedText::MaxWidth leaves them out),
            // so the caret after trailing spaces stays inside the field.
            Unit width = 0_u;
            u32  lines = 1;
            for ( const codepoint c : m_Model.GetTextBuffer() )
                lines += c == U'\n' ? 1u : 0u;

            if ( m_ShapedText )
            {
                for ( u32 i = 0; i < m_ShapedText->LineCount(); ++i )
                    width = std::max( width, m_ShapedText->CaretOffset( i, m_ShapedText->Lines[i].TextEnd ) );
            }

            // A content-sized field fits its placeholder, whether or not it is showing, so the
            // field doesn't change size when the first character is typed.
            if ( const ShapedText* placeholder = GetShapedPlaceholder() )
            {
                width = std::max( width, placeholder->MaxWidth );
                lines = std::max( lines, placeholder->LineCount() );
            }

            // Content size only: the layout engine adds the padding. One line box per line, the same as
            // ShapedText::TotalHeight, so a field and a label with the same style are the same height.
            const Unit height = m_LineHeight * static_cast<f32>( lines ) + m_LineSpacing * static_cast<f32>( lines - 1 );
            return Vec2<Unit>{ std::max( width + CaretWidth, MinWidth ), height };
        }

    protected:

        void OnPaint( const PaintEvent& a_Event ) override
        {
            HandleThemeUpdate();

            // Text that couldn't be shaped when it was measured (e.g. no font yet) needs measuring again once it can.
            const bool wasShaped = m_Shaped;
            EnsureShaped();
            if ( m_Shaped && !wasShaped )
                GetLayout().MarkDirty();

            const LayoutNode& node     = GetLayout();
            const Rect<Unit>& rect     = node.Layout.SceneRect;
            const Rect<Unit>  textRect = GetTextRect( ContentPadding( node ).Apply( rect ) );
            const bool        focused  = IsEditing();

            // The ring shows while editing, and while navigation is on the field.
            a_Event.Drawer.AddBrushRect( rect, FillBrush, BorderColor, BorderThickness, Radius );
            if ( focused || GetScene().IsFocusVisible( GetLayoutID() ) )
                FocusRing.Draw( a_Event.Drawer, rect, Radius );

            // Clip text at the padding horizontally (it scrolls past the edges) but only at the border
            // vertically, so ascenders and descenders that overhang a tight line box stay visible.
            const Rect<Unit> padded = ContentPadding( node ).Apply( rect );
            const Unit       inset  = std::min( BorderThickness, rect.Size[1] * 0.5f );
            a_Event.Drawer.PushClipRect( Rect<Unit>{ Vec2<Unit>{ padded.Origin[0], rect.Origin[1] + inset },
                                                     Vec2<Unit>{ padded.Size[0], rect.Size[1] - inset * 2.f } } );

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
                        const Unit x0   = CaretX( lineStart, from );
                        // Show a sliver for selected newlines so empty selected lines are visible.
                        const Unit x1   = CaretX( lineStart, to ) + ( end > lineEnd ? Unit{ 4.f } : 0_u );

                        a_Event.Drawer.AddRect(
                            Rect<Unit>{ Vec2<Unit>{ textRect.Origin[0] + x0, textRect.Origin[1] + ( m_LineHeight + m_LineSpacing ) * static_cast<f32>( lineIndex ) },
                                        Vec2<Unit>{ x1 - x0, m_LineHeight } },
                            { .FillColor = SelectionColor } );
                    }

                    if ( lineEnd >= end || lineEnd >= text.size() )
                        break;

                    lineStart = lineEnd + 1;
                    ++lineIndex;
                }
            }

            // Line boxes start at textRect; the renderer centres the glyphs in each of them.
            if ( m_ShapedText )
            {
                a_Event.Drawer.AddText( *m_ShapedText, GetDrawStyle( RenderStyle.FillColor ), textRect );
            }
            else if ( m_Model.GetTextBuffer().empty() )
            {
                if ( const ShapedText* placeholder = GetShapedPlaceholder() )
                    a_Event.Drawer.AddText( *placeholder, GetDrawStyle( GetPlaceholderColor() ), textRect );
            }

            if ( focused )
                a_Event.Drawer.AddRect( GetCaretRect(), { .FillColor = CaretColor.value_or( RenderStyle.FillColor ) } );

            a_Event.Drawer.PopClipRect();
        }

        /** @brief Where the caret is drawn. */
        Rect<Unit> GetCaretRect() const
        {
            const LayoutNode& node     = GetLayout();
            const Rect<Unit>  textRect = GetTextRect( ContentPadding( node ).Apply( node.Layout.SceneRect ) );
            const TextBuffer& text     = m_Model.GetTextBuffer();
            const u32         caret    = m_Model.Caret();

            u32 lineStart = 0, lineIndex = 0;
            for ( u32 i = 0; i < caret; ++i )
            {
                if ( text[i] == U'\n' )
                {
                    lineStart = i + 1;
                    ++lineIndex;
                }
            }

            return Rect<Unit>{ Vec2<Unit>{ textRect.Origin[0] + CaretX( lineStart, caret ),
                                           textRect.Origin[1] + ( m_LineHeight + m_LineSpacing ) * static_cast<f32>( lineIndex ) },
                               Vec2<Unit>{ CaretWidth, m_LineHeight } };
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

        /** @brief The text changed: reshape it. */
        void InvalidateShaped()
        {
            m_ShapedText = NullOpt;
            m_Shaped     = false;
            if ( m_Scene )
                GetLayout().MarkDirty();
        }

        /** @brief The font or style changed: every cached measurement is stale. */
        void InvalidateFont()
        {
            m_ShapedPlaceholder = NullOpt;
            m_PlaceholderShaped = false;
            m_LineHeightValid   = false;
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
                if ( Optional<ShapedText> probe = Shape( c_Probe ) ) // A visible glyph: whitespace alone may not shape.
                {
                    // The renderer centres each line's glyphs in its LineHeight box (ShapedText::FirstBaseline),
                    // so the caret and selection boxes line up with the text without any offset.
                    m_LineHeight      = probe->LineHeight;
                    m_LineSpacing     = probe->LineSpacing;
                    m_LineHeightValid = true;
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

            // The caret maps buffer bytes to glyph clusters, so the shaped text must be the buffer's bytes:
            // no transform (e.g. uppercase 'ß' is "SS"), and whitespace kept as typed.
            TextLayoutStyle style = m_LayoutStyle;
            style.Transform       = ETextTransform::None;
            style.Wrap            = TextWrap::PreWrap();

            Optional<PreparedText> prepared = metrics->Prepare( a_Text, style );
            if ( !prepared )
                return NullOpt;

            return metrics->Shape( *prepared, style );
        }

        static constexpr const char* c_Probe = "|";

        /** @brief UTF-8 byte offset of codepoint a_Index: the prepared text's offsets, as the field keeps only '\n' breaks. */
        u32 ByteOffset( u32 a_Index ) const
        {
            const TextBuffer& text  = m_Model.GetTextBuffer();
            u32               bytes = 0;
            for ( u32 i = 0; i < a_Index && i < text.size(); ++i )
                bytes += text[i] < 0x80 ? 1u : text[i] < 0x800 ? 2u : text[i] < 0x10000 ? 3u : 4u;
            return bytes;
        }

        /** @brief Distance from the start of the line beginning at codepoint a_LineStart to a caret before codepoint a_Index. */
        Unit CaretX( u32 a_LineStart, u32 a_Index ) const
        {
            if ( !m_ShapedText || a_Index <= a_LineStart )
                return 0_u;

            const u32 lineByte = ByteOffset( a_LineStart );
            for ( u32 i = 0; i < m_ShapedText->LineCount(); ++i )
            {
                if ( m_ShapedText->Lines[i].TextStart == lineByte )
                    return m_ShapedText->CaretOffset( i, ByteOffset( a_Index ) );
            }
            return 0_u;
        }

    protected:
        TextEditModel          m_Model;
        String                 m_Utf8;           ///< UTF-8 copy of the model's buffer, for shaping and callbacks.
        TextLayoutStyle        m_LayoutStyle{};
        Optional<ShapedText>   m_ShapedText;
        bool                   m_Shaped{ false };
        Unit                   m_LineHeight{ 20_u };
        Unit                   m_LineSpacing{ 0_u }; ///< Extra space between lines (ShapedText::LineSpacing).
        bool                   m_LineHeightValid{ false };
        String                 m_Placeholder;
        Optional<ShapedText>   m_ShapedPlaceholder;
        bool                   m_PlaceholderShaped{ false };
    };

} // namespace RatUI
