/* The inductive-relaxation half of the same gate.  It drops def-to-use arcs
   that are legal only once codegen performs PaCT 1997 §4's displacement
   adjustment (fixRelaxed), which does not exist -- so until it does the flag
   is measurement-only.  See cms-unsound-flags-gated.c.  */

/* { dg-do compile } */
/* { dg-options "-O2 -mcms-inductive-relaxation" } */
/* { dg-error "needs a displacement adjustment in code generation" "relaxation needs the probe" { target *-*-* } 0 } */

void f (double *a, double *b, int n)
{
  for (int i = 1; i < n; i++)
    a[i] = a[i-1] + b[i];
}
