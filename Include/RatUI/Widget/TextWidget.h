#pragma once
#include "../Text/Text.h"
#include "../Text/TextLayout.h"
#include "Theme.h"
#include "Scene.h"
#include "IWidget.h"

namespace RatUI
{
    /** @brief Displays rich text. Plain text is just rich text with no spans. */
    class TextWidget : public IWidget
    {
    public:

        // --------------------------------------------------------------------
        // Render Properties
        // --------------------------------------------------------------------

        TextRenderStyle RenderStyle{};
        u32 VisibleGlyphs = Limits<u32>::max(); ///< Maximum number of glyphs to draw.

        TextWidget( Text a_Text = {}, const TextLayoutStyle& a_Style = {} )
            : m_Text       ( std::move( a_Text ) )
            , m_LayoutStyle( a_Style )
        {}

        TextWidget( StyledText a_Text, const TextLayoutStyle& a_Style = {} )
            : m_Text       ( std::move( a_Text.Content ) )
            , m_Spans      ( std::move( a_Text.Spans ) )
            , m_LayoutStyle( a_Style )
        {}

        ~TextWidget() override = default;

        /** @brief Gets the current text content. */
        const Text& GetText() const { return m_Text; }

        /** @brief Gets the current style spans (byte offsets into the resolved text). */
        const Array<TextSpan>& GetSpans() const { return m_Spans; }

        /**
         * @brief Replaces the text content and clears any spans (they referred to the old text).
         * Triggers full re-prepare + re-shape.
         */
        void SetText( Text a_Text )
        {
            m_Text = std::move( a_Text );
            ::RatUI::Clear( m_Spans );
            m_ResolvedText = NullOpt;
            InvalidatePrepared();
        }

        /** @brief Replaces the text content and its spans. */
        void SetStyledText( StyledText a_Text )
        {
            m_Text  = std::move( a_Text.Content );
            m_Spans = std::move( a_Text.Spans );
            m_ResolvedText = NullOpt;
            InvalidatePrepared();
        }

        /** @brief Replaces only the style spans, keeping the text. */
        void SetSpans( Array<TextSpan> a_Spans )
        {
            m_Spans = std::move( a_Spans );
            InvalidatePrepared();
        }

        /**
         * @brief Replaces the layout style.
         * Triggers full re-prepare + re-shape because all layout metrics can change.
         * An unset Family uses the theme's ThemeKey::FontFamily::Default, then the FontLibrary's default family.
         */
        void SetLayoutStyle( const TextLayoutStyle& a_Style )
        {
            m_LayoutStyle = a_Style;
            InvalidatePrepared();
        }

        /** @brief Gets the layout style as set on this widget (Family may be unset). */
        const TextLayoutStyle& GetLayoutStyle() const { return m_LayoutStyle; }

        /** @brief Gets the shaped result of the last layout pass, if any (e.g. for hit-testing or debug overlays). */
        const Optional<ShapedText>& GetShapedText() const { return m_ShapedText; }

        // --------------------------------------------------------------------
        // IWidget overrides
        // --------------------------------------------------------------------

        Vec2<Unit> OnMeasureContent( const LayoutNode& a_Node, Vec2<Unit> a_AvailableSize, const LayoutContext& ) override
        {
            HandleThemeUpdate();

            TextMetrics* metrics = GetScene().TextMetrics;
            if ( !metrics ) return { 0_u, 0_u };

            const Optional<ResolvedText> resolved = ResolveText( m_Text );
            if ( !resolved || resolved->Data.empty() ) return { 0_u, 0_u };

            if ( !m_ResolvedText || resolved->Version != m_ResolvedText->Version )
            {
                m_ResolvedText = resolved;
                InvalidatePrepared();
            }

            const TextLayoutStyle effectiveStyle = GetEffectiveLayoutStyle();
            if ( !m_PreparedText || effectiveStyle != m_LastLayoutStyle )
            {
                m_PreparedText = metrics->Prepare( StyledTextView{ m_ResolvedText->Data, m_Spans }, effectiveStyle );
                if ( !m_PreparedText )
                    return { 0_u, 0_u };

                m_LastLayoutStyle = effectiveStyle;
                InvalidateShaped();
            }

            const bool widthChanged = !IsApproxEqual( a_AvailableSize[0].ToFloat(), m_ShapedWidth.ToFloat() );
            if ( !m_ShapedText || widthChanged )
            {
                m_ShapedText = metrics->Shape(
                    *m_PreparedText,
                    effectiveStyle,
                    { a_AvailableSize[0], Limits<Unit>::max()
                } );
                m_ShapedWidth = a_AvailableSize[0];
            }

            if ( !m_ShapedText )
                return { 0_u, 0_u };

            return Vec2<Unit>{ m_ShapedText->MaxWidth, m_ShapedText->TotalHeight };
        }

        bool HasWidthDependentContent() const override { return true; }

        void OnPaint( const PaintEvent& a_Event ) override
        {
            HandleThemeUpdate();

            if ( !m_ShapedText )
                return;

            const LayoutNode& node = GetLayout();
            const Rect<Unit> textRect = node.Style.Padding.Apply( node.Layout.FinalRect );

            // Suppress the fade percentage when not in Fade overflow mode so glyphs aren't
            // accidentally faded in Clip/Ellipsis mode.
            TextRenderStyle effectiveStyle = RenderStyle;
            effectiveStyle.FadePercentage  =
                ( m_LayoutStyle.Overflow == ETextOverflow::Fade )
				? effectiveStyle.FadePercentage
                : 0.f;

            a_Event.Drawer.AddText( *m_ShapedText, effectiveStyle, textRect, VisibleGlyphs );
        }

    protected:
        /** @brief The widget's layout style, with an unset family taken from the theme. */
        TextLayoutStyle GetEffectiveLayoutStyle() const
        {
            TextLayoutStyle style = m_LayoutStyle;
            if ( !style.Family.IsValid() )
                style.Family = m_ThemeFamily;
            return style;
        }

        void HandleThemeUpdate()
        {
            if constexpr ( HasMixin<ThemeMixin> )
            {
                if ( !Theme.Update() )
                    return;

                RenderStyle = Theme.GetTextStyle( ThemeKey::TextStyle::Default, RenderStyle );

                // A family set on the widget wins over the theme's.
                const FontFamilyHandle* family = Theme.TryGetFontFamily( ThemeKey::FontFamily::Default );
                const FontFamilyHandle themeFamily = family ? *family : FontFamilyHandle{};
                if ( themeFamily != m_ThemeFamily )
                {
                    m_ThemeFamily = themeFamily;
                    if ( !m_LayoutStyle.Family.IsValid() )
                        InvalidatePrepared();
                }
            }
        }

        // ---- Cache invalidation helpers ----

        void InvalidatePrepared()
        {
            m_PreparedText = NullOpt;
            InvalidateShaped();
            GetLayout().MarkDirty(); // Mark dirty to trigger re-measure and re-shape
        }

        void InvalidateShaped()
        {
            m_ShapedText  = NullOpt;
            m_ShapedWidth = Unit{ -1.f }; // sentinel, any real width will differ
        }

        Text                   m_Text;
        Array<TextSpan>        m_Spans;
        /// Cached result of the last ResolveText() call.
        /// Version is compared each frame to detect localisation/binding changes.
        Optional<ResolvedText> m_ResolvedText;
        TextLayoutStyle        m_LayoutStyle;
        FontFamilyHandle       m_ThemeFamily{}; ///< Family from the theme, used when m_LayoutStyle.Family is unset.

        /// Snapshot of the effective layout style at the time of the last successful Prepare() call.
        TextLayoutStyle        m_LastLayoutStyle{};

        Optional<PreparedText> m_PreparedText;
        Optional<ShapedText>   m_ShapedText;

        /// The maxWidth passed to the last Shape() call.
        /// Compared against the new width each frame to decide whether to re-shape.
        Unit m_ShapedWidth{ -1.f };
    };

} // namespace RatUI
