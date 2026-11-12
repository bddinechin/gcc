;; Constant zero operand for scalar or vector MODE
(define_predicate "const_zero_operand"
  (and (match_code "const_int,const_wide_int,const_double,const_vector")
       (match_test "op == CONST0_RTX (mode)")))

;; The COMP* condition modifiers that test a lane's SIGN BIT.  This is what makes
;; `movemask (COMPN_cc (a, b))' collapse to `COMP_cc (a, b)': a COMPN* lane is 0
;; or -1, so "< 0" and "!= 0" both say "this lane's comparison was true", which
;; is exactly the bit COMP* would have packed.  `.eq' against zero would be the
;; COMPLEMENT, and the unsigned forms are constant on a 0/-1 lane (`.ltu' never,
;; `.geu' always), so only these two are sound.
(define_predicate "lvx_msb_test_modifier"
  (and (match_code "const_string")
       (ior (match_test "!strcmp (XSTR (op, 0), \".lt\")")
	    (match_test "!strcmp (XSTR (op, 0), \".ne\")"))))

;; Constant 0 or -1 operand for scalar or vector MODE
(define_predicate "const_zero_mone_operand"
  (and (match_code "const_int, const_wide_int,const_vector")
       (ior (match_test "op == CONSTM1_RTX (mode)")
            (match_test "op == CONST0_RTX (mode)"))))

;; Constant 1.0 operand for floating-point MODE
(define_predicate "const_float1_operand"
  (and (match_code "const_double,const_vector")
       (match_test "op == CONST1_RTX (mode)")))

;; Register or constant 0/-1.
(define_predicate "reg_zero_mone_operand"
  (ior (match_operand 0 "register_operand")
       (match_operand 0 "const_zero_mone_operand")))

;; Register or constant 1.0 (floating-point).
(define_predicate "register_float1_operand"
  (ior (match_operand 0 "register_operand")
       (match_operand 0 "const_float1_operand")))

;; Allow for LABELs to be used in the mov expander
;; It will split it using add_pcrel insn.
;; This predicates should only be used in the expander as LABELs
;; are not to be accepted during insn matching.
(define_predicate "lvx_mov_operand"
 (ior (match_operand 0 "general_operand")
      (and (match_test "flag_pic")
           (match_code "label_ref"))))

;; An unsigned 6-bit const_int: the predicate that agrees with U06.
(define_predicate "sixbits_unsigned_operand"
  (and (match_code "const_int")
       (match_test "satisfies_constraint_U06 (op)")))

(define_predicate "reg_shift_operand"
  (ior (match_operand 0 "register_operand")
       (match_operand 0 "sixbits_unsigned_operand")))

(define_predicate "fp_conv_shift_operand"
  (match_code "const_int")
{
  /* The scale of a float<->fixed conversion.  Without the shifted form the
     instruction has no scale operand at all, so only a zero scale can be
     honoured; anything else has to be rejected here rather than assembled
     and silently dropped.  */
  if (!HAVE_LVX_FP_CONV_WITH_SHIFT)
    return op == const0_rtx;
  return satisfies_constraint_U06 (op);
})

(define_predicate "const_pow2lt64_operand"
  (match_code "const_int")
{
  return (__builtin_popcountll (INTVAL (op)) == 1)
         && (__builtin_ctzll (INTVAL (op)) < 64);
})

;; A signed 32-bit const_int and nothing else: the predicate that agrees with
;; the I32 constraint.  `immediate_operand' against an I32-only alternative
;; admits a symbolic CONST that no alternative can hold, and LRA cannot reload
;; a vec_duplicate of a constant -- `lvx_makedp' ICEd in curr_insn_transform on
;; (vec_duplicate:V2DI (const (plus (symbol_ref) 16))) once its `i' alternative
;; went.
(define_predicate "const_s32_operand"
  (and (match_code "const_int")
       (match_test "satisfies_constraint_I32 (op)")))

;; A non-negative signed 32-bit const_int.
(define_predicate "const_pos32_operand"
  (and (match_operand 0 "const_s32_operand")
       (match_test "INTVAL (op) >= 0")))

;; A register, or a constant the machine sign-extends from a 32-bit word (I32).
;; This is also the 128-bit scalar source of a .M format -- the TImode
;; arithmetic and bitwise patterns and CMOVEQ -- which reg_or_imm32_operand
;; used to spell separately with the same test.  Those once admitted a
;; replicated word too, via the `.@' modifier; that reading is retired from the
;; ISA, splatting being a property of a SIMD instruction's lane size, and none
;; of these users has one.  Admitting a value no alternative can hold is what
;; makes reload fail rather than diagnose.
;;
;; Neither this nor register_w32_operand takes a symbolic CONST: I32 and W32
;; are const_int only, and the `const' this match_code used to list could never
;; get past them.
(define_predicate "register_s32_operand"
  (ior (match_operand 0 "register_operand")
       (match_operand 0 "const_s32_operand")))

;; A register, or a 32-bit const_int read either signed or unsigned (W32).
(define_predicate "register_w32_operand"
  (ior (match_operand 0 "register_operand")
       (and (match_code "const_int")
            (match_test "satisfies_constraint_W32 (op)"))))

;; Register or immediate up to float 32
(define_predicate "register_f32_operand"
 (and (match_code "reg,subreg,const_double")
      (ior (match_operand 0 "register_operand")
           (match_test "satisfies_constraint_H32(op)"))))

(define_predicate "syscall_operand"
  (match_code "mem")
{
    return (GET_CODE (XEXP (op, 0)) == REG
            || GET_CODE(XEXP (op, 0)) == CONST_INT)
        && lvx_syscall_addrspace_p (op);
})

(define_predicate "jump_operand"
  (match_code "mem")
{
  /* Weak symbols can be resolved to 0 and thus generate long branches that
     don't fit in our 27 bits offsets.  Reject them here and let the call
     expanders handle that case by generating indirect calls.

     A 'farcall' is out of CALL's reach too, but it is accepted here: CALLX
     spans 54 bits, so the call stays direct.  Which of the two forms comes
     out is the business of the patterns, each conditioned on
     lvx_is_farcall_p, not of this predicate.  */

  return GET_CODE (XEXP (op, 0)) == LABEL_REF
	 || (GET_CODE (XEXP (op, 0)) == SYMBOL_REF
	     && !SYMBOL_REF_WEAK (XEXP (op, 0)));
})

;; Integer comparison operators against integer zero.
(define_predicate "zero_comparison_operator"
  (match_code "eq,ne,le,lt,ge,gt"))

;; Integer comparison operators CCB encodes directly, as the ccbcomp values
;; 0-5 (double word) and 8-13 (word).  The other four ordered codes -- gt, le,
;; gtu, leu -- reach CCB by swapping its two register operands, which is what
;; the "%S" output modifier does.
(define_predicate "int_comparison_operator"
  (match_code "lt,ge,ltu,geu,eq,ne"))

;; Floating-point comparisons operators supported.
(define_predicate "float_comparison_operator"
  (match_code "ne,eq,ge,lt,uneq,unge,unlt,ltgt"))

;; Returns TRUE if op is a register or an immediate suitable for sign
;; extension from the format signed10, upper27_lower10 or
;; extend27_upper27_lower10
(define_predicate "lvx_r_s10_s37_s64_operand"
 (ior (and (match_test "!flag_pic")
           (match_operand 0 "nonmemory_operand"))
      (match_code "const_int")
      (match_code "const_double")
      (match_operand 0 "register_operand")
      (match_test "lvx_legitimate_pic_symbolic_ref_p(op)"))
)

(define_predicate "symbolic_operand"
  (match_code "const,symbol_ref,label_ref"))

(define_predicate "lvx_symbol_operand"
  (match_code "symbol_ref,label_ref,const,unspec")
{
  rtx base, offset;
  split_const (op, &base, &offset);
  /* Allow for (const (plus (sym) (const_int offset))) */
  switch (GET_CODE (base))
    {
    case SYMBOL_REF:
      /* TLS symbols are not constant.  */
      // if (SYMBOL_REF_TLS_MODEL (op))
      //   return false;
      return true;
    case LABEL_REF:
      /* For certain code models, the code is near as well.  */
      return true;
    case UNSPEC:
      if (XINT (base, 1) == UNSPEC_GOTOFF
          || XINT (base, 1) == UNSPEC_GOT
          || XINT (base, 1) == UNSPEC_TLS_DTPOFF
          || XINT (base, 1) == UNSPEC_TLS_GD
          || XINT (base, 1) == UNSPEC_TLS_LD
          || XINT (base, 1) == UNSPEC_TLS_IE
          || XINT (base, 1) == UNSPEC_TLS_LE
          || XINT (base, 1) == UNSPEC_PCREL)
          return true;
      break;
    default:
        gcc_unreachable ();
    }
  return false;
})

;; Reject memory addresses that use the .xs addressing mode.
;; Here .xs addressing may appear as (plus (mult (reg) (const_int)) (reg)).
;; In that cases addresses will be rejected by address_operand().
(define_predicate "noxsaddr_operand"
  (match_test "address_operand (op, mode)")
{
  if (GET_CODE (op) == PLUS && GET_CODE (XEXP (op, 0)) == MULT)
    return false;
  return true;
})

;; The addressing modes of the masked and the atomic memory instructions, which
;; are the same two: a bare register (constraint `c') or a 27-bit offset (`d').
;; mematomic_operand was a second copy of this body.
(define_predicate "memsimple_operand"
  (match_code "mem")
{
  return indirect_operand (op, mode)
         || lvx_has_27bit_immediate_p (op);
})

;; Used for do loop pattern where we have an output reload in a jump insn.
;; This is not supported by reload so the insn must handle them.
;; This hack comes from the arc backend.
(define_predicate "shouldbe_register_operand"
  (match_code "reg,subreg,mem")
{
  return ((reload_in_progress || reload_completed)
          ? general_operand : register_operand) (op, mode);
})

(define_predicate "system_register_operand"
  (match_code "reg")
{
  unsigned regno = REGNO (op);
  return REGNO_REG_CLASS (regno) == SFR_REGS;
})

(define_predicate "general_register_operand"
  (match_code "reg,subreg")
{
  if (GET_CODE (op) == SUBREG)
    op = SUBREG_REG (op);
  unsigned regno = REGNO (op);
  return REGNO_REG_CLASS (regno) == GENERAL_REGS;
})

(define_predicate "extension_register_operand"
  (match_code "reg,subreg")
{
  if (GET_CODE (op) == SUBREG)
    op = SUBREG_REG (op);
  unsigned regno = REGNO (op);
  return REGNO_REG_CLASS (regno) == XCR_REGS;
})

(define_predicate "float16_inner_mode"
  (match_code "reg,subreg")
{
  machine_mode inner_mode = GET_MODE_INNER (GET_MODE (op));
  return inner_mode == HFmode;
})

(define_predicate "masked_modifier"
  (match_code "const_string")
{
  const char *modifier = XSTR (op, 0);
  if (modifier[0] == '.' && modifier[1] == 'm')
    return true;
  for (modifier++; *modifier; modifier++)
    if (*modifier == '.') break;
  return modifier[0] == '.' && modifier[1] == 'm';
})

(define_predicate "store_multiple_operation"
  (and (match_code "parallel")
       (match_test "lvx_store_multiple_operation_p (op)")))

;; Return 1 if OP is a load multiple operation, known to be a PARALLEL.
(define_predicate "load_multiple_operation"
  (and (match_code "parallel")
       (match_test "lvx_load_multiple_operation_p (op)")))


;; The else value for a masked load: LVX supports only the "undefined" case
;; (masked-off lanes are not guaranteed to hold any particular value -- the
;; shim reads a contiguous span, so a sparse mask leaves them memory-defined,
;; not zero -- so the vectorizer must blend them before use).  A scratch rtx
;; is how GCC spells MASK_LOAD_ELSE_UNDEFINED.
(define_predicate "maskload_else_operand"
  (match_operand 0 "scratch_operand"))

;; A packed SIMD source the immediate formats can take: a register, or a vector
;; constant whose lanes are all equal and whose lane value the 32-bit field can
;; produce.  The scalar arm this predicate used to carry -- for a CONST_INT
;; standing in for a 64-bit splat -- is gone with the non-lane-wise uses of the
;; immediate: every one of the eleven places that asks is on a 128- or 256-bit
;; vector mode, so GET_MODE_SIZE (mode) <= 8 was unreachable.
(define_predicate "reg_or_lane_imm_operand"
  (ior (match_operand 0 "register_operand")
       (and (match_code "const_vector")
            (match_test "lvx_uniform_lane_imm_p (op)"))))

;; A 128-bit SIMD source that a .M format can take at 64-bit lanes.  The format
;; carries one 32-bit word and the machine replicates it over the operand, so
;; the only value it can build there is the one whose halves are equal -- and
;; the one worth having is zero: `a + 0.0` and `a * 0.0` survive folding because
;; of signed zeros, and otherwise cost two MAKEDs and a register pair.  -0.0 is
;; deliberately not this: its bit pattern is not zero, so it is not CONST0_RTX.
(define_predicate "reg_or_const_zero_operand"
  (ior (match_operand 0 "register_operand")
       (match_operand 0 "const_zero_operand")))

