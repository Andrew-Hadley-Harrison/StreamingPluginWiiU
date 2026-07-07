#pragma once
// Self-contained logging shim (replaces the old libutils <utils/logger.h>).
//
// The plugin never defines __LOGGING__, so all logging was already compiled
// out. We keep the macros/functions as no-ops so we can drop the old libutils
// dependency entirely (its precompiled .a was built against an old wut ABI and
// is the source of the "possibly_broken_reent" flag / boot hang).

#ifdef __cplusplus
extern "C" {
#endif

static inline void log_init(void) {}
static inline void log_deinit(void) {}

#ifdef __cplusplus
}
#endif

#define DEBUG_FUNCTION_LINE(...)         ((void)0)
#define DEBUG_FUNCTION_LINE_VERBOSE(...) ((void)0)
