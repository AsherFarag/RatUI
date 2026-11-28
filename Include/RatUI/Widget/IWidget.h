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

    struct LayoutContext;

    /**
	 * @brief Represents a drag gesture event, containing information about the origin, current position, delta movement, and any modifier keys pressed.
	 * Like when a user clicks and holds the mouse button, then moves the mouse while holding the button down.
	 * Or when a user touches the screen and moves their finger while maintaining contact.
     */
    struct DragEvent
    {
        Vec2<Unit> Origin{};
        Vec2<Unit> Current{};
        Vec2<Unit> Delta{};       ///< Since last DragMove, not since Origin
        EModifier  Modifiers{};
    };

    /**
     * @brief Represents a press gesture event, containing information about the position, press count, and any modifier keys pressed.
	 * Represents either a mouse click or a finger tap on a touch screen.
     */
    struct PressEvent
    {
        Vec2<Unit> Position{};
        u32        PressCount{ 1 };
        EModifier  Modifiers{};

        RATUI_NODISCARD constexpr bool IsDoublePress() const { return PressCount == 2; }
        RATUI_NODISCARD constexpr bool IsTriplePress() const { return PressCount == 3; }
    };

    /**
     * @brief Represents a long press gesture event, containing information about the position and any modifier keys pressed.
	 * Triggered when a user presses and holds the mouse button or touches the screen for a specified duration.
     */
    struct LongPressEvent
    {
        Vec2<Unit> Position{};
        EModifier  Modifiers{};
    };

    struct NavEvent
    {
		ENavAction Action{ ENavAction::None };
    };

    /** @brief Sent to the focused widget when navigation activates it (Enter, Space, gamepad A...), on press and on release. */
    struct ActivateEvent
    {
        bool Pressed{ true };
    };

    /** @brief A press a widget took that ended without its release. */
    struct PressCancelEvent
    {
        Optional<PointerID> Pointer{}; ///< The pointer whose press was cancelled; empty for a key.
    };

    /** @brief Which input can give a widget focus. */
    enum class EFocusPolicy : u8
    {
        None,    ///< Never focused. Navigation passes through it to the widgets inside.
        Pointer, ///< Focused by a press, but skipped by arrow keys and Tab (e.g. a scrollbar).
        All,     ///< Focused by a press, and a stop for keyboard and gamepad navigation.
    };

    /** @brief A change layout made to a node's size or to how far its children extend. */
    struct GeometryEvent
    {
        Vec2<Unit> Size{};
        Vec2<Unit> PreviousSize{};
        Vec2<Unit> ContentSize{};         ///< LayoutResult::ContentSize.
        Vec2<Unit> PreviousContentSize{};
    };

    /** @brief How input, navigation and layout treat a widget. Each widget type passes its defaults to IWidget. */
    struct WidgetTraits
    {
        EFocusPolicy Focus{ EFocusPolicy::None };
        bool Interactable{ false };          ///< Receives pointer events, and keeps them from what is behind it and from the game (Scene::IsPointerOverUI).
        bool NavGroup{ false };              ///< A navigation stop that Activate enters and Cancel leaves. Arrows reach what is inside only once entered; Tab walks straight in.
        bool Engageable{ false };            ///< Can be engaged (see IWidget::CanEngage).
        bool WidthDependentContent{ false }; ///< OnMeasureContent's height depends on the width given (e.g. wrapped text), so it is measured again at the final width.
    };

    /**
     * @brief The base class for all UI elements.
     * Widgets are responsible for rendering themselves and handling input events. 
     * They are organized in a tree structure, with each widget having a corresponding layout node.
     */
    class IWidget : public DefaultWidgetMixins
    {
    public:
        IWidget() = default;
        explicit IWidget( const WidgetTraits& a_Traits ) : m_Traits( a_Traits ) {}
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

        /**
         * @brief The node this widget's children are parented to: its own, unless it keeps them in an inner node
         * (e.g. a scroll container's scrolled content). Scene::CreateWidget parents to it.
         */
        NodeID GetContentNodeID() const { return m_ContentID == c_InvalidNodeID ? m_LayoutID : m_ContentID; }

        /** @brief The nearest widget this one is inside, past any bare layout nodes (e.g. a scroll container's content node). */
        IWidget* GetParentWidget() const
        {
            for ( const LayoutNode* node = GetLayout().Parent(); node; node = node->Parent() )
            {
                if ( node->Widget )
                    return node->Widget.get();
            }
            return nullptr;
        }

        template<std::derived_from<IWidget> WidgetType>
        WidgetType* GetParentWidgetAs() const { return dynamic_cast<WidgetType*>( GetParentWidget() ); }

        // - Lifecycle

        /** @brief Called immediately after the widget is constructed and associated with a layout node. */
        virtual void OnConstruct() {}

        /** @brief Called immediately before the widget is destroyed and disassociated from its layout node. */
        virtual void OnDestroy() {}

        /** */
        virtual Vec2<Unit> OnMeasureContent( const LayoutNode& a_Node, Vec2<Unit> a_AvailableSize, const LayoutContext& a_Ctx ) { return Vec2<Unit>{ 0_u, 0_u }; }

        /**
         * @brief Called when layout changed the size or content size of a node this widget watches: one it set
         * LayoutNode::NotifyGeometry on, whether its own (GetLayout().NotifyGeometry = true in OnConstruct) or an inner
         * one. It runs once layout is done and before anything is painted, so the scene may be changed here; a change
         * that needs layout is laid out before UpdateLayout returns.
         */
        virtual void OnGeometryChanged( const LayoutNode& a_Node, const GeometryEvent& a_Event ) {}

        /**
         * @brief Called on every widget a_Target is inside, innermost first, when it should be brought into view
         * (Scene::BringIntoView, e.g. for keyboard or gamepad focus). A widget that can show more than fits, such as
         * a scroll container, moves a_Target into its visible part.
         */
        virtual void OnBringIntoView( const LayoutNode& a_Target ) {}

        void Paint( const PaintEvent& a_Event )
        {
            LayoutNode& node = GetLayout();
            if ( !Visibility::IsRendered( node.Layout.Visibility ) )
                return;

            if ( !CanPaint( node ) )
                return;

            WidgetMixins::PrePaint( a_Event, node );
            OnPaint( a_Event );
            WidgetMixins::PostPaint( a_Event, node );
        }

        // - Capabilities

        const WidgetTraits& GetTraits() const { return m_Traits; }

        /** @brief Whether this widget receives pointer events. */
        bool IsInteractable() const { return m_Traits.Interactable; }
        void SetInteractable( bool a_Interactable ) { m_Traits.Interactable = a_Interactable; }

        EFocusPolicy GetFocusPolicy() const { return m_Traits.Focus; }

        /** @brief Changes which input can focus this widget. Made unfocusable while focused, it loses focus. */
        void SetFocusPolicy( EFocusPolicy a_Policy );

        /** @brief Whether this widget can hold focus at all. */
        bool IsFocusable() const { return m_Traits.Focus != EFocusPolicy::None; }

        /** @brief Whether keyboard and gamepad navigation stops on this widget. */
        bool IsNavStop() const { return m_Traits.Focus == EFocusPolicy::All; }

        /** @brief Whether this widget is a navigation group (see WidgetTraits::NavGroup). */
        bool IsNavGroup() const { return m_Traits.NavGroup && IsNavStop(); }

        /**
         * @brief Makes this widget a navigation group, or an ordinary widget again. A group is a navigation stop
         * (EFocusPolicy::All); when it stops being one it gets back the focus policy it had before.
         */
        void SetNavGroup( bool a_NavGroup );

        /**
         * @brief Whether this widget can be engaged: put in an edit mode where it takes the keys navigation would
         * otherwise use (e.g. a slider taking the arrows, a text field taking typed text).
         *
         * Navigation stops on the widget without engaging it. Activate (Enter, A) engages it; Activate again, Cancel
         * (Escape, B) or Tab disengage it, as does focus moving away. While engaged, arrow keys the widget leaves
         * unhandled don't move focus. A widget can also engage itself, e.g. on a click, with Reply::Engage.
         */
        bool CanEngage() const { return m_Traits.Engageable; }

        /** @brief Whether this widget is engaged (see CanEngage). */
        bool IsEngaged() const;

        // - Pointer Events: Only called for widgets that return true from IsInteractable()

        /** @brief Called when a pointer (e.g., mouse cursor) enters the widget's bounds. */
        virtual Reply OnPointerEnter( const PointerEvent& ) { return Reply::Unhandled(); }

        /** @brief Called each frame while a pointer is inside (or captured by) this widget. */
        virtual Reply OnPointerMove( const PointerEvent& ) { return Reply::Unhandled(); }

        /** @brief Called when the pointer scrolls over this widget. */
        virtual Reply OnPointerScroll( const PointerEvent& ) { return Reply::Unhandled(); }

        /** @brief Called when a pointer (e.g., mouse cursor) exits the widget's bounds. */
        virtual Reply OnPointerExit( const PointerEvent& ) { return Reply::Unhandled(); }

		// - Gesture Events: Only called for widgets that return true from IsInteractable()

		/** @brief Called when a pointer (e.g., mouse button or touch) is pressed down and released within the widget's bounds. */
		virtual Reply OnPress( const PressEvent& ) { return Reply::Unhandled(); }

		/** @brief Called when a pointer (e.g., mouse button or touch) is pressed and held within the widget's bounds for a certain duration. */
        virtual Reply OnLongPress( const LongPressEvent& ) { return Reply::Unhandled(); }

		/** @brief Called when a pointer (e.g., mouse button or touch) is pressed and moved within the widget's bounds. */
        virtual Reply OnDragStart( const DragEvent& ) { return Reply::Unhandled(); }

		/** @brief Called when a pointer (e.g., mouse button or touch) is moved while the widget is being dragged. */
        virtual Reply OnDragMove( const DragEvent& ) { return Reply::Unhandled(); }

		/** @brief Called when a pointer (e.g., mouse button or touch) is released after a drag operation. */
        virtual Reply OnDragEnd( const DragEvent& ) { return Reply::Unhandled(); }

		// - Focus Events: Only called for widgets that return true from IsFocusable()

        /** @brief Called when this widget receives focus for input. */
        virtual void OnFocusReceived( const FocusEvent& a_Event ) {}

        /** @brief Called when this widget loses focus for input. */
        virtual void OnFocusLost( const FocusEvent& a_Event ) {}

		// - Button Events: For pointer buttons, dispatched to the captured widget (if any) or the hit-tested widget; for non-pointer buttons, dispatched to the focused widget.
        /** @brief Called when an input button is pressed while this widget is focused. */
        virtual Reply OnButtonPressed( const ButtonEvent& a_Event ) { return Reply::Unhandled(); }

        /** @brief Called when an input button is released while this widget is focused. */
        virtual Reply OnButtonReleased( const ButtonEvent& a_Event ) { return Reply::Unhandled(); }

        /** @brief Called with typed text (UTF-8) while this widget is focused. */
        virtual Reply OnTextInput( const TextInputEvent& a_Event ) { return Reply::Unhandled(); }

        /**
         * @brief Called when navigation activates this widget while it is focused, and again on release if it handled
         * the press. Not called for groups (Activate enters them) or engageable widgets (Activate engages them).
         */
        virtual Reply OnActivate( const ActivateEvent& a_Event ) { return Reply::Unhandled(); }

        /**
         * @brief Called when a press this widget took (a key, an activation or a pointer it holds) ended without its
         * release: Scene::ReleaseAllKeys, Scene::CancelPointer. Drop the pressed or dragging state and act on nothing.
         */
        virtual void OnPressCancelled( const PressCancelEvent& a_Event ) {}

        /** @brief Called when the widget becomes engaged (see CanEngage). */
        virtual void OnEngage() {}

        /** @brief Called when the widget stops being engaged, before OnFocusLost if focus is moving. */
        virtual void OnDisengage() {}

        /**
         * @brief Where this widget is taking typed text (its caret), if the platform should be asked for text input
         * while it is focused (e.g. to show an on-screen keyboard or place an IME), as for an engaged text field.
         */
        virtual Optional<Rect<Unit>> GetTextInputRect() const { return NullOpt; }

		// - Navigation

		/**
		 * @brief Called on the engaged widget (see CanEngage) with a navigation action whose button it left unhandled,
		 * before navigation acts on it. Handle a move to use it, e.g. a slider stepping its value; handling Activate
		 * or Cancel keeps the widget engaged.
		 *
		 * Also called with Cancel on the widget of a scope pushed with Scene::PushNavScope, before Cancel pops it.
		 * Handle it to close the dialog yourself (and pop the scope), or to stay open; unhandled, the scope is popped.
		 */
		virtual Reply OnNavigate( const NavEvent& a_Event ) { return Reply::Unhandled(); }

		/**
		 * @brief Called on the widget whose nav scope navigation is in (an entered group, or a widget pushed
		 * with Scene::PushNavScope) when a move finds nothing further that way. By default navigation leaves the scope.
		 */
		virtual NavReply OnNavigationBoundary( const NavEvent& a_Event ) { return NavReply::Escape(); }

    protected:
        /** @brief Called when the widget should render itself. */
        virtual void OnPaint( const PaintEvent& a_Event )
        {
            PaintChildren( a_Event );
        }

        /** @brief Paints the widgets inside this one. */
        void PaintChildren( const PaintEvent& a_Event ) { PaintChildren( a_Event, GetLayout() ); }

        /**
         * @brief Paints the nearest widgets below a_Node, through any bare layout nodes in between. With a_Visible
         * (e.g. a scroll container's clip), children whose rect is wholly outside it are skipped.
         */
        void PaintChildren( const PaintEvent& a_Event, LayoutNode& a_Node, const Rect<Unit>* a_Visible = nullptr );

    protected:
        friend Scene;
		Scene*       m_Scene{ nullptr };
        NodeID       m_LayoutID{};
        NodeID       m_ContentID{}; ///< Set by a widget that keeps its children in an inner node (see GetContentNodeID).
        WidgetTraits m_Traits{};
        EFocusPolicy m_FocusBeforeGroup{ EFocusPolicy::None }; ///< The policy SetNavGroup( false ) restores.
    };

} // namespace RatUI