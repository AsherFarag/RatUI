#pragma once
#include "../../RatUI.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

namespace RatUI::FreeType
{
    /**
     * @brief Returns the line height for the given style and FreeType face.
     * If TextStyle::LineHeight is non-zero, that value is used directly.
     * Otherwise, the line height is derived from the face's size metrics.
     */
    inline f32 GetLineHeight( FT_Face a_Face, const TextStyle& a_Style )
    {
        if ( a_Style.LineHeight > 0.f )
            return a_Style.LineHeight;

        return a_Face->size->metrics.height / 64.f;   
    }

    /**
     * @brief Represents a loaded font, containing both the FreeType face and the HarfBuzz font.
     */
    class Font
    {
    public:
        FT_Face FTFace = nullptr;    ///< The FreeType face object.
        hb_font_t* HBFont = nullptr;

        void Destroy()
        {
            if ( HBFont ) hb_font_destroy( HBFont );
            if ( FTFace ) FT_Done_Face( FTFace );
            HBFont = nullptr;
            FTFace = nullptr;
        }

        static Optional<Font> LoadFromMemory( FT_Library a_FTLib, const void* a_Data, size a_Size, u32 a_PixelSize )
        {
            FT_Face face;
            if ( FT_New_Memory_Face( a_FTLib, static_cast<const FT_Byte*>( a_Data ), static_cast<FT_Long>( a_Size ), 0, &face ) != 0 )
                return NullOpt; // Failed to load font from memory.

            if ( FT_Set_Pixel_Sizes( face, 0, static_cast<FT_UInt>( a_PixelSize ) ) != 0 )
            {
                FT_Done_Face( face );
                return NullOpt; // Failed to set pixel size.
            }

            hb_font_t* hbFont = hb_ft_font_create( face, nullptr );
            if ( !hbFont )
            {
                FT_Done_Face( face );
                return NullOpt; // Failed to create HarfBuzz font.
            }

            return Font{ face, hbFont };
        }

        static Optional<Font> LoadFromFile( FT_Library a_FTLib, const char* a_FilePath, u32 a_PixelSize )
        {
            FT_Face face;
            if ( FT_New_Face( a_FTLib, a_FilePath, 0, &face ) != 0 )
                return NullOpt; // Failed to load font file.

            if ( FT_Set_Pixel_Sizes( face, 0, static_cast<FT_UInt>( a_PixelSize ) ) != 0 )
            {
                FT_Done_Face( face );
                return NullOpt; // Failed to set pixel size.
            }

            hb_font_t* hbFont = hb_ft_font_create( face, nullptr );
            if ( !hbFont )
            {
                FT_Done_Face( face );
                return NullOpt; // Failed to create HarfBuzz font.
            }

            return Font{ face, hbFont };
        }

        Font() = default;
        ~Font() { Destroy(); }

        Font( FT_Face a_FTFace, hb_font_t* a_HBFont ) 
            : FTFace( a_FTFace )
            , HBFont( a_HBFont ) 
        {}

        // Non-copyable
        Font( const Font& ) = delete;
        Font& operator=( const Font& ) = delete;

        // Movable
        Font( Font&& a_Other ) noexcept
            : FTFace( std::exchange( a_Other.FTFace, nullptr ) )
            , HBFont( std::exchange( a_Other.HBFont, nullptr ) )
        {}

        Font& operator=( Font&& a_Other ) noexcept
        {
            if ( this == &a_Other ) return *this;
            Destroy();
            FTFace = std::exchange( a_Other.FTFace, nullptr );
            HBFont = std::exchange( a_Other.HBFont, nullptr );
            return *this;
        }
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

        // Non-copyable because it owns the FT_Library and Font handles.
        FontCache( const FontCache& ) = delete;
        FontCache& operator=( const FontCache& ) = delete;

        FontCache( FontCache&& a_Other ) noexcept
            : m_Library( std::exchange( a_Other.m_Library, nullptr ) )
            , m_Cache( std::move( a_Other.m_Cache ) )
            , m_HandleToPath( std::move( a_Other.m_HandleToPath ) )
        {}

        FontCache& operator=( FontCache&& a_Other ) noexcept
        {
            if ( this == &a_Other ) return *this;

            ::RatUI::Clear( m_Cache );
            ::RatUI::Clear( m_HandleToPath );
            if ( m_Library ) FT_Done_FreeType( m_Library );

            m_Library = std::exchange( a_Other.m_Library, nullptr );
            m_Cache   = std::move( a_Other.m_Cache );
            m_HandleToPath = std::move( a_Other.m_HandleToPath );
            return *this;
        }

        /** @brief Returns true if the FreeType library was initialised successfully. */
        bool IsValid() const { return m_Library != nullptr; }

        /** @brief Returns the underlying FT_Library handle. */
        FT_Library GetLibrary() const { return m_Library; }

        /** 
         * @brief Associates a FontHandle with a file path for later retrieval. 
         *        This is necessary because the public API uses FontHandles, but the cache needs file paths to load fonts.
         * @return True if the handle was valid and registration succeeded, false otherwise.
         * @note Registering the same handle twice overwrites the previous path.
         */
        bool RegisterFontHandle( FontHandle a_Handle, String a_FilePath )
        {
            if ( !a_Handle.IsValid() || Empty( a_FilePath ) )
                return false;

            m_HandleToPath[a_Handle] = std::move( a_FilePath );
            return true;
        }

        /**
         * @brief Retrieves a Font from the cache based on the given handle and pixel size.
         * @return A pointer to the Font if found, or nullptr if the handle is invalid or the font variant is not cached.
         */
        Font* GetFont( FontHandle a_Handle, u32 a_PixelSize )
        {
            if ( !a_Handle.IsValid() )
                return nullptr;

            auto it = Find( m_Cache, FontKey{ a_Handle, a_PixelSize } );
            return it != End( m_Cache ) ? &it->second : nullptr;
        }

        /**
         * @brief Retrieves a Font from the cache based on the given handle and pixel size.
         * If the font is not already cached, it attempts to load it using the registered file path.
         * @return A pointer to the Font if found or loaded successfully, or nullptr if the handle is invalid, no path is registered, or loading fails.
         */
        Font* GetOrLoadFont( FontHandle a_Handle, u32 a_PixelSize )
        {
            if ( Font* cachedFont = GetFont( a_Handle, a_PixelSize ) )
                return cachedFont; // Cache hit

            // Cache miss - need to load the font
            auto pathIt = Find( m_HandleToPath, a_Handle );
            if ( pathIt == End( m_HandleToPath ) )
                return nullptr; // No registered path for this handle

            const String& filePath = pathIt->second;
            auto fontOpt = Font::LoadFromFile( m_Library, CStr( filePath ), a_PixelSize );
            if ( !fontOpt )
                return nullptr; // Failed to load font

            auto [insertIt, success] = Emplace( m_Cache, FontKey{ a_Handle, a_PixelSize }, std::move( *fontOpt ) );
            return success ? &insertIt->second : nullptr;
        }

        /** @brief Evicts a specific font variant from the cache based on its handle and pixel size. */
        void Evict( FontHandle a_Handle, u32 a_PixelSize )
        {
            Erase( m_Cache, FontKey{ a_Handle, a_PixelSize } );
        }

        /** @brief Returns the number of currently cached Font variants. */
        size CachedCount() const { return Size( m_Cache ); }

    private:
        struct FontKey
        {
            FontHandle Handle;
            u32        PixelSize;

            bool operator==( const FontKey& ) const = default;
        };

        struct FontKeyHasher
        {
            size_t operator()( const FontKey& a_Key ) const noexcept
            {
                return std::hash<u32>{}( a_Key.Handle.ID ) ^ ( std::hash<u32>{}( a_Key.PixelSize ) << 1 );
            }
        };

        FT_Library m_Library{ nullptr };
        HashMap<FontKey, Font, FontKeyHasher> m_Cache;
        HashMap<FontHandle, String> m_HandleToPath;
    };

    /**
     * @brief Represents a single shaped glyph, containing the glyph ID and its positioning information.
     */
    struct ShapedGlyph
    {
        u32 GlyphID{0};
        u32 PixelSize{0};
        f32 XAdvance{0.f}, YAdvance{0.f};
        f32 XOffset{0.f}, YOffset{0.f};
    };

    inline void ShapeLine( hb_font_t* a_Font, StringView a_TextUTF8, u32 a_PixelSize, Array<ShapedGlyph>& o_Glyphs )
    {
        hb_buffer_t* buf = hb_buffer_create();

        // Set up the buffer with the text and properties
        hb_buffer_add_utf8( buf, Data( a_TextUTF8 ), static_cast<int>( Size( a_TextUTF8 ) ), 0, -1 );

        // TODO: we're hardcoding direction, script, and language here.
        hb_buffer_set_direction( buf, HB_DIRECTION_LTR );
        hb_buffer_set_script( buf, HB_SCRIPT_LATIN );
        hb_buffer_set_language( buf, hb_language_from_string( "en", -1 ) );

        hb_shape( a_Font, buf, nullptr, 0 );

        unsigned int glyphCount = 0;
        hb_glyph_info_t* infos = hb_buffer_get_glyph_infos( buf, &glyphCount );
        hb_glyph_position_t* positions = hb_buffer_get_glyph_positions( buf, &glyphCount );

        Clear( o_Glyphs );
        Reserve( o_Glyphs, glyphCount );

        // HarfBuzz positions are in 26.6 fixed-point (1/64 pixel), divide by 64
        for ( unsigned i = 0; i < glyphCount; ++i )
        {
            EmplaceBack( o_Glyphs,
                /*.GlyphID  */ infos[i].codepoint, // after shaping, codepoint is the glyph ID
                /*.PixelSize*/ a_PixelSize, // TODO: Do we need this here? In the render func we could just pass in a pixel size instead of per-glyph, since all glyphs in a line will be the same size?
                /*.XAdvance */ positions[i].x_advance / 64.f,
                /*.YAdvance */ positions[i].y_advance / 64.f,
                /*.XOffset  */ positions[i].x_offset / 64.f,
                /*.YOffset  */ positions[i].y_offset / 64.f
            );
        }

        hb_buffer_destroy( buf );
    }

} // namespace RatUI::FreeType