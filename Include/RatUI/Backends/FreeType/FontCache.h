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
        ~Font() { Destroy(); }

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
        Font( Font&& a_Other ) noexcept
            : m_Face    ( std::exchange( a_Other.m_Face,    nullptr ) )
            , m_Font    ( std::exchange( a_Other.m_Font,    nullptr ) )
            , m_Buffer  ( std::exchange( a_Other.m_Buffer,  nullptr ) )
            , m_MsdfFont( std::exchange( a_Other.m_MsdfFont, nullptr ) )
        {}

        Font& operator=( Font&& a_Other ) noexcept
        {
            if ( this == &a_Other ) return *this;
            Destroy();
            m_Face        = std::exchange( a_Other.m_Face,   nullptr );
            m_Font        = std::exchange( a_Other.m_Font,   nullptr );
            m_Buffer      = std::exchange( a_Other.m_Buffer, nullptr );
            m_MsdfFont    = std::exchange( a_Other.m_MsdfFont, nullptr );   
            return *this;
        }

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
        static Optional<Font> LoadFromMemory( FT_Library a_FTLib, const void* a_Data, size a_Size )
        {
            FT_Face face;
            if ( FT_New_Memory_Face( a_FTLib, static_cast<const FT_Byte*>( a_Data ), static_cast<FT_Long>( a_Size ), 0, &face ) != 0 )
                return NullOpt;

            return LoadFromFace( face );
        }

        /**
         * @brief Loads a font from a file path and prepares it for use, returning an optional Font object.
         * @param a_FTLib The FreeType library instance to use for loading the font
         * @param a_FilePath The file path to the font file on disk, which should be a valid font format supported by FreeType (e.g., TTF, OTF).
         * @return An optional Font object containing the loaded font data and associated HarfBuzz objects, or NullOpt if loading failed.
         */
        static Optional<Font> LoadFromFile( FT_Library a_FTLib, const char* a_FilePath )
        {
            FT_Face face;
            if ( FT_New_Face( a_FTLib, a_FilePath, 0, &face ) != 0 )
                return NullOpt;

            return LoadFromFace( face );
        }

    private:
        static Optional<Font> LoadFromFace( FT_Face a_Face )
        {
			// NOTE: We set the character size to 64x64 pixels at 72 DPI, which is a common default for SDF generation and provides a good balance of precision and performance for typical UI font sizes.
            if ( FT_Set_Char_Size( a_Face, 0, 64 * 64, 0, 0 ) != 0 )
            {
                FT_Done_Face( a_Face );
                return NullOpt;
            }

            hb_font_t* hbFont = hb_ft_font_create( a_Face, nullptr );
            if ( !hbFont )
            {
                FT_Done_Face( a_Face );
                return NullOpt;
            }

            hb_buffer_t* hbBuf = hb_buffer_create();
            if ( !hbBuf )
            {
                hb_font_destroy( hbFont );
                FT_Done_Face( a_Face );
                return NullOpt;
            }

            msdfgen::FontHandle* msdfFont = msdfgen::adoptFreetypeFont( a_Face );
            if ( !msdfFont )
            {
                hb_buffer_destroy( hbBuf );
                hb_font_destroy( hbFont );
                FT_Done_Face( a_Face );
                return NullOpt;
            }

            return Font{ a_Face, hbFont, hbBuf, msdfFont };
        }

        void Destroy()
        {
            if ( m_Buffer   ) hb_buffer_destroy(    std::exchange( m_Buffer,   nullptr ) );
            if ( m_MsdfFont ) msdfgen::destroyFont( std::exchange( m_MsdfFont, nullptr ) );
            if ( m_Font     ) hb_font_destroy(      std::exchange( m_Font,     nullptr ) );
            if ( m_Face     ) FT_Done_Face(         std::exchange( m_Face,     nullptr ) );
        }

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
        FontCache()
        {
            if ( FT_Init_FreeType( &m_Library ) != 0 )
            {
                RATUI_ASSERT( false, "Failed to initialize FreeType library." );
                m_Library = nullptr;
            }
        }

        ~FontCache()
        {
            ::RatUI::Clear( m_Cache );
            ::RatUI::Clear( m_HandleToPath );
            if ( m_Library ) FT_Done_FreeType( m_Library );
        }

        FontCache( const FontCache& ) = delete;
        FontCache& operator=( const FontCache& ) = delete;

        FontCache( FontCache&& a_Other ) noexcept
            : m_Library      ( std::exchange( a_Other.m_Library, nullptr ) )
            , m_Cache        ( std::move( a_Other.m_Cache ) )
            , m_HandleToPath ( std::move( a_Other.m_HandleToPath ) )
        {}

        FontCache& operator=( FontCache&& a_Other ) noexcept
        {
            if ( this == &a_Other ) return *this;

            ::RatUI::Clear( m_Cache );
            ::RatUI::Clear( m_HandleToPath );
            if ( m_Library ) FT_Done_FreeType( m_Library );

            m_Library      = std::exchange( a_Other.m_Library, nullptr );
            m_Cache        = std::move( a_Other.m_Cache );
            m_HandleToPath = std::move( a_Other.m_HandleToPath );
            return *this;
        }

        bool IsValid() const { return m_Library != nullptr; }
        FT_Library GetLibrary() const { return m_Library; }

        bool RegisterFontHandle( FontHandle a_Handle, String a_FilePath )
        {
            if ( !a_Handle.IsValid() || Empty( a_FilePath ) )
                return false;

            m_HandleToPath[a_Handle] = std::move( a_FilePath );
            return true;
        }

        Font* GetFont( FontHandle a_Handle )
        {
            if ( !a_Handle.IsValid() )
                return nullptr;

            auto it = Find( m_Cache, a_Handle );
            return it != End( m_Cache ) ? &it->second : nullptr;
        }

        Font* GetOrLoadFont( FontHandle a_Handle )
        {
            if ( Font* cachedFont = GetFont( a_Handle ) )
                return cachedFont;

            auto pathIt = Find( m_HandleToPath, a_Handle );
            if ( pathIt == End( m_HandleToPath ) )
                return nullptr;

            const String& filePath = pathIt->second;
            auto fontOpt = Font::LoadFromFile( m_Library, CStr( filePath ) );
            if ( !fontOpt )
                return nullptr;

            auto [insertIt, success] = Emplace( m_Cache, a_Handle, std::move( *fontOpt ) );
            return success ? &insertIt->second : nullptr;
        }

        void Evict( FontHandle a_Handle )
        {
			Erase( m_Cache, a_Handle );
        }

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
    inline FontUnit ShapeLine(
        Font&                  a_Font,
        StringView             a_TextUTF8,
        const TextLayoutStyle& a_Layout,
        Array<ShapedGlyph>&    o_Glyphs
    )
    {
        if ( Empty( a_TextUTF8 ) )
            return 0_fu;
 
        hb_buffer_t* buf = a_Font.GetHBBuffer();
        hb_buffer_reset( buf );
 
        hb_buffer_add_utf8(
            buf,
            Data( a_TextUTF8 ),
            static_cast<int>( Size( a_TextUTF8 ) ),
            0,
            -1
        );
 
        // TODO: Should be user defined not auto guessed
        hb_buffer_guess_segment_properties( buf ); // auto direction/script/lang
 
        hb_shape(
            a_Font.GetHBFont(),
            buf,
            /*features=*/nullptr, /*num_features=*/0
        );
 
        unsigned int glyphCount = 0;
 
        hb_glyph_info_t* infos =
            hb_buffer_get_glyph_infos( buf, &glyphCount );
 
        hb_glyph_position_t* positions =
            hb_buffer_get_glyph_positions( buf, &glyphCount );
 
        Clear( o_Glyphs );
        Reserve( o_Glyphs, glyphCount );

		// hb_ft_font_create inherits the FreeType face's current pixel size.
        // HarfBuzz positions are in 26.6 fixed-point *scaled pixel* units:
        //   raw_value / 64  -> pixel advance at the face's base render size (ppem)
        //   / ppem          -> EM-normalised [0, 1] FontUnit
        // The combined divisor is 64 * ppem.
        const f32 ppem      = static_cast<f32>( a_Font.GetFace()->size->metrics.y_ppem );
        RATUI_ASSERT( ppem > 0.f, "Invalid font metrics: y_ppem must be > 0." );
        const f32 emNormDiv = 64.f * ppem;
		const Unit fontSize = a_Layout.Size;
 
        // Letter and word spacing arrive as Unit (display-space). Convert to EM-normalised
        // FontUnit so they can be added directly to the HB advances after normalisation.
        const FontUnit letterSpacing = ToFontUnit( a_Layout.LetterSpacing, fontSize );
        const FontUnit wordSpacing   = ToFontUnit( a_Layout.WordSpacing,   fontSize );
 
        FontUnit lineWidth = 0_fu;
 
        // Track the last cluster we checked for word-spacing so that multi-glyph
        // clusters (e.g. ligature components) only receive the extra advance once.
        constexpr u32 c_NoCluster = Limits<u32>::max();
        u32 lastWordSpacingCluster = c_NoCluster;
 
        for ( unsigned i = 0; i < glyphCount; ++i )
        {
            // Convert HarfBuzz 26.6 scaled-pixel positions to EM-normalised FontUnit.
            // Multiplying by fontSizePx at render time then yields correct screen pixels.
            FontUnit xAdvance = FontUnit{ static_cast<f32>( positions[i].x_advance ) / emNormDiv };
            FontUnit yAdvance = FontUnit{ static_cast<f32>( positions[i].y_advance ) / emNormDiv };
            FontUnit xOffset  = FontUnit{ static_cast<f32>( positions[i].x_offset  ) / emNormDiv };
            FontUnit yOffset  = FontUnit{ static_cast<f32>( positions[i].y_offset  ) / emNormDiv };
 
            // Apply letter spacing between clusters, not inside a multi-glyph cluster.
            if ( letterSpacing != 0_fu && i + 1 < glyphCount && infos[i].cluster != infos[i + 1].cluster )
                xAdvance += letterSpacing;
 
            // Apply word spacing for whitespace glyphs (once per unique whitespace cluster).
            if ( wordSpacing != 0_fu && infos[i].cluster != lastWordSpacingCluster )
            {
                lastWordSpacingCluster = infos[i].cluster;
                if ( Unicode::IsWhitespaceCluster( a_TextUTF8, infos[i].cluster ) )
                    xAdvance += wordSpacing;
            }
 
            EmplaceBack(
                o_Glyphs,
                /* GlyphID   */ infos[i].codepoint,
                /* XAdvance  */ xAdvance,
                /* YAdvance  */ yAdvance,
                /* XOffset   */ xOffset,
                /* YOffset   */ yOffset
            );
 
            lineWidth += xAdvance;
        }
 
        return lineWidth;
    }

} // namespace RatUI::FreeType
