#ifndef __MAIN_H__
#define __MAIN_H__

#include <float.h> // FLT_EPSILONG, DBL_EPSILON
#include <inttypes.h> // ptr_t types and format macros
#include <math.h> // M_PI, M_SQRT2
#include <stdbool.h> // bool
#include <stdint.h> // int_t types
#include <stdio.h> // printf
#include <stdlib.h> // malloc, realloc, abort

// Basic macros
#define arrlen(_array) ((usize)(sizeof(_array) / sizeof(_array[0])))
#define max(_x, _y) ((_x) > (_y) ? (_x) : (_y))
#define min(_x, _y) ((_x) < (_y) ? (_x) : (_y))
#define swap(_type, _x, _y) \
    do {                    \
        _type _tmp = _x;    \
        _x = _y;            \
        _y = _tmp;          \
    } while (0)

// Primitive types
typedef float f32;
typedef double f64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef intptr_t isize;
typedef uintptr_t usize;

// Format macros
#define F32 "%g"
#define F64 "%lg"
#define I8 "%" PRIi8
#define I16 "%" PRIi16
#define I32 "%" PRIi32
#define I64 "%" PRIi64
#define U8 "%" PRIu8
#define U16 "%" PRIu16
#define U32 "%" PRIu32
#define U64 "%" PRIu64
#define ISIZE "%" PRIiPTR
#define USIZE "%" PRIuPTR

// Logging
#define log_dbg(...)                                                          \
    do {                                                                      \
        printf("\x1b[34;1mDEBUG\x1b[37;2m %s:%d \x1b[m", __FILE__, __LINE__); \
        printf(__VA_ARGS__);                                                  \
        printf("\n");                                                         \
    } while (0)
#define log_info(...)                                                        \
    do {                                                                     \
        printf("\x1b[32;1mINFO\x1b[37;2m %s:%d \x1b[m", __FILE__, __LINE__); \
        printf(__VA_ARGS__);                                                 \
        printf("\n");                                                        \
    } while (0)
#define log_warn(...)                                                                    \
    do {                                                                                 \
        fprintf(stderr, "\x1b[35;1mWARNING\x1b[37;2m %s:%d \x1b[m", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                                                    \
        fprintf(stderr, "\n");                                                           \
    } while (0)
#define log_err(...)                                                                   \
    do {                                                                               \
        fprintf(stderr, "\x1b[31;1mERROR\x1b[37;2m %s:%d \x1b[m", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                                                  \
        fprintf(stderr, "\n");                                                         \
    } while (0)
#define panic(...)                                                                     \
    do {                                                                               \
        fprintf(stderr, "\x1b[31;1mPANIC\x1b[37;2m %s:%d \x1b[m", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                                                  \
        fprintf(stderr, "\n");                                                         \
        abort();                                                                       \
    } while (0)
#define unreachable() panic("reached code marked as unreachable")

// Maths
#define F32_EPSILON FLT_EPSILON
#define F64_EPSILON DBL_EPSILON
#define PI M_PI
#define TAU (2.0 * PI)
#define PI_2 M_PI_2
#define PI_4 M_PI_4
#define PI_3_2 (3.0 * PI_2)
#define SQRT2 M_SQRT2

#endif // __MAIN_H__
