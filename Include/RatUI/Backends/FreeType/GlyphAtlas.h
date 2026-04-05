#pragma once
#include "../../RatUI.h"
#include "FontCache.h"

namespace RatUI::FreeType
{
    /**
     * @brief A key for identifying a specific glyph in the atlas.
     */
    struct GlyphKey
    {
        u32 GlyphID;   ///< The FreeType glyph index (not Unicode codepoint).
        u32 PixelSize; ///< The pixel size at which the glyph is rasterized.

        constexpr bool operator==( const GlyphKey& ) const = default;
    };

    /**
     * @brief Describes the rectangle and bearing of a rasterized glyph within the atlas texture.
     */
    struct GlyphRect
    {
        Rectu Rect;    ///< The rectangle within the atlas texture where the glyph bitmap is stored, in pixel coordinates.
        Vec2i Bearing; ///< The horizontal and vertical bearing of the glyph, i.e. the offset from the baseline to the top-left of the glyph bitmap.
    };

    /**
     * @brief Describes the position and UV coordinates of a glyph quad for rendering.
     */
    struct GlyphQuad
    {
        Vec2f PosMin{ 0.f, 0.f }, PosMax{ 0.f, 0.f }; ///< The position of the glyph quad in screen space (or whatever coordinate space the caller is using).
        Vec2f UVMin{ 0.f, 0.f }, UVMax{ 0.f, 0.f };   ///< The UV coordinates of the glyph quad in the atlas texture, normalized to [0, 1].
    };

    /** 
     * @brief A texture atlas for storing rasterized glyphs.
     * This class manages a single texture and a mapping of glyph keys to their locations within that texture. 
     * It handles rasterizing glyphs on demand and uploading them to the GPU.
     */
    class GlyphAtlas
    {
    public:
        GlyphAtlas( IRenderer& a_Renderer, u32 a_AtlasWidth, u32 a_AtlasHeight )
            : m_Renderer( a_Renderer )
            , m_AtlasWidth( a_AtlasWidth )
            , m_AtlasHeight( a_AtlasHeight )
        {
            m_Texture = m_Renderer.CreateTexture( a_AtlasWidth, a_AtlasHeight, ETextureFormat::R8, nullptr )
                        .value_or( TextureID::Null() );
        }

        ~GlyphAtlas()
        {
            if ( m_Texture.IsValid() )
                m_Renderer.DestroyTexture( m_Texture );
        }

        /** @brief Returns the GPU texture containing all rasterized glyphs. */
        TextureID GetTexture() const { return m_Texture; }

        /** @brief Gets the width of the glyph atlas texture. */
        u32 GetWidth() const { return m_AtlasWidth; }

        /** @brief Gets the height of the glyph atlas texture. */
        u32 GetHeight() const { return m_AtlasHeight; }

        /**
         * @brief Looks up a glyph in the atlas, rasterizing and uploading it if not already cached.
         * @return The GlyphRect describing the glyph's location and bearing in the atlas, or NullOpt
         *         if the texture is invalid, the glyph could not be rasterized, or the atlas is full.
         */
        Optional<GlyphRect> GetOrRasterizeGlyph( FT_Face a_Face, GlyphKey a_Key )
        {
            if ( !m_Texture.IsValid() )
                return NullOpt;

            auto it = Find( m_GlyphMap, a_Key );
            if ( it != End( m_GlyphMap ) )
                return it->second;

            if ( FT_Load_Glyph( a_Face, a_Key.GlyphID, FT_LOAD_RENDER ) != 0 )
                return NullOpt;

            FT_Bitmap& bmp = a_Face->glyph->bitmap;

            if ( m_CursorX + (i32)bmp.width > (i32)m_AtlasWidth )
            {
                m_CursorX = 0;
                m_CursorY = m_RowBottom;
            }

            if ( m_CursorY + (i32)bmp.rows > (i32)m_AtlasHeight )
                return NullOpt; // atlas full — would need a second atlas page here

            UploadBitmap( bmp, m_CursorX, m_CursorY );

            GlyphRect rect{
                .Rect = Rectu{ Vec2u{ (u32)m_CursorX, (u32)m_CursorY }, Vec2u{ (u32)bmp.width, (u32)bmp.rows } },
                .Bearing = Vec2i{ a_Face->glyph->bitmap_left, a_Face->glyph->bitmap_top }
            };

            m_CursorX   += (i32)bmp.width + 1;  // 1px padding to avoid bleeding
            m_RowBottom  = std::max(m_RowBottom, m_CursorY + (i32)bmp.rows + 1);

            m_GlyphMap[a_Key] = rect;
            return rect;
        }

        /** @brief Overload that takes separate glyph ID and pixel size parameters for convenience. */
        Optional<GlyphRect> GetOrRasterizeGlyph( FT_Face a_Face, u32 a_GlyphID, u32 a_PixelSize )
        {
            return GetOrRasterizeGlyph( a_Face, GlyphKey{ a_GlyphID, a_PixelSize } );
        }

    private:
        struct GlyphKeyHasher
        {
            size_t operator()( const GlyphKey& a_Key ) const noexcept
            {
                // Simple hash combining GlyphID and PixelSize.
                return std::hash<u32>{}( a_Key.GlyphID ) ^ ( std::hash<u32>{}( a_Key.PixelSize ) << 1 );
            }
        };

        void UploadBitmap(const FT_Bitmap& a_Bmp, i32 a_Dx, i32 a_Dy)
        {
            if ( a_Bmp.rows == 0 || a_Bmp.width == 0 )
                return;

            const Rectu region{ Vec2u{ static_cast<u32>( a_Dx ), static_cast<u32>( a_Dy ) },
                                 Vec2u{ a_Bmp.width, a_Bmp.rows } };

            // FT_Bitmap::pitch is the byte stride per row and may be negative (bottom-up) or
            // padded (positive but wider than width). Normalise to an unsigned stride.
            const u32 stride = static_cast<u32>( std::abs( a_Bmp.pitch ) );

            if ( stride == a_Bmp.width )
            {
                // Rows are already packed tightly — upload directly.
                m_Renderer.UpdateTexture( m_Texture, 0, region, a_Bmp.buffer, static_cast<size>( a_Bmp.rows ) * a_Bmp.width );
            }
            else
            {
                // Rows have padding — pack into a contiguous buffer before uploading.
                Array<u8> packed;
                Resize( packed, static_cast<size>( a_Bmp.rows ) * a_Bmp.width );
                for ( u32 row = 0; row < a_Bmp.rows; ++row )
                {
                    std::memcpy( Data( packed ) + row * a_Bmp.width,
                                 a_Bmp.buffer  + row * stride,
                                 a_Bmp.width );
                }
                m_Renderer.UpdateTexture( m_Texture, 0, region, Data( packed ), Size( packed ) );
            }
        }

        IRenderer& m_Renderer;
        TextureID  m_Texture{ TextureID::Null() };
        u32        m_AtlasWidth{ 0 }, m_AtlasHeight{ 0 };
        i32        m_CursorX{ 0 }, m_CursorY{ 0 }, m_RowBottom{ 0 };
        HashMap<GlyphKey, GlyphRect, GlyphKeyHasher> m_GlyphMap;
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