#ifndef GRR_TYPES_H
#define GRR_TYPES_H

#include <stdbool.h>
#include <stdint.h>

// Boolean type
typedef bool Grr_bool;

// Integer types
typedef uint8_t Grr_byte;

typedef uint16_t Grr_u16;
typedef int16_t Grr_i16;

typedef uint32_t Grr_u32;
typedef int32_t Grr_i32;

typedef uint64_t Grr_u64;
typedef int64_t Grr_i64;

// Floating point types
typedef float Grr_f32;
typedef double Grr_f64;

// String type
typedef char *Grr_string;

#endif