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
        void ClearFocus() { SetFocus( c_InvalidWidgetID ); }

        void CapturePointer( WidgetID a_WidgetID ) { m_CapturedWidget = a_WidgetID; }

        void ReleasePointerCapture() { m_CapturedWidget = c_InvalidWidgetID; }

        WidgetID GetCapturedWidget() const { return m_CapturedWidget; }

        // - Navigation

        void Navigate( ENavAction a_Action );

        void PushNavScope( WidgetID a_ScopeID );
        void PopNavScope();
        WidgetID GetCurrentNavScope() const { return Empty( m_NavStack ) ? RootWidget : Back( m_NavStack ).Scope; }

        // - Widget Management

        template<std::derived_from<IWidget> WidgetType, typename... Args>
        WidgetID CreateWidget( NodeID a_ParentID, Args&&... a_Args );

        template<std::derived_from<IWidget> WidgetType, typename... Args>
        WidgetID CreateRootWidget( Args&&... a_Args );

        template<std::derived_from<IWidget> WidgetType, typename... Args>
        WidgetID CreateWidget( WidgetID a_ParentID, Args&&... a_Args )
        {
			if ( IWidget* parentWidget = GetWidget( a_ParentID ) )
				return CreateWidget<WidgetType>( parentWidget->GetLayoutID(), std::forward<Args>( a_Args )... );

			return c_InvalidWidgetID;
        }

        /** @brief Destroys the widget with the specified ID, including its children. */
        bool DestroyWidget( WidgetID a_WidgetID );

        RATUI_NODISCARD IWidget* GetWidget( WidgetID a_ID );

        RATUI_NODISCARD const IWidget* GetWidget( WidgetID a_ID ) const;

        template<std::derived_from<IWidget> WidgetType>
        RATUI_NODISCARD WidgetType* GetWidget( WidgetID a_ID ) { return dynamic_cast<WidgetType*>( GetWidget( a_ID ) ); }

		template<std::derived_from<IWidget> WidgetType>
        RATUI_NODISCARD const WidgetType* GetWidget( WidgetID a_ID ) const { return dynamic_cast<const WidgetType*>( GetWidget( a_ID ) ); }

        template<std::invocable<IWidget&> Func>
        void ForEachChildWidget( NodeID a_NodeID, Func&& a_Func );

        template<std::invocable<const IWidget&> Func>
        void ForEachChildWidget( NodeID a_NodeID, Func&& a_Func ) const;

        /** @brief Clears the scene */
        void Reset();

    protected:
        bool ProcessPointerEvent( const PointerEvent& a_Event );
        bool ProcessButtonEvent( const ButtonEvent& a_Event );
        WidgetID HitTest( WidgetID a_ID, Vec2<Unit> a_LogicalPos );

        WidgetID m_FocusedWidget{ c_InvalidWidgetID };
        WidgetID m_HoveredWidget{ c_InvalidWidgetID };
        WidgetID m_CapturedWidget{ c_InvalidWidgetID };
        PointerEvent m_LastPointerEvent{}; ///< The last pointer event received, used for hit testing and hover state management.

        struct NavScope
        {
            WidgetID Scope{ c_InvalidWidgetID };    ///< The container widget
            WidgetID Restored{ c_InvalidWidgetID }; ///< The widget to restore on pop
        };
        Array<NavScope> m_NavStack{}; ///< Stack of navigation scopes used to manage focus during keyboard/gamepad navigation, allowing for nested navigation contexts.
    };

    // === Inline Implementations ===


    template<std::derived_from<IWidget> WidgetType, typename... Args>
    WidgetID Scene::CreateRootWidget( Args&&... a_Args )
    {
        WidgetID id = CreateWidget<WidgetType>( c_InvalidNodeID, std::forward<Args>( a_Args )... );
        RootWidget = id;
        return id;
    }

    template<std::derived_from<IWidget> WidgetType, typename... Args>
    WidgetID Scene::CreateWidget( NodeID a_ParentID, Args&&... a_Args )
    {
		Unique<IWidget> widgetPtr = MakeUnique<WidgetType>( std::forward<Args>( a_Args )... );
		IWidget* widget = widgetPtr.get();

        // Allocate layout node and widget
        NodeID     nodeID   = Layouts.Allocate();
		WidgetID   widgetID = Widgets.Allocate( std::move( widgetPtr ) );

        LayoutNode* node   = Layouts.Get( nodeID );

        // Set scene pointer for the widget
        widget->m_Scene = this;

        // Wire widget <-> node
        widget->m_ID       = widgetID;
        widget->m_LayoutID = nodeID;
        node->Widget       = widgetID;

		if ( a_ParentID != c_InvalidNodeID )
        {
			if ( LayoutNode* parentNode = Layouts.Get( a_ParentID ) )
				parentNode->PushBackChild( *node );
        }

        // Call construct after fully initialized and linked into hierarchy, in case widget logic depends on that
        widget->OnConstruct();

        return widgetID;
    }

    template<std::invocable<IWidget&> Func>
    void Scene::ForEachChildWidget( NodeID a_NodeID, Func&& a_Func )
    {
        LayoutNode* node = Layouts.Get( a_NodeID );
        if ( !node ) return;

        node->ForEachChild( [&]( LayoutNode& childNode )
        {
            IWidget* childWidget = GetWidget( childNode.Widget );
            if ( childWidget ) a_Func( *childWidget );
        } );
    }

    template<std::invocable<const IWidget&> Func>
    void Scene::ForEachChildWidget( NodeID a_NodeID, Func&& a_Func ) const
    {
        const LayoutNode* node = Layouts.Get( a_NodeID );
        if ( !node ) return;

        node->ForEachChild( [&]( const LayoutNode& childNode )
        {
            const IWidget* childWidget = GetWidget( childNode.Widget );
            if ( childWidget ) a_Func( *childWidget );
        } );
    }

} // namespace RatUI
