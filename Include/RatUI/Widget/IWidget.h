#pragma once
#include "Core.h"
#include "Layout.h"
#include "Renderer/RenderContext.h"

namespace RatUI
{
    /**
     * @brief The IWidget interface defines the core functionality that all UI widgets must implement.
     * 
     * Equivalent to QWidget in Qt or UWidget in Unreal Engine, this interface provides methods for measuring, arranging, and rendering UI elements.
     */
    class IWidget
    {
    public:
        virtual ~IWidget() = default;

        /**
         * @brief Measures the desired size of the widget based on the given constraints.
         * @param a_Constraints The size constraints to consider when measuring the widget.
         * @return The desired size of the widget as a Vec2f, where x is the width and y is the height.
         */
        virtual Vec2f Measure( const Constraints& a_Constraints ) = 0;

        /**
         * @brief Arranges the widget within the given final rectangle, determining its final position and size.
         * @param a_FinalRect The final rectangle within which the widget should be arranged, defined by its center and half-extents.
         */
        virtual void Arrange( const Rectf& a_FinalRect ) {}

        /**
         * @brief Renders the widget using the provided render context.
         * @param a_Context The render context containing necessary information for rendering, such as the renderer instance.
         */
        virtual void Render( const RenderContext& a_Context ) {}
    };

} // namespace RatUI
