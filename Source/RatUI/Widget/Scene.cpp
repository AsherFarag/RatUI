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
        LayoutNode* rootNode = Layouts.Get( RootWidget );
        if ( !rootNode )
            return;

        const auto SyncSubtree = [&]( auto& Self, LayoutNode& node, Vec2<Unit> parentSize ) -> bool
        {
            bool anyChanged = false;

            if ( node.Widget )
            {
                const Vec2<Unit> oldIntrinsics = node.Layout.IntrinsicSize;
                node.Widget->OnSyncLayout( node, parentSize );
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
        if ( LayoutNode* rootNode = Layouts.Get( RootWidget ) )
            if ( rootNode->Widget )
                rootNode->Widget->Paint( a_DrawList );
    }

    void Scene::SetFocus( NodeID a_NodeID )
    {
        FocusEvent focusEvent; // TODO: Populate this with useful information?

        // Callers should not need to check IsFocusable() before calling SetFocus().
        if ( LayoutNode* targetNode = Layouts.Get( a_NodeID ) )
        {
            if ( targetNode->Widget && !targetNode->Widget->IsFocusable() )
                return;
        }

        if ( LayoutNode* currentNode = Layouts.Get( m_FocusedWidget ) )
        {
            if ( m_FocusedWidget == a_NodeID )
                return;

            if ( currentNode->Widget )
                currentNode->Widget->OnFocusLost( focusEvent );
        }

        m_FocusedWidget = a_NodeID;

        if ( LayoutNode* newNode = Layouts.Get( m_FocusedWidget ) )
            if ( newNode->Widget )
                newNode->Widget->OnFocusReceived( focusEvent );
    }

    NavReply Scene::QueryBoundaryReply( ENavAction a_Action, NodeID a_Focused )
    {
        // Walk up from the focused node asking each navigation boundary widget.
        LayoutNode* node = Layouts.Get( a_Focused );
        while ( node )
        {
            if ( node->Widget && node->Widget->IsNavigationBoundary() )
                return node->Widget->OnNavigationBoundary( a_Action );

            node = node->Parent();
        }

        return NavReply::Escape();
    }

    void Scene::Navigate( ENavAction a_Action )
    {
        const auto FocusFirstIn = [&]( NodeID a_ScopeID )
        {
            LayoutNode* scopeNode = Layouts.Get( a_ScopeID );
            if ( !scopeNode ) return;

            for ( LayoutNode* child = scopeNode->FirstChild(); child; child = child->NextSibling() )
            {
                if ( child->Widget && child->Widget->IsFocusable() )
                {
                    SetFocus( child->Widget->GetLayoutID() );
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
            LayoutNode* focusedNode = Layouts.Get( m_FocusedWidget );
            if ( !focusedNode || !focusedNode->Widget )
                return;

            if ( a_Action == ENavAction::ActivatePressed && focusedNode->Style.IsFocusScope )
            {
                PushNavScope( m_FocusedWidget );
                FocusFirstIn( m_FocusedWidget );
                return;
            }

            const ButtonEvent ev{
                .Button = EButtonID::KeyEnter,
                .Pressed = ( a_Action == ENavAction::ActivatePressed ),
                .Released = ( a_Action == ENavAction::ActivateReleased ),
                .Held = ( a_Action == ENavAction::ActivatePressed ),
            };

            // TODO: Shouldnt be making fake events like this, should add something like IWidget::OnPressed/OnHovered etc??
            Reply reply = a_Action == ENavAction::ActivatePressed
                ? focusedNode->Widget->OnButtonPressed( ev )
                : focusedNode->Widget->OnButtonReleased( ev );

            ApplyReply( reply );
            return;
        }

        NodeID      scopeID     = GetCurrentNavScope();
        LayoutNode* scopeNode   = Layouts.Get( scopeID );
        LayoutNode* focusedNode = Layouts.Get( m_FocusedWidget );

        if ( !scopeNode )
            return;

        if ( !focusedNode )
        {
            FocusFirstIn( scopeID );
            return;
        }

        // TODO: If I move navigation from LayoutNodes to Widgets, I should remove this.
        // TODO: Though I do like not having explicit widget types for containers like VBox/HBox/Grid etc and still keep nav for them.

        auto focusableNodes = LayoutChildRange{ scopeNode->FirstChild() }
            | std::views::filter( [&]( LayoutNode* node ) -> bool
            {
                if ( !node ) return false;
                // TODO: Probs dont need LayoutStyle::IsFocusScope anymore
                return ( node->Widget && node->Widget->IsFocusable() ) || node->Style.IsFocusScope;
            } );

        const LayoutNode* nextNode = FindNavigatableNode( a_Action, focusedNode, focusableNodes );

        if ( nextNode )
        {
            SetFocus( nextNode->Widget->GetLayoutID() );
            return;
        }

        // No target found within the current scope - consult boundary policy.
        const NavReply navReply = QueryBoundaryReply( a_Action, m_FocusedWidget );

        switch ( navReply.GetRule() )
        {
            case NavReply::EBoundaryRule::Escape:
                PopNavScope();
                break;

            case NavReply::EBoundaryRule::Stop:
                // Wrap: focus the first/last focusable child depending on direction.
                FocusFirstIn( scopeID );
                break;

            case NavReply::EBoundaryRule::Explicit:
                SetFocus( navReply.GetExplicitTarget() );
                break;

            case NavReply::EBoundaryRule::Custom:
            {
                NodeID target = navReply.ResolveCustom( a_Action, m_FocusedWidget );
                if ( target != c_InvalidNodeID )
                    SetFocus( target );
                break;
            }
        }
    }

    void Scene::PushNavScope( NodeID a_ScopeID )
    {
        PushBack( m_NavStack, NavScope{
            .Scope = a_ScopeID,
            .Restored = m_FocusedWidget,
        } );
    }

    void Scene::PopNavScope()
    {
        ClearFocus();

        if ( Empty( m_NavStack ) )
            return;

        NodeID restored = Back( m_NavStack ).Restored;
        PopBack( m_NavStack );

        if ( restored != c_InvalidNodeID )
            SetFocus( restored );
    }

    IWidget* Scene::GetWidget( NodeID a_ID )
    {
        LayoutNode* node = Layouts.Get( a_ID );
        return node ? node->Widget.get() : nullptr;
    }

    const IWidget* Scene::GetWidget( NodeID a_ID ) const
    {
        const LayoutNode* node = Layouts.Get( a_ID );
        return node ? node->Widget.get() : nullptr;
    }

    bool Scene::DestroyWidget( NodeID a_NodeID )
    {
        LayoutNode* node = Layouts.Get( a_NodeID );
        if ( !node )
            return false;

        node->DetachFromParent();

        node->ForEachChild( [&]( LayoutNode& child )
        {
            if ( child.Widget )
                DestroyWidget( child.Widget->GetLayoutID() );
        } );

        if ( node->Widget )
            node->Widget->OnDestroy();

        Layouts.Deallocate( a_NodeID );

        return true;
    }

    void Scene::Reset()
    {
        Layouts.Clear();
        RootWidget      = c_InvalidNodeID;
        m_HoveredWidget = c_InvalidNodeID;
        ClearFocus();
        Clear( m_NavStack );
    }

    NodeID Scene::HitTest( NodeID a_ID, Vec2<Unit> a_LogicalPos )
    {
        LayoutNode* rootNode = Layouts.Get( a_ID );
        if ( !rootNode )
            return c_InvalidNodeID;

        const auto hitTestNode = [&]( auto&& Self, LayoutNode* a_Node ) -> NodeID
        {
            if ( !a_Node )
                return c_InvalidNodeID;

            if ( !a_Node->Layout.FinalRect.Contains( a_LogicalPos ) )
                return c_InvalidNodeID;

            NodeID result = c_InvalidNodeID;

            if ( Visibility::AreChildrenHitTestable( a_Node->Layout.Visibility ) )
            {
                a_Node->ForEachChild( [&]( LayoutNode& child )
                {
                    NodeID childHit = Self( Self, &child );
                    if ( childHit != c_InvalidNodeID )
                        result = childHit;
                });

                if ( result != c_InvalidNodeID )
                    return result;
            }

            // Only return this node as a hit target if:
            // it is both visibility-hit-testable 
            // AND the widget explicitly opts in via IsInteractable().
            // Non-interactive widgets (Panel, decorative containers, etc) fall
            // through so their interactable ancestor can receive the event instead.
            if ( a_Node->Widget && Visibility::IsHitTestable( a_Node->Layout.Visibility ) )
            {
                if ( a_Node->Widget->IsInteractable() )
                    return a_Node->Widget->GetLayoutID();
            }

            return c_InvalidNodeID;
        };

        return hitTestNode( hitTestNode, rootNode );
    }

    void Scene::ApplyReply( const Reply& a_Reply )
    {
        if ( a_Reply.ShouldCaptureMouse() )
            CapturePointer( a_Reply.GetMouseCaptureTarget() );

        if ( a_Reply.ShouldReleaseMouse() )
            ReleasePointerCapture();

        if ( a_Reply.ShouldSetFocus() )
            SetFocus( a_Reply.GetFocusTarget() );
        else if ( a_Reply.ShouldClearFocus() )
            ClearFocus();
    }

    bool Scene::ProcessPointerEvent( const PointerEvent& a_Event )
    {
        if ( !a_Event.IsMouse() )
            return false;

        m_LastPointerEvent = a_Event;

        // --- Scroll ---
        if ( a_Event.ScrollDelta[0] != 0_u || a_Event.ScrollDelta[1] != 0_u )
        {
            NodeID scrollTarget = m_CapturedWidget != c_InvalidNodeID
                ? m_CapturedWidget
                : HitTest( RootWidget, a_Event.Position );

            if ( IWidget* w = GetWidget( scrollTarget ) )
            {
                Reply reply = w->OnPointerScroll( a_Event );
                ApplyReply( reply );
            }

            return false;
        }

        // --- Move (captured widget takes priority) ---
        if ( m_CapturedWidget != c_InvalidNodeID )
        {
            if ( IWidget* w = GetWidget( m_CapturedWidget ) )
            {
                Reply reply = w->OnPointerMove( a_Event );
                ApplyReply( reply );
            }
            return false;
        }

        // --- Hover tracking ---
        NodeID hovered = HitTest( RootWidget, a_Event.Position );

        if ( hovered != m_HoveredWidget )
        {
            if ( IWidget* prevHovered = GetWidget( m_HoveredWidget ) )
            {
                Reply reply = prevHovered->OnPointerExit( a_Event );
                ApplyReply( reply );
            }

            m_HoveredWidget = hovered;

            if ( IWidget* newHovered = GetWidget( m_HoveredWidget ) )
            {
                Reply reply = newHovered->OnPointerEnter( a_Event );
                ApplyReply( reply );
            }
        }
        else if ( hovered != c_InvalidNodeID )
        {
            if ( IWidget* w = GetWidget( hovered ) )
            {
                Reply reply = w->OnPointerMove( a_Event );
                ApplyReply( reply );
            }
        }

        return false;
    }

    bool Scene::ProcessButtonEvent( const ButtonEvent& a_Event )
    {
        // Captured widget gets all button events first during an active drag/press.
        if ( m_CapturedWidget != c_InvalidNodeID )
        {
            if ( IWidget* w = GetWidget( m_CapturedWidget ) )
            {
                Reply reply = a_Event.Pressed
                    ? w->OnButtonPressed( a_Event )
                    : w->OnButtonReleased( a_Event );

                ApplyReply( reply );

                // Release capture on mouse button up regardless of whether the
                // widget handled it - a capture shouldn't outlive its button press.
                if ( a_Event.Released )
                    ReleasePointerCapture();

                if ( reply.IsHandled() )
                    return true;
            }
        }

        // Hovered widget gets mouse button events.
        if ( IWidget* hovered = GetWidget( m_HoveredWidget ) )
        {
            Reply reply = a_Event.Pressed
                ? hovered->OnButtonPressed( a_Event )
                : hovered->OnButtonReleased( a_Event );

            ApplyReply( reply );

            if ( reply.IsHandled() )
                return true;
        }

        // Focused widget gets keyboard/gamepad events that the hovered widget
        // didn't consume. Skip if hovered == focused to avoid double-dispatch.
        if ( m_FocusedWidget != m_HoveredWidget )
        {
            if ( IWidget* focused = GetWidget( m_FocusedWidget ) )
            {
                Reply reply = a_Event.Pressed
                    ? focused->OnButtonPressed( a_Event )
                    : focused->OnButtonReleased( a_Event );

                ApplyReply( reply );

                if ( reply.IsHandled() )
                    return true;
            }
        }

        return false;
    }

} // namespace RatUI
