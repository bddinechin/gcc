#include "divmodtypes.h"

////////////////////////////////////////////////////////////////////////////////

static inline uint64x8_t
uint64x4_divmod (uint64x4_t a, uint64x4_t b)
{
  float64x4_t double1 = 1.0 - (float64x4_t){};
  int64x4_t bbig = (int64x4_t)b < 0;
  int64x4_t bin01 = (uint64x4_t)b <= 1;
  int64x4_t special = bbig | bin01;
  // uint64x4_t q = bbig ? a >= b : a;
  uint64x4_t q = __builtin_lvx_selectdq (-(a >= b), a, bbig, ".nez");
  // uint64x4_t r = bbig ? a - (b&-q) : 0;
  uint64x4_t r
    = __builtin_lvx_selectdq (a - (b & -q), 0 - (uint64x4_t){}, bbig, ".nez");
  float64x4_t doublea = __builtin_lvx_floatudq (a, 0, ".rn.s");
  float64x4_t doubleb = __builtin_lvx_floatudq (b, 0, ".rn.s");
  float floatb_0 = __builtin_lvx_fnarrowdw (doubleb[0], ".rn.s");
  float floatb_1 = __builtin_lvx_fnarrowdw (doubleb[1], ".rn.s");
  float floatb_2 = __builtin_lvx_fnarrowdw (doubleb[2], ".rn.s");
  float floatb_3 = __builtin_lvx_fnarrowdw (doubleb[3], ".rn.s");
  float floatrec_0 = __builtin_lvx_frecw (floatb_0, ".rn.s");
  float floatrec_1 = __builtin_lvx_frecw (floatb_1, ".rn.s");
  float floatrec_2 = __builtin_lvx_frecw (floatb_2, ".rn.s");
  float floatrec_3 = __builtin_lvx_frecw (floatb_3, ".rn.s");
  if (__builtin_lvx_anydq (b, ".eqz"))
    goto div0;
  float64x4_t doublerec = {__builtin_lvx_fwidenwd (floatrec_0, ".s"),
			   __builtin_lvx_fwidenwd (floatrec_1, ".s"),
			   __builtin_lvx_fwidenwd (floatrec_2, ".s"),
			   __builtin_lvx_fwidenwd (floatrec_3, ".s")};
  float64x4_t doubleq0 = __builtin_lvx_fmuldq (doublea, doublerec, ".rn.s");
  uint64x4_t q0 = __builtin_lvx_fixedudq (doubleq0, 0, ".rn.s");
  int64x4_t a1 = (int64x4_t)(a - q0 * b);
  float64x4_t alpha
    = __builtin_lvx_ffmsdq (doubleb, doublerec, double1, ".rn.s");
  float64x4_t beta
    = __builtin_lvx_ffmadq (alpha, doublerec, doublerec, ".rn.s");
  float64x4_t doublea1 = __builtin_lvx_floatdq (a1, 0, ".rn.s");
  float64x4_t gamma = __builtin_lvx_fmuldq (beta, doublea1, ".rn.s");
  int64x4_t q1 = __builtin_lvx_fixeddq (gamma, 0, ".rn.s");
  int64x4_t rem = a1 - q1 * b;
  uint64x4_t quo = q0 + q1;
  uint64x4_t cond = (uint64x4_t)(rem >> 63);
  // q = !special ? quo + cond : q;
  q = __builtin_lvx_selectdq (quo + cond, q, special, ".eqz");
  // r = !special ? rem + (b & cond) : r;
  r = __builtin_lvx_selectdq (rem + (b & cond), r, special, ".eqz");
  return __builtin_lvx_cat512 (q, r);

div0:
  return 0 - (uint64x8_t){};
}

uint64x4_t
__udivv4di3 (uint64x4_t a, uint64x4_t b)
{
  uint64x8_t divmod = uint64x4_divmod (a, b);
  return __builtin_lvx_low256 (divmod);
}

uint64x4_t
__umodv4di3 (uint64x4_t a, uint64x4_t b)
{
  uint64x8_t divmod = uint64x4_divmod (a, b);
  return __builtin_lvx_high256 (divmod);
}

uint64x4_t
__udivmodv4di4 (uint64x4_t a, uint64x4_t b, uint64x4_t *c)
{
  uint64x8_t divmod = uint64x4_divmod (a, b);
  *c = __builtin_lvx_high256 (divmod);
  return __builtin_lvx_low256 (divmod);
}

int64x4_t
__divv4di3 (int64x4_t a, int64x4_t b)
{
  uint64x4_t absa = __builtin_lvx_absdq (a, "");
  uint64x4_t absb = __builtin_lvx_absdq (b, "");
  uint64x8_t divmod = uint64x4_divmod (absa, absb);
  int64x4_t result = __builtin_lvx_low256 (divmod);
  return __builtin_lvx_selectdq (-result, result, a ^ b, ".ltz");
}

int64x4_t
__modv4di3 (int64x4_t a, int64x4_t b)
{
  uint64x4_t absa = __builtin_lvx_absdq (a, "");
  uint64x4_t absb = __builtin_lvx_absdq (b, "");
  uint64x8_t divmod = uint64x4_divmod (absa, absb);
  int64x4_t result = __builtin_lvx_high256 (divmod);
  return __builtin_lvx_selectdq (-result, result, a, ".ltz");
}

////////////////////////////////////////////////////////////////////////////////

#ifdef TEST_V4DI
#define LANE 0

uint64_t
__udivdi3 (uint64_t a, uint64_t b)
{
  uint64x4_t udivv4di3 = __udivv4di3 (a - (uint64x4_t){}, b - (uint64x4_t){});
  return (uint64_t)udivv4di3[LANE];
}

uint64_t
__umoddi3 (uint64_t a, uint64_t b)
{
  uint64x4_t umodv4di3 = __umodv4di3 (a - (uint64x4_t){}, b - (uint64x4_t){});
  return (uint64_t)umodv4di3[LANE];
}

uint64_t
__udivmoddi4 (uint64_t a, uint64_t b, uint64_t *c)
{
  uint64x4_t c_ = {0, 0};
  uint64x4_t udivmodv4di4
    = __udivmodv4di4 (a - (uint64x4_t){}, b - (uint64x4_t){}, &c_);
  if (c)
    *c = c_[LANE];
  return (uint64_t)udivmodv4di4[LANE];
}

int64_t
__divdi3 (int64_t a, int64_t b)
{
  int64x4_t divv4di3 = __divv4di3 (a - (int64x4_t){}, b - (int64x4_t){});
  return (int64_t)divv4di3[LANE];
}

int64_t
__moddi3 (int64_t a, int64_t b)
{
  int64x4_t modv4di3 = __modv4di3 (a - (int64x4_t){}, b - (int64x4_t){});
  return (int64_t)modv4di3[LANE];
}

#endif // TEST_V4DI

