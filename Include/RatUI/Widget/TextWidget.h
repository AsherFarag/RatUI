#pragma once
#include "../Text/Text.h"
#include "../Text/TextLayout.h"
#include "Scene.h"
#include "IWidget.h"

namespace RatUI
{
    class TextWidget : public IWidget
    {
    public:
        TextWidget( String a_Text = {},
                    const TextLayoutStyle& a_Style       = {},
                    const TextRenderStyle& a_RenderStyle = {} )
            : m_Text       ( std::move( a_Text ) )
            , m_LayoutStyle( a_Style )
            , m_RenderStyle( a_RenderStyle )
        {}

        virtual ~TextWidget() override = default;

        /**
         * @brief Replaces the text content.
         * Triggers full re-prepare + re-shape.
         */
        void SetText( String a_Text )
        {
            m_Text = std::move( a_Text );
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

        /**
         * @brief Replaces the render style.
         * Does NOT invalidate layout caches – render style only affects drawing.
         */
        void SetRenderStyle( const TextRenderStyle& a_RenderStyle )
        {
            m_RenderStyle = a_RenderStyle;
        }

        // =====================================================================
        // IWidget overrides
        // =====================================================================

        void OnSyncLayout( Scene& a_Scene, LayoutNode& a_Node, Vec2<Unit> a_AvailableSize ) override
        {
            ITextMetrics* metrics = a_Scene.TextMetrics;
            a_Node.Layout.IntrinsicSize = { 0_u, 0_u };

            if ( !metrics || Empty( m_Text ) )
                return;

            // ---- Prepare phase ----
            // Re-run when text content or any layout style property changes.
            if ( !m_PreparedText )
            {
                m_PreparedText = metrics->Prepare( m_Text, m_LayoutStyle );
                if ( !m_PreparedText )
                {
                    // Keep trying next frame.
                    return;
                }
                m_LastLayoutStyle = m_LayoutStyle; // snapshot for future change detection
                InvalidateShaped();
            }
            else if ( m_LayoutStyle != m_LastLayoutStyle )
            {
                // Layout style changed since last prepare – full redo.
                m_PreparedText = metrics->Prepare( m_Text, m_LayoutStyle );
                if ( !m_PreparedText )
                    return;
                m_LastLayoutStyle = m_LayoutStyle;
                InvalidateShaped();
            }

            // ---- Determine shaping width ----
            Unit maxWidth = 0_u;

            if ( a_Node.Style.WidthMode == ESizingMode::Fixed )
            {
                maxWidth = a_Node.Style.FixedWidth;
            }
            else if ( a_Node.Style.WidthMode == ESizingMode::Percent &&
                      a_Node.Layout.FinalRect.Size[0] > 0_u )
            {
                maxWidth = a_Node.Layout.FinalRect.Size[0] - a_Node.Style.Padding.Horizontal();
            }
            else if ( a_Node.Style.WidthMode == ESizingMode::Flex )
            {
                // Two-pass approach:
                //   Pass 1: FinalRect is zero – shape at available width as a best-effort
                //           measure.  The resulting intrinsic size triggers a second layout
                //           pass in Scene::UpdateLayout.
                //   Pass 2: FinalRect is populated from the first arrange – shape at the
                //           real allocated width.
                if ( a_Node.Layout.FinalRect.Size[0] > 0_u )
                    maxWidth = a_Node.Layout.FinalRect.Size[0] - a_Node.Style.Padding.Horizontal();
                else
                    maxWidth = std::max( 0_u, a_AvailableSize[0] - a_Node.Style.Padding.Horizontal() );
            }
            else
            {
                // Content-sized or fallback
                maxWidth = std::max( 0_u, a_AvailableSize[0] - a_Node.Style.Padding.Horizontal() );
            }

            maxWidth = std::max( maxWidth, 0_u );

            // ---- Shape phase ----
            // Re-shape if the width changed or if shaped cache is invalid.
            // Use approximate equality to avoid needless reshaping from floating-point drift.
            const bool widthChanged = !IsApproxEqual( maxWidth.ToFloat(), m_ShapedWidth.ToFloat() );

            if ( !m_ShapedText || widthChanged )
            {
                m_ShapedText  = metrics->Shape( *m_PreparedText, m_LayoutStyle,
                                                { maxWidth, Limits<Unit>::max() } );
                m_ShapedWidth = maxWidth;
            }

            if ( !m_ShapedText )
                return;

            a_Node.Layout.IntrinsicSize = { m_ShapedText->MaxWidth, m_ShapedText->TotalHeight };
        }

        void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
        {
            LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );

            if ( !node || !node->Layout.Visibility.IsRendered() )
                return;
                
            if ( !m_ShapedText )
                return;

            const Rect<Unit> textRect = node->Style.Padding.Apply( node->Layout.FinalRect );

            // Suppress the fade percentage when not in Fade overflow mode so the
            // MSDF shader doesn't accidentally fade glyphs in Clip/Ellipsis mode.
            TextRenderStyle effectiveStyle = m_RenderStyle;
            effectiveStyle.FadePercentage  =
                ( m_LayoutStyle.Overflow == ETextOverflow::Fade )
                ? m_RenderStyle.FadePercentage
                : 0.f;

			effectiveStyle.FillColor = Colors::White;

                effectiveStyle.Outline = false;
                effectiveStyle.OutlineColor = Colors::Red;
                effectiveStyle.OutlineWidth = 0.4f;
                effectiveStyle.OutlineSoftness= 0.1f;

                effectiveStyle.Shadow = false;
                effectiveStyle.ShadowColor = Colors::Green;
				effectiveStyle.ShadowOffset = Vec2f{ 4.f, 4.f };
				effectiveStyle.ShadowSoftness = 1.f;
                //effectiveStyle.ShadowOffset = Vec2f{ 0.1f, -0.2f };
                //effectiveStyle.ShadowSoftness = 0.5f;

                effectiveStyle.Glow = true;
                effectiveStyle.GlowColor = Colors::Green;
				effectiveStyle.GlowSpread = 0.1f;


            switch ( m_LayoutStyle.Overflow )
            {
                case ETextOverflow::Clip:
                case ETextOverflow::Fade:
                {
                    a_DrawList.PushClipRect( textRect );
                    a_DrawList.AddText( *m_ShapedText, effectiveStyle, textRect );
                    a_DrawList.PopClipRect();
                    break;
                }
                default:
                {
                    a_DrawList.AddText( *m_ShapedText, effectiveStyle, textRect );
                    break;
                }
            }
        }

    protected:

        // ---- Cache invalidation helpers ----

        void InvalidatePrepared()
        {
            m_PreparedText = NullOpt;
            InvalidateShaped();
        }

        void InvalidateShaped()
        {
            m_ShapedText  = NullOpt;
            m_ShapedWidth = Unit{ -1.f }; // sentinel, any real width will differ
        }

        String          m_Text;
        TextLayoutStyle m_LayoutStyle;
        TextRenderStyle m_RenderStyle;

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