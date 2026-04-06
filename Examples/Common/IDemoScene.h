#pragma once
#include <RatUI/RatUI.h>
#include <RatUI/Widget/TextWidget.h>

using namespace RatUI;

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
	RectWidget( Colorf a_Color, StringView a_Name, CornerRounding a_Rounding = CornerRounding::Uniform( 10_deg ) )
		: Color( a_Color )
		, Name( a_Name )
        , Rounding( a_Rounding )
    {}

	StringView     Name;
    Colorf         Color;
    CornerRounding Rounding;

	void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
    {
        const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
        if ( !node )
            return;

        const Rectf& rect = node->Layout.FinalRect;

		if ( a_Scene.GetFocusedWidget() == GetID() )
			a_DrawList.AddRect( Colorsf::White, rect.Expanded( 4.f ), Rounding + 4_deg );

		a_DrawList.AddRect( Color, rect, Rounding );

        //a_DrawList.PushClipRect( { .Origin = { (u16)rect.Origin[0], (u16)rect.Origin[1] }, .Size = { (u16)rect.Size[0], (u16)rect.Size[1] } } );
        a_Scene.ForEachChildWidget( GetID(), [&](IWidget& child)
        {
            child.OnPaint( a_Scene, a_DrawList );
		} );
        //a_DrawList.PopClipRect();
    }

    bool IsFocusable( Scene& a_Scene ) const override
    {
        return true;
	}
};

class CircleWidget : public IWidget
{
public:

    f32    Radius;
    Colorf Color;
    bool   IsFilled;

    CircleWidget( f32 a_Radius, Colorf a_Color, bool a_Filled = true )
        : Radius( a_Radius )
        , Color( a_Color )
        , IsFilled( a_Filled )
    {}

    void OnPaint( Scene& a_Scene, DrawList& a_DrawList ) override
    {
        const LayoutNode* node = a_Scene.Layouts.Get( GetLayoutID() );
        if ( !node )
            return;

        const Rectf& rect = node->Layout.FinalRect;
        Vec2f center = rect.Center();

        if ( IsFilled )
        {
            if ( a_Scene.GetFocusedWidget() == GetID() )
			    a_DrawList.AddCircle( Colorsf::LightYellow, center, Radius + 4.f );

            a_DrawList.AddCircle( Color, center, Radius );
        }
        else
        {
            const f32 borderThickness = 4.f;
            if ( a_Scene.GetFocusedWidget() == GetID() )
                a_DrawList.AddCircleBorder( Colorsf::LightYellow, center, Radius + 4.f, borderThickness + 2.f );

            a_DrawList.AddCircleBorder( Color, center, Radius, borderThickness );
        }
    }

    bool IsFocusable( Scene& a_Scene ) const override
    {
        return true;
    }
};