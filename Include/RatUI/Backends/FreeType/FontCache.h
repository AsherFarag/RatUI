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
        Font( FT_Face a_FTFace, hb_font_t* a_HBFont ) : FTFace( a_FTFace ), HBFont( a_HBFont ) {}
        ~Font() { Destroy(); }

        // Non-copyable
        Font( const Font& ) = delete;
        Font& operator=( const Font& ) = delete;

        // Movable
        Font( Font&& a_Other ) noexcept = default;
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