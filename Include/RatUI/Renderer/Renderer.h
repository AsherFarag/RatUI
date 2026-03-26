#pragma once
#include "IRenderInterface.h"

namespace RatUI
{
    /** 
     * @brief
     */
    class Renderer
    {
    public:
        /** 
         * @brief Constructs a Renderer with the specified render interface.
         * @param a_RenderInterface The render interface to use for drawing operations.
         */
        Renderer( IRenderInterface& a_RenderInterface )
            : m_RenderInterface( a_RenderInterface )
        {}

        /** @brief Gets access to the underlying render interface for drawing operations. */
        IRenderInterface& GetRenderInterface() { return m_RenderInterface; }

        /** @brief Gets const access to the underlying render interface for drawing operations. */
        const IRenderInterface& GetRenderInterface() const { return m_RenderInterface; }

    protected:
        IRenderInterface& m_RenderInterface;
    };

} // namespace RatUI