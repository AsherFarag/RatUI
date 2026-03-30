#pragma once
#include "../Core.h"

namespace RatUI
{

    using Text = String;

    using TextView = StringView;

    enum class ETextAlign : u8
    {
        Left,
        Center,
        Right,
        Justify
    };

    struct FontHandle
    {
        u32 ID{ 0 }; ///< The unique identifier for the font resource, used to reference the font in text rendering operations.

        /** @brief Checks if the font handle is valid (i.e., has a non-zero ID). */
        constexpr bool IsValid() const { return ID != 0; }
        constexpr bool operator==(const FontHandle& other) const { return ID == other.ID; }
    };

    struct TextStyle
    {
        FontHandle Font{};
        f32        Size{ 16.0f };
        f32        LineHeight{ 0.0f };
        f32        LetterSpacing{ 0.0f };
        bool       Bold{ false };
        bool       Italic{ false };
    };

    struct TextMeasurement
    {
        Vec2f Size{};         ///< The width and height of the measured text block.
        f32 Baseline{ 0.0f }; ///< The distance from the top of the text block to the baseline, which is important for aligning text vertically.
        u32 LineCount{ 0 };   ///< The number of lines in the measured text block, which can be used for multi-line text layout and spacing calculations.
    };

    /**
     * @brief An opaque, backend-owned handle to a pre-rendered/shaped text object.
     * Shaped text has already been measured and its glyph positions computed.
     * Rendering it avoids per-frame re-shaping, which is expensive for long or complex strings.
     *
     * The backend creates and destroys ShapedText objects. RatUI holds handles and submits them
     * in draw commands. The backend is responsible for resource lifetime.
     */
    struct ShapedText
    {
        u64   Handle{ 0 };
        Vec2f Size{};
        f32   Baseline{ 0.f };
        u32   LineCount{ 0 };

        constexpr bool IsValid() const { return Handle != 0; }
    };

} // namespace RatUI
