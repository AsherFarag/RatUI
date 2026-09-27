#include "TestCommon.h"
#include <RatUI/Text/TextEdit.h>

using TextBuffer = TextEditModel::TextBuffer;

// --- Whitespace normalisation ---

TEST_CASE( "NormalizeWhitespace: collapses runs and trims the ends", "[Text]" )
{
    REQUIRE( Unicode::NormalizeWhitespace( "  Hello \t\n  World  " ) == "Hello World" );
    REQUIRE( Unicode::NormalizeWhitespace( "NoSpaces" ) == "NoSpaces" );
    REQUIRE( Unicode::NormalizeWhitespace( " \r\n\t " ).empty() );
}

TEST_CASE( "NormalizeWhitespace: treats Unicode line separators as whitespace", "[Text]" )
{
    REQUIRE( Unicode::NormalizeWhitespace( "A B C" ) == "A B C" );
}

TEST_CASE( "NormalizeWhitespacePreWrap: keeps spaces and normalises line endings", "[Text]" )
{
    REQUIRE( Unicode::NormalizeWhitespacePreWrap( "  A  B  " ) == "  A  B  " );
    REQUIRE( Unicode::NormalizeWhitespacePreWrap( "A\r\nB\rC\fD" ) == "A\nB\nC\nD" );
}

// --- TextEditModel ---

TEST_CASE( "TextEditModel: inserting and deleting at the caret", "[Text][TextEdit]" )
{
    TextEditModel model;
    model.Insert( U"Hello" );
    REQUIRE( model.GetTextBuffer() == U"Hello" );
    REQUIRE( model.Caret() == 5 );

    model.Backspace();
    REQUIRE( model.GetTextBuffer() == U"Hell" );
    REQUIRE( model.Caret() == 4 );

    model.SetCaret( 0 );
    model.Delete();
    REQUIRE( model.GetTextBuffer() == U"ell" );
    REQUIRE( model.Caret() == 0 );

    // Nothing to remove at the edges.
    REQUIRE_FALSE( model.Backspace() );
    model.SetCaret( 3 );
    REQUIRE_FALSE( model.Delete() );
}

TEST_CASE( "TextEditModel: typing replaces the selection", "[Text][TextEdit]" )
{
    TextEditModel model;
    model.SetTextBuffer( TextBuffer{ U"Hello World" } );

    model.SetCaret( 6 );
    model.SetCaret( 11, true );
    REQUIRE( model.HasSelection() );
    REQUIRE( model.SelectionStart() == 6 );
    REQUIRE( model.SelectionEnd() == 11 );

    model.Insert( U"RatUI" );
    REQUIRE( model.GetTextBuffer() == U"Hello RatUI" );
    REQUIRE_FALSE( model.HasSelection() );

    model.SelectAll();
    model.Backspace();
    REQUIRE( model.GetTextBuffer().empty() );
}

TEST_CASE( "TextEditModel: word and line movement", "[Text][TextEdit]" )
{
    TextEditModel model;
    model.SetTextBuffer( TextBuffer{ U"one two\nthree" } );
    model.SetCaret( 0 );

    model.MoveWordRight();
    REQUIRE( model.Caret() == 4 );

    model.MoveEnd();
    REQUIRE( model.Caret() == 7 );

    model.MoveDown();
    REQUIRE( model.Caret() == 13 );

    model.MoveHome( true );
    REQUIRE( model.Caret() == 8 );
    REQUIRE( model.Anchor() == 13 );

    model.MoveWordLeft();
    REQUIRE( model.Caret() == 4 );
    REQUIRE_FALSE( model.HasSelection() );
}

TEST_CASE( "TextEditModel: undo and redo restore text and caret", "[Text][TextEdit]" )
{
    TextEditModel model;
    model.Insert( U"Hello" );
    model.Insert( U" World" );
    model.Backspace();
    REQUIRE( model.GetTextBuffer() == U"Hello Worl" );

    REQUIRE( model.Undo() );
    REQUIRE( model.GetTextBuffer() == U"Hello World" );

    REQUIRE( model.Undo() );
    REQUIRE( model.GetTextBuffer() == U"Hello" );
    REQUIRE( model.Caret() == 5 );

    REQUIRE( model.Redo() );
    REQUIRE( model.GetTextBuffer() == U"Hello World" );
    REQUIRE( model.Caret() == 11 );

    // A new edit discards the redo history.
    model.Insert( U'!' );
    REQUIRE_FALSE( model.Redo() );
    REQUIRE( model.GetTextBuffer() == U"Hello World!" );
}
