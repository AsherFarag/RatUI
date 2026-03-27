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
        Vec2f    Translation{ 0.f, 0.f };    ///< The amount to translate the rendered content by. In pixels.
        Vec2f    Scale{ 1.f, 1.f };          ///< The amount to scale the rendered content by. 1x means no scaling, 2x means double size, etc.
        Vec2f    Pivot{ 0.5f, 0.5f };        ///< The pivot point around which to apply the transformations. (0, 0) Top-left corner, (1, 1) bottom-right corner, etc.
        Radiansf SkewX{ 0.f }, SkewY{ 0.f }; ///< The amount to shear the rendered content by along the X and Y axes, respectively. In radians.
        Radiansf Angle{ 0.f };               ///< The angle to rotate the rendered content by. In radians and clockwise direction.

        constexpr bool operator==( const RenderTransform& ) const = default;

        /** @brief Returns the identity transformation. */
        static constexpr RenderTransform Identity() { return RenderTransform{}; }

        /** @brief Checks if the transformation is the identity transformation. */
        constexpr bool IsIdentity() const { return *this == Identity(); }

        /** @brief Resolves the pivot point to an absolute position based on a given rectangle. */
        constexpr Vec2f ResolvePivot( Rectf a_Rect ) const
        {
            return Vec2f{
                a_Rect.Left() + a_Rect.Width() * Pivot[0],
                a_Rect.Top() + a_Rect.Height() * Pivot[1]
            };
        }

        /** 
         * @brief Converts the transformation to a 3x3 matrix for use in rendering. 
         * @param a_Rect The rectangle representing the area to which the transformation will be applied, used for resolving the pivot point.
         * @return A 3x3 matrix representing the combined transformation.
         */
        Mat3f ToMatrix( Rectf rect ) const
        {
            if (IsIdentity()) // Optimization for the common case of no transformation, to avoid unnecessary calculations.
                return c_Identity<Mat3f>;

            const Vec2f pivot = ResolvePivot(rect);

            const f32 c = std::cos(Angle.Value);
            const f32 s = std::sin(Angle.Value);

            const f32 sx = Scale[0];
            const f32 sy = Scale[1];

            const f32 kx = std::tan(SkewX.Value);
            const f32 ky = std::tan(SkewY.Value);

            // ---- Build linear 2x2 part ----
            // Rotation (clockwise, Y-down):
            //
            // [  c   -s ]
            // [  s    c ]
            //
            // Then scale
            // Then skew
            //
            // Final derived linear matrix:

            const f32 m00 =  sx * ( c + kx * s );
            const f32 m01 =  sy * ( kx * c - s );
            const f32 m10 =  sx * ( s + ky * c );
            const f32 m11 =  sy * ( ky * s + c );

            // ---- Pivot compensation ----
            //
            // We applied linear transform around origin.
            // To rotate/scale around pivot:
            //
            // finalTranslation =
            //   Translation
            // + pivot
            // - M * pivot

            const Vec2f transformedPivot{
                m00 * pivot[0] + m01 * pivot[1],
                m10 * pivot[0] + m11 * pivot[1]
            };

            const Vec2f finalTranslation = Translation + pivot - transformedPivot;

            // TODO: Need to check if the math lib is row or column major and adjust the matrix construction accordingly. Assuming column major for now.
            return Mat3f::from_columns(
                Vec3f{ m00, m10, 0.f },
                Vec3f{ m01, m11, 0.f },
                Vec3f{ finalTranslation[0], finalTranslation[1], 1.f }
            );
        }
    };

} // namespace RatUI