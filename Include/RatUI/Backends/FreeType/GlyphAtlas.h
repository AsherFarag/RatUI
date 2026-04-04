#pragma once
#include "../../RatUI.h"
#include "FontCache.h"

namespace RatUI::FreeType
{
    struct GlyphKey
    {
        u32 GlyphID;
        u32 PixelSize;

        constexpr bool operator==( const GlyphKey& ) const = default;
    };

    struct GlyphRect
    {
        Rectu Rect;
        Vec2i Bearing;
    };

    struct GlyphQuad
    {
        Vec2f PosMin{ 0.f, 0.f }, PosMax{ 0.f, 0.f }; ///< The position of the glyph quad in screen space (or whatever coordinate space the caller is using).
        Vec2f UVMin{ 0.f, 0.f }, UVMax{ 0.f, 0.f };   ///< The UV coordinates of the glyph quad in the atlas texture, normalized to [0, 1].
    };


    class GlyphAtlas
    {
    public:
        GlyphAtlas( IRenderer& a_Renderer, u32 a_AtlasWidth, u32 a_AtlasHeight )
            : m_Renderer( a_Renderer )
            , m_AtlasWidth( a_AtlasWidth )
            , m_AtlasHeight( a_AtlasHeight )
        {
            // Create an empty texture for the atlas.
            // TODO
            //m_Texture = m_Renderer.CreateTexture( a_AtlasWidth, a_AtlasHeight, ETextureFormat::RGBA8, nullptr ).value_or( TextureID::Null() );
        }

        ~GlyphAtlas()
        {
            if ( m_Texture.IsValid() )
                m_Renderer.DestroyTexture( m_Texture );
        }

        /** @brief Gets the texture ID of the glyph atlas. */
        TextureID GetTexture() const { return m_Texture; }

        /** @brief Gets the dimensions of the glyph atlas. */
        u32 GetWidth() const { return m_AtlasWidth; }
        u32 GetHeight() const { return m_AtlasHeight; }

        Optional<GlyphRect> GetOrRasterizeGlyph( FT_Face a_Face, GlyphKey a_Key )
        {
            auto it = Find( m_GlyphMap, a_Key );
            if (it != End( m_GlyphMap )) 
                return it->second; // Glyph already in atlas, return its rect. TODO: Add First() Second() helpers.

            FT_Load_Glyph(a_Face, a_Key.GlyphID, FT_LOAD_RENDER);
            FT_Bitmap& bmp = a_Face->glyph->bitmap;

            if (m_CursorX + (i32)bmp.width > m_AtlasWidth)
            {
                m_CursorX = 0;
                m_CursorY = m_RowBottom;
            }

            if (m_CursorY + (i32)bmp.rows > m_AtlasHeight)
                return NullOpt; // atlas full — would need a second atlas page here

            UploadBitmap(bmp, m_CursorX, m_CursorY);

            GlyphRect rect{
                .Rect = Rectu{ Vec2u{ (u32)m_CursorX, (u32)m_CursorY }, Vec2u{ (u32)bmp.width, (u32)bmp.rows } },
                .Bearing = Vec2i{ a_Face->glyph->bitmap_left, a_Face->glyph->bitmap_top }
            };

            m_CursorX   += (i32)bmp.width + 1;  // 1px padding to avoid bleeding
            m_RowBottom  = std::max(m_RowBottom, m_CursorY + (i32)bmp.rows + 1);

            m_GlyphMap[a_Key] = rect; // Cache the glyph rect for future lookups.
            return rect;
        }

        Optional<GlyphRect> GetOrRasterizeGlyph( FT_Face a_Face, u32 a_GlyphID, u32 a_PixelSize )
        {
            return GetOrRasterizeGlyph( a_Face, GlyphKey{ a_GlyphID, a_PixelSize } );
        }

    private:
        void UploadBitmap(const FT_Bitmap& a_Bmp, i32 a_Dx, i32 a_Dy)
        {
            //void* pixels; i32 pitch;
            //u32* dst = static_cast<u32*>(pixels);
            //for (u32 row = 0; row < a_Bmp.rows; ++row)
            //{
            //    for (u32 col = 0; col < a_Bmp.width; ++col)
            //    {
            //        u8 alpha = a_Bmp.buffer[row * a_Bmp.pitch + col];
            //        dst[(a_Dy + row) * (pitch / 4) + (a_Dx + col)] = (alpha << 24) | 0x00FFFFFF;  // white + alpha
            //    }
            //}

            // TODO: Use m_Renderer.UpdateTexture to upload the pixel data to the atlas texture.
        }

        IRenderer& m_Renderer;
        TextureID  m_Texture{ TextureID::Null() };
        u32        m_AtlasWidth{ 0 }, m_AtlasHeight{ 0 };
        i32        m_CursorX{ 0 }, m_CursorY{ 0 }, m_RowBottom{ 0 };
        HashMap<GlyphKey, GlyphRect> m_GlyphMap;
    };

    /**
     * @brief Renders a shaped line of text using the provided glyph atlas and render function.
     * @tparam RenderGlyphFunc A callable type that can be invoked with the signature `void(const GlyphQuad&)`, which will be called for each glyph to render it.
     * @param a_Atlas The glyph atlas to use for retrieving glyph rectangles.
     * @param a_Face The FreeType face to use for rasterizing glyphs if they are not already in the atlas.
     * @param a_Glyphs A span of shaped glyphs to render, containing their IDs and positioning information.
     * @param a_Position The starting position to render the line of text.
     * @param a_RenderGlyph A callable that will be invoked for each glyph, receiving its GlyphRect, the position to render it at, and the UV coordinates in the atlas. This allows the caller to define how the glyphs are rendered (e.g., as textured quads).
     */
    template<std::invocable<const GlyphQuad&> RenderGlyphFunc>
    void RenderShapedLine( GlyphAtlas& a_Atlas, 
        FT_Face a_Face, 
        Span<const ShapedGlyph> a_Glyphs, 
        Vec2f a_Position, 
        RenderGlyphFunc&& a_RenderGlyph )
    {
        const u32 atlasW = a_Atlas.GetWidth();
        const u32 atlasH = a_Atlas.GetHeight();
        for (const ShapedGlyph& g : a_Glyphs)
        {
            // TODO: Doing a hash lookup for every glyph will add up.
            // Need to optimize with array lookup somehow? or dense hashmap
            Optional<GlyphRect> gr = a_Atlas.GetOrRasterizeGlyph(a_Face, g.GlyphID, g.PixelSize);
            if (!gr || gr->Rect.Width() == 0) 
            { 
                a_Position[0] += g.XAdvance; 
                continue;
            }

            f32 x0 = a_Position[0] + g.XOffset + gr->Bearing[0];
            f32 y0 = a_Position[1] - g.YOffset - gr->Bearing[1];  // Y-down, bearing is Y-up
            f32 x1 = x0 + gr->Rect.Width();
            f32 y1 = y0 + gr->Rect.Height();

            f32 u0 = (f32)gr->Rect.Origin[0] / atlasW;
            f32 v0 = (f32)gr->Rect.Origin[1] / atlasH;
            f32 u1 = (f32)(gr->Rect.Origin[0] + gr->Rect.Width()) / atlasW;
            f32 v1 = (f32)(gr->Rect.Origin[1] + gr->Rect.Height()) / atlasH;

            a_RenderGlyph( GlyphQuad{ Vec2f{ x0, y0 }, Vec2f{ x1, y1 },
                                      Vec2f{ u0, v0 }, Vec2f{ u1, v1 } } );

            a_Position[0] += g.XAdvance;
            a_Position[1] -= g.YAdvance;
        }
    }

} // namespace RatUI::FreeType