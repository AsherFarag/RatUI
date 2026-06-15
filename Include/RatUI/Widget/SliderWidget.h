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
        // =====================================================================
        // Behaviour properties
        // =====================================================================

        EOrientation   Orientation     { EOrientation::Horizontal };
        bool           ShowTrackFill   { true };               ///< Whether to render the filled portion of the track.

        bool           ScaleThumbOnTrackAxis { false }; ///< When enabled, thumb size is scaled along the scrolling axis by ThumbScale.
        f32            ThumbScale      { 1.f };         ///< [0,1] size ratio of the thumb along the scrolling axis when ScaleThumbOnTrackAxis is enabled.

        // =====================================================================
        // Behaviour properties
        // =====================================================================

        f32 Min       { 0.f };
        f32 Max       { 1.f };
        f32 Step      { 0.f };   ///< If > 0, the value will snap to multiples of Step between Min and Max.
        f32 ScrollStep{ 0.05f }; ///< Scroll wheel step.

        Observable<f32> Value{}; ///< Current value, clamped to [Min, Max].

        SliderWidget() = default;

        SliderWidget( ThemeHandle a_Theme )
            : m_Theme( std::move( a_Theme ) ) 
        {}

        SliderWidget( ThemeHandle a_Theme, f32 a_Min, f32 a_Max, f32 a_InitialValue = 0.f )
            : Min( a_Min ), Max( a_Max ), m_Theme( std::move( a_Theme ) )
        {
            Value.Set( std::clamp( a_InitialValue, a_Min, a_Max ) );
        }

        const ThemeHandle& GetTheme() const { return m_Theme; }
        void SetTheme( ThemeHandle a_Theme ) { m_Theme = std::move( a_Theme ); }

        bool IsInteractable() const override { return true; }
        bool IsFocusable()    const override { return true; }

        void OnPaint( DrawList& a_DrawList ) override
        {
            const Rect<Unit>& rect = GetLayout().Layout.FinalRect;

            PaintTrack( a_DrawList, rect );
            PaintThumb( a_DrawList, rect );
        }

        // =====================================================================
        // Input
        // =====================================================================

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

        // =====================================================================
        // Input: pointer
        // =====================================================================

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

            const LayoutNode* node = GetScene().Layouts.Get( GetLayoutID() );
            if ( node )
                UpdateFromPointer( node->Layout.FinalRect, a_Event.Position );

            return Reply::Unhandled();
        }

        Reply OnPointerScroll( const PointerEvent& a_Event ) override
        {
            const bool isHz = ( Orientation == EOrientation::Horizontal );
            const f32  delta = isHz
                ? a_Event.ScrollDelta[0].ToFloat()
				: a_Event.ScrollDelta[1].ToFloat() * -1.f; // Invert vertical scroll to match typical scrollbar behaviour.

            if ( delta != 0.f )
                Nudge( delta * ScrollStep );

            return Reply::Unhandled();
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

    protected:

        bool m_IsDragging     { false };
        bool m_IsHovered      { false };
        bool m_IsThumbPressed { false };
        Vec2<Unit>  m_LastPointerPos{ 0_u, 0_u };
        ThemeHandle m_Theme;

        /** @brief Returns the track rect centred inside @p a_Rect. */
        Rect<Unit> GetTrackRect( Rect<Unit> a_Rect ) const
        {
			const Unit trackThickness = m_Theme.GetMetric( ThemeKey::Metric::SliderTrackThickness, 4_u );
            if ( Orientation == EOrientation::Horizontal )
            {
                const Unit cy = a_Rect.Origin[1] + a_Rect.Size[1] * 0.5f;
                return {
                    { a_Rect.Left(), cy - trackThickness * 0.5f },
                    { a_Rect.Width(), trackThickness }
                };
            }
            else
            {
                const Unit cx = a_Rect.Origin[0] + a_Rect.Size[0] * 0.5f;
                return {
                    { cx - trackThickness * 0.5f, a_Rect.Top() },
                    { trackThickness, a_Rect.Height() }
                };
            }
        }

        /** @brief Returns the rect of the thumb for the current value. */
        Rect<Unit> GetThumbRect( Rect<Unit> a_Rect ) const
        {
            const f32 t = GetNormalized();
            const Vec2<Unit> thumbSize = GetEffectiveThumbSize( a_Rect );

            Vec2<Unit> centre;

            if ( Orientation == EOrientation::Horizontal )
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
            const Rect<Unit> track             = GetTrackRect( a_Rect );
            const Unit trackThickness          = m_Theme.GetMetric( ThemeKey::Metric::SliderTrackThickness, 4_u );
            const Color trackColor             = m_Theme.GetColor( ThemeKey::Color::SliderTrack, Colors::Transparent );
            const Color trackFillColor         = m_Theme.GetColor( ThemeKey::Color::SliderTrackFill, Colors::Transparent );
            const CornerRounding trackRounding = m_Theme.GetRounding( ThemeKey::Rounding::SliderTrack, CornerRounding::Uniform( 0_u ) );

            // Background track
            a_DrawList.AddRect( track,
            {
                .FillColor = trackColor,
                .Rounding  = trackRounding
            } );

            if ( !ShowTrackFill )
                return;

            // Filled portion (min -> thumb)
            const f32 t = GetNormalized();
            if ( t > 0.f )
            {
                Rect<Unit> filled = track;
                if ( Orientation == EOrientation::Horizontal )
                    filled.Size[0] = track.Size[0] * t;
                else
                    filled.Size[1] = track.Size[1] * t;

                a_DrawList.AddRect( filled,
                {
                    .FillColor = trackFillColor,
                    .Rounding  = trackRounding
                } );
            }
        }

        void PaintThumb( DrawList& a_DrawList, const Rect<Unit>& a_Rect ) const
        {
            const Rect<Unit> thumb             = GetThumbRect( a_Rect );
            const Vec2<Unit> thumbSize         = GetThemeThumbSize( a_Rect );
            const Color thumbColor             = m_Theme.GetColor( ThemeKey::Color::SliderThumb, Colors::White );
            const Color thumbHoverColor        = m_Theme.GetColor( ThemeKey::Color::SliderThumbHover, Colors::LightGray );
            const Color thumbPressColor        = m_Theme.GetColor( ThemeKey::Color::SliderThumbPressed, Colors::Gray );
            const CornerRounding thumbRounding = m_Theme.GetRounding( ThemeKey::Rounding::SliderThumb, CornerRounding::Uniform( 8_u ) );

            const Color fill = m_IsThumbPressed ? thumbPressColor
                             : m_IsHovered      ? thumbHoverColor
                                                : thumbColor;

            a_DrawList.AddRect( thumb,
            {
                .FillColor       = fill,
                .BorderColor     = m_IsDragging ? Colors::AccentBlue : Colors::Transparent,
                .BorderThickness = m_IsDragging ? 2_u : 0_u,
                .Rounding        = thumbRounding
            } );
        }

        // ---------------------------------------------------------------------
        // Drag logic
        // ---------------------------------------------------------------------

        void UpdateFromPointer( const Rect<Unit>& a_Rect, Vec2<Unit> a_Pos )
        {
			const auto stepValue = [=]( f32 a_Value ) -> f32
			{
				if ( Step <= 0.f ) return a_Value;
				return std::round( a_Value / Step ) * Step;
			};

            const Vec2<Unit> thumbSize = GetEffectiveThumbSize( a_Rect );

            if ( Orientation == EOrientation::Horizontal )
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
            Vec2<Unit> thumbSize = GetThemeThumbSize( a_Rect );
            if ( !ScaleThumbOnTrackAxis )
                return thumbSize;

            const f32 clampedScale = std::clamp( ThumbScale, 0.f, 1.f );
            if ( Orientation == EOrientation::Horizontal )
            {
                const Unit scaledSize = Unit( a_Rect.Width().ToFloat() * clampedScale );
                thumbSize[0] = std::max( m_Theme.GetMetric( ThemeKey::Metric::SliderMinThumbSize, 8_u ), std::min( a_Rect.Width(), scaledSize ) );
            }
            else
            {
                const Unit scaledSize = Unit( a_Rect.Height().ToFloat() * clampedScale );
                thumbSize[1] = std::max( m_Theme.GetMetric( ThemeKey::Metric::SliderMinThumbSize, 8_u ), std::min( a_Rect.Height(), scaledSize ) );
            }

            return thumbSize;
        }

        Vec2<Unit> GetThemeThumbSize( const Rect<Unit>& ) const
        {
            return { 16_u, 16_u };
        }
    };

} // namespace RatUI