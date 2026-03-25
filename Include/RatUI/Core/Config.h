#pragma once

// === RatUI Version ===

#define RATUI_VERSION_MAJOR 0
#define RATUI_VERSION_MINOR 0
#define RATUI_VERSION_PATCH 0
#define RATUI_VERSION_STRING "0.0.0"

// === Configuration Options ===

// === StringFormat.inl ===

/**
 * @brief Enable or disable string formatting utilities in RatUI. 
 * When enabled, RatUI provides std::format-based string formatting functions that can be used for constructing formatted strings.
 * This defaults to true, but can be disabled to reduce compile times and binary size if string formatting is not needed in the project.
 */
#ifndef RATUI_CONFIG_ENABLE_STRING_FORMATTERS
    #define RATUI_CONFIG_ENABLE_STRING_FORMATTERS true
#endif