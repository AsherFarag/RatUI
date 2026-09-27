#pragma once
#include "Example.h"
#include <RatUI/Widget/PanelWidget.h>
#include <RatUI/Widget/TextWidget.h>
#include <RatUI/Text/SpriteSheetFont.h>
#include <RatUI/Text/FontLibrary.h>

#include <array>
#include <cmath>
#include <format>

/** @brief Font system demo: families, weights, rich text, render modes, pixel fonts and fast text. */
class FontShowcase : public Example
{
public:
    explicit FontShowcase( TextMetrics& a_TextMetrics )
        : Example( a_TextMetrics )
    {
        FontLibrary& fonts = a_TextMetrics.GetFontLibrary();
        m_Digits = RegisterDigitsFont( fonts );

        m_Scene.DefaultTheme = Themes::Dark();

        PanelWidget* root = m_Scene.CreateRootWidget<PanelWidget>();
        root->GetLayout()
            .LayoutType( ELayoutType::Vertical )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Flex )
            .Padding( Edges::All( 16_u ) )
            .Spacing( 10_u );

        AddLabel( root, StyledText{
            .Content = { "Font System  families / weights / rich text / pixel fonts" },
            .Spans   = {
                { 0, 11,  { .Weight = EFontWeight::Bold } },
                { 11, 57, { .Size = 14_u, .FillColor = Colors::TextSecondary } },
            } }, 28_u );

        PanelWidget* columns = Row( root, 12_u );
        columns->GetLayout().HeightMode( ESizing::Flex ).FlexGrow( 1.f );

        PanelWidget* left  = Column( columns );
        PanelWidget* right = Column( columns );

        BuildFamilies( fonts, left );
        BuildRichText( left );
        BuildRenderModes( left );
        BuildPixelFonts( fonts, right );
        BuildFastTextArea( right );
    }

    void Update( f32 a_DeltaSeconds ) override
    {
        m_Time += a_DeltaSeconds;

        // Spawn damage numbers, alternating between the sprite-sheet and pixel TTF fonts.
        m_SpawnTimer -= a_DeltaSeconds;
        if ( m_SpawnTimer <= 0.f )
        {
            m_SpawnTimer = 0.25f;
            DamageNumber& number = m_Numbers[m_NextNumber++ % Size( m_Numbers )];
            number.Age      = 0.f;
            number.X        = 0.1f + 0.8f * Hash01( m_NextNumber * 7u );
            number.Y        = 0.4f + 0.5f * Hash01( m_NextNumber * 13u );
            number.Value    = 1 + static_cast<i32>( Hash01( m_NextNumber * 31u ) * 999.f );
            number.Critical = Hash01( m_NextNumber * 17u ) > 0.8f;
            number.UseDigits = ( m_NextNumber % 2 ) == 0;
        }

        for ( DamageNumber& number : m_Numbers )
            number.Age += a_DeltaSeconds;
    }

    void Render( DrawList& a_DrawList, f32 a_DeltaSeconds ) override
    {
        m_Scene.Render( a_DrawList, a_DeltaSeconds );

        if ( !m_FastTextArea )
            return;

        // Fast text: no widgets or layout, just a string at a point.
        const Rect<Unit> area = m_FastTextArea->GetLayout().Layout.FinalRect;

        for ( const DamageNumber& number : m_Numbers )
        {
            if ( number.Age < 0.f || number.Age > 1.2f )
                continue;

            const f32 t      = number.Age / 1.2f;
            const f32 alpha  = 1.f - t * t;
            const f32 rise   = 40.f * t;

            TextRenderStyle render{};
            render.Align        = ETextAlign::Center;
            render.FillColor    = number.Critical ? Colors::AccentAmber : Colors::White;
            render.FillColor[3] = static_cast<u8>( alpha * 255.f );
            render.Outline      = true;
            render.OutlineColor = Colors::Black;
            render.Shadow       = number.Critical;
            render.ShadowColor  = Colors::AccentRose;

            const FastTextStyle style{
                .Family = number.UseDigits ? m_Digits : m_Minecraft,
                .Weight = number.Critical ? EFontWeight::Bold : EFontWeight::Regular,
                .Size   = number.Critical ? 32_u : 16_u,
            };

            const Vec2<Unit> anchor{
                area.Origin[0] + area.Size[0] * number.X,
                area.Origin[1] + area.Size[1] * number.Y - Unit{ rise },
            };

            const String text = number.Critical ? std::format( "{}!", number.Value ) : std::format( "-{}", number.Value );
            a_DrawList.AddFastText( text, anchor, style, render );
        }

        // A live counter drawn with fast text too.
        TextRenderStyle counter{};
        counter.FillColor = Colors::AccentEmerald;
        a_DrawList.AddFastText( std::format( "{:.2f}", m_Time ), area.Origin + Vec2<Unit>{ 10_u, 30_u },
                                FastTextStyle{ .Family = m_Digits, .Size = 16_u }, counter );
    }

private:
    struct DamageNumber
    {
        f32  Age{ -1.f };
        f32  X{ 0.f }, Y{ 0.f };
        i32  Value{ 0 };
        bool Critical{ false };
        bool UseDigits{ false };
    };

    FontFamilyHandle m_Roboto{ Fonts::Roboto }, m_RobotoRaster{ Fonts::RobotoRaster }, m_Minecraft{ Fonts::Minecraft }, m_Digits;
    PanelWidget*     m_FastTextArea{ nullptr };
    std::array<DamageNumber, 16> m_Numbers{};
    u32 m_NextNumber{ 0 };
    f32 m_SpawnTimer{ 0.f };
    f32 m_Time{ 0.f };

    static f32 Hash01( u32 a_Seed )
    {
        a_Seed = ( a_Seed ^ 61u ) ^ ( a_Seed >> 16 );
        a_Seed *= 9u;
        a_Seed ^= a_Seed >> 4;
        a_Seed *= 0x27d4eb2du;
        a_Seed ^= a_Seed >> 15;
        return static_cast<f32>( a_Seed & 0xFFFF ) / 65535.f;
    }

    // ---------------------------------------------------------------------
    // Layout helpers
    // ---------------------------------------------------------------------

    PanelWidget* Row( IWidget* a_Parent, Unit a_Spacing = 8_u )
    {
        PanelWidget* row = m_Scene.CreateWidget<PanelWidget>( a_Parent->GetLayoutID() );
        row->GetLayout()
            .LayoutType( ELayoutType::Horizontal )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Content )
            .Spacing( a_Spacing );
        return row;
    }

    PanelWidget* Column( IWidget* a_Parent )
    {
        PanelWidget* column = m_Scene.CreateWidget<PanelWidget>( a_Parent->GetLayoutID() );
        column->GetLayout()
            .LayoutType( ELayoutType::Vertical )
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Flex )
            .FlexGrow( 1.f )
            .Padding( Edges::All( 12_u ) )
            .Spacing( 8_u );
        return column;
    }

    TextWidget* AddLabel( IWidget* a_Parent, StyledText a_Text, Unit a_Size, FontFamilyHandle a_Family = {}, TextWrap a_Wrap = TextWrap::NoWrap() )
    {
        TextLayoutStyle layout{};
        layout.Family = a_Family;
        layout.Size   = a_Size;
        layout.Wrap   = a_Wrap;

        TextWidget* text = m_Scene.CreateWidget<TextWidget>( a_Parent->GetLayoutID(), std::move( a_Text ), layout );
        text->RenderStyle.FillColor = Colors::TextPrimary;
        text->GetLayout().WidthMode( ESizing::Flex ).HeightMode( ESizing::Content );
        return text;
    }

    TextWidget* AddLabel( IWidget* a_Parent, String a_Text, Unit a_Size, FontFamilyHandle a_Family = {}, TextWrap a_Wrap = TextWrap::NoWrap() )
    {
        return AddLabel( a_Parent, StyledText{ Text{ std::move( a_Text ) } }, a_Size, a_Family, a_Wrap );
    }

    void Heading( IWidget* a_Parent, String a_Text )
    {
        TextWidget* heading = AddLabel( a_Parent, std::move( a_Text ), 12_u );
        heading->RenderStyle.FillColor = Colors::TextSecondary;
        heading->SetLayoutStyle( TextLayoutStyle{ .Weight = EFontWeight::Bold, .Size = 12_u, .LetterSpacing = 1_u,
                                                  .Wrap = TextWrap::NoWrap(), .Transform = ETextTransform::Uppercase } );
    }

    // ---------------------------------------------------------------------
    // Sections
    // ---------------------------------------------------------------------

    /** @brief Shows what each weight / style request resolves to, and whether it was synthesized. */
    void BuildFamilies( FontLibrary& a_Fonts, IWidget* a_Parent )
    {
        Heading( a_Parent, "Families, weights & synthesis" );

        // "Roboto Raster" has no italic face, so its italic is synthesized.
        struct Request { FontFamilyHandle Family; EFontWeight Weight; EFontStyle Style; const char* Label; };
        const Request requests[] = {
            { m_Roboto,       EFontWeight::Regular, EFontStyle::Normal, "Regular" },
            { m_Roboto,       EFontWeight::Medium,  EFontStyle::Normal, "Medium" },
            { m_Roboto,       EFontWeight::Bold,    EFontStyle::Normal, "Bold" },
            { m_Roboto,       EFontWeight::Regular, EFontStyle::Italic, "Italic" },
            { m_Roboto,       EFontWeight::Black,   EFontStyle::Italic, "Black Italic" },
            { m_RobotoRaster, EFontWeight::Bold,    EFontStyle::Italic, "Raster Bold Italic" },
        };

        for ( const Request& request : requests )
        {
            const ResolvedFace resolved = a_Fonts.Resolve( FontQuery{ request.Family, request.Weight, request.Style } );
            const IFontFace*   face     = a_Fonts.GetFace( resolved.Face );
            const String note = std::format( "  -> {}{}{}", face ? String{ face->GetDebugName() } : String{ "<none>" },
                                             resolved.SynthBold ? " + faux bold" : "", resolved.SynthItalic ? " + faux italic" : "" );

            TextLayoutStyle layout{ .Family = request.Family, .Weight = request.Weight, .Style = request.Style, .Size = 18_u, .Wrap = TextWrap::NoWrap() };
            const String label = std::format( "{} The quick brown fox", request.Label );
            const u32    split = static_cast<u32>( Size( label ) );

            StyledText styled{
                .Content = { label + note },
                .Spans   = { { split, split + static_cast<u32>( Size( note ) ),
                               { .Size = 12_u, .Weight = EFontWeight::Regular, .Style = EFontStyle::Normal, .FillColor = Colors::TextSecondary } } },
            };

            TextWidget* text = m_Scene.CreateWidget<TextWidget>( a_Parent->GetLayoutID(), std::move( styled ), layout );
            text->RenderStyle.FillColor = Colors::TextPrimary;
            text->GetLayout().WidthMode( ESizing::Flex ).HeightMode( ESizing::Content );
        }
    }

    void BuildRichText( IWidget* a_Parent )
    {
        Heading( a_Parent, "Rich text runs" );

        StyledText text{
            .Content = { "Runs can be bold, italic, coloured, underlined, struck, BIG or small, and even switch to a PIXEL FONT "
                         "mid-sentence. Words split across runs like word never break between them. Line heights follow the runs on each line." },
            .Spans   = {
                { 12, 16,   { .Weight = EFontWeight::Bold } },
                { 18, 24,   { .Style = EFontStyle::Italic } },
                { 26, 34,   { .FillColor = Colors::AccentSky } },
                { 36, 46,   { .Decorations = ETextDecoration::Underline } },
                { 48, 54,   { .FillColor = Colors::AccentRose, .Decorations = ETextDecoration::Strikethrough } },
                { 56, 59,   { .Size = 30_u, .FillColor = Colors::AccentAmber } },
                { 63, 68,   { .Size = 11_u } },
                { 91, 101,  { .Family = m_Minecraft, .Size = 16_u, .FillColor = Colors::AccentEmerald } },
                { 145, 147, { .Weight = EFontWeight::Bold } },
                { 147, 149, { .Style = EFontStyle::Italic } },
            },
        };

        AddLabel( a_Parent, std::move( text ), 18_u, m_Roboto, TextWrap::WrapWord() );
    }

    void BuildRenderModes( IWidget* a_Parent )
    {
        Heading( a_Parent, "MTSDF vs Raster at small sizes" );

        for ( const Unit size : { 11_u, 13_u, 16_u } )
        {
            PanelWidget* row = Row( a_Parent, 16_u );
            AddLabel( row, std::format( "MTSDF {}px Hamburg 0123", static_cast<i32>( size.ToFloat() ) ), size, m_Roboto );
            AddLabel( row, std::format( "Raster {}px Hamburg 0123", static_cast<i32>( size.ToFloat() ) ), size, m_RobotoRaster );
        }
    }

    void BuildPixelFonts( FontLibrary& a_Fonts, IWidget* a_Parent )
    {
        Heading( a_Parent, "Pixel fonts (integer scale, snapped)" );

        const IFontFace* face = a_Fonts.GetFace( a_Fonts.Resolve( FontQuery{ m_Minecraft } ).Face );
        const f32 native = face ? face->Metrics().NativePixelSize : 8.f;

        AddLabel( a_Parent, std::format( "Minecraft.ttf native grid: {:.2f} px/em (auto-detected, font size snaps to whole multiples)", native ), 12_u );

        for ( u32 scale = 1; scale <= 3; ++scale )
        {
            TextWidget* text = AddLabel( a_Parent, std::format( "{}x  Pixel perfect text!", scale ), Unit{ native * static_cast<f32>( scale ) }, m_Minecraft );
            text->RenderStyle.FillColor = Colors::White;
        }

        StyledText effectsText{
            .Content = { "Outline + shadow, bold, italic and underline" },
            .Spans   = {
                { 18, 22, { .Weight = EFontWeight::Bold } },
                { 24, 30, { .Style = EFontStyle::Italic } },
                { 35, 44, { .Decorations = ETextDecoration::Underline } },
            },
        };

        TextWidget* effects = AddLabel( a_Parent, std::move( effectsText ), Unit{ native * 2.f }, m_Minecraft, TextWrap::WrapWord() );
        effects->RenderStyle.FillColor    = Colors::AccentAmber;
        effects->RenderStyle.Outline      = true;
        effects->RenderStyle.OutlineColor = Colors::Black;
        effects->RenderStyle.Shadow       = true;
        effects->RenderStyle.ShadowColor  = FromColorF32( 0.f, 0.f, 0.f, 0.6f );
        effects->RenderStyle.ShadowOffset = { 8.f, 8.f };

        Heading( a_Parent, "Sprite-sheet font (built from bytes)" );
        TextWidget* digits = AddLabel( a_Parent, "0123456789 +42 -7!", 24_u, m_Digits );
        digits->RenderStyle.FillColor = Colors::AccentSky;
    }

    void BuildFastTextArea( IWidget* a_Parent )
    {
        Heading( a_Parent, "Fast text (DrawList::AddFastText)" );

        m_FastTextArea = m_Scene.CreateWidget<PanelWidget>( a_Parent->GetLayoutID() );
        m_FastTextArea->FillBrush = SolidBrush{ Colors::Surface800 };
        m_FastTextArea->GetLayout()
            .WidthMode( ESizing::Flex )
            .HeightMode( ESizing::Flex )
            .FlexGrow( 1.f );
    }

    // ---------------------------------------------------------------------
    // A tiny 5x7 sprite-sheet font, generated in memory (no image file needed)
    // ---------------------------------------------------------------------

    static FontFamilyHandle RegisterDigitsFont( FontLibrary& a_Fonts )
    {
        if ( const FontFamilyHandle existing = a_Fonts.FindFamily( "Digits"_id ); existing.IsValid() )
            return existing;

        constexpr StringView c_Characters = "0123456789+-!.";
        constexpr const char* c_Glyphs[][7] = {
            { ".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###." }, // 0
            { "..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###." }, // 1
            { ".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####" }, // 2
            { "#####", "...#.", "..#..", "...#.", "....#", "#...#", ".###." }, // 3
            { "...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#." }, // 4
            { "#####", "#....", "####.", "....#", "....#", "#...#", ".###." }, // 5
            { "..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###." }, // 6
            { "#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..." }, // 7
            { ".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###." }, // 8
            { ".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.." }, // 9
            { ".....", "..#..", "..#..", "#####", "..#..", "..#..", "....." }, // +
            { ".....", ".....", ".....", "#####", ".....", ".....", "....." }, // -
            { "..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.." }, // !
            { ".....", ".....", ".....", ".....", ".....", ".##..", ".##.." }, // .
        };
        constexpr u32 c_GlyphW = 5, c_GlyphH = 7, c_CellW = 6, c_CellH = 8;
        constexpr u32 c_Count = static_cast<u32>( std::size( c_Glyphs ) );

        Array<Color> pixels( c_CellW * c_Count * c_CellH, Colors::Transparent );
        for ( u32 g = 0; g < c_Count; ++g )
        {
            for ( u32 y = 0; y < c_GlyphH; ++y )
                for ( u32 x = 0; x < c_GlyphW; ++x )
                    if ( c_Glyphs[g][y][x] == '#' )
                        pixels[y * c_CellW * c_Count + g * c_CellW + x] = Colors::White;
        }

        SpriteSheetFontDesc desc;
        desc.Pixels     = Data( pixels );
        desc.ImageSize  = { c_CellW * c_Count, c_CellH };
        desc.CellSize   = { c_CellW, c_CellH };
        desc.Characters = c_Characters;
        desc.Baseline   = c_GlyphH;
        desc.SpaceAdvance = 3;
        desc.Name       = "Digits 5x7";

        const FontFamilyHandle family = a_Fonts.RegisterFamily( "Digits"_id );
        a_Fonts.AddFaceToFamily( family, MakeSpriteSheetFace( desc ) );
        return family;
    }
};
