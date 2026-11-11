/* Late Modulo Scheduling for LVX: the framework/engine interface.
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

/* LMS is the framework -- loop admission, the dependence graph, the bounds,
   the relaxations, the resource model, acceptance, live ranges, register
   allocation and pipeline construction.  An ENGINE is the part that chooses
   dates at a given initiation interval, and CMS -- convergent modulo
   scheduling, an unwinding constructor steered by Squeaky Wheel Optimization
   -- is the first of them; SMS and an integer program are intended to follow.

   This header is what an engine may see of the framework, and what the
   framework sees of an engine.  Everything here is deliberately small: a
   rule that is about the PROGRAM belongs in LMS and arrives as an arc or an
   acceptance refusal, so no engine has to know it exists.  */

#ifndef GCC_LVX_LMS_H
#define GCC_LVX_LMS_H

struct lvx_sched_resources
{
  const char *message;
  /* Instructions, counted two ways.  INSN_COUNT is rtl insns, which is what a
     loop-size cutoff means; NINSNS_COUNT is the instructions they issue, which
     is larger wherever a composite issue class covers several and is what the
     issue rate bounds.  */
  unsigned insn_count;
  unsigned ninsns_count;
  /* One accumulated demand per resource column of lvx_issue_table, named after
     the column.  The columns, their availabilities and this list all come from
     Resource.yml through MDS/BE/GCC, so there is no list to maintain here.  */
#define LVX_SCHED_RESOURCE_FIELD(R) unsigned R##_count;
  LVX_ISSUE_RESOURCES (LVX_SCHED_RESOURCE_FIELD)
#undef LVX_SCHED_RESOURCE_FIELD
};

/* One arc of the LMS dependence graph: a dependence from SRC to DST with
   latency THETA and iteration distance OMEGA, so the constraint it imposes on a
   modulo schedule at initiation interval II is

     date (DST) - date (SRC)  >=  THETA - OMEGA * II

   The graph is over the loop body ONCE -- one node per instruction -- with the
   distance carried explicitly here.  That is deliberately not the two-copy form
   of kvx-unfold.cc: a graph over a straightened pair of copies can represent
   neither a circuit (so its RecMII has to be recovered by tracing a spanning
   tree) nor a distance above one (so those dependences vanish rather than being
   approximated).  See docs/cms-foundations.md §7.7b and §7sexies.  */

/* Where an arc came from.  Carried so that the circuit which sets RecMII can
   be named rather than merely measured: a recurrence through a register RAW
   chain is the program's own arithmetic and nothing but algebra will shorten
   it, whereas one through a memory arc or a control arc may be an artefact of
   what the compiler could not prove, and is then worth attacking.  The order
   is the order the builder creates them in.  */

enum lvx_lms_arc_kind
{
  LVX_ARC_REG_RAW,	/* register true dependence; the only latency-bearing kind */
  LVX_ARC_REG_WAR,	/* register anti, kept only where renaming cannot remove it */
  LVX_ARC_REG_WAW,	/* register output, same  */
  LVX_ARC_MEM_INTRA,	/* memory dependence inside one iteration */
  LVX_ARC_MEM_CROSS,	/* memory dependence across the back edge, omega = safelen */
  LVX_ARC_CTRL_ORDER,	/* one branch precedes the next in the iteration */
  LVX_ARC_CTRL_STORE,	/* a branch precedes a store or volatile reference */
  LVX_ARC_CTRL_LIVEOUT,	/* a branch precedes a def live out on its exit edge */
  LVX_ARC_CTRL_BACKEDGE	/* the last branch precedes the first, omega = 1 */
};

struct lvx_lms_arc
{
  unsigned src;
  unsigned dst;
  HOST_WIDE_INT theta;
  HOST_WIDE_INT omega;
  enum lvx_lms_arc_kind kind;
  /* For a REG_RAW arc: WHICH operand of the consumer reads the value, as an
     index into a deterministic sub-rtx walk of its pattern.  -1 otherwise.

     The rename rewrite used to find the operand by REGISTER NUMBER, and that
     is not a key: register allocation maps many values onto one hard
     register, so one instruction can mention the same regno for two
     different classes -- `fixedd.rz r0 = r0', an instruction reading two
     classes both originally in r0, or `sd $r3[$r6] = ...' where r3 is both a
     renamed value's original and an index belonging to something else.  The
     rewrite then put the wrong register in, and §7tricies.22 declined 72 of
     119 loops rather than risk it.

     DF knows exactly which reference is which -- DF_REF_LOC points at the
     rtx inside the pattern -- and the arc builder is already walking those
     refs when it decides the producer.  Recording the position there turns
     "every REG with this number" into "this operand", and the ambiguity
     cannot arise.

     An INDEX rather than the pointer, because the rewrite runs on a COPY of
     the instruction: a pointer into the original addresses nothing in the
     copy, while the traversal order is structural and identical in both.  */
  int useidx;
};

/* The BCU prefix kinds that share a bundle's two BCU syllables.  Tagging each
   occupied slot with its kind keeps a MASKS mask keyed on $rN from aliasing a
   GUARD condition, or a MASKM byte-enable, on the same register: a slot is
   shared only when kind, lanetodo and the key register all match.  */
enum lvx_bcu_kind { BCU_PLAIN, BCU_GUARD, BCU_MASKS, BCU_MASKM };

/* A memory reference's address as an affine function of the iteration number,
   SYM + OFF + STEP * k over SIZE bytes.  */
struct lvx_mem_iv
{
  rtx sym;
  HOST_WIDE_INT off;
  HOST_WIDE_INT step;
  HOST_WIDE_INT size;
  bool valid;
};

/* The framework, as an engine sees it.  */

extern const char *
lvx_lms_accept_p (const auto_vec<rtx_insn *> &seq,
		  const auto_vec<lvx_lms_arc> &arcs,
		  int ii, const auto_vec<int> &sigma, bitmap livein,
		  auto_vec<int> *blame, auto_vec<int> *culprit);

extern bool
lvx_lms_bounds (class loop *loop, basic_block *body,
		const auto_vec<rtx_insn *> &seq,
		const auto_vec<lvx_lms_arc> &arcs,
		const auto_vec<int> &height, int *pm, int *pr, int *pk,
		auto_vec<int> *crit = NULL);

extern int
lvx_lms_dates_in_window (const auto_vec<rtx_insn *> &seq,
			 const auto_vec<lvx_lms_arc> &arcs,
			 const auto_vec<int> &dates,
			 const auto_vec<int> &tosink,
			 int ii, int *first_out, int *makespan_out,
			 int *empty_out, auto_vec<int> *estart_out = NULL,
			 auto_vec<int> *lstart_out = NULL,
			 int makespan_floor = 0);

extern void ATTRIBUTE_PRINTF_1
lvx_lms_dump (const char *fmt, ...);

extern int
lvx_lms_latency (rtx_insn *insn);

extern enum lvx_network_result
lvx_lms_mcrl_dates (const auto_vec<rtx_insn *> &seq,
		    const auto_vec<lvx_lms_arc> &arcs,
		    const auto_vec<int> &estart,
		    const auto_vec<int> &lstart,
		    int ii, auto_vec<int> &dates,
		    HOST_WIDE_INT *cost_out, unsigned *nn_out,
		    unsigned *ne_out);

extern unsigned
lvx_lms_mrt_trial (auto_vec<struct lvx_sched_resources> &mrt,
		   const auto_vec<int> &mrow, rtx_insn *insn, unsigned i,
		   int row);

extern unsigned
lvx_lms_mrt_violations (const auto_vec<struct lvx_sched_resources> &mrt);

extern void
lvx_lms_path_metrics (const auto_vec<rtx_insn *> &seq,
		      const auto_vec<lvx_lms_arc> &arcs,
		      const auto_vec<int> &asap,
		      auto_vec<int> &tosink, auto_vec<int> &slack);

extern void
lvx_lms_resources_add (struct lvx_sched_resources *resources, rtx_insn *insn);

extern void
lvx_lms_resources_sub (struct lvx_sched_resources *resources, rtx_insn *insn);

extern int
lvx_lms_sigma_span (const auto_vec<int> &sigma);

extern bool
lvx_lms_slot_fits_p (const struct lvx_sched_resources *slot, rtx_insn *insn);

extern const char *lvx_lms_verify (const auto_vec<rtx_insn *> &,
				   const auto_vec<lvx_lms_arc> &, int,
				   const auto_vec<int> &,
				   auto_vec<int> * = NULL);

/* THE PROBLEM ONE ENGINE IS ASKED TO SCHEDULE.  §7tricies.224.

   Everything an engine needs about the loop, and nothing about how to search:
   the body in chain order, the arcs with their (theta, omega), the
   intra-iteration dependence depth, the circuit that set RecMII, the
   registers live into the header, and the unwinding budget Delta.  An
   engine's own knobs are its own options, read where it reads them.  */

struct lvx_lms_problem
{
  const auto_vec<rtx_insn *> *seq;
  const auto_vec<lvx_lms_arc> *arcs;
  const auto_vec<int> *height;
  const auto_vec<int> *crit;
  bitmap livein;
  int delta;
};

/* AN ENGINE, WHICH IS ASKED FOR A SCHEDULE AT A GIVEN II AND NOTHING ELSE.

   Two phases, because the ordering an engine computes is a property of the
   LOOP and not of the period: SMS's cost argument is that the nodes are
   ordered once however many initiation intervals are tried, and CMS has its
   own per-loop work (the path metrics) that was being redone at every II.
   PREPARE returns an opaque state, SCHEDULE_AT_II is called once per
   candidate interval, FINISH releases it.

   SCHEDULE_AT_II returns true with SIGMA filled, having already satisfied
   lvx_lms_accept_p; on failure it leaves WHY naming the last obstacle.  Any
   search of its own -- CMS's Squeaky Wheel passes and its two MCRL arms, an
   integer program's branch and bound -- happens inside it, so that two
   engines asked for the same II differ in one thing only.  PASSES accumulates
   whatever the engine wants to report as work done.  */

struct lvx_lms_engine
{
  const char *name;
  void *(*prepare) (const struct lvx_lms_problem *);
  bool (*schedule_at_ii) (void *state, int ii, auto_vec<int> &sigma,
			  const char **why, int *passes);
  void (*finish) (void *state);
};

/* The engines.  */

extern const struct lvx_lms_engine lvx_cms_engine;

extern bool
lvx_cms_swo_at_ii (const auto_vec<rtx_insn *> &seq,
		   const auto_vec<lvx_lms_arc> &arcs,
		   const auto_vec<int> &height,
		   int ii, int delta, auto_vec<int> &sigma,
		   int budget, int *passes, const char **last_bad,
		   int pin, int mcrl_release = -1,
		   bitmap livein = NULL,
		   const auto_vec<int> *crit = NULL);

/* Helpers the framework shares with the rest of the back end.  */

extern int
lvx_arc_latency (rtx_insn *prod, rtx_insn *cons, enum reg_note dep_type);

extern bool
lvx_doloop_end_p (rtx_insn *insn);

extern bool
lvx_fp_insn_p (rtx_insn *insn);

extern rtx
lvx_hwloop_pattern_reg (rtx_insn *insn);

extern bool
lvx_insn_defs_regno_p (rtx_insn *insn, unsigned regno, bool *partial = NULL);

extern bool
lvx_insn_dest_earlyclobber_p (rtx_insn *insn);

extern bool
lvx_insn_has_tied_operand_p (rtx_insn *insn);

extern bool
lvx_insns_may_alias_p (rtx_insn *insn1, rtx_insn *insn2);

extern bool
lvx_load_class_p (enum attr_type type);

extern bool
lvx_loop_has_doloop_p (class loop *loop);

extern bool
lvx_masked_prefix (rtx_insn *insn, enum lvx_bcu_kind *kind, rtx *mask,
		   int *lanetodo);

extern bool
lvx_mem_read_insn_p (rtx_insn *insn);

extern bool
lvx_mem_write_insn_p (rtx_insn *insn);

extern bool
lvx_mems_disjoint_p (const vec<rtx_insn *> &seq, unsigned p, unsigned q);

extern int
lvx_sched_issue_rate (void);

extern void
lvx_sched_resources_add (struct lvx_sched_resources *resources, rtx_insn *insn);

extern unsigned
lvx_sched_resources_full_bundles (struct lvx_sched_resources *resources);

extern bool
lvx_speculatable_p (rtx_insn *insn);

extern rtx
lvx_split_plus_const (rtx x, HOST_WIDE_INT *c);

extern void
lvx_lms_flow_record (rtx_insn *insn, int root, int reg, int nregs, bool is_def,
		     bool dly, int p2);

extern void
lvx_lms_group_tag (rtx_insn *after, int node, int date, int key);

extern int lvx_lms_flow_region;

/* The framework, as the back end sees it.  */

extern void lvx_lms_probe_ddg (struct ddg *, int);

extern void
lvx_lms_dump_cfg (const char *when);

extern int
lvx_lms_load_latency_delta (rtx_insn *insn);

extern void
lvx_lms_pipeline_loops (void);

extern void
lvx_lms_restore_preheaders (void);

#endif /* GCC_LVX_LMS_H */
