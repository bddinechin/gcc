;; V2SI complex

(define_expand "cmulv2si3"
[(set (match_operand:V2SI 0 "register_operand" "=r")
      (mult:V2SI (match_operand:V2SI 1 "register_operand" "r")
                (match_operand:V2SI 2 "register_operand" "r")))]
  ""
  {
    if (HAVE_LVX_CPLX_MULT_V2SI)
      emit_insn (gen_cmulv2si3_1 (operands[0], operands[1], operands[2]));
    else
      {
        rtx temp0 = gen_reg_rtx (V4SImode);
        rtx temp0_lo = gen_lowpart (V2SImode, temp0);
        rtx temp0_hi = gen_highpart (V2SImode, temp0);
        rtx temp0_lo_di = simplify_gen_subreg (DImode, temp0_lo, V2SImode, 0);
        rtx temp0_hi_di = simplify_gen_subreg (DImode, temp0_hi, V2SImode, 0);
        rtx op0_di = simplify_gen_subreg (DImode, operands[0], V2SImode, 0);
        emit_insn (gen_lvx_mm212w (temp0, operands[1], operands[2]));
        emit_insn (gen_lvx_sbmm8d (temp0_hi_di, temp0_hi_di, gen_rtx_CONST_INT (DImode, 0x0804020180402010)));
        emit_insn (gen_addv2si3 (operands[0], temp0_lo, temp0_hi));
        emit_insn (gen_subv2si3 (temp0_lo, temp0_lo, temp0_hi));
        emit_insn (gen_insvdi (op0_di, GEN_INT (32), GEN_INT (0), temp0_lo_di));
      }
    DONE;
  }
)

(define_insn "cmulv2si3_1"
  [(set (match_operand:V2SI 0 "register_operand" "=r,r,r,r")
        (unspec:V2SI [(match_operand:V2SI 1 "register_operand" "r,r,r,r")
                 (match_operand:V2SI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")] UNSPEC_CMULT))]
  "HAVE_LVX_CPLX_MULT_V2SI"
  "mulwc %0 = %1, %2"
  [(set_attr "type" "mult_int,mult_int,mult_int_x,mult_int_y")
   (set_attr "length"      "4,       4,         8,        12")]
)

(define_insn "mulconjv2si3"
  [(set (match_operand:V2SI 0 "register_operand" "=r")
        (unspec:V2SI [(unspec:V2SI [(match_operand:V2SI 1 "register_operand" "r")] UNSPEC_CONJ)
                  (match_operand:V2SI 2 "register_operand" "r")] UNSPEC_CMULT))]
  "HAVE_LVX_CPLX_MULT_V2SI"
  "mulwc.c %0 = %1, %2"
  [(set_attr "type"   "mult_int")]
)

;; V2SF complex

(define_insn "cmulv2sf3"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")
                 (match_operand:V2SF 2 "register_operand" "r")] UNSPEC_CMULT))]
  "HAVE_LVX_CPLX_MULT_V2SF"
  "fmulwc %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn "cmlav2sf4"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")
                (match_operand:V2SF 2 "register_operand" "r")
                (match_operand:V2SF 3 "register_operand" "0")] UNSPEC_CFMA))]
  "HAVE_LVX_CPLX_FMA_V2SF"
  "ffmawc %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "cnmlav2sf4"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(neg:V2SF (match_operand:V2SF 1 "register_operand" "r"))
                (match_operand:V2SF 2 "register_operand" "r")
                (match_operand:V2SF 3 "register_operand" "0")] UNSPEC_CFMA))]
  "HAVE_LVX_CPLX_FMS_V2SF"
  "ffmswc %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "addconjv2sf3"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (plus:V2SF (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")] UNSPEC_CONJ)
                 (match_operand:V2SF 2 "register_operand" "r")))]
  "HAVE_LVX_CPLX_PLUS_CONJ_V2SF"
  "faddwc.c %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn "subconjv2sf3"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (minus:V2SF (match_operand:V2SF 1 "register_operand" "r")
                  (unspec:V2SF [(match_operand:V2SF 2 "register_operand" "r")] UNSPEC_CONJ)))]
  "HAVE_LVX_CPLX_MINUS_CONJ_V2SF"
  "fsbfwc.c %0 = %2, %1"
  [(set_attr "type" "mult_fp4")]
)

(define_insn "mulconjv2sf3"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")] UNSPEC_CONJ)
                 (match_operand:V2SF 2 "register_operand" "r")] UNSPEC_CMULT))]
  "HAVE_LVX_CPLX_MULT_V2SF"
  "fmulwc.c %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

;; used by the combine pass
(define_insn "cmla_conjv2sf4"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")] UNSPEC_CONJ)
                (match_operand:V2SF 2 "register_operand" "r")
                (match_operand:V2SF 3 "register_operand" "0")] UNSPEC_CFMA))]
  "HAVE_LVX_CPLX_FMA_V2SF"
  "ffmawc.c %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

;; used by the combine pass
(define_insn "cnmla_conjv2sf4"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(neg:V2SF (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")] UNSPEC_CONJ))
                (match_operand:V2SF 2 "register_operand" "r")
                (match_operand:V2SF 3 "register_operand" "0")] UNSPEC_CFMA))]
  "HAVE_LVX_CPLX_FMS_V2SF"
  "ffmswc.c %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "conjv2sf2"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")] UNSPEC_CONJ))]
  ""
  "fnegd %0 = %1"
  [(set_attr "type" "alu_thin")]
)

(define_expand "crot90v2sf2"
  [(match_operand:V2SF 0 "register_operand" "")
   (match_operand:V2SF 1 "register_operand" "")]
  ""
  {
    emit_insn (gen_conjv2sf2 (operands[0], operands[1]));
    emit_insn (gen_lvx_sbmm8d (simplify_gen_subreg (DImode, operands[0], V2SFmode, 0),
			       simplify_gen_subreg (DImode, operands[0], V2SFmode, 0),
			       gen_rtx_CONST_INT (DImode, 0x0804020180402010)));
    DONE;
  }
)

(define_expand "crot270v2sf2"
  [(match_operand:V2SF 0 "register_operand" "")
   (match_operand:V2SF 1 "register_operand" "")]
  ""
  {
    emit_insn (gen_lvx_sbmm8d (simplify_gen_subreg (DImode, operands[0], V2SFmode, 0),
			       simplify_gen_subreg (DImode, operands[1], V2SFmode, 0),
			       gen_rtx_CONST_INT (DImode, 0x0804020180402010)));
    emit_insn (gen_conjv2sf2 (operands[0], operands[0]));
    DONE;
  }
)

;; Patterns for cadd90sc3 and cadd270sc3 are not implemented because LVX cannot
;; accelerate these operations

;; V2DF

(define_insn "addconjv2df3"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (plus:V2DF (unspec:V2DF [(match_operand:V2DF 1 "register_operand" "r")] UNSPEC_CONJ)
                 (match_operand:V2DF 2 "register_operand" "r")))]
  "HAVE_LVX_CPLX_PLUS_CONJ_V2DF"
  "fadddc.c %0 = %1, %2"
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn "subconjv2df3"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (minus:V2DF (match_operand:V2DF 1 "register_operand" "r")
                  (unspec:V2DF [(match_operand:V2DF 2 "register_operand" "r")] UNSPEC_CONJ)))]
  "HAVE_LVX_CPLX_MINUS_CONJ_V2DF"
  "fsbfdc.c %0 = %2, %1"
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn_and_split "conjv2df2"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand" "r")] UNSPEC_CONJ))
   (clobber (match_scratch:DF 2))
   (clobber (match_scratch:DF 3))
   (clobber (match_scratch:DF 4))
   (clobber (match_scratch:DF 5))]
  ""
  "#"
  "reload_completed"
  [(set (match_dup 2) (match_dup 4))
   (set (match_dup 3) (neg:DF (match_dup 5)))]
  {
    for (int i = 0; i < 4; ++i)
      operands[2 + i] =
        simplify_subreg (DFmode, operands[i/2], V2DFmode, i&1?8:0);
  }
)

(define_expand "fast_cmulv2df3"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (mult:V2DF (match_operand:V2DF 1 "register_operand" "r")
                 (match_operand:V2DF 2 "register_operand" "r")))]
  ""
  {
    rtx a_r = simplify_gen_subreg (DFmode, operands[1], V2DFmode, 0);
    rtx a_i = simplify_gen_subreg (DFmode, operands[1], V2DFmode, 8);
    rtx b_r = simplify_gen_subreg (DFmode, operands[2], V2DFmode, 0);
    rtx b_i = simplify_gen_subreg (DFmode, operands[2], V2DFmode, 8);
    rtx res_r = simplify_gen_subreg (DFmode, operands[0], V2DFmode, 0);
    rtx res_i = simplify_gen_subreg (DFmode, operands[0], V2DFmode, 8);
    emit_insn (gen_muldf3 (res_r, a_r, b_r));
    emit_insn (gen_muldf3 (res_i, a_r, b_i));
    emit_insn (gen_fnmadf4 (res_r, a_i, b_i, res_r));
    emit_insn (gen_fmadf4 (res_i, a_i, b_r, res_i));
    DONE;
  }
)

;; V2DI complex

(define_insn_and_split "conjv2di2"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V2DI 1 "register_operand" "r")] UNSPEC_CONJ))
   (clobber (match_scratch:DI 2))
   (clobber (match_scratch:DI 3))
   (clobber (match_scratch:DI 4))
   (clobber (match_scratch:DI 5))]
  ""
  "#"
  "reload_completed"
  [(set (match_dup 2) (match_dup 4))
   (set (match_dup 3) (neg:DI (match_dup 5)))]
  {
    for (int i = 0; i < 4; ++i)
      operands[2 + i] =
        simplify_subreg (DImode, operands[i/2], V2DImode, i&1?8:0);
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

;; vectors of complex

;; 64-bit complex

;; V8QI complex

(define_expand "conjv8qi2"
  [(set (match_operand:V8QI 0 "register_operand" "=r")
        (unspec:V8QI [(match_operand:V8QI 1 "register_operand" "r")] UNSPEC_CONJ))]
  "HAVE_LVX_CPLX_CONJ_V8QI"
  {
    rtx temp = gen_reg_rtx (V8QImode);
    rtx temp_di = simplify_gen_subreg (DImode, temp, V8QImode, 0);
    rtx op0_di = simplify_gen_subreg (DImode, operands[0], V8QImode, 0);
    rtx op1_di = simplify_gen_subreg (DImode, operands[1], V8QImode, 0);
    emit_insn (gen_negv8qi2 (temp, operands[1]));
    emit_insn (gen_lvx_sbmm8d (op0_di, op1_di, gen_rtx_CONST_INT (DImode, 0x0040001000040001)));
    emit_insn (gen_lvx_sbmm8d (temp_di, temp_di, gen_rtx_CONST_INT (DImode, 0x8000200008000200)));
    emit_insn (gen_iordi3 (op0_di, op0_di, temp_di));
    DONE;
  }
)

;; V4HI complex

(define_insn "addconjv4hi3"
  [(set (match_operand:V4HI 0 "register_operand" "=r")
        (plus:V4HI (unspec:V4HI [(match_operand:V4HI 1 "register_operand" "r")] UNSPEC_CONJ)
                    (match_operand:V4HI 2 "register_operand" "r")))]
  "HAVE_LVX_CPLX_PLUS_CONJ_V4HI"
  "addhcp.c %0 = %1, %2"
  [(set_attr "type" "alu_lite")]
)

(define_insn "subconjv4hi3"
  [(set (match_operand:V4HI 0 "register_operand" "=r")
        (minus:V4HI (match_operand:V4HI 1 "register_operand" "r")
                     (unspec:V4HI [(match_operand:V4HI 2 "register_operand" "r")] UNSPEC_CONJ)))]
  "HAVE_LVX_CPLX_MINUS_CONJ_V4HI"
  "sbfhcp.c %0 = %2, %1"
  [(set_attr "type" "alu_lite")]
)

(define_expand "conjv4hi2"
  [(set (match_operand:V4HI 0 "register_operand" "=r")
        (unspec:V4HI [(match_operand:V4HI 1 "register_operand" "r")] UNSPEC_CONJ))]
  ""
  {
    if (HAVE_LVX_CPLX_CONJ_V4HI)
      emit_insn (gen_conjv4hi2_1 (operands[0], operands[1]));
    else
      emit_insn (gen_conjv4hi2_2 (operands[0], operands[1]));
    DONE;
  }
  [(set_attr "type" "alu_tiny_x")
   (set_attr "length" "8")]
)

(define_insn "conjv4hi2_1"
  [(set (match_operand:V4HI 0 "register_operand" "=r")
        (unspec:V4HI [(match_operand:V4HI 1 "register_operand" "r")] UNSPEC_CONJ))]
  "HAVE_LVX_CPLX_CONJ_V4HI"
  "addhcp.c %0 = %1, 0"
  [(set_attr "type" "alu_lite_x")
   (set_attr "length" "8")]
)

(define_insn_and_split "conjv4hi2_2"
  [(set (match_operand:V4HI 0 "register_operand" "=r")
        (unspec:V4HI [(match_operand:V4HI 1 "register_operand" "r")] UNSPEC_CONJ))
   (clobber (match_scratch:V4HI 2 "=&r"))]
  "!HAVE_LVX_CPLX_CONJ_V4HI"
  "#"
  "!HAVE_LVX_CPLX_CONJ_V4HI && reload_completed"
  [(set (match_dup 2) (neg:V4HI (match_dup 1)))
   (set (subreg:DI (match_dup 0) 0)
        (unspec:DI [(subreg:DI (match_dup 1) 0) (const_int 35253091566081)]
                   UNSPEC_SBMM8D))
   (set (subreg:DI (match_dup 2) 0)
        (unspec:DI [(subreg:DI (match_dup 2) 0) (const_int -9205357638210813952)]
                   UNSPEC_SBMM8D))
   (set (subreg:DI (match_dup 0) 0) (ior:DI (subreg:DI (match_dup 0) 0)
                                            (subreg:DI (match_dup 2) 0)))]
  {
    if (GET_CODE (operands[2]) == SCRATCH)
      operands[2] = gen_reg_rtx (V4HImode);
  }
)

;; 128-bit complex

;; V16QI complex

(define_expand "conjv16qi2"
  [(set (match_operand:V16QI 0 "register_operand" "=r")
        (unspec:V16QI [(match_operand:V16QI 1 "register_operand" "r")] UNSPEC_CONJ))]
  "HAVE_LVX_CPLX_CONJ_V16QI"
  {
    emit_insn (gen_conjv8qi2 (gen_lowpart (V8QImode, operands[0]),
                              gen_lowpart (V8QImode, operands[1])));
    emit_insn (gen_conjv8qi2 (gen_highpart (V8QImode, operands[0]),
                              gen_highpart (V8QImode, operands[1])));
    DONE;
  }
)

;; V128CC

(define_insn_and_split "addconj<mode>3"
  [(set (match_operand:V128CC 0 "register_operand" "=r")
        (plus:V128CC (unspec:V128CC [(match_operand:V128CC 1 "register_operand" "r")] UNSPEC_CONJ)
                     (match_operand:V128CC 2 "register_operand" "r")))
   (clobber (match_scratch:<HALF> 3))
   (clobber (match_scratch:<HALF> 4))
   (clobber (match_scratch:<HALF> 5))
   (clobber (match_scratch:<HALF> 6))
   (clobber (match_scratch:<HALF> 7))
   (clobber (match_scratch:<HALF> 8))]
  "" "#" ""
  [(set (match_dup 3) (plus:<HALF> (unspec:<HALF> [(match_dup 5)] UNSPEC_CONJ) (match_dup 7)))
   (set (match_dup 4) (plus:<HALF> (unspec:<HALF> [(match_dup 6)] UNSPEC_CONJ) (match_dup 8)))]
  {
    for (int i = 0; i < 6; ++i)
      operands[3 + i] =
        simplify_gen_subreg (<HALF>mode, operands[i/2], <MODE>mode, i&1?8:0);
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "subconj<mode>3"
  [(set (match_operand:V128CC 0 "register_operand" "=r")
        (minus:V128CC (match_operand:V128CC 1 "register_operand" "r")
                      (unspec:V128CC [(match_operand:V128CC 2 "register_operand" "r")] UNSPEC_CONJ)))
   (clobber (match_scratch:<HALF> 3))
   (clobber (match_scratch:<HALF> 4))
   (clobber (match_scratch:<HALF> 5))
   (clobber (match_scratch:<HALF> 6))
   (clobber (match_scratch:<HALF> 7))
   (clobber (match_scratch:<HALF> 8))]
  "" "#" ""
  [(set (match_dup 3) (minus:<HALF> (match_dup 5) (unspec:<HALF> [(match_dup 7)] UNSPEC_CONJ)))
   (set (match_dup 4) (minus:<HALF> (match_dup 6) (unspec:<HALF> [(match_dup 8)] UNSPEC_CONJ)))]
  {
    for (int i = 0; i < 6; ++i)
      operands[3 + i] =
        simplify_gen_subreg (<HALF>mode, operands[i/2], <MODE>mode, i&1?8:0);
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "conj<mode>2_1"
  [(set (match_operand:V128CC 0 "register_operand" "=r")
        (unspec:V128CC [(match_operand:V128CC 1 "register_operand" "r")] UNSPEC_CONJ))
   (clobber (match_scratch:<HALF> 2))
   (clobber (match_scratch:<HALF> 3))
   (clobber (match_scratch:<HALF> 4))
   (clobber (match_scratch:<HALF> 5))]
  "" "#" ""
  [(set (match_dup 2) (unspec:<HALF> [(match_dup 4)] UNSPEC_CONJ))
   (set (match_dup 3) (unspec:<HALF> [(match_dup 5)] UNSPEC_CONJ))]
  {
    for (int i = 0; i < 4; ++i)
      operands[2 + i] =
        simplify_gen_subreg (<HALF>mode, operands[i/2], <MODE>mode, i&1?8:0);
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "conj<mode>2"
  [(set (match_operand:V128CB 0 "register_operand" "=r")
        (unspec:V128CB [(match_operand:V128CB 1 "register_operand" "r")] UNSPEC_CONJ))]
  ""
  {
    if (HAVE_LVX_CPLX_CONJ_<MODE>)
      emit_insn (gen_conj<mode>2_1 (operands[0], operands[1]));
    else
      {
         emit_insn (gen_conj<half>2 (gen_lowpart (<HALF>mode, operands[0]),
                                     gen_lowpart (<HALF>mode, operands[1])));
         emit_insn (gen_conj<half>2 (gen_highpart (<HALF>mode, operands[0]),
                                     gen_highpart (<HALF>mode, operands[1])));
      }
    DONE;
  }
)

;; V4SI complex

(define_expand "cmulv4si3"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V4SI 1 "register_operand" "r")
                   (match_operand:V4SI 2 "register_operand" "r")] UNSPEC_CMULT))]
  ""
  {
    if (HAVE_LVX_CPLX_MULT_V4SI)
      emit_insn (gen_cmulv4si3_1 (operands[0], operands[1], operands[2]));
    else
      {
        emit_insn (gen_cmulv2si3 (gen_lowpart (V2SImode, operands[0]),
                                gen_lowpart (V2SImode, operands[1]),
                                gen_lowpart (V2SImode, operands[2])));
        emit_insn (gen_cmulv2si3 (gen_highpart (V2SImode, operands[0]),
                                gen_highpart (V2SImode, operands[1]),
                                gen_highpart (V2SImode, operands[2])));
      }
    DONE;
  }
)

(define_insn_and_split "cmulv4si3_1"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V4SI 1 "register_operand" "r")
                    (match_operand:V4SI 2 "register_operand" "r")] UNSPEC_CMULT))]
  "HAVE_LVX_CPLX_MULT_V4SI"
  "#"
  "HAVE_LVX_CPLX_MULT_V4SI && reload_completed"
  [(set (subreg:V2SI (match_dup 0) 0)
        (unspec:V2SI [(subreg:V2SI (match_dup 1) 0)
                  (subreg:V2SI (match_dup 2) 0)] UNSPEC_CMULT))
   (set (subreg:V2SI (match_dup 0) 8)
        (unspec:V2SI [(subreg:V2SI (match_dup 1) 8)
                  (subreg:V2SI (match_dup 2) 8)] UNSPEC_CMULT))]
  ""
)

(define_insn_and_split "cmulconjv4si3"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(unspec:V4SI [(match_operand:V4SI 1 "register_operand" "r")] UNSPEC_CONJ)
                    (match_operand:V4SI 2 "register_operand" "r")] UNSPEC_CMULT))]
  "HAVE_LVX_CPLX_MULT_V4SI"
  "#"
  "HAVE_LVX_CPLX_MULT_V4SI && reload_completed"
  [(set (subreg:V2SI (match_dup 0) 0)
        (unspec:V2SI [(unspec:V2SI [(subreg:V2SI (match_dup 1) 0)] UNSPEC_CONJ)
                  (subreg:V2SI (match_dup 2) 0)] UNSPEC_CMULT))
   (set (subreg:V2SI (match_dup 0) 8)
        (unspec:V2SI [(unspec:V2SI [(subreg:V2SI (match_dup 1) 8)] UNSPEC_CONJ)
                  (subreg:V2SI (match_dup 2) 8)] UNSPEC_CMULT))]
  ""
)

;; V4SF complex

(define_expand "cmulv4sf3"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                   (match_operand:V4SF 2 "register_operand" "r")] UNSPEC_CMULT))]
  ""
  {
    if (!HAVE_LVX_CPLX_MULT_V4SF)
      emit_insn (gen_cmulv4sf3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_cmulv4sf3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn_and_split "cmulv4sf3_1"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                   (match_operand:V4SF 2 "register_operand" "r")] UNSPEC_CMULT))]
  "!HAVE_LVX_CPLX_MULT_V4SF"
  "#"
  "!HAVE_LVX_CPLX_MULT_V4SF && reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 0)
                 (subreg:V2SF (match_dup 2) 0)] UNSPEC_CMULT))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 8)
                 (subreg:V2SF (match_dup 2) 8)] UNSPEC_CMULT))]
  ""
)

(define_insn "cmulv4sf3_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                 (match_operand:V4SF 2 "register_operand" "r")] UNSPEC_CMULT))]
  "HAVE_LVX_CPLX_MULT_V4SF"
  "fmulwcp %0 = %1, %2"
  [(set_attr "type" "mult_int")]
)

(define_insn "cmlav4sf4"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                  (match_operand:V4SF 2 "register_operand" "r")
                  (match_operand:V4SF 3 "register_operand" "0")] UNSPEC_CFMA))]
  "HAVE_LVX_CPLX_FMA_V4SF"
  "ffmawcp %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "cnmlav4sf4"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(neg:V4SF (match_operand:V4SF 1 "register_operand" "r"))
                  (match_operand:V4SF 2 "register_operand" "r")
                  (match_operand:V4SF 3 "register_operand" "0")] UNSPEC_CFMA))]
  "HAVE_LVX_CPLX_FMS_V4SF"
  "ffmswcp %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "addconjv4sf3"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (plus:V4SF (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")] UNSPEC_CONJ)
                   (match_operand:V4SF 2 "register_operand" "r")))]
  "HAVE_LVX_CPLX_PLUS_CONJ_V4SF"
  "faddwcp.c %0 = %1, %2"
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn "subconjv4sf3"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (minus:V4SF (match_operand:V4SF 1 "register_operand" "r")
                    (unspec:V4SF [(match_operand:V4SF 2 "register_operand" "r")] UNSPEC_CONJ)))]
  "HAVE_LVX_CPLX_MINUS_CONJ_V4SF"
  "fsbfwcp.c %0 = %2, %1"
  [(set_attr "type" "mulwq_fp4")]
)

(define_expand "mulconjv4sf3"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")] UNSPEC_CONJ)
                   (match_operand:V4SF 2 "register_operand" "r")] UNSPEC_CMULT))]
  ""
  {
    if (!HAVE_LVX_CPLX_MULT_V4SF)
      emit_insn (gen_mulconjv4sf3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_mulconjv4sf3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn_and_split "mulconjv4sf3_1"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")] UNSPEC_CONJ)
                   (match_operand:V4SF 2 "register_operand" "r")] UNSPEC_CMULT))]
  "!HAVE_LVX_CPLX_MULT_V4SF"
  "#"
  "!HAVE_LVX_CPLX_MULT_V4SF && reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(unspec:V2SF [(subreg:V2SF (match_dup 1) 0)] UNSPEC_CONJ)
                 (subreg:V2SF (match_dup 2) 0)] UNSPEC_CMULT))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(unspec:V2SF [(subreg:V2SF (match_dup 1) 8)] UNSPEC_CONJ)
                 (subreg:V2SF (match_dup 2) 8)] UNSPEC_CMULT))]
  ""
)

(define_insn "mulconjv4sf3_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")] UNSPEC_CONJ)
                   (match_operand:V4SF 2 "register_operand" "r")] UNSPEC_CMULT))]
  "HAVE_LVX_CPLX_MULT_V4SF"
  "fmulwcp.c %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn "fmaconjv4sf4"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")] UNSPEC_CONJ)
                  (match_operand:V4SF 2 "register_operand" "r")
                  (match_operand:V4SF 3 "register_operand" "0")] UNSPEC_CFMA))]
  "HAVE_LVX_CPLX_FMA_V4SF"
  "ffmawcp.c %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "fnmaconjv4sf4"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(neg:V4SF (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")] UNSPEC_CONJ))
                  (match_operand:V4SF 2 "register_operand" "r")
                  (match_operand:V4SF 3 "register_operand" "0")] UNSPEC_CFMA))]
  "HAVE_LVX_CPLX_FMS_V4SF"
  "ffmswcp.c %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn_and_split "conjv4sf2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")] UNSPEC_CONJ))]
  "" "#" ""
  [(set (subreg:V2SF (match_dup 0) 0) (unspec:V2SF [(subreg:V2SF (match_dup 1) 0)] UNSPEC_CONJ))
   (set (subreg:V2SF (match_dup 0) 8) (unspec:V2SF [(subreg:V2SF (match_dup 1) 8)] UNSPEC_CONJ))]
  ""
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

