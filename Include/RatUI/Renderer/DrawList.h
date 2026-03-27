#pragma once
#include "../Core.h"
#include "Brush.h"
#include "RenderTransform.h"

namespace RatUI
{
    using CustomDrawFunc = void(*)( class IRenderInterface& a_Renderer, const struct DrawCmd& a_Cmd );

    struct DrawCmd
    {
        enum class EType : u8
        {
            Unknown = 0,
            Rect,
            RoundedRect,
            RectBorder,
            RoundedRectBorder,
            Circle,
            CircleBorder,
            Custom = 255u
        };

        Brush DrawBrush;
        Mat3f Transform{ c_Identity<Mat3f> };
        Rectf ClipRect;
        EType Type{ EType::Unknown };

        union
        {
            Rectf Rect;

            struct
            {
                Rectf Rect;
                f32 CornerRadius;
            } RoundedRect;

            struct 
            {
                Rectf Rect;
                f32 BorderThickness;
            } RectBorder;

            struct 
            {
                Rectf Rect;
                f32 CornerRadius;
                f32 BorderThickness;
            } RoundedRectBorder;
            
            struct 
            {
                Vec2f Center;
                f32 Radius;
            } Circle;

            struct
            {
                Vec2f Center;
                f32 Radius;
                f32 BorderThickness;
            } CircleBorder;

            struct 
            {
                CustomDrawFunc Func;
                void* UserData;
            } Custom;
        };
    };

    struct DrawList
    {
        Array<DrawCmd> Commands;
        Array<Rectf> ClipStack;
        Array<Mat3f> TransformStack;

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
            if ( !Empty( TransformStack ) )
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
            if ( !Empty( ClipStack ) )
                PopBack( ClipStack );

            return *this;
        }

        DrawList& AddRect( Brush a_Brush, Rectf a_Rect )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
				.Type = DrawCmd::EType::Rect,
                .Rect = a_Rect
			} );
            return *this;
        }

        DrawList& AddRoundedRect( Brush a_Brush, Rectf a_Rect, f32 a_CornerRadius )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Type = DrawCmd::EType::RoundedRect,
                .RoundedRect = {.Rect = a_Rect, .CornerRadius = a_CornerRadius }
			} );
            return *this;
        }

        DrawList& AddRectBorder( Brush a_Brush, Rectf a_Rect, f32 a_Thickness = 1.f )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Type = DrawCmd::EType::RectBorder,
                .RectBorder = {.Rect = a_Rect, .BorderThickness = a_Thickness }
            } );
            return *this;
        }

        DrawList& AddRoundedRectBorder( Brush a_Brush, Rectf a_Rect, f32 a_CornerRadius, f32 a_Thickness = 1.f )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Type = DrawCmd::EType::RoundedRectBorder,
                .RoundedRectBorder = {.Rect = a_Rect, .CornerRadius = a_CornerRadius, .BorderThickness = a_Thickness }
            } );
            return *this;
        }

        DrawList& AddCircle( Brush a_Brush, Vec2f a_Center, f32 a_Radius )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Type = DrawCmd::EType::Circle,
                .Circle = {.Center = a_Center, .Radius = a_Radius }
            } );
            return *this;
        }

        DrawList& AddCircleBorder( Brush a_Brush, Vec2f a_Center, f32 a_Radius, f32 a_Thickness = 1.f )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Type = DrawCmd::EType::CircleBorder,
                .CircleBorder = {.Center = a_Center, .Radius = a_Radius, .BorderThickness = a_Thickness }
            } );
            return *this;
        }

		DrawList& AddCustom( Brush a_Brush, CustomDrawFunc a_Func, void* a_UserData = nullptr )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = std::move( a_Brush ),
                .Transform = CurrentTransform(),
                .ClipRect = CurrentClipRect(),
                .Type = DrawCmd::EType::Custom,
                .Custom = {.Func = a_Func, .UserData = a_UserData }
            } );
            return *this;
        }

        DrawList& AddCustom( CustomDrawFunc a_Func, void* a_UserData = nullptr )
        {
            return AddCustom( SolidBrush{ .Color = Colors::White }, a_Func, a_UserData );
        }
    };

} // namespace RatUI