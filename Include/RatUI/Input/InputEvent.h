#pragma once
#include "../Core.h"

namespace RatUI
{
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

    /**
     * @brief Pointer input event, which can come from a mouse, touch, or pen device. Contains position, movement delta, and device-specific data.
     */
    struct PointerEvent
    {
        Vec2f Position{ 0.f, 0.f };
        Vec2f Delta{ 0.f, 0.f };
        EPointerType Type{ EPointerType::Unknown };

        u32      PointerID{ 0 };          // TouchID or PenID or 0 for mouse
        Vec2f    ScrollDelta{ 0.f, 0.f }; // Mouse
        f32      Pressure{ 0.f };         // Touch/pen pressure (0.0 to 1.0)
		Degreesf TiltX{ 0.f };            // Pen tilt X in degrees
		Degreesf TiltY{ 0.f };            // Pen tilt Y in degrees

        constexpr bool IsMouse() const { return Type == EPointerType::Mouse; }
        constexpr bool IsTouch() const { return Type == EPointerType::Touch; }
        constexpr bool IsPen() const { return Type == EPointerType::Pen; }
    };

    /**
     * @brief Button input event, which can represent keyboard keys, mouse buttons, gamepad buttons, or other digital inputs. Contains the button ID and its state (pressed, released, held).
     */
    struct ButtonEvent
    {
        EButtonID Button{ EButtonID::Unknown };
        bool Pressed{ false };   ///< True on the frame the button was pressed.
        bool Released{ false };  ///< True on the frame the button was released.
        bool Held{ false };      ///< True every frame the button is held down (including the pressed and released frames).
    };

    /**
     * @brief General input event that can represent either a pointer event (mouse/touch/pen) or a button event (keyboard/gamepad).
     */
    struct InputEvent
    {
        EDeviceID Device{ EDeviceID::Unknown };
        Variant<Monostate, PointerEvent, ButtonEvent> Payload;
    };

} // namespace RatUI
