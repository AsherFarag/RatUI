#pragma once
#include "../Core.h"
#include "../Layout/LayoutEngine.h"
#include "../Input/InputEvent.h"
#include "../Input/Navigation.h"
#include "../Text/ITextMetrics.h"
#include "IWidget.h"

#include <iterator>
#include <ranges>

namespace RatUI
{
    // TODO: Idk if this was worth the effort to avoid the array alloc in Navigate,
    // Make this a reusable utility
    namespace Detail
    {
        struct LayoutChildIterator
        {
            using iterator_concept = std::forward_iterator_tag;
            using iterator_category = std::forward_iterator_tag;
            using value_type = LayoutNode*;
            using difference_type = std::ptrdiff_t;

            LayoutNode* Current{ nullptr };

            value_type operator*() const { return Current; }

            LayoutChildIterator& operator++()
            {
                Current = Current ? Current->NextSibling() : nullptr;
                return *this;
            }

            LayoutChildIterator operator++( int )
            {
                LayoutChildIterator copy = *this;
                ++( *this );
                return copy;
            }

            bool operator==( std::default_sentinel_t ) const { return Current == nullptr; }
        };

        struct LayoutChildRange : std::ranges::view_interface<LayoutChildRange>
        {
            LayoutNode* First{ nullptr };

			LayoutChildRange( LayoutNode* a_First ) : First( a_First ) {}
            LayoutChildIterator begin() const { return LayoutChildIterator{ First }; }
            std::default_sentinel_t end() const { return {}; }
        };

    } // namespace Detail

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

        LayoutNodePool Layouts{};     ///< Pool of layout nodes representing the hierarchical structure and layout information of widgets in the scene.
        WidgetPool     Widgets{};     ///< Pool of widgets in the scene, each associated with a layout node via the WidgetID and LayoutID.
        WidgetID       RootWidget{};  ///< The WidgetID of the root widget in the scene, which serves as the entry point for layout and rendering.
        ITextMetrics*  TextMetrics{}; ///< Pointer to a text metrics provider used for measuring text during layout, set by the user.

        // - Scene Management

        bool DispatchInputEvent( const InputEvent& a_Event );

        /**
         * @brief Updates the layout of all widgets in the scene based on the given available size. 
         * This involves measuring and arranging each widget according to its layout properties and the layout algorithm.
         * @param a_AvailableSize The total available size for the scene, which is typically the size of the window or rendering area.
         */
        void UpdateLayout( Vec2<Unit> a_AvailableSize );

        /**
         * @brief Renders the scene by invoking the OnPaint method of the root widget, which recursively renders all child widgets.
         * @param a_DrawList The draw list to which rendering commands should be added.
         */
		void Render( DrawList& a_DrawList );

        // - Focus Management

        /** @brief Returns the WidgetID of the currently focused widget, or c_InvalidPoolID if no widget is focused. */
        WidgetID GetFocusedWidget() const { return m_FocusedWidget; }

        /** @brief Sets the focus to the specified widget, if it is focusable. */
        void SetFocus( WidgetID a_WidgetID );

        /** @brief Clears the focus from the current focused widget. */
        void ClearFocus() { SetFocus( c_InvalidPoolID ); }

        // - Navigation

        void Navigate( ENavAction a_Action );

        void PushNavScope( WidgetID a_ScopeID );
        void PopNavScope();
        WidgetID GetCurrentNavScope() const { return Empty( m_NavStack ) ? RootWidget : Back( m_NavStack ).Scope; }

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
        WidgetID HitTest( WidgetID a_ID, Vec2<Unit> a_LogicalPos );

        WidgetID m_FocusedWidget{ c_InvalidPoolID };
        WidgetID m_HoveredWidget{ c_InvalidPoolID };
        PointerEvent m_LastPointerEvent{}; ///< The last pointer event received, used for hit testing and hover state management.

        struct NavScope
        {
            WidgetID Scope{ c_InvalidPoolID };    ///< The container widget
            WidgetID Restored{ c_InvalidPoolID }; ///< The widget to restore on pop
        };
        Array<NavScope> m_NavStack{}; ///< Stack of navigation scopes used to manage focus during keyboard/gamepad navigation, allowing for nested navigation contexts.
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

    inline void Scene::UpdateLayout( Vec2<Unit> a_AvailableSize )
    {
        IWidget* root = GetWidget( RootWidget );
        if ( !root ) return;
        LayoutNode* rootNode = Layouts.Get( root->GetLayoutID() );
        if ( !rootNode ) return;

        // TODO: This looks horrible but need to research if theres a faster way to handle wrapping text.
        // UI is hell

		// Walks the layout tree, calling OnSyncLayout on each widget to allow them to update their layout properties based on their children and the available size.
		// Returns true if any widget's intrinsic size changed, which indicates a need for a second pass to remeasure and rearrange.
        const auto SyncSubtree = [&]( auto& Self, LayoutNode& node, Vec2<Unit> parentSize ) -> bool
        {
            bool anyChanged = false;

            if ( IWidget* widget = GetWidget( node.WidgetID ) )
            {
                const Vec2<Unit> oldIntrinsics = node.Layout.IntrinsicSize;
                widget->OnSyncLayout( *this, node, parentSize );
                anyChanged |= ( node.Layout.IntrinsicSize != oldIntrinsics );
            }

            const Vec2<Unit> innerSize( node.Layout.FinalRect.Size[0] - node.Style.Padding.Horizontal(),
                                        node.Layout.FinalRect.Size[1] - node.Style.Padding.Vertical() );
            node.ForEachChild( [&]( LayoutNode& child )
            {
                anyChanged |= Self( Self, child, innerSize );
            } );

            return anyChanged;
        };

        // - Pass 1: Sync layout properties and measure

		SyncSubtree( SyncSubtree, *rootNode, a_AvailableSize );
        MeasureLayoutNode( *rootNode, a_AvailableSize );
        ArrangeLayoutNode( *rootNode, Rect<Unit>{ Vec2<Unit>( 0_u, 0_u ), a_AvailableSize } );

        // - Pass 2: If any widget reported a change in intrinsic size during the first pass, 
        // we need to re-run the layout to account for those changes. 
        // This is necessary because changes in intrinsic size can affect the layout of parent and sibling widgets.

		if ( bool needsSecondPass = SyncSubtree( SyncSubtree, *rootNode, a_AvailableSize ) )
        {
            MeasureLayoutNode( *rootNode, a_AvailableSize );
            ArrangeLayoutNode( *rootNode, Rect<Unit>{ Vec2<Unit>( 0_u, 0_u ), a_AvailableSize } );
        }

        // Re-run hit test to update hovered widget based on new layout
        // TODO: This is a bit unclean and potentially incorrect
		ProcessPointerEvent( m_LastPointerEvent ); 
    }

    inline void Scene::Render( DrawList& a_DrawList )
    {
        if ( IWidget* root = GetWidget( RootWidget ) )
            root->OnPaint( *this, a_DrawList );
    }

    inline void Scene::SetFocus( WidgetID a_WidgetID )
    {
        if ( IWidget* currentFocus = GetWidget( m_FocusedWidget ) )
        {
            if ( currentFocus->GetID() == a_WidgetID )
                return; // Already focused

            currentFocus->OnFocusLost( *this );
        }

        m_FocusedWidget = a_WidgetID;
        if ( IWidget* newFocus = GetWidget( m_FocusedWidget ) )
            newFocus->OnFocusReceived( *this );
    }

    inline void Scene::Navigate( ENavAction a_Action )
    {
        const auto FocusFirstIn = [&]( WidgetID a_ScopeID )
        {
            IWidget* scopeWidget = GetWidget( a_ScopeID );
            if ( !scopeWidget ) return;

            LayoutNode* scopeNode = Layouts.Get( scopeWidget->GetLayoutID() );
            if ( !scopeNode ) return;

            // Walk children in layout order, find first focusable
            for ( LayoutNode* child = scopeNode->FirstChild(); child; child = child->NextSibling() )
            {
                IWidget* w = GetWidget( child->WidgetID );
                if ( w && w->IsFocusable( *this ) )
                {
                    SetFocus( child->WidgetID );
                    return;
                }
            }
        };

        if ( a_Action == ENavAction::Cancel )
        {
            PopNavScope();
            return;
        }

        if ( a_Action == ENavAction::ActivatePressed || a_Action == ENavAction::ActivateReleased )
        {
            WidgetID focused = GetFocusedWidget();
            if ( IWidget* w = GetWidget( focused ) )
            {
                LayoutNode* node = Layouts.Get( w->GetLayoutID() );

                // If pressing and the widget is a focus scope, enter it
                if ( a_Action == ENavAction::ActivatePressed && node && node->Style.IsFocusScope )
                {
                    PushNavScope( focused );
                    FocusFirstIn( focused );
                    return;
                }

                const ButtonEvent ev{
                    .Button = EButtonID::KeyEnter,
                    .Pressed = ( a_Action == ENavAction::ActivatePressed ),
                    .Released = ( a_Action == ENavAction::ActivateReleased ),
                    .Held = ( a_Action == ENavAction::ActivatePressed ),
                };

                w->OnPressed( *this, ev );  // OnPressed checks ev.Pressed internally
                w->OnReleased( *this, ev ); // OnReleased checks ev.Released internally
            }
            return;
        }

        // Directional nav within current scope
        WidgetID scopeID = GetCurrentNavScope();
        WidgetID focused = GetFocusedWidget();

        IWidget*    scopeWidget    = GetWidget( scopeID );
        LayoutNode* scopeNode      = scopeWidget ? Layouts.Get( scopeWidget->GetLayoutID() ) : nullptr;
        IWidget*    focusedWidget  = GetWidget( focused );
        LayoutNode* focusedNode    = focusedWidget ? Layouts.Get( focusedWidget->GetLayoutID() ) : nullptr;

        if ( !scopeNode ) return;

        if ( !focusedNode )
        {
            FocusFirstIn( scopeID );
            return;
        }

        auto focusableNodes = Detail::LayoutChildRange{ scopeNode->FirstChild() }
            | std::views::filter( [&]( LayoutNode* node ) -> bool
            {
                if ( !node ) return false;
                IWidget* w = GetWidget( node->WidgetID );
                // Include both leaf-focusable widgets and scope containers
                return w && ( w->IsFocusable( *this ) || node->Style.IsFocusScope );
            });

        const LayoutNode* nextNode = FindNavigatableNode( a_Action, focusedNode, focusableNodes );

        if ( nextNode )
        {
            SetFocus( nextNode->WidgetID );
        }

        // If no candidate found, optionally pop scope (navigated off the edge)
        // else PopScope();
    }

    inline void Scene::PushNavScope( WidgetID a_ScopeID )
    {
        PushBack( m_NavStack, NavScope{ 
            .Scope = a_ScopeID, 
            .Restored = GetFocusedWidget() 
        } );
    }

    inline void Scene::PopNavScope()
    {
        ClearFocus();

        if ( Empty( m_NavStack ) )
			return; // No scopes to pop, already at root
            
        WidgetID restored = Back( m_NavStack ).Restored;
        PopBack( m_NavStack );
        if ( restored != c_InvalidPoolID )
            SetFocus( restored );
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

		node->DetachFromParent(); // Unlink from parent to avoid dangling references during recursive destruction

        // Recursively destroy child widgets
        node->ForEachChild( [&]( LayoutNode& childNode )
        {
            DestroyWidget( childNode.WidgetID );
        });

        widget->OnDestroy( *this ); // TODO: Does it make more sense to call this before or after destroying children?

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
        m_HoveredWidget = c_InvalidPoolID;
        ClearFocus();
        Clear( m_NavStack );
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
                    parentNode->PushBackChild( *node );
            }
        }

        // Call construct after fully initialized and linked into hierarchy, in case widget logic depends on that
        widget->OnConstruct( *this );

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

    inline WidgetID Scene::HitTest( WidgetID a_ID, Vec2<Unit> a_LogicalPos )
    {
        IWidget* widget = GetWidget( a_ID );
        if ( !widget )
            return c_InvalidPoolID;

        LayoutNode* node = Layouts.Get( widget->GetLayoutID() );
        if ( !node )
             return c_InvalidPoolID;

        if ( !node->Layout.FinalRect.Contains( a_LogicalPos ) ) 
            return c_InvalidPoolID;

        WidgetID result = c_InvalidPoolID;

        if ( Visibility::AreChildrenHitTestable( node->Layout.Visibility ) )
        {
            // Check children first (front-to-back, last child wins)
            node->ForEachChild( [&]( LayoutNode& child )
            {
                WidgetID childHit = HitTest( child.WidgetID, a_LogicalPos );
                if ( childHit != c_InvalidPoolID )
                    result = childHit; // deepest child takes priority
            } );

            if ( result != c_InvalidPoolID )
                return result;
        }

        if ( Visibility::IsHitTestable( node->Layout.Visibility ) )
            return a_ID;

        return c_InvalidPoolID;
    }

    inline bool Scene::ProcessPointerEvent( const PointerEvent& a_Event )
    {
        if ( !a_Event.IsMouse() ) 
			return false; // TODO: Only process mouse events for now

		m_LastPointerEvent = a_Event;

        WidgetID hovered = HitTest( RootWidget, a_Event.Position );
        if ( hovered != m_HoveredWidget )
        {
            if ( IWidget* prevHovered = GetWidget( m_HoveredWidget ) )
                prevHovered->OnPointerExit( *this, a_Event );

            m_HoveredWidget = hovered;

            if ( IWidget* newHovered = GetWidget( m_HoveredWidget ) )
                newHovered->OnPointerEnter( *this, a_Event );
        }

        return false;
    }

    inline bool Scene::ProcessButtonEvent( const ButtonEvent& a_Event )
    {
        if ( IWidget* hovered = GetWidget( m_HoveredWidget ) )
        {
            bool consumed = false;

            if ( a_Event.Pressed )
                consumed |= hovered->OnPressed( *this, a_Event );
            else
                consumed |= hovered->OnReleased( *this, a_Event );

            if ( consumed )
				return true; // Event handled by hovered widget
		}

        if ( IWidget* focused = GetWidget( GetFocusedWidget() ) )
        {
            bool consumed = false;

            if ( a_Event.Pressed )
                consumed |= focused->OnPressed( *this, a_Event );
            else
                consumed |= focused->OnReleased( *this, a_Event );

            if ( consumed )
                return true; // Event handled by focused widget
        }

        return false; // Event not handled
    }


} // namespace RatUI
