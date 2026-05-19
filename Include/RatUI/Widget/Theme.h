#pragma once
#include "../Core.h"
#include "../Text/Text.h"
#include "../Layout/Layout.h" // TODO: Remove once CornerRounding is moved to a more appropriate header

namespace RatUI
{
    using Name = StringView; // TODO: Make this a compile-time string hash like entt?

    struct Theme
    {
        HashMap<Name, Color>           Colors;
        HashMap<Name, CornerRounding>  Roundings;
        HashMap<Name, TextRenderStyle> TextStyles;
    };

    namespace ThemeKey
    {
        namespace Color
        {
            inline constexpr Name ButtonNormal  = "Button.Normal";
            inline constexpr Name ButtonHover   = "Button.Hover";
            inline constexpr Name ButtonPressed = "Button.Pressed";

            inline constexpr Name PanelNormal = "Panel.Normal";
        }

        namespace Rounding
        {
            inline constexpr Name Button = "Button";
            inline constexpr Name Panel  = "Panel";
        }

        namespace TextStyle
        {
            inline constexpr Name Default = "Default";
        }
    }

} // namespace RatUI