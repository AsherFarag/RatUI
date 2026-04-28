#pragma once
#include "../Core.h"
#include "DrawBatcher.h"
#include "RenderTransform.h"

namespace RatUI
{
    /**
     * @brief A DrawList is a collection of draw commands that can be recorded and then executed by the renderer.
     * It also maintains a stack of clipping rectangles and transformation matrices, allowing for hierarchical transformations and clipping.
     */
    class DrawList
    {
    public:
        DrawBatcher& Batcher;
		GlyphAtlas&  Atlas;

		DrawList( DrawBatcher& a_Batcher, GlyphAtlas& a_Atlas, f32 a_DPIScale = 1.f )
            : Batcher( a_Batcher )
            , Atlas( a_Atlas )
            , m_DPIScale( a_DPIScale )
        {}

        // ========================
        // Transform Stack
        // ========================

        const Mat3f& GetCurrentTransform() const
        {
            return m_TransformStackSize == 0 ? c_Identity<Mat3f> : m_TransformStack[m_TransformStackSize - 1];
        }

        DrawList& PushTransform( const Mat3f& a_Transform )
        {
            RATUI_ASSERT( m_TransformStackSize < Size(m_TransformStack), "Exceeded maximum transform stack depth." );
            const Mat3f transform = GetCurrentTransform() * a_Transform;
            m_TransformStack[m_TransformStackSize++] = transform;
            UpdateBatchState();
            return *this;
        }

        DrawList& PopTransform()
        {
            RATUI_USER_ASSERT( m_TransformStackSize > 0, "PopTransform called too many times." );
            --m_TransformStackSize;
            UpdateBatchState();
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
            UpdateBatchState();
            return *this;
        }

        DrawList& PopClipRect()
        {
            RATUI_USER_ASSERT( m_ClipStackSize > 0, "PopClipRect called too many times." );
            --m_ClipStackSize;
            UpdateBatchState();
            return *this;
        }

        // ========================
        // Drawing
        // ========================

        DrawList& AddRect( Coloru8 a_Color, const Rect<Unit>& a_Rect )
        {
            EnsureGeoBatch();
            Batcher.EmitRect( ToPixelRect( a_Rect ), a_Color );
            return *this;
        }

        DrawList& AddRect( Coloru8 a_Color, const Rect<Unit>& a_Rect, CornerRounding a_Rounding )
        {
            EnsureGeoBatch();
            Batcher.EmitRoundedRect( ToPixelRect( a_Rect ), a_Rounding, a_Color );
            return *this;
        }

        DrawList& AddRectBorder( Coloru8 a_Color, const Rect<Unit>& a_Rect, Unit a_Thickness = 1_u )
        {
            EnsureGeoBatch();
            Batcher.EmitRectBorder( ToPixelRect( a_Rect ), 0.f, a_Color, ToPixel( a_Thickness, m_DPIScale ).ToFloat() );
            return *this;
        }

        DrawList& AddRectBorder( Coloru8 a_Color, const Rect<Unit>& a_Rect, CornerRounding a_Rounding, Unit a_Thickness = 1_u )
        {
            EnsureGeoBatch();
            Batcher.EmitRoundedRectBorder( ToPixelRect( a_Rect ), a_Rounding, a_Color, ToPixel( a_Thickness, m_DPIScale ).ToFloat() );
            return *this;
        }

        DrawList& AddCircle( Coloru8 a_Color, Vec2<Unit> a_Center, Unit a_Radius )
        {
            EnsureGeoBatch();
            Batcher.EmitCircle( ToPixelVec2( a_Center ), ToPixel( a_Radius, m_DPIScale ).ToFloat(), a_Color );
            return *this;
        }

        DrawList& AddCircleBorder( Coloru8 a_Color, Vec2<Unit> a_Center, Unit a_Radius, Unit a_Thickness = 1_u )
        {
            EnsureGeoBatch();
            Batcher.EmitCircleBorder( ToPixelVec2( a_Center ), ToPixel( a_Radius, m_DPIScale ).ToFloat(), a_Color, ToPixel( a_Thickness, m_DPIScale ).ToFloat() );
            return *this;
        }

        DrawList& AddText( const ShapedText& a_Shaped, TextRenderStyle a_Style, Rect<Unit> a_Rect )
		{
			const f32   baseSize   = Atlas.GetConfig().BaseSize.ToFloat();
            const Pixel fontSizePx = ToPixel( a_Shaped.FontSize, m_DPIScale );
            const f32   msdfScale  = baseSize > 0.f ? fontSizePx.ToFloat() / baseSize : 1.f;

			EnsureMSDFBatch( msdfScale, a_Style );
			Batcher.EmitText( a_Shaped, a_Style, ToPixelRect( a_Rect ), Atlas, m_DPIScale );
            return *this;
        }

        void Finish()
        {
            FlushBatch();
        }

    private:
        static constexpr size  c_MaxStackDepth = 64; 
        FixedArray<Rect<Unit>, c_MaxStackDepth> m_ClipStack;
        FixedArray<Mat3f,      c_MaxStackDepth> m_TransformStack;
        size                 m_ClipStackSize{ 0 };
        size                 m_TransformStackSize{ 0 };

        TextureID            m_CurrentTexture{ TextureID::Null() };
        bool                 m_HasActiveBatch{ false };
        f32                  m_DPIScale{ 1.f };

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

        Optional<Rectu16> GetClipRect() const
        {
            if ( m_ClipStackSize == 0 )
                return NullOpt;

			Rect<Pixel> pixelRect = ToPixelRect( m_ClipStack[m_ClipStackSize - 1 ] );
            return Rectu16{
                Vec2<u16>{ static_cast<u16>( pixelRect.Left().ToFloat() ), static_cast<u16>( pixelRect.Top().ToFloat() ) },
                Vec2<u16>{ static_cast<u16>( pixelRect.Width().ToFloat() ), static_cast<u16>( pixelRect.Height().ToFloat() ) }
			};
        }

        void FlushBatch()
        {
            if ( m_HasActiveBatch )
            {
                Batcher.EndBatch();
                m_HasActiveBatch = false;
            }
        }

        bool IsBatchStateCompatible( const DrawBatch& a_Batch, EBatchType a_Type ) const
        {
            return a_Batch.Type   == a_Type &&
                a_Batch.ClipRect  == GetClipRect() &&
                a_Batch.Transform == GetCurrentTransform() &&
                a_Batch.Texture   == m_CurrentTexture;
        }

        void UpdateBatchState()
        {
            if ( !m_HasActiveBatch )
                return;

            DrawBatch& currentBatch = Back( Batcher.Batches );
            if ( IsBatchStateCompatible( currentBatch, currentBatch.Type ) )
            {
                return; // No change in batch state, no need to flush.
            }

            FlushBatch();
        }

        void EnsureGeoBatch( TextureID a_Texture = TextureID::Null() )
        {
            m_CurrentTexture = a_Texture;
            
            if ( m_HasActiveBatch && IsBatchStateCompatible( Back( Batcher.Batches ), EBatchType::Geometry ) )
                return; // Current batch is already compatible.

            FlushBatch();
            Batcher.BeginBatch( EBatchType::Geometry, GetClipRect(), GetCurrentTransform(), m_CurrentTexture);
            m_HasActiveBatch = true;
        }

        void EnsureMSDFBatch( f32 a_Scale, const TextRenderStyle& a_Style )
        {
            m_CurrentTexture = Atlas.GetTexture();

            if ( m_HasActiveBatch )
            {
                DrawBatch& b = Back( Batcher.Batches );

                if ( IsBatchStateCompatible( b, EBatchType::MSDF ) &&
                     b.MSDF.Scale == a_Scale )
                    return;
            }

            FlushBatch();
            DrawBatch& batch = Batcher.BeginBatch( EBatchType::MSDF, GetClipRect(), GetCurrentTransform(), m_CurrentTexture);

            batch.MSDF.PixelRange = c_MsdfPxRange;
            batch.MSDF.Scale = a_Scale;
            m_HasActiveBatch = true;
        }
    };

} // namespace RatUI
