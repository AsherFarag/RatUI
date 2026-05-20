#pragma once
#include "Types.h"

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

    /**
     * @brief A simple helper class that holds a value and allows clients to subscribe to changes via a callback.
     * @tparam T The type of the value being observed.
     * @tparam _CompareBeforeNotify If true, the Observable will compare the new value with the current value before invoking the callback, and only invoke it if the value has changed. Default is true.
     * @example
     *  Observable<float> health{ 100.f };
     *  
     *  // Subscribe to changes in health. The callback will be invoked immediately with the current value (100) upon subscription.
     *  // (You can pass in false as the second argument to Subscribe if you don't want the callback to be invoked immediately.)
     *  health.Subscribe([](const float& newHealth) { std::cout << "Health changed: " << newHealth << std::endl; });
     * 
     *  health.Set(75.0f); // This will trigger the callback and print "Health changed: 75"
     * 
     * @warning This class is not thread-safe and should only be used from a single thread (e.g. the main UI thread).
     * @warning This class does not handle reentrancy or recursive notifications. 
     *          If the OnChanged callback modifies the Observable's value, it may lead to unexpected behavior or stack overflow.
     */
    template<typename T, bool _CompareBeforeNotify = true>
    class Observable
    {
    public:
        using ValueType = T;
        using CallbackType = Callback<const T&>;

        /**
         * @brief Will compare the new value with the current value before invoking the callback, and only invoke it if the value has changed.
         * If T is not equality comparable, this will be ignored and the callback will always be invoked on Set().
         */
        static constexpr bool CompareBeforeNotify = _CompareBeforeNotify;

        Observable() = default;
        Observable( const Observable& ) = delete;
        Observable& operator=( const Observable& ) = delete;
        Observable( Observable&& ) = default;
        Observable& operator=( Observable&& ) = default;
        
        template<std::convertible_to<T> U>
        Observable& operator=( U&& a_Value )
        {
            Set( std::forward<U>( a_Value ) );
            return *this;
        }

        /** @brief Constructs an Observable with an initial value. */
        template<typename... Args> requires std::constructible_from<T, Args...>
        Observable( Args&&... a_Args )
            : m_Value( std::forward<Args>( a_Args )... )
        {}

        /** 
         * @brief Subscribes a callback to be invoked when the value changes.
         * @param a_Callback The callback to invoke when the value changes. It should be a callable that takes a const reference to T.
         * @param a_InvokeImmediately If true, the callback will be invoked immediately with the current value upon subscription. Default is true.
         * @note This overwrites any previously subscribed callback.
         */
        void Subscribe( CallbackType a_Callback, bool a_InvokeImmediately = true )
        {
            m_OnChanged = std::move( a_Callback );
            if ( a_InvokeImmediately && m_OnChanged )
                m_OnChanged( m_Value );
        }

        /** @brief Retrieves the current value. */
        const T& Get() const noexcept { return m_Value; }

        /**
         * @brief Sets a new value and triggers the OnChanged callback.
         * @tparam U A type that is convertible to T, allowing for flexible assignment. The value will be perfectly forwarded to the internal storage.
         * @param a_Value The new value to set. It can be of any type that is convertible to T, enabling implicit conversions and move semantics.
         */
        template<std::convertible_to<T> U>
        void Set( U&& a_Value )
        {
            if constexpr ( std::equality_comparable<T> && CompareBeforeNotify )
            {
                T newValue = static_cast<T>( std::forward<U>(a_Value) );
                if ( m_Value == newValue )
                    return;
            
                m_Value = std::move( newValue );
            }
            else
            {
                m_Value = std::forward<U>(a_Value);
            }

            if ( m_OnChanged )
                m_OnChanged(m_Value);
        }

        /** @brief Implicit conversion operator to allow Observable<T> to be used as T. */
        operator const T&() const noexcept { return m_Value; }

    private:
        CallbackType m_OnChanged;
        T m_Value;
    };

} // namespace RatUI