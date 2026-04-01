#pragma once
#include "../Core.h"
#include "../Input/InputEvent.h"
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

        // - Lifecycle

        /** @brief Called immediately after the widget is constructed and associated with a layout node. */
        virtual void OnConstruct( Scene& a_Scene ) {}

        /** @brief Called immediately before the widget is destroyed and disassociated from its layout node. */
        virtual void OnDestroy( Scene& a_Scene ) {} ///< Called immediately before the widget is destroyed and disassociated from its layout node.

        /** @brief Called during the layout process, allowing the widget to update its layout properties or perform calculations based on its children. */
        virtual void OnSyncLayout( Scene& a_Scene, LayoutNode& a_Node, Vec2f a_AvailableSize ) {}

        /** @brief Called when the widget should render itself and its children. */
        virtual void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) {}

        // - Input Events

        /** @brief Returns whether this widget can receive focus for input. */
        virtual bool IsFocusable( Scene& a_Scene ) const { return false; }

        /** @brief Called when this widget receives focus for input. */
        virtual void OnFocusReceived( Scene& a_Scene ) {}

		/** @brief Called when this widget loses focus for input. */
        virtual void OnFocusLost( Scene& a_Scene ) {}

        /** @brief Called when a pointer (e.g., mouse cursor) enters the widget's bounds. */
        virtual void OnPointerEnter( Scene& a_Scene, const PointerEvent& a_Event ) {}

        /** @brief Called when a pointer (e.g., mouse cursor) exits the widget's bounds. */
        virtual void OnPointerExit( Scene& a_Scene, const PointerEvent& a_Event ) {}

        /** @brief Called when an input button is pressed while this widget is focused. */
        virtual bool OnPressed( Scene& a_Scene, const ButtonEvent& a_Event ) { return false; }

        /** @brief Called when an input button is released while this widget is focused. */
        virtual bool OnReleased( Scene& a_Scene, const ButtonEvent& a_Event ) { return false; }

    protected:
        friend Scene;
        WidgetID m_ID{};
        NodeID m_LayoutID{};
    };

} // namespace RatUI