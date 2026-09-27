/**
 * @file TestTextEdit.cpp
 * @brief Tests for TextEditModel: insertion, caret movement, deletion, selection and undo/redo.
 */

#include "TestCommon.h"
#include <RatUI/Text/TextEdit.h>

TEST_CASE( "TextEditModel starts empty", "[text-edit]" )
{
    TextEditModel model;

    REQUIRE( model.GetTextBuffer().empty() );
    REQUIRE( model.Caret() == 0 );
    REQUIRE( model.Anchor() == 0 );
}

TEST_CASE( "TextEditModel inserts and moves the caret", "[text-edit]" )
{
    TextEditModel model;

    REQUIRE( model.Insert( U"Hello" ) );
    REQUIRE( model.GetTextBuffer() == U"Hello" );
    REQUIRE( model.Caret() == 5 );
    REQUIRE( model.Anchor() == 5 );

    model.MoveLeft();
    REQUIRE( model.Caret() == 4 );
    model.MoveRight();
    REQUIRE( model.Caret() == 5 );
    model.MoveHome();
    REQUIRE( model.Caret() == 0 );
    model.MoveEnd();
    REQUIRE( model.Caret() == 5 );

    model.SetCaret( 2 );
    REQUIRE( model.Insert( U"XX" ) );
    REQUIRE( model.GetTextBuffer() == U"HeXXllo" );
    REQUIRE( model.Caret() == 4 );
}

TEST_CASE( "TextEditModel backspace and delete", "[text-edit]" )
{
    TextEditModel model;
    model.SetTextBuffer( U"HeXXllo" );
    model.SetCaret( 4 );

    REQUIRE( model.Backspace() );
    REQUIRE( model.GetTextBuffer() == U"HeXllo" );

    model.SetCaret( 2 );
    REQUIRE( model.Delete() );
    REQUIRE( model.GetTextBuffer() == U"Hello" );
}

TEST_CASE( "TextEditModel replaces the selection and supports undo/redo", "[text-edit]" )
{
    TextEditModel model;
    REQUIRE( model.Insert( U"Hello" ) );

    model.SelectAll();
    REQUIRE( model.HasSelection() );
    REQUIRE( model.ReplaceSelection( U"World" ) );
    REQUIRE( model.GetTextBuffer() == U"World" );
    REQUIRE_FALSE( model.HasSelection() );
    REQUIRE( model.Caret() == 5 );

    REQUIRE( model.Undo() );
    REQUIRE( model.GetTextBuffer() == U"Hello" );
    REQUIRE( model.Redo() );
    REQUIRE( model.GetTextBuffer() == U"World" );
}

TEST_CASE( "TextEditModel moves between lines", "[text-edit]" )
{
    TextEditModel model;
    model.SetTextBuffer( U"abc\n123\nXYZ" );

    model.MoveHome();
    REQUIRE( model.Caret() == 0 );

    model.MoveDown();
    model.MoveDown();
    model.MoveUp();
    REQUIRE( model.Caret() == 4 ); // Start of the second line.
}
