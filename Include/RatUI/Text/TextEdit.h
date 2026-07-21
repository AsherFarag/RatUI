#pragma once
#include "../Core/Types.h"
#include "../Input/InputEvent.h"

namespace RatUI
{

    // TODO: Support grapheme clusters
    // TODO: Use more efficient data structure for text buffer (e.g., rope)
    // TODO: Multiline text edits and undo/redo
    // TODO: This doesnt use the user defined types for string as that doesnt support template char types like std::basic_string

    /**
     * @brief A history of text edit actions, allowing for undo and redo functionality.
     * Used by TextEditModel to track changes to the text buffer.
     */
    struct TextEditHistory
    {
        using Char     = codepoint;
        using CharView = std::basic_string_view<Char>;

        struct Action
        {
            u32 Where;        ///< Where in the buffer the action occurred
            u32 InsertLength; ///< Number of characters inserted (0 for delete)
            u32 DeleteLength; ///< Number of characters deleted (0 for insert)
            u32 Caret;        ///< Caret position after the action
            u32 Anchor;       ///< Anchor position after the action
            u32 CharStorage{ Limits<u32>::max() }; ///< Offset into CharStorage: [old text][new text]
        };

        Array<Action> ActionStack;
        Array<Char>   CharStorage;
        u32           Index{ 0 }; ///< Number of actions currently applied (undo point)

        TextEditHistory( u32 a_MaxActions, u32 a_MaxCharStorage )
        {
            ActionStack.reserve( a_MaxActions );
            CharStorage.reserve( a_MaxCharStorage );
        }

        void Reset();

        bool CanUndo() const noexcept { return Index > 0; }
        bool CanRedo() const noexcept { return Index < ActionStack.size(); }

        const Action& Undo() noexcept { return ActionStack[--Index]; }
        const Action& Redo() noexcept { return ActionStack[Index++]; }

        void Push( u32 a_Where, CharView a_OldText, CharView a_NewText, u32 a_Caret, u32 a_Anchor );
    };

    /**
     * @brief A model representing the state of a text edit operation, including the text buffer, caret position, selection, and edit history.
     * Provides methods for manipulating the text, moving the caret, and performing undo/redo operations.
     * This class is designed to be used with a text edit widget, which handles rendering and user input.
     * It does not handle any rendering or input directly, but provides the necessary state and operations for a text edit widget to function.
     */
    class TextEditModel
    {
    public:
        using Char       = codepoint;
        using TextBuffer = std::basic_string<Char>;
        using TextView   = std::basic_string_view<Char>;

        const TextBuffer& GetTextBuffer() const noexcept { return m_Text; }
        void SetTextBuffer( TextBuffer&& a_Text ) noexcept;

        // --- Caret and selection ---

        u32  Caret()  const noexcept { return m_Caret; }
        u32  Anchor() const noexcept { return m_Anchor; }

        bool HasSelection()   const noexcept { return m_Caret != m_Anchor; }
        u32  SelectionStart() const noexcept { return std::min( m_Caret, m_Anchor ); }
        u32  SelectionEnd()   const noexcept { return std::max( m_Caret, m_Anchor ); }

        void SetCaret( u32 a_Caret, bool a_KeepSelection = false ) noexcept;
        void SelectAll() noexcept;

        void ClearSelection() noexcept { m_Anchor = m_Caret; }

        // --- Text manipulation ---

        bool ReplaceSelection( TextView a_New );

        bool Insert( TextView a_New );
        bool Insert( codepoint a_Char ) { return Insert( TextView( &a_Char, 1 ) ); }

        bool Backspace();
        bool Delete();

        // --- Navigation and selection movement ---

        void MoveLeft( bool a_Select = false );
        void MoveRight( bool a_Select = false );
        void MoveWordLeft( bool a_Select = false );
        void MoveWordRight( bool a_Select = false );
        void MoveHome( bool a_Select = false );
        void MoveEnd( bool a_Select = false );
        void MoveUp( bool a_Select = false );
        void MoveDown( bool a_Select = false );

        // --- Undo/redo ---

        bool Undo();
        bool Redo();

    protected:

        u32 LineStart( u32 a_Pos ) const noexcept
        {
            u32 pos = std::min( a_Pos, static_cast<u32>( m_Text.size() ) );
            while ( pos > 0 && m_Text[pos - 1] != U'\n' ) 
                pos--;
            return pos;
        }

        u32 LineEnd( u32 a_Pos ) const noexcept
        {
            const u32 size = static_cast<u32>( m_Text.size() );
            u32 pos = std::min( a_Pos, size );
            while ( pos < size && m_Text[pos] != U'\n' ) pos++;
            return pos;
        }

        u32 PreferredColumn() noexcept
        {
            if ( !m_PreferredColumnValid )
            {
                m_PreferredColumn = m_Caret - LineStart( m_Caret );
                m_PreferredColumnValid = true;
            }
            return m_PreferredColumn;
        }

        void RecordAction( u32 a_Where, TextView a_OldText, TextView a_NewText )
        {
            // Called after the mutation and caret/anchor update, so m_Caret/m_Anchor
            // here are the post-edit values Redo() will restore.
            m_History.Push( a_Where, a_OldText, a_NewText, m_Caret, m_Anchor );
        }

        TextBuffer      m_Text;                 ///< The text buffer being edited.
        TextEditHistory m_History{ 100, 4096 }; ///< History of text edit actions for undo/redo functionality.

        u32  m_Caret{ 0 };                    ///< Current position of the caret (in codepoints).
        u32  m_Anchor{ 0 };                   ///< Current position of the selection anchor (in codepoints).
        u32  m_PreferredColumn{ 0 };          ///< Preferred column for vertical caret movement (in codepoints).
        bool m_PreferredColumnValid{ false }; ///< Whether m_PreferredColumn is valid (i.e., whether the caret has moved horizontally since the last vertical movement).
    };

    /**
     * @brief Represents the outcome of a text edit operation, indicating whether the operation was handled, 
     * changed the text, committed the edit, or canceled the edit.
     * This is used by text edit policies to communicate the result of handling a key event or other input.
     */
    struct TextEditOutcome
    {
        bool Handled  = false; ///< Whether the operation was handled/consumed by the text edit policy (true) or should be passed to other handlers (false).
        bool Changed  = false; ///< Whether the text was changed as a result of the operation (true) or not (false).
        bool Commited = false; ///< Whether the edit was committed (true) or not (false). For example, pressing Enter in a single-line text edit would commit the edit.
        bool Canceled = false; ///< Whether the edit was canceled (true) or not (false). For example, pressing Escape in a text edit would cancel the edit.

        static constexpr TextEditOutcome Unhandle()                       { return { false, false, false, false }; }
        static constexpr TextEditOutcome Handle()                         { return { true,  false, false, false }; }
        static constexpr TextEditOutcome Change( bool a_Changed  = true ) { return { true,  a_Changed, false, false }; }
        static constexpr TextEditOutcome Commit( bool a_Commited = true ) { return { true,  false, a_Commited, false }; }
        static constexpr TextEditOutcome Cancel( bool a_Canceled = true ) { return { true,  false, false, a_Canceled }; }
    };

    /**
     * @brief Interface for defining text edit policies, 
     * which determine how text input and key events are handled in a text edit widget.
     * For example, when creating a text input widget, you would use a text edit policy,
     * in conjunction with a TextEditModel, to define how the widget behaves in response to user input.
     */
    class ITextEditPolicy
    {
    public:
        virtual ~ITextEditPolicy() = default;

        /** @brief Returns whether the text edit policy allows for multi-line text input. */
        virtual bool MultiLine() const = 0;

        /** @brief Returns whether the given character is accepted by the text edit policy. */
        virtual bool AcceptChar( codepoint a_Char ) const = 0;

        /** 
         * @brief Handles a key event operation on a TextEditModel, returning a TextEditOutcome indicating the result of the operation.
         * @param a_Model The TextEditModel to operate on.
         * @param a_Event The TextInputEvent representing the key event to handle.
         * @return A TextEditOutcome indicating whether the operation was handled, changed the text, committed the edit, or canceled the edit.
         */
        virtual TextEditOutcome HandleKey( TextEditModel& a_Model, const TextInputEvent& a_Event );
    };

    /**
     * @brief A text edit policy that allows for single-line text input,
     * where pressing Enter commits the edit and pressing Escape cancels it.
     * This policy does not allow for multi-line input, and ignores Up/Down arrow keys to prevent navigation in multi-line text.
     */
    class SingleLineTextEditPolicy : public ITextEditPolicy
    {
    public:
        bool MultiLine() const override { return false; }

        bool AcceptChar( codepoint a_Char ) const override
        {
            return a_Char != U'\n' && a_Char != U'\r';
        }

        TextEditOutcome HandleKey( TextEditModel& a_Model, const TextInputEvent& a_Event ) override
        {
            switch ( a_Event.Button )
            {
                case EButtonID::KeyEnter:  return TextEditOutcome::Commit();
                case EButtonID::KeyEscape: return TextEditOutcome::Cancel();
                case EButtonID::KeyUp:
                case EButtonID::KeyDown:
                    return TextEditOutcome::Handle(); // no-op, but consume so focus/nav doesn't react
                default:
                    return ITextEditPolicy::HandleKey( a_Model, a_Event );
            }
        }
    };

    /**
     * @brief A text edit policy that allows for multi-line text input,
     * where pressing Enter inserts a newline character, and pressing Escape cancels the edit.
     * This policy allows for Up/Down arrow keys to move the caret vertically through the text.
     * It also allows for text selection and navigation within the multi-line text.
     */
    class MultiLineTextEditPolicy : public ITextEditPolicy
    {
    public:
        bool MultiLine() const override { return true; }

        bool AcceptChar( codepoint a_Char ) const override { return true; }

        TextEditOutcome HandleKey( TextEditModel& a_Model, const TextInputEvent& a_Event ) override
        {
            switch ( a_Event.Button )
            {
                case EButtonID::KeyEnter:
                    return TextEditOutcome::Change( a_Model.Insert( U'\n' ) );

                case EButtonID::KeyEscape:
                    return TextEditOutcome::Cancel();

                case EButtonID::KeyUp:
					a_Model.MoveUp( HasFlag( a_Event.Modifiers, EModifier::Shift ) );
                    return TextEditOutcome::Handle();

                case EButtonID::KeyDown:
					a_Model.MoveDown( HasFlag( a_Event.Modifiers, EModifier::Shift ) );
                    return TextEditOutcome::Handle();

                default:
                    return ITextEditPolicy::HandleKey( a_Model, a_Event );
            }
        }
    };

    /**
     * @brief A text edit policy that allows for numeric input only,
     * where only digits, a minus sign, and (for floating-point types) a decimal point are accepted.
     * Pressing Enter commits the edit, and pressing Escape cancels it.
     * This policy does not allow for multi-line input, and ignores Up/Down arrow keys to prevent navigation in multi-line text.
     * It is templated on the numeric type (integral or floating-point) to determine which characters are accepted.
     * For example, NumericTextEdit
     */
    template<typename ValueType>
        requires ( std::floating_point<ValueType> || std::integral<ValueType> )
    class NumericTextEditPolicy : public ITextEditPolicy
    {
    public:
        bool MultiLine() const override { return false; }

        bool AcceptChar( codepoint a_Char ) const override
        {
            if ( a_Char >= U'0' && a_Char <= U'9' ) 
                return true;

            // Allow minus sign for negative numbers
            if constexpr ( std::signed_integral<ValueType> )
            {
                return a_Char == U'-';
            }
            // Allow decimal point for floating-point numbers
            else if constexpr ( std::floating_point<ValueType> )
            {
                return a_Char == U'.';
            }

            return false;
        }

        TextEditOutcome HandleKey( TextEditModel& a_Model, const TextInputEvent& a_Event ) override
        {
            switch ( a_Event.Button )
            {
                case EButtonID::KeyEnter:  return TextEditOutcome::Commit();
                case EButtonID::KeyEscape: return TextEditOutcome::Cancel();
                case EButtonID::KeyUp:
                case EButtonID::KeyDown:
                    return TextEditOutcome::Unhandle(); // leave to owning widget (step value up/down)

                default:
                    return ITextEditPolicy::HandleKey( a_Model, a_Event );
            }
        }
    };

} // namespace RatUI
