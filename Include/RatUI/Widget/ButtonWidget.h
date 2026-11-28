#pragma once
#include "Theme.h"
#include "Scene.h"
#include "FocusRing.h"

namespace RatUI
{
    /**
     * @brief A base widget that provides common button functionality, such as handling pointer events and click callbacks.
     * This class can be extended to create various types of buttons with different visual styles and behaviors.
     */
    class ButtonBaseWidget : public IWidget
    {
    public:
        using Super = IWidget;
        static constexpr bool c_HasChildren = true;

		using OnClickCallback = Callback<ButtonBaseWidget&>;
        OnClickCallback OnClick; ///< Callback that is invoked when the button is clicked.

        ButtonBaseWidget( OnClickCallback a_OnClick = {} )
            : IWidget( { .Focus = EFocusPolicy::All, .Interactable = true } ), OnClick( std::move( a_OnClick ) )
        {}

        virtual ~ButtonBaseWidget() = default;

        Reply OnButtonPressed( const ButtonEvent& a_Event ) override
        {
            if ( !a_Event.Pressed || a_Event.Button != EButtonID::MouseLeft )
                return Reply::Unhandled();

            m_IsPressed    = true;
            m_PressPointer = a_Event.Pointer.value_or( c_MousePointerID );
            return Reply::Handled();
        }

        Reply OnButtonReleased( const ButtonEvent& a_Event ) override
        {
            if ( !a_Event.Released || a_Event.Button != EButtonID::MouseLeft )
                return Reply::Unhandled();

            // The pressing pointer keeps the button while held, so it may be released somewhere else.
            if ( a_Event.PointerPosition && !IsOver( *a_Event.PointerPosition ) )
                m_IsPressed = false;

            return Release();
        }

        Reply OnPointerMove( const PointerEvent& a_Event ) override
        {
            // Touches have no hover, so leaving is seen here.
            if ( m_PressPointer && *m_PressPointer == a_Event.ID && !IsOver( a_Event.Position ) )
                m_IsPressed = false;
            return Reply::Unhandled();
        }

        void OnPressCancelled( const PressCancelEvent& a_Event ) override
        {
            if ( a_Event.Pointer != m_PressPointer )
                return;
            m_IsPressed    = false;
            m_PressPointer = NullOpt;
        }

        Reply OnActivate( const ActivateEvent& a_Event ) override
        {
            if ( !a_Event.Pressed )
                return Release();

            m_IsPressed    = true;
            m_PressPointer = NullOpt;
            return Reply::Handled();
        }

        Reply OnPointerEnter( const PointerEvent& a_Event ) override
        {
            m_IsHovered = true;
            return Reply::Unhandled();
        }

        Reply OnPointerExit( const PointerEvent& a_Event ) override
        {
            m_IsHovered = false;
            // Only the pointer that pressed the button cancels its press by leaving; another pointer passing by doesn't.
            if ( m_PressPointer && *m_PressPointer == a_Event.ID )
                m_IsPressed = false;
            return Reply::Unhandled();
        }

        bool IsHovered() const { return m_IsHovered; }
        bool IsPressed() const { return m_IsPressed; }

    protected:

        /** @brief Ends a press, clicking if it wasn't cancelled (e.g. by the pointer leaving). */
        Reply Release()
        {
            const bool wasPressed = m_IsPressed;
            m_IsPressed    = false;
            m_PressPointer = NullOpt;

            if ( wasPressed )
                Invoke( OnClick, *this );

            return Reply::Handled();
        }

        bool IsOver( Vec2<Unit> a_Position ) const { return GetScene().IsOver( GetLayoutID(), a_Position ); }

        bool m_IsHovered{ false };
        bool m_IsPressed{ false };
        Optional<PointerID> m_PressPointer{}; ///< The pointer that pressed the button; empty for a key.
    };

    /**
     * @brief A standard button widget that extends ButtonBaseWidget with a default visual style. 
     * It renders a rectangular button with different colors for normal, hovered, and pressed states, as well as an outline when focused. 
     * The button's appearance can be customized through its public properties.
     */
    class ButtonWidget : public ButtonBaseWidget
    {
    public:
        using Super = ButtonBaseWidget;

        ButtonWidget( OnClickCallback a_OnClick = {} )
			: ButtonBaseWidget( std::move( a_OnClick ) )
        {}

        // --------------------------------------------------------------------
        // Render Properties
        // --------------------------------------------------------------------

        Brush NormalBrush{ SolidBrush{ Colors::Surface700 } };  ///< The brush used to fill the button's background in its normal state
        Brush HoverBrush{ SolidBrush{ Colors::Surface600 } };   ///< The brush used to fill the button's background when hovered
        Brush PressedBrush{ SolidBrush{ Colors::Surface500 } }; ///< The brush used to fill the button's background when pressed
        Color BorderColor{ Colors::Transparent };               ///< The color of the panel's border
        Unit  BorderThickness{ 0_u };                           ///< The thickness of the panel's border
        CornerRadius Radius{ CornerRadius::None() };            ///< The corner rounding
        FocusRingStyle FocusRing{};                             ///< Drawn while the widget has focus (theme: FocusOutline keys)

        // --------------------------------------------------------------------
        // IWidget Overrides
        // --------------------------------------------------------------------

        void OnPaint( const PaintEvent& a_Event ) override
        {
            Scene& scene = GetScene();
            const LayoutNode& node = GetLayout();
            const Rect<Unit>& rect = node.Layout.SceneRect;

            if constexpr ( HasMixin<ThemeMixin> )
            {
                if ( Theme.Update() )
                {
                    NormalBrush = Theme.GetBrush( ThemeKey::Brush::ButtonNormal, NormalBrush );
                    HoverBrush = Theme.GetBrush( ThemeKey::Brush::ButtonHover, HoverBrush );
                    PressedBrush = Theme.GetBrush( ThemeKey::Brush::ButtonPressed, PressedBrush );
                    BorderColor = Theme.GetColor( ThemeKey::Color::ButtonBorder, BorderColor );
                    BorderThickness = Theme.GetMetric( ThemeKey::Metric::ButtonBorderThickness, BorderThickness );
                    Radius = Theme.GetRadius( ThemeKey::Radii::Button, Radius );
                    FocusRing.UpdateFromTheme( Theme );
                }
            }

			// Fill brush based on state: pressed > hovered > normal
            const Brush& fillBrush = m_IsPressed ? PressedBrush : ( m_IsHovered ? HoverBrush : NormalBrush );

            a_Event.Drawer.AddBrushRect( rect, fillBrush, BorderColor, BorderThickness, Radius );

            if ( scene.IsFocusVisible( GetLayoutID() ) )
                FocusRing.Draw( a_Event.Drawer, rect, Radius );

            PaintChildren( a_Event );
        }
    };

} // namespace RatUI