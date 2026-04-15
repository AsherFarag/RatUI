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
    // TODO: Shouldnt be here
    inline constexpr double c_MtsdfPxRange = 4.0;

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

			// Total height is the distance from the top of the first line to the bottom of the last line, including line spacing.
			// TotalHeight = Ascender + (LineCount - 1) * LineHeight + abs(Descender)
            result.TotalHeight = result.Ascender
                + std::max( 0.f, static_cast<f32>( result.LineCount() ) - 1.f ) * result.LineHeight
                + std::abs( result.Descender );

            return result;
        }

        bool RasterizeGlyph(
            FontHandle a_Font, u32 a_GlyphIndex, u32 a_FontSize,
            const Coloru8*& o_Pixels, u32& o_Width, u32& o_Height,
            Vec2i& o_Bearing
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
            double advance = 0.0;
            const bool glyphLoaded = msdfgen::loadGlyph( shape, msdfFont, msdfgen::GlyphIndex( a_GlyphIndex ), &advance );
            msdfgen::destroyFont( msdfFont );

            if ( !glyphLoaded )
                return false;

            // Handle empty/invisible glyphs (e.g., space).
            if ( shape.contours.empty() )
            {
                o_Pixels  = nullptr;
                o_Width   = 0;
                o_Height  = 0;
                o_Bearing = Vec2i{ 0, 0 };
                return true;
            }

            // Normalize shape windings and assign edge colors for MTSDF generation.
            shape.normalize();
            msdfgen::edgeColoringSimple( shape, 3.0 );

            // Get shape bounds in font design units.
            double boundsL = 0.0, boundsB = 0.0, boundsR = 0.0, boundsT = 0.0;
            shape.bound( boundsL, boundsB, boundsR, boundsT );

            // Scale from design units to pixels.
            const double emSize = static_cast<double>( face->units_per_EM );
            const double scale  = 64.0 * static_cast<double>( a_FontSize ) / emSize;

            const int padding = static_cast<int>( std::ceil( c_MtsdfPxRange ) );

            // Bitmap dimensions in pixels.
            const int w = static_cast<int>( std::ceil( ( boundsR - boundsL ) * scale ) ) + 2 * padding;
            const int h = static_cast<int>( std::ceil( ( boundsT - boundsB ) * scale ) ) + 2 * padding;

            if ( w <= 0 || h <= 0 )
            {
                o_Pixels  = nullptr;
                o_Width   = 0;
                o_Height  = 0;
                o_Bearing = Vec2i{ 0, 0 };
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
            msdfgen::generateMTSDF( mtsdf, shape, projection, c_MtsdfPxRange );

            // Convert the float RGBA MTSDF bitmap to RGBA8 (Y-down, row-major).
            Resize( m_RasterBuffer, static_cast<size>( w ) * h );

            auto toU8 = []( float v ) -> u8 {
                return static_cast<u8>( std::clamp( static_cast<int>( v * 255.f + 0.5f ), 0, 255 ) );
            };

            for ( int y = 0; y < h; ++y )
            {
                for ( int x = 0; x < w; ++x )
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

            // Bearing: offset from glyph origin to bitmap top-left.
            // X: left edge of glyph in pixels minus padding.
            // Y: top edge of glyph in pixels plus padding (FreeType Y-up convention).
            o_Bearing = Vec2i{
                static_cast<i32>( std::floor( boundsL * scale ) ) - padding,
                static_cast<i32>( std::ceil( boundsT * scale ) ) + padding
            };

            return true;
        }

    protected:
        FontCache* m_FontCache{ nullptr };
        Array<Coloru8> m_RasterBuffer; ///< Persistent buffer for the last rasterized glyph's RGBA8 pixel data.
    };

} // namespace RatUI::FreeType