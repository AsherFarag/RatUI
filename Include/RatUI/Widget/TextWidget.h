#pragma once
#include "../Text/Text.h"
#include "../Text/TextLayout.h"
#include "Theme.h"
#include "Scene.h"
#include "IWidget.h"

namespace RatUI
{
    /**
     * @brief The clip rect for text in ETextOverflow::Clip mode, or NullOpt when the text fits and needs none.
     * Clipping only when needed keeps fitting labels in the same draw batch as their neighbours. On an axis the text
     * overflows it clips at @p a_NodeRect's edge; on an axis it fits it leaves a line's height of room on each side, so
     * glyphs that overhang a tight line box (ascenders, descenders, italics) stay visible.
     *
     * @param a_NodeRect   The text widget's FinalRect.
     * @param a_TextRect   The rect the text is laid out in (the node rect less its content padding).
     * @param a_TextSize   The shaped text's width (its widest line) and height (its line boxes).
     * @param a_LineHeight The height of one line box.
     */
    RATUI_NODISCARD inline Optional<Rect<Unit>> TextClipRect( const Rect<Unit>& a_NodeRect, const Rect<Unit>& a_TextRect,
                                                              Vec2<Unit> a_TextSize, Unit a_LineHeight )
    {
        constexpr f32 c_Epsilon = 0.01f;
        const bool overflowX = a_TextSize[0].ToFloat() > a_TextRect.Size[0].ToFloat() + c_Epsilon;
        const bool overflowY = a_TextSize[1].ToFloat() > a_TextRect.Size[1].ToFloat() + c_Epsilon;
        if ( !overflowX && !overflowY )
            return NullOpt;

        const Vec2<Unit> room{ overflowX ? 0_u : a_LineHeight, overflowY ? 0_u : a_LineHeight };
        return Rect<Unit>::FromMinMax( a_NodeRect.Min() - room, a_NodeRect.Max() + room );
    }

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

        ~TextWidget() override = default;

        /** @brief Gets the current text content. */
        const Text& GetText() const { return m_Text; }

        /**
         * @brief Replaces the text content.
         * Triggers full re-prepare + re-shape.
         */
        void SetText( Text a_Text )
        {
            m_Text = std::move( a_Text );
            m_ResolvedText = NullOpt;
            InvalidatePrepared();
        }

        /**
         * @brief Replaces the layout style.
         * Triggers full re-prepare + re-shape because all layout metrics can change.
         */
        void SetLayoutStyle( const TextLayoutStyle& a_Style )
        {
            m_LayoutStyle = a_Style;
            InvalidatePrepared();
        }

        // --------------------------------------------------------------------
        // IWidget overrides
        // --------------------------------------------------------------------

        Vec2<Unit> OnMeasureContent( const LayoutNode& a_Node, Vec2<Unit> a_AvailableSize, const LayoutContext& ) override
        {
            HandleThemeUpdate();

            ITextMetrics* metrics = GetScene().TextMetrics;
            if ( !metrics ) return { 0_u, 0_u };

            const Optional<ResolvedText> resolved = ResolveText( m_Text );
            if ( !resolved || Empty( resolved->Data ) ) return { 0_u, 0_u };

            if ( !m_ResolvedText || resolved->Version != m_ResolvedText->Version )
            {
                m_ResolvedText = resolved;
                InvalidatePrepared();
            }

            if ( !m_PreparedText || m_LayoutStyle != m_LastLayoutStyle )
            {
                m_PreparedText = metrics->Prepare( m_ResolvedText->Data, m_LayoutStyle );
                if ( !m_PreparedText ) 
                    return { 0_u, 0_u };

                m_LastLayoutStyle = m_LayoutStyle;
                InvalidateShaped();
            }

            const bool widthChanged = !IsApproxEqual( a_AvailableSize[0].ToFloat(), m_ShapedWidth.ToFloat() );
            if ( !m_ShapedText || widthChanged )
            {
                m_ShapedText = metrics->Shape( 
                    *m_PreparedText, 
                    m_LayoutStyle, 
                    { a_AvailableSize[0], Limits<Unit>::max() 
                } );
                m_ShapedWidth = a_AvailableSize[0];
            }

            if ( !m_ShapedText )
                return { 0_u, 0_u };

            return Vec2<Unit>{ m_ShapedText->MaxWidth, m_ShapedText->TotalHeight() };
        }

        bool HasWidthDependentContent() const override { return true; }

        void OnPaint( const PaintEvent& a_Event ) override
        {
            HandleThemeUpdate();

            if ( !m_ShapedText )
                return;

            Scene& scene = GetScene();
            const LayoutNode& node = GetLayout();
            const Rect<Unit> textRect = ContentPadding( node ).Apply( node.Layout.FinalRect );

            // Suppress the fade percentage when not in Fade overflow mode so the
            // MSDF shader doesn't accidentally fade glyphs in Clip/Ellipsis mode.
            TextRenderStyle effectiveStyle = RenderStyle;
            effectiveStyle.FadePercentage  =
                ( m_LayoutStyle.Overflow == ETextOverflow::Fade )
				? effectiveStyle.FadePercentage
                : 0.f;

            switch ( m_LayoutStyle.Overflow )
            {
                case ETextOverflow::Clip:
                {
                    // Text that doesn't fit is cut at the node's edge so it can't draw over its neighbours.
                    const Optional<Rect<Unit>> clip = TextClipRect( node.Layout.FinalRect, textRect,
                        { m_ShapedText->MaxWidth, m_ShapedText->TotalHeight() }, m_ShapedText->LineHeight );

                    if ( clip )
                        a_Event.Drawer.PushClipRect( *clip );
                    a_Event.Drawer.AddText( *m_ShapedText, effectiveStyle, textRect, VisibleGlyphs );
                    if ( clip )
                        a_Event.Drawer.PopClipRect();
                    break;
                }
                case ETextOverflow::Fade:
                {
                    // The fade shader hides the overflow itself.
                    a_Event.Drawer.AddText( *m_ShapedText, effectiveStyle, textRect, VisibleGlyphs );
                    break;
                }
                default:
                {
                    a_Event.Drawer.AddText( *m_ShapedText, effectiveStyle, textRect, VisibleGlyphs );
                    break;
                }
            }
        }

    protected:
        void HandleThemeUpdate()
        {
            if constexpr ( HasMixin<ThemeMixin> )
            {
                if ( !Theme.Update() )
                    return;

                RenderStyle = Theme.GetTextStyle( ThemeKey::TextStyle::Default, RenderStyle );

                if ( const FontHandle* font = Theme.TryGetFont( ThemeKey::Font::Default ) )
                {
                    if ( m_LayoutStyle.Font != *font )
                    {
                        m_LayoutStyle.Font = *font;
                        InvalidatePrepared();
                    }
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
        /// Cached result of the last ResolveText() call.
        /// Version is compared each frame to detect localisation/binding changes.
        Optional<ResolvedText> m_ResolvedText;
        TextLayoutStyle        m_LayoutStyle;

        /// Snapshot of m_LayoutStyle at the time of the last successful Prepare() call.
        /// Used to detect which fields changed and whether re-preparation is needed.
        TextLayoutStyle        m_LastLayoutStyle{};

        Optional<PreparedText> m_PreparedText;
        Optional<ShapedText>   m_ShapedText;

        /// The maxWidth passed to the last Shape() call.
        /// Compared against the new width each frame to decide whether to re-shape.
        Unit m_ShapedWidth{ -1.f };
    };

} // namespace RatUI