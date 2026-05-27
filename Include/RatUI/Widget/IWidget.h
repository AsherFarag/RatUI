#pragma once
#include "../Core.h"
#include "../Input/InputEvent.h"
#include "../Layout/Layout.h"
#include "../Renderer/DrawList.h"

namespace RatUI
{
    class Scene;

    /**
     * @brief The base class for all UI elements.
     * Widgets are responsible for rendering themselves and handling input events. 
     * They are organized in a tree structure, with each widget having a corresponding layout node.
     */
    class IWidget
    {
    public:
        IWidget() = default;
        IWidget( const IWidget& ) = delete;
        IWidget& operator=( const IWidget& ) = delete;
        IWidget( IWidget&& ) = default;
        IWidget& operator=( IWidget&& ) = default;
        virtual ~IWidget() = default;

        /** @brief Returns a reference to the scene to which this widget belongs. */
              Scene& GetScene()       { RATUI_USER_ASSERT( m_Scene, "Call to GetScene() failed: widget is not associated with a scene." ); return *m_Scene; }
        const Scene& GetScene() const { RATUI_USER_ASSERT( m_Scene, "Call to GetScene() failed: widget is not associated with a scene." ); return *m_Scene; }

        /** @brief Returns the unique identifier for this widget. */
        WidgetID GetID() const { return m_ID; }

        /** @brief Returns the layout identifier for this widget. */
        NodeID GetLayoutID() const { return m_LayoutID; }

        // - Lifecycle

        /** @brief Called immediately after the widget is constructed and associated with a layout node. */
        virtual void OnConstruct() {}

        /** @brief Called immediately before the widget is destroyed and disassociated from its layout node. */
        virtual void OnDestroy() {} ///< Called immediately before the widget is destroyed and disassociated from its layout node.

        /** @brief Called during the layout process, allowing the widget to update its layout properties or perform calculations based on its children. */
        virtual void OnSyncLayout( LayoutNode& a_Node, Vec2<Unit> a_AvailableSize ) {}

        /** @brief Called when the widget should render itself and its children. */
        virtual void OnPaint( DrawList& a_DrawList ) {}

        // - Input Events

        /** @brief Returns whether this widget can receive focus for input. */
        virtual bool IsFocusable() const { return false; }

        /** @brief Called when this widget receives focus for input. */
        virtual void OnFocusReceived() {}

		/** @brief Called when this widget loses focus for input. */
        virtual void OnFocusLost() {}

        /** @brief Called when a pointer (e.g., mouse cursor) enters the widget's bounds. */
        virtual void OnPointerEnter( const PointerEvent& a_Event ) {}

        /** @brief Called each frame while a pointer is inside (or captured by) this widget. */
        virtual void OnPointerMove( const PointerEvent& a_Event ) {}

        /** @brief Called when the pointer scrolls over this widget. */
        virtual void OnPointerScroll( const PointerEvent& a_Event ) {}

        /** @brief Called when a pointer (e.g., mouse cursor) exits the widget's bounds. */
        virtual void OnPointerExit( const PointerEvent& a_Event ) {}

        /** @brief Called when an input button is pressed while this widget is focused. */
        virtual bool OnPressed( const ButtonEvent& a_Event ) { return false; }

        /** @brief Called when an input button is released while this widget is focused. */
        virtual bool OnReleased( const ButtonEvent& a_Event ) { return false; }

    protected:
        friend Scene;
		Scene*   m_Scene{ nullptr }; // TODO: Implement this so Scene doesnt need to pass itself as an argument to every function.
        WidgetID m_ID{};
        NodeID   m_LayoutID{};
    };

} // namespace RatUI