/*
 * Copyright (C) 2021 Kalray SA.
 *
 * Routines for LVX division and modulus on 8-bit integers.
 *
 * Based on the "TMS320C5x User Guide".
 *
 *     #define STSU(b, r) ( ((r) >= (b)) ? (((r) - (b)) << 1 | 1) : ((r) << 1) )
 *
 *     divmod_result_t
 *     divmodu_3(uint8_t a, uint8_t b)
 *     {
 *         uint8_t acc = (uint8_t)a;
 *         uint8_t src = (uint8_t)b << (8 - 1);
 *         uint8_t q = 0, r = a;
 *         if (b == 0) TRAP;
 *         if (b > a)
 *             goto end;
 *         for (int i = 0; i < 8; i++) {
 *           acc = STSU(src, acc);
 *         }
 *         q = acc;
 *         r = acc >> 8;
 *     end:;
 *         return (divmod_result_t){ q, r };
 *     }
 *
 * For SIMD execution, we make the `if (b > a)` path pass through the loop by
 * setting `src = b << 8` so iterating 8 times `acc = STSU(src, acc)` computes
 * `acc == a << 8` then `q == 0` and `r == a`.
 *
 *     divmod_result_t
 *     divmodu_4(uint8_t a, uint8_t b)
 *     {
 *         uint8_t acc = (uint8_t)a;
 *         uint8_t src = (uint8_t)b << (8 - 1);
 *         if (b == 0) TRAP;
 *         if (b > a)
 *           src <<= 1;
 *         for (int i = 0; i < 8; i++) {
 *           acc = STSU(src, acc);
 *         }
 *         uint8_t q = acc;
 *         uint8_t r = acc >> 8;
 *     end:;
 *         return (divmod_result_t){ q, r };
 *     }
 *
 * -- Benoit Dupont de Dinechin (benoit.dinechin@kalray.eu)
 */

#include "divmodtypes.h"

#if defined(__lvxarch_lvx_1)
////////////////////////////////////////////////////////////////////////////////

static inline uint8x32_t
uint8x16_divmod (uint8x16_t a, uint8x16_t b)
{
  uint16x16_t src = __builtin_lvx_widenbhx (b, ".z") << (8 - 1);
  uint16x16_t wb = __builtin_lvx_widenbhx (b, ".z");
  DIV_BY_ZERO_MAY_TRAP (__builtin_lvx_anybx, b);
  uint16x16_t acc = __builtin_lvx_widenbhx (a, ".z");
  // As `src == b << (8 -1)` adding src yields `src == b << 8`.
  src += src & (wb > acc);
#pragma GCC unroll 8
  for (int i = 0; i < 8; i++)
    {
      acc = __builtin_lvx_stsuhx (src, acc);
    }
  uint8x16_t q = __builtin_lvx_narrowhbx (acc, "");
  uint8x16_t r = __builtin_lvx_narrowhbx (acc >> 8, "");
  return __builtin_lvx_cat256 (q, r);
}

uint8x16_t
__udivv16qi3 (uint8x16_t a, uint8x16_t b)
{
  uint8x32_t divmod = uint8x16_divmod (a, b);
  return __builtin_lvx_low128 (divmod);
}

uint8x16_t
__umodv16qi3 (uint8x16_t a, uint8x16_t b)
{
  uint8x32_t divmod = uint8x16_divmod (a, b);
  return __builtin_lvx_high128 (divmod);
}

uint8x16_t
__udivmodv16qi4 (uint8x16_t a, uint8x16_t b, uint8x16_t *c)
{
  uint8x32_t divmod = uint8x16_divmod (a, b);
  *c = __builtin_lvx_high128 (divmod);
  return __builtin_lvx_low128 (divmod);
}

int8x16_t
__divv16qi3 (int8x16_t a, int8x16_t b)
{
  uint8x16_t absa = __builtin_lvx_absbx (a, "");
  uint8x16_t absb = __builtin_lvx_absbx (b, "");
  uint8x32_t divmod = uint8x16_divmod (absa, absb);
  int8x16_t result = __builtin_lvx_low128 (divmod);
  return __builtin_lvx_selectbx (-result, result, a ^ b, ".ltz");
}

int8x16_t
__modv16qi3 (int8x16_t a, int8x16_t b)
{
  uint8x16_t absa = __builtin_lvx_absbx (a, "");
  uint8x16_t absb = __builtin_lvx_absbx (b, "");
  uint8x32_t divmod = uint8x16_divmod (absa, absb);
  int8x16_t result = __builtin_lvx_high128 (divmod);
  return __builtin_lvx_selectbx (-result, result, a, ".ltz");
}

////////////////////////////////////////////////////////////////////////////////

static inline uint8x64_t
uint8x32_divmod (uint8x32_t a, uint8x32_t b)
{
  uint16x32_t src = __builtin_lvx_widenbhv (b, ".z") << (8 - 1);
  uint16x32_t wb = __builtin_lvx_widenbhv (b, ".z");
  DIV_BY_ZERO_MAY_TRAP (__builtin_lvx_anybv, b);
  uint16x32_t acc = __builtin_lvx_widenbhv (a, ".z");
  // As `src == b << (8 -1)` adding src yields `src == b << 8`.
  src += src & (wb > acc);
#pragma GCC unroll 8
  for (int i = 0; i < 8; i++)
    {
      acc = __builtin_lvx_stsuhv (src, acc);
    }
  uint8x32_t q = __builtin_lvx_narrowhbv (acc, "");
  uint8x32_t r = __builtin_lvx_narrowhbv (acc >> 8, "");
  return __builtin_lvx_cat512 (q, r);
}

uint8x32_t
__udivv32qi3 (uint8x32_t a, uint8x32_t b)
{
  uint8x64_t divmod = uint8x32_divmod (a, b);
  return __builtin_lvx_low256 (divmod);
}

uint8x32_t
__umodv32qi3 (uint8x32_t a, uint8x32_t b)
{
  uint8x64_t divmod = uint8x32_divmod (a, b);
  return __builtin_lvx_high256 (divmod);
}

uint8x32_t
__udivmodv32qi4 (uint8x32_t a, uint8x32_t b, uint8x32_t *c)
{
  uint8x64_t divmod = uint8x32_divmod (a, b);
  *c = __builtin_lvx_high256 (divmod);
  return __builtin_lvx_low256 (divmod);
}

int8x32_t
__divv32qi3 (int8x32_t a, int8x32_t b)
{
  uint8x32_t absa = __builtin_lvx_absbv (a, "");
  uint8x32_t absb = __builtin_lvx_absbv (b, "");
  uint8x64_t divmod = uint8x32_divmod (absa, absb);
  int8x32_t result = __builtin_lvx_low256 (divmod);
  return __builtin_lvx_selectbv (-result, result, a ^ b, ".ltz");
}

int8x32_t
__modv32qi3 (int8x32_t a, int8x32_t b)
{
  uint8x32_t absa = __builtin_lvx_absbv (a, "");
  uint8x32_t absb = __builtin_lvx_absbv (b, "");
  uint8x64_t divmod = uint8x32_divmod (absa, absb);
  int8x32_t result = __builtin_lvx_high256 (divmod);
  return __builtin_lvx_selectbv (-result, result, a, ".ltz");
}

#endif//__lvx_1__

