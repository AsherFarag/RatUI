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
        Rectu Rect;    ///< Rectangle within the atlas texture, in pixel coordinates.
        Vec2i Bearing; ///< Offset from the baseline to the top-left of the glyph bitmap.
    };

    /**
     * @brief Describes the position and UV coordinates of a glyph quad for rendering.
     */
    struct GlyphQuad
    {
        Vec2f PosMin{ 0.f, 0.f }, PosMax{ 0.f, 0.f };
        Vec2f UVMin { 0.f, 0.f }, UVMax { 0.f, 0.f };
    };

    /**
     * @brief A texture atlas for storing rasterized glyphs.
     * 
     * TODO: Currently it can just fill up and fail when full. Should either:
     * - Add page support (texture array) but this may be slow for rendering.
     * - Add an ImGui-style system where once its full, it evicts the oldest glyphs and reuses their space. 
     *   But this is bad if the text sizes dynamically change etc, as there will be constant texture updates.
     */
    class GlyphAtlas
    {
    public:
        GlyphAtlas( IRenderer& a_Renderer, u32 a_AtlasWidth, u32 a_AtlasHeight )
            : m_Renderer   ( a_Renderer )
            , m_AtlasWidth ( a_AtlasWidth )
            , m_AtlasHeight( a_AtlasHeight )
        {
            m_Texture = m_Renderer.CreateTexture( a_AtlasWidth, a_AtlasHeight, ETextureFormat::R8, nullptr )
                        .value_or( TextureID::Null() ); // TODO add ValueOr()

            // Pre-allocate the R8->RGBA scratch buffer to the full atlas size so
            // UpdateTexture never heap-allocates during a glyph upload.
            Resize( m_ScratchRGBA, static_cast<size>( a_AtlasWidth ) * a_AtlasHeight * 4u );
        }

        ~GlyphAtlas()
        {
            if ( m_Texture.IsValid() )
                m_Renderer.DestroyTexture( m_Texture );
        }

        TextureID GetTexture()  const { return m_Texture; }
        u32       GetWidth()    const { return m_AtlasWidth; }
        u32       GetHeight()   const { return m_AtlasHeight; }

        /**
         * @brief Returns true if the atlas ran out of space since the last Reset().
         *        Callers should allocate a new atlas page or rebuild when this is set.
         */
        bool IsAtlasFull() const { return m_AtlasFull; }

        /**
         * @brief Evicts all glyphs and resets the cursor to the top-left corner.
         *        The underlying GPU texture is not recreated - it will be overwritten
         *        as new glyphs are rasterized.
         *        @warning Both the glyph map and the indexed rect array are cleared.
         *                 Any ShapedText objects shaped against this atlas will have stale
         *                 GlyphIDs after a Reset() and must be re-shaped.
         */
        void Reset()
        {
            m_GlyphMap.clear();
            Clear( m_GlyphRects );
            m_CursorX   = 0;
            m_CursorY   = 0;
            m_RowBottom = 0;
            m_AtlasFull = false;
        }

        /**
         * @brief Looks up a glyph in the atlas, rasterizing and uploading it if not cached.
         *        Returns the glyph's index in the internal array, which can be stored in
         *        RatUI::ShapedGlyph::GlyphID for O(1) lookup at render time.
         *
         * @return The atlas index, or NullOpt if:
         *  - the GPU texture is invalid,
         *  - FreeType failed to load/render the glyph, or
         *  - the atlas is full (IsAtlasFull() will return true).
         */
        Optional<u32> GetOrRasterizeGlyphIndex( FT_Face a_Face, GlyphKey a_Key )
        {
            if ( !m_Texture.IsValid() )
                return NullOpt;

            auto it = Find( m_GlyphMap, a_Key );
            if ( it != End( m_GlyphMap ) )
                return it->second;

            if ( m_AtlasFull )
                return NullOpt; // Fast exit once full - don't re-attempt every frame.

            if ( FT_Load_Glyph( a_Face, a_Key.GlyphID, FT_LOAD_RENDER ) != 0 )
                return NullOpt;

            FT_Bitmap& bmp = a_Face->glyph->bitmap;

            // Advance to next row if the glyph doesn't fit on the current one.
            if ( m_CursorX + static_cast<i32>( bmp.width ) > static_cast<i32>( m_AtlasWidth ) )
            {
                m_CursorX = 0;
                m_CursorY = m_RowBottom;
            }

            // Check vertical overflow.
            if ( m_CursorY + static_cast<i32>( bmp.rows ) > static_cast<i32>( m_AtlasHeight ) )
            {
                m_AtlasFull = true;
                RATUI_ASSERT( false, "GlyphAtlas is full - increase atlas dimensions" );
                return NullOpt;
            }

            UploadBitmap( bmp, m_CursorX, m_CursorY );

            GlyphRect rect{
                .Rect    = Rectu{ Vec2u{ static_cast<u32>( m_CursorX ), static_cast<u32>( m_CursorY ) },
                                  Vec2u{ static_cast<u32>( bmp.width  ), static_cast<u32>( bmp.rows ) } },
                .Bearing = Vec2i{ a_Face->glyph->bitmap_left, a_Face->glyph->bitmap_top }
            };

            m_CursorX  += static_cast<i32>( bmp.width ) + 1; // 1 px padding to prevent bleeding
            m_RowBottom = std::max( m_RowBottom, m_CursorY + static_cast<i32>( bmp.rows ) + 1 );

            const u32 index = static_cast<u32>( Size( m_GlyphRects ) );
            PushBack( m_GlyphRects, rect );
            m_GlyphMap[a_Key] = index;

            return index;
        }

        /** @brief Convenience overload taking separate glyph ID and pixel size parameters. */
        Optional<u32> GetOrRasterizeGlyphIndex( FT_Face a_Face, u32 a_GlyphID, u32 a_PixelSize )
        {
            return GetOrRasterizeGlyphIndex( a_Face, GlyphKey{ a_GlyphID, a_PixelSize } );
        }

        /**
         * @brief Looks up a glyph in the atlas, rasterizing and uploading it if not cached.
         *
         * @return The GlyphRect, or NullOpt if:
         *  - the GPU texture is invalid,
         *  - FreeType failed to load/render the glyph, or
         *  - the atlas is full (IsAtlasFull() will return true).
         */
        Optional<GlyphRect> GetOrRasterizeGlyph( FT_Face a_Face, GlyphKey a_Key )
        {
            Optional<u32> index = GetOrRasterizeGlyphIndex( a_Face, a_Key );
            if ( !index )
                return NullOpt;
            return m_GlyphRects[*index];
        }

        /** @brief Convenience overload taking separate glyph ID and pixel size parameters. */
        Optional<GlyphRect> GetOrRasterizeGlyph( FT_Face a_Face, u32 a_GlyphID, u32 a_PixelSize )
        {
            return GetOrRasterizeGlyph( a_Face, GlyphKey{ a_GlyphID, a_PixelSize } );
        }

        /**
         * @brief Returns a pointer to the GlyphRect for a glyph stored at @p a_Index,
         *        as returned by GetOrRasterizeGlyphIndex().
         * @return Pointer to the GlyphRect, or nullptr if the index is out of range.
         */
        const GlyphRect* GetGlyphRect( u32 a_Index ) const
        {
            if ( a_Index >= static_cast<u32>( Size( m_GlyphRects ) ) )
                return nullptr;
            return &m_GlyphRects[a_Index];
        }

    private:
        struct GlyphKeyHasher
        {
            size_t operator()( const GlyphKey& a_Key ) const noexcept
            {
                return std::hash<u32>{}( a_Key.GlyphID ) ^ ( std::hash<u32>{}( a_Key.PixelSize ) << 1 );
            }
        };

        /**
         * @brief Uploads a FreeType bitmap to the atlas texture.
         *
         * All SDL2 atlas textures use RGBA32 internally (see SDL2Renderer::CreateTexture).
         * R8 (single-channel) bitmaps are expanded here using the pre-allocated scratch
         * buffer so we never heap-allocate during a per-glyph upload.
         *
         * Packed bitmaps (pitch == width) are also handled without a copy by expanding
         * directly into the scratch buffer row-by-row.
         */
        void UploadBitmap( const FT_Bitmap& a_Bmp, i32 a_Dx, i32 a_Dy )
        {
            if ( a_Bmp.rows == 0 || a_Bmp.width == 0 )  
                return;

            const Rectu region{ Vec2u{ static_cast<u32>( a_Dx ), static_cast<u32>( a_Dy ) },
                                Vec2u{ static_cast<u32>( a_Bmp.width ), static_cast<u32>( a_Bmp.rows ) } };

            const u32   stride     = static_cast<u32>( std::abs( a_Bmp.pitch ) );
            const size  pixelCount = static_cast<size>( a_Bmp.rows ) * a_Bmp.width;

            // FreeType produces R8 bitmaps.  We always pass through the IRenderer
            // UpdateTexture path which handles R8->RGBA expansion, so we just need to
            // ensure the source bytes are tightly packed (no row padding).
            if ( stride == a_Bmp.width )
            {
                // Already packed - upload directly.
                m_Renderer.UpdateTexture( m_Texture, 0, region, a_Bmp.buffer, pixelCount );
            }
            else
            {
                // Rows have padding - pack into the scratch buffer first.
                // The scratch buffer is sized to the full atlas (width * height bytes for R8),
                // which is always large enough for any single glyph.
                for ( u32 row = 0; row < a_Bmp.rows; ++row )
                {
                    std::memcpy(
                        Data( m_ScratchRGBA ) + row * a_Bmp.width,
                        a_Bmp.buffer          + row * stride,
                        static_cast<size>( a_Bmp.width )
                    );
                }

                m_Renderer.UpdateTexture( m_Texture, 0, region, Data( m_ScratchRGBA ), pixelCount );
            }
        }

        IRenderer&   m_Renderer;
        TextureID    m_Texture{ TextureID::Null() };
        u32          m_AtlasWidth{ 0 }, m_AtlasHeight{ 0 };
        i32          m_CursorX{ 0 }, m_CursorY{ 0 }, m_RowBottom{ 0 };
        bool         m_AtlasFull{ false };

        // Persistent scratch buffer for R8->packed conversion.
        // Reusing it avoids a heap allocation on every glyph upload.
        // Sized to hold R8 data for the full atlas (w * h bytes).
        Array<u8> m_ScratchRGBA;

        // Indexed glyph storage: the map stores an index into m_GlyphRects so that
        // callers can cache the index (as RatUI::ShapedGlyph::GlyphID) and perform
        // O(1) lookups at render time without hashing.
        HashMap<GlyphKey, u32, GlyphKeyHasher> m_GlyphMap;
        Array<GlyphRect>                       m_GlyphRects;
    };

    /**
     * @brief Renders a line of text from a pre-shaped RatUI::ShapedText.
     *        Glyphs are looked up by their atlas index (RatUI::ShapedGlyph::GlyphID),
     *        so no font or rasterization is needed at render time.
     * @tparam RenderGlyphFunc  Callable matching `void(const GlyphQuad&)`.
     * @param a_Atlas       The glyph atlas that was used during Shape().
     * @param a_Glyphs      Glyph span for one line (a_Text.Glyphs[line.Start .. line.End]).
     * @param a_Position    Baseline origin (x = left edge, y = baseline) in screen space.
     * @param a_RenderGlyph Callback invoked once per visible glyph with its quad data.
     */
    template<std::invocable<const GlyphQuad&> RenderGlyphFunc>
    void RenderShapedTextLine(
        const GlyphAtlas&              a_Atlas,
        Span<const ShapedGlyph> a_Glyphs,
        Vec2f                          a_Position,
        RenderGlyphFunc&&              a_RenderGlyph )
    {
        const f32 atlasW = static_cast<f32>( a_Atlas.GetWidth() );
        const f32 atlasH = static_cast<f32>( a_Atlas.GetHeight() );

        for ( const RatUI::ShapedGlyph& g : a_Glyphs )
        {
            const GlyphRect* gr = a_Atlas.GetGlyphRect( g.GlyphID );
            if ( !gr || gr->Rect.Width() == 0 )
            {
                a_Position[0] += g.XAdvance;
                continue;
            }

            f32 x0 = a_Position[0] + g.XOffset + static_cast<f32>( gr->Bearing[0] );
            f32 y0 = a_Position[1] - g.YOffset - static_cast<f32>( gr->Bearing[1] ); // Y-down, bearing is Y-up
            f32 x1 = x0 + static_cast<f32>( gr->Rect.Width() );
            f32 y1 = y0 + static_cast<f32>( gr->Rect.Height() );

            f32 u0 = static_cast<f32>( gr->Rect.Origin[0] )                    / atlasW;
            f32 v0 = static_cast<f32>( gr->Rect.Origin[1] )                    / atlasH;
            f32 u1 = static_cast<f32>( gr->Rect.Origin[0] + gr->Rect.Width()  ) / atlasW;
            f32 v1 = static_cast<f32>( gr->Rect.Origin[1] + gr->Rect.Height() ) / atlasH;

            a_RenderGlyph( GlyphQuad{
                Vec2f{ x0, y0 }, Vec2f{ x1, y1 },
                Vec2f{ u0, v0 }, Vec2f{ u1, v1 }
            } );

            a_Position[0] += g.XAdvance;
            a_Position[1] -= g.YAdvance;
        }
    }

} // namespace RatUI::FreeType