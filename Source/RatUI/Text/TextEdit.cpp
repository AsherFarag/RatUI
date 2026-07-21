#include <RatUI/Text/TextEdit.h>
#include <RatUI/Text/Unicode.h>

namespace RatUI
{
    // ============================================================
    // TextEditHistory
    // ============================================================

    void TextEditHistory::Reset()
    {
        ActionStack.clear();
        CharStorage.clear();
        Index = 0;
    }

    void TextEditHistory::Push( u32 a_Where, CharView a_OldText, CharView a_NewText, u32 a_Caret, u32 a_Anchor )
    {
        // A new edit invalidates redo history and the char storage backing it.
        if ( Index < ActionStack.size() )
        {
            Resize( ActionStack, Index );
            Resize( CharStorage, Empty( ActionStack ) ? 0
                : Back( ActionStack ).CharStorage 
                    + Back( ActionStack ).DeleteLength 
                    + Back( ActionStack ).InsertLength );
        }

        // If the action stack is full, remove the oldest action to make room for the new one.
        // TODO: Currently this just removes the oldest and pushes back new,
        // but we should instead treat the action stack and char storage as a circular buffer to avoid shifting all the actions and chars.
        if ( Size( ActionStack ) == Capacity( ActionStack ) )
        {
            Erase( ActionStack, Begin( ActionStack ) );
        }

        PushBack( ActionStack, Action{
            .Where        = a_Where,
            .InsertLength = static_cast<u32>( a_NewText.size() ),
            .DeleteLength = static_cast<u32>( a_OldText.size() ),
            .Caret        = a_Caret,
            .Anchor       = a_Anchor,
            .CharStorage  = static_cast<u32>( Size( CharStorage ) )
        } );

        Insert( CharStorage, End( CharStorage ), a_OldText.begin(), a_OldText.end() );
        Insert( CharStorage, End( CharStorage ), a_NewText.begin(), a_NewText.end() );

        Index = static_cast<u32>( Size( ActionStack ) );
    }

    // ============================================================
    // TextEditModel
    // ============================================================

    void TextEditModel::SetTextBuffer( TextBuffer&& a_Text ) noexcept
    {
        m_Text = std::move( a_Text );
        m_History.Reset();
        m_Caret = 0;
        m_Anchor = 0;
        m_PreferredColumnValid = false;
    }

    void TextEditModel::SetCaret( u32 a_Caret, bool a_KeepSelection ) noexcept
    {
        m_Caret = std::min( a_Caret, static_cast<u32>( m_Text.size() ) );
        if ( !a_KeepSelection )
        {
            m_Anchor = m_Caret;
        }
        m_PreferredColumnValid = false;
    }

    void TextEditModel::SelectAll() noexcept
    {
        m_Anchor = 0;
        m_Caret = static_cast<u32>( m_Text.size() );
        m_PreferredColumnValid = false;
    }

    bool TextEditModel::ReplaceSelection( TextView a_New )
    {
        if ( !HasSelection() )
        {
            return Insert( a_New );
        }

        const u32 min = SelectionStart(), max = SelectionEnd();
        const TextView oldText( m_Text.data() + min, max - min );

		// Record the action before we mutate the text, so that oldText is valid.
        m_Caret = min + static_cast<u32>( a_New.size() );
        m_Anchor = m_Caret;
        RecordAction( min, oldText, a_New );

        m_Text.replace( min, max - min, a_New );

        m_PreferredColumnValid = false;
        return true;
    }

    bool TextEditModel::Insert( TextView a_New )
    {
        if ( a_New.empty() )
        {
            return false;
        }

        if ( HasSelection() )
        {
            const u32 min = SelectionStart(), max = SelectionEnd();
            const TextView oldText( m_Text.data() + min, max - min );

            m_Caret = min + static_cast<u32>( a_New.size() );
            RecordAction( min, oldText, a_New );

            m_Text.replace( min, max - min, a_New );
        }
        else
        {
            const u32 where = m_Caret;
            m_Caret += static_cast<u32>( a_New.size() );
            RecordAction( where, TextView(), a_New );

            m_Text.insert( where, a_New );
        }

        m_Anchor = m_Caret;
        m_PreferredColumnValid = false;
        return true;
    }

    bool TextEditModel::Backspace()
    {
        if ( HasSelection() )
        {
            const u32 min = SelectionStart(), max = SelectionEnd();
            const TextView oldText( m_Text.data() + min, max - min );
            m_Caret = min;
            m_Anchor = m_Caret;
            RecordAction( min, oldText, TextView() );

            m_Text.erase( min, max - min );
            m_PreferredColumnValid = false;
            return true;
        }

        if ( m_Caret == 0 )
        {
            return false;
        }

        const TextView oldText( m_Text.data() + m_Caret - 1, 1 );
        const u32 where = m_Caret - 1;

        m_Caret = where;
        m_Anchor = m_Caret;
        RecordAction( where, oldText, TextView() );

        m_Text.erase( where, 1 );

        m_PreferredColumnValid = false;
        return true;
    }

    bool TextEditModel::Delete()
    {
        if ( HasSelection() )
        {
            const u32 min = SelectionStart(), max = SelectionEnd();
            const TextView oldText( m_Text.data() + min, max - min );

            m_Caret = min;
            m_Anchor = m_Caret;
            RecordAction( min, oldText, TextView() );

            m_Text.erase( min, max - min );

            m_PreferredColumnValid = false;
            return true;
        }

        if ( m_Caret >= m_Text.size() )
        {
            return false;
        }

        const TextView oldText( m_Text.data() + m_Caret, 1 );
        m_Anchor = m_Caret;
        RecordAction( m_Caret, oldText, TextView() );

        m_Text.erase( m_Caret, 1 );

        m_PreferredColumnValid = false;
        return true;
    }

    void TextEditModel::MoveLeft( bool a_Select )
    {
        if ( m_Caret > 0 )
        {
            m_Caret--;
            if ( !a_Select ) m_Anchor = m_Caret;
        }
        m_PreferredColumnValid = false;
    }

    void TextEditModel::MoveRight( bool a_Select )
    {
        if ( m_Caret < m_Text.size() )
        {
            m_Caret++;
            if ( !a_Select ) m_Anchor = m_Caret;
        }
        m_PreferredColumnValid = false;
    }

    void TextEditModel::MoveWordLeft( bool a_Select )
    {
        u32 pos = m_Caret;
        while ( pos > 0 &&  Unicode::IsWhitespace( m_Text[pos - 1] ) ) pos--;
        while ( pos > 0 && !Unicode::IsWhitespace( m_Text[pos - 1] ) ) pos--;

        m_Caret = pos;
        if ( !a_Select ) m_Anchor = m_Caret;
        m_PreferredColumnValid = false;
    }

    void TextEditModel::MoveWordRight( bool a_Select )
    {
        const u32 size = static_cast<u32>( m_Text.size() );
        u32 pos = m_Caret;
        while ( pos < size && !Unicode::IsWhitespace( m_Text[pos] ) ) pos++;
        while ( pos < size &&  Unicode::IsWhitespace( m_Text[pos] ) ) pos++;

        m_Caret = pos;
        if ( !a_Select ) m_Anchor = m_Caret;
        m_PreferredColumnValid = false;
    }

    void TextEditModel::MoveHome( bool a_Select )
    {
        u32 pos = m_Caret;
        while ( pos > 0 && m_Text[pos - 1] != U'\n' ) pos--;

        m_Caret = pos;
        if ( !a_Select ) m_Anchor = m_Caret;
        m_PreferredColumnValid = false;
    }

    void TextEditModel::MoveEnd( bool a_Select )
    {
        const u32 size = static_cast<u32>( m_Text.size() );
        u32 pos = m_Caret;
        while ( pos < size && m_Text[pos] != U'\n' ) pos++;

        m_Caret = pos;
        if ( !a_Select ) m_Anchor = m_Caret;
        m_PreferredColumnValid = false;
    }

    void TextEditModel::MoveUp( bool a_Select )
    {
        const u32 lineStart = LineStart( m_Caret );
        if ( lineStart == 0 )
        {
            MoveHome( a_Select );
            return;
        }

        const u32 column        = PreferredColumn();
        const u32 prevLineEnd   = lineStart - 1;
        const u32 prevLineStart = LineStart( prevLineEnd );
        const u32 prevLineLen   = prevLineEnd - prevLineStart;

        m_Caret = prevLineStart + std::min( column, prevLineLen );
        if ( !a_Select ) m_Anchor = m_Caret;
    }

    void TextEditModel::MoveDown( bool a_Select )
    {
        const u32 size = static_cast<u32>( m_Text.size() );
        const u32 lineEnd = LineEnd( m_Caret );
        if ( lineEnd >= size )
        {
            MoveEnd( a_Select );
            return;
        }

        const u32 column        = PreferredColumn();
        const u32 nextLineStart = lineEnd + 1;
        const u32 nextLineEnd   = LineEnd( nextLineStart );
        const u32 nextLineLen   = nextLineEnd - nextLineStart;

        m_Caret = nextLineStart + std::min( column, nextLineLen );
        if ( !a_Select ) m_Anchor = m_Caret;
    }

    bool TextEditModel::Undo()
    {
        if ( !m_History.CanUndo() ) return false;

        const auto& action = m_History.Undo();
        const TextView oldText( m_History.CharStorage.data() + action.CharStorage, action.DeleteLength );

        m_Text.erase( action.Where, action.InsertLength );
        m_Text.insert( action.Where, oldText );

        // Select the restored text so the edit is visible
        m_Anchor = action.Where;
        m_Caret  = action.Where + action.DeleteLength;
        m_PreferredColumnValid = false;
        return true;
    }

    bool TextEditModel::Redo()
    {
        if ( !m_History.CanRedo() ) return false;

        const auto& action = m_History.Redo();
        const TextView newText( m_History.CharStorage.data() + action.CharStorage + action.DeleteLength, action.InsertLength );

        m_Text.erase( action.Where, action.DeleteLength );
        m_Text.insert( action.Where, newText );

        m_Caret  = action.Caret;
        m_Anchor = action.Anchor;
        m_PreferredColumnValid = false;
        return true;
    }

    // ============================================================
    // ITextEditPolicy
    // ============================================================

    TextEditOutcome ITextEditPolicy::HandleKey( TextEditModel& a_Model, const TextInputEvent& a_Event )
    {
		const bool isCtrl = HasFlag( a_Event.Modifiers, EModifier::Ctrl );
		const bool isShift = HasFlag( a_Event.Modifiers, EModifier::Shift );

        switch ( a_Event.Button )
        {
            case EButtonID::KeyLeft:
                if ( isCtrl ) a_Model.MoveWordLeft( isShift );
                else          a_Model.MoveLeft( isShift );
                return TextEditOutcome::Handle();

            case EButtonID::KeyRight:
                if ( isCtrl ) a_Model.MoveWordRight( isShift );
                else          a_Model.MoveRight( isShift );
                return TextEditOutcome::Handle();

            case EButtonID::KeyHome:
                a_Model.MoveHome( isShift );
                return TextEditOutcome::Handle();

            case EButtonID::KeyEnd:
                a_Model.MoveEnd( isShift );
                return TextEditOutcome::Handle();

            case EButtonID::KeyBackspace:
                return TextEditOutcome::Change( a_Model.Backspace() );

            case EButtonID::KeyDelete:
                return TextEditOutcome::Change( a_Model.Delete() );

            case EButtonID::KeyA:
                if ( isCtrl )
                {
                    a_Model.SelectAll();
                    return TextEditOutcome::Handle();
                }
                return TextEditOutcome::Unhandle();

            case EButtonID::KeyZ:
                if ( isCtrl )
                {
                    return isShift ? TextEditOutcome::Change( a_Model.Redo() )
                                   : TextEditOutcome::Change( a_Model.Undo() );
                }
                return TextEditOutcome::Unhandle();

            case EButtonID::KeyY:
                if ( isCtrl ) return TextEditOutcome::Change( a_Model.Redo() );
                else          return TextEditOutcome::Unhandle();

            default:
                return TextEditOutcome::Unhandle();
        }
    }

} // namespace RatUI
