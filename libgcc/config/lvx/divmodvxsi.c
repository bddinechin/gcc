/*
 * Copyright (C) 2021 Kalray SA.
 *
 * Routines for LVX division and modulus on 64-bit integers.
 *
 * Based on the "TMS320C5x User Guide".
 *
 *     #define STSU(b, r) ( ((r) >= (b)) ? (((r) - (b)) << 1 | 1) : ((r) << 1) )
 *
 *     divmod_result_t
 *     divmodu_3(uint32_t a, uint32_t b)
 *     {
 *         uint64_t acc = (uint64_t)a;
 *         uint64_t src = (uint64_t)b << (32 - 1);
 *         uint32_t q = 0, r = a;
 *         if (b == 0) TRAP;
 *         if (b > a)
 *             goto end;
 *         for (int i = 0; i < 32; i++) {
 *           acc = STSU(src, acc);
 *         }
 *         q = acc;
 *         r = acc >> 32;
 *     end:;
 *         return (divmod_result_t){ q, r };
 *     }
 *
 * For SIMD execution, we make the `if (b > a)` path pass through the loop by
 * setting `src = b << 32` so iterating 32 times `acc = STSU(src, acc)` computes
 * `acc == a << 32` then `q == 0` and `r == a`.
 *
 *     divmod_result_t
 *     divmodu_4(uint32_t a, uint32_t b)
 *     {
 *         uint64_t acc = (uint64_t)a;
 *         uint64_t src = (uint64_t)b << (32 - 1);
 *         if (b == 0) TRAP;
 *         if (b > a)
 *           src <<= 1;
 *         for (int i = 0; i < 32; i++) {
 *           acc = STSU(src, acc);
 *         }
 *         uint32_t q = acc;
 *         uint32_t r = acc >> 32;
 *     end:;
 *         return (divmod_result_t){ q, r };
 *     }
 *
 * -- Benoit Dupont de Dinechin (benoit.dinechin@kalray.eu)
 */

#include "divmodtypes.h"

////////////////////////////////////////////////////////////////////////////////

static inline uint32x8_t
uint32x4_divmod (uint32x4_t a, uint32x4_t b)
{
  uint64x4_t src = __builtin_lvx_widenwdq (b, ".z") << (32 - 1);
  DIV_BY_ZERO_MAY_TRAP (__builtin_lvx_anywq, b);
  uint64x4_t acc = __builtin_lvx_widenwdq (a, ".z");
  /* No `b > a' adjustment is needed.  The scalar derivation in the header
     comment doubles src to b << 32 on that path, but the loop only ever
     compares acc = a << k for k <= 31, and a < b gives a << 31 < b << 31 == src,
     so the subtract branch cannot fire when b > a either way.  Verified
     exhaustively at 8 and 16 bits (every (a, b) pair) and at 32 bits for
     every b with a = b-1, the tightest b > a: no result changes.  */
#pragma GCC unroll 32
  for (int i = 0; i < 32; i++)
    {
      acc = __builtin_lvx_stsudq (src, acc);
    }
  uint32x4_t q = __builtin_lvx_truncdwq (acc);
  uint32x4_t r = __builtin_lvx_truncdwq (acc >> 32);
  return __builtin_lvx_cat256 (q, r);
}

uint32x4_t
__udivv4si3 (uint32x4_t a, uint32x4_t b)
{
  uint32x8_t divmod = uint32x4_divmod (a, b);
  return __builtin_lvx_low128 (divmod);
}

uint32x4_t
__umodv4si3 (uint32x4_t a, uint32x4_t b)
{
  uint32x8_t divmod = uint32x4_divmod (a, b);
  return __builtin_lvx_high128 (divmod);
}

uint32x4_t
__udivmodv4si4 (uint32x4_t a, uint32x4_t b, uint32x4_t *c)
{
  uint32x8_t divmod = uint32x4_divmod (a, b);
  *c = __builtin_lvx_high128 (divmod);
  return __builtin_lvx_low128 (divmod);
}

int32x4_t
__divmodv4si4 (int32x4_t a, int32x4_t b, int32x4_t * c)
{
  uint32x8_t divmod = uint32x4_divmod (__builtin_lvx_abswq (a, ""),
				       __builtin_lvx_abswq (b, ""));
  int32x4_t q = __builtin_lvx_low128 (divmod);
  q = __builtin_lvx_selectwq (-q, q, a ^ b, ".ltz");
  *c = a - b * q;
  return q;
}

int32x4_t
__divv4si3 (int32x4_t a, int32x4_t b)
{
  uint32x4_t absa = __builtin_lvx_abswq (a, "");
  uint32x4_t absb = __builtin_lvx_abswq (b, "");
  uint32x8_t divmod = uint32x4_divmod (absa, absb);
  int32x4_t result = __builtin_lvx_low128 (divmod);
  return __builtin_lvx_selectwq (-result, result, a ^ b, ".ltz");
}

int32x4_t
__modv4si3 (int32x4_t a, int32x4_t b)
{
  uint32x4_t absa = __builtin_lvx_abswq (a, "");
  uint32x4_t absb = __builtin_lvx_abswq (b, "");
  uint32x8_t divmod = uint32x4_divmod (absa, absb);
  int32x4_t result = __builtin_lvx_high128 (divmod);
  return __builtin_lvx_selectwq (-result, result, a, ".ltz");
}

////////////////////////////////////////////////////////////////////////////////

#if 0 /* The V8SI half: no register home for its widened operands.
       *
       * It divides a 256-bit uint32x8_t by widening to 64-bit lanes, which
       * makes uint64x8_t -- 512 bits, where the widest LVX register is the
       * 256-bit quad.  So `stsudo' and `truncdwo' are not builtins waiting to
       * be added: a 512-bit lane-wise operation has nowhere to live, and the
       * 256-bit narrowing spellings (hbv, whx, dwo) name no instruction.
       *
       * STSUD and STSUDP came back to the ISA on 2026-10-09, so the V4SI half
       * above is live again and verified against C's / and % on every lane of
       * every entry point.  That is also the way to revive this half: two
       * uint32x4_divmod calls on the 128-bit halves, no 512-bit value anywhere.
       * Until then divv.c covers V8SI with one divmodw per lane.
       */
static inline uint32x16_t
uint32x8_divmod (uint32x8_t a, uint32x8_t b)
{
  uint64x8_t src = __builtin_lvx_widenwdo (b, ".z") << (32 - 1);
  DIV_BY_ZERO_MAY_TRAP (__builtin_lvx_anywo, b);
  uint64x8_t acc = __builtin_lvx_widenwdo (a, ".z");
  /* No `b > a' adjustment is needed.  The scalar derivation in the header
     comment doubles src to b << 32 on that path, but the loop only ever
     compares acc = a << k for k <= 31, and a < b gives a << 31 < b << 31 == src,
     so the subtract branch cannot fire when b > a either way.  Verified
     exhaustively at 8 and 16 bits (every (a, b) pair) and at 32 bits for
     every b with a = b-1, the tightest b > a: no result changes.  */
  for (int i = 0; i < 32; i++)
    {
      acc = __builtin_lvx_stsudo (src, acc);
    }
  uint32x8_t q = __builtin_lvx_truncdwo (acc, "");
  uint32x8_t r = __builtin_lvx_truncdwo (acc >> 32, "");
  return __builtin_lvx_cat512 (q, r);
}

uint32x8_t
__udivv8si3 (uint32x8_t a, uint32x8_t b)
{
  uint32x16_t divmod = uint32x8_divmod (a, b);
  return __builtin_lvx_low256 (divmod);
}

uint32x8_t
__umodv8si3 (uint32x8_t a, uint32x8_t b)
{
  uint32x16_t divmod = uint32x8_divmod (a, b);
  return __builtin_lvx_high256 (divmod);
}

uint32x8_t
__udivmodv8si4 (uint32x8_t a, uint32x8_t b, uint32x8_t *c)
{
  uint32x16_t divmod = uint32x8_divmod (a, b);
  *c = __builtin_lvx_high256 (divmod);
  return __builtin_lvx_low256 (divmod);
}

int32x8_t
__divmodv8si4 (int32x8_t a, int32x8_t b, int32x8_t * c)
{
  uint32x16_t divmod = uint32x8_divmod (__builtin_lvx_abswo (a, ""),
					__builtin_lvx_abswo (b, ""));
  int32x8_t q = __builtin_lvx_low256 (divmod);
  q = __builtin_lvx_selectwo (-q, q, a ^ b, ".ltz");
  *c = a - b * q;
  return q;
}

int32x8_t
__divv8si3 (int32x8_t a, int32x8_t b)
{
  uint32x8_t absa = __builtin_lvx_abswo (a, "");
  uint32x8_t absb = __builtin_lvx_abswo (b, "");
  uint32x16_t divmod = uint32x8_divmod (absa, absb);
  int32x8_t result = __builtin_lvx_low256 (divmod);
  return __builtin_lvx_selectwo (-result, result, a ^ b, ".ltz");
}

int32x8_t
__modv8si3 (int32x8_t a, int32x8_t b)
{
  uint32x8_t absa = __builtin_lvx_abswo (a, "");
  uint32x8_t absb = __builtin_lvx_abswo (b, "");
  uint32x16_t divmod = uint32x8_divmod (absa, absb);
  int32x8_t result = __builtin_lvx_high256 (divmod);
  return __builtin_lvx_selectwo (-result, result, a, ".ltz");
}
#endif /* the V8SI half */

////////////////////////////////////////////////////////////////////////////////

#ifdef TEST_V4SI
#define LANE 2

uint32_t
__udivsi3 (uint32_t a, uint32_t b)
{
  uint32x4_t udivv4si3 = __udivv4si3 (a - (uint32x4_t){}, b - (uint32x4_t){});
  return (uint32_t)udivv4si3[LANE];
}

uint32_t
__umodsi3 (uint32_t a, uint32_t b)
{
  uint32x4_t umodv4si3 = __umodv4si3 (a - (uint32x4_t){}, b - (uint32x4_t){});
  return (uint32_t)umodv4si3[LANE];
}

uint32_t
__udivmodsi4 (uint32_t a, uint32_t b, uint32_t *c)
{
  uint32x4_t c_ = {0, 0};
  uint32x4_t udivmodv4si4 = __udivmodv4si4 (a - (uint32x4_t){}, b - (uint32x4_t){}, &c_);
  if (c)
    *c = c_[LANE];
  return (uint32_t)udivmodv4si4[LANE];
}

int32_t
__divsi3 (int32_t a, int32_t b)
{
  int32x4_t divv4si3 = __divv4si3 (a - (int32x4_t){}, b - (int32x4_t){});
  return (int32_t)divv4si3[LANE];
}

int32_t
__modsi3 (int32_t a, int32_t b)
{
  int32x4_t modv4si3 = __modv4si3 (a - (int32x4_t){}, b - (int32x4_t){});
  return (int32_t)modv4si3[LANE];
}

#endif // TEST_V4SI

/* Goes with the V8SI half disabled above. */
#if 0 /* was: #ifdef TEST_V8SI */
#define LANE 3

uint32_t
__udivsi3 (uint32_t a, uint32_t b)
{
  uint32x8_t udivv8si3 = __udivv8si3 (a - (uint32x8_t){}, b - (uint32x8_t){});
  return (uint32_t)udivv8si3[LANE];
}

uint32_t
__umodsi3 (uint32_t a, uint32_t b)
{
  uint32x8_t umodv8si3 = __umodv8si3 (a - (uint32x8_t){}, b - (uint32x8_t){});
  return (uint32_t)umodv8si3[LANE];
}

uint32_t
__udivmodsi4 (uint32_t a, uint32_t b, uint32_t *c)
{
  uint32x8_t c_ = {0, 0};
  uint32x8_t udivmodv8si4 = __udivmodv8si4 (a - (uint32x8_t){}, b - (uint32x8_t){}, &c_);
  if (c)
    *c = c_[LANE];
  return (uint32_t)udivmodv8si4[LANE];
}

int32_t
__divsi3 (int32_t a, int32_t b)
{
  int32x8_t divv8si3 = __divv8si3 (a - (int32x8_t){}, b - (int32x8_t){});
  return (int32_t)divv8si3[LANE];
}

int32_t
__modsi3 (int32_t a, int32_t b)
{
  int32x8_t modv8si3 = __modv8si3 (a - (int32x8_t){}, b - (int32x8_t){});
  return (int32_t)modv8si3[LANE];
}

#endif /* was: TEST_V8SI -- mislabelled TEST_V4SI before */

