#include <RatUI/Backends/FreeType/TextMetrics.h>

#include <algorithm>
#include <cmath>

namespace RatUI::FreeType
{
    Optional<PreparedText> TextMetrics::Prepare( StringView a_Text, const TextLayoutStyle& a_Style )
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

        return TextLayout::Prepare( text, a_Style.Wrap, [font, &a_Style]( StringView sv )
                                    { return TextUtil::MeasureLineWidth( *font, sv, a_Style ); } );
    }

    Optional<ShapedText> TextMetrics::Shape( const PreparedText& a_Prepared, const TextLayoutStyle& a_Style, Vec2<Unit> a_MaxSize )
    {
        if ( a_MaxSize[0] <= 0_u || a_MaxSize[1] <= 0_u )
            return NullOpt;

        if ( !m_FontCache || !a_Style.Font.IsValid() || Empty( a_Prepared.Segments ) )
            return NullOpt;

        Font* font = m_FontCache->GetOrLoadFont( a_Style.Font );
        if ( !font )
            return NullOpt;

        FT_Face face = font->GetFace();
        const f32 unitsPerEM = static_cast<f32>( face->units_per_EM );

        ShapedText result;
        result.Font = a_Style.Font;
        result.FontSize = a_Style.Size;
        result.LineHeight = GetLineHeight( face, a_Style ) + a_Style.LineSpacing;
        result.Ascender = Unit{ ( static_cast<f32>( face->ascender ) / unitsPerEM ) * a_Style.Size.ToFloat() };
        result.Descender = Unit{ ( static_cast<f32>( face->descender ) / unitsPerEM ) * a_Style.Size.ToFloat() };
        result.UnderlinePosition = Unit{ -( static_cast<f32>( face->underline_position ) / unitsPerEM ) * a_Style.Size.ToFloat() };
        result.UnderlineThickness = Unit{ std::max( 1.f, ( static_cast<f32>( face->underline_thickness ) / unitsPerEM ) * a_Style.Size.ToFloat() ) };

        const Unit maxWidth = a_MaxSize[0];
        const u32 maxLines = a_Style.MaxLines;

        bool exceededMaxLines = false;
        if ( maxLines > 0 )
        {
            u32 totalLines = 0;
            TextLayout::WalkLines( a_Prepared, maxWidth, maxLines + 1u, [&]( u32, u32, Unit )
                                   { ++totalLines; } );
            exceededMaxLines = ( totalLines > maxLines );
        }

        const bool ellipsis = ( a_Style.Overflow == ETextOverflow::Ellipsis );
        const bool hasWidthConstraint = ( maxWidth < Limits<Unit>::max() );
        const StringView norm = a_Prepared.NormalizedText;

        Array<ShapedGlyph> lineGlyphs;
        Array<ShapedGlyph> ellipsisGlyphs;
        Unit ellipsisWidth = 0_u;

        if ( ellipsis )
        {
            constexpr StringView c_Ellipsis = "...";
            ellipsisWidth = ToUnit( ShapeLine( *font, c_Ellipsis, a_Style, ellipsisGlyphs ), a_Style.Size );
        }

        TextLayout::WalkLines( a_Prepared, maxWidth, maxLines, [&]( u32 lineStartSeg, u32 lineEndSeg, Unit paintWidth )
                               {
            const auto& segs = a_Prepared.Segments;
            u32 end = lineEndSeg;
            while ( end > lineStartSeg && segs[end - 1].Kind != ESegmentKind::Text )
                --end;

            StringView lineText{};
            if ( end > lineStartSeg )
            {
                const u32 byteStart = segs[lineStartSeg].StartByte;
                const u32 byteEnd = segs[end - 1].StartByte + segs[end - 1].ByteLength;
                lineText = StringView{ Data( norm ) + byteStart, byteEnd - byteStart };
            }

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
                    u32 keepCount = 0;

                    for ( u32 i = 0; i < Size( lineGlyphs ); ++i )
                    {
                        const Unit adv = ToUnit( lineGlyphs[i].XAdvance, a_Style.Size );

                        if ( accumulated + adv > prefixBudget )
                            break;

                        accumulated += adv;
                        ++keepCount;
                    }

                    Resize( lineGlyphs, keepCount );
                    Insert( lineGlyphs, End( lineGlyphs ), Begin( ellipsisGlyphs ), End( ellipsisGlyphs ) );

                    paintWidth = accumulated + ellipsisWidth;
                }
                else
                {
                    Clear( lineGlyphs );
                    paintWidth = 0_u;
                }
            }

            const u32 glyphStart = static_cast<u32>( Size( result.Glyphs ) );
            Insert( result.Glyphs, End( result.Glyphs ), Begin( lineGlyphs ), End( lineGlyphs ) );
            const u32 glyphEnd = static_cast<u32>( Size( result.Glyphs ) );

            PushBack( result.Lines, ShapedLine{
                                       .Start = glyphStart,
                                       .End = glyphEnd,
                                       .Width = paintWidth,
                                   } );

            result.MaxWidth = Unit{ std::max( result.MaxWidth.ToFloat(), paintWidth.ToFloat() ) }; } );

        if ( Empty( result.Lines ) )
            return NullOpt;

        const f32 baseSize = static_cast<f32>( c_MsdfPxRange * 2 );
        const f32 sdfPadEm = static_cast<f32>( c_MsdfPxRange ) / baseSize;
        const f32 sdfPadDisplay = sdfPadEm * a_Style.Size.ToFloat();

        result.Ascender += Unit{ sdfPadDisplay };
        result.TotalHeight = result.LineHeight * result.LineCount() + Unit{ std::abs( result.Descender.ToFloat() ) + sdfPadDisplay };

        return result;
    }

    bool TextMetrics::RasterizeGlyph( FontHandle a_Font, GlyphID a_GlyphID, u32 a_SdfPixelSize,
                                      const Color*& o_Pixels, u32& o_Width, u32& o_Height,
                                      Vec2<FontUnit>& o_Bearing, FontUnit& o_XAdvance )
    {
        o_Pixels = nullptr;
        o_Width = 0;
        o_Height = 0;
        o_Bearing = Vec2<FontUnit>{ 0_fu, 0_fu };
        o_XAdvance = 0_fu;

        if ( !m_FontCache )
            return false;

        Font* font = m_FontCache->GetOrLoadFont( a_Font );
        if ( !font || !font->IsValid() )
            return false;

        msdfgen::FontHandle* msdfFont = font->GetMsdfFont();

        msdfgen::Shape shape;
        f64 advance = 0.0;

        if ( !msdfgen::loadGlyph( shape, msdfFont, msdfgen::GlyphIndex{ ToUnderlying( a_GlyphID ) }, msdfgen::FONT_SCALING_EM_NORMALIZED, &advance ) )
            return false;

        o_XAdvance = FontUnit{ static_cast<f32>( advance ) };

        if ( shape.contours.empty() )
            return true;

        msdfgen::edgeColoringSimple( shape, 3.0 );

        f64 l = 0.0, b = 0.0, r = 0.0, t = 0.0;
        shape.bound( l, b, r, t );

        const f64 scale = static_cast<f64>( a_SdfPixelSize );
        const i32 padding = static_cast<i32>( std::ceil( c_MsdfPxRange ) );

        const i32 w = static_cast<i32>( std::ceil( ( r - l ) * scale ) ) + 2 * padding;
        const i32 h = static_cast<i32>( std::ceil( ( t - b ) * scale ) ) + 2 * padding;

        if ( w <= 0 || h <= 0 )
            return true;

        const f64 padEm = static_cast<f64>( padding ) / scale;
        const msdfgen::Projection projection(
            msdfgen::Vector2( scale, scale ),
            msdfgen::Vector2( padEm - l, padEm - b ) );

        msdfgen::Bitmap<f32, 4> mtsdf( w, h );
        msdfgen::generateMTSDF( mtsdf, shape, projection, c_MsdfPxRange / scale );

        Resize( m_RasterBuffer, static_cast<size>( w ) * h );

        auto toU8 = +[]( f32 v ) -> u8
        {
            return static_cast<u8>( std::clamp( static_cast<i32>( v * 255.0f + 0.5f ), 0, 255 ) );
        };

        for ( i32 y = 0; y < h; ++y )
        {
            for ( i32 x = 0; x < w; ++x )
            {
                const size idx = static_cast<size>( y ) * w + x;
                auto px = mtsdf( x, h - 1 - y );

                RawAt( m_RasterBuffer, idx ) = Color{
                    toU8( px[0] ),
                    toU8( px[1] ),
                    toU8( px[2] ),
                    toU8( px[3] ),
                };
            }
        }

        o_Pixels = Data( m_RasterBuffer );
        o_Width = static_cast<u32>( w );
        o_Height = static_cast<u32>( h );

        o_Bearing = Vec2<FontUnit>{
            static_cast<FontUnit>( l - padEm ),
            static_cast<FontUnit>( t + padEm ),
        };

        return true;
    }

} // namespace RatUI::FreeType
