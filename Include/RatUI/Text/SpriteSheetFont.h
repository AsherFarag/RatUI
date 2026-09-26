#pragma once
#include "../Core.h"
#include "IFontFace.h"

namespace RatUI
{
    struct SpriteSheetKerningPair
    {
        codepoint Left { 0 };
        codepoint Right{ 0 };
        i32       Amount{ 0 }; ///< Native pixels.
    };

    /**
     * @brief A grid bitmap font: one glyph per fixed-size cell, cells listed row-major in Characters.
     * TODO: Loading straight from an image file would be nice, but core doesn't have an image loader.
     */
    struct SpriteSheetFontDesc
    {
        const Color* Pixels{ nullptr };     ///< RGBA8, copied.
        Vec2u        ImageSize{ 0, 0 };
        Vec2u        CellSize { 0, 0 };
        Vec2u        CellOrigin { 0, 0 };   ///< Offset of the first cell.
        Vec2u        CellSpacing{ 0, 0 };   ///< Gap between cells.
        StringView   Characters;            ///< UTF-8, one codepoint per cell.

        u32  Baseline{ 0 };                 ///< Rows above the baseline. 0 = CellSize.y.
        u32  LineHeight{ 0 };               ///< In native pixels. 0 = CellSize.y + 1.
        bool Monospace{ false };            ///< Otherwise glyphs are trimmed to their inked columns.
        i32  LetterSpacing{ 1 };            ///< Added after each trimmed glyph.
        u32  SpaceAdvance{ 0 };             ///< Used when ' ' is missing or blank. 0 = half a cell.
        u8   AlphaThreshold{ 1 };           ///< Alpha that counts as ink when trimming.
        bool PreserveColor{ false };        ///< Keep the sheet's colours instead of treating it as a white mask.

        Array<SpriteSheetKerningPair> Kerning;
        StringView Name;
    };

    /** @brief Creates a Pixel-mode face whose em is one cell high. Returns nullptr if the desc is invalid. */
    Unique<IFontFace> MakeSpriteSheetFace( const SpriteSheetFontDesc& a_Desc );

} // namespace RatUI
