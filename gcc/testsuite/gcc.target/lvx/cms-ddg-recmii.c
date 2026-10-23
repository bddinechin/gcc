/* The CMS recurrence bound must agree with SMS's, computed from the same
   dependence graph.

   This is the oracle for the one part of CMS that has no other check.  SMS
   builds its ddg BEFORE register allocation, so its register arcs are true
   dataflow over pseudos; lvx_cms_build_graph reconstructs them AFTER
   allocation, from reaching definitions.  Comparing the two recurrence bounds
   on one graph is therefore a test of that reconstruction against ground
   truth -- and the reconstruction is the part that has been wrong twice, once
   by manufacturing all-pairs cross-iteration arcs and once by keeping WAR arcs
   after the RAW walk was fixed.  Both would have failed this test.
   See docs/cms-foundations.md §7nonies.

   The loop is a floating-point accumulation, so the recurrence is the
   accumulator's own dependence and K is exactly the FP add latency: 4.  Two
   independent algorithms -- SMS's SCC recurrence_length and lvx_plc's
   parametric critical ratio -- must both say 4.  If the latency table changes,
   both numbers move together and this test should be updated in one place;
   if only one moves, that is the drift it exists to catch.  */

/* { dg-do compile } */
/* { dg-options "-O2 -march=lvx-1 -ffast-math -fmodulo-sched -fmodulo-sched-allow-regmoves -mcms-probe -fdump-rtl-sms" } */

void f (double *__restrict a, const double *__restrict b, int n)
{
  double s = 0.0;
  for (int i = 0; i < n; i++)
    s += b[i];
  a[0] = s;
}

/* SMS's own recurrence bound, the first of the three it prints.  */
/* { dg-final { scan-rtl-dump "SMS iis 4 " "sms" } } */

/* CMS's, over the same graph, via the TARGET_SCHED_SMS_RES_MII piggyback.  */
/* { dg-final { scan-rtl-dump "cms-on-ddg:\[^\n\]*RecMII=4 " "sms" } } */
