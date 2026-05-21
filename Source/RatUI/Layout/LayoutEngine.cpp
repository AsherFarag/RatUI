#include <RatUI/Layout/LayoutEngine.h>

namespace RatUI
{
    namespace
    {
        struct GridDimensions
        {
            u32 Columns;
            u32 Rows;
        };
    } // namespace

    /**
     * @brief Resolves the final arranged size of a child on both axes,
     * substituting Percent-based sizes from the known inner container size.
     */
    static Vec2<Unit> ResolveChildArrangeSize( const LayoutNode& a_Child, Vec2<Unit> a_InnerSize )
    {
        const LayoutStyle& s = a_Child.Style;
        Vec2<Unit> size = a_Child.Layout.DesiredSize;
        
        if ( s.WidthMode  == ESizingMode::Percent ) size[0] = s.PercentWidth  * a_InnerSize[0];
        if ( s.HeightMode == ESizingMode::Percent ) size[1] = s.PercentHeight * a_InnerSize[1];

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

    // =========================================================================
    // Measure
    // =========================================================================

    Vec2<Unit> MeasureLayoutNode( LayoutNode& a_Node, Vec2<Unit> a_AvailableSize )
    {
        ResolveNodeVisibility( a_Node );

        if ( !Visibility::AffectsLayout( a_Node.Layout.Visibility ) )
        {
            a_Node.Layout.DesiredSize = { 0_u, 0_u };
            return { 0_u, 0_u };
        }

        const LayoutStyle& s = a_Node.Style;
        Vec2<Unit> desired{ 0_u, 0_u };

        switch ( s.WidthMode )
        {
            case ESizingMode::Fixed:   desired[0] = s.FixedWidth; break;
            case ESizingMode::Percent: desired[0] = s.PercentWidth * a_AvailableSize[0]; break;
            case ESizingMode::Flex:
            case ESizingMode::Content: desired[0] = a_Node.Layout.IntrinsicSize[0]; break;
        }

        switch ( s.HeightMode )
        {
            case ESizingMode::Fixed:   desired[1] = s.FixedHeight; break;
            case ESizingMode::Percent: desired[1] = s.PercentHeight * a_AvailableSize[1]; break;
            case ESizingMode::Flex:
            case ESizingMode::Content: desired[1] = a_Node.Layout.IntrinsicSize[1]; break;
        }

        {
            const Vec2<Unit> padding = s.Padding.Total();

            // Children get the inner available size - either derived from a
            // known fixed/percent dimension or from the available size passed in.
            Vec2<Unit> childAvailSize;
            childAvailSize[0] = ( s.WidthMode  == ESizingMode::Fixed || s.WidthMode  == ESizingMode::Percent )
                ? std::max( 0_u, desired[0] - s.Padding.Horizontal() )
                : std::max( 0_u, a_AvailableSize[0] - s.Padding.Horizontal() );
            childAvailSize[1] = ( s.HeightMode == ESizingMode::Fixed || s.HeightMode == ESizingMode::Percent )
                ? std::max( 0_u, desired[1] - s.Padding.Vertical() )
                : std::max( 0_u, a_AvailableSize[1] - s.Padding.Vertical() );

            Vec2<Unit>   contentSize         = a_Node.Layout.IntrinsicSize;
            u32          numFlow             = 0;

            // TODO: Replace with an arena allocator
            Array<Vec2<Unit>> gridChildSizes = {}; // Only populated for Grid layouts

            a_Node.ForEachChild( [&]( LayoutNode& child )
            {
                ResolveNodeVisibility( child );

                if ( !Visibility::AffectsLayout( child.Layout.Visibility ) )
                    return;

                if ( child.Style.PositionMode == EPositioningMode::Anchored )
                {
                    // Anchored children are measured but don't contribute to
                    // the parent's content size.
                    MeasureLayoutNode( child, a_AvailableSize );
                    return;
                }

                numFlow++;

                MeasureLayoutNode( child, childAvailSize );

                // Percent children don't contribute to the parent's intrinsic
                // size - they depend on it, not the other way around.
                const Vec2<Unit> childDesired{
                    child.Style.WidthMode  == ESizingMode::Percent ? 0_u : child.Layout.DesiredSize[0] + child.Style.Margin.Horizontal(),
                    child.Style.HeightMode == ESizingMode::Percent ? 0_u : child.Layout.DesiredSize[1] + child.Style.Margin.Vertical()
                };

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
                        EmplaceBack( gridChildSizes, childDesired );
                        break;
                }
            });

            // Remove the trailing spacing that was added after the last child.
            if ( numFlow > 0 )
            {
                if ( s.LayoutType == ELayoutType::Horizontal ) contentSize[0] -= s.Spacing;
                if ( s.LayoutType == ELayoutType::Vertical   ) contentSize[1] -= s.Spacing;
            }

            // Grid: sum per-track maximums to get the grid's intrinsic size.
            if ( s.LayoutType == ELayoutType::Grid && !Empty( gridChildSizes ) )
            {
                const GridDimensions dims = ResolveGridDimensions( s, static_cast<u32>( Size( gridChildSizes ) ) );

                // TODO: Replace with an arena allocator
                Array<Unit> colWidths ( dims.Columns, 0_u );
                Array<Unit> rowHeights( dims.Rows,    0_u );

                for ( u32 i = 0; i < Size( gridChildSizes ); ++i )
                {
                    const u32 row = i / dims.Columns;
                    const u32 col = i % dims.Columns;

                    if ( row >= dims.Rows )
                        break;

                    colWidths[col]   = std::max( colWidths[col],   gridChildSizes[i][0] );
                    rowHeights[row]  = std::max( rowHeights[row],  gridChildSizes[i][1] );
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

            contentSize = contentSize + padding;

            if ( s.WidthMode  == ESizingMode::Content ) desired[0] = contentSize[0];
            if ( s.HeightMode == ESizingMode::Content ) desired[1] = contentSize[1];
        }

        desired[0] = std::clamp( desired[0], s.SizeConstraints.MinSize[0], s.SizeConstraints.MaxSize[0] );
        desired[1] = std::clamp( desired[1], s.SizeConstraints.MinSize[1], s.SizeConstraints.MaxSize[1] );

        a_Node.Layout.DesiredSize = desired;
        return desired;
    }

    // =========================================================================
    // Alignment helpers
    // =========================================================================

    static EAlignment ResolveAlign( const LayoutNode& a_Child, const LayoutNode& a_Parent )
    {
        return a_Child.Style.SelfAlign != EAlignment::Inherit
            ? a_Child.Style.SelfAlign
            : a_Parent.Style.ChildAlign;
    }

    static Rect<Unit> AlignRect( Vec2<Unit> a_ContentSize, Rect<Unit> a_Container, EAlignment a_Align )
    {
        Vec2<Unit> offset{ 0_u, 0_u };

             if ( HasFlag( a_Align, EAlignment::HCenter ) ) offset[0] = ( a_Container.Size[0] - a_ContentSize[0] ) / 2.f;
        else if ( HasFlag( a_Align, EAlignment::Right   ) ) offset[0] =   a_Container.Size[0] - a_ContentSize[0];

             if ( HasFlag( a_Align, EAlignment::VCenter ) ) offset[1] = ( a_Container.Size[1] - a_ContentSize[1] ) / 2.f;
        else if ( HasFlag( a_Align, EAlignment::Bottom  ) ) offset[1] =   a_Container.Size[1] - a_ContentSize[1];

        return { .Origin = a_Container.Origin + offset, .Size = a_ContentSize };
    }

    static Unit AlignCrossAxis( Unit a_ChildSize, Unit a_ParentPos, Unit a_ParentSize,
                                EAlignment a_Align, bool a_IsMainAxisHorizontal )
    {
        const bool center = a_IsMainAxisHorizontal
            ? HasFlag( a_Align, EAlignment::VCenter )
            : HasFlag( a_Align, EAlignment::HCenter );

        const bool end = a_IsMainAxisHorizontal
            ? HasFlag( a_Align, EAlignment::Bottom )
            : HasFlag( a_Align, EAlignment::Right  );

        if ( center ) return a_ParentPos + ( a_ParentSize - a_ChildSize ) * 0.5f;
        if ( end    ) return a_ParentPos +   a_ParentSize - a_ChildSize;
        return a_ParentPos;
    }

    // =========================================================================
    // Arrange
    // =========================================================================

    static void ArrangeAnchored( LayoutNode& a_Node, Rect<Unit> a_Container )
    {
        const Anchor&    anchor   = a_Node.Style.Anchor;
        const Vec2<Unit> parentSz = a_Container.Size;

        const bool stretchX = anchor.Min[0] != anchor.Max[0];
        const bool stretchY = anchor.Min[1] != anchor.Max[1];

        Vec2<Unit> origin{ 0_u, 0_u };
        Vec2<Unit> size = a_Node.Layout.DesiredSize;

        if ( stretchX )
        {
            origin[0]  = a_Container.Origin[0] + ( parentSz[0] * anchor.Min[0] ) + anchor.Offset[0];
            Unit right = a_Container.Origin[0] + ( parentSz[0] * anchor.Max[0] ) - anchor.Offset[0];
            size[0] = std::max( 0_u, right - origin[0] );
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
            size[1] = std::max( 0_u, bottom - origin[1] );
        }
        else
        {
            const Unit anchorY = a_Container.Origin[1] + ( parentSz[1] * anchor.Min[1] );
            origin[1] = anchorY - ( size[1] * anchor.Pivot[1] ) + anchor.Offset[1];
        }

        ArrangeLayoutNode( a_Node, Rect<Unit>{ origin, size } );
    }

    static void ArrangeOverlay( LayoutNode& a_Node, Rect<Unit> a_Inner )
    {
        a_Node.ForEachChild( [&]( LayoutNode& child )
        {
            ResolveNodeVisibility( child );

            if ( !Visibility::AffectsLayout( child.Layout.Visibility ) )
                return;

            if ( child.Style.PositionMode == EPositioningMode::Anchored )
            {
                ArrangeAnchored( child, a_Inner );
                return;
            }

            Vec2<Unit> childSize = ResolveChildArrangeSize( child, a_Inner.Size );

            if ( child.Style.WidthMode  == ESizingMode::Flex ) childSize[0] = a_Inner.Size[0];
            if ( child.Style.HeightMode == ESizingMode::Flex ) childSize[1] = a_Inner.Size[1];

            const Rect<Unit> childRect = AlignRect( childSize, a_Inner, ResolveAlign( child, a_Node ) );
            ArrangeLayoutNode( child, childRect );
        });
    }

    static void ArrangeLinear( LayoutNode& a_Node, Rect<Unit> a_Inner )
    {
        const LayoutStyle& s    = a_Node.Style;
        const bool         isHz = s.LayoutType == ELayoutType::Horizontal;

        // ---- First pass: accumulate fixed sizes and flex weights ----

        Unit totalFixed     = 0_u;
        Unit flexMarginSpace = 0_u;
        f32  totalGrow      = 0.f;
        u32  numFlow        = 0;

        a_Node.ForEachChild( [&]( const LayoutNode& child )
        {
            const EVisibility childVis = Visibility::Apply( a_Node.Layout.Visibility, child.Style.Visibility );

            if ( child.Style.PositionMode == EPositioningMode::Anchored ) return;
            if ( !Visibility::AffectsLayout( childVis ) ) return;

            const bool isFlexMain    = ( isHz && child.Style.WidthMode  == ESizingMode::Flex )
                                    || ( !isHz && child.Style.HeightMode == ESizingMode::Flex );
            const bool isPercentMain = ( isHz && child.Style.WidthMode  == ESizingMode::Percent )
                                    || ( !isHz && child.Style.HeightMode == ESizingMode::Percent );

            const Unit marginMain = isHz ? child.Style.Margin.Horizontal() : child.Style.Margin.Vertical();

            if ( !isFlexMain && !isPercentMain )
                totalFixed += ( isHz ? child.Layout.DesiredSize[0] : child.Layout.DesiredSize[1] ) + marginMain;

            if ( isFlexMain && !isPercentMain )
                flexMarginSpace += marginMain;

            totalGrow += child.Style.FlexGrow > 0.f ? child.Style.FlexGrow : ( isFlexMain ? 1.f : 0.f );

            ++numFlow;
        });

        const Unit available = ( isHz ? a_Inner.Size[0] : a_Inner.Size[1] )
            - s.Spacing * static_cast<f32>( numFlow > 0 ? numFlow - 1 : 0 );

        const Unit leftover = std::max( 0_u, available - totalFixed - flexMarginSpace );
        Unit cursor = isHz ? a_Inner.Origin[0] : a_Inner.Origin[1];

        // ---- Second pass: place each child ----

        a_Node.ForEachChild( [&]( LayoutNode& child )
        {
            ResolveNodeVisibility( child );

            if ( child.Style.PositionMode == EPositioningMode::Anchored )
            {
                ArrangeAnchored( child, a_Inner );
                return;
            }

            if ( !Visibility::AffectsLayout( child.Layout.Visibility ) )
                return;

            Vec2<Unit> childSize = ResolveChildArrangeSize( child, a_Inner.Size );

            const bool isFlexMain = ( isHz  && child.Style.WidthMode  == ESizingMode::Flex )
                                 || ( !isHz && child.Style.HeightMode  == ESizingMode::Flex );

            f32 growWeight = child.Style.FlexGrow > 0.f ? child.Style.FlexGrow : ( isFlexMain ? 1.f : 0.f );

            if ( growWeight > 0.f && totalGrow > 0.f )
            {
                const Constraints& c    = child.Style.SizeConstraints;
                const Unit         share = leftover * ( growWeight / totalGrow );

                if ( isHz )
                    childSize[0] = std::clamp( isFlexMain ? share : childSize[0] + share, c.MinSize[0], c.MaxSize[0] );
                else
                    childSize[1] = std::clamp( isFlexMain ? share : childSize[1] + share, c.MinSize[1], c.MaxSize[1] );
            }

            const EAlignment align = ResolveAlign( child, a_Node );

            // Cross-axis flex/stretch fills the full cross-axis extent.
            if (  isHz && ( HasFlag( align, EAlignment::VStretch ) || child.Style.HeightMode == ESizingMode::Flex ) ) childSize[1] = a_Inner.Size[1];
            if ( !isHz && ( HasFlag( align, EAlignment::HStretch ) || child.Style.WidthMode  == ESizingMode::Flex ) ) childSize[0] = a_Inner.Size[0];

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

            // Apply margins (inset the rect, not offset the origin).
            childRect.Origin[0] += child.Style.Margin.Left;
            childRect.Origin[1] += child.Style.Margin.Top;
            childRect.Size[0]   -= child.Style.Margin.Horizontal();
            childRect.Size[1]   -= child.Style.Margin.Vertical();
            childRect.Size[0]    = std::max( 0_u, childRect.Size[0] );
            childRect.Size[1]    = std::max( 0_u, childRect.Size[1] );

            const Unit advance = isHz
                ? childSize[0] + child.Style.Margin.Horizontal()
                : childSize[1] + child.Style.Margin.Vertical();

            cursor += advance + s.Spacing;

            ArrangeLayoutNode( child, childRect );
        });
    }

    static void ArrangeGrid( LayoutNode& a_Node, Rect<Unit> a_Inner )
    {
        const LayoutStyle& s = a_Node.Style;

        // ---- Collect flow children ----

        // TODO: Replace with an arena allocator
        Array<LayoutNode*> flowChildren;

        a_Node.ForEachChild( [&]( LayoutNode& child )
        {
            ResolveNodeVisibility( child );

            if ( child.Style.PositionMode == EPositioningMode::Anchored )
            {
                ArrangeAnchored( child, a_Inner );
                return;
            }

            if ( !Visibility::AffectsLayout( child.Layout.Visibility ) )
                return;

            PushBack( flowChildren, &child );
        });

        if ( Empty( flowChildren ) )
            return;

        const GridDimensions dims = ResolveGridDimensions( s, static_cast<u32>( Size( flowChildren ) ) );

        // ---- Compute per-child arranged sizes ----

        // TODO: Replace with an arena allocator
        Array<Vec2<Unit>> childSizes( Size( flowChildren ), Vec2<Unit>{ 0_u, 0_u } );
        Array<Unit>       colWidths ( dims.Columns, 0_u );
        Array<Unit>       rowHeights( dims.Rows,    0_u );

        for ( u32 i = 0; i < Size( flowChildren ); ++i )
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

        // TODO: Replace with an arena allocator
        Array<Unit> colOrigins( dims.Columns, a_Inner.Origin[0] );
        Array<Unit> rowOrigins( dims.Rows,    a_Inner.Origin[1] );

        for ( u32 c = 1; c < dims.Columns; ++c )
            colOrigins[c] = colOrigins[c - 1] + colWidths[c - 1]  + s.Spacing;

        for ( u32 r = 1; r < dims.Rows; ++r )
            rowOrigins[r] = rowOrigins[r - 1] + rowHeights[r - 1] + s.Spacing;

        // ---- Arrange each child within its cell ----

        for ( u32 i = 0; i < Size( flowChildren ); ++i )
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

            Vec2<Unit>       childSize = childSizes[i];
            const EAlignment align     = ResolveAlign( child, a_Node );

            if ( HasFlag( align, EAlignment::HStretch ) || child.Style.WidthMode  == ESizingMode::Flex ) childSize[0] = cellRect.Size[0];
            if ( HasFlag( align, EAlignment::VStretch ) || child.Style.HeightMode == ESizingMode::Flex ) childSize[1] = cellRect.Size[1];

            Rect<Unit> childRect = AlignRect( childSize, cellRect, align );
            childRect.Origin[0] += child.Style.Margin.Left;
            childRect.Origin[1] += child.Style.Margin.Top;
            childRect.Size[0]   -= child.Style.Margin.Horizontal();
            childRect.Size[1]   -= child.Style.Margin.Vertical();
            childRect.Size[0]    = std::max( 0_u, childRect.Size[0] );
            childRect.Size[1]    = std::max( 0_u, childRect.Size[1] );

            ArrangeLayoutNode( child, childRect );
        }
    }

    void ArrangeLayoutNode( LayoutNode& a_Node, Rect<Unit> a_AllocatedRect )
    {
        ResolveNodeVisibility( a_Node );

        a_Node.Layout.FinalRect = a_AllocatedRect;

        if ( !a_Node.FirstChild() )
            return;

        const Rect<Unit> inner = a_Node.Style.Padding.Apply( a_AllocatedRect );

        switch ( a_Node.Style.LayoutType )
        {
            case ELayoutType::Horizontal:
            case ELayoutType::Vertical:
                ArrangeLinear( a_Node, inner );
                break;

            case ELayoutType::Overlay:
                ArrangeOverlay( a_Node, inner );
                break;

            case ELayoutType::Grid:
                ArrangeGrid( a_Node, inner );
                break;
        }
    }

} // namespace RatUI
