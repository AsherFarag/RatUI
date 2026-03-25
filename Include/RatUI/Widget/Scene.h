#pragma once
#include "../Core.h"
#include "../Layout/LayoutEngine.h"
#include "IWidget.h"

namespace RatUI
{
    using LayoutNodePool = Pool<LayoutNode>;

    using WidgetPool = Pool<Unique<IWidget>>;
    using WidgetID = PoolID;

    struct Scene
    {
        LayoutNodePool LayoutPool{};
        WidgetPool WidgetPool{};
        WidgetID RootWidget{ c_InvalidPoolID };

        template<std::derived_from<IWidget> WidgetType, typename... Args>
        WidgetID CreateWidget( Args&&... a_Args )
        {
            return WidgetPool.Allocate( MakeUnique<WidgetType>( std::forward<Args>( a_Args )... ) );
        }

        RATUI_NODISCARD IWidget* GetWidget( WidgetID a_ID )
        {
            if ( Unique<IWidget>* widget = WidgetPool.Get( a_ID ) )
                return widget->get();

            return nullptr;
        }

        template<std::derived_from<IWidget> WidgetType>
        RATUI_NODISCARD WidgetType* GetWidget( WidgetID a_ID )
        {
            return dynamic_cast<WidgetType*>( GetWidget( a_ID ) );
        }

        void UpdateLayout( Vec2f a_AvailableSize )
        {
            if ( IWidget* root = GetWidget( RootWidget ) )
            {
                if ( LayoutNode* rootNode = LayoutPool.Get( root->LayoutID ) )
                {
                    MeasureLayoutNode( *rootNode, a_AvailableSize );
                    ArrangeLayoutNode( *rootNode, Rectf{ Vec2f{ 0.f, 0.f }, a_AvailableSize } );
                }
            }
        }

        void Render( const RenderContext& a_Context )
        {
            if ( IWidget* root = GetWidget( RootWidget ) )
                root->OnPaint( *this, a_Context );
        }
    };

} // namespace RatUI