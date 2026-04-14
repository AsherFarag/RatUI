#pragma once
#include "../../RatUI.h"
#include "../FreeType/GlyphAtlas.h"
#include "../FreeType/FontCache.h"
#include "../FreeType/TextUtil.h"
#include <SDL.h>

namespace RatUI::SDL2
{
    /**
     * @brief SDL2-backed renderer.
     */
    class SDL2Renderer : public IRenderer
    {
    public:
        SDL2Renderer() = default;
        SDL2Renderer( SDL_Renderer* a_Renderer, FreeType::FontCache* a_FontCache, FreeType::GlyphAtlas* a_GlyphAtlas = nullptr )
            : m_Renderer  ( a_Renderer )
            , m_FontCache ( a_FontCache )
            , m_GlyphAtlas( a_GlyphAtlas )
        {
            Reserve( m_VertexScratch, 1024 * 4 ); // Avoid initial allocations for small draw calls.
            Reserve( m_IndexScratch,  1024 * 6 );
        }

        SDL_Renderer*         GetSDLRenderer() const { return m_Renderer; }
        FreeType::FontCache*  GetFontCache()   const { return m_FontCache; }
        FreeType::GlyphAtlas* GetGlyphAtlas()  const { return m_GlyphAtlas; }

        void SetSDLRenderer( SDL_Renderer* a_Renderer )     { m_Renderer   = a_Renderer; }
        void SetFontCache( FreeType::FontCache* a_Cache )   { m_FontCache  = a_Cache;    }
        void SetGlyphAtlas( FreeType::GlyphAtlas* a_Atlas ) { m_GlyphAtlas = a_Atlas;    }

        void Execute( Span<const DrawCmd> a_Commands ) override;

        Optional<TextureID> CreateTexture( u32 a_Width, u32 a_Height, ETextureFormat a_Format, const void* a_Data ) override;
        bool UpdateTexture( TextureID a_Texture, u32 a_MipLevel, Rectu a_Region, const void* a_Data, size a_DataSizeBytes ) override;
        void DestroyTexture( TextureID a_Texture ) override;
        bool IsValidTexture( TextureID a_Texture ) const override;

        void RenderFillRoundedRect(
            Rectf a_Rect, CornerRounding a_Rounding, Colorf a_Color, const Mat3f& a_Transform );

        void RenderFillRoundedRectBorder(
            Rectf a_Rect, CornerRounding a_Rounding, Colorf a_Color, f32 a_Thickness, const Mat3f& a_Transform );

        void RenderFillCircle(
            Vec2f a_Center, f32 a_Radius, Colorf a_Color, const Mat3f& a_Transform );

        void RenderFillCircleBorder(
            Vec2f a_Center, f32 a_Radius, Colorf a_Color, f32 a_Thickness, const Mat3f& a_Transform );

        void RenderShapedText(
            const ShapedText& a_ShapedText, const TextRenderStyle& a_Style, Rectf a_Rect, const Mat3f& a_Transform );

        // === Helpers

        static SDL_FPoint TransformPoint( f32 a_X, f32 a_Y, const Mat3f& a_Transform )
        {
            Vec3f p = a_Transform * Vec3f{ a_X, a_Y, 1.f };
            return SDL_FPoint{ p[0], p[1] };
        };

        static SDL_Color ToSDLColor( Colorf a_Color )
        {
            return SDL_Color{
                static_cast<Uint8>( a_Color[0] * 255.f ),
                static_cast<Uint8>( a_Color[1] * 255.f ),
                static_cast<Uint8>( a_Color[2] * 255.f ),
                static_cast<Uint8>( a_Color[3] * 255.f )
            };
        }

        static SDL_Color ToSDLColor( Coloru8 a_Color )
        {
            return SDL_Color{ a_Color[0], a_Color[1], a_Color[2], a_Color[3] };
        }

    protected:
        SDL_Renderer*         m_Renderer  { nullptr };
        FreeType::FontCache*  m_FontCache { nullptr };
        FreeType::GlyphAtlas* m_GlyphAtlas{ nullptr };

        // Persistent scratch buffer for R8->RGBA expansion in UpdateTexture.
        // Grows as needed but never shrinks - avoids per-upload allocations.
        Array<u8> m_RGBAScratch;

        Array<SDL_Vertex> m_VertexScratch; ///< Scratch buffer for vertex data, to avoid per-render allocations.
        Array<int>        m_IndexScratch;  ///< Scratch buffer for index data, to avoid per-render allocations.
    };

    // =========================================================================
    // Inline implementations
    // =========================================================================

    inline void SDL2Renderer::Execute( Span<const DrawCmd> a_Commands )
    {
        if ( !m_Renderer )
            return;

        SDL_Rect prevClip;
        SDL_RenderGetClipRect( m_Renderer, &prevClip );

		SDL_BlendMode prevBlendMode;
		SDL_GetRenderDrawBlendMode( m_Renderer, &prevBlendMode );
        SDL_SetRenderDrawBlendMode( m_Renderer, SDL_BLENDMODE_BLEND );

        for ( const DrawCmd& cmd : a_Commands )
        {
            if ( cmd.ClipRect.IsInfinite() )
                SDL_RenderSetClipRect( m_Renderer, nullptr );
            else
            {
                SDL_Rect sdlClip{
                    static_cast<int>( cmd.ClipRect.Origin[0] ),
                    static_cast<int>( cmd.ClipRect.Origin[1] ),
                    static_cast<int>( cmd.ClipRect.Size[0] ),
                    static_cast<int>( cmd.ClipRect.Size[1] )
                };
                SDL_RenderSetClipRect( m_Renderer, &sdlClip );
            }

            if      ( Holds<DrawCmd::RectCmd>        ( cmd.Payload ) ) { const auto& c = Get<DrawCmd::RectCmd>        ( cmd.Payload ); RenderFillRoundedRect( c.Rect, c.Rounding, c.Color, cmd.Transform ); }
            else if ( Holds<DrawCmd::RectBorderCmd>  ( cmd.Payload ) ) { const auto& c = Get<DrawCmd::RectBorderCmd>  ( cmd.Payload ); RenderFillRoundedRectBorder( c.Rect, c.Rounding, c.Color, c.Thickness, cmd.Transform ); }
            else if ( Holds<DrawCmd::CircleCmd>      ( cmd.Payload ) ) { const auto& c = Get<DrawCmd::CircleCmd>      ( cmd.Payload ); RenderFillCircle( c.Center, c.Radius, c.Color, cmd.Transform ); }
            else if ( Holds<DrawCmd::CircleBorderCmd>( cmd.Payload ) ) { const auto& c = Get<DrawCmd::CircleBorderCmd>( cmd.Payload ); RenderFillCircleBorder( c.Center, c.Radius, c.Color, c.Thickness, cmd.Transform ); }
            else if ( Holds<DrawCmd::ShapedTextCmd>  ( cmd.Payload ) ) { const auto& c = Get<DrawCmd::ShapedTextCmd>  ( cmd.Payload ); if ( c.Shaped ) RenderShapedText( *c.Shaped, c.Style, c.Rect, cmd.Transform ); }
            else if ( Holds<DrawCmd::CustomCmd>      ( cmd.Payload ) ) { const auto& c = Get<DrawCmd::CustomCmd>      ( cmd.Payload ); if ( c.Func ) c.Func( *this, cmd ); }
        }

		SDL_SetRenderDrawBlendMode( m_Renderer, prevBlendMode );
        SDL_RenderSetClipRect( m_Renderer, &prevClip );
    }

    inline Optional<TextureID> SDL2Renderer::CreateTexture( u32 a_Width, u32 a_Height, ETextureFormat a_Format, const void* a_Data )
    {
        if ( !m_Renderer )
            return NullOpt;

        SDL_Texture* tex = SDL_CreateTexture(
            m_Renderer,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STREAMING,
            static_cast<int>( a_Width ),
            static_cast<int>( a_Height ) );

        if ( !tex )
            return NullOpt;

        SDL_SetTextureBlendMode( tex, SDL_BLENDMODE_BLEND );

        if ( a_Data )
        {
            const Rectu fullRegion{ Vec2u{ 0u, 0u }, Vec2u{ a_Width, a_Height } };
            const size  dataSize = static_cast<size>( a_Width ) * a_Height
                                   * ( a_Format == ETextureFormat::RGBA8 ? 4u : 1u );
            UpdateTexture( TextureID{ .Ptr = tex }, 0, fullRegion, a_Data, dataSize );
        }

        TextureID id;
        id.Ptr = tex;
        return id;
    }

    inline bool SDL2Renderer::UpdateTexture(
        TextureID   a_Texture,
        u32         /*a_MipLevel*/,
        Rectu       a_Region,
        const void* a_Data,
        size        a_DataSizeBytes )
    {
        SDL_Texture* tex = static_cast<SDL_Texture*>( a_Texture.Ptr );
        if ( !tex || !a_Data )
            return false;

        const u32 w = a_Region.Width();
        const u32 h = a_Region.Height();
        if ( w == 0 || h == 0 )
            return true;

        RATUI_USER_ASSERT( a_DataSizeBytes == static_cast<size>( w ) * h ||
                           a_DataSizeBytes == static_cast<size>( w ) * h * 4u,
                           "UpdateTexture: a_DataSizeBytes must match R8 (w*h) or RGBA8 (w*h*4)." );

        const bool isR8 = ( a_DataSizeBytes == static_cast<size>( w ) * h );

        SDL_Rect rect{
            static_cast<int>( a_Region.Origin[0] ),
            static_cast<int>( a_Region.Origin[1] ),
            static_cast<int>( w ),
            static_cast<int>( h )
        };

        if ( isR8 )
        {
            // Expand R8 -> RGBA using the persistent scratch buffer.
            // R=255, G=255, B=255, A=source_byte lets vertex colour tint the glyph.
            const size needed = static_cast<size>( w ) * h * 4u;
            if ( Size( m_RGBAScratch ) < needed )
                Resize( m_RGBAScratch, needed );

            const u8* src = static_cast<const u8*>( a_Data );
            u8*       dst = Data( m_RGBAScratch );
            
            std::memset(dst, 0xFF, needed);  // set all bytes to 255
            for (u32 i = 0; i < w * h; ++i)
                dst[i * 4 + 3] = src[i];    // then overwrite alpha channel only

            return SDL_UpdateTexture( tex, &rect, dst, static_cast<int>( w ) * 4 ) == 0;
        }
        else
        {
            return SDL_UpdateTexture( tex, &rect, a_Data, static_cast<int>( w ) * 4 ) == 0;
        }
    }

    inline void SDL2Renderer::DestroyTexture( TextureID a_Texture )
    {
        if ( SDL_Texture* tex = static_cast<SDL_Texture*>( a_Texture.Ptr ) )
            SDL_DestroyTexture( tex );
    }

    inline bool SDL2Renderer::IsValidTexture( TextureID a_Texture ) const
    {
        return a_Texture.Ptr != nullptr;
    }

    inline void SDL2Renderer::RenderFillRoundedRect(
        Rectf a_Rect, CornerRounding a_Rounding, Colorf a_Color, const Mat3f& a_Transform )
    {
        constexpr int c_Segments   = 8;
        constexpr int c_MaxVerts   = 5 * 4 + 4 * ( 1 + c_Segments + 1 );
        constexpr int c_MaxIndices = 5 * 6 + 4 * c_Segments * 3;
        constexpr f32 c_PI_2       = Pi<f32> / 2.f;

        const SDL_Color sdlColor = ToSDLColor( a_Color );

        SDL_Vertex vertices[c_MaxVerts];
        int        indices[c_MaxIndices];
        int        vertCount = 0;
        int        idxCount  = 0;

        const f32 x = a_Rect.Origin[0];
        const f32 y = a_Rect.Origin[1];
        const f32 w = a_Rect.Size[0];
        const f32 h = a_Rect.Size[1];

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

        auto pushVertex = [&]( f32 px, f32 py ) -> int
        {
            vertices[vertCount] = { TransformPoint( px, py, a_Transform ), sdlColor, { 0.f, 0.f } };
            return vertCount++;
        };

        auto pushQuad = [&]( int tl, int tr, int br, int bl )
        {
            indices[idxCount++] = tl; indices[idxCount++] = tr; indices[idxCount++] = br;
            indices[idxCount++] = tl; indices[idxCount++] = br; indices[idxCount++] = bl;
        };

        // Center rect
        {
            int tl = pushVertex( x + tlRadius,     y + tlRadius     );
            int tr = pushVertex( x + w - trRadius, y + trRadius     );
            int br = pushVertex( x + w - brRadius, y + h - brRadius );
            int bl = pushVertex( x + blRadius,     y + h - blRadius );
            pushQuad( tl, tr, br, bl );
        }

        // Top strip
        {
            int tl = pushVertex( x + tlRadius,     y            );
            int tr = pushVertex( x + w - trRadius, y            );
            int br = pushVertex( x + w - trRadius, y + trRadius );
            int bl = pushVertex( x + tlRadius,     y + tlRadius );
            pushQuad( tl, tr, br, bl );
        }

        // Bottom strip
        {
            int tl = pushVertex( x + blRadius,     y + h - blRadius );
            int tr = pushVertex( x + w - brRadius, y + h - brRadius );
            int br = pushVertex( x + w - brRadius, y + h            );
            int bl = pushVertex( x + blRadius,     y + h            );
            pushQuad( tl, tr, br, bl );
        }

        // Left strip
        {
            int tl = pushVertex( x,            y + tlRadius     );
            int tr = pushVertex( x + tlRadius, y + tlRadius     );
            int br = pushVertex( x + blRadius, y + h - blRadius );
            int bl = pushVertex( x,            y + h - blRadius );
            pushQuad( tl, tr, br, bl );
        }

        // Right strip
        {
            int tl = pushVertex( x + w - trRadius, y + trRadius     );
            int tr = pushVertex( x + w,            y + trRadius     );
            int br = pushVertex( x + w,            y + h - brRadius );
            int bl = pushVertex( x + w - brRadius, y + h - brRadius );
            pushQuad( tl, tr, br, bl );
        }

        auto addCorner = [&]( f32 cx, f32 cy, f32 startAngle, f32 r )
        {
            int center = pushVertex( cx, cy );
            f32 step   = c_PI_2 / c_Segments;
            int prev   = -1;
            for ( int s = 0; s <= c_Segments; ++s )
            {
                f32 angle = startAngle + s * step;
                int cur   = pushVertex( cx + cosf( angle ) * r, cy + sinf( angle ) * r );
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

        SDL_RenderGeometry( m_Renderer, nullptr, vertices, vertCount, indices, idxCount );
    }

    inline void SDL2Renderer::RenderFillRoundedRectBorder(
        Rectf /*a_Rect*/, CornerRounding /*a_Rounding*/, Colorf /*a_Color*/, f32 /*a_Thickness*/, const Mat3f& /*a_Transform*/ )
    {
        // TODO: implement rounded rect border rendering
    }

    inline void SDL2Renderer::RenderFillCircle(
        Vec2f a_Center, f32 a_Radius, Colorf a_Color, const Mat3f& a_Transform )
    {
        constexpr int c_Segments   = 32;
        constexpr int c_MaxVerts   = 1 + c_Segments;
        constexpr int c_MaxIndices = c_Segments * 3;

        const SDL_Color sdlColor = ToSDLColor( a_Color );
        SDL_Vertex vertices[c_MaxVerts];
        int        indices[c_MaxIndices];
        int        vertCount = 0;
        int        idxCount  = 0;

        auto pushVertex = [&]( f32 px, f32 py ) -> int
        {
            vertices[vertCount] = { TransformPoint( px, py, a_Transform ), sdlColor, { 0.f, 0.f } };
            return vertCount++;
        };

        int center = pushVertex( a_Center[0], a_Center[1] );
        f32 step   = ( Pi<f32> * 2.f ) / c_Segments;

        for ( int s = 0; s < c_Segments; ++s )
        {
            f32 angle = s * step;
            pushVertex( a_Center[0] + cosf( angle ) * a_Radius,
                        a_Center[1] + sinf( angle ) * a_Radius );
        }

        for ( int s = 0; s < c_Segments; ++s )
        {
            indices[idxCount++] = center;
            indices[idxCount++] = 1 + s;
            indices[idxCount++] = 1 + ( s + 1 ) % c_Segments;
        }

        SDL_RenderGeometry( m_Renderer, nullptr, vertices, vertCount, indices, idxCount );
    }

    inline void SDL2Renderer::RenderFillCircleBorder(
        Vec2f a_Center, f32 a_Radius, Colorf a_Color, f32 a_Thickness, const Mat3f& a_Transform )
    {
        constexpr int c_Segments   = 32;
        constexpr int c_MaxVerts   = c_Segments * 2;
        constexpr int c_MaxIndices = c_Segments * 6;

        const SDL_Color sdlColor = ToSDLColor( a_Color );
        const f32       rOuter   = a_Radius;
        const f32       rInner   = std::max( 0.f, a_Radius - a_Thickness );

        SDL_Vertex vertices[c_MaxVerts];
        int        indices[c_MaxIndices];
        int        vertCount = 0;
        int        idxCount  = 0;

        auto pushVertex = [&]( f32 px, f32 py ) -> int
        {
            vertices[vertCount] = { TransformPoint( px, py, a_Transform ), sdlColor, { 0.f, 0.f } };
            return vertCount++;
        };

        f32 step = ( Pi<f32> * 2.f ) / c_Segments;
        for ( int s = 0; s < c_Segments; ++s )
        {
            f32 angle = s * step;
            f32 cosA  = cosf( angle );
            f32 sinA  = sinf( angle );
            pushVertex( a_Center[0] + cosA * rOuter, a_Center[1] + sinA * rOuter );
            pushVertex( a_Center[0] + cosA * rInner, a_Center[1] + sinA * rInner );
        }

        for ( int s = 0; s < c_Segments; ++s )
        {
            int o0 = s * 2;
            int i0 = s * 2 + 1;
            int o1 = ( ( s + 1 ) % c_Segments ) * 2;
            int i1 = ( ( s + 1 ) % c_Segments ) * 2 + 1;

            indices[idxCount++] = o0; indices[idxCount++] = o1; indices[idxCount++] = i1;
            indices[idxCount++] = o0; indices[idxCount++] = i1; indices[idxCount++] = i0;
        }

        SDL_RenderGeometry( m_Renderer, nullptr, vertices, vertCount, indices, idxCount );
    }

    inline void SDL2Renderer::RenderShapedText(
        const ShapedText&      a_Shaped,
        const TextRenderStyle& a_Style,
        Rectf                  a_Rect,
        const Mat3f&           a_Transform )
    {
        if ( !m_Renderer || !m_GlyphAtlas )
            return;

        if ( Empty( a_Shaped.Lines ) || Empty( a_Shaped.Glyphs ) )
            return;

		if ( a_Rect.Size[0] <= 0.f || a_Rect.Size[1] <= 0.f ) // No point rendering if the rect has no area - text won't be visible anyway.
            return;

        const SDL_Color sdlColor     = ToSDLColor( a_Style.Color );
        SDL_Texture*    atlasTexture = static_cast<SDL_Texture*>( m_GlyphAtlas->GetTexture().Ptr );

        auto pushQuad = [&]( const Vec2f a_PosMin, const Vec2f a_PosMax, const Vec2f a_UVMin, const Vec2f a_UVMax, f32 a_Alpha )
        {
            SDL_Color col = sdlColor;
            col.a = static_cast<Uint8>( a_Alpha * sdlColor.a );

            EmplaceBack( m_VertexScratch, TransformPoint( a_PosMin[0], a_PosMin[1], a_Transform ), col, SDL_FPoint{ a_UVMin[0], a_UVMin[1] } );
            EmplaceBack( m_VertexScratch, TransformPoint( a_PosMax[0], a_PosMin[1], a_Transform ), col, SDL_FPoint{ a_UVMax[0], a_UVMin[1] } );
            EmplaceBack( m_VertexScratch, TransformPoint( a_PosMax[0], a_PosMax[1], a_Transform ), col, SDL_FPoint{ a_UVMax[0], a_UVMax[1] } );
            EmplaceBack( m_VertexScratch, TransformPoint( a_PosMin[0], a_PosMax[1], a_Transform ), col, SDL_FPoint{ a_UVMin[0], a_UVMax[1] } );

            int baseIndex = Size( m_VertexScratch ) - 4;
            PushBack( m_IndexScratch, baseIndex );
            PushBack( m_IndexScratch, baseIndex + 1 );
            PushBack( m_IndexScratch, baseIndex + 2 );
            PushBack( m_IndexScratch, baseIndex );
            PushBack( m_IndexScratch, baseIndex + 2 );
            PushBack( m_IndexScratch, baseIndex + 3 );                    
        };

        // 1: Compute initial Y position based on baseline alignment.

        f32 lineY = a_Rect.Origin[1];
        switch ( a_Style.Baseline )
        {
            case ETextBaseline::Middle:
                // Vertically center the text block within the rect.
                lineY += std::max( 0.f, ( a_Rect.Size[1] - a_Shaped.TotalHeight ) * 0.5f );
                break;
            case ETextBaseline::Bottom:
                // Pin the bottom of the text block to the bottom of the rect.
                lineY += std::max( 0.f, a_Rect.Size[1] - a_Shaped.TotalHeight );
                break;
            case ETextBaseline::Top:
            case ETextBaseline::Alphabetic:
            case ETextBaseline::Hanging:
            default:
                // Top of the text block at the top of the rect (default behaviour).
                break;
        }

        // 2: Iterate lines and append quads for glyphs to vertex/index scratch buffers

        for ( const ShapedLine& line : a_Shaped.Lines )
        {
            // Compute X offset for horizontal alignment.
            f32 xOffset = 0.f;
            switch ( a_Style.Align )
            {
                case ETextAlign::Center: xOffset = std::max( 0.f, ( a_Rect.Size[0] - line.Width ) * 0.5f ); break;
                case ETextAlign::Right:  xOffset = std::max( 0.f,   a_Rect.Size[0] - line.Width );          break;
                default: break;
            }

            const f32   lineX     = a_Rect.Origin[0] + xOffset;               // Left edge of the line's bounding box, in world space.
            const Vec2f origin    = { lineX, lineY + a_Shaped.Ascender };     // Baseline origin for the line, in world space.
            const f32   fadeWidth = a_Rect.Size[0] * a_Style.FadePercentage;  // Width of the fade-out region at the right end of the line, in world space.
            const f32   fadeStart = lineX + a_Rect.Size[0] - fadeWidth;       // X position where fade-out starts, in world space.
            const f32   fadeEnd   = lineX + a_Rect.Size[0];                   // X position where fade-out ends (fully transparent), in world space.

            // Helper to compute alpha for a given X position based on fade-out settings.
            // Returns 1.0f for fully opaque, 0.0f for fully transparent.
            auto computeFade = [&](f32 x) -> f32
            {
                if (x <= fadeStart)
                    return 1.0f;
            
                f32 t = (x - fadeStart) / fadeWidth;
                t = std::clamp(t, 0.0f, 1.0f);
                return 1.f - t;
            };

            // 2.1: Draw glyphs in the line as quads with texture coordinates from the glyph atlas, applying fade-out if enabled.

            // Add quads for each glyph in the line, applying fade-out if enabled and we're in the fade region.
            Span<const ShapedGlyph> lineGlyphs{ a_Shaped.Glyphs.data() + line.Start, line.End - line.Start };
            FreeType::RenderShapedTextLine(
                *m_GlyphAtlas, lineGlyphs, origin,
                [&]( const FreeType::GlyphQuad& q )
                {
                    const f32 alpha = computeFade( q.PosMin[0] + ( q.PosMax[0] - q.PosMin[0] ) * 0.5f );
                    pushQuad( q.PosMin, q.PosMax, q.UVMin, q.UVMax, alpha );
                } );

            // 2.2: Add decorations (underline, strikethrough) as horizontal quads, applying the same fade-out logic.

            // Text decorations (underline / strikethrough)
            if ( a_Style.Underline || a_Style.Strikethrough )
            {
                const f32 baselineY = origin[1];
                const f32 thickness = a_Shaped.UnderlineThickness;

                // Helper to draw a horizontal line with optional fade-out at the right end, used for both underline and strikethrough.
                const auto drawFadedLine = [&]( f32 y )
                {
                    const f32 lineStart = lineX;
                    const f32 lineEnd   = lineX + line.Width;
                
                    if (lineEnd <= lineStart)
                        return;
                
                    const f32 halfT = thickness * 0.5f;
                
                    const f32 solidEnd = std::min(fadeStart, lineEnd);
                    const f32 fadeEndX = std::min(fadeEnd, lineEnd);
                
                    SDL_Color transparent = sdlColor;
                    transparent.a = 0;
                
                    SDL_Vertex vertices[8];
                    int indices[12];
                    int v = 0;
                    int i = 0;
                
                    // Solid part of the line (fully opaque)
                    if ( solidEnd > lineStart )
                    {
                        vertices[v+0] = { TransformPoint( lineStart, y - halfT, a_Transform ), sdlColor, {0,0} };
                        vertices[v+1] = { TransformPoint( solidEnd,  y - halfT, a_Transform ), sdlColor, {0,0} };
                        vertices[v+2] = { TransformPoint( solidEnd,  y + halfT, a_Transform ), sdlColor, {0,0} };
                        vertices[v+3] = { TransformPoint( lineStart, y + halfT, a_Transform ), sdlColor, {0,0} };
                    
                        indices[i+0] = v+0; indices[i+1] = v+1; indices[i+2] = v+2;
                        indices[i+3] = v+0; indices[i+4] = v+2; indices[i+5] = v+3;
                    
                        v += 4;
                        i += 6;
                    }
                
                    // Fading part of the line (alpha interpolates to 0)
                    if ( fadeEndX > solidEnd )
                    {
                        vertices[v+0] = { TransformPoint( solidEnd, y - halfT, a_Transform ), sdlColor,    {0,0} };
                        vertices[v+1] = { TransformPoint( fadeEndX, y - halfT, a_Transform ), transparent, {0,0} };
                        vertices[v+2] = { TransformPoint( fadeEndX, y + halfT, a_Transform ), transparent, {0,0} };
                        vertices[v+3] = { TransformPoint( solidEnd, y + halfT, a_Transform ), sdlColor,    {0,0} };
                    
                        indices[i+0] = v+0; indices[i+1] = v+1; indices[i+2] = v+2;
                        indices[i+3] = v+0; indices[i+4] = v+2; indices[i+5] = v+3;
                    
                        v += 4;
                        i += 6;
                    }
                
                    SDL_RenderGeometry(m_Renderer, nullptr, vertices, v, indices, i);
                };

                if (a_Style.Underline)
                    drawFadedLine(baselineY + a_Shaped.UnderlinePosition);

                if (a_Style.Strikethrough)
                    drawFadedLine(baselineY - a_Shaped.Ascender * 0.35f);
            }

            lineY += a_Shaped.LineHeight;
        }

        // 3: Flush the vertex/index buffers to the GPU in one draw call, then clear them for the next batch.
        if ( !Empty( m_VertexScratch ) && !Empty( m_IndexScratch ) )
        {
            SDL_RenderGeometry( m_Renderer, atlasTexture, 
                Data( m_VertexScratch ), static_cast<int>( Size( m_VertexScratch ) ), 
                Data( m_IndexScratch ), static_cast<int>( Size( m_IndexScratch ) ) );
        }

        Clear( m_VertexScratch );
        Clear( m_IndexScratch );
    }

} // namespace RatUI::SDL2
