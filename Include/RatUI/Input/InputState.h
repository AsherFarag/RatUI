#pragma once
#include "../Core.h"
#include "InputEvent.h"
#include "Navigation.h"

namespace RatUI
{
    /**
     * @brief Whether input is currently being driven by pointer devices (mouse/touch/pen)
     * or by directional navigation (keyboard/gamepad). Widgets can query this to decide
     * whether to render focus rings, hover states, etc.
     */
    enum class EInputMode : u8
    {
        Pointer,
        Navigation
    };

    /**
     * @brief Tunable thresholds for gesture recognition (multi-click, drag, long-press).
     */
    struct GestureConfig
    {
        f64  MultiPressWindowSeconds{ 0.35 };
        Unit MultiPressDistance{ 6_u };
        Unit DragThreshold{ 4_u };
        f64  LongPressSeconds{ 0.5 };
    };

    struct PointerGestureState
    {
        Vec2<Unit> DownPosition{};
        f64        LastDownTime{ 0.0 };
        f64        LastPressTime{ 0.0 };
        u32        PressCount{ 0 };
        NodeID     PressedWidget{ c_InvalidNodeID };
        NodeID     CapturedWidget{ c_InvalidNodeID }; ///< Gets this pointer's events until it is released, wherever the pointer goes.
        bool       IsDown{ false };
        bool       IsDragging{ false };
        bool       LongPressFired{ false };
    };

    struct NavScope
    {
        NodeID Scope{ c_InvalidNodeID };    ///< The container widget
        NodeID Restored{ c_InvalidNodeID }; ///< The widget to restore on pop
    };

    /** @brief The widget that took a held key's press; the key's release goes to the same widget. */
    struct KeyPressTarget
    {
        NodeID Widget{ c_InvalidNodeID };
        bool   Activate{ false }; ///< Pressed through ENavAction::ActivatePressed, so release as ActivateReleased.
    };

    /**
     * @brief Represents the current state of input devices and navigation.
     */
    struct InputState
    {
        GestureConfig Config{};
        InputNavMap   NavMap{};
        EInputMode    InputMode{ EInputMode::Pointer };
        EModifier     ModifierState{ EModifier::None };

        NodeID FocusedWidget{ c_InvalidNodeID };
        NodeID HoveredWidget{ c_InvalidNodeID };

        PointerEvent LastPointerEvent{};
        HashMap<EButtonID, KeyPressTarget> KeyPresses{}; ///< Held keyboard keys whose press a widget took, so focus changes can't split a key stroke.
        bool DropNextTextInput{ false }; ///< The last key press was used by navigation, so the text it produces (e.g. Space's " ") is not typed.
        HashMap<PointerID, PointerGestureState> PointerStates{};
        Array<NavScope> NavStack{};

        // --------------------------------------------------------------------
        // Modifiers
        // --------------------------------------------------------------------

        /** @brief Call for every ButtonEvent (press or release) before dispatch, to keep modifier state current. */
        void UpdateModifiers( EButtonID a_Button, bool a_Pressed )
        {
            const EModifier bit = ModifierFor( a_Button );
            if ( bit == EModifier::None )
                return;

            if ( a_Pressed ) ModifierState |= bit;
            else             ModifierState &= ~bit;
        }

        // --------------------------------------------------------------------
        // Navigation Scope Stack
        // --------------------------------------------------------------------

        void PushNavScope( NodeID a_ScopeID, NodeID a_Restored = c_InvalidNodeID )
        {
            PushBack( NavStack, NavScope{ a_ScopeID, a_Restored } );
        }

        /** @brief Pops the top nav scope and returns it, or NullOpt if the stack was empty. */
        Optional<NavScope> PopNavScope()
        {
            if ( Empty( NavStack ) )
                return NullOpt;

            NavScope top = Back( NavStack );
            PopBack( NavStack );
            return top;
        }

        NodeID GetCurrentNavScope( NodeID a_DefaultRoot = c_InvalidNodeID ) const
        {
            return Empty( NavStack ) ? a_DefaultRoot : Back( NavStack ).Scope;
        }

        bool HasNavScopes() const { return !Empty( NavStack ); }

        // --------------------------------------------------------------------

        /** @brief Clears all input state. */
        void Reset() { *this = InputState{}; }
    };

} // namespace RatUI
