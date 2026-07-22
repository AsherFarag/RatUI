#pragma once
#include "../Core.h"

namespace RatUI
{
    using PointerID = u32;

    /** 
     * @brief Bitflag enum representing different input devices. Used in input events to indicate the source of the input.
     */ 
    enum class EDeviceID : u32
    {
        Unknown  = 0,
        Mouse    = 1 << 0,
        Touch    = 1 << 1,
        Pen      = 1 << 2,
        Keyboard = 1 << 3,
        Gamepad  = 1 << 4,
    };
    RATUI_ENUM_ENABLE_BITMASK_OPERATORS( EDeviceID, u32 )

    /**
     * @brief A Pointer is a generalized input representing a position and movement on a 2D plane, which can come from a mouse, touch, or pen input.
     * For Touch and Pen inputs, the PointerID can be used to track multiple simultaneous inputs (e.g. multiple fingers or pen + touch).
     */
    enum class EPointerType : u8
    {
        Unknown = 0,
        Mouse,
        Touch,
        Pen
    };

    enum class EModifier : u8
    {
        None  = 0,
        Shift = 1 << 0,
        Ctrl  = 1 << 1,
        Alt   = 1 << 2,
        Super = 1 << 3, ///< Windows / Command key
    };
    RATUI_ENUM_ENABLE_BITMASK_OPERATORS( EModifier, u8 )

    /**
     * @brief Enum representing various input buttons and controls across different device types.
     */
    enum class EButtonID : u32
    {
        Unknown = 0,

        // ========================
        // Mouse (1 - 99)
        // ========================
        Mouse0 = 1,
        Mouse1,
        Mouse2,
        Mouse3,
        Mouse4,
        Mouse5,
        Mouse6,
        Mouse7,

        MouseLeft   = Mouse0,
        MouseRight  = Mouse1,
        MouseMiddle = Mouse2,

        MouseWheelUp,
        MouseWheelDown,

        // ========================
        // Keyboard (100 - 199)
        // ========================
        KeyA = 100,
        KeyB, KeyC, KeyD, KeyE, KeyF, KeyG, KeyH, KeyI, KeyJ,
        KeyK, KeyL, KeyM, KeyN, KeyO, KeyP, KeyQ, KeyR, KeyS,
        KeyT, KeyU, KeyV, KeyW, KeyX, KeyY, KeyZ,

        Key0, Key1, Key2, Key3, Key4,
        Key5, Key6, Key7, Key8, Key9,

        KeyEnter,
        KeyEscape,
        KeySpace,
        KeyTab,
        KeyBackspace,

        KeyInsert,
        KeyDelete,
        KeyHome,
        KeyEnd,
        KeyPageUp,
        KeyPageDown,

        KeyLeft,
        KeyRight,
        KeyUp,
        KeyDown,

        KeyCapsLock,
        KeyScrollLock,
        KeyNumLock,
        KeyPrintScreen,
        KeyPause,

        KeyF1,  KeyF2,  KeyF3,  KeyF4,
        KeyF5,  KeyF6,  KeyF7,  KeyF8,
        KeyF9,  KeyF10, KeyF11, KeyF12,
        KeyF13, KeyF14, KeyF15, KeyF16,
        KeyF17, KeyF18, KeyF19, KeyF20,
        KeyF21, KeyF22, KeyF23, KeyF24,

        KeyLeftShift,
        KeyRightShift,
        KeyLeftCtrl,
        KeyRightCtrl,
        KeyLeftAlt,
        KeyRightAlt,
        KeyLeftSuper,   // Windows / Command
        KeyRightSuper,  // Windows / Command

        KeyMenu,        // Context menu key (if available)

        // ========================
        // Gamepad (200 - 299)
        // ========================
        GamepadA = 200,
        GamepadB,
        GamepadX,
        GamepadY,

        GamepadLeftBumper,
        GamepadRightBumper,

        GamepadBack,        // View / Select
        GamepadStart,       // Menu
        GamepadGuide,       // Xbox / PS / Home button

        GamepadLeftStick,   // Press
        GamepadRightStick,  // Press

        GamepadDPadUp,
        GamepadDPadDown,
        GamepadDPadLeft,
        GamepadDPadRight,

        GamepadLeftTrigger,     // Digital threshold press
        GamepadRightTrigger,    // Digital threshold press

        // ========================
        // Mobile / System (300 - 399)
        // ========================
        TouchTap = 300,     // Primary touch tap (for UI confirm abstraction)
        TouchLongPress,

        SystemBack,         // Android back
        SystemMenu,
        SystemHome,

        VolumeUp,
        VolumeDown,
        Power,

        // ========================
        // VR / XR (400 - 499)
        // ========================
        XRTrigger = 400,
        XRGrip,
        XRPrimaryButton,
        XRSecondaryButton,
        XRMenu,
    };

    inline constexpr bool IsPointerButton( EButtonID a_Button )
    {
        return a_Button >= EButtonID::Mouse0 && a_Button <= EButtonID::Mouse7;
    }

    /** @brief Maps a raw button to the modifier it represents, or EModifier::None if it isn't one. */
    inline constexpr EModifier ModifierFor( EButtonID a_Button )
    {
        switch ( a_Button )
        {
            case EButtonID::KeyLeftShift:
            case EButtonID::KeyRightShift: return EModifier::Shift;
            case EButtonID::KeyLeftCtrl:
            case EButtonID::KeyRightCtrl:  return EModifier::Ctrl;
            case EButtonID::KeyLeftAlt:
            case EButtonID::KeyRightAlt:   return EModifier::Alt;
            case EButtonID::KeyLeftSuper:
            case EButtonID::KeyRightSuper: return EModifier::Super;
            default:                       return EModifier::None;
        }
    }

	inline constexpr bool IsAlpha( EButtonID a_Button )
	{
		return a_Button >= EButtonID::KeyA && a_Button <= EButtonID::KeyZ;
	}

	inline constexpr bool IsDigit( EButtonID a_Button )
	{
		return a_Button >= EButtonID::Key0 && a_Button <= EButtonID::Key9;
	}

	inline constexpr bool IsAlphanumeric( EButtonID a_Button )
	{
		return IsAlpha( a_Button ) || IsDigit( a_Button );
	}

    struct KeyCharPair
    {
        char Normal{};
        char Shifted{};
    };

    inline constexpr auto KeyMap = []()
    {
        FixedArray<KeyCharPair, 256> map{};

        // Letters
        for ( u32 i = 0; i < 26; ++i )
        {
            map[(u32)EButtonID::KeyA + i] =
            {
                static_cast<char>( 'a' + i ),
                static_cast<char>( 'A' + i )
            };
        }

        // Digits
        constexpr char shiftedDigits[] =
        {
            ')', '!', '@', '#', '$',
            '%', '^', '&', '*', '('
        };

        for ( u32 i = 0; i < 10; ++i )
        {
            map[(u32)EButtonID::Key0 + i] =
            {
                static_cast<char>( '0' + i ),
                shiftedDigits[i]
            };
        }

        // Common whitespace / control keys
        map[(u32)EButtonID::KeySpace]     = { ' ' , ' ' };
        map[(u32)EButtonID::KeyTab]       = { '\t', '\t' };
        map[(u32)EButtonID::KeyEnter]     = { '\n', '\n' };
        map[(u32)EButtonID::KeyBackspace] = { '\b', '\b' };

        return map;
    }( );

    inline constexpr Optional<char> ToChar( EButtonID button, bool shift = false )
    {
        const auto index = static_cast<u32>( button );

		if ( index >= Size( KeyMap ) )
            return NullOpt;

        const auto& pair = KeyMap[index];

        if ( pair.Normal == '\0' )
            return NullOpt;

        return shift ? pair.Shifted : pair.Normal;
    }

    /**
     * @brief Pointer input event, which can come from a mouse, touch, or pen device. 
     * Contains position, movement delta, and device-specific data.
     */
    struct PointerEvent
    {
        Vec2<Unit>   Position{ 0_u, 0_u };
        Vec2<Unit>   Delta{ 0_u, 0_u };
        EPointerType Type{ EPointerType::Unknown };
        EModifier    Modifiers{ EModifier::None };

        PointerID  ID{ 0 };                 ///< TouchID or PenID or 0 for mouse
        Vec2<Unit> ScrollDelta{ 0_u, 0_u }; ///< Mouse
        f32        Pressure{ 0.f };         ///< Touch/pen pressure (0.0 to 1.0)
		Degrees    TiltX{ 0.f };            ///< Pen tilt X in degrees
		Degrees    TiltY{ 0.f };            ///< Pen tilt Y in degrees

        constexpr bool IsMouse() const { return Type == EPointerType::Mouse; }
        constexpr bool IsTouch() const { return Type == EPointerType::Touch; }
        constexpr bool IsPen()   const { return Type == EPointerType::Pen; }
    };

    /**
     * @brief Button input event, which can represent keyboard keys,
     * mouse buttons, gamepad buttons, or other digital inputs. 
     * Contains the button ID and its state (pressed, released, held).
     */
    struct ButtonEvent
    {
        EButtonID Button{ EButtonID::Unknown };
        EModifier Modifiers{ EModifier::None };
        bool Pressed{ false };   ///< True on the frame the button was pressed.
        bool Released{ false };  ///< True on the frame the button was released.
        bool Held{ false };      ///< True every frame the button is held down (including the pressed and released frames).

        Optional<PointerID>  Pointer{};
        Optional<Vec2<Unit>> PointerPosition;
    };

    // TODO: Clean this up
    struct TextInputEvent
    {
        //codepoint Character{}; ///< The Unicode code point of the character that was input.
        EButtonID Button{ EButtonID::Unknown };
        EModifier Modifiers{ EModifier::None };
    };

    /**
     * @brief General input event that can represent either a pointer event (mouse/touch/pen) or a button event (keyboard/gamepad).
     */
    struct InputEvent
    {
        EDeviceID Device{ EDeviceID::Unknown };
        Variant<Monostate, PointerEvent, ButtonEvent> Payload;
    };

    /**
     * @brief Reply structure for handling input events and managing their propagation.
     */
    class Reply
    {
    public:
        /** @brief Factory method to create an event that was not handled; propagate to next candidate. */
        static constexpr Reply Unhandled() { return Reply{ false }; }

        /** @brief Factory method to create an event that was handled; stop propagation. */
        static constexpr Reply Handled() { return Reply{ true }; }

        /** @brief Capture all future pointer events to this widget. */
        constexpr Reply& CaptureMouse( NodeID a_Widget )
        {
            m_MouseCapture = a_Widget;
            m_RequestCapture = true;
            return *this;
        }

        /** @brief Release mouse capture if currently held. */
        constexpr Reply& ReleaseMouseCapture()
        {
            m_ReleaseMouse = true;
            return *this;
        }

        /** @brief Set keyboard focus to the specified widget. */
        constexpr Reply& SetFocus( NodeID a_Widget )
        {
            m_FocusWidget = a_Widget;
            return *this;
        }

        /** @brief Clear keyboard focus if currently held. */
        constexpr Reply& ClearFocus()
        {
            m_ClearFocus = true;
            return *this;
        }

        /** @brief Prevent the default behavior associated with this event (e.g., text input, button clicks). */
        constexpr Reply& PreventDefault()
        {
            m_PreventDefault = true;
            return *this;
        }

        constexpr bool IsHandled()          const { return m_Handled; }
        constexpr bool ShouldCaptureMouse() const { return m_RequestCapture; }
        constexpr bool ShouldReleaseMouse() const { return m_ReleaseMouse; }
        constexpr bool ShouldSetFocus()     const { return m_FocusWidget != c_InvalidNodeID; }
        constexpr bool ShouldClearFocus()   const { return m_ClearFocus; }
        constexpr bool IsDefaultPrevented() const { return m_PreventDefault; }

        /** @brief Get the widget that is currently capturing mouse events. */
        constexpr NodeID GetMouseCaptureTarget() const { return m_MouseCapture; }
        /** @brief Get the widget that currently has keyboard focus. */
        constexpr NodeID GetFocusTarget()        const { return m_FocusWidget; }

    private:
        constexpr explicit Reply( bool a_Handled ) : m_Handled( a_Handled ) {}

        NodeID m_MouseCapture     { c_InvalidNodeID };
        NodeID m_FocusWidget      { c_InvalidNodeID };
        bool     m_Handled        : 1 { false };
        bool     m_RequestCapture : 1 { false };
        bool     m_ReleaseMouse   : 1 { false };
        bool     m_ClearFocus     : 1 { false };
        bool     m_PreventDefault : 1 { false };
    };

} // namespace RatUI
