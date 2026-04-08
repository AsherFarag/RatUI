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


    constexpr f32 c_LineFitEpsilon = 0.01f; ///< Tolerance for floating-point line-fit checks

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

    // =========================================================================
    // Prepare phase
    // =========================================================================

    /**
     * @brief Normalises and prepares text for layout, returning a PreparedText with pre-measured segments.
     * @param a_Text The input text to prepare.
     * @param a_Wrap The text wrapping mode to use (NoWrap, WrapWord, or WrapChar).
     * @param a_PreWrap Whether to apply pre-wrap normalisation rules (e.g. converting newlines to HardBreak segments).
     * @param a_Measure A callable that takes a StringView and returns its measured width in pixels.  This is used to pre-measure segments during preparation.
     * @return A PreparedText containing the normalised text and its pre-measured segments.
     */
    template<std::invocable<StringView> MeasureFn>
    inline PreparedText Prepare( StringView a_Text, ETextWrap a_Wrap, bool a_PreWrap, MeasureFn&& a_Measure )
    {
        // TODO: This doesnt handle 'graphemes' correctly, e.g. emoji sequences with skin-tone modifiers or ZWJ.  
        // The segmentation should ideally be based on extended grapheme clusters (e.g. via ICU) rather than just codepoints, 
        // to avoid splitting user-perceived characters across lines.
        // But I need to research this more.

        PreparedText result;
        result.NormalizedText = a_PreWrap
            ? Unicode::NormalizeWhitespacePreWrap( a_Text )
            : Unicode::NormalizeWhitespace( a_Text );

        const StringView norm = result.NormalizedText; 

        if ( Empty( norm ) )
            return result; // No text, return early with empty segments.

        // Pre-measure the hyphen width for potential soft-hyphen support.
        // TODO: currently don't emit soft hyphens, but we reserve the width here in case we add that later.
        result.HyphenWidth = a_Measure( StringView{ "-" } );

        // For NoWrap mode, we can skip segmentation and just measure the whole text as a single line.
        if ( a_Wrap == ETextWrap::NoWrap )
        {
            const f32 lineWidth = a_Measure( norm );
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
        auto flushWord =[&]( size upTo )
        {
            if ( upTo <= wordStartByteOffset )
                return; // No word to flush.

            const u32 start = static_cast<u32>( wordStartByteOffset );
            const u32 len   = static_cast<u32>( upTo - wordStartByteOffset );
            const f32 width = a_Measure( StringView{ Data( norm ) + start, len } );

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

        if ( a_Wrap == ETextWrap::WrapChar )
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
            if ( a_PreWrap && cp == U'\n' )
            {
                flushWord( idx );
                PushBack( result.Segments,
                    TextSegment{ 
                        .StartByte = static_cast<u32>( idx ), 
                        .ByteLength = static_cast<u32>( len ),
                        .Width = 0.f,
                        .PaintWidth = 0.f, 
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
                const f32  sw       = a_Measure( StringView{ Data( norm ) + sStart, sLen } );

                // PaintWidth is 0: trailing spaces are invisible (they hang off the edge).
                PushBack( result.Segments,
                    TextSegment{
                        .StartByte = sStart,
                        .ByteLength = sLen,
                        .Width = sw,
                        .PaintWidth = 0.f,
                        .Kind = ESegmentKind::Space,
                        .IsCJKChar = false 
                    } 
                );

                wordStartByteOffset = spaceEnd;
                continue;
            }

            // WrapChar: each codepoint is its own Text segment
            if ( a_Wrap == ETextWrap::WrapChar )
            {
                RATUI_ASSERT( wordStartByteOffset == idx, "In WrapChar mode, we should never have an outstanding word run." );
                flushWord( idx );

                const bool isCJK = Unicode::IsCJK( cp );
                const f32  w     = a_Measure( StringView{ Data( norm ) + idx, static_cast<u32>( len ) } );

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
                const f32 w = a_Measure( StringView{ Data( norm ) + idx, static_cast<u32>( len ) } );

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

    /**
     * @brief Walks the lines of a prepared text layout, invoking a callback for each line.
     */
    template<std::invocable<u32/*lineStartSeg*/, u32/*lineEndSeg*/, f32/*paintWidth*/> OnLineFn>
    inline u32 WalkLines( const PreparedText& a_Prepared, f32 a_MaxWidth, u32 a_MaxLines, OnLineFn&& a_OnLine )
    {
        const auto& segs = a_Prepared.Segments;
        if ( Empty( segs ) )
            return 0;

        constexpr u32 c_InvalidSeg = Limits<u32>::max();
        const u32 segCount = static_cast<u32>( Size( segs ) );
         

        u32  lineCount          = 0;
        f32  lineW              = 0.f;
        bool hasContent         = false;
        u32  lineStartSeg       = 0;
        u32  pendingBreakSeg    = c_InvalidSeg; // First seg of the NEXT line at break
        f32  pendingBreakPaintW = 0.f;          // PaintLineW at the recorded break

        return lineCount;
    }

} // namespace RatUI::TextLayout
