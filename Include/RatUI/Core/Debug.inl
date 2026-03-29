#pragma once
#include "../Core.h"

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