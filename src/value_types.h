#ifndef VALUE_TYPES_H
#define VALUE_TYPES_H

/*
 * Defines the numeric types used to represent literal values in the
 * Turbine scripting language, shared across the parser, compiler, and VM.
 *
 * These types (value_int_t, value_float_t) are distinct from internal
 * bookkeeping types, and reflect the semantics of language-level values.
 *
 * Also provides portable printf macros (e.g., PRIival, PRIfval) for I/O.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

typedef int64_t value_int_t;
typedef double  value_float_t;

#define PRIival PRId64
#define PRIfval "f"

#endif /* _H */
