#pragma once
#include "Layout.h"

namespace RatUI
{
    struct BumpAllocator
    {
        byte* Buffer;
		u32   Capacity;
        u32   Offset;

        explicit BumpAllocator( byte* a_Buffer, u32 a_Capacity ) noexcept
			: Buffer( a_Buffer ), Capacity( a_Capacity ), Offset( 0 )
		{}

		BumpAllocator( const BumpAllocator& ) = delete;
		BumpAllocator& operator=( const BumpAllocator& ) = delete;

        Span<byte> Allocate( u32 a_Size, u32 a_Alignment = 1 )
        {
			RATUI_ASSERT( a_Alignment > 0 && ( a_Alignment & ( a_Alignment - 1 ) ) == 0, 
                          "Alignment must be a power of two." );

			RATUI_USER_ASSERT( Offset + a_Size <= Capacity,
                               "Layout ran out of scratch memory: raise it with Scene::SetLayoutScratchSize." );

			u32 alignedOffset = ( Offset + ( a_Alignment - 1 ) ) & ~( a_Alignment - 1 );
			Offset = alignedOffset + a_Size;
			return Span<byte>( Buffer + alignedOffset, a_Size );
        }

		template<typename T> 
            requires ( std::is_trivially_destructible_v<T> )
        Span<T> Allocate( u32 a_Count )
        {
			Span<byte> mem = Allocate( sizeof( T ) * a_Count, alignof( T ) );
            return Span<T>( reinterpret_cast<T*>( Data( mem ) ), a_Count );
        }

		u32 Mark() const noexcept
        { 
            return Offset;
        }

		void Rollback( u32 a_Mark ) noexcept
        { 
			RATUI_USER_ASSERT( a_Mark <= Offset, 
                               "Rollback mark is out of bounds." );
            Offset = a_Mark; 
        }
    };

    struct ScopedMark
    {
        BumpAllocator& Allocator;
        u32            SavedOffset;

        explicit ScopedMark( BumpAllocator& a_Allocator ) 
            : Allocator( a_Allocator ), SavedOffset( a_Allocator.Mark() ) 
        {}

        ~ScopedMark() { Allocator.Rollback( SavedOffset ); }

        ScopedMark( const ScopedMark& ) = delete;
        ScopedMark& operator=( const ScopedMark& ) = delete;
    };

    /** @brief A node whose size or content size layout changed, with what they were before. */
    struct GeometryChange
    {
        LayoutNode* Node{ nullptr };
        Vec2<Unit>  PreviousSize{};
        Vec2<Unit>  PreviousContentSize{};
    };

    struct LayoutContext
    {
		BumpAllocator& Allocator;
		Edges          RemainingSafeArea{}; ///< The safe area (Scene::SetSafeArea) not yet cleared by an ancestor of the node being measured.

        /** @brief Receives the nodes with LayoutNode::NotifyGeometry that arrange changed. A node arranged twice is listed twice. */
        Array<GeometryChange>* GeometryChanges{ nullptr };
    };

    /**
     * @brief The safe area a node keeps its content clear of: what reached it in its last measure, on the edges in its
     * LayoutStyle::SafeAreaEdges. For widgets that place things themselves.
     */
    RATUI_NODISCARD inline Edges SafeAreaInsets( const LayoutNode& a_Node )
    {
        const Edges&         safe  = a_Node.Layout.LastSafeArea;
        const ESafeAreaEdges edges = a_Node.Style.SafeAreaEdges;
        return Edges{
            .T = HasFlag( edges, ESafeAreaEdges::Top )    ? safe.T : 0_u,
            .R = HasFlag( edges, ESafeAreaEdges::Right )  ? safe.R : 0_u,
            .B = HasFlag( edges, ESafeAreaEdges::Bottom ) ? safe.B : 0_u,
            .L = HasFlag( edges, ESafeAreaEdges::Left )   ? safe.L : 0_u,
        };
    }

    /** @brief The padding a node lays its content out in: its own, plus the safe area it keeps clear of. */
    RATUI_NODISCARD inline Edges ContentPadding( const LayoutNode& a_Node )
    {
        return a_Node.Style.Padding + SafeAreaInsets( a_Node );
    }

    /**
     * @brief Measures the desired size of a layout node based on its content and constraints,
     *        and recursively measures its children. This is the first pass of the layout process, 
     *        where each node calculates how much space it would like to have.
     * @param a_Node The layout node to measure.
     * @param a_AvailableSize The available size for this node to fit within, as determined by its parent. 
     * @return The desired size of the node after measurement, which may be used by its parent to allocate space during the Arrange phase.
     */
	Vec2<Unit> MeasureLayoutNode( LayoutNode& a_Node, Vec2<Unit> a_AvailableSize, LayoutContext& a_Ctx );

    /**
     * @brief Arranges a layout node within the given allocated rectangle, 
     *        positioning and sizing it according to its layout style and 
     *        the results of the Measure phase. This is the second pass of the layout process, 
     *        where each node is given a specific area to occupy and must position itself and its children within that area.
     * @param a_Node The layout node to arrange.
     * @param a_AllocatedRect The rectangle allocated for this node by its parent, in the parent's space. It becomes the
     *                        node's ParentRect; SceneRects are set afterwards by ComposeLayout.
     * @return true if any widget in this subtree required a content reflow after being
     *         given its final size (e.g. wrapped text whose height changed with final width).
     *         Callers should re-run Measure + Arrange once when this returns true.
     */
    bool       ArrangeLayoutNode( LayoutNode& a_Node, Rect<Unit> a_AllocatedRect, LayoutContext& a_Ctx );

    /**
     * @brief Sets the SceneRect of everything below a_Node from each node's ParentRect and its parent's ContentOffset.
     * Run it on the root after arranging; a root's SceneRect is its ParentRect.
     */
    void       ComposeLayout( LayoutNode& a_Node );

    /** @brief Sets a_Node's ContentOffset and moves everything below it there at once. Layout isn't touched. Returns whether it changed. */
    bool       SetContentOffset( LayoutNode& a_Node, Vec2<Unit> a_Offset );
} // namespace RatUI
