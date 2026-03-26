#pragma once

#ifndef RATUI_CALLBACK_IMPL
    #include <functional>
    #define RATUI_CALLBACK_IMPL std::function
#endif

namespace RatUI
{

    template<typename... Args>
    using Callback = RATUI_CALLBACK_IMPL<void( Args... )>;

} // namespace RatUI