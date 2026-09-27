#pragma once
#include <RatUI/RatUI.h>

using namespace RatUI;

/** @brief Fonts the Application loads from Resources/Fonts. */
namespace Fonts
{
    inline constexpr FontHandle Roboto   { 1 };
    inline constexpr FontHandle Minecraft{ 2 };
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
    explicit Example( ITextMetrics& a_TextMetrics ) { m_Scene.TextMetrics = &a_TextMetrics; }
    virtual ~Example() = default;

    /** @brief Called once per frame, before layout. */
    virtual void Update( f32 a_DeltaSeconds ) {}

    Scene& GetScene() { return m_Scene; }

protected:
    Scene m_Scene;
};
