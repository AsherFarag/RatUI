#pragma once
#include "../Core.h"

namespace RatUI
{
    
    enum class ENavAction : u8
    {
        None = 0,

        MoveLeft,
        MoveRight,
        MoveUp,
        MoveDown,

        Activate,  ///< Activate the currently focused item, e.g., XBox A button, Enter key, etc.
        Cancel,    ///< Cancel or go back, e.g., XBox B button, Escape key, etc.
    };

} // namespace RatUI
