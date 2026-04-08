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
        TextWidget( Text a_Text = {}, TextStyle a_Style = {} )
            : m_Text( std::move( a_Text ) )
            , m_Style( std::move( a_Style ) )
        {}

        virtual ~TextWidget() override = default;

        void SetText( Text a_Text )
        {
            m_Text = std::move( a_Text );
            m_PreparedDirty = true;
        }

        void SetStyle( TextStyle a_Style )
        {
            m_Style = std::move( a_Style );
            m_PreparedDirty = true;
        }

        void OnSyncLayout( Scene& a_Scene, LayoutNode& a_Node, Vec2f a_AvailableSize ) override
        {
            ITextMetrics* metrics = a_Scene.TextMetrics;
			if ( !metrics || Empty( m_Text ) )
            {
                a_Node.Layout.IntrinsicSize = { 0.f, 0.f };
                return;
            }

            // Re-prepare only when text content or style changes (expensive – segments are pre-measured).
            if ( m_PreparedDirty )
            {
                m_PreparedText = metrics->Prepare( m_Text, m_Style );
                m_PreparedDirty = false;
            }

            f32 maxWidth = 0.f;

            if ( a_Node.Style.WidthMode == ESizingMode::Fixed )
                maxWidth = a_Node.Style.FixedWidth;
            else
                maxWidth = a_AvailableSize[0] - a_Node.Style.Padding.Horizontal();

            TextMeasurement t = metrics->Measure( m_PreparedText, m_Style, std::max( maxWidth, 0.f ) );
            a_Node.Layout.IntrinsicSize = t.Size;
        }

        void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
        {
            LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );

            if ( !node || !node->Layout.Visibility.IsRendered() ) 
                return;

            if ( Empty( m_Text ) || Empty( m_PreparedText.Segments ) ) 
                return;

            const Rectf textRect = node->Style.Padding.Apply( node->Layout.FinalRect );

			a_DrawList.PushClipRect( textRect.Cast<u16>() );
            a_DrawList.AddRect( Colorsf::Red, textRect, {} );
			a_DrawList.AddText( m_PreparedText, m_Style, textRect );
			a_DrawList.PopClipRect();
        }

    protected:
        Text         m_Text;                  ///< The text content to be displayed by the widget.
        TextStyle    m_Style;                 ///< The style to apply when rendering the text.
        PreparedText m_PreparedText;          ///< Cached prepared text (segments + normalized string). Rebuilt when text or style changes.
        bool         m_PreparedDirty{ true }; ///< True when m_PreparedText needs to be rebuilt before the next layout/paint.
    };

} // namespace RatUI