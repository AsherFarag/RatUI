#pragma once
#include "../Core.h"

namespace RatUI
{

    using Text = String;

    using TextView = StringView;

    /**
     * @brief Specifies the horizontal alignment of text within its layout box.
     */
    enum class ETextAlign : u8
    {
        Left,   ///< The text is aligned to the left edge of the layout box.
        Center, ///< The text is centered horizontally within the layout box.
        Right,  ///< The text is aligned to the right edge of the layout box.
        Justify ///< The text is stretched to fill the entire width of the layout box, with extra space distributed between words.
    };

    /**
     * @brief Specifies the vertical alignment of text relative to its layout box.
     */
    enum class ETextBaseline : u8
    {
        Top,        ///< The top of the text is aligned with the top of the layout box.
        Middle,     ///< The vertical center of the text is aligned with the vertical center of the layout box.
        Bottom,     ///< The bottom of the text is aligned with the bottom of the layout box.
        Alphabetic, ///< The baseline of the text is aligned with the alphabetic baseline of the layout box.
        Hanging     ///< The baseline of the text is aligned with the hanging baseline of the layout box.
    };

    /**
     * @brief Specifies how to handle text that exceeds the available space in a layout.
     */
    enum class ETextOverflow : u8
    {
        Clip,     ///< Text that exceeds the available space is simply cut off, without any indication to the user.
        Ellipsis, ///< Text that exceeds the available space is truncated and an ellipsis ("...") is appended to indicate that there is more text that is not visible.
        Fade      ///< Text that exceeds the available space gradually fades out, providing a visual cue that there is more text that is not visible.
    };

    /**
     * @brief Specifies how to handle text wrapping within its layout box.
     */
    enum class ETextWrap : u8
    {
        NoWrap,    ///< Text is not wrapped and will continue on a single line, potentially overflowing the layout box.
        WrapWord,  ///< Text is wrapped at word boundaries, ensuring that words are not split across lines.
        WrapChar,  ///< Text is wrapped at character boundaries, allowing words to be split across lines.
    };

    /**
     * @brief An opaque, backend-owned handle to a font resource.
     * The backend is responsible for loading font data, creating font resources, and managing their lifetimes.
     */
    struct FontHandle
    {
        u32 ID{ 0 }; ///< The unique identifier for the font resource, used to reference the font in text rendering operations.

        /** @brief Checks if the font handle is valid (i.e., has a non-zero ID). */
        constexpr bool IsValid() const { return ID != 0; }
        constexpr bool operator==(const FontHandle& other) const { return ID == other.ID; }
    };

    /**
     * @brief A struct that encapsulates the styling information for rendering text.
     */
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
