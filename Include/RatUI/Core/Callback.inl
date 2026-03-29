#pragma once
#include "../Core.h"

#ifndef RATUI_OVERRIDE_CALLBACK_IMPL
    #include <functional>

    namespace RatUI
    {
        // TODO: Maybe add support for return type?

        template<typename... Args>
        using CallbackImpl = std::function<void( Args... )>;

        template<typename... Args>
        struct CoreTraits<CallbackImpl<Args...>>
        {
            using Type = CallbackImpl<Args...>;
            using Signature = void( Args... );
            using ReturnType = void;
            using ArgsTuple = std::tuple<std::decay_t<Args>...>;

            static constexpr auto Invoke(const Type& a_Callback, Args&&... a_Args)
            {
                if ( a_Callback )
                    return a_Callback( std::forward<Args>( a_Args )... );
            }
        };
    }

#endif

namespace RatUI
{

    template<typename... Args>
    using Callback = CallbackImpl<Args...>;

    template<typename... Args>
    constexpr auto Invoke(const Callback<Args...>& a_Callback, Args&&... a_Args)
    {
        return CoreTraits<Callback<Args...>>::Invoke(a_Callback, std::forward<Args>(a_Args)...);
    }

} // namespace RatUI