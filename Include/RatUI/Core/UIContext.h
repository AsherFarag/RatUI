#pragma once
#include "../Core.h"

namespace RatUI
{
    struct UIContext
    {
        f32 GlobalScale{ 1.0f }; ///< The global scale factor applied to all UI elements, this affects both layout and rendering.
    };
};