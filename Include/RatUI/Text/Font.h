#pragma once
#include "../Core.h"

namespace RatUI
{
    /**
     * @brief An opaque, backend-owned handle to a font resource.
     * The backend is responsible for loading font data, creating font resources, and managing their lifetimes.
     */
    struct FontHandle
    {
        u32 ID{ 0 }; ///< The unique identifier for the font resource, used to reference the font in text rendering operations.

        /** @brief Checks if the font handle is valid (i.e., has a non-zero ID). */
        constexpr bool IsValid() const { return ID != 0; }
        constexpr bool operator==(const FontHandle& other) const { return ID == other.ID; }
    };

} // namespace RatUI

// TODO: Should implement a RatUI::Hash<T> type instead of using std::hash
namespace std
{
    template<>
    struct hash<RatUI::FontHandle>
    {
        size_t operator()( const RatUI::FontHandle& a_Handle ) const noexcept
        {
            return std::hash<RatUI::u32>{}( a_Handle.ID );
        }
    };
}