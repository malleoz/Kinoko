#pragma once

/// @file Logger.hh
/// @brief Provides logging macros for assertions, warnings, reports, and debug messages.

#include <cstdio>
#include <cstdlib>

/// @brief Converts a macro argument to a string literal.
#define TOSTRINGIMPL(x) #x

/// @brief Converts a macro argument to a string literal after macro expansion.
#define TOSTRING(x) TOSTRINGIMPL(x)

/// @brief Macro that expands to the current file name
#ifdef __FILE_NAME__
#define FILE_NAME __FILE_NAME__
#else
#define FILE_NAME __FILE__
#endif

/// @brief Static assert that displays the file name and line number in the assertion message
#define STATIC_ASSERT(e) static_assert(e, "[" FILE_NAME ":" TOSTRING(__LINE__) "] ASSERT: " #e)

/// @brief Runtime assert that displays the file name and line number in the assertion message
#define RUNTIME_ASSERT(e) \
    do { \
        if (!(e)) { \
            printf("[" FILE_NAME ":" TOSTRING(__LINE__) "] ASSERT: " #e "\n"); \
            abort(); \
        } \
    } while (0)

/// @brief Runtime assert that is used as the default assertion mechanism
#define ASSERT(e) RUNTIME_ASSERT(e)

/// @brief Logs a panic message and aborts the program
#define PANIC(m, ...) \
    do { \
        printf("[" FILE_NAME ":" TOSTRING(__LINE__) "] PANIC: " m "\n", ##__VA_ARGS__); \
        abort(); \
    } while (0)

/// @brief Logs a warning message without aborting the program
#define WARN(m, ...) \
    do { \
        printf("[" FILE_NAME ":" TOSTRING(__LINE__) "] WARN: " m "\n\n", ##__VA_ARGS__); \
    } while (0)

/// @brief Logs a report message for informational purposes
#define REPORT(m, ...) \
    do { \
        printf("[" FILE_NAME ":" TOSTRING(__LINE__) "] REPORT: " m "\n", ##__VA_ARGS__); \
    } while (0)

/// @brief Logs a debug message for development purposes
#ifdef BUILD_DEBUG
#define DEBUG(m, ...) \
    do { \
        printf("[" FILE_NAME ":" TOSTRING(__LINE__) "] DEBUG: " m "\n", ##__VA_ARGS__); \
    } while (0)
#else
#define DEBUG(m, ...)
#endif
