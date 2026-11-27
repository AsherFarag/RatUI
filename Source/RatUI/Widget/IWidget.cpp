#include <RatUI/Widget/IWidget.h>
#include <RatUI/Widget/Scene.h>

#include <utility>

namespace RatUI
{
    LayoutNode& IWidget::GetLayout()
    { 
		LayoutNode* node = GetScene().GetLayoutNode( m_LayoutID );
        // @note This should never fail because a widget should always be associated with a valid layout node for its entire lifetime. 
        // If this assertion fails, it indicates a critical bug in the widget lifecycle management 
        // (e.g., a widget being used after its layout node was destroyed).
        RATUI_ASSERT( node, "Call to GetLayout() failed: widget is not associated with a valid layout node." ); 
        return *node;
    }

    void IWidget::PaintChildren( const PaintEvent& a_Event, LayoutNode& a_Node )
    {
        for ( LayoutNode* child = a_Node.FirstChild(); child; child = child->NextSibling() )
        {
            if ( child->Widget )
                child->Widget->Paint( a_Event );
            else if ( Visibility::IsRendered( child->Layout.Visibility ) )
                PaintChildren( a_Event, *child );
        }
    }

    bool IWidget::IsEngaged() const
    {
        return GetScene().IsEngaged( m_LayoutID );
    }

    void IWidget::SetFocusPolicy( EFocusPolicy a_Policy )
    {
        m_Traits.Focus = a_Policy;
        if ( m_Scene )
            m_Scene->OnFocusTraitsChanged( *this );
    }

    void IWidget::SetNavGroup( bool a_NavGroup )
    {
        if ( a_NavGroup && !m_Traits.NavGroup )
            m_FocusBeforeGroup = std::exchange( m_Traits.Focus, EFocusPolicy::All );
        else if ( !a_NavGroup && m_Traits.NavGroup )
            m_Traits.Focus = m_FocusBeforeGroup;
        m_Traits.NavGroup = a_NavGroup;
        if ( m_Scene )
            m_Scene->OnFocusTraitsChanged( *this );
    }

    const LayoutNode& IWidget::GetLayout() const
    { 
        const LayoutNode* node = GetScene().GetLayoutNode( m_LayoutID );
        // @note This should never fail because a widget should always be associated with a valid layout node for its entire lifetime. 
        // If this assertion fails, it indicates a critical bug in the widget lifecycle management 
        // (e.g., a widget being used after its layout node was destroyed).
        RATUI_ASSERT( node, "Call to GetLayout() failed: widget is not associated with a valid layout node." ); 
        return *node; 
    }
}