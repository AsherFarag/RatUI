#include <RatUI/Widget/Scene.h>

namespace RatUI
{
    void Scene::UpdateLayout( Vec2<Unit> a_AvailableSize )
    {
        CleanupDestroyedWidgets();

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
            {
                anyChanged |= Self( Self, child, innerSize );
            } );

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

		//ProcessPointerEvent( Input.LastPointerEvent );
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
                ApplyReply( widget->OnLongPress( evt ) );
            }
        }
    }

    void Scene::Render( DrawList& a_DrawList, f32 a_DeltaSeconds )
    {
        CleanupDestroyedWidgets();

        if ( LayoutNode* rootNode = Layouts.Get( RootWidget ) )
            if ( rootNode->Widget )
                rootNode->Widget->Paint( PaintEvent{ a_DrawList, a_DeltaSeconds } );
    }

    // ========================================================================
    // Widget Lookup
    // ========================================================================

    IWidget* Scene::GetWidget( NodeID a_ID )
    {
        LayoutNode* node = Layouts.Get( a_ID );
        return ( node && node->Widget ) ? node->Widget.get() : nullptr;
    }

    const IWidget* Scene::GetWidget( NodeID a_ID ) const
    {
        const LayoutNode* node = Layouts.Get( a_ID );
        return ( node && node->Widget ) ? node->Widget.get() : nullptr;
    }

    // ========================================================================
    // Reply Application
    // ========================================================================

    void Scene::ApplyReply( const Reply& a_Reply )
    {
        if ( a_Reply.ShouldCaptureMouse() )
            Input.CapturedWidget = a_Reply.GetMouseCaptureTarget();

        if ( a_Reply.ShouldReleaseMouse() )
            Input.CapturedWidget = c_InvalidNodeID;

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

        if ( IWidget* next = GetWidget( a_Node ) )
            next->OnFocusReceived( FocusEvent{} );
    }

    void Scene::EnsureInitialFocus()
    {
        if ( Input.FocusedWidget != c_InvalidNodeID )
            return;

        LayoutNode* root = Layouts.Get( RootWidget );
        if ( !root )
            return;

        Array<const LayoutNode*> candidates;
        CollectFocusableCandidates( *root, candidates );

        if ( !Empty( candidates ) )
            SetFocus( candidates[0]->Widget->GetLayoutID() );
    }

    // ========================================================================
    // Hit Testing
    // ========================================================================

    NodeID Scene::HitTest( NodeID a_ID, Vec2<Unit> a_LogicalPos )
    {
        LayoutNode* node = Layouts.Get( a_ID );
        if ( !node )
            return c_InvalidNodeID;

        if ( !Visibility::IsHitTestable( node->Layout.Visibility ) )
            return c_InvalidNodeID;

        if ( !node->Layout.FinalRect.Contains( a_LogicalPos ) )
            return c_InvalidNodeID;

        // Children are checked first, topmost (last-drawn) wins.
        NodeID childHit = c_InvalidNodeID;
        node->ForEachChildReverse( [&]( LayoutNode& child )
        {
            if ( childHit != c_InvalidNodeID || !child.Widget )
                return;

            childHit = HitTest( child.Widget->GetLayoutID(), a_LogicalPos );
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

		return false;
    }

    bool Scene::ProcessPointerEvent( const PointerEvent& a_Event )
    {
        Input.LastPointerEvent = a_Event;

        if ( a_Event.IsMouse() || Math::LengthSq( a_Event.Delta ) > 0_u )
            Input.InputMode = EInputMode::Pointer;

        const NodeID captured = Input.CapturedWidget;
        const NodeID target = captured != c_InvalidNodeID ? captured : HitTest( RootWidget, a_Event.Position );

        // --- Hover transitions ---
        if ( captured == c_InvalidNodeID && target != Input.HoveredWidget )
        {
            if ( IWidget* prev = GetWidget( Input.HoveredWidget ) )
                ApplyReply( prev->OnPointerExit( a_Event ) );

            Input.HoveredWidget = target;

            if ( IWidget* next = GetWidget( target ) )
                ApplyReply( next->OnPointerEnter( a_Event ) );
        }

        bool handled = false;

        // --- Drag detection & movement ---
        PointerGestureState& gesture = Input.PointerStates[a_Event.ID];
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
                    ApplyReply( widget->OnDragStart( drag ) );
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
                ApplyReply( widget->OnDragMove( drag ) );
            }
        }

        // --- Raw pointer dispatch (move / scroll) ---
        if ( IWidget* widget = GetWidget( target ) )
        {
            if ( a_Event.ScrollDelta[0] != 0_u || a_Event.ScrollDelta[1] != 0_u )
                ApplyReply( widget->OnPointerScroll( a_Event ) );

            Reply moveReply = widget->OnPointerMove( a_Event );
            ApplyReply( moveReply );
            handled = handled || moveReply.IsHandled();
        }

        return handled;
    }

    bool Scene::ProcessButtonEvent( const ButtonEvent& a_Event )
    {
        if ( a_Event.Pressed || a_Event.Released )
            Input.UpdateModifiers( a_Event.Button, a_Event.Pressed );

		// --- Navigation mapping ---
        const ENavAction navAction = Input.NavMap.Resolve( a_Event.Button );
        if ( navAction != ENavAction::None )
        {
            Input.InputMode = EInputMode::Navigation;
            EnsureInitialFocus();

            if ( navAction == ENavAction::ActivatePressed || navAction == ENavAction::ActivateReleased )
            {
                if ( a_Event.Pressed )  Navigate( ENavAction::ActivatePressed );
                if ( a_Event.Released ) Navigate( ENavAction::ActivateReleased );
            }
            else if ( a_Event.Pressed )
            {
                Navigate( navAction );
            }

            return true; // Nav mappings always consume the button; it doesn't reach the focused widget raw.
        }

        // --- Gesture from button press/release ---
        NodeID targetWidget = Input.FocusedWidget;

        if ( a_Event.PointerPosition && IsPointerButton( a_Event.Button ) )
        {
            UpdateGestureState( a_Event );

            // Pointer buttons target whatever's captured/hit, not keyboard focus
            targetWidget = Input.CapturedWidget != c_InvalidNodeID
                ? Input.CapturedWidget
                : HitTest( RootWidget, *a_Event.PointerPosition );
        }

		// --- Dispatch to focused widget ---
        if ( IWidget* focused = GetWidget( targetWidget ) )
        {
            Reply reply = Reply::Unhandled();
            if ( a_Event.Pressed )
                reply = focused->OnButtonPressed( a_Event );
			else if ( a_Event.Released )
				reply = focused->OnButtonReleased( a_Event );

            ApplyReply( reply );
            return reply.IsHandled();
        }

        return false;
    }

    void Scene::UpdateGestureState( const ButtonEvent& a_Event )
    {
        PointerGestureState& gesture = Input.PointerStates[a_Event.PointerID.value_or( 0 )];
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
                    Input.CapturedWidget = target;
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
                    ApplyReply( widget->OnDragEnd( drag ) );
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
                    ApplyReply( reply );

                    if ( widget->IsFocusable() )
                        SetFocus( gesture.PressedWidget );
                }
            }

            gesture.LastPressTime = now;
            gesture.IsDown = false;
            gesture.IsDragging = false;

            // Release capture on button up
            Input.CapturedWidget = c_InvalidNodeID;
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
        LayoutNode* node = Layouts.Get( a_Focused );
        while ( node )
        {
            if ( node->Widget && node->Widget->IsNavigationBoundary() )
                return node->Widget->OnNavigationBoundary( a_Action );

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
        Input.InputMode = EInputMode::Navigation;

        if ( a_Action == ENavAction::ActivatePressed || a_Action == ENavAction::ActivateReleased )
        {
            IWidget* focused = GetWidget( Input.FocusedWidget );
            if ( !focused )
                return;

            ButtonEvent synthetic{};
            synthetic.Pressed   = a_Action == ENavAction::ActivatePressed;
            synthetic.Released  = a_Action == ENavAction::ActivateReleased;
            synthetic.Modifiers = Input.ModifierState;

            Reply reply = synthetic.Pressed ? focused->OnButtonPressed( synthetic )
                                             : focused->OnButtonReleased( synthetic );
            ApplyReply( reply );
            return;
        }

        if ( a_Action == ENavAction::Cancel )
        {
            NavReply boundary = QueryBoundaryReply( a_Action, Input.FocusedWidget );

            if ( boundary.GetRule() == NavReply::EBoundaryRule::Explicit )
            {
                SetFocus( boundary.GetExplicitTarget() );
                return;
            }

            if ( Input.HasNavScopes() )
                PopNavScope();

            return;
        }

        NavReply boundary = QueryBoundaryReply( a_Action, Input.FocusedWidget );

        switch ( boundary.GetRule() )
        {
            case NavReply::EBoundaryRule::Explicit:
                SetFocus( boundary.GetExplicitTarget() );
                return;

            case NavReply::EBoundaryRule::Custom:
            {
                NodeID target = boundary.ResolveCustom( a_Action, Input.FocusedWidget );
                if ( target != c_InvalidNodeID )
                    SetFocus( target );
                return;
            }

            default:
                break; // Escape / Stop fall through to spatial search below.
        }

        const NodeID scopeID = GetCurrentNavScope();
        LayoutNode* start = Layouts.Get( Input.FocusedWidget );
        LayoutNode* scope = Layouts.Get( scopeID );

        if ( !start || !scope )
            return;

        Array<const LayoutNode*> candidates;
        CollectFocusableCandidates( *scope, candidates );

        if ( const LayoutNode* best = FindNavigatableNode( a_Action, start, candidates ) )
        {
            SetFocus( best->Widget->GetLayoutID() );
            return;
        }

        if ( boundary.GetRule() == NavReply::EBoundaryRule::Stop )
            return; // Trapped; no candidate found. (Wrap-around is a future extension, not implemented here.)

        // Escape and nothing found locally: pop up a scope and retry there.
        if ( Input.HasNavScopes() )
        {
            PopNavScope();
            Navigate( a_Action );
        }
    }

    // ========================================================================
    // Reset
    // ========================================================================

    void Scene::Reset()
    {
        Layouts.Clear();
        RootWidget = c_InvalidNodeID;
        Input.Reset();
        Clear( m_ToDestory );
    }

} // namespace RatUI
