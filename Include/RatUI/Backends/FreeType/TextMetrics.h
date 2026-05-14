#pragma once
#include "FontCache.h"
#include "TextUtil.h"
#include "../../Text/GlyphAtlas.h"

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

            Font* font = m_FontCache->GetOrLoadFont( a_Style.Font );
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

        Optional<ShapedText> Shape( const PreparedText& a_Prepared, const TextLayoutStyle& a_Style, Vec2<Unit> a_MaxSize = { Limits<Unit>::max(), Limits<Unit>::max() } ) override
        {
            if ( a_MaxSize[0] <= 0_u || a_MaxSize[1] <= 0_u )
                return NullOpt; // Return empty shaped text if the available space is zero or negative.

            if ( !m_FontCache || !a_Style.Font.IsValid() || Empty( a_Prepared.Segments ) )
                return NullOpt; // Return empty shaped text if there are no segments to shape.

            Font* font = m_FontCache->GetOrLoadFont( a_Style.Font );
            if ( !font )
                return NullOpt; // Return empty shaped text if the font couldn't be loaded.

            FT_Face   face      = font->GetFace();
            const u32 pixelSize = static_cast<u32>( a_Style.Size.ToFloat() );

            const f32 unitsPerEM = static_cast<f32>( face->units_per_EM );

            ShapedText result;
            result.Font               = a_Style.Font;
            result.FontSize           = a_Style.Size;
            result.LineHeight         = GetLineHeight( face, a_Style ) + a_Style.LineSpacing;
            result.Ascender           = Unit{ ( static_cast<f32>( face->ascender )  / unitsPerEM ) * a_Style.Size.ToFloat() };
            result.Descender          = Unit{ ( static_cast<f32>( face->descender ) / unitsPerEM ) * a_Style.Size.ToFloat() };
            result.UnderlinePosition  = Unit{ -( static_cast<f32>( face->underline_position  ) / unitsPerEM ) * a_Style.Size.ToFloat() };
			result.UnderlineThickness = Unit{ std::max( 1.f, ( static_cast<f32>( face->underline_thickness ) / unitsPerEM ) * a_Style.Size.ToFloat() ) };

            const Unit maxWidth = a_MaxSize[0];
            const u32 maxLines = a_Style.MaxLines;

            // Pre-walk to detect whether the text would exceed the line limit (for forced ellipsis on the last line).
            bool exceededMaxLines = false;
            if ( maxLines > 0 )
            {
                u32 totalLines = 0;
                TextLayout::WalkLines( a_Prepared, maxWidth, maxLines + 1u, [&]( u32, u32, Unit ) { ++totalLines; } );
                exceededMaxLines = ( totalLines > maxLines );
            }

            const bool ellipsis           = ( a_Style.Overflow == ETextOverflow::Ellipsis );
            const bool hasWidthConstraint = ( maxWidth < Limits<Unit>::max() );
            const StringView norm         = a_Prepared.NormalizedText;

            Array<ShapedGlyph> lineGlyphs; // reused per line

            Array<ShapedGlyph> ellipsisGlyphs;
            Unit ellipsisWidth = 0_u;

            if ( ellipsis )
            {
                constexpr StringView c_Ellipsis = "...";
                ellipsisWidth = ToUnit(
                    ShapeLine( *font, c_Ellipsis, a_Style, ellipsisGlyphs ),
                    a_Style.Size
                );
            }

            TextLayout::WalkLines( a_Prepared, maxWidth, maxLines,
                [&]( u32 lineStartSeg, u32 lineEndSeg, Unit paintWidth )
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

                    // Shape the line with HarfBuzz to get per-glyph positions.
					paintWidth = ToUnit( ShapeLine( *font, lineText, a_Style, lineGlyphs ), a_Style.Size );

                    const bool isLastLine = ( maxLines > 0 && result.LineCount() == maxLines - 1 );
                    const bool forceEllipsis = ellipsis && exceededMaxLines && isLastLine;
                    const bool lineOverflows = ellipsis && hasWidthConstraint
                        && paintWidth > maxWidth + TextLayout::c_LineFitEpsilon;

                    if ( ( forceEllipsis || lineOverflows ) && !Empty( lineGlyphs ) )
                    {
                        if ( ellipsisWidth <= maxWidth )
                        {
                            const Unit prefixBudget = maxWidth - ellipsisWidth;

                            Unit accumulated = 0_u;
                            u32  keepCount = 0;

                            for ( u32 i = 0; i < Size( lineGlyphs ); ++i )
                            {
                                const Unit adv = ToUnit( lineGlyphs[i].XAdvance, a_Style.Size );

                                if ( accumulated + adv > prefixBudget )
                                    break;

                                accumulated += adv;
                                ++keepCount;
                            }

                            Resize( lineGlyphs, keepCount );

                            Insert(
                                lineGlyphs,
                                End( lineGlyphs ),
                                Begin( ellipsisGlyphs ),
                                End( ellipsisGlyphs )
                            );

                            paintWidth = accumulated + ellipsisWidth;
                        }
                        else
                        {
                            Clear( lineGlyphs );
                            paintWidth = 0_u;
                        }
                    }

                    // Append glyphs to the output and record the line metadata.
                    const u32 glyphStart = static_cast<u32>( Size( result.Glyphs ) );
                    Insert( result.Glyphs, End( result.Glyphs ), Begin( lineGlyphs ), End( lineGlyphs ) );
                    const u32 glyphEnd = static_cast<u32>( Size( result.Glyphs ) );

                    PushBack( result.Lines, ShapedLine{
                        .Start   = glyphStart,
                        .End     = glyphEnd,
                        .Width   = paintWidth
                    } );

                    result.MaxWidth = Unit{ std::max( result.MaxWidth.ToFloat(), paintWidth.ToFloat() ) };
                }
            );

            if ( Empty( result.Lines ) )
                return NullOpt;

            // Convert SDF padding from atlas pixels to display units.
            // At baseSize pixels per EM, the padding in EM space is pxRange/baseSize.
            // Multiplying by fontSize gives display units.
            const f32 baseSize      = static_cast<f32>( c_MsdfPxRange * 2 ); // or your atlas BaseSize
            const f32 sdfPadEm      = static_cast<f32>( c_MsdfPxRange ) / baseSize;
            const f32 sdfPadDisplay = sdfPadEm * a_Style.Size.ToFloat();

            result.Ascender += Unit{ sdfPadDisplay };
            result.TotalHeight = result.LineHeight * result.LineCount()
                + Unit{ std::abs( result.Descender.ToFloat() ) + sdfPadDisplay };

            return result;
        }

        bool RasterizeGlyph(
            FontHandle a_Font, codepoint a_Codepoint, u32 a_SdfPixelSize,
            const Color*& o_Pixels, u32& o_Width, u32& o_Height,
            Vec2<FontUnit>& o_Bearing, FontUnit& o_XAdvance
        ) override
        {
            o_Pixels    = nullptr;
            o_Width     = 0;
            o_Height    = 0;
            o_Bearing   = Vec2<FontUnit>{ 0_fu, 0_fu };
            o_XAdvance  = 0_fu;
        
            if (!m_FontCache)
                return false;
        
            Font* font = m_FontCache->GetOrLoadFont( a_Font );
            if (!font || !font->IsValid())
                return false;
        
            const FT_Face face = font->GetFace();
            msdfgen::FontHandle* msdfFont = font->GetMsdfFont();
        
            // ----------------------------
            // Load glyph shape + advance
            // ----------------------------
            msdfgen::Shape shape;
            f64 advance = 0.0;
        
            if (!msdfgen::loadGlyph(
                    shape,
                    msdfFont,
                    msdfgen::GlyphIndex(a_Codepoint),
                    msdfgen::FONT_SCALING_EM_NORMALIZED,
                    &advance
                ))
            {
                return false;
            }

            o_XAdvance = FontUnit{ static_cast<f32>(advance) };
        
            // Empty glyph (space, etc.)
            if (shape.contours.empty())
                return true;
        
            //shape.normalize();
            msdfgen::edgeColoringSimple(shape, 3.0);
        
            // ----------------------------
            // Shape bounds (EM-normalised font space)
            // ----------------------------
            f64 l = 0.0, b = 0.0, r = 0.0, t = 0.0;
            shape.bound(l, b, r, t);
        
            // ----------------------------
            // Scale: EM-normalised units -> atlas pixels
            // With FONT_SCALING_EM_NORMALIZED, 1.0 == 1 EM.
            // ----------------------------
            const f64 scale = static_cast<f64>( a_SdfPixelSize );
        
            // Add padding to prevent clipping of SDF fringes (in atlas pixels)
            const i32 padding = static_cast<i32>( std::ceil( c_MsdfPxRange ) );
        
            const i32 w = static_cast<i32>(std::ceil((r - l) * scale)) + 2 * padding;
            const i32 h = static_cast<i32>(std::ceil((t - b) * scale)) + 2 * padding;
        
            if (w <= 0 || h <= 0)
                return true;
        
            // ----------------------------
            // Projection (EM-normalised space -> atlas pixel space)
            //
            // msdfgen::Projection::project(coord) = scale * (coord + translate)
            // so translate must be in EM space, not pixel space.
            //
            // We want: project(l) = padding  and  project(b) = padding
            //   scale * (l + translate.x) = padding  =>  translate.x = padding/scale - l
            //   scale * (b + translate.y) = padding  =>  translate.y = padding/scale - b
            // ----------------------------
            const f64 padEm = static_cast<f64>(padding) / scale; // SDF padding in EM units
            const msdfgen::Projection projection(
                msdfgen::Vector2(scale, scale),
                msdfgen::Vector2(
                    padEm - l,
                    padEm - b
                )
            );
        
            msdfgen::Bitmap<f32, 4> mtsdf(w, h);
            // c_MsdfPxRange is in atlas pixels; generateMTSDF expects the range in shape-space
            // units (EM-normalised, since FONT_SCALING_EM_NORMALIZED is used).
            // Convert: range_em = pxRange / pixels_per_EM = c_MsdfPxRange / scale.
            msdfgen::generateMTSDF(mtsdf, shape, projection, c_MsdfPxRange / scale);
        
            // ----------------------------
            // Convert to RGBA8
            // ----------------------------
            {
                Resize(m_RasterBuffer, static_cast<size>(w) * h);
                
                auto toU8 = +[](f32 v) -> u8
                {
                    return static_cast<u8>(
                        std::clamp(static_cast<i32>(v * 255.0f + 0.5f), 0, 255)
                    );
                };
            
                // TODO: SIMD this?
                for (i32 y = 0; y < h; ++y)
                {
                    for (i32 x = 0; x < w; ++x)
                    {
                        const size idx = static_cast<size>(y) * w + x;
                        auto px = mtsdf( x, h - 1 - y );

                        RawAt( m_RasterBuffer, idx ) = Color{
                            toU8( px[0] ),
                            toU8( px[1] ),
                            toU8( px[2] ),
                            toU8( px[3] )
                        };
                    }
                }
            }
            
        
            o_Pixels = Data(m_RasterBuffer);
            o_Width  = static_cast<u32>(w);
            o_Height = static_cast<u32>(h);
        
            // ----------------------------
            // Bearing (EM-normalised space, same coordinate system as FontUnit)
            //
            // The atlas bitmap includes `padding` pixels of SDF fringe on every side,
            // so the bitmap's top-left corner in EM space is:
            //
            //   left  = l - padEm   (padEm pixels to the LEFT  of the shape left edge)
            //   top   = t + padEm   (padEm pixels ABOVE the shape top edge)
            //
            // In DrawBatcher::EmitText the bearing is used to position the glyph quad
            // (which covers the full bitmap, fringe included):
            //   gx = penX + XOffset_display + Bearing[0] * fontSize
            //   gy = penY + YOffset_display - Bearing[1] * fontSize
            // ----------------------------
            o_Bearing = Vec2<FontUnit>{
                static_cast<FontUnit>(l - padEm),   // bitmap left edge in EM space
                static_cast<FontUnit>(t + padEm)    // bitmap top  edge in EM space (positive = above baseline)
            };
        
            return true;
        }

    protected:
        FontCache* m_FontCache{ nullptr };
        Array<Color> m_RasterBuffer; ///< Persistent buffer for the last rasterized glyph's RGBA8 pixel data.
    };

} // namespace RatUI::FreeType
