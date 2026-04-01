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
    void RenderFillRoundedRectBorder( SDL_FRect a_Rect, RatUI::f32 a_Radius, SDL_Color a_Color, RatUI::f32 a_Thickness );

    void RenderFillCircle( RatUI::Vec2f a_Center, RatUI::f32 a_Radius, SDL_Color a_Color );

    void RenderFillCircleBorder( RatUI::Vec2f a_Center, RatUI::f32 a_Radius, SDL_Color a_Color, RatUI::f32 a_Thickness );

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

inline void SDL2Renderer::Execute( RatUI::Span<const RatUI::DrawCmd> a_Commands )
{
    using namespace RatUI;

    if ( !m_Renderer )
    {
		return; // Can't render without a valid SDL_Renderer
    }

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
        }
        else if ( Holds<DrawCmd::CircleCmd>( cmd.Payload ) )
        {
            const auto& circleCmd = Get<DrawCmd::CircleCmd>( cmd.Payload );
        }
        else if ( Holds<DrawCmd::CircleBorderCmd>( cmd.Payload ) )
        {
            const auto& circleBorderCmd = Get<DrawCmd::CircleBorderCmd>( cmd.Payload );
        }
        else if ( Holds<DrawCmd::CustomCmd>( cmd.Payload ) )
        {
            const auto& customCmd = Get<DrawCmd::CustomCmd>( cmd.Payload );
            customCmd.Func( *this, cmd );
        }
    }
}

inline void SDL2Renderer::RenderFillRoundedRect(
    RatUI::Rectf a_Rect,
    RatUI::f32 a_Radius,
    RatUI::Colorf a_Color,
    const RatUI::Mat3f& a_Transform )
{
    using namespace RatUI;

    constexpr int c_Segments = 8; // segments per corner
    constexpr int c_MaxVertices = 4 + (c_Segments + 2) * 4; // center quad + 4 corners
    constexpr int c_MaxIndices = 6 + c_Segments * 3 * 4;     // center quad + corner triangles
	constexpr f32 c_PI_2 = Pi<f32> * 0.5f;

    const SDL_Color sdlColor = ToSDLColor( a_Color );

    SDL_Vertex vertices[c_MaxVertices];
    int indices[c_MaxIndices];

    int vertCount = 0;
    int idxCount  = 0;

    const float r = SDL_min(a_Radius, SDL_min(a_Rect.Size[0], a_Rect.Size[1]) * 0.5f);

    auto pushVertex = [&](float x, float y) -> int {
        Vec3f p = a_Transform * Vec3f{ x, y, 1.f };
        vertices[vertCount] = { { p[0], p[1] }, sdlColor, {0.f, 0.f} };
        return vertCount++;
    };

    // Center quad
    int i0 = pushVertex(a_Rect.Origin[0] + r,     a_Rect.Origin[1] + r);
    int i1 = pushVertex(a_Rect.Origin[0] + a_Rect.Size[0] - r, a_Rect.Origin[1] + r);
    int i2 = pushVertex(a_Rect.Origin[0] + a_Rect.Size[0] - r, a_Rect.Origin[1] + a_Rect.Size[1] - r);
    int i3 = pushVertex(a_Rect.Origin[0] + r,     a_Rect.Origin[1] + a_Rect.Size[1] - r);

    indices[idxCount++] = i0; indices[idxCount++] = i1; indices[idxCount++] = i2;
    indices[idxCount++] = i0; indices[idxCount++] = i2; indices[idxCount++] = i3;

    auto addCorner = [&](float cx, float cy, float startAngle) {
        int center = pushVertex(cx, cy);
        int prev = -1;
        float step = (float)c_PI_2 / c_Segments;

        for (int s = 0; s <= c_Segments; ++s) {
            float angle = startAngle + s * step;
            float px = cx + cosf(angle) * r;
            float py = cy + sinf(angle) * r;
            int cur = pushVertex(px, py);

            if (prev != -1) {
                indices[idxCount++] = center;
                indices[idxCount++] = prev;
                indices[idxCount++] = cur;
            }
            prev = cur;
        }
    };

    // Top-left
    addCorner(a_Rect.Origin[0] + r, a_Rect.Origin[1] + r, M_PI);
    // Top-right
    addCorner(a_Rect.Origin[0] + a_Rect.Size[0] - r, a_Rect.Origin[1] + r, -c_PI_2 );
    // Bottom-right
    addCorner(a_Rect.Origin[0] + a_Rect.Size[0] - r, a_Rect.Origin[1] + a_Rect.Size[1] - r, 0.f);
    // Bottom-left
    addCorner(a_Rect.Origin[0] + r, a_Rect.Origin[1] + a_Rect.Size[1] - r, c_PI_2 );

    SDL_RenderGeometry(m_Renderer,
                       nullptr,
                       vertices,
                       vertCount,
                       indices,
                       idxCount);
}