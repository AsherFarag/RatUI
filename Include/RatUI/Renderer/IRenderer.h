#pragma once
#include "../Core.h"

namespace RatUI
{
    /** 
     * @brief IRenderer is a user-defined interface for the basic drawing operations required by the UI elements.
     */
    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        /** @brief Executes the given draw commands, rendering all the operations contained within them. */
        virtual void Execute( Span<const struct DrawCmd> a_Commands ) = 0;
    };
}