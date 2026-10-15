#pragma once
#include "Scene.h"
#include "IWidget.h"

#include "Theme.h"

namespace RatUI
{
    /**
     * @brief
     */
    class SliderWidget : public IWidget
    {
    public:
        // --------------------------------------------------------------------
        // Render properties
        // --------------------------------------------------------------------

        EOrient Orientation   { EOrient::Horizontal };
        bool ShowTrackFill         { true };  ///< Whether to render the filled portion of the track.
        bool ScaleThumbOnTrackAxis { false }; ///< When enabled, thumb size is scaled along the scrolling axis by ThumbScale.
        f32  ThumbScale            { 1.f };   ///< [0,1] size ratio of the thumb along the scrolling axis when ScaleThumbOnTrackAxis is enabled.

        // --- Track properties ---

        Unit  TrackThickness         { 4_u };                                  ///< Thickness of the track across the scrolling axis.
        Brush TrackBrush             { SolidBrush{ Colors::Surface700 } };     ///< Brush of the track.
        Brush TrackFillBrush         { SolidBrush{ Colors::Transparent } };    ///< Brush of the filled portion of the track (Useful for progress bars).
        CornerRadius TrackRadius     { CornerRadius::All( 0_u ) };             ///< Corner rounding of the track.

        // --- Thumb properties ---

        Unit  ThumbSize              { 16_u };                                 ///< Size of the thumb (square) when not scaled along the track axis.
        Unit  MinThumbSize           { 8_u };                                  ///< Minimum thumb length along the track axis when ScaleThumbOnTrackAxis is enabled.
        Brush ThumbBrushNormal       { SolidBrush{ Colors::Surface500 } };     ///< Brush of the thumb.
        Brush ThumbBrushHover        { SolidBrush{ Colors::Surface600 } };     ///< Brush of the thumb when hovered.
        Brush ThumbBrushPressed      { SolidBrush{ Colors::Surface700 } };     ///< Brush of the thumb when pressed.
        Color ThumbBorderColor       { Colors::Transparent };                  ///< Border color of the thumb.
        Color ThumbBorderColorPressed{ Colors::AccentBlue };                   ///< Border color of the thumb while dragging.
        Unit  ThumbBorderThickness   { 0_u };                                  ///< Border thickness of the thumb.
        Unit  ThumbBorderThicknessPressed{ 2_u };                              ///< Border thickness of the thumb while dragging.
        CornerRadius ThumbRadius     { CornerRadius::All( 0_u ) };             ///< Corner rounding of the thumb.

        // --------------------------------------------------------------------
        // Behaviour properties
        // --------------------------------------------------------------------

        f32 Min       { 0.f };
        f32 Max       { 1.f };
        f32 Step      { 0.f };   ///< If > 0, the value will snap to multiples of Step between Min and Max.
        f32 ScrollStep{ 0.05f }; ///< Scroll wheel step.

        Observable<f32> Value{}; ///< Current value, clamped to [Min, Max].

        SliderWidget() = default;
        SliderWidget( f32 a_Min, f32 a_Max, f32 a_InitialValue = 0.f )
            : Min( a_Min ), Max( a_Max )
        {
            Value.Set( std::clamp( a_InitialValue, a_Min, a_Max ) );
        }

        /** @brief Set the normalised [0, 1] position of the thumb. */
        void SetNormalized( f32 a_T )
        {
            const f32 clamped = std::clamp( a_T, 0.f, 1.f );
            Value.Set( Min + clamped * ( Max - Min ) );
        }

        /** @brief Get the normalised [0, 1] position of the thumb. */
        f32 GetNormalized() const
        {
            const f32 range = Max - Min;
            if ( range <= 0.f ) return 0.f;
            return std::clamp( ( Value.Get() - Min ) / range, 0.f, 1.f );
        }

        /** @brief Nudge the value by @p a_Delta in value space. */
        void Nudge( f32 a_Delta )
        {
            Value.Set( std::clamp( Value.Get() + a_Delta, Min, Max ) );
        }

        // --------------------------------------------------------------------
        // IWidget Overrides
        // --------------------------------------------------------------------

        bool IsInteractable() const override { return true; }
        bool IsFocusable()    const override { return true; }

        void OnPaint( const PaintEvent& a_Event ) override
        {
            const Rect<Unit>& rect = GetLayout().Layout.FinalRect;

            SyncTheme();

            PaintTrack( a_Event.Drawer, rect );
            PaintThumb( a_Event.Drawer, rect );
        }

        // --------------------------------------------------------------------
        // Input
        // --------------------------------------------------------------------

        Reply OnButtonPressed( const ButtonEvent& a_Event ) override
        {
            if ( !a_Event.Pressed )
                return Reply::Unhandled();

            // Mouse button begins a drag
            if ( a_Event.Button == EButtonID::MouseLeft )
            {
                m_IsDragging = true;
                GetScene().CapturePointer( GetLayoutID() );
                UpdateFromPointer( GetLayout().Layout.FinalRect, m_LastPointerPos );
                
                return Reply::Handled();
            }

            return Reply::Unhandled();
        }

        Reply OnButtonReleased( const ButtonEvent& a_Event ) override
        {
            if ( a_Event.Button == EButtonID::MouseLeft && m_IsDragging )
            {
                m_IsDragging = false;
                GetScene().ReleasePointerCapture();
                return Reply::Handled();
            }
            
            return Reply::Unhandled();
        }

        Reply OnPointerEnter( const PointerEvent& a_Event ) override
        {
            m_IsHovered = true;
            m_LastPointerPos = a_Event.Position;
            return Reply::Unhandled();
        }

        Reply OnPointerExit( const PointerEvent& a_Event ) override
        {
            m_IsHovered = false;
            if ( !m_IsDragging )
                m_IsThumbPressed = false;
            return Reply::Unhandled();
        }

        Reply OnPointerMove( const PointerEvent& a_Event ) override
        {
            m_LastPointerPos = a_Event.Position;

            if ( !m_IsDragging )
                return Reply::Unhandled();

            UpdateFromPointer( GetLayout().Layout.FinalRect, a_Event.Position );

            return Reply::Unhandled();
        }

        Reply OnPointerScroll( const PointerEvent& a_Event ) override
        {
            const bool isHz = ( Orientation == EOrient::Horizontal );
            const f32  delta = isHz
                ? a_Event.ScrollDelta[0].ToFloat()
				: a_Event.ScrollDelta[1].ToFloat() * -1.f; // Invert vertical scroll to match typical scrollbar behaviour.

            if ( delta != 0.f )
                Nudge( delta * ScrollStep );

            return Reply::Unhandled();
        }

    protected:

        bool m_IsDragging     { false };
        bool m_IsHovered      { false };
        bool m_IsThumbPressed { false };
        Vec2<Unit> m_LastPointerPos{ 0_u, 0_u };

        /** @brief Pulls the cached render properties from the theme if it changed. */
        void SyncTheme()
        {
            if constexpr ( HasMixin<ThemeMixin> )
            {
                if ( Theme.Update() )
                {
                    TrackThickness = Theme.GetMetric( ThemeKey::Metric::SliderTrackThickness, TrackThickness );
                    TrackBrush = Theme.GetBrush( ThemeKey::Brush::SliderTrack, TrackBrush );
                    TrackFillBrush = Theme.GetBrush( ThemeKey::Brush::SliderTrackFill, TrackFillBrush );
                    TrackRadius = Theme.GetRadius( ThemeKey::Radii::SliderTrack, TrackRadius );

                    ThumbSize = Theme.GetMetric( ThemeKey::Metric::SliderThumbSize, ThumbSize );
                    MinThumbSize = Theme.GetMetric( ThemeKey::Metric::SliderMinThumbSize, MinThumbSize );
                    ThumbBrushNormal = Theme.GetBrush( ThemeKey::Brush::SliderThumb, ThumbBrushNormal );
                    ThumbBrushHover = Theme.GetBrush( ThemeKey::Brush::SliderThumbHover, ThumbBrushHover );
                    ThumbBrushPressed = Theme.GetBrush( ThemeKey::Brush::SliderThumbPressed, ThumbBrushPressed );
                    ThumbBorderColor = Theme.GetColor( ThemeKey::Color::SliderThumbBorder, ThumbBorderColor );
                    ThumbBorderColorPressed = Theme.GetColor( ThemeKey::Color::SliderThumbBorderPressed, ThumbBorderColorPressed );
                    ThumbBorderThickness = Theme.GetMetric( ThemeKey::Metric::SliderThumbBorderThickness, ThumbBorderThickness );
                    ThumbBorderThicknessPressed = Theme.GetMetric( ThemeKey::Metric::SliderThumbBorderThicknessPressed, ThumbBorderThicknessPressed );
                    ThumbRadius = Theme.GetRadius( ThemeKey::Radii::SliderThumb, ThumbRadius );
                }
            }
        }

        /** @brief Returns the track rect centred inside @p a_Rect. */
        Rect<Unit> GetTrackRect( Rect<Unit> a_Rect ) const
        {
            if ( Orientation == EOrient::Horizontal )
            {
                const Unit cy = a_Rect.Origin[1] + a_Rect.Size[1] * 0.5f;
                return {
                    { a_Rect.Left(), cy - TrackThickness * 0.5f },
                    { a_Rect.Width(), TrackThickness }
                };
            }
            else
            {
                const Unit cx = a_Rect.Origin[0] + a_Rect.Size[0] * 0.5f;
                return {
                    { cx - TrackThickness * 0.5f, a_Rect.Top() },
                    { TrackThickness, a_Rect.Height() }
                };
            }
        }

        /** @brief Returns the rect of the thumb for the current value. */
        Rect<Unit> GetThumbRect( Rect<Unit> a_Rect ) const
        {
            const f32 t = GetNormalized();
            const Vec2<Unit> thumbSize = GetEffectiveThumbSize( a_Rect );

            Vec2<Unit> centre;

            if ( Orientation == EOrient::Horizontal )
            {
                // Reserve half-thumb width at each end so thumb never clips.
                const Unit travel = a_Rect.Width() - thumbSize[0];
                centre = {
                    a_Rect.Left() + thumbSize[0] * 0.5f + travel * t,
                    a_Rect.Origin[1] + a_Rect.Size[1] * 0.5f
                };
            }
            else
            {
                const Unit travel = a_Rect.Height() - thumbSize[1];
                centre = {
                    a_Rect.Origin[0] + a_Rect.Size[0] * 0.5f,
                    a_Rect.Top() + thumbSize[1] * 0.5f + travel * t
                };
            }

            return {
                centre - thumbSize * 0.5_u,
                thumbSize
            };
        }

        void PaintTrack( DrawList& a_DrawList, const Rect<Unit>& a_Rect ) const
        {
            const Rect<Unit> track = GetTrackRect( a_Rect );

            a_DrawList.AddBrushRect( track, TrackBrush, Colors::Transparent, 0_u, TrackRadius );

            if ( !ShowTrackFill )
                return;

            // Filled portion (min -> thumb)
            const f32 t = GetNormalized();
            if ( t > 0.f )
            {
                Rect<Unit> filled = track;
                if ( Orientation == EOrient::Horizontal )
                    filled.Size[0] = track.Size[0] * t;
                else
                    filled.Size[1] = track.Size[1] * t;

                a_DrawList.AddBrushRect( filled, TrackFillBrush, Colors::Transparent, 0_u, TrackRadius );
            }
        }

        void PaintThumb( DrawList& a_DrawList, const Rect<Unit>& a_Rect ) const
        {
            const Rect<Unit> thumb = GetThumbRect( a_Rect );

            const Brush& fill = m_IsDragging ? ThumbBrushPressed : ( m_IsHovered ? ThumbBrushHover : ThumbBrushNormal );

            a_DrawList.AddBrushRect( thumb, fill,
                                     m_IsDragging ? ThumbBorderColorPressed : ThumbBorderColor,
                                     m_IsDragging ? ThumbBorderThicknessPressed : ThumbBorderThickness,
                                     ThumbRadius );
        }

        // ---------------------------------------------------------------------
        // Drag logic
        // ---------------------------------------------------------------------

        void UpdateFromPointer( const Rect<Unit>& a_Rect, Vec2<Unit> a_Pos )
        {
            SyncTheme(); // Input can arrive before the first paint or after a theme swap.

			const auto stepValue = [=]( f32 a_Value ) -> f32
			{
				if ( Step <= 0.f ) return a_Value;
				return std::round( a_Value / Step ) * Step;
			};

            const Vec2<Unit> thumbSize = GetEffectiveThumbSize( a_Rect );

            if ( Orientation == EOrient::Horizontal )
            {
                const Unit travel = a_Rect.Width() - thumbSize[0];
                if ( travel <= 0_u ) return;

                const f32 t = ( a_Pos[0] - ( a_Rect.Left() + thumbSize[0] * 0.5f ) ).ToFloat()
                              / travel.ToFloat();
                SetNormalized( stepValue( t ) );
            }
            else
            {
                const Unit travel = a_Rect.Height() - thumbSize[1];
                if ( travel <= 0_u ) return;

                const f32 t = ( a_Pos[1] - ( a_Rect.Top() + thumbSize[1] * 0.5f ) ).ToFloat()
                              / travel.ToFloat();
				SetNormalized( stepValue( t ) );
            }
        }

        Vec2<Unit> GetEffectiveThumbSize( const Rect<Unit>& a_Rect ) const
        {
            Vec2<Unit> thumbSize = { ThumbSize, ThumbSize };
            if ( !ScaleThumbOnTrackAxis )
                return thumbSize;

            const f32 clampedScale = std::clamp( ThumbScale, 0.f, 1.f );
            if ( Orientation == EOrient::Horizontal )
            {
                const Unit scaledSize = Unit( a_Rect.Width().ToFloat() * clampedScale );
                thumbSize[0] = std::max( MinThumbSize, std::min( a_Rect.Width(), scaledSize ) );
            }
            else
            {
                const Unit scaledSize = Unit( a_Rect.Height().ToFloat() * clampedScale );
                thumbSize[1] = std::max( MinThumbSize, std::min( a_Rect.Height(), scaledSize ) );
            }

            return thumbSize;
        }
    };

} // namespace RatUI