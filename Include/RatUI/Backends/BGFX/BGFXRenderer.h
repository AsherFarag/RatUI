#pragma once
#include "../../RatUI.h"

#ifdef RATUI_BGFX_INCLUDE
#   include RATUI_BGFX_INCLUDE
#else
#   include <bgfx/bgfx.h>
#endif

namespace RatUI::BGFX
{
    struct Config
    {
        bgfx::ProgramHandle SDFProgram{ BGFX_INVALID_HANDLE };
        bgfx::ProgramHandle TextProgram{ BGFX_INVALID_HANDLE };
        bool OwnPrograms{ false };

        bgfx::ViewId ViewID{ 0 };
        u16 ViewportWidth{ 0 };
        u16 ViewportHeight{ 0 };
    };

    class BGFXRenderer : public IRenderer
    {
    public:
        explicit BGFXRenderer( Config a_Config )
            : m_Config( std::move( a_Config ) )
        {
            RATUI_ASSERT( bgfx::isValid( m_Config.SDFProgram ), "BGFXRenderer: invalid SDF program handle" );
            RATUI_ASSERT( bgfx::isValid( m_Config.TextProgram ), "BGFXRenderer: invalid text program handle" );

            m_UniformPVM            = bgfx::createUniform( "u_PVM", bgfx::UniformType::Mat4 );
            m_UniformSDFTexture     = bgfx::createUniform( "u_Texture", bgfx::UniformType::Sampler );
            m_UniformTextAtlas      = bgfx::createUniform( "u_Atlas", bgfx::UniformType::Sampler );
            m_UniformTextPxRange    = bgfx::createUniform( "u_PxRange", bgfx::UniformType::Vec4 );
            m_UniformTextScale      = bgfx::createUniform( "u_Scale", bgfx::UniformType::Vec4 );
            m_UniformTextFillColor  = bgfx::createUniform( "u_FillColor", bgfx::UniformType::Vec4 );
            m_UniformTextFillSoft   = bgfx::createUniform( "u_FillSoftness", bgfx::UniformType::Vec4 );
            m_UniformTextFillThresh = bgfx::createUniform( "u_FillThreshold", bgfx::UniformType::Vec4 );

            m_UniformTextOutlineColor = bgfx::createUniform( "u_OutlineColor", bgfx::UniformType::Vec4 );
            m_UniformTextOutlineWidth = bgfx::createUniform( "u_OutlineWidth", bgfx::UniformType::Vec4 );
            m_UniformTextOutlineSoft  = bgfx::createUniform( "u_OutlineSoftness", bgfx::UniformType::Vec4 );

            m_UniformTextShadowColor  = bgfx::createUniform( "u_ShadowColor", bgfx::UniformType::Vec4 );
            m_UniformTextShadowOffset = bgfx::createUniform( "u_ShadowOffset", bgfx::UniformType::Vec4 );
            m_UniformTextShadowSoft   = bgfx::createUniform( "u_ShadowSoftness", bgfx::UniformType::Vec4 );
            m_UniformTextShadowSpread = bgfx::createUniform( "u_ShadowSpread", bgfx::UniformType::Vec4 );

            m_UniformTextGlowColor  = bgfx::createUniform( "u_GlowColor", bgfx::UniformType::Vec4 );
            m_UniformTextGlowSpread = bgfx::createUniform( "u_GlowSpread", bgfx::UniformType::Vec4 );
            m_UniformTextGlowPower  = bgfx::createUniform( "u_GlowPower", bgfx::UniformType::Vec4 );

            m_UniformTextInnerGlowColor = bgfx::createUniform( "u_InnerGlowColor", bgfx::UniformType::Vec4 );
            m_UniformTextInnerGlowRange = bgfx::createUniform( "u_InnerGlowRange", bgfx::UniformType::Vec4 );
            m_UniformTextInnerGlowSoft  = bgfx::createUniform( "u_InnerGlowSoftness", bgfx::UniformType::Vec4 );

            m_SDFLayout
                .begin()
                .add( bgfx::Attrib::Position, 2, bgfx::AttribType::Float )
                .add( bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float )
                .add( bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float )
                .add( bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true )
                .add( bgfx::Attrib::Color1, 4, bgfx::AttribType::Uint8, true )
                .add( bgfx::Attrib::TexCoord2, 1, bgfx::AttribType::Float )
                .add( bgfx::Attrib::TexCoord3, 2, bgfx::AttribType::Float )
                .add( bgfx::Attrib::TexCoord4, 1, bgfx::AttribType::Float )
                .add( bgfx::Attrib::TexCoord5, 1, bgfx::AttribType::Float )
                .end();

            m_TextLayout
                .begin()
                .add( bgfx::Attrib::Position, 2, bgfx::AttribType::Float )
                .add( bgfx::Attrib::TexCoord0, 1, bgfx::AttribType::Float )
                .add( bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float )
                .end();

            SetViewport( m_Config.ViewportWidth, m_Config.ViewportHeight );
        }

        ~BGFXRenderer() override
        {
            if ( m_WhitePixelTexture != TextureID::Null() )
                DestroyTexture( m_WhitePixelTexture );

            for ( auto& [_, tex] : m_Textures )
            {
                if ( bgfx::isValid( tex.Handle ) )
                    bgfx::destroy( tex.Handle );
            }

            if ( bgfx::isValid( m_UniformPVM ) )            bgfx::destroy( m_UniformPVM );
            if ( bgfx::isValid( m_UniformSDFTexture ) )     bgfx::destroy( m_UniformSDFTexture );
            if ( bgfx::isValid( m_UniformTextAtlas ) )      bgfx::destroy( m_UniformTextAtlas );
            if ( bgfx::isValid( m_UniformTextPxRange ) )    bgfx::destroy( m_UniformTextPxRange );
            if ( bgfx::isValid( m_UniformTextScale ) )      bgfx::destroy( m_UniformTextScale );
            if ( bgfx::isValid( m_UniformTextFillColor ) )  bgfx::destroy( m_UniformTextFillColor );
            if ( bgfx::isValid( m_UniformTextFillSoft ) )   bgfx::destroy( m_UniformTextFillSoft );
            if ( bgfx::isValid( m_UniformTextFillThresh ) ) bgfx::destroy( m_UniformTextFillThresh );

            if ( bgfx::isValid( m_UniformTextOutlineColor ) ) bgfx::destroy( m_UniformTextOutlineColor );
            if ( bgfx::isValid( m_UniformTextOutlineWidth ) ) bgfx::destroy( m_UniformTextOutlineWidth );
            if ( bgfx::isValid( m_UniformTextOutlineSoft ) )  bgfx::destroy( m_UniformTextOutlineSoft );

            if ( bgfx::isValid( m_UniformTextShadowColor ) )  bgfx::destroy( m_UniformTextShadowColor );
            if ( bgfx::isValid( m_UniformTextShadowOffset ) ) bgfx::destroy( m_UniformTextShadowOffset );
            if ( bgfx::isValid( m_UniformTextShadowSoft ) )   bgfx::destroy( m_UniformTextShadowSoft );
            if ( bgfx::isValid( m_UniformTextShadowSpread ) ) bgfx::destroy( m_UniformTextShadowSpread );

            if ( bgfx::isValid( m_UniformTextGlowColor ) )  bgfx::destroy( m_UniformTextGlowColor );
            if ( bgfx::isValid( m_UniformTextGlowSpread ) ) bgfx::destroy( m_UniformTextGlowSpread );
            if ( bgfx::isValid( m_UniformTextGlowPower ) )  bgfx::destroy( m_UniformTextGlowPower );

            if ( bgfx::isValid( m_UniformTextInnerGlowColor ) ) bgfx::destroy( m_UniformTextInnerGlowColor );
            if ( bgfx::isValid( m_UniformTextInnerGlowRange ) ) bgfx::destroy( m_UniformTextInnerGlowRange );
            if ( bgfx::isValid( m_UniformTextInnerGlowSoft ) )  bgfx::destroy( m_UniformTextInnerGlowSoft );

            if ( m_Config.OwnPrograms )
            {
                if ( bgfx::isValid( m_Config.SDFProgram ) ) bgfx::destroy( m_Config.SDFProgram );
                if ( bgfx::isValid( m_Config.TextProgram ) ) bgfx::destroy( m_Config.TextProgram );
            }
        }

        BGFXRenderer( const BGFXRenderer& )            = delete;
        BGFXRenderer& operator=( const BGFXRenderer& ) = delete;

        void SetViewport( u16 a_Width, u16 a_Height )
        {
            BuildOrthoProjection( a_Width, a_Height );
            bgfx::setViewRect( m_ViewID, 0, 0, a_Width, a_Height );
        }

        void Execute( const DrawBatcher& a_Batcher ) override
        {
            if ( Empty( a_Batcher.GetVertices() ) || Empty( a_Batcher.GetIndices() ) )
                return;

            const Span<const byte> allVertices = a_Batcher.GetVertices();
            const Span<const u16> allIndices = a_Batcher.GetIndices();
            const Span<const DrawBatch> batches = a_Batcher.GetBatches();

            for ( size i = 0; i < Size( batches ); ++i )
            {
                const DrawBatch& batch = batches[i];
                if ( batch.IndexCount == 0 )
                    continue;

                const u32 vertexByteBegin = batch.VertexByteOffset;
                const u32 vertexByteEnd = ( i + 1 < Size( batches ) )
                    ? batches[i + 1].VertexByteOffset
                    : static_cast<u32>( Size( allVertices ) );

                if ( vertexByteEnd <= vertexByteBegin )
                    continue;

                const u32 vertexBytes = vertexByteEnd - vertexByteBegin;

                bgfx::TransientVertexBuffer tvb{};
                bgfx::TransientIndexBuffer tib{};

                const bool isSDF = std::holds_alternative<SDFDrawData>( batch.Data );
                const bgfx::VertexLayout& layout = isSDF ? m_SDFLayout : m_TextLayout;

                const u32 vertexStride = isSDF ? static_cast<u32>( sizeof( SDFVertex ) ) : static_cast<u32>( sizeof( TextVertex ) );
                if ( vertexBytes % vertexStride != 0 )
                    continue;

                const uint32_t vertexCount = vertexBytes / vertexStride;
                if ( vertexCount == 0 )
                    continue;

                if ( bgfx::getAvailTransientVertexBuffer( vertexCount, layout ) < vertexCount )
                    continue;

                if ( bgfx::getAvailTransientIndexBuffer( batch.IndexCount ) < batch.IndexCount )
                    continue;

                bgfx::allocTransientVertexBuffer( &tvb, vertexCount, layout );
                bgfx::allocTransientIndexBuffer( &tib, batch.IndexCount );

                std::memcpy( tvb.data, Data( allVertices ) + vertexByteBegin, vertexBytes );
                std::memcpy( tib.data, Data( allIndices ) + batch.IndexOffset, static_cast<size>( batch.IndexCount ) * sizeof( u16 ) );

                SetClipRect( batch.ClipRect );

                f32 pvm[16];
                ToMat4( m_Projection * batch.Transform, pvm );
                bgfx::setUniform( m_UniformPVM, pvm );

                if ( isSDF )
                {
                    DispatchBatch( std::get<SDFDrawData>( batch.Data ) );
                    bgfx::setVertexBuffer( 0, &tvb );
                    bgfx::setIndexBuffer( &tib );
                    bgfx::submit( m_ViewID, m_Programs.SDFProgram );
                }
                else
                {
                    DispatchBatch( std::get<MSDFTextDrawData>( batch.Data ) );
                    bgfx::setVertexBuffer( 0, &tvb );
                    bgfx::setIndexBuffer( &tib );
                    bgfx::submit( m_ViewID, m_Programs.TextProgram );
                }
            }

            bgfx::setScissor( 0, 0, 0, 0 );
        }

        TextureHandle CreateTexture( TextureInfo a_Info, const void* a_Data ) override
        {
            const bgfx::TextureFormat::Enum format = FormatToBGFX( a_Info.Format );
            const uint64_t flags = SamplerToBGFXFlags( a_Info.Sampler );

            const bgfx::Memory* mem = nullptr;
            const uint32_t bytesPerPixel = BytesPerPixel( a_Info.Format );
            const uint32_t dataSize = static_cast<uint32_t>( a_Info.Size[0] * a_Info.Size[1] * bytesPerPixel );
            if ( a_Data && dataSize > 0 )
                mem = bgfx::copy( a_Data, dataSize );

            bgfx::TextureHandle tex = bgfx::createTexture2D(
                static_cast<uint16_t>( a_Info.Size[0] ),
                static_cast<uint16_t>( a_Info.Size[1] ),
                false,
                1,
                format,
                flags,
                mem );

            if ( !bgfx::isValid( tex ) )
                return TextureHandle::Null();

            const TextureID id = AllocateTextureID();
            m_Textures[id.ID] = TextureRecord{ .Handle = tex, .Info = a_Info };
            return TextureHandle( MakeShared<Texture>( *this, id ) );
        }

        bool UpdateTexture( TextureID a_Texture, u32 a_MipLevel, Rectu a_Region, const void* a_Data, size a_DataSizeBytes ) override
        {
            auto it = m_Textures.find( a_Texture.ID );
            if ( it == m_Textures.end() || !a_Data )
                return false;

            const TextureInfo& info = it->second.Info;

            if ( a_Region.Origin[0] + a_Region.Size[0] > info.Size[0] ||
                 a_Region.Origin[1] + a_Region.Size[1] > info.Size[1] )
                return false;

            const uint32_t bpp = BytesPerPixel( info.Format );
            const uint32_t expectedSize = a_Region.Size[0] * a_Region.Size[1] * bpp;
            if ( expectedSize != static_cast<uint32_t>( a_DataSizeBytes ) )
                return false;

            const bgfx::Memory* mem = bgfx::copy( a_Data, expectedSize );
            bgfx::updateTexture2D(
                it->second.Handle,
                0,
                static_cast<uint8_t>( a_MipLevel ),
                static_cast<uint16_t>( a_Region.Origin[0] ),
                static_cast<uint16_t>( a_Region.Origin[1] ),
                static_cast<uint16_t>( a_Region.Size[0] ),
                static_cast<uint16_t>( a_Region.Size[1] ),
                mem,
                static_cast<uint16_t>( a_Region.Size[0] * bpp ) );
            return true;
        }

        void DestroyTexture( TextureID a_Texture ) override
        {
            auto it = m_Textures.find( a_Texture.ID );
            if ( it == m_Textures.end() )
                return;

            if ( bgfx::isValid( it->second.Handle ) )
                bgfx::destroy( it->second.Handle );

            if ( m_WhitePixelTexture == a_Texture )
                m_WhitePixelTexture = TextureID::Null();

            m_Textures.erase( it );
        }

        bool IsValidTexture( TextureID a_Texture ) const override
        {
            return m_Textures.contains( a_Texture.ID );
        }

        Optional<TextureInfo> QueryTextureInfo( TextureID a_Texture ) const override
        {
            auto it = m_Textures.find( a_Texture.ID );
            if ( it == m_Textures.end() )
                return NullOpt;
            return it->second.Info;
        }

    private:
        struct TextureRecord
        {
            bgfx::TextureHandle Handle{ BGFX_INVALID_HANDLE };
            TextureInfo Info{};
        };

        Config m_Config;

        bgfx::UniformHandle m_UniformPVM{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformSDFTexture{ BGFX_INVALID_HANDLE };

        bgfx::UniformHandle m_UniformTextAtlas{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextPxRange{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextScale{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextFillColor{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextFillSoft{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextFillThresh{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextOutlineColor{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextOutlineWidth{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextOutlineSoft{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextShadowColor{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextShadowOffset{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextShadowSoft{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextShadowSpread{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextGlowColor{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextGlowSpread{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextGlowPower{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextInnerGlowColor{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextInnerGlowRange{ BGFX_INVALID_HANDLE };
        bgfx::UniformHandle m_UniformTextInnerGlowSoft{ BGFX_INVALID_HANDLE };

        bgfx::VertexLayout m_SDFLayout{};
        bgfx::VertexLayout m_TextLayout{};

        Mat3f m_Projection{};

        TextureID m_WhitePixelTexture{ TextureID::Null() };
        uptr m_NextTextureID{ 1 };
        HashMap<uptr, TextureRecord> m_Textures;

        static bgfx::TextureFormat::Enum FormatToBGFX( ETextureFormat a_Format )
        {
            switch ( a_Format )
            {
                case ETextureFormat::R8: return bgfx::TextureFormat::R8;
                case ETextureFormat::RG8: return bgfx::TextureFormat::RG8;
                case ETextureFormat::RGB8: return bgfx::TextureFormat::RGB8;
                case ETextureFormat::RGBA8: return bgfx::TextureFormat::RGBA8;
                default: return bgfx::TextureFormat::RGBA8;
            }
        }

        static uint32_t BytesPerPixel( ETextureFormat a_Format )
        {
            switch ( a_Format )
            {
                case ETextureFormat::R8: return 1;
                case ETextureFormat::RG8: return 2;
                case ETextureFormat::RGB8: return 3;
                case ETextureFormat::RGBA8: return 4;
                default: return 4;
            }
        }

        static uint64_t SamplerToBGFXFlags( const TextureSampler& a_Sampler )
        {
            uint64_t flags = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
            if ( a_Sampler.Filter == ETextureFilter::Nearest )
                flags |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;
            return flags;
        }

        TextureID AllocateTextureID()
        {
            TextureID id{};
            id.ID = m_NextTextureID++;
            return id;
        }

        void DispatchBatch( const SDFDrawData& a_Data )
        {
            const TextureID texID = a_Data.Texture.GetID();
            bgfx::TextureHandle tex = BGFX_INVALID_HANDLE;

            if ( IsValidTexture( texID ) )
            {
                tex = m_Textures.find( texID.ID )->second.Handle;
            }
            else
            {
                if ( m_WhitePixelTexture == TextureID::Null() )
                {
                    const u8 whitePixel[4] = { 255, 255, 255, 255 };
                    TextureHandle created = CreateTexture( { .Size = { 1, 1 }, .Format = ETextureFormat::RGBA8 }, whitePixel );
                    m_WhitePixelTexture = created.GetID();
                }

                if ( IsValidTexture( m_WhitePixelTexture ) )
                    tex = m_Textures.find( m_WhitePixelTexture.ID )->second.Handle;
            }

            if ( bgfx::isValid( tex ) )
                bgfx::setTexture( 0, m_UniformSDFTexture, tex );

            constexpr uint64_t c_State = BGFX_STATE_WRITE_RGB
                                       | BGFX_STATE_WRITE_A
                                       | BGFX_STATE_BLEND_ALPHA
                                       | BGFX_STATE_MSAA;
            bgfx::setState( c_State );
        }

        void DispatchBatch( const MSDFTextDrawData& a_Data )
        {
            if ( IsValidTexture( a_Data.FontAtlas.GetID() ) )
            {
                const bgfx::TextureHandle atlas = m_Textures.find( a_Data.FontAtlas.GetID().ID )->second.Handle;
                bgfx::setTexture( 0, m_UniformTextAtlas, atlas );
            }

            SetUniformFloat( m_UniformTextPxRange, a_Data.PixelRange );
            SetUniformFloat( m_UniformTextScale, a_Data.Scale );

            SetUniformColor( m_UniformTextFillColor, a_Data.FillColor );
            SetUniformFloat( m_UniformTextFillSoft, a_Data.FillSoftness );
            SetUniformFloat( m_UniformTextFillThresh, a_Data.FillThreshold );

            SetUniformColor( m_UniformTextOutlineColor, a_Data.OutlineEnable ? a_Data.OutlineColor : Colors::Transparent );
            SetUniformFloat( m_UniformTextOutlineWidth, a_Data.OutlineEnable ? a_Data.OutlineWidth : 0.f );
            SetUniformFloat( m_UniformTextOutlineSoft, a_Data.OutlineEnable ? a_Data.OutlineSoftness : 0.f );

            SetUniformColor( m_UniformTextShadowColor, a_Data.ShadowEnable ? a_Data.ShadowColor : Colors::Transparent );
            SetUniformVec2( m_UniformTextShadowOffset, a_Data.ShadowEnable ? a_Data.ShadowOffsetUV : Vec2f{ 0.f, 0.f } );
            SetUniformFloat( m_UniformTextShadowSoft, a_Data.ShadowEnable ? a_Data.ShadowSoftness : 0.f );
            SetUniformFloat( m_UniformTextShadowSpread, a_Data.ShadowEnable ? a_Data.ShadowSpread : 0.f );

            SetUniformColor( m_UniformTextGlowColor, a_Data.GlowEnable ? a_Data.GlowColor : Colors::Transparent );
            SetUniformFloat( m_UniformTextGlowSpread, a_Data.GlowEnable ? a_Data.GlowSpread : 0.f );
            SetUniformFloat( m_UniformTextGlowPower, a_Data.GlowEnable ? a_Data.GlowPower : 0.f );

            SetUniformColor( m_UniformTextInnerGlowColor, Colors::Transparent );
            SetUniformFloat( m_UniformTextInnerGlowRange, 0.f );
            SetUniformFloat( m_UniformTextInnerGlowSoft, 0.f );

            constexpr uint64_t c_State = BGFX_STATE_WRITE_RGB
                                       | BGFX_STATE_WRITE_A
                                       | BGFX_STATE_BLEND_ALPHA
                                       | BGFX_STATE_MSAA;
            bgfx::setState( c_State );
        }

        static void SetUniformFloat( bgfx::UniformHandle a_Uniform, f32 a_Value )
        {
            const f32 v[4] = { a_Value, 0.f, 0.f, 0.f };
            bgfx::setUniform( a_Uniform, v );
        }

        static void SetUniformVec2( bgfx::UniformHandle a_Uniform, Vec2f a_Value )
        {
            const f32 v[4] = { a_Value[0], a_Value[1], 0.f, 0.f };
            bgfx::setUniform( a_Uniform, v );
        }

        static void SetUniformColor( bgfx::UniformHandle a_Uniform, Color a_Color )
        {
            const f32 v[4] = {
                a_Color[0] / 255.f,
                a_Color[1] / 255.f,
                a_Color[2] / 255.f,
                a_Color[3] / 255.f
            };
            bgfx::setUniform( a_Uniform, v );
        }

        void SetClipRect( const Optional<Rectu16>& a_ClipRect )
        {
            if ( !HasValue( a_ClipRect ) )
            {
                bgfx::setScissor( 0, 0, 0, 0 );
                return;
            }

            const Rectu16& cr = *a_ClipRect;
            bgfx::setScissor( cr.Origin[0], cr.Origin[1], cr.Size[0], cr.Size[1] );
        }

        static void ToMat4( const Mat3f& a_Mat, f32 o_Result[16] )
        {
            o_Result[0] = a_Mat[0][0];
            o_Result[1] = a_Mat[0][1];
            o_Result[2] = 0.f;
            o_Result[3] = 0.f;

            o_Result[4] = a_Mat[1][0];
            o_Result[5] = a_Mat[1][1];
            o_Result[6] = 0.f;
            o_Result[7] = 0.f;

            o_Result[8] = 0.f;
            o_Result[9] = 0.f;
            o_Result[10] = 1.f;
            o_Result[11] = 0.f;

            o_Result[12] = a_Mat[2][0];
            o_Result[13] = a_Mat[2][1];
            o_Result[14] = 0.f;
            o_Result[15] = 1.f;
        }

        void BuildOrthoProjection( int a_Width, int a_Height )
        {
            const float w = static_cast<float>( a_Width );
            const float h = static_cast<float>( a_Height );

            m_Projection[0u][0] = 2.f / w;
            m_Projection[0u][1] = 0.f;
            m_Projection[0u][2] = 0.f;

            m_Projection[1u][0] = 0.f;
            m_Projection[1u][1] = -2.f / h;
            m_Projection[1u][2] = 0.f;

            m_Projection[2u][0] = -1.f;
            m_Projection[2u][1] = 1.f;
            m_Projection[2u][2] = 1.f;
        }
    };
}
