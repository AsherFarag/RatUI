#pragma once
#include "Texture.h"
#include "../Text/GlyphAtlas.h"

namespace RatUI
{
    /**
     * @brief Represents the rounding of the corners of a rectangle. 
     * Each corner can have its own rounding value, allowing for asymmetric designs.
     */
    struct CornerRounding
    {
        Degreesf TopLeft{ 0.f };
        Degreesf TopRight{ 0.f };
        Degreesf BottomLeft{ 0.f };
        Degreesf BottomRight{ 0.f };

        static constexpr CornerRounding None() { return {}; }
        static constexpr CornerRounding Uniform( Degreesf a_Radius ) { return { a_Radius, a_Radius, a_Radius, a_Radius }; }
        static constexpr CornerRounding Symmetric( Degreesf a_Top, Degreesf a_Bottom ) { return { a_Top, a_Top, a_Bottom, a_Bottom }; }

        constexpr CornerRounding operator+( Degreesf a_Amount ) const
        {
            return {
                .TopLeft = TopLeft + a_Amount,
                .TopRight = TopRight + a_Amount,
                .BottomLeft = BottomLeft + a_Amount,
                .BottomRight = BottomRight + a_Amount
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
    struct Vertex
    {
        Vec2<Pixel> Position;
        Coloru8     Color;
        Vec2f       UV;
    };

    enum class EBatchType
    {
        Geometry,
        MSDF,
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
        TextureID         Texture{ 0 };
        u32               IndexOffset{ 0 };
        u32               IndexCount{ 0 };
        EBatchType        Type{ EBatchType::Geometry };

        struct
        {
            f32 PixelRange{ c_MsdfPxRange };
            f32 Scale{ 1.f };

            bool OutlineEnable : 1 = false;
            bool ShadowEnable  : 1 = false;
            bool GlowEnable    : 1 = false;
            bool InnerGlowEnable : 1 = false;

            // - Fill properties

            Coloru8 FillColor{ Colorsu8::White }; ///< The color used for filling the text glyphs. Default is white.
            f32     FillSoftness{ 0.5f };         ///< Edge anti-alias softness for the fill, in SDF units [0, 0.5].
            f32     FillThreshold{ 0.5f };        ///< The threshold for determining the filled area of the text, in SDF units [0, 1], typically 0.5.

            // - Outline properties

            Coloru8 OutlineColor{ Colorsu8::Transparent }; ///< The color used for the text outline. Default is white.
            f32     OutlineWidth{ 0.f };            ///< The width of the text outline, in SDF units [0, 0.5], typically 0.05-0.2.
            f32     OutlineSoftness{ 0.f };        ///< Edge anti-alias softness for the outline, in SDF units [0, 0.5].

            // - Shadow properties

            Coloru8 ShadowColor   { Colorsu8::Transparent }; ///< The color used for the text shadow. Default is black.
            Vec2f   ShadowOffsetUV{ 0.f, 0.f }; ///< Precomputed UV offset for drop shadow (atlas UV space).
            f32     ShadowSoftness{ 0.f };      ///< Edge anti-alias softness for the shadow, in SDF units [0, 0.5], typically 0.1-0.4.
            f32     ShadowSpread  { 0.f };      ///< The expansion of the shadow's SDF threshold, in SDF units [0, 0.5], typically 0.05-0.2.

            // - Glow properties

            Coloru8 GlowColor { Colorsu8::Transparent }; ///< The color used for the text glow. Default is white.
            f32     GlowSpread{ 0.f };          ///< How far glow extends beyond outline (0.0-0.5).
            f32     GlowPower { 0.0f };            ///< The falloff curve of the glow. Higher values create a tighter and brighter core, while lower values create a softer glow. Typically in the range of 1.0 to 4.0.
        } MSDF;

        // TODO: Add custom draw callbacks.
    };

    /**
     * @brief A utility class for batching draw calls together. 
     * It allows for efficient rendering by minimizing state changes and draw calls on the backend.
     */
    struct DrawBatcher
    {
        Array<Vertex>    Vertices;
        Array<u16>       Indices;
        Array<DrawBatch> Batches;

        /** @brief Reserves space for a specified number of vertices. */
        Span<Vertex> ReserveVertices( u32 a_Count )
        {
            u32 offset = static_cast<u32>( Size( Vertices ) );
            Resize( Vertices, offset + a_Count );
            return Span<Vertex>{ Data( Vertices ) + offset, a_Count };
        }

        /** @brief Reserves space for a specified number of indices. */
        Span<u16> ReserveIndices( u32 a_Count )
        {
            u32 offset = static_cast<u32>( Size( Indices ) );
            Resize( Indices, offset + a_Count );
            return Span<u16>{ Data( Indices ) + offset, a_Count };
        }

        void Clear()
        {
            ::RatUI::Clear( Vertices );
            ::RatUI::Clear( Indices );
            ::RatUI::Clear( Batches );
        }

        DrawBatch& BeginBatch( EBatchType a_Type, const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, TextureID a_Texture )
        {
            EmplaceBack( Batches, DrawBatch{ a_ClipRect, a_Transform, a_Texture, static_cast<u32>( Indices.size() ), 0, a_Type } );
            return Back( Batches );
        }

        /** @brief Ends the current draw batch, calculating the number of indices used. */
        void EndBatch()
        {
            if ( Empty( Batches ) )
                return;

            DrawBatch& currentBatch = Back( Batches );
            currentBatch.IndexCount = static_cast<u32>( Size( Indices ) - currentBatch.IndexOffset );
        }

        /** 
         * @brief Pushes a rectangle into the batcher, creating the necessary vertices and indices for rendering.
         * @param a_Rect The rectangle to render, defined by its origin and size.
         * @param a_Color The color to use for the rectangle.
         */
        void EmitRect( const Rect<Pixel>& a_Rect, Coloru8 a_Color )
        {
            const u32 vertexOffset = static_cast<u32>( Size( Vertices ) );

            auto vertices = ReserveVertices( 4 );
            vertices[0] = Vertex{ a_Rect.TopLeft(),     a_Color, Vec2f{ 0.f, 0.f } };
            vertices[1] = Vertex{ a_Rect.TopRight(),    a_Color, Vec2f{ 1.f, 0.f } };
            vertices[2] = Vertex{ a_Rect.BottomLeft(),  a_Color, Vec2f{ 0.f, 1.f } };
            vertices[3] = Vertex{ a_Rect.BottomRight(), a_Color, Vec2f{ 1.f, 1.f } };
        
            auto indices  = ReserveIndices( 6 );
            indices[0] = vertexOffset + 0; indices[1] = vertexOffset + 1; indices[2] = vertexOffset + 2;
            indices[3] = vertexOffset + 1; indices[4] = vertexOffset + 3; indices[5] = vertexOffset + 2;
        }

        /** 
         * @brief Pushes a rectangle border into the batcher, creating the necessary vertices and indices for rendering. 
         * The border is created by emitting four rectangles (one for each side) with the specified thickness.
         * @param a_Rect The rectangle to render the border for, defined by its origin and size.
         * @param a_Color The color to use for the border.
         * @param a_Thickness The thickness of the border in pixels.
         */
        void EmitRectBorder( const Rect<Pixel>& a_Rect, f32 a_Radius, Coloru8 a_Color, f32 a_Thickness )
        {
            if ( a_Thickness <= 0.f )
                return;

            const f32 t = a_Thickness;
            const Vec2<Pixel> tl = a_Rect.TopLeft();
            const Vec2<Pixel> tr = a_Rect.TopRight();
            const Vec2<Pixel> bl = a_Rect.BottomLeft();
            const Vec2<Pixel> br = a_Rect.BottomRight();

            Reserve( Vertices, 4 * 4 ); // 4 rects with 4 vertices each
            Reserve( Indices,  4 * 6 ); // 4 rects with 2 triangles (6 indices) each

            // Top
            EmitRect( Rect<Pixel>::FromMinMax( tl, Vec2<Pixel>{ tr[0], tl[1] + Pixel{ t } } ), a_Color );
            // Bottom
            EmitRect( Rect<Pixel>::FromMinMax( Vec2<Pixel>{ bl[0], bl[1] - Pixel{ t } }, Vec2<Pixel>{ br[0], bl[1] } ), a_Color );
            // Left
            EmitRect( Rect<Pixel>::FromMinMax( Vec2<Pixel>{ tl[0], tl[1] + Pixel{ t } }, Vec2<Pixel>{ tl[0] + Pixel{ t }, bl[1] - Pixel{ t } } ), a_Color );
            // Right
            EmitRect( Rect<Pixel>::FromMinMax( Vec2<Pixel>{ tr[0] - Pixel{ t }, tr[1] + Pixel{ t } }, Vec2<Pixel>{ tr[0], br[1] - Pixel{ t } } ), a_Color );
        }

        /** 
         * @brief Pushes a rounded rectangle into the batcher, creating the necessary vertices and indices for rendering. 
         * The rounded rectangle is created by emitting a center rectangle, four edge rectangles, and four corner fans based on the specified corner rounding.
         * @param a_Rect The rectangle to render, defined by its origin and size.
         * @param a_Rounding The rounding values for each corner of the rectangle.
         * @param a_Color The color to use for the rectangle.
         */
        void EmitRoundedRect( const Rect<Pixel>& a_Rect, CornerRounding a_Rounding, Coloru8 a_Color )
        {
            // TODO: Add UV's

            constexpr u32 c_Segments   = 8;
            constexpr u32 c_MaxVerts   = 5 * 4 + 4 * ( 1 + c_Segments + 1 );
            constexpr u32 c_MaxIndices = 5 * 6 + 4 * c_Segments * 3;
            constexpr f32 c_PI_2       = Pi<f32> / 2.f;

            const u32 vertexOffset = static_cast<u32>( Size( Vertices ) );

            auto vertices  = ReserveVertices( c_MaxVerts );
            auto indices   = ReserveIndices( c_MaxIndices );
            u32  vertCount = 0;
            u32  idxCount  = 0;

            const f32 x = a_Rect.Origin[0].ToFloat();
            const f32 y = a_Rect.Origin[1].ToFloat();
            const f32 w = a_Rect.Size[0].ToFloat();
            const f32 h = a_Rect.Size[1].ToFloat();

            f32 tlRadius = a_Rounding.TopLeft.Value;
            f32 trRadius = a_Rounding.TopRight.Value;
            f32 brRadius = a_Rounding.BottomRight.Value;
            f32 blRadius = a_Rounding.BottomLeft.Value;

            // Scale down radii if they exceed the rect dimensions.
            {
                f32 scaleX = w / std::max( 1.f, tlRadius + trRadius );
                f32 scaleY = h / std::max( 1.f, tlRadius + blRadius );
                scaleX     = std::min( scaleX, w / std::max( 1.f, blRadius + brRadius ) );
                scaleY     = std::min( scaleY, h / std::max( 1.f, trRadius + brRadius ) );
                f32 scale  = std::min( 1.f, std::min( scaleX, scaleY ) );
                tlRadius  *= scale; trRadius *= scale; brRadius *= scale; blRadius *= scale;
            }

            auto pushVertex = [&]( f32 px, f32 py, f32 u = 0.f, f32 v = 0.f ) -> u32
            {
                vertices[vertCount] = Vertex{ Vec2<Pixel>{ Pixel{ px }, Pixel{ py } }, a_Color, Vec2f{ u, v } };
                return vertexOffset + vertCount++;
            };

            auto pushQuad = [&]( u32 tl, u32 tr, u32 br, u32 bl )
            {
                indices[idxCount++] = tl; indices[idxCount++] = tr; indices[idxCount++] = br;
                indices[idxCount++] = tl; indices[idxCount++] = br; indices[idxCount++] = bl;
            };

            // Center rect
            {
                u32 tl = pushVertex( x + tlRadius,     y + tlRadius     );
                u32 tr = pushVertex( x + w - trRadius, y + trRadius     );
                u32 br = pushVertex( x + w - brRadius, y + h - brRadius );
                u32 bl = pushVertex( x + blRadius,     y + h - blRadius );
                pushQuad( tl, tr, br, bl );
            }

            // Top strip
            {
                u32 tl = pushVertex( x + tlRadius,     y            );
                u32 tr = pushVertex( x + w - trRadius, y            );
                u32 br = pushVertex( x + w - trRadius, y + trRadius );
                u32 bl = pushVertex( x + tlRadius,     y + tlRadius );
                pushQuad( tl, tr, br, bl );
            }

            // Bottom strip
            {
                u32 tl = pushVertex( x + blRadius,     y + h - blRadius );
                u32 tr = pushVertex( x + w - brRadius, y + h - brRadius );
                u32 br = pushVertex( x + w - brRadius, y + h            );
                u32 bl = pushVertex( x + blRadius,     y + h            );
                pushQuad( tl, tr, br, bl );
            }

            // Left strip
            {
                u32 tl = pushVertex( x,            y + tlRadius     );
                u32 tr = pushVertex( x + tlRadius, y + tlRadius     );
                u32 br = pushVertex( x + blRadius, y + h - blRadius );
                u32 bl = pushVertex( x,            y + h - blRadius );
                pushQuad( tl, tr, br, bl );
            }

            // Right strip
            {
                u32 tl = pushVertex( x + w - trRadius, y + trRadius     );
                u32 tr = pushVertex( x + w,            y + trRadius     );
                u32 br = pushVertex( x + w,            y + h - brRadius );
                u32 bl = pushVertex( x + w - brRadius, y + h - brRadius );
                pushQuad( tl, tr, br, bl );
            }

            auto addCorner = [&]( f32 cx, f32 cy, f32 startAngle, f32 r )
            {
                u32 center = pushVertex( cx, cy );
                f32 step   = c_PI_2 / c_Segments;
                u32 prev   = static_cast<u32>( -1 );

                for ( u32 s = 0; s <= c_Segments; ++s )
                {
                    f32 angle = startAngle + s * step;
                    u32 cur   = pushVertex( cx + cosf( angle ) * r, cy + sinf( angle ) * r );
                    
                    if ( s > 0 ) 
                    { 
                        indices[idxCount++] = center; 
                        indices[idxCount++] = prev; 
                        indices[idxCount++] = cur; 
                    }

                    prev = cur;
                }
            };

            addCorner( x+tlRadius,     y+tlRadius,     Pi<f32>,        tlRadius );
            addCorner( x+w-trRadius,   y+trRadius,     Pi<f32> * 1.5f, trRadius );
            addCorner( x+w-brRadius,   y+h-brRadius,   0.f,            brRadius );
            addCorner( x+blRadius,     y+h-blRadius,   c_PI_2,         blRadius );
        }

        /** 
         * @brief Pushes a rounded rectangle border into the batcher, creating the necessary vertices and indices for rendering. 
         * The border is created by emitting four rounded rectangles (one for each corner) and four edge rectangles with the specified thickness.
         * @param a_Rect The rectangle to render the border for, defined by its origin and size.
         * @param a_Rounding The rounding values for each corner of the rectangle.
         * @param a_Color The color to use for the border.
         * @param a_Thickness The thickness of the border in pixels.
         */
        void EmitRoundedRectBorder( const Rect<Pixel>& a_Rect, CornerRounding a_Rounding, Coloru8 a_Color, f32 a_Thickness )
        {
            // TODO
        }

        /** 
         * @brief Pushes a circle into the batcher, creating the necessary vertices and indices for rendering. 
         * The circle is approximated using a triangle fan with a specified number of segments.
         * @param a_Center The center position of the circle.
         * @param a_Radius The radius of the circle.
         * @param a_Color The color to use for the circle.
         */
        void EmitCircle( const Vec2<Pixel>& a_Center, f32 a_Radius, Coloru8 a_Color )
        {
            constexpr u32 c_Segments   = 32;
            constexpr u32 c_MaxVerts   = 1 + c_Segments;
            constexpr u32 c_MaxIndices = c_Segments * 3;

            const u32 vertexOffset = static_cast<u32>( Size( Vertices ) );

            auto vertices  = ReserveVertices( c_MaxVerts );
            auto indices   = ReserveIndices( c_MaxIndices );
            u32  vertCount = 0;
            u32  idxCount  = 0;

            auto pushVertex = [&]( f32 x, f32 y, f32 u = 0.f, f32 v = 0.f ) -> u32
            {
                vertices[vertCount++] = Vertex{ Vec2<Pixel>{ Pixel{ x }, Pixel{ y } }, a_Color, Vec2f{ u, v } };
                return vertexOffset + vertCount - 1;
            };

            u32 center = pushVertex( a_Center[0].ToFloat(), a_Center[1].ToFloat() );
            f32 step   = ( Pi<f32> * 2.f ) / c_Segments;

            for ( u32 s = 0; s < c_Segments; ++s )
            {
                f32 angle = s * step;
                pushVertex( a_Center[0].ToFloat() + cosf( angle ) * a_Radius,
                            a_Center[1].ToFloat() + sinf( angle ) * a_Radius,
                            ( cosf( angle ) + 1.f ) * 0.5f, ( sinf( angle ) + 1.f ) * 0.5f );
            }

            for ( u32 s = 0; s < c_Segments; ++s )
            {
                indices[idxCount++] = static_cast<u16>( center );
                indices[idxCount++] = static_cast<u16>( vertexOffset + 1 + s  );
                indices[idxCount++] = static_cast<u16>( vertexOffset + 1 + ( s + 1 ) % c_Segments );
            }
        }

        /**
         * @brief Pushes a circle border into the batcher, creating the necessary vertices and indices for rendering.
         * The border is created by emitting a triangle strip that forms a ring between the outer and
         * inner radii defined by the specified thickness.
         * @param a_Center The center position of the circle.
         * @param a_Radius The radius of the circle.
         * @param a_Color The color to use for the circle border.
         * @param a_Thickness The thickness of the border in pixels.
         */
        void EmitCircleBorder( const Vec2<Pixel>& a_Center, f32 a_Radius, Coloru8 a_Color, f32 a_Thickness )
        {
            constexpr u32 c_Segments   = 32;
            constexpr u32 c_MaxVerts   = c_Segments * 2;
            constexpr u32 c_MaxIndices = c_Segments * 6;

            const f32 rOuter = a_Radius;
            const f32 rInner = std::max( 0.f, a_Radius - a_Thickness );

            const u32 vertexOffset = static_cast<u32>( Size( Vertices ) );

            auto vertices  = ReserveVertices( c_MaxVerts );
            auto indices   = ReserveIndices( c_MaxIndices );
            u32  vertCount = 0;
            u32  idxCount  = 0;

            auto pushVertex = [&]( f32 x, f32 y )
            {
                vertices[vertCount++] = Vertex{ Vec2<Pixel>{ Pixel{ x }, Pixel{ y } }, a_Color, Vec2f{ 0.f, 0.f } };
            };

            f32 step = ( Pi<f32> * 2.f ) / c_Segments;
            for ( u32 s = 0; s < c_Segments; ++s )
            {
                f32 angle = s * step;
                f32 cosA  = cosf( angle );
                f32 sinA  = sinf( angle );
                pushVertex( a_Center[0].ToFloat() + cosA * rOuter, a_Center[1].ToFloat() + sinA * rOuter );
                pushVertex( a_Center[0].ToFloat() + cosA * rInner, a_Center[1].ToFloat() + sinA * rInner );
            }

            for ( u32 s = 0; s < c_Segments; ++s )
            {
                u16 o0 = static_cast<u16>( vertexOffset + s * 2 );
                u16 i0 = static_cast<u16>( vertexOffset + s * 2 ) + 1;
                u16 o1 = static_cast<u16>( vertexOffset + ( ( s + 1 ) % c_Segments ) * 2 );
                u16 i1 = static_cast<u16>( vertexOffset + ( ( s + 1 ) % c_Segments ) * 2 ) + 1;

                indices[idxCount++] = o0; indices[idxCount++] = o1; indices[idxCount++] = i1;
                indices[idxCount++] = o0; indices[idxCount++] = i1; indices[idxCount++] = i0;
            }
        }

        /**
         * @brief Emits glyph quads for a block of shaped text into the vertex/index buffers.
         */
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
			const Pixel textWidth  = ToPixel( a_Text.MaxWidth, a_DpiScale );
			const Pixel textHeight = ToPixel( a_Text.TotalHeight, a_DpiScale );
			const Pixel ascender   = ToPixel( a_Text.Ascender, a_DpiScale );

            // baseSize is the atlas raster size in pixels. Dividing atlas pixel dimensions
            // by baseSize gives EM-normalised glyph extents, which are then scaled by
            // fontSizePx to get on-screen pixel sizes.
            const f32 baseSize   = a_Atlas.GetConfig().BaseSize.ToFloat();
            const f32 rcpBase    = baseSize > 0.f ? 1.f / baseSize : 0.f;
			const f32 fontSizePx = ToPixel( fontSize, a_DpiScale ).ToFloat();

            const Pixel layoutLeft  = a_LayoutRect.Origin[0];
            const Pixel layoutRight = a_LayoutRect.Origin[0] + a_LayoutRect.Size[0];
            const f32   fadePct     = std::clamp( a_Style.FadePercentage, 0.0f, 1.0f );
            const Pixel fadeWidth   = a_LayoutRect.Size[0] * fadePct;
            const Pixel fadeStartX  = layoutRight - fadeWidth;
            const Pixel fadeEndX    = layoutRight;

            auto computeFadeAlpha = [&]( Pixel x ) -> u8
            {
                if ( fadePct <= 0.f )  return a_Style.FillColor[3];
                if ( x <= fadeStartX ) return a_Style.FillColor[3];
                if ( x >= fadeEndX )   return 0;
            
                const f32 t = ( x - fadeStartX ).ToFloat() / ( fadeEndX - fadeStartX ).ToFloat();
                const f32 alpha = ( 1.0f - t ) * a_Style.FillColor[3];
            
                return static_cast<u8>( std::clamp( alpha, 0.0f, 255.0f ) );
            };

            // Compute baseline position
            Pixel baselineY = a_LayoutRect.Origin[1];

            switch ( a_Style.Baseline )
            {
                case ETextBaseline::Top:
                    // Top of text box aligned to layout top
                    baselineY += ascender;
                    break;
            
                case ETextBaseline::Middle:
                    baselineY += ( a_LayoutRect.Size[1] - textHeight ) * 0.5f;
                    baselineY += ascender;
                    break;
            
                case ETextBaseline::Bottom:
                    baselineY += a_LayoutRect.Size[1] - textHeight;
                    baselineY += ascender;
                    break;
            
                case ETextBaseline::Hanging:
                    // Ascender line aligned to layout top
                    baselineY += ascender;
                    break;
            
                case ETextBaseline::Alphabetic:
                default:
                    // Baseline aligned directly to layout top
                    break;
            }

            Pixel penY = baselineY;

            for ( u32 lineIdx = 0; lineIdx < a_Text.LineCount(); ++lineIdx )
            {
                const ShapedLine& line = a_Text.Lines[ lineIdx ];

                // Horizontal alignment offset.
                Pixel lineX = a_LayoutRect.Origin[0];
                switch ( a_Style.Align )
                {
                    case ETextAlign::Center:  lineX += ( a_LayoutRect.Size[0] - ToPixel( line.Width, a_DpiScale ) ) * 0.5f; break;
					case ETextAlign::Right:   lineX +=   a_LayoutRect.Size[0] - ToPixel( line.Width, a_DpiScale );          break;
					case ETextAlign::Justify: // TODO: Distribute extra space between words for justified text. Requires word boundary information in ShapedLine.
                        break;
                    case ETextAlign::Left:   
                    default: 
                        break;
                }

                Pixel penX = lineX;

                for ( u32 g = line.Start; g < line.End; ++g )
                {
                    const ShapedGlyph& sg = a_Text.Glyphs[ g ];

                    // Get the glyph in the atlas.
                    Optional<GlyphMetrics> gr = a_Atlas.GetOrRasterizeGlyph( a_Text.Font, sg.Codepoint );

                    if ( gr && gr->AtlasRect.Size[0] > 0 && gr->AtlasRect.Size[1] > 0 )
                    {
                        // Glyph position in screen pixels.
                        const Pixel gx = penX + ToPixel( sg.XOffset + gr->Bearing[0], fontSize, a_DpiScale );
                        const Pixel gy = penY + ToPixel( sg.YOffset - gr->Bearing[1], fontSize, a_DpiScale );

                        // On-screen glyph size in pixels.
						// gr->AtlasRect.Size is in atlas pixels.
                        // Divide by baseSize to get EM-normalised units, then multiply by fontSizePx to get display pixels.
                        const Pixel gw = static_cast<Pixel>( gr->AtlasRect.Size[0] ) * rcpBase * fontSizePx;
                        const Pixel gh = static_cast<Pixel>( gr->AtlasRect.Size[1] ) * rcpBase * fontSizePx;

                        // Atlas UV coordinates.
                        const f32 u0 = static_cast<f32>( gr->AtlasRect.Origin[0]                         ) * rcpAtlasW;
                        const f32 v0 = static_cast<f32>( gr->AtlasRect.Origin[1]                         ) * rcpAtlasH;
                        const f32 u1 = static_cast<f32>( gr->AtlasRect.Origin[0] + gr->AtlasRect.Size[0] ) * rcpAtlasW;
                        const f32 v1 = static_cast<f32>( gr->AtlasRect.Origin[1] + gr->AtlasRect.Size[1] ) * rcpAtlasH;

                        // If the glyph is in the fade region, we need to lerp the fade color based on the distance to the fade edge. 
                        // This creates a smooth fade-out effect for glyphs that are partially outside the layout rect.
                        const Coloru8 colorA = { a_Style.FillColor[0], a_Style.FillColor[1], a_Style.FillColor[2], computeFadeAlpha( gx ) };
                        const Coloru8 colorB = { a_Style.FillColor[0], a_Style.FillColor[1], a_Style.FillColor[2], computeFadeAlpha( gx + gw ) };

                        const u32 vertexOffset = static_cast<u32>( Size( Vertices ) );
                        auto verts = ReserveVertices( 4 );
                        verts[0] = Vertex{ Vec2<Pixel>{ Pixel{ gx      }, Pixel{ gy      } }, colorA, Vec2f{ u0, v0 } };
                        verts[1] = Vertex{ Vec2<Pixel>{ Pixel{ gx + gw }, Pixel{ gy      } }, colorB, Vec2f{ u1, v0 } };
                        verts[2] = Vertex{ Vec2<Pixel>{ Pixel{ gx      }, Pixel{ gy + gh } }, colorA, Vec2f{ u0, v1 } };
                        verts[3] = Vertex{ Vec2<Pixel>{ Pixel{ gx + gw }, Pixel{ gy + gh } }, colorB, Vec2f{ u1, v1 } };

                        auto idx = ReserveIndices( 6 );
                        idx[0] = vertexOffset + 0; idx[1] = vertexOffset + 1; idx[2] = vertexOffset + 2;
                        idx[3] = vertexOffset + 1; idx[4] = vertexOffset + 3; idx[5] = vertexOffset + 2;
                    }

                    // Advance the pen by the glyph's horizontal advance.
                    // sg.XAdvance is EM-normalised; multiply by fontSizePx to get pixels.
					penX += ToPixel( sg.XAdvance, fontSize, a_DpiScale );
                }

                // Advance to the next line.
				penY += ToPixel( a_Text.LineHeight, a_DpiScale );
            }
        }
    };

} // namespace RatUI
