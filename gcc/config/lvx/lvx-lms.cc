/* Late Modulo Scheduling for LVX: the framework.
   Copyright (C) 2026 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* Everything a modulo scheduler needs except the scheduling: loop admission
   and preconditioning, the dependence graph, ResMII and RecMII, the
   resource-free relaxations (Estart/Lstart and the minimum cumulative
   register lifetime), the resource model, acceptance, live ranges, register
   allocation and renaming, and the construction and emission of the software
   pipeline.  The engines -- CMS in lvx-cms.cc, and SMS and an integer program
   intended to follow -- decide DATES at a given initiation interval and
   nothing else.

   The interface in both directions is lvx-lms.h, and it is deliberately
   small: a rule that is about the PROGRAM belongs here and reaches an engine
   as an arc or as an acceptance refusal, so that no engine has to know it
   exists.

   Moved here from lvx.cc unchanged.  What stayed behind is the bundle and
   group tagging, which is the EMISSION's partner rather than the
   pipeliner's.  */

#define IN_TARGET_CODE 1

#include "config.h"

/* Asks "system.h" to include the c++ header file `string'.  */
#define INCLUDE_STRING

/* Base headers which need to be included before everything else.  */
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "rtl.h"
#include "tree.h"
#include "memmodel.h"
#include "tm_p.h"

#include "attribs.h"
#include "builtins.h"
#include "calls.h"
#include "cfganal.h"
#include "cfgcleanup.h"
#include "cfghooks.h"
#include "cfgloop.h"
#include "cfgrtl.h"
#include "cfgloopmanip.h"
#include "cgraph.h"
#include "ddg.h"
#include "df.h"
#include "diagnostic.h"
#include "dumpfile.h"
#include "dwarf2.h"
#include "emit-rtl.h"
#include "explow.h"
#include "expmed.h"
#include "expr.h"
#include "flags.h"
#include "fold-const.h"
#include "gimple.h"
#include "gimplify.h"
#include "hw-doloop.h"
#include "ifcvt.h"
#include "insn-attr.h"
#include "langhooks.h"
#include "optabs.h"
#include "opts.h"
#include "output.h"
#include "predict.h"
#include "profile.h"
#include "recog.h"
#include "regs.h"
#include "selftest.h"
#include "function-abi.h"
#include "regrename.h"
#include "config/lvx/lvx-plc.h"
#include "config/lvx/lvx-network.h"
#include "reload.h"
#include "rtl-iter.h"
#include "sched-int.h"
#include "sel-sched.h"
#include "stor-layout.h"
#include "stringpool.h"
#include "tm-constrs.h"
#include "tree-vectorizer.h"
#include "varasm.h"

#define NULL_BLOCK ((basic_block) NULL)

/* This file should be included last.  */
#include "target-def.h"
#include "targhooks.h"
#include "config/lvx/lvx-issue-table.inc"
#include "config/lvx/lvx-lms.h"

static const char *const lvx_lms_arc_kind_name[] = {
  "reg-raw", "reg-war", "reg-waw", "mem-intra", "mem-cross",
  "ctrl-order", "ctrl-store", "ctrl-liveout", "ctrl-backedge"
};


/* LMS AND CMS, AND WHY THERE ARE TWO PREFIXES BELOW.

   LMS is LATE MODULO SCHEDULING: the framework, late because it runs after
   register allocation and after the hardware-loop mapping.  Everything an
   engine needs but does not decide is `lvx_lms_' -- loop admission and
   preconditioning, ResMII and RecMII, the dependence graph, the relaxations
   (Estart/Lstart, MCRL), the resource model and the acceptance oracle
   (lvx_lms_verify plus the colouring), live ranges, register allocation and
   the pipeline construction and emission.

   CMS is CONVERGENT MODULO SCHEDULING: one engine, and only six functions of
   it -- lvx_cms_converge, which schedules unwound copies against a monotone
   clock and projects, and lvx_cms_swo_at_ii, which drives it through a
   Squeaky Wheel cycle, with earliest_cycle, rnd and rwc_weight in support.
   Its own options keep the `-mcms-' spelling and the `lvx_cms_' prefix.

   The line between them is the question "would another engine decide this
   differently?".  A rule that is about the PROGRAM belongs in LMS and every
   engine then inherits it -- the tainted-load rule of §7tricies.221 is the
   worked example, since it lands in lvx_lms_build_graph as ordinary arcs and
   an engine never learns it exists.  A rule that is about the SEARCH belongs
   in CMS.

   THE OPTIONS FOLLOW THE SAME LINE.  The forty-five generic ones are
   `-mlms-' since §7tricies.223, the nineteen that steer CMS's own search are
   still `-mcms-', and every old spelling survives as an Alias() so that the
   command lines recorded in docs/ and in commit messages still run -- a
   measurement that cannot be re-run is not a record.  A retired option named
   in a comment below keeps the spelling it had when it existed.

   The dump prefix is still `;; lvx cms:'.  It is not an option and no harness
   keys on it -- they match the field names -- but leaving it alone is what
   lets each of these steps be verified by a BYTE-IDENTICAL census rather than
   by reading.

   The comments below still say CMS in many places where they now mean LMS.
   That is a reading pass, not a correctness one.  */

/* THE TRIP COUNT, RECOVERED AFTER RELOAD.  §7tricies.84.

   get_max_loop_iterations needs loop->any_upper_bound, which record_niter_bound
   sets at TREE level, while loop_optimizer_init here re-discovers loops from the
   CFG and hands back fresh structures.  Measured: maxtrip was -1 for all 294
   loop analyses in TSVC2 and all three in FFT -- the field is never set by the
   time CMS runs, so every rule that needs a trip count was dead.

   What survives reload is the DOLOOP COUNTER.  LVX's doloop_end_<mode> tests
   `(ne (match_operand 0) (const_int 1))' and decrements match_dup 0, and that
   register holds the iterations still to start (§7tricies.62) -- so when the
   loop's entry block initialises it from a constant, that constant IS the trip
   count.  The pattern is still present here because CMS deliberately runs
   before reorg_loops.

   Deliberately narrow, because a wrong trip count is a miscompile and a missing
   one only costs an optimisation: a single_set of the counter to a CONST_INT in
   the entry block, nothing traced further back, nothing reconstructed from an
   exit test.  A loop with no doloop, a counter set from a register, or an
   initialisation further away all return -1 and the callers behave as they did
   when nothing was known.  */

static HOST_WIDE_INT
lvx_lms_doloop_trip_count (class loop *loop, basic_block *body)
{
  rtx cnt = NULL_RTX;
  basic_block entry = NULL;
  edge e;
  edge_iterator ei;

  /* THE COUNTER IS WHAT THE HW-DOLOOP CALLBACK SAYS IT IS.  reorg_loops gets
     it from `hooks->end_pattern_reg', which for LVX is lvx_hwloop_pattern_reg:
     it recognises the insn by CODE_FOR_doloop_end_si/di rather than by pattern
     shape, and takes the register from the DECREMENT's SET_DEST, which is where
     the counter is defined.  Calling the same callback here is not tidiness --
     it makes CMS and reorg_loops agree on which insn ends the loop and which
     register counts it, by construction rather than by two derivations that
     can drift.  The REG_P test inside it also covers doloop_end_<mode>'s
     second alternative, which allows the counter in memory.  */
  for (unsigned i = 0; i < loop->num_nodes && cnt == NULL_RTX; i++)
    cnt = lvx_hwloop_pattern_reg (BB_END (body[i]));
  if (cnt == NULL_RTX)
    return -1;

  /* The entry edge, found rather than taken from loop_preheader_edge: CMS runs
     loop_optimizer_init with AVOID_CFG_MODIFICATIONS, so there may be no
     preheader to ask for.  */
  FOR_EACH_EDGE (e, ei, loop->header->preds)
    if (!flow_bb_inside_loop_p (loop, e->src))
      {
	if (entry)
	  return -1;		/* two entries: no single initialisation */
	entry = e->src;
      }
  if (!entry)
    return -1;

  for (rtx_insn *insn = BB_END (entry);
       insn && insn != PREV_INSN (BB_HEAD (entry));
       insn = PREV_INSN (insn))
    {
      rtx set;

      if (!NONDEBUG_INSN_P (insn))
	continue;
      set = single_set (insn);
      if (!set || !REG_P (SET_DEST (set))
	  || REGNO (SET_DEST (set)) != REGNO (cnt))
	{
	  /* Any other write to the counter in between makes the constant below
	     the wrong one, so stop rather than look past it.  */
	  if (reg_set_p (cnt, insn))
	    return -1;
	  continue;
	}
      if (!CONST_INT_P (SET_SRC (set)) || INTVAL (SET_SRC (set)) <= 0)
	return -1;
      return INTVAL (SET_SRC (set));
    }
  return -1;
}

/* The LOOPDO reorg_loops has ALREADY put on LOOP, or NULL.  §7tricies.196.

   Only non-NULL when CMS runs second (-mcms-after-hwloop), and then it is the
   whole of the hardware-loop decision: an insn here means the loop IS mapped,
   its doloop_end survives as a decoration over an intact software loop
   (§7tricies.85) and emits no instruction, so charging it is wrong; its
   absence means either the loop never was a doloop or lvx_hwloop_fail has
   already converted it to decrement-test-branch, and then the latch is real.

   Searched in the block the loop is entered from, which is where
   lvx_hwloop_optimize emits it -- and the entry is found by hand for the same
   reason lvx_lms_pipeline_loop finds its own: CMS runs loop_optimizer_init
   with AVOID_CFG_MODIFICATIONS, so there may be no preheader to ask for.  */
static rtx_insn *
lvx_lms_find_loopdo (class loop *loop)
{
  basic_block entry = NULL;
  rtx_insn *insn;
  edge e;
  edge_iterator ei;

  if (loop->header == NULL)
    return NULL;
  FOR_EACH_EDGE (e, ei, loop->header->preds)
    if (!flow_bb_inside_loop_p (loop, e->src))
      {
	if (entry != NULL)
	  return NULL;		/* two entries: not a hardware loop's shape */
	entry = e->src;
      }
  if (entry == NULL)
    return NULL;
  FOR_BB_INSNS_REVERSE (entry, insn)
    if (NONDEBUG_INSN_P (insn)
	&& recog_memoized (insn) == CODE_FOR_lvx_loopdo)
      return insn;
  return NULL;
}

/* The latency GCC would charge a dependence of DEP_TYPE from PROD to CONS,
   composed as haifa-sched.cc's dep_cost_1 does, and in particular passed
   through TARGET_SCHED_ADJUST_COST.  That hook is not a refinement here: it
   zeroes the cost of an arc into a branch that does not test the produced
   value, and a superblock body ends every one of its blocks in a branch, so
   taking insn_default_latency alone -- as a first version of this did --
   charges full producer latency on a great many arcs that cost nothing, and
   inflates both the height and the recurrence bound.

   The base is taken directly rather than from insn_sched_cost, which caches
   into the haifa insn data and so may only be called inside the scheduler.
   For the same reason note that lvx_sched_adjust_cost's REG_DEP_ANTI branch
   walks SD_LIST_BACK when the producer is a CLOBBER.  That is unreachable here
   three times over -- lvx_lms_build_graph skips USE and CLOBBER patterns, skips
   anything with recog_memoized < 0, and the guard below returns 0 when either
   side fails recog_memoized, which a CLOBBER always does -- but it is why this
   function must not be handed arbitrary insns.

   DO NOT replace this with dep_cost.  It was considered and rejected
   (2026-10-22, docs/cms-foundations.md §7sedecies.8).  dep_cost takes a dep_t,
   which exists only in sched_analyze's lists, and those carry the omega = 0
   dependences and nothing else; CMS is the single-copy design and computes
   omega itself from reaching definitions, so sourcing arcs from sched_analyze
   would mean giving that up and going back to two loop copies, which cannot
   represent omega > 1.  And the substitution is measured as a null change:
   §7nonies ran CMS on the SMS ddg, whose latencies are dep_cost-derived, and
   the recurrence bound agreed on 336 of 336 loops.  */

/* Dump to DUMP_FILE if there is one.

   The CMS probe was written when `-fdump-rtl-mach' was its only entry, so
   DUMP_FILE could be assumed non-null after the one early return at the top
   of lvx_lms_pipeline_loops.  `-mlms-codegen' broke that assumption: it runs the
   probe for its side effect -- the schedule -- with no dump requested, and
   the first unguarded fprintf then takes a null FILE * into vfprintf and
   segfaults.  The compiler died at pipe.c:10 that way while trmm stayed
   clean, because trmm was always compiled with a dump.

   One guard in one place, rather than twenty `if (dump_file)' at the call
   sites: the probe's dumping is unconditional by design and the condition
   belongs to the sink, not to each caller.  */

void ATTRIBUTE_PRINTF_1
lvx_lms_dump (const char *fmt, ...)
	      {
	      va_list ap;

  if (!dump_file)
    return;

  va_start (ap, fmt);
  vfprintf (dump_file, fmt, ap);
  va_end (ap);
}

/* How much longer -mlms-load-latency says this instruction's result takes than
   the machine description does, or 0.

   A load carries a "variant" (cached, speculate, uncached, preload) and the
   description prices them apart: a cached or speculative load is 3 cycles (4
   for an xload) while an UNCACHED one is 15, because it commits at SR, the last
   pipeline stage.  The probe otherwise sees whatever GCC emitted, which is the
   cached form -- so every figure in §7septies through §7undecies is a
   cached-memory figure.  A kernel streaming data it will not reuse, or one
   whose speculated loads can only be had as `.us' (uncached AND dismissible),
   pays 15.  This lever charges that so the sensitivity can be measured rather
   than guessed; see §7septies.8.

   Returned as a DELTA added to whatever the normal path computed, so that the
   bypasses survive: a load feeding a store's data operand keeps its one-cycle
   discount (§7sedecies.4b) rather than having it overwritten.  */

int
lvx_lms_load_latency_delta (rtx_insn *insn)
{
  if (lvx_lms_load_latency == 0 || recog_memoized (insn) < 0)
    return 0;
  if (!lvx_load_class_p (get_attr_type (insn)))
    return 0;
  int have = insn_default_latency (insn);
  return (int) lvx_lms_load_latency > have ? (int) lvx_lms_load_latency - have : 0;
}

/* Declared here, defined with the rest of the promotion machinery: a load CMS
   is going to rewrite to its dismissible form.  §7tricies.224.  */

static bool lvx_lms_promotable_load_p (rtx_insn *, bitmap);

/* WHY INSN cannot be speculated, as one word for the probe, and
   "speculatable" when it can be.  §7tricies.223.  The tests are in the SAME
   ORDER as lvx_speculatable_p above and test the same things, so the two
   cannot disagree about which property is the obstacle -- a probe that named
   a different reason than the predicate acted on would be worse than none.
   Keep them together if either changes.  */

static const char *
lvx_lms_unspec_reason (rtx_insn *insn, bitmap bodydef)
{
  if (!NONDEBUG_INSN_P (insn))
    return "not-an-insn";
  /* §7tricies.224: a load CMS will promote is speculatable, and saying
     otherwise is what made crc32 pin 16 loads against consumers that the
     emitted code issues as `ld.s'.  */
  if (bodydef != NULL && lvx_lms_promotable_load_p (insn, bodydef))
    return "speculatable";
  if (CALL_P (insn))
    return "call";
  if (JUMP_P (insn))
    return "jump";
  if (volatile_insn_p (PATTERN (insn))
      || asm_noperands (PATTERN (insn)) >= 0)
    return "volatile-or-asm";
  if (lvx_mem_write_insn_p (insn))
    return "store";
  if (lvx_mem_read_insn_p (insn))
    {
      enum lvx_variant v = lvx_insn_variant (insn);

      if (v != LVX_VARIANT_SPECULATE && v != LVX_VARIANT_PRELOAD)
	return "load-not-dismissible";
    }
  if (flag_trapping_math && lvx_fp_insn_p (insn))
    return "trapping-fp";
  return "speculatable";
}

/* The upper bound on software-pipeline overlap that inductive relaxation is
   allowed to assume, the paper's `o'.  It bounds how far a displacement may be
   adjusted, so it must not exceed what codegen can actually produce.  */
#define LVX_LMS_MAX_OVERLAP 8

/* Signed 10 bits: the displacement an LVX base+displacement operand holds
   without an IMMX extension syllable (format signed10, widened to signed37 by
   .X and to signed64 by .Y -- see lvx_r_s10_s37_s64_operand).  Relaxation must
   not push a displacement out of the field it already occupies, or it changes
   the instruction's encoding and its cost.  */

static inline bool
lvx_lms_disp_fits_p (HOST_WIDE_INT d)
{
  return d >= -512 && d < 512;
}

/* A SIMPLE INDUCTION, after Dupont de Dinechin, "A Unified Software Pipeline
   Construction Scheme for Modulo Scheduled Loops", PaCT 1997, section 4: a
   register updated with only ONE addition or subtraction in the loop body,
   whose step fits in an immediate constant.

   Why they are worth recovering as a kind of their own.  A use of a simple
   induction as the BASE of a base+displacement operand does not need that
   iteration's value: successive values differ by a multiple of the step, so the
   displacement can absorb the difference and the operand can read whichever
   instance was most recently computed.  Every arc from the induction's
   definition to such a use may therefore be dropped from the scheduling graph.
   That is INDUCTIVE RELAXATION, and it is what replaces modulo expansion for
   the induction register itself -- the register pressure win being the reason
   the technique exists.

   The paper's own caution is worth carrying, because it predicts the result of
   measuring this: against generalized modulo expansion, removing the arcs made
   no significant difference to schedule quality, "although generalized modulo
   expansion keeps flow dependences in the scheduling problem, these cannot
   close a recurrence cycle in the case of a simple induction".  So the expected
   effect is a smaller and less constrained graph, NOT a lower RecMII.  An
   induction's own self-recurrence contributes a circuit of theta = the add's
   latency over omega = 1, which is 1, and its def-to-use arcs run forward
   within the iteration, so neither can bind.

   Fills DELTA[r] with the step of each simple induction and sets its bit in
   INDUCTIONS.  A zero step is not an induction.  */

static void
lvx_lms_find_simple_inductions (const auto_vec<rtx_insn *> &seq,
				auto_vec<HOST_WIDE_INT> &delta,
				bitmap inductions)
{
  unsigned n = seq.length ();

  delta.safe_grow_cleared (FIRST_PSEUDO_REGISTER);

  /* Count the definitions of every register in the body, and remember the one
     insn that defines each.  More than one disqualifies it outright: the paper
     asks for ONE addition or subtraction, and two updates make the step depend
     on where in the body a use sits.  */
  auto_vec<int> ndefs, where;
  ndefs.safe_grow_cleared (FIRST_PSEUDO_REGISTER);
  where.safe_grow_cleared (FIRST_PSEUDO_REGISTER);
  for (unsigned i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    where[i] = -1;

  for (unsigned i = 0; i < n; i++)
    {
      df_ref def;

      if (!NONDEBUG_INSN_P (seq[i]))
	continue;
      FOR_EACH_INSN_DEF (def, seq[i])
	{
	  unsigned r = DF_REF_REGNO (def);
	  if (r >= FIRST_PSEUDO_REGISTER)
	    continue;
	  ndefs[r]++;
	  where[r] = (int) i;
	}
    }

  for (unsigned r = 0; r < FIRST_PSEUDO_REGISTER; r++)
    {
      if (ndefs[r] != 1 || where[r] < 0)
	continue;

      rtx pat = PATTERN (seq[where[r]]);
      if (GET_CODE (pat) != SET
	  || !REG_P (SET_DEST (pat))
	  || REGNO (SET_DEST (pat)) != r
	  || GET_CODE (SET_SRC (pat)) != PLUS
	  || !REG_P (XEXP (SET_SRC (pat), 0))
	  || REGNO (XEXP (SET_SRC (pat), 0)) != r
	  || !CONST_INT_P (XEXP (SET_SRC (pat), 1)))
	continue;

      /* A subtraction reaches here as an addition of a negative constant,
	 which is how the RTL spells it.  */
      HOST_WIDE_INT d = INTVAL (XEXP (SET_SRC (pat), 1));
      if (d == 0 || !lvx_lms_disp_fits_p (d))
	continue;

      delta[r] = d;
      bitmap_set_bit (inductions, r);
    }
}

/* True if INSN uses REGNO as the base of a base+displacement memory operand
   whose displacement can still absorb an adjustment of up to
   LVX_LMS_MAX_OVERLAP steps of DELTA -- the paper's `displacement +- o.delta
   still fits in the operand'.  Such a use is RELAXED: it may read any instance
   of the induction, so it needs no dependence on the one definition.  */

static bool
lvx_lms_relaxed_base_use_p (rtx_insn *insn, unsigned regno,
			    HOST_WIDE_INT delta)
{
  subrtx_iterator::array_type array;
  rtx mem = NULL_RTX;

  if (!NONDEBUG_INSN_P (insn))
    return false;

  FOR_EACH_SUBRTX (iter, array, PATTERN (insn), NONCONST)
    if (MEM_P (*iter))
      {
	if (mem != NULL_RTX)
	  return false;
	mem = const_cast<rtx> (*iter);
      }
  if (mem == NULL_RTX)
    return false;

  HOST_WIDE_INT disp;
  rtx base = lvx_split_plus_const (XEXP (mem, 0), &disp);
  if (!REG_P (base) || REGNO (base) != regno)
    return false;

  /* The register must be used ONLY as that base.  A second use of it in the
     same instruction -- as the stored value, say -- needs the real value and
     cannot be relaxed.  */
  df_ref use;
  int uses = 0;
  FOR_EACH_INSN_USE (use, insn)
    if (DF_REF_REGNO (use) == regno)
      uses++;
  if (uses != 1)
    return false;

  HOST_WIDE_INT reach = LVX_LMS_MAX_OVERLAP * (delta > 0 ? delta : -delta);
  return (lvx_lms_disp_fits_p (disp + reach)
	  && lvx_lms_disp_fits_p (disp - reach));
}

/* Collect a description of every memory reference in SEQ, LOOP's body in chain
   order, as an affine function of the iteration number.

   NOT via iv_analyze, which cannot serve here: loop-iv.cc's simple_reg_p
   refuses hard registers outright --

       r = REGNO (reg);
       if (HARD_REGISTER_NUM_P (r))
         return false;

   -- and machine_reorg runs after reload, where every register is hard.  So
   every reference came back unanalysed, systematically, for a reason no amount
   of plumbing fixes.  It also wanted LOOPS_HAVE_PREHEADERS, because it goes
   through df_analyze_loop; doing the recognition here needs no preheader, no
   DF rebuild and no df_analyze per loop, so the probe stays read-only.

   What is recognised is the one form that matters post-reload: an address
   register whose every definition in the body is `R = R + constant'.  The step
   is then the total of those constants, and a reference's own offset is its MEM
   displacement plus the increments that PRECEDE it in chain order -- which is
   what makes a load before the pointer bump and a store after it come out at
   the same address, as they must for `C[i][j] *= beta'.

   Anything else -- a register written by something other than such an
   increment, a reference whose address is not a register plus a constant, an
   unknown access size, an instruction holding two references -- leaves the
   description invalid, and an invalid description never disproves a
   dependence.  */

static void
lvx_lms_collect_mem_ivs (class loop *, const auto_vec<rtx_insn *> &seq,
			 auto_vec<lvx_mem_iv> &ivs)
{
  unsigned n = seq.length ();

  ivs.safe_grow_cleared (n);
  if (!lvx_lms_stride_test)
    return;

  /* Pass 1: each reference's MEM, its displacement off a base register, and
     its size.  SYM holds the base register for now; STEP is filled in below.  */
  auto_vec<int> base;
  base.safe_grow_cleared (n);
  for (unsigned i = 0; i < n; i++)
    {
      subrtx_iterator::array_type array;
      rtx mem = NULL_RTX;
      bool twice = false;

      base[i] = -1;
      if (!NONDEBUG_INSN_P (seq[i]))
	continue;

      FOR_EACH_SUBRTX (iter, array, PATTERN (seq[i]), NONCONST)
	if (MEM_P (*iter))
	  {
	    if (mem != NULL_RTX)
	      twice = true;
	    mem = const_cast<rtx> (*iter);
	  }
      if (mem == NULL_RTX || twice || !MEM_SIZE_KNOWN_P (mem))
	continue;

      HOST_WIDE_INT size;
      if (!MEM_SIZE (mem).is_constant (&size) || size <= 0)
	continue;

      HOST_WIDE_INT off;
      rtx addr = lvx_split_plus_const (XEXP (mem, 0), &off);
      if (!REG_P (addr))
	continue;

      base[i] = (int) REGNO (addr);
      ivs[i].off = off;
      ivs[i].size = size;
    }

  /* Pass 2: for every base register used, check that its definitions in the
     body are all constant increments, and total them.  */
  for (unsigned i = 0; i < n; i++)
    {
      if (base[i] < 0)
	continue;

      unsigned regno = (unsigned) base[i];
      HOST_WIDE_INT step = 0;
      HOST_WIDE_INT before = 0;
      bool ok = true;

      for (unsigned j = 0; j < n && ok; j++)
	{
	  if (!NONDEBUG_INSN_P (seq[j]) || !lvx_insn_defs_regno_p (seq[j], regno))
	    continue;

	  rtx pat = PATTERN (seq[j]);
	  HOST_WIDE_INT c;

	  if (GET_CODE (pat) != SET
	      || !REG_P (SET_DEST (pat))
	      || REGNO (SET_DEST (pat)) != regno
	      || GET_CODE (SET_SRC (pat)) != PLUS
	      || !REG_P (XEXP (SET_SRC (pat), 0))
	      || REGNO (XEXP (SET_SRC (pat), 0)) != regno
	      || !CONST_INT_P (XEXP (SET_SRC (pat), 1)))
	    {
	      ok = false;
	      break;
	    }
	  c = INTVAL (XEXP (SET_SRC (pat), 1));
	  step += c;
	  if (j < i)
	    before += c;
	}

      if (!ok)
	continue;

      ivs[i].sym = regno_reg_rtx[regno];
      ivs[i].off += before;
      ivs[i].step = step;
      ivs[i].valid = true;
    }
}

/* True if reference A in some iteration and reference B in the iteration D
   later provably do not overlap.

   With both addresses affine in the iteration number and sharing a symbolic
   base, their separation is a constant:

       b (k + d) - a (k)  =  (off_b - off_a) + step * d

   and two byte ranges overlap exactly when that separation lies strictly
   between -size_b and +size_a.  Outside it, there is no dependence at distance
   D however the two instructions are ordered.

   This is what disproves the arcs a type-based alias oracle cannot: for
   `C[i][j] *= beta' the load and the store touch one address per iteration and
   advance by eight bytes, so the store of iteration k and the load of
   iteration k + 1 are eight bytes apart and cannot conflict, while the pair
   within one iteration is zero apart and does.  may_alias_p sees only that
   both references are to C and says yes to both.

   Requiring the SAME step is what keeps this honest: two references advancing
   at different rates meet periodically, and deciding when needs a diophantine
   argument this does not make.  */

static bool
lvx_lms_mem_independent_p (const struct lvx_mem_iv &a,
			   const struct lvx_mem_iv &b, HOST_WIDE_INT d)
{
  if (!a.valid || !b.valid)
    return false;
  if (a.step != b.step)
    return false;
  if (a.sym == NULL_RTX || b.sym == NULL_RTX
      || !rtx_equal_p (a.sym, b.sym))
    return false;

  /* Guard the multiply: a distance times a stride can overflow for an absurd
     stride, and a wrong answer here is a miscompile rather than a lost
     optimisation.  */
  if (d != 0 && a.step != 0
      && (d > HOST_WIDE_INT_MAX / (a.step > 0 ? a.step : -a.step)))
    return false;

  HOST_WIDE_INT delta = (b.off - a.off) + a.step * d;
  return !(delta > -b.size && delta < a.size);
}

static inline void

lvx_lms_push_arc (auto_vec<lvx_lms_arc> &arcs, unsigned src, unsigned dst,
		  HOST_WIDE_INT theta, HOST_WIDE_INT omega,
		  enum lvx_lms_arc_kind kind)
{
  lvx_lms_arc a = { src, dst, theta, omega, kind, -1 };
  arcs.safe_push (a);
}

/* The position of LOC in a sub-rtx walk of PAT, counting only REGs, or -1 if
   it is not there.  The same walk at rewrite time finds the same position in
   a copy of PAT, which is what makes the index portable where the pointer is
   not.  */

static int
lvx_lms_reg_index (rtx pat, rtx *loc)
{
  subrtx_ptr_iterator::array_type array;
  int i = 0;

  if (!loc)
    return -1;
  FOR_EACH_SUBRTX_PTR (it, array, &pat, NONCONST)
    {
      rtx *here = *it;
      if (!here || !*here || !REG_P (*here))
	continue;
      if (here == loc)
	return i;
      i++;
    }
  return -1;
}

/* The REG at index IDX of that walk, or null.  */

static rtx *
lvx_lms_reg_at (rtx *pat, int idx)
{
  subrtx_ptr_iterator::array_type array;
  int i = 0;

  if (idx < 0)
    return NULL;
  FOR_EACH_SUBRTX_PTR (it, array, pat, NONCONST)
    {
      rtx *here = *it;
      if (!here || !*here || !REG_P (*here))
	continue;
      if (i == idx)
	return here;
      i++;
    }
  return NULL;
}

/* True if INSN loads through an address that varies across iterations, i.e. one
   whose base is written somewhere in BODYDEF.  Such a load is the one a modulo
   schedule actually speculates: it gets hoisted into an earlier stage, above
   the exit test of the iteration it belongs to.  A loop-INVARIANT load would
   have been hoisted out of the loop entirely instead.

   This matters because the dependence graph's branch constraint (1) already
   lets a load cross a conditional branch -- "a load may cross, which is what
   the dismissible variant is for".  That is not an optimisation the codegen may
   decline: a plain `ld' can fault, so the schedule is only legal if every such
   load is emitted as `.s' or `.us'.  Counting them is how a measurement stays
   honest about what it is assuming.  */

static bool
lvx_lms_speculated_load_p (rtx_insn *insn, bitmap bodydef)
{
  subrtx_iterator::array_type array;
  rtx pat = PATTERN (insn);

  if (!lvx_mem_read_insn_p (insn) || lvx_mem_write_insn_p (insn))
    return false;

  enum lvx_variant v = lvx_insn_variant (insn);
  if (v == LVX_VARIANT_SPECULATE || v == LVX_VARIANT_PRELOAD)
    /* Already dismissible, so nothing to promote.  */
    return false;

  FOR_EACH_SUBRTX (iter, array, pat, NONCONST)
    {
      const_rtx x = *iter;
      if (!MEM_P (x))
	continue;
      subrtx_iterator::array_type inner;
      rtx addr = XEXP (x, 0);
      FOR_EACH_SUBRTX (ait, inner, addr, NONCONST)
	if (REG_P (*ait) && bitmap_bit_p (bodydef, REGNO (*ait)))
	  return true;
    }
  return false;
}

/* True if INSN is a load CMS IS GOING TO REWRITE to `.s'/`.us'.
   §7tricies.224.

   This exists because of an ORDERING, not a capability: the taint fixpoint
   runs in lvx_lms_build_graph, the promotion below runs after the II search,
   so every promotable load looked NON-dismissible to the analysis and tainted
   whatever fed its address.  Measured in crc32 loop 1: 16 loads pinned and
   16 loads forced to `.s' -- the same sixteen, and the emitted kernel issues
   those consumers as `ld.s'.  The pins were against a state the code does not
   have.

   THE CONDITIONS MIRROR THE PROMOTER EXACTLY and must keep doing so: it is a
   candidate by lvx_lms_speculated_load_p, and every MEM in it is in the
   generic address space, because that is the only one set_mem_addr_space is
   applied to below.  If they ever drift apart the taint under-approximates
   and the divmodvx class of wrong code comes back, so the promoter asserts
   that nothing promotable survives it.  */

static bool
lvx_lms_promotable_load_p (rtx_insn *insn, bitmap bodydef)
{
  subrtx_iterator::array_type array;

  if (!lvx_lms_force_spec_loads || bodydef == NULL)
    return false;
  if (!lvx_lms_speculated_load_p (insn, bodydef))
    return false;
  FOR_EACH_SUBRTX (iter, array, PATTERN (insn), NONCONST)
    if (MEM_P (*iter) && MEM_ADDR_SPACE (*iter) != ADDR_SPACE_GENERIC)
      return false;
  return true;
}

/* True if INSN may be speculated once CMS has promoted what it promotes.  The
   taint fixpoint asks THIS, not lvx_speculatable_p, because it runs before the
   promotion.  §7tricies.224.  */

static bool
lvx_lms_speculatable_after_promotion_p (rtx_insn *insn, bitmap bodydef)
{
  return (lvx_speculatable_p (insn)
	  || lvx_lms_promotable_load_p (insn, bodydef));
}

/* Promote the loop-variant-address loads of SEQ to their dismissible variant.

   This is not an optimisation: the dependence graph's branch constraint (1)
   already lets a load cross a conditional branch, so a schedule built on that
   graph is legal ONLY if every such load cannot fault.  `.s' keeps the cached
   latency -- it is Dismissible, not uncached -- so promoting changes the
   emitted mnemonic and nothing the scheduler reasoned about.  Returns the
   count.  */

static int
lvx_lms_force_speculative_loads (const auto_vec<rtx_insn *> &seq,
				 bitmap bodydef)
{
  int promoted = 0;

  for (unsigned i = 0; i < seq.length (); i++)
    {
      if (!lvx_lms_speculated_load_p (seq[i], bodydef))
	continue;

      subrtx_ptr_iterator::array_type array;
      rtx pat = PATTERN (seq[i]);
      bool changed = false;
      FOR_EACH_SUBRTX_PTR (iter, array, &pat, NONCONST)
	if (MEM_P (**iter)
	    && MEM_ADDR_SPACE (**iter) == ADDR_SPACE_GENERIC)
	  {
	    set_mem_addr_space (**iter, LVX_ADDR_SPACE_SPECULATE);
	    changed = true;
	  }
      if (changed)
	{
	  df_insn_rescan (seq[i]);
	  promoted++;
	}
    }

  /* §7tricies.224: THE TIE.  lvx_lms_build_graph's taint fixpoint treated
     every promotable load as speculatable on the strength of this function
     promoting it.  If one survives, that assumption was false for it and the
     graph is missing a constraint -- which is the divmodvx bug.  Checked
     rather than commented.  */
  if (flag_checking)
    for (unsigned i = 0; i < seq.length (); i++)
      gcc_assert (!lvx_lms_promotable_load_p (seq[i], bodydef));

  return promoted;
}

/* The furthest iteration distance the scheduler keeps copies for.  Beyond it an
   arc is slack by arithmetic; see lvx_lms_delta.  */
#define LVX_LMS_MAX_DELTA 8

/* Delta: the largest iteration distance any arc spans that can bind.  This is what replaces
   kvx-unfold.cc's `max_distance = 1; // FIXME' -- that literal was the two-copy
   structure asserting nothing spans more than one instance, which is an
   artefact of having two physical copies rather than anything CMS requires.
   LVX's graph carries omega explicitly, so Delta is simply measured.  */

static int
lvx_lms_delta (const auto_vec<lvx_lms_arc> &arcs)
{
  int delta = 1;
  for (unsigned i = 0; i < arcs.length (); i++)
    {
      HOST_WIDE_INT w = arcs[i].omega < 0 ? -arcs[i].omega : arcs[i].omega;
      if (w > LVX_LMS_MAX_DELTA)
	/* An arc reaching farther than the window cannot bind -- its
	   requirement theta - omega * II is already hugely negative -- so it
	   must not inflate the window itself.  This is the rule that a
	   dependence farther away than the iterations in flight may be
	   dropped, applied at the window rather than at a stage count the
	   scheduler does not know until it has finished.  A safelen of INT_MAX
	   from #pragma GCC ivdep arrives here.  */
	continue;
      if (delta < (int) w)
	delta = (int) w;
    }
  return delta;
}

/* True if INSN can join the bundle already accumulated in SLOT.  Checked
   against the counted resource model -- the availabilities Resource.yml emits
   as lvx_issue_availability -- rather than against the scheduling automaton,
   so this needs no dfa_start ().  That is the whole advantage recorded in
   docs/cms-foundations.md §7.5: the DFA fit kvx-unfold.cc uses is portable to a
   target with no counted resources, but where the counters exist they give the
   exact subset bound instead of an order-dependent greedy fit.  */

/* Accumulate INSN's demand on a bundle for CMS, which is the machine
   description's answer for everything EXCEPT a doloop_end.

   A doloop_end carries no `issue' attribute, so it takes the default, which is
   `all' -- the saturating vector that claims every function unit and so an
   entire exclusive bundle.  As a description of the code that is emitted that
   is badly wrong in one direction: the instruction prints `# loopdo end', a
   comment, and the loop engine does the decrement and the branch for free.  As
   a description of what CMS would emit it is wrong in the other, because a
   modulo schedule cannot keep the hardware loop as it stands: the trip count
   changes by the stages in flight, and a prolog and epilog need somewhere to
   branch, which an implicit loopback has not got.

   So the honest charge is neither `all' nor zero but the decrement-test-branch
   the hardware loop stands for: one ALU_TINY for `addd $r = $r, -1' and one
   BCU reading it for `cb.dnez $r? .L'.  That is what the RTL already says --
   the pattern is a PARALLEL of exactly that branch and that decrement, which
   is why the register dependences were right all along and only the resource
   charge was not.

   It is conservative by one branch in K, and deliberately.  With the unroll
   factor a power of two the k-unrolled kernel's own loopback converts back to
   a hardware loop cheaply, the trip-count division being a shift, while the
   other K - 1 exits inverted become early exits from the kernel and stay real
   branches.  So one iteration in K pays nothing and K - 1 pay in full, and
   charging every iteration in full overstates by 1/K -- which is the right
   direction for a bound, and vanishes as K grows.  */

/* THE PREFIX A GUARDED OR MASKED OP CARRIES, CHARGED ONE PER OP.
   §7tricies.194.

   WHAT WAS MISSING.  lvx_sched_dfa_new_cycle accounts for GUARD, MASKS and
   MASKM exactly -- get_attr_bcu_used says whether an op needs a BCU slot, a
   COND_EXEC's guard condition is the key and a mask register with its lanetodo
   is the key for the masked form, and lvx_bcu_prefix_shared_p makes a prefix
   already in the bundle cost nothing more, so every op under ONE GUARD spends
   one BCU slot between them.  None of that reaches CMS, whose
   lvx_sched_resources_add charges only get_attr_issue, the bare insn.  So CMS
   could build a cycle the bundler then had to split -- invisible while SCHED2
   owns the bundles, and an assembler error the moment a CMS cycle IS one:
   mad's libz inftrees.c wanted 3 BRRP of 2 from a guarded store plus a ccb.

   DELIBERATELY CONSERVATIVE: one BCU_BRRP per prefixed op, with no sharing.
   That OVER-charges where the bundler would have shared -- two ops under the
   same guard pay twice here and once there -- which costs II and never
   legality.  Under-charging is the direction that cannot be allowed: it lets
   CMS emit a cycle that does not fit a bundle, and splitting a parallel group
   is the irrecoverable error (§7tricies.191).  Modelling the sharing properly
   means keying on the guard condition across a whole cycle, which is the
   bundler's bookkeeping and not a resource count.

   Charged only when CMS owns the bundles.  Otherwise SCHED2 still decides
   them and enforces the real rule, so the charge would buy nothing and cost
   II on every pipelined loop.  */
static void
lvx_lms_resources_add_prefix (struct lvx_sched_resources *resources,
			      rtx_insn *insn)
{
  enum lvx_bcu_kind kind;
  rtx key;
  int lanetodo;

  if (!lvx_lms_own_bundles)
    return;
  if (!NONDEBUG_INSN_P (insn) || INSN_CODE (insn) < 0)
    return;
  if (get_attr_bcu_used (insn) != BCU_USED_YES)
    return;
  /* A plain BCU op -- branch, jump, sysget -- is the BCU op itself and its own
     reservation already says so; only a PREFIX is uncounted.  */
  if (GET_CODE (PATTERN (insn)) != COND_EXEC
      && !lvx_masked_prefix (insn, &kind, &key, &lanetodo))
    return;

  {
    const struct lvx_issue_resources *br = &lvx_issue_table[ISSUE_BCU_BRRP];

    resources->ninsns_count += br->ninsns;
#define LVX_LMS_PREFIX_ADD(R) resources->R##_count += br->R;
    LVX_ISSUE_RESOURCES (LVX_LMS_PREFIX_ADD)
#undef LVX_LMS_PREFIX_ADD
  }
}

void
lvx_lms_resources_add (struct lvx_sched_resources *resources, rtx_insn *insn)
{
  if (!lvx_doloop_end_p (insn))
    {
      lvx_sched_resources_add (resources, insn);
      lvx_lms_resources_add_prefix (resources, insn);
      return;
    }

  /* MEASUREMENT ONLY.  §7tricies.88.  A mapped doloop_end emits `# loopdo end'
     and occupies nothing, so charging it is an overcharge for exactly the loops
     that become hardware loops -- 19 of TSVC2's, 13 of them doubling ResMII,
     and 24 converged schedules had a placement pushed to a later cycle by this
     charge alone.  None of that says whether the charge costs II, span or
     depth, which is what this lever is for: charge nothing, and diff the
     schedules.  It is not sound as a default, because reorg_loops can still
     refuse the loop (LOOPDO's reach, ITER_REG_USED) and then the decrement and
     the branch are real.  */
  if (lvx_lms_free_doloop_end)
    return;

  const struct lvx_issue_resources *dec = &lvx_issue_table[ISSUE_TINY];
  const struct lvx_issue_resources *br = &lvx_issue_table[ISSUE_BCU_BRRP];

  resources->insn_count++;
  resources->ninsns_count += dec->ninsns + br->ninsns;
#define LVX_LMS_DOLOOP_ADD(R) resources->R##_count += dec->R + br->R;
  LVX_ISSUE_RESOURCES (LVX_LMS_DOLOOP_ADD)
#undef LVX_LMS_DOLOOP_ADD
}

/* The issue counters by name and index, so a blame can say WHICH resource
   refused a task.  The index past the last counter stands for the issue rate,
   which is a bound on the bundle rather than a counter in Resource.yml.  */

static const char *const lvx_lms_resource_name[] = {
#define LVX_LMS_RESOURCE_NAME(R) #R,
  LVX_ISSUE_RESOURCES (LVX_LMS_RESOURCE_NAME)
#undef LVX_LMS_RESOURCE_NAME
  "issue"
};

#define LVX_LMS_NRESOURCES   ((int) (sizeof (lvx_lms_resource_name) / sizeof (lvx_lms_resource_name[0])))

/* Which counter refuses INSN in SLOT, as an index into
   lvx_lms_resource_name, or -1 if it fits.  The first that overflows is
   reported: a task displaced from a cycle is displaced by one thing, and
   attributing it to the first is enough to aim the next construction.  */

static int
lvx_lms_blocking_resource (const struct lvx_sched_resources *slot,
			   rtx_insn *insn)
{
  struct lvx_sched_resources trial = *slot;
  int idx = 0;

  lvx_lms_resources_add (&trial, insn);

#define LVX_LMS_BLOCK(R)						\
  {									\
    if (trial.R##_count > (unsigned) lvx_issue_availability.R)		\
      return idx;							\
    idx++;								\
  }
  LVX_ISSUE_RESOURCES (LVX_LMS_BLOCK)
#undef LVX_LMS_BLOCK

  if (trial.ninsns_count > (unsigned) lvx_sched_issue_rate ())
    return idx;
  return -1;
}

/* THE ROLLING MRT.  §7tricies.183 (author).

   An II-row residue table carrying exactly ONE instance per instruction -- the
   latest placed.  When instance i of X is scheduled, X's instance i-1 is
   subtracted from the row it occupied and instance i added at its new row.  So
   the table always holds the n latest instances folded by residue, which is
   exactly the candidate SIGMA the oracle would verify: the residues of a
   periodic schedule are instance-invariant, instance q of task i sitting at
   sigma_i + (q-1) II, so the latest instance's residue IS sigma_i mod II.

   Transient overflow is expected during ramp-up and is not an error.  When no
   counter is violated, the latest-instance set is resource-feasible at this
   II -- the resource half of lvx_lms_verify, maintained incrementally instead
   of rebuilt per call.

   READ-ONLY HERE.  This probe answers two questions before any policy reads
   it: does the violation count FALL as a schedule converges, so that it is a
   usable gradient, and does it reach zero EARLIER than the copy-completion
   checkpoint the oracle fires on.  The intended use -- deprioritising a
   candidate whose placement would overflow a residue, a hybrid with classic
   modulo scheduling -- is only worth building if both answers are yes.

   The subtraction must use the row the previous instance ACTUALLY occupied,
   not one derived from the new date: the two coincide only once the schedule
   is stationary for that task, which is the property being converged to.  */

void
lvx_lms_resources_sub (struct lvx_sched_resources *resources, rtx_insn *insn)
{
  struct lvx_sched_resources zero, one;

  memset (&zero, 0, sizeof (zero));
  one = zero;
  lvx_lms_resources_add (&one, insn);
  resources->insn_count -= one.insn_count;
  resources->ninsns_count -= one.ninsns_count;
#define LVX_LMS_SUB(R) resources->R##_count -= one.R##_count;
  LVX_ISSUE_RESOURCES (LVX_LMS_SUB)
#undef LVX_LMS_SUB
}

/* How many counters are over their availability across the whole table: the
   distance to resource feasibility, zero meaning feasible.  */

unsigned
lvx_lms_mrt_violations (const auto_vec<struct lvx_sched_resources> &mrt)
{
  unsigned v = 0;

  for (unsigned r = 0; r < mrt.length (); r++)
    {
      const struct lvx_sched_resources *slot = &mrt[r];

#define LVX_LMS_MRTV(R)							      if (slot->R##_count > (unsigned) lvx_issue_availability.R)			v += slot->R##_count - (unsigned) lvx_issue_availability.R;
      LVX_ISSUE_RESOURCES (LVX_LMS_MRTV)
#undef LVX_LMS_MRTV
      if (slot->ninsns_count > (unsigned) lvx_sched_issue_rate ())
	v += slot->ninsns_count - (unsigned) lvx_sched_issue_rate ();
    }
  return v;
}

/* The violation count the rolling MRT WOULD have if task I's latest instance
   moved to ROW.  Mutate and undo: exact, and cheaper than a copy of the whole
   table.  §7tricies.184.  */

unsigned
lvx_lms_mrt_trial (auto_vec<struct lvx_sched_resources> &mrt,
		   const auto_vec<int> &mrow, rtx_insn *insn, unsigned i,
		   int row)
{
  int old = mrow[i];
  unsigned v;

  if (old >= 0)
    lvx_lms_resources_sub (&mrt[(unsigned) old], insn);
  lvx_lms_resources_add (&mrt[(unsigned) row], insn);
  v = lvx_lms_mrt_violations (mrt);
  lvx_lms_resources_sub (&mrt[(unsigned) row], insn);
  if (old >= 0)
    lvx_lms_resources_add (&mrt[(unsigned) old], insn);
  return v;
}

/* Whether INSN reserves anything of the counter at index WANT.  Used to find
   the tasks holding the resource that displaced another -- the culprits a
   release date should move out of the way.  */

static bool
lvx_lms_insn_uses_resource_p (rtx_insn *insn, int want)
{
  struct lvx_sched_resources d;
  int idx = 0;

  memset (&d, 0, sizeof (d));
  lvx_lms_resources_add (&d, insn);

#define LVX_LMS_USES(R)							\
  {									\
    if (idx == want)							\
      return d.R##_count > 0;						\
    idx++;								\
  }
  LVX_ISSUE_RESOURCES (LVX_LMS_USES)
#undef LVX_LMS_USES

  return idx == want && d.ninsns_count > 0;
}

/* INSN's processing time in cycles: how long after its issue date its result
   is ready, which is what a makespan runs to.  Floored at one cycle because an
   instruction occupies its issue cycle whatever the DFA says, and some say
   zero -- a doloop_end among them, whose `all'/alu attribute pair matches no
   reservation at all.  A mapped-back decrement-test-branch is a branch, and a
   branch feeds no in-iteration consumer, so one cycle is its whole cost.  */

/* Interference graph over the UNROLLED modulo kernel, coloured by Smith's
   squeeze.  See docs/cms-foundations.md §7ter.9 and §7ter.10.

   Why unroll first rather than test interference cyclically.  kvx-unfold.cc
   replicates each range p2unroll - 1 times and then asks
   unfold_range_overlap_p, which shifts one range's dates by multiples of II in
   both directions until it finds an overlap or runs past.  That is correct but
   delicate -- and its two `first_def_p' terms are commented out, so the
   earlyclobber adjustment of §7ter.2a is not applied.  Materialising KUNROLL
   instances instead turns the question back into a plain overlap, and the
   power-of-two closure of §7quater.5 is what makes it exact: p2unroll divides
   KUNROLL, so name J of a value occupies exactly KUNROLL / p2unroll intervals,
   evenly spaced p2unroll * II apart, and a lifetime never exceeds p2unroll * II
   so those intervals cannot overlap each other.

   A node is therefore (value, name) with an occupancy bitmap over the unrolled
   period P = KUNROLL * II, and two nodes interfere iff their bitmaps meet.  */

#define LVX_LMS_IG_MAX_NODES 512
#define LVX_LMS_IG_MAX_PERIOD 1024

/* Register classes, by how many consecutive registers a value occupies.  */
enum lvx_lms_rclass { LVX_RC_SINGLE = 0, LVX_RC_PAIR, LVX_RC_QUAD, LVX_RC_N };

static const int lvx_lms_rclass_size[LVX_RC_N] = { 64, 32, 16 };

/* worst_1 (N, C): the most names of class N that one neighbour of class C can
   deny.  Nine numbers, and they are this simple only because LVX's tuples are
   ALIGNED, which makes the alias relation a laminar family -- a single belongs
   to exactly one pair and one quad, never to two overlapping ones.  Smith's
   motivating case has worst_1 (double, single) = 2 as well, which needs
   unaligned doubles; here every reverse direction is 1.  And because aligned
   classes partition rather than overlap,

       worst_m (N, C) = min (m * worst_1 (N, C), |N|)

   so no search over subsets is needed.  */
static const int lvx_lms_worst1[LVX_RC_N][LVX_RC_N] = {
  /*        C=SINGLE  PAIR  QUAD */
  /* N=SINGLE */ { 1,   2,   4 },
  /* N=PAIR   */ { 1,   1,   2 },
  /* N=QUAD   */ { 1,   1,   1 },
};

static enum lvx_lms_rclass
lvx_lms_rclass_of (int nregs)
{
  if (nregs >= 4)
    return LVX_RC_QUAD;
  if (nregs >= 2)
    return LVX_RC_PAIR;
  return LVX_RC_SINGLE;
}

/* Smith's squeeze for node N given how many live neighbours it has of each
   class, with the saturation bound: several classes together cannot deny more
   than their alias sets hold, which on a laminar family is |N| itself.  */
static int
lvx_lms_squeeze (enum lvx_lms_rclass n, const int deg[LVX_RC_N])
{
  int sq = 0;
  for (int c = 0; c < LVX_RC_N; c++)
    {
      int w = deg[c] * lvx_lms_worst1[n][c];
      int cap = lvx_lms_rclass_size[n];
      sq += MIN (w, cap);
    }
  return MIN (sq, lvx_lms_rclass_size[n]);
}

/* STAGE SCHEDULING: shift operations by whole multiples of II to shorten live
   ranges.  Eichenberger's skip factors (MICRO-27 1994, thesis ch. 4);
   §7octodecies.2 and §7octodecies.3.

   Moving a node by a multiple of II leaves SIGMA mod II unchanged, so it keeps
   its slot in the modulo reservation table: resources stay feasible by
   construction and the II cannot change.  Only the STAGE moves, which is what
   lifetimes are measured in.

   Only OUTGOING arcs can break when a node moves LATER, since every incoming
   constraint sigma(i) - sigma(h) >= theta - omega*II only loosens.  So the room
   a node has is

       min over arcs (i -> j) of  floor ((sigma(j) - sigma(i) - theta
					 + omega*II) / II)

   and shifting a definition later by that much shortens its own value's range
   without touching anything else's lower bound.  Applied greedily, which
   Mangione-Smith et al. proved optimal only for values with a single use --
   our graphs have CSE and loop-carried dependences, so this is a heuristic
   (§7octodecies.3).  */

static int
lvx_lms_stage_schedule (const auto_vec<lvx_lms_arc> &arcs, unsigned n,
			auto_vec<int> &sigma, int ii)
{
  int moved = 0;
  for (int round = 0; round < 8; round++)
    {
      bool any = false;
      for (unsigned i = 0; i < n; i++)
	{
	  /* How many whole periods this node may be delayed.  */
	  int room = INT_MAX;
	  for (unsigned a = 0; a < arcs.length (); a++)
	    if (arcs[a].src == i)
	      {
		int lhs = sigma[arcs[a].dst] - sigma[i]
			  - (int) arcs[a].theta + (int) arcs[a].omega * ii;
		int k = lhs / ii;
		if (lhs < 0)
		  k = 0;
		if (k < room)
		  room = k;
	      }
	  if (room == INT_MAX)
	    room = 0;		/* nothing reads it: moving buys nothing */
	  if (room <= 0)
	    continue;

	  /* Delaying I shortens the ranges of the values I defines, and
	     lengthens those of the values it reads -- so take the step only
	     if the sum of the spans it touches falls.  */
	  int before = 0, after = 0;
	  for (unsigned a = 0; a < arcs.length (); a++)
	    {
	      if (arcs[a].kind != LVX_ARC_REG_RAW)
		continue;
	      if (arcs[a].src == i)
		{
		  before += sigma[arcs[a].dst] + (int) arcs[a].omega * ii
			    - sigma[i];
		  after += sigma[arcs[a].dst] + (int) arcs[a].omega * ii
			   - (sigma[i] + ii);
		}
	      else if (arcs[a].dst == i)
		{
		  before += sigma[i] + (int) arcs[a].omega * ii
			    - sigma[arcs[a].src];
		  after += sigma[i] + ii + (int) arcs[a].omega * ii
			   - sigma[arcs[a].src];
		}
	    }
	  if (after >= before)
	    continue;

	  sigma[i] += ii;
	  moved++;
	  any = true;
	}
      if (!any)
	break;
    }
  return moved;
}

/* Would splitting the value defined by SEQ[I], or spilling it, still fit the II
   we are currently trying?  §7novodecies.

   The author's constraint (2026-10-27), and it is what distinguishes this from
   Touzeau, Dehnert and Ruttenberg: **only consider a range split or a
   spill/restore that does not push either bound above the II being scheduled
   and allocated for.**  All three references let the II grow -- Touzeau's
   binary search on DEPTH, Dehnert's reschedule-and-retry, Ruttenberg's doubling
   escalation -- so none of them needs this test.  Under it a repair is free in
   rate or it is not a candidate.

   Note the bound is the CURRENT II, not MinII, and the difference matters:
   a loop scheduled above its lower bound has slack, and a repair may consume
   that slack without costing anything.  Testing against MinII would reject
   repairs that are in fact free.  Equivalently, each of ResMII and RecMII must
   stay at or below the II being tried -- which is weaker than "MinII must not
   change" and is the right question.

   Two ways to break it, both checkable before anything is emitted:

   ResMII -- the added operations must fit in the resource budget the current II
	     already provides.  COST_TINY extra TINY slots raise ResMII only if
	     they push the counted demand past II * availability.

   RecMII -- a copy on the dataflow path from a definition to its consumers
	     lengthens every path through it, so a circuit containing it grows
	     by the copy's latency.  Re-solving the parametric period with the
	     affected arcs lengthened answers it exactly, which is cheap enough
	     to do per candidate: the graph is small and lvx_plc is a label
	     correcting solve, not an enumeration.

   A spill is the same test with two instructions and the load's latency in
   place of a copy's one -- which is why an UNCACHED spill reload (15 cycles,
   §7septies.8) can almost never be MinII-preserving inside a recurrence, and
   why the distinction between a value on a critical circuit and one off it
   decides everything here.  */

static bool
lvx_lms_repair_fits_ii_p (const auto_vec<rtx_insn *> &seq,
			    const auto_vec<lvx_lms_arc> &arcs,
			    const struct lvx_sched_resources *demand,
			    unsigned i, int ii, int extra_lat, int cost_tiny,
			    int cost_lsu = 0, int cost_memw = 0)
{
  unsigned n = seq.length ();

  /* ResMII: would the extra operations need another bundle?  A copy takes a
     TINY; a store or a reload takes a TINY and an LSU, and a store takes the
     MEMW write port too, so the caller passes all three.  */
  struct lvx_sched_resources with = *demand;
  with.tiny_count += cost_tiny;
  with.lsu_count += cost_lsu;
  with.memw_count += cost_memw;
  with.ninsns_count += cost_tiny;
  if ((int) lvx_sched_resources_full_bundles (&with) > ii)
    return false;

  /* RecMII: lengthen every true-dependence arc out of I by the inserted
     latency -- the copy or the reload sits between the definition and its
     readers -- and ask the parametric period again.  */
  lvx_plc plc;
  for (unsigned k = 0; k <= n; k++)
    plc.add_node ();
  for (unsigned k = 0; k < n; k++)
    plc.add_arc (0, k + 1, 0, 0);
  for (unsigned a = 0; a < arcs.length (); a++)
    {
      HOST_WIDE_INT th = arcs[a].theta;
      if (arcs[a].src == i && arcs[a].kind == LVX_ARC_REG_RAW)
	th += extra_lat;
      plc.add_arc (arcs[a].src + 1, arcs[a].dst + 1, th, arcs[a].omega);
    }
  if (plc.solve (1) != LVX_PLC_SOLVED)
    return false;
  return (int) plc.period () <= ii;
}


/* MRIS lineage formation on the OMEGA = 0 SUBGRAPH.  §7octodecies.5.

   Govindarajan et al. (IPDPS 2001, IEEE TC 2003) form lineages -- chains of
   descendants that can all share one register, the register passing from each
   node to its HEIR -- and add a sequencing edge from every other descendant to
   the heir, so the heir really is the last use of its parent's value.  The
   construction is defined on a DAG and the heir is chosen by HEIGHT so that the
   added edges cannot close a cycle.

   A modulo-scheduling graph is not a DAG, which is why the author proposed
   restricting the construction to the omega = 0 arcs: those are the
   intra-iteration dataflow and they ARE acyclic (a zero-distance circuit with
   positive latency is infeasible, so lvx_plc would already have refused the
   loop).  Height is therefore well founded on that subgraph and the heir rule
   keeps its guarantee.

   A value whose consumers are all cross-iteration ends its lineage: it is live
   across the back edge, so it cannot hand its register to a consumer inside the
   iteration.  That is the natural boundary the restriction creates, and it is
   where this departs from the papers.  */

/* Heights over the omega = 0 true-dependence subgraph: 1 for a node with no
   such consumer, else one more than the tallest consumer.  */
static void
lvx_lms_lineage_heights (const auto_vec<lvx_lms_arc> &arcs, unsigned n,
			 auto_vec<int> &ht)
{
  ht.safe_grow_cleared (n);
  for (unsigned i = 0; i < n; i++)
    ht[i] = 0;
  /* Longest path by relaxation; the subgraph is acyclic so n rounds suffice.  */
  for (unsigned round = 0; round < n; round++)
    {
      bool moved = false;
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].omega == 0 && arcs[a].kind == LVX_ARC_REG_RAW)
	  {
	    int want = ht[arcs[a].dst] + 1;
	    if (ht[arcs[a].src] < want)
	      ht[arcs[a].src] = want, moved = true;
	  }
      if (!moved)
	break;
    }
}

/* Form lineages and report what they cost and buy.  LIN[i] is the lineage id
   of the value defined by SEQ[i], or -1 if SEQ[i] defines nothing read.  */
static void
lvx_lms_lineages (const auto_vec<lvx_lms_arc> &arcs, unsigned n,
		  const auto_vec<bool> &defines,
		  auto_vec<int> &lin, int *nlin_out, int *nseq_out)
{
  auto_vec<int> ht;
  lvx_lms_lineage_heights (arcs, n, ht);

  /* HEIR[i] is the consumer that inherits i's register, or -1.  A node can be
     the heir of at most one parent, which is what makes the lineages disjoint
     chains rather than a tree.  */
  auto_vec<int> heir, taken;
  heir.safe_grow_cleared (n);
  taken.safe_grow_cleared (n);
  for (unsigned i = 0; i < n; i++)
    heir[i] = -1, taken[i] = 0;

  int nseq = 0;
  for (unsigned i = 0; i < n; i++)
    {
      if (!defines[i])
	continue;
      int best = -1, bestht = -1, cons = 0;
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].src == i && arcs[a].omega == 0
	    && arcs[a].kind == LVX_ARC_REG_RAW)
	  {
	    unsigned d = arcs[a].dst;
	    cons++;
	    /* Height priority, and only a node nobody has claimed: that is
	       what keeps the augmented graph acyclic and the chains
	       disjoint.  */
	    if (!taken[d] && defines[d] && ht[d] > bestht)
	      bestht = ht[d], best = (int) d;
	  }
      if (best >= 0)
	{
	  heir[i] = best;
	  taken[best] = 1;
	  /* One sequencing edge from each OTHER descendant to the heir.  */
	  if (cons > 1)
	    nseq += cons - 1;
	}
    }

  /* Walk the chains from their heads.  */
  lin.safe_grow_cleared (n);
  for (unsigned i = 0; i < n; i++)
    lin[i] = -1;
  int nlin = 0;
  for (unsigned i = 0; i < n; i++)
    {
      if (!defines[i] || taken[i])
	continue;			/* not a chain head */
      int id = nlin++;
      for (int at = (int) i; at >= 0; at = heir[at])
	{
	  lin[at] = id;
	  if (heir[at] < 0)
	    break;
	}
    }
  *nlin_out = nlin;
  *nseq_out = nseq;
}

/* SIGMA's minimum, the schedule's origin.  */
static int
lvx_lms_sigma_min (const auto_vec<int> &sigma)
{
  int lo = sigma.is_empty () ? 0 : sigma[0];
  for (unsigned i = 1; i < sigma.length (); i++)
    if (sigma[i] < lo)
      lo = sigma[i];
  return lo;
}

/* The cyclic makespan of a schedule: first issue to last issue inclusive.  The
   latency of the last instruction is NOT part of it -- see the long note at the
   main dump site and §7quinvicies.5.  */
int
lvx_lms_sigma_span (const auto_vec<int> &sigma)
{
  if (sigma.is_empty ())
    return 0;
  int lo = sigma[0], hi = sigma[0];
  for (unsigned i = 1; i < sigma.length (); i++)
    {
      if (sigma[i] < lo)
	lo = sigma[i];
      if (sigma[i] > hi)
	hi = sigma[i];
    }
  return hi - lo + 1;
}

/* The values a schedule creates, as the colouring and the split analysis both
   want them: for each instruction defining a register that something reads,
   its definition date, the date of its last reader (in absolute cycles, so a
   cross-iteration read counts OMEGA periods later), the expansion degree, the
   width in registers, and whether the destination is earlyclobber.  */
static void
lvx_lms_collect_values (const auto_vec<rtx_insn *> &seq,
			const auto_vec<lvx_lms_arc> &arcs,
			const auto_vec<int> &sigma, int ii,
			auto_vec<int> &def_, auto_vec<int> &last_,
			auto_vec<int> &deg, auto_vec<int> &nregs_,
			auto_vec<int> &ec_, auto_vec<int> *owner = NULL,
			auto_vec<int> *regno_ = NULL,
			auto_vec<int> *usefirst_ = NULL)
{
  for (unsigned i = 0; i < seq.length (); i++)
    {
      int last = -1;
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].src == i && arcs[a].kind == LVX_ARC_REG_RAW)
	  {
	    int when = sigma[arcs[a].dst] + (int) arcs[a].omega * ii;
	    if (when > last)
	      last = when;
	  }
      if (last < 0)
	continue;
      int life = last - sigma[i];
      if (life <= 0)
	continue;
      def_.safe_push (sigma[i]);
      last_.safe_push (last);
      deg.safe_push ((life + ii - 1) / ii);
      rtx ds = single_set (seq[i]);
      int nr = (ds && REG_P (SET_DEST (ds))) ? REG_NREGS (SET_DEST (ds)) : 1;
      nregs_.safe_push (MAX (nr, 1));
      /* PER INSTRUCTION HERE, AND THAT IS RIGHT.  §7tricies.96.  This model is
	 keyed by instruction, not by live range, so the flag of the
	 instruction in hand is exactly the right one -- do not "unify" it with
	 lvx_lms_lrange_ec_p, which answers a different question about a
	 different object.  */
      ec_.safe_push (lvx_insn_dest_earlyclobber_p (seq[i]) ? 1 : 0);
      if (owner)
	owner->safe_push ((int) i);

      /* THE ORIGINAL REGISTER, which the assignment needs twice over
	 (§7ter.14).  To rewrite an operand codegen must know which register it
	 is replacing, and §7ter.4's USE_FIRST normalisation needs it to pin a
	 loop-carried value to the register the preheader wrote.  -1 where the
	 destination is not a plain REG, which an assignment must then decline
	 rather than guess at.  */
      if (regno_)
	regno_->safe_push (ds && REG_P (SET_DEST (ds))
			   ? (int) REGNO (SET_DEST (ds)) : -1);

      /* USE_FIRST: the value crosses the back edge AND is read earlier in the
	 body than it is written, so within one kernel body the use precedes
	 the def and is holding the PREVIOUS iteration's value (§7ter.4).  The
	 reference finds this by merging the last range at a regno into the
	 first; here it falls out of the graph, since a RAW arc at omega >= 1
	 to a consumer positioned before the producer is exactly that shape.
	 Such a value's name 0 comes from outside the kernel, so it must keep
	 the original register rather than be renamed.  */
      if (usefirst_)
	{
	  int uf = 0;
	  for (unsigned a = 0; a < arcs.length (); a++)
	    if (arcs[a].src == i && arcs[a].kind == LVX_ARC_REG_RAW
		&& arcs[a].omega >= 1 && arcs[a].dst < i)
	      {
		uf = 1;
		break;
	      }
	  usefirst_->safe_push (uf);
	}
    }
}

/* Build the interference graph over the kernel unrolled KUNROLL times and
   colour it with the squeeze, reporting the counts and how many nodes could
   not be removed.  Shared by the II search -- where it is the feasibility test
   §7.8 asks for -- and by the report.  §7ter.10.

   AND, WHEN ASKED, PRODUCE THE ASSIGNMENT (§7ter.13).  Until now this answered
   only `is the kernel colourable', which is what §7.8 needs inside the II
   search; codegen needs a register per name, because modulo expansion IS the
   renaming (§7quindecies.5: "a post-RA pipeliner that cannot rename cannot
   pipeline").  The simplify phase already computes the order; what was missing
   is that the order was not RECORDED, so there was no stack to pop.

   ANODE_VALUE, ANODE_NAME and ANODE_REGNO, when non-null, come back parallel:
   one entry per interference node, naming the value it belongs to, which of
   that value's expansion names it is, and the register it was given -- or -1
   where the node could not be coloured, which is the same set STUCK reports.

   AVAIL is the allocatable register set as a bitmap over r0..r63, so a caller
   that knows what the function has left free can say so.  Defaulting it to all
   ones keeps the feasibility answer exactly as it was and makes MAXREG -- the
   highest register the assignment needed -- the actionable number: if it
   exceeds what is free, the II must rise.  §7ter.10 already records that using
   the architectural class size rather than the allocatable one makes the
   colouring OPTIMISTIC, so a failure is meaningful and a success is not a
   guarantee; that is unchanged here and is why AVAIL exists rather than being
   assumed.  */

static void
lvx_lms_colour (const auto_vec<int> &def_, const auto_vec<int> &last_,
		const auto_vec<int> &deg, const auto_vec<int> &nregs_,
		const auto_vec<int> &ec_, int ii, int lo0,
		int *nn_out, int *ne_out, int *msq_out, int *spl_out,
		int *per_out, int *kun_out, auto_vec<int> *stuck = NULL,
		auto_vec<int> *anode_value = NULL,
		auto_vec<int> *anode_name = NULL,
		auto_vec<int> *anode_regno = NULL,
		int *maxreg_out = NULL,
		unsigned HOST_WIDE_INT avail = ~(unsigned HOST_WIDE_INT) 0,
		const auto_vec<int> *vregno = NULL,
		const auto_vec<int> *vusefirst = NULL,
		int *pinned_out = NULL, int *pinfail_out = NULL,
		const auto_vec<int> *vnotown = NULL,
		const auto_vec<int> *vprefer = NULL,
		unsigned HOST_WIDE_INT entrybusy = 0,
		const auto_vec<int> *vunpin = NULL)
{
  int nn = 0, ne = 0, msq = 0, spl = 0, per = 0, kun = 1, maxreg = -1;
  int pinned = 0, pinfail = 0;

    /* KUNROLL IS THE LCM OF THE PER-VALUE DEGREES, and the unrolled period is
       KUNROLL * II.  Written as the lcm because that is the requirement --
       every value's name count must DIVIDE the unroll factor, since name J
       carries the iterations congruent to J modulo that count and the
       kernel's J-th copy must always be the same name.  §7.8 states it as the
       MAXIMUM power-of-two degree and is right, because the lcm of a set of
       powers of two is the largest of them; the lcm form says why.

       It mattered because the rounding rule lived in two places and only one
       of them read -mlms-exact-degree, so a value could have exact degree 3
       while KUNROLL was the power-of-two 4.  The names then rotate 0,1,2,0
       through one unrolled kernel and 1,2,0,1 through the next -- the copy
       that wrote a register is not the copy that reads it.
       lvx_lms_verify_rename found it as five register conflicts on one loop
       of the gate's own test program.

       THE LCM DOES NOT MAKE EXACT DEGREES USABLE FOR CODEGEN, and treating it
       as if it did was a mistake worth recording.  Code generation needs
       KUNROLL to be a power of two -- the preheader peels `count % kunroll'
       with a mask and takes `count / kunroll' trips with a shift, and on LVX
       that count feeds LOOPDO (§7quinquies.2b, requirement 2).  No power of
       two is divisible by three, so under exact degrees the two requirements
       are incompatible rather than merely in tension, and lvx_option_override
       refuses the combination.  On the codegen path every degree is therefore
       a power of two and this lcm equals the maximum; it is here to state the
       invariant, not to widen it.  */
    for (unsigned v = 0; v < deg.length (); v++)
      {
	int p2 = 1;
	if (lvx_lms_exact_degree)
	  p2 = MAX (deg[v], 1);
	else
	  while (p2 < deg[v])
	    p2 <<= 1;
	/* lcm (kun, p2), by gcd.  Both are small and positive.  */
	int a = kun, b = p2;
	while (b)
	  {
	    int t = a % b;
	    a = b;
	    b = t;
	  }
	kun = kun / a * p2;
	/* An lcm can run away where a maximum cannot -- degrees 3, 4 and 5
	   give 60 -- so it is bounded here and the period test below then
	   declines the loop, rather than letting a 60-fold unroll be built.  */
	if (kun > LVX_LMS_IG_MAX_PERIOD)
	  break;
      }
    per = kun * ii;

    if (per > 0 && per <= LVX_LMS_IG_MAX_PERIOD)
      {
	auto_vec<unsigned HOST_WIDE_INT *> occ;
	auto_vec<int> ncls, nval, nname, nnreg;
	int words = (per + 63) / 64;

	for (unsigned v = 0;
	     v < deg.length () && nn < LVX_LMS_IG_MAX_NODES; v++)
	  {
	    /* The range starts one cycle AFTER a normal definition and AT an
	       earlyclobber one (§7ter.2a).  kvx-unfold.cc computes the
	       equivalent UNFOLD_RANGE_FLAG_DEF_FIRST, dumps it, and has both
	       consumers commented out -- see §7ter.11 for why re-enabling it
	       there is coupled to fixing the effect classification.  */
	    int ec = ec_[v];
	    int d = def_[v] - lo0 + (ec ? 0 : 1);
	    /* THE RANGE INCLUDES THE READER'S CYCLE.  This was
	       `last - def - (ec ? 0 : 1)', ending one cycle before the last
	       reader, on the reading that a VLIW's read-before-write makes the
	       final cycle free.  It does -- WITHIN A BUNDLE.  A write in a
	       DIFFERENT, earlier cycle has no such protection, and that is
	       exactly what modulo expansion creates: name 1's write lands
	       II cycles after name 0's, which can be one cycle BEFORE name 0's
	       reader.

	       Measured on validation/cms/reduce.c, a seven-instruction
	       reduction: a value defined at date 3 and read at 7 was given two
	       names, name 1 written at 6.  The old range made them 4..6 and
	       7..9 -- disjoint, so the colouring put both in r5 -- while the
	       truth is 4..7 and 7..10, which overlap.  The second copy
	       destroyed the first before its read and the program returned a
	       wrong answer.

	       THE INTERVAL IS [def + !earlyclobber, last].  The START was
	       always right and says when the write LANDS: at the end of the
	       defining cycle, so that cycle is not occupied -- unless the
	       destination is earlyclobber, where it is claimed at issue.  The
	       END was wrong because it applied the same read-before-write
	       reasoning at the far end, where it does not hold: a DIFFERENT
	       instance's write landing at the end of cycle last - 1 leaves the
	       register holding the new value during cycle last, and that is
	       not the same bundle.

	       So the length is last - def + (ec ? 1 : 0), not
	       last - def - (ec ? 1 : 0), which was the first attempt at this
	       and shortened an earlyclobber range by two instead of
	       lengthening it by one.  Another value may start at last + 1,
	       which is its_def + 1 with its_def == last -- a write issued in
	       the same cycle as the read, which read-before-write does cover.

	       With the reader's cycle included, a zero-life value gets l = 1
	       naturally and the MAX (l, 1) clamp that §7tricies.4 added becomes
	       redundant -- the sign this is the right convention rather than
	       another adjustment.  */
	    int l = last_[v] - def_[v] + (ec ? 1 : 0);
	    int nr = nregs_[v];
	    /* One name per stage the value spans.  Rounded up to a power of two
	       because that is what the reference allocates (§7quater.5) -- but
	       the rounding is not neutral when ranges are COMBINED, since
	       p2 (a + b) can exceed p2 (a) + p2 (b) even though
	       ceil ((a+b)/II) <= ceil (a/II) + ceil (b/II) always holds.
	       -mlms-exact-degree separates the two effects.  */
	    int p2 = 1;
	    if (lvx_lms_exact_degree)
	      p2 = MAX (deg[v], 1);
	    else
	      while (p2 < deg[v])
		p2 <<= 1;
	    /* A VALUE WHOSE ADJUSTED LIFE COLLAPSES TO ZERO IS STILL A VALUE.
	       This used to `continue', dropping it from the graph: it is read
	       in the cycle after it is written, so the range the model computes
	       is empty and it looked like it needed no register.  It does -- it
	       needs its register through the cycle its reader occupies, and in a
	       pipelined kernel two such values coexist where in the original
	       loop they were sequential.

	       lvx_lms_verify_rename found it on gemm, where RA had reused r0 for
	       six values of one body.  Two of them were zero-life in r0, so
	       neither was renamed, and instance i of one landed in the same
	       kernel cycle as instance i-1 of the other: two writes to r0 in one
	       bundle, each with its own reader.  Clamping the range to one cycle
	       puts them in the graph, where they interfere and are given
	       separate registers.  The degree stays 1, so this adds a node and a
	       register but no expansion copy.

	       The MaxLive computation's own `l <= 0' test is a different
	       quantity -- the raw life, not the adjusted one -- and is left
	       alone: what MaxLive counts is a separate question from what the
	       renaming needs.  */
	    l = MAX (l, 1);

	    /* One node per NAME of this value.  Name J carries the instances
	       of iterations congruent to J modulo P2, which is every P2-th
	       kernel copy -- exact because P2 divides KUNROLL.  */
	    for (int j = 0; j < p2 && nn < LVX_LMS_IG_MAX_NODES; j++)
	      {
		unsigned HOST_WIDE_INT *b = XCNEWVEC (unsigned HOST_WIDE_INT,
						      words);
		for (int k = j; k < kun; k += p2)
		  for (int t = 0; t < l; t++)
		    {
		      int c = (d + k * ii + t) % per;
		      b[c / 64] |= (unsigned HOST_WIDE_INT) 1 << (c % 64);
		    }
		occ.safe_push (b);
		ncls.safe_push ((int) lvx_lms_rclass_of (nr));
		nval.safe_push ((int) v);
		nname.safe_push (j);
		nnreg.safe_push (nr);
		nn++;
	      }
	  }

	/* Interference: two nodes meet iff their occupancies share a cycle.
	   Per-class neighbour counts are what the squeeze consumes.  */
	auto_vec<int> degc;
	degc.safe_grow_cleared (nn * LVX_RC_N);
	for (int a = 0; a < nn; a++)
	  for (int b2 = a + 1; b2 < nn; b2++)
	    {
	      bool meet = false;
	      for (int w = 0; w < words && !meet; w++)
		if (occ[a][w] & occ[b2][w])
		  meet = true;
	      if (!meet)
		continue;
	      ne++;
	      degc[a * LVX_RC_N + ncls[b2]]++;
	      degc[b2 * LVX_RC_N + ncls[a]]++;
	    }

	/* SIMPLIFY: repeatedly remove a node whose squeeze is below its class
	   size, which is Smith's trivially-colourable test in place of
	   Chaitin's degree one.  Whatever cannot be removed would have to be
	   spilled or the II raised -- which is the feasibility answer §7.8
	   wants inside the II search.  */
	auto_vec<bool> gone;
	gone.safe_grow_cleared (nn);
	/* The removal order IS the colouring order, reversed.  Recording it is
	   the whole of what select needs and the whole of what was absent.  */
	auto_vec<int> order;
	int removed = 0;
	bool progress = true;
	while (progress)
	  {
	    progress = false;
	    for (int a = 0; a < nn; a++)
	      {
		if (gone[a])
		  continue;
		int deg[LVX_RC_N];
		for (int c = 0; c < LVX_RC_N; c++)
		  deg[c] = degc[a * LVX_RC_N + c];
		int sq = lvx_lms_squeeze ((enum lvx_lms_rclass) ncls[a], deg);
		if (sq > msq)
		  msq = sq;
		if (sq < lvx_lms_rclass_size[ncls[a]])
		  {
		    gone[a] = true;
		    order.safe_push (a);
		    removed++;
		    progress = true;
		    for (int b2 = 0; b2 < nn; b2++)
		      {
			if (gone[b2] || b2 == a)
			  continue;
			bool meet = false;
			for (int w = 0; w < words && !meet; w++)
			  if (occ[a][w] & occ[b2][w])
			    meet = true;
			if (meet)
			  degc[b2 * LVX_RC_N + ncls[a]]--;
		      }
		  }
	      }
	  }
	spl = nn - removed;
	if (stuck)
	  for (int a = 0; a < nn; a++)
	    if (!gone[a])
	      stuck->safe_push (nval[a]);

	/* SELECT.  Pop the simplify stack and give each node the lowest
	   register its neighbours leave free.  Three things make this more than
	   a graph-colouring exercise on LVX:

	   ALIGNMENT IS THE WHOLE DIFFICULTY, and it is why Chaitin's degree was
	   not enough in the first place (§7ter.9).  A pair must be
	   even-aligned and a quad 4-aligned -- `lvx_hard_regno_mode_ok'
	   enforces it and `lvx_pgr_reg_name'/`lvx_qgr_reg_name' assert it --
	   so a candidate must satisfy `regno % nregs == 0' and have all NREGS
	   registers free.  A single placed mid-run denying every aligned quad
	   is exactly the fragmentation `squeeze' predicts and a plain degree
	   cannot (§7ter.10), and here it is the thing that can actually fail.

	   A NEIGHBOUR BLOCKS ITS WHOLE TUPLE, not one register: the alias
	   relation is a laminar family because the tuples are aligned
	   (§7ter.9), so a pair at r4 denies r4 and r5 and nothing else.

	   AND A NODE LEFT IN THE GRAPH GETS -1 rather than a guess.  Those are
	   the STUCK nodes above; inventing a register for one would turn a
	   feasibility failure the II search knows how to act on into a
	   miscompile it does not.  */
	auto_vec<int> reg;
	reg.safe_grow_cleared (nn);
	for (int a = 0; a < nn; a++)
	  reg[a] = -1;

	/* THE USE_FIRST PIN, applied BEFORE select so the free choices work
	   around it rather than having to be undone (§7ter.14).  A value whose
	   use precedes its def across the back edge gets its name 0 from
	   OUTSIDE the kernel -- the preheader wrote the original register --
	   so name 0 must be that register and cannot be renamed.  §7ter.4's
	   normalisation states the rule; §7ter.7 item 3 records the reference
	   doing it, and calls it correct for the same reason.
	   The other names of the same value are ordinary and stay free.

	   A pin that collides is COUNTED, not forced: two USE_FIRST values
	   that interfere and arrived in the same register would be a
	   contradiction in the input, so reporting it is the honest response
	   where overwriting one of them silently is not.  */
	if (vregno && vusefirst)
	  for (int a = 0; a < nn; a++)
	    {
	      int v = nval[a];
	      if (nname[a] != 0
		  || v >= (int) vregno->length () || (*vregno)[v] < 0)
		continue;
	      /* ONLY USE_FIRST IS PRE-COLOURED, and the measurement that says
		 so is worth keeping (§7ter.14).

		 A USE_FIRST value's name 0 comes from outside the kernel -- the
		 preheader wrote that register -- so pinning it is the external
		 constraint §7ter.4 describes.  223 such pins over 163 PolyBench
		 loops, 0 of them unsatisfiable.

		 I ALSO PINNED THE UNEXPANDED VALUES, reasoning that a value of
		 degree 1 has no expansion copy and so nothing to rename: it
		 should keep the register allocation already gave it.  That is
		 WRONG, and the measurement said so loudly -- 476 pins and 141
		 of them unsatisfiable across 45 of 163 loops.

		 The reason is the point of §7ter and worth stating here rather
		 than only in the document: REGISTER ALLOCATION'S ASSIGNMENT IS
		 NOT VALID FOR THE UNROLLED KERNEL.  Two values that never
		 interfere in the original loop can interfere once the kernel is
		 unrolled KUNROLL times with iterations overlapping, because
		 their occupancies meet modulo the unrolled period.  RA was
		 solving a different problem.  So the kernel's allocation is a
		 fresh one and every value is renameable, including those whose
		 lifetime fits in one stage -- which is also why §7quater.5's
		 reconciliation moves exist on the exit side, and why the entry
		 side needs the same treatment rather than a pin.  */
	      /* §7tricies.156: with the preheader copy there is no external
		 constraint left to express -- the copy puts the incoming
		 value into whatever register name 0 is given, so name 0 is
		 an ordinary free choice like names 1..p2-1.  This is the
		 "entry side needs the same treatment rather than a pin" the
		 paragraph above asks for.

		 §7tricies.158: per value rather than for all of them, so
		 -mlms-ph-mask can keep the pin on every range but one and
		 ask which one's un-pinning is unsound.  A range left pinned
		 gets reg[0] == regno and so no entry copy, the emission
		 keying on exactly that.  */
	      if (lvx_lms_preheader_copy && (lvx_lms_ph_parts & 1)
		  && vunpin && v < (int) vunpin->length () && (*vunpin)[v])
		continue;
	      if (v >= (int) vusefirst->length () || !(*vusefirst)[v])
		continue;
	      int r = (*vregno)[v];
	      int nr = nnreg[a];
	      if (r < 0 || r + nr > 64 || r % nr != 0)
		{
		  pinfail++;			/* unalignable, so unpinnable */
		  continue;
		}
	      bool clash = false;
	      for (int b2 = 0; b2 < nn && !clash; b2++)
		{
		  if (b2 == a || reg[b2] < 0)
		    continue;
		  bool meet = false;
		  for (int w = 0; w < words && !meet; w++)
		    if (occ[a][w] & occ[b2][w])
		      meet = true;
		  if (meet && r < reg[b2] + nnreg[b2] && reg[b2] < r + nr)
		    clash = true;
		}
	      if (clash)
		{
		  pinfail++;
		  continue;
		}
	      reg[a] = r;
	      if (r + nr - 1 > maxreg)
		maxreg = r + nr - 1;
	      pinned++;
	    }

	for (int k = (int) order.length () - 1; k >= 0; k--)
	  {
	    int a = order[k];
	    if (reg[a] >= 0)
	      continue;			/* pinned above */
	    int nr = nnreg[a];
	    unsigned HOST_WIDE_INT busy = 0;
	    for (int b2 = 0; b2 < nn; b2++)
	      {
		if (b2 == a || reg[b2] < 0)
		  continue;
		bool meet = false;
		for (int w = 0; w < words && !meet; w++)
		  if (occ[a][w] & occ[b2][w])
		    meet = true;
		if (!meet)
		  continue;
		for (int t = 0; t < nnreg[b2] && reg[b2] + t < 64; t++)
		  busy |= (unsigned HOST_WIDE_INT) 1 << (reg[b2] + t);
	      }
	    /* AND NAME 0 OF A USE-HEADED RANGE MUST NOT TAKE A REGISTER WHOSE
	       INCOMING VALUE THE PIPELINE STILL READS.  §7tricies.156.

	       AVAIL is what the colouring may use in the KERNEL, where the
	       values the preheader left behind are dead.  The entry copy runs
	       BEFORE the prologue, where they are not: measured on zlib's
	       inflate_blocks loop 5, name 0 of the range carried in r9 was
	       given r3, so the entry copy `r3 <- r9' destroyed the incoming
	       r3 that the prologue's very next instruction reads as the
	       source of `addw $r40 = $r3, 8'.

	       The pin hid this by construction -- name 0 WAS the architected
	       register, so it never landed on another value's -- which is why
	       freeing name 0 needs this constraint to replace it rather than
	       simply removing one.  ENTRYBUSY is the live-in set at the loop
	       header, and it binds only the name the entry copy writes: names
	       1..p2-1 are written inside the kernel, after those values have
	       been read for the last time.  */
	    if (entrybusy && nname[a] == 0 && vusefirst
		&& nval[a] < (int) vusefirst->length ()
		&& (*vusefirst)[nval[a]])
	      busy |= entrybusy;

	    /* MUST NOT KEEP ITS OWN REGISTER.  §7tricies.113.

	       A live-out range whose CTRL_LIVEOUT arc was dropped promised
	       that the body writes a name of its OWN and the drain copies it
	       back -- so the one register it may not be given is the
	       architected one it would otherwise still be writing.  Expressed
	       as an exclusion here, where the selection already knows which
	       registers are unavailable, rather than checked afterwards:
	       verifying the promise refused 32 loops on TSVC2 and cost 18
	       built pipelines, because nothing had told the colouring to keep
	       it.  A constraint the allocator can satisfy beats a condition it
	       can only fail.  */
	    if (vnotown && nval[a] < (int) vnotown->length ()
		&& (*vnotown)[nval[a]] >= 0)
	      {
		int own = (*vnotown)[nval[a]];

		for (int t = 0; t < nr && own + t < 64; t++)
		  busy |= (unsigned HOST_WIDE_INT) 1 << (own + t);
	      }
	    /* THE CHEAPEST COPY IS THE ONE THE NAME MAKES UNNECESSARY.
	       §7tricies.122, after Moon & Ebcioglu 1992.

	       The live-out copy already deletes itself when the name it would
	       read is the architected register --

		 if (nm < 0 || nm == (int) q->regno) continue;

	       -- but nothing ever tried to arrange that.  The selection below
	       takes the LOWEST free aligned register, which is as good as any
	       other for pressure and worse than one particular choice for code
	       size: the range's own REGNO, where it is free, costs the same
	       register and removes a copy from the drain.

	       Moon & Ebcioglu state it as a preference and not a constraint --
	       "the program prefers a destination register d' equal to the
	       original destination register d ... since this will eliminate
	       the need for leaving a copy operation d := d'" -- and a
	       preference is what it has to be: taking REGNO when it is busy
	       would be a pin, and pins that cannot be satisfied were measured
	       at 476 with 141 unsatisfiable.  So it is tried first and the
	       first-fit scan follows if it is not available.

	       VPREFER is populated only where the colouring can KNOW the copy
	       would be deleted, which is degree one: the drain reads
	       `lvx_lms_lrange_reg (q, best, delayed)' with a BEST decided at
	       emission, and at one name every BEST gives the same register.
	       At a higher degree the preference would be a guess at which name
	       the drain reads, and guessing the wrong one spends a
	       higher-numbered register to delete nothing.  */
	    if (vprefer && a < nn && nval[a] < (int) vprefer->length ()
		&& (*vprefer)[nval[a]] >= 0)
	      {
		int p = (*vprefer)[nval[a]];
		unsigned HOST_WIDE_INT want = 0;

		if (p >= 0 && p + nr <= 64 && p % nr == 0)
		  {
		    for (int t = 0; t < nr; t++)
		      want |= (unsigned HOST_WIDE_INT) 1 << (p + t);
		    if (!(busy & want) && !(~avail & want))
		      {
			reg[a] = p;
			if (p + nr - 1 > maxreg)
			  maxreg = p + nr - 1;
		      }
		  }
	      }
	    if (reg[a] >= 0)
	      continue;			/* took the copy-deleting register */
	    for (int r = 0; r + nr <= 64; r += nr)	/* aligned by construction */
	      {
		unsigned HOST_WIDE_INT want = 0;
		for (int t = 0; t < nr; t++)
		  want |= (unsigned HOST_WIDE_INT) 1 << (r + t);
		if ((busy & want) || (~avail & want))
		  continue;
		reg[a] = r;
		if (r + nr - 1 > maxreg)
		  maxreg = r + nr - 1;
		break;
	      }
	  }

	/* SELECT CAN FAIL WHERE SIMPLIFY SUCCEEDED, and that was invisible.
	   §7tricies.39.

	   The loop above leaves REG[A] at -1 when no aligned register in AVAIL
	   is free, and recorded nothing: STUCK is filled from the SIMPLIFY
	   phase only -- the nodes it could not remove -- and SPL is `nn -
	   removed', so a node removed cleanly and then left uncoloured
	   appeared in neither.  The verify block below made it worse by
	   skipping `reg[a] < 0', so the one check advertised as "loud, not
	   silent" was silent about exactly this.

	   It is not a corner case.  The II search's feasibility test reads
	   SPL and passes ANODE_REGNO as null, so the verify block does not
	   even run there: an II was accepted whenever SIMPLIFY succeeded,
	   while SELECT could still run out of registers at that II.  On
	   heat-3d's loop 7 (44 nodes, maxreg 63) all four names of two
	   live ranges came back unassigned, and because lvx_lms_colour_lranges
	   seeds REG[] with the original regno -- which is right for a held
	   live range -- they read back as r10,r10,r10,r10: a p2 = 4 live range with
	   no expansion at all, whose 15-cycle span then self-overlaps at
	   II = 7.  The occupancy check reported it as a conflict with another
	   live range's name 3, which looked like a dropped anti-dependence and is
	   not one.

	   Reported through STUCK as well as SPL: STUCK's documented meaning is
	   the set of nodes that could not be coloured, the simplify phase is
	   only one way to get there, and the search's blame loop needs the
	   unit to attribute pressure to.  */
	{
	  int unassigned = 0;

	  for (int a = 0; a < nn; a++)
	    if (reg[a] < 0 && gone[a])
	      {
		unassigned++;
		if (stuck)
		  stuck->safe_push (nval[a]);
	      }
	  spl += unassigned;
	}

	/* VERIFY, because an assignment is checkable and a feasibility answer
	   was not.  Two interfering nodes may not share a register, and every
	   tuple must be aligned.  Reported through SPL -- the same channel a
	   colouring failure uses -- so a broken assignment cannot be mistaken
	   for a working one by a caller that only reads the counts.  */
	if (anode_regno)
	  {
	    int bad = 0;
	    for (int a = 0; a < nn; a++)
	      {
		if (reg[a] < 0)
		  continue;
		if (reg[a] % nnreg[a] != 0)
		  bad++;
		for (int b2 = a + 1; b2 < nn; b2++)
		  {
		    if (reg[b2] < 0)
		      continue;
		    bool meet = false;
		    for (int w = 0; w < words && !meet; w++)
		      if (occ[a][w] & occ[b2][w])
			meet = true;
		    if (meet
			&& reg[a] < reg[b2] + nnreg[b2]
			&& reg[b2] < reg[a] + nnreg[a])
		      bad++;
		  }
	      }
	    if (bad)
	      spl += bad;			/* loud, not silent */
	  }

	if (anode_value)
	  for (int a = 0; a < nn; a++)
	    anode_value->safe_push (nval[a]);
	if (anode_name)
	  for (int a = 0; a < nn; a++)
	    anode_name->safe_push (nname[a]);
	if (anode_regno)
	  for (int a = 0; a < nn; a++)
	    anode_regno->safe_push (reg[a]);

	for (int a = 0; a < nn; a++)
	  XDELETEVEC (occ[a]);
      }
  
  *nn_out = nn; *ne_out = ne; *msq_out = msq; *spl_out = spl;
  *per_out = per; *kun_out = kun;
  if (maxreg_out)
    *maxreg_out = maxreg;
  if (pinned_out)
    *pinned_out = pinned;
  if (pinfail_out)
    *pinfail_out = pinfail;
}

/* Declared in lvx-lms.h.  */

/* Is every date within its Estart/Lstart window at this II?  §7quinvicies.2.

   The author's invariant (2026-10-27): a date handed to a scheduler -- an MCRL
   date, or any other -- must satisfy

       Estart (i)  <=  date (i)  <=  Lstart (i)

   at the II in force, "else something is wrong".  It is the right check
   because both bounds are properties of the graph and the period alone, so a
   date outside them cannot be realised by any feasible schedule: the
   formulation that produced it is wrong, not merely unlucky.

   Estart is taken from lvx_plc, which is exact and parametric -- the earliest
   potentials at the period, over arc lengths theta - lambda * omega, so the
   cross-iteration arcs are accounted for rather than ignored.

   Lstart needs a makespan, and THE MAKESPAN MUST BE BOUNDED BY THE GRAPH, not
   read off the schedule (author, 2026-10-27: "the makespan of the graph must be
   limited like for the LStart dates").  Using the achieved span would make the
   test self-referential -- the schedule would define the window it is then
   judged against, and could not violate it.  Huff's bound is the one to use:

       Lstart (Stop) = ceil (Estart (Stop) / II) * II

   the critical path rounded up to a whole number of stages, which is a
   function of the graph and the period alone.  Huff sets it deliberately loose
   in exactly this way to buy slack and reduce backtracking (§7tervicies).

   WHAT THIS MUST AND MUST NOT BE APPLIED TO (author, 2026-10-27: "it is not
   the CMS dates, it is the first dates from Bellman-Ford at the current II").
   The window is a DEPENDENCE-ONLY object.  A resource-feasible schedule may
   legitimately sit outside it, because resources stretch a schedule past the
   dependence critical path -- measured on PolyBench lvx-2, SIGMA lies outside
   the window in 45 of 163 loops over 267 nodes, with spans of 38 against a
   bound of 24.  None of those is an error.  So the invariant belongs to the
   dates the SCHEDULER STARTS FROM -- Bellman-Ford's at the current II, and the
   MCRL dates that refine them -- not to the schedule it ends with.

   Two things are therefore checkable.  EMPTY tells whether the window itself
   is non-empty, Estart (i) <= Lstart (i) for every node, which is a property of
   the graph, the period and the makespan bound; an empty window means the II
   cannot carry the dependence graph within the bound.  And BAD counts the
   given dates that fall outside, which is the gate an MCRL date must pass.

   Returns BAD, with the first offender through FIRST, the bound through
   MAKESPAN, and the empty-window count through EMPTY.  */

int
lvx_lms_dates_in_window (const auto_vec<rtx_insn *> &seq,
			 const auto_vec<lvx_lms_arc> &arcs,
			 const auto_vec<int> &dates,
			 const auto_vec<int> &tosink,
			 int ii, int *first_out, int *makespan_out,
			 int *empty_out, auto_vec<int> *estart_out,
			 auto_vec<int> *lstart_out,
			 int makespan_floor)
{
  unsigned n = seq.length ();
  *first_out = -1;

  /* Estart at this II, from the parametric potentials.  */
  lvx_plc plc;
  for (unsigned k = 0; k <= n; k++)
    plc.add_node ();
  for (unsigned k = 0; k < n; k++)
    plc.add_arc (0, k + 1, 0, 0);
  for (unsigned a = 0; a < arcs.length (); a++)
    plc.add_arc (arcs[a].src + 1, arcs[a].dst + 1, arcs[a].theta,
		 arcs[a].omega);
  if (plc.solve (ii) != LVX_PLC_SOLVED)
    return -1;				/* cannot judge */

  int lo = dates.is_empty () ? 0 : dates[0];
  for (unsigned i = 1; i < dates.length (); i++)
    if (dates[i] < lo)
      lo = dates[i];

  int root = (int) plc.date (0);

  /* Estart (Stop): the earliest the sink can go, over every node's own
     processing time.  Then round it up to whole stages.  */
  int estop = 1;
  for (unsigned i = 0; i < n; i++)
    {
      int e = (int) plc.date (i + 1) - root + lvx_lms_latency (seq[i]);
      if (e > estop)
	estop = e;
    }
  int makespan = ((estop + ii - 1) / ii) * ii;
  /* The author's feedback (§7quinvicies.4): the dependence-only bound can sit
     BELOW the span list scheduling achieves -- measured in 45 of 163 loops --
     and solving inside such a window asks for dates the resources have already
     disproved.  MAKESPAN_FLOOR carries the achieved span back in, rounded up
     to whole stages like the bound itself.  Zero on the first solve, which is
     where the iteration starts.  */
  if (makespan_floor > makespan)
    makespan = ((makespan_floor + ii - 1) / ii) * ii;
  *makespan_out = makespan;

  int bad = 0, empty = 0;
  for (unsigned i = 0; i < n; i++)
    {
      int estart = (int) plc.date (i + 1) - root;
      int lstart = makespan - tosink[i];
      if (estart_out)
	estart_out->safe_push (estart);
      if (lstart_out)
	lstart_out->safe_push (lstart);
      if (estart > lstart)
	empty++;
      if (dates.length () != n)
	continue;			/* nothing to judge */
      int d = dates[i] - lo;
      if (d < estart || d > lstart)
	{
	  if (*first_out < 0)
	    *first_out = (int) i;
	  bad++;
	}
    }
  *empty_out = empty;
  return bad;
}

/* MCRL: minimum cumulative register lifetime dates, by min-cost flow.
   Dupont de Dinechin, LCPC 1996, the half lvx-plc.cc does not implement.
   §7quinvicies.

   THE PRIMAL.  With DATE the dates, LIFE the lifetimes and NREGS the width in
   registers of the value a node produces, at period II:

       min   sum over i of  NREGS (i) * LIFE (i)
       s.t.  date (j) - date (i)  >=  theta - II * omega        (D, every arc)
             life (i) - date (j) + date (i)  >=  II * omega     (L, RAW arcs)

   (L) is the lifetime of i's value as seen by consumer j: the value i writes
   in iteration k is read by j in iteration k + omega, so the span is
   date (j) + II * omega - date (i).  The objective is therefore a sum of
   MAXIMA, which is what makes this an LP rather than a longest path, and the
   dual of that LP is a flow.

   THE DUAL, derived rather than asserted, because every sign here is a place
   to be wrong.  Let y_a >= 0 be the multiplier of (D) on arc a and z_a >= 0
   that of (L).  Then

     per LIFE (i):  sum of z over the RAW arcs out of i  =  NREGS (i)
     per DATE (i):  sum_in y + sum_{z of i} z  =  sum_out y + sum_{z into i} z

   The second is flow conservation ONCE z is carried on the REVERSED arc: z_a
   for a = (i, j) leaves the CONSUMER j and arrives at the PRODUCER i.  The
   first is not a conservation law but a fixed total, so the producer is split
   in two: a date node D_i carrying the y-flow, and a collector C_i that the
   z-arcs enter.  Requiring D_i to SUPPLY nregs (i) and C_i to CONSUME it makes
   the two equations hold together -- which is exactly the author's adjustment
   (2026-10-27): THE FLOW LEAVING A PRODUCER NODE IS THE NUMBER OF REGISTERS IN
   THE PRODUCED TUPLE.  It is a requirement on a node, not a cost on an arc, so
   nothing in the objective is rescaled and the tuple widths enter by
   conservation.

   The dual maximises sum y (theta - II omega) + sum z (II omega), so as a
   MINIMISATION the arc costs are the negatives:

       y-arc  D_i -> D_j   cost  II * omega - theta      capacity unbounded
       z-arc  D_j -> C_i   cost  -II * omega             capacity unbounded

   READING THE DATES BACK.  This solver keeps shortest-path potentials,
   pi (head) <= pi (tail) + cost on every residual arc.  On the y-arc that is
   pi (D_j) - pi (D_i) <= II omega - theta, so with

       date (i)  =  -pi (D_i)

   we get date (j) - date (i) >= theta - II * omega: the dependences hold BY
   CONSTRUCTION, for every arc, because every capacity is unbounded and so no
   arc can saturate.  Where an arc carries flow its reverse residual gives the
   opposite inequality too, hence equality -- complementary slackness putting
   the tight dependences exactly on the critical arcs.

   WHY THE WINDOW ARCS.  Unbounded dates would let a node with no consumer
   float anywhere feasible.  A virtual node D_0 and the two arcs

       D_0 -> D_i  cost -estart (i)        (date (i) >= estart (i))
       D_i -> D_0  cost  lstart (i)        (date (i) <= lstart (i))

   pin every date into the window at this II, which is the author's
   requirement that the makespan be limited like the Lstart dates.  Their
   round trip costs lstart (i) - estart (i) >= 0 exactly when the window is
   non-empty, and a y-cycle costs II * Omega - Theta >= 0 exactly when
   II >= RecMII, so at a feasible II with a non-empty window the network has
   no negative circuit and the solve is bounded.  C_i has no outgoing arc, so
   no z-arc lies on a cycle.

   Returns the network status.  On LVX_NETWORK_SOLVED, DATES holds one date per
   node, normalised so the smallest is zero.  */

enum lvx_network_result
lvx_lms_mcrl_dates (const auto_vec<rtx_insn *> &seq,
		    const auto_vec<lvx_lms_arc> &arcs,
		    const auto_vec<int> &estart,
		    const auto_vec<int> &lstart,
		    int ii, auto_vec<int> &dates,
		    HOST_WIDE_INT *cost_out, unsigned *nn_out,
		    unsigned *ne_out)
{
  unsigned n = seq.length ();
  *cost_out = 0;
  *nn_out = *ne_out = 0;
  if (n == 0 || estart.length () != n || lstart.length () != n)
    return LVX_NETWORK_INFEASIBLE;

  lvx_network net;

  /* D_0, then one date node per instruction, then the collectors.  */
  unsigned d0 = net.add_node (0);
  auto_vec<unsigned> dnode, cnode;
  for (unsigned i = 0; i < n; i++)
    dnode.safe_push (net.add_node (0));
  for (unsigned i = 0; i < n; i++)
    cnode.safe_push (net.add_node (0));

  /* NREGS per producer, counted only where the value is actually read: a
     definition nothing consumes has no lifetime and must not demand flow,
     which would make the instance infeasible rather than free.  This is the
     same criterion lvx_lms_collect_values applies.  */
  auto_vec<int> nregs;
  for (unsigned i = 0; i < n; i++)
    {
      bool consumed = false;
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].src == i && arcs[a].kind == LVX_ARC_REG_RAW)
	  {
	    consumed = true;
	    break;
	  }
      int nr = 0;
      if (consumed)
	{
	  rtx ds = single_set (seq[i]);
	  nr = (ds && REG_P (SET_DEST (ds))) ? REG_NREGS (SET_DEST (ds)) : 1;
	  nr = MAX (nr, 1);
	}
      nregs.safe_push (nr);
      if (nr)
	{
	  net.set_requirement (dnode[i], nr);
	  net.set_requirement (cnode[i], -nr);
	}
    }

  /* The dependence arcs, every kind: they all constrain the dates.  */
  for (unsigned a = 0; a < arcs.length (); a++)
    net.add_arc (dnode[arcs[a].src], dnode[arcs[a].dst], -1,
		 (HOST_WIDE_INT) ii * arcs[a].omega - arcs[a].theta);

  /* The lifetime arcs, RAW only, and reversed: consumer to producer.  */
  for (unsigned a = 0; a < arcs.length (); a++)
    if (arcs[a].kind == LVX_ARC_REG_RAW && nregs[arcs[a].src])
      net.add_arc (dnode[arcs[a].dst], cnode[arcs[a].src], -1,
		   -(HOST_WIDE_INT) ii * arcs[a].omega);

  /* The window.  */
  for (unsigned i = 0; i < n; i++)
    {
      net.add_arc (d0, dnode[i], -1, -estart[i]);
      net.add_arc (dnode[i], d0, -1, lstart[i]);
    }

  *nn_out = net.node_count ();
  *ne_out = net.arc_count ();

  enum lvx_network_result r = net.solve ();
  if (r != LVX_NETWORK_SOLVED)
    return r;

  *cost_out = net.cost ();

  int lo = 0;
  for (unsigned i = 0; i < n; i++)
    {
      int d = -(int) net.potential (dnode[i]);
      dates.safe_push (d);
      if (i == 0 || d < lo)
	lo = d;
    }
  for (unsigned i = 0; i < n; i++)
    dates[i] -= lo;

  return LVX_NETWORK_SOLVED;
}

/* The cumulative register lifetime of DATES, weighted by tuple width, which
   is the quantity MCRL minimises and the only honest way to say whether it
   did better than the alternative.  */

static HOST_WIDE_INT
lvx_lms_cumulative_lifetime (const auto_vec<rtx_insn *> &seq,
			     const auto_vec<lvx_lms_arc> &arcs,
			     const auto_vec<int> &dates, int ii)
{
  HOST_WIDE_INT total = 0;
  for (unsigned i = 0; i < seq.length (); i++)
    {
      int life = 0;
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].src == i && arcs[a].kind == LVX_ARC_REG_RAW)
	  {
	    int span = dates[arcs[a].dst] + (int) arcs[a].omega * ii - dates[i];
	    if (span > life)
	      life = span;
	  }
      if (life <= 0)
	continue;
      rtx ds = single_set (seq[i]);
      int nr = (ds && REG_P (SET_DEST (ds))) ? REG_NREGS (SET_DEST (ds)) : 1;
      total += (HOST_WIDE_INT) MAX (nr, 1) * life;
    }
  return total;
}

/* THE PIPELINE PLAN: what codegen would emit, settled before an edge moves.
   §7quinquies.8.

   §7quinquies.6 keeps the reference's two-phase split -- build the whole CFG,
   then fill it -- because "it is what allows the epilogue shape to be settled
   before a single insn is committed".  This goes one phase earlier: the shape
   is settled before a single EDGE is committed, by computing the master
   schedule and the node assignment as data.  Nothing here touches the CFG, so
   it is checkable and measurable at no risk, and it produces the one quantity
   Llosa's whole axis is about (§7undecies) and that §7undecies.2a measured only
   indirectly through SC, BS and Kmin: HOW MUCH CODE a pipelined loop costs.

   THE MASTER SCHEDULE, after PaCT 1997 §1.2 and §7.9.  An instance of task I is
   a copy at (issuedate + l * II, iteration + l), and the order is the paper's
   total order on (issuedate, iteration, ordering).  Instances run over
   0 .. nstages + kunroll - 1, which is what `unfold_codegen_make_nodes' uses.

   ITER_SLOT closes the kernel, and §7quinquies.2a is the constraint that
   matters: it must be an instance of the ORIGINAL LOOPBACK BRANCH, not any
   conditional branch, or the kernel's back edge cannot fold into a doloop_end.
   The reference carries UNFOLD_TASK_FLAG_LOOP for exactly this and tests the
   narrow flag; here the loopback is the last JUMP_P in SEQ, which is in chain
   order from the header and so is the latch terminator (§7undecies.2b).  The
   `iteration > 0' conjunct is a second and different constraint -- it forces a
   non-zero iteration so the insns can be recycled rather than copied.

   Reports, per loop: the master slot count, how many fall in the prologue, the
   kernel and the main epilogue, how many blocks would be REUSED against
   ALLOCATED (§7quinquies.2's three cases), and how many per-exit epilogues the
   structure needs.  */

/* ONE SLOT OF THE MASTER SCHEDULE, and the node it is emitted into.

   The master schedule is the whole pipeline laid out flat: every instance of
   every instruction at its own date, walked in (date, iteration, order).
   Recording it is what lets the plan's counts and the emission's placement be
   the SAME walk, rather than two walks that agree today.  §7quinquies.2.  */

struct lvx_lms_slot
{
  int insn;			/* index into SEQ */
  int iter;			/* which iteration's instance */
  int date;			/* master-schedule cycle, origin at zero */
  int node;			/* index into the node list */
};

enum lvx_lms_nodekind
{
  LVX_LMS_NODE_PROLOGUE,
  LVX_LMS_NODE_KERNEL,
  LVX_LMS_NODE_EPILOGUE
};

/* A BLOCK THE PIPELINE NEEDS.  The prologue stages and the kernel come out of
   the walk up to ITER_SLOT; the epilogues come from the exits found in it --
   one per non-chosen jump instance, plus the main one that ITER_SLOT itself
   falls into.  EXIT_DATE and EXIT_ITER name the instance whose exit enters an
   epilogue, which is what decides WHICH iterations that epilogue drains.  */

struct lvx_lms_node
{
  enum lvx_lms_nodekind kind;
  int stage;
  int exit_date, exit_iter, exit_insn;	/* the jump instance whose exit
					   enters it -- which is also where
					   a path into it LEAVES the walk */
  bool main_epilogue;
};

struct lvx_lms_plan
{
  int slots;			/* master slots in all */
  int prologue;			/* slots before the kernel head */
  int kernel;			/* slots in the k-unrolled kernel */
  int epilogue;			/* slots after it, the main epilogue */
  int epilogue_sym;		/* what the old symmetry assumption gave */
  int exit_slots;		/* slots in all the early-exit epilogues */
  int steady_bad;		/* instructions NOT appearing kunroll times in
				   the kernel -- nonzero means the window is
				   not a steady state */
  int blocks_reused;		/* source body blocks taken over */
  int blocks_allocated;		/* fresh blocks the stages need */
  int exit_epilogues;		/* one per exit before ITER_SLOT */
  bool ok;
  const char *why;
};

static void
lvx_lms_build_plan (const auto_vec<rtx_insn *> &seq,
		    const auto_vec<int> &sigma, int ii, int kunroll,
		    int nstages, unsigned nblocks, struct lvx_lms_plan *pl,
		    auto_vec<lvx_lms_slot> *slots = NULL,
		    auto_vec<lvx_lms_node> *nodes_out = NULL)
{
  memset (pl, 0, sizeof (*pl));
  pl->why = "no loopback branch";
  int loopback = -1;
  for (unsigned i = 0; i < seq.length (); i++)
    if (JUMP_P (seq[i]))
      loopback = (int) i;		/* the LAST jump: the latch terminator */
  if (loopback < 0 || seq.is_empty () || ii <= 0)
    return;

  int lo = lvx_lms_sigma_min (sigma);
  int instances = nstages + kunroll;

  /* ITER_SLOT: the lowest instance of the LOOPBACK branch whose iteration is
     non-zero and which sits at or after the adjusted slot, the latter being
     the loopback at iteration kunroll - 1 (§7quinquies.2a).  */
  /* THE ADJUSTED SLOT IS MEASURED FROM THE LAST-SCHEDULED INSTRUCTION, not
     from the loopback.  The kernel window ends at ITER_SLOT and is
     kunroll * II cycles long, so every instruction's phase must fall inside
     it -- and an instruction scheduled AFTER the loopback has a phase beyond
     iter_date, which clips its instances and leaves the kernel short of a
     steady state.

     Taking it from the loopback's own phase left 87 of 124 loops not a steady
     state, with a deficit of one to three instructions each: exactly the
     instructions scheduled after the branch.  `main' loop 1 puts the jump at
     phase 5 and its last instruction at phase 6, which is the shape -- and
     the branch being last is an assumption, not a property, since sigma
     places it wherever the resources allow.

     With the maximum phase the window is [iter_date - kunroll * II + 1,
     iter_date] and every phase from 0 to max is inside it, so each
     instruction contributes exactly kunroll instances.  For a loop whose
     branch IS last the two formulas agree, which is why f1 was already
     right.  */
  int maxphase = 0;
  for (unsigned i = 0; i < seq.length (); i++)
    if (sigma[i] - lo > maxphase)
      maxphase = sigma[i] - lo;
  int adjusted = maxphase + (kunroll - 1) * ii;
  int iter_date = -1, iter_iteration = -1;
  for (int l = 1; l < instances; l++)
    {
      int d = sigma[loopback] - lo + l * ii;
      if (d >= adjusted)
	{
	  iter_date = d;
	  iter_iteration = l;
	  break;
	}
    }
  pl->why = "no qualifying loopback instance";
  if (iter_date < 0)
    return;
  /* THE KERNEL IS THE KUNROLL * II CYCLES ENDING AT ITER_SLOT, not the dates
     at or after (nstages - 1) * II.

     "(nstages - 1) * II is the cycle by which every stage has started, which
     is where the steady state begins" is true and is not the same statement
     as "the kernel starts there".  The kernel must execute every instruction
     exactly KUNROLL times per trip -- that is what a steady state IS -- and
     the window has to be KUNROLL * II cycles long for that to hold.  The
     loopback instance that closes it sits at ITER_SLOT, so the window is the
     one ENDING there.

     Found by hand-tracing the emitted f1.  With the window [5, 12] -- eight
     cycles where kunroll * II is ten -- the kernel held two loads, two IV
     updates, two copies and two branches but ONE fma, so the loop advanced
     its pointer twice per accumulate and returned a wrong answer.  The
     window [3, 12] holds exactly two of each.  Nothing in the plan's own
     counts could have shown this: 12 slots is 12 slots however they are
     distributed, which is why the check below counts per instruction.

     KERNEL_FIRST can go negative when the schedule is shorter than the
     unrolled period; clamped to zero, where the prologue is empty and the
     kernel starts at the first date.  */
  int kernel_first = iter_date - kunroll * ii + 1;
  if (kernel_first < 0)
    kernel_first = 0;


  /* Walk every instance of every task in (date, iteration, order) order,
     counting what falls where.  Everything after ITER_SLOT is filtered out,
     which is what leaves prolog + unrolled kernel + main epilog (§7.9).  */
  int prev_exit = 0, nodes = 0, split = 0;
  bool kernel_seen = false;
  auto_vec<int> exit_date, exit_iter, exit_insn;
  if (slots)
    slots->truncate (0);
  if (nodes_out)
    nodes_out->truncate (0);
  /* TWO PASSES OVER THE FINAL DATE, so the closing branch is emitted LAST
     without the instances beside it being thrown away.  §7tricies.34.

     The filter used to be `d == iter_date && l > iter_iteration', described
     as "everything after ITER_SLOT is filtered out".  At the final date the
     walk visits instances in ascending iteration, so the chosen loopback
     instance -- the lowest iteration reaching that date -- came first and
     every instance of a LATER iteration at the same cycle was dropped.  Those
     instances are the pipelined work of later iterations and belong to the
     kernel: dropping them leaves it short of a steady state, which is a wrong
     answer on every trip (see STEADYBAD below).

     They sit in the SAME CYCLE as the branch, so there is nothing to order
     between them -- the bundler decides that -- and the only requirement is
     that the branch be the block's terminator.  Hence the two passes: pass 0
     emits every instance at this date except the chosen loopback, pass 1
     emits the chosen loopback.  For every other date pass 1 emits nothing.

     On `A[i] = (i % 7) + 1' the window is [10, 25] and the instructions at
     phases 1, 5 and 9 each had their instance at date 25 dropped: 61 kernel
     slots where 16 * 4 = 64 were needed.  */
  for (int d = 0; d <= iter_date; d++)
    for (int pass = 0; pass < 2; pass++)
    for (int l = 0; l < instances; l++)
      for (unsigned i = 0; i < seq.length (); i++)
	{
	  bool chosen = (d == iter_date && l == iter_iteration
			 && (int) i == loopback);

	  if (sigma[i] - lo + l * ii != d)
	    continue;
	  if (pass == 0 ? chosen : !chosen)
	    continue;
	  pl->slots++;
	  if (d < kernel_first)
	    pl->prologue++;
	  else
	    pl->kernel++;

	  /* A new node after every exit, and at the kernel head -- the first
	     because an exit must end a block, the second because the kernel
	     head must be a branch target (§7quinquies.2).  */
	  /* THE KERNEL BEGINS AT THE FIRST SLOT AT OR AFTER KERNEL_FIRST, not
	     at a slot whose date EQUALS it.  §7tricies.49.

	     KERNEL_FIRST is iter_date - kunroll * II + 1, an arithmetic cycle
	     number that no instance need occupy: seidel-2d's loop 2 has phases
	     0, 1, 5, 9 and 12 at II = 5, so its instances fall on 9, 10, 11,
	     12, ... and KERNEL_FIRST is 8.  With an equality test the kernel
	     node was never created at the boundary and every slot from 9
	     onwards was appended to the last PROLOGUE node -- so PL->KERNEL
	     counted 40 slots by date while only 30 of them sat in a node of
	     kernel kind, and each of the ten instructions appeared three times
	     instead of four.

	     STEADYBAD is what caught it, counting per instruction in nodes of
	     kernel kind rather than trusting the by-date total.  Emitted, the
	     kernel block would have been missing a quarter of its work with
	     the rest marooned in the prologue.  */
	  bool enter_kernel = (d >= kernel_first && !kernel_seen);
	  if (prev_exit || enter_kernel || nodes == 0)
	    {
	      if (d >= kernel_first)
		kernel_seen = true;
	      if (nodes && !prev_exit)
		split++;		/* case 1: the kernel begins mid-block */
	      if (nodes - split >= (int) nblocks)
		pl->blocks_allocated++;	/* case 2: source blocks used up */
	      else
		pl->blocks_reused++;	/* case 3: reuse the source node */
	      nodes++;
	      if (nodes_out)
		{
		  lvx_lms_node n;
		  n.kind = (d < kernel_first ? LVX_LMS_NODE_PROLOGUE
			    : LVX_LMS_NODE_KERNEL);
		  n.stage = d / ii;
		  n.exit_date = n.exit_iter = n.exit_insn = -1;
		  n.main_epilogue = false;
		  nodes_out->safe_push (n);
		}
	    }
	  if (slots)
	    {
	      lvx_lms_slot sl;
	      sl.insn = (int) i;
	      sl.iter = l;
	      sl.date = d;
	      sl.node = nodes - 1;
	      slots->safe_push (sl);
	    }
	  prev_exit = JUMP_P (seq[i]) ? 1 : 0;
	  /* EVERY JUMP INSTANCE IN THE WALK EXCEPT ITER_SLOT ITSELF IS AN
	     EARLY EXIT, the loopback's own instances included.  This excluded
	     the loopback (`i != loopback') and undercounted: §7quinquies.2a
	     says the chosen instance closes the k-unrolled kernel and "the
	     other K-1 instances become inverted early exits from it", and the
	     loopback is not always in the last stage either -- one loop of the
	     gate's own program schedules it at stage 1 of 3, so its iteration-0
	     instance lands in the PROLOGUE and is an exit from there.

	     The figure this changes is the one that sizes make_insns, so it
	     mattered: counting only the non-loopback exits said 117 of 124
	     PolyBench loops needed no epilogue machinery at all, which was the
	     wrong scope to build to.  */
	  if (JUMP_P (seq[i])
	      && !(d == iter_date && l == iter_iteration
		   && (int) i == loopback))
	    {
	      pl->exit_epilogues++;
	      exit_date.safe_push (d);
	      exit_iter.safe_push (l);
	      exit_insn.safe_push ((int) i);
	    }
	}

  /* THE EPILOGUES, ENUMERATED RATHER THAN ASSUMED SYMMETRIC.

     This read `pl->epilogue = pl->prologue', "symmetric by construction: the
     ramp down mirrors the ramp up".  That is true of the STAGE COUNT and not
     of the slot count, because the walk up to ITER_SLOT is truncated by the
     ITER_SLOT filter and the drain is not, so the two tails hold different
     numbers of instances.  The old value is kept as EPILOGUE_SYM so the two
     can be compared rather than one silently standing for the other.

     An epilogue drains the iterations in flight when its exit is taken.  Exit
     at the loopback's instance for iteration L_E means no iteration after L_E
     will start, and the iterations already past their own test are L <= L_E --
     so the drain is every slot of those iterations that the walk has not
     already emitted.  Iterations above L_E may have begun speculatively; they
     are discarded, which is sound only because the control dependences from
     the loopback to anything with a side effect are real arcs in the graph
     (that is what -fno-hwloop buys, §7.0) and the schedule therefore never
     put such an instruction before the test that guards it.

     The main epilogue is the same formula at the ITER_SLOT instance, which is
     why there is one loop here and not two.  */
  int maxd = 0;
  for (unsigned i = 0; i < seq.length (); i++)
    if (sigma[i] - lo > maxd)
      maxd = sigma[i] - lo;

  pl->epilogue_sym = pl->prologue;
  for (unsigned e = 0; e <= exit_date.length (); e++)
    {
      /* THE MAIN EPILOGUE IS FIRST, so its block is the one immediately after
	 the last kernel node and the kernel can FALL THROUGH into it --
	 §7quinquies.4's "if the epilogue node is the one immediately after
	 the iteration node".  Taking it last, which read more naturally, put
	 the early-exit epilogues between the kernel and the drain, so the
	 kernel's fall-through landed in an epilogue reachable only from an
	 exit that had not been taken.  The early exits keep the walk's order
	 after it.  */
      bool is_main = (e == 0);
      int de = is_main ? iter_date : exit_date[e - 1];
      int le = is_main ? iter_iteration : exit_iter[e - 1];
      int ie = is_main ? loopback : exit_insn[e - 1];
      int node = -1;

      /* THE NODE IS CREATED WHETHER OR NOT IT HOLDS A SLOT.  An epilogue can
	 be legitimately EMPTY -- in a two-stage pipeline whose loopback sits
	 in the last stage, the iteration whose test fails has already
	 finished, and the later ones are the speculatively started ones that
	 get discarded -- but it is still the block the exit branches to, so
	 it has to exist.  Creating it lazily left f1's two epilogues with no
	 block at all and the node count describing only the prologue and the
	 kernel.  */
      if (nodes_out)
	{
	  lvx_lms_node n;
	  n.kind = LVX_LMS_NODE_EPILOGUE;
	  n.stage = (int) e;
	  n.exit_date = de;
	  n.exit_iter = le;
	  n.exit_insn = ie;
	  n.main_epilogue = is_main;
	  nodes_out->safe_push (n);
	  node = nodes_out->length () - 1;
	}
      else
	node = nodes++;

      for (int d = de; d <= maxd + le * ii; d++)
	for (int l = 0; l <= le; l++)
	  for (unsigned i = 0; i < seq.length (); i++)
	    {
	      if (sigma[i] - lo + l * ii != d)
		continue;
	      /* ALREADY RUN IS A POSITION IN THE WALK, NOT A BODY INDEX.
		 §7tricies.69.

		 At the exit's own date every slot the walk reached BEFORE the
		 exit has executed -- the walk order there is (iteration,
		 insn), and the intra-cycle order moves jumps last within
		 their group, so no non-jump at that date in that node follows
		 the branch.  Comparing `i <= ie' alone keeps an instance of
		 an EARLIER iteration whose insn index happens to be higher,
		 and the drain then re-runs it.

		 Re-running is harmless only if the instance is idempotent,
		 and it need not be: its INPUTS may have been destroyed by the
		 branch's own bundle.  On
		 `if (A[i] == stop) break; s += A[i] * 3 + B[i];' the kernel
		 emits

		     addd $r2 = $r9, $r8      <- the term, iteration 0
		     muld $r9 = $r10, 3       <- iteration 1, clobbers r9
		     ccb.deq $r10, $r1? .L11

		 all in one bundle, which is correct because a VLIW reads
		 every operand before any write lands.  The drain then re-ran
		 the first of those with r9 already holding iteration 1's
		 product, so the term came out A[1]*3 + B[0] instead of
		 A[0]*3 + B[0] and the sum was high by one term -- 780 of
		 1722 points of validation/cms/dataexit.c's sweep, every one
		 of them a data-exit path that had accumulated at least one
		 iteration.

		 So the test is lexicographic on (iteration, insn) against the
		 exit's, which is the walk's own order.  */
	      if (d < de
		  || (d == de
		      && (l < le || (l == le && (int) i <= ie))))
		continue;
	      /* THE EXITING ITERATION STOPS AT ITS EXIT.  §7tricies.46.

		 The iteration bound above (l <= le) already keeps the drain
		 from running iterations the exit discarded.  What it does not
		 do is stop iteration LE itself: an instance of LE scheduled
		 after the exit's date was included, and in program order it
		 comes AFTER the exit, so the original loop never ran it.

		 That is invisible for a BOTTOM test, where the exit is the
		 last instruction of the body and `i > ie' is impossible --
		 which is why every single-exit loop was right.  For a TOP
		 test it is the whole difference: on validation/cms/twoexit.c
		 the exit-A drain ran `acc += A[i]' and `i++' for the very
		 iteration whose test had just broken out, so both the sum and
		 the returned index were wrong.  Instances of LE BEFORE the
		 exit stay: the original loop did run them, and the schedule
		 may have placed them after the exit's date.  */
	      if (l == le && (int) i > ie)
		continue;
	      /* NO JUMP IN AN EPILOGUE, whichever jump it is.  §7tricies.45.

		 A drain is entered because some exit fired at iteration L.
		 Every iteration below L completed in the original program, so
		 none of ITS jumps would have exited; every iteration above L
		 must not run at all, so none of its jumps may exit.  Either
		 way a jump instance reaching the drain would not branch, and
		 dropping it is what emits the drain as the straight line it
		 is.  The epilogue's own redirect carries it to the right
		 destination.

		 This was `i == loopback', which is the same statement for a
		 single-jump body and leaves a second jump to be emitted as an
		 exit the drain has no epilogue for.  */
	      if (JUMP_P (seq[i]))
		continue;
	      if (is_main)
		pl->epilogue++;
	      else
		pl->exit_slots++;
	      if (slots)
		{
		  lvx_lms_slot sl;
		  sl.insn = (int) i;
		  sl.iter = l;
		  sl.date = d;
		  sl.node = node;
		  slots->safe_push (sl);
		}
	    }
    }

  /* STEADYBAD: instructions not appearing exactly KUNROLL times in the
     kernel.  It found a real defect -- a window eight cycles long where the
     period was ten held two of four instructions and one of the fifth, and
     the loop advanced its pointer twice per accumulate -- and it is NOT a
     correctness criterion, which took the executable gate to establish.

     IT IS A CORRECTNESS CRITERION AFTER ALL, and the argument that said
     otherwise is worth keeping because it is wrong in an instructive way.  It
     ran: a kernel closed at ITER_SLOT is asymmetric whenever the loopback is
     not the last slot of the window, the instances after the branch belong to
     the epilogue by construction, and "the deficit is absorbed across trips by
     the prologue and the epilogue".

     THE PROLOGUE AND THE EPILOGUE COVER THE BOUNDARY, NOT THE MIDDLE.  A
     kernel block short of one instance of one instruction is short of it on
     EVERY trip: the steady state executes kunroll iterations' work minus that
     instance, and nothing ever supplies it.  The entry and the drain fix up
     the first and last trips, which is why a short trip count still agrees
     and why `main' loop 1 at STEADYBAD = 2 looked like a counterexample.

     Found by the executable gate on `A[i] = (i % 7) + 1', the signed-modulo
     fixup: PLKER = 61 where the body is 16 instructions and KUNROLL is 4, so
     three instances missing, and the emitted kernel has
     `sraw $rX = $rIV, 31' in three of its four copies.  Every fourth
     iteration computed its remainder without the negative-value correction.
     B[i] = 64 - i, which needs no fixup, passes -- which is why the fault
     looked like it was about stores for as long as it did.

     So PLKER must be KUNROLL * |body| exactly, and this counts the shortfall.
     The construction that produces it -- walking the master schedule and
     cutting at ITER_SLOT, rather than folding each instruction's KUNROLL
     instances into the block by sigma mod (KUNROLL * II) -- is the thing to
     fix; until then the emission refuses a loop with STEADYBAD != 0.  */
  {
    auto_vec<int> per;
    per.safe_grow_cleared (seq.length ());
    if (slots)
      for (unsigned q = 0; q < slots->length (); q++)
	{
	  int nd = (*slots)[q].node;
	  if (nodes_out && nd >= 0 && nd < (int) nodes_out->length ()
	      && (*nodes_out)[nd].kind == LVX_LMS_NODE_KERNEL
	      && (*slots)[q].insn >= 0
	      && (*slots)[q].insn < (int) seq.length ())
	    per[(*slots)[q].insn]++;
	}
    for (unsigned i = 0; i < seq.length (); i++)
      if (slots && per[i] != kunroll)
	pl->steady_bad++;
    if (slots && pl->steady_bad && lvx_lms_probe && dump_file)
      {
	lvx_lms_dump (";; lvx cms:   steadybad kunroll=%d per-insn:", kunroll);
	for (unsigned i = 0; i < seq.length (); i++)
	  lvx_lms_dump (" %u:%d%s", i, per[i],
			per[i] != kunroll ? "*" : "");
	lvx_lms_dump ("\n");
      }
  }

  pl->ok = true;
  pl->why = NULL;
}

/* THE REGISTERS THE PIPELINE MUST NOT TOUCH.

   Everything the body reads or writes, minus the values the renaming will
   rewrite every reference to.  That subtraction is computable BEFORE the
   colouring, which is what breaks the apparent circularity -- AVAIL is an
   input to the colouring, the colouring decides which values get expansion
   names, and the names decide what is freed.  But a value gets a name exactly
   when its adjusted life is positive, and that is a function of the schedule
   alone.  A USE_FIRST value is not freed: its name 0 IS the original register
   and has to stay (§7ter.4).

   Without this the colouring is handed AVAIL = all ones and hands back
   registers the loop is already using.  Measured on the gate's own program
   before it existed: every one of the four loops had anodes landing on held
   registers, and on one of them the assignment put a value's four expansion
   copies in r1, r3, r2, r0 while r2 and r3 held two zero-life values.  The
   schedule was right, the colouring was internally consistent, and the code
   would have been wrong -- which is the shape of defect §7ter.10's note about
   the colouring being OPTIMISTIC predicted and did not quantify.  */

static void
lvx_lms_reserved_regs (const auto_vec<rtx_insn *> &seq,
		       const auto_vec<int> &def_, const auto_vec<int> &last_,
		       const auto_vec<int> &nregs_, const auto_vec<int> &ec_,
		       const auto_vec<int> &vregno,
		       const auto_vec<int> &vusefirst,
		       bitmap livein, HARD_REG_SET *reserved)
{
  /* A register is FREEABLE only when EVERY definition of it in the body
     belongs to a value the renaming rewrites.  Clearing it per value is not
     enough, and the difference is not hypothetical: RA reuses a register for
     two values in one loop, and on the gate's own program r3 held both a
     renamed value and a zero-life one, so freeing it for the first destroyed
     the second.  Counting definitions per register is what separates them.

     Reads need no separate treatment while AMBIG is zero, which is gated:
     with each read of a register belonging to exactly one producing value,
     a read of a freeable register is a consumer of the value that defines it
     and is rewritten with it.  A register with no in-body definition is a
     live-in and is never freeable, which the ndefs > 0 test gives.  */
  int ndefs[FIRST_PSEUDO_REGISTER], nren[FIRST_PSEUDO_REGISTER];

  CLEAR_HARD_REG_SET (*reserved);
  memset (ndefs, 0, sizeof (ndefs));
  memset (nren, 0, sizeof (nren));

  /* THE BODY'S OWN REFERENCES ARE NOT THE WHOLE HELD SET.  A register can be
     live ACROSS the loop and never mentioned inside it -- the stack pointer
     is the obvious one, and the return address, and any callee-saved register
     the function is using elsewhere.  None appears in SEQ, so all of them
     looked free, and the colouring handed one out.

     It handed out r12.  On validation/cms/reduce.c the rename map read
     `0@0:r0x1->r12' -- the STACK POINTER as an expansion register -- and the
     ISS died on `writeBlob(0xffffffffffffff90)', a store to -112.  The same
     cause explains the fetch from 0x11 on a larger program (a clobbered
     return address) and the hangs: four diagnoses of "the control-flow
     defect" and it was never control flow at all, it was the allocatable set.

     FIXED_REG_SET covers the architectural ones; the loop's LIVE-IN covers
     everything the function is using around it.  Values the renaming rewrites
     are subtracted below, so a live-in that IS a renamed value -- the
     induction variable, the accumulator -- is still freed.  */
  /* The reservations nothing in the loop can release, kept aside so the
     body-derived subtraction below cannot clear them.  */
  HARD_REG_SET hard;

  *reserved = fixed_reg_set;

  /* A CALLEE-SAVED REGISTER THE FUNCTION HAS NOT ALREADY SAVED IS NOT
     AVAILABLE.  The prologue's save set was decided by
     pass_thread_prologue_and_epilogue, which runs BEFORE machine reorg, so
     nothing will go back and save a register this pass decides to use.

     On store.c's `init' the colouring took r17 through r21 -- callee-saved,
     in a function with no stack frame at all, saving nothing -- and
     destroyed the caller's values.  The ISS died on
     writeBlob(0x41efffffffc20700), which is a double's bit pattern used as
     an address: main's r18 held a pointer, init returned it holding a float.

     So a register is usable only if the function already has it live --
     df_regs_ever_live_p, which is what drove the prologue -- or if the ABI
     lets a call clobber it, which makes it free inside a loop that contains
     no call (and eligibility rejects a loop that holds one).  */
  for (int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
    if (!df_regs_ever_live_p (r) && !crtl->abi->clobbers_full_reg_p (r))
      SET_HARD_REG_BIT (*reserved, r);

  if (livein)
    {
      bitmap_iterator bi;
      unsigned r;
      EXECUTE_IF_SET_IN_BITMAP (livein, 0, r, bi)
	if (r < FIRST_PSEUDO_REGISTER)
	  SET_HARD_REG_BIT (*reserved, r);
    }
  hard = *reserved;

  for (unsigned i = 0; i < seq.length (); i++)
    {
      df_ref r;
      FOR_EACH_INSN_DEF (r, seq[i])
	if (DF_REF_REGNO (r) < FIRST_PSEUDO_REGISTER)
	  {
	    SET_HARD_REG_BIT (*reserved, DF_REF_REGNO (r));
	    ndefs[DF_REF_REGNO (r)]++;
	  }
      FOR_EACH_INSN_USE (r, seq[i])
	if (DF_REF_REGNO (r) < FIRST_PSEUDO_REGISTER)
	  SET_HARD_REG_BIT (*reserved, DF_REF_REGNO (r));
    }

  for (unsigned v = 0; v < def_.length (); v++)
    {
      /* Same convention as lvx_lms_colour: [def + !earlyclobber, last].  A
	 check that models the schedule differently from the allocator tests a
	 different program.  */
      int life = last_[v] - def_[v] + (ec_[v] ? 1 : 0);
      int reg = v < vregno.length () ? vregno[v] : -1;
      bool uf = v < vusefirst.length () && vusefirst[v];
      if (life <= 0 || reg < 0 || uf)
	continue;
      for (int w = 0; w < MAX (nregs_[v], 1); w++)
	if (reg + w < FIRST_PSEUDO_REGISTER)
	  nren[reg + w]++;
    }

  /* THE SUBTRACTION MUST NOT FREE A HARD RESERVATION.  It ran last and
     cleared any register whose definitions all belong to renamed values --
     overriding the fixed, live-in, live-out and not-already-saved bits set
     above, because a register can satisfy both tests at once.

     It freed r3 on store.c's `init'.  r3 held a renamed value AND was the
     array base of a store, the base being a loop-invariant defined in the
     preheader and therefore not in the value table at all.  Freed, r3 was
     handed to the value, the substitution rewrote EVERY r3 in the pattern
     including the base, and the emitted store read
     `sd $r32[$r6] = $r32' -- the same register as base and as data, with a
     double's bit pattern used as an address.  The ISS died on
     writeBlob(0x41efffffffc20380).

     So the hard set is kept aside and ORed back after the freeing.  The
     body-derived bits remain freeable, which is the whole point of the
     subtraction; what is not freeable is what something outside the loop
     relies on.  */
  for (int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
    if (ndefs[r] > 0 && ndefs[r] == nren[r])
      CLEAR_HARD_REG_BIT (*reserved, r);

  *reserved |= hard;
}

/* LIVE RANGES WITH EFFECTS, the rename unit.  §7tricies.25.

   The renaming was built over VALUES -- one per instruction defining a
   register something reads -- then over tie CLASSES, merging only what the
   target forbids separating.  That is a strictly smaller merge than a live
   range, and every gate §7tricies.20-24 had to add is a consequence:
   two classes could share a hard register (72 loops declined), a class could
   be live out with its reader unrewritten (25), a pin forced degree 1 (27),
   and a tied operand had no class to merge with (4).

   kvx-unfold.cc does not have those problems because its unit is the live
   range and each range enumerates its own references:

       struct unfold_range_ { ... unfold_effect_ *effects; ... };
       int index = (iteration + use_first - delayed) & (p2unroll - 1);
       return rename->ranges[index];

   A RANGE IS ONE DEFINITION OF ONE HARD REGISTER plus every reference to it.
   Two values in one register are two ranges, and they are told apart not by
   the register -- which is what my version tried and could not -- but by
   WHICH REFERENCE is being rewritten: the def lookup matches a range whose
   first effect is this instruction, a use lookup searches past the first
   effect.  An instruction cannot read one register for two ranges, because
   two reads of one register in one instruction read the same value.

   THE CYCLIC WRAP IS A NORMALISATION, not a special case.  If a register's
   first range begins with a definition and its last begins with uses -- uses
   that precede the first def in body order, so they read the previous
   iteration -- the last range's effects move into the first, flagged DELAYED,
   and the range is marked USE_FIRST.  The index arithmetic then carries both:
   `+ use_first' shifts the stack so the incoming value sits at index 0 and
   the first definition at index 1, and `- delayed' sends a wrapped reference
   back one instance.  No pinned name 0, so no forced degree 1, so none of
   §7tricies.19's unsoundness.

   This stage BUILDS and CHECKS the structure and renames nothing.  */

/* AN EFFECT IS ONE READ OR WRITE OF ONE OPERAND, not an instruction.

   It was (insn, is_def), which identifies a reference only up to the
   instruction, and that is not enough in two places.  `x * x' reads one
   register through two operands, which the structural check had to count
   separately as TWICE because the lookup could not tell them apart; and
   `fixedd.rz r0 = r0' reads two different live ranges that RA put in one
   register, where answering with either is wrong half the time.

   SLOT is the operand: the ordinal of the reference's location in a
   deterministic sub-rtx walk of the pattern, which is exactly what
   lvx_lms_reg_index computes from DF_REF_LOC.  It is an index rather than the
   location because the rewrite runs on a COPY, where a pointer into the
   original addresses nothing and the traversal order is identical.  Keyed on
   (insn, slot) the lookup needs no is_def argument and cannot be ambiguous --
   one operand belongs to one live range.  */

/* AN INDIRECT USE HAS NO OPERAND, so it has no slot.  A branch reads, by
   transferring control to a block where the register is live, every register
   live-in on its exit edge -- a use that is entirely real while the OPERAND is
   not there to be found, which is why it cannot be keyed like the others.

   The term is the KVX work's: `indirect' says the use is mediated by the CFG
   edge rather than by the instruction's operands.  Not `pseudo', which in a
   GCC back end means a pseudo-REGISTER -- FIRST_PSEUDO_REGISTER is all over
   this file -- and these are hard registers after allocation, so the word
   would say the opposite of what is meant.

   The value is -2 and not -1 so that it cannot be confused with
   lvx_lms_reg_index's "not in this pattern", and it is negative so that every
   lookup keyed on a real operand position passes it by: lvx_lms_lrange_of
   returns null for a negative slot, and the renaming walks the pattern's REGs
   and so never visits an effect that has none.  */
#define LVX_LMS_SLOT_INDIRECT (-2)

/* AN EFFECT IS ON A SUB-OPERAND, so it carries its OFFSET within the
   live range's span as well as its width.  §7tricies.71.

   A live range is the renaming unit for a whole tuple, and references to it need
   not all be the same width: on PolyBench a quad is loaded whole and read as
   two pairs,

       insn 1 slot 0  defines  r32 r33 r34 r35
       insn 8 slot 1  reads    r32 r33            offset 0, nregs 2
       insn 9 slot 1  reads    r34 r35            offset 2, nregs 2

   which is the out-tree of effects on sub-operands: the root is the widest
   span and every reference hangs on the node it covers.  For LVX that tree is
   at most quad -> two pairs -> four singles, because the ISA has only ALIGNED
   sub-tuples -- singleReg, pairedReg, quadReg, the parity classes having been
   retired -- so the spans nest perfectly and every reference lands on exactly
   one node.  The offset is what the renaming adds to the live range's name:
   reg[index] + offset, at the reference's own width, which is the
   "sub-register offset the renaming does not carry" that the old mixed-width
   fallback named as its reason for giving up.  */

struct lvx_lms_effect
{
  unsigned insn;		/* index into SEQ */
  int slot;			/* the operand: ordinal in the REG walk */
  int nregs;			/* THIS reference's width in registers */
  bool is_def;			/* a write (it may also head the range) */
  bool delayed;			/* reads the previous instance */
  int offset;			/* registers from the live range's REGNO */
};

#define LVX_LMS_MAX_KUNROLL 32

/* A PART OF A LIVE RANGE, NOT A LIVE RANGE.  §7tricies.98.

   The renaming unit -- the thing that gets P2UNROLL names, one occupancy and
   one colour -- is a UNION of these, identified by its root through
   lvx_lms_lrange_root.  One object is a member of it, and only a root object
   stands for the whole; the colouring says so where it skips the others
   ("ONE UNIT PER LIVE RANGE"), and so does the verifier ("one live range, one
   occupancy").  The type was called `lvx_lms_lrange' and so read as the whole
   while denoting a part, which is the same imprecision the lineage/live-range
   rename removed from the prose.

   Most live ranges are a single part: measured over 194 loops of TSVC2 and
   PolyBench, 1527 units from 92 unions, with a union occurring in 63 loops.
   That is exactly why the distinction is easy to forget -- the compensating
   call to lvx_lms_lrange_root is correct in all but a sixteenth of cases even
   if omitted.

   The functions keep the `lvx_lms_lrange_' prefix as a module name: ..._root
   maps a part to its live range, ..._union joins two, and the rest take a
   part and are written to be called on the root where the answer is the live
   range's.  */

struct lvx_lms_lrange_part
{
  unsigned regno;
  int nregs;
  bool use_first;
  bool exit_live;
  /* THE OCCUPANCY END, SEPARATE FROM LAST_DATE.  §7tricies.137.
     LAST_DATE is the last reference INSIDE the body, and it is what the
     DEGREE must be computed from: that is how long the value really lives,
     so that is how many names its copies need.  OCC_LAST is how long its
     REGISTER must be reserved, which for a live-out range runs to the end of
     the schedule because the drain's replayed work must not overwrite the
     register the reconciliation copy will read (§7tricies.135).  The two were
     one field, and extending it moved the degree as a side effect -- forcing
     modulo expansion on ranges that then failed `recog', 17 pipelines lost
     for a requirement that was only ever about not SHARING.  */
  int occ_last;
  bool renamed;			/* its union gets a stack of names */
  int first_date, last_date;
  int p2unroll;
  bool ec;			/* the early clobber the COLOURING used, stored so the
			   verifier cannot recompute a different one.  §7tricies.90 */
  bool capped;			/* p2unroll was forced to 1 by LVX_LMS_MAX_KUNROLL,
			   not derived from LIFE.  §7tricies.92 */
  bool noname;			/* this PART cannot take a stack of names, so its
			   union cannot either.  Sticky because `renamed' is
			   RESET optimistically once the unions are final:
			   a guard that fires before that point has to leave
			   its answer somewhere the reset does not touch.
			   §7tricies.118 */
  int self;			/* own index into LR */
  int parent;			/* union-find over LR: the live range */
  int reg[LVX_LMS_MAX_KUNROLL];
  auto_vec<lvx_lms_effect> effects;
};

/* THE EARLY CLOBBER OF A RANGE, NOT OF AN INSTRUCTION.  §7tricies.90.

   `ec' decides whether the range's register is busy from its definition's
   issue cycle or from the cycle after, so it lengthens LIFE by one and moves
   the occupancy start back by one.  It was read in three places from
   `effects[0]' with three different formulas, and the one that SET p2unroll
   omitted it entirely -- harmless only because the colouring overwrites that
   value with its own ec-aware degree, which is the kind of agreement that
   holds by accident.

   The property belongs to one instruction; the decision belongs to the range,
   which spans many.  So it is computed once, here, and STORED on the range:

     0  what the three sites did -- `effects[0]', whatever it is
     1  conservative -- assume it for every range, safe for every member
     2  exact -- the first DEF of the range, which is the instruction that
	actually governs when the register becomes busy

   Policy 2 is correct only because a range whose `effects[0]' is not a
   definition is marked `renamed = false' and never reaches an allocation
   decision; were that to change, the loop below still finds the first real
   definition rather than trusting position zero.  */

static bool
lvx_lms_lrange_ec_p (const auto_vec<rtx_insn *> &seq, const lvx_lms_lrange_part *g)
{
  if (g->effects.is_empty ())
    return false;
  /* THE RANGE'S OWN FIRST DEFINITION, and nothing else.  A range whose first
     def is not an early clobber is NOT affected by one elsewhere in the loop:
     the flag decides when THIS register becomes busy.  The alternative --
     assuming it for every range -- was measured and is expensive, costing 6
     of 111 built pipelines and 2 of 16 deep ones on TSVC2 with the sum of IIs
     unchanged, so the loss was entirely ranges pushed to a larger P2UNROLL by
     one needless cycle of occupancy.  §7tricies.96.  */
  for (unsigned e = 0; e < g->effects.length (); e++)
    if (g->effects[e].is_def)
      return (g->effects[e].insn < seq.length ()
	      && lvx_insn_dest_earlyclobber_p (seq[g->effects[e].insn]));
  /* No definition in the body: the rotation of §7tricies.91 leaves these
     unrenameable, and their occupancy does not depend on the flag.  */
  return false;
}

/* The representative of a range's tie union, with path compression.  */

static int
lvx_lms_lrange_root (const auto_vec<lvx_lms_lrange_part *> &lr, int k)
{
  while (lr[k]->parent != k)
    {
      lr[k]->parent = lr[lr[k]->parent]->parent;
      k = lr[k]->parent;
    }
  return k;
}

/* Put two ranges in one live range.  Returns true if they were not already.

   THE ROOT IS CHOSEN BY PROGRAM ORDER, NOT BY ARGUMENT ORDER.  §7tricies.119.

   Union-find carries no order, and the parts of a live range are ordered:
   LR is appended to as the walk meets each opening reference, so a part's
   index IS its position in program order.  Most consumers do not care,
   because what they read off the root is a commutative merge -- FIRST_DATE is
   a MIN, LAST_DATE a MAX, EXIT_LIVE and USE_FIRST an OR, NREGS a MAX.  Three
   do care, and they read `effects[0]' off the root as *the definition that
   heads this live range*: the colouring's VNOTOWN exclusion, the live-out
   split verification, and the live-out copy placement, which takes the
   instruction to copy after from `q->effects[0].insn'.

   For those three the root must be the LAST part of the union, because the
   value that leaves the iteration is the one the last definition produced.
   It already was -- but only because both call sites happen to pass the newer
   part as GA and this function happened to make GA's root the root.  Nothing
   said so and nothing checked it, and a third call site passing its arguments
   the other way would have moved the live-out copy with no diagnostic.

   So the rule is stated here instead: the root is the member with the
   greatest index, which is the latest in program order.  The two existing
   call sites are unaffected, which is the point -- an invariant that was
   holding by accident now holds by construction.  */

static bool
lvx_lms_lrange_union (const auto_vec<lvx_lms_lrange_part *> &lr,
		      lvx_lms_lrange_part *ga, lvx_lms_lrange_part *gb)
{
  int a, b;

  if (!ga || !gb || ga == gb)
    return false;
  a = lvx_lms_lrange_root (lr, ga->self);
  b = lvx_lms_lrange_root (lr, gb->self);
  if (a == b)
    return false;
  if (a < b)
    std::swap (a, b);
  lr[b]->parent = a;
  return true;
}

/* Build the live ranges of SEQ.  One per definition of a hard register, in
   body order, with every reference to that register attached to the range
   the reaching definition opened.  */

/* IS THE OPERAND AT USEIDX TIED TO A DESTINATION?  §7tricies.106.

   Per operand, not per instruction.  lvx_insn_has_tied_operand_p answers
   "does this insn carry any tie", which for copy propagation rejects far too
   much: an instruction may have a tied accumulator and separately read an
   untied value through a copy, and only the first is a reason to refuse.

   USEIDX indexes the arc builder's sub-rtx walk, not recog_data.operand[], so
   the two are joined by POINTER IDENTITY: lvx_lms_reg_at gives the location
   inside the pattern, and recog_data.operand_loc[] holds those same
   locations.  Matching on register number instead is what §7tricies's arc
   note already records as not being a key -- one instruction can mention one
   regno for two different values.

   Answers TRUE when it cannot tell, which is the direction that refuses the
   transformation rather than performing it unsoundly.  */

static bool
lvx_lms_operand_tied_p (rtx_insn *insn, int useidx)
{
  const operand_alternative *alt;
  rtx *loc;

  if (useidx < 0)
    return true;
  loc = lvx_lms_reg_at (&PATTERN (insn), useidx);
  if (loc == NULL)
    return true;
  if (recog_memoized (insn) < 0)
    return true;
  extract_insn (insn);
  /* PREPROCESS_CONSTRAINTS, OR WHICH_OP_ALT READS ANOTHER INSN'S TABLE.
     §7tricies.150.

     `recog_op_alt' is filled per insn CODE by preprocess_constraints, and
     which_op_alt indexes it as `which_alternative * recog_data.n_operands'.
     extract_insn and constrain_operands fill `recog_data' and choose the
     alternative but leave that table alone -- recog.h says so beside the
     declaration: "Only valid after calling preprocess_constraints and
     constrain_operands."

     Without this call the predicate reads whatever insn was preprocessed
     last, with this insn's operand count and alternative index.  It did not
     crash while the only callers ran late, after a pass that had preprocessed
     the same insns; called from lvx_lms_build_lranges it segfaults on an
     insn whose table is shorter.  The silent half is worse than the crash:
     every answer this predicate has given came off whichever table happened
     to be loaded.  */
  preprocess_constraints (insn);
  if (!constrain_operands (1, get_enabled_alternatives (insn)))
    return true;
  if (which_alternative < 0)
    return true;
  if (which_alternative >= recog_data.n_alternatives)
    return true;
  alt = which_op_alt ();
  if (alt == NULL)
    return true;
  for (int i = 0; i < recog_data.n_operands; i++)
    if (recog_data.operand_loc[i] == loc)
      return alt[i].matches >= 0 || alt[i].matched >= 0;
  /* The arc names a register the constrained operand list does not: a
     hard-coded operand or one inside a sub-expression.  Not rewritable.  */
  return true;
}

/* IS THIS LIVE-OUT ARC'S RANGE ONE THE COLOURING CANNOT FREE?  §7tricies.114.

   The drop promises the body writes a name of its own.  A value carried across
   the back edge cannot promise it: name 0 comes from the preheader, which wrote
   the architected register, so the pin holds that register and the body does
   write it.  Ranges do not exist when the arcs are built, so the test is on the
   instruction -- does it define a register live on entry to the loop header.  */

/* WILL THE DRAIN RECONCILE WHAT THIS ARC PROTECTS?  §7tricies.154.

   The drop and the reconciliation were decided from two different sets, and
   that is the miscompile.  The drop asked only whether the defining
   instruction writes a register live-in at the loop HEADER -- a value carried
   across the back edge, which is pinned and keeps its arc.  Everything else
   was dropped, on the expectation that the drain copies it back.

   But the drain's copy loop and lvx_lms_verify_liveout_split both iterate
   EXIT_LIVE ranges that are def-headed and not use-headed, which is a
   strictly smaller set.  A definition outside it loses its arc and gains no
   copy: the scheduler is then free to hoist the next iteration's write of a
   live-out register above the exit branch, and the exit returns a value one
   iteration too far.  That is exactly the unsoundness
   -mlms-drop-liveout-arcs is documented to have and is refused with codegen
   for -- reached here by a drop wider than its own promise.

   Measured on TSVC2's initialise_arrays: the loop dropped FIVE arcs with ONE
   exit-live range, and the reconciliation covered that one.  The remaining
   four produced a wild store, because the register the exit read held the
   next iteration's packed float data.

   So the predicate becomes the drain's own: an arc is droppable only where
   the range its definition HEADS is one the drain will copy back.  RENAMED
   cannot be asked here -- the colouring has not run -- which is precisely
   what lvx_lms_verify_liveout_split checks afterwards, so the pair is now
   bound, transform, re-bound over ONE set instead of two.  */

static bool
lvx_lms_liveout_arc_reconciled_p (const auto_vec<lvx_lms_lrange_part *> &lr,
				  unsigned q)
{
  bool found = false;

  for (unsigned k = 0; k < lr.length (); k++)
    {
      lvx_lms_lrange_part *g = lr[k];

      if (g->effects.is_empty ()
	  || lvx_lms_lrange_root (lr, (int) k) != (int) k
	  || g->effects[0].insn != q)
	continue;
      /* A range this instruction heads but the drain will not copy: the arc
	 is real and must stay.  */
      if (!g->exit_live || !g->effects[0].is_def)
	return false;
      /* §7tricies.155: the PIN is the only thing keeping this arc, so it is
	 the one a preheader copy would remove.  The lever says what that
	 would be worth; without the copy the pin is real.  */
      if (g->use_first && !lvx_lms_usefirst_unpinned
	  && !(lvx_lms_preheader_copy && (lvx_lms_ph_parts & 4)))
	return false;
      found = true;
    }
  return found;
}

static bool
lvx_lms_liveout_arc_pinned_p (class loop *loop,
			      const auto_vec<rtx_insn *> &seq, unsigned q)
{
  df_ref def;

  if (q >= seq.length ())
    return true;
  FOR_EACH_INSN_DEF (def, seq[q])
    if (DF_REF_REGNO (def) < FIRST_PSEUDO_REGISTER
	&& bitmap_bit_p (df_get_live_in (loop->header), DF_REF_REGNO (def)))
      return true;
  return false;
}

/* DID THE SPLIT ACTUALLY HAPPEN?  §7tricies.112.

   -mlms-liveout-split drops the CTRL_LIVEOUT arcs on the expectation that the
   renaming writes a name of its own and the drain reconciles it.  That is
   decided after scheduling, so it is a commitment to be checked, not a fact to
   be assumed: a range that came out holding its architected register has a
   body instruction writing what the exit edge reads, with the arc that said so
   removed.  The loop is refused rather than emitted.

   Checked over the whole set rather than a recorded subset, because the arcs
   were dropped for all of them: every exit-live range with a body definition
   must be renamed, and no name of it may be its own regno.  */

static bool
lvx_lms_verify_liveout_split (const auto_vec<lvx_lms_lrange_part *> &lr,
			      int *nbad_out)
{
  int nbad = 0;

  *nbad_out = 0;
  for (unsigned k = 0; k < lr.length (); k++)
    {
      lvx_lms_lrange_part *g = lr[k];

      if (g->effects.is_empty () || !g->exit_live
	  || lvx_lms_lrange_root (lr, (int) k) != (int) k
	  || !g->effects[0].is_def)
	continue;
      /* THE CHECK MUST MATCH THE DROP.  §7tricies.113.  A USE_FIRST range
	 keeps its arc -- its name 0 is pinned to the register the preheader
	 wrote, so it does write the architected register in the body -- and
	 demanding the promise of it anyway failed 31 of the 32 loops this
	 verifier refused, on ranges whose arcs had correctly been kept.  A
	 verifier stricter than the transformation it guards reports the
	 transformation's successes as failures.  */
      /* §7tricies.156: with the preheader copy a use-headed range is
	 renamed like any other and the drop admits it, so it must be IN the
	 set this verifier checks -- the whole lesson of §7tricies.154.  */
      if (g->use_first
	  && !(lvx_lms_preheader_copy && (lvx_lms_ph_parts & 8)))
	continue;
      if (!g->renamed)
	{
	  nbad++;
	  continue;
	}
      for (int j = 0; j < g->p2unroll && j < LVX_LMS_MAX_KUNROLL; j++)
	if (g->reg[j] == (int) g->regno)
	  {
	    nbad++;
	    break;
	  }
    }
  *nbad_out = nbad;
  return nbad == 0;
}

/* THE PROPAGATION RECORD, WITH THE PRODUCER PINNED.  §7tricies.110.

   One entry per operand that is to be taken as reading a copy's source
   instead of its destination.  Keyed by the insn's index in SEQ and the slot
   the arc builder numbers operands with -- never by register number, since
   one instruction can mention one regno for two different values, which is
   why the arc carries USEIDX at all.

   PRODUCER IS IN THE RECORD, and that is the whole lesson of the first
   attempt.  Naming only the register leaves the arc builder to resolve the
   reaching definition itself, by searching backward for the last write of
   that register before the reader.  Post-RA that search finds the wrong
   answer: a hard register carries several values -- `unroll' has three live
   ranges on r3 -- and for a LOOP-CARRIED read, where the reader precedes the
   copy in program order, the nearest earlier definition is generally not the
   one that fed the copy at all.  The graph would then be built against a
   different value, silently.

   DELAYED says the operand reads the PREVIOUS instance.  The reader took the
   copy's destination as the copy wrote it an iteration earlier -- a
   use-headed reference the cyclic normalisation folds in with this flag set
   -- and the source's range is def-headed, so injecting the use plainly
   would read the current instance: right register, wrong iteration.  The
   flag is a bool and the index computation subtracts exactly one, so a reader
   two or more iterations behind has no representation and is refused rather
   than approximated.  */

struct lvx_lms_prop
{
  unsigned insn;		/* the reader, as an index into SEQ */
  int slot;			/* which operand of it */
  unsigned regno;		/* the register it is to be taken to read */
  unsigned producer;		/* and WHICH definition of it: an index into SEQ */
  HOST_WIDE_INT omega;		/* the distance from that definition */
  bool delayed;			/* it reads the previous instance */
};

/* The entry for SLOT of insn I, or NULL if that operand was not propagated.  */

static const lvx_lms_prop *
lvx_lms_prop_lookup (const auto_vec<lvx_lms_prop> *prop, unsigned i, int slot)
{
  if (prop == NULL || slot < 0)
    return NULL;
  for (unsigned k = 0; k < prop->length (); k++)
    if ((*prop)[k].insn == i && (*prop)[k].slot == slot)
      return &(*prop)[k];
  return NULL;
}

/* THE EXIT LIVENESS OF EACH BRANCH, CAPTURED BEFORE ANY MUTATION.
   §7tricies.104.

   For every jump in SEQ, the registers live on its out-of-loop successors.
   lvx_lms_build_lranges needs this to decide which ranges are live out, and
   computed it from `jbb->succs' at the point of use -- which is correct on
   the probe path and a NULL dereference on the codegen one: by then
   lvx_lms_make_nodes has created blocks and redirected edges, so a successor
   can be a block DF has never seen and `df_get_live_in' returns nothing at
   all.  Compiling libgcc with -mlms-codegen segfaulted here.

   A null check would have been the wrong repair: a branch whose exit liveness
   silently read as empty leaves every range it should have marked `exit_live'
   unmarked, and the renaming then rewrites a value the exit path reads --
   a miscompile in place of a crash.  The liveness wanted is the ORIGINAL
   loop's, so it is taken while that is still what the CFG holds and threaded
   in.

   A HARD_REG_SET rather than a bitmap because the consumer only ever asks
   about hard registers, and because a value type needs no freeing on the
   paths that bail out early.  */

static void
lvx_lms_capture_exit_live (const auto_vec<rtx_insn *> &seq,
			   auto_vec<HARD_REG_SET> &xl)
{
  xl.safe_grow_cleared (seq.length ());
  for (unsigned i = 0; i < seq.length (); i++)
    {
      basic_block jbb;
      edge e;
      edge_iterator ei;

      CLEAR_HARD_REG_SET (xl[i]);
      if (!JUMP_P (seq[i]) || (jbb = BLOCK_FOR_INSN (seq[i])) == NULL)
	continue;
      FOR_EACH_EDGE (e, ei, jbb->succs)
	{
	  bitmap lin;
	  bool inbody = false;

	  for (unsigned z = 0; z < seq.length (); z++)
	    if (BLOCK_FOR_INSN (seq[z]) == e->dest)
	      {
		inbody = true;
		break;
	      }
	  if (inbody || (lin = df_get_live_in (e->dest)) == NULL)
	    continue;
	  for (int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
	    if (bitmap_bit_p (lin, r))
	      SET_HARD_REG_BIT (xl[i], r);
	}
    }
}

/* Declared ahead of their definitions: the three operand lookups, which the
   build's unification passes need and which read the structure the build has
   just produced.  */
static lvx_lms_lrange_part *lvx_lms_lrange_of (const auto_vec<lvx_lms_lrange_part *> &,
					  unsigned, int, bool *,
					  int * = NULL, int * = NULL);
static lvx_lms_lrange_part *
lvx_lms_lrange_def_of (const auto_vec<lvx_lms_lrange_part *> &, unsigned, unsigned);
static lvx_lms_lrange_part *
lvx_lms_lrange_use_of (const auto_vec<lvx_lms_lrange_part *> &, unsigned, unsigned);

/* A LOOP-INVARIANT RANGE: every reference is a read, so the body never writes
   it and its value comes from outside the loop.  §7tricies.167.  */

static bool
lvx_lms_lrange_invariant_p (const auto_vec<lvx_lms_lrange_part *> &lr,
			    int root)
{
  bool any = false;

  for (unsigned k = 0; k < lr.length (); k++)
    {
      if (lr[k]->effects.is_empty ()
	  || lvx_lms_lrange_root (lr, (int) k) != root)
	continue;
      for (unsigned e = 0; e < lr[k]->effects.length (); e++)
	{
	  if (lr[k]->effects[e].is_def)
	    return false;
	  any = true;
	}
    }
  if (!any)
    return false;

  /* AND NO WIDE DEFINITION MAY REACH THIS SPAN.  §7tricies.168.

     "No definition among its own effects" is not "not written in the body",
     and §7tricies.80 says exactly why: DF reports a tuple reference once at
     its BASE register, so the other words of a wide definition are accounted
     NOWHERE.  A range sitting on one of those words sees no definition of its
     own and looks invariant while its value is produced by that write.

     Measured on TSVC2: every one of the 34 ranges this predicate called
     invariant is reached by a wide DEF as well as a wide read -- `s291's
     lr0 r3+1' by `lr4's quad definition at insn 1, nregs 4'.  Relocating it
     to a preheader copy would have fed it the INCOMING value while its real
     producer was inside the loop.  So the predicate has to ask, and asking
     leaves it with no population until the non-base words of a wide
     reference are accounted somewhere -- which is the structural fix
     §7tricies.80 already names and neither a copy nor an exemption
     substitutes for.  */
  {
    unsigned rlo = lr[root]->regno;
    unsigned rhi = rlo + MAX (lr[root]->nregs, 1);

    for (unsigned k = 0; k < lr.length (); k++)
      {
	if (lr[k]->effects.is_empty ()
	    || lvx_lms_lrange_root (lr, (int) k) == root)
	  continue;
	for (unsigned e = 0; e < lr[k]->effects.length (); e++)
	  {
	    unsigned base, top;

	    if (!lr[k]->effects[e].is_def
		|| lr[k]->effects[e].slot == LVX_LMS_SLOT_INDIRECT
		|| MAX (lr[k]->effects[e].nregs, 1) <= 1)
	      continue;
	    base = lr[k]->regno + lr[k]->effects[e].offset;
	    top = base + MAX (lr[k]->effects[e].nregs, 1);
	    if (rlo < top && base < rhi)
	      return false;
	  }
      }
  }
  return true;
}

/* PROBE (§7tricies.168): clear RENAMED and say which guard did it, for the
   ranges whose holding is under investigation.  Every guard between the
   optimistic reset and the colouring is silent, so a range that comes out
   held names no reason; this makes the first one that clears it say so.  */

static void
lvx_lms_hold_range (const auto_vec<lvx_lms_lrange_part *> &lr, int root,
		    int guard)
{
  if (root < 0 || root >= (int) lr.length ())
    return;
  if (lr[root]->renamed && dump_file && lvx_lms_probe
      && lvx_lms_lrange_invariant_p (lr, root))
    lvx_lms_dump (";; lvx cms:   HELDBY guard=%d lr%d r%u+%d\n", guard, root,
		  lr[root]->regno, lr[root]->nregs);
  lr[root]->renamed = false;
}


static void
lvx_lms_build_lranges (const auto_vec<rtx_insn *> &seq,
		       auto_vec<lvx_lms_lrange_part *> &lr, int *ntie_out,
		       int *nunit_out, int *nheld_out, int *nwiden_out = NULL,
		       const auto_vec<HARD_REG_SET> *pre_xlive = NULL,
		       const auto_vec<lvx_lms_prop> *prop = NULL)
{
  int ntie = 0;
  /* References that CONTAIN an already-open range rather than sitting inside
     it -- the one sub-operand shape §7tricies.71 holds instead of renaming.
     Counted so that "cannot arise on LVX" is a measurement.  */
  int nwiden_local = 0, *nwiden = &nwiden_local;

  /* The range currently open for each hard register, as an index into LR, or
     -1.  A use with no open range is a reference to the previous iteration's
     value and opens one of its own, which the normalisation below folds into
     the register's first range.  */
  int open[FIRST_PSEUDO_REGISTER];
  /* The hard registers the body references at all, which is what an indirect
     use is allowed to open a range on -- see the long note at the indirect
     uses below.  */
  auto_bitmap touched;

  for (int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
    open[r] = -1;
  for (unsigned i = 0; i < seq.length (); i++)
    {
      df_ref ref;

      FOR_EACH_INSN_USE (ref, seq[i])
	if (DF_REF_REGNO (ref) < FIRST_PSEUDO_REGISTER)
	  bitmap_set_bit (touched, DF_REF_REGNO (ref));
      FOR_EACH_INSN_DEF (ref, seq[i])
	if (DF_REF_REGNO (ref) < FIRST_PSEUDO_REGISTER)
	  bitmap_set_bit (touched, DF_REF_REGNO (ref));
    }

  for (unsigned i = 0; i < seq.length (); i++)
    {
      df_ref ref;
      /* A TUPLE REFERENCE IS ONE OPERAND, and DF reports it once PER HARD
	 REGISTER -- all of them at the SAME location, so all at the same
	 slot.  §7tricies.70.  Keyed on the reference's own regno and with a
	 range opened per register, as the first version was, a 256-bit load
	 became FOUR ranges each claiming the same operand:

	   lr3 r32 root=3 eff=1/0D,8/1U      lo.s $r32r33r34r35 = ...
	   lr4 r33 root=4 eff=1/0D,8/1U
	   lr5 r34 root=5 eff=1/0D,9/1U
	   lr6 r35 root=6 eff=1/0D,9/1U

	 and lvx_lms_verify_lranges, which counts the distinct ROOTS claiming
	 each operand, reported four for one -- `dup', which refused the loop.
	 It refused 44 of 131 PolyBench loops, every one of them a body with a
	 tuple reference and not one without: the dominant blocker, and it was
	 the vectorised bodies.

	 So the range is keyed on the operand's BASE register, which is
	 REGNO (DF_REF_REG) -- the whole tuple's rtx -- and not on
	 DF_REF_REGNO, which is the individual word.  And the N references are
	 collapsed to one effect, since the SLOT identifies the operand: a
	 slot is the position of a REG in the pattern's sub-rtx walk, so two
	 different operands cannot share one.  SEENUSE and SEENDEF carry that
	 within the instruction, the two sides separately because a
	 read-modify-write operand is legitimately both.  */
      auto_vec<int> seenuse, seendef;

      /* Uses first: they read what the open range holds.  */
      FOR_EACH_INSN_USE (ref, seq[i])
	{
	  /* THE OPERAND'S WIDTH COMES FROM THE PATTERN, not from the
	     reference.  DF_REF_REG gives the individual word -- REG_NREGS of
	     it is 1 even for a quad -- while DF_REF_LOC points at the REG as
	     WRITTEN, whose mode carries the real width and whose REGNO is the
	     tuple's base.  Taking the width from the reference left every
	     range one register wide: the colouring then allocated a single
	     name for a four-register operand and the substitution wrote an
	     unaligned tuple base, which lvx_qgr_reg_name and
	     lvx_pgr_reg_name assert on.  */
	  rtx *uloc = DF_REF_LOC (ref);
	  rtx ureg = (uloc && *uloc && REG_P (*uloc)) ? *uloc
							: DF_REF_REG (ref);
	  int nr = ureg && REG_P (ureg) ? REG_NREGS (ureg) : 1;
	  unsigned r = (ureg && REG_P (ureg)) ? REGNO (ureg)
					      : DF_REF_REGNO (ref);
	  lvx_lms_effect e = { i, lvx_lms_reg_index (PATTERN (seq[i]),
						    DF_REF_LOC (ref)),
			       nr, false, false };

	  if (r >= FIRST_PSEUDO_REGISTER)
	    continue;
	  /* §7tricies.110: the propagated operand belongs to the SOURCE's
	     range, and reads the previous instance of it where the copy it
	     replaced was loop-carried.  The renaming then substitutes from
	     that range with no special case at emission.  */
	  {
	    const lvx_lms_prop *pe = lvx_lms_prop_lookup (prop, i, e.slot);

	    if (pe)
	      r = pe->regno, e.delayed = pe->delayed;
	  }
	  if (e.slot >= 0)
	    {
	      if (seenuse.contains (e.slot))
		continue;		/* another word of one operand */
	      seenuse.safe_push (e.slot);
	    }
	  /* A SUB-OPERAND OF AN OPEN RANGE JOINS IT.  §7tricies.71.  OPEN is
	     filled across a range's whole span, so a narrower reference finds
	     the range covering it and hangs on it at its own offset instead
	     of opening a second range that would PARTIALLY overlap the first
	     -- which is what left 334 overlaps and refused the loop.

	     Containment is the test, and for LVX it is the only case that can
	     arise: the sub-tuples are aligned, so two spans are either
	     disjoint, equal, or one inside the other.  A reference that
	     instead CONTAINS the open range is counted and the live range held,
	     since widening a range mid-walk would move every offset already
	     recorded against it.  */
	  if (open[r] >= 0)
	    {
	      unsigned blo = lr[open[r]]->regno;
	      unsigned bhi = blo + MAX (lr[open[r]]->nregs, 1);

	      if (r >= blo && r + nr <= bhi)
		e.offset = (int) (r - blo);		/* contained */
	      else
		{
		  if (nwiden)
		    (*nwiden)++;
		  lr[open[r]]->noname = true;
		  e.offset = 0;
		}
	    }
	  if (open[r] < 0)
	    {
	      lvx_lms_lrange_part *n = new lvx_lms_lrange_part ();
	      n->regno = r;
	      n->nregs = nr;
	      /* USE_FIRST means the CYCLIC WRAP and nothing else, so the
		 normalisation below is the only place that sets it.  A range
		 that is still use-headed afterwards belongs to a register the
		 body never writes, where the flag would say nothing: p2unroll
		 is 1 and the index is 0 whatever it holds.  */
	      n->use_first = false;
	      n->exit_live = false;
	      n->renamed = false;
	      n->noname = false;
	      n->p2unroll = 1;
	      n->ec = false;
	      n->capped = false;
	      n->self = n->parent = (int) lr.length ();
	      n->first_date = n->last_date = 0;
	      open[r] = (int) lr.length ();
	      lr.safe_push (n);
	      for (int w = 1; w < nr && r + w < FIRST_PSEUDO_REGISTER; w++)
		open[r + w] = open[r];
	    }
	  lr[open[r]]->effects.safe_push (e);
	}

      /* THE BRANCH'S INDIRECT USES, between its operands' reads and any
	 write it makes, because that is where they happen: a branch reads
	 every register live-in on its exit edge by transferring control to a
	 block that reads it.  Entered as USES, so each attaches to the range
	 the register has open at this point -- which is the whole answer to
	 "which of the several ranges on this register is the one live out",
	 by the same rule that keys any other reference to its range.

	 WHERE NO RANGE IS OPEN YET, one is opened, exactly as an ordinary use
	 does -- and that case is the IMPORTANT one, not an edge.  In a
	 top-tested loop the exit branch precedes the definition it is
	 indirectly reading: `for (i...) { if (A[i] == stop) break; s += ...; }'
	 puts the branch at body index 2 and the accumulate at 6, so at the
	 branch there is no open range for the accumulator and the value live
	 on the exit edge is the PREVIOUS iteration's.  That is precisely a
	 cyclic use, and opening a use-headed range lets the normalisation
	 below fold it into the def-headed range at the bottom -- which is
	 what puts the branch's indirect use and the later definition in ONE
	 live range, and is why nothing has to union them by hand.

	 Requiring an already-open range instead, which the first version did,
	 dropped the indirect use in exactly that shape and so dropped the
	 only case where the dependence is not already implied.

	 TOUCHED is what keeps the tuple case out.  DF reports a tuple at its
	 first register, so the constituents above it are the regno of no
	 reference in the body; a range invented for one of them would be a
	 range on a register nothing names.  A live-out register the body
	 never references at all is excluded by the same test, and correctly:
	 it still holds the register the code after the loop reads, so there
	 is nothing to move.  NREGS comes from the open range where there is
	 one, because an effect of a different width makes the whole live range
	 non-renameable and would silently switch renaming off for every
	 live-out tuple.

	 Every out-of-loop successor counts, EXIT_BLOCK included.  The arc
	 builder's own version of this picks ONE such successor per branch
	 block and skips the branch entirely when that search finds none,
	 which leaves a branch exiting straight to the function's exit
	 unguarded; here the set is the union over the edges, so there is no
	 such hole.  */
      if (JUMP_P (seq[i]) && BLOCK_FOR_INSN (seq[i]) != NULL)
	{
	  basic_block jbb = BLOCK_FOR_INSN (seq[i]);
	  HARD_REG_SET xlive;
	  edge e;
	  edge_iterator ei;

	  /* THREADED IN WHERE THE CFG HAS MOVED.  §7tricies.104.  On the
	     codegen path make_nodes has already created blocks and redirected
	     edges, so a successor here can be one DF never saw; the caller
	     captured this set while the CFG was still the loop's.  */
	  if (pre_xlive && i < pre_xlive->length ())
	    xlive = (*pre_xlive)[i];
	  else
	    {
	      CLEAR_HARD_REG_SET (xlive);
	      FOR_EACH_EDGE (e, ei, jbb->succs)
		{
		  bitmap lin;
		  bool inbody = false;

		  for (unsigned z = 0; z < seq.length (); z++)
		    if (BLOCK_FOR_INSN (seq[z]) == e->dest)
		      {
			inbody = true;
			break;
		      }
		  if (inbody || (lin = df_get_live_in (e->dest)) == NULL)
		    continue;
		  for (int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
		    if (bitmap_bit_p (lin, r))
		      SET_HARD_REG_BIT (xlive, r);
		}
	    }
	  for (int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
	    {
	      lvx_lms_effect e2;

	      if (!TEST_HARD_REG_BIT (xlive, r) || !bitmap_bit_p (touched, r))
		continue;
	      if (open[r] < 0)
		{
		  lvx_lms_lrange_part *n = new lvx_lms_lrange_part ();
		  n->regno = r;
		  n->nregs = 1;
		  n->use_first = false;
		  n->exit_live = false;
		  n->renamed = false;
		  n->noname = false;
		  n->p2unroll = 1;
		  n->ec = false;
		  n->capped = false;
		  n->self = n->parent = (int) lr.length ();
		  n->first_date = n->last_date = 0;
		  open[r] = (int) lr.length ();
		  lr.safe_push (n);
		}
	      /* ONE INDIRECT USE PER RANGE, not one per register.  Since
		 §7tricies.71 fills OPEN across a range's whole span, every
		 register of a tuple finds the same range here and the
		 per-register loop would push the same effect once per word --
		 four copies at (insn, SLOT_INDIRECT) for a quad, which is the
		 same "one operand in several places" mistake in the other
		 direction.  */
	      {
		bool had = false;

		for (unsigned q = 0; q < lr[open[r]]->effects.length (); q++)
		  if (lr[open[r]]->effects[q].insn == i
		      && lr[open[r]]->effects[q].slot == LVX_LMS_SLOT_INDIRECT)
		    {
		      had = true;
		      break;
		    }
		if (had)
		  continue;
	      }
	      e2.insn = i;
	      e2.slot = LVX_LMS_SLOT_INDIRECT;
	      e2.nregs = MAX (lr[open[r]]->nregs, 1);
	      e2.is_def = false;
	      e2.delayed = false;
	      lr[open[r]]->effects.safe_push (e2);
	      /* And this is what EXIT_LIVE now means: a range with an
		 indirect use is one whose value leaves the loop.  */
	      lr[open[r]]->exit_live = true;
	    }
	}

      /* Then the definitions: each opens a new range.  */
      FOR_EACH_INSN_DEF (ref, seq[i])
	{
	  rtx *dloc = DF_REF_LOC (ref);
	  rtx reg = (dloc && *dloc && REG_P (*dloc)) ? *dloc
						     : DF_REF_REG (ref);
	  int nr = reg && REG_P (reg) ? REG_NREGS (reg) : 1;
	  unsigned r = (reg && REG_P (reg)) ? REGNO (reg)
					    : DF_REF_REGNO (ref);
	  lvx_lms_effect e = { i, lvx_lms_reg_index (PATTERN (seq[i]),
						    DF_REF_LOC (ref)),
			       nr, true, false };
	  bool nofull = false;
	  /* A WRITE THAT DOES NOT FULLY AND UNCONDITIONALLY DEFINE ITS VALUE
	     CANNOT BE EXPANDED.  §7tricies.117.

	     Modulo expansion gives instance I a name of its own and assumes
	     that name is completely produced each iteration.  Two writes
	     break the assumption without breaking anything a later check
	     looks at:

	       PARTIAL     the bytes it does not write still hold what that
			   name held, which under expansion is instance
			   I - P2UNROLL's data rather than I - 1's.
	       PREDICATED  a false guard writes nothing at all, so the name
			   keeps instance I - P2UNROLL's value.  The same
			   defect in time rather than in bytes.

	     Neither is caught downstream, and that is why this is a guard
	     and not a verification: the renamed instruction is perfectly
	     valid -- a legal write to a legal register -- and `recog'
	     accepts it.  What is wrong is which instance the UNWRITTEN part
	     belongs to, and nothing examines that.

	     The third member of the family, a TIED write, is deliberately NOT
	     here, and §7tricies.148 is why.  Its miss is a refusal rather
	     than a miscompile -- the expansion puts source and destination in
	     different registers and the tie fails `recog' -- so the rename
	     site can simply decline to rewrite a tied operand, which is where
	     it is handled.  Holding tied-headed ranges here as well was
	     measured and costs 16 pipelines for nothing once §7tricies.150
	     let that site read the right constraint table.

	     Held rather than capped at degree one, which is the same thing
	     for a range with a single definition and the safer statement for
	     one with several.  */
	  if ((DF_REF_FLAGS (ref) & DF_REF_PARTIAL) != 0
	      || GET_CODE (PATTERN (seq[i])) == COND_EXEC)
	    {
	      if (r < FIRST_PSEUDO_REGISTER)
		{
		  if (open[r] >= 0)
		    lr[open[r]]->noname = true;
		  nofull = true;	/* and the range it may OPEN below */
		}
	    }


	  /* A READ-MODIFY-WRITE REFERENCE IS ONE OPERAND, and DF reports it
	     twice -- as a use and as a definition at the SAME location.  The
	     use already entered it; entering it again as a definition would
	     put one operand in two live ranges, which is the ambiguity this
	     structure exists to not have.  The write is what opens a range,
	     so the range opens and the operand stays the effect it already
	     is, now marked as the definition that heads it.  */

	  if (r >= FIRST_PSEUDO_REGISTER)
	    continue;
	  if (e.slot >= 0)
	    {
	      if (seendef.contains (e.slot))
		continue;		/* another word of one operand */
	      seendef.safe_push (e.slot);
	    }

	  /* A DEFINITION OF A SUB-SPAN IS A PARTIAL DEFINITION OF THE RANGE,
	     not a range of its own.  §7tricies.75.

	     Opening one made two live ranges PARTIALLY overlap -- [4,8) from
	     a quad write and [6,8) from a pair write inside it -- which made
	     the partial-overlap rule hold BOTH, which put them on the same
	     architectural registers, which left an intra-cycle circuit that
	     cannot be linearised.  That chain accounted for every remaining
	     refusal on PolyBench and for the 141 partial overlaps, and a
	     probe confirmed it: suppressing only the hold made all nine
	     circuits disappear.

	     Attaching costs almost nothing, because a partial definition is
	     just an effect with an offset:

	       - the HEAD is unchanged.  EFFECTS[0], USE_FIRST, the cyclic
		 normalisation and the live-out copy identify where the value
		 ENTERS the range, and a definition in the middle does not
		 change that;
	       - ONE INTERVAL is still right.  The range owns its whole span
		 continuously from first effect to last, which is what
		 interference needs; a per-sub-span interval would only let
		 another value pack into part of the span, an optimisation and
		 not a requirement;
	       - ONE P2UNROLL is still right.  Every effect addresses
		 reg[index] + offset, so iteration i's partial definition
		 writes a different name from iteration i+1's;
	       - DUP is untouched, precisely because nothing is split: one
		 operand stays in one root, and the colouring still sees one
		 unit per tuple so the aligned allocation stays free.

	     And the range then spanning two GENERATIONS of the sub-span
	     within one iteration needs no renaming to tell them apart: in a
	     sequential stream the register holds whatever was written last,
	     and the order between an earlier reader and the partial
	     definition is an ordinary anti-dependence the schedule already
	     carries.  */
	  if (open[r] >= 0 && e.slot >= 0)
	    {
	      unsigned blo = lr[open[r]]->regno;
	      unsigned bhi = blo + MAX (lr[open[r]]->nregs, 1);

	      if (r >= blo && r + nr <= bhi && !(r == blo && r + nr == bhi))
		{
		  unsigned q;

		  e.offset = (int) (r - blo);
		  /* A read-modify-write of the sub-span is one operand: the
		     use already entered it, so mark that effect as the
		     definition rather than adding a second.  */
		  for (q = 0; q < lr[open[r]]->effects.length (); q++)
		    if (lr[open[r]]->effects[q].insn == i
			&& lr[open[r]]->effects[q].slot == e.slot)
		      {
			lr[open[r]]->effects[q].is_def = true;
			break;
		      }
		  if (q == lr[open[r]]->effects.length ())
		    lr[open[r]]->effects.safe_push (e);
		  continue;		/* OPEN still owns the whole span */
		}
	    }

	  lvx_lms_lrange_part *n = new lvx_lms_lrange_part ();
	  n->regno = r;
	  n->nregs = nr;
	  n->use_first = false;
	  n->exit_live = false;
	  n->renamed = false;
	  n->noname = nofull;
	  n->p2unroll = 1;
	  n->ec = false;
	  n->capped = false;
	  n->self = n->parent = (int) lr.length ();
	  n->first_date = n->last_date = 0;
	  n->effects.safe_push (e);
	  lr.safe_push (n);
	  if (e.slot >= 0 && open[r] >= 0)
	    for (unsigned q = 0; q < lr[open[r]]->effects.length (); q++)
	      if (lr[open[r]]->effects[q].insn == i
		  && lr[open[r]]->effects[q].slot == e.slot)
		{
		  /* The same operand: drop the use's copy, and put the two
		     ranges in one live range -- the operand is read and written
		     in place, so one register has to serve both.  */
		  lr[open[r]]->effects.ordered_remove (q);
		  lvx_lms_lrange_union (lr, n, lr[open[r]]);
		  break;
		}
	  open[r] = (int) lr.length () - 1;
	  for (int w = 1; w < nr && r + w < FIRST_PSEUDO_REGISTER; w++)
	    open[r + w] = open[r];
	}
    }

  /* THE CYCLIC NORMALISATION.  A register read before it is written in the
     body is reading the value the LAST definition of that register produced
     in the PREVIOUS iteration.  Those references -- the ones that opened a
     use-headed range because no definition had been seen yet -- move into the
     range the last definition opened, flagged DELAYED, and that range is
     marked USE_FIRST.  The index arithmetic then carries the wrap:
     `+ use_first' puts the incoming value at index 0 and the first definition
     at index 1, `- delayed' sends a wrapped reference back one instance.

     This is unfold_igraph_make_ranges, and the direction is worth stating
     because the reference reads the other way round: unfold_igraph_map_range
     PREPENDS, so its `first_range' is the newest range and its `last_range'
     the oldest.  Its test -- first is 'D', last is neither 'D' nor 'C' -- is
     therefore `the latest range is def-headed and the earliest is use-headed',
     which is the test below.  Transcribed with my own ordering it read
     `lr[first]->use_first || !lr[last]->use_first' and could never fire:
     OPEN[r] is never cleared, so once any definition of R is seen no later
     use can open a range, and the latest range is use-headed only when R has
     no definition at all -- the case FIRST == LAST already excludes.  */
  for (int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
    {
      int first = -1, last = -1;

      for (unsigned k = 0; k < lr.length (); k++)
	if ((int) lr[k]->regno == r && !lr[k]->effects.is_empty ())
	  {
	    if (first < 0)
	      first = (int) k;
	    last = (int) k;
	  }
      if (first < 0 || first == last)
	continue;
      /* The earliest range must be use-headed and the latest def-headed.  */
      if (lr[first]->effects[0].is_def || !lr[last]->effects[0].is_def)
	continue;

      for (unsigned e = 0; e < lr[first]->effects.length (); e++)
	{
	  lvx_lms_effect d = lr[first]->effects[e];
	  d.delayed = true;
	  lr[last]->effects.safe_push (d);
	}
      lr[last]->use_first = true;
      lr[first]->effects.truncate (0);
    }

  /* THE TIE UNIFICATION.  §7tricies.28.

     A TIED OPERAND MUST IMPLY ONE RENAMING UNIT, and it does not imply one
     range: an insn's uses are entered before its definitions, so the read of
     a tied accumulator attaches to the range the REACHING definition opened
     while the write opens a new one.  Those coincide only when the register
     has a single definition in the body -- then the cyclic normalisation
     merges the pre-def read into it -- and gemm's body defines r1 three times
     because RA reused it.  There the fma's read landed on the load's range
     (four names) and its write on its own (one name), and `fma r1 = a, r41'
     does not recognise: 8 refused instances on that loop alone.

     The reference marks the tied DEFINITION non-renameable instead (type 'R'
     in unfold_igraph_store_effects, from op_alt[def].matched) and leaves the
     tied READ in the preceding range, where it is still renamed -- which
     emits exactly `fma r1 = a, r41'.  That is not followed here.

     The fix is a UNION over ranges rather than a merge of them: the two
     ranges share one stack of names, and every reference still belongs to
     exactly one range, so the lookup stays exact.  That is what the tie
     CLASSES were reaching for and could not have, because a class had merged
     away the information needed to find the operand again.

     Soundness of sharing a stack between two definitions: when the tie makes
     the second read what the first wrote, the RAW arc between them already
     serialises them within the iteration, and across iterations the names
     differ whenever the union expands.  Where it does not expand the
     anti-dependence is the one lvx_lms_verify_antidep checks.  */
  for (unsigned i = 0; i < seq.length (); i++)
    {
      rtx set = single_set (seq[i]);
      const operand_alternative *op_alt;
      int nop, d = -1, m;

      if (!set || !REG_P (SET_DEST (set))
	  || REGNO (SET_DEST (set)) >= FIRST_PSEUDO_REGISTER)
	continue;
      if (!INSN_P (seq[i]) || INSN_CODE (seq[i]) < 0)
	continue;
      /* Same care as lvx_insn_has_tied_operand_p: the non-aborting form, and
	 the alternative's range tested before which_op_alt asserts it.  */
      extract_insn (seq[i]);
      if (!constrain_operands (1, get_enabled_alternatives (seq[i])))
	continue;
      preprocess_constraints (seq[i]);
      if (!IN_RANGE (which_alternative, 0, recog_data.n_alternatives - 1))
	continue;
      nop = recog_data.n_operands;
      op_alt = which_op_alt ();
      for (int o = 0; o < nop; o++)
	if (recog_data.operand[o] == SET_DEST (set))
	  {
	    d = o;
	    break;
	  }
      if (d < 0 || (m = op_alt[d].matched) < 0 || m >= nop)
	continue;
      /* The matched operand is the input this destination must equal.  */
      {
	rtx u = recog_data.operand[m];
	lvx_lms_lrange_part *gd, *gu;

	if (!u || !REG_P (u) || REGNO (u) >= FIRST_PSEUDO_REGISTER)
	  continue;
	gd = lvx_lms_lrange_def_of (lr, i, REGNO (SET_DEST (set)));
	gu = lvx_lms_lrange_use_of (lr, i, REGNO (u));
	if (lvx_lms_lrange_union (lr, gd, gu))
	  ntie++;
      }
    }

  /* THE KEPT ANTI-DEPENDENCES ARE LIVE RANGE CONSTRAINTS, and that is now a
     property of the GRAPH, not a repair applied after it.  §7tricies.29.

     A kept register WAR or WAW arc asserts that two references end in one
     register.  If they are two live ranges with two stacks of names, the
     renaming has separated exactly what the arc says cannot be separated --
     so either the arc is redundant or the live ranges must be one.

     The first version read the arcs and unioned: 20 of 113 kept arcs joined
     distinct live ranges, and repairing them that way is the wrong direction,
     because those 20 are precisely the arcs that did not need to be kept.
     The partition here is built from the body alone -- the walk, the
     read-modify-write operands and the tie pass, none of which need a
     schedule -- so lvx_lms_build_graph can consult it while it decides which
     anti-dependences to keep, and the invariant holds by construction.  That
     ordering is the reference's too: unfold_igraph_make_ranges runs before
     scheduling and unfold_igraph_set_range_dates after it.  */

  /* THE UNION IS THE RENAMING UNIT, so its span and its flags are the union
     of its members'.  A union holding a range that is not def-headed is not
     renamed at all: that range's value comes from outside the body, and the
     reference says the same of an unexpanded USE_FIRST range -- it keeps the
     original REGNO.  Here the tie would force the incoming register on every
     name, which is a stack of one.  */
  for (unsigned k = 0; k < lr.length (); k++)
    {
      int root;

      if (lr[k]->effects.is_empty ())
	continue;
      root = lvx_lms_lrange_root (lr, (int) k);
      if ((int) k == root)
	continue;
      /* A UNION ACROSS DIFFERENT BASES IS NOT RENAMEABLE.  §7tricies.71.
	 Each member's effect offsets are relative to ITS OWN regno, so
	 taking the widest span here would leave the narrower member's
	 offsets measured from the wrong base and the substitution would
	 write the wrong register.  Rebasing them is what the out-tree proper
	 would do; until the case is shown to occur it is held, which is
	 sound, and NWIDEN-style counting is what would show it.  */
      if (lr[k]->regno != lr[root]->regno)
	lr[k]->noname = true;
      lr[root]->nregs = MAX (lr[root]->nregs, lr[k]->nregs);
      lr[root]->use_first |= lr[k]->use_first;
    }
  for (unsigned k = 0; k < lr.length (); k++)
    {
      int root = lr[k]->effects.is_empty () ? (int) k
		 : lvx_lms_lrange_root (lr, (int) k);

      lr[root]->renamed = true;
    }
  /* AND NOW THE GUARDS THAT FIRED BEFORE THE RESET.  §7tricies.118.

     `renamed' is reset optimistically above, once the unions are final,
     because the union does not merge flags: a guard writing `false' on a part
     that later becomes a non-root member would lose its answer.  The cost of
     that design is that a guard running EARLIER than the reset is silently
     undone, and three were -- the widening fallback, the different-bases
     union, and the partial/predicated write of §7tricies.117.  They set
     `noname' on the part instead, which the reset does not touch, and it is
     applied to the root here.  */
  for (unsigned k = 0; k < lr.length (); k++)
    if (lr[k]->noname)
      lr[lvx_lms_lrange_root (lr, (int) k)]->renamed = false;
  /* STILL USE-HEADED MEANS THE BODY NEVER WRITES IT.  §7tricies.91.

     This looks like a refusal to rename the loop-carried values, which are
     exactly the ones worth renaming, and it is not: the normalisation above
     has already ROTATED them.  A register with a use-headed range at the top
     and a def-headed one at the bottom -- the cyclic wrap -- has the wrapped
     uses folded into the def-headed range with `delayed' set, so the merged
     range IS def-headed and passes this test.  That rotation is what makes
     `(iteration + use_first - delayed) & (p2unroll - 1)' the whole of the
     index computation.

     What reaches here still use-headed is the case the normalisation skips,
     FIRST == LAST: one range, opened by a use.  A definition always opens a
     range of its own, so a single use-headed range means the body contains no
     definition of that register at all -- a loop-invariant or a value defined
     outside.  There is one value for every iteration, p2unroll of 1 is the
     right answer rather than a conservative one, and giving it kunroll names
     would mean copying the incoming value into each of them.  So this is not
     a restriction to be lifted; it is the statement that there is nothing to
     expand.  Counted as `held' in the lranges dump.  */
  /* §7tricies.167: NOTHING TO EXPAND IS NOT NOTHING TO RELOCATE.  The
     paragraph above is right that P2UNROLL of 1 is the answer and that
     kunroll names would mean copying the incoming value into each of them.
     It does not follow that the one name has to be the register the value
     ARRIVED in, and holding it there is what makes it collide with a tuple
     that needs that register -- fourteen of the fifteen `a dropped
     anti-dependence is not separated' refusals are a held narrow range
     sitting inside a held wide one (§7tricies.165).

     Relocating costs ONE copy in the preheader, not kunroll of them,
     precisely because p2unroll stays 1.  So the range keeps its single name
     and gives up only its address.  */
  for (unsigned k = 0; k < lr.length (); k++)
    if (!lr[k]->effects.is_empty () && !lr[k]->effects[0].is_def)
      {
	int rt = lvx_lms_lrange_root (lr, (int) k);

	lvx_lms_hold_range (lr, rt, 3);
      }

  /* MULTI-REGISTER OPERANDS: THE FALLBACK.  §7tricies.33.

     A reference to a register TUPLE covers [regno, regno + nregs) and DF
     reports it as one reference at the first register.  Another instruction
     may reference part of that span on its own -- a widening multiply writes
     a pair and the shift after it reads only the high half -- and then one
     live range's span lies inside another's.  Renamed independently the pair
     moves to a fresh aligned pair while the half moves elsewhere, and the two
     stop naming the same storage.

     Two better answers exist and neither is this one: keep live ranges at
     single-register granularity and associate those that share an operand, or
     keep wide live ranges and treat each narrower reference as a partial write,
     tracking per effect WHICH registers of the span it touches.  Both need
     the renaming to carry a sub-register offset, which it does not.

     So the fallback, which is immediate and sound: a live range whose effects
     are not all the same width, or whose span PARTIALLY overlaps another
     live range's, is not renamed -- it keeps the register RA gave it, like any
     held live range.  An exact overlap is fine: that is the ordinary case of two
     references to one tuple.  */
  for (unsigned k = 0; k < lr.length (); k++)
    {
      int root;

      if (lr[k]->effects.is_empty ())
	continue;
      root = lvx_lms_lrange_root (lr, (int) k);
      /* NOT "all the same width" ANY MORE, but "an aligned sub-range of the
	 root's span".  §7tricies.71: an effect carries its offset, so a
	 narrower reference is renameable -- reg[index] + offset at its own
	 width -- and only a reference that leaves the span, or sits at an
	 offset its own width does not divide, cannot be.  On LVX the latter
	 cannot arise from the register classes; it is tested rather than
	 assumed because nothing in this file should rely on that.  */
      for (unsigned e = 0; e < lr[k]->effects.length (); e++)
	{
	  int off = lr[k]->effects[e].offset;
	  int w = MAX (lr[k]->effects[e].nregs, 1);

	  if (lr[k]->effects[e].slot == LVX_LMS_SLOT_INDIRECT)
	    continue;		/* no operand, nothing to substitute */
	  if (off < 0 || off + w > MAX (lr[root]->nregs, 1) || off % w != 0)
	    {
	      lvx_lms_hold_range (lr, root, 4);
	      break;
	    }
	}
    }
  for (unsigned a = 0; a < lr.length (); a++)
    {
      if (lr[a]->effects.is_empty ())
	continue;
      for (unsigned b = a + 1; b < lr.length (); b++)
	{
	  unsigned alo, ahi, blo, bhi;
	  int ra, rb;

	  if (lr[b]->effects.is_empty ())
	    continue;
	  ra = lvx_lms_lrange_root (lr, (int) a);
	  rb = lvx_lms_lrange_root (lr, (int) b);
	  if (ra == rb)
	    continue;
	  alo = lr[a]->regno;
	  ahi = alo + MAX (lr[a]->nregs, 1);
	  blo = lr[b]->regno;
	  bhi = blo + MAX (lr[b]->nregs, 1);
	  if (alo < bhi && blo < ahi && !(alo == blo && ahi == bhi))
	    {
	      /* §7tricies.167: AN INVARIANT RANGE IS NOT A SUB-REGISTER VIEW.
		 The reason above -- "renamed independently the pair moves to
		 a fresh aligned pair while the half moves elsewhere, and the
		 two stop naming the same storage" -- is about a reference
		 that IS part of the wide value, where the aliasing carries a
		 dependence.  A range whose every reference is a read has no
		 definition anywhere in the body, so nothing in the body
		 produced its value and it is not a view of the other's
		 output: the two merely occupy overlapping storage, and
		 separating them is the point rather than the hazard.

		 Sound even when the other range stays held and keeps writing
		 those registers, because the relocated range's copy is taken
		 in the preheader FROM the original register, before the body
		 writes anything -- and sound when both are invariant, for the
		 same reason: each copy reads the incoming storage, so the two
		 still agree about the value they came from.  */
	      lvx_lms_hold_range (lr, ra, 5);
	      lvx_lms_hold_range (lr, rb, 5);
	    }
	}
    }

  /* A WIDE REFERENCE REACHES REGISTERS ANOTHER RANGE OWNS.  §7tricies.80.

     The loop above compares RANGE SPANS, and a range's span comes from its
     heading definition.  An EFFECT may be wider than that: DF reports a tuple
     read once per register but all at one pattern location, so it is one slot
     and one effect, attached to the range of its BASE register and carrying
     its own NREGS.  The other words of that reference are then accounted
     nowhere, and a range holding only them does not overlap anything by span.

     s116 on lvx-2 is the case.  `fmuldp $r32r33' reads a pair; the read
     attaches to r32's range (nregs 1, effect nregs 2, so the width rule above
     holds it, correctly).  r33 was DEFINED separately -- `copyd $r33 = $r4',
     earlier in the body, so nothing was open on r33 and it opened its own
     one-register range -- and that range has a definition and NO use: the
     pair read never reached it.  It therefore looked dead, was renamed to r4,
     and the write to r33 was deleted as dead while the fmuldp went on reading
     r33.  Lane 1 of every pair was stale.

     So the covered ranges must be held too.  Held rather than unioned
     because it is immediate and sound, which is this file's own stated
     fallback for the general shape; unioning them into one range of the
     reference's span is the better answer and the one §7tricies.71 is working
     towards.  */
  for (unsigned k = 0; k < lr.length (); k++)
    {
      if (lr[k]->effects.is_empty ())
	continue;
      for (unsigned e = 0; e < lr[k]->effects.length (); e++)
	{
	  unsigned base, top;
	  int rk;

	  if (lr[k]->effects[e].slot == LVX_LMS_SLOT_INDIRECT)
	    continue;
	  if (MAX (lr[k]->effects[e].nregs, 1) <= 1)
	    continue;
	  base = lr[k]->regno + lr[k]->effects[e].offset;
	  top = base + MAX (lr[k]->effects[e].nregs, 1);
	  rk = lvx_lms_lrange_root (lr, (int) k);
	  for (unsigned j = 0; j < lr.length (); j++)
	    {
	      unsigned jlo, jhi;
	      int rj;

	      if (j == k || lr[j]->effects.is_empty ())
		continue;
	      rj = lvx_lms_lrange_root (lr, (int) j);
	      if (rj == rk)
		continue;
	      jlo = lr[j]->regno;
	      jhi = jlo + MAX (lr[j]->nregs, 1);
	      if (jlo < top && base < jhi)
		{
		  if (dump_file && lvx_lms_probe
		      && lvx_lms_lrange_invariant_p (lr, rj))
		    lvx_lms_dump (";; lvx cms:   WIDEREACH lr%d r%u+%d held by"
				  " lr%u's effect insn=%u nregs=%d %s\n", rj,
				  lr[rj]->regno, lr[rj]->nregs, k,
				  lr[k]->effects[e].insn,
				  lr[k]->effects[e].nregs,
				  lr[k]->effects[e].is_def ? "DEF" : "read");
		  lvx_lms_hold_range (lr, rj, 6);
		  lvx_lms_hold_range (lr, rk, 6);
		}
	    }
	}
    }

  /* THE DOLOOP COUNTER IS HELD, NOT RENAMED.  §7tricies.64.

     doloop_end_<mode> holds its counter FOUR times -- the `ne' comparison's
     match_operand, and three match_dups across the decrement and the
     unspec -- while the live range records the one effect DF reports.  The
     substitution rewrites a REG by its position in a sub-rtx walk, so a
     renamed copy has the new register in one place and the old one in the
     others, the match_dups no longer match, and recog rejects it.  That is
     the dry-run's "a slot's insn refuses the rename substitution", and while
     a doloop latch was declined outright it was unreachable; with the latch
     kept it cost seven PolyBench loops that the -fno-hwloop arm pipelines.

     Holding the counter costs nothing.  It is a self-recurrence at distance
     one whose successive definitions are a true dependence, so the copies
     read what the previous copy wrote whether or not the name rotates, and
     RecMII is unchanged.  And the emission wants it held for its own reason:
     the explicit counter tests it builds for the non-closing instances, and
     the SETUP arithmetic for an unrolled kernel, resolve the counter through
     this live range and need it to name one register.

     Asked of the DOLOOP pattern specifically rather than of any pattern that
     mentions a register more often than the live range has effects on it.  The
     general rule is the right one eventually -- and the retry it wants is to
     hold the live range and re-colour, not to decline -- but a tied operand's
     read and write are also two occurrences of one register, so the general
     form would hold every accumulator of every fused multiply-add on the
     strength of a guess about how those effects are recorded.  */
  for (unsigned i = 0; i < seq.length (); i++)
    {
      if (!JUMP_P (seq[i]) || !lvx_doloop_end_p (seq[i]))
	continue;
      for (unsigned k = 0; k < lr.length (); k++)
	{
	  if (lr[k]->effects.is_empty ())
	    continue;
	  for (unsigned e = 0; e < lr[k]->effects.length (); e++)
	    if (lr[k]->effects[e].insn == i)
	      {
		lvx_lms_hold_range (lr, lvx_lms_lrange_root (lr, (int) k), 7);
		break;
	      }
	}
    }

  if (nwiden_out)
    *nwiden_out = nwiden_local;
  if (ntie_out)
    *ntie_out = ntie;
  if (nunit_out || nheld_out)
    {
      int nunit = 0, nheld = 0;

      for (unsigned k = 0; k < lr.length (); k++)
	if (!lr[k]->effects.is_empty ()
	    && lvx_lms_lrange_root (lr, (int) k) == (int) k)
	  {
	    nunit++;
	    if (!lr[k]->renamed)
	      nheld++;
	  }
      if (nunit_out)
	*nunit_out = nunit;
      if (nheld_out)
	*nheld_out = nheld;
    }
}

/* THE DATES, WHICH NEED A SCHEDULE.  §7tricies.30.

   Split from the structural build because the structure must exist BEFORE the
   graph is finished -- lvx_lms_build_graph consults the live range partition to
   decide which anti-dependences to keep -- while the dates cannot exist until
   SIGMA does.  The reference splits it the same way, and at the same seam:
   unfold_igraph_make_ranges then unfold_igraph_set_range_dates.

   A DELAYED effect is dated one period later than its instruction, because it
   belongs to the previous instance: that is the whole of what the cyclic wrap
   costs in span, and it is why the accumulator of a loop-carried recurrence
   comes out with a life of exactly II.

   The span, the flags and the stack all belong to the LIVE RANGE, so they are
   aggregated onto the root and then copied back, and P2UNROLL is computed
   once from the root's span.  */

static void
lvx_lms_set_lrange_dates (const auto_vec<rtx_insn *> &seq,
			  auto_vec<lvx_lms_lrange_part *> &lr,
			  const auto_vec<int> &sigma, int ii, bitmap liveout)
{
  for (unsigned k = 0; k < lr.length (); k++)
    {
      lvx_lms_lrange_part *g = lr[k];
      bool first = true;

      g->first_date = g->last_date = 0;
      if (g->effects.is_empty ())
	continue;
      for (unsigned e = 0; e < g->effects.length (); e++)
	{
	  unsigned i = g->effects[e].insn;
	  int d;

	  if (i >= sigma.length ())
	    continue;
	  d = sigma[i] + (g->effects[e].delayed ? ii : 0);
	  if (first)
	    {
	      g->first_date = g->last_date = d;
	      first = false;
	    }
	  else
	    {
	      g->first_date = MIN (g->first_date, d);
	      g->last_date = MAX (g->last_date, d);
	    }
	}
      /* The heading definition is the start of the span whatever the effect
	 order says: a delayed read dated a period later must not become the
	 first date, and the definition is effects[0].  */
      if (g->effects[0].is_def && g->effects[0].insn < sigma.length ())
	g->first_date = sigma[g->effects[0].insn];
      /* EXIT_LIVE IS NOT SET HERE ANY MORE.  §7tricies.67.

	 It used to be `bitmap_bit_p (liveout, g->regno) && g->renamed',
	 which asks about a REGISTER: after allocation a live-out register may
	 hold an unrelated value, and a body may hold several ranges on one
	 register -- the very thing this structure exists to tell apart.  Both
	 went wrong on store.c, where an intra-iteration range and the
	 loop-carried one share r1 and the register test flagged both.

	 lvx_lms_build_lranges sets it instead, from the branch's INDIRECT
	 USES: the range that has one is the range live on the exit edge, by
	 the same rule that keys every other reference to its range.  That is
	 a structural answer, not a predicate over the side, and it needs no
	 dataflow of its own.

	 LIVEOUT stays in the signature because the callers hold it and
	 because a cross-check belongs here if the two are ever suspected of
	 disagreeing; it is deliberately not a second way to compute the
	 answer, since a rule stated twice goes stale.  */
      (void) liveout;
    }

  /* Onto the live range.  */
  for (unsigned k = 0; k < lr.length (); k++)
    {
      int root;

      if (lr[k]->effects.is_empty ())
	continue;
      root = lvx_lms_lrange_root (lr, (int) k);
      if (root == (int) k)
	continue;
      /* §7tricies.119: the root is the LATEST part of its union, which is
	 what makes `lr[root]->effects[0]' the definition that heads the live
	 range for the three consumers that read it.  Asserted rather than
	 trusted, because union-find states no order and the property used to
	 rest on the two call sites' argument order.  */
      gcc_assert (root > (int) k);
      lr[root]->first_date = MIN (lr[root]->first_date, lr[k]->first_date);
      lr[root]->last_date = MAX (lr[root]->last_date, lr[k]->last_date);
      lr[root]->exit_live |= lr[k]->exit_live;
    }

  /* A LIVE-OUT RANGE LIVES TO THE END OF ITS DRAIN, NOT TO ITS LAST IN-LOOP
     USE.  §7tricies.135.

     The interval used to stop at LAST_DATE, the last reference inside the
     body.  That is wrong for a range that leaves the loop, and the way it is
     wrong is not about interference between its own copies: two DIFFERENT
     live-out ranges then get disjoint intervals and the colouring gives them
     ONE register, which no drain can unpick.

     Measured on mad libz's inflate_fast loop 2, II = 11: lr3 (architectural
     r7, interval [1,3]) and lr7 (architectural r8, interval [4,11]) were both
     given r8, and the two intervals TILE the circle of circumference
     kunroll * II exactly, so r8 always holds one of them.  Both leave the
     loop, so every drain must restore both architectural registers and emits
     `r7 <- r8' unconditionally -- and which value is resident depends on where
     the exit was taken.  The decoded match came out with the wrong length AND
     distance while the bit position stayed exact.

     AND THE EXIT'S OWN DATE IS NOT THE BOUND EITHER.  A drain replays the
     work still in flight when its exit was taken: the drain for the exit at
     date 1 holds insn 4 at date 3 and insn 7 at date 4, and insn 4 is lr7's
     definition.  So the drain's own replayed work overwrites r8 before the
     reconciliation copy at the end of that drain reads it.  The value has to
     survive to the end of the DRAIN, which is bounded by the end of the
     schedule.

     So the interval runs to the schedule's last date.  Conservative -- it
     makes every live-out range conflict with everything scheduled after its
     definition -- and that is the direction a register's contents must be
     reasoned about when a drain is going to copy them back.  The control is
     -mno-lms-exit-live-span.  */
  {
    int endd = 0;

    for (unsigned i = 0; i < sigma.length (); i++)
      endd = MAX (endd, sigma[i] + ii);
    for (unsigned k = 0; k < lr.length (); k++)
      {
	lr[k]->occ_last = lr[k]->last_date;
	if (lvx_lms_exit_live_span
	    && !lr[k]->effects.is_empty ()
	    && lvx_lms_lrange_root (lr, (int) k) == (int) k
	    && lr[k]->exit_live)
	  lr[k]->occ_last = MAX (lr[k]->last_date, endd);
      }
  }

  /* §7tricies.66's disambiguation -- keeping, among the ranges on one
     register, the one with the greatest LAST_DATE -- is gone with the
     predicate that needed it.  An indirect use attaches to exactly one range
     per register by construction, so there is nothing left to disambiguate
     and the heuristic that approximated it would only be a second answer to
     the same question.  */

  /* Each live range's unroll factor: the instances its life spans, rounded up to
     a power of two so the index is a mask (§7tricies.2's third place the
     power of two pays).  A live range that is not renamed is not expanded --
     after the normalisation that is a register the body never writes, where
     every iteration reads the same value and the span says nothing about
     pressure.  The reference says the same in one line, "In case of USE_FIRST
     and not EXPANDED, allocate the original REGNO".  */
  for (unsigned k = 0; k < lr.length (); k++)
    {
      int root = lr[k]->effects.is_empty () ? (int) k
		 : lvx_lms_lrange_root (lr, (int) k);
      /* PROVISIONAL, AND READ ONLY BY THE DUMP.  §7tricies.90.  The colouring
	 overwrites P2UNROLL with its own degree, and every functional read --
	 the index computation, the invariant check -- happens after that.  So
	 the `maxp2' of the lranges dump line is THIS value, not the one the
	 renaming uses: a trap worth knowing, since it makes that column
	 useless for measuring anything the colouring decides.  It carries the
	 same ec anyway, so the two never disagree in the direction that
	 matters.  */
      int life = (lr[root]->last_date - lr[root]->first_date
		  + (lvx_lms_lrange_ec_p (seq, lr[root]) ? 1 : 0));
      int need = ii > 0 ? (life + ii - 1) / ii : 1;
      int p2 = 1;

      if (!lr[root]->renamed)
	need = 1;
      while (p2 < need && p2 < LVX_LMS_MAX_KUNROLL)
	p2 <<= 1;
      lr[k]->p2unroll = MAX (p2, 1);
      lr[k]->use_first = lr[root]->use_first;
      lr[k]->first_date = lr[root]->first_date;
      lr[k]->last_date = lr[root]->last_date;
      for (int x = 0; x < LVX_LMS_MAX_KUNROLL; x++)
	lr[k]->reg[x] = (int) lr[k]->regno;
    }
}

/* THE LIVE RANGES, PRINTED.  A refusal count says what was rejected; this says
   what the structure IS, which is what a wrong answer needs.  */

static void
lvx_lms_dump_lranges (const auto_vec<rtx_insn *> &seq,
		      const auto_vec<lvx_lms_lrange_part *> &lr, const char *tag)
{
  for (unsigned k = 0; k < lr.length (); k++)
    {
      lvx_lms_lrange_part *g = lr[k];

      if (g->effects.is_empty ())
	continue;
      lvx_lms_dump (";; lvx cms:   %s lr%u r%u+%d root=%d p2=%d uf=%d"
		    " ren=%d xl=%d [%d,%d] regs=", tag, k, g->regno,
		    g->nregs, lvx_lms_lrange_root (lr, (int) k), g->p2unroll,
		    g->use_first ? 1 : 0, g->renamed ? 1 : 0,
		    g->exit_live ? 1 : 0, g->first_date, g->last_date);
      for (int j = 0; j < g->p2unroll && j < LVX_LMS_MAX_KUNROLL; j++)
	lvx_lms_dump ("%sr%d", j ? "," : "", g->reg[j]);
      lvx_lms_dump (" eff=");
      for (unsigned e = 0; e < g->effects.length (); e++)
	lvx_lms_dump ("%s%u/%d%s%s", e ? "," : "", g->effects[e].insn,
		      g->effects[e].slot, g->effects[e].is_def ? "D" : "U",
		      g->effects[e].delayed ? "~" : "");
      lvx_lms_dump ("\n");
    }
  (void) seq;
}

/* OVERLAPPING SPANS MUST BE ONE LIVE RANGE.  §7tricies.33.

   A reference to a register TUPLE covers [regno, regno + nregs), and DF
   reports it as one reference at the first register.  Another instruction may
   reference one of the other registers on its own -- a widening multiply
   writes a pair and the shift that follows reads only its high half -- and
   that opens a SECOND live range inside the first one's span.  Renamed
   independently, the pair moves to a fresh aligned pair while the half moves
   somewhere else, and the two no longer name the same storage.

   So this is an invariant of the structure, checked rather than assumed: two
   live ranges may not overlap in registers.  The reference carries the same
   constraint as UNFOLD_EFFECT_FLAG_ALIASED and a PARENT link per
   sub-register, which lvx_lms_build_lranges did not have.  */

static int
lvx_lms_lrange_overlaps (const auto_vec<lvx_lms_lrange_part *> &lr)
{
  int bad = 0;

  for (unsigned a = 0; a < lr.length (); a++)
    {
      if (lr[a]->effects.is_empty ())
	continue;
      for (unsigned b = a + 1; b < lr.length (); b++)
	{
	  unsigned alo, ahi, blo, bhi;

	  if (lr[b]->effects.is_empty ())
	    continue;
	  if (lvx_lms_lrange_root (lr, (int) a)
	      == lvx_lms_lrange_root (lr, (int) b))
	    continue;		/* one live range, one stack: consistent */
	  alo = lr[a]->regno;
	  ahi = alo + MAX (lr[a]->nregs, 1);
	  blo = lr[b]->regno;
	  bhi = blo + MAX (lr[b]->nregs, 1);
	  if (alo < bhi && blo < ahi && !(alo == blo && ahi == bhi))
	    bad++;		/* a PARTIAL overlap: the defect */
	}
    }
  return bad;
}

/* THE ANTI-DEPENDENCES THE GRAPH DROPPED, CHECKED AGAINST THE ASSIGNMENT.
   §7tricies.38.

   lvx_lms_antidep_real_p drops a register WAR or WAW arc whenever the two
   references belong to different live ranges and at least one of them is renamed,
   on the assumption that modulo expansion separates them.  That assumption is
   about the FINAL assignment -- which register each live range's each name got --
   and nothing on the live range path was checking it: lvx_lms_verify_antidep
   still runs, but over the class arrays, and site.antidep_ok stopped gating
   anything when the class gates went.  This is the check that belongs there.

   IT IS AN OCCUPANCY QUESTION, not a pairwise arc question.  If no register is
   held by two different live ranges in the same cycle of the unrolled kernel,
   then no WAR or WAW between two live ranges can exist to be violated -- the
   second reference simply is not using the first's storage.  So this rebuilds
   the occupancy from SIGMA and the assigned stacks and counts occupants per
   (cycle, register), the same shape as lvx_lms_verify_rename and for the same
   reason: the colouring decided interference from a model it built itself, so
   a bug in that model cannot be caught by the colouring.

   WHAT IT MUST COVER THAT THE COLOURING DOES NOT.  A HELD live range is not in
   the interference graph at all -- lvx_lms_colour_lranges skips every root
   that is not renamed -- so its register is invisible there and is kept out of
   AVAIL instead.  Here it is given an occupancy of its own, so a renamed name
   landing on it is caught rather than assumed away.

   THAT OCCUPANCY IS ITS LIFE AT EVERY ITERATION OFFSET, NOT THE WHOLE PERIOD.
   This paragraph used to say the period, and the code below has not done that
   since §7tricies.93 -- the sentence outlived its own fix and is why the
   fifteen "a dropped anti-dependence is not separated" refusals were first
   read as the verifier over-claiming.  A held range has P2UNROLL 1 by
   construction, so every iteration's instance reuses the one register: the
   occupancy is LIFE cycles at each of the KUNROLL offsets, which is what the
   loop below builds and which makes a held range whose life approaches II
   overlap ITSELF across iterations.  §7tricies.165.

   A VIOLATING PAIR IS REPORTED, not just counted, because fixRelaxed needs to
   know which two live ranges to reinstate an arc between.  */

static bool
lvx_lms_verify_lr_antidep (const auto_vec<rtx_insn *> &seq,
			   const auto_vec<lvx_lms_lrange_part *> &lr,
			   int ii, int lo, int kunroll, int *nconf_out,
			   int *first_a, int *first_b, int *first_reg,
			   int *first_cycle, int *nheld_out, int *ninv_out,
			   int *nocc_out)
{
  int per = kunroll * ii;
  /* ONE COUNT FOR THREE CAUSES WAS THE DEFECT.  §7tricies.92.  A loop that
     fails for register pressure and a loop whose expansion invariant broke
     reported the same way, so neither could be acted on: the invariant is a
     statement about our own arithmetic and must be asserted, while an
     occupancy collision is an allocation outcome an input can reach and must
     be refused.  Counted apart so the difference is visible.  */
  int nheld = 0, ninv = 0, nocc = 0;
  int nconf = 0;

  *nconf_out = *nheld_out = *ninv_out = *nocc_out = 0;
  *first_a = *first_b = *first_reg = *first_cycle = -1;
  if (ii <= 0 || kunroll <= 0 || per > LVX_LMS_IG_MAX_PERIOD)
    return true;		/* not modelled: the caller's other gates */

  /* OCC[cycle][reg] is the live range root holding that register then, or -1.  */
  auto_vec<int> occ;
  occ.safe_grow_cleared (per * 64);
  for (int x = 0; x < per * 64; x++)
    occ[x] = -1;

  for (unsigned k = 0; k < lr.length (); k++)
    {
      lvx_lms_lrange_part *g = lr[k];
      int root, ec, d, life, nr;

      if (g->effects.is_empty ())
	continue;
      root = lvx_lms_lrange_root (lr, (int) k);
      if (root != (int) k)
	continue;		/* one live range, one occupancy */
      nr = MAX (g->nregs, 1);

      ec = g->ec ? 1 : 0;	/* what the COLOURING used.  §7tricies.90 */
      d = g->first_date - lo + (ec ? 0 : 1);
      life = MAX (g->last_date - g->first_date + (ec ? 1 : 0), 1);

      if (!g->renamed)
	{
	  /* A HELD RANGE IS BUSY FOR ITS LIFE, NOT FOR THE PERIOD.
	     §7tricies.93.

	     It used to claim its register for all PER cycles, on the reasoning
	     that a range the expansion cannot rename is simply unavailable.
	     That over-claims by the whole period: two SEQUENTIAL ranges of one
	     hard register then collide at every cycle, though they never
	     overlap and the incoming allocation already proved they do not.
	     Measured: 768 such conflicts across MiBench, every one of them
	     spurious, refusing three loops -- among them

	       lr2 r11+1 p2=1 ren=0 [1,2] eff=1/0D,7/1U
	       lr9 r11+1 p2=1 ren=0 [2,3] eff=7/0D,10/1U

	     where insn 7 reads the first and defines the second, both in r11.

	     The right occupancy is the one the expansion path below already
	     computes, for P2UNROLL of 1: a single name, busy for LIFE cycles
	     at every iteration offset in the period.  So this is the same loop
	     with REGNO as the only name -- no new model, one less
	     approximation -- and it keeps the ec convention, which is what
	     separates the two ranges above into cycles 2 and 3 rather than
	     leaving them adjacent at 2.

	     Still skipping the invariant check below, and that is not an
	     oversight: a held range's P2UNROLL is 1 by construction rather
	     than derived from its LIFE, so LIFE <= P2UNROLL * II is not a
	     statement about it.  §7tricies.92.  */
	  for (int cp = 0; cp < kunroll; cp++)
	    for (int t = 0; t < life; t++)
	      {
		int c = ((d + cp * ii + t) % per + per) % per;

		for (int w = 0; w < nr; w++)
		  {
		    int r = (int) g->regno + w;

		    if (r < 0 || r >= 64)
		      continue;
		    if (occ[c * 64 + r] >= 0 && occ[c * 64 + r] != root)
		      {
			if (nconf == 0)
			  {
			    *first_a = occ[c * 64 + r];
			    *first_b = root;
			    *first_reg = r;
			    *first_cycle = c;
			  }
			nconf++, nheld++;
		      }
		    else
		      occ[c * 64 + r] = root;
		  }
	      }
	  continue;
	}

      /* THE SAME-LIVE RANGE DROP, CHECKED.  §7tricies.79.

	 lvx_lms_antidep_real_p drops a WAR or WAW between two references of
	 ONE live range at OMEGA > 0 on the grounds that the expansion separates
	 the instances.  The occupancy test below cannot see that: it compares
	 pairs of DIFFERENT live ranges, and one live range on its own register is
	 never a conflict, so this class of drop was unverified from the start.

	 It is separated exactly when instance I's range is over before the
	 instance that reuses its register begins, which is LIFE <= P2UNROLL *
	 II.  The colouring chooses P2UNROLL >= ceil (LIFE / II), so this holds
	 whenever the colouring did its job and costs nothing; it fires only if
	 that ever stops being true, and then the loop is refused rather than
	 miscompiled.  */
      if (life > g->p2unroll * ii)
	{
	  /* NOT BAD LUCK WITH A LOOP: OUR OWN ARITHMETIC.  §7tricies.92.
	     P2UNROLL is derived from LIFE to satisfy exactly this, so no
	     input can break it -- unless the derivation was abandoned, which
	     happens in one place only: a range needing more names than
	     LVX_LMS_MAX_KUNROLL has its degree capped, and says so.  That
	     case is a capacity limit an input can reach and is refused
	     below; every other way of arriving here is a bug, and refusing
	     the loop would hide it behind a silently unpipelined loop.  */
	  gcc_assert (g->capped);
	  if (nconf == 0)
	    {
	      *first_a = root;
	      *first_b = root;
	      *first_reg = (int) g->regno;
	      *first_cycle = -1;
	    }
	  nconf++, ninv++;
	}

      /* Name J carries the copies congruent to J modulo P2UNROLL, which is
	 exact because P2UNROLL divides KUNROLL.  */
      for (int j = 0; j < g->p2unroll && j < LVX_LMS_MAX_KUNROLL; j++)
	for (int cp = j; cp < kunroll; cp += g->p2unroll)
	  for (int t = 0; t < life; t++)
	    {
	      int c = ((d + cp * ii + t) % per + per) % per;

	      for (int w = 0; w < nr; w++)
		{
		  int r = g->reg[j] + w;

		  if (r < 0 || r >= 64)
		    continue;
		  if (occ[c * 64 + r] >= 0 && occ[c * 64 + r] != root)
		    {
		      if (nconf == 0)
			{
			  *first_a = occ[c * 64 + r];
			  *first_b = root;
			  *first_reg = r;
			  *first_cycle = c;
			}
		      nconf++, nocc++;
		    }
		  else
		    occ[c * 64 + r] = root;
		}
	    }
    }

  *nconf_out = nconf;
  *nheld_out = nheld;
  *ninv_out = ninv;
  *nocc_out = nocc;
  return nconf == 0;
}

/* IS A REGISTER ANTI-DEPENDENCE WORTH KEEPING?  §7tricies.29.

   The arc is real when the two references END IN THE SAME REGISTER, and the
   partition decides that -- but NOT simply by whether the live range is one.

   ONE LIVE RANGE IS NOT ONE REGISTER.  A live range is a stack of P2UNROLL names,
   and within it a delayed read of iteration L takes index L + use_first - 1
   while the heading definition takes L + use_first: different names as soon
   as the live range expands.  So `one live range' means `one register' only where
   the live range is NOT expanded.

   That is where the circularity sits, and it is why the original code tested
   the instruction's shape instead: expansion is P2UNROLL = ceil (life / II),
   life comes from the schedule and the schedule comes from these arcs.  The
   split that breaks it is the one §7tricies.19 already relies on -- assume
   the expansion while scheduling, then CHECK the result -- and the sound,
   non-circular part of the assumption is exactly this:

     - one live range that is HELD: never expanded, so one register.  Real.
     - two live ranges that both keep the original register: two live ranges on one
       regno, so they still collide.  Real.  (`Keeps the original register' is
       not `not renamed': a renamed USE_FIRST live range's name 0 is PINNED to
       it, because the preheader wrote it, and name 0 comes round every
       P2UNROLL iterations.)
     - one live range that is renamed: dropped here, and whether the expansion
       really separated it is what lvx_lms_verify_antidep asks of the chosen
       SIGMA and the chosen P2 -- the condition being life < p2 * II, which
       p2 = ceil (life / II) gives, so only a live range that came out at p2 = 1
       can fail it.
     - otherwise one of them has moved to a register of its own.  Redundant.

   Two wrong versions of this, both measured rather than reasoned, and the
   measurements are kept because each says something.  As `either side keeps
   its register' (OR instead of AND) it was wrong in the one-held-one-renamed
   case.  As `one live range is real' it kept the anti-dependence of every
   loop-carried register in the body -- the normalisation puts every pre-def
   read into its definition's live range, so every induction variable and every
   accumulator qualified -- which is `assume no expansion at all': nwar 12 ->
   687, 534 further arcs kept, sum II 699 -> 1664 over 124 loops.

   An unresolvable reference keeps the arc.  */

static bool
lvx_lms_lrange_keeps_regno_p (const auto_vec<lvx_lms_lrange_part *> &lr, int root)
{
  /* §7tricies.156: USE_FIRST keeps its register only because name 0 is
     pinned to it.  The preheader copy removes the pin, so a renamed
     use-headed range keeps no register and its anti-dependences are as
     spurious as any other renamed range's.  */
  return !lr[root]->renamed
	 || (lr[root]->use_first
	     && !(lvx_lms_preheader_copy && (lvx_lms_ph_parts & 2)));
}

static bool
lvx_lms_antidep_real_p (const auto_vec<lvx_lms_lrange_part *> &lr,
			lvx_lms_lrange_part *ga, lvx_lms_lrange_part *gb,
			int *why /* [3]: unresolved, one live range, both keep */,
			HOST_WIDE_INT omega, unsigned ia = ~0u,
			unsigned ib = ~0u, bool src_is_def = false)
{
  int ra, rb;

  if (!ga || !gb)
    {
      why[0]++;
      return true;
    }
  ra = lvx_lms_lrange_root (lr, ga->self);
  rb = lvx_lms_lrange_root (lr, gb->self);
  if (ra == rb)
    {
      /* THE EXPANSION SEPARATES ITERATIONS, NOT A SINGLE ITERATION'S OWN
	 REFERENCES.  §7tricies.76.

	 "Renamed, so the expansion separates them" is sound for a
	 LOOP-CARRIED dependence, where the two ends are consecutive
	 instances and the rotation gives them different names.  At OMEGA
	 zero both ends are the SAME instance, so both use reg[index] for
	 the same index and nothing is separated at all.

	 It was unreachable until a sub-span definition became a partial
	 definition of its range (§7tricies.75): before that, two writes
	 within one iteration opened two ranges and took the cross-range
	 path.  Now they are one live range, and dropping their output
	 dependence let two writers of one register share a cycle -- an
	 intra-cycle circuit, which is the symptom that went from nine loops
	 to thirteen.  */
      if (omega == 0)
	return true;
      /* The expansion separates them, which is CHECKED rather than assumed:
	 lvx_lms_verify_lr_antidep requires LIFE <= P2UNROLL * II for every
	 renamed live range, and that inequality is exactly the statement that
	 instance I's range ends before instance I + P2UNROLL reuses its
	 register.  §7tricies.79.

	 Keeping the arc instead is NOT the conservative choice it looks like,
	 and measuring it was the only way to see that: a self-anti-dependence
	 at OMEGA 1 reads sigma (def at I + 1) - sigma (use at I) >= -II, which
	 CAPS EVERY LIVE RANGE'S LIFETIME AT ABOUT II -- precisely what modulo
	 variable expansion exists to escape.  Keeping them took TSVC2 from 27
	 loops at nstages >= 3 to 7, with the built count unmoved, so the cost
	 was invisible in the count and total in the depth.  */
      /* OMEGA IS NOT THE SAME QUESTION AS "DIFFERENT INSTANCES".
	 §7tricies.115.

	 The drop reasons that at OMEGA > 0 the two references belong to
	 iterations I and I + OMEGA, which the expansion puts in different
	 registers.  That holds only when both references index the expansion
	 the same way.  They do not when their DELAYED flags differ: the index
	 is `(iteration + use_first - delayed) & (p2unroll - 1)', so a
	 reference at iteration I + 1 carrying DELAYED lands on the SAME name
	 as one at iteration I without it.

	 THE SHAPE IS THE ROTATION, NOT THE INSTRUCTION.  A write that also
	 READS its destination -- tied, partial or predicated, and the reason
	 does not matter -- standing before the full write that heads the
	 range produces exactly it.  The effects walk enters uses first, so
	 that write's read opens a USE-HEADED range; the full write opens a
	 def-headed one; and the normalisation folds the first into the second
	 with DELAYED set, carrying the earlier WRITE in as a delayed
	 reference.  It then indexes `(i + 1) - 1' against the heading write's
	 `i': one name, two writers, OMEGA 1.  Dropping the arc lets the next
	 iteration's write clobber this one's.

	 Keying on DELAYED rather than on the instruction's shape is what
	 covers all three kinds without enumerating them.

	 The exact test is `(omega - delta delayed) mod p2unroll != 0', and
	 P2UNROLL does not exist here -- it is derived after scheduling.  So
	 the conservative half is taken: where the two references disagree
	 about DELAYED, the arc is kept.  The occupancy verifier cannot stand
	 in for this, since both references share a root and it only reports
	 collisions BETWEEN roots.  */
      if (ia != ~0u && ib != ~0u)
	{
	  bool da = false, db = false, ha = false, hb = false;

	  /* THE REFERENCE THE ARC NAMES, NOT ANY REFERENCE AT THAT INSN.
	     §7tricies.126.  One instruction can hold BOTH a def and a use of
	     one register -- `r4 = r4 + 32' is the ordinary induction variable
	     -- and after the cyclic normalisation the use is DELAYED while the
	     def is not.  Scanning for "an effect at this insn" took whichever
	     came last, compared a def against a use, invented a disagreement
	     and kept the arc.  That is what held saxpy's induction recurrence:
	     its WAR relates the store's READ to the increment's WRITE, both
	     undelayed, and the test saw the increment's own delayed read.

	     A WAR or WAW destination is a write, so IB matches a definition.
	     The source is a read for a WAR and a write for a WAW, which
	     SRC_IS_DEF says.  */
	  for (unsigned e = 0; e < lr[ra]->effects.length (); e++)
	    {
	      if (lr[ra]->effects[e].insn == ia && !ha
		  && lr[ra]->effects[e].is_def == src_is_def)
		da = lr[ra]->effects[e].delayed, ha = true;
	      if (lr[ra]->effects[e].insn == ib && !hb
		  && lr[ra]->effects[e].is_def)
		db = lr[ra]->effects[e].delayed, hb = true;
	    }
	  if (ha && hb && da != db)
	    {
	      why[2]++;
	      return true;
	    }
	}
      if (lr[ra]->renamed)
	return false;
      why[1]++;
      return true;
    }
  /* A KILLING DEF OF A RENAMEABLE RANGE CANNOT BE THE DESTINATION OF A WAR OR
     A WAW.  §7tricies.126.

     CMS runs after register allocation, so every register anti- and output
     dependence is an artefact of the allocator having reused a register --
     not a fact about the dataflow.  GB is the destination at both call sites
     (the WAR pushes src=read dst=write, the WAW pushes src=p dst=q).  If the
     range GB belongs to begins with a killing write and may be renamed, then
     the renaming gives that write a name of its own and the arc never existed
     in the values.  Nothing about II, lifetime or degree enters the
     judgement.

     THE DEGREE IS A CONSEQUENCE, NOT A PRECONDITION, which is what makes this
     sound without knowing P2UNROLL here.  Dropping the arc lets the earlier
     reference slip past the later write; the range's interval then WIDENS to
     cover it, LVX_LMS_SET_LRANGE_DATES measures that on the schedule the drop
     produced, and `ceil (life / II)' yields the second name by itself.  The
     invariant `life <= p2unroll * II' is checked afterwards rather than
     assumed, so the obligation is verified where it is discharged.

     USE_FIRST IS NOT AN EXCEPTION, and treating it as one is what this
     replaces.  The test used to be `both ends keep their architected
     register', with lvx_lms_lrange_keeps_regno_p = `!renamed || use_first'.
     That predicate belongs to §7tricies.26, where the question is which
     registers the colouring may FREE, and there USE_FIRST matters because
     name 0 is pinned to what the preheader wrote.  It is the wrong question
     here: name 0 being pinned does not stop names 1..p2-1 from separating two
     references.  Borrowing it cost saxpy its recurrence -- the ivtmp's
     `r4 = r4 + 32' is a killing def of a renameable range, and its WAR was
     kept for a reason that was never about this arc.

     The source side keeps its old meaning: if the READER has been moved to a
     different register, the arc is equally spurious.  */
  if (!lvx_lms_lrange_keeps_regno_p (lr, ra))
    return false;
  if (lr[rb]->renamed && !lr[rb]->effects.is_empty ()
      && lr[rb]->effects[0].is_def)
    return false;
  why[2]++;
  return true;
}

/* THE REGISTERS THE RANGE MODEL HOLDS.  §7tricies.26.

   lvx_lms_reserved_regs had to count definitions per register and compare the
   total against how many of them belonged to a renamed value, because a value
   did not know its own references: a register was freeable when `every
   definition of it belongs to a value the renaming rewrites', and reads were
   covered only by the separate promise that AMBIG was zero.

   A range enumerates its references, so the rule is the statement itself: a
   register is freeable exactly when EVERY range on it is one the colouring
   rewrites -- def-headed, so it has a stack of names, and not USE_FIRST, whose
   name 0 is pinned to this very register because the preheader wrote it.

   That is not a simplification of the old rule, it is a repair of it.
   store.c's `init' freed r3, which held a renamed value AND was the base of a
   store; the base was defined in the preheader, so it was not in the value
   table at all and the definition count knew nothing about it.  In the range
   model that read opens a def-less range on r3, so r3 has a range the
   colouring does not rewrite and is not freeable.  The case that needed a
   hard-set override and three wrong diagnoses is now the ordinary answer.  */

static void
lvx_lms_lrange_reserved (const auto_vec<rtx_insn *> &seq,
			 const auto_vec<lvx_lms_lrange_part *> &lr,
			 bitmap livein, HARD_REG_SET *reserved,
			 int *pool_out = NULL)
{
  /* Per register: how many ranges live on it, and how many of those the
     colouring will rewrite.  */
  int nrange[FIRST_PSEUDO_REGISTER], nren[FIRST_PSEUDO_REGISTER];
  HARD_REG_SET hard;

  memset (nrange, 0, sizeof (nrange));
  memset (nren, 0, sizeof (nren));

  *reserved = fixed_reg_set;
  for (int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
    if (!df_regs_ever_live_p (r) && !crtl->abi->clobbers_full_reg_p (r))
      SET_HARD_REG_BIT (*reserved, r);
  if (livein)
    {
      bitmap_iterator bi;
      unsigned r;
      EXECUTE_IF_SET_IN_BITMAP (livein, 0, r, bi)
	if (r < FIRST_PSEUDO_REGISTER)
	  SET_HARD_REG_BIT (*reserved, r);
    }
  hard = *reserved;

  for (unsigned i = 0; i < seq.length (); i++)
    {
      df_ref r;
      FOR_EACH_INSN_DEF (r, seq[i])
	if (DF_REF_REGNO (r) < FIRST_PSEUDO_REGISTER)
	  SET_HARD_REG_BIT (*reserved, DF_REF_REGNO (r));
      FOR_EACH_INSN_USE (r, seq[i])
	if (DF_REF_REGNO (r) < FIRST_PSEUDO_REGISTER)
	  SET_HARD_REG_BIT (*reserved, DF_REF_REGNO (r));
    }

  for (unsigned k = 0; k < lr.length (); k++)
    {
      lvx_lms_lrange_part *g = lr[k];
      bool rewritten;

      if (g->effects.is_empty ())
	continue;
      /* A register is freeable only if the LIVE RANGE that holds it is renamed:
	 a range whose union is held keeps its register, and so does a
	 USE_FIRST live range, whose name 0 is pinned to this very register
	 because the preheader wrote it.  */
      rewritten = lr[lvx_lms_lrange_root (lr, (int) k)]->renamed
		  && !lr[lvx_lms_lrange_root (lr, (int) k)]->use_first;
      for (int w = 0; w < MAX (g->nregs, 1); w++)
	if (g->regno + w < FIRST_PSEUDO_REGISTER)
	  {
	    nrange[g->regno + w]++;
	    if (rewritten)
	      nren[g->regno + w]++;
	  }
    }

  for (int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
    if (nrange[r] > 0 && nrange[r] == nren[r])
      CLEAR_HARD_REG_BIT (*reserved, r);

  *reserved |= hard;

  /* THE POOL A SAVE AND A RESTORE WOULD RECOVER.  §7tricies.41.

     A register reserved because it is LIVE-IN but never referenced by the
     body is held for one reason only: something outside the loop is using it.
     Saving it in the preheader and restoring it in the epilogue would hand it
     to the pipeline at the cost of two instructions outside the loop, which a
     loop pays back on its first trip.  Not implemented; measured here so the
     future work is sized rather than asserted, and because it is exactly the
     lever the loops whose II had to rise were short of.  */
  if (pool_out)
    {
      int pool = 0;

      for (int r = 0; r < 64 && r < FIRST_PSEUDO_REGISTER; r++)
	if (TEST_HARD_REG_BIT (*reserved, r)
	    && !TEST_HARD_REG_BIT (fixed_reg_set, r)
	    && nrange[r] == 0
	    && (df_regs_ever_live_p (r) || crtl->abi->clobbers_full_reg_p (r))
	    && livein && bitmap_bit_p (livein, r))
	  pool++;
      *pool_out = pool;
    }
}

/* COLOUR THE RANGES AND FILL THEIR REG[] STACKS.  §7tricies.26.

   lvx_lms_colour is unchanged and takes the same parallel arrays; what changes
   is what one entry IS.  A unit used to be a value -- one definition that
   something reads, identified by the register RA gave it -- then a tie class
   merging the values a tied operand forces together.  It is now a range, and
   the three gates the class model needed go with it:

     - AMBIGUITY, because a reference finds its range by being IN its effects
       list, so two ranges sharing a register are not confusable,
     - the LIVE-OUT refusal, because EXIT_LIVE is a field on the range and the
       epilogue copy is a property of the object rather than a reason to stop,
     - the FMA TIE, because a tied operand's read and write are references to
       the same register in the same instruction and therefore already one
       range -- there is nothing to merge.

   P2UNROLL IS READ BACK, not recomputed.  lvx_lms_build_lranges puts a
   provisional power-of-two round in it so the structural report has something
   to say; what the renaming uses is the number of anodes the colouring
   actually built, which is the discipline that fixed the -mlms-exact-degree
   bug (§7ter.14): a rounding rule in two places goes stale, a count of what
   exists cannot.  */

/* Declared ahead of their definitions: the two accessors over the anode
   arrays, which the value path keeps next to its own rename map.  */
static int lvx_lms_value_p2 (const auto_vec<int> &, int);
static int lvx_lms_anode_of (const auto_vec<int> &, const auto_vec<int> &,
			     int, int);

/* DOES THIS LIVE RANGE CARRY A TIED REFERENCE?  §7tricies.120.

   A tie forces one register on two references, so the range holding them
   cannot be expanded: the definition has `delayed' clear and the tied read
   has it set, and the index computation then hands them different names.
   Asked of the ROOT's whole union, because the register is the union's.  */

/* THE COPY THAT MAKES A CONSTRAINT GROUP ROTATABLE.  §7tricies.140.

   A group with a delayed read cannot rotate (§7tricies.139), so the range
   holding it is capped at one name and the loop is refused whenever its life
   exceeds II.  The life exceeds II because of DOWNSTREAM READERS, not because
   of the group: the group's own read is the previous instance of its own
   definition, dated sigma + II, so on its own it needs exactly one name.

   SO ONE COPY IS ENOUGH, AND IT GOES AFTER.

       acc = fma (a, b, acc)     the group, {o0, o3}, one register, p2 = 1
       t   = acc                 the copy
       ... later ... read t      t rotates freely, untied

   There is no case needing a copy BEFORE it.  The group's read takes its
   value from the group's own definition one period earlier, so the value is
   already in the tied register and never has to be brought in from a rotating
   name -- ranges are per-definition and the normalisation folds the cyclic
   wrap into the one def-headed range (§7tricies.91), so a second definition
   of that register is a different range and cannot feed this read.

   WHAT THIS DECLINES, AND THE PLANNED SECOND HALF.  A reader BEFORE the group
   in body order reads the PREVIOUS iteration's value, so it is not dominated
   by the copy and is left alone today: the copy is only placed where it
   dominates every reader it rewrites.

   The extension is one more copy, in the PREHEADER, and it completes the
   transformation rather than merely widening it:

       preheader:  t = acc          once per loop ENTRY
       body:       ... read t ...   the reader before the group
		   acc = fma (a, b, acc)
		   t = acc             once per ITERATION

   At the top of iteration i, T already holds ACC as of iteration i - 1 --
   written by the body copy at the end of i - 1 -- which is exactly what a
   reader placed before the group wants.  The only instance that copy does not
   cover is the first, and the preheader initialises T with ACC's loop-entry
   value for it.  So the per-iteration cost does not change: the preheader copy
   executes once.

   With it, every reader can be rewritten and the group's range is split
   unconditionally, so `tied AND delayed' stops costing a pipeline at all.
   Not built yet: it needs the preheader CMS's phase A already creates
   (create_preheader with CP_SIMPLE_PREHEADERS) and its own validation, and
   mixing it with the dominated case would have left neither measured.

   Post-RA, so T must be a hard register no part of the loop touches.

   MEASURED AND NOT VIABLE AS WRITTEN -- default off, and these are the three
   reasons, each of which has to be answered before it is turned on:

     1. IT IS TOO EAGER.  The need is known only after colouring, so this
	inserts syntactically -- every group with a downstream reader -- and
	puts in 1352 copies over four suites where the groups that genuinely
	cannot rotate number about five.  The gate has to be the real one
	(tied, delayed, degree would exceed 1), which needs the schedule, so
	the phase wants the bound-transform-re-bound retry the claim
	negotiation and the live-out arc drop both use.  Inserting before the
	graph is right; deciding before the schedule is not.
     2. IT COSTS WHAT IT WAS MEANT TO SAVE.  emitted 949 -> 877 and
	nstages>=3 293 -> 151, because 1352 extra instructions in the kernels
	raise ResMII.
     3. IT BREAKS DOWNSTREAM.  Seven files stop compiling, and the one looked
	at ICEs in GCC's own regrename: "in scan_rtx_reg, at
	regrename.cc:1145" on TSVC2's tsvc.c.  A copy emitted here with a hard
	register the loop does not touch is evidently still not what
	regrename's chain building expects, so the register choice or the
	dataflow update after it is wrong.  */

static int
lvx_lms_free_hard_reg (class loop *loop, basic_block *body, machine_mode mode)
{
  HARD_REG_SET taken;
  df_ref ref;
  rtx_insn *insn;

  CLEAR_HARD_REG_SET (taken);
  for (unsigned b = 0; b < loop->num_nodes; b++)
    {
      bitmap_iterator bi;
      unsigned r;

      EXECUTE_IF_SET_IN_BITMAP (df_get_live_in (body[b]), 0, r, bi)
	if (r < FIRST_PSEUDO_REGISTER)
	  SET_HARD_REG_BIT (taken, r);
      EXECUTE_IF_SET_IN_BITMAP (df_get_live_out (body[b]), 0, r, bi)
	if (r < FIRST_PSEUDO_REGISTER)
	  SET_HARD_REG_BIT (taken, r);
      FOR_BB_INSNS (body[b], insn)
	{
	  if (!INSN_P (insn))
	    continue;
	  FOR_EACH_INSN_DEF (ref, insn)
	    if (DF_REF_REGNO (ref) < FIRST_PSEUDO_REGISTER)
	      SET_HARD_REG_BIT (taken, DF_REF_REGNO (ref));
	  FOR_EACH_INSN_USE (ref, insn)
	    if (DF_REF_REGNO (ref) < FIRST_PSEUDO_REGISTER)
	      SET_HARD_REG_BIT (taken, DF_REF_REGNO (ref));
	}
    }
  for (int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
    if (!TEST_HARD_REG_BIT (taken, r)
	&& !fixed_regs[r] && !global_regs[r]
	&& targetm.hard_regno_mode_ok (r, mode)
	&& end_hard_regno (mode, r) <= FIRST_PSEUDO_REGISTER)
      {
	bool clash = false;

	for (unsigned k = 1; k < hard_regno_nregs (r, mode); k++)
	  if (TEST_HARD_REG_BIT (taken, r + k) || fixed_regs[r + k])
	    clash = true;
	if (!clash)
	  return r;
      }
  return -1;
}

/* The group's output operand, or NULL: the operand some other operand's
   matching constraint names.  §7tricies.139.  */

static rtx *
lvx_lms_group_output (rtx_insn *insn)
{
  if (!INSN_P (insn) || recog_memoized (insn) < 0)
    return NULL;
  extract_insn (insn);
  for (int o = 0; o < recog_data.n_operands; o++)
    {
      const char *c = recog_data.constraints[o];

      /* A matching constraint is a digit naming another operand.  Only
	 single-digit ones occur here and `0' is the only one this port
	 writes, but the digit is read rather than assumed.  */
      for (; c && *c; c++)
	if (ISDIGIT (*c))
	  {
	    int m = *c - '0';

	    if (m >= 0 && m < recog_data.n_operands
		&& recog_data.operand_loc[m]
		&& *recog_data.operand_loc[m]
		&& REG_P (*recog_data.operand_loc[m]))
	      return recog_data.operand_loc[m];
	  }
    }
  return NULL;
}

/* Insert the copies of §7tricies.140 into LOOP's body.  Returns how many went
   in; the caller re-derives everything afterwards, since the body changed.  */

static int
lvx_lms_tied_copies (class loop *loop, basic_block *body)
{
  int ncopy = 0;

  for (unsigned b = 0; b < loop->num_nodes; b++)
    {
      rtx_insn *insn, *next;

      for (insn = BB_HEAD (body[b]); insn; insn = next)
	{
	  rtx *out;
	  unsigned outreg;
	  machine_mode mode;
	  int t;
	  bool before = false, after = false;
	  rtx_insn *j;

	  next = (insn == BB_END (body[b]) ? NULL : NEXT_INSN (insn));
	  if (!NONJUMP_INSN_P (insn))
	    continue;
	  out = lvx_lms_group_output (insn);
	  if (!out || !*out || !REG_P (*out)
	      || REGNO (*out) >= FIRST_PSEUDO_REGISTER)
	    continue;
	  outreg = REGNO (*out);
	  mode = GET_MODE (*out);

	  /* READERS, AND WHICH SIDE THEY ARE ON.  A reader after the group is
	     dominated by the copy and may be rewritten; one before it reads
	     the previous iteration's value and would need the copy's register
	     initialised in the preheader, so its presence declines the
	     loop.  */
	  for (unsigned b2 = 0; b2 < loop->num_nodes; b2++)
	    FOR_BB_INSNS (body[b2], j)
	      {
		df_ref ref;

		if (!INSN_P (j) || j == insn)
		  continue;
		FOR_EACH_INSN_USE (ref, j)
		  if (DF_REF_REGNO (ref) == outreg)
		    {
		      if (b2 == b && DF_INSN_LUID (j) > DF_INSN_LUID (insn))
			after = true;
		      else
			before = true;
		    }
	      }
	  if (!after || before)
	    continue;

	  t = lvx_lms_free_hard_reg (loop, body, mode);
	  if (t < 0)
	    {
	      if (dump_file && lvx_lms_probe)
		lvx_lms_dump (";; lvx cms: fn=%s loop %d tied copy declined,"
			      " no spare hard register for r%u\n",
			      current_function_name (), loop->num, outreg);
	      continue;
	    }

	  /* The copy, then the readers after it.  */
	  emit_insn_after_noloc (gen_rtx_SET (gen_rtx_REG (mode, t),
					      gen_rtx_REG (mode, outreg)),
				 insn, body[b]);
	  for (j = NEXT_INSN (insn); j && j != NEXT_INSN (BB_END (body[b]));
	       j = NEXT_INSN (j))
	    {
	      df_ref ref;

	      if (!INSN_P (j))
		continue;
	      FOR_EACH_INSN_USE (ref, j)
		if (DF_REF_REGNO (ref) == outreg
		    && DF_REF_LOC (ref) && *DF_REF_LOC (ref)
		    && REG_P (*DF_REF_LOC (ref)))
		  validate_change (j, DF_REF_LOC (ref),
				   gen_rtx_REG (GET_MODE (*DF_REF_LOC (ref)),
						t), 1);
	    }
	  if (!apply_change_group ())
	    {
	      if (dump_file && lvx_lms_probe)
		lvx_lms_dump (";; lvx cms: fn=%s loop %d tied copy declined,"
			      " a reader refuses r%d\n",
			      current_function_name (), loop->num, t);
	      continue;
	    }
	  ncopy++;
	  lvx_lms_dump (";; lvx cms: fn=%s loop %d TIEDCOPY r%u -> r%d after"
			" insn %d\n", current_function_name (), loop->num,
			outreg, t, INSN_UID (insn));
	}
    }
  if (ncopy)
    df_analyze ();
  return ncopy;
}

/* A CONSTRAINT GROUP, and the one shape of it that cannot rotate.
   §7tricies.139.

   A CONSTRAINT GROUP is the set of operands of one instruction that the
   TARGET requires to hold the same register.  On this port it is always a
   pair: an output and an input joined by a matching constraint, written `"0"'
   on the input and naming operand 0 --

     (set (match_operand:SF 0 "register_operand" "=r")          ; fmasf4
          (fma:SF (match_operand:SF 1 "register_operand" "r")
                  (match_operand:SF 2 "register_operand" "r")
                  (match_operand:SF 3 "register_operand" "0")))

   -- and the group is {o0, o3}.  The accumulating multiplies, the FMAs and
   the conditional moves all have one; there are 36 matching constraints in
   scalar.md, 43 in builtin.md, 15 in control.md and 9 in vector.md.  THE
   GROUP IS THE RENAMING UNIT, not the operand: give its members different
   registers and `constrain_operands' fails, so the rewritten insn does not
   recognise and the whole loop is declined -- late, after a schedule was
   built on a promise the target never accepts.

   WHICH GROUPS CANNOT ROTATE IS A ONE-LINE CONSEQUENCE OF THE INDEX.  Both
   members belong to the same live range -- a tied operand's read and write
   are references to the same register in the same instruction -- and the
   name each gets is

       (iteration + use_first - delayed) & (p2unroll - 1)

   so the DEF (delayed = false) and the read differ by exactly one name when
   the read is DELAYED and p2unroll >= 2.  They then always disagree and the
   group can never be satisfied.  A read that is NOT delayed lands on the
   def's own index and is fine at any degree.

   So the predicate is `tied AND delayed', not `tied'.

   AND CAPPING ON IT IS STILL WRONG -- all three attempts were, and the
   measurements say why.  Capping denies the range a second name, the
   colouring then assigns one, `life <= p2unroll * ii' fails and the loop is
   REFUSED; `capped' only silences the assert.  So the cap MANUFACTURES the
   refusal it claims to prevent:

     predicate                       fires   emitted (pushed default: 950)
     tied                             2222     584
     tied AND delayed                 1273     693
     tied AND delayed, certain only   1281     568

   while `recog' declines FIVE loops.  Those ~1280 ranges evidently do get
   their names and the target accepts them, so there is nothing to prevent;
   the predicate is kept as the correct STATEMENT of which groups cannot
   rotate, and as the gate for the copy of §7tricies.140, not as a cap.  */

/* TIED FOR CERTAIN, not tied for want of an answer.  §7tricies.141.

   lvx_lms_operand_tied_p answers TRUE on every path where it cannot decide --
   no operand index, no location, the insn does not recognise, the constraints
   do not hold, no alternative chosen, the operand not in the constrained list.
   That is the right default for its original caller, which withholds a
   conservative claim, since not knowing must not license the claim.

   It is the WRONG default for a cap.  Capping a range denies it a second name
   and the capacity invariant then refuses the loop, so "undecidable" would
   cost a pipeline: measured, the conservative predicate fires 2222 times over
   four suites and 1273 even with the DELAYED filter, against the five loops
   `recog' actually declines -- and capping those cost 366 and 257 pipelines
   respectively.

   So this variant answers FALSE when it cannot tell, and TRUE only where a
   matching constraint was positively found on that operand.  A wrong FALSE
   costs nothing: the substitution goes ahead and `recog' refuses it exactly
   as before, which is the behaviour this is trying to improve on rather than
   a regression from it.  */

static bool
lvx_lms_operand_tied_certain_p (rtx_insn *insn, int useidx)
{
  const operand_alternative *alt;
  rtx *loc;

  if (useidx < 0)
    return false;
  loc = lvx_lms_reg_at (&PATTERN (insn), useidx);
  if (loc == NULL)
    return false;
  if (recog_memoized (insn) < 0)
    return false;
  extract_insn (insn);
  preprocess_constraints (insn);	/* §7tricies.150, as above */
  if (!constrain_operands (1, get_enabled_alternatives (insn)))
    return false;
  if (which_alternative < 0)
    return false;
  if (which_alternative >= recog_data.n_alternatives)
    return false;
  alt = which_op_alt ();
  if (alt == NULL)
    return false;
  for (int i = 0; i < recog_data.n_operands; i++)
    if (recog_data.operand_loc[i] == loc)
      return alt[i].matches >= 0 || alt[i].matched >= 0;
  return false;
}

static bool
lvx_lms_lrange_tied_delayed_p (const auto_vec<rtx_insn *> &seq,
			       const auto_vec<lvx_lms_lrange_part *> &lr,
			       int root)
{
  for (unsigned k = 0; k < lr.length (); k++)
    {
      if (lr[k]->effects.is_empty ()
	  || lvx_lms_lrange_root (lr, (int) k) != root)
	continue;
      for (unsigned e = 0; e < lr[k]->effects.length (); e++)
	{
	  unsigned i = lr[k]->effects[e].insn;

	  if (lr[k]->effects[e].is_def
	      || !lr[k]->effects[e].delayed
	      || lr[k]->effects[e].slot == LVX_LMS_SLOT_INDIRECT
	      || i >= seq.length ())
	    continue;
	  if (!lvx_lms_operand_tied_certain_p (seq[i], lr[k]->effects[e].slot))
	    continue;
	  /* AND THE SAME RANGE MUST DEF THAT INSTRUCTION.  §7tricies.144.
	     The group's two members diverge only when BOTH resolve to this
	     range: the def taking name[(iter + uf) & (p2 - 1)] and this
	     delayed read name[(iter + uf - 1) & (p2 - 1)].  Where the def
	     belongs to another range the two names are unrelated and may well
	     agree -- which is why the weaker test also named build_tree,
	     ct_init, _tr_init and _tr_flush_block, every one of which EMITS.
	     Four predicates in, the lesson is that a condition merely
	     CORRELATING with the failure is worth nothing here: it has to be
	     the condition itself.  */
	  for (unsigned k2 = 0; k2 < lr.length (); k2++)
	    {
	      if (lr[k2]->effects.is_empty ()
		  || lvx_lms_lrange_root (lr, (int) k2) != root)
		continue;
	      for (unsigned e2 = 0; e2 < lr[k2]->effects.length (); e2++)
		if (lr[k2]->effects[e2].is_def
		    && lr[k2]->effects[e2].insn == i)
		  return true;
	    }
	}
    }
  return false;
}

static bool
lvx_lms_lrange_tied_p (const auto_vec<rtx_insn *> &seq,
		       const auto_vec<lvx_lms_lrange_part *> &lr, int root)
{
  for (unsigned k = 0; k < lr.length (); k++)
    {
      if (lr[k]->effects.is_empty ()
	  || lvx_lms_lrange_root (lr, (int) k) != root)
	continue;
      for (unsigned e = 0; e < lr[k]->effects.length (); e++)
	{
	  unsigned i = lr[k]->effects[e].insn;

	  if (lr[k]->effects[e].is_def
	      || lr[k]->effects[e].slot == LVX_LMS_SLOT_INDIRECT
	      || i >= seq.length ())
	    continue;
	  if (lvx_lms_operand_tied_p (seq[i], lr[k]->effects[e].slot))
	    return true;
	}
    }
  return false;
}

static bool
lvx_lms_colour_lranges (const auto_vec<rtx_insn *> &seq,
			auto_vec<lvx_lms_lrange_part *> &lr, int ii, int lo,
			unsigned HOST_WIDE_INT avail, int *nn_out,
			int *stuck_out, int *maxreg_out, int *kun_out,
			int *pin_out, int *pinfail_out, int *wide_out,
			int claim_def_cycle = 0,
			auto_vec<int> *stuck_lr = NULL,
			/* §7tricies.156: registers name 0 of a use-headed
			   range may not take, because the entry copy would
			   destroy an incoming value the pipeline still
			   reads.  Zero where there is no entry copy.  */
			unsigned HOST_WIDE_INT entrybusy = 0)
{
  auto_vec<int> idx, def_, last_, deg, nregs_, ec_, vregno, vuf;
  auto_vec<int> av, an, ar, stuck;
  int nn = 0, ne = 0, msq = 0, spl = 0, per = 0, kun = 1, maxreg = -1;
  int pinned = 0, pinfail = 0, wide = 0, unres = 0;

  for (unsigned k = 0; k < lr.length (); k++)
    {
      lvx_lms_lrange_part *g = lr[k];
      int ec, life;
      bool ec_tied_dbg = false, ec_ecp_dbg = false;

      /* ONE UNIT PER LIVE RANGE.  A member that is not the root shares the
	 root's stack and is filled in from it below; a live range that is not
	 renamed holds its register and is not a unit at all.  */
      if (g->effects.is_empty () || !g->renamed
	  || lvx_lms_lrange_root (lr, (int) k) != (int) k)
	continue;
      /* CLAIM_DEF_CYCLE makes every interval [def, last] instead of
	 [def + !earlyclobber, last].  That is not the true interval -- a VLIW
	 reads before it writes -- but it is the one that stops two live ranges
	 being packed into one register across a cycle boundary, which is what
	 makes a cycle's reads and writes unlinearisable (§7tricies.35).  Used
	 only as a retry, so the pressure it costs is paid only where the exact
	 interval cannot be emitted.  */
      /* CLAIM_DEF_CYCLE: 0 none, 1 every range, 2 every range EXCEPT one
	 carrying a tie.  §7tricies.120.

	 The claim lengthens an interval by a cycle so two live ranges cannot
	 be packed into one register across a cycle boundary (§7tricies.35).
	 Inside this model the longer interval genuinely needs a second name
	 when it passes II, so the degree must follow it -- decoupling the two
	 makes `life <= p2unroll * II' false and fires the capacity assert.

	 But a range carrying a TIE cannot take a second name at all, and
	 claiming for it turned five loops' entire pipeline into a late
	 `recog' refusal.  Mode 2 withholds the claim from exactly those
	 ranges, which is LESS conservative and therefore not assumed to be
	 sound: lvx_lms_slot_order is run on the result and the mode is used
	 only if it still linearises.  */
      /* BOTH PREDICATES ASKED ONCE, IN A DEFINED ORDER.  §7tricies.147.

	 lvx_lms_lrange_tied_p and lvx_lms_lrange_ec_p each run
	 extract_insn and constrain_operands, so each LEAVES recog_data and
	 WHICH_ALTERNATIVE pointing at whatever insn it looked at last.  Asked
	 inside one expression they were therefore order-dependent: the `||'
	 evaluated the tie test first and the earlyclobber test second, and
	 asking the same two in the other order produced a DIFFERENT answer --
	 which is how a range with claim=2, tied and not earlyclobber still
	 came out with ec=1, taking a second name it did not need and
	 splitting a constraint group across two registers.

	 Hoisted into locals, so the order is written down rather than implied
	 by `||', and so the dump can print exactly what the decision used.  */
      {
	int root_k = lvx_lms_lrange_root (lr, (int) k);
	bool tied_k = lvx_lms_lrange_tied_p (seq, lr, root_k);
	bool ecp_k = lvx_lms_lrange_ec_p (seq, g);

	ec = ((claim_def_cycle == 1
	       || (claim_def_cycle == 2 && !tied_k))
	      || ecp_k) ? 1 : 0;
	ec_tied_dbg = tied_k;
	ec_ecp_dbg = ecp_k;
      }
      /* STORED, so the verifier checks the ec the colouring actually used.
	 Recomputing it there is what let claim_def_cycle diverge.  */
      g->ec = ec != 0;
      life = g->last_date - g->first_date + (ec ? 1 : 0);
      idx.safe_push ((int) k);
      def_.safe_push (g->first_date);
      /* The OCCUPANCY, which is what the interference edges are built from --
	 "two nodes meet iff their occupancies share a cycle".  DEG below
	 stays on LIFE, so reserving the register longer costs no extra name.
	 §7tricies.137.  */
      last_.safe_push (MAX (g->occ_last, g->last_date));
      /* §7tricies.167: an invariant range keeps ONE name -- that is what
	 §7tricies.91 established and relocation does not change it.  But one
	 name only works while LIFE fits in it, so where it does not the
	 range goes back to being held rather than silently asking the
	 capacity invariant to fail.  */
      deg.safe_push (ii > 0 ? MAX ((life + ii - 1) / ii, 1) : 1);
      /* §7tricies.147: the degree's own inputs, for ranges that will rotate.
	 Deducing EC from the pattern and the claim mode got the wrong answer
	 twice; this prints what the computation actually used.  */
      if (dump_file && lvx_lms_probe && deg.last () > 1)
	lvx_lms_dump (";; lvx cms:   DEG lr%u r%u [%d,%d] ec=%d life=%d ii=%d"
		      " deg=%d claim=%d tied=%d ecp=%d\n", k, g->regno,
		      g->first_date, g->last_date, ec, life, ii, deg.last (),
		      claim_def_cycle,
		      ec_tied_dbg ? 1 : 0, ec_ecp_dbg ? 1 : 0);
      /* §7tricies.160: WHICH REMEDY WOULD THIS RANGE NEED?

	 A range that cannot rotate is forced to one name, and then the
	 capacity invariant demands LIFE <= II.  Over-long means LIFE > II
	 with no rotation available, and WHICH SIDE of the definition the
	 excess lies on decides the remedy (§7tricies.148):

	   LIFE = LAST_DATE - FIRST_DATE, and for a DEF-HEADED range
	   FIRST_DATE *is* the definition's date -- so the excess can only be
	   at the far end, a reader later than def + II.  That is remedy 1,
	   the copy AFTER, and remedy 2 cannot apply to such a range at all.

	 So remedy 2 -- the copy BEFORE, which is what needs a preheader
	 instance to seed iteration 0 -- is specifically for USE-HEADED
	 ranges, where the value arrives from outside the iteration and the
	 excess sits between its arrival and its consumption.  Counted here
	 so the population is measured rather than asserted.  */
      if (dump_file && lvx_lms_probe && ii > 0 && life > ii
	  && !g->renamed)
	lvx_lms_dump (";; lvx cms:   REMEDY lr%u r%u uf=%d life=%d ii=%d"
		      " excess=%d want=%d\n", k, g->regno,
		      g->use_first ? 1 : 0, life, ii, life - ii,
		      g->use_first ? 2 : 1);
      nregs_.safe_push (MAX (g->nregs, 1));
      ec_.safe_push (ec);
      vregno.safe_push ((int) g->regno);
      vuf.safe_push (g->use_first ? 1 : 0);
    }

  /* MORE NAMES DO NOT LOOSEN THE COLOURING.  §7tricies.97.

     A name carries the kernel copies congruent to it modulo its degree, so a
     range of degree 2 in a kernel unrolled 4 times has copies 0 and 2 FORCED
     to share one name; raising every renamed range's degree to KUNROLL makes
     each name carry one copy and the interference graph sparser by that
     factor.  It costs no instruction -- the copies already exist as instances
     in the unrolled kernel -- so it is a renaming and not a split.  It was
     implemented, measured on the loops built in BOTH arms, and removed:

       TSVC2     102 common, II changed on 0, maxreg on 21, NINE loops lost
       PolyBench  17 common, II changed on 0, maxreg on 14, net +34 registers

     Not one loop's II moved.  The colouring does assign differently, so the
     transformation was effective rather than inert; it simply buys nothing,
     because `stuck' and `pinfail' are zero everywhere on these corpora -- the
     colouring never fails, and freedom to avoid a failure cannot pay.  The
     code is in the commit that removed it, for a target whose colouring does
     get stuck.

     P2UNROLL ITSELF IS NOT KUNROLL, even when every reallocatable range ends
     up expanded KUNROLL times: it is the per-range degree the index
     computation masks with, the quantity LIFE <= P2UNROLL * II is stated over,
     and 1 for a held range.

     KEPT AS A CONTROL, default off, for experimenting with a different
     register allocator: the measurement above says the mechanism works and is
     unrewarded HERE, not that it is wrong.  An allocator that does get stuck
     is where it would pay, and that is a property of the allocator rather
     than of this transformation.  */
  if (lvx_lms_force_p2unroll && !deg.is_empty ())
    {
      int mx = 1, k2 = 1, forced = 0;

      for (unsigned u = 0; u < deg.length (); u++)
	mx = MAX (mx, deg[u]);
      while (k2 < mx && k2 < LVX_LMS_MAX_KUNROLL)
	k2 <<= 1;
      for (unsigned u = 0; u < deg.length (); u++)
	{
	  if (deg[u] >= k2)
	    continue;
	  /* Setting 1 spares the ranges that pay: which name holds a live-out
	     value depends on the rotation its exit came from, so raising their
	     degree multiplies the drain's reconciliation copies -- once per
	     loop ENTRY, not per iteration.  */
	  if (lvx_lms_force_p2unroll < 2 && lr[idx[u]]->exit_live)
	    continue;
	  deg[u] = k2, forced++;
	}
      /* `fn=' IS LOAD-BEARING.  Every harness here filters dump lines on it,
	 so a line without it is invisible -- which is how this counter read
	 zero while the forcing was firing, and why the option first looked
	 inert.  */
      if (forced && lvx_lms_probe)
	lvx_lms_dump (";; lvx cms: fn=%s forcep2 kunroll=%d forced=%d of %u\n",
		      current_function_name (), k2, forced, deg.length ());
    }

  /* §7tricies.113: when the CTRL_LIVEOUT arcs have been dropped, an exit-live
     range has PROMISED to write a name of its own, so the architected register
     is excluded from its choices.  Indexed like VREGNO, by value.  A range
     that is also USE_FIRST is left alone -- its name 0 is pinned to the
     register the preheader wrote, which is an external constraint the promise
     cannot override, and such a range keeps its arc.  */
  /* THE COPY-DELETING PREFERENCE.  §7tricies.122.  Indexed like VREGNO, by
     value: the register this value would like, because taking it deletes the
     drain's copy back to the architected register.  Only where the colouring
     can know that -- an exit-live range at degree one, whose single name is
     what the drain reads whichever iteration it drains from.  A range
     excluded from its own register by VNOTOWN is left at -1: that exclusion
     is a correctness constraint for the live-out drop and outranks a
     code-size preference.  */
  auto_vec<int> vprefer;
  vprefer.safe_grow_cleared (idx.length ());
  for (unsigned u = 0; u < idx.length (); u++)
    {
      lvx_lms_lrange_part *g = lr[idx[u]];

      vprefer[u] = (g->exit_live && !g->effects.is_empty ()
		    && g->effects[0].is_def
		    && u < deg.length () && deg[u] == 1)
		   ? (int) g->regno : -1;
    }

  /* §7tricies.158: which use-headed ranges may lose their pin, by ordinal
     within this loop, so one can be held back at a time.  */
  auto_vec<int> vunpin;
  vunpin.safe_grow_cleared (idx.length ());
  {
    int ord = 0;

    for (unsigned u = 0; u < idx.length (); u++)
      if (lr[idx[u]]->use_first)
	{
	  vunpin[u] = (ord < 32
		       && (lvx_lms_ph_mask >> ord) & 1) ? 1 : 0;
	  ord++;
	}
  }

  auto_vec<int> vnotown;
  if (lvx_lms_liveout_split)
    {
      vnotown.safe_grow_cleared (idx.length ());
      for (unsigned u = 0; u < idx.length (); u++)
	{
	  lvx_lms_lrange_part *g = lr[idx[u]];

	  /* §7tricies.156: and so must the exclusion that makes the promise
	     satisfiable, or the colouring would hand a use-headed range its
	     own register and the verifier would then refuse the loop.  */
	  vnotown[u] = (g->exit_live
			&& (!g->use_first
			    || (lvx_lms_preheader_copy
				&& (lvx_lms_ph_parts & 8)))
			&& !g->effects.is_empty () && g->effects[0].is_def)
		       ? (int) g->regno : -1;
	  if (vnotown[u] >= 0)
	    vprefer[u] = -1;	/* the exclusion outranks the preference */
	}
    }

  lvx_lms_colour (def_, last_, deg, nregs_, ec_, ii, lo, &nn, &ne, &msq,
		  &spl, &per, &kun, &stuck, &av, &an, &ar, &maxreg, avail,
		  &vregno, &vuf, &pinned, &pinfail,
		  lvx_lms_liveout_split ? &vnotown : NULL,
		  lvx_lms_copy_pref ? &vprefer : NULL, entrybusy, &vunpin);

  for (unsigned u = 0; u < idx.length (); u++)
    {
      lvx_lms_lrange_part *g = lr[idx[u]];
      int p2 = lvx_lms_value_p2 (av, (int) u);

      for (int j = 0; j < LVX_LMS_MAX_KUNROLL; j++)
	g->reg[j] = (int) g->regno;
      if (p2 > LVX_LMS_MAX_KUNROLL)
	{
	  /* More names than the stack can hold.  Not a correctness choice
	     to make quietly: the range would silently wrap onto a name it
	     does not own, which is the 3-does-not-divide-4 bug again.  */
	  wide++;
	  g->p2unroll = 1;
	  g->capped = true;
	  continue;
	}
      g->p2unroll = MAX (p2, 1);
      for (int j = 0; j < p2; j++)
	{
	  int q = lvx_lms_anode_of (av, an, (int) u, j);
	  if (q >= 0 && ar[q] >= 0)
	    g->reg[j] = ar[q];
	  else
	    /* COUNTED, NOT DUMPED.  The feasibility test calls this per
	       candidate schedule, so a per-name line here is hundreds of
	       lines of noise per loop; the count is what the caller acts
	       on.  */
	    unres++;
	}
    }

  /* THE LIVE RANGE'S STACK IS EVERY MEMBER'S STACK.  Copied rather than followed
     through the parent at lookup time: the index arithmetic then reads one
     range and one range only, which is what keeps lvx_lms_lrange_reg the
     single place a register is decided.  */
  for (unsigned k = 0; k < lr.length (); k++)
    {
      int root = lvx_lms_lrange_root (lr, (int) k);

      if (root == (int) k)
	continue;
      lr[k]->p2unroll = lr[root]->p2unroll;
      lr[k]->ec = lr[root]->ec;
      lr[k]->capped = lr[root]->capped;
      lr[k]->use_first = lr[root]->use_first;
      lr[k]->renamed = lr[root]->renamed;
      for (int j = 0; j < LVX_LMS_MAX_KUNROLL; j++)
	lr[k]->reg[j] = lr[root]->reg[j];
    }

  *nn_out = nn;
  *stuck_out = (int) stuck.length ();
  *maxreg_out = maxreg;
  *kun_out = kun;
  *pin_out = pinned;
  *pinfail_out = pinfail;
  /* A NAME WITH NO COLOURED ANODE IS NOT A FALLBACK TO THE ORIGINAL REGISTER.
     REG[] is seeded with the original regno so that a held live range reads
     correctly, but for a RENAMED live range an unresolved name leaves two of its
     names sharing one register -- no expansion where the span needs it -- and
     the occupancy check then finds the live range conflicting with whatever else
     holds that register.  Counted and refused rather than emitted.  */
  *wide_out = wide + unres;
  if (stuck_lr)
    {
      /* STUCK holds UNIT indices; the caller wants live ranges.  */
      stuck_lr->truncate (0);
      for (unsigned q = 0; q < stuck.length (); q++)
	if (stuck[q] >= 0 && stuck[q] < (int) idx.length ()
	    && !stuck_lr->contains (idx[stuck[q]]))
	  stuck_lr->safe_push (idx[stuck[q]]);
    }
  return nn > 0 && stuck.is_empty () && pinfail == 0 && wide == 0
	 && unres == 0;
}

/* THE RANGE A REFERENCE BELONGS TO.  Mirrors
   unfold_kernel_find_rename_range: a definition matches a range whose FIRST
   effect is this instruction, a use matches one with a later effect on it.
   That is what tells two ranges in one register apart, and it is the whole of
   what §7tricies.22's gate was standing in for.  */

static lvx_lms_lrange_part *
lvx_lms_lrange_of (const auto_vec<lvx_lms_lrange_part *> &lr, unsigned insn,
		   int slot, bool *delayed_out, int *offset_out,
		   int *nregs_out)
{
  if (slot < 0)
    return NULL;
  for (unsigned k = 0; k < lr.length (); k++)
    {
      lvx_lms_lrange_part *g = lr[k];

      for (unsigned e = 0; e < g->effects.length (); e++)
	if (g->effects[e].insn == insn && g->effects[e].slot == slot)
	  {
	    if (delayed_out)
	      *delayed_out = g->effects[e].delayed;
	    /* THE SUB-OPERAND'S OFFSET, which the renaming adds to the
	       live range's name (§7tricies.71).  */
	    if (offset_out)
	      *offset_out = g->effects[e].offset;
	    if (nregs_out)
	      *nregs_out = MAX (g->effects[e].nregs, 1);
	    return g;
	  }
    }
  return NULL;
}

/* The same, by register and side, for the places that have an operand only as
   `the destination of this insn' -- the tie pass reading recog_data.  Keyed on
   the heading definition, which is unique per range.  */

/* A RANGE COVERS A SPAN, AND THE ARCS ASK ABOUT ITS COMPONENTS.  §7tricies.125.

   A live range on a tuple is registered under its BASE: `r8+4' spans r8 to
   r11 and its REGNO is 8.  The dependence graph, though, builds register WAR
   and WAW arcs one COMPONENT at a time, because that is how DF reports them --
   so an arc on r9 asked for "the range whose regno is 9", found none, and the
   caller read the NULL as "unresolvable reference, keep the arc".

   Matching the base exactly therefore lost every anti-dependence drop on all
   but the FIRST register of every tuple.  Scalar code never noticed; a
   vectorized loop loses three components in four.  In saxpy, vectorized to
   V8SF across r8-r11 and r32-r35, fourteen arcs were kept this way -- the
   whole of its `akunres' -- and one of them closes the recurrence that pins
   II at 7 against a ResMII of 2.

   The span test is what the range already means everywhere else: EFFECTS
   carry an OFFSET within the span (§7tricies.71) precisely so that a
   narrower reference belongs to the wider range.  */

static lvx_lms_lrange_part *
lvx_lms_lrange_def_of (const auto_vec<lvx_lms_lrange_part *> &lr, unsigned insn,
		       unsigned regno)
{
  for (unsigned k = 0; k < lr.length (); k++)
    {
      if (lr[k]->effects.is_empty ()
	  || regno < lr[k]->regno
	  || regno >= lr[k]->regno + (unsigned) MAX (lr[k]->nregs, 1))
	continue;
      /* ANY DEFINITION IN THE RANGE, NOT ONLY THE ONE THAT HEADS IT.
	 §7tricies.125.  A read-modify-write does not open a range -- that is
	 the whole point of the in-place rule -- so its WRITE is an effect in
	 the middle of the range its read already belongs to.  Requiring
	 EFFECTS[0] made every such write unfindable, and the caller again
	 read NULL as "unresolvable, keep the arc".  Resolving it instead
	 returns the SAME range for both endpoints, which is the "one live
	 range" answer §7tricies.29 wants: two writes that share a register by
	 construction, whose order is real.  */
      for (unsigned e = 0; e < lr[k]->effects.length (); e++)
	if (lr[k]->effects[e].is_def && lr[k]->effects[e].insn == insn)
	  return lr[k];
    }
  return NULL;
}

/* And by register for a READ, where the caller knows the register but not the
   operand: the range holding a non-definition effect on INSN.  Several reads
   of one register in one insn are several operands and may be several
   live ranges, so this answers only where there is exactly one; it returns null
   rather than a guess otherwise.  */

static lvx_lms_lrange_part *
lvx_lms_lrange_use_of (const auto_vec<lvx_lms_lrange_part *> &lr, unsigned insn,
		       unsigned regno)
{
  lvx_lms_lrange_part *found = NULL;

  for (unsigned k = 0; k < lr.length (); k++)
    {
      if (lr[k]->regno != regno)
	continue;
      for (unsigned e = 0; e < lr[k]->effects.length (); e++)
	if (lr[k]->effects[e].insn == insn && !lr[k]->effects[e].is_def)
	  {
	    if (found && found != lr[k])
	      return NULL;
	    found = lr[k];
	    break;
	  }
    }
  return found;
}

/* The register a range's copy for ITERATION lives in.  The one index
   computation, and the only place the renaming decides anything.  */

static int
lvx_lms_lrange_reg (lvx_lms_lrange_part *g, int iteration, bool delayed)
{
  int index = (iteration + (g->use_first ? 1 : 0) - (delayed ? 1 : 0))
	      & (g->p2unroll - 1);
  return g->reg[index];
}

/* CHECK THE STRUCTURE: every reference of every body instruction must land on
   exactly one range.  A reference on none would be renamed by nothing; one on
   two would be the ambiguity this is meant to retire.  */

static bool
lvx_lms_verify_lranges (const auto_vec<rtx_insn *> &seq,
			const auto_vec<lvx_lms_lrange_part *> &lr,
			int *nrange_out, int *orphan_out, int *dup_out,
			int *noloc_out)
{
  int orphan = 0, dup = 0, noloc = 0;

  /* A LIVE RANGE IS A CONTIGUOUS SUBLIST OF ONE REGISTER'S PARTS.
     §7tricies.119.

     LR is appended to as the walk meets each opening reference, so a part's
     index is its position in program order, and the measured shape of every
     union -- 114 of them over 275 loops of TSVC2, PolyBench and MiBench --
     is a CONSECUTIVE RUN of the parts of one register.  That is a stronger
     and far more checkable statement than the root rule it implies, and it
     is what makes the positional facts positional: the run's LAST part heads
     the live range, only its FIRST part can be use-headed (a definition
     opens every later one), and the parts of one register partition into
     runs that ARE its live ranges.

     Asserted, because a violation is a design error and not bad luck with a
     loop: a union with a hole would mean two values on one register joined
     around a third, and the merged interval would then span a live range
     that is not in the union.

     The one case deliberately NOT asserted is a union across two different
     registers, which the tie pass can in principle build.  That is sound and
     held -- §7tricies.71 marks it `noname' -- rather than wrong, so it is
     excluded here instead of trapped.  It has never been observed to
     occur.  */
  for (unsigned k = 0; k < lr.length (); k++)
    {
      int lo = -1, hi = -1;
      bool onereg = true;

      if (lr[k]->effects.is_empty ()
	  || lvx_lms_lrange_root (lr, (int) k) != (int) k)
	continue;
      for (unsigned j = 0; j < lr.length (); j++)
	if (!lr[j]->effects.is_empty ()
	    && lvx_lms_lrange_root (lr, (int) j) == (int) k)
	  {
	    if (lo < 0)
	      lo = (int) j;
	    hi = (int) j;
	    if (lr[j]->regno != lr[k]->regno)
	      onereg = false;
	  }
      if (!onereg || lo < 0)
	continue;
      for (int j = lo + 1; j < hi; j++)
	gcc_assert (lr[j]->effects.is_empty ()
		    || lr[j]->regno != lr[k]->regno
		    || lvx_lms_lrange_root (lr, j) == (int) k);
    }

  /* EVERY OPERAND ON EXACTLY ONE LIVE RANGE.  Asked of DF rather than of the
     builder: for each reference of each body instruction, resolve the operand
     it sits at and count the LIVE RANGES that claim it.  Two ranges of one
     live range answering is not a defect -- they share a stack, so they give the
     same register -- which is why the count is over roots and not ranges.

     An operand DF gives no location for is counted apart.  It is not an
     orphan (nothing is wrong with the structure) and not renameable either,
     so the rewrite has to refuse it; conflating the two hid the question.  */
  for (unsigned i = 0; i < seq.length (); i++)
    {
      df_ref ref;

      FOR_EACH_INSN_DEF (ref, seq[i])
	{
	  unsigned r = DF_REF_REGNO (ref);
	  int slot, nroot = 0, root = -1;

	  if (r >= FIRST_PSEUDO_REGISTER)
	    continue;
	  slot = lvx_lms_reg_index (PATTERN (seq[i]), DF_REF_LOC (ref));
	  if (slot < 0)
	    {
	      noloc++;
	      continue;
	    }
	  for (unsigned k = 0; k < lr.length (); k++)
	    for (unsigned e = 0; e < lr[k]->effects.length (); e++)
	      if (lr[k]->effects[e].insn == i
		  && lr[k]->effects[e].slot == slot)
		{
		  int rt = lvx_lms_lrange_root (lr, (int) k);

		  if (root < 0)
		    {
		      root = rt;
		      nroot = 1;
		    }
		  else if (rt != root)
		    nroot++;
		  break;
		}
	  if (nroot == 0)
	    orphan++;
	  else if (nroot > 1)
	    dup++;
	}

      FOR_EACH_INSN_USE (ref, seq[i])
	{
	  unsigned r = DF_REF_REGNO (ref);
	  int slot, nroot = 0, root = -1;

	  if (r >= FIRST_PSEUDO_REGISTER)
	    continue;
	  slot = lvx_lms_reg_index (PATTERN (seq[i]), DF_REF_LOC (ref));
	  if (slot < 0)
	    {
	      noloc++;
	      continue;
	    }
	  for (unsigned k = 0; k < lr.length (); k++)
	    for (unsigned e = 0; e < lr[k]->effects.length (); e++)
	      if (lr[k]->effects[e].insn == i
		  && lr[k]->effects[e].slot == slot)
		{
		  int rt = lvx_lms_lrange_root (lr, (int) k);

		  if (root < 0)
		    {
		      root = rt;
		      nroot = 1;
		    }
		  else if (rt != root)
		    nroot++;
		  break;
		}
	  if (nroot == 0)
	    orphan++;
	  else if (nroot > 1)
	    dup++;
	}
    }

  int live = 0;
  for (unsigned k = 0; k < lr.length (); k++)
    if (!lr[k]->effects.is_empty ())
      live++;

  *nrange_out = live;
  *orphan_out = orphan;
  *dup_out = dup;
  *noloc_out = noloc;
  return orphan == 0 && dup == 0;
}

/* THE RENAME MAP.  §7ter.14, and the first half of make_insns.

   Modulo expansion IS the renaming, so this is the part of codegen that
   cannot be deferred: there is no useful subset of make_insns that moves
   instructions without it (§7quinquies.8 records the measurement -- kunroll
   is 1 on only 5 of 162 loops, and no loop shape with kunroll == 1 also has
   nstages >= 2, so the restricted class where renaming is unnecessary is
   empty).  It is also where the KVX implementation hit its ICEs, which is why
   the map is built and CHECKED as data here, before any RTL is touched.

   WHAT THE MAP IS.  Rau's scheme, and it is one line: value V's copy for
   iteration L lives in the register given to V's name (L mod P2(V)).  Name J
   carries the iterations congruent to J modulo P2(V) -- which is exactly what
   lvx_lms_colour's occupancy construction assumed when it built the
   interference graph, so the two cannot disagree without one of them being
   rewritten.  A definition in instance L of instruction OWNER(V) writes that
   register; a use in instance L of a consumer reading V across OMEGA
   iterations reads the copy iteration L - OMEGA wrote, so its name is
   (L - OMEGA) mod P2(V).

   P2 IS NOT PASSED IN; it is the number of anodes the colouring made for the
   value.  Recomputing it from DEG and -mlms-exact-degree would be six lines
   duplicating lvx_lms_colour, and a duplicate of a rounding rule is a thing
   that goes stale -- this cannot, because it counts what the allocator
   actually built.  A value with no anode has P2 == 0 and is not renamed at
   all: it keeps the register RA gave it.  */

/* Value V's expansion degree as the allocator built it, and the anode index of
   its name J, or -1.  Linear in the anode count, which is bounded by
   LVX_LMS_IG_MAX_NODES; called per operand, so the loops are the cost of not
   keeping a parallel index that could disagree with the list it indexes.  */

static int
lvx_lms_value_p2 (const auto_vec<int> &anode_value, int v)
{
  int n = 0;
  for (unsigned q = 0; q < anode_value.length (); q++)
    if (anode_value[q] == v)
      n++;
  return n;
}

static int
lvx_lms_anode_of (const auto_vec<int> &anode_value,
		  const auto_vec<int> &anode_name, int v, int name)
{
  for (unsigned q = 0; q < anode_value.length (); q++)
    if (anode_value[q] == v && anode_name[q] == name)
      return (int) q;
  return -1;
}

/* The register value V's iteration-L copy lives in, or -1 to mean "leave the
   operand alone".  Returns the original register for an unexpanded value, so
   a caller can rewrite unconditionally without asking whether a value was
   renamed -- which is what keeps the emission free of a special case that
   could be got wrong in one direction only.  */

static int
lvx_lms_rename_regno (const auto_vec<int> &anode_value,
		      const auto_vec<int> &anode_name,
		      const auto_vec<int> &anode_regno,
		      const auto_vec<int> &vregno, int v, int l)
{
  int p2 = lvx_lms_value_p2 (anode_value, v);
  if (p2 <= 0)
    return v < (int) vregno.length () ? vregno[v] : -1;
  /* Floored modulo: L - OMEGA goes negative in the prologue, where a use
     reaches back past iteration zero.  C's % would give a negative name and
     index nothing.  */
  int name = ((l % p2) + p2) % p2;
  int q = lvx_lms_anode_of (anode_value, anode_name, v, name);
  return q < 0 ? -1 : anode_regno[q];
}

/* VERIFY THE ASSIGNMENT, independently of the graph that produced it.

   The colouring is a squeeze over an interference graph it built itself, so a
   bug in the occupancy construction cannot be caught by the colouring: the
   same wrong bitmaps decide both who interferes and who is safe.  This
   rebuilds the occupancy from the schedule and asks the only question that
   matters at the point of emission -- is any register written by two values
   that are live at the same cycle of the unrolled kernel -- by counting
   occupants per (cycle, register) rather than by consulting any edge.

   RESERVED is the hole the colouring structurally cannot see, and the first
   attempt at measuring it was dead code: it skipped a value whose adjusted
   life collapses to zero using the very test lvx_lms_colour skips it with, so
   the counter could never fire and reported zero for the wrong reason.

   The hole is larger than that value class anyway.  The interference graph
   contains exactly the values that get an expansion name; it does not contain

     - a value whose adjusted life is zero, which still CLAIMS its register at
       the cycle it is written, even though the model gives it no occupancy
       (the consumer reads in the cycle the producer writes, which a VLIW
       permits -- so the range is empty but the write is not),
     - a definition with no reader inside the loop at all, which
       lvx_lms_collect_values drops before the graph is built,
     - every register the body merely reads: an address base, a loop-invariant,
       anything live across the loop.

   None of those is renamed, so each holds its register for the whole kernel,
   and a renamed anode handed that register destroys it.  The fix is in AVAIL,
   which codegen must pass as the allocatable set MINUS the registers the body
   touches and the renaming does not replace.  RESERVED counts that set and
   CLASH counts the anodes landing in it, which makes the figure a measurement
   instead of an assumption -- and makes it impossible to run emission while
   believing the default AVAIL of all-ones is safe, because it is not.

   AMBIGUITY is the other thing emission needs decided here.  To rewrite a use
   the emitter must find the operand reading the value, and it finds it by
   original register number.  If two distinct values reaching one consumer
   carry the same original regno -- RA reusing a register -- the operand is not
   determined and the loop must be declined rather than guessed at.  */

static bool
lvx_lms_verify_rename (const auto_vec<rtx_insn *> &seq,
		       const auto_vec<lvx_lms_arc> &arcs,
		       const auto_vec<int> &def_, const auto_vec<int> &last_,
		       const auto_vec<int> &nregs_, const auto_vec<int> &ec_,
		       const auto_vec<int> &owner, const auto_vec<int> &vregno,
		       const auto_vec<int> &vusefirst, bitmap livein,
		       const auto_vec<int> &anode_value,
		       const auto_vec<int> &anode_name,
		       const auto_vec<int> &anode_regno,
		       int ii, int lo, int kunroll,
		       int *conflicts_out, int *unseen_out, int *ambig_out,
		       int *unassigned_out, int *misaligned_out,
		       int *nondiv_out, int *reserved_out, int *clash_out)
{
  int conflicts = 0, unseen = 0, ambig = 0, unassigned = 0, misaligned = 0;
  int nondiv = 0, nreserved = 0, clash = 0;
  int per = kunroll * ii;

  *conflicts_out = *unseen_out = *ambig_out = 0;
  *unassigned_out = *misaligned_out = *nondiv_out = 0;
  *reserved_out = *clash_out = 0;
  if (per <= 0 || per > LVX_LMS_IG_MAX_PERIOD)
    return false;

  /* Occupants per (cycle, register) over the unrolled period.  Rebuilt from
     the schedule, which is the point: the colouring's own bitmaps cannot
     check themselves.  */
  auto_vec<int> occ, occ_unseen;
  occ.safe_grow_cleared (per * FIRST_PSEUDO_REGISTER);
  /* Whether a (cycle, register) already holds something the interference
     graph did not contain.  Needed to attribute a collision: charging it on
     the CURRENT occupant alone counts a clash with a model-invisible value as
     a colouring bug, which is the conflation this check exists to avoid.  */
  occ_unseen.safe_grow_cleared (per * FIRST_PSEUDO_REGISTER);

  /* RESERVED comes from lvx_lms_reserved_regs, the same set the caller used
     to build AVAIL -- recomputing it here would let the check and the
     colouring disagree about what is held, which is the one disagreement this
     function exists to make impossible.  */
  HARD_REG_SET reserved;
  lvx_lms_reserved_regs (seq, def_, last_, nregs_, ec_, vregno, vusefirst,
			 livein, &reserved);
  for (int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
    if (TEST_HARD_REG_BIT (reserved, r))
      nreserved++;

  for (unsigned v = 0; v < def_.length (); v++)
    {
      int d = def_[v] - lo + (ec_[v] ? 0 : 1);
      /* Same convention as lvx_lms_colour: [def + !earlyclobber, last].  A
	 check that models the schedule differently from the allocator tests a
	 different program.  */
      int life = last_[v] - def_[v] + (ec_[v] ? 1 : 0);
      int nr = MAX (nregs_[v], 1);
      int p2 = lvx_lms_value_p2 (anode_value, (int) v);

      /* THE INVARIANT THE WHOLE SCHEME RESTS ON: P2 divides KUNROLL, so that
	 the kernel's K-th copy is always name K mod P2.  Reported as its own
	 defect because it is the root cause and the register conflicts are
	 only its symptom -- diagnosing it from the conflicts cost a wrong
	 hypothesis first.  */
      if (p2 > 0 && kunroll % p2 != 0)
	nondiv++;

      for (int k = 0; k < kunroll; k++)
	{
	  int reg;
	  bool seen;

	  if (p2 > 0)
	    {
	      int q = lvx_lms_anode_of (anode_value, anode_name, (int) v,
					k % p2);
	      reg = q < 0 ? -1 : anode_regno[q];
	      seen = true;
	      if (reg < 0)
		{
		  unassigned++;
		  continue;
		}
	      /* A tuple's base must be a multiple of its width, which is what
		 lvx_pgr_reg_name and lvx_qgr_reg_name assert and what the
		 select phase is supposed to have enforced.  Checked here
		 because an assert in the printer fires long after the choice
		 that was wrong.  */
	      if (nr > 1 && reg % nr != 0)
		misaligned++;
	    }
	  else
	    {
	      /* No expansion name: the register RA gave it, which the graph
		 never contained.  Its occupancy is empty under the model --
		 the consumer reads in the cycle the producer writes -- but the
		 WRITE is not empty, so it claims the definition cycle and is
		 entered with a single-cycle range to make that visible.  */
	      reg = (int) v < (int) vregno.length () ? vregno[v] : -1;
	      seen = false;
	      if (reg < 0)
		continue;
	    }

	  if (reg + nr > FIRST_PSEUDO_REGISTER)
	    continue;

	  /* MAX (life, 1) and the start at D, exactly as lvx_lms_colour clamps
	     them: a check that models the schedule differently from the
	     allocator tests a different program.  */
	  for (int t = 0; t < MAX (life, 1); t++)
	    {
	      int c = (d + k * ii + t) % per;
	      for (int w = 0; w < nr; w++)
		{
		  int at = c * FIRST_PSEUDO_REGISTER + reg + w;
		  if (occ[at])
		    {
		      /* A colouring bug only when BOTH occupants were nodes
			 the graph contained; otherwise the colouring was
			 never given the chance to separate them.  */
		      if (seen && !occ_unseen[at])
			{
			  conflicts++;
			  /* WHERE, not just how many.  A count cannot be
			     checked against the schedule it describes, and
			     reasoning about this one from the count alone
			     went in a circle once already.  */
			  if (conflicts <= 6)
			    lvx_lms_dump (";; lvx cms:   rename conflict"
					  " cycle=%d reg=r%d v=%u k=%d"
					  " seen=%d p2=%d life=%d\n",
					  c, reg + w, v, k, (int) seen, p2,
					  life);
			}
		      else
			{
			  unseen++;
			  if (unseen <= 4)
			    lvx_lms_dump (";; lvx cms:   rename unseen"
					  " cycle=%d reg=r%d v=%u k=%d"
					  " seen=%d p2=%d life=%d orig=r%d"
					  " resv=%d\n", c, reg + w, v, k,
					  (int) seen, p2, life,
					  v < vregno.length () ? vregno[v] : -1,
					  (int) TEST_HARD_REG_BIT (reserved,
								   reg + w));
			}
		    }
		  occ[at]++;
		  if (!seen)
		    occ_unseen[at] = 1;
		}
	    }
	}
    }

  /* CLASH: anodes landing on a register the body holds for something the
     renaming does not rewrite.  Once per anode, which is once per bad choice
     -- counting it per unroll copy would multiply one wrong register by
     KUNROLL and make the figure unreadable against ANODES.  */
  for (unsigned q = 0; q < anode_regno.length (); q++)
    {
      int reg = anode_regno[q];
      int v = anode_value[q];
      int nr = v >= 0 && v < (int) nregs_.length () ? MAX (nregs_[v], 1) : 1;
      if (reg < 0)
	continue;
      /* A USE_FIRST value's name 0 IS the original register and the reserved
	 set keeps it on purpose (§7ter.4), so it is not a clash.  Without this
	 exclusion CLASH read 1 or 2 on every loop and matched the USE_FIRST
	 count exactly -- a counter that reports the intended case as a defect
	 is worse than no counter, because the number it prints is never zero
	 and so never alarms.  */
      if (anode_name[q] == 0 && v >= 0 && v < (int) vusefirst.length ()
	  && vusefirst[v])
	continue;
      for (int w = 0; w < nr; w++)
	if (reg + w < FIRST_PSEUDO_REGISTER
	    && TEST_HARD_REG_BIT (reserved, reg + w))
	  {
	    clash++;
	    break;
	  }
    }

  /* AMBIGUITY: two values with the same original regno reaching one consumer.
     Walked over the arcs rather than over the values, because it is the
     consumer's operand that is or is not determined.  */
  for (unsigned j = 0; j < seq.length (); j++)
    for (unsigned a = 0; a < arcs.length (); a++)
      {
	if (arcs[a].kind != LVX_ARC_REG_RAW || arcs[a].dst != j)
	  continue;
	for (unsigned b = a + 1; b < arcs.length (); b++)
	  {
	    if (arcs[b].kind != LVX_ARC_REG_RAW || arcs[b].dst != j
		|| arcs[b].src == arcs[a].src)
	      continue;
	    int va = -1, vb = -1;
	    for (unsigned v = 0; v < owner.length (); v++)
	      {
		if (owner[v] == (int) arcs[a].src)
		  va = (int) v;
		if (owner[v] == (int) arcs[b].src)
		  vb = (int) v;
	      }
	    if (va < 0 || vb < 0)
	      continue;
	    if (vregno[va] >= 0 && vregno[va] == vregno[vb])
	      ambig++;
	  }
      }

  *conflicts_out = conflicts;
  *unseen_out = unseen;
  *ambig_out = ambig;
  *unassigned_out = unassigned;
  *misaligned_out = misaligned;
  *nondiv_out = nondiv;
  *reserved_out = nreserved;
  *clash_out = clash;
  /* CLASH and RESERVED are reported, not gated on: AVAIL defaults to all-ones
     in the probe, so a clash there is the expected consequence of asking for
     an assignment without telling the colouring what is taken.  What gates is
     the part that would be wrong however AVAIL is set.  */
  return conflicts == 0 && unseen == 0 && ambig == 0 && unassigned == 0
	 && misaligned == 0 && nondiv == 0;
}

/* RENAME CLASSES: values a tied operand forces to share a register.
   §7tricies.11.

   §7tricies.10 found that every refused substitution in the population is an
   `fma', whose addend is tied to its destination.  A tie is not a preference
   the allocator expressed; it is the machine saying the two operands ARE one
   register.  So the producer's value and the consumer's value cannot be given
   different expansion names, and the unit of renaming is not the value but
   the class a tie closes over.

   DETECTED BY ASKING THE TARGET, one arc at a time.  The first attempt used
   the register numbers -- a tie that RA satisfied leaves the consumer's
   destination holding the same register as the producer it reads -- and that
   test OVER-MERGES badly: it also fires on every coincidental reuse along a
   RAW edge, which is common.  Measured, it collapsed 841 values into 548
   classes across 94 of 124 loops, where only 41 loops have a refusal at all
   and only 4.7% of substitutions are refused.  Merging on a coincidence is
   not wrong, but it gives up an expansion copy the machine does not forbid,
   and at that rate it would give up most of them.

   So each incoming arc's substitution is tried ALONE, with the register the
   map actually assigns, and the pair is merged only when `insn_invalid_p'
   refuses it.  That is precise by construction: the classes are exactly the
   pairs the target will not let be renamed apart.  It costs one
   verify_changes per arc per slot, which is nothing beside being right.

   The class is a union-find over values; a class's range is the union of its
   members' ranges, which is what makes the merge cost something: a longer
   range means a higher degree and possibly more names.  That cost is the
   point of measuring it before rewiring the colouring onto it.  */

static int
lvx_lms_tie_find (auto_vec<int> &parent, int v)
{
  while (parent[v] != v)
    {
      parent[v] = parent[parent[v]];
      v = parent[v];
    }
  return v;
}

/* Would substituting value PV's register for NEU in INSN's uses be accepted?
   One arc's worth of change, asked and cancelled.  */

static bool
lvx_lms_subst_use_ok_p (rtx_insn *insn, int old, int neu)
{
  rtx pat = PATTERN (insn);
  rtx set = single_set (insn);
  int base = num_validated_changes ();
  bool ok = true;

  if (old < 0 || neu < 0 || old == neu)
    return true;

  subrtx_ptr_iterator::array_type array;
  FOR_EACH_SUBRTX_PTR (it, array, &pat, NONCONST)
    {
      rtx *loc = *it;
      if (!*loc || !REG_P (*loc) || (int) REGNO (*loc) != old)
	continue;
      if (set && loc == &SET_DEST (set))
	continue;
      validate_change (insn, loc, gen_rtx_REG (GET_MODE (*loc), neu), 1);
    }
  if (num_validated_changes () > base)
    ok = verify_changes (base);
  cancel_changes (base);
  return ok;
}

static void
lvx_lms_tie_classes (const auto_vec<rtx_insn *> &seq,
		     const auto_vec<lvx_lms_arc> &arcs,
		     const auto_vec<int> &owner,
		     const auto_vec<int> &vregno,
		     const auto_vec<int> &anode_value,
		     const auto_vec<int> &anode_name,
		     const auto_vec<int> &anode_regno,
		     auto_vec<int> &parent, int *nclass_out,
		     int *nmerged_out, int *biggest_out, int *orphan_out)
{
  unsigned n = owner.length ();
  int orphan = 0;

  parent.truncate (0);
  for (unsigned v = 0; v < n; v++)
    parent.safe_push ((int) v);

  for (unsigned a = 0; a < arcs.length (); a++)
    {
      if (arcs[a].kind != LVX_ARC_REG_RAW)
	continue;
      int pv = -1, cv = -1;
      for (unsigned v = 0; v < n; v++)
	{
	  if (owner[v] == (int) arcs[a].src)
	    pv = (int) v;
	  if (owner[v] == (int) arcs[a].dst)
	    cv = (int) v;
	}
      if (pv < 0 || arcs[a].dst >= seq.length ())
	continue;

      /* The register this use would become at some iteration where the name
	 differs from the original.  Name 1 exists whenever the value expands
	 at all; a value with one name is never renamed and cannot be tied
	 apart from anything.  */
      int old = vregno[pv];
      int neu = lvx_lms_rename_regno (anode_value, anode_name, anode_regno,
				      vregno, pv, 1);
      if (old < 0 || neu < 0 || neu == old)
	continue;
      if (lvx_lms_subst_use_ok_p (seq[arcs[a].dst], old, neu))
	continue;

      /* Refused.  The consumer's own value must share this register -- and
	 when the consumer owns no value there is nothing to merge with, which
	 is a loop codegen has to decline rather than repair.  */
      if (cv < 0 || pv == cv)
	{
	  orphan++;
	  continue;
	}
      int rp = lvx_lms_tie_find (parent, pv);
      int rc = lvx_lms_tie_find (parent, cv);
      if (rp != rc)
	parent[rc] = rp;
    }

  int nclass = 0, biggest = 0;
  auto_vec<int> size;
  size.safe_grow_cleared (n);
  for (unsigned v = 0; v < n; v++)
    size[lvx_lms_tie_find (parent, (int) v)]++;
  for (unsigned v = 0; v < n; v++)
    if (lvx_lms_tie_find (parent, (int) v) == (int) v)
      {
	nclass++;
	if (size[v] > biggest)
	  biggest = size[v];
      }

  *nclass_out = nclass;
  *nmerged_out = (int) n - nclass;
  *biggest_out = biggest;
  *orphan_out = orphan;
}

/* CAN EVERY SLOT BE RENAMED AND STILL RECOGNISE?  §7tricies.10.

   The rename map says which register each instance's operands become
   (§7tricies.1) and lvx_lms_verify_rename says the assignment is consistent.
   Neither asks whether the INSTRUCTION survives the substitution.  Post-RA it
   need not: an operand's constraint may admit only a subset of the register
   file, a tuple's alternative may require an aligned base, and a pattern may
   match on the register class rather than on the mode.  A substitution that
   fails those does not produce wrong code -- it produces an unrecognisable
   insn, which is an ICE in the next pass that looks at it, and arrives with
   no trace of the choice that caused it.

   Asked here as a DRY RUN, before a block exists to emit into, because the
   answer is a property of the schedule and the assignment alone.  The
   validate_change / verify_changes / cancel_changes trio is GCC's own way to
   ask "would this change be legal" -- verify_changes runs insn_invalid_p over
   every touched insn and cancel_changes puts the insn back exactly as it was,
   so nothing is modified and no copy has to be built to ask.

   DEFS AND USES ARE REWRITTEN SEPARATELY, from SET_DEST and from the rest of
   the pattern, because one register can be both: an induction variable
   updated in place has its own regno on both sides, and its def takes name
   L while its use takes name L - OMEGA.  Walking the whole pattern with one
   substitution would pick whichever it met first.  */

static bool
lvx_lms_rename_insn_p (rtx_insn *insn, int iter,
		       const auto_vec<lvx_lms_arc> &arcs,
		       const auto_vec<int> &insn2cls,
		       const auto_vec<int> &cregno,
		       const auto_vec<int> &anode_value,
		       const auto_vec<int> &anode_name,
		       const auto_vec<int> &anode_regno,
		       unsigned self, int *nsubst, bool apply)
{
  rtx pat = PATTERN (insn);
  rtx set = single_set (insn);
  int base = num_validated_changes ();
  bool ok = true;

  /* THE DEFINITION.  The rename unit this instruction defines into, if any,
     moves to its iteration's name.  Addressed by CLASS rather than by value,
     because a tied operand makes two values one register (§7tricies.11) and
     the unit of renaming is therefore the class.  */
  if (set && REG_P (SET_DEST (set)))
    {
      int c = self < insn2cls.length () ? insn2cls[self] : -1;
      int old = c >= 0 ? cregno[c] : -1;
      int neu = c >= 0 ? lvx_lms_rename_regno (anode_value, anode_name,
					      anode_regno, cregno, c, iter)
			: -1;
      if (old >= 0 && neu >= 0 && neu != old
	  && (int) REGNO (SET_DEST (set)) == old)
	{
	  validate_change (insn, &SET_DEST (set),
			   gen_rtx_REG (GET_MODE (SET_DEST (set)), neu), 1);
	  (*nsubst)++;
	}
    }

  /* THE USES.  One pass per incoming RAW arc, since each names a different
     class and a different distance.  */
  for (unsigned a = 0; a < arcs.length (); a++)
    {
      if (arcs[a].kind != LVX_ARC_REG_RAW || arcs[a].dst != self)
	continue;
      int pv = arcs[a].src < insn2cls.length () ? insn2cls[arcs[a].src] : -1;
      if (pv < 0)
	continue;
      int old = cregno[pv];
      int pl = iter - (int) arcs[a].omega;
      if (old < 0)
	continue;
      /* An instance reaching back past iteration zero reads what the
	 preheader left, which is the original register (§7ter.4).  */
      if (pl < 0)
	continue;
      int neu = lvx_lms_rename_regno (anode_value, anode_name, anode_regno,
				      cregno, pv, pl);
      if (neu < 0 || neu == old)
	continue;
      /* NO SKIP WHEN THE USE IS IN THE CONSUMER'S OWN CLASS.  The first
	 version skipped exactly that case, reasoning about double-rewriting,
	 and it is the one case that must NOT be skipped: a tie puts the use
	 and the destination in one class on purpose, so rewriting the
	 destination and leaving the use is how the tie gets broken.  It made
	 the class colouring score worse than the value colouring it was meant
	 to fix -- 81 loops against 83 -- which is what said the merge was not
	 the thing that was wrong.

	 At OMEGA = 0 both sides take name L and land on the same register, so
	 the tie survives.  At OMEGA >= 1 they take different names and it
	 cannot: such a class must not expand at all, which is handled where
	 the class degree is computed rather than here.  */

      /* THE OPERAND THE ARC NAMES, not every REG with that number.  The arc
	 recorded which reference reads this class when the graph was built
	 (§7tricies.24), so there is nothing to disambiguate: one arc, one
	 operand.  An arc without a locator -- DF gave no DF_REF_LOC -- cannot
	 be rewritten precisely, and the loop is refused rather than guessed
	 at.  */
      {
	rtx *loc = lvx_lms_reg_at (&pat, arcs[a].useidx);

	if (!loc)
	  {
	    ok = false;
	    break;
	  }
	if ((int) REGNO (*loc) != old)
	  continue;
	validate_change (insn, loc, gen_rtx_REG (GET_MODE (*loc), neu), 1);
	(*nsubst)++;
      }
    }

  /* APPLY OR CANCEL, one function and not two.  The dry run and the real
     substitution must agree about every operand they touch, and the only way
     to be sure of that is for there to be one piece of code -- a separate
     emitter would be a second rounding rule, which §7tricies.1 already
     records as the thing that goes stale.  */
  if (num_validated_changes () > base)
    {
      ok = verify_changes (base);
      if (ok && apply)
	{
	  confirm_change_group ();
	  return true;
	}
    }
  cancel_changes (base);
  return ok;
}

/* REWRITE ONE INSTANCE'S REGISTERS FROM THE RANGES.  §7tricies.27.

   ORIG is the body instruction DF knows about; TARGET is the instruction to
   change -- the same insn for the dry run, the emitted copy for the real
   thing.  They have to be separate arguments because copy_insn produces an
   insn DF has never seen: the references are resolved on the original and
   applied to the copy BY POSITION, lvx_lms_reg_index giving a reference's
   ordinal in the pattern's REG walk and lvx_lms_reg_at recovering that slot.

   EVERY REFERENCE IS REWRITTEN, AND EACH FROM ITS OWN RANGE.  The arc-driven
   version could only rewrite what an arc named -- one pass per incoming RAW
   arc -- so a read the value model had no arc for was left alone, and the
   definition had to be addressed by tie class because a value did not know
   which operand was its own.  DF enumerates the references and the range owns
   them, so there is nothing left to match up and no gate standing in for the
   match.

   THE INDEX IS THE WHOLE DECISION:

       (iteration + use_first - delayed) & (p2unroll - 1)

   and two special cases the arc version carried fall out of it.  A delayed
   reference at iteration 0 lands on index 0, which for a USE_FIRST range is
   the register the colouring pinned -- the one the preheader wrote -- so
   `an instance reaching back past iteration zero reads the original register'
   needs no test.  And an in-place update's read and write are one range
   differing only by `delayed', so at p2unroll 1 they agree and the tie
   survives with nothing said about ties.  */

/* Probe only (§7tricies.156): which node the emission is filling, so the
   UFREAD trace can be read per path instead of per reference.  */
static int lvx_lms_ufread_node = -1;
static int lvx_lms_ufread_kind = -1;

/* The ROOT of the live range G belongs to.  The renaming sites hold the part
   pointer and not its index, and only the root identifies the live range --
   §7tricies.98's part-versus-whole distinction, which is why this is not just
   `g' itself.  */
static int
lvx_lms_flow_root_of (const auto_vec<lvx_lms_lrange_part *> &lr,
		      const lvx_lms_lrange_part *g)
{
  for (unsigned z = 0; z < lr.length (); z++)
    if (lr[z] == g)
      return lvx_lms_lrange_root (lr, (int) z);
  return -1;
}

static bool
lvx_lms_rename_lr_insn_p (rtx_insn *orig, rtx_insn *target, unsigned self,
			  int iter, const auto_vec<lvx_lms_lrange_part *> &lr,
			  int *nsubst, bool apply, bool trace0 = false)
{
  /* §7tricies.156: the option enables the same trace from every call site,
     so the emission's substitutions can be diffed between two arms without
     threading a flag through each caller.  */
  bool trace = trace0 || lvx_lms_subst_trace;
  int base = num_validated_changes ();
  auto_vec<int> at, to;
  bool ok = true;
  df_ref ref;

  FOR_EACH_INSN_DEF (ref, orig)
    {
      /* THE OPERAND'S BASE, not the reference's word: DF reports a tuple
	 once per register, and the comparison below is against the register
	 the substitution would replace -- the whole operand's.  */
      rtx *rloc = DF_REF_LOC (ref);
      rtx rreg = (rloc && *rloc && REG_P (*rloc)) ? *rloc
						  : DF_REF_REG (ref);
      unsigned r = (rreg && REG_P (rreg)) ? REGNO (rreg)
					  : DF_REF_REGNO (ref);
      lvx_lms_lrange_part *g;
      int idx, neu, off = 0;

      if (r >= FIRST_PSEUDO_REGISTER)
	continue;
      idx = lvx_lms_reg_index (PATTERN (orig), DF_REF_LOC (ref));
      g = lvx_lms_lrange_of (lr, self, idx, NULL, &off);
      if (!g)
	{
	  if (dump_file && lvx_lms_probe)
	    lvx_lms_dump (";; lvx cms:   RENREFUSE norange-def insn=%u"
			  " slot=%d\n", self, idx);
	  ok = false;
	  break;
	}
      neu = lvx_lms_lrange_reg (g, iter, false);
      /* The live range names its whole span; this reference is at OFF within
	 it, and keeps its own mode -- so its width -- below.  */
      if (neu >= 0)
	neu += off;
      /* RECORDED BEFORE THE SKIP BELOW.  §7tricies.209: an operand whose name
	 happens to equal the register it already had needs no substitution and
	 is just as much part of the dataflow, so the flow check must see it.  */
      /* ONLY THE WALK THAT EMITS.  §7tricies.211.  `apply' is false for
	 lvx_lms_rename_lr_dryrun, which the claim negotiation runs up to
	 three times per loop and two further callers run again -- throwaway
	 plans whose insns are never emitted and never reach the group table.
	 Recording them made FLOWTAB read `operands=772 checked=163', so the
	 check looked to cover a fifth of the dataflow when it in fact covered
	 nearly all of what exists.  */
      /* THE RANGE'S WIDTH, AND THE OPERAND'S WAS TRIED AND IS WORSE.
	 §7tricies.213: recording `hard_regno_nregs' of the reference's own
	 mode instead took trees.c from 16 false positives to 160, so the
	 narrower width is not the answer even though the range's width is
	 demonstrably too wide -- "insn 1013 reads r8 AND r9 expecting lr1"
	 where one register is read.  Left at the measured better of the two,
	 with the residual characterised rather than guessed at again.  */
      if (apply && target != NULL && neu >= 0)
	lvx_lms_flow_record (target, lvx_lms_flow_root_of (lr, g),
			     neu, MAX (g->nregs, 1), true, false,
			     g->p2unroll);
      if (neu < 0 || neu == (int) r)
	continue;
      if (trace)
	lvx_lms_dump (";; lvx cms:     SUBST node=%d kind=%d insn=%u iter=%d"
		      " D slot=%d dly=0 r%u p2=%d uf=%d -> r%d\n",
		      lvx_lms_ufread_node, lvx_lms_ufread_kind, self, iter,
		      idx, r, g->p2unroll, g->use_first ? 1 : 0, neu);
      if (idx < 0)
	{
	  if (dump_file && lvx_lms_probe)
	    lvx_lms_dump (";; lvx cms:   RENREFUSE noidx-def insn=%u\n",
			  self);
	  ok = false;
	  break;
	}
      at.safe_push (idx);
      to.safe_push (neu);
    }

  if (ok)
    FOR_EACH_INSN_USE (ref, orig)
      {
	rtx *rloc = DF_REF_LOC (ref);
	rtx rreg = (rloc && *rloc && REG_P (*rloc)) ? *rloc
						    : DF_REF_REG (ref);
	unsigned r = (rreg && REG_P (rreg)) ? REGNO (rreg)
					    : DF_REF_REGNO (ref);
	lvx_lms_lrange_part *g;
	bool dly = false;
	int idx, neu, off = 0;

	if (r >= FIRST_PSEUDO_REGISTER)
	  continue;
	idx = lvx_lms_reg_index (PATTERN (orig), DF_REF_LOC (ref));
	g = lvx_lms_lrange_of (lr, self, idx, &dly, &off);
	if (!g)
	  {
	    /* An operand no live range claims.  With the read-modify-write
	       operand entered once, the only way here is a reference DF gave
	       no location for, which cannot be rewritten precisely.  */
	    if (dump_file && lvx_lms_probe)
	      lvx_lms_dump (";; lvx cms:   RENREFUSE norange-use insn=%u"
			    " slot=%d\n", self, idx);
	    ok = false;
	    break;
	  }
	neu = lvx_lms_lrange_reg (g, iter, dly);
	/* §7tricies.209: the read side of the flow record, recorded with the
	   offset applied just below so it names the component actually read. */
	if (neu >= 0)
	  neu += off;
	if (apply && target != NULL && neu >= 0)
	  lvx_lms_flow_record (target, lvx_lms_flow_root_of (lr, g),
			       neu, MAX (g->nregs, 1), false, dly,
			       g->p2unroll);
	if (g->use_first && dly && lvx_lms_preheader_copy)
	  lvx_lms_dump (";; lvx cms:     UFREAD node=%d kind=%d insn=%u"
			" slot=%d iter=%d r%u p2=%d reg0=%d -> r%d\n",
			lvx_lms_ufread_node, lvx_lms_ufread_kind, self, idx,
			iter, r, g->p2unroll, g->reg[0], neu);
	if (neu < 0 || neu == (int) r)
	  continue;
	if (trace)
	  lvx_lms_dump (";; lvx cms:     SUBST node=%d kind=%d insn=%u iter=%d"
			" U slot=%d dly=%d r%u p2=%d uf=%d -> r%d\n",
			lvx_lms_ufread_node, lvx_lms_ufread_kind, self, iter,
			idx, dly ? 1 : 0, r, g->p2unroll,
			g->use_first ? 1 : 0, neu);
	if (idx < 0)
	  {
	    if (dump_file && lvx_lms_probe)
	      lvx_lms_dump (";; lvx cms:   RENREFUSE noidx-use insn=%u\n",
			    self);
	    ok = false;
	    break;
	  }
	at.safe_push (idx);
	to.safe_push (neu);
      }

  /* ONE SLOT, ONE REGISTER.  DF reports a read-modify-write reference as both
     a definition and a use AT THE SAME LOCATION; it cannot take two names,
     and applying both would silently keep whichever came second.  Refusing is
     the honest answer and it is countable.  It should also be rare, because
     the normalisation puts an in-place update's read and write in one range
     where they agree unless that range expands.  */
  for (unsigned a = 0; a < at.length () && ok; a++)
    for (unsigned b = a + 1; b < at.length (); b++)
      if (at[a] == at[b] && to[a] != to[b])
	{
	  /* §7tricies.137.  Counted apart from the recog refusal below,
	     because the two say different things: THIS is one RTL location
	     that DF reports as both a definition and a use -- an in-place
	     update -- being asked for two names, which only happens once its
	     range EXPANDS; recog refusing is the pattern rejecting a register
	     it was given.  Lumping them hid which of the two the live-out
	     interval extension (§7tricies.135) was paying for.  */
	  if (dump_file && lvx_lms_probe)
	    lvx_lms_dump (";; lvx cms:   RENREFUSE rmw insn=%u slot=%d\n",
			  self, at[a]);
	  ok = false;
	  break;
	}

  if (ok)
    {
      rtx pat = PATTERN (target);

      for (unsigned a = 0; a < at.length (); a++)
	{
	  rtx *loc = lvx_lms_reg_at (&pat, at[a]);

	  if (!loc || !REG_P (*loc))
	    {
	      if (dump_file && lvx_lms_probe)
		lvx_lms_dump (";; lvx cms:   RENREFUSE noloc insn=%u slot=%d\n",
			      self, at[a]);
	      ok = false;
	      break;
	    }
	  validate_change (target, loc,
			   gen_rtx_REG (GET_MODE (*loc), to[a]), 1);
	  (*nsubst)++;
	}
    }

  /* Apply or cancel, the same trio as the value path: one piece of code for
     the dry run and the emission, so they cannot disagree.  */
  if (ok && num_validated_changes () > base)
    {
      ok = verify_changes (base);
      if (!ok && dump_file && lvx_lms_probe)
	{
	  int code = INSN_CODE (target);

	  lvx_lms_dump (";; lvx cms:   RENREFUSE recog insn=%u pat=%s subst=",
			self,
			(code >= 0 ? insn_data[code].name : "unrecognised"));
	  for (unsigned a = 0; a < at.length (); a++)
	    lvx_lms_dump ("%sslot%d->r%d", a ? "," : "", at[a], to[a]);

	  /* THE CONSTRAINT GROUP, NAMED.  §7tricies.139.  A late `recog'
	     decline is a renaming rule the upstream did not know, so the
	     report has to say WHICH rule: for every operand, the register it
	     holds, the matching constraint it is under, and the register this
	     substitution would give it.  Reading the slot numbers and
	     guessing at the tie is how the first three accounts of this went
	     wrong -- the slot is a positional register index from
	     lvx_lms_reg_index, not the match_operand number, and the two do
	     not agree (fmasf4 ties operand 3 to 0, *cmov ties operand 4).  */
	  /* EVERY OPERAND, INCLUDING THE SKIPPED ONES, WITH ITS RANGE.
	     §7tricies.144.

	     The substitution list above shows only the operands that MOVED.
	     An operand is skipped when the name its range gives equals the
	     register it already holds (`neu == r'), and for a constraint group
	     that is exactly the interesting case: one member renamed, the
	     other pinned, the group split across two registers.  So the report
	     has to name, per DF reference, which live RANGE owns it and what
	     that range would give it -- which is the question the slot numbers
	     cannot answer.  */
	  {
	    df_ref r2;

	    lvx_lms_dump (" refs=");
	    FOR_EACH_INSN_DEF (r2, orig)
	      {
		int ix = lvx_lms_reg_index (PATTERN (orig), DF_REF_LOC (r2));
		int off2 = 0;
		lvx_lms_lrange_part *g2
		  = lvx_lms_lrange_of (lr, self, ix, NULL, &off2);
		int nm2 = g2 ? lvx_lms_lrange_reg (g2, iter, false) : -1;

		lvx_lms_dump ("D%d:r%u", ix, DF_REF_REGNO (r2));
		if (g2)
		  {
		    int k2 = -1;

		    for (unsigned z = 0; z < lr.length (); z++)
		      if (lr[z] == g2)
			k2 = (int) z;
		    lvx_lms_dump ("[lr%d root%d p2=%d uf=%d ren=%d ->r%d]", k2,
				  k2 >= 0 ? lvx_lms_lrange_root (lr, k2) : -1,
				  g2->p2unroll, g2->use_first ? 1 : 0,
				  g2->renamed ? 1 : 0,
				  nm2 >= 0 ? nm2 + off2 : -1);
		  }
		else
		  lvx_lms_dump ("[norange]");
		lvx_lms_dump (" ");
	      }
	    FOR_EACH_INSN_USE (r2, orig)
	      {
		int ix = lvx_lms_reg_index (PATTERN (orig), DF_REF_LOC (r2));
		int off2 = 0;
		bool dly2 = false;
		lvx_lms_lrange_part *g2
		  = lvx_lms_lrange_of (lr, self, ix, &dly2, &off2);
		int nm2 = g2 ? lvx_lms_lrange_reg (g2, iter, dly2) : -1;

		lvx_lms_dump ("U%d:r%u", ix, DF_REF_REGNO (r2));
		if (g2)
		  {
		    int k2 = -1;

		    for (unsigned z = 0; z < lr.length (); z++)
		      if (lr[z] == g2)
			k2 = (int) z;
		    lvx_lms_dump ("[lr%d root%d p2=%d uf=%d ren=%d dly=%d"
				  " ->r%d]", k2,
				  k2 >= 0 ? lvx_lms_lrange_root (lr, k2) : -1,
				  g2->p2unroll, g2->use_first ? 1 : 0,
				  g2->renamed ? 1 : 0, dly2 ? 1 : 0,
				  nm2 >= 0 ? nm2 + off2 : -1);
		  }
		else
		  lvx_lms_dump ("[norange]");
		lvx_lms_dump (" ");
	      }
	  }

	  if (code >= 0)
	    {
	      /* The STATIC constraints, not the chosen alternative: the
		 substitution is what made `constrain_operands' fail, so
		 asking it again answers nothing.  A `0' here is the matching
		 constraint, and the operand it names must hold the same
		 register.  */
	      extract_insn (target);
	      lvx_lms_dump (" ops=");
	      for (int o = 0; o < recog_data.n_operands; o++)
		{
		  rtx x = *recog_data.operand_loc[o];

		  lvx_lms_dump ("%so%d:", o ? " " : "", o);
		  if (x && REG_P (x))
		    lvx_lms_dump ("r%u", REGNO (x));
		  else
		    lvx_lms_dump ("-");
		  lvx_lms_dump ("\"%s\"",
				recog_data.constraints[o]
				? recog_data.constraints[o] : "");
		}
	    }
	  lvx_lms_dump ("\n");
	}
      if (ok && apply)
	{
	  confirm_change_group ();
	  return true;
	}
    }
  cancel_changes (base);
  return ok;
}

/* Every slot, dry-run, over the ranges.  */

static bool
lvx_lms_rename_lr_dryrun (const auto_vec<rtx_insn *> &seq,
			  const auto_vec<lvx_lms_slot> &slots,
			  const auto_vec<lvx_lms_lrange_part *> &lr,
			  int *subst_out, int *fail_out,
			  auto_vec<int> *refused = NULL)
{
  int subst = 0, fail = 0;

  if (refused)
    refused->truncate (0);
  for (unsigned q = 0; q < slots.length (); q++)
    {
      int n = 0;
      unsigned i = (unsigned) slots[q].insn;

      if (i >= seq.length ())
	continue;
      if (!lvx_lms_rename_lr_insn_p (seq[i], seq[i], i, slots[q].iter, lr,
				     &n, false))
	{
	  fail++;
	  if (refused && !refused->contains ((int) i))
	    refused->safe_push ((int) i);
	  if (fail <= 2)
	    {
	      int code = INSN_CODE (seq[i]);
	      int n2 = 0;

	      lvx_lms_dump (";; lvx cms:   lrrename refused insn=%u iter=%d"
			    " uid=%u subst=%d pattern=%s\n", i, slots[q].iter,
			    INSN_UID (seq[i]), n,
			    code >= 0 ? get_insn_name (code) : "?");
	      /* Again, tracing every reference.  A refusal count says only
		 that something was rejected; which reference took which name
		 is what says WHY, and guessing at that cost four wrong
		 diagnoses earlier in this work.  */
	      lvx_lms_rename_lr_insn_p (seq[i], seq[i], i, slots[q].iter, lr,
					&n2, false, true);
	    }
	}
      subst += n;
    }

  *subst_out = subst;
  *fail_out = fail;
  return fail == 0;
}

/* Every slot, dry-run.  Reports how many instances needed a substitution and
   how many of those the target would refuse.  */

static bool
lvx_lms_rename_dryrun (const auto_vec<rtx_insn *> &seq,
		       const auto_vec<lvx_lms_arc> &arcs,
		       const auto_vec<lvx_lms_slot> &slots,
		       const auto_vec<int> &insn2cls,
		       const auto_vec<int> &cregno,
		       const auto_vec<int> &anode_value,
		       const auto_vec<int> &anode_name,
		       const auto_vec<int> &anode_regno,
		       int *subst_out, int *fail_out,
		       auto_vec<int> *refused = NULL)
{
  int subst = 0, fail = 0;

  if (refused)
    refused->truncate (0);
  for (unsigned q = 0; q < slots.length (); q++)
    {
      int n = 0;
      unsigned i = (unsigned) slots[q].insn;
      if (i >= seq.length ())
	continue;
      if (!lvx_lms_rename_insn_p (seq[i], slots[q].iter, arcs, insn2cls,
				  cregno, anode_value, anode_name,
				  anode_regno, i, &n, false))
	{
	  fail++;
	  if (refused && !refused->contains ((int) i))
	    refused->safe_push ((int) i);
	  /* WHICH insn and WHICH instance, bounded.  A refusal count cannot be
	     acted on: the decision is whether to decline the loop or to fix
	     the substitution, and that needs the pattern.  */
	  if (fail <= 3)
	    {
	      int code = INSN_CODE (seq[i]);
	      lvx_lms_dump (";; lvx cms:   rename refused insn=%u iter=%d"
			    " uid=%u subst=%d pattern=%s\n", i, slots[q].iter,
			    INSN_UID (seq[i]), n,
			    code >= 0 ? get_insn_name (code) : "?");
	    }
	}
      subst += n;
    }

  *subst_out = subst;
  *fail_out = fail;
  return fail == 0;
}

/* VERIFY THE MASTER SCHEDULE'S DATAFLOW, over the recorded slots.

   The plan counts slots and the rename map assigns registers, and neither
   answers the question emission actually depends on: does every instance
   READ a value some earlier instance WROTE, along the path that reaches it?
   A pipeline can be correctly counted, correctly coloured, and still deliver
   an instance whose operand was never produced on the path taken -- which is
   a wrong answer, not a crash, and so the kind of defect an executable gate
   finds late and a data check finds now.

   THREE PATHS REACH AN INSTRUCTION, and each is checked as its own linear
   order:

     - the prologue then the kernel's first trip,
     - the prologue, the kernel, and the main epilogue,
     - the prologue, the kernel, and one early-exit epilogue.

   For a RAW arc from producer P to consumer C at distance OMEGA, consumer
   instance L reads what producer instance L - OMEGA wrote.  That instance
   must appear EARLIER on the path, or be absent because it belongs to an
   iteration before the loop began -- L - OMEGA < 0 -- which is the live-in
   case and is exactly what USE_FIRST pins to the preheader's register
   (§7ter.4).  Anything else is a read of an undefined register.

   The kernel is CYCLIC, so "earlier" there is not position in the slot list:
   a kernel instance may legitimately read what the PREVIOUS trip's instance
   wrote.  So a kernel consumer is satisfied by a producer anywhere in the
   kernel when OMEGA >= 1, and only by an earlier position when OMEGA == 0.
   Getting that wrong in the strict direction would have reported every
   loop-carried value as a violation.  */

static bool
lvx_lms_verify_slots (const auto_vec<rtx_insn *> &seq,
		      const auto_vec<lvx_lms_arc> &arcs,
		      const auto_vec<lvx_lms_slot> &slots,
		      const auto_vec<lvx_lms_node> &nodes,
		      int *undef_out, int *order_out, int *paths_out)
{
  int undef = 0, order = 0, paths = 0;

  *undef_out = *order_out = *paths_out = 0;

  /* One path per epilogue, plus the bare kernel path.  Each is the prologue
     and kernel slots in walk order followed by that epilogue's slots.  */
  for (unsigned ep = 0; ep <= nodes.length (); ep++)
    {
      /* EP == length () is the path that stays in the kernel.  */
      bool bare = (ep == nodes.length ());
      if (!bare && nodes[ep].kind != LVX_LMS_NODE_EPILOGUE)
	continue;
      paths++;

      /* Position of each (insn, iter) on this path, and -1 for absent.  The
	 iteration range is bounded by the slots themselves.  */
      int maxiter = 0;
      for (unsigned q = 0; q < slots.length (); q++)
	if (slots[q].iter > maxiter)
	  maxiter = slots[q].iter;

      auto_vec<int> pos, inkernel;
      pos.safe_grow_cleared (seq.length () * (maxiter + 1));
      inkernel.safe_grow_cleared (seq.length () * (maxiter + 1));
      for (unsigned t = 0; t < pos.length (); t++)
	pos[t] = -1;

      /* A PATH LEAVES THE WALK AT ITS EXIT.  The first version put every
	 Phase A slot on every path, which is wrong for an epilogue path: the
	 slots after the exit are not executed on it.  Their presence made a
	 drain copy of an instance overwrite the Phase A copy's position, and
	 the check then reported a producer as coming AFTER its consumer --
	 six loops' worth of violations that were in the verification, not in
	 the construction.  */
      int at = 0;
      for (unsigned q = 0; q < slots.length (); q++)
	{
	  int nd = slots[q].node;
	  bool is_ep = (nd >= 0 && nd < (int) nodes.length ()
			&& nodes[nd].kind == LVX_LMS_NODE_EPILOGUE);
	  if (is_ep && (bare || nd != (int) ep))
	    continue;
	  if (!is_ep && !bare)
	    {
	      /* Past the exit this path takes?  Then it is not on this path.
		 Walk order is (date, iteration, insn).  */
	      int xd = nodes[ep].exit_date, xl = nodes[ep].exit_iter;
	      int xi = nodes[ep].exit_insn;
	      if (slots[q].date > xd
		  || (slots[q].date == xd
		      && (slots[q].iter > xl
			  || (slots[q].iter == xl && slots[q].insn > xi))))
		continue;
	    }
	  int t = slots[q].insn * (maxiter + 1) + slots[q].iter;
	  pos[t] = at++;
	  if (nd >= 0 && nd < (int) nodes.length ()
	      && nodes[nd].kind == LVX_LMS_NODE_KERNEL)
	    inkernel[t] = 1;
	}

      for (unsigned q = 0; q < slots.length (); q++)
	{
	  int nd = slots[q].node;
	  bool is_ep = (nd >= 0 && nd < (int) nodes.length ()
			&& nodes[nd].kind == LVX_LMS_NODE_EPILOGUE);
	  if (is_ep && (bare || nd != (int) ep))
	    continue;
	  if (!is_ep && !bare)
	    {
	      int xd = nodes[ep].exit_date, xl = nodes[ep].exit_iter;
	      int xi = nodes[ep].exit_insn;
	      if (slots[q].date > xd
		  || (slots[q].date == xd
		      && (slots[q].iter > xl
			  || (slots[q].iter == xl && slots[q].insn > xi))))
		continue;
	    }

	  int ct = slots[q].insn * (maxiter + 1) + slots[q].iter;
	  for (unsigned a = 0; a < arcs.length (); a++)
	    {
	      if (arcs[a].kind != LVX_ARC_REG_RAW
		  || (int) arcs[a].dst != slots[q].insn)
		continue;
	      int pi = (int) arcs[a].src;
	      int pl = slots[q].iter - (int) arcs[a].omega;
	      if (pl < 0)
		continue;		/* live-in: the preheader wrote it */
	      if (pl > maxiter)
		continue;		/* no such instance in the walk */
	      int pt = pi * (maxiter + 1) + pl;
	      if (pos[pt] < 0)
		{
		  undef++;
		  continue;
		}
	      /* Order, with the kernel's cyclicity allowed for: a
		 loop-carried read inside the kernel is satisfied by the
		 previous trip.  */
	      if (pos[pt] >= pos[ct]
		  && !(arcs[a].omega >= 1 && inkernel[pt] && inkernel[ct]))
		{
		  order++;
		  if (order <= 3)
		    lvx_lms_dump (";; lvx cms:   slot order path=%u"
				  " cons=%d@%d/pos%d prod=%d@%d/pos%d"
				  " omega=%u kcons=%d kprod=%d\n", ep,
				  slots[q].insn, slots[q].iter, pos[ct],
				  pi, pl, pos[pt], arcs[a].omega,
				  inkernel[ct], inkernel[pt]);
		}
	    }
	}
    }

  *undef_out = undef;
  *order_out = order;
  *paths_out = paths;
  return undef == 0 && order == 0;
}

/* MAKE_NODES: build the pipeline's block structure.  §7quinquies.9.

   The reference's two-phase split, kept for the reason §7quinquies.6 gives --
   "build the whole CFG, then fill it", because that is what lets the epilogue
   shape be settled before an insn is committed.  This is phase one.

   PHASE ONE USES `split_edge', NOT HAND-ROLLED SURGERY, and the reference's
   reason for avoiding it does not apply here.  §7quinquies.1 quotes that
   reason -- "Cannot use split_edge since block from_edge->src is still empty"
   -- and it is about filling blocks that do not exist yet; this inserts an
   empty block between two blocks that both already have insns, which is
   exactly what split_edge is for.  It gets the NOTE_INSN_BASIC_BLOCK in the
   right place, the layout right (load-bearing, since `schedule_ebbs' walks
   `bb->next_bb' rather than the edges, §7quaterdecies.1), the loop
   registration right, and the redirect right.  Hand-rolled edge work is what
   make_insns needs, not make_nodes.

   WHAT THIS BUILDS AND WHAT IT DOES NOT.  The prologue stages and the epilogue
   are created EMPTY and the body is left where it is, so the loop still runs
   exactly as before: an empty block with a fall-through edge is a no-op, and
   the function is semantically unchanged.  That is deliberate -- the edge
   surgery is the part most likely to be wrong, and this way it can be verified
   against a known-good program (`verify_flow_info' plus the 71-program
   differential on both cores) before the schedule is allowed to move a single
   instruction.  Filling the blocks is `make_insns', and until it exists this
   costs a few empty blocks and changes no behaviour.

   Returns false and leaves the CFG untouched if anything cannot be built,
   which the caller treats as "do not pipeline this loop".  */

/* What one pipelineable loop needs, as RAW BASIC BLOCKS.  A `class loop *' does
   not survive loop_optimizer_finalize and neither does anything reached through
   it, so the analysis records blocks and the transformation re-derives its edges
   from them.  §7quinquies.10.  */

/* What one pipelineable loop needs, gathered in one place so make_nodes takes
   a described site rather than eight arguments.  Raw basic blocks, with the
   edges re-derived at use: an edge pointer is CFG state and would survive, but
   re-finding it cannot go stale against a redirect elsewhere.  */
struct lvx_lms_site
{
  /* The loop itself, so the emission can ask which of a jump's successors
     leaves it -- a multi-exit body needs that per jump, not once.  */
  class loop *loop;
  basic_block preheader, header, latch, exit_src, exit_dest;
  int nstages;
  struct lvx_lms_plan plan;
  /* Whether every slot's insn survives the rename substitution and still
     recognises (§7tricies.10).  Recorded rather than gated on here: phase one
     renames nothing, so declining a loop for it would cost coverage for no
     reason -- but the phase that emits a second instance must require it,
     because a tied operand makes the substitution unrealisable rather than
     merely unwise.  */
  bool renameable;
  /* Whether every register the renaming leaves alone keeps its order
     (§7tricies.17).  A pinned class is such a register by construction, so
     this is the check that pinning does not reintroduce the defect
     §7tricies.7 found in the reorder.  */
  bool antidep_ok;
  /* Whether the CLASS-level assignment -- the one codegen renames from --
     passes the occupancy check.  The value-level one was always verified;
     this one never was, and it was wrong.  */
  bool classocc_ok;
  /* The unroll factor the plan and the renaming agreed on (§7tricies.47),
     which the hardware loop's trip count has to be divided by.  */
  int kunroll;
  /* DOES ENTERING THE LOOP GUARANTEE THAT ITERATION 0 COMPLETES?
     §7tricies.43.  The prologue's iteration-0 side effects are sound only if
     it does, and that is a property of the loop's SHAPE: with a single jump
     in the body and every exit leaving from that jump's block, the path from
     the header to it is unconditional, so entering runs the whole body once
     -- a bottom-tested loop, where the zero-trip case was decided before the
     preheader.  A TOP-tested loop has its single jump at the head and can be
     entered with nothing to do, and there an iteration-0 store in the
     prologue is a real speculation.

     Checked rather than assumed: replacing the blanket refusal with "entering
     admits iteration 0" is only an improvement if that sentence is known to
     be true of the loop in hand.  */
  bool entry_admits;
  /* The lvx_loopdo reorg_loops had already put on this loop, or NULL.
     RECORDED HERE because it must be found BEFORE make_nodes splits the
     preheader edge: after that the header's non-loop predecessor is CMS's own
     prologue block and the LOOPDO is no longer reachable from it.
     §7tricies.196.  */
  rtx_insn *loopdo;
};

/* THE BLOCK A JUMP LEAVES THE LOOP FOR, or null if it has none.  A multi-exit
   body needs this per jump: lu's loop 7 exits to bb 23 from the header and to
   bb 18 from the latch, so one SITE->EXIT_DEST cannot describe both.  */

static basic_block
lvx_lms_jump_exit_dest (const struct lvx_lms_site *site, rtx_insn *jmp)
{
  basic_block bb = BLOCK_FOR_INSN (jmp);
  edge e;
  edge_iterator ei;

  if (!bb || !site->loop)
    return NULL;
  FOR_EACH_EDGE (e, ei, bb->succs)
    if (!flow_bb_inside_loop_p (site->loop, e->dest))
      return e->dest;
  return NULL;
}

/* Does this jump CONTINUE the loop when taken?  The early-exit emission has to
   invert such a jump so that leaving becomes the taken direction, and must NOT
   invert one that already exits when taken -- which is the difference between
   a bottom test and a top test, and the reason a second jump cannot simply be
   treated like the loopback.  */

static bool
lvx_lms_jump_taken_stays_p (const struct lvx_lms_site *site, rtx_insn *jmp)
{
  basic_block bb = BLOCK_FOR_INSN (jmp);
  edge ft, taken = NULL;
  edge e;
  edge_iterator ei;

  if (!bb || !site->loop)
    return true;
  ft = find_fallthru_edge (bb->succs);
  FOR_EACH_EDGE (e, ei, bb->succs)
    if (e != ft)
      taken = e;
  if (!taken)
    return true;
  return flow_bb_inside_loop_p (site->loop, taken->dest);
}

/* THE SKELETON BLOCKS.  §7tricies.136.

   This is what `lvx_lms_node' is, and the name is unfortunate because two
   different things in CMS are called nodes:

     SKELETON BLOCK   one lvx_lms_node, one BASIC BLOCK of the emitted
		      pipeline.  BLOCKS[k] is the block, NODES[k].kind says
		      which part of the pipeline it is, and SLOTS[q].node says
		      which block a slot is emitted into.
     ANODE            a node of the circular-arc INTERFERENCE graph, one per
		      (value, name) pair -- lvx_lms_anode_of, counted by NN.
		      Nothing to do with blocks.

   A software pipeline is a chain of skeleton blocks, in this order, and this
   is the whole construct:

     PREHEADER        not a skeleton block: the original loop's, where the
		      trip count is divided when the kernel is k-unrolled.

     PROLOGUE[0..]    the stages being filled.  Each block is a PARTIAL copy
		      of the body, and the chain lengthens as the pipe fills.

     KERNEL[0..]      the steady state, entered with every stage occupied.
		      The last block closes with the loop-back branch.
		      KUNROLL copies of the body live here when the kernel is
		      unrolled, and the modulo-expanded names rotate across
		      them.

   HOW MANY BLOCKS EACH PART HAS IS NOT A FUNCTION OF NSTAGES.  One rule in
   lvx_lms_build_plan decides every boundary --

     if (prev_exit || enter_kernel || nodes == 0) ... new block ...

   -- so a block starts at the beginning, after EVERY EXIT INSTANCE, and where
   the kernel begins, and nothing counts stages.  inflate_fast loop 2 with
   NSTAGES = 2 has FOUR prologue blocks, cut after the exits at insn 8 and
   insn 9 (both date 1) and after the loop-back instance at insn 16 (date 11),
   and three kernel blocks for the same reason.

   `S - 1' HOLDS FOR A SINGLE-BASIC-BLOCK BODY, S being the number of stages:
   a doloop or a while whose body is one block has only the loop-back as an
   exit instance, one per stage being filled, so the cuts land one per stage
   and the prologue has S - 1 blocks.  That is the textbook picture, and it
   follows FROM the exit-cutting rule under that shape rather than being a
   second rule.  Once the body is more than one block, S says nothing about
   the count.

     EPILOGUE[0..]    the DRAINS, one per exit -- EXEP of them, plus the main
		      one for the loop-back falling out.  A drain is NOT just
		      cleanup: it REPLAYS the work still in flight when its
		      exit was taken, so it holds the later stages of the
		      iterations that had already started (EPISYM counts those
		      replicated slots).  It then ends with the live-out
		      reconciliation copies (§7tricies.63, .66), which is why a
		      live-out value must survive to the END OF ITS DRAIN and
		      not merely to its exit: the replayed work can overwrite
		      the register the copy is about to read (§7tricies.135).

   PATHS is the number of ways through this chain -- one per epilogue plus the
   bare kernel path -- and lvx_lms_verify_slots checks each separately,
   because an instance's operands must be defined on EVERY path that reaches
   it, not merely somewhere in the schedule.

   A finding kept although the code that needed it is gone, because make_insns
   will face it again: `redirect_edge_and_branch' retargets the JUMP and
   REFUSES a fall-through edge outright, while `redirect_edge_succ' moves the
   edge and leaves the jump alone -- right for a fall-through, silently wrong
   for a branch.  And which edge is which is not what intuition says: in a
   bottom-tested loop the latch branches BACK to the header and FALLS THROUGH
   to the exit, so the back edge is the branch and the loop's exit is the
   fall-through.  */

/* EVERY BLOCK'S SUCCESSORS, so a verify_flow_info failure can be READ rather
   than guessed at.  §7tricies.196.  */
void
lvx_lms_dump_cfg (const char *when)
{
  basic_block bb;

  if (!dump_file)
    return;
  fprintf (dump_file, ";; lvx cms: CFGALL %s:\n", when);
  FOR_EACH_BB_FN (bb, cfun)
    {
      edge e;
      edge_iterator ei;

      fprintf (dump_file, ";;   bb%d%s ->", bb->index,
	       (BB_END (bb) && JUMP_P (BB_END (bb)))
	       ? (any_condjump_p (BB_END (bb)) ? " C" : " J") : "");
      /* Counted the way verify_flow_info counts: n_branch excludes
	 fall-through, EH and abnormal edges.  */
      int nbr = 0;

      FOR_EACH_EDGE (e, ei, bb->succs)
	{
	  const char *k = ((e->flags & EDGE_EH) ? "e"
			   : (e->flags & EDGE_ABNORMAL_CALL) ? "c"
			   : (e->flags & EDGE_SIBCALL) ? "s"
			   : (e->flags & EDGE_FALLTHRU) ? "f"
			   : (e->flags & EDGE_ABNORMAL) ? "a" : "b");

	  if (*k == 'b')
	    nbr++;
	  fprintf (dump_file, " bb%d%s", e->dest->index, k);
	}
      fprintf (dump_file, "  nbranch=%d%s\n", nbr,
	       (nbr > 1 && BB_END (bb) && JUMP_P (BB_END (bb))
		&& (any_condjump_p (BB_END (bb))
		    || any_uncondjump_p (BB_END (bb)))) ? "  ILLEGAL" : "");
    }
}

static bool
lvx_lms_make_nodes (const struct lvx_lms_site *site, class loop *loop,
		    const auto_vec<lvx_lms_node> *nodes,
		    auto_vec<basic_block> *blocks, const char **why)
{
  int nstages = site->nstages;
  *why = "plan unusable";
  if (!site->plan.ok || nstages < 2)
    return false;

  /* Both edges are re-derived from the recorded blocks rather than stored:
     an edge pointer is CFG state and survives, but re-finding it is cheap and
     cannot go stale against a redirect somewhere else.  */
  /* NULL when the header had no single out-of-loop predecessor
     (§7tricies.197); find_edge would dereference it.  */
  edge pre = (site->preheader != NULL
	      ? find_edge (site->preheader, site->header) : NULL);
  *why = "no single preheader outside the loop";
  if (!pre)
    return false;

  /* THE DRAIN GOES ON THE LATCH'S EXIT EDGE, not on "the loop's single exit".
     The first version tested that the loop had exactly one edge leaving it,
     which rejected all four trmm loops and would reject every multi-block one:
     a chain body has one exit per block, which IS the superblock shape CMS
     exists for (§7.5a).  What the drain actually needs is the edge the kernel
     falls out of, which is the latch's.  */
  edge exit_e = find_edge (site->exit_src, site->exit_dest);
  *why = "latch exit edge is gone";
  if (!exit_e)
    return false;


  /* PROLOGUE and DRAIN, both by `split_edge'.

     THE HAND-ROLLED VERSION WAS WRONG, and reading GCC rather than guessing is
     what found it.  `create_basic_block (NULL, NULL, after)' does not put the
     new block's insn where AFTER is: `create_basic_block_structure' does

         head = end = bb_note
           = emit_note_after (NOTE_INSN_BASIC_BLOCK, get_last_insn ());

     so with a null head the note is appended after the last insn of the WHOLE
     FUNCTION, and AFTER only positions the block in the block list.  The block
     order and the insn chain then disagree, and the ICE was a later pass
     deleting such a block: remove_insn's gcc_assert (seq) fires because the
     note has no NEXT_INSN and is not the recorded last of any sequence.
     GCC's own code never passes a null head for this -- see
     force_nonfallthru_and_redirect, which hands it BB_HEAD (e->dest).

     `split_edge' is the maintained way to put an empty block on an existing
     edge, and it does every part correctly: the note in the right place, the
     layout, the loop registration, and the redirect -- which also retires the
     fall-through-versus-branch question that cost two cycles, since split_edge
     picks the right mechanism itself.

     §7quinquies.1 quotes the reference's reason for NOT using it -- "cannot use
     split_edge since block from_edge->src is still empty" -- and that reason
     does not apply here: it is about filling blocks that do not exist yet,
     where this is inserting a block between two blocks that both have insns.
     Hand-rolling is needed for make_insns, not for make_nodes.  */
  /* ONE BLOCK PER PLAN NODE, in the plan's own order, all on the preheader
     edge.  §7tricies.12.

     This used to create `nstages - 1' stage blocks plus one drain on the
     latch's exit, which was the right skeleton for a pipeline with no early
     exits and is not the skeleton the plan asks for: the plan's blocks are the
     prologue stages SPLIT AT EVERY EXIT, the k-unrolled kernel split the same
     way, and one epilogue per exit plus the main one (§7tricies.5 --
     plexep = kunroll - 1, so there are always more than one).  Creating them
     all here, in node order, means make_insns places by index rather than by
     re-deriving a structure that has to agree with the plan's counts.

     THE WHOLE CHAIN GOES ON THE PREHEADER EDGE, including the epilogues.
     That looks wrong and is what §7quinquies.4 and .5 describe: the epilogues
     are laid out after the main one, which "jumps over them", and each is
     reached only from its own exit.  A block in the fall-through chain that
     nothing falls into is reached only by its edges, so building them here
     costs an edge to remove later and buys a single creation path.  While the
     blocks are empty the chain is a sequence of no-ops and the loop still
     runs exactly as before -- which is the property that lets the skeleton be
     verified against a working program before an instruction moves.

     Each split takes the edge still reaching the HEADER, so the chain grows
     in order and there is nothing to walk.  The previous version re-found the
     edge by walking the single-successor chain, bounded by the stage count;
     that was only ever needed because it split the same edge repeatedly
     instead of the frontier one.  */
  *why = "no node list";
  if (!nodes || nodes->is_empty ())
    return false;

  *why = "cannot split the preheader edge";
  basic_block prev = site->preheader;
  for (unsigned k = 0; k < nodes->length (); k++)
    {
      edge e = find_edge (prev, site->header);
      if (!e)
	return false;
      basic_block bb = split_edge (e);
      if (!bb)
	return false;
      bb->count = site->header->count;
      if (blocks)
	blocks->safe_push (bb);
      prev = bb;
    }

  /* EXIT_E is read for its existence only: the drain it used to carry is now
     the main epilogue node, created above.  Keeping the test is not
     redundant -- a loop whose latch has no exit edge is one whose kernel
     cannot fall out, and the emission has nowhere to send the main
     epilogue.  */
  (void) exit_e;

  *why = NULL;
  return true;
}

/* MAKE_INSNS PHASE ONE IS RETIRED.  §7tricies.18.

   It put one block's instructions into the schedule's order and changed no
   edge, and its stated purpose was to exercise the machinery that moves and
   rewrites instructions on a running program before that machinery was also
   asked to build a pipeline -- the KVX implementation's ICEs having been in
   exactly that code.  That purpose is spent: the pipeline exists, and two
   gate sources exercise it under a compiler built with rtl and df checking.

   It is retired rather than fixed because the transformation was never the
   right one.  SIGMA is optimised for the CYCLIC problem subject to arcs that
   assume renaming, so applying it acyclically to a single iteration is
   neither correct -- §7tricies.7 measured 47% of loops reordered unsoundly
   against a graph that omits the WAR arcs a non-renaming pass needs -- nor
   profitable, since ordering the body for the acyclic problem is what the
   local scheduler already does, with the right dependence graph.  Giving
   phase one its own graph would have meant reimplementing that scheduler.

   -mlms-codegen alone now builds the block skeleton and emits nothing, which
   is the state §7quinquies.9 designed it for: the edge surgery verified
   against a working program before the schedule is allowed to move an
   instruction.  -mlms-pipeline is what emits.  */

/* THE ANTI-DEPENDENCES RENAMING DOES NOT REMOVE.  §7tricies.19.

   The graph drops register WAR and WAW arcs where modulo expansion is
   expected to rename the conflict away, which is right and is most of what
   modulo scheduling buys.  What is left is what the renaming does not reach,
   and that has an exact answer -- arrived at after three guessed bounds,
   each of which §7tricies.17 records condemning most of the population.

   DERIVATION.  Take a class with owner O (its write) and a consumer C at
   distance OMEGA.  Instance (O, L) writes name L mod P2; instance (C, M)
   reads name (M - OMEGA) mod P2.  The read must happen before the NEXT write
   to that name, which is instance (O, M - OMEGA + P2):

       sigma (C) + M * II  <  sigma (O) + (M - OMEGA + P2) * II
       sigma (C) - sigma (O) + OMEGA * II  <  P2 * II
       life  <  P2 * II

   which is exactly what P2 = ceil (life / II) gives.  So for a properly
   expanded class THERE IS NOTHING TO CHECK: the degree computation IS the
   anti-dependence constraint.  That is why the arcs can be dropped, and it
   is the thing three bounds over instruction pairs were groping for.

   WHAT PINNING BREAKS is therefore exact.  A pinned class (§7tricies.15) is
   given degree 1 whatever its life, so the inequality fails precisely when
   its life exceeds II.  One comparison per class, not a bound over pairs.

   WHAT HAS NO CLASS still needs the pair test: a register the body writes
   that no class owns is renamed by nothing, so its accesses keep their order
   -- 0 <= sigma (j) - sigma (i) < II, the pair of arcs the graph would have
   carried.  That set is far smaller than "every surviving register", which
   is what the earlier versions tested and why they condemned 118, then 89,
   then 36 of 124 loops: a class's original register is surviving, and part
   one already covers it.  */

static bool
lvx_lms_verify_antidep (const auto_vec<rtx_insn *> &seq,
			const auto_vec<int> &sigma, int ii,
			const HARD_REG_SET *surviving,
			const auto_vec<int> &classreg,
			const auto_vec<int> &classlife,
			const auto_vec<int> &classnames,
			int *short_out, int *pair_out,
			int *first_a, int *first_b, int *first_c)
{
  int nshort = 0, npair = 0;

  *short_out = *pair_out = 0;
  *first_a = *first_b = *first_c = -1;
  if (ii <= 0)
    return false;

  /* PART ONE: every class needs at least ceil (life / II) names.  */
  for (unsigned c = 0; c < classlife.length (); c++)
    {
      int life = classlife[c];
      int need = life <= 0 ? 1 : (life + ii - 1) / ii;

      if (classnames[c] <= 0 || classnames[c] >= need)
	continue;
      if (nshort == 0)
	{
	  *first_a = (int) c;
	  *first_b = classnames[c];
	  *first_c = need;
	}
      nshort++;
    }

  /* PART TWO: a register no class owns is renamed by nothing.  */
  for (unsigned i = 0; i < seq.length (); i++)
    for (unsigned j = i + 1; j < seq.length (); j++)
      {
	int d = sigma[j] - sigma[i];
	df_ref ra, rb;
	int reg = -1;

	if (d >= 0 && d < ii)
	  continue;

	FOR_EACH_INSN_DEF (ra, seq[j])
	  {
	    unsigned r = DF_REF_REGNO (ra);
	    if (r >= FIRST_PSEUDO_REGISTER
		|| !TEST_HARD_REG_BIT (*surviving, r)
		|| (r < classreg.length () && classreg[r]))
	      continue;
	    FOR_EACH_INSN_USE (rb, seq[i])
	      if (DF_REF_REGNO (rb) == r)
		reg = (int) r;
	    FOR_EACH_INSN_DEF (rb, seq[i])
	      if (DF_REF_REGNO (rb) == r)
		reg = (int) r;
	  }
	if (reg < 0)
	  FOR_EACH_INSN_USE (ra, seq[j])
	    {
	      unsigned r = DF_REF_REGNO (ra);
	      if (r >= FIRST_PSEUDO_REGISTER
		  || !TEST_HARD_REG_BIT (*surviving, r)
		  || (r < classreg.length () && classreg[r]))
		continue;
	      FOR_EACH_INSN_DEF (rb, seq[i])
		if (DF_REF_REGNO (rb) == r)
		  reg = (int) r;
	    }

	if (reg < 0)
	  continue;
	if (npair == 0 && nshort == 0)
	  {
	    *first_a = (int) i;
	    *first_b = (int) j;
	    *first_c = reg;
	  }
	npair++;
      }

  *short_out = nshort;
  *pair_out = npair;
  return nshort == 0 && npair == 0;
}

/* READS BEFORE WRITES WITHIN A CYCLE.  §7tricies.35.

   A VLIW bundle reads its operands before it writes them, and the modulo
   schedule uses that: a value's live interval is [def + !earlyclobber, last],
   so another live range may be DEFINED in the very cycle this one is last READ,
   and the colouring will hand both the same register.  Emitted RTL is not a
   bundle -- it is a sequential stream that the bundler groups afterwards --
   so a reader placed after the writer reads the new value.

   The plan walks (date, iteration, body index), so within a cycle the order
   is body order, which has nothing to do with it.  On `A[i] = (i % 7) + 1'
   live ranges lr9 ([2,3], the zero-extended index) and lr5 ([4,4]) both got r17,
   disjoint by one cycle; at date 3 insn 12 reads lr9's r17 while insn 3
   writes lr5's r17.  Body order put insn 3 first, so the shift took
   `i + (i * magic >> 32)' as its index, the zero-extend became dead and was
   deleted, and the emitted function had no `zxwd' at all.  That is why the
   missing instruction looked like the defect when it was a symptom.

   The constraint is on the RENAMED registers, not the original ones: two
   slots touching one original register in one cycle are unconstrained if
   their live ranges were given different registers, which after modulo expansion
   is the common case.  So the registers are resolved from the live ranges first,
   then each cycle's slots are ordered by `every reader of a register written
   here comes before its writer'.

   A CYCLE IN THAT ORDER CANNOT BE EMITTED.  Two slots exchanging registers in
   one bundle -- A reads r writes s, B reads s writes r -- is legal in a VLIW
   and inexpressible as a sequential stream.  It arises when the colouring
   packs two live ranges into one register with touching intervals in both
   directions, so the caller's answer is to recolour with the definition cycle
   CLAIMED, which makes touching intervals interfere and removes every
   intra-cycle edge; only if that fails too is the loop declined.

   ASKED BEFORE ANYTHING IS EMITTED, which is why this is a function and not
   part of the emission: the emission mutates RTL, so a failure discovered
   half way through cannot be retried.  */

static bool
lvx_lms_slot_order (const auto_vec<rtx_insn *> &seq,
		    const auto_vec<lvx_lms_slot> &slots,
		    const auto_vec<lvx_lms_lrange_part *> &lr, int loopback,
		    auto_vec<int> &order, bool *any_rw = NULL)
{
  /* ANY_RW: did ANY cycle hold a reader-writer pair on one register, cyclic
     or not?  §7tricies.182.  The return value answers the CYCLIC question --
     do those pairs form a circuit, so that no sequential order exists -- and
     the colouring escalates CLAIM on it.  But the allocation fact is the PAIR:
     one instance reads a register another instance writes in the same cycle,
     which is what says the current KUNROLL has them sharing a name.  A single
     non-mutual pair linearises, so the circuit test does not see it.  This
     out-parameter is how the two are compared without changing either.  */
  if (any_rw)
    *any_rw = false;
  auto_vec<unsigned HOST_WIDE_INT> srd, swr;

  order.truncate (0);

  for (unsigned q = 0; q < slots.length (); q++)
    {
      unsigned HOST_WIDE_INT rd = 0, wr = 0;
      unsigned i = (unsigned) slots[q].insn;

      if (i < seq.length ())
	{
	  df_ref ref;

	  FOR_EACH_INSN_USE (ref, seq[i])
	    {
	      unsigned r = DF_REF_REGNO (ref);
	      int slot = lvx_lms_reg_index (PATTERN (seq[i]),
					    DF_REF_LOC (ref));
	      bool dly = false;
	      int off = 0, wid = 1;
	      lvx_lms_lrange_part *g = lvx_lms_lrange_of (lr, i, slot, &dly, &off,
						     &wid);
	      /* THE EFFECT'S SPAN, NOT THE RANGE'S.  §7tricies.77.  Taking
		 the range's name and width recorded a partial reference as
		 touching the WHOLE span, which manufactured intra-cycle
		 edges in both directions between a wide reference and a
		 narrow one inside it -- the symmetric all-four-registers
		 circuit that looked like two writers of one register.  */
	      int neu = g ? lvx_lms_lrange_reg (g, slots[q].iter, dly) + off
			  : (int) r;
	      int nw = g ? wid : 1;

	      for (int w = 0; w < nw; w++)
		if (neu + w >= 0 && neu + w < 64)
		  rd |= (unsigned HOST_WIDE_INT) 1 << (neu + w);
	    }
	  FOR_EACH_INSN_DEF (ref, seq[i])
	    {
	      unsigned r = DF_REF_REGNO (ref);
	      int slot = lvx_lms_reg_index (PATTERN (seq[i]),
					    DF_REF_LOC (ref));
	      int off = 0, wid = 1;
	      lvx_lms_lrange_part *g = lvx_lms_lrange_of (lr, i, slot, NULL, &off,
						     &wid);
	      int neu = g ? lvx_lms_lrange_reg (g, slots[q].iter, false) + off
			  : (int) r;
	      int nw = g ? wid : 1;

	      for (int w = 0; w < nw; w++)
		if (neu + w >= 0 && neu + w < 64)
		  wr |= (unsigned HOST_WIDE_INT) 1 << (neu + w);
	    }
	}
      srd.safe_push (rd);
      swr.safe_push (wr);
    }

  /* Each run of slots sharing (node, date), topologically sorted.  */
  for (unsigned a = 0; a < slots.length (); )
    {
      unsigned b = a;
      while (b < slots.length () && slots[b].node == slots[a].node
	     && slots[b].date == slots[a].date)
	b++;
      {
	unsigned n = b - a;
	auto_vec<int> deg, pick;
	auto_vec<bool> done;

	for (unsigned x = 0; x < n; x++)
	  {
	    deg.safe_push (0);
	    done.safe_push (false);
	  }
	/* THE EDGE SET, BUILT ONCE AND USED FOR BOTH THE IN-DEGREES AND THE
	   DECREMENTS.  §7tricies.179.

	   These were two expressions of the same relation -- the in-degree
	   loop incremented on one condition and the selection loop
	   decremented on a RE-DERIVED copy of it -- so a new edge kind had to
	   be added in both places or a slot's in-degree never drained.  That
	   is exactly what happened on the first attempt here: the jump's
	   in-degree was raised and never lowered, and the circuit report
	   named a single slot, `circuit 2/0'.  One matrix, two readers.

	   EDGE[x * n + y] means x must be emitted before y.  Three kinds:

	   1 reader -> writer, so a read of a register another slot in this
	     cycle writes is emitted first;
	   2 everything -> the loopback, which closes the kernel;
	   Kinds 1's exemption and 3 are behind -mcms-jump-last-on-node, which
	   is Init(0) BECAUSE IT HANGS: with them on, the four suites' census
	   improves (both terminator-class refusals cleared, emitted 1008 ->
	   1014, gate 8/8) and then BOTH zlib round trips run for the full
	   5400-second timeout without producing a checksum -- an infinite
	   loop in the emitted code.  §7tricies.179.  The matrix below is
	   unconditional and changes nothing on its own.

	   3 every non-jump -> a jump SHARING ITS NODE, because the plan walk
	     closes a node at each jump and §7tricies.130 refuses a slot
	     following its node's jump.

	   Kind 1 is dropped where the reader is a jump on the writer's own
	   node: same date and same node is the same BLOCK, hence one bundle,
	   and a bundle reads every operand before writing any -- so the jump
	   sees the old value whatever the textual order, and read-before-write
	   has nothing to enforce.  Keeping both is the deadlock §7tricies.178
	   measured.  The exemption stays inside one node deliberately: a
	   cycle's slots are spread over skeleton blocks (§7tricies.136), and
	   across two blocks the pair is a sequential stream where order is
	   the semantics.  */
	auto_vec<bool> edge;

	edge.safe_grow_cleared (n * n);
	for (unsigned x = 0; x < n; x++)
	  for (unsigned y = 0; y < n; y++)
	    {
	      unsigned ux = (unsigned) slots[a + x].insn;
	      unsigned uy = (unsigned) slots[a + y].insn;
	      bool jx = ux < seq.length () && JUMP_P (seq[ux]);
	      bool jy = uy < seq.length () && JUMP_P (seq[uy]);
	      bool same = slots[a + x].node == slots[a + y].node;
	      bool e = false;

	      if (x == y)
		continue;
	      if ((srd[a + x] & swr[a + y]) != 0)
		{
		  if (any_rw)
		    *any_rw = true;
		  if (!(lvx_cms_jump_last_on_node && jx && same))
		    e = true;			/* 1 */
		}
	      if ((int) uy == loopback)
		e = true;			/* 2 */
	      if (lvx_cms_jump_last_on_node && jy && !jx && same)
		e = true;			/* 3 */
	      if (e)
		{
		  edge[x * n + y] = true;
		  deg[y]++;
		}
	    }

	for (unsigned step = 0; step < n; step++)
	  {
	    int take = -1;

	    for (unsigned x = 0; x < n && take < 0; x++)
	      if (!done[x] && deg[x] == 0)
		take = (int) x;
	    if (take < 0)
	      {
		/* THE OFFENDING PAIR, PRINTED.  §7tricies.74.  The slots
		   still undone with a nonzero in-degree are the circuit;
		   every edge among them is a register that one of them reads
		   and another writes in this same cycle, which is a swap --
		   legal in a bundle, inexpressible as a sequential stream.
		   Reporting only "cannot be linearised" left no way to tell
		   an unlucky colouring from a forced one.  */
		if (lvx_lms_probe)
		  {
		    /* WHETHER THE CIRCUIT IS CONFINED TO ONE NODE.
		       §7tricies.181.  A circuit whose members all share a
		       node is one BLOCK, hence one bundle by the design's
		       invariant, where a read precedes a write whatever the
		       textual order -- so it needs no extra names and this
		       failure is too strong.  A circuit spanning nodes really
		       is a sequential stream and the names are needed.  The
		       colouring's model search escalates CLAIM on this
		       failure (§7tricies.180), so which kind it is decides
		       whether that escalation is necessary or waste.  */
		    {
		      int n0 = -1;
		      bool onenode = true;

		      for (unsigned x = 0; x < n; x++)
			if (!done[x])
			  {
			    if (n0 < 0)
			      n0 = slots[a + x].node;
			    else if (slots[a + x].node != n0)
			      onenode = false;
			  }
		      lvx_lms_dump (";; lvx cms:   LINEARSPAN %s nodes"
				    " date=%d\n",
				    onenode ? "ONE" : "MANY", slots[a].date);
		    }
		    lvx_lms_dump (";; lvx cms:   LINEAR node%d date%d: circuit",
				  slots[a].node, slots[a].date);
		    for (unsigned x = 0; x < n; x++)
		      if (!done[x])
			lvx_lms_dump (" %d/%d", slots[a + x].insn,
				      slots[a + x].iter);
		    lvx_lms_dump ("\n");
		    for (unsigned x = 0; x < n; x++)
		      if (!done[x])
			for (unsigned y = 0; y < n; y++)
			  if (!done[y] && x != y)
			    {
			      unsigned HOST_WIDE_INT m
				= srd[a + x] & swr[a + y];

			      for (int rr = 0; rr < 64; rr++)
				if ((m >> rr) & 1)
				  lvx_lms_dump (";; lvx cms:     %d/%d reads"
						" r%d that %d/%d writes\n",
						slots[a + x].insn,
						slots[a + x].iter, rr,
						slots[a + y].insn,
						slots[a + y].iter);
			    }
		  }
		return false;
	      }
	    done[take] = true;
	    pick.safe_push (a + take);
	    for (unsigned y = 0; y < n; y++)
	      if (!done[y] && edge[(unsigned) take * n + y])
		deg[y]--;
	  }
	for (unsigned x = 0; x < n; x++)
	  order.safe_push (pick[x]);
      }
      a = b;
    }

  return true;
}

/* MAKE_INSNS, PHASE TWO: emit the pipeline.  §7tricies.13.

   Phase one put one block's instructions into schedule order and changed no
   edge (§7tricies.7).  This fills the skeleton §7tricies.12 built: every slot
   of the master schedule, renamed for its iteration, into the block its node
   names, with the control flow closed around them.

   WHAT THE SKELETON LEAVES TO DO.  The blocks exist, empty, in a fall-through
   chain from the preheader, in plan-node order: the prologue stages, then the
   k-unrolled kernel, then the main epilogue, then the early-exit epilogues.
   Three things make that a pipeline:

     - every epilogue is redirected to the loop's exit, so it leaves the chain
       and is reachable only from its own exit (§7quinquies.5's layout, where
       the main epilogue "jumps over" the others);
     - the ITER_SLOT instance of the loopback closes the kernel, branching to
       the first kernel block and falling through to the main epilogue;
     - every other loopback instance is INVERTED into a branch to its
       epilogue, falling through to the next block in the chain
       (§7quinquies.2a).

   The inversion is not an optimisation.  A block ends with one jump, whose
   two successors are the target and the layout successor, so "if continuing
   go on, else drain" can only be spelled as "if NOT continuing go to the
   epilogue" -- otherwise the epilogue would have to be the layout successor
   of every exit, and there are kunroll - 1 of them.

   THE ORIGINAL BODY IS LEFT TO DIE.  Once the last epilogue is redirected to
   the exit, nothing reaches the header, and an unreachable block is removed
   by the cleanup that follows.  Deleting it here would mean unlinking a block
   that is still a live loop's header while the loop structures exist, which
   is the shape of the ICE §7quinquies.10 already paid for once.  */

/* SEQUENTIALISE A PARALLEL COPY.  Boissinot, Darte, Rastello, Dupont de
   Dinechin and Guillon, "Revisiting Out-of-SSA Translation for Correctness,
   Code Quality, and Efficiency", CGO 2009, Algorithm 1 -- transcribed rather
   than reinvented, and the reference says a similar algorithm is C. May's.

   The epilogue copies are PARALLEL: each live-out live range moves its last
   name back to the register the code after the loop reads, and one live range's
   source may be another's destination.  Emitted in an arbitrary sequential
   order they clobber a source before it is read -- and for a cyclic
   permutation no order works at all, which is what the one fresh register is
   for.  The algorithm emits the minimum number of copies: one extra per
   cycle, none otherwise.

   PSRC/PDST/PMODE are the parallel copies (PDST[k] := PSRC[k]), whose
   destinations must be distinct -- which §7tricies.66's disambiguation
   guarantees, one live-out live range per register, and which is also the
   algorithm's "(unique) predecessor" requirement.  SCRATCH is the fresh
   register, or -1 when none was free: then a cycle is refused rather than
   broken wrongly.  LOC and PRED are indexed by register number, which is
   what lets the whole thing be two arrays and two stacks.  */

static bool
lvx_lms_seq_pcopy (const auto_vec<int> &psrc, const auto_vec<int> &pdst,
		   const auto_vec<machine_mode> &pmode, int scratch,
		   auto_vec<int> *osrc, auto_vec<int> *odst,
		   auto_vec<machine_mode> *omode)
{
  int loc[FIRST_PSEUDO_REGISTER], pred[FIRST_PSEUDO_REGISTER];
  machine_mode vmode[FIRST_PSEUDO_REGISTER];
  auto_vec<int> ready, todo;
  unsigned k;

  if (psrc.length () != pdst.length () || psrc.length () != pmode.length ())
    return false;
  for (int r = 0; r < FIRST_PSEUDO_REGISTER; r++)
    {
      loc[r] = pred[r] = -1;
      vmode[r] = VOIDmode;
    }
  for (k = 0; k < pdst.length (); k++)
    if (psrc[k] < 0 || pdst[k] >= FIRST_PSEUDO_REGISTER
	|| pdst[k] < 0 || psrc[k] >= FIRST_PSEUDO_REGISTER)
      return false;

  /* Lines 2-3: clear the destinations' locations and the sources'
     predecessors, before line 4 fills them in -- the order matters for a
     register that is both.  */
  for (k = 0; k < pdst.length (); k++)
    {
      loc[pdst[k]] = -1;
      pred[psrc[k]] = -1;
    }
  /* Lines 4-7.  */
  for (k = 0; k < pdst.length (); k++)
    {
      loc[psrc[k]] = psrc[k];
      vmode[psrc[k]] = pmode[k];
      pred[pdst[k]] = psrc[k];
      todo.safe_push (pdst[k]);
    }
  /* Lines 8-9: a destination nothing needs can be overwritten at once.  */
  for (k = 0; k < pdst.length (); k++)
    if (loc[pdst[k]] < 0)
      ready.safe_push (pdst[k]);

  while (!todo.is_empty ())
    {
      int b;

      while (!ready.is_empty ())
	{
	  int a, c;

	  b = ready.pop ();
	  a = pred[b];
	  if (a < 0)
	    continue;
	  c = loc[a];
	  if (c < 0)
	    continue;
	  if (c != b)			/* a copy to itself is not one */
	    {
	      osrc->safe_push (c);
	      odst->safe_push (b);
	      omode->safe_push (vmode[c] != VOIDmode ? vmode[c] : DImode);
	    }
	  vmode[b] = vmode[c];
	  loc[a] = b;			/* now available in b */
	  if (a == c && pred[a] >= 0)
	    ready.safe_push (a);	/* just copied, can be overwritten */
	}

      b = todo.pop ();
      if (pred[b] >= 0 && b != loc[pred[b]])
	{
	  /* A circuit: break it through the fresh register.  */
	  if (scratch < 0 || scratch >= FIRST_PSEUDO_REGISTER)
	    return false;
	  osrc->safe_push (b);
	  odst->safe_push (scratch);
	  omode->safe_push (vmode[b] != VOIDmode ? vmode[b] : DImode);
	  vmode[scratch] = vmode[b];
	  loc[b] = scratch;
	  ready.safe_push (b);
	}
    }
  return true;
}

static bool
lvx_lms_emit_pipeline (const struct lvx_lms_site *site,
		       const auto_vec<rtx_insn *> &seq,
		       const auto_vec<lvx_lms_slot> &slots,
		       const auto_vec<lvx_lms_node> &nodes,
		       const auto_vec<basic_block> &blocks,
		       const auto_vec<lvx_lms_lrange_part *> &lr,
		       unsigned HOST_WIDE_INT avail,
		       int *nemit_out, const char **why)
{
  unsigned nnode = nodes.length ();
  int nemit = 0;

  *nemit_out = 0;
  *why = "skeleton does not match the plan";
  if (blocks.length () != nnode || nnode == 0)
    return false;

  /* THE LOOPBACK IS THE LATCH'S JUMP, not the last jump in body order.
     §7tricies.45.

     This used to require exactly ONE jump and take it as the loopback, which
     declined every multi-exit body -- a loop tested at the top AND at the
     bottom, which is what lu's loop 7 and ludcmp's loop 8 are.  Two jumps are
     now allowed and told apart by the CFG: the loopback is the one whose block
     is the latch and which has the header among its successors; every other
     jump is an exit with its own destination.

     Taking the last jump in body order happened to agree on both of those
     loops, and agreeing by accident is not the same as being derived.  */
  int loopback = -1, njump = 0;
  for (unsigned i = 0; i < seq.length (); i++)
    if (JUMP_P (seq[i]))
      {
	basic_block jb = BLOCK_FOR_INSN (seq[i]);

	njump++;
	if (jb && jb == site->latch)
	  {
	    edge e;
	    edge_iterator ei;

	    FOR_EACH_EDGE (e, ei, jb->succs)
	      if (e->dest == site->header)
		loopback = (int) i;
	  }
      }
  *why = "no jump in the latch returns to the header";
  if (loopback < 0)
    return false;

  /* A DOLOOP LATCH IS NOT A BRANCH TO RETARGET.  §7tricies.59.

     reorg_loops turns a counted loop's latch into `doloop_end', a PARALLEL of
     (set (pc) (if_then_else (ne counter 1) (label_ref top) (pc))), the
     counter's own decrement, and an UNSPEC_ENDLOOP marker -- which it later
     consumes into a LOOPDO in the preheader and a `# loopdo end' comment,
     the hardware repeating the body with no branch at all.

     The emission treated it as an ordinary conditional branch and tried
     invert-and-retarget on it.  The inversion succeeds and the REDIRECT does
     not, and the refusal was raised in the middle of the emission -- after
     the epilogue edges were redirected -- so the CFG was left half-built and
     verify_flow_info reported `wrong amount of branch edges after conditional
     jump'.  Two lines reproduce it:

         double A[1024], B[1024], Cc[1024];
         void g (int n)
         { for (int i = 0; i < n; i++) A[i] = B[i] * 1.25 + Cc[i]; }

     at -O2 -ffast-math -mlms-pipeline, where the base compiles to a LOOPDO.
     That is why every CMS study passes -fno-hwloop, and it is the whole of
     what TSVC2's s312 was: an ordinary hardware loop.

     Declined HERE, before anything is touched, so the loop keeps the hardware
     loop it already had.  Pipelining it properly means re-forming the
     DOLOOP -- the kernel repeating under LOOPDO with the count divided by
     KUNROLL -- which is the next piece of work and is not something to
     approximate by retargeting the pattern.  */
  /* RE-FORMING THE HARDWARE LOOP.  §7tricies.61.

     The doloop latch is kept rather than declined: the
     kernel's own closing instance stays a doloop_end, which reorg_loops --
     running after this pass -- consumes into a LOOPDO over the kernel, and
     every OTHER loopback instance becomes an explicit `counter -= 1; if zero
     goto drain' on the loop counter.  That is what keeps the counter's updates
     alive for the early exits while the kernel's repetition costs no
     instruction at all.

     Leaving the loop early is legal: the ISS's loop-back is the fetch
     engine's, taken when the SEQUENTIAL next bundle reaches LE, and
     "a branch taken in this bundle wins over the loop-back (and jumps out of
     the loop)" (lvx-gem5 static_inst.cc).  LC is decremented only on the
     fall-through, so an early exit simply leaves it, and the next LOOPDO
     reloads LS/LE/LC.  So the final partial trip needs no peeling -- the
     kernel's own early exits end it.

     At KUNROLL == 1 the count is the loop's own and nothing has to be
     computed: the hardware loop is exactly the one the loop already had.
     KUNROLL > 1 needs the count divided, and the division is not the whole
     story -- see ROUNDING DOWN below.  */
  bool doloop = lvx_doloop_end_p (seq[loopback]);

  /* NO GATE HERE ANY MORE.  §7tricies.81.

     A doloop latch used to be declined unless -mcms-hwloop was given, and
     that option's whole effect was this one refusal -- it generated no code
     and changed no decision anywhere else.  It was staging, and said so
     ("Off by default while incomplete").  The staging is over: the doloop and
     branch-loop arms build the same 27 loops at nstages >= 3 with identical
     nstages histograms, agree with the unpipelined reference on all 151 TSVC2
     checksums across three configurations, and the doloop arm is far cheaper
     in code -- 1.02x the dynamic bundles against 1.26x -- because the
     loopback branch is implicit and the exit test costs no issue slot.  So
     the pipeliner considers any superblock inner loop, doloop or while, and
     -fno-hwloop is no longer a prerequisite for pipelining anything.

     The two refusals below are NOT staging and stay: a power-of-two unroll
     factor is what makes the trip-count division a shift, and a kunroll > 1
     kernel needs a prologue to set the count up from.  */
  /* AN ALREADY-MAPPED LOOP IS *NOT* HANDLED, AND THE CLAIM THAT IT WAS CAME
     FROM A COMPILE-TIME COUNT.  §7tricies.196 said "handled, and measurably
     so" on the strength of 1134 such loops reaching emission; that is a
     CENSUS number, and a census stops at `-S'.  It cannot see a wrong trip
     count.  The gate's hardware-loop arm can, and with hardware loops enabled
     -mcms-after-hwloop FAILS ALL EIGHT TESTS -- `base', `codegen' and
     `pipeline' all agreeing, only `hwloop' wrong -- while the zlib
     hardware-loop round trip does not even link.  That arm had never been run
     for this order: the earlier "8/8 PASS" was taken without HW=1.
     §7tricies.198.

     THE REASON IS STRUCTURAL.  site->loopdo is RECORDED AND NEVER READ, so
     the kernel is unrolled by KUNROLL and overlapped by NSTAGES while the
     LOOPDO in the preheader still holds the ORIGINAL loop's trip count, and
     the hardware iterates the wrong number of times.  Maintaining it means
     the same `hw = M - 1' range shift the before-mapping order already
     computes for the doloop; until that exists, -mcms-after-hwloop is
     MEASUREMENT ONLY and must not become the default.

     Declining mapped loops instead was measured and is not a way out: 498 of
     them, taking emitted from 743 to 230.  The record is kept because
     site->loopdo is where the maintenance will hook, found before make_nodes
     splits the preheader edge, and the stats line reports `hwloop mapped'.  */

  *why = "a doloop kernel's unroll factor is not a power of two";
  if (doloop && (site->kunroll < 1
		 || (site->kunroll & (site->kunroll - 1)) != 0))
    return false;
  /* ROUNDING DOWN, AND WHY THERE IS NO CHOICE ABOUT IT.  §7tricies.62.

     An unrolled kernel holds KUNROLL instances of the loopback, one per copy,
     and all but the closing one are early exits.  The obvious way to keep the
     hardware loop would be to divide the count UP -- ceil (M / KUNROLL)
     trips, the exits ending the final partial trip, which the ISS permits.
     It does not work, and the reason is in hw-doloop.cc rather than in the
     architecture: `scan_loop' sets ITER_REG_USED for any insn in the loop
     other than the doloop_end that so much as MENTIONS the counter, and
     (lvx_hwloop_optimize) refuses on it.  A counted early exit mentions the
     counter by construction.  So the exits cannot stay inside the body.

     They therefore leave it, which is what rounding the count DOWN buys: the
     hardware loop runs the complete trips and a SECOND copy of the kernel,
     executed at most once after it, carries the leftover iterations and the
     exits that end them.  The leftover is never zero, so that copy needs no
     guard:

       M        REMAINING LOOPBACK INSTANCES when the kernel is reached, which
		is what the counter holds -- every remaining instance decrements
		it and the one that sees 1 exits.  This used to read "iterations
		still to start", and that is WRONG BY NSTAGES - 2: the prologue
		starts NSTAGES - 1 iterations but executes only the loopback
		instances whose dates fall inside it.  huft_build loop 6 of
		zlib's inftrees.c (nstages = 3, II = 5, sigma (loopback) = 8) has
		TWO iterations started in its prologue and ONE instance executed,
		so the counter holds N - 1 while iterations still to start is
		N - 2.  The code is right because it reads the counter and the
		consumers are instances -- KUNROLL per hardware trip, R in the
		remainder -- but the gloss sent one investigation looking for an
		off-by-one that is not there,
       T        = (M - 1) / KUNROLL, the complete trips: a trip is complete
		when all KUNROLL of its instances see a count above 1, so the
		exit always falls in the trip after the last complete one,
       R        = M - T * KUNROLL = ((M - 1) mod KUNROLL) + 1, in [1, KUNROLL].

     T may be zero, and LOOPDO with a count of zero skips the body entirely
     ("as in Tensilica's LOOPNEZ", Instruction.yml) rather than looping
     forever, so there is nothing to guard there either.

     The remainder copy is the kernel the plan already built: its KUNROLL - 1
     early exits are the explicit counter tests above, and its CLOSING
     instance -- reached exactly when R == KUNROLL -- is dropped so the block
     falls into the main epilogue, which is the drain that instance's exit
     would have gone to.

     Two registers are in play and they must be different ones: the hardware
     loop's own counter, which nothing but its doloop_end may mention, and the
     pipeline's counter live range, which the remainder copy's tests read.  */
  /* GATED, BECAUSE THE STRUCTURE BELOW IS KNOWN TO MISCOMPILE.  §7tricies.199.
     The note further down says it plainly -- "REACHABLE ONLY FOR TRACING at
     the moment: §7tricies.65 showed the loops this admits are miscompiled" --
     and nothing enforced it: the only thing keeping it out of the shipped
     configuration was that `doloop' is false under -fno-hwloop, which every
     development arm passed.  Remove that flag, as §7tricies.189 did, and the
     path emits at -O2.  §7tricies.190's zlib miscompile carries hwkern=1.

     Refusing it is not a loss of correctness anywhere: the loop falls back to
     the counted kernel the plan already built, whose KUNROLL - 1 counter
     tests make reorg_loops decline the mapping (ITER_REG_USED) so
     lvx_hwloop_fail turns it into a branch loop.  That is slower and right.  */
  bool hwkern = (doloop && site->kunroll > 1 && lvx_lms_hwkern);
  /* Every jump must have a destination outside the loop to drain to, or it is
     an intra-loop branch this emission does not model.  */
  /* ONE REGION PER EMITTING WALK.  §7tricies.212: the flow check compares
     live-range roots, which are only meaningful within a single loop's
     colouring, so each walk that emits gets its own id and the checker drops
     its OWNER map when the id changes.  */
  lvx_lms_flow_region++;

  *why = "a jump has no exit out of the loop";
  for (unsigned i = 0; i < seq.length (); i++)
    if (JUMP_P (seq[i]) && (int) i != loopback
	&& !lvx_lms_jump_exit_dest (site, seq[i]))
      return false;

  /* The first kernel node is the branch target that closes the loop, and the
     main epilogue carries the ITER_SLOT instance -- which is how an emitted
     loopback is told from an early exit.  Reading the kernel node's own
     EXIT_DATE would not work: a kernel node has none, it is -1.  */
  int kfirst = -1, mainep = -1;
  for (unsigned k = 0; k < nnode; k++)
    if (nodes[k].kind == LVX_LMS_NODE_KERNEL)
      {
	kfirst = (int) k;
	break;
      }
  for (unsigned k = 0; k < nnode; k++)
    if (nodes[k].kind == LVX_LMS_NODE_EPILOGUE && nodes[k].main_epilogue)
      mainep = (int) k;
  *why = "no kernel node or no main epilogue";
  if (kfirst < 0 || mainep < 0)
    return false;

  /* A NOTE ON WHAT IS *NOT* A PRECONDITION.  The plan splits the kernel at
     every exit and the loopback's own KUNROLL - 1 non-closing instances ARE
     exits, so a kernel node count of one is not the condition.  Nor is the
     absence of a data-dependent exit: the exit-free copy carries those too, as
     a chain of blocks, each branching to the SAME drain the leftover copy's
     instance branches to.  "Exit-free" means free of the COUNTED exits, which
     is the whole of what hw-doloop.cc's ITER_REG_USED forbids.

     REACHABLE ONLY FOR TRACING at the moment: §7tricies.65 showed the loops
     this admits are miscompiled, by something that is neither the drain nor
     bundle speculation, so the chain is here to let the executing instance
     set be traced against the plan.  */
  *why = "a doloop kernel with kunroll > 1 has no prologue to set up from";
  if (hwkern && kfirst == 0)
    return false;

  /* A SIDE EFFECT IN THE PROLOGUE IS NOT THE SAME AS A SPECULATED ONE.
     §7tricies.42.

     This refused any store, call or volatile reference landing in a prologue
     node, on the reading that "the prologue executes the early stages of
     iterations 0 .. nstages - 2 BEFORE any exit test has run, so every
     instruction it holds is speculated".  The premise is false in both
     directions, and the CONTROL DEPENDENCE is what makes it false.

     WHAT ADMITS AN ITERATION.  Iteration 0 is admitted by ENTERING THE LOOP:
     the pipeline replaces the original loop and is reached only on the same
     condition, so iteration 0 always runs and nothing of it is speculated.
     Iteration l >= 1 is admitted by the loopback branch of iteration l - 1.

     AND THE GRAPH ALREADY ORDERS THAT.  LVX_ARC_CTRL_STORE is pushed from
     every branch to every store in the block that follows it, at theta = 1
     and -- for a store that precedes the branch in body order, which is the
     shape of every bottom-tested loop -- at omega = 1.  So

         sigma (store) - sigma (branch) >= 1 - II,

     and therefore, instance by instance,

         date (store, l) - date (branch, l - 1)
           = sigma (store) - sigma (branch) + II >= 1,

     STRICTLY after the branch that admits it.  The intra-cycle order
     (§7tricies.35) forces the terminator last in its cycle, so `strictly'
     is what is needed: a store sharing the branch's cycle would be emitted
     before it.  Every jump instance before ITER_SLOT is emitted as an early
     exit, so when that branch is taken the loop leaves for an epilogue and
     the store is never reached.

     So no prologue side effect is speculated, and the refusal was refusing
     the thing the arc exists to make safe.  The measurement that motivated
     it -- main's initialisation loop storing past the end of both arrays --
     was taken while two other defects were live: the kernel window dropped
     branch instances (§7tricies.34) and the early-exit count was short, so
     the admitting branches were not all being emitted.  Both are fixed.

     WHAT REPLACES IT IS THE CHECK, not the assumption.  Each prologue side
     effect is asked whether its admitting instance precedes it, which is
     cheap, is decided on the plan, and refuses the loop if the arc is ever
     absent or the dates disagree -- rather than trusting that it is there.  */
  *why = "a prologue side effect precedes the branch that admits it";
  int nse0 = 0, nsel = 0, nse0g = 0, nse0u = 0;
  for (unsigned q = 0; q < slots.length (); q++)
    {
      int nd = slots[q].node;
      unsigned i = (unsigned) slots[q].insn;
      bool sideeff = false;

      if (nd < 0 || nd >= (int) nnode || i >= seq.length ())
	continue;
      if (nodes[nd].kind != LVX_LMS_NODE_PROLOGUE)
	continue;

      /* A CALL IS STILL REFUSED OUTRIGHT.  Loop eligibility declines a body
	 holding one, so this is unreachable; it is kept because `admitted by
	 its branch' says nothing about a call's other effects.  */
      if (CALL_P (seq[i]))
	{
	  *why = "the prologue would speculate a call";
	  return false;
	}

      /* THE SAME DETECTION THE ARC BUILDER USED, so that what is checked here
	 is what was constrained there.  A MEM in a pattern with no single_set
	 is the case neither can read precisely, and stays a side effect.  */
      if (lvx_mem_write_insn_p (seq[i]) || volatile_insn_p (PATTERN (seq[i])))
	sideeff = true;
      else
	{
	  subrtx_iterator::array_type array;
	  rtx pat = PATTERN (seq[i]);
	  rtx set = single_set (seq[i]);

	  FOR_EACH_SUBRTX (it, array, pat, NONCONST)
	    if (MEM_P (*it) && (!set || *it == SET_DEST (set)))
	      {
		sideeff = true;
		break;
	      }
	}
      if (!sideeff)
	continue;

      /* Iteration 0 is admitted by ENTERING the loop -- but only where
	 entering means the body runs once, which site.entry_admits records
	 and §7tricies.43 explains.  Where it does not, this is the
	 speculation the blanket refusal was named for, and it is refused.  */
      if (slots[q].iter <= 0)
	{
	  /* GUARDED BY AN EARLIER EXIT OF ITS OWN ITERATION.  §7tricies.177.

	     ENTRY_ADMITS is sufficient, not necessary, and it is the only
	     thing this case used to consult -- where the iteration >= 1 case
	     below asks the sharper question, whether the admitting branch is
	     actually emitted ahead of the side effect.  Asked here too, the
	     answer is often yes: a side effect downstream of an early exit of
	     the SAME iteration is guarded by that exit exactly as the original
	     program guarded it.

	     That rests on the emission, which was read rather than assumed.
	     `prev_exit = JUMP_P (seq[i])' ends a plan node at every jump
	     instance, so the exit and anything after it land in DIFFERENT
	     blocks; each early exit is paired with its own epilogue node by
	     (exit_date, exit_iter, exit_insn); and that epilogue's block is
	     redirected with `redirect_edge_and_branch_force' to
	     `lvx_lms_jump_exit_dest', the destination the original jump had.
	     So the branch really is emitted as a branch out, and a later node
	     is on its fall-through path.

	     The epilogue pairing is required rather than JUMP_P alone: a jump
	     with no early-exit epilogue is not emitted as an exit here (the
	     drain emits it as the straight line it is), and would guard
	     nothing.  */
	  bool guarded = false;

	  for (unsigned r = 0; r < q && !guarded; r++)
	    {
	      unsigned j = (unsigned) slots[r].insn;

	      if (slots[r].iter != slots[q].iter
		  || j >= seq.length () || !JUMP_P (seq[j]))
		continue;
	      for (unsigned e = 0; e < nnode; e++)
		if (nodes[e].kind == LVX_LMS_NODE_EPILOGUE
		    && !nodes[e].main_epilogue
		    && nodes[e].exit_date == slots[r].date
		    && nodes[e].exit_iter == slots[r].iter
		    && nodes[e].exit_insn == (int) j)
		  {
		    guarded = true;
		    break;
		  }
	    }
	  if (guarded)
	    {
	      nse0g++;
	      lvx_lms_dump (";; lvx cms:   prologue iter0 side effect insn=%u"
			    " date=%d guarded by an earlier exit\n", i,
			    slots[q].date);
	      continue;
	    }

	  /* NO BRANCH OF ITS ITERATION PRECEDES IT, SO ENTERING IS THE GUARD.
	     §7tricies.189.

	     ENTRY_ADMITS asks whether the body runs TO COMPLETION once per
	     entry.  That is the right question for a side effect some branch of
	     the body governs, and it is the WRONG one -- strictly too strong --
	     for one that no branch governs: a slot with no jump before it in the
	     body's program order is performed by every iteration that BEGINS,
	     and iteration 0 begins exactly when the loop is entered.

	     The prologue is where that becomes exact rather than plausible.
	     lvx_lms_make_nodes puts every prologue block on
	     `split_edge (find_edge (preheader, header))', so control reaches the
	     prologue on precisely the paths that reached the header.  Performing
	     iteration 0's ungoverned side effect there is therefore not
	     speculation: it is the same unconditional execution loop entry
	     already performed.

	     PROGRAM ORDER IS THE TEST, not the schedule's.  SEQ is ONE body copy
	     -- lvx_lms_build_plan replicates it for KUNROLL -- so `no jump below
	     I' is a per-iteration statement, and an insn in a later unrolled copy
	     still has the earlier copy's jumps before it and is refused.  With
	     JOINS refused at admission the body is a chain with exits off it, so
	     program order IS dominance order and no earlier branch means no
	     branch at all.

	     AND THE SKIP DIRECTION NEEDS NO SEPARATE TEST, because the scan above
	     has already consumed it.  The worry is the mirror of speculation: an
	     exit emitted BEFORE a store the original performed unconditionally
	     would LOSE the effect rather than speculate it.  But an earlier
	     same-iteration jump paired with its own epilogue is exactly what
	     GUARDED matched, so reaching here means every earlier jump of this
	     iteration is unpaired -- and §7tricies.177 records that an unpaired
	     jump is emitted as the straight line it is, which guards nothing and
	     therefore skips nothing.

	     The corpus' two are jpeg_calc_output_dimensions loop 1 (insn 0, the
	     body's first insn) and LZWDecode loop 5 (insn 1, preceded only by an
	     add).  Both print ENTRYADMITS=0 for NOTLATCH=1 alone -- an early exit
	     AFTER the side effect, which cannot make it conditional.  */
	  {
	    bool governed = false;

	    for (unsigned j = 0; j < i && !governed; j++)
	      if (JUMP_P (seq[j]))
		governed = true;
	    if (!governed)
	      {
		nse0u++;
		lvx_lms_dump (";; lvx cms:   prologue iter0 side effect insn=%u"
			      " date=%d ungoverned, entering performs it\n", i,
			      slots[q].date);
		continue;
	      }
	  }

	  if (!site->entry_admits)
	    {
	      *why = "the prologue would speculate iteration 0's side effect";
	      lvx_lms_dump (";; lvx cms:   prologue iter0 side effect insn=%u"
			    " date=%d, entry does not admit it\n", i,
			    slots[q].date);
	      return false;
	    }
	  nse0++;
	  continue;
	}
      nsel++;

      /* Otherwise the loopback instance of the previous iteration must be
	 emitted strictly earlier.  */
      {
	int admit = -1;

	for (unsigned r = 0; r < slots.length (); r++)
	  if (slots[r].insn == loopback
	      && slots[r].iter == slots[q].iter - 1
	      && (admit < 0 || slots[r].date < admit))
	    admit = slots[r].date;
	if (admit < 0 || admit >= slots[q].date)
	  {
	    lvx_lms_dump (";; lvx cms:   prologue side effect insn=%u iter=%d"
			  " date=%d admit=%d\n", i, slots[q].iter,
			  slots[q].date, admit);
	    return false;
	  }
      }
    }

  /* How many prologue side effects each kind of admission covered.  A zero
     NSEL would mean the iteration >= 1 case is unexercised by the corpus,
     which is worth knowing about a path that is trusted to the arc.  */
  lvx_lms_dump (";; lvx cms:   prologue sideeffects iter0=%d iter0guarded=%d"
		" iter0ungoverned=%d iterN=%d\n", nse0, nse0g, nse0u, nsel);

  /* THE EPILOGUES LEAVE THE CHAIN FIRST, before any insn is emitted: once
     they are redirected the chain no longer reaches the header, so nothing
     that follows can be confused by a fall-through into the old body.
     `redirect_edge_and_branch_force' is the entry that will create the jump a
     fall-through block has not got -- `redirect_edge_and_branch' refuses a
     fall-through outright, which is the distinction make_nodes' long note
     records.  */
  /* THE LIVE-OUT COPIES' STRUCTURAL REFUSALS, DECIDED BEFORE ANYTHING IS
     ANYTHING IS MUTATED.  §7tricies.72.

     The copies themselves are emitted after the slot loop, which is where
     they belong -- they go at the end of each drain.  Their REFUSALS do not:
     a `return false' from there happens with the pipeline already written
     into the blocks, which is §7tricies.56's defect exactly, "a refusal that
     has already mutated the CFG", and this reintroduced it.

     AND "BEFORE EMISSION" IS NOT EARLY ENOUGH EITHER.  The first mutation is
     the epilogue redirection below, not the slot loop: a refusal placed
     between the two still leaves every drain redirected to a loop exit it
     will now never reach, which on a 256-bit reduction lost the vector
     loop's entire sum rather than one iteration of it.  So this sits ahead
     of the redirection, and anything else that refuses on a property of the
     LIVE RANGES belongs here with it.

     It was invisible until §7tricies.71 let these loops reach emission at
     all: before that they were refused earlier, by the anti-dependence check.
     The symptom was a 256-bit reduction losing its last vector iteration --
     for n = 13, a sum low by B[8] + B[9] + B[10] + B[11], one group of four
     -- because the half-emitted loop ran a kernel whose drain never
     materialised.

     So every refusal that is a property of the LIVE RANGES rather than of the
     emission is taken here.  What remains late is the parallel copy's own
     sequentialisation and the recognition of a hard-register move, and those
     are noted at their site.  */
  for (unsigned g = 0; g < lr.length (); g++)
    {
      lvx_lms_lrange_part *q = lr[g];

      if (q->effects.is_empty () || !q->exit_live
	  || lvx_lms_lrange_root (lr, (int) g) != (int) g
	  || !q->effects[0].is_def)
	continue;
      /* A TUPLE IS NOT REFUSED.  §7tricies.73: a tuple copy IS the parallel
	 copy of its component registers, so it is decomposed into them and
	 the register-level sequentialisation does the rest.  */
      *why = "a live-out range's definition has no operand";
      if (q->effects[0].insn >= seq.length ()
	  || !lvx_lms_reg_at (&PATTERN (seq[q->effects[0].insn]),
			      q->effects[0].slot))
	return false;
    }

  /* NOTHING MAY FOLLOW A PATH'S TERMINATOR, DECIDED BEFORE ANYTHING IS
     MUTATED.  §7tricies.131.

     A path node becomes one basic block and GCC gives a block ONE terminator,
     at the end.  So if any slot is emitted after this node's jump, the result
     is a jump inside a basic block, which the RTL verifier rejects -- "flow
     control insn inside a basic block", four files across TSVC2, MiBench,
     PolyBench and lvx-kernels on the compiler before this.

     THE CONDITION IS NOT TWO BRANCHES IN A CYCLE, which is what the schedule
     of zlib's inflate_fast made it look like (`8:1/0J 9:1/0J', two jumps at
     one date).  Measured on the gate's own earlyexit.c the node carries ONE
     jump slot and the blocked slot is an ORDINARY instruction: a node spans a
     range of dates, lvx_lms_slot_order puts the terminator last only WITHIN A
     CYCLE (§7tricies.35), and an ordinary slot at a later date in the same
     node therefore follows it.  Two branches in one cycle is a real and
     separate question -- the architecture allows it, BCU availability being 2
     and the ISS resolving BCU0-first (lvx-gem5 tests/lvx/diff/brprio.s) -- but
     it is not what these failures are.

     SO THE TEST IS THE EMITTER'S OWN: walk the order emission will walk, and
     refuse if any slot follows a jump on its node.  Asking
     lvx_lms_slot_order here rather than reasoning about dates means the check
     cannot drift from the order it is checking.

     AND IT IS TAKEN HERE because two earlier attempts refused from inside the
     emit loop, and both were unsound for the reason §7tricies.72 records: by
     then the epilogues are redirected and part of the pipeline is written
     into the blocks, so a refusal leaves a half-built loop -- a WRONG ANSWER
     rather than a declined one.  The gate's earlyexit.c returned
     4581331092030434362 against an oracle of -479514617468511649 that way.
     Everything below this line mutates; this is the last place to say no.  */
  {
    auto_vec<int> probe_order;
    auto_vec<bool> closed;

    *why = "a cycle's reads and writes cannot be linearised";
    if (!lvx_lms_slot_order (seq, slots, lr, loopback, probe_order))
      return false;

    auto_vec<int> closedby, closedat;

    closed.safe_grow_cleared (nnode);
    closedby.safe_grow_cleared (nnode);
    closedat.safe_grow_cleared (nnode);
    for (unsigned k = 0; k < nnode; k++)
      closedby[k] = closedat[k] = -1;
    for (unsigned oq = 0; oq < probe_order.length (); oq++)
      {
	unsigned q = (unsigned) probe_order[oq];
	int nd = slots[q].node;
	unsigned i = (unsigned) slots[q].insn;

	if (nd < 0 || nd >= (int) nnode || i >= seq.length ())
	  continue;
	*why = "a slot follows its own path's terminator";
	if (closed[nd])
	  {
	    /* WHICH slot followed WHICH terminator, and on what node.  The
	       node boundaries were assigned by the plan walk in (date, iter)
	       order; this check walks lvx_lms_slot_order, the order emission
	       uses.  Where the two disagree a slot can land after its own
	       node's jump, and only the pair tells which it was.  */
	    lvx_lms_dump (";; lvx cms:   TWOTERM node=%d kind=%d blocked"
			  " insn=%u date=%d iter=%d jmp=%d, closed by insn=%d"
			  " date=%d\n", nd, (int) nodes[nd].kind, i,
			  slots[q].date, slots[q].iter,
			  JUMP_P (seq[i]) ? 1 : 0, closedby[nd], closedat[nd]);
	    return false;
	  }
	if (JUMP_P (seq[i]))
	  {
	    closed[nd] = true;
	    closedby[nd] = (int) i;
	    closedat[nd] = slots[q].date;
	  }
      }
  }

  *why = "cannot redirect an epilogue to the loop exit";
  for (unsigned k = 0; k < nnode; k++)
    {
      /* EACH DRAIN ENDS WHERE ITS OWN EXIT WENT.  §7tricies.45.  Every
	 epilogue used to be redirected to SITE->EXIT_DEST, the latch's
	 out-of-loop successor, which is right only while there is one exit.
	 A drain for a jump that left the loop elsewhere has to arrive where
	 that jump went; the MAIN epilogue is the ITER_SLOT fall-out and does
	 belong to the latch.  */
      basic_block dest = site->exit_dest;

      if (nodes[k].kind != LVX_LMS_NODE_EPILOGUE)
	continue;
      if (!nodes[k].main_epilogue && nodes[k].exit_insn >= 0
	  && nodes[k].exit_insn < (int) seq.length ())
	{
	  dest = lvx_lms_jump_exit_dest (site, seq[nodes[k].exit_insn]);
	  if (!dest)
	    return false;
	}
      if (!single_succ_p (blocks[k]))
	return false;
      if (single_succ (blocks[k]) == dest)
	continue;
      /* The return value is the block this had to SPLIT, and null on the
	 ordinary path -- so testing it for failure would fail on success.
	 The function asserts rather than returning on a redirect it cannot
	 do, which is why there is nothing to test.  */
      redirect_edge_and_branch_force (single_succ_edge (blocks[k]), dest);
    }

  /* EMIT.  The slot list is already in (date, iteration, order), which is the
     order the instructions have to appear in, so this is one pass with no
     sorting and no per-block bookkeeping beyond where the last insn went.  */
  auto_vec<rtx_insn *> tail;
  for (unsigned k = 0; k < nnode; k++)
    tail.safe_push (NULL);

  /* The order the slots are emitted in: reads before writes within a cycle,
     the terminator last (§7tricies.35).  Asked again here rather than passed
     in, so the emission cannot be handed an order computed from a different
     assignment.  */
  auto_vec<int> order;

  *why = "a cycle's reads and writes cannot be linearised";
  if (!lvx_lms_slot_order (seq, slots, lr, loopback, order))
    return false;
  for (unsigned oq = 0; oq < order.length (); oq++)
    {
      unsigned q = (unsigned) order[oq];
      int nd = slots[q].node;
      unsigned i = (unsigned) slots[q].insn;
      lvx_lms_ufread_node = nd;
      lvx_lms_ufread_kind = (nd >= 0 && nd < (int) nnode) ? nodes[nd].kind : -1;
      if (nd < 0 || nd >= (int) nnode || i >= seq.length ())
	continue;
      basic_block bb = blocks[nd];
      bool is_iter = ((int) i == loopback
		      && slots[q].date == nodes[mainep].exit_date
		      && slots[q].iter == nodes[mainep].exit_iter);

      /* Where to put it: after the last insn this block received, or after
	 the block note on the first.  BB_HEAD can be a CODE_LABEL and the
	 note follows it -- §7tricies.8, and the reason phase one's first
	 version failed the verifier twice.  */
      rtx_insn *after = tail[nd];
      if (!after)
	{
	  after = BB_HEAD (bb);
	  if (LABEL_P (after))
	    after = NEXT_INSN (after);
	  if (!after || !NOTE_INSN_BASIC_BLOCK_P (after))
	    {
	      *why = "no NOTE_INSN_BASIC_BLOCK to emit after";
	      return false;
	    }
	}

      /* A PATH THAT HAS ALREADY BRANCHED CANNOT RECEIVE MORE.  §7tricies.130.

	 TAIL[nd] is where this path's next slot goes, and when the previous
	 slot on it was a jump that is the block's TERMINATOR -- emitting after
	 it leaves a flow-control insn inside a basic block, which the RTL
	 verifier rejects outright ("flow control insn inside a basic block",
	 rtl_verify_bb_insns).

	 lvx_lms_slot_order puts the terminator last, but it does so WITHIN A
	 CYCLE (§7tricies.35), and that is not the same thing: in a multi-exit
	 body a data-dependent exit can be scheduled in an earlier cycle than
	 other slots on its own path, and then the order is correct per cycle
	 and wrong per block.  Reachable in practice -- count_bits in MiBench's
	 lame takehiro.c at II=7, which is its lower bound, ICEd here.

	 Declining is the conservative half of the fix: the SCHEDULE is legal,
	 so what is missing is the ability to express it, which needs the block
	 split after the branch and the remaining slots emitted into the
	 fall-through successor -- what the hardware-loop chain already does
	 for its own exits.  Until then CMS gives this II up rather than
	 emitting RTL the verifier will reject.  */
      if (after && JUMP_P (after))
	{
	  *why = "a slot follows its own path's exit branch";
	  return false;
	}

      if (JUMP_P (seq[i]))
	{
	  /* THE BRANCH.  Every jump goes through here, not just the loopback:
	     the test was `i == loopback', which for a multi-exit body sent the
	     OTHER jump down the ordinary-instruction path below, where
	     emit_insn_after_noloc copies a jump pattern into a non-JUMP insn.
	     The CFG then knows nothing of it while the assembler still prints a
	     branch to the original label, so the label is never emitted:
	     `undefined reference to .L8', caught by validation/cms/twoexit.c
	     and by nothing that merely compiles (§7tricies.45).

	     What differs between the kinds is where it goes and whether the
	     condition is inverted.  */
	  rtx_insn *j
	    = emit_jump_insn_after_noloc (copy_insn (PATTERN (seq[i])), after);
	  /* The copy carries the original label_ref, so JUMP_LABEL is set to
	     the original target before retargeting: `redirect_jump' and
	     `invert_jump' both read it as the label to replace, and a null one
	     leaves the copy ill-formed rather than merely untargeted.  */
	  JUMP_LABEL (j) = JUMP_LABEL (seq[i]);
	  if (JUMP_LABEL (j) && LABEL_P (JUMP_LABEL (j)))
	    LABEL_NUSES (JUMP_LABEL (j))++;
	  rtx_jump_insn *jj = as_a <rtx_jump_insn *> (j);
	  basic_block target;

	  if (is_iter && hwkern)
	    {
	      /* THE REMAINDER COPY DOES NOT LOOP BACK.  Its closing instance
		 is reached only when R == KUNROLL, and then it always exits,
		 so it is not a branch at all: drop it and let the block fall
		 into the main epilogue, which is where its exit went.  The
		 counter it would have decremented is dead from here on.

		 JUMP_LABEL is left as it was set above: delete_insn reads it
		 to drop the LABEL_NUSES this emission had just claimed.  */
	      delete_insn (j);
	      lvx_lms_dump (";; lvx cms:   closing branch dropped, the"
			    " remainder copy falls into the drain\n");
	      continue;
	    }
	  else if (is_iter)
	    {
	      target = blocks[kfirst];
	      if (!redirect_jump (jj, block_label (target), 0))
		{
		  *why = "cannot retarget the kernel's closing branch";
		  return false;
		}
	    }
	  else if (lvx_doloop_end_p (seq[i]))
	    {
	      /* AN EARLY EXIT OF A DOLOOP IS NOT A COPY OF IT.  Only one
		 doloop_end may exist per loop -- it is what marks LE -- so
		 every instance that is not the kernel's closing one is
		 emitted as the explicit form reorg_loops itself falls back to
		 (lvx_hwloop_fail): decrement the counter, and branch out when
		 it reaches zero.  The sense is inverted against that fallback
		 because this is an EXIT, not a loop-back.

		 The counter is read and written at one operand, so its
		 live range gives one register for both halves of the
		 decrement -- which is sound only if that live range does not
		 rotate, since a rotating one would put the next iteration's
		 counter in a different name than the decrement wrote.

		 Asked per INSTANCE, not per loop: a multi-exit body's other
		 jump is an ordinary cbranch whose pattern is not a PARALLEL,
		 so the loop-level flag sent it in here to read an operand it
		 has not got (validation/cms/twoexit.c).  */
	      int ep = -1;
	      rtx cnt = SET_DEST (XVECEXP (PATTERN (seq[i]), 0, 1));
	      int cslot = lvx_lms_reg_index (PATTERN (seq[i]),
					     &SET_DEST (XVECEXP (PATTERN
								   (seq[i]),
								 0, 1)));
	      bool cdly = false;
	      lvx_lms_lrange_part *cg = lvx_lms_lrange_of (lr, i, cslot, &cdly);
	      int creg = cg ? lvx_lms_lrange_reg (cg, slots[q].iter, cdly)
			    : (REG_P (cnt) ? (int) REGNO (cnt) : -1);
	      machine_mode cm = GET_MODE (cnt);
	      rtx creg_rtx;

	      for (unsigned e = 0; e < nnode; e++)
		if (nodes[e].kind == LVX_LMS_NODE_EPILOGUE
		    && !nodes[e].main_epilogue
		    && nodes[e].exit_date == slots[q].date
		    && nodes[e].exit_iter == slots[q].iter
		    && nodes[e].exit_insn == (int) i)
		  ep = (int) e;
	      *why = "a doloop early exit has no epilogue";
	      if (ep < 0 || creg < 0 || !REG_P (cnt))
		return false;
	      *why = "the doloop counter's range rotates";
	      if (cg && cg->p2unroll != 1)
		return false;
	      creg_rtx = gen_rtx_REG (cm, creg);

	      /* The copied jump is not wanted: delete it and put the two
		 instructions in its place.  */
	      delete_insn (j);
	      {
		rtx decr = (cm == SImode
			    ? gen_addsi3 (creg_rtx, creg_rtx, constm1_rtx)
			    : gen_adddi3 (creg_rtx, creg_rtx, constm1_rtx));
		rtx_insn *di = emit_insn_after_noloc (decr, after, NULL);
		rtx test = gen_rtx_EQ (VOIDmode, NULL_RTX, NULL_RTX);
		rtx cb = (cm == SImode
			  ? gen_cbranchsi4 (test, creg_rtx, const0_rtx,
					    block_label (blocks[ep]))
			  : gen_cbranchdi4 (test, creg_rtx, const0_rtx,
					    block_label (blocks[ep])));
		rtx_insn *cj = emit_jump_insn_after_noloc (cb, di);

		JUMP_LABEL (cj) = block_label (blocks[ep]);
		LABEL_NUSES (JUMP_LABEL (cj))++;
		if (!find_edge (bb, blocks[ep]))
		  {
		    edge ne = make_edge (bb, blocks[ep], 0);
		    edge fe;

		    ne->probability = profile_probability::even ();
		    fe = find_fallthru_edge (bb->succs);
		    if (fe)
		      fe->probability = profile_probability::even ();
		  }
		/* §7tricies.218: an EPILOGUE insn is left untagged, which is
		   what leaves its block to SCHED2 -- lvx_lms_own_blocks
		   derives BB_DISABLE_SCHEDULE from these tags, and the group
		   invariant must not be asserted over a block CMS no longer
		   owns.  */
		if (!lvx_lms_sched2_epilogue
		    || nodes[nd].kind != LVX_LMS_NODE_EPILOGUE)
		  lvx_lms_group_tag (after, nd, slots[q].date,
				     slots[q].iter * 65536 + (int) i);
		tail[nd] = cj;
		nemit++;
		lvx_lms_dump (";; lvx cms:   doloop early exit insn=%u iter=%d"
			      " counter=r%d -> node %d\n", i, slots[q].iter,
			      creg, ep);
		continue;
	      }
	    }
	  else
	    {
	      /* Its own epilogue: the node whose EXIT_DATE, EXIT_ITER and
		 EXIT_INSN are this instance.  EXIT_INSN is part of the key
		 because two jumps of a multi-exit body can have instances at
		 one date and iteration, and the drain they need is not the
		 same one (§7tricies.45).  */
	      int ep = -1;
	      for (unsigned e = 0; e < nnode; e++)
		if (nodes[e].kind == LVX_LMS_NODE_EPILOGUE
		    && !nodes[e].main_epilogue
		    && nodes[e].exit_date == slots[q].date
		    && nodes[e].exit_iter == slots[q].iter
		    && nodes[e].exit_insn == (int) i)
		  ep = (int) e;
	      if (ep < 0)
		{
		  *why = "an exit has no epilogue";
		  return false;
		}
	      target = blocks[ep];
	      /* INVERT ONLY A JUMP THAT CONTINUES THE LOOP WHEN TAKEN.  The
		 loopback does, so leaving has to become the taken direction.
		 A top test does not -- it already exits when taken -- and
		 inverting it would send the CONTINUING case to the drain.  */
	      if (lvx_lms_jump_taken_stays_p (site, seq[i]))
		{
		  if (!invert_jump (jj, block_label (target), 0))
		    {
		      *why = "cannot invert an early exit";
		      return false;
		    }
		}
	      else if (!redirect_jump (jj, block_label (target), 0))
		{
		  *why = "cannot retarget an early exit";
		  return false;
		}
	    }

	  /* THE BRANCH'S OPERANDS ARE RENAMED TOO, which the first version
	     omitted -- it emitted the jump, retargeted it, and moved on.  A
	     branch reads the induction variable like any other consumer: the
	     ITER_SLOT instance is the loopback at iteration kunroll - 1, so it
	     must compare that iteration's IV name, not name 0.  Emitted
	     unrenamed, f1's closing test compared a pointer already advanced
	     kunroll times past the right one and the loop never exited -- the
	     ISS ran until it was killed, which is how an omission in the
	     emission looks from outside.

	     After the retarget, because the label is not a register and the
	     substitution walks only REGs.  */
	  {
	    int n = 0;
	    if (!lvx_lms_rename_lr_insn_p (seq[i], j, i, slots[q].iter, lr,
					   &n, true))
	      {
		*why = "the renamed branch does not recognise";
		return false;
	      }
	  }

	  if (!find_edge (bb, target))
	    {
	      edge ne = make_edge (bb, target, 0);
	      /* Two successors now, and verify_flow_info wants their
		 probabilities to sum to one.  An even split is a placeholder
		 and says so: the real figures are the kernel's trip estimate
		 for the closing branch and near-never for an early exit, and
		 getting them right is a quality question, not a correctness
		 one -- but leaving them uninitialised is neither.  */
	      ne->probability = profile_probability::even ();
	      edge fe = find_fallthru_edge (bb->succs);
	      if (fe)
		fe->probability = profile_probability::even ();
	    }
	  /* §7tricies.218: see above -- an epilogue insn carries no group.  */
	  if (!lvx_lms_sched2_epilogue
	      || nodes[nd].kind != LVX_LMS_NODE_EPILOGUE)
	    lvx_lms_group_tag (after, nd, slots[q].date,
			       slots[q].iter * 65536 + (int) i);
	  tail[nd] = j;
	  nemit++;
	  continue;
	}

      /* An ordinary instruction: a copy, renamed for this iteration.  */
      rtx_insn *c
	= emit_insn_after_noloc (copy_insn (PATTERN (seq[i])), after, NULL);
      int n = 0;
      /* RESOLVED ON THE ORIGINAL, APPLIED TO THE COPY.  copy_insn produces an
	 insn DF has never seen, so the references are looked up on SEQ[I] and
	 written to C by operand position (§7tricies.27).  */
      if (!lvx_lms_rename_lr_insn_p (seq[i], c, i, slots[q].iter, lr, &n,
				     true, lvx_lms_probe != 0))
	{
	  *why = "the renamed copy does not recognise";
	  return false;
	}
      lvx_lms_group_tag (after, nd, slots[q].date,
			     slots[q].iter * 65536 + (int) i);
      tail[nd] = c;
      nemit++;
    }

  /* THE EPILOGUE COPIES FOR THE LIVE-OUT LIVE RANGES.  §7tricies.63 and .66.

     A modulo-expanded live range ends its last instance in one of its rotating
     names, while the code after the loop reads the register it was
     originally allocated to.  So each drain ends by moving the value back:
     one copy per live-out live range, from the name that live range's LAST
     executed instance on THAT path wrote.

     Which name is known exactly and needs no analysis -- it is
     reg[(L + use_first - delayed) & (p2unroll - 1)] with L the greatest
     iteration whose heading definition appears on the path into this
     epilogue, taken with the same path filter lvx_lms_verify_slots uses so
     the two agree about what a path is.  If no definition appears at all the
     live-out value is the preheader's, which a USE_FIRST live range's pinned
     name 0 already holds, and nothing is copied.

     THE COPIES ARE PARALLEL.  One live range's source name can be another's
     destination register, so emitting them in an arbitrary order clobbers a
     source before it is read, and a cyclic permutation has no valid order at
     all.  They are therefore sequentialised properly -- Boissinot et al.,
     CGO 2009, Algorithm 1 -- which costs one extra copy per cycle through a
     fresh register and nothing otherwise.  */
  for (unsigned k = 0; k < nnode; k++)
    {
      auto_vec<int> psrc, pdst, osrc, odst;
      auto_vec<machine_mode> pmode, omode;
      unsigned HOST_WIDE_INT free_regs = avail;
      int scratch = -1;
      rtx_insn *after;

      if (nodes[k].kind != LVX_LMS_NODE_EPILOGUE)
	continue;

      for (unsigned g = 0; g < lr.length (); g++)
	{
	  lvx_lms_lrange_part *q = lr[g];
	  unsigned dins;
	  int dslot, best = -1, nm, nr;
	  rtx *dloc;

	  if (q->effects.is_empty () || !q->exit_live
	      || lvx_lms_lrange_root (lr, (int) g) != (int) g
	      || !q->effects[0].is_def)
	    continue;
	  dins = q->effects[0].insn;
	  dslot = q->effects[0].slot;
	  if (dins >= seq.length ())
	    continue;

	  /* The last instance of the heading definition on this path.  */
	  for (unsigned z = 0; z < slots.length (); z++)
	    {
	      int nd = slots[z].node;
	      bool is_ep;

	      if (nd < 0 || nd >= (int) nnode
		  || (unsigned) slots[z].insn != dins)
		continue;
	      is_ep = (nodes[nd].kind == LVX_LMS_NODE_EPILOGUE);
	      if (is_ep && nd != (int) k)
		continue;
	      if (!is_ep
		  && (slots[z].date > nodes[k].exit_date
		      || (slots[z].date == nodes[k].exit_date
			  && (slots[z].iter > nodes[k].exit_iter
			      || (slots[z].iter == nodes[k].exit_iter
				  && slots[z].insn > nodes[k].exit_insn)))))
		continue;
	      if (slots[z].iter > best)
		best = slots[z].iter;
	    }
	  if (best < 0)
	    {
	      /* "LIVE-IN WINS" WAS A CONSEQUENCE OF THE PIN.  §7tricies.156.

		 On a path where no instance of the heading definition ran,
		 the architected register still holds what the preheader put
		 there -- so there was nothing to copy.  That is only true
		 while name 0 IS the architected register.  Once the entry
		 copy moves the incoming value into a name of its own, this
		 path leaves the architected register STALE and no copy
		 behind, which is the §7tricies.154 defect over again: a
		 promise discharged on one set of paths and assumed on
		 another.

		 Such a path drains name 0 and nothing else, because name 0
		 is what the entry copy wrote and no definition has advanced
		 the rotation.  Measured on zlib's inflate_blocks loop 5,
		 which has five use-headed ranges and seven exit paths out of
		 a five-slot prologue, so most of its drains are exactly this
		 case.  */
	      if (!lvx_lms_preheader_copy || !(lvx_lms_ph_parts & 1)
		  || !q->use_first || !q->renamed
		  || q->reg[0] < 0 || q->reg[0] == (int) q->regno)
		continue;
	      nm = q->reg[0];
	    }
	  else
	    nm = lvx_lms_lrange_reg (q, best, q->effects[0].delayed);
	  if (nm < 0 || nm == (int) q->regno)
	    continue;		/* already in the architectural register */
	  nr = MAX (q->nregs, 1);
	  /* Both structural refusals were taken before emission began
	     (§7tricies.72); these are the same conditions, asserted rather
	     than decided, so that a change on one side cannot silently
	     reintroduce a late refusal.  */
	  dloc = lvx_lms_reg_at (&PATTERN (seq[dins]), dslot);
	  gcc_checking_assert (dloc && *dloc && REG_P (*dloc));
	  if (!dloc || !*dloc || !REG_P (*dloc))
	    continue;

	  /* A TUPLE COPY IS THE PARALLEL COPY OF ITS COMPONENTS.
	     §7tricies.73.

	     `[regno, regno+n) <- [nm, nm+n)' is componentwise by definition,
	     so it is pushed as n single-register copies and the existing
	     register-level sequentialisation handles the rest.  That is
	     strictly better than teaching the algorithm about spans: PRED
	     stays unique per register because the destination spans are
	     already disjoint (§7tricies.66 leaves one live-out live range per
	     register), a cycle needs ONE scratch register rather than n
	     consecutive aligned ones, and a copy whose source and
	     destination spans OVERLAP -- r32r33 <- r33r34 -- decomposes into
	     a chain the algorithm already sequentialises, which is what it
	     exists for.  It costs n moves instead of one in drain code that
	     runs once per loop exit.

	     The component mode is DImode and not the tuple's: the register
	     file is 64 bits, so every component of every tuple is one word,
	     whatever the value's own mode is.  A width-one live range keeps its
	     operand's mode, so an SImode value still moves as one.  */
	  for (int k = 0; k < nr; k++)
	    {
	      psrc.safe_push (nm + k);
	      pdst.safe_push ((int) q->regno + k);
	      pmode.safe_push (nr == 1 ? GET_MODE (*dloc) : DImode);
	    }
	}

      if (psrc.is_empty ())
	continue;

      /* The fresh register a cycle would need: one the colouring did not
	 take, and not itself a source or a destination here.  */
      /* A RANGE OCCUPIES NREGS REGISTERS, NOT ONE.  §7tricies.154.

	 This cleared only the BASE of each name, so a 2- or 4-register value
	 at r24 left r25, r26 and r27 looking free and the scratch could be
	 chosen INSIDE a live tuple.  Found as a wild store on TSVC2's
	 initialise_arrays loop 58, a 256-bit vectorised body: the address
	 register came out holding packed float data.  */
      for (unsigned g = 0; g < lr.length (); g++)
	for (int x = 0; x < LVX_LMS_MAX_KUNROLL; x++)
	  if (lr[g]->reg[x] >= 0 && lr[g]->reg[x] < 64)
	    for (int t = 0; t < MAX (lr[g]->nregs, 1)
			    && lr[g]->reg[x] + t < 64; t++)
	      free_regs
		&= ~((unsigned HOST_WIDE_INT) 1 << (lr[g]->reg[x] + t));
      for (unsigned z = 0; z < psrc.length (); z++)
	{
	  free_regs &= ~((unsigned HOST_WIDE_INT) 1 << psrc[z]);
	  free_regs &= ~((unsigned HOST_WIDE_INT) 1 << pdst[z]);
	}
      for (int r = 0; r < 64; r++)
	if ((free_regs >> r) & 1)
	  {
	    scratch = r;
	    break;
	  }

      *why = "a live-out parallel copy cannot be sequentialised";
      if (!lvx_lms_seq_pcopy (psrc, pdst, pmode, scratch, &osrc, &odst,
			      &omode))
	return false;

      /* At the END of the drain, after every instance of every definition it
	 holds -- and still before the block's terminator, which
	 emit_insn_after_noloc from TAIL gives.  */
      after = tail[k];
      if (!after)
	{
	  after = BB_HEAD (blocks[k]);
	  if (LABEL_P (after))
	    after = NEXT_INSN (after);
	  *why = "no NOTE_INSN_BASIC_BLOCK for a live-out copy";
	  if (!after || !NOTE_INSN_BASIC_BLOCK_P (after))
	    return false;
	}
      for (unsigned z = 0; z < osrc.length (); z++)
	{
	  rtx_insn *mv
	    = emit_insn_after_noloc (gen_rtx_SET
				       (gen_rtx_REG (omode[z], odst[z]),
					gen_rtx_REG (omode[z], osrc[z])),
				     after, NULL);

	  *why = "a live-out copy does not recognise";
	  if (recog_memoized (mv) < 0)
	    return false;
	  after = mv;
	  tail[k] = mv;
	  nemit++;
	}
      lvx_lms_dump (";; lvx cms:   live-out node%u bb%d: %u parallel copies"
		    " -> %u sequential, scratch r%d\n", k, blocks[k]->index,
		    psrc.length (), osrc.length (), scratch);
      if (lvx_lms_preheader_copy)
       for (unsigned z = 0; z < psrc.length (); z++)
	lvx_lms_dump (";; lvx cms:     LOCOPY node%u kind=%d r%d <- r%d\n",
		      k, (int) nodes[k].kind, pdst[z], psrc[z]);
      if (lvx_lms_preheader_copy)
       for (unsigned z = 0; z < osrc.length (); z++)
	lvx_lms_dump (";; lvx cms:     LOSEQ  node%u kind=%d r%d <- r%d\n",
		      k, (int) nodes[k].kind, odst[z], osrc[z]);
    }

  /* THE ENTRY-SIDE RECONCILIATION.  §7tricies.156.

     The exit side has had reconciliation moves since §7quater.5; the entry
     side was a PIN instead, and the paragraph beside that pin says it should
     not have been -- "register allocation's assignment is not valid for the
     unrolled kernel ... which is also why §7quater.5's reconciliation moves
     exist on the exit side, and why the entry side needs the same treatment
     rather than a pin".  This is that treatment.

     A use-headed range takes its name 0 from outside the kernel, so name 0
     had to BE the register the preheader wrote.  One copy
     `reg[0] <- regno' before the pipeline starts removes the constraint:
     the incoming value is placed in whatever register the colouring chose,
     and name 0 becomes an ordinary free choice.  The index arithmetic is
     untouched -- `(iteration + use_first - delayed) & (p2unroll - 1)' still
     puts the incoming value at index 0 and the first body definition at
     index 1 -- so this adds no new naming rule, it only stops one register
     being dictated from outside.

     ONE COPY PER RANGE AND ONE EXECUTION PER LOOP ENTRY, which is what
     separates it from -mlms-tied-copy: that phase put a copy in the BODY for
     every constraint group and spent 1352 of them on about five needs,
     taking emitted from 949 to 877.  The set is a PARALLEL copy -- two
     use-headed ranges can want each other's registers -- so it goes through
     lvx_lms_seq_pcopy and the scratch machinery the drain already uses,
     including the tuple-span fix of §7tricies.154, since a loop-carried
     value can be a register tuple.

     IT MUST RUN ONCE, so it needs a block entered only from the preheader.
     BLOCKS[0] is such a block when NODES[0] is a prologue stage, which is
     the case the phase exists for; where the kernel comes first there is no
     prologue to hide it in and the copy would execute every iteration, so
     the loop is refused instead of being given a copy in the wrong place.  */
  if (lvx_lms_preheader_copy && (lvx_lms_ph_parts & 1))
    {
      auto_vec<int> psrc, pdst, osrc, odst;
      auto_vec<machine_mode> pmode, omode;
      unsigned HOST_WIDE_INT free_regs = avail;
      int scratch = -1;
      rtx_insn *after;

      for (unsigned g = 0; g < lr.length (); g++)
	{
	  lvx_lms_lrange_part *q = lr[g];
	  rtx *uloc;
	  int nr, nm;

	  /* §7tricies.167: the entry copy is for a value that arrives from
	     OUTSIDE the loop, which is the use-headed case and equally the
	     invariant one -- every reference a read, so nothing in the body
	     ever writes it.  Keyed on use_first alone it missed the second.  */
	  if (q->effects.is_empty ()
	      || lvx_lms_lrange_root (lr, (int) g) != (int) g
	      || !q->renamed
	      || !(q->use_first
		   || lvx_lms_lrange_invariant_p (lr, (int) g)))
	    continue;
	  nm = q->reg[0];
	  /* Nothing to do where the colouring took the architected register
	     for name 0 anyway -- which VPREFER's copy-deleting preference
	     (§7tricies.122) arranges wherever it is free.  */
	  if (nm < 0 || nm == (int) q->regno)
	    continue;
	  if (q->effects[0].insn >= seq.length ()
	      || q->effects[0].slot == LVX_LMS_SLOT_INDIRECT)
	    {
	      *why = "a range from outside the loop has no operand to copy"
		     " through";
	      return false;
	    }
	  uloc = lvx_lms_reg_at (&PATTERN (seq[q->effects[0].insn]),
				 q->effects[0].slot);
	  if (!uloc || !*uloc || !REG_P (*uloc))
	    {
	      *why = "a use-headed range's first reference is not a register";
	      return false;
	    }
	  /* Componentwise, and DImode per component for a tuple, for the
	     reasons §7tricies.73 gives on the exit side.  */
	  nr = MAX (q->nregs, 1);
	  for (int t = 0; t < nr; t++)
	    {
	      psrc.safe_push ((int) q->regno + t);
	      pdst.safe_push (nm + t);
	      pmode.safe_push (nr == 1 ? GET_MODE (*uloc) : DImode);
	    }
	}

      if (!psrc.is_empty ())
	{
	  *why = "the first pipeline block is not entered from the preheader"
		 " alone";
	  if (!single_pred_p (blocks[0])
	      || single_pred (blocks[0]) != site->preheader)
	    return false;

	  for (unsigned g = 0; g < lr.length (); g++)
	    for (int x = 0; x < LVX_LMS_MAX_KUNROLL; x++)
	      if (lr[g]->reg[x] >= 0 && lr[g]->reg[x] < 64)
		for (int t = 0; t < MAX (lr[g]->nregs, 1)
				&& lr[g]->reg[x] + t < 64; t++)
		  free_regs
		    &= ~((unsigned HOST_WIDE_INT) 1 << (lr[g]->reg[x] + t));
	  for (unsigned z = 0; z < psrc.length (); z++)
	    {
	      free_regs &= ~((unsigned HOST_WIDE_INT) 1 << psrc[z]);
	      free_regs &= ~((unsigned HOST_WIDE_INT) 1 << pdst[z]);
	    }
	  for (int r = 0; r < 64; r++)
	    if ((free_regs >> r) & 1)
	      {
		scratch = r;
		break;
	      }

	  *why = "an entry parallel copy cannot be sequentialised";
	  if (!lvx_lms_seq_pcopy (psrc, pdst, pmode, scratch, &osrc, &odst,
				  &omode))
	    return false;

	  after = BB_HEAD (blocks[0]);
	  if (LABEL_P (after))
	    after = NEXT_INSN (after);
	  *why = "no NOTE_INSN_BASIC_BLOCK for an entry copy";
	  if (!after || !NOTE_INSN_BASIC_BLOCK_P (after))
	    return false;
	  for (unsigned z = 0; z < osrc.length (); z++)
	    {
	      rtx_insn *mv
		= emit_insn_after_noloc (gen_rtx_SET
					   (gen_rtx_REG (omode[z], odst[z]),
					    gen_rtx_REG (omode[z], osrc[z])),
					 after, NULL);

	      *why = "an entry copy does not recognise";
	      if (recog_memoized (mv) < 0)
		return false;
	      after = mv;
	      nemit++;
	    }
	  /* IS THE ENTRY COPY ON EVERY PATH INTO THE PIPELINE?
	     §7tricies.157.

	     The copy is the only writer of name 0 before the first read, so a
	     pipeline block reachable WITHOUT passing through BLOCKS[0] reads
	     an undefined register -- which is the shape §7tricies.156's
	     layout- and pressure-sensitivity points at.  SINGLE_PRED_P on
	     BLOCKS[0] says the copy runs once; it does not say it runs
	     always, because the emission retargets exits and the skeleton
	     chain is broken up as it goes.

	     Asked by reachability rather than by dominators, which are stale
	     after the mutation: walk the CFG from the entry block, refusing
	     to cross BLOCKS[0], and see whether any block holding a slot of
	     this pipeline is reached.  */
	  {
	    auto_bitmap seen, pipe;
	    auto_vec<basic_block> work;
	    unsigned bad = 0;

	    for (unsigned z = 0; z < nnode; z++)
	      bitmap_set_bit (pipe, blocks[z]->index);
	    bitmap_set_bit (seen, blocks[0]->index);	/* the barrier */
	    bitmap_set_bit (seen, ENTRY_BLOCK_PTR_FOR_FN (cfun)->index);
	    work.safe_push (ENTRY_BLOCK_PTR_FOR_FN (cfun));
	    while (!work.is_empty ())
	      {
		basic_block bb = work.pop ();
		edge e;
		edge_iterator ei;

		FOR_EACH_EDGE (e, ei, bb->succs)
		  if (!bitmap_bit_p (seen, e->dest->index))
		    {
		      bitmap_set_bit (seen, e->dest->index);
		      if (bitmap_bit_p (pipe, e->dest->index))
			{
			  lvx_lms_dump (";; lvx cms:   ENTRYBYPASS bb%d"
					" reached without bb%d\n",
					e->dest->index, blocks[0]->index);
			  bad++;
			}
		      work.safe_push (e->dest);
		    }
	      }
	    *why = "a pipeline block is reachable without the entry copy";
	    if (bad)
	      return false;
	  }
	  lvx_lms_dump (";; lvx cms:   entry bb%d: %u parallel copies -> %u"
			" sequential, scratch r%d\n", blocks[0]->index,
			psrc.length (), osrc.length (), scratch);
	  for (unsigned z = 0; z < psrc.length (); z++)
	    lvx_lms_dump (";; lvx cms:     ENTRYCOPY r%d <- r%d\n",
			  pdst[z], psrc[z]);
	  for (unsigned z = 0; z < osrc.length (); z++)
	    lvx_lms_dump (";; lvx cms:     ENTRYSEQ  r%d <- r%d\n",
			  odst[z], osrc[z]);
	}
    }

  /* THE HARDWARE LOOP ITSELF: the exit-free copy of the kernel, and the two
     blocks around it.  §7tricies.62, continued from ROUNDING DOWN above.

     Laid out between the last prologue node and the kernel the plan built,
     which becomes the REMAINDER copy:

       ... prologue ...          the counter holds M on leaving it
       SETUP                     hw = M - 1; cnt = (hw & (K-1)) + 1;
				 hw >>= log2 K
       KERNEL (exit-free)        the kernel's slots with no loopback instance
				 at all, closed by a doloop_end on HW, which
				 (reorg_loops) consumes into LOOPDO -- so the
				 repetition costs no instruction
       remainder copy            the plan's kernel, its K-1 counter tests and
       ... epilogues ...         its drains unchanged

     Both new blocks come from SPLIT_EDGE on the fall-through into the kernel,
     which is the one entry a hardware loop may have (process_incoming_edges:
     INCOMING_DEST must be the head) and which also gives (lvx_hwloop_optimize)
     the dedicated preheader it looks for the counter's SET in.  */
  if (hwkern)
    {
      rtx cpat = PATTERN (seq[loopback]);
      rtx cnt = SET_DEST (XVECEXP (cpat, 0, 1));
      int cslot = lvx_lms_reg_index (cpat, &SET_DEST (XVECEXP (cpat, 0, 1)));
      bool cdly = false;
      lvx_lms_lrange_part *cg = lvx_lms_lrange_of (lr, loopback, cslot, &cdly);
      machine_mode cm = GET_MODE (cnt);
      int creg = cg ? lvx_lms_lrange_reg (cg, 0, cdly)
		    : (REG_P (cnt) ? (int) REGNO (cnt) : -1);
      unsigned HOST_WIDE_INT free_regs = avail;
      int hwreg = -1, k2 = site->kunroll, logk = 0;
      rtx cnt_rtx, hw_rtx;
      basic_block sbb, kbb;
      edge e;

      *why = "the doloop counter has no single register";
      if (creg < 0 || !REG_P (cnt) || (cg && cg->p2unroll != 1))
	return false;

      /* A REGISTER NO LIVE RANGE HOLDS.  The hardware loop's counter may be
	 mentioned by nothing but its own doloop_end, so it cannot be the
	 pipeline's counter live range -- which the remainder copy reads -- nor
	 any other live range's name.  AVAIL is what the colouring was allowed to
	 use; what it did not take is free across the whole pipeline.  */
      for (unsigned g = 0; g < lr.length (); g++)
	{
	  lvx_lms_lrange_part *q = lr[g];
	  for (int x = 0; x < LVX_LMS_MAX_KUNROLL; x++)
	    if (q->reg[x] >= 0 && q->reg[x] < 64)
	      /* NREGS, not one -- §7tricies.154, as above.  */
	      for (int t = 0; t < MAX (q->nregs, 1) && q->reg[x] + t < 64; t++)
		free_regs &= ~((unsigned HOST_WIDE_INT) 1 << (q->reg[x] + t));
	}
      for (int r = 0; r < 64; r++)
	if ((free_regs >> r) & 1)
	  {
	    hwreg = r;
	    break;
	  }
      *why = "no free register for the hardware loop's own counter";
      if (hwreg < 0)
	return false;

      while ((1 << (logk + 1)) <= k2)
	logk++;
      cnt_rtx = gen_rtx_REG (cm, creg);
      hw_rtx = gen_rtx_REG (cm, hwreg);

      e = find_edge (blocks[kfirst - 1], blocks[kfirst]);
      *why = "the kernel is not entered by a fall-through";
      if (!e || !(e->flags & EDGE_FALLTHRU))
	return false;
      sbb = split_edge (e);
      e = find_edge (sbb, blocks[kfirst]);
      if (!e)
	return false;
      kbb = split_edge (e);

      /* SETUP.  The order matters: HW carries M - 1 until the shift, so the
	 remainder is taken from it before it is divided.  */
      {
	rtx_insn *at = BB_END (sbb);
	rtx seq4[4];
	seq4[0] = (cm == SImode ? gen_addsi3 (hw_rtx, cnt_rtx, constm1_rtx)
		   : gen_adddi3 (hw_rtx, cnt_rtx, constm1_rtx));
	seq4[1] = (cm == SImode
		   ? gen_andsi3 (cnt_rtx, hw_rtx, GEN_INT (k2 - 1))
		   : gen_anddi3 (cnt_rtx, hw_rtx, GEN_INT (k2 - 1)));
	seq4[2] = (cm == SImode ? gen_addsi3 (cnt_rtx, cnt_rtx, const1_rtx)
		   : gen_adddi3 (cnt_rtx, cnt_rtx, const1_rtx));
	seq4[3] = (cm == SImode
		   ? gen_lshrsi3 (hw_rtx, hw_rtx, GEN_INT (logk))
		   : gen_lshrdi3 (hw_rtx, hw_rtx, GEN_INT (logk)));
	if (!at || !NOTE_INSN_BASIC_BLOCK_P (at))
	  {
	    at = BB_HEAD (sbb);
	    if (LABEL_P (at))
	      at = NEXT_INSN (at);
	  }
	*why = "no NOTE_INSN_BASIC_BLOCK in the hardware loop's preheader";
	if (!at)
	  return false;
	for (int x = 0; x < 4; x++)
	  at = emit_insn_after_noloc (seq4[x], at, NULL), nemit++;
      }

      /* THE EXIT-FREE KERNEL, as a chain of blocks.  The same slots, in the
	 same linearised order and renamed for the same iterations as the
	 leftover copy, and carrying the same DATA-dependent exits -- each
	 ends its block and branches to the same drain the leftover copy's
	 instance does.  Only the COUNTED instances are absent, which is the
	 whole of what ITER_REG_USED forbids.  */
      {
	basic_block cur = kbb;
	rtx_insn *at = BB_HEAD (cur);
	rtx_insn *de;
	int nk = 0, nx = 0;

	if (LABEL_P (at))
	  at = NEXT_INSN (at);
	*why = "no NOTE_INSN_BASIC_BLOCK in the exit-free kernel";
	if (!at || !NOTE_INSN_BASIC_BLOCK_P (at))
	  return false;
	lvx_lms_dump (";; lvx cms:   K0 trace (exit-free copy):");
	for (unsigned oq = 0; oq < order.length (); oq++)
	  {
	    unsigned q = (unsigned) order[oq];
	    int nd = slots[q].node;
	    unsigned i = (unsigned) slots[q].insn;
	    rtx_insn *c;
	    int n = 0;

	    if (nd < 0 || nd >= (int) nnode || i >= seq.length ())
	      continue;
	    if (nodes[nd].kind != LVX_LMS_NODE_KERNEL)
	      continue;

	    if (JUMP_P (seq[i]))
	      {
		int ep = -1;
		rtx_insn *j;
		rtx_jump_insn *jj;
		basic_block target;

		if ((int) i == loopback)
		  continue;		/* no counted instance at all */
		for (unsigned z = 0; z < nnode; z++)
		  if (nodes[z].kind == LVX_LMS_NODE_EPILOGUE
		      && !nodes[z].main_epilogue
		      && nodes[z].exit_date == slots[q].date
		      && nodes[z].exit_iter == slots[q].iter
		      && nodes[z].exit_insn == (int) i)
		    ep = (int) z;
		*why = "an exit of the exit-free kernel has no drain";
		if (ep < 0)
		  return false;
		target = blocks[ep];

		j = emit_jump_insn_after_noloc (copy_insn (PATTERN (seq[i])),
						at);
		JUMP_LABEL (j) = JUMP_LABEL (seq[i]);
		if (JUMP_LABEL (j) && LABEL_P (JUMP_LABEL (j)))
		  LABEL_NUSES (JUMP_LABEL (j))++;
		jj = as_a <rtx_jump_insn *> (j);
		if (lvx_lms_jump_taken_stays_p (site, seq[i]))
		  {
		    *why = "cannot invert an exit of the exit-free kernel";
		    if (!invert_jump (jj, block_label (target), 0))
		      return false;
		  }
		else
		  {
		    *why = "cannot retarget an exit of the exit-free kernel";
		    if (!redirect_jump (jj, block_label (target), 0))
		      return false;
		  }
		*why = "the renamed branch does not recognise";
		if (!lvx_lms_rename_lr_insn_p (seq[i], j, i, slots[q].iter,
					       lr, &n, true))
		  return false;
		if (!find_edge (cur, target))
		  {
		    edge ne = make_edge (cur, target, 0);
		    edge fe;

		    ne->probability = profile_probability::even ();
		    fe = find_fallthru_edge (cur->succs);
		    if (fe)
		      fe->probability = profile_probability::even ();
		  }
		lvx_lms_dump (" bb%d:%u/%d@%dX->bb%d", cur->index, i,
			      slots[q].iter, slots[q].date, target->index);
		nemit++;
		nx++;

		e = find_edge (cur, blocks[kfirst]);
		*why = "the exit-free kernel's chain has no fall-through";
		if (!e || !(e->flags & EDGE_FALLTHRU))
		  return false;
		cur = split_edge (e);
		at = BB_HEAD (cur);
		if (LABEL_P (at))
		  at = NEXT_INSN (at);
		*why = "no NOTE_INSN_BASIC_BLOCK in an exit-free chain block";
		if (!at || !NOTE_INSN_BASIC_BLOCK_P (at))
		  return false;
		continue;
	      }

	    c = emit_insn_after_noloc (copy_insn (PATTERN (seq[i])), at, NULL);
	    *why = "the renamed copy does not recognise";
	    if (!lvx_lms_rename_lr_insn_p (seq[i], c, i, slots[q].iter, lr,
					   &n, true))
	      return false;
	    lvx_lms_dump (" bb%d:%u/%d@%d", cur->index, i, slots[q].iter,
			  slots[q].date);
	    at = c;
	    nemit++;
	    nk++;
	  }
	lvx_lms_dump ("\n");
	*why = "the exit-free kernel came out empty";
	if (nk == 0)
	  return false;

	de = emit_jump_insn_after_noloc
	       ((cm == SImode
		 ? gen_doloop_end_si (hw_rtx, block_label (kbb))
		 : gen_doloop_end_di (hw_rtx, block_label (kbb))),
		at);
	JUMP_LABEL (de) = block_label (kbb);
	LABEL_NUSES (JUMP_LABEL (de))++;
	nemit++;
	if (!find_edge (cur, kbb))
	  {
	    edge be = make_edge (cur, kbb, 0);
	    edge fe;

	    be->probability = profile_probability::even ();
	    fe = find_fallthru_edge (cur->succs);
	    if (fe)
	      fe->probability = profile_probability::even ();
	  }
	lvx_lms_dump (";; lvx cms:   hwloop kernel bb%d..bb%d: %d insns,"
		      " %d data exits, counter r%d, trips (M-1)>>%d,"
		      " leftover in r%d bb%d\n", kbb->index, cur->index, nk,
		      nx, hwreg, logk, creg, blocks[kfirst]->index);
      }
    }

  /* THE INSTANCE SET EACH BLOCK RECEIVED, printed so that what EXECUTES can
     be traced against the plan rather than reasoned about.  §7tricies.69.
     One line per node: its kind, its block, and every (insn/iteration@date)
     it holds in emission order, with X marking a jump.  */
  if (lvx_lms_probe)
    for (unsigned k = 0; k < nnode; k++)
      {
	const char *kind = (nodes[k].kind == LVX_LMS_NODE_PROLOGUE ? "pro"
			    : nodes[k].kind == LVX_LMS_NODE_KERNEL ? "ker"
			    : nodes[k].main_epilogue ? "mainep" : "exitep");

	lvx_lms_dump (";; lvx cms:   trace node%u %s bb%d:", k, kind,
		      blocks[k]->index);
	for (unsigned oq = 0; oq < order.length (); oq++)
	  {
	    unsigned q = (unsigned) order[oq];

	    if (slots[q].node != (int) k)
	      continue;
	    lvx_lms_dump (" %d/%d@%d%s", slots[q].insn, slots[q].iter,
			  slots[q].date,
			  (slots[q].insn >= 0
			   && (unsigned) slots[q].insn < seq.length ()
			   && JUMP_P (seq[slots[q].insn])) ? "X" : "");
	  }
	lvx_lms_dump ("\n");
      }

  /* THE CFG AS BUILT, printed rather than inferred from the assembly.  Four
     diagnoses of the remaining fault were made by reading emitted code and
     all four were wrong; the edges are what the fault is about, so they are
     what gets dumped.  */
  if (lvx_lms_probe)
    for (unsigned k = 0; k < nnode; k++)
      {
	edge e;
	edge_iterator ei;
	const char *kind = (nodes[k].kind == LVX_LMS_NODE_PROLOGUE ? "pro"
			    : nodes[k].kind == LVX_LMS_NODE_KERNEL ? "ker"
			    : nodes[k].main_epilogue ? "mainep" : "exitep");
	lvx_lms_dump (";; lvx cms:   cfg node%u %s bb%d ->", k, kind,
		      blocks[k]->index);
	FOR_EACH_EDGE (e, ei, blocks[k]->succs)
	  lvx_lms_dump (" bb%d%s", e->dest->index,
			(e->flags & EDGE_FALLTHRU) ? "f" : "");
	lvx_lms_dump ("\n");
      }

  *nemit_out = nemit;
  *why = NULL;
  return true;
}

/* Repair an uncolourable kernel by SPILL/RESTORE at the current II.
   §7duovicies.

   The difference from splitting, and it is the whole point: a split leaves two
   CONTIGUOUS ranges covering the same span, so occupancy -- and therefore
   MaxLive -- is unchanged (§7unvicies).  A spill leaves two ranges with a GAP
   between the store and the reload, and across that gap the value is in memory
   and holds no register.  So a spill is the only one of these repairs that
   lowers pressure.

   For a value defined at D with consumers at U1 < ... < Uk, the register must
   be live at D and at every Ui, so the spillable interval is the largest gap
   between consecutive required points.  Store just after the earlier point,
   reload the cached-load latency before the later one, and the value's
   occupancy loses the interior.

   Costs, charged to the II test: two instructions per register, each a TINY and
   an LSU, the store also a MEMW; and the reload's latency on the path, priced
   CACHED at 3 because the slot is in the frame (§7novodecies).  One spill slot
   per expansion copy, per the author -- which the frame pays for and the
   register file does not, so it does not enter this test.  */

#define LVX_LMS_RELOAD_LATENCY 3

static int
lvx_lms_spill_until_colourable (const auto_vec<rtx_insn *> &seq,
				const auto_vec<lvx_lms_arc> &arcs,
				const auto_vec<int> &sigma,
				const struct lvx_sched_resources *demand,
				int ii, int lo, int *spill_out, int *nodes_out,
				int *spill0_out)
{
  auto_vec<int> cdef, clast, cdeg, cnr, cec, cown;
  lvx_lms_collect_values (seq, arcs, sigma, ii, cdef, clast, cdeg, cnr, cec,
			  &cown);

  auto_vec<int> tiny, lsu;
  for (int c = 0; c < ii; c++)
    tiny.safe_push (0), lsu.safe_push (0);
  for (unsigned i = 0; i < seq.length (); i++)
    {
      struct lvx_sched_resources one;
      memset (&one, 0, sizeof (one));
      lvx_lms_resources_add (&one, seq[i]);
      tiny[(sigma[i] - lo) % ii] += (int) one.tiny_count;
      lsu[(sigma[i] - lo) % ii] += (int) one.lsu_count;
    }
  int tinycap = (int) lvx_issue_availability.tiny;
  int lsucap = (int) lvx_issue_availability.lsu;

  auto_vec<bool> declined;
  declined.safe_grow_cleared (cdeg.length ());
  for (unsigned v = 0; v < cdeg.length (); v++)
    declined[v] = false;

  int added_tiny = 0, added_lsu = 0, spills = 0;
  int why_nogap = 0, why_ii = 0, why_slot = 0, why_nocand = 0;
  for (int attempt = 0; attempt <= 8; attempt++)
    {
      int nn, ne, msq, spl, per, kun;
      lvx_lms_colour (cdef, clast, cdeg, cnr, cec, ii, lo,
		      &nn, &ne, &msq, &spl, &per, &kun);
      if (attempt == 0)
	*spill0_out = spl;
      if (spl == 0 || attempt == 8)
	{
	  *spill_out = spl;
	  *nodes_out = why_nogap * 1000000 + why_ii * 1000
		       + why_slot + why_nocand * 1000000000;
	  return spills;
	}

      /* Ruttenberg's victim: span over references, the largest first -- a long
	 range with one reader is worth spilling, one with many is not.  */
      int best = -1;
      double bestratio = -1.0;
      for (unsigned v = 0; v < cdeg.length (); v++)
	{
	  if (declined[v] || cdeg[v] <= 1 || cown[v] < 0)
	    continue;
	  int refs = 0;
	  for (unsigned a = 0; a < arcs.length (); a++)
	    if (arcs[a].src == (unsigned) cown[v]
		&& arcs[a].kind == LVX_ARC_REG_RAW)
	      refs++;
	  double ratio = (double) (clast[v] - cdef[v]) / MAX (refs, 1);
	  if (ratio > bestratio)
	    bestratio = ratio, best = (int) v;
	}
      if (best < 0)
	{
	  *spill_out = spl;
	  *nodes_out = why_nogap * 1000000 + why_ii * 1000
		       + why_slot + why_nocand * 1000000000;
	  return spills;
	}

      int d = cdef[best], last = clast[best], nr = cnr[best];
      int i = cown[best];

      /* The required points: the definition and every consumer inside this
	 value's own range.  */
      auto_vec<int> pts;
      pts.safe_push (d);
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].src == (unsigned) i && arcs[a].kind == LVX_ARC_REG_RAW)
	  {
	    int when = sigma[arcs[a].dst] + (int) arcs[a].omega * ii;
	    if (when > d && when <= last)
	      pts.safe_push (when);
	  }
      for (unsigned x = 0; x + 1 < pts.length (); x++)
	for (unsigned y = x + 1; y < pts.length (); y++)
	  if (pts[y] < pts[x])
	    std::swap (pts[x], pts[y]);

      /* The widest gap between consecutive required points.  */
      int gp = -1, gq = -1, gap = 0;
      for (unsigned x = 0; x + 1 < pts.length (); x++)
	if (pts[x + 1] - pts[x] > gap)
	  gap = pts[x + 1] - pts[x], gp = pts[x], gq = pts[x + 1];
      if (gp < 0)
	{
	  why_nocand++;
	  declined[best] = true;
	  continue;
	}

      int latd = MAX (1, lvx_lms_latency (seq[i]));
      int ts = (gp == d) ? d + latd : gp + 1;
      int tr = gq - LVX_LMS_RELOAD_LATENCY;
      if (tr <= ts)
	{
	  why_nogap++;
	  declined[best] = true;	/* no room for a memory round trip */
	  continue;
	}

      struct lvx_sched_resources dem2 = *demand;
      dem2.tiny_count += added_tiny;
      dem2.lsu_count += added_lsu;
      dem2.ninsns_count += added_tiny;
      if (!lvx_lms_repair_fits_ii_p (seq, arcs, &dem2, i, ii,
				     LVX_LMS_RELOAD_LATENCY, 2 * nr,
				     2 * nr, nr))
	{
	  why_ii++;
	  declined[best] = true;
	  continue;
	}

      /* Slots for the store and the reload, in their own modulo cycles.  */
      int cs = (ts - lo) % ii, cr = (tr - lo) % ii;
      if (tiny[cs] + nr > tinycap || lsu[cs] + nr > lsucap
	  || tiny[cr] + nr > tinycap || lsu[cr] + nr > lsucap)
	{
	  why_slot++;
	  declined[best] = true;
	  continue;
	}
      tiny[cs] += nr, lsu[cs] += nr;
      tiny[cr] += nr, lsu[cr] += nr;
      added_tiny += 2 * nr, added_lsu += 2 * nr;

      /* The value now occupies [D, TS] and [TR, LAST], with no register in
	 between -- which is what lowers MaxLive.  */
      clast[best] = ts;
      cdeg[best] = (ts - d + ii - 1) / ii;
      cdef.safe_push (tr);
      clast.safe_push (last);
      cdeg.safe_push ((last - tr + ii - 1) / ii);
      cnr.safe_push (nr);
      cec.safe_push (0);
      cown.safe_push (-1);		/* the reload, not a graph instruction */
      declined.safe_push (true);
      spills++;
    }
  return spills;
}

/* Repair an uncolourable kernel by SPLITTING RANGES at the current II, and
   report whether it worked.  §7unvicies.

   Each split replaces one value by two -- definition..copy and copy..last use
   -- and costs one COPYD per register at the copy's cycle.  A candidate is
   taken only if it keeps the II (lvx_lms_repair_fits_ii_p: the extra TINY must
   fit the resource budget and the extra cycle of latency must not push the
   parametric period past the II), and only if a cycle exists inside the
   producer-latency window with a free TINY in the cumulative table.

   Victim order is descending degree, which is what sets the expansion and so
   the node count.  Ruttenberg's span-over-references ratio would be the better
   choice for a SPILL, where the cost is the reference count; for a split the
   degree is what the repair reduces, so it is the natural key.  Capped at 8
   splits, after Ruttenberg's cap on escalation.  */

static int
lvx_lms_split_until_colourable (const auto_vec<rtx_insn *> &seq,
				const auto_vec<lvx_lms_arc> &arcs,
				const auto_vec<int> &sigma,
				const struct lvx_sched_resources *demand,
				int ii, int lo, int *spill_out, int *nodes_out,
				int *spill0_out)
{
  auto_vec<int> cdef, clast, cdeg, cnr, cec, cown;
  lvx_lms_collect_values (seq, arcs, sigma, ii, cdef, clast, cdeg, cnr, cec,
			  &cown);

  /* Modulo TINY occupancy of the schedule, which the copies draw down.  */
  auto_vec<int> tiny;
  for (int c = 0; c < ii; c++)
    tiny.safe_push (0);
  for (unsigned i = 0; i < seq.length (); i++)
    {
      struct lvx_sched_resources one;
      memset (&one, 0, sizeof (one));
      lvx_lms_resources_add (&one, seq[i]);
      tiny[(sigma[i] - lo) % ii] += (int) one.tiny_count;
    }
  int tinycap = (int) lvx_issue_availability.tiny;
  int added_tiny = 0;

  /* Declining a candidate must NOT touch CDEG: that array is the colouring's
     input, so marking a value "skip" by zeroing its degree would shrink the
     graph and manufacture a colouring.  A separate flag keeps the two
     apart.  */
  auto_vec<bool> declined;
  declined.safe_grow_cleared (cdeg.length ());
  for (unsigned v = 0; v < cdeg.length (); v++)
    declined[v] = false;

  int splits = 0;
  for (int attempt = 0; attempt <= 8; attempt++)
    {
      int nn, ne, msq, spl, per, kun;
      lvx_lms_colour (cdef, clast, cdeg, cnr, cec, ii, lo,
		      &nn, &ne, &msq, &spl, &per, &kun);
      /* The colouring BEFORE any split, over the complete value list, so the
	 caller can see whether that list agrees with the one the report built
	 -- it does not, and §7unvicies says why.  */
      if (attempt == 0)
	*spill0_out = spl;
      if (spl == 0 || attempt == 8)
	{
	  *spill_out = spl;
	  *nodes_out = nn;
	  return splits;
	}

      /* Pick the splittable value with the largest degree.  */
      int best = -1;
      for (unsigned v = 0; v < cdeg.length (); v++)
	if (!declined[v] && cdeg[v] > 1 && cown[v] >= 0
	    && (best < 0 || cdeg[v] > cdeg[best]))
	  best = (int) v;
      if (best < 0)
	{
	  *spill_out = spl;
	  *nodes_out = nn;
	  return splits;
	}

      int d = cdef[best], last = clast[best], nr = cnr[best];
      int i = cown[best];

      /* Does a copy here keep the II, counting the copies already added?  */
      struct lvx_sched_resources dem2 = *demand;
      dem2.tiny_count += added_tiny;
      dem2.ninsns_count += added_tiny;
      if (!lvx_lms_repair_fits_ii_p (seq, arcs, &dem2, i, ii, 1, nr))
	{
	  declined[best] = true;	/* look at the next one */
	  continue;
	}

      /* A cycle in the producer-latency window with NR free TINY.  */
      int lat = MAX (1, lvx_lms_latency (seq[i]));
      int t = -1;
      for (int c = d + lat; c <= last - 1 && t < 0; c++)
	if (tiny[(c - lo) % ii] + nr <= tinycap)
	  t = c;
      if (t < 0)
	{
	  declined[best] = true;
	  continue;
	}

      /* Split: the value becomes def..copy, and a new value copy..last use.  */
      tiny[(t - lo) % ii] += nr;
      added_tiny += nr;
      clast[best] = t;
      cdeg[best] = (t - d + ii - 1) / ii;
      cdef.safe_push (t);
      clast.safe_push (last);
      cdeg.safe_push ((last - t + ii - 1) / ii);
      cnr.safe_push (nr);
      cec.safe_push (0);
      cown.safe_push (i);
      declined.safe_push (false);
      splits++;
    }
  return splits;
}

int
lvx_lms_latency (rtx_insn *insn)
{
  if (lvx_doloop_end_p (insn))
    return 1;
  return MAX (1, insn_default_latency (insn)
		 + lvx_lms_load_latency_delta (insn));
}

bool
lvx_lms_slot_fits_p (const struct lvx_sched_resources *slot, rtx_insn *insn)
{
  struct lvx_sched_resources trial = *slot;
  lvx_lms_resources_add (&trial, insn);

  if (trial.ninsns_count > (unsigned) lvx_sched_issue_rate ())
    return false;
#define LVX_LMS_FITS(R)							  if (trial.R##_count > (unsigned) lvx_issue_availability.R)		    return false;
  LVX_ISSUE_RESOURCES (LVX_LMS_FITS)
#undef LVX_LMS_FITS
  return true;
}

/* Longest path from each node to the end over the INTRA-iteration arcs,
   counting the node's own processing time: TOSINK.  The makespan is the largest
   ASAP + latency, ALAP is the makespan less TOSINK, and SLACK is the room
   between ALAP and ASAP -- how many cycles a node may be delayed without
   lengthening one iteration.

   ASAP, passed in, is HEIGHT: a FORWARD relaxation, so it is the longest path
   FROM the source, not to the sink.  Worth saying because the list scheduler's
   own comment claimed otherwise for a while, and the two are different
   orderings with different characters: ASAP ascending issues breadth-first from
   the inputs, TOSINK descending issues the critical path first.  */

void
lvx_lms_path_metrics (const auto_vec<rtx_insn *> &seq,
		      const auto_vec<lvx_lms_arc> &arcs,
		      const auto_vec<int> &asap,
		      auto_vec<int> &tosink, auto_vec<int> &slack)
{
  unsigned n = seq.length ();

  tosink.truncate (0);
  slack.truncate (0);
  tosink.safe_grow_cleared (n);
  slack.safe_grow_cleared (n);

  for (unsigned i = 0; i < n; i++)
    tosink[i] = lvx_lms_latency (seq[i]);

  /* Reverse relaxation.  An omega = 0 arc runs forward in chain order, so one
     reverse sweep settles it; a second costs nothing at these sizes and makes
     the result independent of that assumption.  */
  for (unsigned round = 0; round < 2; round++)
    for (int i = (int) n - 1; i >= 0; i--)
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].omega == 0 && (int) arcs[a].src == (unsigned) i)
	  {
	    int cand = (int) arcs[a].theta + tosink[arcs[a].dst];
	    if (tosink[i] < cand)
	      tosink[i] = cand;
	  }

  int makespan = 1;
  for (unsigned i = 0; i < n; i++)
    makespan = MAX (makespan, asap[i] + lvx_lms_latency (seq[i]));
  for (unsigned i = 0; i < n; i++)
    slack[i] = (makespan - tosink[i]) - asap[i];
}
/* ACCEPTANCE, WHICH EVERY ENGINE IS JUDGED BY.  §7tricies.222.

   A schedule is accepted when it VERIFIES -- every arc satisfied and the
   modulo reservation table within its availabilities -- and when the kernel it
   implies can be COLOURED.  Both tests are properties of the schedule and of
   the machine, not of the search that found it, so they belong to LMS and not
   to any one engine: CMS calls this inside its Squeaky Wheel cycle because a
   rejection is its squeak, and an engine with no search of its own calls it
   once.  Judging two engines by two tests would make the comparison
   meaningless before it started.

   Returns NULL when the schedule is accepted, and otherwise the reason, which
   the caller reports as its last failure.  BLAME and CULPRIT, when given,
   receive the instructions a rejection implicates, which is what SWO reads.

   Extracted from lvx_cms_swo_at_ii unchanged.  */

const char *
lvx_lms_accept_p (const auto_vec<rtx_insn *> &seq,
		  const auto_vec<lvx_lms_arc> &arcs,
		  int ii, const auto_vec<int> &sigma, bitmap livein,
		  auto_vec<int> *blame, auto_vec<int> *culprit)
{
  unsigned n = seq.length ();
  const char *bad = lvx_lms_verify (seq, arcs, ii, sigma);

  if (bad)
    return bad;

  /* ALLOCATION IS PART OF ACCEPTANCE, AT THIS II.
     A schedule that verifies may still produce a kernel the
     allocator cannot colour -- §7ter.10 measured 27 of 158 lvx-2
     loops, twelve of them at a pressure that fits.  The author's
     instruction (2026-10-27) is that colourability must NOT be
     wired into the II search: raising II is not the answer, and
     among the schedules SWO already explores at a given II some
     will allocate and some will not.  So an uncolourable kernel is
     just another squeak -- the values whose nodes could not be
     removed are blamed, the parameters move, and the budget is
     spent looking for a schedule at THIS II that both verifies and
     allocates.  Only when the budget runs out does the II rise, by
     the ordinary path.  */
  const char *why = NULL;
  if (lvx_lms_regalloc_feasible)
    {
      /* ONE MODEL FOR THE SEARCH AND FOR CODEGEN.  §7tricies.40.

	 This asked the VALUE colouring, with AVAIL defaulting to
	 all 64 registers, while codegen asks the LIVE RANGE colouring
	 against the registers actually free.  Two models and two
	 register files: the search accepted an II whose kernel
	 codegen then could not colour, and since select failure
	 was silent (§7tricies.39) the loop was emitted with two of
	 its names sharing a register.  heat-3d's loop 7 is the
	 case -- 44 nodes, maxreg 63, and the search believed the
	 whole file was free.

	 So the test is the same build, the same colouring and the
	 same AVAIL the emission will use.  It is more expensive
	 per candidate II, and it raises II where the registers are
	 genuinely not there, which is the honest answer: the lower
	 II was not achievable, it was only unmeasured.  */
      auto_vec<lvx_lms_lrange_part *> flr;
      auto_vec<int> fstuck;
      HARD_REG_SET fresv;
      unsigned HOST_WIDE_INT favail = 0;
      int fnn = 0, fst = 0, fmx = -1, fkun = 1;
      int fpin = 0, fpinf = 0, fwide = 0;

      lvx_lms_build_lranges (seq, flr, NULL, NULL, NULL);
      lvx_lms_set_lrange_dates (seq, flr, sigma, ii, NULL);
      lvx_lms_lrange_reserved (seq, flr, livein, &fresv);
      for (int r = 0; r < 64 && r < FIRST_PSEUDO_REGISTER; r++)
	if (!TEST_HARD_REG_BIT (fresv, r))
	  favail |= (unsigned HOST_WIDE_INT) 1 << r;

      if (!lvx_lms_colour_lranges (seq, flr, ii,
				   lvx_lms_sigma_min (sigma),
				   favail, &fnn, &fst, &fmx,
				   &fkun, &fpin, &fpinf, &fwide,
				   false, &fstuck))
	{

	  why = "kernel not colourable";
	  /* Blame the DEFINING instruction of every live range left
	     uncoloured.  Its range is what interferes, and the
	     release lever below moves a task later -- which
	     shortens a live range from its definition end,
	     lowering interference without touching II.  */
	  for (unsigned q = 0; q < fstuck.length (); q++)
	    {
	      int k = fstuck[q];

	      if (k < 0 || k >= (int) flr.length ()
		  || flr[k]->effects.is_empty ())
		continue;
	      {
		unsigned i = flr[k]->effects[0].insn;

		if (i < n)
		  (*blame)[i]++, (*culprit)[i]++;
	      }
	    }
	}
      for (unsigned k = 0; k < flr.length (); k++)
	delete flr[k];
    }

  return why;
}

/* Verify SIGMA is a legal modulo schedule at II.  Returns null, or the first
   violation found.  Two things must hold, and they are the definition:

     every arc:   sigma (dst) - sigma (src)  >=  theta - omega * II
     resources:   the nodes sharing a slot (sigma mod II) fit one bundle

   The second is why a modulo reservation table is the right structure even
   though the scheduler itself works in absolute cycles -- in steady state every
   II cycles the same slot comes round.  */

const char *
lvx_lms_verify (const auto_vec<rtx_insn *> &seq,
		const auto_vec<lvx_lms_arc> &arcs,
		int ii, const auto_vec<int> &sigma,
		auto_vec<int> *guilty)
{
  if (ii <= 0 || sigma.length () != seq.length ())
    return "malformed schedule";

  for (unsigned a = 0; a < arcs.length (); a++)
    {
      HOST_WIDE_INT need = arcs[a].theta - arcs[a].omega * (HOST_WIDE_INT) ii;
      if ((HOST_WIDE_INT) (sigma[arcs[a].dst] - sigma[arcs[a].src]) < need)
	{
	  /* THE SQUEAK, NAMED.  §7tricies.129.  Both ends are implicated:
	     the destination is too early, or the source too late.  */
	  if (guilty && arcs[a].dst < guilty->length ())
	    (*guilty)[arcs[a].dst]++;
	  if (guilty && arcs[a].src < guilty->length ())
	    (*guilty)[arcs[a].src]++;
	  return "dependence arc violated";
	}
    }

  auto_vec<struct lvx_sched_resources> slot;
  for (int i = 0; i < ii; i++)
    {
      struct lvx_sched_resources empty;
      memset (&empty, 0, sizeof (empty));
      slot.safe_push (empty);
    }
  for (unsigned i = 0; i < seq.length (); i++)
    {
      int at = ((sigma[i] % ii) + ii) % ii;
      if (!lvx_lms_slot_fits_p (&slot[at], seq[i]))
	{
	  /* THE SQUEAK, NAMED.  §7tricies.129.  An overflow is a COMPETITION
	     for one residue, so everything already standing in it is as guilty
	     as the instruction that did not fit -- blaming only the latter
	     would make the squeak an artefact of iteration order.  */
	  if (guilty && i < guilty->length ())
	    (*guilty)[i]++;
	  for (unsigned j = 0; guilty && j < i; j++)
	    if (((sigma[j] % ii) + ii) % ii == at && j < guilty->length ())
	      (*guilty)[j]++;
	  return "modulo reservation overflows";
	}
      lvx_lms_resources_add (&slot[at], seq[i]);
    }
  return NULL;
}


/* Build the CMS dependence graph of innermost LOOP over BODY: the body's
   instructions in chain order into SEQ, the (theta, omega) arcs over them into
   ARCS -- numbered 0-based over SEQ -- and the intra-iteration dependence depth
   of each into HEIGHT.  False if the body is not a chain the walk can cover.

   Extracted from the unfold's profitability test so CMS can build the graph
   itself: the two have DIFFERENT candidate sets.  The unfold declines a
   hardware loop and sends it here, so every doloop -- which is most of what
   PolyBench is -- reaches CMS and never reached the unfold's probe.  */

static bool
lvx_lms_build_graph (class loop *loop, basic_block *body,
		     auto_vec<rtx_insn *> &seq,
		     auto_vec<lvx_lms_arc> &arcs,
		     auto_vec<int> &height,
		     int *dropped_safelen, int *lrdrop_out /* [4] */ = NULL,
		    const auto_vec<lvx_lms_prop> *prop = NULL,
		    const char **why = NULL)
{
  /* EVERY EXIT NAMES ITSELF.  §7tricies.143.  The caller used to print one
     blanket "body is not a chain the analysis can walk" for all of them,
     which made two different declines indistinguishable and -- because two
     exits ALSO printed their own reason -- double-counted them: `partial
     definition of rN' and `body is not a chain' both read 55 over four
     suites because they were the SAME 55 loops, once from here and once from
     the caller.  A bucket that cannot be attributed cannot be sized, and that
     one was sized twice.  */
  if (why)
    *why = "the body graph cannot be built";
  /* The weakest safelen of any memory arc dropped below, or 0 if none was.
     The caller must check it against the converged NSTAGES: dropping is sound
     only while safelen >= nstages.  */
  *dropped_safelen = 0;
  auto_bitmap inloop;
  rtx_insn *insn;
  /* For the trip-count rule below.  -1 when unknown, which post-reload is the
     common case; see the note at its use.  */
  HOST_WIDE_INT maxtrip = get_max_loop_iterations_int (loop);
  const char *tripsrc = maxtrip > 0 ? "niter" : "none";
  unsigned ntripdrop = 0;

  if (maxtrip <= 0)
    {
      maxtrip = lvx_lms_doloop_trip_count (loop, body);
      tripsrc = maxtrip > 0 ? "doloop" : "none";
    }

  for (unsigned i = 0; i < loop->num_nodes; i++)
    bitmap_set_bit (inloop, body[i]->index);

  /* The body in chain order from the header.  */
  basic_block walk = loop->header;
  for (unsigned i = 0; i < loop->num_nodes; i++)
    {
      edge e;
      edge_iterator ei;
      basic_block nxt = NULL;

      FOR_BB_INSNS (walk, insn)
	if (NONDEBUG_INSN_P (insn)
	    && GET_CODE (PATTERN (insn)) != USE
	    && GET_CODE (PATTERN (insn)) != CLOBBER
	    && recog_memoized (insn) >= 0)
	  seq.safe_push (insn);

      FOR_EACH_EDGE (e, ei, walk->succs)
	if (bitmap_bit_p (inloop, e->dest->index))
	  nxt = e->dest;
      if (nxt == NULL)
	{
	  /* Not a chain: measuring a prefix would be worse than refusing.
	     The gate above should have caught this already.  The message used
	     to carry the `lvx unfold:' prefix, copied from that pass, so it
	     was attributed to a phase that had not run.  */
	  if (why)
	    *why = "a body block has no successor inside the loop";
	  return false;
	}
      walk = nxt;
    }

  unsigned n = seq.length ();
  if (n == 0)
    {
      if (why)
	*why = "the body holds no schedulable instruction";
      return false;
    }

  /* Addresses as affine functions of the iteration, for the stride test that
     the memory arcs below use to disprove what the alias oracle cannot.  */
  auto_vec<lvx_mem_iv> mivs;
  lvx_lms_collect_mem_ivs (loop, seq, mivs);

  /* Simple inductions, for inductive relaxation below.  */
  auto_vec<HOST_WIDE_INT> ivdelta;
  auto_bitmap inductions;
  lvx_lms_find_simple_inductions (seq, ivdelta, inductions);

  /* The intra-iteration arcs, kept to compute the dependence height.  */
  height.safe_grow_cleared (n);

  /* RAW, from the REACHING definition only.

     This used to run an arc from every def of a register to every use of it,
     which for pseudo-registers would merely be redundant but post-RA is wrong.
     A hard register is reused as scratch many times over in one body, and
     "every def to every use" then MANUFACTURES loop-carried dependences that
     do not exist: in gemm's init_array the chain

         39  r0 = zxn(r2)*zxn(r7)      41  r0 = zxt(r0,...)   ...
         56  r0 = r0*r6                57  [r1-0x8] = r0

     got an arc 56 -> 41 at distance 1, as if insn 41 read the r0 that 56 wrote
     last iteration.  It does not: insn 39 defines r0 first and kills that
     value.  The phantom circuit set RecMII to 12 where the real recurrence is
     the induction variable's own increment, and so pinned II at 12 against a
     ResMII of 3.

     The chain order makes the reaching definition exact rather than
     approximate: the body is a chain, each block having one in-loop successor,
     so control flow through it is linear and the def that reaches a use is
     simply the LAST one before it.  If none precedes the use in this iteration
     the value comes from the previous one, from the last def in the body --
     which may be the using instruction itself, the read-modify-write case that
     is a genuine self-loop at distance 1.

     Dropping the non-reaching pairs loses no ordering: the killing definition
     carries its own arcs, and the anti-dependences that would otherwise
     protect the old value are the ones renaming removes, which is the
     assumption this graph already makes below.  */
  unsigned nrelaxed = 0;
  for (unsigned q = 0; q < n; q++)
    {
      df_ref use;

      FOR_EACH_INSN_USE (use, seq[q])
	{
	  unsigned regno = DF_REF_REGNO (use);
	  int p = -1;
	  HOST_WIDE_INT w = 0;
	  /* PROPAGATED: take this operand as reading the copy's source, FROM
	     THE RECORDED DEFINITION.  §7tricies.110.  Searching backward for
	     the nearest write of that register is what the first attempt did
	     and it finds the wrong one post-RA: a hard register carries
	     several values, and for a loop-carried read the reader precedes
	     the copy, so the nearest earlier definition is generally not the
	     one that fed it.  */
	  const lvx_lms_prop *pe
	    = lvx_lms_prop_lookup (prop, q,
				   lvx_lms_reg_index (PATTERN (seq[q]),
						      DF_REF_LOC (use)));
	  if (pe)
	    {
	      int lat = lvx_arc_latency (seq[pe->producer], seq[q],
					 REG_DEP_TRUE);

	      lvx_lms_push_arc (arcs, pe->producer, q, lat, pe->omega,
				LVX_ARC_REG_RAW);
	      arcs.last ().useidx
		= lvx_lms_reg_index (PATTERN (seq[q]), DF_REF_LOC (use));
	      if (pe->omega == 0 && height[q] < height[pe->producer] + lat)
		height[q] = height[pe->producer] + lat;
	      continue;
	    }

	  /* The last definition strictly before Q.  */
	  bool partial_def = false;
	  for (int i = (int) q - 1; i >= 0 && p < 0; i--)
	    if (lvx_insn_defs_regno_p (seq[i], regno, &partial_def))
	      p = i;

	  /* A PARTIAL REACHING DEFINITION NEEDS AN ARC FROM EVERY CONTRIBUTOR.
	     §7tricies.142.

	     One arc from the last definition would omit the dependence on
	     whatever supplied the rest of the value, so this used to refuse
	     the loop -- 55 of them over four suites, 18 on r0 alone, which is
	     this ABI's first argument and return register, so a narrow write
	     into a wider one feeding a wide read.

	     Refusing was the right conservative answer and the wrong
	     permanent one: the value a use reads is the last FULL definition
	     plus every PARTIAL definition after it, and an arc from each of
	     those is exactly the dependence.  So walk back pushing an arc per
	     contributor and stop at the first full definition, which supplies
	     the whole value and makes anything before it dead.  DF_REF_PARTIAL
	     carries no extents, so coverage cannot be computed -- but it does
	     not need to be: stopping at the full definition is precise, and
	     every arc added is a CONSTRAINT, so the result can only be more
	     constrained than the truth, never less.

	     If the walk reaches the top without a full definition the
	     remainder comes from the previous iteration, which is what the
	     `p < 0' path below already handles, so the loop-carried arc is
	     left to it.  */
	  if (partial_def && lvx_lms_partial_arcs)
	    {
	      bool full_seen = false;
	      int nc = 0;

	      for (int i = (int) q - 1; i >= 0; i--)
		{
		  bool ipart = false;

		  if (!lvx_insn_defs_regno_p (seq[i], regno, &ipart))
		    continue;
		  {
		    int lat2 = lvx_arc_latency (seq[i], seq[q], REG_DEP_TRUE);

		    lvx_lms_push_arc (arcs, (unsigned) i, q, lat2, 0,
				      LVX_ARC_REG_RAW);
		    arcs.last ().useidx
		      = lvx_lms_reg_index (PATTERN (seq[q]), DF_REF_LOC (use));
		    if (height[q] < height[i] + lat2)
		      height[q] = height[i] + lat2;
		    nc++;
		  }
		  if (!ipart)
		    {
		      full_seen = true;
		      break;
		    }
		}
	      if (dump_file && lvx_lms_probe)
		lvx_lms_dump (";; lvx cms:   PARTIALARCS r%u use=%u arcs=%d"
			      " full=%d\n", regno, q, nc, full_seen ? 1 : 0);
	      /* Covered by the contributors above, or by the previous
		 iteration if no full definition was found -- which the
		 p < 0 path supplies.  */
	      if (full_seen || nc > 0)
		continue;
	    }
	  else if (partial_def)
	    {
	      if (why)
		*why = "a partial definition reaches a use";
	      if (dump_file)
		lvx_lms_dump (";; lvx cms: fn=%s loop %d declined, partial"
			      " definition of r%u reaches a use\n",
			      current_function_name (), loop->num, regno);
	      return false;
	    }

	  /* None: the value is the previous iteration's, from the last
	     definition anywhere in the body.  */
	  if (p < 0)
	    {
	      for (int i = (int) n - 1; i >= 0 && p < 0; i--)
		if (lvx_insn_defs_regno_p (seq[i], regno))
		  p = i;
	      w = 1;
	    }

	  /* Live in and never redefined here: no arc.  */
	  if (p < 0)
	    continue;

	  /* INDUCTIVE RELAXATION.  A base+displacement operand based on a
	     simple induction may read any instance of it, the displacement
	     absorbing the difference, so it needs no dependence on the one
	     definition.  Dropping the arc is only sound if codegen then adjusts
	     that displacement -- the paper's fixRelaxed step -- which is why
	     this is behind a flag and not yet on by default.  */
	  if (lvx_lms_inductive_relaxation
	      && regno < FIRST_PSEUDO_REGISTER
	      && bitmap_bit_p (inductions, regno)
	      && lvx_lms_relaxed_base_use_p (seq[q], regno, ivdelta[regno]))
	    {
	      nrelaxed++;
	      continue;
	    }

	  int lat = lvx_arc_latency (seq[p], seq[q], REG_DEP_TRUE);
	  lvx_lms_push_arc (arcs, (unsigned) p, q, lat, w, LVX_ARC_REG_RAW);
	  arcs.last ().useidx
	    = lvx_lms_reg_index (PATTERN (seq[q]), DF_REF_LOC (use));
	  if (w == 0 && height[q] < height[p] + lat)
	    height[q] = height[p] + lat;
	}
    }

  if (dump_file && lvx_lms_probe)
    lvx_lms_dump (";; lvx cms: fn=%s loop %d, %u simple inductions,"
		  " %u relaxed arcs dropped\n", current_function_name (), loop->num,
		  (unsigned) bitmap_count_bits (inductions), nrelaxed);

  /* THE LIVE RANGE PARTITION, built here because the anti-dependence decisions
     below are made from it (§7tricies.29).  It needs the body and nothing
     else -- no schedule, no arcs -- which is exactly why it can come first.
     Dates and the expansion degree are set later, by
     lvx_lms_set_lrange_dates, once SIGMA exists.  */
  auto_vec<lvx_lms_lrange_part *> lrange;
  int ndrop = 0, awhy[3] = { 0, 0, 0 };

  lvx_lms_build_lranges (seq, lrange, NULL, NULL, NULL);

  /* WAR, to the KILLING definition only -- the same correction RAW needs, and
     for the same reason.  A use is anti-dependent on the definition that
     destroys the value it read, which is the FIRST one after it, not on every
     later definition of that register.  With r0 reused six times over in one
     body, all-pairs produced backward arcs like `use at 56 -> def at 41' at
     distance 1, which close phantom circuits exactly as the RAW ones did.

     A use in an instruction that also defines the register needs no arc at
     all: its own definition kills the value, so it is ordered before every
     later one by the output dependences below.  */
  for (unsigned q = 0; q < n; q++)
    {
      df_ref use;

      FOR_EACH_INSN_USE (use, seq[q])
	{
	  unsigned regno = DF_REF_REGNO (use);
	  int p = -1;
	  HOST_WIDE_INT w = 0;
	  /* §7tricies.110: and here the propagated operand must read rS, so
	     that the anti-dependences keeping rS alive to this reader are the
	     ones built.  Omitting this is what made the first attempt
	     miscompile: the renaming had the right register and nothing
	     stopped a later writer of it being scheduled in between.  */
	  {
	    const lvx_lms_prop *pq
	      = lvx_lms_prop_lookup (prop, q,
				     lvx_lms_reg_index (PATTERN (seq[q]),
							DF_REF_LOC (use)));
	    if (pq)
	      regno = pq->regno;
	  }

	  if (lvx_insn_defs_regno_p (seq[q], regno))
	    continue;

	  for (unsigned i = q + 1; i < n && p < 0; i++)
	    if (lvx_insn_defs_regno_p (seq[i], regno))
	      p = (int) i;
	  if (p < 0)
	    {
	      for (unsigned i = 0; i < n && p < 0; i++)
		if (lvx_insn_defs_regno_p (seq[i], regno))
		  p = (int) i;
	      w = 1;
	    }
	  if (p < 0)
	    continue;

	  /* A PARTIAL KILLING WRITE KEEPS ITS ARC, AS ON THE WAW PATH.
	     §7tricies.116.  That path has guarded against this since
	     §7tricies.29 -- a partial write "leaves part of the old value in
	     place, which no live range question captures" -- and this one
	     never did, though the reasoning is the same and does not depend on
	     the arc's kind.  A partial write does not fully define its name's
	     instance: the bytes it does not write still hold what that name
	     held before, which belongs to a DIFFERENT instance, so the
	     expansion does not separate the two references however far apart
	     their iterations are.  */
	  {
	    bool wpartial = false;

	    lvx_insn_defs_regno_p (seq[p], regno, &wpartial);
	    if (wpartial)
	      {
		awhy[2]++;
		lvx_lms_push_arc (arcs, q, (unsigned) p,
				  lvx_arc_latency (seq[q], seq[p],
						   REG_DEP_ANTI),
				  w, LVX_ARC_REG_WAR);
		continue;
	      }
	  }

	  /* Only where renaming cannot remove it, which is a question about
	     the LIVE RANGES the two references belong to (§7tricies.29) and not
	     about the killing instruction's shape.  It was
	     `lvx_insn_def_reads_dest_p (seq[p])', an approximation of
	     `one live range' -- and that predicate was answering FALSE for every
	     instruction, so no register anti-dependence was kept at all.  */
	  if (!lvx_lms_antidep_real_p
		 (lrange,
		  lvx_lms_lrange_of (lrange, q,
				     lvx_lms_reg_index (PATTERN (seq[q]),
							DF_REF_LOC (use)),
				     NULL),
		  lvx_lms_lrange_def_of (lrange, (unsigned) p, regno), awhy,
		  w, q, (unsigned) p))
	    {
	      ndrop++;
	      continue;
	    }

	  lvx_lms_push_arc (arcs, q, (unsigned) p,
			    lvx_arc_latency (seq[q], seq[p], REG_DEP_ANTI),
			    w, LVX_ARC_REG_WAR);
	}
    }

  /* WAW, to the NEXT definition only.  Chaining consecutive definitions orders
     all of them transitively, which is all an output dependence needs; running
     one from every definition to every other adds the same phantom backward
     arcs.  */
  for (unsigned p = 0; p < n; p++)
    {
      df_ref def;

      FOR_EACH_INSN_DEF (def, seq[p])
	{
	  unsigned regno = DF_REF_REGNO (def);
	  /* A partial write into a register tuple leaves the untouched part in
	     place, which is that same destination-as-source.  */
	  bool partial = (DF_REF_FLAGS (def) & DF_REF_PARTIAL) != 0;
	  int q = -1;
	  HOST_WIDE_INT w = 0;

	  for (unsigned i = p + 1; i < n && q < 0; i++)
	    if (lvx_insn_defs_regno_p (seq[i], regno))
	      q = (int) i;
	  if (q < 0)
	    {
	      for (unsigned i = 0; i < n && q < 0; i++)
		if (lvx_insn_defs_regno_p (seq[i], regno))
		  q = (int) i;
	      w = 1;
	    }
	  if (q < 0 || (unsigned) q == p)
	    continue;

	  /* Only where renaming cannot remove it.  Two full, unconditional
	     writes to one register with no read between them are independent
	     once each has its own register; it is only when a write reads its
	     own destination that the later result depends on the earlier --
	     and `reads its own destination' is what puts the two references in
	     ONE LIVE RANGE, which is the question actually asked (§7tricies.29).

	     The shape test was also an OR over the two instructions, so one
	     in-place write kept the arc to a plain full write that the
	     expansion does separate: 20 of 113 kept arcs on the PolyBench
	     loops were of that kind.  A partial write still keeps its arc
	     unconditionally -- it leaves part of the old value in place,
	     which no live range question captures.  */
	  if (!partial
	      && !lvx_lms_antidep_real_p
		    (lrange, lvx_lms_lrange_def_of (lrange, p, regno),
		     lvx_lms_lrange_def_of (lrange, (unsigned) q, regno),
		     awhy, w, p, (unsigned) q, true))
	    {
	      ndrop++;
	      continue;
	    }

	  lvx_lms_push_arc (arcs, p, (unsigned) q,
			    lvx_arc_latency (seq[p], seq[q], REG_DEP_OUTPUT),
			    w, LVX_ARC_REG_WAW);
	}
    }

  /* Memory arcs, shaped as ddg.cc's add_intra_loop_mem_dep and
     add_inter_loop_mem_dep.  The vocabulary here is the memory one -- flow,
     anti, output, input -- not the register one, and the distinction matters:
     dropping a register anti- or output dependence is sound because the
     renaming that -munfold-inner-loops implies removes it, whereas nothing
     renames memory, so flow, anti and output are all kept.  Input
     dependences, read after read, constrain nothing and are not built.

     A memory recurrence is usually the cycle that binds the period, so
     leaving these out made RecMII too small -- the direction that unfolds a
     loop it should refuse.  */
  for (unsigned p = 0; p < n; p++)
    {
      rtx_insn *a = seq[p];
      bool a_w = lvx_mem_write_insn_p (a);
      bool a_r = lvx_mem_read_insn_p (a);

      if (!a_w && !a_r)
	continue;

      for (unsigned q = 0; q < n; q++)
	{
	  rtx_insn *b = seq[q];
	  bool b_w = lvx_mem_write_insn_p (b);
	  bool b_r = lvx_mem_read_insn_p (b);

	  if ((!b_w && !b_r) || !lvx_insns_may_alias_p (a, b))
	    continue;

	  /* Within one iteration, only forward along the chain, and only when
	     the two references cannot be told apart by their addresses.  */
	  if (q > p && !lvx_mems_disjoint_p (seq, p, q))
	    {
	      int lat = 0;
	      if (lvx_lms_mem_independent_p (mivs[p], mivs[q], 0))
		continue;

	      bool have = false;
	      if (a_w && b_r)
		lat = lvx_arc_latency (a, b, REG_DEP_TRUE), have = true;
	      else if (a_w && b_w)
		lat = lvx_arc_latency (a, b, REG_DEP_OUTPUT), have = true;
	      else if (b_w)
		lat = lvx_arc_latency (a, b, REG_DEP_ANTI), have = true;
	      if (have)
		{
		  lvx_lms_push_arc (arcs, p, q, lat, 0, LVX_ARC_MEM_INTRA);
		  if (height[q] < height[p] + lat)
		    height[q] = height[p] + lat;
		}
	    }

	  /* Across the back edge.  loop->safelen is how many iterations may run
	     concurrently without a memory dependence binding, so a dependence
	     that exists at all cannot bind NEARER than SAFELEN iterations away:
	     its distance is SAFELEN, not one.  #pragma GCC ivdep leaves INT_MAX
	     there and #pragma omp simd safelen(n) leaves n.  The generic ddg
	     does not read it; this does.

	     Carrying it as the distance is both correct and self-limiting,
	     which is why there is no separate rule dropping the arc: the
	     requirement is theta - omega * II, so a large distance makes that
	     hugely negative and the arc simply never binds.  An arc farther
	     away than the iterations in flight is slack by arithmetic rather
	     than by a special case.

	     Dropping the arc outright -- which this did until 2026-10-21 -- is
	     WRONG for any finite safelen.  A dependence at distance 4 in a loop
	     scheduled with six stages overlapping does bind, and skipping it
	     would have produced a schedule that violates it.

	     SAFELEN is a floor on the distance, not the distance: the stride
	     test below scans upward from it for the first distance that can
	     actually conflict.  */
	  int sl = lvx_lms_assume_ivdep ? INT_MAX
		   : (loop->safelen > 1 ? loop->safelen : 1);

	  if (sl > LVX_LMS_MAX_DELTA)
	    /* Farther away than the iterations this scheduler ever keeps in
	       flight, so it cannot bind and is dropped rather than carried as
	       a huge distance -- which would be arcs that never bind, and
	       INT_MAX arithmetic through theta - omega * II.  Soundness is not
	       assumed: the weakest dropped safelen is reported out, and the
	       caller rejects the schedule unless safelen >= nstages.  */
	    {
	      if (*dropped_safelen == 0 || *dropped_safelen > sl)
		*dropped_safelen = sl;
	      continue;
	    }

	  /* The stride test, which is what makes a self-dependence of one array
	     through a unit-stride pointer go away without a pragma.

	     SCANNED, not tested once.  Testing only at SL was unsound: for
	     `b[i] = b[i-4] + a[i]' the two references are four elements apart,
	     so they are provably independent at distance one and the arc was
	     DROPPED -- losing a real dependence whose distance is four, which
	     binds as soon as five iterations are in flight.  PolyBench could
	     not expose it, every same-array pair there having its real
	     dependence at distance zero or one, which is also why no circuit in
	     that corpus had a distance above one.

	     So find the SMALLEST distance at or beyond SL at which the two are
	     not provably independent, and carry the arc at that distance.  The
	     scan is what lets a computed distance exceed SL at all, and so it is
	     the only mechanism here that produces omega > 1 -- neither safelen,
	     which is 0 in practice, nor the generic ddg, which hard-codes 1.

	     Independent at every distance in range means the references never
	     conflict within the window the scheduler can overlap.  Beyond the
	     window nothing is claimed: the distance is reported out as a dropped
	     safelen, and the caller rejects the schedule unless it is at least
	     nstages -- the same check that already guards the SL > MAX_DELTA
	     case above.  */
	  HOST_WIDE_INT mw = sl;
	  while (mw <= LVX_LMS_MAX_DELTA
		 && lvx_lms_mem_independent_p (mivs[p], mivs[q], mw))
	    mw++;

	  /* THE TRIP-COUNT RULE.  §7tricies.83.

	     A dependence at distance MW cannot bind in a loop whose iterations
	     never get MW apart.  With MAXTRIP iterations the largest separation
	     available is MAXTRIP - 1, so MW >= MAXTRIP is a PROOF that the two
	     references never conflict in this loop -- not a deferral like the
	     MW > MAX_DELTA case below, which only says "farther than this
	     scheduler overlaps" and therefore owes the caller a DROPPED_SAFELEN
	     to re-check against NSTAGES.  Here nothing is owed and nothing is
	     reported: the arc is simply absent.

	     The motivating shape is MiBench's FFT butterfly, whose trip count
	     is BLOCKEND and whose two references are BLOCKEND elements apart
	     (`k = j + BlockEnd'), so the conflicting distance equals the trip
	     count exactly and the loop is parallel -- which is invisible to a
	     conventional test because the distance and the trip count are each
	     computed in isolation and never compared.  Measured there: the
	     memory arcs cost RecMII 48 against a true register recurrence of 5.

	     WHETHER THIS FIRES AT ALL IS MEASURED, NOT ASSUMED, and the honest
	     expectation is that it rarely does today: get_max_loop_iterations
	     needs loop->any_upper_bound, which record_niter_bound sets at TREE
	     level, while loop_optimizer_init here re-discovers loops from the
	     CFG and hands back fresh structures.  loop-iv.cc could rebuild it
	     but refuses hard registers, and everything is hard after reload.
	     So TRIPDROP in the dump is the figure that says whether recovering
	     the trip count post-reload is worth doing -- and for the FFT it
	     would also have to be SYMBOLIC, the count there being a variable.  */
	  if (maxtrip > 0 && mw >= maxtrip)
	    {
	      ntripdrop++;
	      continue;
	    }

	  if (mw > LVX_LMS_MAX_DELTA)
	    {
	      if (*dropped_safelen == 0 || *dropped_safelen > mw)
		*dropped_safelen = mw;
	      continue;
	    }

	  if (a_w && b_r)
	    lvx_lms_push_arc (arcs, p, q,
			 lvx_arc_latency (a, b, REG_DEP_TRUE), mw,
			 LVX_ARC_MEM_CROSS);
	  else if (a_w && b_w && p != q)
	    lvx_lms_push_arc (arcs, p, q,
			 lvx_arc_latency (a, b, REG_DEP_OUTPUT), mw,
			 LVX_ARC_MEM_CROSS);
	  else if (a_r && b_w && p != q)
	    {
	      lvx_lms_push_arc (arcs, p, q,
			   lvx_arc_latency (a, b, REG_DEP_ANTI), mw,
			   LVX_ARC_MEM_CROSS);
	      lvx_lms_push_arc (arcs, q, p,
			   lvx_arc_latency (b, a, REG_DEP_TRUE), mw,
			   LVX_ARC_MEM_CROSS);
	    }
	}
    }

  /* Reported AFTER the scan, not with the induction counts above -- those are
     printed before the memory arcs exist, so folding TRIPDROP into that line
     would have printed zero always.  MAXTRIP is reported even when nothing was
     dropped, because "the bound is unknown here" is the finding.  */
  if (dump_file && lvx_lms_probe)
    lvx_lms_dump (";; lvx cms: fn=%s loop %d maxtrip="
		  HOST_WIDE_INT_PRINT_DEC " tripsrc=%s tripdrop=%u\n",
		  current_function_name (), loop->num, maxtrip, tripsrc,
		  ntripdrop);

  /* What a conditional branch constrains.  Three things, all at zero latency
     because each is an ordering rather than a delay.

     (1) It guards every instruction below it that cannot be speculated --
     a store or a volatile reference; a load may cross, which is what the
     dismissible variant is for.  For the loop-closing branch, "below it"
     means the next iteration, which is what the distance of one expresses.

     (2) It implicitly USES every register live on the edge by which it
     leaves the EBB, so a write to one of those must not be hoisted above it:
     on the exit path the live-out value would already be clobbered.  This
     one has to be carried here rather than left to the renaming, because
     renaming will not preserve it -- which is the difference from software
     pipelining, where modulo expansion renames and the constraint dissolves.

     (3) Branches keep their relative order, which is the one rule a
     multiple-exit software pipeline construction needs of a local schedule.

     The distance is one for anything that is not forward in the chain, since
     the instance it constrains is then the next iteration's.

     THE LATENCY IS ONE, NOT ZERO, except between two branches.  Every one of
     these arcs was pushed with theta = 0, which permits
     sigma (guarded) == sigma (branch) -- the guarded instruction BUNDLED WITH
     the branch.  On a VLIW that is not "after": both issue together and the
     store executes whatever the branch decides, so a theta of zero expresses
     "may be speculated" while the whole purpose of the arc is to say it may
     not.  Theta of one makes it strictly later.

     BRANCH-TO-BRANCH IS ALSO ONE, since §7tricies.180.  Zero was deliberate
     -- LVX has two BCU slots, so keeping two branches in relative order does
     not require separating them -- but a BLOCK holds one terminator, so two
     jump instances at one date are split across two plan nodes and repaired
     late.  Expressing the separation here instead is what makes "one date in
     one node" mean "one bundle", which §7tricies.179 needed and did not
     have.

     Found from the emitted code, not from the graph: a pipelined
     initialisation loop stored past the end of its array, because the
     prologue ran a stage containing a store before any branch had executed.
     With theta = 1 that cannot be scheduled -- iteration L's store is
     strictly after iteration L-1's branch in absolute date -- so the
     emission's "refuse a prologue store" precondition becomes a check that
     should never fire rather than the thing standing between the analysis and
     a wrong store.  */
  /* WHOSE RESULT REACHES AN INSTRUCTION THAT CANNOT BE SPECULATED.
     §7tricies.221.  Constraint (1) below exempts every load from a branch's
     guard, on the ground that a dismissible load may cross.  It may -- the
     FAULT is suppressed -- but the VALUE it read is then garbage, and that is
     only harmless while nothing consumes it.  An instruction using a
     speculated result must itself be speculatable, and a conditional branch
     and a store are not.

     divmodvx was the case: a pipelined 8-trip loop speculatively read UB[8],
     one element past a uint16_t[8], got zero, and the zero reached the
     conditional branch of GCC's isolated divide-by-zero path, which traps.

     TRANSITIVE through speculatable consumers, because a load feeding an add
     feeding a branch is the same bug one step removed.  The fixpoint is over
     the body, whose length is tens of insns, so its cost is nothing against
     the II search.  A use ANYWHERE in the body counts, including one textually
     before the def -- that is the cross-iteration use, and counting it is the
     conservative direction.  */
  auto_vec<bool> tainted;
  /* §7tricies.223: WHO tainted each insn, so the probe can name the obstacle
     rather than only count it, and WHICH loads the rule actually pinned --
     npinned counts ARCS, one per guarding branch, which is not the number of
     loads and was reported as if it were.  */
  auto_vec<int> taint_via;
  auto_vec<bool> pinned_load;
  int npinned = 0;
  /* §7tricies.224: the body's own definitions -- what makes a load's address
     loop-variant, and so makes it one CMS is going to promote to `.s'.  The
     same set lvx_lms_force_speculative_loads builds; built again here because
     the fixpoint below runs BEFORE that promotion and has to judge a load by
     what it is going to be rather than by what it still is.  */
  auto_bitmap bodydef;

  for (unsigned i = 0; i < n; i++)
    {
      df_ref bd;

      if (!NONDEBUG_INSN_P (seq[i]))
	continue;
      FOR_EACH_INSN_DEF (bd, seq[i])
	bitmap_set_bit (bodydef, DF_REF_REGNO (bd));
    }

  tainted.safe_grow_cleared (n);
  taint_via.safe_grow (n);
  pinned_load.safe_grow_cleared (n);
  for (unsigned i = 0; i < n; i++)
    taint_via[i] = -1;
  if (lvx_lms_pin_tainted_loads)
    {
      bool changed = true;

      while (changed)
	{
	  changed = false;
	  for (unsigned i = 0; i < n; i++)
	    {
	      df_ref d;

	      if (tainted[i] || !NONDEBUG_INSN_P (seq[i]))
		continue;
	      FOR_EACH_INSN_DEF (d, seq[i])
		{
		  unsigned r = DF_REF_REGNO (d);

		  for (unsigned j = 0; j < n && !tainted[i]; j++)
		    {
		      df_ref u;
		      bool taints;

		      if (j == i || !NONDEBUG_INSN_P (seq[j]))
			continue;

		      taints = (tainted[j]
				|| !lvx_lms_speculatable_after_promotion_p
				     (seq[j], bodydef));

		      /* §7tricies.222: a consumer that constraint (1) has
			 ALREADY pinned does not taint what feeds it.  (1)
			 gives a store and a volatile reference an arc from
			 the guarding branch, so neither can be scheduled
			 above it and neither executes speculatively.  For a
			 load feeding only such a consumer: if the next
			 iteration exists, the load read the right address
			 and the store commits the CORRECT value; if it does
			 not, that iteration's store never runs and the
			 garbage is never committed.  A load feeding a BRANCH
			 is the opposite case -- with kunroll=2 the duplicated
			 body's branch evaluates its condition in the same
			 kernel pass, which is divmodvx -- so branches, calls,
			 non-dismissible loads (whose ADDRESS a speculated
			 value can fault) and trapping FP keep tainting.

			 tainted[j] still propagates: an exempt consumer that
			 is itself tainted stays conservative.  */
		      if (taints && !tainted[j]
			  && !lvx_lms_taint_through_stores
			  && (lvx_mem_write_insn_p (seq[j])
			      || volatile_insn_p (PATTERN (seq[j]))))
			taints = false;

		      if (!taints)
			continue;

		      FOR_EACH_INSN_USE (u, seq[j])
			if (DF_REF_REGNO (u) == r)
			  {
			    tainted[i] = true;
			    taint_via[i] = (int) j;
			    changed = true;
			    break;
			  }
		    }
		  if (tainted[i])
		    break;
		}
	    }
	}
    }

  int first_branch = -1, prev_branch = -1;
  for (unsigned p = 0; p < n; p++)
    {
      basic_block bb, exitbb = NULL;
      edge e;
      edge_iterator ei;

      if (!JUMP_P (seq[p]))
	continue;

      /* (3), AT THETA ONE.  §7tricies.180 (author).

	 This was zero, on the argument that LVX has two BCU slots so keeping
	 two branches in relative order does not require separating them, and
	 two branches may therefore be bundled.  They may not, and the cost of
	 pretending otherwise is paid LATE: a block holds one terminator, so
	 two jump instances at one date and iteration are split across two
	 plan nodes, each inverted and redirected to its own drain
	 (§7tricies.45).  That late fix is the only thing that can make "one
	 date in one node" fail to mean "one bundle" -- and that premise is
	 what §7tricies.179's jump-last ordering rule rested on, which
	 miscompiled zlib's Huffman tables into a Z_DATA_ERROR.

	 AND THE DESIGN FORBIDS IT OUTRIGHT (author): on LVX an instruction
	 schedule group must always fit in a bundle, and bundling must never
	 break a parallel group the scheduler assembled -- if it does, that is
	 an irrecoverable error, not a thing to repair.  Two jumps at one date
	 fit a bundle as far as RESOURCES go, BCU availability being 2; what
	 they do not fit is a basic BLOCK, which holds one terminator.  So the
	 late fix split the group across two blocks, and a group split across
	 blocks is not one bundle: the repair was itself the violation.

	 So the separation belongs in the dependence graph, where the scheduler
	 can honour it, rather than in a repair after the fact.  At theta one
	 two branches can never share a cycle, every cycle has at most one
	 terminator, a cycle's slots in one node really are one bundle, and no
	 schedule group is ever handed to bundling that cannot be bundled.

	 INTERIM, AND THE PLANNED RECOVERY IS NOT THIS (author).  Theta one
	 forbids the dual-jump bundle outright, which costs schedules: 6 loops
	 of the four-suite census (MiBench emitted 537 -> 531).  The intended
	 fix is a COMPOSITE INSTRUCTION combining two jumps that presents to
	 GCC as a SINGLE terminator -- then the pair occupies one block, the
	 schedule group is bundleable as the design requires, and the two BCU
	 slots are usable again.  Deferred.  Until it exists this arc is the
	 only thing keeping the scheduler from emitting a group that cannot be
	 bundled, so it should not be relaxed back to zero on the grounds that
	 it costs loops -- the loops it costs are the ones whose groups were
	 being split.  */
      if (prev_branch >= 0)
	lvx_lms_push_arc (arcs, prev_branch, p, 1, 0, LVX_ARC_CTRL_ORDER);
      else
	first_branch = (int) p;
      prev_branch = (int) p;

      bb = BLOCK_FOR_INSN (seq[p]);
      if (bb != NULL)
	FOR_EACH_EDGE (e, ei, bb->succs)
	  if (!bitmap_bit_p (inloop, e->dest->index)
	      && e->dest != EXIT_BLOCK_PTR_FOR_FN (cfun))
	    exitbb = e->dest;

      /* (1) REACHES ONLY THE BLOCK THAT IMMEDIATELY FOLLOWS, not every store
	 in the body.  Because the branches are serialized by (3) at latency
	 ZERO, a path p -> p' -> q gives sigma (q) >= sigma (p) + 1, which is
	 exactly what a direct arc p -> q at latency one gives: every arc to a
	 store beyond the next block is implied and only costs graph size.
	 For a single-block loop the next block IS the block itself, so the
	 two formulations coincide -- which is why this changed nothing
	 measurable on PolyBench, where 117 of 119 bodies are single-block.
	 Kept because an implied arc that is also pushed is a second statement
	 of one rule, and those go stale.  */
      basic_block nextbb = bb;
      if (bb != NULL)
	{
	  FOR_EACH_EDGE (e, ei, bb->succs)
	    if (bitmap_bit_p (inloop, e->dest->index))
	      nextbb = e->dest;
	}

      for (unsigned q = 0; q < n; q++)
	{
	  HOST_WIDE_INT w = (q > p) ? 0 : 1;
	  df_ref def;

	  if (q == p)
	    continue;

	  /* (1) -- and a TAINTED LOAD is guarded exactly as a store is.
	     §7tricies.221: the exemption for loads holds only while nothing
	     consumes what a speculated one read.  */
	  if ((lvx_mem_write_insn_p (seq[q])
	       || volatile_insn_p (PATTERN (seq[q]))
	       || (lvx_lms_pin_tainted_loads && tainted[q]
		   && lvx_mem_read_insn_p (seq[q])
		   && !lvx_mem_write_insn_p (seq[q])))
	      && BLOCK_FOR_INSN (seq[q]) == nextbb)
	    {
	      if (lvx_lms_pin_tainted_loads && tainted[q]
		  && lvx_mem_read_insn_p (seq[q])
		  && !lvx_mem_write_insn_p (seq[q]))
		{
		  npinned++;
		  pinned_load[q] = true;
		}
	      lvx_lms_push_arc (arcs, p, q, 1, w, LVX_ARC_CTRL_STORE);
	    }

	  /* (2) */
	  if (exitbb == NULL)
	    continue;
	  /* MEASUREMENT ONLY, AND UNSOUND WITHOUT THE SPLIT.  §7tricies.99.
	     Dropping these arcs is what the SPLIT achieves soundly -- compute
	     into a scratch register and copy to the architected one, so only
	     the copy carries the arc (mlir-pipeliner-design.md §5a2b/§5a2c).
	     Dropping them alone lets the next iteration's definition of a
	     live-out register move above the exit branch, so the exit returns
	     a value one iteration too far: a wrong program, not a faster one.
	     The lever exists to size the OPPORTUNITY before building the
	     phase, which is the step skipped twice (§7tricies.82, .97), and
	     lvx_option_override refuses it with codegen.  */
	  if (lvx_lms_drop_liveout_arcs)
	    continue;
	  /* THE SPLIT, WHICH THE RENAMING ALREADY PERFORMS.  §7tricies.112.

	     The arc exists because a body instruction writes a register the
	     exit edge reads, so the branch must precede it.  That is only
	     true if the body instruction writes the ARCHITECTED register --
	     and where the range is renamed it does not: it writes a name of
	     its own and the drain copies that back on the way out, which is
	     Young & Smith's live off-trace renaming with the reconciliation
	     off-trace rather than in the loop (§5a2c-bis).  Measured: 123
	     live-out ranges across TSVC2 and MiBench are already renamed and
	     already reconciled in the drains, so for those the arc is a
	     constraint on a write that does not happen.

	     Whether a range IS renamed is decided after scheduling, so the
	     arc is dropped optimistically and the commitment verified before
	     emission: every exit-live range with a body definition must come
	     out with no name equal to its architected register, or the loop
	     is refused.  Bound, transform, re-bound.  */
	  FOR_EACH_INSN_DEF (def, seq[q])
	    if (DF_REF_REGNO (def) < FIRST_PSEUDO_REGISTER
		&& bitmap_bit_p (df_get_live_in (exitbb), DF_REF_REGNO (def)))
	      {
		lvx_lms_push_arc (arcs, p, q, 1, w, LVX_ARC_CTRL_LIVEOUT);
		/* §7tricies.114: whether this arc is DROPPED is decided after
		   the bounds, not here -- it turns on RecMII exceeding
		   ResMII, and neither exists yet.  What is decided here is
		   whether it is droppable at all: a range carried across the
		   back edge is use-headed, its name 0 pinned to the register
		   the preheader wrote, so it does write the architected
		   register in the body and its arc is real whatever the
		   bounds say.  Marked by leaving OMEGA alone and recording
		   the register, which the filter below reads.  */
		break;
	      }
	}
    }

  /* (3) across the back edge: the last branch still precedes the first.  */
  /* §7tricies.221: report what the rule cost, so it is a measurement and not
     an assumption.  Printed even at zero: "no load in this loop feeds a
     branch or a store" is the finding for most loops, and the number only
     means something against the loops where it is not.  */
  if (dump_file && lvx_lms_probe && lvx_lms_pin_tainted_loads)
    {
      int ntaint = 0, nloads = 0;

      for (unsigned i = 0; i < n; i++)
	{
	  if (tainted[i])
	    ntaint++;
	  if (pinned_load[i])
	    nloads++;
	}
      lvx_lms_dump (";; lvx cms: fn=%s loop %d PINNED loads=%d arcs=%d of %d"
		    " tainted insns, body=%u\n", current_function_name (),
		    loop->num, nloads, npinned, ntaint, n);

      /* §7tricies.223: and NAME THE OBSTACLE for each load pinned.  Counting
	 told us the rule costs mibench 39% of its pipelines; it could not tell
	 us WHY a given load is tainted, and two hypotheses died of that -- the
	 store half of the rule looked redundant (it is, and exempting it
	 recovered nothing because these loads do not reach a store) and the
	 drain fix looked like the remedy (it is not, 74% of pinning loops have
	 no known trip count).

	 The chain is walked to its ROOT: taint_via[q] is the consumer whose
	 use tainted q, and that consumer is either itself non-speculatable --
	 the root -- or was tainted in turn, so follow it while the reason
	 reads "speculatable".  The walk is bounded by n because each step
	 moves to a strictly earlier-tainted insn, but it is bounded
	 explicitly anyway: a cycle here would hang the compiler, and a probe
	 must never be able to do that.  */
      for (unsigned q = 0; q < n; q++)
	if (pinned_load[q])
	  {
	    int root = taint_via[q], prev = (int) q;
	    unsigned guard = 0;
	    int qcode = INSN_CODE (seq[q]), rcode;

	    while (root >= 0 && guard++ < n
		   && !strcmp (lvx_lms_unspec_reason (seq[root], bodydef),
			       "speculatable"))
	      {
		prev = root;
		root = taint_via[root];
	      }
	    if (root < 0)
	      root = prev;
	    rcode = INSN_CODE (seq[root]);
	    lvx_lms_dump (";; lvx cms:   PINWHY fn=%s loop %d load uid=%d %s"
			  " <- uid=%d %s %s\n", current_function_name (),
			  loop->num, INSN_UID (seq[q]),
			  qcode >= 0 ? insn_data[qcode].name : "?",
			  INSN_UID (seq[root]),
			  lvx_lms_unspec_reason (seq[root], bodydef),
			  rcode >= 0 ? insn_data[rcode].name : "?");
	  }
    }

  if (first_branch >= 0 && prev_branch > first_branch)
    lvx_lms_push_arc (arcs, prev_branch, first_branch, 0, 1,
		      LVX_ARC_CTRL_BACKEDGE);

  for (unsigned k = 0; k < lrange.length (); k++)
    delete lrange[k];
  /* How many anti-dependences the live range test dropped that the shape test
     would have kept.  The partition itself is not handed out: the caller
     calls the same function on the same body, which is one rule called twice
     rather than two copies of a rule.  */
  if (lrdrop_out)
    lrdrop_out[0] = ndrop, lrdrop_out[1] = awhy[0], lrdrop_out[2] = awhy[1],
    lrdrop_out[3] = awhy[2];

  return true;
}

/* M, R and K for a built graph: the makespan of one iteration, the resource
   bound and the recurrence bound.

   R and K bound the RATE -- both are lower bounds on II, and max (K, R) is
   the lower bound CMS searches upward from.  M is a LENGTH, the span of one
   iteration scheduled alone, and is not a bound on II at all: it is the
   numerator of nstages = ceil (span / II), which is why K >= M is the test
   that says pipelining can buy nothing.  The unfold compares all three.

   Returns false if the parametric solve did not converge, which leaves the
   recurrence bound unknown and so the lower bound unknown.  */

bool
lvx_lms_bounds (class loop *loop, basic_block *body,
		const auto_vec<rtx_insn *> &seq,
		const auto_vec<lvx_lms_arc> &arcs,
		const auto_vec<int> &height, int *pm, int *pr, int *pk,
		auto_vec<int> *crit)
{
  unsigned n = seq.length ();
  rtx_insn *insn;

  /* M, the MAKESPAN of one iteration with no overlap: the last instruction's
     start date plus its own processing time, not plus one.  HEIGHT[i] is a
     start date, so a schedule is finished when every instruction has had its
     processing time, which is what lvx_lms_latency gives -- an issue slot is
     what an instruction takes, but a multi-cycle result is not ready when the
     slot closes.  Taking + 1 instead measured the span as if every instruction
     retired in the cycle it issued, which understates any body ending in a
     load, a multiply or a reduction's FP add.  */
  int m = 1;
  for (unsigned i = 0; i < n; i++)
    m = MAX (m, height[i] + lvx_lms_latency (seq[i]));

  /* R, from the machine description.  */
  struct lvx_sched_resources resources;
  /* And the same counters with the doloop_end charged NOTHING, to measure what
     charging it costs.  §7tricies.86: lvx_lms_resources_add charges a
     doloop_end as ISSUE_TINY + ISSUE_BCU_BRRP -- a decrement and a branch --
     but if reorg_loops maps the loop, that insn emits `# loopdo end' and
     occupies no slot at all, so the charge is an OVERCHARGE for exactly the
     loops that become hardware loops.  It is deliberately conservative, CMS
     being unable to know whether the mapping will succeed (LOOPDO's reach and
     ITER_REG_USED can still refuse it), but "deliberately conservative" and
     "free" are different claims and this is the one that says which.  */
  struct lvx_sched_resources nodl;
  memset (&resources, 0, sizeof (resources));
  memset (&nodl, 0, sizeof (nodl));
  for (unsigned i = 0; i < loop->num_nodes; i++)
    FOR_BB_INSNS (body[i], insn)
      {
	lvx_lms_resources_add (&resources, insn);
	if (!lvx_doloop_end_p (insn))
	  lvx_lms_resources_add (&nodl, insn);
      }
  int r = (int) lvx_sched_resources_full_bundles (&resources);
  r = r ? r : 1;
  {
    int rnodl = (int) lvx_sched_resources_full_bundles (&nodl);

    rnodl = rnodl ? rnodl : 1;
    if (dump_file && lvx_lms_probe && rnodl != r)
      lvx_lms_dump (";; lvx cms: fn=%s loop %d ResMII=%d, uncharged doloop_end"
		    " would give %d\n", current_function_name (), loop->num,
		    r, rnodl);
  }

  /* K, the recurrence bound.  The solver wants node 0 as its root and node
     i + 1 as SEQ[i], with a root arc to every node so each is reachable.  */
  lvx_plc plc;
  for (unsigned i = 0; i <= n; i++)
    plc.add_node ();
  for (unsigned i = 0; i < n; i++)
    plc.add_arc (0, i + 1, 0, 0);
  for (unsigned i = 0; i < arcs.length (); i++)
    plc.add_arc (arcs[i].src + 1, arcs[i].dst + 1, arcs[i].theta,
		 arcs[i].omega);
  if (plc.solve (1) != LVX_PLC_SOLVED)
    return false;

  /* The circuit that last forced the period up is the one that sets K.  Map
     its PLC arc indices back onto ARCS: the first N arcs added were the root's
     one-per-node, so PLC arc N + J is ARCS[J], and a root arc -- which cannot
     be part of a circuit -- maps to -1.  */
  if (crit != NULL)
    {
      const vec<unsigned> &circ = plc.critical_circuit ();
      for (unsigned i = 0; i < circ.length (); i++)
	crit->safe_push (circ[i] >= n ? (int) (circ[i] - n) : -1);
    }

  *pm = m;
  *pr = r;
  *pk = (int) plc.period ();
  return true;
}

/* Schedule LOOP's body as a modulo schedule and report what it took.  Tries
   initiation intervals from the lower bound upward and reports the first that
   converges AND verifies.

   Convergence is NECESSARY BUT NOT SUFFICIENT, so verification is part of
   acceptance.  The scheduler works in absolute cycles while the product is a
   modulo schedule, and those agree only in steady state: two nodes can each
   fit their own absolute cycle and still collide in slot (date mod II) once
   the kernel repeats.  Rather than constrain the search to reserve a modulo
   table from the start -- which would make it iterative modulo scheduling and
   forfeit the convergent character -- a converged schedule that does not
   verify simply fails that II.  The reference does the same shape of thing in
   its checkpoint, which can reject a converged interval because register
   allocation failed.

   Dump only, for now: this is the measurement docs/cms-foundations.md §7bis.6
   asks for -- lowest converging II against the true lower bound -- and it
   needs no cycle counts and no ISS, being a property of the scheduler.  */

/* COPY PROPAGATION ON THE DEPENDENCE GRAPH: THE CANDIDATES.  §7tricies.106.

   An instruction that reads the result of a register copy can be made to read
   the copy's SOURCE instead.  On the graph that is three nodes and two arcs --
   `p -> c' carrying rS into the copy `c: rD = rS', and `c -> u' carrying rD
   into the reader -- and the rewrite replaces the second by

       p -> u    theta = theta (p->c)    omega = omega (p->c) + omega (c->u)

   the omegas ADDING, which is what makes it work across iterations with no
   separate mechanism: `u' in iteration i+1 consumes what `p' produced in i as
   one arc at omega 1.  A circuit through `p -> c -> u' loses the copy's theta.

   It is the only refinement measured in this file that can move BOTH bounds
   the right way: a copy that loses every reader and is not live out DIES, and
   its TINY slot leaves the body, so ResMII falls as well.

   THIS FUNCTION ONLY COUNTS.  Redirecting the arcs without the matching
   rewrite at emission is not an incomplete feature, it is a miscompile: the
   schedule would assume `u' reads rS while the code still reads rD.  So the
   analysis lands first, with its filters measured, and the transformation
   follows behind an option refused with -mlms-codegen until emission can
   honour it.

   The filters, each able to reject on its own:

     TIED    the operand of `u' that reads rD is matched to u's destination.
	     Post-RA copies often exist precisely so an instruction can hold
	     an input and its output in one register; rewriting the input
	     while the destination stays put gives `fma r1 = a, r41',
	     §7tricies.28's unrecognisable insn.  Checked PER OPERAND through
	     the arc's USEIDX -- lvx_insn_has_tied_operand_p answers the
	     coarser question "has any tie" and would reject too much.
   TWO FILTERS WERE TRIED AND BOTH ASKED THE WRONG THING, which is worth
   recording because the right question turns out not to be a graph predicate
   at all.  The first required rS to have no other definition in the body and
   rejected 98 of 98 copies -- post-RA code reuses every hard register, so that
   is not a conservative subset but an empty one, and it asked about the
   REGISTER where the question is about the VALUE.  The second required the
   copy's source reference and the definition reaching it to be one live range,
   and rejected NOTHING on either corpus, because a REG_RAW arc exists exactly
   when a definition reaches a use and the live-range construction puts those
   in one range: the test is a tautology.

   What actually decides legality is whether extending rS's range to the new
   reader leaves the schedule feasible -- its life still within p2unroll * II
   (§7tricies.92) -- and that is a property of a schedule, not of the graph.
   So the candidate count below is an UPPER BOUND, and the judge has to be
   bound/transform/re-bound with the colouring and the invariant deciding.
     MULTIP  the copy has more than one producing arc, so the reader would
	     have to wait on all of them and the composition is not one arc.
     WIDE    the copy is not a single register, where propagating a part of a
	     tuple is not the same transformation.  */

/* RECOMPUTE THE DEPENDENCE HEIGHTS FROM THE ARCS.  §7tricies.106.

   lvx_lms_build_graph fills HEIGHT as it walks the body, in one forward pass
   (`height[q] = height[p] + lat' for every intra-iteration arc), and
   lvx_lms_bounds then READS it to form M.  Redirecting an arc therefore leaves
   HEIGHT describing the graph as it was -- and since a redirection only ever
   shortens a path, the stale value over-states M rather than under-stating it,
   which is conservative and still wrong.  A comment here first claimed the
   bound recomputed it; it does not, and that claim is the kind this file has
   been bitten by twice (§7tricies.103, .104).

   Longest path by relaxation over the intra-iteration arcs.  n rounds suffice
   because that subgraph is acyclic.  Called only when the propagation has
   actually changed something, so the ordinary path keeps construction's own
   values byte for byte.  */

static void
lvx_lms_recompute_heights (const auto_vec<lvx_lms_arc> &arcs,
			   auto_vec<int> &height)
{
  unsigned n = height.length ();

  for (unsigned i = 0; i < n; i++)
    height[i] = 0;
  for (unsigned round = 0; round < n; round++)
    {
      bool moved = false;

      for (unsigned a = 0; a < arcs.length (); a++)
	{
	  /* THE SAME KINDS CONSTRUCTION COUNTS, and no others.  It updates
	     HEIGHT in exactly two places, for REG_RAW and for MEM_INTRA at
	     omega 0.  Including the control arcs here would be defensible as
	     an acyclic makespan and would also make this arm incomparable to
	     the baseline, M moving for a reason that is not the propagation.  */
	  if (arcs[a].omega != 0
	      || (arcs[a].kind != LVX_ARC_REG_RAW
		  && arcs[a].kind != LVX_ARC_MEM_INTRA)
	      || arcs[a].src >= n || arcs[a].dst >= n)
	    continue;
	  int want = height[arcs[a].src] + (int) arcs[a].theta;
	  if (height[arcs[a].dst] < want)
	    height[arcs[a].dst] = want, moved = true;
	}
      if (!moved)
	break;
    }
}

/* COPY PROPAGATION: FIND THE CANDIDATES.  §7tricies.110.

   For each candidate `(p, c, u)' replace `c -> u' by `p -> u' at
   theta (p->c) and omega (p->c) + omega (c->u).  Returns how many arcs were
   redirected.

   MEASUREMENT ONLY, AND REFUSED WITH -mlms-codegen.  Doing this without the
   matching rewrite at emission is not an incomplete feature but a miscompile:
   the schedule would assume `u' reads rS while the code still reads rD.  The
   emission side substitutes registers already (the renaming does it per
   operand through the arc's USEIDX), so the rewrite belongs there -- and
   until it exists, this may only inform a bound, never a program.

   The copy is NOT deleted even when it loses every reader.  Its death is what
   would lower ResMII, and claiming that here would overstate the result: a
   dead copy still occupies its slot until emission omits it.  */

static int
lvx_lms_find_copy_prop (const auto_vec<rtx_insn *> &seq,
			const auto_vec<lvx_lms_arc> &arcs,
			auto_vec<lvx_lms_prop> *prop)
{
  int ndone = 0;

  for (unsigned c = 0; c < seq.length (); c++)
    {
      rtx set = single_set (seq[c]);
      int np = 0, pidx = -1;

      if (set == NULL_RTX || !REG_P (SET_DEST (set)) || !REG_P (SET_SRC (set)))
	continue;
      if (REGNO (SET_DEST (set)) >= FIRST_PSEUDO_REGISTER
	  || REGNO (SET_SRC (set)) >= FIRST_PSEUDO_REGISTER
	  || REG_NREGS (SET_DEST (set)) != 1
	  || REG_NREGS (SET_SRC (set)) != 1)
	continue;
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].dst == c && arcs[a].kind == LVX_ARC_REG_RAW)
	  np++, pidx = (int) a;
      if (np != 1)
	continue;
      for (unsigned a = 0; a < arcs.length (); a++)
	{
	  if (arcs[a].src != c || arcs[a].kind != LVX_ARC_REG_RAW)
	    continue;
	  if (lvx_lms_operand_tied_p (seq[arcs[a].dst], arcs[a].useidx))
	    continue;
	  /* ONE OPERAND, ONE ANSWER.  Two arcs reaching one operand from
	     different copies is the graph describing something this rewrite
	     cannot express, and the lookup would answer with whichever was
	     recorded first.  Decline the second rather than lose it.  */
	  if (lvx_lms_prop_lookup (prop, arcs[a].dst, arcs[a].useidx))
	    continue;
	  /* One iteration back is all DELAYED can say.  */
	  if (arcs[a].omega + arcs[pidx].omega > 1)
	    continue;
	  {
	    lvx_lms_prop pe;

	    pe.insn = arcs[a].dst;
	    pe.slot = arcs[a].useidx;
	    pe.regno = REGNO (SET_SRC (set));
	    /* PINNED: the definition that fed THIS copy, not whichever write
	       of that register happens to precede the reader.  */
	    pe.producer = arcs[pidx].src;
	    pe.omega = arcs[a].omega + arcs[pidx].omega;
	    pe.delayed = pe.omega != 0;
	    prop->safe_push (pe);
	  }
	  lvx_lms_dump (";; lvx cms:   cprop insn %u slot %d: r%u<-r%u"
			" from insn %u omega %d\n",
			arcs[a].dst, arcs[a].useidx,
			REGNO (SET_DEST (set)), REGNO (SET_SRC (set)),
			arcs[pidx].src,
			(int) (arcs[a].omega + arcs[pidx].omega));
	  ndone++;
	}
    }
  return ndone;
}

static void
lvx_lms_count_copy_prop (const auto_vec<rtx_insn *> &seq,
			 const auto_vec<lvx_lms_arc> &arcs,
			 int *ncand_out, int *ntied_out, int *nmultidef_out,
			 int *nmultip_out, int *nwide_out)
{
  int ncand = 0, ntied = 0, nmultidef = 0, nmultip = 0, nwide = 0;

  *ncand_out = *ntied_out = *nmultidef_out = *nmultip_out = *nwide_out = 0;

  for (unsigned c = 0; c < seq.length (); c++)
    {
      rtx set = single_set (seq[c]);
      unsigned sreg, dreg;
      int np = 0, pidx = -1, ndefs = 0;

      if (set == NULL_RTX || !REG_P (SET_DEST (set)) || !REG_P (SET_SRC (set)))
	continue;
      dreg = REGNO (SET_DEST (set));
      sreg = REGNO (SET_SRC (set));
      if (dreg >= FIRST_PSEUDO_REGISTER || sreg >= FIRST_PSEUDO_REGISTER)
	continue;
      if (REG_NREGS (SET_DEST (set)) != 1 || REG_NREGS (SET_SRC (set)) != 1)
	{
	  nwide++;
	  continue;
	}

      /* The producing arcs of the copy, and the definitions of rS anywhere
	 in the body.  */
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].dst == c && arcs[a].kind == LVX_ARC_REG_RAW)
	  np++, pidx = (int) a;
      if (np != 1)
	{
	  nmultip++;
	  continue;
	}
      (void) ndefs;

      for (unsigned a = 0; a < arcs.length (); a++)
	{
	  if (arcs[a].src != c || arcs[a].kind != LVX_ARC_REG_RAW)
	    continue;
	  if (lvx_lms_operand_tied_p (seq[arcs[a].dst], arcs[a].useidx))
	    ntied++;
	  else
	    ncand++;
	}
      (void) pidx;
    }
  *ncand_out = ncand;
  *ntied_out = ntied;
  *nmultidef_out = nmultidef;
  *nmultip_out = nmultip;
  *nwide_out = nwide;
}

static void
lvx_lms_pipeline_loop (class loop *loop, basic_block *body)
{
  /* BISECTING A WRONG ANSWER DOWN TO ONE LOOP.  §7tricies.133.  Scoping CMS
     to one translation unit is as fine as the flags went, and a function with
     several pipelined loops then gives one verdict for all of them -- which
     is how `CMS miscompiles inflate_fast' stayed unattributed.  Numbered by
     loop->num, which is what every dump line already prints, so a dump and a
     bisect arm name the same loop.  */
  if (lvx_lms_only_loop != 0 && loop->num != (int) lvx_lms_only_loop)
    {
      lvx_lms_dump (";; lvx cms: fn=%s loop %d skipped by -mlms-only-loop\n",
		    current_function_name (), loop->num);
      return;
    }

  /* THE TIED COPIES GO IN BEFORE THE GRAPH.  §7tricies.140.  Their resource
     cost then sits inside the bounds the II search respects, which is the
     same discipline as the live-out arc drop: bound, transform, re-bound --
     and not a promise emission has to keep afterwards.  */
  if (lvx_lms_tied_copy)
    {
      int nc = lvx_lms_tied_copies (loop, body);

      if (nc && dump_file && lvx_lms_probe)
	lvx_lms_dump (";; lvx cms: fn=%s loop %d %d tied copy(ies) inserted\n",
		      current_function_name (), loop->num, nc);
    }

  auto_vec<rtx_insn *> seq;
  auto_vec<lvx_lms_arc> arcs;
  auto_vec<int> height;
  int dropped_safelen = 0;

  int lrdropped[4] = { 0, 0, 0, 0 };
  {
    const char *gwhy = "the body graph cannot be built";

    if (!lvx_lms_build_graph (loop, body, seq, arcs, height, &dropped_safelen,
			      lrdropped, NULL, &gwhy))
      {
	lvx_lms_dump (";; lvx cms: fn=%s loop %d declined, %s\n",
		      current_function_name (), loop->num, gwhy);
	return;
      }
  }

  /* COPY PROPAGATION, BETWEEN THE GRAPH AND THE BOUNDS.  §7tricies.106.
     Here because it changes RecMII and the bounds are computed next, and
     because it needs nothing a schedule would have to provide: the candidate
     test is the arcs plus a per-operand tie check.  */
  /* IDENTIFY, THEN REBUILD.  §7tricies.110.

     Redirecting the arcs in place and telling only the live-range builder
     gave the renaming the right register and nothing to protect it: the arcs
     had been constructed before the record existed, so no anti-dependence
     kept the source live from its definition to the new reader.  Wrong
     answers on two of the seven gate programs.

     So the record is made first and the graph is built again knowing it.  One
     description of the dependences, derived once, with the flow arc and the
     anti-dependences both coming out of the same walks -- and HEIGHT out of
     the rebuild rather than needing its own repair.  */
  auto_vec<lvx_lms_prop> prop;
  if (lvx_lms_copy_prop && lvx_lms_find_copy_prop (seq, arcs, &prop))
    {
      lvx_lms_dump (";; lvx cms: fn=%s loop %d cprop found=%u, rebuilding\n",
		    current_function_name (), loop->num, prop.length ());
      arcs.truncate (0);
      height.truncate (0);
      if (!lvx_lms_build_graph (loop, body, seq, arcs, height,
				&dropped_safelen, lrdropped, &prop))
	{
	  /* The propagated graph need not walk where the original did: the
	     reader may now precede its producer in program order, so an
	     intra-iteration arc runs backward.  Declining is correct -- and
	     it is a decline, not a different graph, because the producer is
	     pinned rather than searched for.  */
	  lvx_lms_dump (";; lvx cms: fn=%s loop %d declined, the propagated"
			" graph does not walk\n",
			current_function_name (), loop->num);
	  return;
	}
    }

  /* THE HEIGHTS, SEPARATELY.  §7tricies.107.

     The propagation needs these recomputed -- lvx_lms_bounds READS HEIGHT
     rather than deriving it, so a redirected arc would otherwise leave the
     makespan describing the graph as it was.  But recomputing only on that
     path confounded the experiment, because the recompute does not agree with
     construction even when nothing is redirected: construction fills HEIGHT
     DURING the arc-building walk, so an arc out of P can be handled before an
     arc INTO P has raised P's height, and the single interleaved pass
     UNDER-states the longest path.  Measured as M rising on s276 loop 2 (30 to
     36) and s443 loop 2 (30 to 35) under propagation, which redirection alone
     cannot do since it only shortens paths.

     So the recompute is its own lever, and the propagation arm has to be read
     against a baseline that shares it.  Whether construction's under-estimate
     is worth fixing on its own account is a separate question with its own
     answer -- M feeds the stage count and the span.  */
  if (lvx_lms_recompute_heights_p)
    lvx_lms_recompute_heights (arcs, height);

  int m, r, k;
  auto_vec<int> crit;
  if (!lvx_lms_bounds (loop, body, seq, arcs, height, &m, &r, &k, &crit))
    {
      lvx_lms_dump (";; lvx cms: fn=%s loop %d declined, the recurrence"
		    " bound did not solve\n", current_function_name (), loop->num);
      return;
    }

  /* THE LIVE-OUT DROP, DECIDED ON THE BOUNDS.  §7tricies.114.

     Dropping a CTRL_LIVEOUT arc can only lower RecMII, never ResMII, so
     `lower' improves only where the RECURRENCE is what binds:
     `RecMII > ResMII', strictly.  At equality the resource bound already pins
     the maximum, so lowering RecMII moves nothing -- and the drop still
     perturbs the scheduler, which was measured taking `s315 loop 1' from II 4
     to 5.  `>=' would admit precisely the cases whose upside is zero and whose
     downside is not.

     Decided here rather than in the arc builder because neither bound exists
     there.  It needs no graph rebuild: a CTRL_LIVEOUT arc carries theta 1 and
     never feeds HEIGHT, which construction updates for REG_RAW and MEM_INTRA
     alone, so removing one leaves the makespan untouched.

     A range carried across the back edge keeps its arc whatever the bounds
     say: it is use-headed, name 0 pinned to the register the preheader wrote,
     so it does write the architected register and the colouring cannot promise
     otherwise (§7tricies.113).

     Kept only if the bound STRICTLY improves.  Equal is the perturbation
     without the gain.  */
  if (lvx_lms_liveout_split && k > r)
    {
      auto_vec<lvx_lms_arc> kept;
      auto_vec<int> crit2;
      auto_vec<lvx_lms_lrange_part *> dlr;
      int m2, r2, k2, ndrop = 0;

      /* The same function on the same body, for the same reason
	 §7tricies.110 gives: the drop must read the partition the drain
	 will act on, not a different description of the program.  Only
	 EXIT_LIVE, EFFECTS[0] and USE_FIRST are consulted, all of which
	 lvx_lms_build_lranges settles without dates.  */
      lvx_lms_build_lranges (seq, dlr, NULL, NULL, NULL, NULL, NULL, &prop);

      for (unsigned a = 0; a < arcs.length (); a++)
	{
	  /* §7tricies.155: BOTH keeps express the same pin, so the lever has
	     to lift both or it measures nothing.  LVX_LMS_LIVEOUT_ARC_PINNED_P
	     keeps the arc of an insn defining a register live-in at the loop
	     HEADER -- the loop-carried case, which is the use-headed case --
	     so a lever that only relaxed RECONCILED_P's USE_FIRST clause was
	     unreachable for exactly the arcs it meant to free, and both arms
	     measured identically.  */
	  if (arcs[a].kind == LVX_ARC_CTRL_LIVEOUT
	      && (lvx_lms_usefirst_unpinned
		  || (lvx_lms_preheader_copy && (lvx_lms_ph_parts & 4))
		  || !lvx_lms_liveout_arc_pinned_p (loop, seq, arcs[a].dst))
	      && lvx_lms_liveout_arc_reconciled_p (dlr, arcs[a].dst))
	    {
	      ndrop++;
	      continue;
	    }
	  kept.safe_push (arcs[a]);
	}
      for (unsigned z = 0; z < dlr.length (); z++)
	delete dlr[z];
      dlr.truncate (0);
      if (ndrop
	  && lvx_lms_bounds (loop, body, seq, kept, height, &m2, &r2, &k2,
			     &crit2)
	  && MAX (k2, r2) < MAX (k, r))
	{
	  lvx_lms_dump (";; lvx cms: fn=%s loop %d liveout split: dropped %d"
			" arc(s), lower %d -> %d\n", current_function_name (),
			loop->num, ndrop, MAX (k, r), MAX (k2, r2));
	  arcs.truncate (0);
	  for (unsigned a = 0; a < kept.length (); a++)
	    arcs.safe_push (kept[a]);
	  m = m2, r = r2, k = k2;
	  crit.truncate (0);
	  for (unsigned a = 0; a < crit2.length (); a++)
	    crit.safe_push (crit2[a]);
	}
    }

  int delta = lvx_lms_delta (arcs);
  int lower = MAX (k, r);

  /* How tight the resource bound is at LOWER: for the most-demanded counter,
     DEMAND against LOWER * AVAILABILITY.  When those are equal, achieving
     ResMII needs a PERFECT packing of the modulo reservation table -- every
     cycle completely full in that resource -- which is a bin-packing question
     and usually infeasible.  Reported because it decides whether an II of
     lower + 1 is a scheduler failure or an unachievable bound.  */
  struct lvx_sched_resources rdem;
  memset (&rdem, 0, sizeof (rdem));
  for (unsigned i = 0; i < loop->num_nodes; i++)
    {
      rtx_insn *bi;
      FOR_BB_INSNS (body[i], bi)
	lvx_lms_resources_add (&rdem, bi);
    }
  const char *tight_name = "none";
  int tight_dem = 0, tight_cap = 0;
  double tight_frac = 0.0;
#define LVX_LMS_TIGHT(R)						\
  {									\
    unsigned avail = lvx_issue_availability.R;				\
    if (avail > 0 && rdem.R##_count > 0)				\
      {									\
	double f = (double) rdem.R##_count					\
		   / ((double) lower * avail);				\
	if (f > tight_frac)						\
	  {								\
	    tight_frac = f;						\
	    tight_name = #R;						\
	    tight_dem = (int) rdem.R##_count;				\
	    tight_cap = (int) (lower * avail);				\
	  }								\
      }									\
  }
  LVX_ISSUE_RESOURCES (LVX_LMS_TIGHT)
#undef LVX_LMS_TIGHT
  auto_vec<int> sigma;
  int found = 0;
  const char *last_bad = NULL;

  /* At each candidate II, spend the SWO budget before moving on.  A budget of
     one is a single greedy construction, which is what this did before the SWO
     cycle existed, so the baseline is reproducible with -mcms-swo-budget=1.  */
  int passes = 0;
  int budget = MAX (1, lvx_cms_swo_budget);
  /* The branch pin can raise II -- it is a release floor, so it cannot make the
     problem infeasible, but it can make this II infeasible and push the search
     on.  Measured on PolyBench it improves 17 loops and costs one (syr2k's
     loop 8, II 3 -> 6).  Policy 3 therefore takes the pin only where it is
     free: try each II pinned, and before giving up on that II, try it again
     unpinned.  II is never lost and the epilogue saving is taken wherever it
     costs nothing.  */
  int pin = lvx_cms_branch_stage;
  /* -mlms-ii-bias starts the search above the bound.  It exists to answer a
     question the ordinary search cannot: when two schedulers disagree only
     because one of them had to take a larger II, comparing them needs both to
     be at the SAME larger II.  §7quinvicies.  */
  int first_ii = lower + (int) lvx_lms_ii_bias;
  /* THE MCRL GATE (§7septvicies.4).  MCRL release dates reduce pressure 12.8%
     and recover the dependence-only makespan on the loops that overshot it
     most -- cholesky, lu and ludcmp from 18 stages to 10 -- but applied
     unconditionally they cost II in 43 of 162 loops, because a release date is
     a floor and a loop that was not register-constrained pays for pressure
     that was not binding.  So they are gated, in the shape the branch pin's
     policy 3 already uses: try each II WITHOUT them first, and only reach for
     them before giving up on that II.  II is never lost.

     That is arm A, the register case.  Arm B is the makespan case, which the
     first arm cannot see because there the un-seeded construction SUCCEEDS:
     if its span overshoots the graph's own makespan bound, MCRL is tried at
     the same II and kept only if it verifies and the span is shorter.  */
  int mcrl_used = 0, mcrl_arm = 0;
  for (int try_ii = first_ii; try_ii <= first_ii + 8 && !found; try_ii++)
    {
      /* The un-seeded attempt -- un-seeded by DEFAULT, since
	 lvx_cms_mcrl_release is 0 unless asked for.  Passing a literal 0 here
	 instead made `-mcms-mcrl-release=' dead, which the normalised lever
	 sweep caught by reading it as bit-identical to the baseline in every
	 column (§7octovicies.4).  */
      if (lvx_cms_swo_at_ii (seq, arcs, height, try_ii, delta, sigma,
			     budget, &passes, &last_bad,
			     pin == 3 ? 1 : pin, lvx_cms_mcrl_release,
			     df_get_live_in (loop->header), &crit))
	found = try_ii;
      else if (pin == 3
	       && lvx_cms_swo_at_ii (seq, arcs, height, try_ii, delta, sigma,
				     budget, &passes, &last_bad, 0,
				     lvx_cms_mcrl_release,
				     df_get_live_in (loop->header), &crit))
	found = try_ii;
      /* Arm A: this II refused an un-seeded construction.  Before raising it,
	 ask MCRL where the tasks want to go.

	 ARM A IS ONLY SOUND WITH THE COLOURING IN ACCEPTANCE, and measurement
	 showed why rather than argument: with -mno-lms-regalloc-feasible the
	 acceptance test is verification alone, so arm A happily takes a
	 SMALLER II with a kernel that cannot be coloured -- fdtd-2d's loop 14
	 went II 9 -> 7 and 0 -> 20 uncolourable values, a smaller II being more
	 stages and so more pressure (§7octodecies).  With the colouring in
	 acceptance the same loop goes 11 -> 9 and stays colourable.  That is
	 why -mlms-regalloc-feasible now defaults ON: the two are one mechanism,
	 and the gate is what pays back most of what the colouring costs in II.
	 §7septvicies.5.  */
      else if (lvx_cms_mcrl_gate
	       && lvx_cms_swo_at_ii (seq, arcs, height, try_ii, delta, sigma,
				     budget, &passes, &last_bad,
				     pin == 3 ? 1 : pin, 2,
				     df_get_live_in (loop->header), &crit))
	found = try_ii, mcrl_used = 1, mcrl_arm = 1;
    }

  /* Arm B: the span overshoots the graph's makespan bound, so a shorter one
     exists as far as the dependences are concerned and list scheduling simply
     did not find it.  Of the 18 PolyBench loops in that state, MCRL recovers
     the bound on five -- and those five are the deep pipelines whose stage
     count is what the expansion pays for.  */
  if (found && lvx_cms_mcrl_gate && !mcrl_used && !lvx_cms_mcrl_release)
    {
      auto_vec<int> ts, sl, nodates;
      int wfirst = -1, wmake = 0, wempty = 0;
      lvx_lms_path_metrics (seq, arcs, height, ts, sl);
      lvx_lms_dates_in_window (seq, arcs, nodates, ts, found, &wfirst, &wmake,
			       &wempty);
      int have = lvx_lms_sigma_span (sigma);
      if (wmake > 0 && have > wmake)
	{
	  auto_vec<int> alt;
	  int p2 = 0;
	  const char *b2 = NULL;
	  if (lvx_cms_swo_at_ii (seq, arcs, height, found, delta, alt, budget,
				 &p2, &b2, pin == 3 ? 1 : pin, 2,
				 df_get_live_in (loop->header), &crit)
	      && lvx_lms_sigma_span (alt) < have)
	    {
	      sigma.truncate (0);
	      for (unsigned i = 0; i < alt.length (); i++)
		sigma.safe_push (alt[i]);
	      passes += p2;
	      mcrl_used = 1;
	      mcrl_arm = 2;
	    }
	}
    }

  /* BLAMED= IS RETIRED.  §7tricies.188.  It named the counter that cost the
     most displacement across the search, which required the per-resource
     attribution the forward walk produced -- and that walk was the only thing
     that indexed a cycle other than the clock, so it was what the 75-row table
     paid for.  The static pressure `tight=' remains, computed from the body's
     own demand and independent of any schedule.  */
  const char *blamed = "retired";
  int blamed_cost = 0;

  /* Whether reorg_loops will put a LOOPDO on this loop.  Reported rather than
     refused: a hardware loop is a HELP to a pipeliner, not an obstacle --
     the loopback branch is implicit, so control speculation past it needs no
     justification and the exit test costs no issue slot.  What it does cost is
     codegen, which has to adjust the trip count for the stages in flight.  */
  bool hw = lvx_loop_has_doloop_p (loop);
  /* §7tricies.196: and whether the mapping has ALREADY happened, which is what
     decides whether the doloop_end costs anything.  */
  rtx_insn *loopdo = lvx_lms_find_loopdo (loop);

  if (!found)
    {
      lvx_lms_dump (";; lvx cms: fn=%s loop %d NO schedule through II=%d"
		    " (lower=%d delta=%d M=%d passes=%d, %u nodes %u arcs)%s%s%s\n",
		    current_function_name (), loop->num, lower + 8, lower, delta, m,
		    passes,
		    seq.length (), arcs.length (), hw ? " hwloop" : "",
		    last_bad ? " -- converged but unverified: " : "",
		    last_bad ? last_bad : "");
      return;
    }

  /* The telling figure is not II against its lower bound but II against ONE
     ITERATION'S span: nstages = ceil (span / II) is how many iterations are
     in flight, and nstages <= 1 means pipelining buys nothing however tight
     the II.  */
  /* SPAN is the ACHIEVED schedule's extent, which can exceed M when resources
     forced stretching; M is its dependence-only lower bound.  nstages counts
     iterations in flight, so it is the achieved one that decides.

     THE CYCLIC MAKESPAN DOES NOT CARRY THE LAST INSTRUCTION'S LATENCY, and
     this is the author's point (2026-10-27): a makespan runs from the first
     schedule date to the last cycle a RESOURCE is in use, and under CMS's
     model every reservation occupies exactly one cycle
     (lvx_lms_slot_fits_p tests a single cycle's counters), so that last cycle
     is the last ISSUE and the makespan is hi - lo + 1.  `kvx-unfold.cc' has
     it right: `makespan = max_sched_date - min_sched_date + 1'.

     Adding the final latency is right ACYCLICALLY, for a reason that is
     specifically acyclic: nothing follows the last instruction, so if the
     region's makespan does not wait for its result, nobody does.  Cyclically
     the latency is on the ARC, and the arcs are what a modulo schedule
     satisfies by construction: a consumer at omega = 0 sits at >= sigma +
     theta, already inside the span, and a consumer at omega > 0 has its
     latency paid by sigma (dst) - sigma (src) >= theta - omega * II, which is
     what RecMII is made of.  Charging it to the span charges it twice.

     And from what nstages is FOR: an iteration is in flight in the KERNEL
     from its first issue to its last issue -- once its last instruction has
     issued the kernel has nothing further to do for it, and the result still
     in the pipe is collected by a consumer that belongs to its own stage and
     is counted THERE.  The one place the latency belongs is the epilogue,
     where a live-out must be complete before the exit reads it; that is the
     live-out reconciliation of §7quater.5, an epilogue length rather than a
     stage count.  So M keeps the correction (M is one iteration with NO
     overlap, the acyclic figure compared against K) and the span loses it.

     The + 1 is required, not cosmetic: dates 0..6 at II = 3 overlap three
     iterations at cycle 6 -- iteration 0's last, 1's middle, 2's first -- and
     ceil (7/3) = 3 where ceil (6/3) = 2.

     SPANLAT is the latency-inclusive figure, kept for the DROP_OK gate below
     and to measure the difference.  §7quinvicies.5.  */
  int lo = sigma[0], hi = sigma[0], hilat = sigma[0] + lvx_lms_latency (seq[0]);
  for (unsigned i = 1; i < sigma.length (); i++)
    {
      int done = sigma[i] + lvx_lms_latency (seq[i]);
      if (lo > sigma[i])
	lo = sigma[i];
      if (hi < sigma[i])
	hi = sigma[i];
      if (hilat < done)
	hilat = done;
    }
  int span = hi - lo + 1;
  int spanlat = hilat - lo;
  int nstages = (span + found - 1) / found;

  /* The drop rule, checked rather than assumed: a memory dependence dropped
     at distance SAFELEN is only absent from the schedule if iterations k and
     k + safelen never overlap, which needs safelen >= nstages.  Deliberately
     on the LATENCY-INCLUSIVE span: a correctness gate should not be loosened
     as a side effect of a makespan definition, and spanlat >= span always.  */
  int nstageslat = (spanlat + found - 1) / found;
  bool drop_ok = (dropped_safelen == 0 || dropped_safelen >= nstageslat);

  /* Empty modulo cycles.  Bundle formation on LVX is an annotation: asm output
     closes a bundle only where LVX_SCHED2_INSN_STOP is set, and nothing
     materialises a cycle that holds no instruction.  So a kernel whose modulo
     cycle C is empty is emitted one bundle short and runs in fewer than II
     cycles, which is not the schedule that was computed -- on an interlocked
     machine it is correct but stalls, and the achieved II is not the scheduled
     one.  Counting them says whether the annotation has to emit explicit nop
     bundles.  See docs/cms-foundations.md §7sedecies.  */
  int emptycyc = 0;
  {
    auto_vec<bool> occupied;
    for (int c = 0; c < found; c++)
      occupied.safe_push (false);
    for (unsigned i = 0; i < seq.length (); i++)
      occupied[(sigma[i] - lo) % found] = true;
    for (int c = 0; c < found; c++)
      if (!occupied[c])
	emptycyc++;
  }

  /* BS, the stage in which the loop-back branch issues, and the partial
     epilogues it implies.  Llosa & Freudenberger (MICRO-35 2002) factor a
     pipelined loop's code size into three terms -- the stage count, this
     branch stage, and the modulo expansion degree -- and SC - BS - 1 partial
     epilogues are needed to finish the iterations left in flight by the early
     exits that a prologue-scheduled branch leaves open.  A branch in the last
     stage therefore costs none, which is what a hardware loop gives for free.
     See docs/cms-foundations.md §7undecies.2.

     SEQ is in chain order from the header, so the loop-back branch is the LAST
     jump in it; an earlier jump is an exit, not the back edge.  */
  int bs = -1;
  for (unsigned i = 0; i < seq.length (); i++)
    if (JUMP_P (seq[i]))
      bs = (sigma[i] - lo) / found;
  int preps = bs < 0 ? 0 : MAX (0, nstages - bs - 1);

  /* The modulo expansion degree, per value and for the loop.  A value defined
     by SEQ[i] and read through a reg-RAW arc (i -> u) of distance OMEGA is
     consumed, in absolute time, at SIGMA[u] + OMEGA * II, so the value's
     lifetime is the latest such date less its own.  PaCT 1997 §3 needs
     m_i = ceil (lifetime / II) names for it.

     Three figures, because the three designs of §7undecies.3 differ in exactly
     this: MVEMAX is the largest m_i, which is the kernel unroll factor only if
     every value is expanded that many times (Llosa's global Kmin); MVEP2 is the
     largest m_i rounded up to a power of two, which is the reference's KUNROLL
     and is a legal common multiple because the rounding closes the degrees
     under divisibility; MVELCM is what exact per-value degrees would require,
     reported as 0 when it exceeds the 2^20 cap, which already means unusable.
     MVEN counts the values needing expansion at all and MVETOP how many attain
     the maximum -- Llosa observed that a large degree is usually caused by a
     single lifetime, which MVETOP == 1 is the test of.  MVE2ND is the largest
     degree below the maximum, which is what says whether splitting the longest
     lifetime with one copy would actually help: after one split the degree
     becomes max (MVE2ND, ceil (MVEMAX / 2)).  */
  int mvemax = 1, mvep2 = 1, mven = 0, mvetop = 0, mve2nd = 0, mveregs = 0;
  /* Per expanded value, what a splitting copy would have to work with.  */
  auto_vec<int> sp_def, sp_lat, sp_last, sp_deg, sp_nregs, sp_ec;
  unsigned HOST_WIDE_INT mvelcm = 1;
  bool mvelcm_overflow = false;
  for (unsigned i = 0; i < seq.length (); i++)
    {
      int last = -1;
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].src == i && arcs[a].kind == LVX_ARC_REG_RAW)
	  {
	    int when = sigma[arcs[a].dst] + (int) arcs[a].omega * found;
	    if (when > last)
	      last = when;
	  }
      if (last < 0)
	continue;
      int life = last - sigma[i];
      if (life <= 0)
	continue;
      int m_i = (life + found - 1) / found;
      if (m_i <= 1)
	continue;
      mven++;
      sp_def.safe_push (sigma[i]);
      sp_lat.safe_push (lvx_lms_latency (seq[i]));
      sp_last.safe_push (last);
      sp_deg.safe_push (m_i);
      /* How many 64-bit registers the value occupies, which is how many copy
	 instructions a split costs: a pair needs two COPYD or one CATDQ, a
	 quad two CATDQ.  See §7undecies.4d.  */
      {
	rtx dset = single_set (seq[i]);
	int nr = 1;
	if (dset && REG_P (SET_DEST (dset)))
	  nr = REG_NREGS (SET_DEST (dset));
	sp_nregs.safe_push (MAX (nr, 1));
      }
      sp_ec.safe_push (lvx_insn_dest_earlyclobber_p (seq[i]) ? 1 : 0);
      int p2 = 1;
      while (p2 < m_i)
	p2 <<= 1;
      if (m_i > mvemax)
	{
	  if (mvemax > mve2nd)
	    mve2nd = mvemax;
	  mvemax = m_i;
	  mvetop = 1;
	}
      else if (m_i == mvemax)
	mvetop++;
      else if (m_i > mve2nd)
	mve2nd = m_i;
      if (p2 > mvep2)
	mvep2 = p2;

      /* The REGISTER COST of expanding this value: one name per stage it
	 spans, times the registers each name occupies.  This is the figure
	 uncached loads make interesting -- a 15-cycle load at II=4 gives its
	 result a four-stage lifetime, so four names, and a quad result four
	 aligned quads.  Summed over the loop it is what modulo expansion
	 actually demands of the 64 GPRs, and it is the pressure the degree is
	 only a proxy for.  MVEREGS uses the power-of-two degree because that is
	 what the reference allocates (§7quater.5); MVEREGSX the degree that
	 survives copy insertion.  */
      {
	rtx dset0 = single_set (seq[i]);
	int nr0 = 1;
	if (dset0 && REG_P (SET_DEST (dset0)))
	  nr0 = REG_NREGS (SET_DEST (dset0));
	mveregs += MAX (nr0, 1) * p2;
      }
      /* lcm, guarded: a product past a few thousand already says "unusable",
	 so saturate rather than wrap.  */
      if (!mvelcm_overflow)
	{
	  unsigned HOST_WIDE_INT a = mvelcm, b = (unsigned HOST_WIDE_INT) m_i;
	  while (b)
	    {
	      unsigned HOST_WIDE_INT t = a % b;
	      a = b;
	      b = t;
	    }
	  unsigned HOST_WIDE_INT nl = mvelcm / a * (unsigned HOST_WIDE_INT) m_i;
	  if (nl > 1u << 20)
	    mvelcm_overflow = true;
	  else
	    mvelcm = nl;
	}
    }

  /* Loads the schedule relies on being promoted to .s/.us.  */
  auto_bitmap bodydef;
  for (unsigned i = 0; i < seq.length (); i++)
    {
      df_ref def;
      FOR_EACH_INSN_DEF (def, seq[i])
	bitmap_set_bit (bodydef, DF_REF_REGNO (def));
    }
  int spec = 0;
  for (unsigned i = 0; i < seq.length (); i++)
    if (lvx_lms_speculated_load_p (seq[i], bodydef))
      spec++;

  if (lvx_lms_force_spec_loads && spec)
    spec = -lvx_lms_force_speculative_loads (seq, bodydef);

  /* Stage scheduling, if asked: shift operations by whole periods to shorten
     live ranges.  Done here, after the II search has converged and before
     anything measures the result, because it cannot change the II or the
     resource feasibility -- only the stages.  §7octodecies.2.  */
  int staged = 0;
  if (lvx_lms_stage_sched)
    staged = lvx_lms_stage_schedule (arcs, seq.length (), sigma, found);

  /* REGISTER PRESSURE of the modulo kernel: MaxLive, in registers.
     (Author, 2026-10-27: the name count is not a useful measure; what counts
     is pressure and colourability.)

     In steady state every value recurs every II cycles, so a value defined at
     D with lifetime L has, at the cycle whose phase relative to D is P, exactly

	 ceil ((L - P) / II)      instances live, for P in [0, II)

     which is ceil (L/II) at P = 0 and falls by one each time P crosses a
     multiple of II.  Summing that over the values, weighted by how many
     registers each occupies, and taking the maximum over the II cycles of the
     kernel gives MaxLive -- the figure an allocator must fit into 64 GPRs, and
     the one a splitting copy cannot reduce.

     Note this is NOT the sum of the per-value degrees: a value contributes
     ceil(L/II) only at one phase and less at the others, so summing degrees
     over-counts.  The old mveregs did exactly that.  */
  int mvepress = 0;
  {
    for (int c = 0; c < found; c++)
      {
	int live = 0;
	for (unsigned i = 0; i < seq.length (); i++)
	  {
	    int lastu = -1;
	    for (unsigned a = 0; a < arcs.length (); a++)
	      if (arcs[a].src == i && arcs[a].kind == LVX_ARC_REG_RAW)
		{
		  int when = sigma[arcs[a].dst] + (int) arcs[a].omega * found;
		  if (when > lastu)
		    lastu = when;
		}
	    if (lastu < 0)
	      continue;
	    int l = lastu - sigma[i];
	    if (l <= 0)
	      continue;
	    int ph = ((c - (sigma[i] - lo)) % found + found) % found;
	    if (l <= ph)
	      continue;
	    int inst = (l - ph + found - 1) / found;
	    rtx ds = single_set (seq[i]);
	    int nr = (ds && REG_P (SET_DEST (ds))) ? REG_NREGS (SET_DEST (ds)) : 1;
	    live += MAX (nr, 1) * inst;
	  }
	if (live > mvepress)
	  mvepress = live;
      }
  }

  /* Can a splitting copy actually be placed?  §7undecies.4 projects a large
     reduction in the expansion degree from splitting the longest lifetime with
     one copy, and that projection is unconditional -- which the author objected
     to (2026-10-22): a copy reading the value cannot issue before the
     producer's result exists, so the split point is confined to

	 [ sigma (def) + latency (def),  last_use - latency (copy) ]

     and if that window is empty there is no split, while if it is narrow the
     split cannot be balanced and buys far less than half.  On an exposed-latency
     VLIW this is a correctness bound rather than a stall, so it is strict.
     Llosa & Freudenberger state the CONSEQUENCE -- Kmin is floored by the
     longest operation latency -- but never the window, and never measure it.

     MVESPLIT applies the window; MVESPLITR additionally requires a free issue
     slot in the copy's modulo cycle, which is the whole premise of doing this
     in spare resources.  MVENOSPL counts values whose window is empty.  */
  int mvesplit0 = 1, mvesplit = 1, mvesplitr = 1, mvesplitx = 1, mvenospl = 0;
  int mvewide = 0, mvewidemax = 1, mvecatdq = 0, mveregsx = 0;
  {
    /* Modulo-cycle occupancy of the TINY counter, which is what a register
       move takes: availability 4, covering ALU0/ALU1/LSU0/LSU1.  */
    auto_vec<int> tinyuse, liteuse;
    for (int c = 0; c < found; c++)
      {
	tinyuse.safe_push (0);
	liteuse.safe_push (0);
      }
    for (unsigned i = 0; i < seq.length (); i++)
      {
	struct lvx_sched_resources one;
	memset (&one, 0, sizeof (one));
	lvx_lms_resources_add (&one, seq[i]);
	tinyuse[(sigma[i] - lo) % found] += (int) one.tiny_count;
	liteuse[(sigma[i] - lo) % found] += (int) one.lite_count;
      }
    int tinycap = (int) lvx_issue_availability.tiny;

    for (unsigned v = 0; v < sp_deg.length (); v++)
      {
	int d = sp_def[v], lat = sp_lat[v], last = sp_last[v];
	int deg = sp_deg[v];
	/* A move costs one cycle before its reader.  */
	int tlo = d + MAX (1, lat), thi = last - 1;
	int best = deg, bestr = deg, best0 = deg;

	/* MVESPLIT0 is the same minimisation with the window ignored -- the
	   unconstrained projection, and the only honest baseline for what the
	   window costs.  Splitting every expanded value once in both, so the
	   two differ in exactly the constraint under test.  */
	for (int t = d; t <= last; t++)
	  {
	    int a = (t - d + found - 1) / found;
	    int b = (last - t + found - 1) / found;
	    int cand = MAX (MAX (a, b), 1);
	    if (cand < best0)
	      best0 = cand;
	  }

	if (tlo > thi)
	  mvenospl++;
	else
	  for (int t = tlo; t <= thi; t++)
	    {
	      int a = (t - d + found - 1) / found;
	      int b = (last - t + found - 1) / found;
	      int cand = MAX (MAX (a, b), 1);
	      if (cand < best)
		best = cand;
	      if (cand < bestr && tinyuse[(t - lo) % found] < tinycap)
		bestr = cand;
	    }
	if (best0 > mvesplit0)
	  mvesplit0 = best0;
	if (best > mvesplit)
	  mvesplit = best;
	if (bestr > mvesplitr)
	  mvesplitr = bestr;
      }

    /* MVESPLITX, the exact figure.  The three above test each value against the
       ORIGINAL occupancy, so each is told a slot is free that an earlier copy
       may already have taken -- optimistic by construction, and a loop needs
       2.09 copies on average.  This places them for real: largest degree first,
       since that is the value setting the loop's degree, and each placement
       reserves its TINY in a table the next placement sees.

       The window is the same one, and it is where "the producer latency must
       have elapsed" is enforced:

	   t >= sigma (def) + latency (def)        the value exists to be copied
	   t <= last_use - 1                       the copy lands before its reader

       LAT comes from lvx_lms_latency, the producer's own processing time.  That
       is the right number here rather than lvx_arc_latency's bypassed one,
       and the reason is worth recording: the bypasses in scheduling.md shorten
       a producer's latency only into a STORE's data operand or an imadd
       accumulator (§7sedecies.4b), and a register copy is neither -- so
       lvx_arc_latency (prod, copy, REG_DEP_TRUE) would return
       insn_default_latency unchanged.  A copy is the one consumer that gets no
       discount, which is exactly why the window binds at all.  */
    auto_vec<int> order;
    for (unsigned v = 0; v < sp_deg.length (); v++)
      order.safe_push ((int) v);
    for (unsigned a = 0; a + 1 < order.length (); a++)
      for (unsigned b = a + 1; b < order.length (); b++)
	if (sp_deg[order[b]] > sp_deg[order[a]])
	  std::swap (order[a], order[b]);

    auto_vec<int> use, luse;
    for (int c = 0; c < found; c++)
      {
	use.safe_push (tinyuse[c]);
	luse.safe_push (liteuse[c]);
      }
    int litecap = (int) lvx_issue_availability.lite;

    for (unsigned oi = 0; oi < order.length (); oi++)
      {
	int v = order[oi];
	int d = sp_def[v], lat = sp_lat[v], last = sp_last[v];
	int deg = sp_deg[v];
	int nr = sp_nregs[v];
	if (nr > 1)
	  {
	    mvewide++;
	    if (nr > mvewidemax)
	      mvewidemax = nr;
	  }
	int tlo = d + MAX (1, lat), thi = last - 1;
	int best = deg, bestt = -1, bestt_t = nr, bestt_l = 0;
	for (int t = tlo; t <= thi; t++)
	  {
	    int slot = (t - lo) % found;

	    /* The copies must all land before the first use of the copy --
	       taken here as the same cycle, which is also the only way the new
	       name is complete at a single date.  Two routes for a value wider
	       than one register, and they spend different counters
	       (lvx-issue-table.inc: a LITE instruction reserves a TINY too):

		 COPYD  one per 64-bit register   NR tiny
		 CATDQ  one per 128 bits          NR/2 lite + NR/2 tiny

	       CATDQ halves the TINY cost of a pair but spends the scarcer
	       counter -- availability 2 against 4 -- and a quad's two CATDQ
	       saturate LITE outright.  With -mlms-split-wide=1 take CATDQ
	       where it fits and fall back to COPYD, which is the better way
	       round: TINY is the binding counter on this corpus (§7septies.3's
	       tight= field names it in 46 of 51 missing loops).  */
	    int need_t = nr, need_l = 0;
	    if (lvx_lms_split_wide == 1 && nr >= 2)
	      {
		int cat = nr / 2;
		if (luse[slot] + cat <= litecap && use[slot] + cat <= tinycap)
		  {
		    need_t = cat;
		    need_l = cat;
		  }
	      }
	    if (use[slot] + need_t > tinycap
		|| luse[slot] + need_l > litecap)
	      continue;
	    int aa = (t - d + found - 1) / found;
	    int bb = (last - t + found - 1) / found;
	    int cand = MAX (MAX (aa, bb), 1);
	    if (cand < best)
	      {
		best = cand;
		bestt = t;
		bestt_t = need_t;
		bestt_l = need_l;
	      }
	  }
	if (bestt >= 0)
	  {
	    use[(bestt - lo) % found] += bestt_t;
	    luse[(bestt - lo) % found] += bestt_l;
	    if (bestt_l > 0)
	      mvecatdq++;
	  }
	if (best > mvesplitx)
	  mvesplitx = best;

	/* The register cost AFTER splitting, and the arithmetic is not the
	   same as the degree's (author, 2026-10-27).  MVESPLITX is the larger
	   of the two halves, because the KERNEL UNROLL FACTOR is a maximum
	   over values -- that is a code-size quantity.  The NAME COUNT is not:
	   both halves need names, so it is the SUM.

	   Splitting therefore *raises* the name count.  Lifetime 10 at II 4
	   needs ceil (10/4) = 3 names; split in half it needs
	   ceil (5/4) + ceil (5/4) = 4.  A copy cannot reduce register
	   pressure -- at the copy the source and the destination are both
	   live -- and it cannot reduce the number of names either.  What it
	   buys is a smaller unroll factor and a more colourable interference
	   graph at the same pressure, which is Llosa's claim and Stotzer's.
	   An earlier version of this charged only MVESPLITX here and so
	   reported a reduction that cannot exist.  */
	{
	  int h1 = bestt >= 0 ? (bestt - d + found - 1) / found : deg;
	  int h2 = bestt >= 0 ? (last - bestt + found - 1) / found : 0;
	  int p1 = 1, p2b = 1;
	  while (p1 < MAX (h1, 1))
	    p1 <<= 1;
	  if (h2 > 0)
	    while (p2b < h2)
	      p2b <<= 1;
	  else
	    p2b = 0;
	  mveregsx += nr * (p1 + p2b);
	}
      }
  }

  int ignodes = 0, igedges = 0, igmaxsq = 0, igspill = 0, igperiod = 0;
  int igkunroll = 1, igec = 0;
  for (unsigned v = 0; v < sp_ec.length (); v++)
    igec += sp_ec[v];
  /* The report asks for the ASSIGNMENT, not only the feasibility answer
     (§7ter.13).  IGMAXREG is the highest register it needed and IGASSIGNED how
     many of the IGNODES names got one -- the two numbers that say whether the
     kernel can actually be renamed, as against whether a colouring exists.
     The II search's own call (inside SWO acceptance) deliberately still asks
     only for feasibility: it is deciding whether to accept a schedule, and
     the assignment is codegen's to take once one is accepted.  */
  /* THE SCHEDULE ITSELF, one line per loop.  Everything else here is an
     aggregate, and an aggregate cannot be checked against the kernel it
     describes.  Each entry is `index:cycle/stage', where cycle is
     (sigma - lo) mod II -- the kernel position the §7sedecies.1 annotation
     writes -- and stage is (sigma - lo) / II, which is the iteration's data
     and so what modulo expansion names.  §7sedecies.2 keeps those two apart
     and this is where they can be read apart.  */
  if (!seq.is_empty ())
    {
      lvx_lms_dump (";; lvx cms: fn=%s loop %d schedule II=%d:",
		    current_function_name (), loop->num, found);
      for (unsigned i = 0; i < seq.length (); i++)
	lvx_lms_dump (" %u:%d/%d%s", i, (sigma[i] - lo) % found,
		      (sigma[i] - lo) / found, JUMP_P (seq[i]) ? "J" : "");
      lvx_lms_dump ("\n");
    }

  int igmaxreg = -1, igassigned = 0, igpinned = 0, igpinfail = 0;
  /* HOISTED OUT OF THE BLOCK THEY WERE COMPUTED IN, because codegen needs the
     assignment and not just its summary: the rename map is a function of
     (AV, AN, AR) and of the value table, and a count of how many nodes got a
     register says nothing about which.  §7ter.14.  */
  auto_vec<int> q_def, q_last, q_deg, q_nr, q_ec, q_own, q_regno, q_uf;
  auto_vec<int> av, an, ar;
  {
    /* Re-collect with the original regno and the USE_FIRST flag, which the
       assignment needs and the feasibility test never did (§7ter.14).  */
    lvx_lms_collect_values (seq, arcs, sigma, found, q_def, q_last, q_deg,
			    q_nr, q_ec, &q_own, &q_regno, &q_uf);
    /* AVAIL IS THE ALLOCATABLE SET MINUS WHAT THE LOOP ALREADY HOLDS, not all
       ones.  Passing all ones is what §7ter.10 calls the OPTIMISTIC
       colouring, and for a feasibility answer inside the II search that is
       the intended reading -- a failure means something.  For an ASSIGNMENT
       it is not a reading, it is wrong: the colouring hands back registers
       the body is using and the renaming would destroy them.  The II search's
       own call deliberately still passes all ones, so what the search accepts
       and what can be assigned are now two different questions, and the gap
       between them is a figure to measure rather than a thing to assume
       away.  */
    HARD_REG_SET resv;
    lvx_lms_reserved_regs (seq, q_def, q_last, q_nr, q_ec, q_regno, q_uf,
			   df_get_live_in (loop->header), &resv);
    unsigned HOST_WIDE_INT avail = 0;
    for (int r = 0; r < 64 && r < FIRST_PSEUDO_REGISTER; r++)
      if (!TEST_HARD_REG_BIT (resv, r))
	avail |= (unsigned HOST_WIDE_INT) 1 << r;
    lvx_lms_colour (q_def, q_last, q_deg, q_nr, q_ec, found, lo,
		    &ignodes, &igedges, &igmaxsq, &igspill, &igperiod,
		    &igkunroll, NULL, &av, &an, &ar, &igmaxreg,
		    avail, &q_regno, &q_uf,
		    &igpinned, &igpinfail);
    for (unsigned q = 0; q < ar.length (); q++)
      if (ar[q] >= 0)
	igassigned++;
  }

  /* THE RENAME MAP, verified against the schedule rather than against the
     graph that produced the assignment.  Reported for every scheduled loop,
     whether or not codegen runs, because the question "could this loop's
     registers be renamed at all" is answerable on the whole population and is
     the precondition make_insns has no way around.  */
  {
    int rc = 0, ru = 0, ra_ = 0, rn = 0, rm = 0, rd = 0, rr = 0, rx = 0;
    bool rok = lvx_lms_verify_rename (seq, arcs, q_def, q_last, q_nr, q_ec,
				      q_own, q_regno, q_uf,
				      df_get_live_in (loop->header),
				      av, an, ar, found,
				      lo, igkunroll > 0 ? igkunroll : 1,
				      &rc, &ru, &ra_, &rn, &rm, &rd, &rr, &rx);
    lvx_lms_dump (";; lvx cms: fn=%s loop %d rename %s anodes=%u assigned=%d"
		  " values=%u conflicts=%d unseen=%d ambig=%d unassigned=%d"
		  " misaligned=%d nondiv=%d reserved=%d clash=%d\n",
		  current_function_name (), loop->num, rok ? "ok" : "BAD",
		  ar.length (), igassigned, q_def.length (), rc, ru, ra_, rn,
		  rm, rd, rr, rx);
    if (lvx_lms_probe && !q_def.is_empty ())
      {
	/* The map itself, one entry per value: the defining instruction, the
	   register RA gave it, its width, and the registers its expansion
	   copies were given in name order.  An aggregate cannot be checked
	   against the kernel it describes (§7sedecies.1), and this is what
	   the per-insn schedule line above has to be read against.  */
	lvx_lms_dump (";; lvx cms: fn=%s loop %d rename map:",
		      current_function_name (), loop->num);
	for (unsigned v = 0; v < q_def.length (); v++)
	  {
	    int p2 = lvx_lms_value_p2 (av, (int) v);
	    lvx_lms_dump (" %u@%d:r%dx%d%s", v, q_own[v], q_regno[v], q_nr[v],
			  q_uf[v] ? "F" : "");
	    for (int j = 0; j < p2; j++)
	      {
		int q = lvx_lms_anode_of (av, an, (int) v, j);
		lvx_lms_dump ("%sr%d", j ? "," : "->",
			      q < 0 ? -1 : ar[q]);
	      }
	    if (p2 == 0)
	      lvx_lms_dump ("->=");
	  }
	lvx_lms_dump ("\n");
      }
  }

  /* What codegen would emit, computed as data (§7quinquies.8).  AFTER the
     colouring, because KUNROLL is its output and the plan is a function of it:
     the master schedule runs over nstages + kunroll instances and ITER_SLOT
     sits at (kunroll - 1) * II, so a wrong kunroll is a wrong plan rather than
     a slightly-off one.  Called eleven lines too early at first, which put
     kunroll at its initial 1 and understated every figure -- caught by hand
     arithmetic on one loop's schedule, not by anything in the dump.  */
  struct lvx_lms_plan pl;
  auto_vec<lvx_lms_slot> plslots;
  auto_vec<lvx_lms_node> plnodes;
  /* THE PLAN'S KUNROLL MUST BE THE RENAMING'S.  §7tricies.47.

     This was IGKUNROLL, from the VALUE colouring, and that was right while
     codegen renamed from the value/class assignment.  It renames from the
     LIVE RANGE assignment now, whose kunroll is its own, and when the two differ
     the kernel is built with one number of copies while the names rotate with
     another: the copy that should write name 1 writes name 0, so a register
     the loop depends on is written only in the prologue.

     On `for (k = 0; k < 16; k++) acc = acc * 3 + A[k]' the plan built a
     one-copy kernel (PLKER = 5 for a five-instruction body) while the
     live ranges asked for two, and the emitted kernel held neither the load nor
     the pointer increment of its second copy -- so the exit test compared a
     register nothing updated and the loop did not terminate.  Caught by
     validation/cms/twoexit.c, whose main happens to contain that loop, and by
     nothing in the four older gate programs.

     So the live range colouring runs FIRST, and only its kunroll reaches the
     plan.  The codegen block below colours again from the same inputs and
     checks the two agree, so a future divergence is refused rather than
     emitted.  */
  /* AND THE RETRY IS SETTLED HERE TOO, not in the codegen block.
     §7tricies.51.

     The claim_def_cycle recolour (§7tricies.35) exists because a cycle's
     reads and writes may not be linearisable, and it changes the interval
     model -- so it can change KUNROLL.  Deciding it after the plan was built
     left the two disagreeing on two loops, which the check below then had to
     refuse.

     The decision needs LVX_LMS_SLOT_ORDER, which needs slots, which need a
     plan: so the negotiation builds a THROWAWAY plan at each candidate
     kunroll and asks the order of it.  A plan is a walk over the schedule and
     costs nothing to discard, which is what makes trying rather than
     predicting affordable.  What comes out is the kunroll AND the claim flag
     the emission will use, so the codegen block no longer decides the retry
     -- it is told.  */
  int plkun = 1;
  int plclaimed = 0;
  {
    auto_vec<lvx_lms_lrange_part *> klr;
    HARD_REG_SET kresv;
    unsigned HOST_WIDE_INT kavail = 0;
    int klb = -1;

    for (unsigned z = 0; z < seq.length (); z++)
      if (JUMP_P (seq[z]) && BLOCK_FOR_INSN (seq[z]) == loop->latch)
	{
	  edge e;
	  edge_iterator ei;

	  FOR_EACH_EDGE (e, ei, BLOCK_FOR_INSN (seq[z])->succs)
	    if (e->dest == loop->header)
	      klb = (int) z;
	}

    lvx_lms_build_lranges (seq, klr, NULL, NULL, NULL, NULL, NULL,
			   &prop);
    /* No LIVEOUT here: it sets EXIT_LIVE only, which the epilogue copy wants
       and the unroll factor does not, and the bitmap is not in scope yet.  */
    lvx_lms_set_lrange_dates (seq, klr, sigma, found, NULL);
    lvx_lms_lrange_reserved (seq, klr, df_get_live_in (loop->header), &kresv);
    for (int r = 0; r < 64 && r < FIRST_PSEUDO_REGISTER; r++)
      if (!TEST_HARD_REG_BIT (kresv, r))
	kavail |= (unsigned HOST_WIDE_INT) 1 << r;

    plkun = igkunroll > 0 ? igkunroll : 1;
    /* THREE MODELS, AND THE TEST IS WHETHER THE RESULT CAN BE EMITTED.
       §7tricies.120.

       The order matters, and it is NOT least-to-most conservative, which is
       what this comment claimed until §7tricies.210: attempt 1 claims for
       EVERY range and attempt 2 withholds the claim from tie-carrying ranges,
       as the `ec' expression in lvx_lms_colour_lranges says and as the
       comment beside it has always said.  So the order is no claim, then the
       MOST conservative model, then the middle one -- which is still
       monotone in what it accepts, because 2 is tried only when 1 has been
       refused.
       Inserting the middle one is strictly additive -- the two it sits
       between are unchanged, so a loop that settled before can only settle
       the same way or earlier.

       And the acceptance test now asks LVX_LMS_RENAME_LR_DRYRUN as well as
       LVX_LMS_SLOT_ORDER.  Asking only whether the model linearises is what
       let the negotiation choose a model whose renaming the target then
       refused: five loops linearised under the full claim, were accepted on
       that basis, and lost their whole pipeline at emission when `recog'
       rejected an FMA whose tie the claim had split.  A model that cannot be
       emitted is not a model, and the plan it needs is a throwaway walk --
       which is what makes asking affordable.  */
    for (int attempt = (int) lvx_lms_claim_floor; attempt < 3; attempt++)
      {
	int claim = attempt;
	int knn = 0, kst = 0, kmx = -1, kkun = 1;
	int kpin = 0, kpinf = 0, kwide = 0;

	if (!lvx_lms_colour_lranges (seq, klr, found, lo, kavail, &knn, &kst,
				     &kmx, &kkun, &kpin, &kpinf, &kwide,
				     claim))
	  continue;
	{
	  struct lvx_lms_plan tp;
	  auto_vec<lvx_lms_slot> tslots;
	  auto_vec<lvx_lms_node> tnodes;
	  auto_vec<int> torder;
	  int tsubst = 0, trefused = 0;

	  lvx_lms_build_plan (seq, sigma, found, kkun > 0 ? kkun : 1, nstages,
			      loop->num_nodes, &tp, &tslots, &tnodes);
	  bool torw = false;
	  bool tlin = lvx_lms_slot_order (seq, tslots, klr, klb, torder,
					  &torw);
	  /* THE TWO TESTS SIDE BY SIDE.  §7tricies.182.  CYCLIC is what
	     decides today; PAIR is the direct allocation condition.  Reported
	     per attempt so the two sets can be compared loop by loop before
	     either replaces the other.  */
	  if (dump_file && lvx_lms_probe)
	    lvx_lms_dump (";; lvx cms: CLAIMTEST fn=%s loop %d claim=%d"
			  " kunroll=%d cyclic=%d pair=%d\n",
			  current_function_name (), loop->num, claim,
			  kkun > 0 ? kkun : 1, tlin ? 0 : 1, torw ? 1 : 0);
	  if (tlin
	      && lvx_lms_rename_lr_dryrun (seq, tslots, klr, &tsubst,
					   &trefused))
	    {
	      plkun = kkun > 0 ? kkun : 1;
	      plclaimed = claim;
	      break;
	    }
	  /* Not usable at this kunroll: fall through to the next model, and
	     if every one fails keep the last kunroll so the plan is still
	     built from the assignment the emission will check.  */
	  plkun = kkun > 0 ? kkun : 1;
	  plclaimed = claim;
	}
      }
    for (unsigned z = 0; z < klr.length (); z++)
      delete klr[z];
  }
  lvx_lms_build_plan (seq, sigma, found, plkun,
		      nstages, loop->num_nodes, &pl, &plslots, &plnodes);

  /* THE LOCAL SCHEDULE, ONE BODY INSTANCE WITH ITS CYCLES.  §7tricies.146.
     SIGMA is the schedule of a single iteration, so this is the thing the
     recurrence and the resource bounds are both statements about, and the
     thing a reader needs to see to judge either.  Printed in cycle order
     rather than program order, since that is what a VLIW schedule means.  */
  if (dump_file && lvx_lms_probe)
    {
      int last = 0;

      for (unsigned i = 0; i < seq.length (); i++)
	last = MAX (last, sigma[i]);
      for (int c = 0; c <= last; c++)
	for (unsigned i = 0; i < seq.length (); i++)
	  if (sigma[i] == c)
	    {
	      int code = INSN_CODE (seq[i]);

	      lvx_lms_dump (";; lvx cms:   LOCALSCHED fn=%s loop %d cycle=%d"
			    " i=%u uid=%d %s\n", current_function_name (),
			    loop->num, c, i, INSN_UID (seq[i]),
			    code >= 0 ? insn_data[code].name : "?");
	      if (dump_file)
		print_rtl_single (dump_file, seq[i]);
	    }
    }

  /* THE MASTER SCHEDULE'S DATAFLOW, checked on the recorded slots before any
     of it is emitted.  The plan counts and the rename map colours; neither
     answers whether an instance reads a value some earlier instance on the
     same path wrote, and a pipeline can be correctly counted, correctly
     coloured and still deliver an undefined operand -- a wrong answer rather
     than a crash.  §7quinquies.9.  */
  int slundef = 0, slorder = 0, slpaths = 0;
  bool slok = lvx_lms_verify_slots (seq, arcs, plslots, plnodes,
				    &slundef, &slorder, &slpaths);
  /* §7tricies.136.  The slot table itself, so an abutment in the circular-arc
     model can be checked against the BLOCK each of its two slots lands in:
     the read-before-write exemption that lets two arcs abut is only valid
     inside one bundle, and a cycle's slots are distributed over SKELETON
     BLOCKS -- see the definition at lvx_lms_make_nodes.  */
  if (dump_file && lvx_lms_probe)
    for (unsigned z = 0; z < plslots.length (); z++)
      {
	int nd = plslots[z].node;
	lvx_lms_dump (";; lvx cms:   SLOT fn=%s loop %d z=%u insn=%d iter=%d"
		      " date=%d node=%d kind=%d\n", current_function_name (),
		      loop->num, z, plslots[z].insn, plslots[z].iter,
		      plslots[z].date,
		      nd, (nd >= 0 && nd < (int) plnodes.length ()
			   ? (int) plnodes[nd].kind : -1));
      }

  lvx_lms_dump (";; lvx cms: fn=%s loop %d slots %s n=%u nodes=%u paths=%d"
		" undef=%d order=%d pro=%d ker=%d epi=%d episym=%d"
		" exitslots=%d exep=%d steadybad=%d\n", current_function_name (),
		loop->num, slok ? "ok" : "BAD", plslots.length (),
		plnodes.length (), slpaths, slundef, slorder, pl.prologue,
		pl.kernel, pl.epilogue, pl.epilogue_sym, pl.exit_slots,
		pl.exit_epilogues, pl.steady_bad);

  /* THE RENAME UNIT IS THE CLASS, NOT THE VALUE.  §7tricies.11.

     A tied operand makes two values one register, so they cannot be given
     different expansion names.  The classes are found by asking the target
     (lvx_lms_tie_classes), then COLOURED AS VALUES IN THEIR OWN RIGHT: one
     entry per class with the union of its members' ranges, which is the whole
     cost of the merge -- a longer range means a higher degree and possibly an
     extra name.

     The value-level colouring above is kept and still reports every figure
     the study is built on, so none of its numbers move.  This second
     colouring exists only to be the one codegen renames from.  */
  int rnsubst = 0, rnfail = 0;
  bool rnok = false;
  int tnclass = 0, tnmerged = 0, tbiggest = 0, torphan = 0;
  /* The registers live on an edge LEAVING the loop.  A class holding one of
     them cannot be renamed, because nothing rewrites the reader.  */
  int cgliveout_renamed = 0, cgambig_classes = 0;
  bitmap liveout = BITMAP_ALLOC (NULL);
  {
    edge e;
    edge_iterator ei;
    for (unsigned b = 0; b < loop->num_nodes; b++)
      FOR_EACH_EDGE (e, ei, body[b]->succs)
	if (!flow_bb_inside_loop_p (loop, e->dest)
	    && e->dest != EXIT_BLOCK_PTR_FOR_FN (cfun))
	  bitmap_ior_into (liveout, df_get_live_in (e->dest));
  }

  int cmaxreg = -1, cassigned = 0, cnodes = 0, cedges = 0, ckunroll = 1;
  int cgpasses = 0, cgpinned = 0;
  HARD_REG_SET cgsurv;
  auto_vec<int> cgperiod, cglife, cgnames;
  bool cgocc = false;
  int cgconf = 0, cgunseen = 0, cgambig = 0, cgunass = 0, cgmisal = 0;
  int cgnondiv = 0, cgresv2 = 0, cgclash2 = 0;
  CLEAR_HARD_REG_SET (cgsurv);
  /* The class assignment outlives the block that computes it: codegen renames
     from it, so it cannot be scoped to the measurement.  */
  auto_vec<int> cgi2c, cgcregno, cgav, cgan, cgar;
  {
    auto_vec<int> tparent;
    lvx_lms_tie_classes (seq, arcs, q_own, q_regno, av, an, ar, tparent,
			 &tnclass, &tnmerged, &tbiggest, &torphan);

    /* Dense class indices, and the per-class table: the earliest definition,
       the latest reader, the widest member, and the register they share --
       which they do share, because the tie is what put them in one class and
       RA satisfied it.  */
    auto_vec<int> v2c, c_def, c_last, c_deg, c_nr, c_ec, c_regno, c_uf;
    v2c.safe_grow_cleared (q_own.length ());
    for (unsigned v = 0; v < q_own.length (); v++)
      v2c[v] = -1;
    for (unsigned v = 0; v < q_own.length (); v++)
      {
	int r = lvx_lms_tie_find (tparent, (int) v);
	if (v2c[r] < 0)
	  {
	    v2c[r] = (int) c_def.length ();
	    c_def.safe_push (q_def[r]);
	    c_last.safe_push (q_last[r]);
	    c_nr.safe_push (q_nr[r]);
	    c_ec.safe_push (q_ec[r]);
	    c_regno.safe_push (q_regno[r]);
	    c_uf.safe_push (q_uf[r]);
	  }
	int ci = v2c[r];
	if (q_def[v] < c_def[ci])
	  {
	    c_def[ci] = q_def[v];
	    c_ec[ci] = q_ec[v];
	    c_regno[ci] = q_regno[v];
	  }
	if (q_last[v] > c_last[ci])
	  c_last[ci] = q_last[v];
	if (q_nr[v] > c_nr[ci])
	  c_nr[ci] = q_nr[v];
	if (q_uf[v])
	  c_uf[ci] = 1;
      }
    for (unsigned v = 0; v < q_own.length (); v++)
      v2c[v] = v2c[lvx_lms_tie_find (tparent, (int) v)];

    /* Insn -> class, which is all the substitution needs: the defining insn
       of every member maps to the class, so a use found through an arc and a
       def found through ownership agree by construction.  Built here, before
       the degrees are adjusted, because the cross-iteration-tie test below
       needs it.  */
    auto_vec<int> insn2cls_pre;
    insn2cls_pre.safe_grow_cleared (seq.length ());
    for (unsigned i = 0; i < seq.length (); i++)
      insn2cls_pre[i] = -1;
    for (unsigned v = 0; v < q_own.length (); v++)
      if (q_own[v] >= 0 && q_own[v] < (int) seq.length ())
	insn2cls_pre[q_own[v]] = v2c[v];
    for (unsigned c = 0; c < c_def.length (); c++)
      c_deg.safe_push ((c_last[c] - c_def[c] + found - 1) / found);

    /* A CLASS LIVE OUT OF THE LOOP CANNOT BE RENAMED, AND THE LOOP IS
       DECLINED RATHER THAN PINNED.  §7tricies.21.

       The renaming rewrites references inside the loop; the code after it is
       untouched and still reads the register RA chose.  Found on a vectorized
       reduction with two partial accumulators: the map read `4@4:r6x2->r2',
       the kernel accumulated into r2r3, and the code after the loop did
       `fadddp $r6r7 = ..., $r6r7'.  The other accumulator renamed to itself,
       which is why half the sum was right and the answer was 33 of 55.

       PINNING IT WAS TRIED AND REGRESSED pipe.c from a correct answer to a
       wrong one.  A pin is USE_FIRST plus degree 1, and degree 1 is unsound
       when the life exceeds II -- the condition §7tricies.19 derives -- so
       pinning a long-lived live-out class trades one wrong answer for
       another.  The antidep check should then decline the loop, and on pipe.c
       it did not, which is a second question; until it is answered the
       conservative reading is the one kept.

       So a class that is live out AND would be renamed declines the loop.
       The fix that buys those loops back is a copy in the epilogue from the
       final name to the original register, which is real work and is not
       this.  */
    /* TWO CLASSES SHARING AN ORIGINAL REGISTER MAKE THE SUBSTITUTION
       AMBIGUOUS.  §7tricies.22.

       The rewrite finds the operand to change BY REGISTER NUMBER: it walks
       the pattern and replaces every REG whose regno is the class's
       original.  If two classes have the same original -- ordinary RA reuse
       -- an instruction mentioning both has one of them rewritten with the
       other's register.

       AMBIG has reported this at value level since §7tricies.1 and the
       class-level dry run never tested it, which was recorded as the reason
       `correlation' loop 2 was declined and then not acted on.  It is what
       store.c's `init' was failing on: two classes in r3, one an integer byte
       offset and one a double, and the emitted store read
       `sd $r32[$r6] = $r32' -- the double's register as the index.  The ISS
       died on writeBlob(0x41efffffffc20380), which is that double's bit
       pattern.

       Declined rather than repaired.  Repairing it means rewriting by df_ref
       location instead of by regno, so the operand that reads a class is the
       one the arc names; that is the right answer and it is not this.

       NARROWED TO ONE INSTRUCTION AT A TIME.  Declining whenever any two
       classes anywhere in the loop share a register left 9 of 119 loops
       emitting -- RA reuses registers heavily, so almost every loop has a
       pair.  The substitution is per instruction, so what makes it ambiguous
       is one instruction mentioning the same regno for two different
       classes: its own definition and a class it reads, or two classes it
       reads.  Where no instruction mixes them the blanket regno rewrite is
       unambiguous, because the only references it can reach belong to the
       one class.  */
    for (unsigned i = 0; i < seq.length (); i++)
      {
	/* THE INSTRUCTION'S OWN CLASS COUNTS TOO.  Excluding it was tried, on
	   the reading that the substitution already separates def from use --
	   SET_DEST alone for the def, everything else for the uses -- which
	   would make `fixedd.rz r0 = r0' unambiguous and recover most of the
	   population.  store.c failed again immediately.

	   The separation is not sufficient: a READ occurrence can belong to
	   the def's own class, where the instruction reads its own previous
	   value at omega >= 1, and the use pass then rewrites it with the
	   other class's register.  So any two classes an instruction touches,
	   however it touches them, make the regno rewrite ambiguous.  */
	auto_vec<int> touched;
	int own = i < insn2cls_pre.length () ? insn2cls_pre[i] : -1;

	if (own >= 0)
	  touched.safe_push (own);
	for (unsigned a = 0; a < arcs.length (); a++)
	  if (arcs[a].kind == LVX_ARC_REG_RAW && arcs[a].dst == i
	      && arcs[a].src < insn2cls_pre.length ()
	      && insn2cls_pre[arcs[a].src] >= 0
	      && !touched.contains (insn2cls_pre[arcs[a].src]))
	    touched.safe_push (insn2cls_pre[arcs[a].src]);

	for (unsigned a = 0; a < touched.length (); a++)
	  for (unsigned b = a + 1; b < touched.length (); b++)
	    if (c_regno[touched[a]] >= 0
		&& c_regno[touched[a]] == c_regno[touched[b]])
	      cgambig_classes++;
      }

    for (unsigned c = 0; c < c_regno.length (); c++)
      {
	if (c_regno[c] < 0 || c_uf[c])
	  continue;		/* USE_FIRST already keeps the original */
	for (int w2 = 0; w2 < MAX (c_nr[c], 1); w2++)
	  if (liveout && bitmap_bit_p (liveout, c_regno[c] + w2))
	    {
	      cgliveout_renamed++;
	      break;
	    }
      }

    /* AVAIL for the class colouring, from the same reserved set -- which is
       computed over values and stays correct, since a class's register is a
       member's register and the freeable rule counts definitions.  */
    HARD_REG_SET cresv;
    lvx_lms_reserved_regs (seq, q_def, q_last, q_nr, q_ec, q_regno, q_uf,
			   df_get_live_in (loop->header), &cresv);
    unsigned HOST_WIDE_INT cavail = 0;
    for (int r = 0; r < 64 && r < FIRST_PSEUDO_REGISTER; r++)
      if (!TEST_HARD_REG_BIT (cresv, r))
	cavail |= (unsigned HOST_WIDE_INT) 1 << r;

    /* PIN WHAT CANNOT BE RENAMED, BY ASKING RATHER THAN BY REASONING.
       §7tricies.15.

       Merging a tied pair fixes the fma whose addend is its own destination
       (§7tricies.11), and it does not fix every refusal: the consumer may own
       no class at all -- a definition whose only reader is outside the loop
       is dropped from the value table, so there is nothing to merge the
       producer with -- and a destination may be tied to an operand that is no
       RAW producer, where no merge exists either.  Eighteen loops stayed
       unrenameable on that.

       What every such case needs is the same thing: the class must keep the
       register RA gave it.  And that is exactly what a USE_FIRST pin already
       means -- name 0 IS the original register -- so a pinned class is
       spelled as USE_FIRST with degree 1, which leaves it one name, equal to
       the original, and therefore never substituted.  No new mechanism.

       WHICH classes to pin is decided by the dry run, not by a model of the
       constraints.  A refusal names an instruction; its own class and the
       classes it reads are pinned, the colouring is redone, and the dry run
       asked again.  Each pass pins at least one class and a fully pinned
       graph has no substitutions to refuse, so it terminates -- bounded here
       anyway, since a loop that needs many passes is one whose expansion has
       mostly gone and is better declined than ground down.

       Pinning the read classes as well as the defining one is deliberately a
       superset: identifying the single culprit operand would mean probing
       each substitution separately, and the thing being avoided is a second
       model of the target's constraints that can disagree with the first.  */
    auto_vec<int> cav, can, car;
    int cmsq = 0, cspill = 0, cperiod = 0;
    int cpasses = 0, cpinned = 0;
    for (cpasses = 1; cpasses <= 6; cpasses++)
      {
	auto_vec<int> cref;

	cav.truncate (0);
	can.truncate (0);
	car.truncate (0);
	cnodes = cedges = cmsq = cspill = cperiod = 0;
	ckunroll = 1;
	cmaxreg = -1;
	cassigned = 0;
	lvx_lms_colour (c_def, c_last, c_deg, c_nr, c_ec, found, lo,
			&cnodes, &cedges, &cmsq, &cspill, &cperiod, &ckunroll,
			NULL, &cav, &can, &car, &cmaxreg, cavail, &c_regno,
			&c_uf, NULL, NULL);
	for (unsigned q = 0; q < car.length (); q++)
	  if (car[q] >= 0)
	    cassigned++;

	rnok = lvx_lms_rename_dryrun (seq, arcs, plslots, insn2cls_pre,
				      c_regno, cav, can, car, &rnsubst,
				      &rnfail, &cref);
	if (rnok || cref.is_empty ())
	  break;

	bool progress = false;
	for (unsigned k = 0; k < cref.length (); k++)
	  {
	    int i = cref[k];
	    auto_vec<int> want;

	    if (i >= 0 && i < (int) insn2cls_pre.length ()
		&& insn2cls_pre[i] >= 0)
	      want.safe_push (insn2cls_pre[i]);
	    for (unsigned a = 0; a < arcs.length (); a++)
	      if (arcs[a].kind == LVX_ARC_REG_RAW
		  && arcs[a].dst == (unsigned) i
		  && arcs[a].src < insn2cls_pre.length ()
		  && insn2cls_pre[arcs[a].src] >= 0
		  && !want.contains (insn2cls_pre[arcs[a].src]))
		want.safe_push (insn2cls_pre[arcs[a].src]);

	    for (unsigned w = 0; w < want.length (); w++)
	      {
		int c = want[w];
		if (c_uf[c] && c_deg[c] == 1)
		  continue;		/* already pinned */
		c_uf[c] = 1;
		c_deg[c] = 1;
		cpinned++;
		progress = true;
		/* A PINNED CLASS'S REGISTER GOES BACK INTO THE RESERVED SET.
		   AVAIL was computed from the value-level reserved set, where
		   this class's members looked freeable -- their references
		   were going to be rewritten.  Pinning is the decision not to
		   rewrite them, so the register is held after all, and leaving
		   it in AVAIL would let the colouring hand it to another
		   class.  Re-reserving it and recomputing AVAIL inside the
		   loop is why the loop recolours rather than patching the
		   assignment it already has.  */
		if (c_regno[c] >= 0)
		  for (int w2 = 0; w2 < MAX (c_nr[c], 1); w2++)
		    if (c_regno[c] + w2 < FIRST_PSEUDO_REGISTER)
		      {
			SET_HARD_REG_BIT (cresv, c_regno[c] + w2);
			cavail &= ~((unsigned HOST_WIDE_INT) 1
				    << (c_regno[c] + w2));
		      }
	      }
	  }
	if (!progress)
	  break;
      }
    /* The registers the renaming leaves alone: what the body holds and does
       not rewrite, plus every class the loop above pinned -- CRESV is both by
       the time it gets here.  §7tricies.17.  */
    cgsurv = cresv;
    /* Regno -> the number of names its class was given, so the check knows
       how often the register is actually revisited.  */
    /* VERIFY THE CLASS ASSIGNMENT'S OCCUPANCY.  lvx_lms_verify_rename had
       only ever run on the VALUE-level assignment -- the one the study
       reports -- while the CLASS-level assignment is the one codegen renames
       from, and it was never checked at all.

       It was wrong.  On a one-loop reduction the class colouring gave both
       names of a two-name class the same register: the rename map read
       `->r5,r5' and `->r0,r0', and two names exist only when the degree is at
       least two, which means the life exceeds II, which means they interfere.
       Two live values in one register is every symptom at once -- a wrong
       answer on one program, a wild address on another, a hang on a third --
       and all three had been chased as separate control-flow defects.

       One owner insn per class is needed for the ambiguity walk; the first
       member's will do, since the walk only asks which classes a consumer
       reads.  */
    {
      auto_vec<int> c_own;
      for (unsigned c = 0; c < c_regno.length (); c++)
	c_own.safe_push (-1);
      for (unsigned v = 0; v < q_own.length (); v++)
	if (v2c[v] >= 0 && c_own[v2c[v]] < 0)
	  c_own[v2c[v]] = q_own[v];
      cgocc = lvx_lms_verify_rename (seq, arcs, c_def, c_last, c_nr, c_ec,
				     c_own, c_regno, c_uf,
				     df_get_live_in (loop->header),
				     cav, can, car,
				     found, lo, ckunroll > 0 ? ckunroll : 1,
				     &cgconf, &cgunseen, &cgambig, &cgunass,
				     &cgmisal, &cgnondiv, &cgresv2,
				     &cgclash2);
    }

    /* Which registers a class owns -- part two's pair test skips them, since
       part one covers every class by its name count.  And, per class, the
       life the occupancy uses and the names it was actually given.  */
    cgperiod.safe_grow_cleared (FIRST_PSEUDO_REGISTER);
    for (unsigned c = 0; c < c_regno.length (); c++)
      {
	int p2 = lvx_lms_value_p2 (cav, (int) c);
	cglife.safe_push (c_last[c] - c_def[c] + (c_ec[c] ? 1 : 0));
	cgnames.safe_push (p2);
	for (int w2 = 0; w2 < MAX (c_nr[c], 1); w2++)
	  if (c_regno[c] >= 0 && c_regno[c] + w2 < FIRST_PSEUDO_REGISTER)
	    cgperiod[c_regno[c] + w2] = 1;
      }
    cgpasses = cpasses;
    cgpinned = cpinned;
    cgi2c.safe_splice (insn2cls_pre);
    cgcregno.safe_splice (c_regno);
    cgav.safe_splice (cav);
    cgan.safe_splice (can);
    cgar.safe_splice (car);
  }
  lvx_lms_dump (";; lvx cms: fn=%s loop %d renameable %s subst=%d refused=%d"
		" slots=%u values=%u classes=%d merged=%d biggest=%d"
		" orphan=%d cnodes=%d cassigned=%d cmaxreg=%d ckunroll=%d"
		" pinpasses=%d pinned=%d\n",
		current_function_name (), loop->num, rnok ? "ok" : "BAD",
		rnsubst, rnfail, plslots.length (), q_own.length (), tnclass,
		tnmerged, tbiggest, torphan, cnodes, cassigned, cmaxreg,
		ckunroll, cgpasses, cgpinned);

  /* LIVE RANGES WITH EFFECTS, built and checked.  §7tricies.25.  Stage one:
     the structure the renaming is being moved onto, verified against DF
     before anything is renamed from it.  */
  {
    auto_vec<lvx_lms_lrange_part *> lr;
    int nrange = 0, lrorph = 0, lrdup = 0, lrnoloc = 0;
    int lrmaxp2 = 0, lruf = 0, lrxl = 0;
    int lrtie = 0, lrunit = 0, lrheld = 0;

    int nwar = 0, nwaw = 0, nlo = 0, nlotight = 0;
    for (unsigned a = 0; a < arcs.length (); a++)
      {
	if (arcs[a].kind == LVX_ARC_REG_WAR)
	  nwar++;
	if (arcs[a].kind == LVX_ARC_REG_WAW)
	  nwaw++;
	/* IS A CTRL_LIVEOUT ARC EVER BINDING?  §7tricies.68.  The arc keeps a
	   definition of a live-out register out of its branch's cycle, and
	   narrowing it was proposed as an II win -- on no evidence that any
	   of them constrains the schedule at all.  A SLACK arc provably costs
	   nothing; only a tight one can be on a critical path.  Counted, not
	   argued.  */
	if (arcs[a].kind == LVX_ARC_CTRL_LIVEOUT
	    && arcs[a].src < sigma.length () && arcs[a].dst < sigma.length ())
	  {
	    nlo++;
	    if (sigma[arcs[a].dst] - sigma[arcs[a].src]
		== arcs[a].theta - arcs[a].omega * found)
	      nlotight++;
	  }
      }
    /* The same function the graph builder called, on the same body: the
       partition the anti-dependence decisions were made from.  */
    /* §7tricies.110: the same record for every set built here, or two
       of them would describe different programs.  */
    lvx_lms_build_lranges (seq, lr, &lrtie, &lrunit, &lrheld, NULL,
			   NULL, &prop);
    lvx_lms_set_lrange_dates (seq, lr, sigma, found, liveout);
    bool lrok = lvx_lms_verify_lranges (seq, lr, &nrange, &lrorph, &lrdup,
					&lrnoloc);
    int lrovl = lvx_lms_lrange_overlaps (lr);
    for (unsigned k = 0; k < lr.length (); k++)
      if (!lr[k]->effects.is_empty ())
	{
	  if (lr[k]->p2unroll > lrmaxp2)
	    lrmaxp2 = lr[k]->p2unroll;
	  if (lr[k]->use_first)
	    lruf++;
	  if (lr[k]->exit_live)
	    lrxl++;
	  /* TWO NOTIONS OF `LIVES PAST THE LOOP', COMPARED.  §7tricies.134.
	     EXIT_LIVE is set on a range OPEN AT A JUMP whose exit edge reads
	     the register; LIVEOUT is the union of liveness over every edge
	     leaving the loop.  Only EXIT_LIVE earns a reconciliation copy, so
	     a range whose register is in LIVEOUT while EXIT_LIVE is clear is
	     renamed with nothing to copy it back -- the code after the loop
	     then reads an architectural register the pipeline stopped
	     writing.  */
	  if (dump_file && lvx_lms_probe)
	    {
	      bool lo = false;
	      for (int w = 0; w < MAX (lr[k]->nregs, 1); w++)
		if (liveout && bitmap_bit_p (liveout, lr[k]->regno + w))
		  lo = true;
	      if (lo != lr[k]->exit_live)
		lvx_lms_dump (";; lvx cms:   XL-DISAGREE g%u r%u nregs=%d"
			      " exit_live=%d liveout=%d uf=%d p2=%d\n", k,
			      lr[k]->regno, MAX (lr[k]->nregs, 1),
			      lr[k]->exit_live ? 1 : 0, lo ? 1 : 0,
			      lr[k]->use_first ? 1 : 0, lr[k]->p2unroll);
	    }
	}
    lvx_lms_dump (";; lvx cms: fn=%s loop %d lranges %s n=%d orphan=%d"
		  " dup=%d noloc=%d overlap=%d maxp2=%d usefirst=%d"
		  " exitlive=%d tie=%d units=%d held=%d nwar=%d nwaw=%d"
		  " nlo=%d/%d"
		  " adropped=%d akunres=%d akone=%d akkeep=%d narcs=%u\n",
		  current_function_name (), loop->num,
		  lrok && lrovl == 0 ? "ok" : "BAD",
		  nrange, lrorph, lrdup, lrnoloc, lrovl, lrmaxp2, lruf, lrxl,
		  lrtie, lrunit, lrheld, nwar, nwaw, nlotight, nlo,
		  lrdropped[0],
		  lrdropped[1], lrdropped[2], lrdropped[3], arcs.length ());
    /* §7tricies.106: the copy-propagation candidates, each filter's
       rejections apart so the opportunity and the cost of every rule are both
       visible.  Reported here rather than beside the circuit because the
       filter asks about LIVE RANGES, and this is where they exist.  */
    {
      int pcand, ptied, pmdef, pmp, pwide;

      lvx_lms_count_copy_prop (seq, arcs, &pcand, &ptied, &pmdef, &pmp,
			       &pwide);
      lvx_lms_dump (";; lvx cms: fn=%s loop %d cprop=%d tiedop=%d"
		    " otherrange=%d multip=%d wide=%d\n",
		    current_function_name (), loop->num, pcand, ptied, pmdef,
		    pmp, pwide);
    }

    if (lvx_lms_probe && dump_file)
      lvx_lms_dump_lranges (seq, lr, "probe");

    /* STAGE TWO (a): COLOUR THE RANGES, and report the assignment beside the
       class one.  Nothing is renamed from it yet -- the comparison is the
       point, because the class figures are what the restriction was measured
       against.  §7tricies.26.  */
    if (lrok)
      {
	HARD_REG_SET lrresv;
	unsigned HOST_WIDE_INT lravail = 0;
	int lrnn = 0, lrstuck = 0, lrmax = -1, lrkun = 1;
	int lrpin = 0, lrpinf = 0, lrwide = 0;
	bool lrcok;

	lvx_lms_lrange_reserved (seq, lr, df_get_live_in (loop->header),
				 &lrresv);
	for (int r = 0; r < 64 && r < FIRST_PSEUDO_REGISTER; r++)
	  if (!TEST_HARD_REG_BIT (lrresv, r))
	    lravail |= (unsigned HOST_WIDE_INT) 1 << r;

	lrcok = lvx_lms_colour_lranges (seq, lr, found, lo, lravail, &lrnn,
					&lrstuck, &lrmax, &lrkun, &lrpin,
					&lrpinf, &lrwide);
	lvx_lms_dump (";; lvx cms: fn=%s loop %d lrcolour %s nodes=%d"
		      " stuck=%d maxreg=%d kunroll=%d pinned=%d pinfail=%d"
		      " wide=%d avail=%d cmaxreg=%d ckunroll=%d\n",
		      current_function_name (), loop->num,
		      lrcok ? "ok" : "no", lrnn, lrstuck, lrmax, lrkun,
		      lrpin, lrpinf, lrwide,
		      (int) popcount_hwi (lravail), cmaxreg, ckunroll);

	/* STAGE TWO (b): DRY-RUN THE RANGE REWRITE over the same slots the
	   class rewrite is measured on, so the two refusal counts stand side
	   by side.  §7tricies.27.  */
	if (lrcok)
	  {
	    int lrsub = 0, lrfail = 0;
	    bool lrrok = lvx_lms_rename_lr_dryrun (seq, plslots, lr, &lrsub,
						   &lrfail);

	    lvx_lms_dump (";; lvx cms: fn=%s loop %d lrrename %s subst=%d"
			  " refused=%d slots=%u csubst=%d crefused=%d\n",
			  current_function_name (), loop->num,
			  lrrok ? "ok" : "BAD", lrsub, lrfail,
			  plslots.length (), rnsubst, rnfail);
	  }
      }
    for (unsigned k = 0; k < lr.length (); k++)
      delete lr[k];
  }

  /* THE ANTI-DEPENDENCES RENAMING DOES NOT REMOVE.  §7tricies.17.  Asked of
     the schedule and the surviving-register set, with no slot enumeration:
     for a conflicting pair the condition is 0 <= sigma (j) - sigma (i) < II,
     which is the pair of arcs the graph would have carried had it kept
     them.  */
  int adshort = 0, adpair = 0, ada = -1, adb = -1, adc = -1;
  bool adok = lvx_lms_verify_antidep (seq, sigma, found, &cgsurv, cgperiod,
				      cglife, cgnames, &adshort, &adpair,
				      &ada, &adb, &adc);
  lvx_lms_dump (";; lvx cms: fn=%s loop %d antidep %s short=%d pairs=%d"
		" first=%d/%d/%d\n", current_function_name (), loop->num,
		adok ? "ok" : "BAD", adshort, adpair, ada, adb, adc);
  lvx_lms_dump (";; lvx cms: fn=%s loop %d liveoutrenamed=%d"
		" ambigclasses=%d\n", current_function_name (), loop->num,
		cgliveout_renamed, cgambig_classes);
  lvx_lms_dump (";; lvx cms: fn=%s loop %d classocc %s conf=%d unseen=%d"
		" ambig=%d unass=%d misal=%d nondiv=%d clash=%d\n",
		current_function_name (), loop->num, cgocc ? "ok" : "BAD",
		cgconf, cgunseen, cgambig, cgunass, cgmisal, cgnondiv,
		cgclash2);

  /* THE RESOURCE DEMAND, TAKEN BEFORE ANY EMISSION.  §7tricies.103.

     This used to be computed beside its consumer, several hundred lines below
     and AFTER code generation.  By then the caller's BODY array -- filled by
     get_loop_body before this function was entered -- names basic blocks
     emission has replaced, and LOOP->NUM_NODES no longer describes the loop
     either, so the walk dereferenced freed blocks: a segfault in
     `__deregister_frame_info_bases' the moment libgcc was compiled with
     -mlms-codegen, and the reason code generation could not be turned on by
     default.

     It is the third appearance of one hazard -- state captured before
     emission, used after it -- which §VI of the design note says to enforce
     structurally rather than by discipline.  Structurally, here, means
     computing it while the body is still the loop's.  The value is a property
     of the ORIGINAL body, which is also what its consumer wants.  */
  /* And the per-branch exit liveness, for the same reason and at the same
     point: §7tricies.104.  */
  auto_vec<HARD_REG_SET> xlcap;
  lvx_lms_capture_exit_live (seq, xlcap);

  struct lvx_sched_resources dem;
  memset (&dem, 0, sizeof (dem));
  {
    basic_block *dbody = get_loop_body (loop);
    for (unsigned i = 0; i < loop->num_nodes; i++)
      {
	rtx_insn *bi;
	FOR_BB_INSNS (dbody[i], bi)
	  lvx_lms_resources_add (&dem, bi);
      }
    free (dbody);
  }

  /* CODEGEN, phase one: the block structure (§7quinquies.9).  Behind
     -mlms-codegen and off by default, because it is incomplete -- the blocks
     are built empty and no instruction moves, so the loop is unchanged and the
     edge surgery can be verified against a working program before the schedule
     is allowed to touch it.

     AND BECAUSE IT STILL READS PRE-MUTATION STATE AFTER MUTATING.
     §7tricies.104.  Turning it on by default was measured to be worth it --
     CMS 0.9551 on MiBench against 0.9961 for the unfold it replaces, and
     susan_smoothing at 0.4880 -- and then withdrawn, because compiling libgcc
     with it segfaults the compiler in two separate places:

       lvx_lms_pipeline_loop  a resource walk over the caller's BODY array,
			      whose blocks emission had replaced (fixed:
			      §7tricies.103 computes it before emission)
       lvx_lms_build_lranges  df_get_live_in (e->dest) on an exit edge whose
			      destination make_nodes had just created, so the
			      block has no DF info at all -- NULL dereference

     Both are the hazard §VI of mlir-pipeliner-design.md names: state captured
     before emission, used after it.  That note says to enforce the boundary
     STRUCTURALLY rather than by discipline, and these are the third and
     fourth instances of ignoring it.  The second one wants the exit liveness
     threaded in from before make_nodes, not a null check -- a null check
     would silently drop a range's exit_live flag and miscompile instead of
     crashing.  So the default waits for the boundary, not for another patch.  */
  if (lvx_lms_codegen && pl.ok && nstages >= 2)
    {
      /* RECORD, do not transform: the loop structures are finalized before
	 anything is built (§7quinquies.10), so what is captured is raw basic
	 blocks -- a `class loop *' does not survive that, and neither does
	 anything reached through it.  */
      struct lvx_lms_site site;
      memset (&site, 0, sizeof (site));
      site.header = loop->header;
      site.latch = loop->latch;
      site.nstages = nstages;
      site.plan = pl;
      site.renameable = rnok;
      site.antidep_ok = adok;
      site.classocc_ok = cgocc;
      site.loop = loop;
      site.loopdo = loopdo;
      site.kunroll = plkun;
      /* §7tricies.43: all exits from the loopback's own block, so the body
	 runs to completion once per entry.  */
      {
	int lb = -1;
	int nexit = 0, nelse = 0;

	for (unsigned z = 0; z < seq.length (); z++)
	  if (JUMP_P (seq[z]))
	    lb = (int) z;
	if (lb >= 0 && BLOCK_FOR_INSN (seq[lb]))
	  {
	    basic_block lbb = BLOCK_FOR_INSN (seq[lb]);

	    for (unsigned b = 0; b < loop->num_nodes; b++)
	      {
		edge e;
		edge_iterator ei;

		FOR_EACH_EDGE (e, ei, body[b]->succs)
		  if (!flow_bb_inside_loop_p (loop, e->dest))
		    {
		      nexit++;
		      if (body[b] != lbb)
			nelse++;
		    }
	      }
	  }
	/* AND THE JUMP MUST BE THE LATCH'S TERMINATOR.  `all exits leave from
	   the jump's block' is not enough on its own: a single jump in the
	   MIDDLE of the body -- exit or fall through to further body blocks --
	   also satisfies it, and then those later blocks, iteration 0's side
	   effect among them, are conditional on that very test.  Requiring the
	   jump to close the latch puts it at the bottom, so everything before
	   it is unconditional and entering runs all of it.  */
	site.entry_admits = (lb >= 0 && nexit > 0 && nelse == 0
			     && BLOCK_FOR_INSN (seq[lb]) == loop->latch);

	/* A MULTI-JUMP BODY, PRINTED.  The refusal says only that there is
	   more than one jump; what the fix needs is which jump is the
	   loopback, where the others go, and whether their targets are
	   inside the loop.  */
	{
	  int nj = 0;

	  for (unsigned z = 0; z < seq.length (); z++)
	    if (JUMP_P (seq[z]))
	      nj++;
	  if (nj > 1)
	    for (unsigned z = 0; z < seq.length (); z++)
	      {
		basic_block zb = BLOCK_FOR_INSN (seq[z]);
		int code = INSN_CODE (seq[z]);

		lvx_lms_dump (";; lvx cms:   body %u uid=%d bb=%d%s%s %s",
			      z, INSN_UID (seq[z]), zb ? zb->index : -1,
			      zb == loop->header ? " HDR" : "",
			      zb == loop->latch ? " LATCH" : "",
			      code >= 0 ? get_insn_name (code) : "?");
		if (JUMP_P (seq[z]))
		  {
		    edge e;
		    edge_iterator ei;

		    lvx_lms_dump (" JUMP label=%d succs=",
				  JUMP_LABEL (seq[z])
				  ? INSN_UID (JUMP_LABEL (seq[z])) : -1);
		    if (zb)
		      FOR_EACH_EDGE (e, ei, zb->succs)
			lvx_lms_dump ("%d%s ", e->dest->index,
				      flow_bb_inside_loop_p (loop, e->dest)
				      ? "(in)" : "(OUT)");
		  }
		lvx_lms_dump ("\n");
	      }
	}
	lvx_lms_dump (";; lvx cms: fn=%s loop %d entryadmits=%d exits=%d"
		      " notlatch=%d jumpislatch=%d\n",
		      current_function_name (), loop->num,
		      site.entry_admits ? 1 : 0, nexit, nelse,
		      (lb >= 0 && BLOCK_FOR_INSN (seq[lb]) == loop->latch)
		      ? 1 : 0);
      }
      edge e;
      edge_iterator ei;
      {
	/* §7tricies.157: HOW MANY ENTRIES DOES THE HEADER HAVE?
	   lvx_lms_eligible_loop_p never asks, and the loop below takes the
	   LAST non-latch predecessor as the preheader with no uniqueness
	   check -- so a two-entry loop would have one edge split by
	   make_nodes and the other reaching the header with the whole
	   prologue, and the entry copies, bypassed.  Counted before anything
	   relies on it.  */
	unsigned nent = 0;

	FOR_EACH_EDGE (e, ei, loop->header->preds)
	  if (!flow_bb_inside_loop_p (loop, e->src))
	    nent++;
	if (nent != 1)
	  lvx_lms_dump (";; lvx cms: fn=%s loop %d ENTRIES=%u"
			" *** NOT ONE ***\n", current_function_name (),
			loop->num, nent);
	/* §7tricies.162: IS THE BODY A TRACE AT ALL?  A superblock is
	   single-entry and multiple-exit, so a body block with two IN-LOOP
	   predecessors is a side entry and the body is not one.  Counted per
	   loop, with the header excluded because its second predecessor is
	   the back edge by construction.  */
	{
	  unsigned njoin = 0;

	  for (unsigned b = 0; b < loop->num_nodes; b++)
	    {
	      edge e2;
	      edge_iterator ei2;
	      unsigned inpred = 0;

	      if (body[b] == loop->header)
		continue;
	      FOR_EACH_EDGE (e2, ei2, body[b]->preds)
		if (flow_bb_inside_loop_p (loop, e2->src))
		  inpred++;
	      if (inpred > 1)
		njoin++;
	    }
	  lvx_lms_dump (";; lvx cms: fn=%s loop %d JOINS=%u blocks=%u\n",
			current_function_name (), loop->num, njoin,
			(unsigned) loop->num_nodes);
	}
      }
      /* THE PREHEADER IS A PREDECESSOR OUTSIDE THE LOOP, and this used to
	 ask only `e->src != loop->latch'.  §7tricies.197.

	 Those are not the same thing: a header may have an IN-LOOP
	 predecessor that is not the latch -- a join, which the JOINS count
	 just above exists to report -- and then the preheader came out INSIDE
	 the loop, make_nodes split an in-loop edge, and its prologue chain was
	 inserted into the loop body.  The shape that produces is a conditional
	 jump with an edge to its own block, `bb113 C -> bb55f bb22 bb113',
	 which verify_flow_info rejects as "too many outgoing branch edges".
	 There was also no `break', so with several candidates the last one won
	 arbitrarily.

	 Latent in the shipped order and exposed by -mcms-after-hwloop, where
	 reorg_loops' process_incoming_edges has restructured the header's
	 incoming edges to give the LOOPDO a dedicated preheader.  Two entries
	 leave it NULL and the loop is declined, which is what
	 lvx_lms_pipeline_loop's own entry search already does for the same
	 reason.  */
      site.preheader = NULL;
      FOR_EACH_EDGE (e, ei, loop->header->preds)
	if (!flow_bb_inside_loop_p (loop, e->src))
	  {
	    if (site.preheader != NULL)
	      {
		site.preheader = NULL;	/* two entries: no single preheader */
		break;
	      }
	    site.preheader = e->src;
	  }
      /* The drain goes on the LATCH's exit -- the edge the kernel falls out
	 of.  See lvx_lms_make_nodes on why "the loop's single exit" was the
	 wrong criterion.  */
      FOR_EACH_EDGE (e, ei, loop->latch->succs)
	if (!flow_bb_inside_loop_p (loop, e->dest))
	  {
	    site.exit_src = loop->latch;
	    site.exit_dest = e->dest;
	  }
      /* AND WHERE THE LATCH HAS NONE, THE LATEST EXIT IN BODY ORDER.
	 §7tricies.162.

	 49 loops decline here with their whole pipeline already computed --
	 `build_tree' loop 8 reaches II=4, nstages=3 and 86 slots, every
	 check ok, and is then thrown away.  All 49 are one shape: the back
	 edge comes from a latch that only jumps back, so the latch has no
	 out-of-loop successor, while the exit test sits earlier in the body:

	     bb31 HDR  *ccbsi -> {35 in, 32 in}
	     bb32      *ccbsi -> {33 in, 34 in}
	     bb34      *cbdi  -> {35 in, 36 OUT}   <- the only exit
	     bb35 LATCH  adddi3; *goto -> {31 in}  <- no exit edge

	 The criterion itself is right -- it replaced a COUNTING test that
	 rejected every multi-block body, and the drain does need the edge the
	 kernel leaves by.  What is wrong is asking only the latch for it.  So
	 where the latch offers none, take the exit whose source is LATEST in
	 SEQ, the body order the schedule is built in: among the exits that
	 exist, that is the one the kernel reaches last, which is what "falls
	 out" means in a body with no bottom test.

	 SEQ order rather than block order, because the body is a DAG here and
	 not the chain the unfold cost model assumes -- bb31 and bb32 both
	 branch two ways inside the loop -- so there is no layout order to
	 appeal to, while SEQ is exactly what the slots and the plan were
	 built from.  */
      if (!site.exit_dest && lvx_lms_drain_from_exit)
	{
	  for (unsigned z = 0; z < seq.length (); z++)
	    {
	      basic_block zb = BLOCK_FOR_INSN (seq[z]);

	      if (!zb)
		continue;
	      FOR_EACH_EDGE (e, ei, zb->succs)
		if (!flow_bb_inside_loop_p (loop, e->dest))
		  {
		    site.exit_src = zb;
		    site.exit_dest = e->dest;
		  }
	    }
	  if (site.exit_dest)
	    lvx_lms_dump (";; lvx cms: fn=%s loop %d drain from exit bb%d ->"
			  " bb%d (latch bb%d has none)\n",
			  current_function_name (), loop->num,
			  site.exit_src->index, site.exit_dest->index,
			  loop->latch->index);
	}
      if (site.preheader && site.exit_dest)
	{
	  /* BUILD HERE, with the loop structures still live.  The previous
	     attempt deferred this to after loop_optimizer_finalize, on the
	     reading that the ICE came from transforming inside the walk.  That
	     was wrong, and instrumenting both phases is what showed it: the
	     block indices come out IDENTICAL either side of the finalize
	     (pre=3 hdr=4 latch=4 exit=4->5), find_edge on the preheader edge
	     still succeeds, and only the latch's exit edge has gone -- so the
	     CFG is intact and the deferral itself is what lost the edge.

	     The ICE was the FLAG: LOOPS_NORMAL restructures the CFG to give
	     every loop a preheader and a simple latch, and what that leaves is
	     what reorg_loops' cleanup cannot remove.  Two things were changed
	     at once and only one of them was the cause.  AVOID_CFG_MODIFICATIONS
	     is kept, which costs nothing here because none of this uses a loop
	     helper -- create_basic_block, make_edge and redirect_edge_and_branch
	     are raw CFG calls and do not consult the flag.  */
	  /* THE SKELETON IS BUILT LAST, AFTER EVERY CHECK HAS PASSED.
	     §7tricies.145.

	     lvx_lms_make_nodes CREATES BASIC BLOCKS and redirects edges, and
	     it used to run HERE -- before the colouring, the linearisation and
	     the rename dry run, each of which can refuse.  A refusal then left
	     the blocks behind: §7tricies.72's defect exactly, "a refusal that
	     has already mutated the CFG", and the class that returned a wrong
	     answer on the gate's own earlyexit.c when a late refusal left a
	     half-built pipeline.

	     Nothing between creating the skeleton and emitting into it reads
	     it -- the checks want SEQ, PLSLOTS and CGLR, and CGBLOCKS appears
	     only in make_nodes' own dump line and in emit_pipeline's
	     arguments.  So the mutation moves to the last point before
	     emission, and every refusal above it becomes non-mutating by
	     construction rather than by review.

	     WITHOUT -mlms-pipeline it stays where it was: -mlms-codegen alone
	     exists to build the block skeleton and emit nothing (§7tricies.?),
	     so there the skeleton IS the product and there is no later point
	     to defer it to.  */
	  const char *nwhy = NULL;
	  auto_vec<basic_block> cgblocks;
	  bool built = true;

	  if (!lvx_lms_pipeline)
	    {
	      built = lvx_lms_make_nodes (&site, loop, &plnodes, &cgblocks,
					  &nwhy);
	      lvx_lms_dump (";; lvx cms: fn=%s loop %d make_nodes %s blocks=%u"
			    " nodes=%u\n", current_function_name (),
			    loop->num, built ? "built" : nwhy,
			    cgblocks.length (), plnodes.length ());
	    }

	  /* PHASE TWO.  -mlms-pipeline emits the pipeline itself; without it
	     the body is only put into schedule order, which is verified
	     (§7tricies.7).  Two flags rather than one so that a broken
	     pipeline cannot regress the reordering.  */
	  if (built && lvx_lms_pipeline)
	    {
	      const char *iwhy = NULL;
	      int nemit = 0;
	      bool emitted = false;

	      /* THE LIVE RANGE PATH IS WHAT EMITS.  §7tricies.32.  Four of the
		 five gates that stood here were properties of the CLASS
		 model and are gone with it:

		   - `two classes share an original register' (72 of 119
		     loops): an operand belongs to one live range, so there is
		     nothing to disambiguate,
		   - `a class live out of the loop would be renamed' (25):
		     EXIT_LIVE is a field on the live range,
		   - `the class assignment puts two live names in one
		     register': the occupancy is checked over live ranges,
		   - the anti-dependence pin (27), which forced degree 1 and
		     was §7tricies.19's unsoundness.

		 What remains is the substitution actually recognising, which
		 is asked of the live range assignment itself and now refuses
		 nothing on any of the 124 loops measured.  */
	      {
		auto_vec<lvx_lms_lrange_part *> cglr;
		HARD_REG_SET cgresv;
		unsigned HOST_WIDE_INT cgavail = 0, cgentry = 0;
		int nn = 0, nst = 0, mx = -1, kun = 1, pin = 0, pinf = 0;
		int wide = 0, nr = 0, orph = 0, dupl = 0, nol = 0;
		int subst = 0, refused = 0;
		int cgloopback = -1;
		int nconf = 0, ca = -1, cb = -1, creg = -1, ccyc = -1;
		/* §7tricies.92: apart, because they mean different things.  */
		int cnheld = 0, cninv = 0, cnocc = 0;
		int pool = 0;
		bool cok, vok, rok2, aok = true, lok = true, claimed = false;
		/* §7tricies.151: LOSOK is the LIVE-OUT SPLIT's own verdict.  It
		   used to be folded into AOK, so both failures printed "a
		   dropped anti-dependence is not separated" -- two unrelated
		   commitments under one name, which is the shape that made the
		   two 55s of §7tricies.143 look like 110 loops.  */
		bool losok = true;

		for (unsigned z = 0; z < seq.length (); z++)
		  if (JUMP_P (seq[z]))
		    cgloopback = (int) z;

		/* §7tricies.104: the exit liveness from before make_nodes.  */
		lvx_lms_build_lranges (seq, cglr, NULL, NULL, NULL, NULL,
				       &xlcap, &prop);
		lvx_lms_set_lrange_dates (seq, cglr, sigma, found, liveout);
		vok = lvx_lms_verify_lranges (seq, cglr, &nr, &orph, &dupl,
					      &nol);
		lvx_lms_lrange_reserved (seq, cglr,
					 df_get_live_in (loop->header),
					 &cgresv, &pool);
		for (int r = 0; r < 64 && r < FIRST_PSEUDO_REGISTER; r++)
		  if (!TEST_HARD_REG_BIT (cgresv, r))
		    cgavail |= (unsigned HOST_WIDE_INT) 1 << r;
		/* §7tricies.156: what the entry copy may not overwrite.  A
		   register live on entry to the header may still be holding
		   an incoming value the prologue reads, even though AVAIL
		   frees it because the body's writer of it gets renamed.  */
		if (lvx_lms_preheader_copy)
		  for (int r = 0; r < 64 && r < FIRST_PSEUDO_REGISTER; r++)
		    if (bitmap_bit_p (df_get_live_in (loop->header), r))
		      cgentry |= (unsigned HOST_WIDE_INT) 1 << r;
		cok = lvx_lms_colour_lranges (seq, cglr, found, lo, cgavail,
					      &nn, &nst, &mx, &kun, &pin,
					      &pinf, &wide, 0, NULL, cgentry);
		/* §7tricies.158: AND NO ASSIGNED REGISTER MAY LIE OUTSIDE
		   AVAIL.  The colouring is handed AVAIL and picks within it
		   by construction -- `(busy & want) || (~avail & want)' --
		   so this can only fail if a name was filled from somewhere
		   other than the first-fit scan.  Asked because the
		   remaining suspicion for §7tricies.156 is precisely that
		   the assignment hands out a register that is not free, and
		   because the bisect shows the failure needs the wholly
		   unconstrained colouring.  */
		if (dump_file)
		  for (unsigned g = 0; g < cglr.length (); g++)
		    for (int x = 0;
			 x < cglr[g]->p2unroll && x < LVX_LMS_MAX_KUNROLL; x++)
		      {
			int rg = cglr[g]->reg[x];

			if (rg < 0 || cglr[g]->effects.is_empty ()
			    || lvx_lms_lrange_root (cglr, (int) g) != (int) g
			    || !cglr[g]->renamed)
			  continue;
			for (int w = 0; w < MAX (cglr[g]->nregs, 1); w++)
			  if (rg + w >= 64
			      || !((cgavail >> (rg + w)) & 1))
			    lvx_lms_dump (";; lvx cms:   REGNOTAVAIL lr%u"
					  " name%d r%d (+%d) OUTSIDE"
					  " AVAIL\n", g, x, rg, w);
		      }

		/* §7tricies.157: NO ASSIGNED REGISTER MAY BE LIVE ACROSS THE
		   LOOP WITHOUT THE BODY REFERENCING IT.

		   lvx_lms_lrange_reserved is supposed to make this impossible
		   twice over -- a live-in register goes into HARD and HARD is
		   re-applied after the freeing, and the freeing itself needs
		   `nrange[r] > 0' so a register with no range on it is never
		   freed.  Asserted here rather than trusted, because both
		   arguments rest on df_get_live_in being accurate at this
		   point and the whole phase of §7tricies.156 turns on which
		   registers are really free.  */
		if (dump_file)
		  {
		    bitmap li = df_get_live_in (loop->header);
		    HARD_REG_SET refd;

		    CLEAR_HARD_REG_SET (refd);
		    for (unsigned z = 0; z < seq.length (); z++)
		      {
			df_ref rr;
			FOR_EACH_INSN_DEF (rr, seq[z])
			  if (DF_REF_REGNO (rr) < FIRST_PSEUDO_REGISTER)
			    SET_HARD_REG_BIT (refd, DF_REF_REGNO (rr));
			FOR_EACH_INSN_USE (rr, seq[z])
			  if (DF_REF_REGNO (rr) < FIRST_PSEUDO_REGISTER)
			    SET_HARD_REG_BIT (refd, DF_REF_REGNO (rr));
		      }
		    for (unsigned g = 0; g < cglr.length (); g++)
		      for (int x = 0;
			   x < cglr[g]->p2unroll && x < LVX_LMS_MAX_KUNROLL;
			   x++)
			{
			  int rg = cglr[g]->reg[x];

			  /* Only the names that EXIST: reg[] beyond p2unroll
			     is never assigned, and reading it made the first
			     version of this check report 1920 violations in
			     both arms.  */
			  if (rg < 0 || rg >= 64
			      || cglr[g]->effects.is_empty ()
			      || lvx_lms_lrange_root (cglr, (int) g) != (int) g
			      || !cglr[g]->renamed)
			    continue;
			  for (int w = 0; w < MAX (cglr[g]->nregs, 1); w++)
			    {
			      int rr2 = rg + w;

			      if (rr2 >= 64)
				break;
			      if (li && bitmap_bit_p (li, rr2))
				{
				  lvx_lms_dump (";; lvx cms:   REGLIVEIN lr%u"
						" name%d r%d LIVE-IN%s\n", g,
						x, rr2,
						TEST_HARD_REG_BIT (refd, rr2)
						? "" : " AND UNREFERENCED");
				  /* With the pin this is CORRECT and fires 24
				     times on infblock: name 0 of a use-headed
				     range IS the live-in register, which is
				     what the pin says.  With the preheader
				     copy it must never happen -- name 0 is a
				     free register and the copy puts the
				     incoming value there -- and measures 0.
				     So it is a refusal only under the phase,
				     where a hit would mean the colouring had
				     handed out a register that is not free
				     and the entry copy would destroy a live
				     value.  */
				  if (lvx_lms_preheader_copy)
				    cok = false;
				}
			    }
			}
		  }
		lvx_lms_dump (";; lvx cms: fn=%s loop %d ENTRYBUSY avail=%d"
			      " entry=%d first8=%c%c%c%c%c%c%c%c\n",
			      current_function_name (), loop->num,
			      (int) popcount_hwi (cgavail),
			      (int) popcount_hwi (cgentry),
			      (cgentry >> 0) & 1 ? '1' : '0',
			      (cgentry >> 1) & 1 ? '1' : '0',
			      (cgentry >> 2) & 1 ? '1' : '0',
			      (cgentry >> 3) & 1 ? '1' : '0',
			      (cgentry >> 4) & 1 ? '1' : '0',
			      (cgentry >> 5) & 1 ? '1' : '0',
			      (cgentry >> 6) & 1 ? '1' : '0',
			      (cgentry >> 7) & 1 ? '1' : '0');

		/* THE CLAIM IS DECIDED, NOT RE-DECIDED.  §7tricies.51.  The
		   negotiation before the plan already settled whether the
		   claimed interval model is needed, and the plan was built
		   from that kunroll; retrying here would change the kunroll
		   under a plan that cannot follow.  So the colouring above is
		   redone with the flag it chose, and the order is asked only
		   to confirm.  */
		if (plclaimed)
		  {
		    claimed = true;
		    cok = lvx_lms_colour_lranges (seq, cglr, found, lo,
						  cgavail, &nn, &nst, &mx,
						  &kun, &pin, &pinf, &wide,
						  plclaimed, NULL, cgentry);
		  }
		{
		  auto_vec<int> probe_order;

		  lok = lvx_lms_slot_order (seq, plslots, cglr, cgloopback,
					    probe_order);
		}
		rok2 = lvx_lms_rename_lr_dryrun (seq, plslots, cglr, &subst,
						 &refused);
		aok = lvx_lms_verify_lr_antidep (seq, cglr, found, lo,
						 kun > 0 ? kun : 1, &nconf,
						 &ca, &cb, &creg, &ccyc,
						 &cnheld, &cninv, &cnocc);
		/* §7tricies.112: and the commitment the dropped CTRL_LIVEOUT
		   arcs were made on.  Before emission, with the rest.  */
		if (lvx_lms_liveout_split)
		  {
		    int losbad = 0;

		    if (!lvx_lms_verify_liveout_split (cglr, &losbad))
		      {
			lvx_lms_dump (";; lvx cms: fn=%s loop %d liveout"
				      " split BAD, %d range(s) kept the"
				      " architected register\n",
				      current_function_name (), loop->num,
				      losbad);
			losok = false;
		      }
		  }
		if (!aok && lvx_lms_probe && dump_file)
		  {
		    /* The two live ranges as the CODEGEN assignment left them,
		       which is not what the probe's dump shows.  */
		    lvx_lms_dump (";; lvx cms:   conflict at r%d cycle %d"
				  " between:\n", creg, ccyc);
		    for (int z = 0; z < 2; z++)
		      {
			int kk = z ? cb : ca;
			lvx_lms_lrange_part *g;

			if (kk < 0 || kk >= (int) cglr.length ())
			  continue;
			g = cglr[kk];
			lvx_lms_dump (";; lvx cms:     lr%d r%u+%d p2=%d"
				      " uf=%d ren=%d ec=%d [%d,%d] regs=",
				      kk, g->regno, g->nregs, g->p2unroll,
				      g->use_first ? 1 : 0,
				      g->renamed ? 1 : 0,
				      g->effects.is_empty () ? -1
				      : (lvx_insn_dest_earlyclobber_p
					   (seq[g->effects[0].insn]) ? 1 : 0),
				      g->first_date, g->last_date);
			for (int j = 0; j < g->p2unroll
					&& j < LVX_LMS_MAX_KUNROLL; j++)
			  lvx_lms_dump ("%sr%d", j ? "," : "", g->reg[j]);
			lvx_lms_dump (" eff=");
			for (unsigned e = 0; e < g->effects.length (); e++)
			  lvx_lms_dump ("%s%u/%d%s%s", e ? "," : "",
					g->effects[e].insn,
					g->effects[e].slot,
					g->effects[e].is_def ? "D" : "U",
					g->effects[e].delayed ? "~" : "");
			lvx_lms_dump ("\n");
		      }
		  }

		if (kun != plkun)
		  /* §7tricies.47: the kernel was built for a different number
		     of copies than the names rotate through.  */
		  iwhy = "the plan and the renaming disagree about kunroll";
		else if (pl.steady_bad)
		  /* The kernel is not a steady state: see the long note at
		     STEADYBAD's computation.  A block missing one instance of
		     one instruction is missing it on every trip.  */
		  iwhy = "the kernel is not a steady state";
		else if (!vok)
		  iwhy = "an operand belongs to no live range";
		else if (!cok)
		  iwhy = "the live ranges do not colour";
		else if (!lok)
		  iwhy = "a cycle's reads and writes cannot be linearised";
		else if (!aok)
		  iwhy = "a dropped anti-dependence is not separated";
		else if (!losok)
		  /* §7tricies.151: the live-out split's own commitment, which
		     is that every dropped CTRL_LIVEOUT arc's range got a name
		     of its own.  A range that kept the architected register
		     did not, and the drop promised it would.  */
		  iwhy = "a live-out split range kept the architected register";
		else if (!rok2)
		  iwhy = "a slot's insn refuses the rename substitution";
		else
		  {
		    /* Every check has passed: now the CFG may be touched.
		       §7tricies.145.  */
		    built = lvx_lms_make_nodes (&site, loop, &plnodes,
						&cgblocks, &nwhy);
		    lvx_lms_dump (";; lvx cms: fn=%s loop %d make_nodes %s"
				  " blocks=%u nodes=%u\n",
				  current_function_name (), loop->num,
				  built ? "built" : nwhy, cgblocks.length (),
				  plnodes.length ());
		    if (!built)
		      iwhy = nwhy;
		    else
		      emitted = lvx_lms_emit_pipeline (&site, seq, plslots,
						       plnodes, cgblocks,
						       cglr, cgavail, &nemit,
						       &iwhy);
		  }
		lvx_lms_dump (";; lvx cms: fn=%s loop %d cgcolour %s nodes=%d"
			      " maxreg=%d kunroll=%d subst=%d refused=%d"
			      " linear=%d claimed=%d lrantidep=%s losplit=%s"
			      " conf=%d"
			      " held=%d inv=%d occ=%d"
			      " pair=%d/%d/r%d@%d avail=%d savepool=%d\n",
			      current_function_name (), loop->num,
			      cok ? "ok" : "no", nn, mx, kun, subst, refused,
			      lok ? 1 : 0, plclaimed,
			      aok ? "ok" : "BAD",
			      losok ? "ok" : "BAD", nconf,
			      cnheld, cninv, cnocc, ca, cb, creg, ccyc,
			      (int) popcount_hwi (cgavail), pool);
		/* THE CODEGEN SET, TAGGED.  Only the probe set was dumped,
		   and the two are built at different stages from different
		   schedules -- reading the probe set as if it were this one
		   produced two wrong diagnoses (§7tricies.71).  */
		if (lvx_lms_probe && dump_file)
		  lvx_lms_dump_lranges (seq, cglr, "cg");
		for (unsigned k = 0; k < cglr.length (); k++)
		  delete cglr[k];
	      }
	      /* THE INHERITED LOOPDO IS NOW STALE, SO IT GOES.  §7tricies.201.
		 In the after-mapping order a mapped loop arrives with an
		 lvx_loopdo in its preheader, and the pipeline has just
		 replaced the loop that LOOPDO described: its count is the
		 original trip count, its end label the original loop's, and
		 its POSITION is before the prologue -- so the hardware loop
		 would span the prologue as well.  All three are wrong and
		 none is worth patching, because the second reorg_loops will
		 map the kernel properly from the final CFG.  Deleting it is
		 what makes room for that.

		 It is the whole of 7tricies.196's wrong code: site->loopdo
		 was recorded and never read, so the kernel ran unrolled and
		 overlapped against the original count, and the gate's
		 hardware-loop arm failed all eight tests.  */
	      if (emitted && site.loopdo != NULL)
		{
		  lvx_lms_dump (";; lvx cms: fn=%s loop %d LOOPDODROP uid=%d,"
				" the second mapping will remap the kernel\n",
				current_function_name (), loop->num,
				INSN_UID (site.loopdo));
		  delete_insn (site.loopdo);
		  site.loopdo = NULL;
		}
	      lvx_lms_dump (";; lvx cms: fn=%s loop %d pipeline %s emitted=%d"
			    " slots=%u\n", current_function_name (),
			    loop->num, emitted ? "built" : iwhy, nemit,
			    plslots.length ());
	    }

	}
      else if (dump_file)
	lvx_lms_dump (";; lvx cms: fn=%s loop %d codegen declined, %s\n",
		      current_function_name (), loop->num,
		      !site.preheader ? "no preheader" : "latch has no exit edge");
    }

  /* LINEAGES over the omega = 0 subgraph, and the interference graph they give
     instead of the one over individual values.  §7octodecies.5.  */
  int nlin = 0, nseq = 0, linnodes = 0, linedges = 0, linsq = 0, linspill = 0;
  {
    auto_vec<bool> defines;
    defines.safe_grow_cleared (seq.length ());
    for (unsigned i = 0; i < seq.length (); i++)
      defines[i] = false;
    for (unsigned a = 0; a < arcs.length (); a++)
      if (arcs[a].kind == LVX_ARC_REG_RAW)
	defines[arcs[a].src] = true;

    auto_vec<int> lin;
    lvx_lms_lineages (arcs, seq.length (), defines, lin, &nlin, &nseq);

    /* A lineage's live range runs from the definition of its head to the last
       use of its tail -- the concatenation of its members' ranges, which is
       the point of forming it.  Its class is the widest member's, since one
       register must hold them all.  */
    if (nlin > 0)
      {
	auto_vec<int> ldef, llast, lnr;
	ldef.safe_grow_cleared (nlin);
	llast.safe_grow_cleared (nlin);
	lnr.safe_grow_cleared (nlin);
	for (int k = 0; k < nlin; k++)
	  ldef[k] = INT_MAX, llast[k] = -1, lnr[k] = 1;

	for (unsigned i = 0; i < seq.length (); i++)
	  {
	    if (lin[i] < 0)
	      continue;
	    int k = lin[i];
	    if (sigma[i] < ldef[k])
	      ldef[k] = sigma[i];
	    for (unsigned a = 0; a < arcs.length (); a++)
	      if (arcs[a].src == i && arcs[a].kind == LVX_ARC_REG_RAW)
		{
		  int when = sigma[arcs[a].dst] + (int) arcs[a].omega * found;
		  if (when > llast[k])
		    llast[k] = when;
		}
	    rtx ds = single_set (seq[i]);
	    int nr = (ds && REG_P (SET_DEST (ds))) ? REG_NREGS (SET_DEST (ds)) : 1;
	    if (MAX (nr, 1) > lnr[k])
	      lnr[k] = MAX (nr, 1);
	  }

	auto_vec<int> cdef, clast, cdeg, cnr, cec;
	for (int k = 0; k < nlin; k++)
	  {
	    if (ldef[k] == INT_MAX || llast[k] <= ldef[k])
	      continue;
	    cdef.safe_push (ldef[k]);
	    clast.safe_push (llast[k]);
	    cdeg.safe_push ((llast[k] - ldef[k] + found - 1) / found);
	    cnr.safe_push (lnr[k]);
	    cec.safe_push (0);
	  }
	int per, kun;
	lvx_lms_colour (cdef, clast, cdeg, cnr, cec, found,
			lvx_lms_sigma_min (sigma), &linnodes, &linedges,
			&linsq, &linspill, &per, &kun);
      }
  }

  /* How many expanded values admit a repair that keeps MinII?  §7novodecies.
     SPLITOK counts those a splitting COPYD could serve (one extra TINY per
     register, latency 1); SPILLOK those a store/reload could (two extra
     instructions, and the reload's latency, which under -mlms-load-latency is
     the uncached figure).  */
  int splitok = 0, spillok = 0, cand = 0, nsplit = 0, spl_after = -1;
  int spl_nodes = 0, spl_before = -1, nspill = 0, sp_after = -1, sp_why = 0;
  {
    /* DEM is computed above, before emission: §7tricies.103 explains why it
       cannot be taken here.  */
    /* A spill reload is a CACHED load whatever the data loads are (author,
       2026-10-27): the slot is in the frame, which is hot, so it is priced at
       the description's cached figure of 3 and NOT at -mlms-load-latency's
       uncached one.  That option models the loop's data stream, which may be
       streamed and not reused; the frame is neither.  The distinction matters
       because 15 cycles of reload latency inside a recurrence could almost
       never fit the current II, which would have made spilling look useless
       for exactly the loops where the uncached regime makes it necessary.  */
    int rl = 3;
    for (unsigned i = 0; i < seq.length (); i++)
      {
	/* Only values that are expanded at all are candidates: a range inside
	   one stage has nothing to split.  */
	int last = -1;
	for (unsigned a = 0; a < arcs.length (); a++)
	  if (arcs[a].src == i && arcs[a].kind == LVX_ARC_REG_RAW)
	    {
	      int when = sigma[arcs[a].dst] + (int) arcs[a].omega * found;
	      if (when > last)
		last = when;
	    }
	if (last < 0 || last - sigma[i] <= found)
	  continue;
	cand++;
	rtx ds = single_set (seq[i]);
	int nr = (ds && REG_P (SET_DEST (ds))) ? REG_NREGS (SET_DEST (ds)) : 1;
	nr = MAX (nr, 1);
	if (lvx_lms_repair_fits_ii_p (seq, arcs, &dem, i, found, 1, nr))
	  splitok++;
	if (lvx_lms_repair_fits_ii_p (seq, arcs, &dem, i, found, rl, 2 * nr))
	  spillok++;
      }

    /* And does splitting actually make the kernel colourable?  Only asked when
       it was not, since otherwise there is nothing to repair.  */
    if (igspill > 0)
      {
	nsplit = lvx_lms_split_until_colourable (seq, arcs, sigma, &dem, found,
						 lvx_lms_sigma_min (sigma),
						 &spl_after, &spl_nodes,
						 &spl_before);
	int dummy;
	nspill = lvx_lms_spill_until_colourable (seq, arcs, sigma, &dem, found,
						 lvx_lms_sigma_min (sigma),
						 &sp_after, &sp_why, &dummy);
      }
  }

  /* The author's window invariant.  Checked on the WINDOW, not on SIGMA: a
     resource-feasible schedule may sit outside a dependence-only window and
     usually does, so the thing worth asserting here is that the window is
     non-empty at this II -- Estart (i) <= Lstart (i) everywhere, within the
     graph's own makespan bound.  Applying it to an MCRL date is the gate that
     comes when the MCRL graph construction lands.  §7quinvicies.2.  */
  int winempty = 0, winfirst = -1, winmake = 0;
  /* MCRL at this II, and the author's makespan feedback.  Two solves: one in
     the dependence-only window, one in the window the achieved SPAN widens to.
     MCRLBAD counts dates outside the window -- which must be zero, the window
     arcs being part of the network -- and MCRLDEP counts violated dependences,
     which must be zero by complementary slackness.  Both are assertions
     written as counters so a failure is reported rather than silent.
     §7quinvicies.4, §7quinvicies.6.  */
  int mcrl = -1, mcrlbad = -1, mcrldep = -1, mcrlmake = 0;
  int mcrl2 = -1, mcrlmake2 = 0;
  unsigned mcrlnn = 0, mcrlne = 0;
  HOST_WIDE_INT mcrllife = -1, mcrllife2 = -1, mcrlsigma = -1;
  {
    auto_vec<int> ts, sl, nodates;
    lvx_lms_path_metrics (seq, arcs, height, ts, sl);
    for (int round = 0; round < 2; round++)
      {
	auto_vec<int> es, ls;
	int floor_ = round ? span : 0;
	int wempty = 0, wfirst = -1, wmake = 0;
	lvx_lms_dates_in_window (seq, arcs, nodates, ts, found,
				 &wfirst, &wmake, &wempty, &es, &ls, floor_);
	if (round == 0)
	  {
	    winempty = wempty;
	    winfirst = wfirst;
	    winmake = wmake;
	    mcrlmake = wmake;
	  }
	else
	  mcrlmake2 = wmake;
	if (wempty || es.length () != seq.length ())
	  continue;

	auto_vec<int> md;
	HOST_WIDE_INT mcost;
	unsigned nn, ne;
	enum lvx_network_result r
	  = lvx_lms_mcrl_dates (seq, arcs, es, ls, found, md, &mcost, &nn, &ne);
	if (round == 0)
	  {
	    mcrl = (int) r;
	    mcrlnn = nn;
	    mcrlne = ne;
	  }
	else
	  mcrl2 = (int) r;
	if (r != LVX_NETWORK_SOLVED)
	  continue;

	HOST_WIDE_INT life
	  = lvx_lms_cumulative_lifetime (seq, arcs, md, found);
	if (round == 0)
	  {
	    mcrllife = life;
	    /* The two checks.  */
	    int bad = 0;
	    for (unsigned i = 0; i < seq.length (); i++)
	      if (md[i] < es[i] || md[i] > ls[i])
		bad++;
	    mcrlbad = bad;
	    int dep = 0;
	    for (unsigned a = 0; a < arcs.length (); a++)
	      if (md[arcs[a].dst] - md[arcs[a].src]
		  < arcs[a].theta - (HOST_WIDE_INT) found * arcs[a].omega)
		dep++;
	    mcrldep = dep;
	    /* What the achieved schedule costs on the same objective.  */
	    auto_vec<int> sg;
	    int slo = lvx_lms_sigma_min (sigma);
	    for (unsigned i = 0; i < sigma.length (); i++)
	      sg.safe_push (sigma[i] - slo);
	    mcrlsigma = lvx_lms_cumulative_lifetime (seq, arcs, sg, found);
	  }
	else
	  mcrllife2 = life;
      }
  }

  lvx_lms_dump (";; lvx cms: fn=%s loop %d II=%d lower=%d excess=%d M=%d"
		" ResMII=%d RecMII=%d span=%d spanlat=%d nstages=%d BS=%d preps=%d"
		" mvemax=%d mvep2=%d mvelcm=%d mven=%d mvetop=%d mve2nd=%d"
		" mvesplit0=%d mvesplit=%d mvesplitr=%d mvesplitx=%d"
		" mvenospl=%d mvewide=%d mvewidemax=%d mvecatdq=%d"
		" mveregs=%d mveregsx=%d mvepress=%d"
		" ig=%d/%d kunroll=%d period=%d sq=%d spill=%d ec=%d"
		" igassigned=%d igmaxreg=%d igpinned=%d igpinfail=%d"
		" plan=%d plpro=%d plker=%d plreuse=%d plalloc=%d plexep=%d"
		" lin=%d seqe=%d linig=%d/%d linsq=%d linspill=%d"
		" cand=%d splitok=%d spillok=%d staged=%d nsplit=%d before=%d after=%d"
		" nspill=%d spafter=%d spwhy=%d winempty=%d winmake=%d"
		" mcrl=%d mcrlbad=%d mcrldep=%d mcrlmake=%d mcrlnn=%u mcrlne=%u"
		" mcrllife=" HOST_WIDE_INT_PRINT_DEC " mcrlsigma=" HOST_WIDE_INT_PRINT_DEC
		" mcrl2=%d mcrlmake2=%d mcrllife2=" HOST_WIDE_INT_PRINT_DEC
		" emptycyc=%d"
		" delta=%d safelen=%d"
		" passes=%d tight=%s:%d/%d blamed=%s:%d, %u nodes %u arcs,"
		" %d loads %s .s%s -- %s\n",
		current_function_name (), loop->num, found, lower, found - lower, m,
		r, k, span, spanlat,
		nstages, bs, preps, mvemax, mvep2,
		mvelcm_overflow ? 0 : (int) mvelcm,
		mven, mvetop, mve2nd, mvesplit0, mvesplit, mvesplitr, mvesplitx,
		mvenospl, mvewide, mvewidemax, mvecatdq, mveregs, mveregsx,
		mvepress, ignodes, igedges, igkunroll, igperiod, igmaxsq, igspill,
		igec, igassigned, igmaxreg, igpinned, igpinfail,
		pl.ok ? pl.slots : -1, pl.prologue, pl.kernel, pl.blocks_reused,
		pl.blocks_allocated, pl.exit_epilogues, nlin, nseq, linnodes, linedges, linsq, linspill,
		cand, splitok, spillok, staged, nsplit, spl_before, spl_after,
		nspill, sp_after, sp_why, winempty, winmake,
		mcrl, mcrlbad, mcrldep, mcrlmake, mcrlnn, mcrlne,
		mcrllife, mcrlsigma, mcrl2, mcrlmake2, mcrllife2,
		emptycyc,
		delta, (int) loop->safelen, passes, tight_name, tight_dem, tight_cap,
		blamed, blamed_cost, seq.length (), arcs.length (),
		spec < 0 ? -spec : spec, spec < 0 ? "forced to" : "need",
		hw ? (loopdo ? " hwloop mapped" : " hwloop") : "",
		!drop_ok ? "UNSOUND: dropped safelen below nstages"
		: nstages > 1 ? "PIPELINES" : "no gain");

  /* Name the circuit that set K.  Measuring RecMII says how deep the pipeline
     can be; naming its circuit says what to do about it.  A recurrence through
     a chain of reg-raw arcs is the program's own arithmetic, and only algebra
     -- reassociation, or more accumulators -- will shorten it.  One that runs
     through mem-cross is a dependence the compiler could not disprove, and
     restrict or ivdep may remove it outright.  One through ctrl-* is the exit
     structure rather than the computation, and is what control speculation and
     the loopback's own treatment address.

     Printed with the sum of theta and of omega along the circuit, because K is
     their ratio and the ratio is what says whether the circuit is long or
     merely near.  */
  if (!crit.is_empty ())
    {
      HOST_WIDE_INT st = 0, sw = 0;
      unsigned nreal = 0;
      for (unsigned i = 0; i < crit.length (); i++)
	if (crit[i] >= 0)
	  {
	    st += arcs[crit[i]].theta;
	    sw += arcs[crit[i]].omega;
	    nreal++;
	  }
      lvx_lms_dump (";; lvx cms: fn=%s loop %d RecMII circuit, %u arcs,"
		    " sum theta=" HOST_WIDE_INT_PRINT_DEC
		    " sum omega=" HOST_WIDE_INT_PRINT_DEC ":",
		    current_function_name (), loop->num, nreal, st, sw);
      for (unsigned i = 0; i < crit.length (); i++)
	if (crit[i] >= 0)
	  {
	    const lvx_lms_arc &a = arcs[crit[i]];
	    lvx_lms_dump (" %s(%d->%d,%d,%d)",
			  lvx_lms_arc_kind_name[(int) a.kind],
			  INSN_UID (seq[a.src]), INSN_UID (seq[a.dst]),
			  (int) a.theta, (int) a.omega);
	  }
      /* WHICH OF THE CIRCUIT'S NODES ARE REGISTER COPIES.  §7tricies.105.
	 A copy on a critical circuit contributes its own latency to the
	 recurrence, and a reader can be rewritten to take the copy's SOURCE
	 instead -- `p -> c -> u' becoming `p -> u' with the thetas of the
	 first arc and the omegas added, which is legal across iterations and
	 drops the copy's theta from the circuit.  Dumped rather than
	 implemented, because the last three refinements measured here were
	 worth nothing, nothing and nothing-usable, and the number that
	 decides this one is how many critical circuits contain such a node.  */
      {
	int ncopy = 0, ntied = 0;

	for (unsigned i = 0; i < crit.length (); i++)
	  if (crit[i] >= 0)
	    {
	      rtx_insn *ci = seq[arcs[crit[i]].src];
	      rtx set = single_set (ci);

	      if (set == NULL_RTX || !REG_P (SET_DEST (set))
		  || !REG_P (SET_SRC (set)))
		continue;
	      ncopy++;
	      /* AND WHETHER THE TIE FORBIDS IT.  §7tricies.105.  These are
		 POST-RA copies, and the allocator inserts many of them
		 precisely so that an instruction can have an input and its
		 output in one register.  Rewriting that input to the copy's
		 source while the output stays put breaks the match and
		 produces an insn that does not recognise -- `fma r1 = a,
		 r41', the §7tricies.28 failure, 8 refused instances on one
		 loop.  So a copy whose reader carries a tied operand is not
		 a candidate, and counting those apart is what turns the
		 opportunity from an upper bound into a figure.  */
	      if (lvx_insn_has_tied_operand_p (seq[arcs[crit[i]].dst]))
		ntied++;
	    }
	lvx_lms_dump (" copies=%d tied=%d", ncopy, ntied);
      }
      lvx_lms_dump ("\n");
    }
}

/* Run the convergent scheduler over SMS's OWN dependence graph, so the two
   schedulers can be compared from identical input.  R is what this target just
   answered for ResMII, and is reused rather than recomputed: the point of
   feeding one graph to both is that the bounds are the same by construction,
   and recomputing R from the ddg with the CMS resource model would reintroduce
   the one difference worth eliminating.

   The conversion is mechanical because a ddg node IS an insn and a ddg edge
   already carries exactly (latency, distance):

     seq[i]  = g->nodes[i].insn,  and cuid == i (create_ddg sets it so)
     arc     = (src->cuid, dest->cuid, e->latency, e->distance)

   Single copy, not the base/twin pair kvx-unfold.cc built from this same graph.
   Two copies cannot represent a distance above one, and here they need not: the
   distance is carried on the arc.  It happens that a ddg never has one anyway --
   add_inter_loop_mem_dep hard-codes 1 and the register paths use 0 or 1, which
   is what kvx-unfold.cc's `gcc_assert (latency >= 0 && distance <= 1)' relies
   on -- but the single-copy form does not depend on that holding.

   On classifying the arcs: dep_data_type has FOUR values, not two
   (REG_OR_MEM_DEP, REG_DEP, MEM_DEP, REG_AND_MEM_DEP), so a bare `== REG_DEP'
   test silently lumps three of them together.  That is the bug in
   kvx-unfold.cc:4998, which sets its "register dependence" flag on memory
   dependences and clears it on register ones -- latent only because nothing
   reads the flag.  Anything not purely REG_DEP is treated as memory here.

   What this does NOT equalise, and must be said rather than discovered: SMS
   builds this graph BEFORE register allocation, so its register arcs are true
   dataflow over pseudos, while lvx_lms_build_graph reconstructs them after
   allocation from reaching definitions.  Comparing the two K values for one
   loop is therefore a test of that reconstruction against ground truth, which
   is the most valuable thing this arrangement buys.  */

void
lvx_lms_probe_ddg (struct ddg *g, int r)
{
  auto_vec<rtx_insn *> seq;
  auto_vec<lvx_lms_arc> arcs;
  auto_vec<int> height;
  unsigned n = (unsigned) g->num_nodes;

  if (n == 0)
    return;

  for (unsigned i = 0; i < n; i++)
    seq.safe_push (g->nodes[i].insn);
  height.safe_grow_cleared (n);

  for (unsigned i = 0; i < n; i++)
    for (ddg_edge_ptr e = g->nodes[i].out; e != NULL; e = e->next_out)
      {
	unsigned src = (unsigned) g->nodes[i].cuid;
	unsigned dst = (unsigned) e->dest->cuid;
	enum lvx_lms_arc_kind kind;

	if (e->data_type == REG_DEP)
	  kind = (e->type == TRUE_DEP ? LVX_ARC_REG_RAW
		  : e->type == ANTI_DEP ? LVX_ARC_REG_WAR
		  : LVX_ARC_REG_WAW);
	else
	  kind = e->distance ? LVX_ARC_MEM_CROSS : LVX_ARC_MEM_INTRA;

	lvx_lms_push_arc (arcs, src, dst, e->latency, e->distance, kind);

	/* The dependence height, relaxed forward.  An intra-iteration edge runs
	   forward in program order -- create_ddg asserts src->cuid <
	   dest->cuid -- so ascending order settles it in one sweep.  */
	if (e->distance == 0 && height[dst] < height[src] + e->latency)
	  height[dst] = height[src] + e->latency;
      }

  /* K, by the same parametric solve the loop path uses: node 0 is the solver's
     root and node i + 1 is seq[i], with a root arc to each so every node is
     reachable.  */
  lvx_plc plc;
  for (unsigned i = 0; i <= n; i++)
    plc.add_node ();
  for (unsigned i = 0; i < n; i++)
    plc.add_arc (0, i + 1, 0, 0);
  for (unsigned i = 0; i < arcs.length (); i++)
    plc.add_arc (arcs[i].src + 1, arcs[i].dst + 1, arcs[i].theta,
		 arcs[i].omega);
  if (plc.solve (1) != LVX_PLC_SOLVED)
    {
      lvx_lms_dump (";; lvx cms-on-ddg: recurrence bound did not solve\n");
      return;
    }
  int k = (int) plc.period ();

  int m = 1;
  for (unsigned i = 0; i < n; i++)
    m = MAX (m, height[i] + lvx_lms_latency (seq[i]));

  int delta = lvx_lms_delta (arcs);
  int lower = MAX (k, r);
  auto_vec<int> sigma;
  int passes = 0, found = 0;
  const char *last_bad = NULL;
  int budget = MAX (1, lvx_cms_swo_budget);

  for (int try_ii = lower; try_ii <= lower + 8 && !found; try_ii++)
    if (lvx_cms_swo_at_ii (seq, arcs, height, try_ii, delta, sigma,
			   budget, &passes, &last_bad,
			   lvx_cms_branch_stage == 3 ? 1
			   : lvx_cms_branch_stage))
      found = try_ii;

  if (!found)
    {
      lvx_lms_dump (";; lvx cms-on-ddg: NO schedule through II=%d"
		    " lower=%d ResMII=%d RecMII=%d M=%d passes=%d,"
		    " %u nodes %u arcs%s%s\n", lower + 8, lower, r, k, m, passes,
		    n, arcs.length (),
		    last_bad ? " -- converged but unverified: " : "",
		    last_bad ? last_bad : "");
      return;
    }

  int lo = sigma[0], hi = sigma[0];
  for (unsigned i = 1; i < n; i++)
    {
      if (lo > sigma[i])
	lo = sigma[i];
      if (hi < sigma[i])
	hi = sigma[i];
    }
  /* The cyclic makespan: last ISSUE, not last completion.  See the long note
     at the main dump site and §7quinvicies.5.  */
  int span = hi - lo + 1;
  int nstages = (span + found - 1) / found;

  lvx_lms_dump (";; lvx cms-on-ddg: II=%d lower=%d excess=%d ResMII=%d"
		" RecMII=%d M=%d span=%d nstages=%d delta=%d passes=%d,"
		" %u nodes %u arcs\n", found, lower, found - lower, r, k, m, span,
		nstages, delta, passes, n, arcs.length ());
}

/* Whether CMS may consider LOOP at all.  This was NOT the unfold's own test
   (removed with it, §7tricies.101):
   that gate is sized for code duplication and asks for things a modulo
   schedule does not need, while missing nothing CMS does need.  The
   differences, each deliberate:

   - No `can_duplicate_loop_p'.  The unfold duplicates the body through the
     CFG; CMS replicates the KERNEL by its stage count during modulo
     expansion, which is a different operation on a different object.

   - No block-count bound.  The unfold's is there because duplication doubles
     the blocks; CMS linearises the chain and schedules instructions, so what
     bounds it is instruction count, not block count.

   - Its own instruction bound, -mlms-max-insns, because the cost of a CMS
     attempt is the convergence loop over DELTA + 4 copies of the body, not a
     doubling of code size.

   - Hardware loops are ALLOWED.  The unfold refuses them because halving a
     trip count means peeling; a pipeliner has to adjust the trip count
     anyway, and an implicit loopback branch is a help.  lvx_lms_pipeline_loop
     reports which loops have one so the measurement can separate them.

   What CMS genuinely requires, and this gate establishes: a single latch, so
   the chain walk has one back edge to measure distance against; no call,
   volatile reference or asm, none of which can be moved across an iteration
   boundary or priced; and no computed jump or complex edge, which the chain
   walk cannot follow.  The chain property itself is left to
   lvx_lms_build_graph, which is the code that needs it and so is the code that
   should report it.

   Note the ORDER against the unfold's gate: the call scan comes before the
   edge counting, so a loop that is both multi-block and call-holding is named
   for the call.  The unfold's gate tests per block and reports whichever test
   that block fails first, which is why PolyBench's print_array loops come out
   as "not a chain" when they also hold an fprintf.  A diagnostic should name
   the disqualifier a reader can act on.  */

static bool
lvx_lms_eligible_loop_p (class loop *loop, basic_block *body, unsigned insns,
			 const char **why)
{
  *why = "multiple latches";
  if (loop->latch == NULL)
    return false;
  *why = "too many insns";
  if (insns > (unsigned) lvx_lms_max_insns)
    return false;

  /* The contents of every block before the shape of any, so the reason
     reported is the one a reader can act on.  Interleaving the two -- testing
     each block's shape and then its contents, as the unfold's gate does --
     makes the answer depend on which block happens to come first in the body
     array, and a loop that holds a call is then liable to be reported for its
     control flow.  */
  for (unsigned i = 0; i < loop->num_nodes; i++)
    {
      rtx_insn *insn, *end = BB_END (body[i]);

      for (insn = BB_HEAD (body[i]); insn != NEXT_INSN (end);
	   insn = NEXT_INSN (insn))
	if (NONDEBUG_INSN_P (insn))
	  {
	    *why = "holds a call";
	    if (CALL_P (insn))
	      return false;
	    *why = "holds a volatile reference";
	    if (volatile_refs_p (PATTERN (insn)))
	      return false;
	    *why = "holds an asm";
	    if (asm_noperands (PATTERN (insn)) >= 0)
	      return false;
	  }
    }

  for (unsigned i = 0; i < loop->num_nodes; i++)
    {
      basic_block bb = body[i];
      edge e;
      edge_iterator ei;

      *why = "block not schedulable";
      if ((bb->flags & BB_DISABLE_SCHEDULE) != 0
	  || computed_jump_p (BB_END (bb)))
	return false;

      FOR_EACH_EDGE (e, ei, bb->succs)
	{
	  *why = "complex edge";
	  if (e->flags & EDGE_COMPLEX)
	    return false;
	}

      /* THE BODY MUST BE A TRACE, AND NOTHING SAID SO.  §7tricies.162.

	 A superblock is single-entry and multiple-exit, so a body block with
	 two IN-LOOP predecessors is a side entry and the body is not one.
	 Rotation cannot remove a side entry -- §7tricies.152 established that
	 for the masked-store hammock -- so such a loop is not pipelineable by
	 this emission at all.

	 CMS never asked.  Measured: all 962 loops that emit are join-free, so
	 the requirement has always held in fact, and what enforced it was an
	 unrelated test -- "latch has no exit edge", which 22 of its 49
	 refusals owe to the body having joins rather than to the latch.  An
	 invariant held by accident is one that breaks when the accident is
	 fixed, which is exactly what relaxing the latch test would have done.

	 The header is excluded: its second predecessor is the back edge by
	 construction.  */
      {
	unsigned inpred = 0;

	if (bb != loop->header)
	  {
	    FOR_EACH_EDGE (e, ei, bb->preds)
	      if (flow_bb_inside_loop_p (loop, e->src))
		inpred++;
	    *why = "a body block has two in-loop predecessors";
	    if (inpred > 1)
	      return false;
	  }
      }
    }
  *why = NULL;
  return true;
}

/* CAN THE PIPELINE'S NODE STRUCTURE BE BUILT FOR THIS LOOP?  §7quinquies.7.

   This is deliberately NOT part of lvx_lms_eligible_loop_p, and §7.5a says why:
   the reference interleaves three concerns in one 29-reason gate --
   transformation legality, analysis feasibility, and PIPELINE CODEGEN
   feasibility -- and importing the whole thing cost the unfold 1.6 measured
   points by refusing loops it could have taken.  CMS needs the third column
   back, but "as its own predicate, because it will be answering a different
   question (can this loop's node structure be built?) from the one the unfold
   asks".  So this answers only that, and the analysis gate answers only its
   own question.

   What the structure requires, from §7quinquies:

     - EXACTLY TWO SUCCESSORS per block.  `unfold_codegen_make_nodes' assigns
       `node->body_edge' and `node->exit_edge' from the two, and every later
       phase consumes that quadruple -- make_insns redirects through it and the
       per-exit epilogues are built from it.  A block with one successor has no
       exit_edge, so the node model cannot represent it (§7.5a).  This is
       CODEGEN's requirement, not the analysis's, which is the whole reason the
       two predicates differ.
     - BODY_EDGE IS NOT A LOOP EXIT AND EXIT_EDGE IS.  Without the distinction
       the quadruple is assignable but wrong, and the kernel's back edge would
       be some other iteration's exit test.
     - LAYOUT CONTIGUITY.  `schedule_ebbs' walks `bb->next_bb', not the edge
       (§7quaterdecies.1), and `make_nodes' reuses source blocks in that order,
       so the body must fall through in next_bb order to one of its successors.
     - THE HEADER FOLLOWS THE PREHEADER, and the post block is the latch's
       next_bb.  Both are layout tails the reference checks last.

   Reports WHICH condition failed, because §7septies.7 records that a
   diagnostic naming only a symptom already cost one misdiagnosis here.  */

static bool
lvx_lms_codegen_feasible_p (class loop *loop, basic_block *body,
			    const char **why)
{
  *why = "no preheader";
  if (loop->header->preds == NULL || EDGE_COUNT (loop->header->preds) != 2)
    return false;
  {
    edge pre = NULL;
    edge e;
    edge_iterator ei;
    FOR_EACH_EDGE (e, ei, loop->header->preds)
      if (e->src != loop->latch)
	pre = e;
    if (!pre)
      return false;
    *why = "header does not follow the preheader";
    if (pre->src->next_bb != loop->header)
      return false;
  }

  *why = "no post block after the latch";
  if (loop->latch->next_bb == NULL
      || loop->latch->next_bb == EXIT_BLOCK_PTR_FOR_FN (cfun))
    return false;

  int backedges = 0;
  for (unsigned i = 0; i < loop->num_nodes; i++)
    {
      basic_block bb = body[i];
      edge e, body_e = NULL, exit_e = NULL;
      edge_iterator ei;

      *why = "block does not have exactly two successors";
      if (EDGE_COUNT (bb->succs) != 2)
	return false;

      FOR_EACH_EDGE (e, ei, bb->succs)
	{
	  if (flow_bb_inside_loop_p (loop, e->dest))
	    {
	      if (body_e)
		{
		  /* Both successors inside the loop: a diamond, not a chain.
		     The analysis gate rejects this too, but for its own
		     reason -- the profitability walk needs a path.  */
		  *why = "both successors are in the loop";
		  return false;
		}
	      body_e = e;
	    }
	  else
	    exit_e = e;
	}

      *why = "no in-loop successor";
      if (!body_e)
	return false;
      *why = "no exit successor";
      if (!exit_e)
	return false;
      *why = "the in-loop edge is marked a loop exit";
      if (body_e->flags & EDGE_LOOP_EXIT)
	return false;

      /* A BACK EDGE IS ONE WHOSE DESTINATION IS THE HEADER, derived rather
	 than read off EDGE_DFS_BACK.  That flag is set by
	 `mark_dfs_back_edges', which NOTHING in the LVX path calls -- this was
	 its only reader in the file -- so what it holds at machine_reorg is
	 whatever an earlier pass happened to leave.  Reading it reported 6 of
	 163 PolyBench loops as having "not exactly one back edge" when the
	 eligibility gate has already established a single latch, which is what
	 said the flag was stale rather than the loops unusual.  */
      if (body_e->dest == loop->header)
	backedges++;

      *why = "neither successor falls through";
      if (!(body_e->flags & EDGE_FALLTHRU)
	  && !(exit_e->flags & EDGE_FALLTHRU))
	return false;
      *why = "the fall-through successor is not the next block";
      if (bb->next_bb != body_e->dest && bb->next_bb != exit_e->dest)
	return false;
    }

  *why = "not exactly one back edge";
  if (backedges != 1)
    return false;

  *why = NULL;
  return true;
}

/* CMS's own walk over the innermost loops, independent of the unfold.

   It used to ride lvx_loop_profitable_to_unfold_p, which meant the probe only
   saw loops that had already passed the unfold's candidacy -- so it needed
   -munfold-inner-loops to fire at all, and silently inherited
   can_duplicate_loop_p, the hardware-loop exclusion and size limits sized for
   doubling code.  Measuring one transformation through another's eligibility
   is how the PolyBench run first came back with 10 loops instead of 380.  */

void
lvx_lms_pipeline_loops (void)
{
  /* The dump is one consumer and codegen is the other, so the walk runs for
     either.  Reporting below goes through lvx_lms_dump, which is null-safe:
     on the codegen path there is no dump file at all, and the claim that
     every fprintf was "already guarded" -- written here, unverified -- was
     false for fifteen of seventeen of them.  The compiler segfaulted inside
     vfprintf on the gate's own test file.  */
  if (!dump_file && !lvx_lms_codegen)
    return;

  /* Self-contained, so the probe does not depend on -msuperblock-loops having
     run and set up loops for its own phases.  It modifies nothing: the stride
     test does its own address recognition rather than calling iv_analyze, which
     would have wanted preheaders and a DF rebuild per loop.  */
  calculate_dominance_info (CDI_DOMINATORS);
  /* AVOID_CFG_MODIFICATIONS, FOR CODEGEN TOO, and §7.0 says why -- a point I
     had written down and then designed against.  The reference "finalizes loop
     info before any transformation, so every later phase works on the raw CFG,
     which is why all the edge manipulation is hand-rolled rather than using the
     loop helpers."

     Switching this to LOOPS_NORMAL instead was measured as an ICE: with all
     four trmm loops declining and make_nodes building nothing, the compiler
     still aborted in remove_insn via reorg_loops -> cleanup_cfg, so the flag
     alone was the cause.  LOOPS_NORMAL restructures the CFG to give every loop
     a preheader and a simple latch, and what that leaves behind is what the
     later cleanup cannot remove.

     So the transformation is deferred instead: the walk below only ANALYSES,
     records what each pipelineable loop needs as raw basic blocks, and
     lvx_lms_pipeline_loops finalizes the loop structures before building
     anything.  §7quinquies.10.  */
  loop_optimizer_init (AVOID_CFG_MODIFICATIONS);
  /* lvx_lms_build_graph reads df_get_live_in to find what a branch implicitly
     uses on its exit edge, so the liveness problem has to be solved first.  */
  df_analyze ();
  /* The arc latencies come from the DFA, which needs the insn-code cache the
     scheduler would otherwise own.  */
  dfa_start ();

  for (auto loop : loops_list (cfun, LI_ONLY_INNERMOST))
    {
      basic_block *body;
      unsigned insns = 0;
      const char *why = "";

      if (loop->latch == NULL)
	continue;

      /* §7tricies.153: the per-function companion to -mlms-only-loop, for
	 bisecting a wrong answer down to the function that produces it.  */
      if (lvx_lms_only_fn && lvx_lms_only_fn[0]
	  && !strstr (current_function_name (), lvx_lms_only_fn))
	continue;

      body = get_loop_body (loop);
      for (unsigned i = 0; i < loop->num_nodes; i++)
	{
	  rtx_insn *insn;
	  FOR_BB_INSNS (body[i], insn)
	    if (NONDEBUG_INSN_P (insn))
	      insns++;
	}

      if (!lvx_lms_eligible_loop_p (loop, body, insns, &why))
	lvx_lms_dump (";; lvx cms: fn=%s loop %d declined, %s\n",
		      current_function_name (), loop->num, why);
      else
	{
	  /* Can code be emitted for this loop, as against analysed?  Reported
	     rather than gated: the two predicates answer different questions
	     (§7quinquies.7), and a loop CMS can schedule but not yet emit for
	     is a codegen limit to size, not an analysis failure to hide.  */
	  const char *cwhy = NULL;
	  bool cfeas = lvx_lms_codegen_feasible_p (loop, body, &cwhy);
	  lvx_lms_dump (";; lvx cms: fn=%s loop %d codegen %s\n",
			current_function_name (), loop->num,
			cfeas ? "feasible" : cwhy);
	  lvx_lms_pipeline_loop (loop, body);
	}

      free (body);
    }

  dfa_finish ();
  loop_optimizer_finalize ();

  /* THE TIDY-UP reorg_loops USED TO PROVIDE.  §7tricies.196.

     CMS's emission creates blocks and redirects edges, leaving a stale
     dominator tree and -- the part that actually bites -- blocks no longer
     reachable.  While CMS ran BEFORE reorg_loops, that pass's own cleanup_cfg
     swept both; running after, nothing does, and the first symptom was
     verify_dominators ("dominator of 10 should be 7, not 9"), then
     calc_dfs_tree asserting outright on the unreachable blocks, across 127 of
     276 corpus files.

     After loop_optimizer_finalize, not before: it verifies the loop structure
     under checking and reads the tree, so the tree must still exist -- stale
     is survivable there, absent is not, which is the order CMS already relied
     on when it ran first.  */
  /* §7tricies.217: unconditional.  This ran only in the after order; the
     after order is now the only order.  */
  {
    basic_block bb;

      /* AND THE BRANCH PROBABILITIES, RECONCILED.  The surgery redirects
	 edges, and an edge created or retargeted here has no probability while
	 the jump that controls it still carries its REG_BR_PROB note -- which
	 verify_flow_info rejects outright, "REG_BR_PROB is set but cfg
	 probability is not", on eight PolyBench files.  The note is the
	 stale half: CMS sets the edges it creates to `even', so the profile is
	 already approximate, and dropping a note it cannot honour says so
	 rather than inventing a figure.  */
      FOR_EACH_BB_FN (bb, cfun)
	{
	  rtx_insn *j = BB_END (bb);
	  bool unknown = false;
	  edge e;
	  edge_iterator ei;

	  if (j == NULL || !JUMP_P (j)
	      || find_reg_note (j, REG_BR_PROB, NULL_RTX) == NULL_RTX)
	    continue;
	  FOR_EACH_EDGE (e, ei, bb->succs)
	    if (!e->probability.initialized_p ())
	      unknown = true;
	  if (unknown)
	    remove_note (j, find_reg_note (j, REG_BR_PROB, NULL_RTX));
	}

      free_dominance_info (CDI_DOMINATORS);
      cleanup_cfg (0);
    }

}

/* Decide whether unfolding LOOP can pay, by comparing what one iteration
   costs today against what two scheduled together could cost.

   With H the dependence height of the body -- its longest latency path over
   the intra-iteration arcs -- R the resource bound and K the recurrence
   bound, one iteration costs about max (H, R) today.  After the body is
   doubled the second copy cannot start before K, so the pair costs at least
   max (2R, K + H) and each iteration about half of that.  Requiring the
   second to be smaller reduces, in both the resource-bound and the
   dependence-bound case, to

       max (K, R) < H

   which is exactly what the parametric solver returns when seeded with R.
   The margin then asks that the saving be worth the doubled code.

   The dependence graph is built with the anti- and output dependences that
   register allocation introduced left out, since the renaming that
   -munfold-inner-loops implies removes them -- except where it cannot: a
   tied operand keeps its anti-dependence, and a guarded write or a partial
   write into a register tuple keeps its output dependence.

   The body is taken in CHAIN order -- from the header along each block's
   single in-loop successor, which the eligibility gate has established is
   well defined -- so a multi-block superblock body linearises and the
   iteration distance of an arc is simply whether it runs forward in that
   order.  Judging multi-block bodies is the whole point: those are the loops
   the superblock work exists for.

   One limit, deliberate: only register dependences are seen, so a loop bound
   by a memory recurrence has its K underestimated and may be unfolded when
   it should not.  Memory arcs want the scheduler's dependence machinery,
   which is not set up at this point in reorg.  */


/* True if innermost LOOP can be unfolded and measured.  This is deliberately
   NOT lvx_inner_superblock_loop_p: that one reproduces unfold_inner_find's
   shape, which demands every body block end in a conditional branch, because
   the software pipeliner's code generation needs an exit in every block.  An
   acyclic unfold needs no such thing -- a block with a single unconditional
   successor is perfectly good -- and importing the stricter rule refused
   loops that measured as gains, qsort_small's two among them.

   What is genuinely required: a single latch, so the duplication has one
   edge to work from; exactly one in-loop successor per block, so the chain
   walk covers the body and an arc's iteration distance is just whether it
   runs forward; no call, volatile reference or asm, none of which the cost
   model prices; no computed jump or complex edge, which duplication cannot
   handle; and the size bounds.  */



/* GIVE THE MAPPED LOOPS BACK THEIR SINGLE ENTRY.  §7tricies.216.

   `lvx_superblock_inner_loops' Phase A already straightens every innermost
   loop to a single-entry preheader, because "7.3% of innermost loops have a
   header with two or more out-of-loop predecessors and are refused outright
   by the pipeliner for it" -- but it runs BEFORE reorg_loops, and reorg_loops
   undoes it.  In the before order that does not matter: CMS has already run.
   In the AFTER order it is the whole of the throughput gap.  Measured on
   syrk at MINI/double: 6 pipelines in the before order, 4 in the after one,
   the two lost loops reported as

     fn=main loop 7 ENTRIES=2 *** NOT ONE ***
     fn=main loop 7 codegen declined, no preheader

   and the file 6.8% slower while emitting LESS code -- fewer bundles, fewer
   instructions, no pipeline.  Note loopdo=0 there: reorg_loops mapped nothing
   at all and still cost two pipelines, so this is its CFG work and not the
   mapping.

   NO cfg_layout_initialize HERE, deliberately, unlike Phase A.  Entering
   cfglayout after reorg_loops is what its own comment warns against -- the
   merge on finalisation is what displaced the LOOPDO end label (§7tricies.214)
   -- and splitting an entry edge needs none of it: `create_preheader' only
   inserts a block ahead of the header, leaving the body's layout and so
   LOOPDO's 17-bit end-address range untouched.  */

void
lvx_lms_restore_preheaders (void)
{
  int made = 0;

  if (crtl->has_bb_partition)
    return;

  loop_optimizer_init (AVOID_CFG_MODIFICATIONS);
  for (auto loop : loops_list (cfun, LI_ONLY_INNERMOST))
    /* create_preheader needs loop->latch, which a loop with several latches
       has not got -- and such a loop is refused anyway, for the back edges.  */
    if (loop->latch != NULL
	&& create_preheader (loop, CP_SIMPLE_PREHEADERS) != NULL)
      made++;
  loop_optimizer_finalize ();

  if (made)
    df_analyze ();
  if (dump_file && lvx_lms_probe)
    lvx_lms_dump (";; lvx cms: restored %d single-entry preheader%s after"
		  " reorg_loops\n", made, made == 1 ? "" : "s");
}
