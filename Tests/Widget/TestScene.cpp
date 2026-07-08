/**
 * @file TestScene.cpp
 * @brief Tests for RatUI::Scene: widget creation/destruction, layout updates,
 *   input dispatch, focus management, navigation scopes, and reset.
 */

#include "../Common/Common.h"
#include <RatUI/Widget/ButtonWidget.h>

using namespace RatUI;

using WidgetID = NodeID;
inline constexpr WidgetID c_InvalidWidgetID = c_InvalidNodeID;

template<typename WidgetType, typename... Args>
static WidgetID CreateRootWidgetID( Scene& a_Scene, Args&&... a_Args )
{
    return a_Scene.CreateRootWidget<WidgetType>( std::forward<Args>( a_Args )... )->GetLayoutID();
}

template<typename WidgetType, typename... Args>
static WidgetID CreateWidgetID( Scene& a_Scene, WidgetID a_ParentID, Args&&... a_Args )
{
    return a_Scene.CreateWidget<WidgetType>( a_ParentID, std::forward<Args>( a_Args )... )->GetLayoutID();
}

// =============================================================================
// Minimal test widget helpers
// =============================================================================

// TODO: Need to upgrade widget tests

/** A simple widget that records the callbacks it receives. */
class TrackingWidget : public IWidget
{
public:
    int ConstructCount{ 0 };
    int DestroyCount{ 0 };
    int PaintCount{ 0 };

    int PointerEnterCount{ 0 };
    int PointerExitCount{ 0 };
    int PressedCount{ 0 };
    int ReleasedCount{ 0 };
    int FocusReceivedCount{ 0 };
    int FocusLostCount{ 0 };

    bool Focusable{ false };
    bool ConsumePress{ false };

    void OnConstruct() override                   { ++ConstructCount; }
    void OnDestroy() override                     { ++DestroyCount; }
    void OnPaint( const PaintEvent& ) override   { ++PaintCount; }

    bool IsFocusable() const override             { return Focusable; }
    bool IsInteractable() const override          { return true; }
    bool IsNavigationBoundary() const override    { return true; }

    void OnFocusReceived( const FocusEvent& ) override               { ++FocusReceivedCount; }
    void OnFocusLost( const FocusEvent& ) override                   { ++FocusLostCount; }

    Reply OnPointerEnter( const PointerEvent& ) override { ++PointerEnterCount; return Reply::Unhandled(); }
    Reply OnPointerExit( const PointerEvent& ) override  { ++PointerExitCount; return Reply::Unhandled(); }

    Reply OnButtonPressed ( const ButtonEvent& ) override { ++PressedCount;  return Reply::Unhandled(); }
    Reply OnButtonReleased( const ButtonEvent& ) override { ++ReleasedCount; return Reply::Unhandled(); }
};

/** Builds a mouse PointerEvent at the given position. */
static InputEvent MakeMouseMove( Vec2f a_Pos )
{
    return InputEvent{
        .Device  = EDeviceID::Mouse,
        .Payload = PointerEvent{ .Position = ToUnitVec2( a_Pos ), .Type = EPointerType::Mouse }
    };
}

/** Builds a mouse button-press InputEvent. */
static InputEvent MakeMousePress( bool a_Pressed )
{
    return InputEvent{
        .Device  = EDeviceID::Mouse,
        .Payload = ButtonEvent{ .Button = EButtonID::MouseLeft, .Pressed = a_Pressed, .Released = !a_Pressed }
    };
}

// =============================================================================
// CreateRootWidget / CreateWidget
// =============================================================================

TEST_CASE( "CreateRootWidget sets RootWidget and calls OnConstruct", "[scene]" )
{
    Scene scene;
    WidgetID id = CreateRootWidgetID<TrackingWidget>( scene );

    REQUIRE( id != c_InvalidWidgetID );
    REQUIRE( scene.RootWidget == id );

    TrackingWidget* w = scene.GetWidget<TrackingWidget>( id );
    REQUIRE( w != nullptr );
    REQUIRE( w->ConstructCount == 1 );
}

TEST_CASE( "CreateWidget attaches child to parent layout node", "[scene]" )
{
    Scene scene;
    WidgetID rootID  = CreateRootWidgetID<TrackingWidget>( scene );
    WidgetID childID = CreateWidgetID<TrackingWidget>( scene, rootID );

    IWidget* root  = scene.GetWidget( rootID );
    IWidget* child = scene.GetWidget( childID );
    REQUIRE( root  != nullptr );
    REQUIRE( child != nullptr );

    LayoutNode* rootNode  = scene.Layouts.Get( root->GetLayoutID() );
    LayoutNode* childNode = scene.Layouts.Get( child->GetLayoutID() );
    REQUIRE( rootNode  != nullptr );
    REQUIRE( childNode != nullptr );

    REQUIRE( rootNode->FirstChild() == childNode );
    REQUIRE( childNode->Parent()    == rootNode );
    REQUIRE( rootNode->NumChildren == 1 );
}

TEST_CASE( "CreateWidget calls OnConstruct on each created widget", "[scene]" )
{
    Scene scene;
    WidgetID rootID  = CreateRootWidgetID<TrackingWidget>( scene );
    WidgetID childID = CreateWidgetID<TrackingWidget>( scene, rootID );

    REQUIRE( scene.GetWidget<TrackingWidget>( rootID  )->ConstructCount == 1 );
    REQUIRE( scene.GetWidget<TrackingWidget>( childID )->ConstructCount == 1 );
}

// =============================================================================
// GetWidget
// =============================================================================

TEST_CASE( "GetWidget returns nullptr for invalid ID", "[scene]" )
{
    Scene scene;
    REQUIRE( scene.GetWidget( c_InvalidWidgetID ) == nullptr );
}

TEST_CASE( "GetWidget<Type> returns typed pointer for matching type", "[scene]" )
{
    Scene scene;
    WidgetID id = CreateRootWidgetID<TrackingWidget>( scene );

    TrackingWidget* typed = scene.GetWidget<TrackingWidget>( id );
    REQUIRE( typed != nullptr );
    REQUIRE( typed->GetLayoutID() == id );
}

// =============================================================================
// DestroyWidget
// =============================================================================

TEST_CASE( "DestroyWidget calls OnDestroy and invalidates the ID", "[scene]" )
{
    Scene scene;
    WidgetID id = CreateRootWidgetID<TrackingWidget>( scene );
    TrackingWidget* raw = scene.GetWidget<TrackingWidget>( id );
    REQUIRE( raw != nullptr );

    bool destroyed = scene.DestroyWidget( id );
    REQUIRE( destroyed );

    scene.UpdateLayout( ToUnitVec2( Vec2f{ 1.0f, 1.0f } ) );

    REQUIRE( raw->DestroyCount == 1 );
    REQUIRE( scene.GetWidget( id ) == nullptr );
}

TEST_CASE( "DestroyWidget returns false for an invalid ID", "[scene]" )
{
    Scene scene;
    REQUIRE( scene.DestroyWidget( c_InvalidWidgetID ) == true );
}

TEST_CASE( "DestroyWidget recursively destroys child widgets", "[scene]" )
{
    Scene scene;
    WidgetID rootID  = CreateRootWidgetID<TrackingWidget>( scene );
    WidgetID childID = CreateWidgetID<TrackingWidget>( scene, rootID );

    scene.DestroyWidget( rootID );
    scene.UpdateLayout( ToUnitVec2( Vec2f{ 1.0f, 1.0f } ) );

    REQUIRE( scene.GetWidget( rootID  ) == nullptr );
    REQUIRE( scene.GetWidget( childID ) == nullptr );
}

// =============================================================================
// ForEachChildWidget
// =============================================================================

TEST_CASE( "ForEachChildWidget iterates each direct child widget", "[scene]" )
{
    Scene scene;
    WidgetID rootID   = CreateRootWidgetID<TrackingWidget>( scene );
    WidgetID childID1 = CreateWidgetID<TrackingWidget>( scene, rootID );
    WidgetID childID2 = CreateWidgetID<TrackingWidget>( scene, rootID );

    Array<WidgetID> seen{};
    scene.ForEachChildWidget( scene.GetWidget( rootID )->GetLayoutID(), [&]( IWidget& w ) { PushBack( seen, w.GetLayoutID() ); } );

    REQUIRE( Size( seen ) == 2 );
    REQUIRE( seen[ 0 ] == childID1 );
    REQUIRE( seen[ 1 ] == childID2 );
}

// =============================================================================
// UpdateLayout
// =============================================================================

TEST_CASE( "UpdateLayout assigns a non-zero FinalRect to the root when root is fixed-sized", "[scene]" )
{
    Scene scene;
    WidgetID rootID = CreateRootWidgetID<TrackingWidget>( scene );

    LayoutNode* rootNode = scene.Layouts.Get( scene.GetWidget( rootID )->GetLayoutID() );
    REQUIRE( rootNode != nullptr );
    rootNode->Style.WidthMode   = ESizingMode::Fixed;
    rootNode->Style.HeightMode  = ESizingMode::Fixed;
    rootNode->Style.FixedWidth  = Unit{ 800.0f };
    rootNode->Style.FixedHeight = Unit{ 600.0f };

    scene.UpdateLayout( ToUnitVec2( Vec2f{ 800.0f, 600.0f } ) );

    Rectf rootRect = ToFloatRect( rootNode->Layout.FinalRect );
    REQUIRE( rootRect.Size[ 0 ] == Catch::Approx( 800.0f ).epsilon( 1e-5f ) );
    REQUIRE( rootRect.Size[ 1 ] == Catch::Approx( 600.0f ).epsilon( 1e-5f ) );
}

TEST_CASE( "UpdateLayout positions child widgets in a horizontal layout", "[scene]" )
{
    Scene scene;
    WidgetID rootID   = CreateRootWidgetID<TrackingWidget>( scene );
    WidgetID childID1 = CreateWidgetID<TrackingWidget>( scene, rootID );
    WidgetID childID2 = CreateWidgetID<TrackingWidget>( scene, rootID );

    LayoutNode* rootNode = scene.Layouts.Get( scene.GetWidget( rootID )->GetLayoutID() );
    rootNode->Style.LayoutType  = ELayoutType::Horizontal;
    rootNode->Style.WidthMode   = ESizingMode::Fixed;
    rootNode->Style.HeightMode  = ESizingMode::Fixed;
    rootNode->Style.FixedWidth  = Unit{ 200.0f };
    rootNode->Style.FixedHeight = Unit{ 100.0f };

    LayoutNode* child1Node = scene.Layouts.Get( scene.GetWidget( childID1 )->GetLayoutID() );
    child1Node->Style.WidthMode   = ESizingMode::Fixed;
    child1Node->Style.HeightMode  = ESizingMode::Fixed;
    child1Node->Style.FixedWidth  = Unit{ 80.0f };
    child1Node->Style.FixedHeight = Unit{ 60.0f };

    LayoutNode* child2Node = scene.Layouts.Get( scene.GetWidget( childID2 )->GetLayoutID() );
    child2Node->Style.WidthMode   = ESizingMode::Fixed;
    child2Node->Style.HeightMode  = ESizingMode::Fixed;
    child2Node->Style.FixedWidth  = Unit{ 60.0f };
    child2Node->Style.FixedHeight = Unit{ 40.0f };

    scene.UpdateLayout( ToUnitVec2( Vec2f{ 200.0f, 100.0f } ) );

    // First child starts at origin
    REQUIRE( child1Node->Layout.FinalRect.Origin[ 0 ] == Catch::Approx( 0.0f ).epsilon( 1e-5f ) );
    REQUIRE( child1Node->Layout.FinalRect.Size[ 0 ]   == Catch::Approx( 80.0f ).epsilon( 1e-5f ) );

    // Second child starts right after the first
    REQUIRE( child2Node->Layout.FinalRect.Origin[ 0 ] == Catch::Approx( 80.0f ).epsilon( 1e-5f ) );
    REQUIRE( child2Node->Layout.FinalRect.Size[ 0 ]   == Catch::Approx( 60.0f ).epsilon( 1e-5f ) );
}

TEST_CASE( "UpdateLayout with no root widget is a no-op", "[scene]" )
{
    Scene scene;
    scene.UpdateLayout( ToUnitVec2( Vec2f{ 800.0f, 600.0f } ) ); // should not crash
}

// =============================================================================
// Focus management
// =============================================================================

TEST_CASE( "GetFocusedWidget returns invalid ID when nothing is focused", "[scene][focus]" )
{
    Scene scene;
    REQUIRE( scene.GetFocusedNode() == c_InvalidWidgetID );
}

TEST_CASE( "SetFocus calls OnFocusReceived on newly focused widget", "[scene][focus]" )
{
    Scene scene;
    WidgetID rootID = CreateRootWidgetID<TrackingWidget>( scene );
    TrackingWidget* w = scene.GetWidget<TrackingWidget>( rootID );
    w->Focusable = true;

    scene.SetFocus( rootID );

    REQUIRE( scene.GetFocusedNode() == rootID );
    REQUIRE( w->FocusReceivedCount == 1 );
    REQUIRE( w->FocusLostCount     == 0 );
}

TEST_CASE( "SetFocus calls OnFocusLost on the previously focused widget", "[scene][focus]" )
{
    Scene scene;
    WidgetID id1 = CreateRootWidgetID<TrackingWidget>( scene );
    WidgetID id2 = CreateWidgetID<TrackingWidget>( scene, id1 );

    TrackingWidget* w1 = scene.GetWidget<TrackingWidget>( id1 );
    TrackingWidget* w2 = scene.GetWidget<TrackingWidget>( id2 );
    w1->Focusable = true;
    w2->Focusable = true;

    scene.SetFocus( id1 );
    scene.SetFocus( id2 );

    REQUIRE( w1->FocusLostCount     == 1 );
    REQUIRE( w2->FocusReceivedCount == 1 );
    REQUIRE( scene.GetFocusedNode() == id2 );
}

TEST_CASE( "ClearFocus removes the current focused widget", "[scene][focus]" )
{
    Scene scene;
    WidgetID id = CreateRootWidgetID<TrackingWidget>( scene );
    scene.GetWidget<TrackingWidget>( id )->Focusable = true;

    scene.SetFocus( id );
    REQUIRE( scene.GetFocusedNode() == id );

    scene.ClearFocus();
    REQUIRE( scene.GetFocusedNode() == c_InvalidWidgetID );
}

TEST_CASE( "SetFocus is idempotent when called twice with the same ID", "[scene][focus]" )
{
    Scene scene;
    WidgetID id = CreateRootWidgetID<TrackingWidget>( scene );
    scene.GetWidget<TrackingWidget>( id )->Focusable = true;

    scene.SetFocus( id );
    scene.SetFocus( id ); // second call should be a no-op

    REQUIRE( scene.GetWidget<TrackingWidget>( id )->FocusReceivedCount == 1 );
}

// =============================================================================
// Input dispatch – pointer events
// =============================================================================

TEST_CASE( "DispatchInputEvent pointer over root triggers OnPointerEnter", "[scene][input]" )
{
    Scene scene;
    WidgetID rootID = CreateRootWidgetID<TrackingWidget>( scene );

    LayoutNode* node = scene.Layouts.Get( scene.GetWidget( rootID )->GetLayoutID() );
    node->Style.WidthMode   = ESizingMode::Fixed;
    node->Style.HeightMode  = ESizingMode::Fixed;
    node->Style.FixedWidth  = Unit{ 200.0f };
    node->Style.FixedHeight = Unit{ 200.0f };
    scene.UpdateLayout( ToUnitVec2( Vec2f{ 200.0f, 200.0f } ) );

    scene.DispatchInputEvent( MakeMouseMove( Vec2f{ 50.0f, 50.0f } ) );

    TrackingWidget* w = scene.GetWidget<TrackingWidget>( rootID );
    REQUIRE( w->PointerEnterCount == 1 );
    REQUIRE( w->PointerExitCount  == 0 );
}

TEST_CASE( "DispatchInputEvent pointer leaving widget triggers OnPointerExit", "[scene][input]" )
{
    Scene scene;
    WidgetID rootID = CreateRootWidgetID<TrackingWidget>( scene );

    LayoutNode* node = scene.Layouts.Get( scene.GetWidget( rootID )->GetLayoutID() );
    node->Style.WidthMode   = ESizingMode::Fixed;
    node->Style.HeightMode  = ESizingMode::Fixed;
    node->Style.FixedWidth  = Unit{ 100.0f };
    node->Style.FixedHeight = Unit{ 100.0f };
    // UpdateLayout allocates the root the full available rect {0,0,100,100}
    scene.UpdateLayout( ToUnitVec2( Vec2f{ 100.0f, 100.0f } ) );

    // Move inside the 100x100 viewport
    scene.DispatchInputEvent( MakeMouseMove( Vec2f{ 50.0f, 50.0f } ) );
    // Move outside the 100x100 viewport
    scene.DispatchInputEvent( MakeMouseMove( Vec2f{ 200.0f, 200.0f } ) );

    TrackingWidget* w = scene.GetWidget<TrackingWidget>( rootID );
    REQUIRE( w->PointerExitCount == 1 );
}

TEST_CASE( "DispatchInputEvent button press dispatches to hovered widget", "[scene][input]" )
{
    Scene scene;
    WidgetID rootID = CreateRootWidgetID<TrackingWidget>( scene );

    LayoutNode* node = scene.Layouts.Get( scene.GetWidget( rootID )->GetLayoutID() );
    node->Style.WidthMode   = ESizingMode::Fixed;
    node->Style.HeightMode  = ESizingMode::Fixed;
    node->Style.FixedWidth  = Unit{ 200.0f };
    node->Style.FixedHeight = Unit{ 200.0f };
    scene.UpdateLayout( ToUnitVec2( Vec2f{ 200.0f, 200.0f } ) );

    // Hover over root
    scene.DispatchInputEvent( MakeMouseMove( Vec2f{ 50.0f, 50.0f } ) );

    // Press
    scene.DispatchInputEvent( MakeMousePress( true ) );

    TrackingWidget* w = scene.GetWidget<TrackingWidget>( rootID );
    REQUIRE( w->PressedCount == 1 );
}

TEST_CASE( "DispatchInputEvent button press dispatches to focused widget when nothing hovered", "[scene][input]" )
{
    Scene scene;
    WidgetID rootID = CreateRootWidgetID<TrackingWidget>( scene );
    TrackingWidget* w = scene.GetWidget<TrackingWidget>( rootID );
    w->Focusable = true;

    // Focus the widget but don't hover it
    scene.SetFocus( rootID );

    scene.DispatchInputEvent( MakeMousePress( true ) );

    REQUIRE( w->PressedCount == 1 );
}

// =============================================================================
// Navigation scopes
// =============================================================================

TEST_CASE( "GetCurrentNavScope returns RootWidget when nav stack is empty", "[scene][nav]" )
{
    Scene scene;
    WidgetID rootID = CreateRootWidgetID<TrackingWidget>( scene );
    REQUIRE( scene.GetCurrentNavScope() == rootID );
}

TEST_CASE( "PushNavScope adds a scope to the nav stack", "[scene][nav]" )
{
    Scene scene;
    WidgetID rootID  = CreateRootWidgetID<TrackingWidget>( scene );
    WidgetID childID = CreateWidgetID<TrackingWidget>( scene, rootID );

    scene.PushNavScope( childID );

    REQUIRE( scene.GetCurrentNavScope() == childID );
}

TEST_CASE( "PopNavScope removes the top scope and restores previous focus", "[scene][nav]" )
{
    Scene scene;
    WidgetID rootID  = CreateRootWidgetID<TrackingWidget>( scene );
    WidgetID childID = CreateWidgetID<TrackingWidget>( scene, rootID );

    scene.GetWidget<TrackingWidget>( rootID )->Focusable = true;
    scene.SetFocus( rootID );

    scene.PushNavScope( childID );
    REQUIRE( scene.GetCurrentNavScope() == childID );

    scene.PopNavScope();
    REQUIRE( scene.GetCurrentNavScope() == rootID );
    REQUIRE( scene.GetFocusedNode() == rootID );
}

TEST_CASE( "PopNavScope on empty nav stack is a no-op", "[scene][nav]" )
{
    Scene scene;
    CreateRootWidgetID<TrackingWidget>( scene );
    scene.PopNavScope(); // should not crash
    REQUIRE( scene.GetCurrentNavScope() == scene.RootWidget );
}

// =============================================================================
// Navigate
// =============================================================================

TEST_CASE( "Navigate focuses the first focusable child when no widget is focused", "[scene][nav]" )
{
    Scene scene;
    WidgetID rootID  = CreateRootWidgetID<TrackingWidget>( scene );
    WidgetID childID = CreateWidgetID<TrackingWidget>( scene, rootID );

    scene.GetWidget<TrackingWidget>( childID )->Focusable = true;

    scene.Navigate( ENavAction::MoveRight );

    REQUIRE( scene.GetFocusedNode() == childID );
}

TEST_CASE( "Navigate MoveRight advances focus from first to second sibling", "[scene][nav]" )
{
    // Place two children side-by-side so MoveRight can find the second one.
    Scene scene;
    WidgetID rootID   = CreateRootWidgetID<TrackingWidget>( scene );
    WidgetID childID1 = CreateWidgetID<TrackingWidget>( scene, rootID );
    WidgetID childID2 = CreateWidgetID<TrackingWidget>( scene, rootID );

    scene.GetWidget<TrackingWidget>( childID1 )->Focusable = true;
    scene.GetWidget<TrackingWidget>( childID2 )->Focusable = true;

    LayoutNode* rootNode = scene.Layouts.Get( scene.GetWidget( rootID )->GetLayoutID() );
    rootNode->Style.LayoutType  = ELayoutType::Horizontal;
    rootNode->Style.WidthMode   = ESizingMode::Fixed;
    rootNode->Style.HeightMode  = ESizingMode::Fixed;
    rootNode->Style.FixedWidth  = Unit{ 200.0f };
    rootNode->Style.FixedHeight = Unit{ 50.0f };

    LayoutNode* node1 = scene.Layouts.Get( scene.GetWidget( childID1 )->GetLayoutID() );
    node1->Style.WidthMode  = ESizingMode::Fixed;
    node1->Style.HeightMode = ESizingMode::Fixed;
    node1->Style.FixedWidth  = Unit{ 80.0f };
    node1->Style.FixedHeight = Unit{ 50.0f };

    LayoutNode* node2 = scene.Layouts.Get( scene.GetWidget( childID2 )->GetLayoutID() );
    node2->Style.WidthMode  = ESizingMode::Fixed;
    node2->Style.HeightMode = ESizingMode::Fixed;
    node2->Style.FixedWidth  = Unit{ 80.0f };
    node2->Style.FixedHeight = Unit{ 50.0f };

    scene.UpdateLayout( ToUnitVec2( Vec2f{ 200.0f, 50.0f } ) );

    scene.SetFocus( childID1 );
    scene.Navigate( ENavAction::MoveRight );

    REQUIRE( scene.GetFocusedNode() == childID2 );
}

TEST_CASE( "Navigate Cancel pops the nav scope", "[scene][nav]" )
{
    Scene scene;
    WidgetID rootID  = CreateRootWidgetID<TrackingWidget>( scene );
    WidgetID childID = CreateWidgetID<TrackingWidget>( scene, rootID );

    scene.PushNavScope( childID );
    REQUIRE( scene.GetCurrentNavScope() == childID );

    scene.Navigate( ENavAction::Cancel );
    REQUIRE( scene.GetCurrentNavScope() == rootID );
}

// =============================================================================
// Reset
// =============================================================================

TEST_CASE( "Reset clears all widgets and resets state", "[scene]" )
{
    Scene scene;
    WidgetID rootID  = CreateRootWidgetID<TrackingWidget>( scene );
    WidgetID childID = CreateWidgetID<TrackingWidget>( scene, rootID );

    scene.GetWidget<TrackingWidget>( rootID )->Focusable = true;
    scene.SetFocus( rootID );

    scene.Reset();

    REQUIRE( scene.RootWidget == c_InvalidWidgetID );
    REQUIRE( scene.GetFocusedNode() == c_InvalidWidgetID );
    REQUIRE( scene.GetWidget( rootID  ) == nullptr );
    REQUIRE( scene.GetWidget( childID ) == nullptr );
    REQUIRE( scene.GetCurrentNavScope() == c_InvalidWidgetID );
}
