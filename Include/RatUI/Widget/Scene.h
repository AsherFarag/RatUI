#pragma once
#include "../Core.h"
#include "../Layout/LayoutEngine.h"
#include "IWidget.h"

namespace RatUI
{
    using LayoutNodePool = Pool<LayoutNode>;
    using WidgetPool = Pool<Unique<IWidget>>;

    struct Scene
    {
        LayoutNodePool LayoutPool{};
        WidgetPool WidgetPool{};
        WidgetID RootWidget{ c_InvalidPoolID };

        template<std::derived_from<IWidget> WidgetType, typename... Args>
        WidgetID CreateWidget( Args&&... a_Args )
        {
			Unique<IWidget> widgetPtr = MakeUnique<WidgetType>( std::forward<Args>( a_Args )... );
			IWidget* widget = widgetPtr.get();

            // Allocate layout node and widget
            NodeID     nodeID   = LayoutPool.Allocate();
			WidgetID   widgetID = WidgetPool.Allocate( std::move( widgetPtr ) );

            LayoutNode* node   = LayoutPool.Get( nodeID );

            // Wire widget <-> node
            widget->ID       = widgetID;
            widget->LayoutID = nodeID;
            node->WidgetID   = widgetID;

            return widgetID;
        }

        RATUI_NODISCARD IWidget* GetWidget( WidgetID a_ID )
        {
            if ( Unique<IWidget>* widget = WidgetPool.Get( a_ID ) )
                return widget->get();

            return nullptr;
        }

        RATUI_NODISCARD const IWidget* GetWidget( WidgetID a_ID ) const
        {
            if ( const Unique<IWidget>* widget = WidgetPool.Get( a_ID ) )
                return widget->get();

            return nullptr;
		}

        template<std::derived_from<IWidget> WidgetType>
        RATUI_NODISCARD WidgetType* GetWidget( WidgetID a_ID )
        {
            return dynamic_cast<WidgetType*>( GetWidget( a_ID ) );
        }

		template<std::derived_from<IWidget> WidgetType>
        RATUI_NODISCARD const WidgetType* GetWidget( WidgetID a_ID ) const
        {
            return dynamic_cast<const WidgetType*>( GetWidget( a_ID ) );
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

        template<std::invocable<IWidget&> Func>
        void ForEachChildWidget( WidgetID a_WidgetID, Func&& a_Func )
        {
            IWidget* widget = GetWidget( a_WidgetID );
            if ( !widget ) return;
        
            LayoutNode* node = LayoutPool.Get( widget->LayoutID );
            if ( !node ) return;
        
            node->ForEachChild( [&]( LayoutNode& childNode )
            {
                IWidget* childWidget = GetWidget( childNode.WidgetID );
                if ( childWidget ) a_Func( *childWidget );
            });
        }

		template<std::invocable<const IWidget&> Func>
        void ForEachChildWidget( WidgetID a_WidgetID, Func&& a_Func ) const
        {
            const IWidget* widget = GetWidget( a_WidgetID );
            if ( !widget ) return;
        
            const LayoutNode* node = LayoutPool.Get( widget->LayoutID );
            if ( !node ) return;
        
            node->ForEachChild( [&]( const LayoutNode& childNode )
            {
                const IWidget* childWidget = GetWidget( childNode.WidgetID );
                if ( childWidget ) a_Func( *childWidget );
            });
		}
    };

} // namespace RatUI