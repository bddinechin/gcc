/* Machine description for LVX Machine architecture.
   Copyright (C) 2018 Free Software Foundation, Inc.

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

#ifndef GCC_LVX_MPPA_LINUX
#define GCC_LVX_MPPA_LINUX

/* 18.2 Controlling the Compilation Driver {{{  */

#define DRIVER_SELF_SPECS DRIVER_SELF_SPECS_COMMON

#undef CPP_SPEC
#define CPP_SPEC "%{pthread:-D_REENTRANT}"

#define LINK_SPEC                                                              \
  "%{h*}		\
   %{static:-Bstatic}				\
   %{shared:-shared}				\
   %{symbolic:-Bsymbolic}			\
   %{!static:					\
     %{rdynamic:-export-dynamic}		\
     %{!shared:-dynamic-linker " GNU_USER_DYNAMIC_LINKER64 "}} \
   -X"

/* 18.2 Controlling the Compilation Driver }}}  */


/* 18.3 Run-time Target Specification {{{  */
#define TARGET_OS_CPP_BUILTINS()                                               \
                                                                               \
  do                                                                           \
    {                                                                          \
      GNU_USER_TARGET_OS_CPP_BUILTINS ();                                      \
    }                                                                          \
  while (0)

/* 18.3 Run-time Target Specification }}}  */


/* 18.5 Storage Layout {{{  */

/* Do not force alignment on word boundaries on linux. */
#undef DATA_ALIGNMENT

/* Align at least on BITS_PER_WORD for local variables. */
#undef LOCAL_ALIGNMENT
#define LOCAL_ALIGNMENT(EXP, ALIGN)                                            \
  ((ALIGN) < BITS_PER_WORD ? BITS_PER_WORD : (ALIGN))

/* 18.5 Storage Layout }}}  */


/* 18.9 Stack Layout and Calling Conventions {{{ */

/* 18.9.12 Generating Code for Profiling {{{ */

#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER(FILE, LABELNO) \
    lvx_output_function_profiler (FILE)

#undef PROFILE_HOOK

#define NO_PROFILE_COUNTERS 1

/* This is a macro unique to the LVX port, used in the function
   LVX_OUTPUT_FUNCTION_PROFILER.  */
#define PROFILE_REGNO (32)

/* 18.9.12 Generating Code for Profiling }}} */

/* 18.9 Stack Layout and Calling Conventions }}} */

#define GLIBC_DYNAMIC_LINKER64 "/lib/ld-linux-lvx.so.1"
#undef MUSL_DYNAMIC_LINKER64
#define MUSL_DYNAMIC_LINKER64 "/lib/ld-musl-lvx.so.1"

/* Needed for dso_handle defined libgcc/crtstuff.c
 * configure option --enable-__cxa_atexit is applicable to host and not target.
 * Since gcc 10
 */
#define DEFAULT_USE_CXA_ATEXIT 2

#endif
