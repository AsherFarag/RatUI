#include <RatUI/Widget/IWidget.h>

namespace RatUI
{
    LayoutNode& IWidget::GetLayout()
    { 
        // @note This should never fail because a widget should always be associated with a valid layout node for its entire lifetime. 
        // If this assertion fails, it indicates a critical bug in the widget lifecycle management 
        // (e.g., a widget being used after its layout node was destroyed).
        RATUI_ASSERT( GetScene().Layouts.IsValid( m_LayoutID ), 
            "Call to GetLayout() failed: widget is not associated with a valid layout node." ); 
        return *GetScene().Layouts.Get( m_LayoutID ); 
    }

    const LayoutNode& IWidget::GetLayout() const
    { 
        // @note This should never fail because a widget should always be associated with a valid layout node for its entire lifetime. 
        // If this assertion fails, it indicates a critical bug in the widget lifecycle management 
        // (e.g., a widget being used after its layout node was destroyed).
        RATUI_ASSERT( GetScene().Layouts.IsValid( m_LayoutID ), 
            "Call to GetLayout() failed: widget is not associated with a valid layout node." ); 
        return *GetScene().Layouts.Get( m_LayoutID ); 
    }
}