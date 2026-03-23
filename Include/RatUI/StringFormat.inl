#pragma once
#include "RatUI.h"

/**
 * @file StringFormat.inl
 * @brief This file contains string formatting utilities for RatUI, including std::format-based formatting functions.
 * It is included by RatUI.h and should not be included directly by user code.
 */

#if RATUI_CONFIG_ENABLE_STRING_FORMATTERS

#include <format>

// std::formatter specializations must live in the std namespace (or global
// namespace via ADL).  They are NOT placed inside namespace RatUI.

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
            case AlignTopLeft:               return std::format_to(out, "AlignTopLeft");
            case AlignTopCenter:             return std::format_to(out, "AlignTopCenter");
            case AlignTopRight:              return std::format_to(out, "AlignTopRight");
            case AlignCenterLeft:            return std::format_to(out, "AlignCenterLeft");
            case AlignCenter:                return std::format_to(out, "AlignCenter");
            case AlignCenterRight:           return std::format_to(out, "AlignCenterRight");
            case AlignBottomLeft:            return std::format_to(out, "AlignBottomLeft");
            case AlignBottomCenter:          return std::format_to(out, "AlignBottomCenter");
            case AlignBottomRight:           return std::format_to(out, "AlignBottomRight");
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
        if (v & AlignLeft)      append("AlignLeft", first);
        if (v & AlignHCenter)   append("AlignHCenter", first);
        if (v & AlignRight)     append("AlignRight", first);
    
        // Vertical
        if (v & AlignTop)       append("AlignTop", first);
        if (v & AlignVCenter)   append("AlignVCenter", first);
        if (v & AlignBottom)    append("AlignBottom", first);
    
        if (first) // nothing written
        {
            return std::format_to(out, "None");
        }
    
        return out;
    }
};

template<>
struct std::formatter<RatUI::ELayoutDirection>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin(); // no custom format options
    }

    template<typename FormatContext>
    auto format(RatUI::ELayoutDirection value, FormatContext& ctx) const
    {
        using enum RatUI::ELayoutDirection;
    
        switch (value)
        {
            case Horizontal: return std::format_to(ctx.out(), "Horizontal");
            case Vertical:   return std::format_to(ctx.out(), "Vertical");
            case Stack:      return std::format_to(ctx.out(), "Stack");
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

#endif // RATUI_CONFIG_ENABLE_STRING_FORMATTERS