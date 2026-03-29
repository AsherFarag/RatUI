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
        LayoutNodePool Layouts{};
        WidgetPool Widgets{};
        WidgetID RootWidget{};

        template<std::derived_from<IWidget> WidgetType, typename... Args>
        WidgetID CreateWidget( WidgetID a_ParentID, Args&&... a_Args )
        {
			Unique<IWidget> widgetPtr = MakeUnique<WidgetType>( std::forward<Args>( a_Args )... );
			IWidget* widget = widgetPtr.get();

            // Allocate layout node and widget
            NodeID     nodeID   = Layouts.Allocate();
			WidgetID   widgetID = Widgets.Allocate( std::move( widgetPtr ) );

            LayoutNode* node   = Layouts.Get( nodeID );

            // Wire widget <-> node
            widget->m_ID       = widgetID;
            widget->m_LayoutID = nodeID;
            node->WidgetID     = widgetID;

			if ( a_ParentID != c_InvalidPoolID )
            {
                if ( IWidget* parentWidget = GetWidget( a_ParentID ) )
                {
                    if ( LayoutNode* parentNode = Layouts.Get( parentWidget->GetLayoutID() ) )
                        parentNode->AddChild( *node );
                }
            }

            return widgetID;
        }

        template<std::derived_from<IWidget> WidgetType, typename... Args>
        WidgetID CreateRootWidget( Args&&... a_Args )
        {
            WidgetID id = CreateWidget<WidgetType>( c_InvalidPoolID, std::forward<Args>( a_Args )... );
            RootWidget = id;
            return id;
        }

        RATUI_NODISCARD IWidget* GetWidget( WidgetID a_ID )
        {
            if ( Unique<IWidget>* widget = Widgets.Get( a_ID ) )
                return widget->get();

            return nullptr;
        }

        RATUI_NODISCARD const IWidget* GetWidget( WidgetID a_ID ) const
        {
            if ( const Unique<IWidget>* widget = Widgets.Get( a_ID ) )
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
                if ( LayoutNode* rootNode = Layouts.Get( root->GetLayoutID() ) )
                {
                    MeasureLayoutNode( *rootNode, a_AvailableSize );
                    ArrangeLayoutNode( *rootNode, Rectf{ Vec2f{ 0.f, 0.f }, a_AvailableSize } );
                }
            }
        }

		void Render( DrawList& a_DrawList )
        {
            if ( IWidget* root = GetWidget( RootWidget ) )
                root->OnPaint( *this, a_DrawList );
        }

        void ProcessInput( Vec2f a_PhysicalMousePos, bool a_MouseDown, f32 a_Scale )
        {
            Vec2f logicalPos = a_PhysicalMousePos / a_Scale;

            WidgetID hovered = HitTest( RootWidget, logicalPos );

            // Hover enter/exit
            if ( hovered != m_HoveredWidget )
            {
                if ( IWidget* prev = GetWidget( m_HoveredWidget ) ) prev->OnHoverExit();
                if ( IWidget* next = GetWidget( hovered ) )         next->OnHoverEnter();
                m_HoveredWidget = hovered;
            }

            // Press/release
            if ( a_MouseDown && !m_MouseWasDown )
                if ( IWidget* w = GetWidget( hovered ) ) w->OnPressed();

            if ( !a_MouseDown && m_MouseWasDown )
                if ( IWidget* w = GetWidget( hovered ) ) w->OnReleased();

            m_MouseWasDown = a_MouseDown;
        }

        template<std::invocable<IWidget&> Func>
        void ForEachChildWidget( WidgetID a_WidgetID, Func&& a_Func )
        {
            IWidget* widget = GetWidget( a_WidgetID );
            if ( !widget ) return;
        
            LayoutNode* node = Layouts.Get( widget->GetLayoutID() );
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
        
            const LayoutNode* node = Layouts.Get( widget->GetLayoutID() );
            if ( !node ) return;
        
            node->ForEachChild( [&]( const LayoutNode& childNode )
            {
                const IWidget* childWidget = GetWidget( childNode.WidgetID );
                if ( childWidget ) a_Func( *childWidget );
            });
		}

    protected:
        WidgetID HitTest( WidgetID a_ID, Vec2f a_LogicalPos )
        {
            IWidget* widget = GetWidget( a_ID );
            if ( !widget ) return c_InvalidPoolID;

            LayoutNode* node = Layouts.Get( widget->GetLayoutID() );
            if ( !node || !node->Layout.Visibility.IsHitTestable() ) return c_InvalidPoolID;

            if ( !node->Layout.FinalRect.Contains( a_LogicalPos ) ) return c_InvalidPoolID;

            // Check children first (front-to-back, last child wins)
            WidgetID result = a_ID; // self is the fallback
            node->ForEachChild( [&]( LayoutNode& child )
            {
                WidgetID childHit = HitTest( child.WidgetID, a_LogicalPos );
                if ( childHit != c_InvalidPoolID )
                    result = childHit; // deepest child takes priority
            });

            return result;
        }

        WidgetID m_HoveredWidget{ c_InvalidPoolID };
        bool     m_MouseWasDown{ false };
    };

} // namespace RatUI