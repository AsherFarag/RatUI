#pragma once
#include "../../RatUI.h"
#include "../../Renderer/Shaders/GLSL.h"
#include "../FreeType/FontCache.h"

/**
 * @file OpenGLRenderer.h
 * @brief OpenGL renderer backend with MSDF text support.
 *
 * **Usage**
 * 1. Create an OpenGLRenderer after an OpenGL context is current.
 * 2. Call Execute( batcher ) each frame.
 *
 * **GL loader**
 * Include your preferred OpenGL loader header *before* this file, e.g.
 *   @code
 *   #include <glad/glad.h>
 *   #include <RatUI/Backends/OpenGL/OpenGLRenderer.h>
 *   @endcode
 * Alternatively define RATUI_OPENGL_INCLUDE to your loader path and the
 * renderer will include it automatically:
 *   @code
 *   #define RATUI_OPENGL_INCLUDE <glad/glad.h>
 *   @endcode
 *
 * **Coordinate system**
 * Clip-space transform is injected via the u_PVM uniform (ortho 2-D,
 * top-left origin, Y-down).  Call SetViewport() when the window is resized.
 */

#ifdef RATUI_OPENGL_INCLUDE
#   include RATUI_OPENGL_INCLUDE
#endif

namespace RatUI::OpenGL
{
    /**
     * @brief OpenGL 3.3 renderer for RatUI.
     *
     * Maintains two VAOs — one for SDF shapes (SDFVertex), one for MSDF text
     * (TextVertex) — both sharing a single VBO + IBO that are re-uploaded each
     * frame via GL_STREAM_DRAW.
     *
     * Because all batches share one contiguous VBO, attrib pointers are
     * re-specified each batch using the batch's VertexByteOffset so the GPU
     * reads from the correct region without any extra copies or base-vertex
     * extensions.
     *
     * Programs are compiled once on construction:
     *   - SDF program   : SDFVertex layout — rounded rects, circles, borders.
     *   - MSDF program  : TextVertex layout — MSDF glyph rendering.
     *
     * The renderer uses a standard 2-D orthographic projection (top-left origin,
     * Y-down).  Call SetViewport() on every framebuffer resize.
     */
    class OpenGLRenderer : public IRenderer
    {
    public:
        /**
         * @param a_ViewportWidth   Initial viewport / framebuffer width in pixels.
         * @param a_ViewportHeight  Initial viewport / framebuffer height in pixels.
         */
        explicit OpenGLRenderer( int a_ViewportWidth = 800, int a_ViewportHeight = 600 );
        ~OpenGLRenderer() override;

        OpenGLRenderer( const OpenGLRenderer& )            = delete;
        OpenGLRenderer& operator=( const OpenGLRenderer& ) = delete;

        /** @brief Updates the orthographic projection to match a new framebuffer size. */
        void SetViewport( int a_Width, int a_Height );
        void Execute( const DrawBatcher& a_Batcher ) override;
        TextureHandle CreateTexture( TextureInfo a_Info, const void* a_Data ) override;
        bool UpdateTexture( TextureID a_Texture, u32 a_MipLevel, Rectu a_Region, const void* a_Data, size a_DataSizeBytes ) override;
        void DestroyTexture( TextureID a_Texture ) override;
        bool IsValidTexture( TextureID a_Texture ) const override;
		Optional<TextureInfo> QueryTextureInfo( TextureID a_Texture ) const override;

    private:

        // =====================================================================
        // Batch dispatch — one overload per draw data type
        // =====================================================================

        void DispatchBatch( const SDFDrawData& a_Data, u32 a_VertexByteOffset, const f32 a_PVM[16] );
        void DispatchBatch( const MSDFTextDrawData& a_Data, u32 a_VertexByteOffset, const f32 a_PVM[16] );

        // =====================================================================
        // Helpers
        // =====================================================================

        /** @brief Enable or disable scissor test and set the clip rect (Y-flipped for GL). */
        void SetClipRect( const Optional<Rectu16>& a_ClipRect );

        static GLenum FormatToGLInternal( ETextureFormat a_Format );

        static GLenum FormatToGLBase( ETextureFormat a_Format );

        /**
         * @brief Build a column-major 3x3 orthographic projection for RatUI's Mat3f.
         *
         * Maps pixel coordinates (origin top-left, Y-down) to NDC [-1, 1]:
         *   x' =  2/w * x - 1
         *   y' = -2/h * y + 1
         */
        void BuildOrthoProjection( int a_Width, int a_Height );

        // =====================================================================
        // Members
        // =====================================================================

        GLuint m_VBO       { 0 };
        GLuint m_IBO       { 0 };
        GLuint m_SDFVAO    { 0 };
        GLuint m_TextVAO   { 0 };
        GLuint m_SDFProgram  { 0 };
        GLuint m_MSDFProgram { 0 };

        GLint m_SDFUniforms [GLSL::ESDFUniform_UniformCount ]{};
        GLint m_TextUniforms[GLSL::ETextUniform_UniformCount]{};

        Mat3f m_Projection    {};
        i32   m_ViewportWidth { 800 };
        i32   m_ViewportHeight{ 600 };

        TextureHandle m_WhitePixelTexture; // TODO: Expose a default white texture for users to avoid creating their own
    };

    // =====================================================================
    // Implementation
    // =====================================================================

    namespace Detail
    {
        /**
         * @brief Compile a GLSL shader of the given type from source, with error checking.
         * @param a_Type  GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
         * @param a_Src   Null-terminated GLSL source string.
         * @return OpenGL shader object ID.
         */
        GLuint CompileShader( GLenum a_Type, const char* a_Src )
        {
            GLuint shader = glCreateShader( a_Type );
            glShaderSource( shader, 1, &a_Src, nullptr );
            glCompileShader( shader );

            GLint ok = 0;
            glGetShaderiv( shader, GL_COMPILE_STATUS, &ok );
            if ( !ok )
            {
                char log[512];
                glGetShaderInfoLog( shader, sizeof( log ), nullptr, log );
                RATUI_ASSERT( false, "OpenGLRenderer: shader compile error" );
            }

            return shader;
        }

        /**
         * @brief Link a GLSL program from vertex and fragment shader sources, with error checking.
         * @param a_VertSrc  Null-terminated vertex shader GLSL source.
         * @param a_FragSrc  Null-terminated fragment shader GLSL source.
         * @return OpenGL program object ID.
         */
        GLuint LinkProgram( const char* a_VertSrc, const char* a_FragSrc )
        {
            GLuint vert = CompileShader( GL_VERTEX_SHADER, a_VertSrc );
            GLuint frag = CompileShader( GL_FRAGMENT_SHADER, a_FragSrc );

            GLuint prog = glCreateProgram();
            glAttachShader( prog, vert );
            glAttachShader( prog, frag );
            glLinkProgram( prog );

            GLint ok = 0;
            glGetProgramiv( prog, GL_LINK_STATUS, &ok );
            if ( !ok )
            {
                char log[512];
                glGetProgramInfoLog( prog, sizeof( log ), nullptr, log );
                RATUI_ASSERT( false, "OpenGLRenderer: program link error" );
            }

            glDeleteShader( vert );
            glDeleteShader( frag );
            return prog;
        }

        /**
         * @brief Expand a 3x3 affine matrix into a column-major 4x4 matrix for OpenGL.
         *
         * The 2-D affine Mat3f is laid out as:
         *   col0 = (sx, shy, 0)   col1 = (shx, sy, 0)   col2 = (tx, ty, 1)
         * Expanded to 4x4 with Z pass-through and W=1:
         *   [ sx   shx  0  tx ]
         *   [ shy  sy   0  ty ]
         *   [ 0    0    1   0 ]
         *   [ 0    0    0   1 ]
         */
        void ToMat4( const Mat3f& a_Mat, f32 o_Result[16] )
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

        /** @brief Upload a Color (4 x u8) as a normalised vec4 uniform. */
        void UniformColor( GLint a_Loc, Color a_Color )
        {
            glUniform4f( a_Loc,
                         a_Color[0] / 255.f,
                         a_Color[1] / 255.f,
                         a_Color[2] / 255.f,
                         a_Color[3] / 255.f );
        }

        /** @brief Upload a zero vec4 (transparent black) to a colour uniform. */
        void UniformColorZero( GLint a_Loc )
        {
            glUniform4f( a_Loc, 0.f, 0.f, 0.f, 0.f );
        }
    } // namespace Detail

    OpenGLRenderer::OpenGLRenderer( int a_ViewportWidth, int a_ViewportHeight )
    {
        static_assert( sizeof( SDFVertex ) == 52, "SDFVertex layout assumption broken" );
        static_assert( sizeof( TextVertex ) == 20, "TextVertex layout assumption broken" );

        m_SDFProgram = Detail::LinkProgram( GLSL::c_SDFVertSrc, GLSL::c_SDFFragSrc );
        m_MSDFProgram = Detail::LinkProgram( GLSL::c_TextVertSrc, GLSL::c_TextFragSrc );

        const auto collectUniforms = []( GLuint a_Program, const char* const* a_Names, GLint* a_OutLocs, i32 a_Count )
        {
            for ( i32 i = 0; i < a_Count; ++i )
                a_OutLocs[i] = glGetUniformLocation( a_Program, a_Names[i] );
        };

        collectUniforms( m_SDFProgram, GLSL::c_SDFUniformNames, m_SDFUniforms, (i32)GLSL::ESDFUniform_UniformCount );
        collectUniforms( m_MSDFProgram, GLSL::c_TextUniformNames, m_TextUniforms, (i32)GLSL::ETextUniform_UniformCount );

        glGenBuffers( 1, &m_VBO );
        glGenBuffers( 1, &m_IBO );

        glGenVertexArrays( 1, &m_SDFVAO );
        glBindVertexArray( m_SDFVAO );
        glBindBuffer( GL_ARRAY_BUFFER, m_VBO );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBO );
        for ( int i = 0; i < 9; ++i )
            glEnableVertexAttribArray( i );
        glBindVertexArray( 0 );

        glGenVertexArrays( 1, &m_TextVAO );
        glBindVertexArray( m_TextVAO );
        glBindBuffer( GL_ARRAY_BUFFER, m_VBO );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBO );
        for ( int i = 0; i < 3; ++i )
            glEnableVertexAttribArray( i );
        glBindVertexArray( 0 );

        SetViewport( a_ViewportWidth, a_ViewportHeight );
    }

    OpenGLRenderer::~OpenGLRenderer()
    {
        glDeleteVertexArrays( 1, &m_SDFVAO );
        glDeleteVertexArrays( 1, &m_TextVAO );
        glDeleteBuffers( 1, &m_VBO );
        glDeleteBuffers( 1, &m_IBO );
        glDeleteProgram( m_SDFProgram );
        glDeleteProgram( m_MSDFProgram );
    }

    void OpenGLRenderer::SetViewport( int a_Width, int a_Height )
    {
        m_ViewportWidth = a_Width;
        m_ViewportHeight = a_Height;
        BuildOrthoProjection( a_Width, a_Height );
    }

    void OpenGLRenderer::Execute( const DrawBatcher& a_Batcher )
    {
        if ( Empty( a_Batcher.GetVertices() ) || Empty( a_Batcher.GetIndices() ) )
            return;

        glBindBuffer( GL_ARRAY_BUFFER, m_VBO );
        glBufferData( GL_ARRAY_BUFFER,
                      static_cast<GLsizeiptr>( SizeBytes( a_Batcher.GetVertices() ) ),
                      Data( a_Batcher.GetVertices() ),
                      GL_STREAM_DRAW );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBO );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                      static_cast<GLsizeiptr>( SizeBytes( a_Batcher.GetIndices() ) ),
                      Data( a_Batcher.GetIndices() ),
                      GL_STREAM_DRAW );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable( GL_DEPTH_TEST );
        glDisable( GL_CULL_FACE );

        for ( const DrawBatch& batch : a_Batcher.GetBatches() )
        {
            if ( batch.IndexCount == 0 )
                continue;

            SetClipRect( batch.ClipRect );

            f32 pvm[16];
            Detail::ToMat4( m_Projection * batch.Transform, pvm );

            const GLvoid* indexByteOffset = reinterpret_cast<const GLvoid*>(
                static_cast<uintptr_t>( batch.IndexOffset ) * sizeof( u16 ) );

            std::visit( [&]( const auto& a_Data )
                        { DispatchBatch( a_Data, batch.VertexByteOffset, pvm ); },
                        batch.Data );

            glDrawElements( GL_TRIANGLES,
                            static_cast<GLsizei>( batch.IndexCount ),
                            GL_UNSIGNED_SHORT,
                            indexByteOffset );
        }

        glDisable( GL_SCISSOR_TEST );
        glBindVertexArray( 0 );
    }

    TextureHandle OpenGLRenderer::CreateTexture( TextureInfo a_Info, const void* a_Data )
    {
        GLuint texID = 0;
        glGenTextures( 1, &texID );
        glBindTexture( GL_TEXTURE_2D, texID );

        // Filtering
        const GLint filter =
            a_Info.Sampler.Filter == ETextureFilter::Nearest
            ? GL_NEAREST
            : GL_LINEAR;

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter );

        // Wrapping
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

        // Upload
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            static_cast<GLint>( FormatToGLInternal( a_Info.Format ) ),
            static_cast<GLsizei>( a_Info.Size[0] ),
            static_cast<GLsizei>( a_Info.Size[1] ),
            0,
            FormatToGLBase( a_Info.Format ),
            GL_UNSIGNED_BYTE,
            a_Data
        );

        glBindTexture( GL_TEXTURE_2D, 0 );

        TextureID id;
        id.ID = static_cast<uptr>( texID );

        return TextureHandle(  MakeShared<Texture>( *this, id ) );
    }

    bool OpenGLRenderer::UpdateTexture( TextureID a_Texture, u32 /*a_MipLevel*/, Rectu a_Region, const void* a_Data, size a_DataSizeBytes )
    {
        if ( a_Texture == TextureID::Null() || !a_Data )
            return false;

        const u32 w = a_Region.Size[0];
        const u32 h = a_Region.Size[1];
        if ( w == 0 || h == 0 )
            return true;

        const size pixelCount = static_cast<size>( w ) * h;
        GLenum fmt = GL_RED;
        if ( a_DataSizeBytes == pixelCount * 3u )
            fmt = GL_RGB;
        else if ( a_DataSizeBytes == pixelCount * 4u )
            fmt = GL_RGBA;

        glBindTexture( GL_TEXTURE_2D, static_cast<GLuint>( a_Texture.ID ) );
        glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
        glTexSubImage2D( GL_TEXTURE_2D, 0,
                         static_cast<GLint>( a_Region.Origin[0] ),
                         static_cast<GLint>( a_Region.Origin[1] ),
                         static_cast<GLsizei>( w ),
                         static_cast<GLsizei>( h ),
                         fmt, GL_UNSIGNED_BYTE, a_Data );
        glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
        glBindTexture( GL_TEXTURE_2D, 0 );
        return true;
    }

    void OpenGLRenderer::DestroyTexture( TextureID a_Texture )
    {
        if ( IsValidTexture( a_Texture ) )
        {
            GLuint texID = static_cast<GLuint>( a_Texture.ID );
            glDeleteTextures( 1, &texID );
        }
    }

    bool OpenGLRenderer::IsValidTexture( TextureID a_Texture ) const
    {
        return a_Texture.ID != 0;
    }

    Optional<TextureInfo> OpenGLRenderer::QueryTextureInfo( TextureID a_Texture ) const
    {
        if ( !IsValidTexture( a_Texture ) )
            return NullOpt;

        glBindTexture( GL_TEXTURE_2D, static_cast<GLuint>( a_Texture.ID ) );

        TextureInfo info{};

        // Size
        GLint width = 0;
        GLint height = 0;

        glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width );
        glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height );

        info.Size = Vec2u(
            static_cast<u32>( width ),
            static_cast<u32>( height )
        );

        // Format
        GLint internalFormat = 0;
        glGetTexLevelParameteriv(
            GL_TEXTURE_2D,
            0,
            GL_TEXTURE_INTERNAL_FORMAT,
            &internalFormat
        );

        switch ( internalFormat )
        {
            case GL_R8:    info.Format = ETextureFormat::R8; break;
            case GL_RG8:   info.Format = ETextureFormat::RG8; break;
            case GL_RGB8:  info.Format = ETextureFormat::RGB8; break;
            case GL_RGBA8: info.Format = ETextureFormat::RGBA8; break;
            default:       info.Format = ETextureFormat::Unknown; break;
        }

        // Filter
        GLint minFilter = 0;
        glGetTexParameteriv(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            &minFilter
        );

        switch ( minFilter )
        {
            case GL_NEAREST:
            case GL_NEAREST_MIPMAP_NEAREST:
            case GL_NEAREST_MIPMAP_LINEAR:
                info.Sampler.Filter = ETextureFilter::Nearest;
                break;

            case GL_LINEAR:
            case GL_LINEAR_MIPMAP_NEAREST:
            case GL_LINEAR_MIPMAP_LINEAR:
            default:
                info.Sampler.Filter = ETextureFilter::Linear;
                break;
        }

        return info;
    }

    void OpenGLRenderer::DispatchBatch( const SDFDrawData& a_Data, u32 a_VertexByteOffset, const f32 a_PVM[16] )
    {
        glActiveTexture( GL_TEXTURE0 );

        if ( TextureID texID = a_Data.Texture.GetID(); IsValidTexture( texID ) )
        {
            glBindTexture( GL_TEXTURE_2D, static_cast<GLuint>( texID.ID ) );
        }
        else
        {
            if ( m_WhitePixelTexture.GetID() == TextureID::Null() )
            {
                const u8 whitePixel[4] = { 255, 255, 255, 255 };
                m_WhitePixelTexture = CreateTexture( { .Size = { 1, 1 }, .Format = ETextureFormat::RGBA8 }, whitePixel );
            }

            glBindTexture( GL_TEXTURE_2D, static_cast<GLuint>( m_WhitePixelTexture.GetID().ID ) );
        }

        glBindVertexArray( m_SDFVAO );
        const auto vo = static_cast<uintptr_t>( a_VertexByteOffset );
        glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 0 ) );
        glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 8 ) );
        glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 16 ) );
        glVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( SDFVertex ), (const void*)( vo + 24 ) );
        glVertexAttribPointer( 4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( SDFVertex ), (const void*)( vo + 28 ) );
        glVertexAttribPointer( 5, 1, GL_FLOAT, GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 32 ) );
        glVertexAttribPointer( 6, 2, GL_FLOAT, GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 36 ) );
        glVertexAttribPointer( 7, 1, GL_FLOAT, GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 44 ) );
		glVertexAttribPointer( 8, 1, GL_FLOAT, GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 48 ) );

        glUseProgram( m_SDFProgram );
        glUniformMatrix4fv( m_SDFUniforms[GLSL::ESDFUniform_PVM], 1, GL_FALSE, a_PVM );
    }

    void OpenGLRenderer::DispatchBatch( const MSDFTextDrawData& a_Data, u32 a_VertexByteOffset, const f32 a_PVM[16] )
    {
        if ( TextureID texID = a_Data.FontAtlas.GetID(); IsValidTexture( texID ) )
        {
            glActiveTexture( GL_TEXTURE0 );
            glBindTexture( GL_TEXTURE_2D, static_cast<GLuint>( texID.ID ) );
        }

        glBindVertexArray( m_TextVAO );
        const auto vo = static_cast<uintptr_t>( a_VertexByteOffset );
        glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( TextVertex ), (const void*)( vo + 0 ) );
        glVertexAttribPointer( 1, 1, GL_FLOAT, GL_FALSE, sizeof( TextVertex ), (const void*)( vo + 8 ) );
        glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( TextVertex ), (const void*)( vo + 12 ) );

        glUseProgram( m_MSDFProgram );
        glUniformMatrix4fv( m_TextUniforms[GLSL::ETextUniform_PVM], 1, GL_FALSE, a_PVM );
        glUniform1i( m_TextUniforms[GLSL::ETextUniform_Atlas], 0 );
        glUniform1f( m_TextUniforms[GLSL::ETextUniform_PxRange], a_Data.PixelRange );
        glUniform1f( m_TextUniforms[GLSL::ETextUniform_Scale], a_Data.Scale );

        Detail::UniformColor( m_TextUniforms[GLSL::ETextUniform_FillColor], a_Data.FillColor );
        glUniform1f( m_TextUniforms[GLSL::ETextUniform_FillSoftness], a_Data.FillSoftness );
        glUniform1f( m_TextUniforms[GLSL::ETextUniform_FillThreshold], a_Data.FillThreshold );

        if ( a_Data.ShadowEnable )
        {
            Detail::UniformColor( m_TextUniforms[GLSL::ETextUniform_ShadowColor], a_Data.ShadowColor );
            glUniform2f( m_TextUniforms[GLSL::ETextUniform_ShadowOffset], a_Data.ShadowOffsetUV[0], a_Data.ShadowOffsetUV[1] );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_ShadowSoftness], a_Data.ShadowSoftness );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_ShadowSpread], a_Data.ShadowSpread );
        }
        else
        {
            Detail::UniformColorZero( m_TextUniforms[GLSL::ETextUniform_ShadowColor] );
            glUniform2f( m_TextUniforms[GLSL::ETextUniform_ShadowOffset], 0.f, 0.f );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_ShadowSoftness], 0.f );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_ShadowSpread], 0.f );
        }

        if ( a_Data.OutlineEnable )
        {
            Detail::UniformColor( m_TextUniforms[GLSL::ETextUniform_OutlineColor], a_Data.OutlineColor );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_OutlineWidth], a_Data.OutlineWidth );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_OutlineSoftness], a_Data.OutlineSoftness );
        }
        else
        {
            Detail::UniformColorZero( m_TextUniforms[GLSL::ETextUniform_OutlineColor] );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_OutlineWidth], 0.f );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_OutlineSoftness], 0.f );
        }

        if ( a_Data.GlowEnable )
        {
            Detail::UniformColor( m_TextUniforms[GLSL::ETextUniform_GlowColor], a_Data.GlowColor );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_GlowSpread], a_Data.GlowSpread );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_GlowPower], a_Data.GlowPower );
        }
        else
        {
            Detail::UniformColorZero( m_TextUniforms[GLSL::ETextUniform_GlowColor] );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_GlowSpread], 0.f );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_GlowPower], 0.f );
        }

        Detail::UniformColorZero( m_TextUniforms[GLSL::ETextUniform_InnerGlowColor] );
        glUniform1f( m_TextUniforms[GLSL::ETextUniform_InnerGlowRange], 0.f );
        glUniform1f( m_TextUniforms[GLSL::ETextUniform_InnerGlowSoftness], 0.f );
    }

    void OpenGLRenderer::SetClipRect( const Optional<Rectu16>& a_ClipRect )
    {
        if ( !HasValue( a_ClipRect ) )
        {
            glDisable( GL_SCISSOR_TEST );
            return;
        }

        const Rectu16& cr = *a_ClipRect;
        glEnable( GL_SCISSOR_TEST );
        glScissor(
            static_cast<GLint>( cr.Origin[0] ),
            static_cast<GLint>( m_ViewportHeight ) - static_cast<GLint>( cr.Origin[1] ) - static_cast<GLint>( cr.Size[1] ),
            static_cast<GLsizei>( cr.Size[0] ),
            static_cast<GLsizei>( cr.Size[1] ) );
    }

    GLenum OpenGLRenderer::FormatToGLInternal( ETextureFormat a_Format )
    {
        switch ( a_Format )
        {
            case ETextureFormat::R8: return GL_R8;
            case ETextureFormat::RG8: return GL_RG8;
            case ETextureFormat::RGB8: return GL_RGB8;
            case ETextureFormat::RGBA8: return GL_RGBA8;
            default: return GL_RGBA8;
        }
    }

    GLenum OpenGLRenderer::FormatToGLBase( ETextureFormat a_Format )
    {
        switch ( a_Format )
        {
            case ETextureFormat::R8: return GL_RED;
            case ETextureFormat::RG8: return GL_RG;
            case ETextureFormat::RGB8: return GL_RGB;
            case ETextureFormat::RGBA8: return GL_RGBA;
            default: return GL_RGBA;
        }
    }

    void OpenGLRenderer::BuildOrthoProjection( int a_Width, int a_Height )
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

} // namespace RatUI::OpenGL
