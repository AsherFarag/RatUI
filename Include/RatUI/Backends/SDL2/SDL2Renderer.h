#pragma once
#include "../../RatUI.h"
#include "SDL2FontCache.h"
#include "SDL2TextLayout.h"
#include <SDL.h>
#include <SDL_ttf.h>

namespace RatUI::SDL2
{

    /**
     * @brief SDL2-backed renderer.
     */
    class SDL2Renderer : public IRenderer
    {
    public:
        SDL2Renderer() = default;
        SDL2Renderer( SDL_Renderer* a_Renderer, SDL2FontCache* a_FontCache )
            : m_Renderer( a_Renderer )
            , m_FontCache( a_FontCache )
        {}

        SDL_Renderer* GetSDLRenderer() const { return m_Renderer; }
        SDL2FontCache* GetFontCache() const { return m_FontCache; }

        void SetSDLRenderer( SDL_Renderer* a_Renderer ) { m_Renderer = a_Renderer; }
        void SetFontCache( SDL2FontCache* a_Cache ) { m_FontCache = a_Cache;  }

        void Execute( Span<const DrawCmd> a_Commands ) override;

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
        SDL_Renderer* m_Renderer  { nullptr };
        SDL2FontCache* m_FontCache{ nullptr };
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

    inline void SDL2Renderer::RenderText(
        TextView         a_Text,
        const TextStyle& a_Style,
        Rectf            a_Rect,
        const Mat3f&     a_Transform )
    {
        if ( !m_Renderer || !m_FontCache )
            return; // Early out if we don't have a valid SDL_Renderer or FontCache to work with.

        if ( a_Text.empty() || !a_Style.Font.IsValid() )
            return; // Early out if there's no text to render or the font handle is invalid.

        TTF_Font* font = m_FontCache->GetFont( a_Style );
        if ( !font )
            return; // Early out if we couldn't get a TTF_Font* for the given TextStyle (e.g. font failed to load).

        // - Build the line array
        Array<String> lines;
        RatUI::SDL2::TextLayoutUtils::BuildTextLines( font, a_Style, a_Text, lines, a_Rect.Size[0] );

        const SDL_Color sdlColor = ToSDLColor( a_Style.Color );
        const f32 lineHeight = SDL2FontCache::GetLineHeight( font, a_Style );

        // - Render each line
        // SDL_ttf blended rendering produces an ARGB surface which we blit into
        // a texture, position at the correct line origin, then render as a quad
        // using SDL_RenderGeometry so the transform matrix is respected.

        f32 lineY = a_Rect.Origin[1];

        for ( const String& line : lines )
        {
            if ( !Empty( line ) )
            {
                SDL_Surface* surface = TTF_RenderUTF8_Blended( font, CStr( line ), sdlColor );
                if ( surface )
                {
                    SDL_Texture* texture = SDL_CreateTextureFromSurface( m_Renderer, surface );
                    SDL_FreeSurface( surface );

                    if ( texture )
                    {
                        int texW = 0, texH = 0;
                        SDL_QueryTexture( texture, nullptr, nullptr, &texW, &texH );

                        // Resolve text alignment within the rect.
                        f32 lineX = a_Rect.Origin[0];
                        if ( a_Style.Align == ETextAlign::Center )
                            lineX += ( a_Rect.Size[0] - static_cast<f32>( texW ) ) * 0.5f;
                        else if ( a_Style.Align == ETextAlign::Right )
                            lineX += a_Rect.Size[0] - static_cast<f32>( texW );

                        // Build a quad and transform its vertices.
                        const float x0 = lineX;
                        const float y0 = lineY;
                        const float x1 = lineX + static_cast<float>( texW );
                        const float y1 = lineY + static_cast<float>( texH );

                        SDL_Color white{ 255, 255, 255, 255 };

                        auto transformPoint = [&]( float px, float py ) -> SDL_FPoint
                        {
                            Vec3f p = a_Transform * Vec3f{ px, py, 1.f };
                            return SDL_FPoint{ p[0], p[1] };
                        };

                        SDL_Vertex verts[4];
                        verts[0] = { transformPoint( x0, y0 ), white, { 0.f, 0.f } };
                        verts[1] = { transformPoint( x1, y0 ), white, { 1.f, 0.f } };
                        verts[2] = { transformPoint( x1, y1 ), white, { 1.f, 1.f } };
                        verts[3] = { transformPoint( x0, y1 ), white, { 0.f, 1.f } };

                        const int indices[6] = { 0, 1, 2, 0, 2, 3 };

                        SDL_RenderGeometry( m_Renderer, texture, verts, 4, indices, 6 );
                        SDL_DestroyTexture( texture );
                    }
                }
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