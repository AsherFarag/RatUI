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
		using OnClickCallback = Callback<ButtonBaseWidget&>;
        OnClickCallback OnClick; ///< Callback that is invoked when the button is clicked.

        ButtonBaseWidget() = default;
        ButtonBaseWidget( OnClickCallback a_OnClick )
            : OnClick( std::move( a_OnClick ) )
        {}

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
                Invoke( OnClick, *this );

            return Reply::Handled();
        }

        void OnPaint( DrawList& a_DrawList ) override
        {
            GetScene().ForEachChildWidget( GetLayoutID(), [&](IWidget& a_Child)
            {
                a_Child.Paint( a_DrawList );
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

        ButtonWidget( ThemeHandle a_Theme, OnClickCallback a_OnClick = {} )
			: ButtonBaseWidget( std::move( a_OnClick ) ), m_Theme( std::move( a_Theme ) )
        {}

        void SetTheme( ThemeHandle a_Theme )
        {
            m_Theme = std::move( a_Theme );
        }

        void OnPaint( DrawList& a_DrawList ) override
        {
            Scene& scene = GetScene();
            const LayoutNode& node = GetLayout();

            const Rect<Unit>& rect = node.Layout.FinalRect;

			// Fill brush based on state: pressed > hovered > normal
            const Brush& fillBrush = m_IsPressed ? m_Theme.GetBrush( ThemeKey::Brush::ButtonPressed, SolidBrush{ Colors::Surface500 } )
                                                 : ( m_IsHovered 
                                                     ? m_Theme.GetBrush( ThemeKey::Brush::ButtonHover,  SolidBrush{ Colors::Surface600 } )
                                                     : m_Theme.GetBrush( ThemeKey::Brush::ButtonNormal, SolidBrush{ Colors::Surface700 } ) );

            if ( std::holds_alternative<SolidBrush>( fillBrush ) )
            {
                const SolidBrush& solid = std::get<SolidBrush>( fillBrush );
                a_DrawList.AddRect( rect, 
                {
                    .FillColor = solid.Fill,
                    .BorderColor = m_Theme.GetColor( ThemeKey::Color::ButtonBorder, Colors::Transparent ),
                    .BorderThickness = m_Theme.GetMetric( ThemeKey::Metric::ButtonBorderThickness, 0_u ),
                    .Rounding = m_Theme.GetRounding( ThemeKey::Rounding::Button, CornerRounding::None() )
                } );
            }
            else if ( std::holds_alternative<TextureBrush>( fillBrush ) )
            {
                const TextureBrush& texture = std::get<TextureBrush>( fillBrush );
                a_DrawList.AddRect( rect, 
                {
                    .FillColor = texture.Tint,
                    .BorderColor = m_Theme.GetColor( ThemeKey::Color::ButtonBorder, Colors::Transparent ),
                    .BorderThickness = m_Theme.GetMetric( ThemeKey::Metric::ButtonBorderThickness, 0_u ),
                    .Rounding = m_Theme.GetRounding( ThemeKey::Rounding::Button, CornerRounding::None() ),
                    .Texture = texture.Texture
                } );
            }
            else if ( std::holds_alternative<NineSliceBrush>( fillBrush ) )
            {
                const NineSliceBrush& nineSlice = std::get<NineSliceBrush>( fillBrush );
                a_DrawList.AddSlicedRect( rect, 
                {
                    .Texture = nineSlice.Texture,
                    .Slice = nineSlice.Slice,
                    .Tint = nineSlice.Tint,
                } );
            }

            // Draw focus ring 
            // TODO: Should this be a util or even handled here?
            if ( scene.GetFocusedNode() == GetLayoutID() )
            {
                a_DrawList.AddRect( rect,
                {
                    .FillColor = Colors::Transparent,
                    .BorderColor = m_Theme.GetColor( ThemeKey::Color::FocusOutline, Colors::White ),
                    .BorderThickness = m_Theme.GetMetric( ThemeKey::Metric::FocusOutlineThickness, 2_u ),
                    .Rounding = m_Theme.GetRounding( ThemeKey::Rounding::FocusOutline, CornerRounding::None() )
                } );
            }

            scene.ForEachChildWidget( GetLayoutID(), [&]( IWidget& a_Child )
            {
                a_Child.Paint( a_DrawList );
            } );
        }

    private:
        ThemeHandle m_Theme;
    };

} // namespace RatUI