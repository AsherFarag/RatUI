#pragma once
#include "../Core.h"
#include "../Layout/Layout.h"
#include "../Renderer/DrawList.h"

namespace RatUI
{
    struct Scene;

    /**
     * @brief
     */
    class IWidget
    {
    public:
        WidgetID ID{};
        NodeID LayoutID{};

        virtual ~IWidget() = default;

        /**
         * @brief Called once per frame after layout is resolved.
         * @param a_Context The rendering context used for painting this widget.
         */
        virtual void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) {}

        /** @brief Called when a pointer (e.g., mouse cursor) enters the widget's bounds. */
        virtual void OnHoverEnter() {}

        /** @brief Called when a pointer (e.g., mouse cursor) exits the widget's bounds. */
        virtual void OnHoverExit() {}

        // TODO: These would require an input button type and not sure how I want to go about that yet
        virtual void OnPressed() {}
        virtual void OnReleased() {}
        //virtual void OnInput() {}
    };

} // namespace RatUI