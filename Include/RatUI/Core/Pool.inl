#pragma once
#include "../Core.h"
#include <bitset>

namespace RatUI
{
    // TODO: Support pmr allocators for the buckets?

    /**
     * @brief Represents a safe and unique identifier for a layout node.
     */
    struct PoolID
    {
        union
        {
            struct
            {
                u32 Index   : 24; ///< The index of the node in the layout system's internal storage.
                u32 Version : 8;  ///< A version number that can be used to detect stale references to nodes that have been removed and potentially reused.
            };

            u32 Packed{ ~0u };    ///< The packed representation of the PoolID, combining the index and version into a single 32-bit value for efficient storage and comparison.
        };

        constexpr PoolID() = default;
        constexpr PoolID( u32 a_Index, u8 a_Version ) : Index( a_Index ), Version( a_Version ) {}
        constexpr explicit PoolID( u32 a_Packed ) : Packed( a_Packed ) {}

        constexpr bool operator==( const PoolID& a_Other ) const { return Packed == a_Other.Packed; }
    };

    static constexpr PoolID c_InvalidPoolID{};

    /**
     * @brief A versioned, bucket-based pool allocator providing stable pointers and safe handle validation via PoolID.
     * Grows by allocating new buckets on the heap - existing bucket pointers and item references are never invalidated.
     *
     * @tparam T The type of item to store. Must be default-constructible.
     * @tparam ItemsPerBucket The number of items to store in each bucket.
     */
    template<typename T, u32 ItemsPerBucket = 256>
    class Pool
    {
    public:
        using ElementType = T;
        static constexpr u32 c_ItemsPerBucket = ItemsPerBucket;

        /**
         * @brief A bucket containing a fixed number of items, along with versioning and occupancy tracking for safe allocation and deallocation.
         */
        struct Bucket
        {
            alignas(T) FixedArray<u8, c_ItemsPerBucket * sizeof(T)> RawData{};
            FixedArray<u8, c_ItemsPerBucket> Versions{};
            std::bitset<   c_ItemsPerBucket> Occupancy{};

            RATUI_NODISCARD T* GetItems() { return reinterpret_cast<T*>( RawData.data() ); }
            RATUI_NODISCARD const T* GetItems() const { return reinterpret_cast<const T*>( RawData.data() ); }

            RATUI_NODISCARD bool IsOccupied( u32 a_LocalIndex ) const
            {
                return Occupancy.test( a_LocalIndex );
            }

            RATUI_NODISCARD bool IsValid( PoolID a_ID ) const
            {
                u32 local = a_ID.Index % c_ItemsPerBucket;
                return IsOccupied( local ) && Versions[ local ] == a_ID.Version;
            }

            template<typename... Args>
            RATUI_NODISCARD PoolID Allocate( u32 a_GlobalIndex, Args&&... a_Args )
            {
                u32 local = a_GlobalIndex % c_ItemsPerBucket;
                std::construct_at( GetItems() + local, std::forward<Args>( a_Args )... );
                Versions[ local ]++;
                Occupancy.set( local );
                return PoolID{ a_GlobalIndex, Versions[ local ] };
            }

            void Deallocate( u32 a_GlobalIndex )
            {
                u32 local = a_GlobalIndex % c_ItemsPerBucket;
                std::destroy_at( GetItems() + local );
                Versions[ local ]++; // invalidate existing PoolIDs
                Occupancy.reset( local );
            }


            RATUI_NODISCARD T*       Get( u32 a_GlobalIndex )       { return GetItems() + ( a_GlobalIndex % c_ItemsPerBucket ); }
            RATUI_NODISCARD const T* Get( u32 a_GlobalIndex ) const { return GetItems() + ( a_GlobalIndex % c_ItemsPerBucket ); }

            Bucket() = default;
            ~Bucket()
            {
                // Destroy any occupied items in the bucket
                for ( u32 i = 0; i < c_ItemsPerBucket; ++i )
                {
                    if ( Occupancy.test( i ) )
                        std::destroy_at( GetItems() + i );
                }
            }
        };

        Array<Unique<Bucket>> Buckets{};
        Array<u32>            FreeList{};

        /** @brief Checks if the given PoolID is valid (i.e., refers to an allocated item in the pool). */
        RATUI_NODISCARD bool IsValid( PoolID a_ID ) const
        {
            const Bucket* bucket = BucketAt( a_ID.Index / c_ItemsPerBucket );
            return bucket && bucket->IsValid( a_ID );
        }

        /** @brief Retrieves a pointer to the item associated with the given PoolID, or nullptr if the ID is invalid. */
        RATUI_NODISCARD T* Get( PoolID a_ID )
        {
            Bucket* bucket = BucketAt( a_ID.Index / c_ItemsPerBucket );
            if ( !bucket || !bucket->IsValid( a_ID ) ) return nullptr;
            return bucket->Get( a_ID.Index );
        }

        /** @brief Retrieves a const pointer to the item associated with the given PoolID, or nullptr if the ID is invalid. */
        RATUI_NODISCARD const T* Get( PoolID a_ID ) const
        {
            const Bucket* bucket = BucketAt( a_ID.Index / c_ItemsPerBucket );
            if ( !bucket || !bucket->IsValid( a_ID ) ) return nullptr;
            return bucket->Get( a_ID.Index );
        }

        /** @brief Allocates a new item in the pool with the given arguments, returning a PoolID that can be used to access it. */
        template<typename... Args>
        RATUI_NODISCARD PoolID Allocate( Args&&... a_Args )
        {
            if ( Empty( FreeList ) )
                Grow();

            u32 index = Back( FreeList );
            PopBack( FreeList );
            return BucketForItem( index )->Allocate( index, std::forward<Args>( a_Args )... );
        }

        /** @brief Deallocates the item associated with the given PoolID, making its slot available for future allocations. */
        void Deallocate( PoolID a_ID )
        {
            Bucket* bucket = BucketAt( a_ID.Index / c_ItemsPerBucket );
            if ( !bucket || !bucket->IsValid( a_ID ) ) return;
            bucket->Deallocate( a_ID.Index );
			PushBack( FreeList, static_cast<u32>( a_ID.Index ) );
        }

        /** @brief Clears all items from the pool and resets it to an empty state. */
        void Clear()
        {
            Buckets.clear();
            FreeList.clear();
        }

        template<std::invocable<T&> Func>
        void ForEach( Func&& a_Func )
        {
            for ( const auto& bucket : Buckets )
            {
                if ( bucket == nullptr )
                    continue;

                for ( u32 i = 0; i < c_ItemsPerBucket; ++i )
                {
                    if ( bucket->Occupancy.test( i ) )
                        std::forward<Func>( a_Func )( bucket->GetItems()[ i ] );
                }
            }
        }

        template<std::invocable<const T&> Func>
        void ForEach( Func&& a_Func ) const
        {
            for ( const auto& bucket : Buckets )
            {
                if ( bucket == nullptr )
                    continue;

                for ( u32 i = 0; i < c_ItemsPerBucket; ++i )
                {
                    if ( bucket->Occupancy.test( i ) )
                        std::forward<Func>( a_Func )( bucket->GetItems()[ i ] );
                }
            }
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

        Bucket*       BucketForItem( u32 a_ItemIndex )       { return BucketAt( a_ItemIndex / c_ItemsPerBucket ); }
        const Bucket* BucketForItem( u32 a_ItemIndex ) const { return BucketAt( a_ItemIndex / c_ItemsPerBucket ); }

        void Grow()
        {
            u32 newBucketIndex = static_cast<u32>( Size( Buckets ) );
            EmplaceBack( Buckets, MakeUnique<Bucket>() );

            // Push in reverse so lowest index is popped first
            Reserve( FreeList, Size( FreeList ) + c_ItemsPerBucket );
            for ( u32 i = c_ItemsPerBucket; i-- > 0; )
                PushBack( FreeList, newBucketIndex * c_ItemsPerBucket + i );
        }
    };

} // namespace RatUI
