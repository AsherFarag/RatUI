#pragma once
#include "../../RatUI.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

namespace RatUI::FreeType
{
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
                m_Library = nullptr;
        }

        ~FontCache()
        {
            // Destroy all Font objects before releasing the FT_Library they reference.
            m_Cache.clear();
            if ( m_Library )
                FT_Done_FreeType( m_Library );
        }

        // Non-copyable because it owns the FT_Library and Font handles.
        FontCache( const FontCache& ) = delete;
        FontCache& operator=( const FontCache& ) = delete;

        FontCache( FontCache&& a_Other ) noexcept
            : m_Library( std::exchange( a_Other.m_Library, nullptr ) )
            , m_Cache( std::move( a_Other.m_Cache ) )
        {}

        FontCache& operator=( FontCache&& a_Other ) noexcept
        {
            if ( this != &a_Other )
            {
                this->~FontCache(); // Clean up current resources
                m_Library = std::exchange( a_Other.m_Library, nullptr );
                m_Cache   = std::move( a_Other.m_Cache );
            }
            return *this;
        }

        /** @brief Returns true if the FreeType library was initialised successfully. */
        bool IsValid() const { return m_Library != nullptr; }

        /** @brief Returns the underlying FT_Library handle. */
        FT_Library GetLibrary() const { return m_Library; }

        /**
         * @brief Returns a pointer to a cached Font for the given file path and pixel size,
         *        loading and caching it on first use.
         * @return A valid Font* on success, or nullptr if the library is uninitialised or loading fails.
         */
        Font* GetOrLoad( const char* a_FilePath, u32 a_PixelSize )
        {
            if ( !m_Library || !a_FilePath )
                return nullptr;

            FontKey key{ a_FilePath, a_PixelSize };
            auto it = Find( m_Cache, key );
            if ( it != End( m_Cache ) )
                return &it->second; // Found in cache, return it.

            Optional<Font> loaded = Font::LoadFromFile( m_Library, a_FilePath, a_PixelSize );
            if ( !loaded )
                return nullptr; // Failed to load font, return nullptr.

            auto result = Emplace( m_Cache, std::move( key ), std::move( *loaded ) );
            return &result.first->second;
        }

        /** @brief Removes the cached Font for the given file path and pixel size, if present. */
        void Evict( const char* a_FilePath, u32 a_PixelSize )
        {
            // TODO: Not a fan of requiring String alloc here
            if ( a_FilePath ) Erase( m_Cache, FontKey{ String( a_FilePath ), a_PixelSize } );
        }

        /** @brief Destroys all cached Font objects. */
        void Clear() { ::RatUI::Clear( m_Cache ); }

        /** @brief Returns the number of currently cached Font variants. */
        size CachedCount() const { return Size( m_Cache ); }

    private:
        struct FontKey
        {
            String Path;
            u32    PixelSize;

            bool operator==( const FontKey& ) const = default;
        };

        struct FontKeyHasher
        {
            size_t operator()( const FontKey& a_Key ) const noexcept
            {
                // FNV-1a hash with platform-appropriate constants (32-bit or 64-bit size_t).
                static constexpr size_t Basis = sizeof(size_t) == 8
                    ? size_t( 14695981039346656037ULL ) : size_t( 2166136261u );
                static constexpr size_t Prime = sizeof(size_t) == 8
                    ? size_t( 1099511628211ULL ) : size_t( 16777619u );

                size_t h = Basis;
                for ( unsigned char c : a_Key.Path )
                    h = ( h ^ static_cast<size_t>( c ) ) * Prime;
                h = ( h ^ std::hash<u32>{}( a_Key.PixelSize ) ) * Prime;
                return h;
            }
        };

        FT_Library m_Library{ nullptr };
        HashMap<FontKey, Font, FontKeyHasher> m_Cache;
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

    inline Array<ShapedGlyph> ShapeLine( hb_font_t* a_Font, StringView a_TextUTF8, u32 a_PixelSize )
    {
        hb_buffer_t* buf = hb_buffer_create();

        // Set up the buffer with the text and properties
        hb_buffer_add_utf8(buf, Data( a_TextUTF8 ), static_cast<int>( Size( a_TextUTF8 ) ), 0, -1 );

        // TODO: we're hardcoding direction, script, and language here.
        hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
        hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
        hb_buffer_set_language(buf, hb_language_from_string("en", -1));

        hb_shape(a_Font, buf, nullptr, 0);

        unsigned int glyphCount = 0;
        hb_glyph_info_t*     infos     = hb_buffer_get_glyph_infos(buf, &glyphCount);
        hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buf, &glyphCount);

        Array<ShapedGlyph> result;
        Reserve( result, glyphCount );

        // HarfBuzz positions are in 26.6 fixed-point (1/64 pixel), divide by 64
        for (unsigned i = 0; i < glyphCount; ++i)
        {
            EmplaceBack( result,
                /*.GlyphID  */ infos[i].codepoint, // after shaping, codepoint is the glyph ID
                /*.PixelSize*/ a_PixelSize, // TODO: Do we need this here? In the render func we could just pass in a pixel size instead of per-glyph, since all glyphs in a line will be the same size?
                /*.XAdvance */ positions[i].x_advance / 64.f,
                /*.YAdvance */ positions[i].y_advance / 64.f,
                /*.XOffset  */ positions[i].x_offset  / 64.f,
                /*.YOffset  */ positions[i].y_offset  / 64.f
            );
        }

        hb_buffer_destroy(buf);
        return result;
    }

} // namespace RatUI::FreeType