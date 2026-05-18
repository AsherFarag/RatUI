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
 * **Shader inputs expected from the host program**
 * The renderer manages its own GLSL programs internally.
 *
 * **Coordinate system**
 * Clip-space transform is injected via the u_Projection uniform (ortho 2-D,
 * top-left origin, Y-down).  Call SetViewport() or call SetProjection()
 * directly.
 */

#ifdef RATUI_OPENGL_INCLUDE
#   include RATUI_OPENGL_INCLUDE
#endif

namespace RatUI::OpenGL
{
    namespace Detail
    {
        /**
         * @brief Compile a GLSL shader of the given type from source, with error checking.
         * @param a_Type  GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
         * @param a_Src   Null-terminated GLSL source code string.
         * @return OpenGL shader object ID.
         */
        inline GLuint CompileShader( GLenum a_Type, const char* a_Src )
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
                // TODO: Add user log func
                RATUI_ASSERT( false, "OpenGLRenderer: shader compile error" );
            }

            return shader;
        }

        /**
         * @brief Link a GLSL program from vertex and fragment shader sources, with error checking.
         * @param a_VertSrc   Null-terminated vertex shader GLSL source code string.
         * @param a_FragSrc   Null-terminated fragment shader GLSL source code string.
         * @return OpenGL program object ID.
         */
        inline GLuint LinkProgram( const char* a_VertSrc, const char* a_FragSrc )
        {
            GLuint vert = CompileShader( GL_VERTEX_SHADER,   a_VertSrc  );
            GLuint frag = CompileShader( GL_FRAGMENT_SHADER, a_FragSrc  );

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
                // TODO: Add user log func
                RATUI_ASSERT( false, "OpenGLRenderer: program link error" );
            }

            glDeleteShader( vert );
            glDeleteShader( frag );
            return prog;
        }

        /**
         * @brief Convert a 3x3 matrix to a 4x4 matrix.
         * @param a_Mat   The 3x3 matrix to convert.
         * @param o_Result The resulting 4x4 matrix.
         */
        static void ToMat4( const Mat3f& a_Mat, f32 o_Result[16] )
        {
            // Column-major (OpenGL expects column-major when transpose = GL_FALSE)

            o_Result[0] = a_Mat[0][0];
            o_Result[1] = a_Mat[0][1];
            o_Result[2] = 0.f;
            o_Result[3] = 0.f;

            o_Result[4] = a_Mat[1][0];
            o_Result[5] = a_Mat[1][1];
            o_Result[6] = 0.f;
            o_Result[7] = 0.f;

            o_Result[8]  = 0.f;
            o_Result[9]  = 0.f;
            o_Result[10] = 1.f;
            o_Result[11] = 0.f;

            o_Result[12] = a_Mat[2][0];
            o_Result[13] = a_Mat[2][1];
            o_Result[14] = 0.f;
            o_Result[15] = 1.f;
        }

    } // namespace Detail

    // =========================================================================
    // OpenGLRenderer
    // =========================================================================

    /**
     * @brief OpenGL renderer for RatUI.
     *
     * Maintains two VAO/VBO/IBO sets — one for SDF shapes, one for MSDF text —
     * both sharing a single VBO + IBO that are re-uploaded each frame.
     *
     * Programs compiled once on construction:
     * - SDF      : SDFVertex layout — rounded rects, circles, borders.
     * - MSDFText : TextVertex layout — MSDF glyph rendering.
     *
     * The renderer assumes a standard 2-D orthographic projection with the
     * top-left corner at (0, 0).  Call SetViewport() when the window is resized.
     */
    class OpenGLRenderer : public IRenderer
    {
    public:
        /**
         * @param a_ViewportWidth   Initial viewport / framebuffer width in pixels.
         * @param a_ViewportHeight  Initial viewport / framebuffer height in pixels.
         */
        explicit OpenGLRenderer( int a_ViewportWidth = 800, int a_ViewportHeight = 600 )
        {
            static_assert( sizeof( SDFVertex )  == 48, "SDFVertex layout assumption broken" );
            static_assert( sizeof( TextVertex ) == 20, "TextVertex layout assumption broken" );

            // ----- GLSL programs -------------------------------------------
            m_SDFProgram  = Detail::LinkProgram( GLSL::c_SDFVertSrc, GLSL::c_SDFFragSrc );
            m_MSDFProgram = Detail::LinkProgram( GLSL::c_TextVertSrc, GLSL::c_TextFragSrc );

            const auto collectUniformLocations = [this]( GLuint program, const char* const* names, GLint* outLocs, i32 count )
            {
                for ( i32 i = 0; i < count; ++i )
                    outLocs[i] = glGetUniformLocation( program, names[i] );
            };

            collectUniformLocations( m_SDFProgram,  GLSL::c_SDFUniformNames,  m_SDFUniforms,  (i32)GLSL::ESDFUniform_UniformCount );
            collectUniformLocations( m_MSDFProgram, GLSL::c_TextUniformNames, m_TextUniforms, (i32)GLSL::ETextUniform_UniformCount );

            // ----- Shared VBO + IBO ----------------------------------------
            glGenBuffers( 1, &m_VBO );
            glGenBuffers( 1, &m_IBO );

            // ----- SDF VAO (SDFVertex layout) --------------------------------
            // SDFVertex offsets (verified by static_assert above):
            //   0  : Position        (2 x float)
            //   8  : LocalPos        (2 x float)
            //  16  : UV              (2 x float, reserved)
            //  24  : FillColor       (4 x u8, normalised)
            //  28  : BorderColor     (4 x u8, normalised)
            //  32  : BorderThickness (1 x float)
            //  36  : HalfSize        (2 x float)
            //  44  : CornerRadius    (1 x float)
            glGenVertexArrays( 1, &m_SDFVAO );
            glBindVertexArray( m_SDFVAO );
            glBindBuffer( GL_ARRAY_BUFFER,         m_VBO );
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBO );

            for ( int i = 0; i <= 7; ++i )
                glEnableVertexAttribArray( i );

            // Initial attrib pointers (byte offsets relative to VBO start = 0).
            // These are re-specified per batch in Execute() to account for VertexByteOffset.
            glVertexAttribPointer( 0, 2, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)  0 );
            glVertexAttribPointer( 1, 2, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)  8 );
            glVertexAttribPointer( 2, 2, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*) 16 );
            glVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof( SDFVertex ), (const void*) 24 );
            glVertexAttribPointer( 4, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof( SDFVertex ), (const void*) 28 );
            glVertexAttribPointer( 5, 1, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*) 32 );
            glVertexAttribPointer( 6, 2, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*) 36 );
            glVertexAttribPointer( 7, 1, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*) 44 );

            // ----- Text VAO (TextVertex layout) ------------------------------
            // TextVertex offsets:
            //   0  : Position (2 x float)
            //   8  : Tint     (4 x u8, normalised)
            //  12  : UV       (2 x float)
            glGenVertexArrays( 1, &m_TextVAO );
            glBindVertexArray( m_TextVAO );
            glBindBuffer( GL_ARRAY_BUFFER,         m_VBO );
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBO );

            glEnableVertexAttribArray( 0 );
            glEnableVertexAttribArray( 1 );
            glEnableVertexAttribArray( 2 );

            glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( TextVertex ), (const void*)  0 );
            glVertexAttribPointer( 1, 1, GL_FLOAT, GL_FALSE, sizeof( TextVertex ), (const void*)( 8 ) );
            glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( TextVertex ), (const void*) 12 );

            glBindVertexArray( 0 );

            SetViewport( a_ViewportWidth, a_ViewportHeight );
        }

        ~OpenGLRenderer() override
        {
            glDeleteBuffers( 1, &m_VBO );
            glDeleteBuffers( 1, &m_IBO );
            glDeleteVertexArrays( 1, &m_SDFVAO );
            glDeleteVertexArrays( 1, &m_TextVAO );
            glDeleteProgram( m_SDFProgram );
            glDeleteProgram( m_MSDFProgram );
        }

        OpenGLRenderer( const OpenGLRenderer& ) = delete;
        OpenGLRenderer& operator=( const OpenGLRenderer& ) = delete;

        /** @brief Updates the orthographic projection to match a new viewport size. */
        void SetViewport( int a_Width, int a_Height )
        {
            m_ViewportWidth  = a_Width;
            m_ViewportHeight = a_Height;
            BuildOrthoProjection( a_Width, a_Height );
        }

        void Execute( const DrawBatcher& a_Batcher ) override
        {
            if ( Empty( a_Batcher.Vertices ) || Empty( a_Batcher.Indices ) )
                return;

            // Upload vertex data (raw bytes) and index data.
            glBindBuffer( GL_ARRAY_BUFFER, m_VBO );
            glBufferData( GL_ARRAY_BUFFER,
                          static_cast<GLsizeiptr>( Size( a_Batcher.Vertices ) ),
                          Data( a_Batcher.Vertices ),
                          GL_STREAM_DRAW );

            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBO );
            glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                          static_cast<GLsizeiptr>( Size( a_Batcher.Indices ) * sizeof( u16 ) ),
                          Data( a_Batcher.Indices ),
                          GL_STREAM_DRAW );

            // Global render state.
            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glDisable( GL_DEPTH_TEST );
            glDisable( GL_CULL_FACE );

            for ( const DrawBatch& batch : a_Batcher.Batches )
            {
                if ( batch.IndexCount == 0 )
                    continue;

                // Scissor / clip rect.
                if ( HasValue( batch.ClipRect ) )
                {
                    glEnable( GL_SCISSOR_TEST );
                    const Rectu16& cr = *batch.ClipRect;
                    // GL scissor Y-axis: origin at bottom-left -> flip.
                    glScissor(
                        static_cast<GLint>( cr.Origin[0] ),
                        static_cast<GLint>( m_ViewportHeight ) - static_cast<GLint>( cr.Origin[1] ) - static_cast<GLint>( cr.Size[1] ),
                        static_cast<GLsizei>( cr.Size[0] ),
                        static_cast<GLsizei>( cr.Size[1] )
                    );
                }
                else
                {
                    glDisable( GL_SCISSOR_TEST );
                }

                f32 pvm[16];
                Detail::ToMat4( m_Projection * batch.Transform, pvm );

                const GLvoid* indexByteOffset = reinterpret_cast<const GLvoid*>(
                    static_cast<uintptr_t>( batch.IndexOffset ) * sizeof( u16 ) );

                // Dispatch per batch type, configure vertex layout, set uniforms, draw.
                if ( std::holds_alternative<SDFDrawData>( batch.Data ) )
                {
                    const SDFDrawData& sdf = std::get<SDFDrawData>( batch.Data );

                    if ( IsValidTexture( sdf.Texture ) )
                    {
                        glActiveTexture( GL_TEXTURE0 );
                        glBindTexture( GL_TEXTURE_2D, static_cast<GLuint>( sdf.Texture.ID ) );
                    }

                    // Re-specify SDF attrib pointers for this batch's vertex byte range.
                    glBindVertexArray( m_SDFVAO );
                    const uintptr_t vo = static_cast<uintptr_t>( batch.VertexByteOffset );
                    glVertexAttribPointer( 0, 2, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)( vo +  0 ) );
                    glVertexAttribPointer( 1, 2, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)( vo +  8 ) );
                    glVertexAttribPointer( 2, 2, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 16 ) );
                    glVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof( SDFVertex ), (const void*)( vo + 24 ) );
                    glVertexAttribPointer( 4, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof( SDFVertex ), (const void*)( vo + 28 ) );
                    glVertexAttribPointer( 5, 1, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 32 ) );
                    glVertexAttribPointer( 6, 2, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 36 ) );
                    glVertexAttribPointer( 7, 1, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 44 ) );

                    glUseProgram( m_SDFProgram );
                    glUniformMatrix4fv( m_SDFUniforms[GLSL::ESDFUniform_PVM], 1, GL_FALSE, pvm );
                }
                else // MSDFTextDrawData
                {
                    const MSDFTextDrawData& text = std::get<MSDFTextDrawData>( batch.Data );

                    if ( IsValidTexture( text.FontAtlas ) )
                    {
                        glActiveTexture( GL_TEXTURE0 );
                        glBindTexture( GL_TEXTURE_2D, static_cast<GLuint>( text.FontAtlas.ID ) );
                    }

                    // Re-specify Text attrib pointers for this batch's vertex byte range.
                    glBindVertexArray( m_TextVAO );
                    const uintptr_t vo = static_cast<uintptr_t>( batch.VertexByteOffset );
                    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( TextVertex ), (const void*)( vo +  0 ) );
                    glVertexAttribPointer( 1, 1, GL_FLOAT, GL_FALSE, sizeof( TextVertex ), (const void*)( vo +  8 ) );
                    glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( TextVertex ), (const void*)( vo + 12 ) );

                    glUseProgram( m_MSDFProgram );
                    glUniformMatrix4fv( m_TextUniforms[GLSL::ETextUniform_PVM], 1, GL_FALSE, pvm );
                    glUniform1i( m_TextUniforms[GLSL::ETextUniform_Atlas],         0                     );
                    glUniform1f( m_TextUniforms[GLSL::ETextUniform_PxRange],       text.PixelRange        );
                    glUniform1f( m_TextUniforms[GLSL::ETextUniform_Scale],         text.Scale             );
                    glUniform4f( m_TextUniforms[GLSL::ETextUniform_FillColor],
                                 text.FillColor[0] / 255.f, text.FillColor[1] / 255.f,
                                 text.FillColor[2] / 255.f, text.FillColor[3] / 255.f );
                    glUniform1f( m_TextUniforms[GLSL::ETextUniform_FillSoftness],  text.FillSoftness      );
                    glUniform1f( m_TextUniforms[GLSL::ETextUniform_FillThreshold], text.FillThreshold     );

                    // Shadow
                    if ( text.ShadowEnable )
                    {
                        glUniform4f( m_TextUniforms[GLSL::ETextUniform_ShadowColor],
                                     text.ShadowColor[0] / 255.f, text.ShadowColor[1] / 255.f,
                                     text.ShadowColor[2] / 255.f, text.ShadowColor[3] / 255.f );
                        glUniform2f( m_TextUniforms[GLSL::ETextUniform_ShadowOffset],   text.ShadowOffsetUV[0], text.ShadowOffsetUV[1] );
                        glUniform1f( m_TextUniforms[GLSL::ETextUniform_ShadowSoftness], text.ShadowSoftness    );
                        glUniform1f( m_TextUniforms[GLSL::ETextUniform_ShadowSpread],   text.ShadowSpread      );
                    }
                    else
                    {
                        glUniform4f( m_TextUniforms[GLSL::ETextUniform_ShadowColor],    0.f, 0.f, 0.f, 0.f );
                        glUniform2f( m_TextUniforms[GLSL::ETextUniform_ShadowOffset],   0.f, 0.f           );
                        glUniform1f( m_TextUniforms[GLSL::ETextUniform_ShadowSoftness], 0.f                 );
                        glUniform1f( m_TextUniforms[GLSL::ETextUniform_ShadowSpread],   0.f                 );
                    }

                    // Outline
                    if ( text.OutlineEnable )
                    {
                        glUniform4f( m_TextUniforms[GLSL::ETextUniform_OutlineColor],
                                     text.OutlineColor[0] / 255.f, text.OutlineColor[1] / 255.f,
                                     text.OutlineColor[2] / 255.f, text.OutlineColor[3] / 255.f );
                        glUniform1f( m_TextUniforms[GLSL::ETextUniform_OutlineWidth],    text.OutlineWidth    );
                        glUniform1f( m_TextUniforms[GLSL::ETextUniform_OutlineSoftness], text.OutlineSoftness );
                    }
                    else
                    {
                        glUniform4f( m_TextUniforms[GLSL::ETextUniform_OutlineColor],    0.f, 0.f, 0.f, 0.f );
                        glUniform1f( m_TextUniforms[GLSL::ETextUniform_OutlineWidth],    0.f                 );
                        glUniform1f( m_TextUniforms[GLSL::ETextUniform_OutlineSoftness], 0.f                 );
                    }

                    // Glow
                    if ( text.GlowEnable )
                    {
                        glUniform4f( m_TextUniforms[GLSL::ETextUniform_GlowColor],
                                     text.GlowColor[0] / 255.f, text.GlowColor[1] / 255.f,
                                     text.GlowColor[2] / 255.f, text.GlowColor[3] / 255.f );
                        glUniform1f( m_TextUniforms[GLSL::ETextUniform_GlowSpread], text.GlowSpread );
                        glUniform1f( m_TextUniforms[GLSL::ETextUniform_GlowPower],  text.GlowPower  );
                    }
                    else
                    {
                        glUniform4f( m_TextUniforms[GLSL::ETextUniform_GlowColor],  0.f, 0.f, 0.f, 0.f );
                        glUniform1f( m_TextUniforms[GLSL::ETextUniform_GlowSpread], 0.f                 );
                        glUniform1f( m_TextUniforms[GLSL::ETextUniform_GlowPower],  0.f                 );
                    }

                    // Inner glow (TODO)
                    glUniform4f( m_TextUniforms[GLSL::ETextUniform_InnerGlowColor],    0.f, 0.f, 0.f, 0.f );
                    glUniform1f( m_TextUniforms[GLSL::ETextUniform_InnerGlowRange],    0.f                 );
                    glUniform1f( m_TextUniforms[GLSL::ETextUniform_InnerGlowSoftness], 0.f                 );
                }

                glDrawElements( GL_TRIANGLES,
                                static_cast<GLsizei>( batch.IndexCount ),
                                GL_UNSIGNED_SHORT,
                                indexByteOffset );
            }

            glDisable( GL_SCISSOR_TEST );
            glBindVertexArray( 0 );
        }

        Optional<TextureID> CreateTexture( u32 a_Width, u32 a_Height,
                                           ETextureFormat a_Format, const void* a_Data ) override
        {
            GLuint texID = 0;
            glGenTextures( 1, &texID );
            glBindTexture( GL_TEXTURE_2D, texID );

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

            const GLenum internalFmt = FormatToGLInternal( a_Format );
            const GLenum baseFmt     = FormatToGLBase( a_Format );

            glTexImage2D( GL_TEXTURE_2D, 0, static_cast<GLint>( internalFmt ),
                          static_cast<GLsizei>( a_Width ), static_cast<GLsizei>( a_Height ),
                          0, baseFmt, GL_UNSIGNED_BYTE, a_Data );

            glBindTexture( GL_TEXTURE_2D, 0 );

            TextureID id;
            id.ID = static_cast<uptr>( texID );
            return id;
        }

        bool UpdateTexture( TextureID a_Texture, u32 /*a_MipLevel*/,
                            Rectu a_Region, const void* a_Data, size a_DataSizeBytes ) override
        {
            if ( !a_Texture.IsValid() || !a_Data )
                return false;

            const GLuint texID = static_cast<GLuint>( a_Texture.ID );
            glBindTexture( GL_TEXTURE_2D, texID );

            // Detect format from bytes-per-pixel ratio.
            const u32 w = a_Region.Size[0], h = a_Region.Size[1];
            if ( w == 0 || h == 0 )
                return true;

            const size pixelCount = static_cast<size>( w ) * h;
            const bool isRGB8  = ( a_DataSizeBytes == pixelCount * 3u );
            const bool isRGBA8 = ( a_DataSizeBytes == pixelCount * 4u );

            GLenum fmt = GL_RED;
            if ( isRGB8  ) fmt = GL_RGB;
            if ( isRGBA8 ) fmt = GL_RGBA;

            // Ensure tight row packing for sub-regions.
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

        void DestroyTexture( TextureID a_Texture ) override
        {
            if ( a_Texture.IsValid() )
            {
                GLuint texID = static_cast<GLuint>( a_Texture.ID );
                glDeleteTextures( 1, &texID );
            }
        }

        bool IsValidTexture( TextureID a_Texture ) const override
        {
            return a_Texture.ID != 0;
        }

    private:

        static GLenum FormatToGLInternal( ETextureFormat a_Format )
        {
            switch ( a_Format )
            {
                case ETextureFormat::R8:    return GL_R8;
                case ETextureFormat::RG8:   return GL_RG8;
                case ETextureFormat::RGB8:  return GL_RGB8;
                case ETextureFormat::RGBA8: return GL_RGBA8;
                default:                    return GL_RGBA8;
            }
        }

        static GLenum FormatToGLBase( ETextureFormat a_Format )
        {
            switch ( a_Format )
            {
                case ETextureFormat::R8:    return GL_RED;
                case ETextureFormat::RG8:   return GL_RG;
                case ETextureFormat::RGB8:  return GL_RGB;
                case ETextureFormat::RGBA8: return GL_RGBA;
                default:                    return GL_RGBA;
            }
        }

        /** @brief Builds a column-major 4x4 orthographic projection (OpenGL convention). */
        void BuildOrthoProjection( int width, int height )
        {
            const float w = static_cast<float>( width );
            const float h = static_cast<float>( height );

            m_Projection[0u][0] = 2.0f / w;
            m_Projection[0u][1] = 0.0f;
            m_Projection[0u][2] = 0.0f;

            m_Projection[1u][0] = 0.0f;
            m_Projection[1u][1] = -2.0f / h;
            m_Projection[1u][2] = 0.0f;

            m_Projection[2u][0] = -1.0f;
            m_Projection[2u][1] = 1.0f;
            m_Projection[2u][2] = 1.0f;
        }

        GLuint m_VBO{ 0 }, m_IBO{ 0 };
        GLuint m_SDFVAO{ 0 };
        GLuint m_TextVAO{ 0 };
        GLuint m_SDFProgram{ 0 };
        GLuint m_MSDFProgram{ 0 };

        GLint m_SDFUniforms [GLSL::ESDFUniform_UniformCount]{};
        GLint m_TextUniforms[GLSL::ETextUniform_UniformCount]{};

		Mat3f m_Projection{};
        i32   m_ViewportWidth { 800 };
        i32   m_ViewportHeight{ 600 };
    };

} // namespace RatUI::OpenGL