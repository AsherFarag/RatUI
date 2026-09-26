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

} // namespace RatUI::Null