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
    void RenderFillRoundedRect( RatUI::Rectf a_Rect, RatUI::CornerRounding a_Rounding, RatUI::Colorf a_Color, const RatUI::Mat3f& a_Transform );

    // TODO: Currently only handles rounding the same for all corners
    void RenderFillRoundedRectBorder( RatUI::Rectf a_Rect, RatUI::f32 a_Radius, RatUI::Colorf a_Color, RatUI::f32 a_Thickness, const RatUI::Mat3f& a_Transform );

    void RenderFillCircle( RatUI::Vec2f a_Center, RatUI::f32 a_Radius, RatUI::Colorf a_Color, const RatUI::Mat3f& a_Transform );

    void RenderFillCircleBorder( RatUI::Vec2f a_Center, RatUI::f32 a_Radius, RatUI::Colorf a_Color, RatUI::f32 a_Thickness, const RatUI::Mat3f& a_Transform );

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

    // TODO: Capture the renderer state and restore it after rendering, to avoid interfering with the application's own rendering code

    for ( const DrawCmd& cmd : a_Commands )
    {
    
        SDL_Rect sdlClipRect{
            static_cast<int>( cmd.ClipRect.Origin[0] ),
            static_cast<int>( cmd.ClipRect.Origin[1] ),
            static_cast<int>( cmd.ClipRect.Size[0] ),
            static_cast<int>( cmd.ClipRect.Size[1] )
        };

        // TODO: Get the intersection of the user set clip rect and the current scissor rect
        SDL_RenderSetClipRect( m_Renderer, &sdlClipRect );

        if ( Holds<DrawCmd::RectCmd>( cmd.Payload ) )
        {
            const auto& rectCmd = Get<DrawCmd::RectCmd>( cmd.Payload );
            RenderFillRoundedRect( rectCmd.Rect, rectCmd.Rounding, rectCmd.Color, cmd.Transform );
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
                customCmd.Func( *this, cmd );
        }
    }
}

inline void SDL2Renderer::RenderFillRoundedRect(
    RatUI::Rectf          a_Rect,
    RatUI::CornerRounding a_Rounding,
    RatUI::Colorf         a_Color,
    const RatUI::Mat3f&   a_Transform )
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
    RatUI::Rectf        a_Rect,
    RatUI::f32          a_Radius,
    RatUI::Colorf       a_Color,
    RatUI::f32          a_Thickness,
    const RatUI::Mat3f& a_Transform )
{
    // Nope

    // TODO: Would be good to add helper functions or something so used wont have to deal with it.
    // Something like an imgui-style batch draw commands into vertex and index buffers so users can handle that instead
}

inline void SDL2Renderer::RenderFillCircle(
    RatUI::Vec2f        a_Center,
    RatUI::f32          a_Radius,
    RatUI::Colorf       a_Color,
    const RatUI::Mat3f& a_Transform )
{
    using namespace RatUI;
    
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
    RatUI::Vec2f        a_Center,
    RatUI::f32          a_Radius,
    RatUI::Colorf       a_Color,
    RatUI::f32          a_Thickness,
    const RatUI::Mat3f& a_Transform )
{
    using namespace RatUI;

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
