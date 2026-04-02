#pragma once
#include "../Core.h"

#ifndef RATUI_OVERRIDE_STRING_IMPL
    #include <string>

    namespace RatUI
    {
        using StringImpl = std::string;

        template<>
        struct CoreTraits<StringImpl>
        {
            using Type        = StringImpl;
            using CharType    = typename Type::value_type;
            using CharTraits  = typename Type::traits_type;
            using ElementType = CharType;
            using SizeType    = typename Type::size_type;
            using Iter        = typename Type::iterator;
            using ConstIter   = typename Type::const_iterator;
            using RIter       = typename Type::reverse_iterator;
            using ConstRIter  = typename Type::const_reverse_iterator;
 
            static SizeType           Size(const Type& a_String) { return a_String.size(); }
            static bool               Empty(const Type& a_String) { return a_String.empty(); }
            static const CharType*    CStr(const Type& a_String) { return a_String.c_str(); }
            static void               Clear(Type& a_String) { a_String.clear(); }
            static void               Reserve(Type& a_String, SizeType a_Count) { a_String.reserve(a_Count); }
            static void               Resize(Type& a_String, SizeType a_Count) { a_String.resize(a_Count); }
            static CharType&          Front(Type& a_String) { return a_String.front(); }
            static const CharType&    Front(const Type& a_String) { return a_String.front(); }
            static CharType&          Back(Type& a_String) { return a_String.back(); }
            static const CharType&    Back(const Type& a_String) { return a_String.back(); }
            static CharType&          RawAt(Type& a_String, SizeType a_Index) { return a_String[a_Index]; }
            static const CharType&    RawAt(const Type& a_String, SizeType a_Index) { return a_String[a_Index]; }
            static CharType&          At(Type& a_String, SizeType a_Index) { return a_String.at(a_Index); }
            static const CharType&    At(const Type& a_String, SizeType a_Index) { return a_String.at(a_Index); }
            static CharType*          Data(Type& a_String) { return a_String.data(); }
            static const CharType*    Data(const Type& a_String) { return a_String.data(); }
            static void               PushBack(Type& a_String, const CharType& a_Char) { a_String.push_back(a_Char); }
            static void               PushBack(Type& a_String, CharType&& a_Char) { a_String.push_back(std::move(a_Char)); }
            static void               PushBack(Type& a_String, const Type& a_Other) { a_String.append(a_Other); }
            static void               PushBack(Type& a_String, Type&& a_Other) { a_String.append(std::move(a_Other)); }
            static void               PopBack(Type& a_String) { a_String.pop_back(); }

            // - Iterators

            static Iter       Begin(Type& a_String) { return a_String.begin(); }
            static ConstIter  Begin(const Type& a_String) { return a_String.begin(); }
            static Iter       End(Type& a_String) { return a_String.end(); }
            static ConstIter  End(const Type& a_String) { return a_String.end(); }
            static RIter      RBegin(Type& a_String) { return a_String.rbegin(); }
            static ConstRIter RBegin(const Type& a_String) { return a_String.rbegin(); }
            static RIter      REnd(Type& a_String) { return a_String.rend(); }
            static ConstRIter REnd(const Type& a_String) { return a_String.rend(); }

        };
    }

#endif // Default to std::string if no custom string implementation is provided.

#ifndef RATUI_STRING_VIEW_IMPL
    #include <string_view>

    namespace RatUI
    {
        using StringViewImpl = std::string_view;

        template<>
        struct CoreTraits<StringViewImpl>
        {
            using Type        = StringViewImpl;
            using CharType    = typename Type::value_type;
            using CharTraits  = typename Type::traits_type;
            using ElementType = CharType;
            using SizeType    = typename Type::size_type;
            using Iter        = typename Type::iterator;
            using ConstIter   = typename Type::const_iterator;
            using RIter       = typename Type::reverse_iterator;
            using ConstRIter  = typename Type::const_reverse_iterator;

            static SizeType           Size(const Type& a_StringView) { return a_StringView.size(); }
            static bool               Empty(const Type& a_StringView) { return a_StringView.empty(); }
            static const CharType&    Front(const Type& a_StringView) { return a_StringView.front(); }
            static const CharType&    Back(const Type& a_StringView) { return a_StringView.back(); }
            static const CharType&    RawAt(const Type& a_StringView, SizeType a_Index) { return a_StringView[a_Index]; }
            static const CharType&    At(const Type& a_StringView, SizeType a_Index) { return a_StringView.at(a_Index); }
            static const CharType*    Data(const Type& a_StringView) { return a_StringView.data(); }

            // - Iterators

            static ConstIter          Begin(const Type& a_StringView) { return a_StringView.begin(); }
            static ConstIter          End(const Type& a_StringView) { return a_StringView.end(); }
            static ConstRIter         RBegin(const Type& a_StringView) { return a_StringView.rbegin(); }
            static ConstRIter         REnd(const Type& a_StringView) { return a_StringView.rend(); }
        };
    }

#endif // Default to std::string_view if no custom string view implementation is provided.

namespace RatUI
{ 
    /**
     * @brief String is a dynamic array of characters
     */
    using String = StringImpl;

    /**
     * @brief StringView is a non-owning view over a string, useful for read-only access to string data without copying.
     */
    using StringView = StringViewImpl;

    /** 
     * @brief CStr returns a pointer to the underlying character array of the string, suitable for interop with C-style APIs.
     * @note The string must be null-terminated.
     */
    inline const char* CStr( const String& a_String ) { return CoreTraits<String>::CStr( a_String ); }

} // namespace RatUI


