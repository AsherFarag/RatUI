#pragma once
#include "FontCache.h"
#include "TextUtil.h"

namespace RatUI::FreeType
{

    class TextMetrics : public ITextMetrics
    {
    public:
        TextMetrics( FontCache* a_FontCache = nullptr )
            : m_FontCache( a_FontCache )
        {}

        /** @brief Gets the underlying font cache set by the user. */
        FontCache* GetFontCache() const { return m_FontCache; }

        /** @brief Sets the font cache. */
        void SetFontCache( FontCache* a_Cache ) { m_FontCache = a_Cache; }

        PreparedText Prepare( StringView a_Text, const TextLayoutStyle& a_Style ) override
        {
            if ( !m_FontCache || !a_Style.Font.IsValid() || Empty( a_Text ) )
                return {};

            Font* font = m_FontCache->GetOrLoadFont( a_Style.Font, static_cast<u32>( a_Style.Size ) );
            if ( !font )
                return {};

            String buffer;
			StringView text = a_Text;
            if ( a_Style.Transform != ETextTransform::None )
            {
                buffer = TextUtil::ApplyTextTransform( a_Text, a_Style.Transform );
                text = buffer;
			}

            return TextLayout::Prepare( text, a_Style.Wrap,
                [font, &a_Style]( StringView sv )
                {
                    return TextUtil::MeasureLineWidth( *font, sv, a_Style );
                } );
        }

        TextMeasurement Measure( const PreparedText& a_Prepared, const TextLayoutStyle& a_Style, f32 a_MaxWidth = Limits<f32>::max() ) override
        {
            if ( !m_FontCache || !a_Style.Font.IsValid() || Empty( a_Prepared.Segments ) )
                return {}; // Return zero size if there are no segments to measure.

            Font* font = m_FontCache->GetOrLoadFont( a_Style.Font, static_cast<u32>( a_Style.Size ) );
            if ( !font )
                return {}; // Return zero size if the font couldn't be loaded.

            f32 maxLineWidth = 0.f;
            u32 lineCount    = 0;

            TextLayout::WalkLines( a_Prepared, a_MaxWidth, a_Style.MaxLines,
                [&]( u32, u32, f32 paintWidth )
                {
                    maxLineWidth = std::max( maxLineWidth, paintWidth );
                    ++lineCount;
                } );

            const f32 lineHeight  = GetLineHeight( font->GetFace(), a_Style );
			const f32 lineSpacing = a_Style.LineSpacing;
            const f32 baseline    = font->GetFace()->size->metrics.ascender / 64.f;

            TextMeasurement result;
            result.Size[0]   = lineCount > 0 ? std::clamp( maxLineWidth, 0.f, a_MaxWidth ) : 0.f;
            result.Baseline  = baseline;
            result.LineCount = lineCount;

            if ( lineCount > 0 )
            {
                const f32 descender = std::abs( font->GetFace()->size->metrics.descender / 64.f );

                result.Size[1] =
                    baseline +
                    ( static_cast<f32>( lineCount - 1 ) * lineHeight ) +
					( lineCount - 1 ) * lineSpacing +
                    descender;
            }
            else
            {
                result.Size[1] = 0.f;
            }

            return result;
        }

    protected:
        FontCache* m_FontCache{ nullptr };
    };

} // namespace RatUI::FreeType