#pragma once
#include "../../RatUI.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cctype>

namespace RatUI::FreeType::TextUtil
{
    // TODO: I forgot to treat the text as utf8 and just wrote it as ascii.
    // Please fix.

    // TODO: Probably shouldve gotten a library for this
    // TODO: make this a universal utility instead of SDL2 specific since it doesn't actually use any SDL2 features
    /** @brief An iterator for traversing UTF-8 encoded strings. */
    class UTF8Iterator
    {
    public:
        using value_type = char32_t;

        UTF8Iterator(StringView a_Str, size_t a_ByteIndex = 0)
            : m_Data(a_Str), m_Index(a_ByteIndex)
        {
            if (m_Index < m_Data.size())
                m_Current = Decode();
        }

        static UTF8Iterator End(StringView a_Str)
        {
            return UTF8Iterator(a_Str, Size(a_Str));
        }

        char32_t operator*() const { return m_Current; }

        UTF8Iterator& operator++()
        {
            if (m_Index >= Size(m_Data))
                return *this;

            Advance();
            if (m_Index < Size(m_Data))
                m_Current = Decode();

            return *this;
        }

        bool operator==(const UTF8Iterator& a_Other) const
        {
            return Data( m_Data ) == Data( a_Other.m_Data ) && m_Index == a_Other.m_Index;
        }

        explicit operator bool() const
        {
            return m_Index < m_Data.size();
        }

        size_t ByteIndex() const { return m_Index; }

    private:
        StringView m_Data;
        size_t     m_Index   = 0;
        char32_t   m_Current = 0;

        static constexpr char32_t c_ReplacementCodepoint = 0xFFFD;

        // - UTF-8 structural constants

        // Byte patterns
        static constexpr u8 c_AsciiMax              = 0x7F;
        static constexpr u8 c_ContinuationMask      = 0xC0;
        static constexpr u8 c_ContinuationPattern   = 0x80;

        // Leading byte prefix patterns (after shifting)
        static constexpr u8 c_2ByteLeadPrefix = 0b110;   // 0x6
        static constexpr u8 c_3ByteLeadPrefix = 0b1110;  // 0xE
        static constexpr u8 c_4ByteLeadPrefix = 0b11110; // 0x1E

        // Bit masks for extracting payload bits
        static constexpr u8 c_2ByteMask = 0x1F;
        static constexpr u8 c_3ByteMask = 0x0F;
        static constexpr u8 c_4ByteMask = 0x07;
        static constexpr u8 c_ContMask  = 0x3F;

        // Shift amounts
        static constexpr i32 c_Shift6  = 6;
        static constexpr i32 c_Shift12 = 12;
        static constexpr i32 c_Shift18 = 18;

        // Byte counts
        static constexpr size c_1ByteLength = 1;
        static constexpr size c_2ByteLength = 2;
        static constexpr size c_3ByteLength = 3;
        static constexpr size c_4ByteLength = 4;

        /** @brief Advances the iterator to the next UTF-8 character. */
        void Advance()
        {
            u8 lead = (u8)m_Data[m_Index];
            m_Index += CharLength(lead);
        }

        /** @brief Decodes the UTF-8 character at the current position. */
        char32_t Decode() const
        {
            const u8* s = (const u8*)m_Data.data() + m_Index;

            size_t remaining = m_Data.size() - m_Index;
            u8 b0 = s[0];

            // ASCII (single byte)
            if (b0 <= c_AsciiMax)
                return b0;

            // 2-byte sequence
            if ((b0 >> 5) == c_2ByteLeadPrefix &&
                remaining >= c_2ByteLength &&
                IsCont(s[1]))
            {
                return ((b0 & c_2ByteMask) << c_Shift6) |
                       (s[1] & c_ContMask);
            }

            // 3-byte sequence
            if ((b0 >> 4) == c_3ByteLeadPrefix &&
                remaining >= c_3ByteLength &&
                IsCont(s[1]) &&
                IsCont(s[2]))
            {
                return ((b0 & c_3ByteMask) << c_Shift12) |
                       ((s[1] & c_ContMask) << c_Shift6) |
                       (s[2] & c_ContMask);
            }

            // 4-byte sequence
            if ((b0 >> 3) == c_4ByteLeadPrefix &&
                remaining >= c_4ByteLength &&
                IsCont(s[1]) &&
                IsCont(s[2]) &&
                IsCont(s[3]))
            {
                return ((b0 & c_4ByteMask) << c_Shift18) |
                       ((s[1] & c_ContMask) << c_Shift12) |
                       ((s[2] & c_ContMask) << c_Shift6) |
                       (s[3] & c_ContMask);
            }

            return c_ReplacementCodepoint;
        }

        /** @brief Checks if a byte is a UTF-8 continuation byte. */
        static bool IsCont(u8 a_Byte)
        {
            return (a_Byte & c_ContinuationMask) == c_ContinuationPattern;
        }

        /** @brief Determines the length of a UTF-8 character based on its leading byte. */
        static size_t CharLength(u8 a_Lead)
        {
            if ( a_Lead       <= c_AsciiMax)        return c_1ByteLength;
            if ((a_Lead >> 5) == c_2ByteLeadPrefix) return c_2ByteLength;
            if ((a_Lead >> 4) == c_3ByteLeadPrefix) return c_3ByteLength;
            if ((a_Lead >> 3) == c_4ByteLeadPrefix) return c_4ByteLength;
            return c_1ByteLength;
        }
    };

    inline bool IsAsciiWhitespace( char32_t a_CP )
    {
        return a_CP == U' '  ||
               a_CP == U'\t' ||
               a_CP == U'\n' ||
               a_CP == U'\r';
    }

    /**
     * @brief Measures the width of a single line of text using FreeType, taking into account letter spacing from the TextStyle.
     * @param a_Face The FT_Face to use for measurement.
     * @param a_Line The line of text to measure.
     * @param a_Style The TextStyle containing letter spacing information.
     * @return The width of the line in pixels, including letter spacing.
     */
    inline f32 MeasureLineWidth( FT_Face a_Face, StringView a_Line, const TextStyle& a_Style )
    {
        if ( !a_Face || a_Line.empty() )
            return 0.f;

        f32 width = 0.f;

        u32 prevGlyphIdx = 0;
        bool hasPrev = false;

        UTF8Iterator it( a_Line );
        while ( it )
        {
            u32 cp = *it;
            u32 glyphIdx = FT_Get_Char_Index( a_Face, cp );

            if ( FT_Load_Glyph( a_Face, glyphIdx, FT_LOAD_DEFAULT ) == 0 )
            {
                if ( hasPrev )
                {
                    if ( FT_HAS_KERNING( a_Face ) )
                    {
                        FT_Vector kerning;
                        if ( FT_Get_Kerning( a_Face, prevGlyphIdx, glyphIdx, FT_KERNING_DEFAULT, &kerning ) == 0 )
                            width += kerning.x / 64.f;
                    }
                    width += a_Style.LetterSpacing;
                }

                width += a_Face->glyph->advance.x / 64.f;
                prevGlyphIdx = glyphIdx;
                hasPrev = true;
            }

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
    inline String ApplyTextTransform(TextView a_Text, ETextTransform a_Transform)
    {
        if (a_Transform == ETextTransform::None)
            return String(Begin(a_Text), End(a_Text));
    
        String result;
        Reserve(result, Size(a_Text));
    
        UTF8Iterator it(a_Text);
        UTF8Iterator end = UTF8Iterator::End(a_Text);
    
        while (it != end)
        {
            size_t start = it.ByteIndex();
            char32_t cp  = *it;
            ++it;
            size_t endByte = it.ByteIndex();
        
            if (cp <= 0x7F) // ASCII only
            {
                char c = static_cast<char>(cp);
            
                switch (a_Transform)
                {
                    case ETextTransform::Uppercase:
                        c = static_cast<char>(std::toupper((unsigned char)c));
                        break;
                    case ETextTransform::Lowercase:
                        c = static_cast<char>(std::tolower((unsigned char)c));
                        break;
                    case ETextTransform::Capitalize:
                        c = static_cast<char>(std::toupper((unsigned char)c));
                        // TODO:
                        break;
                    default:
                        break;
                }
            
                PushBack(result, c);
            }
            else
            {
                // Copy full UTF-8 byte sequence unchanged
                for (size i = start; i < endByte; ++i)
                    PushBack(result, RawAt(a_Text, i));
            }
        }
    
        return result;
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
     * @param a_Face The FT_Face to use for measuring text width during wrapping.
     * @param a_Text The input text to wrap into lines.
     * @param a_Style The TextStyle containing letter spacing information that affects line width calculations.
     * @param a_MaxWidth The maximum width in pixels that each line of text should not exceed. Lines will be wrapped accordingly.
     * @param o_Lines An array to be populated with the resulting lines of text after wrapping. 
     * The caller is responsible for ensuring that this array is properly initialized before calling this function.
     */
    inline void WrapText(
        FT_Face a_Face,
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
                return MeasureLineWidth( a_Face, current, a_Style ) <= a_MaxWidth;
            };

            auto appendCodepoint = [&](UTF8Iterator& it)
            {
                size_t start = it.ByteIndex();
                char32_t cp = *it;
                ++it;
                size_t end = it.ByteIndex();
            
                size_t count = end - start;
            
                size_t oldSize = Size(current);
                Resize(current, oldSize + count);
            
                for (size_t i = 0; i < count; ++i)
                    RawAt(current, oldSize + i) = RawAt(a_Paragraph, start + i);
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

            auto appendTokenSplitByChar = [&](UTF8Iterator begin, UTF8Iterator end)
            {
                while (begin != end)
                {
                    size_t cpStart = begin.ByteIndex();
                    ++begin;
                    size_t cpEnd = begin.ByteIndex();
                
                    const size byteCount = cpEnd - cpStart;
                    const size oldSize   = Size(current);
                
                    Resize(current, oldSize + byteCount);
                
                    for (size i = 0; i < byteCount; ++i)
                        RawAt(current, oldSize + i) =
                            RawAt(a_Paragraph, cpStart + i);
                
                    if (!fitsCurrent())
                    {
                        Resize(current, oldSize);
                        flushCurrent();
                    
                        Resize(current, byteCount);
                        for (size i = 0; i < byteCount; ++i)
                            RawAt(current, i) =
                                RawAt(a_Paragraph, cpStart + i);
                    }
                }
            };

            if ( a_Style.Wrap == ETextWrap::WrapChar )
            {
                appendTokenSplitByChar( UTF8Iterator( a_Paragraph ), UTF8Iterator::End( a_Paragraph ) );
                flushCurrent();
                return;
            }

            UTF8Iterator it(a_Paragraph);
            UTF8Iterator end = UTF8Iterator::End(a_Paragraph);

            while (it != end)
            {
                UTF8Iterator tokenStart = it;
                bool isSpace = IsAsciiWhitespace(*it);
            
                // Consume contiguous whitespace or non-whitespace
                while (it != end && IsAsciiWhitespace(*it) == isSpace)
                    ++it;
            
                size_t byteStart = tokenStart.ByteIndex();
                size_t byteEnd   = it.ByteIndex();
            
                if (Empty(current) || fitsAfterAppendRange(byteStart, byteEnd))
                {
                    appendRange(byteStart, byteEnd);
                    continue;
                }
            
                flushCurrent();
            
                if (fitsAfterAppendRange(byteStart, byteEnd))
                {
                    appendRange(byteStart, byteEnd);
                }
                else
                {
                    appendTokenSplitByChar(tokenStart, it);
                }
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

    inline String TruncateLineWithEllipsis(
        FT_Face         a_Face,
        StringView      a_Line,
        const TextStyle& a_Style,
        f32             a_MaxWidth,
        bool            a_ForceEllipsis )
    {
        if ( !a_ForceEllipsis && MeasureLineWidth( a_Face, a_Line, a_Style ) <= a_MaxWidth )
            return String( Begin( a_Line ), End( a_Line ) );

        constexpr StringView c_Ellipsis = "...";
        const f32 ellipsisWidth = MeasureLineWidth( a_Face, c_Ellipsis, a_Style );
        if ( ellipsisWidth > a_MaxWidth )
            return {};

        size bestPrefixByteCount = 0;

        UTF8Iterator it( a_Line );
        UTF8Iterator end = UTF8Iterator::End( a_Line );
        while ( it != end )
        {
            ++it;
            const size currentPrefixBytes = it.ByteIndex();

            String candidate;
            Reserve( candidate, currentPrefixBytes + Size( c_Ellipsis ) );

            for ( size i = 0; i < currentPrefixBytes; ++i )
                PushBack( candidate, RawAt( a_Line, i ) );
            for ( const char c : c_Ellipsis )
                PushBack( candidate, c );

            if ( MeasureLineWidth( a_Face, candidate, a_Style ) <= a_MaxWidth )
                bestPrefixByteCount = currentPrefixBytes;
            else
                break;
        }

        String result;
        Reserve( result, bestPrefixByteCount + Size( c_Ellipsis ) );
        for ( size i = 0; i < bestPrefixByteCount; ++i )
            PushBack( result, RawAt( a_Line, i ) );
        for ( const char c : c_Ellipsis )
            PushBack( result, c );

        return result;
    }

    /**
     * @brief Builds the array of text lines to be rendered based on the input text, text style, and layout constraints.
     * This function first applies text transformations, then splits the text into lines based on newline characters
     * and finally wraps lines that exceed the maximum width. The resulting lines are stored in the output array.
     * For example, if the input text is "Hello World\nThis is a test" with a maximum width that only allows "Hello World" and "This is a", then the output lines would be "Hello World", "This is a", and "test".
     * @param a_Face The FT_Face to use for measuring text width during layout.
     * @param a_Style The TextStyle containing font, size, letter spacing, and transformation information that affects text layout.
     * @param a_Text The input text to layout into lines.
     * @param o_Lines An array to be populated with the resulting lines of text after layout.
     * The caller is responsible for ensuring that this array is properly initialized before calling this function.
     * @param a_MaxWidth The maximum width in pixels that each line of text should not exceed. Lines will be wrapped accordingly.
     */
    inline void BuildTextLines( FT_Face a_Face, const TextStyle& a_Style, TextView a_Text, Array<String>& o_Lines, f32 a_MaxWidth = Limits<f32>::max() )
    {
        Clear( o_Lines );

        const bool needsTransform = ( a_Style.Transform != ETextTransform::None );

        // Only allocate a transformed string if we actually need to apply a transform - otherwise we can just work with the original TextView directly. 
        String transformedText;
        if ( needsTransform )
            transformedText = ApplyTextTransform( a_Text, a_Style.Transform );

        const StringView textToRender = needsTransform ? StringView{ transformedText } : a_Text;

        const bool noWrap = ( a_Style.Wrap == ETextWrap::NoWrap ) 
                         || ( a_MaxWidth >= Limits<f32>::max() );

        if ( noWrap )
        {
            SplitTextLines( textToRender, o_Lines );
        } 
        else
        {
            WrapText( a_Face, textToRender, a_Style, a_MaxWidth, o_Lines );
        } 

        const bool hasWidthConstraint = ( a_MaxWidth < Limits<f32>::max() );
        const bool exceededMaxLines = ( a_Style.MaxLines > 0 && Size( o_Lines ) > a_Style.MaxLines );

        if ( a_Style.Overflow == ETextOverflow::Clip || a_Style.Overflow == ETextOverflow::Fade )
        {
            // Fade is currently handled by clipping semantics in text layout.
            if ( exceededMaxLines )
                Resize( o_Lines, a_Style.MaxLines );
            return;
        }

        if ( a_Style.Overflow == ETextOverflow::Ellipsis )
        {
            if ( exceededMaxLines )
                Resize( o_Lines, a_Style.MaxLines );

            const bool requiresWidthEllipsis = hasWidthConstraint;
            const bool requiresLastLineIndicator = exceededMaxLines;

            if ( Empty( o_Lines ) || ( !requiresWidthEllipsis && !requiresLastLineIndicator ) )
                return;

            const size lastLineIndex = Size( o_Lines ) - 1;

            for ( size i = 0; i < Size( o_Lines ); ++i )
            {
                const bool forceEllipsis = requiresLastLineIndicator && i == lastLineIndex;
                const bool lineOverflowsWidth = requiresWidthEllipsis
                    && MeasureLineWidth( a_Face, o_Lines[i], a_Style ) > a_MaxWidth;

                if ( !forceEllipsis && !lineOverflowsWidth )
                    continue;

                o_Lines[i] = TruncateLineWithEllipsis( a_Face, o_Lines[i], a_Style, a_MaxWidth, forceEllipsis );
            }
        }
    }

} // namespace RatUI::FreeType::TextUtil
