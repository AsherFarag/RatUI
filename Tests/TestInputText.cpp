#include "TestSceneCommon.h"
#include <catch2/catch_approx.hpp>
#include <RatUI/Backends/Null.h>

#if RATUI_BACKEND_FREETYPE
    #include <RatUI/Backends/FreeType/FontCache.h>
    #include <RatUI/Backends/FreeType/TextMetrics.h>
    #include <filesystem>
#endif

namespace
{
    /** @brief A field with its own look and none of InputTextWidget's: it counts what the base asks of it. */
    struct UnderlineField : InputTextBaseWidget
    {
        using InputTextBaseWidget::InputTextBaseWidget;

        int        Paints{ 0 };
        int        ThemeApplies{ 0 };
        int        SelectionRects{ 0 };
        Rect<Unit> Caret{};

    protected:
        void ApplyTheme( const ThemeHandle& a_Theme ) override
        {
            InputTextBaseWidget::ApplyTheme( a_Theme );
            ++ThemeApplies;
        }

        void OnPaintField( const PaintEvent& ) override
        {
            ++Paints;
            SelectionRects = 0;
            ForEachSelectionRect( [this]( const Rect<Unit>& ) { ++SelectionRects; } );
            Caret = GetCaretRect();
        }
    };

    struct FieldFixture : NavFixture
    {
        Null::NullRenderer    Renderer;
        Null::NullTextMetrics NullMetrics;
        GlyphAtlas            Atlas{ Renderer, NullMetrics };
        DrawList              Drawer{ Atlas };

        void Frame()
        {
            Layout();
            Scene.Render( Drawer, 0.f );
        }
    };
}

TEST_CASE( "InputTextBaseWidget: a custom look gets typing, focus and paint without the default style", "[Scene][Text]" )
{
    FieldFixture    f;
    UnderlineField* field = f.Scene.CreateWidget<UnderlineField>( f.Root->GetLayoutID() );
    f.Frame();
    REQUIRE( field->Paints == 1 );

    f.Tap( EButtonID::KeyDown );
    f.Tap( EButtonID::KeyEnter );
    REQUIRE( field->IsEditing() );

    f.Type( "hi" );
    REQUIRE( field->GetText() == "hi" );

    // MinWidth and CaretWidth are the base's, so a custom field still measures.
    f.Frame();
    REQUIRE( field->GetLayout().Layout.ParentRect.Size[0] >= field->MinWidth );
    REQUIRE( field->Caret.Size[0] == field->CaretWidth );
    REQUIRE( field->GetLayout().Layout.SceneRect.Contains( field->Caret.Origin ) );
}

TEST_CASE( "InputTextBaseWidget: ApplyTheme runs once per theme change", "[Scene][Text]" )
{
    FieldFixture  f;
    Shared<Theme> theme = MakeShared<Theme>();
    f.Scene.DefaultTheme = theme;

    UnderlineField* field = f.Scene.CreateWidget<UnderlineField>( f.Root->GetLayoutID() );
    f.Frame();
    f.Frame();
    REQUIRE( field->ThemeApplies == 1 ); // Measure and paint share one update.

    theme->SetColor( ThemeKey::Color::TextSelection, Colors::AccentBlue );
    f.Frame();
    REQUIRE( field->ThemeApplies == 2 );
}

TEST_CASE( "InputTextWidget: the default look reads its theme keys", "[Scene][Text]" )
{
    FieldFixture  f;
    Shared<Theme> theme = MakeShared<Theme>();
    theme->SetColor( ThemeKey::Color::TextSelection, Colors::AccentBlue );
    theme->SetMetric( ThemeKey::Metric::ButtonBorderThickness, 0_u );
    f.Scene.DefaultTheme = theme;

    InputTextWidget* field = f.Scene.CreateWidget<InputTextWidget>( f.Root->GetLayoutID() );
    f.Frame();
    REQUIRE( field->SelectionColor == Colors::AccentBlue );
    REQUIRE( field->BorderThickness == 1_u ); // A field always has a border.
}

#if RATUI_BACKEND_FREETYPE

TEST_CASE( "InputTextBaseWidget: caret and selection rects follow the shaped text", "[Scene][Text][FreeType]" )
{
    const std::filesystem::path font = std::filesystem::path( RATUI_SOURCE_DIR ) / "Examples/Resources/Fonts/Roboto-Medium.ttf";
    FreeType::FontCache cache;
    REQUIRE( cache.RegisterFontHandle( FontHandle{ 1 }, font.string() ) );
    FreeType::TextMetrics metrics{ cache };

    FieldFixture f;
    f.Scene.TextMetrics = &metrics;

    UnderlineField* field = f.Scene.CreateWidget<UnderlineField>( f.Root->GetLayoutID(), MakeUnique<MultiLineTextEditPolicy>() );
    TextLayoutStyle style{};
    style.Font = FontHandle{ 1 };
    style.Size = 16_u;
    field->SetLayoutStyle( style );
    field->SetText( "ab\ncd" );

    f.Scene.SetFocus( field->GetLayoutID() );
    f.Scene.Engage( field->GetLayoutID() );
    f.Frame();

    // The caret is at the end: after "cd", on the second line.
    const Rect<Unit> textRect = field->GetTextRect();
    REQUIRE( field->Caret.Origin[0] > textRect.Origin[0] );
    REQUIRE( field->Caret.Origin[1].ToFloat() == Catch::Approx( ( textRect.Origin[1] + field->GetLineHeight() ).ToFloat() ).margin( 2.f ) );
    REQUIRE( field->Caret.Size[1] == field->GetLineHeight() );
    REQUIRE( field->SelectionRects == 0 );

    f.Tap( EButtonID::KeyA, EModifier::Ctrl );
    f.Frame();
    REQUIRE( field->SelectionRects == 2 ); // One per line.
}

#endif
