#pragma once
#include "../../RatUI.h"
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
    // =========================================================================
    // GLSL shader sources
    // =========================================================================

    namespace Detail
    {
        /** @brief Vertex shader shared by both geometry and MSDF text programs. */
        inline constexpr const char* c_VertSrc = R"(
            #version 330 core
            layout(location = 0) in vec2  a_Pos;
            layout(location = 1) in vec4  a_Color;
            layout(location = 2) in vec2  a_UV;
            
            uniform mat4 u_Projection;
            
            out vec4 v_Color;
            out vec2 v_UV;
            
            void main()
            {
                gl_Position = u_Projection * vec4(a_Pos, 0.0, 1.0);
                v_Color     = a_Color;
                v_UV        = a_UV;
            }
        )";

        /** @brief Fragment shader for plain geometry (rectangles, circles). */
        inline constexpr const char* c_GeomFragSrc = R"(
            #version 330 core
            in  vec4 v_Color;
            in  vec2 v_UV;
            
            uniform sampler2D u_Texture;
            uniform bool      u_UseTexture;
            
            out vec4 FragColor;
            
            void main()
            {
                vec4 color = v_Color;
                if (u_UseTexture)
                {
                    // R8 atlas: alpha stored in red channel; vertex color provides tint.
                    float alpha = texture(u_Texture, v_UV).r;
                    color.a *= alpha;
                }
                FragColor = color;
            }
        )";

        /**
         * @brief Fragment shader for MSDF glyphs.
         *
         * Decodes the signed distance stored in the RGB channels via the median
         * operator, then uses a smoothstep edge with a width derived from the
         * current SDF pixel range and scale so the edge quality is independent of
         * display size.
         */
        inline constexpr const char* c_MSDFFragSrc = R"(
            #version 330 core
            in  vec4 v_Color;
            in  vec2 v_UV;
            
            uniform sampler2D u_Texture;
            /** SDF search radius in output pixels (matches atlas pxRange). */
            uniform float     u_PxRange;
            /** Visual scale = FontSize / sdf_base_size. */
            uniform float     u_Scale;
            
            out vec4 FragColor;
            
            float median(float r, float g, float b)
            {
                return max(min(r, g), min(max(r, g), b));
            }
            
            void main()
            {
                // MTSDF: RGB = multi-channel SDF, A = single-channel SDF fallback.
                vec4  mtsdf = texture(u_Texture, v_UV);
                float dist  = median(mtsdf.r, mtsdf.g, mtsdf.b);
                // Use the SDF alpha channel as a fallback so MSDF corner artifacts
                // (where the median can dip below 0.5 inside the glyph) are corrected.
                dist = max(dist, mtsdf.a);

                // screenPxRange: how many screen pixels the SDF transition region spans.
                // fwidth(v_UV) gives the UV change per screen pixel; multiplying by the texture
                // dimensions converts to texels per screen pixel.  Dividing pxRange (in texels)
                // by that value yields the range in screen pixels.
                vec2  texSize          = vec2(textureSize(u_Texture, 0));
                float screenPxPerTexel = length(fwidth(v_UV) * texSize);
                float screenPxRange    = u_PxRange / screenPxPerTexel;
                screenPxRange          = max(screenPxRange, 1.0); // Clamp so very small glyphs don't produce
                                                                  // excessively thin smoothing edges that alias.
            
                float smoothW = 0.5 / screenPxRange;
                float alpha   = smoothstep(0.5 - smoothW, 0.5 + smoothW, dist);
            
                FragColor = vec4(v_Color.rgb, v_Color.a * alpha);
            }
        )";

        // -----------------------------------------------------------------
        // Compile / link helpers
        // -----------------------------------------------------------------

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

    } // namespace Detail

    // =========================================================================
    // OpenGLRenderer
    // =========================================================================

    /**
     * @brief OpenGL renderer for RatUI.
     *
     * Maintains one VAO + VBO + IBO that are re-uploaded each frame.
     * Two GLSL programs are compiled once on construction:
     * - Geometry : used for DrawBatches with Type == EBatchType::Geometry.
     * - MSDFText : used for DrawBatches with Type == EBatchType::MSDFText.
     *
     * The renderer assumes a standard 2-D orthographic projection with the
     * top-left corner at (0, 0). Call SetViewport() when the window is resized.
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
            // ----- GLSL programs -------------------------------------------
            m_GeomProgram = Detail::LinkProgram( Detail::c_VertSrc, Detail::c_GeomFragSrc );
            m_MSDFProgram = Detail::LinkProgram( Detail::c_VertSrc, Detail::c_MSDFFragSrc );

            // Cache uniform locations.
            m_GeomLoc_Projection  = glGetUniformLocation( m_GeomProgram, "u_Projection"  );
            m_GeomLoc_Texture     = glGetUniformLocation( m_GeomProgram, "u_Texture"     );
            m_GeomLoc_UseTexture  = glGetUniformLocation( m_GeomProgram, "u_UseTexture"  );

            m_MSDFLoc_Projection  = glGetUniformLocation( m_MSDFProgram, "u_Projection"  );
            m_MSDFLoc_Texture     = glGetUniformLocation( m_MSDFProgram, "u_Texture"     );
            m_MSDFLoc_PxRange     = glGetUniformLocation( m_MSDFProgram, "u_PxRange"     );
            m_MSDFLoc_Scale       = glGetUniformLocation( m_MSDFProgram, "u_Scale"       );

            // ----- GPU buffers ---------------------------------------------
            glGenVertexArrays( 1, &m_VAO );
            glGenBuffers( 1, &m_VBO );
            glGenBuffers( 1, &m_IBO );

            glBindVertexArray( m_VAO );
            glBindBuffer( GL_ARRAY_BUFFER,         m_VBO );
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBO );

            // Layout must match RatUI::Vertex: { Vec2f Pos, Coloru8 Color, Vec2f UV }
            // Offsets within Vertex:
            //   0  : Pos   (2 x float)
            //   8  : Color (4 x u8, normalized)
            //   12 : UV    (2 x float)
            static_assert( sizeof( Vertex ) == 20, "Vertex layout assumption broken" );

            glEnableVertexAttribArray( 0 );
            glVertexAttribPointer( 0, 2, GL_FLOAT,         GL_FALSE, sizeof( Vertex ),
                                   reinterpret_cast<const void*>( offsetof( Vertex, Position ) ) );

            glEnableVertexAttribArray( 1 );
            glVertexAttribPointer( 1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof( Vertex ),
                                   reinterpret_cast<const void*>( offsetof( Vertex, Color ) ) );

            glEnableVertexAttribArray( 2 );
            glVertexAttribPointer( 2, 2, GL_FLOAT,         GL_FALSE, sizeof( Vertex ),
                                   reinterpret_cast<const void*>( offsetof( Vertex, UV ) ) );

            glBindVertexArray( 0 );

            SetViewport( a_ViewportWidth, a_ViewportHeight );
        }

        ~OpenGLRenderer() override
        {
            glDeleteBuffers( 1, &m_VBO );
            glDeleteBuffers( 1, &m_IBO );
            glDeleteVertexArrays( 1, &m_VAO );
            glDeleteProgram( m_GeomProgram );
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

            // Upload vertex + index data.
            glBindVertexArray( m_VAO );

            glBindBuffer( GL_ARRAY_BUFFER, m_VBO );
            glBufferData( GL_ARRAY_BUFFER,
                          static_cast<GLsizeiptr>( Size( a_Batcher.Vertices ) * sizeof( Vertex ) ),
                          Data( a_Batcher.Vertices ),
                          GL_STREAM_DRAW );

            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBO );
            glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                          static_cast<GLsizeiptr>( Size( a_Batcher.Indices ) * sizeof( u16 ) ),
                          Data( a_Batcher.Indices ),
                          GL_STREAM_DRAW );

            // State setup.
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

                // Bind texture (if any).
                GLuint glTex = 0;
                if ( IsValidTexture( batch.Texture ) )
                {
                    glTex = static_cast<GLuint>( batch.Texture.ID );
                    glActiveTexture( GL_TEXTURE0 );
                    glBindTexture( GL_TEXTURE_2D, glTex );
                }

                // Select program and set uniforms.
                if ( batch.Type == EBatchType::MSDF )
                {
                    glUseProgram( m_MSDFProgram );
                    glUniformMatrix4fv( m_MSDFLoc_Projection, 1, GL_FALSE, m_Projection );
                    glUniform1i( m_MSDFLoc_Texture,  0 );
                    glUniform1f( m_MSDFLoc_PxRange,  static_cast<float>( c_MsdfPxRange ) );
                    glUniform1f( m_MSDFLoc_Scale,    batch.MSDF.Scale );
                }
                else
                {
                    glUseProgram( m_GeomProgram );
                    glUniformMatrix4fv( m_GeomLoc_Projection, 1, GL_FALSE, m_Projection );
                    glUniform1i( m_GeomLoc_Texture,    0 );
                    glUniform1i( m_GeomLoc_UseTexture, glTex != 0 ? GL_TRUE : GL_FALSE );
                }

                // Draw.
                const GLvoid* indexOffset = reinterpret_cast<const GLvoid*>(
                    static_cast<uintptr_t>( batch.IndexOffset ) * sizeof( u16 ) );

                glDrawElements( GL_TRIANGLES,
                                static_cast<GLsizei>( batch.IndexCount ),
                                GL_UNSIGNED_SHORT,
                                indexOffset );
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
        void BuildOrthoProjection( int a_Width, int a_Height )
        {
            // Map [0, w] -> [-1, 1], [0, h] -> [1, -1] (Y-down -> NDC).
            const float L = 0.f, R = static_cast<float>( a_Width );
            const float T = 0.f, B = static_cast<float>( a_Height );
            const float N = -1.f, F = 1.f;

            m_Projection[ 0] =  2.f / ( R - L );
            m_Projection[ 1] =  0.f;
            m_Projection[ 2] =  0.f;
            m_Projection[ 3] =  0.f;

            m_Projection[ 4] =  0.f;
            m_Projection[ 5] =  2.f / ( T - B ); // T - B is negative -> flips Y
            m_Projection[ 6] =  0.f;
            m_Projection[ 7] =  0.f;

            m_Projection[ 8] =  0.f;
            m_Projection[ 9] =  0.f;
            m_Projection[10] = -2.f / ( F - N );
            m_Projection[11] =  0.f;

            m_Projection[12] = -( R + L ) / ( R - L );
            m_Projection[13] = -( T + B ) / ( T - B );
            m_Projection[14] = -( F + N ) / ( F - N );
            m_Projection[15] =  1.f;
        }

        GLuint m_VAO{ 0 }, m_VBO{ 0 }, m_IBO{ 0 };

        GLuint m_GeomProgram{ 0 };
        GLuint m_MSDFProgram{ 0 };

        // Geometry program uniform locations.
        GLint m_GeomLoc_Projection{ -1 };
        GLint m_GeomLoc_Texture   { -1 };
        GLint m_GeomLoc_UseTexture{ -1 };

        // MSDF program uniform locations.
        GLint m_MSDFLoc_Projection{ -1 };
        GLint m_MSDFLoc_Texture   { -1 };
        GLint m_MSDFLoc_PxRange   { -1 };
        GLint m_MSDFLoc_Scale     { -1 };

        float m_Projection[16]{ };
        int   m_ViewportWidth { 800 };
        int   m_ViewportHeight{ 600 };
    };

} // namespace RatUI::OpenGL