#include <RatUI/Widget/IWidget.h>
#include <RatUI/Widget/Scene.h>

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
        m_Traits.NavGroup = a_NavGroup;
        if ( a_NavGroup )
            m_Traits.Focus = EFocusPolicy::All;
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