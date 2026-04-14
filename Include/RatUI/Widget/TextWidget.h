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
        TextWidget( String a_Text = {}, const TextLayoutStyle& a_Style = {}, const TextRenderStyle& a_RenderStyle = {} )
            : m_Text( std::move( a_Text ) )
            , m_LayoutStyle( a_Style )
            , m_RenderStyle( a_RenderStyle )
        {}

        virtual ~TextWidget() override = default;

        /**
         * @brief Sets the text content of the widget. 
         * This will trigger a re-preparation and re-shaping of the text, which can be expensive operations, 
         * so it should be used judiciously (e.g., avoid calling this every frame in an update loop).
         */
        void SetText( String a_Text )
        {
            m_Text = std::move( a_Text );
            InvalidatePrepared(); // Text content changes can affect both preparation and shaping, so we need to invalidate both caches.
        }

        /**
         * @brief Sets the layout style for the text.
         * This will trigger a re-preparation and re-shaping of the text, which can be expensive operations,
         * so it should be used judiciously (e.g., avoid calling this every frame in an update loop).
         */
        void SetLayoutStyle( const TextLayoutStyle& a_Style )
        {
            m_LayoutStyle = a_Style;
            InvalidatePrepared(); // Style changes can affect both preparation and shaping, so we need to invalidate both caches.
        }

        /**
         * @brief Sets the render style for the text.
         */
        void SetRenderStyle( const TextRenderStyle& a_RenderStyle )
        {
            m_RenderStyle = a_RenderStyle;
            // Render style changes don't affect preparation or shaping, so we don't need to invalidate those caches.
        }

        /**
         * @brief 
         */
        void OnSyncLayout( Scene& a_Scene, LayoutNode& a_Node, Vec2f a_AvailableSize ) override
        {
            ITextMetrics* metrics = a_Scene.TextMetrics;
            a_Node.Layout.IntrinsicSize = { 0.f, 0.f };

			if ( !metrics || Empty( m_Text ) )
                return;

            // Re-prepare only when text content or style changes (expensive - segments are pre-measured).
            if ( !m_PreparedText )
            {
                m_PreparedText  = metrics->Prepare( m_Text, m_LayoutStyle );
                
                if ( !m_PreparedText )
                {
                    InvalidatePrepared(); // If preparation failed, invalidate prepared text to trigger a retry on the next layout pass. 
                    return;
                }

                InvalidateShaped();
            }

            f32 maxWidth = 0.f;

            if ( a_Node.Style.WidthMode == ESizingMode::Fixed )
            {
                maxWidth = a_Node.Style.FixedWidth;
            }
            else if ( a_Node.Style.WidthMode == ESizingMode::Flex && a_Node.Layout.FinalRect.Size[0] > 0.f )
            {
                // TODO: This is a bit of a hack to work around the circular dependency 
                // where we need to know the final width of the node to shape the text, 
                // but the final width depends on the intrinsic size which depends on shaping the text.
                maxWidth = a_Node.Layout.FinalRect.Size[0] - a_Node.Style.Padding.Horizontal();
            }
            else
            {
                maxWidth = a_AvailableSize[0] - a_Node.Style.Padding.Horizontal();
            }
                
            maxWidth = std::max( maxWidth, 0.f );

            if ( !IsApproxEqual( maxWidth, m_LastAvailableWidth ) )
            {
                m_LastAvailableWidth = maxWidth;
                InvalidateShaped();
            }

            // Re-shape only when prepared text changes or when available width changes (Expensive).
            if ( !m_ShapedText || !IsApproxEqual( maxWidth, m_ShapedWidth ) )
            {
                m_ShapedText  = metrics->Shape( *m_PreparedText, m_LayoutStyle, { maxWidth, Limits<f32>::max() } );                  
                m_ShapedWidth = maxWidth;
            }

            if ( !m_ShapedText )
            {
                InvalidateShaped(); // If shaping failed, invalidate shaped text to trigger a retry on the next layout pass.
                return;
            }

            a_Node.Layout.IntrinsicSize = { m_ShapedText->MaxWidth, m_ShapedText->TotalHeight };
        }

        /**
         * @brief
         */
        void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
        {
            LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );

            if ( !node || !node->Layout.Visibility.IsRendered() ) 
                return; // Don't attempt to paint if we don't have a valid layout node or if the node is not visible.

            if ( !m_ShapedText ) 
                return; // Don't attempt to paint if we have no text or if the text isn't prepared or shaped.

            const Rectf textRect = node->Style.Padding.Apply( node->Layout.FinalRect );

            // TODO: Pushing a clip rect doesnt work if this text is rotated or transformed in some other way. 
            // Need to use stencil clipping instead of scissor

            // TODO: Hack to disable fading when not using ETextOverflow::Fade, since the fade percentage is still applied in the shader even when overflow mode is Ellipsis or Clip. 
            // We should ideally refactor this to avoid needing a hack like this.
            TextRenderStyle effectiveRenderStyle = m_RenderStyle;
            effectiveRenderStyle.FadePercentage = m_LayoutStyle.Overflow == ETextOverflow::Fade ? m_RenderStyle.FadePercentage : 0.f;

            switch ( m_LayoutStyle.Overflow )
            {
                case ETextOverflow::Clip:
                {
                    a_DrawList.PushClipRect( textRect.Cast<u16>() );
                    a_DrawList.AddText( *m_ShapedText, effectiveRenderStyle, textRect );
                    a_DrawList.PopClipRect();
                    break;
                }
                case ETextOverflow::Fade:
                {
                    a_DrawList.PushClipRect( textRect.Cast<u16>() );
                    a_DrawList.AddText( *m_ShapedText, effectiveRenderStyle, textRect );
                    a_DrawList.PopClipRect();
                    break;
                }
                default:
                {
                    a_DrawList.AddText( *m_ShapedText, effectiveRenderStyle, textRect );
                    break;
                }
            }
        }

    protected:
        void InvalidatePrepared()
        {
            m_PreparedText.reset(); // TODO: Add Reset() method
            InvalidateShaped(); // Prepared text is an input to shaping, so we need to invalidate shaped text when prepared text changes.
        }

        void InvalidateShaped()
        {
            m_ShapedText.reset(); // TODO: Add Reset() method
        }

        String                 m_Text;         ///< The text content to be displayed by the widget.
        TextLayoutStyle        m_LayoutStyle;  ///< The layout style for the text, including font, size, alignment, wrapping, etc.
        TextRenderStyle        m_RenderStyle;  ///< The render style for the text, including color and decorations like underline or strikethrough.
        Optional<PreparedText> m_PreparedText; ///< Cached prepared text (segments + normalized string). Rebuilt when text or style changes.
        Optional<ShapedText>   m_ShapedText;   ///< Cached shaped text (glyph atlas indices + line metadata). Rebuilt when prepared text or width changes.
        f32 m_ShapedWidth       { -1.f }; ///< The maxWidth used for the last Shape() call; used to detect when re-shaping is needed.
        f32 m_LastAvailableWidth{ -1.f }; ///< The last effective width constraint used during layout; used to invalidate stale shaped text.
    };

} // namespace RatUI