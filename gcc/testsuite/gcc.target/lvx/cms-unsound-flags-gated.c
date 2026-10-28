/* The two unsound CMS measurement switches must not reach a real compile.
   §7decies' only retirement item with a correctness deadline: both drop
   dependences that nothing verifies, so without -mcms-probe they would
   silently miscompile rather than merely measure.  Gated in
   lvx_option_override; this is the test that the gate fires.  */

/* { dg-do compile } */
/* { dg-options "-O2 -mcms-assume-ivdep" } */
/* { dg-error "drops real cross-iteration memory dependences" "ivdep needs the probe" { target *-*-* } 0 } */

void f (double *a, double *b, int n)
{
  for (int i = 1; i < n; i++)
    a[i] = a[i-1] + b[i];
}
