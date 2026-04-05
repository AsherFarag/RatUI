#pragma once
#include "../Core.h"
#include "Font.h"

namespace RatUI
{

    using Text = String;

    using TextView = StringView;

    /**
     * @brief Specifies the horizontal alignment of text within its layout box.
     */
    enum class ETextAlign : u8
    {
        Left,    ///< The text is aligned to the left edge of the layout box.
        Center,  ///< The text is centered horizontally within the layout box.
        Right,   ///< The text is aligned to the right edge of the layout box.
        Justify, ///< The text is stretched to fill the entire width of the layout box, with extra space distributed between words.

        _NumBits = 2
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
        Hanging,    ///< The baseline of the text is aligned with the hanging baseline of the layout box.

        _NumBits = 3
    };

    /**
     * @brief Specifies how to handle text that exceeds the available space in a layout.
     */
    enum class ETextOverflow : u8
    {
        Clip,     ///< Text that exceeds the available space is simply cut off, without any indication to the user.
        Ellipsis, ///< Text that exceeds the available space is truncated and an ellipsis ("...") is appended to indicate that there is more text that is not visible.
        Fade,     ///< Text that exceeds the available space gradually fades out, providing a visual cue that there is more text that is not visible.

        _NumBits = 2
    };

    /**
     * @brief Specifies how to handle text wrapping within its layout box.
     */
    enum class ETextWrap : u8
    {
        NoWrap,    ///< Text is not wrapped and will continue on a single line, potentially overflowing the layout box.
        WrapWord,  ///< Text is wrapped at word boundaries, ensuring that words are not split across lines.
        WrapChar,  ///< Text is wrapped at character boundaries, allowing words to be split across lines.

        _NumBits = 2
    };

    enum class ETextTransform : u8
    {
        None,       ///< No transformation is applied to the text; it is rendered as-is.
        Uppercase,  ///< All characters in the text are transformed to uppercase.
        Lowercase,  ///< All characters in the text are transformed to lowercase.
        Capitalize, ///< The first character of each word is transformed to uppercase.

        _NumBits = 2
    };

    /**
     * @brief A struct that encapsulates the styling information for rendering text.
     */
    struct TextStyle
    {
        FontHandle     Font{};                 ///< The font to use for rendering the text, specified as a FontHandle. If not set, a default font will be used.
        f32            Size{ 16.0f };          ///< The size of the font in points, which determines the height of the characters. Default is 16.0f.          
        f32            LineHeight{ 0.0f };     ///< The height of each line of text, including spacing. If set to 0, it will be automatically calculated based on the font size and metrics.
        f32            LetterSpacing{ 0.0f };  ///< The spacing between characters in the text, specified in points. Default is 0.0f.

        Coloru8        Color{ Colorsu8::White };

        u16            MaxLines{ 0 };          ///< The maximum number of lines to render. If set to 0, there is no limit on the number of lines.

        ETextAlign     Align     : (u8)ETextAlign::_NumBits     { ETextAlign::Left };
        ETextWrap      Wrap      : (u8)ETextWrap::_NumBits      { ETextWrap::WrapWord }; 
        ETextOverflow  Overflow  : (u8)ETextOverflow::_NumBits  { ETextOverflow::Clip };
        ETextTransform Transform : (u8)ETextTransform::_NumBits { ETextTransform::None };
        ETextBaseline  Baseline  : (u8)ETextBaseline::_NumBits  { ETextBaseline::Alphabetic };

        // TODO: Should maybe remove some of these features and add it to a RichTextStyle or something instead.

        bool Bold          : 1 = false;
        bool Italic        : 1 = false;
        bool Underline     : 1 = false;
        bool Strikethrough : 1 = false;

		constexpr bool operator==( const TextStyle& a_Other ) const = default;
    };
    static_assert( std::is_trivially_copyable_v<TextStyle> );
	static_assert( sizeof( TextStyle ) == 24, "TextStyle should be 24 bytes in size - Reevaluate padding if this assertion fails." );

    /**
	 * @brief Stores measurement results of a block of text created by ITextMetrics.
     */
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
		u64             Handle{ 0 };   ///< Backend-specific identifier for the shaped text resource, used to reference the pre-rendered text in draw commands.
		TextMeasurement Measurement{};

        constexpr bool IsValid() const { return Handle != 0; }
		constexpr bool operator==( const ShapedText& other ) const { return Handle == other.Handle; }
    };

} // namespace RatUI
