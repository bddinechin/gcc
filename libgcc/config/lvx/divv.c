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
typedef int    v4si __attribute__ ((vector_size (16)));

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

v4si
__divv4si3 (v4si a, v4si b)
{
  return (v4si) { a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3] };
}
