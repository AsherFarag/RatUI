#pragma once
#include "../../RatUI.h"
#include "FontCache.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cctype>

namespace RatUI::FreeType::TextUtil
{
    using ::RatUI::Unicode::UTF8Iterator;
    using ::RatUI::Unicode::UTF8Range;

    /**
     * @brief Measures the pixel width of a single (newline-free) line of text.
     *
     * Uses Font::GetAdvance - a glyph advance cache populated lazily by FreeType -
     * so the common case (same glyphs measured multiple times per frame) never calls
     * FT_Load_Glyph more than once per glyph per pixel size.
     *
     * Kerning is still fetched via FT_Get_Kerning on every pair because FreeType
     * does not cache it.  If kerning queries prove expensive, consider building a
     * pair cache in Font alongside the advance cache.
     *
     * @param a_Font  The loaded Font (provides FTFace + advance cache).
     * @param a_Line  A newline-free string view to measure.
     * @param a_Style TextStyle for letter spacing.
     * @return The measured width in pixels.
     */
    inline f32 MeasureLineWidth( Font& a_Font, StringView a_Line, const TextStyle& a_Style )
    {
        if ( Empty( a_Line ) )
            return 0.f;

        FT_Face face     = a_Font.GetFace();
        f32     width    = 0.f;
        u32     prevGlyph = 0;
        bool    hasPrev  = false;

        for ( const c32 cp : UTF8Range( a_Line ) )
        {
            const u32 glyphIdx = FT_Get_Char_Index( face, cp );
            const f32 advance = a_Font.GetAdvanceX( glyphIdx );

            if ( hasPrev )
            {
                if ( FT_HAS_KERNING( face ) )
                {
                    FT_Vector kerning;
                    if ( FT_Get_Kerning( face, prevGlyph, glyphIdx, FT_KERNING_DEFAULT, &kerning ) == 0 )
                        width += kerning.x / 64.f;
                }
                width += a_Style.LetterSpacing;
            }

            width    += advance;
            prevGlyph = glyphIdx;
            hasPrev   = true;
        }

        return width;
    }

    /**
     * @brief Applies the specified text transformation (uppercase, lowercase, capitalize) to a UTF-8 encoded string.
      * For ASCII characters, it uses std::toupper and std::tolower. For non-ASCII characters, it leaves them unchanged.
      * The transformation is applied according to the ETextTransform mode:
      * - Uppercase: All characters are transformed to uppercase.
      * - Lowercase: All characters are transformed to lowercase.
      * - Capitalize: The first character of each word is transformed to uppercase (currently implemented as all uppercase for simplicity).
      *
      * @param a_Text The input text to transform, provided as a StringView.
      * @param a_Transform The type of transformation to apply, specified as an ETextTransform enum value.
      * @return A new String containing the transformed text. The caller is responsible for managing the memory of the returned string.
      * TODO: Support locale-aware transformations and proper word-boundary detection for capitalization.
     */
    inline String ApplyTextTransform( TextView a_Text, ETextTransform a_Transform )
    {
        if ( a_Transform == ETextTransform::None )
            return String( Begin( a_Text ), End( a_Text ) );

        String result;
        Reserve( result, Size( a_Text ) );

        UTF8Range range{ a_Text };
        for ( auto it = range.begin(); it != range.end(); ++it )
        {
            const c32 cp = *it;

            if ( cp <= 0x7F )
            {
                char c = static_cast<char>( cp );
                switch ( a_Transform )
                {
                    case ETextTransform::Uppercase:  c = static_cast<char>( std::toupper( static_cast<unsigned char>( c ) ) ); break;
                    case ETextTransform::Lowercase:  c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) ); break;
                    case ETextTransform::Capitalize: c = static_cast<char>( std::toupper( static_cast<unsigned char>( c ) ) ); break; // TODO: word-boundary awareness
                    default: break;
                }
                PushBack( result, c );
            }
            else
            {
                const size count   = it.SequenceByteLength();
                const size oldSize = Size( result );
                Resize( result, oldSize + count );
                std::memcpy( Data( result ) + oldSize, Data( a_Text ) + it.ByteIndex(), count );
            }
        }

        return result;
    }

    /**
     * @brief Splits a UTF-8 encoded string into lines based on newline characters ('\n').
     * Each line is represented as a StringView that references a portion of the original string, avoiding unnecessary copying.
     * @param a_Text The input text to split, provided as a StringView.
     * @param o_Lines An output array that will be populated with StringView slices corresponding to each line of text. 
     *        The caller is responsible for ensuring that the lifetime of the original string exceeds
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
     * @brief Wraps text into lines that fit within a_MaxWidth.
     * @param a_Font     Font providing the FTFace and advance cache.
     * @param a_Text     Text to wrap (may contain newlines - each newline starts a new paragraph).
     * @param a_Style    Text style (letter spacing, wrap mode).
     * @param a_MaxWidth Maximum line width in pixels.
     * @param o_Lines    Output array of StringView slices into a_Text (or a_Text's backing storage).
     */
    inline void WrapText(
        Font&              a_Font,
        TextView           a_Text,
        const TextStyle&   a_Style,
        f32                a_MaxWidth,
        Array<StringView>& o_Lines )
    {
        Clear( o_Lines );

        FT_Face face = a_Font.GetFace();

        auto wrapSingleParagraph = [&]( TextView a_Paragraph )
        {
            size_t lineByteStart = 0;
            size_t lineByteEnd   = 0;
            f32    currentWidth  = 0.f;
            u32    lastGlyphIdx  = 0;
            bool   hasLastGlyph  = false;

            auto flushCurrent = [&]()
            {
                PushBack( o_Lines, StringView( Data( a_Paragraph ) + lineByteStart, lineByteEnd - lineByteStart ) );
                lineByteStart = lineByteEnd;
                currentWidth  = 0.f;
                lastGlyphIdx  = 0;
                hasLastGlyph  = false;
            };

            // Measures [a_ByteStart, a_ByteEnd) accounting for leading kerning from the current
            // line's last glyph.  Uses the advance cache on every character.
            auto measureTokenWidth = [&]( size_t a_ByteStart, size_t a_ByteEnd,
                                          u32& o_LastGlyph, bool& o_HasLast ) -> f32
            {
                f32  w         = 0.f;
                u32  prevGlyph = lastGlyphIdx;
                bool hasPrev   = hasLastGlyph;

                const StringView token( Data( a_Paragraph ) + a_ByteStart, a_ByteEnd - a_ByteStart );
                for ( c32 cp : Unicode::UTF8Range{ token } )
                {
                    const u32 glyphIdx = FT_Get_Char_Index( face, cp );

                    if ( hasPrev )
                    {
                        if ( FT_HAS_KERNING( face ) )
                        {
                            FT_Vector kerning;
                            if ( FT_Get_Kerning( face, prevGlyph, glyphIdx, FT_KERNING_DEFAULT, &kerning ) == 0 )
                                w += kerning.x / 64.f;
                        }

                        w += a_Style.LetterSpacing;
                    }

                    w += a_Font.GetAdvanceX( glyphIdx ); // cache hit after first encounter
                    prevGlyph = glyphIdx;
                    hasPrev   = true;
                }

                o_LastGlyph = prevGlyph;
                o_HasLast   = hasPrev;
                return w;
            };

            // Appends characters one-by-one, flushing to a new line on overflow.
            auto appendTokenSplitByChar = [&]( UTF8Iterator a_Begin, UTF8Iterator a_End )
            {
                while ( a_Begin != a_End )
                {
                    const size_t cpStart  = a_Begin.ByteIndex();
                    const u32    cp       = *a_Begin;
                    const size_t cpEnd    = (++a_Begin).ByteIndex();
                    const u32 glyphIdx    = FT_Get_Char_Index( face, cp );
                    const f32 rawAdvance  = a_Font.GetAdvanceX( glyphIdx );

                    f32 glyphContrib = rawAdvance;
                    if ( hasLastGlyph )
                    {
                        if ( FT_HAS_KERNING( face ) )
                        {
                            FT_Vector kerning;
                            if ( FT_Get_Kerning( face, lastGlyphIdx, glyphIdx, FT_KERNING_DEFAULT, &kerning ) == 0 )
                                glyphContrib += kerning.x / 64.f;
                        }
                        glyphContrib += a_Style.LetterSpacing;
                    }

                    if ( lineByteEnd > lineByteStart && currentWidth + glyphContrib > a_MaxWidth )
                    {
                        flushCurrent();
                        glyphContrib = rawAdvance; // no leading contribution on the new line
                    }

                    lineByteEnd   = cpEnd;
                    currentWidth += glyphContrib;
                    if ( rawAdvance > 0.f || glyphIdx != 0 )
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
                bool         isSpace    = Unicode::IsAsciiWhitespace( *it );

                while ( it != end && Unicode::IsAsciiWhitespace( *it ) == isSpace )
                    ++it;

                const size_t byteStart = tokenStart.ByteIndex();
                const size_t byteEnd   = it.ByteIndex();

                u32  tokenLastGlyph = 0;
                bool tokenHasLast   = false;
                const f32 tokenWidth = measureTokenWidth( byteStart, byteEnd, tokenLastGlyph, tokenHasLast );

                if ( lineByteEnd == lineByteStart || currentWidth + tokenWidth <= a_MaxWidth )
                {
                    lineByteEnd  = byteEnd;
                    currentWidth += tokenWidth;
                    lastGlyphIdx = tokenLastGlyph;
                    hasLastGlyph = tokenHasLast;
                    continue;
                }

                flushCurrent();

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
                    PushBack( o_Lines, StringView() );
                else
                    wrapSingleParagraph( TextView( Data( a_Text ) + paragraphStart, i - paragraphStart ) );

                paragraphStart = i + 1;
            }
        }

        if ( paragraphStart == Size( a_Text ) )
            PushBack( o_Lines, StringView() );
        else
            wrapSingleParagraph( TextView( Data( a_Text ) + paragraphStart, Size( a_Text ) - paragraphStart ) );
    }

    /**
     * @brief Truncates a line so that (prefix + "...") fits within a_MaxWidth.
     * @param a_Font           Font providing FTFace and advance cache.
     * @param a_Line           The line to truncate (must be newline-free).
     * @param a_Style          Text style (letter spacing).
     * @param a_MaxWidth       Maximum allowed width of the final string.
     * @param a_ForceEllipsis  When true, always appends "..." even if the line fits.
     * @return The truncated string, or an empty string if even "..." does not fit.
     */
    inline String TruncateLineWithEllipsis(
        Font&            a_Font,
        StringView       a_Line,
        const TextStyle& a_Style,
        f32              a_MaxWidth,
        bool             a_ForceEllipsis )
    {
        FT_Face face = a_Font.GetFace();

        if ( !a_ForceEllipsis && MeasureLineWidth( a_Font, a_Line, a_Style ) <= a_MaxWidth )
            return String( Begin( a_Line ), End( a_Line ) );

        constexpr StringView c_Ellipsis = "...";
        const f32 ellipsisWidth = MeasureLineWidth( a_Font, c_Ellipsis, a_Style );
        if ( ellipsisWidth > a_MaxWidth )
            return {};

        const u32 dotGlyphIdx = FT_Get_Char_Index( face, U'.' );

        size_t bestPrefixByteCount = 0;
        f32    prefixWidth         = 0.f;
        u32    prevGlyphIdx        = 0;
        bool   hasPrev             = false;

        UTF8Range range{ a_Line };
		for ( auto it = range.begin(); it != range.end(); ++it )
        {
			const c32 cp = *it;
            const u32 glyphIdx = FT_Get_Char_Index( face, cp );
            const f32 advance = a_Font.GetAdvanceX( glyphIdx );

            if ( hasPrev )
            {
                if ( FT_HAS_KERNING( face ) )
                {
                    FT_Vector kerning;
                    if ( FT_Get_Kerning( face, prevGlyphIdx, glyphIdx, FT_KERNING_DEFAULT, &kerning ) == 0 )
                        prefixWidth += kerning.x / 64.f;
                }
                prefixWidth += a_Style.LetterSpacing;
            }

            prefixWidth  += advance;
            prevGlyphIdx  = glyphIdx;
            hasPrev       = true;

            // Candidate = prefix + LetterSpacing + crossKerning(last, '.') + "..."
            f32 crossKerning = 0.f;
            if ( FT_HAS_KERNING( face ) )
            {
                FT_Vector kerning;
                if ( FT_Get_Kerning( face, prevGlyphIdx, dotGlyphIdx, FT_KERNING_DEFAULT, &kerning ) == 0 )
                    crossKerning = kerning.x / 64.f;
            }
            const f32 candidateWidth = prefixWidth + a_Style.LetterSpacing + crossKerning + ellipsisWidth;

            if ( candidateWidth <= a_MaxWidth ) bestPrefixByteCount = it.ByteIndex();
            else break; // Widths are non-decreasing - no longer prefix can fit.
        }

        String result;
        Resize( result, bestPrefixByteCount );
        Reserve( result, bestPrefixByteCount + Size( c_Ellipsis ) );

        if ( bestPrefixByteCount > 0 )
            std::memcpy( Data( result ), Data( a_Line ), bestPrefixByteCount );

        for ( const char c : c_Ellipsis )
            PushBack( result, c );

        return result;
    }

    // -------------------------------------------------------------------------
    // BuildTextLines
    // -------------------------------------------------------------------------

    /**
     * @brief Builds the final array of lines to render from raw text, applying
     *        transforms, wrapping, line limits, and overflow/ellipsis handling.
     * @param a_Font       Font providing FTFace and advance cache.
     * @param a_Style      Text style controlling wrapping, transform, overflow, etc.
     * @param a_Text       Raw input text.
     * @param o_Lines      Output: StringView slices into a_Text or o_Storage.
     * @param o_Storage    Output: owns any String objects that o_Lines slices point into
     *                     (transformed text, ellipsis-truncated lines).  Must outlive o_Lines.
     * @param a_MaxWidth   Maximum line width for wrapping and overflow checks.
     */
    inline void BuildTextLines(
        Font&              a_Font,
        const TextStyle&   a_Style,
        TextView           a_Text,
        Array<StringView>& o_Lines,
        Array<String>&     o_Storage,
        f32                a_MaxWidth = Limits<f32>::max() )
    {
        Clear( o_Lines );
        Clear( o_Storage );

        const bool needsTransform = ( a_Style.Transform != ETextTransform::None );

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
            SplitTextLines( textToRender, o_Lines );
        else
            WrapText( a_Font, textToRender, a_Style, a_MaxWidth, o_Lines );

        const bool hasWidthConstraint  = ( a_MaxWidth < Limits<f32>::max() );
        const bool exceededMaxLines    = ( a_Style.MaxLines > 0 && Size( o_Lines ) > a_Style.MaxLines );

        if ( a_Style.Overflow == ETextOverflow::Clip || a_Style.Overflow == ETextOverflow::Fade )
        {
            if ( exceededMaxLines )
                Resize( o_Lines, a_Style.MaxLines );
            return;
        }

        if ( a_Style.Overflow == ETextOverflow::Ellipsis )
        {
            if ( exceededMaxLines )
                Resize( o_Lines, a_Style.MaxLines );

            const bool requiresWidthEllipsis    = hasWidthConstraint;
            const bool requiresLastLineIndicator = exceededMaxLines;

            if ( Empty( o_Lines ) || ( !requiresWidthEllipsis && !requiresLastLineIndicator ) )
                return;

            const size lastLineIndex = Size( o_Lines ) - 1;

            // Reserve enough capacity so subsequent PushBack calls never reallocate
            // o_Storage, keeping previously captured StringViews valid.
            Reserve( o_Storage, Size( o_Storage ) + Size( o_Lines ) );

            for ( size i = 0; i < Size( o_Lines ); ++i )
            {
                const bool forceEllipsis = requiresLastLineIndicator && ( i == lastLineIndex );

                // Fast path: skip lines that don't overflow and don't need the "more" indicator.
                // MeasureLineWidth now uses the advance cache so even this fast-path check is cheap.
                if ( !forceEllipsis )
                {
                    if ( !requiresWidthEllipsis )
                        continue;

                    if ( MeasureLineWidth( a_Font, o_Lines[i], a_Style ) <= a_MaxWidth )
                        continue;
                }

                PushBack( o_Storage, TruncateLineWithEllipsis( a_Font, o_Lines[i], a_Style, a_MaxWidth, forceEllipsis ) );
                o_Lines[i] = StringView{ Back( o_Storage ) };
            }
        }
    }

} // namespace RatUI::FreeType::TextUtil
