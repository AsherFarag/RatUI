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

        void OnSyncLayout( Scene& a_Scene, LayoutNode& a_Node ) override
        {
            ITextMetrics* metrics = a_Scene.TextMetrics;
			if ( !metrics || /* TODO: StringTraits Empty( m_Text )*/ m_Text.empty() )
            {
                a_Node.Layout.IntrinsicSize = { 0.f, 0.f };
                return;
            }

            // Determine wrapping width from the node's own style if possible,
            // otherwise unconstrained - the layout pass will constrain it.
            f32 maxWidth = Limits<f32>::max();
            if ( a_Node.Style.WidthMode == ESizingMode::Fixed )
                maxWidth = a_Node.Style.FixedWidth;

            TextMeasurement t = metrics->Measure( m_Text, m_Style, maxWidth );

            a_Node.Layout.IntrinsicSize = t.Size;

            // TODO: Shaped text

            //if ( m_Dirty || m_LastMaxWidth != maxWidth )
            //{
            //    if ( m_ShapedText.IsValid() )
            //        metrics->ReleaseShapedText( m_ShapedText );
//
            //    m_ShapedText   = metrics->Shape( m_Text, m_Style, maxWidth );
            //    m_LastMaxWidth = maxWidth;
            //    m_Dirty        = false;
            //}
//
            //a_Node.Layout.IntrinsicSize = m_ShapedText.Size;
        }

        void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
        {
           // if ( !m_ShapedText.IsValid() ) return;

            LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
            if ( !node || !node->Layout.Visibility.IsRendered() ) return;

            // Align within the final rect
            const Rectf& rect = node->Layout.FinalRect;
            Vec2f pos = rect.Origin; // TODO: This only works for left aligned text. Need to calculate the position based on alignment and available space.

            //a_DrawList.AddShapedText( SolidBrush{ m_Color }, m_ShapedText, pos ); TODO

			a_DrawList.AddText( SolidBrush{ m_Color }, m_Text, m_Style, Rectf{ pos, node->Layout.IntrinsicSize } );
        }

    protected:
        Text       m_Text;       ///< The text content to be displayed by the widget.
        ShapedText m_ShapedText; ///< Cached shaped text.
        TextStyle  m_Style;      ///< The style to apply when rendering the text.
        Colorf     m_Color{ Colors::White };    ///< The color of the text.
        ETextAlign m_Align{ ETextAlign::Left }; ///< Text alignment within the widget's bounds.

        bool  m_Dirty{ true }; ///< Whether the shaped text needs to be updated due to changes in text content, style, or available width.
        f32   m_LastMaxWidth{ Limits<f32>::max() }; ///< The last maximum width used for shaping the text.
    };

} // namespace RatUI