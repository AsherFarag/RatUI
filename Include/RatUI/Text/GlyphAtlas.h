#pragma once
#include "../Renderer/IRenderer.h"
#include "FontLibrary.h"

namespace RatUI
{
    struct GlyphAtlasConfig
    {
        u16 PageSize       { 1024 }; ///< Width / height of each page texture.
        u8  MaxPagesPerMode{ 8 };    ///< When full, the least recently used page is recycled.
        u16 SDFBaseSize    { 56 };   ///< MTSDF glyphs are generated at this pixel size.
        f32 SDFPixelRange  { 16.f }; ///< MTSDF distance range in pixels at SDFBaseSize.
    };

    /** @brief Everything that changes a glyph's pixels, so e.g. faux bold never shares the regular glyph's entry. */
    struct GlyphCacheKey
    {
        FontFaceHandle   Face{};
        GlyphID          Glyph{};
        EGlyphRenderMode Mode{ EGlyphRenderMode::MTSDF };
        u16              PixelSize{ 0 };      ///< Raster only, MTSDF / Pixel glyphs are scaled at draw time.
        bool             SynthBold  { false }; ///< Raster / Pixel only.
        bool             SynthItalic{ false }; ///< Raster / Pixel only.

        constexpr bool operator==( const GlyphCacheKey& ) const = default;

        static constexpr GlyphCacheKey For( const ResolvedFace& a_Face, GlyphID a_Glyph, EGlyphRenderMode a_Mode, u16 a_PixelSize )
        {
            const bool bitmap = a_Mode != EGlyphRenderMode::MTSDF;
            return GlyphCacheKey{
                .Face        = a_Face.Face,
                .Glyph       = a_Glyph,
                .Mode        = a_Mode,
                .PixelSize   = a_Mode == EGlyphRenderMode::Raster ? a_PixelSize : u16{ 0 },
                .SynthBold   = bitmap && a_Face.SynthBold,
                .SynthItalic = bitmap && a_Face.SynthItalic,
            };
        }
    };

    struct AtlasGlyph
    {
        const TextureHandle* Page{ nullptr }; ///< Stable for the lifetime of the atlas.
        Rectu16              Rect{};          ///< Zero size for blank glyphs.
        Vec2f                Bearing{};       ///< Pen to top-left, Y-up. MTSDF: em. Raster / Pixel: pixels.

        constexpr bool IsBlank() const { return Rect.Size[0] == 0 || Rect.Size[1] == 0; }
    };

    /** @brief */
    class SkylinePacker
    {
    public:
        void Reset( u16 a_Width, u16 a_Height );
        Optional<Vec2<u16>> Allocate( u16 a_Width, u16 a_Height );

    private:
        struct Node { i32 X, Y, Width; };

        bool Fits( size a_Index, i32 a_Width, i32 a_Height, i32& o_Y ) const;

        Array<Node> m_Nodes;
        i32         m_Width{ 0 }, m_Height{ 0 };
    };

    /**
     * @brief Rasterizes glyphs on demand into texture pages, one pool per render mode (Pixel pages use nearest sampling).
     * Each page starts with a small white block, used to draw underlines / strikethroughs in the text batch.
     * TODO: Support multithreading
     */
    class GlyphAtlas
    {
    public:
        static constexpr StringView c_CommonASCIIGlyphs = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                          "abcdefghijklmnopqrstuvwxyz"
                                                          "0123456789"
                                                          ".,!?-+/():;%&`\"*#=[]";

        static constexpr u16 c_WhiteBlockSize = 8;

        GlyphAtlas( IRenderer& a_Renderer, FontLibrary& a_Fonts, const GlyphAtlasConfig& a_Config = {} );

        GlyphAtlas( const GlyphAtlas& ) = delete;
        GlyphAtlas& operator=( const GlyphAtlas& ) = delete;

        const GlyphAtlasConfig& GetConfig() const { return m_Config; }
        IRenderer& GetRenderer() const { return m_Renderer; }
        FontLibrary& GetFontLibrary() const { return m_Fonts; }

        /** @brief Pages used this frame are never recycled. Called by DrawList::Clear(). */
        void BeginFrame() { ++m_Frame; }

        /** @brief Rasterizes and uploads the glyph on first use. NullOpt if it can't be made or no page is free this frame. */
        Optional<AtlasGlyph> GetOrRasterizeGlyph( const GlyphCacheKey& a_Key );

        /** @brief Solid white texels in a page of @p a_Mode, for decorations. */
        Optional<AtlasGlyph> GetWhiteBlock( EGlyphRenderMode a_Mode );

        /** @brief */
        void LoadGlyphs( const ResolvedFace& a_Face, u16 a_PixelSize, StringView a_Text );

        /** @brief Drops every cached glyph, keeping the pages. */
        void Clear();

        u32 GetPageCount( EGlyphRenderMode a_Mode ) const { return static_cast<u32>( Size( m_Pools[ToUnderlying( a_Mode )] ) ); }
        u32 GetGlyphCount() const { return static_cast<u32>( Size( m_Glyphs ) ); }
        u32 GetEvictionCount() const { return m_EvictionCount; }

    private:
        struct Page
        {
            TextureHandle Texture;
            SkylinePacker Packer;
            u64           LastUsedFrame{ 0 };
        };

        struct CachedGlyph
        {
            u8      PageIndex{ 0 };
            Rectu16 Rect{};
            Vec2f   Bearing{};
        };

        struct GlyphKeyHasher
        {
            size_t operator()( const GlyphCacheKey& a_Key ) const
            {
                // FNV-1a hash combine
                u64 hash = 1469598103934665603ull;
                const auto mix = [&]( u64 a_Value ) { hash = ( hash ^ a_Value ) * 1099511628211ull; };
                mix( a_Key.Face.ID );
                mix( ToUnderlying( a_Key.Glyph ) );
                mix( ( static_cast<u64>( ToUnderlying( a_Key.Mode ) ) << 32 ) | ( static_cast<u64>( a_Key.PixelSize ) << 8 )
                     | ( a_Key.SynthBold ? 1u : 0u ) | ( a_Key.SynthItalic ? 2u : 0u ) );
                return static_cast<size_t>( hash );
            }
        };

        using PagePool = Array<Unique<Page>>;

        Optional<CachedGlyph> Rasterize( const GlyphCacheKey& a_Key );
        Optional<Vec2<u16>>   AllocateRegion( EGlyphRenderMode a_Mode, u16 a_Width, u16 a_Height, u8& o_PageIndex );
        Page*                 CreatePage( EGlyphRenderMode a_Mode );
        void                  ResetPage( EGlyphRenderMode a_Mode, u8 a_PageIndex );
        AtlasGlyph            ToAtlasGlyph( EGlyphRenderMode a_Mode, const CachedGlyph& a_Glyph );

        IRenderer&       m_Renderer;
        FontLibrary&     m_Fonts;
        GlyphAtlasConfig m_Config;
        u64              m_Frame{ 1 };
        u32              m_EvictionCount{ 0 };

        FixedArray<PagePool, static_cast<size>( EGlyphRenderMode::Count )> m_Pools;
        HashMap<GlyphCacheKey, CachedGlyph, GlyphKeyHasher> m_Glyphs;
        Array<Color> m_UploadBuffer;
    };

} // namespace RatUI
