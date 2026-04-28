#pragma once
#include "../Core.h"
#include "Text.h"
#include "Unicode.h"

/**
 * @file TextLayout.h
 * @brief Text layout utilities for wrapping and measuring text, based on https://github.com/chenglou/pretext.
 *
 * The design follows a two-phase model:
 *   1. Prepare()   – one-time, potentially expensive: normalise whitespace, segment the
 *                    text into atomic units, measure each segment.  The result is cached.
 *   2. WalkLines() – the fast hot-path: pure arithmetic over pre-computed widths.  Safe
 *                    to call every frame or on every resize without re-measuring.
 */

namespace RatUI::TextLayout
{
    // TODO: How to handle Rich text with multiple styles?  We could either:
    //       1. Include style information in TextSegment and pre-measure each segment with its style during Prepare().
    //          This would allow for accurate measurement of mixed-style text, but would increase the complexity of Prepare() and the size of PreparedText.
    //       2. Keep RichText as a separate higher-level system?
    //       I want to support things like image's in text etc.
    //       I hate text man.


    constexpr Unit c_LineFitEpsilon = 0.01_u; ///< Tolerance for floating-point line-fit checks

    // =========================================================================
    // Prepare phase
    // =========================================================================

    /**
     * @brief Normalises and prepares text for layout, returning a PreparedText with pre-measured segments.
     * @param a_Text The input text to prepare.
     * @param a_Wrap The text wrapping mode to use (NoWrap, WrapWord, or WrapChar).
     * @param a_Measure A callable that takes a StringView and returns its measured width in pixels.  This is used to pre-measure segments during preparation.
     * @return A PreparedText containing the normalised text and its pre-measured segments.
     */
	template<typename MeasureFn> requires std::is_invocable_r_v<Unit, MeasureFn, StringView>
    inline PreparedText Prepare( StringView a_Text, TextWrap a_Wrap, MeasureFn&& a_Measure )
    {
        // TODO: This doesnt handle 'graphemes' correctly, e.g. emoji sequences with skin-tone modifiers or ZWJ.  
        // The segmentation should ideally be based on extended grapheme clusters (e.g. via ICU) rather than just codepoints, 
        // to avoid splitting user-perceived characters across lines.
        // But I need to research this more.

        PreparedText result;
        result.NormalizedText = a_Wrap.Prewrap()
            ? Unicode::NormalizeWhitespacePreWrap( a_Text )
            : Unicode::NormalizeWhitespace( a_Text );

        const StringView norm = result.NormalizedText; 

        if ( Empty( norm ) )
            return result; // No text, return early with empty segments.

        // Pre-measure the hyphen width for potential soft-hyphen support.
        // TODO: currently don't emit soft hyphens, but we reserve the width here in case we add that later.
        result.HyphenWidth = a_Measure( StringView{ "-" } );

        // For NoWrap mode, we can skip segmentation and just measure the whole text as a single line.
        if ( a_Wrap.BreakMode == EBreakMode::None )
        {
            const Unit lineWidth = a_Measure( norm );
            PushBack( result.Segments,
                TextSegment{
                    .StartByte = 0,
                    .ByteLength = static_cast<u32>( Size( norm ) ),
                    .Width      = lineWidth,
                    .PaintWidth = lineWidth,
                    .Kind       = ESegmentKind::Text,
                    .IsCJKChar  = false
                }
            );
            return result;
        }

        // For WrapWord and WrapChar modes, scan codepoint by codepoint.

        size wordStartByteOffset = 0;

        // Flush any pending Latin/script word run ( wordStartByteOffset .. upTo ) as a Text segment.
        auto flushWord = [&]( size upTo )
        {
            if ( upTo <= wordStartByteOffset )
                return; // No word to flush.

            const u32  start = static_cast<u32>( wordStartByteOffset );
            const u32  len   = static_cast<u32>( upTo - wordStartByteOffset );
            const Unit width = a_Measure( StringView{ Data( norm ) + start, len } );

            PushBack( result.Segments,
                TextSegment{ 
                  .StartByte = start,
                  .ByteLength = len,
                  .Width = width,
                  .PaintWidth = width,
                  .Kind = ESegmentKind::Text,
                  .IsCJKChar = false 
                } 
            );

            wordStartByteOffset = upTo;
        };

        if ( a_Wrap.BreakMode == EBreakMode::Char )
        {
            // Reserve segments for each codepoint, since in WrapChar mode each codepoint is its own segment.
            // TODO: Calculate better heuristic for reserving segments in WrapWord mode, e.g. based on counting whitespace runs.
            //       Or shrink to fit?
            Reserve( result.Segments, Size( norm ) ); // worst case: every byte is a separate codepoint
        }

        for ( Unicode::UTF8Iterator it( norm ); it; )
        {
            const c32  cp  = *it;
            const size idx = it.ByteIndex();
            const size len = it.SequenceByteLength();

            // Pre-wrap newline -> HardBreak segment
            if ( a_Wrap.Prewrap() && cp == U'\n' )
            {
                flushWord( idx );
                PushBack( result.Segments,
                    TextSegment{ 
                        .StartByte = static_cast<u32>( idx ), 
                        .ByteLength = static_cast<u32>( len ),
                        .Width = 0_u,
                        .PaintWidth = 0_u, 
                        .Kind = ESegmentKind::HardBreak, 
                        .IsCJKChar = false 
                    } 
                );
                ++it;
                wordStartByteOffset = it.ByteIndex();
                continue;
            }

            // Whitespace -> Space segment (absorbs a run of spaces)
            if ( Unicode::IsWhitespace( cp ) )
            {
                flushWord( idx );
                const size spaceStart = idx;

                // Consume the entire run of whitespace as part of this space segment
                while ( it && Unicode::IsWhitespace( *it ) )
                    ++it;

                const size spaceEnd = it.ByteIndex();
                const u32  sStart   = static_cast<u32>( spaceStart );
                const u32  sLen     = static_cast<u32>( spaceEnd - spaceStart );
                const Unit sw       = a_Measure( StringView{ Data( norm ) + sStart, sLen } );

                // PaintWidth is 0: trailing spaces are invisible (they hang off the edge).
                PushBack( result.Segments,
                    TextSegment{
                        .StartByte = sStart,
                        .ByteLength = sLen,
                        .Width = sw,
                        .PaintWidth = 0_u,
                        .Kind = ESegmentKind::Space,
                        .IsCJKChar = false 
                    } 
                );

                wordStartByteOffset = spaceEnd;
                continue;
            }

            // WrapChar: each codepoint is its own Text segment
            if ( a_Wrap.BreakMode == EBreakMode::Char )
            {
                RATUI_ASSERT( wordStartByteOffset == idx, "In WrapChar mode, we should never have an outstanding word run." );
                flushWord( idx );

                const bool isCJK = Unicode::IsCJK( cp );
                const Unit w     = a_Measure( StringView{ Data( norm ) + idx, static_cast<u32>( len ) } );

                PushBack( result.Segments,
                    TextSegment{ 
                        .StartByte = static_cast<u32>( idx ),
                        .ByteLength = static_cast<u32>( len ),
                        .Width = w,
                        .PaintWidth = w,
                        .Kind = ESegmentKind::Text,
                        .IsCJKChar = isCJK 
                    } 
                );

                ++it;
                wordStartByteOffset = it.ByteIndex();
                continue;
            }

            // WrapWord: CJK codepoint -> its own Text segment
            if ( Unicode::IsCJK( cp ) )
            {
                flushWord( idx );
                const Unit w = a_Measure( StringView{ Data( norm ) + idx, static_cast<u32>( len ) } );

                PushBack( result.Segments,
                    TextSegment{ 
                        .StartByte = static_cast<u32>( idx ),
                        .ByteLength = static_cast<u32>( len ),
                        .Width = w,
                        .PaintWidth = w,
                        .Kind = ESegmentKind::Text,
                        .IsCJKChar = true 
                    } 
                );

                ++it;
                wordStartByteOffset = it.ByteIndex();
                continue;
            }

            // WrapWord: Latin / script character -> accumulate into word run
            ++it;
        }

        // Flush any pending word at the end of the text.
        flushWord( Size( norm ) );

        return result;
    }

    // =========================================================================
    // Layout phase (WalkLines)
    // =========================================================================

    /**
     * @brief Decodes the first Unicode codepoint of segment @p a_SegIdx.
     *
     * Returns 0 for Space and HardBreak segments (they never need codepoint checks)
     * or when the segment is empty.  Used internally by WalkLines() to evaluate
     * line-break prohibition rules without storing codepoints in TextSegment.
     */


    /**
     * @brief Walks the lines of a prepared text layout, invoking @p a_OnLine for each line.
     *
     * Features:
     *  - Word-level and CJK character-level break opportunities.
     *  - CJK line-break prohibition rules (JIS X 4051 / CSS Text Level 3):
     *      - Characters in IsLineStartProhibited() or IsLineStartStickyPunctuation()
     *        are kept off the start of a line whenever a prior break point exists.
     *      - Characters in IsLineEndProhibited() do not record a break opportunity
     *        after themselves (the following content stays on the same line).
     *  - Optional hard maximum line count via @p a_MaxLines.
     *  - Correct paint-width tracking: trailing Space segments are excluded.
     *
     * @tparam OnLineFn  Callable `void(u32 lineStartSeg, u32 lineEndSeg, f32 paintWidth)`.
     *                   @p lineEndSeg is exclusive (one past the last segment of the line).
     *                   @p paintWidth is the rendered pixel width, excluding trailing spaces.
     *
     * @param a_Prepared  Pre-measured text produced by Prepare().
     * @param a_MaxWidth  Maximum pixel width of a line.  Pass Limits<f32>::max() for
     *                    unlimited width (no wrapping).
     * @param a_MaxLines  Maximum number of lines to emit.  0 means unlimited.
     * @param a_OnLine    Callback invoked for each emitted line.
     * @return            Total number of lines emitted.
     */
    template<std::invocable<u32/*lineStartSeg*/, u32/*lineEndSeg*/, Unit/*paintWidth*/> OnLineFn>
    inline u32 WalkLines( const PreparedText& a_Prepared, Unit a_MaxWidth, u32 a_MaxLines,
                          OnLineFn&& a_OnLine )
    {
        const auto& segs = a_Prepared.Segments;
        if ( Empty( segs ) )
            return 0;

        constexpr u32 c_InvalidSeg = Limits<u32>::max();
        const u32 segCount = static_cast<u32>( Size( segs ) );

        u32  lineCount          = 0;
        Unit lineW              = 0_u;   // total accumulated width (used for fit-checks)
        Unit paintLineW         = 0_u;   // width of visible content (excludes trailing spaces)
        bool hasContent         = false; // whether we've seen any non-space content on the current line (used to skip leading spaces and for paint width)
        u32  lineStartSeg       = 0;
        u32  pendingBreakSeg    = c_InvalidSeg; // first seg of the NEXT line at break
        Unit pendingBreakPaintW = 0_u;          // paintLineW at the recorded break

        // Returns true when the caller should continue emitting lines.
        auto emitLine = [&]( u32 endSeg, Unit paintW ) -> bool
        {
            a_OnLine( lineStartSeg, endSeg, paintW );
            ++lineCount;
            lineW = 0_u;
            paintLineW = 0_u;
            hasContent = false;
            pendingBreakSeg = c_InvalidSeg;
            pendingBreakPaintW = 0_u;

            return a_MaxLines == 0u || lineCount < a_MaxLines;
        };

        // Helper to get the first codepoint of a segment, or 0 for non-Text segments.  Used for line-break prohibition checks.
        auto segmentFirstCP = [&]( u32 segIndex ) -> c32
        {
            const TextSegment& seg = a_Prepared.Segments[segIndex];
            if ( seg.ByteLength == 0 || seg.Kind != ESegmentKind::Text )
                return 0;

            StringView view{ Data( a_Prepared.NormalizedText ) + seg.StartByte, seg.ByteLength };
            Unicode::UTF8Iterator it( view );
            return it ? *it : 0;
        };

        // Returns true if segment @p idx starts with a character that must not begin a line.
        auto isLineStartForbidden = [&]( u32 idx ) -> bool
        {
            const c32 cp = segmentFirstCP( idx );
            return cp != 0 && ( Unicode::IsLineStartProhibited( cp ) ||
                                Unicode::IsLineStartStickyPunctuation( cp ) );
        };

        u32 i = 0;
        while ( i < segCount )
        {
            // At the start of a new line, consume (discard) leading Space segments.
            if ( !hasContent )
            {
                while ( i < segCount && segs[i].Kind == ESegmentKind::Space )
                    ++i;

                if ( i >= segCount )
                    break;

                lineStartSeg = i;
            }

            const TextSegment& seg = segs[i];
            const Unit         w = seg.Width;
            const ESegmentKind k = seg.Kind;

            // - Hard break: force-emit the current line.
            if ( k == ESegmentKind::HardBreak )
            {
                if ( !emitLine( i, hasContent ? paintLineW : 0_u ) )
                    return lineCount;
                lineStartSeg = i + 1;
                ++i;
                continue;
            }

            // - First content segment on this line: always fits.
            if ( !hasContent )
            {
                hasContent = true;
                lineW = w;
                paintLineW = w; // first segment is always Text (spaces were skipped above)
                ++i;
                continue;
            }

            const Unit newW = lineW + w;

            // - Space segment.
            if ( k == ESegmentKind::Space )
            {
                if ( newW <= a_MaxWidth + c_LineFitEpsilon )
                {
                    // Space fits.  Record a break opportunity UNLESS the word
                    // after this space is line-start-forbidden.
                    lineW = newW;
                    // paintLineW is unchanged (trailing space is not painted).
                    const bool nextForbidden = ( i + 1 < segCount ) &&
                        isLineStartForbidden( i + 1 );
                    if ( !nextForbidden )
                    {
                        pendingBreakSeg = i + 1;
                        pendingBreakPaintW = paintLineW; // width before this space
                    }
                }
                else
                {
                    // Space itself overflows: emit the current line.
                    if ( !emitLine( i, paintLineW ) )
                        return lineCount;
                    lineStartSeg = i + 1; // skip the overflowing space on the next line
                }
                ++i;
                continue;
            }

            // - Text / CJK segment.

            if ( newW <= a_MaxWidth + c_LineFitEpsilon )
            {
                // Fits on the current line.
                lineW = newW;
                paintLineW = newW; // Text is always painted.

                // Record break opportunities for both CJK and non-CJK characters.
                // CJK characters respect line-break prohibition rules.
                // Non-CJK characters (including WrapChar mode) can always break after fitting segments.
                if ( seg.IsCJKChar )
                {
                    const c32  thisCp = segmentFirstCP( i );
                    const bool endProhibited = Unicode::IsLineEndProhibited( thisCp );
                    const bool nextForbidden = ( i + 1 < segCount ) &&
                        isLineStartForbidden( i + 1 );

                    if ( !endProhibited && !nextForbidden )
                    {
                        pendingBreakSeg = i + 1;
                        pendingBreakPaintW = paintLineW; // after including this CJK char
                    }
                }
                else
                {
                    // Non-CJK text segment: record break opportunity unless
                    // the next segment is forbidden to start a line.
                    const bool nextForbidden = ( i + 1 < segCount ) &&
                        isLineStartForbidden( i + 1 );
                    if ( !nextForbidden )
                    {
                        pendingBreakSeg = i + 1;
                        pendingBreakPaintW = paintLineW;
                    }
                }

                ++i;
                continue;
            }

            // - Overflow: the current segment does not fit.

            if ( pendingBreakSeg != static_cast<u32>( -1 ) )
            {
                // Break at the last recorded opportunity (word-space or CJK boundary).
                if ( !emitLine( pendingBreakSeg, pendingBreakPaintW ) )
                    return lineCount;
                lineStartSeg = pendingBreakSeg;
                // Do NOT advance i; re-process the current segment on the new line.
                continue;
            }

            // No prior break opportunity: forced break before the current segment.
            // If the current segment is line-start-forbidden (e.g. closing bracket),
            // include it in the current line to avoid a prohibition violation even
            // though it slightly overflows; then break after it.
            if ( isLineStartForbidden( i ) )
            {
                lineW = newW;
                paintLineW = newW;
                ++i;
                if ( !emitLine( i, paintLineW ) )
                    return lineCount;
                lineStartSeg = i;
                continue;
            }

            if ( !emitLine( i, paintLineW ) )
                return lineCount;
            lineStartSeg = i;

            // Do NOT advance i, re-process on the new line.
        }

        if ( hasContent && ( a_MaxLines == 0u || lineCount < a_MaxLines ) )
            emitLine( segCount, paintLineW );

        return lineCount;
    }

    /**
     * @brief Overload of WalkLines() with no maximum line count (unlimited).
     *
     * Convenience wrapper that passes 0 for @p a_MaxLines.
     */
    template<std::invocable<u32, u32, f32> OnLineFn>
    inline u32 WalkLines( const PreparedText& a_Prepared, f32 a_MaxWidth,
                          OnLineFn&& a_OnLine )
    {
        return WalkLines( a_Prepared, a_MaxWidth, 0u, std::forward<OnLineFn>( a_OnLine ) );
    }

} // namespace RatUI::TextLayout
