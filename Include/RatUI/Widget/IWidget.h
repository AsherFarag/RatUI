#pragma once
#include "../Core.h"
#include "../Input/InputEvent.h"
#include "../Input/Navigation.h"
#include "../Layout/Layout.h"
#include "../Renderer/DrawList.h"
#include "WidgetMixins.h"

namespace RatUI
{
    class Scene;

    struct FocusEvent 
    {
        // TODO: Add more stuff here
        // TODO: Should this be in a different file?
    };

    struct TextInputEvent
    {
        codepoint Character{}; ///< The Unicode code point of the character that was input.
    }

    /**
     * @brief The base class for all UI elements.
     * Widgets are responsible for rendering themselves and handling input events. 
     * They are organized in a tree structure, with each widget having a corresponding layout node.
     */
    class IWidget : public DefaultWidgetMixins
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

        /** @brief Returns a reference to the layout node associated with this widget. */
              LayoutNode& GetLayout();
        const LayoutNode& GetLayout() const;

        /** @brief Returns the layout identifier for this widget. */
        NodeID GetLayoutID() const { return m_LayoutID; }

        IWidget* GetParentWidget() const
        {
            const LayoutNode& node = GetLayout();
            return node.Parent() ? node.Parent()->Widget.get() : nullptr;
        }

        template<std::derived_from<IWidget> WidgetType>
        WidgetType* GetParentWidgetAs() const { return dynamic_cast<WidgetType*>( GetParentWidget() ); }

        // - Lifecycle

        /** @brief Called immediately after the widget is constructed and associated with a layout node. */
        virtual void OnConstruct() {}

        /** @brief Called immediately before the widget is destroyed and disassociated from its layout node. */
        virtual void OnDestroy() {} ///< Called immediately before the widget is destroyed and disassociated from its layout node.

        /** @brief Called during the layout process, allowing the widget to update its layout properties or perform calculations based on its children. */
        virtual void OnSyncLayout( LayoutNode& a_Node, Vec2<Unit> a_AvailableSize ) {}

        void Paint( DrawList& a_DrawList )
        {
            LayoutNode& node = GetLayout();
            if ( !Visibility::IsRendered( node.Layout.Visibility ) )
                return;

            if ( !CanPaint( node ) )
                return;

            WidgetMixins::PrePaint( a_DrawList, node );
            OnPaint( a_DrawList );
            WidgetMixins::PostPaint( a_DrawList, node );
        }

        // - Capabilities

        /** @brief Returns whether this widget can receive pointer events. */
        virtual bool IsInteractable() const { return false; }

        /** @brief Returns whether this widget can hold keyboard/gamepad focus. */
        virtual bool IsFocusable() const { return false; }

        /** @brief Whether this widget defines a navigation boundary. */
        virtual bool IsNavigationBoundary() const { return false; }

        // - Pointer Events: Only called for widgets that return true from IsInteractable()

        /** @brief Called when a pointer (e.g., mouse cursor) enters the widget's bounds. */
        virtual Reply OnPointerEnter( const PointerEvent& a_Event ) { return Reply::Unhandled(); }

        /** @brief Called each frame while a pointer is inside (or captured by) this widget. */
        virtual Reply OnPointerMove( const PointerEvent& a_Event ) { return Reply::Unhandled(); }

        /** @brief Called when the pointer scrolls over this widget. */
        virtual Reply OnPointerScroll( const PointerEvent& a_Event ) { return Reply::Unhandled(); }

        /** @brief Called when a pointer (e.g., mouse cursor) exits the widget's bounds. */
        virtual Reply OnPointerExit( const PointerEvent& a_Event ) { return Reply::Unhandled(); }

		// - Focus Events: Only called for widgets that return true from IsFocusable()

        /** @brief Called when this widget receives focus for input. */
        virtual void OnFocusReceived( const FocusEvent& a_Event ) {}

        /** @brief Called when this widget loses focus for input. */
        virtual void OnFocusLost( const FocusEvent& a_Event ) {}

        // - Button Events:

        /** @brief Called when an input button is pressed while this widget is focused. */
        virtual Reply OnButtonPressed( const ButtonEvent& a_Event ) { return Reply::Unhandled(); }

        /** @brief Called when an input button is released while this widget is focused. */
        virtual Reply OnButtonReleased( const ButtonEvent& a_Event ) { return Reply::Unhandled(); }

        virtual Reply OnTextInput( const TextInputEvent& a_Event ) { return Reply::Unhandled(); }

		// - Navigation: Only called for widgets that return true from IsNavigationBoundary()

		/** @brief */
        // TODO: Should I make a_Action a struct with more info and for api stability?
		virtual NavReply OnNavigationBoundary( ENavAction a_Action ) { return NavReply::Escape(); }

    protected:
        /** @brief Called when the widget should render itself. */
        virtual void OnPaint( DrawList& a_DrawList )
        {
            PaintChildren( a_DrawList );
        }

        void PaintChildren( DrawList& a_DrawList )
        {
            GetLayout().ForEachChild( [&]( LayoutNode& childNode )
            {
                if ( childNode.Widget )
                    childNode.Widget->Paint( a_DrawList );
            } );
        }

    protected:
        friend Scene;
		Scene*   m_Scene{ nullptr };
        NodeID   m_LayoutID{};
    };

} // namespace RatUI