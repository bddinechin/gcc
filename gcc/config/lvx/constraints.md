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
  "A real value that fits on 32bits."
  (and (match_code "const_double")
       (match_test "lvx_float_fits_bits(rval,32,mode)")))

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

(define_constraint "S10"
  "A vector constant whose value fits in signed 10 bits."
  (and (match_code "const_vector")
       (match_test "lvx_has_10bit_vector_const_p (op)")))

(define_constraint "S16"
  "A vector constant whose value fits in signed 16 bits."
  (and (match_code "const_vector")
       (match_test "lvx_has_16bit_vector_const_p (op)")))

(define_constraint "S32"
  "A vector constant whose value fits in signed 32 bits."
  (and (match_code "const_vector")
       (match_test "lvx_has_32bit_vector_const_p (op)")))

(define_constraint "S37"
  "A vector constant whose value fits in signed 37 bits."
  (and (match_code "const_vector")
       (match_test "lvx_has_37bit_vector_const_p (op)")))

(define_constraint "S43"
  "A vector constant whose value fits in signed 43 bits."
  (and (match_code "const_vector")
       (match_test "lvx_has_43bit_vector_const_p (op)")))

(define_constraint "SX2"
  "A 64-bit vector constant made of two identical 32-bit values."
  (and (match_code "const_vector")
       (match_test "lvx_has_32x2bit_vector_const_p (op)")))

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

(define_memory_constraint "e"
  "Memory operands with 54-bit immediate."
  (and (match_code "mem")
       (match_test "lvx_has_54bit_immediate_p (op)")))

(define_address_constraint "Aa"
  "Memory address with 10-bit immediate or register."
  (and (match_test "address_operand (op, mode)")
       (match_test "lvx_has_10bit_imm_or_reg_p (op)")))

(define_address_constraint "Ab"
  "Memory address with 37-bit immediate."
  (and (match_test "address_operand (op, mode)")
       (match_test "lvx_has_37bit_immediate_p (op)")))

