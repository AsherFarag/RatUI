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
		GlyphAtlas& Atlas;

		DrawList( GlyphAtlas& a_Atlas, f32 a_DPIScale = 1.f )
            : Atlas( a_Atlas )
            , m_DPIScale( a_DPIScale )
        {}

        void SetDPIScale( f32 a_DPIScale ) { m_DPIScale = a_DPIScale; }
        f32  GetDPIScale() const { return m_DPIScale; }

        void Clear()
        {
            m_ClipStackSize = 0;
            m_TransformStackSize = 0;
			m_CurrentLayer = 0;
            for ( auto& [_, batcher] : m_Batchers )
                batcher.Clear();
        }

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

        DrawList& AddRect( Color a_Color, Rect<Unit> a_Rect, CornerRounding a_Rounding, Unit a_BorderThickness = 0_u, Color a_BorderColor = Colors::Transparent )
        {
            DrawBatcher& batcher = GetCurrentBatcher();
            batcher.EnsureSDFBatch( GetPixelClipRect(), GetPixelTransform(), TextureHandle::Null() );
            batcher.EmitRect( ToPixelRect( a_Rect ), a_Color, ToPixel( a_BorderThickness, m_DPIScale ).ToFloat(), a_BorderColor, ToPixelRounding( a_Rounding ) );
            return *this;
        }

        DrawList& AddImage( TextureHandle a_Texture, 
                            Rect<Unit> a_Rect, 
                            Color a_Tint = Colors::White, 
                            CornerRounding a_Rounding = CornerRounding::None(), 
                            Unit a_BorderThickness = 0_u, 
                            Color a_BorderColor = Colors::Transparent )
        {
            DrawBatcher& batcher = GetCurrentBatcher();
            batcher.EnsureSDFBatch( GetPixelClipRect(), GetPixelTransform(), std::move( a_Texture ) );
            batcher.EmitRect( ToPixelRect( a_Rect ), a_Tint, ToPixel( a_BorderThickness, m_DPIScale ).ToFloat(), a_BorderColor, ToPixelRounding( a_Rounding ) );
            return *this;
        }

        DrawList& AddRect( Color a_Color, const Rect<Unit>& a_Rect, Unit a_BorderThickness = 0_u, Color a_BorderColor = Colors::Transparent )
        {
            return AddRect( a_Color, a_Rect, CornerRounding::None(), a_BorderThickness, a_BorderColor );
        }

        DrawList& AddRectBorder( Color a_Color, const Rect<Unit>& a_Rect, Unit a_Thickness = 1_u )
        {
            return AddRect( Colors::Transparent, a_Rect, CornerRounding::None(), a_Thickness, a_Color );
        }

        DrawList& AddRectBorder( Color a_Color, const Rect<Unit>& a_Rect, CornerRounding a_Rounding, Unit a_Thickness = 1_u )
        {
            return AddRect( Colors::Transparent, a_Rect, a_Rounding, a_Thickness, a_Color );
        }

        DrawList& AddCircle( Color a_Color, Vec2<Unit> a_Center, Unit a_Radius, Unit a_BorderThickness = 0_u, Color a_BorderColor = Colors::Transparent )
        {
            DrawBatcher& batcher = GetCurrentBatcher();
            batcher.EnsureSDFBatch( GetPixelClipRect(), GetPixelTransform(), TextureHandle::Null() );
            batcher.EmitCircle( ToPixelVec2( a_Center ), ToPixel( a_Radius, m_DPIScale ), a_Color, ToPixel( a_BorderThickness, m_DPIScale ), a_BorderColor );
            return *this;
        }

        DrawList& AddCircleBorder( Color a_Color, Vec2<Unit> a_Center, Unit a_Radius, Unit a_Thickness = 1_u )
        {
            return AddCircle( Colors::Transparent, a_Center, a_Radius, a_Thickness, a_Color );
        }

        DrawList& AddText( const ShapedText& a_Shaped, const TextRenderStyle& a_Style, Rect<Unit> a_Rect )
		{
			const f32   baseSize   = Atlas.GetConfig().BaseSize.ToFloat();
            const Pixel fontSizePx = ToPixel( a_Shaped.FontSize, m_DPIScale );
            const f32   msdfScale  = baseSize > 0.f ? fontSizePx.ToFloat() / baseSize : 1.f;

			DrawBatcher& batcher = GetCurrentBatcher();
            batcher.EnsureMSDFTextBatch( GetPixelClipRect(), GetPixelTransform(), MSDFTextDrawData::From( Atlas.GetTexture(), a_Style, msdfScale));
			batcher.EmitText( a_Shaped, a_Style, ToPixelRect( a_Rect ), Atlas, m_DPIScale );
            return *this;
        }

        void Flush( IRenderer& a_Renderer )
        {
            for ( auto& [_, batcher] : m_Batchers )
                a_Renderer.Execute( batcher );
        }

    private:
        static constexpr size  c_MaxStackDepth = 64; 
        FixedArray<Rect<Unit>, c_MaxStackDepth> m_ClipStack;
        FixedArray<Mat3<Unit>, c_MaxStackDepth> m_TransformStack;
        size                                    m_ClipStackSize{ 0 };
        size                                    m_TransformStackSize{ 0 };

        f32 m_DPIScale{ 1.f };
        i32 m_CurrentLayer{ 0 };
        std::map<i32, DrawBatcher> m_Batchers; // TODO: Add RatUI::Map

        DrawBatcher& GetBatcherForLayer( i32 a_Layer ) { return m_Batchers[a_Layer]; }
        DrawBatcher& GetCurrentBatcher() { return GetBatcherForLayer( m_CurrentLayer ); }

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

        Vec4<Pixel> ToPixelRounding( const CornerRounding& a_Rounding ) const
        {
            return Vec4<Pixel>{
                ToPixel( a_Rounding.TopLeft, m_DPIScale ),
                ToPixel( a_Rounding.TopRight, m_DPIScale ),
                ToPixel( a_Rounding.BottomLeft, m_DPIScale ),
                ToPixel( a_Rounding.BottomRight, m_DPIScale )
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

			Rect<Pixel> pixelRect = ToPixelRect( m_ClipStack[m_ClipStackSize - 1 ] );
            return Rectu16{
                Vec2<u16>{ static_cast<u16>( pixelRect.Left().ToFloat() ), static_cast<u16>( pixelRect.Top().ToFloat() ) },
                Vec2<u16>{ static_cast<u16>( pixelRect.Width().ToFloat() ), static_cast<u16>( pixelRect.Height().ToFloat() ) }
			};
        }
    };

} // namespace RatUI
