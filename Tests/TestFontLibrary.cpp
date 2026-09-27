#include "TestFontCommon.h"

namespace
{
    struct WeightFixture
    {
        FontLibrary      Fonts;
        FontFamilyHandle Family = Fonts.RegisterFamily( "Family"_id );

        FontFaceHandle Add( EFontWeight a_Weight, EFontStyle a_Style = EFontStyle::Normal )
        {
            return AddFixedFace( Fonts, Family, {}, { a_Weight, a_Style } );
        }

        ResolvedFace Resolve( EFontWeight a_Weight, EFontStyle a_Style = EFontStyle::Normal, FontSynthesis a_Synthesis = {} ) const
        {
            return Fonts.Resolve( FontQuery{ Family, a_Weight, a_Style, a_Synthesis } );
        }
    };

    constexpr EFontWeight W( u16 a_Weight ) { return static_cast<EFontWeight>( a_Weight ); }
}

TEST_CASE( "FontLibrary registers families and faces", "[Font][FontLibrary]" )
{
    FontLibrary fonts;
    const FontFamilyHandle a = fonts.RegisterFamily( "A"_id );
    const FontFamilyHandle b = fonts.RegisterFamily( "B"_id );

    REQUIRE( a.IsValid() );
    REQUIRE( b.IsValid() );
    REQUIRE( a != b );
    REQUIRE( fonts.RegisterFamily( "A"_id ) == a );
    REQUIRE( fonts.FindFamily( "B"_id ) == b );
    REQUIRE_FALSE( fonts.FindFamily( "Missing"_id ).IsValid() );

    SECTION( "The first family becomes the default" )
    {
        REQUIRE( fonts.GetDefaultFamily() == a );
    }

    SECTION( "Null faces are rejected" )
    {
        REQUIRE_FALSE( fonts.AddFace( nullptr ).IsValid() );
    }

    SECTION( "An unset family resolves through the default family" )
    {
        const FontFaceHandle face = AddFixedFace( fonts, b, {} );
        fonts.SetDefaultFamily( b );
        REQUIRE( fonts.Resolve( FontQuery{} ).Face == face );
    }

    SECTION( "A family without faces resolves to nothing" )
    {
        REQUIRE_FALSE( fonts.Resolve( FontQuery{ a } ).IsValid() );
    }
}

TEST_CASE( "FontLibrary weight matching follows CSS Fonts 4", "[Font][FontLibrary]" )
{
    WeightFixture f;

    SECTION( "Dense set" )
    {
        const FontFaceHandle w300 = f.Add( W( 300 ) );
        const FontFaceHandle w400 = f.Add( W( 400 ) );
        const FontFaceHandle w500 = f.Add( W( 500 ) );
        const FontFaceHandle w700 = f.Add( W( 700 ) );

        REQUIRE( f.Resolve( W( 400 ) ).Face == w400 );
        REQUIRE( f.Resolve( W( 450 ) ).Face == w500 ); // [desired, 500] ascending first
        REQUIRE( f.Resolve( W( 500 ) ).Face == w500 );
        REQUIRE( f.Resolve( W( 600 ) ).Face == w700 ); // > 500: heavier first
        REQUIRE( f.Resolve( W( 900 ) ).Face == w700 ); // nothing heavier: lighter, descending
        REQUIRE( f.Resolve( W( 350 ) ).Face == w300 ); // < 400: lighter first
        REQUIRE( f.Resolve( W( 100 ) ).Face == w300 ); // nothing lighter: heavier, ascending
    }

    SECTION( "Sparse set" )
    {
        const FontFaceHandle w300 = f.Add( W( 300 ) );
        const FontFaceHandle w700 = f.Add( W( 700 ) );

        REQUIRE( f.Resolve( W( 400 ) ).Face == w300 ); // nothing in [400, 500]: below desired, descending
        REQUIRE( f.Resolve( W( 500 ) ).Face == w300 );
        REQUIRE( f.Resolve( W( 550 ) ).Face == w700 );
    }

    SECTION( "Heavier is preferred over lighter for bold requests" )
    {
        f.Add( W( 100 ) );
        const FontFaceHandle w900 = f.Add( W( 900 ) );
        REQUIRE( f.Resolve( W( 600 ) ).Face == w900 );
    }
}

TEST_CASE( "FontLibrary style matching falls back italic -> oblique -> normal", "[Font][FontLibrary]" )
{
    WeightFixture f;

    SECTION( "Italic request uses oblique before normal, without synthesis" )
    {
        f.Add( EFontWeight::Regular, EFontStyle::Normal );
        const FontFaceHandle oblique = f.Add( EFontWeight::Regular, EFontStyle::Oblique );

        const ResolvedFace resolved = f.Resolve( EFontWeight::Regular, EFontStyle::Italic );
        REQUIRE( resolved.Face == oblique );
        REQUIRE_FALSE( resolved.SynthItalic );
    }

    SECTION( "Style narrows before weight" )
    {
        f.Add( EFontWeight::Bold, EFontStyle::Normal );
        const FontFaceHandle lightItalic = f.Add( EFontWeight::Light, EFontStyle::Italic );

        // A bold italic request prefers the italic face even though its weight is further away.
        REQUIRE( f.Resolve( EFontWeight::Bold, EFontStyle::Italic ).Face == lightItalic );
    }

    SECTION( "Normal request with only italic faces uses them, unsynthesized" )
    {
        const FontFaceHandle italic = f.Add( EFontWeight::Regular, EFontStyle::Italic );
        const ResolvedFace resolved = f.Resolve( EFontWeight::Regular, EFontStyle::Normal );
        REQUIRE( resolved.Face == italic );
        REQUIRE_FALSE( resolved.SynthItalic );
    }
}

TEST_CASE( "FontLibrary only synthesizes when no real face qualifies", "[Font][FontLibrary]" )
{
    WeightFixture f;
    const FontFaceHandle regular = f.Add( EFontWeight::Regular );

    SECTION( "Bold from regular is synthesized" )
    {
        const ResolvedFace resolved = f.Resolve( EFontWeight::Bold );
        REQUIRE( resolved.Face == regular );
        REQUIRE( resolved.SynthBold );
        REQUIRE_FALSE( resolved.SynthItalic );
    }

    SECTION( "A real bold face is never faux-bolded (no double bold)" )
    {
        const FontFaceHandle bold = f.Add( EFontWeight::Bold );
        const ResolvedFace resolved = f.Resolve( EFontWeight::Black );
        REQUIRE( resolved.Face == bold );
        REQUIRE_FALSE( resolved.SynthBold );
    }

    SECTION( "SemiBold (600) from Medium (500) is synthesized, Medium from Regular is not" )
    {
        REQUIRE( f.Resolve( EFontWeight::SemiBold ).SynthBold );
        REQUIRE_FALSE( f.Resolve( EFontWeight::Medium ).SynthBold );
    }

    SECTION( "Italic from upright is synthesized" )
    {
        const ResolvedFace resolved = f.Resolve( EFontWeight::Regular, EFontStyle::Italic );
        REQUIRE( resolved.Face == regular );
        REQUIRE( resolved.SynthItalic );
    }

    SECTION( "Query synthesis policy is respected" )
    {
        const ResolvedFace resolved = f.Resolve( EFontWeight::Bold, EFontStyle::Italic, FontSynthesis::None() );
        REQUIRE( resolved.Face == regular );
        REQUIRE_FALSE( resolved.SynthBold );
        REQUIRE_FALSE( resolved.SynthItalic );
    }

    SECTION( "Family synthesis policy is respected" )
    {
        f.Fonts.SetFamilySynthesis( f.Family, FontSynthesis{ .Bold = false, .Italic = true } );
        REQUIRE_FALSE( f.Resolve( EFontWeight::Bold ).SynthBold );
        REQUIRE( f.Resolve( EFontWeight::Regular, EFontStyle::Italic ).SynthItalic );
    }
}

TEST_CASE( "FontLibrary resolves codepoints through fallback chains", "[Font][FontLibrary]" )
{
    FontLibrary fonts;
    const FontFamilyHandle latin  = fonts.RegisterFamily( "Latin"_id );
    const FontFamilyHandle kana   = fonts.RegisterFamily( "Kana"_id );
    const FontFamilyHandle symbol = fonts.RegisterFamily( "Symbol"_id );

    const FontFaceHandle latinFace   = AddFixedFace( fonts, latin, { .Characters = U"abc" } );
    const FontFaceHandle kanaRegular = AddFixedFace( fonts, kana, { .Characters = U"\u3042" }, { EFontWeight::Regular } );
    const FontFaceHandle kanaBold    = AddFixedFace( fonts, kana, { .Characters = U"\u3042" }, { EFontWeight::Bold } );
    const FontFaceHandle symbolFace  = AddFixedFace( fonts, symbol, { .Characters = U"\u2605" } );

    fonts.SetFallbacks( latin, { kana } );
    fonts.SetGlobalFallbacks( { symbol } );

    REQUIRE( fonts.ResolveCodepoint( FontQuery{ latin }, U'a' ).Face == latinFace );
    REQUIRE( fonts.ResolveCodepoint( FontQuery{ latin }, U'\u3042' ).Face == kanaRegular );
    REQUIRE( fonts.ResolveCodepoint( FontQuery{ latin }, U'\u2605' ).Face == symbolFace );

    SECTION( "Fallbacks are matched with the same weight" )
    {
        const ResolvedFace resolved = fonts.ResolveCodepoint( FontQuery{ latin, EFontWeight::Bold }, U'\u3042' );
        REQUIRE( resolved.Face == kanaBold );
        REQUIRE_FALSE( resolved.SynthBold );
    }

    SECTION( "Unsupported codepoints stay on the primary face (drawn as .notdef)" )
    {
        REQUIRE( fonts.ResolveCodepoint( FontQuery{ latin }, U'Z' ).Face == latinFace );
    }
}
