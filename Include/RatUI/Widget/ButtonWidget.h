#pragma once
#include "Theme.h"
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
        ButtonBaseWidget( Shared<const Theme> a_Theme, Callback<Scene&, WidgetID> a_OnClick )
            : OnClick( std::move( a_OnClick ) )
        {
            (void)a_Theme;
        }

        virtual ~ButtonBaseWidget() = default;

        bool IsInteractable() const override { return true; }
        bool IsFocusable()    const override { return true; }

        Reply OnButtonPressed( const ButtonEvent& a_Event ) override
        {
            if ( !a_Event.Pressed )
                return Reply::Unhandled();

            if ( IsActivationButton( a_Event.Button ) )
            {
                m_IsPressed = true;
                return Reply::Handled();
            }

            return Reply::Unhandled();
        }

        Reply OnButtonReleased( const ButtonEvent& a_Event ) override
        {
            if ( !a_Event.Released )
                return Reply::Unhandled();

            if ( !IsActivationButton( a_Event.Button ) )
                return Reply::Unhandled();

            const bool wasPressed = m_IsPressed;
            m_IsPressed = false;

            if ( wasPressed )
                Invoke( OnClick, GetScene(), GetID() );

            return Reply::Handled();
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

        Reply OnPointerEnter( const PointerEvent& a_Event ) override
        {
            m_IsHovered = true;
            return Reply::Unhandled();
        }

        Reply OnPointerExit( const PointerEvent& a_Event ) override
        {
            m_IsHovered = false;
            m_IsPressed = false;
            return Reply::Unhandled();
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
        ButtonWidget() = default;

        ButtonWidget( Shared<const Theme> a_Theme )
        {
            m_Theme = std::move( a_Theme );
        }

        ButtonWidget( Shared<const Theme> a_Theme, Callback<Scene&, WidgetID> a_OnClick )
            : ButtonBaseWidget( a_Theme, std::move( a_OnClick ) )
        {
            m_Theme = std::move( a_Theme );
        }

        void SetTheme( Shared<const Theme> a_Theme )
        {
            m_Theme = std::move( a_Theme );
        }

        void OnPaint( DrawList& a_DrawList ) override
        {
            Scene& scene = GetScene();
            const LayoutNode* node = scene.Layouts.Get( GetLayoutID() );
            if ( !node || !Visibility::IsRendered( node->Layout.Visibility ) )
                return;

            const Rect<Unit>& rect = node->Layout.FinalRect;

			// Fill color based on state: pressed > hovered > normal
            const Color fill = m_IsPressed ? GetThemeColor( ThemeKey::Color::ButtonPressed, Colors::Surface500 )
                                           : ( m_IsHovered 
                                               ? GetThemeColor( ThemeKey::Color::ButtonHover, Colors::Surface600 )
                                               : GetThemeColor( ThemeKey::Color::ButtonNormal, Colors::Surface700 ) );

            a_DrawList.AddRect( rect,
            {
                .FillColor = fill,
                .BorderColor = GetThemeColor( ThemeKey::Color::ButtonHover, Colors::Transparent ),
                .BorderThickness = GetThemeMetric( ThemeKey::Metric::ButtonBorderThickness, 1_u ),
                .Rounding = GetThemeRounding( ThemeKey::Rounding::Button, CornerRounding::None() )
            } );

            // Draw focus ring 
            // TODO: Should this be a util or even handled here?
            if ( scene.GetFocusedWidget() == GetID() )
            {
                a_DrawList.AddRect( rect,
                {
                    .FillColor = Colors::Transparent,
                    .BorderColor = GetThemeColor( ThemeKey::Color::FocusOutline, Colors::White ),
                    .BorderThickness = GetThemeMetric( ThemeKey::Metric::FocusOutlineThickness, 2_u ),
                    .Rounding = GetThemeRounding( ThemeKey::Rounding::FocusOutline, CornerRounding::None() )
                } );
            }

            a_DrawList.PushClipRect( rect );
            scene.ForEachChildWidget( GetLayoutID(), [&]( IWidget& a_Child )
            {
                a_Child.OnPaint( a_DrawList );
            } );
            a_DrawList.PopClipRect();
        }

    private:
        Shared<const Theme> m_Theme;

        Color GetThemeColor( ThemeID a_ID, Color a_Default ) const
        {
            return m_Theme ? m_Theme->GetColor( a_ID, a_Default ) : a_Default;
        }

        CornerRounding GetThemeRounding( ThemeID a_ID, CornerRounding a_Default ) const
        {
            return m_Theme ? m_Theme->GetRounding( a_ID, a_Default ) : a_Default;
        }

        Unit GetThemeMetric( ThemeID a_ID, Unit a_Default ) const
        {
            return m_Theme ? m_Theme->GetMetric( a_ID, a_Default ) : a_Default;
        }

    };

} // namespace RatUI