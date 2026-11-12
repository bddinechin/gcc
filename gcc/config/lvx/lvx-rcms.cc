/* Resource-Constrained Modulo Scheduling for LVX: one engine of the LMS
   framework.
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

/* RCMS -- THE MODULO SCHEDULE AS AN INTEGER PROGRAM.

   WHAT IT IS.  The time-indexed formulation of Dupont de Dinechin's
   habilitation, chapter 2: a binary x_i^t per (operation, date in its
   window), one assignment row per operation, one row per dependence, and
   one row per (resource, residue) -- (2.17).  At a FIXED lambda the whole
   thing is linear, which is what the engine seam buys: the II is a constant
   by the time the model is written, so lambda*omega and lambda*k are
   coefficients rather than products of variables.

   WHAT IT IS A PORT OF.  LAO's Formulation.c, the author's own
   implementation (kv4-csw/lao/LAO/PRO/SCD), specifically its `Dinechin'
   arm -- buildVariables/Objective/Assignments/Dependences/Resources/Bounds.
   The correspondences are exact and worth naming, because they are what
   makes this a port rather than a re-derivation:

     IssueTable_availability      lvx_issue_availability
     IssueItem_REQUIREMENTS       lvx_lms_resources_add into a zeroed vector
     DDGraphArc THETA/OMEGA       lvx_lms_arc theta/omega
     earlyDates/lateDates         lvx_rcms_windows, below
     Formulation_buildResources   lvx_rcms_emit, the (resource, residue) rows

   WHAT IS DELIBERATELY NOT PORTED.  LAO owns its own ladder --
   Formulation_Schedule tries lambda_min, then lambda_max, then bisects --
   and ours does not: the LMS driver walks the interval and asks each engine
   `schedule_at_ii', so RCMS answers one lambda and keeps no search of its
   own.  That is the same contract SMS honours, and it is what lets the
   three engines be compared on one corpus.  LAO's Large Neighbourhood
   Search sits in that same ladder and goes with it; the margins it needs
   are a window narrowing, which -mlms-rcms-margin reaches when wanted.

   THE LIFETIME VARIABLES ARE OUT, on purpose.  A killer node per value
   doubles the binaries and contributes nothing to (2.17); the objective
   here is the makespan, which needs ONE extra variable rather than one per
   value.  See docs/modulo-scheduler-interface.md section 9.1.

   WHY THE SOLVER IS A SEPARATE PROCESS.  No library is linked into cc1:
   no configure dependency, no licence question, and the model file is an
   artefact that can be archived beside a measurement and re-solved by a
   DIFFERENT solver, which is the only way to tell a formulation bug from a
   solver bug.  The compiler writes CPLEX LP, execs a wrapper, and reads a
   status line and name/value pairs back.

   AND WHY THAT IS SAFE.  Whatever the solver returns goes through
   lvx_lms_accept_p exactly like any other engine's answer -- verified
   against every arc and the modulo reservation table, then coloured.  A
   wrong solution cannot produce wrong code, only a refusal.  That property
   is what lets an external, untrusted, possibly buggy program sit inside
   the compiler at all.

   HOW THE FORMULATION IS CHECKED WITHOUT A SOLVER.  -mlms-rcms-check takes
   the schedule another engine produced at this lambda and tests it against
   every row this file would emit.  A schedule lvx_lms_accept_p has blessed
   MUST be feasible in the model; if the model rejects it, the model is
   wrong -- a sign error, a residue off by one, a window too tight.  That is
   the control arm, it needs no solver, and it is what says the emitter is
   right before any optimality claim is made.  */

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
/* THE INCLUDE ORDER MATTERS at the tail: lvx-lms.h builds struct
   lvx_sched_resources by expanding LVX_ISSUE_RESOURCES, which
   lvx-issue-table.inc defines.  The other way round the struct comes out
   with no fields at all and every per-column accessor below fails on a
   member that should exist.  Same order as lvx-cms.cc and lvx-sms.cc.  */

/* A date beyond which no window is allowed to run.  The horizon is derived
   from the critical path, so this only bounds a pathological graph.  */
#define LVX_RCMS_MAX_HORIZON 4096

/* Per-operation window, recomputed at every lambda.  */
struct lvx_rcms_window
{
  int lo;			/* earliest date, LAO's earlyDates */
  int hi;			/* latest date, LAO's lateDates */
};

struct lvx_rcms_state
{
  const struct lvx_lms_problem *p;
  unsigned n;
  /* b_i, one demand vector per operation, charged exactly as every other
     engine charges it -- lvx_lms_resources_add, prefix included.  */
  auto_vec<struct lvx_sched_resources> demand;
  /* Recomputed per lambda.  */
  auto_vec<lvx_rcms_window> win;
  int horizon;
  /* Reported back to the driver.  */
  bool ok;
};

/* One accessor per resource column, so that every place below which must
   stay in step with Resource.yml is a use of LVX_ISSUE_RESOURCES and not a
   list to maintain.  */
#define LVX_RCMS_ACCESSOR(R)						\
  static unsigned							\
  lvx_rcms_demand_##R (const struct lvx_sched_resources *s)		\
  { return s->R##_count; }
LVX_ISSUE_RESOURCES (LVX_RCMS_ACCESSOR)
#undef LVX_RCMS_ACCESSOR

/* The latency an operation occupies for the makespan rows: p_i of (2.1).  */

static int
lvx_rcms_plength (rtx_insn *insn)
{
  int l = lvx_lms_latency (insn);
  return l > 0 ? l : 1;
}

/* THE WINDOWS, LAO's earlyDates/lateDates.

   Longest path from a virtual source gives the earliest date, longest path
   to a virtual sink gives the latest -- both over the arc weights
   theta - lambda*omega, which is where the II enters and why this is
   recomputed per lambda rather than once.  Bellman-Ford rather than
   all-pairs: LAO computes the full matrix because its LNS needs pairwise
   margins, and a single source is all the windows need.

   A positive cycle means lambda is below the graph's own period and there
   is nothing to solve.  That is a REFUSAL, not an error: the ladder simply
   moves on.  */

static bool
lvx_rcms_windows (struct lvx_rcms_state *st, int ii, const char **why)
{
  const struct lvx_lms_problem *p = st->p;
  const auto_vec<lvx_lms_arc> &arcs = *p->arcs;
  const auto_vec<rtx_insn *> &seq = *p->seq;
  unsigned n = st->n, i;
  auto_vec<int> asap, alap;

  asap.safe_grow_cleared (n);
  alap.safe_grow_cleared (n);

  /* Earliest: relax until fixpoint, at most n passes.  An n+1'th change is
     a positive cycle.  */
  for (unsigned pass = 0; pass <= n; pass++)
    {
      bool moved = false;
      for (unsigned a = 0; a < arcs.length (); a++)
	{
	  const lvx_lms_arc *ar = &arcs[a];
	  int w = (int) (ar->theta - (HOST_WIDE_INT) ii * ar->omega);
	  if (ar->src >= n || ar->dst >= n)
	    continue;
	  if (asap[ar->src] + w > asap[ar->dst])
	    {
	      asap[ar->dst] = asap[ar->src] + w;
	      moved = true;
	    }
	}
      if (!moved)
	break;
      if (pass == n)
	{
	  *why = "rcms: positive cycle, ii below the graph period";
	  return false;
	}
    }

  /* The horizon: the critical path, plus whatever slack the margin asks
     for.  A tighter horizon is a smaller model and a weaker guarantee --
     it can make a feasible lambda look infeasible -- so the default adds
     one full interval, which is what lets an operation be placed a stage
     later than its earliest.  */
  int h = 0;
  for (i = 0; i < n; i++)
    h = MAX (h, asap[i] + lvx_rcms_plength (seq[i]));
  h += MAX (0, lvx_lms_rcms_margin) * ii;
  if (h > LVX_RCMS_MAX_HORIZON)
    {
      *why = "rcms: horizon too large";
      return false;
    }
  st->horizon = h;

  /* Latest: longest path to the sink, from the horizon back.  */
  for (i = 0; i < n; i++)
    alap[i] = h - lvx_rcms_plength (seq[i]);
  for (unsigned pass = 0; pass <= n; pass++)
    {
      bool moved = false;
      for (unsigned a = 0; a < arcs.length (); a++)
	{
	  const lvx_lms_arc *ar = &arcs[a];
	  int w = (int) (ar->theta - (HOST_WIDE_INT) ii * ar->omega);
	  if (ar->src >= n || ar->dst >= n)
	    continue;
	  if (alap[ar->dst] - w < alap[ar->src])
	    {
	      alap[ar->src] = alap[ar->dst] - w;
	      moved = true;
	    }
	}
      if (!moved)
	break;
      if (pass == n)
	{
	  *why = "rcms: positive cycle on the backward pass";
	  return false;
	}
    }

  st->win.truncate (0);
  st->win.safe_grow_cleared (n);
  for (i = 0; i < n; i++)
    {
      st->win[i].lo = asap[i];
      st->win[i].hi = alap[i];
      if (st->win[i].hi < st->win[i].lo)
	{
	  *why = "rcms: empty window";
	  return false;
	}
    }
  return true;
}

/* THE MODEL, WRITTEN ONCE AND READ TWO WAYS.

   With F non-null the rows are emitted as CPLEX LP; with CHECK non-null the
   same rows are tested against a schedule another engine produced.  One
   function rather than two on purpose: a checker that walks its own idea of
   the model checks nothing.  Every row this emits is a row that gets
   verified, and a divergence between them cannot hide.  */

static bool
lvx_rcms_model (struct lvx_rcms_state *st, int ii, FILE *f,
		const auto_vec<int> *check, const char **why)
{
  const struct lvx_lms_problem *p = st->p;
  const auto_vec<lvx_lms_arc> &arcs = *p->arcs;
  const auto_vec<rtx_insn *> &seq = *p->seq;
  unsigned n = st->n, i;
  int t;

  /* ---- the objective: minimise the makespan.  (2.1), one sink variable
     and not one killer per value.  ---- */
  if (f)
    fprintf (f, "\\ RCMS fn=%s ii=%d n=%u horizon=%d\n",
	     current_function_name (), ii, n, st->horizon);
  /* THE OBJECTIVE IS A LEVER, AND IT HAD BETTER BE.  §7tricies.229.
     Minimising the makespan compresses the schedule, and a compressed
     modulo schedule holds MORE values in flight at once -- that is what
     instruction-level parallelism is.  Measured on polybench, the commonest
     reason a solved RCMS schedule was thrown away is "kernel not
     colourable": the optimum of (2.1) is a schedule the register allocator
     cannot realise.  So the objective that looks obviously right is the one
     fighting the allocator, and section 9.2's order -- feasibility FIRST,
     makespan second -- is not a formality.

     0 asks for feasibility alone (gamma = . in the habilitation's
     notation): the zero objective, so any schedule satisfying every row
     will do.  1 minimises the makespan.  Both are solved over exactly the
     same rows, which is what makes the difference attributable.  */
  /* Which operations define a value some other operation reads?  A REG_RAW
     arc IS that relation, so the lifetime variables need no new analysis:
     one killer per producer, bounded below by each consumer's date.

     AND EACH ONE IS WEIGHTED BY THE WIDTH OF THE TUPLE IT PRODUCES.
     §7tricies.230.  A 128-bit value lives in an aligned GPR PAIR and a
     256-bit one in a QUAD, so a value is not one register and a lifetime is
     not one register-cycle: what the allocator has to fit is
     nregs * (killer - definition).  Weighting every value 1 prices a quad
     like a single GPR and so under-prices precisely the values whose
     lifetimes hurt most.

     This is the author's adjustment of 2026-10-27, already in the
     framework's own MCRL relaxation -- lvx_lms_mcrl_dates makes the flow
     leaving a producer node NREGS (i) for exactly this reason -- and it is
     the main difference from the LAO equations.  The criterion for which
     producers count, and how their width is read, is the same one
     lvx_lms_mcrl_dates applies, so the ILP and the relaxation price the
     same quantity.  */
  auto_vec<int> nregs;
  bool any_life = false;
  nregs.safe_grow_cleared (n);
  for (i = 0; i < n; i++)
    nregs[i] = 0;
  for (unsigned a = 0; a < arcs.length (); a++)
    if (arcs[a].kind == LVX_ARC_REG_RAW
	&& arcs[a].src < n && arcs[a].dst < n
	&& nregs[arcs[a].src] == 0)
      {
	rtx ds = single_set (seq[arcs[a].src]);
	int nr = (ds && REG_P (SET_DEST (ds))) ? REG_NREGS (SET_DEST (ds)) : 1;

	nregs[arcs[a].src] = MAX (nr, 1);
	any_life = true;
      }

  const bool mcrl = (lvx_lms_rcms_objective == 2 && any_life);
  const bool want_span = !mcrl;

  if (f)
    {
      if (mcrl)
	{
	  /* MINIMUM CUMULATIVE REGISTER LIFETIME.  Sum over producers of
	     (killer - definition), which is what the register allocator
	     actually has to fit.  The makespan objective minimises the
	     opposite quantity in effect: a shorter schedule at the same
	     interval is a MORE parallel one, so it holds more values in
	     flight, and measurement said the commonest reason a solved
	     schedule was thrown away is that its kernel would not colour.  */
	  fprintf (f, "Minimize\n obj:");
	  for (i = 0; i < n; i++)
	    if (nregs[i])
	      fprintf (f, " + %d k_%u", nregs[i], i);
	  for (i = 0; i < n; i++)
	    if (nregs[i])
	      for (t = st->win[i].lo; t <= st->win[i].hi; t++)
		if (t != 0)
		  fprintf (f, " %+d x_%u_%d", -nregs[i] * t, i, t);
	  fprintf (f, "\nSubject To\n");
	}
      else
	fprintf (f, "Minimize\n obj: %sspan\nSubject To\n",
		 lvx_lms_rcms_objective == 0 ? "0 " : "");
    }

  /* ---- assignment: sum_t x_i^t = 1 ---- */
  for (i = 0; i < n; i++)
    {
      if (check)
	{
	  int s = (*check)[i];
	  if (s < st->win[i].lo || s > st->win[i].hi)
	    {
	      *why = "rcms check: date outside the window";
	      return false;
	    }
	}
      if (f)
	{
	  fprintf (f, " asg_%u:", i);
	  for (t = st->win[i].lo; t <= st->win[i].hi; t++)
	    fprintf (f, " + x_%u_%d", i, t);
	  fprintf (f, " = 1\n");
	}
    }

  /* ---- dependences: sigma_j - sigma_i >= theta - ii*omega, written as
     sum_t t x_i^t - sum_t t x_j^t <= -(theta - ii*omega).  LAO's
     buildDependencesDinechin, aggregated form.  ---- */
  for (unsigned a = 0; a < arcs.length (); a++)
    {
      const lvx_lms_arc *ar = &arcs[a];
      int w = (int) (ar->theta - (HOST_WIDE_INT) ii * ar->omega);

      if (ar->src >= n || ar->dst >= n)
	continue;

      if (ar->src == ar->dst)
	{
	  /* A self arc carries no variables: it is satisfiable iff its own
	     weight is not positive.  Emitting an empty row would be an LP
	     syntax error, and silently dropping it would lose a real
	     infeasibility.  */
	  if (w > 0)
	    {
	      *why = "rcms: positive self arc";
	      return false;
	    }
	  continue;
	}

      if (check)
	{
	  int si = (*check)[ar->src], sj = (*check)[ar->dst];
	  if (sj - si < w)
	    {
	      *why = "rcms check: dependence violated";
	      return false;
	    }
	}
      if (f)
	{
	  fprintf (f, " dep_%u:", a);
	  for (t = st->win[ar->src].lo; t <= st->win[ar->src].hi; t++)
	    if (t != 0)
	      fprintf (f, " %+d x_%u_%d", t, ar->src, t);
	  for (t = st->win[ar->dst].lo; t <= st->win[ar->dst].hi; t++)
	    if (t != 0)
	      fprintf (f, " %+d x_%u_%d", -t, ar->dst, t);
	  fprintf (f, " <= %d\n", -w);
	}
    }

  /* ---- resources: one row per (column, residue).  (2.17), and exact
     rather than conservative because the stage offsets are uniform, so
     charging at issue only relabels the residues -- see
     docs/modulo-scheduler-interface.md section 3.3a.  ---- */
#define LVX_RCMS_RES_ROWS(R)						\
  do {									\
    unsigned avail = lvx_issue_availability.R;				\
    unsigned total = 0;							\
    for (i = 0; i < n; i++)						\
      total += lvx_rcms_demand_##R (&st->demand[i]);			\
    if (total != 0 && avail != 0)					\
      for (int tau = 0; tau < ii; tau++)				\
	{								\
	  unsigned most = 0, used = 0;					\
	  for (i = 0; i < n; i++)					\
	    {								\
	      unsigned b = lvx_rcms_demand_##R (&st->demand[i]);	\
	      if (b == 0)						\
		continue;						\
	      for (t = st->win[i].lo; t <= st->win[i].hi; t++)		\
		if (((t % ii) + ii) % ii == tau)			\
		  { most += b; break; }					\
	      if (check && ((((*check)[i] % ii) + ii) % ii) == tau)	\
		used += b;						\
	    }								\
	  if (check && used > avail)					\
	    { *why = "rcms check: " #R " oversubscribed"; return false; }\
	  /* LAO's kill: a row whose worst case fits can never bind.  */	\
	  if (f && most > avail)					\
	    {								\
	      fprintf (f, " res_%s_%d:", #R, tau);			\
	      for (i = 0; i < n; i++)					\
		{							\
		  unsigned b = lvx_rcms_demand_##R (&st->demand[i]);	\
		  if (b == 0)						\
		    continue;						\
		  for (t = st->win[i].lo; t <= st->win[i].hi; t++)	\
		    if (((t % ii) + ii) % ii == tau)			\
		      fprintf (f, " + %u x_%u_%d", b, i, t);		\
		}							\
	      fprintf (f, " <= %u\n", avail);				\
	    }								\
	}								\
  } while (0);
  LVX_ISSUE_RESOURCES (LVX_RCMS_RES_ROWS)
#undef LVX_RCMS_RES_ROWS

  /* ---- the lifetime rows: k_i >= sigma_j + ii*omega for every RAW arc
     i -> j.  A use in a later iteration keeps the value alive a whole
     interval longer, which is what the omega term is; dropping it would
     price a loop-carried value as though it died in its own iteration.
     n general integers and |RAW arcs| rows -- the cheap encoding of
     section 9.1, not a time-indexed node per value.  ---- */
  if (f && mcrl)
    for (unsigned a = 0; a < arcs.length (); a++)
      {
	const lvx_lms_arc *ar = &arcs[a];

	if (ar->kind != LVX_ARC_REG_RAW || ar->src >= n || ar->dst >= n
	    || nregs[ar->src] == 0)
	  continue;
	fprintf (f, " life_%u: k_%u", a, ar->src);
	for (t = st->win[ar->dst].lo; t <= st->win[ar->dst].hi; t++)
	  if (t != 0)
	    fprintf (f, " %+d x_%u_%d", -t, ar->dst, t);
	fprintf (f, " >= " HOST_WIDE_INT_PRINT_DEC "\n",
		 (HOST_WIDE_INT) ii * ar->omega);
      }

  /* ---- the makespan rows: span >= sigma_i + p_i.  One dummy sink with an
     arc from every operation, which is why this objective costs one
     variable where the cumulative-lifetime one costs a window per value.
     ---- */
  if (f && want_span)
    for (i = 0; i < n; i++)
      {
	fprintf (f, " mk_%u: span", i);
	for (t = st->win[i].lo; t <= st->win[i].hi; t++)
	  if (t != 0)
	    fprintf (f, " %+d x_%u_%d", -t, i, t);
	fprintf (f, " >= %d\n", lvx_rcms_plength (seq[i]));
      }

  if (f)
    {
      fprintf (f, "Bounds\n");
      if (want_span)
	fprintf (f, " 0 <= span <= %d\n", st->horizon + 1);
      if (mcrl)
	for (i = 0; i < n; i++)
	  if (nregs[i])
	    fprintf (f, " 0 <= k_%u <= %d\n", i, st->horizon + ii + 1);
      fprintf (f, "General\n");
      if (want_span)
	fprintf (f, " span\n");
      if (mcrl)
	for (i = 0; i < n; i++)
	  if (nregs[i])
	    fprintf (f, " k_%u\n", i);
      fprintf (f, "Binaries\n");
      for (i = 0; i < n; i++)
	for (t = st->win[i].lo; t <= st->win[i].hi; t++)
	  fprintf (f, " x_%u_%d\n", i, t);
      fprintf (f, "End\n");
    }
  return true;
}

/* Run the wrapper.  The compiler never learns any solver's flag spelling:
   the option names a PROGRAM, called as <prog> <model> <solution> <seconds>,
   and validation/cms carries one script per solver.  A new solver is a new
   script rather than a new compiler.  */

static bool
lvx_rcms_solve (const char *model, const char *sol, const char **why)
{
  char secs[32];
  char *argv[5];
  int status = 0, err = 0;
  const char *msg;

  if (lvx_lms_rcms_solver == NULL || lvx_lms_rcms_solver[0] == '\0')
    {
      *why = "rcms: no solver configured (-mlms-rcms-solver=)";
      return false;
    }

  sprintf (secs, "%d", MAX (1, lvx_lms_rcms_seconds));
  argv[0] = const_cast<char *> (lvx_lms_rcms_solver);
  argv[1] = const_cast<char *> (model);
  argv[2] = const_cast<char *> (sol);
  argv[3] = secs;
  argv[4] = NULL;

  msg = pex_one (PEX_SEARCH | PEX_LAST, lvx_lms_rcms_solver, argv,
		 "lvx-rcms", NULL, NULL, &status, &err);
  if (msg != NULL)
    {
      *why = "rcms: could not run the solver wrapper";
      return false;
    }
  if (status != 0)
    {
      *why = "rcms: solver wrapper exited non-zero";
      return false;
    }
  return true;
}

/* Read the wrapper's answer: a status line, then name/value pairs.  The four
   words are the whole vocabulary the compiler depends on, and INFEASIBLE
   must stay distinct from UNKNOWN -- feasibility at a fixed lambda is
   NP-complete, so a solver that returns nothing has not proved anything, and
   collapsing the two would turn the ladder's minimum into an upper bound
   wearing a minimum's name.  */

static bool
lvx_rcms_read (struct lvx_rcms_state *st, const char *sol,
	       auto_vec<int> &sigma, const char **why)
{
  FILE *f = fopen (sol, "r");
  char line[512];
  unsigned n = st->n, i;
  auto_vec<int> got;
  bool have_status = false, usable = false;

  if (f == NULL)
    {
      *why = "rcms: no solution file";
      return false;
    }

  got.safe_grow_cleared (n);
  for (i = 0; i < n; i++)
    got[i] = -1;

  while (fgets (line, sizeof (line), f) != NULL)
    {
      char name[256];
      double v;

      if (!have_status)
	{
	  char word[64];
	  if (sscanf (line, "STATUS %63s", word) == 1)
	    {
	      have_status = true;
	      if (!strcmp (word, "OPTIMAL") || !strcmp (word, "FEASIBLE"))
		usable = true;
	      else if (!strcmp (word, "INFEASIBLE"))
		*why = "rcms: infeasible at this ii (proved)";
	      else
		*why = "rcms: solver returned no answer (unknown)";
	      continue;
	    }
	}
      if (sscanf (line, "%255s %lf", name, &v) == 2 && v >= 0.5)
	{
	  unsigned idx;
	  int t;
	  if (sscanf (name, "x_%u_%d", &idx, &t) == 2 && idx < n)
	    {
	      if (got[idx] >= 0)
		{
		  fclose (f);
		  *why = "rcms: two dates for one operation";
		  return false;
		}
	      got[idx] = t;
	    }
	}
    }
  fclose (f);

  if (!have_status)
    {
      *why = "rcms: solution file has no STATUS line";
      return false;
    }
  if (!usable)
    return false;

  for (i = 0; i < n; i++)
    if (got[i] < 0)
      {
	*why = "rcms: solution does not place every operation";
	return false;
      }

  sigma.truncate (0);
  sigma.safe_grow_cleared (n);
  for (i = 0; i < n; i++)
    sigma[i] = got[i];
  return true;
}

/* THE CONTROL ARM, AND IT NEEDS NO SOLVER.  §7tricies.228.

   CMS's own search produces a schedule at this lambda; a schedule the
   framework would accept MUST satisfy every row this file emits.  Checking
   that catches the bugs that actually kill a formulation -- a sign the
   wrong way round, a residue off by one, a window a cycle too tight --
   before any optimality claim rests on it, and it catches them on the real
   corpus rather than on a hand-written example.

   A failure here is a bug in the MODEL, not in CMS: lvx_lms_accept_p has
   already verified the schedule against every arc and the reservation
   table.  That asymmetry is what makes the test worth having.  */

static void
lvx_rcms_selfcheck (struct lvx_rcms_state *st, int ii)
{
  const struct lvx_lms_problem *p = st->p;
  auto_vec<int> probe;
  const char *last_bad = NULL;
  int passes = 0;
  const char *why = NULL;

  if (!lvx_cms_swo_at_ii (*p->seq, *p->arcs, *p->height, ii, p->delta, probe,
			  1, &passes, &last_bad, 0, -1, p->livein, p->crit))
    {
      if (lvx_lms_rcms_trace)
	lvx_lms_dump (";; lvx rcms:   check ii=%d: cms found nothing to"
		      " check\n", ii);
      return;
    }

  auto_vec<int> cblame, cculprit;
  cblame.safe_grow_cleared (st->n);
  cculprit.safe_grow_cleared (st->n);
  const char *acc = lvx_lms_accept_p (*p->seq, *p->arcs, ii, probe,
				      p->livein, &cblame, &cculprit);
  if (acc != NULL)
    {
      if (lvx_lms_rcms_trace)
	lvx_lms_dump (";; lvx rcms:   check ii=%d: cms schedule not accepted"
		      " (%s), not a model test\n", ii, acc);
      return;
    }

  if (lvx_rcms_model (st, ii, NULL, &probe, &why))
    lvx_lms_dump (";; lvx rcms: fn=%s ii=%d modelcheck=ok\n",
		  current_function_name (), ii);
  else
    lvx_lms_dump (";; lvx rcms: fn=%s ii=%d MODELCHECK=FAILED %s\n",
		  current_function_name (), ii, why);
}

/* Model size, for the dump: what the solver is being asked to do.  */

static void
lvx_rcms_size (struct lvx_rcms_state *st, int ii, int *nvar, int *nrow)
{
  unsigned n = st->n, i;
  int v = 1, r = 0;

  for (i = 0; i < n; i++)
    v += st->win[i].hi - st->win[i].lo + 1;
  r += n;			  /* assignments */
  r += st->p->arcs->length ();	  /* dependences, self arcs over-counted */
  r += n;			  /* makespan */
  r += 15 * ii;			  /* resources, before LAO's kill */
  *nvar = v;
  *nrow = r;
}

static void *
lvx_rcms_prepare (const struct lvx_lms_problem *p)
{
  struct lvx_rcms_state *st = new lvx_rcms_state;
  unsigned i;

  st->p = p;
  st->n = p->seq->length ();
  st->horizon = 0;
  st->ok = st->n != 0;

  /* b_i, charged exactly as every other engine charges it -- the prefix
     conservatism of the GUARD family included.  RCMS being measured against
     CMS and SMS means it must pay for the same things they pay for.  */
  st->demand.safe_grow_cleared (st->n);
  for (i = 0; i < st->n; i++)
    {
      memset (&st->demand[i], 0, sizeof (st->demand[i]));
      lvx_lms_resources_add (&st->demand[i], (*p->seq)[i]);
    }
  return st;
}

static void
lvx_rcms_finish (void *state)
{
  delete (struct lvx_rcms_state *) state;
}

static bool
lvx_rcms_schedule_at_ii (void *state, int ii, auto_vec<int> &sigma,
			 const char **why, int *passes)
{
  struct lvx_rcms_state *st = (struct lvx_rcms_state *) state;
  const char *model = NULL, *sol = NULL;
  auto_vec<int> blame, culprit;
  bool ok = false;

  if (!st->ok)
    {
      *why = "rcms: empty loop";
      return false;
    }

  (*passes)++;

  if (!lvx_rcms_windows (st, ii, why))
    goto out;

  if (lvx_lms_rcms_check)
    lvx_rcms_selfcheck (st, ii);

  if (dump_file && lvx_lms_probe)
    {
      int nvar, nrow;
      lvx_rcms_size (st, ii, &nvar, &nrow);
      lvx_lms_dump (";; lvx rcms: fn=%s ii=%d horizon=%d vars=%d rows<=%d\n",
		    current_function_name (), ii, st->horizon, nvar, nrow);
    }

  /* The model file is an ARTEFACT, not a scratch buffer: with
     -mlms-rcms-dir it is kept beside the measurement, so the same instance
     can be re-solved later and by a different solver.  That is the only way
     to tell a formulation bug from a solver bug.  */
  if (lvx_lms_rcms_dir != NULL && lvx_lms_rcms_dir[0] != '\0')
    {
      /* The name has to carry the INTERVAL and a serial, not just the
	 function: the ladder asks the same function for several intervals
	 and a function holds several loops, so a name that carries only
	 the function silently keeps the last model and throws the rest
	 away -- which would make an archived instance the wrong one.  */
      static int serial = 0;
      char tag[64];

      sprintf (tag, ".ii%d.%d.lp", ii, serial++);
      model = ACONCAT ((lvx_lms_rcms_dir, "/", current_function_name (),
			tag, NULL));
      sol = ACONCAT ((model, ".sol", NULL));
    }
  else
    {
      model = make_temp_file (".lp");
      sol = make_temp_file (".sol");
    }

  {
    FILE *f = fopen (model, "w");
    if (f == NULL)
      {
	*why = "rcms: cannot write the model";
	goto out;
      }
    if (!lvx_rcms_model (st, ii, f, NULL, why))
      {
	fclose (f);
	goto out;
      }
    fclose (f);
  }

  if (!lvx_rcms_solve (model, sol, why))
    goto out;
  if (!lvx_rcms_read (st, sol, sigma, why))
    goto out;

  /* Acceptance is the framework's, not the engine's.  This is what makes an
     external solver safe to trust: whatever it returned is verified against
     every arc and the modulo table before it can reach code generation.

     BLAME AND CULPRIT ARE NOT OPTIONAL.  §7tricies.229.  They look like
     out-parameters a caller may decline, and the colourability path writes
     through them WITHOUT a null check (lvx-lms.cc, the flow-record loop).
     Passing NULL segfaulted the compiler on exactly the loops whose kernel
     would not colour -- so the engine crashed where it should have reported
     a refusal, and the refusal diagnostics that did appear were the
     survivors of the ones that did not.  Allocate them, as SMS does.  */
  blame.safe_grow_cleared (st->n);
  culprit.safe_grow_cleared (st->n);
  *why = lvx_lms_accept_p (*st->p->seq, *st->p->arcs, ii, sigma,
			   st->p->livein, &blame, &culprit);
  ok = (*why == NULL);
  if (lvx_lms_rcms_trace)
    lvx_lms_dump (";; lvx rcms:   ii=%d acceptance: %s\n", ii,
		  ok ? "taken" : *why);

out:
  /* A REFUSAL THAT DOES NOT SAY WHY CANNOT BE DIAGNOSED.  §7tricies.229.
     RCMS answered on 382 of the 582 loops CMS answered on, and the solver
     verdicts said the solver was not the bottleneck -- three timeouts in
     462 -- so the losses are before or after it.  Which of the two, and in
     what proportion, is not derivable from the counts that existed: this
     line is what makes it countable.  */
  if (!ok && dump_file && lvx_lms_probe)
    lvx_lms_dump (";; lvx rcms: fn=%s ii=%d rcmsrefused: %s\n",
		  current_function_name (), ii,
		  (why != NULL && *why != NULL) ? *why : "unstated");

  if (!(lvx_lms_rcms_dir != NULL && lvx_lms_rcms_dir[0] != '\0')
      && !lvx_lms_rcms_keep)
    {
      if (model)
	unlink (model);
      if (sol)
	unlink (sol);
    }
  return ok;
}

const struct lvx_lms_engine lvx_rcms_engine = {
  "rcms", lvx_rcms_prepare, lvx_rcms_schedule_at_ii, lvx_rcms_finish
};
