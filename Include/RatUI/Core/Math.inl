#pragma once

#include "../Core.h"
#include "Containers.inl"
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

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
        // TODO: This implementation requires many member functions,
        // ideally it should just be a simple struct with operator[] and the rest of the functionality should be implemented as free functions. 
        // So libraries like 'glm' can be easily integrated without users writing custom wrappers

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
        
            constexpr Vec()
            {
                for ( size i = 0; i < Dim; ++i )
                {
                    Data[ i ] = T{};
                }
            }

            constexpr explicit Vec( const T& a_Value )
            {
                for ( size i = 0; i < Dim; ++i )
                {
                    Data[ i ] = a_Value;
                }
            }
        
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

            constexpr Vec& operator+=( const Vec& a_Rhs )
            {
                for ( size i = 0; i < Dim; ++i ) Data[ i ] += a_Rhs[ i ];
                return *this;
            }
        
            constexpr Vec operator-( const Vec& a_Rhs ) const
            {
                Vec Result;
                for ( size i = 0; i < Dim; ++i ) Result[ i ] = Data[ i ] - a_Rhs[ i ];
                return Result;
            }

            constexpr Vec& operator-=( const Vec& a_Rhs )
            {
                for ( size i = 0; i < Dim; ++i ) Data[ i ] -= a_Rhs[ i ];
                return *this;
            }
        
            constexpr Vec operator*( T a_Scalar ) const
            {
                Vec Result;
                for ( size i = 0; i < Dim; ++i ) Result[ i ] = Data[ i ] * a_Scalar;
                return Result;
            }

            constexpr Vec& operator*=( T a_Scalar )
            {
                for ( size i = 0; i < Dim; ++i ) Data[ i ] *= a_Scalar;
                return *this;
            }
        
            constexpr Vec operator/( T a_Scalar ) const
            {
                Vec Result;
                for ( size i = 0; i < Dim; ++i ) Result[ i ] = Data[ i ] / a_Scalar;
                return Result;
            }

            constexpr Vec& operator/=( T a_Scalar )
            {
                for ( size i = 0; i < Dim; ++i ) Data[ i ] /= a_Scalar;
                return *this;
            }
        
            constexpr bool operator==( const Vec& a_Rhs ) const
            {
                for ( size i = 0; i < Dim; ++i )
                    if ( Data[ i ] != a_Rhs[ i ] ) return false;
                return true;
            }

            constexpr bool operator!=( const Vec& a_Rhs ) const
            {
                return !( *this == a_Rhs );
            }
        
            constexpr T Dot( const Vec& a_Rhs ) const
            {
                T Result{};
                for ( size i = 0; i < Dim; ++i ) Result += Data[ i ] * a_Rhs[ i ];
                return Result;
            }
        
            constexpr T LengthSq() const { return Dot( *this ); }
            constexpr T Length()   const { return static_cast<T>( std::sqrt( LengthSq() ) ); }

            constexpr Vec Normalized() const
            {
                const T CurrentLength = Length();
                if constexpr ( std::is_floating_point_v<T> )
                {
                    if ( CurrentLength <= std::numeric_limits<T>::epsilon() )
                    {
                        return Vec{};
                    }
                }
                else
                {
                    if ( CurrentLength == T{} )
                    {
                        return Vec{};
                    }
                }

                return *this / CurrentLength;
            }
        };

        template<typename T, size Dim>
        constexpr Vec<T, Dim> operator*( T a_Scalar, const Vec<T, Dim>& a_Vector )
        {
            return a_Vector * a_Scalar;
        }
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

        static constexpr Rectf FromMinMax( Vec2f a_Min, Vec2f a_Max )
        {
            const Vec2f Size = a_Max - a_Min;
            return { a_Min + ( Size * 0.5f ), Size * 0.5f };
        }

        constexpr f32   Top() const { return Center[ 1 ] - HalfExtents[ 1 ]; }
        constexpr f32   Bottom() const { return Center[ 1 ] + HalfExtents[ 1 ]; }
        constexpr f32   Left() const { return Center[ 0 ] - HalfExtents[ 0 ]; }
        constexpr f32   Right() const { return Center[ 0 ] + HalfExtents[ 0 ]; }

        constexpr Vec2f TopLeft() const { return Center - HalfExtents; }
        constexpr Vec2f TopRight() const { return Vec2f{ Center[ 0 ] + HalfExtents[ 0 ], Center[ 1 ] - HalfExtents[ 1 ] }; }
        constexpr Vec2f BottomLeft() const { return Vec2f{ Center[ 0 ] - HalfExtents[ 0 ], Center[ 1 ] + HalfExtents[ 1 ] }; }
        constexpr Vec2f BottomRight() const { return Center + HalfExtents; }

        constexpr Vec2f Min() const { return TopLeft(); }
        constexpr Vec2f Max() const { return BottomRight(); }

        constexpr Vec2f Size() const { return HalfExtents * 2; }

        constexpr bool Intersects( const Rectf& a_Other ) const
        {
            return Left() <= a_Other.Right() && Right() >= a_Other.Left() &&
                   Top() <= a_Other.Bottom() && Bottom() >= a_Other.Top();
        }

        constexpr bool Contains( Vec2f a_Point ) const
        {
            Vec2f TopLeft = this->TopLeft();
            Vec2f BottomRight = this->BottomRight();
            return ( a_Point[ 0 ] >= TopLeft[ 0 ] && a_Point[ 0 ] <= BottomRight[ 0 ] ) &&
                   ( a_Point[ 1 ] >= TopLeft[ 1 ] && a_Point[ 1 ] <= BottomRight[ 1 ] );
        }

        constexpr Rectf Intersection( const Rectf& a_Other ) const
        {
            const Vec2f NewMin{ ( Left() > a_Other.Left() ) ? Left() : a_Other.Left(),
                                ( Top() > a_Other.Top() ) ? Top() : a_Other.Top() };
            const Vec2f NewMax{ ( Right() < a_Other.Right() ) ? Right() : a_Other.Right(),
                                ( Bottom() < a_Other.Bottom() ) ? Bottom() : a_Other.Bottom() };

            if ( NewMax[ 0 ] < NewMin[ 0 ] || NewMax[ 1 ] < NewMin[ 1 ] )
            {
                return {};
            }

            return Rectf::FromMinMax( NewMin, NewMax );
        }
    };

} // namespace RatUI