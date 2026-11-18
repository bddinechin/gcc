/* An HF compare is one FCOMPH, on both cores.  It used to widen both
   operands with FWIDENHW and compare with FCOMPW.  An HF constant rides in
   the 32-bit immediate, which FCOMPH reads at 16 bits, negative or not.  */
/* { dg-do compile } */
/* { dg-options "-O2" } */

int lt (_Float16 a, _Float16 b) { return a < b; }
int ne (_Float16 a, _Float16 b) { return a != b; }
int lt_k (_Float16 a) { return a < (_Float16) -1.5; }
int br (_Float16 a, _Float16 b) { if (a < b) return 7; return 3; }

/* { dg-final { scan-assembler "fcomph\\.olt \\\$r0 = \\\$r0, \\\$r1" } } */
/* { dg-final { scan-assembler "fcomph\\.une \\\$r0 = \\\$r0, \\\$r1" } } */
/* { dg-final { scan-assembler "fcomph\\.olt \\\$r0 = \\\$r0, 0xbe00" } } */
/* { dg-final { scan-assembler-not "fwidenhw" } } */
/* { dg-final { scan-assembler-not "fcompw" } } */
