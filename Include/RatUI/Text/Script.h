#pragma once
#include "../Core/Types.h"
#include "../Core/Containers.h"

namespace RatUI
{
    /**
     * @brief Utility function to create a 4-character script tag as a 32-bit integer.
     * This is used to define OpenType script tags in a human-readable way while ensuring they are stored efficiently as integers for comparison and hashing.
     * The characters are packed in big-endian order, matching the standard OpenType tag format.
     * @param a_C0 The first character of the script tag (most significant byte).
     * @param a_C1 The second character of the script tag.
     * @param a_C2 The third character of the script tag.
     * @param a_C3 The fourth character of the script tag (least significant byte).
     * @return A 32-bit integer representing the combined script tag.
     */
    inline constexpr u32 ScriptTag( char a_C0, char a_C1, char a_C2, char a_C3 )
    {
        return ( static_cast<u32>( a_C0 ) << 24 ) |
               ( static_cast<u32>( a_C1 ) << 16 ) |
               ( static_cast<u32>( a_C2 ) << 8  ) |
               ( static_cast<u32>( a_C3 )       );
    }

    /**
     * @brief Utility function to convert a 32-bit script tag back into its constituent characters.
     * This is useful for debugging and logging purposes, allowing us to display the script tag in a human-readable format.
     * The characters are extracted in big-endian order, matching the standard OpenType tag format.
     * @param a_Tag The 32-bit integer representing the script tag.
     * @return A FixedArray of 4 characters corresponding to the original script tag.
     */
    inline constexpr FixedArray<char, 4> ScriptTagToChars( u32 a_Tag )
    {
        return {
            static_cast<char>( ( a_Tag >> 24 ) & 0xFF ),
            static_cast<char>( ( a_Tag >> 16 ) & 0xFF ),
            static_cast<char>( ( a_Tag >> 8  ) & 0xFF ),
            static_cast<char>(   a_Tag         & 0xFF )
        };
    }

    /**
     * @brief Enumeration of OpenType script tags, represented as 4-character codes packed into a 32-bit integer.
     * These tags match the standard OpenType script tags defined by Microsoft and Adobe, and are used to identify the writing system of a given text run for shaping and layout purposes.
     * The "Invalid" tag serves as a sentinel value for uninitialized or unknown scripts,
     * while "Common", "Inherited", and "Unknown" are special tags defined by the Unicode standard for characters that are not specific to any single script.
     * @note When updating this enum, ensure that the _Count value is also updated to reflect the total number of defined script tags for validation and iteration purposes.
     */
    enum class EScript : u32
    {
        Invalid                  = ScriptTag(  0 ,  0 ,  0 ,  0  ), ///< Sentinel value representing an invalid or uninitialized script tag.

        Common                   = ScriptTag( 'Z', 'y', 'y', 'y' ), ///< The "Common" script tag, used for characters that are not specific to any single script (e.g. punctuation, digits).
        Inherited                = ScriptTag( 'Z', 'i', 'n', 'h' ), ///< The "Inherited" script tag, used for characters that inherit their script from the preceding character (e.g. diacritics).
        Unknown                  = ScriptTag( 'Z', 'z', 'z', 'z' ), ///< The "Unknown" script tag, used for characters whose script is not known or not assigned.
        Arabic                   = ScriptTag( 'A', 'r', 'a', 'b' ),
        Armenian                 = ScriptTag( 'A', 'r', 'm', 'n' ),
        Bengali                  = ScriptTag( 'B', 'e', 'n', 'g' ),
        Cyrillic                 = ScriptTag( 'C', 'y', 'r', 'l' ),
        Devanagari               = ScriptTag( 'D', 'e', 'v', 'a' ),
        Georgian                 = ScriptTag( 'G', 'e', 'o', 'r' ),
        Greek                    = ScriptTag( 'G', 'r', 'e', 'k' ),
        Gujarati                 = ScriptTag( 'G', 'u', 'j', 'r' ),
        Gurmukhi                 = ScriptTag( 'G', 'u', 'r', 'u' ),
        Hangul                   = ScriptTag( 'H', 'a', 'n', 'g' ),
        Han                      = ScriptTag( 'H', 'a', 'n', 'i' ),
        Hebrew                   = ScriptTag( 'H', 'e', 'b', 'r' ),
        Hiragana                 = ScriptTag( 'H', 'i', 'r', 'a' ),
        Kannada                  = ScriptTag( 'K', 'n', 'd', 'a' ),
        Katakana                 = ScriptTag( 'K', 'a', 'n', 'a' ),
        Lao                      = ScriptTag( 'L', 'a', 'o', 'o' ),
        Latin                    = ScriptTag( 'L', 'a', 't', 'n' ),
        Malayalam                = ScriptTag( 'M', 'l', 'y', 'm' ),
        Oriya                    = ScriptTag( 'O', 'r', 'y', 'a' ),
        Tamil                    = ScriptTag( 'T', 'a', 'm', 'l' ),
        Telugu                   = ScriptTag( 'T', 'e', 'l', 'u' ),
        Thai                     = ScriptTag( 'T', 'h', 'a', 'i' ),
        Tibetan                  = ScriptTag( 'T', 'i', 'b', 't' ),
        Bopomofo                 = ScriptTag( 'B', 'o', 'p', 'o' ),
        Braille                  = ScriptTag( 'B', 'r', 'a', 'i' ),
        CanadianSyllabics        = ScriptTag( 'C', 'a', 'n', 's' ),
        Cherokee                 = ScriptTag( 'C', 'h', 'e', 'r' ),
        Ethiopic                 = ScriptTag( 'E', 't', 'h', 'i' ),
        Khmer                    = ScriptTag( 'K', 'h', 'm', 'r' ),
        Mongolian                = ScriptTag( 'M', 'o', 'n', 'g' ),
        Myanmar                  = ScriptTag( 'M', 'y', 'm', 'r' ),
        Ogham                    = ScriptTag( 'O', 'g', 'a', 'm' ),
        Runic                    = ScriptTag( 'R', 'u', 'n', 'r' ),
        Sinhala                  = ScriptTag( 'S', 'i', 'n', 'h' ),
        Syriac                   = ScriptTag( 'S', 'y', 'r', 'c' ),
        Thaana                   = ScriptTag( 'T', 'h', 'a', 'a' ),
        Yi                       = ScriptTag( 'Y', 'i', 'i', 'i' ),
        Deseret                  = ScriptTag( 'D', 's', 'r', 't' ),
        Gothic                   = ScriptTag( 'G', 'o', 't', 'h' ),
        OldItalic                = ScriptTag( 'I', 't', 'a', 'l' ),
        Buhid                    = ScriptTag( 'B', 'u', 'h', 'd' ),
        Hanunoo                  = ScriptTag( 'H', 'a', 'n', 'o' ),
        Tagalog                  = ScriptTag( 'T', 'g', 'l', 'g' ),
        Tagbanwa                 = ScriptTag( 'T', 'a', 'g', 'b' ),
        Cypriot                  = ScriptTag( 'C', 'p', 'r', 't' ),
        Limbu                    = ScriptTag( 'L', 'i', 'm', 'b' ),
        LinearB                  = ScriptTag( 'L', 'i', 'n', 'b' ),
        Osmanya                  = ScriptTag( 'O', 's', 'm', 'a' ),
        Shavian                  = ScriptTag( 'S', 'h', 'a', 'w' ),
        TaiLe                    = ScriptTag( 'T', 'a', 'l', 'e' ),
        Ugaritic                 = ScriptTag( 'U', 'g', 'a', 'r' ),
        Buginese                 = ScriptTag( 'B', 'u', 'g', 'i' ),
        Coptic                   = ScriptTag( 'C', 'o', 'p', 't' ),
        Glagolitic               = ScriptTag( 'G', 'l', 'a', 'g' ),
        Kharoshthi               = ScriptTag( 'K', 'h', 'a', 'r' ),
        NewTaiLue                = ScriptTag( 'T', 'a', 'l', 'u' ),
        OldPersian               = ScriptTag( 'X', 'p', 'e', 'o' ),
        SylotiNagri              = ScriptTag( 'S', 'y', 'l', 'o' ),
        Tifinagh                 = ScriptTag( 'T', 'f', 'n', 'g' ),
        Balinese                 = ScriptTag( 'B', 'a', 'l', 'i' ),
        Cuneiform                = ScriptTag( 'X', 's', 'u', 'x' ),
        Nko                      = ScriptTag( 'N', 'k', 'o', 'o' ),
        PhagsPa                  = ScriptTag( 'P', 'h', 'a', 'g' ),
        Phoenician               = ScriptTag( 'P', 'h', 'n', 'x' ),
        Carian                   = ScriptTag( 'C', 'a', 'r', 'i' ),
        Cham                     = ScriptTag( 'C', 'h', 'a', 'm' ),
        KayahLi                  = ScriptTag( 'K', 'a', 'l', 'i' ),
        Lepcha                   = ScriptTag( 'L', 'e', 'p', 'c' ),
        Lycian                   = ScriptTag( 'L', 'y', 'c', 'i' ),
        Lydian                   = ScriptTag( 'L', 'y', 'd', 'i' ),
        OlChiki                  = ScriptTag( 'O', 'l', 'c', 'k' ),
        Rejang                   = ScriptTag( 'R', 'j', 'n', 'g' ),
        Saurashtra               = ScriptTag( 'S', 'a', 'u', 'r' ),
        Sundanese                = ScriptTag( 'S', 'u', 'n', 'd' ),
        Vai                      = ScriptTag( 'V', 'a', 'i', 'i' ),
        Avestan                  = ScriptTag( 'A', 'v', 's', 't' ),
        Bamum                    = ScriptTag( 'B', 'a', 'm', 'u' ),
        EgyptianHieroglyphs      = ScriptTag( 'E', 'g', 'y', 'p' ),
        ImperialAramaic          = ScriptTag( 'A', 'r', 'm', 'i' ),
        InscriptionalPahlavi     = ScriptTag( 'P', 'h', 'l', 'i' ),
        InscriptionalParthian    = ScriptTag( 'P', 'r', 't', 'i' ),
        Javanese                 = ScriptTag( 'J', 'a', 'v', 'a' ),
        Kaithi                   = ScriptTag( 'K', 't', 'h', 'i' ),
        Lisu                     = ScriptTag( 'L', 'i', 's', 'u' ),
        MeeteiMayek              = ScriptTag( 'M', 't', 'e', 'i' ),
        OldSouthArabian          = ScriptTag( 'S', 'a', 'r', 'b' ),
        OldTurkic                = ScriptTag( 'O', 'r', 'k', 'h' ),
        Samaritan                = ScriptTag( 'S', 'a', 'm', 'r' ),
        TaiTham                  = ScriptTag( 'L', 'a', 'n', 'a' ),
        TaiViet                  = ScriptTag( 'T', 'a', 'v', 't' ),
        Batak                    = ScriptTag( 'B', 'a', 't', 'k' ),
        Brahmi                   = ScriptTag( 'B', 'r', 'a', 'h' ),
        Mandaic                  = ScriptTag( 'M', 'a', 'n', 'd' ),
        Chakma                   = ScriptTag( 'C', 'a', 'k', 'm' ),
        MeroiticCursive          = ScriptTag( 'M', 'e', 'r', 'c' ),
        MeroiticHieroglyphs      = ScriptTag( 'M', 'e', 'r', 'o' ),
        Miao                     = ScriptTag( 'P', 'l', 'r', 'd' ),
        Sharada                  = ScriptTag( 'S', 'h', 'r', 'd' ),
        SoraSompeng              = ScriptTag( 'S', 'o', 'r', 'a' ),
        Takri                    = ScriptTag( 'T', 'a', 'k', 'r' ),
        BassaVah                 = ScriptTag( 'B', 'a', 's', 's' ),
        CaucasianAlbanian        = ScriptTag( 'A', 'g', 'h', 'b' ),
        Duployan                 = ScriptTag( 'D', 'u', 'p', 'l' ),
        Elbasan                  = ScriptTag( 'E', 'l', 'b', 'a' ),
        Grantha                  = ScriptTag( 'G', 'r', 'a', 'n' ),
        Khojki                   = ScriptTag( 'K', 'h', 'o', 'j' ),
        Khudawadi                = ScriptTag( 'S', 'i', 'n', 'd' ),
        LinearA                  = ScriptTag( 'L', 'i', 'n', 'a' ),
        Mahajani                 = ScriptTag( 'M', 'a', 'h', 'j' ),
        Manichaean               = ScriptTag( 'M', 'a', 'n', 'i' ),
        MendeKikakui             = ScriptTag( 'M', 'e', 'n', 'd' ),
        Modi                     = ScriptTag( 'M', 'o', 'd', 'i' ),
        Mro                      = ScriptTag( 'M', 'r', 'o', 'o' ),
        Nabataean                = ScriptTag( 'N', 'b', 'a', 't' ),
        OldNorthArabian          = ScriptTag( 'N', 'a', 'r', 'b' ),
        OldPermic                = ScriptTag( 'P', 'e', 'r', 'm' ),
        PahahwHmong              = ScriptTag( 'H', 'm', 'n', 'g' ),
        Palmyrene                = ScriptTag( 'P', 'a', 'l', 'm' ),
        PauCinHau                = ScriptTag( 'P', 'a', 'u', 'c' ),
        PsalterPahlavi           = ScriptTag( 'P', 'h', 'l', 'p' ),
        Siddham                  = ScriptTag( 'S', 'i', 'd', 'd' ),
        Tirhuta                  = ScriptTag( 'T', 'i', 'r', 'h' ),
        WarangCiti               = ScriptTag( 'W', 'a', 'r', 'a' ),
        Ahom                     = ScriptTag( 'A', 'h', 'o', 'm' ),
        AnatolianHieroglyphs     = ScriptTag( 'H', 'l', 'u', 'w' ),
        Hatran                   = ScriptTag( 'H', 'a', 't', 'r' ),
        Multani                  = ScriptTag( 'M', 'u', 'l', 't' ),
        OldHungarian             = ScriptTag( 'H', 'u', 'n', 'g' ),
        Signwriting              = ScriptTag( 'S', 'g', 'n', 'w' ),
        Adlam                    = ScriptTag( 'A', 'd', 'l', 'm' ),
        Bhaiksuki                = ScriptTag( 'B', 'h', 'k', 's' ),
        Marchen                  = ScriptTag( 'M', 'a', 'r', 'c' ),
        Osage                    = ScriptTag( 'O', 's', 'g', 'e' ),
        Tangut                   = ScriptTag( 'T', 'a', 'n', 'g' ),
        Newa                     = ScriptTag( 'N', 'e', 'w', 'a' ),
        MasaramGondi             = ScriptTag( 'G', 'o', 'n', 'm' ),
        Nushu                    = ScriptTag( 'N', 's', 'h', 'u' ),
        Soyombo                  = ScriptTag( 'S', 'o', 'y', 'o' ),
        ZanabazarSquare          = ScriptTag( 'Z', 'a', 'n', 'b' ),
        Dogra                    = ScriptTag( 'D', 'o', 'g', 'r' ),
        GunjalaGondi             = ScriptTag( 'G', 'o', 'n', 'g' ),
        HanifiRohingya           = ScriptTag( 'R', 'o', 'h', 'g' ),
        Makasar                  = ScriptTag( 'M', 'a', 'k', 'a' ),
        Medefaidrin              = ScriptTag( 'M', 'e', 'd', 'f' ),
        OldSogdian               = ScriptTag( 'S', 'o', 'g', 'o' ),
        Sogdian                  = ScriptTag( 'S', 'o', 'g', 'd' ),
        Elymaic                  = ScriptTag( 'E', 'l', 'y', 'm' ),
        Nandinagari              = ScriptTag( 'N', 'a', 'n', 'd' ),
        NyiakengPuachueHmong     = ScriptTag( 'H', 'm', 'n', 'p' ),
        Wancho                   = ScriptTag( 'W', 'c', 'h', 'o' ),
        Chorasmian               = ScriptTag( 'C', 'h', 'r', 's' ),
        DivesAkuru               = ScriptTag( 'D', 'i', 'a', 'k' ),
        KhitanSmallScript        = ScriptTag( 'K', 'i', 't', 's' ),
        Yezidi                   = ScriptTag( 'Y', 'e', 'z', 'i' ),
        CyproMinoan              = ScriptTag( 'C', 'p', 'm', 'n' ),
        OldUyghur                = ScriptTag( 'O', 'u', 'g', 'r' ),
        Tangsa                   = ScriptTag( 'T', 'n', 's', 'a' ),
        Toto                     = ScriptTag( 'T', 'o', 't', 'o' ),
        Vithkuqi                 = ScriptTag( 'V', 'i', 't', 'h' ),
        Math                     = ScriptTag( 'Z', 'm', 't', 'h' ),
        Kawi                     = ScriptTag( 'K', 'a', 'w', 'i' ),
        NagMundari               = ScriptTag( 'N', 'a', 'g', 'm' ),

        _Count                   = 165 ///< The total number of defined script tags, excluding Invalid, used for validation and iteration.
    };
}