#pragma once
#include "../Core.h"
#include "Brush.h"
#include "RenderTransform.h"

namespace RatUI
{
    /**
     * @brief Used for custom rendering commands that don't fit into the predefined categories. 
     * @param a_Renderer The renderer to use for drawing.
     * @param a_Cmd The draw command containing the necessary information for rendering.
     */
    using CustomDrawFunc = void(*)( class IRenderer& a_Renderer, const struct DrawCmd& a_Cmd );

    /**
     * @brief Represents a single drawing command, which are buffered in a DrawList and later executed by the renderer. 
     */
    struct DrawCmd
    {
        struct RectCmd { Rectf Rect; };

        struct RectBorderCmd { Rectf Rect; f32 BorderThickness; };

        struct RoundedRectCmd { Rectf Rect; f32 CornerRadius; };

        struct RoundedRectBorderCmd { Rectf Rect; f32 CornerRadius; f32 BorderThickness; };

        struct CircleCmd { Vec2f Center; f32 Radius; };

        struct CircleBorderCmd { Vec2f Center; f32 Radius; f32 BorderThickness; };

        struct CustomCmd { CustomDrawFunc Func; void* UserData; };

        Brush DrawBrush;
        Mat3f Transform{ c_Identity<Mat3f> };
        Rectf ClipRect;

        Variant<
            RectCmd,
            RectBorderCmd,
            RoundedRectCmd,
            RoundedRectBorderCmd,
            CircleCmd,
            CircleBorderCmd,
            CustomCmd
        > Payload;
    };

    /**
     * @brief A DrawList is a collection of draw commands that can be recorded and then executed by the renderer.
     * It also maintains a stack of clipping rectangles and transformation matrices, allowing for hierarchical transformations and clipping.
     */
    struct DrawList
    {
        Array<DrawCmd> Commands;     ///< Buffered draw commands to be executed by the renderer.
        Array<Rectf> ClipStack;      ///< Stack of clipping rectangles. The current clipping rectangle is the intersection of all rectangles in the stack.
        Array<Mat3f> TransformStack; ///< Stack of transformation matrices. The current transformation is the product of all matrices in the stack.

        void Clear()
        {
            ::RatUI::Clear( Commands );
            ::RatUI::Clear( ClipStack );
            ::RatUI::Clear( TransformStack );
        }

        const Mat3f& CurrentTransform() const 
        {
            return Empty( TransformStack ) ? c_Identity<Mat3f> : Back( TransformStack );
        }

        DrawList& PushTransform( const Mat3f& a_Transform )
        {
            PushBack( TransformStack, CurrentTransform() * a_Transform );
            return *this;
        }

        DrawList& PopTransform()
        {
            RATUI_USER_ASSERT( !Empty( TransformStack ), "Called PopTransform too many times: no transform to pop." );
            PopBack( TransformStack );      
            return *this;
        }

        Rectf CurrentClipRect() const
        {
            if ( Empty( ClipStack ) )
                return Rectf::Infite();

            return Back( ClipStack );
        }

        DrawList& PushClipRect( Rectf a_Rect )
        {
            // Intersect with current clip rect if one exists
            if ( !Empty( ClipStack ) )
                a_Rect = a_Rect.Intersection( Back( ClipStack ) );

            PushBack( ClipStack, a_Rect );
            return *this;
        }

        DrawList& PopClipRect()
        {
            RATUI_USER_ASSERT( !Empty( ClipStack ), "Called PopClipRect too many times: no clip rect to pop." );
            PopBack( ClipStack );
            return *this;
        }

        DrawList& AddRect( Brush a_Brush, Rectf a_Rect )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Payload = DrawCmd::RectCmd{ .Rect = a_Rect }
			} );
            return *this;
        }

        DrawList& AddRoundedRect( Brush a_Brush, Rectf a_Rect, f32 a_CornerRadius )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Payload = DrawCmd::RoundedRectCmd{ .Rect = a_Rect, .CornerRadius = a_CornerRadius }
			} );
            return *this;
        }

        DrawList& AddRectBorder( Brush a_Brush, Rectf a_Rect, f32 a_Thickness = 1.f )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Payload = DrawCmd::RectBorderCmd{ .Rect = a_Rect, .BorderThickness = a_Thickness }
            } );
            return *this;
        }

        DrawList& AddRoundedRectBorder( Brush a_Brush, Rectf a_Rect, f32 a_CornerRadius, f32 a_Thickness = 1.f )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Payload = DrawCmd::RoundedRectBorderCmd{ .Rect = a_Rect, .CornerRadius = a_CornerRadius, .BorderThickness = a_Thickness }
            } );
            return *this;
        }

        DrawList& AddCircle( Brush a_Brush, Vec2f a_Center, f32 a_Radius )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Payload = DrawCmd::CircleCmd{ .Center = a_Center, .Radius = a_Radius }
            } );
            return *this;
        }

        DrawList& AddCircleBorder( Brush a_Brush, Vec2f a_Center, f32 a_Radius, f32 a_Thickness = 1.f )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Payload = DrawCmd::CircleBorderCmd{ .Center = a_Center, .Radius = a_Radius, .BorderThickness = a_Thickness }
            } );
            return *this;
        }

		DrawList& AddCustom( Brush a_Brush, CustomDrawFunc a_Func, void* a_UserData = nullptr )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Payload = DrawCmd::CustomCmd{ .Func = a_Func, .UserData = a_UserData }
            } );
            return *this;
        }

        DrawList& AddCustom( CustomDrawFunc a_Func, void* a_UserData = nullptr )
        {
            return AddCustom( SolidBrush{ .Color = Colors::White }, a_Func, a_UserData );
        }
    };

} // namespace RatUI