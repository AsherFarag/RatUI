#pragma once
#include "FontCache.h"
#include "TextUtil.h"
#include "../../Text/GlyphAtlas.h"
#include <msdfgen.h>
#include <ext/import-font.h>
#include <msdfgen-ext.h>

#include <cmath>

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
            result.Font               = a_Style.Font;
            result.FontSize           = a_Style.Size;
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

            const f32 sdfPadDisplay = static_cast<f32>( c_MsdfPxRange ) * a_Style.Size / 64.f;
            result.Ascender += sdfPadDisplay;         // shifts penY down → headroom above caps
            result.TotalHeight = result.Ascender
                + ( result.LineCount() - 1.f ) * result.LineHeight
                + std::abs( result.Descender ) + sdfPadDisplay;   // extra depth below descenders

            return result;
        }

        bool RasterizeGlyph(
            FontHandle a_Font, u32 a_GlyphIndex, u32 a_FontSize,
            const Coloru8*& o_Pixels, u32& o_Width, u32& o_Height,
            Vec2i& o_Bearing, Vec2i& o_PlaneSize
        ) override
        {
            if ( !m_FontCache )
                return false;

            Font* font = m_FontCache->GetOrLoadFont( a_Font, a_FontSize );
            if ( !font )
                return false;

            FT_Face face = font->GetFace();

            // Wrap the existing FreeType face for msdfgen (non-owning).
            msdfgen::FontHandle* msdfFont = msdfgen::adoptFreetypeFont( face );
            if ( !msdfFont )
                return false;

            // Load the glyph shape from the font.
            msdfgen::Shape shape;
            f64 advance = 0.0;
            const bool glyphLoaded = msdfgen::loadGlyph( shape, msdfFont, msdfgen::GlyphIndex( a_GlyphIndex ), &advance );
            msdfgen::destroyFont( msdfFont );

            if ( !glyphLoaded )
                return false;

            // Handle empty/invisible glyphs (e.g., space).
            if ( shape.contours.empty() )
            {
                o_Pixels    = nullptr;
                o_Width     = 0;
                o_Height    = 0;
                o_Bearing   = Vec2i{ 0, 0 };
                o_PlaneSize = Vec2i{ 0, 0 };
                return true;
            }

            // Normalize shape windings and assign edge colors for MTSDF generation.
            shape.normalize();
            msdfgen::edgeColoringSimple( shape, 3.0 );

            // Get shape bounds in font design units.
            f64 boundsL = 0.0, boundsB = 0.0, boundsR = 0.0, boundsT = 0.0;
            shape.bound( boundsL, boundsB, boundsR, boundsT );

            // Scale from design units to pixels.
            const f64 emSize = static_cast<f64>( face->units_per_EM );
            const f64 scale  = 64.0 * static_cast<f64>( a_FontSize ) / emSize;

            const i32 padding = static_cast<i32>( std::ceil( c_MsdfPxRange ) );

            // Bitmap dimensions in pixels.
            const i32 w = static_cast<i32>( std::ceil( ( boundsR - boundsL ) * scale ) ) + 2 * padding;
            const i32 h = static_cast<i32>( std::ceil( ( boundsT - boundsB ) * scale ) ) + 2 * padding;

            if ( w <= 0 || h <= 0 )
            {
                o_Pixels    = nullptr;
                o_Width     = 0;
                o_Height    = 0;
                o_Bearing   = Vec2i{ 0, 0 };
                o_PlaneSize = Vec2i{ 0, 0 };
                return true;
            }

            // Projection: map shape design-unit coordinates to bitmap pixel coordinates.
            // The translation positions the glyph so that its bounds start at the padding offset.
            const msdfgen::Projection projection(
                msdfgen::Vector2( scale, scale ),
                msdfgen::Vector2( -boundsL * scale + padding, -boundsB * scale + padding )
            );

            // Generate the multi-channel typed signed distance field (RGBA: RGB = MSDF, A = SDF).
            msdfgen::Bitmap<float, 4> mtsdf( w, h );
            msdfgen::generateMTSDF( mtsdf, shape, projection, c_MsdfPxRange );

            // Convert the float RGBA MTSDF bitmap to RGBA8 (Y-down, row-major).
            Resize( m_RasterBuffer, static_cast<size>( w ) * h );

            auto toU8 = []( float v ) -> u8 {
                return static_cast<u8>( std::clamp( static_cast<i32>( v * 255.f + 0.5f ), 0, 255 ) );
            };

            for ( i32 y = 0; y < h; ++y )
            {
                for ( i32 x = 0; x < w; ++x )
                {
                    const float* px = mtsdf( x, h - 1 - y ); // msdfgen is Y-up; we need Y-down.
                    RawAt( m_RasterBuffer, static_cast<size>( y ) * w + x ) = Coloru8{
                        toU8( px[0] ), toU8( px[1] ), toU8( px[2] ), toU8( px[3] )
                    };
                }
            }

            o_Pixels = Data( m_RasterBuffer );
            o_Width  = static_cast<u32>( w );
            o_Height = static_cast<u32>( h );

            // Bearing: offset from the baseline origin to the top-left corner of the SDF bitmap
            // (including SDF padding). X is the left edge in base-size pixels from the pen origin;
            // Y is the top edge in base-size pixels above the baseline (Y-up convention).
            o_Bearing = Vec2i{
                static_cast<i32>( std::floor( boundsL * scale ) ) - padding,
                static_cast<i32>( std::ceil( boundsT * scale ) ) + padding
            };

            // PlaneSize: ink-only extent in base-size pixels (bitmap minus the two SDF padding rings).
            o_PlaneSize = Vec2i{
                w - 2 * padding,
                h - 2 * padding
            };

            return true;
        }

    protected:
        FontCache* m_FontCache{ nullptr };
        Array<Coloru8> m_RasterBuffer; ///< Persistent buffer for the last rasterized glyph's RGBA8 pixel data.
    };

} // namespace RatUI::FreeType