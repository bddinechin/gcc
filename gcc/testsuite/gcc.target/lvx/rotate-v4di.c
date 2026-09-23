/* The 64-bit-lane rotate must select the native ROLDP/RORDP at the 256-bit
   width the vectorizer picks by default -- not the shift/shift/or fallback.
   This guards instruction selection, which a value-only run cannot.
   A fixed iteration count keeps the cost model from declining the loop.  */
/* { dg-do compile } */
/* { dg-options "-O3 -march=lvx-2 -ffreestanding -fno-strict-aliasing -fwrapv" } */

typedef unsigned long u64;
#define N 1024

void rotl_v (u64 *__restrict d, const u64 *__restrict a)
{
  for (int i = 0; i < N; i++)
    { u64 x = a[i]; d[i] = (x << 13) | (x >> 51); }
}

void rotr_v (u64 *__restrict d, const u64 *__restrict a)
{
  for (int i = 0; i < N; i++)
    { u64 x = a[i]; d[i] = (x >> 13) | (x << 51); }
}

/* { dg-final { scan-assembler "roldp" } } */
/* { dg-final { scan-assembler "rordp" } } */
/* { dg-final { scan-assembler-not "slldp" } } */
/* { dg-final { scan-assembler-not "srldp" } } */
