#include <RatUI/Text/TextMetrics.h>
#include <RatUI/Text/TextLayout.h>

#include <algorithm>

namespace RatUI
{
    namespace
    {
        /** @brief How far a run reaches above / below the baseline, with and without its line gap. */
        struct RunExtent
        {
            Unit Ascent { 0_u };
            Unit Descent{ 0_u };
            Unit Above  { 0_u }; ///< Ascent + half the line gap.
            Unit Below  { 0_u }; ///< Descent + the rest of the line gap.

            void Include( const RunExtent& a_Other )
            {
                Ascent  = std::max( Ascent, a_Other.Ascent );
                Descent = std::max( Descent, a_Other.Descent );
                Above   = std::max( Above, a_Other.Above );
                Below   = std::max( Below, a_Other.Below );
            }
        };

        RunExtent ComputeRunExtent( const IFontFace& a_Face, Unit a_Size )
        {
            using namespace TextShaping;
            const FontFaceMetrics& m = a_Face.Metrics();

            RunExtent extent;
            extent.Ascent  = EmToUnit( a_Face, m.Ascender, a_Size );
            extent.Descent = EmToUnit( a_Face, -m.Descender, a_Size );

            // Half-leading like CSS, except pixel faces keep the gap below so the baseline stays on the grid.
            const Unit gap      = std::max( 0_u, EmToUnit( a_Face, m.LineGap, a_Size ) );
            const Unit gapAbove = IsPixelFace( a_Face ) ? 0_u : gap * 0.5f;
            extent.Above = extent.Ascent + gapAbove;
            extent.Below = extent.Descent + gap - gapAbove;
            return extent;
        }
    }

    u32 TextMetrics::FindRun( const Array<TextRun>& a_Runs, u32 a_Byte )
    {
        const auto it = std::upper_bound( Begin( a_Runs ), End( a_Runs ), a_Byte,
                                          []( u32 a_Value, const TextRun& a_Run ) { return a_Value < a_Run.EndByte; } );
        return static_cast<u32>( it - Begin( a_Runs ) );
    }

    // =========================================================================
    // Prepare
    // =========================================================================

    Optional<PreparedText> TextMetrics::Prepare( StringView a_Text, const TextLayoutStyle& a_Style )
    {
        if ( Empty( a_Text ) )
            return NullOpt;

        const ResolvedFace baseFace = m_Fonts.Resolve( a_Style.GetFontQuery() );
        if ( !baseFace.IsValid() )
            return NullOpt; // No fonts.

        String transformed;
        StringView text = a_Text;
        if ( a_Style.Transform != ETextTransform::None )
        {
            transformed = Unicode::ApplyTextTransformASCII( String{ text }, a_Style.Transform );
            text = transformed;
        }

        PreparedText result;
        result.NormalizedText = TextLayout::NormalizeText( text, a_Style.Wrap );

        if ( Empty( result.NormalizedText ) )
            return result;

        Itemize( result.NormalizedText, a_Style, result.Runs );

        TextLayout::Segment( result, a_Style.Wrap, [&]( u32 a_Start, u32 a_Length )
        {
            return MeasureRange( result, a_Style, a_Start, a_Length );
        } );

        if ( const IFontFace* face = m_Fonts.GetFace( baseFace.Face ) )
        {
            const Unit size = TextShaping::EffectiveSize( *face, a_Style.Size );
            result.HyphenWidth = ToUnit( face->GetAdvance( face->GetGlyphIndex( U'-' ) ), size );
        }

        return result;
    }

    void TextMetrics::Itemize( StringView a_Text, const TextLayoutStyle& a_Style, Array<TextRun>& o_Runs ) const
    {
        Clear( o_Runs );

        const u32 length = static_cast<u32>( Size( a_Text ) );
        if ( length == 0 )
            return;

        const FontQuery    query   = a_Style.GetFontQuery();
        const ResolvedFace primary = m_Fonts.Resolve( query );

        const auto emitRun = [&]( u32 a_Start, u32 a_End, const ResolvedFace& a_Face )
        {
            if ( a_Start >= a_End )
                return;

            const IFontFace* face = m_Fonts.GetFace( a_Face.Face );
            PushBack( o_Runs, TextRun{
                .StartByte     = a_Start,
                .EndByte       = a_End,
                .Face          = a_Face,
                .Size          = face ? TextShaping::EffectiveSize( *face, a_Style.Size ) : a_Style.Size,
                .LetterSpacing = a_Style.LetterSpacing,
            } );
        };

        // Split wherever font fallback picks a different face.
        ResolvedFace current = primary;
        u32 runStart = 0;

        for ( Unicode::UTF8Iterator it( a_Text ); it; ++it )
        {
            const codepoint cp  = *it;
            const u32       pos = static_cast<u32>( it.ByteIndex() );

            // Combining marks etc. stay with their base character's face.
            const ResolvedFace face = ( pos > 0 && Unicode::IsClusterExtender( cp ) )
                ? current
                : m_Fonts.ResolveCodepoint( query, primary, cp );

            if ( pos == 0 )
                current = face;
            else if ( face != current )
            {
                emitRun( runStart, pos, current );
                current = face;
                runStart = pos;
            }
        }

        emitRun( runStart, length, current );
    }

    Unit TextMetrics::ShapeRunRange( const PreparedText& a_Prepared, const TextRun& a_Run, const TextLayoutStyle& a_Style,
                                     u32 a_Start, u32 a_End, Array<ShapedGlyph>& o_Glyphs )
    {
        IFontFace* face = m_Fonts.GetFace( a_Run.Face.Face );
        if ( !face || a_Start >= a_End )
            return 0_u;

        const TextShaping::PieceParams params{
            .Face          = a_Run.Face,
            .Size          = a_Run.Size,
            .LetterSpacing = a_Run.LetterSpacing,
            .WordSpacing   = a_Style.WordSpacing,
            .Direction     = a_Style.Direction,
            .Script        = a_Style.Script,
        };

        const StringView piece{ Data( a_Prepared.NormalizedText ) + a_Start, a_End - a_Start };
        return TextShaping::ShapePiece( *face, piece, a_Start, params, o_Glyphs );
    }

    Unit TextMetrics::MeasureRange( const PreparedText& a_Prepared, const TextLayoutStyle& a_Style, u32 a_Start, u32 a_Length )
    {
        const u32 end = a_Start + a_Length;
        const auto& runs = a_Prepared.Runs;

        Unit width = 0_u;
        for ( u32 r = FindRun( runs, a_Start ); r < Size( runs ) && runs[r].StartByte < end; ++r )
        {
            const u32 start = std::max( a_Start, runs[r].StartByte );
            const u32 stop  = std::min( end, runs[r].EndByte );

            Clear( m_Scratch );
            width += ShapeRunRange( a_Prepared, runs[r], a_Style, start, stop, m_Scratch );
        }

        return width;
    }

    // =========================================================================
    // Shape
    // =========================================================================

    Optional<ShapedText> TextMetrics::Shape( const PreparedText& a_Prepared, const TextLayoutStyle& a_Style, Vec2<Unit> a_MaxSize )
    {
        // TODO: No bidi reordering yet, so a line mixing LTR and RTL runs comes out in logical order.
        //       Need to look into UAX #9 (or just use ICU / fribidi) before RTL is actually usable.

        if ( a_MaxSize[0] <= 0_u || a_MaxSize[1] <= 0_u )
            return NullOpt;

        if ( Empty( a_Prepared.Segments ) || Empty( a_Prepared.Runs ) )
            return NullOpt;

        const Unit maxWidth = a_MaxSize[0];
        const u32  maxLines = a_Style.MaxLines;

        bool exceededMaxLines = false;
        if ( maxLines > 0 )
        {
            u32 totalLines = 0;
            TextLayout::WalkLines( a_Prepared, maxWidth, maxLines + 1u, [&]( u32, u32, Unit ) { ++totalLines; } );
            exceededMaxLines = ( totalLines > maxLines );
        }

        const bool ellipsis           = ( a_Style.Overflow == ETextOverflow::Ellipsis );
        const bool hasWidthConstraint = ( maxWidth < Limits<Unit>::max() );
        const u32  textLength         = static_cast<u32>( Size( a_Prepared.NormalizedText ) );
        const auto& segs              = a_Prepared.Segments;
        const auto& runs              = a_Prepared.Runs;

        ShapedText result;
        Unit cursorY = 0_u;

        const auto makeShapedRun = [&]( const TextRun& a_Run, u32 a_GlyphStart, u32 a_GlyphEnd )
        {
            const IFontFace* face = m_Fonts.GetFace( a_Run.Face.Face );
            return ShapedRun{
                .GlyphStart   = a_GlyphStart,
                .GlyphEnd     = a_GlyphEnd,
                .Face         = a_Run.Face,
                .Mode         = face ? face->GetMode() : EGlyphRenderMode::MTSDF,
                .Size         = a_Run.Size,
                .FillColor    = a_Run.FillColor,
                .HasFillColor = a_Run.HasFillColor,
                .Decorations  = a_Run.Decorations,
            };
        };

        // Truncates the current line to fit an ellipsis. Returns the new line width.
        const auto applyEllipsis = [&]( const ShapedLine& a_Line ) -> Unit
        {
            const TextRun& styleRun = runs[std::min<u32>( FindRun( runs, Back( result.Glyphs ).Cluster ), static_cast<u32>( Size( runs ) ) - 1 )];
            IFontFace* face = m_Fonts.GetFace( styleRun.Face.Face );

            Clear( m_Scratch );
            Unit ellipsisWidth = 0_u;
            if ( face )
            {
                const StringView ellipsisText = face->HasGlyph( 0x2026 ) ? StringView{ "\xE2\x80\xA6" } : StringView{ "..." };
                const TextShaping::PieceParams params{ .Face = styleRun.Face, .Size = styleRun.Size, .LetterSpacing = styleRun.LetterSpacing };
                ellipsisWidth = TextShaping::ShapePiece( *face, ellipsisText, 0, params, m_Scratch );
            }

            const u32 lineGlyphEnd = static_cast<u32>( Size( result.Glyphs ) );
            if ( ellipsisWidth > maxWidth || !face )
            {
                Resize( result.Glyphs, a_Line.Start );
                Resize( result.Runs, a_Line.RunStart );
                return 0_u;
            }

            const Unit budget = maxWidth - ellipsisWidth;
            Unit kept = 0_u;
            u32 keep = a_Line.Start;
            while ( keep < lineGlyphEnd && kept + result.Glyphs[keep].XAdvance <= budget )
                kept += result.Glyphs[keep++].XAdvance;

            // Don't split a cluster.
            while ( keep > a_Line.Start && keep < lineGlyphEnd && result.Glyphs[keep].Cluster == result.Glyphs[keep - 1].Cluster )
                kept -= result.Glyphs[--keep].XAdvance;

            const u32 cutCluster = keep < lineGlyphEnd ? result.Glyphs[keep].Cluster : Back( result.Glyphs ).Cluster;

            Resize( result.Glyphs, keep );
            while ( Size( result.Runs ) > a_Line.RunStart && Back( result.Runs ).GlyphStart >= keep )
                PopBack( result.Runs );
            if ( Size( result.Runs ) > a_Line.RunStart )
                Back( result.Runs ).GlyphEnd = keep;

            for ( ShapedGlyph glyph : m_Scratch )
            {
                glyph.Cluster = cutCluster;
                PushBack( result.Glyphs, glyph );
            }

            const ShapedRun ellipsisRun = makeShapedRun( styleRun, keep, static_cast<u32>( Size( result.Glyphs ) ) );
            if ( Size( result.Runs ) > a_Line.RunStart && Back( result.Runs ).Face == ellipsisRun.Face
                 && Back( result.Runs ).Size == ellipsisRun.Size && Back( result.Runs ).FillColor == ellipsisRun.FillColor
                 && Back( result.Runs ).HasFillColor == ellipsisRun.HasFillColor && Back( result.Runs ).Decorations == ellipsisRun.Decorations )
                Back( result.Runs ).GlyphEnd = ellipsisRun.GlyphEnd;
            else
                PushBack( result.Runs, ellipsisRun );

            return kept + ellipsisWidth;
        };

        TextLayout::WalkLines( a_Prepared, maxWidth, maxLines, [&]( u32 a_LineStartSeg, u32 a_LineEndSeg, Unit )
        {
            // Trailing spaces / hard breaks aren't shaped.
            u32 end = a_LineEndSeg;
            while ( end > a_LineStartSeg && segs[end - 1].Kind != ESegmentKind::Text )
                --end;

            u32 byteStart = a_LineStartSeg < Size( segs ) ? segs[a_LineStartSeg].StartByte : textLength;
            u32 byteEnd   = byteStart;
            if ( end > a_LineStartSeg )
                byteEnd = segs[end - 1].StartByte + segs[end - 1].ByteLength;

            ShapedLine line{
                .Start    = static_cast<u32>( Size( result.Glyphs ) ),
                .RunStart = static_cast<u32>( Size( result.Runs ) ),
            };

            Unit width = 0_u;
            for ( u32 r = FindRun( runs, byteStart ); r < Size( runs ) && runs[r].StartByte < byteEnd; ++r )
            {
                const u32 start = std::max( byteStart, runs[r].StartByte );
                const u32 stop  = std::min( byteEnd, runs[r].EndByte );
                const u32 glyphStart = static_cast<u32>( Size( result.Glyphs ) );

                width += ShapeRunRange( a_Prepared, runs[r], a_Style, start, stop, result.Glyphs );

                const u32 glyphEnd = static_cast<u32>( Size( result.Glyphs ) );
                if ( glyphEnd > glyphStart )
                    PushBack( result.Runs, makeShapedRun( runs[r], glyphStart, glyphEnd ) );
            }

            const bool isLastLine    = ( maxLines > 0 && result.LineCount() == maxLines - 1 );
            const bool forceEllipsis = ellipsis && exceededMaxLines && isLastLine;
            const bool lineOverflows = ellipsis && hasWidthConstraint && width > maxWidth + TextLayout::c_LineFitEpsilon;

            if ( ( forceEllipsis || lineOverflows ) && Size( result.Glyphs ) > line.Start )
                width = applyEllipsis( line );

            line.End    = static_cast<u32>( Size( result.Glyphs ) );
            line.RunEnd = static_cast<u32>( Size( result.Runs ) );
            line.Width  = width;

            // Line box from the runs on this line. Empty lines use the run at their position.
            RunExtent extent{};
            bool anyRun = false;
            const auto include = [&]( const ResolvedFace& a_Face, Unit a_Size )
            {
                if ( const IFontFace* face = m_Fonts.GetFace( a_Face.Face ) )
                {
                    extent.Include( ComputeRunExtent( *face, a_Size ) );
                    anyRun = true;
                }
            };

            for ( u32 r = line.RunStart; r < line.RunEnd; ++r )
                include( result.Runs[r].Face, result.Runs[r].Size );

            if ( !anyRun )
            {
                const u32 r = std::min<u32>( FindRun( runs, byteStart ), static_cast<u32>( Size( runs ) ) - 1 );
                include( runs[r].Face, runs[r].Size );
            }

            if ( a_Style.LineHeight > 0_u )
            {
                line.Height   = a_Style.LineHeight;
                line.Baseline = cursorY + ( line.Height - extent.Ascent - extent.Descent ) * 0.5f + extent.Ascent;
            }
            else
            {
                line.Height   = extent.Above + extent.Below;
                line.Baseline = cursorY + extent.Above;
            }

            line.Top     = cursorY;
            line.Height += a_Style.LineSpacing;
            cursorY     += line.Height;

            PushBack( result.Lines, line );
            result.MaxWidth = std::max( result.MaxWidth, width );
        } );

        if ( Empty( result.Lines ) )
            return NullOpt;

        // Line spacing goes between lines, not after the last one.
        result.TotalHeight = cursorY - a_Style.LineSpacing;
        return result;
    }

} // namespace RatUI
