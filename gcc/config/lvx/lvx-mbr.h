/* Machine description for LVX Machine architecture.
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

#define CPP_SPEC "-D__machine_bare_runtime__"

#undef LINK_SPEC
#define LINK_SPEC LINK_SPEC_COMMON

/* Link against Newlib, because the bare (elf) backend assumes it: libc plus
   libgloss, with the circular dependence between the two resolved by the
   group.  libgcc is added by the driver.  This part can be modified for OS
   porting and other libc.

   There is deliberately no bare-runtime library and no linker script here.
   The KVX-derived spec this replaces named -lmppahal, -lmppabareruntime and
   mppabareruntime.ld; none of the three exists for LVX, so every default link
   failed:

     cannot find -lmppahal
     cannot find -lmppabareruntime
     unable to locate default linker script 'mppabareruntime.ld'

   An LVX machine bare runtime needs a physical memory map and an exception
   vector layout, neither of which is defined yet.  Until then ld's built-in
   script is the right default: it is what the gem5 SE-mode harness already
   links with, and SE mode emulates the scall interface itself, so none of the
   boot/exception/MMU machinery a real bare runtime provides is reachable.

   When that runtime does exist, add its library and script back here.  The
   KVX spec picked between two scripts on -mhal; that option selected nothing
   else, so it went with them.  */
#undef LIB_SPEC
#define LIB_SPEC                                                               \
  "-z max-page-size=4096 "                                                     \
  "--start-group -lc -lgloss --end-group "

#define STARTFILE_SPEC " crti%O%s crtbegin%O%s crt0%O%s"

#define ENDFILE_SPEC " crtend%O%s crtn%O%s"
/* 18.2 Controlling the Compilation Driver }}}  */

#endif /* GCC_LVX_MPPA_MBR */
