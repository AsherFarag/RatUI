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

    void DrawBatcher::EmitSlicedRect( Rect<Pixel> a_Rect, NineSlice a_Slice, Color a_Tint )
    {
        // TODO: Need cleaner and safer api here
		const TextureHandle& texture = std::get<SDFDrawData>( Back( m_Batches ).Data ).Texture;

        Optional<TextureInfo> texInfo = texture.QueryInfo();

        if ( !texInfo )
        {
			RATUI_ASSERT( false, "Failed to query texture info for nine-slice rect" );
            return;
        }

		const Vec2u texSize = texInfo->Size;

        const f32 rcpTexW = 1.f / static_cast<f32>( texSize[0] );
        const f32 rcpTexH = 1.f / static_cast<f32>( texSize[1] );

        const f32 rectW = a_Rect.Size[0].ToFloat();
        const f32 rectH = a_Rect.Size[1].ToFloat();

        // Apply user scaling first
        const f32 scaledLeft   = static_cast<f32>( a_Slice.Left )   * a_Slice.Scale[0];
        const f32 scaledRight  = static_cast<f32>( a_Slice.Right )  * a_Slice.Scale[0];
        const f32 scaledTop    = static_cast<f32>( a_Slice.Top )    * a_Slice.Scale[1];
        const f32 scaledBottom = static_cast<f32>( a_Slice.Bottom ) * a_Slice.Scale[1];

        // Scale corners down if they would overlap
        const f32 rawCornerW = scaledLeft + scaledRight;
        const f32 rawCornerH = scaledTop  + scaledBottom;

        const f32 fitScaleX = ( rawCornerW > rectW && rawCornerW > 0.f )
            ? rectW / rawCornerW
            : 1.f;

        const f32 fitScaleY = ( rawCornerH > rectH && rawCornerH > 0.f )
            ? rectH / rawCornerH
            : 1.f;

        const f32 dstLeft   = scaledLeft   * fitScaleX;
        const f32 dstRight  = scaledRight  * fitScaleX;
        const f32 dstTop    = scaledTop    * fitScaleY;
        const f32 dstBottom = scaledBottom * fitScaleY;

        // Destination X/Y split points
        const f32 x0 = a_Rect.Origin[0].ToFloat();
        const f32 x1 = x0 + dstLeft;
        const f32 x3 = x0 + rectW;
        const f32 x2 = x3 - dstRight;

        const f32 y0 = a_Rect.Origin[1].ToFloat();
        const f32 y1 = y0 + dstTop;
        const f32 y3 = y0 + rectH;
        const f32 y2 = y3 - dstBottom;

        // Source UV split points
        const f32 u0 = 0.f;
        const f32 u1 = static_cast<f32>( a_Slice.Left ) * rcpTexW;
        const f32 u2 = 1.f - static_cast<f32>( a_Slice.Right ) * rcpTexW;
        const f32 u3 = 1.f;

        const f32 v0 = 0.f;
        const f32 v1 = static_cast<f32>( a_Slice.Top ) * rcpTexH;
        const f32 v2 = 1.f - static_cast<f32>( a_Slice.Bottom ) * rcpTexH;
        const f32 v3 = 1.f;

        const auto EmitQuad = [&]( f32 dx0, f32 dy0, f32 dx1, f32 dy1,
                                   f32 su0, f32 sv0, f32 su1, f32 sv1 )
        {
            if ( dx1 - dx0 <= 0.f || dy1 - dy0 <= 0.f )
                return;

            const Vec2<Pixel> halfSize{ Pixel{ ( dx1 - dx0 ) * 0.5f }, 
                                        Pixel{ ( dy1 - dy0 ) * 0.5f } };

            const u32 vertexBase = ( static_cast<u32>( Size( m_Vertices ) )
                                   - Back( m_Batches ).VertexByteOffset ) / sizeof( SDFVertex );

			// TODO: Since we know how many vertices/indices we're going to emit, 
            // we could reserve them all at once before the loop instead of per quad.

            auto verts = ReserveVertices<SDFVertex>( 4 );

            verts[0] = { .Position = { Pixel{ dx0 }, Pixel{ dy0 } }, .LocalPos = { -halfSize[0], -halfSize[1] },
                         .UV = { su0, sv0 }, .FillColor = a_Tint, .BorderColor = Colors::Transparent,
						 .BorderThickness = 0_px, .HalfSize = halfSize, .CornerRadius = 0_px, .Softness = 0.f };

            verts[1] = { .Position = { Pixel{ dx1 }, Pixel{ dy0 } }, .LocalPos = {  halfSize[0], -halfSize[1] },
                         .UV = { su1, sv0 }, .FillColor = a_Tint, .BorderColor = Colors::Transparent,
                         .BorderThickness = 0_px, .HalfSize = halfSize, .CornerRadius = 0_px, .Softness = 0.f };

            verts[2] = { .Position = { Pixel{ dx0 }, Pixel{ dy1 } }, .LocalPos = { -halfSize[0],  halfSize[1] },
                         .UV = { su0, sv1 }, .FillColor = a_Tint, .BorderColor = Colors::Transparent,
                         .BorderThickness = 0_px, .HalfSize = halfSize, .CornerRadius = 0_px, .Softness = 0.f };

            verts[3] = { .Position = { Pixel{ dx1 }, Pixel{ dy1 } }, .LocalPos = {  halfSize[0],  halfSize[1] },
                         .UV = { su1, sv1 }, .FillColor = a_Tint, .BorderColor = Colors::Transparent,
                         .BorderThickness = 0_px, .HalfSize = halfSize, .CornerRadius = 0_px, .Softness = 0.f };

            auto idx = ReserveIndices( 6 );
            idx[0] = vertexBase + 0; idx[1] = vertexBase + 1; idx[2] = vertexBase + 2;
            idx[3] = vertexBase + 1; idx[4] = vertexBase + 3; idx[5] = vertexBase + 2;
            AddIndicesToCurrentBatch( 6 );
        };

        // Row-major: TL, T, TR, L, C, R, BL, B, BR
        EmitQuad( x0, y0, x1, y1, u0, v0, u1, v1 );
        EmitQuad( x1, y0, x2, y1, u1, v0, u2, v1 );
        EmitQuad( x2, y0, x3, y1, u2, v0, u3, v1 );

        EmitQuad( x0, y1, x1, y2, u0, v1, u1, v2 );
        EmitQuad( x1, y1, x2, y2, u1, v1, u2, v2 );
        EmitQuad( x2, y1, x3, y2, u2, v1, u3, v2 );

        EmitQuad( x0, y2, x1, y3, u0, v2, u1, v3 );
        EmitQuad( x1, y2, x2, y3, u1, v2, u2, v3 );
        EmitQuad( x2, y2, x3, y3, u2, v2, u3, v3 );

        TryFlatten();
    }

    void DrawBatcher::EmitText( const ShapedText& a_Text, const TextRenderStyle& a_Style, Rect<Pixel> a_LayoutRect, GlyphAtlas& a_Atlas, f32 a_DpiScale )
    {
        if ( Empty( a_Text.Glyphs ) || Empty( a_Text.Lines ) )
            return;
    
        const f32 atlasW    = static_cast<f32>( a_Atlas.GetConfig().AtlasWidth );
        const f32 atlasH    = static_cast<f32>( a_Atlas.GetConfig().AtlasHeight );
        const f32 rcpAtlasW = atlasW > 0.f ? 1.f / atlasW : 0.f;
        const f32 rcpAtlasH = atlasH > 0.f ? 1.f / atlasH : 0.f;
    
        const Unit  fontSize   = a_Text.FontSize;
        const Pixel textHeight = ToPixel( a_Text.TotalHeight, a_DpiScale );
        const Pixel ascender   = ToPixel( a_Text.Ascender, a_DpiScale );
    
        const f32 baseSize   = a_Atlas.GetConfig().BaseSize.ToFloat();
        const f32 rcpBase    = baseSize > 0.f ? 1.f / baseSize : 0.f;
        const f32 fontSizePx = ToPixel( fontSize, a_DpiScale ).ToFloat();
    
        const bool isSingleLine   = a_Text.LineCount() == 1;
        const bool overflowsY     = textHeight > a_LayoutRect.Size[1];
        const bool fadeHorizontal = isSingleLine;
        const bool fadeVertical   = !isSingleLine && overflowsY;
    
        // Horizontal fade setup (single-line only)
        const Pixel layoutRight = a_LayoutRect.Right();
        const f32   fadePct     = fadeHorizontal ? std::clamp( a_Style.FadePercentage, 0.0f, 1.0f ) : 0.f;
        const Pixel fadeStartX  = layoutRight - a_LayoutRect.Size[0] * fadePct;
        const Pixel fadeEndX    = layoutRight;
    
        // Vertical fade setup (multi-line overflow only)
        const Pixel layoutBottom  = a_LayoutRect.Bottom();
        const f32   fadePctV      = fadeVertical ? std::clamp( a_Style.FadePercentage, 0.0f, 1.0f ) : 0.f;
        const Pixel fadeStartY    = layoutBottom - a_LayoutRect.Size[1] * fadePctV;
        const Pixel fadeEndY      = layoutBottom;
    
        auto computeFadeAlpha = [&]( Pixel x, Pixel y ) -> u8
        {
            f32 alpha = static_cast<f32>( a_Style.FillColor[3] );
    
            if ( fadeHorizontal && fadePct > 0.f && x > fadeStartX )
            {
                if ( x >= fadeEndX )
                    return 0;
                const f32 t = ( x - fadeStartX ).ToFloat() / ( fadeEndX - fadeStartX ).ToFloat();
                alpha = ( 1.0f - t ) * alpha;
            }
    
            if ( fadeVertical && fadePctV > 0.f && y > fadeStartY )
            {
                if ( y >= fadeEndY )
                    return 0;
                const f32 t = ( y - fadeStartY ).ToFloat() / ( fadeEndY - fadeStartY ).ToFloat();
                alpha = ( 1.0f - t ) * alpha;
            }
    
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
    
                Optional<GlyphMetrics> gr = a_Atlas.GetOrRasterizeGlyph( a_Text.Font, sg.GlyphIndex );
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
    
                // Sample fade at all four corners to correctly interpolate across the glyph quad
                const f32 opacityTL = computeFadeAlpha( gx,      gy      ) / 255.0f;
                const f32 opacityTR = computeFadeAlpha( gx + gw, gy      ) / 255.0f;
                const f32 opacityBL = computeFadeAlpha( gx,      gy + gh ) / 255.0f;
                const f32 opacityBR = computeFadeAlpha( gx + gw, gy + gh ) / 255.0f;
    
                auto verts = ReserveVertices<TextVertex>( 4 );
                verts[0] = TextVertex{ Vec2<Pixel>{ gx,      gy      }, opacityTL, Vec2f{ u0, v0 } };
                verts[1] = TextVertex{ Vec2<Pixel>{ gx + gw, gy      }, opacityTR, Vec2f{ u1, v0 } };
                verts[2] = TextVertex{ Vec2<Pixel>{ gx,      gy + gh }, opacityBL, Vec2f{ u0, v1 } };
                verts[3] = TextVertex{ Vec2<Pixel>{ gx + gw, gy + gh }, opacityBR, Vec2f{ u1, v1 } };
    
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
