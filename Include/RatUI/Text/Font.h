#pragma once
#include "../Core.h"

namespace RatUI
{
    /** @brief Handle to a single font face owned by a FontLibrary. */
    struct FontFaceHandle
    {
        u32 ID{ 0 }; ///< 1-based index into the FontLibrary. 0 = invalid.

        constexpr bool IsValid() const { return ID != 0; }
        constexpr bool operator==( const FontFaceHandle& ) const = default;
    };

    /** @brief Handle to a font family owned by a FontLibrary. Invalid = the library's default family. */
    struct FontFamilyHandle
    {
        u32 ID{ 0 };

        constexpr bool IsValid() const { return ID != 0; }
        constexpr bool operator==( const FontFamilyHandle& ) const = default;
    };

    /** @brief Font weight on the CSS / OpenType scale (1-1000). */
    enum class EFontWeight : u16
    {
        Thin       = 100,
        ExtraLight = 200,
        Light      = 300,
        Regular    = 400,
        Medium     = 500,
        SemiBold   = 600,
        Bold       = 700,
        ExtraBold  = 800,
        Black      = 900,
    };

    enum class EFontStyle : u8
    {
        Normal,
        Italic,
        Oblique,
    };

    /** @brief How a face's glyphs are rasterized and drawn. */
    enum class EGlyphRenderMode : u8
    {
        MTSDF,  ///< Distance field, scales to any size. Best for large / animated text.
        Raster, ///< Hinted bitmaps at the exact pixel size. Sharpest for small UI text.
        Pixel,  ///< Pixel-art fonts: native size, whole-number scales, nearest sampling.

        Count
    };

    /** @brief Whether bold / italic may be faked when no real face matches (CSS font-synthesis). */
    struct FontSynthesis
    {
        bool Bold   : 1 = true;
        bool Italic : 1 = true;

        constexpr bool operator==( const FontSynthesis& ) const = default;

        static constexpr FontSynthesis None() { return { false, false }; }
        static constexpr FontSynthesis All()  { return { true, true }; }
    };

    /** @brief Describes a face within its family. TODO: Variable font axes. */
    struct FontFaceDesc
    {
        EFontWeight Weight{ EFontWeight::Regular };
        EFontStyle  Style { EFontStyle::Normal };

        constexpr bool operator==( const FontFaceDesc& ) const = default;
    };

    struct FontQuery
    {
        FontFamilyHandle Family{};
        EFontWeight      Weight{ EFontWeight::Regular };
        EFontStyle       Style { EFontStyle::Normal };
        FontSynthesis    Synthesis{};

        constexpr bool operator==( const FontQuery& ) const = default;
    };

    /** @brief A matched face plus any synthesis to apply. */
    struct ResolvedFace
    {
        FontFaceHandle Face{};
        bool           SynthBold  { false };
        bool           SynthItalic{ false };

        constexpr bool IsValid() const { return Face.IsValid(); }
        constexpr bool operator==( const ResolvedFace& ) const = default;
    };

    /** @brief Face-wide metrics in em, Y-up from the baseline (Descender is negative). */
    struct FontFaceMetrics
    {
        FontUnit Ascender          { 0.8_fu  };
        FontUnit Descender         { -0.2_fu };
        FontUnit LineGap           { 0_fu    };
        FontUnit UnderlinePosition { -0.1_fu }; ///< Centre of the underline.
        FontUnit UnderlineThickness{ 0.05_fu };
        FontUnit StrikeoutPosition { 0.25_fu }; ///< Centre of the strikethrough.
        FontUnit StrikeoutThickness{ 0.05_fu };
        FontUnit CapHeight         { 0.7_fu  };
        FontUnit XHeight           { 0.5_fu  };

        f32              NativePixelSize{ 0.f }; ///< Pixel mode: design pixels per em (can be fractional). 0 for scalable faces.
        EGlyphRenderMode Mode{ EGlyphRenderMode::MTSDF };

        constexpr FontUnit LineHeight() const { return Ascender - Descender + LineGap; }
    };

} // namespace RatUI

// TODO: Should implement a RatUI::Hash<T> type instead of using std::hash
namespace std
{
    template<>
    struct hash<RatUI::FontFaceHandle>
    {
        size_t operator()( const RatUI::FontFaceHandle& a_Handle ) const noexcept
        {
            return std::hash<RatUI::u32>{}( a_Handle.ID );
        }
    };

    template<>
    struct hash<RatUI::FontFamilyHandle>
    {
        size_t operator()( const RatUI::FontFamilyHandle& a_Handle ) const noexcept
        {
            return std::hash<RatUI::u32>{}( a_Handle.ID );
        }
    };
}
