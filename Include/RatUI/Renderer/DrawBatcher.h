#pragma once
#include "Texture.h"
#include "../Text/GlyphAtlas.h"

namespace RatUI
{
    /**
     * @brief Used for custom rendering commands that don't fit into the predefined categories. 
     * @param a_Renderer The renderer to use for drawing.
     * @param a_Cmd The draw command containing the necessary information for rendering.
     */
    using CustomDrawFunc = void(*)( class IRenderer& a_Renderer, void* a_UserData, const Mat3f& a_CurrentTransform );

    /**
     * @brief Represents a single vertex in the rendering pipeline, containing position, color, and texture coordinates.
     */
    struct TextVertex
    {
        Vec2<Pixel> Position;
        f32         Opacity;  ///< The opacity of the vertex, used for fading effects. Range [0, 1].
        Vec2f       UV;
    };

    /**
     * @brief Represents a vertex used for Signed Distance Field (SDF) rendering, containing information about the position, color, border, and shape of the vertex.
     * For rectangles:
     *  - HalfSize can be any positive value. (E.g. for a 100x50 rect, HalfSize would be (50, 25))
     *  - CornerRadius: 0 = rectangle, >0 = rounded rectangle.
     * For circles:
     *  - HalfSize = (radius, radius)
     *  - CornerRadius = radius
     * 
     * @note This is pretty big for a vertex, but it is okay since there are only a few per shape (e.g. 4 for rectangles and circles).
     */
    struct SDFVertex
    {
        Vec2<Pixel> Position;  ///< The position of the vertex in screen space, used for rasterization.
        Vec2<Pixel> LocalPos;  ///< The position of the vertex relative to the center of the shape, used for SDF calculations.
        Vec2f       UV;        ///< The texture coordinates for sampling an optional texture, used for textured shapes.

        Color       FillColor; ///< The color used for filling the shape. Default is white.

        Color       BorderColor;     ///< The color used for the border of the shape.
        Pixel       BorderThickness; ///< 0 = no border

        Vec2<Pixel> HalfSize;        ///< Half of the width and height of the shape, used for SDF calculations. For rectangles, this is half the size of the rect. For circles, this is the radius in both dimensions.
        Pixel       CornerRadius;    ///< 0 = rect
    };

    /**
     * @brief Represents the draw data for rendering SDF shapes.
     */
    struct SDFDrawData
    {
        TextureHandle Texture{};

        /** 
         * @brief Determines if this SDFDrawData can be flattened with another, 
         * meaning they can be drawn together in the same batch without causing visual artifacts.
         * This is true if all properties that affect the visual output are equal between the two draw data.
         */
        bool CanFlattenWith( const SDFDrawData& a_Other ) const
        {
            return Texture == a_Other.Texture;
        }
    };

    /**
     * @brief Represents the draw data for rendering MSDF text.
     */
    struct MSDFTextDrawData
    {
        TextureHandle FontAtlas{}; ///< The texture handle of the font atlas to use for rendering the text.

        f32 PixelRange{ c_MsdfPxRange };
        f32 Scale{ 1.f };

        bool OutlineEnable : 1 = false;
        bool ShadowEnable  : 1 = false;
        bool GlowEnable    : 1 = false;
        bool InnerGlowEnable : 1 = false;

        // - Fill properties

        Color FillColor    { Colors::White }; ///< The color used for filling the text glyphs. Default is white.
        f32   FillSoftness { 0.5f };          ///< Edge anti-alias softness for the fill, in SDF units [0, 0.5].
        f32   FillThreshold{ 0.5f };          ///< The threshold for determining the filled area of the text, in SDF units [0, 1], typically 0.5.

        // - Outline properties

        Color OutlineColor   { Colors::Transparent }; ///< The color used for the text outline. Default is white.
        f32   OutlineWidth   { 0.f };                 ///< The width of the text outline, in SDF units [0, 0.5], typically 0.05-0.2.
        f32   OutlineSoftness{ 0.f };                 ///< Edge anti-alias softness for the outline, in SDF units [0, 0.5].

        // - Shadow properties

        Color ShadowColor   { Colors::Transparent }; ///< The color used for the text shadow. Default is black.
        Vec2f ShadowOffsetUV{ 0.f, 0.f }; ///< Precomputed UV offset for drop shadow (atlas UV space).
        f32   ShadowSoftness{ 0.f };      ///< Edge anti-alias softness for the shadow, in SDF units [0, 0.5], typically 0.1-0.4.
        f32   ShadowSpread  { 0.f };      ///< The expansion of the shadow's SDF threshold, in SDF units [0, 0.5], typically 0.05-0.2.

        // - Glow properties

        Color GlowColor { Colors::Transparent }; ///< The color used for the text glow. Default is white.
        f32   GlowSpread{ 0.f };                 ///< How far glow extends beyond outline (0.0-0.5).
        f32   GlowPower { 0.0f };                ///< The falloff curve of the glow. Higher values create a tighter and brighter core, 
                                                 ///< while lower values create a softer glow. Typically in the range of 1.0 to 4.0.

        static MSDFTextDrawData From( TextureHandle a_FontAtlas, const TextRenderStyle& a_Style, f32 a_MSDFScale )
        {
            MSDFTextDrawData result{
                .FontAtlas = std::move( a_FontAtlas ),
                .Scale = a_MSDFScale
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
                    a_Style.ShadowOffset[1]
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

        /** 
         * @brief Determines if this MSDFTextDrawData can be flattened with another, 
         * meaning they can be drawn together in the same batch without causing visual artifacts.
         * This is true if all properties that affect the visual output are equal between the two draw 
         */
        bool CanFlattenWith( const MSDFTextDrawData& a_Other ) const
        {
            if ( FontAtlas != a_Other.FontAtlas || PixelRange != a_Other.PixelRange || Scale != a_Other.Scale )
                return false;

            if ( OutlineEnable   != a_Other.OutlineEnable )   return false;
            if ( ShadowEnable    != a_Other.ShadowEnable )    return false;
            if ( GlowEnable      != a_Other.GlowEnable )      return false;
            if ( InnerGlowEnable != a_Other.InnerGlowEnable ) return false;

            // Compare fill properties
            if ( FillColor     != a_Other.FillColor )     return false;
            if ( FillSoftness  != a_Other.FillSoftness )  return false;
            if ( FillThreshold != a_Other.FillThreshold ) return false;

            // Compare outline properties
            if ( OutlineEnable )
            {
                if ( OutlineColor    != a_Other.OutlineColor )    return false;
                if ( OutlineWidth    != a_Other.OutlineWidth )    return false;
                if ( OutlineSoftness != a_Other.OutlineSoftness ) return false;
            }

            // Compare shadow properties
            if ( ShadowEnable )
            {
                if ( ShadowColor    != a_Other.ShadowColor )    return false;
                if ( ShadowOffsetUV != a_Other.ShadowOffsetUV ) return false;
                if ( ShadowSoftness != a_Other.ShadowSoftness ) return false;
                if ( ShadowSpread   != a_Other.ShadowSpread )   return false;
            }

            // Compare glow properties
            if ( GlowEnable )
            {
                if ( GlowColor  != a_Other.GlowColor )  return false;
                if ( GlowSpread != a_Other.GlowSpread ) return false;
                if ( GlowPower  != a_Other.GlowPower )  return false;
            }

            // Compare inner glow properties
            if ( InnerGlowEnable )
            {
                // TODO: Inner Glow
            }

            // If we reach this point, all relevant properties are equal, so the draw data can be flattened together.
            return true;
        }
    };

    /**
     * @brief Represents a batch of draw calls that can be executed together. 
     * Each batch contains information about the clipping rectangle, transformation, texture, 
     * and the range of vertices and indices to use for drawing.
     */
    struct DrawBatch
    {
        Optional<Rectu16> ClipRect;
        Mat3f             Transform;
        u32               VertexByteOffset{ 0 }; ///< Byte offset into the shared vertex buffer where this batch's vertices start.
        u32               IndexOffset{ 0 };
        u32               IndexCount{ 0 };

        Variant<
            SDFDrawData, 
            MSDFTextDrawData> Data;

        /** @brief Checks if this batch can be flattened with another batch. */
        constexpr bool CanFlattenWith( const DrawBatch& a_Other ) const
        {
            if ( ClipRect != a_Other.ClipRect || Transform != a_Other.Transform )
                return false;
            
            // Check if the draw data can be flattened together
            return std::visit( [&]( const auto& a_Data )
            {
                using T = std::decay_t<decltype( a_Data )>;
                if ( !std::holds_alternative<T>( a_Other.Data ) )
                    return false;

                const T& otherData = std::get<T>( a_Other.Data );
                return a_Data.CanFlattenWith( otherData );
            }, Data );
        }
    };

    // TODO: Theres still a lot of optimization I can do here and clean up the api.
    // Maybe have fixed size bump buffers?

    /**
     * @brief A utility class for batching draw calls together. 
     * It allows for efficient rendering by minimizing state changes and draw calls on the backend.
     */
    class DrawBatcher
    {
    public:
		Span<const byte>      GetVertices() const noexcept { return m_Vertices; }
		Span<const u16>       GetIndices()  const noexcept { return m_Indices; }
		Span<const DrawBatch> GetBatches()  const noexcept { return m_Batches; }

        void Clear()
        {
            ::RatUI::Clear( m_Vertices );
            ::RatUI::Clear( m_Indices );
            ::RatUI::Clear( m_Batches );
        }

        DrawBatch& EnsureSDFBatch( const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, TextureHandle a_Texture )
        {
            return EnsureBatch( a_ClipRect, a_Transform, SDFDrawData{ .Texture = std::move( a_Texture ) } );
        }

        DrawBatch& EnsureMSDFTextBatch( const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, const MSDFTextDrawData& a_Data )
        {
            return EnsureBatch( a_ClipRect, a_Transform, a_Data );
        }

        void EmitRect( Rect<Pixel> a_Rect,
                       Color a_FillColor,
                       Pixel a_BorderThickness = 0_px,
                       Color a_BorderColor = Colors::Transparent,
                       Vec4<Pixel> a_Rounding = {} )
        {
            const Pixel w = a_Rect.Size[0];
            const Pixel h = a_Rect.Size[1];
        
            const Pixel halfW = w * 0.5f;
            const Pixel halfH = h * 0.5f;
        
            const Pixel cx = a_Rect.Origin[0] + halfW;
            const Pixel cy = a_Rect.Origin[1] + halfH;
        
            constexpr Pixel aaPad  = 1.5_px;
            const Pixel     border = a_BorderThickness > 0_px ? a_BorderThickness : 0_px;
        
            const Pixel outerHalfW = halfW + border + aaPad;
            const Pixel outerHalfH = halfH + border + aaPad;
        
            const u32 vertexBase =
                ( static_cast<u32>( Size( m_Vertices ) )
                  - Back( m_Batches ).VertexByteOffset )
                / sizeof( SDFVertex );
        
            auto verts = ReserveVertices<SDFVertex>( 4 );
        
            // ------------------------------------------------------------
            // Normalize rounding:
            // If the rounding is too large, it can cause artifacts, 
            // so we need to scale it down to fit within the rect.
            // ------------------------------------------------------------
        
            Pixel r0 = a_Rounding[0] > 0_px ? a_Rounding[0] : 0_px;
            Pixel r1 = a_Rounding[1] > 0_px ? a_Rounding[1] : 0_px;
            Pixel r2 = a_Rounding[2] > 0_px ? a_Rounding[2] : 0_px;
            Pixel r3 = a_Rounding[3] > 0_px ? a_Rounding[3] : 0_px;
        
            f32 scale = 1.0f;
        
            const Pixel top    = r0 + r1;
            const Pixel bottom = r2 + r3;
            const Pixel left   = r0 + r2;
            const Pixel right  = r1 + r3;
        
            if (top    > w) scale = std::min(scale, f32(w / top));
            if (bottom > w) scale = std::min(scale, f32(w / bottom));
            if (left   > h) scale = std::min(scale, f32(h / left));
            if (right  > h) scale = std::min(scale, f32(h / right));
        
            if (scale < 1.0f)
            {
                r0 *= scale;
                r1 *= scale;
                r2 *= scale;
                r3 *= scale;
            }
        
            // ------------------------------------------------------------
            // Emit vertices
            // ------------------------------------------------------------

            verts[0] = {
                .Position        = { cx - outerHalfW, cy - outerHalfH },
                .LocalPos        = { -outerHalfW, -outerHalfH },
                .UV              = { 0.f, 0.f },
                .FillColor       = a_FillColor,
                .BorderColor     = a_BorderColor,
                .BorderThickness = border,
                .HalfSize        = { halfW, halfH },
                .CornerRadius    = r0
            };
        
            verts[1] = {
                .Position        = { cx + outerHalfW, cy - outerHalfH },
                .LocalPos        = {  outerHalfW, -outerHalfH },
                .UV              = { 1.f, 0.f },
                .FillColor       = a_FillColor,
                .BorderColor     = a_BorderColor,
                .BorderThickness = border,
                .HalfSize        = { halfW, halfH },
                .CornerRadius    = r1
            };
        
            verts[2] = {
                .Position        = { cx - outerHalfW, cy + outerHalfH },
                .LocalPos        = { -outerHalfW,  outerHalfH },
                .UV              = { 0.f, 1.f },
                .FillColor       = a_FillColor,
                .BorderColor     = a_BorderColor,
                .BorderThickness = border,
                .HalfSize        = { halfW, halfH },
                .CornerRadius    = r2
            };
        
            verts[3] = {
                .Position        = { cx + outerHalfW, cy + outerHalfH },
                .LocalPos        = {  outerHalfW,  outerHalfH },
                .UV              = { 1.f, 1.f },
                .FillColor       = a_FillColor,
                .BorderColor     = a_BorderColor,
                .BorderThickness = border,
                .HalfSize        = { halfW, halfH },
                .CornerRadius    = r3
            };
        
            auto idx = ReserveIndices(6);
            idx[0] = vertexBase + 0;
            idx[1] = vertexBase + 1;
            idx[2] = vertexBase + 2;
            idx[3] = vertexBase + 1;
            idx[4] = vertexBase + 3;
            idx[5] = vertexBase + 2;
            
            AddIndicesToCurrentBatch(6);

            TryFlatten();
        }

        void EmitText(
            const ShapedText&      a_Text,
            const TextRenderStyle& a_Style,
            Rect<Pixel>            a_LayoutRect,
            GlyphAtlas&            a_Atlas,
            f32                    a_DpiScale )
        {
            if ( Empty( a_Text.Glyphs ) || Empty( a_Text.Lines ) )
                return;

            const f32 atlasW    = static_cast<f32>( a_Atlas.GetConfig().AtlasWidth );
            const f32 atlasH    = static_cast<f32>( a_Atlas.GetConfig().AtlasHeight );
            const f32 rcpAtlasW = atlasW > 0.f ? 1.f / atlasW : 0.f;
            const f32 rcpAtlasH = atlasH > 0.f ? 1.f / atlasH : 0.f;

            const Unit  fontSize   = a_Text.FontSize;
            const Pixel textHeight = ToPixel( a_Text.TotalHeight, a_DpiScale );
            const Pixel ascender   = ToPixel( a_Text.Ascender,    a_DpiScale );

            const f32 baseSize   = a_Atlas.GetConfig().BaseSize.ToFloat();
            const f32 rcpBase    = baseSize > 0.f ? 1.f / baseSize : 0.f;
            const f32 fontSizePx = ToPixel( fontSize, a_DpiScale ).ToFloat();

            const Pixel layoutRight = a_LayoutRect.Origin[0] + a_LayoutRect.Size[0];
            const f32   fadePct     = std::clamp( a_Style.FadePercentage, 0.0f, 1.0f );
            const Pixel fadeWidth   = a_LayoutRect.Size[0] * fadePct;
            const Pixel fadeStartX  = layoutRight - fadeWidth;
            const Pixel fadeEndX    = layoutRight;

            auto computeFadeAlpha = [&]( Pixel x ) -> u8
            {
                if ( fadePct <= 0.f || x <= fadeStartX ) return a_Style.FillColor[3];
                if ( x >= fadeEndX )                     return 0;

                const f32 t     = ( x - fadeStartX ).ToFloat() / ( fadeEndX - fadeStartX ).ToFloat();
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
                const ShapedLine& line = a_Text.Lines[ lineIdx ];

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

                u32 vertexBase = ( static_cast<u32>( Size( m_Vertices ) ) 
                                   - Back( m_Batches ).VertexByteOffset ) 
                                 / sizeof( TextVertex );

                for ( u32 g = line.Start; g < line.End; ++g )
                {
                    const ShapedGlyph& sg = a_Text.Glyphs[ g ];

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

                    const f32 u0 = static_cast<f32>( gr->AtlasRect.Origin[0]                          ) * rcpAtlasW;
                    const f32 v0 = static_cast<f32>( gr->AtlasRect.Origin[1]                          ) * rcpAtlasH;
                    const f32 u1 = static_cast<f32>( gr->AtlasRect.Origin[0] + gr->AtlasRect.Size[0]  ) * rcpAtlasW;
                    const f32 v1 = static_cast<f32>( gr->AtlasRect.Origin[1] + gr->AtlasRect.Size[1]  ) * rcpAtlasH;

                    const f32 opacityA = computeFadeAlpha( gx      ) / 255.0f;
                    const f32 opacityB = computeFadeAlpha( gx + gw ) / 255.0f;

                    auto verts = ReserveVertices<TextVertex>( 4 );
                    verts[0] = TextVertex{ Vec2<Pixel>{ gx,      gy      }, opacityA, Vec2f{ u0, v0 } };
                    verts[1] = TextVertex{ Vec2<Pixel>{ gx + gw, gy      }, opacityB, Vec2f{ u1, v0 } };
                    verts[2] = TextVertex{ Vec2<Pixel>{ gx,      gy + gh }, opacityA, Vec2f{ u0, v1 } };
                    verts[3] = TextVertex{ Vec2<Pixel>{ gx + gw, gy + gh }, opacityB, Vec2f{ u1, v1 } };

                    auto idx = ReserveIndices( 6 );
                    idx[0] = vertexBase + 0; idx[1] = vertexBase + 1; idx[2] = vertexBase + 2;
                    idx[3] = vertexBase + 1; idx[4] = vertexBase + 3; idx[5] = vertexBase + 2;
                    AddIndicesToCurrentBatch( 6 );

                    penX       += ToPixel( sg.XAdvance, fontSize, a_DpiScale );
                    vertexBase += 4;
                }

                penY += ToPixel( a_Text.LineHeight, a_DpiScale );
            }

            TryFlatten();
        }

    protected:
        Array<byte>      m_Vertices;
        Array<u16>       m_Indices;
        Array<DrawBatch> m_Batches;

        void TryFlatten()
        {
            if ( Size( m_Batches ) < 2 )
                return;

            const DrawBatch& consumable = Back( m_Batches );
                  DrawBatch& consumer   = m_Batches[Size( m_Batches ) - 2];

            if ( consumable.CanFlattenWith( consumer ) )
            {
                consumer.IndexCount += consumable.IndexCount;
                PopBack( m_Batches );
            }
        }

        template<typename DrawDataT>
        DrawBatch& EnsureBatch( const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, const DrawDataT& a_Data )
        {
            DrawBatch newBatch{
                .ClipRect         = a_ClipRect,
                .Transform        = a_Transform,
                .VertexByteOffset = static_cast<u32>( Size( m_Vertices ) ),
                .IndexOffset      = static_cast<u32>( Size( m_Indices ) ),
                .IndexCount       = 0,
                .Data             = a_Data
            };

            if ( Empty( m_Batches ) || !Back( m_Batches ).CanFlattenWith( newBatch ) )
                EmplaceBack( m_Batches, newBatch );

            return Back( m_Batches );
        }

        template<typename VertexT>
        Span<VertexT> ReserveVertices( u32 a_Count )
        {
            const u32 byteOffset = static_cast<u32>( Size( m_Vertices ) );
            Resize( m_Vertices, byteOffset + a_Count * sizeof( VertexT ) );
            return Span<VertexT>{ 
                reinterpret_cast<VertexT*>( Data( m_Vertices ) + byteOffset ), 
                a_Count };
        }

        Span<u16> ReserveIndices( u32 a_Count )
        {
            const u32 offset = static_cast<u32>( Size( m_Indices ) );
            Resize( m_Indices, offset + a_Count );
            return Span<u16>{ Data( m_Indices ) + offset, a_Count };
        }

        void AddIndicesToCurrentBatch( u32 a_Count )
        {
            RATUI_ASSERT( !Empty( m_Batches ), "Emit call requires an active batch. Call an Ensure*Batch method first." );
            Back( m_Batches ).IndexCount += a_Count;
        }
    };

} // namespace RatUI
