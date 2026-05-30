#pragma once
#include "Texture.h"
#include "../Text/GlyphAtlas.h"

namespace RatUI
{
    /**
     * @brief Represents a single vertex in the rendering pipeline, containing position, color, and texture coordinates.
     */
    struct TextVertex
    {
        Vec2<Pixel> Position;
        f32         Opacity;  ///< The opacity of the vertex, used for fading effects. Range [0, 1].
        Vec2f       UV;
    };

    /**
     * @brief Represents a vertex used for Signed Distance Field (SDF) rendering, containing information about the position, color, border, and shape of the vertex.
     * For rectangles:
     *  - HalfSize can be any positive value. (E.g. for a 100x50 rect, HalfSize would be (50, 25))
     *  - CornerRadius: 0 = rectangle, >0 = rounded rectangle.
     * For circles:
     *  - HalfSize = (radius, radius)
     *  - CornerRadius = radius
     * 
     * @note This is pretty big for a vertex, but it is okay since there are only a few per shape (e.g. 4 for rectangles and circles).
     */
    struct SDFVertex
    {
        Vec2<Pixel> Position;  ///< The position of the vertex in screen space, used for rasterization.
        Vec2<Pixel> LocalPos;  ///< The position of the vertex relative to the center of the shape, used for SDF calculations.
        Vec2f       UV;        ///< The texture coordinates for sampling an optional texture, used for textured shapes.

        Color       FillColor; ///< The color used for filling the shape. Default is white.

        Color       BorderColor;     ///< The color used for the border of the shape.
        Pixel       BorderThickness; ///< 0 = no border

        Vec2<Pixel> HalfSize;        ///< Half of the width and height of the shape, used for SDF calculations. For rectangles, this is half the size of the rect. For circles, this is the radius in both dimensions.
        Pixel       CornerRadius;    ///< 0 = rect

		f32         Softness = 0.5;  ///< Edge anti-alias softness, in SDF units. 
                                     ///< Higher values create softer edges but may cause more blurring. Typically in the range of 0.0 to 0.5.
    };

    /**
     * @brief Represents the draw data for rendering SDF shapes.
     */
    struct SDFDrawData
    {
        TextureHandle Texture{};

        /** 
         * @brief Determines if this SDFDrawData can be flattened with another, 
         * meaning they can be drawn together in the same batch without causing visual artifacts.
         * This is true if all properties that affect the visual output are equal between the two draw data.
         */
        bool CanFlattenWith( const SDFDrawData& a_Other ) const
        {
            return Texture == a_Other.Texture;
        }
    };

    /**
     * @brief Represents the draw data for rendering MSDF text.
     */
    struct MSDFTextDrawData
    {
        TextureHandle FontAtlas{}; ///< The texture handle of the font atlas to use for rendering the text.

        f32 PixelRange{ c_MsdfPxRange };
        f32 Scale{ 1.f };

        bool OutlineEnable : 1 = false;
        bool ShadowEnable  : 1 = false;
        bool GlowEnable    : 1 = false;
        bool InnerGlowEnable : 1 = false;

        // - Fill properties

        Color FillColor    { Colors::White }; ///< The color used for filling the text glyphs. Default is white.
        f32   FillSoftness { 0.5f };          ///< Edge anti-alias softness for the fill, in SDF units [0, 0.5].
        f32   FillThreshold{ 0.5f };          ///< The threshold for determining the filled area of the text, in SDF units [0, 1], typically 0.5.

        // - Outline properties

        Color OutlineColor   { Colors::Transparent }; ///< The color used for the text outline. Default is white.
        f32   OutlineWidth   { 0.f };                 ///< The width of the text outline, in SDF units [0, 0.5], typically 0.05-0.2.
        f32   OutlineSoftness{ 0.f };                 ///< Edge anti-alias softness for the outline, in SDF units [0, 0.5].

        // - Shadow properties

        Color ShadowColor   { Colors::Transparent }; ///< The color used for the text shadow. Default is black.
        Vec2f ShadowOffsetUV{ 0.f, 0.f }; ///< Precomputed UV offset for drop shadow (atlas UV space).
        f32   ShadowSoftness{ 0.f };      ///< Edge anti-alias softness for the shadow, in SDF units [0, 0.5], typically 0.1-0.4.
        f32   ShadowSpread  { 0.f };      ///< The expansion of the shadow's SDF threshold, in SDF units [0, 0.5], typically 0.05-0.2.

        // - Glow properties

        Color GlowColor { Colors::Transparent }; ///< The color used for the text glow. Default is white.
        f32   GlowSpread{ 0.f };                 ///< How far glow extends beyond outline (0.0-0.5).
        f32   GlowPower { 0.0f };                ///< The falloff curve of the glow. Higher values create a tighter and brighter core, 
                                                 ///< while lower values create a softer glow. Typically in the range of 1.0 to 4.0.

        static MSDFTextDrawData From( TextureHandle a_FontAtlas, const TextRenderStyle& a_Style, f32 a_MSDFScale );

        /** 
         * @brief Determines if this MSDFTextDrawData can be flattened with another, 
         * meaning they can be drawn together in the same batch without causing visual artifacts.
         * This is true if all properties that affect the visual output are equal between the two draw 
         */
        bool CanFlattenWith( const MSDFTextDrawData& a_Other ) const;
    };

    /**
     * @brief Represents a batch of draw calls that can be executed together. 
     * Each batch contains information about the clipping rectangle, transformation, texture, 
     * and the range of vertices and indices to use for drawing.
     */
    struct DrawBatch
    {
        Optional<Rectu16> ClipRect;
        Mat3f             Transform;
        u32               VertexByteOffset{ 0 }; ///< Byte offset into the shared vertex buffer where this batch's vertices start.
        u32               IndexOffset{ 0 };
        u32               IndexCount{ 0 };

        Variant<
            SDFDrawData, 
            MSDFTextDrawData> Data;

        /** @brief Checks if this batch can be flattened with another batch. */
        bool CanFlattenWith( const DrawBatch& a_Other ) const;
    };

    // TODO: Theres still a lot of optimization I can do here and clean up the api.
    // Maybe have fixed size bump buffers?

    /**
     * @brief A utility class for batching draw calls together. 
     * It allows for efficient rendering by minimizing state changes and draw calls on the backend.
     */
    class DrawBatcher
    {
    public:
		Span<const byte>      GetVertices() const noexcept { return m_Vertices; }
		Span<const u16>       GetIndices()  const noexcept { return m_Indices; }
		Span<const DrawBatch> GetBatches()  const noexcept { return m_Batches; }

        void Clear();
        DrawBatch& EnsureSDFBatch( const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, TextureHandle a_Texture );
        DrawBatch& EnsureMSDFTextBatch( const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, const MSDFTextDrawData& a_Data );

        void EmitRect( Rect<Pixel> a_Rect,
                       Color a_FillColor,
                       Pixel a_BorderThickness = 0_px,
                       Color a_BorderColor = Colors::Transparent,
                       Vec4<Pixel> a_Rounding = {} );

        void EmitSlicedRect( Rect<Pixel> a_Rect, 
                             NineSlice a_NineSlice, 
                             Color a_Tint = Colors::White );

        void EmitText(
            const ShapedText&      a_Text,
            const TextRenderStyle& a_Style,
            Rect<Pixel>            a_LayoutRect,
            GlyphAtlas&            a_Atlas,
            f32                    a_DpiScale );

    protected:
        Array<byte>      m_Vertices;
        Array<u16>       m_Indices;
        Array<DrawBatch> m_Batches;

        void TryFlatten();

        template<typename DrawDataT>
        DrawBatch& EnsureBatch( const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, const DrawDataT& a_Data )
        {
            DrawBatch newBatch{
                .ClipRect         = a_ClipRect,
                .Transform        = a_Transform,
                .VertexByteOffset = static_cast<u32>( Size( m_Vertices ) ),
                .IndexOffset      = static_cast<u32>( Size( m_Indices ) ),
                .IndexCount       = 0,
                .Data             = a_Data
            };

            if ( Empty( m_Batches ) || !Back( m_Batches ).CanFlattenWith( newBatch ) )
                EmplaceBack( m_Batches, newBatch );

            return Back( m_Batches );
        }

        template<typename VertexT>
        Span<VertexT> ReserveVertices( u32 a_Count )
        {
            const u32 byteOffset = static_cast<u32>( Size( m_Vertices ) );
            Resize( m_Vertices, byteOffset + a_Count * sizeof( VertexT ) );
            return Span<VertexT>{ 
                reinterpret_cast<VertexT*>( Data( m_Vertices ) + byteOffset ), 
                a_Count };
        }

        Span<u16> ReserveIndices( u32 a_Count );

        void AddIndicesToCurrentBatch( u32 a_Count );
    };

} // namespace RatUI
