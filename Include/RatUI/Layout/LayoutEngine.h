#pragma once
#include "Layout.h"
#include <algorithm>

namespace RatUI
{
    // Forward declarations of layout functions
    Vec2<Unit> MeasureLayoutNode   ( LayoutNode& a_Node, Vec2<Unit> a_AvailableSize );
    void       ArrangeLayoutNode   ( LayoutNode& a_Node, Rect<Unit> a_AllocatedRect );
    
    void  ArrangeAnchored ( LayoutNode& a_Node, Rect<Unit> a_Container );
    void  ArrangeOverlay  ( LayoutNode& a_Node, Rect<Unit> a_Inner );
    void  ArrangeLinear   ( LayoutNode& a_Node, Rect<Unit> a_Inner );

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
    inline Vec2<Unit> ResolveChildArrangeSize( const LayoutNode& a_Child, Vec2<Unit> a_InnerSize )
    {
        Vec2<Unit> size = a_Child.Layout.DesiredSize;

        if ( a_Child.Style.WidthMode  == ESizingMode::Percent )
            size[0] = a_Child.Style.PercentWidth  * a_InnerSize[0];
        if ( a_Child.Style.HeightMode == ESizingMode::Percent )
            size[1] = a_Child.Style.PercentHeight * a_InnerSize[1];

        return size;
    }


    // TODO: These functions are getting pretty fat, should consider making RatUI a static lib or doing something like stb lib
    // TODO: Currently uses recursion but will need to switch to an iterative approach with an explicit stack for deep hierarchies to avoid stack overflow
    inline Vec2<Unit> MeasureLayoutNode( LayoutNode& a_Node, Vec2<Unit> a_AvailableSize )
    {
        // Collapsed LayoutNodes take no space
		if ( !Visibility::AffectsLayout( a_Node.Layout.Visibility ) )
        {
            a_Node.Layout.DesiredSize = Vec2<Unit>( 0_u, 0_u );
            return Vec2<Unit>( 0_u, 0_u );
        }
    
        const LayoutStyle& s = a_Node.Style;
        Vec2<Unit> desired( 0_u, 0_u );

        switch (s.WidthMode)
        {
            case ESizingMode::Fixed:   desired[0] = s.FixedWidth;                         break;
            case ESizingMode::Percent: desired[0] = s.PercentWidth  * a_AvailableSize[0]; break;
            case ESizingMode::Flex:
            case ESizingMode::Content: desired[0] = a_Node.Layout.IntrinsicSize[0];       break;
        }
    
        switch (s.HeightMode)
        {
            case ESizingMode::Fixed:   desired[1] = s.FixedHeight;                        break;
            case ESizingMode::Percent: desired[1] = s.PercentHeight * a_AvailableSize[1]; break;
            case ESizingMode::Flex:
            case ESizingMode::Content: desired[1] = a_Node.Layout.IntrinsicSize[1];       break;
        }
    
        // Accumulate children for content mode
        {
            const Vec2<Unit> padding = s.Padding.Total();

			// For Fixed/Percent parents the inner size is already determined, 
            // so we can use that to measure children with Percent sizing.
            // For Content/Flex parents the inner size isn't known yet, so fall back to the
            // outer available space minus padding.
            Vec2<Unit> childAvailSize;
            childAvailSize[0] = ( s.WidthMode  == ESizingMode::Fixed || s.WidthMode  == ESizingMode::Percent )
                                ? std::max( 0_u, desired[0] - s.Padding.Horizontal() )
                                : std::max( 0_u, a_AvailableSize[0] - s.Padding.Horizontal() );
            childAvailSize[1] = ( s.HeightMode == ESizingMode::Fixed || s.HeightMode == ESizingMode::Percent )
                                ? std::max( 0_u, desired[1] - s.Padding.Vertical() )
                                : std::max( 0_u, a_AvailableSize[1] - s.Padding.Vertical() );

            Vec2<Unit> contentSize = a_Node.Layout.IntrinsicSize; // Start with intrinsic size, if any
            u32        numFlow = 0;
        
            a_Node.ForEachChild( [&]( LayoutNode& child )
            {
                if ( !Visibility::AffectsLayout( child.Layout.Visibility ) )
                    return;
            
                if ( child.Style.PositionMode == EPositioningMode::Anchored )
                {
                    MeasureLayoutNode( child, a_AvailableSize );
                    return;
                }
            
                numFlow++;

                // Percent children size themselves relative to the parent's full inner size, not relative to siblings. TODO: This is apparently how CSS does it but it doesnt feel right. Research 
                // They must not inflate the parent's content size -
                // otherwise a Content-sized parent would grow to accommodate both its fixed
                // children and the full resolved Percent extent, double-counting the space.
                // We still call Measure so DesiredSize is populated for the Arrange pass.
                MeasureLayoutNode( child, childAvailSize );
                const bool isPercentW = child.Style.WidthMode  == ESizingMode::Percent;
                const bool isPercentH = child.Style.HeightMode == ESizingMode::Percent;
                const Vec2<Unit> childDesired(
                    isPercentW ? 0_u : child.Layout.DesiredSize[0] + child.Style.Margin.Horizontal(),
                    isPercentH ? 0_u : child.Layout.DesiredSize[1] + child.Style.Margin.Vertical()
                );
                                        
                switch ( s.LayoutType )
                {
                    case ELayoutType::Horizontal:
                        contentSize[0] += childDesired[0] + s.Spacing;
                        contentSize[1]  = std::max( contentSize[1], childDesired[1] );
                        break;
                
                    case ELayoutType::Vertical:
                        contentSize[0]  = std::max( contentSize[0], childDesired[0] );
                        contentSize[1] += childDesired[1] + s.Spacing;
                        break;
                
                    case ELayoutType::Overlay:
                        contentSize[0] = std::max( contentSize[0], childDesired[0] );
                        contentSize[1] = std::max( contentSize[1], childDesired[1] );
                        break;
                
                    case ELayoutType::Grid:
                        RATUI_USER_ASSERT( s.GridColumns != 0 || s.GridRows != 0, 
                            "Grid layout requires at least one of GridColumns or GridRows to be set" );

                        // TODO:
                        // This is harder than I thought.
                        // First we need to determine the number of columns and rows.
                        // Then we need to measure each child and keep track of the max width for each column and max height for each row.
                        // Then can sum them up to get the total content size.

                        // TODO: Might need to implement a frame allocator or something and give it to this function 

                        break;
                }
            } );
        
            // Remove trailing spacing added after last child
            if ( numFlow > 0 )
            {
                if ( s.LayoutType == ELayoutType::Horizontal ) contentSize[0] -= s.Spacing;
                if ( s.LayoutType == ELayoutType::Vertical )   contentSize[1] -= s.Spacing;
            }
        
            contentSize = contentSize + padding; // Add padding after calculating content size
        
            // If node is sized to content, use the accumulated content size
            if ( s.WidthMode  == ESizingMode::Content ) desired[0] = contentSize[0];
            if ( s.HeightMode == ESizingMode::Content ) desired[1] = contentSize[1];
        }
    
        // Clamp to constraints
        desired[0] = std::clamp( desired[0], s.SizeConstraints.MinSize[0], s.SizeConstraints.MaxSize[0] );
        desired[1] = std::clamp( desired[1], s.SizeConstraints.MinSize[1], s.SizeConstraints.MaxSize[1] );
    
        a_Node.Layout.DesiredSize = desired;
        return desired;
    }

    /** Resolves which alignment to use - child's SelfAlign overrides parent's ChildAlign */
    inline EAlignment ResolveAlign( const LayoutNode& a_Child, const LayoutNode& a_Parent )
    {
        return (a_Child.Style.SelfAlign != EAlignment::Inherit)
               ? a_Child.Style.SelfAlign
               : a_Parent.Style.ChildAlign;
    }

    /** Aligns a rectangle of the given content size within the container rect according to the specified alignment flags. */
    inline Rect<Unit> AlignRect( Vec2<Unit> a_ContentSize, Rect<Unit> a_Container, EAlignment a_Align )
    {
        Vec2<Unit> offset( 0_u, 0_u );

        if ( ( a_Align & EAlignment::HCenter ) == EAlignment::HCenter )
            offset[0] = ( a_Container.Size[0] - a_ContentSize[0] ) / 2.f;
        else if ( ( a_Align & EAlignment::Right ) == EAlignment::Right )
            offset[0] = a_Container.Size[0] - a_ContentSize[0];

        if ( ( a_Align & EAlignment::VCenter ) == EAlignment::VCenter )
            offset[1] = ( a_Container.Size[1] - a_ContentSize[1] ) / 2.f;
        else if ( ( a_Align & EAlignment::Bottom ) == EAlignment::Bottom )
            offset[1] = a_Container.Size[1] - a_ContentSize[1];

        return Rect{ .Origin = a_Container.Origin + offset, .Size = a_ContentSize };
    }

    /** Aligns a LayoutNode with an anchored position within a container. */
    inline void ArrangeAnchored( LayoutNode& a_Node, Rect<Unit> a_Container )
    {
        const Anchor&    anchor   = a_Node.Style.Anchor;
        const Vec2<Unit> parentSz = a_Container.Size;

        const bool stretchX = anchor.Min[0] != anchor.Max[0];
        const bool stretchY = anchor.Min[1] != anchor.Max[1];

        Vec2<Unit> origin( 0_u, 0_u ); // Top left corner of the node's final rect
        Vec2<Unit> size = a_Node.Layout.DesiredSize;

        // X axis
        if ( stretchX )
        {
            // Span the anchor region - offset pushes the edges inward symmetrically
            origin[0]  = a_Container.Origin[0] + ( parentSz[0] * anchor.Min[0] ) + anchor.Offset[0];
            Unit right = a_Container.Origin[0] + ( parentSz[0] * anchor.Max[0] ) - anchor.Offset[0];
            size[0] = std::max( 0_u, right - origin[0] );
        }
        else
        {
            // Point anchor - place pivot at anchor point, then nudge by offset
            Unit anchorX = a_Container.Origin[0] + ( parentSz[0] * anchor.Min[0] );
            origin[0] = anchorX - ( size[0] * anchor.Pivot[0] ) + anchor.Offset[0];
        }

        // Y axis
        if ( stretchY )
        {
            origin[1]   = a_Container.Origin[1] + ( parentSz[1] * anchor.Min[1] ) + anchor.Offset[1];
            Unit bottom = a_Container.Origin[1] + ( parentSz[1] * anchor.Max[1] ) - anchor.Offset[1];
            size[1] = std::max( 0_u, bottom - origin[1] );
        }
        else
        {
            // Point anchor - place pivot at anchor point, then nudge by offset
            Unit anchorY = a_Container.Origin[1] + ( parentSz[1] * anchor.Min[1] );
            origin[1]    = anchorY - ( size[1] * anchor.Pivot[1] ) + anchor.Offset[1];
        }

        ArrangeLayoutNode( a_Node, Rect<Unit>{ origin, size } );
    }

    /** Arranges children in an overlay layout, aligning each child within the inner rect according to its alignment flags. */
    inline void ArrangeOverlay( LayoutNode& a_Node, Rect<Unit> a_Inner )
    {
        a_Node.ForEachChild( [&]( LayoutNode& child )
        {
            if (child.Style.PositionMode == EPositioningMode::Anchored)
            {
                ArrangeAnchored(child, a_Inner);
                return;
            }

            // Re-resolve Percent dimensions now that the true inner size is known.
            Vec2<Unit> childSize = ResolveChildArrangeSize( child, a_Inner.Size );

            // Flex children fill the entire overlay container on their flex axis,
            // consistent with the cross-axis stretch behaviour in ArrangeLinear.
            if ( child.Style.WidthMode  == ESizingMode::Flex ) childSize[0] = a_Inner.Size[0];
            if ( child.Style.HeightMode == ESizingMode::Flex ) childSize[1] = a_Inner.Size[1];
            
            const Rect<Unit> childRect = AlignRect(childSize, a_Inner, ResolveAlign(child, a_Node));

            ArrangeLayoutNode(child, childRect);
        });
    }

    /** @brief Aligns a child LayoutNode along the cross axis - used by linear layouts to position children that don't stretch */
    inline Unit AlignCrossAxis( Unit a_ChildSize, Unit a_ParentPos,
                                Unit a_ParentSize, EAlignment a_Align, bool a_IsMainAxisHorizontal )
    {
        // Cross axis of horizontal layout is vertical - check V flags
        bool center = a_IsMainAxisHorizontal ? (a_Align & EAlignment::VCenter)
                                             : (a_Align & EAlignment::HCenter);
        bool end    = a_IsMainAxisHorizontal ? (a_Align & EAlignment::Bottom)
                                             : (a_Align & EAlignment::Right);

        if (center) return a_ParentPos + (a_ParentSize - a_ChildSize) * 0.5f;
        if (end)    return a_ParentPos +  a_ParentSize - a_ChildSize;
        return a_ParentPos;
    }

    /** Arranges children in a linear layout, positioning them along the main axis and aligning them on the cross axis. */
    inline void ArrangeLinear( LayoutNode& a_Node, Rect<Unit> a_Inner )
    {
        const LayoutStyle& s    = a_Node.Style;
        const bool         isHz = s.LayoutType == ELayoutType::Horizontal;

        Unit totalFixed      = 0_u;
        Unit flexMarginSpace = 0_u;
        f32  totalGrow       = 0.f;
        u32  numFlow         = 0;

        // -------------------------
        // Pass 1: Gather metrics
        // -------------------------

        a_Node.ForEachChild( [&]( const LayoutNode& child )
        {
            if ( child.Style.PositionMode == EPositioningMode::Anchored ) return;
            if ( !Visibility::AffectsLayout( child.Layout.Visibility ) )  return;

            const bool isFlexMain =
                ( isHz  && child.Style.WidthMode  == ESizingMode::Flex ) ||
                ( !isHz && child.Style.HeightMode == ESizingMode::Flex );

            const bool isPercentMain =
                ( isHz  && child.Style.WidthMode  == ESizingMode::Percent ) ||
                ( !isHz && child.Style.HeightMode == ESizingMode::Percent );

            const Unit marginMain =
                isHz ? child.Style.Margin.Horizontal()
                     : child.Style.Margin.Vertical();

            // Fixed space (exclude flex + percent)
            if ( !isFlexMain && !isPercentMain )
            {
                totalFixed += ( isHz ? child.Layout.DesiredSize[0]
                                     : child.Layout.DesiredSize[1] )
                            + marginMain;
            }

            // Flex margin space (flex only, percent excluded)
            if ( isFlexMain && !isPercentMain )
                flexMarginSpace += marginMain;

            // Grow weight (explicit or implicit)
            if ( child.Style.FlexGrow > 0.f )
            {
                totalGrow += child.Style.FlexGrow;
            }
            else if ( isFlexMain )
            {
                totalGrow += 1.f; // implicit grow
            }

            numFlow++;
        } );

        Unit available =
            ( isHz ? a_Inner.Size[0] : a_Inner.Size[1] )
            - s.Spacing * static_cast<f32>( std::max( 0u, numFlow - 1 ) );

        Unit leftover = std::max( 0_u, available - totalFixed - flexMarginSpace );

        // -------------------------
        // Pass 2: Arrange
        // -------------------------

        Unit cursor = isHz ? a_Inner.Origin[0] : a_Inner.Origin[1];

        a_Node.ForEachChild( [&]( LayoutNode& child )
        {
            if ( child.Style.PositionMode == EPositioningMode::Anchored )
            {
                ArrangeAnchored( child, a_Inner );
                return;
            }

            if ( !Visibility::AffectsLayout( child.Layout.Visibility ) )
                return;

            Vec2<Unit> childSize =
                ResolveChildArrangeSize( child, a_Inner.Size );

            const bool isFlexMain =
                ( isHz  && child.Style.WidthMode  == ESizingMode::Flex ) ||
                ( !isHz && child.Style.HeightMode == ESizingMode::Flex );

            f32 growWeight = child.Style.FlexGrow;
            if ( growWeight == 0.f && isFlexMain )
                growWeight = 1.f;

            // Flex distribution (percent never flexes)
            if ( growWeight > 0.f && totalGrow > 0.f )
            {
                const Constraints& c = child.Style.SizeConstraints;
                Unit share = leftover * ( growWeight / totalGrow );

                if ( isHz )
                    childSize[0] = std::clamp( isFlexMain ? share : childSize[0] + share,
                                               c.MinSize[0], c.MaxSize[0] );
                else
                    childSize[1] = std::clamp( isFlexMain ? share : childSize[1] + share,
                                               c.MinSize[1], c.MaxSize[1] );
            }

            // Cross-axis stretch
            EAlignment align = ResolveAlign( child, a_Node );

            if (  isHz && ( align & EAlignment::VStretch ) == EAlignment::VStretch ) childSize[1] = a_Inner.Size[1];
            if ( !isHz && ( align & EAlignment::HStretch ) == EAlignment::HStretch ) childSize[0] = a_Inner.Size[0];

            // Circular fallback
            if (  isHz && child.Style.HeightMode == ESizingMode::Flex )
                childSize[1] = a_Inner.Size[1];

            if ( !isHz && child.Style.WidthMode == ESizingMode::Flex )
                childSize[0] = a_Inner.Size[0];

            Rect<Unit> childRect;

            if ( isHz )
            {
                childRect.Origin[0] = cursor;
                childRect.Origin[1] =
                    AlignCrossAxis( childSize[1], a_Inner.Origin[1], a_Inner.Size[1], align, isHz );
            }
            else
            {
                childRect.Origin[0] =
                    AlignCrossAxis( childSize[0], a_Inner.Origin[0], a_Inner.Size[0], align, isHz );
                childRect.Origin[1] = cursor;
            }

            childRect.Size = childSize;

            // Apply margins
            childRect.Origin[0] += child.Style.Margin.Left;
            childRect.Origin[1] += child.Style.Margin.Top;
            childRect.Size[0]   -= child.Style.Margin.Horizontal();
            childRect.Size[1]   -= child.Style.Margin.Vertical();

            childRect.Size[0]    = std::max( 0_u, childRect.Size[0] );
            childRect.Size[1]    = std::max( 0_u, childRect.Size[1] );

            const Unit advance = isHz ? childSize[0] + child.Style.Margin.Horizontal()
                                      : childSize[1] + child.Style.Margin.Vertical();

            cursor += advance + s.Spacing;

            ArrangeLayoutNode( child, childRect );
        } );
    }

    inline void ArrangeGrid( LayoutNode& a_Node, Rect<Unit> a_Inner )
    {
        // TODO:
    }

    inline void ArrangeLayoutNode( LayoutNode& a_Node, Rect<Unit> a_AllocatedRect )
    {
        const LayoutStyle& s = a_Node.Style;
        a_Node.Layout.FinalRect = a_AllocatedRect;

        if ( !a_Node.FirstChild() )
            return; // No need to arrange children if there are none
        
        const Rect<Unit> inner = s.Padding.Apply( a_AllocatedRect );

        switch (s.LayoutType)
        {
            case ELayoutType::Horizontal:
            case ELayoutType::Vertical:
                ArrangeLinear(a_Node, inner);
                break;

            case ELayoutType::Overlay:
                ArrangeOverlay(a_Node, inner);
                break;

            case ELayoutType::Grid:
                // TODO: Implement grid layout arrangement logic
                break;
        }
    }

} // namespace RatUI
