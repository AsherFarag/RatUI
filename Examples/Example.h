#pragma once
#include <RatUI/RatUI.h>

using namespace RatUI;

/** @brief Font families the Application registers from Resources/Fonts, set before any Example is created. */
namespace Fonts
{
    inline FontFamilyHandle Roboto;
    inline FontFamilyHandle RobotoRaster;
    inline FontFamilyHandle Minecraft;
}

/**
 * @brief Base class for an example.
 *
 * Build your widgets into m_Scene in the constructor. The Application feeds it input,
 * lays it out and draws it every frame.
 */
class Example
{
public:
    explicit Example( TextMetrics& a_TextMetrics ) { m_Scene.TextMetrics = &a_TextMetrics; }
    virtual ~Example() = default;

    /** @brief Called once per frame, before layout. */
    virtual void Update( f32 a_DeltaSeconds ) {}

    /** @brief Draws the scene. Override to draw extra things on top, e.g. fast text. */
    virtual void Render( DrawList& a_DrawList, f32 a_DeltaSeconds ) { m_Scene.Render( a_DrawList, a_DeltaSeconds ); }

    Scene& GetScene() { return m_Scene; }

protected:
    Scene m_Scene;
};
