#pragma once
#include "../RatUI.h"

namespace RatUI::Null
{
    class NullRenderer : public IRenderer
    {
    public:
        void Execute( const DrawBatcher& ) override {}
        TextureHandle CreateTexture( TextureInfo, const void* ) override { return nullptr; }
        bool UpdateTexture( const TextureHandle&, u32, Rectu, const void*, size ) override { return false; }
        void DestroyTexture( const TextureHandle& ) override {}
        bool IsValidTexture( const TextureHandle& ) const override { return false; }
        Optional<TextureInfo> QueryTextureInfo( const TextureHandle& ) const override { return NullOpt; }
    };

    class NullTextMetrics : public ITextMetrics
    {
    public:
        Optional<PreparedText> Prepare( StringView, const TextLayoutStyle& ) override { return NullOpt; }
        Optional<ShapedText> Shape( const PreparedText&, const TextLayoutStyle&, Vec2<Unit> ) override { return NullOpt; }
        bool RasterizeGlyph( FontHandle, GlyphID, u32, const Color*&, u32&, u32&, Vec2<FontUnit>&, FontUnit& ) override { return false; }
    };

} // namespace RatUI::Null