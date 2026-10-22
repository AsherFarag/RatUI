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
