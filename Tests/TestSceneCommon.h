#pragma once

/**
 * @file TestSceneCommon.h
 * @brief Fixtures shared by the tests that drive a whole Scene: input helpers and a widget that counts what it is told.
 */

#include "TestLayoutCommon.h"
#include <RatUI/Widget/ButtonWidget.h>
#include <RatUI/Widget/InputTextWidget.h>
#include <RatUI/Widget/PanelWidget.h>
#include <RatUI/Widget/ScrollContainerWidget.h>
#include <RatUI/Widget/SliderWidget.h>

/** @brief A scene with desktop navigation bound and a vertical root panel. */
struct NavFixture
{
    RatUI::Scene Scene;
    PanelWidget* Root{ nullptr };

    NavFixture()
    {
        Scene.Input.NavMap.BindDefaultDesktop();
        Root = Scene.CreateRootWidget<PanelWidget>();
        Root->GetLayout().LayoutType( ELayoutType::Vertical ).Padding( Edges::All( 10_u ) ).Spacing( 10_u );
    }

    ButtonWidget* AddButton( NodeID a_Parent, int* a_Clicks = nullptr )
    {
        ButtonWidget* button = Scene.CreateWidget<ButtonWidget>( a_Parent, [a_Clicks]( ButtonBaseWidget& ) { if ( a_Clicks ) ++*a_Clicks; } );
        button->GetLayout().FixedWidth( 100_u ).FixedHeight( 40_u );
        return button;
    }

    void Layout() { Scene.UpdateLayout( { 400_u, 300_u } ); }

    bool Key( EButtonID a_Button, bool a_Pressed, EModifier a_Modifiers = EModifier::None )
    {
        return Scene.DispatchInputEvent( { .Device  = EDeviceID::Keyboard,
                                           .Payload = ButtonEvent{ .Button = a_Button, .Modifiers = a_Modifiers, .Pressed = a_Pressed, .Released = !a_Pressed } } );
    }

    /** @brief Presses and releases a key; returns whether the press was handled. */
    bool Tap( EButtonID a_Button, EModifier a_Modifiers = EModifier::None )
    {
        const bool handled = Key( a_Button, true, a_Modifiers );
        Key( a_Button, false, a_Modifiers );
        return handled;
    }

    void Click( Vec2<Unit> a_Position )
    {
        Scene.DispatchInputEvent( { .Device = EDeviceID::Mouse, .Payload = PointerEvent{ .Position = a_Position, .Type = EPointerType::Mouse } } );
        for ( const bool pressed : { true, false } )
        {
            Scene.DispatchInputEvent( { .Device  = EDeviceID::Mouse,
                                        .Payload = ButtonEvent{ .Button = EButtonID::MouseLeft, .Pressed = pressed, .Released = !pressed,
                                                                .Pointer = c_MousePointerID, .PointerPosition = a_Position } } );
        }
    }

    void Type( StringView a_Text )
    {
        Scene.DispatchInputEvent( { .Device = EDeviceID::Keyboard, .Payload = TextInputEvent{ .Text = String( a_Text ) } } );
    }
};

/** @brief What a ProbeWidget has been told. Kept outside the widget, so it can be read after the widget is destroyed. */
struct ProbeCounts
{
    int Measures{ 0 };
    int GeometryChanges{ 0 };
    int FocusLost{ 0 };
    int Disengaged{ 0 };
};

struct ProbeWidget : IWidget
{
    ProbeCounts&                   Counts;
    Callback<const GeometryEvent&> OnGeometry;

    ProbeWidget( ProbeCounts& a_Counts, const WidgetTraits& a_Traits = {} ) : IWidget( a_Traits ), Counts( a_Counts ) {}

    Vec2<Unit> OnMeasureContent( const LayoutNode&, Vec2<Unit>, const LayoutContext& ) override
    {
        ++Counts.Measures;
        return { 50_u, 20_u };
    }

    void OnGeometryChanged( const LayoutNode&, const GeometryEvent& a_Event ) override
    {
        ++Counts.GeometryChanges;
        if ( OnGeometry ) OnGeometry( a_Event );
    }

    void OnFocusLost( const FocusEvent& ) override { ++Counts.FocusLost; }
    void OnDisengage() override { ++Counts.Disengaged; }
};
