#pragma once
#include "../Core.h"
#include "../Text/Text.h"
#include "RenderTransform.h"

namespace RatUI
{
    struct CornerRounding
    {
        Degreesf TopLeft{ 0.f };
        Degreesf TopRight{ 0.f };
        Degreesf BottomLeft{ 0.f };
        Degreesf BottomRight{ 0.f };

        static constexpr CornerRounding None() { return {}; }
        static constexpr CornerRounding Uniform( Degreesf a_Radius ) { return { a_Radius, a_Radius, a_Radius, a_Radius }; }
        static constexpr CornerRounding Symmetric( Degreesf a_Top, Degreesf a_Bottom ) { return { a_Top, a_Top, a_Bottom, a_Bottom }; }

        constexpr CornerRounding operator+( Degreesf a_Amount ) const
        {
            return {
                .TopLeft = TopLeft + a_Amount,
                .TopRight = TopRight + a_Amount,
                .BottomLeft = BottomLeft + a_Amount,
                .BottomRight = BottomRight + a_Amount
            };
        }
    };

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
        struct RectCmd 
        { 
            Colorf Color;
            Rectf Rect;
            CornerRounding Rounding;
        };

        struct RectBorderCmd 
        { 
            Colorf Color;
            Rectf Rect;
            CornerRounding Rounding;
            f32 Thickness;
        };

        struct CircleCmd 
        { 
            Colorf Color;
            Vec2f Center; 
            f32 Radius; 
        };

        struct CircleBorderCmd 
        { 
            Colorf Color;
            Vec2f Center; 
            f32 Radius; 
            f32 Thickness;
        };

        // TODO: Would be safer to store the string but not too sure yet for perf
        struct TextCmd 
        { 
            TextView Text; 
            TextStyle Style;
            Rectf Rect; 
        };

        struct CustomCmd { CustomDrawFunc Func; void* UserData; };

        Mat3f Transform{ c_Identity<Mat3f> };
        Rectf ClipRect;

        Variant<
            RectCmd,
            RectBorderCmd,
            CircleCmd,
            CircleBorderCmd,
            TextCmd,
            //ShapedTextCmd,
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
                return Rectf::Infinite();

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

        DrawList& AddRect( Colorf a_Color, Rectf a_Rect, CornerRounding a_Rounding = {} )
        {
            PushBack( Commands, DrawCmd{
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Payload = DrawCmd::RectCmd{ .Color = a_Color, .Rect = a_Rect, .Rounding = a_Rounding }
			} );
            return *this;
        }

        DrawList& AddRectBorder( Colorf a_Color, Rectf a_Rect, CornerRounding a_Rounding = {}, f32 a_Thickness = 1.f )
        {
            PushBack( Commands, DrawCmd{
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Payload = DrawCmd::RectBorderCmd{ .Color = a_Color, .Rect = a_Rect, .Rounding = a_Rounding, .Thickness = a_Thickness }
            } );
            return *this;
        }

        DrawList& AddCircle( Colorf a_Color, Vec2f a_Center, f32 a_Radius )
        {
            PushBack( Commands, DrawCmd{
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Payload = DrawCmd::CircleCmd{ .Color = a_Color, .Center = a_Center, .Radius = a_Radius }
            } );
            return *this;
        }

        DrawList& AddCircleBorder( Colorf a_Color, Vec2f a_Center, f32 a_Radius, f32 a_Thickness = 1.f )
        {
            PushBack( Commands, DrawCmd{
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Payload = DrawCmd::CircleBorderCmd{ .Color = a_Color, .Center = a_Center, .Radius = a_Radius, .Thickness = a_Thickness }
            } );
            return *this;
        }

        DrawList& AddText( TextView a_Text, TextStyle a_Style, Rectf a_Rect )
        {
            PushBack( Commands, DrawCmd{
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Payload = DrawCmd::TextCmd{ .Text = a_Text, .Style = a_Style, .Rect = a_Rect }
            } );
            return *this;
        }

		DrawList& AddCustom( CustomDrawFunc a_Func, void* a_UserData = nullptr )
        {
			RATUI_USER_ASSERT( a_Func, "Custom draw command requires a valid function pointer." );

			if ( a_Func )
            {
                PushBack( Commands, DrawCmd{
                    .Transform = CurrentTransform(),
                    .ClipRect = CurrentClipRect(),
                    .Payload = DrawCmd::CustomCmd{.Func = a_Func, .UserData = a_UserData }
                } );
            }

            return *this;
        }
    };

} // namespace RatUI