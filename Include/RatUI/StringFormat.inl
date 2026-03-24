#pragma once
#include "RatUI.h"

/**
 * @file StringFormat.inl
 * @brief This file contains string formatting utilities for RatUI, including std::format-based formatting functions.
 * It is included by RatUI.h and should not be included directly by user code.
 */

#if RATUI_CONFIG_ENABLE_STRING_FORMATTERS

#include <format>

// std::formatter specializations must live outside namespace RatUI.

// === Math.inl ===
template<typename T, unsigned Dim>
struct std::formatter<RatUI::Vec<T, Dim>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin(); // no custom format options
    }

    template<typename FormatContext>
	auto format( const RatUI::Vec<T, Dim>& value, FormatContext& ctx ) const
    {
        auto out = ctx.out();
        *out++ = '(';
        for (unsigned i = 0; i < Dim; ++i)
        {
            if (i > 0) { *out++ = ','; *out++ = ' '; }
            out = std::format_to(out, "{}", value[i]);
        }
        *out++ = ')';
        return out;
    }
};

// === Layout.h ===

template<>
struct std::formatter<RatUI::EAlignment>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin(); // no custom format options
    }

    template<typename FormatContext>
    auto format(RatUI::EAlignment value, FormatContext& ctx) const
    {
        using namespace RatUI;
        using enum EAlignment;
    
        auto out = ctx.out();
    
        // ---- Fast path: exact presets (most common in UI) ----
        switch (value)
        {
            case TopLeft:               return std::format_to(out, "TopLeft");
            case TopCenter:             return std::format_to(out, "TopCenter");
            case TopRight:              return std::format_to(out, "TopRight");
            case CenterLeft:            return std::format_to(out, "CenterLeft");
            case Center:                return std::format_to(out, "Center");
            case CenterRight:           return std::format_to(out, "CenterRight");
            case BottomLeft:            return std::format_to(out, "BottomLeft");
            case BottomCenter:          return std::format_to(out, "BottomCenter");
            case BottomRight:           return std::format_to(out, "BottomRight");
            case static_cast<EAlignment>(0): return std::format_to(out, "None");
            default:                         break;
        }
    
        // ---- Slow path: bit decomposition (still zero alloc) ----
    
        auto append = [&](RatUI::StringView s, bool& first)
        {
            if (!first)
            {
                *out++ = ' ';
                *out++ = '|';
                *out++ = ' ';
            }
            first = false;
            out = std::copy(s.begin(), s.end(), out);
        };
    
        bool first = true;
    
        const RatUI::u16 v = static_cast<RatUI::u16>(value);
    
        // Horizontal (mutually exclusive in valid cases)
        if (v & Left)      append("Left", first);
        if (v & HCenter)   append("HCenter", first);
        if (v & Right)     append("Right", first);
    
        // Vertical
        if (v & Top)       append("Top", first);
        if (v & VCenter)   append("VCenter", first);
        if (v & Bottom)    append("Bottom", first);
    
        if (first) // nothing written
        {
            return std::format_to(out, "None");
        }
    
        return out;
    }
};

template<>
struct std::formatter<RatUI::ELayoutType>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin(); // no custom format options
    }

    template<typename FormatContext>
    auto format(RatUI::ELayoutType value, FormatContext& ctx) const
    {
        using namespace RatUI;
        using enum ELayoutType;
    
        switch (value)
        {
            case Horizontal: return std::format_to(ctx.out(), "Horizontal");
            case Vertical:   return std::format_to(ctx.out(), "Vertical");
            case Overlay:    return std::format_to(ctx.out(), "Overlay");
            case Grid:       return std::format_to(ctx.out(), "Grid");
            default:         return std::format_to(ctx.out(), "Unknown");
        }
    }
};

template<>
struct std::formatter<RatUI::Constraints>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin(); // no custom format options
    }

    template<typename FormatContext>
    auto format(const RatUI::Constraints& value, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "Constraints(Min: {}, Max: {})", value.MinSize, value.MaxSize);
    }
};

// === Units.inl ===

template<typename T>
struct std::formatter<RatUI::Radians<T>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin(); // no custom format options
    }

    template<typename FormatContext>
    auto format(const RatUI::Radians<T>& value, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "{} radians", value.Value);
    }
};

template<typename T>
struct std::formatter<RatUI::Degrees<T>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin(); // no custom format options
    }

    template<typename FormatContext>
    auto format(const RatUI::Degrees<T>& value, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "{} degrees", value.Value);
    }
};

#endif // RATUI_CONFIG_ENABLE_STRING_FORMATTERS
