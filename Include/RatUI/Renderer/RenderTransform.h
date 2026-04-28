#pragma once
#include "../Core.h"

namespace RatUI
{
    // TODO: This assumes Y-down, should note this somewhere. 

    /**
     * @brief A struct representing a combination of transformations that can be applied to rendered content.
     * @note Used by widgets to specify transformations that only apply during rendering, e.g., for hover effects, without affecting layout or hit testing.
     */
    struct RenderTransform
    {
        Vec2<Unit>    Translation{ 0_u, 0_u };    ///< The amount to translate the rendered content by. In pixels.
        Vec2<Unit>    Scale{ 1_u, 1_u };          ///< The amount to scale the rendered content by. 1x means no scaling, 2x means double size, etc.
        Vec2<Unit>    Pivot{ 0.5_u, 0.5_u };      ///< The pivot point around which to apply the transformations. (0, 0) Top-left corner, (1, 1) bottom-right corner, etc.
        Radians<Unit> SkewX{ 0_u }, SkewY{ 0_u }; ///< The amount to shear the rendered content by along the X and Y axes, respectively.
        Radians<Unit> Angle{ 0_u };               ///< The angle to rotate the rendered content by. Clockwise direction.

        constexpr bool operator==( const RenderTransform& ) const = default;

        /** @brief Returns the identity transformation. */
        static constexpr RenderTransform Identity() { return RenderTransform{}; }

        /** @brief Checks if the transformation is the identity transformation. */
        constexpr bool IsIdentity() const { return *this == Identity(); }

        /** @brief Resolves the pivot point to an absolute position based on a given rectangle. */
        constexpr Vec2<Unit> ResolvePivot( Rect<Unit> a_Rect ) const
        {
            return Vec2<Unit>{
                a_Rect.Left() + a_Rect.Width() * Pivot[0],
                a_Rect.Top() + a_Rect.Height() * Pivot[1]
            };
        }

        /** 
         * @brief Converts the transformation to a 3x3 matrix for use in rendering. 
         * @param a_Rect The rectangle representing the area to which the transformation will be applied, used for resolving the pivot point.
         * @return A 3x3 matrix representing the combined transformation.
         */
        Mat3<Unit> ToMatrix( Rect<Unit> a_Rect ) const
        {
            if ( IsIdentity() )
                return c_Identity<Mat3<Unit>>;

            // --- Translation ---
            const Mat3<Unit> trans = Mat3<Unit>::from_columns(
                Vec3<Unit>{ 1_u, 0_u, 0_u },
                Vec3<Unit>{ 0_u, 1_u, 0_u },
                Vec3<Unit>{ Translation[0], Translation[1], 1_u }
            );

            // --- Pivot translate ---
            const Vec2<Unit> pivot = ResolvePivot( a_Rect );
            const Mat3<Unit> transPiv = Mat3<Unit>::from_columns(
                Vec3<Unit>{ 1_u, 0_u, 0_u },
                Vec3<Unit>{ 0_u, 1_u, 0_u },
                Vec3<Unit>{ pivot[0], pivot[1], 1_u }
            );

            const Mat3<Unit> transNegPiv = Mat3<Unit>::from_columns(
                Vec3<Unit>{ 1_u, 0_u, 0_u },
                Vec3<Unit>{ 0_u, 1_u, 0_u },
                Vec3<Unit>{ -pivot[0], -pivot[1], 1_u }
            );

            // --- Scale ---
            const Unit sx = Scale[0];
            const Unit sy = Scale[1];
            const Mat3<Unit> scale = Mat3<Unit>::from_columns(
                Vec3<Unit>{ sx, 0_u, 0_u },
                Vec3<Unit>{ 0_u, sy, 0_u },
                Vec3<Unit>{ 0_u, 0_u, 1_u }
            );

            // --- Rotation (clockwise, Y-down) ---
            const Unit c{ std::cos( Angle.Value.ToFloat() ) };
            const Unit s{ std::sin( Angle.Value.ToFloat() ) };
            const Mat3<Unit> rot = Mat3<Unit>::from_columns(
                Vec3<Unit>{  c, s, 0_u },
                Vec3<Unit>{ -s, c, 0_u },
                Vec3<Unit>{ 0_u, 0_u, 1_u }
            );

            // --- Skew ---
            const Unit kx{ std::tan( SkewX.Value.ToFloat() ) };
            const Unit ky{ std::tan( SkewY.Value.ToFloat() ) };
            const Mat3<Unit> skew = Mat3<Unit>::from_columns(
                Vec3<Unit>{ 1_u, ky, 0_u },
                Vec3<Unit>{ kx, 1_u, 0_u },
                Vec3<Unit>{ 0_u, 0_u, 1_u }
            );

            // Compose (column-major)
            return trans * transPiv * rot * skew * scale * transNegPiv;
        }
    };

} // namespace RatUI