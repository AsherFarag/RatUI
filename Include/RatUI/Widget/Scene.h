#pragma once
#include "../Core.h"
#include "../Layout/LayoutEngine.h"
#include "../Input/InputEvent.h"
#include "IWidget.h"

namespace RatUI
{
    using LayoutNodePool = Pool<LayoutNode>;
    using WidgetPool = Pool<Unique<IWidget>>;

    /**
     * @brief Represents a UI scene containing a hierarchy of widgets and their associated layout nodes.
     * The Scene class manages the lifecycle of widgets, processes input events, updates layout, and handles rendering.
     * 
     * @example
     * // Example usage of the Scene class:
     * Scene scene;
     * 
     * // In your initialization code:
     * WidgetID root = scene.CreateRootWidget<ContainerWidget>();
     * WidgetID child1 = scene.CreateWidget<ButtonWidget>( root );
     * WidgetID child2 = scene.CreateWidget<TextWidget>( root );
     * 
     * // In your main loop:
     * scene.ProcessInput( GetMousePosition(), IsMouseDown(), GetUIScale() );
     * scene.UpdateLayout( Vec2f{ 800.0f, 600.0f } );
     * scene.Render( drawList );
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

        LayoutNodePool Layouts{}; ///< Pool of layout nodes representing the hierarchical structure and layout information of widgets in the scene.
        WidgetPool Widgets{};     ///< Pool of widgets in the scene, each associated with a layout node via the WidgetID and LayoutID.
        WidgetID RootWidget{};    ///< The WidgetID of the root widget in the scene, which serves as the entry point for layout and rendering.

        // - Scene Management

        bool DispatchInputEvent( const InputEvent& a_Event );

        /**
         * @brief Updates the layout of all widgets in the scene based on the given available size. 
         * This involves measuring and arranging each widget according to its layout properties and the layout algorithm.
         * @param a_AvailableSize The total available size for the scene, which is typically the size of the window or rendering area.
         */
        void UpdateLayout( Vec2f a_AvailableSize );

        /**
         * @brief Renders the scene by invoking the OnPaint method of the root widget, which recursively renders all child widgets.
         * @param a_DrawList The draw list to which rendering commands should be added.
         */
		void Render( DrawList& a_DrawList );

        // - Focus Management

        /** @brief Gets the currently focused widget by returning the top WidgetID from the focus stack, or an invalid ID if the stack is empty. */
        WidgetID GetFocusedWidget() const { return Empty( m_FocusStack ) ? c_InvalidPoolID : Back( m_FocusStack ); }

        /** @brief Checks if the specified widget is currently focused by comparing its WidgetID to the top of the focus stack. */
        bool IsWidgetFocused( WidgetID a_WidgetID ) const { return a_WidgetID != c_InvalidPoolID && a_WidgetID == GetFocusedWidget(); }

        /**
         * @brief Pushes a widget onto the focus stack, making it the currently focused widget.
         * @param a_WidgetID The ID of the widget to focus.
         */
        void PushFocus( WidgetID a_WidgetID );

        /** @brief Removes the currently focused widget from the focus stack. */
        void PopFocus();

        /** @brief Clears the focus from the current focused widget. */
        void ClearFocus();

        /**
         * @brief Clears the current focus stack and sets the focus to the specified widget.
         * @param a_WidgetID The ID of the widget to set focus to. This widget will become the only focused widget after this call.
         */
        void SetFocus( WidgetID a_WidgetID )
        {
            ClearFocus();
            PushFocus( a_WidgetID );
        }

        // - Widget Management

        template<std::derived_from<IWidget> WidgetType, typename... Args>
        WidgetID CreateWidget( WidgetID a_ParentID, Args&&... a_Args );

        template<std::derived_from<IWidget> WidgetType, typename... Args>
        WidgetID CreateRootWidget( Args&&... a_Args );

        /** @brief Destroys the widget with the specified ID, including its children. */
        bool DestroyWidget( WidgetID a_WidgetID );

        RATUI_NODISCARD IWidget* GetWidget( WidgetID a_ID );

        RATUI_NODISCARD const IWidget* GetWidget( WidgetID a_ID ) const;

        template<std::derived_from<IWidget> WidgetType>
        RATUI_NODISCARD WidgetType* GetWidget( WidgetID a_ID ) { return dynamic_cast<WidgetType*>( GetWidget( a_ID ) ); }

		template<std::derived_from<IWidget> WidgetType>
        RATUI_NODISCARD const WidgetType* GetWidget( WidgetID a_ID ) const { return dynamic_cast<const WidgetType*>( GetWidget( a_ID ) ); }

        template<std::invocable<IWidget&> Func>
        void ForEachChildWidget( WidgetID a_WidgetID, Func&& a_Func );

		template<std::invocable<const IWidget&> Func>
        void ForEachChildWidget( WidgetID a_WidgetID, Func&& a_Func ) const;

        /** @brief Clears the scene */
        void Reset();

    protected:
        bool ProcessPointerEvent( const PointerEvent& a_Event );
        bool ProcessButtonEvent( const ButtonEvent& a_Event );
        WidgetID HitTest( WidgetID a_ID, Vec2f a_LogicalPos );

        Array<WidgetID> m_FocusStack{}; ///< Stack of WidgetIDs representing the current focus hierarchy, with the top of the stack being the currently focused widget.

        WidgetID m_HoveredWidget{ c_InvalidPoolID };
        bool     m_MouseWasDown{ false };
    };

    // === Inline Implementations ===

    inline bool Scene::DispatchInputEvent( const InputEvent& a_Event )
    {
        if ( Holds<PointerEvent>( a_Event.Payload ) )
            return ProcessPointerEvent( Get<PointerEvent>( a_Event.Payload ) );

        if ( Holds<ButtonEvent>( a_Event.Payload ) )
            return ProcessButtonEvent( Get<ButtonEvent>( a_Event.Payload ) );

        return false; // Event type not handled
    }

    inline void Scene::UpdateLayout( Vec2f a_AvailableSize )
    {
        if ( IWidget* root = GetWidget( RootWidget ) )
        {
            if ( LayoutNode* rootNode = Layouts.Get( root->GetLayoutID() ) )
            {
                MeasureLayoutNode( *rootNode, a_AvailableSize );
                ArrangeLayoutNode( *rootNode, Rectf{ Vec2f{ 0.f, 0.f }, a_AvailableSize } );
            }
        }
    }

    inline void Scene::Render( DrawList& a_DrawList )
    {
        if ( IWidget* root = GetWidget( RootWidget ) )
            root->OnPaint( *this, a_DrawList );
    }

    inline void Scene::PushFocus( WidgetID a_WidgetID )
    {
        IWidget* widget = GetWidget( a_WidgetID );
        if ( !widget || !widget->IsFocusable( *this ) )
            return; // Only focus if widget exists and is focusable

        // Notify previous focused widget that it lost focus (if any)
        if ( !Empty( m_FocusStack ) )
        {
            WidgetID currentFocus = Back( m_FocusStack );
            if ( IWidget* currentWidget = GetWidget( currentFocus ) )
                currentWidget->OnFocusLost( *this );
        }

        // Push new widget onto focus stack and notify it that it received focus
        PushBack( m_FocusStack, a_WidgetID );
        widget->OnFocusReceived( *this );
    }

    inline void Scene::PopFocus()
    {
        if ( Empty( m_FocusStack ) )
            return; // No widget to pop

        // Notify current focused widget that it lost focus
        WidgetID currentFocus = Back( m_FocusStack );
        if ( IWidget* currentWidget = GetWidget( currentFocus ) )
            currentWidget->OnFocusLost( *this );

        // Pop the focus stack
        PopBack( m_FocusStack );

        // Notify new focused widget that it received focus (if any)
        if ( !Empty( m_FocusStack ) )
        {
            WidgetID newFocus = Back( m_FocusStack );
            if ( IWidget* newWidget = GetWidget( newFocus ) )
                newWidget->OnFocusReceived( *this );
        }
    }

    inline void Scene::ClearFocus()
    {
        if ( !Empty( m_FocusStack ) )
        {
            WidgetID currentFocus = Back( m_FocusStack );
            if ( IWidget* currentWidget = GetWidget( currentFocus ) )
                currentWidget->OnFocusLost( *this ); // Notify current focused widget that it lost focus

            Clear( m_FocusStack );
        }
    }

    inline IWidget* Scene::GetWidget( WidgetID a_ID )
    {
        if ( Unique<IWidget>* widget = Widgets.Get( a_ID ) )
            return widget->get();

        return nullptr;
    }

    inline const IWidget* Scene::GetWidget( WidgetID a_ID ) const
    {
        if ( const Unique<IWidget>* widget = Widgets.Get( a_ID ) )
            return widget->get();

        return nullptr;
	}

    inline bool Scene::DestroyWidget( WidgetID a_WidgetID )
    {
        IWidget* widget = GetWidget( a_WidgetID );
        if ( !widget )
            return false; // Widget not found

        LayoutNode* node = Layouts.Get( widget->GetLayoutID() );
        if ( !node )
            return false; // Layout node not found

        // Recursively destroy child widgets
        node->ForEachChild( [&]( LayoutNode& childNode )
        {
            DestroyWidget( childNode.WidgetID );
        });

        // Deallocate widget and layout node
        Widgets.Deallocate( widget->GetID() );
        Layouts.Deallocate( widget->GetLayoutID() );

        return true;
    }

    inline void Scene::Reset()
    {
        Layouts.Clear();
        Widgets.Clear();
        RootWidget = c_InvalidPoolID;
        Clear( m_FocusStack );
        m_HoveredWidget = c_InvalidPoolID;
        m_MouseWasDown = false;
    }

    template<std::derived_from<IWidget> WidgetType, typename... Args>
    WidgetID Scene::CreateRootWidget( Args&&... a_Args )
    {
        WidgetID id = CreateWidget<WidgetType>( c_InvalidPoolID, std::forward<Args>( a_Args )... );
        RootWidget = id;
        return id;
    }

    template<std::derived_from<IWidget> WidgetType, typename... Args>
    WidgetID Scene::CreateWidget( WidgetID a_ParentID, Args&&... a_Args )
    {
		Unique<IWidget> widgetPtr = MakeUnique<WidgetType>( std::forward<Args>( a_Args )... );
		IWidget* widget = widgetPtr.get();

        // Allocate layout node and widget
        NodeID     nodeID   = Layouts.Allocate();
		WidgetID   widgetID = Widgets.Allocate( std::move( widgetPtr ) );

        LayoutNode* node   = Layouts.Get( nodeID );

        // Wire widget <-> node
        widget->m_ID       = widgetID;
        widget->m_LayoutID = nodeID;
        node->WidgetID     = widgetID;

		if ( a_ParentID != c_InvalidPoolID )
        {
            if ( IWidget* parentWidget = GetWidget( a_ParentID ) )
            {
                if ( LayoutNode* parentNode = Layouts.Get( parentWidget->GetLayoutID() ) )
                    parentNode->AddChild( *node );
            }
        }

        return widgetID;
    }

    template<std::invocable<IWidget&> Func>
    void Scene::ForEachChildWidget( WidgetID a_WidgetID, Func&& a_Func )
    {
        IWidget* widget = GetWidget( a_WidgetID );
        if ( !widget ) return;
    
        LayoutNode* node = Layouts.Get( widget->GetLayoutID() );
        if ( !node ) return;
    
        node->ForEachChild( [&]( LayoutNode& childNode )
        {
            IWidget* childWidget = GetWidget( childNode.WidgetID );
            if ( childWidget ) a_Func( *childWidget );
        });
    }

    template<std::invocable<const IWidget&> Func>
    void Scene::ForEachChildWidget( WidgetID a_WidgetID, Func&& a_Func ) const
    {
        const IWidget* widget = GetWidget( a_WidgetID );
        if ( !widget ) return;
    
        const LayoutNode* node = Layouts.Get( widget->GetLayoutID() );
        if ( !node ) return;
    
        node->ForEachChild( [&]( const LayoutNode& childNode )
        {
            const IWidget* childWidget = GetWidget( childNode.WidgetID );
            if ( childWidget ) a_Func( *childWidget );
        });
    }

    inline WidgetID Scene::HitTest( WidgetID a_ID, Vec2f a_LogicalPos )
    {
        IWidget* widget = GetWidget( a_ID );
        if ( !widget ) return c_InvalidPoolID;

        LayoutNode* node = Layouts.Get( widget->GetLayoutID() );
        if ( !node || !node->Layout.Visibility.IsHitTestable() ) return c_InvalidPoolID;

        if ( !node->Layout.FinalRect.Contains( a_LogicalPos ) ) return c_InvalidPoolID;

        // Check children first (front-to-back, last child wins)
        WidgetID result = a_ID; // self is the fallback
        node->ForEachChild( [&]( LayoutNode& child )
        {
            WidgetID childHit = HitTest( child.WidgetID, a_LogicalPos );
            if ( childHit != c_InvalidPoolID )
                result = childHit; // deepest child takes priority
        });

        return result;
    }

    inline bool Scene::ProcessPointerEvent( const PointerEvent& a_Event )
    {
        return false; // Event not handled
    }

    inline bool Scene::ProcessButtonEvent( const ButtonEvent& a_Event )
    {
        WidgetID focused = GetFocusedWidget();

        if ( focused != c_InvalidPoolID )
        {
            if ( IWidget* widget = GetWidget( focused ) )
            {
                if ( a_Event.Pressed )
                    widget->OnPressed( *this, a_Event );
                else
                    widget->OnReleased( *this, a_Event );

                return true; // Event handled
            }
        }

        return false; // Event not handled
    }


} // namespace RatUI