/*

   Copyright (C) 2009-2014 Kalray SA.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef LVX_PROTOS_H
#define LVX_PROTOS_H

void expand_builtin_trap (void);

void lvx_expand_builtin_maddt (rtx operands[], int add);
void lvx_expand_builtin_fmuldc (rtx operands[], int lanes);

#ifdef HAVE_ATTR_arch
extern enum attr_arch lvx_arch_schedule;
#endif /* HAVE_ATTR_arch */

extern bool lvx_is_farcall_p (rtx op);
extern bool lvx_jump_long_offset_p (rtx_insn *insn);

/* Pass management. */

extern rtl_opt_pass *make_pass_prologue_stack_limit (gcc::context *);
extern rtl_opt_pass *make_pass_lvx_shaker (gcc::context *);

/* lvx-shaker */

void lvx_final_prescan_insn (rtx_insn *insn);
const char * lvx_asm_output_opcode (FILE *stream, const char * code);

#ifdef RTX_CODE
#include "tree-pass.h"

extern void lvx_output_function_profiler (FILE *);
extern HOST_WIDE_INT lvx_first_parm_offset (tree decl);

extern void lvx_output_load_multiple (rtx *operands);

extern void lvx_expand_tablejump (rtx op0, rtx op1);

extern rtx lvx_return_addr_rtx (int count, rtx frameaddr);

extern void lvx_expand_prologue (void);

extern void lvx_expand_epilogue (void);

extern void lvx_emit_set_address (rtx dest, rtx address);
extern rtx lvx_get_mem_rtx (rtx x, int nth);
extern bool lvx_modifier_enabled_p (const char * mod, rtx x);
extern rtx lvx_modifier_rounding (rtx x);

extern void lvx_ifcvt_machdep_init (struct ce_if_block *, bool after_combine);
extern void lvx_ifcvt_modify_tests (ce_if_block *ce_info,  rtx true_expr, rtx false_expr,
				    rtx_insn *then_start, rtx_insn *then_end,
				    rtx_insn *else_start, rtx_insn *else_end);
extern rtx lvx_ifcvt_modify_insn (struct ce_if_block *, rtx, rtx_insn *);

extern void lvx_print_operand (FILE *file, rtx x, int code);

extern void lvx_print_operand_address (FILE *file, rtx x);

extern bool lvx_print_punct_valid_p (unsigned char code);

extern bool lvx_syscall_addrspace_p (rtx op);
extern bool lvx_is_uncached_mem_op_p (rtx op);

extern bool lvx_expand_load_multiple (rtx operands[]);
extern bool lvx_expand_store_multiple (rtx operands[]);

extern bool lvx_load_multiple_operation_p (rtx op, bool is_uncached);
extern bool lvx_expand_unpack (rtx op0, rtx op1, bool signed_p, bool hi_p);

extern bool lvx_store_multiple_operation_p (rtx op);

extern void lvx_init_expanders (void);

extern void lvx_init_cumulative_args (CUMULATIVE_ARGS *cum, const_tree fntype,
				      rtx libname, tree fndecl,
				      int n_named_args);

extern void lvx_expand_mov_constant (rtx operands[]);

extern bool lvx_legitimate_pic_operand_p (rtx x);

extern bool lvx_legitimate_pic_symbolic_ref_p (rtx op);

extern bool lvx_split_mem (rtx x, rtx *base_out, rtx *offset_out, bool strict);

extern bool lvx_pack_load_store (rtx operands[], int nops);

extern bool lvx_is_farcall_p (rtx);

extern bool lvx_expand_memset_mul (rtx *operands, machine_mode mode);

extern void lvx_override_options (void);

extern bool lvx_hardreg_aligned_p (rtx op, machine_mode mode);
extern void lvx_split_128bits_move (rtx dst, rtx src);
extern void lvx_split_256bits_move (rtx dst, rtx src);
extern void lvx_make_128bit_const (rtx dst, rtx src);
extern void lvx_make_256bit_const (rtx dst, rtx src);
extern void lvx_make_512bit_const (rtx dst, rtx src);

extern bool lvx_has_10bit_imm_or_reg_p (rtx x);
extern bool lvx_has_10bit_immediate_p (rtx x);
extern bool lvx_has_37bit_immediate_p (rtx x);
extern bool lvx_has_64bit_immediate_p (rtx x);
extern bool lvx_has_27bit_immediate_p (rtx x);
extern bool lvx_has_54bit_immediate_p (rtx x);

extern HOST_WIDE_INT lvx_const_vector_value (rtx x, int index);

extern bool lvx_has_10bit_vector_const_p (rtx x);

extern bool lvx_has_16bit_vector_const_p (rtx x);

extern bool lvx_has_32bit_vector_const_p (rtx x);

extern bool lvx_has_37bit_vector_const_p (rtx x);

extern bool lvx_has_43bit_vector_const_p (rtx x);

extern bool lvx_has_32x2bit_vector_const_p (rtx x);

extern enum machine_mode lvx_get_predicate_mode (enum machine_mode mode);

extern void lvx_lower_comparison (rtx pred, rtx comp, enum machine_mode mode);

extern void lvx_expand_conditional_move (rtx target, rtx select1, rtx select2, rtx cmp);

extern void lvx_expand_masked_move (rtx target, rtx select1, rtx select2,
				    rtx mask);

extern void lvx_expand_vector_insert (rtx target, rtx source, rtx where);

extern void lvx_expand_vector_extract (rtx target, rtx source, rtx where);

extern void lvx_expand_chunk_splat (rtx target, rtx source,
				    enum machine_mode inner_mode);

extern void lvx_expand_vector_init (rtx target, rtx source);

extern void lvx_expand_vector_duplicate (rtx target, rtx source);

extern bool lvx_expand_vec_perm_const (rtx target, rtx source1, rtx source2, rtx selector);

extern void lvx_expand_vector_shift (rtx target, rtx source, rtx chunk,
				     unsigned bits, int left);

extern void lvx_expand_any64_eqz (rtx target, rtx source, enum machine_mode mode);

extern void lvx_emit_pre_barrier (rtx);
extern void lvx_emit_post_barrier (rtx);
extern void lvx_expand_compare_and_swap (rtx, rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern void lvx_expand_atomic_op (enum rtx_code, rtx, bool, rtx, rtx, rtx);
extern void lvx_expand_atomic_test_and_set (rtx op[]);

extern int lvx_branch_tested_bypass_p (rtx_insn *prod_insn, rtx_insn *cons_insn);
extern int lvx_stored_value_bypass_p (rtx_insn *prod_insn, rtx_insn *cons_insn);
extern int lvx_accumulator_bypass_p (rtx_insn *prod_insn, rtx_insn *cons_insn);

extern int lvx_has_tls_reference (rtx x);

extern bool lvx_float_fits_bits (const REAL_VALUE_TYPE *r, unsigned bitsz,
				 enum machine_mode mode);

extern poly_int64 lvx_initial_elimination_offset (int, int);

extern int lvx_get_real_frame_size (function *);

/*
 */
enum lvx_symbol_type
{
  SYMBOL_UNKNOWN,
  LABEL_PCREL_ABSOLUTE,
  LABEL_ABSOLUTE,
  SYMBOL_ABSOLUTE,
  SYMBOL_GOT,
  SYMBOL_GOTOFF,

  SYMBOL_TLSGD,
  SYMBOL_TLSLD,
  SYMBOL_TLSIE,
  SYMBOL_TLSLE
};

extern GTY (()) rtx lvx_link_reg_rtx;

extern GTY (()) rtx lvx_divmod_zero;
extern void lvx_expand_divmod (rtx, rtx, rtx, rtx, machine_mode, bool);

extern GTY (()) unsigned long lvx_builtin_undef_counter;

#endif /* RTX_CODE */
#endif /* lvx-protos.h */
