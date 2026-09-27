#include <RatUI/RatUI.h>
#include <RatUI/Widget/PanelWidget.h>
#include <cstdio>

using namespace RatUI;

int main()
{
    Scene scene;
    PanelWidget* root  = scene.CreateRootWidget<PanelWidget>();
    PanelWidget* child = scene.CreateWidget<PanelWidget>( root->GetLayoutID() );
    child->GetLayout().FixedWidth( 100_u ).FixedHeight( 50_u );
    scene.UpdateLayout( { 800_u, 600_u } );

    const Vec2<Unit> size = child->GetLayout().Layout.FinalRect.Size;
    std::printf( "RatUI %s: child is %.0fx%.0f\n", RATUI_VERSION_STRING, size[0].ToFloat(), size[1].ToFloat() );
    return size[0].ToFloat() == 100.f && size[1].ToFloat() == 50.f ? 0 : 1;
}
