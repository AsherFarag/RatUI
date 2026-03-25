#pragma once
#include "../Core.h"
#include "IWidget.h"

namespace RatUI
{
    using LayoutNodePool = Pool<LayoutNode>;
    using NodeID = PoolID;

    using WidgetPool = Pool<Unique<IWidget>>;
    using WidgetID = PoolID;

    class Scene
    {
    public:

    private:
        LayoutNodePool m_LayoutPool{};
        WidgetPool m_WidgetPool{};
    };

} // namespace RatUI