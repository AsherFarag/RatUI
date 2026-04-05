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

            if ( FT_Load_Glyph( a_Face, glyphIdx, FT_LOAD_ADVANCE_ONLY  ) == 0 )
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
                const size count   = endByte - start;
                const size oldSize = Size( result );
                Resize( result, oldSize + count );
                std::memcpy( Data( result ) + oldSize, Data( a_Text ) + start, count );
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
    inline void SplitTextLines( TextView a_Text, Array<StringView>& o_Lines )
    {
        Clear( o_Lines );

        size lineStart = 0;
        for ( size i = 0; i < Size( a_Text ); ++i )
        {
            if ( RawAt( a_Text, i ) == '\n' )
            {
                PushBack( o_Lines, StringView( Data( a_Text ) + lineStart, i - lineStart ) );
                lineStart = i + 1;
            }
        }

        PushBack( o_Lines, StringView( Data( a_Text ) + lineStart, Size( a_Text ) - lineStart ) );
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
        Array<StringView>& o_Lines )
    {
        Clear( o_Lines );

        // Wraps a single newline-free paragraph into o_Lines.
        // Instead of building a String accumulator, we track the start and end byte positions
        // of the current line directly into a_Paragraph - every output StringView is a
        // zero-copy slice of the original text.
        auto wrapSingleParagraph = [&]( TextView a_Paragraph )
        {
            // [lineByteStart, lineByteEnd) is the byte range of the current line in a_Paragraph.
            size_t lineByteStart = 0;
            size_t lineByteEnd   = 0;

            // Incremental width state for the current line being built.
            f32  currentWidth = 0.f;
            u32  lastGlyphIdx = 0;
            bool hasLastGlyph = false;

            auto flushCurrent = [&]()
            {
                PushBack( o_Lines, StringView( Data( a_Paragraph ) + lineByteStart, lineByteEnd - lineByteStart ) );
                lineByteStart = lineByteEnd;
                currentWidth  = 0.f;
                lastGlyphIdx  = 0;
                hasLastGlyph  = false;
            };

            // Measures the width contribution of the byte range [a_ByteStart, a_ByteEnd) in
            // a_Paragraph, accounting for leading kerning/spacing from the current line's last
            // glyph state (lastGlyphIdx / hasLastGlyph). Does NOT modify shared state.
            // On return, o_LastGlyph / o_HasLast hold the final glyph state of the token.
            auto measureTokenWidth = [&]( size_t a_ByteStart, size_t a_ByteEnd,
                                          u32& o_LastGlyph, bool& o_HasLast ) -> f32
            {
                f32  w         = 0.f;
                u32  prevGlyph = lastGlyphIdx;
                bool hasPrev   = hasLastGlyph;

                const StringView token( Data( a_Paragraph ) + a_ByteStart, a_ByteEnd - a_ByteStart );
                UTF8Iterator it( token );
                while ( it )
                {
                    const u32 cp       = *it;
                    ++it;
                    const u32 glyphIdx = FT_Get_Char_Index( a_Face, cp );
                    if ( FT_Load_Glyph( a_Face, glyphIdx, FT_LOAD_ADVANCE_ONLY ) != 0 )
                        continue;

                    if ( hasPrev )
                    {
                        if ( FT_HAS_KERNING( a_Face ) )
                        {
                            FT_Vector kerning;
                            if ( FT_Get_Kerning( a_Face, prevGlyph, glyphIdx, FT_KERNING_DEFAULT, &kerning ) == 0 )
                                w += kerning.x / 64.f;
                        }
                        w += a_Style.LetterSpacing;
                    }
                    w        += a_Face->glyph->advance.x / 64.f;
                    prevGlyph = glyphIdx;
                    hasPrev   = true;
                }
                o_LastGlyph = prevGlyph;
                o_HasLast   = hasPrev;
                return w;
            };

            // Appends characters one by one from [a_Begin, a_End), flushing to a new line
            // whenever a character would exceed a_MaxWidth. Uses byte-position tracking so
            // no String allocation is needed.
            auto appendTokenSplitByChar = [&]( UTF8Iterator a_Begin, UTF8Iterator a_End )
            {
                while ( a_Begin != a_End )
                {
                    const size_t cpStart   = a_Begin.ByteIndex();
                    const u32    cp        = *a_Begin;
                    ++a_Begin;
                    const size_t cpEnd     = a_Begin.ByteIndex();

                    const u32  glyphIdx = FT_Get_Char_Index( a_Face, cp );
                    const bool glyphOk  = ( FT_Load_Glyph( a_Face, glyphIdx, FT_LOAD_ADVANCE_ONLY ) == 0 );

                    f32 glyphContrib = 0.f;
                    if ( glyphOk )
                    {
                        glyphContrib = a_Face->glyph->advance.x / 64.f;
                        if ( hasLastGlyph )
                        {
                            if ( FT_HAS_KERNING( a_Face ) )
                            {
                                FT_Vector kerning;
                                if ( FT_Get_Kerning( a_Face, lastGlyphIdx, glyphIdx, FT_KERNING_DEFAULT, &kerning ) == 0 )
                                    glyphContrib += kerning.x / 64.f;
                            }
                            glyphContrib += a_Style.LetterSpacing;
                        }
                    }

                    // Flush if the character would overflow. A line with no content always accepts
                    // the character (lineByteEnd > lineByteStart means content is present).
                    // Characters are iterated sequentially, so at this point lineByteEnd equals
                    // cpStart; flushCurrent() therefore correctly sets lineByteStart to cpStart.
                    if ( lineByteEnd > lineByteStart && currentWidth + glyphContrib > a_MaxWidth )
                    {
                        flushCurrent(); // lineByteStart = lineByteEnd = cpStart (sequential iteration)
                        // After flush, this is the first glyph of the new line — no leading contribution.
                        glyphContrib = glyphOk ? ( a_Face->glyph->advance.x / 64.f ) : 0.f;
                    }

                    lineByteEnd   = cpEnd;
                    currentWidth += glyphContrib;
                    if ( glyphOk )
                    {
                        lastGlyphIdx = glyphIdx;
                        hasLastGlyph = true;
                    }
                }
            };

            if ( a_Style.Wrap == ETextWrap::WrapChar )
            {
                appendTokenSplitByChar( UTF8Iterator( a_Paragraph ), UTF8Iterator::End( a_Paragraph ) );
                flushCurrent();
                return;
            }

            UTF8Iterator it( a_Paragraph );
            UTF8Iterator end = UTF8Iterator::End( a_Paragraph );

            while ( it != end )
            {
                UTF8Iterator tokenStart = it;
                bool isSpace = IsAsciiWhitespace( *it );

                // Consume a contiguous run of whitespace or non-whitespace.
                while ( it != end && IsAsciiWhitespace( *it ) == isSpace )
                    ++it;

                const size_t byteStart = tokenStart.ByteIndex();
                const size_t byteEnd   = it.ByteIndex();

                // Measure the token's width including leading kerning/spacing from the current
                // line end. This is O(token_length) rather than O(current_line_length).
                u32  tokenLastGlyph = 0;
                bool tokenHasLast   = false;
                const f32 tokenWidth = measureTokenWidth( byteStart, byteEnd, tokenLastGlyph, tokenHasLast );

                if ( lineByteEnd == lineByteStart || currentWidth + tokenWidth <= a_MaxWidth )
                {
                    // Fast path: fits on the current line — just extend lineByteEnd.
                    lineByteEnd   = byteEnd;
                    currentWidth += tokenWidth;
                    lastGlyphIdx  = tokenLastGlyph;
                    hasLastGlyph  = tokenHasLast;
                    continue;
                }

                // Token does not fit — flush the current line and try on a fresh one.
                // Tokens cover the paragraph without gaps, so lineByteEnd == byteStart here;
                // flushCurrent() therefore sets lineByteStart to byteStart of the new token.
                flushCurrent();

                // Re-measure without leading kerning (flushCurrent reset hasLastGlyph to false).
                const f32 freshTokenWidth = measureTokenWidth( byteStart, byteEnd, tokenLastGlyph, tokenHasLast );

                if ( freshTokenWidth <= a_MaxWidth )
                {
                    lineByteEnd  = byteEnd;
                    currentWidth = freshTokenWidth;
                    lastGlyphIdx = tokenLastGlyph;
                    hasLastGlyph = tokenHasLast;
                }
                else
                {
                    // Token itself is wider than the max — split it character by character.
                    appendTokenSplitByChar( tokenStart, it );
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
                    PushBack( o_Lines, StringView() );
                }
                else
                {
                    wrapSingleParagraph( TextView( Data( a_Text ) + paragraphStart, i - paragraphStart ) );
                }

                paragraphStart = i + 1;
            }
        }

        if ( paragraphStart == Size( a_Text ) )
            PushBack( o_Lines, StringView() );
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

        // Find the longest prefix P of a_Line such that width(P + "...") <= a_MaxWidth.
        // Accumulate the prefix width incrementally (O(n)) rather than re-measuring the full
        // candidate string on every iteration (O(n^2)).
        const u32 dotGlyphIdx = FT_Get_Char_Index( a_Face, U'.' );

        size bestPrefixByteCount = 0;
        f32  prefixWidth         = 0.f;
        u32  prevGlyphIdx        = 0;
        bool hasPrev             = false;

        UTF8Iterator it( a_Line );
        UTF8Iterator end = UTF8Iterator::End( a_Line );
        while ( it != end )
        {
            const u32 cp = *it;
            ++it;

            const u32 glyphIdx = FT_Get_Char_Index( a_Face, cp );
            if ( FT_Load_Glyph( a_Face, glyphIdx, FT_LOAD_ADVANCE_ONLY ) != 0 )
                continue;

            if ( hasPrev )
            {
                if ( FT_HAS_KERNING( a_Face ) )
                {
                    FT_Vector kerning;
                    if ( FT_Get_Kerning( a_Face, prevGlyphIdx, glyphIdx, FT_KERNING_DEFAULT, &kerning ) == 0 )
                        prefixWidth += kerning.x / 64.f;
                }
                prefixWidth += a_Style.LetterSpacing;
            }
            prefixWidth  += a_Face->glyph->advance.x / 64.f;
            prevGlyphIdx  = glyphIdx;
            hasPrev       = true;

            // Compute width(currentPrefix + "...") accounting for the kerning/spacing
            // between the last prefix glyph and the first '.' of the ellipsis.
            f32 crossKerning = 0.f;
            if ( FT_HAS_KERNING( a_Face ) )
            {
                FT_Vector kerning;
                if ( FT_Get_Kerning( a_Face, prevGlyphIdx, dotGlyphIdx, FT_KERNING_DEFAULT, &kerning ) == 0 )
                    crossKerning = kerning.x / 64.f;
            }
            // hasPrev is always true here (we just added a glyph), so letter spacing applies.
            const f32 candidateWidth = prefixWidth + a_Style.LetterSpacing + crossKerning + ellipsisWidth;

            if ( candidateWidth <= a_MaxWidth )
                bestPrefixByteCount = it.ByteIndex();
            else
                break; // widths are non-decreasing, so no longer prefix can fit
        }

        String result;
        Reserve( result, bestPrefixByteCount + Size( c_Ellipsis ) );
        Resize( result, bestPrefixByteCount );
        if ( bestPrefixByteCount > 0 )
            std::memcpy( Data( result ), Data( a_Line ), bestPrefixByteCount );
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
    inline void BuildTextLines( FT_Face a_Face, const TextStyle& a_Style, TextView a_Text,
                                Array<StringView>& o_Lines, Array<String>& o_Storage,
                                f32 a_MaxWidth = Limits<f32>::max() )
    {
        Clear( o_Lines );
        Clear( o_Storage );

        const bool needsTransform = ( a_Style.Transform != ETextTransform::None );

        // If a transform is required, store the owning String in o_Storage first, then take
        // a StringView into it. Both steps are in the same branch so back() is always safe.
        StringView textToRender;
        if ( needsTransform )
        {
            PushBack( o_Storage, ApplyTextTransform( a_Text, a_Style.Transform ) );
            textToRender = StringView{ o_Storage.back() };
        }
        else
        {
            textToRender = a_Text;
        }

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

            // Reserve enough capacity for every line to need an ellipsis string. This single
            // reservation ensures that subsequent PushBack calls never reallocate o_Storage's
            // internal buffer, keeping all previously captured StringViews valid.
            Reserve( o_Storage, Size( o_Storage ) + Size( o_Lines ) );

            for ( size i = 0; i < Size( o_Lines ); ++i )
            {
                const bool forceEllipsis = requiresLastLineIndicator && i == lastLineIndex;
                const bool lineOverflowsWidth = requiresWidthEllipsis
                    && MeasureLineWidth( a_Face, o_Lines[i], a_Style ) > a_MaxWidth;

                if ( !forceEllipsis && !lineOverflowsWidth )
                    continue;

                PushBack( o_Storage, TruncateLineWithEllipsis( a_Face, o_Lines[i], a_Style, a_MaxWidth, forceEllipsis ) );
				o_Lines[i] = StringView{ Back( o_Storage ) };
            }
        }
    }

} // namespace RatUI::FreeType::TextUtil
