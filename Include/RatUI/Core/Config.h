#pragma once

/** 
 * @file Config.h
 * @brief This file contains configuration options and macros for RatUI. 
 * Users can modify these options to customize the behavior of RatUI to better fit their needs.
 * This file is included by Core.h and should not be included directly by user code.
 */

// === RatUI Version ===

#define RATUI_VERSION_MAJOR 0
#define RATUI_VERSION_MINOR 0
#define RATUI_VERSION_PATCH 0
#define RATUI_VERSION_STRING "0.0.0"

// === Configuration Options ===

#ifndef RATUI_NODISCARD
    #define RATUI_NODISCARD [[nodiscard]]
#endif

/**
 * @brief Enable or disable assertions in RatUI.
 * When enabled, RatUI will perform runtime checks to ensure the correctness of its operations.
 * This defaults to true, but can be disabled to reduce runtime overhead in release builds.
 */
#ifndef RATUI_ENABLE_ASSERTS
    #define RATUI_ENABLE_ASSERTS true
#endif

/**
 * @brief Enable or disable user assertions in RatUI.
 * User asserts are used for validating user input and usage of the API, and are separate from internal asserts which validate the internal state of RatUI.
 * These assertions are in places where incorrect usage of the API by the user could cause issues, e.g., popping a transform when there are none on the stack, etc.
 * This defaults to true, but can be disabled to reduce runtime overhead in release builds or if the user is confident in their usage of the API and wants to avoid the overhead of these checks.
 * @note This has no effect if RATUI_ENABLE_ASSERTS is false, as user asserts rely on the assertion mechanism to function. 
 *       If RATUI_ENABLE_ASSERTS is false, all asserts including user asserts will be disabled regardless of the value of RATUI_ENABLE_USER_ASSERTS.
 * @see RATUI_ENABLE_ASSERTS
 */
#ifndef RATUI_ENABLE_USER_ASSERTS
    #define RATUI_ENABLE_USER_ASSERTS true
#endif