#pragma once
#include "../../Text/FontLibrary.h"
#include "Config.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

namespace msdfgen { class FontHandle; }

namespace RatUI::FreeType
{
    struct FontFaceOptions
    {
        EGlyphRenderMode Mode{ EGlyphRenderMode::MTSDF };
        f32              NativePixelSize{ 0.f }; ///< Pixel mode: design pixels per em. 0 = auto-detect.
        u32              FaceIndex{ 0 };         ///< Face within a .ttc / .otc collection.
        StringView       Name;                   ///< Debug name. Defaults to "Family Style" from the file.
    };

    /** @brief Where a Pixel face's native size came from. */
    enum class EPixelGridSource : u8
    {
        None,
        Manual,
        EmbeddedBitmap,
        OutlineGrid,
        Fallback, ///< Detection failed and 16 was assumed.
    };

    /** @brief Owns the FT_Library. Shared by every face, so faces can outlive the loader. */
    struct LibraryHandle
    {
        FT_Library Library{ nullptr };
        ~LibraryHandle();
    };

    /** @brief FreeType-backed face: HarfBuzz shaping, msdfgen MTSDF, FreeType Raster / Pixel bitmaps. */
    class FontFace final : public IFontFace
    {
    public:
        ~FontFace() override;

        // Non-copyable
        FontFace( const FontFace& ) = delete;
        FontFace& operator=( const FontFace& ) = delete;

        EFontFaceKind          Kind() const override { return EFontFaceKind::FreeType; }
        const FontFaceMetrics& Metrics() const override { return m_Metrics; }
        StringView             GetDebugName() const override { return m_Name; }

        GlyphID  GetGlyphIndex( codepoint a_Codepoint ) const override;
        FontUnit GetAdvance( GlyphID a_Glyph ) const override;
        FontUnit GetKerning( GlyphID a_Left, GlyphID a_Right ) const override;
        bool     RasterizeGlyph( const GlyphRasterRequest& a_Request, GlyphBitmap& o_Bitmap ) override;
        void     ShapeRun( StringView a_Text, const ShapeRunParams& a_Params, Array<ShapedGlyph>& o_Glyphs ) override;

        FT_Face          GetFTFace() const { return m_Face; }
        hb_font_t*       GetHBFont() const { return m_HBFont; }
        EPixelGridSource GetPixelGridSource() const { return m_PixelGridSource; }

        /** @brief */
        static f32 DetectPixelGrid( FT_Face a_Face, EPixelGridSource* o_Source = nullptr );

    private:
        friend class FontLoader;
        FontFace() = default;

        bool Initialize( Shared<LibraryHandle> a_Library, FT_Face a_Face, Array<u8> a_Data, const FontFaceOptions& a_Options );

        bool RasterizeMTSDF( const GlyphRasterRequest& a_Request, GlyphBitmap& o_Bitmap );
        bool RasterizeCoverage( const GlyphRasterRequest& a_Request, GlyphBitmap& o_Bitmap );
        bool RasterizeGridSnapped( const GlyphRasterRequest& a_Request, GlyphBitmap& o_Bitmap );

        // TODO: m_HBBuffer is not thread-safe if we want to shape text from multiple threads. We could either:
        //     1. Add a mutex to protect access to m_HBBuffer, allowing it to be shared across threads at the cost of potential contention.
        //     2. Remove m_HBBuffer and require callers to create and manage their own hb_buffer_t instances for shaping,
        //        which would allow for thread-local buffers without synchronization overhead.

        Shared<LibraryHandle> m_Library;
        Array<u8>             m_Data;          ///< Font bytes for memory-loaded faces.
        FT_Face               m_Face{ nullptr };
        hb_font_t*            m_HBFont{ nullptr };
        hb_buffer_t*          m_HBBuffer{ nullptr };
        msdfgen::FontHandle*  m_MsdfFont{ nullptr };
        f32                   m_RcpUnitsPerEm{ 1.f };
        f32                   m_GridUnits{ 0.f };    ///< Pixel mode: font units per native pixel.
        FontFaceMetrics       m_Metrics;
        EPixelGridSource      m_PixelGridSource{ EPixelGridSource::None };
        String                m_Name;
        Array<Color>          m_Scratch;
    };

    /** @brief Loads FreeType faces. Can be destroyed while its faces are still in use. */
    class FontLoader
    {
    public:
        FontLoader();

        bool IsValid() const { return m_Library && m_Library->Library; }
        FT_Library GetLibrary() const { return m_Library ? m_Library->Library : nullptr; }

        Unique<IFontFace> LoadFromFile( const char* a_FilePath, const FontFaceOptions& a_Options = {} );

        /** @brief */
        Unique<IFontFace> LoadFromMemory( const void* a_Data, size a_Size, const FontFaceOptions& a_Options = {} );

        /** @brief Number of faces in a font file (> 1 for collections). */
        u32 GetFaceCount( const char* a_FilePath ) const;

    private:
        Unique<IFontFace> Finish( FT_Face a_Face, Array<u8> a_Data, const FontFaceOptions& a_Options );

        Shared<LibraryHandle> m_Library;
    };

} // namespace RatUI::FreeType
