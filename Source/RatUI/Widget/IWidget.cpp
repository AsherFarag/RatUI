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

    void IWidget::SetNavRole( ENavRole a_NavRole )
    {
        const ENavRole previous = m_NavRole;
        m_NavRole = a_NavRole;
        if ( !m_Scene )
            return;

        // Navigation was inside it as a group, so it leaves.
        if ( previous == ENavRole::Group && a_NavRole != ENavRole::Group )
        {
            Array<NavScope>& stack = m_Scene->Input.NavStack;
            stack.erase( std::remove_if( stack.begin(), stack.end(), [this]( const NavScope& a_Scope ) { return a_Scope.Scope == m_LayoutID; } ),
                         stack.end() );
        }

        if ( a_NavRole == ENavRole::None && m_Scene->GetFocusedNode() == m_LayoutID )
            m_Scene->ClearFocus();
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