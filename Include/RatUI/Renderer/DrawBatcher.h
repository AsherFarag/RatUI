#pragma once
#include "Texture.h"
#include "../Text/GlyphAtlas.h"
#include "../Text/TextShaping.h"

namespace RatUI
{
    /** @brief Vertex for both text pipelines (MTSDF and bitmap). */
    struct TextVertex
    {
        Vec2<Pixel> Position;
        Vec2f       UV;
        Color       Tint;
        f32         Weight;  ///< MTSDF: SDF threshold offset (synthetic bold). Bitmap: 1 = alpha-only silhouette (shadow / outline).
        f32         Opacity; ///< Fade multiplier for every layer.
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
        TextureView Texture{};

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

    /** @brief MTSDF text batch. Colour and weight are per vertex, the effects are per batch. */
    struct MSDFTextDrawData
    {
        TextureHandle FontAtlas{};
        f32           PixelRange{ 16.f };

        bool OutlineEnable : 1 = false;
        bool ShadowEnable  : 1 = false;
        bool GlowEnable    : 1 = false;

        // - Fill

        f32   FillSoftness { 0.5f };
        f32   FillThreshold{ 0.5f };

        // - Outline

        Color OutlineColor   { Colors::Transparent };
        f32   OutlineWidth   { 0.f };
        f32   OutlineSoftness{ 0.f };

        // - Shadow

        Color ShadowColor   { Colors::Transparent };
        Vec2f ShadowOffsetUV{ 0.f, 0.f };
        f32   ShadowSoftness{ 0.f };
        f32   ShadowSpread  { 0.f };

        // - Glow

        Color GlowColor { Colors::Transparent };
        f32   GlowSpread{ 0.f };
        f32   GlowPower { 0.0f };

        static MSDFTextDrawData From( TextureHandle a_Page, const TextRenderStyle& a_Style, f32 a_PxRange, u16 a_PageSize );

        bool CanFlattenWith( const MSDFTextDrawData& a_Other ) const;
    };

    /** @brief Raster / Pixel text batch: textured, per-vertex coloured quads. The page's sampler picks the filtering. */
    struct BitmapTextDrawData
    {
        TextureHandle Page{};

        bool CanFlattenWith( const BitmapTextDrawData& a_Other ) const { return Page == a_Other.Page; }
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
            MSDFTextDrawData,
            BitmapTextDrawData> Data;

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
        DrawBatch& EnsureSDFBatch( const Optional<Rectu16>& a_ClipRect, const Mat3f& a_Transform, TextureView a_Texture );
        void EmitRect( Rect<Pixel> a_Rect,
                       Color a_FillColor,
                       Pixel a_BorderThickness = 0_px,
                       Color a_BorderColor = Colors::Transparent,
                       Vec4<Pixel> a_Rounding = {},
                       Rect<f32> a_UVRect = Rect<f32>{ Vec2f{ 0.f, 0.f }, Vec2f{ 1.f, 1.f } } );

        void EmitSlicedRect( Rect<Pixel> a_Rect, 
                             NineSlice a_NineSlice,
                             Vec2u a_SliceSize,
                             Color a_Tint = Colors::White,
                             Rect<f32> a_UVRect = Rect<f32>{ Vec2f{ 0.f, 0.f }, Vec2f{ 1.f, 1.f } } );

        /** @brief Draws shaped text in @p a_LayoutRect, stopping after @p a_MaxGlyphs glyphs. */
        void EmitText(
            const ShapedText&        a_Text,
            const TextRenderStyle&   a_Style,
            Rect<Pixel>              a_LayoutRect,
            GlyphAtlas&              a_Atlas,
            f32                      a_DpiScale,
            const Optional<Rectu16>& a_ClipRect,
            const Mat3f&             a_Transform,
            u32                      a_MaxGlyphs = Limits<u32>::max() );

    protected:
        Array<byte>      m_Vertices;
        Array<u16>       m_Indices;
        Array<DrawBatch> m_Batches;

        /** @brief A placed glyph / decoration quad. Collected first so bitmap effects can be drawn in passes. */
        struct TextQuad
        {
            const TextureHandle* Page{ nullptr };
            bool                 SDF{ false };
            f32                  X0, Y0, X1, Y1;
            f32                  U0, V0, U1, V1;
            f32                  BaselineY;       ///< Shear pivot for synthetic italic.
            f32                  Skew{ 0.f };     ///< Synthetic italic, MTSDF only.
            f32                  Weight{ 0.f };   ///< Synthetic bold, MTSDF only.
            Color                FillColor;
            i16                  ShadowDX{ 0 }, ShadowDY{ 0 };
            i16                  OutlineStep{ 0 };
            u8                   OutlineRadius{ 0 };
        };

        struct TextEmitParams
        {
            const TextRenderStyle* Style{ nullptr };
            GlyphAtlas*            Atlas{ nullptr };
            Optional<Rectu16>      ClipRect;
            Mat3f                  Transform;
            Rect<Pixel>            FadeRect{};
            bool                   FadeHorizontal{ false };
            bool                   FadeVertical{ false };
            f32                    FadePercent{ 0.f };
        };

        /** @brief Bitmap shadow / outline offsets for a run, in whole pixels. */
        struct BitmapEffects
        {
            i16 ShadowDX{ 0 }, ShadowDY{ 0 };
            i16 OutlineStep{ 0 };
            u8  OutlineRadius{ 0 };
        };

        Array<TextQuad> m_TextQuads;

        BitmapEffects ComputeBitmapEffects( const TextRenderStyle& a_Style, const IFontFace& a_Face, Unit a_Size, f32 a_DpiScale, const GlyphAtlasConfig& a_Config ) const;

        void PlaceGlyph( GlyphAtlas& a_Atlas, const IFontFace& a_Face, const ResolvedFace& a_Resolved, GlyphID a_Glyph,
                         Unit a_Size, f32 a_PenX, f32 a_BaselineY, f32 a_DpiScale, Color a_Color, const BitmapEffects& a_Effects );

        void PlaceDecoration( GlyphAtlas& a_Atlas, const IFontFace& a_Face, Unit a_Size, u8 a_Decoration,
                              f32 a_X0, f32 a_X1, f32 a_BaselineY, f32 a_DpiScale, Color a_Color, const BitmapEffects& a_Effects );

        /** @brief Turns m_TextQuads into vertices: bitmap shadows, then bitmap outlines, then fills. */
        void FlushTextQuads( const TextEmitParams& a_Params );

        /** @brief Continues the current batch in a new one, before it overflows u16 indices. */
        void SplitCurrentBatch();

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
