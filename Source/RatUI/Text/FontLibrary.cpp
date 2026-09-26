#include <RatUI/Text/FontLibrary.h>

namespace RatUI
{
    namespace
    {
        /** @brief CSS Fonts 4 weight matching order, lower is better: ( group << 16 ) | distance. */
        u32 WeightRank( u32 a_Desired, u32 a_Candidate )
        {
            const auto rank = []( u32 a_Group, u32 a_Distance ) { return ( a_Group << 16 ) | ( a_Distance & 0xFFFF ); };

            // 400-500: up to 500, then lighter, then heavier.
            if ( a_Desired >= 400 && a_Desired <= 500 )
            {
                if ( a_Candidate >= a_Desired && a_Candidate <= 500 )
                    return rank( 0, a_Candidate - a_Desired );
                if ( a_Candidate < a_Desired )
                    return rank( 1, a_Desired - a_Candidate );
                return rank( 2, a_Candidate - a_Desired );
            }

            // Below 400: lighter first, then heavier.
            if ( a_Desired < 400 )
            {
                if ( a_Candidate <= a_Desired )
                    return rank( 0, a_Desired - a_Candidate );
                return rank( 1, a_Candidate - a_Desired );
            }

            // Above 500: heavier first, then lighter.
            if ( a_Candidate >= a_Desired )
                return rank( 0, a_Candidate - a_Desired );
            return rank( 1, a_Desired - a_Candidate );
        }

        /** @brief CSS style fallback order. */
        FixedArray<EFontStyle, 3> StyleOrder( EFontStyle a_Desired )
        {
            switch ( a_Desired )
            {
                case EFontStyle::Italic:  return { EFontStyle::Italic,  EFontStyle::Oblique, EFontStyle::Normal };
                case EFontStyle::Oblique: return { EFontStyle::Oblique, EFontStyle::Italic,  EFontStyle::Normal };
                case EFontStyle::Normal:
                default:                  return { EFontStyle::Normal,  EFontStyle::Oblique, EFontStyle::Italic };
            }
        }

        constexpr u32 c_BoldThreshold = 600;
    }

    FontFaceHandle FontLibrary::AddFace( Unique<IFontFace> a_Face )
    {
        if ( !a_Face )
            return {};

        PushBack( m_Faces, std::move( a_Face ) );
        InvalidateCache();
        return FontFaceHandle{ static_cast<u32>( Size( m_Faces ) ) };
    }

    IFontFace* FontLibrary::GetFace( FontFaceHandle a_Face ) const
    {
        if ( !a_Face.IsValid() || a_Face.ID > Size( m_Faces ) )
            return nullptr;

        return m_Faces[a_Face.ID - 1].get();
    }

    FontFamilyHandle FontLibrary::RegisterFamily( StringID a_Name )
    {
        if ( const FontFamilyHandle existing = FindFamily( a_Name ); existing.IsValid() )
            return existing;

        PushBack( m_Families, Family{ .Name = a_Name } );
        const FontFamilyHandle handle{ static_cast<u32>( Size( m_Families ) ) };

        if ( !m_DefaultFamily.IsValid() )
            m_DefaultFamily = handle;

        InvalidateCache();
        return handle;
    }

    FontFamilyHandle FontLibrary::FindFamily( StringID a_Name ) const
    {
        for ( u32 i = 0; i < Size( m_Families ); ++i )
        {
            if ( m_Families[i].Name == a_Name )
                return FontFamilyHandle{ i + 1 };
        }
        return {};
    }

    bool FontLibrary::AddFaceToFamily( FontFamilyHandle a_Family, FontFaceHandle a_Face, FontFaceDesc a_Desc )
    {
        if ( !a_Family.IsValid() || a_Family.ID > Size( m_Families ) || !GetFace( a_Face ) )
            return false;

        PushBack( m_Families[a_Family.ID - 1].Faces, FamilyFace{ a_Face, a_Desc } );
        InvalidateCache();
        return true;
    }

    FontFaceHandle FontLibrary::AddFaceToFamily( FontFamilyHandle a_Family, Unique<IFontFace> a_Face, FontFaceDesc a_Desc )
    {
        const FontFaceHandle face = AddFace( std::move( a_Face ) );
        if ( !AddFaceToFamily( a_Family, face, a_Desc ) )
            return {};
        return face;
    }

    void FontLibrary::SetFamilySynthesis( FontFamilyHandle a_Family, FontSynthesis a_Synthesis )
    {
        if ( !a_Family.IsValid() || a_Family.ID > Size( m_Families ) )
            return;

        m_Families[a_Family.ID - 1].Synthesis = a_Synthesis;
        InvalidateCache();
    }

    void FontLibrary::SetFallbacks( FontFamilyHandle a_Family, Array<FontFamilyHandle> a_Fallbacks )
    {
        if ( !a_Family.IsValid() || a_Family.ID > Size( m_Families ) )
            return;

        m_Families[a_Family.ID - 1].Fallbacks = std::move( a_Fallbacks );
    }

    void FontLibrary::SetGlobalFallbacks( Array<FontFamilyHandle> a_Fallbacks )
    {
        m_GlobalFallbacks = std::move( a_Fallbacks );
    }

    void FontLibrary::SetDefaultFamily( FontFamilyHandle a_Family )
    {
        m_DefaultFamily = a_Family;
        InvalidateCache();
    }

    const FontLibrary::Family* FontLibrary::GetFamily( FontFamilyHandle a_Family ) const
    {
        if ( !a_Family.IsValid() )
            a_Family = m_DefaultFamily;

        if ( !a_Family.IsValid() || a_Family.ID > Size( m_Families ) )
            return nullptr;

        return &m_Families[a_Family.ID - 1];
    }

    u64 FontLibrary::QueryKey( const FontQuery& a_Query )
    {
        return ( static_cast<u64>( a_Query.Family.ID ) << 32 )
             | ( static_cast<u64>( ToUnderlying( a_Query.Weight ) ) << 16 )
             | ( static_cast<u64>( ToUnderlying( a_Query.Style ) ) << 8 )
             | ( a_Query.Synthesis.Bold ? 1u : 0u )
             | ( a_Query.Synthesis.Italic ? 2u : 0u );
    }

    ResolvedFace FontLibrary::Resolve( const FontQuery& a_Query ) const
    {
        const u64 key = QueryKey( a_Query );
        if ( const auto it = Find( m_ResolveCache, key ); it != End( m_ResolveCache ) )
            return it->second;

        const ResolvedFace result = ResolveUncached( a_Query );
        m_ResolveCache[key] = result;
        return result;
    }

    ResolvedFace FontLibrary::ResolveUncached( const FontQuery& a_Query ) const
    {
        const Family* family = GetFamily( a_Query.Family );
        if ( !family || Empty( family->Faces ) )
            return {};

        // Style first: the first style (in CSS order) that has any faces.
        EFontStyle chosenStyle = EFontStyle::Normal;
        bool found = false;
        for ( EFontStyle style : StyleOrder( a_Query.Style ) )
        {
            for ( const FamilyFace& face : family->Faces )
            {
                if ( face.Desc.Style == style )
                {
                    chosenStyle = style;
                    found = true;
                    break;
                }
            }
            if ( found )
                break;
        }

        const u32 desired = ToUnderlying( a_Query.Weight );
        const FamilyFace* best = nullptr;
        u32 bestRank = Limits<u32>::max();

        for ( const FamilyFace& face : family->Faces )
        {
            if ( face.Desc.Style != chosenStyle )
                continue;

            const u32 rank = WeightRank( desired, ToUnderlying( face.Desc.Weight ) );
            if ( rank < bestRank )
            {
                bestRank = rank;
                best = &face;
            }
        }

        if ( !best )
            return {};

        // Fake it only when no real face qualified.
        const bool allowBold   = a_Query.Synthesis.Bold && family->Synthesis.Bold;
        const bool allowItalic = a_Query.Synthesis.Italic && family->Synthesis.Italic;

        return ResolvedFace{
            .Face        = best->Face,
            .SynthBold   = allowBold && desired >= c_BoldThreshold && ToUnderlying( best->Desc.Weight ) < c_BoldThreshold,
            .SynthItalic = allowItalic && a_Query.Style != EFontStyle::Normal && chosenStyle == EFontStyle::Normal,
        };
    }

    ResolvedFace FontLibrary::ResolveCodepoint( const FontQuery& a_Query, codepoint a_Codepoint ) const
    {
        return ResolveCodepoint( a_Query, Resolve( a_Query ), a_Codepoint );
    }

    ResolvedFace FontLibrary::ResolveCodepoint( const FontQuery& a_Query, const ResolvedFace& a_Primary, codepoint a_Codepoint ) const
    {
        if ( const IFontFace* primary = GetFace( a_Primary.Face ); primary && primary->HasGlyph( a_Codepoint ) )
            return a_Primary;

        const auto tryFamily = [&]( FontFamilyHandle a_Family ) -> ResolvedFace
        {
            FontQuery fallbackQuery = a_Query;
            fallbackQuery.Family = a_Family;

            const ResolvedFace resolved = Resolve( fallbackQuery );
            const IFontFace* face = GetFace( resolved.Face );
            return ( face && face->HasGlyph( a_Codepoint ) ) ? resolved : ResolvedFace{};
        };

        if ( const Family* family = GetFamily( a_Query.Family ) )
        {
            for ( FontFamilyHandle fallback : family->Fallbacks )
            {
                if ( const ResolvedFace resolved = tryFamily( fallback ); resolved.IsValid() )
                    return resolved;
            }
        }

        for ( FontFamilyHandle fallback : m_GlobalFallbacks )
        {
            if ( const ResolvedFace resolved = tryFamily( fallback ); resolved.IsValid() )
                return resolved;
        }

        return a_Primary;
    }

} // namespace RatUI
