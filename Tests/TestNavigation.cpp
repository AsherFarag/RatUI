#include "TestSceneCommon.h"

/**
 * @file TestNavigation.cpp
 * @brief Keyboard and gamepad navigation: moving focus, groups and scopes, activation, and engaging text fields.
 */

namespace
{
    /** @brief A button, then a panel holding two more buttons, stacked vertically. The panel is a nav group if asked. */
    struct PanelNavFixture : NavFixture
    {
        ButtonWidget* Top{ nullptr };
        PanelWidget*  Group{ nullptr };
        ButtonWidget* Inner[2]{};

        explicit PanelNavFixture( bool a_Group )
        {
            Top   = AddButton( Root->GetLayoutID() );
            Group = Scene.CreateWidget<PanelWidget>( Root->GetLayoutID() );
            Group->GetLayout().LayoutType( ELayoutType::Vertical );
            Group->SetNavGroup( a_Group );
            for ( ButtonWidget*& button : Inner )
                button = AddButton( Group->GetLayoutID() );

            Layout();
        }
    };

    /** @brief A button, a single-line text field and another button, stacked vertically. */
    struct TextFieldNavFixture : NavFixture
    {
        ButtonWidget*    Top{ nullptr };
        InputTextWidget* Field{ nullptr };
        ButtonWidget*    Bottom{ nullptr };
        String           Submitted;

        TextFieldNavFixture()
        {
            Top   = AddButton( Root->GetLayoutID() );
            Field = Scene.CreateWidget<InputTextWidget>( Root->GetLayoutID() );
            Field->GetLayout().FixedWidth( 100_u ).FixedHeight( 30_u );
            Field->OnSubmit = [this]( const String& a_Text ) { Submitted = a_Text; };
            Bottom = AddButton( Root->GetLayoutID() );

            Layout();
        }
    };

    /** @brief A panel that takes one key, to see keys bubble up from a focused child. */
    class KeyCatcherPanel : public PanelWidget
    {
    public:
        int Caught{ 0 };

        Reply OnButtonPressed( const ButtonEvent& a_Event ) override
        {
            if ( a_Event.Button != EButtonID::KeyF2 )
                return Reply::Unhandled();
            ++Caught;
            return Reply::Handled();
        }
    };

    /** @brief A panel that keeps navigation inside it: moving past either end wraps round to the other. */
    struct WrapPanel : PanelWidget
    {
        NavReply OnNavigationBoundary( const NavEvent& ) override { return NavReply::Wrap(); }
    };
}

// ============================================================================
// Moving focus
// ============================================================================

TEST_CASE( "Navigation: arrows move between buttons, the first press only picks the start", "[Navigation]" )
{
    NavFixture    f;
    ButtonWidget* first  = f.AddButton( f.Root->GetLayoutID() );
    ButtonWidget* second = f.AddButton( f.Root->GetLayoutID() );
    f.Layout();

    f.Tap( EButtonID::KeyDown );
    REQUIRE( f.Scene.GetFocusedNode() == first->GetLayoutID() );

    f.Tap( EButtonID::KeyDown );
    REQUIRE( f.Scene.GetFocusedNode() == second->GetLayoutID() );

    f.Tap( EButtonID::KeyUp );
    REQUIRE( f.Scene.GetFocusedNode() == first->GetLayoutID() );
}

TEST_CASE( "Navigation: a pointer-only widget is skipped by arrows and Tab but focused by a press", "[Navigation]" )
{
    NavFixture    f;
    ButtonWidget* first  = f.AddButton( f.Root->GetLayoutID() );
    ButtonWidget* middle = f.AddButton( f.Root->GetLayoutID() );
    ButtonWidget* last   = f.AddButton( f.Root->GetLayoutID() );
    middle->SetFocusPolicy( EFocusPolicy::Pointer );
    f.Layout();

    f.Scene.SetFocus( first->GetLayoutID() );
    f.Tap( EButtonID::KeyDown );
    REQUIRE( f.Scene.GetFocusedNode() == last->GetLayoutID() );

    f.Tap( EButtonID::KeyTab );
    REQUIRE( f.Scene.GetFocusedNode() == first->GetLayoutID() );

    f.Click( { 50_u, 85_u } );
    REQUIRE( f.Scene.GetFocusedNode() == middle->GetLayoutID() );
}

TEST_CASE( "Scene: a widget focused before it is laid out is brought into view once it is", "[Navigation]" )
{
    NavFixture             f;
    ScrollContainerWidget* scroll = f.Scene.CreateWidget<ScrollContainerWidget>( f.Root->GetLayoutID() );
    for ( int i = 0; i < 12; ++i )
        f.AddButton( scroll->GetLayoutID() );
    f.Layout();

    f.Scene.Input.InputMode = EInputMode::Navigation;
    ButtonWidget* added = f.AddButton( scroll->GetLayoutID() );
    f.Scene.SetFocus( added->GetLayoutID() );
    REQUIRE_UNIT( scroll->GetScrollOffset()[1], 0.f ); // Where it is isn't known yet.

    f.Layout();
    REQUIRE( scroll->GetMaxScrollOffset()[1] > 0_u );
    REQUIRE_UNIT( scroll->GetScrollOffset()[1], scroll->GetMaxScrollOffset()[1].ToFloat() );
    REQUIRE( added->GetLayout().Layout.SceneRect.Bottom() <= scroll->GetContentNode()->Layout.SceneRect.Bottom() );
}

TEST_CASE( "InputNavMap: the default gamepad bindings move with the D-pad", "[Navigation]" )
{
    InputNavMap map;
    map.BindDefaultGamepad();

    REQUIRE( map.Resolve( EButtonID::GamepadDPadLeft ) == ENavAction::MoveLeft );
    REQUIRE( map.Resolve( EButtonID::GamepadDPadRight ) == ENavAction::MoveRight );
    REQUIRE( map.Resolve( EButtonID::GamepadDPadUp ) == ENavAction::MoveUp );
    REQUIRE( map.Resolve( EButtonID::GamepadDPadDown ) == ENavAction::MoveDown );
    REQUIRE( map.Resolve( EButtonID::GamepadLeftStick ) == ENavAction::None );
}

TEST_CASE( "Scene: a key the focused widget leaves unhandled bubbles to its ancestors", "[Scene][Navigation]" )
{
    RatUI::Scene scene;
    KeyCatcherPanel* root = scene.CreateRootWidget<KeyCatcherPanel>();
    ButtonWidget* button = scene.CreateWidget<ButtonWidget>( root->GetLayoutID() );
    button->GetLayout().FixedWidth( 100_u ).FixedHeight( 30_u );
    scene.UpdateLayout( { 400_u, 300_u } );
    scene.SetFocus( button->GetLayoutID() );

    scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = ButtonEvent{ .Button = EButtonID::KeyF2, .Pressed = true } } );
    REQUIRE( root->Caught == 1 );
}

// ============================================================================
// Groups and scopes
// ============================================================================

TEST_CASE( "PanelWidget: a plain panel is passed through by navigation", "[Scene][Navigation]" )
{
    PanelNavFixture fixture{ false };
    REQUIRE( !fixture.Group->IsFocusable() );

    fixture.Scene.SetFocus( fixture.Top->GetLayoutID() );
    fixture.Tap( EButtonID::KeyDown );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Inner[0]->GetLayoutID() );
    fixture.Tap( EButtonID::KeyTab );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Inner[1]->GetLayoutID() );
}

TEST_CASE( "PanelWidget: a panel made a nav group is entered and left", "[Scene][Navigation]" )
{
    PanelNavFixture fixture{ true };
    REQUIRE( fixture.Group->IsFocusable() );

    fixture.Scene.SetFocus( fixture.Top->GetLayoutID() );
    fixture.Tap( EButtonID::KeyDown );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Group->GetLayoutID() );

    fixture.Tap( EButtonID::KeyEnter );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Inner[0]->GetLayoutID() );
    REQUIRE( fixture.Scene.GetCurrentNavScope() == fixture.Group->GetLayoutID() );
    fixture.Tap( EButtonID::KeyDown );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Inner[1]->GetLayoutID() );

    fixture.Tap( EButtonID::KeyEscape );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Group->GetLayoutID() );
    REQUIRE( fixture.Scene.GetCurrentNavScope() == fixture.Root->GetLayoutID() );
}

TEST_CASE( "IWidget: a group that stops being one is left by navigation", "[Scene][Navigation]" )
{
    PanelNavFixture fixture{ true };
    fixture.Scene.SetFocus( fixture.Inner[0]->GetLayoutID() );
    REQUIRE( fixture.Scene.GetCurrentNavScope() == fixture.Group->GetLayoutID() );

    fixture.Group->SetNavGroup( false );
    REQUIRE( fixture.Scene.GetCurrentNavScope() != fixture.Group->GetLayoutID() );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Inner[0]->GetLayoutID() );
}

TEST_CASE( "Navigation: Tab walks into and out of groups", "[Navigation]" )
{
    NavFixture    f;
    ButtonWidget* before = f.AddButton( f.Root->GetLayoutID() );
    PanelWidget*  group  = f.Scene.CreateWidget<PanelWidget>( f.Root->GetLayoutID() );
    group->SetNavGroup( true );
    group->GetLayout().LayoutType( ELayoutType::Vertical ).FixedWidth( 200_u ).FixedHeight( 100_u );
    ButtonWidget* innerA = f.AddButton( group->GetLayoutID() );
    ButtonWidget* innerB = f.AddButton( group->GetLayoutID() );
    ButtonWidget* after  = f.AddButton( f.Root->GetLayoutID() );
    f.Layout();

    f.Tap( EButtonID::KeyTab );
    REQUIRE( f.Scene.GetFocusedNode() == before->GetLayoutID() );

    f.Tap( EButtonID::KeyTab );
    REQUIRE( f.Scene.GetFocusedNode() == innerA->GetLayoutID() );
    REQUIRE( f.Scene.GetCurrentNavScope() == group->GetLayoutID() );

    f.Tap( EButtonID::KeyTab );
    REQUIRE( f.Scene.GetFocusedNode() == innerB->GetLayoutID() );

    f.Tap( EButtonID::KeyTab );
    REQUIRE( f.Scene.GetFocusedNode() == after->GetLayoutID() );
    REQUIRE( f.Scene.GetCurrentNavScope() == f.Root->GetLayoutID() );

    f.Tap( EButtonID::KeyTab );
    REQUIRE( f.Scene.GetFocusedNode() == before->GetLayoutID() );

    f.Tap( EButtonID::KeyTab, EModifier::LShift );
    REQUIRE( f.Scene.GetFocusedNode() == after->GetLayoutID() );

    // From the group itself, Tab goes on to what is inside it.
    f.Scene.SetFocus( group->GetLayoutID() );
    f.Tap( EButtonID::KeyTab );
    REQUIRE( f.Scene.GetFocusedNode() == innerA->GetLayoutID() );
}

TEST_CASE( "Navigation: a scope that wraps goes round to its other end", "[Navigation]" )
{
    NavFixture f;
    WrapPanel* group = f.Scene.CreateWidget<WrapPanel>( f.Root->GetLayoutID() );
    group->SetNavGroup( true );
    group->GetLayout().LayoutType( ELayoutType::Vertical ).FixedWidth( 200_u ).FixedHeight( 150_u );
    ButtonWidget* top    = f.AddButton( group->GetLayoutID() );
    f.AddButton( group->GetLayoutID() );
    ButtonWidget* bottom = f.AddButton( group->GetLayoutID() );
    f.Layout();

    f.Scene.SetFocus( top->GetLayoutID() );
    f.Tap( EButtonID::KeyUp );
    REQUIRE( f.Scene.GetFocusedNode() == bottom->GetLayoutID() );

    f.Tap( EButtonID::KeyDown );
    REQUIRE( f.Scene.GetFocusedNode() == top->GetLayoutID() );
}

TEST_CASE( "Scene: a scope pushed by hand decides what happens at its edges", "[Scene][Navigation]" )
{
    RatUI::Scene scene;
    scene.Input.NavMap.BindDefaultDesktop();
    PanelWidget* root = scene.CreateRootWidget<PanelWidget>();
    root->GetLayout().LayoutType( ELayoutType::Vertical );

    ButtonWidget* behind = scene.CreateWidget<ButtonWidget>( root->GetLayoutID() );
    behind->GetLayout().FixedWidth( 100_u ).FixedHeight( 30_u );

    WrapPanel* modal = scene.CreateWidget<WrapPanel>( root->GetLayoutID() );
    modal->GetLayout().LayoutType( ELayoutType::Vertical );
    ButtonWidget* first  = scene.CreateWidget<ButtonWidget>( modal->GetLayoutID() );
    ButtonWidget* second = scene.CreateWidget<ButtonWidget>( modal->GetLayoutID() );
    first->GetLayout().FixedWidth( 100_u ).FixedHeight( 30_u );
    second->GetLayout().FixedWidth( 100_u ).FixedHeight( 30_u );
    scene.UpdateLayout( { 400_u, 300_u } );

    scene.SetFocus( behind->GetLayoutID() );
    scene.PushNavScope( modal->GetLayoutID() );
    scene.SetFocus( second->GetLayoutID() );

    // Down from the last button: Wrap stays inside the modal rather than leaving it.
    scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = ButtonEvent{ .Button = EButtonID::KeyDown, .Pressed = true } } );
    REQUIRE( scene.GetFocusedNode() == first->GetLayoutID() );
    REQUIRE( scene.GetCurrentNavScope() == modal->GetLayoutID() );
}

// ============================================================================
// Activation and what the host gets back
// ============================================================================

TEST_CASE( "Navigation: every activation key clicks a button exactly once", "[Navigation]" )
{
    NavFixture    f;
    int           clicks = 0;
    ButtonWidget* button = f.AddButton( f.Root->GetLayoutID(), &clicks );
    f.Scene.Input.NavMap.BindDefaultGamepad();
    f.Layout();
    f.Scene.SetFocus( button->GetLayoutID() );

    REQUIRE( f.Tap( EButtonID::KeyEnter ) );
    REQUIRE( clicks == 1 );

    REQUIRE( f.Tap( EButtonID::KeySpace ) );
    REQUIRE( clicks == 2 );

    REQUIRE( f.Tap( EButtonID::GamepadA ) );
    REQUIRE( clicks == 3 );

    // Holding the key down doesn't click again.
    f.Key( EButtonID::KeyEnter, true );
    f.Scene.DispatchInputEvent( { .Device  = EDeviceID::Keyboard,
                                  .Payload = ButtonEvent{ .Button = EButtonID::KeyEnter, .Pressed = true, .Repeat = true } } );
    f.Key( EButtonID::KeyEnter, false );
    REQUIRE( clicks == 4 );
}

TEST_CASE( "Navigation: a key that isn't bound to Activate doesn't click", "[Navigation]" )
{
    NavFixture    f;
    int           clicks = 0;
    ButtonWidget* button = f.AddButton( f.Root->GetLayoutID(), &clicks );
    Erase( f.Scene.Input.NavMap.ButtonMap, EButtonID::KeyEnter );
    f.Layout();
    f.Scene.SetFocus( button->GetLayoutID() );

    REQUIRE_FALSE( f.Tap( EButtonID::KeyEnter ) );
    REQUIRE( clicks == 0 );
}

TEST_CASE( "Navigation: keys navigation has no use for are left to the host", "[Navigation]" )
{
    NavFixture    f;
    ButtonWidget* button = f.AddButton( f.Root->GetLayoutID() );
    f.Layout();

    // Nothing to cancel, and Cancel doesn't pick a widget to focus.
    REQUIRE_FALSE( f.Key( EButtonID::KeyEscape, true ) );
    REQUIRE_FALSE( f.Key( EButtonID::KeyEscape, false ) );
    REQUIRE( f.Scene.GetFocusedNode() == c_InvalidNodeID );

    f.Scene.SetFocus( button->GetLayoutID() );
    REQUIRE_FALSE( f.Key( EButtonID::KeyRight, true ) );
    REQUIRE( f.Scene.GetFocusedNode() == button->GetLayoutID() );

    // A press navigation used takes its release with it.
    REQUIRE( f.Key( EButtonID::KeyTab, true ) );
    REQUIRE( f.Key( EButtonID::KeyTab, false ) );
}

TEST_CASE( "Navigation: a key's release goes to the widget that took its press", "[Navigation]" )
{
    NavFixture    f;
    int           firstClicks = 0, secondClicks = 0;
    ButtonWidget* first  = f.AddButton( f.Root->GetLayoutID(), &firstClicks );
    ButtonWidget* second = f.AddButton( f.Root->GetLayoutID(), &secondClicks );
    f.Layout();

    f.Scene.SetFocus( first->GetLayoutID() );
    f.Key( EButtonID::KeyEnter, true );
    f.Scene.SetFocus( second->GetLayoutID() );
    f.Key( EButtonID::KeyEnter, false );

    REQUIRE( firstClicks == 1 );
    REQUIRE( secondClicks == 0 );
}

// ============================================================================
// Text fields and engaging
// ============================================================================

TEST_CASE( "InputTextWidget: navigating to a text field doesn't start editing it", "[Scene][Navigation][Text]" )
{
    TextFieldNavFixture fixture;
    fixture.Scene.SetFocus( fixture.Top->GetLayoutID() );

    fixture.Tap( EButtonID::KeyDown );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Field->GetLayoutID() );
    REQUIRE( !fixture.Field->IsEditing() );
    REQUIRE( !fixture.Scene.WantsTextInput() );

    fixture.Type( "a" );
    REQUIRE( fixture.Field->GetText().empty() );

    // Arrows keep navigating past it.
    fixture.Tap( EButtonID::KeyDown );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Bottom->GetLayoutID() );
}

TEST_CASE( "InputTextWidget: Enter starts editing and Escape stops", "[Scene][Navigation][Text]" )
{
    TextFieldNavFixture fixture;
    fixture.Scene.SetFocus( fixture.Field->GetLayoutID() );

    fixture.Tap( EButtonID::KeyEnter );
    REQUIRE( fixture.Field->IsEditing() );
    fixture.Type( "hi" );
    REQUIRE( fixture.Field->GetText() == "hi" );

    // Up and Down don't leave a single-line field while editing.
    fixture.Tap( EButtonID::KeyDown );
    REQUIRE( fixture.Field->IsEditing() );

    fixture.Tap( EButtonID::KeyEscape );
    REQUIRE_FALSE( fixture.Field->IsEditing() );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Field->GetLayoutID() );
    REQUIRE( fixture.Field->GetText() == "hi" );
}

TEST_CASE( "Navigation: a text field types only while engaged, and the key that engaged it isn't typed", "[Navigation]" )
{
    NavFixture       f;
    InputTextWidget* field = f.Scene.CreateWidget<InputTextWidget>( f.Root->GetLayoutID() );
    f.Layout();

    f.Tap( EButtonID::KeyDown );
    REQUIRE( f.Scene.GetFocusedNode() == field->GetLayoutID() );
    REQUIRE_FALSE( f.Scene.WantsTextInput() );

    f.Type( "x" );
    REQUIRE( field->GetText().empty() );

    // Space activates, and the " " the platform sends for it is dropped.
    f.Key( EButtonID::KeySpace, true );
    f.Type( " " );
    f.Key( EButtonID::KeySpace, false );
    REQUIRE( field->IsEditing() );
    REQUIRE( f.Scene.WantsTextInput() );
    REQUIRE( field->GetText().empty() );

    f.Type( "a" );
    REQUIRE( field->GetText() == "a" );

    f.Tap( EButtonID::KeyEscape );
    REQUIRE_FALSE( field->IsEditing() );
    REQUIRE( f.Scene.GetFocusedNode() == field->GetLayoutID() );
}

TEST_CASE( "InputTextWidget: Enter in a single-line field submits and stops editing", "[Scene][Navigation][Text]" )
{
    TextFieldNavFixture fixture;
    fixture.Scene.SetFocus( fixture.Field->GetLayoutID() );

    fixture.Tap( EButtonID::KeyEnter );
    fixture.Type( "go" );
    fixture.Tap( EButtonID::KeyEnter );
    REQUIRE( fixture.Submitted == "go" );
    REQUIRE_FALSE( fixture.Field->IsEditing() );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Field->GetLayoutID() );
}

TEST_CASE( "InputTextWidget: Tab while editing moves on to the next widget", "[Scene][Navigation][Text]" )
{
    TextFieldNavFixture fixture;
    fixture.Scene.SetFocus( fixture.Field->GetLayoutID() );

    fixture.Tap( EButtonID::KeyEnter );
    fixture.Tap( EButtonID::KeyTab );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Bottom->GetLayoutID() );
}

TEST_CASE( "InputTextWidget: clicking a text field starts editing it", "[Scene][Text]" )
{
    TextFieldNavFixture fixture;

    fixture.Click( { 50_u, 75_u } ); // The field is the second row.
    REQUIRE( fixture.Field->IsEditing() );
    fixture.Type( "x" );
    REQUIRE( fixture.Field->GetText() == "x" );
}

TEST_CASE( "InputTextWidget: submitting doesn't close another scope when OnSubmit moves focus", "[Scene][Navigation][Text]" )
{
    TextFieldNavFixture fixture;

    // The root stands in for a dialog, pushed while Top had focus.
    fixture.Scene.SetFocus( fixture.Top->GetLayoutID() );
    fixture.Scene.PushNavScope( fixture.Top->GetLayout().Parent()->ID );
    fixture.Field->OnSubmit = [&]( const String& ) { fixture.Scene.SetFocus( fixture.Bottom->GetLayoutID() ); };

    fixture.Scene.SetFocus( fixture.Field->GetLayoutID() );
    fixture.Tap( EButtonID::KeyEnter );
    fixture.Type( "ok" );
    fixture.Tap( EButtonID::KeyEnter );

    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Bottom->GetLayoutID() );
    REQUIRE_FALSE( fixture.Field->IsEditing() );
    REQUIRE( fixture.Scene.GetCurrentNavScope() == fixture.Top->GetLayout().Parent()->ID );
}

TEST_CASE( "Scene: focus moving away disengages, and only engageable widgets engage", "[Scene][Navigation]" )
{
    TextFieldNavFixture fixture;

    fixture.Scene.Engage( fixture.Field->GetLayoutID() );
    REQUIRE( fixture.Field->IsEditing() );
    REQUIRE( fixture.Scene.WantsTextInput() );

    fixture.Scene.SetFocus( fixture.Bottom->GetLayoutID() );
    REQUIRE( !fixture.Field->IsEditing() );
    REQUIRE( !fixture.Scene.IsEngaged( fixture.Field->GetLayoutID() ) );
    REQUIRE( !fixture.Scene.WantsTextInput() );

    // Buttons can't be engaged.
    fixture.Scene.Engage( fixture.Bottom->GetLayoutID() );
    REQUIRE( !fixture.Bottom->IsEngaged() );
}

TEST_CASE( "Scene: an engaged text field gets keys before its ancestors", "[Scene][Navigation][Text]" )
{
    NavFixture       f;
    KeyCatcherPanel* panel = f.Scene.CreateWidget<KeyCatcherPanel>( f.Root->GetLayoutID() );
    InputTextWidget* field = f.Scene.CreateWidget<InputTextWidget>( panel->GetLayoutID() );
    f.Layout();
    f.Scene.SetFocus( field->GetLayoutID() );

    // Not editing, the field passes the key up.
    f.Tap( EButtonID::KeyF2 );
    REQUIRE( panel->Caught == 1 );

    // Editing, it keeps every key its policy doesn't leave to navigation.
    f.Scene.Engage( field->GetLayoutID() );
    f.Tap( EButtonID::KeyF2 );
    REQUIRE( panel->Caught == 1 );
    REQUIRE( field->IsEditing() );
}

TEST_CASE( "InputTextWidget: holding Enter submits once", "[Scene][Navigation][Text]" )
{
    TextFieldNavFixture fixture;
    int                 submits = 0;
    fixture.Field->OnSubmit     = [&]( const String& ) { ++submits; };
    fixture.Scene.Engage( fixture.Field->GetLayoutID() );

    fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = ButtonEvent{ .Button = EButtonID::KeyEnter, .Pressed = true, .Repeat = true } } );
    REQUIRE( submits == 0 );
    REQUIRE( fixture.Field->IsEditing() );

    fixture.Key( EButtonID::KeyEnter, true );
    fixture.Scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = ButtonEvent{ .Button = EButtonID::KeyEnter, .Pressed = true, .Repeat = true } } );
    fixture.Key( EButtonID::KeyEnter, false );
    REQUIRE( submits == 1 );
    REQUIRE_FALSE( fixture.Field->IsEditing() );
}

TEST_CASE( "InputTextWidget: OnChanged reports each edit, and SetText normalises line breaks", "[Scene][Text]" )
{
    TextFieldNavFixture fixture;
    Array<String>       changes;
    fixture.Field->OnChanged = [&]( const String& a_Text ) { PushBack( changes, a_Text ); };
    fixture.Scene.Engage( fixture.Field->GetLayoutID() );

    fixture.Type( "a" );
    fixture.Type( "\xC3\xA9" );
    fixture.Tap( EButtonID::KeyBackspace );
    REQUIRE( changes == Array<String>{ "a", "a\xC3\xA9", "a" } );

    // A key that changes nothing reports nothing.
    fixture.Tap( EButtonID::KeyDelete );
    REQUIRE( Size( changes ) == 3 );

    fixture.Field->SetText( "x\r\ny" );
    REQUIRE( fixture.Field->GetText() == "x\ny" );
    REQUIRE( fixture.Field->GetModel().Caret() == 3 );
    REQUIRE( Back( changes ) == "x\ny" );
}

TEST_CASE( "InputTextWidget: a single-line field centres its line, a multi-line one starts at the top", "[Scene][Text]" )
{
    // Without text metrics the line is the default 20 tall.
    NavFixture       f;
    InputTextWidget* single = f.Scene.CreateWidget<InputTextWidget>( f.Root->GetLayoutID() );
    InputTextWidget* multi  = f.Scene.CreateWidget<InputTextWidget>( f.Root->GetLayoutID(), MakeUnique<MultiLineTextEditPolicy>() );
    single->GetLayout().FixedHeight( 40_u ).Padding( Edges::All( 4_u ) );
    multi->GetLayout().FixedHeight( 40_u ).Padding( Edges::All( 4_u ) );
    f.Layout();

    const Rect<Unit>& singleRect = single->GetLayout().Layout.SceneRect;
    REQUIRE_UNIT( single->GetTextRect().Origin[1], singleRect.Origin[1].ToFloat() + 4.f + 6.f ); // (32 - 20) / 2
    REQUIRE_UNIT( multi->GetTextRect().Origin[1], multi->GetLayout().Layout.SceneRect.Origin[1].ToFloat() + 4.f );

    // A content-sized field is one line plus its padding, counted once.
    InputTextWidget* fitted = f.Scene.CreateWidget<InputTextWidget>( f.Root->GetLayoutID() );
    fitted->GetLayout().Padding( Edges::All( 4_u ) );
    f.Layout();
    REQUIRE_UNIT( fitted->GetLayout().Layout.SceneRect.Size[1], 28.f );
}

TEST_CASE( "InputTextWidget: ShowsPlaceholder only while the field is empty", "[Text]" )
{
    InputTextWidget field;
    REQUIRE_FALSE( field.ShowsPlaceholder() ); // No placeholder.

    field.SetPlaceholder( "Name" );
    REQUIRE( field.ShowsPlaceholder() );

    field.SetText( "Rat" );
    REQUIRE_FALSE( field.ShowsPlaceholder() );
}

// ============================================================================
// Tab order: scopes, ends and collapsed widgets
// ============================================================================

TEST_CASE( "Navigation: Shift+Tab with nothing focused goes to the last stop", "[Navigation]" )
{
    NavFixture f;
    f.AddButton( f.Root->GetLayoutID() );
    ButtonWidget* last = f.AddButton( f.Root->GetLayoutID() );
    f.Layout();

    f.Tap( EButtonID::KeyTab, EModifier::LShift );
    REQUIRE( f.Scene.GetFocusedNode() == last->GetLayoutID() );
}

TEST_CASE( "Navigation: Tab skips collapsed widgets", "[Navigation]" )
{
    NavFixture    f;
    ButtonWidget* a = f.AddButton( f.Root->GetLayoutID() );
    ButtonWidget* b = f.AddButton( f.Root->GetLayoutID() );
    ButtonWidget* c = f.AddButton( f.Root->GetLayoutID() );
    b->GetLayout().Visibility( EVisibility::Collapsed );
    f.Layout();

    f.Scene.SetFocus( a->GetLayoutID() );
    f.Tap( EButtonID::KeyTab );
    REQUIRE( f.Scene.GetFocusedNode() == c->GetLayoutID() );
}

TEST_CASE( "Navigation: Tab stays inside a pushed scope", "[Navigation]" )
{
    NavFixture    f;
    ButtonWidget* behind = f.AddButton( f.Root->GetLayoutID() );
    PanelWidget*  modal  = f.Scene.CreateWidget<PanelWidget>( f.Root->GetLayoutID() );
    modal->GetLayout().LayoutType( ELayoutType::Vertical );
    ButtonWidget* first  = f.AddButton( modal->GetLayoutID() );
    ButtonWidget* second = f.AddButton( modal->GetLayoutID() );
    f.Layout();

    f.Scene.SetFocus( behind->GetLayoutID() );
    f.Scene.PushNavScope( modal->GetLayoutID() );
    f.Scene.SetFocus( second->GetLayoutID() );

    f.Tap( EButtonID::KeyTab );
    REQUIRE( f.Scene.GetFocusedNode() == first->GetLayoutID() );

    f.Tap( EButtonID::KeyTab, EModifier::LShift );
    REQUIRE( f.Scene.GetFocusedNode() == second->GetLayoutID() );
}

TEST_CASE( "Navigation: Shift+Tab while editing stops editing and moves back", "[Navigation][Text]" )
{
    TextFieldNavFixture fixture;
    fixture.Scene.SetFocus( fixture.Field->GetLayoutID() );
    fixture.Tap( EButtonID::KeyEnter );
    REQUIRE( fixture.Field->IsEditing() );

    fixture.Tap( EButtonID::KeyTab, EModifier::LShift );
    REQUIRE( fixture.Scene.GetFocusedNode() == fixture.Top->GetLayoutID() );
    REQUIRE_FALSE( fixture.Field->IsEditing() );
}
