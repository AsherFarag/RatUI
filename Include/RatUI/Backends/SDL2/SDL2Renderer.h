#pragma once
#include "../../RatUI.h"
#include <SDL.h>
//#include <SDL_ttf.h>
//#include <SDL_image.h>

class SDL2Renderer : public RatUI::IRenderer
{
public:
    SDL2Renderer( SDL_Renderer* a_Renderer = nullptr )
        : m_Renderer( a_Renderer )
    {}

    void SetSDLRenderer( SDL_Renderer* a_Renderer )
    {
        m_Renderer = a_Renderer;
    }

    void Execute( RatUI::Span<const RatUI::DrawCmd> a_Commands ) override;

    // TODO: Currently only handles rounding the same for all corners
    void RenderFillRoundedRect( RatUI::Rectf a_Rect, RatUI::f32 a_Radius, RatUI::Colorf a_Color, const RatUI::Mat3f& a_Transform );

    // TODO: Currently only handles rounding the same for all corners
    void RenderFillRoundedRectBorder( RatUI::Rectf a_Rect, RatUI::f32 a_Radius, RatUI::Colorf a_Color, RatUI::f32 a_Thickness, const RatUI::Mat3f& a_Transform ) {};

    void RenderFillCircle( RatUI::Vec2f a_Center, RatUI::f32 a_Radius, RatUI::Colorf a_Color, const RatUI::Mat3f& a_Transform ) {};

    void RenderFillCircleBorder( RatUI::Vec2f a_Center, RatUI::f32 a_Radius, RatUI::Colorf a_Color, RatUI::f32 a_Thickness, const RatUI::Mat3f& a_Transform ) {};

    static SDL_Color ToSDLColor( RatUI::Colorf a_Color )
    {
        return SDL_Color{
            static_cast<Uint8>( a_Color[0] * 255.f ),
            static_cast<Uint8>( a_Color[1] * 255.f ),
            static_cast<Uint8>( a_Color[2] * 255.f ),
            static_cast<Uint8>( a_Color[3] * 255.f )
        };
    }

protected:
    SDL_Renderer* m_Renderer;
};

// ---------------------------------------------------------------------------
// Execute
// ---------------------------------------------------------------------------

inline void SDL2Renderer::Execute( RatUI::Span<const RatUI::DrawCmd> a_Commands )
{
    using namespace RatUI;

    if ( !m_Renderer )
        return;

    for ( const DrawCmd& cmd : a_Commands )
    {
        if ( Holds<DrawCmd::RectCmd>( cmd.Payload ) )
        {
            const auto& rectCmd = Get<DrawCmd::RectCmd>( cmd.Payload );
            RenderFillRoundedRect( rectCmd.Rect, rectCmd.Rounding.TopLeft.Value, rectCmd.Color, cmd.Transform );
        }
        else if ( Holds<DrawCmd::RectBorderCmd>( cmd.Payload ) )
        {
            const auto& borderCmd = Get<DrawCmd::RectBorderCmd>( cmd.Payload );
            RenderFillRoundedRectBorder( borderCmd.Rect, borderCmd.Rounding.TopLeft.Value, borderCmd.Color, borderCmd.Thickness, cmd.Transform );
        }
        else if ( Holds<DrawCmd::CircleCmd>( cmd.Payload ) )
        {
            const auto& circleCmd = Get<DrawCmd::CircleCmd>( cmd.Payload );
            RenderFillCircle( circleCmd.Center, circleCmd.Radius, circleCmd.Color, cmd.Transform );
        }
        else if ( Holds<DrawCmd::CircleBorderCmd>( cmd.Payload ) )
        {
            const auto& circleBorderCmd = Get<DrawCmd::CircleBorderCmd>( cmd.Payload );
            RenderFillCircleBorder( circleBorderCmd.Center, circleBorderCmd.Radius, circleBorderCmd.Color, circleBorderCmd.Thickness, cmd.Transform );
        }
        else if ( Holds<DrawCmd::CustomCmd>( cmd.Payload ) )
        {
            const auto& customCmd = Get<DrawCmd::CustomCmd>( cmd.Payload );
			if ( customCmd.Func )
            {
                customCmd.Func( *this, cmd );
            }
        }
    }
}

inline void SDL2Renderer::RenderFillRoundedRect(
    RatUI::Rectf        a_Rect,
    RatUI::f32          a_Radius,
    RatUI::Colorf       a_Color,
    const RatUI::Mat3f& a_Transform )
{
    using namespace RatUI;

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

    const float r = SDL_min( a_Radius, SDL_min( a_Rect.Size[0], a_Rect.Size[1] ) * 0.5f );
    const float x = a_Rect.Origin[0];
    const float y = a_Rect.Origin[1];
    const float w = a_Rect.Size[0];
    const float h = a_Rect.Size[1];

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
        int a = pushVertex( x + r,     y + r     );
        int b = pushVertex( x + w - r, y + r     );
        int c = pushVertex( x + w - r, y + h - r );
        int d = pushVertex( x + r,     y + h - r );
        pushQuad( a, b, c, d );
    }

    // Top strip
    {
        int a = pushVertex( x + r,     y     );
        int b = pushVertex( x + w - r, y     );
        int c = pushVertex( x + w - r, y + r );
        int d = pushVertex( x + r,     y + r );
        pushQuad( a, b, c, d );
    }

    // Bottom strip
    {
        int a = pushVertex( x + r,     y + h - r );
        int b = pushVertex( x + w - r, y + h - r );
        int c = pushVertex( x + w - r, y + h     );
        int d = pushVertex( x + r,     y + h     );
        pushQuad( a, b, c, d );
    }

    // Left strip
    {
        int a = pushVertex( x,     y + r     );
        int b = pushVertex( x + r, y + r     );
        int c = pushVertex( x + r, y + h - r );
        int d = pushVertex( x,     y + h - r );
        pushQuad( a, b, c, d );
    }

    // Right strip
    {
        int a = pushVertex( x + w - r, y + r     );
        int b = pushVertex( x + w,     y + r     );
        int c = pushVertex( x + w,     y + h - r );
        int d = pushVertex( x + w - r, y + h - r );
        pushQuad( a, b, c, d );
    }

    // Corner fans - Center is the inset corner point, arc goes from inner to outer edge in CCW direction
    auto addCorner = [&]( float cx, float cy, float startAngle )
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

    addCorner( x + r,     y + r,     Pi<f32>        ); // Top-left
    addCorner( x + w - r, y + r,     Pi<f32> * 1.5f ); // Top-right
    addCorner( x + w - r, y + h - r, 0.f            ); // Bottom-right
    addCorner( x + r,     y + h - r, c_PI_2         ); // Bottom-left

    SDL_RenderGeometry( m_Renderer, nullptr, vertices, vertCount, indices, idxCount );
}