#pragma once
#include "Layout.h"
#include <algorithm>

namespace RatUI
{
    // Forward declarations of layout functions
    Vec2f MeasureLayoutNode   ( LayoutNode& a_LayoutNode, Vec2f a_AvailableSize );
    void  ArrangeLayoutNode   ( LayoutNode& a_LayoutNode, Rectf a_AllocatedRect );
    
    void  ArrangeAnchored ( LayoutNode& a_LayoutNode, Rectf a_Container );
    void  ArrangeOverlay  ( LayoutNode& a_LayoutNode, Rectf a_Inner );
    void  ArrangeLinear   ( LayoutNode& a_LayoutNode, Rectf a_Inner );

    EAlignment ResolveAlign( const LayoutNode& a_Child, const LayoutNode& a_Parent );
    Rectf AlignRect( Vec2f a_ContentSize, Rectf a_Container, EAlignment a_Align );
    f32   AlignCrossAxis( f32 a_ChildSize, f32 a_ParentPos,
                          f32 a_ParentSize, EAlignment a_Align, bool a_IsMainAxisHorizontal );


    // TODO: These functions are getting pretty fat, should consider making RatUI a static lib or doing something like stb lib
    // TODO: Currently uses recursion but will need to switch to an iterative approach with an explicit stack for deep hierarchies to avoid stack overflow
    inline Vec2f MeasureLayoutNode( LayoutNode& a_LayoutNode, Vec2f a_AvailableSize )
    {
        // Collapsed LayoutNodes take no space
        if (!a_LayoutNode.Layout.Visibility.AffectsLayout())
        {
            a_LayoutNode.Layout.DesiredSize = { 0.f, 0.f };
            return { 0.f, 0.f };
        }
    
        const LayoutStyle& s = a_LayoutNode.Style;
        Vec2f desired{ 0.0f, 0.0f };
    
        // Resolve width
        switch (s.WidthMode)
        {
            case ESizingMode::Fixed:   desired[0] = s.FixedWidth;                        break;
            case ESizingMode::Fill:    desired[0] = a_AvailableSize[0] * s.PercentWidth; break;
            case ESizingMode::Content: desired[0] = 0.f;                                 break;
        }
    
        // Resolve height
        switch (s.HeightMode)
        {
            case ESizingMode::Fixed:   desired[1] = s.FixedHeight;                        break;
            case ESizingMode::Fill:    desired[1] = a_AvailableSize[1] * s.PercentHeight; break;
            case ESizingMode::Content: desired[1] = 0.f;                                  break;
        }
    
        // Accumulate children for content mode
        {
            const Vec2f padding = s.Padding.Total();
            Vec2f       contentSize{ 0.0f, 0.0f };
            Vec2f       childAvailSize = a_AvailableSize - padding;
            u32         numFlow = 0;
        
            a_LayoutNode.ForEachChild( [&]( LayoutNode& child )
            {
                if ( !child.Layout.Visibility.AffectsLayout() )
                    return;
            
                if ( child.Style.PositionMode == EPositioningMode::Anchored )
                {
                    MeasureLayoutNode( child, a_AvailableSize );
                    return;
                }
            
                numFlow++;
            
                // If a child wants to Fill but the parent is Content-sized on that axis,
                // the dependency is circular — treat the child as Content for measure so
                // the parent can resolve its own size. Arrange will expand it correctly.
                ESizingMode effectiveW = child.Style.WidthMode;
                ESizingMode effectiveH = child.Style.HeightMode;
            
                if ( s.WidthMode  == ESizingMode::Content && effectiveW == ESizingMode::Fill )
                    effectiveW = ESizingMode::Content;
                if ( s.HeightMode == ESizingMode::Content && effectiveH == ESizingMode::Fill )
                    effectiveH = ESizingMode::Content;
            
                // Temporarily override sizing mode so recursive MeasureLayoutNode sees the fallback
                const ESizingMode savedW = child.Style.WidthMode;
                const ESizingMode savedH = child.Style.HeightMode;
            
                child.Style.WidthMode  = effectiveW;
                child.Style.HeightMode = effectiveH;
            
                const Vec2f childDesired = MeasureLayoutNode( child, childAvailSize )
                                         + Vec2f{ child.Style.Margin.Horizontal(),
                                                  child.Style.Margin.Vertical() };
                                        
                child.Style.WidthMode  = savedW;
                child.Style.HeightMode = savedH;
                                        
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
                        // TODO: Implement grid layout measurement
                        break;
                }
            } );
        
            // Remove trailing spacing added after last child
            if ( numFlow > 1 )
            {
                if ( s.LayoutType == ELayoutType::Horizontal ) contentSize[0] -= s.Spacing;
                if ( s.LayoutType == ELayoutType::Vertical )   contentSize[1] -= s.Spacing;
            }
        
            contentSize = contentSize + padding;
        
            if ( s.WidthMode  == ESizingMode::Content ) desired[0] = contentSize[0];
            if ( s.HeightMode == ESizingMode::Content ) desired[1] = contentSize[1];
        }
    
        // Clamp to constraints
        desired[0] = std::clamp( desired[0], s.SizeConstraints.MinSize[0], s.SizeConstraints.MaxSize[0] );
        desired[1] = std::clamp( desired[1], s.SizeConstraints.MinSize[1], s.SizeConstraints.MaxSize[1] );
    
        a_LayoutNode.Layout.DesiredSize = desired;
        return desired;
    }

    /** Resolves which alignment to use — child's SelfAlign overrides parent's ChildAlign */
    inline EAlignment ResolveAlign( const LayoutNode& a_Child, const LayoutNode& a_Parent )
    {
        return (a_Child.Style.SelfAlign != EAlignment::Inherit)
               ? a_Child.Style.SelfAlign
               : a_Parent.Style.ChildAlign;
    }

    /** Aligns a rectangle of the given content size within the container rect according to the specified alignment flags. */
    inline Rectf AlignRect( Vec2f a_ContentSize, Rectf a_Container, EAlignment a_Align )
    {
        Vec2f offset{ 0.0f, 0.0f };

        if ( ( a_Align & EAlignment::HCenter ) == EAlignment::HCenter )
            offset[0] = ( a_Container.Size[0] - a_ContentSize[0] ) / 2.f;
        else if ( ( a_Align & EAlignment::Right ) == EAlignment::Right )
            offset[0] = a_Container.Size[0] - a_ContentSize[0];

        if ( ( a_Align & EAlignment::VCenter ) == EAlignment::VCenter )
            offset[1] = ( a_Container.Size[1] - a_ContentSize[1] ) / 2.f;
        else if ( ( a_Align & EAlignment::Bottom ) == EAlignment::Bottom )
            offset[1] = a_Container.Size[1] - a_ContentSize[1];

        return Rectf{ .Origin = a_Container.Origin + offset, .Size = a_ContentSize };
    }

    /** Aligns a LayoutNode with an anchored position within a container. */
    inline void ArrangeAnchored( LayoutNode& a_LayoutNode, Rectf a_Container )
    {
        const Anchor& anchor   = a_LayoutNode.Style.Anchor;
        const Vec2f   parentSz = a_Container.Size;

        // Stretch case: Min != Max means the LayoutNode spans a region
        if (anchor.Min[0] != anchor.Max[0] || anchor.Min[1] != anchor.Max[1])
        {
            Vec2f finalMin = a_Container.Origin + parentSz * anchor.Min + anchor.Offset;
            Vec2f finalMax = a_Container.Origin + parentSz * anchor.Max - anchor.Offset;
            ArrangeLayoutNode( a_LayoutNode, { .Origin = finalMin, .Size = finalMax - finalMin } );
            return;
        }

        // Point anchor: position LayoutNode relative to anchor point, offset by pivot
        Vec2f anchorPoint = a_Container.Origin + parentSz * anchor.Min + anchor.Offset;
        Vec2f size        = a_LayoutNode.Layout.DesiredSize;
        Vec2f origin      = anchorPoint - size * anchor.Pivot; // pivot shifts origin

        ArrangeLayoutNode( a_LayoutNode, { .Origin = origin, .Size = size } );
    }

    /** Arranges children in an overlay layout, aligning each child within the inner rect according to its alignment flags. */
    inline void ArrangeOverlay( LayoutNode& a_LayoutNode, Rectf a_Inner )
    {
        a_LayoutNode.ForEachChild( [&]( LayoutNode& child )
        {
            if (child.Style.PositionMode == EPositioningMode::Anchored)
            {
                ArrangeAnchored(child, a_Inner);
                return;
            }

            Rectf childRect = AlignRect(child.Layout.DesiredSize, a_Inner, ResolveAlign(child, a_LayoutNode));
            ArrangeLayoutNode(child, childRect);
        });
    }

    /** @brief Aligns a child LayoutNode along the cross axis - used by linear layouts to position children that don't stretch */
    inline f32 AlignCrossAxis( f32 a_ChildSize, f32 a_ParentPos,
                               f32 a_ParentSize, EAlignment a_Align, bool a_IsMainAxisHorizontal )
    {
        // Cross axis of horizontal layout is vertical — check V flags
        bool center = a_IsMainAxisHorizontal ? (a_Align & EAlignment::VCenter)
                                             : (a_Align & EAlignment::HCenter);
        bool end    = a_IsMainAxisHorizontal ? (a_Align & EAlignment::Bottom)
                                             : (a_Align & EAlignment::Right);

        if (center) return a_ParentPos + (a_ParentSize - a_ChildSize) * 0.5f;
        if (end)    return a_ParentPos +  a_ParentSize - a_ChildSize;
        return a_ParentPos;
    }

    /** Arranges children in a linear layout, positioning them along the main axis and aligning them on the cross axis. */
    inline void ArrangeLinear( LayoutNode& a_LayoutNode, Rectf a_Inner )
    {
        const LayoutStyle& s    = a_LayoutNode.Style;
        const bool         isHz = s.LayoutType == ELayoutType::Horizontal;

        // Pass 1: sum fixed space and total grow weight
        f32 totalFixed = 0.f;
        f32 totalGrow  = 0.f;
        u32 numFlow    = 0;

        a_LayoutNode.ForEachChild( [&]( const LayoutNode& child )
        {
            if ( child.Style.PositionMode == EPositioningMode::Anchored ) return;
            if ( !child.Layout.Visibility.AffectsLayout() )               return;

            totalFixed += isHz ? child.Layout.DesiredSize[0] + child.Style.Margin.Horizontal()
                               : child.Layout.DesiredSize[1] + child.Style.Margin.Vertical();
            totalGrow  += child.Style.FlexGrow;
            numFlow++;
        } );

        f32 available = ( isHz ? a_Inner.Size[0] : a_Inner.Size[1] )
                      - ( s.Spacing * std::max( 0u, numFlow - 1 ) );
        f32 leftover  = std::max( 0.f, available - totalFixed );

        // Fill children that were measured as Content (due to circular dependency) also
        // claim a share of leftover space — treat them as FlexGrow 1 if they have no grow set.
        a_LayoutNode.ForEachChild( [&]( const LayoutNode& child )
        {
            if ( child.Style.PositionMode == EPositioningMode::Anchored ) return;
            if ( !child.Layout.Visibility.AffectsLayout() )               return;
            if ( child.Style.FlexGrow > 0.f )                             return;

            bool wantsFillW = child.Style.WidthMode  == ESizingMode::Fill && isHz;
            bool wantsFillH = child.Style.HeightMode == ESizingMode::Fill && !isHz;

            if ( wantsFillW || wantsFillH )
                totalGrow += 1.f; // implicit grow weight for Fill children with no explicit FlexGrow
        } );

        // Pass 2: assign rects
        f32 cursor = isHz ? a_Inner.Origin[0] : a_Inner.Origin[1];

        a_LayoutNode.ForEachChild( [&]( LayoutNode& child )
        {
            if ( child.Style.PositionMode == EPositioningMode::Anchored )
            {
                ArrangeAnchored( child, a_Inner );
                return;
            }

            if ( !child.Layout.Visibility.AffectsLayout() )
                return;

            Vec2f childSize = child.Layout.DesiredSize;

            // Determine effective grow weight — explicit FlexGrow or implicit 1 for Fill children
            f32 growWeight = child.Style.FlexGrow;
            if ( growWeight == 0.f )
            {
                bool wantsFillW = child.Style.WidthMode  == ESizingMode::Fill && isHz;
                bool wantsFillH = child.Style.HeightMode == ESizingMode::Fill && !isHz;
                if ( wantsFillW || wantsFillH )
                    growWeight = 1.f;
            }

            // Distribute leftover space
            if ( growWeight > 0.f && totalGrow > 0.f )
            {
                const Constraints& c     = child.Style.SizeConstraints;
                f32                share = leftover * ( growWeight / totalGrow );

                // TODO: Iterative clamped distribution — if a child hits MaxSize, remaining
                // leftover should redistribute to uncapped siblings. Not worth doing until needed.
                if ( isHz )
                    childSize[0] = std::clamp( childSize[0] + share, c.MinSize[0], c.MaxSize[0] );
                else
                    childSize[1] = std::clamp( childSize[1] + share, c.MinSize[1], c.MaxSize[1] );
            }

            // Fill on the cross axis — always expand regardless of grow
            EAlignment align = ResolveAlign( child, a_LayoutNode );
            if (  isHz && ( align & EAlignment::VStretch ) == EAlignment::VStretch ) childSize[1] = a_Inner.Size[1];
            if ( !isHz && ( align & EAlignment::HStretch ) == EAlignment::HStretch ) childSize[0] = a_Inner.Size[0];

            // Fill on cross axis from SizingMode (not just alignment stretch)
            if (  isHz && child.Style.HeightMode == ESizingMode::Fill ) childSize[1] = a_Inner.Size[1] * child.Style.PercentHeight;
            if ( !isHz && child.Style.WidthMode  == ESizingMode::Fill ) childSize[0] = a_Inner.Size[0] * child.Style.PercentWidth;

            // Position on main axis, align on cross axis
            Rectf childRect;
            if ( isHz )
            {
                childRect.Origin[0] = cursor;
                childRect.Origin[1] = AlignCrossAxis( childSize[1], a_Inner.Origin[1], a_Inner.Size[1], align, isHz );
            }
            else
            {
                childRect.Origin[0] = AlignCrossAxis( childSize[0], a_Inner.Origin[0], a_Inner.Size[0], align, isHz );
                childRect.Origin[1] = cursor;
            }
            childRect.Size = childSize;

            // Apply margins
            childRect.Origin[0] += child.Style.Margin.Left;
            childRect.Origin[1] += child.Style.Margin.Top;
            childRect.Size[0]   -= child.Style.Margin.Horizontal();
            childRect.Size[1]   -= child.Style.Margin.Vertical();

            childRect.Size[0] = std::max( 0.f, childRect.Size[0] );
            childRect.Size[1] = std::max( 0.f, childRect.Size[1] );

            cursor += s.Spacing;
            cursor += isHz ? childSize[0] + child.Style.Margin.Horizontal()
                           : childSize[1] + child.Style.Margin.Vertical();

            ArrangeLayoutNode( child, childRect );
        } );
    }

    inline void ArrangeGrid( LayoutNode& a_LayoutNode, Rectf a_Inner )
    {
        // TODO:
    }

    inline void ArrangeLayoutNode( LayoutNode& a_LayoutNode, Rectf a_AllocatedRect )
    {
        const LayoutStyle& s = a_LayoutNode.Style;
        a_LayoutNode.Layout.FinalRect = a_AllocatedRect;

        if (!a_LayoutNode.FirstChild)
            return; // No need to arrange children if there are none

        
        const Rectf inner = s.Padding.Apply( a_AllocatedRect );

        switch (s.LayoutType)
        {
            case ELayoutType::Horizontal:
            case ELayoutType::Vertical:
                ArrangeLinear(a_LayoutNode, inner);
                break;

            case ELayoutType::Overlay:
                ArrangeOverlay(a_LayoutNode, inner);
                break;

            case ELayoutType::Grid:
                // TODO: Implement grid layout arrangement logic
                break;
        }
    }

} // namespace RatUI