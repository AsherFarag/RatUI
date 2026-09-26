#pragma once
#include "../Core.h"
#include "IFontFace.h"

namespace RatUI
{
    /**
     * @brief Owns font faces, groups them into families, and resolves (family, weight, style) to a face.
     * Matching follows CSS Fonts 4; bold / italic are only faked when no real face matches.
     */
    class FontLibrary
    {
    public:
        FontLibrary() = default;
        FontLibrary( const FontLibrary& ) = delete;
        FontLibrary& operator=( const FontLibrary& ) = delete;

        // - Faces

        FontFaceHandle AddFace( Unique<IFontFace> a_Face );
        IFontFace* GetFace( FontFaceHandle a_Face ) const;
        u32 GetFaceCount() const { return static_cast<u32>( Size( m_Faces ) ); }

        // - Families

        /** @brief */
        FontFamilyHandle RegisterFamily( StringID a_Name );
        FontFamilyHandle FindFamily( StringID a_Name ) const;

        bool AddFaceToFamily( FontFamilyHandle a_Family, FontFaceHandle a_Face, FontFaceDesc a_Desc = {} );
        FontFaceHandle AddFaceToFamily( FontFamilyHandle a_Family, Unique<IFontFace> a_Face, FontFaceDesc a_Desc = {} );

        /** @brief */
        void SetFamilySynthesis( FontFamilyHandle a_Family, FontSynthesis a_Synthesis );

        /** @brief Families searched (same weight / style) for characters the family doesn't have. */
        void SetFallbacks( FontFamilyHandle a_Family, Array<FontFamilyHandle> a_Fallbacks );

        /** @brief Searched after a family's own fallbacks. */
        void SetGlobalFallbacks( Array<FontFamilyHandle> a_Fallbacks );

        void SetDefaultFamily( FontFamilyHandle a_Family );
        FontFamilyHandle GetDefaultFamily() const { return m_DefaultFamily; }

        // - Matching

        /** @brief */
        ResolvedFace Resolve( const FontQuery& a_Query ) const;

        /** @brief Resolves a query for one codepoint, walking the fallbacks. Returns the primary face if nothing has it. */
        ResolvedFace ResolveCodepoint( const FontQuery& a_Query, codepoint a_Codepoint ) const;
        ResolvedFace ResolveCodepoint( const FontQuery& a_Query, const ResolvedFace& a_Primary, codepoint a_Codepoint ) const;

    private:
        struct FamilyFace
        {
            FontFaceHandle Face;
            FontFaceDesc   Desc;
        };

        struct Family
        {
            StringID                Name;
            Array<FamilyFace>       Faces;
            Array<FontFamilyHandle> Fallbacks;
            FontSynthesis           Synthesis{};
        };

        const Family* GetFamily( FontFamilyHandle a_Family ) const;
        ResolvedFace  ResolveUncached( const FontQuery& a_Query ) const;
        void          InvalidateCache() { ::RatUI::Clear( m_ResolveCache ); }

        static u64 QueryKey( const FontQuery& a_Query );

        Array<Unique<IFontFace>> m_Faces;
        Array<Family>            m_Families;
        Array<FontFamilyHandle>  m_GlobalFallbacks;
        FontFamilyHandle         m_DefaultFamily{};

        mutable HashMap<u64, ResolvedFace> m_ResolveCache;
    };

} // namespace RatUI
