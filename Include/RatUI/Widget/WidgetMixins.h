#pragma once
#include "../Animation/Animation.h"
#include "../Layout/Layout.h"
#include "../Renderer/RenderTransform.h"
#include "../Renderer/DrawList.h"
#include "Theme.h"

namespace RatUI
{
    struct FocusEvent 
    {
        // TODO: Add more stuff here
        // TODO: Should this be in a different file?
    };

    struct PaintEvent
    {
        DrawList& Drawer;       ///< The draw list to which the widget should add its rendering commands.
        f32       DeltaSeconds; ///< The time in seconds since the last paint event, useful for animations and time-based effects.
    };

    /**
     * @brief WidgetMixinBase is a base class for all widget mixins. 
     * Mixins are a way to compose additional functionality into widgets, 
     * allowing you to pick and choose which features you want for each widget without needing a complex inheritance hierarchy.
     */
    struct WidgetMixinBase
    {
        /** 
         * @brief Determines whether this widget can be painted based on the given layout node. 
         * @param a_Node The layout node associated with the widget being painted.
         * @return true if the widget should be painted, false otherwise.
         */
        bool CanPaint( LayoutNode& ) const { return true; }

        /** 
         * @brief Called before 'OnPaint' is called on the widget, allowing the mixin to modify the draw list or perform other actions. 
         * @param a_DrawList The draw list that will be used for rendering the widget. Mixins can push transforms, set styles, etc. on this draw list.
         * @param a_Node The layout node associated with the widget being painted, providing access to layout information and properties.
         */
        void PrePaint( const PaintEvent&, LayoutNode& ) {}

        /** 
         * @brief Called after 'OnPaint' is called on the widget, allowing the mixin to clean up any state or perform additional drawing. 
         * @param a_DrawList The draw list that was used for rendering the widget. Mixins can use this to pop transforms, draw debug information, etc.
         * @param a_Node The layout node associated with the widget being painted, providing access to layout information and properties.
         */
        void PostPaint( const PaintEvent&, LayoutNode& ) {}
    };

    /**
     * @brief WidgetMixins is a variadic template that allows for composing multiple mixin classes into a single widget class.
     */
    template<std::derived_from<WidgetMixinBase>... Mixins>
    class WidgetMixins : public Mixins...
    {
    public:
        template<std::derived_from<WidgetMixinBase> MixinType>
        static inline constexpr bool HasMixin = ( std::derived_from<MixinType, Mixins> || ... );

        bool CanPaint( LayoutNode& node ) const
        {
            return ( ... && Mixins::CanPaint( node ) );
        }

        void PrePaint( const PaintEvent& a_Event, LayoutNode& a_Node )
        {
            ( Mixins::PrePaint( a_Event, a_Node ), ... );
        }

        void PostPaint( const PaintEvent& a_Event, LayoutNode& a_Node )
        {
            ( Mixins::PostPaint( a_Event, a_Node ), ... );
        }
    };

    /**
     * @brief Adds support for applying a render transform to a widget.
     * It automatically pushes the appropriate transformation matrix to the draw list before painting and pops it afterward, 
     * ensuring that the transform is applied correctly to all drawing operations performed by the widget and its children.
     */
    struct RenderTransformMixin : WidgetMixinBase
    {
        RenderTransform Transform{};

        void PrePaint( const PaintEvent& a_Event, LayoutNode& a_Node )
        {
            if ( !Transform.IsIdentity() )
            {
                a_Event.Drawer.PushTransform( Transform.ToMatrix( a_Node.Layout.FinalRect ) );
                m_Pushed++;
            }  
        }

        void PostPaint( const PaintEvent& a_Event, LayoutNode& a_Node )
        {
            if ( m_Pushed > 0 )
            {
                a_Event.Drawer.PopTransform();
                m_Pushed--;
                RATUI_USER_ASSERT( m_Pushed >= 0, "Mismatched Push/PopTransform calls in RenderTransformMixin." );
            }
        }

    private:
        i32 m_Pushed{ 0 };
    };

    /**
     * @brief Adds animation capabilities to a widget.
     * It maintains an AnimationPlayer instance that is ticked during the PrePaint phase, 
     * allowing any animations associated with the widget to progress over time.
     */
    struct AnimationMixin : WidgetMixinBase
    {
        AnimationPlayer Animator;

        void PrePaint( const PaintEvent& a_Event, LayoutNode& a_Node )
        {
            Animator.Tick( a_Event.DeltaSeconds );
        }
    };

    struct ThemeMixin : WidgetMixinBase
    {
        ThemeHandle Theme;
    };

    /**
     * @brief A mixin that adds debug drawing capabilities to a widget. 
     * When enabled, it draws the widget's bounds and center point, which can be useful for visualizing layout and debugging rendering issues. 
     * The debug drawing is only performed if the draw list has debug mode enabled.
     */
	struct DebugMixin : WidgetMixinBase
    {
        String DebugName; ///< An optional name for the widget that can be displayed in debug mode to help identify it.

        void PostPaint( const PaintEvent& a_Event, LayoutNode& a_Node )
        {
            if ( !a_Event.Drawer.IsDebugEnabled() )
                return;

			constexpr Color c_BoundsColor = Colors::Red;

            // Draw bounds
            a_Event.Drawer.AddRect( a_Node.Layout.FinalRect,
            {
                .BorderColor = c_BoundsColor,
                .BorderThickness = 1_u
            } );

            // Draw center point
			a_Event.Drawer.AddCircle( a_Node.Layout.FinalRect.Center(), 4_u,
			{
				.FillColor = c_BoundsColor
			} );
        }
    };

    /**
     * @brief DefaultWidgetMixins is a convenient alias for a common set of mixins that are applied to all widgets.
     * IWidget inherits from this type.
     */
    using DefaultWidgetMixins = WidgetMixins<
        RenderTransformMixin, 
        AnimationMixin,
        ThemeMixin
        #if RATUI_DEBUG
        , DebugMixin
        #endif
    >;

} // namespace RatUI