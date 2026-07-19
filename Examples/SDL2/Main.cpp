#include "SDL2Application.h"
#include <RatUI/Backends/FreeType/TextMetrics.h>
#include <RatUI/Widget/TextWidget.h>
#include <iostream>
#include <functional>
#include <filesystem>

//#include "../Common/DynamicTextScene.h"
#include "../Common/ThemeShowcaseScene.h"
#include <RatUI/Animation/Animation.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../Common/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

using namespace RatUI;
using namespace RatUI::Literals;

static IRenderer* g_Renderer = nullptr;

#include <RatUI/Text/TextEdit.h>
#include <cassert>

void TestTextEditModel()
{
    //-------------------------------------------------------------------------
    // Basic insertion
    //-------------------------------------------------------------------------

    TextEditModel model;

    assert( model.GetTextBuffer().empty() );
    assert( model.Caret() == 0 );
    assert( model.Anchor() == 0 );

    assert( model.Insert( U"Hello" ) );
    assert( model.GetTextBuffer() == U"Hello" );
    assert( model.Caret() == 5 );
    assert( model.Anchor() == 5 );

    //-------------------------------------------------------------------------
    // Caret movement
    //-------------------------------------------------------------------------

    model.MoveLeft();
    assert( model.Caret() == 4 );

    model.MoveRight();
    assert( model.Caret() == 5 );

    model.MoveHome();
    assert( model.Caret() == 0 );

    model.MoveEnd();
    assert( model.Caret() == 5 );

    //-------------------------------------------------------------------------
    // Insert in middle
    //-------------------------------------------------------------------------

    model.SetCaret( 2 );
    assert( model.Insert( U"XX" ) );

    assert( model.GetTextBuffer() == U"HeXXllo" );
    assert( model.Caret() == 4 );

    //-------------------------------------------------------------------------
    // Backspace
    //-------------------------------------------------------------------------

    assert( model.Backspace() );
    assert( model.GetTextBuffer() == U"HeXllo" );

    //-------------------------------------------------------------------------
    // Delete
    //-------------------------------------------------------------------------

    model.SetCaret( 2 );
    assert( model.Delete() );
    assert( model.GetTextBuffer() == U"Hello" );

    //-------------------------------------------------------------------------
    // Selection replacement
    //-------------------------------------------------------------------------

    model.SelectAll();
    assert( model.HasSelection() );

    assert( model.ReplaceSelection( U"World" ) );

    assert( model.GetTextBuffer() == U"World" );
    assert( !model.HasSelection() );
    assert( model.Caret() == 5 );

    //-------------------------------------------------------------------------
    // Undo / Redo
    //-------------------------------------------------------------------------

    assert( model.Undo() );
    assert( model.GetTextBuffer() == U"Hello" );

    assert( model.Redo() );
    assert( model.GetTextBuffer() == U"World" );

    //-------------------------------------------------------------------------
    // Multiline movement
    //-------------------------------------------------------------------------

    model.SetTextBuffer( U"abc\n123\nXYZ" );

    model.MoveHome();
    assert( model.Caret() == 0 );

    model.MoveDown();
    model.MoveDown();
    model.MoveUp();

	assert( model.Caret() == 4 ); // Should be at the start of the second line
}

/**
 * @brief The RatUI Sandbox application.
 *
 * A scratch-pad for experimenting with RatUI widgets and layouts.
 * Add widgets to OnInitialize() and update/render them in OnUpdate()/OnRender().
 */
class SandboxApp : public SDL2Application
{
public:
    SandboxApp()
        : SDL2Application( { "RatUI Sandbox", ToColorF32( Colors::Surface900 ), 1280, 720 } )
    {}

protected:

    Optional<FreeType::FontCache> m_FontCache;
    Optional<FreeType::TextMetrics> m_TextMetrics;
    Optional<GlyphAtlas> m_Atlas;
    Optional<DrawList> m_DrawList;
    Unique<IDemoScene> m_Scene;

	f32 m_UIScale{ 1.f };

	f32 m_DeltaSeconds{ 0.f };

    bool OnInitialize() override
    {
        g_Renderer = GetRenderer();

        const FontHandle fontHandle = { 1 };
		const FontHandle minecraftFontHandle = { 2 };
        m_FontCache.emplace();
        m_FontCache->RegisterFontHandle( fontHandle, "Resources/Fonts/Roboto-Medium.ttf" );
        m_FontCache->RegisterFontHandle( minecraftFontHandle, "Resources/Fonts/Minecraft.ttf" );

        m_TextMetrics.emplace( *m_FontCache );
        m_Atlas.emplace( *m_Renderer, *m_TextMetrics );
		m_DrawList.emplace( *m_Atlas );

        m_Scene = MakeUnique<ThemeShowcaseScene>( fontHandle, &*m_TextMetrics );
    
        return true;
    }

    void OnUpdate() override
    {
        static f32 prevTime = 0.f;
		const f32 currTime = SDL_GetTicks();
        m_DeltaSeconds = ( currTime - prevTime ) / 1000.f;
        prevTime = currTime;

		//m_UIScale = 1.f + 0.5f * std::sin( currTime / 1000.f ); // Oscillate UI scale between  and 1.5 over time.

        if ( !m_Scene )
        {
            return;
        }

        m_Scene->Update( m_DeltaSeconds );

        // Update the layout with the current window size as the available space.
        i32 windowWidth, windowHeight;
        SDL_GetWindowSize( GetWindow(), &windowWidth, &windowHeight );
        Vec2<Unit> windowSize{ Unit{ static_cast<f32>( windowWidth ) }, Unit{ static_cast<f32>( windowHeight ) } };

		windowSize[0] /= m_UIScale;
		windowSize[1] /= m_UIScale;

        m_Scene->GetScene().UpdateLayout( windowSize );
    }

    void OnRender( IRenderer& a_Renderer ) override
    {
        if ( !m_Scene )
        {
            return;
        }

		// Get dpi scale for current window (for correct text rendering on high-DPI displays)
        f32 dpiscale = 1.f;
        SDL_GetDisplayDPI( SDL_GetWindowDisplayIndex( GetWindow() ), nullptr, &dpiscale, nullptr );
		dpiscale /= 96.f; // Convert from DPI to scale factor (assuming 96 DPI is the baseline)
        m_DrawList->SetDPIScale( dpiscale * m_UIScale );

        m_DrawList->Clear();
		m_Scene->Render( *m_DrawList, m_DeltaSeconds );
		m_DrawList->Flush( a_Renderer );
    }

    bool OnShutdown() override
    {
        m_Scene.reset();
		m_DrawList.reset();
		m_Atlas.reset();
		m_TextMetrics.reset();
        m_FontCache.reset();
        return true;
    }

    void OnInputEvent( const InputEvent& a_Event ) override
    {
        if ( m_Scene )
        {
            m_Scene->OnInputEvent( a_Event );
        }
    }
};

#undef main // SDL2 redefines main() on some platforms, so we undefine it here to avoid conflicts with our own main() function.

int main( int argc, char** argv )
{
    TestTextEditModel();
    SandboxApp app;
    return app.Run() ? 0 : 1;
}

TextureHandle LoadTexture( const char* a_FilePath, TextureSampler a_Sampler )
{
    // Use stb_image to load the image file into memory, then create a GPU texture from it and return a handle to that texture.

    int width, height, channels;
    stbi_uc* data = stbi_load( a_FilePath, &width, &height, &channels, 4 ); // Force 4 channels (RGBA)
    if ( !data )
    {
        std::cerr << "Failed to load texture: " << a_FilePath << "\n";
        return {};
    }


    TextureHandle handle = g_Renderer->CreateTexture( 
    { 
        .Size = { (u32)width, (u32)height },
        .Format = ETextureFormat::RGBA8,
        .Sampler = a_Sampler 
    }, data );

    stbi_image_free( data );
    return handle;
}