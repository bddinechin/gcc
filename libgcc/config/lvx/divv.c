/* Lane-wise vector divide helpers for LVX (lvx-2).
 *
 * LVX has scalar fdivw/fdivd and integer divmodw/divmodd, but no *vector*
 * divide.  The middle end lowers a vector `a / b` to a call to __div<mode>3,
 * so on lvx-2 every one of these is a link-time undefined symbol unless
 * libgcc provides it -- there is no source for the floating-point ones
 * anywhere, which is what this file supplies.
 *
 * Each is the division done one lane at a time and the results reassembled.
 * Writing them as an explicit brace constructor of scalar divides, rather than
 * a loop, is load-bearing: a loop at -O2 would re-vectorise straight back into
 * the __div<mode>3 call this file is defining, an infinite recursion the
 * constructor form cannot express.  The scalar `/` lowers to fdivw/fdivd for
 * float and to the divmodw/divmodd hardware for int, and the reassembly is the
 * SPLAT/INSF vec_init path -- so a v4sf divide is four fdivw and one build.
 *
 * IEEE semantics come for free: each lane is a genuine scalar fdiv, so
 * rounding, NaN, inf and the $cs exception flags are exactly the scalar ones,
 * which is what a lane-wise vector divide is required to be.
 *
 * The constructor form alone is not enough: at -O2 the SLP vectoriser
 * reassembles the four scalar divides into a v4sf divide and emits a call to
 * the very function being defined.  Disabling tree vectorisation for the file
 * is the standard libgcc guard against that recursion.
 */
#pragma GCC optimize ("no-tree-vectorize")

typedef float  v4sf __attribute__ ((vector_size (16)));
typedef float  v8sf __attribute__ ((vector_size (32)));
typedef double v2df __attribute__ ((vector_size (16)));
typedef double v4df __attribute__ ((vector_size (32)));
typedef signed char v16qi __attribute__ ((vector_size (16)));
typedef unsigned char v16qiu __attribute__ ((vector_size (16)));
typedef signed char v32qi __attribute__ ((vector_size (32)));
typedef unsigned char v32qiu __attribute__ ((vector_size (32)));
typedef short v8hi __attribute__ ((vector_size (16)));
typedef unsigned short v8hiu __attribute__ ((vector_size (16)));
typedef int    v4si __attribute__ ((vector_size (16)));
typedef unsigned int v4siu __attribute__ ((vector_size (16)));
typedef long v2di __attribute__ ((vector_size (16)));
typedef unsigned long v2diu __attribute__ ((vector_size (16)));
typedef short v16hi __attribute__ ((vector_size (32)));
typedef unsigned short v16hiu __attribute__ ((vector_size (32)));
typedef int v8si __attribute__ ((vector_size (32)));
typedef unsigned int v8siu __attribute__ ((vector_size (32)));
typedef long v4di __attribute__ ((vector_size (32)));
typedef unsigned long v4diu __attribute__ ((vector_size (32)));

/* libgcc builds with -Wmissing-prototypes -Werror; declare before defining. */
v4sf __divv4sf3 (v4sf, v4sf);
v8sf __divv8sf3 (v8sf, v8sf);
v2df __divv2df3 (v2df, v2df);
v4df __divv4df3 (v4df, v4df);
v4si __divv4si3 (v4si, v4si);
v16qi __divv16qi3 (v16qi, v16qi);
v16qi __modv16qi3 (v16qi, v16qi);
v16qiu __udivv16qi3 (v16qiu, v16qiu);
v16qiu __umodv16qi3 (v16qiu, v16qiu);
v32qi __divv32qi3 (v32qi, v32qi);
v32qi __modv32qi3 (v32qi, v32qi);
v32qiu __udivv32qi3 (v32qiu, v32qiu);
v32qiu __umodv32qi3 (v32qiu, v32qiu);
v8hi __divv8hi3 (v8hi, v8hi);
v8hi __modv8hi3 (v8hi, v8hi);
v8hiu __udivv8hi3 (v8hiu, v8hiu);
v8hiu __umodv8hi3 (v8hiu, v8hiu);
v4si __modv4si3 (v4si, v4si);
v4siu __udivv4si3 (v4siu, v4siu);
v4siu __umodv4si3 (v4siu, v4siu);
v2di __divv2di3 (v2di, v2di);
v2di __modv2di3 (v2di, v2di);
v2diu __udivv2di3 (v2diu, v2diu);
v2diu __umodv2di3 (v2diu, v2diu);
v16hi __divv16hi3 (v16hi, v16hi);
v16hi __modv16hi3 (v16hi, v16hi);
v16hiu __udivv16hi3 (v16hiu, v16hiu);
v16hiu __umodv16hi3 (v16hiu, v16hiu);
v8si __divv8si3 (v8si, v8si);
v8si __modv8si3 (v8si, v8si);
v8siu __udivv8si3 (v8siu, v8siu);
v8siu __umodv8si3 (v8siu, v8siu);
v4di __divv4di3 (v4di, v4di);
v4di __modv4di3 (v4di, v4di);
v4diu __udivv4di3 (v4diu, v4diu);
v4diu __umodv4di3 (v4diu, v4diu);

v4sf
__divv4sf3 (v4sf a, v4sf b)
{
  return (v4sf) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3] };
}

v8sf
__divv8sf3 (v8sf a, v8sf b)
{
  return (v8sf) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3],
		  a[4] / b[4], a[5] / b[5], a[6] / b[6], a[7] / b[7] };
}

v2df
__divv2df3 (v2df a, v2df b)
{
  return (v2df) { a[0] / b[0], a[1] / b[1] };
}

v4df
__divv4df3 (v4df a, v4df b)
{
  return (v4df) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3] };
}

/* The integer lane-wise dividers, on the hardware divmodw/divmodd: every
   div/mod/udiv/umod<mode>3 expander of vector.md over V16QI/V8HI/V4SI/V2DI and
   V32QI/V16HI/V8SI/V4DI calls one of these.  (divmodvx*.c, the bit-serial stsu
   versions, are a later optimisation: they predate the builtin renames.)  */

v16qi
__divv16qi3 (v16qi a, v16qi b)
{
  return (v16qi) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3], a[4] / b[4], a[5] / b[5], a[6] / b[6], a[7] / b[7], a[8] / b[8], a[9] / b[9], a[10] / b[10], a[11] / b[11], a[12] / b[12], a[13] / b[13], a[14] / b[14], a[15] / b[15] };
}

v16qi
__modv16qi3 (v16qi a, v16qi b)
{
  return (v16qi) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3], a[4] % b[4], a[5] % b[5], a[6] % b[6], a[7] % b[7], a[8] % b[8], a[9] % b[9], a[10] % b[10], a[11] % b[11], a[12] % b[12], a[13] % b[13], a[14] % b[14], a[15] % b[15] };
}

v16qiu
__udivv16qi3 (v16qiu a, v16qiu b)
{
  return (v16qiu) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3], a[4] / b[4], a[5] / b[5], a[6] / b[6], a[7] / b[7], a[8] / b[8], a[9] / b[9], a[10] / b[10], a[11] / b[11], a[12] / b[12], a[13] / b[13], a[14] / b[14], a[15] / b[15] };
}

v16qiu
__umodv16qi3 (v16qiu a, v16qiu b)
{
  return (v16qiu) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3], a[4] % b[4], a[5] % b[5], a[6] % b[6], a[7] % b[7], a[8] % b[8], a[9] % b[9], a[10] % b[10], a[11] % b[11], a[12] % b[12], a[13] % b[13], a[14] % b[14], a[15] % b[15] };
}

v32qi
__divv32qi3 (v32qi a, v32qi b)
{
  return (v32qi) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3], a[4] / b[4], a[5] / b[5], a[6] / b[6], a[7] / b[7], a[8] / b[8], a[9] / b[9], a[10] / b[10], a[11] / b[11], a[12] / b[12], a[13] / b[13], a[14] / b[14], a[15] / b[15], a[16] / b[16], a[17] / b[17], a[18] / b[18], a[19] / b[19], a[20] / b[20], a[21] / b[21], a[22] / b[22], a[23] / b[23], a[24] / b[24], a[25] / b[25], a[26] / b[26], a[27] / b[27], a[28] / b[28], a[29] / b[29], a[30] / b[30], a[31] / b[31] };
}

v32qi
__modv32qi3 (v32qi a, v32qi b)
{
  return (v32qi) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3], a[4] % b[4], a[5] % b[5], a[6] % b[6], a[7] % b[7], a[8] % b[8], a[9] % b[9], a[10] % b[10], a[11] % b[11], a[12] % b[12], a[13] % b[13], a[14] % b[14], a[15] % b[15], a[16] % b[16], a[17] % b[17], a[18] % b[18], a[19] % b[19], a[20] % b[20], a[21] % b[21], a[22] % b[22], a[23] % b[23], a[24] % b[24], a[25] % b[25], a[26] % b[26], a[27] % b[27], a[28] % b[28], a[29] % b[29], a[30] % b[30], a[31] % b[31] };
}

v32qiu
__udivv32qi3 (v32qiu a, v32qiu b)
{
  return (v32qiu) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3], a[4] / b[4], a[5] / b[5], a[6] / b[6], a[7] / b[7], a[8] / b[8], a[9] / b[9], a[10] / b[10], a[11] / b[11], a[12] / b[12], a[13] / b[13], a[14] / b[14], a[15] / b[15], a[16] / b[16], a[17] / b[17], a[18] / b[18], a[19] / b[19], a[20] / b[20], a[21] / b[21], a[22] / b[22], a[23] / b[23], a[24] / b[24], a[25] / b[25], a[26] / b[26], a[27] / b[27], a[28] / b[28], a[29] / b[29], a[30] / b[30], a[31] / b[31] };
}

v32qiu
__umodv32qi3 (v32qiu a, v32qiu b)
{
  return (v32qiu) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3], a[4] % b[4], a[5] % b[5], a[6] % b[6], a[7] % b[7], a[8] % b[8], a[9] % b[9], a[10] % b[10], a[11] % b[11], a[12] % b[12], a[13] % b[13], a[14] % b[14], a[15] % b[15], a[16] % b[16], a[17] % b[17], a[18] % b[18], a[19] % b[19], a[20] % b[20], a[21] % b[21], a[22] % b[22], a[23] % b[23], a[24] % b[24], a[25] % b[25], a[26] % b[26], a[27] % b[27], a[28] % b[28], a[29] % b[29], a[30] % b[30], a[31] % b[31] };
}
v8hi
__divv8hi3 (v8hi a, v8hi b)
{
  return (v8hi) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3], a[4] / b[4], a[5] / b[5], a[6] / b[6], a[7] / b[7] };
}

v8hi
__modv8hi3 (v8hi a, v8hi b)
{
  return (v8hi) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3], a[4] % b[4], a[5] % b[5], a[6] % b[6], a[7] % b[7] };
}

v8hiu
__udivv8hi3 (v8hiu a, v8hiu b)
{
  return (v8hiu) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3], a[4] / b[4], a[5] / b[5], a[6] / b[6], a[7] / b[7] };
}

v8hiu
__umodv8hi3 (v8hiu a, v8hiu b)
{
  return (v8hiu) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3], a[4] % b[4], a[5] % b[5], a[6] % b[6], a[7] % b[7] };
}

v4si
__divv4si3 (v4si a, v4si b)
{
  return (v4si) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3] };
}

v4si
__modv4si3 (v4si a, v4si b)
{
  return (v4si) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3] };
}

v4siu
__udivv4si3 (v4siu a, v4siu b)
{
  return (v4siu) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3] };
}

v4siu
__umodv4si3 (v4siu a, v4siu b)
{
  return (v4siu) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3] };
}

v2di
__divv2di3 (v2di a, v2di b)
{
  return (v2di) { a[0] / b[0], a[1] / b[1] };
}

v2di
__modv2di3 (v2di a, v2di b)
{
  return (v2di) { a[0] % b[0], a[1] % b[1] };
}

v2diu
__udivv2di3 (v2diu a, v2diu b)
{
  return (v2diu) { a[0] / b[0], a[1] / b[1] };
}

v2diu
__umodv2di3 (v2diu a, v2diu b)
{
  return (v2diu) { a[0] % b[0], a[1] % b[1] };
}

v16hi
__divv16hi3 (v16hi a, v16hi b)
{
  return (v16hi) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3], a[4] / b[4], a[5] / b[5], a[6] / b[6], a[7] / b[7], a[8] / b[8], a[9] / b[9], a[10] / b[10], a[11] / b[11], a[12] / b[12], a[13] / b[13], a[14] / b[14], a[15] / b[15] };
}

v16hi
__modv16hi3 (v16hi a, v16hi b)
{
  return (v16hi) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3], a[4] % b[4], a[5] % b[5], a[6] % b[6], a[7] % b[7], a[8] % b[8], a[9] % b[9], a[10] % b[10], a[11] % b[11], a[12] % b[12], a[13] % b[13], a[14] % b[14], a[15] % b[15] };
}

v16hiu
__udivv16hi3 (v16hiu a, v16hiu b)
{
  return (v16hiu) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3], a[4] / b[4], a[5] / b[5], a[6] / b[6], a[7] / b[7], a[8] / b[8], a[9] / b[9], a[10] / b[10], a[11] / b[11], a[12] / b[12], a[13] / b[13], a[14] / b[14], a[15] / b[15] };
}

v16hiu
__umodv16hi3 (v16hiu a, v16hiu b)
{
  return (v16hiu) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3], a[4] % b[4], a[5] % b[5], a[6] % b[6], a[7] % b[7], a[8] % b[8], a[9] % b[9], a[10] % b[10], a[11] % b[11], a[12] % b[12], a[13] % b[13], a[14] % b[14], a[15] % b[15] };
}

v8si
__divv8si3 (v8si a, v8si b)
{
  return (v8si) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3], a[4] / b[4], a[5] / b[5], a[6] / b[6], a[7] / b[7] };
}

v8si
__modv8si3 (v8si a, v8si b)
{
  return (v8si) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3], a[4] % b[4], a[5] % b[5], a[6] % b[6], a[7] % b[7] };
}

v8siu
__udivv8si3 (v8siu a, v8siu b)
{
  return (v8siu) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3], a[4] / b[4], a[5] / b[5], a[6] / b[6], a[7] / b[7] };
}

v8siu
__umodv8si3 (v8siu a, v8siu b)
{
  return (v8siu) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3], a[4] % b[4], a[5] % b[5], a[6] % b[6], a[7] % b[7] };
}

v4di
__divv4di3 (v4di a, v4di b)
{
  return (v4di) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3] };
}

v4di
__modv4di3 (v4di a, v4di b)
{
  return (v4di) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3] };
}

v4diu
__udivv4di3 (v4diu a, v4diu b)
{
  return (v4diu) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3] };
}

v4diu
__umodv4di3 (v4diu a, v4diu b)
{
  return (v4diu) { a[0] % b[0], a[1] % b[1], a[2] % b[2], a[3] % b[3] };
}
