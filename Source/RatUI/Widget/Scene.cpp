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
		Array<LayoutNode*> clampedScrolls;
		LayoutContext layoutCtx{ allocator, m_SafeArea, &clampedScrolls };

        // Arrange can ask for another pass: wrapped text changing height, or an Auto scrollbar appearing, which can
        // make the other axis overflow too. Bounded, as content that never settles would loop forever.
        constexpr u32 c_MaxLayoutPasses = 4;
        for ( u32 pass = 0; pass < c_MaxLayoutPasses; ++pass )
        {
            MeasureLayoutNode( *rootNode, a_AvailableSize, layoutCtx );
            if ( !ArrangeLayoutNode( *rootNode, rootRect, layoutCtx ) )
                break;
        }

        // A scroll request is used once: content that grows later doesn't pull the view back to it.
        for ( LayoutNode* node : clampedScrolls )
            node->ScrollOffset = node->Layout.ScrollOffset;

        RefreshHover();

        // Last, as these may change the scene. Changes to layout show at the next UpdateLayout.
        Array<NodeID> afterLayout;
        std::swap( afterLayout, m_AfterLayout );
        for ( size i = 0; i < Size( afterLayout ); ++i )
        {
            const NodeID id = afterLayout[i];
            if ( std::find( afterLayout.begin(), afterLayout.begin() + i, id ) != afterLayout.begin() + i )
                continue;
            if ( IWidget* widget = GetWidget( id ) )
                widget->OnLayoutDone();
        }
    }

    void Scene::NotifyAfterLayout( NodeID a_Node )
    {
        PushBack( m_AfterLayout, a_Node );
    }

    void Scene::RefreshHover()
    {
        // Touches have no hover, and a pen that Tick has forgotten no longer points anywhere.
        const PointerEvent& last = Input.LastPointerEvent;
        if ( last.Type == EPointerType::Unknown || last.IsTouch() )
            return;

        const PointerGestureState* gesture = FindValue( Input.PointerStates, last.ID );
        if ( !last.IsMouse() && !gesture )
            return;

        // A captured pointer stays with the widget that captured it.
        if ( gesture && gesture->CapturedWidget != c_InvalidNodeID )
            return;

        // The pointer is where it was: only what is under it changed.
        PointerEvent still = last;
        still.Delta        = { 0_u, 0_u };
        still.ScrollDelta  = { 0_u, 0_u };
        SetHovered( HitTest( RootWidget, still.Position ), still );
    }

    bool Scene::DispatchWheel( const PointerEvent& a_Event, NodeID a_Captured )
    {
        WheelLatch&          latch = Input.Wheel;
        const GestureConfig& cfg   = Input.Config;

        // Moving the pointer or pausing ends the gesture, and a widget holding the pointer takes its wheel.
        const bool latched = a_Captured == c_InvalidNodeID && latch.Widget != c_InvalidNodeID && latch.Pointer == a_Event.ID &&
                             m_ClockSeconds - latch.LastTime <= cfg.WheelLatchSeconds &&
                             Math::DistanceSq( a_Event.Position, latch.Position ) <= Math::Sq( cfg.WheelLatchDistance );
        if ( latched )
        {
            if ( IWidget* widget = GetWidget( latch.Widget ) )
            {
                latch.LastTime = m_ClockSeconds;
                ApplyReply( widget->OnPointerScroll( a_Event ), a_Event.ID );
                return true; // Still the latched widget's at its end: the rest of the gesture doesn't pass to the widgets around it.
            }
        }
        latch = {};

        // Bubbles up, so a scroll container scrolls with the pointer over any of its content.
        // From the deepest node, not just interactable ones, so the wheel works over a scroll container's empty space too.
        const NodeID from = a_Captured != c_InvalidNodeID ? a_Captured : HitTestDeepest( RootWidget, a_Event.Position );
        for ( LayoutNode* node = m_Layouts.Get( from ); node; node = node->Parent() )
        {
            if ( !node->Widget )
                continue;

            const Reply reply = node->Widget->OnPointerScroll( a_Event );
            ApplyReply( reply, a_Event.ID );
            if ( reply.IsHandled() )
            {
                latch = WheelLatch{ .Widget = node->ID, .Pointer = a_Event.ID, .Position = a_Event.Position, .LastTime = m_ClockSeconds };
                return true;
            }
        }
        return false;
    }

    void Scene::SetHovered( NodeID a_Target, const PointerEvent& a_Event )
    {
        if ( a_Target == Input.HoveredWidget )
            return;

        if ( IWidget* prev = GetWidget( Input.HoveredWidget ) )
            ApplyReply( prev->OnPointerExit( a_Event ), a_Event.ID );

        Input.HoveredWidget = a_Target;

        if ( IWidget* next = GetWidget( a_Target ) )
            ApplyReply( next->OnPointerEnter( a_Event ), a_Event.ID );
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

        if ( a_Reply.ShouldDisengage() )
            Disengage();

        if ( a_Reply.ShouldSetFocus() )
            SetFocus( a_Reply.GetFocusTarget() );

        if ( a_Reply.ShouldClearFocus() )
            SetFocus( c_InvalidNodeID );

        if ( a_Reply.ShouldEngage() )
            Engage( a_Reply.GetFocusTarget() );
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

        // An engaged widget stays engaged only while it has focus.
        Disengage();

        if ( IWidget* prev = GetWidget( Input.FocusedWidget ) )
            prev->OnFocusLost( FocusEvent{} );

        Input.FocusedWidget = a_Node;
        SyncNavScopes( a_Node );

        // A pointer focuses what it can already see; keyboard or gamepad focus may land out of view.
        if ( Input.InputMode == EInputMode::Navigation )
            ScrollIntoView( a_Node );

        if ( IWidget* next = GetWidget( a_Node ) )
            next->OnFocusReceived( FocusEvent{} );
    }

    void Scene::Engage( NodeID a_Node )
    {
        IWidget* widget = GetWidget( a_Node );
        if ( !widget || !widget->CanEngage() )
            return;

        SetFocus( a_Node );
        if ( Input.FocusedWidget != a_Node || Input.EngagedWidget == a_Node )
            return;

        Input.EngagedWidget = a_Node;
        widget->OnEngage();
    }

    void Scene::Disengage()
    {
        const NodeID engaged = Input.EngagedWidget;
        if ( engaged == c_InvalidNodeID )
            return;

        Input.EngagedWidget = c_InvalidNodeID;
        if ( IWidget* widget = GetWidget( engaged ) )
            widget->OnDisengage();
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
            if ( below && scope->Widget && scope->Widget->GetNavRole() == ENavRole::Group )
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
            CollectNavTargets( *scope, candidates );

        if ( Empty( candidates ) )
        {
            if ( LayoutNode* root = m_Layouts.Get( RootWidget ) )
                CollectNavTargets( *root, candidates );
        }

        if ( !Empty( candidates ) )
            SetFocus( candidates[0]->ID );
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

        NodeID childHit = c_InvalidNodeID;
        node->ForEachChildReverse( [&]( LayoutNode& child )
        {
            if ( childHit != c_InvalidNodeID )
                return; // TODO: Need a way to break out of ForEachChildReverse early, or use a different iteration method.

            childHit = HitTest( child.ID, a_LogicalPos );
        } );

        if ( childHit != c_InvalidNodeID )
            return childHit;

        if ( node->Widget && node->Widget->IsInteractable() )
            return a_ID;

        return c_InvalidNodeID;
    }

    NodeID Scene::HitTestDeepest( NodeID a_ID, Vec2<Unit> a_LogicalPos )
    {
        LayoutNode* node = m_Layouts.Get( a_ID );
        if ( !node )
            return c_InvalidNodeID;

        if ( !Visibility::IsHitTestable( node->Layout.Visibility ) &&
             !Visibility::AreChildrenHitTestable( node->Layout.Visibility ) )
            return c_InvalidNodeID;

        if ( !node->Layout.FinalRect.Contains( a_LogicalPos ) )
            return c_InvalidNodeID;

        for ( LayoutNode* child = node->LastChild(); child; child = child->PrevSibling() )
        {
            if ( const NodeID hit = HitTestDeepest( child->ID, a_LogicalPos ); hit != c_InvalidNodeID )
                return hit;
        }

        return a_ID;
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
        if ( a_Event.Type != EPointerType::Touch && captured == c_InvalidNodeID )
            SetHovered( target, a_Event );

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
        if ( a_Event.ScrollDelta[0] != 0_u || a_Event.ScrollDelta[1] != 0_u )
            handled = DispatchWheel( a_Event, captured );

        if ( IWidget* widget = GetWidget( target ) )
        {
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

        // Resolved up front, though the key reaches the widgets before navigation.
        ENavAction navAction = Input.NavMap.Resolve( a_Event.Button );
        if ( navAction == ENavAction::Next && HasAnyFlag( a_Event.Modifiers, EModifier::Shift ) )
            navAction = ENavAction::Previous;
        if ( navAction != ENavAction::None )
            Input.InputMode = EInputMode::Navigation;

        // --- Key presses: the focused widget, then its ancestors, then navigation ---
        if ( isKey && a_Event.Pressed )
        {
            for ( LayoutNode* node = m_Layouts.Get( Input.FocusedWidget ); node; node = node->Parent() )
            {
                if ( !node->Widget )
                    continue;

                const NodeID handler = node->ID;
                const Reply  reply   = node->Widget->OnButtonPressed( a_Event );
                ApplyReply( reply );
                if ( reply.IsHandled() )
                {
                    Input.KeyPresses[a_Event.Button] = { .Widget = handler };
                    return true;
                }
            }
        }

        // --- Navigation ---
        if ( navAction != ENavAction::None )
        {
            bool pickedStart = false;
            if ( a_Event.Pressed )
            {
                Input.DropNextTextInput = isKey;
                const bool hadFocus = Input.FocusedWidget != c_InvalidNodeID;
                EnsureInitialFocus(); // Not on release, or releasing Escape would re-focus what it just unfocused.
                pickedStart = !hadFocus && Input.FocusedWidget != c_InvalidNodeID;
            }

            // An engaged widget left this key unhandled: arrows stay with it, the rest disengage it.
            if ( a_Event.Pressed && IsEngaged( Input.FocusedWidget ) )
            {
                switch ( navAction )
                {
                    case ENavAction::MoveLeft:
                    case ENavAction::MoveRight:
                    case ENavAction::MoveUp:
                    case ENavAction::MoveDown:
                        break;

                    case ENavAction::Next:
                    case ENavAction::Previous:
                        Disengage();
                        Navigate( navAction );
                        break;

                    default:
                        // Holding Activate down engages once; its key repeats must not disengage again.
                        if ( !a_Event.Repeat )
                            Disengage();
                        break;
                }
                return true;
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
                    const bool        getsPress  = targetNode && targetNode->Widget &&
                                                   targetNode->Widget->GetNavRole() != ENavRole::Group &&
                                                   !targetNode->Widget->CanEngage();

                    Navigate( ENavAction::ActivatePressed );
                    if ( getsPress )
                        Input.KeyPresses[a_Event.Button] = { .Widget = target, .Activate = true };
                }
            }
            else if ( a_Event.Pressed )
            {
                // With nothing focused, the first move or Tab only picks the starting widget. Shift+Tab still
                // moves on from it, wrapping to the last widget.
                const bool movesOn = navAction != ENavAction::Previous && navAction != ENavAction::Cancel;
                if ( !( pickedStart && movesOn ) )
                    Navigate( navAction );
            }

            return true;
        }

        // Unhandled key presses were offered to every widget that could take them above.
        if ( isKey && a_Event.Pressed )
            return false;

        // --- Pointer buttons go to the captured widget or the one under the pointer; other releases to the focused widget ---
        NodeID targetWidget = Input.FocusedWidget;

        const PointerID pointer = a_Event.Pointer.value_or( c_MousePointerID );
        if ( a_Event.PointerPosition && IsPointerButton( a_Event.Button ) )
        {
            const NodeID capturedBefore = GetCapturedWidget( pointer );
            Input.InputMode = EInputMode::Pointer;

            UpdateGestureState( a_Event );

            // Pointer buttons should dispatch to the captured widget (if any) so release stays with the press target.
            targetWidget = capturedBefore != c_InvalidNodeID
                ? capturedBefore
                : HitTest( RootWidget, *a_Event.PointerPosition );
        }

        if ( IWidget* widget = GetWidget( targetWidget ) )
        {
            Reply reply = Reply::Unhandled();
            if ( a_Event.Pressed )
                reply = widget->OnButtonPressed( a_Event );
            else if ( a_Event.Released )
                reply = widget->OnButtonReleased( a_Event );

            ApplyReply( reply, pointer );
            return reply.IsHandled();
        }

        return false;
    }

    void Scene::UpdateGestureState( const ButtonEvent& a_Event )
    {
        if ( !a_Event.PointerPosition )
            return;

        // An event without a pointer ID is the mouse's, as in ProcessButtonEvent.
        const PointerID pointer = a_Event.Pointer.value_or( c_MousePointerID );
        PointerGestureState& gesture = Input.PointerStates[pointer];
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

                // Focus moves on press, before the widget sees it, so a drag or a long press focuses too.
                // Pressing something that can't take focus (e.g. a scrollbar) leaves focus where it is.
                if ( widget->IsFocusable() )
                    SetFocus( target );
            }
            else
            {
                // Pressing empty space (or a non-interactable panel) unfocuses, e.g. leaves a text field.
                SetFocus( c_InvalidNodeID );
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
                    ApplyReply( widget->OnDragEnd( drag ), pointer );
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
                    ApplyReply( reply, pointer );
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

    NavReply Scene::QueryBoundaryReply( ENavAction a_Action )
    {
        // The scope navigation is in decides: an entered group, or a widget pushed with PushNavScope (e.g. a modal).
        if ( IWidget* scope = GetWidget( GetCurrentNavScope() ) )
            return scope->OnNavigationBoundary( NavEvent{ .Action = a_Action } );
        return NavReply::Escape();
    }

    void Scene::CollectNavTargets( LayoutNode& a_Scope, Array<const LayoutNode*>& a_Out )
    {
        a_Scope.ForEachChild( [&]( LayoutNode& child )
        {
            if ( !Visibility::IsRendered( child.Layout.Visibility ) )
                return;

            const ENavRole role = child.Widget ? child.Widget->GetNavRole() : ENavRole::None;
            if ( role != ENavRole::None )
                PushBack( a_Out, &child );

            // A group's contents are reached by entering it.
            if ( role != ENavRole::Group )
                CollectNavTargets( child, a_Out );
        } );
    }

    void Scene::ScrollIntoView( NodeID a_Node )
    {
        const LayoutNode* node = m_Layouts.Get( a_Node );
        if ( !node )
            return;

        // Innermost first: scrolling a container moves the node, so each outer one sees where it ended up.
        for ( LayoutNode* scroller = node->Parent(); scroller; scroller = scroller->Parent() )
        {
            if ( scroller->Style.Overflow != EOverflow::Scroll )
                continue;

            const Rect<Unit>& viewport = scroller->Layout.FinalRect;
            const Rect<Unit>& rect     = node->Layout.FinalRect;
            Vec2<Unit>        offset   = scroller->Layout.ScrollOffset;
            for ( u32 axis = 0; axis < 2; ++axis )
            {
                const Unit before = rect.Origin[axis] - viewport.Origin[axis];
                const Unit after  = before + rect.Size[axis] - viewport.Size[axis];
                if ( before < 0_u )
                    offset[axis] = offset[axis] + before;
                else if ( after > 0_u )
                    offset[axis] = offset[axis] + std::min( after, before ); // Taller than the viewport: show its start.
            }
            ScrollLayoutNode( *scroller, offset );
        }
    }

    void Scene::Navigate( ENavAction a_Action )
    {
        const auto FocusFirstIn = [&]( NodeID a_ScopeID )
        {
            LayoutNode* scopeNode = m_Layouts.Get( a_ScopeID );
            if ( !scopeNode ) return;

            Array<const LayoutNode*> targets;
            CollectNavTargets( *scopeNode, targets );
            if ( !Empty( targets ) )
                SetFocus( targets[0]->ID );
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

            if ( a_Action == ENavAction::ActivatePressed && focusedNode->Widget->GetNavRole() == ENavRole::Group )
            {
                PushNavScope( Input.FocusedWidget );
                FocusFirstIn( Input.FocusedWidget );
                return;
            }

            if ( a_Action == ENavAction::ActivatePressed && focusedNode->Widget->CanEngage() )
            {
                Engage( Input.FocusedWidget );
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

        Array<const LayoutNode*> targets;
        CollectNavTargets( *scopeNode, targets );

        // Tab order is layout order, wrapping at either end of the scope.
        if ( a_Action == ENavAction::Next || a_Action == ENavAction::Previous )
        {
            if ( Empty( targets ) )
                return;

            // The focused widget, or the target it sits inside (e.g. a group it was clicked into).
            size current = Size( targets );
            for ( size i = 0; i < Size( targets ) && current == Size( targets ); ++i )
            {
                for ( const LayoutNode* node = focusedNode; node; node = node->Parent() )
                {
                    if ( node == targets[i] )
                    {
                        current = i;
                        break;
                    }
                }
            }

            const size count = Size( targets );
            size next;
            if ( current == count )
                next = a_Action == ENavAction::Next ? 0 : count - 1;
            else
                next = a_Action == ENavAction::Next ? ( current + 1 ) % count : ( current + count - 1 ) % count;

            SetFocus( targets[next]->ID );
            return;
        }

        const LayoutNode* nextNode = FindNavigatableNode( a_Action, focusedNode, targets );

        if ( nextNode )
        {
            SetFocus( nextNode->ID );
            return;
        }

        // No target found within the current scope - consult boundary policy.
        const NavReply navReply = QueryBoundaryReply( a_Action );

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
