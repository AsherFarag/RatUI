#pragma once
#include "../Core.h"

#ifndef RATUI_STRING_IMPL
    #include <string>
    #define RATUI_STRING_IMPL std::string
#endif // Default to std::string if no custom string implementation is provided.

#ifndef RATUI_STRING_VIEW_IMPL
    #include <string_view>
    #define RATUI_STRING_VIEW_IMPL std::string_view
#endif // Default to std::string_view if no custom string view implementation is provided.

namespace RatUI
{ 
    /**
     * @brief String is a dynamic string class for storing and manipulating text.
     */
    using String = RATUI_STRING_IMPL;

    /**
     * @brief StringView is a non-owning view over a string, useful for read-only access to string data without copying.
     */
    using StringView = RATUI_STRING_VIEW_IMPL;

} // namespace RatUI


