#pragma once
#include "Config.h"

#if RATUI_ENABLE_ASSERTS
    #ifndef RATUI_ASSERT
        #include <assert.h>
        #define RATUI_ASSERT( expr, msg ) assert( ( expr ) && msg )
    #endif
#else
    #ifndef RATUI_ASSERT
        #define RATUI_ASSERT( expr, msg )
    #endif
#endif

#if RATUI_ENABLE_USER_ASSERTS
     #ifndef RATUI_USER_ASSERT
        #define RATUI_USER_ASSERT( expr, msg ) RATUI_ASSERT( expr, "User Error: " msg )
    #endif
#else
    #ifndef RATUI_USER_ASSERT
        #define RATUI_USER_ASSERT( expr, msg )
    #endif
#endif

namespace RatUI::Detail
{
    [[noreturn]] inline void Unreachable()
    {
    #if defined(_MSC_VER) && !defined(__clang__)
            __assume(false);
    #else
            __builtin_unreachable();
    #endif
    }
}

#ifndef RATUI_UNREACHABLE
    #define RATUI_UNREACHABLE( msg ) RatUI::Detail::Unreachable()
#endif