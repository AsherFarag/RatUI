#include "TestSceneCommon.h"
#include <RatUI/Widget/ButtonWidget.h>
#include <RatUI/Widget/PanelWidget.h>
#include <RatUI/Widget/SliderWidget.h>

/**
 * @brief A vertical root panel with two fixed-size buttons that count their clicks.
 */
struct ButtonSceneFixture
{
    RatUI::Scene  Scene;
    ButtonWidget* First{ nullptr };
    ButtonWidget* Second{ nullptr };
    int           FirstClicks{ 0 };
    int           SecondClicks{ 0 };

    ButtonSceneFixture()
    {
        PanelWidget* root = Scene.CreateRootWidget<PanelWidget>();
        root->GetLayout().LayoutType( ELayoutType::Vertical ).Padding( Edges::All( 10_u ) ).Spacing( 10_u );

        First  = Scene.CreateWidget<ButtonWidget>( root->GetLayoutID(), [this]( ButtonBaseWidget& ) { ++FirstClicks; } );
        Second = Scene.CreateWidget<ButtonWidget>( root->GetLayoutID(), [this]( ButtonBaseWidget& ) { ++SecondClicks; } );
        First->GetLayout().FixedWidth( 100_u ).FixedHeight( 40_u );
        Second->GetLayout().FixedWidth( 100_u ).FixedHeight( 40_u );

        Scene.UpdateLayout( { 400_u, 300_u } );
    }

    /** @brief Presses or releases the touch @p a_Pointer at @p a_Position. */
    void Touch( PointerID a_Pointer, Vec2<Unit> a_Position, bool a_Pressed )
    {
        Scene.DispatchInputEvent( { .Device  = EDeviceID::Touch,
                                    .Payload = ButtonEvent{ .Button          = EButtonID::MouseLeft,
                                                            .Pressed         = a_Pressed,
                                                            .Released        = !a_Pressed,
                                                            .Pointer         = a_Pointer,
                                                            .PointerPosition = a_Position } } );
    }

    /** @brief Moves the touch @p a_Pointer to @p a_Position. */
    void TouchMove( PointerID a_Pointer, Vec2<Unit> a_Position )
    {
        Scene.DispatchInputEvent( { .Device = EDeviceID::Touch, .Payload = PointerEvent{ .Position = a_Position, .Type = EPointerType::Touch, .ID = a_Pointer } } );
    }

    void MouseMove( Vec2<Unit> a_Position )
    {
        Scene.DispatchInputEvent( { .Device = EDeviceID::Mouse, .Payload = PointerEvent{ .Position = a_Position, .Type = EPointerType::Mouse } } );
    }

    void Click( Vec2<Unit> a_Position )
    {
        Scene.DispatchInputEvent( { .Device = EDeviceID::Mouse, .Payload = PointerEvent{ .Position = a_Position, .Type = EPointerType::Mouse } } );
        for ( const bool pressed : { true, false } )
        {
            Scene.DispatchInputEvent( { .Device  = EDeviceID::Mouse,
                                        .Payload = ButtonEvent{ .Button          = EButtonID::MouseLeft,
                                                                .Pressed         = pressed,
                                                                .Released        = !pressed,
                                                                .Pointer         = PointerID{ 0 },
                                                                .PointerPosition = a_Position } } );
        }
    }
};

TEST_CASE( "Scene: widgets are laid out by their layout nodes", "[Scene]" )
{
    ButtonSceneFixture fixture;

    REQUIRE_RECT( fixture.First->GetLayout().Layout.SceneRect,  10.f, 10.f, 100.f, 40.f );
    REQUIRE_RECT( fixture.Second->GetLayout().Layout.SceneRect, 10.f, 60.f, 100.f, 40.f );
}

TEST_CASE( "Scene: clicking a button invokes its callback", "[Scene]" )
{
    ButtonSceneFixture fixture;

    fixture.Click( { 50_u, 75_u } );
    REQUIRE( fixture.FirstClicks == 0 );
    REQUIRE( fixture.SecondClicks == 1 );

    fixture.Click( { 50_u, 25_u } );
    REQUIRE( fixture.FirstClicks == 1 );
    REQUIRE( fixture.SecondClicks == 1 );
}

TEST_CASE( "Scene: clicking empty space invokes nothing", "[Scene]" )
{
    ButtonSceneFixture fixture;

    fixture.Click( { 300_u, 250_u } );
    REQUIRE( fixture.FirstClicks == 0 );
    REQUIRE( fixture.SecondClicks == 0 );
}

TEST_CASE( "Scene: keyboard navigation focuses and activates buttons", "[Scene]" )
{
    ButtonSceneFixture fixture;
    REQUIRE( fixture.Scene.GetFocusedNode() == c_InvalidNodeID );

    fixture.Scene.EnsureInitialFocus();
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.First->GetLayoutID() );

    fixture.Scene.Navigate( ENavAction::MoveDown );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Second->GetLayoutID() );

    fixture.Scene.Navigate( ENavAction::ActivatePressed );
    fixture.Scene.Navigate( ENavAction::ActivateReleased );
    REQUIRE( fixture.FirstClicks == 0 );
    REQUIRE( fixture.SecondClicks == 1 );
}

TEST_CASE( "Scene: destroyed widgets are removed from the scene", "[Scene]" )
{
    ButtonSceneFixture fixture;
    const NodeID first = fixture.First->GetLayoutID();

    fixture.Scene.DestroyWidget( first );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );

    REQUIRE( fixture.Scene.GetWidget( first ) == nullptr );
    REQUIRE_RECT( fixture.Second->GetLayout().Layout.SceneRect, 10.f, 10.f, 100.f, 40.f );
}

TEST_CASE( "Scene: each pointer keeps its own capture, so two touches press two buttons at once", "[Scene]" )
{
    ButtonSceneFixture fixture;
    const Vec2<Unit> first{ 50_u, 25_u };
    const Vec2<Unit> second{ 50_u, 75_u };

    // One finger holds the first button while another taps the second.
    fixture.Touch( 1, first, true );
    REQUIRE( fixture.Scene.GetCapturedWidget( 1 ) == fixture.First->GetLayoutID() );

    fixture.Touch( 2, second, true );
    fixture.Touch( 2, second, false );
    REQUIRE( fixture.SecondClicks == 1 );
    REQUIRE( fixture.FirstClicks == 0 );
    REQUIRE( fixture.Scene.GetCapturedWidget( 1 ) == fixture.First->GetLayoutID() );
    REQUIRE( fixture.Scene.GetCapturedWidget( 2 ) == c_InvalidNodeID );

    fixture.Touch( 1, first, false );
    REQUIRE( fixture.FirstClicks == 1 );
    REQUIRE( fixture.Scene.GetCapturedWidget( 1 ) == c_InvalidNodeID );
}

TEST_CASE( "Scene: another pointer leaving a button doesn't cancel the press a touch holds on it", "[Scene]" )
{
    ButtonSceneFixture fixture;
    const Vec2<Unit> first{ 50_u, 25_u };

    fixture.MouseMove( first );
    fixture.Touch( 1, first, true );
    fixture.MouseMove( { 300_u, 250_u } );
    fixture.Touch( 1, first, false );
    REQUIRE( fixture.FirstClicks == 1 );
}

TEST_CASE( "Scene: touches don't hover", "[Scene]" )
{
    ButtonSceneFixture fixture;

    fixture.TouchMove( 1, { 50_u, 25_u } );
    REQUIRE_FALSE( fixture.First->IsHovered() );

    fixture.MouseMove( { 50_u, 25_u } );
    REQUIRE( fixture.First->IsHovered() );
}

TEST_CASE( "Scene: a lifted touch is forgotten once it can't continue a multi-press, and the mouse is kept", "[Scene]" )
{
    ButtonSceneFixture fixture;

    fixture.Click( { 50_u, 25_u } );
    fixture.Touch( 7, { 50_u, 75_u }, true );
    fixture.Touch( 7, { 50_u, 75_u }, false );
    REQUIRE( FindValue( fixture.Scene.Input.PointerStates, PointerID{ 7 } ) != nullptr );

    fixture.Scene.Tick( fixture.Scene.Input.Config.MultiPressWindowSeconds + 0.1 );
    REQUIRE( FindValue( fixture.Scene.Input.PointerStates, PointerID{ 7 } ) == nullptr );
    REQUIRE( FindValue( fixture.Scene.Input.PointerStates, c_MousePointerID ) != nullptr );
}

TEST_CASE( "Scene: destroying a widget frees the pointers it captured", "[Scene]" )
{
    ButtonSceneFixture fixture;
    const NodeID first = fixture.First->GetLayoutID();

    fixture.Touch( 1, { 50_u, 25_u }, true );
    REQUIRE( fixture.Scene.GetCapturedWidget( 1 ) == first );

    fixture.Scene.DestroyWidget( first );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );
    REQUIRE( fixture.Scene.GetCapturedWidget( 1 ) == c_InvalidNodeID );
}

TEST_CASE( "SliderWidget: a drag starts where the pointer presses and follows only that pointer", "[Scene]" )
{
    RatUI::Scene scene;
    PanelWidget* root = scene.CreateRootWidget<PanelWidget>();
    root->GetLayout().LayoutType( ELayoutType::Vertical ).Padding( Edges::All( 10_u ) );
    SliderWidget* slider = scene.CreateWidget<SliderWidget>( root->GetLayoutID(), 0.f, 1.f, 0.f );
    slider->GetLayout().FixedWidth( 200_u ).FixedHeight( 20_u );
    scene.UpdateLayout( { 400_u, 300_u } );

    const auto touch = [&]( PointerID a_Pointer, Vec2<Unit> a_Position, bool a_Pressed ) {
        scene.DispatchInputEvent( { .Device  = EDeviceID::Touch,
                                    .Payload = ButtonEvent{ .Button = EButtonID::MouseLeft, .Pressed = a_Pressed, .Released = !a_Pressed,
                                                            .Pointer = a_Pointer, .PointerPosition = a_Position } } );
    };
    const auto move = [&]( PointerID a_Pointer, Vec2<Unit> a_Position ) {
        scene.DispatchInputEvent( { .Device = EDeviceID::Touch, .Payload = PointerEvent{ .Position = a_Position, .Type = EPointerType::Touch, .ID = a_Pointer } } );
    };

    // Pressed three quarters along, with no move before the press, as touches arrive.
    touch( 1, { 160_u, 20_u }, true );
    const f32 pressed = slider->Value.Get();
    REQUIRE( pressed > 0.6f );

    // A second finger passing over the slider or lifting doesn't move or end the drag.
    move( 2, { 12_u, 20_u } );
    touch( 2, { 12_u, 20_u }, true );
    touch( 2, { 12_u, 20_u }, false );
    REQUIRE( slider->Value.Get() == pressed );

    move( 1, { 12_u, 20_u } );
    REQUIRE( slider->Value.Get() < 0.1f );

    touch( 1, { 12_u, 20_u }, false );
    REQUIRE( scene.GetCapturedWidget( 1 ) == c_InvalidNodeID );
}

namespace
{
    /** @brief A child panel of @p a_Parent that fills it. */
    PanelWidget* AddFillPanel( RatUI::Scene& a_Scene, NodeID a_Parent )
    {
        PanelWidget* panel = a_Scene.CreateWidget<PanelWidget>( a_Parent );
        panel->GetLayout().WidthMode( ESizing::Flex ).HeightMode( ESizing::Flex );
        return panel;
    }

    void RequireEdges( const Edges& a_Edges, f32 a_T, f32 a_R, f32 a_B, f32 a_L )
    {
        REQUIRE( a_Edges.T.ToFloat() == Catch::Approx( a_T ) );
        REQUIRE( a_Edges.R.ToFloat() == Catch::Approx( a_R ) );
        REQUIRE( a_Edges.B.ToFloat() == Catch::Approx( a_B ) );
        REQUIRE( a_Edges.L.ToFloat() == Catch::Approx( a_L ) );
    }
}

TEST_CASE( "Scene: a full-screen node keeps its children inside the safe area", "[Scene][SafeArea]" )
{
    ButtonSceneFixture fixture;
    PanelWidget* root = fixture.Scene.GetWidget<PanelWidget>( fixture.Scene.RootWidget );
    root->GetLayout().SafeAreaEdges( ESafeAreaEdges::All );

    // One UpdateLayout is enough, with nothing marked dirty: the cache is keyed on the safe area reaching each node.
    fixture.Scene.SetSafeArea( Edges{ .T = 50_u, .R = 0_u, .B = 30_u, .L = 20_u } );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );
    REQUIRE_RECT( fixture.First->GetLayout().Layout.SceneRect, 30.f, 60.f, 100.f, 40.f );
    RequireEdges( SafeAreaInsets( root->GetLayout() ), 50.f, 0.f, 30.f, 20.f );

    fixture.Scene.SetSafeArea( Edges{} );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );
    REQUIRE_RECT( fixture.First->GetLayout().Layout.SceneRect, 10.f, 10.f, 100.f, 40.f );
    RequireEdges( SafeAreaInsets( root->GetLayout() ), 0.f, 0.f, 0.f, 0.f );
}

TEST_CASE( "Scene: an opted-in node is inset by the full remaining safe area wherever it sits", "[Scene][SafeArea]" )
{
    RatUI::Scene scene;
    PanelWidget* root = scene.CreateRootWidget<PanelWidget>();
    root->GetLayout().LayoutType( ELayoutType::Vertical );

    PanelWidget* spacer = scene.CreateWidget<PanelWidget>( root->GetLayoutID() );
    spacer->GetLayout().FixedWidth( 10_u ).FixedHeight( 100_u );

    PanelWidget* panel = scene.CreateWidget<PanelWidget>( root->GetLayoutID() );
    panel->GetLayout().FixedWidth( 200_u ).FixedHeight( 100_u ).SafeAreaEdges( ESafeAreaEdges::Top );
    ButtonWidget* button = scene.CreateWidget<ButtonWidget>( panel->GetLayoutID(), []( ButtonBaseWidget& ) {} );
    button->GetLayout().FixedWidth( 50_u ).FixedHeight( 20_u );

    // The panel starts below the 50-unit notch band, but the inset doesn't depend on position.
    scene.SetSafeArea( Edges{ .T = 50_u } );
    scene.UpdateLayout( { 400_u, 300_u } );
    RequireEdges( SafeAreaInsets( panel->GetLayout() ), 50.f, 0.f, 0.f, 0.f );
    REQUIRE_RECT( button->GetLayout().Layout.SceneRect, 0.f, 150.f, 50.f, 20.f );
}

TEST_CASE( "Scene: nested safe-area nodes don't inset twice", "[Scene][SafeArea]" )
{
    RatUI::Scene scene;
    PanelWidget* root = scene.CreateRootWidget<PanelWidget>();
    root->GetLayout().SafeAreaEdges( ESafeAreaEdges::All );
    PanelWidget* inner = AddFillPanel( scene, root->GetLayoutID() );
    inner->GetLayout().SafeAreaEdges( ESafeAreaEdges::All );
    ButtonWidget* button = scene.CreateWidget<ButtonWidget>( inner->GetLayoutID(), []( ButtonBaseWidget& ) {} );
    button->GetLayout().FixedWidth( 50_u ).FixedHeight( 20_u );

    scene.SetSafeArea( Edges{ .T = 50_u, .L = 20_u } );
    scene.UpdateLayout( { 400_u, 300_u } );

    REQUIRE_RECT( inner->GetLayout().Layout.SceneRect, 20.f, 50.f, 380.f, 250.f );
    RequireEdges( SafeAreaInsets( inner->GetLayout() ), 0.f, 0.f, 0.f, 0.f );
    REQUIRE_RECT( button->GetLayout().Layout.SceneRect, 20.f, 50.f, 50.f, 20.f );
}

TEST_CASE( "Scene: a node only clears the safe area on the edges it asks for", "[Scene][SafeArea]" )
{
    ButtonSceneFixture fixture;
    PanelWidget* root = fixture.Scene.GetWidget<PanelWidget>( fixture.Scene.RootWidget );
    root->GetLayout().SafeAreaEdges( ESafeAreaEdges::Bottom );

    fixture.Scene.SetSafeArea( Edges::All( 40_u ) );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );

    RequireEdges( SafeAreaInsets( root->GetLayout() ), 0.f, 0.f, 40.f, 0.f );
    REQUIRE_RECT( fixture.First->GetLayout().Layout.SceneRect, 10.f, 10.f, 100.f, 40.f );
}

TEST_CASE( "Scene: a full-screen background beside the inset content reaches the screen's edges", "[Scene][SafeArea]" )
{
    RatUI::Scene scene;
    PanelWidget* root = scene.CreateRootWidget<PanelWidget>();
    PanelWidget* background = AddFillPanel( scene, root->GetLayoutID() );
    PanelWidget* content = AddFillPanel( scene, root->GetLayoutID() );
    content->GetLayout().SafeAreaEdges( ESafeAreaEdges::All );
    ButtonWidget* button = scene.CreateWidget<ButtonWidget>( content->GetLayoutID(), []( ButtonBaseWidget& ) {} );
    button->GetLayout().FixedWidth( 50_u ).FixedHeight( 20_u );

    scene.SetSafeArea( Edges::All( 20_u ) );
    scene.UpdateLayout( { 400_u, 300_u } );

    // The node that clears the safe area keeps its full rect, so its own fill covers the unsafe area too.
    REQUIRE_RECT( background->GetLayout().Layout.SceneRect, 0.f, 0.f, 400.f, 300.f );
    REQUIRE_RECT( content->GetLayout().Layout.SceneRect, 0.f, 0.f, 400.f, 300.f );
    REQUIRE_RECT( button->GetLayout().Layout.SceneRect, 20.f, 20.f, 50.f, 20.f );
}

TEST_CASE( "Scene: the safe area leaves nodes that don't ask for it alone", "[Scene][SafeArea]" )
{
    ButtonSceneFixture fixture;

    fixture.Scene.SetSafeArea( Edges::All( 40_u ) );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );
    REQUIRE_RECT( fixture.First->GetLayout().Layout.SceneRect, 10.f, 10.f, 100.f, 40.f );
}

TEST_CASE( "Scene: a pen that Tick has forgotten doesn't hover what layout moves under it", "[Scene]" )
{
    ButtonSceneFixture fixture;
    const Vec2<Unit>   second{ 50_u, 75_u };
    const auto         pen = [&]( auto a_Payload ) { fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Pen, .Payload = a_Payload } ); };

    // A pen hovers the second button, taps it, then lifts away for good.
    pen( PointerEvent{ .Position = second, .Type = EPointerType::Pen, .ID = 7 } );
    REQUIRE( fixture.Second->IsHovered() );
    pen( ButtonEvent{ .Button = EButtonID::MouseLeft, .Pressed = true, .Pointer = PointerID{ 7 }, .PointerPosition = second } );
    pen( ButtonEvent{ .Button = EButtonID::MouseLeft, .Released = true, .Pointer = PointerID{ 7 }, .PointerPosition = second } );
    fixture.Second->GetLayout().Visibility( EVisibility::Hidden ); // So hover isn't left over from the pen.
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );

    fixture.Scene.Tick( fixture.Scene.Input.Config.MultiPressWindowSeconds + 0.1 );
    REQUIRE( FindValue( fixture.Scene.Input.PointerStates, PointerID{ 7 } ) == nullptr );

    // Layout moves the first button under where the pen was: nothing hovers it.
    fixture.First->GetLayout().Padding( Edges{ .T = 50_u } ).FixedHeight( 90_u );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );
    REQUIRE( fixture.First->GetLayout().Layout.SceneRect.Contains( second ) );
    REQUIRE_FALSE( fixture.First->IsHovered() );
    REQUIRE( FindValue( fixture.Scene.Input.PointerStates, PointerID{ 7 } ) == nullptr );
}

namespace
{
    /** @brief A button that counts the pointer moves it is sent. */
    class MoveCountingButton : public ButtonWidget
    {
    public:
        int Moves{ 0 };

        Reply OnPointerMove( const PointerEvent& a_Event ) override
        {
            ++Moves;
            return ButtonWidget::OnPointerMove( a_Event );
        }
    };
}

TEST_CASE( "Scene: hover follows layout moving widgets under a still mouse, without a pointer move", "[Scene]" )
{
    RatUI::Scene scene;
    PanelWidget* root = scene.CreateRootWidget<PanelWidget>();
    root->GetLayout().LayoutType( ELayoutType::Vertical );

    MoveCountingButton* first  = scene.CreateWidget<MoveCountingButton>( root->GetLayoutID() );
    MoveCountingButton* second = scene.CreateWidget<MoveCountingButton>( root->GetLayoutID() );
    first->GetLayout().FixedWidth( 100_u ).FixedHeight( 40_u );
    second->GetLayout().FixedWidth( 100_u ).FixedHeight( 40_u );
    scene.UpdateLayout( { 400_u, 300_u } );

    scene.DispatchInputEvent( { .Device = EDeviceID::Mouse, .Payload = PointerEvent{ .Position = { 50_u, 60_u }, .Type = EPointerType::Mouse } } );
    REQUIRE( second->IsHovered() );
    const int movesBefore = second->Moves;

    // The first button collapses, so the second moves up from under the mouse, which now points at nothing.
    first->GetLayout().Visibility( EVisibility::Collapsed );
    scene.UpdateLayout( { 400_u, 300_u } );
    REQUIRE( !second->IsHovered() );
    REQUIRE( second->Moves == movesBefore );

    // Back again, and keyboard focus stays visible: a layout isn't the user reaching for the mouse.
    first->GetLayout().Visibility( EVisibility::Visible );
    scene.Input.InputMode = EInputMode::Navigation;
    scene.UpdateLayout( { 400_u, 300_u } );
    REQUIRE( second->IsHovered() );
    REQUIRE( second->Moves == movesBefore );
    REQUIRE( scene.GetInputMode() == EInputMode::Navigation );
}

TEST_CASE( "IWidget: the focus policy decides focus, and making a focused widget unfocusable clears focus", "[Scene][Navigation]" )
{
    ButtonSceneFixture fixture;
    REQUIRE( fixture.First->IsNavStop() );

    fixture.Scene.SetFocus( fixture.First->GetLayoutID() );
    fixture.First->SetFocusPolicy( EFocusPolicy::None );
    REQUIRE( fixture.Scene.GetFocusedNode() == c_InvalidNodeID );

    // Navigation now skips it.
    fixture.Scene.Input.NavMap.BindDefaultDesktop();
    fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = ButtonEvent{ .Button = EButtonID::KeyTab, .Pressed = true } } );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Second->GetLayoutID() );
}

TEST_CASE( "Scene: pressing focuses, before the release", "[Scene]" )
{
    ButtonSceneFixture fixture;
    const auto press = [&]( Vec2<Unit> a_Position, bool a_Pressed )
    {
        fixture.Scene.DispatchInputEvent( { .Device  = EDeviceID::Mouse,
                                            .Payload = ButtonEvent{ .Button = EButtonID::MouseLeft, .Pressed = a_Pressed, .Released = !a_Pressed,
                                                                    .PointerPosition = a_Position } } );
    };

    press( { 20_u, 20_u }, true );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.First->GetLayoutID() );
    REQUIRE( fixture.Scene.GetInputMode() == EInputMode::Pointer );
    press( { 20_u, 20_u }, false );

    // Empty space clears focus on press too.
    press( { 300_u, 250_u }, true );
    REQUIRE( fixture.Scene.GetFocusedNode() == c_InvalidNodeID );
    press( { 300_u, 250_u }, false );
}

#include <RatUI/Widget/TextWidget.h>

TEST_CASE( "TextClipRect: text that fits isn't clipped, so it batches with its neighbours", "[Text][Clip]" )
{
    const Rect<Unit> node{ Vec2<Unit>{ 10_u, 10_u }, Vec2<Unit>{ 100_u, 20_u } };
    REQUIRE_FALSE( TextClipRect( node, node, { 80_u, 20_u }, 20_u ).has_value() );
}

TEST_CASE( "TextClipRect: clips at the node's edge only on the axis the text overflows", "[Text][Clip]" )
{
    const Rect<Unit> node{ Vec2<Unit>{ 10_u, 10_u }, Vec2<Unit>{ 100_u, 40_u } };

    // Too tall: cut at the top and bottom edges, a line's room left and right for overhanging glyphs.
    const Optional<Rect<Unit>> tall = TextClipRect( node, node, { 80_u, 120_u }, 20_u );
    REQUIRE( tall.has_value() );
    REQUIRE_RECT( *tall, -10.f, 10.f, 140.f, 40.f );

    // Too wide: cut at the left and right edges, room above and below for ascenders and descenders.
    const Optional<Rect<Unit>> wide = TextClipRect( node, node, { 300_u, 20_u }, 20_u );
    REQUIRE( wide.has_value() );
    REQUIRE_RECT( *wide, 10.f, -10.f, 100.f, 80.f );
}

// ============================================================================
// Layout callbacks and skipping
// ============================================================================

TEST_CASE( "Scene: OnGeometryChanged is called for widgets that ask, when layout changes their size", "[Scene]" )
{
    NavFixture   f;
    ProbeCounts  plain, asked;
    f.Scene.CreateWidget<ProbeWidget>( f.Root->GetLayoutID(), plain );
    ProbeWidget* probe = f.Scene.CreateWidget<ProbeWidget>( f.Root->GetLayoutID(), asked );
    probe->GetLayout().NotifyGeometry = true;

    GeometryEvent last;
    probe->OnGeometry = [&]( const GeometryEvent& a_Event ) { last = a_Event; };
    f.Layout();

    REQUIRE( plain.GeometryChanges == 0 );
    REQUIRE( asked.GeometryChanges == 1 );
    REQUIRE_VEC2( last.PreviousSize, 0.f, 0.f );
    REQUIRE_VEC2( last.Size, 50.f, 20.f );

    // Laid out again without changing: nothing to report.
    probe->GetLayout().MarkDirty();
    f.Layout();
    REQUIRE( asked.GeometryChanges == 1 );

    probe->GetLayout().FixedWidth( 80_u );
    f.Layout();
    REQUIRE( asked.GeometryChanges == 2 );
    REQUIRE_VEC2( last.PreviousSize, 50.f, 20.f );
    REQUIRE_VEC2( last.Size, 80.f, 20.f );
}

TEST_CASE( "Scene: a layout change made in OnGeometryChanged is laid out before UpdateLayout returns", "[Scene]" )
{
    NavFixture    f;
    ProbeCounts   counts;
    ProbeWidget*  probe  = f.Scene.CreateWidget<ProbeWidget>( f.Root->GetLayoutID(), counts );
    probe->GetLayout().NotifyGeometry = true;
    ButtonWidget* button = f.AddButton( f.Root->GetLayoutID() );

    // The button is kept as tall as the probe is wide.
    probe->OnGeometry = [&]( const GeometryEvent& a_Event ) { button->GetLayout().FixedHeight( a_Event.Size[0] ); };
    f.Layout();
    REQUIRE_UNIT( button->GetLayout().Layout.SceneRect.Size[1], 50.f );

    probe->GetLayout().FixedWidth( 70_u );
    f.Layout();
    REQUIRE_UNIT( button->GetLayout().Layout.SceneRect.Size[1], 70.f );
}

TEST_CASE( "Scene: layout only runs when something changed", "[Scene]" )
{
    NavFixture    f;
    ProbeCounts   counts;
    ProbeWidget*  probe  = f.Scene.CreateWidget<ProbeWidget>( f.Root->GetLayoutID(), counts );
    ButtonWidget* first  = f.AddButton( f.Root->GetLayoutID() );
    ButtonWidget* second = f.AddButton( f.Root->GetLayoutID() );
    f.Layout();
    REQUIRE( counts.Measures == 1 );

    f.Layout();
    REQUIRE( counts.Measures == 1 );

    SECTION( "a dirty node" )
    {
        probe->GetLayout().MarkDirty();
        f.Layout();
        REQUIRE( counts.Measures == 2 );
    }

    SECTION( "a new size" )
    {
        f.Scene.UpdateLayout( { 500_u, 300_u } );
        REQUIRE( counts.Measures == 2 );
    }

    SECTION( "a new safe area" )
    {
        f.Scene.SetSafeArea( Edges::All( 5_u ) );
        f.Layout();
        REQUIRE( counts.Measures == 2 );
    }

    SECTION( "a widget added" )
    {
        ButtonWidget* added = f.AddButton( f.Root->GetLayoutID() );
        f.Layout();
        REQUIRE_VEC2( added->GetLayout().Layout.SceneRect.Size, 100.f, 40.f );
        REQUIRE( added->GetLayout().Layout.SceneRect.Top() > second->GetLayout().Layout.SceneRect.Top() );
    }

    SECTION( "a widget removed" )
    {
        const Unit top = second->GetLayout().Layout.SceneRect.Top();
        f.Scene.DestroyWidget( first->GetLayoutID() );
        f.Layout();
        REQUIRE( second->GetLayout().Layout.SceneRect.Top() < top );
    }

    SECTION( "nothing for a content offset" )
    {
        f.Scene.SetContentOffset( f.Root->GetLayoutID(), { 0_u, -5_u } );
        const Unit top = first->GetLayout().Layout.SceneRect.Top();
        f.Layout();
        REQUIRE( counts.Measures == 1 );
        REQUIRE( first->GetLayout().Layout.SceneRect.Top() == top );
    }
}

TEST_CASE( "Scene: a root other than the one last laid out is laid out, even at the same size", "[Scene]" )
{
    RatUI::Scene scene;
    PanelWidget* first  = scene.CreateRootWidget<PanelWidget>();
    PanelWidget* second = scene.CreateRootWidget<PanelWidget>();
    scene.UpdateLayout( { 800_u, 600_u } );

    scene.RootWidget = first->GetLayoutID();
    scene.UpdateLayout( { 800_u, 600_u } );
    scene.UpdateLayout( { 1280_u, 720_u } );

    // The second root was last laid out at the old size, and nothing in it is dirty.
    scene.RootWidget = second->GetLayoutID();
    scene.UpdateLayout( { 1280_u, 720_u } );
    REQUIRE_VEC2( second->GetLayout().Layout.SceneRect.Size, 1280.f, 720.f );
}

// ============================================================================
// Widget lifetime and pointers
// ============================================================================

TEST_CASE( "Scene: destroying the focused, engaged widget tells it and leaves no focus behind", "[Scene]" )
{
    NavFixture   f;
    ProbeCounts  counts;
    ProbeWidget* probe = f.Scene.CreateWidget<ProbeWidget>( f.Root->GetLayoutID(), counts,
                                                           WidgetTraits{ .Focus = EFocusPolicy::All, .Interactable = true, .Engageable = true } );
    const NodeID id = probe->GetLayoutID();
    f.Layout();

    f.Scene.Engage( id );
    REQUIRE( f.Scene.IsEngaged( id ) );

    f.Scene.DestroyWidget( id );
    f.Layout();
    REQUIRE( counts.Disengaged == 1 );
    REQUIRE( counts.FocusLost == 1 );
    REQUIRE( f.Scene.GetFocusedNode() == c_InvalidNodeID );
    REQUIRE( f.Scene.Input.EngagedWidget == c_InvalidNodeID );
}

TEST_CASE( "Scene: destroying an entered group leaves its nav scope", "[Scene]" )
{
    NavFixture   f;
    PanelWidget* group = f.Scene.CreateWidget<PanelWidget>( f.Root->GetLayoutID() );
    group->SetNavGroup( true );
    ButtonWidget* inner = f.AddButton( group->GetLayoutID() );
    f.Layout();

    f.Scene.SetFocus( inner->GetLayoutID() );
    REQUIRE( f.Scene.GetCurrentNavScope() == group->GetLayoutID() );

    f.Scene.DestroyWidget( group->GetLayoutID() );
    f.Layout();
    REQUIRE( f.Scene.GetCurrentNavScope() == f.Root->GetLayoutID() );
    REQUIRE( f.Scene.GetFocusedNode() == c_InvalidNodeID );
    REQUIRE( f.Scene.Input.NavStack.empty() );
}

TEST_CASE( "Scene: another button pressed during a drag doesn't take the pointer from the widget being dragged", "[Scene]" )
{
    NavFixture    f;
    SliderWidget* slider = f.Scene.CreateWidget<SliderWidget>( f.Root->GetLayoutID(), 0.f, 1.f, 0.f );
    slider->GetLayout().FixedWidth( 200_u ).FixedHeight( 20_u );
    f.AddButton( f.Root->GetLayoutID() );
    f.Layout();

    const auto button = [&]( EButtonID a_Button, bool a_Pressed, Vec2<Unit> a_Position )
    {
        f.Scene.DispatchInputEvent( { .Device  = EDeviceID::Mouse,
                                      .Payload = ButtonEvent{ .Button = a_Button, .Pressed = a_Pressed, .Released = !a_Pressed,
                                                              .Pointer = c_MousePointerID, .PointerPosition = a_Position } } );
    };
    const auto move = [&]( Vec2<Unit> a_Position )
    {
        f.Scene.DispatchInputEvent( { .Device = EDeviceID::Mouse, .Payload = PointerEvent{ .Position = a_Position, .Type = EPointerType::Mouse } } );
    };

    const Vec2<Unit> onSlider{ 60_u, 20_u };
    const Vec2<Unit> onButton{ 50_u, 60_u };

    move( onSlider );
    button( EButtonID::MouseLeft, true, onSlider );
    REQUIRE( f.Scene.GetCapturedWidget( c_MousePointerID ) == slider->GetLayoutID() );

    move( onButton );
    button( EButtonID::MouseRight, true, onButton );
    button( EButtonID::MouseRight, false, onButton );
    REQUIRE( f.Scene.GetCapturedWidget( c_MousePointerID ) == slider->GetLayoutID() );

    // The left release ends the drag: the slider no longer follows the pointer.
    button( EButtonID::MouseLeft, false, onButton );
    REQUIRE( f.Scene.GetCapturedWidget( c_MousePointerID ) == c_InvalidNodeID );
    const f32 value = slider->Value.Get();
    move( { 190_u, 20_u } );
    REQUIRE( slider->Value.Get() == value );
}

TEST_CASE( "Scene: the text input rect is the caret of the field being edited", "[Scene]" )
{
    NavFixture       f;
    InputTextWidget* field = f.Scene.CreateWidget<InputTextWidget>( f.Root->GetLayoutID() );
    f.Layout();

    f.Scene.SetFocus( field->GetLayoutID() );
    REQUIRE_FALSE( HasValue( f.Scene.GetTextInputRect() ) );

    f.Scene.Engage( field->GetLayoutID() );
    const Optional<Rect<Unit>> rect = f.Scene.GetTextInputRect();
    REQUIRE( HasValue( rect ) );
    REQUIRE( field->GetLayout().Layout.SceneRect.Contains( rect->Origin ) );

    f.Scene.Disengage();
    REQUIRE_FALSE( HasValue( f.Scene.GetTextInputRect() ) );
}

// ============================================================================
// Pressing and releasing off a button
// ============================================================================

TEST_CASE( "Scene: dragging off a pressed button and releasing doesn't click it", "[Scene]" )
{
    ButtonSceneFixture fixture;
    const Vec2<Unit>   on{ 50_u, 25_u };
    const Vec2<Unit>   off{ 300_u, 250_u };

    SECTION( "Mouse" )
    {
        fixture.MouseMove( on );
        fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Mouse, .Payload = ButtonEvent{ .Button = EButtonID::MouseLeft, .Pressed = true, .Pointer = c_MousePointerID, .PointerPosition = on } } );
        REQUIRE( fixture.First->IsPressed() );

        fixture.MouseMove( off );
        REQUIRE_FALSE( fixture.First->IsPressed() );
        REQUIRE_FALSE( fixture.First->IsHovered() );

        fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Mouse, .Payload = ButtonEvent{ .Button = EButtonID::MouseLeft, .Released = true, .Pointer = c_MousePointerID, .PointerPosition = off } } );
        REQUIRE( fixture.FirstClicks == 0 );
    }

    SECTION( "A touch lifted somewhere else, without a move in between" )
    {
        fixture.Touch( 1, on, true );
        fixture.Touch( 1, off, false );
        REQUIRE( fixture.FirstClicks == 0 );
        REQUIRE( fixture.Scene.GetCapturedWidget( 1 ) == c_InvalidNodeID );
    }

    SECTION( "Moving within the button keeps the press" )
    {
        fixture.Touch( 1, on, true );
        fixture.TouchMove( 1, { 90_u, 45_u } );
        fixture.Touch( 1, { 90_u, 45_u }, false );
        REQUIRE( fixture.FirstClicks == 1 );
    }
}

// ============================================================================
// Hit testing
// ============================================================================

TEST_CASE( "Scene: a widget that isn't hit-testable or is hidden takes no clicks but keeps its place", "[Scene]" )
{
    ButtonSceneFixture fixture;

    SECTION( "Not hit-testable" ) { fixture.First->GetLayout().HitTestable( false ); }
    SECTION( "Hidden" )           { fixture.First->GetLayout().Visibility( EVisibility::Hidden ); }

    fixture.Scene.UpdateLayout( { 400_u, 300_u } );
    fixture.Click( { 50_u, 25_u } );
    REQUIRE( fixture.FirstClicks == 0 );
    REQUIRE_UNIT( fixture.Second->GetLayout().Layout.SceneRect.Origin[1], 60.f );

    fixture.Click( { 50_u, 75_u } );
    REQUIRE( fixture.SecondClicks == 1 );
}

TEST_CASE( "Scene: a collapsed widget takes no clicks and the next one moves up", "[Scene]" )
{
    ButtonSceneFixture fixture;
    fixture.First->GetLayout().Visibility( EVisibility::Collapsed );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );

    fixture.Click( { 50_u, 25_u } );
    REQUIRE( fixture.FirstClicks == 0 );
    REQUIRE( fixture.SecondClicks == 1 );
}

// ============================================================================
// Focus rings
// ============================================================================

TEST_CASE( "Scene: a focus ring shows after keyboard navigation, not after a click", "[Scene][Navigation]" )
{
    NavFixture    f;
    ButtonWidget* a = f.AddButton( f.Root->GetLayoutID() );
    ButtonWidget* b = f.AddButton( f.Root->GetLayoutID() );
    f.Layout();

    f.Click( { 50_u, 25_u } );
    REQUIRE( f.Scene.GetFocusedNode() == a->GetLayoutID() );
    REQUIRE_FALSE( f.Scene.IsFocusVisible( a->GetLayoutID() ) );

    f.Tap( EButtonID::KeyDown );
    REQUIRE( f.Scene.GetFocusedNode() == b->GetLayoutID() );
    REQUIRE( f.Scene.IsFocusVisible( b->GetLayoutID() ) );
    REQUIRE_FALSE( f.Scene.IsFocusVisible( a->GetLayoutID() ) );

    // Using the mouse again hides it.
    f.Click( { 50_u, 75_u } );
    REQUIRE_FALSE( f.Scene.IsFocusVisible( b->GetLayoutID() ) );
}

// ============================================================================
// Layout passes
// ============================================================================

namespace
{
    /** @brief Height 1000 / width, like wrapped text. */
    struct SceneWrappingProbe : IWidget
    {
        SceneWrappingProbe() : IWidget( { .WidthDependentContent = true } ) {}

        Vec2<Unit> OnMeasureContent( const LayoutNode&, Vec2<Unit> a_Available, const LayoutContext& ) override
        {
            const f32 width = std::max( 1.f, std::min( a_Available[0].ToFloat(), 100.f ) );
            return { Unit{ width }, Unit{ 1000.f / width } };
        }
    };

    /** @brief Never settles: each measure asks for a different height than the last. */
    struct RestlessProbe : IWidget
    {
        RestlessProbe() : IWidget( { .WidthDependentContent = true } ) {}
        f32 Height{ 10.f };

        Vec2<Unit> OnMeasureContent( const LayoutNode&, Vec2<Unit>, const LayoutContext& ) override
        {
            Height = Height == 10.f ? 20.f : 10.f;
            return { 50_u, Unit{ Height } };
        }
    };
}

TEST_CASE( "Scene: content that grows at its final width grows the content-sized panels around it", "[Scene]" )
{
    NavFixture   f;
    // Measured 100 wide (10 tall), arranged at its 40 max width (25 tall).
    PanelWidget* panel = f.Scene.CreateWidget<PanelWidget>( f.Root->GetLayoutID() );
    panel->GetLayout().LayoutType( ELayoutType::Vertical ).FlexWidth();
    SceneWrappingProbe* wrapping = f.Scene.CreateWidget<SceneWrappingProbe>( panel->GetLayoutID() );
    wrapping->GetLayout().FlexWidth().SizeConstraints( Constraints{ .Max = { 40_u, Limits<Unit>::max() } } );
    ButtonWidget* below = f.AddButton( f.Root->GetLayoutID() );

    f.Layout();
    REQUIRE_UNIT( wrapping->GetLayout().Layout.SceneRect.Size[1], 25.f );
    REQUIRE_UNIT( panel->GetLayout().Layout.SceneRect.Size[1], 25.f );
    REQUIRE_UNIT( below->GetLayout().Layout.SceneRect.Origin[1], 10.f + 25.f + 10.f );
    REQUIRE_FALSE( f.Scene.IsLayoutPending() );
}

TEST_CASE( "Scene: layout that never settles stops after a few passes and carries on next time", "[Scene]" )
{
    NavFixture f;
    f.Scene.CreateWidget<RestlessProbe>( f.Root->GetLayoutID() );

    f.Layout();
    const u64 first = f.Scene.GetLayoutCount();
    f.Layout(); // Same size, nothing dirty: laid out again, as the last layout didn't finish.
    REQUIRE( f.Scene.GetLayoutCount() > first );
}

TEST_CASE( "Scene: a collapsed root is laid out once, then left alone", "[Scene]" )
{
    RatUI::Scene scene;
    PanelWidget* root = scene.CreateRootWidget<PanelWidget>();
    scene.CreateWidget<PanelWidget>( root->GetLayoutID() );
    root->GetLayout().Visibility( EVisibility::Collapsed );

    scene.UpdateLayout( { 100_u, 100_u } );
    const u64 count = scene.GetLayoutCount();
    REQUIRE_FALSE( scene.IsLayoutPending() );

    scene.UpdateLayout( { 100_u, 100_u } );
    REQUIRE( scene.GetLayoutCount() == count );

    root->GetLayout().Visibility( EVisibility::Visible );
    REQUIRE( scene.IsLayoutPending() );
    scene.UpdateLayout( { 100_u, 100_u } );
    REQUIRE( scene.GetLayoutCount() > count );
}

// ============================================================================
// Geometry events
// ============================================================================

TEST_CASE( "Scene: a watched node without a widget is reported to the widget it is inside", "[Scene]" )
{
    NavFixture   f;
    ProbeCounts  counts;
    ProbeWidget* probe = f.Scene.CreateWidget<ProbeWidget>( f.Root->GetLayoutID(), counts );
    LayoutNode&  inner = f.Scene.CreateLayoutNode( {}, probe->GetLayoutID() );
    inner.FixedWidth( 30_u ).FixedHeight( 10_u );
    inner.NotifyGeometry = true;

    GeometryEvent last;
    probe->OnGeometry = [&]( const GeometryEvent& a_Event ) { last = a_Event; };
    f.Layout();

    REQUIRE( counts.GeometryChanges == 1 );
    REQUIRE_VEC2( last.Size, 30.f, 10.f );
}

TEST_CASE( "Scene: a content size change is reported with what it was", "[Scene]" )
{
    NavFixture   f;
    ProbeCounts  counts;
    ProbeWidget* probe = f.Scene.CreateWidget<ProbeWidget>( f.Root->GetLayoutID(), counts );
    probe->GetLayout().FixedWidth( 100_u ).FixedHeight( 100_u ).NotifyGeometry = true;
    LayoutNode& inner = f.Scene.CreateLayoutNode( {}, probe->GetLayoutID() );
    inner.FixedWidth( 10_u ).FixedHeight( 10_u );

    GeometryEvent last;
    probe->OnGeometry = [&]( const GeometryEvent& a_Event ) { last = a_Event; };
    f.Layout();

    inner.FixedHeight( 150_u );
    f.Layout();
    REQUIRE( counts.GeometryChanges == 2 );
    REQUIRE_VEC2( last.Size, 100.f, 100.f );
    REQUIRE_VEC2( last.PreviousSize, 100.f, 100.f );
    REQUIRE_VEC2( last.PreviousContentSize, 10.f, 10.f );
    REQUIRE_VEC2( last.ContentSize, 10.f, 150.f );
}

TEST_CASE( "Scene: a node that changes in several passes of one layout is reported once, against where it started", "[Scene]" )
{
    NavFixture   f;
    ProbeCounts  counts;
    ProbeWidget* probe = f.Scene.CreateWidget<ProbeWidget>( f.Root->GetLayoutID(), counts );
    probe->GetLayout().LayoutType( ELayoutType::Vertical ).FixedWidth( 50_u ).NotifyGeometry = true;
    f.Scene.CreateWidget<SceneWrappingProbe>( probe->GetLayoutID() )->GetLayout().FlexWidth().SizeConstraints( Constraints{ .Max = { 40_u, Limits<Unit>::max() } } );

    GeometryEvent last;
    probe->OnGeometry = [&]( const GeometryEvent& a_Event ) { last = a_Event; };
    f.Layout();

    // The content reflows at its 40 max width: two passes, one report.
    REQUIRE( counts.GeometryChanges == 1 );
    REQUIRE_VEC2( last.PreviousSize, 0.f, 0.f );
    REQUIRE( last.Size == probe->GetLayout().Layout.ParentRect.Size );
}

TEST_CASE( "Scene: destroying a widget destroys every descendant, widgets and bare nodes alike", "[Scene]" )
{
    NavFixture   f;
    PanelWidget* outer = f.Scene.CreateWidget<PanelWidget>( f.Root->GetLayoutID() );
    PanelWidget* inner = f.Scene.CreateWidget<PanelWidget>( outer->GetLayoutID() );
    f.AddButton( inner->GetLayoutID() );
    f.Scene.CreateLayoutNode( {}, inner->GetLayoutID() );
    f.Layout();

    Array<NodeID> descendants;
    outer->GetLayout().ForEachDescendant( [&]( LayoutNode& a_Node ) { PushBack( descendants, a_Node.ID ); } );
    REQUIRE( Size( descendants ) == 3 );

    const NodeID outerID = outer->GetLayoutID();
    f.Scene.DestroyWidget( outerID );
    f.Layout();

    REQUIRE( f.Scene.GetLayoutNode( outerID ) == nullptr );
    for ( NodeID id : descendants )
        REQUIRE( f.Scene.GetLayoutNode( id ) == nullptr );
    REQUIRE( f.Root->GetLayout().ChildCount() == 0 );
}
