/* Convergent Modulo Scheduling for LVX: one engine of the LMS framework.
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

/* CMS schedules the loop body's instructions as a series of UNWOUND COPIES
   against a monotone clock and projects the result onto one period, and
   steers that construction with Squeaky Wheel Optimization: construct,
   analyse what the projection refused, re-prioritise, construct again.  It is
   one engine of the framework in lvx-lms.h -- it decides DATES at a given
   initiation interval and nothing else, and lvx_lms_accept_p judges what it
   produces exactly as it judges any other engine's.

   Moved here from lvx.cc unchanged.  */

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


/* INSN'S OWN RESOURCE DEMAND, WEIGHTED BY SCARCITY.  §7tricies.186.

   The scarcity of a counter is DEM / (II * AVAIL): what the body demands of
   it against what the whole initiation interval supplies.  That is the same
   quantity the `tight=' report already computes and then discards -- it was
   measured, printed and never fed to a decision.  Scaled by 256, so an
   integer priority can carry a fraction of a cycle.  */

static int
lvx_cms_rwc_weight (rtx_insn *insn, const struct lvx_sched_resources *dem,
		    int ii)
{
  struct lvx_sched_resources u;
  int w = 0;

  if (ii <= 0)
    return 0;
  memset (&u, 0, sizeof (u));
  lvx_lms_resources_add (&u, insn);
#define LVX_CMS_RWCW(R)							  {									    unsigned avail = lvx_issue_availability.R;				    if (avail > 0 && u.R##_count > 0 && dem->R##_count > 0)		      w += (int) ((256u * dem->R##_count * u.R##_count)					  / ((unsigned) ii * avail));				  }
  LVX_ISSUE_RESOURCES (LVX_CMS_RWCW)
#undef LVX_CMS_RWCW
  return w;
}

/* THE EARLIEST CYCLE A CELL MAY ISSUE IN: the modulo constraint from its own
   previous copy, the incoming arcs at their omega distance, and its release
   date, II-periodically.  -1 if a predecessor is unscheduled, which a ready
   cell cannot have.

   IT NO LONGER WALKS FORWARD.  §7tricies.188.  It used to continue from
   EARLIEST to the first cycle whose resources admitted the instruction, and
   report which counter refused the first cycle tried.  Under a monotone clock
   neither is wanted: the placement is AT the clock, and taking the walk's
   answer is what let dates run backward.  The refusal it reported paid for the
   whole flat cycle table, which is gone with it -- see the blame below.  */


static int
lvx_cms_earliest_cycle (const auto_vec<lvx_lms_arc> &arcs,
			const auto_vec<int> &release,
			const auto_vec<int> &date,
			unsigned n, int ncopy, int ii, unsigned bi, int bc)
{
  int earliest = 0;

  if (bc > 0)
    earliest = date[(unsigned) (bc - 1) * n + bi] + ii;
  for (unsigned a = 0; a < arcs.length (); a++)
    if (arcs[a].dst == bi)
      {
	int sc = bc - (int) arcs[a].omega;
	int d;

	if (sc < 0 || sc >= ncopy)
	  continue;
	d = date[(unsigned) sc * n + arcs[a].src];
	if (d < 0)
	  return -1;
	if (earliest < d + (int) arcs[a].theta)
	  earliest = d + (int) arcs[a].theta;
      }
  if (earliest < 0)
    earliest = 0;
  if (!release.is_empty () && release[bi] > 0)
    {
      int r = release[bi] + bc * ii;

      if (earliest < r)
	earliest = r;
    }
  return earliest;
}

/* Convergent modulo scheduling of the body SEQ at initiation interval II.

   One task per instruction and per-copy arrays indexed by the copy number,
   which is the single-copy form of docs/cms-foundations.md §7sexies.  Copy C of
   node I is that instruction's instance in iteration C; an arc
   (SRC, DST, THETA, OMEGA) constrains copy C of DST against copy C - OMEGA of
   SRC, so the graph needs no second physical copy to express a cross-iteration
   dependence and an OMEGA above one is an ordinary arc rather than something
   the construction has to be able to see.

   The copies are scheduled FREELY in absolute cycles, and convergence is the
   discovery that copy C repeats copy C - 1 exactly II cycles later: at that
   point SIGMA (I) = date (I, C) - C * II is a modulo schedule.  What forces the
   period to II is the one place the two-copy form needed an arc for -- its `M'
   arc from base to twin at latency +II.  With a single task per instruction
   that constraint relates a node to ITSELF at the next copy, so it is a rule in
   the scheduler rather than an edge in the graph.

   Returns true and fills SIGMA on convergence.  */

/* The convergence ORACLE, defined below: it answers whether a candidate
   periodic schedule satisfies every arc (omega included) and fits the modulo
   reservation table.  §7tricies.128.  */

/* VALUE-BIASED STOCHASTIC SAMPLING.  §7tricies.170.

   Bresina's HBSS and Cicirello's VBSS keep a greedy constructor's heuristic
   but make the SELECTION stochastic: score the ready candidates, turn the
   scores into weights through a bias function, and sample proportionally
   instead of taking the argmax.  HBSS biases on a candidate's RANK, VBSS on
   its VALUE -- and the value form is the one that matters here, because
   ranking discards exactly the information we need, that two candidates are
   nearly equal rather than merely first and second (Lihu & Holban 2009:
   "Choices are only sorted, so heuristic values are not used to their full
   potential").

   Why this axis and not another: -mcms-priority=, -mcms-select= and
   -mcms-release= all swap one DETERMINISTIC rule for another, and eight such
   configurations produced the identical answer for saxpy.  Improving SWO's
   signal (§7tricies.129) and grading its move (§7tricies.169) both measured
   flat, and both changed an argmax's INPUTS.  This changes the argmax itself.

   xorshift32, and the seed is derived from the loop's SHAPE (task count, II,
   pass) rather than from any address, so a build is reproducible and two
   compilations of the same function draw the same sequence.  */

static inline unsigned
lvx_cms_rnd (unsigned *state)
{
  unsigned x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x ? x : 0x9e3779b9u;		/* xorshift32 dies at zero */
  return *state;
}

static bool
lvx_cms_converge (const auto_vec<rtx_insn *> &seq,
		  const auto_vec<lvx_lms_arc> &arcs,
		  const auto_vec<int> &prio,
		  const auto_vec<int> &release,
		  int ii, int delta, auto_vec<int> &sigma,
		  auto_vec<int> *blame, auto_vec<int> *culprit,
		  unsigned vseed = 0)
{
  unsigned n = seq.length ();
  if (n == 0 || ii <= 0)
    return false;
  /* VSEED == 0 means take the argmax, which is what pass 0 always does: the
     deterministic draw is kept so that budget 1 reproduces the pre-VBSS
     scheduler exactly and sampling can only ADD attempts, never replace the
     one that was there.  */
  unsigned vstate = vseed;

  /* COPIES ARE A SEARCH BUDGET, AND THE ORACLE DECIDES.  §7tricies.128.

     Dinechin & Munier Kordon 2014, Theorem 10, gives Δ >= max{Δ1, Δ2} as
     SUFFICIENT for the projection of an unwound schedule to be a legal
     periodic one -- Δ1 = max |h_ij| for the precedences (Lemma 5), Δ2 =
     ceil ((dmax - rmin) / w) for the resources (Lemma 9).  Waiting for that
     many stationary copies was implemented and measured: it demands copies
     the heuristic does not need, and the loops it recovered the exact test
     recovers at HALF the budget.

     So Δ2 is not computed and the budget stays at Δ1 + 4.  The acceptance
     test below asks lvx_lms_verify directly, which is the property Δ2 was
     standing in for.  Exhausting this budget means `not settled' and never
     `II infeasible': no bound on copies could establish the latter, since
     deciding modulo schedulability at a fixed II under resource constraints
     is NP-complete.  */
  int ncopy = delta + 4;
  unsigned cells = n * (unsigned) ncopy;

  auto_vec<int> date, pred;
  date.safe_grow_cleared (cells);
  pred.safe_grow_cleared (cells);

  /* THE ORACLE IS THE SQUEAK.  §7tricies.129.
     Squeaky Wheel Optimization needs the constructor to FAIL on identifiable
     tasks, and a cycle scheduler never fails: it places every instruction, so
     the only signal left was `when - earliest', the delay a placement
     suffered, attributed to a RESOURCE TYPE rather than to an instruction --
     which cannot say which task to move.  The real failure is in the
     PROJECTION, and lvx_lms_verify knows exactly which instructions caused
     it.  Every copy that completes asks the oracle, and every refusal names
     its culprits here; what survives when the budget runs out is the squeak,
     accumulated over the whole construction rather than over one copy.  */
  auto_vec<int> guilty;
  guilty.safe_grow_cleared (n);
  for (unsigned k = 0; k < cells; k++)
    date[k] = -1;

  /* WHEN EACH CELL BECAME READY, for the monotonicity hunt: a backward step
     whose chosen cell became ready on the PREVIOUS step is a fresh successor
     (so a non-positive theta explains it); one that was ready earlier refutes
     the claim that an already-ready candidate's clock advance can only rise.  */
  auto_vec<int> ready_at;
  ready_at.safe_grow_cleared (cells);
  for (unsigned k = 0; k < cells; k++)
    ready_at[k] = -1;
  if (lvx_cms_place_trace && dump_file && lvx_lms_probe)
    {
      HOST_WIDE_INT tmin = 0, tmax = 0;
      unsigned nneg = 0, nzero = 0;
      for (unsigned a2 = 0; a2 < arcs.length (); a2++)
	{
	  if (a2 == 0 || arcs[a2].theta < tmin) tmin = arcs[a2].theta;
	  if (a2 == 0 || arcs[a2].theta > tmax) tmax = arcs[a2].theta;
	  if (arcs[a2].theta < 0) nneg++;
	  else if (arcs[a2].theta == 0) nzero++;
	}
      lvx_lms_dump (";; lvx cms: PLACEARCS ii=%d narcs=%u thetamin=" HOST_WIDE_INT_PRINT_DEC
		    " thetamax=" HOST_WIDE_INT_PRINT_DEC " nneg=%u nzero=%u\n",
		    ii, arcs.length (), tmin, tmax, nneg, nzero);
    }

  /* Entering-arc counts per copy.  A source copy below zero is a prologue
     instance, already issued before the kernel, so the arc is satisfied; the
     self modulo constraint accounts for the extra predecessor of every copy
     after the first.  */
  for (unsigned a = 0; a < arcs.length (); a++)
    for (int c = 0; c < ncopy; c++)
      {
	int sc = c - (int) arcs[a].omega;
	if (sc >= 0 && sc < ncopy)
	  pred[(unsigned) c * n + arcs[a].dst]++;
      }
  for (int c = 1; c < ncopy; c++)
    for (unsigned i = 0; i < n; i++)
      pred[(unsigned) c * n + i]++;
  for (unsigned k = 0; k < cells; k++)
    if (pred[k] == 0)
      ready_at[k] = 0;

  /* ONE CYCLE'S RESOURCES, NOT A TABLE.  §7tricies.188.  A monotone clock
     never revisits a cycle, so the only reservation state a placement needs is
     the cycle it is in; CUR is reset when the clock advances.  The table this
     replaces averaged 75.7 rows against a mean II of 13.7 -- 5.5 times an MRT
     -- and existed to be indexed away from the clock by the forward walk.  */
  struct lvx_sched_resources curcyc;

  memset (&curcyc, 0, sizeof (curcyc));
  /* THE SAME TABLE WITH THE DOLOOP_END UNCHARGED.  §7tricies.87.
     lvx_lms_resources_add charges a doloop_end as a decrement plus a branch,
     and on a loop that gets mapped it occupies no slot (it emits
     `# loopdo end').  Measuring that on ResMII measured the harmless half: the
     bound only seeds the II search, which increments until CONVERGE succeeds,
     so an overstated bound costs search steps and not the answer.  What can
     cost the answer is THIS table -- if the charge is why a cycle refuses an
     instruction, the placement walks to a later cycle and the schedule is
     really worse.  So the shadow table is maintained in lockstep, charged
     everything except the doloop_end, and every refusal is retried against it:
     NDLBLOCK counts the placements the charge alone pushed out of a cycle.
     Read-only -- the shadow never decides anything.  */

  /* THE ROLLING MRT, read-only.  §7tricies.183.  MROW[i] is the residue row
     instruction i's latest instance occupies, -1 before it has one.  */
  auto_vec<struct lvx_sched_resources> mrt;
  auto_vec<int> mrow;
  /* THE MONOTONE CLOCK.  §7tricies.185 (author): CMS is cycle-based by
     foundation, and the operation-driven walk was a drift from it.  Time only
     advances; at each cycle the eligible cells are those whose dependences and
     release allow them AT OR BEFORE it and that fit its resources, and a
     placement is always AT the clock -- never at `the first cycle that fits
     from earliest', which is what let dates run backward (§7tricies.171
     measured 1.83%, all at copy transitions).

     STALL counts consecutive advances that placed nothing, and bounds the
     residue policy below: residues repeat with period II, so after II idle
     advances a candidate that overflows everywhere must be forced through
     rather than deferred forever.  */
  int clock = 0;
  unsigned stall = 0;
  int clockcap = (ncopy + 2) * ii + (int) n + 64;
  unsigned mrt_first_zero = 0;	/* placement step at which it first fit */
  bool mrt_saw_zero = false;
  unsigned mrt_prev = 0, mrt_rises = 0, mrt_falls = 0, mrt_placed = 0;

  if (lvx_cms_rolling_mrt)
    {
      struct lvx_sched_resources empty;

      memset (&empty, 0, sizeof (empty));
      for (int r = 0; r < ii; r++)
	mrt.safe_push (empty);
      for (unsigned i = 0; i < n; i++)
	mrow.safe_push (-1);
    }

  int placed = 0;
  while (placed < (int) cells)
    {
      /* Pick a ready cell: lowest copy first, then by PRIO ascending, then
	 program order.  Ascending, so a priority is a cost and not a merit --
	 longest-path-to-sink therefore enters negated.  This comment used to
	 say "the longest remaining dependence path", which is the path to the
	 SINK and the opposite of what the default priority is.  */
      /* Both rules are evaluated every time, and the policy only decides which
	 answer is used, so that DISAGREE counts how often they would differ.
	 Measuring that was the point: policy 1 produced byte-identical assembly
	 on all 149 TSVC2 loops, which is either a dead branch or two criteria
	 that genuinely coincide, and only the count can say which.  CMS's
	 priority is longest-path-to-sink, and a task with a long path to the
	 sink tends to be early in the dependence order and so to land early,
	 so coincidence is plausible -- but plausible is not measured.  */
      int best = -1;

      /* CYCLE-BASED SELECTION AT THE MONOTONE CLOCK.  §7tricies.185.

	 The eligible set at CLOCK, then the author's residue policy over it:
	 a candidate whose placement would push the rolling MRT above a
	 resource bound is DEPRIORITISED FOR THIS CYCLE -- but issues anyway
	 if it is the only candidate.  When several are eligible and all would
	 overflow, none issues and the clock advances: that leaves a unit idle
	 while something was eligible, which is exactly the escape from the
	 parallel SGS's non-delay restriction (§7tricies.172), and the residues
	 differ at the next cycle so the deferral is not idle waiting.

	 §7tricies.184 measured the precondition: 3.8 of 5.3 ready candidates
	 avoid the overflow, so there is usually something better to issue, and
	 least-clock-advance was systematically taking the one that did not.  */
      while (best < 0)
	{
	  unsigned cur = mrt.is_empty () ? 0 : lvx_lms_mrt_violations (mrt);
	  auto_vec<unsigned> elig;
	  auto_vec<bool> ovf;
	  unsigned nok = 0;

	  if (clock > clockcap)
	    return false;
	  for (int c = 0; c < ncopy; c++)
	    for (unsigned i = 0; i < n; i++)
	      {
		unsigned k = (unsigned) c * n + i;
		int e0 = 0;

		if (date[k] >= 0 || pred[k] > 0)
		  continue;
		e0 = lvx_cms_earliest_cycle (arcs, release, date, n, ncopy, ii,
					     i, c);
		if (e0 < 0 || e0 > clock)
		  continue;			/* unready, or not released */
		if (!lvx_lms_slot_fits_p (&curcyc, seq[i]))
		  continue;			/* no room in this cycle */
		{
		  bool bad = false;

		  if (!mrt.is_empty ())
		    bad = lvx_lms_mrt_trial (mrt, mrow, seq[i], i,
					     ((clock % ii) + ii) % ii) > cur;
		  elig.safe_push (k);
		  ovf.safe_push (bad);
		  if (!bad)
		    nok++;
		}
	      }
	  if (elig.is_empty ())
	    {
	      clock++, stall++;
	      memset (&curcyc, 0, sizeof (curcyc));
	      continue;
	    }
	  /* The policy: the rolling MRT REORDERS PREFERENCE WITHIN A CYCLE AND
	     NEVER PREVENTS ISSUE (author).  Prefer a candidate that does not
	     push a counter over its bound; when every eligible candidate
	     would, issue one anyway rather than leave the cycle idle.

	     AND IDLING IS FORBIDDEN BY THE DESIGN, not merely expensive
	     (author): CMS approaches what an out-of-order superscalar's
	     hardware scheduler does, so it is WORK-CONSERVING -- it never
	     declines available work.  Schedule diversity comes from tuning
	     PRIORITIES and RELEASE DATES instead, and that is also how the
	     parallel SGS's non-delay restriction is escaped (§7tricies.172): a
	     release date defers a task BEFORE it becomes eligible, so a unit
	     can end up idle without the scheduler ever having idled it.

	     The first version here deferred overflowers -- advancing with an
	     empty cycle unless the overflower was alone or II advances had
	     passed.  That is a different machine, and it also measured worse:
	     same sum(II) as the clock alone and +102 sum(makespan), which is
	     what deliberate idling costs.  Deprioritising decides WHICH of
	     several eligible candidates goes now, never whether anything
	     goes.  */
	  bool take_bad = (nok == 0);
	  /* Among the admissible, the existing criterion: priority then
	     program order, sampled by VBSS on later passes.  Least clock
	     advance has nothing left to say -- every candidate is at CLOCK,
	     which is the point.  */
	  {
	    auto_vec<unsigned> cand;

	    for (unsigned q = 0; q < elig.length (); q++)
	      if (take_bad || !ovf[q])
		cand.safe_push (elig[q]);
	    if (vstate != 0 && lvx_cms_vbss > 0 && cand.length () > 1)
	      {
		int pmin = prio[cand[0] % n];
		unsigned total = 0;
		auto_vec<unsigned> wgt;

		for (unsigned q = 1; q < cand.length (); q++)
		  if (prio[cand[q] % n] < pmin)
		    pmin = prio[cand[q] % n];
		for (unsigned q = 0; q < cand.length (); q++)
		  {
		    unsigned excess = (unsigned) (prio[cand[q] % n] - pmin);
		    unsigned den = 1;

		    for (int g = 0; g < lvx_cms_vbss && den < (1u << 18); g++)
		      den *= (excess + 1);
		    unsigned v = (1u << 18) / den;

		    if (v == 0)
		      v = 1;
		    wgt.safe_push (v);
		    total += v;
		  }
		unsigned r = total ? lvx_cms_rnd (&vstate) % total : 0;
		unsigned pick = 0;

		for (unsigned q = 0; q < wgt.length (); q++)
		  {
		    if (r < wgt[q])
		      { pick = q; break; }
		    r -= wgt[q];
		    pick = q;
		  }
		best = (int) cand[pick];
	      }
	    else
	      for (unsigned q = 0; q < cand.length (); q++)
		{
		  unsigned kk = cand[q];

		  if (best < 0
		      || prio[kk % n] < prio[(unsigned) best % n]
		      || (prio[kk % n] == prio[(unsigned) best % n]
			  && kk < (unsigned) best))
		    best = (int) kk;
		}
	  }
	  stall = 0;
	  if (lvx_cms_clock_trace && dump_file && lvx_lms_probe)
	    lvx_lms_dump (";; lvx cms:   CLOCKPICK ii=%d clock=%d elig=%u"
			  " ok=%u forced=%d insn=%d\n", ii, clock,
			  elig.length (), nok, take_bad ? 1 : 0, best % (int) n);
	}

      if (best < 0)
	/* Nothing ready and cells remain.  This gives up at this II; it does
	   NOT establish that no schedule exists there.  Deciding modulo
	   schedulability at a fixed II under resource constraints is
	   NP-complete, so no search licenses that claim.  §7tricies.127.  */
	return false;
      unsigned bi = (unsigned) best % n;
      int bc = best / (int) n;
      /* PLACEMENT ORDER AGAINST ASSIGNED DATE.  Asks whether the dates come
	 out monotone, which decides whether this construction could drive an
	 automaton resource model -- one can be advanced but never reversed
	 (docs/dfa-resource-interfaces.md).  Trace only; decides nothing.  */
      /* Where it lands, from the same function the selection used, so the two
	 cannot disagree.  The release-date rationale that used to sit here is
	 kept below; the arithmetic itself moved into lvx_lms_place_cycle.  */
      /* The RELEASE DATE, II-PERIODIC, enforced here rather than by an arc
      	 from the dependence graph's source node.

      	 Two reasons it belongs in the scheduler and not in the graph.  First,
      	 periodicity: copy C of task I is iteration C's instance, so its release
      	 is RELEASE[I] + C * II, and an arc from the source would pin only copy
      	 zero -- expressing the periodic form would need one arc per copy, which
      	 is a graph that changes with II.  Second, and the reason this is a safe
      	 SWO parameter at all: the graph is left alone, so RecMII and the lower
      	 bound are the same whatever the release dates are, and no amount of
      	 perturbation can make the problem infeasible the way an added arc can.
      	 A release only ever delays a task that is already dependence-ready; it
      	 never removes it from the ready set, so the construction still
      	 terminates.  */
      int earliest = lvx_cms_earliest_cycle (arcs, release, date, n, ncopy, ii,
					     bi, bc);
      int when;

      if (earliest < 0)
	return false;
      /* AT THE CLOCK -- not a variant of anything, it IS the placement.
	 §7tricies.187.  The cell was admitted because its earliest is at or
	 before CLOCK and CLOCK had room for it.  */
      when = clock;
      lvx_lms_resources_add (&curcyc, seq[bi]);
      date[(unsigned) best] = when;

      /* ROLLING MRT: retire this task's previous instance, install the new
	 one, and measure the distance to resource feasibility.  §7tricies.183. */
      if (lvx_cms_rolling_mrt && ii > 0)
	{
	  int row = ((when % ii) + ii) % ii;

	  if (mrow[bi] >= 0)
	    lvx_lms_resources_sub (&mrt[(unsigned) mrow[bi]], seq[bi]);
	  lvx_lms_resources_add (&mrt[(unsigned) row], seq[bi]);
	  mrow[bi] = row;

	  {
	    unsigned v = lvx_lms_mrt_violations (mrt);
	    bool full = true;

	    for (unsigned i = 0; i < n && full; i++)
	      if (mrow[i] < 0)
		full = false;
	    mrt_placed++;
	    if (mrt_placed > 1)
	      {
		if (v > mrt_prev)
		  mrt_rises++;
		else if (v < mrt_prev)
		  mrt_falls++;
	      }
	    mrt_prev = v;
	    /* Only meaningful once every task has an instance: before that the
	       table describes a partial sigma.  */
	    if (full && v == 0 && !mrt_saw_zero)
	      mrt_saw_zero = true, mrt_first_zero = (unsigned) placed;
	    if (lvx_cms_rolling_mrt > 1 && dump_file && lvx_lms_probe)
	      lvx_lms_dump (";; lvx cms:   MRTROLL ii=%d step=%d insn=%u"
			    " row=%d viol=%u full=%d\n", ii, placed, bi, row,
			    v, full ? 1 : 0);
	  }
	}
      /* PLACEMENT ORDER AGAINST ASSIGNED DATE.  Asks whether the dates come
	 out monotone, which decides whether this construction could drive an
	 automaton resource model -- one can be advanced but never reversed
	 (docs/dfa-resource-interfaces.md).  Trace only; decides nothing.  */
      if (lvx_cms_place_trace && dump_file && lvx_lms_probe)
	lvx_lms_dump (";; lvx cms: PLACE ii=%d step=%d insn=%u copy=%d"
		      " date=%d rdy=%d\n", ii, placed, bi, bc, when,
		      ready_at[(unsigned) best]);
      placed++;

      /* BLAME, for SWO's analysis phase: how far resource contention pushed
	 this task past the date its dependences and release allowed.  A task
	 that is repeatedly displaced is the squeaky wheel.

	 And ATTRIBUTION, which is what lets the next construction aim rather
	 than merely try harder.  The displaced task is one half of the story;
	 the other is whoever was holding the counter that refused it.  Those
	 are credited as CULPRITS, and a culprit with slack is exactly what a
	 release date should move out of the way -- where the previous rule
	 delayed every slack task indiscriminately, whether or not it was ever
	 in anyone's way.  */
      /* BLAME: THE WAIT, AND ONLY THE WAIT.  §7tricies.188.

	 CLOCK - EARLIEST is how long resource contention held this cell past
	 the date its dependences and release allowed, and under a monotone
	 WORK-CONSERVING clock that is a sound resource statement by
	 construction: the loop places every eligible cell before advancing, so
	 a cell that waited did not FIT -- losing a selection reorders within a
	 cycle and cannot delay anything.  Measured at 100%: of 33371 delayed
	 placements, every one had a refusing counter.

	 WHICH counter is no longer recorded.  RESBLAME and the culprit window
	 scan were the only readers of the forward walk, and the walk was the
	 only thing indexing a cycle other than the clock -- so they were what
	 the 75-row table was for.  The attribution was sound and never paid:
	 §7octies measured every release policy conditioned on it as worse than
	 ignoring it (policy 2 indistinguishable from no lever at all over 176
	 extra passes, policy 3 at 89 against 79), and §7tricies.129's
	 oracle-sourced version measured flat.  The lever that did pay reads
	 the residue table, not this.

	 CULPRIT survives, from the oracle (§7tricies.129): a projection that
	 fails names the instructions responsible directly, which is a better
	 signal than a window scan inferred from a resource.  */
      if (blame != NULL && when > earliest)
	(*blame)[bi] += when - earliest;

      /* Release the successors of this cell.  */
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].src == bi)
	  {
	    int dc = bc + (int) arcs[a].omega;
	    if (dc >= 0 && dc < ncopy)
	      {
		unsigned dk = (unsigned) dc * n + arcs[a].dst;
		if (--pred[dk] == 0 && ready_at[dk] < 0)
		  ready_at[dk] = placed;
	      }
	  }
      if (bc + 1 < ncopy)
	{
	  unsigned sk = (unsigned) (bc + 1) * n + bi;
	  if (--pred[sk] == 0 && ready_at[sk] < 0)
	    ready_at[sk] = placed;
	}

      /* Stationarity: a copy that repeats its predecessor exactly II cycles
	 later IS the kernel.  */
      if (bc > 0 && (unsigned) placed % n == 0)
	{
	  /* THE ORACLE, NOT A PROXY FOR IT.  §7tricies.128.

	     What convergence must establish is that the PROJECTION is a legal
	     modulo schedule: every arc satisfied at distance omega, and the
	     modulo reservation table fitting.  lvx_lms_verify computes exactly
	     that from a candidate SIGMA, so it can be asked directly instead
	     of waiting for a structural property that implies it.

	     Stationarity was the proxy.  One adjacent pair repeating at II is
	     too weak -- it accepted two-copy `kernels' whose projection put
	     six instructions on one residue (§7tricies.127).  Theorem 10's
	     max{Δ1, Δ2} consecutive pairs is SOUND but conservative: it is a
	     SUFFICIENT condition, so it demands copies the heuristic may not
	     need, and measured over four suites it moved two loops of 565 to
	     their lower bound for no change in anything else.

	     The oracle is exact in both directions.  It never accepts an
	     invalid projection, and it accepts the moment one is valid --
	     including the schedules stationarity structurally cannot see,
	     where a periodic schedule is legal without consecutive copies
	     being exactly II apart.  Theorem 10 keeps its role as the reason
	     the search TERMINATES: some Δ always suffices, so the copies are
	     bounded; it is simply not the acceptance test.  */
	  bool settled = false;
	  {
	    bool whole = true;
	    for (unsigned i = 0; i < n && whole; i++)
	      if (date[(unsigned) bc * n + i] < 0)
		whole = false;
	    if (whole)
	      {
		auto_vec<int> cand;
		for (unsigned i = 0; i < n; i++)
		  cand.safe_push (date[(unsigned) bc * n + i] - bc * ii);
		settled = lvx_lms_verify (seq, arcs, ii, cand, &guilty) == NULL;
	      }
	  }
	  if (settled && lvx_cms_rolling_mrt && dump_file && lvx_lms_probe)
	    lvx_lms_dump (";; lvx cms: MRTFIT fn=%s ii=%d oracle_at=%d"
			  " mrt_zero_at=%s rises=%u falls=%u steps=%u\n",
			  current_function_name (), ii, placed,
			  mrt_saw_zero ? "yes" : "never", mrt_rises, mrt_falls,
			  mrt_placed);
	  if (mrt_saw_zero && dump_file && lvx_lms_probe && settled)
	    lvx_lms_dump (";; lvx cms: MRTZERO fn=%s ii=%d first_zero=%u"
			  " oracle=%d\n", current_function_name (), ii,
			  mrt_first_zero, placed);
	  if (settled)
	    {
	      sigma.truncate (0);
	      for (unsigned i = 0; i < n; i++)
		sigma.safe_push (date[(unsigned) bc * n + i] - bc * ii);
	      /* Reported only on the converged II, and only when non-zero: the
		 question is whether the doloop_end's charge ever actually moved
		 a placement in the schedule that was kept.  §7tricies.87.  */
	      return true;
	    }
	}
    }

  /* THE SQUEAK, DELIVERED.  §7tricies.129.  The budget ran out, so no copy's
     projection ever verified; GUILTY holds, per instruction, how many times
     the oracle named it as the reason.  Those are the tasks whose placement
     has to change, and CULPRIT is the array whose meaning is already `move
     this out of the way' -- policies 2 and 3 push a culprit with slack later,
     which is precisely how an instruction leaves the residue it is
     overflowing or the cycle that makes an arc infeasible.

     Unlike the delay signal this one is attributed to INSTRUCTIONS and comes
     from an actual failure, which is what SWO's analysis phase assumes it has
     and what a cycle scheduler otherwise cannot give it.  */
  if (culprit != NULL && lvx_cms_squeak != 0)
    {
      unsigned named = 0, total = 0;
      for (unsigned i = 0; i < n; i++)
	{
	  (*culprit)[i] += guilty[i];
	  if (guilty[i] > 0)
	    named++, total += guilty[i];
	}
      if (dump_file && lvx_lms_probe)
	lvx_lms_dump (";; lvx cms: fn=%s II=%d SQUEAK named=%u/%u"
		      " accusations=%u\n", current_function_name (), ii,
		      named, n, total);
    }

  return false;
}



/* SQUEAKY WHEEL OPTIMIZATION at a fixed II, after Joslin and Clements, JAIR
   1999: Construct, Analyze, Prioritize, and go round again.

   The construction is the greedy list scheduler of lvx_cms_converge, which does
   not backtrack -- that is the point.  SWO does not search within one
   construction; it changes the PARAMETERS the next construction reads, so each
   pass is as cheap as the first and the whole cycle is linear in the budget.
   The two parameters are the two the scheduler takes:

     PRIORITY   which of the dependence-ready tasks is picked first.
     RELEASE    when a task becomes eligible at all, II-periodically.

   Release is the stronger of the two and the one a pure priority scheme cannot
   imitate.  Priority only reorders tasks that are ALREADY ready, so it can
   never hold a task back; release can.  That matters here because the
   characteristic failure of a greedy modulo scheduler at a tight II is placing
   a task with slack early, spending a resource that a critical task needs in
   that cycle and cannot defer.  No priority can fix it -- the slack task is
   ready and something must be chosen -- but a release date on the slack task
   leaves the slot alone.

   ANALYZE blames each task for the cycles resource contention pushed it past
   its dependence-feasible date, summed over the copies.  PRIORITIZE then:

     - escalates the priority of blamed tasks, cumulatively, so a repeat
       offender keeps rising rather than oscillating;
     - and pushes tasks that have SLACK and no blame back to their latest
       feasible start, which is what frees the early slots the blamed tasks
       were denied.

   Slack comes from a backward pass over the intra-iteration arcs against the
   makespan the previous construction achieved: a task whose latest start
   exceeds its earliest has room to be delayed for free.

   Returns true with SIGMA set when a construction both converged and verified.
   The budget bounds the passes; exhausting it is how the caller learns to try
   II + 1 instead.  */

bool
lvx_cms_swo_at_ii (const auto_vec<rtx_insn *> &seq,
		   const auto_vec<lvx_lms_arc> &arcs,
		   const auto_vec<int> &height,
		   int ii, int delta, auto_vec<int> &sigma,
		   int budget, int *passes, const char **last_bad,
		   int pin, int mcrl_release,
		   bitmap livein,
		   const auto_vec<int> *crit)
{
  /* -1 means `whatever the option says'; the gate below passes an explicit
     setting so it can try a II both ways.  */
  if (mcrl_release < 0)
    mcrl_release = lvx_cms_mcrl_release;
  unsigned n = seq.length ();
  auto_vec<int> prio, release, blame, culprit, cumulative, tosink, slack, base;
  prio.safe_grow_cleared (n);
  release.safe_grow_cleared (n);
  cumulative.safe_grow_cleared (n);
  base.safe_grow_cleared (n);
  lvx_lms_path_metrics (seq, arcs, height, tosink, slack);

  /* THE MCRL DATES AS THE FIRST RELEASE DATES (author, 2026-10-27).

     MCRL (§7quinvicies.6) gives, before any placement, the dates minimising
     the cumulative register lifetime weighted by tuple width -- measured 24.2%
     below what SWO's own schedule achieves on that objective.  Those dates are
     not a schedule: they ignore resources entirely.  But they are exactly what
     a release date is for, which is to say where a task WANTS to go before
     contention is known.

     Why this is safe as an SWO parameter, and it is the same argument the
     release mechanism already rests on: the graph is untouched, so RecMII and
     the lower bound do not move, and a release only ever delays a task that is
     already dependence-ready.  No perturbation here can make the problem
     infeasible, unlike an added arc (§7octies).

     Two readings of `the first release dates around which SWO diversifies',
     separated because the release policy ZEROES every task it declines to move
     at the end of each pass -- the same erasure the branch pin had to be
     structural to survive:

       1  pass-0 only.  MCRL seeds the first construction and the policy takes
	  over from pass 1.  The literal reading of `first'.
       2  a persistent base.  The policy's `decline to move' returns a task to
	  its MCRL date rather than to zero, so every pass diversifies AROUND
	  the MCRL dates instead of away from them once and for all.

     The makespan floor is zero here: there is no achieved span yet on the
     first call, and §7quinvicies.6.2 measured the feedback as changing the
     MCRL optimum in one loop of forty-five, so nothing is lost by it.  */
  auto_vec<int> mcrl;
  if (mcrl_release)
    {
      auto_vec<int> es, ls, nodates;
      int wfirst = -1, wmake = 0, wempty = 0;
      lvx_lms_dates_in_window (seq, arcs, nodates, tosink, ii,
			       &wfirst, &wmake, &wempty, &es, &ls, 0);
      if (!wempty && es.length () == n && ls.length () == n)
	{
	  HOST_WIDE_INT mcost;
	  unsigned nn, ne;
	  auto_vec<int> md;
	  if (lvx_lms_mcrl_dates (seq, arcs, es, ls, ii, md, &mcost, &nn, &ne)
	      == LVX_NETWORK_SOLVED)
	    {
	      for (unsigned i = 0; i < n; i++)
		mcrl.safe_push (md[i]);
	      /* As FLOORS only when asked: priority 3 wants the dates for
		 ORDERING, and applying them as floors too would measure both
		 mechanisms at once.  */
	      if (mcrl_release)
		for (unsigned i = 0; i < n; i++)
		  release[i] = md[i];
	    }
	}
    }

  /* PIN THE LOOP-BACK BRANCH TO THE LAST STAGE.
     Llosa & Freudenberger factor a pipelined loop's code size into the stage
     count, the modulo expansion degree, and the stage the loop-back branch
     lands in -- the last of these because SC - BS - 1 partial epilogues are
     needed to finish the iterations an early prologue exit leaves in flight.
     A branch in the last stage needs none.  §7undecies.2a measured BS = 0 on
     73% of loops and 98% once the hardware loop is kept, so we were giving
     that term away wholesale.

     Why a floor set HERE and not left to the release policy, which already
     moves every slack task to its ALAP: the policy assigns RELEASE only at the
     END of a pass, for the next one, so a loop that schedules on its first
     pass never gets a release at all -- and most do.  The branch needs a
     structural pin, not an SWO reaction, which is also why this is cheap: no
     search, no extra pass.

     The loop-back branch is the LAST jump in SEQ, which is in chain order from
     the header, so it is the latch terminator.  An earlier jump is an exit,
     not the back edge -- the distinction §7quinquies.2a found the reference
     making with a flag of its own, and getting right.  */
  int branch_i = -1;
  int branch_floor = 0;
  if (pin > 0)
    {
      for (unsigned i = 0; i < n; i++)
	if (JUMP_P (seq[i]))
	  branch_i = (int) i;
      if (branch_i >= 0)
	{
	  if (pin == 1)
	    /* Its ALAP: as late as the dependences allow without lengthening
	       the schedule.  Cannot cost anything in II; may not reach the
	       last stage.  */
	    branch_floor = height[branch_i] + slack[branch_i];
	  else
	    {
	      /* The last stage of the dependence-only makespan.  Reaches the
		 target by construction, but is not clamped to the ALAP, so it
		 can stretch the schedule -- which is the trade to measure.  */
	      int m = 1;
	      for (unsigned i = 0; i < n; i++)
		m = MAX (m, height[i] + lvx_lms_latency (seq[i]));
	      int stages = (m + ii - 1) / ii;
	      branch_floor = MAX (stages - 1, 0) * ii;
	    }
	  if (branch_floor < 0)
	    branch_floor = 0;
	  /* A FLOOR over whatever MCRL asked for: the pin is structural and
	     must win, but MCRL may already want the branch later still.  */
	  if (release[branch_i] < branch_floor)
	    release[branch_i] = branch_floor;
	}
    }

  /* The base priority, before SWO escalates it.  The pick is ascending, so a
     merit has to be negated to become a cost.

     0  ASAP, the longest path FROM the source: issue breadth-first from the
	inputs.  What this scheduler used before the three were compared.
     1  the longest path TO the sink, negated: issue the critical path first,
	which is the classical list-scheduling choice.  THE DEFAULT: best at
	every budget measured, 65 against 68 for slack and 69 for ASAP on
	PolyBench under assumed ivdep at budget 16, and 45 against 46 and 49
	on the default levers.  The per-loop virtual best over all three is 63,
	so little is left on the table and a restart over priorities -- which
	only three loops in 128 would benefit from -- is not worth building.
     2  slack, ALAP - ASAP: issue what has least room to move, which is the
	critical path by another name on a chain but differs wherever the body
	branches, since slack accounts for the path on BOTH sides of a node.  */
  for (unsigned i = 0; i < n; i++)
    switch (lvx_cms_priority)
      {
      case 1:  base[i] = -tosink[i]; break;
      case 2:  base[i] = slack[i];   break;
      /* There was a priority 3 here, the MCRL date ascending, on the
	 reasoning that a release date is a FLOOR and so can only delay, while
	 an ORDERING costs no II by construction.  Measured and removed:
	 bit-identical to the default on 157 of 162 loops -- same span, same
	 pressure, same stages -- and it changed II in five, four of them for
	 the worse.  The MCRL dates satisfy the dependences tightly, so ordering
	 by them is a topological order the critical-path priority already
	 produces.  The effect lives entirely in the FLOORS.  §7septvicies.3. */
	/* There was a case 3 here, "latest start first", meant as a PRESSURE
	   priority after Dehnert's remedy of placing definitions close to
	   their uses.  It is removed because it is VACUOUS: ALAP is
	   makespan - tosink (see lvx_lms_path_metrics, where
	   slack = makespan - tosink - asap and asap is HEIGHT), so
	   height + slack = makespan - tosink, and ordering by that is
	   ordering by -tosink -- which is case 1.  The dumps were
	   byte-identical, as they had to be.  §7vicies.  */
      default: base[i] = height[i];  break;
      }

  /* THE RESOURCE-WEIGHTED COST TERM.  §7tricies.186.

     Priority (n) = alpha * CriticalPath (n) + beta * RWC (n), the shape of
     Fisher 1981's height heuristic extended by a resource term.  BASE is a
     COST and ascending, and -TOSINK is how the critical path enters it, so
     RWC is SUBTRACTED: a node above a long chain of contended-resource users
     becomes more urgent, not less.

     ACCUMULATED LIKE TOSINK, over omega = 0 arcs by reverse relaxation, so
     the term is the heaviest downstream CHAIN of weighted demand rather than
     a sum over immediate successors.  That is deliberate: the rule this
     implements is "a node blocking a long chain of instructions that require
     a contended resource", and an immediate-successor sum does not express a
     chain -- nor does it share a scale with TOSINK, which is itself a
     longest path.  The published form of this idea is Russell 2006's
     resource-based distance, whose r2 (i, t) = |desc (i, t)| / k_t is a
     whole-cone measure; a longest weighted path is the O(arcs) proxy for it
     and matches the traversal already here.

     The three measurements that point at this term: §7octies found every
     latency-shaped priority variant landing on the same answer for saxpy
     while `blamed=tiny' reported the real obstacle; §7tricies.184 found
     least-clock-advance taking an overflowing candidate where 3.8 of 5.3
     alternatives avoided it; and `tight=' has been computing the scarcity
     weights all along without anything reading them.  */
  if (lvx_cms_rwc > 0)
    {
      struct lvx_sched_resources dem;
      auto_vec<int> own, rwc;

      memset (&dem, 0, sizeof (dem));
      for (unsigned i = 0; i < n; i++)
	lvx_lms_resources_add (&dem, seq[i]);
      own.safe_grow_cleared (n);
      rwc.safe_grow_cleared (n);
      for (unsigned i = 0; i < n; i++)
	own[i] = rwc[i] = lvx_cms_rwc_weight (seq[i], &dem, ii);
      for (unsigned round = 0; round < 2; round++)
	for (int i = (int) n - 1; i >= 0; i--)
	  for (unsigned a = 0; a < arcs.length (); a++)
	    if (arcs[a].omega == 0 && arcs[a].src == (unsigned) i)
	      {
		int cand = own[i] + rwc[arcs[a].dst];

		if (rwc[i] < cand)
		  rwc[i] = cand;
	      }
      for (unsigned i = 0; i < n; i++)
	base[i] -= (lvx_cms_rwc * rwc[i]) / 256;
      if (lvx_cms_rwc_probe && dump_file && lvx_lms_probe)
	for (unsigned i = 0; i < n; i++)
	  lvx_lms_dump (";; lvx cms:   RWC ii=%d insn=%u own=%d chain=%d"
			" tosink=%d base=%d\n", ii, i, own[i], rwc[i],
			tosink[i], base[i]);
    }

  for (unsigned i = 0; i < n; i++)
    prio[i] = base[i];

  /* BEST-OF-N RATHER THAN FIRST-SUCCESS.  §7tricies.174.

     SWO's budget was a budget for FAILURE: the first pass whose projection
     verified returned, and the rest went unspent.  But VBSS is `run I times
     and keep the best' (Lihu & Holban: "After a given number of runs, the best
     solution is extracted"), so CMS implemented the sampling and not the
     selection -- and §7tricies.173 measured a seed band of 368-376 at s.d. 3.1
     under the cycle-based scheme, which is a band a selection can exploit.

     THE CRITERION, BESIDES II, IS THE LOCAL SCHEDULE LENGTH (author).  II is
     already settled by the search, which is bottom-up and takes the first
     feasible one; among schedules AT that II the span is what decides the stage
     count, hence the prologue and epilogue code, the number of values in
     flight, and the modulo expansion degree.  Note it pulls AGAINST the
     `nstages>=3' census figure by construction -- fewer stages follow from a
     shorter span -- so that metric is expected to FALL here while the code
     improves, and must not be read as a regression.  */
  /* WHICH TASKS SIT ON THE CRITICAL RECURRENCE.  §7tricies.176.

     The release lever trades II against span (§7tricies.175): holding every
     task with slack to its ALAP reaches the lowest II but stretches the
     schedule, while holding selectively keeps the span tight and gives II
     back.  The author's reading of why: a task ON the critical recurrence is
     on the longest cycle, so delaying it lengthens the schedule directly,
     whereas delaying a task OFF it frees issue slots for free.  So hold
     exactly the off-recurrence tasks.

     CRIT comes from lvx_lms_bounds, which already computes the critical
     circuit for the RecMII bound and reports it in the `RecMII circuit' dump;
     entries are task indices, with -1 where a circuit node is not a task.
     When it is unavailable the set is empty, so policy 6 degenerates to
     policy 4 -- hold everything with slack -- which is the right fallback: an
     unknown recurrence should not stop the lever working.  */
  auto_vec<bool> oncrit;
  oncrit.safe_grow_cleared (n);
  unsigned ncrit = 0;
  /* CRIT HOLDS ARC INDICES, NOT TASK INDICES.  lvx_lms_bounds maps the PLC
     critical circuit back onto ARCS -- "PLC arc N + J is ARCS[J], and a root
     arc, which cannot be part of a circuit, maps to -1" -- so an entry is an
     index into ARCS and a task is ON the recurrence when it is an ENDPOINT of
     one of those arcs.  Reading the entries as task indices (the first version
     of this) silently dropped every arc index >= n on the bounds test and
     marked unrelated tasks with the rest, which made the measured set 9% of
     the body and the policy indistinguishable from release=4.  */
  if (crit != NULL)
    for (unsigned q = 0; q < crit->length (); q++)
      {
	int a = (*crit)[q];
	if (a < 0 || (unsigned) a >= arcs.length ())
	  continue;
	unsigned sr = arcs[a].src, ds = arcs[a].dst;
	if (sr < n && !oncrit[sr])
	  oncrit[sr] = true, ncrit++;
	if (ds < n && !oncrit[ds])
	  oncrit[ds] = true, ncrit++;
      }

  auto_vec<int> bestsig;
  int bestspan = -1;
  bool have_best = false;

  if (dump_file && lvx_lms_probe && lvx_cms_release_policy == 6)
    lvx_lms_dump (";; lvx cms: ONCRIT ii=%d n=%u oncrit=%u holdable=%u\n",
		  ii, n, ncrit, n - ncrit);

  for (int pass = 0; pass < budget; pass++)
    {
      unsigned sqk_named = 0, sqk_movable = 0;
      blame.truncate (0);
      blame.safe_grow_cleared (n);
      culprit.truncate (0);
      culprit.safe_grow_cleared (n);

      (*passes)++;
      /* PASS 0 IS THE DETERMINISTIC DRAW, every later pass a VBSS sample.
	 Keeping pass 0 exact means budget 1 reproduces the pre-VBSS scheduler
	 and sampling can only add attempts.  The seed is a function of the
	 loop's SHAPE and the pass index -- never an address, so the same
	 function compiles to the same code twice, which a stochastic scheduler
	 has to guarantee before it can be a default.  */
      /* RANDOMISED DELAY RELATIVE TO THE READY DATE.  §7tricies.172.
	 The diversification belongs on the DELAY, not on the priority: under a
	 time-monotonic constructor (-mcms-select=1) the schedule class is
	 decided by which tasks are held back past their ready date, because
	 that is the only way a unit is left idle while something is eligible
	 -- the non-delay restriction of Kolisch & Hartmann's parallel SGS.
	 So SWO's analysis chooses WHICH tasks are delayed, exactly as its
	 blame and culprit arrays already do, and the draw chooses HOW LONG.
	 Seeded from the loop's shape and the pass, like VBSS.  */
      unsigned rstate = 0;
      if (lvx_cms_release_rand > 0 && pass > 0)
	{
	  rstate = 0x7f4a7c15u ^ (n * 0x9e3779b9u)
		   ^ ((unsigned) ii * 0x85ebca6bu)
		   ^ ((unsigned) pass * 0xc2b2ae35u)
		   ^ ((unsigned) lvx_cms_vbss_seed * 27644437u);
	  if (rstate == 0)
	    rstate = 0x7f4a7c15u;
	}
      unsigned vseed = 0;
      if (lvx_cms_vbss > 0 && pass > 0)
	{
	  vseed = 0x9e3779b9u ^ (n * 2654435761u)
		  ^ ((unsigned) ii * 40503u) ^ ((unsigned) pass * 2246822519u)
		  ^ ((unsigned) lvx_cms_vbss_seed * 0x85ebca6bu);
	  if (vseed == 0)
	    vseed = 0x9e3779b9u;
	}
      if (lvx_cms_converge (seq, arcs, prio, release, ii, delta, sigma,
			    &blame, &culprit, vseed))
	{
	  /* ACCEPTANCE IS LMS'S, NOT THE ENGINE'S.  §7tricies.222.  Verifying
	     and colouring are what any engine must be judged by, so they are
	     one call here and the same call elsewhere.  */
	  const char *bad = lvx_lms_accept_p (seq, arcs, ii, sigma, livein,
					      &blame, &culprit);
	  if (!bad)
	    {
	      if (!lvx_cms_best_of_n)
		{
		  return true;
		}
	      /* Keep the shortest local schedule seen, and carry on
		 spending the budget.  SIGMA is overwritten by every later
		 construction, so the winner has to be copied out.  */
	      int sp = lvx_lms_sigma_span (sigma);
	      if (!have_best || sp < bestspan)
		{
		  bestspan = sp;
		  have_best = true;
		  bestsig.truncate (0);
		  for (unsigned q = 0; q < sigma.length (); q++)
		    bestsig.safe_push (sigma[q]);
		}
	      if (dump_file && lvx_lms_probe)
		lvx_lms_dump (";; lvx cms: BESTOFN ii=%d pass=%d span=%d"
			      " best=%d\n", ii, pass, sp, bestspan);
	    }
	  else
	    *last_bad = bad;
	}

      /* ANALYZE: nothing squeaked, so no parameter change would differ and
	 another pass would reconstruct the same schedule.  */
      bool squeaked = false;
      for (unsigned i = 0; i < n; i++)
	if (blame[i] > 0)
	  squeaked = true;
      if (!squeaked)
	{
	  /* THE PREMISE IS DETERMINISM, and VBSS breaks it.  §7tricies.174.
	     "Another pass would reconstruct the same schedule" holds only
	     while the constructor is a function of its parameters; under
	     value-biased sampling a fresh draw differs at identical
	     parameters, so with selection enabled the search must continue.
	     This exit is also why VBSS saturated at budget 4 (§7tricies.170):
	     on a loop that never squeaked it was taking exactly one draw.  */
	  if (!(lvx_cms_best_of_n && lvx_cms_vbss > 0))
	    {
	      /* BREAK rather than RETURN FALSE: a recorded best is a success,
		 and this exit means `no more worth sampling', not `failed'.
		 Returning false here discarded it -- measured as 132 loops of
		 tsvc.c losing their schedule entirely, because on a winning
		 pass nothing squeaks by construction.  */
	      if (have_best)
		break;
	      return false;
	    }
	}

      /* PRIORITIZE, first lever: escalate, cumulatively.  */
      for (unsigned i = 0; i < n; i++)
	{
	  cumulative[i] += blame[i];
	  prio[i] = base[i] - cumulative[i];
	}

      /* PRIORITIZE, second lever: delay what has slack and did not squeak.

	 ASAP is the intra-iteration longest path already in HEIGHT.  ALAP is
	 the backward pass against the makespan: a task's latest start is the
	 earliest over its successors of their latest start minus the arc's
	 latency.  Only omega = 0 arcs take part, the cross-iteration ones being
	 accounted for by the modulo constraint rather than by this slack.  */
      /* PRIORITIZE, second lever: the release dates.

	 Policy 1, every unblamed task with slack to its ALAP, is a GLOBAL bias
	 change -- it turns an ASAP-leaning construction into an ALAP-leaning
	 one for everything that can move, which compacts the whole schedule.

	 Policy 2 aims instead: move only a CULPRIT, a task that held the
	 counter which displaced someone, and only when it has slack and was
	 not itself displaced -- delaying a victim would make the contention
	 worse.  It is the better-motivated rule and it measures WORSE, which
	 is why both are kept selectable rather than one being replaced.  On
	 PolyBench under assumed ivdep at budget 16 it left the 32 loops it was
	 designed for exactly as they were, 32 still missing the bound, and
	 degraded the saturated ones from 15 missing to 19; total excess 65 to
	 79, over more passes.

	 The likely reason is that it has no damping.  Release is recomputed
	 from scratch each pass, so a culprit delayed in one pass becomes a
	 victim in the next, resets to zero, and is a culprit again -- a limit
	 cycle, visible in the non-monotone excess across budgets (90, 95, 84,
	 79).  Priority escalates cumulatively and does not oscillate this way.
	 Damping the release the same way is the obvious next thing to try.  */
      /* Policy 3, DAMPED: the integral of accumulated culpability rather than a
	 proportional response to an instantaneous signal.

	 Policy 2 oscillates because its trigger is self-defeating -- it moves a
	 task BECAUSE it is a culprit, and moving it stops it being one, so the
	 condition is measured on the very quantity the action destroys.  Three
	 changes together:

	   MEMORY     GUILT accumulates and is never reset within an II attempt,
		      the way CUMULATIVE does for priority, so a task cannot
		      snap back to zero the pass after it is moved.
	   SMALL STEP the delay is one cycle per unit of guilt, clamped to the
		      task's slack, so a repeat offender drifts late instead of
		      jumping the whole way to its ALAP into a region nothing
		      has measured.
	   RATCHET    release only ever rises within the attempt, which removes
		      the snap-back half of the cycle outright.

	 Note what is NOT conditioned on: instantaneous blame.  A task that was
	 itself displaced this pass keeps whatever delay its history earned,
	 because withdrawing it is what closed the loop through the
	 measurement.  */
      /* THE MOVE'S MAGNITUDE.  §7tricies.169.  The release lever's amount has
	 always been the ALAP: every moved task goes all the way to the wall.
	 That is a BINARY move driven by a GRADED signal -- `when - earliest'
	 is summed into BLAME and then used only to decide WHETHER a task
	 moves, never HOW FAR.  An all-the-way move is the kind that
	 overshoots, vacates the contended cycle and creates contention
	 elsewhere, which is a candidate mechanism for the oscillation Kramer
	 2008 documents and §7tricies.129 measured here.

	 Policy 3 is the one arm that ever graded the amount
	 (MIN (slack, guilt)) and it measured WORSE -- but it also RATCHETED,
	 so what was refuted is graded-with-ratchet, and graded-without-ratchet
	 is an untested cell.  See docs/schedule-search-diversity.md.

	 DMAX is the worst delay any task suffered this pass: moving a task
	 further than that cannot be needed to relieve the contention actually
	 observed.  Grade 1 carries no magnitude at all, so the two arms
	 separate `do not overshoot' from `use the measurement'.  */
      int dmax = 0;
      for (unsigned i = 0; i < n; i++)
	if (blame[i] > dmax)
	  dmax = blame[i];

      for (unsigned i = 0; i < n; i++)
	{
	  bool move = false;
	  switch (lvx_cms_release_policy)
	    {
	    case 0:  move = false; break;
	    case 5:
	      /* HUFF'S STRETCHABLE-SIDE DECISION (§7tervicies, §7quatervicies).
		 Placing an operation EARLY stretches the values it defines;
		 placing it LATE stretches the values it reads.  Huff decides
		 per operation which side is the stretchable one and places it
		 away from that side -- which is what makes the heuristic
		 bidirectional rather than the global bias §7vicies found
		 vacuous.

		 Here "place late" is a release date at the ALAP, which is the
		 lever §7octies already has, so the decision costs nothing
		 structurally and leaves the graph alone.

		 The arithmetic: moving I from its ASAP to its ALAP shortens
		 each out-value's span by SLACK and lengthens each in-value's
		 by SLACK, so the net is SLACK * (in_count - out_count) and
		 going late pays iff OUT_COUNT > IN_COUNT.  Huff's refinement
		 is to count only the values this operation can ACTUALLY
		 stretch: a consumer U cannot stretch its input when
		 Estart(def) + MinLT >= omega*II + Lstart(U), because something
		 else already holds the value that long.  MinLT is taken as
		 max over the out-arcs of (omega*II + theta), a lower bound on
		 Huff's MinDist version -- which makes the pruning
		 conservative, counting a value stretchable when in doubt.  */
	      {
		int minlt = 0;
		for (unsigned a = 0; a < arcs.length (); a++)
		  if (arcs[a].src == i && arcs[a].kind == LVX_ARC_REG_RAW)
		    {
		      int c = (int) arcs[a].omega * ii + (int) arcs[a].theta;
		      if (c > minlt)
			minlt = c;
		    }
		int nout = 0, nin = 0;
		for (unsigned a = 0; a < arcs.length (); a++)
		  {
		    if (arcs[a].kind != LVX_ARC_REG_RAW)
		      continue;
		    if (arcs[a].src == i)
		      {
			unsigned u = arcs[a].dst;
			int lstart_u = height[u] + slack[u];
			if (height[i] + minlt
			    < (int) arcs[a].omega * ii + lstart_u)
			  nout++;		/* U can stretch it */
		      }
		    else if (arcs[a].dst == i)
		      nin++;
		  }
		move = (nout > nin && slack[i] > 0);
	      }
	      break;
	    /* VACUOUS UNLESS -mcms-squeak IS 1 OR 2.  §7tricies.188: CULPRIT
	       was filled from two sources, the window scan over the blocking
	       resource and the oracle's named instructions.  The scan is
	       retired with the forward walk it read, so at squeak=0 CULPRIT
	       is always empty and this policy never moves anything.  */
	    case 2:  move = (culprit[i] > 0 && blame[i] == 0 && slack[i] > 0);
		     break;
	    case 4:  move = (slack[i] > 0); break;	/* no selectivity at all */
	    /* OFF THE CRITICAL RECURRENCE ONLY.  §7tricies.176.  Delaying a
	       task on the longest cycle lengthens the schedule; delaying one
	       off it buys issue slots without that cost.  */
	    case 6:  move = (slack[i] > 0 && !oncrit[i]); break;
	    default: move = (blame[i] == 0 && slack[i] > 0); break;
	    }

	  /* THE SQUEAK REACHES THE ACTIVE ARM.  §7tricies.129.
	     Merging the oracle's verdict into CULPRIT is inert on its own,
	     because the default policy -- the one that is actually on -- does
	     not read CULPRIT: it moves every UNBLAMED task with slack, so an
	     instruction that both overflowed a residue and was displaced
	     during construction is excluded by its own blame, which is exactly
	     backwards.  At squeak=1 the oracle's verdict is OR-ed in, so a
	     named task with slack moves whatever its blame; at squeak=2 the
	     oracle REPLACES the arm, and the only tasks that move are the ones
	     a failed projection actually accused.  */
	  if (lvx_cms_squeak == 1 && culprit[i] > 0 && slack[i] > 0)
	    move = true;
	  else if (lvx_cms_squeak == 2
		   && lvx_cms_release_policy != 0
		   && lvx_cms_release_policy != 3)
	    move = (culprit[i] > 0 && slack[i] > 0);
	  /* Its ALAP when moved.  When not moved, back to the MCRL date under
	     the persistent-base reading and to nothing under the pass-0 one. */
	  int home = (mcrl_release == 2 && !mcrl.is_empty ()
		      ? mcrl[i] : 0);
	  int amount = slack[i];
	  /* The draw, when asked for: uniform over [0, slack], so a task may
	     be held anywhere between its ready date and its ALAP.  Zero is a
	     legal outcome and means this pass declines to delay it, which is
	     what makes the search cover both schedule classes rather than
	     replacing one restriction with another.  */
	  if (rstate != 0 && lvx_cms_release_rand > 0 && slack[i] > 0)
	    {
	      unsigned span = (unsigned) slack[i] + 1;
	      unsigned draw = lvx_cms_rnd (&rstate) % span;
	      if (lvx_cms_release_rand >= 2)
		{
		  /* Biased towards the ALAP: the best deterministic policy
		     measured is `every slack task to its ALAP' (release=4
		     under select=1), so the draw should sit near that end and
		     sample away from it, not around the middle.  */
		  unsigned d2 = lvx_cms_rnd (&rstate) % span;
		  if (d2 > draw)
		    draw = d2;
		}
	      amount = (int) draw;
	      goto grade_done;
	    }
	  switch (lvx_cms_release_grade)
	    {
	    /* Half the slack: a partial move carrying no magnitude.  */
	    case 1: amount = (slack[i] + 1) / 2; break;
	    /* The measurement.  DMAX == 0 means nothing was displaced during
	       construction, and then this declines to move at all -- which is
	       faithful to the signal but cannot make progress when the
	       projection, not the construction, is what refused.  */
	    case 2: amount = MIN (slack[i], dmax); break;
	    /* The measurement with a floor, so a pass always moves something
	       even when the construction reported no delay.  */
	    case 3: amount = MIN (slack[i], MAX (dmax, 1)); break;
	    default: break;			/* 0: the ALAP, as before.  */
	    }
	grade_done:
	  release[i] = move ? height[i] + amount : home;
	  if (culprit[i] > 0)
	    {
	      sqk_named++;
	      if (slack[i] > 0)
		sqk_movable++;
	    }
	}

      /* CAN THE SQUEAK BE ACTED ON AT ALL?  §7tricies.129.  The release lever
	 only ever DELAYS a task, and only one that has slack, so a named task
	 at zero slack is a failure the analysis can see and the perturbation
	 cannot touch -- the next construction is then identical and SWO spins.
	 Reported because it is the question that decides whether the signal is
	 useful or merely correct.  */
      if (dump_file && lvx_lms_probe && lvx_cms_squeak != 0 && sqk_named)
	lvx_lms_dump (";; lvx cms: fn=%s II=%d SQUEAK pass=%d named=%u"
		      " movable=%u\n", current_function_name (), ii, pass,
		      sqk_named, sqk_movable);

      /* The branch pin is structural, so it survives the policy's
	 reassignment above -- which sets RELEASE to 0 for every task it
	 declines to move, and would otherwise undo the pin on the first
	 pass that blames the branch.  */
      if (branch_i >= 0 && release[branch_i] < branch_floor)
	release[branch_i] = branch_floor;
    }

  if (have_best)
    {
      sigma.truncate (0);
      for (unsigned q = 0; q < bestsig.length (); q++)
	sigma.safe_push (bestsig[q]);
      return true;
    }

  return false;
}
