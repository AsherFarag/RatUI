#pragma once
#include <RatUI/RatUI.h>
#include <RatUI/Widget/TextWidget.h>
#include <RatUI/Widget/ButtonWidget.h>
#include <RatUI/Widget/PanelWidget.h>

using namespace RatUI;

extern TextureHandle LoadTexture( const char* a_FilePath, TextureSampler a_Sampler = {} );

inline Text MakeText( String a_String )
{
	return Text{ std::move( a_String ) };
}

/**
 * @brief Interface for demo scenes in the RatUI examples. 
 */
class IDemoScene
{
public:
    IDemoScene( ITextMetrics* a_TextMetrics ) : m_Scene() { m_Scene.TextMetrics = a_TextMetrics; }
    virtual ~IDemoScene() = default;

    Scene& GetScene() { return m_Scene; }
    const Scene& GetScene() const { return m_Scene; }

    virtual void OnInputEvent( const InputEvent& a_Event ) {}
    virtual void Update( f32 a_DeltaTime ) {}
    virtual void Render( DrawList& a_DrawList ) {}

protected:
    Scene m_Scene;
};

class CircleWidget : public IWidget
{
public:
    Unit  Radius;
    Color FillColor;

    CircleWidget( Unit a_Radius, Color a_Color )
        : Radius( a_Radius )
        , FillColor( a_Color )
    {}

    void OnPaint( DrawList& a_DrawList ) override
    {
        Scene& scene = GetScene();
        const LayoutNode* node = scene.Layouts.Get( GetLayoutID() );
        if ( !node )
            return;

        const Rect<Unit>& rect = node->Layout.FinalRect;
        Vec2<Unit> center = rect.Center();

        if ( scene.GetFocusedWidget() == GetID() )
        {
            a_DrawList.AddCircle( center, Radius,
            {
                .FillColor = FillColor,
                .BorderColor = Colors::White,
                .BorderThickness = 4_u,
            } );
        }
        else
        {
            a_DrawList.AddCircle( center, Radius,
            {
                .FillColor = FillColor,
            } );
        }
    }

    bool IsFocusable() const override
    {
        return true;
    }
};