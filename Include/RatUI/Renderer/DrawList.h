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

		DrawList( DrawBatcher& a_Batcher, GlyphAtlas& a_Atlas )
            : Batcher( a_Batcher )
            , Atlas( a_Atlas )
        {}

        void Clear()
        {
            ::RatUI::Clear( m_ClipStack );
            ::RatUI::Clear( m_TransformStack );

            m_CurrentClip.reset();
            m_CurrentTransform = c_Identity<Mat3f>;
            m_CurrentTexture = TextureID::Null();
            m_HasActiveBatch = false;
        }

        // ========================
        // Transform Stack
        // ========================

        const Mat3f& GetCurrentTransform() const
        {
            return Empty( m_TransformStack )
                ? c_Identity<Mat3f>
                : Back( m_TransformStack );
        }

        DrawList& PushTransform( const Mat3f& a_Transform )
        {
            PushBack( m_TransformStack, Empty( m_TransformStack ) ? a_Transform : Back( m_TransformStack ) * a_Transform );
            m_CurrentTransform = Back( m_TransformStack );
            UpdateBatchState();
            return *this;
        }

        DrawList& PopTransform()
        {
            RATUI_USER_ASSERT( !Empty( m_TransformStack ),
                "PopTransform called too many times." );

            PopBack( m_TransformStack );
            m_CurrentTransform = GetCurrentTransform();

            UpdateBatchState();
            return *this;
        }

        // ========================
        // Clip Stack
        // ========================

        DrawList& PushClipRect( Rectu16 a_Rect )
        {
            if ( !Empty( m_ClipStack ) )
                a_Rect = a_Rect.Intersection( Back( m_ClipStack ) );

            PushBack( m_ClipStack, a_Rect );
            m_CurrentClip = a_Rect;

            UpdateBatchState();
            return *this;
        }

        DrawList& PopClipRect()
        {
            RATUI_USER_ASSERT( !Empty( m_ClipStack ),
                "PopClipRect called too many times." );

            PopBack( m_ClipStack );

            if ( Empty( m_ClipStack ) ) m_CurrentClip.reset();
            else                        m_CurrentClip = Back( m_ClipStack );

            UpdateBatchState();
            return *this;
        }

        // ========================
        // Drawing
        // ========================

        DrawList& AddRect( Coloru8 a_Color, Rectf a_Rect )
        {
            EnsureGeoBatch();
            Batcher.EmitRect( a_Rect, a_Color );
            return *this;
        }

        DrawList& AddRect( Coloru8 a_Color, Rectf a_Rect, CornerRounding a_Rounding )
        {
            EnsureGeoBatch();
            Batcher.EmitRoundedRect( a_Rect, a_Rounding, a_Color );
            return *this;
        }

        DrawList& AddRectBorder( Coloru8 a_Color, Rectf a_Rect, f32 a_Thickness = 1.f )
        {
            EnsureGeoBatch();
            Batcher.EmitRectBorder( a_Rect, 0.f, a_Color, a_Thickness );
            return *this;
        }

        DrawList& AddRectBorder( Coloru8 a_Color, Rectf a_Rect, CornerRounding a_Rounding, f32 a_Thickness = 1.f )
        {
            EnsureGeoBatch();
            Batcher.EmitRoundedRectBorder( a_Rect, a_Rounding, a_Color, a_Thickness );
            return *this;
        }

        DrawList& AddCircle( Coloru8 a_Color, Vec2f a_Center, f32 a_Radius )
        {
            EnsureGeoBatch();
            Batcher.EmitCircle( a_Center, a_Radius, a_Color );
            return *this;
        }

        DrawList& AddCircleBorder( Coloru8 a_Color, Vec2f a_Center, f32 a_Radius, f32 a_Thickness = 1.f )
        {
            EnsureGeoBatch();
            Batcher.EmitCircleBorder( a_Center, a_Radius, a_Color, a_Thickness );
            return *this;
        }

        DrawList& AddText( const ShapedText& a_Shaped, TextRenderStyle a_Style, Rectf a_Rect )
        {
			EnsureMSDFBatch( a_Shaped.FontSize / Atlas.GetBaseSize() );
			Batcher.EmitText( a_Shaped, a_Style, a_Rect, Atlas );
            return *this;
        }

        void Finish()
        {
            FlushBatch();
        }

    private:
        Array<Rectu16>    m_ClipStack;
        Array<Mat3f>      m_TransformStack;

        Optional<Rectu16> m_CurrentClip;
        Mat3f             m_CurrentTransform{ c_Identity<Mat3f> };
        TextureID         m_CurrentTexture{ TextureID::Null() };
        bool              m_HasActiveBatch{ false };

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
            return a_Batch.Type == a_Type &&
                a_Batch.ClipRect == m_CurrentClip &&
                a_Batch.Transform == m_CurrentTransform &&
                a_Batch.Texture == m_CurrentTexture;
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
            Batcher.BeginBatch( EBatchType::Geometry, m_CurrentClip, m_CurrentTransform, m_CurrentTexture );
            m_HasActiveBatch = true;
        }

        void EnsureMSDFBatch( f32 a_Scale )
        {
            m_CurrentTexture = Atlas.GetTexture(); // For Text TODO( Probably wont only be for text so we may need to update this )

            if ( m_HasActiveBatch )
            {
                DrawBatch& b = Back( Batcher.Batches );

                if ( IsBatchStateCompatible( b, EBatchType::MSDF ) &&
                     b.MSDF.Scale == a_Scale )
                    return;
            }

            FlushBatch();
            DrawBatch& batch = Batcher.BeginBatch( EBatchType::MSDF, m_CurrentClip, m_CurrentTransform, m_CurrentTexture );

            batch.MSDF.Scale = a_Scale;
            m_HasActiveBatch = true;
        }
    };

} // namespace RatUI