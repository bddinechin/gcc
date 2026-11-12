/* sched2 runs with DO_PREDICATION and without DO_SPECULATION
   (lvx_sched_set_sched_flags).  Under DO_PREDICATION a REG_DEP_CONTROL
   dependence counts as speculative, and when update_dep turned one into a
   REG_DEP_ANTI, change_spec_dep_to_hard cleared its bit in
   spec_dependency_cache -- which exists only under DO_SPECULATION.  A NULL
   base, and a segfault in the mach pass.  Reduced from
   validation/tests/micro/atomics.c; it needs a large region, hence the bulk.  */
/* { dg-do compile } */
/* { dg-options "-O3 -march=lvx-1 -ffreestanding -fno-strict-aliasing -fwrapv" } */

static unsigned h = 0x811c9dc5u;
static void fold (unsigned long long v)
{
 for (int i = 0; i < 8; i++) {
  h ^= (unsigned char) (v >> (i * 8));
  h *= 0x01000193u;
 }
}
static inline signed char al_min_b (signed char *p, signed char v) { return __builtin_lvx_alminb (p, v); }
static inline short al_max_h (short *p, short v) { return __builtin_lvx_almaxh (p, v); }
static inline int al_min_w (int *p, int v) { return __builtin_lvx_alminw (p, v); }
static inline unsigned al_minu_w (unsigned *p, unsigned v) { return __builtin_lvx_alminuw (p, v); }
static inline unsigned long al_maxu_d (unsigned long *p, unsigned long v) { return __builtin_lvx_almaxud (p, v); }
static inline unsigned al_dus_w (unsigned *p, unsigned v) { return __builtin_lvx_aldusw (p, v); }
static inline unsigned long al_dus_d (unsigned long *p, unsigned long v) { return __builtin_lvx_aldusd (p, v); }
static inline void as_min_b (signed char *p, signed char v) { __builtin_lvx_asminb (v, p); }
static inline void as_max_w (int *p, int v) { __builtin_lvx_asmaxw (v, p); }
static inline void as_maxu_d (unsigned long *p, unsigned long v) { __builtin_lvx_asmaxud (v, p); }
static inline void as_dus_h (unsigned short *p, unsigned short v) { __builtin_lvx_asdush (v, p); }
int test_main (void)
{
 do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = (0x5a); a[2] = (0x3c); signed char r = (__atomic_fetch_add ((signed char *)&a[1], (0x27), 0)); fold ((unsigned long long) r); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = (0x5a); a[2] = (0x3c); signed char r = (__atomic_fetch_sub ((signed char *)&a[1], (0x27), 0)); fold ((unsigned long long) r); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = (0x5a); a[2] = (0x3c); signed char r = (__atomic_fetch_and ((signed char *)&a[1], (0x27), 0)); fold ((unsigned long long) r); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = (0x5a); a[2] = (0x3c); signed char r = (__atomic_fetch_or ((signed char *)&a[1], (0x27), 0)); fold ((unsigned long long) r); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = (0x5a); a[2] = (0x3c); signed char r = (__atomic_fetch_xor ((signed char *)&a[1], (0x27), 0)); fold ((unsigned long long) r); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = (0x5a); a[2] = (0x3c); signed char r = (__atomic_fetch_nand ((signed char *)&a[1], (0x27), 0)); fold ((unsigned long long) r); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = (0x5a); a[2] = (0x3c); signed char r = (__atomic_add_fetch ((signed char *)&a[1], (0x27), 5)); fold ((unsigned long long) r); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = (0x5a); a[2] = (0x3c); signed char r = (__atomic_sub_fetch ((signed char *)&a[1], (0x27), 5)); fold ((unsigned long long) r); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = (0x5a); a[2] = (0x3c); signed char r = (__atomic_and_fetch ((signed char *)&a[1], (0x27), 5)); fold ((unsigned long long) r); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = (0x5a); a[2] = (0x3c); signed char r = (__atomic_or_fetch ((signed char *)&a[1], (0x27), 2)); fold ((unsigned long long) r); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = (0x5a); a[2] = (0x3c); signed char r = (__atomic_xor_fetch ((signed char *)&a[1], (0x27), 3)); fold ((unsigned long long) r); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = (0x5a); a[2] = (0x3c); signed char r = (__atomic_exchange_n ((signed char *)&a[1], (0x27), 0)); fold ((unsigned long long) r); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile int a[3]; a[0] = (0x3c3c3c3c); a[1] = (0x5a5a5a5a); a[2] = (0x3c3c3c3c); int e = (0x7fffffff); int ok = __atomic_compare_exchange_n ((int *)&a[1], &e, (0x11111111), 1, 0, 0); fold (ok); fold ((unsigned long long) e); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile long a[3]; a[0] = (0x3c3cL); a[1] = (0x5a5a5a5aL); a[2] = (0x3c3cL); long e = (0x5a5a5a5aL); int ok = __atomic_compare_exchange_n ((long *)&a[1], &e, (0x1111L), 1, 0, 0); fold (ok); fold ((unsigned long long) e); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 {
  static volatile __int128 a[3];
  const __int128 init = ((__int128) 0x0123456789abcdefL << 64)
           | (unsigned long long) 0x3c3c3c3c3c3c3c3cUL;
  for (int fail = 0; fail < 2; fail++) {
   fold ((unsigned long long) a[0]);
   fold ((unsigned long long) (a[2] >> 64));
  }
 }
 do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = ((signed char) 0xf0); a[2] = (0x3c); fold ((unsigned long long) al_min_b ((signed char *) &a[1], (0x10))); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = (0x10); a[2] = (0x3c); fold ((unsigned long long) al_min_b ((signed char *) &a[1], ((signed char) 0xf0))); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile short a[3]; a[0] = (0x3c3c); a[1] = ((short) 0xf000); a[2] = (0x3c3c); fold ((unsigned long long) al_max_h ((short *) &a[1], (0x1000))); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile int a[3]; a[0] = (0x3c3c3c3c); a[1] = ((int) 0xf0000000); a[2] = (0x3c3c3c3c); fold ((unsigned long long) al_min_w ((int *) &a[1], (0x10000000))); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile unsigned a[3]; a[0] = (0x3c3c3c3cu); a[1] = (0xf0000000u); a[2] = (0x3c3c3c3cu); fold ((unsigned long long) al_minu_w ((unsigned *) &a[1], (0x10000000u))); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile unsigned long a[3]; a[0] = (0x3c3cUL); a[1] = (0xf000000000000000UL); a[2] = (0x3c3cUL); fold ((unsigned long long) al_maxu_d ((unsigned long *) &a[1], (0x1000UL))); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile unsigned a[3]; a[0] = (0x3c3c3c3cu); a[1] = (0x50000000u); a[2] = (0x3c3c3c3cu); fold ((unsigned long long) al_dus_w ((unsigned *) &a[1], (0x10000000u))); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile unsigned a[3]; a[0] = (0x3c3c3c3cu); a[1] = (0x10000000u); a[2] = (0x3c3c3c3cu); fold ((unsigned long long) al_dus_w ((unsigned *) &a[1], (0x50000000u))); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile unsigned long a[3]; a[0] = (0x3c3cUL); a[1] = (0x5000UL); a[2] = (0x3c3cUL); fold ((unsigned long long) al_dus_d ((unsigned long *) &a[1], (0x1000UL))); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile unsigned long a[3]; a[0] = (0x3c3cUL); a[1] = (0x1000UL); a[2] = (0x3c3cUL); fold ((unsigned long long) al_dus_d ((unsigned long *) &a[1], (0x5000UL))); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile signed char a[3]; a[0] = (0x3c); a[1] = ((signed char) 0xf0); a[2] = (0x3c); as_min_b ((signed char *) &a[1], (0x10)); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile int a[3]; a[0] = (0x3c3c3c3c); a[1] = ((int) 0xf0000000); a[2] = (0x3c3c3c3c); as_max_w ((int *) &a[1], (0x10000000)); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile unsigned long a[3]; a[0] = (0x3c3cUL); a[1] = (0xf000000000000000UL); a[2] = (0x3c3cUL); as_maxu_d ((unsigned long *) &a[1], (0x1000UL)); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile unsigned short a[3]; a[0] = (0x3c3c); a[1] = (0x5000); a[2] = (0x3c3c); as_dus_h ((unsigned short *) &a[1], (0x1000)); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 do { static volatile unsigned short a[3]; a[0] = (0x3c3c); a[1] = (0x1000); a[2] = (0x3c3c); as_dus_h ((unsigned short *) &a[1], (0x5000)); fold ((unsigned long long) a[1]); fold ((unsigned long long) a[0]); fold ((unsigned long long) a[2]); } while (0);
 return (int) (h & 0x7fffffff);
}