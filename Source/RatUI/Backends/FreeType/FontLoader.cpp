#include <RatUI/Backends/FreeType/FontLoader.h>
#include <RatUI/Text/PixelGlyphOps.h>
#include <RatUI/Text/TextShaping.h>

#include FT_OUTLINE_H
#include FT_TRUETYPE_TABLES_H
#include <hb-ft.h>
#include <hb-ot.h>
#include <msdfgen.h>
#include <msdfgen-ext.h>

#include <algorithm>
#include <cmath>

namespace RatUI::FreeType
{
    LibraryHandle::~LibraryHandle()
    {
        if ( Library )
            FT_Done_FreeType( Library );
    }

    // =========================================================================
    // FontLoader
    // =========================================================================

    FontLoader::FontLoader()
    {
        m_Library = MakeShared<LibraryHandle>();
        if ( FT_Init_FreeType( &m_Library->Library ) != 0 )
        {
            RATUI_ASSERT( false, "Failed to initialize FreeType library." );
            m_Library->Library = nullptr;
        }
    }

    Unique<IFontFace> FontLoader::LoadFromFile( const char* a_FilePath, const FontFaceOptions& a_Options )
    {
        if ( !IsValid() || !a_FilePath )
            return nullptr;

        FT_Face face = nullptr;
        if ( FT_New_Face( m_Library->Library, a_FilePath, static_cast<FT_Long>( a_Options.FaceIndex ), &face ) != 0 )
            return nullptr;

        return Finish( face, {}, a_Options );
    }

    Unique<IFontFace> FontLoader::LoadFromMemory( const void* a_Data, size a_Size, const FontFaceOptions& a_Options )
    {
        if ( !IsValid() || !a_Data || a_Size == 0 )
            return nullptr;

        // FreeType reads from the buffer for the face's lifetime.
        Array<u8> data( static_cast<const u8*>( a_Data ), static_cast<const u8*>( a_Data ) + a_Size );

        FT_Face face = nullptr;
        if ( FT_New_Memory_Face( m_Library->Library, Data( data ), static_cast<FT_Long>( Size( data ) ),
                                 static_cast<FT_Long>( a_Options.FaceIndex ), &face ) != 0 )
            return nullptr;

        return Finish( face, std::move( data ), a_Options );
    }

    u32 FontLoader::GetFaceCount( const char* a_FilePath ) const
    {
        if ( !IsValid() || !a_FilePath )
            return 0;

        // Face index -1 only queries the face count.
        FT_Face face = nullptr;
        if ( FT_New_Face( m_Library->Library, a_FilePath, -1, &face ) != 0 )
            return 0;

        const u32 count = static_cast<u32>( face->num_faces );
        FT_Done_Face( face );
        return count;
    }

    Unique<IFontFace> FontLoader::Finish( FT_Face a_Face, Array<u8> a_Data, const FontFaceOptions& a_Options )
    {
        Unique<FontFace> face{ new FontFace() };
        if ( !face->Initialize( m_Library, a_Face, std::move( a_Data ), a_Options ) )
            return nullptr; // Initialize() released a_Face.
        return face;
    }

    // =========================================================================
    // FontFace: setup and metrics
    // =========================================================================

    FontFace::~FontFace()
    {
        if ( m_HBBuffer )
            hb_buffer_destroy( m_HBBuffer );
        if ( m_HBFont )
            hb_font_destroy( m_HBFont );
        if ( m_MsdfFont )
            msdfgen::destroyFont( m_MsdfFont );
        if ( m_Face )
            FT_Done_Face( m_Face );
    }

    bool FontFace::Initialize( Shared<LibraryHandle> a_Library, FT_Face a_Face, Array<u8> a_Data, const FontFaceOptions& a_Options )
    {
        m_Library = std::move( a_Library );
        m_Data    = std::move( a_Data );
        m_Face    = a_Face;

        if ( !FT_IS_SCALABLE( m_Face ) || m_Face->units_per_EM == 0 )
            return false; // TODO: Bitmap-only formats (.fon / .bdf / .pcf) could be supported as Pixel faces.

        const f32 unitsPerEm = static_cast<f32>( m_Face->units_per_EM );
        m_RcpUnitsPerEm = 1.f / unitsPerEm;

        // Shape at design scale with HarfBuzz's own functions, so it doesn't depend on FreeType's current pixel size.
        hb_face_t* hbFace = hb_ft_face_create_referenced( m_Face );
        m_HBFont = hb_font_create( hbFace );
        hb_face_destroy( hbFace );
        if ( !m_HBFont )
            return false;

        hb_ot_font_set_funcs( m_HBFont );
        hb_font_set_scale( m_HBFont, static_cast<int>( unitsPerEm ), static_cast<int>( unitsPerEm ) );

        m_HBBuffer = hb_buffer_create();
        if ( !m_HBBuffer )
            return false;

        if ( a_Options.Mode == EGlyphRenderMode::MTSDF )
        {
            m_MsdfFont = msdfgen::adoptFreetypeFont( m_Face );
            if ( !m_MsdfFont )
                return false;
        }

        // - Metrics
        m_Metrics.Mode = a_Options.Mode;
        {
            const auto em = [&]( FT_Short a_Units ) { return FontUnit{ static_cast<f32>( a_Units ) * m_RcpUnitsPerEm }; };

            m_Metrics.Ascender           = em( m_Face->ascender );
            m_Metrics.Descender          = em( m_Face->descender );
            m_Metrics.LineGap            = FontUnit{ std::max( 0.f, static_cast<f32>( m_Face->height - ( m_Face->ascender - m_Face->descender ) ) * m_RcpUnitsPerEm ) };
            m_Metrics.UnderlinePosition  = em( m_Face->underline_position );
            m_Metrics.UnderlineThickness = FontUnit{ std::max( 1.f, static_cast<f32>( m_Face->underline_thickness ) ) * m_RcpUnitsPerEm };

            if ( const TT_OS2* os2 = static_cast<const TT_OS2*>( FT_Get_Sfnt_Table( m_Face, FT_SFNT_OS2 ) ); os2 && os2->version != 0xFFFF )
            {
                if ( os2->yStrikeoutSize > 0 )
                {
                    m_Metrics.StrikeoutThickness = em( os2->yStrikeoutSize );
                    m_Metrics.StrikeoutPosition  = FontUnit{ ( static_cast<f32>( os2->yStrikeoutPosition ) + static_cast<f32>( os2->yStrikeoutSize ) * 0.5f ) * m_RcpUnitsPerEm };
                }
                if ( os2->version >= 2 && os2->sxHeight > 0 )
                    m_Metrics.XHeight = em( os2->sxHeight );
                if ( os2->version >= 2 && os2->sCapHeight > 0 )
                    m_Metrics.CapHeight = em( os2->sCapHeight );
            }
            else
            {
                m_Metrics.StrikeoutThickness = m_Metrics.UnderlineThickness;
                m_Metrics.StrikeoutPosition  = FontUnit{ m_Metrics.XHeight.ToFloat() * 0.5f };
            }
        }

        if ( a_Options.Mode == EGlyphRenderMode::Pixel )
        {
            if ( a_Options.NativePixelSize > 0.f )
            {
                m_Metrics.NativePixelSize = a_Options.NativePixelSize;
                m_PixelGridSource = EPixelGridSource::Manual;
            }
            else if ( const f32 detected = DetectPixelGrid( m_Face, &m_PixelGridSource ); detected > 0.f )
            {
                m_Metrics.NativePixelSize = detected;
            }
            else
            {
                m_Metrics.NativePixelSize = 16.f;
                m_PixelGridSource = EPixelGridSource::Fallback;
            }

            m_GridUnits = unitsPerEm / m_Metrics.NativePixelSize;
        }

        m_Name = String{ a_Options.Name };
        if ( Empty( m_Name ) && m_Face->family_name )
        {
            m_Name = m_Face->family_name;
            if ( m_Face->style_name )
            {
                m_Name += ' ';
                m_Name += m_Face->style_name;
            }
        }

        return true;
    }

    f32 FontFace::DetectPixelGrid( FT_Face a_Face, EPixelGridSource* o_Source )
    {
        if ( FT_HAS_FIXED_SIZES( a_Face ) && a_Face->num_fixed_sizes > 0 )
        {
            FT_Pos smallest = a_Face->available_sizes[0].y_ppem;
            for ( FT_Int i = 1; i < a_Face->num_fixed_sizes; ++i )
                smallest = std::min( smallest, a_Face->available_sizes[i].y_ppem );

            if ( o_Source )
                *o_Source = EPixelGridSource::EmbeddedBitmap;
            return static_cast<f32>( smallest ) / 64.f; // 26.6 -> pixels.
        }

        if ( !FT_IS_SCALABLE( a_Face ) || a_Face->units_per_EM == 0 )
            return 0.f;

        // Use distances between coordinates, not positions, so glyphs offset from the grid still reveal it.
        Array<f64> distances;
        Array<FT_Pos> xs, ys;
        const auto addDistances = [&]( Array<FT_Pos>& a_Values )
        {
            std::sort( Begin( a_Values ), End( a_Values ) );
            for ( size i = 1; i < Size( a_Values ); ++i )
                if ( a_Values[i] != a_Values[i - 1] )
                    PushBack( distances, static_cast<f64>( a_Values[i] - a_Values[i - 1] ) );
        };

        for ( const codepoint cp : { U'H', U'o', U'g', U'M', U'x', U'0', U'A', U'e', U'k' } )
        {
            const FT_UInt glyph = FT_Get_Char_Index( a_Face, cp );
            if ( glyph == 0 || FT_Load_Glyph( a_Face, glyph, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP ) != 0 )
                continue;

            const FT_Outline& outline = a_Face->glyph->outline;
            Clear( xs );
            Clear( ys );
            for ( short i = 0; i < outline.n_points; ++i )
            {
                PushBack( xs, outline.points[i].x );
                PushBack( ys, outline.points[i].y );
            }
            addDistances( xs );
            addDistances( ys );
        }

        if ( Size( distances ) < 4 )
            return 0.f;

        // Coarsest spacing all distances are multiples of. Outline fonts only fit tiny spacings, which the limits reject.
        constexpr f64 c_MinPixelsPerEm = 4.0, c_MaxPixelsPerEm = 64.0, c_Tolerance = 0.06;
        const f64 unitsPerEm = static_cast<f64>( a_Face->units_per_EM );

        f64 best = 0.0;
        for ( const f64 distance : distances )
        {
            for ( i32 divisor = 1; divisor <= 4; ++divisor )
            {
                const f64 spacing = distance / divisor;
                const f64 pixelsPerEm = unitsPerEm / spacing;
                if ( spacing <= best || pixelsPerEm < c_MinPixelsPerEm || pixelsPerEm > c_MaxPixelsPerEm )
                    continue;

                const bool fits = std::all_of( Begin( distances ), End( distances ), [&]( f64 a_Distance )
                {
                    const f64 steps = a_Distance / spacing;
                    return std::abs( steps - std::round( steps ) ) < c_Tolerance;
                } );

                if ( fits )
                    best = spacing;
            }
        }

        if ( best <= 0.0 )
            return 0.f;

        if ( o_Source )
            *o_Source = EPixelGridSource::OutlineGrid;
        return static_cast<f32>( unitsPerEm / best );
    }

    GlyphID FontFace::GetGlyphIndex( codepoint a_Codepoint ) const
    {
        return GlyphID{ FT_Get_Char_Index( m_Face, static_cast<FT_ULong>( a_Codepoint ) ) };
    }

    FontUnit FontFace::GetAdvance( GlyphID a_Glyph ) const
    {
        return FontUnit{ static_cast<f32>( hb_font_get_glyph_h_advance( m_HBFont, ToUnderlying( a_Glyph ) ) ) * m_RcpUnitsPerEm };
    }

    FontUnit FontFace::GetKerning( GlyphID a_Left, GlyphID a_Right ) const
    {
        // Legacy 'kern' table only, ShapeRun() handles GPOS.
        if ( !FT_HAS_KERNING( m_Face ) )
            return 0_fu;

        FT_Vector kerning{};
        if ( FT_Get_Kerning( m_Face, ToUnderlying( a_Left ), ToUnderlying( a_Right ), FT_KERNING_UNSCALED, &kerning ) != 0 )
            return 0_fu;

        return FontUnit{ static_cast<f32>( kerning.x ) * m_RcpUnitsPerEm };
    }

    // =========================================================================
    // Shaping
    // =========================================================================

    void FontFace::ShapeRun( StringView a_Text, const ShapeRunParams& a_Params, Array<ShapedGlyph>& o_Glyphs )
    {
        if ( Empty( a_Text ) )
            return;

        hb_buffer_t* buffer = m_HBBuffer;
        hb_buffer_reset( buffer );
        hb_buffer_add_utf8( buffer, Data( a_Text ), static_cast<int>( Size( a_Text ) ), 0, static_cast<int>( Size( a_Text ) ) );

        switch ( a_Params.Direction )
        {
            case ETextDirection::LTR: hb_buffer_set_direction( buffer, HB_DIRECTION_LTR ); break;
            case ETextDirection::RTL: hb_buffer_set_direction( buffer, HB_DIRECTION_RTL ); break;
            case ETextDirection::Auto:
            default: break; // Guessed from the text below.
        }

        if ( a_Params.Script != EScript::Invalid )
            hb_buffer_set_script( buffer, static_cast<hb_script_t>( a_Params.Script ) );

        //hb_buffer_set_language( buffer, hb_language_get_default() ); // TODO: Expose language?
        hb_buffer_guess_segment_properties( buffer );
        hb_shape( m_HBFont, buffer, nullptr, 0 );

        u32 glyphCount = 0;
        const hb_glyph_info_t*     infos     = hb_buffer_get_glyph_infos( buffer, &glyphCount );
        const hb_glyph_position_t* positions = hb_buffer_get_glyph_positions( buffer, &glyphCount );

        const f32 toUnits = m_RcpUnitsPerEm * a_Params.Size.ToFloat();

        Reserve( o_Glyphs, Size( o_Glyphs ) + glyphCount );
        for ( u32 i = 0; i < glyphCount; ++i )
        {
            EmplaceBack( o_Glyphs, ShapedGlyph{
                .GlyphIndex = GlyphID{ infos[i].codepoint }, // After shaping, codepoint holds the glyph index.
                .Cluster    = a_Params.ClusterBase + infos[i].cluster,
                .XAdvance   = Unit{ static_cast<f32>( positions[i].x_advance ) * toUnits },
                .XOffset    = Unit{ static_cast<f32>( positions[i].x_offset ) * toUnits },
                .YOffset    = Unit{ -static_cast<f32>( positions[i].y_offset ) * toUnits }, // HarfBuzz is Y-up.
            } );
        }
    }

    // =========================================================================
    // Rasterization
    // =========================================================================

    bool FontFace::RasterizeGlyph( const GlyphRasterRequest& a_Request, GlyphBitmap& o_Bitmap )
    {
        o_Bitmap = {};
        switch ( a_Request.Mode )
        {
            case EGlyphRenderMode::MTSDF:  return m_MsdfFont ? RasterizeMTSDF( a_Request, o_Bitmap ) : false;
            case EGlyphRenderMode::Raster:
            case EGlyphRenderMode::Pixel:  return RasterizeCoverage( a_Request, o_Bitmap );
            default:                       return false;
        }
    }

    bool FontFace::RasterizeGridSnapped( const GlyphRasterRequest& a_Request, GlyphBitmap& o_Bitmap )
    {
        // Snap every point to the design grid (1 grid step = 1 pixel) so the 1-bit render reproduces the designer's pixels.
        if ( FT_Load_Glyph( m_Face, ToUnderlying( a_Request.Glyph ), FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP ) != 0 )
            return false;

        FT_GlyphSlot slot = m_Face->glyph;
        if ( slot->format != FT_GLYPH_FORMAT_OUTLINE )
            return false;

        FT_Outline& outline = slot->outline;
        if ( outline.n_points == 0 )
            return true; // Blank glyph (e.g. space).

        const f64 rcpGrid = 1.0 / static_cast<f64>( m_GridUnits );
        for ( short i = 0; i < outline.n_points; ++i )
        {
            outline.points[i].x = static_cast<FT_Pos>( std::lround( static_cast<f64>( outline.points[i].x ) * rcpGrid ) ) * 64;
            outline.points[i].y = static_cast<FT_Pos>( std::lround( static_cast<f64>( outline.points[i].y ) * rcpGrid ) ) * 64;
        }

        FT_BBox box;
        FT_Outline_Get_CBox( &outline, &box );
        const FT_Pos xMin = box.xMin & ~63, yMin = box.yMin & ~63;
        const FT_Pos xMax = ( box.xMax + 63 ) & ~63, yMax = ( box.yMax + 63 ) & ~63;

        u32 width  = static_cast<u32>( ( xMax - xMin ) >> 6 );
        u32 height = static_cast<u32>( ( yMax - yMin ) >> 6 );
        if ( width == 0 || height == 0 )
            return true;

        FT_Outline_Translate( &outline, -xMin, -yMin );

        const i32 pitch = static_cast<i32>( ( width + 7 ) / 8 );
        Array<u8> mono( static_cast<size>( pitch ) * height, 0 );

        FT_Bitmap target{};
        target.rows       = height;
        target.width      = width;
        target.pitch      = pitch;
        target.buffer     = Data( mono );
        target.num_grays  = 2;
        target.pixel_mode = FT_PIXEL_MODE_MONO;
        if ( FT_Outline_Get_Bitmap( m_Library->Library, &outline, &target ) != 0 )
            return false;

        Resize( m_Scratch, static_cast<size>( width ) * height );
        for ( u32 y = 0; y < height; ++y )
        {
            const u8* row = Data( mono ) + static_cast<size>( y ) * pitch;
            for ( u32 x = 0; x < width; ++x )
                m_Scratch[static_cast<size>( y ) * width + x] = Color{ 255, 255, 255, ( row[x >> 3] & ( 0x80 >> ( x & 7 ) ) ) ? u8{ 255 } : u8{ 0 } };
        }

        const i32 top = static_cast<i32>( yMax >> 6 );
        if ( a_Request.SynthItalic )
            PixelGlyphOps::Italicize( m_Scratch, width, height, top );
        if ( a_Request.SynthBold )
            PixelGlyphOps::Embolden( m_Scratch, width, height );

        o_Bitmap.Pixels  = Data( m_Scratch );
        o_Bitmap.Width   = width;
        o_Bitmap.Height  = height;
        o_Bitmap.Bearing = Vec2f{ static_cast<f32>( xMin >> 6 ), static_cast<f32>( top ) };
        return true;
    }

    bool FontFace::RasterizeMTSDF( const GlyphRasterRequest& a_Request, GlyphBitmap& o_Bitmap )
    {
        msdfgen::Shape shape;
        if ( !msdfgen::loadGlyph( shape, m_MsdfFont, msdfgen::GlyphIndex{ ToUnderlying( a_Request.Glyph ) }, msdfgen::FONT_SCALING_EM_NORMALIZED ) )
            return false;

        if ( shape.contours.empty() )
            return true; // Blank glyph.

        shape.normalize();
        msdfgen::edgeColoringSimple( shape, 3.0 );

        f64 l = 0.0, b = 0.0, r = 0.0, t = 0.0;
        shape.bound( l, b, r, t );

        const f64 scale   = static_cast<f64>( std::max<u16>( a_Request.PixelSize, 1 ) );
        const f64 range   = static_cast<f64>( a_Request.SDFPixelRange );
        const i32 padding = static_cast<i32>( std::ceil( range ) );

        const i32 w = static_cast<i32>( std::ceil( ( r - l ) * scale ) ) + 2 * padding;
        const i32 h = static_cast<i32>( std::ceil( ( t - b ) * scale ) ) + 2 * padding;
        if ( w <= 0 || h <= 0 )
            return true;

        const f64 padEm = static_cast<f64>( padding ) / scale;
        const msdfgen::Projection projection( msdfgen::Vector2( scale, scale ), msdfgen::Vector2( padEm - l, padEm - b ) );

        msdfgen::Bitmap<f32, 4> mtsdf( w, h );
        msdfgen::generateMTSDF( mtsdf, shape, projection, range / scale );

        Resize( m_Scratch, static_cast<size>( w ) * h );
        const auto toU8 = []( f32 a_Value ) -> u8 { return static_cast<u8>( std::clamp( static_cast<i32>( a_Value * 255.f + 0.5f ), 0, 255 ) ); };

        for ( i32 y = 0; y < h; ++y )
        {
            for ( i32 x = 0; x < w; ++x )
            {
                const f32* px = mtsdf( x, h - 1 - y ); // msdfgen is Y-up.
                m_Scratch[static_cast<size>( y ) * w + x] = Color{ toU8( px[0] ), toU8( px[1] ), toU8( px[2] ), toU8( px[3] ) };
            }
        }

        o_Bitmap.Pixels  = Data( m_Scratch );
        o_Bitmap.Width   = static_cast<u32>( w );
        o_Bitmap.Height  = static_cast<u32>( h );
        o_Bitmap.Bearing = Vec2f{ static_cast<f32>( l - padEm ), static_cast<f32>( t + padEm ) }; // em, includes padding.
        return true;
    }

    bool FontFace::RasterizeCoverage( const GlyphRasterRequest& a_Request, GlyphBitmap& o_Bitmap )
    {
        const bool pixel = a_Request.Mode == EGlyphRenderMode::Pixel;
        if ( pixel && m_PixelGridSource != EPixelGridSource::EmbeddedBitmap && m_GridUnits > 0.f )
            return RasterizeGridSnapped( a_Request, o_Bitmap );

        const u16 ppem = pixel ? static_cast<u16>( std::lround( m_Metrics.NativePixelSize ) ) : a_Request.PixelSize;
        if ( ppem == 0 )
            return false;

        if ( FT_Set_Pixel_Sizes( m_Face, 0, ppem ) != 0 )
            return false;

        // Pixel: embedded strike, 1-bit. Raster: light (vertical-only) hinting, so advances aren't distorted.
        const FT_Int32 loadFlags = pixel ? ( FT_LOAD_NO_HINTING | FT_LOAD_TARGET_MONO )
                                         : ( FT_LOAD_TARGET_LIGHT | FT_LOAD_NO_BITMAP );
        if ( FT_Load_Glyph( m_Face, ToUnderlying( a_Request.Glyph ), loadFlags ) != 0 )
            return false;

        FT_GlyphSlot slot = m_Face->glyph;

        if ( slot->format == FT_GLYPH_FORMAT_OUTLINE && !pixel )
        {
            // Synthesize on the outline so the result is anti-aliased.
            if ( a_Request.SynthItalic )
            {
                FT_Matrix shear{ 0x10000, static_cast<FT_Fixed>( TextShaping::c_SynthItalicSlant * 0x10000 ), 0, 0x10000 };
                FT_Outline_Transform( &slot->outline, &shear );
            }
            if ( a_Request.SynthBold )
            {
                const FT_Pos strength = static_cast<FT_Pos>( TextShaping::SynthBoldEm( Unit{ static_cast<f32>( ppem ) } ) * ppem * 64.f );
                FT_Outline_EmboldenXY( &slot->outline, strength, 0 );
            }
        }

        if ( slot->format != FT_GLYPH_FORMAT_BITMAP )
        {
            if ( FT_Render_Glyph( slot, pixel ? FT_RENDER_MODE_MONO : FT_RENDER_MODE_NORMAL ) != 0 )
                return false;
        }

        const FT_Bitmap& bitmap = slot->bitmap;
        u32 width  = bitmap.width;
        u32 height = bitmap.rows;
        if ( width == 0 || height == 0 )
            return true;

        Resize( m_Scratch, static_cast<size>( width ) * height );
        for ( u32 y = 0; y < height; ++y )
        {
            const u8* row = bitmap.buffer + static_cast<std::ptrdiff_t>( y ) * bitmap.pitch;
            for ( u32 x = 0; x < width; ++x )
            {
                u8 coverage = 0;
                switch ( bitmap.pixel_mode )
                {
                    case FT_PIXEL_MODE_MONO: coverage = ( row[x >> 3] & ( 0x80 >> ( x & 7 ) ) ) ? 255 : 0; break;
                    case FT_PIXEL_MODE_GRAY: coverage = row[x]; break;
                    case FT_PIXEL_MODE_BGRA: coverage = row[x * 4 + 3]; break;
                    default: break;
                }

                // Pixel fonts are 1-bit.
                if ( pixel )
                    coverage = coverage >= 128 ? 255 : 0;

                m_Scratch[static_cast<size>( y ) * width + x] = Color{ 255, 255, 255, coverage };
            }
        }

        if ( pixel )
        {
            if ( a_Request.SynthItalic )
                PixelGlyphOps::Italicize( m_Scratch, width, height, slot->bitmap_top );
            if ( a_Request.SynthBold )
                PixelGlyphOps::Embolden( m_Scratch, width, height );
        }

        o_Bitmap.Pixels  = Data( m_Scratch );
        o_Bitmap.Width   = width;
        o_Bitmap.Height  = height;
        o_Bitmap.Bearing = Vec2f{ static_cast<f32>( slot->bitmap_left ), static_cast<f32>( slot->bitmap_top ) };
        return true;
    }

} // namespace RatUI::FreeType
