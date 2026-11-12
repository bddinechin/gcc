/* lvx_makedp's predicate admitted a symbolic CONST its I32-only constraint
   could not hold, and LRA ICEd ("unable to generate reloads") on newlib's
   _mallocr.c.  A splat of an address must go through MAKED + SPLATDQ.  */
/* { dg-do compile } */
/* { dg-options "-march=lvx-2 -O2" } */

typedef long v2di __attribute__ ((vector_size (16)));
extern char buf[];

v2di
f (void)
{
  long a = (long) (buf + 16);
  return (v2di) { a, a };
}

/* { dg-final { scan-assembler-not "makedp\[^\\n\]*buf" } } */
