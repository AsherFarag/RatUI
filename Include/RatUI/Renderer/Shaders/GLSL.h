#pragma once

namespace RatUI::GLSL
{
    enum ESDFUniform
    {
        ESDFUniform_PVM = 0,
		ESDFUniform_Texture,

        ESDFUniform_UniformCount
    };

    inline constexpr const char* c_SDFUniformNames[] = {
        "u_PVM",
		"u_Texture"
    };

    enum ETextUniform
    {
        ETextUniform_PVM = 0,
        ETextUniform_Atlas,
        ETextUniform_PxRange,
        ETextUniform_Scale,

        ETextUniform_FillColor,
        ETextUniform_FillSoftness,
        ETextUniform_FillThreshold,

        ETextUniform_OutlineColor,
        ETextUniform_OutlineWidth,
        ETextUniform_OutlineSoftness,

        ETextUniform_ShadowEnable,
        ETextUniform_ShadowColor,
        ETextUniform_ShadowOffset,
        ETextUniform_ShadowSoftness,
        ETextUniform_ShadowSpread,

        ETextUniform_GlowEnable,
        ETextUniform_GlowColor,
        ETextUniform_GlowSpread,
        ETextUniform_GlowPower,

        ETextUniform_InnerGlowEnable,
        ETextUniform_InnerGlowColor,
        ETextUniform_InnerGlowRange,
        ETextUniform_InnerGlowSoftness,

        ETextUniform_UniformCount
    };

    inline constexpr const char* c_TextUniformNames[] = {
        "u_PVM",
        "u_Atlas",
        "u_PxRange",
        "u_Scale",
        "u_FillColor",
        "u_FillSoftness",
        "u_FillThreshold",
        "u_OutlineColor",
        "u_OutlineWidth",
        "u_OutlineSoftness",
        "u_ShadowEnable",
        "u_ShadowColor",
        "u_ShadowOffset",
        "u_ShadowSoftness",
        "u_ShadowSpread",
        "u_GlowEnable",
        "u_GlowColor",
        "u_GlowSpread",
        "u_GlowPower",
        "u_InnerGlowEnable",
        "u_InnerGlowColor",
        "u_InnerGlowRange",
        "u_InnerGlowSoftness"
    };

    // -----------------------------------------------------------------
    // SDF shape shaders
    // -----------------------------------------------------------------

    /**
     * @brief Vertex shader for SDF shapes (SDFVertex layout).
     */
    inline constexpr const char* c_SDFVertSrc = R"(
    #version 330 core
    layout(location = 0) in vec2  a_Pos;
    layout(location = 1) in vec2  a_LocalPos;
    layout(location = 2) in vec2  a_UV;
    layout(location = 3) in vec4  a_FillColor;
    layout(location = 4) in vec4  a_BorderColor;
    layout(location = 5) in float a_BorderThickness;
    layout(location = 6) in vec2  a_HalfSize;
    layout(location = 7) in float a_CornerRadius;
    layout(location = 8) in float a_Softness;

    uniform mat4 u_PVM;

    out vec2  v_LocalPos;
    out vec2  v_UV;
    out vec4  v_FillColor;
    out vec4  v_BorderColor;
    out float v_BorderThickness;
    out vec2  v_HalfSize;
    out float v_CornerRadius;
    out float v_Softness;

    void main()
    {
        gl_Position       = u_PVM * vec4(a_Pos, 0.0, 1.0);
        v_LocalPos        = a_LocalPos;
        v_UV              = a_UV;
        v_FillColor       = a_FillColor;
        v_BorderColor     = a_BorderColor;
        v_BorderThickness = a_BorderThickness;
        v_HalfSize        = a_HalfSize;
        v_CornerRadius    = a_CornerRadius;
        v_Softness        = a_Softness;
    }
    )";

    /**
     * @brief Fragment shader for SDF shapes.
     *
     * Implements a rounded-rectangle / circle signed distance field.
     * Fill and border are composited using premultiplied alpha so that
     * the border ring sits cleanly outside the filled area.
     *
     * SDF convention: d < 0 = inside, d > 0 = outside.
     */
    inline constexpr const char* c_SDFFragSrc = R"(
    #version 330 core

    in vec2  v_LocalPos;
    in vec2  v_UV;
    in vec4  v_FillColor;
    in vec4  v_BorderColor;
    in float v_BorderThickness;
    in vec2  v_HalfSize;
    in float v_CornerRadius;
    in float v_Softness;

    out vec4 FragColor;

    uniform sampler2D u_Texture;

    // Signed distance to a rounded rectangle.
    // a_LocalPos  = local position from the shape centre.
    // a_HalfSize  = half-extents of the fill region.
    // a_CornerRadius = corner radius (0 = sharp corners, == a_HalfSize.x == a_HalfSize.y -> circle).
    float SDRoundedBox(vec2 a_LocalPos, vec2 a_HalfSize, float a_CornerRadius)
    {
        vec2 b = a_HalfSize;
        float r = a_CornerRadius;
        b = max(b - vec2(r), vec2(0.0));
        vec2 q = abs(a_LocalPos) - b;
        return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
    }

    void main()
    {
        float d  = SDRoundedBox(v_LocalPos, v_HalfSize, v_CornerRadius);
        float aa = max(fwidth(d) * v_Softness, 0.5);

        // 1 inside the fill area, smooth edge at d=0.
        float fillMask = 1.0 - smoothstep(
            -aa - v_Softness,
             aa + v_Softness,
             d );

        // Border band: outside fill (d > 0) up to d == borderThickness.
        float borderMask = 0.0;
        if (v_BorderThickness > 0.0)
        {
            float outerMask = 1.0 - smoothstep(
                -aa - v_Softness, 
                 aa + v_Softness,
                 d - v_BorderThickness );

            borderMask = clamp(outerMask - fillMask, 0.0, 1.0);
        }

        vec4 tex = texture(u_Texture, v_UV);
        
        // Composite fill "over" border using premultiplied alpha.
        float bAlpha    = borderMask * v_BorderColor.a;
        float fAlpha    = fillMask   * v_FillColor.a;
        
        vec3 fillRGB    = v_FillColor.rgb * tex.rgb; // TEXTURE ONLY AFFECTS FILL
        
        vec3 borderPre  = v_BorderColor.rgb * bAlpha;
        vec3 fillPre    = fillRGB * fAlpha;
        
        float outA   = fAlpha + bAlpha * (1.0 - fAlpha);
        vec3  outRGB = fillPre + borderPre * (1.0 - fAlpha);

        if (outA < 0.004) discard;
        FragColor = vec4(outRGB / max(outA, 0.001), outA);
    }
    )";

    inline constexpr const char* c_TextVertSrc = R"(
    #version 330 core
    layout(location = 0) in vec2  a_Pos;
    layout(location = 1) in float a_Opacity;
    layout(location = 2) in vec2  a_UV;
    
    uniform mat4 u_PVM;
    
    out float v_Opacity;
    out vec2 v_UV;
    
    void main()
    {
        gl_Position = u_PVM * vec4(a_Pos, 0.0, 1.0);
        v_Opacity   = a_Opacity;
        v_UV        = a_UV;
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
    inline constexpr const char* c_TextFragSrc = R"(
    #version 330 core

    in vec2 v_UV;
    in float v_Opacity;
    out vec4 FragColor;

    // - Atlas
    uniform sampler2D u_Atlas;          // MTSDF atlas (RGBA, linear filtering)
    uniform float     u_PxRange;        // msdfgen pxrange (e.g. 4.0)
    uniform float     u_Scale;          // Glyph scale (from batch)

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
    float ScreenPxRange(vec2 a_UV ) 
    {
        vec2 unitRange = vec2(u_PxRange) / textureSize(u_Atlas, 0).xy;
        vec2 screenTexSize = vec2(1.0) / fwidth(a_UV);
        return max(0.5 * dot(unitRange, screenTexSize), 1.0);
    }

    //  Sample MTSDF at a given UV and return the signed distance value in [0,1].
    float SampleMTSDF(vec4 a_MTSDF)
    {
        float msdf = Median(a_MTSDF.r, a_MTSDF.g, a_MTSDF.b);
        float tsdf = a_MTSDF.a;
        float pxRange = ScreenPxRange(v_UV);
        float weight  = clamp(1.0 - (pxRange - 1.0) / 4.0, 0.0, 1.0);
        return mix(msdf, max(msdf, tsdf), weight);
    }

    // Helper: convert SDF distance to alpha with smoothstep edge.
    //  a_Softness is in user-facing pixel units. Adding 0.5 guarantees a minimum
    //  half-pixel AA transition even at softness=0, preventing hard aliasing.
    //  The combined value is then divided by pxRange to convert into SDF units.
    float SDFAlpha(float a_Dist, float a_Threshold, float a_Softness, float a_PxRange) 
    {
        float sdfSoft = (a_Softness + 0.5) / a_PxRange;
        return smoothstep(a_Threshold - sdfSoft, a_Threshold + sdfSoft, a_Dist);
    }

    //  Shadow: multi-tap Gaussian-approximated blur along the shadow offset direction.
    //  Blur radius is clamped to the SDF gradient band to prevent out-of-tile sampling.
    const int c_ShadowTaps = 5;

    float ShadowAlpha(vec2 a_UV, float a_Threshold, float a_Softness, float a_PxRange) 
    {
        // Precomputed Gaussian weights for 5 taps, normalized so their sum is 1.
        // WARNING: Update these if you change c_ShadowTaps!
        const float W[c_ShadowTaps] = float[](0.0625, 0.25, 0.375, 0.25, 0.0625);

        float offsetLen = length(u_ShadowOffset);
        vec2  blurDir   = offsetLen > 1e-5
                            ? (u_ShadowOffset / offsetLen)
                            : vec2(1.0, 0.0);

        // Radius in SDF units, clamped so taps stay within the SDF gradient band
        // and cannot wander into neighbouring atlas tiles or empty atlas space.
        vec2  atlasSize    = vec2(textureSize(u_Atlas, 0)); 
        float blurSDF      = clamp(a_Softness, 0.0, u_PxRange * 0.5);
        float blurRadiusUV = (blurSDF / u_PxRange) / min(atlasSize.x, atlasSize.y);

        float alpha = 0.0;
        for (int i = 0; i < c_ShadowTaps; i++)
        {
            float t     = float(i) / float(c_ShadowTaps - 1) - 0.5;
            vec2  tapUV = a_UV + blurDir * t * blurRadiusUV;
            float d     = texture(u_Atlas, tapUV).a;
            // Use the pxRange from the original UV — derivatives at offset UVs
            // are invalid inside a loop and would produce incorrect softness.
            alpha      += W[i] * SDFAlpha(d, a_Threshold, a_Softness, a_PxRange);
        }
        return clamp(alpha, 0.0, 1.0);
    }

    // Main:
    // Renders text with up to 4 layers of effects, composited in this order:
    //  1. Drop shadow (beneath everything)
    //  2. Outer glow (above shadow but beneath outline and fill, so it can glow both inside and outside the glyph edges)
    //  3. Outline (overrides glow and fill at edges)
    //  4. Fill (overrides glow at edges, but under the outline)
    void main() 
    {
        float pxRange = ScreenPxRange(v_UV);
        vec4  mtsdf   = texture(u_Atlas, v_UV);
        float dist    = SampleMTSDF(mtsdf);

        // TODO: Should maybe add a roundness factor that blends between MSDF and TSDF?
        float tsdf    = mtsdf.a; // Single channel SDF for soft effects like glow that require accurate distance values, stored in alpha channel of MTSDF atlas.

        vec4 color = vec4(0.0);

        // - 1. Drop Shadow
        //   Sampled first so it appears beneath all other effects, even when shadow and glow overlap.
        if (u_ShadowColor.a > 0.0)
        {
            vec2  shadowUV     = v_UV - u_ShadowOffset;
            float shadowThresh = 0.5 - u_ShadowSpread;
            float sAlpha       = ShadowAlpha(shadowUV, shadowThresh, u_ShadowSoftness, pxRange);
            color = mix(color, u_ShadowColor, sAlpha * u_ShadowColor.a);
        }

        // - 2. Outer Glow
        //   Sampled before the outline so it appears beneath the outline, and can glow both inside and outside the glyph edges.
        if (u_GlowColor.a > 0.0 && u_GlowSpread > 0.0)
        {
            float innerEdge = 0.5 - u_OutlineWidth;
            float outerEdge = max(innerEdge - u_GlowSpread, 0.0);
            float bandWidth = innerEdge - outerEdge;

            if (bandWidth > 0.0)
            {
                // Remap dist to [0,1] within the band: 0 at outerEdge, 1 at innerEdge.
                float bandT     = clamp((tsdf - outerEdge) / bandWidth, 0.0, 1.0);
                float glowAlpha = pow(bandT, u_GlowPower);
                
                color = mix(color, u_GlowColor, glowAlpha * u_GlowColor.a);
            }
        }

        // - 3. Outline
        //   Overrides the glow and fill at the edges, so it appears crisp even with a soft glow.  Sampled after the glow so it can overlap the glow on the inside of the glyph if u_OutlineWidth < u_GlowSpread.
        if (u_OutlineColor.a > 0.0)
        {
            float outlineThresh = 0.5 - u_OutlineWidth;
            float outlineAlpha  = SDFAlpha(dist, outlineThresh, u_OutlineSoftness, pxRange);
            color = mix(color, u_OutlineColor, outlineAlpha * u_OutlineColor.a);
        }

        // - 4. Fill
        float fillAlpha = SDFAlpha(dist, u_FillThreshold, u_FillSoftness, pxRange);
        color = mix(color, u_FillColor, fillAlpha * u_FillColor.a);

        color.a *= v_Opacity; // Apply vertex alpha at the end so it affects all layers.
        FragColor = color;
    }
    )";
} // namespace RatUI::GLSL