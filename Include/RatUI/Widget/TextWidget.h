#pragma once
#include "../Text/Text.h"
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

        void OnDestroy( Scene& a_Scene ) override
        {
            if ( m_ShapedText.IsValid() && a_Scene.TextMetrics )
                a_Scene.TextMetrics->ReleaseShapedText( m_ShapedText );
        }

        void SetText( Text a_Text ) 
        { 
            m_Text = std::move( a_Text );
            m_ShapedText = {}; 
        }

		void OnSyncLayout( Scene& a_Scene, LayoutNode& a_Node, Vec2f a_AvailableSize ) override
        {
            ITextMetrics* metrics = a_Scene.TextMetrics;
            if ( !metrics || m_Text.empty() )
            {
                a_Node.Layout.IntrinsicSize = { 0.f, 0.f };
                return;
            }

            f32 maxWidth = 0.f;

            if ( a_Node.Style.WidthMode == ESizingMode::Fixed ) 
                maxWidth = a_Node.Style.FixedWidth;
            else
                maxWidth = a_AvailableSize[0] - a_Node.Style.Padding.Horizontal();

			TextMeasurement t = metrics->Measure( m_Text, m_Style, std::max( maxWidth, 0.f ) );
            a_Node.Layout.IntrinsicSize = t.Size;
        }

        void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
        {
            LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
            if ( !node || !node->Layout.Visibility.IsRendered() ) return;
            if ( m_Text.empty() ) return;

            const Rectf& finalRect = node->Layout.FinalRect;
            Rectf textRect{ finalRect.Origin, node->Layout.IntrinsicSize };
            textRect = node->Style.Padding.Apply( textRect );

            a_DrawList.AddText( m_Text, m_Style, textRect );
        }

    protected:
        Text       m_Text;       ///< The text content to be displayed by the widget.
        ShapedText m_ShapedText; ///< Cached shaped text.
        TextStyle  m_Style;      ///< The style to apply when rendering the text.
    };

} // namespace RatUI