#pragma once
#include "../../RatUI.h"
#include "../../Text/ITextMetrics.h"
#include "../FreeType/FontCache.h"
#include "../FreeType/TextUtil.h"

namespace RatUI::SDL2
{
    /**
     * @brief SDL2-backed text metrics provider.
     * Measures text using FreeType and provides shaping support (TODO).
     */
    class SDL2TextMetrics : public ITextMetrics
    {
    public:
        SDL2TextMetrics( FreeType::FontCache* a_FontCache = nullptr ) : m_FontCache( a_FontCache ) {}

        /** @brief Sets the font cache for this text metrics instance. */
        void SetFontCache( FreeType::FontCache* a_Cache ) { m_FontCache = a_Cache; }

        ShapedText Shape( TextView a_Text, const TextStyle& a_Style, f32 a_MaxWidth = Limits<f32>::max() ) override
        {
            // TODO:
            return {};
        }

        void ReleaseShapedText( const ShapedText& a_ShapedText ) override
        {
            // TODO: Release any resources associated with the ShapedText handle.
        }

        TextMeasurement Measure( TextView a_Text, const TextStyle& a_Style, f32 a_MaxWidth = Limits<f32>::max() ) override;

    private:
        FreeType::FontCache* m_FontCache;
    };

    // = Inline Impl

    inline TextMeasurement SDL2TextMetrics::Measure( TextView a_Text, const TextStyle& a_Style, f32 a_MaxWidth )
    {
        if ( !m_FontCache )
        {
            RATUI_USER_ASSERT( false, "SDL2TextMetrics requires a reference to a FreeType::FontCache for measuring text." );
            return {};
        }    

        if ( !a_Style.Font.IsValid() || a_Text.empty() )
            return {}; // Early out for invalid font or empty text.

		FreeType::Font* font = m_FontCache->GetOrLoadFont( a_Style.Font, a_Style.Size );
        if ( !font )
        {
            // TODO: Should add a LOG api and use it here
            return {}; // Early out if the font failed to load for some reason.
        }

        // - Build the line array
        Array<StringView> lines;
		Array<String> linesStorage;
        FreeType::TextUtil::BuildTextLines( *font, a_Style, a_Text, lines, linesStorage, a_MaxWidth );

        // - Measure each line and compute overall metrics
        f32 maxWidth = 0.f;
        for ( const StringView line : lines )
            maxWidth = std::max( maxWidth, FreeType::TextUtil::MeasureLineWidth( *font, line, a_Style ) );

        const f32 lineHeight = FreeType::GetLineHeight( font->GetFace(), a_Style);
        const f32 baseline   = font->GetFace()->size->metrics.ascender / 64.f;

        TextMeasurement result;
        result.Size[0]   = std::clamp( maxWidth, 0.f, a_MaxWidth );
        result.Size[1]   = lineHeight * static_cast<f32>( Size( lines ) );
        result.Baseline  = baseline;
        result.LineCount = static_cast<u32>( Size( lines ) );
        return result;
    }

} // namespace RatUI::SDL2