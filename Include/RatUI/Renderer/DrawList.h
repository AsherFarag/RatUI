#pragma once
#include "../Core.h"
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
    using CustomDrawFunc = void(*)( class IRenderer& a_Renderer, void* a_UserData, const Mat3f& a_CurrentTransform );

    /**
     * @brief Represents a single drawing command, which are buffered in a DrawList and later executed by the renderer. 
     */
    struct DrawCmd
    {
        struct SetClipRectCmd 
        { 
            Optional<Rectf> Rect; ///< If nullopt, indicates that clipping should be disabled. Otherwise, the specified rectangle should be used for clipping.
        };

        struct SetTransformCmd 
        { 
            Mat3f Transform;
        };

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

        struct CustomCmd 
        { 
            CustomDrawFunc Func; 
            void* UserData; 
        };

        Variant<
            SetClipRectCmd,
            SetTransformCmd,
            RectCmd,
            RectBorderCmd,
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

        // === Transform Stack Operations ===

        const Mat3f& CurrentTransform() const 
        {
            return Empty( TransformStack ) ? c_Identity<Mat3f> : Back( TransformStack );
        }

        DrawList& PushTransform( const Mat3f& a_Transform )
        {
            PushBack( TransformStack, CurrentTransform() * a_Transform );
            EmplaceBack( Commands, DrawCmd::SetTransformCmd{ .Transform = CurrentTransform() } );
            return *this;
        }

        DrawList& PopTransform()
        {
            RATUI_USER_ASSERT( !Empty( TransformStack ), "Called PopTransform too many times: no transform to pop." );
            PopBack( TransformStack );      
            EmplaceBack( Commands, DrawCmd::SetTransformCmd{ .Transform = CurrentTransform() } );
            return *this;
        }

        // === Clip Stack Operations ===

        Optional<Rectf> CurrentClipRect() const
        {
            if ( Empty( ClipStack ) )
                return NullOpt;

            return Back( ClipStack );
        }

        DrawList& PushClipRect( Rectf a_Rect )
        {
            // Intersect with current clip rect if one exists
            if ( !Empty( ClipStack ) )
                a_Rect = a_Rect.Intersection( Back( ClipStack ) );

            EmplaceBack( ClipStack, a_Rect );
            EmplaceBack( Commands, DrawCmd::SetClipRectCmd{ .Rect = a_Rect } );

            return *this;
        }

        DrawList& PopClipRect()
        {
            RATUI_USER_ASSERT( !Empty( ClipStack ), "Called PopClipRect too many times: no clip rect to pop." );

            PopBack( ClipStack );
            EmplaceBack( Commands, DrawCmd::SetClipRectCmd{ .Rect = CurrentClipRect() } );

            return *this;
        }

        // === Draw Commands ===

        DrawList& AddRect( Colorf a_Color, Rectf a_Rect, CornerRounding a_Rounding = {} )
        {
            EmplaceBack( Commands, DrawCmd::RectCmd{ .Color = a_Color, .Rect = a_Rect, .Rounding = a_Rounding } );
            return *this;
        }

        DrawList& AddRectBorder( Colorf a_Color, Rectf a_Rect, CornerRounding a_Rounding = {}, f32 a_Thickness = 1.f )
        {
            EmplaceBack( Commands, DrawCmd::RectBorderCmd{ .Color = a_Color, .Rect = a_Rect, .Rounding = a_Rounding, .Thickness = a_Thickness } );
            return *this;
        }

        DrawList& AddCircle( Colorf a_Color, Vec2f a_Center, f32 a_Radius )
        {
            EmplaceBack( Commands, DrawCmd::CircleCmd{ .Color = a_Color, .Center = a_Center, .Radius = a_Radius } );
            return *this;
        }

        DrawList& AddCircleBorder( Colorf a_Color, Vec2f a_Center, f32 a_Radius, f32 a_Thickness = 1.f )
        {
            EmplaceBack( Commands, DrawCmd::CircleBorderCmd{ .Color = a_Color, .Center = a_Center, .Radius = a_Radius, .Thickness = a_Thickness } );
            return *this;
        }

		DrawList& AddCustom( CustomDrawFunc a_Func, void* a_UserData = nullptr )
        {
			RATUI_USER_ASSERT( a_Func, "Custom draw command requires a valid function pointer." );

			if ( a_Func )
            {
                EmplaceBack( Commands, DrawCmd::CustomCmd{ .Func = a_Func, .UserData = a_UserData } );
            }

            return *this;
        }
    };

} // namespace RatUI