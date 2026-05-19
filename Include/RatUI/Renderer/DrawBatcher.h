#pragma once
#include "Texture.h"
#include "../Text/GlyphAtlas.h"

namespace RatUI
{
    /**
     * @brief Represents the radius of each corner of a rectangle, allowing for asymmetric rounding.
     */
    struct CornerRounding
    {
        Unit TopLeft{};
        Unit TopRight{};
        Unit BottomLeft{};
        Unit BottomRight{};

        static constexpr CornerRounding None() { return {}; }
        static constexpr CornerRounding Uniform( Unit a_Value ) { return { a_Value, a_Value, a_Value, a_Value }; }
        static constexpr CornerRounding Symmetric( Unit a_Top, Unit a_Bottom ) { return { a_Top, a_Top, a_Bottom, a_Bottom }; }
        static constexpr CornerRounding Asymmetric( Unit a_TopLeft, Unit a_TopRight, Unit a_BottomLeft, Unit a_BottomRight ) 
        { 
            return { a_TopLeft, a_TopRight, a_BottomLeft, a_BottomRight }; 
        }

        constexpr CornerRounding operator+( Unit a_Amount ) const
        {
            return {
                .TopLeft = TopLeft + a_Amount,
                .TopRight = TopRight + a_Amount,
                .BottomLeft = BottomLeft + a_Amount,
                .BottomRight = BottomRight + a_Amount
            };
        }

        constexpr CornerRounding operator-( Unit a_Amount ) const
        {
            return {
                .TopLeft = TopLeft - a_Amount,
                .TopRight = TopRight - a_Amount,
                .BottomLeft = BottomLeft - a_Amount,
                .BottomRight = BottomRight - a_Amount
            };
        }

        constexpr CornerRounding operator*( Unit a_Scalar ) const
        {
            return {
                .TopLeft = TopLeft * a_Scalar,
                .TopRight = TopRight * a_Scalar,
                .BottomLeft = BottomLeft * a_Scalar,
                .BottomRight = BottomRight * a_Scalar
            };
        }

        constexpr CornerRounding operator/( Unit a_Scalar ) const
        {
            return {
                .TopLeft = TopLeft / a_Scalar,
                .TopRight = TopRight / a_Scalar,
                .BottomLeft = BottomLeft / a_Scalar,
                .BottomRight = BottomRight / a_Scalar
            };
        }
    };

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
    struct DrawBatcher
    {
        Array<byte>      Vertices;
        Array<u16>       Indices;
        Array<DrawBatch> Batches;

        template<typename VertexT>
        Span<VertexT> ReserveVertices( u32 a_Count )
        {
            const u32 byteOffset = static_cast<u32>( Size( Vertices ) );
            Resize( Vertices, byteOffset + a_Count * sizeof( VertexT ) );
            return Span<VertexT>{ reinterpret_cast<VertexT*>( Data( Vertices ) + byteOffset ), a_Count };
        }

        Span<u16> ReserveIndices( u32 a_Count )
        {
            const u32 offset = static_cast<u32>( Size( Indices ) );
            Resize( Indices, offset + a_Count );
            return Span<u16>{ Data( Indices ) + offset, a_Count };
        }

        void Clear()
        {
            ::RatUI::Clear( Vertices );
            ::RatUI::Clear( Indices );
            ::RatUI::Clear( Batches );
        }

        void Flatten()
        {
            if ( Size( Batches ) < 2 )
                return;

            size write = 0;
            for ( size read = 1; read < Size( Batches ); ++read )
            {
                DrawBatch& current = Batches[read];
                DrawBatch& last    = Batches[write];

                if ( current.CanFlattenWith( last ) )
                {
                    last.IndexCount += current.IndexCount;
                }
                else
                {
                    ++write;
                    if ( write != read )
                        Batches[write] = current;
                }
            }

            Resize( Batches, write + 1 );
        }

        void TryFlatten()
        {
            if ( Size( Batches ) < 2 )
                return;

            const DrawBatch& consumable = Back( Batches );
                  DrawBatch& consumer   = Batches[ Size( Batches ) - 2 ];

            if ( consumable.CanFlattenWith( consumer ) )
            {
                consumer.IndexCount += consumable.IndexCount;
                PopBack( Batches );
            }
        }

        DrawBatch& EnsureSDFBatch( const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, TextureHandle a_Texture )
        {
            return EnsureBatch( a_ClipRect, a_Transform, SDFDrawData{ .Texture = std::move( a_Texture ) } );
        }

        DrawBatch& EnsureMSDFTextBatch( const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, const MSDFTextDrawData& a_Data )
        {
            return EnsureBatch( a_ClipRect, a_Transform, a_Data );
        }

        void EmitRect( Rect<Pixel> a_Rect, Color a_FillColor,
                       f32 a_BorderThickness = 0.f, Color a_BorderColor = Colors::Transparent,
                       Vec4<Pixel> a_Rounding = {} )
        {
            const f32 w  = a_Rect.Size[0].ToFloat();
            const f32 h  = a_Rect.Size[1].ToFloat();
            const f32 cx = a_Rect.Origin[0].ToFloat() + w * 0.5f;
            const f32 cy = a_Rect.Origin[1].ToFloat() + h * 0.5f;

            const Vec2f halfSize{ w * 0.5f, h * 0.5f };

            // Expand the quad by border + aaPad pixels on every side so the SDF edge
            // gradient and any border are never clipped by the triangle boundary.
            constexpr f32 aaPad = 1.5f;
            const f32     border = std::max( 0.f, a_BorderThickness );
            const Vec2f   outerHalf{ halfSize[0] + border + aaPad, halfSize[1] + border + aaPad };

            // Index base = number of SDFVertices already written for the current batch.
            const u32 vertexBase = ( static_cast<u32>( Size( Vertices ) ) - Back( Batches ).VertexByteOffset ) / sizeof( SDFVertex );

            auto verts = ReserveVertices<SDFVertex>( 4 );

            auto makeVert = [&]( f32 xs, f32 ys, Vec2f uv, Pixel radius ) -> SDFVertex
            {
                return SDFVertex{
                    .Position        = Vec2<Pixel>{ Pixel{ cx + xs * outerHalf[0] },
                                                    Pixel{ cy + ys * outerHalf[1] } },
                    .LocalPos        = Vec2<Pixel>{ Pixel{ xs * outerHalf[0] },
                                                    Pixel{ ys * outerHalf[1] } },
                    .UV              = uv,
                    .FillColor       = a_FillColor,
                    .BorderColor     = a_BorderColor,
                    .BorderThickness = Pixel{ border },
                    .HalfSize        = Vec2<Pixel>{ Pixel{ halfSize[0] }, Pixel{ halfSize[1] } },
                    .CornerRadius    = radius,
                };
            };

            // Each vertex carries its own corner radius so the fragment shader
            // can interpolate and select the correct value per corner.
            verts[0] = makeVert( -1.f, -1.f, { 0.f, 0.f }, a_Rounding[0] );
            verts[1] = makeVert(  1.f, -1.f, { 1.f, 0.f }, a_Rounding[1] );
            verts[2] = makeVert( -1.f,  1.f, { 0.f, 1.f }, a_Rounding[2] );
            verts[3] = makeVert(  1.f,  1.f, { 1.f, 1.f }, a_Rounding[3] );

            auto idx = ReserveIndices( 6 );
            idx[0] = vertexBase + 0; idx[1] = vertexBase + 1; idx[2] = vertexBase + 2;
            idx[3] = vertexBase + 1; idx[4] = vertexBase + 3; idx[5] = vertexBase + 2;
            AddIndicesToCurrentBatch( 6 );

            TryFlatten();
        }

        void EmitCircle( Vec2<Pixel> a_Center, Pixel a_Radius, Color a_Color, Pixel a_BorderThickness = 0_px, Color a_BorderColor = Colors::Transparent )
        {
            // A circle is a rounded rect whose corner radius equals its half-size.
            const Rect<Pixel> rect{
                a_Center - Vec2<Pixel>{ a_Radius, a_Radius },
                Vec2<Pixel>{ a_Radius * 2.f, a_Radius * 2.f }
            };

            EmitRect( rect, a_Color, a_BorderThickness.ToFloat(), a_BorderColor,
                      Vec4<Pixel>{ a_Radius, a_Radius, a_Radius, a_Radius } );
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

                    // Index base in terms of TextVertex count within the current batch.
                    const u32 vertexBase = ( static_cast<u32>( Size( Vertices ) ) - Back( Batches ).VertexByteOffset ) / sizeof( TextVertex );

                    auto verts = ReserveVertices<TextVertex>( 4 );
                    verts[0] = TextVertex{ Vec2<Pixel>{ gx,      gy      }, opacityA, Vec2f{ u0, v0 } };
                    verts[1] = TextVertex{ Vec2<Pixel>{ gx + gw, gy      }, opacityB, Vec2f{ u1, v0 } };
                    verts[2] = TextVertex{ Vec2<Pixel>{ gx,      gy + gh }, opacityA, Vec2f{ u0, v1 } };
                    verts[3] = TextVertex{ Vec2<Pixel>{ gx + gw, gy + gh }, opacityB, Vec2f{ u1, v1 } };

                    auto idx = ReserveIndices( 6 );
                    idx[0] = vertexBase + 0; idx[1] = vertexBase + 1; idx[2] = vertexBase + 2;
                    idx[3] = vertexBase + 1; idx[4] = vertexBase + 3; idx[5] = vertexBase + 2;
                    AddIndicesToCurrentBatch( 6 );

                    penX += ToPixel( sg.XAdvance, fontSize, a_DpiScale );
                }

                penY += ToPixel( a_Text.LineHeight, a_DpiScale );
            }

            TryFlatten();
        }

    private:
        template<typename DrawDataT>
        DrawBatch& EnsureBatch( const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, const DrawDataT& a_Data )
        {
            DrawBatch newBatch{
                .ClipRect = a_ClipRect,
                .Transform = a_Transform,
                .VertexByteOffset = static_cast<u32>( Size( Vertices ) ),
                .IndexOffset = static_cast<u32>( Size( Indices ) ),
                .IndexCount = 0,
                .Data = a_Data
            };

            if ( Empty( Batches ) || !Back( Batches ).CanFlattenWith( newBatch ) )
                EmplaceBack( Batches, newBatch );

            return Back( Batches );
        }

        void AddIndicesToCurrentBatch( u32 a_Count )
        {
            RATUI_ASSERT( !Empty( Batches ), "Emit call requires an active batch. Call an Ensure*Batch method first." );
            Back( Batches ).IndexCount += a_Count;
        }
    };

} // namespace RatUI
