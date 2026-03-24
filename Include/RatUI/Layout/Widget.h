#pragma once
#include "Layout.h"
#include <concepts>
#include <algorithm>

namespace RatUI
{

    /**
     * @brief Represents a UI element in the RatUI layout system, containing layout styles, results, and hierarchical relationships with other widgets.
     */
    struct Widget
    {
        LayoutStyle Style{};
        LayoutResult Layout{};
        Widget* Parent{ nullptr };
        Widget* FirstChild{ nullptr };
        Widget* LastChild{ nullptr };
        Widget* PrevSibling{ nullptr };
        Widget* NextSibling{ nullptr };
        u32 NumChildren{ 0 };

        /**
         * @brief Applies the given function to each child widget of this widget.
         * @tparam Func The type of the function to apply to each child widget. It must be invocable with a Widget reference.
         * @param a_Func A callable that takes a Widget reference. It will be invoked for each child widget of this widget.
         */
        template<typename Func>
        void ForEachChild( Func&& a_Func )
        {
            for (Widget* child = FirstChild; child != nullptr; child = child->NextSibling)
            {
                a_Func(*child);
            }
        }

        /**
         * @brief Applies the given function to each child widget of this widget (const version).
         * @tparam Func The type of the function to apply to each child widget. It must be invocable with a const Widget reference.
         * @param a_Func A callable that takes a const Widget reference. It will be invoked for each child widget of this widget.
         */
        template<typename Func>
        void ForEachChild( Func&& a_Func ) const
        {
            for (const Widget* child = FirstChild; child != nullptr; child = child->NextSibling)
            {
                a_Func(*child);
            }
        }
    };

    // TODO: These functions are getting pretty fat, should consider making RatUI a static lib or doing something like stb lib
    inline Vec2f MeasureWidget( Widget& a_Widget, Vec2f a_AvailableSize )
    {
        const LayoutStyle& s = a_Widget.Style;
        Vec2f desired{ 0.0f, 0.0f };

        // Resolve width
        switch (s.WidthMode)
        {
            case ESizingMode::Fixed:   desired[0] = s.FixedWidth; break;
            case ESizingMode::Fill:    desired[0] = a_AvailableSize[0] * s.PercentWidth; break;
            case ESizingMode::Content: desired[0] = 0.f; break; // children fill this in
        }

        // Resolve height
        switch (s.HeightMode)
        {
            case ESizingMode::Fixed:   desired[1] = s.FixedHeight; break;
            case ESizingMode::Fill:    desired[1] = a_AvailableSize[1] * s.PercentHeight; break;
            case ESizingMode::Content: desired[1] = 0.f; break; // children fill this in
        } 

        // Accumulate children for content mode
        {
            Vec2f contentSize{ 0.0f, 0.0f };
            Vec2f childAvailSize = a_AvailableSize - s.Padding.Total();

            a_Widget.ForEachChild( [&]( Widget& child )
            {
                const Vec2f childDesired = MeasureWidget( child, childAvailSize );

                switch (s.LayoutType)
                {
                    case ELayoutType::Horizontal:
                        contentSize[0] += childDesired[0] + s.Spacing;
                        contentSize[1] = std::max( contentSize[1], childDesired[1] );
                        break;

                    case ELayoutType::Vertical:
                        contentSize[0] = std::max( contentSize[0], childDesired[0] );
                        contentSize[1] += childDesired[1] + s.Spacing;
                        break;

                    case ELayoutType::Overlay:
                        contentSize[0] = std::max( contentSize[0], childDesired[0] );
                        contentSize[1] = std::max( contentSize[1], childDesired[1] );
                        break;

                    case ELayoutType::Grid:
                        // TODO: Implement grid layout measurement logic which requires rows and columns
                        break;
                }
            } );

            // Remove trailing spacing
            if (a_Widget.FirstChild)
            {
                if ( s.LayoutType == ELayoutType::Horizontal )
                    contentSize[0] -= s.Spacing;
                else if (s.LayoutType == ELayoutType::Vertical)
                    contentSize[1] -= s.Spacing;
            }

            // Add padding
            contentSize = contentSize + s.Padding.Total();

            // Override desired size with content size for Content mode
            if (s.WidthMode == ESizingMode::Content)  desired[0] = contentSize[0];
            if (s.HeightMode == ESizingMode::Content) desired[1] = contentSize[1];
        }

        // Clamp to constraints
        desired[0] = std::clamp( desired[0], s.SizeConstraints.MinSize[0], s.SizeConstraints.MaxSize[0] );
        desired[1] = std::clamp( desired[1], s.SizeConstraints.MinSize[1], s.SizeConstraints.MaxSize[1] );

        a_Widget.Layout.DesiredSize = desired;
        return desired;
    }

} // namespace RatUI