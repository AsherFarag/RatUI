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
		FocusEvent focusEvent; // TODO: Populate this with useful information?

        // Callers should not need to check IsFocusable() before calling SetFocus().
        if ( IWidget* target = GetWidget( a_WidgetID ) )
        {
            if ( !target->IsFocusable() )
                return;
        }

        if ( IWidget* currentFocus = GetWidget( m_FocusedWidget ) )
        {
            if ( currentFocus->GetID() == a_WidgetID )
                return;

            currentFocus->OnFocusLost( focusEvent );
        }

        m_FocusedWidget = a_WidgetID;

        if ( IWidget* newFocus = GetWidget( m_FocusedWidget ) )
            newFocus->OnFocusReceived( focusEvent );
    }

    NavigationReply Scene::QueryBoundaryReply( ENavAction a_Action, WidgetID a_Focused )
    {
        // Walk up from the focused widget asking each navigation boundary widget.
        IWidget* w = GetWidget( a_Focused );
        while ( w )
        {
            if ( w->IsNavigationBoundary() )
                return w->OnNavigationBoundary( a_Action );

            LayoutNode* node = Layouts.Get( w->GetLayoutID() );
            LayoutNode* parent = node ? node->Parent() : nullptr;
            w = parent ? GetWidget( parent->Widget ) : nullptr;
        }

        return NavigationReply::Escape();
    }

    void Scene::Navigate( ENavAction a_Action )
    {
        const auto FocusFirstIn = [&]( WidgetID a_ScopeID )
        {
            IWidget* scopeWidget = GetWidget( a_ScopeID );
            if ( !scopeWidget ) return;

            LayoutNode* scopeNode = Layouts.Get( scopeWidget->GetLayoutID() );
            if ( !scopeNode ) return;

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

                // TODO: Shouldnt be making fake events like this, should add something like IWidget::OnPressed/OnHovered etc??
                Reply reply = a_Action == ENavAction::ActivatePressed
                    ? w->OnButtonPressed( ev )
                    : w->OnButtonReleased( ev );

                ApplyReply( reply );
            }
            return;
        }

        WidgetID    scopeID       = GetCurrentNavScope();
        WidgetID    focused       = GetFocusedWidget();
        IWidget*    scopeWidget   = GetWidget( scopeID );
        LayoutNode* scopeNode     = scopeWidget ? Layouts.Get( scopeWidget->GetLayoutID() ) : nullptr;
        IWidget*    focusedWidget = GetWidget( focused );
        LayoutNode* focusedNode   = focusedWidget ? Layouts.Get( focusedWidget->GetLayoutID() ) : nullptr;

        if ( !scopeNode )
            return;

        if ( !focusedNode )
        {
            FocusFirstIn( scopeID );
            return;
        }

        // TODO: If I move navigation from LayoutNodes to Widgets, I should remove ts
		// TODO: Though I do like not having explicit widget types for containers like VBox/HBox/Grid etc and still keep nav for them.

        auto focusableNodes = LayoutChildRange{ scopeNode->FirstChild() }
            | std::views::filter( [&]( LayoutNode* node ) -> bool
            {
                if ( !node ) return false;
                IWidget* w = GetWidget( node->Widget );
                // TODO: Probs dont need LayoutStyle::IsFocusScope anymore
                return w && ( w->IsFocusable() || node->Style.IsFocusScope );
            } );

        const LayoutNode* nextNode = FindNavigatableNode( a_Action, focusedNode, focusableNodes );

        if ( nextNode )
        {
            SetFocus( nextNode->Widget );
            return;
        }

        // No target found within the current scope - consult boundary policy.
        const NavigationReply navReply = QueryBoundaryReply( a_Action, focused );

        switch ( navReply.GetRule() )
        {
            case NavigationReply::EBoundaryRule::Escape:
                PopNavScope();
                break;

            case NavigationReply::EBoundaryRule::Stop:
                // Wrap: focus the first/last focusable child depending on direction.
                FocusFirstIn( scopeID );
                break;

            case NavigationReply::EBoundaryRule::Explicit:
                SetFocus( navReply.GetExplicitTarget() );
                break;

            case NavigationReply::EBoundaryRule::Custom:
            {
                WidgetID target = navReply.ResolveCustom( a_Action, focused );
                if ( target != c_InvalidWidgetID )
                    SetFocus( target );
                break;
            }
        }
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

        const auto hitTestNode = [&]( auto&& Self, LayoutNode* a_Node ) -> WidgetID
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
                        result = childHit;
                });

                if ( result != c_InvalidWidgetID )
                    return result;
            }

            // Only return this node as a hit target if:
            // it is both visibility-hit-testable 
            // AND the widget explicitly opts in via IsInteractable().
            // Non-interactive widgets (Panel, decorative containers, etc) fall
            // through so their interactable ancestor can receive the event instead.
            if ( Visibility::IsHitTestable( a_Node->Layout.Visibility ) &&
                 a_Node->Widget != c_InvalidWidgetID )
            {
                IWidget* w = GetWidget( a_Node->Widget );
                if ( w && w->IsInteractable() )
                    return a_Node->Widget;
            }

            return c_InvalidWidgetID;
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
            WidgetID scrollTarget = m_CapturedWidget != c_InvalidWidgetID
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
        if ( m_CapturedWidget != c_InvalidWidgetID )
        {
            if ( IWidget* w = GetWidget( m_CapturedWidget ) )
            {
                Reply reply = w->OnPointerMove( a_Event );
                ApplyReply( reply );
            }
            return false;
        }

        // --- Hover tracking ---
        WidgetID hovered = HitTest( RootWidget, a_Event.Position );

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
        else if ( hovered != c_InvalidWidgetID )
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
        if ( m_CapturedWidget != c_InvalidWidgetID )
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
        WidgetID focusedID = GetFocusedWidget();
        if ( focusedID != m_HoveredWidget )
        {
            if ( IWidget* focused = GetWidget( focusedID ) )
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
