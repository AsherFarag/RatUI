#pragma once
#include "../Core.h"
#include "../Layout/Layout.h"

#include <ranges>
#include <limits>

namespace RatUI
{
    /**
     * @brief Represents a navigation action for moving focus between widgets in the UI.
     * This is used for keyboard and gamepad navigation.
     */
    enum class ENavAction : u8
    {
        None = 0,

        MoveLeft,
        MoveRight,
        MoveUp,
        MoveDown,

		ActivatePressed,  ///< Sent when the activation button/key is initially pressed, e.g., XBOX A button, Enter key, etc.
        ActivateReleased, ///< Sent when the activation button/key is released after being pressed.
        Cancel,           ///< Cancel or go back, e.g., XBOX B button, Escape key, etc.
    };

    /**
     * @brief Finds the most suitable navigatable LayoutNode in the specified direction from the starting node, based on their layout positions.
     * @tparam Range A range of LayoutNode references to consider as navigation targets. Must be a range of const LayoutNode*.
     * @param a_Action The navigation action indicating the direction to move (e.g., MoveLeft, MoveRight, etc.).
     * @param a_Start The starting LayoutNode from which to navigate. This is typically the currently focused node.
     * @param a_Nodes A range of 'focusable' LayoutNodes to consider as potential navigation targets.
     * @param a_LateralPenaltyWeight A weight factor that determines how much to penalize candidates that are laterally offset from the ideal navigation direction.
     *                               Higher values will prefer candidates that are more directly in the navigation direction, while lower values will allow for more diagonal movement.
     * 
     */
    template<std::ranges::input_range Range> 
    requires std::convertible_to<std::ranges::range_value_t<Range>, const LayoutNode*>
    const LayoutNode* FindNavigatableNode( 
        ENavAction a_Action,
        LayoutNode* a_Start, 
        Range&& a_Nodes,
        f32 a_LateralPenaltyWeight = 2.f )
    {
        if ( !a_Start )
    		return nullptr; // Invalid starting node

        // Determine the preferred direction vector
        Vec2<Unit> dir{};
        switch ( a_Action )
        {
            case ENavAction::MoveLeft:  dir = { -1_u,  0_u }; break;
            case ENavAction::MoveRight: dir = {  1_u,  0_u }; break;
            case ENavAction::MoveUp:    dir = {  0_u, -1_u }; break;
            case ENavAction::MoveDown:  dir = {  0_u,  1_u }; break;
    		default:                    return nullptr; // Unsupported action
        }

        const Vec2<Unit> startCenter = a_Start->Layout.FinalRect.Center();

    	const LayoutNode* best = nullptr;
        f32 bestScore = Limits<f32>::max();
        
        for ( const LayoutNode* candidate : a_Nodes )
        {
            if ( candidate == a_Start ) continue;
        
    		const Vec2<Unit> candidateCenter = candidate->Layout.FinalRect.Center();
            const Vec2<Unit> delta = candidateCenter - startCenter;
        
    		// Reject candidates not in the movement direction
            const Unit forward = Math::Dot( delta, dir );
            if ( forward <= 0_u ) continue;
        
    		// Score candidates based on a combination of forward distance and lateral deviation
            const Vec2<Unit> lateralVec = delta - dir * forward;
            const f32 lateral = Math::Length( Vec2f{ lateralVec[0].ToFloat(), lateralVec[1].ToFloat() } );
            const f32 score   = forward.ToFloat() + lateral * a_LateralPenaltyWeight;
        
            if ( score < bestScore ) 
            { 
                bestScore = score;
                best = candidate; 
            }
        }

        return best;
    }

    /**
     * @brief Represents the result of a navigation attempt and determines how the focus should be handled.
     */
    class NavigationReply
    {
    public:
        enum class EBoundaryRule : u8
        {
            Escape,   ///< Focus can leave this boundary normally. Default.
            Stop,     ///< Trap focus - wrap around within this boundary.
            Explicit, ///< Redirect focus to a specific widget.
            Custom,   ///< Invoke a callback to decide at runtime.
        };
    
        static NavigationReply Escape()
        {
            return NavigationReply{ EBoundaryRule::Escape };
        }
    
        static NavigationReply Stop()
        {
            return NavigationReply{ EBoundaryRule::Stop };
        }
    
        static NavigationReply Explicit( WidgetID a_Target )
        {
            NavigationReply r{ EBoundaryRule::Explicit };
            r.m_ExplicitTarget = a_Target;
            return r;
        }
    
        static NavigationReply Custom( Callback<ENavAction, WidgetID /*current*/> a_Handler )
        {
            NavigationReply r{ EBoundaryRule::Custom };
            r.m_CustomHandler = std::move( a_Handler );
            return r;
        }
    
        EBoundaryRule GetRule()           const { return m_Rule; }
        WidgetID      GetExplicitTarget() const { return m_ExplicitTarget; }
    
        WidgetID ResolveCustom( ENavAction a_Action, WidgetID a_Current ) const
        {
            // TODO: I was stupid and thought Callback<> wouldnt need a return type. Upgrade Callback to support returns.
            if ( m_CustomHandler )
                /*return*/ m_CustomHandler( a_Action, a_Current );

            return c_InvalidWidgetID;
        }
    
    private:
        explicit NavigationReply( EBoundaryRule a_Rule ) : m_Rule( a_Rule ) {}
    
        Callback<ENavAction, WidgetID>           m_CustomHandler;
        WidgetID                                 m_ExplicitTarget{ c_InvalidWidgetID };
        EBoundaryRule                            m_Rule{ EBoundaryRule::Escape };
    };

} // namespace RatUI
