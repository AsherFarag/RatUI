#pragma once
#include "../Core.h"
#include "../Text/Text.h"
#include "../Renderer/Brush.h"
#include "../Layout/Layout.h" // TODO: Remove once CornerRounding is moved to a more appropriate header

namespace RatUI
{
    /**
     * @brief Apply @p x to each of the theme property types.
     * Type, Singular, Plural
     */
    #define RATUI_THEME_PROPERTIES(X) \
        X(Color,           Color,     Colors     ) \
        X(CornerRounding,  Rounding,  Roundings  ) \
        X(TextRenderStyle, TextStyle, TextStyles ) \
        X(Unit,            Metric,    Metrics    ) \
        X(FontHandle,      Font,      Fonts      ) \
        X(Brush,           Brush,     Brushes    )

    /**
     * @brief A collection of typed theme properties (colors, metrics, etc.)
     * that can optionally inherit from a parent, cascading lookups up the chain.
     */
    class Theme
    {
    public:
        template<typename T>
        using ValueMap = HashMap<StringID, T, StringIDHash, std::equal_to<StringID>>;
    
        Theme() = default;
        Theme( const Theme& a_Other ) { CopyFrom( a_Other ); }
        Theme( Theme&& ) noexcept = default;
        explicit Theme( Shared<const Theme> a_Parent ) 
            : m_Parent( std::move( a_Parent ) ) 
        {}
    
        Theme& operator=( const Theme& a_Other )
        {
            if ( this != &a_Other )
            {
                RATUI_USER_ASSERT( ValidateParentChain( a_Other.m_Parent.get() ), 
                                   "Cannot assign a theme to one of its ancestors (circular parent chain)" );
                CopyFrom( a_Other );
                ++m_Version; // assignment is a mutation of this theme
            }

            return *this;
        }
    
        Theme& operator=( Theme&& a_Other ) noexcept
        {
            if ( this != &a_Other )
            {
                RATUI_USER_ASSERT( ValidateParentChain( a_Other.m_Parent.get() ), 
                                   "Cannot assign a theme to one of its ancestors (circular parent chain)" );
                MoveFrom( std::move( a_Other ) );
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
        u64 GetVersion() const { return m_Version; }

        /**
         * @brief Validates that the given parent theme is not an ancestor of this theme, preventing circular references.
         */
        bool ValidateParentChain(const Theme* a_Parent) const
        {
            while (a_Parent)
            {
                if (a_Parent == this)
                    return false;
            
                a_Parent = a_Parent->m_Parent.get();
            }
        
            return true;
        }
    
        // -------------------------------------------------------------------------
        // Per-type accessors  (generated via below)
        // -------------------------------------------------------------------------
        //
        // For each type T, the following methods are generated:
        //
        //   const T&           GetColor( StringID, const T& default = {} ) const
        //   const T*           TryGetColor( StringID ) const           
        //   Theme&             SetColor( StringID, T )                 
        //   Theme&             SetColors( std::initializer_list<...> )
        //   const ValueMap<T>& GetColors() const
    
    #define RATUI_THEME_METHODS( Type, Singular, Plural )                                   \
        const Type& Get##Singular( StringID a_ID, const Type& a_Default = {} ) const        \
        {                                                                                   \
            if ( const Type* v = TryGet##Singular( a_ID ) ) return *v;                      \
            return a_Default;                                                               \
        }                                                                                   \
        const Type* TryGet##Singular( StringID a_ID ) const                                 \
        {                                                                                   \
            if ( auto it = Find( m_##Plural, a_ID ); it != End( m_##Plural ) )              \
                return &it->second;                                                         \
            return m_Parent ? m_Parent->TryGet##Singular( a_ID ) : nullptr;                 \
        }                                                                                   \
        Theme& Set##Singular( StringID a_ID, Type a_Value )                                 \
        {                                                                                   \
            m_##Plural[a_ID] = std::move( a_Value );                                        \
            ++m_Version;                                                                    \
            return *this;                                                                   \
        }                                                                                   \
        Theme& Set##Plural( std::initializer_list<std::pair<const StringID, Type>> a_List ) \
        {                                                                                   \
            for ( auto& [id, val] : a_List )                                                \
                m_##Plural[id] = val;                                                       \
            ++m_Version;                                                                    \
            return *this;                                                                   \
        }                                                                                   \
        const ValueMap<Type>& Get##Plural() const { return m_##Plural; }

        RATUI_THEME_PROPERTIES( RATUI_THEME_METHODS )
    
    #undef RATUI_THEME_METHODS
    
    private:
        void CopyFrom( const Theme& a_Other )
        {
            m_Parent = a_Other.m_Parent;
        #define COPY_VALUE_MAP( Type, Singular, Plural ) m_##Plural = a_Other.m_##Plural;
            RATUI_THEME_PROPERTIES( COPY_VALUE_MAP )
        #undef COPY_VALUE_MAP
        }

        void MoveFrom( Theme&& a_Other )
        {
            m_Parent = std::move( a_Other.m_Parent );
        #define MOVE_VALUE_MAP( Type, Singular, Plural ) m_##Plural = std::move( a_Other.m_##Plural );
            RATUI_THEME_PROPERTIES( MOVE_VALUE_MAP )
        #undef MOVE_VALUE_MAP
        }

        Shared<const Theme> m_Parent;
        u64 m_Version { 0 };

    #define DECLARE_VALUE_MAP( Type, Singular, Plural ) ValueMap<Type> m_##Plural;
        RATUI_THEME_PROPERTIES( DECLARE_VALUE_MAP )
    #undef DECLARE_VALUE_MAP
    };

    /**
     * @brief A handle to a theme, which may or may not contain a valid theme pointer. 
     * Provides convenient accessors that return default values when the pointer is null.
     */
    class ThemeHandle
    {
    public:
		ThemeHandle() = default;
		ThemeHandle( Shared<const Theme> a_Theme ) noexcept 
            : m_Ptr( std::move( a_Theme ) ), m_Version( ~0ull ) 
        {}
        ThemeHandle( Shared<Theme> a_Theme ) noexcept
            : m_Ptr( std::move( a_Theme ) ), m_Version( ~0ull )
        {}
        ThemeHandle( const ThemeHandle& a_Other )           
            : m_Ptr( a_Other.m_Ptr ), m_Version( ~0ull ) 
        {}
        ThemeHandle( ThemeHandle&& a_Other ) noexcept 
            : m_Ptr( std::move( a_Other.m_Ptr ) ), m_Version( ~0ull ) 
        { 
            a_Other.Invalidate();
        }

        ThemeHandle& operator=( const ThemeHandle& a_Other )
        {
            if ( this == &a_Other ) return *this;
            Reset( a_Other.m_Ptr );
            return *this;
        }

        ThemeHandle& operator=( ThemeHandle&& a_Other ) noexcept
        {
            if ( this == &a_Other ) return *this;
            Reset( std::move( a_Other.m_Ptr ) );
            a_Other.Invalidate();
            return *this;
        }

        ThemeHandle& operator=( Shared<const Theme> a_Theme )
        {
            Reset( std::move( a_Theme ) );
            return *this;
        }

        ThemeHandle& operator=( Shared<Theme> a_Theme )
        {
            Reset( std::move( a_Theme ) );
            return *this;
        }

        /**
         * @brief Checks if the handle currently points to a valid theme.
         * @return True if the handle is valid, false otherwise.
         */
        RATUI_NODISCARD bool IsValid() const 
        { 
            return m_Ptr != nullptr; 
        }

        RATUI_NODISCARD bool IsOutdated() const 
        { 
            return m_Ptr && m_Ptr->GetVersion() != m_Version; 
        }

        /**
         * @brief Invalidates the cached version number, forcing the next call to Update() to return true.
         */
        void Invalidate() 
        { 
            m_Version = ~0ull; 
        }

        /**
         * @brief Resets the handle to an empty state, clearing the underlying theme pointer and invalidating the cached version number.
         */
        void Reset( Shared<const Theme> a_Theme = nullptr ) 
        { 
            m_Ptr = std::move( a_Theme ); 
            Invalidate(); 
        }

        /**
         * @brief Checks if the underlying theme has changed since the last time this handle was updated.
         * This is useful for caching purposes, to determine if any theme-dependent values need to be recalculated.
         * If the theme has changed, this method updates the cached version number to the current version of the theme.
         * @return True if the theme has changed, false otherwise.
         * 
         * @example
         * if ( themeHandle.Update() )
         * {
         *    BorderThickness = themeHandle.GetMetric( ... );
         * }
         */
        bool Update()
        {
            if ( m_Ptr && m_Ptr->GetVersion() != m_Version )
            {
                m_Version = m_Ptr->GetVersion();
                return true;
            }

            return false;
        }

        operator bool() const { return IsValid(); }

        // -------------------------------------------------------------------------
        // Per-type accessors  (generated via below)
        // -------------------------------------------------------------------------
        //
        // For each type T, the following methods are generated:
        //
        //   const T&           GetColor( StringID, const T& default = {} ) const
        //   const T*           TryGetColor( StringID ) const           
        //   bool               HasColor( StringID ) const              

    #define RATUI_THEME_METHODS( Type, Name, Plural )                                              \
        RATUI_NODISCARD const Type& Get##Name( StringID a_ID, const Type& a_Default = {} ) const   \
        {                                                                                          \
            return m_Ptr ? m_Ptr->Get##Name( a_ID, a_Default ) : a_Default;                        \
        }                                                                                          \
        RATUI_NODISCARD const Type* TryGet##Name( StringID a_ID ) const                            \
        {                                                                                          \
            return m_Ptr ? m_Ptr->TryGet##Name( a_ID ) : nullptr;                                  \
        }
        
        RATUI_THEME_PROPERTIES( RATUI_THEME_METHODS )

    #undef RATUI_THEME_METHODS

    private:
        Shared<const Theme> m_Ptr;
        u64 m_Version{ ~0ull }; ///< Cached version of the theme, used to detect changes in the underlying theme.
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
            inline constexpr StringID FocusOutline = "FocusOutline"_id;

            inline constexpr StringID ButtonBorder  = "Button.Border"_id;

            inline constexpr StringID PanelBorder = "Panel.Border"_id;

            inline constexpr StringID SliderTrack        = "Slider.Track"_id;
            inline constexpr StringID SliderTrackFill    = "Slider.TrackFill"_id;
            inline constexpr StringID SliderThumb        = "Slider.Thumb"_id;
            inline constexpr StringID SliderThumbHover   = "Slider.ThumbHover"_id;
            inline constexpr StringID SliderThumbPressed = "Slider.ThumbPressed"_id;
        }

        namespace Rounding
        {
            inline constexpr StringID FocusOutline = "FocusOutline"_id;
            inline constexpr StringID Button       = "Button"_id;
            inline constexpr StringID Panel        = "Panel"_id;
            inline constexpr StringID SliderTrack  = "Slider.Track"_id;
            inline constexpr StringID SliderThumb  = "Slider.Thumb"_id;
        }

        namespace TextStyle
        {
            inline constexpr StringID Default = "Default"_id;
        }

        namespace Metric
        {
			inline constexpr StringID FocusOutlineThickness = "FocusOutlineThickness"_id;
            inline constexpr StringID ButtonBorderThickness = "Button.BorderThickness"_id;
            inline constexpr StringID PanelBorderThickness  = "Panel.BorderThickness"_id;
            inline constexpr StringID SliderTrackThickness  = "Slider.TrackThickness"_id;
            inline constexpr StringID SliderMinThumbSize    = "Slider.MinThumbSize"_id;
        }

		namespace Font 
        {
			inline constexpr StringID Default = "Default"_id;
		}

        namespace Brush
        {
            inline constexpr StringID ButtonNormal  = "Button.Normal"_id;
            inline constexpr StringID ButtonHover   = "Button.Hover"_id;
            inline constexpr StringID ButtonPressed = "Button.Pressed"_id;

            inline constexpr StringID PanelNormal   = "Panel.Normal"_id;
        }
    }

} // namespace RatUI