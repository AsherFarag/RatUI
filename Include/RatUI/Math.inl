#pragma once

#include "Core.h"
#include "Containers.inl"

#ifndef RATUI_VEC_IMPL
    #define RATUI_VEC_IMPL ::RatUI::Detail::Vec
#endif // Default to Vec template if no custom vector implementation is provided.

#ifndef RATUI_COLOR_IMPL
    #define RATUI_COLOR_IMPL RATUI_VEC_IMPL<RatUI::f32, 4>
#endif // Default to Vec<f32, 4> if no custom Color implementation is provided.

namespace RatUI
{
    namespace Detail
    {
        /**
         * @brief A simple fixed-size vector class template for mathematical operations.
         * @brief This is a basic implementation and can be extended with more functionality as needed.
         * @tparam T The type of the elements in the vector (e.g., float, int).
         * @tparam Dim The dimension of the vector (e.g., 2 for Vec2, 3 for Vec3).
         */
        template<typename T, size Dim>
        struct Vec
        {
            static constexpr size c_Dim = Dim;

            FixedArray<T, Dim> Data;
        
            constexpr Vec() = default;
        
            template<typename... Args>
            constexpr Vec( Args&&... a_Args ) : Data{ std::forward<Args>( a_Args )... } {}
        
            constexpr T&       operator[]( size a_Index )       { return Data[ a_Index ]; }
            constexpr const T& operator[]( size a_Index ) const { return Data[ a_Index ]; }
        
            constexpr Vec operator+( const Vec& a_Rhs ) const
            {
                Vec Result;
                for ( size i = 0; i < Dim; ++i ) Result[ i ] = Data[ i ] + a_Rhs[ i ];
                return Result;
            }
        
            constexpr Vec operator-( const Vec& a_Rhs ) const
            {
                Vec Result;
                for ( size i = 0; i < Dim; ++i ) Result[ i ] = Data[ i ] - a_Rhs[ i ];
                return Result;
            }
        
            constexpr Vec operator*( T a_Scalar ) const
            {
                Vec Result;
                for ( size i = 0; i < Dim; ++i ) Result[ i ] = Data[ i ] * a_Scalar;
                return Result;
            }
        
            constexpr Vec operator/( T a_Scalar ) const
            {
                Vec Result;
                for ( size i = 0; i < Dim; ++i ) Result[ i ] = Data[ i ] / a_Scalar;
                return Result;
            }
        
            constexpr bool operator==( const Vec& a_Rhs ) const
            {
                for ( size i = 0; i < Dim; ++i )
                    if ( Data[ i ] != a_Rhs[ i ] ) return false;
                return true;
            }
        
            constexpr T Dot( const Vec& a_Rhs ) const
            {
                T Result{};
                for ( size i = 0; i < Dim; ++i ) Result += Data[ i ] * a_Rhs[ i ];
                return Result;
            }
        
            constexpr T LengthSq() const { return Dot( *this ); }
            constexpr T Length()   const { return static_cast<T>( std::sqrt( LengthSq() ) ); }
        
            constexpr Vec Normalized() const { return *this / Length(); }
        };
    }

    using Vec2f = RATUI_VEC_IMPL<RatUI::f32, 2>;
    using Vec2i = RATUI_VEC_IMPL<RatUI::i32, 2>;
    using Vec2u = RATUI_VEC_IMPL<RatUI::u32, 2>;
    using Vec3f = RATUI_VEC_IMPL<RatUI::f32, 3>;
    using Vec3i = RATUI_VEC_IMPL<RatUI::i32, 3>;
    using Vec3u = RATUI_VEC_IMPL<RatUI::u32, 3>;
    using Vec4f = RATUI_VEC_IMPL<RatUI::f32, 4>;
    using Vec4i = RATUI_VEC_IMPL<RatUI::i32, 4>;
    using Vec4u = RATUI_VEC_IMPL<RatUI::u32, 4>;
    using Color = RATUI_COLOR_IMPL;

    /**
     * @brief A simple axis-aligned rectangle structure defined by its center and half-extents.
     * @brief This is a basic implementation and can be extended with more functionality as needed.
     */
    struct Rectf
    {
        Vec2f Center{ 0, 0 };
        Vec2f HalfExtents{ 0, 0 };

        constexpr f32   Top() const { return Center[ 1 ] - HalfExtents[ 1 ]; }
        constexpr f32   Bottom() const { return Center[ 1 ] + HalfExtents[ 1 ]; }
        constexpr f32   Left() const { return Center[ 0 ] - HalfExtents[ 0 ]; }
        constexpr f32   Right() const { return Center[ 0 ] + HalfExtents[ 0 ]; }

        constexpr Vec2f TopLeft() const { return Center - HalfExtents; }
        constexpr Vec2f TopRight() const { return Vec2f{ Center[ 0 ] + HalfExtents[ 0 ], Center[ 1 ] - HalfExtents[ 1 ] }; }
        constexpr Vec2f BottomLeft() const { return Vec2f{ Center[ 0 ] - HalfExtents[ 0 ], Center[ 1 ] + HalfExtents[ 1 ] }; }
        constexpr Vec2f BottomRight() const { return Center + HalfExtents; }

        constexpr Vec2f Size() const { return HalfExtents * 2; }

        constexpr bool Contains( Vec2f a_Point ) const
        {
            Vec2f TopLeft = this->TopLeft();
            Vec2f BottomRight = this->BottomRight();
            return ( a_Point[ 0 ] >= TopLeft[ 0 ] && a_Point[ 0 ] <= BottomRight[ 0 ] ) &&
                   ( a_Point[ 1 ] >= TopLeft[ 1 ] && a_Point[ 1 ] <= BottomRight[ 1 ] );
        }
    };

} // namespace RatUI