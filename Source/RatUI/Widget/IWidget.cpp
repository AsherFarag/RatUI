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