#include "TestLayoutCommon.h"
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

    REQUIRE_RECT( fixture.First->GetLayout().Layout.FinalRect,  10.f, 10.f, 100.f, 40.f );
    REQUIRE_RECT( fixture.Second->GetLayout().Layout.FinalRect, 10.f, 60.f, 100.f, 40.f );
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
    REQUIRE_RECT( fixture.Second->GetLayout().Layout.FinalRect, 10.f, 10.f, 100.f, 40.f );
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
