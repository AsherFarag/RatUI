/**
 * @file TestNodePool.cpp
 * @brief Tests for NodePool handle validation (stale NodeIDs after Deallocate/Clear) and LayoutNode's copy/move restrictions.
 */

#include "TestLayoutCommon.h"
#include <type_traits>

using Traits = NodeIDTraits<NodeID>;

static_assert( !std::is_copy_constructible_v<LayoutNode> );
static_assert( !std::is_move_constructible_v<LayoutNode> );
static_assert( !std::is_copy_assignable_v<LayoutNode> );
static_assert( !std::is_move_assignable_v<LayoutNode> );

TEST_CASE( "NodePool: allocated node knows its own ID", "[nodepool]" )
{
    NodePool pool;
    NodeID id = pool.Allocate();
    LayoutNode* node = pool.Get( id );

    REQUIRE( node != nullptr );
    REQUIRE( node->ID == id );
    REQUIRE( pool.IsValid( id ) );
}

TEST_CASE( "NodePool: Deallocate invalidates the ID, and a reused slot gets a different ID", "[nodepool]" )
{
    NodePool pool;
    NodeID first = pool.Allocate();
    pool.Deallocate( first );

    REQUIRE( pool.Get( first ) == nullptr );

    NodeID second = pool.Allocate();
    REQUIRE( Traits::Index( second ) == Traits::Index( first ) );
    REQUIRE( !( second == first ) );
    REQUIRE( pool.Get( first ) == nullptr );
    REQUIRE( pool.Get( second ) != nullptr );
}

TEST_CASE( "NodePool: Clear invalidates previously issued IDs", "[nodepool]" )
{
    NodePool pool;
    NodeID stale = pool.Allocate();
    pool.Clear();

    REQUIRE( pool.Get( stale ) == nullptr );

    NodeID fresh = pool.Allocate();
    REQUIRE( Traits::Index( fresh ) == Traits::Index( stale ) );
    REQUIRE( pool.Get( stale ) == nullptr );
    REQUIRE( pool.Get( fresh ) != nullptr );
}

TEST_CASE( "NodePool: ForEach visits only live nodes", "[nodepool]" )
{
    NodePool pool;
    NodeID a = pool.Allocate();
    NodeID b = pool.Allocate();
    pool.Deallocate( a );

    int count = 0;
    pool.ForEach( [&]( const LayoutNode& a_Node ) { REQUIRE( a_Node.ID == b ); ++count; } );
    REQUIRE( count == 1 );
}

static_assert( std::is_const_v<decltype( LayoutNode::ID )> );
static_assert( NodeID{} == c_InvalidNodeID );

TEST_CASE( "NodePool: the invalid ID never refers to a node", "[nodepool]" )
{
    NodePool pool;
    REQUIRE( pool.Get( c_InvalidNodeID ) == nullptr );
    REQUIRE_FALSE( pool.IsValid( c_InvalidNodeID ) );

    (void)pool.Allocate();
    REQUIRE( pool.Get( c_InvalidNodeID ) == nullptr );
    pool.Deallocate( c_InvalidNodeID ); // Ignored.
}

TEST_CASE( "NodePool: deallocating a stale ID frees nothing", "[nodepool]" )
{
    NodePool pool;
    const NodeID first = pool.Allocate();
    pool.Deallocate( first );
    pool.Deallocate( first );

    // The slot went back on the free list once, so two allocations get two slots.
    const NodeID a = pool.Allocate();
    const NodeID b = pool.Allocate();
    REQUIRE( Traits::Index( a ) != Traits::Index( b ) );
    REQUIRE( pool.IsValid( a ) );
    REQUIRE( pool.IsValid( b ) );
}

TEST_CASE( "NodePool: growing past a bucket keeps every node where it was", "[nodepool]" )
{
    NodePool           pool;
    const NodeID       first = pool.Allocate();
    const LayoutNode*  where = pool.Get( first );

    NodeID last{};
    for ( int i = 0; i < 4096; ++i )
        last = pool.Allocate();

    REQUIRE( Traits::Index( last ) == 4096 );
    REQUIRE( pool.Get( first ) == where );
    REQUIRE( pool.Get( last ) != nullptr );

    u32 live = 0;
    pool.ForEach( [&]( const LayoutNode& ) { ++live; } );
    REQUIRE( live == 4097 );
}

TEST_CASE( "NodePool: Allocate passes its arguments to the node", "[nodepool]" )
{
    NodePool     pool;
    const NodeID id = pool.Allocate( LayoutStyle{ .Spacing = 7_u, .LayoutType = ELayoutType::Grid } );

    REQUIRE( pool.Get( id )->Style.Spacing == 7_u );
    REQUIRE( pool.Get( id )->Style.LayoutType == ELayoutType::Grid );
}

TEST_CASE( "NodePool: an ID stays stale however often its slot is reused", "[nodepool]" )
{
    NodePool     pool;
    const NodeID first = pool.Allocate();
    pool.Deallocate( first );

    for ( int i = 0; i < 1000; ++i )
    {
        const NodeID reused = pool.Allocate();
        REQUIRE( Traits::Index( reused ) == Traits::Index( first ) );
        REQUIRE_FALSE( pool.IsValid( first ) );
        pool.Deallocate( reused );
    }
}
