;; HI

(define_insn "bswaphi2"
  [(set (match_operand:HI 0 "register_operand" "=r")
        (bswap:HI (match_operand:HI 1 "register_operand" "r")))]
  ""
  "sbmm8d %0 = %1, 0x0102"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny_x")
   (set_attr "length"        "8")]
)

;; No _sx sibling: this emits sbmm8d, a D-form instruction with no
;; signextw modifier, so the extension cannot be folded into it.
(define_insn "*bswapsi2_zext"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (zero_extend:DI (bswap:HI (match_operand:HI 1 "register_operand" "r"))))]
  ""
  "sbmm8d %0 = %1, 0x0102"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny_x")
   (set_attr "length"        "8")]
)

;; SIDI (SI/DI)

(define_expand "usadd<mode>3"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand:SIDI 2 "<imm32pred>" "")]
  ""
  {
    if (!HAVE_LVX_US_PLUS_<MODE>)
      emit_insn (gen_usadd<mode>3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_usadd<mode>3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn_and_split "usadd<mode>3_1"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (us_plus:SIDI (match_operand:SIDI 1 "register_operand" "r")
                      (match_operand:SIDI 2 "register_operand" "r")))
   (clobber (match_scratch:SIDI 3 "=&r"))]
  "!HAVE_LVX_US_PLUS_<MODE>"
  "#"
  "!HAVE_LVX_US_PLUS_<MODE>"
  [(set (match_dup 0)
        (plus:SIDI (match_dup 1) (match_dup 2)))
   (set (match_dup 3)
        (ltu:SIDI (match_dup 0) (match_dup 1)))
   (set (match_dup 0)
        (if_then_else:SIDI
            (ne (match_dup 3) (const_int 0))
            (const_int -1)
            (match_dup 0)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
  }
)

(define_expand "ussub<mode>3"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand:SIDI 2 "<imm32pred>" "")]
  ""
  {
    if (!HAVE_LVX_US_MINUS_<MODE>)
      emit_insn (gen_ussub<mode>3_1 (operands[0], operands[1], operands[2]));
    else
      {
	/* sbfus prints "%0 = %2, %1", so operand 2 lands in the register-only
	   slot while operand 1 takes the .M immediate.  gen_* does not check
	   predicates, so a constant operand 2 must be forced here.  */
	rtx op2 = force_reg (<MODE>mode, operands[2]);
	emit_insn (gen_ussub<mode>3_2 (operands[0], operands[1], op2));
      }
    DONE;
  }
)

(define_insn_and_split "ussub<mode>3_1"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (us_minus:SIDI (match_operand:SIDI 1 "register_operand" "r")
                       (match_operand:SIDI 2 "register_operand" "r")))
   (clobber (match_scratch:SIDI 3 "=&r"))]
  "!HAVE_LVX_US_MINUS_<MODE>"
  "#"
  "!HAVE_LVX_US_MINUS_<MODE>"
  [(set (match_dup 3)
        (umin:SIDI (match_dup 1) (match_dup 2)))
   (set (match_dup 0)
        (minus:SIDI (match_dup 1) (match_dup 3)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
  }
)

(define_expand "usashl<mode>3"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand:SI 2 "reg_shift_operand" "")]
  ""
  {
    if (!HAVE_LVX_US_ASHIFT_<MODE>)
      emit_insn (gen_usashl<mode>3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_usashl<mode>3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn_and_split "usashl<mode>3_1"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (us_ashift:SIDI (match_operand:SIDI 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))
   (clobber (match_scratch:SIDI 3 "=&r"))
   (clobber (match_scratch:SIDI 4 "=&r"))]
  "!HAVE_LVX_US_ASHIFT_<MODE>"
  "#"
  "!HAVE_LVX_US_ASHIFT_<MODE>"
  [(set (match_dup 0)
        (ashift:SIDI (match_dup 1) (match_dup 2)))
   (set (match_dup 3)
        (lshiftrt:SIDI (match_dup 0) (match_dup 2)))
   (set (match_dup 4)
        (ne:SIDI (match_dup 3) (match_dup 1)))
   (set (match_dup 0)
        (if_then_else:SIDI
            (ne (match_dup 4) (const_int 0))
            (const_int -1)
            (match_dup 0)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
  }
)

(define_insn "negdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (neg:DI (match_operand:DI 1 "register_operand" "r")))]
  ""
  "negd %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "negsi2<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (neg:SI (match_operand:SI 1 "register_operand" "r")))]
  ""
  "negw<_sx> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

;; No "neghi2"/"negqi2".  They emitted neghq and negbo, which are not scalar
;; 16-/8-bit negates at all: hq is four halves and bo eight bytes packed in a
;; 64-bit register, i.e. the 64-bit SIMD family that the LVX ISA dropped.  The
;; surviving neg* opcodes are negd/negw (scalar) and negho/negbx (128-bit
;; SIMD); LVX has no subword scalar negate.  Without these patterns the middle
;; end widens a QI/HI negate to SImode and uses negw, which is what the
;; hardware can actually do.

(define_insn "ssneg<mode>2<arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (ss_neg:SIDI (match_operand:SIDI 1 "register_operand" "r")))]
  ""
  "sbfs<suffix><_sx> %0 = %1, 0"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny_x")
   (set_attr "length"        "8")]
)

;; ABSD_registerW_registerZ_simple and ABSW_signextw_registerW_registerZ_simple
;; are the only forms, on both cores, and "simple" is one syllable.  There is
;; no two-syllable abs to choose between, so there is nothing to gate.
(define_insn "abs<mode>2<arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (abs:SIDI (match_operand:SIDI 1 "register_operand" "r")))]
  ""
  "abs<suffix><_sx> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_expand "ssabs<mode>2"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")]
  ""
  {
    if (!HAVE_LVX_SS_ABS_<MODE>)
      emit_insn (gen_ssabs<mode>2_1 (operands[0], operands[1]));
    else
      emit_insn (gen_ssabs<mode>2_2 (operands[0], operands[1]));
    DONE;
  }
)

(define_insn_and_split "ssabs<mode>2_1"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (ss_abs:SIDI (match_operand:SIDI 1 "register_operand" "r")))]
  "!HAVE_LVX_SS_ABS_<MODE>"
  "#"
  "!HAVE_LVX_SS_ABS_<MODE>"
  [(set (match_dup 0)
        (ss_neg:SIDI (match_dup 1)))
   (set (match_dup 0)
        (abs:SIDI (match_dup 0)))]
  ""
)

(define_insn "ssabs<mode>2_2<arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (ss_abs:SIDI (match_operand:SIDI 1 "register_operand" "r")))]
  "HAVE_LVX_SS_ABS_<MODE>"
  "abss<suffix><_sx> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

;; fix: need merge
(define_expand "abd<mode>3"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand:SIDI 2 "<imm32pred>" "")]
  ""
  {
    emit_insn (gen_abd<mode>3_4 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn "abd<mode>3_4<arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (minus:SIDI (smax:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                               (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>"))
                    (smin:SIDI (match_dup 1) (match_dup 2))))]
  "HAVE_LVX_ABD_ONLY_I32_IMMEDIATE"
  "abd<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length"      "4,         8")]
)

(define_insn "abd<mode>3_4s<arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (minus:SIDI (smax:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                               (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>"))
                    (smin:SIDI (match_dup 2) (match_dup 1))))]
  "HAVE_LVX_ABD_ONLY_I32_IMMEDIATE"
  "abd<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length"      "4,         8")]
)

(define_expand "abds<mode>3"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand:SIDI 2 "<imm32pred>" "")]
  ""
  {
    if (!HAVE_LVX_SS_ABD_<MODE>)
      emit_insn (gen_abds<mode>3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_abds<mode>3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn_and_split "abds<mode>3_1"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (ss_minus:SIDI (smax:SIDI (match_operand:SIDI 1 "register_operand" "r")
                                  (match_operand:SIDI 2 "register_operand" "r"))
                       (smin:SIDI (match_dup 1) (match_dup 2))))
   (clobber (match_scratch:SIDI 3 "=&r"))
   (clobber (match_scratch:SIDI 4 "=&r"))]
  "!HAVE_LVX_SS_ABD_<MODE>"
  "#"
  "!HAVE_LVX_SS_ABD_<MODE>"
  [(set (match_dup 3)
        (smax:SIDI (match_dup 1) (match_dup 2)))
   (set (match_dup 4)
        (smin:SIDI (match_dup 1) (match_dup 2)))
   (set (match_dup 0)
        (ss_minus:SIDI (match_dup 3) (match_dup 4)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
  }
)

(define_insn "abds<mode>3_2<arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (ss_minus:SIDI (smax:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                                  (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>"))
                       (smin:SIDI (match_dup 1) (match_dup 2))))]
  "HAVE_LVX_SS_ABD_<MODE>"
  "abds<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length"      "4,         8")]
)
(define_expand "abdu<mode>3"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand:SIDI 2 "<imm32pred>" "")]
  ""
  {
    if (!HAVE_LVX_UABD_<MODE>)
      emit_insn (gen_abdu<mode>3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_abdu<mode>3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn_and_split "abdu<mode>3_1"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (minus:SIDI (umax:SIDI (match_operand:SIDI 1 "register_operand" "r")
                               (match_operand:SIDI 2 "register_operand" "r"))
                    (umin:SIDI (match_dup 1) (match_dup 2))))
   (clobber (match_scratch:SIDI 3 "=&r"))
   (clobber (match_scratch:SIDI 4 "=&r"))]
  "!HAVE_LVX_UABD_<MODE>"
  "#"
  "!HAVE_LVX_UABD_<MODE>"
  [(set (match_dup 3)
        (umax:SIDI (match_dup 1) (match_dup 2)))
   (set (match_dup 4)
        (umin:SIDI (match_dup 1) (match_dup 2)))
   (set (match_dup 0)
        (minus:SIDI (match_dup 3) (match_dup 4)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
  }
)

(define_insn "abdu<mode>3_2<arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (minus:SIDI (umax:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                               (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>"))
                    (umin:SIDI (match_dup 1) (match_dup 2))))]
  "HAVE_LVX_UABD_<MODE>"
  "abdu<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length"      "4,         8")]
)
(define_insn "*addx2<suffix><arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (ashift:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                                (const_int 1))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL02_ADD_<MODE>"
  "addx2<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
;; multiplicative version of addx2<suffix>
(define_insn "*addx2<suffix>_m<arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (mult:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                              (const_int 2))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL02_ADD_<MODE>"
  "addx2<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
(define_insn "*addx4<suffix><arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (ashift:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                                (const_int 2))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL04_ADD_<MODE>"
  "addx4<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
;; multiplicative version of addx4<suffix>
(define_insn "*addx4<suffix>_m<arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (mult:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                              (const_int 4))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL04_ADD_<MODE>"
  "addx4<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
(define_insn "*addx8<suffix><arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (ashift:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                                (const_int 3))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL08_ADD_<MODE>"
  "addx8<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
;; multiplicative version of addx8<suffix>
(define_insn "*addx8<suffix>_m<arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (mult:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                              (const_int 8))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL08_ADD_<MODE>"
  "addx8<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
(define_insn "*addx16<suffix><arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (ashift:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                                (const_int 4))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL16_ADD_<MODE>"
  "addx16<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
;; multiplicative version of addx16<suffix>
(define_insn "*addx16<suffix>_m<arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (mult:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                              (const_int 16))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL16_ADD_<MODE>"
  "addx16<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
(define_insn "*addx32<suffix><arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (ashift:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                                (const_int 5))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL32_ADD_<MODE>"
  "addx32<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
(define_insn "*addx64<suffix><arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (ashift:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                                (const_int 6))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL64_ADD_<MODE>"
  "addx64<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
(define_insn "*addx32<suffix><arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (ashift:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                                (const_int 5))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL32_ADD_<MODE>"
  "addx32<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
;; multiplicative version of addx32<suffix>
(define_insn_and_split "*addx32<suffix>_m"
  [(set (match_operand:SIDI 0 "register_operand" "=&r,&r")
        (plus:SIDI (mult:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                              (const_int 32))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "!HAVE_LVX_MUL32_ADD_<MODE>"
  "#"
  "!HAVE_LVX_MUL32_ADD_<MODE>"
  [(set (match_dup 0) (ashift:SIDI (match_dup 1) (const_int 5)))
   (set (match_dup 0) (plus:SIDI (match_dup 0) (match_dup 2)))]
)
(define_insn "*addx32<suffix>_m<arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (mult:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                              (const_int 32))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL32_ADD_<MODE>"
  "addx32<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
(define_insn "*addx64<suffix><arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (ashift:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                                (const_int 6))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL64_ADD_<MODE>"
  "addx64<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
;; multiplicative version of addx64<suffix>
(define_insn_and_split "*addx64<suffix>_m"
  [(set (match_operand:SIDI 0 "register_operand" "=&r,&r")
        (plus:SIDI (mult:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                              (const_int 64))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "!HAVE_LVX_MUL64_ADD_<MODE>"
  "#"
  "!HAVE_LVX_MUL64_ADD_<MODE>"
  [(set (match_dup 0) (ashift:SIDI (match_dup 1) (const_int 6)))
   (set (match_dup 0) (plus:SIDI (match_dup 0) (match_dup 2)))]
)
(define_insn "*addx64<suffix>_m<arith_zx><arith_sx>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r")
        (plus:SIDI (mult:SIDI (match_operand:SIDI 1 "register_operand" "r,r")
                              (const_int 64))
                   (match_operand:SIDI 2 "<imm32pred>" "r,<imm32cons>")))]
  "HAVE_LVX_MUL64_ADD_<MODE>"
  "addx64<suffix><_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "     4,         8")]
)
(define_insn "lvx_land<suffix>"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (and:SIDI (ne:SIDI (match_operand:SIDI 1 "register_operand" "%r") (const_int 0))
                  (ne:SIDI (match_operand:SIDI 2 "register_operand" "r") (const_int 0))))]
  ""
  "land<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)
(define_insn "*ext_landw"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (and:DI (ne:DI (match_operand:SI 1 "register_operand" "%r") (const_int 0))
                (ne:DI (match_operand:SI 2 "register_operand" "r") (const_int 0))))]
  ""
  "landw %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "lvx_lnand<suffix>"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (ior:SIDI (eq:SIDI (match_operand:SIDI 1 "register_operand" "%r") (const_int 0))
                  (eq:SIDI (match_operand:SIDI 2 "register_operand" "r") (const_int 0))))]
  ""
  "lnand<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)
(define_insn "*ext_lnandw"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (ior:DI (eq:DI (match_operand:SI 1 "register_operand" "%r") (const_int 0))
                (eq:DI (match_operand:SI 2 "register_operand" "r") (const_int 0))))]
  ""
  "lnandw %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "lvx_lior<suffix>"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (ior:SIDI (ne:SIDI (match_operand:SIDI 1 "register_operand" "%r") (const_int 0))
                  (ne:SIDI (match_operand:SIDI 2 "register_operand" "r") (const_int 0))))]
  ""
  "lior<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)
(define_insn "*ext_liorw"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (ior:DI (ne:DI (match_operand:SI 1 "register_operand" "%r") (const_int 0))
                (ne:DI (match_operand:SI 2 "register_operand" "r") (const_int 0))))]
  ""
  "liorw %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "*lior<suffix>"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (ne:SIDI (ior:SIDI (match_operand:SIDI 1 "register_operand" "%r")
                           (match_operand:SIDI 2 "register_operand" "r"))
                 (const_int 0)))]
  ""
  "lior<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)
(define_insn "*ext_liorw"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (ne:DI (ior:SI (match_operand:SI 1 "register_operand" "%r")
                       (match_operand:SI 2 "register_operand" "r"))
               (const_int 0)))]
  ""
  "liorw %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "lvx_lnior<suffix>"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (and:SIDI (eq:SIDI (match_operand:SIDI 1 "register_operand" "%r") (const_int 0))
                  (eq:SIDI (match_operand:SIDI 2 "register_operand" "r") (const_int 0))))]
  ""
  "lnior<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)
(define_insn "*ext_lniorw"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (and:DI (eq:DI (match_operand:SI 1 "register_operand" "%r") (const_int 0))
                (eq:DI (match_operand:SI 2 "register_operand" "r") (const_int 0))))]
  ""
  "lniorw %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "*lnior<suffix>"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (eq:SIDI (ior:SIDI (match_operand:SIDI 1 "register_operand" "%r")
                           (match_operand:SIDI 2 "register_operand" "r"))
                 (const_int 0)))]
  ""
  "lnior<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)
(define_insn "*ext_lniorw"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (eq:DI (ior:SI (match_operand:SI 1 "register_operand" "%r")
                       (match_operand:SI 2 "register_operand" "r"))
                 (const_int 0)))]
  ""
  "lniorw %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "*zxh_and"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (and:SI (match_operand:SI 1 "register_operand" "r")
                (const_int 65535) ))]
  ""
  "zxhd %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

;; A bitfield extract must lie within the operand's mode.  combine folds a right
;; shift into the extract's offset (see make_extraction) without shrinking the
;; width — turning zero_extract(x, 32, 0) applied to (x >> 16) into
;; zero_extract(x, 32, 16), whose 48-bit reach runs past the SImode value.  On a
;; packed SIMD register (a V4SI lane is an SImode subreg of a wider GPR) those
;; extra bits are the *next lane*, so extfzd would read across the lane boundary
;; (a saturating-narrow miscompile: vqshrn_n_u32 over-saturated at -O3).  The
;; guard on the private insn rejects the out-of-mode field, so combine keeps the
;; shift (which is correct); a legitimate extv/extzv from expand is always in
;; range and matches.  The named expander keeps the standard optab available —
;; its HAVE_* query must stay operand-independent, so the guard cannot live there.
(define_expand "extv<mode>"
  [(set (match_operand:SIDI 0 "register_operand")
        (sign_extract:SIDI (match_operand:SIDI 1 "register_operand")
                           (match_operand 2 "sixbits_unsigned_operand")
                           (match_operand 3 "sixbits_unsigned_operand")))]
  "")

(define_insn "*extv<mode>"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (sign_extract:SIDI (match_operand:SIDI 1 "register_operand" "r")
                           (match_operand 2 "sixbits_unsigned_operand" "i")
                           (match_operand 3 "sixbits_unsigned_operand" "i")))]
  "lvx_extract_field_ok_p (operands[1], INTVAL (operands[2]), INTVAL (operands[3]))"
  "extfsd %0 = %1, %2, %3"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_expand "extzv<mode>"
  [(set (match_operand:SIDI 0 "register_operand")
        (zero_extract:SIDI (match_operand:SIDI 1 "register_operand")
                           (match_operand 2 "sixbits_unsigned_operand")
                           (match_operand 3 "sixbits_unsigned_operand")))]
  "")

(define_insn "*extzv<mode>"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (zero_extract:SIDI (match_operand:SIDI 1 "register_operand" "r")
                           (match_operand 2 "sixbits_unsigned_operand" "i")
                           (match_operand 3 "sixbits_unsigned_operand" "i")))]
  "lvx_extract_field_ok_p (operands[1], INTVAL (operands[2]), INTVAL (operands[3]))"
  "extfzd %0 = %1, %2, %3"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "insv<mode>"
  [(set (zero_extract:SIDI (match_operand:SIDI 0 "register_operand" "+r")
                           (match_operand 1 "sixbits_unsigned_operand" "i")
                           (match_operand 2 "sixbits_unsigned_operand" "i"))
        (match_operand:SIDI 3 "register_operand" "r"))]
  ""
  "insfd %0 = %3, %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

;; The same insert written out by hand -- (base & ~M) | ((val << pos) & M) --
;; never reaches insv.  combine's make_field_assignment recognises only an
;; assignment whose destination *is* the base (rtx_equal_for_field_assignment_p),
;; so an insert producing a new value is invisible to it and stayed four
;; instructions: slld + andd + andd + iord.  Only the C-bitfield path, which
;; reaches insv straight from store_bit_field, got the single insfd.
;;
;; combine does offer the whole expression to recog, so match it here.  Four
;; things about the shape:
;;
;; - the destination is a full register and base wears a "0" constraint, not a
;;   zero_extract destination with "+r".  Same instruction, but the RTL is a
;;   total def: no partial write for dataflow to reason about, and LRA inserts
;;   the copy only where base is live past the insert (5.6% of the insfd in
;;   newlib+libgcc+libm+libstdc++).  insv has to keep the zero_extract form
;;   because that is the optab's contract.
;; - the width and the position live in the mask, not in operands of their own,
;;   so the template is a C block (mnemonic: insfd -- the text audit in
;;   ../../../../CLAUDE.md cannot see it).  lvx_insf_mask_p is the whole
;;   correctness condition; insfd masks the source itself, so the value needs no
;;   bits cleared above the field, but a discontiguous mask, a mask pair that is
;;   not an exact complement, and a shift count disagreeing with the mask
;;   position must all be refused.
;; - IOR is commutative and both operand orders occur, so each shape needs its
;;   mirror.  commutative_operand_precedence ranks the two ANDs equally, so
;;   nothing canonicalises them: measured, a value in a register comes out
;;   base-first while an in-place insert through memory comes out value-first.
;;   One pattern catches half the cases and looks like it works.
;; - the mask on the shifted value disappears when the field runs to the top of
;;   the mode, since the shift has then already cleared everything outside it.
;;   lvx_insf_topmask_p is that case; without it "replace the high word" stays
;;   three instructions.

(define_insn "*insf<mode>_ior"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (ior:SIDI
          (and:SIDI (match_operand:SIDI 1 "register_operand" "0")
                    (match_operand 2 "const_int_operand"))
          (and:SIDI (ashift:SIDI (match_operand:SIDI 3 "register_operand" "r")
                                 (match_operand 4 "const_int_operand"))
                    (match_operand 5 "const_int_operand"))))]
  "lvx_insf_mask_p (<MODE>mode, operands[2], operands[5], operands[4])"
  { return lvx_output_insf (UINTVAL (operands[5]), <MODE>mode, 3); }
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "*insf<mode>_ior_v"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (ior:SIDI
          (and:SIDI (ashift:SIDI (match_operand:SIDI 3 "register_operand" "r")
                                 (match_operand 4 "const_int_operand"))
                    (match_operand 5 "const_int_operand"))
          (and:SIDI (match_operand:SIDI 1 "register_operand" "0")
                    (match_operand 2 "const_int_operand"))))]
  "lvx_insf_mask_p (<MODE>mode, operands[2], operands[5], operands[4])"
  { return lvx_output_insf (UINTVAL (operands[5]), <MODE>mode, 3); }
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

;; A field at bit 0 leaves combine no ashift to fold in.  The two ANDs are then
;; interchangeable in shape as well as in order, so lvx_insf_mask_p is what
;; decides which operand is the base: it requires the value's mask to be the low
;; run of ones and the base's to be its exact complement, which only one
;; assignment of the pair can satisfy.
(define_insn "*insf<mode>_ior0"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (ior:SIDI
          (and:SIDI (match_operand:SIDI 1 "register_operand" "0")
                    (match_operand 2 "const_int_operand"))
          (and:SIDI (match_operand:SIDI 3 "register_operand" "r")
                    (match_operand 4 "const_int_operand"))))]
  "lvx_insf_mask_p (<MODE>mode, operands[2], operands[4], NULL_RTX)"
  { return lvx_output_insf (UINTVAL (operands[4]), <MODE>mode, 3); }
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "*insf<mode>_ior0_v"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (ior:SIDI
          (and:SIDI (match_operand:SIDI 3 "register_operand" "r")
                    (match_operand 4 "const_int_operand"))
          (and:SIDI (match_operand:SIDI 1 "register_operand" "0")
                    (match_operand 2 "const_int_operand"))))]
  "lvx_insf_mask_p (<MODE>mode, operands[2], operands[4], NULL_RTX)"
  { return lvx_output_insf (UINTVAL (operands[4]), <MODE>mode, 3); }
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

;; Field at the top of the mode: the shift has cleared everything below it, so
;; combine drops the mask on the value and the base's mask is all that is left.
(define_insn "*insf<mode>_iortop"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (ior:SIDI
          (and:SIDI (match_operand:SIDI 1 "register_operand" "0")
                    (match_operand 2 "const_int_operand"))
          (ashift:SIDI (match_operand:SIDI 3 "register_operand" "r")
                       (match_operand 4 "const_int_operand"))))]
  "lvx_insf_topmask_p (<MODE>mode, operands[2], operands[4])"
  { return lvx_output_insf (~UINTVAL (operands[2]), <MODE>mode, 3); }
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "*insf<mode>_iortop_v"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (ior:SIDI
          (ashift:SIDI (match_operand:SIDI 3 "register_operand" "r")
                       (match_operand 4 "const_int_operand"))
          (and:SIDI (match_operand:SIDI 1 "register_operand" "0")
                    (match_operand 2 "const_int_operand"))))]
  "lvx_insf_topmask_p (<MODE>mode, operands[2], operands[4])"
  { return lvx_output_insf (~UINTVAL (operands[2]), <MODE>mode, 3); }
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)


;; SI


(define_insn "addsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (plus:SI (match_operand:SI 1 "register_operand" "r,r,r")
                 (match_operand:SI 2 "register_w32_operand" "r,I10,W32")))]
  ""
  "addw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "ssaddsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
        (ss_plus:SI (match_operand:SI 1 "register_operand" "r,r")
                    (match_operand:SI 2 "register_w32_operand" "r,W32")))]
  ""
  "addsw<_sx> %0 = %1, %2"
  [(set_attr "type"   "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "4,       8")]
)

(define_insn "usaddsi3_2<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
        (us_plus:SI (match_operand:SI 1 "register_operand" "r,r")
                    (match_operand:SI 2 "register_w32_operand" "r,W32")))]
  "HAVE_LVX_US_PLUS_SI"
  "addusw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length"      "4,         8")]
)

(define_insn "subsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (minus:SI (match_operand:SI 1 "register_w32_operand" "r,I10,W32")
                  (match_operand:SI 2 "register_operand" "r,r,r")))]
  ""
  "sbfw<_sx> %0 = %2, %1"
  [(set_attr "type" "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "sssubsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
        (ss_minus:SI (match_operand:SI 1 "register_w32_operand" "r,W32")
                     (match_operand:SI 2 "register_operand" "r,r")))]
  ""
  "sbfsw<_sx> %0 = %2, %1"
  [(set_attr "type"   "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "4,       8")]
)

(define_insn "ussubsi3_2<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
        (us_minus:SI (match_operand:SI 1 "register_operand" "r,r")
                     (match_operand:SI 2 "register_w32_operand" "r,W32")))]
  "HAVE_LVX_US_MINUS_SI"
  "sbfusw<_sx> %0 = %2, %1"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length"      "4,         8")]
)

(define_insn "mulsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
        (mult:SI (match_operand:SI 1 "register_operand" "r,r")
                 (match_operand:SI 2 "register_w32_operand" "r,W32")))]
  ""
  "mulw<_sx> %0 = %1, %2"
  [(set_attr "type" "imul, imul")
   (set_attr "issue" "lite, lite_x")
   (set_attr "length"      "4,          8")]
)

;; MULN* is a multiply whose result is negated, so `-(a * b)` is one
;; instruction instead of a multiply and a NEG.
;;
;; These match (mult (neg A) B), NOT (neg (mult A B)).  md.texi "Insn
;; Canonicalizations" moves a neg *inside* a mult as far as it will go --
;; "(neg (mult A B)) is canonicalized as (mult (neg A) B)" -- so a pattern
;; written the outside-in way is never offered and silently never fires.
;;
;; There is also no immediate alternative anywhere in this family, although
;; MULND/MULNW/MULN<lane> all have one in the ISA: with a constant operand the
;; negation folds into the constant instead (`-(a * 5)` becomes `a * -5`), so
;; the plain multiply already covers it and the MULN immediate encoding is
;; unreachable from C.  Only the register-register forms are selectable.
(define_insn "mulnegsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (mult:SI (neg:SI (match_operand:SI 1 "register_operand" "r"))
                 (match_operand:SI 2 "register_operand" "r")))]
  ""
  "mulnw<_sx> %0 = %1, %2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_insn "mulsidi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (mult:DI (sign_extend:DI (match_operand:SI 1 "register_operand" "r"))
                 (sign_extend:DI (match_operand:SI 2 "register_operand" "r"))))]
  ""
  "mulxwd %0 = %1, %2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_insn "umulsidi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (mult:DI (zero_extend:DI (match_operand:SI 1 "register_operand" "r"))
                 (zero_extend:DI (match_operand:SI 2 "register_operand" "r"))))]
  ""
  "mulxwd.u %0 = %1, %2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_insn "usmulsidi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (mult:DI (zero_extend:DI (match_operand:SI 1 "register_operand" "r"))
                 (sign_extend:DI (match_operand:SI 2 "register_operand" "r"))))]
  ""
  "mulxwd.su %0 = %2, %1"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_insn "mulnegsidi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (mult:DI (neg:DI (sign_extend:DI (match_operand:SI 1 "register_operand" "r")))
                 (sign_extend:DI (match_operand:SI 2 "register_operand" "r"))))]
  ""
  "mulnxwd %0 = %1, %2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_insn "umulnegsidi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (mult:DI (neg:DI (zero_extend:DI (match_operand:SI 1 "register_operand" "r")))
                 (zero_extend:DI (match_operand:SI 2 "register_operand" "r"))))]
  ""
  "mulnxwd.u %0 = %1, %2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

;; .su prints the sign-extended source first, and the neg can sit on either
;; operand -- mult is commutative, so both orders reach recog.
(define_insn "usmulnegsidi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (mult:DI (neg:DI (sign_extend:DI (match_operand:SI 1 "register_operand" "r")))
                 (zero_extend:DI (match_operand:SI 2 "register_operand" "r"))))]
  ""
  "mulnxwd.su %0 = %1, %2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_insn "usmulnegsidi3_rev"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (mult:DI (neg:DI (zero_extend:DI (match_operand:SI 1 "register_operand" "r")))
                 (sign_extend:DI (match_operand:SI 2 "register_operand" "r"))))]
  ""
  "mulnxwd.su %0 = %2, %1"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_expand "divsi3"
  [(set (match_operand:SI 0 "register_operand" "")
        (div:SI (match_operand:SI 1 "register_operand" "")
                (match_operand 2 "nonmemory_operand" "")))]
  ""
  {
    if (const_pow2lt64_operand (operands[2], VOIDmode))
      {
        HOST_WIDE_INT constant = INTVAL (operands[2]);
        operands[2] = gen_rtx_CONST_INT (VOIDmode, __builtin_ctzll (constant));
        emit_insn (gen_sshrsi3 (operands[0], operands[1], operands[2]));
        DONE;
      }
    lvx_expand_divmod (operands[0], NULL_RTX, operands[1], operands[2], SImode,
                       /*unsignedp=*/false);
    DONE;
  }
)

(define_expand "modsi3"
  [(set (match_operand:SI 0 "register_operand" "")
        (mod:SI (match_operand:SI 1 "register_operand" "")
                (match_operand 2 "nonmemory_operand" "")))]
  ""
  {
    lvx_expand_divmod (NULL_RTX, operands[0], operands[1], operands[2], SImode,
                       /*unsignedp=*/false);
    DONE;
  }
)

(define_expand "udivsi3"
  [(set (match_operand:SI 0 "register_operand" "")
        (udiv:SI (match_operand:SI 1 "register_operand" "")
                (match_operand 2 "nonmemory_operand" "")))]
  ""
  {
    if (const_pow2lt64_operand (operands[2], VOIDmode))
      {
        HOST_WIDE_INT constant = INTVAL (operands[2]);
        operands[2] = gen_rtx_CONST_INT (VOIDmode, __builtin_ctzll (constant));
        emit_insn (gen_lshrsi3 (operands[0], operands[1], operands[2]));
        DONE;
      }
    lvx_expand_divmod (operands[0], NULL_RTX, operands[1], operands[2], SImode,
                       /*unsignedp=*/true);
    DONE;
  }
)

(define_expand "umodsi3"
  [(set (match_operand:SI 0 "register_operand" "")
        (umod:SI (match_operand:SI 1 "register_operand" "")
                (match_operand 2 "nonmemory_operand" "")))]
  ""
  {
    lvx_expand_divmod (NULL_RTX, operands[0], operands[1], operands[2], SImode,
                       /*unsignedp=*/true);
    DONE;
  }
)

(define_insn "maddsisi4<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (plus:SI (mult:SI (match_operand:SI 1 "register_operand" "r")
                          (match_operand:SI 2 "register_operand" "r"))
                 (match_operand:SI 3 "register_operand" "0")))]
  ""
  "maddw<_sx> %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)
(define_insn "maddsidi4"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (plus:DI (mult:DI (sign_extend:DI (match_operand:SI 1 "register_operand" "r"))
                          (sign_extend:DI (match_operand:SI 2 "register_operand" "r")))
                 (match_operand:DI 3 "register_operand" "0")))]
  ""
  "maddxwd %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "umaddsidi4"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (plus:DI (mult:DI (zero_extend:DI (match_operand:SI 1 "register_operand" "r"))
                          (zero_extend:DI (match_operand:SI 2 "register_operand" "r")))
                 (match_operand:DI 3 "register_operand" "0")))]
  ""
  "maddxwd.u %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "*maddsuwd"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (plus:DI (mult:DI (sign_extend:DI (match_operand:SI 1 "register_operand" "r"))
                          (zero_extend:DI (match_operand:SI 2 "register_operand" "r")))
                 (match_operand:DI 3 "register_operand" "0")))]
  ""
  "maddxwd.su %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "usmaddsidi4"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (plus:DI (mult:DI (zero_extend:DI (match_operand:SI 1 "register_operand" "r"))
                          (sign_extend:DI (match_operand:SI 2 "register_operand" "r")))
                 (match_operand:DI 3 "register_operand" "0")))]
  ""
  "maddxwd.su %0 = %2, %1"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "msubsisi4<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (minus:SI (match_operand:SI 3 "register_operand" "0")
                  (mult:SI (match_operand:SI 1 "register_operand" "r")
                           (match_operand:SI 2 "register_operand" "r"))))]
  ""
  "msbfw<_sx> %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)
(define_insn "msubsidi4"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (minus:DI (match_operand:DI 3 "register_operand" "0")
                  (mult:DI (sign_extend:DI (match_operand:SI 1 "register_operand" "r"))
                           (sign_extend:DI (match_operand:SI 2 "register_operand" "r")))))]
  ""
  "msbfxwd %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "umsubsidi4"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (minus:DI (match_operand:DI 3 "register_operand" "0")
                  (mult:DI (zero_extend:DI (match_operand:SI 1 "register_operand" "r"))
                           (zero_extend:DI (match_operand:SI 2 "register_operand" "r")))))]
  ""
  "msbfxwd.u %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "*msbfsuwd"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (minus:DI (match_operand:DI 3 "register_operand" "0")
                  (mult:DI (sign_extend:DI (match_operand:SI 1 "register_operand" "r"))
                           (zero_extend:DI (match_operand:SI 2 "register_operand" "r")))))]
  ""
  "msbfxwd.su %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "usmsubsidi4"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (minus:DI (match_operand:DI 3 "register_operand" "0")
                  (mult:DI (zero_extend:DI (match_operand:SI 1 "register_operand" "r"))
                           (sign_extend:DI (match_operand:SI 2 "register_operand" "r")))))]
  ""
  "msbfxwd.su %0 = %2, %1"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "ashlsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (ashift:SI (match_operand:SI 1 "register_operand" "r")
                   (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  ""
  "sllw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "ssashlsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (ss_ashift:SI (match_operand:SI 1 "register_operand" "r")
                      (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  ""
  "slsw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "usashlsi3_2<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (us_ashift:SI (match_operand:SI 1 "register_operand" "r")
                      (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "HAVE_LVX_US_ASHIFT_SI"
  "slusw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "ashrsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (ashiftrt:SI (match_operand:SI 1 "register_operand" "r")
                     (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  ""
  "sraw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "lshrsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (lshiftrt:SI (match_operand:SI 1 "register_operand" "r")
                     (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  ""
  "srlw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "sshrsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (unspec:SI [(match_operand:SI 1 "register_operand" "r")
                    (match_operand:SI 2 "reg_shift_operand" "rU06")] UNSPEC_SRS))]
  ""
  "srsw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "sminsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (smin:SI (match_operand:SI 1 "register_operand" "r,r,r")
                 (match_operand:SI 2 "register_w32_operand" "r,I10,W32")))]
  ""
  "minw<_sx> %0 = %1, %2"
  [(set_attr "type"   "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "smaxsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (smax:SI (match_operand:SI 1 "register_operand" "r,r,r")
                 (match_operand:SI 2 "register_w32_operand" "r,I10,W32")))]
  ""
  "maxw<_sx> %0 = %1, %2"
  [(set_attr "type"   "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "uminsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (umin:SI (match_operand:SI 1 "register_operand" "r,r,r")
                 (match_operand:SI 2 "register_w32_operand" "r,I10,W32")))]
  ""
  "minuw<_sx> %0 = %1, %2"
  [(set_attr "type"   "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "umaxsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (umax:SI (match_operand:SI 1 "register_operand" "r,r,r")
                 (match_operand:SI 2 "register_w32_operand" "r,I10,W32")))]
  ""
  "maxuw<_sx> %0 = %1, %2"
  [(set_attr "type"   "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "andsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (and:SI (match_operand:SI 1 "register_operand" "r,r,r")
                (match_operand:SI 2 "register_w32_operand" "r,I10,W32")))]
  ""
  "andw<_sx> %0 = %1, %2"
  [(set_attr "type"   "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "*nandw<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (ior:SI (not:SI (match_operand:SI 1 "register_operand" "r,r,r"))
                (not:SI (match_operand:SI 2 "register_w32_operand" "r,I10,W32"))))]
  ""
  "nandw<_sx> %0 = %1, %2"
  [(set_attr "type"   "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "*andnw<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (and:SI (not:SI (match_operand:SI 1 "register_operand" "r,r,r"))
                (match_operand:SI 2 "register_w32_operand" "r,I10,W32")))]
  ""
  "andnw<_sx> %0 = %1, %2"
  [(set_attr "type"   "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "iorsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (ior:SI (match_operand:SI 1 "register_operand" "r,r,r")
                (match_operand:SI 2 "register_w32_operand" "r,I10,W32")))]
  ""
  "iorw<_sx> %0 = %1, %2"
  [(set_attr "type"   "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "*niorw<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (and:SI (not:SI (match_operand:SI 1 "register_operand" "r,r,r"))
                (not:SI (match_operand:SI 2 "register_w32_operand" "r,I10,W32"))))]
  ""
  "niorw<_sx> %0 = %1, %2"
  [(set_attr "type"   "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "*iornw<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (ior:SI (not:SI (match_operand:SI 1 "register_operand" "r,r,r"))
                (match_operand:SI 2 "register_w32_operand" "r,I10,W32")))]
  ""
  "iornw<_sx> %0 = %1, %2"
  [(set_attr "type"   "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "xorsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (xor:SI (match_operand:SI 1 "register_operand" "r,r,r")
                (match_operand:SI 2 "register_w32_operand" "r,I10,W32")))]
  ""
  "eorw<_sx> %0 = %1, %2"
  [(set_attr "type"   "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "*nxorw<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
        (not:SI (xor:SI (match_operand:SI 1 "register_operand" "r,r,r")
                        (match_operand:SI 2 "register_w32_operand" "r,I10,W32"))))]
  ""
  "neorw<_sx> %0 = %1, %2"
  [(set_attr "type"   "alu, alu, alu")
   (set_attr "issue" "tiny, tiny_x, tiny_x")
   (set_attr_alternative "length"
     [(const_int 4)
      (const_int 8)
      (const_int 8)])]
)

(define_insn "rotlsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (rotate:SI (match_operand:SI 1 "register_operand" "r")
                   (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  ""
  "rolw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "rotrsi3<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (rotatert:SI (match_operand:SI 1 "register_operand" "r")
                     (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  ""
  "rorw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "avgsi3_floor<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
        (unspec:SI [(match_operand:SI 1 "register_operand" "r,r")
                    (match_operand:SI 2 "register_w32_operand" "r,W32")] UNSPEC_AVG))]
  ""
  "avgw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "4,8")]
)

(define_insn "avgsi3_ceil<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
        (unspec:SI [(match_operand:SI 1 "register_operand" "r,r")
                    (match_operand:SI 2 "register_w32_operand" "r,W32")] UNSPEC_AVGR))]
  ""
  "avgrw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "4,8")]
)

(define_insn "uavgsi3_floor<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
        (unspec:SI [(match_operand:SI 1 "register_operand" "r,r")
                    (match_operand:SI 2 "register_w32_operand" "r,W32")] UNSPEC_AVGU))]
  ""
  "avguw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "4,8")]
)

(define_insn "uavgsi3_ceil<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
        (unspec:SI [(match_operand:SI 1 "register_operand" "r,r")
                    (match_operand:SI 2 "register_w32_operand" "r,W32")] UNSPEC_AVGRU))]
  ""
  "avgruw<_sx> %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length" "4,8")]
)

(define_insn "bswapsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (bswap:SI (match_operand:SI 1 "register_operand" "r")))]
  ""
  "sbmm8d %0 = %1, 0x01020408"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny_x")
   (set_attr "length"        "8")]
)
(define_insn "*bswapsi2_zext"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (zero_extend:DI (bswap:SI (match_operand:SI 1 "register_operand" "r"))))]
  ""
  "sbmm8d %0 = %1, 0x01020408"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny_x")
   (set_attr "length"        "8")]
)

;; zero-extend version of ssnegsi2
;; zero-extend version of ssabssi2
(define_insn "clrsbsi2<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (clrsb:SI (match_operand:SI 1 "register_operand" "r")))]
  ""
  "clsw<_sx> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)
(define_insn "clzsi2<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (clz:SI (match_operand:SI 1 "register_operand" "r")))]
  ""
  "clzw<_sx> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)
(define_insn "ctzsi2<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (ctz:SI (match_operand:SI 1 "register_operand" "r")))]
  ""
  "ctzw<_sx> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)
(define_insn "popcountsi2<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (popcount:SI (match_operand:SI 1 "register_operand" "r")))]
  ""
  "cbsw<_sx> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)
(define_insn "one_cmplsi2<arith_zx><arith_sx>"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (not:SI (match_operand:SI 1 "register_operand" "r")))]
  ""
  "notw<_sx> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

;; QI/HI logicals.  Only the lane-mask reaches these modes (see the QIHI
;; iterator in iterators.md); the 32-bit word instruction computes the low
;; bits, which is all a QI/HI value defines.  Register operands only — a
;; constant mask is materialised into a register, which the mask paths never do.
(define_insn "and<mode>3"
  [(set (match_operand:QIHI 0 "register_operand" "=r")
        (and:QIHI (match_operand:QIHI 1 "register_operand" "r")
                  (match_operand:QIHI 2 "register_operand" "r")))]
  ""
  "andw %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "ior<mode>3"
  [(set (match_operand:QIHI 0 "register_operand" "=r")
        (ior:QIHI (match_operand:QIHI 1 "register_operand" "r")
                  (match_operand:QIHI 2 "register_operand" "r")))]
  ""
  "iorw %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "xor<mode>3"
  [(set (match_operand:QIHI 0 "register_operand" "=r")
        (xor:QIHI (match_operand:QIHI 1 "register_operand" "r")
                  (match_operand:QIHI 2 "register_operand" "r")))]
  ""
  "eorw %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "one_cmpl<mode>2"
  [(set (match_operand:QIHI 0 "register_operand" "=r")
        (not:QIHI (match_operand:QIHI 1 "register_operand" "r")))]
  ""
  "notw %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "lvx_stsuw"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (unspec:SI [(match_operand:SI 1 "register_operand" "r")
                    (match_operand:SI 2 "register_operand" "r")] UNSPEC_STSU))]
  ""
  "stsuw %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)


;; DI

(define_insn "adddi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (plus:DI (match_operand:DI 1 "register_operand" "r,r,r,r")
                 (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")))]
  ""
  "addd %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,12")]
)
(define_insn "ssadddi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
        (ss_plus:DI (match_operand:DI 1 "register_operand" "r,r")
                    (match_operand:DI 2 "register_s32_operand" "r,I32")))]
  ""
  ;; ALU_DWRR1.M gives a sign-extended 32-bit immediate in the third slot --
  ;; the operand printed last.  Any Format ending .W or .M enables it.  These
  ;; have no ALU_DWRI variant, so unlike addd/sbfd there is no s10/s37/s64.
  "addsd %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length"    "4,          8")]
)

(define_insn "usadddi3_2"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
        (us_plus:DI (match_operand:DI 1 "register_operand" "r,r")
                    (match_operand:DI 2 "register_s32_operand" "r,I32")))]
  "HAVE_LVX_US_PLUS_DI"
  ;; ALU_DWRR1.M gives a sign-extended 32-bit immediate in the third slot --
  ;; the operand printed last.  Any Format ending .W or .M enables it.  These
  ;; have no ALU_DWRI variant, so unlike addd/sbfd there is no s10/s37/s64.
  "addusd %0 = %1, %2"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length"    "4,          8")]
)

(define_insn "subdi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (minus:DI (match_operand:DI 1 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")
                  (match_operand:DI 2 "register_operand" "r,r,r,r")))]
  ""
  "sbfd %0 = %2, %1"
  [(set_attr "type" "alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,12")]
)
(define_insn "sssubdi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
        (ss_minus:DI (match_operand:DI 1 "register_s32_operand" "r,I32")
                     (match_operand:DI 2 "register_operand" "r,r")))]
  ""
  ;; ALU_DWRR1.M gives a sign-extended 32-bit immediate in the third slot --
  ;; the operand printed last.  Any Format ending .W or .M enables it.  These
  ;; have no ALU_DWRI variant, so unlike addd/sbfd there is no s10/s37/s64.
  "sbfsd %0 = %2, %1"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length"    "4,          8")]
)

(define_insn "ussubdi3_2"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
        (us_minus:DI (match_operand:DI 1 "register_s32_operand" "r,I32")
                     (match_operand:DI 2 "register_operand" "r,r")))]
  "HAVE_LVX_US_MINUS_DI"
  ;; ALU_DWRR1.M gives a sign-extended 32-bit immediate in the third slot --
  ;; the operand printed last.  Any Format ending .W or .M enables it.  These
  ;; have no ALU_DWRI variant, so unlike addd/sbfd there is no s10/s37/s64.
  "sbfusd %0 = %2, %1"
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "tiny, tiny_x")
   (set_attr "length"    "4,          8")]
)

(define_expand "muldi3"
  [(set (match_operand:DI 0 "register_operand" "")
        (mult:DI (match_operand:DI 1 "register_operand" "")
                 (match_operand:DI 2 "register_s32_operand" "")))]
  ""
  ""
)

;; fixme: try merging those two.
(define_insn "muldi3_1"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (mult:DI (match_operand:DI 1 "register_operand" "r,r,r,r")
                 (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")))]
  "!HAVE_LVX_MULD_ONLY_I32_IMMEDIATE"
  "muld %0 = %1, %2"
  [(set_attr "type" "imul, imul, imul, imul")
   (set_attr "issue" "lite, lite, lite_x, lite_x2")
   (set_attr "length"      "4,        4,          8,         12")]
)
(define_insn "muldi3_4"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
        (mult:DI (match_operand:DI 1 "register_operand" "r,r")
                 (match_operand:DI 2 "register_s32_operand" "r,I32")))]
  "HAVE_LVX_MULD_ONLY_I32_IMMEDIATE"
  "muld %0 = %1, %2"
  [(set_attr "type" "imul, imul")
   (set_attr "issue" "lite, lite_x")
   (set_attr "length"      "4,          8")]
)

(define_insn "mulnegdi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (mult:DI (neg:DI (match_operand:DI 1 "register_operand" "r"))
                 (match_operand:DI 2 "register_operand" "r")))]
  ""
  "mulnd %0 = %1, %2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_expand "divdi3"
  [(set (match_operand:DI 0 "register_operand" "")
        (div:DI (match_operand:DI 1 "register_operand" "")
                (match_operand 2 "nonmemory_operand" "")))]
  ""
  {
    if (const_pow2lt64_operand (operands[2], VOIDmode))
      {
        HOST_WIDE_INT constant = INTVAL (operands[2]);
        operands[2] = gen_rtx_CONST_INT (VOIDmode, __builtin_ctzll (constant));
        emit_insn (gen_sshrdi3 (operands[0], operands[1], operands[2]));
        DONE;
      }
    lvx_expand_divmod (operands[0], NULL_RTX, operands[1], operands[2], DImode,
                       /*unsignedp=*/false);
    DONE;
  }
)

(define_expand "moddi3"
  [(set (match_operand:DI 0 "register_operand" "")
        (mod:DI (match_operand:DI 1 "register_operand" "")
                (match_operand 2 "nonmemory_operand" "")))]
  ""
  {
    lvx_expand_divmod (NULL_RTX, operands[0], operands[1], operands[2], DImode,
                       /*unsignedp=*/false);
    DONE;
  }
)

(define_expand "udivdi3"
  [(set (match_operand:DI 0 "register_operand" "")
        (udiv:DI (match_operand:DI 1 "register_operand" "")
                (match_operand 2 "nonmemory_operand" "")))]
  ""
  {
    if (const_pow2lt64_operand (operands[2], VOIDmode))
      {
        HOST_WIDE_INT constant = INTVAL (operands[2]);
        operands[2] = gen_rtx_CONST_INT (VOIDmode, __builtin_ctzll (constant));
        emit_insn (gen_lshrdi3 (operands[0], operands[1], operands[2]));
        DONE;
      }
    lvx_expand_divmod (operands[0], NULL_RTX, operands[1], operands[2], DImode,
                       /*unsignedp=*/true);
    DONE;
  }
)

(define_expand "umoddi3"
  [(set (match_operand:DI 0 "register_operand" "")
        (umod:DI (match_operand:DI 1 "register_operand" "")
                (match_operand 2 "nonmemory_operand" "")))]
  ""
  {
    lvx_expand_divmod (NULL_RTX, operands[0], operands[1], operands[2], DImode,
                       /*unsignedp=*/true);
    DONE;
  }
)

;; 128-bit (TImode) native operations.  The ISA has quadword ALU instructions
;; on register pairs (GPR128) -- addq/sbfq/negq and the full and/ior/xor family
;; with their fused-not forms -- so __int128 add, subtract, negate and the
;; bitwise operators are one instruction rather than the two-doubleword-plus-
;; carry sequence GCC would otherwise synthesise (addq carries across bit 63,
;; verified in the ISS).  Register operands only: a 128-bit literal has no
;; short encoding worth a constraint, so the middle end loads it first.

(define_insn "addti3"
  [(set (match_operand:TI 0 "register_operand" "=r,r,r")
        (plus:TI (match_operand:TI 1 "register_operand" "r,r,r")
                 (match_operand:TI 2 "reg_or_splat32_or_imm32_operand" "r,I32,SXW")))]
  ""
  "@
   addq %0 = %1, %2
   addq %0 = %1, %2
   addq %0 = %1, %W2.@"
  [(set_attr "type" "alu,alu,alu")
   (set_attr "issue" "lite,lite_x,lite_x")
   (set_attr "length" "4,8,8")]
)

(define_insn "subti3"
  [(set (match_operand:TI 0 "register_operand" "=r,r,r")
        (minus:TI (match_operand:TI 1 "reg_or_splat32_or_imm32_operand" "r,I32,SXW")
                  (match_operand:TI 2 "register_operand" "r,r,r")))]
  ""
  "@
   sbfq %0 = %2, %1
   sbfq %0 = %2, %1
   sbfq %0 = %2, %W1.@"
  [(set_attr "type" "alu,alu,alu")
   (set_attr "issue" "lite,lite_x,lite_x")
   (set_attr "length" "4,8,8")]
)

(define_insn "negti2"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (neg:TI (match_operand:TI 1 "register_operand" "r")))]
  ""
  "negq %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "andti3"
  [(set (match_operand:TI 0 "register_operand" "=r,r,r")
        (and:TI (match_operand:TI 1 "register_operand" "r,r,r")
                (match_operand:TI 2 "reg_or_splat32_or_imm32_operand" "r,I32,SXW")))]
  ""
  "@
   andq %0 = %1, %2
   andq %0 = %1, %2
   andq %0 = %1, %W2.@"
  [(set_attr "type" "alu,alu,alu")
   (set_attr "issue" "lite,lite_x,lite_x")
   (set_attr "length" "4,8,8")]
)

(define_insn "iorti3"
  [(set (match_operand:TI 0 "register_operand" "=r,r,r")
        (ior:TI (match_operand:TI 1 "register_operand" "r,r,r")
                (match_operand:TI 2 "reg_or_splat32_or_imm32_operand" "r,I32,SXW")))]
  ""
  "@
   iorq %0 = %1, %2
   iorq %0 = %1, %2
   iorq %0 = %1, %W2.@"
  [(set_attr "type" "alu,alu,alu")
   (set_attr "issue" "lite,lite_x,lite_x")
   (set_attr "length" "4,8,8")]
)

(define_insn "xorti3"
  [(set (match_operand:TI 0 "register_operand" "=r,r,r")
        (xor:TI (match_operand:TI 1 "register_operand" "r,r,r")
                (match_operand:TI 2 "reg_or_splat32_or_imm32_operand" "r,I32,SXW")))]
  ""
  "@
   eorq %0 = %1, %2
   eorq %0 = %1, %2
   eorq %0 = %1, %W2.@"
  [(set_attr "type" "alu,alu,alu")
   (set_attr "issue" "lite,lite_x,lite_x")
   (set_attr "length" "4,8,8")]
)

(define_insn "one_cmplti2"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (not:TI (match_operand:TI 1 "register_operand" "r")))]
  ""
  "notq %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

;; Fused not-logic (reached by combine), mirroring the doubleword forms.
(define_insn "*nandq"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (ior:TI (not:TI (match_operand:TI 1 "register_operand" "r"))
                (not:TI (match_operand:TI 2 "register_operand" "r"))))]
  ""
  "nandq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "*andnq"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (and:TI (not:TI (match_operand:TI 1 "register_operand" "r"))
                (match_operand:TI 2 "register_operand" "r")))]
  ""
  "andnq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "*norq"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (and:TI (not:TI (match_operand:TI 1 "register_operand" "r"))
                (not:TI (match_operand:TI 2 "register_operand" "r"))))]
  ""
  "niorq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "*ornq"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (ior:TI (not:TI (match_operand:TI 1 "register_operand" "r"))
                (match_operand:TI 2 "register_operand" "r")))]
  ""
  "iornq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "*nxorq"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (not:TI (xor:TI (match_operand:TI 1 "register_operand" "r")
                        (match_operand:TI 2 "register_operand" "r"))))]
  ""
  "neorq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

;; Bitwise select: for each set bit of the mask take the value's bit, else keep
;; the base's.  BSELQ does exactly that -- (val & mask) | (base & ~mask) -- but
;; the RTL to match is the XOR form, base ^ ((base ^ val) & mask), because
;; match.pd canonicalises the and/or/not spelling into it: three operations
;; instead of four.  Written as ior/and/not this never matches and the sequence
;; stays eorq + andq + eorq.
;;
;; The base appears twice, so it is a match_dup rather than a second operand,
;; and it carries the "0" constraint because BSELQ reads its destination -- no
;; LVX format has four register operands, so every three-input ALU instruction
;; is RMW.  LRA inserts a copy only where the base is live past the select.
;;
;; Both orders of the inner XOR occur: its two operands are plain registers of
;; equal commutative_operand_precedence, so nothing canonicalises them and the
;; order is whichever the source wrote.  Measured, (b & ~m) | (v & m) gives
;; base-first and (v & m) | (b & ~m) gives value-first.
(define_insn "*bselq"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (xor:TI
          (and:TI (xor:TI (match_operand:TI 1 "register_operand" "0")
                          (match_operand:TI 2 "register_operand" "r"))
                  (match_operand:TI 3 "register_operand" "r"))
          (match_dup 1)))]
  ""
  "bselq %0 = %2, %3"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "*bselq_v"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (xor:TI
          (and:TI (xor:TI (match_operand:TI 2 "register_operand" "r")
                          (match_operand:TI 1 "register_operand" "0"))
                  (match_operand:TI 3 "register_operand" "r"))
          (match_dup 1)))]
  ""
  "bselq %0 = %2, %3"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

;; The 64-bit partner.  Same XOR form, same match_dup base on a "0" constraint;
;; ALU_TINY rather than ALU_LITE, so the issue class differs from *bselq.
;;
;; The SIDI iterator gives SImode too, emitting the same bseld: a mask confined
;; to the low 32 bits leaves bits 32..63 taking the base's, which an SImode value
;; does not care about -- the same reason insv<mode> serves both widths with one
;; insfd.  There is no bselw.
(define_insn "*bseld<mode>"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (xor:SIDI
          (and:SIDI (xor:SIDI (match_operand:SIDI 1 "register_operand" "0")
                              (match_operand:SIDI 2 "register_operand" "r"))
                    (match_operand:SIDI 3 "register_operand" "r"))
          (match_dup 1)))]
  ""
  "bseld %0 = %2, %3"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "*bseld<mode>_v"
  [(set (match_operand:SIDI 0 "register_operand" "=r")
        (xor:SIDI
          (and:SIDI (xor:SIDI (match_operand:SIDI 2 "register_operand" "r")
                              (match_operand:SIDI 1 "register_operand" "0"))
                    (match_operand:SIDI 3 "register_operand" "r"))
          (match_dup 1)))]
  ""
  "bseld %0 = %2, %3"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "mulditi3"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (mult:TI (sign_extend:TI (match_operand:DI 1 "register_operand" "r"))
                 (sign_extend:TI (match_operand:DI 2 "register_operand" "r"))))]
  ""
  "mulxdq %0 = %1, %2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_insn "umulditi3"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (mult:TI (zero_extend:TI (match_operand:DI 1 "register_operand" "r"))
                 (zero_extend:TI (match_operand:DI 2 "register_operand" "r"))))]
  ""
  "mulxdq.u %0 = %1, %2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_insn "usmulditi3"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (mult:TI (zero_extend:TI (match_operand:DI 1 "register_operand" "r"))
                 (sign_extend:TI (match_operand:DI 2 "register_operand" "r"))))]
  ""
  "mulxdq.su %0 = %2, %1"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_insn "mulnegditi3"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (mult:TI (neg:TI (sign_extend:TI (match_operand:DI 1 "register_operand" "r")))
                 (sign_extend:TI (match_operand:DI 2 "register_operand" "r"))))]
  ""
  "mulnxdq %0 = %1, %2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_insn "umulnegditi3"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (mult:TI (neg:TI (zero_extend:TI (match_operand:DI 1 "register_operand" "r")))
                 (zero_extend:TI (match_operand:DI 2 "register_operand" "r"))))]
  ""
  "mulnxdq.u %0 = %1, %2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

;; .su prints the sign-extended source first, and the neg can sit on either
;; operand -- mult is commutative, so both orders reach recog.
(define_insn "usmulnegditi3"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (mult:TI (neg:TI (sign_extend:TI (match_operand:DI 1 "register_operand" "r")))
                 (zero_extend:TI (match_operand:DI 2 "register_operand" "r"))))]
  ""
  "mulnxdq.su %0 = %1, %2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_insn "usmulnegditi3_rev"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (mult:TI (neg:TI (zero_extend:TI (match_operand:DI 1 "register_operand" "r")))
                 (sign_extend:TI (match_operand:DI 2 "register_operand" "r"))))]
  ""
  "mulnxdq.su %0 = %2, %1"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

(define_insn_and_split "smuldi3_highpart"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (truncate:DI
          (lshiftrt:TI
            (mult:TI
              (sign_extend:TI (match_operand:DI 1 "register_operand" "r"))
              (sign_extend:TI (match_operand:DI 2 "register_operand" "r")))
            (const_int 64))))
   (clobber (match_scratch:TI 3 "=&r"))]
  ""
  "#"
  ""
  [(set (match_dup 3)
        (mult:TI
          (sign_extend:TI (match_dup 1))
          (sign_extend:TI (match_dup 2))))
   (set (match_dup 0)
        (subreg:DI (match_dup 3) 8))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (TImode);
  }
)

(define_insn_and_split "umuldi3_highpart"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (truncate:DI
          (lshiftrt:TI
            (mult:TI
              (zero_extend:TI (match_operand:DI 1 "register_operand" "r"))
              (zero_extend:TI (match_operand:DI 2 "register_operand" "r")))
            (const_int 64))))
   (clobber (match_scratch:TI 3 "=&r"))]
  ""
  "#"
  ""
  [(set (match_dup 3)
        (mult:TI
          (zero_extend:TI (match_dup 1))
          (zero_extend:TI (match_dup 2))))
   (set (match_dup 0)
        (subreg:DI (match_dup 3) 8))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (TImode);
  }
)

(define_insn "madddidi4"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (plus:DI (mult:DI (match_operand:DI 1 "register_operand" "r")
                          (match_operand:DI 2 "register_operand" "r"))
                 (match_operand:DI 3 "register_operand" "0")))]
  ""
  "maddd %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "maddditi4"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (plus:TI (mult:TI (sign_extend:TI (match_operand:DI 1 "register_operand" "r"))
                          (sign_extend:TI (match_operand:DI 2 "register_operand" "r")))
                 (match_operand:TI 3 "register_operand" "0")))]
  ""
  "maddxdq %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "umaddditi4"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (plus:TI (mult:TI (zero_extend:TI (match_operand:DI 1 "register_operand" "r"))
                          (zero_extend:TI (match_operand:DI 2 "register_operand" "r")))
                 (match_operand:TI 3 "register_operand" "0")))]
  ""
  "maddxdq.u %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

;; No "*madduzdt": the inherited .uz form accumulated into the high half of the
;; 128-bit accumulator (note the lshiftrt by 64 above).  LVX expresses the
;; widening multiply flavour through the widemult modifier, whose only
;; members are "." , ".U" and ".SU" -- there is no ".UZ" -- so this shape has
;; no LVX instruction and must be left to the generic expansion.

(define_insn "*maddsudt"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (plus:TI (mult:TI (sign_extend:TI (match_operand:DI 1 "register_operand" "r"))
                          (zero_extend:TI (match_operand:DI 2 "register_operand" "r")))
                 (match_operand:TI 3 "register_operand" "0")))]
  ""
  "maddxdq.su %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "usmaddditi4"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (plus:TI (mult:TI (zero_extend:TI (match_operand:DI 1 "register_operand" "r"))
                          (sign_extend:TI (match_operand:DI 2 "register_operand" "r")))
                 (match_operand:TI 3 "register_operand" "0")))]
  ""
  "maddxdq.su %0 = %2, %1"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "msubdidi4"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (minus:DI (match_operand:DI 3 "register_operand" "0")
                  (mult:DI (match_operand:DI 1 "register_operand" "r")
                           (match_operand:DI 2 "register_operand" "r"))))]
  ""
  "msbfd %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "msubditi4"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (minus:TI (match_operand:TI 3 "register_operand" "0")
                  (mult:TI (sign_extend:TI (match_operand:DI 1 "register_operand" "r"))
                           (sign_extend:TI (match_operand:DI 2 "register_operand" "r")))))]
  ""
  "msbfxdq %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "umsubditi4"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (minus:TI (match_operand:TI 3 "register_operand" "0")
                  (mult:TI (zero_extend:TI (match_operand:DI 1 "register_operand" "r"))
                           (zero_extend:TI (match_operand:DI 2 "register_operand" "r")))))]
  ""
  "msbfxdq.u %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

;; No "*msbfuzdt", for the same reason as *madduzdt above: no .UZ member in
;; the LVX widemult modifier.

(define_insn "*msbfsudt"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (minus:TI (match_operand:TI 3 "register_operand" "0")
                  (mult:TI (sign_extend:TI (match_operand:DI 1 "register_operand" "r"))
                           (zero_extend:TI (match_operand:DI 2 "register_operand" "r")))))]
  ""
  "msbfxdq.su %0 = %1, %2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "usmsubditi4"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (minus:TI (match_operand:TI 3 "register_operand" "0")
                  (mult:TI (zero_extend:TI (match_operand:DI 1 "register_operand" "r"))
                           (sign_extend:TI (match_operand:DI 2 "register_operand" "r")))))]
  ""
  "msbfxdq.su %0 = %2, %1"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn "ashldi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (ashift:DI (match_operand:DI 1 "register_operand" "r")
                   (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  ""
  "slld %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "ssashldi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (ss_ashift:DI (match_operand:DI 1 "register_operand" "r")
                      (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  ""
  "slsd %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "usashldi3_2"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (us_ashift:DI (match_operand:DI 1 "register_operand" "r")
                      (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "HAVE_LVX_US_ASHIFT_DI"
  "slusd %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "ashrdi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (ashiftrt:DI (match_operand:DI 1 "register_operand" "r")
                     (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  ""
  "srad %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "lshrdi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (lshiftrt:DI (match_operand:DI 1 "register_operand" "r")
                     (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  ""
  "srld %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

;; 128-bit (TImode) shifts.  SLLQ/SRAQ/SRLQ shift a register pair by a count in
;; a GPR (0..127) or a 6-bit immediate (0..63); a constant >= 64 does not fit
;; U06 so it lands in the register alternative, which the hardware masks to 7
;; bits -- so one instruction covers the whole range, no split.
(define_insn "ashlti3"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (ashift:TI (match_operand:TI 1 "register_operand" "r")
                   (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  ""
  "sllq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "ashrti3"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (ashiftrt:TI (match_operand:TI 1 "register_operand" "r")
                     (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  ""
  "sraq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "lshrti3"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (lshiftrt:TI (match_operand:TI 1 "register_operand" "r")
                     (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  ""
  "srlq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "sshrdi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unspec:DI [(match_operand:DI 1 "register_operand" "r")
                    (match_operand:SI 2 "reg_shift_operand" "rU06")] UNSPEC_SRS))]
  ""
  "srsd %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "smindi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (smin:DI (match_operand:DI 1 "register_operand" "r,r,r,r")
                 (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")))]
  ""
  "mind %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,12")]
)

(define_insn "smaxdi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (smax:DI (match_operand:DI 1 "register_operand" "r,r,r,r")
                 (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")))]
  ""
  "maxd %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,12")]
)

(define_insn "umindi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (umin:DI (match_operand:DI 1 "register_operand" "r,r,r,r")
                 (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")))]
  ""
  "minud %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,12")]
)

(define_insn "umaxdi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (umax:DI (match_operand:DI 1 "register_operand" "r,r,r,r")
                 (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")))]
  ""
  "maxud %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,12")]
)

(define_insn "anddi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r,r")
        (and:DI (match_operand:DI 1 "register_operand" "r,r,r,r,r")
                (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,SXW,i")))]
  ""
  "@
   andd %0 = %1, %2
   andd %0 = %1, %2
   andd %0 = %1, %2
   andd %0 = %1, %W2.@
   andd %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,8,12")]
)

(define_insn "*nandd"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (ior:DI (not:DI (match_operand:DI 1 "register_operand" "r,r,r,r"))
                (not:DI (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i"))))]
  ""
  "nandd %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,12")]
)

(define_insn "*andnd"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (and:DI (not:DI (match_operand:DI 1 "register_operand" "r,r,r,r"))
                (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")))]
  ""
  "andnd %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,12")]
)

(define_insn "iordi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r,r")
        (ior:DI (match_operand:DI 1 "register_operand" "r,r,r,r,r")
                (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,SXW,i")))]
  ""
  "@
   iord %0 = %1, %2
   iord %0 = %1, %2
   iord %0 = %1, %2
   iord %0 = %1, %W2.@
   iord %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,8,12")]
)

(define_insn "*nord"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (and:DI (not:DI (match_operand:DI 1 "register_operand" "r,r,r,r"))
                (not:DI (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i"))))]
  ""
  "niord %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,12")]
)

(define_insn "*ornd"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (ior:DI (not:DI (match_operand:DI 1 "register_operand" "r,r,r,r"))
                (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")))]
  ""
  "iornd %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,12")]
)

(define_insn "xordi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r,r")
        (xor:DI (match_operand:DI 1 "register_operand" "r,r,r,r,r")
                (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,SXW,i")))]
  ""
  "@
   eord %0 = %1, %2
   eord %0 = %1, %2
   eord %0 = %1, %2
   eord %0 = %1, %W2.@
   eord %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,8,12")]
)

(define_insn "*nxord"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (not:DI (xor:DI (match_operand:DI 1 "register_operand" "r,r,r,r")
                        (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i"))))]
  ""
  "neord %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,12")]
)

(define_insn_and_split "rotldi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (rotate:DI (match_operand:DI 1 "register_operand" "r")
                   (match_operand:SI 2 "register_operand" "r")))
   (clobber (match_scratch:SI 3 "=&r"))
   (clobber (match_scratch:DI 4 "=&r"))
   (clobber (match_scratch:DI 5 "=&r"))]
  ""
  "#"
  ""
  [(set (match_dup 3) (neg:SI (match_dup 2)))
   (set (match_dup 4) (ashift:DI (match_dup 1) (match_dup 2)))
   (set (match_dup 5) (lshiftrt:DI (match_dup 1) (match_dup 3)))
   (set (match_dup 0) (ior:DI (match_dup 4) (match_dup 5)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (SImode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (DImode);
    if (GET_CODE (operands[5]) == SCRATCH)
      operands[5] = gen_reg_rtx (DImode);
  }
)

(define_insn_and_split "rotrdi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (rotatert:DI (match_operand:DI 1 "register_operand" "r")
                     (match_operand:SI 2 "register_operand" "r")))
   (clobber (match_scratch:SI 3 "=&r"))
   (clobber (match_scratch:DI 4 "=&r"))
   (clobber (match_scratch:DI 5 "=&r"))]
  ""
  "#"
  ""
  [(set (match_dup 3) (neg:SI (match_dup 2)))
   (set (match_dup 4) (lshiftrt:DI (match_dup 1) (match_dup 2)))
   (set (match_dup 5) (ashift:DI (match_dup 1) (match_dup 3)))
   (set (match_dup 0) (ior:DI (match_dup 4) (match_dup 5)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (SImode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (DImode);
    if (GET_CODE (operands[5]) == SCRATCH)
      operands[5] = gen_reg_rtx (DImode);
  }
)

(define_insn "bswapdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (bswap:DI (match_operand:DI 1 "register_operand" "r")))]
  ""
  "sbmm8d %0 = %1, 0x0102040810204080"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny_x2")
   (set_attr "length"       "12")]
)

(define_insn "clrsbdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (clrsb:DI (match_operand:DI 1 "register_operand" "r")))]
  ""
  "clsd %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "clzdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (clz:DI (match_operand:DI 1 "register_operand" "r")))]
  ""
  "clzd %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "ctzdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (ctz:DI (match_operand:DI 1 "register_operand" "r")))]
  ""
  "ctzd %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "popcountdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (popcount:DI (match_operand:DI 1 "register_operand" "r")))]
  ""
  "cbsd %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "one_cmpldi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (not:DI (match_operand:DI 1 "register_operand" "r")))]
  ""
  "notd %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)


;; TI

(define_insn "lvx_tilow64"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (subreg:DI (match_operand:TI 1 "nonimmediate_operand" "r") 0))]
  ""
  "copyd %0 = %x1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "lvx_tihigh64"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (subreg:DI (match_operand:TI 1 "nonimmediate_operand" "r") 8))]
  ""
  "copyd %0 = %y1"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

;; 128-bit add/sub/neg are not defined here: LVX has no add-with-carry
;; (ADDCD) or subtract-with-borrow (SBFCD) instructions, so GCC falls
;; back to libgcc (__addti3, __subti3, __negti2).

(define_expand "multi3"
  [(set (match_operand:TI 0 "register_operand")
        (mult:TI (match_operand:TI 1 "register_operand")
                 (match_operand:TI 2 "nonmemory_operand")))]
  ""
  {
    if (!lvx_expand_memset_mul (operands, TImode))
      {
        operands[2] = force_reg (TImode, operands[2]);
        rtx hi_0 = simplify_gen_subreg (DImode, operands[0], TImode, 8);
        rtx lo_1 = simplify_gen_subreg (DImode, operands[1], TImode, 0);
        rtx hi_1 = simplify_gen_subreg (DImode, operands[1], TImode, 8);
        rtx lo_2 = simplify_gen_subreg (DImode, operands[2], TImode, 0);
        rtx hi_2 = simplify_gen_subreg (DImode, operands[2], TImode, 8);
        emit_insn (gen_umulditi3 (operands[0], lo_1, lo_2));
        emit_insn (gen_madddidi4 (hi_0, lo_1, hi_2, hi_0));
        emit_insn (gen_madddidi4 (hi_0, hi_1, lo_2, hi_0));
      }
    DONE;
  }
)


;; OI

;; Pattern for the expansion of the stored constant in memset.
(define_expand "muloi3"
  [(set (match_operand:OI 0 "register_operand")
        (mult:OI (match_operand:OI 1 "register_operand")
                 (match_operand:OI 2 "nonmemory_operand")))]
  ""
  {
    if (lvx_expand_memset_mul (operands, OImode))
      DONE;
    FAIL;
  }
)

;; HF

(define_insn "addhf3"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (plus:HF (match_operand:HF 1 "register_operand" "r")
                 (match_operand:HF 2 "register_operand" "r")))]
  ""
  "faddh %0 = %1, %2"
  [(set_attr "type" "fmuls")
   (set_attr "issue" "lite")]
)

(define_insn "subhf3"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (minus:HF (match_operand:HF 1 "register_operand" "r")
                  (match_operand:HF 2 "register_operand" "r")))]
  ""
  "fsbfh %0 = %2, %1"
  [(set_attr "type" "fmuls")
   (set_attr "issue" "lite")]
)

(define_insn "mulhf3"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (mult:HF (match_operand:HF 1 "register_operand" "r")
                 (match_operand:HF 2 "register_operand" "r")))]
  ""
  "fmulh %0 = %1, %2"
  [(set_attr "type" "fmuls")
   (set_attr "issue" "lite")]
)

(define_insn "*fmulhw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (mult:SF (float_extend:SF (match_operand:HF 1 "register_operand" "r"))
                 (float_extend:SF (match_operand:HF 2 "register_operand" "r"))))]
  "HAVE_LVX_MULT_SF_HF_HF"
  "fmulhw %0 = %1, %2"
  [(set_attr "type" "fmuls")
   (set_attr "issue" "lite")]
)

(define_expand "divhf3"
  [(set (match_operand:HF 0 "register_operand" "")
        (div:HF (match_operand:HF 1 "register_operand" "")
                (match_operand:HF 2 "register_operand" "")))]
  ""
  {
    rtx a = gen_reg_rtx (SFmode);
    rtx b = gen_reg_rtx (SFmode);
    rtx d = gen_reg_rtx (SFmode);
    emit_insn (gen_extendhfsf2 (a, operands[1]));
    emit_insn (gen_extendhfsf2 (b, operands[2]));
    rtx rm = gen_rtx_CONST_STRING (VOIDmode, "");
    emit_insn (gen_lvx_fdivw (d, a, b, rm));
    emit_insn (gen_truncsfhf2 (operands[0], d));
    DONE;
  }
)

(define_insn "fmahf4"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (fma:HF  (match_operand:HF 1 "register_operand" "r")
                 (match_operand:HF 2 "register_operand" "r")
                 (match_operand:HF 3 "register_operand" "0")))]
  ""
  "ffmah %0 = %1, %2"
  [(set_attr "type" "fmadds")
   (set_attr "issue" "lite")]
)

(define_insn "*ffmahw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (fma:SF  (float_extend:SF (match_operand:HF 1 "register_operand" "r"))
                 (float_extend:SF (match_operand:HF 2 "register_operand" "r"))
                 (match_operand:SF 3 "register_operand" "0")))]
  "HAVE_LVX_FMA_SF_HF_HF"
  "ffmahw %0 = %1, %2"
  [(set_attr "type" "fmadds")
   (set_attr "issue" "lite")]
)

(define_insn "fnmahf4"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (fma:HF  (neg:HF (match_operand:HF 1 "register_operand" "r"))
                 (match_operand:HF 2 "register_operand" "r")
                 (match_operand:HF 3 "register_operand" "0")))]
  ""
  "ffmsh %0 = %1, %2"
  [(set_attr "type" "fmadds")
   (set_attr "issue" "lite")]
)

(define_insn "*ffmshw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (fma:SF (float_extend:SF (neg:HF (match_operand:HF 1 "register_operand" "r")))
                (float_extend:SF (match_operand:HF 2 "register_operand" "r"))
                (match_operand:SF 3 "register_operand" "0")))]
  "HAVE_LVX_FMS_SF_HF_HF"
  "ffmshw %0 = %1, %2"
  [(set_attr "type" "fmadds")
   (set_attr "issue" "lite")]
)

(define_insn "*ffmshw2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (fma:SF (float_extend:SF (match_operand:HF 1 "register_operand" "r"))
                (float_extend:SF (neg:HF (match_operand:HF 2 "register_operand" "r")))
                (match_operand:SF 3 "register_operand" "0")))]
  "HAVE_LVX_FMS_SF_HF_HF"
  "ffmshw %0 = %1, %2"
  [(set_attr "type" "fmadds")
   (set_attr "issue" "lite")]
)

; NOTE: `fmin<mode>3`/`fmax<mode>3` are GCC's standard names for C's
; fmin/fmax, which are IEEE 754-2008 minNum/maxNum: they return the
; *other* operand when one is a NaN.  LVX has both families, and the
; only inputs that tell them apart are NaN and signed zero:
;
;   fminn{h,w,d}  f{16,32,64}_minNum  -- IEEE-2008, returns the non-NaN
;   fmin{h,w,d}   f{16,32,64}_min     -- IEEE-2019, propagates the NaN
;
; These patterns emitted the -2019 form until 2026-08-04, so
; __builtin_fmin(NaN, x) returned NaN instead of x.  Every non-NaN case
; agreed, which is why it went unnoticed; validation/tests/micro/minmax.c
; is the regression test.  Do not "simplify" these back to fmin/fmax.
(define_insn "sminhf3"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (smin:HF (match_operand:HF 1 "register_operand" "r")
                 (match_operand:HF 2 "register_operand" "r")))]
  "HAVE_LVX_MIN_HF && !(HAVE_LVX_BUG_FMIN && flag_signaling_nans)"
  "fminnh %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "smaxhf3"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (smax:HF (match_operand:HF 1 "register_operand" "r")
                 (match_operand:HF 2 "register_operand" "r")))]
  "HAVE_LVX_MAX_HF && !(HAVE_LVX_BUG_FMAX && flag_signaling_nans)"
  "fmaxnh %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "neghf2"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (neg:HF (match_operand:HF 1 "register_operand" "r")))]
  ""
  "fnegh %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "abshf2"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (abs:HF (match_operand:HF 1 "register_operand" "r")))]
  ""
  "fabsh %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_expand "floatsihf2"
  [(set (match_operand:HF 0 "register_operand" "")
        (float:HF (match_operand:SI 1 "register_operand" "")))]
  ""
  {
    rtx tempsf = gen_reg_rtx (SFmode);
    emit_insn (gen_floatsisf2 (tempsf, operands[1]));
    emit_insn (gen_truncsfhf2 (operands[0], tempsf));
    DONE;
  }
)

(define_expand "floatdihf2"
  [(set (match_operand:HF 0 "register_operand" "")
        (float:HF (match_operand:DI 1 "register_operand" "")))]
  ""
  {
    rtx tempdf = gen_reg_rtx (DFmode);
    emit_insn (gen_floatdidf2 (tempdf, operands[1]));
    rtx tempsf = gen_reg_rtx (SFmode);
    emit_insn (gen_truncdfsf2 (tempsf, tempdf));
    emit_insn (gen_truncsfhf2 (operands[0], tempsf));
    DONE;
  }
)

(define_expand "floatunssihf2"
  [(set (match_operand:HF 0 "register_operand" "")
        (unsigned_float:HF (match_operand:SI 1 "register_operand" "")))]
  ""
  {
    rtx tempsf = gen_reg_rtx (SFmode);
    emit_insn (gen_floatunssisf2 (tempsf, operands[1]));
    emit_insn (gen_truncsfhf2 (operands[0], tempsf));
    DONE;
  }
)

(define_expand "floatunsdihf2"
  [(set (match_operand:HF 0 "register_operand" "")
        (unsigned_float:HF (match_operand:DI 1 "register_operand" "")))]
  ""
  {
    rtx tempdf = gen_reg_rtx (DFmode);
    emit_insn (gen_floatunsdidf2 (tempdf, operands[1]));
    rtx tempsf = gen_reg_rtx (SFmode);
    emit_insn (gen_truncdfsf2 (tempsf, tempdf));
    emit_insn (gen_truncsfhf2 (operands[0], tempsf));
    DONE;
  }
)

(define_insn "extendhfsf2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (float_extend:SF (match_operand:HF 1 "register_operand" "r")))]
  ""
  "fwidenhw %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_expand "extendhfdf2"
  [(set (match_operand:DF 0 "register_operand" "")
        (float_extend:DF (match_operand:HF 1 "register_operand" "")))]
  ""
  {
    rtx tempsf = gen_reg_rtx (SFmode);
    emit_insn (gen_extendhfsf2 (tempsf, operands[1]));
    emit_insn (gen_extendsfdf2 (operands[0], tempsf));
    DONE;
  }
)


;; SF

(define_insn "addsf3"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (plus:SF (match_operand:SF 1 "register_operand" "r")
                 (match_operand:SF 2 "register_operand" "r")))]
  ""
  "faddw %0 = %1, %2"
  [(set_attr "type" "fmuld")
   (set_attr "issue" "lite")]
)

(define_insn "subsf3"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (minus:SF (match_operand:SF 1 "register_operand" "r")
                  (match_operand:SF 2 "register_operand" "r")))]
  ""
  "fsbfw %0 = %2, %1"
  [(set_attr "type" "fmuld")
   (set_attr "issue" "lite")]
)

(define_insn "mulsf3"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (mult:SF (match_operand:SF 1 "register_operand" "r")
                 (match_operand:SF 2 "register_operand" "r")))]
  ""
  "fmulw %0 = %1, %2"
  [(set_attr "type" "fmuld")
   (set_attr "issue" "lite")]
)

(define_insn "*fmulwd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (mult:DF (float_extend:DF (match_operand:SF 1 "register_operand" "r"))
                 (float_extend:DF (match_operand:SF 2 "register_operand" "r"))))]
  "HAVE_LVX_MULT_DF_SF_SF"
  "fmulwd %0 = %1, %2"
  [(set_attr "type" "fmuld")
   (set_attr "issue" "lite")]
)

(define_expand "divsf3"
  [(set (match_operand:SF 0 "register_operand" "")
        (div:SF (match_operand:SF 1 "register_operand" "")
                (match_operand:SF 2 "register_operand" "")))]
  ""
  {
    rtx rm = gen_rtx_CONST_STRING (VOIDmode, "");
    emit_insn (gen_lvx_fdivw (operands[0], operands[1], operands[2], rm));
    DONE;
  }
)

(define_insn "fmasf4"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (fma:SF  (match_operand:SF 1 "register_operand" "r")
                 (match_operand:SF 2 "register_operand" "r")
                 (match_operand:SF 3 "register_operand" "0")))]
  ""
  "ffmaw %0 = %1, %2"
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_insn "*ffmawd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (fma:DF  (float_extend:DF (match_operand:SF 1 "register_operand" "r"))
                 (float_extend:DF (match_operand:SF 2 "register_operand" "r"))
                 (match_operand:DF 3 "register_operand" "0")))]
  "HAVE_LVX_FMA_DF_SF_SF"
  "ffmawd %0 = %1, %2"
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_insn "fnmasf4"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (fma:SF  (neg:SF (match_operand:SF 1 "register_operand" "r"))
                 (match_operand:SF 2 "register_operand" "r")
                 (match_operand:SF 3 "register_operand" "0")))]
  ""
  "ffmsw %0 = %1, %2"
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_insn "*ffmswd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (fma:DF (float_extend:DF (neg:SF (match_operand:SF 1 "register_operand" "r")))
                (float_extend:DF (match_operand:SF 2 "register_operand" "r"))
                (match_operand:DF 3 "register_operand" "0")))]
  "HAVE_LVX_FMS_DF_SF_SF"
  "ffmswd %0 = %1, %2"
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_insn "*ffmswd2"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (fma:DF (float_extend:DF (match_operand:SF 1 "register_operand" "r"))
                (float_extend:DF (neg:SF (match_operand:SF 2 "register_operand" "r")))
                (match_operand:DF 3 "register_operand" "0")))]
  "HAVE_LVX_FMS_DF_SF_SF"
  "ffmswd %0 = %1, %2"
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_insn "sminsf3"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (smin:SF (match_operand:SF 1 "register_operand" "r")
                 (match_operand:SF 2 "register_operand" "r")))]
  "HAVE_LVX_MIN_SF && !(HAVE_LVX_BUG_FMIN && flag_signaling_nans)"
  "fminnw %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "smaxsf3"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (smax:SF (match_operand:SF 1 "register_operand" "r")
                 (match_operand:SF 2 "register_operand" "r")))]
  "HAVE_LVX_MAX_SF && !(HAVE_LVX_BUG_FMAX && flag_signaling_nans)"
  "fmaxnw %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "negsf2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (neg:SF (match_operand:SF 1 "register_operand" "r")))]
  ""
  "fnegw %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "abssf2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (abs:SF (match_operand:SF 1 "register_operand" "r")))]
  ""
  "fabsw %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "floatsisf2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (float:SF (match_operand:SI 1 "register_operand" "r")))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "floatw.rn %0 = %1, 0";
    return "floatw.rn %0 = %1";
  }
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "floatunssisf2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unsigned_float:SF (match_operand:SI 1 "register_operand" "r")))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "floatuw.rn %0 = %1, 0";
    return "floatuw.rn %0 = %1";
  }
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "floatdisf2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (float:SF (match_operand:DI 1 "register_operand" "r")))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "floatdw.rn %0 = %1, 0";
    return "floatdw.rn %0 = %1";
  }
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "floatunsdisf2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unsigned_float:SF (match_operand:DI 1 "register_operand" "r")))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "floatudw.rn %0 = %1, 0";
    return "floatudw.rn %0 = %1";
  }
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "fix_truncsfsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (fix:SI (match_operand:SF 1 "register_operand" "r")))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixedw.rz %0 = %1, 0";
    return "fixedw.rz %0 = %1";
  }
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)
;; No _sx sibling: the fixedw family is fpucode6-encoded and carries no
;; signextw modifier.
(define_insn "*fix_truncsfsi2_zext"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (zero_extend:DI (fix:SI (match_operand:SF 1 "register_operand" "r"))))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixedw.rz %0 = %1, 0";
    return "fixedw.rz %0 = %1";
  }
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "fixuns_truncsfsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (unsigned_fix:SI (match_operand:SF 1 "register_operand" "r")))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixeduw.rz %0 = %1, 0";
    return "fixeduw.rz %0 = %1";
  }
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)
;; No _sx sibling: see fix_truncsfsi2 above.
(define_insn "*fixuns_truncsfsi2_zext"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (zero_extend:DI (unsigned_fix:SI (match_operand:SF 1 "register_operand" "r"))))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixeduw.rz %0 = %1, 0";
    return "fixeduw.rz %0 = %1";
  }
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

;; V2SF <-> V2SI, on the 128-bit unit.
;;
;; These used to emit FIXEDWP/FLOATWP and their unsigned forms -- the 64-bit
;; SIMD family -- and that was wrong code, not just a stale choice: every
;; ALU_FWP* encoding is a strict subset of ALU_FWCOP's, so `fixedwp.rz`
;; assembled to a word the machine executes as `faddwc.c.mi.rz`, a complex
;; float add with a third register that was never written.  The wp family is
;; retired for that reason (lvx-mds, 2026-09-27) and these go the way V2SF
;; arithmetic already does: the operand into the low half of a zeroed 128-bit
;; pair, one FIXEDWQ/FLOATWQ on all four lanes, the low half back out.
;;
;; The upper lanes must be ZERO rather than garbage.  Integer arithmetic raises
;; nothing so v64vector.c's lanes may hold anything, but a conversion sets the
;; CS exception flags: float(garbage) can round and raise Inexact, and
;; fix(garbage float) can raise Invalid.  Zeroing costs one MAKED the register
;; allocator usually hoists.

(define_expand "fix_truncv2sfv2si2"
  [(set (match_operand:V2SI 0 "register_operand")
        (fix:V2SI (match_operand:V2SF 1 "register_operand")))]
  "LVX_2"
  {
    lvx_expand_cvt64 (FIX, V4SFmode, V4SImode, operands);
    DONE;
  }
)

(define_expand "fixuns_truncv2sfv2si2"
  [(set (match_operand:V2SI 0 "register_operand")
        (unsigned_fix:V2SI (match_operand:V2SF 1 "register_operand")))]
  "LVX_2"
  {
    lvx_expand_cvt64 (UNSIGNED_FIX, V4SFmode, V4SImode, operands);
    DONE;
  }
)

(define_expand "floatv2siv2sf2"
  [(set (match_operand:V2SF 0 "register_operand")
        (float:V2SF (match_operand:V2SI 1 "register_operand")))]
  "LVX_2"
  {
    lvx_expand_cvt64 (FLOAT, V4SImode, V4SFmode, operands);
    DONE;
  }
)

(define_expand "floatunsv2siv2sf2"
  [(set (match_operand:V2SF 0 "register_operand")
        (unsigned_float:V2SF (match_operand:V2SI 1 "register_operand")))]
  "LVX_2"
  {
    lvx_expand_cvt64 (UNSIGNED_FLOAT, V4SImode, V4SFmode, operands);
    DONE;
  }
)

(define_insn "truncsfhf2"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (float_truncate:HF (match_operand:SF 1 "register_operand" "r")))]
  ""
  "fnarrowwh %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "extendsfdf2"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (float_extend:DF (match_operand:SF 1 "register_operand" "r")))]
  ""
  "fwidenwd %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_expand "rsqrthf2"
  [(match_operand:HF 0 "register_operand" "")
   (match_operand:HF 1 "register_operand" "")]
  ""
  {
    rtx rm = gen_rtx_CONST_STRING (VOIDmode, "");
    rtx a = gen_reg_rtx (SFmode);
    rtx r = gen_reg_rtx (SFmode);
    operands[1] = force_reg (GET_MODE (operands[1]), operands[1]);
    emit_insn (gen_extendhfsf2 (a, operands[1]));
    emit_insn (gen_lvx_fsrsrw (r, a));
    emit_insn (gen_truncsfhf2 (operands[0], r));
    DONE;
  }
)

(define_insn "rsqrtsf2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SF 1 "register_operand" "r")]
                   UNSPEC_FRSR))]
  ""
  "fsrsrw %0 = %1"
  [(set_attr "type" "fdiv")
   (set_attr "issue" "lite")]
)

(define_expand "roundevensf2"
  [(set (match_operand:SF 0 "register_operand" "")
        (match_operand:SF 1 "register_operand" ""))]
  ""
  {
    rtx cs_val = gen_reg_rtx (DImode);
    rtx tempsi = gen_reg_rtx (SImode);
    rtx tempsf = gen_reg_rtx (SFmode);
    rtx cs = gen_rtx_REG (DImode, 68);
    rtx rn = gen_rtx_CONST_STRING (VOIDmode, ".rn");
    rtx deqz = gen_rtx_CONST_STRING (VOIDmode, ".deqz");
    /* 1. Reset the error bits (IO and XIO) we check in CS */
    emit_insn (gen_lvx_get (cs_val, cs));
    emit_insn (gen_anddi3 (cs_val, cs_val, GEN_INT (0xfffffffffffffdfd)));
    emit_insn (gen_lvx_set (cs, cs_val));
    /* 2. Convert floating point to fixed point */
    emit_insn (gen_lvx_fixedw (tempsi, operands[1], const0_rtx, rn));
    emit_insn (gen_lvx_get (cs_val, cs));
    emit_insn (gen_anddi3 (cs_val, cs_val, GEN_INT (0x202)));
    /* 3. Convert back to floating point, and take into account corner cases. */
    emit_insn (gen_lvx_floatw (tempsf, tempsi, const0_rtx, rn));
    emit_insn (gen_lvx_selectfw (operands[0], tempsf, operands[1], cs_val, deqz));
    DONE;
  }
)


;; DF

(define_insn "adddf3"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (plus:DF (match_operand:DF 1 "register_operand" "r")
                 (match_operand:DF 2 "register_operand" "r")))]
  ""
  "faddd %0 = %1, %2"
  [(set_attr "type" "fmuld")
   (set_attr "issue" "lite")]
)

(define_insn "subdf3"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (minus:DF (match_operand:DF 1 "register_operand" "r")
                  (match_operand:DF 2 "register_operand" "r")))]
  ""
  "fsbfd %0 = %2, %1"
  [(set_attr "type" "fmuld")
   (set_attr "issue" "lite")]
)

(define_insn "muldf3"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (mult:DF (match_operand:DF 1 "register_operand" "r")
                 (match_operand:DF 2 "register_operand" "r")))]
  ""
  "fmuld %0 = %1, %2"
  [(set_attr "type" "fmuld")
   (set_attr "issue" "lite")]
)

(define_expand "divdf3"
  [(set (match_operand:DF 0 "register_operand" "")
        (div:DF (match_operand:DF 1 "register_operand" "")
                (match_operand:DF 2 "register_operand" "")))]
  ""
  {
    rtx rm = gen_rtx_CONST_STRING (VOIDmode, "");
    emit_insn (gen_lvx_fdivd (operands[0], operands[1], operands[2], rm));
    DONE;
  }
)

(define_insn "fmadf4"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (fma:DF  (match_operand:DF 1 "register_operand" "r")
                 (match_operand:DF 2 "register_operand" "r")
                 (match_operand:DF 3 "register_operand" "0")))]
  ""
  "ffmad %0 = %1, %2"
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_insn "fnmadf4"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (fma:DF  (neg:DF (match_operand:DF 1 "register_operand" "r"))
                 (match_operand:DF 2 "register_operand" "r")
                 (match_operand:DF 3 "register_operand" "0")))]
  ""
  "ffmsd %0 = %1, %2"
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_insn "smindf3"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (smin:DF (match_operand:DF 1 "register_operand" "r")
                 (match_operand:DF 2 "register_operand" "r")))]
  "HAVE_LVX_MIN_DF && !(HAVE_LVX_BUG_FMIN && flag_signaling_nans)"
  "fminnd %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "smaxdf3"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (smax:DF (match_operand:DF 1 "register_operand" "r")
                 (match_operand:DF 2 "register_operand" "r")))]
  "HAVE_LVX_MAX_DF && !(HAVE_LVX_BUG_FMAX && flag_signaling_nans)"
  "fmaxnd %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "negdf2"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (neg:DF (match_operand:DF 1 "register_operand" "r")))]
  ""
  "fnegd %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "absdf2"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (abs:DF (match_operand:DF 1 "register_operand" "r")))]
  ""
  "fabsd %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "floatdidf2"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (float:DF (match_operand:DI 1 "register_operand" "r")))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "floatd.rn %0 = %1, 0";
    return "floatd.rn %0 = %1";
  }
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "floatunsdidf2"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unsigned_float:DF (match_operand:DI 1 "register_operand" "r")))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "floatud.rn %0 = %1, 0";
    return "floatud.rn %0 = %1";
  }
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "fix_truncdfdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (fix:DI (match_operand:DF 1 "register_operand" "r")))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixedd.rz %0 = %1, 0";
    return "fixedd.rz %0 = %1";
  }
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "fixuns_truncdfdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unsigned_fix:DI (match_operand:DF 1 "register_operand" "r")))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixedud.rz %0 = %1, 0";
    return "fixedud.rz %0 = %1";
  }
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_expand "truncdfhf2"
  [(set (match_operand:HF 0 "register_operand" "")
        (float_truncate:HF (match_operand:DF 1 "register_operand" "")))]
  ""
  {
    rtx tempsf = gen_reg_rtx (SFmode);
    emit_insn (gen_truncdfsf2 (tempsf, operands[1]));
    emit_insn (gen_truncsfhf2 (operands[0], tempsf));
    DONE;
  }
)

(define_insn "truncdfsf2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (float_truncate:SF (match_operand:DF 1 "register_operand" "r")))]
  ""
  "fnarrowdw %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_expand "roundevendf2"
  [(set (match_operand:DF 0 "register_operand" "")
        (match_operand:DF 1 "register_operand" ""))]
  ""
  {
    rtx cs_val = gen_reg_rtx (DImode);
    rtx tempdi = gen_reg_rtx (DImode);
    rtx tempdf = gen_reg_rtx (DFmode);
    rtx cs = gen_rtx_REG (DImode, 68);
    rtx rn = gen_rtx_CONST_STRING (VOIDmode, ".rn");
    rtx deqz = gen_rtx_CONST_STRING (VOIDmode, ".deqz");
    /* 1. Reset the error bits (IO and XIO) we check in CS */
    emit_insn (gen_lvx_get (cs_val, cs));
    emit_insn (gen_anddi3 (cs_val, cs_val, GEN_INT (0xfffffffffffffdfd)));
    emit_insn (gen_lvx_set (cs, cs_val));
    /* 2. Convert floating point to fixed point */
    emit_insn (gen_lvx_fixedd (tempdi, operands[1], const0_rtx, rn));
    emit_insn (gen_lvx_get (cs_val, cs));
    emit_insn (gen_anddi3 (cs_val, cs_val, GEN_INT (2)));
    /* 3. Convert back to floating point, and take into account corner cases. */
    emit_insn (gen_lvx_floatd (tempdf, tempdi, const0_rtx, rn));
    emit_insn (gen_lvx_selectfd (operands[0], tempdf, operands[1], cs_val, deqz));
    DONE;
  }
)

;; ---- DIVMOD*: hardware integer divide/modulo ------------------------------
;;
;; divmodw / divmoduw / divmodd / divmodud compute the quotient and the
;; remainder in one FULL-slot instruction, writing both into a 128-bit
;; register pair: quotient in the low 64 bits, remainder in the high 64.
;; All four are on lvx_v1.
;;
;; The 32-bit forms take the signextw modifier.  The scalar patterns use .sx
;; so the SImode results are sign-extended the way sub-word values are kept in
;; registers; the plain zero-extending form exists so a SIMD divmod can be
;; composed by OR-ing shifted lane results without masking.
;;
;; Division by zero yields zero rather than trapping, and INT_MIN / -1 yields
;; INT_MIN with remainder zero -- both defined, so no undefined behaviour
;; leaks.  When -mdivmod0-trap is in effect the callers below still emit the
;; explicit check.

(define_insn "lvx_divmodsi"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (unspec:TI [(match_operand:SI 1 "register_operand" "r")
                    (match_operand:SI 2 "register_operand" "r")] UNSPEC_DIVMOD))]
  ""
  "divmodw.sx %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "full")]
)

(define_insn "lvx_udivmodsi"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (unspec:TI [(match_operand:SI 1 "register_operand" "r")
                    (match_operand:SI 2 "register_operand" "r")] UNSPEC_DIVMODU))]
  ""
  "divmoduw.sx %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "full")]
)

(define_insn "lvx_divmoddi"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (unspec:TI [(match_operand:DI 1 "register_operand" "r")
                    (match_operand:DI 2 "register_operand" "r")] UNSPEC_DIVMOD))]
  ""
  "divmodd %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "full")]
)

(define_insn "lvx_udivmoddi"
  [(set (match_operand:TI 0 "register_operand" "=r")
        (unspec:TI [(match_operand:DI 1 "register_operand" "r")
                    (match_operand:DI 2 "register_operand" "r")] UNSPEC_DIVMODU))]
  ""
  "divmodud %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "full")]
)

(define_expand "divmodsi4"
  [(set (match_operand:SI 0 "register_operand" "")
        (div:SI (match_operand:SI 1 "register_operand" "")
                (match_operand:SI 2 "register_operand" "")))
   (set (match_operand:SI 3 "register_operand" "")
        (mod:SI (match_dup 1) (match_dup 2)))]
  ""
  {
    lvx_expand_divmod (operands[0], operands[3], operands[1], operands[2],
                       SImode, /*unsignedp=*/false);
    DONE;
  }
)

(define_expand "udivmodsi4"
  [(set (match_operand:SI 0 "register_operand" "")
        (udiv:SI (match_operand:SI 1 "register_operand" "")
                 (match_operand:SI 2 "register_operand" "")))
   (set (match_operand:SI 3 "register_operand" "")
        (umod:SI (match_dup 1) (match_dup 2)))]
  ""
  {
    lvx_expand_divmod (operands[0], operands[3], operands[1], operands[2],
                       SImode, /*unsignedp=*/true);
    DONE;
  }
)

(define_expand "divmoddi4"
  [(set (match_operand:DI 0 "register_operand" "")
        (div:DI (match_operand:DI 1 "register_operand" "")
                (match_operand:DI 2 "register_operand" "")))
   (set (match_operand:DI 3 "register_operand" "")
        (mod:DI (match_dup 1) (match_dup 2)))]
  ""
  {
    lvx_expand_divmod (operands[0], operands[3], operands[1], operands[2],
                       DImode, /*unsignedp=*/false);
    DONE;
  }
)

(define_expand "udivmoddi4"
  [(set (match_operand:DI 0 "register_operand" "")
        (udiv:DI (match_operand:DI 1 "register_operand" "")
                 (match_operand:DI 2 "register_operand" "")))
   (set (match_operand:DI 3 "register_operand" "")
        (umod:DI (match_dup 1) (match_dup 2)))]
  ""
  {
    lvx_expand_divmod (operands[0], operands[3], operands[1], operands[2],
                       DImode, /*unsignedp=*/true);
    DONE;
  }
)

;; ---- FSQRT*: hardware square root ------------------------------------------
;;
;; fsqrth / fsqrtw / fsqrtd, all on lvx_v1, take a floatmode rounding modifier
;; (".rn", ".rz", ... or "" for the mode in $cs).  These replace the previous
;; sqrtsf2/sqrthf2, which were guarded by flag_reciprocal_math and computed
;; x * rsqrt_seed(x) -- an approximation, and only with fast-math; without it
;; the middle end fell back to a libm call, and there was no sqrtdf2 at all.
;; The hardware gives the exact result in one instruction for all three modes.  These are the exact-result
;; instructions; the reciprocal-square-root *seed* fsrsr* is a different thing
;; and stays behind flag_reciprocal_math where it already was.

(define_insn "lvx_fsqrt<fmode>"
  [(set (match_operand:FLOATM 0 "register_operand" "=r")
        (unspec:FLOATM [(match_operand:FLOATM 1 "register_operand" "r")
                        (match_operand 2 "" "")] UNSPEC_FSQRT))]
  ""
  "fsqrt<fmode>%2 %0 = %1"
  [(set_attr "type" "fdiv")
   (set_attr "issue" "full")]
)

; `frint` rounds a float to an integral float using the rounding mode named
; by its modifier; the empty modifier means "use the rounding mode currently
; in CS", which is exactly C's rint().  Without these patterns GCC emitted a
; call to the library rint() even though the instruction exists -- found by
; auditing which ISA mnemonics the back end never emits (2026-08-04).
;
; Only rint() is provided, deliberately.  nearbyint() must not raise the
; inexact exception, and this instruction "may raise exception bits in the CS
; register" (Description.yml), so it cannot implement nearbyint.  The
; directed-rounding modifiers (.rz/.rd/.ru/.rm) would likewise give
; btrunc/floor/ceil/round, which are worth adding but are a separate change.

(define_insn "lvx_frint<fmode>"
  [(set (match_operand:FLOATM 0 "register_operand" "=r")
        (unspec:FLOATM [(match_operand:FLOATM 1 "register_operand" "r")
                        (match_operand 2 "" "")] UNSPEC_FRINT))]
  ""
  "frint<fmode>%2 %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_expand "rint<mode>2"
  [(set (match_operand:FLOATM 0 "register_operand" "")
        (unspec:FLOATM [(match_operand:FLOATM 1 "register_operand" "")]
                       UNSPEC_FRINT))]
  ""
  {
    rtx rm = gen_rtx_CONST_STRING (VOIDmode, "");
    emit_insn (gen_lvx_frint<fmode> (operands[0], operands[1], rm));
    DONE;
  }
)

(define_expand "sqrt<mode>2"
  [(set (match_operand:FLOATM 0 "register_operand" "")
        (sqrt:FLOATM (match_operand:FLOATM 1 "register_operand" "")))]
  ""
  {
    rtx rm = gen_rtx_CONST_STRING (VOIDmode, "");
    emit_insn (gen_lvx_fsqrt<fmode> (operands[0], operands[1], rm));
    DONE;
  }
)
