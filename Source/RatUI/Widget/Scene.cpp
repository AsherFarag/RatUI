#include <RatUI/Widget/Scene.h>

#include <algorithm>

namespace RatUI
{
    // TODO: Make this a reusable utility?
    namespace 
    {
        /** @brief The ButtonEvent a widget receives for ENavAction::ActivatePressed/ActivateReleased. */
        ButtonEvent MakeActivateEvent( bool a_Pressed )
        {
            return ButtonEvent{
                .Button   = EButtonID::KeyEnter,
                .Pressed  = a_Pressed,
                .Released = !a_Pressed,
                .Held     = a_Pressed,
            };
        }

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

    void Scene::UpdateLayout( Vec2<Unit> a_AvailableSize )
    {
        CleanupDestroyedWidgets();

        LayoutNode* rootNode = m_Layouts.Get( RootWidget );
        if ( !rootNode )
            return;

        alignas( 16 ) static thread_local byte scratchBuffer[1024 * 1024]; // TODO: Make this configurable
		BumpAllocator allocator{ scratchBuffer, sizeof( scratchBuffer ) };
		const Rect<Unit> rootRect{ Vec2<Unit>{ 0_u, 0_u }, a_AvailableSize };
		LayoutContext layoutCtx{ allocator, m_SafeArea };

        MeasureLayoutNode( *rootNode, a_AvailableSize, layoutCtx );
        const bool needsSecondPass = ArrangeLayoutNode( *rootNode, rootRect, layoutCtx );
        if ( needsSecondPass )
        {
            MeasureLayoutNode( *rootNode, a_AvailableSize, layoutCtx );
            ArrangeLayoutNode( *rootNode, rootRect, layoutCtx );
        }

        // Note: This is hacky but currently it's necessary to do this to avoid invalid input state after a layout change.
		// For example, if a widget is removed while the pointer is over it, we need to clear the hovered state.
		// Or if we click and hold a button, move the pointer off it, this re-process helps clear the pressed state.
		// Otherwise, we would have to wait until the next pointer event to fix the state.
        // Only the mouse is replayed unconditionally. A lifted finger points at nothing (touches have no hover), and a
        // touch or pen that Tick has forgotten must not be brought back, so those replay only while still tracked.
        const PointerEvent& last = Input.LastPointerEvent;
        if ( last.Type != EPointerType::Unknown )
        {
            const PointerGestureState* gesture = FindValue( Input.PointerStates, last.ID );
            const bool replay = last.IsMouse() || ( gesture && ( !last.IsTouch() || gesture->IsDown ) );
            if ( replay )
                ProcessPointerEvent( last );
        }
    }

    void Scene::SetSafeArea( const Edges& a_SafeArea )
    {
        // Measure keys its cache on the safe area reaching each node, so nothing needs marking dirty here.
        m_SafeArea = a_SafeArea;
    }

    void Scene::Tick( f64 a_DeltaSeconds )
    {
        m_ClockSeconds += a_DeltaSeconds;

        const f64 now = m_ClockSeconds;
        const GestureConfig& cfg = Input.Config;

        // Long-press detection: scan pointers currently down, not dragging, not already fired.
        for ( auto& [pointerID, gesture] : Input.PointerStates )
        {
            if ( !gesture.IsDown || gesture.IsDragging || gesture.LongPressFired )
                continue;

            if ( now - gesture.LastDownTime < cfg.LongPressSeconds )
                continue;

            gesture.LongPressFired = true;

            if ( IWidget* widget = GetWidget( gesture.PressedWidget ) )
            {
                LongPressEvent evt{ .Position = gesture.DownPosition, .Modifiers = Input.ModifierState };
                ApplyReply( widget->OnLongPress( evt ), pointerID );
            }
        }

        // Touches and pens get a new ID per contact on some platforms, so a pointer that has lifted and can no longer
        // continue a multi-press is forgotten. The mouse is kept: it is always there.
        Array<PointerID> finished;
        for ( const auto& [pointerID, gesture] : Input.PointerStates )
        {
            if ( pointerID != c_MousePointerID && !gesture.IsDown && gesture.CapturedWidget == c_InvalidNodeID
                 && now - gesture.LastPressTime > cfg.MultiPressWindowSeconds )
                PushBack( finished, pointerID );
        }
        for ( const PointerID pointerID : finished )
            Erase( Input.PointerStates, pointerID );
    }

    void Scene::Render( DrawList& a_DrawList, f32 a_DeltaSeconds )
    {
        CleanupDestroyedWidgets();

        if ( LayoutNode* rootNode = m_Layouts.Get( RootWidget ) )
            if ( rootNode->Widget )
                rootNode->Widget->Paint( PaintEvent{ a_DrawList, a_DeltaSeconds } );
    }

    // ========================================================================
    // Widget Lookup
    // ========================================================================

    IWidget* Scene::GetWidget( NodeID a_ID )
    {
        LayoutNode* node = m_Layouts.Get( a_ID );
        return ( node && node->Widget ) ? node->Widget.get() : nullptr;
    }

    const IWidget* Scene::GetWidget( NodeID a_ID ) const
    {
        const LayoutNode* node = m_Layouts.Get( a_ID );
        return ( node && node->Widget ) ? node->Widget.get() : nullptr;
    }

    // ========================================================================
    // Reply Application
    // ========================================================================

    void Scene::ApplyReply( const Reply& a_Reply, PointerID a_Pointer )
    {
        if ( a_Reply.ShouldCapturePointer() )
            CapturePointer( a_Pointer, a_Reply.GetPointerCaptureTarget() );

        if ( a_Reply.ShouldReleasePointer() )
            ReleasePointerCapture( a_Pointer );

        if ( a_Reply.ShouldSetFocus() )
            SetFocus( a_Reply.GetFocusTarget() );

        if ( a_Reply.ShouldClearFocus() )
            SetFocus( c_InvalidNodeID );
    }

    // ========================================================================
    // Focus
    // ========================================================================

    void Scene::SetFocus( NodeID a_Node )
    {
        if ( Input.FocusedWidget == a_Node )
            return;

        // Reject focusing a non-existent or non-focusable widget; c_InvalidNodeID (clear) is always allowed.
        if ( a_Node != c_InvalidNodeID )
        {
            IWidget* target = GetWidget( a_Node );
            if ( !target || !target->IsFocusable() )
                return;
        }

        if ( IWidget* prev = GetWidget( Input.FocusedWidget ) )
            prev->OnFocusLost( FocusEvent{} );

        Input.FocusedWidget = a_Node;
        SyncNavScopes( a_Node );

        if ( IWidget* next = GetWidget( a_Node ) )
            next->OnFocusReceived( FocusEvent{} );
    }

    void Scene::SyncNavScopes( NodeID a_Focused )
    {
        LayoutNode* node = m_Layouts.Get( a_Focused );
        if ( !node )
            return; // Clearing focus leaves the scopes as they are.

        // Ancestors of the focused widget, outermost first.
        Array<LayoutNode*> path;
        for ( LayoutNode* parent = node->Parent(); parent; parent = parent->Parent() )
            PushBack( path, parent );
        std::reverse( path.begin(), path.end() );

        const auto IsOnPath = [&]( NodeID a_ID )
        {
            return std::any_of( path.begin(), path.end(), [&]( const LayoutNode* a_Node ) { return a_Node->ID == a_ID; } );
        };

        // Keep the scopes that still contain the widget (this also keeps scopes pushed by hand,
        // e.g. a dialog with a custom restore target); drop the rest.
        size keep = 0;
        while ( keep < Size( Input.NavStack ) && IsOnPath( Input.NavStack[keep].Scope ) )
            ++keep;
        while ( Size( Input.NavStack ) > keep )
            PopBack( Input.NavStack );

        // Push the focus scopes between the innermost kept scope (or the root) and the widget.
        // Each one restores to itself on pop, as if it had been entered with Activate.
        const NodeID innermost = keep > 0 ? Input.NavStack[keep - 1].Scope : RootWidget;
        bool         below     = false;
        for ( LayoutNode* scope : path )
        {
            if ( below && scope->Style.IsFocusScope )
                Input.PushNavScope( scope->ID, scope->ID );
            if ( scope->ID == innermost )
                below = true;
        }
    }

    void Scene::EnsureInitialFocus()
    {
        if ( Input.FocusedWidget != c_InvalidNodeID )
            return;

        // Start in the current nav scope (e.g. the group of a text field that Escape just left),
        // and only fall back to the whole scene if that scope has nothing focusable.
        Array<const LayoutNode*> candidates;
        if ( LayoutNode* scope = m_Layouts.Get( GetCurrentNavScope() ) )
            CollectFocusableCandidates( *scope, candidates );

        if ( Empty( candidates ) )
        {
            if ( LayoutNode* root = m_Layouts.Get( RootWidget ) )
                CollectFocusableCandidates( *root, candidates );
        }

        if ( !Empty( candidates ) )
            SetFocus( candidates[0]->Widget->GetLayoutID() );
    }

    // ========================================================================
    // Hit Testing
    // ========================================================================

    NodeID Scene::HitTest( NodeID a_ID, Vec2<Unit> a_LogicalPos )
    {
        LayoutNode* node = m_Layouts.Get( a_ID );
        if ( !node )
            return c_InvalidNodeID;

        if ( !Visibility::IsHitTestable( node->Layout.Visibility ) &&
             !Visibility::AreChildrenHitTestable( node->Layout.Visibility ) )
            return c_InvalidNodeID;

        if ( !node->Layout.FinalRect.Contains( a_LogicalPos ) )
            return c_InvalidNodeID;

        // Children may be visually offset from their FinalRect translate 
        // the test position into their local space before recursing.
        const Vec2<Unit> childPos = a_LogicalPos + node->ChildHitTestOffset;

        NodeID childHit = c_InvalidNodeID;
        node->ForEachChildReverse( [&]( LayoutNode& child )
        {
            if ( childHit != c_InvalidNodeID )
                return; // TODO: Need a way to break out of ForEachChildReverse early, or use a different iteration method.

            childHit = HitTest( child.ID, childPos );
        } );

        if ( childHit != c_InvalidNodeID )
            return childHit;

        if ( node->Widget && node->Widget->IsInteractable() )
            return a_ID;

        return c_InvalidNodeID;
    }

    // ========================================================================
    // Input Dispatch
    // ========================================================================

    bool Scene::DispatchInputEvent( const InputEvent& a_Event )
    {
		if ( Holds<PointerEvent>( a_Event.Payload ) )
			return ProcessPointerEvent( Get<PointerEvent>( a_Event.Payload ) );

		if ( Holds<ButtonEvent>( a_Event.Payload ) )
			return ProcessButtonEvent( Get<ButtonEvent>( a_Event.Payload ) );

		if ( Holds<TextInputEvent>( a_Event.Payload ) )
			return ProcessTextInputEvent( Get<TextInputEvent>( a_Event.Payload ) );

		return false;
    }

    bool Scene::WantsTextInput() const
    {
        const IWidget* focused = GetWidget( Input.FocusedWidget );
        return focused && focused->WantsTextInput();
    }

    void Scene::ReleaseAllKeys()
    {
        Input.KeyPresses.clear();
        Input.ModifierState     = EModifier::None;
        Input.DropNextTextInput = false;
    }

    bool Scene::ProcessTextInputEvent( const TextInputEvent& a_Event )
    {
        // Text produced by a key that navigation used (e.g. Space activating a group, which may
        // focus a text field before the " " arrives) is not typed.
        if ( Input.DropNextTextInput )
        {
            Input.DropNextTextInput = false;
            return true;
        }

        IWidget* focused = GetWidget( Input.FocusedWidget );
        if ( !focused )
            return false;

        const Reply reply = focused->OnTextInput( a_Event );
        ApplyReply( reply );
        return reply.IsHandled();
    }

    bool Scene::ProcessPointerEvent( const PointerEvent& a_Event )
    {
        Input.LastPointerEvent = a_Event;

        if ( a_Event.IsMouse() || Math::LengthSq( a_Event.Delta ) > 0_u )
            Input.InputMode = EInputMode::Pointer;

        PointerGestureState& gesture = Input.PointerStates[a_Event.ID];
        const NodeID captured = gesture.CapturedWidget;
        const NodeID target = captured != c_InvalidNodeID ? captured : HitTest( RootWidget, a_Event.Position );

        // --- Hover transitions ---
        // Touches have no hover: a finger only points while it presses, and hover is shared by every pointer.
        if ( a_Event.Type != EPointerType::Touch && captured == c_InvalidNodeID && target != Input.HoveredWidget )
        {
            if ( IWidget* prev = GetWidget( Input.HoveredWidget ) )
                ApplyReply( prev->OnPointerExit( a_Event ), a_Event.ID );

            Input.HoveredWidget = target;

            if ( IWidget* next = GetWidget( target ) )
                ApplyReply( next->OnPointerEnter( a_Event ), a_Event.ID );
        }

        bool handled = false;

        // --- Drag detection & movement ---
        const GestureConfig& cfg = Input.Config;

        if ( gesture.IsDown && !gesture.IsDragging )
        {
            if ( Math::DistanceSq( a_Event.Position, gesture.DownPosition ) > Math::Sq( cfg.DragThreshold ) )
            {
                gesture.IsDragging = true;

                if ( IWidget* widget = GetWidget( gesture.PressedWidget ) )
                {
                    DragEvent drag{
                        .Origin = gesture.DownPosition,
                        .Current = a_Event.Position,
                        .Delta = a_Event.Position - gesture.DownPosition,  // Initial delta (threshold exceeded)
                        .Modifiers = a_Event.Modifiers
                    };
                    ApplyReply( widget->OnDragStart( drag ), a_Event.ID );
                }
            }
        }

        // --- Drag move ---
        if ( gesture.IsDragging )
        {
            if ( IWidget* widget = GetWidget( gesture.PressedWidget ) )
            {
                DragEvent drag{
                    .Origin = gesture.DownPosition,
                    .Current = a_Event.Position,
                    .Delta = a_Event.Delta,  // Per-frame delta
                    .Modifiers = a_Event.Modifiers
                };
                ApplyReply( widget->OnDragMove( drag ), a_Event.ID );
            }
        }

        // --- Raw pointer dispatch (move / scroll) ---
        if ( IWidget* widget = GetWidget( target ) )
        {
            if ( a_Event.ScrollDelta[0] != 0_u || a_Event.ScrollDelta[1] != 0_u )
                ApplyReply( widget->OnPointerScroll( a_Event ), a_Event.ID );

            Reply moveReply = widget->OnPointerMove( a_Event );
            ApplyReply( moveReply, a_Event.ID );
            handled = handled || moveReply.IsHandled();
        }

        return handled;
    }

    bool Scene::ProcessButtonEvent( const ButtonEvent& a_Event )
    {
        if ( a_Event.Pressed || a_Event.Released )
            Input.UpdateModifiers( a_Event.Button, a_Event.Pressed );

        const bool isKey = !IsPointerButton( a_Event.Button );

        // A key's text arrives between its press and release, so a later key event ends the window
        // in which text is dropped (see ProcessTextInputEvent).
        if ( isKey )
            Input.DropNextTextInput = false;

        // --- Key release: goes to the widget that took the press, even if focus moved since ---
        if ( isKey && a_Event.Released )
        {
            if ( auto it = Input.KeyPresses.find( a_Event.Button ); it != Input.KeyPresses.end() )
            {
                const KeyPressTarget press = it->second;
                Input.KeyPresses.erase( it );

                if ( IWidget* target = GetWidget( press.Widget ) )
                    ApplyReply( target->OnButtonReleased( press.Activate ? MakeActivateEvent( false ) : a_Event ) );
                return true;
            }
        }

        // The focused widget, if it was already offered this key below; it isn't offered it twice.
        NodeID alreadyOffered = c_InvalidNodeID;

        // --- Text input gets key presses before navigation ---
        if ( isKey && a_Event.Pressed && WantsTextInput() )
        {
            const NodeID focusedID = Input.FocusedWidget;
            if ( IWidget* focused = GetWidget( focusedID ) )
            {
                const Reply reply = focused->OnButtonPressed( a_Event );
                ApplyReply( reply );
                if ( reply.IsHandled() )
                {
                    Input.KeyPresses[a_Event.Button] = { .Widget = focusedID };
                    return true;
                }
                alreadyOffered = focusedID;
            }
        }

		// --- Navigation mapping ---
        const ENavAction navAction = Input.NavMap.Resolve( a_Event.Button );
        if ( navAction != ENavAction::None )
        {
            Input.InputMode = EInputMode::Navigation;
            if ( a_Event.Pressed )
            {
                Input.DropNextTextInput = isKey;
                EnsureInitialFocus(); // Not on release, or releasing Escape would re-focus what it just unfocused.
            }

            if ( navAction == ENavAction::ActivatePressed || navAction == ENavAction::ActivateReleased )
            {
                // Releases of presses a widget took were routed above. An unmatched release had no
                // press target, so it is dropped rather than activating whatever is focused now.
                // Holding the key down activates once, not once per key repeat.
                if ( a_Event.Pressed && !a_Event.Repeat )
                {
                    const NodeID      target     = Input.FocusedWidget;
                    const LayoutNode* targetNode = m_Layouts.Get( target );
                    const bool        getsPress  = targetNode && targetNode->Widget && !targetNode->Style.IsFocusScope;

                    Navigate( ENavAction::ActivatePressed );
                    if ( getsPress )
                        Input.KeyPresses[a_Event.Button] = { .Widget = target, .Activate = true };
                }
            }
            else if ( a_Event.Pressed )
            {
                Navigate( navAction );
            }

            return true; // Nav mappings always consume the button; it doesn't reach the focused widget raw.
        }

        // --- Gesture from button press/release ---
        NodeID targetWidget = Input.FocusedWidget;

        const PointerID pointer = a_Event.Pointer.value_or( c_MousePointerID );
        if ( a_Event.PointerPosition && IsPointerButton( a_Event.Button ) )
        {
            const NodeID capturedBefore = GetCapturedWidget( pointer );

            UpdateGestureState( a_Event );

            // Pointer buttons should dispatch to the captured widget (if any) so release stays with the press target.
            targetWidget = capturedBefore != c_InvalidNodeID
                ? capturedBefore
                : HitTest( RootWidget, *a_Event.PointerPosition );
        }

		// --- Dispatch to focused widget ---
        if ( isKey && targetWidget == alreadyOffered && alreadyOffered != c_InvalidNodeID )
            return false;

        if ( IWidget* focused = GetWidget( targetWidget ) )
        {
            Reply reply = Reply::Unhandled();
            if ( a_Event.Pressed )
                reply = focused->OnButtonPressed( a_Event );
			else if ( a_Event.Released )
				reply = focused->OnButtonReleased( a_Event );

            ApplyReply( reply, pointer );
            if ( isKey && a_Event.Pressed && reply.IsHandled() )
                Input.KeyPresses[a_Event.Button] = { .Widget = targetWidget };
            return reply.IsHandled();
        }

        return false;
    }

    void Scene::UpdateGestureState( const ButtonEvent& a_Event )
    {
        if ( !a_Event.Pointer )
            return;

        PointerGestureState& gesture = Input.PointerStates[*a_Event.Pointer];
        const GestureConfig& cfg = Input.Config;
        const f64 now = m_ClockSeconds;

        if ( a_Event.Pressed )
        {
            const NodeID target = HitTest( RootWidget, *a_Event.PointerPosition );

            const bool withinWindow = ( now - gesture.LastPressTime ) <= cfg.MultiPressWindowSeconds;
            const bool withinRange = a_Event.PointerPosition
                ? Math::DistanceSq( *a_Event.PointerPosition, gesture.DownPosition ) <= Math::Sq( cfg.MultiPressDistance )
                : false;

            if ( !withinWindow || !withinRange )
                gesture.PressCount = 0;

            gesture.PressCount += 1;
            gesture.DownPosition = *a_Event.PointerPosition;
            gesture.LastDownTime = now;
            gesture.PressedWidget = target;
            gesture.IsDragging = false;
            gesture.LongPressFired = false;
            gesture.IsDown = true;

            // Capture pointer on press so drag/release stay on this widget
            if ( IWidget* widget = GetWidget( target ) )
            {
                // Widget can override capture in OnButtonPressed, but we default-capture
                // for interactable widgets to ensure drag/click completion
                if ( widget->IsInteractable() )
                    gesture.CapturedWidget = target;
            }
        }
        else if ( a_Event.Released && gesture.IsDown )
        {
            // --- Drag end ---
            if ( gesture.IsDragging )
            {
                if ( IWidget* widget = GetWidget( gesture.PressedWidget ) )
                {
                    DragEvent drag{
                        .Origin = gesture.DownPosition,
                        .Current = *a_Event.PointerPosition,
                        .Delta = *a_Event.PointerPosition - gesture.DownPosition,  // Total delta
                        .Modifiers = a_Event.Modifiers
                    };
                    ApplyReply( widget->OnDragEnd( drag ), *a_Event.Pointer );
                }
            }
            // --- Click (single, double, triple, etc.) ---
            else if ( !gesture.LongPressFired )
            {
                if ( IWidget* widget = GetWidget( gesture.PressedWidget ) )
                {
                    PressEvent press{
                        .Position = *a_Event.PointerPosition,
                        .PressCount = gesture.PressCount,
                        .Modifiers = a_Event.Modifiers
                    };
                    Reply reply = widget->OnPress( press );
                    ApplyReply( reply, *a_Event.Pointer );

                    if ( widget->IsFocusable() )
                        SetFocus( gesture.PressedWidget );
                }
            }

            gesture.LastPressTime = now;
            gesture.IsDown = false;
            gesture.IsDragging = false;

            // Release this pointer's capture on button up; other pointers keep theirs
            gesture.CapturedWidget = c_InvalidNodeID;
        }
    }

    // ========================================================================
    // Navigation
    // ========================================================================

    void Scene::PushNavScope( NodeID a_ScopeID )
    {
        Input.PushNavScope( a_ScopeID, Input.FocusedWidget );
    }

    void Scene::PopNavScope()
    {
        if ( Optional<NavScope> popped = Input.PopNavScope() )
        {
            if ( popped->Restored != c_InvalidNodeID )
                SetFocus( popped->Restored );
        }
    }

    NavReply Scene::QueryBoundaryReply( ENavAction a_Action, NodeID a_Focused )
    {
        LayoutNode* node = m_Layouts.Get( a_Focused );
        while ( node )
        {
            if ( node->Widget && node->Widget->IsNavigationBoundary() )
                return node->Widget->OnNavigationBoundary( NavEvent{ .Action = a_Action } );

            node = node->Parent();
        }
        return NavReply::Escape();
    }

    void Scene::CollectFocusableCandidates( LayoutNode& a_Scope, Array<const LayoutNode*>& a_Out )
    {
        a_Scope.ForEachChild( [&]( LayoutNode& child )
        {
            if ( child.Widget && child.Widget->IsFocusable() )
                PushBack( a_Out, &child );

            // Don't descend into a nested boundary's subtree -- that's a separate scope,
            // only entered by explicitly pushing it.
            if ( !child.Widget || !child.Widget->IsNavigationBoundary() )
                CollectFocusableCandidates( child, a_Out );
        } );
    }

    void Scene::Navigate( ENavAction a_Action )
    {
        const auto FocusFirstIn = [&]( NodeID a_ScopeID )
        {
            LayoutNode* scopeNode = m_Layouts.Get( a_ScopeID );
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
            LayoutNode* focusedNode = m_Layouts.Get( Input.FocusedWidget );
            if ( !focusedNode || !focusedNode->Widget )
                return;

            if ( a_Action == ENavAction::ActivatePressed && focusedNode->Style.IsFocusScope )
            {
                PushNavScope( Input.FocusedWidget );
                FocusFirstIn( Input.FocusedWidget );
                return;
            }

            const ButtonEvent ev = MakeActivateEvent( a_Action == ENavAction::ActivatePressed );

            // TODO: Shouldnt be making fake events like this, should add something like IWidget::OnPressed/OnHovered etc??
            Reply reply = a_Action == ENavAction::ActivatePressed
                ? focusedNode->Widget->OnButtonPressed( ev )
                : focusedNode->Widget->OnButtonReleased( ev );

            ApplyReply( reply );
            return;
        }

        NodeID      scopeID = GetCurrentNavScope();
        LayoutNode* scopeNode = m_Layouts.Get( scopeID );
        LayoutNode* focusedNode = m_Layouts.Get( Input.FocusedWidget );

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
        const NavReply navReply = QueryBoundaryReply( a_Action, Input.FocusedWidget );

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
                NodeID target = navReply.ResolveCustom( a_Action, Input.FocusedWidget );
                if ( target != c_InvalidNodeID )
                    SetFocus( target );
                break;
            }
        }
    }

    // ========================================================================
    // Reset
    // ========================================================================

    void Scene::Reset()
    {
        m_Layouts.Clear();
        RootWidget = c_InvalidNodeID;
        Input.Reset();
        Clear( m_ToDestroy );
    }

    LayoutNode& Scene::CreateLayoutNode( LayoutStyle a_Style, NodeID a_ParentID )
    {
        NodeID nodeID = m_Layouts.Allocate( a_Style );
        LayoutNode* node = m_Layouts.Get( nodeID );
        RATUI_ASSERT( node, "Failed to allocate layout node." );

        if ( a_ParentID != c_InvalidNodeID )
        {
            if ( LayoutNode* parentNode = m_Layouts.Get( a_ParentID ) )
                parentNode->PushBackChild( *node );
        }

        return *node;
    }

} // namespace RatUI
