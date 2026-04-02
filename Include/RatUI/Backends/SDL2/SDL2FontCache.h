#pragma once
#include "../../RatUI.h"
#include <SDL_ttf.h>
#include <unordered_map>

namespace RatUI::SDL2
{
    // TODO: Should a HashMap core trait be added so we can use that instead of std::unordered_map here and in TextLayout?

    /**
     * @brief Caches TTF_Font* instances for different font styles to avoid redundant loading.
     * Maintains a mapping from FontHandle IDs to file paths, and from FontKey (style variants) to loaded TTF_Font*.
     */
    class SDL2FontCache
    {
    public:

        /** @brief Key for identifying a specific font variant in the cache. */
        struct FontKey
        {
            FontHandle Handle  = {};
            i32  Size          = 0; ///< Truncated to int because SDL_ttf does not support sub-pixel sizes.
            bool Bold          = false;
            bool Italic        = false;
            bool Underline     = false;
            bool Strikethrough = false;

            bool operator==( const FontKey& ) const = default;
        };

        /** @brief Hash function for FontHandle to be used in unordered_map. */
        struct FontHandleHasher
        {
            size_t operator()( const FontHandle& h ) const noexcept
            { 
                return std::hash<u32>{}( h.ID );
            }
        };

        /** @brief Hash function for FontKey to be used in unordered_map. */
        struct FontKeyHasher
        {
            size_t operator()( const FontKey& k ) const noexcept
            {
                // FNV1a hash combine
                constexpr size_t basis = 2166136261u;
                constexpr size_t prime = 16777619u;

                size_t h = basis;
                h = ( h ^ FontHandleHasher{} ( k.Handle        ) ) * prime;
                h = ( h ^ std::hash<i32>{}   ( k.Size          ) ) * prime;
                h = ( h ^ std::hash<bool>{}  ( k.Bold          ) ) * prime;
                h = ( h ^ std::hash<bool>{}  ( k.Italic        ) ) * prime;
                h = ( h ^ std::hash<bool>{}  ( k.Underline     ) ) * prime;
                h = ( h ^ std::hash<bool>{}  ( k.Strikethrough ) ) * prime;
                return h;
            }
        };
  
        SDL2FontCache() = default;
        ~SDL2FontCache() { Clear(); }

        // Non-copyable because it owns TTF_Font* handles.
        SDL2FontCache( const SDL2FontCache& ) = delete;
        SDL2FontCache& operator=( const SDL2FontCache& ) = delete;

        SDL2FontCache( SDL2FontCache&& other ) noexcept
            : m_FontFiles( std::move( other.m_FontFiles ) )
            , m_FontCache( std::move( other.m_FontCache ) )
        {}

        SDL2FontCache& operator=( SDL2FontCache&& other ) noexcept
        {
            if ( this != &other )
            {
                Clear();
                m_FontFiles = std::move( other.m_FontFiles );
                m_FontCache = std::move( other.m_FontCache );
            }

            return *this;
        }

        /**
         * @brief Associates a FontHandle ID with a font file path.
         * @param a_Handle The font handle to register.
         * @param a_Filepath Path to the .ttf or .otf file.
         */
        void RegisterFont( FontHandle a_Handle, String a_Filepath )
        {
            // If this ID was registered before with a different path, evict stale entries.
            if ( auto it = m_FontFiles.find( a_Handle ); it != m_FontFiles.end() )
            {
                if ( it->second != a_Filepath )
                    EvictID( a_Handle );

                it->second = std::move( a_Filepath );
            }
            else
            {
                m_FontFiles.emplace( a_Handle, std::move( a_Filepath ) );
            }
        }

        /** @brief Returns true if the given FontHandle has a registered file path. */
        bool IsRegistered( FontHandle a_Handle ) const
        {
            return m_FontFiles.contains( a_Handle );
        }

        /**
         * @brief Returns a TTF_Font* for the given style, loading and caching it on first use.
         * @return A valid TTF_Font* on success, nullptr if the FontHandle is invalid or if loading fails.
         */
        TTF_Font* GetFont( const TextStyle& a_Style )
        {
            return GetFont( MakeKey( a_Style ) );
        }

        /**
         * @brief Overload that accepts a pre-built FontKey directly.
         * Useful when the caller has already constructed the key.
         */
        TTF_Font* GetFont( const FontKey& a_Key )
        {
            if ( !a_Key.Handle.IsValid() )
                return nullptr;

            if ( auto it = m_FontCache.find( a_Key ); it != m_FontCache.end() )
                return it->second;

            return LoadAndCache( a_Key );
        }

        /**
         * @brief Evicts all cached font variants for the given handle ID.
         * Does not unregister the file path - the font can still be loaded again.
         */
        void EvictID( FontHandle a_Handle )
        {
            for ( auto it = m_FontCache.begin(); it != m_FontCache.end(); )
            {
                if ( it->first.Handle == a_Handle )
                {
                    TTF_CloseFont( it->second );
                    it = m_FontCache.erase( it );
                }
                else
                {
                    ++it;
                }
            }
        }

        /** @brief Closes all cached fonts and clears all registrations. */
        void Clear()
        {
            for ( auto& [_, font] : m_FontCache )
                TTF_CloseFont( font );

            m_FontCache.clear();
            m_FontFiles.clear();
        }

        /** @brief Returns the number of currently cached font variants. */
        size_t CachedCount() const { return m_FontCache.size(); }

        /** @brief Builds a FontKey from a TextStyle. */
        static constexpr FontKey MakeKey( const TextStyle& a_Style )
        {
            return FontKey{
                .Handle        = a_Style.Font,
                .Size          = static_cast<i32>( a_Style.Size ),
                .Bold          = a_Style.Bold,
                .Italic        = a_Style.Italic,
                .Underline     = a_Style.Underline,
                .Strikethrough = a_Style.Strikethrough
            };
        }

        /**
         * @brief Returns the line height for the given style and font.
         * If TextStyle::LineHeight is non-zero, that value is used directly.
         * Otherwise, TTF_FontLineSkip is used (recommended by SDL_ttf docs).
         */
        static f32 GetLineHeight( TTF_Font* a_Font, const TextStyle& a_Style )
        {
            if ( a_Style.LineHeight > 0.f )
                return a_Style.LineHeight;

            return static_cast<f32>( TTF_FontLineSkip( a_Font ) );
        }

    private:

        /**
         * @brief Loads a font from disk with the style encoded in FontKey, inserts it into the cache, and returns the pointer.
         * @return A valid TTF_Font* on success, nullptr if loading fails.
         */
        TTF_Font* LoadAndCache( const FontKey& a_Key )
        {
            auto fileIt = m_FontFiles.find( a_Key.Handle );
            if ( fileIt == m_FontFiles.end() )
                return nullptr; // No file registered for this ID.

            TTF_Font* font = TTF_OpenFont( fileIt->second.c_str(), a_Key.Size );
            if ( !font )
                return nullptr; // SDL_ttf will have logged the error.

            i32 flags = TTF_STYLE_NORMAL;
            if ( a_Key.Bold          ) flags |= TTF_STYLE_BOLD;
            if ( a_Key.Italic        ) flags |= TTF_STYLE_ITALIC;
            if ( a_Key.Underline     ) flags |= TTF_STYLE_UNDERLINE;
            if ( a_Key.Strikethrough ) flags |= TTF_STYLE_STRIKETHROUGH;
            TTF_SetFontStyle( font, flags );

            m_FontCache.emplace( a_Key, font );
            return font;
        }

        std::unordered_map<FontHandle, String, FontHandleHasher> m_FontFiles;
        std::unordered_map<FontKey, TTF_Font*, FontKeyHasher>    m_FontCache;
    };

} // namespace RatUI::SDL2