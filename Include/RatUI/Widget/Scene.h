#pragma once
#include "../Core.h"
#include "../Layout/LayoutEngine.h"
#include "../Input/InputEvent.h"
#include "../Input/Navigation.h"
#include "../Input/InputState.h"
#include "../Text/ITextMetrics.h"
#include "IWidget.h"

namespace RatUI
{

    /**
     * @brief Represents a UI scene containing a hierarchy of widgets and their associated layout nodes.
     * The Scene class manages the lifecycle of widgets, processes input events, updates layout, and handles rendering.
     *
     * @example
     * Scene scene;
     * scene.Input.NavMap.BindDefaultDesktop();
     *
     * PanelWidget*  root   = scene.CreateRootWidget<PanelWidget>();
     * ButtonWidget* button = scene.CreateWidget<ButtonWidget>( root->GetLayoutID() );
     *
     * // In your main loop:
     * scene.Tick( deltaSeconds );
     * scene.DispatchInputEvent( event );
     * scene.UpdateLayout( Vec2f{ 800.0f, 600.0f } );
     * scene.Render( drawList, deltaSeconds );
     */
    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;
        Scene( const Scene& ) = delete;
        Scene& operator=( const Scene& ) = delete;
        Scene( Scene&& ) = delete; // Widgets point back at their scene.
        Scene& operator=( Scene&& ) = delete;

        NodeID         RootWidget{};   ///< The NodeID of the root widget in the scene, which serves as the entry point for layout and rendering.
        ITextMetrics*  TextMetrics{};  ///< Pointer to a text metrics provider used for measuring text during layout, set by the user.
        InputState     Input{};        ///< The current input state. Prefer Scene's methods (SetFocus, Navigate, etc.) over mutating this directly.
        ThemeHandle    DefaultTheme{}; ///< Widgets (with ThemeMixin) will be set automatically to this theme before OnConstruct() is called, if they don't have a theme set already.

        // - Scene Management

        /**
         * @brief Sends an input event to the widgets. Returns whether the UI used it; an event it returns false for
         * (e.g. Escape with nothing to cancel) is the host's to act on. A pointer event is the UI's while the pointer
         * is over an interactable widget or held by one (see IsPointerOverUI), whatever the widget does with it.
         * A press is tracked by its first button, so with several buttons held a release can be returned true for
         * a button whose press was the game's: act on releases whatever this returns.
         */
        bool DispatchInputEvent( const InputEvent& a_Event );

        /**
         * @brief Updates the layout of all widgets in the scene based on the given available size.
         * Does nothing when no node was marked dirty and the size and safe area are unchanged since the last call.
         * Layout results (ParentRect, SceneRect, ContentSize) are up to date when it returns.
         * @param a_AvailableSize The total available size for the scene, which is typically the size of the window or rendering area.
         */
        void UpdateLayout( Vec2<Unit> a_AvailableSize );

        /** @brief How many times layout has run. Lets a widget tell whether a layout has gone by since it noted the count. */
        RATUI_NODISCARD u64 GetLayoutCount() const { return m_LayoutCount; }

        /** @brief Whether a node changed since the last layout, so the next UpdateLayout will lay the scene out again. */
        RATUI_NODISCARD bool IsLayoutPending() const
        {
            const LayoutNode* root = m_Layouts.Get( RootWidget );
            return root && ( root->Layout.IsDirty || root->Layout.IsDescendantDirty );
        }

        /** @brief Sets how much scratch memory layout may use per UpdateLayout (default 1 MiB). Raise it for very large scenes. */
        void SetLayoutScratchSize( u32 a_Bytes ) { m_LayoutScratchSize = a_Bytes; }

        /**
         * @brief Sets the insets from the scene's edges that content should avoid, such as a phone's notch,
         * rounded corners and home bar, or a TV's overscan. A node opts in per edge with LayoutStyle::SafeAreaEdges and
         * is inset by what its ancestors haven't already cleared (SafeAreaInsets( node )). Applied by the next
         * UpdateLayout, which re-measures only the nodes the change reaches.
         */
        void SetSafeArea( const Edges& a_SafeArea ) { m_SafeArea = a_SafeArea; }
        RATUI_NODISCARD const Edges& GetSafeArea() const { return m_SafeArea; }

        /** @brief Advances the input clock and resolves time-based gestures (currently: long-press). Call once per frame. */
        void Tick( f64 a_DeltaSeconds );

        /**
         * @brief Renders the scene by invoking the OnPaint method of the root widget, which recursively renders all child widgets.
         * @param a_DrawList The draw list to which rendering commands should be added.
         * @param a_DeltaSeconds The time elapsed since the last frame, which can be used for animations or time-based effects during rendering.
         */
        void Render( DrawList& a_DrawList, f32 a_DeltaSeconds );

        // - Focus Management

        /** @brief Returns the NodeID of the currently focused widget, or c_InvalidNodeID if no widget is focused. */
        RATUI_NODISCARD NodeID GetFocusedNode() const { return Input.FocusedWidget; }

        /**
         * @brief Whether a_Node should draw its focus ring: it has focus and input is navigation-driven.
         * A click focuses a button too, but pointer users don't need a ring for it.
         */
        RATUI_NODISCARD bool IsFocusVisible( NodeID a_Node ) const
        {
            return Input.FocusedWidget == a_Node && Input.InputMode == EInputMode::Navigation;
        }

        /** @brief Sets the focus to the specified widget, if it is focusable. Fires OnFocusLost/OnFocusReceived. */
        void SetFocus( NodeID a_Node );

        /** @brief Clears the focus from the current focused widget. */
        void ClearFocus() { SetFocus( c_InvalidNodeID ); }

        /**
         * @brief Asks every widget a_Node is inside, innermost first, to bring it into view (IWidget::OnBringIntoView),
         * e.g. a scroll container scrolling to it. Done for focus that keyboard or gamepad navigation moves.
         * While a layout is pending, where the node is isn't known yet: it is done at the end of the next UpdateLayout.
         */
        void BringIntoView( NodeID a_Node );

        /**
         * @brief Moves a_Node's children by a_Offset from where layout put them (LayoutNode::ContentOffset), e.g. to
         * scroll them. Layout isn't run: only where they are drawn and hit changes.
         */
        void SetContentOffset( NodeID a_Node, Vec2<Unit> a_Offset );

        /** @brief Focuses a_Node and engages it, if it can be engaged (see IWidget::CanEngage). */
        void Engage( NodeID a_Node );

        /** @brief Disengages the engaged widget, if any, keeping it focused. */
        void Disengage();

        /** @brief Whether a_Node is the engaged widget. */
        RATUI_NODISCARD bool IsEngaged( NodeID a_Node ) const
        {
            return a_Node != c_InvalidNodeID && Input.EngagedWidget == a_Node;
        }

        // - Pointer Capture: each pointer (the mouse, each touch, each pen) is captured on its own, so several fingers can
        //   hold different widgets at once.

        /** @brief Sends a_Pointer's events to a_Node until the capture is released, wherever the pointer goes. */
        void CapturePointer( PointerID a_Pointer, NodeID a_Node ) { Input.PointerStates[a_Pointer].CapturedWidget = a_Node; }

        /** @brief Ends a_Pointer's capture, if it has one. */
        void ReleasePointerCapture( PointerID a_Pointer )
        {
            if ( PointerGestureState* gesture = FindValue( Input.PointerStates, a_Pointer ) )
                gesture->CapturedWidget = c_InvalidNodeID;
        }

        /** @brief The widget capturing a_Pointer, or c_InvalidNodeID. A widget destroyed mid-press stays here until the release. */
        RATUI_NODISCARD NodeID GetCapturedWidget( PointerID a_Pointer ) const
        {
            const PointerGestureState* gesture = FindValue( Input.PointerStates, a_Pointer );
            return gesture ? gesture->CapturedWidget : c_InvalidNodeID;
        }

        /**
         * @brief Whether a_Position would hit a_Node or something inside it: inside its parents' clips and not under
         * another interactable widget. A captured widget is hovered only while this holds.
         */
        RATUI_NODISCARD bool IsOver( NodeID a_Node, Vec2<Unit> a_Position ) const;

        /**
         * @brief Whether the pointer is the UI's, so the game should leave it alone: it is over an interactable
         * widget (WidgetTraits::Interactable), or a widget has captured it. A press that landed on no widget is the
         * game's until it is released, so a drag begun in the game stays the game's when it crosses the UI. A widget
         * that releases its capture mid-press leaves the pointer to whatever is under it.
         * With several pointers it is true while any is captured; DispatchInputEvent answers for the pointer of
         * its event. Touches don't hover, so a touch host should use what DispatchInputEvent returns.
         */
        RATUI_NODISCARD bool IsPointerOverUI() const;

        /**
         * @brief Tells the scene the mouse or pen is no longer over it, e.g. it left the window or the game took it
         * for mouse look. Nothing is hovered until it moves again.
         */
        void ClearHover();

        /** @brief Whether input is currently pointer-driven or navigation-driven; useful for widgets deciding whether to show a focus ring. */
        RATUI_NODISCARD EInputMode GetInputMode() const { return Input.InputMode; }

        /**
         * @brief Where the focused widget is taking typed text (see IWidget::GetTextInputRect), or empty if it isn't.
         * Platform layers use it to show an on-screen keyboard and place the IME.
         */
        RATUI_NODISCARD Optional<Rect<Unit>> GetTextInputRect() const;

        /** @brief Whether the focused widget is taking typed text. Platform layers can use this to skip their own key shortcuts. */
        RATUI_NODISCARD bool WantsTextInput() const { return HasValue( GetTextInputRect() ); }

        /**
         * @brief Forgets held keys and modifiers without sending releases. Call when the window loses
         * keyboard focus, since the releases of keys held at that moment go to another window.
         * Each widget holding a key is told (IWidget::OnPressCancelled), so nothing stays pressed or gets activated.
         */
        void ReleaseAllKeys();

        /**
         * @brief Ends a_Pointer's press and capture without a release, e.g. when the platform cancels a touch.
         * The widget holding it is told (IWidget::OnPressCancelled); no click or drag end is sent.
         */
        void CancelPointer( PointerID a_Pointer );

        /**
         * @brief CancelPointer for every pointer, e.g. when the window loses the mouse while a button is down.
         * A press whose release never arrives keeps its pointer (no widget hovers during one that began outside
         * the UI) until this, CancelPointer, or the next press of the same button.
         */
        void ReleaseAllPointers();

        // - Navigation

        /** @brief Performs a navigation action. Returns whether it did anything (moved focus, entered or left a scope, activated a widget). */
        bool Navigate( ENavAction a_Action );

        /**
         * @brief Confines navigation to a_ScopeID's widgets (e.g. a dialog) until it is popped, remembering the
         * focused widget so it can be restored then. Cancel pops it, unless its widget handles the Cancel in
         * IWidget::OnNavigate.
         */
        void PushNavScope( NodeID a_ScopeID );

        /** @brief Leaves the current nav scope, restoring the focus it remembered. Returns false if there was none. */
        bool PopNavScope();
        RATUI_NODISCARD NodeID GetCurrentNavScope() const { return Input.GetCurrentNavScope( RootWidget ); }

        /** @brief If no widget is currently focused, focuses the first focusable widget in the scene. Called automatically the first time navigation input arrives. */
        void EnsureInitialFocus();

        // - Widget Management

        /**
         * @brief Creates a layout node as the last child of a_ParentID. If the parent is a widget that keeps its
         * children in an inner node (IWidget::GetContentNodeID, e.g. a scroll container), the node goes there.
         */
        LayoutNode& CreateLayoutNode( LayoutStyle a_Style = {}, NodeID a_ParentID = c_InvalidNodeID );

        RATUI_NODISCARD       LayoutNode* GetLayoutNode( NodeID a_ID )       { return m_Layouts.Get( a_ID ); }
        RATUI_NODISCARD const LayoutNode* GetLayoutNode( NodeID a_ID ) const { return m_Layouts.Get( a_ID ); }

        /**
         * @brief Creates a widget as the last child of a_ParentID. If the parent is a widget that keeps its children in
         * an inner node (IWidget::GetContentNodeID, e.g. a scroll container), the widget goes there.
         * a_ParentID must be a live node, or c_InvalidNodeID for a widget with no parent.
         */
        template<std::derived_from<IWidget> WidgetType, typename... Args>
        WidgetType* CreateWidget( NodeID a_ParentID, Args&&... a_Args );

        template<std::derived_from<IWidget> WidgetType, typename... Args>
        WidgetType* CreateRootWidget( Args&&... a_Args );

        /** @brief Destroys the widget with the specified ID, including its children, at the start of the next UpdateLayout or Render. */
        void DestroyWidget( NodeID a_WidgetID ) { PushBack( m_ToDestroy, a_WidgetID ); }

        RATUI_NODISCARD IWidget* GetWidget( NodeID a_ID );

        RATUI_NODISCARD const IWidget* GetWidget( NodeID a_ID ) const;

        template<std::derived_from<IWidget> WidgetType>
        RATUI_NODISCARD WidgetType* GetWidget( NodeID a_ID ) { return dynamic_cast<WidgetType*>( GetWidget( a_ID ) ); }

        template<std::derived_from<IWidget> WidgetType>
        RATUI_NODISCARD const WidgetType* GetWidget( NodeID a_ID ) const { return dynamic_cast<const WidgetType*>( GetWidget( a_ID ) ); }

        template<std::invocable<IWidget&> Func>
        void ForEachChildWidget( NodeID a_NodeID, Func&& a_Func );

        template<std::invocable<const IWidget&> Func>
        void ForEachChildWidget( NodeID a_NodeID, Func&& a_Func ) const;

        /** @brief Clears the scene */
        void Reset();

    protected:
        friend class IWidget;

        // --- Internal State ---

        NodePool           m_Layouts{};      ///< Pool of layout nodes representing the hierarchical structure and layout information of widgets in the scene.
        Array<NodeID>      m_ToDestroy;
        u64                m_LayoutCount{ 0 };   ///< Layouts run so far.
        Array<byte>        m_LayoutScratch;  ///< Scratch memory for one UpdateLayout.
        u32                m_LayoutScratchSize{ 1024 * 1024 };
		f64                m_ClockSeconds{ 0.0 };
        Edges              m_SafeArea{};
        Vec2<Unit>         m_LaidOutSize{ Unit{ -1.f }, Unit{ -1.f } }; ///< What the last layout ran with, to skip one that would change nothing. Negative: none yet.
        Edges              m_LaidOutSafeArea{};
        NodeID             m_LaidOutRoot{};
        NodeID             m_PendingBringIntoView{}; ///< Asked for while a layout was pending.

        // --- Internal Methods ---

        /** @brief Applies a_Reply's requests; a capture request or release applies to a_Pointer. */
        void ApplyReply( const Reply& a_Reply, PointerID a_Pointer = c_MousePointerID );

        /** @brief A widget's focus policy or group flag changed: drops the focus and group scope it can no longer hold. */
        void OnFocusTraitsChanged( IWidget& a_Widget );

        /** @brief What to do when a move inside the current nav scope finds nothing: asks the group it is inside. */
        NavReply QueryBoundaryReply( ENavAction a_Action );

        /**
         * @brief Makes the nav scope stack match the newly focused widget's position in the tree.
         * Scopes that don't contain the widget are dropped, and its enclosing groups are pushed,
         * so arrow keys move between its siblings after it was focused by a click, Tab or SetFocus().
         */
        void SyncNavScopes( NodeID a_Focused );

        bool ProcessPointerEvent( const PointerEvent& a_Event );

        /**
         * @brief Re-hit-tests the last pointer position after layout moved or removed widgets, so hover follows
         * without a new pointer event. Sends only OnPointerExit/OnPointerEnter: the pointer itself didn't move.
         */
        void RefreshHover();

        /**
         * @brief Sends a wheel event to the widget its gesture is latched to (WheelLatch), or else bubbles it up from the
         * deepest node under the pointer (or its captor) and latches the gesture to the widget that takes it.
         */
        bool DispatchWheel( const PointerEvent& a_Event, NodeID a_Captured );

        /** @brief Moves hover to a_Target, sending OnPointerExit and OnPointerEnter if it changed. */
        void SetHovered( NodeID a_Target, const PointerEvent& a_Event );

        bool ProcessButtonEvent( const ButtonEvent& a_Event );
        bool ProcessTextInputEvent( const TextInputEvent& a_Event );

        /** @brief Handles a press of a button mapped to a_Action. Returns whether navigation used it. */
        bool ProcessNavPress( ENavAction a_Action, const ButtonEvent& a_Event );

        /** @brief Tracks a pointer button's gesture (press, drag, multi-press). a_Hit is the widget under a press. */
        void UpdateGestureState( const ButtonEvent& a_Event, NodeID a_Hit );

        /**
         * @brief The topmost node under a_Position: the topmost interactable widget, or with a_InteractableOnly false
         * any node (e.g. the empty area of a scroll container).
         */
        NodeID HitTest( Vec2<Unit> a_Position, bool a_InteractableOnly = true ) const;

        /**
         * @brief The widget a pointer's events go to: the one capturing it, none while it is down in a press that
         * landed on no widget (the game's), else the one under a_Position. a_Gesture may be null.
         */
        NodeID PointerTarget( const PointerGestureState* a_Gesture, Vec2<Unit> a_Position ) const;

        /** @brief What a pointer at a_Position hovers: a_Target (its PointerTarget), but a capturing widget only while the pointer is over it. */
        NodeID HoverTarget( const PointerGestureState* a_Gesture, NodeID a_Target, Vec2<Unit> a_Position ) const;

        /**
         * @brief The navigation stops inside a_Scope, in layout order. Arrow keys move between stops outside groups:
         * a group is a stop itself and its contents are reached by entering it. With a_IntoGroups (Tab) the stops
         * inside groups are collected instead of the groups, except a_Keep, the group focus is on.
         */
        void CollectNavTargets( const LayoutNode& a_Scope, Array<const LayoutNode*>& a_Out, bool a_IntoGroups = false, NodeID a_Keep = c_InvalidNodeID ) const;

        void DestroyWidgetImmediately( LayoutNode& a_Node );
        void CleanupDestroyedWidgets();
    };

    // === Inline Implementations ===

    template<std::derived_from<IWidget> WidgetType, typename... Args>
    WidgetType* Scene::CreateRootWidget( Args&&... a_Args )
    {
        WidgetType* widget = CreateWidget<WidgetType>( c_InvalidNodeID, std::forward<Args>( a_Args )... );
        RootWidget = widget->GetLayoutID();
        return widget;
    }

    template<std::derived_from<IWidget> WidgetType, typename... Args>
    WidgetType* Scene::CreateWidget( NodeID a_ParentID, Args&&... a_Args )
    {
        // Allocate layout node and widget
        LayoutNode& node = CreateLayoutNode( {}, a_ParentID );
        node.Widget     = MakeUnique<WidgetType>( std::forward<Args>( a_Args )... );

        // Wire up back-references for the widget
        node.Widget->m_Scene    = this;
        node.Widget->m_LayoutID = node.ID;

        if constexpr ( IWidget::HasMixin<ThemeMixin> )
        {
            if ( !node.Widget->Theme )
                node.Widget->Theme = DefaultTheme;
        }

        // Call construct after fully initialized and linked into hierarchy, in case widget logic depends on that
        WidgetType& widget = static_cast<WidgetType&>( *node.Widget );
        widget.OnConstruct();

        return &widget;
    }

    template<std::invocable<IWidget&> Func>
    void Scene::ForEachChildWidget( NodeID a_NodeID, Func&& a_Func )
    {
        LayoutNode* node = m_Layouts.Get( a_NodeID );
        if ( !node ) return;

        node->ForEachChild( [&]( LayoutNode& childNode )
        {
            if ( childNode.Widget ) a_Func( *childNode.Widget );
        } );
    }

    template<std::invocable<const IWidget&> Func>
    void Scene::ForEachChildWidget( NodeID a_NodeID, Func&& a_Func ) const
    {
        const LayoutNode* node = m_Layouts.Get( a_NodeID );
        if ( !node ) return;

        node->ForEachChild( [&]( const LayoutNode& childNode )
        {
            if ( childNode.Widget ) a_Func( *childNode.Widget );
        } );
    }

} // namespace RatUI
