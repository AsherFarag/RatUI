#pragma once
#include "FontCache.h"
#include "TextUtil.h"

namespace RatUI::FreeType
{

    class TextMetrics : public ITextMetrics
    {
    public:
        TextMetrics( FontCache* a_FontCache = nullptr )
            : m_FontCache( a_FontCache )
        {}

        /** @brief Gets the underlying font cache set by the user. */
        FontCache* GetFontCache() const { return m_FontCache; }

        /** @brief Sets the font cache. */
        void SetFontCache( FontCache* a_Cache ) { m_FontCache = a_Cache; }

        Optional<PreparedText> Prepare( StringView a_Text, const TextLayoutStyle& a_Style ) override
        {
            if ( !m_FontCache || !a_Style.Font.IsValid() || Empty( a_Text ) )
                return NullOpt;

            Font* font = m_FontCache->GetOrLoadFont( a_Style.Font, static_cast<u32>( a_Style.Size ) );
            if ( !font )
                return NullOpt;

            String buffer;
			StringView text = a_Text;
            if ( a_Style.Transform != ETextTransform::None )
            {
                buffer = TextUtil::ApplyTextTransform( String{ a_Text }, a_Style.Transform );
                text = buffer;
			}

            return TextLayout::Prepare( text, a_Style.Wrap,
                [font, &a_Style]( StringView sv )
                {
                    return TextUtil::MeasureLineWidth( *font, sv, a_Style );
                } );
        }

        Optional<ShapedText> Shape( const PreparedText& a_Prepared, const TextLayoutStyle& a_Style, Vec2f a_MaxSize = { Limits<f32>::max(), Limits<f32>::max() } ) override
        {
            if ( a_MaxSize[0] <= 0.f || a_MaxSize[1] <= 0.f )
                return NullOpt; // Return empty shaped text if the available space is zero or negative.

            if ( !m_FontCache || !a_Style.Font.IsValid() || Empty( a_Prepared.Segments ) )
                return NullOpt; // Return empty shaped text if there are no segments to shape.

            Font* font = m_FontCache->GetOrLoadFont( a_Style.Font, static_cast<u32>( a_Style.Size ) );
            if ( !font )
                return NullOpt; // Return empty shaped text if the font couldn't be loaded.

            FT_Face   face      = font->GetFace();
            const u32 pixelSize = static_cast<u32>( a_Style.Size );

            ShapedText result;
            result.PixelSize          = pixelSize;
            result.LineHeight         = GetLineHeight( face, a_Style ) + a_Style.LineSpacing;
            result.Ascender           = face->size->metrics.ascender  / 64.f;
            result.Descender          = face->size->metrics.descender / 64.f;
            result.UnderlinePosition  = -( FT_MulFix( face->underline_position,  face->size->metrics.y_scale ) / 64.f );
            result.UnderlineThickness = std::max( 1.f, FT_MulFix( face->underline_thickness, face->size->metrics.y_scale ) / 64.f );

            const f32 maxWidth = a_MaxSize[0];
            const u32 maxLines = a_Style.MaxLines;

            // Pre-walk to detect whether the text would exceed the line limit (for forced ellipsis on the last line).
            bool exceededMaxLines = false;
            if ( maxLines > 0 )
            {
                u32 totalLines = 0;
                TextLayout::WalkLines( a_Prepared, maxWidth, maxLines + 1u, [&]( u32, u32, f32 ) { ++totalLines; } );
                exceededMaxLines = ( totalLines > maxLines );
            }

            const bool ellipsis           = ( a_Style.Overflow == ETextOverflow::Ellipsis );
            const bool hasWidthConstraint = ( maxWidth < Limits<f32>::max() );
            const StringView norm         = a_Prepared.NormalizedText;

            Array<ShapedGlyph> lineGlyphs; // reused per line

            TextLayout::WalkLines( a_Prepared, maxWidth, maxLines,
                [&]( u32 lineStartSeg, u32 lineEndSeg, f32 paintWidth )
                {
                    // Materialize the line text from segment byte ranges.
                    const auto& segs = a_Prepared.Segments;
                    u32 end = lineEndSeg;
                    while ( end > lineStartSeg && segs[end - 1].Kind != ESegmentKind::Text )
                        --end;

                    StringView lineText{};
                    if ( end > lineStartSeg )
                    {
                        const u32 byteStart = segs[lineStartSeg].StartByte;
                        const u32 byteEnd   = segs[end - 1].StartByte + segs[end - 1].ByteLength;
                        lineText = StringView{ Data( norm ) + byteStart, byteEnd - byteStart };
                    }

                    // Ellipsis: force-truncate the last line when maxLines is exceeded, or truncate
                    // any line that is wider than the available width (e.g. NoWrap + Ellipsis mode).
                    String truncated;
                    const bool isLastLine    = ( maxLines > 0 && result.LineCount() == maxLines - 1 );
                    const bool forceEllipsis = ellipsis && exceededMaxLines && isLastLine;
                    const bool lineOverflows = ellipsis && hasWidthConstraint
                                              && paintWidth > maxWidth + TextLayout::c_LineFitEpsilon;

                    if ( ( forceEllipsis || lineOverflows ) && !Empty( lineText ) )
                    {
                        truncated  = TextUtil::TruncateLineWithEllipsis( *font, lineText, a_Style, maxWidth, forceEllipsis );
                        lineText   = StringView{ truncated };
                    }

                    // Shape the line with HarfBuzz to get per-glyph positions.
                    paintWidth = ShapeLine( *font, lineText, a_Style, lineGlyphs );

                    // Append glyphs to the output and record the line metadata.
                    const u32 glyphStart = static_cast<u32>( Size( result.Glyphs ) );
                    Insert( result.Glyphs, End( result.Glyphs ), Begin( lineGlyphs ), End( lineGlyphs ) );
                    const u32 glyphEnd = static_cast<u32>( Size( result.Glyphs ) );

                    PushBack( result.Lines, ShapedLine{
                        .Start   = glyphStart,
                        .End     = glyphEnd,
                        .Width   = paintWidth,
                    } );

                    result.MaxWidth = std::max( result.MaxWidth, paintWidth );
                }
            );

            if ( Empty( result.Lines ) )
                return NullOpt;

			// Total height is the distance from the top of the first line to the bottom of the last line, including line spacing.
			// TotalHeight = Ascender + (LineCount - 1) * LineHeight + abs(Descender)
            result.TotalHeight = result.Ascender
                + std::max( 0.f, static_cast<f32>( result.LineCount() ) - 1.f ) * result.LineHeight
                + std::abs( result.Descender );

            return result;
        }

        bool RasterizeGlyph(
            FontHandle a_Font, c32 a_CodePoint, u32 a_FontSize,
            const Coloru8*& a_OutPixels, u32& a_OutWidth, u32& a_OutHeight,
            Vec2i& a_OutBearing
        )
        {
            if ( !m_FontCache )
                return false;

            Font* font = m_FontCache->GetOrLoadFont( a_Font, a_FontSize );
            if ( !font )
                return false;

            FT_Face face = font->GetFace();

            FT_UInt glyphIndex = FT_Get_Char_Index( face, a_CodePoint );
            if ( glyphIndex == 0 )
                return false; // Glyph not found in the font.

            if ( FT_Load_Glyph( face, glyphIndex, FT_LOAD_RENDER ) != 0 )
                return false; // Failed to load and render the glyph.

            FT_GlyphSlot slot = face->glyph;
            a_OutPixels       = reinterpret_cast<const Coloru8*>( slot->bitmap.buffer );
            a_OutWidth        = slot->bitmap.width;
            a_OutHeight       = slot->bitmap.rows;
            a_OutBearing      = Vec2i{ slot->bitmap_left, slot->bitmap_top };

            return true;
        }

    protected:
        FontCache* m_FontCache{ nullptr };
    };

} // namespace RatUI::FreeType