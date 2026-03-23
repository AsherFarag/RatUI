#pragma once
#include "Core.h"
#include "Layout.h"
#include "Renderer/RenderContext.h"

namespace RatUI
{
    class IWidget;

    /**
     * @brief Represents a child widget that has been arranged within the parent widget.
     */
    struct ArrangedChild
    {
        Geometry FinalGeometry;
        IWidget& Widget;
    };

    /**
     * @brief A collection of arranged child widgets.
     */
    class ArrangedChildren
    {
    public:
        using ArrayType = SmallArray<ArrangedChild, c_ChildrenSBOSize>;

        /** @brief Provides access to the internal array of arranged children. */
        ArrayType& GetArray() { return m_Children; }

        /** @brief Provides const access to the internal array of arranged children. */
        const ArrayType& GetArray() const { return m_Children; }

    private:
        ArrayType m_Children;
    };

    /**
     * @brief The IWidget interface defines the core functionality that all UI widgets must implement.
     * 
     * Equivalent to QWidget in Qt or UWidget in Unreal Engine, this interface provides methods for measuring, arranging, and rendering UI elements.
     */
    class IWidget
    {
    public:
        virtual ~IWidget() = default;

        // TODO: Hmm
        virtual Span<const IWidget*> GetChildren() const { return {}; }

    protected:
        friend class LayoutEngine; // TODO: idk

        /**
         * @brief Measures the desired size of the widget based on the given constraints.
         * @param a_Constraints The size constraints to consider when measuring the widget.
         * @return The desired size of the widget as a Vec2f, where x is the width and y is the height.
         */
        virtual Vec2f Measure( const Constraints& a_Constraints ) = 0;

        /**
         * @brief
         * @param a_FinalGeometry The final geometry rectangle within which the widget should be arranged.
         * @param a_ArrangedChildren A collection to which the widget should add its arranged children after arranging itself.
         */
        virtual void Arrange( const Geometry& a_FinalGeometry, ArrangedChildren& a_ArrangedChildren ) = 0;

        /**
         * @brief Renders the widget using the provided render context.
         * @param a_Context The render context containing necessary information for rendering, such as the renderer instance.
         */
        virtual void Render( const RenderContext& a_Context ) {}
    };

} // namespace RatUI