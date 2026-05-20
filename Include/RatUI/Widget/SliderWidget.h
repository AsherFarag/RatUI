#pragma once
#include "Scene.h"
#include "IWidget.h"

namespace RatUI
{

    class SliderWidget : public IWidget
    {
    public:
        EOrientation   Orientation{ EOrientation::Horizontal }; ///< Orientation of the slider, either horizontal or vertical.
        Unit           TrackThickness{ 4_u };                   ///< Thickness of the slider track.
        CornerRounding TrackRounding{ 2_u };                    ///< Rounding of the slider track corners.
        Color          TrackColor{ Colors::Surface600 };        ///< Color of the slider track.

        Vec2<Unit>     ThumbSize{ 12_u, 12_u };          ///< Size of the slider thumb.
        CornerRounding ThumbRounding{ 6_u };             ///< Rounding of the slider thumb corners.
        Color          ThumbColor{ Colors::Surface500 }; ///< Color of the slider thumb.

        Observable<f32> Value{}; ///< The current value of the slider, typically in the range [0.0, 1.0]. 

        bool IsFocusable( Scene& a_Scene ) const override { return true; }

        void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
        {
            const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
            if ( !node || !Visibility::IsRendered( node->Layout.Visibility ) )
                return;

            const Rect<Unit>& rect = node->Layout.FinalRect;

            if ( a_Scene.GetFocusedWidget() == GetID() )
            {
				a_DrawList.AddRect( rect,
				{
					.BorderColor = Colors::White,
					.BorderThickness = 2_u,
					.Rounding = TrackRounding
				} );
            }

            DrawTrack( a_DrawList, rect );
            DrawThumb( a_DrawList, rect, false );
        }

    protected:

        void DrawTrack( DrawList& a_DrawList, Rect<Unit> a_Rect ) const
        {
            Rect<Unit> trackRect;
            if ( Orientation == EOrientation::Horizontal )
            {
                trackRect = {
                    .Origin = { a_Rect.Left(), a_Rect.Center()[1] - TrackThickness / 2 },
                    .Size   = { a_Rect.Width(), TrackThickness }
                };
            }
            else
            {
                trackRect = {
                    .Origin = { a_Rect.Center()[0] - TrackThickness / 2, a_Rect.Top() },
                    .Size   = { TrackThickness, a_Rect.Height() }
                };
            }

            a_DrawList.AddRect( trackRect,
            {
                .FillColor = TrackColor,
                .Rounding  = TrackRounding
            } );
        }

        void DrawThumb( DrawList& a_DrawList, Rect<Unit> a_Rect, bool a_ThumbFocused ) const
        {
            Vec2<Unit> thumbCenter = a_Rect.Center();

            const f32 clampedValue = std::clamp( Value.Get(), 0.f, 1.f );

            if ( Orientation == EOrientation::Horizontal )
            {
                Unit x = a_Rect.Left() + a_Rect.Width() * clampedValue;
                thumbCenter = { x, a_Rect.Center()[1] };
            }
            else
            {
                Unit y = a_Rect.Top() + a_Rect.Height() * clampedValue;
                thumbCenter = { a_Rect.Center()[0], y };
            }

            Rect<Unit> thumbRect{
                thumbCenter - ThumbSize / 2_u,
                ThumbSize
            };

            if ( a_ThumbFocused )
            {
                a_DrawList.AddRect( thumbRect,
                {
                    .FillColor = ThumbColor,
                    .BorderColor = Colors::White,
                    .BorderThickness = 2_u,
                    .Rounding  = ThumbRounding
                } );
            }
            else
            {
                a_DrawList.AddRect( thumbRect,
                {
                    .FillColor = ThumbColor,
                    .Rounding  = ThumbRounding
                } );
            }
            
        }

    };

} // namespace RatUI
