/* Swing Modulo Scheduling for LVX: one engine of the LMS framework.
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

/* SWING MODULO SCHEDULING, after Llosa, Gonzalez, Ayguade and Valero,
   "Swing Modulo Scheduling: A Lifetime-Sensitive Approach", PACT'96.

   WHAT IT IS, in one paragraph.  Order the nodes once, then place each in
   turn as close as possible to whichever of its neighbours is already
   placed -- as soon as possible when only predecessors are, as late as
   possible when only successors are.  That bi-directional placement is what
   shortens lifetimes, and the ORDER is what makes it work: it is built so
   that a node almost always has neighbours on one side only, which happens
   for every node except one per recurrence.  The idea of placing early or
   late is Huff's (PLDI 1993), the neighbour-driven form is HRMS's (1995),
   and SMS's own contribution is the ordering, which considers how critical
   each node is where HRMS did not.  See docs/swing-modulo-scheduling.md.

   WHY IT IS HERE.  CMS places every task as early as its monotone clock
   allows, which is exactly the top-down strategy the paper opens by
   demolishing, and the repairs measured against that -- stage scheduling,
   range splitting, spilling, Huff's stretchable side as a release policy --
   each fixed nothing (cms-foundations.md 7vicies to 7quatervicies).  The
   lever those measurements kept pointing at is bi-directional placement,
   which is a different scheduler and not a priority function.  This is it.

   WHAT THIS IS A PORT OF.  LLVM's MachinePipeliner, which is the faithful
   implementation of the paper -- it keeps the distance terms in the node
   functions, the real subset test in Pred_L/Suc_L, and the placement
   trichotomy, where GCC's own modulo-sched.cc departs from all three.
   Nothing here comes from GCC's.  The pieces are named after theirs so the
   two can be read side by side: computeNodeFunctions, findCircuits,
   computeNodeOrder, checkValidNodeOrder, computeStart, insert.

   WHAT IS DELIBERATELY NOT PORTED.  Everything that is pre-RA, SSA or
   Hexagon-specific: the PHI machinery, registerPressureFilter (we have the
   colouring in lvx_lms_accept_p instead), the register-pressure detector,
   orderDependence and fixupRegisterOverlaps (lvx_lms_slot_order solves that
   later and on renamed registers), and the code generator.  LLVM's
   ResourceManager is replaced by the LVX modulo reservation table.

   THE ONE CORRECTION TO THE PORT.  LLVM's calculateRecMII assumes every
   circuit carries distance one, so a circuit at distance two is
   over-constrained by a factor of two.  Here each node set's recurrence
   bound is the exact period of its induced subgraph, from the parametric
   solver lvx-plc.cc -- which is the same machinery that computes RecMII for
   the loop, validated against GCC's own ddg on 336 independent agreements
   (cms-foundations.md 7nonies).  -mlms-sms-recmii=llvm restores LLVM's
   number, so a divergence can be isolated.  */

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
/* THE NODE SET, which is LLVM's NodeSet and the paper's "set" of the
   two-level ordering: a recurrence and whatever joins it to the sets already
   taken, or -- for the nodes in no recurrence -- a connected component.  */

struct lvx_sms_set
{
  auto_vec<unsigned> nodes;	/* in insertion order, as LLVM's SetVector */
  int recmii;			/* the set's recurrence bound */
  int maxmov;			/* max mobility over the set */
  int maxdepth;			/* max depth over the set */
  bool rec;			/* built from a circuit, not from a component */
};

/* The engine's per-loop state: everything the ordering needs, computed once
   however many initiation intervals are tried.  That is the paper's own cost
   argument -- "the nodes are ordered only once, even if the scheduling step
   has to do several trials" -- and it is why lvx_lms_engine has two phases.  */

struct lvx_sms_state
{
  const struct lvx_lms_problem *p;
  unsigned n;
  int mii;			/* the node functions are computed at MII */

  /* The node functions of the paper's section 4.1, LLVM's
     computeNodeFunctions.  */
  auto_vec<int> asap, alap, mov, depth, height, zldepth, zlheight;

  /* A topological order of the omega = 0 subgraph, and the inverse.  LLVM
     gets this from the DAG builder; here it is computed, because an arc
     carries its distance rather than being a different kind of edge.  */
  auto_vec<unsigned> topo, topopos;

  /* The adjacency Johnson's algorithm walks, as CSR.  */
  auto_vec<unsigned> adj_start, adj_list;

  auto_vec<lvx_sms_set *> sets;
  auto_vec<unsigned> order;	/* the answer prepare exists to produce */
  bool ok;			/* false if the loop defeated the ordering */
};

/* THE GRAPH THE ORDERING SEES, AND WHY IT IS NOT THE WHOLE GRAPH.

   The paper is explicit about its dependence graph: "Only data dependences
   (flow, anti and output-dependences) are included since the type of loops
   that SMS can handle only include one branch instruction at the end"
   (section 4.1).  LVX's graph is not that graph.  It is a POST-REGISTER-
   ALLOCATION graph of a body that may hold several branches, so
   lvx_lms_build_graph adds ctrl-order, ctrl-store, ctrl-liveout and
   ctrl-backedge arcs, and a branch is a hub joined to much of the body.

   Measured, feeding those to the ordering is what breaks it.  The two-level
   order exists to guarantee that a node has placed neighbours on ONE side
   only -- every node but one per recurrence -- and a control hub turns
   ordinary chains into diamonds, so that guarantee fails for a third of the
   nodes: syrk's four loops reported 3, 1, 2 and 5 violations of
   checkValidNodeOrder out of 10, 6, 6 and 11 nodes, and the placement then
   met empty windows and gave the interval up.  The first loop needed II 6
   where its bound was 2.

   So the ORDERING reads the paper's graph -- the register and memory
   dependences -- and the PLACEMENT and lvx_lms_verify read all of it.  That
   is not a relaxation: a control arc still constrains where a node may go
   and is still checked before the schedule is accepted.  It only stops a
   control arc from deciding what is ordered next, which is a question about
   the program's data flow.  */

static inline bool
lvx_sms_data_arc_p (const lvx_lms_arc &a)
{
  return (a.kind == LVX_ARC_REG_RAW || a.kind == LVX_ARC_REG_WAR
	  || a.kind == LVX_ARC_REG_WAW || a.kind == LVX_ARC_MEM_INTRA
	  || a.kind == LVX_ARC_MEM_CROSS);
}

/* Does ARC take part in the node functions?  LLVM's
   SwingSchedulerDDGEdge::ignoreDependence (IgnoreAnti = true): an anti
   dependence or a loop-carried one is left out, which is what keeps ASAP and
   ALAP finite without having to pick a backward edge to drop.  A control arc
   is left out for the reason above.  */

static inline bool
lvx_sms_ignore_for_funcs (const lvx_lms_arc &a)
{
  return (!lvx_sms_data_arc_p (a) || a.omega != 0
	  || a.kind == LVX_ARC_REG_WAR);
}

/* And the weaker test LLVM uses when walking the order (ignoreDependence
   (false)): there the loop-carried edges stay, and only the control arcs
   go.  */

static inline bool
lvx_sms_ignore_for_order (const lvx_lms_arc &a)
{
  return !lvx_sms_data_arc_p (a);
}

/* A TOPOLOGICAL ORDER OF THE OMEGA = 0 SUBGRAPH.

   It has one: an intra-iteration cycle would be a dependence cycle inside one
   iteration, which lvx_lms_bounds reports as an infeasible circuit before any
   engine is asked.  Kahn's algorithm, and if a cycle does appear the state is
   marked not-ok and the engine declines rather than looping.  */

static bool
lvx_sms_topo_order (struct lvx_sms_state *st)
{
  unsigned n = st->n;
  const auto_vec<lvx_lms_arc> &arcs = *st->p->arcs;
  auto_vec<int> indeg;

  indeg.safe_grow_cleared (n);
  for (unsigned a = 0; a < arcs.length (); a++)
    if (arcs[a].omega == 0 && arcs[a].src != arcs[a].dst)
      indeg[arcs[a].dst]++;

  for (unsigned i = 0; i < n; i++)
    if (indeg[i] == 0)
      st->topo.safe_push (i);

  for (unsigned k = 0; k < st->topo.length (); k++)
    {
      unsigned v = st->topo[k];
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].omega == 0 && arcs[a].src == v && arcs[a].dst != v
	    && --indeg[arcs[a].dst] == 0)
	  st->topo.safe_push (arcs[a].dst);
    }

  if (st->topo.length () != n)
    return false;

  st->topopos.safe_grow_cleared (n);
  for (unsigned k = 0; k < n; k++)
    st->topopos[st->topo[k]] = k;
  return true;
}

/* THE NODE FUNCTIONS, section 4.1 of the paper and LLVM's
   computeNodeFunctions.

     ASAP (u) = max over predecessors of ASAP (v) + theta - omega * MII
     ALAP (u) = min over successors   of ALAP (v) - theta + omega * MII
     MOV  (u) = ALAP - ASAP, zero on the critical path
     D    (u) = depth,  the longest latency-weighted path from a source
     H    (u) = height, the same to a sink

   The distance terms are the half GCC's implementation drops, which is why
   its node functions describe the acyclic projection rather than the periodic
   graph; they are kept here because the port is of LLVM's.

   ZLDEPTH and ZLHEIGHT count consecutive ZERO-LATENCY edges and are LLVM's
   addition, used only to break ties in the ordering before mobility does.

   Computed at MII and not at the candidate interval, deliberately: the order
   must be stable across the ladder, or a difference between one interval and
   the next would be a different algorithm rather than a different placement.

   A NOTE ON WHAT WAS NOT DONE.  lvx_lms_dates_in_window would give exact
   Estart and Lstart from the parametric solver, over the whole bi-valued
   graph and with no edge dropped -- strictly better bounds than these.  They
   are not used here because the ORDER is defined by these functions, and the
   first version of an engine should be the thing it claims to be; swapping
   them is a lever to measure, not a correction to make silently.  */

static void
lvx_sms_node_functions (struct lvx_sms_state *st)
{
  unsigned n = st->n;
  const auto_vec<lvx_lms_arc> &arcs = *st->p->arcs;
  const auto_vec<rtx_insn *> &seq = *st->p->seq;
  int mii = st->mii;

  st->asap.safe_grow_cleared (n);
  st->alap.safe_grow_cleared (n);
  st->mov.safe_grow_cleared (n);
  st->depth.safe_grow_cleared (n);
  st->height.safe_grow_cleared (n);
  st->zldepth.safe_grow_cleared (n);
  st->zlheight.safe_grow_cleared (n);

  int maxasap = 0;
  for (unsigned k = 0; k < n; k++)
    {
      unsigned u = st->topo[k];
      int asap = 0, zld = 0, dep = 0;

      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].dst == u)
	  {
	    unsigned v = arcs[a].src;
	    int theta = (int) arcs[a].theta;

	    if (theta == 0 && arcs[a].omega == 0)
	      zld = MAX (zld, st->zldepth[v] + 1);
	    if (lvx_sms_ignore_for_funcs (arcs[a]))
	      continue;
	    asap = MAX (asap, st->asap[v] + theta - (int) arcs[a].omega * mii);
	    dep = MAX (dep, st->depth[v] + theta);
	  }
      st->asap[u] = asap;
      st->zldepth[u] = zld;
      st->depth[u] = dep;
      maxasap = MAX (maxasap, asap);
    }

  for (int k = (int) n - 1; k >= 0; k--)
    {
      unsigned u = st->topo[k];
      int alap = maxasap, zlh = 0, hgt = 0;

      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].src == u)
	  {
	    unsigned v = arcs[a].dst;
	    int theta = (int) arcs[a].theta;

	    if (theta == 0 && arcs[a].omega == 0)
	      zlh = MAX (zlh, st->zlheight[v] + 1);
	    if (lvx_sms_ignore_for_funcs (arcs[a]))
	      continue;
	    alap = MIN (alap, st->alap[v] - theta + (int) arcs[a].omega * mii);
	    hgt = MAX (hgt, st->height[v] + theta);
	  }
      st->alap[u] = alap;
      st->zlheight[u] = zlh;
      st->height[u] = hgt;
    }

  for (unsigned i = 0; i < n; i++)
    st->mov[i] = st->alap[i] - st->asap[i];

  if (dump_file && lvx_lms_probe && lvx_sms_trace)
    for (unsigned i = 0; i < n; i++)
      lvx_lms_dump (";; lvx sms: node %u insn %d ASAP=%d ALAP=%d MOV=%d"
		    " D=%d H=%d ZLD=%d ZLH=%d\n", i, INSN_UID (seq[i]),
		    st->asap[i], st->alap[i], st->mov[i], st->depth[i],
		    st->height[i], st->zldepth[i], st->zlheight[i]);
}

/* JOHNSON'S CIRCUIT ENUMERATION, LLVM's SwingSchedulerDAG::Circuits.

   The paper's first-level ordering wants the RECURRENCES, most constrained
   first.  LLVM enumerates circuits rather than strongly connected components
   -- GCC's modulo-sched.cc takes the SCC of each back-arc instead, which is
   coarser -- and bounds the search at MaxPaths, because the number of
   circuits is exponential in general and the ordering only needs the
   expensive ones.

   The adjacency leaves out anti dependences, as LLVM's
   createAdjacencyStructure does: an intra-iteration anti dependence closes a
   circuit that is an artefact of having to pick an order for two unrelated
   writes, not a recurrence the schedule has to respect at a period.

   A circuit is recorded only when it has no back edge other than the one that
   closes it (LLVM's HasBackedge), which is the same "simple cycle with a
   single back-arc" restriction GCC's ddg.cc makes, and it is what keeps the
   enumeration to the circuits an ordering can use.  */

#define LVX_SMS_MAX_PATHS 5

struct lvx_sms_circuits
{
  struct lvx_sms_state *st;
  auto_vec<bool> blocked;
  auto_vec<unsigned> stack;
  /* B[v] as a flat bitmap: blist[v * n + w].  */
  auto_vec<bool> blist;
  int npaths;
};

static void
lvx_sms_unblock (struct lvx_sms_circuits *c, unsigned u)
{
  unsigned n = c->st->n;

  c->blocked[u] = false;
  for (unsigned w = 0; w < n; w++)
    if (c->blist[u * n + w])
      {
	c->blist[u * n + w] = false;
	if (c->blocked[w])
	  lvx_sms_unblock (c, w);
      }
}

static bool
lvx_sms_circuit (struct lvx_sms_circuits *c, unsigned v, unsigned s,
		 bool has_backedge)
{
  struct lvx_sms_state *st = c->st;
  unsigned n = st->n;
  bool f = false;

  c->stack.safe_push (v);
  c->blocked[v] = true;

  for (unsigned k = st->adj_start[v]; k < st->adj_start[v + 1]; k++)
    {
      unsigned w = st->adj_list[k];

      if (c->npaths > LVX_SMS_MAX_PATHS)
	break;
      if (w < s)
	continue;
      if (w == s)
	{
	  if (!has_backedge)
	    {
	      lvx_sms_set *ns = new lvx_sms_set;

	      for (unsigned q = 0; q < c->stack.length (); q++)
		ns->nodes.safe_push (c->stack[q]);
	      ns->rec = true;
	      ns->recmii = 0;
	      ns->maxmov = 0;
	      ns->maxdepth = 0;
	      st->sets.safe_push (ns);
	    }
	  f = true;
	  c->npaths++;
	  break;
	}
      if (!c->blocked[w]
	  && lvx_sms_circuit (c, w, s,
			      st->topopos[w] < st->topopos[v]
			      ? true : has_backedge))
	f = true;
    }

  if (f)
    lvx_sms_unblock (c, v);
  else
    for (unsigned k = st->adj_start[v]; k < st->adj_start[v + 1]; k++)
      {
	unsigned w = st->adj_list[k];

	if (w < s)
	  continue;
	c->blist[w * n + v] = true;
      }

  c->stack.pop ();
  return f;
}

static void
lvx_sms_find_circuits (struct lvx_sms_state *st)
{
  unsigned n = st->n;
  const auto_vec<lvx_lms_arc> &arcs = *st->p->arcs;
  struct lvx_sms_circuits c;

  /* The adjacency, as CSR.  */
  st->adj_start.safe_grow_cleared (n + 1);
  for (unsigned i = 0; i < n; i++)
    {
      st->adj_start[i] = st->adj_list.length ();
      for (unsigned j = 0; j < n; j++)
	{
	  if (i == j)
	    continue;
	  for (unsigned a = 0; a < arcs.length (); a++)
	    if (arcs[a].src == i && arcs[a].dst == j
		&& lvx_sms_data_arc_p (arcs[a])
		&& arcs[a].kind != LVX_ARC_REG_WAR)
	      {
		st->adj_list.safe_push (j);
		break;
	      }
	}
    }
  st->adj_start[n] = st->adj_list.length ();

  c.st = st;
  c.npaths = 0;
  c.blocked.safe_grow_cleared (n);
  c.blist.safe_grow_cleared (n * n);
  for (unsigned i = 0; i < n; i++)
    {
      for (unsigned q = 0; q < n; q++)
	{
	  c.blocked[q] = false;
	  for (unsigned w = 0; w < n; w++)
	    c.blist[q * n + w] = false;
	}
      c.stack.truncate (0);
      c.npaths = 0;
      lvx_sms_circuit (&c, i, i, false);
    }
}

/* THE RECURRENCE BOUND OF ONE SET, and this is the one place the port
   corrects LLVM rather than reproducing it.

   LLVM's calculateRecMII takes the circuit's total latency and divides by a
   distance it assumes to be one, so a circuit carrying distance two is
   over-constrained by a factor of two.  The exact answer is the PERIOD of the
   subgraph the set induces, which is what lvx-plc.cc computes parametrically
   for the loop as a whole -- the same solver, on fewer nodes.

   -mlms-sms-recmii=llvm restores LLVM's number, so that a divergence between
   the two implementations can be attributed rather than guessed at.  */

static int
lvx_sms_set_recmii (struct lvx_sms_state *st, lvx_sms_set *ns)
{
  const auto_vec<lvx_lms_arc> &arcs = *st->p->arcs;
  unsigned m = ns->nodes.length ();
  auto_vec<int> idx;

  if (m == 0)
    return 0;

  if (lvx_sms_recmii_llvm)
    {
      /* LLVM: the latency around the circuit, distance assumed one.  */
      HOST_WIDE_INT delay = 0;
      for (unsigned q = 0; q < m; q++)
	{
	  unsigned u = ns->nodes[q], v = ns->nodes[(q + 1) % m];
	  HOST_WIDE_INT best = 0;
	  for (unsigned a = 0; a < arcs.length (); a++)
	    if (arcs[a].src == u && arcs[a].dst == v && arcs[a].theta > best)
	      best = arcs[a].theta;
	  delay += best;
	}
      return (int) delay;
    }

  idx.safe_grow_cleared (st->n);
  for (unsigned i = 0; i < st->n; i++)
    idx[i] = -1;
  for (unsigned q = 0; q < m; q++)
    idx[ns->nodes[q]] = (int) q;

  lvx_plc plc;
  for (unsigned q = 0; q <= m; q++)
    plc.add_node ();
  for (unsigned q = 0; q < m; q++)
    plc.add_arc (0, q + 1, 0, 0);
  for (unsigned a = 0; a < arcs.length (); a++)
    if (idx[arcs[a].src] >= 0 && idx[arcs[a].dst] >= 0)
      plc.add_arc (idx[arcs[a].src] + 1, idx[arcs[a].dst] + 1,
		   arcs[a].theta, arcs[a].omega);
  if (plc.solve (1) != LVX_PLC_SOLVED)
    return 0;
  return (int) plc.period ();
}

/* Set membership, kept simple: the sets are few and small.  */

static bool
lvx_sms_in_set (const lvx_sms_set *s, unsigned u)
{
  for (unsigned q = 0; q < s->nodes.length (); q++)
    if (s->nodes[q] == u)
      return true;
  return false;
}

static void
lvx_sms_add (lvx_sms_set *s, unsigned u)
{
  if (!lvx_sms_in_set (s, u))
    s->nodes.safe_push (u);
}

/* removeDuplicateNodes: a node belongs to ONE set, the earliest that claimed
   it, because the sets are taken in priority order and a later set must not
   re-order what an earlier one has already placed in the list.  */

static void
lvx_sms_remove_duplicates (struct lvx_sms_state *st)
{
  for (unsigned i = 0; i < st->sets.length (); i++)
    for (unsigned j = i + 1; j < st->sets.length (); j++)
      {
	lvx_sms_set *s = st->sets[j];
	for (unsigned q = 0; q < s->nodes.length ();)
	  if (lvx_sms_in_set (st->sets[i], s->nodes[q]))
	    s->nodes.ordered_remove (q);
	  else
	    q++;
      }
  for (unsigned i = 0; i < st->sets.length ();)
    if (st->sets[i]->nodes.is_empty ())
      {
	delete st->sets[i];
	st->sets.ordered_remove (i);
      }
    else
      i++;
}

/* computeNodeSetInfo: the three keys the sets are ordered by.  */

static void
lvx_sms_set_info (struct lvx_sms_state *st, lvx_sms_set *s)
{
  s->maxmov = 0;
  s->maxdepth = 0;
  for (unsigned q = 0; q < s->nodes.length (); q++)
    {
      s->maxmov = MAX (s->maxmov, st->mov[s->nodes[q]]);
      s->maxdepth = MAX (s->maxdepth, st->depth[s->nodes[q]]);
    }
}

/* LLVM's NodeSet::operator<, which is a "more constrained first" order:
   larger recurrence bound, then smaller mobility, then greater depth.  */

static int
lvx_sms_set_cmp (const void *a, const void *b)
{
  const lvx_sms_set *x = *(const lvx_sms_set *const *) a;
  const lvx_sms_set *y = *(const lvx_sms_set *const *) b;

  if (x->recmii != y->recmii)
    return y->recmii - x->recmii;
  if (x->maxmov != y->maxmov)
    return x->maxmov - y->maxmov;
  if (x->maxdepth != y->maxdepth)
    return y->maxdepth - x->maxdepth;
  /* Ties broken by the lowest node number, so the order is a function of the
     loop and not of the sort's stability.  */
  return (int) x->nodes[0] - (int) y->nodes[0];
}

/* Pred_L and Suc_L of the paper: the predecessors (successors) of a list that
   are not themselves in it.  LLVM's pred_L/succ_L, and like LLVM's they skip
   the dependences the node functions skipped.  */

static bool
lvx_sms_pred_l (struct lvx_sms_state *st, const auto_vec<unsigned> &o,
		auto_vec<unsigned> &preds, const lvx_sms_set *limit)
{
  const auto_vec<lvx_lms_arc> &arcs = *st->p->arcs;

  preds.truncate (0);
  for (unsigned q = 0; q < o.length (); q++)
    for (unsigned a = 0; a < arcs.length (); a++)
      if (arcs[a].dst == o[q] && !lvx_sms_ignore_for_funcs (arcs[a]))
	{
	  unsigned v = arcs[a].src;
	  bool have = false;

	  for (unsigned k = 0; k < o.length (); k++)
	    if (o[k] == v)
	      have = true;
	  if (have || (limit && !lvx_sms_in_set (limit, v)))
	    continue;
	  for (unsigned k = 0; k < preds.length (); k++)
	    if (preds[k] == v)
	      have = true;
	  if (!have)
	    preds.safe_push (v);
	}
  return !preds.is_empty ();
}

static bool
lvx_sms_succ_l (struct lvx_sms_state *st, const auto_vec<unsigned> &o,
		auto_vec<unsigned> &succs, const lvx_sms_set *limit)
{
  const auto_vec<lvx_lms_arc> &arcs = *st->p->arcs;

  succs.truncate (0);
  for (unsigned q = 0; q < o.length (); q++)
    for (unsigned a = 0; a < arcs.length (); a++)
      if (arcs[a].src == o[q] && !lvx_sms_ignore_for_funcs (arcs[a]))
	{
	  unsigned v = arcs[a].dst;
	  bool have = false;

	  for (unsigned k = 0; k < o.length (); k++)
	    if (o[k] == v)
	      have = true;
	  if (have || (limit && !lvx_sms_in_set (limit, v)))
	    continue;
	  for (unsigned k = 0; k < succs.length (); k++)
	    if (succs[k] == v)
	      have = true;
	  if (!have)
	    succs.safe_push (v);
	}
  return !succs.is_empty ();
}

/* Is every member of SMALL also in BIG?  The paper's Pred_L (O) subset S,
   which GCC's implementation weakens to a non-empty intersection and LLVM
   keeps.  Keeping it is the point: the subset test is what guarantees that a
   sweep starts from nodes whose outside neighbours are already placed.  */

static bool
lvx_sms_subset (const auto_vec<unsigned> &small, const lvx_sms_set *big)
{
  for (unsigned q = 0; q < small.length (); q++)
    if (!lvx_sms_in_set (big, small[q]))
      return false;
  return true;
}

/* addConnectedNodes: the connected component of U within what is left.  */

static void
lvx_sms_add_connected (struct lvx_sms_state *st, unsigned u, lvx_sms_set *ns,
		       auto_vec<bool> &added)
{
  const auto_vec<lvx_lms_arc> &arcs = *st->p->arcs;

  lvx_sms_add (ns, u);
  added[u] = true;
  for (unsigned a = 0; a < arcs.length (); a++)
    {
      if (!lvx_sms_data_arc_p (arcs[a]))
	continue;
      if (arcs[a].src == u && !added[arcs[a].dst])
	lvx_sms_add_connected (st, arcs[a].dst, ns, added);
      if (arcs[a].dst == u && !added[arcs[a].src])
	lvx_sms_add_connected (st, arcs[a].src, ns, added);
    }
}

/* Is there a path from CUR to a node of DEST that avoids EXCLUDE?  LLVM's
   computePath, and the nodes on such a path are what joins one recurrence to
   the sets already taken -- the paper's "nodes located at any path that joins
   the nodes in any previous set and the recurrence of this set".  */

static bool
lvx_sms_path (struct lvx_sms_state *st, unsigned cur, auto_vec<unsigned> &path,
	      const auto_vec<unsigned> &dest, const auto_vec<unsigned> &exclude,
	      auto_vec<bool> &visited)
{
  const auto_vec<lvx_lms_arc> &arcs = *st->p->arcs;
  bool found = false;

  for (unsigned q = 0; q < exclude.length (); q++)
    if (exclude[q] == cur)
      return false;
  for (unsigned q = 0; q < dest.length (); q++)
    if (dest[q] == cur)
      return true;
  if (visited[cur])
    {
      for (unsigned q = 0; q < path.length (); q++)
	if (path[q] == cur)
	  return true;
      return false;
    }
  visited[cur] = true;

  for (unsigned a = 0; a < arcs.length (); a++)
    if (arcs[a].src == cur && !lvx_sms_ignore_for_order (arcs[a]))
      found |= lvx_sms_path (st, arcs[a].dst, path, dest, exclude, visited);
  for (unsigned a = 0; a < arcs.length (); a++)
    if (arcs[a].dst == cur && arcs[a].kind == LVX_ARC_REG_WAR
	&& arcs[a].omega == 0)
      found |= lvx_sms_path (st, arcs[a].src, path, dest, exclude, visited);

  if (found)
    {
      bool have = false;
      for (unsigned q = 0; q < path.length (); q++)
	if (path[q] == cur)
	  have = true;
      if (!have)
	path.safe_push (cur);
    }
  return found;
}

/* groupRemainingNodes: every node must end up in exactly one set.  Nodes on a
   path between two recurrences join the later one, so that the order never
   places a node after both a predecessor and a successor; what is left is
   grouped by connected component.  */

static void
lvx_sms_group_remaining (struct lvx_sms_state *st)
{
  unsigned n = st->n;
  auto_vec<unsigned> added, n1, path;
  auto_vec<bool> visited, inadded;

  visited.safe_grow_cleared (n);
  inadded.safe_grow_cleared (n);

  for (unsigned i = 0; i < st->sets.length (); i++)
    {
      lvx_sms_set *s = st->sets[i];

      if (lvx_sms_succ_l (st, s->nodes, n1, NULL))
	{
	  path.truncate (0);
	  for (unsigned q = 0; q < n1.length (); q++)
	    {
	      for (unsigned k = 0; k < n; k++)
		visited[k] = false;
	      lvx_sms_path (st, n1[q], path, added, s->nodes, visited);
	    }
	  for (unsigned q = 0; q < path.length (); q++)
	    lvx_sms_add (s, path[q]);
	}

      if (lvx_sms_succ_l (st, added, n1, NULL))
	{
	  path.truncate (0);
	  for (unsigned q = 0; q < n1.length (); q++)
	    {
	      for (unsigned k = 0; k < n; k++)
		visited[k] = false;
	      lvx_sms_path (st, n1[q], path, s->nodes, added, visited);
	    }
	  for (unsigned q = 0; q < path.length (); q++)
	    lvx_sms_add (s, path[q]);
	}

      for (unsigned q = 0; q < s->nodes.length (); q++)
	if (!inadded[s->nodes[q]])
	  {
	    inadded[s->nodes[q]] = true;
	    added.safe_push (s->nodes[q]);
	  }
    }

  /* Then the connected components of whatever has no set yet.  */
  for (unsigned i = 0; i < n; i++)
    if (!inadded[i])
      {
	lvx_sms_set *ns = new lvx_sms_set;

	ns->rec = false;
	ns->recmii = 0;
	ns->maxmov = 0;
	ns->maxdepth = 0;
	lvx_sms_add_connected (st, i, ns, inadded);
	st->sets.safe_push (ns);
      }
}

/* THE ORDER, section 4.2 and Figure 6 of the paper, LLVM's computeNodeOrder.

   For each set in priority order, seed a working set and a direction -- the
   predecessors of what is already ordered if they all lie in this set, else
   the successors, else the node with the greatest ASAP -- then sweep.  Going
   up, take the greatest DEPTH; going down, the greatest HEIGHT; ties to the
   greater zero-latency depth or height, then to the LEAST MOBILITY, which is
   the paper's "nodes in a more critical path first".  When the working set
   empties the direction flips and is reseeded from the other side.  */

static void
lvx_sms_compute_order (struct lvx_sms_state *st)
{
  const auto_vec<lvx_lms_arc> &arcs = *st->p->arcs;
  auto_vec<unsigned> r, nn;

  for (unsigned i = 0; i < st->sets.length (); i++)
    {
      lvx_sms_set *s = st->sets[i];
      bool topdown;

      r.truncate (0);
      if (lvx_sms_pred_l (st, st->order, nn, NULL) && lvx_sms_subset (nn, s))
	{
	  for (unsigned q = 0; q < nn.length (); q++)
	    r.safe_push (nn[q]);
	  topdown = false;
	}
      else if (lvx_sms_succ_l (st, st->order, nn, NULL)
	       && lvx_sms_subset (nn, s))
	{
	  for (unsigned q = 0; q < nn.length (); q++)
	    r.safe_push (nn[q]);
	  topdown = true;
	}
      else
	{
	  /* The node of this set with the greatest ASAP, which is the bottom
	     of its most critical path.  */
	  unsigned best = s->nodes[0];

	  for (unsigned q = 1; q < s->nodes.length (); q++)
	    if (st->asap[s->nodes[q]] > st->asap[best]
		|| (st->asap[s->nodes[q]] == st->asap[best]
		    && s->nodes[q] > best))
	      best = s->nodes[q];
	  r.safe_push (best);
	  topdown = false;
	}

      while (!r.is_empty ())
	{
	  while (!r.is_empty ())
	    {
	      unsigned best = r[0];

	      for (unsigned q = 1; q < r.length (); q++)
		{
		  unsigned c = r[q];
		  int bk = topdown ? st->height[best] : st->depth[best];
		  int ck = topdown ? st->height[c] : st->depth[c];
		  int bz = topdown ? st->zlheight[best] : st->zldepth[best];
		  int cz = topdown ? st->zlheight[c] : st->zldepth[c];

		  if (ck > bk
		      || (ck == bk && cz > bz)
		      || (ck == bk && cz == bz && st->mov[c] < st->mov[best]))
		    best = c;
		}

	      st->order.safe_push (best);
	      for (unsigned q = 0; q < r.length (); q++)
		if (r[q] == best)
		  {
		    r.ordered_remove (q);
		    break;
		  }

	      /* The neighbours on the side the sweep is moving towards, plus
		 -- as LLVM does -- the anti dependences on the other side,
		 which would otherwise never enter the working set.  */
	      for (unsigned a = 0; a < arcs.length (); a++)
		{
		  unsigned v;

		  if (topdown
		      ? (arcs[a].src == best && !lvx_sms_ignore_for_order (arcs[a]))
		      : (arcs[a].dst == best && !lvx_sms_ignore_for_order (arcs[a])))
		    v = topdown ? arcs[a].dst : arcs[a].src;
		  else if (arcs[a].kind == LVX_ARC_REG_WAR
			   && (topdown ? arcs[a].dst == best
				       : arcs[a].src == best))
		    v = topdown ? arcs[a].src : arcs[a].dst;
		  else
		    continue;

		  if (!lvx_sms_in_set (s, v))
		    continue;
		  bool have = false;
		  for (unsigned k = 0; k < st->order.length (); k++)
		    if (st->order[k] == v)
		      have = true;
		  for (unsigned k = 0; k < r.length (); k++)
		    if (r[k] == v)
		      have = true;
		  if (!have)
		    r.safe_push (v);
		}
	    }

	  /* The sweep emptied: turn round and reseed from the other side of
	     what is now ordered, within this set.  */
	  topdown = !topdown;
	  if (topdown)
	    {
	      if (lvx_sms_succ_l (st, st->order, nn, s))
		for (unsigned q = 0; q < nn.length (); q++)
		  r.safe_push (nn[q]);
	    }
	  else if (lvx_sms_pred_l (st, st->order, nn, s))
	    for (unsigned q = 0; q < nn.length (); q++)
	      r.safe_push (nn[q]);
	}
    }
}

/* checkValidNodeOrder, LLVM's verifier and the cheapest guard there is
   against a mis-transcribed sweep: no node may be ordered after BOTH a
   predecessor and a successor unless it is in a circuit, because that is the
   case the two-level ordering exists to avoid and the one where placement has
   to compromise between its two sides.  Returns the number of violations.  */

static int
lvx_sms_check_order (struct lvx_sms_state *st)
{
  const auto_vec<lvx_lms_arc> &arcs = *st->p->arcs;
  unsigned n = st->n;
  auto_vec<int> pos;
  int bad = 0;

  pos.safe_grow_cleared (n);
  for (unsigned i = 0; i < n; i++)
    pos[i] = -1;
  for (unsigned k = 0; k < st->order.length (); k++)
    pos[st->order[k]] = (int) k;

  for (unsigned k = 0; k < st->order.length (); k++)
    {
      unsigned u = st->order[k];
      bool predbefore = false, succbefore = false;

      for (unsigned a = 0; a < arcs.length (); a++)
	{
	  if (!lvx_sms_data_arc_p (arcs[a]))
	    continue;
	  if (arcs[a].dst == u && pos[arcs[a].src] >= 0
	      && pos[arcs[a].src] < (int) k)
	    predbefore = true;
	  if (arcs[a].src == u && pos[arcs[a].dst] >= 0
	      && pos[arcs[a].dst] < (int) k)
	    succbefore = true;
	}
      if (!predbefore || !succbefore)
	continue;

      bool in_circuit = false;
      for (unsigned i = 0; i < st->sets.length (); i++)
	if (st->sets[i]->rec && lvx_sms_in_set (st->sets[i], u))
	  in_circuit = true;
      if (!in_circuit)
	bad++;
    }
  return bad;
}

/* THE PLACEMENT, section 4.3 of the paper and LLVM's schedulePipeline.

   Each node in turn, into a window of at most II cycles decided by which of
   its neighbours are already placed:

     predecessors only   as soon as possible, scanning forward from Early
     successors only     as late as possible, scanning backward from Late
     both                forward from Early to min (Late, Early + II - 1)
     neither             forward from ASAP

   Scanning more than II cycles is pointless under the modulo constraint, and
   the "both" case is the one the ordering exists to make rare -- it happens
   for one node per recurrence and for none at all on an acyclic body.

   The reservation is the LVX modulo table: one lvx_sched_resources per
   residue, charged through lvx_lms_resources_add, which carries the GUARD and
   MASK prefixes and the doloop_end's decrement and branch.  LLVM's
   ResourceManager plays this part there.  */

struct lvx_sms_place
{
  auto_vec<int> date;		/* absolute cycle, or INT_MIN */
  auto_vec<bool> done;
  auto_vec<struct lvx_sched_resources> mrt;
  int ii;
};

static bool
lvx_sms_insert (struct lvx_sms_place *pl, const auto_vec<rtx_insn *> &seq,
		unsigned u, int start, int end)
{
  int step = start <= end ? 1 : -1;
  int term = end + step;

  for (int c = start; c != term; c += step)
    {
      int r = ((c % pl->ii) + pl->ii) % pl->ii;

      if (!lvx_lms_slot_fits_p (&pl->mrt[r], seq[u]))
	continue;
      lvx_lms_resources_add (&pl->mrt[r], seq[u]);
      pl->date[u] = c;
      pl->done[u] = true;
      return true;
    }
  return false;
}

/* computeStart: the window the already-placed neighbours leave.  */

static void
lvx_sms_compute_start (struct lvx_sms_state *st, struct lvx_sms_place *pl,
		       unsigned u, int *early, int *late,
		       bool *only_carried_preds)
{
  const auto_vec<lvx_lms_arc> &arcs = *st->p->arcs;
  bool any_pred = false, carried_only = true;

  *early = INT_MIN;
  *late = INT_MAX;
  for (unsigned a = 0; a < arcs.length (); a++)
    {
      int theta = (int) arcs[a].theta, omega = (int) arcs[a].omega;

      if (arcs[a].dst == u && pl->done[arcs[a].src])
	{
	  int e = pl->date[arcs[a].src] + theta - omega * pl->ii;

	  *early = MAX (*early, e);
	  any_pred = true;
	  if (omega == 0
	      || !(arcs[a].kind == LVX_ARC_REG_WAW
		   || arcs[a].kind == LVX_ARC_MEM_CROSS
		   || arcs[a].kind == LVX_ARC_CTRL_ORDER
		   || arcs[a].kind == LVX_ARC_CTRL_STORE
		   || arcs[a].kind == LVX_ARC_CTRL_LIVEOUT
		   || arcs[a].kind == LVX_ARC_CTRL_BACKEDGE))
	    carried_only = false;
	}
      if (arcs[a].src == u && pl->done[arcs[a].dst])
	{
	  int l = pl->date[arcs[a].dst] - theta + omega * pl->ii;

	  *late = MIN (*late, l);
	}
    }
  *only_carried_preds = any_pred && carried_only;
}

/* One construction at one initiation interval.  */

static bool
lvx_sms_place_at_ii (struct lvx_sms_state *st, int ii, auto_vec<int> &sigma,
		     const char **why)
{
  const auto_vec<rtx_insn *> &seq = *st->p->seq;
  unsigned n = st->n;
  struct lvx_sms_place pl;
  struct lvx_sched_resources empty;

  memset (&empty, 0, sizeof (empty));
  pl.ii = ii;
  pl.date.safe_grow_cleared (n);
  pl.done.safe_grow_cleared (n);
  for (unsigned i = 0; i < n; i++)
    pl.date[i] = INT_MIN, pl.done[i] = false;
  for (int r = 0; r < ii; r++)
    pl.mrt.safe_push (empty);

  for (unsigned k = 0; k < st->order.length (); k++)
    {
      unsigned u = st->order[k];
      int early, late;
      bool carried;
      bool ok;

      lvx_sms_compute_start (st, &pl, u, &early, &late, &carried);

      if (early != INT_MIN && late != INT_MAX && early > late)
	{
	  if (dump_file && lvx_lms_probe && lvx_sms_trace)
	    lvx_lms_dump (";; lvx sms:     ii=%d node %u (insn %d) empty:"
			  " early=%d late=%d\n", ii, u, INSN_UID (seq[u]),
			  early, late);
	  *why = "sms window is empty";
	  return false;
	}

      if (early != INT_MIN && late == INT_MAX)
	ok = lvx_sms_insert (&pl, seq, u, early, early + ii - 1);
      else if (early == INT_MIN && late != INT_MAX)
	ok = lvx_sms_insert (&pl, seq, u, late, late - ii + 1);
      else if (early != INT_MIN && late != INT_MAX)
	{
	  late = MIN (late, early + ii - 1);
	  /* LLVM searches BACKWARD when every placed predecessor is a
	     loop-carried output or order dependence: those say almost
	     nothing about where the node wants to go, and empirically the
	     late end is where it fits.  Its other reason, a PHI, has no
	     counterpart after register allocation.  */
	  ok = carried ? lvx_sms_insert (&pl, seq, u, late, early)
		       : lvx_sms_insert (&pl, seq, u, early, late);
	}
      else
	ok = lvx_sms_insert (&pl, seq, u, st->asap[u], st->asap[u] + ii - 1);

      if (!ok)
	{
	  *why = "sms found no free slot in the window";
	  return false;
	}
    }

  /* SIGMA, normalised so its minimum is zero.  A uniform shift permutes the
     residues uniformly, so it cannot invalidate a table that just fitted.  */
  int lo = pl.date[0];
  for (unsigned i = 1; i < n; i++)
    lo = MIN (lo, pl.date[i]);
  sigma.truncate (0);
  for (unsigned i = 0; i < n; i++)
    sigma.safe_push (pl.date[i] - lo);
  return true;
}

/* ---------------------------------------------------------------------- */
/* The engine.  */

static void *
lvx_sms_prepare (const struct lvx_lms_problem *p)
{
  struct lvx_sms_state *st = new lvx_sms_state;

  st->p = p;
  st->n = p->seq->length ();
  st->mii = MAX (1, p->lower);
  st->ok = false;

  if (st->n == 0 || !lvx_sms_topo_order (st))
    return st;

  lvx_sms_node_functions (st);
  lvx_sms_find_circuits (st);
  lvx_sms_remove_duplicates (st);
  for (unsigned i = 0; i < st->sets.length (); i++)
    st->sets[i]->recmii = lvx_sms_set_recmii (st, st->sets[i]);
  for (unsigned i = 0; i < st->sets.length (); i++)
    lvx_sms_set_info (st, st->sets[i]);
  st->sets.qsort (lvx_sms_set_cmp);
  lvx_sms_group_remaining (st);
  for (unsigned i = 0; i < st->sets.length (); i++)
    lvx_sms_set_info (st, st->sets[i]);

  lvx_sms_compute_order (st);
  st->ok = st->order.length () == st->n;

  if (dump_file && lvx_lms_probe)
    {
      int bad = st->ok ? lvx_sms_check_order (st) : -1;

      lvx_lms_dump (";; lvx sms: fn=%s order n=%u sets=%u mii=%d"
		    " ordered=%u badorder=%d\n", current_function_name (),
		    st->n, st->sets.length (), st->mii, st->order.length (),
		    bad);
      if (lvx_sms_trace)
	{
	  for (unsigned i = 0; i < st->sets.length (); i++)
	    {
	      lvx_lms_dump (";; lvx sms:   set %u rec=%d recmii=%d maxmov=%d"
			    " maxdepth=%d:", i, (int) st->sets[i]->rec,
			    st->sets[i]->recmii, st->sets[i]->maxmov,
			    st->sets[i]->maxdepth);
	      for (unsigned q = 0; q < st->sets[i]->nodes.length (); q++)
		lvx_lms_dump (" %u", st->sets[i]->nodes[q]);
	      lvx_lms_dump ("\n");
	    }
	  lvx_lms_dump (";; lvx sms:   order:");
	  for (unsigned q = 0; q < st->order.length (); q++)
	    lvx_lms_dump (" %u", st->order[q]);
	  lvx_lms_dump ("\n");
	}
    }
  return st;
}

static void
lvx_sms_finish (void *state)
{
  struct lvx_sms_state *st = (struct lvx_sms_state *) state;

  for (unsigned i = 0; i < st->sets.length (); i++)
    delete st->sets[i];
  delete st;
}

static bool
lvx_sms_schedule_at_ii (void *state, int ii, auto_vec<int> &sigma,
			const char **why, int *passes)
{
  struct lvx_sms_state *st = (struct lvx_sms_state *) state;
  const struct lvx_lms_problem *p = st->p;
  auto_vec<int> blame, culprit;

  if (!st->ok)
    {
      *why = "sms could not order the nodes";
      return false;
    }

  (*passes)++;
  if (!lvx_sms_place_at_ii (st, ii, sigma, why))
    {
      if (dump_file && lvx_lms_probe && lvx_sms_trace)
	lvx_lms_dump (";; lvx sms:   ii=%d refused: %s\n", ii, *why);
      return false;
    }

  /* Acceptance is LMS's and not the engine's: the same verification and the
     same colouring every other engine is judged by.  SMS has no search of its
     own to feed a refusal back into -- the order is computed once -- so a
     refusal simply means this interval is not the one.  */
  blame.safe_grow_cleared (st->n);
  culprit.safe_grow_cleared (st->n);
  *why = lvx_lms_accept_p (*p->seq, *p->arcs, ii, sigma, p->livein,
			   &blame, &culprit);
  if (dump_file && lvx_lms_probe && lvx_sms_trace)
    lvx_lms_dump (";; lvx sms:   ii=%d placed, acceptance: %s\n", ii,
		  *why ? *why : "taken");
  return *why == NULL;
}

const struct lvx_lms_engine lvx_sms_engine = {
  "sms", lvx_sms_prepare, lvx_sms_schedule_at_ii, lvx_sms_finish
};
