#pragma once
#include "Scene.h"
#include "IWidget.h"

#include "Theme.h"
#include "FocusRing.h"

namespace RatUI
{
    /**
     * @brief A track with a draggable thumb.
     *
     * For keyboard and gamepad it is engaged to be adjusted (see IWidget::CanEngage): navigation stops on the slider,
     * Activate (Enter, A) engages it, and the arrows along the track then move the value until it is disengaged.
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
        FocusRingStyle FocusRing{};                                            ///< Drawn while focused (theme: FocusOutline keys).

        // --------------------------------------------------------------------
        // Behaviour properties
        // --------------------------------------------------------------------

        f32 Min       { 0.f };
        f32 Max       { 1.f };
        f32 Step      { 0.f };   ///< If > 0, the value snaps to Min plus multiples of Step, and arrow keys move one Step. If 0, arrow keys move 1% of the range.
        f32 ScrollStep{ 0.05f }; ///< Scroll wheel step.

        Observable<f32> Value{}; ///< Current value, clamped to [Min, Max].

        SliderWidget() : IWidget( ENavRole::Stop ) {}
        SliderWidget( f32 a_Min, f32 a_Max, f32 a_InitialValue = 0.f )
            : IWidget( ENavRole::Stop ), Min( a_Min ), Max( a_Max )
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
        bool CanEngage()      const override { return true; }

        void OnPaint( const PaintEvent& a_Event ) override
        {
            const Rect<Unit>& rect = GetLayout().Layout.FinalRect;

            SyncTheme();

            PaintTrack( a_Event.Drawer, rect );
            PaintThumb( a_Event.Drawer, rect );

            // Around the thumb while engaged, around the whole slider while navigation is on it.
            if ( IsEngaged() )
                FocusRing.Draw( a_Event.Drawer, GetThumbRect( rect ), ThumbRadius );
            else if ( GetScene().IsFocusVisible( GetLayoutID() ) )
                FocusRing.Draw( a_Event.Drawer, rect, ThumbRadius );
        }

        // --------------------------------------------------------------------
        // Input
        // --------------------------------------------------------------------

        Reply OnButtonPressed( const ButtonEvent& a_Event ) override
        {
            if ( !a_Event.Pressed )
                return Reply::Unhandled();

            // A pointer press begins a drag, which only that pointer moves and ends
            if ( a_Event.Button == EButtonID::MouseLeft && !m_IsDragging )
            {
                m_IsDragging = true;
                m_DragPointer = a_Event.Pointer.value_or( c_MousePointerID );
                if ( a_Event.PointerPosition )
                    m_LastPointerPos = *a_Event.PointerPosition;
                GetScene().CapturePointer( m_DragPointer, GetLayoutID() );
                UpdateFromPointer( GetLayout().Layout.FinalRect, m_LastPointerPos );

                return Reply::Handled();
            }

            // While engaged, the arrows along the track move the value. Down and right increase it, as the thumb moves.
            const bool      isHz = Orientation == EOrient::Horizontal;
            const EButtonID less = isHz ? EButtonID::KeyLeft : EButtonID::KeyUp;
            const EButtonID more = isHz ? EButtonID::KeyRight : EButtonID::KeyDown;
            if ( IsEngaged() && ( a_Event.Button == less || a_Event.Button == more ) )
            {
                StepValue( a_Event.Button == more );
                return Reply::Handled();
            }

            return Reply::Unhandled();
        }

        Reply OnButtonReleased( const ButtonEvent& a_Event ) override
        {
            if ( a_Event.Button == EButtonID::MouseLeft && m_IsDragging && a_Event.Pointer.value_or( c_MousePointerID ) == m_DragPointer )
            {
                m_IsDragging = false;
                GetScene().ReleasePointerCapture( m_DragPointer );
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
            if ( m_IsDragging && a_Event.ID != m_DragPointer )
                return Reply::Unhandled();

            m_LastPointerPos = a_Event.Position;

            if ( !m_IsDragging )
                return Reply::Unhandled();

            UpdateFromPointer( GetLayout().Layout.FinalRect, a_Event.Position );

            return Reply::Unhandled();
        }

        Reply OnPointerScroll( const PointerEvent& a_Event ) override
        {
            // Only once focused (e.g. clicked), so the wheel over a slider in scrolling content scrolls the content.
            if ( GetScene().GetFocusedNode() != GetLayoutID() )
                return Reply::Unhandled();

            // Horizontal: the sideways wheel if there is one, else the vertical wheel (up raises the value), so a plain
            // mouse wheel works. Vertical: like a scrollbar, wheel down moves the thumb down.
            const f32 dx    = a_Event.ScrollDelta[0].ToFloat();
            const f32 dy    = a_Event.ScrollDelta[1].ToFloat();
            const f32 delta = Orientation == EOrient::Horizontal ? ( dx != 0.f ? dx : dy ) : -dy;

            const f32 before = Value.Get();
            if ( delta != 0.f )
                Nudge( delta * ScrollStep );

            // Unmoved at either end, the scroll passes on to the widgets around it.
            return Value.Get() != before ? Reply::Handled() : Reply::Unhandled();
        }

    protected:

        bool m_IsDragging     { false };
        bool m_IsHovered      { false };
        bool m_IsThumbPressed { false };
        Vec2<Unit> m_LastPointerPos{ 0_u, 0_u };
        PointerID  m_DragPointer{ c_MousePointerID }; ///< The pointer dragging the slider, while m_IsDragging

        /**
         * @brief Moves to the next multiple of Step (from Min) up or down, so an off-step value lands back on the
         * steps, or by 1% of the range when Step is 0.
         */
        void StepValue( bool a_Up )
        {
            if ( Step <= 0.f )
            {
                Nudge( ( Max - Min ) * ( a_Up ? 0.01f : -0.01f ) );
                return;
            }

            constexpr f32 c_Epsilon = 1e-4f;
            const f32     index     = ( Value.Get() - Min ) / Step;
            const f32     next      = a_Up ? std::floor( index + 1.f + c_Epsilon ) : std::ceil( index - 1.f - c_Epsilon );
            Value.Set( std::clamp( Min + next * Step, Min, Max ) );
        }

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
                    FocusRing.UpdateFromTheme( Theme );
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

            // a_T is the thumb's [0, 1] position; Step applies to the value.
            const auto setFromPosition = [this]( f32 a_T )
            {
                f32 value = Min + std::clamp( a_T, 0.f, 1.f ) * ( Max - Min );
                if ( Step > 0.f )
                    value = Min + std::round( ( value - Min ) / Step ) * Step;
                Value.Set( std::clamp( value, Min, Max ) );
            };

            const Vec2<Unit> thumbSize = GetEffectiveThumbSize( a_Rect );

            if ( Orientation == EOrient::Horizontal )
            {
                const Unit travel = a_Rect.Width() - thumbSize[0];
                if ( travel <= 0_u ) return;

                const f32 t = ( a_Pos[0] - ( a_Rect.Left() + thumbSize[0] * 0.5f ) ).ToFloat()
                              / travel.ToFloat();
                setFromPosition( t );
            }
            else
            {
                const Unit travel = a_Rect.Height() - thumbSize[1];
                if ( travel <= 0_u ) return;

                const f32 t = ( a_Pos[1] - ( a_Rect.Top() + thumbSize[1] * 0.5f ) ).ToFloat()
                              / travel.ToFloat();
				setFromPosition( t );
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