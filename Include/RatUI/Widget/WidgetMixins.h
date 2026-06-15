#pragma once
#include "../Layout/Layout.h"
#include "../Renderer/RenderTransform.h"
#include "../Renderer/DrawList.h"

namespace RatUI
{
    struct WidgetMixinBase
    {
        bool CanPaint( LayoutNode& ) const { return true; }
        void PrePaint( DrawList&, LayoutNode& ) {}
        void PostPaint( DrawList&, LayoutNode& ) {}
    };

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

        void PrePaint( DrawList& a_DrawList, LayoutNode& a_Node )
        {
            ( Mixins::PrePaint( a_DrawList, a_Node ), ... );
        }

        void PostPaint( DrawList& a_DrawList, LayoutNode& a_Node )
        {
            ( Mixins::PostPaint( a_DrawList, a_Node ), ... );
        }

    };

    struct RenderTransformMixin : WidgetMixinBase
    {
        RenderTransform Transform{};

        void PrePaint( DrawList& a_DrawList, LayoutNode& a_Node )
        {
            if ( !Transform.IsIdentity() )
            {
                a_DrawList.PushTransform( Transform.ToMatrix( a_Node.Layout.FinalRect ) );
                m_Pushed++;
            }
                
        }

        void PostPaint( DrawList& a_DrawList, LayoutNode& a_Node )
        {
            if ( m_Pushed > 0 )
            {
                a_DrawList.PopTransform();
                m_Pushed--;

                RATUI_USER_ASSERT( m_Pushed >= 0, "Mismatched Push/PopTransform calls in RenderTransformMixin." );
            }
        }

    private:
        i32 m_Pushed{ 0 };
    };

    struct AnimationMixin : WidgetMixinBase
    {
        // TODO: Finish the animation system
    };

	struct DebugMixin : WidgetMixinBase
    {
        String DebugName;

        void PostPaint( DrawList& a_DrawList, LayoutNode& a_Node )
        {
            if ( !a_DrawList.IsDebugEnabled() )
                return;

			constexpr Color c_BoundsColor = Colors::Red;

            // Draw bounds
            a_DrawList.AddRect( a_Node.Layout.FinalRect,
            {
                .BorderColor = c_BoundsColor,
                .BorderThickness = 1_u
            } );

            // Draw
			a_DrawList.AddCircle( a_Node.Layout.FinalRect.Center(), 4_u,
			{
				.FillColor = c_BoundsColor
			} );
        }
    };

    using DefaultWidgetMixins = WidgetMixins<RenderTransformMixin, AnimationMixin, DebugMixin>;

};