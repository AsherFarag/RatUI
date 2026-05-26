#pragma once
#include <RatUI/RatUI.h>
#include <RatUI/Widget/TextWidget.h>
#include <RatUI/Widget/ButtonWidget.h>
#include <RatUI/Widget/PanelWidget.h>

using namespace RatUI;

extern TextureHandle LoadTexture( const char* a_FilePath );

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

    virtual void Init() {}
    virtual void OnInputEvent( const InputEvent& a_Event ) {}
    virtual void Update( f32 a_DeltaTime ) {}
    virtual void Render( DrawList& a_DrawList ) {}
    virtual void Shutdown() {}

protected:
    Scene m_Scene; 
};

class RectWidget : public IWidget
{
public:
	RectWidget( Color a_Color, StringView a_Name, CornerRounding a_Rounding = CornerRounding::Uniform( 10_u ) )
		: Color( a_Color )
		, Name( a_Name )
        , Rounding( a_Rounding )
    {}

	StringView     Name;
    Color          Color;
    CornerRounding Rounding;

	void OnPaint( DrawList& a_DrawList ) override
    {
        Scene& scene = GetScene();
        const LayoutNode* node = scene.Layouts.Get( GetLayoutID() );
        if ( !node )
            return;

        const Rect<Unit>& rect = node->Layout.FinalRect;

        if ( scene.GetFocusedWidget() == GetID() )
        {
            a_DrawList.AddRect( rect,
            {
                .FillColor = Color,
                .BorderColor = Colors::White,
                .BorderThickness = 4_u,
                .Rounding = Rounding
            } );
        }
        else
        {
            a_DrawList.AddRect( rect,
            {
                .FillColor = Color,
                .Rounding = Rounding
            } );
        }

        scene.ForEachChildWidget( GetLayoutID(), [&](IWidget& child)
        {
            child.OnPaint( a_DrawList );
		} );
    }

    bool IsFocusable() const override
    {
        return true;
	}
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