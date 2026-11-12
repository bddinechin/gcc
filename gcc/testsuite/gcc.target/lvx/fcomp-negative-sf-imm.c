/* FCOMPW reads its whole 32-bit immediate, so a negative float constant
   rides in it.  H32 used to zero-extend the pattern and then ask for a signed
   fit, which refused every negative float and spent a MAKED.  A double must
   still be a sign-extended word: -1.5 is not, 0.0 is.  */
/* { dg-do compile } */
/* { dg-options "-O2" } */

int f (float x) { return x < -1.5f; }
int g (double x) { return x < 0.0; }
int h (double x) { return x < -1.5; }

/* { dg-final { scan-assembler "fcompw\\.olt \\\$r0 = \\\$r0, 0xbfc00000" } } */
/* { dg-final { scan-assembler "fcompd\\.olt \\\$r0 = \\\$r0, 0x0000000000000000" } } */
/* { dg-final { scan-assembler-times "maked" 1 } } */
