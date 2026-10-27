/* Minimum-cost network flow, for Minimum Cumulative Register Lifetime dates.
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

/* The other half of Dupont de Dinechin, "Parametric Computation of Margins
   and of Minimum Cumulative Register Lifetime Dates", LCPC 1996.  lvx-plc.cc
   implements the margins half -- parametric label correcting over the
   bi-valued graph, giving RecMII and the earliest potentials.  This file
   implements the MCRL half.

   THE PROBLEM.  A register lifetime is the time between the issue of the
   instruction that writes a register and the last read of that value.  With
   DATE the schedule dates and LIFE the lifetimes, for a period LAMBDA:

       date (j) - date (i)  >=  theta - lambda * omega     (dependence)
       life (i)             >=  date (j) - date (i)        (per consumer J of I)

   and MCRL minimises the cumulative lifetime.  The paper notes that this and
   Ning & Gao's minimum-buffers problem -- whose inequalities are
   `lambda * buffer (i) >= date (j) - date (i)', counting registers where this
   counts cycles -- are both maximum-cost network flow problems, which is why
   the margins and the lifetimes share one machine: the single-source
   longest-path problem is itself solved by maximising the cost of a flow.

   WHAT MCRL IS FOR.  It yields, for each not-yet-scheduled instruction, a
   tentative date minimising the cumulative lifetime -- a lifetime-sensitive
   priority computed BEFORE placement.  That matters because
   docs/cms-foundations.md §7vicies through §7quatervicies measured four
   mechanisms that repair a finished schedule (stage scheduling, range
   splitting, spill/restore, Huff's stretchable-side decision) and none of
   them repaired a single loop.

   REGISTER TUPLES.  LVX's singles, pairs and quads are three overlapping
   granularities of one 64-register file rather than disjoint files, so a
   pair's lifetime occupies two registers and a quad's four.  The adjustment
   (author, 2026-10-27) is that THE FLOW LEAVING A PRODUCER NODE IS THE NUMBER
   OF REGISTERS IN THE PRODUCED TUPLE: the objective

       min  sum over i of  nregs (i) * life (i)

   then falls out of flow conservation instead of being imposed on it, so no
   arc cost is scaled and no objective rewritten.  See §7quinvicies.

   WHAT THIS IS NOT.  LAO's CDT/CAL/Network.c solves these with a primal-dual
   network SIMPLEX, 1683 lines.  This is successive shortest paths with
   potentials -- exact, far smaller, and verifiable by selftest; its cost is
   asymptotic, not accuracy.  A simplex is the upgrade if the flow ever sits on
   a compile-time path, which measurement should decide rather than assumption.

   AND THE OPEN QUESTION, recorded so it is not mistaken for solved: a
   cumulative lifetime is an L1 objective, while what actually binds a modulo
   kernel is MaxLive, the MAXIMUM over cycles of concurrently live registers
   (§7octodecies.5), and colourability can fail even above MaxLive through
   tuple-alignment fragmentation (§7ter.10).  Weighting the sum by nregs makes
   it count registers rather than values, which is right, but a weighted sum is
   still not a maximum.  */

#ifndef GCC_LVX_NETWORK_H
#define GCC_LVX_NETWORK_H

/* What a solve concluded.  */
enum lvx_network_result
{
  /* An optimal flow was found.  */
  LVX_NETWORK_SOLVED,
  /* The requirements cannot be met by the arc capacities.  */
  LVX_NETWORK_INFEASIBLE,
  /* A negative-cost circuit of infinite capacity: the optimum is unbounded.  */
  LVX_NETWORK_UNBOUNDED,
  /* The iteration bound was hit; report rather than loop.  */
  LVX_NETWORK_ITERATION_LIMIT
};

/* A minimum-cost flow instance.  Nodes carry a REQUIREMENT, positive where
   flow is supplied and negative where it is consumed, which is the field the
   tuple adjustment above writes: a producer of an N-register tuple supplies N.
   Arcs carry a capacity and a cost per unit.  */

class lvx_network
{
public:
  lvx_network () : m_total_supply (0) {}

  /* Add a node with the given flow requirement and return its index.
     Positive supplies, negative consumes.  */
  unsigned add_node (HOST_WIDE_INT requirement = 0);

  /* Add an arc and return its index.  A negative COST is allowed; an
     unbounded capacity is spelled -1.  */
  unsigned add_arc (unsigned tail, unsigned head, HOST_WIDE_INT capacity,
		    HOST_WIDE_INT cost);

  /* Set a node's requirement after the fact, which is how a graph builder
     fills in tuple widths once it knows them.  */
  void set_requirement (unsigned node, HOST_WIDE_INT requirement);

  unsigned node_count (void) const { return m_nodes.length (); }
  unsigned arc_count (void) const { return m_arcs.length (); }

  /* Solve.  On LVX_NETWORK_SOLVED, flow () and potential () are meaningful
     and cost () is the optimum.  */
  enum lvx_network_result solve (void);

  HOST_WIDE_INT flow (unsigned arc) const { return m_arcs[arc].flow; }
  HOST_WIDE_INT cost (void) const { return m_cost; }

  /* The node potential, which is what MCRL reads back as a date: the dual
     variable of flow conservation at that node.  */
  HOST_WIDE_INT potential (unsigned node) const { return m_nodes[node].pi; }

private:
  struct node
  {
    HOST_WIDE_INT requirement;
    HOST_WIDE_INT pi;			/* dual potential */
  };

  struct arc
  {
    unsigned tail, head;
    HOST_WIDE_INT capacity;		/* -1 for unbounded */
    HOST_WIDE_INT cost;
    HOST_WIDE_INT flow;
  };

  auto_vec<node> m_nodes;
  auto_vec<arc> m_arcs;
  HOST_WIDE_INT m_total_supply;
  HOST_WIDE_INT m_cost;

  bool initial_potentials (void);
  bool augment (unsigned src, unsigned dst, HOST_WIDE_INT amount);
};

#if CHECKING_P
namespace selftest {
extern void lvx_network_cc_tests (void);
}
#endif

#endif /* GCC_LVX_NETWORK_H */
