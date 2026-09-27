#include "TestLayoutCommon.h"
#include <RatUI/Widget/ButtonWidget.h>
#include <RatUI/Widget/PanelWidget.h>

/**
 * @brief A vertical root panel with two fixed-size buttons that count their clicks.
 */
struct ButtonSceneFixture
{
    Scene         Scene;
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
