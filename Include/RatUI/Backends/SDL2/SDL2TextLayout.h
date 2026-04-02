#pragma once
#include "../../RatUI.h"
#include <SDL_ttf.h>
#include <cctype>

namespace RatUI::SDL2::TextLayoutUtils
{
    // TODO: I forgot to treat the text as utf8 and just wrote it as ascii.
    // Please fix.

    // TODO: AI generated this code. clean it up or find a cleaner solution
    class UTF8Iterator 
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = char32_t;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const char32_t*;
        using reference         = const char32_t&;

        UTF8Iterator(StringView str, size_t pos = 0)
            : data(str), index(pos) {
            if (index < data.size()) {
                current = decode();
            }
        }

        char32_t operator*() const { return current; }

        UTF8Iterator& operator++() {
            if (index >= data.size()) return *this;
            advance();
            if (index < data.size()) {
                current = decode();
            }
            return *this;
        }

        bool operator!=(const UTF8Iterator& other) const {
            return index != other.index || &data != &other.data;
        }

        static UTF8Iterator end(StringView str) {
            return UTF8Iterator(str, str.size());
        }

        operator bool() const {
            return index < data.size();
        }

    private:
        StringView data;
        size_t index;
        char32_t current = 0;

        void advance() {
            unsigned char lead = static_cast<unsigned char>(data[index]);
            size_t length = utf8CharLength(lead);
            index += length;
        }

        char32_t decode() const {
            unsigned char lead = static_cast<unsigned char>(data[index]);
            size_t length = utf8CharLength(lead);

            if (index + length > data.size()) {
                RATUI_ASSERT(false, "Invalid UTF-8 sequence: unexpected end of string");
                return 0xFFFD; // Unicode replacement character
            }

            char32_t codepoint = 0;
            if (length == 1) {
                codepoint = lead;
            } else {
                codepoint = lead & ((1 << (8 - length - 1)) - 1);
                for (size_t i = 1; i < length; ++i) {
                    unsigned char ch = static_cast<unsigned char>(data[index + i]);
                    if ((ch & 0xC0) != 0x80) {
                        RATUI_ASSERT(false, "Invalid UTF-8 sequence: expected continuation byte");
                        return 0xFFFD; // Unicode replacement character
                    }
                    codepoint = (codepoint << 6) | (ch & 0x3F);
                }
            }
            return codepoint;
        }

        static size_t utf8CharLength(unsigned char lead) {
            if (lead < 0x80) return 1;
            else if ((lead >> 5) == 0x6) return 2;
            else if ((lead >> 4) == 0xE) return 3;
            else if ((lead >> 3) == 0x1E) return 4;
            else { RATUI_ASSERT(false,  "Invalid UTF-8 lead byte"); return 1; }
        }
    };


    /**
     * @brief Measures the width of a single line of text using SDL_ttf, taking into account letter spacing from the TextStyle.
     * @param a_Font The TTF_Font* to use for measurement.
     * @param a_Line The line of text to measure.
     * @param a_Style The TextStyle containing letter spacing information.
     * @return The width of the line in pixels, including letter spacing.
     */
    inline f32 MeasureLineWidth(TTF_Font* font, StringView line, const TextStyle& style)
    {
        if (!font || line.empty())
            return 0.f;

        f32 width = 0.f;

        u32 prevGlyph = 0;
        bool hasPrev = false;

        UTF8Iterator it(line);
        while (it)
        {
            u32 glyph = *it;

            int advance = 0;
            TTF_GlyphMetrics32(font, glyph, nullptr, nullptr, nullptr, nullptr, &advance);

            if (hasPrev)
            {
                width += TTF_GetFontKerningSizeGlyphs32(font, prevGlyph, glyph);
                width += style.LetterSpacing;
            }

            width += advance;

            prevGlyph = glyph;
            hasPrev = true;
            ++it;
        }

        return width;
    }

    /**
     * @brief Applies the specified text transformation to the input text and returns the transformed string.
     * For example, if the transform is ETextTransform::Uppercase, all characters in the text will be converted to uppercase.
     * @param a_Text The input text to transform.
     * @param a_Transform The type of text transformation to apply (e.g., uppercase, lowercase, capitalize).
     * @return A new String containing the transformed text.
     */
    inline String ApplyTextTransform( TextView a_Text, ETextTransform a_Transform )
    {
        String text( Begin( a_Text ), End( a_Text ) );

        switch ( a_Transform )
        {
            case ETextTransform::Uppercase:
                for ( size i = 0; i < Size( a_Text ); ++i )
                {
                    RawAt( text, i ) = static_cast<char>( std::toupper( static_cast<unsigned char>( RawAt( a_Text, i ) ) ) );
                }
                break;
            case ETextTransform::Lowercase:
                for ( size i = 0; i < Size( a_Text ); ++i )
                {
                    RawAt( text, i ) = static_cast<char>( std::tolower( static_cast<unsigned char>( RawAt( a_Text, i ) ) ) );
                }
                break;
            case ETextTransform::Capitalize:
            {
                bool capitalizeNext = true;
                for ( size i = 0; i < Size( a_Text ); ++i )
                {
                    const unsigned char uc = static_cast<unsigned char>( RawAt( a_Text, i ) );
                    if ( std::isalpha( uc ) )
                    {
                        RawAt( text, i ) = capitalizeNext 
                            ? static_cast<char>( std::toupper( uc ) ) 
                            : static_cast<char>( std::tolower( uc ) );

                        capitalizeNext = false;
                    }
                    else if ( std::isspace( uc ) )
                    {
                        capitalizeNext = true;
                    }
                }
                break;
            }
            case ETextTransform::None:
            default: 
                break;
        }

        return text;
    }

    /**
     * @brief Splits the input text into lines based on newline characters ('\n') and populates the output array with the resulting lines.
     * For example, the text "Hello\nWorld" would be split into two lines: "Hello" and "World".
     * @param a_Text The input text to split into lines.
     * @param o_Lines An array to be populated with the resulting lines of text after splitting. 
     * The caller is responsible for ensuring that this array is properly initialized before calling this function.
     */
    inline void SplitTextLines( TextView a_Text, Array<String>& o_Lines )
    {
        Clear( o_Lines );

        size lineStart = 0;
        for ( size i = 0; i < Size( a_Text ); ++i )
        {
            if ( RawAt( a_Text, i ) == '\n' )
            {
                PushBack( o_Lines, String( Begin( a_Text ) + lineStart, Begin( a_Text ) + i ) );
                lineStart = i + 1;
            }
        }

        PushBack( o_Lines, String( Begin( a_Text ) + lineStart, End( a_Text ) ) );
    }

    /**
     * @brief Wraps the input text into multiple lines based on the specified maximum width and populates the output array with the resulting lines.
     * This function takes into account word boundaries and letter spacing from the TextStyle to ensure that lines do not exceed the maximum width.
     * For example, if the input text is "This is a long line of text" and the maximum width only allows for "This is a long", "line of", and "text", then those three lines would be output.
     * @param a_Font The TTF_Font* to use for measuring text width during wrapping.
     * @param a_Text The input text to wrap into lines.
     * @param a_Style The TextStyle containing letter spacing information that affects line width calculations.
     * @param a_MaxWidth The maximum width in pixels that each line of text should not exceed. Lines will be wrapped accordingly.
     * @param o_Lines An array to be populated with the resulting lines of text after wrapping. 
     * The caller is responsible for ensuring that this array is properly initialized before calling this function.
     */
    inline void WrapText(
        TTF_Font* a_Font,
        TextView a_Text,
        const TextStyle& a_Style,
        f32 a_MaxWidth,
        Array<String>& o_Lines )
    {
        // TODO: Should comment and clean up these functions for users
        // text is hell 

        Clear( o_Lines );

        auto wrapSingleParagraph = [&]( TextView a_Paragraph )
        {
            String current;
            Reserve( current, Size( a_Paragraph ) );

            auto flushCurrent = [&]()
            {
                PushBack( o_Lines, current );
                Clear( current );
            };

            auto fitsCurrent = [&]() -> bool
            {
                return MeasureLineWidth( a_Font, current, a_Style ) <= a_MaxWidth;
            };

            auto appendRange = [&]( size_t a_Start, size_t a_End )
            {
                const size oldSize = Size( current );
                const size count   = a_End - a_Start;
                Resize( current, oldSize + count );
                for ( size offset = 0; offset < count; ++offset )
                    RawAt( current, oldSize + offset ) = RawAt( a_Paragraph, a_Start + offset );
            };

            auto fitsAfterAppendRange = [&]( size_t a_Start, size_t a_End ) -> bool
            {
                const size oldSize = Size( current );
                appendRange( a_Start, a_End );
                const bool isFit = fitsCurrent();
                Resize( current, oldSize );
                return isFit;
            };

            auto appendTokenSplitByChar = [&]( size_t a_Start, size_t a_End )
            {
                for ( size i = a_Start; i < a_End; ++i )
                {
                    const size oldSize = Size( current );
                    Resize( current, oldSize + 1 );
                    RawAt( current, oldSize ) = RawAt( a_Paragraph, i );

                    if ( Size( current ) > 1 && !fitsCurrent() )
                    {
                        Resize( current, oldSize );
                        flushCurrent();
                        Resize( current, 1 );
                        RawAt( current, 0 ) = RawAt( a_Paragraph, i );
                    }
                }
            };

            if ( a_Style.Wrap == ETextWrap::WrapChar )
            {
                appendTokenSplitByChar( 0, Size( a_Paragraph ) );
                flushCurrent();
                return;
            }

            size i = 0;
            while ( i < Size( a_Paragraph ) )
            {
                size tokenEnd = i;
                const bool isSpace = std::isspace( static_cast<unsigned char>( RawAt( a_Paragraph, i ) ) ) != 0;

                // Keep consuming characters until we hit a change in whitespace vs non-whitespace or we reach the end of the paragraph.
                while ( tokenEnd < Size( a_Paragraph ) &&
                        ( std::isspace( static_cast<unsigned char>( RawAt( a_Paragraph, tokenEnd ) ) ) != 0 ) == isSpace )
                {
                    ++tokenEnd;
                }

                if ( Empty( current ) || fitsAfterAppendRange( i, tokenEnd ) )
                {
                    appendRange( i, tokenEnd );
                    i = tokenEnd;
                    continue;
                }

                flushCurrent();

                if ( fitsAfterAppendRange( i, tokenEnd ) )
                {
                    appendRange( i, tokenEnd );
                }
                else
                {
                    appendTokenSplitByChar( i, tokenEnd );
                }

                i = tokenEnd;
            }

            flushCurrent();
        };

        size paragraphStart = 0;
        for ( size i = 0; i < Size( a_Text ); ++i )
        {
            if ( RawAt( a_Text, i ) == '\n' )
            {
                if ( i == paragraphStart )
                {
                    PushBack( o_Lines, String() );
                }
                else
                {
                    wrapSingleParagraph( TextView( Data( a_Text ) + paragraphStart, i - paragraphStart ) );
                }

                paragraphStart = i + 1;
            }
        }

        if ( paragraphStart == Size( a_Text ) )
            PushBack( o_Lines, String() );
        else
            wrapSingleParagraph( TextView( Data( a_Text ) + paragraphStart, Size( a_Text ) - paragraphStart ) );
    }

    /**
     * @brief Builds the array of text lines to be rendered based on the input text, text style, and layout constraints.
     * This function first applies text transformations, then splits the text into lines based on newline characters
     * and finally wraps lines that exceed the maximum width. The resulting lines are stored in the output array.
     * For example, if the input text is "Hello World\nThis is a test" with a maximum width that only allows "Hello World" and "This is a", then the output lines would be "Hello World", "This is a", and "test".
     * @param a_Font The TTF_Font* to use for measuring text width during layout.
     * @param a_Style The TextStyle containing font, size, letter spacing, and transformation information that affects text layout.
     * @param a_Text The input text to layout into lines.
     * @param o_Lines An array to be populated with the resulting lines of text after layout.
     * The caller is responsible for ensuring that this array is properly initialized before calling this function.
     * @param a_MaxWidth The maximum width in pixels that each line of text should not exceed. Lines will be wrapped accordingly.
     */
    inline void BuildTextLines( TTF_Font* a_Font, const TextStyle& a_Style, TextView a_Text, Array<String>& o_Lines, f32 a_MaxWidth = Limits<f32>::max() )
    {
        // TODO: I forgot to handle overflow behavior

        Clear( o_Lines );

        const bool needsTransform = ( a_Style.Transform != ETextTransform::None );

        // Only allocate a transformed string if we actually need to apply a transform - otherwise we can just work with the original TextView directly. 
        String transformedText;
        if ( needsTransform )
            transformedText = RatUI::SDL2::TextLayoutUtils::ApplyTextTransform( a_Text, a_Style.Transform );

        const StringView textToRender = needsTransform ? StringView{ transformedText } : a_Text;

        const bool noWrap = ( a_Style.Wrap == ETextWrap::NoWrap ) 
                         || ( a_MaxWidth >= Limits<f32>::max() );

        if ( noWrap )
        {
            RatUI::SDL2::TextLayoutUtils::SplitTextLines( textToRender, o_Lines );
        } 
        else
        {
            RatUI::SDL2::TextLayoutUtils::WrapText( a_Font, textToRender, a_Style, a_MaxWidth, o_Lines );
        } 

        // Truncate to MaxLines if needed
        if ( a_Style.MaxLines > 0 && Size( o_Lines ) > a_Style.MaxLines )
            Resize( o_Lines, a_Style.MaxLines );
    }

} // namespace RatUI::SDL2::TextLayoutUtils
