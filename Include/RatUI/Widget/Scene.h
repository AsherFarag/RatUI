#pragma once
#include "../Core.h"
#include "../Layout/LayoutEngine.h"
#include "../Input/InputEvent.h"
#include "../Input/Navigation.h"
#include "../Input/InputState.h"
#include "../Text/ITextMetrics.h"
#include "IWidget.h"

#include <iterator>
#include <ranges>

namespace RatUI
{
    using LayoutNodePool = Pool<LayoutNode>;
    using WidgetPool = Pool<Unique<IWidget>>;

    /**
     * @brief Represents a UI scene containing a hierarchy of widgets and their associated layout nodes.
     * The Scene class manages the lifecycle of widgets, processes input events, updates layout, and handles rendering.
     *
     * @example
     * Scene scene;
     * scene.Input.NavMap.BindDefaultDesktop();
     *
     * NodeID root = scene.CreateRootWidget<ContainerWidget>();
     * NodeID child1 = scene.CreateWidget<ButtonWidget>( root );
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
        Scene( Scene&& ) = default;
        Scene& operator=( Scene&& ) = default;

        NodeID         RootWidget{};   ///< The NodeID of the root widget in the scene, which serves as the entry point for layout and rendering.
        ITextMetrics*  TextMetrics{};  ///< Pointer to a text metrics provider used for measuring text during layout, set by the user.
        InputState     Input{};        ///< The current input state. Prefer Scene's methods (SetFocus, Navigate, etc.) over mutating this directly.
        ThemeHandle    DefaultTheme{}; ///< Widgets (with ThemeMixin) will be set automatically to this theme before OnConstruct() is called, if they don't have a theme set already.

        // - Scene Management

        bool DispatchInputEvent( const InputEvent& a_Event );

        /**
         * @brief Updates the layout of all widgets in the scene based on the given available size.
         * This involves measuring and arranging each widget according to its layout properties and the layout algorithm.
         * @param a_AvailableSize The total available size for the scene, which is typically the size of the window or rendering area.
         */
        void UpdateLayout( Vec2<Unit> a_AvailableSize );

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

        /** @brief Sets the focus to the specified widget, if it is focusable. Fires OnFocusLost/OnFocusReceived. */
        void SetFocus( NodeID a_Node );

        /** @brief Clears the focus from the current focused widget. */
        void ClearFocus() { SetFocus( c_InvalidNodeID ); }

        void CapturePointer( NodeID a_Node ) { Input.CapturedWidget = a_Node; }

        void ReleasePointerCapture() { Input.CapturedWidget = c_InvalidNodeID; }

        RATUI_NODISCARD NodeID GetCapturedWidget() const { return Input.CapturedWidget; }

        /** @brief Whether input is currently pointer-driven or navigation-driven; useful for widgets deciding whether to show a focus ring. */
        RATUI_NODISCARD EInputMode GetInputMode() const { return Input.InputMode; }

        // - Navigation

        void Navigate( ENavAction a_Action );

        /** @brief Pushes a navigation scope, remembering the currently focused widget so it can be restored on pop. */
        void PushNavScope( NodeID a_ScopeID );
        void PopNavScope();
        RATUI_NODISCARD NodeID GetCurrentNavScope() const { return Input.GetCurrentNavScope( RootWidget ); }

        /** @brief If no widget is currently focused, focuses the first focusable widget in the scene. Called automatically the first time navigation input arrives. */
        void EnsureInitialFocus();

        // - Widget Management

        LayoutNode& CreateLayoutNode( LayoutStyle a_Style = {}, NodeID a_ParentID = c_InvalidNodeID );

        RATUI_NODISCARD       LayoutNode* GetLayoutNode( NodeID a_ID )       { return m_Layouts.Get( a_ID ); }
        RATUI_NODISCARD const LayoutNode* GetLayoutNode( NodeID a_ID ) const { return m_Layouts.Get( a_ID ); }

        template<std::derived_from<IWidget> WidgetType, typename... Args>
        WidgetType* CreateWidget( NodeID a_ParentID, WidgetArgs a_WidgetArgs = {}, Args&&... a_Args );

        template<std::derived_from<IWidget> WidgetType, typename... Args>
        WidgetType* CreateRootWidget( Args&&... a_Args );

        /** @brief Destroys the widget with the specified ID, including its children. */
        bool DestroyWidget( NodeID a_WidgetID ) { PushBack( m_ToDestroy, a_WidgetID ); return true; }

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
        // --- Internal State ---

        LayoutNodePool m_Layouts{};      ///< Pool of layout nodes representing the hierarchical structure and layout information of widgets in the scene.
        Array<NodeID>  m_ToDestroy;
		f64            m_ClockSeconds{ 0.0 };

        // --- Internal Methods ---

        void ApplyReply( const Reply& a_Reply );
        NavReply QueryBoundaryReply( ENavAction a_Action, NodeID a_Focused );

        bool ProcessPointerEvent( const PointerEvent& a_Event );
        bool ProcessButtonEvent( const ButtonEvent& a_Event );
        void UpdateGestureState( const ButtonEvent& a_Event );
        NodeID HitTest( NodeID a_ID, Vec2<Unit> a_LogicalPos );

        /** @brief Recursively collects focusable widgets within a_Scope's subtree, not descending into nested navigation boundaries. */
        void CollectFocusableCandidates( LayoutNode& a_Scope, Array<const LayoutNode*>& a_Out );

        void DestroyWidgetImmediately( LayoutNode& a_Node )
        {
            a_Node.DetachFromParent();
            a_Node.ForEachChild( [&]( LayoutNode& child ) {
                DestroyWidgetImmediately( child );
            } );

            if (a_Node.Widget)
                a_Node.Widget->OnDestroy();

            m_Layouts.Deallocate( a_Node.ID );
        }

        void CleanupDestroyedWidgets()
        {
            for (NodeID nodeID : m_ToDestroy)
            {
                if (LayoutNode* node = m_Layouts.Get(nodeID))
                    DestroyWidgetImmediately( *node );
            }

            Clear( m_ToDestroy );
        }
    };

    // === Inline Implementations ===

    template<std::derived_from<IWidget> WidgetType, typename... Args>
    WidgetType* Scene::CreateRootWidget( Args&&... a_Args )
    {
        WidgetType* widget = CreateWidget<WidgetType>( c_InvalidNodeID, WidgetArgs{}, std::forward<Args>( a_Args )... );
        RootWidget = widget->GetLayoutID();
        return widget;
    }

    template<std::derived_from<IWidget> WidgetType, typename... Args>
    WidgetType* Scene::CreateWidget( NodeID a_ParentID, WidgetArgs a_WidgetArgs, Args&&... a_Args )
    {
        // Allocate layout node and widget
        LayoutNode& node = CreateLayoutNode( {}, a_ParentID );
        node.Widget     = MakeUnique<WidgetType>( std::forward<Args>( a_Args )... );

        // Wire up back-references for the widget
        node.Widget->m_Scene    = this;
        node.Widget->m_LayoutID = node.ID;

        WidgetType& widget = static_cast<WidgetType&>( *node.Widget );

        // Apply widget arguments
        std::move( a_WidgetArgs ).ApplyTo( widget );

		// Apply default theme if the widget has a ThemeMixin and no theme was provided
        if constexpr ( IWidget::HasMixin<ThemeMixin> )
        {
            if ( !node.Widget->Theme )
                node.Widget->Theme = DefaultTheme;
        }

        // Call construct after fully initialized and linked into hierarchy, in case widget logic depends on that
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
