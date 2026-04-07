#pragma once
#include "../Core.h"

/**
 * @file Unicode.h
 * @brief Unicode-related utilities based on the Unicode Standard (15.1) and
 *        the CSS Text Module Level 3.
 *
 * Unicode version coverage note: CJK block detection covers through
 * Extension H (Unicode 15.1, U+31350–U+323AF). If targeting a later Unicode
 * version, audit IsCJK() for newly assigned Extension blocks.
 *
 * UTF-8 error handling: ill-formed byte sequences emit one U+FFFD replacement
 * character per ill-formed sequence (not per byte), per Unicode conformance
 * requirement C.10 / Table 3-7.
 */

namespace RatUI::Unicode
{
    /**
     * @brief Returns true if @p a_CP is a CJK (or CJK-adjacent) codepoint that
     *        allows a line-break opportunity between consecutive characters.
     *
     * Covers Unified Ideographs, Extensions A–H (Unicode 15.1),
     * Compatibility Ideographs, CJK Symbols & Punctuation, Hiragana,
     * Katakana, Hangul Syllables, and the Halfwidth/Fullwidth Forms block.
     *
     * @note Last audited against Unicode 15.1. Check for new CJK Extension
     *       blocks when upgrading the targeted Unicode version.
     */
    constexpr inline bool IsCJK( c32 a_CP )
    {
        return ( a_CP >= 0x4E00  && a_CP <= 0x9FFF  ) || // CJK Unified Ideographs
               ( a_CP >= 0x3400  && a_CP <= 0x4DBF  ) || // CJK Extension A
               ( a_CP >= 0x20000 && a_CP <= 0x2A6DF ) || // CJK Extension B
               ( a_CP >= 0x2A700 && a_CP <= 0x2B73F ) || // CJK Extension C
               ( a_CP >= 0x2B740 && a_CP <= 0x2B81F ) || // CJK Extension D
               ( a_CP >= 0x2B820 && a_CP <= 0x2CEAF ) || // CJK Extension E
               ( a_CP >= 0x2CEB0 && a_CP <= 0x2EBEF ) || // CJK Extension F
               ( a_CP >= 0x30000 && a_CP <= 0x3134F ) || // CJK Extension G
               ( a_CP >= 0x31350 && a_CP <= 0x323AF ) || // CJK Extension H (Unicode 15.1)
               ( a_CP >= 0xF900  && a_CP <= 0xFAFF  ) || // CJK Compatibility Ideographs
               ( a_CP >= 0x2F800 && a_CP <= 0x2FA1F ) || // CJK Compat. Ideographs Supplement
               ( a_CP >= 0x3000  && a_CP <= 0x303F  ) || // CJK Symbols and Punctuation
               ( a_CP >= 0x3040  && a_CP <= 0x309F  ) || // Hiragana
               ( a_CP >= 0x30A0  && a_CP <= 0x30FF  ) || // Katakana
               ( a_CP >= 0xAC00  && a_CP <= 0xD7AF  ) || // Hangul Syllables
               ( a_CP >= 0xFF00  && a_CP <= 0xFFEF  );   // Halfwidth and Fullwidth Forms
    }

    // -------------------------------------------------------------------------
    // Line-break prohibition classification (JIS X 4051 / CSS Text Level 3)
    //
    // CSS spec terminology used here:
    //   IsLineStartProhibited  — character MUST NOT appear at the start of a line
    //                            (called "line-start prohibit" in CSS / 行頭禁則 in JIS)
    //   IsLineEndProhibited    — character MUST NOT appear at the end of a line
    //                            (called "line-end prohibit"   in CSS / 行末禁則 in JIS)
    //
    // Note on script coverage: IsLineStartProhibited and IsLineEndProhibited
    // currently cover CJK/Japanese punctuation only.
    // IsLineStartStickyPunctuation covers Latin-script and several other
    // scripts (Arabic, Devanagari, Myanmar). Full bidirectional symmetry
    // across all scripts is not yet implemented; see IsLineStartStickyPunctuation.
    // -------------------------------------------------------------------------

    /**
     * @brief Returns true if @p a_CP must not appear at the START of a line
     *        (行頭禁則 / CSS "line-start prohibit").
     *
     * These are typically closing brackets, terminal punctuation, and
     * iteration marks that must always follow the character they relate to.
     */
    constexpr inline bool IsLineStartProhibited( c32 a_CP )
    {
        switch ( a_CP )
        {
            case 0xFF0C: // ，  full-width comma
            case 0xFF0E: // ．  full-width full stop
            case 0xFF01: // ！  full-width exclamation mark
            case 0xFF1A: // ：  full-width colon
            case 0xFF1B: // ；  full-width semicolon
            case 0xFF1F: // ？  full-width question mark
            case 0x3001: // 、  ideographic comma
            case 0x3002: // 。  ideographic full stop
            case 0x30FB: // ・  katakana middle dot
            case 0xFF09: // ）  full-width right parenthesis
            case 0x3015: // 〕  right tortoise shell bracket
            case 0x3009: // 〉  right angle bracket
            case 0x300B: // 》  right double angle bracket
            case 0x300D: // 」  right corner bracket
            case 0x300F: // 』  right white corner bracket
            case 0x3011: // 】  right black lenticular bracket
            case 0x3017: // 〗  right white lenticular bracket
            case 0x3019: // 〙  right white tortoise shell bracket
            case 0x301B: // 〛  right white square bracket
            case 0x30FC: // ー  katakana-hiragana prolonged sound mark
            case 0x3005: // 々  ideographic iteration mark
            case 0x303B: // 〻  vertical ideographic iteration mark
            case 0x309D: // ゝ  hiragana iteration mark
            case 0x309E: // ゞ  hiragana voiced iteration mark
            case 0x30FD: // ヽ  katakana iteration mark
            case 0x30FE: // ヾ  katakana voiced iteration mark
                return true;
            default:
                return false;
        }
    }

    /**
     * @brief Returns true if @p a_CP must not appear at the END of a line
     *        (行末禁則 / CSS "line-end prohibit").
     *
     * These are typically opening brackets and quotation marks that must
     * always precede the content they open.
     */
    constexpr inline bool IsLineEndProhibited( c32 a_CP )
    {
        switch ( a_CP )
        {
            case U'"':   // ASCII double-quote (opening)
            case U'(':   // ASCII left parenthesis
            case U'[':   // ASCII left square bracket
            case U'{':   // ASCII left curly bracket
            case 0x201C: // "  left double quotation mark
            case 0x2018: // '  left single quotation mark
            case 0x00AB: // «  left-pointing double angle quotation mark
            case 0x2039: // ‹  single left-pointing angle quotation mark
            case 0xFF08: // （  full-width left parenthesis
            case 0x3014: // 〔  left tortoise shell bracket
            case 0x3008: // 〈  left angle bracket
            case 0x300A: // 《  left double angle bracket
            case 0x300C: // 「  left corner bracket
            case 0x300E: // 『  left white corner bracket
            case 0x3010: // 【  left black lenticular bracket
            case 0x3016: // 〖  left white lenticular bracket
            case 0x3018: // 〘  left white tortoise shell bracket
            case 0x301A: // 〚  left white square bracket
                return true;
            default:
                return false;
        }
    }

    /**
     * @brief Returns true if @p a_CP is "line-start sticky" punctuation for
     *        Latin-script and several other scripts.
     *
     * Line-start-sticky punctuation must not be separated from the word or
     * character it follows; it should not appear at the start of a new line.
     *
     * @note Script coverage: Latin, Arabic, Devanagari, Myanmar.
     *       Opening equivalents (line-end-sticky) are not yet implemented for
     *       these scripts; see IsLineEndProhibited for CJK opening brackets.
     */
    constexpr inline bool IsLineStartStickyPunctuation( c32 a_CP )
    {
        switch ( a_CP )
        {
            case U'.': case U',': case U'!': case U'?':
            case U':': case U';': case U')': case U']':
            case U'}': case U'%':
            case 0x060C: // Arabic comma
            case 0x061B: // Arabic semicolon
            case 0x061F: // Arabic question mark
            case 0x0964: // Devanagari danda
            case 0x0965: // Devanagari double danda
            case 0x104A: // Myanmar sign little section
            case 0x104B: // Myanmar sign section
            case 0x104C: // Myanmar symbol locative
            case 0x104D: // Myanmar symbol completed
            case 0x104F: // Myanmar symbol genitive
            case 0x201D: // "  right double quotation mark
            case 0x2019: // '  right single quotation mark
            case 0x00BB: // »  right-pointing double angle quotation mark
            case 0x203A: // ›  single right-pointing angle quotation mark
            case 0x2026: // …  horizontal ellipsis
                return true;
            default:
                return false;
        }
    }

    /**
     * @brief Normalises text using CSS 'white-space: normal' rules.
     *
     * All whitespace runs (U+0020 SPACE, U+0009 TAB, U+000A LINE FEED,
     * U+000D CARRIAGE RETURN, U+000C FORM FEED, U+0085 NEXT LINE,
     * U+2028 LINE SEPARATOR, U+2029 PARAGRAPH SEPARATOR) are collapsed to a
     * single ASCII space.  Leading and trailing whitespace is removed.
     *
     * U+0085, U+2028, and U+2029 are decoded from their UTF-8 representations
     * inline so that this function operates on raw UTF-8 without a full decode
     * pass.
     *
     * @param a_Text  Raw input text (UTF-8).
     * @return Normalised string.
     */
    inline String NormalizeWhitespace( StringView a_Text )
    {
        String result;
        // Reserve a heuristic half-capacity since whitespace runs collapse.
        // ShrinkToFit is called at the end to release excess.
        Reserve( result, Size( a_Text ) / 2 + 1 );

        // Treat the beginning as "after space" to strip leading whitespace.
        bool lastWasSpace = true;

        for ( size i = 0; i < Size( a_Text ); )
        {
            const u8 c = static_cast<u8>( RawAt( a_Text, i ) );

            // Detect multi-byte Unicode whitespace before the ASCII fast-path.
            // U+0085 NEXT LINE            -> 0xC2 0x85      (2 bytes)
            // U+2028 LINE SEPARATOR       -> 0xE2 0x80 0xA8 (3 bytes)
            // U+2029 PARAGRAPH SEPARATOR  -> 0xE2 0x80 0xA9 (3 bytes)
            if ( c == 0xC2 && i + 1 < Size( a_Text ) &&
                 static_cast<u8>( RawAt( a_Text, i + 1 ) ) == 0x85 )
            {
                // U+0085: treat as whitespace, consume 2 bytes.
                if ( !lastWasSpace )
                {
                    PushBack( result, ' ' );
                    lastWasSpace = true;
                }
                i += 2;
                continue;
            }

            if ( c == 0xE2 && i + 2 < Size( a_Text ) &&
                 static_cast<u8>( RawAt( a_Text, i + 1 ) ) == 0x80 )
            {
                const u8 b2 = static_cast<u8>( RawAt( a_Text, i + 2 ) );
                if ( b2 == 0xA8 || b2 == 0xA9 ) // U+2028 or U+2029
                {
                    if ( !lastWasSpace )
                    {
                        PushBack( result, ' ' );
                        lastWasSpace = true;
                    }
                    i += 3;
                    continue;
                }
            }

            // ASCII whitespace fast-path.
            const bool ws = ( c == 0x20 || c == 0x09 || c == 0x0A ||
                              c == 0x0D || c == 0x0C );
            if ( ws )
            {
                if ( !lastWasSpace )
                {
                    PushBack( result, ' ' );
                    lastWasSpace = true;
                }
            }
            else
            {
                PushBack( result, static_cast<char>( c ) );
                lastWasSpace = false;
            }
            ++i;
        }

        // Strip any trailing space that was written for a trailing whitespace run.
        if ( !Empty( result ) && RawAt( result, Size( result ) - 1 ) == ' ' )
            Resize( result, Size( result ) - 1 );

        // TODO ShrinkToFit( result );
        return result;
    }

    /**
     * @brief Normalises text using CSS 'white-space: pre-wrap' rules.
     *
     * Ordinary spaces are preserved.  Line-ending sequences are normalised:
     *   '\r\n'             -> '\n'
     *   lone '\r'          -> '\n'
     *   '\f'               -> '\n'
     *   U+0085 NEXT LINE   -> '\n'
     *   U+2028 LINE SEP    -> '\n'
     *   U+2029 PARA SEP    -> '\n'
     *
     * @param a_Text  Raw input text (UTF-8).
     * @return Normalised string.
     */
    inline String NormalizeWhitespacePreWrap( StringView a_Text )
    {
        String result;
        Reserve( result, Size( a_Text ) );

        for ( size i = 0; i < Size( a_Text ); )
        {
            const u8 c = static_cast<u8>( RawAt( a_Text, i ) );

            if ( c == '\r' )
            {
                // Collapse \r\n -> \n; lone \r -> \n.
                if ( i + 1 < Size( a_Text ) && RawAt( a_Text, i + 1 ) == '\n' )
                    ++i;
                PushBack( result, '\n' );
                ++i;
                continue;
            }

            if ( c == '\f' )
            {
                PushBack( result, '\n' );
                ++i;
                continue;
            }

            // U+0085 NEXT LINE (0xC2 0x85) -> '\n'
            if ( c == 0xC2 && i + 1 < Size( a_Text ) &&
                 static_cast<u8>( RawAt( a_Text, i + 1 ) ) == 0x85 )
            {
                PushBack( result, '\n' );
                i += 2;
                continue;
            }

            // U+2028 LINE SEPARATOR / U+2029 PARAGRAPH SEPARATOR (0xE2 0x80 0xA8/0xA9) -> '\n'
            if ( c == 0xE2 && i + 2 < Size( a_Text ) &&
                 static_cast<u8>( RawAt( a_Text, i + 1 ) ) == 0x80 )
            {
                const u8 b2 = static_cast<u8>( RawAt( a_Text, i + 2 ) );
                if ( b2 == 0xA8 || b2 == 0xA9 )
                {
                    PushBack( result, '\n' );
                    i += 3;
                    continue;
                }
            }

            PushBack( result, static_cast<char>( c ) );
            ++i;
        }

        return result;
    }

    /**
     * @brief Forward iterator that traverses a UTF-8 string as Unicode codepoints.
     *
     * Design notes:
     *  - This is NOT STL-compatible.
     *  - For range traversal prefer the UTF8Range helper below.
     *  - Ill-formed byte sequences emit exactly one U+FFFD per ill-formed
     *    sequence (per Unicode conformance requirement C.10) and advance past
     *    the maximal subpart of the ill-formed sequence.
     */
    class UTF8Iterator
    {
    public:
        UTF8Iterator() = default;

        /**
         * @param a_String  The string view to iterate.
         * @param a_Start   Byte offset to start from (must be on a codepoint boundary).
         */
        constexpr explicit UTF8Iterator( StringView a_String, size a_Start = 0 )
            : m_Data( a_String ), m_Index( a_Start )
        {
            if ( m_Index < Size( m_Data ) )
                DecodeAt( m_Index, m_Current, m_SequenceLen );
        }

        /** Returns an iterator positioned at the end of @p a_String. */
        static constexpr UTF8Iterator End( StringView a_String )
        {
            return UTF8Iterator( a_String, Size( a_String ) );
        }

        constexpr c32 operator*()          const { return m_Current;                }
        constexpr explicit operator bool()      const { return m_Index < Size( m_Data ); }
        constexpr size     ByteIndex()          const { return m_Index;                  }
        constexpr size     SequenceByteLength() const { return m_SequenceLen;            }

        constexpr UTF8Iterator& operator++()
        {
            if ( m_Index < Size( m_Data ) )
            {
                m_Index += m_SequenceLen; // advance past the current (possibly ill-formed) sequence
                if ( m_Index < Size( m_Data ) )
                    DecodeAt( m_Index, m_Current, m_SequenceLen );
            }
            return *this;
        }

        constexpr bool operator==( const UTF8Iterator& a_Other ) const
        {
            return Data( m_Data ) == Data( a_Other.m_Data ) && m_Index == a_Other.m_Index;
        }

        constexpr bool operator!=( const UTF8Iterator& a_Other ) const
        {
            return !( *this == a_Other );
        }

    private:
        StringView m_Data;
        size       m_Index{ 0 };
        c32   m_Current{ 0 };
        size       m_SequenceLen{ 0 }; // byte length of m_Current's sequence (>= 1)

        static constexpr bool IsCont( u8 b ) { return ( b & 0xC0 ) == 0x80; }

        /**
         * @brief Decode one codepoint at byte offset @p a_At in m_Data.
         *
         * Sets @p a_CP and @p a_Len.  For ill-formed sequences, @p a_CP is set
         * to U+FFFD and @p a_Len is set to the length of the maximal subpart of
         * the ill-formed sequence (always >= 1), so that the caller advances past
         * exactly those bytes and emits exactly one U+FFFD per ill-formed sequence.
         */
        constexpr void DecodeAt( size a_At, c32& a_CP, size& a_Len ) const
        {
            const u8*  s   = reinterpret_cast<const u8*>( Data( m_Data ) ) + a_At;
            const size rem = Size( m_Data ) - a_At;
            const u8   b0  = s[0];

            // 1-byte (ASCII)
            if ( b0 <= 0x7F )
            {
                a_CP  = b0;
                a_Len = 1;
                return;
            }

            // 2-byte sequence
            if ( ( b0 >> 5 ) == 0b110 )
            {
                if ( rem >= 2 && IsCont( s[1] ) )
                {
                    const c32 cp = static_cast<c32>( ( ( b0 & 0x1F ) << 6 ) | ( s[1] & 0x3F ) );
                    if ( cp >= 0x80 ) // reject overlong (cp < 0x80 would be encoded in 1 byte)
                    {
                        a_CP  = cp;
                        a_Len = 2;
                        return;
                    }
                }
                // Ill-formed: consume just the leading byte (maximal subpart = 1 if
                // continuation is missing/wrong; continuation will be re-evaluated).
                a_CP  = 0xFFFD;
                a_Len = 1;
                return;
            }

            // 3-byte sequence
            if ( ( b0 >> 4 ) == 0b1110 )
            {
                if ( rem >= 2 && IsCont( s[1] ) )
                {
                    if ( rem >= 3 && IsCont( s[2] ) )
                    {
                        const c32 cp = static_cast<c32>(
                            ( ( b0 & 0x0F ) << 12 ) | ( ( s[1] & 0x3F ) << 6 ) | ( s[2] & 0x3F ) );
                        // Reject overlong (< 0x800) and surrogates (0xD800–0xDFFF)
                        if ( cp >= 0x800 && !( cp >= 0xD800 && cp <= 0xDFFF ) )
                        {
                            a_CP  = cp;
                            a_Len = 3;
                            return;
                        }
                    }
                    // First continuation valid, second missing/wrong: maximal subpart = 2
                    a_CP  = 0xFFFD;
                    a_Len = 2;
                    return;
                }
                // No valid continuation at all: maximal subpart = 1
                a_CP  = 0xFFFD;
                a_Len = 1;
                return;
            }

            // 4-byte sequence
            if ( ( b0 >> 3 ) == 0b11110 )
            {
                if ( rem >= 2 && IsCont( s[1] ) )
                {
                    if ( rem >= 3 && IsCont( s[2] ) )
                    {
                        if ( rem >= 4 && IsCont( s[3] ) )
                        {
                            const c32 cp = static_cast<c32>(
                                ( ( b0 & 0x07 ) << 18 ) | ( ( s[1] & 0x3F ) << 12 ) |
                                ( ( s[2] & 0x3F ) << 6 )  | ( s[3] & 0x3F ) );
                            // Reject overlong (< 0x10000) and out-of-Unicode (> 0x10FFFF)
                            if ( cp >= 0x10000 && cp <= 0x10FFFF )
                            {
                                a_CP  = cp;
                                a_Len = 4;
                                return;
                            }
                        }
                        // 3 bytes valid, 4th wrong/missing: maximal subpart = 3
                        a_CP  = 0xFFFD;
                        a_Len = 3;
                        return;
                    }
                    // 2 bytes valid, 3rd wrong/missing: maximal subpart = 2
                    a_CP  = 0xFFFD;
                    a_Len = 2;
                    return;
                }
                // No valid continuation: maximal subpart = 1
                a_CP  = 0xFFFD;
                a_Len = 1;
                return;
            }

            // Unexpected continuation byte or invalid lead byte (0xF8–0xFF):
            // maximal subpart = 1 byte.
            a_CP  = 0xFFFD;
            a_Len = 1;
        }
    };

    /**
     * @brief Lightweight range adaptor that exposes begin()/end() over a
     *        UTF8Iterator pair, enabling range-for loops:
     *
     * @code
     *     for ( c32 cp : UTF8Range( myStringView ) )
     *         Process( cp );
     * @endcode
     */
    struct UTF8Range
    {
        constexpr explicit UTF8Range( StringView a_String ) : m_String( a_String ) {}

        constexpr UTF8Iterator begin() const { return UTF8Iterator( m_String );      }
        constexpr UTF8Iterator end()   const { return UTF8Iterator::End( m_String ); }

    private:
        StringView m_String;
    };

} // namespace RatUI::Unicode
