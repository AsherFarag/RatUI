#pragma once
#include "Scene.h"

namespace RatUI
{
    /**
     * @brief A base widget that provides common button functionality, such as handling pointer events and click callbacks.
     * This class can be extended to create various types of buttons with different visual styles and behaviors.
     */
    class ButtonBaseWidget : public IWidget
    {
    public:
        Callback<Scene&, WidgetID> OnClick; ///< Callback that is invoked when the button is clicked.

        ButtonBaseWidget() = default;
        ButtonBaseWidget( Callback<Scene&, WidgetID> a_OnClick ) : OnClick( std::move( a_OnClick ) ) {}

        virtual ~ButtonBaseWidget() = default;

        bool IsFocusable() const override { return true; }

        bool OnPressed( const ButtonEvent& a_Event ) override
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

        bool OnReleased( const ButtonEvent& a_Event ) override
        {
            if ( !a_Event.Released )
                return false;

            if ( !IsActivationButton( a_Event.Button ) )
                return false;

            const bool wasPressed = m_IsPressed;
            m_IsPressed = false;

            if ( wasPressed )
                Invoke( OnClick, GetScene(), GetID() );

            return true;
        }

        void OnPaint( DrawList& a_DrawList ) override
        {
            Scene& scene = GetScene();
            const LayoutNode* node = scene.Layouts.Get( GetLayoutID() );
            if ( !node || !Visibility::IsRendered( node->Layout.Visibility ) )
                return;

            scene.ForEachChildWidget( GetLayoutID(), [&]( IWidget& a_Child )
            {
                a_Child.OnPaint( a_DrawList );
            } );
        }

        void OnPointerEnter( const PointerEvent& a_Event ) override { m_IsHovered = true; }
        void OnPointerExit( const PointerEvent& a_Event ) override
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

    /**
     * @brief A standard button widget that extends ButtonBaseWidget with a default visual style. 
     * It renders a rectangular button with different colors for normal, hovered, and pressed states, as well as an outline when focused. 
     * The button's appearance can be customized through its public properties.
     */
    class ButtonWidget : public ButtonBaseWidget
    {
    public:
        Color          NormalColor{ Colors::Surface700 };
        Color          HoverColor{ Colors::Surface600 };
        Color          PressedColor{ Colors::Surface500 };
        Color          FocusOutlineColor{ Colors::White };
        CornerRounding Rounding{ CornerRounding::Uniform( 8_u ) };
        Unit           BorderThickness{ 1_u };

        using ButtonBaseWidget::ButtonBaseWidget;

        void OnPaint( DrawList& a_DrawList ) override
        {
            Scene& scene = GetScene();
            const LayoutNode* node = scene.Layouts.Get( GetLayoutID() );
            if ( !node || !Visibility::IsRendered( node->Layout.Visibility ) )
                return;

            const Rect<Unit>& rect = node->Layout.FinalRect;
            const Color fill = m_IsPressed ? PressedColor 
                                           : ( m_IsHovered ? HoverColor : NormalColor );

            if ( scene.GetFocusedWidget() == GetID() )
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
            scene.ForEachChildWidget( GetLayoutID(), [&]( IWidget& a_Child )
            {
                a_Child.OnPaint( a_DrawList );
            } );
            a_DrawList.PopClipRect();
        }

    };

} // namespace RatUI