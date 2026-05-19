/**
 * @file TestScrollContainerWidget.cpp
 * @brief Tests for RatUI::ScrollContainerWidget input and clamping behavior.
 */

#include "../Common/Common.h"
#include <RatUI/Widget/ScrollContainerWidget.h>

using namespace RatUI;

class DummyWidget : public IWidget {};

static InputEvent MakeMouseMove( Vec2f a_Pos )
{
    return InputEvent{
        .Device  = EDeviceID::Mouse,
        .Payload = PointerEvent{
            .Position = ToUnitVec2( a_Pos ),
            .Type = EPointerType::Mouse
        }
    };
}

static InputEvent MakeMouseScroll( Vec2f a_Pos, Vec2f a_Scroll )
{
    return InputEvent{
        .Device  = EDeviceID::Mouse,
        .Payload = PointerEvent{
            .Position = ToUnitVec2( a_Pos ),
            .ScrollDelta = ToUnitVec2( a_Scroll ),
            .Type = EPointerType::Mouse
        }
    };
}

TEST_CASE( "ScrollContainerWidget consumes wheel scroll and clamps to content bounds", "[widget][scroll]" )
{
    Scene scene;
    WidgetID rootID = scene.CreateRootWidget<ScrollContainerWidget>();
    auto* scroll = scene.GetWidget<ScrollContainerWidget>( rootID );
    REQUIRE( scroll != nullptr );

    LayoutNode* rootNode = scene.Layouts.Get( scene.GetWidget( rootID )->GetLayoutID() );
    REQUIRE( rootNode != nullptr );
    rootNode->Style.WidthMode = ESizingMode::Fixed;
    rootNode->Style.HeightMode = ESizingMode::Fixed;
    rootNode->Style.FixedWidth = 100_u;
    rootNode->Style.FixedHeight = 100_u;

    WidgetID childID = scene.CreateWidget<DummyWidget>( rootID );
    LayoutNode* childNode = scene.Layouts.Get( scene.GetWidget( childID )->GetLayoutID() );
    REQUIRE( childNode != nullptr );
    childNode->Style.WidthMode = ESizingMode::Fixed;
    childNode->Style.HeightMode = ESizingMode::Fixed;
    childNode->Style.FixedWidth = 100_u;
    childNode->Style.FixedHeight = 300_u;

    scene.UpdateLayout( Vec2<Unit>{ 100_u, 100_u } );
    scene.DispatchInputEvent( MakeMouseMove( Vec2f{ 10.f, 10.f } ) );
    scene.DispatchInputEvent( MakeMouseScroll( Vec2f{ 10.f, 10.f }, Vec2f{ 0.f, -1.f } ) );

    REQUIRE( scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 48.f ).epsilon( 1e-5f ) );
    REQUIRE( scroll->GetMaxScrollOffset()[1].ToFloat() == Catch::Approx( 200.f ).epsilon( 1e-5f ) );

    scroll->SetScrollOffset( scene, Vec2<Unit>{ 0_u, 999_u } );
    REQUIRE( scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 200.f ).epsilon( 1e-5f ) );
}

TEST_CASE( "ScrollContainerWidget keyboard paging and home/end update offset", "[widget][scroll]" )
{
    Scene scene;
    WidgetID rootID = scene.CreateRootWidget<ScrollContainerWidget>();
    auto* scroll = scene.GetWidget<ScrollContainerWidget>( rootID );
    REQUIRE( scroll != nullptr );

    LayoutNode* rootNode = scene.Layouts.Get( scene.GetWidget( rootID )->GetLayoutID() );
    REQUIRE( rootNode != nullptr );
    rootNode->Style.WidthMode = ESizingMode::Fixed;
    rootNode->Style.HeightMode = ESizingMode::Fixed;
    rootNode->Style.FixedWidth = 120_u;
    rootNode->Style.FixedHeight = 100_u;

    WidgetID childID = scene.CreateWidget<DummyWidget>( rootID );
    LayoutNode* childNode = scene.Layouts.Get( scene.GetWidget( childID )->GetLayoutID() );
    REQUIRE( childNode != nullptr );
    childNode->Style.WidthMode = ESizingMode::Fixed;
    childNode->Style.HeightMode = ESizingMode::Fixed;
    childNode->Style.FixedWidth = 120_u;
    childNode->Style.FixedHeight = 500_u;

    scene.UpdateLayout( Vec2<Unit>{ 120_u, 100_u } );
    scene.SetFocus( rootID );

    scene.DispatchInputEvent( InputEvent{
        .Device = EDeviceID::Keyboard,
        .Payload = ButtonEvent{ .Button = EButtonID::KeyPageDown, .Pressed = true }
    } );
    REQUIRE( scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 90.f ).epsilon( 1e-5f ) );

    scene.DispatchInputEvent( InputEvent{
        .Device = EDeviceID::Keyboard,
        .Payload = ButtonEvent{ .Button = EButtonID::KeyEnd, .Pressed = true }
    } );
    REQUIRE( scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 400.f ).epsilon( 1e-5f ) );

    scene.DispatchInputEvent( InputEvent{
        .Device = EDeviceID::Keyboard,
        .Payload = ButtonEvent{ .Button = EButtonID::KeyHome, .Pressed = true }
    } );
    REQUIRE( scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 0.f ).epsilon( 1e-5f ) );
}

TEST_CASE( "ScrollContainerWidget ignores horizontal wheel when horizontal scrolling is disabled", "[widget][scroll]" )
{
    Scene scene;
    WidgetID rootID = scene.CreateRootWidget<ScrollContainerWidget>();
    auto* scroll = scene.GetWidget<ScrollContainerWidget>( rootID );
    REQUIRE( scroll != nullptr );

    LayoutNode* rootNode = scene.Layouts.Get( scene.GetWidget( rootID )->GetLayoutID() );
    REQUIRE( rootNode != nullptr );
    rootNode->Style.WidthMode = ESizingMode::Fixed;
    rootNode->Style.HeightMode = ESizingMode::Fixed;
    rootNode->Style.FixedWidth = 100_u;
    rootNode->Style.FixedHeight = 100_u;

    WidgetID childID = scene.CreateWidget<DummyWidget>( rootID );
    LayoutNode* childNode = scene.Layouts.Get( scene.GetWidget( childID )->GetLayoutID() );
    REQUIRE( childNode != nullptr );
    childNode->Style.WidthMode = ESizingMode::Fixed;
    childNode->Style.HeightMode = ESizingMode::Fixed;
    childNode->Style.FixedWidth = 400_u;
    childNode->Style.FixedHeight = 100_u;

    scene.UpdateLayout( Vec2<Unit>{ 100_u, 100_u } );
    scene.DispatchInputEvent( MakeMouseMove( Vec2f{ 10.f, 10.f } ) );
    scene.DispatchInputEvent( MakeMouseScroll( Vec2f{ 10.f, 10.f }, Vec2f{ -1.f, 0.f } ) );

    REQUIRE( scroll->GetScrollOffset()[0].ToFloat() == Catch::Approx( 0.f ).epsilon( 1e-5f ) );
}
