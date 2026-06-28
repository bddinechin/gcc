/* Machine description for LVX MPPA architecture.
   Copyright (C) 2018 Kalray Inc.

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

#ifndef GCC_LVX_MPPA_MBR
#define GCC_LVX_MPPA_MBR

/* 18.2 Controlling the Compilation Driver {{{  */

#define DRIVER_SELF_SPECS DRIVER_SELF_SPECS_COMMON

#define CPP_SPEC "%{!mhal: -D__mppa_bare_runtime__}"

#undef LINK_SPEC
#define LINK_SPEC LINK_SPEC_COMMON

/* Link against Newlib libraries, because the bare (elf) backend assumes Newlib.
   This part can be modified for OS porting and other libc.
   Handle the circular dependence between libc and libgloss.
   Link against MPPA Bare Runtime
 */
#undef LIB_SPEC
#define LIB_SPEC                                                               \
  "-z max-page-size=4096 "                                                     \
  "%{!mhal: --defsym=exec_on_main=1 --defsym=mppa_bare_runtime=1} --start-group -lmppahal -lmppabareruntime -lc -lgloss --end-group "         \
  "%{!nostartfiles:%{!nodefaultlibs:%{!nostdlib:%{!T*: %{!mhal: %Tmppabareruntime.ld;: %Tbare.ld}} }} }"

#define STARTFILE_SPEC " crti%O%s crtbegin%O%s crt0%O%s"

#define ENDFILE_SPEC " crtend%O%s crtn%O%s"
/* 18.2 Controlling the Compilation Driver }}}  */

#endif /* GCC_LVX_MPPA_MBR */
