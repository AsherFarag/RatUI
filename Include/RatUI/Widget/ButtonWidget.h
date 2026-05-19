#pragma once
#include "Scene.h"
#include "IWidget.h"

namespace RatUI
{
    /**
     * @brief
     */
    class ButtonWidget : public IWidget
    {
    public:
        Callback<Scene&, WidgetID> OnClick; ///< Callback that is invoked when the button is clicked.

        // TODO: Temp?
        Color          NormalColor{ Colors::Surface700 };
        Color          HoverColor{ Colors::Surface600 };
        Color          PressedColor{ Colors::Surface500 };
        Color          FocusOutlineColor{ Colors::White };
        CornerRounding Rounding{ CornerRounding::Uniform( 8_u ) };
        Unit           BorderThickness{ 1_u };

        ButtonWidget() = default;
        ButtonWidget( Callback<Scene&, WidgetID> a_OnClick ) : OnClick( std::move( a_OnClick ) ) {}

        virtual ~ButtonWidget() = default;

        bool IsFocusable( Scene& a_Scene ) const override { return true; }

        bool OnPressed( Scene& a_Scene, const ButtonEvent& a_Event ) override
        {
            if ( !a_Event.Pressed )
                return false;

            if ( IsActivationButton( a_Event.Button ) )
            {
                m_IsPressed = true;
                return true;
            }

            return false;
        }

        bool OnReleased( Scene& a_Scene, const ButtonEvent& a_Event ) override
        {
            if ( !a_Event.Released )
                return false;

            if ( !IsActivationButton( a_Event.Button ) )
                return false;

            const bool wasPressed = m_IsPressed;
            m_IsPressed = false;

            if ( wasPressed )
                Invoke( OnClick, a_Scene, GetID() );

            return true;
        }

        void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
        {
            const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
            if ( !node || !Visibility::IsRendered( node->Layout.Visibility ) )
                return;

            const Rect<Unit>& rect = node->Layout.FinalRect;
            const Color fill = m_IsPressed ? PressedColor : ( m_IsHovered ? HoverColor : NormalColor );

			// TODO: We shouldnt be doing drawing here, users should compose the style, like add a PanelWidget as a child and style that.
            if ( a_Scene.GetFocusedWidget() == GetID() )
            {
                a_DrawList.AddRect( rect,
                {
                    .FillColor = fill,
                    .BorderColor = Colors::White,
                    .BorderThickness = 2_u,
                    .Rounding = Rounding
                } );
            }
            else
            {
                a_DrawList.AddRect( rect,
                {
                    .FillColor = fill,
                    .Rounding = Rounding
                } );
            }

            a_DrawList.PushClipRect( rect );
            a_Scene.ForEachChildWidget( GetID(), [&]( IWidget& a_Child )
            {
                a_Child.OnPaint( a_Scene, a_DrawList );
            } );
            a_DrawList.PopClipRect();
        }

        void OnPointerEnter( Scene& a_Scene, const PointerEvent& a_Event ) override { m_IsHovered = true; }
        void OnPointerExit( Scene& a_Scene, const PointerEvent& a_Event ) override
        {
            m_IsHovered = false;
            m_IsPressed = false;
        }

        bool IsHovered() const { return m_IsHovered; }
        bool IsPressed() const { return m_IsPressed; }

    protected:

        // TODO: This shouldn't be handled here, what if users wanted to change the activation buttons or have multiple buttons with different activation buttons?
        static constexpr bool IsActivationButton( EButtonID a_Button )
        {
            return a_Button == EButtonID::MouseLeft || a_Button == EButtonID::KeyEnter;
        }

        bool m_IsHovered{ false };
        bool m_IsPressed{ false };
    };

} // namespace RatUI