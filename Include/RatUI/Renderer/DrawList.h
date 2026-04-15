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
    struct DrawList
    {
        DrawBatcher& Batcher;
		GlyphAtlas&  Atlas;

        Array<Rectu16> ClipStack;
        Array<Mat3f>   TransformStack;

        Optional<Rectu16> CurrentClip;
        Mat3f             CurrentTransform{ c_Identity<Mat3f> };
        TextureID         CurrentTexture  { TextureID::Null() };
        bool              HasActiveBatch  { false };

        void Clear()
        {
            ::RatUI::Clear( ClipStack );
            ::RatUI::Clear( TransformStack );

            CurrentClip.reset();
            CurrentTransform = c_Identity<Mat3f>;
            CurrentTexture   = TextureID::Null();

            HasActiveBatch = false;
        }

    private:

        void FlushBatch()
        {
            if ( HasActiveBatch )
            {
                Batcher.EndBatch();
                HasActiveBatch = false;
            }
        }

        void EnsureBatch()
        {
            if ( !HasActiveBatch )
            {
                Batcher.BeginGeoBatch( CurrentClip, CurrentTransform, CurrentTexture );
                HasActiveBatch = true;
            }
        }

        void UpdateBatchState()
        {
            FlushBatch();
            EnsureBatch();
        }

    public:

        // ========================
        // Transform Stack
        // ========================

        const Mat3f& GetCurrentTransform() const
        {
            return Empty( TransformStack )
                ? c_Identity<Mat3f>
                : Back( TransformStack );
        }

        DrawList& PushTransform( const Mat3f& a_Transform )
        {
            PushBack( TransformStack, Empty( TransformStack ) ? a_Transform : Back( TransformStack ) * a_Transform );
            CurrentTransform = Back( TransformStack );

            UpdateBatchState();
            return *this;
        }

        DrawList& PopTransform()
        {
            RATUI_USER_ASSERT( !Empty( TransformStack ),
                "PopTransform called too many times." );

            PopBack( TransformStack );
            CurrentTransform = GetCurrentTransform();

            UpdateBatchState();
            return *this;
        }

        // ========================
        // Clip Stack
        // ========================

        DrawList& PushClipRect( Rectu16 a_Rect )
        {
            if ( !Empty( ClipStack ) )
                a_Rect = a_Rect.Intersection( Back( ClipStack ) );

            PushBack( ClipStack, a_Rect );
            CurrentClip = a_Rect;

            UpdateBatchState();
            return *this;
        }

        DrawList& PopClipRect()
        {
            RATUI_USER_ASSERT( !Empty( ClipStack ),
                "PopClipRect called too many times." );

            PopBack( ClipStack );

            if ( Empty( ClipStack ) ) CurrentClip.reset();
            else                      CurrentClip = Back( ClipStack );

            UpdateBatchState();
            return *this;
        }

        // ========================
        // Drawing
        // ========================

        DrawList& AddRect( Coloru8 a_Color, Rectf a_Rect )
        {
            EnsureBatch();
            Batcher.EmitRect( a_Rect, a_Color );
            return *this;
        }

        DrawList& AddRect( Coloru8 a_Color, Rectf a_Rect, CornerRounding a_Rounding )
        {
            EnsureBatch();
            Batcher.EmitRoundedRect( a_Rect, a_Rounding, a_Color );
            return *this;
        }

        DrawList& AddRectBorder( Coloru8 a_Color, Rectf a_Rect, f32 a_Thickness = 1.f )
        {
            EnsureBatch();
            Batcher.EmitRectBorder( a_Rect, 0.f, a_Color, a_Thickness );
            return *this;
        }

        DrawList& AddRectBorder( Coloru8 a_Color, Rectf a_Rect, CornerRounding a_Rounding, f32 a_Thickness = 1.f )
        {
            EnsureBatch();
            Batcher.EmitRoundedRectBorder( a_Rect, a_Rounding, a_Color, a_Thickness );
            return *this;
        }

        DrawList& AddCircle( Coloru8 a_Color, Vec2f a_Center, f32 a_Radius )
        {
            EnsureBatch();
            Batcher.EmitCircle( a_Center, a_Radius, a_Color );
            return *this;
        }

        DrawList& AddCircleBorder( Coloru8 a_Color, Vec2f a_Center, f32 a_Radius, f32 a_Thickness = 1.f )
        {
            EnsureBatch();
            Batcher.EmitCircleBorder( a_Center, a_Radius, a_Color, a_Thickness );
            return *this;
        }

        DrawList& AddText( const ShapedText& a_Shaped, TextRenderStyle a_Style, Rectf a_Rect )
        {
            EnsureBatch();
			Batcher.EmitText( a_Shaped, a_Style, a_Rect, Atlas );
            return *this;
        }

        void Finish()
        {
            FlushBatch();
        }
    };

} // namespace RatUI