#pragma once
#include "Scene.h"
#include "IWidget.h"

namespace RatUI
{
    /**
     * @brief A clipped container that can scroll its children on either axis.
     */
    class ScrollContainerWidget : public IWidget
    {
    public:
        Callback<Scene&, WidgetID, Vec2<Unit>> OnScrollChanged;

        bool IsFocusableByDefault{ true };
        bool ConsumeScrollInput{ true };
        bool AllowPointerWheel{ true };
        bool AllowKeyboardScroll{ true };
        bool EnableHorizontalScroll{ false };
        bool EnableVerticalScroll{ true };
        bool DrawBackground{ false };
        bool ShowHorizontalScrollbar{ true };
        bool ShowVerticalScrollbar{ true };

        Unit WheelStepX{ 48_u };
        Unit WheelStepY{ 48_u };
        Unit KeyboardStepX{ 24_u };
        Unit KeyboardStepY{ 24_u };
        f32  PageScrollFactor{ 0.9f };

        Unit ScrollbarThickness{ 8_u };
        Unit ScrollbarMargin{ 2_u };
        Unit ScrollbarMinThumbSize{ 20_u };

        Color          BackgroundColor{ Colors::Surface700 };
        Color          BorderColor{ Colors::Surface500 };
        Unit           BorderThickness{ 0_u };
        CornerRounding Rounding{ CornerRounding::Uniform( 6_u ) };
        Color          FocusOutlineColor{ Colors::White };
        Unit           FocusOutlineThickness{ 2_u };
        Color          ScrollbarTrackColor{ FromColorF32( 1.f, 1.f, 1.f, 0.12f ) };
        Color          ScrollbarThumbColor{ FromColorF32( 1.f, 1.f, 1.f, 0.45f ) };

        Vec2<Unit> GetScrollOffset() const    { return m_ScrollOffset; }
        Vec2<Unit> GetMaxScrollOffset() const { return m_MaxScrollOffset; }
        Vec2<Unit> GetContentSize() const     { return m_ContentSize; }
        Vec2<Unit> GetViewportSize() const    { return m_ViewportSize; }

        bool SetScrollOffset( Scene& a_Scene, Vec2<Unit> a_Offset )
        {
            UpdateScrollMetrics( a_Scene );
            return SetScrollOffsetInternal( a_Scene, ClampOffset( a_Offset ), true );
        }

        bool ScrollBy( Scene& a_Scene, Vec2<Unit> a_Delta )
        {
            UpdateScrollMetrics( a_Scene );
            return SetScrollOffsetInternal( a_Scene, m_ScrollOffset + a_Delta, true );
        }

        bool ScrollToStart( Scene& a_Scene )
        {
            return SetScrollOffset( a_Scene, Vec2<Unit>{ 0_u, 0_u } );
        }

        bool ScrollToEnd( Scene& a_Scene )
        {
            UpdateScrollMetrics( a_Scene );
            return SetScrollOffset( a_Scene, m_MaxScrollOffset );
        }

        bool IsFocusable( Scene& a_Scene ) const override { return IsFocusableByDefault; }

        bool OnPointerScroll( Scene& a_Scene, const PointerEvent& a_Event ) override
        {
            if ( !AllowPointerWheel )
                return false;

            Vec2<Unit> delta{ 0_u, 0_u };
            if ( EnableHorizontalScroll && !IsApproxEqual( a_Event.ScrollDelta[0].ToFloat(), 0.f ) )
                delta[0] = Unit{ -a_Event.ScrollDelta[0].ToFloat() * WheelStepX.ToFloat() };

            if ( EnableVerticalScroll && !IsApproxEqual( a_Event.ScrollDelta[1].ToFloat(), 0.f ) )
                delta[1] = Unit{ -a_Event.ScrollDelta[1].ToFloat() * WheelStepY.ToFloat() };

            if ( IsApproxEqual( delta[0].ToFloat(), 0.f ) && IsApproxEqual( delta[1].ToFloat(), 0.f ) )
                return false;

            const bool changed = ScrollBy( a_Scene, delta );
            return changed && ConsumeScrollInput;
        }

        bool OnPressed( Scene& a_Scene, const ButtonEvent& a_Event ) override
        {
            if ( !AllowKeyboardScroll || !a_Event.Pressed )
                return false;

            UpdateScrollMetrics( a_Scene );

            Vec2<Unit> target = m_ScrollOffset;
            bool handled = true;

            switch ( a_Event.Button )
            {
                case EButtonID::KeyUp:       target[1] -= KeyboardStepY; break;
                case EButtonID::KeyDown:     target[1] += KeyboardStepY; break;
                case EButtonID::KeyLeft:     target[0] -= KeyboardStepX; break;
                case EButtonID::KeyRight:    target[0] += KeyboardStepX; break;
                case EButtonID::KeyHome:     target = Vec2<Unit>{ 0_u, 0_u }; break;
                case EButtonID::KeyEnd:      target = m_MaxScrollOffset; break;
                case EButtonID::KeyPageUp:   target[1] -= m_ViewportSize[1] * PageScrollFactor; break;
                case EButtonID::KeyPageDown: target[1] += m_ViewportSize[1] * PageScrollFactor; break;
                default: handled = false; break;
            }

            if ( !handled )
                return false;

            target = ClampOffset( target );
            const bool changed = SetScrollOffsetInternal( a_Scene, target, true );
            return changed && ConsumeScrollInput;
        }

        void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
        {
            const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
            if ( !node || !Visibility::IsRendered( node->Layout.Visibility ) )
                return;

            UpdateScrollMetrics( a_Scene );

            const Rect<Unit>& rect = node->Layout.FinalRect;

            if ( DrawBackground || BorderThickness > 0_u )
                a_DrawList.AddRect( BackgroundColor, rect, Rounding, BorderThickness, BorderColor );

            if ( a_Scene.GetFocusedWidget() == GetID() && FocusOutlineThickness > 0_u )
                a_DrawList.AddRectBorder( FocusOutlineColor, rect.Expanded( 2_u ), Rounding + 2_u, FocusOutlineThickness );

            a_DrawList.PushClipRect( rect );

            const bool hasTranslation = !IsApproxEqual( m_ScrollOffset[0].ToFloat(), 0.f ) || !IsApproxEqual( m_ScrollOffset[1].ToFloat(), 0.f );
            if ( hasTranslation )
            {
                const Mat3<Unit> translation = Mat3<Unit>::from_columns(
                    Vec3<Unit>{ 1_u, 0_u, 0_u },
                    Vec3<Unit>{ 0_u, 1_u, 0_u },
                    Vec3<Unit>{ -m_ScrollOffset[0], -m_ScrollOffset[1], 1_u } );
                a_DrawList.PushTransform( translation );
            }

            a_Scene.ForEachChildWidget( GetID(), [&]( IWidget& a_Child )
            {
                a_Child.OnPaint( a_Scene, a_DrawList );
            });

            if ( hasTranslation )
                a_DrawList.PopTransform();

            DrawScrollbars( a_DrawList, rect );

            a_DrawList.PopClipRect();
        }

    private:
        Vec2<Unit> m_ScrollOffset{ 0_u, 0_u };
        Vec2<Unit> m_MaxScrollOffset{ 0_u, 0_u };
        Vec2<Unit> m_ContentSize{ 0_u, 0_u };
        Vec2<Unit> m_ViewportSize{ 0_u, 0_u };

        Vec2<Unit> ClampOffset( Vec2<Unit> a_Offset ) const
        {
            if ( !EnableHorizontalScroll )
                a_Offset[0] = 0_u;
            else
                a_Offset[0] = Math::Clamp( a_Offset[0], 0_u, m_MaxScrollOffset[0] );

            if ( !EnableVerticalScroll )
                a_Offset[1] = 0_u;
            else
                a_Offset[1] = Math::Clamp( a_Offset[1], 0_u, m_MaxScrollOffset[1] );

            return a_Offset;
        }

        bool SetScrollOffsetInternal( Scene& a_Scene, Vec2<Unit> a_NewOffset, bool a_EmitEvent )
        {
            a_NewOffset = ClampOffset( a_NewOffset );
            if ( a_NewOffset == m_ScrollOffset )
                return false;

            m_ScrollOffset = a_NewOffset;
            if ( a_EmitEvent )
                Invoke( OnScrollChanged, a_Scene, GetID(), m_ScrollOffset );

            return true;
        }

        void UpdateScrollMetrics( Scene& a_Scene )
        {
            const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
            if ( !node )
            {
                m_ContentSize = { 0_u, 0_u };
                m_ViewportSize = { 0_u, 0_u };
                m_MaxScrollOffset = { 0_u, 0_u };
                m_ScrollOffset = { 0_u, 0_u };
                return;
            }

            const Rect<Unit>& viewportRect = node->Layout.FinalRect;

            m_ViewportSize = Vec2<Unit>{
                std::max( 0_u, viewportRect.Size[0] ),
                std::max( 0_u, viewportRect.Size[1] )
            };

            Vec2<Unit> contentMax{ 0_u, 0_u };
            a_Scene.ForEachChildWidget( GetID(), [&]( IWidget& a_Child )
            {
                const LayoutNode* childNode = a_Scene.Layouts.Get( a_Child.GetLayoutID() );
                if ( !childNode || !Visibility::AffectsLayout( childNode->Layout.Visibility ) )
                    return;

                const Rect<Unit>& childRect = childNode->Layout.FinalRect;
                contentMax[0] = std::max( contentMax[0], std::max( 0_u, childRect.Right() - viewportRect.Left() ) );
                contentMax[1] = std::max( contentMax[1], std::max( 0_u, childRect.Bottom() - viewportRect.Top() ) );
            });

            m_ContentSize = contentMax;
            m_MaxScrollOffset = Vec2<Unit>{
                EnableHorizontalScroll ? std::max( 0_u, m_ContentSize[0] - m_ViewportSize[0] ) : 0_u,
                EnableVerticalScroll ? std::max( 0_u, m_ContentSize[1] - m_ViewportSize[1] ) : 0_u
            };

            m_ScrollOffset = ClampOffset( m_ScrollOffset );
        }

        void DrawScrollbars( DrawList& a_DrawList, const Rect<Unit>& a_Rect ) const
        {
            const Unit thickness = std::max( 0_u, ScrollbarThickness );
            const Unit margin = std::max( 0_u, ScrollbarMargin );
            const Unit minThumb = std::max( 0_u, ScrollbarMinThumbSize );
            const CornerRounding round = CornerRounding::Uniform( thickness * 0.5f );

            if ( thickness <= 0_u )
                return;

            if ( EnableVerticalScroll && ShowVerticalScrollbar && m_MaxScrollOffset[1] > 0_u )
            {
                Rect<Unit> track{
                    Vec2<Unit>{ a_Rect.Right() - margin - thickness, a_Rect.Top() + margin },
                    Vec2<Unit>{ thickness, std::max( 0_u, a_Rect.Size[1] - margin * 2.f ) }
                };

                if ( track.Size[1] > 0_u )
                {
                    const f32 ratio = m_ContentSize[1].ToFloat() > 0.f
                        ? std::min( 1.f, m_ViewportSize[1].ToFloat() / m_ContentSize[1].ToFloat() )
                        : 1.f;

                    Unit thumbSize = std::max( minThumb, track.Size[1] * ratio );
                    thumbSize = std::min( thumbSize, track.Size[1] );
                    const Unit travel = std::max( 0_u, track.Size[1] - thumbSize );

                    const f32 t = m_MaxScrollOffset[1].ToFloat() > 0.f
                        ? Math::Clamp( m_ScrollOffset[1].ToFloat() / m_MaxScrollOffset[1].ToFloat(), 0.f, 1.f )
                        : 0.f;

                    Rect<Unit> thumb{
                        Vec2<Unit>{ track.Origin[0], track.Origin[1] + travel * t },
                        Vec2<Unit>{ track.Size[0], thumbSize }
                    };

                    a_DrawList.AddRect( ScrollbarTrackColor, track, round );
                    a_DrawList.AddRect( ScrollbarThumbColor, thumb, round );
                }
            }

            if ( EnableHorizontalScroll && ShowHorizontalScrollbar && m_MaxScrollOffset[0] > 0_u )
            {
                Rect<Unit> track{
                    Vec2<Unit>{ a_Rect.Left() + margin, a_Rect.Bottom() - margin - thickness },
                    Vec2<Unit>{ std::max( 0_u, a_Rect.Size[0] - margin * 2.f ), thickness }
                };

                if ( track.Size[0] > 0_u )
                {
                    const f32 ratio = m_ContentSize[0].ToFloat() > 0.f
                        ? std::min( 1.f, m_ViewportSize[0].ToFloat() / m_ContentSize[0].ToFloat() )
                        : 1.f;

                    Unit thumbSize = std::max( minThumb, track.Size[0] * ratio );
                    thumbSize = std::min( thumbSize, track.Size[0] );
                    const Unit travel = std::max( 0_u, track.Size[0] - thumbSize );

                    const f32 t = m_MaxScrollOffset[0].ToFloat() > 0.f
                        ? Math::Clamp( m_ScrollOffset[0].ToFloat() / m_MaxScrollOffset[0].ToFloat(), 0.f, 1.f )
                        : 0.f;

                    Rect<Unit> thumb{
                        Vec2<Unit>{ track.Origin[0] + travel * t, track.Origin[1] },
                        Vec2<Unit>{ thumbSize, track.Size[1] }
                    };

                    a_DrawList.AddRect( ScrollbarTrackColor, track, round );
                    a_DrawList.AddRect( ScrollbarThumbColor, thumb, round );
                }
            }
        }
    };

} // namespace RatUI
