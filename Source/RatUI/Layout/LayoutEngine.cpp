#include <RatUI/Layout/LayoutEngine.h>
#include <RatUI/Widget/IWidget.h>

namespace RatUI
{
namespace
{
    struct GridDimensions
    {
        u32 Columns;
        u32 Rows;
    };

    /**
     * @brief Resolves the final arranged size of a child on both axes,
     * substituting Percent-based sizes from the known inner container size.
     */
    static Vec2<Unit> ResolveChildArrangeSize( const LayoutNode& a_Child, Vec2<Unit> a_InnerSize )
    {
        const LayoutStyle& s = a_Child.Style;
        Vec2<Unit> size = a_Child.Layout.DesiredSize;
        
        if ( s.WidthMode  == ESizing::Percent ) size[0] = s.PercentWidth  * a_InnerSize[0];
        if ( s.HeightMode == ESizing::Percent ) size[1] = s.PercentHeight * a_InnerSize[1];

        return size;
    }

    /**
     * @brief Resolves the grid column/row counts from the layout style and
     * the number of flow children.  At least one of GridColumns / GridRows
     * must be non-zero.  The other dimension is auto-calculated.  If both
     * are set but their product is smaller than a_NumFlowChildren, the
     * primary dimension (whichever was explicitly set) is kept fixed and
     * the other is expanded to fit.
     */
    static GridDimensions ResolveGridDimensions( const LayoutStyle& a_Style, u32 a_NumFlowChildren )
    {
        RATUI_USER_ASSERT( a_Style.GridColumns != 0 || a_Style.GridRows != 0,
                           "Grid layout requires at least one of GridColumns or GridRows to be set" );

        const auto CeilDiv = []( u32 a_Numerator, u32 a_Denominator ) -> u32
        {
            RATUI_USER_ASSERT( a_Denominator != 0, "Grid layout dimension cannot divide by zero" );
            return ( a_Numerator + a_Denominator - 1 ) / a_Denominator;
        };

        u32 cols = a_Style.GridColumns;
        u32 rows = a_Style.GridRows;

        if ( a_NumFlowChildren == 0 )
        {
            if ( cols == 0 ) cols = 1;
            if ( rows == 0 ) rows = 1;
            return { cols, rows };
        }

        if ( cols == 0 ) cols = std::max( 1u, CeilDiv( a_NumFlowChildren, rows ) );
        if ( rows == 0 ) rows = std::max( 1u, CeilDiv( a_NumFlowChildren, cols ) );

        // If both were explicitly set but don't fit all children, expand
        // whichever dimension was auto-resolved.
        if ( cols * rows < a_NumFlowChildren )
        {
            if ( a_Style.GridColumns != 0 ) rows = CeilDiv( a_NumFlowChildren, cols );
            else                            cols = CeilDiv( a_NumFlowChildren, rows );
        }

        return { std::max( 1u, cols ), std::max( 1u, rows ) };
    }

    // =========================================================================
    // Visibility
    // =========================================================================

    static EVisibility ResolveNodeVisibility( LayoutNode& a_Node )
    {
        if ( const LayoutNode* parent = a_Node.Parent() )
            a_Node.Layout.Visibility = Visibility::Apply( parent->Layout.Visibility, a_Node.Style.Visibility );
        else
            a_Node.Layout.Visibility = a_Node.Style.Visibility;

        return a_Node.Layout.Visibility;
    }

} // namespace

    // =========================================================================
    // Measure
    // =========================================================================

    Vec2<Unit> MeasureLayoutNode( LayoutNode& a_Node, Vec2<Unit> a_AvailableSize, LayoutContext& a_Ctx )
    {
        if ( !a_Node.Layout.IsDirty && 
             !a_Node.Layout.IsDescendantDirty && 
             a_Node.Layout.LastAvailableSize == a_AvailableSize )
        {
            return a_Node.Layout.DesiredSize; // Unchanged, skip entirely
        }

        ResolveNodeVisibility( a_Node );

        if ( !Visibility::AffectsLayout( a_Node.Layout.Visibility ) )
        {
            a_Node.Layout.DesiredSize = { 0_u, 0_u };
            return { 0_u, 0_u };
        }

        const LayoutStyle& s = a_Node.Style;
        Vec2<Unit> desired{ 0_u, 0_u };

        // - Step 1:
        // Resolve the desired size of the node itself, based on its sizing mode and available size.
        // If it is sized to content, measure the content size of the node's widget (if any) and use that as the desired size.
        // For example, if the widget is a text label, the content size would be the size of the text. 

             if ( s.WidthMode == ESizing::Fixed )    desired[0] = s.FixedWidth;
        else if ( s.WidthMode == ESizing::Percent )  desired[0] = s.PercentWidth * a_AvailableSize[0];

             if ( s.HeightMode == ESizing::Fixed )   desired[1] = s.FixedHeight;
        else if ( s.HeightMode == ESizing::Percent ) desired[1] = s.PercentHeight * a_AvailableSize[1];

        const Vec2<Unit> childAvailSize{
            ( s.WidthMode == ESizing::Fixed || s.WidthMode == ESizing::Percent )
                ? std::max( 0_u, desired[0] - s.Padding.Horizontal() )
                : std::max( 0_u, a_AvailableSize[0] - s.Padding.Horizontal() ),
            ( s.HeightMode == ESizing::Fixed || s.HeightMode == ESizing::Percent )
                ? std::max( 0_u, desired[1] - s.Padding.Vertical() )
                : std::max( 0_u, a_AvailableSize[1] - s.Padding.Vertical() )
        };

        Vec2<Unit> contentSize{ 0_u, 0_u };
        if ( a_Node.Widget )
			contentSize = a_Node.Widget->OnMeasureContent( a_Node, childAvailSize, a_Ctx );

        if ( s.WidthMode == ESizing::Flex  || s.WidthMode == ESizing::Content )  desired[0] = contentSize[0];
        if ( s.HeightMode == ESizing::Flex || s.HeightMode == ESizing::Content ) desired[1] = contentSize[1];

        // - Step 2:
        // Resolve the desired size of the node's children, based on the layout type and available size.

        const Vec2<Unit> padding = s.Padding.Total();
        u32              numFlow = 0;

        const auto computeChildDesired = +[]( const LayoutNode& child ) -> Vec2<Unit>
        {
            return Vec2<Unit>{
                child.Style.WidthMode  == ESizing::Percent ? 0_u : child.Layout.DesiredSize[0] + child.Style.Margin.Horizontal(),
                child.Style.HeightMode == ESizing::Percent ? 0_u : child.Layout.DesiredSize[1] + child.Style.Margin.Vertical()
            };
        };

        if ( s.LayoutType == ELayoutType::Horizontal || s.LayoutType == ELayoutType::Vertical )
        {
            const auto accumulateChild = [&]( const LayoutNode& child )
            {
                const Vec2<Unit> childDesired = computeChildDesired( child );
            
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
            
                    default:
                        RATUI_UNREACHABLE( "Measured node has had its LayoutType changed whilst measuring." );
                }
            };

            const bool isHz = s.LayoutType == ELayoutType::Horizontal;

            // Pass 1: 
            // Measure non-flex-main children at the full available size, and
            // accumulate totals for flex-main children, mirrors ArrangeLinear's first
            // pass, so flex-main children are measured against their true share of
            // space rather than the container's full inner size.
            Unit totalFixed = 0_u;
            Unit flexMarginSpace = 0_u;
            f32  totalGrow = 0.f;

            ScopedMark        flexMark( a_Ctx.Allocator );
            Span<LayoutNode*> flexMainChildren = a_Ctx.Allocator.Allocate<LayoutNode*>( a_Node.ChildCount() );
            u32               flexMainCount = 0;

            a_Node.ForEachChild( [&]( LayoutNode& child )
            {
                ResolveNodeVisibility( child );
                if ( !Visibility::AffectsLayout( child.Layout.Visibility ) ) return;

                if ( child.Style.PositionMode == EPositioning::Anchored )
                {
                    MeasureLayoutNode( child, a_AvailableSize, a_Ctx );
                    return;
                }

                numFlow++;

                const bool isFlexMain    = isHz ? ( child.Style.WidthMode == ESizing::Flex ) : ( child.Style.HeightMode == ESizing::Flex );
                const bool isPercentMain = isHz ? ( child.Style.WidthMode == ESizing::Percent ) : ( child.Style.HeightMode == ESizing::Percent );
                const Unit marginMain    = isHz ? child.Style.Margin.Horizontal() : child.Style.Margin.Vertical();

                if ( isFlexMain )
                {
					flexMainChildren[flexMainCount++] = &child;
                    if ( !isPercentMain ) 
                        flexMarginSpace += marginMain;

                    totalGrow += child.Style.FlexGrow > 0.f ? child.Style.FlexGrow : 1.f;
                    return; // measured in pass 2, once its share of space is known
                }

                MeasureLayoutNode( child, childAvailSize, a_Ctx );

                if ( !isPercentMain )
                    totalFixed += ( isHz ? child.Layout.DesiredSize[0] : child.Layout.DesiredSize[1] ) + marginMain;

                accumulateChild( child );
            } );

            const Unit availableMain = ( isHz ? childAvailSize[0] : childAvailSize[1] )
                - s.Spacing * static_cast<f32>( numFlow > 0 ? numFlow - 1 : 0 );
            const Unit leftover = std::max( 0_u, availableMain - totalFixed - flexMarginSpace );

            // Pass 2: 
            // Measure flex-main children with their true distributed share.
			for ( u32 i = 0; i < flexMainCount; ++i )
            {
                LayoutNode& child = *flexMainChildren[i];

                const f32          growWeight = child.Style.FlexGrow > 0.f ? child.Style.FlexGrow : 1.f;
                const Unit         share = totalGrow > 0.f ? leftover * ( growWeight / totalGrow ) : 0_u;
                const Constraints& c = child.Style.SizeConstraints;

                Vec2<Unit> flexAvail = childAvailSize;
                if ( isHz ) flexAvail[0] = std::clamp( share, c.Min[0], c.Max[0] );
                else        flexAvail[1] = std::clamp( share, c.Min[1], c.Max[1] );

                MeasureLayoutNode( child, flexAvail, a_Ctx );
                accumulateChild( child );
            }

			// Cache the totals for ArrangeLinear to use.
            a_Node.Layout.CachedLinear = { 
                .TotalFixed = totalFixed, 
                .FlexMarginSpace = flexMarginSpace, 
                .TotalGrow = totalGrow, 
                .NumFlow = numFlow 
            };
        }
        else // Overlay, Grid - no main-axis space-sharing between siblings
        {
            ScopedMark       gridSizesMark( a_Ctx.Allocator );
            Span<Vec2<Unit>> gridChildSizes = {};
            u32              gridChildCount = 0;

            if ( s.LayoutType == ELayoutType::Grid )
            {
                gridChildSizes = a_Ctx.Allocator.Allocate<Vec2<Unit>>( a_Node.ChildCount() );
            }

            const auto accumulateChild = [&]( const LayoutNode& child )
            {
                const Vec2<Unit> childDesired = computeChildDesired( child );
            
                switch ( s.LayoutType )
                {
                    case ELayoutType::Overlay:
                        contentSize[0] = std::max( contentSize[0], childDesired[0] );
                        contentSize[1] = std::max( contentSize[1], childDesired[1] );
                        break;
                
                    case ELayoutType::Grid:
                        gridChildSizes[gridChildCount++] = childDesired;
                        break;

                    default:
                        RATUI_UNREACHABLE( "Measured node has had its LayoutType changed whilst measuring." );
                }
            };

            a_Node.ForEachChild( [&]( LayoutNode& child )
            {
                ResolveNodeVisibility( child );
                if ( !Visibility::AffectsLayout( child.Layout.Visibility ) ) 
                {
                    return;
                }

                if ( child.Style.PositionMode == EPositioning::Anchored )
                {
                    MeasureLayoutNode( child, a_AvailableSize, a_Ctx );
                    return;
                }

                numFlow++;
                MeasureLayoutNode( child, childAvailSize, a_Ctx );
                accumulateChild( child );
            } );

            // Grid: sum per-track maximums to get the grid's intrinsic size.
            if ( s.LayoutType == ELayoutType::Grid && gridChildCount > 0 )
            {
                const GridDimensions dims = ResolveGridDimensions( s, static_cast<u32>( Size( gridChildSizes ) ) );

                ScopedMark trackMark( a_Ctx.Allocator );
                Span<Unit> colWidths = a_Ctx.Allocator.Allocate<Unit>( dims.Columns );
                Span<Unit> rowHeights = a_Ctx.Allocator.Allocate<Unit>( dims.Rows );
		    	std::fill( Begin( colWidths ), End( colWidths ), 0_u );
		    	std::fill( Begin( rowHeights ), End( rowHeights ), 0_u );

                for ( u32 i = 0; i < gridChildCount; ++i )
                {
                    const u32 row = i / dims.Columns, col = i % dims.Columns;
                    if ( row >= dims.Rows ) 
                        break;

                    colWidths[col]  = std::max( colWidths[col], gridChildSizes[i][0] );
                    rowHeights[row] = std::max( rowHeights[row], gridChildSizes[i][1] );
                }

                Unit gridW = 0_u;
                for ( Unit w : colWidths  ) 
                    gridW += w;

                Unit gridH = 0_u;
                for ( Unit h : rowHeights ) 
                    gridH += h;

                if ( dims.Columns > 1 ) gridW += s.Spacing * static_cast<f32>( dims.Columns - 1 );
                if ( dims.Rows    > 1 ) gridH += s.Spacing * static_cast<f32>( dims.Rows    - 1 );

                contentSize[0] = std::max( contentSize[0], gridW );
                contentSize[1] = std::max( contentSize[1], gridH );
            }
        }

        // Remove the trailing spacing that was added after the last child.
        if ( numFlow > 0 )
        {
                 if ( s.LayoutType == ELayoutType::Horizontal ) contentSize[0] = std::max( 0_u, contentSize[0] - s.Spacing );
            else if ( s.LayoutType == ELayoutType::Vertical   ) contentSize[1] = std::max( 0_u, contentSize[1] - s.Spacing );
        }

        contentSize = contentSize + padding;

        if ( s.WidthMode  == ESizing::Content ) desired[0] = contentSize[0];
        if ( s.HeightMode == ESizing::Content ) desired[1] = contentSize[1];

        // - Step 3:
        // Clamp the desired size to the node's size constraints, and store the final desired size in the layout node.

        desired[0] = std::clamp( desired[0], s.SizeConstraints.Min[0], s.SizeConstraints.Max[0] );
        desired[1] = std::clamp( desired[1], s.SizeConstraints.Min[1], s.SizeConstraints.Max[1] );

        a_Node.Layout.DesiredSize = desired;
        a_Node.Layout.LastAvailableSize = a_AvailableSize;

        // TODO: Since we mark the node as clean here, we can't check if a node is dirty during the arrange phase.
		// I might need to make the arrange phase clean the node instead of the measure phase idk
        a_Node.Layout.IsDirty = false;
        a_Node.Layout.IsDescendantDirty = false;

        return desired;
    }

    // =========================================================================
    // Alignment helpers
    // =========================================================================

namespace
{

    static EAlign ResolveAlign( const LayoutNode& a_Child, const LayoutNode& a_Parent )
    {
        return a_Child.Style.SelfAlign != EAlign::Inherit
            ? a_Child.Style.SelfAlign
            : a_Parent.Style.ChildAlign;
    }

    static Rect<Unit> AlignRect( Vec2<Unit> a_ContentSize, Rect<Unit> a_Container, EAlign a_Align )
    {
        Vec2<Unit> offset{ 0_u, 0_u };

             if ( HasFlag( a_Align, EAlign::HCenter ) ) offset[0] = ( a_Container.Size[0] - a_ContentSize[0] ) / 2.f;
        else if ( HasFlag( a_Align, EAlign::Right   ) ) offset[0] =   a_Container.Size[0] - a_ContentSize[0];

             if ( HasFlag( a_Align, EAlign::VCenter ) ) offset[1] = ( a_Container.Size[1] - a_ContentSize[1] ) / 2.f;
        else if ( HasFlag( a_Align, EAlign::Bottom  ) ) offset[1] =   a_Container.Size[1] - a_ContentSize[1];

        return { .Origin = a_Container.Origin + offset, .Size = a_ContentSize };
    }

    static Unit AlignCrossAxis( Unit a_ChildSize, Unit a_ParentPos, Unit a_ParentSize,
                                EAlign a_Align, bool a_IsMainAxisHorizontal )
    {
        const bool center = a_IsMainAxisHorizontal
            ? HasFlag( a_Align, EAlign::VCenter )
            : HasFlag( a_Align, EAlign::HCenter );

        const bool end = a_IsMainAxisHorizontal
            ? HasFlag( a_Align, EAlign::Bottom )
            : HasFlag( a_Align, EAlign::Right  );

        if ( center ) return a_ParentPos + ( a_ParentSize - a_ChildSize ) * 0.5f;
        if ( end    ) return a_ParentPos +   a_ParentSize - a_ChildSize;
        return a_ParentPos;
    }

    // =========================================================================
    // Arrange
    // =========================================================================

    static bool ArrangeAnchored( LayoutNode& a_Node, Rect<Unit> a_Container, LayoutContext& a_Ctx )
    {
        const Anchor&    anchor   = a_Node.Style.PositionAnchor;
        const Vec2<Unit> parentSz = a_Container.Size;

        const bool stretchX = anchor.Min[0] != anchor.Max[0];
        const bool stretchY = anchor.Min[1] != anchor.Max[1];

        Vec2<Unit> origin{ 0_u, 0_u };
        Vec2<Unit> size = a_Node.Layout.DesiredSize;

        if ( stretchX )
        {
            origin[0]  = a_Container.Origin[0] + ( parentSz[0] * anchor.Min[0] ) + anchor.Offset[0];
            Unit right = a_Container.Origin[0] + ( parentSz[0] * anchor.Max[0] ) - anchor.Offset[0];
            size[0]    = std::max( 0_u, right - origin[0] );
        }
        else
        {
            const Unit anchorX = a_Container.Origin[0] + ( parentSz[0] * anchor.Min[0] );
            origin[0] = anchorX - ( size[0] * anchor.Pivot[0] ) + anchor.Offset[0];
        }

        if ( stretchY )
        {
            origin[1]    = a_Container.Origin[1] + ( parentSz[1] * anchor.Min[1] ) + anchor.Offset[1];
            Unit bottom  = a_Container.Origin[1] + ( parentSz[1] * anchor.Max[1] ) - anchor.Offset[1];
            size[1]      = std::max( 0_u, bottom - origin[1] );
        }
        else
        {
            const Unit anchorY = a_Container.Origin[1] + ( parentSz[1] * anchor.Min[1] );
            origin[1] = anchorY - ( size[1] * anchor.Pivot[1] ) + anchor.Offset[1];
        }

        return ArrangeLayoutNode( a_Node, Rect<Unit>{ origin, size }, a_Ctx );
    }

    static bool ArrangeOverlay( LayoutNode& a_Node, Rect<Unit> a_Inner, LayoutContext& a_Ctx )
    {
        bool reflowed = false;
        a_Node.ForEachChild( [&]( LayoutNode& child )
        {
            ResolveNodeVisibility( child );

            if ( !Visibility::AffectsLayout( child.Layout.Visibility ) )
                return;

            if ( child.Style.PositionMode == EPositioning::Anchored )
            {
                reflowed |= ArrangeAnchored( child, a_Inner, a_Ctx );
                return;
            }

            Vec2<Unit> childSize = ResolveChildArrangeSize( child, a_Inner.Size );

            if (child.Style.WidthMode == ESizing::Flex) childSize[0] = std::max( 0_u, a_Inner.Size[0] - child.Style.Margin.Horizontal() );
            if (child.Style.HeightMode == ESizing::Flex) childSize[1] = std::max( 0_u, a_Inner.Size[1] - child.Style.Margin.Vertical() );

            // Align within the margin-inset container space
            const Rect<Unit> marginInnerRect{
                .Origin = { a_Inner.Origin[0] + child.Style.Margin.L, a_Inner.Origin[1] + child.Style.Margin.T },
                .Size   = { std::max( 0_u, a_Inner.Size[0] - child.Style.Margin.Horizontal() ),
                            std::max( 0_u, a_Inner.Size[1] - child.Style.Margin.Vertical() ) }
            };

            Rect<Unit> childRect = AlignRect( childSize, marginInnerRect, ResolveAlign( child, a_Node ) );
            reflowed |= ArrangeLayoutNode( child, childRect, a_Ctx );
        });
		return reflowed;
    }

    static bool ArrangeLinear( LayoutNode& a_Node, Rect<Unit> a_Inner, LayoutContext& a_Ctx )
    {
        const LayoutStyle&     s    = a_Node.Style;
        const bool             isHz = s.LayoutType == ELayoutType::Horizontal;
		const LinearAggregate& cached = a_Node.Layout.CachedLinear;

        const Unit available = ( isHz ? a_Inner.Size[0] : a_Inner.Size[1] )
            - s.Spacing * static_cast<f32>( cached.NumFlow > 0 ? cached.NumFlow - 1 : 0 );

        const Unit leftover = std::max( 0_u, available - cached.TotalFixed - cached.FlexMarginSpace );
        Unit cursor = isHz ? a_Inner.Origin[0] : a_Inner.Origin[1];

        // ---- Second pass: place each child ----

        bool reflowed = false;
        a_Node.ForEachChild( [&]( LayoutNode& child )
        {
            ResolveNodeVisibility( child );

            if ( child.Style.PositionMode == EPositioning::Anchored )
            {
                reflowed |= ArrangeAnchored( child, a_Inner, a_Ctx );
                return;
            }

            if ( !Visibility::AffectsLayout( child.Layout.Visibility ) )
                return;

            Vec2<Unit> childSize = ResolveChildArrangeSize( child, a_Inner.Size );

            const bool isFlexMain = ( isHz  && child.Style.WidthMode  == ESizing::Flex )
                                 || ( !isHz && child.Style.HeightMode  == ESizing::Flex );

            f32 growWeight = child.Style.FlexGrow > 0.f ? child.Style.FlexGrow : ( isFlexMain ? 1.f : 0.f );

            if ( growWeight > 0.f && cached.TotalGrow > 0.f )
            {
                const Constraints& c    = child.Style.SizeConstraints;
                const Unit         share = leftover * ( growWeight / cached.TotalGrow );

                if ( isHz )
                    childSize[0] = std::clamp( isFlexMain ? share : childSize[0] + share, c.Min[0], c.Max[0] );
                else
                    childSize[1] = std::clamp( isFlexMain ? share : childSize[1] + share, c.Min[1], c.Max[1] );
            }

            const EAlign align = ResolveAlign( child, a_Node );

            // Cross-axis flex/stretch fills the full cross-axis extent.
            if (isHz && (HasFlag( align, EAlign::VStretch ) || child.Style.HeightMode == ESizing::Flex))
                childSize[1] = std::max( 0_u, a_Inner.Size[1] - child.Style.Margin.Vertical() );
            if (!isHz && (HasFlag( align, EAlign::HStretch ) || child.Style.WidthMode == ESizing::Flex))
                childSize[0] = std::max( 0_u, a_Inner.Size[0] - child.Style.Margin.Horizontal() );

            Rect<Unit> childRect;

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

            childRect = child.Style.Margin.Apply( childRect );

            const Unit advance = isHz
                ? childSize[0] + child.Style.Margin.Horizontal()
                : childSize[1] + child.Style.Margin.Vertical();

            cursor += advance + s.Spacing;

            reflowed |= ArrangeLayoutNode( child, childRect, a_Ctx );
        });
		return reflowed;
    }

    static bool ArrangeGrid( LayoutNode& a_Node, Rect<Unit> a_Inner, LayoutContext& a_Ctx )
    {
        const LayoutStyle& s = a_Node.Style;
		BumpAllocator& alloc = a_Ctx.Allocator;

        // ---- Collect flow children ----

        ScopedMark flowMark( alloc );
        Span       flowChildren = alloc.Allocate<LayoutNode*>( a_Node.ChildCount() );
        u32        flowCount = 0;

		bool reflowed = false;
        a_Node.ForEachChild( [&]( LayoutNode& child )
        {
            ResolveNodeVisibility( child );

            if ( child.Style.PositionMode == EPositioning::Anchored )
            {
                reflowed |= ArrangeAnchored( child, a_Inner, a_Ctx );
                return;
            }

            if ( !Visibility::AffectsLayout( child.Layout.Visibility ) )
                return;

            flowChildren[flowCount++] = &child;
        });

		if ( flowCount == 0 )
            return reflowed;

        const GridDimensions dims = ResolveGridDimensions( s, static_cast<u32>( Size( flowChildren ) ) );

        // ---- Compute per-child arranged sizes ----

        // TODO: Replace with an arena allocator
        ScopedMark cellsMark( alloc );
        Span       childSizes = alloc.Allocate<Vec2<Unit>>( flowCount );
        Span       colWidths = alloc.Allocate<Unit>( dims.Columns );
        Span       rowHeights = alloc.Allocate<Unit>( dims.Rows );
		std::fill( Begin( colWidths ), End( colWidths ), 0_u );
        std::fill( Begin( rowHeights ), End( rowHeights ), 0_u );

		for ( u32 i = 0; i < flowCount; ++i )
        {
            const LayoutNode& child = *flowChildren[i];
            const u32 row = i / dims.Columns;
            const u32 col = i % dims.Columns;

            if ( row >= dims.Rows )
                break;

            const Vec2<Unit> childSize = ResolveChildArrangeSize( child, a_Inner.Size );
            childSizes[i] = childSize;

            const Vec2<Unit> occupied{
                childSize[0] + child.Style.Margin.Horizontal(),
                childSize[1] + child.Style.Margin.Vertical()
            };

            colWidths[col]   = std::max( colWidths[col],   occupied[0] );
            rowHeights[row]  = std::max( rowHeights[row],  occupied[1] );
        }

        // If both dimensions are explicitly set, divide the inner rect evenly
        // across all tracks so children fill the container.
        if ( s.GridColumns > 0 && s.GridRows > 0 )
        {
            const Unit hGaps = s.Spacing * static_cast<f32>( dims.Columns > 1 ? dims.Columns - 1 : 0 );
            const Unit vGaps = s.Spacing * static_cast<f32>( dims.Rows    > 1 ? dims.Rows    - 1 : 0 );

            const Unit colW = std::max( 0_u, a_Inner.Size[0] - hGaps ) / static_cast<f32>( dims.Columns );
            const Unit rowH = std::max( 0_u, a_Inner.Size[1] - vGaps ) / static_cast<f32>( dims.Rows    );

            for ( Unit& w : colWidths  ) w = colW;
            for ( Unit& h : rowHeights ) h = rowH;
        }

        // ---- Build cumulative origin arrays ----

        ScopedMark originsMark( alloc );
        Span colOrigins = alloc.Allocate<Unit>( dims.Columns );
        Span rowOrigins = alloc.Allocate<Unit>( dims.Rows );
        colOrigins[0]   = a_Inner.Origin[0];
        rowOrigins[0]   = a_Inner.Origin[1];

        for ( u32 c = 1; c < dims.Columns; ++c ) 
            colOrigins[c] = colOrigins[c - 1] + colWidths[c - 1] + s.Spacing;
        for ( u32 r = 1; r < dims.Rows; ++r ) 
            rowOrigins[r] = rowOrigins[r - 1] + rowHeights[r - 1] + s.Spacing;

        // ---- Arrange each child within its cell ----

		for ( u32 i = 0; i < flowCount; ++i )
        {
            LayoutNode& child = *flowChildren[i];
            const u32 row = i / dims.Columns;
            const u32 col = i % dims.Columns;

            if ( row >= dims.Rows )
                break;

            const Rect<Unit> cellRect{
                .Origin = { colOrigins[col], rowOrigins[row] },
                .Size   = { colWidths[col],  rowHeights[row] }
            };

            Vec2<Unit>   childSize = childSizes[i];
            const EAlign align     = ResolveAlign( child, a_Node );

            if ( HasFlag( align, EAlign::HStretch ) || child.Style.WidthMode  == ESizing::Flex ) childSize[0] = cellRect.Size[0];
            if ( HasFlag( align, EAlign::VStretch ) || child.Style.HeightMode == ESizing::Flex ) childSize[1] = cellRect.Size[1];

            Rect<Unit> childRect = AlignRect( childSize, cellRect, align );
			childRect            = child.Style.Margin.Apply( childRect );
            childRect.Size[0]    = std::max( 0_u, childRect.Size[0] );
            childRect.Size[1]    = std::max( 0_u, childRect.Size[1] );

            reflowed |= ArrangeLayoutNode( child, childRect, a_Ctx );
        }

		return reflowed;
    }

} // namespace

    bool ArrangeLayoutNode( LayoutNode& a_Node, Rect<Unit> a_AllocatedRect, LayoutContext& a_Ctx )
    {
        ResolveNodeVisibility( a_Node );
        a_Node.Layout.FinalRect = a_AllocatedRect;
    
        bool reflowed = false;
    
        if ( a_Node.Widget && a_Node.Widget->HasWidthDependentContent() )
        {
            const LayoutStyle& s = a_Node.Style;
            const Vec2<Unit>   contentSize = s.Padding.Apply( a_AllocatedRect ).Size;
            const Vec2<Unit>   newIntrinsic = a_Node.Widget->OnMeasureContent( a_Node, contentSize, a_Ctx );
    
            if ( s.HeightMode == ESizing::Content || s.HeightMode == ESizing::Flex )
            {
                const Unit newHeight = std::clamp( newIntrinsic[1] + s.Padding.Vertical(),
                                                   s.SizeConstraints.Min[1], s.SizeConstraints.Max[1] );
    
                if ( !IsApproxEqual( newHeight.ToFloat(), a_Node.Layout.DesiredSize[1].ToFloat() ) )
                {
                    a_Node.Layout.DesiredSize[1] = newHeight;
                    reflowed = true;
                }
            }
        }
    
        if ( a_Node.FirstChild() )
        {
            const Rect<Unit> inner = a_Node.Style.Padding.Apply( a_AllocatedRect );
    
            switch ( a_Node.Style.LayoutType )
            {
                case ELayoutType::Horizontal:
                case ELayoutType::Vertical:
                    reflowed |= ArrangeLinear( a_Node, inner, a_Ctx );
                    break;
    
                case ELayoutType::Overlay:
                    reflowed |= ArrangeOverlay( a_Node, inner, a_Ctx );
                    break;
    
                case ELayoutType::Grid:
                    reflowed |= ArrangeGrid( a_Node, inner, a_Ctx );
                    break;
            }
        }
    
        return reflowed;
    }

} // namespace RatUI
