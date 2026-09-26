#include <RatUI/Text/SpriteSheetFont.h>
#include <RatUI/Text/PixelGlyphOps.h>
#include <RatUI/Text/Unicode.h>

#include <algorithm>
#include <cmath>

namespace RatUI
{
    namespace
    {
        class SpriteSheetFace final : public IFontFace
        {
        public:
            struct Glyph
            {
                Array<Color> Pixels;          ///< Trimmed bitmap.
                u32          Width { 0 };
                u32          Height{ 0 };
                i32          BearingX{ 0 };   ///< Native px from the pen to the bitmap's left edge.
                i32          BearingY{ 0 };   ///< Native px from the baseline up to the bitmap's top edge.
                u32          Advance{ 0 };    ///< Native px.
            };

            bool Build( const SpriteSheetFontDesc& a_Desc )
            {
                const u32 cellW = a_Desc.CellSize[0];
                const u32 cellH = a_Desc.CellSize[1];
                if ( !a_Desc.Pixels || cellW == 0 || cellH == 0 || Empty( a_Desc.Characters ) )
                    return false;

                m_Name = String{ a_Desc.Name };
                m_Cell = { cellW, cellH };

                const u32 baseline   = a_Desc.Baseline > 0 ? std::min( a_Desc.Baseline, cellH ) : cellH;
                const u32 lineHeight = a_Desc.LineHeight > 0 ? a_Desc.LineHeight : cellH + 1;
                const u32 stepX      = cellW + a_Desc.CellSpacing[0];
                const u32 stepY      = cellH + a_Desc.CellSpacing[1];
                const u32 columns    = a_Desc.ImageSize[0] > a_Desc.CellOrigin[0]
                                        ? ( a_Desc.ImageSize[0] - a_Desc.CellOrigin[0] + a_Desc.CellSpacing[0] ) / stepX
                                        : 0;
                if ( columns == 0 )
                    return false;

                m_Baseline = baseline;

                const u32 defaultSpace = a_Desc.SpaceAdvance > 0
                                        ? a_Desc.SpaceAdvance
                                        : ( a_Desc.Monospace ? cellW : std::max( 1u, cellW / 2 ) );

                // Glyph 0 is .notdef: a hollow box so missing characters are visible.
                PushBack( m_Glyphs, MakeNotDefGlyph( cellW, baseline, a_Desc.Monospace ? cellW : cellW / 2 + 2 ) );

                u32 cellIndex = 0;
                for ( Unicode::UTF8Iterator it( a_Desc.Characters ); it; ++it, ++cellIndex )
                {
                    const codepoint cp = *it;
                    const u32 cellX = a_Desc.CellOrigin[0] + ( cellIndex % columns ) * stepX;
                    const u32 cellY = a_Desc.CellOrigin[1] + ( cellIndex / columns ) * stepY;

                    if ( cellX + cellW > a_Desc.ImageSize[0] || cellY + cellH > a_Desc.ImageSize[1] )
                        break; // Ran off the image.

                    Glyph glyph = ExtractGlyph( a_Desc, cellX, cellY, baseline );

                    if ( Unicode::IsWhitespace( cp ) && glyph.Width == 0 )
                        glyph.Advance = defaultSpace;

                    PushBack( m_Glyphs, std::move( glyph ) );
                    m_CharMap[cp] = static_cast<u32>( Size( m_Glyphs ) - 1 );
                }

                if ( Find( m_CharMap, U' ' ) == End( m_CharMap ) )
                {
                    PushBack( m_Glyphs, Glyph{ .Advance = defaultSpace } );
                    m_CharMap[U' '] = static_cast<u32>( Size( m_Glyphs ) - 1 );
                }

                for ( const SpriteSheetKerningPair& pair : a_Desc.Kerning )
                {
                    const u32 left  = ToUnderlying( GetGlyphIndex( pair.Left ) );
                    const u32 right = ToUnderlying( GetGlyphIndex( pair.Right ) );
                    if ( left != 0 && right != 0 )
                        m_Kerning[( static_cast<u64>( left ) << 32 ) | right] = pair.Amount;
                }

                // The em is one cell high.
                const f32 em = static_cast<f32>( cellH );
                m_Metrics.NativePixelSize    = em;
                m_Metrics.Mode               = EGlyphRenderMode::Pixel;
                m_Metrics.Ascender           = FontUnit{ static_cast<f32>( baseline ) / em };
                m_Metrics.Descender          = FontUnit{ -static_cast<f32>( cellH - baseline ) / em };
                m_Metrics.LineGap            = FontUnit{ static_cast<f32>( static_cast<i32>( lineHeight ) - static_cast<i32>( cellH ) ) / em };
                m_Metrics.UnderlinePosition  = FontUnit{ -1.5f / em };
                m_Metrics.UnderlineThickness = FontUnit{ 1.f / em };
                m_Metrics.StrikeoutPosition  = FontUnit{ ( std::floor( static_cast<f32>( baseline ) * 0.4f ) + 0.5f ) / em };
                m_Metrics.StrikeoutThickness = FontUnit{ 1.f / em };
                m_Metrics.CapHeight          = FontUnit{ static_cast<f32>( baseline ) / em };
                m_Metrics.XHeight            = FontUnit{ std::round( static_cast<f32>( baseline ) * 0.6f ) / em };
                return true;
            }

            EFontFaceKind Kind() const override { return EFontFaceKind::SpriteSheet; }
            const FontFaceMetrics& Metrics() const override { return m_Metrics; }
            StringView GetDebugName() const override { return m_Name; }

            GlyphID GetGlyphIndex( codepoint a_Codepoint ) const override
            {
                const auto it = Find( m_CharMap, a_Codepoint );
                return it != End( m_CharMap ) ? GlyphID{ it->second } : GlyphID{ 0 };
            }

            FontUnit GetAdvance( GlyphID a_Glyph ) const override
            {
                const u32 index = ToUnderlying( a_Glyph );
                if ( index >= Size( m_Glyphs ) )
                    return 0_fu;
                return FontUnit{ static_cast<f32>( m_Glyphs[index].Advance ) / static_cast<f32>( m_Cell[1] ) };
            }

            FontUnit GetKerning( GlyphID a_Left, GlyphID a_Right ) const override
            {
                if ( Empty( m_Kerning ) )
                    return 0_fu;

                const u64 key = ( static_cast<u64>( ToUnderlying( a_Left ) ) << 32 ) | ToUnderlying( a_Right );
                const auto it = Find( m_Kerning, key );
                return it != End( m_Kerning ) ? FontUnit{ static_cast<f32>( it->second ) / static_cast<f32>( m_Cell[1] ) } : 0_fu;
            }

            bool RasterizeGlyph( const GlyphRasterRequest& a_Request, GlyphBitmap& o_Bitmap ) override
            {
                o_Bitmap = {};
                const u32 index = ToUnderlying( a_Request.Glyph );
                if ( index >= Size( m_Glyphs ) )
                    return false;

                const Glyph& glyph = m_Glyphs[index];
                if ( glyph.Width == 0 || glyph.Height == 0 )
                    return true;

                m_Scratch = glyph.Pixels;
                u32 width = glyph.Width;

                if ( a_Request.SynthItalic )
                    PixelGlyphOps::Italicize( m_Scratch, width, glyph.Height, glyph.BearingY );
                if ( a_Request.SynthBold )
                    PixelGlyphOps::Embolden( m_Scratch, width, glyph.Height );

                o_Bitmap.Pixels  = Data( m_Scratch );
                o_Bitmap.Width   = width;
                o_Bitmap.Height  = glyph.Height;
                o_Bitmap.Bearing = Vec2f{ static_cast<f32>( glyph.BearingX ), static_cast<f32>( glyph.BearingY ) };
                return true;
            }

        private:
            Glyph ExtractGlyph( const SpriteSheetFontDesc& a_Desc, u32 a_CellX, u32 a_CellY, u32 a_Baseline ) const
            {
                const u32 cellW = a_Desc.CellSize[0];
                const u32 cellH = a_Desc.CellSize[1];

                const auto pixelAt = [&]( u32 x, u32 y ) -> Color
                {
                    return a_Desc.Pixels[( a_CellY + y ) * a_Desc.ImageSize[0] + ( a_CellX + x )];
                };

                u32 minX = cellW, maxX = 0, minY = cellH, maxY = 0;
                for ( u32 y = 0; y < cellH; ++y )
                {
                    for ( u32 x = 0; x < cellW; ++x )
                    {
                        if ( pixelAt( x, y )[3] >= a_Desc.AlphaThreshold )
                        {
                            minX = std::min( minX, x ); maxX = std::max( maxX, x );
                            minY = std::min( minY, y ); maxY = std::max( maxY, y );
                        }
                    }
                }

                Glyph glyph;
                const bool empty = minX > maxX;

                if ( a_Desc.Monospace )
                    glyph.Advance = cellW;
                else
                    glyph.Advance = empty ? 0 : static_cast<u32>( std::max<i32>( 0, static_cast<i32>( maxX - minX + 1 ) + a_Desc.LetterSpacing ) );

                if ( empty )
                    return glyph;

                // Monospace glyphs keep their position in the cell, proportional ones start at the pen.
                glyph.BearingX = a_Desc.Monospace ? static_cast<i32>( minX ) : 0;
                glyph.BearingY = static_cast<i32>( a_Baseline ) - static_cast<i32>( minY );
                glyph.Width    = maxX - minX + 1;
                glyph.Height   = maxY - minY + 1;

                Resize( glyph.Pixels, static_cast<size>( glyph.Width ) * glyph.Height );
                for ( u32 y = 0; y < glyph.Height; ++y )
                {
                    for ( u32 x = 0; x < glyph.Width; ++x )
                    {
                        Color c = pixelAt( minX + x, minY + y );
                        if ( !a_Desc.PreserveColor )
                            c = Color{ 255, 255, 255, c[3] };
                        glyph.Pixels[y * glyph.Width + x] = c;
                    }
                }

                return glyph;
            }

            static Glyph MakeNotDefGlyph( u32 a_CellW, u32 a_Baseline, u32 a_Advance )
            {
                Glyph glyph;
                glyph.Width    = std::max( 3u, std::min( a_CellW, a_Advance ) - 1 );
                glyph.Height   = std::max( 3u, a_Baseline );
                glyph.BearingY = static_cast<i32>( glyph.Height );
                glyph.Advance  = glyph.Width + 1;

                Resize( glyph.Pixels, static_cast<size>( glyph.Width ) * glyph.Height );
                for ( u32 y = 0; y < glyph.Height; ++y )
                {
                    for ( u32 x = 0; x < glyph.Width; ++x )
                    {
                        const bool edge = x == 0 || y == 0 || x == glyph.Width - 1 || y == glyph.Height - 1;
                        glyph.Pixels[y * glyph.Width + x] = edge ? Colors::White : Colors::Transparent;
                    }
                }
                return glyph;
            }

            String                 m_Name;
            FontFaceMetrics        m_Metrics;
            Vec2u                  m_Cell{ 0, 0 };
            u32                    m_Baseline{ 0 };
            Array<Glyph>           m_Glyphs;
            HashMap<codepoint, u32> m_CharMap;
            HashMap<u64, i32>      m_Kerning;
            Array<Color>           m_Scratch;
        };
    }

    Unique<IFontFace> MakeSpriteSheetFace( const SpriteSheetFontDesc& a_Desc )
    {
        auto face = MakeUnique<SpriteSheetFace>();
        if ( !face->Build( a_Desc ) )
            return nullptr;
        return face;
    }

} // namespace RatUI
