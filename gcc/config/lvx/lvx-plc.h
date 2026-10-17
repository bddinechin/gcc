/* Parametric label correcting over a bi-valued dependence graph.
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

/* After Dupont de Dinechin, "Parametric Computation of Margins and of
   Minimum Cumulative Register Lifetime Dates", LCPC 1996, whose PLC is a
   parametric form of the Simplex variant of Goldfarb, Hao and Kai's dynamic
   breadth-first search.  Ported from the lvx-mlir implementation
   (mlir/lib/Dialect/LVX/Analysis/ParametricLabelCorrecting.cpp).

   The graph carries one arc (i, j) per dependence, with a latency THETA and
   an iteration distance OMEGA, constraining a 1-periodic schedule of period
   LAMBDA as

       date (i) + theta - lambda * omega <= date (j).

   Solving it yields the smallest period at which no circuit is positive --
   RecMII, and the circuit that bounds it -- together with the earliest
   potentials at that period.  Seeding the period with ResMII makes the
   answer max (RecMII, ResMII), which is what a decision between unfolding a
   loop and pipelining it needs.

   "Parametric" means LAMBDA is carried symbolically: an arc's length is
   `theta - lambda * omega' and a potential is `mu + lambda * nu', so raising
   the period leaves every potential valid as a function of it and costs no
   recomputation of the spanning tree.  */

#ifndef GCC_LVX_PLC_H
#define GCC_LVX_PLC_H

/* An arc length or node potential as an affine function of the period,
   MU + LAMBDA * NU.  An arc's NU is -OMEGA; a potential's is the negated sum
   of the distances along its spanning-tree path.  */

struct lvx_affine_period
{
  HOST_WIDE_INT mu;
  HOST_WIDE_INT nu;

  HOST_WIDE_INT at (HOST_WIDE_INT lambda) const { return mu + lambda * nu; }
};

inline lvx_affine_period
lvx_affine_add (const lvx_affine_period &a, const lvx_affine_period &b)
{
  lvx_affine_period r = { a.mu + b.mu, a.nu + b.nu };
  return r;
}

inline lvx_affine_period
lvx_affine_sub (const lvx_affine_period &a, const lvx_affine_period &b)
{
  lvx_affine_period r = { a.mu - b.mu, a.nu - b.nu };
  return r;
}

/* What a solve concluded.  */

enum lvx_plc_result
{
  /* A period and potentials were found.  */
  LVX_PLC_SOLVED,
  /* A positive circuit carrying no distance: no period makes it feasible,
     which means a dependence cycle inside one iteration.  */
  LVX_PLC_INFEASIBLE_CIRCUIT,
  /* Some node is not reachable from the root along zero-distance arcs.  */
  LVX_PLC_UNREACHABLE,
  /* The iteration bound was hit; report rather than loop.  */
  LVX_PLC_ITERATION_LIMIT
};

class lvx_plc
{
public:
  lvx_plc () : m_lambda (1) {}

  /* Add a node and return its index.  The first added is the root, whose
     potential is pinned to zero and from which every other node must be
     reachable along zero-distance arcs.  */
  unsigned add_node (void);

  /* Add an arc and return its index.  */
  unsigned add_arc (unsigned tail, unsigned head, HOST_WIDE_INT theta,
		    HOST_WIDE_INT omega);

  /* Raise the period until no circuit is positive, maintaining the earliest
     potentials throughout.  LAMBDA_LOWER_BOUND seeds it, and should be
     ResMII when one is known.  */
  enum lvx_plc_result solve (HOST_WIDE_INT lambda_lower_bound = 1);

  HOST_WIDE_INT period (void) const { return m_lambda; }

  lvx_affine_period potential (unsigned node) const
  {
    return m_nodes[node].pi;
  }

  HOST_WIDE_INT date (unsigned node) const
  {
    return m_nodes[node].pi.at (m_lambda);
  }

  /* The arcs of the circuit that last forced the period up, the entering arc
     first and then the tree path from its tail.  Empty when the period was
     never raised.  */
  const vec<unsigned> &critical_circuit (void) const { return m_circuit; }

  unsigned num_nodes (void) const { return m_nodes.length (); }
  unsigned num_arcs (void) const { return m_arcs.length (); }

private:
  struct arc
  {
    unsigned tail, head;
    HOST_WIDE_INT theta, omega;
  };

  /* A node of the spanning tree.  EDGE is the arc it hangs from, oriented
     away from the root; NEXT and PREV thread the tree in preorder and are
     circular, which is what terminates the subtree walks.  */
  struct node
  {
    int edge;
    lvx_affine_period pi;
    unsigned next, prev;
    int depth;
  };

  /* What the upward walk from an entering arc's tail found.  */
  enum circuit_kind { CIRCUIT_NONE, CIRCUIT_PERIOD_RAISED, CIRCUIT_NO_DISTANCE };

  lvx_affine_period arc_length (const arc &a) const
  {
    lvx_affine_period r = { a.theta, -a.omega };
    return r;
  }

  /* The reduced length of A: positive means the constraint is violated.  */
  lvx_affine_period reduced (const arc &a) const
  {
    return lvx_affine_sub (lvx_affine_add (arc_length (a), m_nodes[a.tail].pi),
			   m_nodes[a.head].pi);
  }

  /* Whether R must be corrected: positive at the period, or zero there and
     rising, which keeps the potentials lexicographically maximal and so
     valid just above the period.  A circuit can never satisfy the second
     disjunct, its NU being the negated sum of non-negative distances, so
     this cannot cycle.  */
  bool violated (const lvx_affine_period &r) const
  {
    HOST_WIDE_INT v = r.at (m_lambda);
    return v > 0 || (v == 0 && r.nu > 0);
  }

  bool build_initial_tree (void);
  enum circuit_kind check_circuit (unsigned arc_id);
  void reparent (unsigned arc_id);
  void enqueue_out_arcs (unsigned n);

  auto_vec<node> m_nodes;
  auto_vec<arc> m_arcs;
  /* Out-arcs as a linked list per node: M_OUT_HEAD[n] is the first arc with
     tail n, or -1, and M_OUT_NEXT[a] the next arc with the same tail.  */
  auto_vec<int> m_out_head;
  auto_vec<int> m_out_next;

  auto_vec<unsigned> m_worklist;
  auto_vec<bool> m_queued;
  auto_vec<unsigned> m_circuit;

  HOST_WIDE_INT m_lambda;
};

#endif /* GCC_LVX_PLC_H */
