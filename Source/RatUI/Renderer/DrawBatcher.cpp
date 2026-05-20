#include <RatUI/Renderer/DrawBatcher.h>

#include <algorithm>

namespace RatUI
{
    MSDFTextDrawData MSDFTextDrawData::From( TextureHandle a_FontAtlas, const TextRenderStyle& a_Style, f32 a_MSDFScale )
    {
        MSDFTextDrawData result{
            .FontAtlas = std::move( a_FontAtlas ),
            .Scale = a_MSDFScale,
        };

        result.FillColor = a_Style.FillColor;
        result.FillSoftness = a_Style.FillSoftness;
        result.FillThreshold = a_Style.FillThreshold;

        result.OutlineEnable = a_Style.Outline;
        if ( a_Style.Outline )
        {
            result.OutlineColor = a_Style.OutlineColor;
            result.OutlineWidth = a_Style.OutlineWidth;
            result.OutlineSoftness = a_Style.OutlineSoftness;
        }

        result.ShadowEnable = a_Style.Shadow;
        if ( a_Style.Shadow )
        {
            result.ShadowColor = a_Style.ShadowColor;
            result.ShadowSoftness = a_Style.ShadowSoftness;
            result.ShadowSpread = a_Style.ShadowSpread;

            result.ShadowOffsetUV = Vec2f{
                a_Style.ShadowOffset[0],
                a_Style.ShadowOffset[1],
            };
        }

        result.GlowEnable = a_Style.Glow;
        if ( a_Style.Glow )
        {
            result.GlowColor = a_Style.GlowColor;
            result.GlowSpread = a_Style.GlowSpread;
            result.GlowPower = a_Style.GlowPower;
        }

        return result;
    }

    bool MSDFTextDrawData::CanFlattenWith( const MSDFTextDrawData& a_Other ) const
    {
        if ( FontAtlas != a_Other.FontAtlas || PixelRange != a_Other.PixelRange || Scale != a_Other.Scale )
            return false;

        if ( OutlineEnable != a_Other.OutlineEnable )
            return false;
        if ( ShadowEnable != a_Other.ShadowEnable )
            return false;
        if ( GlowEnable != a_Other.GlowEnable )
            return false;
        if ( InnerGlowEnable != a_Other.InnerGlowEnable )
            return false;

        if ( FillColor != a_Other.FillColor )
            return false;
        if ( FillSoftness != a_Other.FillSoftness )
            return false;
        if ( FillThreshold != a_Other.FillThreshold )
            return false;

        if ( OutlineEnable )
        {
            if ( OutlineColor != a_Other.OutlineColor )
                return false;
            if ( OutlineWidth != a_Other.OutlineWidth )
                return false;
            if ( OutlineSoftness != a_Other.OutlineSoftness )
                return false;
        }

        if ( ShadowEnable )
        {
            if ( ShadowColor != a_Other.ShadowColor )
                return false;
            if ( ShadowOffsetUV != a_Other.ShadowOffsetUV )
                return false;
            if ( ShadowSoftness != a_Other.ShadowSoftness )
                return false;
            if ( ShadowSpread != a_Other.ShadowSpread )
                return false;
        }

        if ( GlowEnable )
        {
            if ( GlowColor != a_Other.GlowColor )
                return false;
            if ( GlowSpread != a_Other.GlowSpread )
                return false;
            if ( GlowPower != a_Other.GlowPower )
                return false;
        }

        return true;
    }

    bool DrawBatch::CanFlattenWith( const DrawBatch& a_Other ) const
    {
        if ( ClipRect != a_Other.ClipRect || Transform != a_Other.Transform )
            return false;

        return std::visit( [&]( const auto& a_Data )
                           {
            using T = std::decay_t<decltype( a_Data )>;
            if ( !std::holds_alternative<T>( a_Other.Data ) )
                return false;

            const T& otherData = std::get<T>( a_Other.Data );
            return a_Data.CanFlattenWith( otherData ); },
                           Data );
    }

    void DrawBatcher::Clear()
    {
        ::RatUI::Clear( m_Vertices );
        ::RatUI::Clear( m_Indices );
        ::RatUI::Clear( m_Batches );
    }

    DrawBatch& DrawBatcher::EnsureSDFBatch( const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, TextureHandle a_Texture )
    {
        return EnsureBatch( a_ClipRect, a_Transform, SDFDrawData{ .Texture = std::move( a_Texture ) } );
    }

    DrawBatch& DrawBatcher::EnsureMSDFTextBatch( const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, const MSDFTextDrawData& a_Data )
    {
        return EnsureBatch( a_ClipRect, a_Transform, a_Data );
    }

    void DrawBatcher::EmitRect( Rect<Pixel> a_Rect, Color a_FillColor, Pixel a_BorderThickness, Color a_BorderColor, Vec4<Pixel> a_Rounding )
    {
        const Pixel w = a_Rect.Size[0];
        const Pixel h = a_Rect.Size[1];

        const Pixel halfW = w * 0.5f;
        const Pixel halfH = h * 0.5f;

        const Pixel cx = a_Rect.Origin[0] + halfW;
        const Pixel cy = a_Rect.Origin[1] + halfH;

        constexpr Pixel aaPad = 1.5_px;
        const Pixel border = a_BorderThickness > 0_px ? a_BorderThickness : 0_px;

        const Pixel outerHalfW = halfW + border + aaPad;
        const Pixel outerHalfH = halfH + border + aaPad;

        const u32 vertexBase = ( static_cast<u32>( Size( m_Vertices ) ) - Back( m_Batches ).VertexByteOffset ) / sizeof( SDFVertex );
        auto verts = ReserveVertices<SDFVertex>( 4 );

        Pixel r0 = a_Rounding[0] > 0_px ? a_Rounding[0] : 0_px;
        Pixel r1 = a_Rounding[1] > 0_px ? a_Rounding[1] : 0_px;
        Pixel r2 = a_Rounding[2] > 0_px ? a_Rounding[2] : 0_px;
        Pixel r3 = a_Rounding[3] > 0_px ? a_Rounding[3] : 0_px;

        f32 scale = 1.0f;

        const Pixel top = r0 + r1;
        const Pixel bottom = r2 + r3;
        const Pixel left = r0 + r2;
        const Pixel right = r1 + r3;

        if ( top > w )
            scale = std::min( scale, f32( w / top ) );
        if ( bottom > w )
            scale = std::min( scale, f32( w / bottom ) );
        if ( left > h )
            scale = std::min( scale, f32( h / left ) );
        if ( right > h )
            scale = std::min( scale, f32( h / right ) );

        if ( scale < 1.0f )
        {
            r0 *= scale;
            r1 *= scale;
            r2 *= scale;
            r3 *= scale;
        }

        verts[0] = {
            .Position = { cx - outerHalfW, cy - outerHalfH },
            .LocalPos = { -outerHalfW, -outerHalfH },
            .UV = { 0.f, 0.f },
            .FillColor = a_FillColor,
            .BorderColor = a_BorderColor,
            .BorderThickness = border,
            .HalfSize = { halfW, halfH },
            .CornerRadius = r0,
        };

        verts[1] = {
            .Position = { cx + outerHalfW, cy - outerHalfH },
            .LocalPos = { outerHalfW, -outerHalfH },
            .UV = { 1.f, 0.f },
            .FillColor = a_FillColor,
            .BorderColor = a_BorderColor,
            .BorderThickness = border,
            .HalfSize = { halfW, halfH },
            .CornerRadius = r1,
        };

        verts[2] = {
            .Position = { cx - outerHalfW, cy + outerHalfH },
            .LocalPos = { -outerHalfW, outerHalfH },
            .UV = { 0.f, 1.f },
            .FillColor = a_FillColor,
            .BorderColor = a_BorderColor,
            .BorderThickness = border,
            .HalfSize = { halfW, halfH },
            .CornerRadius = r2,
        };

        verts[3] = {
            .Position = { cx + outerHalfW, cy + outerHalfH },
            .LocalPos = { outerHalfW, outerHalfH },
            .UV = { 1.f, 1.f },
            .FillColor = a_FillColor,
            .BorderColor = a_BorderColor,
            .BorderThickness = border,
            .HalfSize = { halfW, halfH },
            .CornerRadius = r3,
        };

        auto idx = ReserveIndices( 6 );
        idx[0] = vertexBase + 0;
        idx[1] = vertexBase + 1;
        idx[2] = vertexBase + 2;
        idx[3] = vertexBase + 1;
        idx[4] = vertexBase + 3;
        idx[5] = vertexBase + 2;

        AddIndicesToCurrentBatch( 6 );
        TryFlatten();
    }

    void DrawBatcher::EmitText( const ShapedText& a_Text, const TextRenderStyle& a_Style, Rect<Pixel> a_LayoutRect, GlyphAtlas& a_Atlas, f32 a_DpiScale )
    {
        if ( Empty( a_Text.Glyphs ) || Empty( a_Text.Lines ) )
            return;

        const f32 atlasW = static_cast<f32>( a_Atlas.GetConfig().AtlasWidth );
        const f32 atlasH = static_cast<f32>( a_Atlas.GetConfig().AtlasHeight );
        const f32 rcpAtlasW = atlasW > 0.f ? 1.f / atlasW : 0.f;
        const f32 rcpAtlasH = atlasH > 0.f ? 1.f / atlasH : 0.f;

        const Unit fontSize = a_Text.FontSize;
        const Pixel textHeight = ToPixel( a_Text.TotalHeight, a_DpiScale );
        const Pixel ascender = ToPixel( a_Text.Ascender, a_DpiScale );

        const f32 baseSize = a_Atlas.GetConfig().BaseSize.ToFloat();
        const f32 rcpBase = baseSize > 0.f ? 1.f / baseSize : 0.f;
        const f32 fontSizePx = ToPixel( fontSize, a_DpiScale ).ToFloat();

        const Pixel layoutRight = a_LayoutRect.Origin[0] + a_LayoutRect.Size[0];
        const f32 fadePct = std::clamp( a_Style.FadePercentage, 0.0f, 1.0f );
        const Pixel fadeWidth = a_LayoutRect.Size[0] * fadePct;
        const Pixel fadeStartX = layoutRight - fadeWidth;
        const Pixel fadeEndX = layoutRight;

        auto computeFadeAlpha = [&]( Pixel x ) -> u8
        {
            if ( fadePct <= 0.f || x <= fadeStartX )
                return a_Style.FillColor[3];
            if ( x >= fadeEndX )
                return 0;

            const f32 t = ( x - fadeStartX ).ToFloat() / ( fadeEndX - fadeStartX ).ToFloat();
            const f32 alpha = ( 1.0f - t ) * static_cast<f32>( a_Style.FillColor[3] );
            return static_cast<u8>( std::clamp( alpha, 0.0f, 255.0f ) );
        };

        Pixel baselineY = a_LayoutRect.Origin[1];
        switch ( a_Style.Baseline )
        {
            case ETextBaseline::Top:
            case ETextBaseline::Hanging:
                baselineY += ascender;
                break;
            case ETextBaseline::Middle:
                baselineY += ( a_LayoutRect.Size[1] - textHeight ) * 0.5f + ascender;
                break;
            case ETextBaseline::Bottom:
                baselineY += a_LayoutRect.Size[1] - textHeight + ascender;
                break;
            case ETextBaseline::Alphabetic:
            default:
                break;
        }

        Pixel penY = baselineY;

        for ( u32 lineIdx = 0; lineIdx < a_Text.LineCount(); ++lineIdx )
        {
            const ShapedLine& line = a_Text.Lines[lineIdx];

            Pixel lineX = a_LayoutRect.Origin[0];
            switch ( a_Style.Align )
            {
                case ETextAlign::Center:
                    lineX += ( a_LayoutRect.Size[0] - ToPixel( line.Width, a_DpiScale ) ) * 0.5f;
                    break;
                case ETextAlign::Right:
                    lineX += a_LayoutRect.Size[0] - ToPixel( line.Width, a_DpiScale );
                    break;
                default:
                    break;
            }

            Pixel penX = lineX;

            u32 vertexBase = ( static_cast<u32>( Size( m_Vertices ) ) - Back( m_Batches ).VertexByteOffset ) / sizeof( TextVertex );

            for ( u32 g = line.Start; g < line.End; ++g )
            {
                const ShapedGlyph& sg = a_Text.Glyphs[g];

                Optional<GlyphMetrics> gr = a_Atlas.GetOrRasterizeGlyph( a_Text.Font, sg.Codepoint );
                if ( !gr || gr->AtlasRect.Size[0] == 0 || gr->AtlasRect.Size[1] == 0 )
                {
                    penX += ToPixel( sg.XAdvance, fontSize, a_DpiScale );
                    continue;
                }

                const Pixel gx = penX + ToPixel( sg.XOffset + gr->Bearing[0], fontSize, a_DpiScale );
                const Pixel gy = penY + ToPixel( sg.YOffset - gr->Bearing[1], fontSize, a_DpiScale );
                const Pixel gw = static_cast<Pixel>( gr->AtlasRect.Size[0] ) * rcpBase * fontSizePx;
                const Pixel gh = static_cast<Pixel>( gr->AtlasRect.Size[1] ) * rcpBase * fontSizePx;

                const f32 u0 = static_cast<f32>( gr->AtlasRect.Origin[0] ) * rcpAtlasW;
                const f32 v0 = static_cast<f32>( gr->AtlasRect.Origin[1] ) * rcpAtlasH;
                const f32 u1 = static_cast<f32>( gr->AtlasRect.Origin[0] + gr->AtlasRect.Size[0] ) * rcpAtlasW;
                const f32 v1 = static_cast<f32>( gr->AtlasRect.Origin[1] + gr->AtlasRect.Size[1] ) * rcpAtlasH;

                const f32 opacityA = computeFadeAlpha( gx ) / 255.0f;
                const f32 opacityB = computeFadeAlpha( gx + gw ) / 255.0f;

                auto verts = ReserveVertices<TextVertex>( 4 );
                verts[0] = TextVertex{ Vec2<Pixel>{ gx, gy }, opacityA, Vec2f{ u0, v0 } };
                verts[1] = TextVertex{ Vec2<Pixel>{ gx + gw, gy }, opacityB, Vec2f{ u1, v0 } };
                verts[2] = TextVertex{ Vec2<Pixel>{ gx, gy + gh }, opacityA, Vec2f{ u0, v1 } };
                verts[3] = TextVertex{ Vec2<Pixel>{ gx + gw, gy + gh }, opacityB, Vec2f{ u1, v1 } };

                auto idx = ReserveIndices( 6 );
                idx[0] = vertexBase + 0;
                idx[1] = vertexBase + 1;
                idx[2] = vertexBase + 2;
                idx[3] = vertexBase + 1;
                idx[4] = vertexBase + 3;
                idx[5] = vertexBase + 2;
                AddIndicesToCurrentBatch( 6 );

                penX += ToPixel( sg.XAdvance, fontSize, a_DpiScale );
                vertexBase += 4;
            }

            penY += ToPixel( a_Text.LineHeight, a_DpiScale );
        }

        TryFlatten();
    }

    void DrawBatcher::TryFlatten()
    {
        if ( Size( m_Batches ) < 2 )
            return;

        const DrawBatch& consumable = Back( m_Batches );
        DrawBatch& consumer = m_Batches[Size( m_Batches ) - 2];

        if ( consumable.CanFlattenWith( consumer ) )
        {
            consumer.IndexCount += consumable.IndexCount;
            PopBack( m_Batches );
        }
    }

    Span<u16> DrawBatcher::ReserveIndices( u32 a_Count )
    {
        const u32 offset = static_cast<u32>( Size( m_Indices ) );
        Resize( m_Indices, offset + a_Count );
        return Span<u16>{ Data( m_Indices ) + offset, a_Count };
    }

    void DrawBatcher::AddIndicesToCurrentBatch( u32 a_Count )
    {
        RATUI_ASSERT( !Empty( m_Batches ), "Emit call requires an active batch. Call an Ensure*Batch method first." );
        Back( m_Batches ).IndexCount += a_Count;
    }

} // namespace RatUI
