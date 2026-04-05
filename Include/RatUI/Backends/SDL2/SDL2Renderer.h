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
            : m_Renderer( a_Renderer )
            , m_FontCache( a_FontCache )
            , m_GlyphAtlas( a_GlyphAtlas )
        {}

        SDL_Renderer*         GetSDLRenderer() const { return m_Renderer; }
        FreeType::FontCache*  GetFontCache()   const { return m_FontCache; }
        FreeType::GlyphAtlas* GetGlyphAtlas()  const { return m_GlyphAtlas; }

        void SetSDLRenderer( SDL_Renderer* a_Renderer )       { m_Renderer   = a_Renderer; }
        void SetFontCache( FreeType::FontCache* a_Cache )     { m_FontCache  = a_Cache;    }
        void SetGlyphAtlas( FreeType::GlyphAtlas* a_Atlas )   { m_GlyphAtlas = a_Atlas;    }

        void Execute( Span<const DrawCmd> a_Commands ) override;

        // = IRenderer texture management

        Optional<TextureID> CreateTexture( u32 a_Width, u32 a_Height, ETextureFormat a_Format, const void* a_Data ) override;
        bool UpdateTexture( TextureID a_Texture, u32 a_MipLevel, Rectu a_Region, const void* a_Data, size a_DataSizeBytes ) override;
        void DestroyTexture( TextureID a_Texture ) override;
        bool IsValidTexture( TextureID a_Texture ) const override;

        // = Geometry rendering

        void RenderFillRoundedRect(
            Rectf          a_Rect,
            CornerRounding a_Rounding,
            Colorf         a_Color,
            const Mat3f&   a_Transform );

        void RenderFillRoundedRectBorder(
            Rectf          a_Rect,
            CornerRounding a_Rounding,
            Colorf         a_Color,
            f32            a_Thickness,
            const Mat3f&   a_Transform );

        void RenderFillCircle(
            Vec2f        a_Center,
            f32          a_Radius,
            Colorf       a_Color,
            const Mat3f& a_Transform );

        void RenderFillCircleBorder(
            Vec2f        a_Center,
            f32          a_Radius,
            Colorf       a_Color,
            f32          a_Thickness,
            const Mat3f& a_Transform );

        void RenderText(
            TextView         a_Text,
            const TextStyle& a_Style,
            Rectf            a_Rect,
            const Mat3f&     a_Transform );

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
            return SDL_Color{
                a_Color[0],
                a_Color[1],
                a_Color[2],
                a_Color[3]
            };
        }

    protected:
        SDL_Renderer*         m_Renderer  { nullptr };
        FreeType::FontCache*  m_FontCache { nullptr };
        FreeType::GlyphAtlas* m_GlyphAtlas{ nullptr };
    };

    // = Inline Implementations

    inline void SDL2Renderer::Execute( Span<const DrawCmd> a_Commands )
    {
        if ( !m_Renderer )
            return; // Early out if we don't have a valid SDL_Renderer to draw with.

        // TODO: Capture the current SDL_Renderer state (blend mode, draw color, etc) and restore it at the end of this function.

        SDL_Rect prevClip;

        SDL_RenderGetClipRect( m_Renderer, &prevClip );

        for ( const DrawCmd& cmd : a_Commands )
        {
            // TODO: This should probably be the intersection of the prevClip and the new clip rect
            
            if ( cmd.ClipRect.IsInfinite() )
            {
                // Disable clipping by passing nullptr to SDL_RenderSetClipRect
                SDL_RenderSetClipRect( m_Renderer, nullptr );
            }
            else
            {
                SDL_Rect sdlClip{
                    .x = static_cast<int>( cmd.ClipRect.Origin[0] ),
                    .y = static_cast<int>( cmd.ClipRect.Origin[1] ),
                    .w = static_cast<int>( cmd.ClipRect.Size[0] ),
                    .h = static_cast<int>( cmd.ClipRect.Size[1] )
                };

                SDL_RenderSetClipRect( m_Renderer, &sdlClip );
            }
            

            if ( Holds<DrawCmd::RectCmd>( cmd.Payload ) )
            {
                const auto& c = Get<DrawCmd::RectCmd>( cmd.Payload );
                RenderFillRoundedRect( c.Rect, c.Rounding, c.Color, cmd.Transform );
            }
            else if ( Holds<DrawCmd::RectBorderCmd>( cmd.Payload ) )
            {
                const auto& c = Get<DrawCmd::RectBorderCmd>( cmd.Payload );
                RenderFillRoundedRectBorder( c.Rect, c.Rounding, c.Color, c.Thickness, cmd.Transform );
            }
            else if ( Holds<DrawCmd::CircleCmd>( cmd.Payload ) )
            {
                const auto& c = Get<DrawCmd::CircleCmd>( cmd.Payload );
                RenderFillCircle( c.Center, c.Radius, c.Color, cmd.Transform );
            }
            else if ( Holds<DrawCmd::CircleBorderCmd>( cmd.Payload ) )
            {
                const auto& c = Get<DrawCmd::CircleBorderCmd>( cmd.Payload );
                RenderFillCircleBorder( c.Center, c.Radius, c.Color, c.Thickness, cmd.Transform );
            }
            else if ( Holds<DrawCmd::TextCmd>( cmd.Payload ) )
            {
                const auto& c = Get<DrawCmd::TextCmd>( cmd.Payload );
                RenderText( c.Text, c.Style, c.Rect, cmd.Transform );
            }
            else if ( Holds<DrawCmd::CustomCmd>( cmd.Payload ) )
            {
                const auto& c = Get<DrawCmd::CustomCmd>( cmd.Payload );
                if ( c.Func ) c.Func( *this, cmd );
            }
        }

        SDL_RenderSetClipRect( m_Renderer, &prevClip );
    }

    inline Optional<TextureID> SDL2Renderer::CreateTexture(
        u32            a_Width,
        u32            a_Height,
        ETextureFormat a_Format,
        const void*    a_Data )
    {
        if ( !m_Renderer )
            return NullOpt;

        // SDL2 does not support a native single-channel (R8) texture format for use with
        // SDL_RenderGeometry. We always create an RGBA texture and expand R8 data to RGBA
        // (white RGB, alpha = R) in UpdateTexture.
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
            return true; // Nothing to upload — not an error.

        // All SDL2 textures created by CreateTexture use SDL_PIXELFORMAT_RGBA32 internally.
        // ETextureFormat::R8 data (single byte per pixel) is expanded to RGBA here:
        //   R=255, G=255, B=255, A=source_byte
        // This lets vertex colours tint the glyph while the bitmap acts as an alpha mask.
        // ETextureFormat::RGBA8 is uploaded directly if a_DataSizeBytes == w * h * 4.
        // Note: only R8 and RGBA8 formats are supported.
        RATUI_USER_ASSERT( a_DataSizeBytes == static_cast<size>( w ) * h ||
                           a_DataSizeBytes == static_cast<size>( w ) * h * 4u,
                           "UpdateTexture: a_DataSizeBytes must match either R8 (w*h) or RGBA8 (w*h*4) format." );

        const bool isR8 = ( a_DataSizeBytes == static_cast<size>( w ) * h );

        if ( isR8 )
        {
            Array<u8> rgba;
            Resize( rgba, static_cast<size>( w ) * h * 4u );

            const u8* src = static_cast<const u8*>( a_Data );
            for ( u32 i = 0; i < w * h; ++i )
            {
                rgba[ i * 4 + 0 ] = 255u;
                rgba[ i * 4 + 1 ] = 255u;
                rgba[ i * 4 + 2 ] = 255u;
                rgba[ i * 4 + 3 ] = src[i];
            }

            SDL_Rect rect{
                static_cast<int>( a_Region.Origin[0] ),
                static_cast<int>( a_Region.Origin[1] ),
                static_cast<int>( w ),
                static_cast<int>( h )
            };

            return SDL_UpdateTexture( tex, &rect, Data( rgba ), static_cast<int>( w ) * 4 ) == 0;
        }
        else
        {
            // RGBA8: upload directly.
            SDL_Rect rect{
                static_cast<int>( a_Region.Origin[0] ),
                static_cast<int>( a_Region.Origin[1] ),
                static_cast<int>( w ),
                static_cast<int>( h )
            };

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

    inline void SDL2Renderer::RenderText(
        TextView         a_Text,
        const TextStyle& a_Style,
        Rectf            a_Rect,
        const Mat3f&     a_Transform )
    {
        if ( !m_Renderer || !m_FontCache || !m_GlyphAtlas )
            return; // Early out if we don't have a valid SDL_Renderer, FontCache, or GlyphAtlas.

        if ( a_Text.empty() || !a_Style.Font.IsValid() )
            return; // Early out if there's no text to render or the font handle is invalid.

        FreeType::Font* font = m_FontCache->GetOrLoadFont( a_Style.Font, static_cast<u32>( a_Style.Size ) );
        if ( !font )
            return; // Early out if we couldn't get a font for the given TextStyle.

        FT_Face face = font->FTFace;
        const u32 pixelSize = static_cast<u32>( a_Style.Size );

        // Build the wrapped line array using FreeType metrics.
        Array<String> lines;
        FreeType::TextUtil::BuildTextLines( face, a_Style, a_Text, lines, a_Rect.Size[0] );

        const SDL_Color sdlColor = ToSDLColor( a_Style.Color );
        const f32 lineHeight     = FreeType::GetLineHeight( face, a_Style );
        const f32 ascender       = face->size->metrics.ascender / 64.f;

        // Obtain the SDL_Texture* backing the glyph atlas.
        SDL_Texture* atlasTexture = static_cast<SDL_Texture*>( m_GlyphAtlas->GetTexture().Ptr );

        f32 lineY = a_Rect.Origin[1];

        for ( const String& line : lines )
        {
            if ( !Empty( line ) )
            {
                // Shape the line using HarfBuzz for correct glyph ordering and advances.
                Array<FreeType::ShapedGlyph> glyphs = FreeType::ShapeLine( font->HBFont, line, pixelSize );

                // Calculate the total advance width for horizontal alignment.
                f32 lineWidth = 0.f;
                for ( const FreeType::ShapedGlyph& g : glyphs )
                    lineWidth += g.XAdvance;

                f32 lineX = a_Rect.Origin[0];
                if ( a_Style.Align == ETextAlign::Center )
                    lineX += ( a_Rect.Size[0] - lineWidth ) * 0.5f;
                else if ( a_Style.Align == ETextAlign::Right )
                    lineX += a_Rect.Size[0] - lineWidth;

                // The baseline sits ascender pixels below the top of the line.
                const Vec2f origin{ lineX, lineY + ascender };

                auto transformPoint = [&]( float px, float py ) -> SDL_FPoint
                {
                    Vec3f p = a_Transform * Vec3f{ px, py, 1.f };
                    return SDL_FPoint{ p[0], p[1] };
                };

                FreeType::RenderShapedLine( *m_GlyphAtlas, face, glyphs, origin,
                    [&]( const FreeType::GlyphQuad& q )
                    {
                        SDL_Vertex verts[4];
                        verts[0] = { transformPoint( q.PosMin[0], q.PosMin[1] ), sdlColor, { q.UVMin[0], q.UVMin[1] } };
                        verts[1] = { transformPoint( q.PosMax[0], q.PosMin[1] ), sdlColor, { q.UVMax[0], q.UVMin[1] } };
                        verts[2] = { transformPoint( q.PosMax[0], q.PosMax[1] ), sdlColor, { q.UVMax[0], q.UVMax[1] } };
                        verts[3] = { transformPoint( q.PosMin[0], q.PosMax[1] ), sdlColor, { q.UVMin[0], q.UVMax[1] } };

                        const int indices[6] = { 0, 1, 2, 0, 2, 3 };
                        SDL_RenderGeometry( m_Renderer, atlasTexture, verts, 4, indices, 6 );
                    } );
            }

            lineY += lineHeight;
        }
    }

    inline void SDL2Renderer::RenderFillRoundedRect(
        Rectf          a_Rect,
        CornerRounding a_Rounding,
        Colorf         a_Color,
        const Mat3f&   a_Transform )
    {
        constexpr int c_Segments = 8;
        // 4 corner fans: each needs 1 center + (c_Segments+1) arc verts
        // 5 rects (center + 4 edge strips): each needs 4 verts
        constexpr int c_MaxVerts   = 5 * 4 + 4 * ( 1 + c_Segments + 1 );
        // 5 rects: 6 indices each; 4 corners: c_Segments*3 each
        constexpr int c_MaxIndices = 5 * 6 + 4 * c_Segments * 3;
        constexpr f32 c_PI_2       = Pi<f32> * 0.5f;

        const SDL_Color sdlColor = ToSDLColor( a_Color );

        SDL_Vertex vertices[c_MaxVerts];
        int        indices[c_MaxIndices];
        int        vertCount = 0;
        int        idxCount  = 0;

        const float x = a_Rect.Origin[0];
        const float y = a_Rect.Origin[1];
        const float w = a_Rect.Size[0];
        const float h = a_Rect.Size[1];

        float tlRadius = a_Rounding.TopLeft.Value;
        float trRadius = a_Rounding.TopRight.Value;
        float brRadius = a_Rounding.BottomRight.Value;
        float blRadius = a_Rounding.BottomLeft.Value;

        // Clamp radius so opposite corners don't exceed the available width/height
        {
            float scaleX = w / std::max( 1.f, ( tlRadius + trRadius ) );   // top
            float scaleY = h / std::max( 1.f, ( tlRadius + blRadius ) );   // left side
            scaleX = std::min( scaleX, w / std::max( 1.f, ( blRadius + brRadius ) ) ); // bottom
            scaleY = std::min( scaleY, h / std::max( 1.f, ( trRadius + brRadius ) ) ); // right side

            float scale = std::min( 1.f, std::min( scaleX, scaleY ) );

            tlRadius *= scale;
            trRadius *= scale;
            brRadius *= scale;
            blRadius *= scale;
        }

        auto pushVertex = [&]( float px, float py ) -> int
        {
            Vec3f p = a_Transform * Vec3f{ px, py, 1.f };
            vertices[vertCount] = { { p[0], p[1] }, sdlColor, { 0.f, 0.f } };
            return vertCount++;
        };

        auto pushQuad = [&]( int a, int b, int c, int d )
        {
            // a=TL, b=TR, c=BR, d=BL
            indices[idxCount++] = a; indices[idxCount++] = b; indices[idxCount++] = c;
            indices[idxCount++] = a; indices[idxCount++] = c; indices[idxCount++] = d;
        };

        // Center rect
        {
            int a = pushVertex( x + tlRadius,     y + tlRadius     );
            int b = pushVertex( x + w - trRadius, y + trRadius     );
            int c = pushVertex( x + w - brRadius, y + h - brRadius );
            int d = pushVertex( x + blRadius,     y + h - blRadius );
            pushQuad( a, b, c, d );
        }

        // Top strip
        {
            int a = pushVertex( x + tlRadius,     y            );
            int b = pushVertex( x + w - trRadius, y            );
            int c = pushVertex( x + w - trRadius, y + trRadius );
            int d = pushVertex( x + tlRadius,     y + tlRadius );
            pushQuad( a, b, c, d );
        }

        // Bottom strip
        {
            int a = pushVertex( x + blRadius,     y + h - blRadius );
            int b = pushVertex( x + w - brRadius, y + h - brRadius );
            int c = pushVertex( x + w - brRadius, y + h            );
            int d = pushVertex( x + blRadius,     y + h            );
            pushQuad( a, b, c, d );
        }

        // Left strip
        {
            int a = pushVertex( x,            y + tlRadius     );
            int b = pushVertex( x + tlRadius, y + tlRadius     );
            int c = pushVertex( x + blRadius, y + h - blRadius );
            int d = pushVertex( x,            y + h - blRadius );
            pushQuad( a, b, c, d );
        }

        // Right strip
        {
            int a = pushVertex( x + w - trRadius, y + trRadius     );
            int b = pushVertex( x + w,            y + trRadius     );
            int c = pushVertex( x + w,            y + h - brRadius );
            int d = pushVertex( x + w - brRadius, y + h - brRadius );
            pushQuad( a, b, c, d );
        }

        // Corner fans - Center is the inset corner point, arc goes from inner to outer edge in CCW direction
        auto addCorner = [&]( float cx, float cy, float startAngle, float r )
        {
            int   center = pushVertex( cx, cy );
            float step   = c_PI_2 / c_Segments;
            int   prev   = -1;

            for ( int s = 0; s <= c_Segments; ++s )
            {
                float angle = startAngle + s * step;
                int   cur   = pushVertex( cx + cosf( angle ) * r, cy + sinf( angle ) * r );
                if ( s > 0 )
                {
                    indices[idxCount++] = center;
                    indices[idxCount++] = prev;
                    indices[idxCount++] = cur;
                }
                prev = cur;
            }
        };

        addCorner( x + tlRadius,     y + tlRadius,     Pi<f32>       , tlRadius ); // Top-left
        addCorner( x + w - trRadius, y + trRadius,     Pi<f32> * 1.5f, trRadius ); // Top-right
        addCorner( x + w - brRadius, y + h - brRadius, 0.f           , brRadius ); // Bottom-right
        addCorner( x + blRadius,     y + h - blRadius, c_PI_2        , blRadius ); // Bottom-left

        SDL_RenderGeometry( m_Renderer, nullptr, vertices, vertCount, indices, idxCount );
    }

    inline void SDL2Renderer::RenderFillRoundedRectBorder(
        Rectf          a_Rect,
        CornerRounding a_Rounding,
        Colorf         a_Color,
        f32            a_Thickness,
        const Mat3f&   a_Transform )
    {
        // Nope

        // TODO: Would be good to add helper functions or something so used wont have to deal with it.
        // Something like an imgui-style batch draw commands into vertex and index buffers so users can handle that instead
    }

    inline void SDL2Renderer::RenderFillCircle(
        Vec2f        a_Center,
        f32          a_Radius,
        Colorf       a_Color,
        const Mat3f& a_Transform )
    {
        // Create a triangle fan

        constexpr int c_Segments   = 32;
        constexpr int c_MaxVerts   = 1 + c_Segments; /// center + arc
        constexpr int c_MaxIndices = c_Segments * 3;

        const SDL_Color sdlColor = ToSDLColor( a_Color );

        SDL_Vertex vertices[c_MaxVerts];
        int        indices[c_MaxIndices];
        int        vertCount = 0;
        int        idxCount  = 0;

        auto pushVertex = [&]( float x, float y ) -> int
        {
            Vec3f p = a_Transform * Vec3f{ x, y, 1.f };
            vertices[vertCount] = { { p[0], p[1] }, sdlColor, { 0.f, 0.f } };
            return vertCount++;
        };

        int center = pushVertex( a_Center[0], a_Center[1] );

        float step = ( Pi<f32> * 2.f ) / c_Segments;
        for ( int s = 0; s < c_Segments; ++s )
        {
            float angle = s * step;
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
        Vec2f        a_Center,
        f32          a_Radius,
        Colorf       a_Color,
        f32          a_Thickness,
        const Mat3f& a_Transform )
    {
        constexpr int c_Segments   = 32;
        constexpr int c_MaxVerts   = c_Segments * 2;  // outer + inner ring
        constexpr int c_MaxIndices = c_Segments * 6;  // 2 tris per segment

        const SDL_Color sdlColor   = ToSDLColor( a_Color );
        const float     rOuter     = a_Radius;
        const float     rInner     = std::max( 0.f, a_Radius - a_Thickness );

        SDL_Vertex vertices[c_MaxVerts];
        int        indices[c_MaxIndices];
        int        vertCount = 0;
        int        idxCount  = 0;

        auto pushVertex = [&]( float x, float y ) -> int
        {
            Vec3f p = a_Transform * Vec3f{ x, y, 1.f };
            vertices[vertCount] = { { p[0], p[1] }, sdlColor, { 0.f, 0.f } };
            return vertCount++;
        };

        float step = ( Pi<f32> * 2.f ) / c_Segments;
        for ( int s = 0; s < c_Segments; ++s )
        {
            float angle = s * step;
            float cosA  = cosf( angle );
            float sinA  = sinf( angle );
            pushVertex( a_Center[0] + cosA * rOuter, a_Center[1] + sinA * rOuter ); // outer
            pushVertex( a_Center[0] + cosA * rInner, a_Center[1] + sinA * rInner ); // inner
        }

        for ( int s = 0; s < c_Segments; ++s )
        {
            int o0 = ( s * 2 );
            int i0 = ( s * 2 ) + 1;
            int o1 = ( ( s + 1 ) % c_Segments ) * 2;
            int i1 = ( ( s + 1 ) % c_Segments ) * 2 + 1;

            indices[idxCount++] = o0;
            indices[idxCount++] = o1;
            indices[idxCount++] = i1;

            indices[idxCount++] = o0;
            indices[idxCount++] = i1;
            indices[idxCount++] = i0;
        }

        SDL_RenderGeometry( m_Renderer, nullptr, vertices, vertCount, indices, idxCount );
    }

} // namespace RatUI::SDL2