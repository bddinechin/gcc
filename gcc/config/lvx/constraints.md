;; There are no "R" (EGR_REGS) and "Q" (OGR_REGS) constraints.  They existed
;; to satisfy the even/odd parity requirement of the LITE paired-word
;; instructions, whose W and Z operands had to come from RegClass worddRegE
;; or worddRegO.  The LVX ISA refactoring removed every Format that relied on
;; those classes: include/opcode/lvx.h now has singleReg, pairedReg and
;; quadReg for the general registers, with no parity variant at all (the only
;; surviving RegE/RegO pair, xwordqRegE/xwordqRegO, is on the XVR extended
;; vector file, not the GPRs).  So the affected operands take plain "r".
;;
;; EGR_REGS itself was a hand edit on top of BE/GCC-generated lvx-registers.h
;; -- the generated file never had it -- and goes away with this.

(define_register_constraint "SAB" "(SAB_REGS)"
     "A system register forcing SET and WFX* alone in bundle.")

(define_register_constraint "SFR" "(SFR_REGS)"
     "A system register constraint.")

(define_register_constraint "x" "(XCR_REGS)"
     "Coprocessor registers.")

(define_constraint "SYM"
  "An immediate or symbol operand."
  (ior (match_code "const_double,const_int")
       (match_test "lvx_symbol_operand(op,mode)")))

(define_constraint "Cp"
  "Code label reference in PIC mode."
  (and (match_code "label_ref")
       (match_test "flag_pic")))

(define_constraint "S01"
  "A zero or minus 1 constant."
  (and (match_code "const_int,const_vector")
       (ior (match_test "op == const0_rtx")
            (match_test "op == constm1_rtx")
            (match_test "op == CONST0_RTX (mode)")
            (match_test "op == CONSTM1_RTX (mode)"))))

(define_constraint "U06"
  "An unsigned 6-bit constant."
  (and (match_code "const_int")
       (match_test "UNSIGNED_INT_FITS_N_BITS (ival, 6)")))

(define_constraint "I10"
  "A signed 10-bit constant."
  (and (match_code "const_int")
       (match_test "ival >= -512 && ival <= 511")))

(define_constraint "H16"
  "A real value that fits on 16bits."
  (and (match_code "const_double")
       (match_test "lvx_float_fits_bits(rval,16,mode)")))

(define_constraint "H32"
  "A real value the 32-bit immediate of an FCOMP*.W format can carry: any SF,
   and a DF whose bits are a sign-extended 32-bit word."
  (and (match_code "const_double")
       (match_test "lvx_float_fits_imm32 (rval, mode)")))

(define_constraint "H43"
  "A real value that fits on 43bits."
  (and (match_code "const_double")
       (match_test "lvx_float_fits_bits(rval,43,mode)")))

(define_constraint "I16"
  "A signed 16-bit non symbolic constant."
  (and (match_code "const_int")
       (match_test "SIGNED_INT_FITS_N_BITS (ival, 16)")))

(define_constraint "I32"
  "A signed 32-bit non symbolic constant."
  (and (match_code "const_int")
       (match_test "SIGNED_INT_FITS_N_BITS (ival, 32)")))

(define_constraint "W32"
  "A signed or unsigned 32-bit non symbolic constant."
  (and (match_code "const_int")
       (ior (match_test "SIGNED_INT_FITS_N_BITS (ival, 32)")
            (match_test "UNSIGNED_INT_FITS_N_BITS (ival, 32)"))))

(define_constraint "I37"
  "A signed 37-bit non symbolic constant."
   (and (match_code "const_int")
       (match_test "SIGNED_INT_FITS_N_BITS (ival, 37)")))

(define_constraint "I43"
  "A signed 43-bit non symbolic constant."
  (and (match_code "const_int")
       (match_test "SIGNED_INT_FITS_N_BITS (ival, 43)")))

(define_constraint "SZ0"
  "The zero vector, which a .M format's replicated 32-bit word can build at any
   lane width -- the only value it can build at 64-bit lanes."
  (and (match_code "const_vector,const_int,const_double")
       (match_test "op == CONST0_RTX (GET_MODE (op))")))

(define_constraint "SXW"
  "A vector constant the packed immediate can carry: every lane equal, and the
   lane value one the 32-bit field produces -- any value at lanes of 32 bits or
   fewer, since the lane is the field's low bits, and a sign-extended 32-bit
   value at 64-bit lanes.  const_int and const_wide_int are no longer accepted:
   they stood for the non-lane-wise uses of the immediate, which went with the
   splat modifier."
  (and (match_code "const_vector")
       (match_test "lvx_uniform_lane_imm_p (op)")))

(define_memory_constraint "a"
  "Memory operands with 10-bit immediate or register."
  (and (match_code "mem")
       (match_test "lvx_has_10bit_imm_or_reg_p (op)")))

(define_memory_constraint "b"
  "Memory operands with 37-bit immediate."
  (and (match_code "mem")
       (match_test "lvx_has_37bit_immediate_p (op)")))

(define_memory_constraint "c"
  "Memory operands with pointer register."
  (and (match_code "mem")
       (match_test "general_register_operand (XEXP (op, 0), Pmode)")))

(define_memory_constraint "d"
  "Memory operands with 27-bit immediate."
  (and (match_code "mem")
       (match_test "lvx_has_27bit_immediate_p (op)")))

(define_address_constraint "Aa"
  "Memory address with 10-bit immediate or register."
  (and (match_test "address_operand (op, mode)")
       (match_test "lvx_has_10bit_imm_or_reg_p (op)")))

(define_address_constraint "Ab"
  "Memory address with 37-bit immediate."
  (and (match_test "address_operand (op, mode)")
       (match_test "lvx_has_37bit_immediate_p (op)")))

