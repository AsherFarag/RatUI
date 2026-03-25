#pragma once
#include "../Core.h"

namespace RatUI
{
    enum class EDeviceID : u32
    {
        Mouse = 1 << 0,
        Touch = 1 << 1,
        Pen   = 1 << 2,
        Keyboard = 1 << 3,
        Gamepad = 1 << 4,
    };

    enum class EButtonID : u32
    {
    };

    enum class EPointerType : u8
    {
        Unknown = 0,
        Mouse,
        Touch,
        Pen
    };

    struct PointerEvent
    {
        EPointerType Type{ EPointerType::Unknown };

        Vec2f Position{ 0.f, 0.f };
        Vec2f Delta{ 0.f, 0.f };

        union
        {
            struct 
            {
                Vec2f WheelDelta{ 0.f, 0.f };
            } Mouse;

            struct
            {
                u32 TouchID{ 0 };
                f32 Pressure{ 0.f };
            } Touch;

            struct 
            {
                u32 PenID{ 0 };
                f32 Pressure{ 0.f };
                f32 TiltX{ 0.f };
                f32 TiltY{ 0.f };
            } Pen{};
        };

        constexpr bool IsMouse() const { return Type == EPointerType::Mouse; }
        constexpr bool IsTouch() const { return Type == EPointerType::Touch; }
        constexpr bool IsPen() const { return Type == EPointerType::Pen; }
    };

    struct ButtonEvent
    {
        EDeviceID Device{ 0 };
        EButtonID Button{ 0 };
    };

    struct InputEvent
    {
        EDeviceID Device{ 0 };
    
        // Common fields for all input events
        bool Pressed{ false };
        bool Released{ false };
        bool Held{ false };
    
        // Device-specific data
        union
        {
            PointerEvent Pointer;
            ButtonEvent Button;
        };  
    };


} // namespace RatUI
