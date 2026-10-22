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
        u32 Value{ ~0u }; ///< The packed index and version. Default-constructed ids are invalid.

        constexpr bool operator==( const NodeID& a_Other ) const = default;
    };

    template<>
    struct NodeIDTraits<NodeID>
    {
        static constexpr u32         c_IndexBits   = 24;
        static constexpr u32         c_IndexMask   = ( 1u << c_IndexBits ) - 1u;
        static constexpr u32         VersionMask   = 0xFF;
        static constexpr NodeID      Invalid       = NodeID{};

        static constexpr u32         Index( NodeID a_ID )   { return a_ID.Value & c_IndexMask; }
        static constexpr u32         Version( NodeID a_ID ) { return a_ID.Value >> c_IndexBits; }
        static constexpr NodeID      Construct( u32 a_Index, u32 a_Version ) { return NodeID{ ( a_Index & c_IndexMask ) | ( ( a_Version & VersionMask ) << c_IndexBits ) }; }
    };
}
#endif

namespace RatUI
{
    static constexpr NodeID c_InvalidNodeID = NodeIDTraits<NodeID>::Invalid;

} // namespace RatUI
