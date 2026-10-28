/* -mcms-exact-degree is a measurement lever and must not reach code
   generation.  The renaming needs every value's expansion degree to divide
   the unroll factor; code generation needs that factor to be a power of two,
   because the preheader peels `count % kunroll' with a mask and takes
   `count / kunroll' trips with a shift, and on LVX the count feeds LOOPDO.
   No power of two is divisible by three, so an exact degree of 3 admits no
   unroll factor satisfying both -- the two requirements are incompatible,
   not merely in tension, and the combination is refused rather than silently
   rounded so the lever's figures keep meaning what they say.  §7tricies.2.  */

/* { dg-do compile } */
/* { dg-options "-O2 -mcms-probe -mcms-codegen -mcms-exact-degree" } */
/* { dg-error "must be one for the preheader to peel" "exact degrees are measurement only" { target *-*-* } 0 } */

double f (double *a, int n)
{
  double s = 1.0;
  for (int i = 0; i < n; i++)
    s = s * 1.03 + a[i];
  return s;
}
