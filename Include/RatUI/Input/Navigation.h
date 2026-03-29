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

        Activate,  ///< Activate the currently focused item, e.g., XBox A button, Enter key, etc.
        Cancel,    ///< Cancel or go back, e.g., XBox B button, Escape key, etc.
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
        Vec2f dir{};
        switch ( a_Action )
        {
            case ENavAction::MoveLeft:  dir = { -1,  0 }; break;
            case ENavAction::MoveRight: dir = {  1,  0 }; break;
            case ENavAction::MoveUp:    dir = {  0, -1 }; break;
            case ENavAction::MoveDown:  dir = {  0,  1 }; break;
    		default:                    return nullptr; // Unsupported action
        }

        const Vec2f startCenter = a_Start->Layout.FinalRect.Center();

    	const LayoutNode* best = nullptr;
        f32 bestScore = Limits<f32>::max();

        for ( const LayoutNode* candidate : a_Nodes )
        {
            if ( candidate == a_Start ) continue;

    		Vec2f candidateCenter = candidate->Layout.FinalRect.Center();
            Vec2f delta = candidateCenter - startCenter;

    		// Reject candidates not in the movement direction
    		const f32 forward = nm::dot( delta, dir );
            if ( forward <= 0 ) continue;

    		// Score candidates based on a combination of forward distance and lateral deviation
            f32 lateral = nm::length( delta - dir * forward );
            f32 score   = forward + lateral * a_LateralPenaltyWeight;

            if ( score < bestScore ) 
            { 
                bestScore = score;
                best = candidate; 
            }
        }

        return best;
    }

} // namespace RatUI
