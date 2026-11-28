#include "TestSceneCommon.h"
#include <RatUI/Widget/ButtonWidget.h>
#include <RatUI/Widget/PanelWidget.h>
#include <RatUI/Widget/ScrollContainerWidget.h>
#include <RatUI/Widget/SliderWidget.h>

/**
 * @brief A 200x100 scroll container holding ten 100x30 buttons (300 tall), so it can scroll 200 vertically.
 */
struct ScrollSceneFixture
{
    Null::NullRenderer    Renderer;
    Null::NullTextMetrics TextMetrics;
    GlyphAtlas            Atlas{ Renderer, TextMetrics };
    DrawList              Drawer{ Atlas };

    RatUI::Scene           Scene;
    ScrollContainerWidget* Scroll{ nullptr };
    Array<ButtonWidget*>   Rows;
    int                    Clicked{ -1 };

    ScrollSceneFixture()
    {
        PanelWidget* root = Scene.CreateRootWidget<PanelWidget>();
        root->GetLayout().LayoutType( ELayoutType::Vertical );

        Scroll = Scene.CreateWidget<ScrollContainerWidget>( root->GetLayoutID() );
        Scroll->GetLayout().FixedWidth( 200_u ).FixedHeight( 100_u );
        Scroll->SetHScrollbarMode( EScrollbarMode::Never );

        for ( int i = 0; i < 10; ++i )
        {
            ButtonWidget* row = Scene.CreateWidget<ButtonWidget>( Scroll->GetContentNodeID(), [this, i]( ButtonBaseWidget& ) { Clicked = i; } );
            row->GetLayout().FixedWidth( 100_u ).FixedHeight( 30_u );
            PushBack( Rows, row );
        }

        Frame();
    }

    void Frame()
    {
        Scene.UpdateLayout( { 400_u, 300_u } );
        Drawer.Clear();
        Scene.Render( Drawer, 0.f );
    }

    void Wheel( Vec2<Unit> a_Position, Vec2<Unit> a_Delta )
    {
        Scene.DispatchInputEvent( { .Device = EDeviceID::Mouse, .Payload = PointerEvent{ .Position = a_Position, .Type = EPointerType::Mouse, .ScrollDelta = a_Delta } } );
    }

    void Click( Vec2<Unit> a_Position )
    {
        Scene.DispatchInputEvent( { .Device = EDeviceID::Mouse, .Payload = PointerEvent{ .Position = a_Position, .Type = EPointerType::Mouse } } );
        for ( const bool pressed : { true, false } )
        {
            Scene.DispatchInputEvent( { .Device  = EDeviceID::Mouse,
                                        .Payload = ButtonEvent{ .Button = EButtonID::MouseLeft, .Pressed = pressed, .Released = !pressed, .PointerPosition = a_Position } } );
        }
    }
};

TEST_CASE( "ScrollContainerWidget: the wheel scrolls the content wherever the pointer is over it", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 0.f ) );

    // Over a row, not the scrollbar; wheel down is a negative y delta.
    fixture.Wheel( { 50_u, 50_u }, { 0_u, -1_u } );
    const f32 afterOne = fixture.Scroll->GetScrollOffset()[1].ToFloat();
    REQUIRE( afterOne > 10.f );

    // Clamped to the content's end.
    for ( int i = 0; i < 50; ++i )
        fixture.Wheel( { 50_u, 50_u }, { 0_u, -1_u } );
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 200.f ) );

    for ( int i = 0; i < 50; ++i )
        fixture.Wheel( { 50_u, 50_u }, { 0_u, 1_u } );
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 0.f ) );
}

TEST_CASE( "ScrollContainerWidget: clicks reach the row under the pointer once scrolled", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    fixture.Scroll->SetScrollOffset( { 0_u, 90_u } );

    // Without a paint in between: y 10 is content y 100, row 3.
    fixture.Click( { 50_u, 10_u } );
    REQUIRE( fixture.Clicked == 3 );
}

TEST_CASE( "ScrollContainerWidget: SetScrollOffset moves the scrollbar and is clamped", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    fixture.Scroll->SetScrollOffset( { 0_u, 1000_u } );
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 200.f ) );

    fixture.Frame();
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 200.f ) );
}

TEST_CASE( "ScrollContainerWidget: an Auto scrollbar keeps its gutter and shows only when the content overflows", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    const LayoutNode*  content = fixture.Scroll->GetContentNode();
    const LayoutNode*  bar     = content->NextSibling(); // The vertical bar.
    const f32          gutter  = 200.f - ScrollContainerWidget::c_ScrollbarSize.ToFloat();

    REQUIRE( content->Layout.SceneRect.Width().ToFloat() == Catch::Approx( gutter ) );
    REQUIRE( bar->Layout.Visibility == EVisibility::Visible );

    for ( size i = 3; i < Size( fixture.Rows ); ++i )
        fixture.Rows[i]->GetLayout().Visibility( EVisibility::Collapsed );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );

    // The content fits now: the bar goes, its strip stays, so nothing shifts.
    REQUIRE( bar->Layout.Visibility == EVisibility::Hidden );
    REQUIRE( content->Layout.SceneRect.Width().ToFloat() == Catch::Approx( gutter ) );

    SECTION( "overlay bars take no space" )
    {
        fixture.Scroll->SetScrollbarPlacement( EScrollbarPlacement::Overlay );
        fixture.Scene.UpdateLayout( { 400_u, 300_u } );
        REQUIRE( content->Layout.SceneRect.Width().ToFloat() == Catch::Approx( 200.f ) );
    }

    SECTION( "a bar that is Never shown has no gutter" )
    {
        fixture.Scroll->SetVScrollbarMode( EScrollbarMode::Never );
        fixture.Scene.UpdateLayout( { 400_u, 300_u } );
        REQUIRE( bar->Layout.Visibility == EVisibility::Collapsed );
        REQUIRE( content->Layout.SceneRect.Width().ToFloat() == Catch::Approx( 200.f ) );
    }

    SECTION( "a bar that is Always shown stays" )
    {
        fixture.Scroll->SetVScrollbarMode( EScrollbarMode::Always );
        fixture.Scene.UpdateLayout( { 400_u, 300_u } );
        REQUIRE( bar->Layout.Visibility == EVisibility::Visible );
    }
}

TEST_CASE( "ScrollContainerWidget: layout places scrolled rows where they are drawn", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    fixture.Scroll->SetScrollOffset( { 0_u, 90_u } );

    // At once, then still after a layout: row 3 (content y 90) is at the top.
    REQUIRE( fixture.Rows[3]->GetLayout().Layout.SceneRect.Top().ToFloat() == Catch::Approx( 0.f ) );
    fixture.Frame();
    REQUIRE( fixture.Rows[3]->GetLayout().Layout.SceneRect.Top().ToFloat() == Catch::Approx( 0.f ) );
    REQUIRE( fixture.Rows[0]->GetLayout().Layout.SceneRect.Top().ToFloat() == Catch::Approx( -90.f ) );
}

TEST_CASE( "ScrollContainerWidget: content shrinking clamps the offset and reports it after the layout", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    fixture.Scroll->SetScrollOffset( { 0_u, 200_u } );

    Vec2<Unit> reported{ -1_u, -1_u };
    fixture.Scroll->OnScroll = [&]( ScrollContainerWidget&, const ScrollEvent& a_Event ) { reported = a_Event.Offset; };

    // Five rows left: 150 tall in a 100 viewport.
    for ( size i = 5; i < Size( fixture.Rows ); ++i )
        fixture.Rows[i]->GetLayout().Visibility( EVisibility::Collapsed );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );

    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 50.f ) );
    REQUIRE( fixture.Scroll->GetMaxScrollOffset()[1].ToFloat() == Catch::Approx( 50.f ) );
    REQUIRE( reported[1].ToFloat() == Catch::Approx( 50.f ) );
    REQUIRE( fixture.Rows[4]->GetLayout().Layout.SceneRect.Bottom().ToFloat() == Catch::Approx( 100.f ) );
}

TEST_CASE( "Scene: BringIntoView scrolls each scroll container a widget is in, innermost first", "[Scene][Scroll]" )
{
    // An outer 100-tall scroll container holding a 60-tall spacer and an inner 80-tall one of four 40-tall buttons.
    RatUI::Scene scene;
    PanelWidget* root = scene.CreateRootWidget<PanelWidget>();
    root->GetLayout().FixedWidth( 200_u ).FixedHeight( 100_u ).LayoutType( ELayoutType::Vertical );

    ScrollContainerWidget* outer = scene.CreateWidget<ScrollContainerWidget>( root->GetLayoutID() );
    outer->SetHScrollbarMode( EScrollbarMode::Never );

    LayoutNode& spacer = scene.CreateLayoutNode( {}, outer->GetContentNodeID() );
    spacer.FixedWidth( 100_u ).FixedHeight( 60_u );

    ScrollContainerWidget* inner = scene.CreateWidget<ScrollContainerWidget>( outer->GetLayoutID() );
    inner->SetHScrollbarMode( EScrollbarMode::Never );
    inner->GetLayout().FixedWidth( 150_u ).FixedHeight( 80_u );

    Array<ButtonWidget*> buttons;
    int                  clicked = -1;
    for ( int i = 0; i < 4; ++i )
    {
        ButtonWidget* button = scene.CreateWidget<ButtonWidget>( inner->GetLayoutID(), [&clicked, i]( ButtonBaseWidget& ) { clicked = i; } );
        button->GetLayout().FixedWidth( 100_u ).FixedHeight( 40_u );
        PushBack( buttons, button );
    }
    scene.UpdateLayout( { 200_u, 100_u } );

    scene.BringIntoView( buttons[3]->GetLayoutID() );

    // The inner one scrolls 80 to show the last button (inner y 120..160), then the outer one 40 to show the inner's bottom.
    REQUIRE( inner->GetScrollOffset()[1].ToFloat() == Catch::Approx( 80.f ) );
    REQUIRE( outer->GetScrollOffset()[1].ToFloat() == Catch::Approx( 40.f ) );
    const Rect<Unit>& rect = buttons[3]->GetLayout().Layout.SceneRect;
    REQUIRE( rect.Top().ToFloat() == Catch::Approx( 60.f ) );
    REQUIRE( rect.Bottom().ToFloat() == Catch::Approx( 100.f ) );

    // A click there reaches it, and the scroll holds through the next layout.
    const Vec2<Unit> point{ 50_u, 80_u };
    scene.DispatchInputEvent( { .Device = EDeviceID::Mouse, .Payload = PointerEvent{ .Position = point, .Type = EPointerType::Mouse } } );
    for ( const bool pressed : { true, false } )
        scene.DispatchInputEvent( { .Device  = EDeviceID::Mouse,
                                    .Payload = ButtonEvent{ .Button = EButtonID::MouseLeft, .Pressed = pressed, .Released = !pressed, .PointerPosition = point } } );
    REQUIRE( clicked == 3 );
    scene.UpdateLayout( { 200_u, 100_u } );
    REQUIRE( buttons[3]->GetLayout().Layout.SceneRect.Top().ToFloat() == Catch::Approx( 60.f ) );
}

TEST_CASE( "ScrollContainerWidget: pressing the scrollbar jumps there, and hit testing follows", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;

    // The bar is at x 184..200; its bottom end is the content's end.
    const Vec2<Unit> bottom{ 192_u, 99_u };
    fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Mouse, .Payload = PointerEvent{ .Position = bottom, .Type = EPointerType::Mouse } } );
    fixture.Scene.DispatchInputEvent( { .Device  = EDeviceID::Mouse,
                                        .Payload = ButtonEvent{ .Button = EButtonID::MouseLeft, .Pressed = true, .PointerPosition = bottom } } );
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 200.f ) );

    // Hit testing follows at once.
    fixture.Scene.DispatchInputEvent( { .Device  = EDeviceID::Mouse,
                                        .Payload = ButtonEvent{ .Button = EButtonID::MouseLeft, .Released = true, .PointerPosition = bottom } } );
    fixture.Click( { 50_u, 95_u } );
    REQUIRE( fixture.Clicked == 9 );
}

TEST_CASE( "ScrollContainerWidget: a scroll set before the layout catches up reaches content added since", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;

    for ( int i = 0; i < 5; ++i )
    {
        ButtonWidget* row = fixture.Scene.CreateWidget<ButtonWidget>( fixture.Scroll->GetContentNodeID(), []( ButtonBaseWidget& ) {} );
        row->GetLayout().FixedWidth( 100_u ).FixedHeight( 30_u );
    }

    // Scroll to the end before the new rows are laid out.
    fixture.Scroll->SetScrollOffset( { 0_u, 10000_u } );
    fixture.Frame();
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 350.f ) );

    // The request is used once, so later paints don't pull the list back to the end.
    fixture.Wheel( { 50_u, 50_u }, { 0_u, 1_u } );
    fixture.Frame();
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 310.f ) );
}

TEST_CASE( "ScrollContainerWidget: the empty area scrolls with the wheel but doesn't take clicks or focus", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;

    // Rows are 100 wide, so x 150 is the content's empty space.
    fixture.Wheel( { 150_u, 50_u }, { 0_u, -1_u } );
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( ScrollContainerWidget::c_WheelStep.ToFloat() ) );

    fixture.Click( { 150_u, 50_u } );
    REQUIRE( fixture.Scene.GetFocusedNode() != fixture.Scroll->GetLayoutID() );
    REQUIRE( fixture.Scene.GetCapturedWidget( c_MousePointerID ) == c_InvalidNodeID );
}

TEST_CASE( "ScrollContainerWidget: the wheel doesn't scroll an axis whose scrollbar is Never", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    fixture.Rows[0]->GetLayout().FixedWidth( 400_u );
    fixture.Frame();

    fixture.Wheel( { 50_u, 50_u }, { 1_u, 0_u } );
    REQUIRE( fixture.Scroll->GetScrollOffset()[0].ToFloat() == Catch::Approx( 0.f ) );

    fixture.Scroll->SetScrollOffset( { 30_u, 0_u } );
    REQUIRE( fixture.Scroll->GetScrollOffset()[0].ToFloat() == Catch::Approx( 30.f ) );
}

TEST_CASE( "ScrollContainerWidget: the wheel over an unfocused slider in the content scrolls the list", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    SliderWidget* slider = fixture.Scene.CreateWidget<SliderWidget>( fixture.Scroll->GetContentNodeID(), 0.f, 1.f, 0.5f );
    slider->GetLayout().FixedWidth( 100_u ).FixedHeight( 16_u );
    fixture.Frame();
    fixture.Scroll->SetScrollOffset( { 0_u, 10000_u } );
    fixture.Frame();
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 216.f ) );

    // The slider is drawn at y 84..100.
    fixture.Wheel( { 50_u, 92_u }, { 0_u, 1_u } );
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 216.f - ScrollContainerWidget::c_WheelStep.ToFloat() ) );
    REQUIRE( slider->Value.Get() == Catch::Approx( 0.5f ) );
}

TEST_CASE( "SliderWidget: a focused horizontal slider takes the vertical wheel when there is no sideways scroll", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    SliderWidget* slider = fixture.Scene.CreateWidget<SliderWidget>( fixture.Scroll->GetContentNodeID(), 0.f, 1.f, 0.5f );
    slider->GetLayout().FixedWidth( 100_u ).FixedHeight( 16_u );
    fixture.Frame();
    fixture.Scroll->SetScrollOffset( { 0_u, 10000_u } );
    fixture.Frame();
    fixture.Scene.SetFocus( slider->GetLayoutID() );
    const f32 offset = fixture.Scroll->GetScrollOffset()[1].ToFloat();

    // The slider is drawn at y 84..100. Wheel up raises the value; the list doesn't scroll.
    fixture.Wheel( { 50_u, 92_u }, { 0_u, 1_u } );
    REQUIRE( slider->Value.Get() > 0.5f );
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( offset ) );

    // The sideways wheel wins when both are present.
    const f32 raised = slider->Value.Get();
    fixture.Wheel( { 60_u, 92_u }, { -1_u, 1_u } );
    REQUIRE( slider->Value.Get() < raised );
}

TEST_CASE( "Scene: a wheel gesture stays with the widget that took it, until the pointer moves", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    SliderWidget* slider = fixture.Scene.CreateWidget<SliderWidget>( fixture.Scroll->GetContentNodeID(), 0.f, 1.f, 0.5f );
    slider->GetLayout().FixedWidth( 100_u ).FixedHeight( 16_u );
    fixture.Frame();
    fixture.Scroll->SetScrollOffset( { 0_u, 176_u } );
    fixture.Scene.SetFocus( slider->GetLayoutID() ); // Focused, so it takes the wheel when the gesture starts on it.

    // Over row 8: the list scrolls to its end, which brings the slider under the pointer (y 84..100).
    const Vec2<Unit> notch{ 1_u, -1_u };
    fixture.Wheel( { 50_u, 92_u }, notch );
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 216.f ) );

    // Still the list's gesture: the slider doesn't take it, though the list can't scroll further.
    fixture.Wheel( { 50_u, 92_u }, notch );
    REQUIRE( slider->Value.Get() == Catch::Approx( 0.5f ) );

    // Moving the pointer starts a new gesture, which the slider under it takes.
    fixture.Wheel( { 60_u, 92_u }, notch );
    REQUIRE( slider->Value.Get() != Catch::Approx( 0.5f ) );
}

namespace
{
    /** @brief Content that is short when measured and tall once arranged, like text that reflows, so layout needs a second pass. */
    class GrowingWidget : public IWidget
    {
    public:
        int Calls{ 0 };

        GrowingWidget() : IWidget( { .WidthDependentContent = true } ) {}

        Vec2<Unit> OnMeasureContent( const LayoutNode&, Vec2<Unit>, const LayoutContext& ) override
        {
            return { 100_u, ++Calls > 1 ? 300_u : 30_u };
        }
    };
}

TEST_CASE( "ScrollContainerWidget: a scroll request reaches content that a later layout pass grows", "[Scene][Scroll]" )
{
    RatUI::Scene scene;
    PanelWidget* root = scene.CreateRootWidget<PanelWidget>();
    ScrollContainerWidget* scroll = scene.CreateWidget<ScrollContainerWidget>( root->GetLayoutID() );
    scroll->GetLayout().FixedWidth( 200_u ).FixedHeight( 100_u );
    scroll->SetHScrollbarMode( EScrollbarMode::Never );
    scene.CreateWidget<GrowingWidget>( scroll->GetContentNodeID() )->GetLayout().WidthMode( ESizing::Content ).HeightMode( ESizing::Content );

    scroll->SetScrollOffset( { 0_u, 10000_u } );
    scene.UpdateLayout( { 400_u, 300_u } );
    REQUIRE( scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 200.f ) );
}

TEST_CASE( "ScrollContainerWidget: OnScroll for an offset layout changed runs once the layout is done", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    fixture.Scroll->SetScrollOffset( { 0_u, 200_u } );

    // The callback changes the layout, as a status label would.
    int  calls         = 0;
    bool sawFullLayout = true;
    fixture.Scroll->OnScroll = [&]( ScrollContainerWidget&, const ScrollEvent& a_Event )
    {
        ++calls;
        sawFullLayout &= a_Event.Cause == EScrollCause::Layout;
        fixture.Rows[0]->GetLayout().FixedHeight( 60_u );
    };

    for ( size i = 5; i < Size( fixture.Rows ); ++i )
        fixture.Rows[i]->GetLayout().Visibility( EVisibility::Collapsed );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );

    // Once for the clamp, once more for the range the callback's own change grew. What it changed is laid out
    // before UpdateLayout returns, so nothing is drawn a frame behind.
    REQUIRE( calls == 2 );
    REQUIRE( sawFullLayout );
    REQUIRE( fixture.Rows[0]->GetLayout().Layout.SceneRect.Height().ToFloat() == Catch::Approx( 60.f ) );
    REQUIRE( fixture.Scroll->GetMaxScrollOffset()[1].ToFloat() == Catch::Approx( 80.f ) );
}

TEST_CASE( "ScrollContainerWidget: navigating to a hidden row scrolls it into view", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    fixture.Scene.Input.NavMap.BindDefaultDesktop();
    fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = ButtonEvent{ .Button = EButtonID::KeyDown, .Pressed = true } } );
    REQUIRE( fixture.Scene.GetInputMode() == EInputMode::Navigation );

    fixture.Scene.SetFocus( fixture.Rows[9]->GetLayoutID() );
    fixture.Frame();
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 200.f ) );

    fixture.Scene.SetFocus( fixture.Rows[1]->GetLayoutID() );
    fixture.Frame();
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 30.f ) );
}

TEST_CASE( "Scene: clicking empty space clears focus", "[Scene]" )
{
    ScrollSceneFixture fixture;

    fixture.Click( { 50_u, 15_u } );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Rows[0]->GetLayoutID() );

    // Outside every widget.
    fixture.Click( { 300_u, 250_u } );
    REQUIRE( fixture.Scene.GetFocusedNode() == c_InvalidNodeID );
}

TEST_CASE( "Scene: arrow keys reach widgets nested in layout nodes", "[Scene][Navigation]" )
{
    ScrollSceneFixture fixture;

    // The rows sit inside the scroll container's content node, not directly in the root.
    fixture.Scene.Navigate( ENavAction::MoveDown );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Rows[0]->GetLayoutID() );

    fixture.Scene.Navigate( ENavAction::MoveDown );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Rows[1]->GetLayoutID() );

    fixture.Scene.Navigate( ENavAction::MoveUp );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Rows[0]->GetLayoutID() );
}

TEST_CASE( "Scene: Tab and Shift+Tab move through widgets in layout order and wrap", "[Scene][Navigation]" )
{
    ScrollSceneFixture fixture;
    fixture.Scene.Input.NavMap.BindDefaultDesktop();

    const auto Tab = [&]( bool a_Shift )
    {
        for ( const bool pressed : { true, false } )
        {
            fixture.Scene.DispatchInputEvent( { .Device  = EDeviceID::Keyboard,
                                                .Payload = ButtonEvent{ .Button    = EButtonID::KeyTab,
                                                                        .Modifiers = a_Shift ? EModifier::Shift : EModifier::None,
                                                                        .Pressed   = pressed,
                                                                        .Released  = !pressed } } );
        }
    };

    Tab( false );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Rows[0]->GetLayoutID() );
    Tab( false );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Rows[1]->GetLayoutID() );

    Tab( true );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Rows[0]->GetLayoutID() );
    Tab( true );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Rows[9]->GetLayoutID() );
    Tab( false );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Rows[0]->GetLayoutID() );
}

TEST_CASE( "Scene: arrow keys compare scrolled widgets where they are drawn", "[Scene][Navigation]" )
{
    ScrollSceneFixture fixture;

    // A button below the list, at y 100..130.
    ButtonWidget* below = fixture.Scene.CreateWidget<ButtonWidget>( fixture.Scroll->GetLayout().Parent()->ID );
    below->GetLayout().FixedWidth( 100_u ).FixedHeight( 30_u );

    // Scrolled to the end, rows 7-9 are drawn at y 10..100. Unscrolled, the row just above the button would be
    // row 3 (y 90..120), which is now out of view.
    fixture.Scroll->SetScrollOffset( { 0_u, 200_u } );
    fixture.Frame();
    fixture.Scene.SetFocus( below->GetLayoutID() );

    fixture.Scene.Navigate( ENavAction::MoveUp );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Rows[9]->GetLayoutID() );
}

TEST_CASE( "Scene: keyboard focus stays visible after the mouse was used", "[Scene][Navigation]" )
{
    ScrollSceneFixture fixture;

    // A mouse event is replayed on every layout to refresh hover; it mustn't count as using the mouse.
    fixture.Scene.Input.NavMap.BindDefaultDesktop();
    fixture.Click( { 300_u, 250_u } );
    fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = ButtonEvent{ .Button = EButtonID::KeyDown, .Pressed = true } } );
    fixture.Frame();

    REQUIRE( fixture.Scene.IsFocusVisible( fixture.Rows[0]->GetLayoutID() ) );
}

TEST_CASE( "SliderWidget: Activate engages the slider to move it with the arrows, and disengages it again", "[Scene][Navigation]" )
{
    ScrollSceneFixture fixture;
    fixture.Scene.Input.NavMap.BindDefaultDesktop();
    SliderWidget* slider = fixture.Scene.CreateWidget<SliderWidget>( fixture.Scroll->GetContentNodeID(), 0.f, 1.f, 0.5f );
    slider->GetLayout().FixedWidth( 100_u ).FixedHeight( 16_u );
    fixture.Frame();
    fixture.Scene.SetFocus( slider->GetLayoutID() );

    const auto Press = [&]( EButtonID a_Key )
    {
        fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = ButtonEvent{ .Button = a_Key, .Pressed = true } } );
        fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = ButtonEvent{ .Button = a_Key, .Released = true } } );
    };

    // Not engaged: arrows navigate and leave the value alone.
    Press( EButtonID::KeyRight );
    REQUIRE( slider->Value.Get() == Catch::Approx( 0.5f ) );
    REQUIRE( !slider->IsEngaged() );

    Press( EButtonID::KeyEnter );
    REQUIRE( slider->IsEngaged() );

    Press( EButtonID::KeyRight );
    REQUIRE( slider->Value.Get() == Catch::Approx( 0.51f ) ); // No Step: 1% of the range.
    Press( EButtonID::KeyLeft );
    REQUIRE( slider->Value.Get() == Catch::Approx( 0.5f ) );

    // Across the track, the slider stays focused and engaged.
    Press( EButtonID::KeyUp );
    REQUIRE( slider->IsEngaged() );
    REQUIRE( fixture.Scene.GetFocusedNode() == slider->GetLayoutID() );

    Press( EButtonID::KeyEscape );
    REQUIRE( !slider->IsEngaged() );
    REQUIRE( fixture.Scene.GetFocusedNode() == slider->GetLayoutID() );

    Press( EButtonID::KeyEnter );
    Press( EButtonID::KeyEnter );
    REQUIRE( !slider->IsEngaged() );
    REQUIRE( fixture.Scene.GetFocusedNode() == slider->GetLayoutID() );

    // The slider is below the last row.
    Press( EButtonID::KeyUp );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Rows[9]->GetLayoutID() );
}

TEST_CASE( "SliderWidget: Tab stops on the slider without engaging it", "[Scene][Navigation]" )
{
    ScrollSceneFixture fixture;
    SliderWidget* slider = fixture.Scene.CreateWidget<SliderWidget>( fixture.Scroll->GetContentNodeID(), 0.f, 1.f, 0.5f );
    slider->GetLayout().FixedWidth( 100_u ).FixedHeight( 16_u );
    fixture.Frame();

    fixture.Scene.SetFocus( fixture.Rows[9]->GetLayoutID() );
    fixture.Scene.Navigate( ENavAction::Next );
    REQUIRE( fixture.Scene.GetFocusedNode() == slider->GetLayoutID() );
    fixture.Scene.Navigate( ENavAction::Next );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Rows[0]->GetLayoutID() );
}

TEST_CASE( "SliderWidget: arrow keys move an engaged slider by its Step, onto the steps", "[Scene][Navigation]" )
{
    ScrollSceneFixture fixture;
    fixture.Scene.Input.NavMap.BindDefaultDesktop();
    SliderWidget* slider = fixture.Scene.CreateWidget<SliderWidget>( fixture.Scroll->GetContentNodeID(), 10.f, 20.f, 13.f );
    slider->GetLayout().FixedWidth( 100_u ).FixedHeight( 16_u );
    slider->Step = 2.f; // Steps at 10, 12, 14...
    fixture.Frame();
    fixture.Scene.Engage( slider->GetLayoutID() );

    const auto Press = [&]( EButtonID a_Key )
    {
        fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = ButtonEvent{ .Button = a_Key, .Pressed = true } } );
        fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = ButtonEvent{ .Button = a_Key, .Released = true } } );
    };

    // From off the steps, to the next step in that direction.
    Press( EButtonID::KeyRight );
    REQUIRE( slider->Value.Get() == Catch::Approx( 14.f ) );
    Press( EButtonID::KeyRight );
    REQUIRE( slider->Value.Get() == Catch::Approx( 16.f ) );
    Press( EButtonID::KeyLeft );
    REQUIRE( slider->Value.Get() == Catch::Approx( 14.f ) );

    for ( int i = 0; i < 10; ++i )
        Press( EButtonID::KeyRight );
    REQUIRE( slider->Value.Get() == Catch::Approx( 20.f ) );
}

TEST_CASE( "SliderWidget: dragging snaps the value, not the thumb position, to Step", "[Scene]" )
{
    ScrollSceneFixture fixture;
    SliderWidget* slider = fixture.Scene.CreateWidget<SliderWidget>( fixture.Scroll->GetContentNodeID(), 10.f, 20.f, 10.f );
    slider->GetLayout().FixedWidth( 100_u ).FixedHeight( 16_u );
    slider->Step = 2.f;
    fixture.Frame();

    // Scrolled to the end, the slider is drawn at y 84..100. Half way along the track is 15, which snaps to 16.
    fixture.Scroll->SetScrollOffset( { 0_u, 10000_u } );
    fixture.Frame();
    fixture.Click( { 50_u, 92_u } );
    REQUIRE( slider->Value.Get() == Catch::Approx( 16.f ) );
}

TEST_CASE( "SliderWidget: Tab disengages the slider and moves on", "[Scene][Navigation]" )
{
    ScrollSceneFixture fixture;
    fixture.Scene.Input.NavMap.BindDefaultDesktop();
    SliderWidget* slider = fixture.Scene.CreateWidget<SliderWidget>( fixture.Scroll->GetContentNodeID(), 0.f, 1.f, 0.5f );
    slider->GetLayout().FixedWidth( 100_u ).FixedHeight( 16_u );
    fixture.Frame();
    fixture.Scene.Engage( slider->GetLayoutID() );
    REQUIRE( slider->IsEngaged() );

    // The slider is the last widget, so Tab wraps to the first row.
    fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = ButtonEvent{ .Button = EButtonID::KeyTab, .Pressed = true } } );
    fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = ButtonEvent{ .Button = EButtonID::KeyTab, .Released = true } } );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Rows[0]->GetLayoutID() );
    REQUIRE_FALSE( slider->IsEngaged() );
}

// ============================================================================
// Scroll events, requests and sticking to the end
// ============================================================================

TEST_CASE( "ScrollContainerWidget: scrolling moves the content without running layout", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    const LayoutNode&  root   = *fixture.Scene.GetLayoutNode( fixture.Scene.RootWidget );
    const Rect<Unit>   parent = fixture.Rows[3]->GetLayout().Layout.ParentRect;

    fixture.Wheel( { 50_u, 50_u }, { 0_u, -1_u } );

    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 40.f ) );
    REQUIRE_FALSE( root.Layout.IsDirty );
    REQUIRE_FALSE( root.Layout.IsDescendantDirty );
    REQUIRE( fixture.Rows[3]->GetLayout().Layout.ParentRect.Origin == parent.Origin );
    REQUIRE( fixture.Rows[3]->GetLayout().Layout.SceneRect.Top().ToFloat() == Catch::Approx( 90.f - 40.f ) );
}

TEST_CASE( "ScrollContainerWidget: OnScroll says what changed and what caused it", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    Array<ScrollEvent> events;
    fixture.Scroll->OnScroll = [&]( ScrollContainerWidget&, const ScrollEvent& a_Event ) { PushBack( events, a_Event ); };

    fixture.Wheel( { 50_u, 50_u }, { 0_u, -1_u } );
    REQUIRE( Size( events ) == 1 );
    REQUIRE( events[0].Cause == EScrollCause::User );
    REQUIRE( events[0].OffsetDelta[1].ToFloat() == Catch::Approx( 40.f ) );
    REQUIRE( events[0].MaxOffset[1].ToFloat() == Catch::Approx( 200.f ) );

    fixture.Scroll->SetScrollOffset( { 0_u, 200_u } );
    REQUIRE( Size( events ) == 2 );
    REQUIRE( events[1].Cause == EScrollCause::Programmatic );
    REQUIRE( events[1].Offset[1].ToFloat() == Catch::Approx( 200.f ) );

    // Content shrinks: layout clamps the offset.
    fixture.Rows[9]->GetLayout().Visibility( EVisibility::Collapsed );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );
    REQUIRE( Size( events ) == 3 );
    REQUIRE( events[2].Cause == EScrollCause::Layout );
    REQUIRE( events[2].OffsetDelta[1].ToFloat() == Catch::Approx( -30.f ) );
    REQUIRE( events[2].MaxOffsetDelta[1].ToFloat() == Catch::Approx( -30.f ) );

    // Content grows: only the range changes.
    fixture.Rows[9]->GetLayout().Visibility( EVisibility::Visible );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );
    REQUIRE( Size( events ) == 4 );
    REQUIRE( events[3].Cause == EScrollCause::Layout );
    REQUIRE( events[3].OffsetDelta[1].ToFloat() == Catch::Approx( 0.f ) );
    REQUIRE( events[3].MaxOffsetDelta[1].ToFloat() == Catch::Approx( 30.f ) );

    // Nothing changed: nothing reported.
    fixture.Scroll->SetScrollOffset( fixture.Scroll->GetScrollOffset() );
    fixture.Scene.UpdateLayout( { 400_u, 300_u } );
    REQUIRE( Size( events ) == 4 );
}

TEST_CASE( "ScrollContainerWidget: ScrollToEnd right after adding rows reaches the new end", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;

    ButtonWidget* row = fixture.Scene.CreateWidget<ButtonWidget>( fixture.Scroll->GetLayoutID(), []( ButtonBaseWidget& ) {} );
    row->GetLayout().FixedWidth( 100_u ).FixedHeight( 30_u );
    fixture.Scroll->ScrollToEnd();
    fixture.Frame();

    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 230.f ) );
    REQUIRE( fixture.Scroll->GetScrollOffset()[0].ToFloat() == Catch::Approx( 0.f ) );
}

TEST_CASE( "ScrollContainerWidget: StickToEnd follows new content while the view is at the end", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    fixture.Scroll->StickToEndY = true;

    const auto addRow = [&]
    {
        ButtonWidget* row = fixture.Scene.CreateWidget<ButtonWidget>( fixture.Scroll->GetLayoutID(), []( ButtonBaseWidget& ) {} );
        row->GetLayout().FixedWidth( 100_u ).FixedHeight( 30_u );
        fixture.Frame();
    };

    // Not at the end: left where it is.
    addRow();
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 0.f ) );

    fixture.Scroll->ScrollToEnd();
    addRow();
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 260.f ) );
    addRow();
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 290.f ) );

    // Scrolled away from the end, it is released.
    fixture.Wheel( { 50_u, 50_u }, { 0_u, 1_u } );
    addRow();
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 250.f ) );
}

// ============================================================================
// Content node, range and defaults
// ============================================================================

TEST_CASE( "ScrollContainerWidget: focus scrolled into view reports OnScroll at once", "[Scene][Scroll]" )
{
    NavFixture             f;
    ScrollContainerWidget* scroll = f.Scene.CreateWidget<ScrollContainerWidget>( f.Root->GetLayoutID() );
    ButtonWidget*          last   = nullptr;
    for ( int i = 0; i < 12; ++i )
        last = f.AddButton( scroll->GetContentNodeID() );
    f.Layout();
    REQUIRE( scroll->GetMaxScrollOffset()[1] > 0_u );

    int          scrolls = 0;
    EScrollCause cause   = EScrollCause::Layout;
    scroll->OnScroll = [&]( ScrollContainerWidget&, const ScrollEvent& a_Event ) { ++scrolls; cause = a_Event.Cause; };

    f.Scene.Input.InputMode = EInputMode::Navigation;
    f.Scene.SetFocus( last->GetLayoutID() );
    REQUIRE( scroll->GetScrollOffset()[1] > 0_u );
    REQUIRE( scrolls == 1 );
    REQUIRE( cause == EScrollCause::Programmatic );

    f.Layout();
    REQUIRE( scrolls == 1 );
}

TEST_CASE( "ScrollContainerWidget: a widget created with the container as its parent goes in the scrolled content", "[Scene][Scroll]" )
{
    NavFixture             f;
    ScrollContainerWidget* scroll = f.Scene.CreateWidget<ScrollContainerWidget>( f.Root->GetLayoutID() );
    ButtonWidget*          button = f.AddButton( scroll->GetLayoutID() );

    REQUIRE( scroll->GetContentNodeID() != scroll->GetLayoutID() );
    REQUIRE( button->GetLayout().Parent() == scroll->GetContentNode() );
    REQUIRE( button->GetParentWidget() == scroll ); // Past the content node, which has no widget of its own.
}

TEST_CASE( "Scene: a layout node created under a scroll container goes in its content, like a widget", "[Scene][Scroll]" )
{
    NavFixture             f;
    ScrollContainerWidget* scroll = f.Scene.CreateWidget<ScrollContainerWidget>( f.Root->GetLayoutID() );
    LayoutNode&            node   = f.Scene.CreateLayoutNode( {}, scroll->GetLayoutID() );

    REQUIRE( node.Parent() == scroll->GetContentNode() );
}

TEST_CASE( "ScrollContainerWidget: by default no strip is kept for a horizontal bar", "[Scene][Scroll]" )
{
    NavFixture             f;
    ScrollContainerWidget* scroll = f.Scene.CreateWidget<ScrollContainerWidget>( f.Root->GetLayoutID() );
    f.AddButton( scroll->GetLayoutID() );
    f.Layout();

    REQUIRE( scroll->GetHScrollbarMode() == EScrollbarMode::Never );
    REQUIRE_UNIT( scroll->GetContentNode()->Layout.ParentRect.Size[1], scroll->GetLayout().Layout.ParentRect.Size[1].ToFloat() );

    scroll->SetHScrollbarMode( EScrollbarMode::Auto );
    f.Layout();
    REQUIRE_UNIT( scroll->GetContentNode()->Layout.ParentRect.Size[1],
                  ( scroll->GetLayout().Layout.ParentRect.Size[1] - ScrollContainerWidget::c_ScrollbarSize ).ToFloat() );
}

TEST_CASE( "ScrollContainerWidget: what overflows a child can be scrolled to", "[Scene][Scroll]" )
{
    NavFixture             f;
    ScrollContainerWidget* scroll = f.Scene.CreateWidget<ScrollContainerWidget>( f.Root->GetLayoutID() );
    scroll->GetLayout().FixedWidth( 200_u ).FixedHeight( 100_u );

    // A 100-tall panel holding 300 of buttons.
    PanelWidget* panel = f.Scene.CreateWidget<PanelWidget>( scroll->GetLayoutID() );
    panel->GetLayout().LayoutType( ELayoutType::Vertical ).FixedWidth( 100_u ).FixedHeight( 100_u );
    for ( int i = 0; i < 3; ++i )
        f.AddButton( panel->GetLayoutID() )->GetLayout().FixedHeight( 100_u );
    f.Layout();

    REQUIRE_UNIT( scroll->GetMaxScrollOffset()[1], 200.f );
}

TEST_CASE( "ScrollContainerWidget: a nested scroll container's own overflow doesn't grow the outer one's range", "[Scene][Scroll]" )
{
    NavFixture             f;
    ScrollContainerWidget* outer = f.Scene.CreateWidget<ScrollContainerWidget>( f.Root->GetLayoutID() );
    outer->GetLayout().FixedWidth( 200_u ).FixedHeight( 100_u );
    ScrollContainerWidget* inner = f.Scene.CreateWidget<ScrollContainerWidget>( outer->GetLayoutID() );
    inner->GetLayout().FixedWidth( 150_u ).FixedHeight( 80_u );
    for ( int i = 0; i < 10; ++i )
        f.AddButton( inner->GetLayoutID() );
    f.Layout();

    REQUIRE( inner->GetMaxScrollOffset()[1] > 0_u );
    REQUIRE_UNIT( outer->GetMaxScrollOffset()[1], 0.f );
}

TEST_CASE( "ScrollContainerWidget: an offset asked for with nothing to lay out isn't kept, however UpdateLayout is called", "[Scene][Scroll]" )
{
    NavFixture             f;
    ScrollContainerWidget* scroll = f.Scene.CreateWidget<ScrollContainerWidget>( f.Root->GetLayoutID() );
    for ( int i = 0; i < 12; ++i )
        f.AddButton( scroll->GetLayoutID() );
    f.Layout();
    const Unit end = scroll->GetMaxScrollOffset()[1];

    // The tree is clean, so the end is the end as it is now, whether the next row comes after one UpdateLayout or three.
    scroll->ScrollToEnd();
    SECTION( "one call" )    { f.Layout(); }
    SECTION( "three calls" ) { f.Layout(); f.Layout(); f.Layout(); }

    f.AddButton( scroll->GetLayoutID() );
    f.Layout();
    REQUIRE_UNIT( scroll->GetScrollOffset()[1], end.ToFloat() );
    REQUIRE( scroll->GetMaxScrollOffset()[1] > end );
}

// ============================================================================
// Scrollbars, nesting and the sideways wheel
// ============================================================================

namespace
{
    SliderWidget* FindScrollbar( RatUI::Scene& a_Scene, ScrollContainerWidget& a_Scroll, EOrient a_Orientation )
    {
        SliderWidget* found = nullptr;
        a_Scroll.GetLayout().ForEachChild( [&]( LayoutNode& a_Child )
        {
            if ( SliderWidget* bar = a_Scene.GetWidget<SliderWidget>( a_Child.ID ); bar && bar->Orientation == a_Orientation )
                found = bar;
        } );
        return found;
    }
}

TEST_CASE( "ScrollContainerWidget: the scrollbar's range and thumb follow the content and viewport", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    SliderWidget*      bar = FindScrollbar( fixture.Scene, *fixture.Scroll, EOrient::Vertical );
    REQUIRE( bar != nullptr );

    // 100 of 300 is in view.
    REQUIRE( bar->Max == Catch::Approx( 200.f ) );
    REQUIRE( bar->ThumbScale == Catch::Approx( 1.f / 3.f ) );

    fixture.Scroll->SetScrollOffset( { 0_u, 90_u } );
    REQUIRE( bar->Value.Get() == Catch::Approx( 90.f ) );

    // Dragging a scrollbar leaves focus where it is.
    REQUIRE( bar->GetTraits().Focus == EFocusPolicy::None );
}

TEST_CASE( "ScrollContainerWidget: an inner container at its end hands the wheel to the outer one", "[Scene][Scroll]" )
{
    RatUI::Scene scene;
    PanelWidget* root = scene.CreateRootWidget<PanelWidget>();

    ScrollContainerWidget* outer = scene.CreateWidget<ScrollContainerWidget>( root->GetLayoutID() );
    outer->GetLayout().FixedWidth( 200_u ).FixedHeight( 100_u );
    outer->SetHScrollbarMode( EScrollbarMode::Never );

    ScrollContainerWidget* inner = scene.CreateWidget<ScrollContainerWidget>( outer->GetContentNodeID() );
    inner->GetLayout().FixedWidth( 150_u ).FixedHeight( 80_u );
    inner->SetHScrollbarMode( EScrollbarMode::Never );
    scene.CreateWidget<PanelWidget>( inner->GetContentNodeID() )->GetLayout().FixedWidth( 100_u ).FixedHeight( 200_u );
    scene.CreateWidget<PanelWidget>( outer->GetContentNodeID() )->GetLayout().FixedWidth( 100_u ).FixedHeight( 200_u );
    scene.UpdateLayout( { 400_u, 300_u } );

    const auto wheel = [&]( Vec2<Unit> a_Position )
    {
        scene.DispatchInputEvent( { .Device = EDeviceID::Mouse, .Payload = PointerEvent{ .Position = a_Position, .Type = EPointerType::Mouse, .ScrollDelta = { 0_u, -1_u } } } );
    };

    // The inner one scrolls first.
    wheel( { 50_u, 40_u } );
    REQUIRE( inner->GetScrollOffset()[1].ToFloat() > 0.f );
    REQUIRE( outer->GetScrollOffset()[1].ToFloat() == Catch::Approx( 0.f ) );

    // At its end, a new gesture (the pointer moved) scrolls the outer one.
    inner->ScrollToEnd();
    const f32 innerEnd = inner->GetScrollOffset()[1].ToFloat();
    wheel( { 60_u, 40_u } );
    REQUIRE( inner->GetScrollOffset()[1].ToFloat() == Catch::Approx( innerEnd ) );
    REQUIRE( outer->GetScrollOffset()[1].ToFloat() == Catch::Approx( ScrollContainerWidget::c_WheelStep.ToFloat() ) );
}

TEST_CASE( "ScrollContainerWidget: the sideways wheel scrolls an axis whose scrollbar shows", "[Scene][Scroll]" )
{
    ScrollSceneFixture fixture;
    fixture.Scroll->SetHScrollbarMode( EScrollbarMode::Auto );
    fixture.Rows[0]->GetLayout().FixedWidth( 400_u );
    fixture.Frame();

    fixture.Wheel( { 50_u, 50_u }, { 1_u, 0_u } );
    REQUIRE( fixture.Scroll->GetScrollOffset()[0].ToFloat() == Catch::Approx( ScrollContainerWidget::c_WheelStep.ToFloat() ) );
    REQUIRE( fixture.Scroll->GetScrollOffset()[1].ToFloat() == Catch::Approx( 0.f ) );

    fixture.Scroll->ScrollToEnd( EOrient::Horizontal );
    REQUIRE( fixture.Scroll->GetScrollOffset()[0].ToFloat() > ScrollContainerWidget::c_WheelStep.ToFloat() );
}

TEST_CASE( "ScrollContainerWidget: a row under a bare layout node is painted", "[Scene][Scroll]" )
{
    struct PaintProbe : IWidget
    {
        int& Paints;
        explicit PaintProbe( int& a_Paints ) : Paints( a_Paints ) {}
        void OnPaint( const PaintEvent& ) override { ++Paints; }
    };

    ScrollSceneFixture fixture;
    LayoutNode&        group  = fixture.Scene.CreateLayoutNode( {}, fixture.Scroll->GetLayoutID() );
    int                paints = 0;
    PaintProbe* probe = fixture.Scene.CreateWidget<PaintProbe>( group.ID, paints );
    probe->GetLayout().FixedWidth( 100_u ).FixedHeight( 30_u );

    // It comes after the ten rows, so it is scrolled to.
    fixture.Frame();
    fixture.Scroll->SetScrollOffset( { 0_u, 230_u } );
    fixture.Frame();
    REQUIRE( paints == 1 );
}

TEST_CASE( "ScrollContainerWidget: rows scrolled out of view aren't painted", "[Scene][Scroll]" )
{
    struct PaintProbe : IWidget
    {
        int& Paints;
        explicit PaintProbe( int& a_Paints ) : Paints( a_Paints ) {}
        void OnPaint( const PaintEvent& ) override { ++Paints; }
    };

    ScrollSceneFixture fixture;

    // Below the ten rows, at 300..330 of the content.
    int         paints = 0;
    PaintProbe* last   = fixture.Scene.CreateWidget<PaintProbe>( fixture.Scroll->GetContentNodeID(), paints );
    last->GetLayout().FixedWidth( 100_u ).FixedHeight( 30_u );

    fixture.Frame();
    REQUIRE( paints == 0 );

    fixture.Scroll->SetScrollOffset( { 0_u, 230_u } );
    fixture.Frame();
    REQUIRE( paints == 1 );
}
