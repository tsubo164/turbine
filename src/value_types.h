#ifndef VALUE_TYPES_H
#define VALUE_TYPES_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

/* Represents an integer value used consistently across the parser, compiler, and VM.
 * This is the canonical integer type for Turbine script execution. */
typedef int64_t value_int_t;

/* Represents a floating-point value used consistently across the parser, compiler, and VM.
 * This is the canonical floating-point type for Turbine script execution. */
typedef double value_float_t;

/* Represents a logical address within the VM's value space.
 * Used to locate values in the virtual memory, not necessarily corresponding to physical memory. */
typedef int64_t value_addr_t;

/* Represents an index into value arrays or VM-managed containers.
 * Typically used for referencing positions in memory blocks or sequences. */
typedef int64_t value_index_t;

/* Represents the size of memory regions or container lengths in the VM.
 * Used for measuring quantities such as the number of values or memory slots. */
typedef int64_t value_size_t;

#define PRIival PRId64
#define PRIfval "f"

#define PRIaddr PRId64
#define PRIindex PRId64

#endif /* _H */
