#pragma once
#include "Layout.h"
#include <concepts>
#include <algorithm>

namespace RatUI
{
    /**
     * @brief Represents a UI element in the RatUI layout system, containing layout styles, results, and hierarchical relationships with other widgets.
     * The hierarchy is represented as a doubly-linked list of children for efficient insertion and removal.
     * This avoids the overhead of dynamic arrays for child management, with minimal traversal costs as Widgets are stored in pools.
     */
    struct Widget
    {
        LayoutStyle Style{};           ///< The layout style properties that define how this widget should be sized and positioned.
        LayoutResult Layout{};         ///< The cached layout result for this widget computed during the layout process.
        Widget* Parent{ nullptr };
        Widget* FirstChild{ nullptr };
        Widget* LastChild{ nullptr };
        Widget* PrevSibling{ nullptr };
        Widget* NextSibling{ nullptr };
        u32 NumChildren{ 0 };

        // TODO: Add proper hierarchy functionality

        /**
         * @brief Adds a child widget to this widget, updating the linked list pointers accordingly.
         * @param a_Child The child widget to add. It will be added as the last child of this widget.
         */
        void AddChild( Widget& a_Child )
        {
            a_Child.Parent     = this;
            a_Child.NextSibling = nullptr;
            if ( LastChild )
            {
                LastChild->NextSibling = &a_Child;
                a_Child.PrevSibling = LastChild;
            }
            else
            {
                FirstChild = &a_Child;
                a_Child.PrevSibling = nullptr;
            }
            LastChild = &a_Child;
            ++NumChildren;
        }

        /**
         * @brief Applies the given function to each child widget of this widget.
         * @tparam Func The type of the function to apply to each child widget. It must be invocable with a Widget reference.
         * @param a_Func A callable that takes a Widget reference. It will be invoked for each child widget of this widget.
         */
        template<std::invocable<Widget&> Func>
        void ForEachChild( Func&& a_Func )
        {
            for (Widget* child = FirstChild; child != nullptr; child = child->NextSibling)
                std::forward<Func>(a_Func)(*child);
        }

        /**
         * @brief Applies the given function to each child widget of this widget (const version).
         * @tparam Func The type of the function to apply to each child widget. It must be invocable with a const Widget reference.
         * @param a_Func A callable that takes a const Widget reference. It will be invoked for each child widget of this widget.
         */
        template<std::invocable<const Widget&> Func>
        void ForEachChild( Func&& a_Func ) const
        {
            for (const Widget* child = FirstChild; child != nullptr; child = child->NextSibling)
                std::forward<Func>(a_Func)(*child);
        }
    };

} // namespace RatUI