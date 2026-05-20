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
        GLuint CompileShader( GLenum a_Type, const char* a_Src );

        /**
         * @brief Link a GLSL program from vertex and fragment shader sources, with error checking.
         * @param a_VertSrc  Null-terminated vertex shader GLSL source.
         * @param a_FragSrc  Null-terminated fragment shader GLSL source.
         * @return OpenGL program object ID.
         */
        GLuint LinkProgram( const char* a_VertSrc, const char* a_FragSrc );

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
        void ToMat4( const Mat3f& a_Mat, f32 o_Result[16] );

        /** @brief Upload a Color (4 x u8) as a normalised vec4 uniform. */
        void UniformColor( GLint a_Loc, Color a_Color );

        /** @brief Upload a zero vec4 (transparent black) to a colour uniform. */
        void UniformColorZero( GLint a_Loc );

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
        explicit OpenGLRenderer( int a_ViewportWidth = 800, int a_ViewportHeight = 600 );
        ~OpenGLRenderer() override;

        OpenGLRenderer( const OpenGLRenderer& )            = delete;
        OpenGLRenderer& operator=( const OpenGLRenderer& ) = delete;

        /** @brief Updates the orthographic projection to match a new framebuffer size. */
        void SetViewport( int a_Width, int a_Height );
        void Execute( const DrawBatcher& a_Batcher ) override;
        TextureHandle CreateTexture( u32 a_Width, u32 a_Height, ETextureFormat a_Format, const void* a_Data ) override;
        bool UpdateTexture( TextureID a_Texture, u32 a_MipLevel, Rectu a_Region, const void* a_Data, size a_DataSizeBytes ) override;
        void DestroyTexture( TextureID a_Texture ) override;
        bool IsValidTexture( TextureID a_Texture ) const override;

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

} // namespace RatUI::OpenGL
