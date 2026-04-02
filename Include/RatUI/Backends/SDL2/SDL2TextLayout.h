#pragma once
#include "../../RatUI.h"
#include <SDL_ttf.h>
#include <cctype>

namespace RatUI::SDL2::TextLayoutUtils
{
    /**
     * @brief Measures the width of a single line of text using SDL_ttf, taking into account letter spacing from the TextStyle.
     * @param a_Font The TTF_Font* to use for measurement.
     * @param a_Line The line of text to measure.
     * @param a_Style The TextStyle containing letter spacing information.
     * @return The width of the line in pixels, including letter spacing.
     */
    inline f32 MeasureLineWidth( TTF_Font* a_Font, const String& a_Line, const TextStyle& a_Style )
    {
        if ( !a_Font || Empty( a_Line ) )
            return 0.0f;

        int width = 0;
        int height = 0;
        if ( TTF_SizeUTF8( a_Font, Data( a_Line ), &width, &height ) != 0 )
            return 0.0f;

        if ( Size( a_Line ) > 1 )
            width += static_cast<int>( static_cast<float>( Size( a_Line ) - 1 ) * a_Style.LetterSpacing );

        return static_cast<f32>( width );
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
