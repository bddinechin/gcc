/*
 * Copyright (C) 2021 Kalray SA.
 *
 * Routines for LVX division and modulus on 16-bit integers.
 *
 * Based on the "TMS320C5x User Guide".
 *
 *     #define STSU(b, r) ( ((r) >= (b)) ? (((r) - (b)) << 1 | 1) : ((r) << 1) )
 *
 *     divmod_result_t
 *     divmodu_3(uint16_t a, uint16_t b)
 *     {
 *         uint32_t acc = (uint32_t)a;
 *         uint32_t src = (uint32_t)b << (16 - 1);
 *         uint16_t q = 0, r = a;
 *         if (b == 0) TRAP;
 *         if (b > a)
 *             goto end;
 *         for (int i = 0; i < 16; i++) {
 *           acc = STSU(src, acc);
 *         }
 *         q = acc;
 *         r = acc >> 16;
 *     end:;
 *         return (divmod_result_t){ q, r };
 *     }
 *
 * For SIMD execution, we make the `if (b > a)` path pass through the loop by
 * setting `src = b << 16` so iterating 16 times `acc = STSU(src, acc)` computes
 * `acc == a << 16` then `q == 0` and `r == a`.
 *
 *     divmod_result_t
 *     divmodu_4(uint16_t a, uint16_t b)
 *     {
 *         uint32_t acc = (uint32_t)a;
 *         uint32_t src = (uint32_t)b << (16 - 1);
 *         if (b == 0) TRAP;
 *         if (b > a)
 *           src <<= 1;
 *         for (int i = 0; i < 16; i++) {
 *           acc = STSU(src, acc);
 *         }
 *         uint16_t q = acc;
 *         uint16_t r = acc >> 16;
 *     end:;
 *         return (divmod_result_t){ q, r };
 *     }
 *
 * -- Benoit Dupont de Dinechin (benoit.dinechin@kalray.eu)
 */

#include "divmodtypes.h"

/* SIMD and the STSU family are lvx-2 only.  */
#if defined(__lvxarch_lvx_2)

////////////////////////////////////////////////////////////////////////////////

static inline uint16x16_t
uint16x8_divmod (uint16x8_t a, uint16x8_t b)
{
  uint32x8_t src = __builtin_lvx_widenhwo (b, ".z") << (16 - 1);
  uint32x8_t wb = __builtin_lvx_widenhwo (b, ".z");
  /* Division by zero returns zero rather than trapping -- see
     divmodtypes.h; LVX has no divide-by-zero trap to raise here.  */
  uint32x8_t acc = __builtin_lvx_widenhwo (a, ".z");
  // As `src == b << (16 -1)` adding src yields `src == b << 16`.
  src += src & (wb > acc);
#pragma GCC unroll 16
  for (int i = 0; i < 16; i++)
    {
      acc = __builtin_lvx_stsuwo (src, acc);
    }
  uint16x8_t q = __builtin_lvx_narrowwho (acc);
  uint16x8_t r = __builtin_lvx_narrowwho (acc >> 16);
  return __builtin_lvx_cat256 (q, r);
}

uint16x8_t
__udivv8hi3 (uint16x8_t a, uint16x8_t b)
{
  uint16x16_t divmod = uint16x8_divmod (a, b);
  return __builtin_lvx_low128 (divmod);
}

uint16x8_t
__umodv8hi3 (uint16x8_t a, uint16x8_t b)
{
  uint16x16_t divmod = uint16x8_divmod (a, b);
  return __builtin_lvx_high128 (divmod);
}

uint16x8_t
__udivmodv8hi4 (uint16x8_t a, uint16x8_t b, uint16x8_t *c)
{
  uint16x16_t divmod = uint16x8_divmod (a, b);
  *c = __builtin_lvx_high128 (divmod);
  return __builtin_lvx_low128 (divmod);
}

int16x8_t
__divmodv8hi4 (int16x8_t a, int16x8_t b, int16x8_t * c)
{
  uint16x16_t divmod = uint16x8_divmod (__builtin_lvx_absho (a, ""),
					__builtin_lvx_absho (b, ""));
  int16x8_t q = __builtin_lvx_low128 (divmod);
  q = __builtin_lvx_selectho (-q, q, a ^ b, ".ltz");
  *c = a - q * b;
  return q;
}

int16x8_t
__divv8hi3 (int16x8_t a, int16x8_t b)
{
  uint16x8_t absa = __builtin_lvx_absho (a, "");
  uint16x8_t absb = __builtin_lvx_absho (b, "");
  uint16x16_t divmod = uint16x8_divmod (absa, absb);
  int16x8_t result = __builtin_lvx_low128 (divmod);
  return __builtin_lvx_selectho (-result, result, a ^ b, ".ltz");
}

int16x8_t
__modv8hi3 (int16x8_t a, int16x8_t b)
{
  uint16x8_t absa = __builtin_lvx_absho (a, "");
  uint16x8_t absb = __builtin_lvx_absho (b, "");
  uint16x16_t divmod = uint16x8_divmod (absa, absb);
  int16x8_t result = __builtin_lvx_high128 (divmod);
  return __builtin_lvx_selectho (-result, result, a, ".ltz");
}

////////////////////////////////////////////////////////////////////////////////

/* A 256-bit divide is two 128-bit ones.  Each iteration needs an STSU at twice
   the lane width, every STSU is 128 bits wide, and a 128-bit divide already
   spends two of them per iteration -- so widening a 256-bit operand as a whole
   would make a 512-bit value, which has no register home and spills through
   memory.  Splitting first keeps every intermediate in a register quad.  */

uint16x16_t
__udivv16hi3 (uint16x16_t a, uint16x16_t b)
{
  return __builtin_lvx_cat256 (__udivv8hi3 (__builtin_lvx_low128 (a),
                                               __builtin_lvx_low128 (b)),
                               __udivv8hi3 (__builtin_lvx_high128 (a),
                                               __builtin_lvx_high128 (b)));
}

uint16x16_t
__umodv16hi3 (uint16x16_t a, uint16x16_t b)
{
  return __builtin_lvx_cat256 (__umodv8hi3 (__builtin_lvx_low128 (a),
                                               __builtin_lvx_low128 (b)),
                               __umodv8hi3 (__builtin_lvx_high128 (a),
                                               __builtin_lvx_high128 (b)));
}

int16x16_t
__divv16hi3 (int16x16_t a, int16x16_t b)
{
  return __builtin_lvx_cat256 (__divv8hi3 (__builtin_lvx_low128 (a),
                                               __builtin_lvx_low128 (b)),
                               __divv8hi3 (__builtin_lvx_high128 (a),
                                               __builtin_lvx_high128 (b)));
}

int16x16_t
__modv16hi3 (int16x16_t a, int16x16_t b)
{
  return __builtin_lvx_cat256 (__modv8hi3 (__builtin_lvx_low128 (a),
                                               __builtin_lvx_low128 (b)),
                               __modv8hi3 (__builtin_lvx_high128 (a),
                                               __builtin_lvx_high128 (b)));
}

uint16x16_t
__udivmodv16hi4 (uint16x16_t a, uint16x16_t b, uint16x16_t *c)
{
  uint16x8_t clo, chi;
  uint16x8_t qlo = __udivmodv8hi4 (__builtin_lvx_low128 (a),
				   __builtin_lvx_low128 (b), &clo);
  uint16x8_t qhi = __udivmodv8hi4 (__builtin_lvx_high128 (a),
				   __builtin_lvx_high128 (b), &chi);
  *c = __builtin_lvx_cat256 (clo, chi);
  return __builtin_lvx_cat256 (qlo, qhi);
}

int16x16_t
__divmodv16hi4 (int16x16_t a, int16x16_t b, int16x16_t *c)
{
  int16x8_t clo, chi;
  int16x8_t qlo = __divmodv8hi4 (__builtin_lvx_low128 (a),
				 __builtin_lvx_low128 (b), &clo);
  int16x8_t qhi = __divmodv8hi4 (__builtin_lvx_high128 (a),
				 __builtin_lvx_high128 (b), &chi);
  *c = __builtin_lvx_cat256 (clo, chi);
  return __builtin_lvx_cat256 (qlo, qhi);
}

#endif//__lvxarch_lvx_2
