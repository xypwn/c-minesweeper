#include "rng.h"

// xoshiro256** 1.0,
// derived from David Blackman and Sebastiano Vigna's public domain implmentation
// <https://prng.di.unimi.it/>
static inline u64 rotl(const u64 x, i32 k)
{
    return (x << k) | (x >> (64 - k));
}

static u64 xoshiro256ss_next(void* _self)
{
    RNG_XoShiRo256ss* self = _self;
    const u64 result = rotl(self->s[1] * 5, 7) * 9;

    const u64 t = self->s[1] << 17;

    self->s[2] ^= self->s[0];
    self->s[3] ^= self->s[1];
    self->s[1] ^= self->s[2];
    self->s[0] ^= self->s[3];

    self->s[2] ^= t;

    self->s[3] = rotl(self->s[3], 45);

    return result;
}

RNG_XoShiRo256ss rng_xoshiro256ss(u64 seed)
{
    RNG_XoShiRo256ss res = {
        .base = {
            .next = xoshiro256ss_next,
        }
    };
    u64 x = seed;
    for (usize i = 0; i < arrlen(res.s); i++) {
        // splitmix64 to fill seed array, derived from
        // Sebastiano Vigna's public domain implementation
        // <https://prng.di.unimi.it/splitmix64.c>
        u64 z = (x += (u64)0x9E3779B97F4A7C15);
        z = (z ^ (z >> 30)) * (u64)0xBF58476D1CE4E5B9;
        z = (z ^ (z >> 27)) * (u64)0x94D049BB133111EB;
        res.s[i] = z ^ (z >> 31);
    }
    return res;
}

void rng_xoshiro256ss_jump(RNG_XoShiRo256ss* self)
{
    static const u64 JUMP[] = {
        0x180ec6d33cfd0aba,
        0xd5a61266f0c9392c,
        0xa9582618e03fc9aa,
        0x39abdc4529b1661c,
    };

    u64 s0 = 0;
    u64 s1 = 0;
    u64 s2 = 0;
    u64 s3 = 0;
    for (usize i = 0; i < arrlen(JUMP); i++) {
        for (usize b = 0; b < 64; b++) {
            if (JUMP[i] & (u64)1 << b) {
                s0 ^= self->s[0];
                s1 ^= self->s[1];
                s2 ^= self->s[2];
                s3 ^= self->s[3];
            }
            rng_next((RNG*)self);
        }
    }

    self->s[0] = s0;
    self->s[1] = s1;
    self->s[2] = s2;
    self->s[3] = s3;
}

u64 rng_u64(RNG* self)
{
    return rng_next(self);
}

u64 rng_u64_cap(RNG* self, u64 cap)
{
    // Bitmask with rejection
    // <https://www.pcg-random.org/posts/bounded-rands.html>
    u64 mask = ~(u64)0;
    cap--;
    mask >>= __builtin_clzll(cap | 1);
    u64 x;
    do {
        x = rng_next(self) & mask;
    } while (x > cap);
    return x;
}

i64 rng_i64(RNG* self)
{
    u64 x = rng_next(self);
    return *((i64*)&x);
}

i64 rng_i64_cap(RNG* self, i64 cap)
{
    return (i64)rng_u64_cap(self, cap);
}

f64 rng_f64(RNG* self)
{
    u64 x = rng_next(self);
    union {
        uint64_t i;
        double d;
    } u = { .i = (u64)0x3FF << 52 | x >> 12 };
    return u.d - 1.0;
}

f64 rng_f64_cap(RNG* self, f64 cap)
{
    return rng_f64(self) * cap;
}

f64 rng_f64_range(RNG* self, f64 min, f64 max)
{
    return min + rng_f64_cap(self, max - min);
}

bool rng_bool(RNG* self, f64 p_true)
{
    return rng_f64(self) < p_true;
}

f64 rng_gauss(RNG* self)
{
    // Box Mueller transform
    // <https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform>
    f64 u;
    do {
        u = rng_f64(self);
    } while (u <= F64_EPSILON);
    f64 v = rng_f64(self);
    return sqrt(-2.0 * log(u)) * cos(TAU * v);
}

f64 rng_gauss_ex(RNG* self, f64 mu, f64 sigma)
{
    return rng_gauss(self) * sigma + mu;
}
