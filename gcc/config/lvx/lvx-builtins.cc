/* Builtin function support for the LVX architecture.
   Copyright (C) 2011-2022 Free Software Foundation, Inc.
   Contributed by Kalray SA (bddinechin@kalray.eu).

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#define IN_TARGET_CODE 1

#include "config.h"
#define INCLUDE_STRING
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "function.h"
#include "basic-block.h"
#include "rtl.h"
#include "tree.h"
#include "gimple.h"
#include "memmodel.h"
#include "tm_p.h"
#include "expmed.h"
#include "optabs.h"
#include "recog.h"
#include "diagnostic-core.h"
#include "fold-const.h"
#include "stor-layout.h"
#include "explow.h"
#include "expr.h"
#include "langhooks.h"
#include "emit-rtl.h"
#include "calls.h"

/* Some useful meta-programming macros.  */

#define EMPTY()
#define DEFER(id) id EMPTY()
#define OBSTRUCT(...) __VA_ARGS__ DEFER(EMPTY)()
#define EXPAND(...) __VA_ARGS__

#define LVX_FST(A, ...) A
#define LVX_RST(A, ...) __VA_ARGS__

#define CONCAT2_(a,b)          a ## b
#define CONCAT3_(a,b,c)        a ## b ## c
#define CONCAT4_(a,b,c,d)      a ## b ## c ## d
#define QUOTE(a)               #a
#define PROTECT(a)             QUOTE (a)
#define CONCAT2(a,b)           CONCAT2_ (a,b)
#define CONCAT3(a,b,c)         CONCAT3_ (a,b,c)
#define CONCAT4(a,b,c,d)       CONCAT4_ (a,b,c,d)

#define GET_MACRO(_1, _2, _3, _4, _5, _6, x,...) x

#define _ARG16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, ...) _15
#define HAS_COMMA(...) _ARG16(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define _TRIGGER_PARENTHESIS_(...) ,

/* This implementation of ISEMPTY comes from:
   https://gustedt.wordpress.com/2010/06/08/detect-empty-macro-arguments */
#define ISEMPTY(...)                                                    \
_ISEMPTY(                                                               \
          /* test if there is just one argument, eventually an empty    \
             one */                                                     \
          HAS_COMMA(__VA_ARGS__),                                       \
          /* test if _TRIGGER_PARENTHESIS_ together with the argument   \
             adds a comma */                                            \
          HAS_COMMA(_TRIGGER_PARENTHESIS_ __VA_ARGS__),                 \
          /* test if the argument together with a parenthesis           \
             adds a comma */                                            \
          HAS_COMMA(__VA_ARGS__ (/*empty*/)),                           \
          /* test if placing it between _TRIGGER_PARENTHESIS_ and the   \
             parenthesis adds a comma */                                \
          HAS_COMMA(_TRIGGER_PARENTHESIS_ __VA_ARGS__ (/*empty*/))      \
          )

#define PASTE5(_0, _1, _2, _3, _4) _0 ## _1 ## _2 ## _3 ## _4
#define _ISEMPTY(_0, _1, _2, _3) HAS_COMMA(PASTE5(_IS_EMPTY_CASE_, _0, _1, _2, _3))
#define _IS_EMPTY_CASE_0001 ,

/* Macros to handle the optional modifier of some builtins.  */

#define LVX_OPT_
#define LVX_OPT__
#define LVX_OPT_ACCESSES  _ACCESSES
#define LVX_OPT_BOOL      _BOOL
#define LVX_OPT_COHERENCY _COHERENCY
#define LVX_OPT_BOOLCASCO _BOOLCASCO
#define LVX_OPT_BOOLCASCO_BOOL _BOOLCASCO_BOOL
#define LVX_GET(OPTIONALS) LVX_OPT_ ## OPTIONALS

struct lvx_modmap
{
  /* The list of valid modifiers in the C source.  */
  const char **imap;

  /* A list mapping IMAP modifiers to OMAP modifiers, i.e., those that will be
   * used by the Machine Description Files or the assembler. */
  const char **omap;
};

/* Accessors for the modifier maps, this is needed by the
 * LVX_BUILTIN_{IN,}DIRECT macros.  */

#define LVX_MOD_VOID       NULL
#define LVX_MOD_VPTR       NULL
#define LVX_MOD_CVPTR      NULL
#define LVX_MOD_BOOL       NULL

#define LVX_MOD_INT8       NULL
#define LVX_MOD_INT16      NULL
#define LVX_MOD_INT32      NULL
#define LVX_MOD_INT64      NULL
#define LVX_MOD_INT128     NULL

#define LVX_MOD_UINT8      NULL
#define LVX_MOD_UINT16     NULL
#define LVX_MOD_UINT32     NULL
#define LVX_MOD_UINT64     NULL
#define LVX_MOD_UINT128    NULL

#define LVX_MOD_FLOAT16    NULL
#define LVX_MOD_FLOAT32    NULL
#define LVX_MOD_FLOAT64    NULL
#define LVX_MOD_COMPLEX64  NULL
#define LVX_MOD_COMPLEX128 NULL

#define LVX_MOD_STRING     NULL

#define LVX_MOD_V8QI       NULL
#define LVX_MOD_V16QI      NULL
#define LVX_MOD_V32QI      NULL
#define LVX_MOD_V64QI      NULL

#define LVX_MOD_V4HI       NULL
#define LVX_MOD_V8HI       NULL
#define LVX_MOD_V16HI      NULL
#define LVX_MOD_V32HI      NULL

#define LVX_MOD_V2SI       NULL
#define LVX_MOD_V4SI       NULL
#define LVX_MOD_V8SI       NULL
#define LVX_MOD_V16SI      NULL

#define LVX_MOD_V2DI       NULL
#define LVX_MOD_V4DI       NULL
#define LVX_MOD_V8DI       NULL

#define LVX_MOD_V4HF       NULL
#define LVX_MOD_V8HF       NULL
#define LVX_MOD_V16HF      NULL
#define LVX_MOD_V32HF      NULL

#define LVX_MOD_V2SF       NULL
#define LVX_MOD_V4SF       NULL
#define LVX_MOD_V8SF       NULL
#define LVX_MOD_V16SF      NULL

#define LVX_MOD_V2DF       NULL
#define LVX_MOD_V4DF       NULL
#define LVX_MOD_V8DF       NULL

#define LVX_MOD_DI         NULL
#define LVX_MOD_V64        NULL
#define LVX_MOD_V128       NULL
#define LVX_MOD_V256       NULL
#define LVX_MOD_V512       NULL

#define LVX_MOD_OI         NULL

#define LVX_MOD_X256       NULL
#define LVX_MOD_X512       NULL
#define LVX_MOD_X1024      NULL
#define LVX_MOD_X2048      NULL
#define LVX_MOD_X4096      NULL
#define LVX_MOD_X8192      NULL

#define LVX_MOD__X256      NULL

#define LVX_MOD_ABDSATUNS  lvx_modmap_abdsatuns
#define LVX_MOD_ACCESSES   lvx_modmap_accesses
#define LVX_MOD_AVERAGE    lvx_modmap_average
#define LVX_MOD_BOOLCASCO  lvx_modmap_boolcasco
#define LVX_MOD_CACHELEV   lvx_modmap_cachelev
#define LVX_MOD_COHERENCY  lvx_modmap_coherency
#define LVX_MOD_CONJUGATE  lvx_modmap_conjugate
#define LVX_MOD_COUNTING   lvx_modmap_counting
#define LVX_MOD_EXTENDMUL  lvx_modmap_extendmul
#define LVX_MOD_FLOATINGS  lvx_modmap_floatings
#define LVX_MOD_LANEWISE   lvx_modmap_lanewise
#define LVX_MOD_LOADCOND   lvx_modmap_loadcond
#define LVX_MOD_MOVECOND   lvx_modmap_movecond
#define LVX_MOD_MULXDT     lvx_modmap_mulxdt
#define LVX_MOD_NARROWINT  lvx_modmap_narrowint
#define LVX_MOD_SATURATE   lvx_modmap_saturate
#define LVX_MOD_SHIFTLEFT  lvx_modmap_shiftleft
#define LVX_MOD_SHIFTRIGHT lvx_modmap_shiftright
#define LVX_MOD_MOSTSIG    lvx_modmap_mostsig
#define LVX_MOD_SIGNEDSAT  lvx_modmap_signedsat
#define LVX_MOD_SIMDCOND   lvx_modmap_simdcond
#define LVX_MOD_SIMDCONDD  lvx_modmap_simdcondd
#define LVX_MOD_STORECOND  lvx_modmap_storecond
#define LVX_MOD_TRANSPOSE  lvx_modmap_transpose
#define LVX_MOD_UNUSED     lvx_modmap_unused
#define LVX_MOD_VARIANT    lvx_modmap_variant
#define LVX_MOD_WIDENINT   lvx_modmap_widenint
#define LVX_MOD_XCHANNEL   lvx_modmap_xchannel
#define LVX_MOD_XCHANNELS  lvx_modmap_xchannels
#define LVX_MOD_XHALF      lvx_modmap_xhalf
#define LVX_MOD_XLOADCOND  lvx_modmap_xloadcond
#define LVX_MOD_XLOADHC    lvx_modmap_xloadhc
#define LVX_MOD_XLOADH     lvx_modmap_xloadh
#define LVX_MOD_XLOADQC    lvx_modmap_xloadqc
#define LVX_MOD_XLOADQ     lvx_modmap_xloadq
#define LVX_MOD_XMATMUL    lvx_modmap_xmatmul
#define LVX_MOD_XPRELOAD   lvx_modmap_xpreload
#define LVX_MOD_XQUARTER   lvx_modmap_xquarter
#define LVX_MOD_XSHUFFLEV  lvx_modmap_xshufflev
#define LVX_MOD_XSHUFFLEX  lvx_modmap_xshufflex
#define LVX_MOD_XVARIANT   lvx_modmap_xvariant

const char *lvx_mod_abdsatuns[] = {
 "", ".s", ".u", NULL
};

const struct lvx_modmap lvx_modmap_abdsatuns_all = {
  lvx_mod_abdsatuns,
  lvx_mod_abdsatuns
};

const struct lvx_modmap *lvx_modmap_abdsatuns = &lvx_modmap_abdsatuns_all;


const char *lvx_mod_accesses_lvx_1[] = {
 "", ".w", ".r", ".wa", NULL
};
const struct lvx_modmap lvx_modmap_accesses_lvx_1 = {
  lvx_mod_accesses_lvx_1,
  lvx_mod_accesses_lvx_1
};

const struct lvx_modmap *lvx_modmap_accesses = NULL;

const char *lvx_mod_average[] = {
 "", ".r", ".u", ".ru", NULL
};

const struct lvx_modmap lvx_modmap_average_all = {
  lvx_mod_average,
  lvx_mod_average
};

const struct lvx_modmap *lvx_modmap_average = &lvx_modmap_average_all;

const char *lvx_mod_boolcasco_lvx_1[] = {
 "", ".g", ".s",
 ".v", ".v.g", ".v.s",
 NULL
};
const struct lvx_modmap lvx_modmap_boolcasco_lvx_1 = {
  lvx_mod_boolcasco_lvx_1,
  lvx_mod_boolcasco_lvx_1
};
const struct lvx_modmap *lvx_modmap_boolcasco = NULL;

const char *lvx_mod_cachelev_lvx_1[] = {
 ".l1", ".l2", NULL
};
const struct lvx_modmap lvx_modmap_cachelev_lvx_1 = {
  lvx_mod_cachelev_lvx_1,
  lvx_mod_cachelev_lvx_1
};
const struct lvx_modmap *lvx_modmap_cachelev = NULL;

const char *lvx_mod_coherency[] = {
 "", ".g", ".s", NULL
};
const struct lvx_modmap lvx_modmap_coherency_all = {
  lvx_mod_coherency, lvx_mod_coherency
};
const struct lvx_modmap *lvx_modmap_coherency = &lvx_modmap_coherency_all;

/* MDS Modifier.yml, ID: conjugate -- members [ ., .C ].  This used to hold
   the rounding modes, which belong to the floatmode modifier: the assembler
   takes either on FMULWC, so the mistake was invisible, but it made ".c" --
   the whole point of a conjugate builtin -- "modifier not recognized".  */
const char *lvx_mod_conjugate_lvx_1_in[] = {
 "", ".c", NULL
};
const char *lvx_mod_conjugate_lvx_1_out[] = {
 "", ".c", NULL
};
const struct lvx_modmap lvx_modmap_conjugate_lvx_1 = {
  lvx_mod_conjugate_lvx_1_in, lvx_mod_conjugate_lvx_1_out
};
const struct lvx_modmap *lvx_modmap_conjugate;

const char *lvx_mod_counting[] = {
 "", ".lz", ".ls", ".tz", NULL
};
const struct lvx_modmap lvx_modmap_counting_all = {
  lvx_mod_counting, lvx_mod_counting
};
const struct lvx_modmap *lvx_modmap_counting = &lvx_modmap_counting_all;

const char *lvx_mod_extendmul[] = {
 "", ".u", ".su", NULL
};
const struct lvx_modmap lvx_modmap_extendmul_all = {
  lvx_mod_extendmul, lvx_mod_extendmul
};
const struct lvx_modmap *lvx_modmap_extendmul = &lvx_modmap_extendmul_all;

const char *lvx_mod_floatings_all_in[] = {
 "", ".rn", ".ru", ".rd", ".rz", NULL
};

const char *lvx_mod_floatings_lvx_1_out[] = {
 "", ".rn", ".ru", ".rd", ".rz", NULL
};
const struct lvx_modmap lvx_modmap_floatings_lvx_1 = {
  lvx_mod_floatings_all_in, lvx_mod_floatings_lvx_1_out
};
const struct lvx_modmap *lvx_modmap_floatings = NULL;

const char *lvx_mod_lanewise[] = {
 ".nez", ".eqz", NULL
};
const struct lvx_modmap lvx_modmap_lanewise_all = {
  lvx_mod_lanewise, lvx_mod_lanewise
};
const struct lvx_modmap *lvx_modmap_lanewise = &lvx_modmap_lanewise_all;


const char *lvx_mod_loadcond_lvx_1[] = {
  /* LO and XLO */
  ".dnez", ".deqz", ".wnez", ".weqz", ".mt", ".mf", ".mtc", ".mfc",
  ".s.dnez", ".s.deqz", ".s.wnez", ".s.weqz", ".s.mt", ".s.mf", ".s.mtc", ".s.mfc",
  ".u.dnez", ".u.deqz", ".u.wnez", ".u.weqz", ".u.mt", ".u.mf", ".u.mtc", ".u.mfc",
  ".us.dnez", ".us.deqz", ".us.wnez", ".us.weqz", ".us.mt", ".us.mf", ".us.mtc", ".us.mfc",
  NULL
};
const struct lvx_modmap lvx_modmap_loadcond_lvx_1 = {
  lvx_mod_loadcond_lvx_1, lvx_mod_loadcond_lvx_1
};
const struct lvx_modmap *lvx_modmap_loadcond = NULL;


const char *lvx_mod_movecond[] = {
 ".wnez", ".weqz", ".wltz", ".wgez", ".wlez", ".wgtz", ".odd", ".even",
 ".dnez", ".deqz", ".dltz", ".dgez", ".dlez", ".dgtz", NULL
};
const struct lvx_modmap lvx_modmap_movecond_all = {
  lvx_mod_movecond, lvx_mod_movecond
};
const struct lvx_modmap *lvx_modmap_movecond = &lvx_modmap_movecond_all;

const char *lvx_mod_mulxdt[] = {
 "", ".u", ".su", ".uz", NULL
};
const struct lvx_modmap lvx_modmap_mulxdt_all = {
  lvx_mod_mulxdt, lvx_mod_mulxdt
};
const struct lvx_modmap *lvx_modmap_mulxdt = &lvx_modmap_mulxdt_all;

const char *lvx_mod_narrowint[] = {
 "", ".q", ".s", ".us", NULL
};
const struct lvx_modmap lvx_modmap_narrowint_all = {
  lvx_mod_narrowint, lvx_mod_narrowint
};
const struct lvx_modmap *lvx_modmap_narrowint = &lvx_modmap_narrowint_all;

const char *lvx_mod_saturate[] = {
 "", ".s", ".us", NULL
};
const struct lvx_modmap lvx_modmap_saturate_all = {
  lvx_mod_saturate, lvx_mod_saturate
};
const struct lvx_modmap *lvx_modmap_saturate = &lvx_modmap_saturate_all;

const char *lvx_mod_shiftleft[] = {
 "", ".s", ".us", ".r", NULL
};
const struct lvx_modmap lvx_modmap_shiftleft_all = {
  lvx_mod_shiftleft, lvx_mod_shiftleft
};
const struct lvx_modmap *lvx_modmap_shiftleft = &lvx_modmap_shiftleft_all;

const char *lvx_mod_shiftright[] = {
 "", ".a", ".as", ".r", NULL
};
const struct lvx_modmap lvx_modmap_shiftright_all = {
  lvx_mod_shiftright, lvx_mod_shiftright
};
const struct lvx_modmap *lvx_modmap_shiftright = &lvx_modmap_shiftright_all;

const char *lvx_mod_signedsat[] = {
 "", ".s", NULL
};
const struct lvx_modmap lvx_modmap_signedsat_all = {
  lvx_mod_signedsat, lvx_mod_signedsat
};
const struct lvx_modmap *lvx_modmap_signedsat = &lvx_modmap_signedsat_all;


const char *lvx_mod_mostsig[] = {
 "", ".m", NULL
};
const struct lvx_modmap lvx_modmap_mostsig_all = {
  lvx_mod_mostsig, lvx_mod_mostsig
};
const struct lvx_modmap *lvx_modmap_mostsig = &lvx_modmap_mostsig_all;

const char *lvx_mod_simdcond[] = {
 ".nez", ".eqz", ".ltz", ".gez", ".lez", ".gtz", ".odd", ".even", NULL
};
const struct lvx_modmap lvx_modmap_simdcond_all = {
  lvx_mod_simdcond, lvx_mod_simdcond
};
const struct lvx_modmap *lvx_modmap_simdcond = &lvx_modmap_simdcond_all;

const char *lvx_mod_simdcondd[] = {
    ".dnez", ".deqz", ".dltz", ".dgez", ".dlez", ".dgtz", ".odd", ".even", NULL
};
const struct lvx_modmap lvx_modmap_simdcondd_all = {
  lvx_mod_simdcond, lvx_mod_simdcondd
};
const struct lvx_modmap *lvx_modmap_simdcondd = &lvx_modmap_simdcondd_all;

const char *lvx_mod_storecond_lvx_1[] = {
  /* Exactly the members of the MDS Modifier-lvx-bcucond, which is what the
     GUARD that carries the condition takes.  There used to be a second "SO and
     XSO" group here repeating four of these and adding ".mt" and ".mf"; the
     repeats were unreachable and no modifier in the description has mt/mf.  */
  ".dnez", ".deqz", ".dltz", ".dgez", ".dlez", ".dgtz", ".odd",
  ".even", ".wnez", ".weqz", ".wltz", ".wgez", ".wlez", ".wgtz",
  NULL
};
const struct lvx_modmap lvx_modmap_storecond_lvx_1 = {
  lvx_mod_storecond_lvx_1, lvx_mod_storecond_lvx_1,
};
const struct lvx_modmap *lvx_modmap_storecond = NULL;

const char *lvx_mod_transpose_all_in[] = {
  "", ".tn", ".nt", ".tt", ".nn",
  ".tn.rn", ".nt.rn", ".tt.rn", ".nn.rn",
  ".rd", ".tn.rd", ".nt.rd", ".tt.rd", ".nn.rd",
  ".ru", ".tn.ru", ".nt.ru", ".tt.ru", ".nn.ru",
  ".rz", ".tn.rz", ".nt.rz", ".tt.rz", ".nn.rz",
  ".rn", ".tn.rn", ".nt.rn", ".tt.rn", ".nn.rn",
  NULL
};
const char *lvx_mod_transpose_all_out[] = {
  "", ".tn", ".nt", ".tt", "",
  ".tn.rn", ".nt.rn", ".tt.rn", ".rn",
  ".rd", ".tn.rd", ".nt.rd", ".tt.rd", ".rd",
  ".ru", ".tn.ru", ".nt.ru", ".tt.ru", ".ru",
  ".rz", ".tn.rz", ".nt.rz", ".tt.rz", ".rz",
  ".rn", ".tn.rn", ".nt.rn", ".tt.rn", ".rn",
  NULL
};
const struct lvx_modmap lvx_modmap_transpose_all = {
  lvx_mod_transpose_all_in, lvx_mod_transpose_all_out
};
const struct lvx_modmap *lvx_modmap_transpose = &lvx_modmap_transpose_all;

const char *lvx_mod_variant[] = {
 "", ".s", ".u", ".us", NULL
};
const struct lvx_modmap lvx_modmap_variant_all = {
  lvx_mod_variant, lvx_mod_variant
};
const struct lvx_modmap *lvx_modmap_variant = &lvx_modmap_variant_all;

const char *lvx_mod_widenint[] = {
 "", ".z", ".q", NULL
};
const struct lvx_modmap lvx_modmap_widenint_all = {
  lvx_mod_widenint, lvx_mod_widenint
};
const struct lvx_modmap *lvx_modmap_widenint = &lvx_modmap_widenint_all;

const char *lvx_mod_xchannel[] = {
 ".f", ".b", NULL
};
const struct lvx_modmap lvx_modmap_xchannel_all = {
  lvx_mod_xchannel, lvx_mod_xchannel
};
const struct lvx_modmap *lvx_modmap_xchannel = &lvx_modmap_xchannel_all;

const char *lvx_mod_xchannels[] = {
 ".f.f", ".f.b", ".b.f", ".b.b", NULL
};
const struct lvx_modmap lvx_modmap_xchannels_all = {
  lvx_mod_xchannels, lvx_mod_xchannels
};
const struct lvx_modmap *lvx_modmap_xchannels = &lvx_modmap_xchannels_all;

const char *lvx_mod_xloadcond_lvx_1[] = {
  // LD, LQ, LO, XLO
  ".dnez", ".deqz", ".dltz", ".dgez", ".dlez", ".dgtz", ".odd",
  ".even", ".wnez", ".weqz", ".wltz", ".wgez", ".wlez", ".wgtz",
  ".s.dnez", ".s.deqz", ".s.dltz", ".s.dgez", ".s.dlez", ".s.dgtz", ".s.odd",
  ".s.even", ".s.wnez", ".s.weqz", ".s.wltz", ".s.wgez", ".s.wlez", ".s.wgtz",
  ".u.dnez", ".u.deqz", ".u.dltz", ".u.dgez", ".u.dlez", ".u.dgtz", ".u.odd",
  ".u.even", ".u.wnez", ".u.weqz", ".u.wltz", ".u.wgez", ".u.wlez", ".u.wgtz",
  ".us.dnez", ".us.deqz", ".us.dltz", ".us.dgez", ".us.dlez", ".us.dgtz", ".us.odd",
  ".us.even", ".us.wnez", ".us.weqz", ".us.wltz", ".us.wgez", ".us.wlez", ".us.wgtz",
  // LO and XLO
  ".dnez", ".deqz", ".wnez", ".weqz", ".mt", ".mf", ".mtc", ".mfc",
  ".s.dnez", ".s.deqz", ".s.wnez", ".s.weqz", ".s.mt", ".s.mf", ".s.mtc", ".s.mfc",
  ".u.dnez", ".u.deqz", ".u.wnez", ".u.weqz", ".u.mt", ".u.mf", ".u.mtc", ".u.mfc",
  ".us.dnez", ".us.deqz", ".us.wnez", ".us.weqz", ".us.mt", ".us.mf", ".us.mtc", ".us.mfc",
  NULL
};
const struct lvx_modmap lvx_modmap_xloadcond_lvx_1 = {
  lvx_mod_xloadcond_lvx_1, lvx_mod_xloadcond_lvx_1,
};
const struct lvx_modmap *lvx_modmap_xloadcond = NULL;

/* This is remapped to .lo .hi */
const char *lvx_mod_xhalf_all_in[] = {
 ".h0", ".h1", NULL
};
const char *lvx_mod_xhalf_all_out[] = {
 ".lo", ".hi", NULL
};
const struct lvx_modmap lvx_modmap_xhalf_all = {
  lvx_mod_xhalf_all_in, lvx_mod_xhalf_all_out
};
const struct lvx_modmap *lvx_modmap_xhalf = &lvx_modmap_xhalf_all;

const char *lvx_mod_xloadqc_lvx_1[] = {
    ".dnez.q0", ".deqz.q0", ".wnez.q0", ".weqz.q0",
    ".s.dnez.q0", ".s.deqz.q0", ".s.wnez.q0", ".s.weqz.q0",
    ".u.dnez.q0", ".u.deqz.q0", ".u.wnez.q0", ".u.weqz.q0",
    ".us.dnez.q0", ".us.deqz.q0", ".us.wnez.q0", ".us.weqz.q0",
    ".dnez.q1", ".deqz.q1", ".wnez.q1", ".weqz.q1",
    ".s.dnez.q1", ".s.deqz.q1", ".s.wnez.q1", ".s.weqz.q1",
    ".u.dnez.q1", ".u.deqz.q1", ".u.wnez.q1", ".u.weqz.q1",
    ".us.dnez.q1", ".us.deqz.q1", ".us.wnez.q1", ".us.weqz.q1",
    ".dnez.q2", ".deqz.q2", ".wnez.q2", ".weqz.q2",
    ".s.dnez.q2", ".s.deqz.q2", ".s.wnez.q2", ".s.weqz.q2",
    ".u.dnez.q2", ".u.deqz.q2", ".u.wnez.q2", ".u.weqz.q2",
    ".us.dnez.q2", ".us.deqz.q2", ".us.wnez.q2", ".us.weqz.q2",
    ".dnez.q3", ".deqz.q3", ".wnez.q3", ".weqz.q3",
    ".s.dnez.q3", ".s.deqz.q3", ".s.wnez.q3", ".s.weqz.q3",
    ".u.dnez.q3", ".u.deqz.q3", ".u.wnez.q3", ".u.weqz.q3",
    ".us.dnez.q3", ".us.deqz.q3", ".us.wnez.q3", ".us.weqz.q3",
    ".mt.q0", ".mf.q0", ".mtc.q0", ".mfc.q0",
    ".s.mt.q0", ".s.mf.q0", ".s.mtc.q0", ".s.mfc.q0",
    ".u.mt.q0", ".u.mf.q0", ".u.mtc.q0", ".u.mfc.q0",
    ".us.mt.q0", ".us.mf.q0", ".us.mtc.q0", ".us.mfc.q0",
    ".mt.q1", ".mf.q1", ".mtc.q1", ".mfc.q1",
    ".s.mt.q1", ".s.mf.q1", ".s.mtc.q1", ".s.mfc.q1",
    ".u.mt.q1", ".u.mf.q1", ".u.mtc.q1", ".u.mfc.q1",
    ".us.mt.q1", ".us.mf.q1", ".us.mtc.q1", ".us.mfc.q1",
    ".mt.q2", ".mf.q2", ".mtc.q2", ".mfc.q2",
    ".s.mt.q2", ".s.mf.q2", ".s.mtc.q2", ".s.mfc.q2",
    ".u.mt.q2", ".u.mf.q2", ".u.mtc.q2", ".u.mfc.q2",
    ".us.mt.q2", ".us.mf.q2", ".us.mtc.q2", ".us.mfc.q2",
    ".mt.q3", ".mf.q3", ".mtc.q3", ".mfc.q3",
    ".s.mt.q3", ".s.mf.q3", ".s.mtc.q3", ".s.mfc.q3",
    ".u.mt.q3", ".u.mf.q3", ".u.mtc.q3", ".u.mfc.q3",
    ".us.mt.q3", ".us.mf.q3", ".us.mtc.q3", ".us.mfc.q3",
    NULL
};
const struct lvx_modmap lvx_modmap_xloadqc_lvx_1 = {
  lvx_mod_xloadqc_lvx_1, lvx_mod_xloadqc_lvx_1
};
const struct lvx_modmap *lvx_modmap_xloadqc = NULL;


const char *lvx_mod_xloadq_lvx_1[] = {
 ".q0", ".s.q0", ".u.q0", ".us.q0",
 ".q1", ".s.q1", ".u.q1", ".us.q1",
 ".q2", ".s.q2", ".u.q2", ".us.q2",
 ".q3", ".s.q3", ".u.q3", ".us.q3",
 NULL
};
const struct lvx_modmap lvx_modmap_xloadq_lvx_1 = {
  lvx_mod_xloadq_lvx_1, lvx_mod_xloadq_lvx_1
};
const struct lvx_modmap *lvx_modmap_xloadq = NULL;

const char *lvx_mod_xmatmul[] = {
 "", ".u", ".su", ".us", NULL
};
const struct lvx_modmap lvx_modmap_xmatmul_all = {
  lvx_mod_xmatmul, lvx_mod_xmatmul,
};
const struct lvx_modmap *lvx_modmap_xmatmul = &lvx_modmap_xmatmul_all;

/* This is remapped to .x .y .z .t */
const char *lvx_mod_xquarter_all_in[] = {
 ".q0", ".q1", ".q2", ".q3", NULL
};
const char *lvx_mod_xquarter_all_out[] = {
 ".x", ".y", ".z", ".t", NULL
};
const struct lvx_modmap lvx_modmap_xquarter_all = {
  lvx_mod_xquarter_all_in, lvx_mod_xquarter_all_out
};
const struct lvx_modmap *lvx_modmap_xquarter = &lvx_modmap_xquarter_all;

const char *lvx_mod_xshufflev[] = {
 "", ".td", NULL
};
const struct lvx_modmap lvx_modmap_xshufflev_all = {
  lvx_mod_xshufflev, lvx_mod_xshufflev,
};
const struct lvx_modmap *lvx_modmap_xshufflev = &lvx_modmap_xshufflev_all;

const char *lvx_mod_xshufflex[] = {
 "", ".zd", ".ud", ".tq", ".tw", ".zw", ".uw", NULL
};
const struct lvx_modmap lvx_modmap_xshufflex_all = {
  lvx_mod_xshufflex, lvx_mod_xshufflex,
};
const struct lvx_modmap *lvx_modmap_xshufflex = &lvx_modmap_xshufflex_all;

const char *lvx_mod_xvariant_lvx_1[] = {
 "", ".s", ".u", ".us", NULL
};
const struct lvx_modmap lvx_modmap_xvariant_lvx_1 = {
  lvx_mod_xvariant_lvx_1, lvx_mod_xvariant_lvx_1,
};
const struct lvx_modmap *lvx_modmap_xvariant = NULL;

#define V64mode V1DImode
#define V128mode V2DImode
#define V256mode V4DImode
#define V512mode V8DImode
#define X256mode V1OImode
#define X512mode V2OImode
#define X1024mode V4OImode
#define X2048mode V8OImode
#define X4096mode V16OImode
#define X8192mode V32OImode
#define V64mode V1DImode
#define V128mode V2DImode
#define V256mode V4DImode
#define V512mode V8DImode
#define X256mode V1OImode
#define X512mode V2OImode
#define X1024mode V4OImode
#define X2048mode V8OImode
#define X4096mode V16OImode
#define X8192mode V32OImode

enum lvx_builtin_type_index
{
  LVX_BTI_STRING,

  LVX_BTI_V8QI,
  LVX_BTI_V16QI,
  LVX_BTI_V32QI,
  LVX_BTI_V64QI,

  LVX_BTI_V4HI,
  LVX_BTI_V8HI,
  LVX_BTI_V16HI,
  LVX_BTI_V32HI,

  LVX_BTI_V2SI,
  LVX_BTI_V4SI,
  LVX_BTI_V8SI,
  LVX_BTI_V16SI,

  LVX_BTI_V2DI,
  LVX_BTI_V4DI,
  LVX_BTI_V8DI,

  LVX_BTI_V4HF,
  LVX_BTI_V8HF,
  LVX_BTI_V16HF,
  LVX_BTI_V32HF,

  LVX_BTI_V2SF,
  LVX_BTI_V4SF,
  LVX_BTI_V8SF,
  LVX_BTI_V16SF,

  LVX_BTI_V2DF,
  LVX_BTI_V4DF,
  LVX_BTI_V8DF,

  LVX_BTI_DI,
  LVX_BTI_V64,
  LVX_BTI_V128,
  LVX_BTI_V256,
  LVX_BTI_V512,

  LVX_BTI_OI,

  LVX_BTI_X256,
  LVX_BTI_X512,
  LVX_BTI_X1024,
  LVX_BTI_X2048,
  LVX_BTI_X4096,
  LVX_BTI_X8192,

  LVX_BTI_PX256,

  LVX_BTI_FLOATINGS,

  LVX_BTI_MAX
};

tree lvx_builtin_types[LVX_BTI_MAX];

#define LVX_FTYPE_NAME0(A) LVX_##A##_FTYPE
#define LVX_FTYPE_NAME1(A, B) LVX_##A##_FTYPE_##B
#define LVX_FTYPE_NAME2(A, B, C) LVX_##A##_FTYPE_##B##_##C
#define LVX_FTYPE_NAME3(A, B, C, D) LVX_##A##_FTYPE_##B##_##C##_##D
#define LVX_FTYPE_NAME4(A, B, C, D, E) LVX_##A##_FTYPE_##B##_##C##_##D##_##E
#define LVX_FTYPE_NAME5(A, B, C, D, E, F) LVX_##A##_FTYPE_##B##_##C##_##D##_##E##_##F

/* LVX_FTYPE_ATYPESN takes N LVX_FTYPES-like type codes and lists
   their associated LVX_ATYPEs.  */
#define LVX_FTYPE_ATYPES0(A) \
  LVX_ATYPE_##A
#define LVX_FTYPE_ATYPES1(A, B) \
  LVX_ATYPE_##A, LVX_ATYPE_##B
#define LVX_FTYPE_ATYPES2(A, B, C) \
  LVX_ATYPE_##A, LVX_ATYPE_##B, LVX_ATYPE_##C
#define LVX_FTYPE_ATYPES3(A, B, C, D) \
  LVX_ATYPE_##A, LVX_ATYPE_##B, LVX_ATYPE_##C, LVX_ATYPE_##D
#define LVX_FTYPE_ATYPES4(A, B, C, D, E) \
  LVX_ATYPE_##A, LVX_ATYPE_##B, LVX_ATYPE_##C, LVX_ATYPE_##D, LVX_ATYPE_##E
#define LVX_FTYPE_ATYPES5(A, B, C, D, E, F) \
  LVX_ATYPE_##A, LVX_ATYPE_##B, LVX_ATYPE_##C, LVX_ATYPE_##D, LVX_ATYPE_##E, LVX_ATYPE_##F

#define LVX_FTYPE_MOD0(A) LVX_MOD_##A
#define LVX_FTYPE_MOD1(A, B) LVX_MOD_##B
#define LVX_FTYPE_MOD2(A, B, C) LVX_MOD_##C
#define LVX_FTYPE_MOD3(A, B, C, D) LVX_MOD_##D
#define LVX_FTYPE_MOD4(A, B, C, D, E) LVX_MOD_##E
#define LVX_FTYPE_MOD5(A, B, C, D, E, F) LVX_MOD_##F

#define LVX_ATYPE_VOID void_type_node
#define LVX_ATYPE_VPTR ptr_type_node
#define LVX_ATYPE_CVPTR const_ptr_type_node
#define LVX_ATYPE_BOOL boolean_type_node

#define LVX_ATYPE_INT8 intQI_type_node
#define LVX_ATYPE_INT16 intHI_type_node
#define LVX_ATYPE_INT32 intSI_type_node
#define LVX_ATYPE_INT64 intDI_type_node
#define LVX_ATYPE_INT128 intTI_type_node

#define LVX_ATYPE_UINT8 unsigned_intQI_type_node
#define LVX_ATYPE_UINT16 unsigned_intHI_type_node
#define LVX_ATYPE_UINT32 unsigned_intSI_type_node
#define LVX_ATYPE_UINT64 unsigned_intDI_type_node
#define LVX_ATYPE_UINT128 unsigned_intTI_type_node

#define LVX_ATYPE_FLOAT16 float16_type_node
#define LVX_ATYPE_FLOAT32 float_type_node
#define LVX_ATYPE_FLOAT64 double_type_node
#define LVX_ATYPE_COMPLEX64 complex_float_type_node
#define LVX_ATYPE_COMPLEX128 complex_double_type_node

#define LVX_ATYPE_STRING (lvx_builtin_types[LVX_BTI_STRING])

#define LVX_ATYPE_V8QI (lvx_builtin_types[LVX_BTI_V8QI])
#define LVX_ATYPE_V16QI (lvx_builtin_types[LVX_BTI_V16QI])
#define LVX_ATYPE_V32QI (lvx_builtin_types[LVX_BTI_V32QI])
#define LVX_ATYPE_V64QI (lvx_builtin_types[LVX_BTI_V64QI])

#define LVX_ATYPE_V4HI (lvx_builtin_types[LVX_BTI_V4HI])
#define LVX_ATYPE_V8HI (lvx_builtin_types[LVX_BTI_V8HI])
#define LVX_ATYPE_V16HI (lvx_builtin_types[LVX_BTI_V16HI])
#define LVX_ATYPE_V32HI (lvx_builtin_types[LVX_BTI_V32HI])

#define LVX_ATYPE_V2SI (lvx_builtin_types[LVX_BTI_V2SI])
#define LVX_ATYPE_V4SI (lvx_builtin_types[LVX_BTI_V4SI])
#define LVX_ATYPE_V8SI (lvx_builtin_types[LVX_BTI_V8SI])
#define LVX_ATYPE_V16SI (lvx_builtin_types[LVX_BTI_V16SI])

#define LVX_ATYPE_V2DI (lvx_builtin_types[LVX_BTI_V2DI])
#define LVX_ATYPE_V4DI (lvx_builtin_types[LVX_BTI_V4DI])
#define LVX_ATYPE_V8DI (lvx_builtin_types[LVX_BTI_V8DI])

#define LVX_ATYPE_V4HF (lvx_builtin_types[LVX_BTI_V4HF])
#define LVX_ATYPE_V8HF (lvx_builtin_types[LVX_BTI_V8HF])
#define LVX_ATYPE_V16HF (lvx_builtin_types[LVX_BTI_V16HF])
#define LVX_ATYPE_V32HF (lvx_builtin_types[LVX_BTI_V32HF])

#define LVX_ATYPE_V2SF (lvx_builtin_types[LVX_BTI_V2SF])
#define LVX_ATYPE_V4SF (lvx_builtin_types[LVX_BTI_V4SF])
#define LVX_ATYPE_V8SF (lvx_builtin_types[LVX_BTI_V8SF])
#define LVX_ATYPE_V16SF (lvx_builtin_types[LVX_BTI_V16SF])

#define LVX_ATYPE_V2DF (lvx_builtin_types[LVX_BTI_V2DF])
#define LVX_ATYPE_V4DF (lvx_builtin_types[LVX_BTI_V4DF])
#define LVX_ATYPE_V8DF (lvx_builtin_types[LVX_BTI_V8DF])

#define LVX_ATYPE_DI  (lvx_builtin_types[LVX_BTI_DI])
#define LVX_ATYPE_V64 (lvx_builtin_types[LVX_BTI_V64])
#define LVX_ATYPE_V128 (lvx_builtin_types[LVX_BTI_V128])
#define LVX_ATYPE_V256 (lvx_builtin_types[LVX_BTI_V256])
#define LVX_ATYPE_V512 (lvx_builtin_types[LVX_BTI_V512])

#define LVX_ATYPE_OI (lvx_builtin_types[LVX_BTI_OI])

#define LVX_ATYPE_X256 (lvx_builtin_types[LVX_BTI_X256])
#define LVX_ATYPE_X512 (lvx_builtin_types[LVX_BTI_X512])
#define LVX_ATYPE_X1024 (lvx_builtin_types[LVX_BTI_X1024])
#define LVX_ATYPE_X2048 (lvx_builtin_types[LVX_BTI_X2048])
#define LVX_ATYPE_X4096 (lvx_builtin_types[LVX_BTI_X4096])
#define LVX_ATYPE_X8192 (lvx_builtin_types[LVX_BTI_X8192])

#define LVX_ATYPE_PX256 (lvx_builtin_types[LVX_BTI_PX256])

#define LVX_ATYPE_ABDSATUNS LVX_ATYPE_STRING
#define LVX_ATYPE_ACCESSES LVX_ATYPE_STRING
#define LVX_ATYPE_AVERAGE LVX_ATYPE_STRING
#define LVX_ATYPE_BOOLCASCO LVX_ATYPE_STRING
#define LVX_ATYPE_CACHELEV LVX_ATYPE_STRING
#define LVX_ATYPE_COHERENCY LVX_ATYPE_STRING
#define LVX_ATYPE_CONJUGATE LVX_ATYPE_STRING
#define LVX_ATYPE_COUNTING LVX_ATYPE_STRING
#define LVX_ATYPE_EXTENDMUL LVX_ATYPE_STRING
#define LVX_ATYPE_FLOATINGS LVX_ATYPE_STRING
#define LVX_ATYPE_LANEWISE LVX_ATYPE_STRING
#define LVX_ATYPE_LOADCOND LVX_ATYPE_STRING
#define LVX_ATYPE_MOVECOND LVX_ATYPE_STRING
#define LVX_ATYPE_MULXDT LVX_ATYPE_STRING
#define LVX_ATYPE_NARROWINT LVX_ATYPE_STRING
#define LVX_ATYPE_SATURATE LVX_ATYPE_STRING
#define LVX_ATYPE_SHIFTLEFT LVX_ATYPE_STRING
#define LVX_ATYPE_SHIFTRIGHT LVX_ATYPE_STRING
#define LVX_ATYPE_MOSTSIG LVX_ATYPE_STRING
#define LVX_ATYPE_SIGNEDSAT LVX_ATYPE_STRING
#define LVX_ATYPE_SIMDCOND LVX_ATYPE_STRING
#define LVX_ATYPE_SIMDCONDD LVX_ATYPE_STRING
#define LVX_ATYPE_STORECOND LVX_ATYPE_STRING
#define LVX_ATYPE_TRANSPOSE LVX_ATYPE_STRING
#define LVX_ATYPE_UNUSED LVX_ATYPE_STRING
#define LVX_ATYPE_VARIANT LVX_ATYPE_STRING
#define LVX_ATYPE_WIDENINT LVX_ATYPE_STRING
#define LVX_ATYPE_XCHANNEL LVX_ATYPE_STRING
#define LVX_ATYPE_XCHANNELS LVX_ATYPE_STRING
#define LVX_ATYPE_XHALF LVX_ATYPE_STRING
#define LVX_ATYPE_XLOADCOND LVX_ATYPE_STRING
#define LVX_ATYPE_XLOADHC LVX_ATYPE_STRING
#define LVX_ATYPE_XLOADH LVX_ATYPE_STRING
#define LVX_ATYPE_XLOADQC LVX_ATYPE_STRING
#define LVX_ATYPE_XLOADQ LVX_ATYPE_STRING
#define LVX_ATYPE_XMATMUL LVX_ATYPE_STRING
#define LVX_ATYPE_XPRELOAD LVX_ATYPE_STRING
#define LVX_ATYPE_XQUARTER LVX_ATYPE_STRING
#define LVX_ATYPE_XSHUFFLEV LVX_ATYPE_STRING
#define LVX_ATYPE_XSHUFFLEX LVX_ATYPE_STRING
#define LVX_ATYPE_XVARIANT LVX_ATYPE_STRING

/* Classifies the prototype of a built-in function.  */
enum lvx_function_type {
#define DEF_LVX_FTYPE(NARGS, LIST) LVX_FTYPE_NAME##NARGS LIST,
#include "config/lvx/lvx-ftypes.def"
#undef DEF_LVX_FTYPE
  LVX_MAX_FTYPE_MAX
};

static const lvx_modmap*
lvx_get_modifiers (enum lvx_function_type type)
{
  switch (type)
  {
#define DEF_LVX_FTYPE(NUM, ARGS)					\
  case LVX_FTYPE_NAME##NUM ARGS:					\
    return LVX_FTYPE_MOD##NUM ARGS;
#include "config/lvx/lvx-ftypes.def"
#undef DEF_LVX_FTYPE
  default:
    return NULL;
  }
}

static tree
lvx_build_function_type (enum lvx_function_type type, int variadic_p)
{
  static tree types[(int) LVX_MAX_FTYPE_MAX];

  // TODO: Check if no memory leak, when reassigning the type.
  // Maybe we would like to store the va_types and the types in 2 different
  // tables.
  if (types[(int) type] == NULL_TREE || variadic_p)
    switch (type)
      {
#define DEF_LVX_FTYPE(NUM, ARGS)                                        \
  case LVX_FTYPE_NAME##NUM ARGS:                                        \
    types[(int) type] = variadic_p                                      \
	? build_varargs_function_type_list (LVX_FTYPE_ATYPES##NUM ARGS, \
				    NULL_TREE)                          \
	: build_function_type_list (LVX_FTYPE_ATYPES##NUM ARGS,         \
					    NULL_TREE);                 \
    break;
#include "config/lvx/lvx-ftypes.def"
#undef DEF_LVX_FTYPE
      default:
	gcc_unreachable ();
      }

  return types[(int) type];
}

/* Specifies how a built-in function should be converted into rtl.  */
enum lvx_builtin_type
{
  /* The function corresponds directly to an .md pattern.  */
  LVX_BLTN_DIRECT,

  /* The function does not map directly to an .md pattern and need a custom
     expander. */
  LVX_BLTN_INDIRECT,
  LVX_BLTN_TYPE_MAX
};

struct lvx_builtin_desc
{
  /* The name of the built-in function.  */
  const char *name;

  /* The code of the main .md file instruction or -1 for LVX_BLTN_INDIRECT. */
  enum insn_code icode;

  /* Specifies how the function should be expanded.  */
  enum lvx_builtin_type type;

  /* The function's prototype.  */
  enum lvx_function_type prototype;

  /* The expander function.  */
  rtx (*expand)(rtx, tree);

  /* Predicate telling whether the built-in function can take a variable
     number of arguments.  */
  unsigned int variadic_p;

  /* The function's prototype.  */
  enum lvx_function_type variadic_prototype;

  /* Whether the function is available.  */
  unsigned int (*avail) (void);
};

/* Declare an availability predicate for built-in functions.  */
#define AVAIL(NAME, COND)		\
 static unsigned int			\
 lvx_builtin_avail_##NAME (void)	\
 {					\
   return (COND);			\
 }

AVAIL (true, true)
AVAIL (lvx_1, LVX_1)
AVAIL (lvx_2, LVX_2)

static inline const char *
lvx_tree_string_constant (tree arg, const char *name)
{
  tree offset_tree = 0;
  arg = string_constant (arg, &offset_tree, 0, 0);
  if (!arg)
    {
      error ("%<%s%> requires a constant string modifier", name);
      return "";
    }
  return TREE_STRING_POINTER (arg);
}

static rtx
build_arg (tree arg, const char *name, const struct lvx_modmap *mod_table)
{
  const char *modifier = lvx_tree_string_constant (arg, name);
  for (int i = 0; mod_table->imap[i]; ++i)
    if (!strcmp (modifier, mod_table->imap[i]))
      return gen_rtx_CONST_STRING (VOIDmode, mod_table->omap[i]);

  char buffer[1024] = { 0 };
  char *btmp = buffer;
  for (int i = 0; mod_table->imap[i]; ++i)
    {
      btmp = stpcpy (btmp, mod_table->imap[i]);
      if (mod_table->imap[i+1])
        btmp = stpcpy (btmp, ", ");
    }
  error ("%<%s%> modifier %<%s%> not recognized", name, modifier);
  inform (input_location, "modifier list: %s", buffer);

  /* The error above already fails the compilation, but every caller passes
     this result straight into a gen_* that dereferences it, so returning
     NULL_RTX turns a diagnostic into a segfault.  Hand back the table's first
     output -- the default modifier -- and let the error do its job.  */
  return gen_rtx_CONST_STRING (VOIDmode, mod_table->omap[0]);
}

static rtx
verify_const_bool_arg (rtx arg, const char *name, const char *where)
{
  if (GET_CODE (arg) == CONST_INT && GET_MODE (arg) == VOIDmode)
    {
      unsigned long long tmp = INTVAL (arg);
      if (tmp == 0LL || tmp == 1LL)
	return arg;
    }
  error ("%<%s%> expects a boolean immediate in %s argument", name,
	 where);
  /* Diagnosed, so the compilation fails either way -- but the caller
     passes this into a gen_* that dereferences it, so an error must not
     become a crash.  Hand back a well-formed constant.  */
  return const0_rtx;
}

static rtx
verify_const_ready_arg (rtx arg, const char *name, const char *where)
{
  if (GET_CODE (arg) == CONST_INT && GET_MODE (arg) == VOIDmode)
    {
      unsigned long long tmp = INTVAL (arg);
      if (tmp == 0LL || tmp == 1LL)
	return arg;
    }
  if (GET_MODE (arg) == SImode)
    return force_not_mem (arg);
  error ("%<%s%> expects a boolean value in %s argument", name,
	 where);
  /* Diagnosed, so the compilation fails either way -- but the caller
     passes this into a gen_* that dereferences it, so an error must not
     become a crash.  Hand back a well-formed constant.  */
  return const0_rtx;
}

__attribute__ ((unused))
static rtx
verify_const_int_arg (rtx arg, int bits, const char *name, const char *where)
{
  if (GET_CODE (arg) == CONST_INT && GET_MODE (arg) == VOIDmode)
    {
      unsigned shift = 64 - bits;
      long long tmp = INTVAL (arg);
      signed long long stmp = tmp;
      if (tmp == (stmp << shift) >> shift)
	return arg;
    }
  error ("%<%s%> expects a %d-bit signed immediate in %s argument",
	 name, bits, where);
  /* Diagnosed, so the compilation fails either way -- but the caller
     passes this into a gen_* that dereferences it, so an error must not
     become a crash.  Hand back a well-formed constant.  */
  return const0_rtx;
}

static rtx
verify_const_uint_arg (rtx arg, int bits, const char *name,
	const char *where, bool strict)
{
  if (GET_CODE (arg) == CONST_INT && GET_MODE (arg) == VOIDmode)
    {
      unsigned shift = 64 - bits;
      unsigned long long utmp = INTVAL (arg);
      if (utmp == (utmp << shift) >> shift)
	return arg;
    }
  if (strict)
    {
      error ("%<%s%> expects a %d-bit unsigned immediate "
	     "in %s argument", name, bits, where);
      /* Diagnosed, and the strict callers -- set, wfxl, wfxm -- use the
	 result.  A NULL_RTX here reaches a gen_* and crashes.  */
      return const0_rtx;
    }

  /* Not strict: this is a probe, and (lvx_expand_builtin_get) tests the
     NULL_RTX to decide whether the register number was a constant.  */
  return NULL_RTX;
}

__attribute__ ((unused))
static rtx
verify_const_field_arg (rtx arg, int bits, const char *name, const char *where)
{
  if (GET_CODE (arg) == CONST_INT && GET_MODE (arg) == VOIDmode)
    {
      unsigned shift = 64 - bits;
      long long tmp = INTVAL (arg);
      signed long long stmp = tmp;
      unsigned long long utmp = tmp;
      if (tmp == (stmp << shift) >> shift)
	return arg;
      if (utmp == (utmp << shift) >> shift)
	return arg;
    }
  error ("%<%s%> expects a %d-bit signed or unsigned immediate in %s argument",
	 name, bits, where);
  /* Diagnosed, so the compilation fails either way -- but the caller
     passes this into a gen_* that dereferences it, so an error must not
     become a crash.  Hand back a well-formed constant.  */
  return const0_rtx;
}

static int
verify_sfr_regno (int regno, const char *name, const char *where)
{
  int gcc_regno = LVX_SFR_FIRST_REGNO + regno;
  if (gcc_regno  > LVX_SFR_LAST_REGNO) {
    error ("%<%s%> passed %d as %s argument: expects a SFR register index between 0 and %d",
	   name, regno, where, LVX_SFR_LAST_REGNO - LVX_SFR_FIRST_REGNO);
  }

  return gcc_regno;
}

unsigned long lvx_builtin_undef_counter;

#define LVX_EXPAND_BUILTIN_0_VOID(name, name2, ...)                            \
  static rtx lvx_expand_builtin_##name (rtx target ATTRIBUTE_UNUSED,           \
                                        tree args ATTRIBUTE_UNUSED)            \
  {                                                                            \
    emit_insn (CONCAT2 (gen_, name2) ());                                      \
    return NULL_RTX;                                                           \
  }

#define LVX_EXPAND_BUILTIN_1_VOID(name, name2, smode)                          \
  static rtx lvx_expand_builtin_##name (rtx target ATTRIBUTE_UNUSED, tree args)\
  {                                                                            \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    arg1 = force_reg (smode, arg1);                                            \
    emit_insn (CONCAT2 (gen_, name2) (arg1));                                  \
    return NULL_RTX;                                                           \
  }

#define LVX_EXPAND_BUILTIN_2_VOID(name, name2, smode)                          \
  static rtx lvx_expand_builtin_##name (rtx target ATTRIBUTE_UNUSED, tree args)\
  {                                                                            \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));                        \
    arg1 = force_reg (smode, arg1);                                            \
    arg2 = force_reg (smode, arg2);                                            \
    emit_insn (gen_##name2 (arg1, arg2));                                      \
    return NULL_RTX;                                                           \
  }

#define LVX_EXPAND_BUILTIN_3_CACHELEVEL(name, name2, smode)                    \
  static rtx lvx_expand_builtin_##name (rtx target ATTRIBUTE_UNUSED, tree args)\
  {                                                                            \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));                        \
    rtx arg3 = build_arg (CALL_EXPR_ARG (args, 2), #name, lvx_modmap_cachelev);\
    arg1 = force_reg (smode, arg1);                                            \
    arg2 = force_reg (smode, arg2);                                            \
    emit_insn (gen_##name2 (arg1, arg2, arg3));                                \
    return NULL_RTX;                                                           \
  }

#define LVX_EXPAND_BUILTIN_SHIFT(name, name2, tmode, smode)                    \
  static rtx lvx_expand_builtin_##name (rtx target, tree args)                 \
  {                                                                            \
    int bits = __builtin_ctz (GET_MODE_NUNITS (tmode));                        \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));                        \
    rtx arg3 = expand_normal (CALL_EXPR_ARG (args, 2));                        \
    arg1 = force_reg (tmode, arg1);                                            \
    arg2 = verify_const_int_arg (arg2, bits + 1, #name, "second");             \
    if (arg3 != CONST0_RTX (smode))                                            \
      {                                                                        \
        if (immediate_operand (arg3, VOIDmode))                                \
          arg3 = force_reg (smode, arg3);                                      \
        else                                                                   \
          arg3 = simplify_gen_subreg (smode, arg3, GET_MODE (arg3), 0);        \
      }                                                                        \
    if (!target)                                                               \
      target = gen_reg_rtx (tmode);                                            \
    else                                                                       \
      target = force_reg (tmode, target);                                      \
    emit_insn (gen_##name2 (target, arg1, arg2, arg3));                        \
    return target;                                                             \
  }

static rtx
lvx_expand_builtin_get (rtx target, tree args)
{
  int regno = -1;
  rtx sys_reg = NULL_RTX;
  rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));
  if (verify_const_uint_arg (arg1, 9, "get", "first", false) != NULL_RTX)
    {
      arg1 = verify_const_uint_arg (arg1, 9, "get", "first", false);
      regno = verify_sfr_regno (INTVAL (arg1), "get", "first");
      sys_reg = gen_rtx_REG (DImode, regno);
    }
  else
    {
      if (GET_MODE (arg1) == VOIDmode)
        error ("%<__builtin_lvx_gets%> expects a 9-bit immediate or a register.");

      /* If sys_reg > LVX_SFR_LAST_REGNO-LVX_SFR_FAKE_GPR_COUNT (this is 175
         at the moment of writing, it will uses iget because the constraint on
         SFR will fail due to some registers being unavailable.  */
      sys_reg = force_reg (GET_MODE (arg1), arg1);
      sys_reg = simplify_gen_subreg (DImode, sys_reg, SImode, 0);
    }

  if (!target)
    target = gen_reg_rtx (DImode);
  else
    target = force_reg (DImode, target);
  if (regno == LVX_PCR_REGNO)
    emit_move_insn (target, sys_reg);
  else
    emit_insn (gen_lvx_get (target, sys_reg));
  return target;
}

static rtx
lvx_expand_builtin_set (rtx target ATTRIBUTE_UNUSED, tree args)
{
  rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));
  rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));
  arg1 = verify_const_uint_arg (arg1, 9, "set", "first", true);
  arg2 = force_reg (DImode, arg2);
  int regno = verify_sfr_regno (INTVAL (arg1), "set", "first");
  rtx sys_reg = gen_rtx_REG (DImode, regno);
  emit_insn (gen_lvx_set (sys_reg, arg2));
  return NULL_RTX;
}

static rtx
lvx_expand_builtin_wfxl (rtx target ATTRIBUTE_UNUSED, tree args)
{
  rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));
  rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));
  arg1 = verify_const_uint_arg (arg1, 9, "wfxl", "first", true);
  arg2 = force_reg (DImode, arg2);
  int regno = verify_sfr_regno (INTVAL (arg1), "wfxl", "first");
  rtx sys_reg = gen_rtx_REG (DImode, regno);
  emit_insn (gen_lvx_wfxl (sys_reg, arg2));
  return NULL_RTX;
}

static rtx
lvx_expand_builtin_wfxm (rtx target ATTRIBUTE_UNUSED, tree args)
{
  rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));
  rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));
  arg1 = verify_const_uint_arg (arg1, 9, "wfxm", "first", true);
  arg2 = force_reg (DImode, arg2);
  int regno = verify_sfr_regno (INTVAL (arg1), "wfxm", "first");
  rtx sys_reg = gen_rtx_REG (DImode, regno);
  emit_insn (gen_lvx_wfxm (sys_reg, arg2));
  return NULL_RTX;
}

static rtx
lvx_expand_builtin_scall (rtx target, tree exp)
{
  int nargs = call_expr_nargs (exp) - 1;

  /* Accept as most 11 arguments, we don't do any argument checking,
     but calling a syscall with anything other than pointers or integers
     will lead to ``interesting'' results.  */
  if (nargs > 11)
    error ("%<__builtin_lvx_scall%> accepts at most 11 arguments");

  /* The type of a syscall function, basically anything that returns
     an uint64_t, and the type of function pointer to a syscall.  */
  tree fntype = build_function_type (long_integer_type_node, NULL_TREE, true);
  tree pfntype = build_pointer_type (fntype);

  /* syscallno is either a const_int or an ssa_name whose contents is
     a const_int, if this is an ssa_name we change its type to make
     gcc believes its a function pointer to an scall.  Otherwise, we
     create a MEM_REF around the const_int to achieve the same purpose. */
  tree syscallno = CALL_EXPR_ARG (exp, 0);
  if (TREE_CODE (syscallno) == SSA_NAME)
    TREE_TYPE (syscallno) = build_pointer_type (fntype);

  tree syscall_memref = build2 (MEM_REF, integer_type_node, syscallno,
				build_int_cst (integer_type_node, 0));
  tree fn = TREE_CODE (syscallno) == SSA_NAME
    ? syscallno : build1 (ADDR_EXPR, pfntype, syscall_memref);

  tree *args = XALLOCAVEC (tree, nargs);

  for (int i = 0; i < nargs; ++i)
    args[i] = (CALL_EXPR_ARG (exp, i + 1));

  tree scall = build_call_array_loc (UNKNOWN_LOCATION, long_integer_type_node,
				     fn, nargs, args);

  /* We let gcc handle the call expansion.  */
  if (target)
    target = expand_call (scall, target, target == const0_rtx);
  else
    expand_call (scall, target, target == const0_rtx);

  /* However, we need to fixup gcc's doing since the standard machinery does not
     like much dealing with raw const_int as addresses, and fails to stick an
     address space to the memory operand of the call.  */
  rtx_call_insn *call_insn = last_call_insn ();
  rtx call = get_call_rtx_from (call_insn);

  if (call && MEM_EXPR (XEXP (call, 0)) != NULL_TREE)
    {
      rtx mem = XEXP (call, 0);
      rtx call_addr = XEXP (mem, 0);
      set_mem_addr_space (mem, LVX_ADDR_SPACE_SYSCALL);
      if (TREE_CODE (syscallno) == SSA_NAME && MEM_P (call_addr))
	goto done;

      /* If the scall no is a const_int and has been indirected in a
         register, we fetch the def and use and replace the use directly
         by the const_int. */
      if (REG_P (call_addr))
	{
	  rtx_insn *reg_def;
	  unsigned tgt_regno = REGNO (call_addr);
	  for (reg_def = get_last_insn (); reg_def;
	       reg_def = PREV_INSN (reg_def))
	    if (GET_CODE (reg_def) == INSN)
	      {
		rtx pat = PATTERN (reg_def);
		if (GET_CODE (pat) == SET
		    && (GET_CODE (XEXP (pat, 0)) == REG
			|| GET_CODE (XEXP (pat, 0)) == SUBREG)
		    && (REGNO (XEXP (pat, 0)) == tgt_regno))
		  break;
	      }
	  if (reg_def)
	    {
	      rtx pat = PATTERN (reg_def);
	      if (GET_CODE (XEXP (pat, 1)) == CONST_INT)
		XEXP (XEXP (call, 0), 0) = XEXP (pat, 1);
	    }
	}
    }

  /* A nop, so that even if scall happens to be at the end of an hardware
     loop, everything is fine. */
  emit_insn (gen_nop_volatile ());

done:
  return target;
}

void
lvx_expand_builtin_maddt (rtx operands[], int add)
{
  operands[0] = simplify_gen_subreg (TImode, operands[0], V2DImode, 0);
  operands[3] = simplify_gen_subreg (TImode, operands[3], V2DImode, 0);
  const char *xstr = XSTR (operands[4], 0);
  rtx pattern = NULL_RTX;

  if (!*xstr)
    {
      rtx mulx = gen_rtx_MULT (TImode, gen_rtx_SIGN_EXTEND (TImode, operands[1]),
				       gen_rtx_SIGN_EXTEND (TImode, operands[2]));
      pattern = add ? gen_rtx_PLUS (TImode, mulx, operands[3]) :
		      gen_rtx_MINUS (TImode, operands[3], mulx);
    }
  else if (xstr[1] == 'u' && xstr[2] == 0)
    {
      rtx mulx = gen_rtx_MULT (TImode, gen_rtx_ZERO_EXTEND (TImode, operands[1]),
				       gen_rtx_ZERO_EXTEND (TImode, operands[2]));
      pattern = add ? gen_rtx_PLUS (TImode, mulx, operands[3]) :
		      gen_rtx_MINUS (TImode, operands[3], mulx);
    }
  else if (xstr[1] == 'u' && xstr[2] == 'z')
    {
      rtx mulx = gen_rtx_MULT (TImode, gen_rtx_ZERO_EXTEND (TImode, operands[1]),
				       gen_rtx_ZERO_EXTEND (TImode, operands[2]));
      rtx sl64 = gen_rtx_LSHIFTRT (TImode, operands[3], GEN_INT (64));
      pattern = add ? gen_rtx_PLUS (TImode, mulx, sl64) :
		      gen_rtx_MINUS (TImode, sl64, mulx);
    }
  else if (xstr[1] == 's' && xstr[2] == 'u')
    {
      rtx mulx = gen_rtx_MULT (TImode, gen_rtx_ZERO_EXTEND (TImode, operands[2]),
				       gen_rtx_SIGN_EXTEND (TImode, operands[1]));
      pattern = add ? gen_rtx_PLUS (TImode, mulx, operands[3]) :
		      gen_rtx_MINUS (TImode, operands[3], mulx);
    }
  else
    gcc_unreachable ();

  emit_insn (gen_rtx_SET (operands[0], pattern));
}

void
lvx_expand_builtin_fmuldc (rtx operands[], int lanes)
{
  rtx modifiers = operands[3];
  machine_mode mode = GET_MODE (operands[0]);
  bool conjugate = lvx_modifier_enabled_p (".c", modifiers);
  if (conjugate)
    modifiers = lvx_modifier_rounding (modifiers);
  for (int i = 0; i < lanes; i++)
    {
      rtx real_0 = simplify_gen_subreg (DFmode, operands[0], mode, i*16+0);
      rtx imag_0 = simplify_gen_subreg (DFmode, operands[0], mode, i*16+8);
      rtx real_1 = simplify_gen_subreg (DFmode, operands[1], mode, i*16+0);
      rtx imag_1 = simplify_gen_subreg (DFmode, operands[1], mode, i*16+8);
      rtx real_2 = simplify_gen_subreg (DFmode, operands[2], mode, i*16+0);
      rtx imag_2 = simplify_gen_subreg (DFmode, operands[2], mode, i*16+8);
      rtx real_t = gen_reg_rtx (DFmode), imag_t = gen_reg_rtx (DFmode);
      if (conjugate)
	{
	  emit_insn (gen_lvx_fmuld (real_t, real_1, real_2, modifiers));
	  emit_insn (gen_lvx_ffmad (real_0, imag_1, imag_2, real_t, modifiers));
	  emit_insn (gen_lvx_fmuld (imag_t, real_1, imag_2, modifiers));
	  emit_insn (gen_lvx_ffmsd (imag_0, real_2, imag_1, imag_t, modifiers));
	}
      else
	{
	  emit_insn (gen_lvx_fmuld (real_t, real_1, real_2, modifiers));
	  emit_insn (gen_lvx_ffmsd (real_0, imag_1, imag_2, real_t, modifiers));
	  emit_insn (gen_lvx_fmuld (imag_t, real_1, imag_2, modifiers));
	  emit_insn (gen_lvx_ffmad (imag_0, real_2, imag_1, imag_t, modifiers));
	}
    }
}


static rtx
lvx_expand_builtin_fence (rtx ARG_UNUSED (target), tree args)
{
  int nargs = call_expr_nargs (args);
  rtx arg1 = nargs >= 1?
	     build_arg (CALL_EXPR_ARG (args, 0), "fence", lvx_modmap_accesses):
	     gen_rtx_CONST_STRING (VOIDmode, "");
  emit_insn (gen_lvx_fence (arg1));
  return NULL_RTX;
}

static rtx
lvx_expand_builtin_ready (rtx target, tree args)
{
  rtx arguments[4];
  int nargs = call_expr_nargs (args);
  for (int i = 0; i < nargs; i++)
    {
      arguments[i] = expand_normal (CALL_EXPR_ARG (args, i));
      machine_mode mode = GET_MODE (arguments[i]);
      arguments[i] = force_reg (mode, arguments[i]);
      arguments[i] = simplify_gen_subreg (DImode, arguments[i], mode, 0);
    }
  if (!target)
    target = gen_reg_rtx (SImode);
  else
    target = force_reg (SImode, target);
  rtvec vec = 0;
  if (nargs == 0)
    return target;
  if (nargs == 1)
    vec = gen_rtvec (1, arguments[0]);
  else if (nargs == 2)
    vec = gen_rtvec (2, arguments[0], arguments[1]);
  else if (nargs == 3)
    vec = gen_rtvec (3, arguments[0], arguments[1], arguments[2]);
  else if (nargs == 4)
    vec = gen_rtvec (4, arguments[0], arguments[1], arguments[2], arguments[3]);
  else
    error ("%<__builtin_lvx_ready%> must have one to four arguments");
  emit_insn (gen_rtx_SET (target, gen_rtx_UNSPEC (SImode, vec, UNSPEC_READY)));
  return target;
}


#define LVX_EXPAND_BUILTIN_ALOAD(name, name2, tmode, mmode)                    \
  static rtx lvx_expand_builtin_##name (rtx target, tree args)                 \
  {                                                                            \
    int nargs = call_expr_nargs (args);                                        \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    rtx arg2 = nargs >= 2?                                                     \
               build_arg (CALL_EXPR_ARG (args, 1), #name, lvx_modmap_coherency):\
               gen_rtx_CONST_STRING (VOIDmode, "");                            \
    arg1 = gen_rtx_MEM (mmode, force_reg (Pmode, arg1));                       \
    if (!target)                                                               \
      target = gen_reg_rtx (tmode);                                            \
    else                                                                       \
      target = force_reg (tmode, target);                                      \
    emit_insn (gen_##name2 (target, arg1, arg2));                              \
    return target;                                                             \
  }


#define LVX_EXPAND_BUILTIN_ALADD(name, name2, tmode, mmode)                    \
  static rtx lvx_expand_builtin_##name (rtx target, tree args)                 \
  {                                                                            \
    int nargs = call_expr_nargs (args);                                        \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));                        \
    rtx arg3 = nargs >= 3?                                                     \
               build_arg (CALL_EXPR_ARG (args, 2), #name, lvx_modmap_coherency):\
               gen_rtx_CONST_STRING (VOIDmode, "");                            \
    arg1 = gen_rtx_MEM (mmode, force_reg (Pmode, arg1));                       \
    arg2 = force_reg (tmode, arg2);                                            \
    if (!target)                                                               \
      target = gen_reg_rtx (tmode);                                            \
    else                                                                       \
      target = force_reg (tmode, target);                                      \
    emit_insn (gen_##name2 (target, arg1, arg2, arg3));                        \
    return target;                                                             \
  }


#define LVX_EXPAND_BUILTIN_ACSWAP(name, name2, tmode, mmode)                   \
  static rtx lvx_expand_builtin_##name (rtx target, tree args)                 \
  {                                                                            \
    int nargs = call_expr_nargs (args);                                        \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));                        \
    rtx arg3 = expand_normal (CALL_EXPR_ARG (args, 2));                        \
    rtx arg4 = nargs >= 4?                                                     \
               build_arg (CALL_EXPR_ARG (args, 3), #name, lvx_modmap_ ## boolcasco):\
               gen_rtx_CONST_STRING (VOIDmode, "");                            \
    rtx arg5 = const0_rtx;                                                     \
    if (nargs > 4)                                                             \
      {                                                                        \
        arg5 = expand_normal (CALL_EXPR_ARG (args, 4));                        \
        arg5 = verify_const_ready_arg (arg5, #name, "fifth");                  \
      }                                                                        \
    arg1 = gen_rtx_MEM (mmode, force_reg (Pmode, arg1));                       \
    rtx new_old = gen_reg_rtx (tmode == TImode ? OImode : TImode);             \
    int high_offset = tmode == TImode ? 16 : 8;                                \
    if (!target)                                                               \
      target = gen_reg_rtx (tmode);                                            \
    else                                                                       \
      target = force_reg (tmode, target);                                      \
    emit_move_insn (gen_rtx_SUBREG (tmode, new_old, 0), arg2);                 \
    emit_move_insn (gen_rtx_SUBREG (tmode, new_old, high_offset), arg3);       \
    emit_insn (gen_##name2 (new_old, arg1, arg4, arg5));                       \
    rtx result = gen_lowpart_SUBREG (tmode, new_old);                          \
    emit_move_insn (target, result);                                           \
    return target;                                                             \
  }

#define LVX_EXPAND_BUILTIN_ASTORE(name, name2, tmode, mmode)                   \
  static rtx lvx_expand_builtin_##name (rtx target, tree args)                 \
  {                                                                            \
    int nargs = call_expr_nargs (args);                                        \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));                        \
    rtx arg3 = nargs >= 3?                                                     \
               build_arg (CALL_EXPR_ARG (args, 2), #name, lvx_modmap_coherency):\
               gen_rtx_CONST_STRING (VOIDmode, "");                            \
    arg1 = force_reg (tmode, arg1);                                            \
    arg2 = gen_rtx_MEM (mmode, force_reg (Pmode, arg2));                       \
    emit_insn (gen_##name2 (arg1, arg2, arg3));                                \
    return target;                                                             \
  }

#define LVX_EXPAND_BUILTIN_LOAD(name, name2, tmode, mmode)                     \
  static rtx lvx_expand_builtin_##name (rtx target, tree args)                 \
  {                                                                            \
    int nargs = call_expr_nargs (args), volatile_p = 0;                        \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    rtx arg2;                                                                  \
    if (#name[0] == 'x')                                                       \
      arg2 = build_arg (CALL_EXPR_ARG (args, 1), #name, lvx_modmap_xvariant);  \
    else                                                                       \
      arg2 = build_arg (CALL_EXPR_ARG (args, 1), #name, lvx_modmap_variant);   \
    arg1 = gen_rtx_MEM (mmode, force_reg (Pmode, arg1));                       \
    if (nargs > 2)                                                             \
      {                                                                        \
        rtx arg3 = expand_normal (CALL_EXPR_ARG (args, 2));                    \
        arg3 = verify_const_bool_arg (arg3, #name, "third");                   \
        volatile_p |= (INTVAL (arg3) != 0);                                    \
      }                                                                        \
    if (GET_MODE_SIZE (mmode) > 32 && volatile_p)                              \
      error ("volatile not allowed for memory access sizes greater than 32");  \
    MEM_VOLATILE_P (arg1) = volatile_p;                                        \
    if (!target)                                                               \
      target = gen_reg_rtx (tmode);                                            \
    else                                                                       \
      target = force_reg (tmode, target);                                      \
    emit_insn (gen_##name2 (target, arg1, arg2));                              \
    return target;                                                             \
  }

#define LVX_EXPAND_BUILTIN_LOADQ(name, name2, tmode, mmode)                    \
  static rtx lvx_expand_builtin_##name (rtx target, tree args)                 \
  {                                                                            \
    int nargs = call_expr_nargs (args), volatile_p = 0;                        \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));                        \
    rtx arg3 = build_arg (CALL_EXPR_ARG (args, 2), #name, lvx_modmap_variant); \
    arg1 = force_reg (tmode, arg1);                                            \
    arg2 = gen_rtx_MEM (mmode, force_reg (Pmode, arg2));                       \
    if (nargs > 3)                                                             \
      {                                                                        \
        rtx arg4 = expand_normal (CALL_EXPR_ARG (args, 3));                    \
        arg4 = verify_const_bool_arg (arg4, #name, "fourth");                  \
        volatile_p |= (INTVAL (arg4) != 0);                                    \
      }                                                                        \
    if (GET_MODE_SIZE (mmode) > 32 && volatile_p)                              \
      error ("volatile not allowed for memory access sizes greater than 32");  \
    MEM_VOLATILE_P (arg2) = volatile_p;                                        \
    if (!target)                                                               \
      target = gen_reg_rtx (tmode);                                            \
    else                                                                       \
      target = force_reg (tmode, target);                                      \
    emit_insn (gen_##name2 (target, arg1, arg2, arg3));                        \
    return target;                                                             \
  }

/* LVX_SPLIT_LOADCOND takes a loadcond or xloadcond modifier and splits it in
   two parts: the condition part and the variant part.  We assume that the
   modifier is valid.  */

static int
lvx_split_loadcond (const char *_mod, rtx *variant, rtx *cond)
{
  if (!_mod || !variant || !cond)
    return 0;

  *cond = *variant = NULL_RTX;
  char *mod = xstrdup (_mod);
  if (!mod)
    return 0;

  int i = 0;
  if (mod[1] == 'u' || mod[1] == 's')
    {
      char tmp = 0;
      i = 1;
      while (mod[i] && mod[i] != '.')
	i += 1;
      tmp = mod[i];
      mod[i] = 0;
      *variant = gen_rtx_CONST_STRING (VOIDmode, xstrdup (mod));
      mod[i] = tmp;
    }
  else
    *variant = gen_rtx_CONST_STRING (VOIDmode, "");

  *cond = gen_rtx_CONST_STRING (VOIDmode, xstrdup (mod + i));
  free (mod);

  return 1;
}

#define LVX_EXPAND_BUILTIN_LOADC(name, name2, tmode, mmode)                    \
  static rtx lvx_expand_builtin_##name (rtx target, tree args)                 \
  {                                                                            \
    int nargs = call_expr_nargs (args), volatile_p = 0, clear = 0;             \
    machine_mode cmode = GET_MODE_SIZE (mmode) > 64 ? TImode : DImode;         \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));                        \
    rtx arg3 = expand_normal (CALL_EXPR_ARG (args, 2));                        \
    const char *modifier =                                                     \
        lvx_tree_string_constant (CALL_EXPR_ARG (args, 3), #name);             \
    if (#name[0] == 'x')                                                       \
      (void) build_arg (CALL_EXPR_ARG (args, 3), #name, lvx_modmap_xloadcond); \
    else                                                                       \
      (void) build_arg (CALL_EXPR_ARG (args, 3), #name, lvx_modmap_loadcond);  \
    arg1 = force_reg (tmode, arg1);                                            \
    arg2 = gen_rtx_MEM (mmode, force_reg (Pmode, arg2));                       \
    arg3 = force_reg (cmode, arg3);                                            \
    if (nargs > 4)                                                             \
      {                                                                        \
        rtx arg5 = expand_normal (CALL_EXPR_ARG (args, 4));                    \
        arg5 = verify_const_bool_arg (arg5, #name, "fifth");                   \
        volatile_p |= (INTVAL (arg5) != 0);                                    \
      }                                                                        \
    /* At this point we now that the modifier is valid.  */                    \
    rtx cond, variant;                                                         \
    if (!lvx_split_loadcond (modifier, &cond, &variant))                       \
      error (#name ": invalid modifier %<%s%>", modifier);                     \
    if (GET_MODE_SIZE (mmode) > 32 && volatile_p)                              \
      error ("volatile not allowed for memory access sizes greater than 32");  \
    MEM_VOLATILE_P (arg2) = volatile_p;                                        \
    if (!target)                                                               \
      target = gen_reg_rtx (tmode);                                            \
    else                                                                       \
      target = force_reg (tmode, target);                                      \
    if (clear && (strstr (modifier, ".mtc") || strstr (modifier, ".mfc")))     \
      clear = true;                                                            \
    if (clear && !strstr (#name, "1024q"))                                     \
      arg1 = CONST0_RTX (tmode);                                               \
    emit_insn (gen_##name2 (target, arg1, arg2, arg3, cond, variant));         \
    return target;                                                             \
  }

#define LVX_EXPAND_BUILTIN_STORE(name, name2, tmode, mmode)                    \
  static rtx lvx_expand_builtin_##name (rtx target, tree args)                 \
  {                                                                            \
    int nargs = call_expr_nargs (args), volatile_p = 0;                        \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));                        \
    rtx arg3 = const0_rtx;                                                     \
    arg1 = force_reg (tmode, arg1);                                            \
    arg2 = gen_rtx_MEM (mmode, force_reg (Pmode, arg2));                       \
    if (nargs > 2)                                                             \
      {                                                                        \
        arg3 = expand_normal (CALL_EXPR_ARG (args, 2));                        \
        arg3 = verify_const_ready_arg (arg3, #name, "third");                  \
        if (GET_CODE (arg3) == CONST_INT)                                      \
          volatile_p |= (INTVAL (arg3) != 0);                                  \
      }                                                                        \
    if (nargs > 3)                                                             \
      {                                                                        \
        rtx arg4 = expand_normal (CALL_EXPR_ARG (args, 3));                    \
        arg4 = verify_const_bool_arg (arg4, #name, "fourth");                  \
        volatile_p |= (INTVAL (arg4) != 0);                                    \
      }                                                                        \
    if (GET_MODE_SIZE (mmode) > 32 && volatile_p)                              \
      error ("volatile not allowed for memory access sizes greater than 32");  \
    MEM_VOLATILE_P (arg2) = volatile_p;                                        \
    emit_insn (gen_##name2 (arg1, arg2, arg3));                                \
    return target;                                                             \
  }


#define LVX_EXPAND_BUILTIN_STOREC(name, name2, tmode, mmode)                   \
  static rtx lvx_expand_builtin_##name (rtx target, tree args)                 \
  {                                                                            \
    int nargs = call_expr_nargs (args), volatile_p = 0;                        \
    machine_mode cmode = GET_MODE_SIZE (mmode) > 64 ? TImode : DImode;         \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));                        \
    rtx arg3 = expand_normal (CALL_EXPR_ARG (args, 2));                        \
    rtx arg4 = build_arg (CALL_EXPR_ARG (args, 3), #name, lvx_modmap_storecond);\
    rtx arg5 = const0_rtx;                                                     \
    arg1 = force_reg (tmode, arg1);                                            \
    arg2 = gen_rtx_MEM (mmode, force_reg (Pmode, arg2));                       \
    arg3 = force_reg (cmode, arg3);                                            \
    if (nargs > 4)                                                             \
      {                                                                        \
        arg5 = expand_normal (CALL_EXPR_ARG (args, 4));                        \
        arg5 = verify_const_ready_arg (arg5, #name, "fifth");                  \
        if (GET_CODE (arg5) == CONST_INT)                                      \
          volatile_p |= (INTVAL (arg5) != 0);                                  \
      }                                                                        \
    if (nargs > 5)                                                             \
      {                                                                        \
        rtx arg6 = expand_normal (CALL_EXPR_ARG (args, 5));                    \
        arg6 = verify_const_bool_arg (arg6, #name, "sixth");                   \
        volatile_p |= (INTVAL (arg6) != 0);                                    \
      }                                                                        \
    if (GET_MODE_SIZE (mmode) > 32 && volatile_p)                              \
      error ("volatile not allowed for memory access sizes greater than 32");  \
    MEM_VOLATILE_P (arg2) = volatile_p;                                        \
    emit_insn (gen_lvx_##name (arg1, arg2, arg3, arg4, arg5));                 \
    return target;                                                             \
  }


#define LVX_EXPAND_BUILTIN_XPRELOAD(name, name2, bmode, mmode)                 \
  static rtx lvx_expand_builtin_##name (rtx target, tree args)                 \
  {                                                                            \
    int nargs = call_expr_nargs (args), volatile_p = 0;                        \
    rtx arg1 = expand_normal (CALL_EXPR_ARG (args, 0));                        \
    rtx arg2 = expand_normal (CALL_EXPR_ARG (args, 1));                        \
    rtx arg3 = expand_normal (CALL_EXPR_ARG (args, 2));                        \
    rtx arg4 = build_arg (CALL_EXPR_ARG (args, 3), #name, lvx_modmap_xvariant);\
    arg1 = force_reg (bmode, arg1);                                            \
    arg2 = gen_rtx_MEM (mmode, force_reg (Pmode, arg2));                       \
    arg3 = force_reg (DImode, arg3);                                           \
    if (nargs > 4)                                                             \
      {                                                                        \
        rtx arg5 = expand_normal (CALL_EXPR_ARG (args, 4));                    \
        arg5 = verify_const_bool_arg (arg5, #name, "fifth");                   \
        volatile_p |= (INTVAL (arg5) != 0);                                    \
      }                                                                        \
    MEM_VOLATILE_P (arg2) = volatile_p;                                        \
    if (!target)                                                               \
      target = gen_reg_rtx (bmode);                                            \
    else                                                                       \
      target = force_reg (bmode, target);                                      \
    emit_insn (gen_##name2 (target, arg1, arg2, arg3, arg4));                  \
    return target;                                                             \
  }


#define LVX_EXPAND_BUILTIN_UNDEF(name, name2, tmode)                           \
  static rtx lvx_expand_builtin_##name (rtx target, tree args ATTRIBUTE_UNUSED)\
  {                                                                            \
    if (!target)                                                               \
      target = gen_reg_rtx (tmode);                                            \
    else                                                                       \
      target = force_reg (tmode, target);                                      \
    rtx arg1 = GEN_INT (lvx_builtin_undef_counter++);                          \
    emit_insn (gen_##name2 (target, arg1));                                    \
    return target;                                                             \
  }


#define LVX_EXPAND_BUILTIN_XUNDEF(name, name2, tmode)                          \
  static rtx lvx_expand_builtin_##name (rtx target, tree args ATTRIBUTE_UNUSED)\
  {                                                                            \
    if (!target)                                                               \
      target = gen_reg_rtx (tmode);                                            \
    else                                                                       \
      target = force_reg (tmode, target);                                      \
    rtx arg1 = GEN_INT (lvx_builtin_undef_counter++);                          \
    emit_insn (gen_##name2 (target, arg1));                                    \
    return target;                                                             \
  }


#define LVX_EXPAND_BUILTIN_XZERO(name, name2, tmode)                           \
  static rtx lvx_expand_builtin_##name (rtx target, tree args ATTRIBUTE_UNUSED)\
  {                                                                            \
    if (!target)                                                               \
      target = gen_reg_rtx (tmode);                                            \
    else                                                                       \
      target = force_reg (tmode, target);                                      \
    emit_insn (gen_##name2 (target, CONST0_RTX (tmode)));                      \
    return target;                                                             \
  }


#define LVX_BUILTIN_DIRECT_(NAME, FTYPE, OPTIONALS, AVAIL)
#define LVX_BUILTIN_DIRECT_INSN_(NAME, INSN, FTYPE, OPTIONALS, AVAIL)
#define LVX_BUILTIN_INDIRECT_(NAME, FTYPE, OPTIONALS, AVAIL)
#define LVX_BUILTIN_INDIRECT_INSN_CUSTOM_ARGS(NAME, INSN, ...) (NAME, INSN, __VA_ARGS__)
#define LVX_BUILTIN_INDIRECT_INSN_CUSTOM_(NAME, INSN, FTYPE, OPTIONALS, AVAIL, CUSTOM) \
  EXPAND(DEFER(CONCAT2 (LVX_EXPAND_BUILTIN_, LVX_FST CUSTOM) \
               LVX_BUILTIN_INDIRECT_INSN_CUSTOM_ARGS(NAME, INSN, LVX_RST CUSTOM)))
#define LVX_BUILTIN_INDIRECT_CUSTOM_ARGS(NAME, ...) (NAME, CONCAT2 (lvx_, NAME), __VA_ARGS__)
#define LVX_BUILTIN_INDIRECT_CUSTOM_(NAME, FTYPE, OPTIONALS, AVAIL, CUSTOM) \
  EXPAND(DEFER(CONCAT2 (LVX_EXPAND_BUILTIN_, LVX_FST CUSTOM) \
	       LVX_BUILTIN_INDIRECT_CUSTOM_ARGS(NAME, LVX_RST CUSTOM)))

#define LVX_BUILTIN_DIRECT(...) \
  GET_MACRO(__VA_ARGS__,, \
      LVX_BUILTIN_DIRECT_INSN_, \
      LVX_BUILTIN_DIRECT_,,,)(__VA_ARGS__)
#define LVX_BUILTIN_INDIRECT(...) \
  GET_MACRO(__VA_ARGS__, \
      LVX_BUILTIN_INDIRECT_INSN_CUSTOM_, \
      LVX_BUILTIN_INDIRECT_CUSTOM_, \
      LVX_BUILTIN_INDIRECT_,,,)(__VA_ARGS__)
#include "config/lvx/lvx-builtins.def"
#undef LVX_BUILTIN_DIRECT_
#undef LVX_BUILTIN_DIRECT_INSN_
#undef LVX_BUILTIN_INDIRECT_
#undef LVX_BUILTIN_INDIRECT_CUSTOM_ARGS
#undef LVX_BUILTIN_INDIRECT_CUSTOM_
#undef LVX_BUILTIN_INDIRECT_INSN_CUSTOM_ARGS
#undef LVX_BUILTIN_INDIRECT_INSN_CUSTOM_
#undef LVX_BUILTIN_DIRECT
#undef LVX_BUILTIN_INDIRECT

#define LVX_BUILTIN_DIRECT_(NAME, FTYPE, OPTIONALS, AVAIL) \
  { "__builtin_lvx_" #NAME, CODE_FOR_lvx_ ## NAME, LVX_BLTN_DIRECT, \
    LVX_ ## FTYPE, NULL, !ISEMPTY(OPTIONALS), \
    CONCAT3(LVX_,FTYPE,LVX_GET(OPTIONALS)), lvx_builtin_avail_ ## AVAIL },

#define LVX_BUILTIN_DIRECT_INSN_(NAME, INSN, FTYPE, OPTIONALS, AVAIL) \
  { "__builtin_lvx_" #NAME, CODE_FOR_ ## INSN, LVX_BLTN_DIRECT, \
    LVX_ ## FTYPE, NULL, !ISEMPTY(OPTIONALS), \
    CONCAT3(LVX_,FTYPE,LVX_GET(OPTIONALS)), lvx_builtin_avail_ ## AVAIL },

#define LVX_BUILTIN_INDIRECT_(NAME, FTYPE, OPTIONALS, AVAIL) \
  { "__builtin_lvx_" #NAME, CODE_FOR_nothing, LVX_BLTN_INDIRECT, \
    LVX_ ## FTYPE, lvx_expand_builtin_ ## NAME, !ISEMPTY(OPTIONALS), \
    CONCAT3(LVX_,FTYPE,LVX_GET(OPTIONALS)), lvx_builtin_avail_ ## AVAIL },

#define LVX_BUILTIN_INDIRECT_CUSTOM_(NAME, FTYPE, OPTIONALS, AVAIL, CUSTOM) \
  { "__builtin_lvx_" #NAME, CODE_FOR_nothing, LVX_BLTN_INDIRECT, \
    LVX_ ## FTYPE, lvx_expand_builtin_ ## NAME, !ISEMPTY(OPTIONALS), \
    CONCAT3(LVX_,FTYPE,LVX_GET(OPTIONALS)), lvx_builtin_avail_ ## AVAIL },

#define LVX_BUILTIN_INDIRECT_INSN_CUSTOM_(NAME, INSN, FTYPE, OPTIONALS, AVAIL, CUSTOM) \
  { "__builtin_lvx_" #NAME, CODE_FOR_nothing, LVX_BLTN_INDIRECT, \
    LVX_ ## FTYPE, lvx_expand_builtin_ ## NAME, !ISEMPTY(OPTIONALS), \
    CONCAT3(LVX_,FTYPE,LVX_GET(OPTIONALS)), lvx_builtin_avail_ ## AVAIL },

#define LVX_BUILTIN_DIRECT(...) \
  GET_MACRO(__VA_ARGS__,, \
      LVX_BUILTIN_DIRECT_INSN_, \
      LVX_BUILTIN_DIRECT_,,,) (__VA_ARGS__)
#define LVX_BUILTIN_INDIRECT(...) \
  GET_MACRO(__VA_ARGS__, \
      LVX_BUILTIN_INDIRECT_INSN_CUSTOM_, \
      LVX_BUILTIN_INDIRECT_CUSTOM_, \
      LVX_BUILTIN_INDIRECT_,,,)(__VA_ARGS__)

static const struct lvx_builtin_desc lvx_builtins[] = {
#include "config/lvx/lvx-builtins.def"
};
#undef LVX_BUILTIN_DIRECT
#undef LVX_BUILTIN_DIRECT_INSN
#undef LVX_BUILTIN_INDIRECT
#undef LVX_BUILTIN_INDIRECT_CUSTOM


static tree lvx_builtin_decls[ARRAY_SIZE (lvx_builtins)];
//static GTY(()) int lvx_builtin_decl_index[NUM_INSN_CODES];

void
lvx_init_builtins (void)
{
  LVX_ATYPE_STRING = build_pointer_type (
    build_qualified_type (char_type_node, TYPE_QUAL_CONST));

  LVX_ATYPE_V8QI  = build_opaque_vector_type (LVX_ATYPE_INT8, 8);
  LVX_ATYPE_V16QI = build_opaque_vector_type (LVX_ATYPE_INT8, 16);
  LVX_ATYPE_V32QI = build_opaque_vector_type (LVX_ATYPE_INT8, 32);
  LVX_ATYPE_V64QI = build_opaque_vector_type (LVX_ATYPE_INT8, 64);

  LVX_ATYPE_V4HI  = build_opaque_vector_type (LVX_ATYPE_INT16, 4);
  LVX_ATYPE_V8HI  = build_opaque_vector_type (LVX_ATYPE_INT16, 8);
  LVX_ATYPE_V16HI = build_opaque_vector_type (LVX_ATYPE_INT16, 16);
  LVX_ATYPE_V32HI = build_opaque_vector_type (LVX_ATYPE_INT16, 32);

  LVX_ATYPE_V2SI  = build_opaque_vector_type (LVX_ATYPE_INT32, 2);
  LVX_ATYPE_V4SI  = build_opaque_vector_type (LVX_ATYPE_INT32, 4);
  LVX_ATYPE_V8SI  = build_opaque_vector_type (LVX_ATYPE_INT32, 8);
  LVX_ATYPE_V16SI = build_opaque_vector_type (LVX_ATYPE_INT32, 16);

  LVX_ATYPE_V2DI  = build_opaque_vector_type (LVX_ATYPE_INT64, 2);
  LVX_ATYPE_V4DI  = build_opaque_vector_type (LVX_ATYPE_INT64, 4);
  LVX_ATYPE_V8DI  = build_opaque_vector_type (LVX_ATYPE_INT64, 8);

  LVX_ATYPE_V4HF  = build_opaque_vector_type (LVX_ATYPE_FLOAT16, 4);
  LVX_ATYPE_V8HF  = build_opaque_vector_type (LVX_ATYPE_FLOAT16, 8);
  LVX_ATYPE_V16HF = build_opaque_vector_type (LVX_ATYPE_FLOAT16, 16);
  LVX_ATYPE_V32HF = build_opaque_vector_type (LVX_ATYPE_FLOAT16, 32);

  LVX_ATYPE_V2SF  = build_opaque_vector_type (LVX_ATYPE_FLOAT32, 2);
  LVX_ATYPE_V4SF  = build_opaque_vector_type (LVX_ATYPE_FLOAT32, 4);
  LVX_ATYPE_V8SF  = build_opaque_vector_type (LVX_ATYPE_FLOAT32, 8);
  LVX_ATYPE_V16SF = build_opaque_vector_type (LVX_ATYPE_FLOAT32, 16);

  LVX_ATYPE_V2DF  = build_opaque_vector_type (LVX_ATYPE_FLOAT64, 2);
  LVX_ATYPE_V4DF  = build_opaque_vector_type (LVX_ATYPE_FLOAT64, 4);
  LVX_ATYPE_V8DF  = build_opaque_vector_type (LVX_ATYPE_FLOAT64, 8);

  add_builtin_type ("__lvx_v8qi", LVX_ATYPE_V8QI);
  add_builtin_type ("__lvx_v16qi", LVX_ATYPE_V16QI);
  add_builtin_type ("__lvx_v32qi", LVX_ATYPE_V32QI);
  add_builtin_type ("__lvx_v64qi", LVX_ATYPE_V64QI);

  add_builtin_type ("__lvx_v4hi", LVX_ATYPE_V4HI);
  add_builtin_type ("__lvx_v8hi", LVX_ATYPE_V8HI);
  add_builtin_type ("__lvx_v16hi", LVX_ATYPE_V16HI);
  add_builtin_type ("__lvx_v32hi", LVX_ATYPE_V32HI);

  add_builtin_type ("__lvx_v2si", LVX_ATYPE_V2SI);
  add_builtin_type ("__lvx_v4si", LVX_ATYPE_V4SI);
  add_builtin_type ("__lvx_v8si", LVX_ATYPE_V8SI);
  add_builtin_type ("__lvx_v16si", LVX_ATYPE_V16SI);

  add_builtin_type ("__lvx_v2di", LVX_ATYPE_V2DI);
  add_builtin_type ("__lvx_v4di", LVX_ATYPE_V4DI);
  add_builtin_type ("__lvx_v8di", LVX_ATYPE_V8DI);

  add_builtin_type ("__lvx_v4hf", LVX_ATYPE_V4HF);
  add_builtin_type ("__lvx_v8hf", LVX_ATYPE_V8HF);
  add_builtin_type ("__lvx_v16hf", LVX_ATYPE_V16HF);
  add_builtin_type ("__lvx_v32hf", LVX_ATYPE_V32HF);

  add_builtin_type ("__lvx_v2sf", LVX_ATYPE_V2SF);
  add_builtin_type ("__lvx_v4sf", LVX_ATYPE_V4SF);
  add_builtin_type ("__lvx_v8sf", LVX_ATYPE_V8SF);
  add_builtin_type ("__lvx_v16sf", LVX_ATYPE_V16SF);

  add_builtin_type ("__lvx_v2df", LVX_ATYPE_V2DF);
  add_builtin_type ("__lvx_v4df", LVX_ATYPE_V4DF);
  add_builtin_type ("__lvx_v8df", LVX_ATYPE_V8DF);

  LVX_ATYPE_DI = make_unsigned_type (64);
  LVX_ATYPE_V64  = build_opaque_vector_type (LVX_ATYPE_DI, 1);
  LVX_ATYPE_V128 = build_opaque_vector_type (LVX_ATYPE_DI, 2);
  LVX_ATYPE_V256 = build_opaque_vector_type (LVX_ATYPE_DI, 4);
  LVX_ATYPE_V512 = build_opaque_vector_type (LVX_ATYPE_DI, 8);

  add_builtin_type ("__lvx_v64", LVX_ATYPE_V64);
  add_builtin_type ("__lvx_v128", LVX_ATYPE_V128);
  add_builtin_type ("__lvx_v256", LVX_ATYPE_V256);
  add_builtin_type ("__lvx_v512", LVX_ATYPE_V512);

  LVX_ATYPE_OI = make_unsigned_type (256);
  SET_TYPE_ALIGN (LVX_ATYPE_OI, 256);
  SET_TYPE_MODE (LVX_ATYPE_OI, OImode);
  layout_type (LVX_ATYPE_OI);
  lang_hooks.types.register_builtin_type (LVX_ATYPE_OI, "__oi");

  /* Support for _Float16.  */
  tree lvx_float16_type_node = make_node (REAL_TYPE);
  TYPE_PRECISION (lvx_float16_type_node) = 16;
  SET_TYPE_MODE (lvx_float16_type_node, HFmode);
  layout_type (lvx_float16_type_node);

  if (!maybe_get_identifier ("_Float16"))
    lang_hooks.types.register_builtin_type (lvx_float16_type_node,
					    "_Float16");

  LVX_ATYPE_X256  = build_vector_type (LVX_ATYPE_OI, 1);
  LVX_ATYPE_X512  = build_vector_type (LVX_ATYPE_OI, 2);
  LVX_ATYPE_X1024 = build_vector_type (LVX_ATYPE_OI, 4);
  LVX_ATYPE_X2048 = build_vector_type (LVX_ATYPE_OI, 8);
  LVX_ATYPE_X4096 = build_vector_type (LVX_ATYPE_OI, 16);
  LVX_ATYPE_X8192 = build_vector_type (LVX_ATYPE_OI, 32);

  LVX_ATYPE_PX256 = build_pointer_type (LVX_ATYPE_X256);

  add_builtin_type ("__lvx_x256", LVX_ATYPE_X256);
  add_builtin_type ("__lvx_x512", LVX_ATYPE_X512);
  add_builtin_type ("__lvx_x1024", LVX_ATYPE_X1024);
  add_builtin_type ("__lvx_x2048", LVX_ATYPE_X2048);
  add_builtin_type ("__lvx_x4096", LVX_ATYPE_X4096);
  add_builtin_type ("__lvx_x8192", LVX_ATYPE_X8192);

  for (int i = 0; i < (int) ARRAY_SIZE (lvx_builtins); ++i)
    {
      const struct lvx_builtin_desc *d = &lvx_builtins[i];
      if (d->avail ())
        {
          tree type = lvx_build_function_type (d->prototype, d->variadic_p);
          lvx_builtin_decls[i] = add_builtin_function (d->name, type, i, BUILT_IN_MD, NULL, NULL);
          //	lvx_builtin_decl_index[d->icode] = i;
        }
    }
  lvx_modmap_loadcond  = &lvx_modmap_loadcond_lvx_1;
  lvx_modmap_xloadcond = &lvx_modmap_xloadcond_lvx_1;
  lvx_modmap_storecond = &lvx_modmap_storecond_lvx_1;
  lvx_modmap_xvariant  = &lvx_modmap_xvariant_lvx_1;
  lvx_modmap_xloadq    = &lvx_modmap_xloadq_lvx_1;
  lvx_modmap_boolcasco = &lvx_modmap_boolcasco_lvx_1;
  lvx_modmap_accesses  = &lvx_modmap_accesses_lvx_1;
  lvx_modmap_cachelev  = &lvx_modmap_cachelev_lvx_1;
  lvx_modmap_xloadqc   = &lvx_modmap_xloadqc_lvx_1;
  lvx_modmap_floatings = &lvx_modmap_floatings_lvx_1;
  lvx_modmap_conjugate = &lvx_modmap_conjugate_lvx_1;
}

tree
lvx_builtin_decl (unsigned code, bool initialize_p ATTRIBUTE_UNUSED)
{
  if (code >= ARRAY_SIZE (lvx_builtins))
    return error_mark_node;
  return lvx_builtin_decls[code];
}


rtx
lvx_expand_builtin (tree exp, rtx target, rtx subtarget ATTRIBUTE_UNUSED,
		    enum machine_mode mode ATTRIBUTE_UNUSED,
		    int ignore ATTRIBUTE_UNUSED)
{
  tree fndecl = TREE_OPERAND (CALL_EXPR_FN (exp), 0);
  int nops = call_expr_nargs (exp);
  unsigned int fcode = DECL_MD_FUNCTION_CODE (fndecl);
  int has_target_p = true;

//  int args_idx = 0;
//  tree t = NULL_TREE;
//  function_args_iterator it;
  const struct lvx_modmap *mod = lvx_get_modifiers (lvx_builtins[fcode].variadic_prototype);
  if (TREE_TYPE (TREE_TYPE (lvx_builtin_decls[fcode])) == LVX_ATYPE_VOID)
      has_target_p = false;
//  FOREACH_FUNCTION_ARGS (TREE_TYPE (lvx_builtin_decls[fcode]), t, it)
//  {
//
//    if (args_idx >= nops)
//      break;
//
//    tree cur_arg = CALL_EXPR_ARG (exp, args_idx);
//    if (TYPE_MODE (TREE_TYPE (cur_arg)) != TYPE_MODE (t))
//        error ("type error");
//
//    tree offset_tree = 0;
//    tree maybe_mod = string_constant (cur_arg, &offset_tree, 0, 0);
//    if (maybe_mod)
//      build_arg (cur_arg, lvx_builtins[fcode].name, mod);
//
//    args_idx += 1;
//  }

  if (lvx_builtins[fcode].type == LVX_BLTN_INDIRECT)
    return lvx_builtins[fcode].expand (target, exp);
  else if (lvx_builtins[fcode].type == LVX_BLTN_DIRECT)
    {
      struct expand_operand ops[MAX_RECOG_OPERANDS];
      rtx argrtx[MAX_RECOG_OPERANDS];
      int opno = 0;
      enum insn_code icode = lvx_builtins[fcode].icode;
      if (has_target_p)
	create_output_operand (&ops[opno++], target, TYPE_MODE (TREE_TYPE (exp)));

      nops += !!has_target_p;

      /* The pattern this builtin names can be compiled out -- its
	 HAVE_LVX_* gate is (0) because the instruction left the ISA -- and
	 then icode is CODE_FOR_nothing and insn_data[icode] describes
	 nothing at all.  The assert below read n_generator_args out of it
	 and crashed on the mismatch; say what is actually wrong instead.  */
      if (icode == CODE_FOR_nothing)
	{
	  error ("%<%s%> is not supported on this target",
		 lvx_builtins[fcode].name);
	  if (!has_target_p)
	    return const0_rtx;
	  return target ? target : gen_reg_rtx (TYPE_MODE (TREE_TYPE (exp)));
	}

      gcc_assert (opno + call_expr_nargs (exp)
		  == insn_data[icode].n_generator_args);
      for (int argno = 0; argno < call_expr_nargs (exp); argno++)
	{
	  tree arg = CALL_EXPR_ARG (exp, argno);
	  tree offset_tree = 0;
	  tree maybe_mod = string_constant (arg, &offset_tree, 0, 0);
	  rtx thing;
	  if (maybe_mod)
	    thing = build_arg (arg, lvx_builtins[fcode].name, mod);
	  else
	    thing = expand_normal (arg);

	  argrtx[argno] = thing;
	  create_input_operand (&ops[opno++], thing, TYPE_MODE (TREE_TYPE (arg)));
	}
      if (!maybe_expand_insn (icode, nops, ops))
	{
	  /* The arguments do not fit the pattern.  This is a mistake in the
	     source rather than an impossibility -- typically an operand the
	     instruction takes as an immediate given a variable, or given a
	     constant it cannot encode -- so say which argument, rather than
	     crashing on the gcc_unreachable that used to be here.  */
	  int bad = -1;
	  for (int argno = 0; argno < call_expr_nargs (exp); argno++)
	    if (!insn_operand_matches (icode, (!!has_target_p) + argno,
				       argrtx[argno]))
	      {
		bad = argno;
		break;
	      }

	  if (bad >= 0)
	    error ("argument %d to %<%s%> is not valid for this instruction; "
		   "the operand must be a constant the instruction can encode",
		   bad + 1, lvx_builtins[fcode].name);
	  else
	    error ("%<%s%> cannot be expanded with these arguments",
		   lvx_builtins[fcode].name);

	  /* Diagnosed, so the compilation fails -- but the caller uses this
	     result, and it has to be of the mode it asked for.  */
	  if (!has_target_p)
	    return const0_rtx;
	  return target ? target : gen_reg_rtx (TYPE_MODE (TREE_TYPE (exp)));
	}

      return has_target_p ? ops[0].value : const0_rtx;
    }
  else
    {
      gcc_unreachable ();
    }

  //internal_error ("unknown builtin code");
  return NULL_RTX;
}

tree
lvx_resolve_overloaded_builtin (location_t ARG_UNUSED (loc),
				tree ob_fndecl,
				void *passed_arglist,
				bool ARG_UNUSED (complain))
{
  (void) loc;
  (void) ob_fndecl;
  (void) passed_arglist;
   return NULL_TREE;
}

