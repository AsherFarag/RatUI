#include <RatUI/Widget/Scene.h>

#include <algorithm>

namespace RatUI
{
    namespace
    {
        NodeID HitTestNode( const LayoutNode& a_Node, Vec2<Unit> a_Position, bool a_InteractableOnly )
        {
            if ( !a_Node.Layout.HitTestable )
                return c_InvalidNodeID;

            if ( !a_Node.Layout.SceneRect.Contains( a_Position ) )
                return c_InvalidNodeID;

            // Later children are drawn on top, so they are tried first.
            for ( const LayoutNode* child = a_Node.LastChild(); child; child = child->PrevSibling() )
            {
                if ( const NodeID hit = HitTestNode( *child, a_Position, a_InteractableOnly ); hit != c_InvalidNodeID )
                    return hit;
            }

            if ( !a_InteractableOnly || ( a_Node.Widget && a_Node.Widget->IsInteractable() ) )
                return a_Node.ID;

            return c_InvalidNodeID;
        }

        constexpr bool IsMove( ENavAction a_Action )
        {
            return a_Action == ENavAction::MoveLeft || a_Action == ENavAction::MoveRight ||
                   a_Action == ENavAction::MoveUp   || a_Action == ENavAction::MoveDown;
        }

    } // namespace

    // ========================================================================
    // Layout
    // ========================================================================

    void Scene::UpdateLayout( Vec2<Unit> a_AvailableSize )
    {
        CleanupDestroyedWidgets();

        LayoutNode* rootNode = m_Layouts.Get( RootWidget );
        if ( !rootNode )
            return;

        const auto NeedsLayout = [&]
        {
            return IsLayoutPending() || !( a_AvailableSize == m_LaidOutSize ) || !( m_SafeArea == m_LaidOutSafeArea ) || !( RootWidget == m_LaidOutRoot );
        };

        // Arrange can ask for another pass (wrapped text changing height), and a geometry handler can change layout,
        // which is then laid out again here. Bounded, as content that never settles would loop forever.
        constexpr u32 c_MaxLayoutPasses = 4;
        u32           passes            = 0;
        while ( passes < c_MaxLayoutPasses && NeedsLayout() )
        {
            ++m_LayoutCount;
            if ( Size( m_LayoutScratch ) != m_LayoutScratchSize )
                Resize( m_LayoutScratch, m_LayoutScratchSize );

            BumpAllocator         allocator{ Data( m_LayoutScratch ), m_LayoutScratchSize };
            const Rect<Unit>      rootRect{ Vec2<Unit>{ 0_u, 0_u }, a_AvailableSize };
            Array<GeometryChange> changes;
            LayoutContext         layoutCtx{ allocator, m_SafeArea, &changes };

            bool settled = false;
            for ( ; passes < c_MaxLayoutPasses && !settled; ++passes )
            {
                MeasureLayoutNode( *rootNode, a_AvailableSize, layoutCtx );
                settled = !ArrangeLayoutNode( *rootNode, rootRect, layoutCtx );
            }

            ComposeLayout( *rootNode );

            // Layout that ran out of passes carries on at the next call.
            m_LaidOutSize     = settled ? a_AvailableSize : Vec2<Unit>{ Unit{ -1.f }, Unit{ -1.f } };
            m_LaidOutSafeArea = m_SafeArea;
            m_LaidOutRoot     = RootWidget;

            // Layout is done, so the handlers may change the scene.
            for ( size i = 0; i < Size( changes ); ++i )
            {
                // A node arranged in several passes is reported once, against what it was before the first.
                LayoutNode& node = *changes[i].Node;
                if ( std::any_of( changes.begin(), changes.begin() + i, [&]( const GeometryChange& a_Change ) { return a_Change.Node == &node; } ) )
                    continue;

                const GeometryEvent event{ .Size                = node.Layout.ParentRect.Size,
                                           .PreviousSize        = changes[i].PreviousSize,
                                           .ContentSize         = node.Layout.ContentSize,
                                           .PreviousContentSize = changes[i].PreviousContentSize };
                if ( event.Size == event.PreviousSize && event.ContentSize == event.PreviousContentSize )
                    continue;

                for ( LayoutNode* owner = &node; owner; owner = owner->Parent() )
                {
                    if ( owner->Widget )
                    {
                        owner->Widget->OnGeometryChanged( node, event );
                        break;
                    }
                }
            }
        }

        if ( m_PendingBringIntoView != c_InvalidNodeID )
        {
            const NodeID target    = m_PendingBringIntoView;
            m_PendingBringIntoView = c_InvalidNodeID;
            BringIntoView( target );
        }

        // Layout or a content offset moved widgets under a pointer that didn't move, or one was shown or hidden.
        if ( rootNode->Layout.IsHitTestDirty )
        {
            rootNode->Layout.IsHitTestDirty = false;
            RefreshHover();
        }
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

        // The pointer is where it was: only what is under it changed.
        PointerEvent still = last;
        still.Delta        = { 0_u, 0_u };
        still.ScrollDelta  = { 0_u, 0_u };
        SetHovered( HoverTarget( still.Position, gesture ? gesture->CapturedWidget : c_InvalidNodeID ), still );
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
        // From the topmost node, not just interactable ones, so the wheel works over a scroll container's empty space too.
        const NodeID from = a_Captured != c_InvalidNodeID ? a_Captured : HitTest( a_Event.Position, false );
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
            Engage( a_Reply.GetEngageTarget() );
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
            BringIntoView( a_Node );

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

    void Scene::OnFocusTraitsChanged( IWidget& a_Widget )
    {
        const NodeID id = a_Widget.GetLayoutID();

        // Navigation was inside it as a group, so it leaves. A scope pushed by hand stays.
        if ( !a_Widget.IsNavGroup() )
        {
            Array<NavScope>& stack = Input.NavStack;
            stack.erase( std::remove_if( stack.begin(), stack.end(), [id]( const NavScope& a_Scope ) { return a_Scope.Scope == id && !a_Scope.Modal; } ),
                         stack.end() );
        }

        if ( !a_Widget.IsFocusable() && Input.FocusedWidget == id )
            ClearFocus();
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

        // A scope pushed by hand (e.g. a dialog) stays until it is popped, wherever focus goes, and so does what is
        // under it. Above the innermost one, keep the scopes that still contain the widget; drop the rest.
        size keep = Size( Input.NavStack );
        while ( keep > 0 && !Input.NavStack[keep - 1].Modal )
            --keep;
        while ( keep < Size( Input.NavStack ) && IsOnPath( Input.NavStack[keep].Scope ) )
            ++keep;
        while ( Size( Input.NavStack ) > keep )
            PopBack( Input.NavStack );

        // Push the groups between the innermost kept scope (or the root) and the widget.
        // Each one restores to itself on pop, as if it had been entered with Activate.
        const NodeID innermost = keep > 0 ? Input.NavStack[keep - 1].Scope : RootWidget;
        bool         below     = false;
        for ( LayoutNode* scope : path )
        {
            if ( below && scope->Widget && scope->Widget->IsNavGroup() )
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
        if ( const LayoutNode* scope = m_Layouts.Get( GetCurrentNavScope() ) )
            CollectNavTargets( *scope, candidates );

        if ( Empty( candidates ) )
        {
            if ( const LayoutNode* root = m_Layouts.Get( RootWidget ) )
                CollectNavTargets( *root, candidates );
        }

        if ( !Empty( candidates ) )
            SetFocus( candidates[0]->ID );
    }

    // ========================================================================
    // Hit Testing
    // ========================================================================

    NodeID Scene::HitTest( Vec2<Unit> a_Position, bool a_InteractableOnly ) const
    {
        const LayoutNode* root = m_Layouts.Get( RootWidget );
        return root ? HitTestNode( *root, a_Position, a_InteractableOnly ) : c_InvalidNodeID;
    }

    bool Scene::IsOver( NodeID a_Node, Vec2<Unit> a_Position ) const
    {
        for ( const LayoutNode* node = m_Layouts.Get( HitTest( a_Position ) ); node; node = node->Parent() )
        {
            if ( node->ID == a_Node )
                return true;
        }
        return false;
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

    Optional<Rect<Unit>> Scene::GetTextInputRect() const
    {
        const IWidget* focused = GetWidget( Input.FocusedWidget );
        return focused ? focused->GetTextInputRect() : NullOpt;
    }

    void Scene::ReleaseAllKeys()
    {
        // Taken first: a handler may change what is held.
        const HashMap<EButtonID, KeyPressTarget> presses = std::move( Input.KeyPresses );
        Input.KeyPresses.clear();
        Input.ModifierState     = EModifier::None;
        Input.DropNextTextInput = false;

        for ( const auto& [button, press] : presses )
        {
            if ( IWidget* target = GetWidget( press.Widget ) )
                target->OnPressCancelled( {} );
        }
    }

    void Scene::CancelPointer( PointerID a_Pointer )
    {
        PointerGestureState* gesture = FindValue( Input.PointerStates, a_Pointer );
        if ( !gesture || ( !gesture->IsDown && gesture->CapturedWidget == c_InvalidNodeID ) )
            return;

        const NodeID held = gesture->CapturedWidget != c_InvalidNodeID ? gesture->CapturedWidget : gesture->PressedWidget;
        gesture->IsDown         = false;
        gesture->IsDragging     = false;
        gesture->PressCount     = 0;
        gesture->CapturedWidget = c_InvalidNodeID;
        gesture->PressedWidget  = c_InvalidNodeID;

        if ( IWidget* widget = GetWidget( held ) )
            widget->OnPressCancelled( { .Pointer = a_Pointer } );

        // The capture is gone, so hover goes back to what is under the pointer.
        RefreshHover();
    }

    void Scene::ReleaseAllPointers()
    {
        Array<PointerID> pointers;
        for ( const auto& [pointerID, gesture] : Input.PointerStates )
            PushBack( pointers, pointerID );
        for ( const PointerID pointerID : pointers )
            CancelPointer( pointerID );
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
        const NodeID target = captured != c_InvalidNodeID ? captured : HitTest( a_Event.Position );

        // --- Hover transitions ---
        // Touches have no hover: a finger only points while it presses, and hover is shared by every pointer.
        if ( a_Event.Type != EPointerType::Touch )
            SetHovered( HoverTarget( a_Event.Position, captured ), a_Event );

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

        // --- Key release: goes to whatever took the press, even if focus moved since ---
        if ( isKey && a_Event.Released )
        {
            if ( auto it = Input.KeyPresses.find( a_Event.Button ); it != Input.KeyPresses.end() )
            {
                const KeyPressTarget press = it->second;
                Input.KeyPresses.erase( it );

                if ( IWidget* target = GetWidget( press.Widget ) )
                    ApplyReply( press.Activate ? target->OnActivate( { .Pressed = false } ) : target->OnButtonReleased( a_Event ) );
                return true;
            }
        }

        if ( isKey && a_Event.Pressed )
        {
            // --- The focused widget, then its ancestors ---
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

            // --- Then navigation ---
            ENavAction navAction = Input.NavMap.Resolve( a_Event.Button );
            if ( navAction == ENavAction::Next && HasAnyFlag( a_Event.Modifiers, EModifier::Shift ) )
                navAction = ENavAction::Previous;

            if ( navAction == ENavAction::None )
                return false;

            Input.InputMode = EInputMode::Navigation;
            if ( !ProcessNavPress( navAction, a_Event ) )
                return false;

            // Navigation used the press, so it takes the text and the release that follow.
            Input.DropNextTextInput = true;
            if ( Input.KeyPresses.find( a_Event.Button ) == Input.KeyPresses.end() )
                Input.KeyPresses[a_Event.Button] = {};
            return true;
        }

        // --- Pointer buttons go to the captured widget or the one under the pointer; other releases to the focused widget ---
        NodeID targetWidget = Input.FocusedWidget;

        const PointerID pointer        = a_Event.Pointer.value_or( c_MousePointerID );
        const NodeID    capturedBefore = GetCapturedWidget( pointer );
        if ( a_Event.PointerPosition && IsPointerButton( a_Event.Button ) )
        {
            const NodeID captured = GetCapturedWidget( pointer );
            const NodeID hit      = a_Event.Pressed || captured == c_InvalidNodeID ? HitTest( *a_Event.PointerPosition ) : c_InvalidNodeID;
            Input.InputMode = EInputMode::Pointer;

            UpdateGestureState( a_Event, hit );

            // The captured widget (if any) gets the event, so a release stays with the press target.
            targetWidget = captured != c_InvalidNodeID ? captured : hit;
        }

        if ( IWidget* widget = GetWidget( targetWidget ) )
        {
            Reply reply = Reply::Unhandled();
            if ( a_Event.Pressed )
                reply = widget->OnButtonPressed( a_Event );
            else if ( a_Event.Released )
                reply = widget->OnButtonReleased( a_Event );

            ApplyReply( reply, pointer );

            // The release ended a capture, so hover goes back to what is under the pointer.
            if ( a_Event.Released && capturedBefore != c_InvalidNodeID && GetCapturedWidget( pointer ) == c_InvalidNodeID )
                RefreshHover();

            return reply.IsHandled();
        }

        return false;
    }

    bool Scene::ProcessNavPress( ENavAction a_Action, const ButtonEvent& a_Event )
    {
        // An engaged widget left this key unhandled: it is offered the action, then arrows stay with it and the rest disengage it.
        if ( IsEngaged( Input.FocusedWidget ) )
        {
            if ( IWidget* engaged = GetWidget( Input.FocusedWidget ) )
            {
                const Reply reply = engaged->OnNavigate( NavEvent{ .Action = a_Action } );
                ApplyReply( reply );
                if ( reply.IsHandled() )
                    return true;
            }

            if ( a_Action == ENavAction::Next || a_Action == ENavAction::Previous )
            {
                Disengage();
                Navigate( a_Action );
            }
            else if ( !IsMove( a_Action ) && !a_Event.Repeat ) // Holding Activate down engages once; its repeats must not disengage again.
            {
                Disengage();
            }
            return true;
        }

        switch ( a_Action )
        {
            case ENavAction::Cancel:
                return Navigate( a_Action );

            case ENavAction::ActivatePressed:
            {
                // Holding the key down activates once, not once per key repeat.
                if ( a_Event.Repeat )
                    return Input.FocusedWidget != c_InvalidNodeID;

                // With nothing focused, the first press only picks the starting widget, as a move does.
                if ( Input.FocusedWidget == c_InvalidNodeID )
                {
                    EnsureInitialFocus();
                    return Input.FocusedWidget != c_InvalidNodeID;
                }

                const NodeID   target       = Input.FocusedWidget;
                const IWidget* widget       = GetWidget( target );
                const bool     sendsRelease = widget && !widget->IsNavGroup() && !widget->CanEngage();

                if ( !Navigate( a_Action ) )
                    return false;

                if ( sendsRelease )
                    Input.KeyPresses[a_Event.Button] = { .Widget = target, .Activate = true };
                return true;
            }

            case ENavAction::MoveLeft:
            case ENavAction::MoveRight:
            case ENavAction::MoveUp:
            case ENavAction::MoveDown:
            case ENavAction::Next:
            case ENavAction::Previous:
            {
                // With nothing focused, the first move or Tab only picks the starting widget. Shift+Tab still
                // moves on from it, wrapping to the last widget.
                const bool hadFocus = Input.FocusedWidget != c_InvalidNodeID;
                EnsureInitialFocus();
                const bool pickedStart = !hadFocus && Input.FocusedWidget != c_InvalidNodeID;
                if ( pickedStart && a_Action != ENavAction::Previous )
                    return true;

                return Navigate( a_Action ) || pickedStart;
            }

            default:
                return false;
        }
    }

    void Scene::UpdateGestureState( const ButtonEvent& a_Event, NodeID a_Hit )
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
            // Another button of a pointer that is already down doesn't start a new gesture or take its capture.
            if ( gesture.IsDown && a_Event.Button != gesture.Button )
                return;

            gesture.Button = a_Event.Button;
            const bool withinWindow = ( now - gesture.LastPressTime ) <= cfg.MultiPressWindowSeconds;
            const bool withinRange  = Math::DistanceSq( *a_Event.PointerPosition, gesture.DownPosition ) <= Math::Sq( cfg.MultiPressDistance );

            if ( !withinWindow || !withinRange )
                gesture.PressCount = 0;

            gesture.PressCount += 1;
            gesture.DownPosition = *a_Event.PointerPosition;
            gesture.LastDownTime = now;
            gesture.PressedWidget = a_Hit;
            gesture.IsDragging = false;
            gesture.LongPressFired = false;
            gesture.IsDown = true;

            if ( IWidget* widget = GetWidget( a_Hit ) )
            {
                // Captured on press, so the drag and the release stay with this widget.
                // The widget can change that in OnButtonPressed.
                gesture.CapturedWidget = a_Hit;

                // Focus moves on press, before the widget sees it, so a drag or a long press focuses too.
                // Pressing something that can't take focus leaves focus where it is.
                if ( widget->IsFocusable() )
                    SetFocus( a_Hit );
            }
            else
            {
                // Pressing empty space (or a non-interactable panel) unfocuses, e.g. leaves a text field.
                SetFocus( c_InvalidNodeID );
            }
        }
        else if ( a_Event.Released && gesture.IsDown && a_Event.Button == gesture.Button )
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
        Input.PushNavScope( a_ScopeID, Input.FocusedWidget, true );
    }

    bool Scene::PopNavScope()
    {
        const Optional<NavScope> popped = Input.PopNavScope();
        if ( !popped )
            return false;

        if ( popped->Restored != c_InvalidNodeID )
            SetFocus( popped->Restored );
        return true;
    }

    NavReply Scene::QueryBoundaryReply( ENavAction a_Action )
    {
        // The scope navigation is in decides: an entered group, or a widget pushed with PushNavScope (e.g. a modal).
        if ( IWidget* scope = GetWidget( GetCurrentNavScope() ) )
            return scope->OnNavigationBoundary( NavEvent{ .Action = a_Action } );
        return NavReply::Escape();
    }

    void Scene::CollectNavTargets( const LayoutNode& a_Scope, Array<const LayoutNode*>& a_Out, bool a_IntoGroups, NodeID a_Keep ) const
    {
        a_Scope.ForEachChild( [&]( const LayoutNode& child )
        {
            if ( !Visibility::IsRendered( child.Layout.Visibility ) )
                return;

            const bool stop  = child.Widget && child.Widget->IsNavStop();
            const bool group = child.Widget && child.Widget->IsNavGroup();

            if ( stop && ( !group || !a_IntoGroups || child.ID == a_Keep ) )
                PushBack( a_Out, &child );

            if ( !group || a_IntoGroups )
                CollectNavTargets( child, a_Out, a_IntoGroups, a_Keep );
        } );
    }

    void Scene::BringIntoView( NodeID a_Node )
    {
        // Until the pending layout has run, the node may not be where (or as large as) it is going to be.
        if ( IsLayoutPending() )
        {
            m_PendingBringIntoView = a_Node;
            return;
        }

        const LayoutNode* node = m_Layouts.Get( a_Node );
        if ( !node )
            return;

        // Innermost first: each widget may move the node, so the next one out sees where it ended up.
        for ( LayoutNode* ancestor = node->Parent(); ancestor; ancestor = ancestor->Parent() )
        {
            if ( ancestor->Widget )
                ancestor->Widget->OnBringIntoView( *node );
        }
    }

    void Scene::SetContentOffset( NodeID a_Node, Vec2<Unit> a_Offset )
    {
        if ( LayoutNode* node = m_Layouts.Get( a_Node ) )
            RatUI::SetContentOffset( *node, a_Offset );
    }

    bool Scene::Navigate( ENavAction a_Action )
    {
        const auto FocusEnd = [&]( NodeID a_ScopeID, bool a_Last )
        {
            Array<const LayoutNode*> targets;
            if ( const LayoutNode* scopeNode = m_Layouts.Get( a_ScopeID ) )
                CollectNavTargets( *scopeNode, targets );
            if ( Empty( targets ) )
                return false;

            SetFocus( ( a_Last ? Back( targets ) : targets[0] )->ID );
            return true;
        };

        if ( a_Action == ENavAction::None )
            return false;

        if ( a_Action == ENavAction::Cancel )
        {
            // A scope pushed by hand belongs to its widget (e.g. a dialog), which may close itself or stay open instead.
            IWidget* owner = !Empty( Input.NavStack ) && Back( Input.NavStack ).Modal ? GetWidget( Back( Input.NavStack ).Scope ) : nullptr;
            if ( owner )
            {
                const Reply reply = owner->OnNavigate( NavEvent{ .Action = a_Action } );
                ApplyReply( reply );
                if ( reply.IsHandled() )
                    return true;
            }
            return PopNavScope();
        }

        if ( a_Action == ENavAction::ActivatePressed || a_Action == ENavAction::ActivateReleased )
        {
            const NodeID focusedID = Input.FocusedWidget;
            IWidget*     focused   = GetWidget( focusedID );
            if ( !focused )
                return false;

            const bool pressed = a_Action == ENavAction::ActivatePressed;
            if ( pressed && focused->IsNavGroup() )
            {
                Input.PushNavScope( focusedID, focusedID );
                FocusEnd( focusedID, false );
                return true;
            }

            if ( pressed && focused->CanEngage() )
            {
                Engage( focusedID );
                return IsEngaged( focusedID );
            }

            const Reply reply = focused->OnActivate( { .Pressed = pressed } );
            ApplyReply( reply );
            return reply.IsHandled();
        }

        const LayoutNode* focusedNode = m_Layouts.Get( Input.FocusedWidget );

        // Tab order is layout order through every group, wrapping at either end. Only a scope pushed by hand confines it.
        if ( a_Action == ENavAction::Next || a_Action == ENavAction::Previous )
        {
            NodeID rootID = RootWidget;
            for ( size i = Size( Input.NavStack ); i-- > 0; )
            {
                if ( Input.NavStack[i].Modal )
                {
                    rootID = Input.NavStack[i].Scope;
                    break;
                }
            }

            Array<const LayoutNode*> targets;
            if ( const LayoutNode* rootNode = m_Layouts.Get( rootID ) )
                CollectNavTargets( *rootNode, targets, true, Input.FocusedWidget );
            if ( Empty( targets ) )
                return false;

            const size  count   = Size( targets );
            const size  current = static_cast<size>( std::find( targets.begin(), targets.end(), focusedNode ) - targets.begin() );
            const bool  forward = a_Action == ENavAction::Next;
            size        next;
            if ( current == count )
                next = forward ? 0 : count - 1;
            else
                next = forward ? ( current + 1 ) % count : ( current + count - 1 ) % count;

            SetFocus( targets[next]->ID );
            return true;
        }

        const NodeID      scopeID   = GetCurrentNavScope();
        const LayoutNode* scopeNode = m_Layouts.Get( scopeID );
        if ( !scopeNode )
            return false;

        if ( !focusedNode )
            return FocusEnd( scopeID, false );

        Array<const LayoutNode*> targets;
        CollectNavTargets( *scopeNode, targets );

        if ( const LayoutNode* nextNode = FindNavigatableNode( a_Action, focusedNode, targets ) )
        {
            SetFocus( nextNode->ID );
            return true;
        }

        // Nothing further that way in the current scope: its widget decides.
        const NavReply navReply = QueryBoundaryReply( a_Action );

        switch ( navReply.GetRule() )
        {
            case NavReply::EBoundaryRule::Escape:
                // Arrows leave a group, back to the group itself. A scope pushed by hand is only left by Cancel or PopNavScope.
                if ( Empty( Input.NavStack ) || Back( Input.NavStack ).Modal )
                    return false;
                return PopNavScope();

            case NavReply::EBoundaryRule::Wrap:
                return FocusEnd( scopeID, a_Action == ENavAction::MoveLeft || a_Action == ENavAction::MoveUp );

            case NavReply::EBoundaryRule::Explicit:
                SetFocus( navReply.GetExplicitTarget() );
                return Input.FocusedWidget == navReply.GetExplicitTarget();
        }

        return false;
    }

    // ========================================================================
    // Widget Lifetime
    // ========================================================================

    void Scene::DestroyWidgetImmediately( LayoutNode& a_Node )
    {
        a_Node.DetachFromParent();

        // Each child detaches itself, so take the first until none are left.
        while ( LayoutNode* child = a_Node.FirstChild() )
            DestroyWidgetImmediately( *child );

        const NodeID id = a_Node.ID;

        // Nothing may still refer to it afterwards: a later widget can reuse the NodeID.
        if ( IWidget* widget = a_Node.Widget.get() )
        {
            if ( Input.EngagedWidget == id )
            {
                Input.EngagedWidget = c_InvalidNodeID;
                widget->OnDisengage();
            }
            if ( Input.FocusedWidget == id )
            {
                Input.FocusedWidget = c_InvalidNodeID;
                widget->OnFocusLost( FocusEvent{} );
            }
            widget->OnDestroy();
        }

        Array<NavScope>& stack = Input.NavStack;
        stack.erase( std::remove_if( stack.begin(), stack.end(), [id]( const NavScope& a_Scope ) { return a_Scope.Scope == id; } ), stack.end() );
        for ( NavScope& scope : stack )
        {
            if ( scope.Restored == id )
                scope.Restored = c_InvalidNodeID;
        }

        for ( auto it = Input.KeyPresses.begin(); it != Input.KeyPresses.end(); )
            it = it->second.Widget == id ? Input.KeyPresses.erase( it ) : std::next( it );

        for ( auto& [pointerID, gesture] : Input.PointerStates )
        {
            if ( gesture.CapturedWidget == id )
                gesture.CapturedWidget = c_InvalidNodeID;
            if ( gesture.PressedWidget == id )
                gesture.PressedWidget = c_InvalidNodeID;
        }
        if ( Input.HoveredWidget == id )
            Input.HoveredWidget = c_InvalidNodeID;
        if ( Input.Wheel.Widget == id )
            Input.Wheel = {};

        m_Layouts.Deallocate( id );
    }

    void Scene::CleanupDestroyedWidgets()
    {
        // Destroying can queue more (e.g. from OnDestroy), so by index, until none are left.
        for ( size i = 0; i < Size( m_ToDestroy ); ++i )
        {
            if ( LayoutNode* node = m_Layouts.Get( m_ToDestroy[i] ) )
                DestroyWidgetImmediately( *node );
        }

        Clear( m_ToDestroy );
    }

    void Scene::Reset()
    {
        m_Layouts.Clear();
        RootWidget = c_InvalidNodeID;
        Input.Reset();
        Clear( m_ToDestroy );
        m_LaidOutSize = { Unit{ -1.f }, Unit{ -1.f } };
        m_PendingBringIntoView = c_InvalidNodeID;
    }

    LayoutNode& Scene::CreateLayoutNode( LayoutStyle a_Style, NodeID a_ParentID )
    {
        if ( const IWidget* parent = GetWidget( a_ParentID ) )
            a_ParentID = parent->GetContentNodeID();

        // A node made under a parent that is gone would never be drawn or destroyed.
        LayoutNode* parentNode = m_Layouts.Get( a_ParentID );
        RATUI_USER_ASSERT( parentNode || a_ParentID == c_InvalidNodeID, "Creating a widget or layout node under a parent that no longer exists." );

        NodeID nodeID = m_Layouts.Allocate( a_Style );
        LayoutNode* node = m_Layouts.Get( nodeID );
        RATUI_ASSERT( node, "Failed to allocate layout node." );

        if ( parentNode )
            parentNode->PushBackChild( *node );

        return *node;
    }

} // namespace RatUI
