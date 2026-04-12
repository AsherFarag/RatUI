#pragma once

/**
 * @file Config.h
 * @brief Configuration macros for the FreeType backend.
 */

/**
 * @brief Define RATUI_FREETYPE_WITH_ICU to 1 to enable FreeType's optional ICU integration for advanced Unicode support 
 * (e.g., complex script shaping, accurate case mapping). 
 * This requires linking against the ICU library and may increase binary size.
 * If not defined or set to 0, FreeType will use its built-in fallback implementations, which may have limitations in certain languages or scripts.
 * For example, without ICU, text transformations like uppercase/lowercase may not work correctly for non-ASCII characters, 
 * and shaping of complex scripts (e.g., Arabic, Indic) may be less accurate.
 */
#ifndef RATUI_FREETYPE_WITH_ICU
#   define RATUI_FREETYPE_WITH_ICU 0
#endif // !defined( RATUI_FREETYPE_WITH_ICU )