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

/* See lvx-network.h for what this is and why.  */

#define IN_TARGET_CODE 1

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "vec.h"
#include "selftest.h"
#include "config/lvx/lvx-network.h"

/* A cap on augmentations, so a mistake reports rather than hangs -- the same
   discipline lvx-plc.cc applies to its relaxation rounds.  */
#define LVX_NETWORK_MAX_ROUNDS 100000

unsigned
lvx_network::add_node (HOST_WIDE_INT requirement)
{
  node n;
  n.requirement = requirement;
  n.pi = 0;
  m_nodes.safe_push (n);
  if (requirement > 0)
    m_total_supply += requirement;
  return m_nodes.length () - 1;
}

unsigned
lvx_network::add_arc (unsigned tail, unsigned head, HOST_WIDE_INT capacity,
		      HOST_WIDE_INT cost)
{
  gcc_checking_assert (tail < m_nodes.length () && head < m_nodes.length ());
  arc a;
  a.tail = tail;
  a.head = head;
  a.capacity = capacity;
  a.cost = cost;
  a.flow = 0;
  m_arcs.safe_push (a);
  return m_arcs.length () - 1;
}

void
lvx_network::set_requirement (unsigned nd, HOST_WIDE_INT requirement)
{
  gcc_checking_assert (nd < m_nodes.length ());
  HOST_WIDE_INT was = m_nodes[nd].requirement;
  if (was > 0)
    m_total_supply -= was;
  m_nodes[nd].requirement = requirement;
  if (requirement > 0)
    m_total_supply += requirement;
}

/* Seed the potentials with a shortest-path labelling over the arc costs, by
   Bellman-Ford because the costs may be negative.  This is what makes the
   reduced costs `cost + pi (tail) - pi (head)' non-negative afterwards, so the
   augmenting searches below can be ordinary shortest-path searches rather than
   negative-weight ones.  Returns false on a negative circuit, which for MCRL
   means the formulation is wrong rather than the loop unschedulable.  */

bool
lvx_network::initial_potentials (void)
{
  unsigned n = m_nodes.length ();
  for (unsigned i = 0; i < n; i++)
    m_nodes[i].pi = 0;

  for (unsigned round = 0; round < n; round++)
    {
      bool moved = false;
      for (unsigned a = 0; a < m_arcs.length (); a++)
	{
	  const arc &e = m_arcs[a];
	  if (e.capacity == 0)
	    continue;
	  HOST_WIDE_INT want = m_nodes[e.tail].pi + e.cost;
	  if (want < m_nodes[e.head].pi)
	    {
	      m_nodes[e.head].pi = want;
	      moved = true;
	    }
	}
      if (!moved)
	return true;
    }

  /* Still moving after N rounds: a negative circuit.  */
  return false;
}

/* A RADIX-4 HEAP WITH DECREASE-KEY.  §7tricies.240.

   The design is Benoit Dupont de Dinechin's PQueue in LAO's CDT/CCL, itself
   after R. E. Tarjan, "Data Structures and Network Algorithms": a d-ary heap
   in an array, children of x at d*x + 1 .. d*x + d and parent at
   (x - 1) / d, with d = 4.  Reimplemented here rather than copied, in GCC's
   containers and with the one change the use demands.

   WHY FOUR AND NOT TWO, which is the whole of what the choice buys: sifting
   UP costs log_d (n) comparisons and sifting DOWN costs d * log_d (n),
   because a down step must find the smallest of d children.  Dijkstra does
   at most one decrease-key per ARC and exactly one extract-min per NODE, and
   m is several times n here, so the frequent operation must be the cheap
   one.  A binary heap makes every decrease-key a quarter dearer in depth for
   an extract-min we perform n times only.

   THE CHANGE: PQueue locates an entry to update by pointer arithmetic into
   its array, which needs the caller to know where the entry currently is.
   Dijkstra knows a NODE and not its slot, so M_POS carries node -> slot and
   is maintained by both sift directions.  That is the only part of a d-ary
   heap that is easy to get wrong, and the selftests below check it.  */

namespace {

class lvx_pqueue
{
public:
  void init (unsigned n)
  {
    m_heap.truncate (0);
    m_pos.truncate (0);
    m_key.truncate (0);
    for (unsigned i = 0; i < n; i++)
      {
	m_pos.safe_push (-1);
	m_key.safe_push (0);
      }
  }

  bool empty (void) const { return m_heap.is_empty (); }

  /* Insert V at K, or lower its key to K when it is already in.  A key that
     is not an improvement is ignored, which is what lets the caller push
     unconditionally on a successful relaxation.  */
  void push (unsigned v, HOST_WIDE_INT k)
  {
    if (m_pos[v] < 0)
      {
	m_key[v] = k;
	m_heap.safe_push (v);
	m_pos[v] = (int) m_heap.length () - 1;
	sift_up ((unsigned) m_pos[v]);
      }
    else if (k < m_key[v])
      {
	m_key[v] = k;
	sift_up ((unsigned) m_pos[v]);
      }
  }

  unsigned pop (void)
  {
    unsigned top = m_heap[0];
    unsigned last = m_heap.pop ();

    m_pos[top] = -1;
    if (!m_heap.is_empty ())
      {
	place (0, last);
	sift_down (0);
      }
    return top;
  }

private:
  void place (unsigned x, unsigned v) { m_heap[x] = v; m_pos[v] = (int) x; }

  void sift_up (unsigned x)
  {
    unsigned v = m_heap[x];

    while (x > 0)
      {
	unsigned p = (x - 1) >> 2;

	if (m_key[m_heap[p]] <= m_key[v])
	  break;
	place (x, m_heap[p]);
	x = p;
      }
    place (x, v);
  }

  void sift_down (unsigned x)
  {
    unsigned n = m_heap.length ();
    unsigned v = m_heap[x];

    for (;;)
      {
	unsigned first = 4 * x + 1, best = first;

	if (first >= n)
	  break;
	for (unsigned c = first + 1; c < first + 4 && c < n; c++)
	  if (m_key[m_heap[c]] < m_key[m_heap[best]])
	    best = c;
	if (m_key[m_heap[best]] >= m_key[v])
	  break;
	place (x, m_heap[best]);
	x = best;
      }
    place (x, v);
  }

  auto_vec<unsigned> m_heap;	/* slot -> node */
  auto_vec<int> m_pos;		/* node -> slot, -1 when absent */
  auto_vec<HOST_WIDE_INT> m_key;
};

} /* anonymous namespace */

/* EVERY ARC AT BOTH ITS ENDS, so a relaxation walks a node's own arcs instead
   of all of them.  §7tricies.240.  The scan this replaces visited every arc
   for every settled node, which made one augmentation O (n * m); the index is
   built once per solve because only flows and potentials change after it.  */

void
lvx_network::build_incidence (void)
{
  unsigned n = m_nodes.length (), m = m_arcs.length ();

  m_first.truncate (0);
  m_inc.truncate (0);
  m_first.safe_grow_cleared (n + 2);
  for (unsigned a = 0; a < m; a++)
    {
      m_first[m_arcs[a].tail + 1]++;
      m_first[m_arcs[a].head + 1]++;
    }
  for (unsigned v = 0; v < n + 1; v++)
    m_first[v + 1] += m_first[v];
  m_inc.safe_grow_cleared (2 * m);
  {
    auto_vec<unsigned> at;
    for (unsigned v = 0; v <= n; v++)
      at.safe_push (m_first[v]);
    for (unsigned a = 0; a < m; a++)
      {
	m_inc[at[m_arcs[a].tail]++] = 2 * a;		/* forward at the tail */
	m_inc[at[m_arcs[a].head]++] = 2 * a + 1;	/* backward at the head */
      }
  }
}

/* Push AMOUNT units from SRC to DST along a cheapest residual path, updating
   the potentials so reduced costs stay non-negative.  Dijkstra over the
   residual network, which is legitimate exactly because of that invariant.  */

bool
lvx_network::augment (unsigned src, unsigned dst, HOST_WIDE_INT amount)
{
  unsigned n = m_nodes.length ();

  while (amount > 0)
    {
      /* Cheapest residual distances, in reduced cost.  Dijkstra over the
	 incidence index with the radix-4 heap above (§7tricies.240); the
	 reduced costs are non-negative by the potentials' invariant, which
	 is what makes a Dijkstra legitimate here at all.  */
      auto_vec<HOST_WIDE_INT> dist;
      auto_vec<int> from_arc;
      auto_vec<bool> forward, done;
      lvx_pqueue pq;

      dist.safe_grow_cleared (n);
      from_arc.safe_grow_cleared (n);
      forward.safe_grow_cleared (n);
      done.safe_grow_cleared (n);
      for (unsigned i = 0; i < n; i++)
	{
	  dist[i] = HOST_WIDE_INT_MAX;
	  from_arc[i] = -1;
	  forward[i] = true;
	  done[i] = false;
	}
      pq.init (n);
      dist[src] = 0;
      pq.push (src, 0);

      while (!pq.empty ())
	{
	  unsigned u = pq.pop ();

	  if (done[u])
	    continue;			/* a stale entry; the heap keeps none,
					   but a guard costs nothing */
	  done[u] = true;

	  for (unsigned k = m_first[u]; k < m_first[u + 1]; k++)
	    {
	      unsigned a = m_inc[k] >> 1;
	      bool back = (m_inc[k] & 1) != 0;
	      const arc &e = m_arcs[a];
	      HOST_WIDE_INT rc;
	      unsigned v;

	      if (!back)
		{
		  /* Forward residual, from this arc's tail.  */
		  if (!(e.capacity < 0 || e.flow < e.capacity))
		    continue;
		  rc = e.cost + m_nodes[e.tail].pi - m_nodes[e.head].pi;
		  v = e.head;
		}
	      else
		{
		  /* Backward residual: undoing flow costs the negation.  */
		  if (e.flow <= 0)
		    continue;
		  rc = -e.cost + m_nodes[e.head].pi - m_nodes[e.tail].pi;
		  v = e.tail;
		}
	      gcc_checking_assert (rc >= 0);
	      if (done[v] || dist[u] + rc >= dist[v])
		continue;
	      dist[v] = dist[u] + rc;
	      from_arc[v] = (int) a;
	      forward[v] = !back;
	      pq.push (v, dist[v]);
	    }
	}

      if (dist[dst] == HOST_WIDE_INT_MAX)
	return false;			/* no residual path: infeasible */

      /* Shift the potentials so the invariant survives the next round.  */
      for (unsigned i = 0; i < n; i++)
	if (dist[i] != HOST_WIDE_INT_MAX)
	  m_nodes[i].pi += dist[i];

      /* The bottleneck along the path.  */
      HOST_WIDE_INT push = amount;
      for (unsigned v = dst; v != src; )
	{
	  int a = from_arc[v];
	  const arc &e = m_arcs[a];
	  HOST_WIDE_INT room = forward[v]
			       ? (e.capacity < 0 ? push : e.capacity - e.flow)
			       : e.flow;
	  if (room < push)
	    push = room;
	  v = forward[v] ? e.tail : e.head;
	}
      gcc_checking_assert (push > 0);

      for (unsigned v = dst; v != src; )
	{
	  int a = from_arc[v];
	  arc &e = m_arcs[a];
	  if (forward[v])
	    {
	      e.flow += push;
	      v = e.tail;
	    }
	  else
	    {
	      e.flow -= push;
	      v = e.head;
	    }
	}
      amount -= push;
    }

  return true;
}

enum lvx_network_result
lvx_network::solve (void)
{
  unsigned n = m_nodes.length ();
  m_cost = 0;
  if (n == 0)
    return LVX_NETWORK_SOLVED;

  for (unsigned a = 0; a < m_arcs.length (); a++)
    m_arcs[a].flow = 0;

  build_incidence ();
  if (!initial_potentials ())
    return LVX_NETWORK_UNBOUNDED;

  /* Route each supply to a deficit.  Taking them one pair at a time keeps the
     invariant simple at the cost of more searches, which is the trade this
     file makes against a simplex.  */
  int rounds = 0;
  for (unsigned s = 0; s < n; s++)
    {
      while (m_nodes[s].requirement > 0)
	{
	  if (++rounds > LVX_NETWORK_MAX_ROUNDS)
	    return LVX_NETWORK_ITERATION_LIMIT;

	  unsigned d = n;
	  for (unsigned t = 0; t < n; t++)
	    if (m_nodes[t].requirement < 0)
	      {
		d = t;
		break;
	      }
	  if (d == n)
	    break;			/* nothing left wanting flow */

	  HOST_WIDE_INT amount = m_nodes[s].requirement;
	  if (-m_nodes[d].requirement < amount)
	    amount = -m_nodes[d].requirement;

	  if (!augment (s, d, amount))
	    return LVX_NETWORK_INFEASIBLE;

	  m_nodes[s].requirement -= amount;
	  m_nodes[d].requirement += amount;
	}
    }

  for (unsigned s = 0; s < n; s++)
    if (m_nodes[s].requirement != 0)
      return LVX_NETWORK_INFEASIBLE;

  for (unsigned a = 0; a < m_arcs.length (); a++)
    m_cost += m_arcs[a].flow * m_arcs[a].cost;

  return LVX_NETWORK_SOLVED;
}

#if CHECKING_P

namespace selftest {

/* One supply, one demand, two paths of different cost: the flow must take the
   cheap one.  */

static void
lvx_network_test_two_paths (void)
{
  lvx_network net;
  unsigned s = net.add_node (3);
  unsigned m1 = net.add_node (0);
  unsigned m2 = net.add_node (0);
  unsigned t = net.add_node (-3);
  unsigned cheap1 = net.add_arc (s, m1, 10, 1);
  unsigned cheap2 = net.add_arc (m1, t, 10, 1);
  unsigned dear1 = net.add_arc (s, m2, 10, 5);
  unsigned dear2 = net.add_arc (m2, t, 10, 5);

  ASSERT_EQ (net.solve (), LVX_NETWORK_SOLVED);
  ASSERT_EQ (net.flow (cheap1), 3);
  ASSERT_EQ (net.flow (cheap2), 3);
  ASSERT_EQ (net.flow (dear1), 0);
  ASSERT_EQ (net.flow (dear2), 0);
  ASSERT_EQ (net.cost (), 6);
}

/* When the cheap path is too narrow the rest must spill onto the dear one,
   which is what distinguishes a min-cost flow from a shortest path.  */

static void
lvx_network_test_capacity_forces_split (void)
{
  lvx_network net;
  unsigned s = net.add_node (4);
  unsigned t = net.add_node (-4);
  unsigned cheap = net.add_arc (s, t, 1, 1);
  unsigned dear = net.add_arc (s, t, 10, 7);

  ASSERT_EQ (net.solve (), LVX_NETWORK_SOLVED);
  ASSERT_EQ (net.flow (cheap), 1);
  ASSERT_EQ (net.flow (dear), 3);
  ASSERT_EQ (net.cost (), 1 * 1 + 3 * 7);
}

/* THE TUPLE CASE, which is the reason this file exists.  Two producers feed
   one consumer over paths of equal cost per unit; the one producing a
   four-register quad supplies four units where the single supplies one, so the
   optimum cost is weighted by the tuple width without any arc cost being
   scaled.  */

static void
lvx_network_test_producer_supplies_nregs (void)
{
  lvx_network net;
  unsigned single = net.add_node (1);		/* nregs = 1 */
  unsigned quad = net.add_node (4);		/* nregs = 4 */
  unsigned sink = net.add_node (-5);
  unsigned a1 = net.add_arc (single, sink, -1, 2);
  unsigned a4 = net.add_arc (quad, sink, -1, 2);

  ASSERT_EQ (net.solve (), LVX_NETWORK_SOLVED);
  ASSERT_EQ (net.flow (a1), 1);
  ASSERT_EQ (net.flow (a4), 4);
  /* 2 * (1 + 4): the quad costs four times the single over the same arc
     length, which is `min sum nregs (i) * life (i)'.  */
  ASSERT_EQ (net.cost (), 10);
}

/* set_requirement must keep the supply total straight, since a graph builder
   fills tuple widths in after the nodes exist.  */

static void
lvx_network_test_set_requirement (void)
{
  lvx_network net;
  unsigned s = net.add_node (0);
  unsigned t = net.add_node (0);
  unsigned e = net.add_arc (s, t, -1, 3);
  net.set_requirement (s, 2);
  net.set_requirement (t, -2);

  ASSERT_EQ (net.solve (), LVX_NETWORK_SOLVED);
  ASSERT_EQ (net.flow (e), 2);
  ASSERT_EQ (net.cost (), 6);

  /* And overwriting a supply replaces it rather than accumulating.  */
  lvx_network net2;
  unsigned s2 = net2.add_node (9);
  unsigned t2 = net2.add_node (-1);
  net2.set_requirement (s2, 1);
  unsigned e2 = net2.add_arc (s2, t2, -1, 1);
  ASSERT_EQ (net2.solve (), LVX_NETWORK_SOLVED);
  ASSERT_EQ (net2.flow (e2), 1);
}

/* A supply with nowhere to go is infeasible, and says so.  */

static void
lvx_network_test_infeasible (void)
{
  lvx_network net;
  net.add_node (2);
  net.add_node (-2);
  /* No arc between them.  */
  ASSERT_EQ (net.solve (), LVX_NETWORK_INFEASIBLE);
}

/* A negative circuit of unbounded capacity has no finite optimum.  */

static void
lvx_network_test_unbounded (void)
{
  lvx_network net;
  unsigned a = net.add_node (1);
  unsigned b = net.add_node (-1);
  net.add_arc (a, b, -1, 1);
  net.add_arc (b, a, -1, -5);
  ASSERT_EQ (net.solve (), LVX_NETWORK_UNBOUNDED);
}

/* An empty instance is solved, not a crash.  */

static void
lvx_network_test_empty (void)
{
  lvx_network net;
  ASSERT_EQ (net.solve (), LVX_NETWORK_SOLVED);
  ASSERT_EQ (net.cost (), 0);
}

/* THE OPTIMALITY CERTIFICATE, over pseudo-random networks.  §7tricies.240.

   The right test for a min-cost flow is not "does it agree with another
   implementation" but the condition that DEFINES an optimum: with potentials
   pi, every residual arc must have a non-negative reduced cost.  Equivalently,
   for every arc, flow < capacity implies rc >= 0 and flow > 0 implies
   rc <= 0 -- so an arc strictly between its bounds has rc exactly 0.  That
   is checkable without a reference solver and it catches precisely what a
   wrong heap would produce: a path that was not the cheapest, leaving some
   arc with a negative reduced cost.

   Conservation is checked alongside, since a heap bug can also lose flow.
   The generator is a fixed-seed xorshift so a failure is reproducible.  */

static void
lvx_network_test_optimality_certificate (void)
{
  unsigned state = 0x9e3779b9u;
  auto rnd = [&state] (unsigned m) -> unsigned
    {
      state ^= state << 13;
      state ^= state >> 17;
      state ^= state << 5;
      return state % m;
    };

  for (int trial = 0; trial < 40; trial++)
    {
      lvx_network net;
      unsigned n = 4 + rnd (12);
      auto_vec<HOST_WIDE_INT> req;
      HOST_WIDE_INT bal = 0;

      for (unsigned i = 0; i < n; i++)
	req.safe_push (0);
      /* A few supply/demand pairs, so the instance is feasible by
	 construction once the arcs below connect everything.  */
      for (unsigned k = 0; k < 1 + rnd (3); k++)
	{
	  unsigned sp = rnd (n), dm = rnd (n);
	  HOST_WIDE_INT amt = 1 + rnd (4);

	  if (sp == dm)
	    continue;
	  req[sp] += amt;
	  req[dm] -= amt;
	}
      for (unsigned i = 0; i < n; i++)
	{
	  net.add_node (req[i]);
	  bal += req[i];
	}
      ASSERT_EQ (bal, 0);
      /* A ring, so every node reaches every other, then random chords.  */
      for (unsigned i = 0; i < n; i++)
	net.add_arc (i, (i + 1) % n, -1, (HOST_WIDE_INT) rnd (9));
      for (unsigned k = 0; k < n * 2; k++)
	{
	  unsigned t = rnd (n), h = rnd (n);

	  if (t != h)
	    net.add_arc (t, h, (HOST_WIDE_INT) (1 + rnd (6)),
			 (HOST_WIDE_INT) rnd (9));
	}

      if (net.solve () != LVX_NETWORK_SOLVED)
	continue;			/* infeasible draws are not the test */

      /* Conservation at every node.  */
      for (unsigned i = 0; i < n; i++)
	{
	  HOST_WIDE_INT net_out = 0;

	  for (unsigned a = 0; a < net.arc_count (); a++)
	    {
	      if (net.arc_tail (a) == i)
		net_out += net.flow (a);
	      if (net.arc_head (a) == i)
		net_out -= net.flow (a);
	    }
	  ASSERT_EQ (net_out, req[i]);
	}

      /* And the certificate.  */
      for (unsigned a = 0; a < net.arc_count (); a++)
	{
	  HOST_WIDE_INT rc = (net.arc_cost (a)
			      + net.potential (net.arc_tail (a))
			      - net.potential (net.arc_head (a)));
	  HOST_WIDE_INT f = net.flow (a), cap = net.arc_capacity (a);

	  if (cap < 0 || f < cap)
	    ASSERT_TRUE (rc >= 0);
	  if (f > 0)
	    ASSERT_TRUE (rc <= 0);
	}
    }
}

/* Run all of the above.  */

void
lvx_network_cc_tests (void)
{
  lvx_network_test_optimality_certificate ();
  lvx_network_test_empty ();
  lvx_network_test_two_paths ();
  lvx_network_test_capacity_forces_split ();
  lvx_network_test_producer_supplies_nregs ();
  lvx_network_test_set_requirement ();
  lvx_network_test_infeasible ();
  lvx_network_test_unbounded ();
}

} /* namespace selftest */

#endif /* CHECKING_P */
