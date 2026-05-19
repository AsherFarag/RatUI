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
    namespace Detail
    {
        /**
         * @brief Compile a GLSL shader of the given type from source, with error checking.
         * @param a_Type  GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
         * @param a_Src   Null-terminated GLSL source string.
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
         * @param a_VertSrc  Null-terminated vertex shader GLSL source.
         * @param a_FragSrc  Null-terminated fragment shader GLSL source.
         * @return OpenGL program object ID.
         */
        inline GLuint LinkProgram( const char* a_VertSrc, const char* a_FragSrc )
        {
            GLuint vert = CompileShader( GL_VERTEX_SHADER,   a_VertSrc );
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
                // TODO: Add user log func
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
        inline void ToMat4( const Mat3f& a_Mat, f32 o_Result[16] )
        {
            // Column 0
            o_Result[ 0] = a_Mat[0][0];
            o_Result[ 1] = a_Mat[0][1];
            o_Result[ 2] = 0.f;
            o_Result[ 3] = 0.f;
            // Column 1
            o_Result[ 4] = a_Mat[1][0];
            o_Result[ 5] = a_Mat[1][1];
            o_Result[ 6] = 0.f;
            o_Result[ 7] = 0.f;
            // Column 2 (Z pass-through)
            o_Result[ 8] = 0.f;
            o_Result[ 9] = 0.f;
            o_Result[10] = 1.f;
            o_Result[11] = 0.f;
            // Column 3 (translation)
            o_Result[12] = a_Mat[2][0];
            o_Result[13] = a_Mat[2][1];
            o_Result[14] = 0.f;
            o_Result[15] = 1.f;
        }

        /** @brief Upload a Color (4 x u8) as a normalised vec4 uniform. */
        inline void UniformColor( GLint a_Loc, Color a_Color )
        {
            glUniform4f( a_Loc,
                         a_Color[0] / 255.f,
                         a_Color[1] / 255.f,
                         a_Color[2] / 255.f,
                         a_Color[3] / 255.f );
        }

        /** @brief Upload a zero vec4 (transparent black) to a colour uniform. */
        inline void UniformColorZero( GLint a_Loc )
        {
            glUniform4f( a_Loc, 0.f, 0.f, 0.f, 0.f );
        }

    } // namespace Detail

    // =========================================================================
    // OpenGLRenderer
    // =========================================================================

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
        explicit OpenGLRenderer( int a_ViewportWidth = 800, int a_ViewportHeight = 600 )
        {
            static_assert( sizeof( SDFVertex  ) == 48, "SDFVertex layout assumption broken"  );
            static_assert( sizeof( TextVertex ) == 20, "TextVertex layout assumption broken" );

            // ----- GLSL programs -------------------------------------------
            m_SDFProgram  = Detail::LinkProgram( GLSL::c_SDFVertSrc,  GLSL::c_SDFFragSrc  );
            m_MSDFProgram = Detail::LinkProgram( GLSL::c_TextVertSrc, GLSL::c_TextFragSrc );

            // Collect uniform locations for both programs up front.
            const auto collectUniforms = []( GLuint a_Program, const char* const* a_Names, GLint* a_OutLocs, i32 a_Count )
            {
                for ( i32 i = 0; i < a_Count; ++i )
                    a_OutLocs[i] = glGetUniformLocation( a_Program, a_Names[i] );
            };

            collectUniforms( m_SDFProgram,  GLSL::c_SDFUniformNames,  m_SDFUniforms,  (i32)GLSL::ESDFUniform_UniformCount  );
            collectUniforms( m_MSDFProgram, GLSL::c_TextUniformNames, m_TextUniforms, (i32)GLSL::ETextUniform_UniformCount );

            // ----- Shared VBO + IBO ----------------------------------------
            glGenBuffers( 1, &m_VBO );
            glGenBuffers( 1, &m_IBO );

            // ----- SDF VAO -------------------------------------------------
            // SDFVertex memory layout (48 bytes total):
            //   loc 0 |  0 | vec2  | Position        (2 x f32)
            //   loc 1 |  8 | vec2  | LocalPos        (2 x f32)
            //   loc 2 | 16 | vec2  | UV              (2 x f32)
            //   loc 3 | 24 | vec4  | FillColor       (4 x u8, normalised)
            //   loc 4 | 28 | vec4  | BorderColor     (4 x u8, normalised)
            //   loc 5 | 32 | float | BorderThickness (1 x f32)
            //   loc 6 | 36 | vec2  | HalfSize        (2 x f32)
            //   loc 7 | 44 | float | CornerRadius    (1 x f32)
            glGenVertexArrays( 1, &m_SDFVAO );
            glBindVertexArray( m_SDFVAO );
            glBindBuffer( GL_ARRAY_BUFFER,         m_VBO );
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBO );
            for ( int i = 0; i < 8; ++i )
                glEnableVertexAttribArray( i );
            glBindVertexArray( 0 );

            // ----- Text VAO ------------------------------------------------
            // TextVertex memory layout (20 bytes total):
            //   loc 0 |  0 | vec2  | Position (2 x f32)
            //   loc 1 |  8 | float | Opacity  (1 x f32)
            //   loc 2 | 12 | vec2  | UV       (2 x f32)
            glGenVertexArrays( 1, &m_TextVAO );
            glBindVertexArray( m_TextVAO );
            glBindBuffer( GL_ARRAY_BUFFER,         m_VBO );
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBO );
            for ( int i = 0; i < 3; ++i )
                glEnableVertexAttribArray( i );
            glBindVertexArray( 0 );

            SetViewport( a_ViewportWidth, a_ViewportHeight );
        }

        ~OpenGLRenderer() override
        {
            glDeleteVertexArrays( 1, &m_SDFVAO   );
            glDeleteVertexArrays( 1, &m_TextVAO  );
            glDeleteBuffers     ( 1, &m_VBO       );
            glDeleteBuffers     ( 1, &m_IBO       );
            glDeleteProgram     ( m_SDFProgram    );
            glDeleteProgram     ( m_MSDFProgram   );
        }

        OpenGLRenderer( const OpenGLRenderer& )            = delete;
        OpenGLRenderer& operator=( const OpenGLRenderer& ) = delete;

        /** @brief Updates the orthographic projection to match a new framebuffer size. */
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

            // Stream the full frame's vertex and index data in one upload each.
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

            // Fixed render state for the entire frame.
            glEnable    ( GL_BLEND );
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glDisable   ( GL_DEPTH_TEST );
            glDisable   ( GL_CULL_FACE  );

            for ( const DrawBatch& batch : a_Batcher.Batches )
            {
                if ( batch.IndexCount == 0 )
                    continue;

                SetClipRect( batch.ClipRect );

                f32 pvm[16];
                Detail::ToMat4( m_Projection * batch.Transform, pvm );

                const GLvoid* indexByteOffset = reinterpret_cast<const GLvoid*>(
                    static_cast<uintptr_t>( batch.IndexOffset ) * sizeof( u16 ) );

                std::visit( [&]( const auto& a_Data )
                {
                    DispatchBatch( a_Data, batch.VertexByteOffset, pvm );
                }, batch.Data );

                glDrawElements( GL_TRIANGLES,
                                static_cast<GLsizei>( batch.IndexCount ),
                                GL_UNSIGNED_SHORT,
                                indexByteOffset );
            }

            glDisable( GL_SCISSOR_TEST );
            glBindVertexArray( 0 );
        }

        TextureHandle CreateTexture( u32 a_Width, u32 a_Height,
                                     ETextureFormat a_Format, const void* a_Data ) override
        {
            GLuint texID = 0;
            glGenTextures( 1, &texID );
            glBindTexture( GL_TEXTURE_2D, texID );

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR        );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR        );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE );

            glTexImage2D( GL_TEXTURE_2D, 0,
                          static_cast<GLint>( FormatToGLInternal( a_Format ) ),
                          static_cast<GLsizei>( a_Width ),
                          static_cast<GLsizei>( a_Height ),
                          0, FormatToGLBase( a_Format ), GL_UNSIGNED_BYTE, a_Data );

            glBindTexture( GL_TEXTURE_2D, 0 );

            TextureID id;
            id.ID = static_cast<uptr>( texID );
            return TextureHandle( MakeShared<Texture>( *this, id ) );
        }

        bool UpdateTexture( TextureID a_Texture, u32 /*a_MipLevel*/,
                            Rectu a_Region, const void* a_Data, size a_DataSizeBytes ) override
        {
            if ( a_Texture == TextureID::Null() || !a_Data )
                return false;

            const u32 w = a_Region.Size[0];
            const u32 h = a_Region.Size[1];
            if ( w == 0 || h == 0 )
                return true;

            // Infer base format from the bytes-per-pixel ratio.
            const size pixelCount = static_cast<size>( w ) * h;
            GLenum fmt = GL_RED;
            if      ( a_DataSizeBytes == pixelCount * 3u ) fmt = GL_RGB;
            else if ( a_DataSizeBytes == pixelCount * 4u ) fmt = GL_RGBA;

            glBindTexture( GL_TEXTURE_2D, static_cast<GLuint>( a_Texture.ID ) );

            // Ensure tight row packing for sub-region uploads.
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
			if ( IsValidTexture( a_Texture ) )
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

        // =====================================================================
        // Batch dispatch — one overload per draw data type
        // =====================================================================

        void DispatchBatch( const SDFDrawData& a_Data, u32 a_VertexByteOffset, const f32 a_PVM[16] )
        {
            if ( IsValidTexture( a_Data.Texture ) )
            {
                glActiveTexture( GL_TEXTURE0 );
                glBindTexture( GL_TEXTURE_2D, static_cast<GLuint>( a_Data.Texture.ID ) );
            }

            // Re-specify attrib pointers for this batch's region of the shared VBO.
            // SDFVertex layout (48 bytes):
            //   loc 0 |  0 | vec2  | Position
            //   loc 1 |  8 | vec2  | LocalPos
            //   loc 2 | 16 | vec2  | UV
            //   loc 3 | 24 | vec4  | FillColor       (u8, normalised)
            //   loc 4 | 28 | vec4  | BorderColor     (u8, normalised)
            //   loc 5 | 32 | float | BorderThickness
            //   loc 6 | 36 | vec2  | HalfSize
            //   loc 7 | 44 | float | CornerRadius
            glBindVertexArray( m_SDFVAO );
            const auto vo = static_cast<uintptr_t>( a_VertexByteOffset );
            glVertexAttribPointer( 0, 2, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)( vo +  0 ) );
            glVertexAttribPointer( 1, 2, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)( vo +  8 ) );
            glVertexAttribPointer( 2, 2, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 16 ) );
            glVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof( SDFVertex ), (const void*)( vo + 24 ) );
            glVertexAttribPointer( 4, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof( SDFVertex ), (const void*)( vo + 28 ) );
            glVertexAttribPointer( 5, 1, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 32 ) );
            glVertexAttribPointer( 6, 2, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 36 ) );
            glVertexAttribPointer( 7, 1, GL_FLOAT,         GL_FALSE, sizeof( SDFVertex ), (const void*)( vo + 44 ) );

            glUseProgram( m_SDFProgram );
            glUniformMatrix4fv( m_SDFUniforms[GLSL::ESDFUniform_PVM], 1, GL_FALSE, a_PVM );
        }

        void DispatchBatch( const MSDFTextDrawData& a_Data, u32 a_VertexByteOffset, const f32 a_PVM[16] )
        {
            if ( IsValidTexture( a_Data.FontAtlas ) )
            {
                glActiveTexture( GL_TEXTURE0 );
                glBindTexture( GL_TEXTURE_2D, static_cast<GLuint>( a_Data.FontAtlas.ID ) );
            }

            // Re-specify attrib pointers for this batch's region of the shared VBO.
            // TextVertex layout (20 bytes):
            //   loc 0 |  0 | vec2  | Position
            //   loc 1 |  8 | float | Opacity
            //   loc 2 | 12 | vec2  | UV
            glBindVertexArray( m_TextVAO );
            const auto vo = static_cast<uintptr_t>( a_VertexByteOffset );
            glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( TextVertex ), (const void*)( vo +  0 ) );
            glVertexAttribPointer( 1, 1, GL_FLOAT, GL_FALSE, sizeof( TextVertex ), (const void*)( vo +  8 ) );
            glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( TextVertex ), (const void*)( vo + 12 ) );

            glUseProgram( m_MSDFProgram );
            glUniformMatrix4fv( m_TextUniforms[GLSL::ETextUniform_PVM],           1,        GL_FALSE,          a_PVM               );
            glUniform1i        ( m_TextUniforms[GLSL::ETextUniform_Atlas],                                      0                   );
            glUniform1f        ( m_TextUniforms[GLSL::ETextUniform_PxRange],                                    a_Data.PixelRange   );
            glUniform1f        ( m_TextUniforms[GLSL::ETextUniform_Scale],                                      a_Data.Scale        );

            // Fill (always present)
            Detail::UniformColor( m_TextUniforms[GLSL::ETextUniform_FillColor], a_Data.FillColor );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_FillSoftness],  a_Data.FillSoftness  );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_FillThreshold], a_Data.FillThreshold );

            // Shadow
            if ( a_Data.ShadowEnable )
            {
                Detail::UniformColor( m_TextUniforms[GLSL::ETextUniform_ShadowColor], a_Data.ShadowColor );
                glUniform2f( m_TextUniforms[GLSL::ETextUniform_ShadowOffset],   a_Data.ShadowOffsetUV[0], a_Data.ShadowOffsetUV[1] );
                glUniform1f( m_TextUniforms[GLSL::ETextUniform_ShadowSoftness], a_Data.ShadowSoftness                              );
                glUniform1f( m_TextUniforms[GLSL::ETextUniform_ShadowSpread],   a_Data.ShadowSpread                                );
            }
            else
            {
                Detail::UniformColorZero( m_TextUniforms[GLSL::ETextUniform_ShadowColor] );
                glUniform2f( m_TextUniforms[GLSL::ETextUniform_ShadowOffset],   0.f, 0.f );
                glUniform1f( m_TextUniforms[GLSL::ETextUniform_ShadowSoftness], 0.f      );
                glUniform1f( m_TextUniforms[GLSL::ETextUniform_ShadowSpread],   0.f      );
            }

            // Outline
            if ( a_Data.OutlineEnable )
            {
                Detail::UniformColor( m_TextUniforms[GLSL::ETextUniform_OutlineColor], a_Data.OutlineColor );
                glUniform1f( m_TextUniforms[GLSL::ETextUniform_OutlineWidth],    a_Data.OutlineWidth    );
                glUniform1f( m_TextUniforms[GLSL::ETextUniform_OutlineSoftness], a_Data.OutlineSoftness );
            }
            else
            {
                Detail::UniformColorZero( m_TextUniforms[GLSL::ETextUniform_OutlineColor] );
                glUniform1f( m_TextUniforms[GLSL::ETextUniform_OutlineWidth],    0.f );
                glUniform1f( m_TextUniforms[GLSL::ETextUniform_OutlineSoftness], 0.f );
            }

            // Glow
            if ( a_Data.GlowEnable )
            {
                Detail::UniformColor( m_TextUniforms[GLSL::ETextUniform_GlowColor], a_Data.GlowColor );
                glUniform1f( m_TextUniforms[GLSL::ETextUniform_GlowSpread], a_Data.GlowSpread );
                glUniform1f( m_TextUniforms[GLSL::ETextUniform_GlowPower],  a_Data.GlowPower  );
            }
            else
            {
                Detail::UniformColorZero( m_TextUniforms[GLSL::ETextUniform_GlowColor] );
                glUniform1f( m_TextUniforms[GLSL::ETextUniform_GlowSpread], 0.f );
                glUniform1f( m_TextUniforms[GLSL::ETextUniform_GlowPower],  0.f );
            }

            // Inner glow (TODO)
            Detail::UniformColorZero( m_TextUniforms[GLSL::ETextUniform_InnerGlowColor] );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_InnerGlowRange],    0.f );
            glUniform1f( m_TextUniforms[GLSL::ETextUniform_InnerGlowSoftness], 0.f );
        }

        // =====================================================================
        // Helpers
        // =====================================================================

        /** @brief Enable or disable scissor test and set the clip rect (Y-flipped for GL). */
        void SetClipRect( const Optional<Rectu16>& a_ClipRect )
        {
            if ( !HasValue( a_ClipRect ) )
            {
                glDisable( GL_SCISSOR_TEST );
                return;
            }

            const Rectu16& cr = *a_ClipRect;
            glEnable( GL_SCISSOR_TEST );
            glScissor(
                static_cast<GLint>   ( cr.Origin[0] ),
                static_cast<GLint>   ( m_ViewportHeight ) - static_cast<GLint>( cr.Origin[1] ) - static_cast<GLint>( cr.Size[1] ),
                static_cast<GLsizei> ( cr.Size[0] ),
                static_cast<GLsizei> ( cr.Size[1] )
            );
        }

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

        /**
         * @brief Build a column-major 3x3 orthographic projection for RatUI's Mat3f.
         *
         * Maps pixel coordinates (origin top-left, Y-down) to NDC [-1, 1]:
         *   x' =  2/w * x - 1
         *   y' = -2/h * y + 1
         */
        void BuildOrthoProjection( int a_Width, int a_Height )
        {
            const float w = static_cast<float>( a_Width  );
            const float h = static_cast<float>( a_Height );

            m_Projection[0u][0] =  2.f / w;
            m_Projection[0u][1] =  0.f;
            m_Projection[0u][2] =  0.f;

            m_Projection[1u][0] =  0.f;
            m_Projection[1u][1] = -2.f / h;
            m_Projection[1u][2] =  0.f;

            m_Projection[2u][0] = -1.f;
            m_Projection[2u][1] =  1.f;
            m_Projection[2u][2] =  1.f;
        }

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
    };

} // namespace RatUI::OpenGL
