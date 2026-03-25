#pragma once
#include "Layout.h"
#include <bitset>
#include <concepts>

namespace RatUI
{
    // TODO: Support pmr allocators for the buckets?
    // TODO: Add documentation

    /**
     * @brief A pool allocator for LayoutNodes, organized into buckets to allow for efficient allocation and deallocation while maintaining pointer stability.
     * Each bucket contains a fixed number of LayoutNodes, and a free list is used to track available nodes for allocation. 
     * When a node is deallocated, its version is incremented to invalidate any existing NodeIDs that reference it, preventing use-after-free errors.
     */
    class LayoutNodePool
    {
    public:
        struct Bucket;

        // TODO: Should be a config option or a template param
        static constexpr u32 c_NodesPerBucket = 256;

        Array<Unique<Bucket>> Buckets{};
        Array<u32>            FreeList{};

        struct Bucket
        {
            FixedArray<LayoutNode, c_NodesPerBucket> Nodes{};
            FixedArray<u8,         c_NodesPerBucket> Versions{};
            std::bitset<           c_NodesPerBucket> Occupancy{};

            RATUI_NODISCARD bool IsOccupied( u32 a_LocalIndex ) const { return Occupancy.test( a_LocalIndex ); }

            RATUI_NODISCARD bool IsValid( NodeID a_ID ) const
            {
                u32 local = a_ID.Index % c_NodesPerBucket;
                return IsOccupied( local ) && Versions[ local ] == a_ID.Version;
            }

            RATUI_NODISCARD NodeID Allocate( u32 a_GlobalIndex )
            {
                u32 local = a_GlobalIndex % c_NodesPerBucket;
                Nodes[ local ] = LayoutNode{}; // reset stale hierarchy/style data
                Versions[ local ]++;
                Occupancy.set( local );
                return NodeID{ a_GlobalIndex, Versions[ local ] };
            }

            RATUI_NODISCARD void Deallocate( u32 a_GlobalIndex )
            {
                u32 local = a_GlobalIndex % c_NodesPerBucket;
                Versions[ local ]++;  // invalidate existing NodeIDs
                Occupancy.reset( local );
            }

            RATUI_NODISCARD LayoutNode*       Get( u32 a_GlobalIndex )       { return &Nodes[ a_GlobalIndex % c_NodesPerBucket ]; }
            RATUI_NODISCARD const LayoutNode* Get( u32 a_GlobalIndex ) const { return &Nodes[ a_GlobalIndex % c_NodesPerBucket ]; }
        };

        RATUI_NODISCARD bool IsValid( NodeID a_ID ) const
        {
            const Bucket* bucket = BucketAt( a_ID.Index / c_NodesPerBucket );
            return bucket && bucket->IsValid( a_ID );
        }

        RATUI_NODISCARD LayoutNode* GetNode( NodeID a_ID )
        {
            Bucket* bucket = BucketAt( a_ID.Index / c_NodesPerBucket );
            if ( !bucket || !bucket->IsValid( a_ID ) ) return nullptr;
            return bucket->Get( a_ID.Index );
        }

        RATUI_NODISCARD const LayoutNode* GetNode( NodeID a_ID ) const
        {
            const Bucket* bucket = BucketAt( a_ID.Index / c_NodesPerBucket );
            if ( !bucket || !bucket->IsValid( a_ID ) ) return nullptr;
            return bucket->Get( a_ID.Index );
        }

        RATUI_NODISCARD NodeID AllocateNode()
        {
            if ( Empty( FreeList ) )
                Grow();

            u32 nodeIndex = Back( FreeList );
            PopBack( FreeList );

            return BucketForNode( nodeIndex )->Allocate( nodeIndex );
        }

        RATUI_NODISCARD void DeallocateNode( NodeID a_ID )
        {
            Bucket* bucket = BucketAt( a_ID.Index / c_NodesPerBucket );
            if ( !bucket || !bucket->IsValid( a_ID ) ) return;
            bucket->Deallocate( a_ID.Index );
            PushBack( FreeList, static_cast<u32>( a_ID.Index ) );
        }

    private:

        Bucket* BucketAt( u32 a_BucketIndex )
        {
            if ( a_BucketIndex >= Size( Buckets ) ) return nullptr;
            return Buckets[ a_BucketIndex ].get();
        }

        const Bucket* BucketAt( u32 a_BucketIndex ) const
        {
            if ( a_BucketIndex >= Size( Buckets ) ) return nullptr;
            return Buckets[ a_BucketIndex ].get();
        }

        Bucket* BucketForNode( u32 a_NodeIndex ) { return BucketAt( a_NodeIndex / c_NodesPerBucket ); }
        const Bucket* BucketForNode( u32 a_NodeIndex ) const { return BucketAt( a_NodeIndex / c_NodesPerBucket ); }

        void Grow()
        {
            u32 newBucketIndex = static_cast<u32>( Size( Buckets ) );
            EmplaceBack( Buckets, MakeUnique<Bucket>() );

            // Push in reverse so lowest index is popped first
            Reserve( FreeList, Size( FreeList ) + c_NodesPerBucket );
            for ( u32 i = c_NodesPerBucket; i-- > 0; )
                PushBack( FreeList, newBucketIndex * c_NodesPerBucket + i );
        }
    };

} // namespace RatUI
