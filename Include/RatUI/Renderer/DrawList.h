#pragma once
#include "../Core.h"
#include "DrawBatcher.h"
#include "RenderTransform.h"
#include "IRenderer.h"

#include <map> // TODO: Replace with RatUI::Map

namespace RatUI
{
    /**
     * @brief A DrawList is a collection of draw commands that can be recorded and then executed by the renderer.
     * It also maintains a stack of clipping rectangles and transformation matrices, allowing for hierarchical transformations and clipping.
     */
    class DrawList
    {
    public:
        struct RectStyle
        {
            Color          FillColor{ Colors::Transparent };
            Color          BorderColor{ Colors::Transparent };
            Unit           BorderThickness{ 0_u };
            CornerRadius   Radius{ CornerRadius::None() };
            TextureView    Texture{};
        };

        struct SlicedRectStyle
        {
            TextureView Texture;
            NineSlice   Slice;
            Color       Tint{ Colors::White };
        };
    
        struct CircleStyle
        {
            Color       FillColor{ Colors::Transparent };
            Color       BorderColor{ Colors::Transparent };
            Unit        BorderThickness{ 0_u };
            TextureView Texture{};
        };

		DrawList( GlyphAtlas& a_Atlas, f32 a_DPIScale = 1.f )
            : m_Atlas( a_Atlas )
            , m_DPIScale( a_DPIScale )
        {}

        // ========================
        // DPI Scaling
        // ========================

        void SetDPIScale( f32 a_DPIScale ) { m_DPIScale = a_DPIScale; }
        f32  GetDPIScale() const { return m_DPIScale; }

        // ========================
        // Layering
        // ========================

        void SetDrawLayer( i32 a_Layer ) { m_CurrentLayer = a_Layer; }
        i32  GetDrawLayer() const        { return m_CurrentLayer; }

        // ========================
        // Transform Stack
        // ========================

        const Mat3<Unit>& GetCurrentTransform() const
        {
            return m_TransformStackSize == 0 ? c_Identity<Mat3<Unit>> 
                                             : m_TransformStack[m_TransformStackSize - 1];
        }

        DrawList& PushTransform( const Mat3<Unit>& a_Transform )
        {
            RATUI_ASSERT( m_TransformStackSize < Size(m_TransformStack), "Exceeded maximum transform stack depth." );
            const Mat3<Unit> transform = GetCurrentTransform() * a_Transform;
            m_TransformStack[m_TransformStackSize++] = transform;
            return *this;
        }

        DrawList& PopTransform()
        {
            RATUI_USER_ASSERT( m_TransformStackSize > 0, "PopTransform called too many times." );
            --m_TransformStackSize;
            return *this;
        }

        // ========================
        // Clip Stack
        // ========================

        Optional<Rect<Unit>> GetCurrentClip() const
        {
            return m_ClipStackSize == 0 ? NullOpt : Optional<Rect<Unit>>{ m_ClipStack[m_ClipStackSize - 1] };
        }

        DrawList& PushClipRect( Rect<Unit> a_Rect )
        {
            RATUI_ASSERT( m_ClipStackSize < Size(m_ClipStack), "Exceeded maximum clip stack depth." );
            if ( Optional<Rect<Unit>> currentClip = GetCurrentClip() )
                a_Rect = a_Rect.Intersection( *currentClip );

            m_ClipStack[m_ClipStackSize++] = a_Rect;
            return *this;
        }

        DrawList& PopClipRect()
        {
            RATUI_USER_ASSERT( m_ClipStackSize > 0, "PopClipRect called too many times." );
            --m_ClipStackSize;
            return *this;
        }

        // ========================
        // Drawing
        // ========================

        /**
		 * @brief Adds a rectangle to the draw list with the specified style. 
         * The rectangle will be transformed and clipped according to the current transform and clip stacks.
         */
        DrawList& AddRect( Rect<Unit> a_Rect, RectStyle a_Style )
        {
            // Only need to query the renderer for the full texture size when the view references
            // a sub-region; a default (infinite) Region always maps to the full [0, 1] UV rect.
            Rect<f32> uvRect{ Vec2f{ 0.f, 0.f }, Vec2f{ 1.f, 1.f } };
            if ( a_Style.Texture.Handle && !a_Style.Texture.Region.IsInfinite() )
            {
                auto texInfo = m_Atlas.GetRenderer().QueryTextureInfo( a_Style.Texture.Handle );
                if ( !texInfo )
                    return *this;

                uvRect = ComputeUVRect( a_Style.Texture.Region, texInfo->Size );
            }

            DrawBatcher& batcher = GetCurrentBatcher();
            batcher.EnsureSDFBatch( GetPixelClipRect(), GetPixelTransform(), std::move( a_Style.Texture ) );
            batcher.EmitRect( ToPixelRect( a_Rect ), 
                a_Style.FillColor, 
                ToPixel( a_Style.BorderThickness, m_DPIScale ), 
                a_Style.BorderColor, 
                ToPixelRadius( a_Style.Radius ),
                uvRect );
            return *this;
        }

		DrawList& AddSlicedRect( Rect<Unit> a_Rect, const SlicedRectStyle& a_Style )
		{
            // TODO: Dodgy way of getting the renderer for the texture.
            // There should be a correct way to get the associated renderer for the texture or at least its size.
            auto texInfo = m_Atlas.GetRenderer().QueryTextureInfo( a_Style.Texture.Handle );
            if ( !texInfo )
            {
                return *this;
            }

			Vec2u sliceSize = texInfo->Size;
			if ( !a_Style.Texture.Region.IsInfinite() )
			{
                sliceSize = {
                    static_cast<u32>( a_Style.Texture.Region.Size[0] ),
                    static_cast<u32>( a_Style.Texture.Region.Size[1] )
                };
			}

            const Rect<f32> uvRect = ComputeUVRect( a_Style.Texture.Region, texInfo->Size );

            DrawBatcher& batcher = GetCurrentBatcher();
            batcher.EnsureSDFBatch( GetPixelClipRect(), GetPixelTransform(), a_Style.Texture );
            batcher.EmitSlicedRect( ToPixelRect( a_Rect ),
                a_Style.Slice,
				sliceSize,
                a_Style.Tint,
                uvRect );

			return *this;
		}
        
        /**
		 * @brief Adds a circle to the draw list with the specified style.
		 * @note This is the same as calling AddRect with a square rect and uniform corner rounding equal to the radius, 
         *       but is provided as a convenience method.
         */
        DrawList& AddCircle( Vec2<Unit> a_Center, Unit a_Radius, CircleStyle a_Style )
        {
            // A circle is a rounded rect whose corner radius equals its half-size.
            const Rect<Unit> rect{
                a_Center - Vec2<Unit>{ a_Radius, a_Radius },
                Vec2<Unit>{ a_Radius * 2.f, a_Radius * 2.f }
            };

            return AddRect( rect, RectStyle{
                .FillColor = a_Style.FillColor,
                .BorderColor = a_Style.BorderColor,
                .BorderThickness = a_Style.BorderThickness,
                .Radius = CornerRadius::All( a_Radius ),
                .Texture = std::move( a_Style.Texture )
            } );
        }

        /**
		 * @brief Adds shaped text to the draw list with the specified style.
		 * The text will be transformed and clipped according to the current transform and clip stacks. 
         * The font size in the ShapedText is in units, but will be scaled to pixels based on the atlas configuration and current DPI scale.
         * @param a_Shaped The shaped text to render, which contains the sequence of glyphs and layout information.
         * @param a_Style The rendering style for the text, including fill color, outline, shadow, etc.
         * @param a_Rect The layout rectangle in which to render the text. The text will be clipped to this rectangle.
         * @param a_MaxGlyphs The maximum number of glyphs to render. Useful for effects in games where you want to reveal text gradually. Default is unlimited.
         * @return A reference to the DrawList, allowing for method chaining.
         */
        DrawList& AddText( 
            const ShapedText& a_Shaped, 
            const TextRenderStyle& a_Style, 
            Rect<Unit> a_Rect, 
            u32 a_MaxGlyphs = Limits<u32>::max() )
		{
			const f32   baseSize   = m_Atlas.GetConfig().BaseSize.ToFloat();
            const Pixel fontSizePx = ToPixel( a_Shaped.FontSize, m_DPIScale );
            const f32   msdfScale  = baseSize > 0.f ? fontSizePx.ToFloat() / baseSize : 1.f;

			DrawBatcher& batcher = GetCurrentBatcher();
            batcher.EnsureMSDFTextBatch( GetPixelClipRect(), GetPixelTransform(), MSDFTextDrawData::From( m_Atlas.GetTexture(), a_Style, msdfScale));
			batcher.EmitText( a_Shaped, a_Style, ToPixelRect( a_Rect ), m_Atlas, m_DPIScale, a_MaxGlyphs );
            return *this;
        }

        /**
         * @brief Flushes the draw list by executing all recorded draw batches on the given renderer. 
         * @note This does not clear the draw list, allowing for multiple flushes if needed. Call Clear() to reset the draw list state.
         */
        void Flush( IRenderer& a_Renderer )
        {
			// Note: Layers are drawn in ascending order, so lower layer numbers will appear behind higher layer numbers.
            for ( auto& [_, batcher] : m_Batchers )
                a_Renderer.Execute( batcher );
        }

        /**
         * @brief Clears the draw list, resetting all state including clip and transform stacks, current layer, and recorded draw batches.
         * This should be called at the beginning of each frame before recording new draw commands.
         */
        void Clear()
        {
            m_ClipStackSize = 0;
            m_TransformStackSize = 0;
			m_CurrentLayer = 0;

            // TODO: Still better optimisations we can do here.
            // e.g. If theres a tool tip popup, it may be drawn often but not every frame, currently 
            // it would cause a new batcher to be created and destroyed every frame which is not ideal. 
            // Maybe have a pool of batchers or something.

			// Clear all batches. 
			// If a batcher has no batches, we can remove it from the map to save memory, 
            // since we expect the number of active layers to be small and not all layers to be used every frame.
			for ( auto it = m_Batchers.begin(); it != m_Batchers.end(); )
            {
				auto& batcher = it->second; // TODO: Need Container util here

                if ( Empty( batcher.GetBatches() ) )
                {
                    it = m_Batchers.erase( it );
                    continue;
                }

                batcher.Clear();
                ++it;
            }
        }

        // ========================
        // Debug
        // ========================

        bool IsDebugEnabled() const { return m_DebugEnabled; }

        void SetDebugEnabled( bool a_Enabled ) { m_DebugEnabled = a_Enabled; }

    protected:
        static constexpr size  c_MaxStackDepth = 64; 

        GlyphAtlas& m_Atlas;

        FixedArray<Rect<Unit>, c_MaxStackDepth> m_ClipStack;
        FixedArray<Mat3<Unit>, c_MaxStackDepth> m_TransformStack;
        size m_ClipStackSize{ 0 };
        size m_TransformStackSize{ 0 };

        f32 m_DPIScale{ 1.f };
        i32 m_CurrentLayer{ 0 };
        std::map<i32, DrawBatcher> m_Batchers; // TODO: Add RatUI::Map

        bool m_DebugEnabled{ false };

        DrawBatcher& GetBatcherForLayer( i32 a_Layer ) { return m_Batchers[a_Layer]; }
        DrawBatcher& GetCurrentBatcher() { return GetBatcherForLayer( m_CurrentLayer ); }

        // TODO: All these conversion functions are gross and shouldnt be here

        Vec2<Pixel> ToPixelVec2( const Vec2<Unit>& a_Vec ) const
        {
            return Vec2<Pixel>{
                ToPixel( a_Vec[ 0 ], m_DPIScale ),
                ToPixel( a_Vec[ 1 ], m_DPIScale )
            };
        }

        Rect<Pixel> ToPixelRect( const Rect<Unit>& a_Rect ) const
        {
            return Rect<Pixel>{
                ToPixelVec2( a_Rect.Origin ),
                ToPixelVec2( a_Rect.Size )
            };
        }

        Vec4<Pixel> ToPixelRadius( const CornerRadius& a_Radius ) const
        {
            return Vec4<Pixel>{
                ToPixel( a_Radius.TL, m_DPIScale ),
                ToPixel( a_Radius.TR, m_DPIScale ),
                ToPixel( a_Radius.BL, m_DPIScale ),
                ToPixel( a_Radius.BR, m_DPIScale )
            };
        }

        // TODO: We call these alot, should cache them

        Mat3f GetPixelTransform() const 
        {
            if ( m_TransformStackSize == 0 )
                return c_Identity<Mat3f>;

            const Mat3<Unit>& currentTransform = m_TransformStack[m_TransformStackSize - 1];
            Mat3f pixelTransform;
            for ( size i = 0; i < 3; ++i )
                for ( size j = 0; j < 3; ++j )
                    pixelTransform[i][j] = ToPixel( currentTransform[i][j], m_DPIScale ).ToFloat();

            return pixelTransform;
        }

        Optional<Rectu16> GetPixelClipRect() const
        {
            if ( m_ClipStackSize == 0 )
                return NullOpt;

            const auto toU16 = +[]( Pixel a_Pixel ) -> u16
            {
                return static_cast<u16>( std::lround( a_Pixel.ToFloat() ) );
            };

			Rect<Pixel> pixelRect = ToPixelRect( m_ClipStack[m_ClipStackSize - 1 ] );
            return Rectu16{
                Vec2<u16>{ toU16( pixelRect.Left()   ), toU16( pixelRect.Top()    ) },
                Vec2<u16>{ toU16( pixelRect.Width()  ), toU16( pixelRect.Height() ) }
			};
        }
    };

} // namespace RatUI
