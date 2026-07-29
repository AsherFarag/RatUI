#pragma once
#include "Layout.h"

namespace RatUI
{
    struct BumpAllocator
    {
		u8* Buffer;
		u32 Capacity;
        u32 Offset;

        explicit BumpAllocator( u8* a_Buffer, u32 a_Capacity )
			: Buffer( a_Buffer ), Capacity( a_Capacity ), Offset( 0 )
		{}

		BumpAllocator( const BumpAllocator& ) = delete;
		BumpAllocator& operator=( const BumpAllocator& ) = delete;

        Span<u8> Allocate( u32 a_Size, u32 a_Alignment = 1 )
        {
			RATUI_ASSERT( a_Alignment > 0 && ( a_Alignment & ( a_Alignment - 1 ) ) == 0, 
                          "Alignment must be a power of two." );

			RATUI_ASSERT( Offset + a_Size <= Capacity, 
                          "BumpAllocator out of memory." );

			u32 alignedOffset = ( Offset + ( a_Alignment - 1 ) ) & ~( a_Alignment - 1 );
			Offset = alignedOffset + a_Size;
			return Span<u8>( Buffer + alignedOffset, a_Size );
        }

		template<typename T> 
            requires ( std::is_trivially_destructible_v<T> )
        Span<T> Allocate( u32 a_Count )
        {
			Span<u8> mem = Allocate( sizeof( T ) * a_Count, alignof( T ) );
            return Span<T>( reinterpret_cast<T*>( Data( mem ) ), a_Count );
        }

		u32 Mark() const 
        { 
            return Offset;
        }

		void Rollback( u32 a_Mark ) 
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

    struct LayoutContext
    {
		BumpAllocator& Allocator;
    };

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
     * @param a_AllocatedRect The rectangle allocated for this node by its parent during the Arrange phase. 
     *                        The node should position itself and its children within this rectangle according to its layout style.
     * @return true if any widget in this subtree required a content reflow after being
     *         given its final size (e.g. wrapped text whose height changed with final width).
     *         Callers should re-run Measure + Arrange once when this returns true.
     */
    bool       ArrangeLayoutNode( LayoutNode& a_Node, Rect<Unit> a_AllocatedRect, LayoutContext& a_Ctx );
} // namespace RatUI
