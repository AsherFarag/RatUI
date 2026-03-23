#pragma once

// === RatUI Version ===

#define RATUI_VERSION_MAJOR 0
#define RATUI_VERSION_MINOR 0
#define RATUI_VERSION_PATCH 0
#define RATUI_VERSION_STRING "0.0.0"

// === Configuration Options ===

// === IWidget.h ===

/**
 * @brief The size of the small buffer used for storing arranged child widgets inline.
 * This is an optimization to avoid heap allocations for common cases where a widget has only a few children.
 * If your applications have widgets with many children, consider increasing this value.
 */
#ifndef RATUI_CONFIG_CHILDREN_SBO_SIZE
    #define RATUI_CONFIG_CHILDREN_SBO_SIZE 4
#endif

// === StringFormat.inl ===

/**
 * @brief Enable or disable string formatting utilities in RatUI. 
 * When enabled, RatUI provides std::format-based string formatting functions that can be used for constructing formatted strings.
 * This defaults to true, but can be disabled to reduce compile times and binary size if string formatting is not needed in the project.
 */
#ifndef RATUI_CONFIG_ENABLE_STRING_FORMATTERS
    #define RATUI_CONFIG_ENABLE_STRING_FORMATTERS true
#endif