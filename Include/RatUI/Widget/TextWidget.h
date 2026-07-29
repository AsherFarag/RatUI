#pragma once
#include "../Text/Text.h"
#include "../Text/TextLayout.h"
#include "Theme.h"
#include "Scene.h"
#include "IWidget.h"

namespace RatUI
{
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
            if ( !resolved || resolved->Data.empty() ) return { 0_u, 0_u };

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

            return Vec2<Unit>{ m_ShapedText->MaxWidth, m_ShapedText->TotalHeight };
        }

        bool HasWidthDependentContent() const override { return true; }

        void OnPaint( const PaintEvent& a_Event ) override
        {
            HandleThemeUpdate();

            if ( !m_ShapedText )
                return;

            Scene& scene = GetScene();
            const LayoutNode& node = GetLayout();
            const Rect<Unit> textRect = node.Style.Padding.Apply( node.Layout.FinalRect );

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
                case ETextOverflow::Fade:
                {
                    //a_Event.DrawList.PushClipRect( textRect );
                    a_Event.Drawer.AddText( *m_ShapedText, effectiveStyle, textRect, VisibleGlyphs );
                    //a_Event.DrawList.PopClipRect();
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