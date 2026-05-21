#pragma once
#include "Layout.h"

namespace RatUI
{
    /**
     * @brief Measures the desired size of a layout node based on its content and constraints,
     *        and recursively measures its children. This is the first pass of the layout process, 
     *        where each node calculates how much space it would like to have.
     * @param a_Node The layout node to measure.
     * @param a_AvailableSize The available size for this node to fit within, as determined by its parent. 
     * @return The desired size of the node after measurement, which may be used by its parent to allocate space during the Arrange phase.
     */
    Vec2<Unit> MeasureLayoutNode( LayoutNode& a_Node, Vec2<Unit> a_AvailableSize );

    /**
     * @brief Arranges a layout node within the given allocated rectangle, 
     *        positioning and sizing it according to its layout style and 
     *        the results of the Measure phase. This is the second pass of the layout process, 
     *        where each node is given a specific area to occupy and must position itself and its children within that area.
     * @param a_Node The layout node to arrange.
     * @param a_AllocatedRect The rectangle allocated for this node by its parent during the Arrange phase. 
     *                        The node should position itself and its children within this rectangle according to its layout style.
     */
    void       ArrangeLayoutNode( LayoutNode& a_Node, Rect<Unit> a_AllocatedRect );
} // namespace RatUI
