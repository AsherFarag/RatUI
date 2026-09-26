#pragma once
#include "../Core.h"

// Synthetic bold / italic for pixel-art glyphs, done per pixel so strokes stay on the grid.
namespace RatUI::PixelGlyphOps
{
    inline constexpr u32 c_ItalicRowsPerStep = 4; ///< Rows per 1px of italic shift (~14 degrees).

    /** @brief Smears the glyph 1px right (width + 1), without closing 1px gaps between strokes. */
    inline void Embolden( Array<Color>& io_Pixels, u32& io_Width, u32 a_Height, u8 a_Threshold = 128 )
    {
        if ( io_Width == 0 || a_Height == 0 )
            return;

        const u32 newWidth = io_Width + 1;
        Array<Color> result( static_cast<size>( newWidth ) * a_Height, Colors::Transparent );

        const auto at = [&]( u32 x, u32 y ) -> Color
        {
            return x < io_Width ? io_Pixels[y * io_Width + x] : Colors::Transparent;
        };

        for ( u32 y = 0; y < a_Height; ++y )
        {
            for ( u32 x = 0; x < newWidth; ++x )
            {
                const Color self = at( x, y );
                const Color left = x > 0 ? at( x - 1, y ) : Colors::Transparent;
                const Color next = at( x + 1, y );

                const bool grow = self[3] < a_Threshold && left[3] >= a_Threshold && next[3] < a_Threshold;
                result[y * newWidth + x] = grow ? left : self;
            }
        }

        io_Pixels = std::move( result );
        io_Width  = newWidth;
    }

    /**
     * @brief
     * @param a_BaselineRow First row below the baseline. Returns how much the width grew.
     */
    inline u32 Italicize( Array<Color>& io_Pixels, u32& io_Width, u32 a_Height, i32 a_BaselineRow )
    {
        if ( io_Width == 0 || a_Height == 0 )
            return 0;

        const auto shiftForRow = [&]( i32 a_Row ) -> u32
        {
            const i32 rowsAboveBaseline = a_BaselineRow - 1 - a_Row;
            return rowsAboveBaseline > 0 ? static_cast<u32>( rowsAboveBaseline ) / c_ItalicRowsPerStep : 0u;
        };

        const u32 maxShift = shiftForRow( 0 );
        if ( maxShift == 0 )
            return 0;

        const u32 newWidth = io_Width + maxShift;
        Array<Color> result( static_cast<size>( newWidth ) * a_Height, Colors::Transparent );

        for ( u32 y = 0; y < a_Height; ++y )
        {
            const u32 shift = shiftForRow( static_cast<i32>( y ) );
            for ( u32 x = 0; x < io_Width; ++x )
                result[y * newWidth + x + shift] = io_Pixels[y * io_Width + x];
        }

        io_Pixels = std::move( result );
        io_Width  = newWidth;
        return maxShift;
    }

} // namespace RatUI::PixelGlyphOps
