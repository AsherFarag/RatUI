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

	void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
    {
        const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
        if ( !node )
            return;

        const Rect<Unit>& rect = node->Layout.FinalRect;

		if ( a_Scene.GetFocusedWidget() == GetID() )
			a_DrawList.AddRect( Colors::White, rect.Expanded( 4_u ), Rounding + 4_u );

        static TextureHandle checkerboardTexture = LoadTexture( "Resources/Textures/LargeSquarePattern.jpg" );

		a_DrawList.AddImage( checkerboardTexture, rect, Color, Rounding );

        a_DrawList.PushClipRect( rect );
        a_Scene.ForEachChildWidget( GetID(), [&](IWidget& child)
        {
            child.OnPaint( a_Scene, a_DrawList );
		} );
        a_DrawList.PopClipRect();
    }

    bool IsFocusable( Scene& a_Scene ) const override
    {
        return true;
	}
};

class CircleWidget : public IWidget
{
public:

    Unit  Radius;
    Color FillColor;
    bool  IsFilled;

    CircleWidget( Unit a_Radius, Color a_Color, bool a_Filled = true )
        : Radius( a_Radius )
        , FillColor( a_Color )
        , IsFilled( a_Filled )
    {}

    void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
    {
        const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
        if ( !node )
            return;

        const Rect<Unit>& rect = node->Layout.FinalRect;
        Vec2<Unit> center = rect.Center();

        a_DrawList.SetDrawLayer( 1 ); // Draw circles above default layer 0 used by RectWidget

        if ( IsFilled )
        {
            if ( a_Scene.GetFocusedWidget() == GetID() )
			    a_DrawList.AddCircle( Colors::LightYellow, center, Radius + 4_u );

            a_DrawList.AddCircle( FillColor, center, Radius );
        }
        else
        {
            const Unit borderThickness = 4_u;
            if ( a_Scene.GetFocusedWidget() == GetID() )
                a_DrawList.AddCircleBorder( Colors::LightYellow, center, Radius + 4_u, borderThickness + 2_u );

            a_DrawList.AddCircleBorder( FillColor, center, Radius, borderThickness );
        }
    }

    bool IsFocusable( Scene& a_Scene ) const override
    {
        return true;
    }
};