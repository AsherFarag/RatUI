#pragma once
#include "../../RatUI.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>
#include <msdfgen.h>
#include <ext/import-font.h>
#include <msdfgen-ext.h>

#include <map> // TODO: Remove me once we have Map<>

namespace RatUI::FreeType
{
    /**
     * @brief Returns the line height for the given style and FreeType face.
     * If TextStyle::LineHeight is non-zero, that value is used directly.
     * Otherwise, the line height is derived from the face's size metrics.
     */
    inline Unit GetLineHeight( FT_Face a_Face, const TextLayoutStyle& a_Style )
    {
        if ( a_Style.LineHeight > 0_u )
            return a_Style.LineHeight;

        // Convert EM -> layout Unit using font size
        return Unit{ ( static_cast<f32>( a_Face->height ) / static_cast<f32>( a_Face->units_per_EM ) ) * a_Style.Size };
    }

    /**
     * @brief Represents a loaded font, containing the FreeType face, the HarfBuzz font,
     *        a persistent HarfBuzz shaping buffer (reset between calls), and a per-size
     *        glyph advance cache to avoid redundant FT_Load_Glyph calls during measurement.
     */
    class Font
    {
    public:
        Font() = default;
        ~Font();

        Font( FT_Face a_Face, hb_font_t* a_Font, hb_buffer_t* a_Buffer, msdfgen::FontHandle* a_MsdfFont )
            : m_Face    ( a_Face )
            , m_Font    ( a_Font )
            , m_Buffer  ( a_Buffer )
            , m_MsdfFont( a_MsdfFont )
        {}

        // Non-copyable
        Font( const Font& ) = delete;
        Font& operator=( const Font& ) = delete;

        // Movable
        Font( Font&& a_Other ) noexcept;
        Font& operator=( Font&& a_Other ) noexcept;

        /** @brief Gets the FreeType face object. */
        FT_Face GetFace() const { return m_Face; }

        /** @brief Gets the HarfBuzz font object. */
        hb_font_t* GetHBFont() const { return m_Font; }

        /** @brief Gets the HarfBuzz buffer object. */
        hb_buffer_t* GetHBBuffer() const { return m_Buffer; }

        /** @brief Gets the msdfgen font handle. */
        msdfgen::FontHandle* GetMsdfFont() const { return m_MsdfFont; }

        /** @brief Checks if the font is valid (i.e., all underlying resources were successfully loaded). */
        bool IsValid() const { return m_Face != nullptr && m_Font != nullptr && m_Buffer != nullptr && m_MsdfFont != nullptr; }

        /**
         * @brief Loads a font from memory and prepares it for use, returning an optional Font object.
         * @param a_FTLib The FreeType library instance to use for loading the font
         * @param a_Data Pointer to the font data in memory
         * @param a_Size Size of the font data in bytes
         * @return An optional Font object containing the loaded font data and associated HarfBuzz objects, or NullOpt if loading failed.
         * @note The caller is responsible for ensuring that the memory pointed to by a_Data remains valid for the lifetime of the returned Font, 
         *       as FreeType may reference this memory directly without copying it.
         */
        static Optional<Font> LoadFromMemory( FT_Library a_FTLib, const void* a_Data, size a_Size );

        /**
         * @brief Loads a font from a file path and prepares it for use, returning an optional Font object.
         * @param a_FTLib The FreeType library instance to use for loading the font
         * @param a_FilePath The file path to the font file on disk, which should be a valid font format supported by FreeType (e.g., TTF, OTF).
         * @return An optional Font object containing the loaded font data and associated HarfBuzz objects, or NullOpt if loading failed.
         */
        static Optional<Font> LoadFromFile( FT_Library a_FTLib, const char* a_FilePath );

    private:
        static Optional<Font> LoadFromFace( FT_Face a_Face );
        void Destroy();

		// TODO: m_Buffer is not thread-safe if we want to shape text from multiple threads. We could either:
		//	   1. Add a mutex to protect access to m_Buffer, allowing it to be shared across threads at the cost of potential contention.
		//	   2. Remove m_Buffer from the Font class and require callers to create and manage their own hb_buffer_t instances for shaping, 
        //        which would allow for thread-local buffers without synchronization overhead.

        FT_Face              m_Face   = nullptr;   ///< The FreeType face object representing the loaded font, used for rasterization and metric queries.
        hb_font_t*           m_Font   = nullptr;   ///< Persistent HarfBuzz font object associated with the FreeType face, used for shaping operations.
        hb_buffer_t*         m_Buffer = nullptr;   ///< Persistent buffer that is reset and reused for each shaping operation to avoid repeated allocations.
        msdfgen::FontHandle* m_MsdfFont = nullptr; ///< The msdfgen font handle that wraps the FreeType face, used for SDF generation of glyph bitmaps.
    };

    class FontCache
    {
    public:
        FontCache();
        ~FontCache();

        FontCache( const FontCache& ) = delete;
        FontCache& operator=( const FontCache& ) = delete;

        FontCache( FontCache&& a_Other ) noexcept;
        FontCache& operator=( FontCache&& a_Other ) noexcept;

        bool IsValid() const { return m_Library != nullptr; }
        FT_Library GetLibrary() const { return m_Library; }

        bool RegisterFontHandle( FontHandle a_Handle, String a_FilePath );

        Font* GetFont( FontHandle a_Handle );

        Font* GetOrLoadFont( FontHandle a_Handle );

        void Evict( FontHandle a_Handle );

        size CachedCount() const { return Size( m_Cache ); }

    private:
        FT_Library m_Library{ nullptr };
        HashMap<FontHandle, Font>   m_Cache;
        HashMap<FontHandle, String>  m_HandleToPath;
    };

    /**
     * @brief Shapes a line of text using HarfBuzz.
     * TODO: direction, script, and language are currently hardcoded to LTR/Latin/en.
     *       These should be derived from the text content (e.g. via ICU or hb-unicode)
     *       to support RTL scripts and non-Latin writing systems correctly.
     */
    FontUnit ShapeLine(
        Font&                  a_Font,
        StringView             a_TextUTF8,
        const TextLayoutStyle& a_Layout,
        Array<ShapedGlyph>&    o_Glyphs );

} // namespace RatUI::FreeType
