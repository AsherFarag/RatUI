#pragma once
#include "IWidget.h"
#include "Scene.h"

namespace RatUI
{
    // TODO: BuilderMixin support?

  //  template<std::derived_from<IWidget> WidgetType>
  //  class Builder
  //  {
  //  public:
  //      WidgetType& Widget;

  //      template<typename... ConstructorArgs>
  //      Builder( Scene& a_Scene, NodeID a_ParentID, ConstructorArgs&&... a_Args )
  //          : Widget( *a_Scene.CreateWidget<WidgetType>( a_ParentID, std::forward<ConstructorArgs>( a_Args )... ) )
  //      {}

  //      Builder& WithLayout( const LayoutStyle& a_Layout )
  //      {
  //          Widget.GetLayout().Style = a_Layout;
  //          return *this;
  //      }

  //      template<
  //          std::derived_from<IWidget> ChildWidgetType, 
  //          std::invocable<Builder<ChildWidgetType>&> ChildBuilderFunc,
  //          typename... ConstructorArgs>
  //      Builder& AddChild( ChildBuilderFunc a_BuilderFunc, ConstructorArgs&&... a_ConstructorArgs )
  //      {
		//	Builder<ChildWidgetType> childBuilder( Widget.GetScene(), Widget.GetLayoutID(), std::forward<ConstructorArgs>( a_ConstructorArgs )... );
  //          a_BuilderFunc( childBuilder );
  //          return *this;
  //      }

		//// TODO: Need to implement a modular way for mixins to add their own builder methods.

		//Builder& WithTheme( ThemeHandle a_Theme ) requires ( WidgetType::template HasMixin<ThemeMixin> )
		//{
  //          Widget.Theme = std::move( a_Theme );
		//	return *this;
		//}

  //      Builder& WithDebugName( String a_DebugName ) requires ( WidgetType::template HasMixin<DebugMixin> )
  //      {
  //          Widget.GetLayout().DebugName = std::move( a_DebugName );
  //          return *this;
  //      }
  //  };

} // namespace RatUI