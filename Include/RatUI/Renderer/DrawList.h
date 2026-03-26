#pragma once
#include "../Core.h"
#include "Brush.h"

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
            Custom = ~0u
        };

        Brush DrawBrush;
        Rectf ClipRect;

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

        EType Type{ EType::Unknown };
    };

    struct DrawList
    {
        Array<DrawCmd> Commands;
        Array<Rectf> ClipStack;

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

        DrawList& AddRect( const Brush& a_Brush, Rectf a_Rect )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = a_Brush,
                .ClipRect = CurrentClipRect(),
				.Rect = a_Rect,
				.Type = DrawCmd::EType::Rect
			} );
            return *this;
        }

        DrawList& AddRoundedRect( const Brush& a_Brush, Rectf a_Rect, f32 a_CornerRadius )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = a_Brush,
                .ClipRect = CurrentClipRect(),
                .RoundedRect = {.Rect = a_Rect, .CornerRadius = a_CornerRadius },
                .Type = DrawCmd::EType::RoundedRect
			} );
            return *this;
        }

        DrawList& AddRectBorder( const Brush& a_Brush, Rectf a_Rect, f32 a_Thickness = 1.f )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = a_Brush,
                .ClipRect = CurrentClipRect(),
                .RectBorder = {.Rect = a_Rect, .BorderThickness = a_Thickness },
                .Type = DrawCmd::EType::RectBorder
            } );
            return *this;
        }

        DrawList& AddRoundedRectBorder( const Brush& a_Brush, Rectf a_Rect, f32 a_CornerRadius, f32 a_Thickness = 1.f )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = a_Brush,
                .ClipRect = CurrentClipRect(),
                .RoundedRectBorder = {.Rect = a_Rect, .CornerRadius = a_CornerRadius, .BorderThickness = a_Thickness },
                .Type = DrawCmd::EType::RoundedRectBorder
            } );
            return *this;
        }

        DrawList& AddCircle( const Brush& a_Brush, Vec2f a_Center, f32 a_Radius )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = a_Brush,
                .ClipRect = CurrentClipRect(),
                .Circle = {.Center = a_Center, .Radius = a_Radius },
                .Type = DrawCmd::EType::Circle
            } );
            return *this;
        }

        DrawList& AddCircleBorder( const Brush& a_Brush, Vec2f a_Center, f32 a_Radius, f32 a_Thickness = 1.f )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = a_Brush,
                .ClipRect = CurrentClipRect(),
                .CircleBorder = {.Center = a_Center, .Radius = a_Radius, .BorderThickness = a_Thickness },
                .Type = DrawCmd::EType::CircleBorder
            } );
            return *this;
        }

		DrawList& AddCustom( CustomDrawFunc a_Func, void* a_UserData = nullptr )
        {
            PushBack( Commands, DrawCmd{
                .DrawBrush = SolidBrush{ .Color = Color{ 0.f, 0.f, 0.f, 0.f } }, // Unused for custom draw commands, but we need to set it to something to avoid uninitialized data
                .ClipRect = CurrentClipRect(),
                .Custom = {.Func = a_Func, .UserData = a_UserData },
                .Type = DrawCmd::EType::Custom
            } );
            return *this;
        }
    };

} // namespace RatUI