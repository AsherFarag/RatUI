#pragma once
#include "../Core.h"
#include "Brush.h"

namespace RatUI
{
    /** 
     * @brief IRenderer is an interface that defines the basic drawing operations for rendering UI elements.
     * It can be implemented using different graphics APIs (e.g., DirectX, OpenGL, Vulkan) or software rendering techniques.
     */
    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        #pragma region Drawing API

        /** 
         * @brief Draws a solid rectangle with the specified brush, position, and size.
         * @param a_Brush The brush to use for drawing.
         * @param a_Position The position of the top-left corner of the rectangle.
         * @param a_Size The size of the rectangle.
         */
        virtual void DrawRect( const Brush& a_Brush, 
                               Vec2f a_Position, 
                               Vec2f a_Size ) = 0;

        /** 
         * @brief Draws a rounded rectangle with the specified brush, position, size, and corner radius.
         * @param a_Brush The brush to use for drawing.
         * @param a_Position The position of the top-left corner of the rectangle.
         * @param a_Size The size of the rectangle.
         * @param a_CornerRadius The radius of the rounded corners.
         */
        virtual void DrawRoundedRect( const Brush& a_Brush, 
                                      Vec2f a_Position,
                                      Vec2f a_Size, 
                                      f32 a_CornerRadius ) = 0;

        /** 
         * @brief Draws the border of a solid rectangle with the specified brush, position, size, and thickness.
         * @param a_Brush The brush to use for drawing.
         * @param a_Position The position of the top-left corner of the rectangle.
         * @param a_Size The size of the rectangle.
         * @param a_Thickness The thickness of the border.
         */
        virtual void DrawRectBorder( const Brush& a_Brush, 
                                     Vec2f a_Position,
                                     Vec2f a_Size, 
                                     f32 a_Thickness ) = 0;

        /** 
         * @brief Draws the border of a rounded rectangle with the specified brush, position, size, corner radius, and thickness.
         * @param a_Brush The brush to use for drawing.
         * @param a_Position The position of the top-left corner of the rectangle.
         * @param a_Size The size of the rectangle.
         * @param a_CornerRadius The radius of the rounded corners.
         * @param a_Thickness The thickness of the border.
         */
        virtual void DrawRoundedRectBorder( const Brush& a_Brush, 
                                            Vec2f a_Position,
                                            Vec2f a_Size, 
                                            f32 a_CornerRadius, 
                                            f32 a_Thickness ) = 0;

        #pragma endregion

        /** 
         * @brief Sets the color override for the renderer.
         * @param a_Color The color to override with.
         */
        void SetColorOverride( Color a_Color ) { m_Color = a_Color; }

        /** 
         * @brief Sets the color for the renderer.
         * @param a_Color The color to set.
         */
        void SetColor( Color a_Color ) { SetColorOverride( m_Color * a_Color ); }

    protected:

        /**
         * @brief A simple stub drawing function that can be used for testing and debugging purposes.
         * Draws a solid gray rectangle at the specified position and size.
         * @param a_Position The position of the top-left corner of the rectangle.
         * @param a_Size The size of the rectangle.
         */
        void DrawStub( Vec2f a_Position, Vec2f a_Size )
        {
            DrawRect( SolidBrush{ .Color = Color{ 0.5f, 0.5f, 0.5f, 1.0f } }, a_Position, a_Size );
        }

    protected:
        Color m_Color{ 1.0f, 1.0f, 1.0f, 1.0f };
    };
}