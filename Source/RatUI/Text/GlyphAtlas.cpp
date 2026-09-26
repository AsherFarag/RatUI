#include <RatUI/Text/GlyphAtlas.h>

#include <algorithm>

namespace RatUI
{
    // =========================================================================
    // SkylinePacker
    // =========================================================================

    void SkylinePacker::Reset( u16 a_Width, u16 a_Height )
    {
        m_Width  = a_Width;
        m_Height = a_Height;
        Clear( m_Nodes );
        PushBack( m_Nodes, Node{ 0, 0, a_Width } );
    }

    bool SkylinePacker::Fits( size a_Index, i32 a_Width, i32 a_Height, i32& o_Y ) const
    {
        const i32 x = m_Nodes[a_Index].X;
        if ( x + a_Width > m_Width )
            return false;

        i32 widthLeft = a_Width;
        i32 y = m_Nodes[a_Index].Y;
        for ( size i = a_Index; widthLeft > 0; ++i )
        {
            if ( i >= Size( m_Nodes ) )
                return false;

            y = std::max( y, m_Nodes[i].Y );
            if ( y + a_Height > m_Height )
                return false;

            widthLeft -= m_Nodes[i].Width;
        }

        o_Y = y;
        return true;
    }

    Optional<Vec2<u16>> SkylinePacker::Allocate( u16 a_Width, u16 a_Height )
    {
        const i32 w = a_Width, h = a_Height;
        if ( w <= 0 || h <= 0 || w > m_Width || h > m_Height )
            return NullOpt;

        // Bottom-left heuristic: lowest resulting top edge, then leftmost.
        size bestIndex = Limits<size>::max();
        i32  bestY = Limits<i32>::max(), bestX = Limits<i32>::max();

        for ( size i = 0; i < Size( m_Nodes ); ++i )
        {
            i32 y = 0;
            if ( !Fits( i, w, h, y ) )
                continue;

            if ( y + h < bestY || ( y + h == bestY && m_Nodes[i].X < bestX ) )
            {
                bestIndex = i;
                bestY = y + h;
                bestX = m_Nodes[i].X;
            }
        }

        if ( bestIndex == Limits<size>::max() )
            return NullOpt;

        const Node newNode{ m_Nodes[bestIndex].X, bestY, w };
        m_Nodes.insert( Begin( m_Nodes ) + static_cast<std::ptrdiff_t>( bestIndex ), newNode );

        // Shrink / remove the nodes now covered by the new one.
        for ( size i = bestIndex + 1; i < Size( m_Nodes ); )
        {
            const Node& prev = m_Nodes[i - 1];
            Node& node = m_Nodes[i];
            const i32 prevRight = prev.X + prev.Width;
            if ( node.X >= prevRight )
                break;

            const i32 shrink = prevRight - node.X;
            node.X     += shrink;
            node.Width -= shrink;
            if ( node.Width > 0 )
                break;

            m_Nodes.erase( Begin( m_Nodes ) + static_cast<std::ptrdiff_t>( i ) );
        }

        // Merge neighbours at the same height.
        for ( size i = 0; i + 1 < Size( m_Nodes ); )
        {
            if ( m_Nodes[i].Y == m_Nodes[i + 1].Y )
            {
                m_Nodes[i].Width += m_Nodes[i + 1].Width;
                m_Nodes.erase( Begin( m_Nodes ) + static_cast<std::ptrdiff_t>( i + 1 ) );
            }
            else
            {
                ++i;
            }
        }

        return Vec2<u16>{ static_cast<u16>( newNode.X ), static_cast<u16>( bestY - h ) };
    }

    // =========================================================================
    // GlyphAtlas
    // =========================================================================

    GlyphAtlas::GlyphAtlas( IRenderer& a_Renderer, FontLibrary& a_Fonts, const GlyphAtlasConfig& a_Config )
        : m_Renderer( a_Renderer )
        , m_Fonts   ( a_Fonts )
        , m_Config  ( a_Config )
    {
        m_Config.PageSize        = std::max<u16>( m_Config.PageSize, 64 );
        m_Config.MaxPagesPerMode = std::max<u8>( m_Config.MaxPagesPerMode, 1 );
    }

    GlyphAtlas::Page* GlyphAtlas::CreatePage( EGlyphRenderMode a_Mode )
    {
        PagePool& pool = m_Pools[ToUnderlying( a_Mode )];
        if ( Size( pool ) >= m_Config.MaxPagesPerMode )
            return nullptr;

        const u16 pageSize = m_Config.PageSize;

        // Start transparent (so gutters are clean) with the white block in the corner.
        Array<Color> initial( static_cast<size_t>( pageSize ) * pageSize, Colors::Transparent );
        for ( u16 y = 0; y < c_WhiteBlockSize; ++y )
            for ( u16 x = 0; x < c_WhiteBlockSize; ++x )
                initial[static_cast<size_t>( y ) * pageSize + x] = Colors::White;

        auto page = MakeUnique<Page>();
        page->Texture = m_Renderer.CreateTexture(
            TextureInfo{
                .Size    = { pageSize, pageSize },
                .Format  = ETextureFormat::RGBA8,
                .Sampler = { .Filter = a_Mode == EGlyphRenderMode::Pixel ? ETextureFilter::Nearest : ETextureFilter::Linear },
            },
            Data( initial ) );

        page->Packer.Reset( pageSize, pageSize );
        page->Packer.Allocate( c_WhiteBlockSize + 1, c_WhiteBlockSize + 1 ); // Reserve the white block (+ gutter).
        page->LastUsedFrame = m_Frame;

        PushBack( pool, std::move( page ) );
        return Back( pool ).get();
    }

    void GlyphAtlas::ResetPage( EGlyphRenderMode a_Mode, u8 a_PageIndex )
    {
        Page& page = *m_Pools[ToUnderlying( a_Mode )][a_PageIndex];
        page.Packer.Reset( m_Config.PageSize, m_Config.PageSize );
        page.Packer.Allocate( c_WhiteBlockSize + 1, c_WhiteBlockSize + 1 );

        for ( auto it = Begin( m_Glyphs ); it != End( m_Glyphs ); )
        {
            if ( it->first.Mode == a_Mode && it->second.PageIndex == a_PageIndex )
                it = m_Glyphs.erase( it );
            else
                ++it;
        }

        ++m_EvictionCount;
    }

    Optional<Vec2<u16>> GlyphAtlas::AllocateRegion( EGlyphRenderMode a_Mode, u16 a_Width, u16 a_Height, u8& o_PageIndex )
    {
        PagePool& pool = m_Pools[ToUnderlying( a_Mode )];

        for ( size i = Size( pool ); i-- > 0; )
        {
            if ( auto position = pool[i]->Packer.Allocate( a_Width, a_Height ) )
            {
                o_PageIndex = static_cast<u8>( i );
                return position;
            }
        }

        if ( CreatePage( a_Mode ) )
        {
            o_PageIndex = static_cast<u8>( Size( pool ) - 1 );
            return Back( pool )->Packer.Allocate( a_Width, a_Height );
        }

        // Then recycle the least recently used page not drawn from this frame.
        // TODO: This throws away every glyph on the page, even ones that are still hot. Fine for now,
        //       but per glyph eviction might be worth it if pages end up thrashing.
        size lru = Limits<size>::max();
        for ( size i = 0; i < Size( pool ); ++i )
        {
            if ( pool[i]->LastUsedFrame < m_Frame && ( lru == Limits<size>::max() || pool[i]->LastUsedFrame < pool[lru]->LastUsedFrame ) )
                lru = i;
        }

        if ( lru == Limits<size>::max() )
            return NullOpt; // Every page is in use this frame: the budget is too small for what's on screen.

        ResetPage( a_Mode, static_cast<u8>( lru ) );
        o_PageIndex = static_cast<u8>( lru );
        return pool[lru]->Packer.Allocate( a_Width, a_Height );
    }

    Optional<GlyphAtlas::CachedGlyph> GlyphAtlas::Rasterize( const GlyphCacheKey& a_Key )
    {
        IFontFace* face = m_Fonts.GetFace( a_Key.Face );
        if ( !face )
            return NullOpt;

        const GlyphRasterRequest request{
            .Glyph         = a_Key.Glyph,
            .Mode          = a_Key.Mode,
            .PixelSize     = a_Key.Mode == EGlyphRenderMode::MTSDF ? m_Config.SDFBaseSize : a_Key.PixelSize,
            .SDFPixelRange = m_Config.SDFPixelRange,
            .SynthBold     = a_Key.SynthBold,
            .SynthItalic   = a_Key.SynthItalic,
        };

        GlyphBitmap bitmap;
        if ( !face->RasterizeGlyph( request, bitmap ) )
            return NullOpt;

        CachedGlyph cached{ .Bearing = bitmap.Bearing };
        if ( bitmap.Width == 0 || bitmap.Height == 0 || !bitmap.Pixels )
            return cached; // Blank glyph (e.g. space): cached so it isn't re-rasterized every frame.

        if ( bitmap.Width + 1 > m_Config.PageSize || bitmap.Height + 1 > m_Config.PageSize )
            return NullOpt;

        // 1 texel gutter on the right / bottom so neighbours never bleed.
        const u16 paddedW = static_cast<u16>( bitmap.Width + 1 );
        const u16 paddedH = static_cast<u16>( bitmap.Height + 1 );

        u8 pageIndex = 0;
        const Optional<Vec2<u16>> position = AllocateRegion( a_Key.Mode, paddedW, paddedH, pageIndex );
        if ( !position )
            return NullOpt;

        Resize( m_UploadBuffer, static_cast<size>( paddedW ) * paddedH );
        std::fill( Begin( m_UploadBuffer ), End( m_UploadBuffer ), Colors::Transparent );
        for ( u32 y = 0; y < bitmap.Height; ++y )
            std::copy_n( bitmap.Pixels + static_cast<size>( y ) * bitmap.Width, bitmap.Width, Data( m_UploadBuffer ) + static_cast<size>( y ) * paddedW );

        Page& page = *m_Pools[ToUnderlying( a_Key.Mode )][pageIndex];
        m_Renderer.UpdateTexture( page.Texture, 0, Rectu{ { ( *position )[0], ( *position )[1] }, { paddedW, paddedH } },
                                  Data( m_UploadBuffer ), Size( m_UploadBuffer ) * sizeof( Color ) );

        cached.PageIndex = pageIndex;
        cached.Rect      = Rectu16{ *position, { static_cast<u16>( bitmap.Width ), static_cast<u16>( bitmap.Height ) } };
        return cached;
    }

    AtlasGlyph GlyphAtlas::ToAtlasGlyph( EGlyphRenderMode a_Mode, const CachedGlyph& a_Glyph )
    {
        const PagePool& pool = m_Pools[ToUnderlying( a_Mode )];
        AtlasGlyph result{ .Rect = a_Glyph.Rect, .Bearing = a_Glyph.Bearing };

        if ( !result.IsBlank() && a_Glyph.PageIndex < Size( pool ) )
        {
            Page& page = *pool[a_Glyph.PageIndex];
            page.LastUsedFrame = m_Frame;
            result.Page = &page.Texture;
        }

        return result;
    }

    Optional<AtlasGlyph> GlyphAtlas::GetOrRasterizeGlyph( const GlyphCacheKey& a_Key )
    {
        if ( const auto it = Find( m_Glyphs, a_Key ); it != End( m_Glyphs ) )
            return ToAtlasGlyph( a_Key.Mode, it->second );

        const Optional<CachedGlyph> cached = Rasterize( a_Key );
        if ( !cached )
            return NullOpt;

        m_Glyphs[a_Key] = *cached;
        return ToAtlasGlyph( a_Key.Mode, *cached );
    }

    Optional<AtlasGlyph> GlyphAtlas::GetWhiteBlock( EGlyphRenderMode a_Mode )
    {
        PagePool& pool = m_Pools[ToUnderlying( a_Mode )];
        if ( Empty( pool ) && !CreatePage( a_Mode ) )
            return NullOpt;

        Page& page = *Back( pool );
        page.LastUsedFrame = m_Frame;

        // 2x2 texels in the middle of the block, so linear sampling stays white.
        constexpr u16 c_Inset = c_WhiteBlockSize / 2 - 1;
        return AtlasGlyph{ .Page = &page.Texture, .Rect = Rectu16{ { c_Inset, c_Inset }, { 2, 2 } } };
    }

    void GlyphAtlas::LoadGlyphs( const ResolvedFace& a_Face, u16 a_PixelSize, StringView a_Text )
    {
        const IFontFace* face = m_Fonts.GetFace( a_Face.Face );
        if ( !face )
            return;

        const EGlyphRenderMode mode = face->GetMode();
        const u16 pixelSize = mode == EGlyphRenderMode::Pixel ? u16{ 0 } : a_PixelSize; // Pixel glyphs are size independent.

        for ( Unicode::UTF8Iterator it( a_Text ); it; ++it )
            GetOrRasterizeGlyph( GlyphCacheKey::For( a_Face, face->GetGlyphIndex( *it ), mode, pixelSize ) );
    }

    void GlyphAtlas::Clear()
    {
        ::RatUI::Clear( m_Glyphs );
        for ( PagePool& pool : m_Pools )
        {
            for ( Unique<Page>& page : pool )
            {
                page->Packer.Reset( m_Config.PageSize, m_Config.PageSize );
                page->Packer.Allocate( c_WhiteBlockSize + 1, c_WhiteBlockSize + 1 );
            }
        }
    }

} // namespace RatUI
