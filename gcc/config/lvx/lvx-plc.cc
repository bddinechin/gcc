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

#define IN_TARGET_CODE 1

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "vec.h"
#include "selftest.h"
#include "config/lvx/lvx-plc.h"

/* Smallest integer not below A / B, for B > 0.  */

static HOST_WIDE_INT
lvx_plc_ceil_div (HOST_WIDE_INT a, HOST_WIDE_INT b)
{
  gcc_checking_assert (b > 0);
  return a >= 0 ? (a + b - 1) / b : -((-a) / b);
}

unsigned
lvx_plc::add_node (void)
{
  node nd;
  nd.edge = -1;
  nd.pi.mu = 0;
  nd.pi.nu = 0;
  nd.next = 0;
  nd.prev = 0;
  nd.depth = -1;
  m_nodes.safe_push (nd);
  m_out_head.safe_push (-1);
  return m_nodes.length () - 1;
}

unsigned
lvx_plc::add_arc (unsigned tail, unsigned head, HOST_WIDE_INT theta,
		  HOST_WIDE_INT omega)
{
  gcc_checking_assert (tail < m_nodes.length () && head < m_nodes.length ());
  /* A negative iteration distance is not a dependence.  */
  gcc_checking_assert (omega >= 0);

  arc a;
  a.tail = tail;
  a.head = head;
  a.theta = theta;
  a.omega = omega;
  m_arcs.safe_push (a);

  unsigned id = m_arcs.length () - 1;
  m_out_next.safe_push (m_out_head[tail]);
  m_out_head[tail] = (int) id;
  return id;
}

void
lvx_plc::enqueue_out_arcs (unsigned n)
{
  for (int a = m_out_head[n]; a >= 0; a = m_out_next[a])
    if (!m_queued[a])
      {
	m_queued[a] = true;
	m_worklist.safe_push ((unsigned) a);
      }
}

/* Grow the initial spanning tree over the zero-distance arcs, breadth first
   from the root, taking each node's potential from its parent.

   It would be tempting to relax in topological order instead, so that label
   correcting starts from the longest path.  That is wrong here: when the
   zero-distance subgraph has a circuit there is no topological order,
   relaxation can pick tree edges that form a cycle, and the result is not a
   tree at all.  Growing can only ever produce a tree, and the circuit is then
   found where it belongs, by check_circuit, with BETA == 0, so it is reported
   as feasible at no period rather than silently mis-solved.

   Return false when some node has no tree edge, which means the root does not
   reach it along zero-distance arcs.  */

bool
lvx_plc::build_initial_tree (void)
{
  unsigned n = m_nodes.length ();
  unsigned i;

  for (i = 0; i < n; i++)
    {
      m_nodes[i].edge = -1;
      m_nodes[i].depth = -1;
      m_nodes[i].pi.mu = 0;
      m_nodes[i].pi.nu = 0;
    }
  m_nodes[0].depth = 0;

  auto_vec<unsigned> order;
  auto_vec<unsigned> child_head;
  auto_vec<int> child_next;
  order.safe_push (0);
  child_head.safe_grow_cleared (n);
  for (i = 0; i < n; i++)
    child_head[i] = (unsigned) -1;
  child_next.safe_grow_cleared (m_arcs.length () + 1);

  for (i = 0; i < order.length (); i++)
    {
      unsigned v = order[i];
      for (int ai = m_out_head[v]; ai >= 0; ai = m_out_next[ai])
	{
	  const arc &a = m_arcs[ai];
	  if (a.omega != 0 || a.head == 0 || m_nodes[a.head].depth >= 0)
	    continue;
	  m_nodes[a.head].edge = ai;
	  m_nodes[a.head].pi = lvx_affine_add (m_nodes[v].pi, arc_length (a));
	  m_nodes[a.head].depth = m_nodes[v].depth + 1;
	  order.safe_push (a.head);
	}
    }
  if (order.length () != n)
    return false;

  /* Thread the tree in preorder; ORDER is breadth first, which is not one.
     Children are recovered from the tree edges rather than recorded, so that
     the walk needs no nested vectors.  */
  auto_vec<unsigned> preorder;
  auto_vec<unsigned> stack;
  stack.safe_push (0);
  while (!stack.is_empty ())
    {
      unsigned v = stack.pop ();
      preorder.safe_push (v);
      /* Push children in reverse index order, so they are visited in
	 increasing order.  */
      for (int c = (int) n - 1; c >= 0; c--)
	if (m_nodes[c].edge >= 0 && m_arcs[m_nodes[c].edge].tail == v)
	  stack.safe_push ((unsigned) c);
    }
  gcc_checking_assert (preorder.length () == n);

  for (i = 0; i < n; i++)
    {
      unsigned v = preorder[i], w = preorder[(i + 1) % n];
      m_nodes[v].next = w;
      m_nodes[w].prev = v;
    }
  return true;
}

/* Walk up the tree from the entering arc's tail.  If it reaches the head, the
   arc closes a circuit whose length is positive by construction -- the walk
   accumulates exactly the arc's reduced length -- so the period is raised to
   make it non-positive.

   The caller must not re-parent when a circuit was found: the head is an
   ancestor of the tail, and moving its subtree under the tail would thread the
   tree into itself.  */

enum lvx_plc::circuit_kind
lvx_plc::check_circuit (unsigned arc_id)
{
  const arc &q = m_arcs[arc_id];
  HOST_WIDE_INT alpha = q.theta, beta = q.omega;
  unsigned a = q.tail;

  while (m_nodes[a].depth > m_nodes[q.head].depth)
    {
      const arc &e = m_arcs[m_nodes[a].edge];
      alpha += e.theta;
      beta += e.omega;
      a = e.tail;
    }
  if (a != q.head)
    return CIRCUIT_NONE;

  m_circuit.truncate (0);
  m_circuit.safe_push (arc_id);
  for (unsigned b = q.tail; m_nodes[b].depth > m_nodes[q.head].depth;)
    {
      m_circuit.safe_push ((unsigned) m_nodes[b].edge);
      b = m_arcs[m_nodes[b].edge].tail;
    }

  /* ALPHA - LAMBDA * BETA > 0 at the current period, so ALPHA > 0.  With no
     distance round the circuit no period can help.  */
  if (beta <= 0)
    return CIRCUIT_NO_DISTANCE;

  HOST_WIDE_INT raised = lvx_plc_ceil_div (alpha, beta);
  if (raised > m_lambda)
    m_lambda = raised;
  return CIRCUIT_PERIOD_RAISED;
}

/* Re-parent the entering arc's head under its tail, shifting the head's
   subtree.  The shift is uniform and equal to the arc's reduced length,
   because re-rooting changes no arc's length; the depth delta is likewise
   uniform because the path is not reversed.  */

void
lvx_plc::reparent (unsigned arc_id)
{
  const arc &q = m_arcs[arc_id];
  unsigned k = q.tail, l = q.head;
  lvx_affine_period shift = reduced (q);
  int depth_delta = m_nodes[k].depth - m_nodes[l].depth + 1;

  unsigned x = m_nodes[l].next, r = m_nodes[l].prev, z = l;
  while (m_nodes[x].depth > m_nodes[l].depth)
    {
      m_nodes[x].depth += depth_delta;
      m_nodes[x].pi = lvx_affine_add (m_nodes[x].pi, shift);
      enqueue_out_arcs (x);
      z = x;
      x = m_nodes[x].next;
    }
  m_nodes[l].depth += depth_delta;
  m_nodes[l].pi = lvx_affine_add (m_nodes[l].pi, shift);
  enqueue_out_arcs (l);

  /* At this point next (z) == x and prev (x) == z already hold: the walk above
     reads links and never writes them.  That is why skipping the splice when
     the subtree is already in place is correct rather than merely cheap --
     what it would assign is what is already there, and next (z) = next (k)
     would point the tail of the subtree back inside it.  */
  if (m_nodes[k].next != l)
    {
      unsigned nx = m_nodes[k].next;
      m_nodes[r].next = x;
      m_nodes[x].prev = r;
      m_nodes[z].next = nx;
      m_nodes[nx].prev = z;
    }
  m_nodes[k].next = l;
  m_nodes[l].prev = k;
  m_nodes[l].edge = (int) arc_id;
}

enum lvx_plc_result
lvx_plc::solve (HOST_WIDE_INT lambda_lower_bound)
{
  unsigned a, e;

  m_lambda = lambda_lower_bound > 0 ? lambda_lower_bound : 1;
  m_circuit.truncate (0);
  if (m_nodes.is_empty ())
    return LVX_PLC_SOLVED;
  if (!build_initial_tree ())
    return LVX_PLC_UNREACHABLE;

  m_queued.truncate (0);
  m_queued.safe_grow_cleared (m_arcs.length ());
  m_worklist.truncate (0);
  for (a = 0, e = m_arcs.length (); a != e; a++)
    {
      m_queued[a] = true;
      m_worklist.safe_push (a);
    }

  /* Generous: every correction raises some potential, and the period only
     ever rises.  Hitting this means a bug, not a hard instance.  */
  const unsigned HOST_WIDE_INT limit
    = 64 * ((unsigned HOST_WIDE_INT) m_arcs.length () + 1)
      * ((unsigned HOST_WIDE_INT) m_nodes.length () + 1) + 1024;
  unsigned HOST_WIDE_INT steps = 0;

  while (!m_worklist.is_empty ())
    {
      if (++steps > limit)
	return LVX_PLC_ITERATION_LIMIT;

      unsigned arc_id = m_worklist.pop ();
      m_queued[arc_id] = false;
      if (!violated (reduced (m_arcs[arc_id])))
	continue;

      switch (check_circuit (arc_id))
	{
	case CIRCUIT_NO_DISTANCE:
	  return LVX_PLC_INFEASIBLE_CIRCUIT;

	case CIRCUIT_PERIOD_RAISED:
	  /* The potentials stay valid as functions of the period -- they are
	     tree path sums -- so nothing is recomputed.  But an arc satisfied
	     at the old period need not be at the new one, so the worklist is
	     refilled.  */
	  for (a = 0, e = m_arcs.length (); a != e; a++)
	    if (!m_queued[a])
	      {
		m_queued[a] = true;
		m_worklist.safe_push (a);
	      }
	  continue;

	case CIRCUIT_NONE:
	  break;
	}

      reparent (arc_id);
    }
  return LVX_PLC_SOLVED;
}

#if CHECKING_P

namespace selftest {

/* The dependence graph of Ning and Gao as used by Figure 1 of Dupont de
   Dinechin, LCPC 1996: three operations, a multiply of latency 2, and one
   recurrence of distance 2 closing the loop.  Two circuits, 1->2->3->1 of
   length 4 over distance 2 and 1->3->1 of length 2 over distance 2; RecMII
   is the larger ceiling, 2.  */

static void
lvx_plc_test_lcpc96_figure1 (void)
{
  lvx_plc g;
  for (unsigned i = 0; i != 4; i++)
    g.add_node ();
  g.add_arc (0, 1, 0, 0);
  g.add_arc (1, 2, 1, 0);
  g.add_arc (1, 3, 1, 0);
  g.add_arc (2, 3, 2, 0);
  unsigned rec = g.add_arc (3, 1, 1, 2);

  ASSERT_EQ (LVX_PLC_SOLVED, g.solve ());
  ASSERT_EQ (2, g.period ());
  ASSERT_EQ (0, g.date (0));
  ASSERT_EQ (0, g.date (1));
  ASSERT_EQ (1, g.date (2));
  ASSERT_EQ (3, g.date (3));
  /* The recurrence is tight.  */
  ASSERT_EQ (g.date (1), g.date (3) + 1 - 2 * g.period ());
  ASSERT_TRUE (!g.critical_circuit ().is_empty ());
  ASSERT_EQ (rec, g.critical_circuit ()[0]);
}

/* With no recurrence the period is whatever lower bound it was given, and the
   potentials are the plain longest path.  */

static void
lvx_plc_test_no_recurrence (void)
{
  lvx_plc g;
  for (unsigned i = 0; i != 4; i++)
    g.add_node ();
  g.add_arc (0, 1, 1, 0);
  g.add_arc (1, 2, 2, 0);
  g.add_arc (1, 3, 1, 0);
  g.add_arc (2, 3, 3, 0);

  ASSERT_EQ (LVX_PLC_SOLVED, g.solve (7));
  ASSERT_EQ (7, g.period ());
  /* 1 + 2 + 3, not the shorter 1 + 1.  */
  ASSERT_EQ (6, g.date (3));
  ASSERT_TRUE (g.critical_circuit ().is_empty ());
}

/* A self-loop is a recurrence like any other: an accumulator read and written
   by one operation, at distance one, sets the period to its own latency.  */

static void
lvx_plc_test_self_loop (void)
{
  lvx_plc g;
  for (unsigned i = 0; i != 2; i++)
    g.add_node ();
  g.add_arc (0, 1, 0, 0);
  g.add_arc (1, 1, 5, 1);

  ASSERT_EQ (LVX_PLC_SOLVED, g.solve ());
  ASSERT_EQ (5, g.period ());
}

/* The period is the maximum over circuits, not the first one found.  */

static void
lvx_plc_test_maximum_over_circuits (void)
{
  lvx_plc g;
  for (unsigned i = 0; i != 5; i++)
    g.add_node ();
  g.add_arc (0, 1, 0, 0);
  g.add_arc (1, 2, 1, 0);
  g.add_arc (2, 1, 1, 1);	/* ceil (2/1) = 2 */
  g.add_arc (0, 3, 0, 0);
  g.add_arc (3, 4, 7, 0);
  g.add_arc (4, 3, 2, 2);	/* ceil (9/2) = 5 */

  ASSERT_EQ (LVX_PLC_SOLVED, g.solve ());
  ASSERT_EQ (5, g.period ());
}

/* Raising the period never lowers it below a resource bound handed in.  */

static void
lvx_plc_test_respects_lower_bound (void)
{
  lvx_plc g;
  for (unsigned i = 0; i != 2; i++)
    g.add_node ();
  g.add_arc (0, 1, 0, 0);
  g.add_arc (1, 1, 3, 1);

  ASSERT_EQ (LVX_PLC_SOLVED, g.solve (11));
  /* max (RecMII 3, ResMII 11).  */
  ASSERT_EQ (11, g.period ());
}

/* A positive circuit carrying no distance cannot be made feasible by any
   period -- it is a dependence cycle within one iteration.  */

static void
lvx_plc_test_positive_circuit_without_distance (void)
{
  lvx_plc g;
  for (unsigned i = 0; i != 3; i++)
    g.add_node ();
  g.add_arc (0, 1, 0, 0);
  g.add_arc (1, 2, 1, 0);
  g.add_arc (2, 1, 1, 0);

  ASSERT_EQ (LVX_PLC_INFEASIBLE_CIRCUIT, g.solve ());
}

/* The root must reach every node along zero-distance arcs, which a dependence
   graph's source node provides; say so rather than producing a potential that
   means nothing.  */

static void
lvx_plc_test_unreachable_node (void)
{
  lvx_plc g;
  for (unsigned i = 0; i != 3; i++)
    g.add_node ();
  g.add_arc (0, 1, 1, 0);
  /* Node 2 is reachable only across an iteration boundary.  */
  g.add_arc (1, 2, 1, 1);

  ASSERT_EQ (LVX_PLC_UNREACHABLE, g.solve ());
}

/* The potentials are affine in the period, and the NU component is the negated
   distance along the tree path -- which is what lets a dichotomy raise the
   period without re-solving.  */

static void
lvx_plc_test_potentials_are_affine (void)
{
  lvx_plc g;
  for (unsigned i = 0; i != 3; i++)
    g.add_node ();
  g.add_arc (0, 1, 0, 0);
  g.add_arc (1, 2, 4, 0);
  g.add_arc (2, 1, 0, 1);	/* forces lambda >= 4 */

  ASSERT_EQ (LVX_PLC_SOLVED, g.solve ());
  ASSERT_EQ (4, g.period ());
  lvx_affine_period p2 = g.potential (2);
  ASSERT_EQ (g.date (2), p2.at (g.period ()));
}

/* Run all of the above.  */

void
lvx_plc_cc_tests (void)
{
  lvx_plc_test_lcpc96_figure1 ();
  lvx_plc_test_no_recurrence ();
  lvx_plc_test_self_loop ();
  lvx_plc_test_maximum_over_circuits ();
  lvx_plc_test_respects_lower_bound ();
  lvx_plc_test_positive_circuit_without_distance ();
  lvx_plc_test_unreachable_node ();
  lvx_plc_test_potentials_are_affine ();
}

} /* namespace selftest */

#endif /* CHECKING_P */
