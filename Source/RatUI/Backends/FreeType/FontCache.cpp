#include <RatUI/Backends/FreeType/FontCache.h>

#include <utility>

namespace RatUI::FreeType
{
    Font::~Font()
    {
        Destroy();
    }

    Font::Font( Font&& a_Other ) noexcept
        : m_Face( std::exchange( a_Other.m_Face, nullptr ) )
        , m_Font( std::exchange( a_Other.m_Font, nullptr ) )
        , m_Buffer( std::exchange( a_Other.m_Buffer, nullptr ) )
        , m_MsdfFont( std::exchange( a_Other.m_MsdfFont, nullptr ) )
    {}

    Font& Font::operator=( Font&& a_Other ) noexcept
    {
        if ( this == &a_Other )
            return *this;
        Destroy();
        m_Face = std::exchange( a_Other.m_Face, nullptr );
        m_Font = std::exchange( a_Other.m_Font, nullptr );
        m_Buffer = std::exchange( a_Other.m_Buffer, nullptr );
        m_MsdfFont = std::exchange( a_Other.m_MsdfFont, nullptr );
        return *this;
    }

    Optional<Font> Font::LoadFromMemory( FT_Library a_FTLib, const void* a_Data, size a_Size )
    {
        FT_Face face;
        if ( FT_New_Memory_Face( a_FTLib, static_cast<const FT_Byte*>( a_Data ), static_cast<FT_Long>( a_Size ), 0, &face ) != 0 )
            return NullOpt;

        return LoadFromFace( face );
    }

    Optional<Font> Font::LoadFromFile( FT_Library a_FTLib, const char* a_FilePath )
    {
        FT_Face face;
        if ( FT_New_Face( a_FTLib, a_FilePath, 0, &face ) != 0 )
            return NullOpt;

        return LoadFromFace( face );
    }

    Optional<Font> Font::LoadFromFace( FT_Face a_Face )
    {
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

    void Font::Destroy()
    {
        if ( m_Buffer )
            hb_buffer_destroy( std::exchange( m_Buffer, nullptr ) );
        if ( m_MsdfFont )
            msdfgen::destroyFont( std::exchange( m_MsdfFont, nullptr ) );
        if ( m_Font )
            hb_font_destroy( std::exchange( m_Font, nullptr ) );
        if ( m_Face )
            FT_Done_Face( std::exchange( m_Face, nullptr ) );
    }

    FontCache::FontCache()
    {
        if ( FT_Init_FreeType( &m_Library ) != 0 )
        {
            RATUI_ASSERT( false, "Failed to initialize FreeType library." );
            m_Library = nullptr;
        }
    }

    FontCache::~FontCache()
    {
        ::RatUI::Clear( m_Cache );
        ::RatUI::Clear( m_HandleToPath );
        if ( m_Library )
            FT_Done_FreeType( m_Library );
    }

    FontCache::FontCache( FontCache&& a_Other ) noexcept
        : m_Library( std::exchange( a_Other.m_Library, nullptr ) )
        , m_Cache( std::move( a_Other.m_Cache ) )
        , m_HandleToPath( std::move( a_Other.m_HandleToPath ) )
    {}

    FontCache& FontCache::operator=( FontCache&& a_Other ) noexcept
    {
        if ( this == &a_Other )
            return *this;

        ::RatUI::Clear( m_Cache );
        ::RatUI::Clear( m_HandleToPath );
        if ( m_Library )
            FT_Done_FreeType( m_Library );

        m_Library = std::exchange( a_Other.m_Library, nullptr );
        m_Cache = std::move( a_Other.m_Cache );
        m_HandleToPath = std::move( a_Other.m_HandleToPath );
        return *this;
    }

    bool FontCache::RegisterFontHandle( FontHandle a_Handle, String a_FilePath )
    {
        if ( !a_Handle.IsValid() || Empty( a_FilePath ) )
            return false;

        m_HandleToPath[a_Handle] = std::move( a_FilePath );
        return true;
    }

    Font* FontCache::GetFont( FontHandle a_Handle )
    {
        if ( !a_Handle.IsValid() )
            return nullptr;

        auto it = Find( m_Cache, a_Handle );
        return it != End( m_Cache ) ? &it->second : nullptr;
    }

    Font* FontCache::GetOrLoadFont( FontHandle a_Handle )
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

    void FontCache::Evict( FontHandle a_Handle )
    {
        Erase( m_Cache, a_Handle );
    }

    FontUnit ShapeLine( Font& a_Font, StringView a_TextUTF8, const TextLayoutStyle& a_Layout, Array<ShapedGlyph>& o_Glyphs )
    {
        if ( Empty( a_TextUTF8 ) )
            return 0_fu;

        hb_buffer_t* buf = a_Font.GetHBBuffer();
        hb_buffer_reset( buf );

        hb_buffer_add_utf8( buf, Data( a_TextUTF8 ), static_cast<int>( Size( a_TextUTF8 ) ), 0, -1 );

        switch ( a_Layout.Direction )
        {
            case ETextDirection::Auto: break; // Let Harfbuzz guess the direction based on the first strong character
            case ETextDirection::LTR: hb_buffer_set_direction( buf, HB_DIRECTION_LTR ); break;
            case ETextDirection::RTL: hb_buffer_set_direction( buf, HB_DIRECTION_RTL ); break;
        }

        if ( a_Layout.Script != EScript::Invalid )
            hb_buffer_set_script( buf, static_cast<hb_script_t>( a_Layout.Script ) );

        //hb_buffer_set_language( buf, hb_language_get_default() ); // TODO: Expose this?
        hb_buffer_guess_segment_properties( buf );

        hb_shape( a_Font.GetHBFont(), buf, nullptr, 0 );

        u32 glyphCount = 0;
        hb_glyph_info_t* infos = hb_buffer_get_glyph_infos( buf, &glyphCount );
        hb_glyph_position_t* positions = hb_buffer_get_glyph_positions( buf, &glyphCount );

        Clear( o_Glyphs );
        Reserve( o_Glyphs, glyphCount );

        const f32 ppem = static_cast<f32>( a_Font.GetFace()->size->metrics.y_ppem );
        const f32 emNormDiv = 64.f * ppem;
        const Unit fontSize = a_Layout.Size;

        RATUI_ASSERT( ppem > 0.f, "Invalid font metrics: y_ppem must be > 0." );

        const FontUnit letterSpacing = ToFontUnit( a_Layout.LetterSpacing, fontSize );
        const FontUnit wordSpacing   = ToFontUnit( a_Layout.WordSpacing, fontSize );

        FontUnit lineWidth = 0_fu;

        u32 lastWordSpacingCluster = Limits<u32>::max();

        for ( u32 i = 0; i < glyphCount; ++i )
        {
            FontUnit xAdvance = FontUnit{ static_cast<f32>( positions[i].x_advance ) / emNormDiv };
            FontUnit yAdvance = FontUnit{ static_cast<f32>( positions[i].y_advance ) / emNormDiv };
            FontUnit xOffset  = FontUnit{ static_cast<f32>( positions[i].x_offset ) / emNormDiv };
            FontUnit yOffset  = FontUnit{ static_cast<f32>( positions[i].y_offset ) / emNormDiv };

			// Only apply letter spacing if this glyph is not the last one in a cluster, 
            // to avoid adding extra space between ligatures and other multi-codepoint clusters
            if ( letterSpacing != 0_fu && 
                 i + 1 < glyphCount && 
                 infos[i].cluster != infos[i + 1].cluster )
                xAdvance += letterSpacing;

			// Only apply word spacing if this glyph is the last one in a cluster,
            // and that cluster corresponds to a whitespace character, to avoid 
            // adding extra space between words that are separated by multiple whitespace characters
            if ( wordSpacing != 0_fu && infos[i].cluster != lastWordSpacingCluster )
            {
                lastWordSpacingCluster = infos[i].cluster;
                if ( Unicode::IsWhitespaceCluster( a_TextUTF8, infos[i].cluster ) )
                    xAdvance += wordSpacing;
            }


			EmplaceBack( o_Glyphs, ShapedGlyph{
                .GlyphIndex = GlyphID{ infos[i].codepoint }, // Harfbuzz codepoint becomes a glyph index once the text is shaped
				.XAdvance = xAdvance,
				.YAdvance = yAdvance,
				.XOffset = xOffset,
				.YOffset = yOffset
			} );

            lineWidth += xAdvance;
        }

        return lineWidth;
    }

} // namespace RatUI::FreeType
