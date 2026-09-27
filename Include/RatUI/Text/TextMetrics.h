#pragma once
#include "../Core.h"
#include "../Layout/Layout.h"
#include "Text.h"
#include "FontLibrary.h"
#include "TextShaping.h"

namespace RatUI
{
    /** @brief Rich text layout over any IFontFace in a FontLibrary. */
    class TextMetrics
    {
    public:
        explicit TextMetrics( FontLibrary& a_Fonts ) : m_Fonts( a_Fonts ) {}

        /** @brief Normalises, splits into styled runs and pre-measures text. Cache the result until the text or style changes. */
        RATUI_NODISCARD Optional<PreparedText> Prepare( const StyledTextView& a_Text, const TextLayoutStyle& a_Style );

        RATUI_NODISCARD Optional<PreparedText> Prepare( StringView a_Text, const TextLayoutStyle& a_Style )
        {
            return Prepare( StyledTextView{ a_Text }, a_Style );
        }

        /** @brief Breaks prepared text into lines that fit @p a_MaxSize and positions the glyphs. */
        RATUI_NODISCARD Optional<ShapedText> Shape( const PreparedText& a_Prepared, const TextLayoutStyle& a_Style, Vec2<Unit> a_MaxSize = { Limits<Unit>::max(), Limits<Unit>::max() } );

        FontLibrary& GetFontLibrary() const { return m_Fonts; }

        /** @brief Splits normalised text into runs of one style and face (spans + font fallback). */
        void Itemize( StringView a_Text, const TextLayoutStyle& a_Style, Span<const TextSpan> a_Spans, Array<TextRun>& o_Runs ) const;

    protected:
        /** @brief Width of a byte range, summed across the runs it touches. */
        Unit MeasureRange( const PreparedText& a_Prepared, const TextLayoutStyle& a_Style, u32 a_Start, u32 a_Length );

        Unit ShapeRunRange( const PreparedText& a_Prepared, const TextRun& a_Run, const TextLayoutStyle& a_Style,
                            u32 a_Start, u32 a_End, Array<ShapedGlyph>& o_Glyphs );

        /** @brief First run whose EndByte is past @p a_Byte. */
        static u32 FindRun( const Array<TextRun>& a_Runs, u32 a_Byte );

        FontLibrary&       m_Fonts;
        Array<ShapedGlyph> m_Scratch;
        Array<u32>         m_OffsetMap;
        Array<TextSpan>    m_RemappedSpans;
    };

} // namespace RatUI
