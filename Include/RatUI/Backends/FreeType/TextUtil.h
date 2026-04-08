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

            // Apply kerning and letter spacing between the previous glyph and this one.
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
      *       Probably need to use ICU. I miss ascii.
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

    /**
     * @brief Builds the final array of lines to render from raw text, applying
     *        transforms, wrapping, line limits, and overflow/ellipsis handling.
     *
     * Internally this calls TextLayout::Prepare() (one-time segment measurement)
     * and TextLayout::WalkLines() (pure-arithmetic layout), so repeated calls
     * with a changed a_MaxWidth are cheap after the first call.
     *
     * @param a_Font       Font providing FTFace and advance cache.
     * @param a_Style      Text style controlling wrapping, transform, overflow, etc.
     * @param a_Text       Raw input text.
     * @param o_Lines      Output: StringView slices valid for the lifetime of o_Storage.
     * @param o_Storage    Output: owns any String objects that o_Lines slices point into
     *                     (normalised text, transformed text, ellipsis-truncated lines).
     *                     Must outlive o_Lines.
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

        // 1. Apply text transform if requested.
        StringView textToRender;
        if ( a_Style.Transform != ETextTransform::None )
        {
            PushBack( o_Storage, ApplyTextTransform( a_Text, a_Style.Transform ) );
            textToRender = StringView{ Back( o_Storage ) };
        }
        else
        {
            textToRender = a_Text;
        }

        // 2. Prepare: segment + pre-measure (one-time cost per text/font/style change).
        const bool preWrap = ( a_Style.Wrap != ETextWrap::NoWrap );
        PreparedText prepared = TextLayout::Prepare(
            textToRender, a_Style.Wrap, preWrap,
            [&]( StringView sv ) -> f32 { return MeasureLineWidth( a_Font, sv, a_Style ); } );

        // Determine whether MaxLines was exceeded by doing an unlimited pre-walk.
        // WalkLines is pure arithmetic, so this extra pass is cheap.
        const u32  maxLines         = a_Style.MaxLines;
        const bool hasWidthConstraint = ( a_MaxWidth < Limits<f32>::max() );
        bool       exceededMaxLines = false;
        if ( maxLines > 0 )
        {
            u32 totalLines = 0;
            TextLayout::WalkLines( prepared, a_MaxWidth, 0u,
                [&]( u32, u32, f32 ) { ++totalLines; } );
            exceededMaxLines = ( totalLines > maxLines );
        }

        // 3. Move NormalizedText into storage so the StringViews we build below remain
        //    valid after BuildTextLines returns.  Reserve first to prevent reallocation
        //    of existing pointers in o_Storage.
        Reserve( o_Storage, Size( o_Storage ) + 1u );
        PushBack( o_Storage, std::move( prepared.NormalizedText ) );
        const String& storedNorm = Back( o_Storage );

        // 4. Walk lines (pure-arithmetic layout) and collect StringViews.
        TextLayout::WalkLines( prepared, a_MaxWidth, maxLines,
            [&]( u32 lineStartSeg, u32 lineEndSeg, f32 )
            {
                // Inline MaterializeLine using storedNorm (NormalizedText was moved out
                // of prepared, but Segments still carry valid byte offsets).
                const auto& segs = prepared.Segments;
                u32 end = lineEndSeg;
                while ( end > lineStartSeg &&
                        segs[ end - 1 ].Kind != ESegmentKind::Text )
                    --end;

                if ( end == lineStartSeg )
                {
                    PushBack( o_Lines, StringView{} );
                    return;
                }

                const u32 byteStart = segs[ lineStartSeg ].StartByte;
                const u32 byteEnd   = segs[ end - 1 ].StartByte + segs[ end - 1 ].ByteLength;
                PushBack( o_Lines,
                          StringView{ Data( storedNorm ) + byteStart, byteEnd - byteStart } );
            } );

        // 5. Overflow/ellipsis handling.
        if ( a_Style.Overflow == ETextOverflow::Clip ||
             a_Style.Overflow == ETextOverflow::Fade )
            return; // WalkLines already respected maxLines; nothing more to do.

        if ( a_Style.Overflow == ETextOverflow::Ellipsis )
        {
            const bool requiresWidthEllipsis     = hasWidthConstraint;
            const bool requiresLastLineIndicator = exceededMaxLines;

            if ( Empty( o_Lines ) || ( !requiresWidthEllipsis && !requiresLastLineIndicator ) )
                return;

            const size lastLineIndex = Size( o_Lines ) - 1;

            // Reserve so that PushBack to o_Storage never reallocates existing StringViews.
            Reserve( o_Storage, Size( o_Storage ) + Size( o_Lines ) );

            for ( size i = 0; i < Size( o_Lines ); ++i )
            {
                const bool forceEllipsis = requiresLastLineIndicator && ( i == lastLineIndex );

                // Fast path: skip lines that don't need truncation.
                if ( !forceEllipsis )
                {
                    if ( !requiresWidthEllipsis )
                        continue;
                    if ( MeasureLineWidth( a_Font, o_Lines[ i ], a_Style ) <= a_MaxWidth )
                        continue;
                }

                PushBack( o_Storage,
                          TruncateLineWithEllipsis( a_Font, o_Lines[ i ], a_Style,
                                                    a_MaxWidth, forceEllipsis ) );
                o_Lines[ i ] = StringView{ Back( o_Storage ) };
            }
        }
    }

} // namespace RatUI::FreeType::TextUtil
