#pragma once
#include "../Core.h"
#include "../Text/Text.h"
#include "../Renderer/Brush.h"
#include "../Layout/Layout.h" // TODO: Remove once CornerRounding is moved to a more appropriate header

namespace RatUI
{
    /**
     * @brief Represents a hash of a theme property name, used for efficient lookup of theme values.
     * ThemeIDs are generated from string names using a consistent hash function.
     */
    struct ThemeID
    {
        u64         Hash{ 0 };
        const char* Name{ nullptr }; ///< Optional: String the hash was generated from, used for debugging purposes. This must be set manually.

        constexpr auto operator<=>( const ThemeID& ) const = default;

        constexpr ThemeID() = default;
        constexpr explicit ThemeID( u64 a_Hash ) : Hash( a_Hash ) {}
        constexpr explicit ThemeID( StringView a_Name )
        {
            // Simple FNV-1a hash implementation for generating theme IDs from string names.
            // We avoid using std::hash on strings because it is not guaranteed to be consistent across different runs or platforms,
            // which would make theme IDs unreliable.
            // TODO: Since we prefer safer hashes over speed for theme IDs (the idea is to cache them),
            // investigate other hash functions.
            u64 hash = 14695981039346656037ull; // FNV offset basis
            for ( char c : a_Name )
            {
                hash ^= static_cast<u64>( c );
                hash *= 1099511628211ull; // FNV prime
            }
            Hash = hash;
        }
    };

    namespace Literals
    {
        /**
         * @brief User-defined literal for creating ThemeIDs from string literals. Usage: "MyWidget.StyleVar"_theme
         */
        constexpr ThemeID operator"" _theme( const char* a_String, size_t a_Length )
        {
            ThemeID id( StringView( a_String, a_Length ) );
            id.Name = a_String; // Store the string pointer for debugging (we assume string literals have static storage duration, so this is safe)
            return id;
        }
    } // namespace Literals

    struct ThemeIDHash
    {
        size_t operator()( const ThemeID& a_ID ) const
        {
            return std::hash<u64>{}( a_ID.Hash );
        }
    };
    
    /**
     * @brief A collection of typed theme properties (colors, metrics, etc.)
     * that can optionally inherit from a parent, cascading lookups up the chain.
     */
    class Theme
    {
    public:
        template<typename T>
        using ValueMap = HashMap<ThemeID, T, ThemeIDHash, std::equal_to<ThemeID>>;
    
        Theme() = default;

        explicit Theme( Shared<const Theme> a_Parent )
            : m_Parent( std::move( a_Parent ) )
        {}
    
        Theme( const Theme& a_Other )
            : m_Parent    ( a_Other.m_Parent )
            , m_Colors    ( a_Other.m_Colors )
            , m_Roundings ( a_Other.m_Roundings )
            , m_TextStyles( a_Other.m_TextStyles )
            , m_Metrics   ( a_Other.m_Metrics )
            , m_Fonts     ( a_Other.m_Fonts )
            , m_Brushes   ( a_Other.m_Brushes )
            // m_Version intentionally starts at 0 for a fresh copy
        {}
    
        Theme( Theme&& ) = default;
    
        Theme& operator=( const Theme& a_Other )
        {
            if ( this != &a_Other )
            {
                RATUI_USER_ASSERT( a_Other.m_Parent.get() != this, 
                                   "Cannot assign a theme to one of its ancestors (circular parent chain)" );

                m_Parent     = a_Other.m_Parent;
                m_Colors     = a_Other.m_Colors;
                m_Roundings  = a_Other.m_Roundings;
                m_TextStyles = a_Other.m_TextStyles;
                m_Metrics    = a_Other.m_Metrics;
				m_Fonts      = a_Other.m_Fonts;
                m_Brushes    = a_Other.m_Brushes;
                ++m_Version; // assignment is a mutation of this theme
            }

            return *this;
        }
    
        Theme& operator=( Theme&& a_Other ) noexcept
        {
            if ( this != &a_Other )
            {
                RATUI_USER_ASSERT( a_Other.m_Parent.get() != this, 
                                   "Cannot assign a theme to one of its ancestors (circular parent chain)" );
                m_Parent     = std::move( a_Other.m_Parent );
                m_Colors     = std::move( a_Other.m_Colors );
                m_Roundings  = std::move( a_Other.m_Roundings );
                m_TextStyles = std::move( a_Other.m_TextStyles );
                m_Metrics    = std::move( a_Other.m_Metrics );
                m_Fonts	     = std::move( a_Other.m_Fonts );
                m_Brushes	 = std::move( a_Other.m_Brushes );
                ++m_Version;
            }
            return *this;
        }
    
        /** @brief Returns the parent theme, if any. */
        const Shared<const Theme>& GetParent() const { return m_Parent; }
    
        /** @brief Checks if the theme has a parent. */
        bool HasParent() const { return m_Parent != nullptr; }

        /**
         * @brief Returns the version number of the theme.
         * Monotonically increasing counter, incremented on every write to this
         * theme (not its parent). Use as a cache-invalidation signal.
         */
        u32 GetVersion() const { return m_Version; }
    
        // -------------------------------------------------------------------------
        // Per-type accessors  (generated via below)
        // -------------------------------------------------------------------------
        //
        // For each type T, the following methods are generated:
        //
        //   const T&           GetColor( ThemeID, const T& default = {} ) const
        //   const T*           TryGetColor( ThemeID ) const           // nullptr if absent
        //   bool               HasColor( ThemeID ) const              // checks parent chain
        //   Theme&             SetColor( ThemeID, T )                 // fluent
        //   Theme&             SetColors( std::initializer_list<...> )// bulk, merges
        //   const ValueMap<T>& GetColors() const
    
    #define RATUI_THEME_PROPERTY( Type, Singular, Plural, Member )                          \
        const Type& Get##Singular( ThemeID a_ID, const Type& a_Default = {} ) const         \
        {                                                                                   \
            if ( const Type* v = TryGet##Singular( a_ID ) ) return *v;                      \
            return a_Default;                                                               \
        }                                                                                   \
        const Type* TryGet##Singular( ThemeID a_ID ) const                                  \
        {                                                                                   \
            if ( auto it = Find( Member, a_ID ); it != End( Member ) )                      \
                return &it->second;                                                         \
            return m_Parent ? m_Parent->TryGet##Singular( a_ID ) : nullptr;                 \
        }                                                                                   \
        bool Has##Singular( ThemeID a_ID ) const                                            \
        {                                                                                   \
            if ( Find( Member, a_ID ) != End( Member ) ) return true;                       \
            return m_Parent && m_Parent->Has##Singular( a_ID );                             \
        }                                                                                   \
        Theme& Set##Singular( ThemeID a_ID, Type a_Value )                                  \
        {                                                                                   \
            Member[a_ID] = std::move( a_Value );                                            \
            ++m_Version;                                                                    \
            return *this;                                                                   \
        }                                                                                   \
        Theme& Set##Plural( std::initializer_list<std::pair<const ThemeID, Type>> a_List )  \
        {                                                                                   \
            for ( auto& [id, val] : a_List )                                                \
                Member[id] = std::move( val );                                              \
            ++m_Version;                                                                    \
            return *this;                                                                   \
        }                                                                                   \
        const ValueMap<Type>& Get##Plural() const { return Member; }
    
        RATUI_THEME_PROPERTY( Color,           Color,     Colors,     m_Colors     )
        RATUI_THEME_PROPERTY( CornerRounding,  Rounding,  Roundings,  m_Roundings  )
        RATUI_THEME_PROPERTY( TextRenderStyle, TextStyle, TextStyles, m_TextStyles )
        RATUI_THEME_PROPERTY( Unit,            Metric,    Metrics,    m_Metrics    )
        RATUI_THEME_PROPERTY( FontHandle,      Font,      Fonts,      m_Fonts      )
        RATUI_THEME_PROPERTY( Brush,           Brush,     Brushes,    m_Brushes    )
    
    #undef RATUI_THEME_PROPERTY
    
    protected:
        Shared<const Theme> m_Parent;
    
        u32 m_Version { 0 };
    
        ValueMap<Color>           m_Colors;
        ValueMap<CornerRounding>  m_Roundings;
        ValueMap<TextRenderStyle> m_TextStyles;
        ValueMap<Unit>            m_Metrics;
		ValueMap<FontHandle>      m_Fonts;
        ValueMap<Brush>           m_Brushes;
    };

    /**
     * @brief A handle to a theme, which may or may not contain a valid theme pointer. 
     * Provides convenient accessors that return default values when the pointer is null.
     */
    struct ThemeHandle
    {
        Shared<const Theme> Ptr;

		ThemeHandle() = default;
		ThemeHandle( Shared<const Theme> a_Theme ) : Ptr( std::move( a_Theme ) ) {}
		ThemeHandle( const Shared<Theme>& a_Theme ) : Ptr( a_Theme ) {}

        // -------------------------------------------------------------------------
        // Per-type accessors  (generated via below)
        // -------------------------------------------------------------------------
        //
        // For each type T, the following methods are generated:
        //
        //   const T&           GetColor( ThemeID, const T& default = {} ) const
        //   const T*           TryGetColor( ThemeID ) const           // nullptr if absent
        //   bool               HasColor( ThemeID ) const              // checks parent chain

    #define THEME_HANDLE_METHODS( Type, Name )                                     \
        const Type& Get##Name( ThemeID a_ID, const Type& a_Default = {} ) const    \
        {                                                                          \
            return Ptr ? Ptr->Get##Name( a_ID, a_Default ) : a_Default;            \
        }                                                                          \
        const Type* TryGet##Name( ThemeID a_ID ) const                             \
        {                                                                          \
            return Ptr ? Ptr->TryGet##Name( a_ID ) : nullptr;                      \
        }                                                                          \
        bool Has##Name( ThemeID a_ID ) const                                       \
        {                                                                          \
            return Ptr && Ptr->Has##Name( a_ID );                                  \
        }
        
        THEME_HANDLE_METHODS( Color,           Color )
        THEME_HANDLE_METHODS( CornerRounding,  Rounding )
        THEME_HANDLE_METHODS( TextRenderStyle, TextStyle )
        THEME_HANDLE_METHODS( Unit,            Metric )
        THEME_HANDLE_METHODS( FontHandle,      Font )
        THEME_HANDLE_METHODS( Brush,           Brush )

    #undef THEME_HANDLE_METHODS
    };

    /**
     * @brief Namespace containing predefined themes that can be used out of the box. 
     */
    namespace Themes
    {
        /**
         * @brief Returns the dark theme.
         * @return A shared pointer to the dark theme.
         */
        const Shared<const Theme>& Dark();
    }

    /**
     * @brief Namespace containing predefined theme property IDs for the built-in widgets.
     * These can be used when setting or getting theme values to ensure consistency across themes.
     */
    namespace ThemeKey
    {
        namespace Color
        {
            inline constexpr ThemeID FocusOutline = "FocusOutline"_theme;

            inline constexpr ThemeID ButtonBorder  = "Button.Border"_theme;

            inline constexpr ThemeID PanelBorder = "Panel.Border"_theme;

            inline constexpr ThemeID SliderTrack        = "Slider.Track"_theme;
            inline constexpr ThemeID SliderTrackFill    = "Slider.TrackFill"_theme;
            inline constexpr ThemeID SliderThumb        = "Slider.Thumb"_theme;
            inline constexpr ThemeID SliderThumbHover   = "Slider.ThumbHover"_theme;
            inline constexpr ThemeID SliderThumbPressed = "Slider.ThumbPressed"_theme;
        }

        namespace Rounding
        {
            inline constexpr ThemeID FocusOutline = "FocusOutline"_theme;
            inline constexpr ThemeID Button       = "Button"_theme;
            inline constexpr ThemeID Panel        = "Panel"_theme;
            inline constexpr ThemeID SliderTrack  = "Slider.Track"_theme;
            inline constexpr ThemeID SliderThumb  = "Slider.Thumb"_theme;
        }

        namespace TextStyle
        {
            inline constexpr ThemeID Default = "Default"_theme;
        }

        namespace Metric
        {
			inline constexpr ThemeID FocusOutlineThickness = "FocusOutlineThickness"_theme;
            inline constexpr ThemeID ButtonBorderThickness = "Button.BorderThickness"_theme;
            inline constexpr ThemeID PanelBorderThickness  = "Panel.BorderThickness"_theme;
            inline constexpr ThemeID SliderTrackThickness  = "Slider.TrackThickness"_theme;
            inline constexpr ThemeID SliderMinThumbSize    = "Slider.MinThumbSize"_theme;
        }

		namespace Font 
        {
			inline constexpr ThemeID Default = "Default"_theme;
		}

        namespace Brush
        {
            inline constexpr ThemeID ButtonNormal  = "Button.Normal"_theme;
            inline constexpr ThemeID ButtonHover   = "Button.Hover"_theme;
            inline constexpr ThemeID ButtonPressed = "Button.Pressed"_theme;

            inline constexpr ThemeID PanelNormal   = "Panel.Normal"_theme;
        }
    }

} // namespace RatUI