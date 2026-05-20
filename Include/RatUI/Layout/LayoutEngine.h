#pragma once
#include "Layout.h"

namespace RatUI
{
    EVisibility ResolveNodeVisibility( LayoutNode& a_Node );

    Vec2<Unit> MeasureLayoutNode   ( LayoutNode& a_Node, Vec2<Unit> a_AvailableSize );
    void       ArrangeLayoutNode   ( LayoutNode& a_Node, Rect<Unit> a_AllocatedRect );
    
    void  ArrangeAnchored ( LayoutNode& a_Node, Rect<Unit> a_Container );
    void  ArrangeOverlay  ( LayoutNode& a_Node, Rect<Unit> a_Inner );
    void  ArrangeLinear   ( LayoutNode& a_Node, Rect<Unit> a_Inner );
    void  ArrangeGrid     ( LayoutNode& a_Node, Rect<Unit> a_Inner );

    EAlignment ResolveAlign( const LayoutNode& a_Child, const LayoutNode& a_Parent );
    Rect<Unit> AlignRect( Vec2<Unit> a_ContentSize, Rect<Unit> a_Container, EAlignment a_Align );
    Unit       AlignCrossAxis( Unit a_ChildSize, Unit a_ParentPos,
                               Unit a_ParentSize, EAlignment a_Align, bool a_IsMainAxisHorizontal );

    /**
     * @brief Resolves the final arranged size of a child node, recomputing Percent dimensions
     * from the now-known inner container size. Called during Arrange to get the authoritative
     * size for children whose Percent base wasn't fully resolved during Measure (e.g. when the
     * parent itself is Flex-sized and its true extent wasn't known until Arrange time).
     */
} // namespace RatUI
