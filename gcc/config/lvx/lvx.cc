/* Definitions of target machine for GNU compiler LVX cores.
   Copyright (C) 1991-2014 Free Software Foundation, Inc.
   Copyright (C) 2017 Kalray

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

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

static void lvx_sched2_dtor (void);
static void lvx_sched2_ctor (int max_uid);
static void lvx_ifcvt_dtor (void);
static void lvx_ifcvt_ctor (void);

enum lvx_ifcvt_ce lvx_ifcvt_ce_level;

#ifndef __OPTIMIZE__
int lvx_ifcvt_count = INT_MAX;
int lvx_aspec_count = INT_MAX;
#endif //__OPTIMIZE__

static struct lvx_ifcvt
{
  enum lvx_ifcvt_ce ce_level;
  int block_count;
  sbitmap block_visited;
  rtx_insn *recog_insn;
  recog_data_d recog_data;
  // Cleared by lvx_ifcvt_reset.
  rtx_insn *then_start;
  rtx_insn *then_end;
  rtx_insn *else_start;
  rtx_insn *else_end;
} lvx_ifcvt[1];

/* The processor resources each issue class occupies, generated from the
   machine description and indexed by the "issue" attribute.  */
#include "config/lvx/lvx-issue-table.inc"

/* Anything that reads memory into a register.  Was a range test over the type
   enum; "type" is semantic now, so ask directly.  */
static bool
lvx_load_class_p (enum attr_type type)
{
  switch (type)
    {
    case TYPE_LOAD: case TYPE_XLOAD:
    case TYPE_ALOAD: case TYPE_ALOADC: case TYPE_ATOMIC:
      return true;
    default:
      return false;
    }
}

/* The multiply-accumulate family, which reads its accumuland a cycle late.  */
static bool
lvx_accumulate_class_p (enum attr_type type)
{
  switch (type)
    {
    case TYPE_IMADD: case TYPE_FMADDS: case TYPE_FMADDD:
    case TYPE_FDOTP: case TYPE_FDMDA:
      return true;
    default:
      return false;
    }
}

/* The multiply/multiply-accumulate family, whose results the scoreboard has to
   wait longer for.  This used to be a range test over the type enum, which
   asked the reader to keep it in step with the order of types.md; "type" is
   semantic now, so the question can be asked directly.  */
static bool
lvx_multiply_class_p (enum attr_type type)
{
  switch (type)
    {
    case TYPE_IMUL: case TYPE_IMADD:
    case TYPE_FMULS: case TYPE_FMADDS:
    case TYPE_FMULD: case TYPE_FMADDD:
    case TYPE_FCVT: case TYPE_FDOTP: case TYPE_FDMDA:
      return true;
    default:
      return false;
    }
}

/* The BCU prefix kinds that share a bundle's two BCU syllables.  Tagging each
   occupied slot with its kind keeps a MASKS mask keyed on $rN from aliasing a
   GUARD condition, or a MASKM byte-enable, on the same register: a slot is
   shared only when kind, lanetodo and the key register all match.  */
enum lvx_bcu_kind { BCU_PLAIN, BCU_GUARD, BCU_MASKS, BCU_MASKM };

/* Defined with the scheduler hooks, used by lvx_insn_cost above them.  */
static bool lvx_masked_prefix (rtx_insn *, enum lvx_bcu_kind *, rtx *, int *);

/* SCHED2 data structure.  */
static struct lvx_sched2
{
  int max_uid;
  int prev_uid;
  short *insn_cycle;
  unsigned char *insn_flags;
  rtx bcu_use[2];                  /* Key rtx occupying each BCU slot, 0 free. */
  enum lvx_bcu_kind bcu_kind[2];   /* Which prefix kind that slot holds. */
  int bcu_lanetodo[2];             /* lanetodo polarity for a mask-prefix slot. */
  rtx_insn *cond_insn;
  /* A scratch DFA state, state_size () bytes -- one byte per automaton, so it
     grows whenever scheduling.md is split into more of them.  It used to be a
     fixed size_t[1] copied with sizeof, which state_transition then read and
     wrote past, into bundle_size below.  state_size () rather than
     dfa_state_size: the constructor below runs before schedule_ebbs, so
     dfa_start () has not set dfa_state_size yet on the first function.  */
  void *bundle_state;
  int bundle_size;
} lvx_sched2[1];

#define LVX_SCHED2_INSN_HEAD 1	// Head of scheduling region.
#define LVX_SCHED2_INSN_START 2	// Start instruction bundle.
#define LVX_SCHED2_INSN_STOP 4	// Stop instruction bundle.
#define LVX_SCHED2_INSN_STALL 8	// Stall (scoreboard bug).
#define LVX_SYLLABLE_SIZE 4
#define LVX_SCHED2_BUNDLE_SIZE 32

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
static tree lvx_handle_fndecl_attribute (tree *node, tree name,
					 tree args ATTRIBUTE_UNUSED,
					 int flags ATTRIBUTE_UNUSED,
					 bool *no_add_attrs);

static bool function_symbol_referenced_p (rtx x);

static bool symbolic_reference_mentioned_p (rtx op);

static bool lvx_output_addr_const_extra (FILE *, rtx);

static bool lvx_legitimate_address_p (enum machine_mode mode, rtx x,
				      bool strict, code_helper ch);

bool lvx_legitimate_pic_symbolic_ref_p (rtx op);

static bool lvx_legitimate_constant_p (enum machine_mode, rtx x);
static bool lvx_shaker_enabled = false;
static char lvx_final_end_bundle[128];

rtx lvx_link_reg_rtx;

rtx lvx_divmod_zero;

/* TLS prototypes {{{ */
static const char *lvx_unspec_tls_asm_op[]
  = { "@tlsgd", "@tlsld", "@tlsle", "@dtpoff", "@tlsie" };

int lvx_has_tls_reference (rtx x);
static int lvx_has_unspec_reference_1 (rtx *x);
static int lvx_has_unspec_reference (rtx x);
static enum tls_model tls_symbolic_operand_type (rtx addr);
static bool lvx_tls_symbol_p (rtx x);
static rtx gen_set_gotp_insn (rtx target);
static enum lvx_symbol_type lvx_classify_tls_symbol (rtx x);
static enum lvx_symbol_type lvx_classify_symbol (rtx x);
static rtx_insn * lvx_call_tls_get_addr (rtx sym, rtx result, int unspec);
static rtx lvx_legitimize_tls_reference (rtx x);
static bool function_symbol_referenced_p (rtx x);
/* TLS prototypes }}} */

bool symbolic_reference_mentioned_p (rtx op);
/* Which arch are we scheduling for */
enum attr_arch lvx_arch_schedule;

static bool lvx_print_offset_zero;


/* Returns TRUE if OP is a hard (sub)register aligned for MODE.  */
bool
lvx_hardreg_aligned_p (rtx op, machine_mode mode)
{
  unsigned align = (GET_MODE_SIZE (mode) + UNITS_PER_WORD - 1) / UNITS_PER_WORD;

  if (REG_P (op))
    {
      if (!HARD_REGISTER_P (op))
	return true;

      if (REGNO (op) % align == 0)
	return true;

      return false;
    }
  else if (SUBREG_P (op) && REG_P (SUBREG_REG (op)))
    {
      if (!HARD_REGISTER_P (SUBREG_REG (op)))
	return true;

      unsigned wordno = SUBREG_BYTE (op) / UNITS_PER_WORD;
      if ((REGNO (SUBREG_REG (op)) + wordno) % align == 0)
	return true;

      return false;
    }

  return true;
}

/* Helper function for lvx_expand_load_multiple and
   lvx_expand_store_multiple */
static bool
lvx_expand_load_store_multiple (rtx operands[], bool is_load)
{
  const int reg_op_idx = is_load ? 0 : 1;
  const int mem_op_idx = is_load ? 1 : 0;

  int count = INTVAL (operands[2]);
  int regno = REGNO (operands[reg_op_idx]);

  if (GET_CODE (operands[2]) != CONST_INT
      || GET_MODE (operands[reg_op_idx]) != DImode
      || (count != 2 && count != 4)
      || ((count == 2) && (regno & 1)) || ((count == 4) && (regno & 3))
      || !MEM_P (operands[mem_op_idx]) || !REG_P (operands[reg_op_idx])
      || (TARGET_STRICT_ALIGN
	  && ((int) MEM_ALIGN (operands[mem_op_idx]))
	       < (count * UNITS_PER_WORD))
      || MEM_VOLATILE_P (operands[mem_op_idx])
      || REGNO (operands[reg_op_idx]) > LVX_GPR_LAST_REGNO)
    return false;

  operands[3] = gen_rtx_PARALLEL (VOIDmode, rtvec_alloc (count));

  rtx base, offset;
  if (!lvx_split_mem (XEXP (operands[mem_op_idx], 0), &base, &offset, false))
    return false;

  if (!REG_P (base))
    base = force_reg (Pmode, base);

  /* Add a PLUS so that we have a simpler match in load multiple patterns */
  XEXP (operands[mem_op_idx], 0) = gen_rtx_PLUS (Pmode, base, offset);

  for (int i = 0; i < count; i++)
    {
      rtx addr
	= adjust_address_nv (operands[mem_op_idx], DImode, i * UNITS_PER_WORD);

      /* Force a PLUS even for offset 0 so that we have a simpler
         match in load multiple patterns */
      if (REG_P (XEXP (addr, 0)))
	XEXP (addr, 0) = gen_rtx_PLUS (Pmode, XEXP (addr, 0), const0_rtx);

      if (is_load)
	XVECEXP (operands[3], 0, i)
	  = gen_rtx_SET (gen_rtx_REG (DImode, regno + i), addr);
      else
	XVECEXP (operands[3], 0, i)
	  = gen_rtx_SET (addr, gen_rtx_REG (DImode, regno + i));
    }

  return true;
}

/* Expands a store multiple with operand 0 being the first destination
   address, operand 1 the first source register and operand 2 the
   number of consecutive stores to pack. */
bool
lvx_expand_store_multiple (rtx operands[])
{
  return lvx_expand_load_store_multiple (operands, false);
}

/* Expands a load multiple with operand 0 being the first destination
   register, operand 1 the first source address and operand 2 the
   number of consecutive loads to pack. */
bool
lvx_expand_load_multiple (rtx operands[])
{
  return lvx_expand_load_store_multiple (operands, true);
}

/* FIXME AUTO: revisit for LVX.  */
/* See T7749 */
static int
lvx_reassociation_width (unsigned int opc, machine_mode mode) ATTRIBUTE_UNUSED;

static int
lvx_reassociation_width (unsigned int opc, machine_mode mode)
{
  int res = 1;

  /* see tree.c:associative_tree_code () for possible values of opc. */

  switch (opc)
    {
    case BIT_IOR_EXPR:
    case BIT_AND_EXPR:
    case BIT_XOR_EXPR:
      if (mode == SImode || mode == HImode || mode == QImode
	  || mode == DImode)
	res = 4;
      else if (mode == TImode)
	res = 2;
      break;
    case PLUS_EXPR:
    case MIN_EXPR:
    case MAX_EXPR:
      if (mode == SImode || mode == HImode || mode == QImode
	  || mode == DImode)
	res = 4;
      break;
    case MULT_EXPR:
      break;
    }

  return res;
}

/* Utils {{{ */

/* Emit insn to set DEST to the expression computed by ADDRESS.  */
void
lvx_emit_set_address (rtx dest, rtx address)
{
  emit_insn (gen_rtx_SET (dest, address));
}

/* Get the NTH MEM rtx in X if any. */
rtx
lvx_get_mem_rtx (rtx x, int nth = 0)
{
  subrtx_var_iterator::array_type array;
  FOR_EACH_SUBRTX_VAR (iter, array, x, NONCONST)
    {
      rtx mem = *iter;
      if (mem && MEM_P (mem) && !nth--)
	return mem;
    }
  return 0;
}

/**
 *  This helper function checks whether the modifier `mod' is present
 *  in `x'.  `x' should be either a CONST_STRING or a concatenation of
 *  CONST_STRINGs.
 *  NB. It is assumed that all CONST_STRING appearing as subexpressions
 *  are semantically valid modifiers. That is, they are of the form
 *  `.xxx' where xxx is a string of arbitrary length.
 */
bool
lvx_modifier_enabled_p (const char * mod, rtx x)
{
  bool res = false;
  int len = strlen (mod);
  int code = GET_CODE (x);
  if (code == CONST_STRING)
    {
      const char *xstr = XSTR (x, 0);
      if (xstr[0] == '.')
	{
	  int win_start = 1;
	  while (!res)
	    {
	      int win_sz = 0;
	      for (;
		   xstr[win_start + win_sz]
		   && xstr[win_start + win_sz] != '.'; ++win_sz);
	      res = len == win_sz + 1
		&& !strncmp (xstr + win_start - 1, mod, win_sz + 1);
	      if (xstr[win_start + win_sz] == '.')
		win_start += win_sz + 1;
	      else
		break;
	    }
	}
    }
  else if (code == CONCAT)
    res = lvx_modifier_enabled_p (mod, XEXP (x, 0))
      || lvx_modifier_enabled_p (mod, XEXP (x, 1));
  return res;
}

/**
 *  This helper function extract the rounding mode requested by the modifier.
 *  The modifier stored in `rtx x' should be either a CONST_STRING or a
 *  concatenation of CONST_STRING.
 *  NB. It is assumed that all rounding modifiers do begin by ".r", and that all
 *  CONST_STRING subexpressions are valid modifiers.
 *  /!\ If multiple rounding modifiers are present the first one found (wrt deep
 *  first strategy) will be returned.
 */
rtx
lvx_modifier_rounding (rtx x)
{
  int code = GET_CODE (x);
  rtx res = NULL;
  if (code == CONST_STRING)
    {
      const char *xstr = XSTR (x, 0);
      if (xstr[0] == '.')
	{
	  int win_start = 1;
	  while (!res)
	    {
	      int win_sz = 0;
	      for (;
		   xstr[win_start + win_sz]
		   && xstr[win_start + win_sz] != '.'; ++win_sz);
	      if (win_sz == 2 && xstr[win_start] == 'r')
		res = gen_rtx_CONST_STRING (VOIDmode, xstr + win_start - 1);
	      else if (xstr[win_start + win_sz] == '.')
		win_start += win_sz + 1;
	      else
		break;
	    }
	}
    }
  else if (code == CONCAT)
    {
      res = lvx_modifier_rounding (XEXP (x, 0));
      res = res ? res : lvx_modifier_rounding (XEXP (x, 1));
    }

  return res ? res : gen_rtx_CONST_STRING (VOIDmode, "");
}

/* Return the mode of a 64-bit chunk from a vector mode, else the mode. */
static enum machine_mode
lvx_get_chunk_mode (enum machine_mode mode)
{
  machine_mode chunk_mode = mode;
  if (VECTOR_MODE_P (mode))
    {
      machine_mode inner_mode = GET_MODE_INNER (mode);
      unsigned inner_size = GET_MODE_SIZE (inner_mode);
      if (inner_size >= UNITS_PER_WORD)
	chunk_mode = inner_mode;
      else
	{
	  unsigned nunits = UNITS_PER_WORD / inner_size;
	  complex_mode cplx_mode;
	  if (is_complex_int_mode (GET_MODE_INNER (mode), &cplx_mode)
	      || is_complex_float_mode (GET_MODE_INNER (mode), &cplx_mode))
	    chunk_mode = mode_for_vector (GET_MODE_INNER (cplx_mode),
					  nunits * 2).require ();
	  else
	    {
	      scalar_mode sc_mode = GET_MODE_INNER (mode);
	      chunk_mode = mode_for_vector (sc_mode, nunits).require ();
	    }
	}
    }
  gcc_assert (GET_MODE_SIZE (chunk_mode) == UNITS_PER_WORD);
  return chunk_mode;
}

/* Return the mode of a predicate resulting from comparing MODE. */
enum machine_mode
lvx_get_predicate_mode (enum machine_mode mode)
{
  if (VECTOR_MODE_P (mode))
    {
      unsigned nunits = GET_MODE_NUNITS (mode);
      scalar_mode inner_mode = GET_MODE_INNER (mode);
      scalar_int_mode pred_mode = int_mode_for_mode (inner_mode).require ();
      if (GET_MODE_NUNITS (mode) == 1)
	return lvx_get_predicate_mode (pred_mode);

      return mode_for_vector (pred_mode, nunits).require ();
    }

  if (GET_MODE_SIZE (mode) <= GET_MODE_SIZE (SImode))
    return SImode;

  return DImode;
}

static bool
lvx_extension_mode_p (enum machine_mode mode)
{
  switch (mode)
    {
    // 256-bit extension modes
    case E_V1OImode:
    // 512-bit extension modes
    case E_V2OImode:
    // 1024-bit extension modes
    case E_V4OImode:
    // 2048-bit extension modes
    case E_V8OImode:
    // 4096-bit extension modes
    case E_V16OImode:
    // 8192-bit extension modes
    case E_V32OImode:
      return true;
    default:
      break;
    }
  return false;
}

/* Return true when a WIDTH-bit field at bit POS may be read from SRC by a
   single extfsd/extfzd.  The instruction operates on the whole GPR, so the
   field must not straddle a boundary the source does not intend.  For an
   ordinary integer that is the mode: POS + WIDTH <= mode bits.

   For a lane of a packed vector — SRC is a subreg of a vector mode, several
   lanes sharing one GPR (pair) — a field is fine when it stays inside one
   element (an ordinary sub-lane bitfield) OR is element-aligned and spans whole
   elements (reading one or several lanes verbatim, which extfzd does correctly).
   What is *not* fine is a field that starts mid-lane and runs past the element,
   pulling in the neighbouring lane's bits.  combine forms exactly that: it folds
   a right shift into a zero_extract's position without shrinking its width
   (make_extraction), so lane0>>16 of a V4SI becomes zero_extract(subreg:DI, 32,
   16) — 32 bits from bit 16, reaching into lane 1.  Rejecting it keeps combine
   from using extfzd and the shift survives (the vqshrn_n_u32 -O3
   over-saturation), while the element-aligned lane extracts vdup_lane relies on
   still match.  */
bool
lvx_extract_field_ok_p (rtx src, HOST_WIDE_INT width, HOST_WIDE_INT pos)
{
  if (SUBREG_P (src) && VECTOR_MODE_P (GET_MODE (SUBREG_REG (src))))
    {
      unsigned HOST_WIDE_INT elem
	= GET_MODE_UNIT_BITSIZE (GET_MODE (SUBREG_REG (src)));
      if (elem == 0)
	return false;
      return (pos % elem) + width <= elem			/* within a lane */
	     || (pos % elem == 0 && width % elem == 0);		/* whole lanes */
    }
  return pos + width <= GET_MODE_BITSIZE (GET_MODE (src));
}

/* Decompose MASK, read in MODE, into one contiguous run of set bits: *POSP its
   lowest bit, *WIDTHP how many.  The empty and the full mask are rejected --
   neither describes a field, and combine has folded both away long before a
   field insert reaches recog.  */

static bool
lvx_field_mask_p (machine_mode mode, unsigned HOST_WIDE_INT mask,
		  int *posp, int *widthp)
{
  unsigned HOST_WIDE_INT m = mask & GET_MODE_MASK (mode);
  if (m == 0 || m == GET_MODE_MASK (mode))
    return false;

  int pos = ctz_hwi (m);
  unsigned HOST_WIDE_INT run = m >> pos;
  if (run & (run + 1))				/* more than one run of ones */
    return false;

  *posp = pos;
  *widthp = popcount_hwi (m);
  return true;
}

/* True when

     (base & BASEMASK) | ((val << SHIFT) & INSMASK)

   is the insert insfd performs: INSMASK one contiguous field at SHIFT, and
   BASEMASK its exact complement.  SHIFT is NULL_RTX for a field at bit 0,
   where combine leaves no ashift behind.

   insfd masks the source itself -- ((val << pos) & mask) | (base & ~mask), see
   INSFD's Behavior in lvx-mds -- so VAL needs no bits cleared above the field,
   and the two masks are the whole of the correctness condition.  A width of 64
   is out because bitwidth encodes six bits, and it would be a plain move.  */

bool
lvx_insf_mask_p (machine_mode mode, rtx basemask, rtx insmask, rtx shift)
{
  int pos, width;

  if (!CONST_INT_P (basemask) || !CONST_INT_P (insmask))
    return false;

  if (!lvx_field_mask_p (mode, UINTVAL (insmask), &pos, &width))
    return false;

  if (((UINTVAL (basemask) ^ ~UINTVAL (insmask)) & GET_MODE_MASK (mode)) != 0)
    return false;

  if (shift ? INTVAL (shift) != pos : pos != 0)
    return false;

  return width < 64;
}

/* True when

     (base & BASEMASK) | (val << SHIFT)

   is that same insert with the mask on the shifted value folded away, which
   combine does when the field runs to the top of the mode and the mask is
   therefore redundant.  BASEMASK is then the only mask left, and the condition
   that made the elision valid is that base keeps exactly the bits below the
   field -- so BASEMASK must be SHIFT ones.  The field insfd writes is
   everything from SHIFT up, which is what the elided mask said.  */

bool
lvx_insf_topmask_p (machine_mode mode, rtx basemask, rtx shift)
{
  if (!CONST_INT_P (basemask) || !CONST_INT_P (shift))
    return false;

  HOST_WIDE_INT pos = INTVAL (shift);
  if (pos <= 0 || pos >= 64)
    return false;

  return ((UINTVAL (basemask) & GET_MODE_MASK (mode))
	  == (HOST_WIDE_INT_1U << pos) - 1);
}

/* Output template for those inserts, with VALOP the operand number holding the
   value and MASK the field (the complement of the base's mask for the elided
   form above).  The width and position are carried by the mask rather than by
   operands of their own, so they cannot be printed with a % code and this
   returns a template with them already substituted.  Mnemonic: insfd.  */

const char *
lvx_output_insf (unsigned HOST_WIDE_INT mask, machine_mode mode, int valop)
{
  static char buf[32];
  int pos, width;

  if (!lvx_field_mask_p (mode, mask, &pos, &width))
    gcc_unreachable ();

  snprintf (buf, sizeof buf, "insfd %%0 = %%%d, %d, %d", valop, width, pos);
  return buf;
}

/* Utils }}} */

/* Options Handling {{{ */

/* Fixed Reg Option {{{ */

static bool
lvx_handle_fixed_reg_option (const char *arg)
{
  const char *dash;
  int first_reg = -1, last_reg = -1, i;

  if (*arg++ != '=')
    return false;

  dash = strchr (arg, '-');

  for (i = 0; i < FIRST_PSEUDO_REGISTER; ++i)
    if (strncmp (reg_names[i], arg, dash - arg) == 0)
      {
	first_reg = i;
	break;
      }

  if (first_reg < 0)
    {
      error ("Unknown register %<%s%> passed to %<-ffixed-reg%>", arg);
      return false;
    }

  if (dash)
    {
      for (i = 0; i < FIRST_PSEUDO_REGISTER; ++i)
	if (strcmp (reg_names[i], dash + 1) == 0)
	  {
	    last_reg = i;
	    break;
	  }
      if (last_reg < 0)
	{
	  error ("Unknown register %<%s%> passed to %<-ffixed-reg%>",
		 dash + 1);
	  return false;
	}
    }
  else
    {
      last_reg = first_reg;
    }

  if (first_reg > last_reg)
    {
      error ("Invalid range passed to -ffixed-reg.");
      return false;
    }

  for (i = first_reg; i <= last_reg; ++i)
    {
      fix_register (reg_names[i], 1, 1);
    }

  return true;
}

/* Fixed Reg Option }}} */

/* -fstack-limit-register {{{ */

/* Return TRUE if target supports -fstack-limit-register */
bool
lvx_has_stack_checking (void)
{
#ifdef GCC_LVX_MPPA_COS
  return true;
#else
  return false;
#endif
}

static bool
lvx_handle_stack_limit_symbol_option (const char *arg)
{
  if (lvx_has_stack_checking ())
    {
      /* Only allow $sr as stack-limit register */
      if (strncmp (arg, "__cos_stack_limit", 17))
	error ("only %<__cos_stack_limit%> can be used as stack-limit symbol");
      else
	{
	  /* Deactivate previous OPT_fstack_limit_register_ options.  */
	  opt_fstack_limit_symbol_arg = arg;
	  opt_fstack_limit_register_no = -1;
	}

      return true;
    }
  else
    error ("%<-fstack-limit-*%> is not supported");

  return false;
}

static bool
lvx_handle_stack_limit_register_option (const char *arg)
{
  if (lvx_has_stack_checking ())
    {
      int reg = decode_reg_name (arg);
      if (reg < 0)
	error ("unrecognized register name %qs", arg);
      /* Only allow $sr as stack-limit register */
      else if (strncmp (arg, "sr", 2))
	error ("only $sr can be used as stack-limit register");
      else
	{
	  /* Deactivate previous OPT_fstack_limit_symbol_ options.  */
	  opt_fstack_limit_symbol_arg = NULL;
	  opt_fstack_limit_register_no = reg;
	}

      return true;
    }
  else
    error ("%<-fstack-limit-*%> is not supported");

  return false;
}

/* -fstack-limit-register }}} */

/* Options Handling }}} */

/* Function Entry and Exit {{{ */

/* Information about a function's frame layout.  */
struct GTY (()) lvx_frame_info
{
  /* The total frame size, used for moving $sp in prologue */
  poly_int64 frame_size;

  /* Offsets of save area from frame bottom */
  poly_int64 saved_reg_sp_offset;

  /* Relative offsets within register save area  */
  HOST_WIDE_INT reg_rel_offset[FIRST_PSEUDO_REGISTER];
  /* Register save area size */
  HOST_WIDE_INT saved_regs_size;
  HARD_REG_SET saved_regs;

  /* Offset of virtual frame pointer from new stack pointer/frame bottom */
  poly_int64 virt_frame_pointer_offset;

  /* Offset of hard frame pointer from new stack pointer/frame bottom */
  poly_int64 hard_frame_pointer_offset;

  /* The offset of arg_pointer_rtx from the new stack pointer/frame bottom.  */
  poly_int64 arg_pointer_offset;

  /* Offset to the static chain pointer, if needed */
  poly_int64 static_chain_offset;

  /* Padding size between local area and incoming/varargs */
  HOST_WIDE_INT padding1;

  /* Padding between local area and register save */
  HOST_WIDE_INT padding2;

  /* Padding size between register save and outgoing args */
  HOST_WIDE_INT padding3;

  bool laid_out;
};

struct GTY (()) machine_function
{
  char save_reg[FIRST_PSEUDO_REGISTER];

  lvx_frame_info frame;

  /* If true, the current function has a STATIC_CHAIN slot within its stack
   * frame.  Functions which are nested should have one.  A function which only
   * calls a nested function needs not this slot.  This slot is used to store
   * the value of $r31, which is the registers in which the calling functions
   * store the address to its static chain. */
  int static_chain_needed;

  rtx stack_check_block_label;
  rtx stack_check_block_seq, stack_check_block_last;
};

/*
				~               ~
				|  ..........   |
				|               |    ^
				|               |    |
				| Incomming     |    | Caller frame
				| Args          | <--/ <- incoming $sp [256-bits aligned]
				+---------------+
				| Varargs       |
				|               |
				|               |
				+---------------+
				|               |
				| padding1      |
				|               |
				+---------------+
Argument Pointer / Virt. FP-->  | [Static chain]| [256-bits aligned]
				+---------------+
				| Local         |
				| Variable      |
				|               |
				+---------------+
				|               |
				| padding2      |
				|               |
				+---------------+
				|               |
				| Register      |
				| Save          |
				|               |
				| $ra           | (if frame_pointer_needed)
				| caller FP     | (<- $fp if frame_pointer_needed) [64-bits aligned]
				+---------------+
				|               |
				| padding3      |
				|               |
				+---------------+
				|               |
				| Outgoing      |
				| Args          |
				|               | <- $sp [256-bits aligned]
				+---------------+

*/

enum spill_action
{
  SPILL_COMPUTE_SIZE,
  SPILL_SAVE,
  SPILL_RESTORE
};

static bool should_be_saved_in_prologue (int regno);

/* Return TRUE if REGNO should be saved in the prologue of current function */
static bool
should_be_saved_in_prologue (int regno)
{
  return (df_regs_ever_live_p (regno)	// reg is used
	  && !call_used_regs[regno]	// reg is callee-saved
	  && REGNO_REG_CLASS (regno) != XCR_REGS
	  && (regno == LVX_RETURN_POINTER_REGNO || !fixed_regs[regno]));
}

static bool
lvx_register_saved_on_entry (int regno)
{
  return cfun->machine->frame.reg_rel_offset[regno] >= 0;
}

/* Returns a REG rtx with the nth hard reg that is safe to use in prologue
   (caller-saved and non fixed reg). Returns NULL_RTX and emits an
   error if no such register can be found. */
static rtx
lvx_get_callersaved_nonfixed_reg (machine_mode ARG_UNUSED (mode), int n)
{
  // start at R16 as as everything before that may be used.
  // We should be able to use the veneer regs if not fixed.
  for (int i = 0, regno = 16; regno < FIRST_PSEUDO_REGISTER; regno++)
    {
      bool candidate = call_used_regs[regno] && !fixed_regs[regno]
	&& REGNO_REG_CLASS (regno) != XCR_REGS;
#ifdef GCC_LVX_MPPA_LINUX
      candidate &= !((regno == PROFILE_REGNO) && (crtl->profile));
#endif

      if (!candidate)
	continue;

      if (i == n)
	return gen_rtx_REG (mode, regno);
      else
	i++;
    }

  error ("No scratch register available in function prologue.");
  return NULL_RTX;
}

static void
lvx_emit_single_spill (rtx mem, rtx reg, bool is_load)
{
  rtx set = gen_rtx_SET (is_load ? reg : mem, is_load ? mem : reg);
  rtx insn = emit_insn (set);
  if (!is_load)
    {
      RTX_FRAME_RELATED_P (insn) = 1;
      add_reg_note (insn, REG_CFA_OFFSET, copy_rtx (set));
    }
}

static void
lvx_emit_multiple_spill (rtx mem, rtx reg, int nr, bool is_load)
{
  gcc_assert (nr == 2 || nr == 4);

  rtx operands[4] =
    { is_load ? reg : mem, is_load ? mem : reg, GEN_INT (nr) };

  if (is_load)
    lvx_expand_load_multiple (operands);
  else
    lvx_expand_store_multiple (operands);
  rtx insn = emit_insn (operands[3]);

  if (!is_load)
    {
      RTX_FRAME_RELATED_P (insn) = 1;
      gcc_assert (XVECLEN (PATTERN (insn), 0) == nr);

      for (int i = 0; i < nr; i++)
	{
	  add_reg_note (insn, REG_CFA_OFFSET,
			copy_rtx (XVECEXP (PATTERN (insn), 0, i)));
	}
    }
}

/* Save/Restore register at offsets previously computed in frame information
 * layout. The routine uses $sp as base register, so it must be set to its
 * initial value in case of dynamic stack allocation.
 */
static void
lvx_save_or_restore_callee_save_registers (bool restore)
{
  struct lvx_frame_info *frame = &cfun->machine->frame;
  rtx insn;
  rtx (*gen_mem_ref) (enum machine_mode, rtx) = gen_rtx_MEM;

  unsigned int regno;

  unsigned int pack_prev_regs[4];
  unsigned int pack_prev_regs_idx = 0;

  enum
  {
    NO_PACK_YET = 0,
    DOUBLE_REG,
    QUAD_REG
  } pack_type = NO_PACK_YET;

  hard_reg_set_iterator rsi;

  EXECUTE_IF_SET_IN_HARD_REG_SET (frame->saved_regs, 0, regno, rsi)
  {
    if (lvx_register_saved_on_entry (regno))
      {
	rtx mem = gen_mem_ref (DImode,
			       plus_constant (Pmode, stack_pointer_rtx,
					      frame->saved_reg_sp_offset
					      + frame->reg_rel_offset[regno]));

	rtx saved_reg = gen_rtx_REG (DImode, regno);
	rtx orig_save_reg = saved_reg;

	if (regno == LVX_RETURN_POINTER_REGNO)
	  {
	    saved_reg = lvx_get_callersaved_nonfixed_reg (DImode, 0);
	    gcc_assert (saved_reg != NULL_RTX);

	    if (restore == false)
	      {
		rtx src_reg = gen_rtx_REG (DImode, regno);
		insn = emit_move_insn (saved_reg, src_reg);
		RTX_FRAME_RELATED_P (insn) = 1;

		add_reg_note (insn, REG_CFA_REGISTER,
			      gen_rtx_SET (saved_reg, src_reg));
	      }
	  }

	if (regno == LVX_RETURN_POINTER_REGNO)
	  {
	    if (restore)
	      {
		insn = emit_move_insn (saved_reg, mem);
		insn =
		  emit_move_insn (gen_rtx_REG (DImode, regno), saved_reg);
	      }
	    else
	      {
		insn = emit_move_insn (mem, saved_reg);
		RTX_FRAME_RELATED_P (insn) = 1;
		add_reg_note (insn, REG_CFA_OFFSET,
			      gen_rtx_SET (mem, orig_save_reg));
	      }
	  }
	else
	  {
	  process_current_reg:
	    switch (pack_type)
	      {
	      case NO_PACK_YET:
		if (regno & 1)
		  {
		    lvx_emit_single_spill (mem, saved_reg, restore);
		  }
		else
		  {
		    pack_type = (regno % 4 == 0) ? QUAD_REG : DOUBLE_REG;
		    pack_prev_regs[0] = regno;
		    pack_prev_regs_idx = 1;
		  }
		break;

	      case DOUBLE_REG:
		if (pack_prev_regs[0] == (regno - 1)
		    && frame->reg_rel_offset[pack_prev_regs[0]]
			 == (frame->reg_rel_offset[regno] - UNITS_PER_WORD))
		  {
		    lvx_emit_multiple_spill (
		      gen_mem_ref (
			DImode,
			plus_constant (
			  Pmode, stack_pointer_rtx,
			  frame->saved_reg_sp_offset
			    + frame->reg_rel_offset[pack_prev_regs[0]])),
		      gen_rtx_REG (DImode, pack_prev_regs[0]), 2, restore);
		    pack_type = NO_PACK_YET;
		    pack_prev_regs_idx = 0;
		  }
		else
		  {
		    /* Emit previous candidate */
		    lvx_emit_single_spill (
		      gen_mem_ref (
			DImode,
			plus_constant (
			  Pmode, stack_pointer_rtx,
			  frame->saved_reg_sp_offset
			    + frame->reg_rel_offset[pack_prev_regs[0]])),
		      gen_rtx_REG (DImode, pack_prev_regs[0]), restore);

		    pack_type = NO_PACK_YET;
		    goto process_current_reg;
		  }
		break;

	      case QUAD_REG:
		if (pack_prev_regs[pack_prev_regs_idx - 1] == (regno - 1)
		    && frame->reg_rel_offset[pack_prev_regs[pack_prev_regs_idx
							    - 1]]
			 == (frame->reg_rel_offset[regno] - UNITS_PER_WORD))
		  {
		    if (pack_prev_regs_idx == 3)
		      {
			/* Emit a quad register load/store */
			lvx_emit_multiple_spill (
			  gen_mem_ref (
			    DImode,
			    plus_constant (
			      Pmode, stack_pointer_rtx,
			      frame->saved_reg_sp_offset
				+ frame->reg_rel_offset[pack_prev_regs[0]])),
			  gen_rtx_REG (DImode, pack_prev_regs[0]), 4, restore);
			pack_type = NO_PACK_YET;
			pack_prev_regs_idx = 0;
		      }
		    else
		      {
			pack_prev_regs[pack_prev_regs_idx++] = regno;
		      }
		  }
		else if (pack_prev_regs_idx == 3)
		  {
		    /* Emit a double followed by a single register load/store */
		    lvx_emit_multiple_spill (
		      gen_mem_ref (
			DImode,
			plus_constant (
			  Pmode, stack_pointer_rtx,
			  frame->saved_reg_sp_offset
			    + frame->reg_rel_offset[pack_prev_regs[0]])),
		      gen_rtx_REG (DImode, pack_prev_regs[0]), 2, restore);
		    lvx_emit_single_spill (
		      gen_mem_ref (
			DImode,
			plus_constant (
			  Pmode, stack_pointer_rtx,
			  frame->saved_reg_sp_offset
			    + frame->reg_rel_offset[pack_prev_regs[2]])),
		      gen_rtx_REG (DImode, pack_prev_regs[2]), restore);

		    pack_type = NO_PACK_YET;
		    pack_prev_regs_idx = 0;
		    goto process_current_reg;
		  }
		else if (pack_prev_regs_idx == 2)
		  {
		    /* Emit a double register load/store and try to pack the
		       next one */
		    lvx_emit_multiple_spill (
		      gen_mem_ref (
			DImode,
			plus_constant (
			  Pmode, stack_pointer_rtx,
			  frame->saved_reg_sp_offset
			    + frame->reg_rel_offset[pack_prev_regs[0]])),
		      gen_rtx_REG (DImode, pack_prev_regs[0]), 2, restore);

		    pack_type = NO_PACK_YET;
		    pack_prev_regs_idx = 0;
		    goto process_current_reg;
		  }
		else
		  {
		    /* Emit a single single register load/store and try to pack
		       the next one */
		    lvx_emit_single_spill (
		      gen_mem_ref (
			DImode,
			plus_constant (
			  Pmode, stack_pointer_rtx,
			  frame->saved_reg_sp_offset
			    + frame->reg_rel_offset[pack_prev_regs[0]])),
		      gen_rtx_REG (DImode, pack_prev_regs[0]), restore);

		    pack_type = NO_PACK_YET;
		    pack_prev_regs_idx = 0;
		    goto process_current_reg;
		  }
	      }
	  }
      }
  }

  /* Purge remaining register load/store that could not be packed */
  if (pack_type == DOUBLE_REG)
    {
      lvx_emit_single_spill (
	gen_mem_ref (
	  DImode, plus_constant (Pmode, stack_pointer_rtx,
				 frame->saved_reg_sp_offset
				   + frame->reg_rel_offset[pack_prev_regs[0]])),
	gen_rtx_REG (DImode, pack_prev_regs[0]), restore);
    }
  else if (pack_type == QUAD_REG)
    {
      if (pack_prev_regs_idx == 1)
	{
	  lvx_emit_single_spill (
	    gen_mem_ref (
	      DImode,
	      plus_constant (Pmode, stack_pointer_rtx,
			     frame->saved_reg_sp_offset
			       + frame->reg_rel_offset[pack_prev_regs[0]])),
	    gen_rtx_REG (DImode, pack_prev_regs[0]), restore);
	}
      else if (pack_prev_regs_idx == 2)
	{
	  lvx_emit_multiple_spill (
	    gen_mem_ref (
	      DImode,
	      plus_constant (Pmode, stack_pointer_rtx,
			     frame->saved_reg_sp_offset
			       + frame->reg_rel_offset[pack_prev_regs[0]])),
	    gen_rtx_REG (DImode, pack_prev_regs[0]), 2, restore);
	}
      else
	{
	  lvx_emit_multiple_spill (
	    gen_mem_ref (
	      DImode,
	      plus_constant (Pmode, stack_pointer_rtx,
			     frame->saved_reg_sp_offset
			       + frame->reg_rel_offset[pack_prev_regs[0]])),
	    gen_rtx_REG (DImode, pack_prev_regs[0]), 2, restore);

	  lvx_emit_single_spill (
	    gen_mem_ref (
	      DImode,
	      plus_constant (Pmode, stack_pointer_rtx,
			     frame->saved_reg_sp_offset
			       + frame->reg_rel_offset[pack_prev_regs[2]])),
	    gen_rtx_REG (DImode, pack_prev_regs[2]), restore);
	}
    }
}

static void
lvx_compute_frame_info (void)
{
  struct lvx_frame_info *frame;

  poly_int64 inc_sp_offset = 0;

  if (reload_completed && cfun->machine->frame.laid_out)
    return;

  frame = &cfun->machine->frame;
  /* This is due to the poly_int which are not really PODs, but since we don't
     rely on any special features of poly_int this is fine.  */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
  memset (frame, 0, sizeof (*frame));
#pragma GCC diagnostic pop
  CLEAR_HARD_REG_SET (frame->saved_regs);

  inc_sp_offset += crtl->args.pretend_args_size;

  /* If any anonymous arg may be in register, push them on the stack */
  if (cfun->stdarg && crtl->args.info.next_arg_reg < LVX_ARG_REG_SLOTS)
    inc_sp_offset
      += UNITS_PER_WORD * (LVX_ARG_REG_SLOTS - crtl->args.info.next_arg_reg);

  if (cfun->machine->static_chain_needed)
    inc_sp_offset += UNITS_PER_WORD;

  HOST_WIDE_INT local_vars_sz = get_frame_size ();
  frame->padding1 = 0;

  if (local_vars_sz > 0)
    {
      frame->padding1 =
	ROUND_UP (inc_sp_offset,
		  STACK_BOUNDARY / BITS_PER_UNIT) - inc_sp_offset;
      inc_sp_offset =
	ROUND_UP (inc_sp_offset, STACK_BOUNDARY / BITS_PER_UNIT);

      /* Next are automatic variables. */
      inc_sp_offset += local_vars_sz;
    }
#define SLOT_NOT_REQUIRED (-2)
#define SLOT_REQUIRED (-1)

  frame->padding2 = ROUND_UP (inc_sp_offset, UNITS_PER_WORD) - inc_sp_offset;
  inc_sp_offset = ROUND_UP (inc_sp_offset, UNITS_PER_WORD);

  HOST_WIDE_INT reg_offset = 0;

  /* Mark which register should be saved... */
  for (int regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++)
    if (should_be_saved_in_prologue (regno))
      {
	SET_HARD_REG_BIT (frame->saved_regs, regno);
	cfun->machine->frame.reg_rel_offset[regno] = SLOT_REQUIRED;
      }
    else
      cfun->machine->frame.reg_rel_offset[regno] = SLOT_NOT_REQUIRED;

  if (frame_pointer_needed)
    {
      SET_HARD_REG_BIT (frame->saved_regs, HARD_FRAME_POINTER_REGNUM);
      SET_HARD_REG_BIT (frame->saved_regs, LVX_RETURN_POINTER_REGNO);

      cfun->machine->frame.reg_rel_offset[HARD_FRAME_POINTER_REGNUM] = 0;
      cfun->machine->frame.reg_rel_offset[LVX_RETURN_POINTER_REGNO]
	= UNITS_PER_WORD;
      reg_offset = UNITS_PER_WORD * 2;
    }

  /* ... assign stack slots */
  for (int regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++)
    if (cfun->machine->frame.reg_rel_offset[regno] == SLOT_REQUIRED)
      {
	cfun->machine->frame.reg_rel_offset[regno] = reg_offset;
	reg_offset += UNITS_PER_WORD;
      }

  frame->saved_regs_size = reg_offset;

  inc_sp_offset += reg_offset;

  /* At the bottom of the frame are any outgoing stack arguments. */
  inc_sp_offset += crtl->outgoing_args_size;
  frame->padding3
    = ROUND_UP (inc_sp_offset, STACK_BOUNDARY / BITS_PER_UNIT) - inc_sp_offset;

  inc_sp_offset = ROUND_UP (inc_sp_offset, STACK_BOUNDARY / BITS_PER_UNIT);

  frame->hard_frame_pointer_offset = frame->saved_reg_sp_offset
    = crtl->outgoing_args_size + frame->padding3;

  frame->static_chain_offset = frame->virt_frame_pointer_offset
    = frame->saved_reg_sp_offset + frame->saved_regs_size + frame->padding2
    + get_frame_size ();

  frame->arg_pointer_offset
    = frame->virt_frame_pointer_offset + frame->padding1
    + (cfun->machine->static_chain_needed ? UNITS_PER_WORD : 0);

  frame->frame_size = inc_sp_offset;
  frame->laid_out = true;
}

int
lvx_get_real_frame_size (function *fun)
{
  return reload_completed
    ? fun->machine->frame.frame_size.to_constant ()
    : -1;
}

static void
lvx_debug_frame_info (struct lvx_frame_info *fi)
{
  if (!dump_file)
    return;
  fprintf (dump_file, "\nLVX Frame info:\n");

  fprintf (dump_file,
	   " |XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX| %ld/0 (caller frame) <- $sp "
	   "(incoming) %s \n",
	   fi->frame_size.to_constant (),
	   cfun->stdarg && crtl->args.info.next_arg_reg < LVX_ARG_REG_SLOTS
	     ? ""
	     : "and virt frame pointer");

#define DFI_SEP fprintf (dump_file, " +------------------------------+    \n")

#define DFI_FIELD(f, size, bottom, decorate_up, decorate_down)                 \
  fprintf (dump_file,                                                          \
	   " |                              | %ld/%ld %s  \n"                  \
	   " |%30s|                              \n"                           \
	   " |size: %24ld| %ld/%ld  %s\n",                                     \
	   (bottom) + (size) -UNITS_PER_WORD,                                  \
	   (fi->frame_size.to_constant ())                                     \
	     - ((bottom) + (size) -UNITS_PER_WORD),                            \
	   decorate_up, f, (size), (bottom),                                   \
	   (fi->frame_size.to_constant ()) - (bottom), decorate_down)

  DFI_SEP;
  if (cfun->stdarg && crtl->args.info.next_arg_reg < LVX_ARG_REG_SLOTS)
    {
      DFI_FIELD ("varargs",
		 (fi->frame_size - fi->arg_pointer_offset).to_constant (),
		 fi->arg_pointer_offset.to_constant (), "",
		 " <- arg pointer");
      DFI_SEP;
    }

  if (fi->padding1 > 0)
    {
      DFI_FIELD ("padding1", fi->padding1,
		 fi->virt_frame_pointer_offset.to_constant ()
		 + (cfun->machine->static_chain_needed ? UNITS_PER_WORD : 0),
		 "",
		 cfun->machine->static_chain_needed
		 ? ""
		 : "<- virt frame pointer");
      DFI_SEP;
    }
  if (cfun->machine->static_chain_needed)
    {
      DFI_FIELD ("static chain", (long) UNITS_PER_WORD,
		 fi->virt_frame_pointer_offset.to_constant (), "",
		 "<- virt frame pointer");
      DFI_SEP;
    }

  if (get_frame_size () > 0)
    {
      DFI_FIELD ("locals", get_frame_size ().to_constant (),
		 fi->virt_frame_pointer_offset.to_constant ()
		 - get_frame_size ().to_constant (), "", "");
      DFI_SEP;
    }
  if (fi->padding2 > 0)
    {
      DFI_FIELD ("padding2", fi->padding2,
		 (fi->hard_frame_pointer_offset +
		  fi->saved_regs_size).to_constant (), "", "");
      DFI_SEP;
    }

  if (fi->saved_regs_size > 0)
    {
      unsigned regno;
      hard_reg_set_iterator rsi;
      /* 64 should be already oversized as there are 64 GPRS + possibly $fp and
       * $ra */
      unsigned stacked_regs[64] = { 0 };

      EXECUTE_IF_SET_IN_HARD_REG_SET (fi->saved_regs, 0, regno, rsi)
	stacked_regs[fi->reg_rel_offset[regno] / UNITS_PER_WORD] = regno;

      for (int i = (fi->saved_regs_size / UNITS_PER_WORD) - 1; i != 0; i--)
	fprintf (dump_file, " |%30s| %ld/-\n", reg_names[stacked_regs[i]],
		 fi->hard_frame_pointer_offset.to_constant ()
		 + fi->reg_rel_offset[stacked_regs[i]]);

      fprintf (dump_file, " |%21s (%ld)%4s| %ld/- %s\n", "saved regs",
	       fi->saved_regs_size, reg_names[stacked_regs[0]],
	       fi->hard_frame_pointer_offset.to_constant (),
	       frame_pointer_needed ? "<- hard frame pointer ($fp)" : "");

      DFI_SEP;
    }
  if (fi->padding3 > 0)
    {
      if (crtl->outgoing_args_size > 0)
	{
	  DFI_FIELD ("padding3", fi->padding3,
		     (crtl->outgoing_args_size + fi->padding3).to_constant (),
		     "", "");
	  DFI_SEP;
	}
      else
	{
	  DFI_FIELD ("padding3", fi->padding3, 0L, "", "<- $sp (callee)");
	  DFI_SEP;
	}
    }
  if (crtl->outgoing_args_size > 0)
    {
      DFI_FIELD ("outgoing", crtl->outgoing_args_size.to_constant (), 0L, "",
		 "<- $sp (callee)");
      DFI_SEP;
    }

  fprintf (dump_file, "Saved regs: ");
  unsigned regno;
  hard_reg_set_iterator rsi;
  EXECUTE_IF_SET_IN_HARD_REG_SET (fi->saved_regs, 0, regno, rsi)
    fprintf (dump_file, " $%s [%ld]", reg_names[regno],
	     fi->reg_rel_offset[regno]);

  fprintf (dump_file, "\n");

  fprintf (dump_file, "LVX Frame info valid :%s\n",
	   fi->laid_out ? "yes" : "no");
}

void
lvx_expand_prologue (void)
{
  lvx_compute_frame_info ();
  struct lvx_frame_info *frame = &cfun->machine->frame;
  HOST_WIDE_INT size = frame->frame_size.to_constant ();
  rtx insn;

  if (flag_stack_usage_info)
    current_function_static_stack_size = size;

  lvx_debug_frame_info (frame);

  if (size > 0)
    {
      insn = emit_insn (gen_add2_insn (stack_pointer_rtx, GEN_INT (-size)));
      RTX_FRAME_RELATED_P (insn) = 1;

      add_reg_note (insn, REG_CFA_ADJUST_CFA, copy_rtx (PATTERN (insn)));

      /* We need a static chain (ie, we are generating the prologue of
       * a nested function.), we move the value of $r31 into the
       * STATIC_CHAIN stac frame slot (at $sp + virt_frame_ptr_offset) */
      if (cfun->machine->static_chain_needed)
	{
	  insn = emit_move_insn (gen_frame_mem (Pmode,
						plus_constant (Pmode,
							       stack_pointer_rtx,
							       frame->
							       virt_frame_pointer_offset)),
				 gen_rtx_REG (Pmode, 31));
	}
    }

  /* Save registers */
  lvx_save_or_restore_callee_save_registers (0);

  if (frame_pointer_needed)
    {
      gcc_assert (frame->reg_rel_offset[HARD_FRAME_POINTER_REGNUM] == 0);
      insn =
	emit_insn (gen_add3_insn
		   (hard_frame_pointer_rtx, stack_pointer_rtx,
		    GEN_INT (frame->hard_frame_pointer_offset)));

      RTX_FRAME_RELATED_P (insn) = 1;
      add_reg_note (insn, REG_CFA_DEF_CFA,
		    gen_rtx_PLUS (Pmode, hard_frame_pointer_rtx,
				  GEN_INT (size -
					   frame->
					   hard_frame_pointer_offset)));
    }
}

void
lvx_expand_epilogue (void)
{
  struct lvx_frame_info *frame = &cfun->machine->frame;
  HOST_WIDE_INT frame_size = frame->frame_size;
  rtx insn;

  if (frame_pointer_needed)
    {
      /* Restore $sp from $fp */
      insn =
	emit_insn (gen_add3_insn
		   (stack_pointer_rtx, hard_frame_pointer_rtx,
		    GEN_INT (-frame->hard_frame_pointer_offset)));

      /* Revert CFA reg to use SP with its initial offset */
      RTX_FRAME_RELATED_P (insn) = 1;
      add_reg_note (insn, REG_CFA_DEF_CFA,
		    gen_rtx_PLUS (DImode, stack_pointer_rtx,
				  GEN_INT (frame->frame_size)));
    }

  /* $sp is now correct and can be used by save_or_restore */
  lvx_save_or_restore_callee_save_registers (1);

  if (frame_size != 0)
    {
      insn = GEN_INT (frame_size);
      insn = emit_insn (gen_add2_insn (stack_pointer_rtx, insn));
      RTX_FRAME_RELATED_P (insn) = 1;
      add_reg_note (insn, REG_CFA_ADJUST_CFA, copy_rtx (PATTERN (insn)));
    }
}

/* Function Entry and Exit }}} */

/* scheduling.md {{{ */

int
lvx_branch_tested_bypass_p (rtx_insn *, rtx_insn *cons_insn)
{
  return 0;

  return JUMP_P (cons_insn) && !simplejump_p (cons_insn);
}

int
lvx_stored_value_bypass_p (rtx_insn *prod_insn, rtx_insn *cons_insn)
{
  rtx prod_set = single_set (prod_insn);
  if (!prod_set)
    return 0;

  rtx cons_set = single_set (cons_insn);
  if (!cons_set)
    return 0;

  rtx produced = SET_DEST (prod_set);
  // MEM store source is the stored value.
  rtx consumed = SET_SRC (cons_set);

  // Depends on renaming constraint so only works in SCHED2.
  return reg_overlap_mentioned_p (produced, consumed);
}

int
lvx_accumulator_bypass_p (rtx_insn *prod_insn, rtx_insn *cons_insn)
{
  rtx prod_set = single_set (prod_insn);
  if (!prod_set)
    return 0;

  rtx cons_set = single_set (cons_insn);
  if (!cons_set)
    return 0;

  rtx produced = SET_DEST (prod_set);
  // LVX ternary insns use the same register for acccumulator and result.
  rtx consumed = SET_DEST (cons_set);

  // Depends on renaming constraint so only works in SCHED2.
  return reg_overlap_mentioned_p (produced, consumed);
}

/* scheduling.md }}} */

/* Debug info and Bundle Interactions {{{ */

/* Used during CFA note fixups.  When a FRAME_RELATED_P insn is being
   moved around a CFA-defining insn, its CFA NOTE must be changed
   accordingly to use correct register instead of OLD_BASE.
*/
static void
lvx_swap_fp_sp_in_note (rtx note, rtx old_base)
{
  XEXP (note, 0) = copy_rtx (XEXP (note, 0));
  rtx note_pat = XEXP (note, 0);

  rtx new_base_reg = (REGNO (old_base) == REGNO (hard_frame_pointer_rtx))
		       ? stack_pointer_rtx
		       : hard_frame_pointer_rtx;
  rtx mem_dest = SET_DEST (note_pat);
  struct lvx_frame_info *frame = &cfun->machine->frame;

  if (frame->hard_frame_pointer_offset == 0)
    {
      if (GET_CODE (XEXP (mem_dest, 0)) == PLUS)
	XEXP (XEXP (mem_dest, 0), 0) = new_base_reg;
      else
	XEXP (mem_dest, 0) = new_base_reg;
    }
  else
    {
      HOST_WIDE_INT new_offset
	= (new_base_reg == hard_frame_pointer_rtx)
	    ? -frame->hard_frame_pointer_offset
	    : frame->hard_frame_pointer_offset;
      if (GET_CODE (XEXP (mem_dest, 0)) == PLUS)
	{
	  rtx plus = XEXP (mem_dest, 0);
	  HOST_WIDE_INT old_offset = INTVAL (XEXP (plus, 1));
	  new_offset += old_offset;
	}
      XEXP (mem_dest, 0) = gen_rtx_PLUS (Pmode, new_base_reg,
					 gen_rtx_CONST_INT (Pmode, new_offset));
    }
}

static void
lvx_fix_debug_for_bundle_1 (rtx_insn *start_insn, rtx_insn *stop_insn)
{
  /* Start from the end so that NOTEs will be added in the correct order. */
  rtx_insn *insn = stop_insn;
  bool last_do = false;
  do
    {
      last_do = (insn == start_insn);
      if (INSN_P (insn) && RTX_FRAME_RELATED_P (insn))
	{
	  bool handled = false;
	  for (rtx note = REG_NOTES (insn); note; note = XEXP (note, 1))
	    {
	      switch (REG_NOTE_KIND (note))
		{
		case REG_CFA_DEF_CFA:
		case REG_CFA_ADJUST_CFA:
		case REG_CFA_REGISTER:
		case REG_CFA_RESTORE:
		case REG_CFA_OFFSET:
		  handled = true;
		  if (insn != stop_insn)
		    {
		      /* Move note to last insn in bundle */
		      add_shallow_copy_of_reg_note (stop_insn, note);
		      remove_note (insn, note);
		    }
		  break;

		case REG_CFA_EXPRESSION:
		case REG_CFA_SET_VDRAP:
		case REG_CFA_WINDOW_SAVE:
		case REG_CFA_FLUSH_QUEUE:
		  error ("Unexpected CFA notes found.");
		  break;

		default:
		  break;
		}
	    }

	  if (!handled)
	    {
	      /* This *must* be some mem write emitted by builtin_save_regs,
	       * or a bug */
	      add_reg_note (stop_insn, REG_CFA_OFFSET,
			    copy_rtx (PATTERN (insn)));
	    }

	  RTX_FRAME_RELATED_P (insn) = 0;
	  RTX_FRAME_RELATED_P (stop_insn) = 1;
	}

      insn = PREV_INSN (insn);
    }
  while (!last_do);
}

static unsigned
lvx_fix_debug_for_bundle_2 (rtx_insn *start_insn, rtx_insn *stop_insn,
			    unsigned cur_cfa_reg)
{
  rtx_insn *insn = start_insn;
  bool last_do = false;
  do
    {
      last_do = (insn == stop_insn);
      if (INSN_P (insn) && RTX_FRAME_RELATED_P (insn))
	{
	  for (rtx note = REG_NOTES (insn); note; note = XEXP (note, 1))
	    {

	      switch (REG_NOTE_KIND (note))
		{
		case REG_CFA_DEF_CFA:
		  {
		    rtx pat = XEXP (note, 0);

		    /* (PLUS ( CFA_REG, OFFSET)) */
		    gcc_assert (GET_CODE (pat) == PLUS);
		    cur_cfa_reg = REGNO (XEXP (pat, 0));
		    break;
		  }
		  /* We only need to fixup register spill */
		case REG_CFA_OFFSET:
		  {
		    rtx pat = XEXP (note, 0);

		    gcc_assert (GET_CODE (pat) == SET);

		    rtx mem_dest = SET_DEST (pat);
		    rtx base_reg;
		    if (GET_CODE (XEXP (mem_dest, 0)) == PLUS)
		      {
			base_reg = XEXP (XEXP (mem_dest, 0), 0);
		      }
		    else
		      {
			base_reg = XEXP (mem_dest, 0);
		      }
		    gcc_assert (REG_P (base_reg));

		    if (REGNO (base_reg) != cur_cfa_reg)
		      {
			/* Most likely an insn was moved around and
			   its note has not been modified accordingly.
			   We need to rebuild an offset based on
			   current CFA.  */
			lvx_swap_fp_sp_in_note (note, base_reg);
		      }
		  }
		  break;

		default:
		  break;
		}
	    }
	}
      insn = NEXT_INSN (insn);
    }
  while (!last_do);
  return cur_cfa_reg;
}

/* Debug info and Bundle Interactions }}} */

/* Split big RTX moves {{{ */

/* Split a 128-bit register move into 64-bit moves.  */
void
lvx_split_128bits_move (rtx dst, rtx src)
{
  enum machine_mode mode = GET_MODE (dst);
  rtx dst_0 = simplify_gen_subreg (DImode, dst, mode, 0);
  rtx dst_1 = simplify_gen_subreg (DImode, dst, mode, 8);
  rtx src_0 = simplify_gen_subreg (DImode, src, mode, 0);
  rtx src_1 = simplify_gen_subreg (DImode, src, mode, 8);
  if (!reg_overlap_mentioned_p (dst, src)
      || reg_or_subregno (dst) < reg_or_subregno (src))
    {
      emit_insn (gen_movdi (dst_0, src_0));
      emit_insn (gen_movdi (dst_1, src_1));
    }
  else
    {
      emit_insn (gen_movdi (dst_1, src_1));
      emit_insn (gen_movdi (dst_0, src_0));
    }
}

/* Split a move of an extension value the instruction set cannot do in one go.
   The extension file moves 256 bits at a time -- xlo, xso and xcopyo -- so a
   wider move is a run over V1OI chunks.  A register-to-register move goes
   through UNSPEC_XCOPY; anything touching memory is a plain set, which the
   256-bit *mov pattern then turns into xlo or xso.

   A port whose ISA has wider extension copies picks a bigger chunk for the
   register case (the V2OI and V4OI variants).  LVX removed xcopyx and xcopyv, so
   HAVE_LVX_EXT_COPY_V1OI is the only one left and V1OI is the only chunk.  */

void
lvx_split_tca_moves (rtx dst, rtx src)
{
  gcc_assert (GET_MODE (dst) == GET_MODE (src));

  machine_mode mode = GET_MODE (dst);
  bool tca_to_tca = extension_register_operand (src, VOIDmode)
		    && extension_register_operand (dst, VOIDmode);
  int chunk_size = GET_MODE_SIZE (V1OImode);

  for (int offset = 0; offset < GET_MODE_SIZE (mode); offset += chunk_size)
    {
      rtx op0 = simplify_gen_subreg (V1OImode, dst, mode, offset);
      rtx op1 = simplify_gen_subreg (V1OImode, src, mode, offset);
      gcc_assert (op0 && op1);

      if (tca_to_tca)
	{
	  rtx modifier = gen_rtx_CONST_STRING (VOIDmode, "");
	  rtx xcopy = gen_rtx_UNSPEC (V1OImode, gen_rtvec (2, op1, modifier),
				      UNSPEC_XCOPY);
	  emit_insn (gen_rtx_SET (op0, xcopy));
	}
      else
	emit_insn (gen_rtx_SET (op0, op1));
    }
}

/* Split a 256-bit register move into 64-bit moves.  */
void
lvx_split_256bits_move (rtx dst, rtx src)
{
  enum machine_mode mode = GET_MODE (dst);
  rtx dst_0 = simplify_gen_subreg (DImode, dst, mode, 0);
  rtx dst_1 = simplify_gen_subreg (DImode, dst, mode, 8);
  rtx dst_2 = simplify_gen_subreg (DImode, dst, mode, 16);
  rtx dst_3 = simplify_gen_subreg (DImode, dst, mode, 24);
  rtx src_0 = simplify_gen_subreg (DImode, src, mode, 0);
  rtx src_1 = simplify_gen_subreg (DImode, src, mode, 8);
  rtx src_2 = simplify_gen_subreg (DImode, src, mode, 16);
  rtx src_3 = simplify_gen_subreg (DImode, src, mode, 24);
  if (!reg_overlap_mentioned_p (dst, src)
      || reg_or_subregno (dst) < reg_or_subregno (src))
    {
      emit_insn (gen_movdi (dst_0, src_0));
      emit_insn (gen_movdi (dst_1, src_1));
      emit_insn (gen_movdi (dst_2, src_2));
      emit_insn (gen_movdi (dst_3, src_3));
    }
  else
    {
      emit_insn (gen_movdi (dst_3, src_3));
      emit_insn (gen_movdi (dst_2, src_2));
      emit_insn (gen_movdi (dst_1, src_1));
      emit_insn (gen_movdi (dst_0, src_0));
    }
}

/* Split big RTX moves }}} */

/* Build big RTX constants {{{ */

/* Write VALUE to both double words of the 128-bit DST, which is a register of
   some 128-bit mode.  One MAKEDP where the ISA has it and the pair is aligned,
   two MAKEDs otherwise -- the fallback is what every 128-bit constant used
   before MAKEDP existed, so lvx-1 and a misaligned pair keep exactly the code
   they had.  */
static void
lvx_emit_64bit_halves (rtx dst, HOST_WIDE_INT value)
{
  machine_mode mode = GET_MODE (dst);
  rtx pair = simplify_gen_subreg (V2DImode, dst, mode, 0);

  if (LVX_2 && pair && lvx_hardreg_aligned_p (pair, V2DImode))
    {
      emit_insn (gen_lvx_makedp (pair, GEN_INT (value)));
      return;
    }

  emit_insn (gen_movdi (simplify_gen_subreg (DImode, dst, mode, 0),
                        GEN_INT (value)));
  emit_insn (gen_movdi (simplify_gen_subreg (DImode, dst, mode, 8),
                        GEN_INT (value)));
}

/* Make a 128bit constant from SRC into DST in DI chunks.  */
void
lvx_make_128bit_const (rtx dst, rtx src)
{
  int nunits = 0;
  HOST_WIDE_INT value_0 = 0, value_1 = 0;

  switch (GET_CODE (src))
    {
    case CONST_INT:
      value_0 = INTVAL (src);
      value_1 = value_0 < 0 ? HOST_WIDE_INT_M1 : HOST_WIDE_INT_0;
      break;
    case CONST_DOUBLE:
      value_0 = HOST_WIDE_INT_0;
      value_1 = HOST_WIDE_INT_0;
      break;
    case CONST_WIDE_INT:
      nunits = CONST_WIDE_INT_NUNITS (src);
      if (nunits > 0)
	value_0 = CONST_WIDE_INT_ELT (src, 0);
      if (nunits > 1)
	value_1 = CONST_WIDE_INT_ELT (src, 1);
      else
	value_1 = value_0 < 0 ? HOST_WIDE_INT_M1 : HOST_WIDE_INT_0;
      break;
    case CONST_VECTOR:
      value_0 = lvx_const_vector_value (src, 0);
      value_1 = lvx_const_vector_value (src, 1);
      break;
    default:
      gcc_unreachable ();
    }

  enum machine_mode mode = GET_MODE (dst);

  /* Equal halves are one MAKEDP: the all-zero and all-ones vectors, and every
     constant uniform at 32-bit lanes or narrower, land here.  */
  if (value_0 == value_1)
    {
      lvx_emit_64bit_halves (dst, value_0);
      return;
    }

  rtx dst_0 = simplify_gen_subreg (DImode, dst, mode, 0);
  rtx dst_1 = simplify_gen_subreg (DImode, dst, mode, 8);

  emit_insn (gen_movdi (dst_0, GEN_INT (value_0)));
  emit_insn (gen_movdi (dst_1, GEN_INT (value_1)));
}

/* Make a 256bit constant from SRC into DST in DI chunks.  */
void
lvx_make_256bit_const (rtx dst, rtx src)
{
  int nunits = 0;
  HOST_WIDE_INT value_0 = 0, value_1 = 0, value_2 = 0, value_3 = 0;

  switch (GET_CODE (src))
    {
    case CONST_INT:
      value_0 = INTVAL (src);
      value_1 = value_0 < 0 ? HOST_WIDE_INT_M1 : HOST_WIDE_INT_0;
      value_2 = value_1 < 0 ? HOST_WIDE_INT_M1 : HOST_WIDE_INT_0;
      value_3 = value_2 < 0 ? HOST_WIDE_INT_M1 : HOST_WIDE_INT_0;
      break;
    case CONST_WIDE_INT:
      nunits = CONST_WIDE_INT_NUNITS (src);
      if (nunits > 0)
	value_0 = CONST_WIDE_INT_ELT (src, 0);
      if (nunits > 1)
	value_1 = CONST_WIDE_INT_ELT (src, 1);
      else
	value_1 = value_0 < 0 ? HOST_WIDE_INT_M1 : HOST_WIDE_INT_0;
      if (nunits > 2)
	value_2 = CONST_WIDE_INT_ELT (src, 2);
      else
	value_2 = value_1 < 0 ? HOST_WIDE_INT_M1 : HOST_WIDE_INT_0;
      if (nunits > 3)
	value_3 = CONST_WIDE_INT_ELT (src, 3);
      else
	value_3 = value_2 < 0 ? HOST_WIDE_INT_M1 : HOST_WIDE_INT_0;
      break;
    case CONST_VECTOR:
      value_0 = lvx_const_vector_value (src, 0);
      value_1 = lvx_const_vector_value (src, 1);
      value_2 = lvx_const_vector_value (src, 2);
      value_3 = lvx_const_vector_value (src, 3);
      break;
    default:
      gcc_unreachable ();
    }

  enum machine_mode mode = GET_MODE (dst);

  /* A 256-bit quad is two pairs, and MAKEDP writes one pair, so the two halves
     are decided separately: an all-zero or all-ones quad is two MAKEDPs, and a
     quad uniform in one half only still saves the MAKED there.  */
  for (int half = 0; half < 2; half++)
    {
      HOST_WIDE_INT lo = half ? value_2 : value_0;
      HOST_WIDE_INT hi = half ? value_3 : value_1;
      unsigned off = half * 16;

      if (lo == hi)
        {
          rtx pair = simplify_gen_subreg (V2DImode, dst, mode, off);
          if (pair)
            {
              lvx_emit_64bit_halves (pair, lo);
              continue;
            }
        }

      emit_insn (gen_movdi (simplify_gen_subreg (DImode, dst, mode, off),
                            GEN_INT (lo)));
      emit_insn (gen_movdi (simplify_gen_subreg (DImode, dst, mode, off + 8),
                            GEN_INT (hi)));
    }
}

/* Make a 512bit constant from SRC into DST in DI chunks.  */
void
lvx_make_512bit_const (rtx dst, rtx src)
{
  gcc_assert (GET_CODE (src) == CONST_VECTOR);

  HOST_WIDE_INT value_0 = lvx_const_vector_value (src, 0);
  HOST_WIDE_INT value_1 = lvx_const_vector_value (src, 1);
  HOST_WIDE_INT value_2 = lvx_const_vector_value (src, 2);
  HOST_WIDE_INT value_3 = lvx_const_vector_value (src, 3);
  HOST_WIDE_INT value_4 = lvx_const_vector_value (src, 4);
  HOST_WIDE_INT value_5 = lvx_const_vector_value (src, 5);
  HOST_WIDE_INT value_6 = lvx_const_vector_value (src, 6);
  HOST_WIDE_INT value_7 = lvx_const_vector_value (src, 7);

  enum machine_mode mode = GET_MODE (dst);
  rtx dst_0 = simplify_gen_subreg (DImode, dst, mode, 0);
  rtx dst_1 = simplify_gen_subreg (DImode, dst, mode, 8);
  rtx dst_2 = simplify_gen_subreg (DImode, dst, mode, 16);
  rtx dst_3 = simplify_gen_subreg (DImode, dst, mode, 24);
  rtx dst_4 = simplify_gen_subreg (DImode, dst, mode, 32);
  rtx dst_5 = simplify_gen_subreg (DImode, dst, mode, 40);
  rtx dst_6 = simplify_gen_subreg (DImode, dst, mode, 48);
  rtx dst_7 = simplify_gen_subreg (DImode, dst, mode, 56);

  emit_insn (gen_movdi (dst_0, GEN_INT (value_0)));
  emit_insn (gen_movdi (dst_1, GEN_INT (value_1)));
  emit_insn (gen_movdi (dst_2, GEN_INT (value_2)));
  emit_insn (gen_movdi (dst_3, GEN_INT (value_3)));
  emit_insn (gen_movdi (dst_4, GEN_INT (value_4)));
  emit_insn (gen_movdi (dst_5, GEN_INT (value_5)));
  emit_insn (gen_movdi (dst_6, GEN_INT (value_6)));
  emit_insn (gen_movdi (dst_7, GEN_INT (value_7)));
}

rtx_insn *
lvx_emit_insn_slices (enum rtx_code code, machine_mode mode,
		      rtx dest, rtx src1, rtx src2)
{
  unsigned mode_size = GET_MODE_SIZE (mode);
  unsigned slice_mode_size = UNITS_PER_WORD * 4;

  if (mode_size <= slice_mode_size)
    return emit_insn (gen_rtx_SET (dest,
				   gen_rtx_fmt_ee (code, mode, src1, src2)));

  scalar_mode inner_mode = GET_MODE_INNER (mode);
  unsigned nunits = slice_mode_size / GET_MODE_SIZE (inner_mode);
  machine_mode slice_mode = mode_for_vector (inner_mode, nunits).require ();

  rtx_insn *insn  = 0;
  gcc_assert (GET_MODE_SIZE (slice_mode) == slice_mode_size);
  for (unsigned offset = 0; offset < mode_size; offset += slice_mode_size)
    {
      rtx slice_dest = simplify_gen_subreg (slice_mode, dest, mode, offset);
      rtx slice_src1 = simplify_gen_subreg (slice_mode, src1, mode, offset);
      rtx slice_src2 = simplify_gen_subreg (slice_mode, src2, mode, offset);
      insn = emit_insn (gen_rtx_SET (slice_dest,
				     gen_rtx_fmt_ee (code, slice_mode,
						     slice_src1, slice_src2)));
    }
  return insn;
}

/* Build big RTX constants }}} */

/* Expanders {{{ */

void
lvx_expand_tablejump (rtx op0, rtx op1)
{
  if (flag_pic)
    {
      rtx tmp_reg = gen_reg_rtx (Pmode);
      emit_move_insn (tmp_reg, gen_rtx_LABEL_REF (VOIDmode, op1));

      /* Relative addrs are stored as 32bits value. */
      if (GET_MODE (op0) != Pmode)
	{
	  rtx dreg = gen_reg_rtx (Pmode);
	  emit_insn (gen_extend_insn (dreg, op0, Pmode, GET_MODE (op0), 0));
	  op0 = dreg;
	}
      emit_insn (gen_add2_insn (op0, tmp_reg));
    }

  emit_jump_insn (gen_tablejump_real_di (op0, op1));
}


/* Lower a comparison between TI registers into a DI register.  */
static void
lvx_lower_timode_comparison (rtx pred, rtx comp, machine_mode comp_mode)
{
  rtx left = XEXP (comp, 0);
  rtx right = XEXP (comp, 1);
  gcc_assert (comp_mode == TImode);
  gcc_assert (GET_MODE (pred) == DImode);
  enum rtx_code comp_code = GET_CODE (comp);
  rtx left_lo = simplify_gen_subreg (DImode, left, TImode, 0);
  rtx left_hi = simplify_gen_subreg (DImode, left, TImode, UNITS_PER_WORD);
  rtx right_lo = simplify_gen_subreg (DImode, right, TImode, 0);
  rtx right_hi = simplify_gen_subreg (DImode, right, TImode, UNITS_PER_WORD);
  enum rtx_code unsign_code = unsigned_condition (comp_code);
  enum rtx_code strict_code = comp_code;

  switch (comp_code)
    {
    case NE:
    case EQ:
      {
	rtx lo_cmp = gen_reg_rtx (DImode);
	emit_insn (gen_cstoredi4 (lo_cmp, comp, left_lo, right_lo));
	rtx hi_cmp = gen_reg_rtx (DImode);
	emit_insn (gen_cstoredi4 (hi_cmp, comp, left_hi, right_hi));
	if (comp_code == NE)
	  emit_insn (gen_iordi3 (pred, lo_cmp, hi_cmp));
	if (comp_code == EQ)
	  emit_insn (gen_anddi3 (pred, lo_cmp, hi_cmp));
	return;
      }
    case GT:
    case LT:
    case GTU:
    case LTU:
      break;
    case GE:
      strict_code = GT;
      break;
    case LE:
      strict_code = LT;
      break;
    case GEU:
      strict_code = GTU;
      break;
    case LEU:
      strict_code = LTU;
      break;
    default:
      gcc_unreachable ();
    }

  rtx unsign_comp = gen_rtx_fmt_ee (unsign_code, DImode, left_lo, right_lo);
  rtx strict_comp = gen_rtx_fmt_ee (strict_code, DImode, left_hi, right_hi);
  rtx equal_comp = gen_rtx_fmt_ee (EQ, DImode, left_hi, right_hi);

  emit_insn (gen_cstoredi4 (pred, strict_comp, left_hi, right_hi));
  rtx lo_ucmp = gen_reg_rtx (DImode);
  emit_insn (gen_cstoredi4 (lo_ucmp, unsign_comp, left_lo, right_lo));
  rtx hi_equal = gen_reg_rtx (DImode);
  emit_insn (gen_cstoredi4 (hi_equal, equal_comp, left_hi, right_hi));

  rtx eq = gen_rtx_NE (VOIDmode, hi_equal, const0_rtx);
  rtx select = gen_rtx_IF_THEN_ELSE (DImode, eq, lo_ucmp, pred);
  emit_insn (gen_rtx_SET (pred, select));
}


/* Wrap a vector comparison C in UNSPEC_PACKCMP when PACK -- i.e. when the result
   is a bit-per-lane mask in a GPR (a scalar int mode), not a full-width 0/-1
   vector.  A bare scalar-mode comparison has nonzero_bits 1, so combine would
   mistake the packed mask for a 0/1 boolean; the unspec keeps its lane bits.  */
static rtx
lvx_pack_cmp (rtx c, machine_mode mask_mode, bool pack)
{
  return (pack && COMPARISON_P (c))
	 ? gen_rtx_UNSPEC (mask_mode, gen_rtvec (1, c), UNSPEC_PACKCMP) : c;
}

/* A 256-bit comparison has no COMP*, FCOMP*: the ISA compares 128 bits at a
   time.  Compare the two 128-bit halves separately -- each COMP*, FCOMP* packs
   nunits/2 lane bits into a GPR -- then stitch the two masks into one with a
   single INSFD, the high half's bits inserted just above the low half's.  The
   combined GPR bit-mask is what a masked 256-bit lo/so consumes.  */
static void
lvx_lower_256_comparison (rtx pred, rtx comp, machine_mode comp_mode)
{
  enum rtx_code code = GET_CODE (comp);
  rtx left  = force_reg (comp_mode, XEXP (comp, 0));
  rtx right = force_reg (comp_mode, XEXP (comp, 1));

  unsigned nunits = GET_MODE_NUNITS (comp_mode);
  unsigned half = nunits / 2;
  scalar_mode inner = GET_MODE_INNER (comp_mode);
  machine_mode half_mode = mode_for_vector (inner, half).require ();
  machine_mode half_pred = int_mode_for_size (half < 8 ? 8 : half, 0).require ();

  rtx masks[2];
  for (int h = 0; h < 2; h++)
    {
      /* The two 128-bit halves live at byte offsets 0 and 16.  */
      rtx l = simplify_gen_subreg (half_mode, left, comp_mode, h * 16);
      rtx r = simplify_gen_subreg (half_mode, right, comp_mode, h * 16);
      masks[h] = gen_reg_rtx (half_pred);
      lvx_lower_comparison (masks[h],
			    gen_rtx_fmt_ee (code, half_pred, l, r), half_mode);
    }

  /* pred = masks[0] | (masks[1] << half), the whole of which is one INSFD once
     the low half already sits in the accumulator.  */
  rtx acc = gen_reg_rtx (SImode);
  emit_move_insn (acc, convert_to_mode (SImode, masks[0], 1));
  rtx hi = convert_to_mode (SImode, masks[1], 1);
  emit_insn (gen_insvsi (acc, GEN_INT (half), GEN_INT (half), hi));
  emit_move_insn (pred, gen_lowpart (GET_MODE (pred), acc));
}

/* Blend for a 256-bit select: DST = MASK ? OP1 : OP2, where MASK is the packed
   GPR lane mask lvx_lower_256_comparison produced.  BLEND* only reaches 128
   bits, so blend each half on its own -- the low half reads the mask's low
   bits, the high half the bits above them, brought down by nunits/2.  */
void
lvx_expand_256_blend (rtx dst, rtx op1, rtx op2, rtx mask)
{
  machine_mode mode = GET_MODE (dst);
  unsigned nunits = GET_MODE_NUNITS (mode);
  unsigned half = nunits / 2;
  scalar_mode inner = GET_MODE_INNER (mode);
  machine_mode half_mode = mode_for_vector (inner, half).require ();
  machine_mode half_mask = int_mode_for_size (half < 8 ? 8 : half, 0).require ();

  for (int h = 0; h < 2; h++)
    {
      rtx d = simplify_gen_subreg (half_mode, dst, mode, h * 16);
      rtx t = simplify_gen_subreg (half_mode, op1, mode, h * 16);
      rtx f = simplify_gen_subreg (half_mode, op2, mode, h * 16);
      rtx m = mask;
      if (h == 1)
	m = expand_shift (RSHIFT_EXPR, GET_MODE (mask), mask, half, NULL_RTX, 1);
      m = gen_lowpart (half_mask, m);

      /* BLEND* is dst = mask ? src : dst; seed dst with the false value.  */
      if (!rtx_equal_p (d, f))
	emit_move_insn (d, force_reg (half_mode, f));
      emit_insn (gen_rtx_SET
		 (d, gen_rtx_UNSPEC (half_mode, gen_rtvec (3, d, t, m),
				     UNSPEC_LVX_BLEND)));
    }
}


/* The comparison each packed lane mask came from, keyed by the mask pseudo.

   COMP* reduces a vector comparison to one bit per lane in a GPR, and by the
   time vcond_mask is expanded that GPR is all it is handed -- the comparison
   itself is gone.  But `mask ? -1 : 0' IS a COMPN*, one instruction, so the
   comparison is worth keeping: the integer vec_cmp expander records it here and
   lvx_expand_vcond_mask reads it back.

   Walking the insn chain instead does not work, which is worth recording
   because it looks like it should: genemit wraps every define_expand body in
   start_sequence (see gen_vcond_mask_v16qihi in insn-emit), so inside an
   expander the chain is a fresh empty one and get_last_insn () is NULL.

   Keyed by REGNO and that is safe: expand gives each SSA name its own pseudo
   and does not recycle pseudos within a function, so a REGNO names one value.
   lvx_init_expanders empties the map per function in any case, so nothing can
   leak across one.  The entry is a hint, never an obligation -- a miss just
   takes the BLENDZ* path.  */
static hash_map<int_hash<unsigned int, 0, UINT_MAX>, rtx> *lvx_packcmp_map;

void
lvx_record_packcmp (rtx mask, rtx cmp)
{
  if (!REG_P (mask) || cmp == NULL_RTX || !COMPARISON_P (cmp))
    return;
  if (!lvx_packcmp_map)
    lvx_packcmp_map
      = new hash_map<int_hash<unsigned int, 0, UINT_MAX>, rtx> (16);
  lvx_packcmp_map->put (REGNO (mask), cmp);
}

/* The comparison recorded for MASK, or NULL_RTX.  Only returned when it still
   describes a usable COMPN* (FCOMPN* for floats): both operands registers of a
   vector with MODE's lane count.

   The operands' mode is NOT required to be MODE, and that is the float case
   rather than laxity.  A float comparison yields an INTEGER 0/-1 vector -- C's
   `a < b' on v4sf has type v4si -- so `*fcompn<suffix>' matches
   (set (reg:V4SI) (lt:V4SI (reg:V4SF) (reg:V4SF))), with the destination and
   the comparison in the integer sibling and only the operands in float.
   Requiring operand mode == MODE is what kept FCOMPN* unreachable.  */
static rtx
lvx_recorded_packcmp (rtx mask, machine_mode mode)
{
  if (!REG_P (mask) || !lvx_packcmp_map)
    return NULL_RTX;
  rtx *slot = lvx_packcmp_map->get (REGNO (mask));
  if (!slot)
    return NULL_RTX;
  rtx cmp = *slot;
  if (!COMPARISON_P (cmp)
      || !REG_P (XEXP (cmp, 0)) || !REG_P (XEXP (cmp, 1)))
    return NULL_RTX;
  machine_mode om = GET_MODE (XEXP (cmp, 0));
  if (om != GET_MODE (XEXP (cmp, 1))
      || !VECTOR_MODE_P (om)
      || GET_MODE_SIZE (om) != 16
      || GET_MODE_NUNITS (om) != GET_MODE_NUNITS (mode))
    return NULL_RTX;
  return cmp;
}

/* vcond_mask: DST = MASK ? T : F, for the 128- and 256-bit lane shapes.

   Three forms, cheapest first, and the two constant cases are worth special
   casing because the vectorizer asks for them constantly -- `(vNqi)(a == b)'
   reaches expand as .VCOND_MASK (mask, {-1...}, {0...}), which is every C
   vector comparison used as a value.

   F == 0 is BLENDZ*, which does not read its destination.  Through BLEND*
   it needs the destination seeded with the zero vector first, and that seeding
   is a real copy in the loop body (two COPYD for a 128-bit value), on top of
   materialising the zero.

   Otherwise BLEND* over a destination seeded with F, as before.

   F == 0 and T == -1 is one COMPN* on its own -- that instruction writes 0/-1
   per lane, which is exactly this select -- and this is what OpenCL-C's and C's
   vector `a == b' means, so it is the shape every value-producing comparison
   takes.  Reaching it needs the comparison, which lvx_packcmp_map carries over
   from the vec_cmp expander; see there for why the insn chain cannot be walked
   instead, and why gimple-isel cannot fold it for us (its `c ? -1 : 0' -> `c'
   wants the mask's mode to equal the result's, and a packed lane mask is 16 bits
   against the vector's 128).  */
void
lvx_expand_vcond_mask (rtx dst, rtx t, rtx f, rtx mask)
{
  machine_mode mode = GET_MODE (dst);

  /* rtx_equal_p, not pointer equality against CONST0_RTX/CONSTM1_RTX: a
     CONST_VECTOR is not interned the way a CONST_INT is, and
     const_vector_from_tree returns the canonical rtx for an all-zero vector
     only -- `if (initializer_zerop (exp)) return CONST0_RTX (mode);'.  An
     all-ones arm therefore arrives as a fresh CONST_VECTOR and a pointer test
     silently misses it, which reads as the cheaper path simply not applying.  */
  bool f_is_zero = rtx_equal_p (f, CONST0_RTX (mode));
  bool t_is_mone = rtx_equal_p (t, CONSTM1_RTX (mode));

  if (f_is_zero)
    {
      if (t_is_mone
	  && GET_MODE_CLASS (mode) == MODE_VECTOR_INT
	  && GET_MODE_SIZE (mode) == 16)
	{
	  rtx cmp = lvx_recorded_packcmp (mask, mode);
	  if (cmp != NULL_RTX)
	    {
	      /* Emit and verify rather than trust: `*compn<suffix>' covers
		 V128N under LVX_2, and a condition it cannot spell must become
		 the BLENDZ* path, not unrecognizable RTL at the end of
		 expand.  */
	      rtx_insn *insn
		= emit_insn (gen_rtx_SET (dst,
					  gen_rtx_fmt_ee (GET_CODE (cmp), mode,
							  XEXP (cmp, 0),
							  XEXP (cmp, 1))));
	      if (recog_memoized (insn) >= 0)
		return;
	      delete_insn (insn);
	    }
	}
      lvx_expand_blendz (dst, t, mask);
      return;
    }

  /* BLEND* is a type-agnostic lane bit-select, so a float select is the integer
     one over the same bits -- view it that way once, here, rather than in each
     width's arm.  */
  machine_mode bmode = mode;
  rtx d = dst, ta = force_reg (mode, t), fa = force_reg (mode, f);
  if (GET_MODE_CLASS (mode) == MODE_VECTOR_FLOAT)
    {
      scalar_int_mode iinner
	= int_mode_for_size (GET_MODE_UNIT_BITSIZE (mode), 0).require ();
      bmode = mode_for_vector (iinner, GET_MODE_NUNITS (mode)).require ();
      d  = simplify_gen_subreg (bmode, dst, mode, 0);
      ta = simplify_gen_subreg (bmode, ta, mode, 0);
      fa = simplify_gen_subreg (bmode, fa, mode, 0);
      gcc_assert (d && ta && fa);
    }

  if (GET_MODE_SIZE (bmode) == 32)
    {
      lvx_expand_256_blend (d, ta, fa, mask);
      return;
    }

  if (!rtx_equal_p (d, fa))
    emit_move_insn (d, fa);
  emit_insn (gen_rtx_SET (d,
			  gen_rtx_UNSPEC (bmode, gen_rtvec (3, d, ta, mask),
					  UNSPEC_LVX_BLEND)));
}

/* DST = MASK ? SRC : 0, lane-wise, with BLENDZ*.

   This is the zeroing select, and the reason it is worth an instruction of its
   own rather than a BLEND* against a zero register: BLEND* reads its
   destination to supply the lanes the mask leaves out, so building `mask ? src
   : 0' with it means seeding a register with zero first -- and GCC
   rematerialises that constant inside the loop (four MAKED per iteration at
   256 bits) rather than keeping a live zero register.  BLENDZ* does not read
   its destination, so the constant never exists.  Measured on
   validation/bench/condadd.c: the BLEND route cost 386 cycles on a 64-element
   f64 sum against 219 unmasked, and the four MAKED are most of the gap.

   MODE may be 128- or 256-bit and integer or float.  BLENDZ* is a lane
   bit-select on the register bits and type-agnostic, so a float vector is
   viewed as its integer sibling of the same lane width; a 256-bit vector is
   two 128-bit halves, each with its own slice of the lane mask, exactly as
   lvx_expand_256_blend splits one -- minus the seeding move, which is the
   point.  */
void
lvx_expand_blendz (rtx dst, rtx src, rtx mask)
{
  machine_mode mode = GET_MODE (dst);
  unsigned nunits = GET_MODE_NUNITS (mode);

  if (GET_MODE_CLASS (mode) == MODE_VECTOR_FLOAT)
    {
      scalar_int_mode iinner
	= int_mode_for_size (GET_MODE_UNIT_BITSIZE (mode), 0).require ();
      machine_mode imode = mode_for_vector (iinner, nunits).require ();
      rtx d = simplify_gen_subreg (imode, dst, mode, 0);
      rtx s = simplify_gen_subreg (imode, src, mode, 0);
      gcc_assert (d && s);
      dst = d;
      src = s;
      mode = imode;
    }
  src = force_reg (mode, src);

  if (GET_MODE_SIZE (mode) == 16)
    {
      emit_insn (gen_rtx_SET (dst,
			      gen_rtx_UNSPEC (mode, gen_rtvec (2, src, mask),
					      UNSPEC_LVX_BLENDZ)));
      return;
    }

  unsigned half = nunits / 2;
  machine_mode half_mode
    = mode_for_vector (GET_MODE_INNER (mode), half).require ();
  machine_mode half_mask
    = int_mode_for_size (half < 8 ? 8 : half, 0).require ();

  for (int h = 0; h < 2; h++)
    {
      rtx d = simplify_gen_subreg (half_mode, dst, mode, h * 16);
      rtx s = simplify_gen_subreg (half_mode, src, mode, h * 16);
      gcc_assert (d && s);
      rtx m = mask;
      if (h == 1)
	m = expand_shift (RSHIFT_EXPR, GET_MODE (mask), mask, half, NULL_RTX, 1);
      m = force_reg (half_mask, gen_lowpart (half_mask, m));
      emit_insn (gen_rtx_SET (d,
			      gen_rtx_UNSPEC (half_mode, gen_rtvec (2, s, m),
					      UNSPEC_LVX_BLENDZ)));
    }
}

/* Lower a comparison COMP between CMP_MODE rtx(es) into a predicate register PRED.
 * In case of floating-point lowering, the left and right operands may be swapped.  */
void
lvx_lower_comparison (rtx pred, rtx comp, machine_mode comp_mode)
{
  if (VECTOR_MODE_P (comp_mode) && GET_MODE_SIZE (comp_mode) == 32
      && SCALAR_INT_MODE_P (GET_MODE (pred)))
    {
      lvx_lower_256_comparison (pred, comp, comp_mode);
      return;
    }

  rtx left = XEXP (comp, 0);
  rtx right = XEXP (comp, 1);
  enum rtx_code comp_code = GET_CODE (comp);
  enum mode_class comp_class = GET_MODE_CLASS (comp_mode);
  bool float_compare_p
    = (comp_class == MODE_FLOAT || comp_class == MODE_VECTOR_FLOAT);

  if (comp_mode == TImode)
    {
      lvx_lower_timode_comparison (pred, comp, comp_mode);
      return;
    }

  machine_mode pred_mode = GET_MODE (pred);
  bool pack = SCALAR_INT_MODE_P (pred_mode) && VECTOR_MODE_P (comp_mode);
  rtx cmp =
    gen_rtx_fmt_ee (comp_code, pred_mode, force_reg (comp_mode, left),
		    force_reg (comp_mode, right));

  if (float_compare_p)
    {
      if (!float_comparison_operator (cmp, VOIDmode))
	{
	  enum rtx_code swapped = swap_condition (comp_code);

	  if (swapped == comp_code)
	    {
	      rtx tmp = gen_reg_rtx (pred_mode);
	      rtx tmp2 = gen_reg_rtx (pred_mode);
	      rtx cmp2 = copy_rtx (cmp);
	      enum rtx_code join_code;

	      if (comp_code == UNORDERED)
		{
		  PUT_CODE (cmp, UNGE);
		  PUT_CODE (cmp2, UNLT);
		  join_code = AND;
		}
	      else if (comp_code == ORDERED)
		{
		  PUT_CODE (cmp, GE);
		  PUT_CODE (cmp2, LT);
		  join_code = IOR;
		}
	      else
		{
		  gcc_unreachable ();
		}

	      emit_insn (gen_rtx_SET (tmp, lvx_pack_cmp (cmp, pred_mode, pack)));
	      emit_insn (gen_rtx_SET (tmp2, lvx_pack_cmp (cmp2, pred_mode, pack)));
	      cmp = gen_rtx_fmt_ee (join_code, pred_mode, tmp, tmp2);
	    }
	  else
	    {
	      PUT_CODE (cmp, swapped);
	      XEXP (cmp, 0) = right;
	      XEXP (cmp, 1) = left;
	    }
	  gcc_assert (swapped == comp_code
		      || float_comparison_operator (cmp, VOIDmode));
	}

      if (!pack || COMPARISON_P (cmp))
	{
	  XEXP (cmp, 0) = force_reg (comp_mode, XEXP (cmp, 0));
	  XEXP (cmp, 1) = force_reg (comp_mode, XEXP (cmp, 1));
	}
    }
  else if (VECTOR_MODE_P (comp_mode))
    {
      XEXP (cmp, 0) = force_reg (comp_mode, XEXP (cmp, 0));
      XEXP (cmp, 1) = force_reg (comp_mode, XEXP (cmp, 1));
    }

  emit_insn (gen_rtx_SET (pred, lvx_pack_cmp (cmp, pred_mode, pack)));
}


void
lvx_expand_conditional_move (rtx target, rtx select1, rtx select2, rtx comp)
{
  machine_mode mode = GET_MODE (target);
  rtx const0_mode_rtx = CONST0_RTX (mode);
  rtx constm1_mode_rtx = CONSTM1_RTX (mode);
  machine_mode comp_mode = GET_MODE (XEXP (comp, 0));
  machine_mode pred_mode = lvx_get_predicate_mode (comp_mode);
  enum mode_class comp_class = GET_MODE_CLASS (comp_mode);
  bool float_compare_p = (comp_class == MODE_FLOAT
			  || comp_class == MODE_VECTOR_FLOAT);
  bool vector_modes_p = VECTOR_MODE_P (mode) && VECTOR_MODE_P (comp_mode);

  int vector_any_and_zero = 0, vector_any_and_mone = 0;
  if (vector_modes_p)
    {
      vector_any_and_zero = (select2 == const0_mode_rtx) - (select1 == const0_mode_rtx);
      vector_any_and_mone = (select2 == constm1_mode_rtx) - (select1 == constm1_mode_rtx);
    }

  // Normalize conditional move as `dst = comp ? src : dst;`.
  rtx src = NULL_RTX, dst = NULL_RTX;
  enum rtx_code comp_code = GET_CODE (comp);
  if (vector_any_and_zero || vector_any_and_mone)
    dst = target;
  else if (!rtx_equal_p (select1, target) && !rtx_equal_p (select2, target))
    {
      if (reg_overlap_mentioned_p (target, comp))
	dst = gen_reg_rtx (mode);
      else
	dst = target;

      src = force_reg (mode, select1);
      emit_move_insn (dst, select2);
    }
  else if (rtx_equal_p (select1, target))
    {
      comp_code =
	float_compare_p ? reverse_condition_maybe_unordered (comp_code) :
	reverse_condition (comp_code);

      src = force_reg (mode, select2);
      dst = target;
    }
  else
    {
      src = force_reg (mode, select1);
      dst = target;
    }

  if (vector_any_and_zero < 0 || vector_any_and_mone > 0)
    comp_code = float_compare_p
      ? reverse_condition_maybe_unordered (comp_code)
      : reverse_condition (comp_code);

  // Put the comparison result into PRED.
  rtx pred = gen_reg_rtx (pred_mode);
  rtx compare = gen_rtx_fmt_ee (comp_code, pred_mode,
				XEXP (comp, 0), XEXP (comp, 1));
  lvx_lower_comparison (pred, compare, comp_mode);

  if (vector_any_and_zero || vector_any_and_mone)
    {
      if (vector_any_and_zero > 0 && select1 != constm1_mode_rtx)
	{
	  select1 = simplify_gen_subreg (pred_mode, force_reg (mode, select1), mode, 0);
	  lvx_emit_insn_slices (AND, pred_mode, pred, pred, select1);
	}
      if (vector_any_and_zero < 0 && select2 != constm1_mode_rtx)
	{
	  select2 = simplify_gen_subreg (pred_mode, force_reg (mode, select2), mode, 0);
	  lvx_emit_insn_slices (AND, pred_mode, pred, pred, select2);
	}
      if (vector_any_and_mone > 0 && select1 != const0_mode_rtx)
	{
	  select1 = simplify_gen_subreg (pred_mode, force_reg (mode, select1), mode, 0);
	  lvx_emit_insn_slices (IOR, pred_mode, pred, pred, select1);
	}
      if (vector_any_and_mone < 0 && select2 != const0_mode_rtx)
	{
	  select2 = simplify_gen_subreg (pred_mode, force_reg (mode, select2), mode, 0);
	  lvx_emit_insn_slices (IOR, pred_mode, pred, pred, select2);
	}
      pred = simplify_gen_subreg (mode, pred, pred_mode, 0);
      emit_insn (gen_rtx_SET (dst, pred));
    }
  else
    // Special cases above do not apply, conditional move with test `PRED NE CONST0`.
    // The combiner should then fold the comparaison into the conditional move test.
    {
      rtx const0_pred_mode_rtx = CONST0_RTX (pred_mode);
      rtx test = gen_rtx_fmt_ee (NE, VOIDmode, pred, const0_pred_mode_rtx);
      emit_insn (gen_rtx_SET (dst, gen_rtx_IF_THEN_ELSE (mode, test, src, dst)));
    }

  if (dst != target)
    emit_move_insn (target, dst);
}

void
lvx_expand_masked_move (rtx target, rtx select1, rtx select2, rtx pred)
{
  machine_mode mode = GET_MODE (target);
  rtx const0_mode_rtx = CONST0_RTX (mode);
  rtx constm1_mode_rtx = CONSTM1_RTX (mode);
  machine_mode pred_mode = GET_MODE (pred);
  bool vector_modes_p = VECTOR_MODE_P (mode) && VECTOR_MODE_P (pred_mode);

  int vector_any_and_zero = 0, vector_any_and_mone = 0;
  if (vector_modes_p)
    {
      vector_any_and_zero = (select2 == const0_mode_rtx) - (select1 == const0_mode_rtx);
      vector_any_and_mone = (select2 == constm1_mode_rtx) - (select1 == constm1_mode_rtx);
    }

  // Normalize conditional move as `dst = comp ? src : dst;`.
  rtx src = NULL_RTX, dst = NULL_RTX;
  enum rtx_code comp_code = NE;
  if (vector_any_and_zero || vector_any_and_mone)
    dst = target;
  else if (!rtx_equal_p (select1, target) && !rtx_equal_p (select2, target))
    {
      if (reg_overlap_mentioned_p (target, pred))
	dst = gen_reg_rtx (mode);
      else
	dst = target;

      src = force_reg (mode, select1);
      emit_move_insn (dst, select2);
    }
  else if (rtx_equal_p (select1, target))
    {
      comp_code = reverse_condition (comp_code);
      src = force_reg (mode, select2);
      dst = target;
    }
  else
    {
      src = force_reg (mode, select1);
      dst = target;
    }

  if (vector_any_and_zero < 0 || vector_any_and_mone > 0)
    comp_code = reverse_condition (comp_code);

  if (vector_any_and_zero || vector_any_and_mone)
    {
      select1 = simplify_gen_subreg (pred_mode, force_reg (mode, select1), mode, 0);
      select2 = simplify_gen_subreg (pred_mode, force_reg (mode, select2), mode, 0);
      if (vector_any_and_zero > 0 && select1 != constm1_mode_rtx)
	{
	  if (pred_mode == mode)
	    lvx_emit_insn_slices (AND, pred_mode, dst, pred, select1);
	  else
	    {
	      lvx_emit_insn_slices (AND, pred_mode, pred, pred, select1);
	      pred = simplify_gen_subreg (mode, pred, pred_mode, 0);
	      emit_insn (gen_rtx_SET (dst, pred));
	    }
	}
      if (vector_any_and_zero < 0 && select2 != constm1_mode_rtx)
	{
	  if (vector_any_and_mone > 0 && select1 != const0_mode_rtx)
	    lvx_emit_insn_slices (IOR, pred_mode, pred, pred, select1);
	  if (vector_any_and_mone < 0 && select2 != const0_mode_rtx)
	    lvx_emit_insn_slices (IOR, pred_mode, pred, pred, select2);

	  lvx_emit_insn_slices (AND, pred_mode, pred, pred, select2);
	  pred = simplify_gen_subreg (mode, pred, pred_mode, 0);
	  emit_insn (gen_rtx_SET (dst, pred));
	}
    }
  else
    {
      rtx const0_pred_mode = CONST0_RTX (pred_mode);
      rtx cmp0 = gen_rtx_fmt_ee (comp_code, VOIDmode, pred, const0_pred_mode);
      emit_insn (gen_rtx_SET (dst, gen_rtx_IF_THEN_ELSE (mode, cmp0, src, dst)));
    }

  if (dst != target)
    emit_move_insn (target, dst);
}


void
lvx_expand_vector_insert (rtx target, rtx source, rtx where)
{
  machine_mode vector_mode = GET_MODE (target);
  machine_mode inner_mode = GET_MODE_INNER (vector_mode);
  unsigned width = GET_MODE_SIZE (inner_mode);

  if (CONST_INT_P (where))
    {
      int index = INTVAL (where);
      int major = (index * width) / UNITS_PER_WORD;
      int minor = (index * width) % UNITS_PER_WORD;

      if (GET_MODE_SIZE (inner_mode) == UNITS_PER_WORD)
	{
	  rtx op0 = simplify_gen_subreg (inner_mode, target, vector_mode,
					 major * UNITS_PER_WORD);
	  rtx op1 = source;
	  emit_move_insn (op0, op1);
	}
      else
	{
	  rtx op0 = simplify_gen_subreg (DImode, target, vector_mode,
					 major * UNITS_PER_WORD);
	  rtx op1 = gen_lowpart (DImode, source);
	  rtx op2 = GEN_INT (width * BITS_PER_UNIT);
	  rtx op3 = GEN_INT (minor * BITS_PER_UNIT);
	  rtx opi = gen_rtx_ZERO_EXTRACT (DImode, op0, op2, op3);
	  emit_insn (gen_rtx_SET (opi, op1));
	}

      return;
    }

  gcc_unreachable ();
}

void
lvx_expand_vector_extract (rtx target, rtx source, rtx where)
{
  machine_mode vector_mode = GET_MODE (source);
  machine_mode inner_mode = GET_MODE_INNER (vector_mode);
  unsigned width = GET_MODE_SIZE (inner_mode);

  if (CONST_INT_P (where))
    {
      int index = INTVAL (where);
      int major = (index * width) / UNITS_PER_WORD;
      int minor = (index * width) % UNITS_PER_WORD;

      if (GET_MODE_SIZE (inner_mode) == UNITS_PER_WORD)
	{
	  rtx op0 = target;
	  rtx op1 = simplify_gen_subreg (inner_mode, source, vector_mode,
					 major * UNITS_PER_WORD);
	  emit_move_insn (op0, op1);
	}
      else
	{
	  rtx op0 = simplify_gen_subreg (DImode, target, inner_mode, 0);
	  rtx op1 = simplify_gen_subreg (DImode, source, vector_mode,
					 major * UNITS_PER_WORD);
	  rtx op2 = GEN_INT (width * BITS_PER_UNIT);
	  rtx op3 = GEN_INT (minor * BITS_PER_UNIT);
	  rtx extract = gen_rtx_ZERO_EXTRACT (DImode, op1, op2, op3);
	  emit_insn (gen_rtx_SET (op0, extract));
	}

      return;
    }

  gcc_unreachable ();
}

#define LVX_SBMM8D_IDENTITY 0x8040201008040201ULL

/* Splat a value of mode smaller than a word into a word size vector chunk.
 * This is used both for initializing a vector from a scalar, and for the
 * vector arithmetic patterns that operate a vector with a scalar.
 *
 * SPLAT{B,H,W}Q broadcast the low element of a GPR across a 128-bit pair, so
 * the chunk is the low half of one of those.  It replaces an sbmm8d against a
 * magic constant, which cost a maked and a register
 * before the multiply -- and which had no insn pattern at all, so every splat
 * that reached here was an unrecognizable insn.  */
void
lvx_expand_chunk_splat (rtx target, rtx source, machine_mode inner_mode)
{
  machine_mode chunk_mode = GET_MODE (target);
  unsigned inner_size = GET_MODE_SIZE (inner_mode);

  if (inner_size == UNITS_PER_WORD)
    {
      emit_move_insn (target, source);
      return;
    }

  machine_mode vmode;
  switch (inner_mode)
    {
    case E_QImode: vmode = V16QImode; break;
    case E_HImode: vmode = V8HImode;  break;
    case E_HFmode: vmode = V8HFmode;  break;
    case E_SImode: vmode = V4SImode;  break;
    case E_SFmode: vmode = V4SFmode;  break;
    default: gcc_unreachable ();
    }

  if (GET_MODE (source) != inner_mode)
    source = gen_lowpart (inner_mode, source);
  source = force_reg (inner_mode, source);

  rtx pair = gen_reg_rtx (vmode);
  emit_insn (gen_rtx_SET (pair, gen_rtx_VEC_DUPLICATE (vmode, source)));
  emit_move_insn (target, simplify_gen_subreg (chunk_mode, pair, vmode, 0));
}

/* The integer value of a constant lane, or NULL_RTX for a variable one.  A
   float lane arrives as a CONST_DOUBLE, and its bits are what goes in the
   chunk, so it folds too.  */
static rtx
lvx_lane_const (rtx x, machine_mode inner_mode, scalar_int_mode int_inner)
{
  if (CONST_INT_P (x))
    return x;
  if (!CONSTANT_P (x))
    return NULL_RTX;
  rtx c = simplify_subreg (int_inner, x, inner_mode, 0);
  return (c && CONST_INT_P (c)) ? c : NULL_RTX;
}

/* Helper function for lvx_expand_vector_init () in case inner mode size < 64 bits.
 * The init source has been partioned into 64-bit chunks, which are inserted into
 * the corresponding 64-bit chunks of the target.
 *
 * A 128-bit vector is a GPR pair, so a lane is a bit-field at a known offset of
 * a known half -- and LVX has a bit-field insert, insf, which the insv pattern
 * spells.  So a chunk is built with the scalar instructions the ISA already
 * has: the constant lanes fold into ONE immediate (a maked covers every constant
 * in the chunk at once), and each variable lane is one insf.  The first lane of
 * an all-variable chunk is a zero-extend rather than an insf into a zero: same
 * result, one instruction fewer.
 *
 * This used to emit UNSPEC_INITX4A..D / UNSPEC_INITX8A..H and VEC_CONCAT on the
 * 64-bit chunk, the departed 64-bit SIMD family, and none of those had a
 * pattern -- `v4si v = {a,b,c,d}` was an unrecognizable insn at every level.
 * Those unspecs are gone from unspec.md too, so the names above are history and
 * will not be found by a grep.  */
static void
lvx_expand_chunk_insert (rtx target, rtx source, int index,
			 machine_mode inner_mode)
{
  /* The driver hands the chunk over as a 64-bit VECTOR subreg of the target
     (V2SI for a V4SI, and so on), which is the right shape for a vector move
     and the wrong one for what happens here: a maked wants an integer
     immediate and insf a zero_extract on an integer.  So work on the DImode
     view of the same bits, which simplify_gen_subreg folds through the vector
     subreg to a DI subreg of the vector itself.  */
  machine_mode chunk_mode = GET_MODE (target);
  if (chunk_mode != DImode)
    {
      target = simplify_gen_subreg (DImode, target, chunk_mode, 0);
      chunk_mode = DImode;
    }
  unsigned inner_bits = GET_MODE_BITSIZE (inner_mode);
  int nlanes = GET_MODE_BITSIZE (chunk_mode) / inner_bits;
  scalar_int_mode int_inner = int_mode_for_mode (inner_mode).require ();
  unsigned HOST_WIDE_INT lane_mask
    = inner_bits >= HOST_BITS_PER_WIDE_INT
      ? HOST_WIDE_INT_M1U : (HOST_WIDE_INT_1U << inner_bits) - 1;

  unsigned HOST_WIDE_INT bits = 0;
  bool any_const = false;
  for (int i = 0; i < nlanes; i++)
    {
      rtx c = lvx_lane_const (XVECEXP (source, 0, index + i), inner_mode, int_inner);
      if (c)
	{
	  bits |= (UINTVAL (c) & lane_mask) << (i * inner_bits);
	  any_const = true;
	}
    }

  bool seeded = false;
  if (any_const)
    {
      emit_move_insn (target, gen_int_mode (bits, chunk_mode));
      seeded = true;
    }

  for (int i = 0; i < nlanes; i++)
    {
      rtx x = XVECEXP (source, 0, index + i);
      if (lvx_lane_const (x, inner_mode, int_inner))
	continue;
      rtx val = force_reg (inner_mode, x);
      if (!seeded)
	{
	  /* No constants, so this is lane 0 of an all-variable chunk.  */
	  gcc_assert (i == 0);
	  emit_insn (gen_rtx_SET (target,
				  gen_rtx_ZERO_EXTEND (chunk_mode,
						       gen_lowpart (int_inner, val))));
	  seeded = true;
	  continue;
	}
      rtx field = gen_rtx_ZERO_EXTRACT (chunk_mode, target,
					GEN_INT (inner_bits),
					GEN_INT (i * inner_bits));
      emit_insn (gen_rtx_SET (field, gen_lowpart (chunk_mode, val)));
    }
}

/* Called by the vec_duplicate<mode> standard pattern and by
 * lvx_expand_vector_init ().  */
void
lvx_expand_vector_duplicate (rtx target, rtx source)
{
  machine_mode vector_mode = GET_MODE (target);
  machine_mode inner_mode = GET_MODE_INNER (vector_mode);
  machine_mode chunk_mode = lvx_get_chunk_mode (vector_mode);
  unsigned vector_size = GET_MODE_SIZE (vector_mode);
  unsigned inner_size = GET_MODE_SIZE (inner_mode);

  /* A 128-bit vector of sub-word lanes is exactly one SPLAT{B,H,W}Q, so emit
     the canonical vec_duplicate and let *splat128 match it.  Going through the
     chunk below would splat into a temporary pair, take its low half, and then
     *dup128 that half back into both halves -- three instructions for one.
     Double-word lanes stay on the chunk route: *dup128 makes those a single
     copyd, which is tiny where splatdq is lite.  */
  if (GET_MODE (source) != inner_mode)
    source = gen_lowpart (inner_mode, source);

  /* A lane in memory splatted across a 256-bit quad is one L{B,H,W,D}SO
     (*lso<mode>), whatever the lane: keep the MEM so the insn can take it.  */
  if (vector_size == 4 * UNITS_PER_WORD && MEM_P (source))
    {
      emit_insn (gen_rtx_SET (target,
			      gen_rtx_VEC_DUPLICATE (vector_mode, source)));
      return;
    }

  /* A 256- or 512-bit vector of sub-word lanes is a SPLAT{B,H,W}Q into the
     low pair and copies of that pair (*splat256, *splat512).  The chunk route
     below cannot express it: its *dup256 split moves the chunk in a 64-bit
     vector mode, which has no mov pattern.  */
  if (vector_size > 2 * UNITS_PER_WORD && inner_size < UNITS_PER_WORD)
    {
      source = force_reg (inner_mode, source);
      emit_insn (gen_rtx_SET (target,
			      gen_rtx_VEC_DUPLICATE (vector_mode, source)));
      return;
    }

  if (vector_size == 2 * UNITS_PER_WORD && inner_size < UNITS_PER_WORD)
    {
      source = force_reg (inner_mode, source);
      emit_insn (gen_rtx_SET (target,
			      gen_rtx_VEC_DUPLICATE (vector_mode, source)));
      return;
    }

  rtx chunk = gen_reg_rtx (chunk_mode);
  lvx_expand_chunk_splat (chunk, source, inner_mode);

  if (vector_size > UNITS_PER_WORD)
    emit_insn (gen_rtx_SET (target,
			    gen_rtx_VEC_DUPLICATE (vector_mode, chunk)));
  else
    emit_insn (gen_rtx_SET (target, chunk));

  return;
}

/* Called by the vec_init<mode> standard pattern. */
void
lvx_expand_vector_init (rtx target, rtx source)
{
  machine_mode vector_mode = GET_MODE (target);
  unsigned vector_size = GET_MODE_SIZE (vector_mode);
  machine_mode inner_mode = GET_MODE_INNER (vector_mode);
  unsigned inner_size = GET_MODE_SIZE (inner_mode);
  int nunits = GET_MODE_NUNITS (vector_mode);

  /* Special-case for the duplicate of a value. */
  bool duplicate = true;
  rtx x0 = XVECEXP (source, 0, 0);

  for (int i = 1; i < nunits; ++i)
    {
      rtx x = XVECEXP (source, 0, i);
      duplicate &= rtx_equal_p (x, x0);
    }
  if (duplicate)
    {
      lvx_expand_vector_duplicate (target, x0);
      return;
    }

  /* Inner mode size not smaller than word size, delegate to move pattern. */
  if (inner_size >= UNITS_PER_WORD)
    {
      for (int i = 0; i < nunits; i++)
	{
	  rtx op1 = XVECEXP (source, 0, i);
	  rtx op0 =
	    simplify_gen_subreg (inner_mode, target, vector_mode,
				 i * inner_size);
	  emit_move_insn (op0, op1);
	}
      return;
    }

  /* Process vector init chunk by chunk */
  int nwords = vector_size / UNITS_PER_WORD;
  int chunk_nunits = UNITS_PER_WORD / inner_size;
  machine_mode chunk_mode = lvx_get_chunk_mode (vector_mode);

  for (int i = 0; i < nwords; i++)
    {
      int nvars = 0;
      rtx splat = XVECEXP (source, 0, i * chunk_nunits);
      rtx op0 =
	simplify_gen_subreg (chunk_mode, target, vector_mode,
			     i * UNITS_PER_WORD);

      /* Count the number of variable elements and compute splat. */
      for (int j = 0; j < chunk_nunits; j++)
	{
	  rtx x = XVECEXP (source, 0, i * chunk_nunits + j);
	  nvars += !(CONST_INT_P (x) || CONST_DOUBLE_P (x));
	  splat = rtx_equal_p (splat, x) ? splat : 0;
	}

      /* No variable elements, delegate to move chunk const vector. */
      if (nvars == 0)
	{
	  rtvec v = rtvec_alloc (chunk_nunits);
	  for (int j = 0; j < chunk_nunits; j++)
	    RTVEC_ELT (v, j) = XVECEXP (source, 0, i * chunk_nunits + j);
	  rtx constants = gen_rtx_CONST_VECTOR (chunk_mode, v);
	  emit_move_insn (op0, constants);
	  continue;
	}

      /* Splat first element to all words in chunk. */
      if (splat)
	{
	  lvx_expand_chunk_splat (op0, splat, inner_mode);
	  continue;
	}

      /* Insert the variable lanes in chunk. */
      lvx_expand_chunk_insert (op0, source, i * chunk_nunits, inner_mode);
    }
}


/* Helper to expand SPNs vec_shl_insert, vec_shl, vec_shr.  */
void
lvx_expand_vector_shift (rtx target, rtx source, rtx chunk,
			 unsigned bits, int left)
{
  machine_mode mode = GET_MODE (target);
  unsigned mode_size = GET_MODE_SIZE (mode);
  gcc_assert ((unsigned) bits < 512U);
  if (chunk != const0_rtx)
    chunk = simplify_gen_subreg (DImode, chunk, GET_MODE (chunk), 0);

  if (bits > mode_size * BITS_PER_UNIT)
    {
      emit_insn (gen_rtx_SET (target, CONST0_RTX (mode)));
      return;
    }

  bits %= mode_size * BITS_PER_UNIT;
  if (!bits)
    {
      emit_insn (gen_rtx_SET (target, source));
      return;
    }

  unsigned offset = bits / BITS_PER_UNIT;

  /* A 128-bit vector shifted by a count that is not a whole number of 64-bit
     words is ONE SLLQ/SRLQ on the TImode view (ashlti3/lshrti3 in scalar.md),
     where the word-at-a-time synthesis below spends four instructions moving
     the bits across the word boundary -- two SRLD/SLLD and two INSFD.  That is
     what a reduction epilogue pays for its last steps: GCC has no
     reduc_plus_scal_<mode> here, so it reduces with the generic log-step
     shift-and-add tree over vec_shr_<mode>, and at lane widths below 64 bits
     the final shifts by one, two or four lanes are all sub-word.

     The word-ALIGNED counts are deliberately left to the code below: there
     they are plain register moves, which is already optimal and better than a
     shift, and a count of 64 or more does not fit SLLQ's 6-bit immediate in any
     case (it would need the register form, costing a MAKED to materialise).

     Only the zero-fill shifts take this path.  vec_shl_insert passes the lane
     it shifts in as CHUNK, and placing that still needs the insert the word
     code does, so it keeps the old route rather than growing a second one.  */
  if (mode_size == 16
      && chunk == const0_rtx
      && (offset % UNITS_PER_WORD) != 0)
    {
      rtx ti_target = simplify_gen_subreg (TImode, target, mode, 0);
      rtx ti_source = simplify_gen_subreg (TImode, source, mode, 0);
      if (ti_target && ti_source)
	{
	  /* U06 holds 0..63; a wider count has to go through a register, which
	     the pattern's "r" alternative takes and the hardware masks to the
	     7 bits a 128-bit shift needs.  */
	  rtx count = GEN_INT (bits);
	  if (bits >= 64)
	    count = force_reg (SImode, count);
	  /* simplify_gen_subreg of a MEM gives back a MEM -- a subreg of memory
	     simplifies to memory at an adjusted address -- and gen_* does not
	     check its predicates, so a memory SOURCE here reaches recog as
	     unmatched RTL.  It is not caught where it is built either: nothing
	     validates until instantiate_virtual_regs, so the ICE names `vregs'
	     and points at this insn with no hint of who made it.  */
	  if (!register_operand (ti_source, TImode))
	    ti_source = force_reg (TImode, ti_source);
	  rtx ti_dst = ti_target;
	  if (!register_operand (ti_dst, TImode))
	    ti_dst = gen_reg_rtx (TImode);
	  if (left)
	    emit_insn (gen_ashlti3 (ti_dst, ti_source, count));
	  else
	    emit_insn (gen_lshrti3 (ti_dst, ti_source, count));
	  if (ti_dst != ti_target)
	    emit_move_insn (ti_target, ti_dst);
	  return;
	}
    }

  unsigned aligned = offset & -UNITS_PER_WORD;
  if (aligned == offset)
    {
      if (left)
	for (int i = 0; i < (int) mode_size; i += UNITS_PER_WORD)
	  {
	    int j = i - (int) offset;
	    rtx target_i = simplify_gen_subreg (DImode, target, mode, i);
	    rtx source_j = j >= 0 ?
	      simplify_gen_subreg (DImode, source, mode, j) : chunk;
	    emit_insn (gen_rtx_SET (target_i, source_j));
	  }
      else
	for (int i = 0; i < (int) mode_size; i += UNITS_PER_WORD)
	  {
	    int j = i + (int) offset;
	    rtx target_i = simplify_gen_subreg (DImode, target, mode, i);
	    rtx source_j = j < (int) mode_size ?
	      simplify_gen_subreg (DImode, source, mode, j) : chunk;
	    emit_insn (gen_rtx_SET (target_i, source_j));
	  }
    }
  else
    {
      rtx shift = GEN_INT ((offset % UNITS_PER_WORD) * BITS_PER_UNIT);
      rtx nshift = GEN_INT ((-offset % UNITS_PER_WORD) * BITS_PER_UNIT);
      if (left)
	for (int i = 0; i < (int) mode_size; i += UNITS_PER_WORD)
	  {
	    int j = i - (int) aligned;
	    int k = j - UNITS_PER_WORD;
	    rtx target_i = simplify_gen_subreg (DImode, target, mode, i);
	    if (k >= 0)
	      {
		rtx source_j = simplify_gen_subreg (DImode, source, mode, j);
		rtx source_k = simplify_gen_subreg (DImode, source, mode, k);
		emit_insn (gen_rtx_SET (target_i, gen_rtx_LSHIFTRT (DImode, source_k, nshift)));
		emit_insn (gen_rtx_SET (gen_rtx_ZERO_EXTRACT (DImode, target_i, nshift, shift), source_j));
	      }
	    else if (j >= 0)
	      {
		rtx source_j = simplify_gen_subreg (DImode, source, mode, j);
		emit_insn (gen_rtx_SET (target_i, gen_rtx_ASHIFT (DImode, source_j, shift)));
		if (chunk != const0_rtx)
		  emit_insn (gen_rtx_SET (gen_rtx_ZERO_EXTRACT (DImode, target_i, shift, const0_rtx), chunk));
	      }
	    else
	      emit_insn (gen_rtx_SET (target_i, chunk));
	  }
      else
	for (int i = 0; i < (int) mode_size; i += UNITS_PER_WORD)
	  {
	    int j = i + (int) aligned;
	    int k = j + UNITS_PER_WORD;
	    rtx target_i = simplify_gen_subreg (DImode, target, mode, i);
	    if (k < (int) mode_size)
	      {
		rtx source_j = simplify_gen_subreg (DImode, source, mode, j);
		rtx source_k = simplify_gen_subreg (DImode, source, mode, k);
		emit_insn (gen_rtx_SET (target_i, gen_rtx_LSHIFTRT (DImode, source_j, shift)));
		emit_insn (gen_rtx_SET (gen_rtx_ZERO_EXTRACT (DImode, target_i, shift, nshift), source_k));
	      }
	    else if (j < (int) mode_size)
	      {
		rtx source_j = simplify_gen_subreg (DImode, source, mode, j);
		emit_insn (gen_rtx_SET (target_i, gen_rtx_LSHIFTRT (DImode, source_j, shift)));
		if (chunk != const0_rtx)
		  emit_insn (gen_rtx_SET (gen_rtx_ZERO_EXTRACT (DImode, target_i, shift, nshift), chunk));
	      }
	    else
	      emit_insn (gen_rtx_SET (target_i, chunk));
	  }
    }
}

/* Emit code that sets TARGET to non-zero or zero if any of the lanes of SOURCE
   in MODE is zero.  */
void
lvx_expand_any64_eqz (rtx target, rtx source, enum machine_mode mode)
{
  gcc_assert (GET_MODE_SIZE (mode) == UNITS_PER_WORD);
  {
      rtx op0 = simplify_gen_subreg (mode, target, DImode, 0);
      rtx compn_eqz = gen_rtx_EQ (mode, source, CONST0_RTX (mode));
      emit_insn (gen_rtx_SET (op0, compn_eqz));
    }
}


/* Helper function to produce manifest _Float16 constants, where the float
 * value represented as integer comes from REAL_VALUE_TO_TARGET_SINGLE.
 * Adapted from http://openkb.fr/Half-precision_floating-point_in_Java */
static unsigned
lvx_float_to_half_as_int (unsigned fbits)
{
  unsigned sign = fbits >> 16 & 0x8000;	// sign only
  unsigned val = (fbits & 0x7fffffff) + 0x1000;	// rounded value
  if (val >= 0x47800000)	// might be or become NaN/Inf
    {				// avoid Inf due to rounding
      if ((fbits & 0x7fffffff) >= 0x47800000)
	{			// is or must become NaN/Inf
	  if (val < 0x7f800000)	// was value but too large
	    return sign | 0x7c00;	// make it +/-Inf
	  return sign | 0x7c00 |	// remains +/-Inf or NaN
	    (fbits & 0x007fffff) >> 13;	// keep NaN (and Inf) bits
	}
      return sign | 0x7bff;	// unrounded not quite Inf
    }
  if (val >= 0x38800000)	// remains normalized value
    return sign | (val - 0x38000000) >> 13;	// exp - 127 + 15
  if (val < 0x33000000)		// too small for subnormal
    return sign;		// becomes +/-0
  val = (fbits & 0x7fffffff) >> 23;	// tmp exp for subnormal calc
  return sign | ((((fbits & 0x7fffff) | 0x800000)	// add subnormal bit
		  + (0x800000 >> (val - 102)))	// round depending on cut off
		 >> (126 - val));	// div by 2^(1-(exp-127+15)) and >> 13 | exp=0
}

HOST_WIDE_INT
lvx_const_vector_value (rtx x, int slice)
{
  HOST_WIDE_INT value = 0;
  if (GET_CODE (x) == CONST_VECTOR)
    {
      machine_mode mode = GET_MODE (x);
      machine_mode inner_mode = GET_MODE_INNER (mode);
      int index = slice * (UNITS_PER_WORD / GET_MODE_SIZE (inner_mode));
      if (inner_mode == QImode)
	{
	  HOST_WIDE_INT val_0 = INTVAL (CONST_VECTOR_ELT (x, index + 0));
	  HOST_WIDE_INT val_1 = INTVAL (CONST_VECTOR_ELT (x, index + 1));
	  HOST_WIDE_INT val_2 = INTVAL (CONST_VECTOR_ELT (x, index + 2));
	  HOST_WIDE_INT val_3 = INTVAL (CONST_VECTOR_ELT (x, index + 3));
	  HOST_WIDE_INT val_4 = INTVAL (CONST_VECTOR_ELT (x, index + 4));
	  HOST_WIDE_INT val_5 = INTVAL (CONST_VECTOR_ELT (x, index + 5));
	  HOST_WIDE_INT val_6 = INTVAL (CONST_VECTOR_ELT (x, index + 6));
	  HOST_WIDE_INT val_7 = INTVAL (CONST_VECTOR_ELT (x, index + 7));
	  value = (val_0 & 0xFF) | (val_1 & 0xFF) << 8 | (val_2 & 0xFF) << 16
		  | (val_3 & 0xFF) << 24 | (val_4 & 0xFF) << 32
		  | (val_5 & 0xFF) << 40 | (val_6 & 0xFF) << 48
		  | (val_7 & 0xFF) << 56;
	}
      else if (inner_mode == HImode)
	{
	  HOST_WIDE_INT val_0 = INTVAL (CONST_VECTOR_ELT (x, index + 0));
	  HOST_WIDE_INT val_1 = INTVAL (CONST_VECTOR_ELT (x, index + 1));
	  HOST_WIDE_INT val_2 = INTVAL (CONST_VECTOR_ELT (x, index + 2));
	  HOST_WIDE_INT val_3 = INTVAL (CONST_VECTOR_ELT (x, index + 3));
	  value = (val_0 & 0xFFFF) | (val_1 & 0xFFFF) << 16
		  | (val_2 & 0xFFFF) << 32 | (val_3 & 0xFFFF) << 48;
	}
      else if (inner_mode == SImode)
	{
	  HOST_WIDE_INT val_0 = INTVAL (CONST_VECTOR_ELT (x, index + 0));
	  HOST_WIDE_INT val_1 = INTVAL (CONST_VECTOR_ELT (x, index + 1));
	  value = (val_0 & 0xFFFFFFFF) | (val_1 & 0xFFFFFFFF) << 32;
	}
      else if (inner_mode == DImode)
	{
	  value = INTVAL (CONST_VECTOR_ELT (x, index + 0));
	}
      else if (inner_mode == CQImode)
	{
	  if (CONST_VECTOR_P (CONST_VECTOR_ELT (x, 0)))
	    {
	      HOST_WIDE_INT val_0_r = INTVAL (CONST_VECTOR_ELT
					      (CONST_VECTOR_ELT
					       (x, index + 0), 0));
	      HOST_WIDE_INT val_0_i =
		INTVAL (CONST_VECTOR_ELT
			(CONST_VECTOR_ELT (x, index + 0), 1));
	      HOST_WIDE_INT val_1_r =
		INTVAL (CONST_VECTOR_ELT
			(CONST_VECTOR_ELT (x, index + 1), 0));
	      HOST_WIDE_INT val_1_i =
		INTVAL (CONST_VECTOR_ELT
			(CONST_VECTOR_ELT (x, index + 1), 1));
	      HOST_WIDE_INT val_2_r =
		INTVAL (CONST_VECTOR_ELT
			(CONST_VECTOR_ELT (x, index + 2), 0));
	      HOST_WIDE_INT val_2_i =
		INTVAL (CONST_VECTOR_ELT
			(CONST_VECTOR_ELT (x, index + 2), 1));
	      HOST_WIDE_INT val_3_r =
		INTVAL (CONST_VECTOR_ELT
			(CONST_VECTOR_ELT (x, index + 3), 0));
	      HOST_WIDE_INT val_3_i =
		INTVAL (CONST_VECTOR_ELT
			(CONST_VECTOR_ELT (x, index + 3), 1));
	      value =
		(val_0_r & 0xFF) | (val_0_i & 0xFF) << 8 | (val_1_r & 0xFF) <<
		16 | (val_1_i & 0xFF) << 24 | (val_2_r & 0xFF) << 32 |
		(val_2_i & 0xFF) << 40 | (val_3_r & 0xFF) << 48 | (val_3_i &
								   0xFF) <<
		56;
	    }
	  else
	    {
	      HOST_WIDE_INT val_0 = INTVAL (CONST_VECTOR_ELT (x, index + 0));
	      HOST_WIDE_INT val_1 = INTVAL (CONST_VECTOR_ELT (x, index + 1));
	      HOST_WIDE_INT val_2 = INTVAL (CONST_VECTOR_ELT (x, index + 2));
	      HOST_WIDE_INT val_3 = INTVAL (CONST_VECTOR_ELT (x, index + 3));
	      value = (val_0 & 0xFFFF) | (val_1 & 0xFFFF) << 16
		| (val_2 & 0xFFFF) << 32 | (val_3 & 0xFFFF) << 48;
	    }

	}
      else if (inner_mode == CHImode)
	{
	  if (CONST_VECTOR_P (CONST_VECTOR_ELT (x, 0)))
	    {
	      HOST_WIDE_INT val_0_r =
		INTVAL (CONST_VECTOR_ELT
			(CONST_VECTOR_ELT (x, index + 0), 0));
	      HOST_WIDE_INT val_0_i =
		INTVAL (CONST_VECTOR_ELT
			(CONST_VECTOR_ELT (x, index + 0), 1));
	      HOST_WIDE_INT val_1_r =
		INTVAL (CONST_VECTOR_ELT
			(CONST_VECTOR_ELT (x, index + 1), 0));
	      HOST_WIDE_INT val_1_i =
		INTVAL (CONST_VECTOR_ELT
			(CONST_VECTOR_ELT (x, index + 1), 1));
	      value =
		(val_0_r & 0xFFFF) | (val_0_i & 0xFFFF) << 16 | (val_1_r &
								 0xFFFF) << 32
		| (val_1_i & 0xFFFF) << 48;
	    }
	  else
	    {
	      HOST_WIDE_INT val_0 = INTVAL (CONST_VECTOR_ELT (x, index + 0));
	      HOST_WIDE_INT val_1 = INTVAL (CONST_VECTOR_ELT (x, index + 1));
	      value = (val_0 & 0xFFFFFFFF) | (val_1 & 0xFFFFFFFF) << 32;
	    }

	}
      else if (inner_mode == CSImode)
	{
	  if (CONST_VECTOR_P (CONST_VECTOR_ELT (x, 0)))
	    {
	      HOST_WIDE_INT val_0_r =
		INTVAL (CONST_VECTOR_ELT
			(CONST_VECTOR_ELT (x, index + 0), 0));
	      HOST_WIDE_INT val_0_i =
		INTVAL (CONST_VECTOR_ELT
			(CONST_VECTOR_ELT (x, index + 0), 1));
	      value = (val_0_r & 0xFFFFFFFF) | (val_0_i & 0xFFFFFFFF) << 32;
	    }
	  else
	    {
	      value = INTVAL (CONST_VECTOR_ELT (x, index + 0));
	    }
	}
      else if (inner_mode == SCmode)
	{
	  if (CONST_VECTOR_P (CONST_VECTOR_ELT (x, 0)))
	    {
	      long val_0 = 0, val_1 = 0;
	      rtx elt_0 =
		CONST_VECTOR_ELT (CONST_VECTOR_ELT (x, index + 0), 0);
	      rtx elt_1 =
		CONST_VECTOR_ELT (CONST_VECTOR_ELT (x, index + 0), 1);
	      REAL_VALUE_TO_TARGET_SINGLE (*CONST_DOUBLE_REAL_VALUE (elt_0),
					   val_0);
	      REAL_VALUE_TO_TARGET_SINGLE (*CONST_DOUBLE_REAL_VALUE (elt_1),
					   val_1);
	      value =
		((HOST_WIDE_INT) val_0 & 0xFFFFFFFF) | ((HOST_WIDE_INT) val_1
							& 0xFFFFFFFF) << 32;
	    }
	  else
	    {
	      long val[2] = { 0, 0 };
	      rtx elt_0 = CONST_VECTOR_ELT (x, index + 0);
	      REAL_VALUE_TO_TARGET_DOUBLE (*CONST_DOUBLE_REAL_VALUE (elt_0),
					   val);
	      value =
		((HOST_WIDE_INT) val[0] & 0xFFFFFFFF) | ((HOST_WIDE_INT)
							 val[1] & 0xFFFFFFFF)
		<< 32;
	    }
	}
      else if (inner_mode == HFmode)
	{
	  long val_0 = 0, val_1 = 0, val_2 = 0, val_3 = 0;
	  rtx elt_0 = CONST_VECTOR_ELT (x, index + 0);
	  rtx elt_1 = CONST_VECTOR_ELT (x, index + 1);
	  rtx elt_2 = CONST_VECTOR_ELT (x, index + 2);
	  rtx elt_3 = CONST_VECTOR_ELT (x, index + 3);
	  REAL_VALUE_TO_TARGET_SINGLE (*CONST_DOUBLE_REAL_VALUE (elt_0), val_0);
	  REAL_VALUE_TO_TARGET_SINGLE (*CONST_DOUBLE_REAL_VALUE (elt_1), val_1);
	  REAL_VALUE_TO_TARGET_SINGLE (*CONST_DOUBLE_REAL_VALUE (elt_2), val_2);
	  REAL_VALUE_TO_TARGET_SINGLE (*CONST_DOUBLE_REAL_VALUE (elt_3), val_3);
	  val_0 = lvx_float_to_half_as_int (val_0);
	  val_1 = lvx_float_to_half_as_int (val_1);
	  val_2 = lvx_float_to_half_as_int (val_2);
	  val_3 = lvx_float_to_half_as_int (val_3);
	  value = (val_0 & 0xFFFF) | (val_1 & 0xFFFF) << 16
		  | (val_2 & 0xFFFF) << 32 | (val_3 & 0xFFFF) << 48;
	}
      else if (inner_mode == SFmode)
	{
	  long val_0 = 0, val_1 = 0;
	  rtx elt_0 = CONST_VECTOR_ELT (x, index + 0);
	  rtx elt_1 = CONST_VECTOR_ELT (x, index + 1);
	  REAL_VALUE_TO_TARGET_SINGLE (*CONST_DOUBLE_REAL_VALUE (elt_0), val_0);
	  REAL_VALUE_TO_TARGET_SINGLE (*CONST_DOUBLE_REAL_VALUE (elt_1), val_1);
	  value = ((HOST_WIDE_INT) val_0 & 0xFFFFFFFF)
		  | ((HOST_WIDE_INT) val_1 & 0xFFFFFFFF) << 32;
	}
      else if ((inner_mode == DFmode))
	{
	  long val[2] = { 0, 0 };
	  rtx elt_0 = CONST_VECTOR_ELT (x, index + 0);
	  REAL_VALUE_TO_TARGET_DOUBLE (*CONST_DOUBLE_REAL_VALUE (elt_0), val);
	  value = ((HOST_WIDE_INT) val[0] & 0xFFFFFFFF)
		  | ((HOST_WIDE_INT) val[1] & 0xFFFFFFFF) << 32;
	}
      else
	gcc_unreachable ();
      return value;
    }
  else
    gcc_unreachable ();
  return value;
}


/* This function is used to expand the SPNs vec_unpack[us]_{hi,lo}.
   Let V(nmemb, elem_size) be a vector of NMEMB of size ELEM_SIZE,
   the SPNs vec_unpack[us]_{hi,lo} extract the high (resp. low) part
   of a vector of type V(N,M) into a vector of type V(N/2,2M) and
   zero (resp. sign) extend the elements.

   op0: destination operand
   op1: source operand
   signed_p: is the source operand signed
   hi_p: if set extracts the high part of op1. */
bool
lvx_expand_unpack (rtx op0, rtx op1, bool signed_p, bool hi_p)
{
  machine_mode op0_mode = GET_MODE (op0);
  unsigned op0_mode_size = GET_MODE_SIZE (op0_mode);
  scalar_mode op0_inner_mode = GET_MODE_INNER (op0_mode);
  unsigned op0_inner_size = GET_MODE_SIZE (op0_inner_mode);
  unsigned op0_nmemb = op0_mode_size / op0_inner_size;
  machine_mode op0_half_mode =
    mode_for_vector (op0_inner_mode, op0_nmemb / 2).require ();

  machine_mode op1_mode = GET_MODE (op1);
  unsigned op1_mode_size = GET_MODE_SIZE (op1_mode);
  scalar_mode op1_inner_mode = GET_MODE_INNER (op1_mode);
  unsigned op1_inner_size = GET_MODE_SIZE (op1_inner_mode);
  unsigned op1_nmemb = op1_mode_size / op1_inner_size;
  machine_mode op1_half_mode =
    mode_for_vector (op1_inner_mode, op1_nmemb / 2).require ();

  gcc_assert (op0_mode_size == op1_mode_size
	      && op0_inner_size == 2 * op1_inner_size);

  /* The recursion bottoms out at 128 bits, where LVX's WIDEN* lane-extension
     instructions apply directly: they read a 128-bit vector, select its least
     or most significant half via the mostsig modifier -- exactly HI_P -- widen
     those lanes and write a 128-bit result.  This used to bottom out at 64
     bits on the inherited sxl/sxm and zxl/zxm pairs, which LVX does not have.  */
  rtx mostsig = gen_rtx_CONST_STRING (VOIDmode, hi_p ? ".m" : "");

  switch (op0_mode)
    {
    case E_V8HImode:
      emit_insn (signed_p ? gen_lvx_widensbho (op0, op1, mostsig)
			  : gen_lvx_widenzbho (op0, op1, mostsig));
      break;
    case E_V4SImode:
      emit_insn (signed_p ? gen_lvx_widenshwq (op0, op1, mostsig)
			  : gen_lvx_widenzhwq (op0, op1, mostsig));
      break;
    case E_V2DImode:
      emit_insn (signed_p ? gen_lvx_widenswdp (op0, op1, mostsig)
			  : gen_lvx_widenzwdp (op0, op1, mostsig));
      break;
    case E_V8SImode:
    case E_V16SImode:
    case E_V16HImode:
    case E_V32HImode:
    case E_V4DImode:
    case E_V8DImode:
      {
	rtx op0_half_lo =
	  simplify_gen_subreg (op0_half_mode, op0, op0_mode, 0);
	rtx op0_half_hi = simplify_gen_subreg (op0_half_mode, op0, op0_mode,
					       op0_mode_size / 2);
	rtx op1_half = simplify_gen_subreg (op1_half_mode, op1, op1_mode,
					    hi_p ? op1_mode_size / 2 : 0);
	lvx_expand_unpack (op0_half_lo, op1_half, signed_p, /*hi_p= */ 0);
	lvx_expand_unpack (op0_half_hi, op1_half, signed_p, /*hi_p= */ 1);
	break;
      }
    default:
      gcc_unreachable ();
    }

  return true;
}

/* Expand the specific case where mul is used to splat the memset `int c` arg
 * into a 128-bit or 256-bit value so that the corresonding stores can be used.
 * Cannot enable this for SImode or DImode as the mul could be computing the
 * argument values, because the currently_expanding_gimple_stmt is already set.
 */
bool
lvx_expand_memset_mul (rtx *operands, machine_mode mode)
{
  /* The splat below is SPLATBQ, an lvx-2 instruction: on lvx-1 the
     multiply stays a multiply (umulditi3 + madddidi4 in multi3).  */
  if (!LVX_2
      || GET_MODE_SIZE (mode) <= UNITS_PER_WORD
      || !currently_expanding_gimple_stmt
      || !gimple_call_builtin_p (currently_expanding_gimple_stmt,
				 BUILT_IN_MEMSET))
    return false;

  if (CONST_WIDE_INT_P (operands[2]))
    {
      HOST_WIDE_INT v8qi_ones = 0x0101010101010101LL;
      int nunits = CONST_WIDE_INT_NUNITS (operands[2]);
      for (int i = 0; i < nunits; i++)
	if (CONST_WIDE_INT_ELT (operands[2], i) != v8qi_ones)
	  return false;
    }
  else
    return false;

  rtx op0[MOVE_MAX / UNITS_PER_WORD];
  int nwords = GET_MODE_SIZE (mode) / UNITS_PER_WORD;

  /* If this assert is not verified, calling LVX_EXPAND_CHUNK_SPLAT on OP0[0] is
     undefined behavior.  */
  gcc_assert (nwords >= 1);

  for (int i = 0; i < nwords; i++)
    op0[i] = simplify_gen_subreg (DImode, operands[0], mode, i * UNITS_PER_WORD);

  rtx op1 = simplify_gen_subreg (DImode, operands[1], mode, 0);
  lvx_expand_chunk_splat (op0[0], op1, QImode);
  for (int i = 1; i < nwords; i++)
    emit_move_insn (op0[i], op0[0]);

  return true;
}

/* Expanders }}} */

/* Load-Store Packing {{{ */

/* Splits X as a base + offset. Returns true if split successful,
   false if not. BASE_OUT and OFFSET_OUT contain the corresponding
   split. If STRICT is false, base is not always a register.
 */
bool
lvx_split_mem (rtx x, rtx *base_out, rtx *offset_out, bool strict)
{
  if (GET_CODE (x) != PLUS && !REG_P (x))
    return false;

  if (strict
      && !((GET_CODE (x) == PLUS && CONST_INT_P (XEXP (x, 1))
	    && REG_P (XEXP (x, 0))) || REG_P (x)))
    return false;

  if (GET_CODE (x) == PLUS && CONST_INT_P (XEXP (x, 1)))
    {
      *base_out = XEXP (x, 0);
      *offset_out = XEXP (x, 1);
      return true;
    }

  *base_out = x;
  *offset_out = const0_rtx;
  return true;
}

#define LVX_MAX_PACKED_LSU (4)
/* Used during peephole to merge consecutive loads/stores.
   Returns TRUE if the merge was successful, FALSE if not.
   NOPS is the number of load/store to consider in OPERANDS array.
 */

/* OPERANDS contains NOPS (set ...) (2 or 4) that must be all load or
   all store.  The sets are checked for correctness wrt packing.
   On success, the function emits the packed instruction and returns
   TRUE. If the packing could not be done, returns FALSE.
 */
bool
lvx_pack_load_store (rtx operands[], int nops)
{
  rtx set_dests[LVX_MAX_PACKED_LSU];
  rtx set_srcs[LVX_MAX_PACKED_LSU];
  rtx sorted_operands[2 * LVX_MAX_PACKED_LSU];

  /* Only:
     ld + ld => lq
     ld + ld + ld + ld => lo

     sd + sd => sq
     sd + sd + sd + sd => so
   */

  if (nops != 2 && nops != 4)
    return false;

  for (int i = 0; i < nops; i++)
    {
      set_dests[i] = operands[2 * i];
      set_srcs[i] = operands[2 * i + 1];
      sorted_operands[2 * i] = sorted_operands[2 * i + 1] = NULL_RTX;
    }

  /* Only for register size accesses */
  for (int i = 0; i < nops; i++)
    if (GET_MODE (set_dests[i]) != DImode)
      return false;

  bool is_load = false;
  bool is_store = false;
  for (int i = 0; i < nops; i++)
    if (MEM_P (set_srcs[i]) && REG_P (set_dests[i]))
      is_load = true;
    else if (MEM_P (set_dests[i]) && REG_P (set_srcs[i]))
      is_store = true;

  if ((is_store && is_load) || !(is_load || is_store))
    return false;

  /* Used to pick correct operands in both cases (load and store) */
  int op_offset = is_load ? 0 : 1;

  unsigned int min_regno = REGNO (operands[op_offset]);

  /* Find first regno for destination (load)/source (store) */
  for (int i = 1; i < nops; i++)
    if (REGNO (operands[i * 2 + op_offset]) < min_regno)
      min_regno = REGNO (operands[i * 2 + op_offset]);

  /* Sort operands based on regno */
  for (int i = 0; i < nops; i++)
    {
      const int regno = REGNO (operands[i * 2 + op_offset]);
      const int idx = 2 * (regno - min_regno);

      /* Registers are not consecutive */
      if (idx >= (2 * nops))
	return false;

      /* Register used twice in operands */
      if (sorted_operands[idx] != NULL_RTX)
	return false;

      sorted_operands[idx] = operands[2 * i];
      sorted_operands[idx + 1] = operands[2 * i + 1];
    }

  /* Check mem addresses are consecutive */
  rtx base_reg, base_offset;
  if (!lvx_split_mem (XEXP (sorted_operands[1 - op_offset], 0), &base_reg,
		      &base_offset, true))
    return false;

  const unsigned int base_regno = REGNO (base_reg);

  /* Base register is modified by one load */
  if (is_load && base_regno >= REGNO (sorted_operands[op_offset])
      && base_regno <= REGNO (sorted_operands[(nops - 1) * 2 + op_offset]))
    {
      bool mod_before_last = false;
      /* Check the base register is modified in the last load */
      for (int i = 0; i < (nops - 1); i++)
	{
	  if (REGNO (operands[2 * i + op_offset]) == base_regno)
	    {
	      mod_before_last = true;
	      break;
	    }
	}
      if (mod_before_last)
	return false;
    }

  unsigned int next_offset = INTVAL (base_offset) + UNITS_PER_WORD;
  for (int i = 1; i < nops; i++)
    {
      rtx elem = XEXP (sorted_operands[2 * i + 1 - op_offset], 0);

      /* Not addressing next memory word */
      const bool is_plus_bad_offset
	= GET_CODE (elem) == PLUS
	&& (!REG_P (XEXP (elem, 0)) || REGNO (XEXP (elem, 0)) != base_regno
	    || !CONST_INT_P (XEXP (elem, 1))
	    || INTVAL (XEXP (elem, 1)) != next_offset);

      const bool is_reg_bad
	= REG_P (elem) && (REGNO (elem) != base_regno || next_offset != 0);

      if (is_reg_bad || is_plus_bad_offset)
	return false;

      next_offset += UNITS_PER_WORD;
    }

  rtx multi_insn;
  if (is_load)
    multi_insn = gen_load_multiple (sorted_operands[0], sorted_operands[1],
				    GEN_INT (nops));
  else
    multi_insn = gen_store_multiple (sorted_operands[0], sorted_operands[1],
				     GEN_INT (nops));
  if (multi_insn == NULL_RTX)
    return false;

  emit_insn (multi_insn);
  return true;
}
#undef LVX_MAX_PACKED_LSU

/* Load-Store Packing }}} */


/* Position Independent Code (PIC) Support {{{ */

/* Returns TRUE if OP is (const (unspec ([] UNSPEC_*))) or
   (unspec ([] UNSPEC_*)) with unspec type compatible with PIC code.  */
bool
lvx_legitimate_pic_symbolic_ref_p (rtx op)
{
  /* Unwrap CONST */
  if (GET_CODE (op) == CONST)
    op = XEXP (op, 0);

  /* Unwrap PLUS */
  if (GET_CODE (op) == PLUS && CONST_INT_P (XEXP (op, 1)))
    op = XEXP (op, 0);

  /* Valid ref are wrapped in UNSPEC */
  if (GET_CODE (op) != UNSPEC)
    return false;

  int unspec = XINT ((op), 1);
  return (unspec == UNSPEC_GOT || unspec == UNSPEC_GOTOFF
	  || unspec == UNSPEC_TLS_DTPOFF || unspec == UNSPEC_TLS_GD
	  || unspec == UNSPEC_TLS_LD || unspec == UNSPEC_TLS_LE
	  || unspec == UNSPEC_TLS_IE || unspec == UNSPEC_PCREL);
}


static rtx
gen_add2_pcrel_insn (rtx dest, rtx pcrel_offset)
{
  return gen_add_pcrel_di (dest, pcrel_offset);
}

/* Expands a mov which second operand is a constant. */
void
lvx_expand_mov_constant (rtx operands[])
{
  rtx dest = operands[0];
  rtx src = operands[1];
  rtx new_rtx;

  if (GET_CODE (src) == SYMBOL_REF || GET_CODE (src) == LABEL_REF
      || GET_CODE (src) == CONST)
    {
      rtx base, offset;
      enum lvx_symbol_type sty;
      rtx pic_reg;

      /* If we have (const (plus symbol offset)), separate out the offset
         before we start classifying the symbol.  */
      split_const (src, &base, &offset);

      sty = lvx_classify_symbol (base);
      switch (sty)
	{
	case SYMBOL_ABSOLUTE:
	case LABEL_ABSOLUTE:
	  /* Emit: dest = sym */
	  emit_insn (gen_rtx_SET (dest, src));
	  break;

	case LABEL_PCREL_ABSOLUTE:
	  /* Emit dest = pc + @pcrel(label) */
	  emit_insn (gen_add2_pcrel_insn (dest, src));
	  break;

	case SYMBOL_GOT:
	  /*
	   * Emit dest = *(@got(sym) + $pic)
	   */
	  pic_reg = gen_reg_rtx (Pmode);
	  emit_insn (gen_set_gotp_insn (pic_reg));

	  new_rtx
	    = gen_rtx_CONST (Pmode, gen_rtx_UNSPEC (Pmode, gen_rtvec (1, base),
						    UNSPEC_GOT));

	  emit_move_insn (dest, pic_reg);

	  emit_move_insn (dest, gen_rtx_MEM (Pmode, gen_rtx_PLUS (Pmode, dest,
								  new_rtx)));
	  if (INTVAL (offset) != 0)
	    emit_insn (gen_add2_insn (dest, offset));

	  crtl->uses_pic_offset_table = true;
	  break;

	case SYMBOL_GOTOFF:
	  /*
	   * Emit dest = @gotoff(sym)[$pic]
	   */
	  pic_reg = gen_reg_rtx (Pmode);
	  emit_insn (gen_set_gotp_insn (pic_reg));

	  new_rtx
	    = gen_rtx_CONST (Pmode, gen_rtx_UNSPEC (Pmode, gen_rtvec (1, base),
						    UNSPEC_GOTOFF));

	  emit_move_insn (dest, pic_reg);

	  emit_move_insn (dest, gen_rtx_PLUS (Pmode, dest, new_rtx));

	  if (INTVAL (offset) != 0)
	    emit_insn (gen_add2_insn (dest, offset));

	  crtl->uses_pic_offset_table = true;
	  break;

	case SYMBOL_TLSIE:
	case SYMBOL_TLSLD:
	case SYMBOL_TLSGD:
	case SYMBOL_TLSLE:
	  operands[1] = lvx_legitimize_tls_reference (base);
	  gcc_assert (operands[1] != src);

	  emit_insn (gen_rtx_SET (dest, operands[1]));

	  if (INTVAL (offset) != 0)
	    emit_insn (gen_add2_insn (dest, offset));

	  break;

	default:
	  gcc_unreachable ();
	}
      return;
    }

  emit_insn (gen_rtx_SET (dest, src));

  /* if (lvx_has_tls_reference (operands[1])) */
  /*   { */
  /*     rtx src = operands[1]; */
  /*     operands[1] = lvx_legitimize_tls_reference (src); */
  /*     gcc_assert (operands[1] != src); */
  /*   } else if (flag_pic) */
  /*   { */
  /*     if (SYMBOLIC_CONST(operands[1])) */
  /*    operands[1] = lvx_legitimize_pic_address (operands[1],
   * operands[0]); */
  /*   } */
  return;
}

/* Position Independent Code (PIC) Support }}} */

/* Atomics {{{ */

/* Emit a barrier, that is appropriate for memory model MODEL, at the
   start of a sequence implementing an atomic operation.  */
void
lvx_emit_pre_barrier (rtx model)
{
  const enum memmodel mm = memmodel_from_int (INTVAL (model));
  switch (mm & MEMMODEL_BASE_MASK)	// treat sync operations as atomic ones
    {
    case MEMMODEL_RELAXED:
    case MEMMODEL_CONSUME:
    case MEMMODEL_ACQUIRE:
      // No pre barrier is required for RELAXED, CONSUME, and ACQUIRE.
      break;
    case MEMMODEL_RELEASE:
    case MEMMODEL_ACQ_REL:
      // FENCE.W.
      {
	rtx modifier = gen_rtx_CONST_STRING (VOIDmode, ".w");
	emit_insn (gen_lvx_fence (modifier));
	break;
      }
    case MEMMODEL_SEQ_CST:
      // FENCE.
      {
	rtx modifier = gen_rtx_CONST_STRING (VOIDmode, "");
	emit_insn (gen_lvx_fence (modifier));
	break;
      }
    default:
      gcc_unreachable ();
    }
}

/* Emit a barrier, that is appropriate for memory model MODEL, at the
   end of a sequence implementing an atomic operation.  */
void
lvx_emit_post_barrier (rtx model)
{
  const enum memmodel mm = memmodel_from_int (INTVAL (model));
  switch (mm & MEMMODEL_BASE_MASK)	// treat sync operations as atomic ones
    {
    case MEMMODEL_RELAXED:
    case MEMMODEL_RELEASE:
      // No post barrier is required for RELAXED, and ACQUIRE .
      break;
    case MEMMODEL_ACQ_REL:
      /* FENCE.R, the acquire half.  This used to emit only lvx_mem_clobber --
	 a compiler-level blockage and no instruction -- so an __ATOMIC_ACQ_REL
	 read-modify-write came out as `fence.w; aladdw' and carried release
	 ordering but NOT acquire: nothing stopped a later load from being
	 observed before the atomic.  The pre-barrier already emits FENCE.W for
	 this model, and FENCE.R is what the other half needs, exactly as the
	 plain ACQUIRE case below emits it.  Single-threaded testing cannot see
	 this, which is presumably how it survived.  */
      {
	rtx modifier = gen_rtx_CONST_STRING (VOIDmode, ".r");
	emit_insn (gen_lvx_fence (modifier));
	break;
      }
    case MEMMODEL_SEQ_CST:
      // FENCE.
      {
	rtx modifier = gen_rtx_CONST_STRING (VOIDmode, "");
	emit_insn (gen_lvx_fence (modifier));
	break;
      }
    case MEMMODEL_ACQUIRE:
    case MEMMODEL_CONSUME:
      // FENCE.R.
      {
	rtx modifier = gen_rtx_CONST_STRING (VOIDmode, ".r");
	emit_insn (gen_lvx_fence (modifier));
	break;
      }
    default:
      gcc_unreachable ();
    }
}

/* Expand a compare and swap pattern.  */

void
lvx_expand_compare_and_swap (rtx ret_, rtx before_, rtx mem_, rtx expected_,
			     rtx desired_, rtx weak_ ATTRIBUTE_UNUSED,
			     rtx mm_succ_, rtx mm_fail_ ATTRIBUTE_UNUSED)
{
  machine_mode mode = GET_MODE (mem_);
  /* The instruction reads a register PAIR holding the new value and the
     expected one, and writes its result over the pair's low half; at 128 bits
     that is a quad holding two pairs.  */
  machine_mode ret_mode = mode == TImode ? OImode : TImode;
  machine_mode half_mode = mode == TImode ? TImode : DImode;
  rtx (*acswap) (rtx, rtx, rtx, rtx);

  gcc_assert (mode == QImode || mode == HImode || mode == SImode
	      || mode == DImode || mode == TImode);

  switch (mode)
    {
    case E_QImode: acswap = gen_lvx_acswapb; break;
    case E_HImode: acswap = gen_lvx_acswaph; break;
    case E_SImode: acswap = gen_lvx_acswapw; break;
    case E_DImode: acswap = gen_lvx_acswapd; break;
    case E_TImode: acswap = gen_lvx_acswapq; break;
    default: gcc_unreachable ();
    }

  rtx tmp = gen_reg_rtx (ret_mode);
  rtx low = gen_lowpart (half_mode, tmp);
  /* GEN_HIGHPART would assert, TImode being wider than UNITS_PER_WORD.  */
  int high_offset = subreg_highpart_offset (half_mode, ret_mode);
  rtx high = simplify_gen_subreg (half_mode, tmp, ret_mode, high_offset);

  /* Compare against our own copy of EXPECTED rather than against the pair's
     high half: the insn declares the whole pair read-write, so nothing
     entitles us to assume the half it does not write survives.  */
  rtx expected = force_reg (mode, expected_);
  rtx keep = gen_reg_rtx (mode);
  emit_move_insn (keep, expected);

  lvx_emit_pre_barrier (mm_succ_);

  /* The pair, in the order the instruction reads it: the new value in the low
     half, the expected one in the high half.  MEM_atomic_cas takes `newval'
     before `expected', and arguments.32[0] is the low register.  */
  emit_move_insn (gen_lowpart (mode, low), force_reg (mode, desired_));
  emit_move_insn (gen_lowpart (mode, high), expected);

  emit_insn (acswap (tmp, mem_, gen_rtx_CONST_STRING (VOIDmode, ".V"),
		     const0_rtx));

  /* `.V', the VALUED flavour: the result is the memory's previous contents, so
     the low half now holds the old value.  The bare spelling would return a
     success flag instead -- which is what this port used to ask for, and why
     it needed a retry loop to tell a real failure from a stale compare.  A
     valued compare-and-swap cannot fail spuriously, so success is one
     comparison and WEAK_ makes no difference: a weak CAS and a strong one are
     the same instruction here.  That is also why MM_FAIL_ is unused -- with no
     branch there is nowhere to put a weaker barrier, and the standard requires
     the failure ordering to be no stronger than the success ordering, so the
     one emitted covers both.  */
  lvx_emit_post_barrier (mm_succ_);

  rtx old = gen_lowpart (mode, low);

  /* BEFORE_ is the memory's contents before the attempt, which is exactly what
     came back.  Take it before RET_, since computing the flag may clobber.  */
  emit_move_insn (before_, old);

  /* RET_ is true when the swap happened, i.e. when the old value was the
     expected one.  */
  rtx flag = emit_store_flag_force (gen_reg_rtx (SImode), EQ,
				    copy_to_reg (old), keep, mode, 0, 0);
  emit_move_insn (ret_, gen_lowpart (SImode, flag));
}

/* The native read-modify-write instruction for CODE at MODE, or CODE_FOR_nothing
   when the ISA has none.  FETCH selects the returning form (al<op>, whose old
   value comes back in the value register) over the discarding one (as<op>).

   add, and, ior and xor are the four the ISA performs directly -- the same set
   riscv gets from amoadd/amoand/amoor/amoxor.  FETCH false returns nothing: GCC
   always expands through the fetch optab and lets DCE drop an unused result, so
   atomic_<op><mode> is never chosen and the as<op> forms would be unreachable
   from here.  They are builtins instead (lvx_as<op><size>).  A subtract is an add of the
   negation, handled by the caller; nand has no instruction and falls through
   to the compare-and-swap loop.  Every one of them exists at QI and HI as well
   as SI and DI, which is why no subword masking path is needed here.  */
static enum insn_code
lvx_amo_code (enum rtx_code code, machine_mode mode, bool fetch)
{
  switch (mode)
    {
#define LVX_AMO_WIDTH(M, sfx)						\
    case E_##M##mode:							\
      switch (code)							\
	{								\
	case PLUS: return fetch ? CODE_FOR_aladd##sfx##_amo		\
				: CODE_FOR_nothing;			\
	case AND:  return fetch ? CODE_FOR_aland##sfx##_amo		\
				: CODE_FOR_nothing;			\
	case IOR:  return fetch ? CODE_FOR_alior##sfx##_amo		\
				: CODE_FOR_nothing;			\
	case XOR:  return fetch ? CODE_FOR_aleor##sfx##_amo		\
				: CODE_FOR_nothing;			\
	/* SET is atomic_exchange: aswap returns the old value, so it is one	\
	   instruction and not a compare-and-swap loop.  */			\
	case SET:  return fetch ? CODE_FOR_lvx_aswap##sfx		\
				: CODE_FOR_nothing;			\
	default:   return CODE_FOR_nothing;				\
	}
    LVX_AMO_WIDTH (QI, b)
    LVX_AMO_WIDTH (HI, h)
    LVX_AMO_WIDTH (SI, w)
    LVX_AMO_WIDTH (DI, d)
#undef LVX_AMO_WIDTH
    default:
      return CODE_FOR_nothing;
    }
}

/* Expand an atomic read-modify-write.  Returns false when this port has no way
   to do it, which the caller turns into FAIL so the middle end emulates: that
   is nand at a byte or half word, and an exchange at those widths, neither
   having an instruction and the compare-and-swap loop below working on words
   and double words only.  Declining is better than widening the loop, because
   the middle end's emulation is correct and this port's loop would have to
   grow the mask-and-shift machinery that having native b/h instructions is
   precisely what spares us.  */
/* MEM is the memory location to operate on with the value in VAL.  If AFTER is
   true, TARGET receives the value held after the operation, otherwise the one
   held before it; a NULL TARGET discards it, which is what selects the
   non-returning as<op> over al<op>.

   The old comment here said "only for SImode and DImode (CAS loop is too
   difficult to handle for HI, QI, and TI modes)".  That was the KV3 position
   and it is no longer true: a byte and a half word have instructions of their
   own, so they take the one-instruction path above and never reach the loop.  */
bool
lvx_expand_atomic_op (enum rtx_code code, rtx target, bool after, rtx mem,
		      rtx val, rtx model)
{
  machine_mode mode = GET_MODE (mem);

  /* A subtract is an add of the negation: the ISA has no alsbf, and this is
     what riscv does with amoadd too.  Fold it before looking for an
     instruction, so `sub' gets the one-instruction path as well.  */
  if (code == MINUS && CONST_INT_P (val))
    {
      val = gen_int_mode (-UINTVAL (val), mode);
      code = PLUS;
    }
  else if (code == MINUS)
    {
      val = expand_simple_unop (mode, NEG, val, NULL_RTX, 1);
      code = PLUS;
    }

  /* The one-instruction path.  `after' asks for the value AFTER the operation,
     which the instruction does not give: al<op> returns the old value, so
     recompute the new one from it with a single ALU op -- again what riscv
     does.  */
  {
    enum insn_code icode = lvx_amo_code (code, mode, target != NULL_RTX);
    if (icode != CODE_FOR_nothing)
      {
	rtx modifier = gen_rtx_CONST_STRING (VOIDmode, "");
	lvx_emit_pre_barrier (model);
	if (target == NULL_RTX)
	  emit_insn (GEN_FCN (icode) (mem, force_reg (mode, val), modifier,
				      NULL_RTX));
	else
	  {
	    rtx old = gen_reg_rtx (mode);
	    emit_move_insn (old, force_reg (mode, val));
	    emit_insn (GEN_FCN (icode) (old, mem, old, modifier));
	    if (after && code == SET)
	      gcc_unreachable ();	/* atomic_exchange is always `before'.  */
	    else if (after)
	      {
		/* The instruction returns the value held BEFORE the operation,
		   so the `_fetch' forms recompute the new one -- one ALU op,
		   as riscv does after an amo.  OPTAB_WIDEN, not OPTAB_DIRECT:
		   at QI and HI this port has no arithmetic optab of its own
		   and expand_simple_binop returns NULL for them, which then
		   reached emit_move_insn and segfaulted the compiler on
		   __atomic_add_fetch of a char.  Widening is what the scalar
		   code does for those modes anyway.  */
		rtx res = expand_simple_binop (mode, code, old,
					       force_reg (mode, val),
					       NULL_RTX, 1, OPTAB_WIDEN);
		gcc_assert (res != NULL_RTX);
		emit_move_insn (target, res);
	      }
	    else
	      emit_move_insn (target, old);
	  }
	lvx_emit_post_barrier (model);
	return true;
      }
    if (mode != SImode && mode != DImode)
      return false;
  }

  rtx csloop = gen_label_rtx ();
  rtx tmp = gen_reg_rtx (TImode);
  rtx ret = gen_reg_rtx (mode);
  rtx new_mem_val = gen_lowpart (DImode, tmp);
  rtx curr_mem_val = gen_highpart (DImode, tmp);
  rtx (*gen) (rtx, rtx, rtx);
  rtx op_res, op_res_copy = NULL_RTX;

  if (target && after)
    op_res_copy = gen_reg_rtx (mode);

  /* Everything that reaches the compare-and-swap loop does so because the ISA
     has no instruction for it: nand, and any mode the AMO table lacks.  The
     loop itself works on words and double words only.  */
  gcc_assert ((mode == SImode || mode == DImode));

  emit_label (csloop);		/* cas loop entry point */
  /* copy memory content to perform op on it (atomic uncached load) */
  gen = mode == SImode ? gen_atomic_loadsi : gen_atomic_loaddi;
  emit_insn (gen
	     (gen_lowpart (mode, curr_mem_val), mem,
	      GEN_INT (MEMMODEL_RELAXED)));

  /* Perform operation in a cas loop, we do not need to convert
     CURR_MEM_VAL, NEW_MEM_VAL (DImode) and VAL (SImode or
     DImode). expand_simple_binop () is smart enough to select the right
     insn depending on MODE.  */
  switch (code)
    {
    case PLUS:
    case IOR:
    case XOR:
    case MINUS:
    case AND:
      op_res =
	expand_simple_binop (mode, code, curr_mem_val, val, new_mem_val, 1,
			     OPTAB_LIB_WIDEN);
      break;
    case MULT:			/* NAND */
      {
	rtx x =
	  expand_simple_binop (mode, AND, curr_mem_val, val, NULL_RTX, 1,
			       OPTAB_LIB_WIDEN);
	op_res = expand_simple_unop (mode, NOT, x, new_mem_val, 1);
	break;
      }
    case SET:
      op_res = gen_reg_rtx (mode);
      emit_move_insn (op_res, val);
      break;
    default:
      gcc_unreachable ();
    }

  if (op_res != gen_lowpart (mode, new_mem_val))
    emit_move_insn (gen_lowpart (mode, new_mem_val), op_res);

  /* Save OP_RES when returning it (the result of the operation),
     because ACSWAP will erase it. */
  if (target && after)
    emit_move_insn (op_res_copy, op_res);

  /* Handle pre fence right before acswap. */
  lvx_emit_pre_barrier (model);

  /* Update memory with op result iff memory hasn't been modified since:
     if CURR_MEM_VAL == MEM then update MEM with NEW_MEM_VAL else try again. */
  rtx modifier = gen_rtx_CONST_STRING (VOIDmode, "");
  emit_insn (mode == SImode ? gen_lvx_acswapw (tmp, mem, modifier, const0_rtx)
			    : gen_lvx_acswapd (tmp, mem, modifier, const0_rtx));

  /* Handle post fence right after acswap. */
  lvx_emit_post_barrier (model);

  /* ACSWAP insn returns 0x0 (fail) or 0x1 (success) in the low part of TMP:
     - if successful: MEM is updated, do not loop
     - if failing: MEM has changed, try again */
  emit_cmp_and_jump_insns (gen_lowpart (mode, new_mem_val), const1_rtx, NE,
			   NULL_RTX, mode, true, csloop);

  /* In addition to modifying MEM, return a value if needed */
  if (target)
    {
      ret = (after
	       /* returning op result */
	       ? op_res_copy
	       /* returning mem content before op */
	       : gen_lowpart (mode, curr_mem_val));
      emit_move_insn (target, ret);
    }
  return true;
}

/* Expand the atomic test-and-set on byte atomic operation using the
   acswapw insn. */

void
lvx_expand_atomic_test_and_set (rtx operands[])
{
  rtx mem = operands[1];
  rtx model = operands[2];

  rtx tmp = gen_reg_rtx (TImode);
  rtx oldval = gen_lowpart (SImode, (gen_highpart (DImode, tmp)));
  rtx newval = gen_lowpart (SImode, (gen_lowpart (DImode, tmp)));
  rtx (*and3) (rtx, rtx, rtx) = Pmode == SImode ? gen_andsi3 : gen_anddi3;
  rtx (*mul3) (rtx, rtx, rtx) = Pmode == SImode ? gen_mulsi3 : gen_muldi3;

  /* Mask POINTER to get the word ADDRESS := POINTER & -4. */
  rtx pointer = gen_reg_rtx (Pmode);
  emit_move_insn (pointer, XEXP (mem, 0));
  rtx address = gen_reg_rtx (Pmode);
  emit_insn (and3 (address, pointer, GEN_INT (-4)));
  rtx memsi = gen_rtx_MEM (SImode, address);

  /* Boolean CAS loop entry point: load OLDVAL from word ADDRESS. */
  rtx retry = gen_label_rtx ();
  emit_label (retry);
  emit_insn (gen_atomic_loadsi (oldval, memsi, GEN_INT (MEMMODEL_RELAXED)));

  /* Byte SHIFT := ((POINTER & 0x3) * BITS_PER_UNIT). */
  rtx shift = gen_reg_rtx (Pmode);
  emit_insn (and3 (shift, pointer, GEN_INT (0x3)));
  emit_insn (mul3 (shift, shift, GEN_INT (BITS_PER_UNIT)));

  /* Byte to test-and-set is BYTE := OLDVAL >> SHIFT & 0xFF. */
  rtx byte = gen_reg_rtx (SImode);
  emit_insn (gen_lshrsi3 (byte, oldval, gen_lowpart (SImode, shift)));
  emit_insn (gen_andsi3 (byte, byte, GEN_INT (0xFF)));

  /* If BYTE is false, try a compare-and-swap with the byte set to TRUE.
     Else return true (i.e. BYTE) because the lock is already acquired. */
  rtx fini = gen_label_rtx ();
  emit_cmp_and_jump_insns (byte, const0_rtx, NE, NULL_RTX, SImode, true,
			   fini);

  /* Build NEWVAL := OLDVAL | TRUE << SHIFT */
  rtx mask = gen_reg_rtx (SImode);
  emit_move_insn (mask, const1_rtx);
  emit_insn (gen_ashlsi3 (mask, mask, gen_lowpart (SImode, shift)));
  emit_insn (gen_iorsi3 (newval, oldval, mask));

  /* Handle pre fence right before acswap. */
  lvx_emit_pre_barrier (model);

  rtx modifier = gen_rtx_CONST_STRING (VOIDmode, "");
  emit_insn (gen_lvx_acswapw (tmp, memsi, modifier, const0_rtx));

  /* Handle post fence right after acswap. */
  lvx_emit_post_barrier (model);

  /* ACSWAP returns 0x0 (fail) or 0x1 (success) in the low part of TMP:
     - if successful: MEM is updated, do not loop,
       lock is acquired, return false (i.e. BYTE)
     - if failing: MEM has changed, try again */
  emit_cmp_and_jump_insns (newval, const1_rtx, NE, NULL_RTX, SImode, true,
			   retry);

  emit_label (fini);
  emit_move_insn (operands[0], gen_lowpart (QImode, byte));
}

/* Atomics }}} */

/* Predicates {{{ */

/* Returns TRUE if OP is a symbol and has the farcall attribute or if
   -mfarcall is in use. */
bool
lvx_is_farcall_p (rtx op)
{
  bool farcall = LVX_FARCALL;
  if (!farcall
      && (GET_CODE (XEXP (op, 0)) == SYMBOL_REF
	  && SYMBOL_REF_FUNCTION_P (XEXP (op, 0))
	  && SYMBOL_REF_DECL (XEXP (op, 0)) != NULL_TREE))
    farcall
      = lookup_attribute ("farcall",
			  DECL_ATTRIBUTES (SYMBOL_REF_DECL (XEXP (op, 0))))
	!= NULL;
  return farcall;
}

/*
 * When IS_LOAD is TRUE, returns TRUE if OP is a load multiple operation whose
 * mems all share one address space.  They have to: a single LO issues one
 * access, so it cannot be half cached, and the variant it prints is read off
 * that shared address space.
 * When IS_LOAD is FALSE, returns TRUE if OP is a store multiple operation.
 * Returns FALSE otherwise.
 */
static bool
lvx_load_store_multiple_operation_p (rtx op, bool is_load)
{
  int count = XVECLEN (op, 0);
  addr_space_t addr_space = ADDR_SPACE_GENERIC;

  /* Perform a quick check so we don't blow up below.  */
  if (count != 2 && count != 4)
    return 0;

  for (int i = 0; i < count; i++)
    {
      rtx set = XVECEXP (op, 0, i);
      if (GET_CODE (set) != SET)
	return false;

      rtx reg_part = is_load ? SET_DEST (set) : SET_SRC (set);
      rtx mem_part = is_load ? SET_SRC (set) : SET_DEST (set);

      if (!REG_P (reg_part) || !MEM_P (mem_part) || MEM_VOLATILE_P (mem_part))
	return false;

      if (is_load)
	{
	  if (i == 0)
	    addr_space = MEM_ADDR_SPACE (mem_part);
	  else if (addr_space != MEM_ADDR_SPACE (mem_part))
	    return false;
	}
    }

  rtx first_mem
    = is_load ? SET_SRC (XVECEXP (op, 0, 0)) : SET_DEST (XVECEXP (op, 0, 0));
  rtx first_reg
    = is_load ? SET_DEST (XVECEXP (op, 0, 0)) : SET_SRC (XVECEXP (op, 0, 0));

  if (TARGET_STRICT_ALIGN
      && ((int) MEM_ALIGN (first_mem)) < (count * UNITS_PER_WORD))
    return false;

  unsigned int dest_regno = REGNO (first_reg);

  /* register number must be correctly aligned */
  if (dest_regno < FIRST_PSEUDO_REGISTER && (dest_regno % count != 0))
    return false;

  HOST_WIDE_INT expected_offset = 0;
  rtx base = NULL_RTX;

  for (int i = 0; i < count; i++)
    {
      rtx elt = XVECEXP (op, 0, i);
      rtx base_cur, offset_cur;
      rtx mem_elt = is_load ? SET_SRC (elt) : SET_DEST (elt);
      rtx reg_elt = is_load ? SET_DEST (elt) : SET_SRC (elt);

      if (!lvx_split_mem (XEXP (mem_elt, 0), &base_cur, &offset_cur, false))
	return false;

      if (i == 0)
	{
	  expected_offset = INTVAL (offset_cur);
	  base = base_cur;
	}
      else
	{
	  expected_offset += UNITS_PER_WORD;
	}

      if (GET_MODE (reg_elt) != DImode || REGNO (reg_elt) != dest_regno + i
	  || GET_MODE (mem_elt) != DImode || !rtx_equal_p (base_cur, base)
	  || expected_offset != INTVAL (offset_cur))

	return false;
    }

  return true;
}

/*
 * Returns TRUE if OP is a load multiple operation.
 */
bool
lvx_load_multiple_operation_p (rtx op)
{
  return lvx_load_store_multiple_operation_p (op, true);
}

/*
 * Returns TRUE if OP is a store multiple operation.
 */
bool
lvx_store_multiple_operation_p (rtx op)
{
  return lvx_load_store_multiple_operation_p (op, false);
}

bool
lvx_syscall_addrspace_p (rtx x)
{
  gcc_assert (MEM_P (x));
  if (!MEM_P (x))
    return false;

  return MEM_ADDR_SPACE (x) == LVX_ADDR_SPACE_SYSCALL;
}

/* Predicates }}} */

/* Constraints {{{ */

bool
lvx_float_fits_bits (const REAL_VALUE_TYPE *r, unsigned bitsz,
		     enum machine_mode mode)
{
  if (bitsz >= 64)
    return true;

  long l[2];
  unsigned long long value = 0;
  if (mode == HFmode)
    {
      REAL_VALUE_TO_TARGET_SINGLE (*r, l[0]);
      value = lvx_float_to_half_as_int (l[0]);
    }
  else if (mode == SFmode)
    {
      REAL_VALUE_TO_TARGET_SINGLE (*r, l[0]);
      value = l[0] & 0xFFFFFFFFULL;
    }
  else if (mode == DFmode)
    {
      REAL_VALUE_TO_TARGET_DOUBLE (*r, l);
      value = (l[0] & 0xFFFFFFFFULL) | ((unsigned long long) l[1] << 32);
    }
  else if (mode == HCmode)
    {
      REAL_VALUE_TO_TARGET_SINGLE (*r, l[0]);
      value = l[0] & 0xFFFFFFFFULL;
    }
  else if (mode == SCmode)
    {
      REAL_VALUE_TO_TARGET_SINGLE (*r, l[0]);
      REAL_VALUE_TO_TARGET_SINGLE (*r, l[1]);
      value = (l[0] & 0xFFFFFFFFULL) | ((unsigned long long) l[1] << 32);
    }
  else if (mode == DCmode)
    return false;
  else
    gcc_unreachable ();

  return SIGNED_INT_FITS_N_BITS (value, bitsz);
}

bool
lvx_has_10bit_vector_const_p (rtx x)
{
  HOST_WIDE_INT value = lvx_const_vector_value (x, 0);
  return SIGNED_INT_FITS_N_BITS (value, 10);
}

bool
lvx_has_16bit_vector_const_p (rtx x)
{
  HOST_WIDE_INT value = lvx_const_vector_value (x, 0);
  return SIGNED_INT_FITS_N_BITS (value, 16);
}

bool
lvx_has_32bit_vector_const_p (rtx x)
{
  HOST_WIDE_INT value = lvx_const_vector_value (x, 0);
  return SIGNED_INT_FITS_N_BITS (value, 32);
}

bool
lvx_has_37bit_vector_const_p (rtx x)
{
  HOST_WIDE_INT value = lvx_const_vector_value (x, 0);
  return SIGNED_INT_FITS_N_BITS (value, 37);
}

bool
lvx_has_43bit_vector_const_p (rtx x)
{
  HOST_WIDE_INT value = lvx_const_vector_value (x, 0);
  return SIGNED_INT_FITS_N_BITS (value, 43);
}

bool
lvx_splat32_const_p (rtx x)
{
  /* The .M formats carry one 32-bit word and the machine replicates it over
     the whole operand, so the constant has to be that word, repeated.  For
     byte, half-word and word lanes any uniform lane constant is; for 64-bit
     lanes only those whose halves repeat, which in practice means zero.  */
  if (GET_CODE (x) == CONST_VECTOR)
    {
      unsigned size = GET_MODE_SIZE (GET_MODE (x));
      if (size != 16 && size != 32)
	return false;

      /* Only the lane modes lvx_const_vector_value has an arm for: it asserts
	 on anything else rather than returning, and this runs from a
	 predicate.  */
      switch (GET_MODE_INNER (GET_MODE (x)))
	{
	case E_QImode: case E_HImode: case E_SImode: case E_DImode:
	case E_HFmode: case E_SFmode: case E_DFmode:
	  break;
	default:
	  return false;
	}

      HOST_WIDE_INT lo = lvx_const_vector_value (x, 0);
      for (unsigned slice = 1; slice < size / 8; slice++)
	if (lo != lvx_const_vector_value (x, slice))
	  return false;
      return ((lo >> 32) & 0xFFFFFFFF) == (lo & 0xFFFFFFFF);
    }

  /* And the same for a scalar: a DImode constant that is one word twice, or a
     TImode one that is four times.  Both carry VOIDmode, so the width comes
     from how many host words the constant needs -- the operand predicate,
     which does know the mode, is the authority.  */
  if (CONST_INT_P (x))
    {
      unsigned HOST_WIDE_INT v = UINTVAL (x);
      return ((v >> 32) & 0xFFFFFFFF) == (v & 0xFFFFFFFF);
    }

  if (CONST_WIDE_INT_P (x))
    {
      unsigned HOST_WIDE_INT lo = CONST_WIDE_INT_ELT (x, 0);
      for (int i = 1; i < CONST_WIDE_INT_NUNITS (x); i++)
	if (lo != (unsigned HOST_WIDE_INT) CONST_WIDE_INT_ELT (x, i))
	  return false;
      return ((lo >> 32) & 0xFFFFFFFF) == (lo & 0xFFFFFFFF);
    }

  return false;
}

/* The 32-bit word a splattable constant is made of.  */
HOST_WIDE_INT
lvx_splat32_value (rtx x)
{
  gcc_assert (lvx_splat32_const_p (x));
  if (GET_CODE (x) == CONST_VECTOR)
    return lvx_const_vector_value (x, 0) & 0xFFFFFFFF;
  if (CONST_WIDE_INT_P (x))
    return CONST_WIDE_INT_ELT (x, 0) & 0xFFFFFFFF;
  return INTVAL (x) & 0xFFFFFFFF;
}

/* Test if X is of the form reg[reg] or .xs reg = reg[reg] or signed10bits[reg]
 */
bool
lvx_has_10bit_imm_or_reg_p (rtx x)
{
  if (MEM_P (x))
    x = XEXP (x, 0);

  if (REG_P (x))
    return true;

  /*
   * ld reg = reg[reg]
   */
  if (GET_CODE (x) == PLUS && REG_P (XEXP (x, 0)) && REG_P (XEXP (x, 1)))
    return true;

  /*
   * ld.xs reg = reg[reg]
   */
  if (HAVE_LVX_ADDRESSING_XS
      && GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 0)) == MULT
      && GET_CODE (XEXP (XEXP (x, 0), 1)) == CONST_INT
      && INTVAL (XEXP (XEXP (x, 0), 1)) > HOST_WIDE_INT_1)
    return true;

  return lvx_has_10bit_immediate_p (x);
}

bool
lvx_has_10bit_immediate_p (rtx x)
{
  if (MEM_P (x))
    x = XEXP (x, 0);

  if (GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 1)) == CONST_INT
      && REG_P (XEXP (x, 0)))
    return IN_RANGE (INTVAL (XEXP (x, 1)), -512, 511);

  return false;
}

bool
lvx_has_37bit_immediate_p (rtx x)
{
  if (MEM_P (x))
    x = XEXP (x, 0);

  if (GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 1)) == CONST_INT
      && REG_P (XEXP (x, 0)))
    return !IN_RANGE (INTVAL (XEXP (x, 1)), -512, 511)
	   && IN_RANGE (INTVAL (XEXP (x, 1)), -(1LL << 36), (1LL << 36) - 1);

  return false;
}

bool
lvx_has_64bit_immediate_p (rtx x)
{
  if (MEM_P (x))
    x = XEXP (x, 0);

  if (GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 1)) == CONST_INT
      && REG_P (XEXP (x, 0)))
    return !IN_RANGE (INTVAL (XEXP (x, 1)), -(1LL << 36), (1LL << 36) - 1);

  if (GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 1)) != CONST_INT
      && REG_P (XEXP (x, 0)))
    return true;

  return false;
}

bool
lvx_has_27bit_immediate_p (rtx x)
{
  if (MEM_P (x))
    x = XEXP (x, 0);

  if (GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 1)) == CONST_INT
      && REG_P (XEXP (x, 0)))
    return IN_RANGE (INTVAL (XEXP (x, 1)), -(1LL << 26), (1LL << 26) - 1);

  return false;
}

bool
lvx_has_54bit_immediate_p (rtx x)
{
  if (MEM_P (x))
    x = XEXP (x, 0);

  if (GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 1)) == CONST_INT
      && REG_P (XEXP (x, 0)))
    return !IN_RANGE (INTVAL (XEXP (x, 1)), -(1LL << 26), (1LL << 26) - 1)
	   && IN_RANGE (INTVAL (XEXP (x, 1)), -(1LL << 53), (1LL << 53) - 1);

  return false;
}

/* Constraints }}} */

/* Load Variants {{{ */

/* Uncached is not an instruction, it is a modifier: the ISA has one LD, and
   LD.U is that LD carrying the "variant" modifier.  Everything below exists so
   that the back end says it once -- one enum, one predicate on the insn, one
   suffix printer -- instead of once per spelling.  The one thing a variant
   changes is the latency, which scheduling.md reads off the "variant"
   attribute whose default is (lvx_insn_variant).  */

/* The variant an address space asks for.  Anything that is not one of the
   three variant spaces -- the generic space, but also __convert and
   __syscall -- is a plain cached access.  */

static enum lvx_variant
lvx_addr_space_variant (addr_space_t as)
{
  switch (as)
    {
    case LVX_ADDR_SPACE_SPECULATE:
      return LVX_VARIANT_SPECULATE;
    case LVX_ADDR_SPACE_BYPASS:
      return LVX_VARIANT_UNCACHED;
    case LVX_ADDR_SPACE_PRELOAD:
      return LVX_VARIANT_PRELOAD;
    default:
      return LVX_VARIANT_CACHED;
    }
}

/* The variant the memory operand MEM asks for.  Not a MEM is not an error:
   %V is written on operands that only sometimes are one.  */

enum lvx_variant
lvx_mem_variant (rtx mem)
{
  return MEM_P (mem) ? lvx_addr_space_variant (MEM_ADDR_SPACE (mem))
		     : LVX_VARIANT_CACHED;
}

/* The variant spelled by an assembly modifier string, as the builtins pass
   one.  MOD is a concatenation of dot-separated tokens of which the variant
   is ".u" or ".us".  Tokens are matched whole, so neither the ".une" of a
   float comparison nor the ".su" of a wide multiply is mistaken for one.  */

static enum lvx_variant
lvx_string_variant (const char *mod)
{
  for (const char *p = mod; p && (p = strchr (p, '.')) != NULL; p++)
    {
      if (p[1] != 'u')
	continue;
      if (p[2] == '\0' || p[2] == '.')
	return LVX_VARIANT_UNCACHED;
      if (p[2] == 's' && (p[3] == '\0' || p[3] == '.'))
	return LVX_VARIANT_PRELOAD;
    }

  return LVX_VARIANT_CACHED;
}

/* The variant INSN's memory access carries, wherever it is spelled: on the
   address space of the memory it touches, or in a modifier string operand.
   This is the default value of the "variant" attribute, so it is asked of
   every insn and has to be total and cheap.  A pattern whose instruction has
   no cached form at all does not come here; it sets the attribute itself.  */

enum lvx_variant
lvx_insn_variant (rtx_insn *insn)
{
  enum lvx_variant variant = LVX_VARIANT_CACHED;
  bool has_mem = false;

  subrtx_iterator::array_type array;
  FOR_EACH_SUBRTX (iter, array, PATTERN (insn), ALL)
    {
      const_rtx x = *iter;

      if (MEM_P (x))
	{
	  enum lvx_variant v = lvx_addr_space_variant (MEM_ADDR_SPACE (x));
	  if (v != LVX_VARIANT_CACHED)
	    return v;
	  has_mem = true;
	}
      else if (GET_CODE (x) == CONST_STRING && variant == LVX_VARIANT_CACHED)
	variant = lvx_string_variant (XSTR (x, 0));
    }

  /* Only a memory access has a variant: the modifier string of, say, a wide
     multiply is none of our business.  */
  return has_mem ? variant : LVX_VARIANT_CACHED;
}

/* Whether VARIANT bypasses the cache, and so pays the long load latency.  */

bool
lvx_uncached_variant_p (enum lvx_variant variant)
{
  return variant == LVX_VARIANT_UNCACHED || variant == LVX_VARIANT_PRELOAD;
}

/* The assembly suffix for VARIANT, as MDS's "variant" modifier spells it.  */

const char *
lvx_variant_suffix (enum lvx_variant variant)
{
  switch (variant)
    {
    case LVX_VARIANT_SPECULATE:
      return ".s";
    case LVX_VARIANT_UNCACHED:
      return ".u";
    case LVX_VARIANT_PRELOAD:
      return ".us";
    default:
      return "";
    }
}

/* Load Variants }}} */

/* Handling of Hardware Loops {{{ */

/* A callback for the hw-doloop pass.  Called when a candidate doloop turns out
   not to be optimizable. The doloop_end pattern must be split into a decrement
   of the loop counter and looping branch if not zero, assuming without reload.
   In case of loop counter reload the doloop_end pattern was already split.  */

static void
lvx_hwloop_fail (hwloop_info loop)
{
  if (recog_memoized (loop->loop_end) != CODE_FOR_doloop_end_si
      && recog_memoized (loop->loop_end) != CODE_FOR_doloop_end_di)
    return;

  rtx (*gen_add) (rtx, rtx, rtx);
  rtx (*gen_cbranch) (rtx, rtx, rtx, rtx);
  machine_mode mode = GET_MODE (loop->iter_reg);
  gen_add = (mode == SImode) ? gen_addsi3 : gen_adddi3;
  gen_cbranch = (mode == SImode) ? gen_cbranchsi4 : gen_cbranchdi4;

  rtx decr = gen_add (loop->iter_reg, loop->iter_reg, constm1_rtx);
  emit_insn_before (decr, loop->loop_end);

  rtx test = gen_rtx_NE (VOIDmode, NULL_RTX, NULL_RTX);
  rtx cbranch
    = gen_cbranch (test, loop->iter_reg, const0_rtx, loop->start_label);
  rtx_insn *jump_insn = emit_jump_insn_before (cbranch, loop->loop_end);

  JUMP_LABEL (jump_insn) = loop->start_label;
  LABEL_NUSES (loop->start_label)++;

  delete_insn (loop->loop_end);
}

/* A callback for the hw-doloop pass.  This function examines INSN; if
   it is a doloop_end pattern we recognize, return the reg rtx for the
   loop counter.  Otherwise, return NULL_RTX.  */

static rtx
lvx_hwloop_pattern_reg (rtx_insn *insn)
{
  if (!JUMP_P (insn)
      || (recog_memoized (insn) != CODE_FOR_doloop_end_si
	  && recog_memoized (insn) != CODE_FOR_doloop_end_di))
    return NULL_RTX;

  rtx reg = SET_DEST (XVECEXP (PATTERN (insn), 0, 1));
  if (!REG_P (reg))
    return NULL_RTX;

  return reg;
}

static bool
lvx_hwloop_optimize (hwloop_info loop)
{
  if (loop->jumps_within)
    {
      if (dump_file)
	fprintf (dump_file, ";; loop %d jumps within\n", loop->loop_no);
      return false;
    }

  if (!loop->incoming_dest)
    {
      if (dump_file)
	fprintf (dump_file, ";; loop %d has more than one entry\n",
		 loop->loop_no);
      return false;
    }

  if (loop->incoming_dest != loop->head)
    {
      if (dump_file)
	fprintf (dump_file, ";; loop %d is not entered from head\n",
		 loop->loop_no);
      return false;
    }

  if (loop->iter_reg_used || loop->iter_reg_used_outside)
    {
      if (dump_file)
	fprintf (dump_file, ";; loop %d uses iteration counter register\n",
		 loop->loop_no);
      return false;
    }

  /* Check if start_label appears before doloop_end.  */
  rtx_insn *insn = loop->start_label;
  while (insn && insn != loop->loop_end)
    insn = NEXT_INSN (insn);

  if (!insn)
    {
      if (dump_file)
	fprintf (dump_file, ";; loop %d start_label not before loop_end\n",
		 loop->loop_no);
      return false;
    }

  /* Get the loop iteration counter register.  */
  rtx iter_reg = loop->iter_reg;
  machine_mode iter_mode = GET_MODE (iter_reg);

  int i = 0;
  edge entry_edge = NULL;
  basic_block entry_bb = NULL;
  FOR_EACH_VEC_SAFE_ELT (loop->incoming, i, entry_edge)
    if (entry_edge->flags & EDGE_FALLTHRU)
      {
	entry_bb = entry_edge->src;
	break;
      }

  if (!entry_bb)
    {
      if (dump_file)
	fprintf (dump_file, ";; loop %d not entered from fall-through edge\n",
		 loop->loop_no);
      return false;
    }

  /* Find the SET of ITER_REG in preheader blocks.  */
  rtx set_iter_reg = 0;
  if (iter_mode != DImode
      && single_succ_p (entry_bb)
      && entry_bb != ENTRY_BLOCK_PTR_FOR_FN (cfun))
    {
      basic_block preheader_bb = entry_bb;
      do
	{
	  FOR_BB_INSNS_REVERSE(preheader_bb, insn)
	    if (NONDEBUG_INSN_P (insn))
	      {
		rtx set = single_set (insn), set_dest = 0;
		if (set && (set_dest = SET_DEST (set))
		    && GET_CODE (set_dest) == REG
		    && REGNO (set_dest) == REGNO (iter_reg))
		  {
		    set_iter_reg = set;
		    break;
		  }
	      }

	    if (single_pred_p (preheader_bb))
	      preheader_bb = single_pred (preheader_bb);
	    else
	      break;
	  }
      while (!set_iter_reg);
    }

  loop->end_label = block_label (loop->successor);

  start_sequence ();

  /* If ITER_MODE is not DImode and the doloop counter may be set with
     a constant, ensure this constant has its 32 higher bits cleared.  */
  if (iter_mode != DImode)
    if (!set_iter_reg
	|| (CONST_INT_P (SET_SRC (set_iter_reg))
	    && INTVAL (SET_SRC (set_iter_reg)) > 0x00000000FFFFFFFFLL))
      emit_insn (gen_rtx_SET (gen_raw_REG (DImode, REGNO (iter_reg)),
			      gen_rtx_ZERO_EXTEND (DImode, iter_reg)));

  rtx_insn *loopdo = emit_insn (gen_lvx_loopdo (iter_reg, loop->end_label));
  rtx_insn *seq = get_insns ();

  end_sequence ();

  /* LOOPDO carries the end-of-loop address as a PC-relative immediate, and the
     hardware takes the back edge when the next bundle's address reaches it, so
     the body has to fit in that immediate's reach.  This is the range question
     a conditional branch answers by picking CCBX over CCB -- with the one
     difference that LOOPDO has no extended form to pick.  A body that does not
     fit cannot be a hardware loop at all, so hand it back and let
     (lvx_hwloop_fail) put the counter decrement and the branch in its place.

     The arithmetic is (lvx_fix_pcreljump_ranges)'s: the immediate is signed
     and scaled by 4, so PCREL bits of it reach 2**(PCREL + 1) bytes, less one
     maximum bundle of slack; and a quarter of the distance is the margin for
     this being an estimate.  It is an estimate for the same reasons as there,
     plus two of its own -- the preheader insns between the LOOPDO and the loop
     head are not counted, and SCHED2 has not run, so a "length" that overstates
     its template is not yet settled.  Erring towards the software loop is the
     cheap direction to err in.  */
  {
    long length = 0;
    unsigned ix;
    basic_block bb;

    FOR_EACH_VEC_ELT (loop->blocks, ix, bb)
      {
	rtx_insn *body_insn;
	FOR_BB_INSNS (bb, body_insn)
	  if (NONDEBUG_INSN_P (body_insn))
	    length += get_attr_length (body_insn);
      }

    long range = (2L << get_attr_pcrel (loopdo)) - LVX_SCHED2_BUNDLE_SIZE;
    if (length + (length >> 2) >= range)
      {
	if (dump_file)
	  fprintf (dump_file, ";; loop %d body is %ld bytes, past LOOPDO's"
		   " %ld-byte reach\n", loop->loop_no, length, range);
	return false;
      }
  }

  /* Place the loopdo instruction in the entry block into the loop body.  */
  if (!single_succ_p (entry_bb) || vec_safe_length (loop->incoming) > 1)
    {
      /* Split the head basic block of the loop before the first instruction to
         create a header wherein the loopdo instruction will be emitted.  */
      edge ee = split_block (loop->head, NEXT_INSN (BB_HEAD (loop->head)));
      basic_block hwloop_header = ee->src;
      basic_block hwloop_body = ee->dest;
      emit_insn_after (seq, NEXT_INSN (BB_HEAD (hwloop_header)));

      edge e;
      edge_iterator ei;
      FOR_EACH_EDGE (e, ei, loop->incoming)
	{
	  if (!(e->flags & EDGE_FALLTHRU))
	    redirect_edge_and_branch_force (e, hwloop_header);
	  else
	    redirect_edge_succ (e, hwloop_header);
	}

      e = find_edge (hwloop_body, hwloop_header);
      if (e != NULL)
	remove_edge (e);
      make_edge (hwloop_body, hwloop_body, 0);
    }
  else if (entry_bb != ENTRY_BLOCK_PTR_FOR_FN (cfun))
    {
      rtx_insn *entry_after = BB_END (entry_bb);
      emit_insn_after (seq, entry_after);
    }
  else
    {
      // Loop is at the very beginning of function
      insert_insn_on_edge (seq,
			   single_succ_edge (ENTRY_BLOCK_PTR_FOR_FN (cfun)));
      commit_one_edge_insertion (single_succ_edge
				 (ENTRY_BLOCK_PTR_FOR_FN (cfun)));
    }

  return true;
}

static struct hw_doloop_hooks lvx_doloop_hooks
  = { lvx_hwloop_pattern_reg, lvx_hwloop_optimize, lvx_hwloop_fail };

/* Handling of Hardware Loops }}} */

/* Long Offset Branches {{{ */

/* An LVX branch encodes its destination as a PC-relative immediate scaled by
   4, and the immediate is narrow: 11 bits for CCB, 17 for CB, 27 for GOTO.
   Each has an X form spending a second syllable on an immediate extension --
   CCBX 38 bits, CBX 44, GOTOX 54 -- which no translation unit exhausts.

   Which form a branch needs is not known when the pattern is written, so both
   forms match the same RTL, are told apart by their condition, and the
   "pcrel" attribute states how many bits of PC-relative immediate each one
   encodes.  Non-zero also marks the patterns this pass owns, which is why
   IGOTO, LOOPDO and the returns need no separate exclusion list.

   The pass measures every direct branch once, and for those that do not fit
   records the fact on the RTL itself so that it survives to final; the two
   patterns then split on lvx_jump_long_offset_p, which reads it back.

   Calls are deliberately absent.  A call names a symbol rather than a label
   in this function, so its distance is not known until link time -- it is
   never in label2offset below, and could not be measured if it were.  Far
   calls are the linker's business, which redirects them through a veneer
   (lvx_type_of_stub in bfd/elfnn-lvx.c).  LOOPDO is absent too: the body of
   a hardware loop should fit in the i-cache, so a loop spanning more than
   CB's 17 bits has larger problems than its branch range.  */

static int
lvx_insn_uid_cmp (const void *a, const void *b)
{
  rtx_insn *insn_a = *(rtx_insn * const *) a;
  rtx_insn *insn_b = *(rtx_insn * const *) b;
  return INSN_UID (insn_a) - INSN_UID (insn_b);
}

/* Find the LABEL_REF a direct branch jumps to: the source of the SET for an
   unconditional jump, and the taken arm of the IF_THEN_ELSE for a
   conditional one.  Only the patterns carrying a non-zero "pcrel" reach
   here, and all of them put the label in that position.  */

static rtx
lvx_jump_label_ref (rtx_insn *insn)
{
  rtx src = SET_SRC (single_set (insn));

  if (GET_CODE (src) == LABEL_REF)
    return src;
  if (GET_CODE (src) == IF_THEN_ELSE && GET_CODE (XEXP (src, 1)) == LABEL_REF)
    return XEXP (src, 1);

  gcc_unreachable ();
}

/* Select the long offset form for every direct branch whose destination is
   out of reach of the immediate its pattern encodes.  */

static void
lvx_fix_pcreljump_ranges (void)
{
  rtx_insn *insn;
  long offset = 0;
  int jump2offset_count = 0;
  hash_map <rtx_insn *, long> jump2offset;
  hash_map <rtx_insn *, long> label2offset;

  /* Approximate every branch's and every label's address by accumulating
     instruction lengths.  Bundling does not change the byte count -- a
     bundle of three syllables is the same twelve bytes as three bundles of
     one -- and nothing between here and final adds or removes instructions,
     so the sum is close.  It is still an estimate: a pattern's "length" is a
     static value that can over-state a template, and alignment padding is
     not counted at all.  The margin below is what covers the difference, in
     either direction.  */
  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      if (INSN_P (insn) && insn->deleted ())
	{
	  SET_INSN_DELETED (insn);
	  continue;
	}

      if (simplejump_p (insn) || any_condjump_p (insn))
	{
	  jump2offset.put (insn, offset);
	  jump2offset_count++;
	}

      if (LABEL_P (insn))
	label2offset.put (insn, offset);

      if (NONDEBUG_INSN_P (insn))
	offset += get_attr_length (insn);
    }

  rtx_insn **pcreljumps = XALLOCAVEC (rtx_insn *, jump2offset_count);

  int pcreljump_count = 0;
  for (hash_map<rtx_insn *, long>::iterator iter = jump2offset.begin ();
       iter != jump2offset.end (); ++iter)
    {
      rtx_insn *insn = (*iter).first;
      long offset = (*iter).second;
      int pcrel_bits = get_attr_pcrel (insn);
      if (!pcrel_bits)
	continue;

      rtx_insn *label = JUMP_LABEL_AS_INSN (insn);
      unsigned jump_partition = BB_PARTITION (BLOCK_FOR_INSN (insn));
      unsigned label_partition = BB_PARTITION (BLOCK_FOR_INSN (label));
      long *_value = label2offset.get (label);
      if (!_value)
	gcc_unreachable ();

      /* The immediate is signed and scaled by 4, so PCREL_BITS of it reach
	 2**(PCREL_BITS + 1) bytes either way; leave one maximum bundle of
	 slack.  The quarter added to the distance is the margin for this
	 scan being an estimate -- a branch just inside its range is widened
	 rather than risked.  A branch to the other partition is always
	 widened: the hot and cold halves end up in different sections, so
	 the distance measured here says nothing about the final one.  */
      long distance = labs (*_value - offset);
      long range = (2L << pcrel_bits) - LVX_SCHED2_BUNDLE_SIZE;
      if (jump_partition != label_partition
	  || distance + (distance >> 2) >= range)
	pcreljumps[pcreljump_count++] = insn;
    }

  /* The hash map iterates in an order that depends on where the insns were
     allocated, so sort before touching anything: the dump has to be the same
     from one compilation of the same input to the next.  */
  if (pcreljump_count > 1)
    qsort (pcreljumps, pcreljump_count, sizeof (rtx_insn *), lvx_insn_uid_cmp);

  for (int i = 0; i < pcreljump_count; i++)
    {
      rtx_insn *pcreljump = pcreljumps[i];

      if (dump_file)
	fprintf (dump_file, "LVX FIX PCRELJUMP insn %d\t(%d bits)\n",
		 INSN_UID (pcreljump), get_attr_pcrel (pcreljump));

      /* Abuse the volatil flag to record the long offset of a PCRELJUMP.
	 LABEL_REF_NONLOCAL_P means "target of a non-local goto", which is
	 meaningless for the destination operand of an ordinary branch, so
	 the bit is free here and, living on the LABEL_REF, travels with the
	 pattern.  Clearing INSN_CODE forces the insn to be recognised
	 again, and this time the short form's condition fails.  */
      LABEL_REF_NONLOCAL_P (lvx_jump_label_ref (pcreljump)) = 1;
      INSN_CODE (pcreljump) = -1;
    }
}

/* Used in insn conditions to select the long offset variant of a branch.  */

bool
lvx_jump_long_offset_p (rtx_insn *insn)
{
  if (!reload_completed)
    return false;

  if (simplejump_p (insn) || any_condjump_p (insn))
    /* Abuse the volatil flag to record the long offset of a PCRELJUMP.  */
    return LABEL_REF_NONLOCAL_P (lvx_jump_label_ref (insn));

  return false;
}

/* Long Offset Branches }}} */

/* 18 Target Description Macros and Functions {{{ */

/* 18.4 Defining data structures for per-function information {{{  */


/* Functions to save and restore machine-specific function data.
   Implements init_machine_status.  */

static struct machine_function *
lvx_init_machine_status (void)
{
  struct machine_function *machine;
  machine = ggc_cleared_alloc<machine_function> ();

  return machine;
}

/* Do anything needed before RTL is emitted for each function.
   Implements INIT_EXPANDERS.  */

/* Expand a hardware divide/modulo.  divmodw/divmoduw/divmodd/divmodud write
   the quotient into the low 64 bits of a 128-bit register pair and the
   remainder into the high 64, so both results come out of one FULL-slot
   instruction.  QUO and/or REM may be NULL when only one is wanted.

   The hardware returns zero for a division by zero instead of trapping, so
   -mdivmod0-trap still needs the explicit check; lvx_divmod_zero is const0_rtx
   when that option is on, which makes the OR below leave the divisor alone.  */

void
lvx_expand_divmod (rtx quo, rtx rem, rtx num, rtx den, machine_mode mode,
		   bool unsignedp)
{
  /* Does the divisor need checking at all?  Ask before force_reg, which would
     hide a constant behind a fresh pseudo.  */
  bool den_maybe_zero = !(CONST_INT_P (den) && INTVAL (den) != 0);

  num = force_reg (mode, num);
  den = force_reg (mode, den);

  /* The check below is a compare-and-branch around a trap: new control flow,
     which an expander may only create while the CFG is still being built.
     This expander also runs later -- doloop_optimize expands a division here
     to compute a loop's trip count -- and emitting a jump, barrier and label
     mid-block there leaves a basic block that verify_flow_info rejects, and in
     fact crashes on, since it reads BLOCK_FOR_INSN of the barrier, whose rtx
     is too short to have that field.  That is why 'while (s > 0) s -= 7;' at
     -Os used to ICE in loop2_doloop.

     Those late divisions divide by the loop step, a nonzero constant, so
     DEN_MAYBE_ZERO already turns the check off for them; the
     currently_expanding_to_rtl test is the general guard, since dropping a
     trap on a divisor that cannot be zero anyway is no loss, and dropping one
     we cannot emit legally beats corrupting the CFG.  */
  if (TARGET_DIVMOD0_TRAP && den_maybe_zero && currently_expanding_to_rtl)
    {
      rtx divnez = gen_label_rtx ();
      rtx pointer = gen_reg_rtx (Pmode);
      rtx notrap = gen_reg_rtx (mode);
      emit_insn (gen_rtx_SET (pointer, lvx_divmod_zero));
      emit_insn (gen_rtx_SET (notrap, gen_rtx_SUBREG (mode, pointer, 0)));
      emit_insn (gen_rtx_SET (notrap, gen_rtx_IOR (mode, den, notrap)));
      emit_cmp_and_jump_insns (notrap, const0_rtx, NE, NULL, mode, 0, divnez,
			       profile_probability::guessed_always ());
      expand_builtin_trap ();
      emit_label (divnez);
    }

  rtx pair = gen_reg_rtx (TImode);
  if (mode == SImode)
    emit_insn (unsignedp ? gen_lvx_udivmodsi (pair, num, den)
			 : gen_lvx_divmodsi (pair, num, den));
  else
    emit_insn (unsignedp ? gen_lvx_udivmoddi (pair, num, den)
			 : gen_lvx_divmoddi (pair, num, den));

  /* Little-endian: offset 0 is the quotient, offset 8 the remainder.  */
  if (quo)
    emit_move_insn (quo, simplify_gen_subreg (mode, pair, TImode, 0));
  if (rem)
    emit_move_insn (rem, simplify_gen_subreg (mode, pair, TImode, 8));
}

void
lvx_init_expanders (void)
{
  /* Arrange to initialize and mark the machine per-function status.  */
  init_machine_status = lvx_init_machine_status;

  if (TARGET_DIVMOD0_TRAP)
    lvx_divmod_zero = const0_rtx;
  else
    lvx_divmod_zero = const1_rtx;

  lvx_builtin_undef_counter = 0;

  lvx_ifcvt_ce_level = LVX_IFCVT_;

  /* The recorded vector comparisons name this function's pseudos; see
     lvx_record_packcmp.  */
  if (lvx_packcmp_map)
    lvx_packcmp_map->empty ();
}

/* 18.4 Defining data structures for per-function information }}}  */


/* 18.5 Storage Layout {{{  */

static enum flt_eval_method
lvx_excess_precision (enum excess_precision_type type)
{
  switch (type)
    {
    case EXCESS_PRECISION_TYPE_FAST:
    case EXCESS_PRECISION_TYPE_STANDARD:
    case EXCESS_PRECISION_TYPE_IMPLICIT:
      return FLT_EVAL_METHOD_PROMOTE_TO_FLOAT16;
    default:
      gcc_unreachable ();
    }
  return FLT_EVAL_METHOD_UNPREDICTABLE;
}

#undef TARGET_C_EXCESS_PRECISION
#define TARGET_C_EXCESS_PRECISION lvx_excess_precision

/* Implements TARGET_PROMOTE_FUNCTION_MODE.  */

static machine_mode
lvx_promote_function_mode (const_tree, machine_mode mode, int *punsignedp,
			   const_tree, int for_return)
{
  if (for_return)
    if (GET_MODE_CLASS (mode) == MODE_INT
	&& GET_MODE_SIZE (mode) < UNITS_PER_WORD)
      {
	mode = word_mode;
	*punsignedp = 1;
      }

  return mode;
}

#undef TARGET_PROMOTE_FUNCTION_MODE
#define TARGET_PROMOTE_FUNCTION_MODE lvx_promote_function_mode

static HOST_WIDE_INT
lvx_constant_alignment (const_tree exp, HOST_WIDE_INT align)
{
  return ((TREE_CODE (exp) == STRING_CST
	   && !optimize_size
	   && align < BITS_PER_WORD) ? BITS_PER_WORD : align);
}

#undef TARGET_CONSTANT_ALIGNMENT
#define TARGET_CONSTANT_ALIGNMENT lvx_constant_alignment

static const char *
lvx_mangle_type (const_tree type)
{
  /* Half-precision float.  */
  if (TREE_CODE (type) == REAL_TYPE && TYPE_PRECISION (type) == 16)
    return "Dh";

  return NULL;
}

#undef TARGET_MANGLE_TYPE
#define TARGET_MANGLE_TYPE lvx_mangle_type

/* 18.5 Storage Layout }}}  */


/* 18.7 Register Usage {{{  */

/* 18.7.1 Register Basics {{{  */

static void
lvx_conditional_register_usage (void)
{
  lvx_link_reg_rtx = gen_rtx_REG (Pmode, LVX_RETURN_POINTER_REGNO);

}

#undef TARGET_CONDITIONAL_REGISTER_USAGE
#define TARGET_CONDITIONAL_REGISTER_USAGE lvx_conditional_register_usage

/* 18.7.1 Register Basics }}}  */

/* 18.7.3 How Values Fit in Registers {{{  */

/* Implements TARGET_HARD_REGNO_NREGS. */
static unsigned int
lvx_hard_regno_nregs (unsigned int regno ATTRIBUTE_UNUSED, machine_mode mode)
{
  return (GET_MODE_SIZE (mode) + UNITS_PER_WORD - 1) / UNITS_PER_WORD;
}

#undef TARGET_HARD_REGNO_NREGS
#define TARGET_HARD_REGNO_NREGS lvx_hard_regno_nregs

/* Implements TARGET_HARD_REGNO_MODE_OK.  */
static bool
lvx_hard_regno_mode_ok (unsigned regno, enum machine_mode mode)
{
  unsigned size = GET_MODE_SIZE (mode);
  unsigned nregs = (size + UNITS_PER_WORD - 1) / UNITS_PER_WORD;
  nregs = nregs >= 1 ? nregs : 1;

  // GPR
  if (IN_RANGE (regno, LVX_GPR_FIRST_REGNO, LVX_GPR_LAST_REGNO))
    {
      // Maximum GPR aligment is 4 64-bit words (256 bits).
      nregs = nregs > 4 ? 4 : nregs;
      return !(regno & (nregs - 1));
    }

  // SFR
  if (IN_RANGE (regno, LVX_SFR_FIRST_REGNO, LVX_SFR_LAST_REGNO))
    return nregs == 1;

  // XCR
  if (IN_RANGE (regno, LVX_XCR_FIRST_REGNO, LVX_XCR_LAST_REGNO))
    return !(regno & (nregs - 1)) && lvx_extension_mode_p (mode);

  return false;
}

#undef TARGET_HARD_REGNO_MODE_OK
#define TARGET_HARD_REGNO_MODE_OK lvx_hard_regno_mode_ok

/* 18.7.3 How Values Fit in Registers }}}  */

/* 18.7 Register Usage }}}  */


/* 18.8 Register Classes {{{  */

/* Implements TARGET_SECONDARY_RELOAD.  */
static reg_class_t
lvx_secondary_reload (bool in_p, rtx x, reg_class_t, enum machine_mode,
		      secondary_reload_info *sri)
{
  if (flag_pic)
    {
      if (in_p && GET_CODE (x) == SYMBOL_REF && SYMBOL_REF_LOCAL_P (x)
	  && !SYMBOL_REF_EXTERNAL_P (x))
	{
	  if (sri->prev_sri == NULL)
	    {
	      return GPR_REGS;
	    }
	  else
	    {
	      sri->icode = GET_MODE (x) == SImode
		? CODE_FOR_reload_in_gotoff_si
		: CODE_FOR_reload_in_gotoff_di;
	      return NO_REGS;
	    }
	}
      else if (SYMBOLIC_CONST (x) && !lvx_legitimate_constant_p (VOIDmode, x))
	{
	  gcc_unreachable ();
	}
    }

#if 0
  if (reload_class == XCR_REGS && CONSTANT_P (x))
    return GENERAL_REGS;
#endif

  return NO_REGS;
}

#undef TARGET_SECONDARY_RELOAD
#define TARGET_SECONDARY_RELOAD lvx_secondary_reload

/* Implements TARGET_CLASS_MAX_NREGS.  */
static unsigned char
lvx_class_max_nregs (reg_class_t regclass ATTRIBUTE_UNUSED,
		     enum machine_mode mode)
{
  return lvx_hard_regno_nregs (0, mode);
}

#undef TARGET_CLASS_MAX_NREGS
#define TARGET_CLASS_MAX_NREGS lvx_class_max_nregs

/* Implements TARGET_COMPUTE_PRESSURE_CLASSES.  */
static int
lvx_compute_pressure_classes (reg_class *classes)
{
  // Register pressure matters in GPR_REGS and XCR_REGS.
  int n = 0;
  classes[n++] = GPR_REGS;
  classes[n++] = XCR_REGS;
  return n;
}

#undef TARGET_COMPUTE_PRESSURE_CLASSES
#define TARGET_COMPUTE_PRESSURE_CLASSES lvx_compute_pressure_classes

/* 18.8 Register Classes }}}  */


/* 18.9 Stack Layout and Calling Conventions {{{ */

/* 18.9.1 Basic Stack Layout {{{ */

HOST_WIDE_INT
lvx_first_parm_offset (tree decl ATTRIBUTE_UNUSED)
{
  struct lvx_frame_info *frame;
  lvx_compute_frame_info ();
  frame = &cfun->machine->frame;

  return frame->arg_pointer_offset - frame->virt_frame_pointer_offset;
}

rtx
lvx_return_addr_rtx (int count, rtx frameaddr ATTRIBUTE_UNUSED)
{
  return count == 0
    ? get_hard_reg_initial_val (Pmode, LVX_RETURN_POINTER_REGNO)
    : NULL_RTX;
}

/* 18.9.1 Basic Stack Layout }}} */

/* 18.9.4 Registers That Address the Stack Frame {{{ */

static rtx
lvx_static_chain (const_tree ARG_UNUSED (fndecl), bool incoming_p)
{
  if (incoming_p)
    {
      cfun->machine->static_chain_needed = 1;
      return gen_frame_mem (Pmode, frame_pointer_rtx);
    }
  else
    {
      return gen_rtx_REG (Pmode, 31);
    }
}

#undef TARGET_STATIC_CHAIN
#define TARGET_STATIC_CHAIN lvx_static_chain

/* 18.9.4 Registers That Address the Stack Frame }}} */

/* 18.9.5 Eliminating Frame Pointer and Arg Pointer {{{ */

/* Implements INITIAL_ELIMINATION_OFFSET.  FROM is either the frame pointer
   or argument pointer.  TO is either the stack pointer or frame
   pointer.  */

poly_int64
lvx_initial_elimination_offset (int from, int to)
{
  lvx_compute_frame_info ();
  struct lvx_frame_info *frame = &cfun->machine->frame;

  /* Should never have anything else FRAME_POINTER_REGNUM -> HFP/SP */
  if (from != FRAME_POINTER_REGNUM
      || (to != STACK_POINTER_REGNUM && to != HARD_FRAME_POINTER_REGNUM))
    gcc_unreachable ();

  if (from == FRAME_POINTER_REGNUM && to == STACK_POINTER_REGNUM)
    return frame->virt_frame_pointer_offset;
  else if (from == FRAME_POINTER_REGNUM && to == HARD_FRAME_POINTER_REGNUM)
    return (frame->virt_frame_pointer_offset
	    - frame->hard_frame_pointer_offset);

  gcc_unreachable ();
}

/* 18.9.5 Eliminating Frame Pointer and Arg Pointer }}} */

/* 18.9.6 Passing Function Arguments on the Stack {{{ */

#undef TARGET_PROMOTE_PROTOTYPES
#define TARGET_PROMOTE_PROTOTYPES hook_bool_const_tree_true

/* 18.9.6 Passing Function Arguments on the Stack }}} */

/* 18.9.7 Passing Arguments in Registers {{{ */

/* Utils {{{ */
/* Information about a single argument.  */
struct lvx_arg_info
{
  /* first register to be used for this arg */
  unsigned int first_reg;

  /* number of registers used */
  int num_regs;

  /* number of words pushed on the stack (in excess of registers) */
  int num_stack;
};

/* Analyzes a single argument and fills INFO struct. Does not modify
   CUM_V. Returns a reg rtx pointing at first argument register to be
   used for given argument or NULL_RTX if argument must be stacked
   because there is no argument slot in registers free/correctly
   aligned. */

static rtx
lvx_get_arg_info (struct lvx_arg_info *info, cumulative_args_t cum_v,
		   const function_arg_info& arg)
{
  CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);
  HOST_WIDE_INT n_bytes
    = arg.type ? int_size_in_bytes (arg.type) : GET_MODE_SIZE (arg.mode);
  HOST_WIDE_INT n_words = (n_bytes + UNITS_PER_WORD - 1) / UNITS_PER_WORD;

  info->first_reg = cum->next_arg_reg;

  /* If all register argument slots are used, then it must go on the
     stack. */
  if (cum->next_arg_reg >= LVX_ARG_REG_SLOTS)
    {
      info->num_stack = n_words;
      info->num_regs = 0;
      return NULL_RTX;
    }

  info->num_regs = LVX_ARG_REG_SLOTS - info->first_reg;

  if (info->num_regs >= n_words)
    {
      /* All arg fits in remaining registers */
      info->num_regs = n_words;
      info->num_stack = 0;
    }
  else
    {
      /* At least one word on stack */
      info->num_stack = n_words - info->num_regs;
    }

  return gen_rtx_REG (arg.mode, LVX_ARGUMENT_POINTER_REGNO + info->first_reg);
}

/* Utils }}} */

/* Implements TARGET_FUNCTION_ARG.
   Returns a reg rtx pointing at first argument register to be
   used for given argument or NULL_RTX if argument must be stacked
   because there is no argument slot in registers free.

   A multi-word argument whose first register cannot hold its mode is handed
   back as a PARALLEL of word-sized pieces rather than as one wide REG.  The
   ABI puts a multi-word argument at the first free argument register whatever
   its parity, so anything consuming one register ahead of a pair starts that
   pair odd: with `int c' in $r0 an __int128 argument lands on $r1$r2, and a
   256-bit one on $r1..$r4.  That placement is the ABI -- clang agrees on it,
   and changing it would break calls between the two compilers -- but a wide
   REG at a register number TARGET_HARD_REGNO_MODE_OK rejects is not valid RTL:
   no instruction can name $r1$r2 as a pair, and one that reaches print_operand
   with it dies in lvx_pgr_reg_name.  A PARALLEL is the standard way to say
   "these registers, in this order" without claiming a wide register exists;
   the middle end moves the pieces with emit_group_load/store and the value
   reaches a correctly aligned pseudo at both ends of the call.

   Fixing this at the pattern level instead -- an lvx_hardreg_aligned_p guard
   on whatever cannot name the pair, the way the wide moves in vector.md and
   the predicated wide load/store in control.md are guarded -- does not hold up
   here, which is worth recording.  That idiom comes from KVX, where the only
   instructions naming a wide register were the wide move and the wide
   load/store, everything else being split into 64-bit halves.  LVX has a
   quadword ALU and 128-bit SIMD, so an outgoing argument register is the
   destination of ordinary arithmetic: `f (c, x | y, ...)' puts an IORTI3
   straight into $r3, with no move anywhere to split.  Covering that means
   guarding every wide-destination pattern in the port rather than a handful,
   and missing one is a silent ICE in final.  Making the misaligned wide hard
   register not exist is the whole fix; making each consumer refuse it is not.

   Aligned starts keep the plain REG, so the common case is untouched, and so
   does an argument split between registers and the stack -- that one is
   assembled in memory, so no wide hard register is formed for it either.  Both
   hooks must agree in shape: a sibling call loads its arguments through
   TARGET_FUNCTION_INCOMING_ARG's rtl (calls.cc's tail_call_reg) and
   emit_group_move requires both sides to be PARALLELs, so returning a PARALLEL
   from one and a REG from the other segfaults expand on the first such tail
   call.  TARGET_FUNCTION_INCOMING_ARG defaults to this hook, which is what
   keeps them in step.  */

static rtx
lvx_function_arg (cumulative_args_t cum_v, const function_arg_info &arg)
{
  struct lvx_arg_info info = { 0, 0, 0 };
  rtx reg = lvx_get_arg_info (&info, cum_v, arg);

  if (reg == NULL_RTX
      || info.num_regs <= 1
      || info.num_stack != 0
      || lvx_hard_regno_mode_ok (REGNO (reg), arg.mode))
    return reg;

  rtx par = gen_rtx_PARALLEL (arg.mode, rtvec_alloc (info.num_regs));
  for (int i = 0; i < info.num_regs; i++)
    XVECEXP (par, 0, i)
      = gen_rtx_EXPR_LIST (VOIDmode,
			   gen_rtx_REG (word_mode, REGNO (reg) + i),
			   GEN_INT (i * UNITS_PER_WORD));
  return par;
}

#undef TARGET_FUNCTION_ARG
#define TARGET_FUNCTION_ARG lvx_function_arg

#undef TARGET_MUST_PASS_IN_STACK
#define TARGET_MUST_PASS_IN_STACK must_pass_in_stack_var_size

/* Implements TARGET_ARG_PARTIAL_BYTES.
   Return the number of bytes, at the beginning of an argument,
   that must be put in registers */

static int
lvx_arg_partial_bytes (cumulative_args_t cum_v, const function_arg_info &arg)
{
  struct lvx_arg_info info = { 0, 0, 0 };
  rtx reg = lvx_get_arg_info (&info, cum_v, arg);
  if (reg != NULL_RTX && info.num_regs > 0 && info.num_stack > 0)
    {
      return info.num_regs * UNITS_PER_WORD;
    }
  return 0;
}

#undef TARGET_ARG_PARTIAL_BYTES
#define TARGET_ARG_PARTIAL_BYTES lvx_arg_partial_bytes

static bool
lvx_pass_by_reference (cumulative_args_t, const function_arg_info &arg)
{
  HOST_WIDE_INT size = GET_MODE_SIZE (arg.mode);

  /* GET_MODE_SIZE (BLKmode) is useless since it is 0.  */
  if (arg.mode == BLKmode && arg.type)
    size = int_size_in_bytes (arg.type);

  /* Aggregates are passed by reference based on their size.  */
  if (arg.type && AGGREGATE_TYPE_P (arg.type))
    size = int_size_in_bytes (arg.type);

  /* Coprocessor arguments are passed by reference.  */
  if (lvx_extension_mode_p (arg.mode))
    return true;

  /* Arguments which are variable sized or larger than 4 registers are
     passed by reference */
  return (size > (4 * UNITS_PER_WORD) || size < 0);
}

#undef TARGET_PASS_BY_REFERENCE
#define TARGET_PASS_BY_REFERENCE lvx_pass_by_reference

/* Implements INIT_CUMULATIVE_ARGS. */

void
lvx_init_cumulative_args (CUMULATIVE_ARGS *cum, const_tree, rtx, tree, int)
{
  cum->next_arg_reg = 0;
  cum->anonymous_arg_offset = 0;
  cum->anonymous_arg_offset_valid = false;
}

static void
lvx_function_arg_advance (cumulative_args_t cum_v,
			  const function_arg_info &arg)
{
  CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);
  struct lvx_arg_info info = { 0, 0, 0 };
  lvx_get_arg_info (&info, cum_v, arg);

  if (info.num_regs > 0)
    cum->next_arg_reg = info.first_reg + info.num_regs;
  else if (arg.named)
    cum->anonymous_arg_offset += info.num_stack * UNITS_PER_WORD;
  else if (!cum->anonymous_arg_offset_valid)
    {
      /* First !named arg is in fact the last named arg */
      cum->anonymous_arg_offset += info.num_stack * UNITS_PER_WORD;
      cum->anonymous_arg_offset_valid = true;
    }
}

#undef TARGET_FUNCTION_ARG_ADVANCE
#define TARGET_FUNCTION_ARG_ADVANCE lvx_function_arg_advance

static bool
lvx_scalar_mode_supported_p (scalar_mode mode)
{
  if (mode == HFmode)
    return true;

  int precision = GET_MODE_PRECISION (mode);
  if (precision > 128)
    return false;

  switch (GET_MODE_CLASS (mode))
    {
    case MODE_PARTIAL_INT:
    case MODE_INT:
      if (precision == SHORT_TYPE_SIZE)
	return true;
      break;

    default:
      break;
    }

  return default_scalar_mode_supported_p (mode);
}

#undef TARGET_SCALAR_MODE_SUPPORTED_P
#define TARGET_SCALAR_MODE_SUPPORTED_P lvx_scalar_mode_supported_p

static bool
lvx_vector_mode_supported_p (enum machine_mode mode)
{
  if (lvx_extension_mode_p (mode))
    return true;

  /* SIMD is lvx-2 only: every packed arithmetic instruction -- addho, addwq,
     adddp, compdp and the rest -- is tagged Opcode-lvx_2 in the ISA, and the
     patterns that emit them are gated on LVX_2.  Claiming the modes anyway
     would let the middle end form vector values there is no pattern to
     operate on, so an explicit vector_size type ICEs with "unrecognizable
     insn" instead of being lowered element-wise.  */
  if (!LVX_2)
    /* V2SF is the exception, and it is not really SIMD: two floats is one
       `float complex`, 64 bits, which lives in a general register rather than
       across a SIMD pair.  FADDWC, FSBFWC, FMULWC, FFMAWC and FCONJWC are
       lvx-1 instructions operating on exactly that, so the mode has to exist
       on lvx-1 or their builtins cannot be called there.  */
    return mode == V2SFmode;

  /* V2SF is the same 64-bit GPR-resident `float complex' exception as on
     lvx-1 (see above); it is not backed by a SIMD register pair.  */
  if (mode == V2SFmode)
    return true;

  /* Native SIMD is 128-bit and wider (V16QI ... up to 64-byte / 8-register
     vectors); the patterns are written over SIMD128/256/512.  A sub-128-bit
     vector -- V2HI, V4QI, V8QI, V4HI, V2SI and the like -- has no packed
     instruction and no mov<mode> pattern, so claiming it lets the middle end
     put a vector value where nothing can move or operate on it: passing such a
     type as an argument ICEs in emit_move_multi_word (gcc.c-torture/execute/
     20050316-1.c).  Bound it from below so those types are lowered
     element-wise instead.  */
  /* V4HI and V2SI are claimed as well, and the arithmetic on them is done
     on the 128-bit unit: the operand goes in the low half of a pair, the
     packed instruction runs, and the low half is the result (the widening
     expanders in vector.md, lvx_expand_widen64).  Claiming a mode is what
     lets a pattern for it be selected at all, and it is cheap on its own --
     with no pattern the middle end lowers exactly as it does for a mode it
     was never told about, so the bitwise operations stay the one ANDD, IORD
     or EORD that the 64-bit container is.

     V4HF joins them for the same reason (2026-09-28): four halves in a GPR
     widen into V8HF's FADDHO/FSBFHO/FMULHO, seven instructions where the
     element-wise lowering measured seventeen (eight LHZ, four FADDH, four
     SH).  The expanders existed before the mode was claimed and were simply
     unreachable -- an unclaimed mode is never offered to them.

     None of these needs a mov<mode> pattern: at eight bytes they are moved
     via the same-size integer (emit_move_via_integer), which is also why the
     four-byte modes (V2HI, V4QI) stay out -- passing one as an argument ICEd
     in emit_move_multi_word, which is what the lower bound below was added
     for (gcc.c-torture/execute/20050316-1.c).  */
  if (mode == V8QImode || mode == V4HImode || mode == V2SImode
      || mode == V4HFmode)
    return true;

  unsigned size = GET_MODE_SIZE (mode);
  return (size >= 16 && size <= UNITS_PER_WORD * 8);
}


#undef TARGET_VECTOR_MODE_SUPPORTED_P
#define TARGET_VECTOR_MODE_SUPPORTED_P lvx_vector_mode_supported_p

static bool
lvx_libgcc_floating_mode_supported_p (scalar_float_mode mode)
{
  return mode == HFmode
    ? true
    : default_libgcc_floating_mode_supported_p (mode);
}

#undef TARGET_LIBGCC_FLOATING_MODE_SUPPORTED_P
#define TARGET_LIBGCC_FLOATING_MODE_SUPPORTED_P                                \
  lvx_libgcc_floating_mode_supported_p

/* 18.9.7 Passing Arguments in Registers }}} */

/* 18.9.8 How Scalar Function Values Are Returned {{{ */

/* Implements TARGET_FUNCTION_VALUE.  */
static rtx
lvx_function_value (const_tree ret_type, const_tree func,
		    bool outgoing ATTRIBUTE_UNUSED)
{
  int unsignedp = TYPE_UNSIGNED (ret_type);
  enum machine_mode mode = TYPE_MODE (ret_type);
  HOST_WIDE_INT size = int_size_in_bytes (ret_type);

  mode = promote_function_mode (ret_type, mode, &unsignedp, func, 1);

  if (mode == BLKmode && (size * BITS_PER_UNIT > LONG_LONG_TYPE_SIZE))
    {
      int nexps = (size + UNITS_PER_WORD - 1) / UNITS_PER_WORD;
      rtx ret = gen_rtx_PARALLEL (BLKmode, rtvec_alloc (nexps));

      for (int i = 0; i < nexps; i++)
	XVECEXP (ret, 0, i)
	  = gen_rtx_EXPR_LIST (VOIDmode,
			       gen_rtx_REG (DImode,
					    LVX_ARGUMENT_POINTER_REGNO + i),
			       GEN_INT (i * UNITS_PER_WORD));
      return ret;
    }

  return gen_rtx_REG (mode, LVX_ARGUMENT_POINTER_REGNO);
}

#undef TARGET_FUNCTION_VALUE
#define TARGET_FUNCTION_VALUE lvx_function_value

#undef TARGET_OMIT_STRUCT_RETURN_REG
#define TARGET_OMIT_STRUCT_RETURN_REG true

#undef TARGET_RETURN_IN_MSB
#define TARGET_RETURN_IN_MSB hook_bool_const_tree_false

/* 18.9.8 How Scalar Function Values Are Returned }}} */

/* 18.9.9 How Large Values Are Returned {{{ */

/* Implements TARGET_RETURN_IN_MEMORY.  */
static bool
lvx_return_in_memory (const_tree type, const_tree fntype ATTRIBUTE_UNUSED)
{
  HOST_WIDE_INT size = int_size_in_bytes (type);

  /* Coprocessor values return in memory. */
  if (lvx_extension_mode_p (TYPE_MODE (type)))
    return true;

  /* Return value can use up to 4 registers (256bits). Larger values
   * or variable sized type must be returned in memory. */
  return (size > (4 * UNITS_PER_WORD) || size < 0);
}

#undef TARGET_RETURN_IN_MEMORY
#define TARGET_RETURN_IN_MEMORY lvx_return_in_memory

/* Implements TARGET_STRUCT_VALUE_RTX.  */
static rtx
lvx_struct_value_rtx (tree fndecl ATTRIBUTE_UNUSED,
		      int incoming ATTRIBUTE_UNUSED)
{
  return gen_rtx_REG (Pmode, LVX_STRUCT_POINTER_REGNO);
}

#undef TARGET_STRUCT_VALUE_RTX
#define TARGET_STRUCT_VALUE_RTX lvx_struct_value_rtx

/* 18.9.9 How Large Values Are Returned }}} */

/* 18.9.11 Function Entry and Exit {{{ */

/* Implements TARGET_ASM_FUNCTION_PROLOGUE.  */
static void
lvx_function_prologue (FILE *)
{
}

#undef TARGET_ASM_FUNCTION_PROLOGUE
#define TARGET_ASM_FUNCTION_PROLOGUE lvx_function_prologue

/* Implements TARGET_ASM_FUNCTION_EPILOGUE.  */
static void
lvx_function_epilogue (FILE *)
{
  lvx_sched2_dtor ();
  if (lvx_ifcvt->ce_level > LVX_IFCVT_)
    lvx_ifcvt_dtor ();
}

#undef TARGET_ASM_FUNCTION_EPILOGUE
#define TARGET_ASM_FUNCTION_EPILOGUE lvx_function_epilogue

static void
lvx_asm_output_mi_thunk (FILE *file, tree thunk_fndecl, HOST_WIDE_INT delta,
			 HOST_WIDE_INT vcall_offset, tree function)
{
  const char *fnname = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (thunk_fndecl));
  rtx xops[1];

  assemble_start_function (thunk_fndecl, fnname);
  if (delta)
    /* FIXME AUTO: this is fixed for build, not checked for correctness ! */
    fprintf (file, "\taddd $r0 = $r0, %i\n", (int) delta);

  if (vcall_offset)
    {
      /* FIXME AUTO: this is fixed for build, not checked for correctness ! */
      fprintf (file, "\tld $r32 = %i[$r0]\n\t;;\n", (int) delta);
      fprintf (file, "\tld $r32 = %i[$r32]\n\t;;\n", (int) vcall_offset);
      fprintf (file, "\taddd $r0 = $r0, $r32\n");
    }
  xops[0] = XEXP (DECL_RTL (function), 0);
  output_asm_insn ("goto\t%0\n\t;;", xops);

  assemble_end_function (thunk_fndecl, fnname);
}

#undef TARGET_ASM_OUTPUT_MI_THUNK
#define TARGET_ASM_OUTPUT_MI_THUNK lvx_asm_output_mi_thunk

static bool
lvx_asm_can_output_mi_thunk (const_tree thunk_fndecl ATTRIBUTE_UNUSED,
			     HOST_WIDE_INT delta ATTRIBUTE_UNUSED,
			     HOST_WIDE_INT vcall_offset ATTRIBUTE_UNUSED,
			     const_tree function ATTRIBUTE_UNUSED)
{
  return true;
}

#undef TARGET_ASM_CAN_OUTPUT_MI_THUNK
#define TARGET_ASM_CAN_OUTPUT_MI_THUNK lvx_asm_can_output_mi_thunk

/* 18.9.11 Function Entry and Exit }}} */

/* 18.9.12 Generating Code for Profiling {{{ */

#ifdef GCC_LVX_MPPA_LINUX
void
lvx_output_function_profiler (FILE *file)
{
  int temp_reg = REGNO (lvx_get_callersaved_nonfixed_reg (Pmode, 2));
  fprintf (file, "\n\tget $r%d = $ra", PROFILE_REGNO);
  if (LVX_FARCALL)
    {
      fprintf (file, "\n\tmaked $r%d = __mcount\n\t;;\n\t", temp_reg);
      fprintf (file, "\n\ticall $r%d\n\t;;\n\t", temp_reg);
    }
  else
    {
      fputs ("\n\t;;\n\tcall __mcount\n\t;;\n\t", file);
    }
}
#endif

#ifndef GCC_LVX_MPPA_LINUX
void
lvx_profile_hook (void)
{
  rtx ra_arg = get_hard_reg_initial_val (Pmode, LVX_RETURN_POINTER_REGNO);
  rtx fun = gen_rtx_SYMBOL_REF (Pmode, "__mcount");
  emit_library_call (fun, LCT_NORMAL, VOIDmode, ra_arg, Pmode);
}
#endif

/* 18.9 Stack Layout and Calling Conventions }}} */

/* 18.9.13 Permitting tail calls {{{ */

static bool
lvx_function_ok_for_sibcall (tree decl, tree exp ATTRIBUTE_UNUSED)
{
  /* Sometimes GCC allocates a call_used_register to hold the sibcall
     destination... This of course  won't work because we can't restore that
     register to the right value and do the call (In fact we could by bundling
     the igoto and the restore together).
     FIXME: I think we can avoid this by defining sibcall and
     sibcall_value expanders that will use scratch registers
     instead of any GPR. */

  if (!decl)
    return false;

  /* A farcall used to be refused here, for the same reason as the weak
     symbol below: it went out as an indirect jump through a register the
     prologue could overwrite.  It no longer does -- a farcall sibcall is a
     GOTOX naming the symbol, with no register in it at all -- so there is
     nothing left to refuse.  (The test that stood here read the attribute
     off TYPE_ATTRIBUTES, where a decl attribute never is, so it only ever
     caught -mfarcall; an __attribute__((farcall)) tail call reached the
     indirect sibcall patterns, which are commented out below, and ICEd.)  */

  /* Do not tail-call calls to weak symbol, whose reference still becomes an
     indirect jump: GCC can allocate the destination to a call-used register
     that the epilogue restores before the jump is taken.
   */
  if (DECL_WEAK (decl))
    {
      return false;
    }

  return true;
}

#undef TARGET_FUNCTION_OK_FOR_SIBCALL
#define TARGET_FUNCTION_OK_FOR_SIBCALL lvx_function_ok_for_sibcall

/* 18.9.13 Permitting tail calls }}} */

/* 18.9.12 Generating Code for Profiling }}} */


/* 18.10 Implementing the Varargs Macros {{{ */

static rtx
lvx_expand_builtin_saveregs (void)
{
  int slot = 0;
  struct lvx_frame_info *frame;

  lvx_compute_frame_info ();
  frame = &cfun->machine->frame;
  HOST_WIDE_INT arg_fp_offset
    = frame->arg_pointer_offset - frame->virt_frame_pointer_offset;
  rtx area = gen_rtx_PLUS (Pmode, frame_pointer_rtx, GEN_INT (arg_fp_offset));

  /* All argument register slots used for named args, nothing to push */
  if (crtl->args.info.next_arg_reg >= LVX_ARG_REG_SLOTS)
    return const0_rtx;

  /* use arg_pointer since saved register slots are not known at that time */
  int regno = crtl->args.info.next_arg_reg;

  if (regno & 1)
    {
      rtx insn = emit_move_insn (gen_rtx_MEM (DImode, area),
				 gen_rtx_REG (DImode,
					      LVX_ARGUMENT_POINTER_REGNO +
					      regno));
      RTX_FRAME_RELATED_P (insn) = 1;
      /* Do not attach a NOTE here as the frame has not been laid out yet.
         Let the lvx_fix_debug_for_bundles function during reorg pass handle
         these.  */

      regno++;
      slot++;
    }

  for (; regno < LVX_ARG_REG_SLOTS; regno += 2, slot += 2)
    {
      rtx addr
	= gen_rtx_MEM (TImode, gen_rtx_PLUS (Pmode, frame_pointer_rtx,
					     GEN_INT (slot * UNITS_PER_WORD
						      + arg_fp_offset)));
      rtx src = gen_rtx_REG (TImode, LVX_ARGUMENT_POINTER_REGNO + regno);

      rtx insn = emit_move_insn (addr, src);
      RTX_FRAME_RELATED_P (insn) = 1;
      /* Do not attach a NOTE here as the frame has not been laid out yet.
         Let the lvx_fix_debug_for_bundles function during reorg pass handle
         these.  */
    }

  return area;
}

#undef TARGET_EXPAND_BUILTIN_SAVEREGS
#define TARGET_EXPAND_BUILTIN_SAVEREGS lvx_expand_builtin_saveregs

/* FIXME: Not in the manual. */
static void
lvx_expand_va_start (tree valist, rtx)
{
  rtx va_start_addr = expand_builtin_saveregs ();
  rtx va_r = expand_expr (valist, NULL_RTX, VOIDmode, EXPAND_WRITE);

  struct lvx_frame_info *frame;

  frame = &cfun->machine->frame;
  HOST_WIDE_INT arg_fp_offset
    = frame->arg_pointer_offset - frame->virt_frame_pointer_offset;

  gcc_assert (frame->laid_out);

  /* All arg registers must be used by named parameter, va_start
     must point to caller frame for first anonymous parameter ... */
  if (va_start_addr == const0_rtx
      && crtl->args.info.anonymous_arg_offset_valid)
    {
      /* ... and there are some more arguments. */
      va_start_addr
	= gen_rtx_PLUS (Pmode, frame_pointer_rtx,
			GEN_INT (crtl->args.info.anonymous_arg_offset
				 + arg_fp_offset));
    }
  else
    {
      /* ... and there are no more argument. */
      va_start_addr
	= gen_rtx_PLUS (Pmode, frame_pointer_rtx, GEN_INT (arg_fp_offset));
    }

  emit_move_insn (va_r, va_start_addr);
}

#undef TARGET_EXPAND_BUILTIN_VA_START
#define TARGET_EXPAND_BUILTIN_VA_START lvx_expand_va_start


/* 18.10 Implementing the Varargs Macros }}} */


/* 18.11 Support for Nested Functions {{{ */

/* Takes a location MEM and write a make opcode in the stack which
   loads the ADDR into the general purpose register REGNO.
   Returns the number of byte written on the stack.  */
static int
lvx_emit_make_of_addr (rtx loc, rtx addr, unsigned int regno)
{
  int offset = 0;
  rtx mem;
  unsigned long mask_extend27 = 0xffffffe000000000U;
  unsigned long mask_upper27 = 0x0000001ffffffc00U;
  unsigned long mask_lower10 = 0x00000000000003ffU;

  rtx addr_lower10 = copy_to_reg (addr);
  mem = adjust_address (loc, Pmode, offset);
  emit_insn (gen_anddi3 (addr_lower10, addr_lower10, GEN_INT (mask_lower10)));
  emit_insn (gen_ashldi3 (addr_lower10, addr_lower10, GEN_INT (6U)));
  emit_insn (gen_add2_insn
	     (addr_lower10, GEN_INT (((0xe0 << 8) + (regno << 2)) << 16)));
  emit_move_insn (mem, addr_lower10);
  offset += 4;

  rtx addr_upper27 = copy_to_reg (addr);
  mem = adjust_address (loc, Pmode, offset);
  emit_insn (gen_anddi3 (addr_upper27, addr_upper27, GEN_INT (mask_upper27)));
  emit_insn (gen_lshrdi3 (addr_upper27, addr_upper27, GEN_INT (10)));
  emit_insn (gen_add2_insn (addr_upper27, GEN_INT (0x8 << 28)));
  emit_move_insn (mem, addr_upper27);
  offset += 4;

  rtx addr_extend27 = copy_to_reg (addr);
  emit_insn (gen_anddi3
	     (addr_extend27, addr_extend27, GEN_INT (mask_extend27)));
  emit_insn (gen_lshrdi3 (addr_extend27, addr_extend27, GEN_INT (37)));
  mem = adjust_address (loc, Pmode, offset);
  emit_move_insn (mem, addr_extend27);
  offset += 4;

  return offset;
}

/* This hook is called to initialize a trampoline.  M_TRAMP is an RTX
   for the memory block for the trampoline; FNDECL is the
   'FUNCTION_DECL' for the nested function; STATIC_CHAIN is an RTX for
   the static chain value that should be passed to the function when
   it is called.

   If the target requires any other actions, such as flushing caches
   or enabling stack execution, these actions should be performed
   after initializing the trampoline proper.  */
static void
lvx_trampoline_init (rtx m_tramp, tree fndecl, rtx chain_value)
{
  rtx mem, fnaddr = XEXP (DECL_RTL (fndecl), 0);
  int offset = 0;

  // make $31 = static_chain_addr
  mem = adjust_address (m_tramp, Pmode, offset);
  offset += lvx_emit_make_of_addr (mem, chain_value, 31);

  // make $30 = fnaddr
  mem = adjust_address (m_tramp, Pmode, offset);
  offset += lvx_emit_make_of_addr (mem, fnaddr, 30);

  // igoto $r30
  mem = adjust_address (m_tramp, SImode, offset);
  emit_move_insn (mem, gen_int_mode (0x0fd8001e, SImode));
  offset += 4;

  emit_insn (gen_lvx_i1invals (chain_value));
  emit_insn (gen_lvx_fence (gen_rtx_CONST_STRING (VOIDmode, "")));

  gcc_assert (offset <= TRAMPOLINE_SIZE);
}

#undef TARGET_TRAMPOLINE_INIT
#define TARGET_TRAMPOLINE_INIT lvx_trampoline_init

/* 18.11 Support for Nested Functions }}} */


/* 18.13 Addressing Modes {{{ */

static bool
lvx_legitimate_address_register_p (rtx reg, bool strict)
{
  machine_mode mode = GET_MODE (reg);

  if (GET_CODE (reg) == SUBREG)
    reg = SUBREG_REG (reg);

  return (REG_P (reg) && IS_GENERAL_REGNO (REGNO (reg), strict)
	  && mode == Pmode);
}

/**
 * Legitimate address :
 * - (reg)
 * - (plus (reg) (constant))
 * - (plus (reg) (reg))
 * - (plus (mult (reg) (constant)) (reg))
 */
static bool
lvx_legitimate_address_p (machine_mode mode, rtx x, bool strict, code_helper ch = ERROR_MARK)
{
  (void) ch;
  /*
   * ld reg = 0[reg]
   */
  if (lvx_legitimate_address_register_p (x, strict))
    return true;

  /*
   * ld reg = @got[reg]
   * ld reg = @gotoff[reg]
   */
  if (GET_CODE (x) == PLUS
      && lvx_legitimate_address_register_p (XEXP (x, 0), strict)
      && (GET_CODE (XEXP (x, 1)) == UNSPEC
	  && (XINT (XEXP (x, 1), 1) == UNSPEC_GOT
	      || XINT (XEXP (x, 1), 1) == UNSPEC_GOTOFF)))
    return true;

  /*
   * ld reg = const[reg]
   * ld reg = symbol[reg]
   * ld reg = @pcrel(symbol)[reg]
   */
  if (GET_CODE (x) == PLUS
      && lvx_legitimate_address_register_p (XEXP (x, 0), strict)
      && ((CONSTANT_P (XEXP (x, 1))
	   && lvx_legitimate_constant_p (VOIDmode, XEXP (x, 1)))
	  || GET_CODE (XEXP (x, 1)) == CONST_INT)
      && immediate_operand (XEXP (x, 1), DImode)
      && (!current_pass || current_pass->tv_id != TV_CPROP))
    return true;

  /*
   * Memory accesses of size > 32 are splitted, ensure simple addressing mode.
   */
  if (GET_MODE_SIZE (mode) > 32)
    return false;

  /*
   * ld reg = reg[reg]
   */
  if (GET_CODE (x) == PLUS
      && lvx_legitimate_address_register_p (XEXP (x, 0), strict)
      && lvx_legitimate_address_register_p (XEXP (x, 1), strict))
    return true;

  /*
   * ld.xs reg = reg[reg]
   */
  if (HAVE_LVX_ADDRESSING_XS
      && GET_CODE (x) == PLUS
      && lvx_legitimate_address_register_p (XEXP (x, 1), strict)
      && GET_CODE (XEXP (x, 0)) == MULT
      && GET_CODE (XEXP (XEXP (x, 0), 1)) == CONST_INT
      && INTVAL (XEXP (XEXP (x, 0), 1)) == GET_MODE_SIZE (mode)
      && lvx_legitimate_address_register_p (XEXP (XEXP (x, 0), 0), strict))
    // The .xs addressing mode applies to object sizes 2, 4, 8, 16, 32.
    return GET_MODE_SIZE (mode) > 1 && GET_MODE_SIZE (mode) <= 32;

  return false;
}

#undef TARGET_LEGITIMATE_ADDRESS_P
#define TARGET_LEGITIMATE_ADDRESS_P lvx_legitimate_address_p

static rtx
lvx_legitimize_address (rtx x, rtx oldx ATTRIBUTE_UNUSED,
			enum machine_mode mode)
{
  if (lvx_has_tls_reference (x))
    return lvx_legitimize_tls_reference (x);
  else if (GET_CODE (x) == PLUS
	   && (GET_CODE (XEXP (x, 0)) == MULT
	       || GET_CODE (XEXP (x, 0)) == ZERO_EXTEND))
    {
      rtx reg = gen_reg_rtx (Pmode);
      rtx cst = XEXP (x, 1);

      XEXP (x, 1) = reg;
      if (lvx_legitimate_address_p (mode, x, false))
	{
	  emit_move_insn (reg, cst);
	  return copy_rtx (x);
	}
      else
	{
	  XEXP (x, 1) = cst;
	  return x;
	}
    }
  return x;
}

#undef TARGET_LEGITIMIZE_ADDRESS
#define TARGET_LEGITIMIZE_ADDRESS lvx_legitimize_address

/* Return true for the .xs addressing modes, else false.  */
static bool
lvx_mode_dependent_address_p (const_rtx addr,
			      addr_space_t space ATTRIBUTE_UNUSED)
{
  const_rtx x = addr;

  // Same logic as .xs addressing mode in lvx_print_operand
  if (HAVE_LVX_ADDRESSING_XS
      && GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 0)) == MULT
      && GET_CODE (XEXP (XEXP (x, 0), 1)) == CONST_INT
      && INTVAL (XEXP (XEXP (x, 0), 1)) > HOST_WIDE_INT_1)
    return true;

  return false;
}

#undef TARGET_MODE_DEPENDENT_ADDRESS_P
#define TARGET_MODE_DEPENDENT_ADDRESS_P lvx_mode_dependent_address_p

static bool
lvx_legitimate_constant_p (enum machine_mode mode ATTRIBUTE_UNUSED, rtx x)
{
  if (lvx_has_tls_reference (x))
    return false;

  if (lvx_has_unspec_reference (x))
    {
      if (GET_CODE (x) == CONST)
	x = XEXP (x, 0);

      if (GET_CODE (x) == UNSPEC)
	return true;

      if (GET_CODE (x) == PLUS || GET_CODE (x) == MINUS)
	return GET_CODE (XEXP (x, 0)) == UNSPEC && CONST_INT_P (XEXP (x, 1));

      return false;
    }

  return true;
}

#undef TARGET_LEGITIMATE_CONSTANT_P
#define TARGET_LEGITIMATE_CONSTANT_P lvx_legitimate_constant_p

#undef TARGET_CANNOT_FORCE_CONST_MEM
#define TARGET_CANNOT_FORCE_CONST_MEM hook_bool_mode_rtx_true

/* Implements TARGET_VECTORIZE_VEC_PERM_CONST {{{ */

/* Collect the SBMM8D immdiate values to implement a swizzle or a shuffle.
   As the largest vector type is 64 bytes and the word is 8 bytes, there
   are at most 8 words to operate in the destination vector. This corresponds
   to the row [8] dimension in values. A shuffle has up to two vector inputs,
   this corresponds to the [16] columns in values.  */
struct
{
  unsigned short from[64];
  union
  {
    unsigned char bytes[UNITS_PER_WORD];
    unsigned long long dword;
  } values[16][8];
} lvx_expand_vec_perm;

void
lvx_expand_vec_perm_print (FILE *file, machine_mode vector_mode)
{
  int nwords = GET_MODE_SIZE (vector_mode) / UNITS_PER_WORD;
  machine_mode inner_mode = GET_MODE_INNER (vector_mode);
  int nunits = GET_MODE_NUNITS (vector_mode);
  int ibytes = GET_MODE_SIZE (inner_mode);

  for (int i = 0; i < nunits * ibytes; i++)
    fprintf (file, "[%2d]", lvx_expand_vec_perm.from[i]);
  fprintf (file, " from[]\n");

  for (int orig = 0; orig < 2 * nwords; orig++)
    {
      for (int dest = 0; dest < nwords; dest++)
	{
	  for (int lane = 0; lane < UNITS_PER_WORD; lane++)
	    fprintf (file, "%03d ",
		     lvx_expand_vec_perm.values[orig][dest].bytes[lane]);
	}
      fprintf (file, "orig[%d]\n", orig);
    }
  for (int dest = 0; dest < nwords; dest++)
    {
      fprintf (file, "            dest[%d]            |", dest);
    }
  fprintf (file, "\n");
}

/* Special case of lvx_expand_vec_perm_const_emit with a single MOVE. */
rtx
lvx_expand_vec_perm_const_emit_move (rtx target, rtx source1, rtx source2,
				     int dest, int orig)
{
  machine_mode vector_mode = GET_MODE (target);
  machine_mode chunk_mode = lvx_get_chunk_mode (vector_mode);
  int nwords = GET_MODE_SIZE (vector_mode) / UNITS_PER_WORD;
  rtx op0 = simplify_gen_subreg (chunk_mode, target, vector_mode,
				 dest * UNITS_PER_WORD);
  rtx source = orig >= nwords ? source2 : source1;
  int offset = orig >= nwords ? orig - nwords : orig;
  rtx op1 = simplify_gen_subreg (chunk_mode, source, vector_mode,
				 offset * UNITS_PER_WORD);
  emit_move_insn (op0, op1);
  return op0;
}

/* Special case of lvx_expand_vec_perm_const_emit with INSF and MOVE.
   Logic based on the SBMM8D constants that have at most 1 bit set per byte. */
rtx
lvx_expand_vec_perm_const_emit_insf (rtx target, rtx source1, rtx source2,
				     int dest, int orig1, int orig2)
{
  machine_mode vector_mode = GET_MODE (target);
  int nwords = GET_MODE_SIZE (vector_mode) / UNITS_PER_WORD;
  HOST_WIDE_INT constant1 = lvx_expand_vec_perm.values[orig1][dest].dword;
  HOST_WIDE_INT constant2 = lvx_expand_vec_perm.values[orig2][dest].dword;
  HOST_WIDE_INT constanti = 0;
  int origm = -1, origi = -1;

  // The move constant is the one with a subset of bits of LVX_SBMM8D_IDENTITY.
  if ((constant1 & ((HOST_WIDE_INT) LVX_SBMM8D_IDENTITY)) == constant1)
    {
      origm = orig1, origi = orig2;
      constanti = constant2;
    }
  else if ((constant2 & ((HOST_WIDE_INT) LVX_SBMM8D_IDENTITY)) == constant2)
    {
      origm = orig2, origi = orig1;
      constanti = constant1;
    }

  if (!constanti)
    return NULL_RTX;

  // The insert constant must be LVX_SBMM8D_IDENTITY shifted left and truncated.
  int shift = __builtin_ctzll (constanti);
  int count = __builtin_popcountll (constanti);
  HOST_WIDE_INT maski
    = (-1ULL >> __builtin_clzll (constanti)) & (LVX_SBMM8D_IDENTITY << shift);

  // For speed we prevent the generation of extract as SBMM8D is faster.
  //if (optimize_insn_for_speed_p () && (shift & 7))
  //  return NULL_RTX;

  if (maski == constanti && origm >= 0 && origi >= 0)
    {
      machine_mode chunk_mode = lvx_get_chunk_mode (vector_mode);
      rtx targeti = lvx_expand_vec_perm_const_emit_move (target, source1,
							 source2, dest,
							 origm);
      rtx sourcei = origi >= nwords ? source2 : source1;
      int offseti = origi >= nwords ? origi - nwords : origi;
      rtx op0 = simplify_gen_subreg (DImode, targeti, chunk_mode, 0);
      rtx op2 = GEN_INT (count * 8);
      rtx op1 = NULL_RTX;

      // If shift is not a multiple of 8, extract is needed.
      if ((shift & 7))
	{
	  op1 = gen_reg_rtx (DImode);
	  rtx op3 = GEN_INT ((shift & 7) * 8);
	  rtx opi = simplify_gen_subreg (DImode, sourcei, vector_mode,
					 offseti * UNITS_PER_WORD);
	  rtx extract = gen_rtx_ZERO_EXTRACT (DImode, opi, op2, op3);
	  emit_insn (gen_rtx_SET (op1, extract));
	}
      else
	op1 = simplify_gen_subreg (DImode, sourcei, vector_mode,
				   offseti * UNITS_PER_WORD);

      rtx op3 = GEN_INT (shift & -8);
      rtx opi = gen_rtx_ZERO_EXTRACT (DImode, op0, op2, op3);
      emit_insn (gen_rtx_SET (opi, op1));
      return op0;
    }

  return NULL_RTX;
}

/* The generic path for one destination pair (words 2 * DPAIR and
   2 * DPAIR + 1 of TARGET), accumulated from the SBMM8 contributions of
   the origin words.

   Per word, the first contribution is SBMM8D with the matrix as its
   immediate and the rest are SBMM8EORD with the matrix in a register:
   the same count as SBMM8D-immediate plus EORD in straight-line code,
   and one op fewer per contribution in a loop, where the register matrix
   is invariant and hoisted.  The accumulator is a fresh DI so the final
   move onto the target word is a plain copy the allocator folds into the
   last op, whatever the sources overlap.

   When every contribution to the pair comes from an origin pair whose two
   words feed the two destination words directly (o0 -> d0 and o1 -> d1)
   and there are at least two such pairs, the pair is accumulated whole
   with SBMM8DP and then SBMM8EORDP on the origin pairs as they sit in the
   register file, with the matrix pairs in registers: that breaks even in
   straight-line code (a pair op and two maked for two immediate ops, then
   one op instead of two per further pair) and halves the loop body.  A
   pair with a lone or a crossed contribution stays on the word path,
   where the immediate form is free.  Words already produced by the
   special cases (SKIP0/SKIP1) are left alone.

   CHAIN selects the accumulating forms; it is set inside a loop, where the
   hoisted matrices make them pay (7-11% on the ISS).  In straight-line
   code they do not: the matrices are immediates for free, and a tied
   accumulation chain holds its register from the first op while the
   sources are still read, which costs copies when the result must land
   where the sources are (x = shuffle (x), a value returned in the
   argument registers).  There the three-address form -- SBMM8D-immediate
   into temporaries, an EORD tree, the target written last -- is what the
   allocator does best.  */
static void
lvx_expand_vec_perm_const_emit_pair (rtx target, rtx source1, rtx source2,
				     int dpair, bool skip0, bool skip1,
				     bool chain)
{
  machine_mode vector_mode = GET_MODE (target);
  int nwords = GET_MODE_SIZE (vector_mode) / UNITS_PER_WORD;
  int range = source2 ? 2 * nwords : nwords;
  int d0 = 2 * dpair, d1 = d0 + 1;

  /* Whole pair?  Every nonzero matrix must sit on a straight pair whose
     other matrix is nonzero too, and there must be at least two pairs.  */
  if (!skip0 && !skip1 && chain)
    {
      int npairs = 0;
      bool whole = true;
      for (int opair = 0; opair < range / 2 && whole; opair++)
	{
	  int o0 = 2 * opair, o1 = o0 + 1;
	  bool s0 = lvx_expand_vec_perm.values[o0][d0].dword != 0;
	  bool s1 = lvx_expand_vec_perm.values[o1][d1].dword != 0;
	  bool x0 = lvx_expand_vec_perm.values[o1][d0].dword != 0;
	  bool x1 = lvx_expand_vec_perm.values[o0][d1].dword != 0;
	  if (x0 || x1 || s0 != s1)
	    whole = false;
	  else if (s0)
	    npairs++;
	}
      if (whole && npairs >= 2)
	{
	  rtx acc = simplify_gen_subreg (V2DImode, target, vector_mode,
					 d0 * UNITS_PER_WORD);
	  bool init = false;
	  for (int opair = 0; opair < range / 2; opair++)
	    {
	      int o0 = 2 * opair, o1 = o0 + 1;
	      HOST_WIDE_INT m0 = lvx_expand_vec_perm.values[o0][d0].dword;
	      HOST_WIDE_INT m1 = lvx_expand_vec_perm.values[o1][d1].dword;
	      if (!m0)
		continue;
	      rtx source = o0 >= nwords ? source2 : source1;
	      int offset = o0 >= nwords ? o0 - nwords : o0;
	      rtx pair = simplify_gen_subreg (V2DImode, source, vector_mode,
					      offset * UNITS_PER_WORD);
	      rtx matrix
		= force_reg (V2DImode,
			     gen_rtx_CONST_VECTOR (V2DImode,
						   gen_rtvec (2, GEN_INT (m0),
							      GEN_INT (m1))));
	      if (!init)
		emit_insn (gen_lvx_sbmm8dp (acc, pair, matrix));
	      else
		emit_insn (gen_lvx_sbmm8eordp (acc, acc, pair, matrix));
	      init = true;
	    }
	  return;
	}
    }

  /* Word by word.  Chained, into the two halves of one V2DI accumulator
     so the allocator can give the pair the registers the target pair
     wants; otherwise each contribution into its own temporary and an EORD
     tree, the target written last.  */
  rtx acc2 = chain ? gen_reg_rtx (V2DImode) : NULL_RTX;
  for (int h = 0; h < 2; h++)
    {
      int d = h ? d1 : d0;
      if (h ? skip1 : skip0)
	continue;
      rtx acc = chain ? simplify_gen_subreg (DImode, acc2, V2DImode,
					     h * UNITS_PER_WORD)
		: NULL_RTX;
      bool init = false;
      for (int orig = 0; orig < range; orig++)
	{
	  HOST_WIDE_INT m = lvx_expand_vec_perm.values[orig][d].dword;
	  if (!m)
	    continue;
	  rtx source = orig >= nwords ? source2 : source1;
	  int offset = orig >= nwords ? orig - nwords : orig;
	  rtx word = simplify_gen_subreg (DImode, source, vector_mode,
					  offset * UNITS_PER_WORD);
	  if (!chain)
	    {
	      rtx tmp = gen_reg_rtx (DImode);
	      emit_insn (gen_lvx_sbmm8d (tmp, word, GEN_INT (m)));
	      if (!init)
		acc = tmp;
	      else
		{
		  rtx sum = gen_reg_rtx (DImode);
		  emit_insn (gen_xordi3 (sum, acc, tmp));
		  acc = sum;
		}
	      init = true;
	    }
	  else if (!init)
	    {
	      emit_insn (gen_lvx_sbmm8d (acc, word, GEN_INT (m)));
	      init = true;
	    }
	  else
	    emit_insn (gen_lvx_sbmm8eord (acc, acc, word,
					  force_reg (DImode, GEN_INT (m))));
	}
      gcc_assert (init);
      if (!chain)
	emit_move_insn (simplify_gen_subreg (DImode, target, vector_mode,
					     d * UNITS_PER_WORD), acc);
    }
  if (!chain)
    return;
  if (!skip0 && !skip1)
    emit_move_insn (simplify_gen_subreg (V2DImode, target, vector_mode,
					 d0 * UNITS_PER_WORD), acc2);
  else
    {
      int d = skip0 ? d1 : d0;
      emit_move_insn (simplify_gen_subreg (DImode, target, vector_mode,
					   d * UNITS_PER_WORD),
		      simplify_gen_subreg (DImode, acc2, V2DImode,
					   (d - d0) * UNITS_PER_WORD));
    }
}

/* Implements swizzle (NULL source2) or shuffle based on lvx_expand_vec_perm.
   The target words that a MOVE alone, or a MOVE with EXTFZ and INSF, can
   produce are special-cased first; the rest are accumulated a pair of words
   at a time by lvx_expand_vec_perm_const_emit_pair.  */
void
lvx_expand_vec_perm_const_emit (rtx target, rtx source1, rtx source2,
				bool chain)
{
  machine_mode vector_mode = GET_MODE (target);
  int nwords = GET_MODE_SIZE (vector_mode) / UNITS_PER_WORD;
  int range = source2 ? 2 * nwords : nwords;
  bool done[8] = { false };

  for (int dest = 0; dest < nwords; dest++)
    {
      int orig0 = -1, orig1 = -1, orig2 = -1, nconst = 0;

      for (int orig = 0; orig < range; orig++)
	{
	  HOST_WIDE_INT constant =
	    lvx_expand_vec_perm.values[orig][dest].dword;
	  if (constant)
	    {
	      if (constant == (HOST_WIDE_INT) LVX_SBMM8D_IDENTITY)
		orig0 = orig;
	      else if (orig1 < 0)
		orig1 = orig;
	      else if (orig2 < 0)
		orig2 = orig;
	      nconst++;
	    }
	}

      if (orig0 >= 0
	  && lvx_expand_vec_perm_const_emit_move (target, source1, source2,
						  dest, orig0))
	{
	  done[dest] = true;
	  continue;
	}

      // Force source1 and source2 in registers since we may insert into them.
      if (source1)
	source1 = force_reg (vector_mode, source1);
      if (source2)
	source2 = force_reg (vector_mode, source2);

      if (nconst == 2
	  && lvx_expand_vec_perm_const_emit_insf (target, source1, source2,
						  dest, orig1, orig2))
	done[dest] = true;
    }

  /* Two words at a time; a 64-bit vector has one word, taken alone.  */
  for (int dpair = 0; 2 * dpair < nwords; dpair++)
    {
      int d0 = 2 * dpair, d1 = d0 + 1;
      bool skip1 = d1 >= nwords || done[d1];
      if (done[d0] && skip1)
	continue;
      source1 = force_reg (vector_mode, source1);
      if (source2)
	source2 = force_reg (vector_mode, source2);
      lvx_expand_vec_perm_const_emit_pair (target, source1, source2, dpair,
					   done[d0], skip1, chain);
    }
}


/* The lane shuffles EVEN*Q, ODD*Q and ZIP*DQ (UZP1, UZP2, ZIP1/ZIP2), read
   off the byte map lvx_expand_vec_perm.from[] at every lane width W the
   instructions have, on a 128-bit vector, or on each 128-bit half of a
   256-bit one:

     UZP1  dest lane j = lane 2j of [source1 source2]      even<w>q s1, s2
     UZP2  dest lane j = lane 2j+1 of [source1 source2]    odd<w>q  s1, s2
     ZIP1  dest lanes 2j, 2j+1 = lane j of s1, of s2 (low halves)
                                                   zip<w>dq s1.lo, s2.lo
     ZIP2  the same on the high halves             zip<w>dq s1.hi, s2.hi

   A 256-bit UZP1 of (s1, s2) is even (s1.lo, s1.hi) then even (s2.lo,
   s2.hi); a 256-bit ZIP1 is zip (s1.w0, s2.w0) then zip (s1.w1, s2.w1),
   ZIP2 on words 2 and 3.  Double-word lanes zip as CATDQ, the even/odd
   of double words is EVENDQ/ODDDQ.  Returns true if it emitted the
   permutation.  */
static bool
lvx_expand_vec_perm_shuffle (rtx target, rtx source1, rtx source2,
			     bool single)
{
  machine_mode vector_mode = GET_MODE (target);
  unsigned size = GET_MODE_SIZE (vector_mode);
  const unsigned short *from = lvx_expand_vec_perm.from;

  if (!LVX_2 || (size != 16 && size != 32))
    return false;

  /* A swizzle's byte map indexes source1 alone: the "second source" of a
     shuffle of a vector with itself is at offset 0, not SIZE.  */
  unsigned s2off = single ? 0 : size;

  /* Does the byte map, restricted to the destination half [base, base+16)
     with sources s1, s2 (byte offsets of the two 128-bit operands in the
     concatenated [source1 source2]), read as the shuffle?  */
  auto is_even_odd = [&] (unsigned base, unsigned s1, unsigned s2, unsigned w,
			  unsigned odd)
    {
      for (unsigned j = 0; j < 16 / w; j++)
	for (unsigned k = 0; k < w; k++)
	  {
	    unsigned lane = 2 * j + odd;
	    unsigned src = lane < 16 / w ? s1 + lane * w : s2 + (lane - 16 / w) * w;
	    if (from[base + j * w + k] != src + k)
	      return false;
	  }
      return true;
    };
  auto is_zip = [&] (unsigned base, unsigned s1, unsigned s2, unsigned w,
		     unsigned half)
    {
      for (unsigned j = 0; j < 8 / w; j++)
	for (unsigned k = 0; k < w; k++)
	  {
	    if (from[base + (2 * j) * w + k] != s1 + half * 8 + j * w + k
		|| from[base + (2 * j + 1) * w + k] != s2 + half * 8 + j * w + k)
	      return false;
	  }
      return true;
    };

  auto quad = [&] (rtx v, unsigned byte_offset, machine_mode m)
    {
      return simplify_gen_subreg (m, v, GET_MODE (v), byte_offset);
    };
  auto dword = [&] (rtx v, unsigned byte_offset)
    {
      return simplify_gen_subreg (DImode, v, GET_MODE (v), byte_offset);
    };
  static const machine_mode lane_mode[4] = { V16QImode, V8HImode, V4SImode,
					     V2DImode };
  typedef rtx (*gen2) (rtx, rtx, rtx);
  static const gen2 gen_even[4] = { gen_lvx_evenbq, gen_lvx_evenhq,
				    gen_lvx_evenwq, gen_lvx_evendq };
  static const gen2 gen_odd[4] = { gen_lvx_oddbq, gen_lvx_oddhq,
				   gen_lvx_oddwq, gen_lvx_odddq };
  static const gen2 gen_zip[3] = { gen_lvx_zipbdq, gen_lvx_ziphdq,
				   gen_lvx_zipwdq };

  unsigned nhalf = size / 16;
  for (unsigned lw = 0; lw < 4; lw++)
    {
      unsigned w = 1 << lw;
      machine_mode lm = lane_mode[lw];
      bool even = true, odd = true, zip1 = true, zip2 = true;
      for (unsigned h = 0; h < nhalf; h++)
	{
	  unsigned base = h * 16;
	  if (nhalf == 1)
	    {
	      even &= is_even_odd (base, 0, s2off, w, 0);
	      odd &= is_even_odd (base, 0, s2off, w, 1);
	      zip1 &= is_zip (base, 0, s2off, w, 0);
	      zip2 &= is_zip (base, 0, s2off, w, 1);
	    }
	  else
	    {
	      /* Half h of a 256-bit UZP takes the two quads of source h;
		 half h of a 256-bit ZIP takes word h (ZIP1) or 2 + h (ZIP2)
		 of each source -- is_zip's HALF selects the word within the
		 quad it is given, so hand it the quad holding that word.  */
	      unsigned sh = h ? s2off : 0;
	      even &= is_even_odd (base, sh, sh + 16, w, 0);
	      odd &= is_even_odd (base, sh, sh + 16, w, 1);
	      zip1 &= is_zip (base, 0, s2off, w, h);
	      zip2 &= is_zip (base, 16, s2off + 16, w, h);
	    }
	}
      if (!even && !odd && !zip1 && !zip2)
	continue;

      source1 = force_reg (vector_mode, source1);
      source2 = force_reg (vector_mode, source2);

      /* A 256-bit result is two quads, each from one insn.  Compute them
	 in two fresh quads and join them with lvx_cat256 (a vec_concat that
	 ties its low half to the result), rather than writing the halves of
	 the result: a partial write of a multi-register pseudo keeps the
	 whole pseudo live before it, which stops it sharing registers with a
	 source that dies there, and every 256-bit shuffle would start with
	 four copyd.  */
      rtx halves[2];
      for (unsigned h = 0; h < nhalf; h++)
	{
	  rtx dest = nhalf == 1 ? quad (target, 0, lm) : gen_reg_rtx (lm);
	  halves[h] = dest;
	  if (even || odd)
	    {
	      rtx a, b;
	      if (nhalf == 1)
		a = quad (source1, 0, lm), b = quad (source2, 0, lm);
	      else
		{
		  rtx src = h ? source2 : source1;
		  a = quad (src, 0, lm), b = quad (src, 16, lm);
		}
	      emit_insn ((even ? gen_even : gen_odd)[lw] (dest, a, b));
	    }
	  else
	    {
	      /* 128-bit: word 0 (ZIP1) or 1 (ZIP2) of each source; 256-bit:
		 word h (ZIP1) or 2 + h (ZIP2).  */
	      unsigned word = nhalf == 1 ? (zip1 ? 0 : 8) : (zip1 ? 0 : 16) + h * 8;
	      rtx a = dword (source1, word), b = dword (source2, word);
	      if (lw == 3)
		emit_insn (gen_lvx_catdq (dest, a, b));
	      else
		emit_insn (gen_zip[lw] (dest, a, b));
	    }
	}
      if (nhalf == 2)
	emit_insn (gen_lvx_cat256 (simplify_gen_subreg (V4DImode, target,
							vector_mode, 0),
				   simplify_gen_subreg (V2DImode, halves[0], lm, 0),
				   simplify_gen_subreg (V2DImode, halves[1], lm, 0)));
      return true;
    }
  return false;
}

/* Called by the vec_perm_const<mode> standard pattern.
   First step identifies whether this is a swizzle (one source) or a shuffle.
   Second step fills the lvx_expand_vec_perm structure with SBMM8D immediates.
   Third step emits the permutation with (lvx_expand_vec_perm_const_emit).  */
bool
lvx_expand_vec_perm_const (rtx target, rtx source1, rtx source2, rtx selector)
{
  machine_mode vector_mode = GET_MODE (target);
  machine_mode inner_mode = GET_MODE_INNER (vector_mode);
  int nwords = GET_MODE_SIZE (vector_mode) / UNITS_PER_WORD;
  int nunits = GET_MODE_NUNITS (vector_mode);
  int ibytes = GET_MODE_SIZE (inner_mode);
  unsigned idx_mask = 2 * nunits - 1, which = 0;

  gcc_assert (nwords <= 8);
  gcc_assert (nunits * ibytes <= 64);
  memset (&lvx_expand_vec_perm, 0, sizeof (lvx_expand_vec_perm));

  // Fill the lvx_expand_vec_perm.from[] array, where each byte of the
  // destination records the index of the source byte in [source1,source2].
  for (int i = 0; i < nunits; i++)
    {
      rtx elt = XVECEXP (selector, 0, i);
      int index = INTVAL (elt) & idx_mask;
      which |= 1 << (index >= nunits);
      for (int j = 0; j < ibytes; j++)
	lvx_expand_vec_perm.from[i * ibytes + j] = index * ibytes + j;
    }

  // In case the two sources are equal, only index into the first source.
  if (rtx_equal_p (source1, source2) && which == 2)
    {
      for (int i = 0; i < nunits * ibytes; i++)
	lvx_expand_vec_perm.from[i] &= nunits * ibytes - 1;
      which = 1;
    }

  // Use the lvx_expand_vec_perm.from[] array to compute the SBMM8D constants.
  // There is one SBMM8D constant per origin word and per destination word.
  for (int i = 0; i < nwords; i++)
    {
      for (int j = 0; j < nunits * ibytes; j++)
	{
	  int k = lvx_expand_vec_perm.from[j];
	  int dest = j / UNITS_PER_WORD;
	  int lane = j % UNITS_PER_WORD;
	  int orig = k / UNITS_PER_WORD;
	  int shift = k % UNITS_PER_WORD;
	  gcc_assert (orig < 2 * nwords && dest < nwords);
	  lvx_expand_vec_perm.values[orig][dest].bytes[lane] = 1 << shift;
	}
    }

#if 0
  lvx_expand_vec_perm_print (stderr, vector_mode);
#endif

  bool overlap = reg_overlap_mentioned_p (target, source1);
  if (which != 1)
    overlap |= reg_overlap_mentioned_p (target, source2);

  rtx temporary = target;
  if (overlap)
    temporary = gen_reg_rtx (vector_mode);

  /* One of the ISA's lane shuffles?  Each writes its quad in one insn, so
     it handles overlap of target and sources itself.  */
  if (lvx_expand_vec_perm_shuffle (target, source1,
				   which == 1 ? source1 : source2, which == 1))
    return true;

  /* Pure lane permutation: every destination 64-bit word is a whole source
     word, its bytes in order.  A v2i64 lane reverse is just the pair swap
     (R0R1 -> R1R0), and any 64-bit-lane shuffle is a handful of register
     moves -- but the SBMM8D path below would byte-reverse the register and
     then un-reverse each lane to get back there (9 insns for a 2-move swap).
     Emit word moves through a fresh temporary instead, so an in-place swap
     does not clobber a word it still has to read.  */
  {
    int srcword[8];
    bool pure = true;
    for (int d = 0; d < nwords && pure; d++)
      {
	int sw = lvx_expand_vec_perm.from[d * UNITS_PER_WORD] / UNITS_PER_WORD;
	for (int j = 0; j < UNITS_PER_WORD; j++)
	  if (lvx_expand_vec_perm.from[d * UNITS_PER_WORD + j]
	      != sw * UNITS_PER_WORD + j)
	    { pure = false; break; }
	srcword[d] = sw;
      }
    if (pure)
      {
	rtx tmp = gen_reg_rtx (vector_mode);
	rtx dsub[8], ssub[8];
	bool ok = true;
	for (int d = 0; d < nwords && ok; d++)
	  {
	    int sw = srcword[d];
	    rtx src = sw < nwords ? source1 : source2;
	    int woff = (sw < nwords ? sw : sw - nwords) * UNITS_PER_WORD;
	    dsub[d] = simplify_gen_subreg (word_mode, tmp, vector_mode,
					   d * UNITS_PER_WORD);
	    ssub[d] = simplify_gen_subreg (word_mode, src, vector_mode, woff);
	    ok = dsub[d] && ssub[d];
	  }
	if (ok)
	  {
	    for (int d = 0; d < nwords; d++)
	      emit_move_insn (dsub[d], ssub[d]);
	    emit_move_insn (target, tmp);
	    return true;
	  }
      }
  }

  /* Whole-vector byte shift.  A single-source byte rotate, or a byte-granular
     two-source funnel/align (the palignr idiom), is a shift of the whole
     128-bit register pair -- which SLLQ and SRLQ each do in one instruction --
     ORed back together with a single IORQ.  Four instructions (one of the two
     shift counts always exceeds the 6-bit immediate field and so needs a MAKED
     for its register form) where the SBMM8D path spends six.  Only for a full
     128-bit vector, where the pair is exactly a TImode shift: a wider vector
     has no 256-bit shift.  The shifts read the sources into fresh registers
     before the IORQ writes the result, so this handles target/source overlap
     itself and needs no temporary.  */
  if (GET_MODE_SIZE (vector_mode) == 16)
    {
      const int N = 16;
      rtx sll_src = NULL_RTX, srl_src = NULL_RTX;
      int sll_bits = 0, srl_bits = 0;

      /* Case A -- one source: a byte rotate, from[j] == (j - k) mod 16.  The
	 word swap (k == 8) is a pure word permutation and never reaches here.  */
      if (which == 1)
	{
	  int k = (N - lvx_expand_vec_perm.from[0]) % N;
	  bool rot = k != 0;
	  for (int j = 0; rot && j < N; j++)
	    if (lvx_expand_vec_perm.from[j] != (j - k + N) % N)
	      rot = false;
	  if (rot)
	    {
	      sll_src = source1; sll_bits = k * 8;
	      srl_src = source1; srl_bits = (N - k) * 8;
	    }
	}
      /* Case B -- both sources: an align, from[j] == j + k, with [0,15] the
	 bytes of source1 and [16,31] those of source2.  dest = (source1 >> k)
	 | (source2 << (16 - k)).  If source1 == source2 this is the same rotate
	 and stays correct.  */
      else if (which == 3)
	{
	  int k = lvx_expand_vec_perm.from[0];
	  bool aln = k >= 1 && k <= N - 1;
	  for (int j = 0; aln && j < N; j++)
	    if (lvx_expand_vec_perm.from[j] != j + k)
	      aln = false;
	  if (aln)
	    {
	      sll_src = source2; sll_bits = (N - k) * 8;
	      srl_src = source1; srl_bits = k * 8;
	    }
	}

      if (sll_src)
	{
	  rtx tsll = simplify_gen_subreg (TImode, sll_src, vector_mode, 0);
	  rtx tsrl = simplify_gen_subreg (TImode, srl_src, vector_mode, 0);
	  rtx tdst = simplify_gen_subreg (TImode, target, vector_mode, 0);
	  if (tsll && tsrl && tdst)
	    {
	      /* A count of 64 or more does not fit the U06 immediate, so it
		 goes through a register; exactly one of the pair always does.  */
	      rtx cl = sll_bits < 64 ? GEN_INT (sll_bits)
				     : force_reg (SImode, GEN_INT (sll_bits));
	      rtx cr = srl_bits < 64 ? GEN_INT (srl_bits)
				     : force_reg (SImode, GEN_INT (srl_bits));
	      rtx a = gen_reg_rtx (TImode), b = gen_reg_rtx (TImode);
	      /* Same as in lvx_expand_vector_shift: these three come from
		 simplify_gen_subreg, which hands back a MEM for a MEM, and
		 gen_* checks no predicate.  */
	      if (!register_operand (tsll, TImode))
		tsll = force_reg (TImode, tsll);
	      if (!register_operand (tsrl, TImode))
		tsrl = force_reg (TImode, tsrl);
	      rtx d = tdst;
	      if (!register_operand (d, TImode))
		d = gen_reg_rtx (TImode);
	      emit_insn (gen_ashlti3 (a, tsll, cl));
	      emit_insn (gen_lshrti3 (b, tsrl, cr));
	      emit_insn (gen_iorti3 (d, a, b));
	      if (d != tdst)
		emit_move_insn (tdst, d);
	      return true;
	    }
	}
    }

  /* The accumulating SBMM8EORD forms pay inside a loop, where their
     register matrices are hoisted; see lvx_expand_vec_perm_const_emit_pair.  */
  bool chain = (currently_expanding_gimple_stmt
		&& gimple_bb (currently_expanding_gimple_stmt)
		&& bb_loop_depth (gimple_bb (currently_expanding_gimple_stmt)) > 0);

  if (which == 1)
    lvx_expand_vec_perm_const_emit (temporary, source1, NULL_RTX, chain);
  else
    lvx_expand_vec_perm_const_emit (temporary, source1, source2, chain);

  if (overlap)
    emit_move_insn (target, temporary);

  return true;
}


static bool
lvx_vectorize_vec_perm_const (machine_mode vmode, machine_mode op_mode,
			      rtx target, rtx op0, rtx op1,
			      const vec_perm_indices &sel)
{

  if (vmode != op_mode)
    return false;

  if (GET_MODE_SIZE (vmode) > 64)
    return false;

  if (vmode == E_V2QImode || vmode == E_V2HImode)
    return false;		// TODO Allow vec perm for these modes.

  /* The selector is a vector of lane indices in the integer mode of the
     same shape.  There need not be one: on lvx-1, V2SF exists for float
     complex while every integer vector mode is lvx-2-only, so
     related_int_vector_mode (V2SFmode) is empty, and handing its VOIDmode
     to vec_perm_indices_to_rtx is an ICE from a query
     (can_vec_perm_const_p, match.pd's VEC_PERM_EXPR folding).  Such a
     permute is not ours to do; the middle end scalarises it.  */
  opt_machine_mode smode = related_int_vector_mode (vmode);
  if (!smode.exists ())
    return false;
  rtx sel_rtx = vec_perm_indices_to_rtx (smode.require (), sel);
  return target
    ? lvx_expand_vec_perm_const (target, op0, op1, sel_rtx)
    : true;
}

#undef TARGET_VECTORIZE_VEC_PERM_CONST
#define TARGET_VECTORIZE_VEC_PERM_CONST lvx_vectorize_vec_perm_const
/* Implements TARGET_VECTORIZE_VEC_PERM_CONST }}} */

static bool
lvx_support_vector_misalignment (enum machine_mode, int, bool, bool)
{
  return !TARGET_STRICT_ALIGN;
}

#undef TARGET_VECTORIZE_SUPPORT_VECTOR_MISALIGNMENT
#define TARGET_VECTORIZE_SUPPORT_VECTOR_MISALIGNMENT \
  lvx_support_vector_misalignment

/* Implements TARGET_VECTORIZE_BUILTIN_VECTORIZATION_COST {{{

   The generic costs in targhooks.cc charge an unaligned vector access twice an
   aligned one.  LVX does not: lq/sq take any address the LSU accepts -- only
   the sized atomics check alignment, and they THROW MISALIGN rather than run
   slowly -- and the description gives each one latency without an alignment
   term, which is what the ISS charges too (gem5's MinorCPU takes opLat from
   the write stage).  lvx_support_vector_misalignment already tells the
   vectorizer the hardware supports it; charging double for it afterwards is
   the one place LVX was paying a cost it does not have.

   It is not a rounding error at 64-bit lanes.  A V2DF body is two lanes, so it
   is weighed against two scalar iterations, and two cost units out of nine is
   the difference between a saving of 1 per vector iteration and a saving of 3
   -- which at -O2, where the cost model is `very-cheap' and demands the static
   profitability threshold be no more than the vectorization factor, decides
   the loop.  syrk and syr2k at double are exactly that case.

   vector_stmt is charged per ALU slot, not per operation: see the case below.

   vec_construct and vec_deconstruct now have the measurement the previous
   version of this comment asked for, and it says leave them alone.  Building a
   V4SF from four GPR-resident lanes is two INSFD and two COPYD, and taking one
   apart is two ZXWD and two EXTFZD -- n instructions for n lanes, against the
   generic n-1 (targhooks.cc).  One instruction in four is not worth a target
   hook, and PolyBench's symm, the loop that prompted the measurement, is
   mispriced by far more than that at the width level.

   vec_perm keeps the generic value, and is the one still owed a measurement:
   it is what a VLIW hides best by packing into otherwise idle slots, but
   "hidden by the bundler" is a claim about the scheduler, not about the
   instruction.  */

static int
lvx_builtin_vectorization_cost (enum vect_cost_for_stmt type_of_cost,
				tree vectype, int misalign)
{
  switch (type_of_cost)
    {
    case unaligned_load:
    case unaligned_store:
      /* The same as the aligned access this would otherwise have been.  With
	 -mstrict-align the vectorizer never asks: lvx_support_vector_
	 misalignment refuses the access and it is peeled or scalarized.  */
      return 1;

    case vector_stmt:
      /* A 256-bit ALU operation is TWO LITE halves issued together -- `issue
	 lite2' in the machine description, which reserves lvx_tiny_x2_u +
	 lvx_lite_x2_u, i.e. both ALU slots -- so the quad buys no ALU
	 throughput over the pair.  That is not a new claim: it is what
	 lvx_autovectorize_vector_modes says when it offers V32QI as an
	 alternative rather than imposing it, and what it relies on
	 VECT_COMPARE_COSTS to act on.  What the quad genuinely buys is on the
	 memory side -- one lo/so per 32 bytes instead of two lq/sq, one L*SO
	 broadcast, half the iterations.

	 Charging a quad operation the same 1 as a pair operation told the
	 comparison the opposite.  Per element a 256-bit body came out cheaper
	 by construction, so the quad won loops with no memory-side gain to pay
	 for it -- including loops with no vector memory access at all.
	 PolyBench's symm at MINI/float is the case that found this: its k-loop
	 walks C[k][j] and B[k][j] down a column, stride N, so every lane is a
	 separate LWZ and there is no lo/so anywhere in the body.  The
	 vectorizer costed V4SF at 34 for 4 lanes (8.50/lane) against V32QI at
	 66 for 8 (8.25/lane), preferred the quad by that 0.25, and paid for it
	 with a 4-element epilogue costed 47 on a loop whose average trip count
	 is 9.5 -- measured, 75966 -> 104270 kernel cycles, +37.3%.

	 With the four body statements charged 2 instead of 1, V32QI is 70/8 =
	 8.75 and the pair wins, which is what the mode-offering hook intended.
	 The margin either way is thin, and deliberately so: this restores the
	 comparison the quad was meant to lose when the memory side does not
	 carry it, and leaves it winning when it does.  */
      if (vectype
	  && known_eq (GET_MODE_SIZE (TYPE_MODE (vectype)),
		       (unsigned HOST_WIDE_INT) 32))
	return 2;
      return default_builtin_vectorization_cost (type_of_cost, vectype,
						 misalign);

    case vec_to_scalar:
      /* A horizontal reduction, and on this target it is not one instruction.
	 There is no reduc_plus_scal_<mode> here, so GCC reduces with its
	 generic log-step shift-and-add tree over vec_shr_<mode> and then
	 extracts lane 0: log2(nunits) shift-and-add steps plus the extract.
	 Measured for V4DF -- two shift steps, two FADDDP and a COPYD, seven
	 instructions with two dependent FP adds on the chain.

	 The default charges 1, and that is what made a masked epilogue look
	 free: a vectorized reduction epilogue needs its OWN horizontal
	 reduction, so the whole tree was priced at 1 and GCC put the
	 profitability threshold at a single iteration.  Measured, that
	 epilogue costs ~25-30 cycles more than the scalar tail it replaces
	 when the tail is one to three elements (validation/bench/condadd.c:
	 258 against 233 at n=65).

	 Three per step rather than two: a shift below 64-bit lanes is one
	 SRLQ but at 64-bit lanes it is a pair of register moves, and the add
	 sits on the FP latency.  Scaled by lane count rather than fixed, so a
	 2-lane reduction is not charged what an 8-lane one costs.  */
      if (vectype)
	{
	  unsigned HOST_WIDE_INT n;
	  if (TYPE_VECTOR_SUBPARTS (vectype).is_constant (&n) && n > 1)
	    return 3 * ceil_log2 (n) + 1;
	}
      return default_builtin_vectorization_cost (type_of_cost, vectype,
						 misalign);

    default:
      return default_builtin_vectorization_cost (type_of_cost, vectype,
						 misalign);
    }
}

#undef TARGET_VECTORIZE_BUILTIN_VECTORIZATION_COST
#define TARGET_VECTORIZE_BUILTIN_VECTORIZATION_COST \
  lvx_builtin_vectorization_cost
/* Implements TARGET_VECTORIZE_BUILTIN_VECTORIZATION_COST }}} */

/* Implements TARGET_VECTORIZE_CREATE_COSTS {{{

   LVX has no gather and no scatter.  When the vectorizer meets a strided
   access it therefore scalarises it: one LWZ or SW per lane, then INSFD and
   COPYD to assemble the pair, and ZXWD/EXTFZD to take a result apart again.
   The generic model prices that at its instruction count and no more --
   n scalar accesses at 1 each plus n-1 for the vec_construct -- and the
   instruction count is right.  The cost is not, because the assembly is a
   dependence chain on the critical path:

     lwz -> insfd -> copyd -> ffmawq -> zxwd/extfzd -> sw

   while the scalar loop it replaces has independent iterations that the
   bundler packs.  "Hidden by the bundler" is the usual VLIW argument for
   charging such glue less; this is the case where the argument runs the other
   way, and it is the scalar alternative that packs.

   PolyBench symm at MINI is the measured case.  Its k-loop walks C[k][j] and
   B[k][j] down a column, and at V4SF the model totals 34 for four lanes
   against a scalar 44 -- so it vectorises, and loses by 26% (75966 -> 96108
   kernel cycles on the ISS).  The emitted instruction counts agree with both
   figures; only the cycles disagree.

   builtin_vectorization_cost cannot fix this, which is why this class exists:
   its arguments are (kind, vectype, misalign), and a scalarised strided
   access asks for exactly the same scalar_load/vec_construct an ordinary
   vector build from unrelated scalars asks for.  The access type lives on the
   SLP node, and add_stmt_cost is the only cost interface that is handed one.
   rs6000 (rs6000.cc, "Power processors do not currently have instructions for
   strided and elementwise loads ... This leads to undercounting of the cost")
   and i386 both do the same surcharge in the same place.

   The surcharge is LVX_STRIDED_LANE_COST per lane on the construct or
   deconstruct, i.e. a charge per lane rather than the generic n-1 for the
   whole group, and it applies only to VMAT_ELEMENTWISE and VMAT_STRIDED_SLP.
   A contiguous access is untouched by construction, which is what keeps this
   away from the loops the 128-bit SIMD actually wins: gemm, syrk, syr2k and
   the stencils are all contiguous.

   Unlike i386 this does not also require DR_STEP to be non-constant.  i386
   penalises only an unknown stride; symm's stride is a compile-time constant
   at MINI and is scalarised just the same, so the guard would make this hook
   do nothing for the case that motivated it.  rs6000 has no such guard
   either.  */

/* Charged per lane of a scalarised strided construct or deconstruct, on top
   of the generic group cost.

   One, and it is the minimum that works rather than a free parameter.  symm's
   V4SF body has to exceed the scalar 44 to be declined, and with four
   scalarised groups at the generic 3 that is 34 - 12 + 4 * (3 + s) > 44, so
   s > 2.5: a per-lane charge of 1 gives s = 4 and flips it, and nothing
   smaller does.

   Two was tried first, following rs6000's "for each scalar load, we use 2 as
   penalized cost", and overshoots to s = 8.  It fixes symm just as well and
   takes syrk and syr2k with it -- kernels whose strided accesses vectorise
   *profitably* here.  Measured over the four lvx-2 cells, 2 against 1:

                               cost=2     cost=1
     syrk   double -restrict   +23.95%     0.00%
     syr2k  double -restrict   +20.35%    -0.09%
     syr2k  double              +8.06%    +0.20%
     syrk   float               +7.06%     0.00%
     syr2k  float -restrict     +3.88%     0.00%
     cholesky double -restrict  +1.15%     0.00%

   and at 2 the -restrict series -- the one that measures the compiler rather
   than PolyBench's own aliasing -- came out net *worse* than with no surcharge
   at all.  So the number is load-bearing in both directions, and symm alone
   cannot set it: symm is satisfied by anything above 2.5, and what picks 1 out
   of that range is the four kernels that must not move.  */
#define LVX_STRIDED_LANE_COST 1

class lvx_vector_costs : public vector_costs
{
public:
  using vector_costs::vector_costs;

  unsigned int add_stmt_cost (int count, vect_cost_for_stmt kind,
			      stmt_vec_info stmt_info, slp_tree node,
			      tree vectype, int misalign,
			      vect_cost_model_location where) override;
};

/* The extra cost, if any, of KIND on NODE beyond what the generic model
   charges.  Non-zero only for the lane assembly of a scalarised strided
   access.  */
static unsigned int
lvx_strided_lane_surcharge (vect_cost_for_stmt kind, slp_tree node,
			    tree vectype)
{
  if (kind != vec_construct && kind != vec_deconstruct)
    return 0;
  if (!node || !vectype)
    return 0;

  vect_memory_access_type mat = SLP_TREE_MEMORY_ACCESS_TYPE (node);
  if (mat != VMAT_ELEMENTWISE && mat != VMAT_STRIDED_SLP)
    return 0;

  /* A one-lane "vector" needs no assembly, and penalising it would charge for
     work that is not emitted -- rs6000 hit this as PR103702.  */
  unsigned HOST_WIDE_INT nunits;
  if (!TYPE_VECTOR_SUBPARTS (vectype).is_constant (&nunits) || nunits <= 1)
    return 0;

  return LVX_STRIDED_LANE_COST * nunits;
}

unsigned int
lvx_vector_costs::add_stmt_cost (int count, vect_cost_for_stmt kind,
				 stmt_vec_info stmt_info, slp_tree node,
				 tree vectype, int misalign,
				 vect_cost_model_location where)
{
  unsigned int cost = builtin_vectorization_cost (kind, vectype, misalign);
  cost += lvx_strided_lane_surcharge (kind, node, vectype);
  return record_stmt_cost (stmt_info, where, cost * count);
}

static vector_costs *
lvx_vectorize_create_costs (vec_info *vinfo, bool costing_for_scalar)
{
  return new lvx_vector_costs (vinfo, costing_for_scalar);
}

#undef TARGET_VECTORIZE_CREATE_COSTS
#define TARGET_VECTORIZE_CREATE_COSTS lvx_vectorize_create_costs
/* Implements TARGET_VECTORIZE_CREATE_COSTS }}} */

/* A 64-bit vector conversion on the 128-bit unit: OPERANDS[1] goes into the low
   half of a zeroed SMODE pair, one conversion runs on every lane, and the low
   half of the DMODE result is the answer.

   Here the zeroing is not optional, where lvx_expand_widen64 leaves the upper
   lanes undefined: a conversion writes the CS exception flags, so garbage above
   would raise Inexact (float of a value needing rounding) or Invalid (fix of a
   NaN or an out-of-range float) for lanes the program never asked about.  */
void
lvx_expand_cvt64 (enum rtx_code code, machine_mode smode, machine_mode dmode,
		  rtx *operands)
{
  machine_mode nmode = GET_MODE (operands[1]);
  machine_mode omode = GET_MODE (operands[0]);

  rtx wide = gen_reg_rtx (smode);
  emit_move_insn (wide, CONST0_RTX (smode));
  emit_move_insn (simplify_gen_subreg (nmode, wide, smode, 0),
		  force_reg (nmode, operands[1]));

  rtx dest = gen_reg_rtx (dmode);
  emit_insn (gen_rtx_SET (dest, gen_rtx_fmt_e (code, dmode, wide)));
  emit_move_insn (operands[0], simplify_gen_subreg (omode, dest, dmode, 0));
}

/* Expand OPERANDS[0] = (CODE OPERANDS[1] OPERANDS[2]) for a 64-bit vector
   mode by doing it on the 128-bit unit WMODE: each vector operand goes in
   the low half of a fresh pair, the packed instruction runs on all its
   lanes, and the low half of the result is the answer.

   The upper lanes are left undefined on purpose.  Integer arithmetic raises
   nothing, so what they compute cannot be observed, and zeroing them would
   cost an instruction apiece -- where the V2SF patterns must zero, floating
   point writing the CS exception flags.

   A shift count stays a scalar and is passed through.  */
void
lvx_expand_widen64 (enum rtx_code code, machine_mode wmode, rtx *operands)
{
  machine_mode mode = GET_MODE (operands[0]);

  auto widen = [&] (rtx op) {
    rtx wide = gen_reg_rtx (wmode);
    emit_move_insn (simplify_gen_subreg (mode, wide, wmode, 0),
		    force_reg (mode, op));
    return wide;
  };

  rtx op1 = widen (operands[1]);
  rtx op2 = operands[2];
  if (GET_MODE (op2) == mode)
    /* A lane operand widens like the first; a shift count is one SImode
       value and goes through unchanged, as the 128-bit shifts take it.  */
    op2 = widen (op2);

  /* expand_simple_binop rather than a hand-built SET: it prepares the
     operands the way the 128-bit pattern's predicates want them.  */
  rtx dest = expand_simple_binop (wmode, code, op1, op2, NULL_RTX,
				  code == LSHIFTRT || code == UMIN
				  || code == UMAX, OPTAB_DIRECT);
  gcc_assert (dest != NULL_RTX);
  emit_move_insn (operands[0], simplify_gen_subreg (mode, dest, wmode, 0));
}

static machine_mode
lvx_vectorize_preferred_simd_mode (scalar_mode mode)
{
  scalar_mode inner_mode = GET_MODE_INNER (mode);
  unsigned inner_size = GET_MODE_SIZE (inner_mode);
  unsigned nunits = (UNITS_PER_WORD * 2) / inner_size;
  if (nunits <= 1)
    return word_mode;
  return mode_for_vector (inner_mode, nunits).require ();
}

#undef TARGET_VECTORIZE_PREFERRED_SIMD_MODE
#define TARGET_VECTORIZE_PREFERRED_SIMD_MODE lvx_vectorize_preferred_simd_mode

/* An integer SIMD select uses a bit-per-lane mask in a GPR: COMP* packs one bit
   per lane, BLEND* reads it, so the vectorizer should carry the mask as the
   integer mode with one bit per lane rather than a full-width 0/-1 vector.  That
   is QImode for a mode of up to 8 lanes and HImode for V16QI's 16 -- always a
   single GPR (the architectural max is 64 lanes / DImode).  Restricted to the
   128-bit integer modes for now; float and wider SIMD keep the default
   full-width vector mask.  */
static opt_machine_mode
lvx_get_mask_mode (machine_mode mode)
{
  if (LVX_2 && (GET_MODE_SIZE (mode) == 16 || GET_MODE_SIZE (mode) == 32)
      && GET_MODE_NUNITS (mode) >= 2
      && (GET_MODE_CLASS (mode) == MODE_VECTOR_INT
	  || GET_MODE_CLASS (mode) == MODE_VECTOR_FLOAT))
    {
      /* One bit per lane in a GPR.  A 256-bit vector's mask is the two 128-bit
	 halves' COMP*, FCOMP* masks stitched with INSFD (lvx_lower_comparison).  */
      unsigned nunits = GET_MODE_NUNITS (mode);
      return int_mode_for_size (nunits < 8 ? 8 : nunits, 0).require ();
    }
  return default_get_mask_mode (mode);
}

#undef TARGET_VECTORIZE_GET_MASK_MODE
#define TARGET_VECTORIZE_GET_MASK_MODE lvx_get_mask_mode

/* Implements TARGET_VECTORIZE_AUTOVECTORIZE_VECTOR_MODES.

   The pair (128 bits) stays the preferred width: lvx-2's SIMD arithmetic
   is 128-bit, and a 256-bit operation is two LITE halves that dual-issue
   in one bundle, so the quad buys no ALU throughput.  What it buys is on
   the memory side and in loop overhead -- one lo/so per 32 bytes instead
   of two lq/sq, one L*SO broadcast, half the iterations: 1.07x-1.29x on
   streaming loops on the ISS, nothing on reductions, at the price of
   quad-aligned register groups and 32-byte epilogues.  So the quad is
   offered as an alternative, not imposed: with VECT_COMPARE_COSTS the
   vectorizer costs both widths per loop and takes the cheaper, and
   vectorises the epilogue of a 256-bit loop at 128 bits.  The first entry
   must be the preferred mode; lvx-1 has no SIMD at all.  */
static unsigned int
lvx_autovectorize_vector_modes (vector_modes *modes, bool)
{
  if (!LVX_2)
    return 0;
  modes->safe_push (V16QImode);
  modes->safe_push (V32QImode);
  return VECT_COMPARE_COSTS;
}

#undef TARGET_VECTORIZE_AUTOVECTORIZE_VECTOR_MODES
#define TARGET_VECTORIZE_AUTOVECTORIZE_VECTOR_MODES \
  lvx_autovectorize_vector_modes

/* Return 1 if TRAIT NAME is present in the OpenMP context's
   device trait set, return 0 if not present in any OpenMP context in the
   whole translation unit, or -1 if not present in the current OpenMP context
   but might be present in another OpenMP context in the same TU.  */

int
lvx_omp_device_kind_arch_isa (enum omp_device_kind_arch_isa trait,
			      const char *name)
{
  switch (trait)
    {
    case omp_device_kind:
      return strcmp (name, "gpu") == 0;
    case omp_device_arch:
      return strcmp (name, "lvx") == 0;
    case omp_device_isa:
      if (!strcmp (name, "lvx-1"))
	return LVX_ARCH_LVX_1;
      if (!strcmp (name, "lvx-2"))
	return LVX_ARCH_LVX_2;
      return 0;
    default:
      gcc_unreachable ();
    }
}

#undef TARGET_OMP_DEVICE_KIND_ARCH_ISA
#define TARGET_OMP_DEVICE_KIND_ARCH_ISA lvx_omp_device_kind_arch_isa

/* 18.13 Addressing Modes }}} */


/* 18.15 Condition Code Status {{{ */

/* 18.15.1 Representation of condition codes using registers {{{ */

/* Canonicalize a comparison from one we don't have to one we do have.  */
static void
lvx_canonicalize_comparison (int *code, rtx *op0, rtx *op1,
			     bool op0_preserve_value)
{
  machine_mode mode = GET_MODE (*op0);
  bool float_compare_p = (GET_MODE_CLASS (mode) == MODE_FLOAT);
  static struct rtx_def rtx_;
  PUT_CODE (&rtx_, (enum rtx_code) *code);

  if (!op0_preserve_value && float_compare_p
      && !float_comparison_operator (&rtx_, VOIDmode))
    {
      rtx temp = *op1;
      *op1 = *op0;
      *op0 = temp;
      *code = (int) swap_condition ((enum rtx_code) *code);
    }
}

#undef TARGET_CANONICALIZE_COMPARISON
#define TARGET_CANONICALIZE_COMPARISON lvx_canonicalize_comparison

/* 18.15.1 Representation of condition codes using registers }}} */

/* 18.15 Condition Code Status }}} */


/* 18.16 Describing Relative Costs of Operations {{{ */

/* Utils {{{ */

#ifndef __OPTIMIZE__
static int lvx_cost_factor = 0;
#endif //__OPTIMIZE__
//#define COST_FACTOR(n) (lvx_cost_factor ? (n + lvx_cost_factor - 1)/lvx_cost_factor : n)
#define COST_FACTOR(n) (((n) + 1) >> 1)

/* The latency of each instruction type, in cycles, as scheduling.md's
   define_insn_reservation states it.  lvx_insn_cost reads that file through
   insn_default_latency, which needs an insn; lvx_rtx_costs is handed a bare
   rtx and cannot, so it reads this table instead.

   The table is therefore a copy, and a copy drifts: MDS/BE/GCC/BIN/
   check-scheduling.py compares every entry against scheduling.md and fails
   if they disagree, which is what keeps it honest.  Where a type has several
   latencies the entry is the common one and the caller handles the rest --
   a load is 3 cached and 24 uncached, and the MEM arm reads the address
   space to tell them apart.  */
static const int lvx_type_latency[] = {
  5,   /* TYPE_ALL      */  1,   /* TYPE_NOP      */  0,   /* TYPE_GHOST    */
  1,   /* TYPE_ALU      */  2,   /* TYPE_IMUL     */  2,   /* TYPE_IMADD    */
  11,  /* TYPE_IDIV     */  4,   /* TYPE_FMULS    */  4,   /* TYPE_FMADDS   */
  4,   /* TYPE_FMULD    */
  4,   /* TYPE_FMADDD   */  4,   /* TYPE_FCVT     */  4,   /* TYPE_FDOTP    */
  4,   /* TYPE_FDMDA    */  13,  /* TYPE_FDIV     */  3,   /* TYPE_LOAD     */
  4,   /* TYPE_XLOAD    */  1,   /* TYPE_STORE    */  1,   /* TYPE_XSTORE   */
  24,  /* TYPE_ALOAD    */  24,  /* TYPE_ALOADC   */  24,  /* TYPE_ATOMIC   */
  3,   /* TYPE_COPY     */  1,   /* TYPE_CACHE    */  1,   /* TYPE_PREFETCH */
  1,   /* TYPE_BRANCH   */  5,   /* TYPE_BRANCH2  */  1,   /* TYPE_JUMP     */
  1,   /* TYPE_IJUMP    */  1,   /* TYPE_SYSGET   */  1,   /* TYPE_XMOVETO  */
  3,   /* TYPE_XMOVEF   */  3,   /* TYPE_XCOPY    */  1,   /* TYPE_EXT      */
  3,   /* TYPE_EXT_INT  */  3,   /* TYPE_EXT_FLOAT */
};

/* How scarce the units one instruction of scheduling class ISSUE occupies
   are, in COSTS_N_INSNS units.  Read out of lvx_issue_table, which BE/GCC
   generates from the Machine Description System, so an ISA change moves it;
   this is the same weighting lvx_insn_cost applies to a real insn.  */
static int
lvx_issue_weight (enum attr_issue issue)
{
  const struct lvx_issue_resources *r = &lvx_issue_table[(int) issue];

  if (issue == ISSUE_ALL)
    /* A singleton bundle: it costs the whole issue width.  */
    return 4;
  if (r->full || r->lsu || r->bcu || r->ext)
    /* A unit there are one or two of, or a port.  */
    return COST_FACTOR (4);
  /* TINY and LITE: the plentiful ALU slots.  */
  return COST_FACTOR (2);
}

/* The cost of NUNITS instructions of type TYPE issuing as ISSUE.  SPEED
   selects the speed or the size cost: for size, an instruction is an
   instruction, except that the singleton-bundle class empties a bundle.  */
static int
lvx_op_cost (enum attr_type type, enum attr_issue issue, int nunits,
	     bool speed)
{
  if (!speed)
    return COSTS_N_INSNS (type == TYPE_ALL ? 4 : 1);

  int latency = lvx_type_latency[(int) type];
  return COSTS_N_INSNS (lvx_issue_weight (issue) * nunits)
	 + (latency > 0 ? latency - 1 : 0);
}

/* Utils }}} */

/* Implements TARGET_REGISTER_MOVE_COST.  (Used by register allocation.)  */
static int
lvx_register_move_cost (machine_mode mode, reg_class_t from ATTRIBUTE_UNUSED,
			reg_class_t to ATTRIBUTE_UNUSED)
{
  int nwords = (GET_MODE_SIZE (mode) + UNITS_PER_WORD - 1) / UNITS_PER_WORD;
  if (lvx_extension_mode_p (mode))
    nwords = GET_MODE_NUNITS (mode);
  nwords = nwords >= 1 ? nwords : 1;

  int cost = 0;
  if (nwords <= 4)
    // Set word MOVE cost to 2, add 1 per extra word.
    cost = 1 + nwords;
  else
    // Set word MOVE cost of 2 per word for subreg split.
    cost = 2 * nwords;

  if (DUMP_COSTS)
    {
      fprintf (stderr,
	       "lvx_register_move_cost(mode=%s, from=%s, to=%s, init=%d)\n",
	       GET_MODE_NAME (mode), reg_class_names[from],
	       reg_class_names[to], !cfun->decl);
      fprintf (stderr, "\tcost=%d\n\n", cost);
    }

  return cost;
}

#undef TARGET_REGISTER_MOVE_COST
#define TARGET_REGISTER_MOVE_COST lvx_register_move_cost

/* Implements TARGET_MEMORY_MOVE_COST. (Used by register allocation.)  */
static int
lvx_memory_move_cost (machine_mode mode, reg_class_t rclass ATTRIBUTE_UNUSED,
		      bool in)
{
  unsigned mode_size = GET_MODE_SIZE (mode);
  unsigned oi_size = GET_MODE_SIZE (OImode);
  int lsucount = mode_size ? (mode_size + oi_size - 1) / oi_size : 1;
  /* A spill reload is an in-cache load and a spill store a store; both
     latencies come from the description.  */
  int cost = lvx_op_cost (in ? TYPE_LOAD : TYPE_STORE,
			  in ? ISSUE_LSU_AUXW : ISSUE_LSU_MEMW_AUXR,
			  lsucount, true);

  if (DUMP_COSTS)
    {
      fprintf (stderr,
	       "lvx_memory_move_cost(mode=%s, rclass=%s, in=%d, init=%d)\n",
	       GET_MODE_NAME (mode), reg_class_names[rclass], in,
	       !cfun->decl);
      fprintf (stderr, "\tcost=%d\n\n", cost);
    }

  gcc_assert (cost > 0);
  return cost;
}

#undef TARGET_MEMORY_MOVE_COST
#define TARGET_MEMORY_MOVE_COST lvx_memory_move_cost

/* Implement TARGET_USE_BY_PIECES_INFRASTRUCTURE_P.  */
static bool
lvx_use_by_pieces_infrastructure_p (unsigned HOST_WIDE_INT size,
				    unsigned int,
				    enum by_pieces_operation, bool speed_p)
{
  return size <= (speed_p ? 256 : 64);
}

#undef TARGET_USE_BY_PIECES_INFRASTRUCTURE_P
#define TARGET_USE_BY_PIECES_INFRASTRUCTURE_P lvx_use_by_pieces_infrastructure_p

/* Implements TARGET_RTX_COSTS {{{ */

/* Compute a (partial) cost for rtx X.  Return true if the complete
   cost has been computed, and false if subexpressions should be
   scanned.  In either case, *TOTAL contains the cost result.
   The initial value of *TOTAL is the default value computed by
   (rtx_cost).  It may be left unmodified.  OUTER_CODE contains the
   code of the superexpression of x.  */
static bool
lvx_rtx_costs (rtx x, machine_mode mode, int outer_code,
	       int opno, int *total, bool speed)
{
  /* Half precision is the one FP width with a shorter pipeline: faddh and
     fmulh are type fmuls (3 cycles) where faddw/faddd are fmuld (4).  */
  auto lvx_half_float_mode_p = [] (machine_mode m) {
    return GET_MODE_INNER (m) == HFmode;
  };
  bool float_mode_p = FLOAT_MODE_P (mode);
  unsigned mode_size = GET_MODE_SIZE (mode);
  unsigned oi_size = GET_MODE_SIZE (OImode);
  int lsucount = mode_size ? (mode_size + oi_size - 1) / oi_size : 1;
  int nwords = (mode_size + UNITS_PER_WORD - 1) / UNITS_PER_WORD;
  if (lvx_extension_mode_p (mode))
    /* An extension-unit mode is lanes in an XVR, not machine words: one xlo
       or xcopyo moves the whole buffer, so counting its 64-bit words charges
       64 instructions for a 512-byte mode.  */
    nwords = GET_MODE_NUNITS (mode);
  nwords = nwords >= 1 ? nwords : 1;

  if (DUMP_COSTS)
    {
      fprintf (stderr,
	       "lvx_rtx_costs(mode=%s, code=%s, opno=%d speed=%d, init=%d)\n",
	       GET_MODE_NAME (mode), GET_RTX_NAME (outer_code), opno, speed,
	       !cfun->decl);
      debug_rtx (x);
    }

  /* *TOTAL arrives holding the generic default that rtx_cost pre-loaded --
     factor * COSTS_N_INSNS (1), or the O(N*N) schoolbook estimate for a
     multiply or a divide.  Keep it for the `default' arm, where the code is
     one this switch does not know: zeroing it for everything is what made
     COPYSIGN, one fsign instruction, cost nothing at all.  The arms that
     compute nothing and break do so because the cost belongs to the
     enclosing insn (a SET costs what its source costs, a shift inside a PLUS
     is the ADDX* that absorbs it), so they keep the zero.  */
  int unknown_total = *total;
  *total = 0;

  switch (GET_CODE (x))
    {
    // RTX_CONST_OBJ:
    case CONST_INT:
      {
	HOST_WIDE_INT intval = INTVAL (x);
	if (SIGNED_INT_FITS_N_BITS (intval, 10))
	  // Assume 10-bit immediates are available at no cost.
	  *total = 0;
	else if (SIGNED_INT_FITS_N_BITS (intval, 32))
	  *total = speed ? 1 : 4;
	else
	  *total = speed ? 2 : 8;
      }
      if (outer_code == SET)
	*total += 2 * nwords;
      goto end_recurse;

    case CONST_WIDE_INT:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case CONST:
    case LABEL_REF:
    case SYMBOL_REF:
      *total = speed ? 2 * nwords : 8 * nwords;
      if (outer_code == SET)
	{
	  *total += lvx_op_cost (TYPE_ALU, ISSUE_TINY, nwords, speed);
	}
      goto end_recurse;

    // RTX_OBJ:
    case REG:
    case SCRATCH:
      goto end_recurse;

    case MEM:
      {
	/* A MEM read is a load and a MEM written is a store; opno is 0 only
	   for the destination of a SET.  An uncached or preload access does
	   not hit the L1, and the description says so: 24 cycles, not 3.  */
	bool is_load = opno != 0;
	enum lvx_variant variant = lvx_mem_variant (x);
	enum attr_type type = is_load ? TYPE_LOAD : TYPE_STORE;
	enum attr_issue issue = is_load ? ISSUE_LSU_AUXW : ISSUE_LSU_MEMW_AUXR;
	*total = lvx_op_cost (type, issue, lsucount, speed);
	if (speed && is_load && lvx_uncached_variant_p (variant))
	  *total += 24 - lvx_type_latency[(int) TYPE_LOAD];
      }
      goto end_recurse;

    // RTX_COMPARE:
    // RTX_COMM_COMPARE:
    case GE:
    case GT:
    case LE:
    case LT:
    case GEU:
    case GTU:
    case LEU:
    case LTU:
    case UNGE:
    case UNGT:
    case UNLE:
    case UNLT:
    case NE:
    case EQ:
    case UNORDERED:
    case ORDERED:
    case UNEQ:
    case LTGT:
      if (outer_code != IF_THEN_ELSE)
	{
	  // COMP* and FCOMP* instructions.
	  *total = lvx_op_cost (TYPE_ALU, float_mode_p ? ISSUE_LITE : ISSUE_TINY,
				nwords, speed);
	}
      // Recurse for immediates.
      break;

    // RTX_UNARY:
    case NEG:
    case NOT:
    case SIGN_EXTEND:
    case ZERO_EXTEND:
      if (outer_code == SET)
	{
	  enum rtx_code inner_code = GET_CODE (XEXP (x, 0));
	  enum rtx_class inner_class = GET_RTX_CLASS (inner_code);
	  if (inner_code == SUBREG || inner_class == RTX_OBJ
	      || (!float_mode_p && inner_class == RTX_UNARY))
	    // Stand-alone NEG / NOT / SIGN_EXTEND / ZERO_EXTEND.
	    {
	      *total = lvx_op_cost (TYPE_ALU, ISSUE_TINY, nwords, speed);
	      goto end_recurse;
	    }
	}
    case TRUNCATE:
      // Recurse for inner arithmetic.
      break;

    case FLOAT_EXTEND:
    case FLOAT_TRUNCATE:
    case FIX:
    case FLOAT:
    case UNSIGNED_FIX:
    case UNSIGNED_FLOAT:
    case ABS:
    case SQRT:
    case BSWAP:
    case FFS:
    case CLRSB:
    case CLZ:
    case CTZ:
    case POPCOUNT:
    case PARITY:
    case COPYSIGN:
    case VEC_DUPLICATE:
    case SS_NEG:
    case US_NEG:
    case SS_ABS:
    case SS_TRUNCATE:
    case US_TRUNCATE:
      {
	enum rtx_code code = GET_CODE (x);
	bool conversion = (code == FLOAT_EXTEND || code == FLOAT_TRUNCATE
			   || code == FIX || code == FLOAT
			   || code == UNSIGNED_FIX || code == UNSIGNED_FLOAT);
	*total = float_mode_p || conversion
	  ? lvx_op_cost (conversion ? TYPE_FCVT : TYPE_ALU, ISSUE_LITE,
			 nwords, speed)
	  : lvx_op_cost (TYPE_ALU, ISSUE_TINY, nwords, speed);
      }
      goto end_recurse;

    // RTX_COMM_ARITH:
    // RTX_BIN_ARITH:
    case PLUS:
    case MINUS:
      *total = float_mode_p
	? lvx_op_cost (lvx_half_float_mode_p (mode) ? TYPE_FMULS : TYPE_FMULD,
		       ISSUE_LITE, nwords, speed)
	: lvx_op_cost (TYPE_ALU, ISSUE_TINY, nwords, speed);
      // Recurse for immediates or for inner MULT or SHIFT.
      break;

    case MULT:
    case SS_MULT:
    case US_MULT:
    case SMUL_HIGHPART:
    case UMUL_HIGHPART:
      *total = float_mode_p
	? lvx_op_cost (lvx_half_float_mode_p (mode) ? TYPE_FMULS : TYPE_FMULD,
		       ISSUE_LITE, nwords, speed)
	: lvx_op_cost (TYPE_IMUL, ISSUE_LITE, nwords, speed);
      if (!float_mode_p && (outer_code == PLUS || outer_code == MINUS))
	{
	  // MADD*, MSBF* instructions: one imadd, and the enclosing PLUS or
	  // MINUS charges nothing of its own.
	  *total = lvx_op_cost (TYPE_IMADD, ISSUE_LITE, nwords, speed)
		   - lvx_op_cost (TYPE_ALU, ISSUE_TINY, nwords, speed);
	  gcc_checking_assert (*total >= 0);
	}
      goto end_recurse;

    case AND:
    case IOR:
    case XOR:
    case SMIN:
    case SMAX:
    case UMIN:
    case UMAX:
    case SS_PLUS:
    case US_PLUS:
      *total = lvx_op_cost (TYPE_ALU, float_mode_p ? ISSUE_LITE : ISSUE_TINY,
			    nwords, speed);
      // Recurse for immediates.
      break;

    case COMPARE:
      // Unused.
      break;

    case DIV:
    case SS_DIV:
    case US_DIV:
    case MOD:
    case UDIV:
    case UMOD:
      if (CONST_INT_P (XEXP (x, 1))
	  && __builtin_popcount (INTVAL (XEXP (x, 1))) == 1)
	{
	  // Integer divide by a power of 2.
	  *total = lvx_op_cost (TYPE_ALU, ISSUE_TINY, nwords, speed);
	  goto end_recurse;
	}
      *total = float_mode_p
	? lvx_op_cost (TYPE_FDIV, ISSUE_FULL, nwords, speed)
	: lvx_op_cost (TYPE_ALL, ISSUE_ALL, 8, speed);
      goto end_recurse;

    case ASHIFT:
    case ROTATE:
    case ASHIFTRT:
    case LSHIFTRT:
    case ROTATERT:
      if (outer_code != PLUS && outer_code != MINUS)
	{
	  // ADDX* and SBFX* instructions.
	  *total = lvx_op_cost (TYPE_ALU, ISSUE_TINY, nwords, speed);
	}
      break;

    case VEC_SELECT:
    case VEC_CONCAT:
    case VEC_SERIES:
    case SS_MINUS:
    case SS_ASHIFT:
    case US_ASHIFT:
    case US_MINUS:
      *total = lvx_op_cost (TYPE_ALU, ISSUE_LITE, nwords, speed);
      break;

    // RTX_TERNARY:
    case IF_THEN_ELSE:
      if (mode == VOIDmode)
	{
	  *total = lvx_op_cost (TYPE_BRANCH, ISSUE_BCU_BRRP, 1, speed);
	}
      else
	{
	  *total = lvx_op_cost (TYPE_ALU, ISSUE_LITE, nwords, speed);
	}
      goto end_recurse;

    case VEC_MERGE:
      break;

    case FMA:
      *total = lvx_op_cost (lvx_half_float_mode_p (mode) ? TYPE_FMADDS
						        : TYPE_FMADDD,
			    ISSUE_LITE, nwords, speed);
      goto end_recurse;

    // RTX_BITFIELD_OPS:
    case SIGN_EXTRACT:
    case ZERO_EXTRACT:
      if (outer_code == SET)
	*total = lvx_op_cost (TYPE_ALU, ISSUE_LITE, nwords, speed);
      goto end_recurse;

    // RTX_EXTRA:
    case SET:
      if (REG_SUBREG_P (SET_SRC (x)) && REG_SUBREG_P (SET_DEST (x)))
	{
	  if (nwords <= 4)
	    // Set word MOVE cost to 2, add 1 per extra word.
	    *total = 1 + nwords;
	  else
	    // Set word MOVE cost of 2 per word for subreg split.
	    *total = 2 * nwords;
	  goto end_recurse;
	}
      break;

    case UNSPEC:
      switch (XINT (x, 1))
	{
	case UNSPEC_SBMM8D:
	case UNSPEC_SBMMT8D:
	case UNSPEC_SBMM8DS:
	case UNSPEC_SBMM8DXY:
	case UNSPEC_SRS:
	  *total = lvx_op_cost (TYPE_ALU, ISSUE_TINY, nwords, speed);
	  goto end_recurse;
	default:
	  *total = lvx_op_cost (TYPE_ALU, ISSUE_LITE, nwords, speed);
	  break;
	}
      break;

    case UNSPEC_VOLATILE:
      *total = lvx_op_cost (TYPE_ALL, ISSUE_ALL, nwords, speed);
      goto end_recurse;

    case PREFETCH:
      *total = lvx_op_cost (TYPE_PREFETCH, ISSUE_LSU, 1, speed);
      goto end_recurse;

    case CALL:
    case RETURN:
      *total = lvx_op_cost (TYPE_BRANCH, ISSUE_BCU_BRRP, 1, speed);
      goto end_recurse;

    default:
      /* An rtx code with no arm here: charge what GCC would have.  */
      *total = unknown_total;
      break;
    }

  if (DUMP_COSTS)
    fprintf (stderr, "\tcost=%d (inner)\n\n", *total);
  return false;

end_recurse:
  if (DUMP_COSTS)
    fprintf (stderr, "\tcost=%d (leaf)\n\n", *total);
  return true;
}

#undef TARGET_RTX_COSTS
#define TARGET_RTX_COSTS lvx_rtx_costs
/* Implements TARGET_RTX_COSTS }}} */

/* Compute the cost of an addressing mode that contains X.  */
static int
lvx_address_cost (rtx x, machine_mode mode ATTRIBUTE_UNUSED,
		  addr_space_t space ATTRIBUTE_UNUSED,
		  bool speed ATTRIBUTE_UNUSED)
{
  // RISC-style offset[reg] addressing mode, default cost is zero.
  int cost = 0;

  // Case of .xs reg[reg] addressing mode.
  if (HAVE_LVX_ADDRESSING_XS
      && GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 0)) == MULT
      && GET_CODE (XEXP (XEXP (x, 0), 1)) == CONST_INT)
    cost = 1;

  // Case of reg[reg] addressing mode.
  else if (GET_CODE (x) == PLUS && REG_P (XEXP (x, 0)) && REG_P (XEXP (x, 1)))
    cost = 2;

  // Case of 0[reg] addressing mode, prefer reg[reg] if possible.
  else if (REG_P (x) || SUBREG_P (x))
    cost = 3;

  if (DUMP_COSTS)
    {
      fprintf (stderr,
	       "lvx_address_cost(mode=%s, space=%d, speed=%d, init=%d)\n",
	       GET_MODE_NAME (mode), space, speed, !cfun->decl);
      debug_rtx (x);
      fprintf (stderr, "\tcost=%d\n\n", cost);
    }

  return cost;
}

#undef TARGET_ADDRESS_COST
#define TARGET_ADDRESS_COST lvx_address_cost

/* Implements TARGET_INSN_COST {{{ */

/* Compute a full rtx cost using the same logic as the generic (rtx_cost).
   Used to observe the full effects of recursively calling (lvx_rtx_costs).  */
static int
lvx_full_rtx_cost (rtx x, machine_mode mode, enum rtx_code outer_code,
		   int opno, bool speed)
{
  int total = 0;
  if (x == 0)
    return 0;

  if (GET_MODE (x) != VOIDmode)
    mode = GET_MODE (x);

  enum rtx_code code = GET_CODE (x);
  if (code == REG)
    return 0;

  // No TARGETS_MODES_TIEABLE_P.
  if (code != SUBREG && code != TRUNCATE)
    if (lvx_rtx_costs (x, mode, outer_code, opno, &total, speed))
      return total;

  const char *fmt = GET_RTX_FORMAT (code);
  for (int i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
    if (fmt[i] == 'e')
      total += lvx_full_rtx_cost (XEXP (x, i), mode, code, i, speed);
    else if (fmt[i] == 'E')
      for (int j = 0; j < XVECLEN (x, i); j++)
	total += lvx_full_rtx_cost (XVECEXP (x, i, j), mode, code, i, speed);

  return total;
}

/* Describes the relative cost of RTL instructions.
   /!\ Synchronize with the types.md type and latencies.  */
static int
lvx_insn_cost (rtx_insn *insn, bool speed)
{
  if (recog_memoized (insn) < 0)
    {
      if (DUMP_COSTS)
	{
	  fprintf (stderr, "lvx_insn_cost(uid=%d, speed=%d, init=%d)\n",
		   INSN_UID (insn), speed, !cfun->decl);
	  debug_rtx (PATTERN (insn));
	  fprintf (stderr, "\tcost=0 (not memoized)\n\n");
	}
      return 0;
    }

  if (get_attr_type (insn) == TYPE_NOP)
    return 0;

  /* A guarded or masked op is preceded by a GUARD, MASKS or MASKM syllable
     that the length attribute does not count, the prefix not being part of
     the insn.  Ops of one bundle sharing a prefix share the syllable, which
     nothing here can know, so charge it: the alternative is charging nothing,
     and a predicated insn is not free.  */
  bool prefixed = (GET_CODE (PATTERN (insn)) == COND_EXEC
		   || lvx_masked_prefix (insn, NULL, NULL, NULL));

  if (!speed)
    return get_attr_length (insn) + (prefixed ? LVX_SYLLABLE_SIZE : 0);

  /* What an instruction costs is how many instructions it is, weighted by how
     scarce the unit it needs is, plus the latency it makes a consumer wait.
     Both come from the description now: the issue class says how many and
     which units, and define_insn_reservation says how long.  */
  const struct lvx_issue_resources *r
    = &lvx_issue_table[(int) get_attr_issue (insn)];
  int weight = 1;
  if (get_attr_type (insn) == TYPE_ALL)
    weight = 4;
  else if (r->full || r->lsu || r->bcu || r->ext)
    weight = COST_FACTOR (4);

  int latency = insn_default_latency (insn);
  int cost = COSTS_N_INSNS (weight * r->ninsns) + (latency > 0 ? latency - 1 : 0);
  if (prefixed)
    cost += COSTS_N_INSNS (lvx_issue_weight (ISSUE_BCU_BRRP));

  if (DUMP_COSTS)
    {
      fprintf (stderr, "lvx_insn_cost(uid=%d, speed=%d, init=%d)\n",
	       INSN_UID (insn), speed, !cfun->decl);
      debug_rtx (PATTERN (insn));
      int rtx_cost =
	lvx_full_rtx_cost (PATTERN (insn), VOIDmode, INSN, 3, speed);
      fprintf (stderr, "\tcost=%d\trtx_cost=%d%s\n\n", cost, rtx_cost,
	       (rtx_cost > cost || rtx_cost == 0) ? "\tFIXME!" : "");
    }

  return cost;
}

#undef TARGET_INSN_COST
#define TARGET_INSN_COST lvx_insn_cost
/* Implements TARGET_INSN_COST }}} */

/* 18.16 Describing Relative Costs of Operations }}} */


/* 18.17 Adjusting the Instruction Scheduler {{{ */

/* Utils {{{ */

static void
lvx_sched2_ctor (int max_uid)
{
  memset ((void *)lvx_sched2, 0, sizeof (*lvx_sched2));
  lvx_sched2->max_uid = max_uid;
  lvx_sched2->prev_uid = -1;
  lvx_sched2->insn_cycle = XNEWVEC (short, lvx_sched2->max_uid);
  memset (lvx_sched2->insn_cycle, -1, sizeof (short) * lvx_sched2->max_uid);
  lvx_sched2->insn_flags = XCNEWVEC (unsigned char, lvx_sched2->max_uid);
  lvx_sched2->bundle_state = xmalloc (state_size ());
  lvx_sched2->cond_insn = make_insn_raw (gen_cond_insn ());
  SET_PREV_INSN (lvx_sched2->cond_insn) = 0;
  SET_NEXT_INSN (lvx_sched2->cond_insn) = 0;
  recog_memoized (lvx_sched2->cond_insn);
}

static void
lvx_sched2_dtor (void)
{
  lvx_sched2->max_uid = 0;
  lvx_sched2->prev_uid = -1;
  XDELETEVEC (lvx_sched2->insn_cycle);
  lvx_sched2->insn_cycle = 0;
  XDELETEVEC (lvx_sched2->insn_flags);
  lvx_sched2->insn_flags = 0;
  free (lvx_sched2->bundle_state);
  lvx_sched2->bundle_state = 0;
}

static void
lvx_dependencies_fprint (FILE *file, rtx_insn *insn) ATTRIBUTE_UNUSED;

static void
lvx_dependencies_fprint (FILE *file, rtx_insn *insn)
{
  dep_t dep;
  sd_iterator_def sd_it;
  fprintf (file, "forward dependences (insn %d)\n", INSN_UID (insn));
  FOR_EACH_DEP (insn, SD_LIST_FORW, sd_it, dep)
    {
      enum reg_note dep_type = DEP_TYPE (dep);
      char type = (dep_type == REG_DEP_TRUE ? 'T' :
		    (dep_type == REG_DEP_OUTPUT ? 'O' :
		      (dep_type == REG_DEP_ANTI ? 'A' :
			(dep_type == REG_DEP_CONTROL ? 'C' : '?'))));
      char reg = DEP_MULTIPLE(dep) ? '*' : (DEP_NONREG(dep) ? ' ': 'r');
      if (DEP_COST (dep) == UNKNOWN_DEP_COST)
	fprintf (file, "\t%c%c-> (insn %d) (cost UNKNOWN)\n",
		 type, reg, INSN_UID (DEP_CON (dep)));
      else
	fprintf (file, "\t%c%c-> (insn %d) (cost %d)\n",
		 type, reg, INSN_UID (DEP_CON (dep)), DEP_COST (dep));
    }
  fprintf (file, "backward dependences (insn %d)\n", INSN_UID (insn));
  FOR_EACH_DEP (insn, SD_LIST_BACK, sd_it, dep)
    {
      enum reg_note dep_type = DEP_TYPE (dep);
      char type = (dep_type == REG_DEP_TRUE ? 'T' :
		    (dep_type == REG_DEP_OUTPUT ? 'O' :
		      (dep_type == REG_DEP_ANTI ? 'A' :
			(dep_type == REG_DEP_CONTROL ? 'C' : '?'))));
      char reg = DEP_MULTIPLE(dep) ? '*' : (DEP_NONREG(dep) ? ' ': 'r');
      if (DEP_COST (dep) == UNKNOWN_DEP_COST)
	fprintf (file, "\t<-%c%c (insn %d) (cost UNKNOWN)\n",
		 type, reg, INSN_UID (DEP_PRO (dep)));
      else
	fprintf (file, "\t<-%c%c (insn %d) (cost %d)\n",
		 type, reg, INSN_UID (DEP_PRO (dep)), DEP_COST (dep));
    }
}


static void
lvx_sched_resources_add (struct lvx_sched_resources *resources, rtx_insn *insn)
{
  if (!NONDEBUG_INSN_P (insn) || INSN_CODE (insn) < 0)
    return;

  resources->insn_count++;

  enum attr_type type = get_attr_type (insn);
  if (type == TYPE_NOP)
    /* A nop is a placeholder the bundler fills, not a demand on the bundle.  */
    return;

  /* The machine description already says what this occupies, and says it once:
     the same vector the automaton reserves.  This used to be a ladder of range
     tests over the type enum, under a comment asking the reader to keep them in
     step with the ORDER of types.md.

     TYPE_ALL needs no case of its own.  It used to claim one of each unit by
     hand, so that nothing could join it; that was the generated `all' row
     written out a second time.  Every type-ALL alternative pairs with issue
     `all', whose vector takes every function unit, so it saturates to one
     bundle per instruction exactly as the hand-claim did -- and it is what the
     automaton reserves, which the hand-claim was free to contradict.  */
  const struct lvx_issue_resources *r
    = &lvx_issue_table[(int) get_attr_issue (insn)];
  resources->ninsns_count += r->ninsns;
#define LVX_SCHED_RESOURCE_ADD(R) resources->R##_count += r->R;
  LVX_ISSUE_RESOURCES (LVX_SCHED_RESOURCE_ADD)
#undef LVX_SCHED_RESOURCE_ADD
}

/* The least number of bundles the region accumulated in RESOURCES can occupy:
   for every resource, ceil(demand / availability), and the largest of those.

   Each vector names every enclosing counter -- a FULL instruction spends a
   LITE and a TINY too, TINY's four being ALU0 ALU1 LSU0 LSU1 -- so this
   independent maximum per counter is already the subset bound, and a
   C6x-style merge_unit_reqs fold over the counters must not be added on top
   of it: the same demand would be counted twice and the bound would come out
   too large.  */
static int lvx_sched_issue_rate (void);
static unsigned
lvx_sched_resources_full_bundles (struct lvx_sched_resources *resources)
{
  unsigned issue_rate = lvx_sched_issue_rate ();
  unsigned result = (resources->ninsns_count + issue_rate - 1) / issue_rate;

  /* The availabilities come from Resource.yml, generated beside the vectors.
     They were four literals here, and had drifted: EXT read as 1 of 4 and AUXR
     as 1 of 2, while AUXW and MEMW were accumulated into nothing and bounded
     nothing -- an under-estimate, which for a ResMII and for the unroll factor
     that divides by it is the dangerous direction.  */
#define LVX_SCHED_RESOURCE_BOUND(R)					\
  {									\
    unsigned avail = lvx_issue_availability.R;				\
    unsigned need = (resources->R##_count + avail - 1) / avail;		\
    if (result < need)							\
      result = need;							\
  }
  LVX_ISSUE_RESOURCES (LVX_SCHED_RESOURCE_BOUND)
#undef LVX_SCHED_RESOURCE_BOUND

  return result;
}

/* Utils }}} */

static int
lvx_sched_issue_rate (void)
{
  return 8;
}

#undef TARGET_SCHED_ISSUE_RATE
#define TARGET_SCHED_ISSUE_RATE lvx_sched_issue_rate

/* Implements TARGET_SCHED_ADJUST_COST.  */

static int
lvx_sched_adjust_cost (rtx_insn *cons_insn, int dep_type,
		       rtx_insn *prod_insn, int cost, unsigned int)
{
  if (dep_type == REG_DEP_TRUE)
    {
      // Use (set_of) instead of (reg_overlap_mentioned_p) to catch cases in
      // SCHED2 of producing a register pair and consuming a single register.
      if (JUMP_P (cons_insn))
	// Zero cost except for the dependence(s) carrying the tested value(s).
	// Case of carrying is when PROD_INSN modifies a REG used by CONS_INSN.
	{
	  rtx x = PATTERN (cons_insn);
	  if (GET_CODE (x) == PARALLEL)
	    x = XVECEXP (x, 0, 0);
	  if (GET_CODE (x) == SET)
	    {
	      x = SET_SRC (x);
	      if (GET_CODE (x) == IF_THEN_ELSE)
		{
		  rtx op = XEXP (x, 0);
		  rtx x0 = XEXP (op, 0);
		  rtx x1 = XEXP (op, 1);
		  if (GET_CODE (x0) == ZERO_EXTRACT)
		    // Case of CB.ODD, CB.EVEN.
		    {
		      x0 = XEXP (x0, 0);
		    }
		  else if (GET_CODE (x0) == AND)
		    // Case of CCB.[WD]ANY, CCB.[WD]NONE.
		    {
		      x1 = XEXP (x0, 1);
		      x0 = XEXP (x0, 0);
		    }
		  if (x1 == const0_rtx)
		    // Case of CB (x1 was updated in the CCB case).
		    {
		      if (!set_of (x0, prod_insn))
			cost = 0;
		    }
		  else
		    // Case of CCB.
		    {
		      if (!set_of (x0, prod_insn)
			  && !set_of (x1, prod_insn))
			cost = 0;
		    }
		}
	    }
	  else if (ANY_RETURN_P (x))
	    {
	      rtx y = PATTERN (prod_insn);
	      if (GET_CODE (y) == PARALLEL)
		y = XVECEXP (y, 0, 0);
	      if (GET_CODE (y) == SET)
		y = SET_DEST (y);
	      if (!REG_P (y) || REGNO (y) != LVX_RA_REGNO)
		cost = 0;
	    }
	}
      else if (CALL_P (cons_insn))
	// Reduce cost except for the dependence carrying the call target.
	// Case of carrying is when PROD_INSN modifies a REG used by CONS_INSN.
	{
	  rtx x = PATTERN (cons_insn);
	  if (GET_CODE (x) == PARALLEL)
	    x = XVECEXP (x, 0, 0);
	  if (GET_CODE (x) == SET)
	    x = SET_SRC (x);
	  if (GET_CODE (x) == CALL)
	    x = XEXP (XEXP (x, 0), 0);
	  if (!REG_P (x) || !set_of (x, prod_insn))
	    cost = 0;
	}
      else if (recog_memoized (prod_insn) >= 0
	       && recog_memoized (cons_insn) >= 0)
	// Adjust cost between instructions based on their type attribute.
	{
	  rtx prod_set = single_set (prod_insn);
	  rtx cons_set = single_set (cons_insn);
	  enum attr_type prod_type = get_attr_type (prod_insn);
	  // If the producer is a load feeding the target of a conditional
	  // or a scatter load, set cost to 1 instead of the load latency.
	  if (lvx_load_class_p (prod_type))
	    {
	      rtx x = prod_set ? SET_DEST (prod_set) : 0;
	      rtx op = cons_set ? SET_SRC (cons_set) : 0;
	      if (x && op && GET_CODE (op) == UNSPEC)
		{
		  int unspec = XINT (op, 1);
		  if (SUBREG_P (x))
		    x = SUBREG_REG (x);
		  rtx y = XVECEXP (op, 0, 0);
		  if (SUBREG_P (y))
		    y = SUBREG_REG (y);
		  if (x == y)
		    if (unspec == UNSPEC_LOADC || unspec == UNSPEC_XLOADC
			|| (unspec >= UNSPEC_XLOADQ0
			    && unspec <= UNSPEC_XLOADCQ3))
		      cost = 1;
		}
	    }
	}
    }
  else if (dep_type == REG_DEP_ANTI)
    {
      cost = 0;
      rtx prod_set = single_set (prod_insn);
      if (prod_set
	  && contains_mem_rtx_p (SET_DEST (prod_set))
	  && contains_mem_rtx_p (PATTERN (cons_insn)))
	// Memory dependence masquerading as REG_DEP_ANTI in (sched_analyze_2).
	cost = 1;

      if (JUMP_P (prod_insn) || CALL_P (prod_insn))
	// Producer INSN is a control-flow change, so consumer INSN cannot issue
	// at same cycle after, unless the first is conditional and second is any JUMP.
	{
	  cost = !(JUMP_P (prod_insn)
		   && !simplejump_p (prod_insn) && JUMP_P (cons_insn));
	}
      else if (GET_CODE (PATTERN (prod_insn)) == CLOBBER)
	// Delay consumer INSN of CLOBBER for non-zero number of clock cycles.
	// This corrects the rewriting of dependencies by (chain_to_prev_insn).
	// Problem appears in cases the CLOBBER of an INSF is located after the
	// producer for the INSF. So we find this producer and apply its cost.
	{
	  cost = 1;
	  dep_t dep;
	  sd_iterator_def sd_it;
	  FOR_EACH_DEP (prod_insn, SD_LIST_BACK, sd_it, dep)
	    {
	      if (DEP_TYPE (dep) == REG_DEP_TRUE)
		{
		  rtx_insn *pro_insn = DEP_PRO (dep);
		  int pro_cost = insn_cost (pro_insn, true);
		  if (cost < pro_cost)
		    cost = pro_cost;
		}
	    }
	}
    }
  else if (dep_type == REG_DEP_OUTPUT)
    {
      cost = 1;
      if (JUMP_P (cons_insn) || CALL_P (cons_insn))
	// Consumer is JUMP or CALL, producer can issue at same clock cycle.
	{
	  cost = 0;
	}
    }

  if (sched_dump && sched_verbose >= 8)
    {
      char type = (dep_type == REG_DEP_TRUE ? 'T' :
		   (dep_type == REG_DEP_OUTPUT ? 'O' :
		    (dep_type == REG_DEP_ANTI ? 'A' :
		     (dep_type == REG_DEP_CONTROL ? 'C' : '?'))));
      fprintf (sched_dump,
	       "\tadjust_cost (insn %d) %c-> (insn %d)\t%d cycles\n",
	       INSN_UID (prod_insn), type, INSN_UID (cons_insn), cost);
    }

  return cost;
}

#undef TARGET_SCHED_ADJUST_COST
#define TARGET_SCHED_ADJUST_COST lvx_sched_adjust_cost

static int
lvx_sched_adjust_priority (rtx_insn *insn, int priority)
{
  rtx x = PATTERN (insn);
  // CLOBBER insns better remain first in scheduling group after SCHED1.
  if (GET_CODE (x) == CLOBBER)
    if (!reload_completed)
      priority += 10;
  return priority;
}

#undef TARGET_SCHED_ADJUST_PRIORITY
#define TARGET_SCHED_ADJUST_PRIORITY lvx_sched_adjust_priority

static int
lvx_sched_reorder2 (FILE *, int, rtx_insn **ready, int *nreadyp, int)
{
  if (!reload_completed)
    return 0;

  if (sched_dump && sched_verbose >= 8)
    {
      fprintf (sched_dump, "lvx_sched_reorder2(%d):", *nreadyp);
      for (int i = *nreadyp - 1; i >= 0; i--)
	{
	  rtx_insn *insn = ready[i];
	  if (INSN_CODE (insn) >= 0)
	    {
	      fprintf (sched_dump, "\t(insn %d)", INSN_UID (insn));
	      enum attr_bcu_used bcu_used = get_attr_bcu_used (insn);
	      if (bcu_used == BCU_USED_YES)
		fprintf (sched_dump, "*");
	    }
	}
      fprintf (sched_dump, "\n");
    }

  return *nreadyp;
}

#undef TARGET_SCHED_REORDER2
#define TARGET_SCHED_REORDER2 lvx_sched_reorder2

/* Implements TARGET_SCHED_DEPENDENCIES_EVALUATION_HOOK.
   If SCHED_VERBOSE is high enough, dumps dependencies information.

   This used to also pin a modulo-scheduled kernel in place, by threading its
   instructions with cost-0 ANTI dependences so that schedule_ebbs would bundle
   the block without reordering it.  That was SMS scaffolding and it is gone:
   see docs/cms-foundations.md §7quindecies.  CMS has no need of it -- its
   kernel schedule already satisfies every dependence and resource constraint,
   so it annotates insn_cycle/insn_flags itself and marks the kernel
   BB_DISABLE_SCHEDULE to keep this pass away.  */
static void
lvx_sched_dependencies_evaluation_hook (rtx_insn *head, rtx_insn *tail)
{
  if (!sched_dump || sched_verbose < 9)
    return;

  rtx_insn *next_tail = next_nonnote_insn (tail);
  rtx_insn *insn = head;
  if (NOTE_P (insn))
    insn = next_nonnote_insn (insn);
  for (; insn != next_tail; insn = next_nonnote_insn (insn))
    lvx_dependencies_fprint (sched_dump, insn);
}

#undef TARGET_SCHED_DEPENDENCIES_EVALUATION_HOOK
#define TARGET_SCHED_DEPENDENCIES_EVALUATION_HOOK                              \
  lvx_sched_dependencies_evaluation_hook

static void
lvx_sched_init (FILE *, int, int)
{
  if (!reload_completed)
    return;

  if ((unsigned) lvx_sched2->prev_uid < (unsigned) lvx_sched2->max_uid)
    lvx_sched2->insn_flags[lvx_sched2->prev_uid] |= LVX_SCHED2_INSN_STOP;

  lvx_sched2->prev_uid = -1;
}

#undef TARGET_SCHED_INIT
#define TARGET_SCHED_INIT lvx_sched_init

static void
lvx_sched_finish (FILE *file ATTRIBUTE_UNUSED, int verbose ATTRIBUTE_UNUSED)
{
  if (!reload_completed)
    return;

  if ((unsigned) lvx_sched2->prev_uid < (unsigned) lvx_sched2->max_uid)
    lvx_sched2->insn_flags[lvx_sched2->prev_uid] |= LVX_SCHED2_INSN_STOP;
}

#undef TARGET_SCHED_FINISH
#define TARGET_SCHED_FINISH lvx_sched_finish

/* If INSN carries a MASKS/MASKM lane-masking prefix -- a
   `(use (unspec [mask lanetodo] UNSPEC_MASKS|UNSPEC_MASKM))' inside its
   PARALLEL -- fill *KIND, *MASK and *LANETODO from it and return true, else
   false.  This is the masked counterpart of reading XEXP (PATTERN, 0) for a
   guarded COND_EXEC: a fixed sub-rtx the scheduler keys BCU-slot sharing on,
   and, being a use of the mask register, the dataflow edge from the COMP or
   EXTB*D that produced it.  KIND, MASK and LANETODO may be null.  */
static bool
lvx_masked_prefix (rtx_insn *insn, enum lvx_bcu_kind *kind, rtx *mask,
		   int *lanetodo)
{
  rtx pattern = PATTERN (insn);
  if (GET_CODE (pattern) != PARALLEL)
    return false;
  for (int i = 0; i < XVECLEN (pattern, 0); i++)
    {
      rtx e = XVECEXP (pattern, 0, i);
      if (GET_CODE (e) != USE || GET_CODE (XEXP (e, 0)) != UNSPEC)
	continue;
      rtx u = XEXP (e, 0);
      enum lvx_bcu_kind k;
      if (XINT (u, 1) == UNSPEC_MASKS)
	k = BCU_MASKS;
      else if (XINT (u, 1) == UNSPEC_MASKM)
	k = BCU_MASKM;
      else
	continue;
      /* KIND, MASK and LANETODO are optional: a caller that only asks
	 whether the insn is masked passes none of them.  */
      if (kind)
	*kind = k;
      if (mask)
	*mask = XVECEXP (u, 0, 0);
      if (lanetodo)
	*lanetodo = INTVAL (XVECEXP (u, 0, 1));
      return true;
    }
  return false;
}

/* Share or claim one of the bundle's two BCU prefix slots for a prefix of KIND
   with sharing key (LANETODO, KEY) -- a guard condition, or a mask register.
   BUNDLE_STATE is CURR_STATE with this insn's own reservation already applied,
   so a claim only has to fit the extra prefix syllable (modelled by cond_insn,
   the shared BCU_BRRP reservation).  Returns 0 when the prefix is shared (free)
   or a fresh syllable was reserved (committed to CURR_STATE), and 1 when the
   bundle cannot take it -- both slots already hold distinct prefixes, or the
   syllable does not fit -- so the caller starts a new cycle.  */
/* Is an identical prefix already in the bundle being built?  Then this op
   shares its syllable and adds nothing, neither a slot nor four bytes.  A
   pure test, so the byte accounting can ask before the slot is claimed.  */
static bool
lvx_bcu_prefix_shared_p (enum lvx_bcu_kind kind, int lanetodo, rtx key)
{
  for (int s = 0; s < 2; s++)
    {
      if (!lvx_sched2->bcu_use[s])
	return false;
      if (lvx_sched2->bcu_kind[s] == kind
	  && lvx_sched2->bcu_lanetodo[s] == lanetodo
	  && rtx_equal_p (lvx_sched2->bcu_use[s], key))
	return true;
    }
  return false;
}

static int
lvx_bcu_prefix_slot (state_t curr_state, void *bundle_state,
		     enum lvx_bcu_kind kind, int lanetodo, rtx key)
{
  for (int s = 0; s < 2; s++)
    {
      if (!lvx_sched2->bcu_use[s])
	{
	  /* Claim slot s: this prefix needs its own BCU syllable.  A syllable
	     that does not fit fails the whole issue -- no other slot helps.  */
	  if (state_transition (bundle_state, lvx_sched2->cond_insn) < 0)
	    {
	      state_transition (curr_state, lvx_sched2->cond_insn);
	      lvx_sched2->bcu_use[s] = key;
	      lvx_sched2->bcu_kind[s] = kind;
	      lvx_sched2->bcu_lanetodo[s] = lanetodo;
	      return 0;
	    }
	  return 1;
	}
      if (lvx_sched2->bcu_kind[s] == kind
	  && lvx_sched2->bcu_lanetodo[s] == lanetodo
	  && rtx_equal_p (lvx_sched2->bcu_use[s], key))
	return 0;  /* An identical prefix already in the bundle: the op is free. */
    }
  return 1;  /* Both slots hold distinct prefixes. */
}

static int
lvx_sched_dfa_new_cycle (FILE *, int, rtx_insn *insn, int last_clock,
			 int clock, int *)
{
  if (!reload_completed)
    return 0;

  if (last_clock != clock)
    {
      lvx_sched2->bcu_use[0] = lvx_sched2->bcu_use[1] = 0;
      lvx_sched2->bundle_size = 0;
    }

  if (INSN_CODE (insn) >= 0)
    {
      /* Would this insn overflow the bundle being built?  Then it does not
	 issue on this cycle, which is what a nonzero result means and what
	 every capacity check below does when a slot is taken.  The scheduler
	 starts a new cycle and the last_clock != clock above resets the size.

	 This used to assert instead, so a bundle one syllable too full
	 crashed the compiler rather than being split -- reachable from five
	 builtins, and from any sequence long enough.  Asking before the BCU
	 bookkeeping below rather than after it also leaves bcu_use[]
	 untouched for an insn that is not going to issue here.  */
      /* What prefix, if any, this op carries.  Decided before the capacity
	 check below, because a prefix that is not already in the bundle costs
	 a syllable of its own -- four bytes the length attribute does not
	 include, since the prefix is not part of the insn.  */
      enum attr_bcu_used bcu_used = get_attr_bcu_used (insn);
      rtx pattern = PATTERN (insn);
      enum lvx_bcu_kind kind = BCU_PLAIN;
      rtx key = NULL_RTX;
      int lanetodo = 0;
      bool prefixed = false;

      if (bcu_used == BCU_USED_YES)
	{
	  if (GET_CODE (pattern) == COND_EXEC)
	    /* An if-converted (guarded) insn: its GUARD condition is the key. */
	    {
	      kind = BCU_GUARD;
	      key = XEXP (pattern, 0);
	      prefixed = true;
	    }
	  else if (lvx_masked_prefix (insn, &kind, &key, &lanetodo))
	    /* A MASKS/MASKM masked insn: mask register (and lanetodo) is the key.
	       An insn is never both guarded and masked -- one op, one prefix
	       syllable -- and COND_EXEC is handled above, so this is exclusive. */
	    prefixed = true;
	}

      /* The bytes this op adds: its own syllables, plus a prefix syllable
	 when it brings a prefix no other op in the bundle already carries.  */
      int prefix_bytes = (prefixed
			  && !lvx_bcu_prefix_shared_p (kind, lanetodo, key))
			 ? LVX_SYLLABLE_SIZE : 0;

      if (lvx_sched2->bundle_size + get_attr_length (insn) + prefix_bytes
	  > LVX_SCHED2_BUNDLE_SIZE)
	return 1;

      if (bcu_used == BCU_USED_YES)
	{
	  if (prefixed)
	    /* GUARD, MASKS and MASKM all spend one BCU_BRRP prefix syllable, and
	       two ops sharing an identical prefix spend only one between them. */
	    {
	      memcpy (lvx_sched2->bundle_state, curr_state, state_size ());
	      if (state_transition (lvx_sched2->bundle_state, insn) >= 0)
		gcc_unreachable ();
	      if (lvx_bcu_prefix_slot (curr_state, lvx_sched2->bundle_state,
				       kind, lanetodo, key))
		return 1;
	    }
	  else
	    /* A plain BCU insn (branch/jump/sysget): it is the BCU op itself, not
	       a prefix, so it claims a whole slot and never shares one. */
	    {
	      if (!lvx_sched2->bcu_use[0])
		{
		  lvx_sched2->bcu_use[0] = const0_rtx;
		  lvx_sched2->bcu_kind[0] = BCU_PLAIN;
		}
	      else if (!lvx_sched2->bcu_use[1])
		{
		  lvx_sched2->bcu_use[1] = const1_rtx;
		  lvx_sched2->bcu_kind[1] = BCU_PLAIN;
		}
	      else
		return 1;
	    }
	}

      lvx_sched2->bundle_size += get_attr_length (insn) + prefix_bytes;
    }

  // Use this hook to record the cycle and flags of INSN in SCHED2.
  int uid = INSN_UID (insn);
  if ((unsigned) uid < (unsigned) lvx_sched2->max_uid
      && GET_CODE (PATTERN (insn)) != USE
      && GET_CODE (PATTERN (insn)) != CLOBBER)
    {
      int prev_uid = lvx_sched2->prev_uid;
      if (prev_uid < 0)
	{
	  // Head of the scheduling region, start a new bundle.
	  lvx_sched2->insn_flags[uid]
	    = LVX_SCHED2_INSN_HEAD | LVX_SCHED2_INSN_START;
	}
      else if (clock > last_clock)
	{
	  // Advanced clock, stop previous bundle and start a new one.
	  lvx_sched2->insn_flags[prev_uid] |= LVX_SCHED2_INSN_STOP;
	  lvx_sched2->insn_flags[uid] = LVX_SCHED2_INSN_START;
	}

      lvx_sched2->insn_cycle[uid] = clock;
      lvx_sched2->prev_uid = uid;

      if (sched_dump && sched_verbose >= 8)
	fprintf (sched_dump,
		 "lvx_sched_dfa_new_cycle(insn %d) clock=%d last=%d bundle_size=%d\n",
		 INSN_UID (insn), clock, last_clock, lvx_sched2->bundle_size);
    }
  return 0;
}

#undef TARGET_SCHED_DFA_NEW_CYCLE
#define TARGET_SCHED_DFA_NEW_CYCLE lvx_sched_dfa_new_cycle

/* Implements TARGET_SCHED_SPECULATE_INSN.
   Called from (sched-ebb.cc:add_deps_for_risky_insns).  */
static int
lvx_sched_speculate_insn (rtx_insn *insn, ds_t ts, rtx *)
{
  if (!reload_completed)
    return -1;

  const_rtx set = 0, mem = 0;
  if ((ts & BEGIN_CONTROL) && ((set = single_set (insn))))
    {
      rtx x = SET_SRC (set);
      subrtx_iterator::array_type array;
      FOR_EACH_SUBRTX (iter, array, x, ALL)
	if (MEM_P ((mem = *iter)))
	  {
	    addr_space_t as = MEM_ADDR_SPACE (mem);
	    if (as == LVX_ADDR_SPACE_PRELOAD || as == LVX_ADDR_SPACE_SPECULATE)
	      return 0;
	  }
    }
  return -1;
}

#undef TARGET_SCHED_SPECULATE_INSN
#define TARGET_SCHED_SPECULATE_INSN lvx_sched_speculate_insn

static void
lvx_sched_set_sched_flags (struct spec_info_def *spec_info)
{
  unsigned int *flags = &(current_sched_info->flags);

  // DO_PREDICATION prevents mapping of REG_DEP_CONTROL to REG_DEP_ANTI.
  if (*flags & SCHED_EBB)	// Implies reload_completed.
    *flags |= DO_PREDICATION;

  // Speculative scheduling is enabled by non-zero spec_info->mask.
  spec_info->mask = 0;
}

#undef TARGET_SCHED_SET_SCHED_FLAGS
#define TARGET_SCHED_SET_SCHED_FLAGS lvx_sched_set_sched_flags

/* Always return true, as long-running instructions are fully pipelined.  */
#undef TARGET_SCHED_CAN_SPECULATE_INSN
#define TARGET_SCHED_CAN_SPECULATE_INSN hook_bool_rtx_insn_true

static void lvx_cms_probe_ddg (struct ddg *, int);

static int
lvx_sched_sms_res_mii (struct ddg *g)
{
  struct lvx_sched_resources resources;
  memset (&resources, 0, sizeof (resources));

  for (int i = 0; i < g->num_nodes; i++)
    {
      rtx_insn *insn = g->nodes[i].insn;
      lvx_sched_resources_add (&resources, insn);
    }

  unsigned full_bundles = lvx_sched_resources_full_bundles (&resources);
  int r = full_bundles ? (int) full_bundles : 1;

  /* The comparison against SMS, run from the same graph.  This hook is the one
     place a target is handed the ddg, once per loop, before scheduling -- which
     is exactly the input and the moment the convergent scheduler needs, so
     kvx-unfold.cc used it the same way.  A piggyback rather than a design: the
     hook's job is to answer ResMII, and the probe is a side effect of being
     called with the right argument.  */
  if (dump_file && lvx_cms_probe)
    lvx_cms_probe_ddg (g, r);

  return r;
}

#undef TARGET_SCHED_SMS_RES_MII
#define TARGET_SCHED_SMS_RES_MII lvx_sched_sms_res_mii

#undef TARGET_SCHED_EXPOSED_PIPELINE
#define TARGET_SCHED_EXPOSED_PIPELINE true

/* FIXME AUTO: revisit for LVX.  */
/* See T7749 */
static int
lvx_sched_reassociation_width (tree_code opc, enum machine_mode mode)
{
  int res = 1;

  /* see tree.c:associative_tree_code () for possible values of opc. */

  switch (opc)
    {
    case BIT_IOR_EXPR:
    case BIT_AND_EXPR:
    case BIT_XOR_EXPR:
      if (mode == SImode || mode == HImode || mode == QImode
	  || mode == DImode)
	res = 4;
      else if (mode == TImode)
	res = 2;
      break;
    case PLUS_EXPR:
    case MIN_EXPR:
    case MAX_EXPR:
      if (mode == SImode || mode == HImode || mode == QImode
	  || mode == DImode)
	res = 4;
      break;
    case MULT_EXPR:
      break;
    }

  return res;
}

#undef TARGET_SCHED_REASSOCIATION_WIDTH
#define TARGET_SCHED_REASSOCIATION_WIDTH lvx_sched_reassociation_width


/* Visit all bundles and force all debug insns after the last insn in
   the bundle. */
static void
lvx_fix_debug_for_bundles (bool selective_scheduling2)
{
  unsigned cur_cfa_reg = REGNO (stack_pointer_rtx);
  rtx_insn *start_insn = 0, *stop_insn = 0;
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    {
      rtx_insn *insn, *last_insn = 0;
      if (selective_scheduling2)
	{
	  FOR_BB_INSNS (bb, insn)
	    {
	      if (NONDEBUG_INSN_P (insn)
		  && GET_CODE (PATTERN (insn)) != USE
		  && GET_CODE (PATTERN (insn)) != CLOBBER)
		{
		  rtx_insn *next_insn = NEXT_INSN (insn);
		  last_insn = insn;
		  if (!start_insn
		      || GET_MODE (insn) == TImode)
		    start_insn = insn;
		  if (insn == BB_END (bb)
		      || GET_MODE (next_insn) == TImode)
		    stop_insn = insn;
		}
	      if (start_insn && stop_insn)
		{
		  lvx_fix_debug_for_bundle_1 (start_insn, stop_insn);
		  cur_cfa_reg
		    = lvx_fix_debug_for_bundle_2 (start_insn, stop_insn,
						  cur_cfa_reg);
		  start_insn = stop_insn = 0;
		}
	    }
	}
      else
	{
	  FOR_BB_INSNS (bb, insn)
	    {
	      if (NONDEBUG_INSN_P (insn)
		  && GET_CODE (PATTERN (insn)) != USE
		  && GET_CODE (PATTERN (insn)) != CLOBBER)
		{
		  int uid = INSN_UID (insn);
		  last_insn = insn;
		  if ((unsigned) uid >= (unsigned) lvx_sched2->max_uid
		      || lvx_sched2->insn_cycle[uid] < 0)
		    {
		      if (!start_insn)
			start_insn = stop_insn = insn;
		    }
		  else
		    {
		      unsigned flags = lvx_sched2->insn_flags[uid];
		      if (flags & LVX_SCHED2_INSN_HEAD)
			cur_cfa_reg = REGNO (stack_pointer_rtx);
		      if (flags & LVX_SCHED2_INSN_START)
			start_insn = insn;
		      else if (!start_insn)
			/* Tail of a bundle whose START flag sits in an
			   earlier block, force closed there: reopen it on
			   this block's first insn so that STOP_INSN can
			   never precede START_INSN.  */
			start_insn = insn;
		      if (flags & LVX_SCHED2_INSN_STOP)
			stop_insn = insn;
		    }
		}
	      if (start_insn && stop_insn)
		{
		  lvx_fix_debug_for_bundle_1 (start_insn, stop_insn);
		  cur_cfa_reg
		    = lvx_fix_debug_for_bundle_2 (start_insn, stop_insn,
						  cur_cfa_reg);
		  start_insn = stop_insn = 0;
		}
	    }
	}

      /* A post-reload scheduling region spans a whole extended basic block,
	 so a bundle still open at BB_END is only closed by the STOP flag of
	 an insn in a *later* block.  Force it closed here: the CFA notes
	 that lvx_fix_debug_for_bundle_1 relocates onto STOP_INSN must stay
	 in the block whose CFI row they describe.  Letting one migrate into
	 a successor makes that successor's predecessors disagree on the
	 unwind row, which trips the cfi_row_equal_p assertion in dwarf2cfi's
	 maybe_record_trace_start.  */
      if (start_insn)
	{
	  if (!stop_insn)
	    stop_insn = last_insn;
	  lvx_fix_debug_for_bundle_1 (start_insn, stop_insn);
	  cur_cfa_reg
	    = lvx_fix_debug_for_bundle_2 (start_insn, stop_insn, cur_cfa_reg);
	  start_insn = stop_insn = 0;
	}
    }
  if (start_insn || stop_insn)
    gcc_assert (!start_insn && !stop_insn);
}

/* Fix the INSN issue cycle accounting for stall effects of AUXR RAW.
   We maintain a scoreboard to compute the cumulative delay implied by stalls
   based on an array of last write dates.  */
static void
lvx_sched2_fix_insn_issue (rtx_insn *insn, rtx *opvec, int noperands)
{
  int uid = INSN_UID (insn);
  gcc_assert ((unsigned) uid < (unsigned) lvx_sched2->max_uid);

  // Scoreboard structure, initialized at each SCHE2 region.
  static struct
  {
    int delay;
    short write[LVX_GPR_LAST_REGNO + 1];
  } scoreboard;
  if (lvx_sched2->insn_flags[uid] & LVX_SCHED2_INSN_HEAD)
    {
      scoreboard.delay = 0;
      memset (scoreboard.write, -1, sizeof (scoreboard.write));
      gcc_assert (LVX_GPR_FIRST_REGNO == 0);
    }

  if (NONDEBUG_INSN_P (insn))
    {
      int stall = 0;
      int cycle = lvx_sched2->insn_cycle[uid] + scoreboard.delay;
      enum attr_type type = get_attr_type (insn);
      if (lvx_accumulate_class_p (type)
	  && noperands > 3 && REG_P (opvec[3]))
	{
	  int regno = REGNO (opvec[3]);
	  int regno_quad = (regno & -4);
	  machine_mode mode = GET_MODE (opvec[3]);
	  unsigned mode_size = GET_MODE_SIZE (mode);
	  gcc_assert (regno <= LVX_GPR_LAST_REGNO);
	  if (mode_size <= UNITS_PER_WORD)
	    {
	      for (int i = 0; i < 4; i += 2)
		{
		  int j = (regno + i) & 3;
		  int write = scoreboard.write[regno_quad + j];
		  int delta = write - cycle;
		  if (stall < delta)
		    stall = delta;
		}
	    }
	  else if (mode_size >= 2 * UNITS_PER_WORD)
	    {
	      for (int i = 0; i < 4; i++)
		{
		  int write = scoreboard.write[regno_quad + i];
		  int delta = write - cycle;
		  if (stall < delta)
		    stall = delta;
		}
	    }
	}
      if (lvx_multiply_class_p (type) && noperands > 0
	  && REG_P (opvec[0]))
	{
	  int regno = REGNO (opvec[0]);
	  machine_mode mode = GET_MODE (opvec[0]);
	  int cost = FLOAT_MODE_P (mode) ? 4 : (INTEGRAL_MODE_P (mode) ? 3 : 1);
	  int i = hard_regno_nregs (regno, mode);
	  while (--i >= 0)
	    {
	      scoreboard.write[regno + i] = cycle + cost + stall;
	    }
	}

      if (stall)
	lvx_sched2->insn_flags[uid] |= LVX_SCHED2_INSN_STALL;
      lvx_sched2->insn_cycle[uid] = cycle + stall;
      scoreboard.delay += stall;
    }
}


/* 18.17 Adjusting the Instruction Scheduler }}} */


/* 18.18 Dividing the Output into Sections {{{ */

#undef TARGET_HAVE_TLS
#define TARGET_HAVE_TLS true

/* 18.18 Dividing the Output into Sections }}} */


/* 18.19 Position Independent Code {{{ */

/* Returns TRUE if X can be used as an operand in PIC code.
 * LABELs are rejected as they should be handled by mov expand
 */

bool
lvx_legitimate_pic_operand_p (rtx x)
{
  if (GET_CODE (x) == SYMBOL_REF || GET_CODE (x) == LABEL_REF
      || (GET_CODE (x) == CONST && GET_CODE (XEXP (x, 0)) == PLUS
	  && (GET_CODE (XEXP (XEXP (x, 0), 0)) == SYMBOL_REF
	      || GET_CODE (XEXP (XEXP (x, 0), 0)) == LABEL_REF)))
    return false;

  return true;
}

/* 18.19 Position Independent Code }}} */


/* 18.20 Defining the Output Assembler Language {{{ */

/* 18.20.2 Output of Data {{{ */

#undef TARGET_ASM_ALIGNED_DI_OP
#define TARGET_ASM_ALIGNED_DI_OP "\t.8byte\t"

/* Recognize machine-specific patterns that may appear within
   constants.  Used for PIC-specific UNSPECs.
   Previously a macro. */
static bool
lvx_output_addr_const_extra (FILE *fp, rtx x)
{
  if (GET_CODE (x) == UNSPEC)
    {
      int unspec_code = XINT ((x), 1);
      switch (unspec_code)
	{
	case UNSPEC_PIC:
	  /* GLOBAL_OFFSET_TABLE or local symbols, no suffix.  */
	  output_addr_const ((fp), XVECEXP ((x), 0, 0));
	  return true;

	case UNSPEC_GOT:
	  fputs ("@got", (fp));
	  fputs ("(", (fp));
	  output_addr_const ((fp), XVECEXP ((x), 0, 0));
	  fputs (")", (fp));
	  return true;

	case UNSPEC_PCREL:
	  fputs ("@pcrel(", (fp));
	  output_addr_const ((fp), XVECEXP ((x), 0, 0));
	  fputs (")", (fp));
	  return true;

	case UNSPEC_GOTOFF:
	  fputs ("@gotoff", (fp));
	  fputs ("(", (fp));
	  output_addr_const ((fp), XVECEXP ((x), 0, 0));
	  fputs (")", (fp));
	  return true;

	case UNSPEC_TLS_GD:
	case UNSPEC_TLS_LD:
	case UNSPEC_TLS_DTPOFF:
	case UNSPEC_TLS_LE:
	case UNSPEC_TLS_IE:
	  fputs (lvx_unspec_tls_asm_op[unspec_code - UNSPEC_TLS_GD], (fp));
	  fputs ("(", (fp));
	  output_addr_const ((fp), XVECEXP ((x), 0, 0));
	  fputs (")", (fp));
	  return true;

	default:
	  return false;
	}
    }
  return false;
}


#undef TARGET_ASM_OUTPUT_ADDR_CONST_EXTRA
#define TARGET_ASM_OUTPUT_ADDR_CONST_EXTRA lvx_output_addr_const_extra
/* 18.20.2 Output of Data }}} */

/* 18.20.7 Output of Assembler Instructions {{{ */

/* The insn being output when it is a guarded COND_EXEC (attribute "guarded",
   set by the define_cond_exec in control.md), else null.  Set by
   FINAL_PRESCAN_INSN, cleared by TARGET_ASM_FINAL_POSTSCAN_INSN, so that
   output_asm_insn calls from outside final_scan_insn (the thunk generator)
   never see a stale one.  */
static rtx_insn *lvx_final_guarded_insn;

/* Print the "guard.<cond> $rN? " prefix of the guarded INSN to STREAM.  The
   spelling is the one the former define_cond_exec templates produced:
   "guard.<w|d><cond>z" for a comparison of a register with zero, "guard.even"
   / "guard.odd" for the test of its bit 0.  */
static void
lvx_output_guard_prefix (FILE *stream, rtx_insn *insn)
{
  rtx test = COND_EXEC_TEST (PATTERN (insn));
  rtx reg = XEXP (test, 0);
  if (GET_CODE (reg) == ZERO_EXTRACT)
    fprintf (stream, "guard.%s $%s? ", GET_CODE (test) == EQ ? "even" : "odd",
	     reg_names[REGNO (XEXP (reg, 0))]);
  else
    fprintf (stream, "guard.%c%sz $%s? ", GET_MODE (reg) == SImode ? 'w' : 'd',
	     GET_RTX_NAME (GET_CODE (test)), reg_names[REGNO (reg)]);
}

/* Implements ASM_OUTPUT_OPCODE.  final.cc calls this at the start of every
   line of an insn's template, which is where a guarded insn's every syllable
   receives its GUARD prefix.  */
const char *
lvx_asm_output_opcode (FILE *stream, const char *code)
{
  if (!lvx_shaker_enabled && lvx_final_end_bundle[0])
    fputs(lvx_final_end_bundle, stream);

  if (lvx_final_guarded_insn)
    lvx_output_guard_prefix (stream, lvx_final_guarded_insn);

  return code;
}

/* Implements FINAL_PRESCAN_INSN.  */
void
lvx_final_prescan_insn (rtx_insn *insn)
{
  lvx_final_end_bundle[0] = '\0';

  lvx_final_guarded_insn = 0;
  if (GET_CODE (PATTERN (insn)) == COND_EXEC && INSN_CODE (insn) >= 0
      && get_attr_guarded (insn) == GUARDED_YES)
    lvx_final_guarded_insn = insn;

  // Fix missing bundle delimiter (because of deleted insn after SCHED2).
  int uid = INSN_UID (insn);
  if ((unsigned) uid < (unsigned) lvx_sched2->max_uid
      && lvx_sched2->insn_cycle && lvx_sched2->insn_cycle[uid] >= 0)
    {
      static int prev_uid;
      static int prev_cycle;
      static unsigned prev_flags;

      int cycle = lvx_sched2->insn_cycle[uid];
      unsigned flags = lvx_sched2->insn_flags[uid];

      if (!(flags & LVX_SCHED2_INSN_HEAD) && prev_cycle < cycle)
	if (!(prev_flags & LVX_SCHED2_INSN_STOP))
	  {
	    gcc_assert ((unsigned) prev_uid < (unsigned) lvx_sched2->max_uid);
	    lvx_sched2->insn_flags[prev_uid] |= LVX_SCHED2_INSN_STOP;
	    sprintf (lvx_final_end_bundle,
		     ";;\t# (end cycle %d)\t(fixup)\n\t", prev_cycle);
	  }

      prev_flags = flags;
      prev_cycle = cycle;
      prev_uid = uid;
    }
}

/* Implements TARGET_ASM_FINAL_POSTSCAN_INSN.  */
static void
lvx_asm_final_postscan_insn (FILE *file, rtx_insn *insn,
			     rtx *opvec ATTRIBUTE_UNUSED,
			     int noperands ATTRIBUTE_UNUSED)
{
  lvx_final_guarded_insn = 0;

  int doloop_end = recog_memoized (insn) == CODE_FOR_doloop_end_si
		   || recog_memoized (insn) == CODE_FOR_doloop_end_di;

  if (lvx_sched2->insn_cycle)
    {
      static int prev_cycle;
      static int jump_cycle;

      int uid = INSN_UID (insn);
      if ((unsigned) uid >= (unsigned) lvx_sched2->max_uid
	  || lvx_sched2->insn_cycle[uid] < 0)
	{
	  if (TARGET_SCHED2_DATES)
	    fprintf (file, "\t;;\t# (unscheduled)\n");
	  else
	    fprintf (file, "\t;;\n");
	  return;
	}


      int cycle = lvx_sched2->insn_cycle[uid];
      unsigned flags = lvx_sched2->insn_flags[uid];

      if (flags & LVX_SCHED2_INSN_STOP)
	{
	  /* The doloop_end insn prints no syllable of its own -- it is the
	     "# loopdo end" comment -- but it does carry the STOP flag of the
	     bundle it shares with the last instructions of the loop body, and
	     that bundle has to be closed here.  The hardware takes the back
	     edge when the address of the *next* bundle reaches the end label,
	     so a bundle left open puts the exit block's first instructions
	     inside the loop and never lands on the end label at all; the
	     assembler meanwhile keeps filling that bundle past what one can
	     hold.  When the body's last bundle ends on a branch, a taken
	     branch wins over the loop-back, so a nop bundle is appended to
	     stand as the last one in its place.  Nothing is printed when the
	     marker is alone in a bundle of its own: the instruction before it
	     closed the body already, and a second ";;" would be an empty
	     bundle.  */
	  const char *note = "";
	  if (doloop_end && jump_cycle == prev_cycle)
	    {
	      fprintf (file, "\tnop\n\t;;\n");
	      note = 0;
	    }
	  else if (doloop_end && cycle != prev_cycle)
	    note = 0;
	  else if (!doloop_end && (flags & LVX_SCHED2_INSN_STALL))
	    note = "\t(stalled)";
	  else if (doloop_end)
	    note = "\t(loop end)";

	  if (note)
	    {
	      if (TARGET_SCHED2_DATES)
		fprintf (file, "\t;;\t# (end cycle %d)%s\n", cycle, note);
	      else
		fprintf (file, "\t;;\n");
	    }
	}

      prev_cycle = cycle;
      if (flags & LVX_SCHED2_INSN_HEAD)
	jump_cycle = -1;
      if (JUMP_P (insn) || CALL_P (insn))
	jump_cycle = prev_cycle;
    }
  else
    {
      fprintf (file, "\t;;\n");
      if (doloop_end)
	fprintf (file, "\tnop\n\t;;\n");
    }

  return;
}

#undef TARGET_ASM_FINAL_POSTSCAN_INSN
#define TARGET_ASM_FINAL_POSTSCAN_INSN lvx_asm_final_postscan_insn

/* TARGET_PRINT_OPERAND {{{ */

static const char *
lvx_pgr_reg_name (unsigned regno)
{
  static const char *pgr_reg_names[] = { LVX_PGR_REGISTER_NAMES };
  unsigned index = regno - LVX_GPR_FIRST_REGNO;
  gcc_assert (index % 2 == 0);
  return pgr_reg_names[index / 2];
}

static const char *
lvx_qgr_reg_name (unsigned regno)
{
  static const char *qgr_reg_names[] = { LVX_QGR_REGISTER_NAMES };
  unsigned index = regno - LVX_GPR_FIRST_REGNO;
  gcc_assert (index % 4 == 0);
  return qgr_reg_names[index / 4];
}

static const char *
lvx_xvr_reg_name (unsigned regno)
{
  static const char *xvr_reg_names[] = { LVX_XVR_REGISTER_NAMES };
  unsigned index = regno - LVX_XCR_FIRST_REGNO;
  gcc_assert (index % 2 == 0 || index % 4 == 0);
  if (index % 2 == 0 && index % 4 != 0)
    return xvr_reg_names[(index - 2) / 4];
  return xvr_reg_names[index / 4];
}

static const char *
lvx_xtr_reg_name (unsigned regno)
{
  static const char *xwr_reg_names[] = { LVX_XTR_REGISTER_NAMES };
  unsigned index = regno - LVX_XCR_FIRST_REGNO;
  gcc_assert (index % 8 == 0);
  return xwr_reg_names[index / 8];
}

static const char *
lvx_xmr_reg_name (unsigned regno)
{
  static const char *xmr_reg_names[] = { LVX_XMR_REGISTER_NAMES };
  unsigned index = regno - LVX_XCR_FIRST_REGNO;
  gcc_assert (index % 16 == 0);
  return xmr_reg_names[index / 16];
}

static const char *
lvx_regname (rtx x)
{
  machine_mode mode = GET_MODE (x);

  switch (GET_CODE (x))
    {
    case REG:
      if (GET_MODE_SIZE (mode) <= UNITS_PER_WORD)
	return reg_names[REGNO (x)];
      else if (GET_MODE_SIZE (mode) <= 2 * UNITS_PER_WORD)
	return lvx_pgr_reg_name (REGNO (x));
      else if (GET_MODE_SIZE (mode) <= 4 * UNITS_PER_WORD)
	return lvx_qgr_reg_name (REGNO (x));
      gcc_unreachable ();
    case SUBREG:
      gcc_unreachable ();
    default:
      gcc_unreachable ();
    }
}


void
lvx_print_operand (FILE *file, rtx x, int code)
{
  rtx operand = x;
  bool force_breg = 0;
  bool force_qreg = 0;
  bool force_preg = 0;
  bool force_mreg = 0;
  bool force_lreg = 0;
  bool force_treg = 0;
  bool force_zreg = 0;
  bool force_yreg = 0;
  bool force_xreg = 0;
  bool addr_mode = false;
  bool as_address = false;
  bool float_compare = false;
  bool reverse_compare = false;
  bool swap_compare = false;

  lvx_print_offset_zero = true;
  switch (code)
    {
    case 0:
      /* No code, print as usual.  */
      break;

    case 'b':
      force_breg = true;
      break;

    case 'o':
      force_qreg = true;
      break;

    /* %L / %M: the least- and most-significant 128-bit half of a 256-bit
       operand, so a 256-bit operation can be issued as two 128-bit
       instructions.  Uppercase deliberately: output_asm_insn in final.cc
       intercepts %l (label), %a (address), %c and %n before they ever reach
       a target's print_operand, so a lowercase 'l' case is unreachable.  */
    case 'M':
      force_mreg = true;
      break;

    case 'L':
      force_lreg = true;
      break;

    case 'q':
      force_preg = true;
      break;

    case 't':
      force_treg = true;
      break;

    case 'z':
      force_zreg = true;
      break;

    case 'y':
      force_yreg = true;
      break;

    case 'x':
      force_xreg = true;
      break;

    case 'A':
      as_address = true;
      break;

    case 'F':
      float_compare = true;
      break;

    case 'R':
      reverse_compare = true;
      break;

    case 'S':
      swap_compare = true;
      break;

    case 'O':
      lvx_print_offset_zero = false;
      break;

    case 'T':
      fprintf (file, "@pcrel(");
      output_addr_const (file, operand);
      fprintf (file, ")");
      return;

    case 'V': /* Print the load variant: '.s', '.u' or '.us'.  */
      fputs (lvx_variant_suffix (lvx_mem_variant (x)), file);
      addr_mode = true;
      break;

    case 'W':
      /* The 32-bit word of a splattable vector constant, which is all a .M
         format encodes -- the machine replicates it over the operand.  */
      fprintf (file, "0x%08" HOST_LONG_FORMAT "x",
               (unsigned long) (lvx_splat32_value (x) & 0xFFFFFFFF));
      return;

    case 'X':
      addr_mode = true;
      break;

    default:
      output_operand_lossage ("invalid operand code '%c'", code);
    }

  if ((as_address || addr_mode) && GET_CODE (x) != MEM)
    {
      x = gen_rtx_MEM (Pmode, x);
      operand = x;
    }

  if (COMPARISON_P (x))
    {
      enum rtx_code code = GET_CODE (x);
      if (!float_compare)
	{
	  if (reverse_compare)
	    code = reverse_condition (code);
	  else if (swap_compare)
	    code = swap_condition (code);
	  fprintf (file, "%s", GET_RTX_NAME (code));
	}
      else
	{
	  const char *name = 0;
	  switch (code)
	    {
	    case NE:
	      name = "une";
	      break;
	    case EQ:
	      name = "oeq";
	      break;
	    case GE:
	      name = "oge";
	      break;
	    case LT:
	      name = "olt";
	      break;
	    case UNEQ:
	      name = "ueq";
	      break;
	    case UNGE:
	      name = "uge";
	      break;
	    case UNLT:
	      name = "ult";
	      break;
	    case LTGT:
	      name = "one";
	      break;
	    default:
	      gcc_unreachable ();
	    }
	  fprintf (file, "%s", name);
	}
      return;
    }

  unsigned size = GET_MODE_SIZE (GET_MODE (x));
  switch (GET_CODE (operand))
    {
    case REG:
      if (REGNO (operand) >= FIRST_PSEUDO_REGISTER)
	error ("incorrect hard register number %d", REGNO (operand));
      else if (extension_register_operand (operand, VOIDmode))
	{
	  if (force_xreg)
	    fprintf (file, "$%s", lvx_xvr_reg_name (REGNO (operand)));
	  else if (force_breg)
	    {
	      unsigned nregs = size / UNITS_PER_WORD;
	      fprintf (file, "$%s..%s", lvx_xvr_reg_name (REGNO (operand)),
		       lvx_xvr_reg_name (REGNO (operand) + nregs - 4));
	    }
	  else if (size == UNITS_PER_WORD * 16)
	    fprintf (file, "$%s", lvx_xmr_reg_name (REGNO (operand)));
	  else if (size == UNITS_PER_WORD * 8)
	    fprintf (file, "$%s", lvx_xtr_reg_name (REGNO (operand)));
	  else
	    fprintf (file, "$%s", lvx_xvr_reg_name (REGNO (operand)));
	}
      else if (force_qreg)
	fprintf (file, "$%s", lvx_qgr_reg_name (REGNO (operand)));
      else if (force_preg)
	fprintf (file, "$%s", lvx_pgr_reg_name (REGNO (operand)));
      else if (force_mreg)
	{
	  if (size < UNITS_PER_WORD * 4)
	    error ("using %%M format with operand smaller than 4 registers");
	  fprintf (file, "$%s", lvx_pgr_reg_name (REGNO (operand) + 2));
	}
      else if (force_lreg)
	{
	  if (size < UNITS_PER_WORD * 4)
	    error ("using %%L format with operand smaller than 4 registers");
	  fprintf (file, "$%s", lvx_pgr_reg_name (REGNO (operand)));
	}
      else if (force_treg)
	{
	  if (size < UNITS_PER_WORD * 4)
	    error ("using %%t format with operand smaller than 4 registers");
	  fprintf (file, "$%s", reg_names[REGNO (operand) + 3]);
	}
      else if (force_zreg)
	{
	  if (size < UNITS_PER_WORD * 4)
	    error ("using %%z format with operand smaller than 4 registers");
	  fprintf (file, "$%s", reg_names[REGNO (operand) + 2]);
	}
      else if (force_yreg)
	{
	  if (size < UNITS_PER_WORD * 2)
	    error ("using %%y format with operand smaller than 2 registers");
	  fprintf (file, "$%s", reg_names[REGNO (operand) + 1]);
	}
      else if (force_xreg)
	{
	  if (size < UNITS_PER_WORD * 2)
	    error ("using %%x format with operand smaller than 2 registers");
	  fprintf (file, "$%s", reg_names[REGNO (operand)]);
	}
      else if (system_register_operand (operand, VOIDmode))
	fprintf (file, "$%s", reg_names[REGNO (operand)]);
      else if (general_register_operand (operand, VOIDmode))
	{
	  if (size == UNITS_PER_WORD * 4)
	    fprintf (file, "$%s", lvx_qgr_reg_name (REGNO (operand)));
	  else if (size == UNITS_PER_WORD * 2)
	    fprintf (file, "$%s", lvx_pgr_reg_name (REGNO (operand)));
	  else
	    fprintf (file, "$%s", reg_names[REGNO (operand)]);
	}
      else
	gcc_unreachable ();
      return;

    case MEM:
      if (addr_mode)
	{
	  if (HAVE_LVX_ADDRESSING_XS)
	    {
	      x = XEXP (x, 0);
	      if (GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 0)) == MULT
		  && GET_CODE (XEXP (XEXP (x, 0), 1)) == CONST_INT
		  && INTVAL (XEXP (XEXP (x, 0), 1)) > HOST_WIDE_INT_1)
		fprintf (file, ".xs");
	    }
	}
      else
	{
	  x = XEXP (x, 0);
	  if (GET_CODE (x) == UNSPEC)
	    lvx_print_operand (file, x, 0);
	  else
	    output_address (GET_MODE (XEXP (operand, 0)), XEXP (operand, 0));
	}
      return;

    case CONST_DOUBLE:
      {
	long l[2];
	REAL_VALUE_TYPE r = *CONST_DOUBLE_REAL_VALUE (operand);
	if (GET_MODE (x) == HFmode)
	  {
	    REAL_VALUE_TO_TARGET_SINGLE (r, l[0]);
	    l[0] = lvx_float_to_half_as_int (l[0]);
	    fprintf (file, "0x%04x", (unsigned int) l[0]);
	    return;
	  }
	else if (GET_MODE (x) == SFmode)
	  {
	    REAL_VALUE_TO_TARGET_SINGLE (r, l[0]);
	    fprintf (file, "0x%08x", (unsigned int) l[0]);
	    return;
	  }
	else if (GET_MODE (x) == DFmode)
	  {
	    REAL_VALUE_TO_TARGET_DOUBLE (r, l);
	    fprintf (file, "0x%08x%08x", (unsigned int) l[1],
		     (unsigned int) l[0]);
	    return;
	  }
	else if (GET_MODE (x) == HCmode)
	  {
	    REAL_VALUE_TO_TARGET_SINGLE (r, l[0]);
	    fprintf (file, "0x%08x", (unsigned int) l[0]);
	    return;
	  }
	else if (GET_MODE (x) == SCmode)
	  {
	    REAL_VALUE_TO_TARGET_DOUBLE (r, l);
	    fprintf (file, "0x%08x%08x", (unsigned int) l[1],
		     (unsigned int) l[0]);
	    return;
	  }
	else if (GET_MODE (x) == DCmode)
	  {
	    gcc_unreachable ();
	  }
      }
      gcc_unreachable ();
      return;


    case CONST_INT:
      {
	HOST_WIDE_INT value = INTVAL (x);
	int signed10 = value >= -512 && value < 512;
	const char *format = signed10 ? HOST_WIDE_INT_PRINT_DEC :
	  "0x" HOST_WIDE_INT_PRINT_PADDED_HEX;
	fprintf (file, format, INTVAL (x));
      }
      return;

    case CONST_VECTOR:
      {
	int slice = 1 * force_yreg + 2 * force_zreg + 3 * force_treg;
	fprintf (file, "0x" HOST_WIDE_INT_PRINT_PADDED_HEX,
		 lvx_const_vector_value (x, slice));
      }
      return;

    case CONST_STRING:
      /* Case for modifier strings */
      fputs (XSTR (operand, 0), file);
      return;

    case CONCAT:
      {
	/* Case to concatenate modifier strings */
	rtx left = XEXP (operand, 0);
	rtx right = XEXP (operand, 1);
	lvx_print_operand (file, left, code);
	lvx_print_operand (file, right, code);
	return;
      }

    default:
      {
	int is_unspec = 0, unspec;

	if (GET_CODE (operand) == CONST)
	  operand = XEXP (operand, 0);

	if (GET_CODE (operand) == UNSPEC)
	  {
	    is_unspec = 1;
	    unspec = XINT (operand, 1);
	    operand = XVECEXP (operand, 0, 0);
	  }

	if (is_unspec)
	  {
	    switch (unspec)
	      {
	      case UNSPEC_TLS_GD:
	      case UNSPEC_TLS_LD:
	      case UNSPEC_TLS_DTPOFF:
	      case UNSPEC_TLS_LE:
	      case UNSPEC_TLS_IE:
		fputs (lvx_unspec_tls_asm_op[unspec - UNSPEC_TLS_GD], (file));
		fputs ("(", (file));
		break;
	      case UNSPEC_GOT:
		fprintf (file, "@got(");
		break;
	      case UNSPEC_GOTOFF:
		fprintf (file, "@gotoff(");
		break;
	      case UNSPEC_PCREL:
		fprintf (file, "@pcrel(");
		break;
	      default:
		gcc_unreachable ();
	      }
	  }

	/* No need to handle all strange variants, let output_addr_const
	   do it for us.  */
	output_addr_const (file, operand);
	if (is_unspec)
	  fprintf (file, ")");
      }
    }
}

#undef TARGET_PRINT_OPERAND
#define TARGET_PRINT_OPERAND lvx_print_operand
/* TARGET_PRINT_OPERAND }}} */

bool
lvx_print_punct_valid_p (unsigned char code)
{
  return code == ';';
}

#undef TARGET_PRINT_OPERAND_PUNCT_VALID_P
#define TARGET_PRINT_OPERAND_PUNCT_VALID_P lvx_print_punct_valid_p

void
lvx_print_operand_address (FILE *file, machine_mode /*mode*/, rtx x)
{
  switch (GET_CODE (x))
    {
    case REG:
    case SUBREG:
      if (lvx_print_offset_zero)
	fprintf (file, "0[$%s]", lvx_regname (x));
      else
	fprintf (file, "[$%s]", lvx_regname (x));
      break;

    case PLUS:
      switch (GET_CODE (XEXP (x, 1)))
	{
	case CONST_INT:
	  fprintf (file, HOST_WIDE_INT_PRINT_DEC, INTVAL (XEXP (x, 1)));
	  break;
	case LABEL_REF:
	case SYMBOL_REF:
	case CONST:
	case UNSPEC:
	  output_addr_const (file, XEXP (x, 1));
	  break;
	case REG:
	case SUBREG:
	  if (GET_CODE (XEXP (x, 0)) == MULT)
	    fprintf (file, "$%s", lvx_regname (XEXP (XEXP (x, 0), 0)));
	  else
	    fprintf (file, "$%s", lvx_regname (XEXP (x, 1)));
	  break;
	default:
	  abort ();
	}
      if (GET_CODE (XEXP (x, 0)) == MULT)
	fprintf (file, "[$%s]", lvx_regname (XEXP (x, 1)));
      else
	fprintf (file, "[$%s]", lvx_regname (XEXP (x, 0)));
      return;

    default:
      output_addr_const (file, x);
      break;
    }
}

#undef TARGET_PRINT_OPERAND_ADDRESS
#define TARGET_PRINT_OPERAND_ADDRESS lvx_print_operand_address

/* 18.20.7 Output of Assembler Instructions }}} */

/* 18.20 Defining the Output Assembler Language }}} */


/* 18.21.5 Macros for DWARF Output {{{ */

#undef TARGET_DELAY_SCHED2
#define TARGET_DELAY_SCHED2 true

#undef TARGET_DELAY_VARTRACK
#define TARGET_DELAY_VARTRACK true

/* 18.21.5 Macros for DWARF Output }}} */


/* 18.24 Defining target-specific uses of '__attribute__' {{{ */

/* Handle an attribute requiring a FUNCTION_DECL;
   arguments as in struct attribute_spec.handler.  */
static tree
lvx_handle_fndecl_attribute (tree *node, tree name, tree, int,
			     bool *no_add_attrs)
{
  if (TREE_CODE (*node) != FUNCTION_DECL)
    {
      warning (OPT_Wattributes, "%qE attribute only applies to functions",
	       name);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Table of machine attributes.  */
TARGET_GNU_ATTRIBUTES (lvx_attribute_table,
{
  /* { name, min_len, max_len, decl_req, type_req, fn_type_req, handler,
     affects_type } */
  {"no_save_regs", 0, 0, true, false, false, false, lvx_handle_fndecl_attribute,
   NULL},
  {"farcall", 0, 0, true, false, false, false, lvx_handle_fndecl_attribute, NULL},
});

#undef TARGET_ATTRIBUTE_TABLE
#define TARGET_ATTRIBUTE_TABLE lvx_attribute_table

/* Implements TARGET_OPTION_OVERRIDE.  */

static void
lvx_option_override (void)
{
  int i;
  cl_deferred_option *opt;
  vec < cl_deferred_option > *v =
    (vec < cl_deferred_option > *)lvx_deferred_options;

  if (v)
    FOR_EACH_VEC_ELT (*v, i, opt)
    {
      switch (opt->opt_index)
	{
	case OPT_ffixed_reg:
	  lvx_handle_fixed_reg_option (opt->arg);
	  break;
	case OPT_fstack_limit_register_:
	  lvx_handle_stack_limit_register_option (opt->arg);
	  break;
	case OPT_fstack_limit_symbol_:
	  lvx_handle_stack_limit_symbol_option (opt->arg);
	  break;
	case OPT_fshaker_seed_:
	  lvx_shaker_enabled = true;
	  break;
	default:
	  gcc_unreachable ();
	}
    }

  /* THE TWO UNSOUND MEASUREMENT SWITCHES MAY NOT REACH A REAL COMPILE.
     §7decies, the one retirement item with a correctness deadline.

     `-mcms-assume-ivdep' drops genuine cross-iteration memory dependences on
     an assumption nothing verifies -- measured in §7sexvicies as changing 103
     of 162 loops' recurrence bound, so on PolyBench it would miscompile 162 of
     162 rather than some corner.  `-mcms-inductive-relaxation' drops arcs that
     are legal only once codegen performs the displacement adjustment of
     PaCT 1997 §4 (`fixRelaxed'), which does not exist yet.

     Both are harmless only while the probe emits nothing.  The moment CMS
     generates code each becomes a flag that silently produces wrong programs,
     so they are tied to the probe HERE, before any codegen exists to be
     endangered -- an error a user cannot miss rather than a miscompile they
     cannot see.  An error, not a silent disable: a sweep that forgot
     `-mcms-probe' would otherwise quietly measure a different compiler than it
     reported.

     `-mcms-force-speculative-loads' is deliberately NOT gated.  It promotes a
     loop-variant-address load to its dismissible variant, which is what the
     schedule it measures actually requires, so it becomes mandatory rather
     than dangerous once codegen exists.  */
  if (lvx_cms_assume_ivdep && !lvx_cms_probe)
    error ("%<-mcms-assume-ivdep%> drops real cross-iteration memory "
	   "dependences and is for measurement only; it requires "
	   "%<-mcms-probe%>");
  if (lvx_cms_inductive_relaxation && !lvx_cms_probe)
    error ("%<-mcms-inductive-relaxation%> needs a displacement adjustment in "
	   "code generation that does not exist yet, and is for measurement "
	   "only; it requires %<-mcms-probe%>");

  /* Unfolding an inner loop implies renaming through it.  The dependence
     model the unfold is justified by drops the anti- and output dependences
     register allocation introduced, which is sound only because a renaming
     step follows and removes them; without it the two copies reuse the same
     registers and serialise against each other, which measures as pure cost.
     SET_OPTION_IF_UNSET, so an explicit -mno-regrename-inner-loops still
     wins -- the implication is the default, not a constraint.  */
  if (lvx_unfold_inner_loops)
    SET_OPTION_IF_UNSET (&global_options, &global_options_set,
			 lvx_regrename_inner_loops_p, 1);

#if 0 // If 1 then gcc/gcc/testsuite/gcc.dg/ipa/iinline-attr.c fails.
  SET_OPTION_IF_UNSET (&global_options, &global_options_set,
		       param_simultaneous_prefetches, 8);
  // Size in bytes of the L1D cache line.
  SET_OPTION_IF_UNSET (&global_options, &global_options_set,
		       param_l1_cache_line_size, 64);
  // Size in KB of the L1D cache.
  SET_OPTION_IF_UNSET (&global_options, &global_options_set,
		       param_l1_cache_size, 16);
#endif

  /* Partial vectors in the EPILOGUE only, never in the main loop.  The
     generic default is 2 -- mask every loop -- and LVX started inheriting it
     the moment cond_add<mode> made reductions eligible for partial vectors at
     all: every sum loop's main body became a masked one, recomputing a lane
     mask per iteration (COMP*, ANDW, EXTB*D), loading through a MASKM prefix
     and merging each accumulator with a BLEND*, where an unmasked body needs
     none of that.  At 1 the main loop keeps full vectors and only the tail is
     masked, which is where masking pays for itself: there it replaces a scalar
     epilogue rather than taxing the hot path.

     SET_OPTION_IF_UNSET, so --param=vect-partial-vector-usage=2 still asks for
     the fully masked form -- the knob is how the two were measured against each
     other, and the pin is only which one is the default.  */
  SET_OPTION_IF_UNSET (&global_options, &global_options_set,
		       param_vect_partial_vector_usage, 1);

  lvx_arch_schedule = LVX_2 ? ARCH_LVX_2 : ARCH_LVX_1;

#ifndef __OPTIMIZE__
  const char *LVX_COST_FACTOR = getenv ("LVX_COST_FACTOR");
  if (LVX_COST_FACTOR)
    lvx_cost_factor = atoi (LVX_COST_FACTOR);
  const char *LVX_IFCVT_COUNT = getenv ("LVX_IFCVT_COUNT");
  if (LVX_IFCVT_COUNT)
    lvx_ifcvt_count = atoi (LVX_IFCVT_COUNT);
  const char *LVX_ASPEC_COUNT = getenv ("LVX_ASPEC_COUNT");
  if (LVX_ASPEC_COUNT)
    lvx_aspec_count = atoi (LVX_ASPEC_COUNT);
#endif //__OPTIMIZE__
}

#undef TARGET_OPTION_OVERRIDE
#define TARGET_OPTION_OVERRIDE lvx_option_override

/* 18.24 Defining target-specific uses of '__attribute__' }}} */


/* 18.30 Adding support for named address spaces {{{  */

/* Implements TARGET_ADDR_SPACE_POINTER_MODE.  */
static scalar_int_mode
lvx_addr_space_pointer_mode (addr_space_t)
{
  return ptr_mode;
}

#undef TARGET_ADDR_SPACE_POINTER_MODE
#define TARGET_ADDR_SPACE_POINTER_MODE lvx_addr_space_pointer_mode

/* Implements TARGET_ADDR_SPACE_ADDRESS_MODE.  */
static scalar_int_mode
lvx_addr_space_address_mode (addr_space_t)
{
  return Pmode;
}

#undef TARGET_ADDR_SPACE_ADDRESS_MODE
#define TARGET_ADDR_SPACE_ADDRESS_MODE lvx_addr_space_address_mode

/* Implements TARGET_ADDR_SPACE_LEGITIMATE_ADDRESS_P.  */
static bool
lvx_addr_space_legitimate_address_p (machine_mode mode, rtx exp, bool strict,
				     addr_space_t as ATTRIBUTE_UNUSED,
                                     code_helper ch = ERROR_MARK)
{
  switch (as)
    {
    default:
      gcc_unreachable ();

    case ADDR_SPACE_GENERIC:
    case LVX_ADDR_SPACE_BYPASS:
    case LVX_ADDR_SPACE_PRELOAD:
    case LVX_ADDR_SPACE_SPECULATE:
      return lvx_legitimate_address_p (mode, exp, strict, ch);

    case LVX_ADDR_SPACE_SYSCALL:
      return mode == DImode || mode == SImode;
    case LVX_ADDR_SPACE_CONVERT:
      return false;
    }
}

#undef TARGET_ADDR_SPACE_LEGITIMATE_ADDRESS_P
#define TARGET_ADDR_SPACE_LEGITIMATE_ADDRESS_P                                 \
  lvx_addr_space_legitimate_address_p

/* Implements TARGET_ADDR_SPACE_LEGITIMIZE_ADDRESS.  */
static rtx
  lvx_addr_space_legitimize_address (rtx x, rtx oldx, machine_mode mode,
				     addr_space_t as)
{
  if (as == LVX_ADDR_SPACE_CONVERT)
    error ("%<__convert%> should be used only in explicit pointer casting");

  return lvx_legitimize_address (x, oldx, mode);
}

#undef TARGET_ADDR_SPACE_LEGITIMIZE_ADDRESS
#define TARGET_ADDR_SPACE_LEGITIMIZE_ADDRESS lvx_addr_space_legitimize_address

/* Implements TARGET_ADDR_SPACE_SUBSET_P.  */
static bool
lvx_addr_space_subset_p (addr_space_t subset ATTRIBUTE_UNUSED,
			 addr_space_t superset ATTRIBUTE_UNUSED)
{
  return true;
}
#undef TARGET_ADDR_SPACE_SUBSET_P
#define TARGET_ADDR_SPACE_SUBSET_P lvx_addr_space_subset_p

/* Implements TARGET_ADDR_SPACE_CONVERT.  */
static rtx
lvx_addr_space_convert (rtx op, tree from_type, tree to_type)
{
  if (LVX_WARN_ADDRESS_SPACE_CONVERSION
      && TYPE_ADDR_SPACE (TREE_TYPE (from_type)) != LVX_ADDR_SPACE_CONVERT
      && TYPE_ADDR_SPACE (TREE_TYPE (to_type)) != LVX_ADDR_SPACE_CONVERT)
    {

      warning (0,
	       TYPE_ADDR_SPACE (TREE_TYPE (from_type)) > ADDR_SPACE_GENERIC
		 ? "Implicit conversion from uncached pointer to cached one"
		 : "Implicit conversion from cached pointer to uncached one");
      inform (input_location,
	      "Use %<(__convert <type> *)%> to acknowledge this conversion");
    }
  return op;
}

#undef TARGET_ADDR_SPACE_CONVERT
#define TARGET_ADDR_SPACE_CONVERT lvx_addr_space_convert

/* 18.30 Adding support for named address spaces }}}  */


/* 18.31 Miscellaneous Parameters {{{ */

#undef TARGET_CONST_ANCHOR
#define TARGET_CONST_ANCHOR 0x200

/* ifcvt utils {{{ */

/* If-conversion on LVX runs three times.  CE1 and CE2, before register
   allocation, are the generic noce pass: an if-then-else that computes one
   value becomes an if_then_else, which is a CMOVED.  CE3, after reload, is
   the generic cond_exec pass, and on LVX a conditionally executed insn is a
   GUARD prefix: the BCU syllable "guard.<cond> $rN?" that suppresses the
   guarded units of its bundle when the condition is false.  Any pattern
   with predicable=yes (the default, see lvx.md) has a COND_EXEC variant for
   it, so CE3 needs no scratch register and no rewriting of the insns; only
   the BCU and singleton-bundle instructions cannot be guarded.

   What the target still has to do is done here, through IFCVT_MACHDEP_INIT
   in CE2 and IFCVT_MODIFY_TESTS/INSN in CE3:

   - In CE2, decide whether the then/else blocks of an if header are what
     CE3 can convert (every insn recognisable as a COND_EXEC, no jump or
     call, no more than MAX_CONDITIONAL_EXECUTE) and, if so, append a USE
     of the tested register to each block.  Register allocation then keeps
     that register live to the end of the blocks, where the guards need it;
     without the USE its range ends at the jump and the allocator is free to
     reuse it inside the blocks.

   - In CE3, speculate what is free: a non-memory, non-trapping insn whose
     destination is dead after the block and unused by the other block runs
     unconditionally, costing no guard syllable.  Everything else is guarded.

   Until 2026-09-20 arithmetic was pseudo-predicated the KV3 way: computed
   into a scratch register scavenged after reload, then CMOVED into place,
   through a fake-SFR placeholder set in CE2, the *wrapped_* patterns of
   if-convert.md and UNSPEC_DEF.  A guarded insn is one syllable where that
   was two plus a register.  */

static void
lvx_ifcvt_ctor (void)
{
  if (dump_file)
    fprintf (dump_file, "LVX_IFCVT_CTOR(%s, CE%d)\n",
	     current_function_name (), (int) lvx_ifcvt_ce_level);
  memset (lvx_ifcvt, 0, sizeof (struct lvx_ifcvt));
  lvx_ifcvt->ce_level = lvx_ifcvt_ce_level;
  lvx_ifcvt->block_count = last_basic_block_for_fn (cfun);
  lvx_ifcvt->block_visited = sbitmap_alloc (lvx_ifcvt->block_count);
  bitmap_clear (lvx_ifcvt->block_visited);
  lvx_ifcvt->recog_insn = make_insn_raw (NULL_RTX);
  SET_NEXT_INSN (lvx_ifcvt->recog_insn) = 0;
  SET_PREV_INSN (lvx_ifcvt->recog_insn) = 0;
  INSN_LOCATION (lvx_ifcvt->recog_insn) = UNKNOWN_LOCATION;
}

static void
lvx_ifcvt_reset (void)
{
  if (dump_file)
    fprintf (dump_file, "LVX_IFCVT_RESET(%s, CE%d)\n",
	     current_function_name (), (int) lvx_ifcvt->ce_level);
  lvx_ifcvt->then_start = lvx_ifcvt->then_end = 0;
  lvx_ifcvt->else_start = lvx_ifcvt->else_end = 0;
}

static void
lvx_ifcvt_dtor (void)
{
  if (dump_file)
    fprintf (dump_file, "LVX_IFCVT_DTOR(%s, CE%d)\n",
	     current_function_name (), (int) lvx_ifcvt->ce_level);
  if (lvx_ifcvt->block_visited)
    sbitmap_free (lvx_ifcvt->block_visited);
  memset (lvx_ifcvt, 0, sizeof (struct lvx_ifcvt));
}

/* Get the test of a register the conditional jump at the end of BLOCK.  */
static rtx
lvx_ifcvt_get_reg_cond (basic_block block, rtx *_reg)
{
  rtx_insn *jump = BB_END (block);
  if (!any_condjump_p (jump))
    return NULL_RTX;

  rtx set = pc_set (jump);
  rtx cond = XEXP (SET_SRC (set), 0);
  rtx tested = XEXP (cond, 0);
  if (GET_CODE (tested) == ZERO_EXTRACT)
    tested = XEXP (tested, 0);
  if (REG_P (tested))
    {
      if (_reg)
	*_reg = tested;
      return cond;
    }

  return NULL_RTX;
}

/* Try to recognize an insn with this PATTERN.  If SPLIT is FALSE, reject
   patterns whose ASM template is '#' (pattern must be split).  */
static int
lvx_ifcvt_ce2_recog_pattern (rtx pattern, bool split)
{
  lvx_ifcvt->recog_data = recog_data;
  PATTERN (lvx_ifcvt->recog_insn) = pattern;
  int icode = recog (pattern, lvx_ifcvt->recog_insn, 0);
  if (icode >= 0)
    {
      if (!split)
	{
	  const char *templ = get_insn_template (icode, lvx_ifcvt->recog_insn);
	  if (templ[0] == '#' && templ[1] == '\0')
	    icode = -1;
	}
    }
  PATTERN (lvx_ifcvt->recog_insn) = NULL_RTX;
  recog_data = lvx_ifcvt->recog_data;
  return icode;
}

/* Check that BLOCK only contains valid candidates for CE3 if-conversion:
   insns whose conditional form under REG_COND is a recognisable pattern --
   a CMOVED for a move, the guarded variant of anything predicable.  Reload
   will still reshape some of these (a constant forced into a register, a
   spill), but into loads, stores and moves, which are predicable too.  */
static bool
lvx_ifcvt_ce2_candidate_ce3 (basic_block block, rtx reg_cond)
{
  int index = block->index;
  if ((unsigned) index >= (unsigned) lvx_ifcvt->block_count
      || bitmap_bit_p (lvx_ifcvt->block_visited, index))
    return false;
  bitmap_set_bit (lvx_ifcvt->block_visited, index);

  if (dump_file)
    fprintf (dump_file, "LVX_IFCVT ce2_candidate_ce3 block_%d in %s\n", index,
	     current_function_name ());

  int count = 0;
  rtx_insn *insn = 0;
  FOR_BB_INSNS (block, insn)
    {
      if (NONJUMP_INSN_P (insn))
	{
	  rtx pattern = PATTERN (insn);
	  // A USE is deleted by cond_exec_process_insns, not predicated.
	  if (GET_CODE (pattern) == USE)
	    continue;
	  if (count++ >= MAX_CONDITIONAL_EXECUTE)
	    {
	      if (dump_file)
		fprintf(dump_file, "LVX_IFCVT not candidate (%d > MAX_CONDITIONAL)\n", count);
	      return false;
	    }

	  rtx new_pattern = gen_rtx_COND_EXEC (VOIDmode, reg_cond, pattern);
	  if (lvx_ifcvt_ce2_recog_pattern (new_pattern, true) < 0)
	    {
	      if (dump_file)
		fprintf(dump_file, "LVX_IFCVT not candidate (insn %d has no cond_exec form)\n", INSN_UID (insn));
	      return false;
	    }
	}
      else if (NONDEBUG_INSN_P (insn))
	{
	  if (dump_file && !CALL_P(insn))
	    fprintf(dump_file, "LVX_IFCVT not candidate (jump_insn %d)\n", INSN_UID (insn));
	  return false;
	}
    }

  return true;
}


/* The hard registers INSN reads, into GEN_REGS: what note_uses finds in its
   pattern, plus, for a call, the argument registers of its
   CALL_INSN_FUNCTION_USAGE -- which the pattern does not mention, and which
   is exactly what a value computed just before the call is live for.
   Missing a use here speculates a write over a live register.  */
static void
lvx_ifcvt_record_uses (rtx_insn *insn, HARD_REG_SET *gen_regs)
{
  note_uses (&PATTERN (insn), record_hard_reg_uses, gen_regs);
  if (CALL_P (insn))
    for (rtx link = CALL_INSN_FUNCTION_USAGE (insn); link;
	 link = XEXP (link, 1))
      if (GET_CODE (XEXP (link, 0)) == USE)
	note_uses (&XEXP (link, 0), record_hard_reg_uses, gen_regs);
}

/* Fill USED_REGS the set of registers that are in use at BLOCK boundaries so
   must not be written by a speculated insn of the other block.

   Assume speculating in the THEN BLOCK, with the if-converted ELSE BLOCK
   laying after the if converted THEN BLOCK.  A speculated insn of the THEN
   BLOCK writes its destination unconditionally, so would clobber a live-in
   or live-out register of the ELSE BLOCK.

   In case there are no head or tail sequences, the USED_REGS of the ELSE BLOCK
   is the union of its live-in and live-out registers.  In case BLOCK has a head
   or a tail sequence, the live-in and live-out sets must be respectively
   replaced by the live registers before sequence start and after sequence end.

   Because the ELSE BLOCK is laid out after the THEN BLOCK, the used registers
   of the THEN BLOCK need not include its register live-in / live before the
   sequence start.  This is controlled by WITH_USED_IN. */
static void
lvx_ifcvt_ce3_compute_used_regs (basic_block block, bool with_used_in,
				 HARD_REG_SET *used_regs)
{
  rtx_insn *start_insn = 0;
  if (lvx_ifcvt->then_start && BLOCK_FOR_INSN (lvx_ifcvt->then_start) == block)
    start_insn = lvx_ifcvt->then_start;
  else if (lvx_ifcvt->else_start
	   && BLOCK_FOR_INSN (lvx_ifcvt->else_start) == block)
    start_insn = lvx_ifcvt->else_start;

  rtx_insn *end_insn = 0;
  if (lvx_ifcvt->then_end && BLOCK_FOR_INSN (lvx_ifcvt->then_end) == block)
    end_insn = lvx_ifcvt->then_end;
  else if (lvx_ifcvt->else_end && BLOCK_FOR_INSN (lvx_ifcvt->else_end) == block)
    end_insn = lvx_ifcvt->else_end;

  HARD_REG_SET used_in_regs;
  if (!start_insn)
    REG_SET_TO_HARD_REG_SET (used_in_regs, df_get_live_in (block));

  HARD_REG_SET used_out_regs;
  if (!end_insn)
    REG_SET_TO_HARD_REG_SET (used_out_regs, df_get_live_out (block));

  // Case BLOCK has a head or a tail sequence excluded from if-conversion.
  if (start_insn || end_insn)
    {
      // Prepare to compute live registers at each INSN of BLOCK.
      HARD_REG_SET live_regs, kill_regs, gen_regs;
      REG_SET_TO_HARD_REG_SET (live_regs, df_get_live_out (block));

      rtx_insn *insn = 0;
      FOR_BB_INSNS_REVERSE (block, insn) if (NONDEBUG_INSN_P (insn))
	{
	  // Live after INSN (iterating BLOCK in reverse).
	  if (insn == end_insn)
	    {
	      used_out_regs = live_regs;
	      if (!with_used_in)
		break;
	    }

	  // LIVE := (LIVE - KILL) U GEN
	  CLEAR_HARD_REG_SET (kill_regs);
	  CLEAR_HARD_REG_SET (gen_regs);
	  note_stores (insn, record_hard_reg_sets, &kill_regs);
	  lvx_ifcvt_record_uses (insn, &gen_regs);
	  live_regs &= ~kill_regs;
	  live_regs |= gen_regs;

	  // Live before INSN (iterating BLOCK in reverse).
	  if (insn == start_insn)
	    {
	      used_in_regs = live_regs;
	      break;
	    }
	}
    }

  if (with_used_in)
    *used_regs = used_in_regs | used_out_regs;
  else
    *used_regs = used_out_regs;
}

/* Can INSN run unconditionally?  A guard costs a share of a BCU syllable;
   an insn that writes only registers, cannot trap and has no other effect
   is free to run on the other path as long as what it writes is dead there.
   Returns the registers it sets in *SETS.  */
static bool
lvx_ifcvt_ce3_speculable_p (rtx_insn *insn, HARD_REG_SET *sets)
{
  rtx pattern = PATTERN (insn);
  rtx set = single_set (insn);
  if (!set || !REG_P (SET_DEST (set)))
    return false;
  if (contains_mem_rtx_p (pattern) || side_effects_p (pattern)
      || may_trap_p (pattern))
    return false;
  CLEAR_HARD_REG_SET (*sets);
  note_stores (insn, record_hard_reg_sets, sets);
  return true;
}

/* Speculate the insns of BLOCK whose every written register is dead at the
   block's tail sequence (or live-out) and unused by the if-converted part of
   the other block: flag them with a REG_NONNEG note, which
   lvx_ifcvt_modify_insn reads to leave them unpredicated.  */
static void
lvx_ifcvt_ce3_speculate (ce_if_block *ce_info, basic_block block)
{
  int index = block->index;
  if ((unsigned) index >= (unsigned) lvx_ifcvt->block_count
      || bitmap_bit_p (lvx_ifcvt->block_visited, index))
    return;
  bitmap_set_bit (lvx_ifcvt->block_visited, index);

  basic_block other_block = block == ce_info->then_bb
    ? ce_info->else_bb
    : ce_info->then_bb;
  bool with_used_in = (other_block == ce_info->else_bb);

  // Prepare to compute live registers at each INSN of BLOCK.
  regset live_out = df_get_live_out (block);
  HARD_REG_SET live_regs, kill_regs, gen_regs;
  REG_SET_TO_HARD_REG_SET (live_regs, live_out);

  // Set OTHER_REGS, the registers used by the if-converted part of OTHER_BLOCK.
  HARD_REG_SET other_regs;
  if (!other_block)
    CLEAR_HARD_REG_SET (other_regs);
  else
    lvx_ifcvt_ce3_compute_used_regs (other_block, with_used_in, &other_regs);

  // Get the end of the if-converted sequence of this BLOCK.
  rtx_insn *end_insn = 0;
  if (lvx_ifcvt->then_end && BLOCK_FOR_INSN (lvx_ifcvt->then_end) == block)
    end_insn = lvx_ifcvt->then_end;
  else if (lvx_ifcvt->else_end && BLOCK_FOR_INSN (lvx_ifcvt->else_end) == block)
    end_insn = lvx_ifcvt->else_end;

  // The set of registers live at this BLOCK tail sequence, else BLOCK live-out.
  HARD_REG_SET live_tail_regs;
  if (!end_insn)
    live_tail_regs = live_regs;
  else
    CLEAR_HARD_REG_SET (live_tail_regs);

  rtx_insn *insn = 0;
  FOR_BB_INSNS_REVERSE (block, insn)
    if (NONDEBUG_INSN_P (insn))
      {
	// Live after INSN (iterating BLOCK in reverse).
	if (insn == end_insn)
	  live_tail_regs = live_regs;

	// LIVE := (LIVE - KILL) U GEN
	CLEAR_HARD_REG_SET (kill_regs);
	CLEAR_HARD_REG_SET (gen_regs);
	note_stores (insn, record_hard_reg_sets, &kill_regs);
	lvx_ifcvt_record_uses (insn, &gen_regs);
	live_regs &= ~kill_regs;
	live_regs |= gen_regs;

	HARD_REG_SET sets;
#ifndef __OPTIMIZE__
	if (lvx_aspec_count-- <= 0)
	  continue;
#endif//__OPTIMIZE__
	if (lvx_ifcvt_ce3_speculable_p (insn, &sets)
	    && hard_reg_set_empty_p (sets & live_tail_regs)
	    && hard_reg_set_empty_p (sets & other_regs))
	  {
	    // Use the REG_NONNEG note to flag insn as speculative.
	    add_reg_note (insn, REG_NONNEG, NULL_RTX);
	    if (dump_file)
	      fprintf (dump_file, "LVX_IFCVT speculate (insn %d)\n",
		       INSN_UID (insn));
	  }
      }
}
/* ifcvt utils }}} */

/* Implements IFCVT_MODIFY_TESTS.
   We don't need to modify the tests. However we need to find the boundaries
   of the common prefix and suffix of the then block and else blocks. These are
   available in the scope of IFCVT_MODIFY_TESTS as THEN_START, THEN_END,
   ELSE_START, ELSE_END.  Then we choose the insns to speculate.  */
void
lvx_ifcvt_modify_tests (ce_if_block *ce_info ATTRIBUTE_UNUSED,
			rtx true_expr ATTRIBUTE_UNUSED,
			rtx false_expr ATTRIBUTE_UNUSED,
			rtx_insn *then_start, rtx_insn *then_end,
			rtx_insn *else_start, rtx_insn *else_end)
{
  if (dump_file)
    fprintf (dump_file, "LVX_IFCVT MODIFY_TESTS\n");

  if (then_start && then_end)
    if (dump_file)
      fprintf (dump_file, "\t(then_start_insn %d)\t(then_end_insn %d)\n",
	       INSN_UID (then_start), INSN_UID (then_end));

  if (else_start && else_end)
    if (dump_file)
      fprintf (dump_file, "\t(else_start_insn %d)\t(else_end_insn %d)\n",
	       INSN_UID (else_start), INSN_UID (else_end));

  lvx_ifcvt->then_start = then_start;
  lvx_ifcvt->then_end = then_end;
  lvx_ifcvt->else_start = else_start;
  lvx_ifcvt->else_end = else_end;

  basic_block then_bb = ce_info->then_bb;
  basic_block else_bb = ce_info->else_bb;

  if (then_bb)
    lvx_ifcvt_ce3_speculate (ce_info, then_bb);

  if (else_bb)
    lvx_ifcvt_ce3_speculate (ce_info, else_bb);
}

/* Implements IFCVT_MODIFY_INSN.
   Called from (cond_exec_process_insns), from (cond_exec_process_if_block).
   So pass is CE3 after register allocation. This function is passed PATTERN
   which is a COND_EXEC.  A speculated insn keeps its pattern.  */
rtx
lvx_ifcvt_modify_insn (ce_if_block *ce_info ATTRIBUTE_UNUSED,
		       rtx pattern, rtx_insn ARG_UNUSED (*insn))
{
#ifndef __OPTIMIZE__
  if (lvx_ifcvt_count-- <= 0)
    return 0;
#endif //__OPTIMIZE__
  rtx old_pattern = PATTERN (insn);

  // No changes if the insn was flagged as speculative.
  for (rtx link = REG_NOTES (insn); link; link = XEXP (link, 1))
    if (REG_NOTE_KIND (link) == REG_NONNEG)
      return old_pattern;

  return pattern;
}

/* Implements IFCVT_MACHDEP_INIT.
   Called from (find_if_header), so in CE1, CE2, CE3. In CE1, do nothing as the
   mapping to conditional moves is on-going. In CE2 identified by after_combine
   and not reload_completed, prepare the if-conversion to be applied in CE3.  */
void
lvx_ifcvt_machdep_init (struct ce_if_block *ce_info, bool after_combine)
{

  lvx_ifcvt_ce_level = reload_completed
    ? LVX_IFCVT_CE3 : (after_combine ? LVX_IFCVT_CE2 : LVX_IFCVT_CE1);

  // Nothing else to do in CE1 of if -fno-if-conversion2.
  if (lvx_ifcvt_ce_level < LVX_IFCVT_CE2 || !flag_if_conversion2)
    return;

  // First time in CE2 or CE3, construct lvx_ifcvt else reset.
  if (lvx_ifcvt->ce_level < lvx_ifcvt_ce_level)
    {
      if (lvx_ifcvt->ce_level > LVX_IFCVT_)
	lvx_ifcvt_dtor ();
      lvx_ifcvt_ctor ();
    }
  else
    lvx_ifcvt_reset ();

  // Detect IF-THEN-ELSE-JOIN as in (noce_find_if_block) in ifcvt.cc.
  basic_block test_bb = ce_info->test_bb;
  basic_block then_bb = 0, else_bb = 0, join_bb = 0;
  /* Recognize an IF-THEN-ELSE-JOIN block.  */
  if (single_pred_p (ce_info->then_bb)
      && single_succ_p (ce_info->then_bb)
      && single_pred_p (ce_info->else_bb)
      && single_succ_p (ce_info->else_bb)
      && single_succ (ce_info->then_bb) == single_succ (ce_info->else_bb))
    {
      then_bb = ce_info->then_bb;
      else_bb = ce_info->else_bb;
      join_bb = single_succ (then_bb);
    }
  /* Recognize an IF-THEN-JOIN block.  */
  else if (single_pred_p (ce_info->then_bb)
	   && single_succ_p (ce_info->then_bb)
	   && single_succ (ce_info->then_bb) == ce_info->else_bb)
    {
      then_bb = ce_info->then_bb;
      else_bb = NULL_BLOCK;
      join_bb = ce_info->else_bb;
    }
  /* Recognize an IF-ELSE-JOIN block.  We can have those because the order
     of basic blocks in cfglayout mode does not matter, so the fallthrough
     edge can go to any basic block (and not just to bb->next_bb, like in
     cfgrtl mode).  */
  else if (single_pred_p (ce_info->else_bb)
	   && single_succ_p (ce_info->else_bb)
	   && single_succ (ce_info->else_bb) == ce_info->then_bb)
    {
      then_bb = NULL_BLOCK;
      else_bb = ce_info->else_bb;
      join_bb = ce_info->then_bb;
    }
  else
    /* Not a form we can handle.  */
    return;
  /* The edges of the THEN and ELSE blocks cannot have complex edges.  */
  if (then_bb
      && single_succ_edge (then_bb)->flags & EDGE_COMPLEX)
    return;
  if (else_bb
      && single_succ_edge (else_bb)->flags & EDGE_COMPLEX)
    return;

  if (dump_file)
    {
      fprintf (dump_file, "LVX_IFCVT MACHDEP_INIT(%s, CE%d):\n",
	       current_function_name (), lvx_ifcvt_ce_level);
      fprintf (dump_file, "\ttest-block %d\n", test_bb ? test_bb->index : -1);
      fprintf (dump_file, "\tthen-block %d\n", then_bb ? then_bb->index : -1);
      fprintf (dump_file, "\telse-block %d\n", else_bb ? else_bb->index : -1);
      fprintf (dump_file, "\tjoin-block %d\n", join_bb ? join_bb->index : -1);
    }

  if (lvx_ifcvt_ce_level == LVX_IFCVT_CE2)
    {
      // Check basic block candidates and prepare for conditional execution.  */
      rtx tested_reg = NULL_RTX;
      rtx reg_cond = lvx_ifcvt_get_reg_cond (test_bb, &tested_reg);
      if (!reg_cond)
	return;

      bool candidate = true;

      if (candidate && then_bb)
	candidate &= lvx_ifcvt_ce2_candidate_ce3 (then_bb, reg_cond);

      if (candidate && else_bb)
	candidate &= lvx_ifcvt_ce2_candidate_ce3 (else_bb, reg_cond);

      if (dump_file)
	{
	  if (!candidate)
	    fprintf (dump_file, "LVX_IFCVT not a CE3 candidate.\n");
	  else
	    fprintf (dump_file, "LVX_IFCVT is a CE3 candidate.\n");
	}

      if (!candidate)
	return;

      if (then_bb)
	{
	  // Find the last insertion point in then_bb.
	  rtx_insn *last_insn = BB_END (then_bb);
	  if (JUMP_P (last_insn))
	    last_insn = PREV_INSN (last_insn);

	  // Insert a use of the tested register in then_bb to pull its live range.
	  emit_insn_after (gen_rtx_USE (VOIDmode, tested_reg), last_insn);
	  df_set_bb_dirty (then_bb);
	}

      if (else_bb)
	{
	  // Find the last insertion point in else_bb.
	  rtx_insn *last_insn = BB_END (else_bb);
	  if (JUMP_P (last_insn))
	    last_insn = PREV_INSN (last_insn);

	  // Insert a use of the tested register in else_bb to pull its live range.
	  // Use TESTED_REG as extracted by lvx_ifcvt_get_reg_cond, not
	  // XEXP (reg_cond, 0): on a bit-test branch the latter is the
	  // ZERO_EXTRACT owned by the jump pattern, and re-emitting it here
	  // shares rtl, which verify_rtl_sharing rejects after ce2.
	  emit_insn_after (gen_rtx_USE (VOIDmode, tested_reg), last_insn);
	  df_set_bb_dirty (else_bb);
	}
    }
}

/* Superblock formation for inner loops {{{ */

/* Move BB so that it immediately follows AFTER in the block layout.

   Only valid in cfglayout mode, where the layout *is* the next_bb chain and
   fixup_reorder_chain re-emits the jumps and barriers to match.  This is the
   same splice reorder_loops (hw-doloop.cc) performs, and for the same reason:
   rtl_move_block_after is a stub returning false in both RTL hook tables, so
   move_block_after cannot do it.  */
static void
lvx_layout_move_after (basic_block bb, basic_block after)
{
  gcc_assert (bb != after
	      && bb != ENTRY_BLOCK_PTR_FOR_FN (cfun)
	      && bb != EXIT_BLOCK_PTR_FOR_FN (cfun)
	      && after != EXIT_BLOCK_PTR_FOR_FN (cfun));

  if (bb->prev_bb == after)
    return;

  bb->prev_bb->next_bb = bb->next_bb;
  bb->next_bb->prev_bb = bb->prev_bb;

  bb->prev_bb = after;
  bb->next_bb = after->next_bb;
  after->next_bb->prev_bb = bb;
  after->next_bb = bb;
}

/* True if the layout can make the edge FROM->TO the fallthru without a block
   being inserted: it already is, or TO is FROM's only successor, or FROM ends
   in a conditional jump whose condition reverses -- which is the test
   fixup_reorder_chain itself applies, through invert_jump.  Rotating a loop
   whose header fails this does not shorten anything: fixup_reorder_chain
   inserts a forwarder between the two blocks, which costs a jump and splits
   the body again.  */
static bool
lvx_can_fallthru_p (basic_block from, basic_block to)
{
  edge e = find_edge (from, to);
  rtx_insn *end;
  rtx set;

  if (e == NULL || (e->flags & (EDGE_COMPLEX | EDGE_ABNORMAL)) != 0)
    return false;
  if (e->flags & EDGE_FALLTHRU)
    return true;
  if (single_succ_p (from))
    return true;

  end = BB_END (from);
  if (!JUMP_P (end)
      || !any_condjump_p (end)
      || extract_asm_operands (PATTERN (end)) != NULL)
    return false;
  set = pc_set (end);
  if (set == NULL_RTX)
    return false;
  return reversed_comparison_code (XEXP (SET_SRC (set), 0), end) != UNKNOWN;
}

/* Lay the body of innermost LOOP out as one run, the header first and
   immediately after the loop's single entry block, with the run's last
   block's out-of-loop successor straight after it when that is a local move.
   Returns true if the layout changed.

   The body keeps the interior order bb-reorder gave it and is merely rotated
   so the header leads: that changes exactly one adjacency, and it is what
   measures as a gain.  A greedy re-order of the same blocks measures as a
   loss -- see docs/ebb-selection.md.  */
static bool
lvx_superblock_layout_loop (class loop *loop)
{
  unsigned nblk = loop->num_nodes;
  basic_block header = loop->header;
  basic_block *body, *ord, *extra, pre = NULL, succ = NULL, prev, bb;
  unsigned nentry = 0, nextra = 0, hpos = 0, i, j;
  int lo = INT_MAX, hi = -1, n = 0;
  bool changed = false;
  edge e;
  edge_iterator ei;
  int *index = XCNEWVEC (int, last_basic_block_for_fn (cfun));
  auto_bitmap inloop;
  auto_bitmap placed;

  FOR_EACH_BB_FN (bb, cfun)
    index[bb->index] = n++;

  FOR_EACH_EDGE (e, ei, header->preds)
    if (!flow_bb_inside_loop_p (loop, e->src))
      {
	pre = e->src;
	nentry++;
      }

  /* One entry block, able to fall through to the header.  Phase A has made
     that so wherever it could; where it could not, leave the loop alone.  */
  if (nentry != 1
      || pre == ENTRY_BLOCK_PTR_FOR_FN (cfun)
      || !single_succ_p (pre)
      || (single_succ_edge (pre)->flags & EDGE_COMPLEX) != 0)
    {
      free (index);
      return false;
    }

  body = get_loop_body (loop);
  for (i = 0; i < nblk; i++)
    {
      bitmap_set_bit (inloop, body[i]->index);
      lo = MIN (lo, index[body[i]->index]);
      hi = MAX (hi, index[body[i]->index]);
    }

  /* The order to lay the body out in.  Prefer the loop's own chain -- walk
     from the header along the single in-loop successor of each block -- which
     is what EBB formation and the pipeliner both want, and which is the only
     order that makes sense for a body the unfold phase has just duplicated,
     since the copy's blocks sit wherever the duplicator left them.  Fall back
     to the layout order rotated so the header leads, for a body that is not a
     single cycle; that fallback is the one the corpus measurement validated,
     and a greedy re-order measures worse than either.  */
  ord = XNEWVEC (basic_block, nblk);
  {
    basic_block walk = header;
    bool chain_ok = true;
    auto_bitmap seen;

    for (i = 0; i < nblk && chain_ok; i++)
      {
	basic_block nxt = NULL;
	edge se;
	edge_iterator sei;

	ord[i] = walk;
	bitmap_set_bit (seen, walk->index);
	FOR_EACH_EDGE (se, sei, walk->succs)
	  if (bitmap_bit_p (inloop, se->dest->index))
	    {
	      if (nxt != NULL)
		chain_ok = false;	/* two in-loop successors */
	      nxt = se->dest;
	    }
	if (nxt == NULL)
	  chain_ok = false;
	else if (i + 1 < nblk && bitmap_bit_p (seen, nxt->index))
	  chain_ok = false;		/* closed early */
	else if (i + 1 == nblk && nxt != header)
	  chain_ok = false;		/* did not close on the header */
	walk = nxt;
      }

    if (!chain_ok)
      {
	for (i = 0; i < nblk; i++)
	  ord[i] = body[i];
	for (i = 0; i < nblk; i++)
	  for (j = i + 1; j < nblk; j++)
	    if (index[ord[j]->index] < index[ord[i]->index])
	      std::swap (ord[i], ord[j]);
	for (i = 0; i < nblk; i++)
	  if (ord[i] == header)
	    hpos = i;
	if (hpos != 0)
	  {
	    basic_block *tmp = XNEWVEC (basic_block, nblk);
	    for (i = 0; i < nblk; i++)
	      tmp[i] = ord[(hpos + i) % nblk];
	    for (i = 0; i < nblk; i++)
	      ord[i] = tmp[i];
	    free (tmp);
	  }
      }
  }

  /* Blocks sitting inside the body's layout span without being in the loop.
     The gather displaces them, and they go straight after the run.  */
  extra = XNEWVEC (basic_block, MAX (hi - lo + 1, 1));
  FOR_EACH_BB_FN (bb, cfun)
    if (index[bb->index] > lo && index[bb->index] < hi
	&& !bitmap_bit_p (inloop, bb->index))
      extra[nextra++] = bb;

  /* Refuse the whole loop unless every adjacency the new order asks for can
     actually be a fallthru.  Otherwise fixup_reorder_chain inserts a
     forwarder, which costs a jump and leaves the body split anyway.  */
  prev = pre;
  for (i = 0; i < nblk; i++)
    {
      if (ord[i]->prev_bb != prev && !lvx_can_fallthru_p (prev, ord[i]))
	{
	  if (dump_file)
	    fprintf (dump_file, ";; lvx superblock: loop %d declined, bb %d"
		     " cannot fall through to bb %d\n",
		     loop->num, prev->index, ord[i]->index);
	  free (extra);
	  free (ord);
	  free (body);
	  free (index);
	  return false;
	}
      prev = ord[i];
    }

  /* Install the run behind the entry block.  */
  prev = pre;
  for (i = 0; i < nblk; i++)
    {
      if (ord[i]->prev_bb != prev)
	{
	  lvx_layout_move_after (ord[i], prev);
	  changed = true;
	}
      bitmap_set_bit (placed, ord[i]->index);
      prev = ord[i];
    }

  /* The pipeliner's loop selection requires the block after the run to be a
     successor of the run's last block, and that block's other successor is
     now the back edge, so it has to be the out-of-loop one.  Move it only
     when it has no other predecessor, so that nothing else loses a fallthru
     and no jump is added anywhere.  */
  FOR_EACH_EDGE (e, ei, prev->succs)
    if (!bitmap_bit_p (inloop, e->dest->index))
      succ = e->dest;

  if (succ != NULL
      && succ != EXIT_BLOCK_PTR_FOR_FN (cfun)
      && single_pred_p (succ)
      && succ->prev_bb != prev)
    {
      lvx_layout_move_after (succ, prev);
      bitmap_set_bit (placed, succ->index);
      changed = true;
      prev = succ;
    }

  for (j = 0; j < nextra; j++)
    {
      if (bitmap_bit_p (placed, extra[j]->index))
	continue;
      if (extra[j]->prev_bb != prev)
	{
	  lvx_layout_move_after (extra[j], prev);
	  changed = true;
	}
      bitmap_set_bit (placed, extra[j]->index);
      prev = extra[j];
    }

  if (changed && dump_file)
    fprintf (dump_file, ";; lvx superblock: loop %d, %u blocks, header %d"
	     " after entry %d%s\n", loop->num, nblk, header->index,
	     pre->index, nextra ? ", span had foreign blocks" : "");

  free (extra);
  free (ord);
  free (body);
  free (index);
  return changed;
}

/* True if INSN is a doloop_end: the decrement-test-branch that reorg_loops
   will consume into a LOOPDO, leaving only a `# loopdo end' comment behind.  */
static bool
lvx_doloop_end_p (rtx_insn *insn)
{
  return (JUMP_P (insn)
	  && condjump_in_parallel_p (insn)
	  && doloop_condition_get (insn) != NULL_RTX);
}

/* True if LOOP is already a hardware loop, which reorg_loops will put a
   LOOPDO on.  Unfolding one of those would mean halving the trip count and
   peeling the odd iteration; a loop without one needs neither, because the
   exit tests a superblock body already carries in every block serve both
   copies unchanged.  */
static bool
lvx_loop_has_doloop_p (class loop *loop)
{
  basic_block *body;
  bool found = false;

  if (loop->latch == NULL)
    return false;

  body = get_loop_body (loop);
  for (unsigned i = 0; i < loop->num_nodes && !found; i++)
    if (lvx_doloop_end_p (BB_END (body[i])))
      found = true;
  free (body);
  return found;
}

/* Whether an insn reads or writes memory, and whether two may alias.  These
   mirror the static helpers of ddg.cc, which is also where the memory
   dependence shape below comes from.  */

static bool lvx_mem_ref_seen;

static void
lvx_mark_mem_use (rtx *x, void *)
{
  subrtx_iterator::array_type array;
  FOR_EACH_SUBRTX (iter, array, *x, NONCONST)
    if (MEM_P (*iter))
      {
	lvx_mem_ref_seen = true;
	break;
      }
}

static void
lvx_mark_mem_store (rtx loc, const_rtx, void *)
{
  if (MEM_P (loc))
    lvx_mem_ref_seen = true;
}

static bool
lvx_mem_read_insn_p (rtx_insn *insn)
{
  lvx_mem_ref_seen = false;
  note_uses (&PATTERN (insn), lvx_mark_mem_use, NULL);
  return lvx_mem_ref_seen;
}

static bool
lvx_mem_write_insn_p (rtx_insn *insn)
{
  lvx_mem_ref_seen = false;
  note_stores (insn, lvx_mark_mem_store, NULL);
  return lvx_mem_ref_seen;
}

static bool
lvx_insns_may_alias_p (rtx_insn *insn1, rtx_insn *insn2)
{
  subrtx_iterator::array_type array1;
  subrtx_iterator::array_type array2;

  FOR_EACH_SUBRTX (iter1, array1, PATTERN (insn1), NONCONST)
    {
      const_rtx x1 = *iter1;
      if (MEM_P (x1))
	FOR_EACH_SUBRTX (iter2, array2, PATTERN (insn2), NONCONST)
	  {
	    const_rtx x2 = *iter2;
	    if (MEM_P (x2) && may_alias_p (x2, x1))
	      return true;
	  }
    }
  return false;
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
   three times over -- lvx_cms_build_graph skips USE and CLOBBER patterns, skips
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

/* How much longer -mcms-load-latency says this instruction's result takes than
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

static int
lvx_cms_load_latency_delta (rtx_insn *insn)
{
  if (lvx_cms_load_latency == 0 || recog_memoized (insn) < 0)
    return 0;
  if (!lvx_load_class_p (get_attr_type (insn)))
    return 0;
  int have = insn_default_latency (insn);
  return (int) lvx_cms_load_latency > have ? (int) lvx_cms_load_latency - have : 0;
}

static int
lvx_arc_latency (rtx_insn *prod, rtx_insn *cons, enum reg_note dep_type)
{
  int cost;

  if (recog_memoized (prod) < 0 || recog_memoized (cons) < 0)
    return 0;

  cost = insn_default_latency (prod);
  if (cost < 0)
    cost = 0;

  if (dep_type == REG_DEP_ANTI)
    cost = 0;
  else if (dep_type == REG_DEP_OUTPUT)
    {
      cost = insn_default_latency (prod) - insn_default_latency (cons);
      if (cost <= 0)
	cost = 1;
    }
  /* bypass_p and insn_latency go through dfa_insn_code, whose dfa_insn_codes
     array is allocated by dfa_start and freed by dfa_finish.  The caller
     brackets the analysis with those, so the queries are legal here; without
     the bracket they corrupt the heap, which is how newlib's
     __call_atexit.c once aborted inside xrealloc.  They are worth the
     bracket: dropping them weakened the latencies enough to turn
     dijkstra_small's loops from declined back to accepted.  */
  else if (bypass_p (prod))
    cost = insn_latency (prod, cons);

  if (targetm.sched.adjust_cost)
    cost = targetm.sched.adjust_cost (cons, (int) dep_type, prod, cost, 0);

  /* The uncached-load study, added after the fact and as a delta so the
     bypasses above keep their effect.  Only a flow dependence carries the
     producer's result, so an anti or output arc is unaffected.  */
  if (dep_type == REG_DEP_TRUE)
    cost += lvx_cms_load_latency_delta (prod);

  return cost < 0 ? 0 : cost;
}

/* True if innermost LOOP has the superblock shape the renaming below assumes:
   one entry, one back edge, every body block ending in a two-way branch with
   exactly one successor inside the loop, no call, no volatile reference, no
   asm, and within the size limits.  This is kvx-unfold.cc's unfold_inner_find
   reduced to its structural checks, and it is not optional: on KVX the
   renaming only ever runs on loops that passed that gate, and it is what makes
   the renaming code's assumptions true.  MAX_BLOCKS and MAX_INSNS are passed
   in because an unfolded body is twice the size of the one the unfold phase
   accepted.  */
static bool
lvx_inner_superblock_loop_p (class loop *loop, unsigned max_blocks,
			     unsigned max_insns)
{
  basic_block *body;
  basic_block header = loop->header;
  unsigned nentry = 0, insns = 0, back_edges = 0;
  bool ok = true;
  edge e;
  edge_iterator ei;
  auto_bitmap inloop;

  if (loop->latch == NULL || loop->num_nodes > max_blocks)
    return false;

  FOR_EACH_EDGE (e, ei, header->preds)
    if (!flow_bb_inside_loop_p (loop, e->src))
      nentry++;
  if (nentry != 1)
    return false;

  body = get_loop_body (loop);
  for (unsigned i = 0; i < loop->num_nodes; i++)
    bitmap_set_bit (inloop, body[i]->index);

  for (unsigned i = 0; i < loop->num_nodes && ok; i++)
    {
      basic_block bb = body[i];
      rtx_insn *end, *insn;
      edge e0, e1;

      if (EDGE_COUNT (bb->succs) != 2
	  || (bb->flags & BB_DISABLE_SCHEDULE) != 0
	  || computed_jump_p (BB_END (bb)))
	{
	  ok = false;
	  break;
	}

      e0 = EDGE_SUCC (bb, 0);
      e1 = EDGE_SUCC (bb, 1);
      if ((e0->flags & EDGE_COMPLEX) || (e1->flags & EDGE_COMPLEX))
	{
	  ok = false;
	  break;
	}
      if (e0->dest == header)
	back_edges++;
      if (e1->dest == header)
	back_edges++;
      /* Exactly one successor inside the loop: the body is a chain.  */
      if (bitmap_bit_p (inloop, e0->dest->index)
	  == bitmap_bit_p (inloop, e1->dest->index))
	{
	  ok = false;
	  break;
	}

      end = BB_END (bb);
      if (JUMP_P (end) && extract_asm_operands (PATTERN (end)) != NULL)
	{
	  ok = false;
	  break;
	}

      for (insn = BB_HEAD (bb); insn != NEXT_INSN (end);
	   insn = NEXT_INSN (insn))
	if (NONDEBUG_INSN_P (insn))
	  {
	    if (++insns > max_insns
		|| CALL_P (insn)
		|| volatile_refs_p (PATTERN (insn))
		|| asm_noperands (PATTERN (insn)) >= 0)
	      {
		ok = false;
		break;
	      }
	  }
    }

  free (body);
  return ok && back_edges == 1;
}

/* Decompose MEM's address into a base register and a constant byte offset.
   False when it is not of that shape.  */

static bool
lvx_mem_base_offset (const_rtx mem, rtx *base, HOST_WIDE_INT *off)
{
  rtx addr = XEXP (mem, 0);

  *off = 0;
  if (GET_CODE (addr) == PLUS && CONST_INT_P (XEXP (addr, 1)))
    {
      *off = INTVAL (XEXP (addr, 1));
      addr = XEXP (addr, 0);
    }
  if (!REG_P (addr))
    return false;
  *base = addr;
  return true;
}

/* True when every memory reference of SEQ[P] is provably disjoint from every
   one of SEQ[Q], by base register and access width.  may_alias_p is
   conservative; this is where a pair that shares a base register and whose
   byte ranges do not overlap is separated, which is the common shape in an
   unrolled or strided body.

   Only sound within one iteration: the base is an induction variable, so
   across the back edge its value differs by the step and the offsets are not
   comparable.  The caller uses it for distance-zero arcs only.  A base
   redefined anywhere between the two references, or by either of them,
   breaks the comparison too, so that is rejected.  */

static bool
lvx_mems_disjoint_p (const vec<rtx_insn *> &seq, unsigned p, unsigned q)
{
  subrtx_iterator::array_type a1, a2;
  rtx_insn *insn1 = seq[p], *insn2 = seq[q];
  bool any = false;

  FOR_EACH_SUBRTX (i1, a1, PATTERN (insn1), NONCONST)
    {
      const_rtx m1 = *i1;
      rtx b1;
      HOST_WIDE_INT o1;

      if (!MEM_P (m1))
	continue;
      if (!lvx_mem_base_offset (m1, &b1, &o1))
	return false;

      FOR_EACH_SUBRTX (i2, a2, PATTERN (insn2), NONCONST)
	{
	  const_rtx m2 = *i2;
	  rtx b2;
	  HOST_WIDE_INT o2;
	  poly_int64 s1, s2;

	  if (!MEM_P (m2))
	    continue;
	  if (!lvx_mem_base_offset (m2, &b2, &o2)
	      || REGNO (b1) != REGNO (b2))
	    return false;

	  s1 = GET_MODE_SIZE (GET_MODE (m1));
	  s2 = GET_MODE_SIZE (GET_MODE (m2));
	  if (!s1.is_constant () || !s2.is_constant ())
	    return false;
	  /* Overlapping byte ranges: nothing proved.  */
	  if (o1 < o2 + s2.to_constant () && o2 < o1 + s1.to_constant ())
	    return false;

	  /* The base must hold the same value at both references.  */
	  for (unsigned k = MIN (p, q); k <= MAX (p, q); k++)
	    {
	      df_ref def;
	      FOR_EACH_INSN_DEF (def, seq[k])
		if (DF_REF_REGNO (def) == REGNO (b1))
		  return false;
	    }
	  any = true;
	}
    }
  return any;
}

/* Whether INSN has an operand tied to another by a matching constraint, in
   which case renaming cannot separate its read from its write and the
   anti-dependence between them is real.  */

/* True if INSN's destination is EARLYCLOBBER in the alternative that was
   chosen: the output is written before the inputs are read, so it must not
   share a register with an input.

   That is a statement about WHEN THE RANGE BEGINS, not about whether the value
   may be renamed (§7ter.2a).  A normal definition's range may start one cycle
   after the def, because the write lands after the reads and sharing an input's
   register is legal; an earlyclobber one may not.  Feeding this into the
   interference test is all the "must differ from the inputs" rule needs -- no
   special case in the allocator.  */

static bool
lvx_insn_dest_earlyclobber_p (rtx_insn *insn)
{
  if (recog_memoized (insn) < 0)
    return false;
  extract_insn (insn);
  preprocess_constraints (insn);
  if (which_alternative < 0)
    return false;

  const operand_alternative *alt
    = &recog_op_alt[which_alternative * recog_data.n_operands];
  for (int i = 0; i < recog_data.n_operands; i++)
    if (recog_data.operand_type[i] != OP_IN && alt[i].earlyclobber)
      return true;
  return false;
}

static bool
lvx_insn_has_tied_operand_p (rtx_insn *insn)
{
  if (recog_memoized (insn) < 0)
    return false;
  extract_insn (insn);
  preprocess_constraints (insn);
  if (which_alternative < 0)
    return false;

  const operand_alternative *alt
    = &recog_op_alt[which_alternative * recog_data.n_operands];
  for (int i = 0; i < recog_data.n_operands; i++)
    if (alt[i].matches >= 0 || alt[i].matched >= 0)
      return true;
  return false;
}

/* True if INSN may be hoisted above a conditional branch -- control
   speculation.  Read off the LVX description rather than guessed:

   - Integer arithmetic, logical, shift, compare, move and conditional move are
     unconditionally safe, and so are the dividers: divmod* returns zero on a
     zero divisor rather than trapping, which is why -mdivmod0-trap exists as a
     separate check.  Of 1741 lvx_2 opcodes only 273 carry a THROW in their
     Behavior and 210 of those are the atomics.
   - A load only in its dismissible variant, `.s' or `.us' -- Modifier.yml
     marks both Dismissible.  THROW is NOT the test here: a plain load carries
     none and can still fault on an unmapped page, which is what the
     dismissible variants exist for.  `.u' is uncached but not dismissible.
   - A store never: the write is the side effect.
   - Floating point only when nothing observes the exception flags it writes
     into $cs, so only under !flag_trapping_math.
   - An atomic, a CSR or TLB access, a call, an asm or anything volatile
     never.  */

static bool
lvx_fp_insn_p (rtx_insn *insn)
{
  subrtx_iterator::array_type array;
  rtx pat = PATTERN (insn);
  FOR_EACH_SUBRTX (iter, array, pat, NONCONST)
    if (FLOAT_MODE_P (GET_MODE (*iter)))
      return true;
  return false;
}

static bool
lvx_speculatable_p (rtx_insn *insn)
{
  if (!NONDEBUG_INSN_P (insn) || CALL_P (insn) || JUMP_P (insn))
    return false;
  if (volatile_insn_p (PATTERN (insn))
      || asm_noperands (PATTERN (insn)) >= 0)
    return false;
  if (lvx_mem_write_insn_p (insn))
    return false;
  if (lvx_mem_read_insn_p (insn))
    {
      enum lvx_variant v = lvx_insn_variant (insn);
      if (v != LVX_VARIANT_SPECULATE && v != LVX_VARIANT_PRELOAD)
	return false;
    }
  if (flag_trapping_math && lvx_fp_insn_p (insn))
    return false;
  return true;
}

/* One arc of the CMS dependence graph: a dependence from SRC to DST with
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

enum lvx_cms_arc_kind
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

static const char *const lvx_cms_arc_kind_name[] = {
  "reg-raw", "reg-war", "reg-waw", "mem-intra", "mem-cross",
  "ctrl-order", "ctrl-store", "ctrl-liveout", "ctrl-backedge"
};

struct lvx_cms_arc
{
  unsigned src;
  unsigned dst;
  HOST_WIDE_INT theta;
  HOST_WIDE_INT omega;
  enum lvx_cms_arc_kind kind;
};

/* A memory reference's address as an affine function of the iteration number:

       address (k) = SYM + OFF + STEP * k

   over SIZE bytes.  SYM is the symbolic part, compared between two references
   by rtx_equal_p; OFF folds the MEM's own displacement together with any
   constant the recognition folds in from the increments preceding it, so that
   two references off one induction variable differ by a known number of
   bytes.  */

struct lvx_mem_iv
{
  rtx sym;
  HOST_WIDE_INT off;
  HOST_WIDE_INT step;
  HOST_WIDE_INT size;
  bool valid;
};

/* True if INSN has a definition of REGNO.  PARTIAL, when non-null, is set when
   that definition writes only part of the register -- a subreg destination, or
   one half of a register tuple -- which is NOT a full definition of the value:
   the untouched part still comes from whatever defined it earlier.

   The reaching-definition walk below needs that distinction and does not model
   it.  Taking the last definition before a use and stopping is right only for a
   FULL write; after a partial one, the use also depends on whichever definition
   supplied the rest, and an arc from the partial write alone omits a true
   dependence -- the unsound direction.

   No such write currently reaches machine_reorg.  Measured on PolyBench
   vectorized for lvx-2, and on 256-bit quads and __int128 pairs written
   deliberately: ten `set (subreg ...)' insns exist at pass_sms, which is why
   SMS refuses those loops outright ("subreg in lhs"), and zero survive register
   allocation.  So this is a latent hazard rather than a live bug, and the
   caller declines the loop rather than modelling it -- a visible refusal in
   place of a silent omission, costing nothing while the count is zero.  */

static bool
lvx_insn_defs_regno_p (rtx_insn *insn, unsigned regno, bool *partial = NULL)
{
  df_ref def;

  FOR_EACH_INSN_DEF (def, insn)
    if (DF_REF_REGNO (def) == regno)
      {
	if (partial != NULL && (DF_REF_FLAGS (def) & DF_REF_PARTIAL) != 0)
	  *partial = true;
	return true;
      }
  return false;
}

/* Peel a trailing constant off X.  */

static rtx
lvx_split_plus_const (rtx x, HOST_WIDE_INT *c)
{
  *c = 0;
  while (GET_CODE (x) == PLUS && CONST_INT_P (XEXP (x, 1)))
    {
      *c += INTVAL (XEXP (x, 1));
      x = XEXP (x, 0);
    }
  return x;
}

/* The upper bound on software-pipeline overlap that inductive relaxation is
   allowed to assume, the paper's `o'.  It bounds how far a displacement may be
   adjusted, so it must not exceed what codegen can actually produce.  */
#define LVX_CMS_MAX_OVERLAP 8

/* Signed 10 bits: the displacement an LVX base+displacement operand holds
   without an IMMX extension syllable (format signed10, widened to signed37 by
   .X and to signed64 by .Y -- see lvx_r_s10_s37_s64_operand).  Relaxation must
   not push a displacement out of the field it already occupies, or it changes
   the instruction's encoding and its cost.  */

static inline bool
lvx_cms_disp_fits_p (HOST_WIDE_INT d)
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
lvx_cms_find_simple_inductions (const auto_vec<rtx_insn *> &seq,
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
      if (d == 0 || !lvx_cms_disp_fits_p (d))
	continue;

      delta[r] = d;
      bitmap_set_bit (inductions, r);
    }
}

/* True if INSN uses REGNO as the base of a base+displacement memory operand
   whose displacement can still absorb an adjustment of up to
   LVX_CMS_MAX_OVERLAP steps of DELTA -- the paper's `displacement +- o.delta
   still fits in the operand'.  Such a use is RELAXED: it may read any instance
   of the induction, so it needs no dependence on the one definition.  */

static bool
lvx_cms_relaxed_base_use_p (rtx_insn *insn, unsigned regno,
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

  HOST_WIDE_INT reach = LVX_CMS_MAX_OVERLAP * (delta > 0 ? delta : -delta);
  return (lvx_cms_disp_fits_p (disp + reach)
	  && lvx_cms_disp_fits_p (disp - reach));
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
lvx_cms_collect_mem_ivs (class loop *, const auto_vec<rtx_insn *> &seq,
			 auto_vec<lvx_mem_iv> &ivs)
{
  unsigned n = seq.length ();

  ivs.safe_grow_cleared (n);
  if (!lvx_cms_stride_test)
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
lvx_cms_mem_independent_p (const struct lvx_mem_iv &a,
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

/* True if INSN's writes read their own destination, so that an output or anti
   dependence on it is real and register renaming cannot remove it.

   Two cases, and the attribute is not one of them.  A COND_EXEC write may not
   happen, so the destination's previous value survives it: the write is a
   read-modify-write, and these are what if-conversion produces, which is most
   of them.  A tied input-output names the destination as a source outright.

   `get_attr_guarded' does NOT answer this.  It defaults to `no' and is set on
   exactly two hand-written patterns in control.md, so testing it kept output
   dependences for those two and dropped them for every COND_EXEC CE3 made --
   precisely backwards.  */

static bool
lvx_insn_def_reads_dest_p (rtx_insn *insn)
{
  return (GET_CODE (PATTERN (insn)) == COND_EXEC
	  || lvx_insn_has_tied_operand_p (insn));
}

static inline void
lvx_cms_push_arc (auto_vec<lvx_cms_arc> &arcs, unsigned src, unsigned dst,
		  HOST_WIDE_INT theta, HOST_WIDE_INT omega,
		  enum lvx_cms_arc_kind kind)
{
  lvx_cms_arc a = { src, dst, theta, omega, kind };
  arcs.safe_push (a);
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
lvx_cms_speculated_load_p (rtx_insn *insn, bitmap bodydef)
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

/* Promote the loop-variant-address loads of SEQ to their dismissible variant.

   This is not an optimisation: the dependence graph's branch constraint (1)
   already lets a load cross a conditional branch, so a schedule built on that
   graph is legal ONLY if every such load cannot fault.  `.s' keeps the cached
   latency -- it is Dismissible, not uncached -- so promoting changes the
   emitted mnemonic and nothing the scheduler reasoned about.  Returns the
   count.  */

static int
lvx_cms_force_speculative_loads (const auto_vec<rtx_insn *> &seq,
				 bitmap bodydef)
{
  int promoted = 0;

  for (unsigned i = 0; i < seq.length (); i++)
    {
      if (!lvx_cms_speculated_load_p (seq[i], bodydef))
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
  return promoted;
}

/* The furthest iteration distance the scheduler keeps copies for.  Beyond it an
   arc is slack by arithmetic; see lvx_cms_delta.  */
#define LVX_CMS_MAX_DELTA 8

/* Delta: the largest iteration distance any arc spans that can bind.  This is what replaces
   kvx-unfold.cc's `max_distance = 1; // FIXME' -- that literal was the two-copy
   structure asserting nothing spans more than one instance, which is an
   artefact of having two physical copies rather than anything CMS requires.
   LVX's graph carries omega explicitly, so Delta is simply measured.  */

static int
lvx_cms_delta (const auto_vec<lvx_cms_arc> &arcs)
{
  int delta = 1;
  for (unsigned i = 0; i < arcs.length (); i++)
    {
      HOST_WIDE_INT w = arcs[i].omega < 0 ? -arcs[i].omega : arcs[i].omega;
      if (w > LVX_CMS_MAX_DELTA)
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

static void
lvx_cms_resources_add (struct lvx_sched_resources *resources, rtx_insn *insn)
{
  if (!lvx_doloop_end_p (insn))
    {
      lvx_sched_resources_add (resources, insn);
      return;
    }

  const struct lvx_issue_resources *dec = &lvx_issue_table[ISSUE_TINY];
  const struct lvx_issue_resources *br = &lvx_issue_table[ISSUE_BCU_BRRP];

  resources->insn_count++;
  resources->ninsns_count += dec->ninsns + br->ninsns;
#define LVX_CMS_DOLOOP_ADD(R) resources->R##_count += dec->R + br->R;
  LVX_ISSUE_RESOURCES (LVX_CMS_DOLOOP_ADD)
#undef LVX_CMS_DOLOOP_ADD
}

/* The issue counters by name and index, so a blame can say WHICH resource
   refused a task.  The index past the last counter stands for the issue rate,
   which is a bound on the bundle rather than a counter in Resource.yml.  */

static const char *const lvx_cms_resource_name[] = {
#define LVX_CMS_RESOURCE_NAME(R) #R,
  LVX_ISSUE_RESOURCES (LVX_CMS_RESOURCE_NAME)
#undef LVX_CMS_RESOURCE_NAME
  "issue"
};

#define LVX_CMS_NRESOURCES   ((int) (sizeof (lvx_cms_resource_name) / sizeof (lvx_cms_resource_name[0])))

/* Which counter refuses INSN in SLOT, as an index into
   lvx_cms_resource_name, or -1 if it fits.  The first that overflows is
   reported: a task displaced from a cycle is displaced by one thing, and
   attributing it to the first is enough to aim the next construction.  */

static int
lvx_cms_blocking_resource (const struct lvx_sched_resources *slot,
			   rtx_insn *insn)
{
  struct lvx_sched_resources trial = *slot;
  int idx = 0;

  lvx_cms_resources_add (&trial, insn);

#define LVX_CMS_BLOCK(R)						\
  {									\
    if (trial.R##_count > (unsigned) lvx_issue_availability.R)		\
      return idx;							\
    idx++;								\
  }
  LVX_ISSUE_RESOURCES (LVX_CMS_BLOCK)
#undef LVX_CMS_BLOCK

  if (trial.ninsns_count > (unsigned) lvx_sched_issue_rate ())
    return idx;
  return -1;
}

/* Whether INSN reserves anything of the counter at index WANT.  Used to find
   the tasks holding the resource that displaced another -- the culprits a
   release date should move out of the way.  */

static bool
lvx_cms_insn_uses_resource_p (rtx_insn *insn, int want)
{
  struct lvx_sched_resources d;
  int idx = 0;

  memset (&d, 0, sizeof (d));
  lvx_cms_resources_add (&d, insn);

#define LVX_CMS_USES(R)							\
  {									\
    if (idx == want)							\
      return d.R##_count > 0;						\
    idx++;								\
  }
  LVX_ISSUE_RESOURCES (LVX_CMS_USES)
#undef LVX_CMS_USES

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

#define LVX_CMS_IG_MAX_NODES 512
#define LVX_CMS_IG_MAX_PERIOD 1024

/* Register classes, by how many consecutive registers a value occupies.  */
enum lvx_cms_rclass { LVX_RC_SINGLE = 0, LVX_RC_PAIR, LVX_RC_QUAD, LVX_RC_N };

static const int lvx_cms_rclass_size[LVX_RC_N] = { 64, 32, 16 };

/* worst_1 (N, C): the most names of class N that one neighbour of class C can
   deny.  Nine numbers, and they are this simple only because LVX's tuples are
   ALIGNED, which makes the alias relation a laminar family -- a single belongs
   to exactly one pair and one quad, never to two overlapping ones.  Smith's
   motivating case has worst_1 (double, single) = 2 as well, which needs
   unaligned doubles; here every reverse direction is 1.  And because aligned
   classes partition rather than overlap,

       worst_m (N, C) = min (m * worst_1 (N, C), |N|)

   so no search over subsets is needed.  */
static const int lvx_cms_worst1[LVX_RC_N][LVX_RC_N] = {
  /*        C=SINGLE  PAIR  QUAD */
  /* N=SINGLE */ { 1,   2,   4 },
  /* N=PAIR   */ { 1,   1,   2 },
  /* N=QUAD   */ { 1,   1,   1 },
};

static enum lvx_cms_rclass
lvx_cms_rclass_of (int nregs)
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
lvx_cms_squeeze (enum lvx_cms_rclass n, const int deg[LVX_RC_N])
{
  int sq = 0;
  for (int c = 0; c < LVX_RC_N; c++)
    {
      int w = deg[c] * lvx_cms_worst1[n][c];
      int cap = lvx_cms_rclass_size[n];
      sq += MIN (w, cap);
    }
  return MIN (sq, lvx_cms_rclass_size[n]);
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
lvx_cms_stage_schedule (const auto_vec<lvx_cms_arc> &arcs, unsigned n,
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
lvx_cms_repair_fits_ii_p (const auto_vec<rtx_insn *> &seq,
			    const auto_vec<lvx_cms_arc> &arcs,
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
lvx_cms_lineage_heights (const auto_vec<lvx_cms_arc> &arcs, unsigned n,
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
lvx_cms_lineages (const auto_vec<lvx_cms_arc> &arcs, unsigned n,
		  const auto_vec<bool> &defines,
		  auto_vec<int> &lin, int *nlin_out, int *nseq_out)
{
  auto_vec<int> ht;
  lvx_cms_lineage_heights (arcs, n, ht);

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
lvx_cms_sigma_min (const auto_vec<int> &sigma)
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
static int
lvx_cms_sigma_span (const auto_vec<int> &sigma)
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
lvx_cms_collect_values (const auto_vec<rtx_insn *> &seq,
			const auto_vec<lvx_cms_arc> &arcs,
			const auto_vec<int> &sigma, int ii,
			auto_vec<int> &def_, auto_vec<int> &last_,
			auto_vec<int> &deg, auto_vec<int> &nregs_,
			auto_vec<int> &ec_, auto_vec<int> *owner = NULL)
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
      ec_.safe_push (lvx_insn_dest_earlyclobber_p (seq[i]) ? 1 : 0);
      if (owner)
	owner->safe_push ((int) i);
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
lvx_cms_colour (const auto_vec<int> &def_, const auto_vec<int> &last_,
		const auto_vec<int> &deg, const auto_vec<int> &nregs_,
		const auto_vec<int> &ec_, int ii, int lo0,
		int *nn_out, int *ne_out, int *msq_out, int *spl_out,
		int *per_out, int *kun_out, auto_vec<int> *stuck = NULL,
		auto_vec<int> *anode_value = NULL,
		auto_vec<int> *anode_name = NULL,
		auto_vec<int> *anode_regno = NULL,
		int *maxreg_out = NULL,
		unsigned HOST_WIDE_INT avail = ~(unsigned HOST_WIDE_INT) 0)
{
  int nn = 0, ne = 0, msq = 0, spl = 0, per = 0, kun = 1, maxreg = -1;

    /* KUNROLL is the max per-value power-of-two degree (§7.8), and the
       unrolled period is KUNROLL * II.  */
    for (unsigned v = 0; v < deg.length (); v++)
      {
	int p2 = 1;
	while (p2 < deg[v])
	  p2 <<= 1;
	if (p2 > kun)
	  kun = p2;
      }
    per = kun * ii;

    if (per > 0 && per <= LVX_CMS_IG_MAX_PERIOD)
      {
	auto_vec<unsigned HOST_WIDE_INT *> occ;
	auto_vec<int> ncls, nval, nname, nnreg;
	int words = (per + 63) / 64;

	for (unsigned v = 0;
	     v < deg.length () && nn < LVX_CMS_IG_MAX_NODES; v++)
	  {
	    /* The range starts one cycle AFTER a normal definition and AT an
	       earlyclobber one (§7ter.2a).  kvx-unfold.cc computes the
	       equivalent UNFOLD_RANGE_FLAG_DEF_FIRST, dumps it, and has both
	       consumers commented out -- see §7ter.11 for why re-enabling it
	       there is coupled to fixing the effect classification.  */
	    int ec = ec_[v];
	    int d = def_[v] - lo0 + (ec ? 0 : 1);
	    int l = last_[v] - def_[v] - (ec ? 0 : 1);
	    int nr = nregs_[v];
	    /* One name per stage the value spans.  Rounded up to a power of two
	       because that is what the reference allocates (§7quater.5) -- but
	       the rounding is not neutral when ranges are COMBINED, since
	       p2 (a + b) can exceed p2 (a) + p2 (b) even though
	       ceil ((a+b)/II) <= ceil (a/II) + ceil (b/II) always holds.
	       -mcms-exact-degree separates the two effects.  */
	    int p2 = 1;
	    if (lvx_cms_exact_degree)
	      p2 = MAX (deg[v], 1);
	    else
	      while (p2 < deg[v])
		p2 <<= 1;
	    if (l <= 0)
	      continue;

	    /* One node per NAME of this value.  Name J carries the instances
	       of iterations congruent to J modulo P2, which is every P2-th
	       kernel copy -- exact because P2 divides KUNROLL.  */
	    for (int j = 0; j < p2 && nn < LVX_CMS_IG_MAX_NODES; j++)
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
		ncls.safe_push ((int) lvx_cms_rclass_of (nr));
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
		int sq = lvx_cms_squeeze ((enum lvx_cms_rclass) ncls[a], deg);
		if (sq > msq)
		  msq = sq;
		if (sq < lvx_cms_rclass_size[ncls[a]])
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
	for (int k = (int) order.length () - 1; k >= 0; k--)
	  {
	    int a = order[k];
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
}

static int lvx_cms_latency (rtx_insn *);

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

static int
lvx_cms_dates_in_window (const auto_vec<rtx_insn *> &seq,
			 const auto_vec<lvx_cms_arc> &arcs,
			 const auto_vec<int> &dates,
			 const auto_vec<int> &tosink,
			 int ii, int *first_out, int *makespan_out,
			 int *empty_out, auto_vec<int> *estart_out = NULL,
			 auto_vec<int> *lstart_out = NULL,
			 int makespan_floor = 0)
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
      int e = (int) plc.date (i + 1) - root + lvx_cms_latency (seq[i]);
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

static enum lvx_network_result
lvx_cms_mcrl_dates (const auto_vec<rtx_insn *> &seq,
		    const auto_vec<lvx_cms_arc> &arcs,
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
     same criterion lvx_cms_collect_values applies.  */
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
lvx_cms_cumulative_lifetime (const auto_vec<rtx_insn *> &seq,
			     const auto_vec<lvx_cms_arc> &arcs,
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

#define LVX_CMS_RELOAD_LATENCY 3

static int
lvx_cms_spill_until_colourable (const auto_vec<rtx_insn *> &seq,
				const auto_vec<lvx_cms_arc> &arcs,
				const auto_vec<int> &sigma,
				const struct lvx_sched_resources *demand,
				int ii, int lo, int *spill_out, int *nodes_out,
				int *spill0_out)
{
  auto_vec<int> cdef, clast, cdeg, cnr, cec, cown;
  lvx_cms_collect_values (seq, arcs, sigma, ii, cdef, clast, cdeg, cnr, cec,
			  &cown);

  auto_vec<int> tiny, lsu;
  for (int c = 0; c < ii; c++)
    tiny.safe_push (0), lsu.safe_push (0);
  for (unsigned i = 0; i < seq.length (); i++)
    {
      struct lvx_sched_resources one;
      memset (&one, 0, sizeof (one));
      lvx_cms_resources_add (&one, seq[i]);
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
      lvx_cms_colour (cdef, clast, cdeg, cnr, cec, ii, lo,
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

      int latd = MAX (1, lvx_cms_latency (seq[i]));
      int ts = (gp == d) ? d + latd : gp + 1;
      int tr = gq - LVX_CMS_RELOAD_LATENCY;
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
      if (!lvx_cms_repair_fits_ii_p (seq, arcs, &dem2, i, ii,
				     LVX_CMS_RELOAD_LATENCY, 2 * nr,
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
   taken only if it keeps the II (lvx_cms_repair_fits_ii_p: the extra TINY must
   fit the resource budget and the extra cycle of latency must not push the
   parametric period past the II), and only if a cycle exists inside the
   producer-latency window with a free TINY in the cumulative table.

   Victim order is descending degree, which is what sets the expansion and so
   the node count.  Ruttenberg's span-over-references ratio would be the better
   choice for a SPILL, where the cost is the reference count; for a split the
   degree is what the repair reduces, so it is the natural key.  Capped at 8
   splits, after Ruttenberg's cap on escalation.  */

static int
lvx_cms_split_until_colourable (const auto_vec<rtx_insn *> &seq,
				const auto_vec<lvx_cms_arc> &arcs,
				const auto_vec<int> &sigma,
				const struct lvx_sched_resources *demand,
				int ii, int lo, int *spill_out, int *nodes_out,
				int *spill0_out)
{
  auto_vec<int> cdef, clast, cdeg, cnr, cec, cown;
  lvx_cms_collect_values (seq, arcs, sigma, ii, cdef, clast, cdeg, cnr, cec,
			  &cown);

  /* Modulo TINY occupancy of the schedule, which the copies draw down.  */
  auto_vec<int> tiny;
  for (int c = 0; c < ii; c++)
    tiny.safe_push (0);
  for (unsigned i = 0; i < seq.length (); i++)
    {
      struct lvx_sched_resources one;
      memset (&one, 0, sizeof (one));
      lvx_cms_resources_add (&one, seq[i]);
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
      lvx_cms_colour (cdef, clast, cdeg, cnr, cec, ii, lo,
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
      if (!lvx_cms_repair_fits_ii_p (seq, arcs, &dem2, i, ii, 1, nr))
	{
	  declined[best] = true;	/* look at the next one */
	  continue;
	}

      /* A cycle in the producer-latency window with NR free TINY.  */
      int lat = MAX (1, lvx_cms_latency (seq[i]));
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

static int
lvx_cms_latency (rtx_insn *insn)
{
  if (lvx_doloop_end_p (insn))
    return 1;
  return MAX (1, insn_default_latency (insn)
		 + lvx_cms_load_latency_delta (insn));
}

static bool
lvx_cms_slot_fits_p (const struct lvx_sched_resources *slot, rtx_insn *insn)
{
  struct lvx_sched_resources trial = *slot;
  lvx_cms_resources_add (&trial, insn);

  if (trial.ninsns_count > (unsigned) lvx_sched_issue_rate ())
    return false;
#define LVX_CMS_FITS(R)							  if (trial.R##_count > (unsigned) lvx_issue_availability.R)		    return false;
  LVX_ISSUE_RESOURCES (LVX_CMS_FITS)
#undef LVX_CMS_FITS
  return true;
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

static bool
lvx_cms_converge (const auto_vec<rtx_insn *> &seq,
		  const auto_vec<lvx_cms_arc> &arcs,
		  const auto_vec<int> &prio,
		  const auto_vec<int> &release,
		  int ii, int delta, auto_vec<int> &sigma,
		  auto_vec<int> *blame, auto_vec<int> *culprit,
		  int *resblame)
{
  unsigned n = seq.length ();
  if (n == 0 || ii <= 0)
    return false;

  /* Enough copies to see a repeat beyond the furthest-reaching arc.  */
  int ncopy = delta + 4;
  unsigned cells = n * (unsigned) ncopy;

  auto_vec<int> date, pred;
  date.safe_grow_cleared (cells);
  pred.safe_grow_cleared (cells);
  for (unsigned k = 0; k < cells; k++)
    date[k] = -1;

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

  auto_vec<struct lvx_sched_resources> cyc;

  int placed = 0;
  while (placed < (int) cells)
    {
      /* Pick a ready cell: lowest copy first, then by PRIO ascending, then
	 program order.  Ascending, so a priority is a cost and not a merit --
	 longest-path-to-sink therefore enters negated.  This comment used to
	 say "the longest remaining dependence path", which is the path to the
	 SINK and the opposite of what the default priority is.  */
      int best = -1;
      for (int c = 0; c < ncopy; c++)
	{
	  for (unsigned i = 0; i < n; i++)
	    {
	      unsigned k = (unsigned) c * n + i;
	      if (date[k] >= 0 || pred[k] > 0)
		continue;
	      if (best < 0
		  || prio[i] < prio[(unsigned) best % n]
		  || (prio[i] == prio[(unsigned) best % n]
		      && i < (unsigned) best % n))
		best = (int) k;
	    }
	  if (best >= 0)
	    break;
	}
      if (best < 0)
	/* Nothing ready and cells remain: the enforced subgraph has a circuit
	   the chosen II cannot break.  */
	return false;

      unsigned bi = (unsigned) best % n;
      int bc = best / (int) n;

      /* Earliest dependence-feasible date.  */
      int earliest = 0;
      if (bc > 0)
	earliest = date[(unsigned) (bc - 1) * n + bi] + ii;
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].dst == bi)
	  {
	    int sc = bc - (int) arcs[a].omega;
	    if (sc < 0 || sc >= ncopy)
	      continue;
	    int d = date[(unsigned) sc * n + arcs[a].src];
	    if (d < 0)
	      return false;
	    if (earliest < d + (int) arcs[a].theta)
	      earliest = d + (int) arcs[a].theta;
	  }
      if (earliest < 0)
	earliest = 0;

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
      if (!release.is_empty () && release[bi] > 0)
	{
	  int r = release[bi] + bc * ii;
	  if (earliest < r)
	    earliest = r;
	}

      /* First resource-feasible date at or after it, remembering WHICH counter
	 refused it the first time -- that is the resource to blame, and the
	 one whose holders a release date should move.  */
      int when = earliest;
      int blocked_by = -1;
      while (true)
	{
	  while ((int) cyc.length () <= when)
	    {
	      struct lvx_sched_resources empty;
	      memset (&empty, 0, sizeof (empty));
	      cyc.safe_push (empty);
	    }
	  if (lvx_cms_slot_fits_p (&cyc[when], seq[bi]))
	    break;
	  if (blocked_by < 0)
	    blocked_by = lvx_cms_blocking_resource (&cyc[when], seq[bi]);
	  when++;
	}
      lvx_cms_resources_add (&cyc[when], seq[bi]);
      date[(unsigned) best] = when;
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
      if (blame != NULL && when > earliest)
	{
	  (*blame)[bi] += when - earliest;

	  if (blocked_by >= 0)
	    {
	      if (resblame != NULL)
		resblame[blocked_by] += when - earliest;

	      if (culprit != NULL)
		for (unsigned ck = 0; ck < cells; ck++)
		  if (date[ck] >= earliest && date[ck] < when
		      && lvx_cms_insn_uses_resource_p (seq[ck % n], blocked_by))
		    (*culprit)[ck % n]++;
	    }
	}

      /* Release the successors of this cell.  */
      for (unsigned a = 0; a < arcs.length (); a++)
	if (arcs[a].src == bi)
	  {
	    int dc = bc + (int) arcs[a].omega;
	    if (dc >= 0 && dc < ncopy)
	      pred[(unsigned) dc * n + arcs[a].dst]--;
	  }
      if (bc + 1 < ncopy)
	pred[(unsigned) (bc + 1) * n + bi]--;

      /* Stationarity: a copy that repeats its predecessor exactly II cycles
	 later IS the kernel.  */
      if (bc > 0 && (unsigned) placed % n == 0)
	{
	  bool settled = true;
	  for (unsigned i = 0; i < n && settled; i++)
	    {
	      int cur = date[(unsigned) bc * n + i];
	      int prev = date[(unsigned) (bc - 1) * n + i];
	      if (cur < 0 || prev < 0 || cur - prev != ii)
		settled = false;
	    }
	  if (settled)
	    {
	      sigma.truncate (0);
	      for (unsigned i = 0; i < n; i++)
		sigma.safe_push (date[(unsigned) bc * n + i] - bc * ii);
	      return true;
	    }
	}
    }

  return false;
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

static void
lvx_cms_path_metrics (const auto_vec<rtx_insn *> &seq,
		      const auto_vec<lvx_cms_arc> &arcs,
		      const auto_vec<int> &asap,
		      auto_vec<int> &tosink, auto_vec<int> &slack)
{
  unsigned n = seq.length ();

  tosink.truncate (0);
  slack.truncate (0);
  tosink.safe_grow_cleared (n);
  slack.safe_grow_cleared (n);

  for (unsigned i = 0; i < n; i++)
    tosink[i] = lvx_cms_latency (seq[i]);

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
    makespan = MAX (makespan, asap[i] + lvx_cms_latency (seq[i]));
  for (unsigned i = 0; i < n; i++)
    slack[i] = (makespan - tosink[i]) - asap[i];
}

static const char *lvx_cms_verify (const auto_vec<rtx_insn *> &,
				   const auto_vec<lvx_cms_arc> &,
				   int, const auto_vec<int> &);

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

static bool
lvx_cms_swo_at_ii (const auto_vec<rtx_insn *> &seq,
		   const auto_vec<lvx_cms_arc> &arcs,
		   const auto_vec<int> &height,
		   int ii, int delta, auto_vec<int> &sigma,
		   int budget, int *passes, const char **last_bad,
		   int *resblame_out, int pin, int mcrl_release = -1)
{
  /* -1 means `whatever the option says'; the gate below passes an explicit
     setting so it can try a II both ways.  */
  if (mcrl_release < 0)
    mcrl_release = lvx_cms_mcrl_release;
  unsigned n = seq.length ();
  auto_vec<int> prio, release, blame, culprit, cumulative, tosink, slack, base;
  auto_vec<int> guilt;
  int resblame[LVX_CMS_NRESOURCES];

  memset (resblame, 0, sizeof (resblame));
  prio.safe_grow_cleared (n);
  release.safe_grow_cleared (n);
  cumulative.safe_grow_cleared (n);
  guilt.safe_grow_cleared (n);
  base.safe_grow_cleared (n);
  lvx_cms_path_metrics (seq, arcs, height, tosink, slack);

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
      lvx_cms_dates_in_window (seq, arcs, nodates, tosink, ii,
			       &wfirst, &wmake, &wempty, &es, &ls, 0);
      if (!wempty && es.length () == n && ls.length () == n)
	{
	  HOST_WIDE_INT mcost;
	  unsigned nn, ne;
	  auto_vec<int> md;
	  if (lvx_cms_mcrl_dates (seq, arcs, es, ls, ii, md, &mcost, &nn, &ne)
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
		m = MAX (m, height[i] + lvx_cms_latency (seq[i]));
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
	   makespan - tosink (see lvx_cms_path_metrics, where
	   slack = makespan - tosink - asap and asap is HEIGHT), so
	   height + slack = makespan - tosink, and ordering by that is
	   ordering by -tosink -- which is case 1.  The dumps were
	   byte-identical, as they had to be.  §7vicies.  */
      default: base[i] = height[i];  break;
      }

  for (unsigned i = 0; i < n; i++)
    prio[i] = base[i];

  for (int pass = 0; pass < budget; pass++)
    {
      blame.truncate (0);
      blame.safe_grow_cleared (n);
      culprit.truncate (0);
      culprit.safe_grow_cleared (n);

      (*passes)++;
      if (lvx_cms_converge (seq, arcs, prio, release, ii, delta, sigma,
			    &blame, &culprit, resblame))
	{
	  const char *bad = lvx_cms_verify (seq, arcs, ii, sigma);
	  if (!bad)
	    {
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
	      bool allocates = true;
	      if (lvx_cms_regalloc_feasible)
		{
		  auto_vec<int> cdef, clast, cdeg, cnr, cec, cown, stuck;
		  lvx_cms_collect_values (seq, arcs, sigma, ii, cdef, clast,
					  cdeg, cnr, cec, &cown);
		  int nn, ne, msq, spl, per, kun;
		  lvx_cms_colour (cdef, clast, cdeg, cnr, cec, ii,
				  lvx_cms_sigma_min (sigma), &nn, &ne, &msq,
				  &spl, &per, &kun, &stuck);
		  if (spl > 0)
		    {
		      allocates = false;
		      *last_bad = "kernel not colourable";
		      /* Blame the DEFINING instruction of every value left in
			 the graph.  Its range is what interferes, and the
			 release lever below moves a task later -- which
			 shortens a value's live range from its definition
			 end, lowering interference without touching II.  */
		      for (unsigned q = 0; q < stuck.length (); q++)
			{
			  int v = stuck[q];
			  if (v >= 0 && v < (int) cown.length ())
			    {
			      int i = cown[v];
			      if (i >= 0 && i < (int) n)
				blame[i]++, culprit[i]++;
			    }
			}
		    }
		}

	      if (allocates)
		{
		  if (resblame_out)
		    memcpy (resblame_out, resblame, sizeof (resblame));
		  return true;
		}
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
	  if (resblame_out)
	    memcpy (resblame_out, resblame, sizeof (resblame));
	  return false;
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
      for (unsigned i = 0; i < n; i++)
	{
	  if (lvx_cms_release_policy == 3)
	    {
	      guilt[i] += culprit[i];
	      int want = height[i] + MIN (slack[i], guilt[i]);
	      if (slack[i] <= 0)
		want = 0;
	      if (release[i] < want)		/* ratchet */
		release[i] = want;
	      continue;
	    }

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
	    case 2:  move = (culprit[i] > 0 && blame[i] == 0 && slack[i] > 0);
		     break;
	    case 4:  move = (slack[i] > 0); break;	/* no selectivity at all */
	    default: move = (blame[i] == 0 && slack[i] > 0); break;
	    }
	  /* Its ALAP when moved.  When not moved, back to the MCRL date under
	     the persistent-base reading and to nothing under the pass-0 one. */
	  int home = (mcrl_release == 2 && !mcrl.is_empty ()
		      ? mcrl[i] : 0);
	  release[i] = move ? height[i] + slack[i] : home;
	}

      /* The branch pin is structural, so it survives the policy's
	 reassignment above -- which sets RELEASE to 0 for every task it
	 declines to move, and would otherwise undo the pin on the first
	 pass that blames the branch.  */
      if (branch_i >= 0 && release[branch_i] < branch_floor)
	release[branch_i] = branch_floor;
    }

  if (resblame_out)
    memcpy (resblame_out, resblame, sizeof (resblame));
  return false;
}

/* Verify SIGMA is a legal modulo schedule at II.  Returns null, or the first
   violation found.  Two things must hold, and they are the definition:

     every arc:   sigma (dst) - sigma (src)  >=  theta - omega * II
     resources:   the nodes sharing a slot (sigma mod II) fit one bundle

   The second is why a modulo reservation table is the right structure even
   though the scheduler itself works in absolute cycles -- in steady state every
   II cycles the same slot comes round.  */

static const char *
lvx_cms_verify (const auto_vec<rtx_insn *> &seq,
		const auto_vec<lvx_cms_arc> &arcs,
		int ii, const auto_vec<int> &sigma)
{
  if (ii <= 0 || sigma.length () != seq.length ())
    return "malformed schedule";

  for (unsigned a = 0; a < arcs.length (); a++)
    {
      HOST_WIDE_INT need = arcs[a].theta - arcs[a].omega * (HOST_WIDE_INT) ii;
      if ((HOST_WIDE_INT) (sigma[arcs[a].dst] - sigma[arcs[a].src]) < need)
	return "dependence arc violated";
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
      if (!lvx_cms_slot_fits_p (&slot[at], seq[i]))
	return "modulo reservation overflows";
      lvx_cms_resources_add (&slot[at], seq[i]);
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
lvx_cms_build_graph (class loop *loop, basic_block *body,
		     auto_vec<rtx_insn *> &seq,
		     auto_vec<lvx_cms_arc> &arcs,
		     auto_vec<int> &height,
		     int *dropped_safelen)
{
  /* The weakest safelen of any memory arc dropped below, or 0 if none was.
     The caller must check it against the converged NSTAGES: dropping is sound
     only while safelen >= nstages.  */
  *dropped_safelen = 0;
  auto_bitmap inloop;
  rtx_insn *insn;

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
	     The gate above should have caught this already.  */
	  if (dump_file)
	    fprintf (dump_file, ";; lvx unfold: loop %d declined, body is not"
		     " a chain\n", loop->num);
	  return false;
	}
      walk = nxt;
    }

  unsigned n = seq.length ();
  if (n == 0)
    return false;

  /* Addresses as affine functions of the iteration, for the stride test that
     the memory arcs below use to disprove what the alias oracle cannot.  */
  auto_vec<lvx_mem_iv> mivs;
  lvx_cms_collect_mem_ivs (loop, seq, mivs);

  /* Simple inductions, for inductive relaxation below.  */
  auto_vec<HOST_WIDE_INT> ivdelta;
  auto_bitmap inductions;
  lvx_cms_find_simple_inductions (seq, ivdelta, inductions);

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

	  /* The last definition strictly before Q.  */
	  bool partial_def = false;
	  for (int i = (int) q - 1; i >= 0 && p < 0; i--)
	    if (lvx_insn_defs_regno_p (seq[i], regno, &partial_def))
	      p = i;

	  /* A partial reaching definition does not define the whole value, so
	     one arc from it would omit the dependence on whatever supplied the
	     rest.  Refuse rather than under-constrain.  */
	  if (partial_def)
	    {
	      if (dump_file)
		fprintf (dump_file, ";; lvx cms: fn=%s loop %d declined, partial"
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
	  if (lvx_cms_inductive_relaxation
	      && regno < FIRST_PSEUDO_REGISTER
	      && bitmap_bit_p (inductions, regno)
	      && lvx_cms_relaxed_base_use_p (seq[q], regno, ivdelta[regno]))
	    {
	      nrelaxed++;
	      continue;
	    }

	  int lat = lvx_arc_latency (seq[p], seq[q], REG_DEP_TRUE);
	  lvx_cms_push_arc (arcs, (unsigned) p, q, lat, w, LVX_ARC_REG_RAW);
	  if (w == 0 && height[q] < height[p] + lat)
	    height[q] = height[p] + lat;
	}
    }

  if (dump_file && lvx_cms_probe)
    fprintf (dump_file, ";; lvx cms: fn=%s loop %d, %u simple inductions,"
	     " %u relaxed arcs dropped\n", current_function_name (), loop->num,
	     bitmap_count_bits (inductions), nrelaxed);

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

	  /* Only where renaming cannot remove it: the killing definition has
	     to be one that names its own destination as a source.  */
	  if (!lvx_insn_def_reads_dest_p (seq[p]))
	    continue;

	  lvx_cms_push_arc (arcs, q, (unsigned) p,
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
	     own destination that the later result depends on the earlier.  */
	  if (!partial
	      && !lvx_insn_def_reads_dest_p (seq[p])
	      && !lvx_insn_def_reads_dest_p (seq[(unsigned) q]))
	    continue;

	  lvx_cms_push_arc (arcs, p, (unsigned) q,
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
	      if (lvx_cms_mem_independent_p (mivs[p], mivs[q], 0))
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
		  lvx_cms_push_arc (arcs, p, q, lat, 0, LVX_ARC_MEM_INTRA);
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
	  int sl = lvx_cms_assume_ivdep ? INT_MAX
		   : (loop->safelen > 1 ? loop->safelen : 1);

	  if (sl > LVX_CMS_MAX_DELTA)
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
	  while (mw <= LVX_CMS_MAX_DELTA
		 && lvx_cms_mem_independent_p (mivs[p], mivs[q], mw))
	    mw++;

	  if (mw > LVX_CMS_MAX_DELTA)
	    {
	      if (*dropped_safelen == 0 || *dropped_safelen > mw)
		*dropped_safelen = mw;
	      continue;
	    }

	  if (a_w && b_r)
	    lvx_cms_push_arc (arcs, p, q,
			 lvx_arc_latency (a, b, REG_DEP_TRUE), mw,
			 LVX_ARC_MEM_CROSS);
	  else if (a_w && b_w && p != q)
	    lvx_cms_push_arc (arcs, p, q,
			 lvx_arc_latency (a, b, REG_DEP_OUTPUT), mw,
			 LVX_ARC_MEM_CROSS);
	  else if (a_r && b_w && p != q)
	    {
	      lvx_cms_push_arc (arcs, p, q,
			   lvx_arc_latency (a, b, REG_DEP_ANTI), mw,
			   LVX_ARC_MEM_CROSS);
	      lvx_cms_push_arc (arcs, q, p,
			   lvx_arc_latency (b, a, REG_DEP_TRUE), mw,
			   LVX_ARC_MEM_CROSS);
	    }
	}
    }

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
     the instance it constrains is then the next iteration's.  */
  int first_branch = -1, prev_branch = -1;
  for (unsigned p = 0; p < n; p++)
    {
      basic_block bb, exitbb = NULL;
      edge e;
      edge_iterator ei;

      if (!JUMP_P (seq[p]))
	continue;

      /* (3) */
      if (prev_branch >= 0)
	lvx_cms_push_arc (arcs, prev_branch, p, 0, 0, LVX_ARC_CTRL_ORDER);
      else
	first_branch = (int) p;
      prev_branch = (int) p;

      bb = BLOCK_FOR_INSN (seq[p]);
      if (bb != NULL)
	FOR_EACH_EDGE (e, ei, bb->succs)
	  if (!bitmap_bit_p (inloop, e->dest->index)
	      && e->dest != EXIT_BLOCK_PTR_FOR_FN (cfun))
	    exitbb = e->dest;

      for (unsigned q = 0; q < n; q++)
	{
	  HOST_WIDE_INT w = (q > p) ? 0 : 1;
	  df_ref def;

	  if (q == p)
	    continue;

	  /* (1) */
	  if (lvx_mem_write_insn_p (seq[q])
	      || volatile_insn_p (PATTERN (seq[q])))
	    lvx_cms_push_arc (arcs, p, q, 0, w, LVX_ARC_CTRL_STORE);

	  /* (2) */
	  if (exitbb == NULL)
	    continue;
	  FOR_EACH_INSN_DEF (def, seq[q])
	    if (DF_REF_REGNO (def) < FIRST_PSEUDO_REGISTER
		&& bitmap_bit_p (df_get_live_in (exitbb), DF_REF_REGNO (def)))
	      {
		lvx_cms_push_arc (arcs, p, q, 0, w, LVX_ARC_CTRL_LIVEOUT);
		break;
	      }
	}
    }

  /* (3) across the back edge: the last branch still precedes the first.  */
  if (first_branch >= 0 && prev_branch > first_branch)
    lvx_cms_push_arc (arcs, prev_branch, first_branch, 0, 1,
		      LVX_ARC_CTRL_BACKEDGE);

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

static bool
lvx_cms_bounds (class loop *loop, basic_block *body,
		const auto_vec<rtx_insn *> &seq,
		const auto_vec<lvx_cms_arc> &arcs,
		const auto_vec<int> &height, int *pm, int *pr, int *pk,
		auto_vec<int> *crit = NULL)
{
  unsigned n = seq.length ();
  rtx_insn *insn;

  /* M, the MAKESPAN of one iteration with no overlap: the last instruction's
     start date plus its own processing time, not plus one.  HEIGHT[i] is a
     start date, so a schedule is finished when every instruction has had its
     processing time, which is what lvx_cms_latency gives -- an issue slot is
     what an instruction takes, but a multi-cycle result is not ready when the
     slot closes.  Taking + 1 instead measured the span as if every instruction
     retired in the cycle it issued, which understates any body ending in a
     load, a multiply or a reduction's FP add.  */
  int m = 1;
  for (unsigned i = 0; i < n; i++)
    m = MAX (m, height[i] + lvx_cms_latency (seq[i]));

  /* R, from the machine description.  */
  struct lvx_sched_resources resources;
  memset (&resources, 0, sizeof (resources));
  for (unsigned i = 0; i < loop->num_nodes; i++)
    FOR_BB_INSNS (body[i], insn)
      lvx_cms_resources_add (&resources, insn);
  int r = (int) lvx_sched_resources_full_bundles (&resources);
  r = r ? r : 1;

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

static void
lvx_cms_probe_loop (class loop *loop, basic_block *body)
{
  auto_vec<rtx_insn *> seq;
  auto_vec<lvx_cms_arc> arcs;
  auto_vec<int> height;
  int dropped_safelen = 0;

  if (!lvx_cms_build_graph (loop, body, seq, arcs, height, &dropped_safelen))
    {
      fprintf (dump_file, ";; lvx cms: fn=%s loop %d declined, body is not a"
	       " chain the analysis can walk\n", current_function_name (),
	       loop->num);
      return;
    }

  int m, r, k;
  auto_vec<int> crit;
  if (!lvx_cms_bounds (loop, body, seq, arcs, height, &m, &r, &k, &crit))
    {
      fprintf (dump_file, ";; lvx cms: fn=%s loop %d declined, the recurrence"
	       " bound did not solve\n", current_function_name (), loop->num);
      return;
    }

  int delta = lvx_cms_delta (arcs);
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
	lvx_cms_resources_add (&rdem, bi);
    }
  const char *tight_name = "none";
  int tight_dem = 0, tight_cap = 0;
  double tight_frac = 0.0;
#define LVX_CMS_TIGHT(R)						  {									    unsigned avail = lvx_issue_availability.R;				    if (avail > 0 && rdem.R##_count > 0)				      {										double f = (double) rdem.R##_count / ((double) lower * avail);		if (f > tight_frac)							  {									    tight_frac = f; tight_name = #R;					    tight_dem = (int) rdem.R##_count;					    tight_cap = (int) (lower * avail);					  }								      }									  }
  LVX_ISSUE_RESOURCES (LVX_CMS_TIGHT)
#undef LVX_CMS_TIGHT
  auto_vec<int> sigma;
  int found = 0;
  const char *last_bad = NULL;

  /* At each candidate II, spend the SWO budget before moving on.  A budget of
     one is a single greedy construction, which is what this did before the SWO
     cycle existed, so the baseline is reproducible with -mcms-swo-budget=1.  */
  int passes = 0;
  int budget = MAX (1, lvx_cms_swo_budget);
  int resblame[LVX_CMS_NRESOURCES];
  memset (resblame, 0, sizeof (resblame));
  /* The branch pin can raise II -- it is a release floor, so it cannot make the
     problem infeasible, but it can make this II infeasible and push the search
     on.  Measured on PolyBench it improves 17 loops and costs one (syr2k's
     loop 8, II 3 -> 6).  Policy 3 therefore takes the pin only where it is
     free: try each II pinned, and before giving up on that II, try it again
     unpinned.  II is never lost and the epilogue saving is taken wherever it
     costs nothing.  */
  int pin = lvx_cms_branch_stage;
  /* -mcms-ii-bias starts the search above the bound.  It exists to answer a
     question the ordinary search cannot: when two schedulers disagree only
     because one of them had to take a larger II, comparing them needs both to
     be at the SAME larger II.  §7quinvicies.  */
  int first_ii = lower + (int) lvx_cms_ii_bias;
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
			     budget, &passes, &last_bad, resblame,
			     pin == 3 ? 1 : pin, lvx_cms_mcrl_release))
	found = try_ii;
      else if (pin == 3
	       && lvx_cms_swo_at_ii (seq, arcs, height, try_ii, delta, sigma,
				     budget, &passes, &last_bad, resblame, 0,
				     lvx_cms_mcrl_release))
	found = try_ii;
      /* Arm A: this II refused an un-seeded construction.  Before raising it,
	 ask MCRL where the tasks want to go.

	 ARM A IS ONLY SOUND WITH THE COLOURING IN ACCEPTANCE, and measurement
	 showed why rather than argument: with -mno-cms-regalloc-feasible the
	 acceptance test is verification alone, so arm A happily takes a
	 SMALLER II with a kernel that cannot be coloured -- fdtd-2d's loop 14
	 went II 9 -> 7 and 0 -> 20 uncolourable values, a smaller II being more
	 stages and so more pressure (§7octodecies).  With the colouring in
	 acceptance the same loop goes 11 -> 9 and stays colourable.  That is
	 why -mcms-regalloc-feasible now defaults ON: the two are one mechanism,
	 and the gate is what pays back most of what the colouring costs in II.
	 §7septvicies.5.  */
      else if (lvx_cms_mcrl_gate
	       && lvx_cms_swo_at_ii (seq, arcs, height, try_ii, delta, sigma,
				     budget, &passes, &last_bad, resblame,
				     pin == 3 ? 1 : pin, 2))
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
      lvx_cms_path_metrics (seq, arcs, height, ts, sl);
      lvx_cms_dates_in_window (seq, arcs, nodates, ts, found, &wfirst, &wmake,
			       &wempty);
      int have = lvx_cms_sigma_span (sigma);
      if (wmake > 0 && have > wmake)
	{
	  auto_vec<int> alt;
	  int p2 = 0;
	  const char *b2 = NULL;
	  if (lvx_cms_swo_at_ii (seq, arcs, height, found, delta, alt, budget,
				 &p2, &b2, NULL, pin == 3 ? 1 : pin, 2)
	      && lvx_cms_sigma_span (alt) < have)
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

  /* The counter that cost the most displacement across the whole search: what
     SWO was fighting, as opposed to what merely bounded ResMII.  */
  const char *blamed = "none";
  int blamed_cost = 0;
  for (int i = 0; i < LVX_CMS_NRESOURCES; i++)
    if (resblame[i] > blamed_cost)
      {
	blamed_cost = resblame[i];
	blamed = lvx_cms_resource_name[i];
      }

  /* Whether reorg_loops will put a LOOPDO on this loop.  Reported rather than
     refused: a hardware loop is a HELP to a pipeliner, not an obstacle --
     the loopback branch is implicit, so control speculation past it needs no
     justification and the exit test costs no issue slot.  What it does cost is
     codegen, which has to adjust the trip count for the stages in flight.  */
  bool hw = lvx_loop_has_doloop_p (loop);

  if (!found)
    {
      fprintf (dump_file, ";; lvx cms: fn=%s loop %d NO schedule through II=%d"
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
     (lvx_cms_slot_fits_p tests a single cycle's counters), so that last cycle
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
  int lo = sigma[0], hi = sigma[0], hilat = sigma[0] + lvx_cms_latency (seq[0]);
  for (unsigned i = 1; i < sigma.length (); i++)
    {
      int done = sigma[i] + lvx_cms_latency (seq[i]);
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
      sp_lat.safe_push (lvx_cms_latency (seq[i]));
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
    if (lvx_cms_speculated_load_p (seq[i], bodydef))
      spec++;

  if (lvx_cms_force_spec_loads && spec)
    spec = -lvx_cms_force_speculative_loads (seq, bodydef);

  /* Stage scheduling, if asked: shift operations by whole periods to shorten
     live ranges.  Done here, after the II search has converged and before
     anything measures the result, because it cannot change the II or the
     resource feasibility -- only the stages.  §7octodecies.2.  */
  int staged = 0;
  if (lvx_cms_stage_sched)
    staged = lvx_cms_stage_schedule (arcs, seq.length (), sigma, found);

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
	lvx_cms_resources_add (&one, seq[i]);
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

       LAT comes from lvx_cms_latency, the producer's own processing time.  That
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
	       saturate LITE outright.  With -mcms-split-wide=1 take CATDQ
	       where it fits and fall back to COPYD, which is the better way
	       round: TINY is the binding counter on this corpus (§7septies.3's
	       tight= field names it in 46 of 51 missing loops).  */
	    int need_t = nr, need_l = 0;
	    if (lvx_cms_split_wide == 1 && nr >= 2)
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
  int igmaxreg = -1, igassigned = 0;
  {
    auto_vec<int> av, an, ar;
    lvx_cms_colour (sp_def, sp_last, sp_deg, sp_nregs, sp_ec, found, lo,
		    &ignodes, &igedges, &igmaxsq, &igspill, &igperiod,
		    &igkunroll, NULL, &av, &an, &ar, &igmaxreg);
    for (unsigned q = 0; q < ar.length (); q++)
      if (ar[q] >= 0)
	igassigned++;
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
    lvx_cms_lineages (arcs, seq.length (), defines, lin, &nlin, &nseq);

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
	lvx_cms_colour (cdef, clast, cdeg, cnr, cec, found,
			lvx_cms_sigma_min (sigma), &linnodes, &linedges,
			&linsq, &linspill, &per, &kun);
      }
  }

  /* How many expanded values admit a repair that keeps MinII?  §7novodecies.
     SPLITOK counts those a splitting COPYD could serve (one extra TINY per
     register, latency 1); SPILLOK those a store/reload could (two extra
     instructions, and the reload's latency, which under -mcms-load-latency is
     the uncached figure).  */
  int splitok = 0, spillok = 0, cand = 0, nsplit = 0, spl_after = -1;
  int spl_nodes = 0, spl_before = -1, nspill = 0, sp_after = -1, sp_why = 0;
  {
    struct lvx_sched_resources dem;
    memset (&dem, 0, sizeof (dem));
    for (unsigned i = 0; i < loop->num_nodes; i++)
      {
	rtx_insn *bi;
	FOR_BB_INSNS (body[i], bi)
	  lvx_cms_resources_add (&dem, bi);
      }
    /* A spill reload is a CACHED load whatever the data loads are (author,
       2026-10-27): the slot is in the frame, which is hot, so it is priced at
       the description's cached figure of 3 and NOT at -mcms-load-latency's
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
	if (lvx_cms_repair_fits_ii_p (seq, arcs, &dem, i, found, 1, nr))
	  splitok++;
	if (lvx_cms_repair_fits_ii_p (seq, arcs, &dem, i, found, rl, 2 * nr))
	  spillok++;
      }

    /* And does splitting actually make the kernel colourable?  Only asked when
       it was not, since otherwise there is nothing to repair.  */
    if (igspill > 0)
      {
	nsplit = lvx_cms_split_until_colourable (seq, arcs, sigma, &dem, found,
						 lvx_cms_sigma_min (sigma),
						 &spl_after, &spl_nodes,
						 &spl_before);
	int dummy;
	nspill = lvx_cms_spill_until_colourable (seq, arcs, sigma, &dem, found,
						 lvx_cms_sigma_min (sigma),
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
    lvx_cms_path_metrics (seq, arcs, height, ts, sl);
    for (int round = 0; round < 2; round++)
      {
	auto_vec<int> es, ls;
	int floor_ = round ? span : 0;
	int wempty = 0, wfirst = -1, wmake = 0;
	lvx_cms_dates_in_window (seq, arcs, nodates, ts, found,
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
	  = lvx_cms_mcrl_dates (seq, arcs, es, ls, found, md, &mcost, &nn, &ne);
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
	  = lvx_cms_cumulative_lifetime (seq, arcs, md, found);
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
	    int slo = lvx_cms_sigma_min (sigma);
	    for (unsigned i = 0; i < sigma.length (); i++)
	      sg.safe_push (sigma[i] - slo);
	    mcrlsigma = lvx_cms_cumulative_lifetime (seq, arcs, sg, found);
	  }
	else
	  mcrllife2 = life;
      }
  }

  fprintf (dump_file, ";; lvx cms: fn=%s loop %d II=%d lower=%d excess=%d M=%d"
	   " ResMII=%d RecMII=%d span=%d spanlat=%d nstages=%d BS=%d preps=%d"
	   " mvemax=%d mvep2=%d mvelcm=%d mven=%d mvetop=%d mve2nd=%d"
	   " mvesplit0=%d mvesplit=%d mvesplitr=%d mvesplitx=%d"
	   " mvenospl=%d mvewide=%d mvewidemax=%d mvecatdq=%d"
	   " mveregs=%d mveregsx=%d mvepress=%d"
	   " ig=%d/%d kunroll=%d period=%d sq=%d spill=%d ec=%d"
	   " igassigned=%d igmaxreg=%d"
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
	   igec, igassigned, igmaxreg, nlin, nseq, linnodes, linedges, linsq, linspill,
	   cand, splitok, spillok, staged, nsplit, spl_before, spl_after,
	   nspill, sp_after, sp_why, winempty, winmake,
	   mcrl, mcrlbad, mcrldep, mcrlmake, mcrlnn, mcrlne,
	   mcrllife, mcrlsigma, mcrl2, mcrlmake2, mcrllife2,
	   emptycyc,
	   delta, (int) loop->safelen, passes, tight_name, tight_dem, tight_cap,
	   blamed, blamed_cost, seq.length (), arcs.length (),
	   spec < 0 ? -spec : spec, spec < 0 ? "forced to" : "need",
	   hw ? " hwloop" : "",
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
      fprintf (dump_file, ";; lvx cms: fn=%s loop %d RecMII circuit, %u arcs,"
	       " sum theta=" HOST_WIDE_INT_PRINT_DEC
	       " sum omega=" HOST_WIDE_INT_PRINT_DEC ":",
	       current_function_name (), loop->num, nreal, st, sw);
      for (unsigned i = 0; i < crit.length (); i++)
	if (crit[i] >= 0)
	  {
	    const lvx_cms_arc &a = arcs[crit[i]];
	    fprintf (dump_file, " %s(%d->%d,%d,%d)",
		     lvx_cms_arc_kind_name[(int) a.kind],
		     INSN_UID (seq[a.src]), INSN_UID (seq[a.dst]),
		     (int) a.theta, (int) a.omega);
	  }
      fprintf (dump_file, "\n");
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
   dataflow over pseudos, while lvx_cms_build_graph reconstructs them after
   allocation from reaching definitions.  Comparing the two K values for one
   loop is therefore a test of that reconstruction against ground truth, which
   is the most valuable thing this arrangement buys.  */

static void
lvx_cms_probe_ddg (struct ddg *g, int r)
{
  auto_vec<rtx_insn *> seq;
  auto_vec<lvx_cms_arc> arcs;
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
	enum lvx_cms_arc_kind kind;

	if (e->data_type == REG_DEP)
	  kind = (e->type == TRUE_DEP ? LVX_ARC_REG_RAW
		  : e->type == ANTI_DEP ? LVX_ARC_REG_WAR
		  : LVX_ARC_REG_WAW);
	else
	  kind = e->distance ? LVX_ARC_MEM_CROSS : LVX_ARC_MEM_INTRA;

	lvx_cms_push_arc (arcs, src, dst, e->latency, e->distance, kind);

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
      fprintf (dump_file, ";; lvx cms-on-ddg: recurrence bound did not solve\n");
      return;
    }
  int k = (int) plc.period ();

  int m = 1;
  for (unsigned i = 0; i < n; i++)
    m = MAX (m, height[i] + lvx_cms_latency (seq[i]));

  int delta = lvx_cms_delta (arcs);
  int lower = MAX (k, r);
  auto_vec<int> sigma;
  int passes = 0, found = 0;
  const char *last_bad = NULL;
  int budget = MAX (1, lvx_cms_swo_budget);

  for (int try_ii = lower; try_ii <= lower + 8 && !found; try_ii++)
    if (lvx_cms_swo_at_ii (seq, arcs, height, try_ii, delta, sigma,
			   budget, &passes, &last_bad, NULL,
			   lvx_cms_branch_stage == 3 ? 1
			   : lvx_cms_branch_stage))
      found = try_ii;

  if (!found)
    {
      fprintf (dump_file, ";; lvx cms-on-ddg: NO schedule through II=%d"
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

  fprintf (dump_file, ";; lvx cms-on-ddg: II=%d lower=%d excess=%d ResMII=%d"
	   " RecMII=%d M=%d span=%d nstages=%d delta=%d passes=%d,"
	   " %u nodes %u arcs\n", found, lower, found - lower, r, k, m, span,
	   nstages, delta, passes, n, arcs.length ());
}

/* Whether CMS may consider LOOP at all.  This is NOT lvx_unfoldable_loop_p:
   that gate is sized for code duplication and asks for things a modulo
   schedule does not need, while missing nothing CMS does need.  The
   differences, each deliberate:

   - No `can_duplicate_loop_p'.  The unfold duplicates the body through the
     CFG; CMS replicates the KERNEL by its stage count during modulo
     expansion, which is a different operation on a different object.

   - No block-count bound.  The unfold's is there because duplication doubles
     the blocks; CMS linearises the chain and schedules instructions, so what
     bounds it is instruction count, not block count.

   - Its own instruction bound, -mcms-max-insns, because the cost of a CMS
     attempt is the convergence loop over DELTA + 4 copies of the body, not a
     doubling of code size.

   - Hardware loops are ALLOWED.  The unfold refuses them because halving a
     trip count means peeling; a pipeliner has to adjust the trip count
     anyway, and an implicit loopback branch is a help.  lvx_cms_probe_loop
     reports which loops have one so the measurement can separate them.

   What CMS genuinely requires, and this gate establishes: a single latch, so
   the chain walk has one back edge to measure distance against; no call,
   volatile reference or asm, none of which can be moved across an iteration
   boundary or priced; and no computed jump or complex edge, which the chain
   walk cannot follow.  The chain property itself is left to
   lvx_cms_build_graph, which is the code that needs it and so is the code that
   should report it.

   Note the ORDER against the unfold's gate: the call scan comes before the
   edge counting, so a loop that is both multi-block and call-holding is named
   for the call.  The unfold's gate tests per block and reports whichever test
   that block fails first, which is why PolyBench's print_array loops come out
   as "not a chain" when they also hold an fprintf.  A diagnostic should name
   the disqualifier a reader can act on.  */

static bool
lvx_cms_eligible_loop_p (class loop *loop, basic_block *body, unsigned insns,
			 const char **why)
{
  *why = "multiple latches";
  if (loop->latch == NULL)
    return false;
  *why = "too many insns";
  if (insns > (unsigned) lvx_cms_max_insns)
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
    }
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

static void
lvx_cms_probe_loops (void)
{
  if (!dump_file)
    return;

  /* Self-contained, so the probe does not depend on -msuperblock-loops having
     run and set up loops for its own phases.  It modifies nothing: the stride
     test does its own address recognition rather than calling iv_analyze, which
     would have wanted preheaders and a DF rebuild per loop.  */
  calculate_dominance_info (CDI_DOMINATORS);
  loop_optimizer_init (AVOID_CFG_MODIFICATIONS);
  /* lvx_cms_build_graph reads df_get_live_in to find what a branch implicitly
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

      body = get_loop_body (loop);
      for (unsigned i = 0; i < loop->num_nodes; i++)
	{
	  rtx_insn *insn;
	  FOR_BB_INSNS (body[i], insn)
	    if (NONDEBUG_INSN_P (insn))
	      insns++;
	}

      if (!lvx_cms_eligible_loop_p (loop, body, insns, &why))
	fprintf (dump_file, ";; lvx cms: fn=%s loop %d declined, %s\n",
		 current_function_name (), loop->num, why);
      else
	lvx_cms_probe_loop (loop, body);

      free (body);
    }

  dfa_finish ();
  loop_optimizer_finalize ();
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

static bool
lvx_loop_profitable_to_unfold_p (class loop *loop, basic_block *body,
				 unsigned insns)
{
  auto_vec<rtx_insn *> seq;
  auto_vec<lvx_cms_arc> arcs;
  auto_vec<int> height;

  int dropped_safelen = 0;
  if (!lvx_cms_build_graph (loop, body, seq, arcs, height, &dropped_safelen))
    {
      if (dump_file)
	fprintf (dump_file, ";; lvx unfold: loop %d declined, body is not"
		 " a chain the analysis can walk\n", loop->num);
      return false;
    }
  unsigned n = seq.length ();

  /* M, R and K, shared with the CMS walk rather than recomputed: the three
     bounds are properties of the graph, not of what the caller means to do
     with it.  */
  int m, r, k;
  if (!lvx_cms_bounds (loop, body, seq, arcs, height, &m, &r, &k))
    {
      if (dump_file)
	fprintf (dump_file, ";; lvx unfold: loop %d declined, the recurrence"
		 " bound did not solve\n", loop->num);
      return false;
    }

  /* The steady-state floor, per iteration.  This is the baseline, and it used
     to be MAX (m, r) -- the makespan of ONE iteration scheduled in isolation,
     which is what SCHED2 reports because an EBB is acyclic.  The machine does
     not stop at the latch: it is a scoreboarded in-order pipeline with no
     barrier there, so successive iterations overlap and what a loop actually
     settles at is bounded below by its recurrence, not by its height.

     Comparing against M credited the unfold with closing a gap that was never
     open.  Measured on the two loops the old criterion accepted: dijkstra's
     `enqueue' pointer chase (M=4 R=1 K=3) was predicted to gain 12%, and
     SCHED2 then produced a schedule with the same 2 bundles and the same 4
     modelled cycles per iteration -- bit-identical dynamic bundle counts, a
     no-op, measured +5.045%.  bitcnts' `bit_shifter' (M=3 R=2 K=1) was
     predicted 33%, did shorten its bundle schedule 3 -> 2.5 per iteration as
     the model said, and measured +7.274% anyway.  */

  int floor_ii = MAX (MAX (k, r), m);

  /* How far copy 2 can be hoisted, which bounds the overlap the unfold can
     actually buy.  The duplicated body puts copy 2 below copy 1's exit
     branch, so only a speculatable instruction may move above it; one that may
     not must issue strictly after the branch.  An instruction of copy 2 at
     depth d sits at OFFSET + d, so a non-speculatable one requires
     OFFSET + d > hb, hb being the depth of the deepest branch in the body.

     With everything speculatable this reduces to OFFSET = K, the recurrence,
     which is what the formula used to assume unconditionally.  The pointer
     chase is the case that assumption got wrong: dijkstra's `enqueue' carries
     its recurrence in a plain `ld', so copy 2's load cannot precede copy 1's
     branch and the two copies cannot overlap at all -- the measured schedule
     puts that load one cycle after the branch, and the dynamic bundle count
     came out bit-identical to the un-unfolded loop.  Branches are left out of
     the scan because their ordering is already an arc of its own.  */
  int hb = -1;
  for (unsigned i = 0; i < n; i++)
    if (JUMP_P (seq[i]) && height[i] > hb)
      hb = height[i];

  int offset = k;
  if (hb >= 0)
    for (unsigned i = 0; i < n; i++)
      if (!JUMP_P (seq[i]) && !lvx_speculatable_p (seq[i]))
	offset = MAX (offset, hb + 1 - height[i]);

  /* What a taken branch costs beyond its own bundle before the target's first
     bundle issues.  Processor@minTaken is 2, for a branch that settles the
     next PC in ID; a `cb'/`ccb' waits for its comparison and settles it in RR,
     so it costs one more.  A loop's back edge is always one of those, hence 3
     rather than 2 -- which is what the ISS charges since lvx-gem5 288d83dba3,
     where it had been charging 2 for both.

     FIXME: this belongs in a generated header.  BE/MLIR already receives it as
     kMinTakenLatency, with a per-mnemonic takenBranchLatencyOf() beside it;
     BE/GCC receives neither, so the value is written out here.  Same shape as
     the resource availabilities before they were emitted -- see
     docs/handoff-mds-resource-availabilities.md.  */
#define LVX_MIN_TAKEN 3

  /* The part of the taken-branch cost a loop does not already absorb.
     Unfolding halves how often the back edge is taken -- the duplicated body
     branches back once per two iterations, the other exit falling through --
     so this is the one term that favours the unfold, and the criterion used to
     omit it entirely.

     It is neither purely additive nor purely max-combined, and getting that
     wrong is easy.  The ISS computes a release cycle
     `branch_issue + 1 + penalty' and refuses to issue the successor before it,
     so the successor issues at MAX (release, its own constraints).  The
     penalty is therefore ADDITIVE to everything up to and including the
     branch's own issue -- the body's issue length, which precedes it -- and
     MAX-combined against whatever constrains the successor, the loop-carried
     recurrence reaching forward past the branch.  Measured (lvx-csw-82's sweep,
     cycles per trip against LVX_TAKEN_BUBBLE):

       10 bundles, no stalls            10.18 12.18 13.17 14.17 15.17  additive
       4 bundles, 15-cycle recurrence   17.20 17.20 17.20 17.20 17.20  saturated
       4 bundles, 3-cycle recurrence     5.21  5.21  6.20  7.20  8.20  partial

     So what hides the bubble is the floor MINUS the issue length, not the whole
     floor: the loop waits `floor', but `issue' of that is spent issuing bundles
     before the branch ever executes.  An earlier form of this used the whole
     floor and so zeroed the term almost everywhere -- over 400 newlib units it
     fired on 9 of 155 loops, every one with floor 1, which is exactly backwards
     from what the sweep shows.

     ResMII is the issue length in bundles, which is what the slack wants.  Note
     the unfolded body has its own, over two iterations: twice the resources and
     one taken branch rather than two.  */
  int slack = MAX (0, floor_ii - r);
  int b = MAX (0, LVX_MIN_TAKEN - slack);

  /* S1 is what two iterations cost now, S2 what they would cost unfolded.
     Doubled to keep the halving exact.

     S2 carries 2*k and 2*r as well as k + m: two iterations of the unfolded
     body must still respect the recurrence twice and the resources twice, and
     the old form omitted both, which is how k + m came out below the floor.  */
  int s1 = 2 * (floor_ii + b);

  int f2 = MAX (MAX (2 * r, 2 * k), offset + m);
  int slack2 = MAX (0, f2 - 2 * r);
  int b2 = MAX (0, LVX_MIN_TAKEN - slack2);
  int s2 = f2 + b2;
  int gain = s1 - s2;

  bool ok = gain * 100 >= s1 * lvx_unfold_min_gain;

  if (dump_file)
    fprintf (dump_file, ";; lvx unfold: loop %d %s -- M=%d ResMII=%d"
	     " RecMII=%d floor=%d offset=%d taken=%d/%d, %d insns, gain %d/%d = %d%%\n",
	     loop->num, ok ? "profitable" : "declined", m, r, k, floor_ii, offset, b, b2,
	     insns, gain, s1, s1 ? gain * 100 / s1 : 0);
  return ok;
}

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

static bool
lvx_unfoldable_loop_p (class loop *loop, basic_block *body, unsigned insns,
		       unsigned max_blocks, unsigned max_insns, const char **why)
{
  auto_bitmap inloop;

  *why = "multiple latches";
  if (loop->latch == NULL)
    return false;
  *why = "too many blocks";
  if (loop->num_nodes > max_blocks)
    return false;
  *why = "too many insns";
  if (insns > max_insns)
    return false;

  for (unsigned i = 0; i < loop->num_nodes; i++)
    bitmap_set_bit (inloop, body[i]->index);

  for (unsigned i = 0; i < loop->num_nodes; i++)
    {
      basic_block bb = body[i];
      rtx_insn *insn, *end = BB_END (bb);
      unsigned inside = 0;
      edge e;
      edge_iterator ei;

      *why = "block not schedulable";
      if ((bb->flags & BB_DISABLE_SCHEDULE) != 0 || computed_jump_p (end))
	return false;

      FOR_EACH_EDGE (e, ei, bb->succs)
	{
	  *why = "complex edge";
	  if (e->flags & EDGE_COMPLEX)
	    return false;
	  if (bitmap_bit_p (inloop, e->dest->index))
	    inside++;
	}
      *why = inside ? "body is not a chain" : "block exits the loop entirely";
      if (inside != 1)
	return false;

      for (insn = BB_HEAD (bb); insn != NEXT_INSN (end);
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
  *why = NULL;
  return true;
}

/* Unfold innermost LOOP once, so its body appears twice and the extended
   basic block the post-pass scheduler forms over it is twice as long.

   This is `unroll_loop_stupid' applied late and deliberately: the duplicate
   keeps every exit test, so no trip count, prolog or zero-trip guard is
   needed, and consecutive iterations become schedulable against each other.
   Returns true if LOOP was unfolded.  */
static bool
lvx_unfold_inner_loop (class loop *loop)
{
  basic_block *body;
  unsigned insns = 0;
  const char *why = "";
  bool ok;

  if (loop->latch == NULL
      || loop->num_nodes > lvx_unfold_max_blocks
      || lvx_loop_has_doloop_p (loop)
      || !can_duplicate_loop_p (loop))
    return false;

  body = get_loop_body (loop);
  for (unsigned i = 0; i < loop->num_nodes; i++)
    {
      rtx_insn *insn;
      FOR_BB_INSNS (body[i], insn)
	if (NONDEBUG_INSN_P (insn))
	  insns++;
    }
  if (insns > lvx_unfold_max_insns)
    {
      free (body);
      return false;
    }

  /* Without a gate a loop holding a call, a volatile reference or asm is
     unfolded, and -- worse -- a body that is not a chain reaches the
     profitability test, whose walk stops at the first block with no in-loop
     successor and so measures a prefix.  */
  if (!lvx_unfoldable_loop_p (loop, body, insns, lvx_unfold_max_blocks,
			      lvx_unfold_max_insns, &why))
    {
      if (dump_file)
	fprintf (dump_file, ";; lvx unfold: loop %d declined, %s\n",
		 loop->num, why);
      free (body);
      return false;
    }

  if (!lvx_loop_profitable_to_unfold_p (loop, body, insns))
    {
      free (body);
      return false;
    }
  free (body);

  /* Every copy keeps its exits, which is what makes the trip count
     irrelevant.  */
  auto_sbitmap wont_exit (2);
  bitmap_clear (wont_exit);

  ok = duplicate_loop_body_to_header_edge (loop, loop_latch_edge (loop), 1,
					   wont_exit, NULL, NULL,
					   DLTHE_FLAG_UPDATE_FREQ);
  if (ok && dump_file)
    fprintf (dump_file, ";; lvx unfold: loop %d doubled, now %u blocks,"
	     " %u insns before\n", loop->num, loop->num_nodes, insns);
  return ok;
}

/* Give every innermost loop a single-entry preheader and lay its body out as
   one run, so that EBB formation sees the body whole and the software
   pipeliner's loop selection accepts it.

   This runs before reorg_loops rather than next to schedule_ebbs: a layout
   change after reorg_loops would invalidate LOOPDO, whose end address is a
   17-bit PC-relative immediate over a contiguous body, and after
   lvx_fix_pcreljump_ranges would invalidate branch widths already chosen by
   distance.  A contiguous body is what LOOPDO wants, so the two agree.  */
static void
lvx_superblock_inner_loops (void)
{
  basic_block bb;
  bool changed = false;

  /* cfg_layout_initialize asserts on this combination, as reorder_loops
     (hw-doloop.cc) documents for the same splice.  */
  if (crtl->has_bb_partition)
    return;

  cfg_layout_initialize (0);
  loop_optimizer_init (AVOID_CFG_MODIFICATIONS);

  /* Phase A: a single-entry preheader able to fall through to the header.
     7.3% of innermost loops have a header with two or more out-of-loop
     predecessors and are refused outright by the pipeliner for it.
     create_preheader needs loop->latch, which a loop with several latches
     has not got -- and such a loop is refused anyway, for the back edges.  */
  for (auto loop : loops_list (cfun, LI_ONLY_INNERMOST))
    if (loop->latch != NULL
	&& create_preheader (loop, CP_SIMPLE_PREHEADERS) != NULL)
      changed = true;

  /* Phase B: unfold, so the body the layout then gathers is twice as long.
     duplicate_loop_body_to_header_edge reads the dominator tree through
     get_loop_body_in_dom_order, and at this point in the pipeline that tree
     can be marked available while being stale -- which makes
     calculate_dominance_info a no-op and the read a segfault.  Force it.  */
  if (lvx_unfold_inner_loops)
    {
      free_dominance_info (CDI_DOMINATORS);
      calculate_dominance_info (CDI_DOMINATORS);
      /* The profitability test reads df_get_live_in to find what a branch
	 implicitly uses on its exit edge, so the liveness problem has to be
	 solved before it, not only after the layout.  */
      df_analyze ();
      /* And it asks the DFA for bypassed latencies, which needs the insn-code
	 cache the scheduler would otherwise own.  */
      dfa_start ();
      for (auto loop : loops_list (cfun, LI_ONLY_INNERMOST))
	if (lvx_unfold_inner_loop (loop))
	  changed = true;
      dfa_finish ();
    }

  /* Phase C: the layout.  */
  for (auto loop : loops_list (cfun, LI_ONLY_INNERMOST))
    if (lvx_superblock_layout_loop (loop))
      changed = true;

  loop_optimizer_finalize ();

  FOR_EACH_BB_FN (bb, cfun)
    bb->aux = (bb->next_bb != EXIT_BLOCK_PTR_FOR_FN (cfun)
	       ? bb->next_bb : NULL);
  cfg_layout_finalize ();
  clear_aux_for_blocks ();

  if (changed)
    {
      compute_bb_for_insn ();
      df_analyze ();
    }
}

/* Superblock formation for inner loops }}} */

/* The blocks regrename must see for LOOP: its body, what its blocks leave to,
   and its entry block when everything that block reaches is already in the
   set.  Shaped after unfold_inner_fill_rename_blocks -- the surroundings have
   to be there so a renamed value's liveness past the loop is accounted for.  */
static void
lvx_regrename_fill_blocks (class loop *loop, bitmap blocks)
{
  basic_block *body = get_loop_body (loop);
  basic_block pre = NULL;
  unsigned nentry = 0;
  edge e;
  edge_iterator ei;

  for (unsigned i = 0; i < loop->num_nodes; i++)
    {
      bitmap_set_bit (blocks, body[i]->index);
      FOR_EACH_EDGE (e, ei, body[i]->succs)
	if (e->dest != EXIT_BLOCK_PTR_FOR_FN (cfun))
	  bitmap_set_bit (blocks, e->dest->index);
    }

  FOR_EACH_EDGE (e, ei, loop->header->preds)
    if (!flow_bb_inside_loop_p (loop, e->src))
      {
	pre = e->src;
	nentry++;
      }

  if (nentry == 1 && pre != ENTRY_BLOCK_PTR_FOR_FN (cfun)
      && single_pred_p (pre))
    {
      unsigned inside = 0;
      FOR_EACH_EDGE (e, ei, pre->succs)
	if (bitmap_bit_p (blocks, e->dest->index))
	  inside++;
      if (inside == EDGE_COUNT (pre->succs))
	bitmap_set_bit (blocks, pre->index);
    }

  free (body);
}

/* Rename hard registers through one innermost LOOP so that successive
   definitions land on different registers, removing the anti- and output
   dependences that otherwise serialise an unfolded body's two copies against
   each other.

   The policy is the KVX reference's: take the LEAST RECENTLY DEFINED available
   register, so the distance back to the previous definition of the one chosen
   is as long as the set allows.  find_rename_reg is told not to apply its own
   preference, because its tick array is static to regrename.cc and cannot be
   seeded from outside -- which is also why rename_chains cannot be reused, it
   being static as well.  Left unseeded, that array is all zeroes for an
   external caller and the choice degrades to the lowest-numbered free
   register, which would recreate the very dependences this removes.

   Two deliberate divergences from the reference, both in the safe direction.
   The chain walk is over the loop body only.  And the reference's
   `unavail_regs &= ~live_in_regs' line, which MAKES the preheader's live-in
   registers available, is not ported: its own comment says those should be
   excluded, so the polarity is at best unclear, and omitting it leaves
   availability narrower and relies on regrename's own liveness analysis
   exactly as the generic rename_chains does.  */
static void
lvx_regrename_one_loop (class loop *loop, HARD_REG_SET fixed_unavail,
			int *ticks, int *tick)
{
  auto_bitmap blocks;
  basic_block *body = get_loop_body (loop);
  basic_block *chain = XNEWVEC (basic_block, loop->num_nodes);
  basic_block walk = loop->header;
  auto_bitmap inloop;
  unsigned n = 0;

  for (unsigned i = 0; i < loop->num_nodes; i++)
    bitmap_set_bit (inloop, body[i]->index);

  /* The body in chain order from the header.  The gate has established that
     each block has exactly one in-loop successor, so this covers it.  */
  for (n = 0; n < loop->num_nodes; n++)
    {
      edge e;
      edge_iterator ei;
      basic_block nxt = NULL;

      chain[n] = walk;
      FOR_EACH_EDGE (e, ei, walk->succs)
	if (bitmap_bit_p (inloop, e->dest->index))
	  nxt = e->dest;
      if (nxt == NULL)
	break;
      walk = nxt;
    }

  lvx_regrename_fill_blocks (loop, blocks);
  regrename_init (true);
  regrename_analyze (blocks);

  for (unsigned b = 0; b < n; b++)
    {
      rtx_insn *insn;

      FOR_BB_INSNS (chain[b], insn)
	{
	  operand_rr_info *op_info;
	  int latency;

	  if (!NONDEBUG_INSN_P (insn)
	      || GET_CODE (PATTERN (insn)) == USE
	      || GET_CODE (PATTERN (insn)) == CLOBBER
	      || volatile_insn_p (PATTERN (insn))
	      || (unsigned) INSN_UID (insn) >= insn_rr.length ())
	    continue;

	  op_info = insn_rr[INSN_UID (insn)].op_info;
	  if (op_info == NULL || op_info->failed)
	    continue;

	  latency = insn_default_latency (insn);
	  if (latency < 0)
	    latency = 0;
	  (*tick)++;

	  for (int i = 0; i < op_info->n_chains; i++)
	    {
	      du_head_p head = op_info->heads[i];
	      unsigned regno, chain_regno;
	      machine_mode mode;
	      HARD_REG_SET unavail, avail;
	      hard_reg_set_iterator hrsi;
	      reg_class super_class;
	      int n_uses = 0, min_tick, new_regno;

	      if (head == NULL)
		continue;
	      if (head->first == NULL)
		head = regrename_chain_from_id (head->id);
	      if (head == NULL || head->first == NULL
		  || head->first->loc == NULL || *head->first->loc == NULL_RTX)
		continue;

	      chain_regno = head->regno;
	      if (chain_regno >= FIRST_PSEUDO_REGISTER
		  || head->nregs <= 0
		  || chain_regno + head->nregs > FIRST_PSEUDO_REGISTER)
		continue;

	      mode = GET_MODE (*head->first->loc);

	      if (head->cannot_rename || head->renamed
		  || fixed_regs[chain_regno] || global_regs[chain_regno])
		{
		  for (int j = 0; j < head->nregs; j++)
		    ticks[chain_regno + j] = *tick + latency;
		  continue;
		}

	      unavail = fixed_unavail;
	      super_class = regrename_find_superclass (head, &n_uses, &unavail);
	      if (n_uses < 2)
		{
		  for (int j = 0; j < head->nregs; j++)
		    ticks[chain_regno + j] = *tick + latency;
		  continue;
		}

	      min_tick = ticks[chain_regno];
	      avail = ~unavail;
	      EXECUTE_IF_SET_IN_HARD_REG_SET (avail, 0, regno, hrsi)
		if (regno < FIRST_PSEUDO_REGISTER
		    && targetm.hard_regno_mode_ok (regno, mode)
		    && min_tick > ticks[regno])
		  min_tick = ticks[regno];

	      new_regno = chain_regno;
	      for (int step = 0; min_tick < ticks[chain_regno];
		   min_tick += step)
		{
		  HARD_REG_SET try_unavail = ~avail;

		  EXECUTE_IF_SET_IN_HARD_REG_SET (avail, 0, regno, hrsi)
		    if (regno < FIRST_PSEUDO_REGISTER
			&& ticks[regno] > min_tick
			&& targetm.hard_regno_mode_ok (regno, mode)
			&& end_hard_regno (mode, regno) <= FIRST_PSEUDO_REGISTER)
		      add_to_hard_reg_set (&try_unavail, mode, regno);

		  new_regno = find_rename_reg (head, super_class, &try_unavail,
					       chain_regno, false);
		  if (new_regno != (int) chain_regno)
		    break;

		  step = (ticks[chain_regno] - min_tick + 7) / 8;
		  if (step <= 0)
		    break;
		}

	      if (new_regno != (int) chain_regno
		  && new_regno >= 0
		  && new_regno + head->nregs <= FIRST_PSEUDO_REGISTER
		  && regrename_do_replace (head, new_regno))
		{
		  if (dump_file)
		    fprintf (dump_file, ";; lvx regrename: loop %d, %s -> %s"
			     " (insn %d, %d uses)\n", loop->num,
			     reg_names[chain_regno], reg_names[new_regno],
			     INSN_UID (insn), n_uses);
		  chain_regno = new_regno;
		}

	      for (int j = 0; j < head->nregs
			      && chain_regno + j < FIRST_PSEUDO_REGISTER; j++)
		ticks[chain_regno + j] = *tick + latency;
	    }
	}
    }

  regrename_finish ();
  free (chain);
  free (body);
}

/* Drive the renaming over the innermost loops, one loop at a time as the KVX
   reference does -- a single analysis over the union of several loops would
   let regrename_analyze merge chains across unrelated regions.  */
static void
lvx_regrename_inner_loops (void)
{
  static int ticks[FIRST_PSEUDO_REGISTER];
  HARD_REG_SET fixed_unavail;
  int tick = 0;

  /* The reference's set, which is stricter than rename_chains': the frame
     pointer unconditionally, and every fixed or global register.  */
  CLEAR_HARD_REG_SET (fixed_unavail);
  add_to_hard_reg_set (&fixed_unavail, Pmode, FRAME_POINTER_REGNUM);
  if (!HARD_FRAME_POINTER_IS_FRAME_POINTER)
    add_to_hard_reg_set (&fixed_unavail, Pmode, HARD_FRAME_POINTER_REGNUM);
  for (unsigned regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++)
    if (fixed_regs[regno] || global_regs[regno])
      SET_HARD_REG_BIT (fixed_unavail, regno);

  memset (ticks, 0, sizeof (ticks));

  loop_optimizer_init (AVOID_CFG_MODIFICATIONS);

  /* Twice the unfold phase's limits: an unfolded body is twice the size of
     the one that phase accepted.  */
  for (auto loop : loops_list (cfun, LI_ONLY_INNERMOST))
    if (lvx_inner_superblock_loop_p (loop, 2 * lvx_unfold_max_blocks,
				     2 * lvx_unfold_max_insns))
      lvx_regrename_one_loop (loop, fixed_unavail, ticks, &tick);

  loop_optimizer_finalize ();
}

#if CHECKING_P
namespace selftest {
extern void lvx_plc_cc_tests (void);
extern void lvx_network_cc_tests (void);

/* Implements TARGET_RUN_TARGET_SELFTESTS.  */

static void
lvx_run_selftests (void)
{
  lvx_plc_cc_tests ();
  lvx_network_cc_tests ();
}

} /* namespace selftest */

#undef TARGET_RUN_TARGET_SELFTESTS
#define TARGET_RUN_TARGET_SELFTESTS selftest::lvx_run_selftests
#endif /* CHECKING_P */

/* Implements TARGET_MACHINE_DEPENDENT_REORG.  */
static void
lvx_machine_dependent_reorg (void)
{
  compute_bb_for_insn ();

  /* Superblock formation for inner loops.  Speed only: it trades layout and
     a forwarder block or two for scheduling freedom, which is not what -Os
     is asking for.  */
  if (optimize && lvx_superblock_loops && optimize_function_for_speed_p (cfun))
    lvx_superblock_inner_loops ();

  /* The CMS measurement, on its own walk.  Before reorg_loops, because
     lvx_loop_has_doloop_p recognises the doloop_end pattern that LOOPDO
     generation then consumes -- after it, a hardware loop is indistinguishable
     from any other.  */
  if (optimize && lvx_cms_probe && optimize_function_for_speed_p (cfun))
    lvx_cms_probe_loops ();

  /* Doloop optimization. */
  if (optimize)
    reorg_loops (true, &lvx_doloop_hooks);

  lvx_fix_pcreljump_ranges ();

  df_analyze ();

  /* Renaming through the inner loops, immediately before the regions are
     formed: it changes no layout, so unlike the superblock pass it belongs
     here, where the schedule it frees up is about to be built.  */
  if (optimize && lvx_regrename_inner_loops_p
      && optimize_function_for_speed_p (cfun))
    {
      lvx_regrename_inner_loops ();
      df_analyze ();
    }

  bool selective_scheduling2 = false;
  if (optimize && flag_schedule_insns_after_reload)
    {
      timevar_push (TV_SCHED2);

      lvx_sched2_ctor (get_max_uid () + 1);
      if (flag_selective_scheduling2 && !maybe_skip_selective_scheduling ())
	{
	  selective_scheduling2 = true;
	  run_selective_scheduling ();
	}
      else
	schedule_ebbs ();

      timevar_pop (TV_SCHED2);
    }

  if (lvx_sched2->max_uid)
    lvx_fix_debug_for_bundles (selective_scheduling2);

  /* This is needed. Else final pass will crash on debug_insn-s */
  if (flag_var_tracking)
    {
      compute_bb_for_insn ();
      timevar_push (TV_VAR_TRACKING);
      variable_tracking_main ();
      timevar_pop (TV_VAR_TRACKING);
    }

  df_finish_pass (false);
}

#undef TARGET_MACHINE_DEPENDENT_REORG
#define TARGET_MACHINE_DEPENDENT_REORG lvx_machine_dependent_reorg

/* Misc. (builtins) {{{ */
void lvx_init_builtins (void);
#undef TARGET_INIT_BUILTINS
#define TARGET_INIT_BUILTINS lvx_init_builtins

tree lvx_builtin_decl (unsigned code, bool initialize_p);
#undef TARGET_BUILTIN_DECL
#define TARGET_BUILTIN_DECL lvx_builtin_decl

rtx lvx_expand_builtin (tree exp, rtx target, rtx subtarget,
			enum machine_mode mode, int ignore);
#undef TARGET_EXPAND_BUILTIN
#define TARGET_EXPAND_BUILTIN lvx_expand_builtin

tree lvx_resolve_overloaded_builtin (location_t loc, tree ob_fndecl,
				     void *passed_arglist, bool complain);
#undef TARGET_RESOLVE_OVERLOADED_BUILTIN
#define TARGET_RESOLVE_OVERLOADED_BUILTIN lvx_resolve_overloaded_builtin

/* Misc. (builtins) }}} */

/* Misc. (doloop) {{{ */

#if 0
static bool
lvx_predict_doloop_p (struct loop *loop)
{
  if (loop->inner != NULL)
    return false;

  return true;
}

#undef TARGET_PREDICT_DOLOOP
#define TARGET_PREDICT_DOLOOP_P lvx_predict_doloop_p
#endif

#undef TARGET_CAN_USE_DOLOOP_P
#define TARGET_CAN_USE_DOLOOP_P can_use_doloop_if_innermost

/* NULL if INSN insn is valid within a low-overhead loop.
   Otherwise return why doloop cannot be applied.  */

static const char *
lvx_invalid_within_doloop (const rtx_insn *insn)
{
  if (CALL_P (insn))
    return "Function call in the loop.";

  if (JUMP_TABLE_DATA_P (insn))
    return "Computed branch in the loop.";

  if (!INSN_P (insn))
    return NULL;

  rtx body = PATTERN (insn);
  rtx asm_ops = extract_asm_operands (body);

  if (asm_ops && GET_CODE (body) == PARALLEL)
    {
      int i, regno, nparallel = XVECLEN (body, 0);

      for (i = 0; i < nparallel; i++)
	{
	  rtx clobber = XVECEXP (body, 0, i);
	  if (GET_CODE (clobber) == CLOBBER && REG_P (XEXP (clobber, 0))
	      && (regno = REGNO (XEXP (clobber, 0)))
	      && (regno == LVX_LC_REGNO || regno == LVX_LS_REGNO
		  || regno == LVX_LE_REGNO))
	    return "HW Loop register clobbered by asm.";
	}
    }

  return NULL;
}

#undef TARGET_INVALID_WITHIN_DOLOOP
#define TARGET_INVALID_WITHIN_DOLOOP lvx_invalid_within_doloop

/* Implements TARGET_PREFERRED_DOLOOP_MODE.  */
static machine_mode
lvx_preferred_doloop_mode (machine_mode mode ATTRIBUTE_UNUSED)
{
  return DImode;
}

#undef TARGET_PREFERRED_DOLOOP_MODE
#define TARGET_PREFERRED_DOLOOP_MODE lvx_preferred_doloop_mode

/* Misc. (doloop) }}} */

/* Implements TARGET_HAVE_CONDITIONAL_EXECUTION.  */
static bool
lvx_have_conditional_execution (void)
{
  // Enable NOCE before combine and COND_EXEC after reload.
  return reload_completed;
}

#undef TARGET_HAVE_CONDITIONAL_EXECUTION
#define TARGET_HAVE_CONDITIONAL_EXECUTION lvx_have_conditional_execution

static bool
lvx_atomic_insn_p (rtx_insn *insn)
{
  int icode = -1;
  if (GET_CODE (insn) != INSN || (icode = INSN_CODE (insn)) < 0)
    return false;

  switch (icode)
    {
      default:
	return false;
      case CODE_FOR_lvx_alw:
      case CODE_FOR_lvx_ald:
      case CODE_FOR_lvx_asw:
      case CODE_FOR_lvx_asd:
      case CODE_FOR_lvx_fence:
      case CODE_FOR_alclrw_2:
      case CODE_FOR_alclrd_2:
      case CODE_FOR_aladdw_2:
      case CODE_FOR_aladdd_2:
      case CODE_FOR_acswapw_2:
      case CODE_FOR_acswapd_2:
      case CODE_FOR_acswapq_2:
      case CODE_FOR_lvx_aladdw:
      case CODE_FOR_lvx_aladdd:
      case CODE_FOR_lvx_alclrw:
      case CODE_FOR_lvx_alclrd:
      case CODE_FOR_atomic_compare_and_swapqi:
      case CODE_FOR_atomic_compare_and_swaphi:
      case CODE_FOR_atomic_compare_and_swapsi:
      case CODE_FOR_atomic_compare_and_swapdi:
      case CODE_FOR_atomic_loadqi:
      case CODE_FOR_atomic_loadhi:
      case CODE_FOR_atomic_loadsi:
      case CODE_FOR_atomic_loaddi:
      case CODE_FOR_atomic_loadti:
      case CODE_FOR_atomic_storeqi:
      case CODE_FOR_atomic_storehi:
      case CODE_FOR_atomic_storesi:
      case CODE_FOR_atomic_storedi:
      case CODE_FOR_atomic_storeti:
      case CODE_FOR_atomic_exchangesi:
      case CODE_FOR_atomic_exchangedi:
      case CODE_FOR_atomic_addsi:
      case CODE_FOR_atomic_orsi:
      case CODE_FOR_atomic_xorsi:
      case CODE_FOR_atomic_subsi:
      case CODE_FOR_atomic_andsi:
      case CODE_FOR_atomic_nandsi:
      case CODE_FOR_atomic_adddi:
      case CODE_FOR_atomic_ordi:
      case CODE_FOR_atomic_xordi:
      case CODE_FOR_atomic_subdi:
      case CODE_FOR_atomic_anddi:
      case CODE_FOR_atomic_nanddi:
      case CODE_FOR_atomic_fetch_addsi:
      case CODE_FOR_atomic_fetch_orsi:
      case CODE_FOR_atomic_fetch_xorsi:
      case CODE_FOR_atomic_fetch_subsi:
      case CODE_FOR_atomic_fetch_andsi:
      case CODE_FOR_atomic_fetch_nandsi:
      case CODE_FOR_atomic_fetch_adddi:
      case CODE_FOR_atomic_fetch_ordi:
      case CODE_FOR_atomic_fetch_xordi:
      case CODE_FOR_atomic_fetch_subdi:
      case CODE_FOR_atomic_fetch_anddi:
      case CODE_FOR_atomic_fetch_nanddi:
      case CODE_FOR_atomic_add_fetchsi:
      case CODE_FOR_atomic_or_fetchsi:
      case CODE_FOR_atomic_xor_fetchsi:
      case CODE_FOR_atomic_sub_fetchsi:
      case CODE_FOR_atomic_and_fetchsi:
      case CODE_FOR_atomic_nand_fetchsi:
      case CODE_FOR_atomic_add_fetchdi:
      case CODE_FOR_atomic_or_fetchdi:
      case CODE_FOR_atomic_xor_fetchdi:
      case CODE_FOR_atomic_sub_fetchdi:
      case CODE_FOR_atomic_and_fetchdi:
      case CODE_FOR_atomic_nand_fetchdi:
      case CODE_FOR_atomic_test_and_set:
	return true;
    }
}

/* Implements TARGET_LOOP_UNROLL_ADJUST.  */
static unsigned
lvx_loop_unroll_adjust (unsigned nunroll, struct loop *loop)
{
  class niter_desc *desc = get_simple_loop_desc (loop);
  bool unroll_stupid_p = !desc->simple_p || desc->assumptions;
  bool unroll_runtime_p = !unroll_stupid_p && !desc->const_iter;
  bool unroll_constant_p = !unroll_stupid_p && desc->const_iter;

  const int max_ninsns = 512;
  const int max_full_bundles = 16;
  static struct lvx_sched_resources resources;
  unsigned max_clz = sizeof (unsigned) * CHAR_BIT - 1;

  // Memoize the loop body analysis in LOOP->AUX.
  if (!loop->aux)
    {
      memset (&resources, 0, sizeof (resources));

      basic_block *body = get_loop_body (loop);
      for (unsigned i = 0; i < loop->num_nodes; i++)
	{
	  rtx_insn *insn = 0;
	  FOR_BB_INSNS (body[i], insn)
	    {
	      lvx_sched_resources_add (&resources, insn);
	      if (resources.insn_count > max_ninsns)
		resources.message = "Too many instructions in loop.";
	      if (CALL_P (insn))
		continue;
	      resources.message = lvx_invalid_within_doloop (insn);
	      if (resources.message)
		break;
	      if (lvx_atomic_insn_p (insn))
		{
		  resources.message = "no unroll (atomic insn found)";
		  break;
		}
	    }
	  if (resources.message)
	    break;
	}
      free (body);
      loop->aux = &resources;
    } else
      gcc_assert (loop->aux == &resources);

    if (resources.message)
      {
	if (dump_file)
	  fprintf (dump_file, "lvx_loop_unroll_adjust: %s\n",
		   resources.message);
	return 0;
      }

  // Force UNROLLING as a power-of-two, with the resulting number of
  // insns not greater than MAX_NINSNS. In case of PRAGMA GCC UNROLL,
  // the user-supplied unroll factor is passed into LOOP->UNROLL.
  if (!unroll_constant_p)
    {
      if (loop->unroll)
	nunroll = loop->unroll;
      unsigned log2_nunroll = (max_clz - __builtin_clz (nunroll | 1));
      unsigned max_nunroll = max_ninsns >> log2_nunroll;
      nunroll = 1U << log2_nunroll;
      if (nunroll > max_nunroll)
	nunroll = max_nunroll;
      gcc_assert (nunroll == (nunroll & -nunroll));
    }

  // Force unrolling of non-constant, non-runtime loop trip count
  // by setting LOOP->LPT_DECISION.DECISION to LPT_UNROLL_STUPID.
  // This an abuse of the (loop-unroll.cc:decide_unroll_stupid) code
  // which normally rejects unrolling of loops with branches inside.
  if (unroll_stupid_p)
    {
      if (!loop->unroll)
	{
	  unsigned full_bundles =
	    lvx_sched_resources_full_bundles (&resources);
	  full_bundles = full_bundles ? full_bundles : 1;
	  nunroll = (max_full_bundles + full_bundles - 1) / full_bundles;
	}
      nunroll = 1U << (max_clz - __builtin_clz (nunroll | 1));
      gcc_assert (nunroll == (nunroll & -nunroll));

      if (nunroll > 1)
	{
	  if (dump_file)
	    fprintf (dump_file, "lvx_loop_unroll_adjust: "
		     "Force LPT_UNROLL_STUPID %d iterations.\n", nunroll);

	  loop->lpt_decision.decision = LPT_UNROLL_STUPID;
	  loop->lpt_decision.times = nunroll - 1;
	}
    }

  if (dump_file)
    fprintf (dump_file, "lvx_loop_unroll_adjust: "
	     "Return nunroll=%d%s\n.", nunroll,
	     (unroll_constant_p ? " [unroll_constant]" :
	      (unroll_runtime_p ? " [unroll_runtime]" :
	       (unroll_stupid_p ? " [unroll_stupid]" : ""))));

  return nunroll;
}

#undef TARGET_LOOP_UNROLL_ADJUST
#define TARGET_LOOP_UNROLL_ADJUST lvx_loop_unroll_adjust

/* 18.31 Miscellaneous Parameters }}} */


/* 18.xx Complex Number Support {{{ */

/* These complex-number target hooks are local extensions not in standard GCC.
   Define compatibility shims so the functions compile (they are dead code
   unless the hooks are also added to target.def / targhooks).  */
typedef int complex_part_t;
#define REAL_P ((complex_part_t) 0)
#define IMAG_P ((complex_part_t) 1)
#define BOTH_P ((complex_part_t) 2)
#define TREE_COMPLEX_BOTH_PARTS(x) (x)

static rtx
lvx_gen_rtx_complex (machine_mode mode, rtx real_part, rtx imag_part)
{
  machine_mode imode = GET_MODE_INNER (mode);

  /* Still use concat for unitialized registers.  */
  if (real_part && imag_part
      && ((CONST_INT_P (real_part) && CONST_INT_P (imag_part))
	  || (CONST_DOUBLE_P (real_part) && CONST_DOUBLE_P (imag_part))))
    return gen_rtx_CONCAT (mode, real_part, imag_part);

  bool saved_generating_concat_p = generating_concat_p;
  generating_concat_p = false;
  rtx complex_reg = gen_reg_rtx (mode);
  generating_concat_p = saved_generating_concat_p;

  if (real_part)
    {
      gcc_assert (imode == GET_MODE (real_part));
      write_complex_part (complex_reg, real_part, REAL_P, true);
    }

  if (imag_part)
    {
      gcc_assert (imode == GET_MODE (imag_part));
      write_complex_part (complex_reg, imag_part, IMAG_P, false);
    }

  return complex_reg;
}

#undef TARGET_GEN_RTX_COMPLEX
#define TARGET_GEN_RTX_COMPLEX lvx_gen_rtx_complex

rtx
lvx_read_complex_part (rtx cplx, complex_part_t part)
{
  machine_mode cmode;
  scalar_mode imode;
  unsigned ibitsize;

  if (GET_CODE (cplx) == CONCAT)
    return (part == BOTH_P) ? cplx : XEXP (cplx, part);

  cmode = GET_MODE (cplx);
  imode = GET_MODE_INNER (cmode);
  ibitsize = GET_MODE_BITSIZE (imode);

  if (COMPLEX_MODE_P (cmode) && (part == BOTH_P))
    return cplx;

  /* For constants under 32-bit vector constans are folded during expand,
     so we need to compensate for it as cplx is an integer constant
     In this case cmode and imode are equal.  */
  if (cmode == imode)
    ibitsize /= 2;

  /* Cannot get a part of a VOIDmode element.  */
  if (cmode == E_VOIDmode)
    return cplx;

  /* Special case reads from complex constants that got spilled to memory.  */
  if (MEM_P (cplx) && GET_CODE (XEXP (cplx, 0)) == SYMBOL_REF)
    {
      tree decl = SYMBOL_REF_DECL (XEXP (cplx, 0));
      if (decl && TREE_CODE (decl) == COMPLEX_CST)
	{
	  tree cplx_part = (part == IMAG_P) ? TREE_IMAGPART (decl)
	    : (part == REAL_P) ? TREE_REALPART (decl)
	    : TREE_COMPLEX_BOTH_PARTS (decl);
	  if (CONSTANT_CLASS_P (cplx_part))
	    return expand_expr (cplx_part, NULL_RTX, imode, EXPAND_NORMAL);
	}
    }

  /* For MEMs simplify_gen_subreg may generate an invalid new address
     because, e.g., the original address is considered mode-dependent
     by the target, which restricts simplify_subreg from invoking
     adjust_address_nv.  Instead of preparing fallback support for an
     invalid address, we call adjust_address_nv directly.  */
  if (MEM_P (cplx))
    {
      if (part == BOTH_P)
	return adjust_address_nv (cplx, cmode, 0);
      else
	return adjust_address_nv (cplx, imode, (part == IMAG_P)
				  ? GET_MODE_SIZE (imode) : 0);
    }

  /* If the sub-object is at least word sized, then we know that subregging
     will work.  This special case is important, since extract_bit_field
     wants to operate on integer modes, and there's rarely an OImode to
     correspond to TCmode.  */
  if (ibitsize >= BITS_PER_WORD
      /* For hard regs we have exact predicates.  Assume we can split
         the original object if it spans an even number of hard regs.
         This special case is important for SCmode on 64-bit platforms
         where the natural size of floating-point regs is 32-bit.  */
      || (REG_P (cplx)
	  && REGNO (cplx) < FIRST_PSEUDO_REGISTER
	  && REG_NREGS (cplx) % 2 == 0))
    {
      rtx ret = simplify_gen_subreg (imode, cplx, cmode, (part == IMAG_P)
				     ? GET_MODE_SIZE (imode) : 0);
      if (ret)
	return ret;
      else
	/* simplify_gen_subreg may fail for sub-word MEMs.  */
	gcc_assert (MEM_P (cplx) && ibitsize < BITS_PER_WORD);
    }

  if (part == BOTH_P)
    return extract_bit_field (cplx, 2 * ibitsize, 0, true, NULL_RTX, cmode,
			      cmode, false, NULL);
  else
    return extract_bit_field (cplx, ibitsize, (part == IMAG_P) ? ibitsize : 0,
			      true, NULL_RTX, imode, imode, false, NULL);
}

#undef TARGET_READ_COMPLEX_PART
#define TARGET_READ_COMPLEX_PART lvx_read_complex_part

void
  lvx_write_complex_part (rtx cplx, rtx val, complex_part_t part,
			  bool undefined_p)
{
  machine_mode cmode, vmode;
  scalar_mode imode;
  unsigned ibitsize;

  cmode = GET_MODE (cplx);
  imode = GET_MODE_INNER (cmode);
  ibitsize = GET_MODE_BITSIZE (imode);

  vmode = cmode;
  if (!mode_for_vector (imode, 2).exists (&vmode) && (part == BOTH_P))
    {
      write_complex_part (cplx, read_complex_part (val, REAL_P), REAL_P, true);
      write_complex_part (cplx, read_complex_part (val, IMAG_P), IMAG_P, false);
      return;
    }

  /* special case for constants */
  if (GET_CODE (val) == CONST_VECTOR)
    {
      if (part == BOTH_P)
	{
	  if (vmode != E_BLKmode)
	    {
	      rtx temp_reg = gen_reg_rtx (vmode);
	      store_bit_field (temp_reg, GET_MODE_BITSIZE (vmode), 0, 0,
			       0, GET_MODE (val), val, false, undefined_p);
	      emit_move_insn (cplx,
			      simplify_gen_subreg (cmode, temp_reg, vmode,
						   0));
	    }
	  else
	    {
	      /* write real part and imag part separately */
	      write_complex_part (cplx, const_vector_elt (val, 0), REAL_P, true);
	      write_complex_part (cplx, const_vector_elt (val, 1), IMAG_P, false);
	    }
	}
      else
	write_complex_part (cplx,
			    const_vector_elt (val,
					      ((part == REAL_P) ? 0 : 1)),
			    part, true);
      return;
    }

  if ((GET_CODE (val) == CONST_DOUBLE) || (GET_CODE (val) == CONST_INT))
    {
      if (part == REAL_P)
	{
	  emit_move_insn (gen_lowpart (imode, cplx), val);
	  return;
	}
      else if (part == IMAG_P)
	{
	  /* cannot set highpart of a pseudo register */
	  if (REG_P (cplx) && REGNO (cplx) < FIRST_PSEUDO_REGISTER)
	    {
	      emit_move_insn (gen_highpart (imode, cplx), val);
	      return;
	    }
	}
      else
	{
	  if (val != CONST0_RTX (cmode) && val != CONST0_RTX (imode))
	    gcc_unreachable ();

	  write_complex_part (cplx, CONST0_RTX (imode), REAL_P, true);
	  write_complex_part (cplx, CONST0_RTX (imode), IMAG_P, false);
	  return;
	}
    }

  if ((part == BOTH_P) && !MEM_P (cplx))
    {
      /* Real and imag parts can be passed in a CONCAT during the expand of a
         COMPLEX_EXPR.  */
      if (GET_CODE (val) == CONCAT)
	{
	  emit_move_insn (simplify_gen_subreg (imode, cplx, cmode, 0),
			  XEXP (val, 0));
	  write_complex_part (cplx, XEXP (val, 1), IMAG_P, false);
	}
      else
	{
	  rtx vec_val = simplify_gen_subreg (vmode, val, GET_MODE (val), 0);
	  rtx vec_cplx = simplify_gen_subreg (vmode, cplx, cmode, 0);
	  emit_move_insn (vec_cplx, vec_val);
	}
      return;
    }

  if (GET_CODE (cplx) == CONCAT)
    {
      emit_move_insn (XEXP (cplx, part), val);
      return;
    }

  /* For MEMs simplify_gen_subreg may generate an invalid new address
     because, e.g., the original address is considered mode-dependent
     by the target, which restricts simplify_subreg from invoking
     adjust_address_nv.  Instead of preparing fallback support for an
     invalid address, we call adjust_address_nv directly.  */
  if (MEM_P (cplx))
    {
      if (part == BOTH_P)
	{
	  /* Real and imag parts can be passed in a CONCAT during the expand of a
	     COMPLEX_EXPR.  */
	  if (GET_CODE (val) == CONCAT)
	    {
	      write_complex_part (cplx, XEXP (val, 0), REAL_P, true);
	      write_complex_part (cplx, XEXP (val, 1), IMAG_P, false);
	    }
	  else
	    {
	      rtx vec_val = (MEM_P (val)) ? adjust_address_nv (val, vmode, 0)
		: simplify_gen_subreg (vmode, val, GET_MODE (val), 0);
	      emit_move_insn (adjust_address_nv (cplx, vmode, 0), vec_val);
	    }
	}
      else
	emit_move_insn (adjust_address_nv (cplx, imode, (part == IMAG_P)
					   ? GET_MODE_SIZE (imode) : 0), val);
      return;
    }

  /* If the sub-object is at least word sized, then we know that subregging
     will work.  This special case is important, since store_bit_field
     wants to operate on integer modes, and there's rarely an OImode to
     correspond to TCmode.  */
  if (ibitsize >= BITS_PER_WORD
      /* For hard regs we have exact predicates.  Assume we can split
         the original object if it spans an even number of hard regs.
         This special case is important for SCmode on 64-bit platforms
         where the natural size of floating-point regs is 32-bit.  */
      || (REG_P (cplx)
	  && REGNO (cplx) < FIRST_PSEUDO_REGISTER
	  && REG_NREGS (cplx) % 2 == 0))
    {
      rtx cplx_part = simplify_gen_subreg (imode, cplx, cmode,
					   (part ==
					    IMAG_P) ? GET_MODE_SIZE (imode) :
					   0);
      if (cplx_part)
	{
	  emit_move_insn (cplx_part, val);
	  return;
	}
      else
	/* simplify_gen_subreg may fail for sub-word MEMs.  */
	gcc_assert (MEM_P (cplx) && ibitsize < BITS_PER_WORD);
    }

  store_bit_field (cplx, ibitsize, (part == IMAG_P) ? ibitsize : 0, 0, 0,
		   imode, val, false, undefined_p);
}

#undef TARGET_WRITE_COMPLEX_PART
#define TARGET_WRITE_COMPLEX_PART lvx_write_complex_part

/* 18.xx Complex Number Support }}} */

/* TLS Support {{{ */

/* This section is after the whole chapter 18 definition because it needs
   TARGET_HAS_TLS to be visible.  */

/* Returns 0 if there is no TLS ref, != 0 if there is.

  Beware that UNSPEC_TLS are not symbol ref, they are offset within
  TLS.
 */
int
lvx_has_tls_reference (rtx x)
{
  if (!TARGET_HAVE_TLS)
    return false;

  subrtx_iterator::array_type array;
  FOR_EACH_SUBRTX (iter, array, x, ALL)
    {
      const_rtx x = *iter;
      if (GET_CODE (x) == SYMBOL_REF && SYMBOL_REF_TLS_MODEL (x) != 0)
	return true;
      /* Don't recurse into UNSPEC_TLS looking for TLS symbols; these are
	 TLS offsets, not real symbol references.  */
      if (GET_CODE (x) == UNSPEC
	  && (XINT (x, 1) == UNSPEC_TLS_GD || XINT (x, 1) == UNSPEC_TLS_LD
	      || XINT (x, 1) == UNSPEC_TLS_DTPOFF
	      || XINT (x, 1) == UNSPEC_TLS_LE || XINT (x, 1) == UNSPEC_TLS_IE))
	iter.skip_subrtxes ();
    }
  return false;
}

static int
lvx_has_unspec_reference_1 (rtx *x)
{
  return (GET_CODE (*x) == UNSPEC
	  && (XINT (*x, 1) == UNSPEC_GOT || XINT (*x, 1) == UNSPEC_GOTOFF
	      || XINT (*x, 1) == UNSPEC_PCREL || XINT (*x, 1) == UNSPEC_TLS_GD
	      || XINT (*x, 1) == UNSPEC_TLS_LD
	      || XINT (*x, 1) == UNSPEC_TLS_LE
	      || XINT (*x, 1) == UNSPEC_TLS_IE));
}

static int
lvx_has_unspec_reference (rtx x)
{
  subrtx_ptr_iterator::array_type array;
  FOR_EACH_SUBRTX_PTR (iter, array, &x, ALL)
    {
      rtx *x = *iter;
      switch (lvx_has_unspec_reference_1 (x))
	{
	case -1:
	  iter.skip_subrtxes ();
	  break;
	case 0:
	  break;
	default:
	  return 1;
	}
    }
  return 0;
  //   return for_each_rtx (&x, &lvx_has_unspec_reference_1, NULL);
}


/* Return the TLS model to use for ADDR.  */

static enum tls_model
tls_symbolic_operand_type (rtx addr)
{
  enum tls_model tls_kind = TLS_MODEL_NONE;

  if (GET_CODE (addr) == CONST)
    {
      if (GET_CODE (XEXP (addr, 0)) == PLUS
	  && GET_CODE (XEXP (XEXP (addr, 0), 0)) == SYMBOL_REF)
	tls_kind = SYMBOL_REF_TLS_MODEL (XEXP (XEXP (addr, 0), 0));
    }
  else if (GET_CODE (addr) == SYMBOL_REF)
    tls_kind = SYMBOL_REF_TLS_MODEL (addr);

  /* General TLS model only supported on Linux. ClusterOS and bare
     only supports local exec. */

#ifndef GCC_LVX_MPPA_LINUX
  if (tls_kind != TLS_MODEL_NONE)
    tls_kind = TLS_MODEL_LOCAL_EXEC;
#endif
  return tls_kind;
}

/* Return true if SYMBOL_REF X is thread local */
static bool
lvx_tls_symbol_p (rtx x)
{
  if (!TARGET_HAVE_TLS)
    return false;

  if (GET_CODE (x) != SYMBOL_REF)
    return false;

  return SYMBOL_REF_TLS_MODEL (x) != 0;
}

static rtx
gen_set_gotp_insn (rtx target)
{
  return gen_set_gotp_di (target);
}

static enum lvx_symbol_type
lvx_classify_tls_symbol (rtx x)
{
  enum tls_model tls_kind = tls_symbolic_operand_type (x);
  switch (tls_kind)
    {
    case TLS_MODEL_LOCAL_EXEC:
      return SYMBOL_TLSLE;

      /* General TLS model only supported on Linux. ClusterOS and bare
         only supports local exec. */
#ifdef GCC_LVX_MPPA_LINUX
    case TLS_MODEL_LOCAL_DYNAMIC:
      return SYMBOL_TLSLD;

    case TLS_MODEL_GLOBAL_DYNAMIC:
      return SYMBOL_TLSGD;

    case TLS_MODEL_INITIAL_EXEC:
      return SYMBOL_TLSIE;
#else
    case TLS_MODEL_LOCAL_DYNAMIC:
    case TLS_MODEL_GLOBAL_DYNAMIC:
    case TLS_MODEL_INITIAL_EXEC:
#endif
    case TLS_MODEL_EMULATED:
    case TLS_MODEL_NONE:
      error ("TLS model not supported.");
      break;

    default:
      gcc_unreachable ();
    }
  return SYMBOL_UNKNOWN;
}

static enum lvx_symbol_type
lvx_classify_symbol (rtx x)
{
  if (lvx_tls_symbol_p (x))
    return lvx_classify_tls_symbol (x);

  /* We keep both way of materializing the absolute address of a label
     because the use of pcrel insn has greater constraints on bundling
     (ALU_FULL) versus a simple make (ALU_TINY) */
  if (GET_CODE (x) == LABEL_REF)
    return flag_pic ? LABEL_PCREL_ABSOLUTE : LABEL_ABSOLUTE;

  if (GET_CODE (x) == SYMBOL_REF)
    {
      if (!flag_pic)
	return SYMBOL_ABSOLUTE;

      if (SYMBOL_REF_LOCAL_P (x) && !SYMBOL_REF_EXTERNAL_P (x))
	return SYMBOL_GOTOFF;
      else
	return SYMBOL_GOT;
    }
  return SYMBOL_ABSOLUTE;
}

static GTY (()) rtx lvx_tls_symbol;

/* Return an instruction sequence that calls __tls_get_addr.  SYM is
   the TLS symbol we are referencing and TYPE is the symbol type to use
   (either global dynamic or local dynamic).  RESULT is an RTX for the
   return value location.  */

static rtx_insn *
lvx_call_tls_get_addr (rtx sym, rtx result, int unspec)
{
  rtx a0 = gen_rtx_REG (Pmode, LVX_ARGUMENT_POINTER_REGNO), func;
  rtx_insn *insn;

  if (!lvx_tls_symbol)
    lvx_tls_symbol = init_one_libfunc ("__tls_get_addr");
  func = gen_rtx_MEM (FUNCTION_MODE, lvx_tls_symbol);

  start_sequence ();

  /* sequence for getting TLS desc address from GOT slot:
   */
  rtx pic_reg = gen_reg_rtx (Pmode);
  emit_insn (gen_set_gotp_insn (pic_reg));

  rtx new_rtx
    = gen_rtx_CONST (Pmode, gen_rtx_UNSPEC (Pmode, gen_rtvec (1, sym), unspec));
  emit_insn (gen_add3_insn (a0, pic_reg, new_rtx));

  insn = emit_call_insn (gen_call_value (result, func, const0_rtx));
  RTL_CONST_CALL_P (insn) = 1;
  use_reg (&CALL_INSN_FUNCTION_USAGE (insn), a0);
  insn = get_insns ();

  end_sequence ();

  return insn;
}

static rtx
lvx_legitimize_tls_reference (rtx x)
{
  rtx addr, pic_reg, eqv;
  rtx dest, tmp;

  if (reload_completed || reload_in_progress)
    return x;

  enum lvx_symbol_type sty = lvx_classify_symbol (x);

  switch (sty)
    {
    case SYMBOL_TLSLE:
      /* address is @tlsle(symbol)[$tp]
       */
      addr
	= gen_rtx_PLUS (Pmode, gen_rtx_REG (Pmode, LVX_LOCAL_POINTER_REGNO),
			gen_rtx_CONST (Pmode,
				       gen_rtx_UNSPEC (Pmode,
						       gen_rtvec (1, x),
						       UNSPEC_TLS_LE)));
      return addr;

    case SYMBOL_TLSGD:
      /* Sequence is:
       * $r0 = @tlsgd(sym) + $got
       * addr = __tls_get_addr()
       */
      tmp = gen_rtx_REG (Pmode, LVX_ARGUMENT_POINTER_REGNO);
      dest = gen_reg_rtx (Pmode);
      emit_libcall_block (lvx_call_tls_get_addr (x, tmp, UNSPEC_TLS_GD),
			  dest /* target */ , tmp /* result */ ,
			  x /* equiv */ );
      return dest;

    case SYMBOL_TLSIE:
      pic_reg = gen_reg_rtx (Pmode);
      dest = gen_reg_rtx (Pmode);

      emit_insn (gen_set_gotp_insn (pic_reg));

      addr
	= gen_rtx_PLUS (Pmode, pic_reg,
			gen_rtx_CONST (Pmode,
				       gen_rtx_UNSPEC (Pmode,
						       gen_rtvec (1, x),
						       UNSPEC_TLS_IE)));
      emit_move_insn (dest, gen_rtx_MEM (Pmode, addr));

      return gen_rtx_PLUS (Pmode,
			   gen_rtx_REG (Pmode, LVX_LOCAL_POINTER_REGNO),
			   dest);

    case SYMBOL_TLSLD:
      tmp = gen_rtx_REG (Pmode, LVX_ARGUMENT_POINTER_REGNO);
      dest = gen_reg_rtx (Pmode);

      /* Attach a unique REG_EQUIV, to allow the RTL optimizers to
         share the LD result with other LD model accesses.  */
      eqv
	= gen_rtx_UNSPEC (Pmode, gen_rtvec (1, const0_rtx), UNSPEC_TLS_LD_CALL);

      emit_libcall_block (lvx_call_tls_get_addr (x, tmp, UNSPEC_TLS_LD),
			  dest /* target */ , tmp /* result */ ,
			  eqv /* equiv */ );

      addr
	= gen_rtx_PLUS (Pmode, dest,
			gen_rtx_CONST (Pmode,
				       gen_rtx_UNSPEC (Pmode,
						       gen_rtvec (1, x),
						       UNSPEC_TLS_DTPOFF)));

      return addr;

    case SYMBOL_UNKNOWN:
    default:
      gcc_unreachable ();
    }
}

__attribute__((unused))
static bool
function_symbol_referenced_p (rtx x)
{
  const char *format;
  int length;
  int j;

  if (GET_CODE (x) == SYMBOL_REF)
    return SYMBOL_REF_FUNCTION_P (x);

  length = GET_RTX_LENGTH (GET_CODE (x));
  format = GET_RTX_FORMAT (GET_CODE (x));

  for (j = 0; j < length; ++j)
    {
      switch (format[j])
	{
	case 'e':
	  if (function_symbol_referenced_p (XEXP (x, j)))
	    return TRUE;
	  break;

	case 'V':
	case 'E':
	  if (XVEC (x, j) != 0)
	    {
	      int k;
	      for (k = 0; k < XVECLEN (x, j); ++k)
		if (function_symbol_referenced_p (XVECEXP (x, j, k)))
		  return TRUE;
	    }
	  break;

	default:
	  /* Nothing to do.  */
	  break;
	}
    }

  return FALSE;
}

/* Returns TRUE if OP contains a symbol reference or a label reference */
bool
symbolic_reference_mentioned_p (rtx op)
{
  const char *fmt;
  int i;

  if (GET_CODE (op) == SYMBOL_REF || GET_CODE (op) == LABEL_REF)
    return TRUE;

  /* UNSPEC_TLS entries for a symbol include a LABEL_REF for the
     referencing instruction, but they are constant offsets, not
     symbols.  */
  if (GET_CODE (op) == UNSPEC
      && (XINT (op, 1) == UNSPEC_TLS_LE
	|| XINT (op, 1) == UNSPEC_TLS_LD
	|| XINT (op, 1) == UNSPEC_TLS_DTPOFF
	|| XINT (op, 1) == UNSPEC_TLS_GD
	|| XINT (op, 1) == UNSPEC_TLS_IE))
    return false;

  fmt = GET_RTX_FORMAT (GET_CODE (op));
  for (i = GET_RTX_LENGTH (GET_CODE (op)) - 1; i >= 0; i--)
    {
      if (fmt[i] == 'E')
	{
	  int j;

	  for (j = XVECLEN (op, i) - 1; j >= 0; j--)
	    if (symbolic_reference_mentioned_p (XVECEXP (op, i, j)))
	      return TRUE;
	}

      else if (fmt[i] == 'e' && symbolic_reference_mentioned_p (XEXP (op, i)))
	return TRUE;
    }

  return FALSE;
}

/* TLS Support }}} */

struct gcc_target targetm = TARGET_INITIALIZER;

/* 18 Target Description Macros and Functions }}} */

#include "gt-lvx.h"
