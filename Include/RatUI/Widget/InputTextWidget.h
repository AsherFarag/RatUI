#pragma once
#include "IWidget.h"
#include "../Text/TextEdit.h"

class InputTextWidget : public IWidget
{
public:
    using TextBuffer = TextEditModel::TextBuffer;

    Unique<ITextEditPolicy>     Policy;
    Callback<const TextBuffer&> OnChanged; ///< Called whenever the buffer changes.
    Callback<const TextBuffer&> OnSubmit;  ///< Called on Enter in single-line mode.

    InputTextWidget( Unique<ITextEditPolicy> a_Policy = MakeUnique<SingleLineTextEditPolicy>() )
        : Policy( std::move( a_Policy ) )
    {}

    const TextEditModel& GetModel()  const noexcept { return m_Model; }

    bool IsInteractable() const override { return true; }
    bool IsFocusable()    const override { return true; }

    Reply OnTextInput( const TextInputEvent& a_Event ) override
    {
        if ( !Policy )
            return Reply::Unhandled();

        const TextEditOutcome outcome = Policy->HandleKey( m_Model, a_Event );
        if ( outcome.Handled )
        {
            if ( outcome.Changed && OnChanged )
                OnChanged( m_Model.GetTextBuffer() );

            if ( outcome.Commited && OnSubmit )
                OnSubmit( m_Model.GetTextBuffer() );

            GetLayout().MarkDirty();
        }

        return Reply::Handled();
    }

protected:

    void OnPaint( const PaintEvent& a_Event ) override
    {}

protected:
    TextEditModel           m_Model;
};

template<typename ValueType>
    requires ( std::floating_point<ValueType> || std::integral<ValueType> )
class InputValueWidget : public IWidget
{};