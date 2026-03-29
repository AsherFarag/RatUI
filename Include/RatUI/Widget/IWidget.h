#pragma once
#include "../Core.h"
#include "../Layout/Layout.h"
#include "../Renderer/DrawList.h"

namespace RatUI
{
    struct Scene;

    /**
     * @brief The base class for all UI elements.
     * Widgets are responsible for rendering themselves and handling input events. 
     * They are organized in a tree structure, with each widget having a corresponding layout node.
     */
    class IWidget
    {
    public:
        virtual ~IWidget() = default;

        /** @brief Returns the unique identifier for this widget. */
        WidgetID GetID() const { return m_ID; }

        /** @brief Returns the layout identifier for this widget. */
        NodeID GetLayoutID() const { return m_LayoutID; }

        /** @brief Called when the widget should render itself and its children. */
        virtual void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) {}

        /** @brief Returns whether this widget can receive focus for input. */
        virtual bool IsFocusable() const { return false; }

        /** @brief Called when this widget receives focus for input. */
        virtual void OnFocusReceived() {}

        /** @brief Called when a pointer (e.g., mouse cursor) enters the widget's bounds. */
        virtual void OnHoverEnter() {}

        /** @brief Called when a pointer (e.g., mouse cursor) exits the widget's bounds. */
        virtual void OnHoverExit() {}

        // TODO: These would require an input button type and not sure how I want to go about that yet
        virtual void OnPressed() {}
        virtual void OnReleased() {}
        //virtual void OnInput() {}

    protected:
        friend Scene;
        WidgetID m_ID{};
        NodeID m_LayoutID{};
    };

} // namespace RatUI