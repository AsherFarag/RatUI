#include <RatUI/Renderer/DrawBatcher.h>

#include <algorithm>
#include <cmath>

namespace RatUI
{
    MSDFTextDrawData MSDFTextDrawData::From( TextureHandle a_Page, const TextRenderStyle& a_Style, f32 a_PxRange, u16 a_PageSize )
    {
        MSDFTextDrawData result{
            .FontAtlas  = std::move( a_Page ),
            .PixelRange = a_PxRange,
        };

        result.FillSoftness  = a_Style.FillSoftness;
        result.FillThreshold = a_Style.FillThreshold;

        result.OutlineEnable = a_Style.Outline;
        if ( a_Style.Outline )
        {
            result.OutlineColor    = a_Style.OutlineColor;
            result.OutlineWidth    = a_Style.OutlineWidth;
            result.OutlineSoftness = a_Style.OutlineSoftness;
        }

        result.ShadowEnable = a_Style.Shadow;
        if ( a_Style.Shadow )
        {
            result.ShadowColor    = a_Style.ShadowColor;
            result.ShadowSoftness = a_Style.ShadowSoftness;
            result.ShadowSpread   = a_Style.ShadowSpread;

            // ShadowOffset is in atlas pixels; the shader works in UV.
            const f32 rcpPage = a_PageSize > 0 ? 1.f / static_cast<f32>( a_PageSize ) : 0.f;
            result.ShadowOffsetUV = Vec2f{ a_Style.ShadowOffset[0] * rcpPage, a_Style.ShadowOffset[1] * rcpPage };
        }

        result.GlowEnable = a_Style.Glow;
        if ( a_Style.Glow )
        {
            result.GlowColor  = a_Style.GlowColor;
            result.GlowSpread = a_Style.GlowSpread;
            result.GlowPower  = a_Style.GlowPower;
        }

        return result;
    }

    bool MSDFTextDrawData::CanFlattenWith( const MSDFTextDrawData& a_Other ) const
    {
        if ( FontAtlas != a_Other.FontAtlas || PixelRange != a_Other.PixelRange )
            return false;

        if ( OutlineEnable != a_Other.OutlineEnable )
            return false;
        if ( ShadowEnable != a_Other.ShadowEnable )
            return false;
        if ( GlowEnable != a_Other.GlowEnable )
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

    DrawBatch& DrawBatcher::EnsureSDFBatch( const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, TextureView a_Texture )
    {
        return EnsureBatch( a_ClipRect, a_Transform, SDFDrawData{ .Texture = std::move( a_Texture ) } );
    }

    void DrawBatcher::EmitRect( Rect<Pixel> a_Rect, Color a_FillColor, Pixel a_BorderThickness, Color a_BorderColor, Vec4<Pixel> a_Rounding, Rect<f32> a_UVRect )
    {
        const auto mapU = [&]( f32 a_U ) { return a_UVRect.Origin[0] + a_U * a_UVRect.Size[0]; };
        const auto mapV = [&]( f32 a_V ) { return a_UVRect.Origin[1] + a_V * a_UVRect.Size[1]; };

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
            .UV = { mapU( 0.f ), mapV( 0.f ) },
            .FillColor = a_FillColor,
            .BorderColor = a_BorderColor,
            .BorderThickness = border,
            .HalfSize = { halfW, halfH },
            .CornerRadius = r0,
        };

        verts[1] = {
            .Position = { cx + outerHalfW, cy - outerHalfH },
            .LocalPos = { outerHalfW, -outerHalfH },
            .UV = { mapU( 1.f ), mapV( 0.f ) },
            .FillColor = a_FillColor,
            .BorderColor = a_BorderColor,
            .BorderThickness = border,
            .HalfSize = { halfW, halfH },
            .CornerRadius = r1,
        };

        verts[2] = {
            .Position = { cx - outerHalfW, cy + outerHalfH },
            .LocalPos = { -outerHalfW, outerHalfH },
            .UV = { mapU( 0.f ), mapV( 1.f ) },
            .FillColor = a_FillColor,
            .BorderColor = a_BorderColor,
            .BorderThickness = border,
            .HalfSize = { halfW, halfH },
            .CornerRadius = r2,
        };

        verts[3] = {
            .Position = { cx + outerHalfW, cy + outerHalfH },
            .LocalPos = { outerHalfW, outerHalfH },
            .UV = { mapU( 1.f ), mapV( 1.f ) },
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

    void DrawBatcher::EmitSlicedRect( Rect<Pixel> a_Rect, NineSlice a_Slice, Vec2u a_SliceSize, Color a_Tint, Rect<f32> a_UVRect )
    {
        if ( a_SliceSize[0] == 0 || a_SliceSize[1] == 0 )
            return; // Avoid division by zero and invalid UV mapping

        const f32 rcpTexW = 1.f / static_cast<f32>( a_SliceSize[0] );
        const f32 rcpTexH = 1.f / static_cast<f32>( a_SliceSize[1] );

        const auto mapU = [&]( f32 a_U ) { return a_UVRect.Origin[0] + a_U * a_UVRect.Size[0]; };
        const auto mapV = [&]( f32 a_V ) { return a_UVRect.Origin[1] + a_V * a_UVRect.Size[1]; };

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

        // Source UV split points, in local [0, 1] slice space, then mapped into the
        // TextureView's UV sub-rect so slices sourced from a texture atlas sample correctly.
        const f32 u0 = mapU( 0.f );
        const f32 u1 = mapU( static_cast<f32>( a_Slice.Left ) * rcpTexW );
        const f32 u2 = mapU( 1.f - static_cast<f32>( a_Slice.Right ) * rcpTexW );
        const f32 u3 = mapU( 1.f );

        const f32 v0 = mapV( 0.f );
        const f32 v1 = mapV( static_cast<f32>( a_Slice.Top ) * rcpTexH );
        const f32 v2 = mapV( 1.f - static_cast<f32>( a_Slice.Bottom ) * rcpTexH );
        const f32 v3 = mapV( 1.f );

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

    // =========================================================================
    // Text
    // =========================================================================

    namespace
    {
        /** @brief Screen pixels per native pixel: the largest whole number that fits, so fractional DPIs never blur. */
        f32 PixelScale( const IFontFace& a_Face, Unit a_Size, f32 a_DpiScale )
        {
            const f32 native = std::max( a_Face.Metrics().NativePixelSize, 1.f );
            return std::max( 1.f, std::floor( a_Size.ToFloat() / native * a_DpiScale + 1e-3f ) );
        }

        /** @brief Whole-pixel pen for pixel runs, so glyph spacing always matches the glyph scale. */
        struct PixelPen
        {
            bool Active{ false };
            f32  Origin{ 0.f };     ///< Run start, in screen pixels.
            f32  Scale{ 1.f };      ///< Screen pixels per native pixel.
            Unit NativeUnit{ 0_u }; ///< Layout units per native pixel.
            f32  Native{ 0.f };     ///< Pen offset from Origin, in native pixels.

            PixelPen( const IFontFace* a_Face, Unit a_Size, f32 a_DpiScale, f32 a_PenX )
            {
                if ( !a_Face || !TextShaping::IsPixelFace( *a_Face ) )
                    return;

                Active     = true;
                Origin     = std::round( a_PenX );
                Scale      = PixelScale( *a_Face, a_Size, a_DpiScale );
                NativeUnit = TextShaping::NativePixelUnit( *a_Face, a_Size );
            }

            f32 ToNative( Unit a_Value ) const { return std::round( a_Value.ToFloat() / NativeUnit.ToFloat() ); }

            f32 GlyphX( Unit a_XOffset ) const { return Origin + ( Native + ToNative( a_XOffset ) ) * Scale; }
            f32 GlyphYOffset( Unit a_YOffset ) const { return ToNative( a_YOffset ) * Scale; }
            void Advance( Unit a_XAdvance ) { Native += ToNative( a_XAdvance ); }
            f32 End() const { return Origin + Native * Scale; }
        };

        /** @brief */
        i32 ToWholeSteps( f32 a_Value )
        {
            if ( a_Value == 0.f )
                return 0;
            const i32 steps = static_cast<i32>( std::round( a_Value ) );
            return steps != 0 ? steps : ( a_Value > 0.f ? 1 : -1 );
        }
    }

    DrawBatcher::BitmapEffects DrawBatcher::ComputeBitmapEffects( const TextRenderStyle& a_Style, const IFontFace& a_Face, Unit a_Size, f32 a_DpiScale, const GlyphAtlasConfig& a_Config ) const
    {
        BitmapEffects effects;
        if ( a_Face.GetMode() == EGlyphRenderMode::MTSDF )
            return effects; // Done in the shader.

        // Sizes are relative to the em, snapped to whole native (Pixel) or screen (Raster) pixels.
        const bool pixel    = TextShaping::IsPixelFace( a_Face );
        const f32  stepPx   = pixel ? PixelScale( a_Face, a_Size, a_DpiScale ) : 1.f;
        const f32  stepsPerEm = pixel ? a_Face.Metrics().NativePixelSize : ToPixel( a_Size, a_DpiScale ).ToFloat();
        const f32  rcpBase  = a_Config.SDFBaseSize > 0 ? 1.f / static_cast<f32>( a_Config.SDFBaseSize ) : 0.f;

        if ( a_Style.Shadow && a_Style.ShadowColor[3] > 0 )
        {
            effects.ShadowDX = static_cast<i16>( ToWholeSteps( a_Style.ShadowOffset[0] * rcpBase * stepsPerEm ) * stepPx );
            effects.ShadowDY = static_cast<i16>( ToWholeSteps( a_Style.ShadowOffset[1] * rcpBase * stepsPerEm ) * stepPx );
        }

        if ( a_Style.Outline && a_Style.OutlineColor[3] > 0 && a_Style.OutlineWidth > 0.f )
        {
            const f32 outlineEm = a_Style.OutlineWidth * a_Config.SDFPixelRange * rcpBase;
            effects.OutlineStep   = static_cast<i16>( stepPx );
            effects.OutlineRadius = static_cast<u8>( std::clamp( ToWholeSteps( outlineEm * stepsPerEm ), 1, 2 ) );
        }

        return effects;
    }

    void DrawBatcher::PlaceGlyph( GlyphAtlas& a_Atlas, const IFontFace& a_Face, const ResolvedFace& a_Resolved, GlyphID a_Glyph,
                                  Unit a_Size, f32 a_PenX, f32 a_BaselineY, f32 a_DpiScale, Color a_Color, const BitmapEffects& a_Effects )
    {
        const GlyphAtlasConfig& config = a_Atlas.GetConfig();
        const EGlyphRenderMode  mode   = a_Face.GetMode();
        const f32               sizePx = ToPixel( a_Size, a_DpiScale ).ToFloat();

        TextQuad quad{};
        quad.FillColor     = a_Color;
        quad.BaselineY     = a_BaselineY;
        quad.ShadowDX      = a_Effects.ShadowDX;
        quad.ShadowDY      = a_Effects.ShadowDY;
        quad.OutlineStep   = a_Effects.OutlineStep;
        quad.OutlineRadius = a_Effects.OutlineRadius;

        Optional<AtlasGlyph> glyph;
        switch ( mode )
        {
            case EGlyphRenderMode::MTSDF:
            {
                glyph = a_Atlas.GetOrRasterizeGlyph( GlyphCacheKey::For( a_Resolved, a_Glyph, mode, 0 ) );
                if ( !glyph || glyph->IsBlank() )
                    return;

                const f32 scale = sizePx / static_cast<f32>( config.SDFBaseSize );
                quad.X0 = a_PenX + glyph->Bearing[0] * sizePx;
                quad.Y0 = a_BaselineY - glyph->Bearing[1] * sizePx;
                quad.X1 = quad.X0 + static_cast<f32>( glyph->Rect.Size[0] ) * scale;
                quad.Y1 = quad.Y0 + static_cast<f32>( glyph->Rect.Size[1] ) * scale;
                quad.SDF = true;

                // Faux bold: grow each edge by half the stroke growth, in SDF units.
                if ( a_Resolved.SynthBold )
                {
                    const f32 emPerSDFUnit = config.SDFPixelRange / static_cast<f32>( config.SDFBaseSize );
                    quad.Weight = TextShaping::SynthBoldEm( a_Size ) * 0.5f / emPerSDFUnit;
                }
                quad.Skew = a_Resolved.SynthItalic ? TextShaping::c_SynthItalicSlant : 0.f;
                break;
            }

            case EGlyphRenderMode::Raster:
            {
                const u16 ppem = static_cast<u16>( std::clamp( std::round( sizePx ), 1.f, 1024.f ) );
                glyph = a_Atlas.GetOrRasterizeGlyph( GlyphCacheKey::For( a_Resolved, a_Glyph, mode, ppem ) );
                if ( !glyph || glyph->IsBlank() )
                    return;

                // Drawn 1:1 on whole pixels.
                quad.X0 = std::round( a_PenX ) + glyph->Bearing[0];
                quad.Y0 = std::round( a_BaselineY ) - glyph->Bearing[1];
                quad.X1 = quad.X0 + static_cast<f32>( glyph->Rect.Size[0] );
                quad.Y1 = quad.Y0 + static_cast<f32>( glyph->Rect.Size[1] );
                break;
            }

            case EGlyphRenderMode::Pixel:
            default:
            {
                const f32 scale  = PixelScale( a_Face, a_Size, a_DpiScale );
                glyph = a_Atlas.GetOrRasterizeGlyph( GlyphCacheKey::For( a_Resolved, a_Glyph, EGlyphRenderMode::Pixel, 0 ) );
                if ( !glyph || glyph->IsBlank() )
                    return;

                // Whole-number scale on a whole pixel.
                quad.X0 = std::round( a_PenX ) + glyph->Bearing[0] * scale;
                quad.Y0 = std::round( a_BaselineY ) - glyph->Bearing[1] * scale;
                quad.X1 = quad.X0 + static_cast<f32>( glyph->Rect.Size[0] ) * scale;
                quad.Y1 = quad.Y0 + static_cast<f32>( glyph->Rect.Size[1] ) * scale;
                break;
            }
        }

        const f32 rcpPage = 1.f / static_cast<f32>( config.PageSize );
        quad.Page = glyph->Page;
        quad.U0   = static_cast<f32>( glyph->Rect.Origin[0] ) * rcpPage;
        quad.V0   = static_cast<f32>( glyph->Rect.Origin[1] ) * rcpPage;
        quad.U1   = static_cast<f32>( glyph->Rect.Origin[0] + glyph->Rect.Size[0] ) * rcpPage;
        quad.V1   = static_cast<f32>( glyph->Rect.Origin[1] + glyph->Rect.Size[1] ) * rcpPage;

        if ( quad.Page )
            PushBack( m_TextQuads, quad );
    }

    void DrawBatcher::PlaceDecoration( GlyphAtlas& a_Atlas, const IFontFace& a_Face, Unit a_Size, u8 a_Decoration,
                                       f32 a_X0, f32 a_X1, f32 a_BaselineY, f32 a_DpiScale, Color a_Color, const BitmapEffects& a_Effects )
    {
        const EGlyphRenderMode mode = a_Face.GetMode();
        const Optional<AtlasGlyph> white = a_Atlas.GetWhiteBlock( mode );
        if ( !white || !white->Page || a_X1 <= a_X0 )
            return;

        const FontFaceMetrics& metrics = a_Face.Metrics();
        const bool underline = ( a_Decoration & ETextDecoration::Underline ) != 0;
        const f32  positionEm  = ( underline ? metrics.UnderlinePosition : metrics.StrikeoutPosition ).ToFloat();
        const f32  thicknessEm = ( underline ? metrics.UnderlineThickness : metrics.StrikeoutThickness ).ToFloat();

        TextQuad quad{};
        quad.FillColor     = a_Color;
        quad.BaselineY     = a_BaselineY;
        quad.ShadowDX      = a_Effects.ShadowDX;
        quad.ShadowDY      = a_Effects.ShadowDY;
        quad.OutlineStep   = a_Effects.OutlineStep;
        quad.OutlineRadius = a_Effects.OutlineRadius;
        quad.SDF           = mode == EGlyphRenderMode::MTSDF;

        if ( TextShaping::IsPixelFace( a_Face ) )
        {
            // At least one native pixel thick, on the native grid.
            const f32 native     = metrics.NativePixelSize;
            const f32 scale      = PixelScale( a_Face, a_Size, a_DpiScale );
            const f32 thickness  = std::max( 1.f, std::round( thicknessEm * native ) );
            const f32 topNative  = std::round( positionEm * native + thickness * 0.5f ); // Y-up
            quad.X0 = std::round( a_X0 );
            quad.X1 = std::round( a_X1 );
            quad.Y0 = std::round( a_BaselineY ) - topNative * scale;
            quad.Y1 = quad.Y0 + thickness * scale;
        }
        else
        {
            // At least one pixel thick, snapped.
            const f32 sizePx    = ToPixel( a_Size, a_DpiScale ).ToFloat();
            const f32 thickness = std::max( 1.f, std::round( thicknessEm * sizePx ) );
            const f32 centre    = a_BaselineY - positionEm * sizePx;
            quad.X0 = a_X0;
            quad.X1 = a_X1;
            quad.Y0 = std::round( centre - thickness * 0.5f );
            quad.Y1 = quad.Y0 + thickness;
        }

        // Constant UV in the white block, so the quad is solid.
        const f32 texel = static_cast<f32>( white->Rect.Origin[0] + 1 ) / static_cast<f32>( a_Atlas.GetConfig().PageSize );
        quad.Page = white->Page;
        quad.U0 = quad.U1 = texel;
        quad.V0 = quad.V1 = texel;

        PushBack( m_TextQuads, quad );
    }

    void DrawBatcher::SplitCurrentBatch()
    {
        RATUI_ASSERT( !Empty( m_Batches ), "SplitCurrentBatch requires an active batch." );
        DrawBatch next = Back( m_Batches );
        next.VertexByteOffset = static_cast<u32>( Size( m_Vertices ) );
        next.IndexOffset      = static_cast<u32>( Size( m_Indices ) );
        next.IndexCount       = 0;
        PushBack( m_Batches, std::move( next ) );
    }

    void DrawBatcher::FlushTextQuads( const TextEmitParams& a_Params )
    {
        if ( Empty( m_TextQuads ) )
            return;

        const TextRenderStyle&  style  = *a_Params.Style;
        const GlyphAtlasConfig& config = a_Params.Atlas->GetConfig();

        const TextureHandle* currentPage = nullptr;
        bool currentSDF = false;

        const auto ensureBatch = [&]( const TextQuad& a_Quad )
        {
            if ( currentPage == a_Quad.Page && currentSDF == a_Quad.SDF && !Empty( m_Batches ) )
                return;

            if ( a_Quad.SDF )
                EnsureBatch( a_Params.ClipRect, a_Params.Transform, MSDFTextDrawData::From( *a_Quad.Page, style, config.SDFPixelRange, config.PageSize ) );
            else
                EnsureBatch( a_Params.ClipRect, a_Params.Transform, BitmapTextDrawData{ *a_Quad.Page } );

            currentPage = a_Quad.Page;
            currentSDF  = a_Quad.SDF;
        };

        // ETextOverflow::Fade: opacity ramps to 0 over the last FadePercent of the box.
        const Rect<Pixel>& fadeRect = a_Params.FadeRect;
        const f32 fadeEndX   = fadeRect.Right().ToFloat();
        const f32 fadeStartX = fadeEndX - fadeRect.Size[0].ToFloat() * a_Params.FadePercent;
        const f32 fadeEndY   = fadeRect.Bottom().ToFloat();
        const f32 fadeStartY = fadeEndY - fadeRect.Size[1].ToFloat() * a_Params.FadePercent;

        const auto fadeAt = [&]( f32 a_X, f32 a_Y ) -> f32
        {
            f32 opacity = 1.f;
            if ( a_Params.FadeHorizontal && a_X > fadeStartX )
                opacity *= a_X >= fadeEndX ? 0.f : 1.f - ( a_X - fadeStartX ) / std::max( fadeEndX - fadeStartX, 1e-3f );
            if ( a_Params.FadeVertical && a_Y > fadeStartY )
                opacity *= a_Y >= fadeEndY ? 0.f : 1.f - ( a_Y - fadeStartY ) / std::max( fadeEndY - fadeStartY, 1e-3f );
            return opacity;
        };

        const auto emitQuad = [&]( const TextQuad& a_Quad, f32 a_DX, f32 a_DY, Color a_Color, f32 a_Weight )
        {
            ensureBatch( a_Quad );

            u32 vertexBase = ( static_cast<u32>( Size( m_Vertices ) ) - Back( m_Batches ).VertexByteOffset ) / sizeof( TextVertex );
            if ( vertexBase + 4 > Limits<u16>::max() )
            {
                SplitCurrentBatch();
                vertexBase = 0;
            }

            const f32 x0 = a_Quad.X0 + a_DX, x1 = a_Quad.X1 + a_DX;
            const f32 y0 = a_Quad.Y0 + a_DY, y1 = a_Quad.Y1 + a_DY;

            // Faux italic: shear around the baseline.
            const f32 skewTop    = a_Quad.Skew * ( a_Quad.BaselineY + a_DY - y0 );
            const f32 skewBottom = a_Quad.Skew * ( a_Quad.BaselineY + a_DY - y1 );

            auto verts = ReserveVertices<TextVertex>( 4 );
            verts[0] = TextVertex{ { Pixel{ x0 + skewTop },    Pixel{ y0 } }, { a_Quad.U0, a_Quad.V0 }, a_Color, a_Weight, fadeAt( x0, y0 ) };
            verts[1] = TextVertex{ { Pixel{ x1 + skewTop },    Pixel{ y0 } }, { a_Quad.U1, a_Quad.V0 }, a_Color, a_Weight, fadeAt( x1, y0 ) };
            verts[2] = TextVertex{ { Pixel{ x0 + skewBottom }, Pixel{ y1 } }, { a_Quad.U0, a_Quad.V1 }, a_Color, a_Weight, fadeAt( x0, y1 ) };
            verts[3] = TextVertex{ { Pixel{ x1 + skewBottom }, Pixel{ y1 } }, { a_Quad.U1, a_Quad.V1 }, a_Color, a_Weight, fadeAt( x1, y1 ) };

            auto idx = ReserveIndices( 6 );
            idx[0] = static_cast<u16>( vertexBase + 0 );
            idx[1] = static_cast<u16>( vertexBase + 1 );
            idx[2] = static_cast<u16>( vertexBase + 2 );
            idx[3] = static_cast<u16>( vertexBase + 1 );
            idx[4] = static_cast<u16>( vertexBase + 3 );
            idx[5] = static_cast<u16>( vertexBase + 2 );
            AddIndicesToCurrentBatch( 6 );
        };

        const auto withAlpha = []( Color a_Effect, Color a_Fill ) -> Color
        {
            a_Effect[3] = static_cast<u8>( ( static_cast<u32>( a_Effect[3] ) * a_Fill[3] + 127 ) / 255 );
            return a_Effect;
        };

        // All bitmap shadows first, so no shadow covers a neighbouring glyph.
        if ( style.Shadow && style.ShadowColor[3] > 0 )
        {
            for ( const TextQuad& quad : m_TextQuads )
            {
                if ( !quad.SDF && ( quad.ShadowDX != 0 || quad.ShadowDY != 0 ) )
                    emitQuad( quad, quad.ShadowDX, quad.ShadowDY, withAlpha( style.ShadowColor, quad.FillColor ), 1.f );
            }
        }

        // Then bitmap outlines, as silhouettes offset around the glyph.
        if ( style.Outline && style.OutlineColor[3] > 0 )
        {
            for ( const TextQuad& quad : m_TextQuads )
            {
                if ( quad.SDF || quad.OutlineStep == 0 )
                    continue;

                const i32 radius = quad.OutlineRadius;
                const Color color = withAlpha( style.OutlineColor, quad.FillColor );
                for ( i32 dy = -radius; dy <= radius; ++dy )
                    for ( i32 dx = -radius; dx <= radius; ++dx )
                        if ( dx != 0 || dy != 0 )
                            emitQuad( quad, static_cast<f32>( dx * quad.OutlineStep ), static_cast<f32>( dy * quad.OutlineStep ), color, 1.f );
            }
        }

        // Then fills. MTSDF quads draw their own effects in the shader.
        for ( const TextQuad& quad : m_TextQuads )
            emitQuad( quad, 0.f, 0.f, quad.FillColor, quad.SDF ? quad.Weight : 0.f );

        ::RatUI::Clear( m_TextQuads );
    }

    void DrawBatcher::EmitText(
        const ShapedText&        a_Text,
        const TextRenderStyle&   a_Style,
        Rect<Pixel>              a_LayoutRect,
        GlyphAtlas&              a_Atlas,
        f32                      a_DpiScale,
        const Optional<Rectu16>& a_ClipRect,
        const Mat3f&             a_Transform,
        u32                      a_MaxGlyphs )
    {
        if ( Empty( a_Text.Lines ) )
            return;

        FontLibrary& fonts = a_Atlas.GetFontLibrary();
        const Pixel textHeight = ToPixel( a_Text.TotalHeight, a_DpiScale );
        const bool  singleLine = a_Text.LineCount() == 1;
        const f32   fadePct    = std::clamp( a_Style.FadePercentage, 0.f, 1.f );

        TextEmitParams params{
            .Style          = &a_Style,
            .Atlas          = &a_Atlas,
            .ClipRect       = a_ClipRect,
            .Transform      = a_Transform,
            .FadeRect       = a_LayoutRect,
            .FadeHorizontal = singleLine && fadePct > 0.f,
            .FadeVertical   = !singleLine && fadePct > 0.f && textHeight > a_LayoutRect.Size[1],
            .FadePercent    = fadePct,
        };

        f32 top = a_LayoutRect.Origin[1].ToFloat();
        switch ( a_Style.Baseline )
        {
            case ETextBaseline::Middle:     top += ( a_LayoutRect.Size[1] - textHeight ).ToFloat() * 0.5f; break;
            case ETextBaseline::Bottom:     top += ( a_LayoutRect.Size[1] - textHeight ).ToFloat(); break;
            case ETextBaseline::Alphabetic: top -= ToPixel( a_Text.Lines[0].Baseline, a_DpiScale ).ToFloat(); break; // First baseline on the box top.
            case ETextBaseline::Top:
            case ETextBaseline::Hanging:
            default: break;
        }

        const u8 styleDecorations = ( a_Style.Underline ? ETextDecoration::Underline : 0 )
                                  | ( a_Style.Strikethrough ? ETextDecoration::Strikethrough : 0 );

        u32  glyphCount = 0;
        bool reachedLimit = false;

        for ( const ShapedLine& line : a_Text.Lines )
        {
            f32 penX = a_LayoutRect.Origin[0].ToFloat();
            switch ( a_Style.Align )
            {
                case ETextAlign::Center: penX += ( a_LayoutRect.Size[0] - ToPixel( line.Width, a_DpiScale ) ).ToFloat() * 0.5f; break;
                case ETextAlign::Right:  penX += ( a_LayoutRect.Size[0] - ToPixel( line.Width, a_DpiScale ) ).ToFloat(); break;
                default: break; // TODO: Justify.
            }

            const f32 baselineY = top + ToPixel( line.Baseline, a_DpiScale ).ToFloat();

            for ( u32 r = line.RunStart; r < line.RunEnd && !reachedLimit; ++r )
            {
                const ShapedRun& run  = a_Text.Runs[r];
                const IFontFace* face = fonts.GetFace( run.Face.Face );
                const Color      color = run.HasFillColor ? run.FillColor : a_Style.FillColor;
                const BitmapEffects effects = face ? ComputeBitmapEffects( a_Style, *face, run.Size, a_DpiScale, a_Atlas.GetConfig() ) : BitmapEffects{};
                const f32 runStartX = penX;
                PixelPen  pixelPen( face, run.Size, a_DpiScale, penX );

                for ( u32 g = run.GlyphStart; g < run.GlyphEnd; ++g )
                {
                    if ( glyphCount >= a_MaxGlyphs )
                    {
                        reachedLimit = true;
                        break;
                    }
                    ++glyphCount;

                    const ShapedGlyph& glyph = a_Text.Glyphs[g];
                    if ( face )
                    {
                        const f32 x = pixelPen.Active ? pixelPen.GlyphX( glyph.XOffset ) : penX + ToPixel( glyph.XOffset, a_DpiScale ).ToFloat();
                        const f32 y = baselineY + ( pixelPen.Active ? pixelPen.GlyphYOffset( glyph.YOffset ) : ToPixel( glyph.YOffset, a_DpiScale ).ToFloat() );
                        PlaceGlyph( a_Atlas, *face, run.Face, glyph.GlyphIndex, run.Size, x, y, a_DpiScale, color, effects );
                    }

                    // Keep following the layout, so later runs stay where layout put them.
                    penX += ToPixel( glyph.XAdvance, a_DpiScale ).ToFloat();
                    pixelPen.Advance( glyph.XAdvance );
                }

                const u8  decorations = run.Decorations | styleDecorations;
                const f32 runEndX     = pixelPen.Active ? pixelPen.End() : penX;
                if ( face && decorations != 0 )
                {
                    if ( decorations & ETextDecoration::Underline )
                        PlaceDecoration( a_Atlas, *face, run.Size, ETextDecoration::Underline, runStartX, runEndX, baselineY, a_DpiScale, color, effects );
                    if ( decorations & ETextDecoration::Strikethrough )
                        PlaceDecoration( a_Atlas, *face, run.Size, ETextDecoration::Strikethrough, runStartX, runEndX, baselineY, a_DpiScale, color, effects );
                }
            }

            if ( reachedLimit )
                break;
        }

        FlushTextQuads( params );
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
