#ifndef __RNG_H__
#define __RNG_H__

#include "main.h"

typedef struct {
    u64 (*next)(void* self);
} RNG;

static inline u64 rng_next(RNG* self)
{
    return self->next(self);
}

// xoshiro256**
// The state is fully contained in the struct,
// meaning a copy of the struct is also
// a copy of its state.
typedef struct {
    RNG base;
    u64 s[4];
} RNG_XoShiRo256ss;

RNG_XoShiRo256ss rng_xoshiro256ss(u64 seed);

// This is the jump function for the generator. It is equivalent
// to 2^128 calls to next(); it can be used to generate 2^128
// non-overlapping subsequences for parallel computations.
void rng_xoshiro256ss_jump(RNG_XoShiRo256ss* self);

// Generate next pseudorandom number
u64 rng_next(RNG* self);
// Generate u64
u64 rng_u64(RNG* self);
// Generate u64 with exclusive limit
u64 rng_u64_cap(RNG* self, u64 cap);
// Generate i64
i64 rng_i64(RNG* self);
// Generate u64 with exclusive limit
i64 rng_i64_cap(RNG* self, i64 cap);
// Generate f64 between 0 and 1
f64 rng_f64(RNG* self);
// Generate f64 with limit
f64 rng_f64_cap(RNG* self, f64 cap);
// Generate f64 in range
f64 rng_f64_range(RNG* self, f64 min, f64 max);
// Generate bool with p_true as probability of outputting true
bool rng_bool(RNG* self, f64 p_true);
// Generate a gauss distributed f64 with unit variance
// (variance of 1) and mean of 0
f64 rng_gauss(RNG* self);
// Generate a gauss distributed f64 with mean (mu)
// and standard deviation (sigma)
f64 rng_gauss_ex(RNG* self, f64 mu, f64 sigma);

#endif // __RNG_H__
