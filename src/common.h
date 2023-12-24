#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>    // include type: bool
#include <stddef.h>     // include type: NULL & size_t
#include <stdint.h>     // include type: uint8_t

// #define CLOX_DEBUG_TRACE_EXECUTION  // this macro will trace all instructions while executing
// #define CLOX_DEBUG_DISASSEMBLE      // this macro will print the disassembled bytecode
#define CLOX_DEBUG_STRESS_GC           // this macro will force the garbage collector to run on every allocation
// #define CLOX_DEBUG_LOG_GC              // this macro will print the garbage collector's status

#define NAN_BOXING                     // this macro will enable NaN-boxing

#endif // clox_common_h