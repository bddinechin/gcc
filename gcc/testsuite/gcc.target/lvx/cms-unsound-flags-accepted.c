/* The other side of the gate: WITH -mcms-probe both switches are accepted,
   since that is the measurement path they exist for.  A gate that rejected
   them unconditionally would make every figure in docs/cms-foundations.md
   unreproducible.  */

/* { dg-do compile } */
/* { dg-options "-O2 -mcms-probe -mcms-assume-ivdep -mcms-inductive-relaxation" } */

void f (double *a, double *b, int n)
{
  for (int i = 1; i < n; i++)
    a[i] = a[i-1] + b[i];
}
