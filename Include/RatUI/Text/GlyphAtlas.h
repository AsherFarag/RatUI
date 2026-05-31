#pragma once
#include "../Renderer/IRenderer.h"
#include "ITextMetrics.h"

namespace RatUI
{
    // TODO: This should be configurable
    inline constexpr f64 c_MsdfPxRange = 16.0;

    /**
     * @brief Metrics describing the position of a rasterized glyph within the atlas texture and how to position it relative to the baseline when rendering.
     */
    struct GlyphMetrics
    {
        Vec2<FontUnit> Bearing;   ///< Offset from the baseline origin to the top-left of the glyph bitmap (Y-up).
        Rectu16        AtlasRect; ///< Rectangle within the glyph atlas texture where the glyph's bitmap is stored, in pixel coordinates.
        FontUnit       XAdvance;  ///< Horizontal advance to the next glyph. TODO: Should we support YAdvance for vertical text layout in the future?
    };

    /**
     * @brief Configuration settings for the glyph atlas.
     */
    struct GlyphAtlasConfig
    {
        u16      AtlasWidth { 2048 }; ///< The width of the glyph atlas texture in pixels.
        u16      AtlasHeight{ 2048 }; ///< The height of the glyph atlas texture in pixels.
        Pixel    BaseSize{ 56_px };   ///< The base font size in pixels at which glyphs are rasterized.
                                      ///< Higher values => better quality at larger sizes but more texture space used. 
                                      ///< Lower values => more efficient for small text but missing details at large sizes.
                                      ///< Ideal range is typically around 48-64.              
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
        static constexpr Span<const codepoint> c_CommonASCIIGlyphs = U"ABCDEFGHIJKLMNOPQRSTUVWXYZ" 
                                                                      "abcdefghijklmnopqrstuvwxyz"
                                                                      "0123456789"
                                                                      ".,!?-+/():;%&`\"*#=[]";

        GlyphAtlas( IRenderer& a_Renderer, ITextMetrics& a_TextMetrics,
                    const GlyphAtlasConfig& a_Config = {} )
            : m_Renderer   ( a_Renderer )
            , m_TextMetrics( a_TextMetrics )
            , m_Config     ( a_Config )
        {
            m_Texture = m_Renderer.CreateTexture( 
                {
					.Size = { m_Config.AtlasWidth, m_Config.AtlasHeight },
					.Format = ETextureFormat::RGBA8,
                },
                nullptr );
        }

        GlyphAtlas( const GlyphAtlas& ) = delete;
        GlyphAtlas& operator=( const GlyphAtlas& ) = delete;

        /** @brief Returns the GPU texture that holds the rasterized glyph bitmaps. */
        const TextureHandle& GetTexture() const { return m_Texture; }

        /** @brief Returns a reference to the configuration settings of the glyph atlas. */
        const GlyphAtlasConfig& GetConfig() const { return m_Config; }

        /** @brief Returns a reference to the renderer used by the glyph atlas. */
        IRenderer& GetRenderer() const { return m_Renderer; }

        /** @brief Returns a reference to the text metrics system used by the glyph atlas. */
        ITextMetrics& GetTextMetrics() const { return m_TextMetrics; }

        /**
         * @brief
         */
		Optional<GlyphMetrics> GetOrRasterizeGlyph( FontHandle a_Font, GlyphID a_GlyphIndex )
        {
            const GlyphKey key{ a_Font, a_GlyphIndex };

            if ( const auto it = Find( m_GlyphMap, key ); it != End( m_GlyphMap ) )
                return it->second;

            if ( !m_Texture.IsValid() )
                return NullOpt; // Can't upload if texture is invalid.

            const Color* pixels = nullptr; // In RGBA8 format
            u32            width  = 0, height = 0;
            Vec2<FontUnit> bearing{};
            FontUnit       xAdvance{};

            const f32 baseSize = m_Config.BaseSize.ToFloat();
            if ( !std::isfinite( baseSize ) || baseSize <= 0.f )
                return NullOpt;

            const u32 baseSizePx = static_cast<u32>( std::max( 1.0f, baseSize ) );

            if ( !m_TextMetrics.RasterizeGlyph( a_Font, a_GlyphIndex, baseSizePx,
                pixels, width, height, bearing, xAdvance ) )
                return NullOpt; // Rasterization failed (e.g. missing glyph in font).

            if ( width == 0 || height == 0 )
            {
                // Cache invisible glyphs as zero-size rects to avoid repeated rasterization attempts.
                constexpr GlyphMetrics invisibleRect{};
                m_GlyphMap[key] = invisibleRect;
                return invisibleRect;
            }

            // Upload the glyph bitmap to the atlas texture.
            if ( auto region = AllocateRegion( static_cast<u16>( width ), static_cast<u16>( height ) ) )
            {
                const size dataSizeBytes = static_cast<size>( width ) * height * sizeof( Color );
                m_Renderer.UpdateTexture( m_Texture.GetID(), 0, region->Cast<u32>(), pixels, dataSizeBytes);

                GlyphMetrics rect { .Bearing   = bearing,
                                    .AtlasRect = *region,
                                    .XAdvance  = xAdvance };

                m_GlyphMap[key] = rect;
                return rect;
            }
            
            return NullOpt; // Failed to allocate space in the atlas.
        }

		void LoadGlyphs( FontHandle a_Font, Span<const GlyphID> a_Glyphs )
		{
			// TODO: Can optimise this by batching rasterization and texture uploads, but for simplicity we will just do them one at a time for now.
			for ( const GlyphID glyphID : a_Glyphs )
			{
				GetOrRasterizeGlyph( a_Font, glyphID );
			}
		}

    protected:

        Optional<Rectu16> AllocateRegion( u16 a_Width, u16 a_Height )
        {
            if ( m_AtlasFull )
                return NullOpt; // Fast exit once full - don't re-attempt every frame.

            if ( a_Width > m_Config.AtlasWidth || a_Height > m_Config.AtlasHeight )
                return NullOpt; // Glyph is too large to fit in the atlas.

            // Check horizontal overflow and move to next row if needed.
            if ( m_CursorX + a_Width > m_Config.AtlasWidth )
            {
                m_CursorX = 0;
                m_CursorY = m_RowBottom;
            }

            // Check vertical overflow.
            if ( m_CursorY + a_Height > m_Config.AtlasHeight )
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
        IRenderer&       m_Renderer;
        ITextMetrics&    m_TextMetrics;
        TextureHandle    m_Texture;
        GlyphAtlasConfig m_Config;
        u16              m_CursorX{ 0 }, m_CursorY{ 0 }, m_RowBottom{ 0 };
        bool             m_AtlasFull{ false };

        struct GlyphKey
        {
            FontHandle Font;
            GlyphID    GlyphIndex;

            bool operator==( const GlyphKey& a_Other ) const = default;
        };

        struct GlyphKeyHasher
        {
            std::size_t operator()( const GlyphKey& a_Key ) const
            {
                // FNV-1a hash combine
                std::size_t hash = 2166136261u;
                hash = ( hash ^ std::hash<FontHandle>{}( a_Key.Font ) ) * 16777619u;
                hash = ( hash ^ std::hash<GlyphID>{}( a_Key.GlyphIndex ) ) * 16777619u;
                return hash;
            }
        };

        HashMap<GlyphKey, GlyphMetrics, GlyphKeyHasher> m_GlyphMap;
    };

} // namespace RatUI
