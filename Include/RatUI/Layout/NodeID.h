#pragma once
#include "../Core.h"

namespace RatUI
{
    /**
     * @brief Describes how a NodeID packs a pool index and a version, and what its invalid value is.
     * Specialise it for a custom NodeID (see RatUINodeIDImpl.h). A specialisation must provide:
         *  - `static constexpr u32 VersionMask`  The largest version value; versions wrap back to 0 after it.
     *  - `static constexpr NodeID Invalid`   The id that never refers to a node.
     *  - `static constexpr u32 Index( NodeID )`, `u32 Version( NodeID )`, `NodeID Construct( u32 a_Index, u32 a_Version )`
     */
    template<typename T>
    struct NodeIDTraits;
}

// A RatUINodeIDImpl.h defines RatUI::NodeID and a RatUI::NodeIDTraits specialisation for it.
#if defined(__has_include)
    #if __has_include("RatUINodeIDImpl.h")
        #include "RatUINodeIDImpl.h"
        #define RATUI_NODE_ID_IMPL
    #elif __has_include(<RatUINodeIDImpl.h>)
        #include <RatUINodeIDImpl.h>
        #define RATUI_NODE_ID_IMPL
    #endif
#endif

#ifndef RATUI_NODE_ID_IMPL
namespace RatUI
{
    /**
     * @brief Represents a safe and unique identifier for a layout node. Use NodeIDTraits<NodeID> to read its index and version.
     */
    struct NodeID
    {
        u64 Value{ ~0ull }; ///< The index (low 32 bits) and version (high 32 bits). Default-constructed ids are invalid.

        constexpr bool operator==( const NodeID& a_Other ) const = default;
    };

    template<>
    struct NodeIDTraits<NodeID>
    {
        // 32 bits of version: an id of a destroyed node stays stale however often its slot is reused.
        static constexpr u32         VersionMask   = 0xFFFFFFFFu;
        static constexpr NodeID      Invalid       = NodeID{};

        static constexpr u32         Index( NodeID a_ID )   { return static_cast<u32>( a_ID.Value ); }
        static constexpr u32         Version( NodeID a_ID ) { return static_cast<u32>( a_ID.Value >> 32 ); }
        static constexpr NodeID      Construct( u32 a_Index, u32 a_Version ) { return NodeID{ static_cast<u64>( a_Index ) | ( static_cast<u64>( a_Version ) << 32 ) }; }
    };
}
#endif

namespace RatUI
{
    static constexpr NodeID c_InvalidNodeID = NodeIDTraits<NodeID>::Invalid;

} // namespace RatUI
