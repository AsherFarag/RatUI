#include <RatUI/Widget/Scene.h>

namespace RatUI
{
    // TODO: Make this a reusable utility?
    namespace 
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

    } // namespace

    bool Scene::DispatchInputEvent( const InputEvent& a_Event )
    {
        if ( Holds<PointerEvent>( a_Event.Payload ) )
            return ProcessPointerEvent( Get<PointerEvent>( a_Event.Payload ) );

        if ( Holds<ButtonEvent>( a_Event.Payload ) )
            return ProcessButtonEvent( Get<ButtonEvent>( a_Event.Payload ) );

        return false;
    }

    void Scene::UpdateLayout( Vec2<Unit> a_AvailableSize )
    {
        IWidget* root = GetWidget( RootWidget );
        if ( !root )
            return;
        LayoutNode* rootNode = Layouts.Get( root->GetLayoutID() );
        if ( !rootNode )
            return;

        const auto SyncSubtree = [&]( auto& Self, LayoutNode& node, Vec2<Unit> parentSize ) -> bool
        {
            bool anyChanged = false;

            if ( IWidget* widget = GetWidget( node.Widget ) )
            {
                const Vec2<Unit> oldIntrinsics = node.Layout.IntrinsicSize;
                widget->OnSyncLayout( node, parentSize );
                anyChanged |= ( node.Layout.IntrinsicSize != oldIntrinsics );
            }

            const Vec2<Unit> innerSize( node.Layout.FinalRect.Size[0] - node.Style.Padding.Horizontal(),
                                        node.Layout.FinalRect.Size[1] - node.Style.Padding.Vertical() );
            node.ForEachChild( [&]( LayoutNode& child )
                               { anyChanged |= Self( Self, child, innerSize ); } );

            return anyChanged;
        };

        SyncSubtree( SyncSubtree, *rootNode, a_AvailableSize );
        MeasureLayoutNode( *rootNode, a_AvailableSize );
        ArrangeLayoutNode( *rootNode, Rect<Unit>{ Vec2<Unit>( 0_u, 0_u ), a_AvailableSize } );

        if ( const bool needsSecondPass = SyncSubtree( SyncSubtree, *rootNode, a_AvailableSize ) )
        {
            (void)needsSecondPass;
            MeasureLayoutNode( *rootNode, a_AvailableSize );
            ArrangeLayoutNode( *rootNode, Rect<Unit>{ Vec2<Unit>( 0_u, 0_u ), a_AvailableSize } );
        }

        ProcessPointerEvent( m_LastPointerEvent );
    }

    void Scene::Render( DrawList& a_DrawList )
    {
        if ( IWidget* root = GetWidget( RootWidget ) )
            root->OnPaint( a_DrawList );
    }

    void Scene::SetFocus( WidgetID a_WidgetID )
    {
        if ( IWidget* currentFocus = GetWidget( m_FocusedWidget ) )
        {
            if ( currentFocus->GetID() == a_WidgetID )
                return;

            currentFocus->OnFocusLost();
        }

        m_FocusedWidget = a_WidgetID;
        if ( IWidget* newFocus = GetWidget( m_FocusedWidget ) )
            newFocus->OnFocusReceived();
    }

    void Scene::Navigate( ENavAction a_Action )
    {
        const auto FocusFirstIn = [&]( WidgetID a_ScopeID )
        {
            IWidget* scopeWidget = GetWidget( a_ScopeID );
            if ( !scopeWidget )
                return;

            LayoutNode* scopeNode = Layouts.Get( scopeWidget->GetLayoutID() );
            if ( !scopeNode )
                return;

            for ( LayoutNode* child = scopeNode->FirstChild(); child; child = child->NextSibling() )
            {
                IWidget* w = GetWidget( child->Widget );
                if ( w && w->IsFocusable() )
                {
                    SetFocus( child->Widget );
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

                w->OnPressed( ev );
                w->OnReleased( ev );
            }
            return;
        }

        WidgetID scopeID = GetCurrentNavScope();
        WidgetID focused = GetFocusedWidget();

        IWidget* scopeWidget = GetWidget( scopeID );
        LayoutNode* scopeNode = scopeWidget ? Layouts.Get( scopeWidget->GetLayoutID() ) : nullptr;
        IWidget* focusedWidget = GetWidget( focused );
        LayoutNode* focusedNode = focusedWidget ? Layouts.Get( focusedWidget->GetLayoutID() ) : nullptr;

        if ( !scopeNode )
            return;

        if ( !focusedNode )
        {
            FocusFirstIn( scopeID );
            return;
        }

        auto focusableNodes = LayoutChildRange{ scopeNode->FirstChild() }
                              | std::views::filter( [&]( LayoutNode* node ) -> bool
                                {
                                    if ( !node ) return false;
                                    IWidget* w = GetWidget( node->Widget );
                                    return w && ( w->IsFocusable() || node->Style.IsFocusScope );
                                } );

        const LayoutNode* nextNode = FindNavigatableNode( a_Action, focusedNode, focusableNodes );
        if ( nextNode )
            SetFocus( nextNode->Widget );
    }

    void Scene::PushNavScope( WidgetID a_ScopeID )
    {
        PushBack( m_NavStack, NavScope{
            .Scope = a_ScopeID,
            .Restored = GetFocusedWidget(),
        } );
    }

    void Scene::PopNavScope()
    {
        ClearFocus();

        if ( Empty( m_NavStack ) )
            return;

        WidgetID restored = Back( m_NavStack ).Restored;
        PopBack( m_NavStack );
        if ( restored != c_InvalidWidgetID )
            SetFocus( restored );
    }

    IWidget* Scene::GetWidget( WidgetID a_ID )
    {
        if ( Unique<IWidget>* widget = Widgets.Get( a_ID ) )
            return widget->get();

        return nullptr;
    }

    const IWidget* Scene::GetWidget( WidgetID a_ID ) const
    {
        if ( const Unique<IWidget>* widget = Widgets.Get( a_ID ) )
            return widget->get();

        return nullptr;
    }

    bool Scene::DestroyWidget( WidgetID a_WidgetID )
    {
        IWidget* widget = GetWidget( a_WidgetID );
        if ( !widget )
            return false;

        LayoutNode* node = Layouts.Get( widget->GetLayoutID() );
        if ( !node )
            return false;

        node->DetachFromParent();

        node->ForEachChild( [&]( LayoutNode& child ) { DestroyWidget( child.Widget ); } );

        widget->OnDestroy();
        Widgets.Deallocate( widget->GetID() );
        Layouts.Deallocate( widget->GetLayoutID() );

        return true;
    }

    void Scene::Reset()
    {
        Layouts.Clear();
        Widgets.Clear();
        RootWidget = c_InvalidWidgetID;
        m_HoveredWidget = c_InvalidWidgetID;
        ClearFocus();
        Clear( m_NavStack );
    }

    WidgetID Scene::HitTest( WidgetID a_ID, Vec2<Unit> a_LogicalPos )
    {
        IWidget* rootWidget = GetWidget( a_ID );
        if ( !rootWidget )
            return c_InvalidWidgetID;

        LayoutNode* rootNode = Layouts.Get( rootWidget->GetLayoutID() );
        if ( !rootNode )
            return c_InvalidWidgetID;

        const auto HitTestNode = [&]( auto&& Self, LayoutNode* a_Node ) -> WidgetID
        {
            if ( !a_Node )
                return c_InvalidWidgetID;

            if ( !a_Node->Layout.FinalRect.Contains( a_LogicalPos ) )
                return c_InvalidWidgetID;

            WidgetID result = c_InvalidWidgetID;

            if ( Visibility::AreChildrenHitTestable( a_Node->Layout.Visibility ) )
            {
                a_Node->ForEachChild( [&]( LayoutNode& child )
                                      {
                    WidgetID childHit = Self( Self, &child );
                    if ( childHit != c_InvalidWidgetID )
                        result = childHit; } );

                if ( result != c_InvalidWidgetID )
                    return result;
            }

            if ( Visibility::IsHitTestable( a_Node->Layout.Visibility ) &&
                 a_Node->Widget != c_InvalidWidgetID )
            {
                return a_Node->Widget;
            }

            return c_InvalidWidgetID;
        };

        return HitTestNode( HitTestNode, rootNode );
    }

    bool Scene::ProcessPointerEvent( const PointerEvent& a_Event )
    {
        if ( !a_Event.IsMouse() )
            return false;

        m_LastPointerEvent = a_Event;

        // --- Scroll ---
        if ( a_Event.ScrollDelta[0] != 0_u || a_Event.ScrollDelta[1] != 0_u )
        {
            WidgetID scrollTarget = m_CapturedWidget != c_InvalidWidgetID
                ? m_CapturedWidget
                : HitTest( RootWidget, a_Event.Position );

            if ( IWidget* w = GetWidget( scrollTarget ) )
                w->OnPointerScroll( a_Event );

            return false;
        }

        // --- Move (captured widget takes priority) ---
        if ( m_CapturedWidget != c_InvalidWidgetID )
        {
            if ( IWidget* w = GetWidget( m_CapturedWidget ) )
                w->OnPointerMove( a_Event );
            return false;
        }

        // --- Hover tracking ---
        WidgetID hovered = HitTest( RootWidget, a_Event.Position );
        if ( hovered != m_HoveredWidget )
        {
            if ( IWidget* prevHovered = GetWidget( m_HoveredWidget ) )
                prevHovered->OnPointerExit( a_Event );

            m_HoveredWidget = hovered;

            if ( IWidget* newHovered = GetWidget( m_HoveredWidget ) )
                newHovered->OnPointerEnter( a_Event );
        }
        else if ( hovered != c_InvalidWidgetID )
        {
            if ( IWidget* w = GetWidget( hovered ) )
                w->OnPointerMove( a_Event );
        }

        return false;
    }

    bool Scene::ProcessButtonEvent( const ButtonEvent& a_Event )
    {
        // Release pointer capture on any mouse button release
        if ( a_Event.Released && m_CapturedWidget != c_InvalidWidgetID )
        {
            if ( IWidget* w = GetWidget( m_CapturedWidget ) )
                w->OnReleased( a_Event );

            ReleasePointerCapture();
            return true;
        }

        if ( IWidget* hovered = GetWidget( m_HoveredWidget ) )
        {
            bool consumed = false;
            if ( a_Event.Pressed )  consumed |= hovered->OnPressed( a_Event );
            else                    consumed |= hovered->OnReleased( a_Event );

            if ( consumed ) return true;
        }

        if ( IWidget* focused = GetWidget( GetFocusedWidget() ) )
        {
            bool consumed = false;
            if ( a_Event.Pressed )  consumed |= focused->OnPressed( a_Event );
            else                    consumed |= focused->OnReleased( a_Event );

            if ( consumed ) return true;
        }

        return false;
    }

} // namespace RatUI
