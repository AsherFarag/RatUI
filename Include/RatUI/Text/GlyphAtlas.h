#pragma once
#include "../Renderer/IRenderer.h"
#include "ITextMetrics.h"

namespace RatUI
{
    // TODO: This should be configurable
    inline constexpr f64 c_MsdfPxRange = 4.0;

    /**
     * @brief Describes the pixel rectangle and glyph bearing within a GlyphAtlas texture.
     * The atlas Rect covers the full padded MSDF bitmap.  Bearing and PlaneSize describe
     * the plane bounds (ink extent only, without SDF padding) in base-size pixels so that
     * the rendered quad never overruns the font metrics and is not clipped by a layout rect.
     */
    struct GlyphRect
    {
        Rectu16 Rect;      ///< Rectangle within the atlas texture, in pixel coordinates (full SDF bitmap including padding).
        Vec2i   Bearing;   ///< Offset from the baseline origin to the top-left of the SDF bitmap (Y-up), including SDF padding.
        Vec2i   PlaneSize; ///< Ink-only width and height in base-size pixels (bitmap minus the two SDF padding rings). Informational.
    };

    /**
     * @brief
     * TODO: Support multithreading
     * TODO: Currently it can just fill up and fail when full. Should either:
     * - Add page support (texture array) but this may be slow for rendering.
     * - Add an ImGui-style system where once its full, it evicts the oldest glyphs and reuses their space. 
     *   But this is bad if the text sizes dynamically change etc, as there will be constant texture updates.
     */
    class GlyphAtlas
    {
    public:
        GlyphAtlas( IRenderer& a_Renderer, ITextMetrics& a_TextMetrics,
                    u16 a_Width = 4096, u16 a_Height = 4096, u32 a_BaseSize = 64u )
            : m_Renderer   ( a_Renderer )
            , m_TextMetrics( a_TextMetrics )
            , m_AtlasWidth ( a_Width )
            , m_AtlasHeight( a_Height )
            , m_BaseSize   ( a_BaseSize )
        {
            m_Texture = m_Renderer.CreateTexture( a_Width, a_Height, ETextureFormat::RGBA8, nullptr )
                        .value_or( TextureID::Null() );
        }

        ~GlyphAtlas()
        {
            if ( m_Texture.IsValid() )
                m_Renderer.DestroyTexture( m_Texture );
        }

        GlyphAtlas( const GlyphAtlas& ) = delete;
        GlyphAtlas& operator=( const GlyphAtlas& ) = delete;

        /** @brief Returns the GPU texture that holds the rasterized glyph bitmaps. */
        TextureID GetTexture() const { return m_Texture; }

        /** @brief Returns the pixel width of the atlas texture. */
        u16 GetWidth() const { return m_AtlasWidth; }

        /** @brief Returns the pixel height of the atlas texture. */
        u16 GetHeight() const { return m_AtlasHeight; }

        /**  @brief Returns true if the atlas ran out of space. */
        bool IsAtlasFull() const { return m_AtlasFull; }

        /** @brief Returns the fixed base size passed to RasterizeGlyph for all codepoints. */
        u32 GetBaseSize() const { return m_BaseSize; }

        /** @brief Returns a reference to the renderer used by the glyph atlas. */
        IRenderer& GetRenderer() const { return m_Renderer; }

        /** @brief Returns a reference to the text metrics system used by the glyph atlas. */
        ITextMetrics& GetTextMetrics() const { return m_TextMetrics; }

        /**
         * @brief Looks up or rasterizes a glyph in the atlas.
         */
        Optional<GlyphRect> GetOrRasterizeGlyph( FontHandle a_Font, u32 a_GlyphIndex )
        {
            const GlyphKey key{ a_Font, a_GlyphIndex };

            if ( const auto it = Find( m_GlyphMap, key ); it != End( m_GlyphMap ) )
                return it->second;

            const Coloru8* pixels     = nullptr; // In RGBA8 format
            u32            width      = 0, height = 0;
            Vec2i          bearing{};
            Vec2i          planeSize{};

            if ( !m_TextMetrics.RasterizeGlyph( a_Font, a_GlyphIndex, m_BaseSize, 
                pixels, width, height, bearing, planeSize ) )
                return NullOpt; // Rasterization failed (e.g. missing glyph in font).

            if ( width == 0 || height == 0 )
            {
                // Cache invisible glyphs as zero-size rects to avoid repeated rasterization attempts.
                GlyphRect invisibleRect{};
                m_GlyphMap[key] = invisibleRect;
                return invisibleRect;
            }

            if ( !m_Texture.IsValid() )
                return NullOpt; // Can't upload if texture is invalid.

            // Upload the glyph bitmap to the atlas texture.
            if ( auto region = AllocateRegion( static_cast<u16>( width ), static_cast<u16>( height ) ) )
            {
                const size dataSizeBytes = static_cast<size>( width ) * height * sizeof( Coloru8 );
                m_Renderer.UpdateTexture( m_Texture, 0, region->Cast<u32>(), pixels, dataSizeBytes );
                GlyphRect rect{ .Rect = *region, .Bearing = bearing, .PlaneSize = planeSize };
                m_GlyphMap[key] = rect;
                return rect;
            }
            
            return NullOpt; // Failed to allocate space in the atlas.
        }

    protected:

        Optional<Rectu16> AllocateRegion( u16 a_Width, u16 a_Height )
        {
            if ( m_AtlasFull )
                return NullOpt; // Fast exit once full - don't re-attempt every frame.

            if ( a_Width > m_AtlasWidth || a_Height > m_AtlasHeight )
                return NullOpt; // Glyph is too large to fit in the atlas.

            // Check horizontal overflow and move to next row if needed.
            if ( m_CursorX + a_Width > m_AtlasWidth )
            {
                m_CursorX = 0;
                m_CursorY = m_RowBottom;
            }

            // Check vertical overflow.
            if ( m_CursorY + a_Height > m_AtlasHeight )
            {
                m_AtlasFull = true;
                RATUI_ASSERT( false, "GlyphAtlas is full - increase atlas dimensions" );
                return NullOpt;
            }

            Rectu16 region{ { m_CursorX, m_CursorY },
                            { a_Width, a_Height } };

            m_CursorX  += a_Width + 1; // 1 px padding to prevent bleeding
            m_RowBottom = std::max( m_RowBottom, static_cast<u16>( m_CursorY + a_Height + 1 ) );

            return region;
        }

    protected:
        IRenderer&     m_Renderer;
        ITextMetrics&  m_TextMetrics;
        TextureID      m_Texture{ TextureID::Null() };
        u16            m_AtlasWidth{ 0 }, m_AtlasHeight{ 0 };
        u32            m_BaseSize{ 64u }; ///< Fixed rasterization size used for all MSDF glyph generation.
        u16            m_CursorX{ 0 }, m_CursorY{ 0 }, m_RowBottom{ 0 };
        bool           m_AtlasFull{ false };

        struct GlyphKey
        {
            FontHandle Font;
            u32        GlyphIndex;

            bool operator==( const GlyphKey& a_Other ) const = default;
        };

        struct GlyphKeyHasher
        {
            std::size_t operator()( const GlyphKey& a_Key ) const
            {
                // FNV-1a hash combine
                std::size_t hash = 2166136261u;
                hash = ( hash ^ std::hash<FontHandle>{}( a_Key.Font ) ) * 16777619u;
                hash = ( hash ^ std::hash<u32>{}( a_Key.GlyphIndex ) ) * 16777619u;
                return hash;
            }
        };

        HashMap<GlyphKey, GlyphRect, GlyphKeyHasher> m_GlyphMap;
    };

} // namespace RatUI