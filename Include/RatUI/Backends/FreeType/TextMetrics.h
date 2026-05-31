#pragma once
#include "FontCache.h"
#include "TextUtil.h"
#include "../../Text/GlyphAtlas.h"

#include <cmath>

namespace RatUI::FreeType
{
    class TextMetrics : public ITextMetrics
    {
    public:
        TextMetrics( FontCache& a_FontCache ) : m_FontCache( a_FontCache ) {}

        /** @brief Gets the underlying font cache set by the user. */
        FontCache& GetFontCache() { return m_FontCache; }
        const FontCache& GetFontCache() const { return m_FontCache; }

        Optional<PreparedText> Prepare( StringView a_Text, const TextLayoutStyle& a_Style ) override;
        Optional<ShapedText> Shape( const PreparedText& a_Prepared, const TextLayoutStyle& a_Style, Vec2<Unit> a_MaxSize = { Limits<Unit>::max(), Limits<Unit>::max() } ) override;

        bool RasterizeGlyph(
            FontHandle a_Font, GlyphID a_GlyphIndex, u32 a_SdfPixelSize,
            const Color*& o_Pixels, u32& o_Width, u32& o_Height,
            Vec2<FontUnit>& o_Bearing, FontUnit& o_XAdvance
        ) override;

    protected:
        FontCache& m_FontCache;
        Array<Color> m_RasterBuffer; ///< Persistent buffer for the last rasterized glyph's RGBA8 pixel data.
    };

} // namespace RatUI::FreeType
