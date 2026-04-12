#pragma once
#include "../Core.h"
#include "Font.h"

namespace RatUI
{

    using Text = String;

    using TextView = StringView;

    /**
     * @brief Specifies the base directionality of text for proper rendering of mixed-direction content.
     */
    enum class ETextDirection : u8
    {
        Auto, ///< Infer from the first strong character (Unicode Bidi Algorithm).
        LTR,  ///< Left-to-right text direction, used for scripts like Latin and Cyrillic.
        RTL,  ///< Right-to-left text direction, used for scripts like Arabic and Hebrew.

        _NumBits = 2
    };

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
        // TODO? Fade,     ///< Text that exceeds the available space gradually fades out, providing a visual cue that there is more text that is not visible.

        _NumBits = 2
    };

    /**
     * @brief Specifies text transformation rules for layout and rendering.
     */
    enum class ETextTransform : u8
    {
        None,       ///< No transformation is applied to the text; it is rendered as-is.
        Uppercase,  ///< All characters in the text are transformed to uppercase.
        Lowercase,  ///< All characters in the text are transformed to lowercase.
        Capitalize, ///< The first character of each word is transformed to uppercase.

        _NumBits = 2
    };

    /**
     * @brief Specifies how text should wrap when it reaches the edge of its layout box.
     */
    namespace EBreakMode
    {
        constexpr u8 None = 0; ///< No wrapping; text will continue on a single line and may overflow its container.
        constexpr u8 Word = 1; ///< Text will wrap at word boundaries, ensuring that words are not split across lines. 
                               ///< If a single word exceeds the container width, it will overflow.
        constexpr u8 Char = 2; ///< Text will wrap at any character boundary, allowing words to be split if necessary to fit within the container width.

        constexpr u8 _NumBits = 2;
    };

    /**
     * @brief Specifies how whitespace characters are handled during text layout and rendering.
     */
    namespace EWhitespace
    {
        constexpr u8 Preserve = 0; ///< Whitespace is preserved as-is. Text will only wrap on line breaks.
        constexpr u8 Collapse = 1; ///< Sequences of whitespace will collapse into a single whitespace. Text will wrap when necessary.

        constexpr u8 _NumBits = 1;
    };

    /**
     * @brief Specifies how newline characters are handled during text layout and rendering.
     */
    namespace ENewline
    {
        constexpr u8 Preserve = 0; ///< Sequences of newlines will collapse into a single newline. Text will wrap when necessary, and on line breaks.
        constexpr u8 Collapse = 1; ///< Newlines are treated as whitespace and will collapse into a single whitespace. Text will wrap when necessary.

        constexpr u8 _NumBits = 1;
    };

    /**
     * @brief Defines text wrapping behavior, including line-breaking rules, whitespace handling, and newline handling.
     */
    struct TextWrap
    {
        u8 BreakMode  : EBreakMode::_NumBits  { EBreakMode::Word };
        u8 Whitespace : EWhitespace::_NumBits { EWhitespace::Collapse };
        u8 Newline    : ENewline::_NumBits    { ENewline::Preserve };

        /** @brief Determines if the text should be pre-wrapped - i.e., whether Prepare() should do an initial layout pass treating newlines as hard breaks. */
        constexpr bool Prewrap() const { return ( BreakMode != EBreakMode::None ) && ( Newline == ENewline::Preserve ); }

        constexpr bool operator==( const TextWrap& ) const = default;

        /** @brief Sequences of whitespace will collapse into a single whitespace. Text will never wrap to the next line. */
        static constexpr TextWrap NoWrap()   { return { EBreakMode::None, EWhitespace::Collapse, ENewline::Preserve }; }

        /** @brief Ordinary wrapping at word boundaries. Sequences of whitespace will collapse into a single whitespace. 
         *  Text will wrap when necessary, and on line breaks. */
        static constexpr TextWrap WrapWord() { return { EBreakMode::Word, EWhitespace::Collapse, ENewline::Preserve }; }

        /** @brief Aggressive wrapping at any character boundary. Sequences of whitespace will collapse into a single whitespace. 
         *  Text will wrap when necessary, and on line breaks. */
        static constexpr TextWrap WrapChar() { return { EBreakMode::Char, EWhitespace::Collapse, ENewline::Preserve }; }

        /** @brief Sequences of whitespace will be preserved as-is. Text will only wrap on line breaks. */
        static constexpr TextWrap Pre()      { return { EBreakMode::None, EWhitespace::Preserve, ENewline::Preserve }; }

        /** @brief Sequences of whitespace will collapse into a single whitespace. Text will only wrap on line breaks. */
        static constexpr TextWrap PreLine()  { return { EBreakMode::Word, EWhitespace::Collapse, ENewline::Preserve }; }

        /** @brief Sequences of whitespace will be preserved as-is. Text will wrap at word boundaries, and on line breaks. 
         *  Newlines will be treated as whitespace and will collapse into a single whitespace. */
        static constexpr TextWrap PreWrap()  { return { EBreakMode::Word, EWhitespace::Preserve, ENewline::Preserve }; }

        /** @brief A default text wrap configuration that provides ordinary wrapping at word boundaries. */
        static constexpr TextWrap Normal() { return WrapWord(); }
    };
    static_assert( TextWrap{} == TextWrap::WrapWord(), "Default TextWrap should be WrapWord" );
    static_assert( TextWrap::Normal() == TextWrap::WrapWord(), "TextWrap::Normal() should be WrapWord" );

    /**
	 * @brief A struct that encapsulates the styling information for layout of text.
     */
    struct TextLayoutStyle
    {
        FontHandle     Font{};                 ///< The font to use for rendering the text, specified as a FontHandle. If not set, a default font will be used.
        f32            Size{ 16.0f };          ///< The size of the font in points, which determines the height of the characters. Default is 16.0f.          
        f32            LineHeight{ 0.0f };     ///< The height of each line of text, including spacing. If set to 0, it will be automatically calculated based on the font size and metrics.
        f32            LetterSpacing{ 0.0f };  ///< The spacing between characters in the text, specified in points. Default is 0.0f.
		f32            WordSpacing{ 0.0f };    ///< The spacing between words in the text, specified in points. Default is 0.0f.
		u16            MaxLines{ 0 };          ///< The maximum number of lines to display. If set to 0, there is no limit and all lines will be displayed.

        ETextDirection Direction  { ETextDirection::Auto };
        ETextAlign     Align      { ETextAlign::Left };
        TextWrap       Wrap       { TextWrap::Normal() }; 
        ETextOverflow  Overflow   { ETextOverflow::Clip };
        ETextTransform Transform  { ETextTransform::None };
        ETextBaseline  Baseline   { ETextBaseline::Alphabetic };

		constexpr bool operator==( const TextLayoutStyle& ) const = default;
    };

	/**
	 * @brief A struct that encapsulates the styling information for rendering of text, such as color and decorations.
     */
    struct TextRenderStyle
    {
		Coloru8 Color{ Colorsu8::White };  ///< The default color of the text. Default is white.
        bool    Underline     : 1 = false; ///< Whether the text should be rendered with an underline decoration. Default is false.
        bool    Strikethrough : 1 = false; ///< Whether the text should be rendered with a strikethrough decoration. Default is false.

		constexpr bool operator==( const TextRenderStyle& ) const = default;
    };

    struct TextStyle
    {
        TextLayoutStyle Layout{};
        TextRenderStyle Render{};

        constexpr FontHandle GetFont() const { return Layout.Font; }
    };

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

    /**
     * @brief The kind of a text segment.
     */
    enum class ESegmentKind : u8
    {
        Text,       ///< A word (Latin / script run) or a single CJK codepoint.
        Space,      ///< Collapsible inter-word whitespace.
        HardBreak,  ///< Explicit newline (only emitted in pre-wrap mode).
    };

    /**
     * @brief A single pre-measured segment of text.
     *
     * Each segment corresponds to a contiguous byte range inside
     * PreparedText::NormalizedText.
     */
    struct TextSegment
    {
        u32 StartByte{ 0 };    ///< Byte offset inside PreparedText::NormalizedText.
        u32 ByteLength{ 0 };   ///< Length in bytes.
        f32 Width{ 0.f };      ///< Pixel advance used for line-fit checks (includes any spacing).
        f32 PaintWidth{ 0.f }; ///< Pixel width of visible rendered content.
                               ///<   Text     : equals Width.
                               ///<   Space    : 0 (trailing spaces hang past the edge).
                               ///<   HardBreak: 0.

        ESegmentKind Kind{ ESegmentKind::Text };
        bool         IsCJKChar{ false }; ///< True when this is a single CJK codepoint.
    };

    /**
     * @brief Result of the prepare phase, used as input to the layout phase.
     * Treat this as an immutable value and only re-run Prepare() when the text content or style changes.
     */
    struct PreparedText
    {
        String             NormalizedText;     ///< Text after whitespace normalisation.
        Array<TextSegment> Segments;           ///< Pre-measured segments in logical order.
        f32                HyphenWidth{ 0.f }; ///< Width of "-" (reserved for soft-hyphen support).
    };

} // namespace RatUI
