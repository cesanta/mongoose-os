#ifndef CS_COMMON_PLATFORM_H_
#define CS_COMMON_PLATFORM_H_

/*
 * For the "custom" platform, includes and dependencies can be
 * provided through mg_locals.h.
 */
#define CS_P_CUSTOM 0
#define CS_P_UNIX 1
#define CS_P_WINDOWS 2
#define CS_P_ESP_LWIP 3
#define CS_P_CC3200 4
#define CS_P_MSP432 5
#define CS_P_CC3100 6

/* If not specified explicitly, we guess platform by defines. */
#ifndef CS_PLATFORM

#if defined(TARGET_IS_MSP432P4XX) || defined(__MSP432P401R__)

#define CS_PLATFORM CS_P_MSP432
#elif defined(cc3200)
#define CS_PLATFORM CS_P_CC3200
#elif defined(__unix__) || defined(__APPLE__)
#define CS_PLATFORM CS_P_UNIX
#elif defined(_WIN32)
#define CS_PLATFORM CS_P_WINDOWS
#endif

#ifndef CS_PLATFORM
#error "CS_PLATFORM is not specified and we couldn't guess it."
#endif

#endif /* !defined(CS_PLATFORM) */

#include "common/platforms/platform_unix.h"
#include "common/platforms/platform_windows.h"
#include "common/platforms/platform_esp_lwip.h"
#include "common/platforms/platform_cc3200.h"
#include "common/platforms/platform_cc3100.h"

/* Common stuff */

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#define NOINLINE __attribute__((noinline))
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define NOINSTR __attribute__((no_instrument_function))
#else
#define NORETURN
#define NOINLINE
#define WARN_UNUSED_RESULT
#define NOINSTR
#endif /* __GNUC__ */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

#endif /* CS_COMMON_PLATFORM_H_ */
