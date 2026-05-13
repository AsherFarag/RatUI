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
            
            uniform mat4 u_PVM;
            
            out vec4 v_Color;
            out vec2 v_UV;
            
            void main()
            {
                gl_Position = u_PVM * vec4(a_Pos, 0.0, 1.0);
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

in vec2 v_UV;
out vec4 FragColor;

// - Atlas
uniform sampler2D u_Atlas;          // MTSDF atlas (RGBA, linear filtering)
uniform float     u_PxRange;        // msdfgen pxrange (e.g. 4.0)
uniform float     u_Scale;          // Glyph scale (from batch)
uniform vec2      u_TextureSize;    // Atlas dimensions in texels (e.g. 1024, 512)

// - Fill
uniform vec4  u_FillColor;    
uniform float u_FillSoftness; 
uniform float u_FillThreshold;

// - Outline
uniform vec4  u_OutlineColor;
uniform float u_OutlineWidth;
uniform float u_OutlineSoftness;

// - Shadow
uniform vec4  u_ShadowColor;
uniform vec2  u_ShadowOffset;  
uniform float u_ShadowSoftness;
uniform float u_ShadowSpread;  

// - Glow
uniform vec4  u_GlowColor;
uniform float u_GlowSpread;
uniform float u_GlowPower; 

// - Inner Glow
uniform vec4  u_InnerGlowColor;
uniform float u_InnerGlowRange;
uniform float u_InnerGlowSoftness;

float Median(float a_Red, float a_Green, float a_Blue) 
{
    return max(min(a_Red, a_Green), min(max(a_Red, a_Green), a_Blue));
}

//  Helper: screen-space derivative scale -> converts SDF units to pixels.
//
//  The MTSDF encodes distance in the range [0, 1] across `u_PxRange`
//  screen pixels. We use fwidth to measure one texel in screen-pixel space
//  and multiply by pixel_range to get the correct conversion factor.
//
//  distancePerPixel > 1 -> glyph too small (texels larger than pixels, blurry)
//  distancePerPixel < 1 -> glyph large enough for crisp rendering
float ScreenPxRange(vec2 a_UV) 
{
    vec2 unitRange = vec2(u_PxRange) / u_TextureSize;
    vec2 screenTexSize = vec2(1.0) / fwidth(a_UV);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

//  Sample MTSDF at a given UV and return the signed distance value in [0,1].
//
//  MTSDF atlas stores:  R = SDF channel 1
//                       G = SDF channel 2
//                       B = SDF channel 3
//                       A = true SDF (optional, for small sizes)
//
//  The multi-channel median gives sharp corners; blending with A (true SDF)
//  improves robustness at very small sizes. We take max(msdf, tsdf * weight).
float SampleMTSDF(vec2 a_UV) 
{
    vec4  s    = texture(u_Atlas, a_UV);
    float msdf = Median(s.r, s.g, s.b);
    float tsdf = s.a;
    // Blend: at very small sizes TSDF is more reliable; blend linearly.
    float pxRange = ScreenPxRange(a_UV);
    float weight  = clamp(1.0 - (pxRange - 1.0) / 4.0, 0.0, 1.0);
    return mix(msdf, max(msdf, tsdf), weight);
}

// Helper: convert SDF distance to alpha with smoothstep edge.
//
//  threshold = 0.5           -> glyph fill edge
//  threshold < 0.5           -> expanded region (outline outer edge, glow, shadow)
//  threshold > 0.5           -> contracted region (inner glow)
//
//  a_Softness is in user-facing pixel units. Adding 0.5 guarantees a minimum
//  half-pixel AA transition even at softness=0, preventing hard aliasing.
//  The combined value is then divided by pxRange to convert into SDF units.
float SDFAlpha(float a_Dist, float a_Threshold, float a_Softness, float a_PxRange) 
{
    float sdfSoft = (a_Softness + 0.5) / a_PxRange;
    return smoothstep(a_Threshold - sdfSoft, a_Threshold + sdfSoft, a_Dist);
}

//  Shadow: sample the atlas at an offset UV with a Gaussian-approximated blur.
//
//  Taps are spread along the normalised shadow offset direction so the blur
//  feathers the shadow's leading edge — the visually dominant edge for a
//  directional shadow. The blur radius is expressed as a fraction of the
//  shadow offset length so it scales naturally with the shadow distance:
//  a tight shadow has tight blur; a far-cast shadow has proportionally wider blur.
//  u_ShadowSoftness acts as a 0..1 multiplier on that radius, giving the
//  artist a stable, intuitive range regardless of offset magnitude.
#define SHADOW_TAPS 5

float ShadowAlpha(vec2 a_UV, float a_Threshold, float a_Softness, float a_PxRange) 
{
    // Gaussian kernel weights (sum = 1.0)
    const float W[SHADOW_TAPS] = float[](0.0625, 0.25, 0.375, 0.25, 0.0625);

    // Spread taps along the shadow offset direction.
    // Fallback to x-axis when offset is zero (pure spread shadow, no direction).
    float offsetLen = length(u_ShadowOffset);
    vec2  blurDir   = offsetLen > 1e-5
                        ? (u_ShadowOffset / offsetLen)
                        : vec2(1.0, 0.0);

    // Blur radius scales with offset length so it is proportional to shadow
    // distance. u_ShadowSoftness (0..1) lets the artist scale it down to zero.
    float blurRadiusUV = offsetLen * a_Softness;

    float alpha = 0.0;
    for (int i = 0; i < SHADOW_TAPS; i++)
    {
        float t     = float(i) / float(SHADOW_TAPS - 1) - 0.5; // -0.5 .. +0.5
        vec2  tapUV = a_UV + blurDir * t * blurRadiusUV;
        float d     = SampleMTSDF(tapUV);
        alpha      += W[i] * SDFAlpha(d, a_Threshold, a_Softness, a_PxRange);
    }
    return clamp(alpha, 0.0, 1.0);
}

// =============================================================================
//  Main
// =============================================================================
void main() 
{
    float pxRange = ScreenPxRange(v_UV);
    float dist    = SampleMTSDF(v_UV);

    // - Working colour accumulator (back to front)
    vec4 color = vec4(0.0);

    // - 1. Outer Glow
    //   Expands beyond the outline's outer edge, fading outward.
    if (u_GlowColor.a > 0.0)
    {
        float glowThresh = 0.5 - u_OutlineWidth - u_GlowSpread;
        float glowAlpha  = SDFAlpha(dist, glowThresh, u_GlowPower, pxRange);

        // Mask out the outline+fill interior so the glow only shows outside.
        // Using the outline's outer edge (0.5 - OutlineWidth) rather than the
        // fill edge means no gap appears between the outline and glow when
        // OutlineWidth > 0.
        float interiorAlpha = SDFAlpha(dist, 0.5 - u_OutlineWidth, u_OutlineSoftness, pxRange);
        glowAlpha = clamp(glowAlpha - interiorAlpha, 0.0, 1.0);

        color = mix(color, u_GlowColor, glowAlpha * u_GlowColor.a);
    }

    // - 2. Drop Shadow
    //   Sample at offset UV; sits between glow and outline.
    if (u_ShadowColor.a > 0.0)
    {
        vec2  shadowUV    = v_UV - u_ShadowOffset;
        float shadowThresh = 0.5 - u_ShadowSpread;
        float sAlpha      = ShadowAlpha(shadowUV, shadowThresh, u_ShadowSoftness, pxRange);
        color = mix(color, u_ShadowColor, sAlpha * u_ShadowColor.a);
    }

    // - 3. Outline
    //   Expand the SDF threshold outward by outline width.
    if (u_OutlineColor.a > 0.0)
    {
        float outlineThresh = 0.5 - u_OutlineWidth;
        float outlineAlpha  = SDFAlpha(dist, outlineThresh, u_OutlineSoftness, pxRange);
        color = mix(color, u_OutlineColor, outlineAlpha * u_OutlineColor.a);
    }

    // - 4. Fill
    float fillAlpha = SDFAlpha(dist, u_FillThreshold, u_FillSoftness, pxRange);
    color = mix(color, u_FillColor, fillAlpha * u_FillColor.a);

    // - 5. Inner Glow
    //   Fades inward from the fill edge toward the glyph centre.
    if (u_InnerGlowColor.a > 0.0) 
    {
        // Threshold is relative to u_FillThreshold so it respects fill dilation.
        // Without this, shifting FillThreshold away from 0.5 would visually
        // detach the inner glow from the fill edge it is supposed to hug.
        float innerThresh = u_FillThreshold + u_InnerGlowRange;
        float innerAlpha  = SDFAlpha(dist, innerThresh, u_InnerGlowSoftness, pxRange);
        // Clip to fill region — prevents bleed at soft/feathered fill edges.
        innerAlpha *= fillAlpha;
        color = mix(color, u_InnerGlowColor, innerAlpha * u_InnerGlowColor.a);
    }

    FragColor = color;
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

            // Cache all uniform locations in one shared array.
            m_UniformLocs[EUniform::GeomPVM]         = glGetUniformLocation( m_GeomProgram, "u_PVM" );
            m_UniformLocs[EUniform::GeomTexture]     = glGetUniformLocation( m_GeomProgram, "u_Texture" );
            m_UniformLocs[EUniform::GeomUseTexture]  = glGetUniformLocation( m_GeomProgram, "u_UseTexture" );

            m_UniformLocs[EUniform::MsdfPVM]              = glGetUniformLocation( m_MSDFProgram, "u_PVM" );
            m_UniformLocs[EUniform::MsdfAtlas]            = glGetUniformLocation( m_MSDFProgram, "u_Atlas" );
            m_UniformLocs[EUniform::MsdfPxRange]          = glGetUniformLocation( m_MSDFProgram, "u_PxRange" );
            m_UniformLocs[EUniform::MsdfScale]            = glGetUniformLocation( m_MSDFProgram, "u_Scale" );
            m_UniformLocs[EUniform::MsdfTextureSize]      = glGetUniformLocation( m_MSDFProgram, "u_TextureSize" );

            m_UniformLocs[EUniform::MsdfFillColor]        = glGetUniformLocation( m_MSDFProgram, "u_FillColor" );
            m_UniformLocs[EUniform::MsdfFillSoftness]     = glGetUniformLocation( m_MSDFProgram, "u_FillSoftness" );
            m_UniformLocs[EUniform::MsdfFillThreshold]    = glGetUniformLocation( m_MSDFProgram, "u_FillThreshold" );

            m_UniformLocs[EUniform::MsdfOutlineColor]     = glGetUniformLocation( m_MSDFProgram, "u_OutlineColor" );
            m_UniformLocs[EUniform::MsdfOutlineWidth]     = glGetUniformLocation( m_MSDFProgram, "u_OutlineWidth" );
            m_UniformLocs[EUniform::MsdfOutlineSoftness]  = glGetUniformLocation( m_MSDFProgram, "u_OutlineSoftness" );

            m_UniformLocs[EUniform::MsdfShadowColor]      = glGetUniformLocation( m_MSDFProgram, "u_ShadowColor" );
            m_UniformLocs[EUniform::MsdfShadowOffset]     = glGetUniformLocation( m_MSDFProgram, "u_ShadowOffset" );
            m_UniformLocs[EUniform::MsdfShadowSoftness]   = glGetUniformLocation( m_MSDFProgram, "u_ShadowSoftness" );
            m_UniformLocs[EUniform::MsdfShadowSpread]     = glGetUniformLocation( m_MSDFProgram, "u_ShadowSpread" );

            m_UniformLocs[EUniform::MsdfGlowColor]        = glGetUniformLocation( m_MSDFProgram, "u_GlowColor" );
            m_UniformLocs[EUniform::MsdfGlowSpread]       = glGetUniformLocation( m_MSDFProgram, "u_GlowSpread" );
            m_UniformLocs[EUniform::MsdfGlowPower]        = glGetUniformLocation( m_MSDFProgram, "u_GlowPower" );

            m_UniformLocs[EUniform::MsdfInnerGlowColor]   = glGetUniformLocation( m_MSDFProgram, "u_InnerGlowColor" );
            m_UniformLocs[EUniform::MsdfInnerGlowRange]   = glGetUniformLocation( m_MSDFProgram, "u_InnerGlowRange" );
            m_UniformLocs[EUniform::MsdfInnerGlowSoftness]= glGetUniformLocation( m_MSDFProgram, "u_InnerGlowSoftness" );

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

				f32 pvm[16];
				Detail::ToMat4( m_Projection * batch.Transform, pvm );

                // Select program and set uniforms.
                if ( batch.Type == EBatchType::MSDF )
                {
                    glUseProgram( m_MSDFProgram );
                    glUniformMatrix4fv( m_UniformLocs[EUniform::MsdfPVM], 1, GL_FALSE, pvm );
                    glUniform1i( m_UniformLocs[EUniform::MsdfAtlas], 0 );
                    glUniform1f( m_UniformLocs[EUniform::MsdfPxRange], batch.MSDF.PixelRange );
                    glUniform1f( m_UniformLocs[EUniform::MsdfScale], batch.MSDF.Scale );

                    // Fill: keep white so per-vertex tint defines color
                    glUniform4f( m_UniformLocs[EUniform::MsdfFillColor], 1.f, 1.f, 1.f, 1.f );
                    glUniform1f( m_UniformLocs[EUniform::MsdfFillSoftness], 0.5f );
                    glUniform1f( m_UniformLocs[EUniform::MsdfFillThreshold], 0.5f );

                    // Provide atlas texture size (needed by blur/tap calculations)
                    if ( glTex != 0 && m_UniformLocs[EUniform::MsdfTextureSize] != -1 )
                    {
                        GLint texW = 1, texH = 1;
                        glBindTexture( GL_TEXTURE_2D, glTex );
                        glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &texW );
                        glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH );
                        glUniform2f( m_UniformLocs[EUniform::MsdfTextureSize], static_cast<float>( texW ), static_cast<float>( texH ) );
                    }

                    // Shadow
                    if ( batch.MSDF.ShadowEnable )
                    {
                        glUniform4f( m_UniformLocs[EUniform::MsdfShadowColor],
                                    batch.MSDF.ShadowColor[0] / 255.f,
                                    batch.MSDF.ShadowColor[1] / 255.f,
                                    batch.MSDF.ShadowColor[2] / 255.f,
                                    batch.MSDF.ShadowColor[3] / 255.f );
                        glUniform2f( m_UniformLocs[EUniform::MsdfShadowOffset],
                                     batch.MSDF.ShadowOffsetUV[0],
                                     batch.MSDF.ShadowOffsetUV[1] );
                        glUniform1f( m_UniformLocs[EUniform::MsdfShadowSoftness], batch.MSDF.ShadowSoftness );
						glUniform1f( m_UniformLocs[EUniform::MsdfShadowSpread], batch.MSDF.ShadowSpread );
                    }
                    else
                    {
                        glUniform4f( m_UniformLocs[EUniform::MsdfShadowColor], 0.f, 0.f, 0.f, 0.f );
                    }

                    // Outline
                    if ( batch.MSDF.OutlineEnable )
                    {
                        glUniform4f( m_UniformLocs[EUniform::MsdfOutlineColor],
                                    batch.MSDF.OutlineColor[0] / 255.f,
                                    batch.MSDF.OutlineColor[1] / 255.f,
                                    batch.MSDF.OutlineColor[2] / 255.f,
                                    batch.MSDF.OutlineColor[3] / 255.f );
                        glUniform1f( m_UniformLocs[EUniform::MsdfOutlineWidth], batch.MSDF.OutlineWidth );
                        glUniform1f( m_UniformLocs[EUniform::MsdfOutlineSoftness], batch.MSDF.OutlineSoftness );
                    }
                    else
                    {
                        glUniform4f( m_UniformLocs[EUniform::MsdfOutlineColor], 0.f, 0.f, 0.f, 0.f );
                    }

                    // Glow
                    if ( batch.MSDF.GlowEnable )
                    {
                        glUniform4f( m_UniformLocs[EUniform::MsdfGlowColor],
                                    batch.MSDF.GlowColor[0] / 255.f,
                                    batch.MSDF.GlowColor[1] / 255.f,
                                    batch.MSDF.GlowColor[2] / 255.f,
                                    batch.MSDF.GlowColor[3] / 255.f );
                        glUniform1f( m_UniformLocs[EUniform::MsdfGlowSpread], batch.MSDF.GlowSpread );
                        glUniform1f( m_UniformLocs[EUniform::MsdfGlowPower], batch.MSDF.GlowPower );
                    }
                    else
                    {
                        glUniform4f( m_UniformLocs[EUniform::MsdfGlowColor], 0.f, 0.f, 0.f, 0.f );
                    }

                    // TODO: Doesnt look good
                    glUniform4f( m_UniformLocs[EUniform::MsdfInnerGlowColor], 0.f, 0.f, 0.f, 0.f );
                    glUniform1f( m_UniformLocs[EUniform::MsdfInnerGlowRange], 0.f );
                    glUniform1f( m_UniformLocs[EUniform::MsdfInnerGlowSoftness], 0.f );
                }
                else 
                {
                    glUseProgram( m_GeomProgram );
                    glUniformMatrix4fv( m_UniformLocs[EUniform::GeomPVM], 1, GL_FALSE, pvm );
                    glUniform1i( m_UniformLocs[EUniform::GeomTexture], 0 );
                    glUniform1i( m_UniformLocs[EUniform::GeomUseTexture], glTex != 0 ? GL_TRUE : GL_FALSE );
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

        GLuint m_VAO{ 0 }, m_VBO{ 0 }, m_IBO{ 0 };

        GLuint m_GeomProgram{ 0 };
        GLuint m_MSDFProgram{ 0 };

        // Uniform locations for all shader programs.
        enum EUniform : int
        {
            GeomPVM = 0,
            GeomTexture,
            GeomUseTexture,

            MsdfPVM,
            MsdfAtlas,
            MsdfPxRange,
            MsdfScale,
            MsdfTextureSize,

            MsdfFillColor,
            MsdfFillSoftness,
            MsdfFillThreshold,

            MsdfOutlineColor,
            MsdfOutlineWidth,
            MsdfOutlineSoftness,

            MsdfShadowEnable,
            MsdfShadowColor,
            MsdfShadowOffset,
            MsdfShadowSoftness,
            MsdfShadowSpread,

            MsdfGlowEnable,
            MsdfGlowColor,
            MsdfGlowSpread,
            MsdfGlowPower,

            MsdfInnerGlowEnable,
            MsdfInnerGlowColor,
            MsdfInnerGlowRange,
            MsdfInnerGlowSoftness,

            UniformCount
        };

        GLint m_UniformLocs[UniformCount]{};

		Mat3f m_Projection{};
        i32   m_ViewportWidth { 800 };
        i32   m_ViewportHeight{ 600 };
    };

} // namespace RatUI::OpenGL