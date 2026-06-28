#ifndef __H__LVX_BUILTINS__
#define __H__LVX_BUILTINS__

#ifdef __llvm__
/** LLVM builtin emulation helpers */

#define __get4low(a) __builtin_shufflevector((a), (a), 0, 1, 2, 3)
#define __get4hi(a) __builtin_shufflevector((a), (a), 4, 5, 6, 7)

#define __get8low(a) __builtin_shufflevector((a), (a), 0, 1, 2, 3, 4, 5, 6, 7)

#define __get8hi(a)                                                            \
  __builtin_shufflevector((a), (a), 8, 9, 10, 11, 12, 13, 14, 15)

#define __join8(a, b) __builtin_shufflevector(a, b, 0, 1, 2, 3, 4, 5, 6, 7)

#define __join16(a, b)                                                         \
  __builtin_shufflevector(a, b, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  \
                          14, 15)

#define __split_op8(a, name, type)                                             \
  (type) __join8(__builtin_lvx_##name(__get4low(a)),                           \
                 __builtin_lvx_##name(__get4hi(a)))

#define __split_binmod_op8(a, b, mod, name, type)                              \
  (type) __join8(__builtin_lvx_##name(__get4low(a), __get4low(b), mod),        \
                 __builtin_lvx_##name(__get4hi(a), __get4hi(b), mod))

#define __split_termod_op8(a, b, c, mod, name, type)                           \
  (type) __join8(                                                              \
      __builtin_lvx_##name(__get4low(a), __get4low(b), __get4low(c), mod),     \
      __builtin_lvx_##name(__get4hi(a), __get4hi(b), __get4hi(c), mod))


#define __LVX_EXT_OP_RED(op, name, inType, midType, outType)                   \
  inline outType __builtin_lvx_##name(inType V) {                              \
    midType M = __builtin_convertvector(V, midType);                           \
    return __builtin_reduce_##op(M);                                           \
  }

#define __LVX_OP_EXT_RED(op, name, inType, outType)                            \
  inline outType __builtin_lvx_##name(inType V) {                              \
    return __builtin_reduce_##op(V);                                           \
  }

__LVX_EXT_OP_RED(add, addrbod, __lvx_v8qi, __lvx_v8di, long)
__LVX_EXT_OP_RED(add, addrbpd, __lvx_v2qi, __lvx_v2di, long)
__LVX_EXT_OP_RED(add, addrbqd, __lvx_v4qi, __lvx_v4di, long)
__LVX_EXT_OP_RED(add, addrbvd, __lvx_v32qi, __lvx_v32di, long)
__LVX_EXT_OP_RED(add, addrbxd, __lvx_v16qi, __lvx_v16di, long)

__LVX_EXT_OP_RED(add, addrhod, __lvx_v8hi, __lvx_v8di, long)
__LVX_EXT_OP_RED(add, addrhpd, __lvx_v2hi, __lvx_v2di, long)
__LVX_EXT_OP_RED(add, addrhqd, __lvx_v4hi, __lvx_v4di, long)
__LVX_EXT_OP_RED(add, addrhvd, __lvx_v32hi, __lvx_v32di, long)
__LVX_EXT_OP_RED(add, addrhxd, __lvx_v16hi, __lvx_v16di, long)

__LVX_EXT_OP_RED(add, addrwod, __lvx_v8si, __lvx_v8di, long)
__LVX_EXT_OP_RED(add, addrwpd, __lvx_v2si, __lvx_v2di, long)
__LVX_EXT_OP_RED(add, addrwqd, __lvx_v4si, __lvx_v4di, long)
__LVX_EXT_OP_RED(add, addrwvd, __lvx_v32si, __lvx_v32di, long)
__LVX_EXT_OP_RED(add, addrwxd, __lvx_v16si, __lvx_v16di, long)

__LVX_EXT_OP_RED(add, addurbod, __lvx_v8qu, __lvx_v8du, unsigned long)
__LVX_EXT_OP_RED(add, addurbpd, __lvx_v2qu, __lvx_v2du, unsigned long)
__LVX_EXT_OP_RED(add, addurbqd, __lvx_v4qu, __lvx_v4du, unsigned long)
__LVX_EXT_OP_RED(add, addurbvd, __lvx_v32qu, __lvx_v32du, unsigned long)
__LVX_EXT_OP_RED(add, addurbxd, __lvx_v16qu, __lvx_v16du, unsigned long)

__LVX_EXT_OP_RED(add, addurhod, __lvx_v8hu, __lvx_v8du, unsigned long)
__LVX_EXT_OP_RED(add, addurhpd, __lvx_v2hu, __lvx_v2du, unsigned long)
__LVX_EXT_OP_RED(add, addurhqd, __lvx_v4hu, __lvx_v4du, unsigned long)
__LVX_EXT_OP_RED(add, addurhvd, __lvx_v32hu, __lvx_v32du, unsigned long)
__LVX_EXT_OP_RED(add, addurhxd, __lvx_v16hu, __lvx_v16du, unsigned long)

__LVX_EXT_OP_RED(add, addurwod, __lvx_v8su, __lvx_v8du, unsigned long)
__LVX_EXT_OP_RED(add, addurwpd, __lvx_v2su, __lvx_v2du, unsigned long)
__LVX_EXT_OP_RED(add, addurwqd, __lvx_v4su, __lvx_v4du, unsigned long)
__LVX_EXT_OP_RED(add, addurwvd, __lvx_v32su, __lvx_v32du, unsigned long)
__LVX_EXT_OP_RED(add, addurwxd, __lvx_v16su, __lvx_v16du, unsigned long)

__LVX_OP_EXT_RED(and, andrbod, __lvx_v8qu, unsigned long)
__LVX_OP_EXT_RED(and, andrbpd, __lvx_v2qu, unsigned long)
__LVX_OP_EXT_RED(and, andrbqd, __lvx_v4qu, unsigned long)
__LVX_OP_EXT_RED(and, andrbvd, __lvx_v32qu, unsigned long)
__LVX_OP_EXT_RED(and, andrbxd, __lvx_v16qu, unsigned long)

__LVX_OP_EXT_RED(and, andrhod, __lvx_v8hu, unsigned long)
__LVX_OP_EXT_RED(and, andrhpd, __lvx_v2hu, unsigned long)
__LVX_OP_EXT_RED(and, andrhqd, __lvx_v4hu, unsigned long)
__LVX_OP_EXT_RED(and, andrhvd, __lvx_v32hu, unsigned long)
__LVX_OP_EXT_RED(and, andrhxd, __lvx_v16hu, unsigned long)

__LVX_OP_EXT_RED(and, andrwod, __lvx_v8su, unsigned long)
__LVX_OP_EXT_RED(and, andrwpd, __lvx_v2su, unsigned long)
__LVX_OP_EXT_RED(and, andrwqd, __lvx_v4su, unsigned long)
__LVX_OP_EXT_RED(and, andrwvd, __lvx_v32su, unsigned long)
__LVX_OP_EXT_RED(and, andrwxd, __lvx_v16su, unsigned long)

__LVX_OP_EXT_RED(max, maxrbod, __lvx_v8qi, long)
__LVX_OP_EXT_RED(max, maxrbpd, __lvx_v2qi, long)
__LVX_OP_EXT_RED(max, maxrbqd, __lvx_v4qi, long)
__LVX_OP_EXT_RED(max, maxrbvd, __lvx_v32qi, long)
__LVX_OP_EXT_RED(max, maxrbxd, __lvx_v16qi, long)

__LVX_OP_EXT_RED(max, maxrhod, __lvx_v8hi, long)
__LVX_OP_EXT_RED(max, maxrhpd, __lvx_v2hi, long)
__LVX_OP_EXT_RED(max, maxrhqd, __lvx_v4hi, long)
__LVX_OP_EXT_RED(max, maxrhvd, __lvx_v32hi, long)
__LVX_OP_EXT_RED(max, maxrhxd, __lvx_v16hi, long)

__LVX_OP_EXT_RED(max, maxrwod, __lvx_v8si, long)
__LVX_OP_EXT_RED(max, maxrwpd, __lvx_v2si, long)
__LVX_OP_EXT_RED(max, maxrwqd, __lvx_v4si, long)
__LVX_OP_EXT_RED(max, maxrwvd, __lvx_v32si, long)
__LVX_OP_EXT_RED(max, maxrwxd, __lvx_v16si, long)

__LVX_OP_EXT_RED(max, maxurbod, __lvx_v8qu, unsigned long)
__LVX_OP_EXT_RED(max, maxurbpd, __lvx_v2qu, unsigned long)
__LVX_OP_EXT_RED(max, maxurbqd, __lvx_v4qu, unsigned long)
__LVX_OP_EXT_RED(max, maxurbvd, __lvx_v32qu, unsigned long)
__LVX_OP_EXT_RED(max, maxurbxd, __lvx_v16qu, unsigned long)

__LVX_OP_EXT_RED(max, maxurhod, __lvx_v8hu, unsigned long)
__LVX_OP_EXT_RED(max, maxurhpd, __lvx_v2hu, unsigned long)
__LVX_OP_EXT_RED(max, maxurhqd, __lvx_v4hu, unsigned long)
__LVX_OP_EXT_RED(max, maxurhvd, __lvx_v32hu, unsigned long)
__LVX_OP_EXT_RED(max, maxurhxd, __lvx_v16hu, unsigned long)

__LVX_OP_EXT_RED(max, maxurwod, __lvx_v8su, unsigned long)
__LVX_OP_EXT_RED(max, maxurwpd, __lvx_v2su, unsigned long)
__LVX_OP_EXT_RED(max, maxurwqd, __lvx_v4su, unsigned long)
__LVX_OP_EXT_RED(max, maxurwvd, __lvx_v32su, unsigned long)
__LVX_OP_EXT_RED(max, maxurwxd, __lvx_v16su, unsigned long)

__LVX_OP_EXT_RED(min, minrbod, __lvx_v8qi, long)
__LVX_OP_EXT_RED(min, minrbpd, __lvx_v2qi, long)
__LVX_OP_EXT_RED(min, minrbqd, __lvx_v4qi, long)
__LVX_OP_EXT_RED(min, minrbvd, __lvx_v32qi, long)
__LVX_OP_EXT_RED(min, minrbxd, __lvx_v16qi, long)

__LVX_OP_EXT_RED(min, minrhod, __lvx_v8hi, long)
__LVX_OP_EXT_RED(min, minrhpd, __lvx_v2hi, long)
__LVX_OP_EXT_RED(min, minrhqd, __lvx_v4hi, long)
__LVX_OP_EXT_RED(min, minrhvd, __lvx_v32hi, long)
__LVX_OP_EXT_RED(min, minrhxd, __lvx_v16hi, long)

__LVX_OP_EXT_RED(min, minrwod, __lvx_v8si, long)
__LVX_OP_EXT_RED(min, minrwpd, __lvx_v2si, long)
__LVX_OP_EXT_RED(min, minrwqd, __lvx_v4si, long)
__LVX_OP_EXT_RED(min, minrwvd, __lvx_v32si, long)
__LVX_OP_EXT_RED(min, minrwxd, __lvx_v16si, long)

__LVX_OP_EXT_RED(min, minurbod, __lvx_v8qu, unsigned long)
__LVX_OP_EXT_RED(min, minurbpd, __lvx_v2qu, unsigned long)
__LVX_OP_EXT_RED(min, minurbqd, __lvx_v4qu, unsigned long)
__LVX_OP_EXT_RED(min, minurbvd, __lvx_v32qu, unsigned long)
__LVX_OP_EXT_RED(min, minurbxd, __lvx_v16qu, unsigned long)

__LVX_OP_EXT_RED(min, minurhod, __lvx_v8hu, unsigned long)
__LVX_OP_EXT_RED(min, minurhpd, __lvx_v2hu, unsigned long)
__LVX_OP_EXT_RED(min, minurhqd, __lvx_v4hu, unsigned long)
__LVX_OP_EXT_RED(min, minurhvd, __lvx_v32hu, unsigned long)
__LVX_OP_EXT_RED(min, minurhxd, __lvx_v16hu, unsigned long)

__LVX_OP_EXT_RED(min, minurwod, __lvx_v8su, unsigned long)
__LVX_OP_EXT_RED(min, minurwpd, __lvx_v2su, unsigned long)
__LVX_OP_EXT_RED(min, minurwqd, __lvx_v4su, unsigned long)
__LVX_OP_EXT_RED(min, minurwvd, __lvx_v32su, unsigned long)
__LVX_OP_EXT_RED(min, minurwxd, __lvx_v16su, unsigned long)

__LVX_OP_EXT_RED(or, orrbod, __lvx_v8qu, unsigned long)
__LVX_OP_EXT_RED(or, orrbpd, __lvx_v2qu, unsigned long)
__LVX_OP_EXT_RED(or, orrbqd, __lvx_v4qu, unsigned long)
__LVX_OP_EXT_RED(or, orrbvd, __lvx_v32qu, unsigned long)
__LVX_OP_EXT_RED(or, orrbxd, __lvx_v16qu, unsigned long)

__LVX_OP_EXT_RED(or, orrhod, __lvx_v8hu, unsigned long)
__LVX_OP_EXT_RED(or, orrhpd, __lvx_v2hu, unsigned long)
__LVX_OP_EXT_RED(or, orrhqd, __lvx_v4hu, unsigned long)
__LVX_OP_EXT_RED(or, orrhvd, __lvx_v32hu, unsigned long)
__LVX_OP_EXT_RED(or, orrhxd, __lvx_v16hu, unsigned long)

__LVX_OP_EXT_RED(or, orrwod, __lvx_v8su, unsigned long)
__LVX_OP_EXT_RED(or, orrwpd, __lvx_v2su, unsigned long)
__LVX_OP_EXT_RED(or, orrwqd, __lvx_v4su, unsigned long)
__LVX_OP_EXT_RED(or, orrwvd, __lvx_v32su, unsigned long)
__LVX_OP_EXT_RED(or, orrwxd, __lvx_v16su, unsigned long)

__LVX_OP_EXT_RED(xor, xorrbod, __lvx_v8qu, unsigned long)
__LVX_OP_EXT_RED(xor, xorrbpd, __lvx_v2qu, unsigned long)
__LVX_OP_EXT_RED(xor, xorrbqd, __lvx_v4qu, unsigned long)
__LVX_OP_EXT_RED(xor, xorrbvd, __lvx_v32qu, unsigned long)
__LVX_OP_EXT_RED(xor, xorrbxd, __lvx_v16qu, unsigned long)

__LVX_OP_EXT_RED(xor, xorrhod, __lvx_v8hu, unsigned long)
__LVX_OP_EXT_RED(xor, xorrhpd, __lvx_v2hu, unsigned long)
__LVX_OP_EXT_RED(xor, xorrhqd, __lvx_v4hu, unsigned long)
__LVX_OP_EXT_RED(xor, xorrhvd, __lvx_v32hu, unsigned long)
__LVX_OP_EXT_RED(xor, xorrhxd, __lvx_v16hu, unsigned long)

__LVX_OP_EXT_RED(xor, xorrwod, __lvx_v8su, unsigned long)
__LVX_OP_EXT_RED(xor, xorrwpd, __lvx_v2su, unsigned long)
__LVX_OP_EXT_RED(xor, xorrwqd, __lvx_v4su, unsigned long)
__LVX_OP_EXT_RED(xor, xorrwvd, __lvx_v32su, unsigned long)
__LVX_OP_EXT_RED(xor, xorrwxd, __lvx_v16su, unsigned long)

#undef __LVX_EXT_OP_RED
#undef __LVX_OP_EXT_RED

#define __builtin_lvx_fadddcq(a, b, mod)                                       \
  __split_binmod_op8(a, b, mod, fadddcp, __lvx_v8df)

#define __builtin_lvx_fconjdcq(a)                                              \
  __builtin_shufflevector(a, -a, 0, 9, 2, 11, 4, 13, 6, 15)

#define __builtin_lvx_ffmadcq(a, b, c, mod)                                    \
  __split_termod_op8(a, b, c, mod, ffmadcp, __lvx_v8df)

#define __builtin_lvx_ffmsdcq(a, b, c, mod)                                    \
  __split_termod_op8(a, b, c, mod, ffmsdcp, __lvx_v8df)

#define __builtin_lvx_fmuldcq(a, b, mod)                                       \
  __split_binmod_op8(a, b, mod, fmuldcp, __lvx_v8df)

#define __builtin_lvx_fsbfdcq(a, b, mod)                                       \
  __split_binmod_op8(a, b, mod, fsbfdcp, __lvx_v8df)

inline __lvx_v4si __builtin_lvx_maddwq(__lvx_v4si acc, __lvx_v4si a,
                                       __lvx_v4si b) {
  return acc + a * b;
}

inline __lvx_v4si __builtin_lvx_maddmwq(__lvx_v4si acc, __lvx_v4si a,
                                        __lvx_v4si b) {
  __lvx_v4di ea = __builtin_convertvector(a, __lvx_v4di);
  __lvx_v4di eb = __builtin_convertvector(b, __lvx_v4di);
  __lvx_v4di mul = (ea * eb) >> 32;
  __lvx_v4si trunc = __builtin_convertvector(mul, __lvx_v4si);
  return acc + trunc;
}

inline __lvx_v4su __builtin_lvx_maddumwq(__lvx_v4su acc, __lvx_v4su a,
                                         __lvx_v4su b) {
  __lvx_v4du ea = __builtin_convertvector(a, __lvx_v4du);
  __lvx_v4du eb = __builtin_convertvector(b, __lvx_v4du);
  __lvx_v4du mul = (ea * eb) >> 32;
  __lvx_v4su trunc = __builtin_convertvector(mul, __lvx_v4su);
  return acc + trunc;
}

inline __lvx_v4si __builtin_lvx_mulmwq(__lvx_v4si a, __lvx_v4si b) {
  __lvx_v4di ea = __builtin_convertvector(a, __lvx_v4di);
  __lvx_v4di eb = __builtin_convertvector(b, __lvx_v4di);
  __lvx_v4di mul = (ea * eb) >> 32;
  return __builtin_convertvector(mul, __lvx_v4si);
}

inline __lvx_v4su __builtin_lvx_mulumwq(__lvx_v4su a, __lvx_v4su b) {
  __lvx_v4du ea = __builtin_convertvector(a, __lvx_v4du);
  __lvx_v4du eb = __builtin_convertvector(b, __lvx_v4du);
  __lvx_v4du mul = (ea * eb) >> 32;
  return __builtin_convertvector(mul, __lvx_v4su);
}

inline __lvx_v4si __builtin_lvx_msbfmwq(__lvx_v4si acc, __lvx_v4si a,
                                        __lvx_v4si b) {
  __lvx_v4di ea = __builtin_convertvector(a, __lvx_v4di);
  __lvx_v4di eb = __builtin_convertvector(b, __lvx_v4di);
  __lvx_v4di mul = (ea * eb) >> 32;
  __lvx_v4si trunc = __builtin_convertvector(mul, __lvx_v4si);
  return acc - trunc;
}

inline __lvx_v4su __builtin_lvx_msbfumwq(__lvx_v4su acc, __lvx_v4su a,
                                         __lvx_v4su b) {
  __lvx_v4du ea = __builtin_convertvector(a, __lvx_v4du);
  __lvx_v4du eb = __builtin_convertvector(b, __lvx_v4du);
  __lvx_v4du mul = (ea * eb) >> 32;
  __lvx_v4su trunc = __builtin_convertvector(mul, __lvx_v4su);
  return acc - trunc;
}

inline __lvx_v4si __builtin_lvx_msbfwq(__lvx_v4si acc, __lvx_v4si a,
                                       __lvx_v4si b) {
  return acc - a * b;
}

inline __lvx_v4si __builtin_lvx_maddsumwq(__lvx_v4si acc, __lvx_v4si a,
                                          __lvx_v4su b) {
  __lvx_v4di ea = __builtin_convertvector(a, __lvx_v4di);
  __lvx_v4du eb = __builtin_convertvector(b, __lvx_v4du);
  __lvx_v4di mul = (ea * eb) >> 32;
  __lvx_v4si trunc = __builtin_convertvector(mul, __lvx_v4si);
  return acc + trunc;
}

inline __lvx_v4si __builtin_lvx_msbfsumwq(__lvx_v4si acc, __lvx_v4si a,
                                          __lvx_v4su b) {
  __lvx_v4di ea = __builtin_convertvector(a, __lvx_v4di);
  __lvx_v4du eb = __builtin_convertvector(b, __lvx_v4du);
  __lvx_v4di mul = (ea * eb) >> 32;
  __lvx_v4si trunc = __builtin_convertvector(mul, __lvx_v4si);
  return acc - trunc;
}

inline __lvx_v4si __builtin_lvx_mulsumwq(__lvx_v4si a, __lvx_v4su b) {
  __lvx_v4di ea = __builtin_convertvector(a, __lvx_v4di);
  __lvx_v4du eb = __builtin_convertvector(b, __lvx_v4du);
  __lvx_v4di mul = (ea * eb) >> 32;
  return __builtin_convertvector(mul, __lvx_v4si);
}

#define __builtin_lvx_xpreloado512(a, b, c, d)                                 \
  __builtin_lvx_xpreload512(a, b, c, d)
#define __builtin_lvx_xpreloado1024(a, b, c, d)                                \
  __builtin_lvx_xpreload1024(a, b, c, d)
#define __builtin_lvx_xpreloado2048(a, b, c, d)                                \
  __builtin_lvx_xpreload2048(a, b, c, d)
#define __builtin_lvx_xpreloado4096(a, b, c, d)                                \
  __builtin_lvx_xpreload4096(a, b, c, d)

#define __builtin_lvx_xpreloadq512(a, b, c, d)                                 \
  __builtin_lvx_xpreload512(a, b, c, d ".q")
#define __builtin_lvx_xpreloadq1024(a, b, c, d)                                \
  __builtin_lvx_xpreload1024(a, b, c, d ".q")
#define __builtin_lvx_xpreloadq2048(a, b, c, d)                                \
  __builtin_lvx_xpreload2048(a, b, c, d ".q")
#define __builtin_lvx_xpreloadq4096(a, b, c, d)                                \
  __builtin_lvx_xpreload4096(a, b, c, d ".q")

#define __builtin_lvx_xpreloadd512(a, b, c, d)                                 \
  __builtin_lvx_xpreload512(a, b, c, d ".d")
#define __builtin_lvx_xpreloadd1024(a, b, c, d)                                \
  __builtin_lvx_xpreload1024(a, b, c, d ".d")
#define __builtin_lvx_xpreloadd2048(a, b, c, d)                                \
  __builtin_lvx_xpreload2048(a, b, c, d ".d")
#define __builtin_lvx_xpreloadd4096(a, b, c, d)                                \
  __builtin_lvx_xpreload4096(a, b, c, d ".d")

#define __builtin_lvx_xpreloads512(a, b, c, d)                                 \
  __builtin_lvx_xpreload512(a, b, c, d ".s")
#define __builtin_lvx_xpreloads1024(a, b, c, d)                                \
  __builtin_lvx_xpreload1024(a, b, c, d ".s")
#define __builtin_lvx_xpreloads2048(a, b, c, d)                                \
  __builtin_lvx_xpreload2048(a, b, c, d ".s")
#define __builtin_lvx_xpreloads4096(a, b, c, d)                                \
  __builtin_lvx_xpreload4096(a, b, c, d ".s")

#define __builtin_lvx_xpreloadh512(a, b, c, d)                                 \
  __builtin_lvx_xpreload512(a, b, c, d ".h")
#define __builtin_lvx_xpreloadh1024(a, b, c, d)                                \
  __builtin_lvx_xpreload1024(a, b, c, d ".h")
#define __builtin_lvx_xpreloadh2048(a, b, c, d)                                \
  __builtin_lvx_xpreload2048(a, b, c, d ".h")
#define __builtin_lvx_xpreloadh4096(a, b, c, d)                                \
  __builtin_lvx_xpreload4096(a, b, c, d ".h")

#define __builtin_lvx_xpreloadb512(a, b, c, d)                                 \
  __builtin_lvx_xpreload512(a, b, c, d ".b")
#define __builtin_lvx_xpreloadb1024(a, b, c, d)                                \
  __builtin_lvx_xpreload1024(a, b, c, d ".b")
#define __builtin_lvx_xpreloadb2048(a, b, c, d)                                \
  __builtin_lvx_xpreload2048(a, b, c, d ".b")
#define __builtin_lvx_xpreloadb4096(a, b, c, d)                                \
  __builtin_lvx_xpreload4096(a, b, c, d ".b")

#define __builtin_lvx_xaccessd512(x, y) __builtin_lvx_xaccesso512(x, y)[0]
#define __builtin_lvx_xaccessd1024(x, y) __builtin_lvx_xaccesso1024(x, y)[0]
#define __builtin_lvx_xaccessd2048(x, y) __builtin_lvx_xaccesso2048(x, y)[0]
#define __builtin_lvx_xaccessd4096(x, y) __builtin_lvx_xaccesso4096(x, y)[0]

#define __builtin_lvx_xaccessq512(x, y)                                        \
  (__lvx_v2di) {                                                               \
    __builtin_lvx_xaccesso512(x, y)[0], __builtin_lvx_xaccesso512(x, y)[1]     \
  }
#define __builtin_lvx_xaccessq1024(x, y)                                       \
  (__lvx_v2di) {                                                               \
    __builtin_lvx_xaccesso1024(x, y)[0], __builtin_lvx_xaccesso1024(x, y)[1]   \
  }
#define __builtin_lvx_xaccessq2048(x, y)                                       \
  (__lvx_v2di) {                                                               \
    __builtin_lvx_xaccesso2048(x, y)[0], __builtin_lvx_xaccesso2048(x, y)[1]   \
  }
#define __builtin_lvx_xaccessq4096(x, y)                                       \
  (__lvx_v2di) {                                                               \
    __builtin_lvx_xaccesso4096(x, y)[0], __builtin_lvx_xaccesso4096(x, y)[1]   \
  }

typedef long long int __lvx_v64;
#endif /* defined(__llvm__) */
#endif /* __H__LVX_BUILTINS__ */
