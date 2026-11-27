/**
 * @file TestTextEdit.cpp
 * @brief TextEditModel (editing, caret movement, selection, undo/redo), the edit policies, and DecodeEditText.
 */

#include "TestCommon.h"
#include <RatUI/Text/TextEdit.h>

using TextBuffer = TextEditModel::TextBuffer;

namespace
{
    ButtonEvent KeyPress( EButtonID a_Button, EModifier a_Modifiers = EModifier::None )
    {
        return ButtonEvent{ .Button = a_Button, .Modifiers = a_Modifiers, .Pressed = true };
    }

    bool Same( const TextEditOutcome& a_A, const TextEditOutcome& a_B )
    {
        return a_A.Handled == a_B.Handled && a_A.Changed == a_B.Changed && a_A.Commited == a_B.Commited && a_A.Canceled == a_B.Canceled;
    }
}

// ============================================================================
// TextEditModel
// ============================================================================

TEST_CASE( "TextEditModel: starts empty", "[Text][TextEdit]" )
{
    TextEditModel model;
    REQUIRE( model.GetTextBuffer().empty() );
    REQUIRE( model.Caret() == 0 );
    REQUIRE( model.Anchor() == 0 );
    REQUIRE_FALSE( model.Undo() );
    REQUIRE_FALSE( model.Redo() );
}

TEST_CASE( "TextEditModel: inserting and deleting at the caret", "[Text][TextEdit]" )
{
    TextEditModel model;
    REQUIRE( model.Insert( U"Hello" ) );
    REQUIRE( model.GetTextBuffer() == U"Hello" );
    REQUIRE( model.Caret() == 5 );
    REQUIRE( model.Anchor() == 5 );

    model.SetCaret( 2 );
    REQUIRE( model.Insert( U"XX" ) );
    REQUIRE( model.GetTextBuffer() == U"HeXXllo" );
    REQUIRE( model.Caret() == 4 );

    REQUIRE( model.Backspace() );
    REQUIRE( model.GetTextBuffer() == U"HeXllo" );
    REQUIRE( model.Caret() == 3 );

    model.SetCaret( 2 );
    REQUIRE( model.Delete() );
    REQUIRE( model.GetTextBuffer() == U"Hello" );
    REQUIRE( model.Caret() == 2 );

    // Nothing to remove at the edges.
    model.SetCaret( 0 );
    REQUIRE_FALSE( model.Backspace() );
    model.SetCaret( 5 );
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

    REQUIRE( model.Insert( U"RatUI" ) );
    REQUIRE( model.GetTextBuffer() == U"Hello RatUI" );
    REQUIRE_FALSE( model.HasSelection() );

    model.SelectAll();
    REQUIRE( model.SelectionStart() == 0 );
    REQUIRE( model.SelectionEnd() == 11 );
    REQUIRE( model.Backspace() );
    REQUIRE( model.GetTextBuffer().empty() );
}

TEST_CASE( "TextEditModel: horizontal, word and line-end movement", "[Text][TextEdit]" )
{
    TextEditModel model;
    model.SetTextBuffer( TextBuffer{ U"one two\nthree" } );
    model.SetCaret( 0 );

    model.MoveRight();
    REQUIRE( model.Caret() == 1 );
    model.MoveLeft();
    REQUIRE( model.Caret() == 0 );
    model.MoveLeft(); // Stops at the start.
    REQUIRE( model.Caret() == 0 );

    model.MoveWordRight();
    REQUIRE( model.Caret() == 4 );

    model.MoveEnd();
    REQUIRE( model.Caret() == 7 );

    // Word moves cross line breaks: the break counts as whitespace.
    model.MoveWordRight();
    REQUIRE( model.Caret() == 8 );

    model.MoveEnd( true );
    REQUIRE( model.Caret() == 13 );
    REQUIRE( model.Anchor() == 8 );

    model.MoveWordLeft();
    REQUIRE( model.Caret() == 8 );
    REQUIRE_FALSE( model.HasSelection() );

    model.MoveWordLeft();
    REQUIRE( model.Caret() == 4 );
}

TEST_CASE( "TextEditModel: selecting word moves keep the anchor", "[Text][TextEdit]" )
{
    TextEditModel model;
    model.SetTextBuffer( TextBuffer{ U"one two three" } );
    model.SetCaret( 0 );

    model.MoveWordRight( true );
    model.MoveWordRight( true );
    REQUIRE( model.Anchor() == 0 );
    REQUIRE( model.Caret() == 8 );

    model.MoveWordLeft( true );
    REQUIRE( model.Anchor() == 0 );
    REQUIRE( model.Caret() == 4 );
}

TEST_CASE( "TextEditModel: vertical movement keeps the column it started from", "[Text][TextEdit]" )
{
    TextEditModel model;
    model.SetTextBuffer( TextBuffer{ U"abcdef\nab\nabcdef" } );
    model.SetCaret( 5 );

    model.MoveDown();
    REQUIRE( model.Caret() == 9 ); // Clamped to the end of the short line.

    model.MoveDown();
    REQUIRE( model.Caret() == 15 ); // Back on column 5.

    model.MoveUp();
    model.MoveUp();
    REQUIRE( model.Caret() == 5 );

    // A horizontal move picks a new column.
    model.MoveLeft();
    model.MoveDown();
    model.MoveDown();
    REQUIRE( model.Caret() == 14 );

    // Past the last line, Down goes to the end; past the first, Up goes to the start.
    model.MoveDown();
    REQUIRE( model.Caret() == 16 );
    model.SetCaret( 3 );
    model.MoveUp();
    REQUIRE( model.Caret() == 0 );
}

TEST_CASE( "TextEditModel: undo and redo restore text and caret", "[Text][TextEdit]" )
{
    TextEditModel model;
    model.Insert( U"Hello" );
    model.Insert( U" World" );
    model.Backspace();
    REQUIRE( model.GetTextBuffer() == U"Hello Worl" );

    // Undo selects what it restored, so the edit is visible.
    REQUIRE( model.Undo() );
    REQUIRE( model.GetTextBuffer() == U"Hello World" );
    REQUIRE( model.Anchor() == 10 );
    REQUIRE( model.Caret() == 11 );

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

TEST_CASE( "TextEditModel: undo history keeps the last 100 edits, and SetTextBuffer clears it", "[Text][TextEdit]" )
{
    TextEditModel model;
    for ( int i = 0; i < 101; ++i )
        model.Insert( U'a' );

    for ( int i = 0; i < 100; ++i )
        REQUIRE( model.Undo() );
    REQUIRE_FALSE( model.Undo() );
    REQUIRE( model.GetTextBuffer() == U"a" ); // The oldest edit was dropped.

    model.SetTextBuffer( TextBuffer{ U"fresh" } );
    REQUIRE_FALSE( model.Undo() );
    REQUIRE_FALSE( model.Redo() );
}

// ============================================================================
// DecodeEditText
// ============================================================================

TEST_CASE( "DecodeEditText: every kind of line break becomes one newline", "[Text][TextEdit]" )
{
    REQUIRE( DecodeEditText( "a\r\nb\rc\fd" ) == U"a\nb\nc\nd" );
    REQUIRE( DecodeEditText( "\r\r\n" ) == U"\n\n" );
    REQUIRE( DecodeEditText( "a\xC2\x85" "b\xE2\x80\xA8" "c\xE2\x80\xA9" "d" ) == U"a\nb\nc\nd" );
    REQUIRE( DecodeEditText( "\xC3\xA9\xE2\x82\xAC" ) == TextBuffer{ codepoint{ 0xE9 }, codepoint{ 0x20AC } } );
}

// ============================================================================
// Edit policies
// ============================================================================

TEST_CASE( "ITextEditPolicy: shared editing keys", "[Text][TextEdit]" )
{
    const SingleLineTextEditPolicy policy;
    TextEditModel                  model;
    model.SetTextBuffer( TextBuffer{ U"one two" } );
    model.SetCaret( 7 );

    // Either Ctrl key moves by word.
    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyLeft, EModifier::LCtrl ) ), TextEditOutcome::Handle() ) );
    REQUIRE( model.Caret() == 4 );
    (void)policy.HandleKey( model, KeyPress( EButtonID::KeyRight, EModifier::RCtrl | EModifier::LShift ) );
    REQUIRE( model.Caret() == 7 );
    REQUIRE( model.Anchor() == 4 );

    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyBackspace ) ), TextEditOutcome::Change() ) );
    REQUIRE( model.GetTextBuffer() == U"one " );

    // Nothing to delete: handled, but nothing changed.
    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyDelete ) ), TextEditOutcome::Change( false ) ) );

    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyZ, EModifier::LCtrl ) ), TextEditOutcome::Change() ) );
    REQUIRE( model.GetTextBuffer() == U"one two" );
    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyZ, EModifier::LCtrl | EModifier::LShift ) ), TextEditOutcome::Change() ) );
    REQUIRE( model.GetTextBuffer() == U"one " );
    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyZ, EModifier::LCtrl ) ), TextEditOutcome::Change() ) );
    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyY, EModifier::LCtrl ) ), TextEditOutcome::Change() ) );
    REQUIRE( model.GetTextBuffer() == U"one " );

    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyA, EModifier::LCtrl ) ), TextEditOutcome::Handle() ) );
    REQUIRE( model.SelectionEnd() - model.SelectionStart() == 4 );

    // Letters without Ctrl are typed text, not keys.
    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyA ) ), TextEditOutcome::Unhandle() ) );
    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyZ ) ), TextEditOutcome::Unhandle() ) );
}

TEST_CASE( "SingleLineTextEditPolicy: Enter commits, Escape cancels, Up and Down are left to navigation", "[Text][TextEdit]" )
{
    const SingleLineTextEditPolicy policy;
    TextEditModel                  model;

    REQUIRE_FALSE( policy.MultiLine() );
    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyEnter ) ), TextEditOutcome::Commit() ) );
    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyEscape ) ), TextEditOutcome::Cancel() ) );
    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyUp ) ), TextEditOutcome::Unhandle() ) );
    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyDown ) ), TextEditOutcome::Unhandle() ) );
    REQUIRE( model.GetTextBuffer().empty() );

    // Pasted line breaks are dropped.
    REQUIRE( Same( policy.HandleText( model, "a\r\nb" ), TextEditOutcome::Change() ) );
    REQUIRE( model.GetTextBuffer() == U"ab" );
    REQUIRE( Same( policy.HandleText( model, "\n" ), TextEditOutcome::Unhandle() ) );
}

TEST_CASE( "MultiLineTextEditPolicy: Enter inserts a line break and Up and Down move between lines", "[Text][TextEdit]" )
{
    const MultiLineTextEditPolicy policy;
    TextEditModel                 model;

    REQUIRE( policy.MultiLine() );
    (void)policy.HandleText( model, "ab" );
    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyEnter ) ), TextEditOutcome::Change() ) );
    (void)policy.HandleText( model, "cd" );
    REQUIRE( model.GetTextBuffer() == U"ab\ncd" );

    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyUp ) ), TextEditOutcome::Handle() ) );
    REQUIRE( model.Caret() == 2 );
    (void)policy.HandleKey( model, KeyPress( EButtonID::KeyDown, EModifier::RShift ) );
    REQUIRE( model.Caret() == 5 );
    REQUIRE( model.Anchor() == 2 );

    REQUIRE( Same( policy.HandleKey( model, KeyPress( EButtonID::KeyEscape ) ), TextEditOutcome::Cancel() ) );

    // Pasted line breaks are kept, as '\n'.
    model.SetTextBuffer( TextBuffer{} );
    (void)policy.HandleText( model, "x\r\ny" );
    REQUIRE( model.GetTextBuffer() == U"x\ny" );
}

TEST_CASE( "NumericTextEditPolicy: digits, and a sign or decimal point by value type", "[Text][TextEdit]" )
{
    const NumericTextEditPolicy<int>      integer;
    const NumericTextEditPolicy<unsigned> natural;
    const NumericTextEditPolicy<float>    real;

    for ( codepoint c = U'0'; c <= U'9'; ++c )
    {
        REQUIRE( integer.AcceptChar( c ) );
        REQUIRE( natural.AcceptChar( c ) );
        REQUIRE( real.AcceptChar( c ) );
    }

    REQUIRE( integer.AcceptChar( U'-' ) );
    REQUIRE_FALSE( natural.AcceptChar( U'-' ) );
    REQUIRE_FALSE( integer.AcceptChar( U'.' ) );
    REQUIRE( real.AcceptChar( U'.' ) );
    REQUIRE_FALSE( real.AcceptChar( U'a' ) );

    TextEditModel model;
    REQUIRE( Same( natural.HandleText( model, "1a2.3" ), TextEditOutcome::Change() ) );
    REQUIRE( model.GetTextBuffer() == U"123" );
    REQUIRE( Same( natural.HandleText( model, "abc" ), TextEditOutcome::Unhandle() ) );

    REQUIRE( Same( real.HandleKey( model, KeyPress( EButtonID::KeyEnter ) ), TextEditOutcome::Commit() ) );
    REQUIRE( Same( real.HandleKey( model, KeyPress( EButtonID::KeyUp ) ), TextEditOutcome::Unhandle() ) );
}
