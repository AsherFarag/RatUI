#pragma once
#include "Scene.h"
#include "IWidget.h"

namespace RatUI
{
    class ButtonBaseWidget : public IWidget
    {
    public:
        Callback<Scene&, WidgetID> OnClick; ///< Callback that is invoked when the button is clicked.

        virtual ~ButtonBaseWidget() = default;

        bool IsFocusable( Scene& a_Scene ) const override { return true; }

        bool OnReleased( Scene& a_Scene, const ButtonEvent& a_Event ) override
        {
            if ( a_Event.Released )
                Invoke( OnClick, a_Scene, GetID() );
            return true;    
        }
    };
} // namespace RatUI