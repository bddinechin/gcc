
;; MADDDT, MDSBDT

(define_expand "lvx_madddt"
  [(match_operand:V2DI 0 "register_operand" "")
   (match_operand:DI 1 "register_operand" "")
   (match_operand:DI 2 "register_operand" "")
   (match_operand:V2DI 3 "register_operand" "")
   (match_operand 4 "" "")]
  ""
  {
    lvx_expand_builtin_maddt (operands, 1);
    DONE;
  }
)

(define_expand "lvx_msbfdt"
  [(match_operand:V2DI 0 "register_operand" "")
   (match_operand:DI 1 "register_operand" "")
   (match_operand:DI 2 "register_operand" "")
   (match_operand:V2DI 3 "register_operand" "")
   (match_operand 4 "" "")]
  ""
  {
    lvx_expand_builtin_maddt (operands, 0);
    DONE;
  }
)

;; ADD*

(define_expand "lvx_add<suffix>"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand:SIDI 2 "register_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[3], 0);
    if (!*xstr)
      emit_insn (gen_add<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 's')
      emit_insn (gen_ssadd<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'u')
      emit_insn (gen_usadd<mode>3 (operands[0], operands[1], operands[2]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_expand "lvx_add<suffix>"
  [(match_operand:LVXI 0 "register_operand" "")
   (match_operand:LVXI 1 "register_operand" "")
   (match_operand:LVXI 2 "register_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[3], 0);
    if (!*xstr)
      emit_insn (gen_add<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 's')
      emit_insn (gen_ssadd<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'u')
      emit_insn (gen_usadd<mode>3 (operands[0], operands[1], operands[2]));
    else
      gcc_unreachable ();
    DONE;
  }
)


;; SBF*

(define_expand "lvx_sbf<suffix>"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand:SIDI 2 "register_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[3], 0);
    if (!*xstr)
      emit_insn (gen_sub<mode>3 (operands[0], operands[2], operands[1]));
    else if (xstr[1] == 's')
      emit_insn (gen_sssub<mode>3 (operands[0], operands[2], operands[1]));
    else if (xstr[1] == 'u')
      emit_insn (gen_ussub<mode>3 (operands[0], operands[2], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_expand "lvx_sbf<suffix>"
  [(match_operand:LVXI 0 "register_operand" "")
   (match_operand:LVXI 1 "register_operand" "")
   (match_operand:LVXI 2 "register_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[3], 0);
    if (!*xstr)
      emit_insn (gen_sub<mode>3 (operands[0], operands[2], operands[1]));
    else if (xstr[1] == 's')
      emit_insn (gen_sssub<mode>3 (operands[0], operands[2], operands[1]));
    else if (xstr[1] == 'u')
      emit_insn (gen_ussub<mode>3 (operands[0], operands[2], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)


;; NEG*

(define_expand "lvx_neg<suffix>"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[2], 0);
    if (!*xstr)
      emit_insn (gen_neg<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 's')
      emit_insn (gen_ssneg<mode>2 (operands[0], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_expand "lvx_neg<suffix>"
  [(match_operand:LVXI 0 "register_operand" "")
   (match_operand:LVXI 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[2], 0);
    if (!*xstr)
      emit_insn (gen_neg<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 's')
      emit_insn (gen_ssneg<mode>2 (operands[0], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)


;; ABS*

(define_expand "lvx_abs<suffix>"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[2], 0);
    if (!*xstr)
      emit_insn (gen_abs<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 's')
      emit_insn (gen_ssabs<mode>2 (operands[0], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_expand "lvx_abs<suffix>"
  [(match_operand:LVXI 0 "register_operand" "")
   (match_operand:LVXI 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[2], 0);
    if (!*xstr)
      emit_insn (gen_abs<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 's')
      emit_insn (gen_ssabs<mode>2 (operands[0], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)


;; ABD*

(define_expand "lvx_abd<suffix>"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand:SIDI 2 "register_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[3], 0);
    if (!*xstr)
      emit_insn (gen_abd<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 's')
      emit_insn (gen_abds<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'u')
      emit_insn (gen_abdu<mode>3 (operands[0], operands[1], operands[2]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_expand "lvx_abd<suffix>"
  [(match_operand:LVXI 0 "register_operand" "")
   (match_operand:LVXI 1 "register_operand" "")
   (match_operand:LVXI 2 "register_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[3], 0);
    if (!*xstr)
      emit_insn (gen_abd<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 's')
      emit_insn (gen_abds<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'u')
      emit_insn (gen_abdu<mode>3 (operands[0], operands[1], operands[2]));
    else
      gcc_unreachable ();
    DONE;
  }
)


;; AVG*

(define_expand "lvx_avgw"
  [(match_operand:SI 0 "register_operand" "")
   (match_operand:SI 1 "register_operand" "")
   (match_operand:SI 2 "register_w32_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[3], 0);
    if (!*xstr)
      emit_insn (gen_avgsi3_floor (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'r' && !xstr[2])
      emit_insn (gen_avgsi3_ceil (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'u' && !xstr[2])
      emit_insn (gen_uavgsi3_floor (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'r' && xstr[2] == 'u')
      emit_insn (gen_uavgsi3_ceil (operands[0], operands[1], operands[2]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_expand "lvx_avg<suffix>"
  [(match_operand:WIDENI 0 "register_operand" "")
   (match_operand:WIDENI 1 "register_operand" "")
   (match_operand:WIDENI 2 "register_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[3], 0);
    if (!*xstr)
      emit_insn (gen_avg<mode>3_floor (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'r' && !xstr[2])
      emit_insn (gen_avg<mode>3_ceil (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'u' && !xstr[2])
      emit_insn (gen_uavg<mode>3_floor (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'r' && xstr[2] == 'u')
      emit_insn (gen_uavg<mode>3_ceil (operands[0], operands[1], operands[2]));
    else
      gcc_unreachable ();
    DONE;
  }
)


;; MULX*

(define_expand "lvx_mulx<widenx>"
  [(match_operand:<WIDE> 0 "register_operand" "")
   (match_operand:VWXI 1 "register_operand" "")
   (match_operand:VWXI 2 "register_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[3], 0);
    if (!*xstr)
      emit_insn (gen_lvx_mul<widenx> (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'u')
      emit_insn (gen_lvx_mulu<widenx> (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 's')
      emit_insn (gen_lvx_mulsu<widenx> (operands[0], operands[1], operands[2]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_insn "lvx_mul<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (mult:<WIDE> (sign_extend:<WIDE> (match_operand:S64I 1 "register_operand" "r"))
                     (sign_extend:<WIDE> (match_operand:S64I 2 "register_operand" "r"))))]
  ""
  "mul<widenx> %0 = %1, %2"
  [(set_attr "type" "mult_int")]
)

(define_insn "lvx_mulu<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (mult:<WIDE> (zero_extend:<WIDE> (match_operand:S64I 1 "register_operand" "r"))
                     (zero_extend:<WIDE> (match_operand:S64I 2 "register_operand" "r"))))]
  ""
  "mulu<widenx> %0 = %1, %2"
  [(set_attr "type" "mult_int")]
)

(define_insn "lvx_mulsu<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (mult:<WIDE> (sign_extend:<WIDE> (match_operand:S64I 1 "register_operand" "r"))
                     (zero_extend:<WIDE> (match_operand:S64I 2 "register_operand" "r"))))]
  ""
  "mulsu<widenx> %0 = %1, %2"
  [(set_attr "type" "mult_int")]
)

(define_insn_and_split "lvx_mul<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=&r")
        (mult:<WIDE> (sign_extend:<WIDE> (match_operand:S128I 1 "register_operand" "r"))
                     (sign_extend:<WIDE> (match_operand:S128I 2 "register_operand" "r"))))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HWIDE> (match_dup 0) 0)
        (mult:<HWIDE> (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 0))
                      (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 0))))
   (set (subreg:<HWIDE> (match_dup 0) 16)
        (mult:<HWIDE> (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 8))
                      (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 8))))]
  ""
  [(set_attr "type" "mult_int")]
)

(define_insn_and_split "lvx_mulu<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=&r")
        (mult:<WIDE> (zero_extend:<WIDE> (match_operand:S128I 1 "register_operand" "r"))
                     (zero_extend:<WIDE> (match_operand:S128I 2 "register_operand" "r"))))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HWIDE> (match_dup 0) 0)
        (mult:<HWIDE> (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 0))
                      (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 0))))
   (set (subreg:<HWIDE> (match_dup 0) 16)
        (mult:<HWIDE> (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 8))
                      (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 8))))]
  ""
  [(set_attr "type" "mult_int")]
)

(define_insn_and_split "lvx_mulsu<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=&r")
        (mult:<WIDE> (sign_extend:<WIDE> (match_operand:S128I 1 "register_operand" "r"))
                     (zero_extend:<WIDE> (match_operand:S128I 2 "register_operand" "r"))))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HWIDE> (match_dup 0) 0)
        (mult:<HWIDE> (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 0))
                      (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 0))))
   (set (subreg:<HWIDE> (match_dup 0) 16)
        (mult:<HWIDE> (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 8))
                      (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 8))))]
  ""
  [(set_attr "type" "mult_int")]
)


;; MADDX*

(define_expand "lvx_maddx<widenx>"
  [(match_operand:<WIDE> 0 "register_operand" "")
   (match_operand:VWXI 1 "register_operand" "")
   (match_operand:VWXI 2 "register_operand" "")
   (match_operand:<WIDE> 3 "register_operand" "")
   (match_operand 4 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[4], 0);
    if (!*xstr)
      emit_insn (gen_lvx_madd<widenx> (operands[0], operands[1], operands[2], operands[3]));
    else if (xstr[1] == 'u')
      emit_insn (gen_lvx_maddu<widenx> (operands[0], operands[1], operands[2], operands[3]));
    else if (xstr[1] == 's')
      emit_insn (gen_lvx_maddsu<widenx> (operands[0], operands[1], operands[2], operands[3]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_insn "lvx_madd<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (plus:<WIDE> (mult:<WIDE> (sign_extend:<WIDE> (match_operand:S64I 1 "register_operand" "r"))
                                  (sign_extend:<WIDE> (match_operand:S64I 2 "register_operand" "r")))
                     (match_operand:<WIDE> 3 "register_operand" "0")))]
  ""
  "madd<widenx> %0 = %1, %2"
  [(set_attr "type" "madd_int")]
)

(define_insn "lvx_maddu<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (plus:<WIDE> (mult:<WIDE> (zero_extend:<WIDE> (match_operand:S64I 1 "register_operand" "r"))
                                  (zero_extend:<WIDE> (match_operand:S64I 2 "register_operand" "r")))
                     (match_operand:<WIDE> 3 "register_operand" "0")))]
  ""
  "maddu<widenx> %0 = %1, %2"
  [(set_attr "type" "madd_int")]
)

(define_insn "lvx_maddsu<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (plus:<WIDE> (mult:<WIDE> (sign_extend:<WIDE> (match_operand:S64I 1 "register_operand" "r"))
                                  (zero_extend:<WIDE> (match_operand:S64I 2 "register_operand" "r")))
                     (match_operand:<WIDE> 3 "register_operand" "0")))]
  ""
  "maddsu<widenx> %0 = %1, %2"
  [(set_attr "type" "madd_int")]
)

(define_insn_and_split "lvx_madd<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=&r")
        (plus:<WIDE> (mult:<WIDE> (sign_extend:<WIDE> (match_operand:S128I 1 "register_operand" "r"))
                                  (sign_extend:<WIDE> (match_operand:S128I 2 "register_operand" "r")))
                      (match_operand:<WIDE> 3 "register_operand" "0")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HWIDE> (match_dup 0) 0)
        (plus:<HWIDE> (mult:<HWIDE> (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 0))
                                    (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 0)))
                      (subreg:<HWIDE> (match_dup 3) 0)))
   (set (subreg:<HWIDE> (match_dup 0) 16)
        (plus:<HWIDE> (mult:<HWIDE> (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 8))
                                    (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 8)))
                      (subreg:<HWIDE> (match_dup 3) 16)))]
  ""
  [(set_attr "type" "madd_int")]
)

(define_insn_and_split "lvx_maddu<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=&r")
        (plus:<WIDE> (mult:<WIDE> (zero_extend:<WIDE> (match_operand:S128I 1 "register_operand" "r"))
                                  (zero_extend:<WIDE> (match_operand:S128I 2 "register_operand" "r")))
                     (match_operand:<WIDE> 3 "register_operand" "0")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HWIDE> (match_dup 0) 0)
        (plus:<HWIDE> (mult:<HWIDE> (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 0))
                                    (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 0)))
                      (subreg:<HWIDE> (match_dup 3) 0)))
   (set (subreg:<HWIDE> (match_dup 0) 16)
        (plus:<HWIDE> (mult:<HWIDE> (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 8))
                                    (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 8)))
                      (subreg:<HWIDE> (match_dup 3) 16)))]
  ""
  [(set_attr "type" "madd_int")]
)

(define_insn_and_split "lvx_maddsu<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=&r")
        (plus:<WIDE> (mult:<WIDE> (sign_extend:<WIDE> (match_operand:S128I 1 "register_operand" "r"))
                                  (zero_extend:<WIDE> (match_operand:S128I 2 "register_operand" "r")))
                     (match_operand:<WIDE> 3 "register_operand" "0")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HWIDE> (match_dup 0) 0)
        (plus:<HWIDE> (mult:<HWIDE> (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 0))
                                    (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 0)))
                      (subreg:<HWIDE> (match_dup 3) 0)))
   (set (subreg:<HWIDE> (match_dup 0) 16)
        (plus:<HWIDE> (mult:<HWIDE> (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 8))
                                    (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 8)))
                      (subreg:<HWIDE> (match_dup 3) 16)))]
  ""
  [(set_attr "type" "madd_int")]
)


;; MSBFX*

(define_expand "lvx_msbfx<widenx>"
  [(match_operand:<WIDE> 0 "register_operand" "")
   (match_operand:VWXI 1 "register_operand" "")
   (match_operand:VWXI 2 "register_operand" "")
   (match_operand:<WIDE> 3 "register_operand" "")
   (match_operand 4 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[4], 0);
    if (!*xstr)
      emit_insn (gen_lvx_msbf<widenx> (operands[0], operands[1], operands[2], operands[3]));
    else if (xstr[1] == 'u')
      emit_insn (gen_lvx_msbfu<widenx> (operands[0], operands[1], operands[2], operands[3]));
    else if (xstr[1] == 's')
      emit_insn (gen_lvx_msbfsu<widenx> (operands[0], operands[1], operands[2], operands[3]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_insn "lvx_msbf<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (minus:<WIDE> (match_operand:<WIDE> 3 "register_operand" "0")
                      (mult:<WIDE> (sign_extend:<WIDE> (match_operand:S64I 1 "register_operand" "r"))
                                   (sign_extend:<WIDE> (match_operand:S64I 2 "register_operand" "r")))))]
  ""
  "msbf<widenx> %0 = %1, %2"
  [(set_attr "type" "madd_int")]
)

(define_insn "lvx_msbfu<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (minus:<WIDE> (match_operand:<WIDE> 3 "register_operand" "0")
                      (mult:<WIDE> (zero_extend:<WIDE> (match_operand:S64I 1 "register_operand" "r"))
                                   (zero_extend:<WIDE> (match_operand:S64I 2 "register_operand" "r")))))]
  ""
  "msbfu<widenx> %0 = %1, %2"
  [(set_attr "type" "madd_int")]
)

(define_insn "lvx_msbfsu<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (minus:<WIDE> (match_operand:<WIDE> 3 "register_operand" "0")
                      (mult:<WIDE> (sign_extend:<WIDE> (match_operand:S64I 1 "register_operand" "r"))
                                   (zero_extend:<WIDE> (match_operand:S64I 2 "register_operand" "r")))))]
  ""
  "msbfsu<widenx> %0 = %1, %2"
  [(set_attr "type" "madd_int")]
)

(define_insn_and_split "lvx_msbf<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=&r")
        (minus:<WIDE> (match_operand:<WIDE> 3 "register_operand" "0")
                      (mult:<WIDE> (sign_extend:<WIDE> (match_operand:S128I 1 "register_operand" "r"))
                                   (sign_extend:<WIDE> (match_operand:S128I 2 "register_operand" "r")))))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HWIDE> (match_dup 0) 0)
        (minus:<HWIDE> (subreg:<HWIDE> (match_dup 3) 0)
                       (mult:<HWIDE> (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 0))
                                     (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 0)))))
   (set (subreg:<HWIDE> (match_dup 0) 16)
        (minus:<HWIDE> (subreg:<HWIDE> (match_dup 3) 16)
                       (mult:<HWIDE> (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 8))
                                     (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 8)))))]
  ""
  [(set_attr "type" "madd_int")]
)

(define_insn_and_split "lvx_msbfu<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=&r")
        (minus:<WIDE> (match_operand:<WIDE> 3 "register_operand" "0")
                      (mult:<WIDE> (zero_extend:<WIDE> (match_operand:S128I 1 "register_operand" "r"))
                                   (zero_extend:<WIDE> (match_operand:S128I 2 "register_operand" "r")))))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HWIDE> (match_dup 0) 0)
        (minus:<HWIDE> (subreg:<HWIDE> (match_dup 3) 0)
                       (mult:<HWIDE> (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 0))
                                     (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 0)))))
   (set (subreg:<HWIDE> (match_dup 0) 16)
        (minus:<HWIDE> (subreg:<HWIDE> (match_dup 3) 16)
                       (mult:<HWIDE> (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 8))
                                     (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 8)))))]
  ""
  [(set_attr "type" "madd_int")]
)

(define_insn_and_split "lvx_msbfsu<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=&r")
        (minus:<WIDE> (match_operand:<WIDE> 3 "register_operand" "0")
                      (mult:<WIDE> (sign_extend:<WIDE> (match_operand:S128I 1 "register_operand" "r"))
                                   (zero_extend:<WIDE> (match_operand:S128I 2 "register_operand" "r")))))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HWIDE> (match_dup 0) 0)
        (minus:<HWIDE> (subreg:<HWIDE> (match_dup 3) 0)
                       (mult:<HWIDE> (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 0))
                                     (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 0)))))
   (set (subreg:<HWIDE> (match_dup 0) 16)
        (minus:<HWIDE> (subreg:<HWIDE> (match_dup 3) 16)
                       (mult:<HWIDE> (sign_extend:<HWIDE> (subreg:<HALF> (match_dup 1) 8))
                                     (zero_extend:<HWIDE> (subreg:<HALF> (match_dup 2) 8)))))]
  ""
  [(set_attr "type" "madd_int")]
)

(define_insn "lvx_mms212w"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V2SI 1 "register_operand" "r")
                      (match_operand:V2SI 2 "register_operand" "r")
		      (match_operand:V4SI 3 "register_operand" "0")] UNSPEC_MMS))]
  ""
  "mms212w %0 = %1, %2"
  [(set_attr "type" "madd_int")]
)

;; SHL*

(define_expand "lvx_shl<suffix>"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand:SI 2 "register_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    operands[2] = force_reg (SImode, operands[2]);
    const char *xstr = XSTR (operands[3], 0);
    if (!*xstr)
      emit_insn (gen_ashl<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 's')
      emit_insn (gen_ssashl<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'u')
      emit_insn (gen_usashl<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'r')
      emit_insn (gen_rotl<mode>3 (operands[0], operands[1], operands[2]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_expand "lvx_shl<suffix>s"
  [(match_operand:LVXI 0 "register_operand" "")
   (match_operand:LVXI 1 "register_operand" "")
   (match_operand:SI 2 "register_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    operands[2] = force_reg (SImode, operands[2]);
    const char *xstr = XSTR (operands[3], 0);
    if (!*xstr)
      emit_insn (gen_ashl<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 's')
      emit_insn (gen_ssashl<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'u')
      emit_insn (gen_usashl<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'r')
      emit_insn (gen_rotl<mode>3 (operands[0], operands[1], operands[2]));
    else
      gcc_unreachable ();
    DONE;
  }
)


;; SHR*

(define_expand "lvx_shr<suffix>"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand:SI 2 "register_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    operands[2] = force_reg (SImode, operands[2]);
    const char *xstr = XSTR (operands[3], 0);
    if (!*xstr)
      emit_insn (gen_lshr<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'a' && !xstr[2])
      emit_insn (gen_ashr<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'a' && xstr[2] == 's')
      emit_insn (gen_sshr<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'r')
      emit_insn (gen_rotr<mode>3 (operands[0], operands[1], operands[2]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_expand "lvx_shr<suffix>s"
  [(match_operand:LVXI 0 "register_operand" "")
   (match_operand:LVXI 1 "register_operand" "")
   (match_operand:SI 2 "register_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    operands[2] = force_reg (SImode, operands[2]);
    const char *xstr = XSTR (operands[3], 0);
    if (!*xstr)
      emit_insn (gen_lshr<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'a' && !xstr[2])
      emit_insn (gen_ashr<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'a' && xstr[2] == 's')
      emit_insn (gen_sshr<mode>3 (operands[0], operands[1], operands[2]));
    else if (xstr[1] == 'r')
      emit_insn (gen_rotr<mode>3 (operands[0], operands[1], operands[2]));
    else
      gcc_unreachable ();
    DONE;
  }
)


;; BITCNT*

(define_expand "lvx_bitcnt<suffix>"
  [(match_operand:SIDI 0 "register_operand" "")
   (match_operand:SIDI 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[2], 0);
    if (!*xstr)
      emit_insn (gen_popcount<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 'l' && xstr[2] == 'z')
      emit_insn (gen_clz<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 'l' && xstr[2] == 's')
      emit_insn (gen_clrsb<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 't')
      emit_insn (gen_ctz<mode>2 (operands[0], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_expand "lvx_bitcnt<suffix>"
  [(match_operand:S64I 0 "register_operand" "")
   (match_operand:S64I 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[2], 0);
    if (!*xstr)
      emit_insn (gen_popcount<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 'l' && xstr[2] == 'z')
      emit_insn (gen_clz<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 'l' && xstr[2] == 's')
      emit_insn (gen_clrsb<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 't')
      emit_insn (gen_ctz<mode>2 (operands[0], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_expand "lvx_bitcnt<suffix>"
  [(match_operand:V128J 0 "register_operand" "")
   (match_operand:V128J 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[2], 0);
    if (!*xstr)
      emit_insn (gen_popcount<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 'l' && xstr[2] == 'z')
      emit_insn (gen_clz<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 'l' && xstr[2] == 's')
      emit_insn (gen_clrsb<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 't')
      emit_insn (gen_ctz<mode>2 (operands[0], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_expand "lvx_bitcnt<suffix>"
  [(match_operand:V256J 0 "register_operand" "")
   (match_operand:V256J 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[2], 0);
    if (!*xstr)
      emit_insn (gen_popcount<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 'l' && xstr[2] == 'z')
      emit_insn (gen_clz<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 'l' && xstr[2] == 's')
      emit_insn (gen_clrsb<mode>2 (operands[0], operands[1]));
    else if (xstr[1] == 't')
      emit_insn (gen_ctz<mode>2 (operands[0], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)


;; WIDEN*, ZX*, SX*, QX*

(define_expand "lvx_widen<widenx>"
  [(match_operand:<WIDE> 0 "register_operand" "")
   (match_operand:WIDENI 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[2], 0);
    if (!*xstr)
      emit_insn (gen_lvx_sx<widenx> (operands[0], operands[1]));
    else if (xstr[1] == 'z')
      emit_insn (gen_lvx_zx<widenx> (operands[0], operands[1]));
    else if (xstr[1] == 'q')
      emit_insn (gen_lvx_qx<widenx> (operands[0], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_expand "lvx_zx<widenx>"
  [(match_operand:<WIDE> 0 "register_operand")
   (match_operand:WIDENI 1 "register_operand")]
  ""
  {
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += UNITS_PER_WORD)
      {
        rtx op1_i = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, offset);
        rtx op0_i = simplify_gen_subreg (<WCHUNK>mode, operands[0], <WIDE>mode, offset * 2);
        emit_insn (gen_lvx_zx<wchunkx>_ (op0_i, op1_i));
      }
    DONE;
  }
)

(define_expand "lvx_zx<widenx>_"
  [(set (match_operand:<WIDE> 0 "register_operand")
        (unspec:<WIDE> [(match_operand:S64M 1 "register_operand")
                        (match_dup 2) (match_dup 3)] UNSPEC_ZX64))]
  ""
  {
    if (!HAVE_LVX_ZERO_EXTEND_<WIDE>_<MODE>)
      {
        operands[2] = gen_reg_rtx (DImode);
        rtx valuev8qi_l ATTRIBUTE_UNUSED = GEN_INT (0x0008000400020001);
        rtx valuev4hi_l ATTRIBUTE_UNUSED = GEN_INT (0x0000080400000201);
        emit_insn (gen_rtx_SET (operands[2], value<mode>_l));
        operands[3] = gen_reg_rtx (DImode);
        rtx valuev8qi_m ATTRIBUTE_UNUSED = GEN_INT (0x0080004000200010);
        rtx valuev4hi_m ATTRIBUTE_UNUSED = GEN_INT (0x0000804000002010);
        emit_insn (gen_rtx_SET (operands[3], value<mode>_m));
      }
    else
      {
        rtx src = gen_rtx_UNSPEC (<WIDE>mode, gen_rtvec (1, operands[1]), UNSPEC_ZX64);
        emit_insn (gen_rtx_SET (operands[0], src));
        DONE;
      }
  }
)

(define_insn "*lvx_zx<widenx>_1"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (unspec:<WIDE> [(match_operand:S64M 1 "register_operand" "r")
                        (match_operand:DI 2 "register_operand" "r")
                        (match_operand:DI 3 "register_operand" "r")] UNSPEC_ZX64))]
  "!HAVE_LVX_ZERO_EXTEND_<WIDE>_<MODE>"
  "sbmm8d %x0 = %1, %2\n\tsbmm8d %y0 = %1, %3"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn "*lvx_zx<widenx>_2"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (unspec:<WIDE> [(match_operand:S64M 1 "register_operand" "r")] UNSPEC_ZX64))]
  "HAVE_LVX_ZERO_EXTEND_<WIDE>_<MODE>"
  "zxl<hwidenx> %x0 = %1\n\tzxm<hwidenx> %y0 = %1"
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)

(define_insn "lvx_zxwdp_"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V2SI 1 "register_operand" "r")] UNSPEC_ZXWDP))]
  ""
  "zxwd %x0 = %1\n\tsrld %y0 = %1, 32"
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)

(define_expand "lvx_sx<widenx>"
  [(match_operand:<WIDE> 0 "register_operand")
   (match_operand:WIDENI 1 "register_operand")]
  ""
  {
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += UNITS_PER_WORD)
      {
        rtx op1_i = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, offset);
        rtx op0_i = simplify_gen_subreg (<WCHUNK>mode, operands[0], <WIDE>mode, offset * 2);
        emit_insn (gen_lvx_sx<wchunkx>_ (op0_i, op1_i));
      }
    DONE;
  }
)

(define_insn "lvx_sx<widenx>_"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (unspec:<WIDE> [(match_operand:S64M 1 "register_operand" "r")] UNSPEC_SX64))]
  ""
  "sxl<hwidenx> %x0 = %1\n\tsxm<hwidenx> %y0 = %1"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn "lvx_sxwdp_"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V2SI 1 "register_operand" "r")] UNSPEC_SXWDP))]
  ""
  "sxwd %x0 = %1\n\tsrad %y0 = %1, 32"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_expand "lvx_qx<widenx>"
  [(match_operand:<WIDE> 0 "register_operand")
   (match_operand:WIDENI 1 "register_operand")]
  ""
  {
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += UNITS_PER_WORD)
      {
        rtx op1_i = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, offset);
        rtx op0_i = simplify_gen_subreg (<WCHUNK>mode, operands[0], <WIDE>mode, offset * 2);
        emit_insn (gen_lvx_qx<wchunkx>_ (op0_i, op1_i));
      }
    DONE;
  }
)

(define_expand "lvx_qx<widenx>_"
  [(set (match_operand:<WIDE> 0 "register_operand")
        (unspec:<WIDE> [(match_operand:S64M 1 "register_operand")
                        (match_dup 2) (match_dup 3)] UNSPEC_QX64))]
  ""
  {
    operands[2] = gen_reg_rtx (DImode);
    rtx valuev8qi_l ATTRIBUTE_UNUSED = GEN_INT (0x0800040002000100);
    rtx valuev4hi_l ATTRIBUTE_UNUSED = GEN_INT (0x0804000002010000);
    emit_insn (gen_rtx_SET (operands[2], value<mode>_l));
    operands[3] = gen_reg_rtx (DImode);
    rtx valuev8qi_m ATTRIBUTE_UNUSED = GEN_INT (0x8000400020001000);
    rtx valuev4hi_m ATTRIBUTE_UNUSED = GEN_INT (0x8040000020100000);
    emit_insn (gen_rtx_SET (operands[3], value<mode>_m));
  }
)

(define_insn "*lvx_qx<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (unspec:<WIDE> [(match_operand:S64M 1 "register_operand" "r")
                        (match_operand:DI 2 "register_operand" "r")
                        (match_operand:DI 3 "register_operand" "r")] UNSPEC_QX64))]
  ""
  "sbmm8d %x0 = %1, %2\n\tsbmm8d %y0 = %1, %3"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_expand "lvx_qxwdp_"
  [(set (match_operand:V2DI 0 "register_operand")
        (unspec:V2DI [(match_operand:V2SI 1 "register_operand")
                      (match_dup 2)] UNSPEC_QXWDP))]
  ""
  {
    operands[2] = gen_reg_rtx (DImode);
    emit_insn (gen_rtx_SET (operands[2], GEN_INT (0xFFFFFFFF00000000)));
  }
)

(define_insn_and_split "*lvx_qxwdp"
  [(set (match_operand:V2DI 0 "register_operand" "=&r")
        (unspec:V2DI [(match_operand:V2SI 1 "register_operand" "r")
                      (match_operand:DI 2 "register_operand" "r")] UNSPEC_QXWDP))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DI (match_dup 0) 0)
        (unspec:DI [(match_dup 1) (const_int 32)] UNSPEC_SLLD))
   (set (subreg:DI (match_dup 0) 8)
        (unspec:DI [(match_dup 1) (match_dup 2)] UNSPEC_ANDD))]
  ""
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)


;; NARROW*, TRUNC*, FRACT*, SAT*, SATU*

(define_expand "lvx_narrow<truncx>"
  [(match_operand:WIDENI 0 "register_operand" "")
   (match_operand:<WIDE> 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[2], 0);
    if (!*xstr)
      emit_insn (gen_lvx_trunc<truncx> (operands[0], operands[1]));
    else if (xstr[1] == 'q')
      emit_insn (gen_lvx_fract<truncx> (operands[0], operands[1]));
    else if (xstr[1] == 's')
      emit_insn (gen_lvx_sat<truncx> (operands[0], operands[1]));
    else if (xstr[1] == 'u')
      emit_insn (gen_lvx_satu<truncx> (operands[0], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_expand "lvx_trunc<truncx>"
  [(match_operand:WIDENI 0 "register_operand")
   (match_operand:<WIDE> 1 "register_operand")]
  ""
  {
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += UNITS_PER_WORD)
      {
        rtx op1_i = simplify_gen_subreg (<WCHUNK>mode, operands[1], <WIDE>mode, offset * 2);
        rtx op0_i = simplify_gen_subreg (<CHUNK>mode, operands[0], <MODE>mode, offset);
        emit_insn (gen_lvx_trunc<nchunkx>_ (op0_i, op1_i));
      }
    DONE;
  }
)

(define_expand "lvx_trunc<truncx>_"
  [(parallel
    [(set (match_operand:S64M 0 "register_operand")
	  (unspec:S64M [(match_operand:<WIDE> 1 "register_operand")
			(match_dup 2) (match_dup 3)] UNSPEC_TRUNC))
     (clobber (match_dup 4))]
  )]
  ""
  {
    operands[2] = gen_reg_rtx (DImode);
    rtx valuev8qi_l ATTRIBUTE_UNUSED = GEN_INT (0x0000000040100401);
    rtx valuev4hi_l ATTRIBUTE_UNUSED = GEN_INT (0x0000000020100201);
    emit_insn (gen_rtx_SET (operands[2], value<mode>_l));
    operands[3] = gen_reg_rtx (DImode);
    rtx valuev8qi_m ATTRIBUTE_UNUSED = GEN_INT (0x4010040100000000);
    rtx valuev4hi_m ATTRIBUTE_UNUSED = GEN_INT (0x2010020100000000);
    emit_insn (gen_rtx_SET (operands[3], value<mode>_m));
    operands[4] = gen_rtx_SCRATCH (<WIDE>mode);
  }
)

(define_insn_and_split "*lvx_trunc<truncx>"
  [(set (match_operand:S64M 0 "register_operand" "=r")
        (unspec:S64M [(match_operand:<WIDE> 1 "register_operand" "r")
                      (match_operand:DI 2 "register_operand" "r")
                      (match_operand:DI 3 "register_operand" "r")] UNSPEC_TRUNC))
   (clobber (match_scratch:<WIDE> 4 "=r"))]
  ""
  "#"
  "reload_completed"
  [(set (match_dup 4)
        (unspec:<WIDE> [(match_dup 1) (match_dup 2) (match_dup 3)] UNSPEC_SBMM8DXY))
   (set (match_dup 0)
        (unspec:S64M [(subreg:S64M (match_dup 4) 0)
                      (subreg:S64M (match_dup 4) 8)] UNSPEC_XORD))]
  ""
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)

(define_expand "lvx_truncdwp_"
  [(match_operand:V2SI 0 "register_operand")
   (match_operand:V2DI 1 "register_operand")]
  ""
  {
    rtx op0_l = gen_reg_rtx (V2SImode);
    rtx op0_m = gen_reg_rtx (V2SImode);
    rtx op1_l = simplify_gen_subreg (DImode, operands[1], V2DImode, 0);
    rtx op1_m = simplify_gen_subreg (DImode, operands[1], V2DImode, 8);
    emit_insn (gen_lvx_truncldw (op0_l, op1_l));
    emit_insn (gen_lvx_truncmdw (op0_m, op1_m));
    rtx xord = gen_rtx_UNSPEC (V2SImode, gen_rtvec (2, op0_l, op0_m), UNSPEC_XORD);
    emit_insn (gen_rtx_SET (operands[0], xord));
    DONE;
  }
)

(define_insn "lvx_truncldw"
  [(set (match_operand:V2SI 0 "register_operand" "=r")
        (unspec:V2SI [(match_operand:DI 1 "register_operand" "r")] UNSPEC_TRUNCL))]
  ""
  "zxwd %0 = %1"
  [(set_attr "type" "alu_tiny")]
)

(define_insn "lvx_truncmdw"
  [(set (match_operand:V2SI 0 "register_operand" "=r")
        (unspec:V2SI [(match_operand:DI 1 "register_operand" "r") (const_int 32)] UNSPEC_TRUNCM))]
  ""
  "slld %0 = %1, 32"
  [(set_attr "type" "alu_tiny")]
)

(define_expand "lvx_fract<truncx>"
  [(match_operand:WIDENI 0 "register_operand")
   (match_operand:<WIDE> 1 "register_operand")]
  ""
  {
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += UNITS_PER_WORD)
      {
        rtx op1_i = simplify_gen_subreg (<WCHUNK>mode, operands[1], <WIDE>mode, offset * 2);
        rtx op0_i = simplify_gen_subreg (<CHUNK>mode, operands[0], <MODE>mode, offset);
        emit_insn (gen_lvx_fract<nchunkx>_ (op0_i, op1_i));
      }
    DONE;
  }
)

(define_expand "lvx_fract<truncx>_"
  [(parallel
    [(set (match_operand:S64M 0 "register_operand")
	  (unspec:S64M [(match_operand:<WIDE> 1 "register_operand")
			(match_dup 2) (match_dup 3)] UNSPEC_FRACT))
     (clobber (match_dup 4))]
  )]
  ""
  {
    operands[2] = gen_reg_rtx (DImode);
    rtx valuev8qi_l ATTRIBUTE_UNUSED = GEN_INT (0x0000000080200802);
    rtx valuev4hi_l ATTRIBUTE_UNUSED = GEN_INT (0x0000000080400804);
    emit_insn (gen_rtx_SET (operands[2], value<mode>_l));
    operands[3] = gen_reg_rtx (DImode);
    rtx valuev8qi_m ATTRIBUTE_UNUSED = GEN_INT (0x8020080200000000);
    rtx valuev4hi_m ATTRIBUTE_UNUSED = GEN_INT (0x8040080400000000);
    emit_insn (gen_rtx_SET (operands[3], value<mode>_m));
    operands[4] = gen_rtx_SCRATCH (<WIDE>mode);
  }
)

(define_insn_and_split "*lvx_fract<truncx>"
  [(set (match_operand:S64M 0 "register_operand" "=r")
        (unspec:S64M [(match_operand:<WIDE> 1 "register_operand" "r")
                      (match_operand:DI 2 "register_operand" "r")
                      (match_operand:DI 3 "register_operand" "r")] UNSPEC_FRACT))
   (clobber (match_scratch:<WIDE> 4 "=r"))]
  ""
  "#"
  "reload_completed"
  [(set (match_dup 4)
        (unspec:<WIDE> [(match_dup 1) (match_dup 2) (match_dup 3)] UNSPEC_SBMM8DXY))
   (set (match_dup 0)
        (unspec:S64M [(subreg:S64M (match_dup 4) 0)
                      (subreg:S64M (match_dup 4) 8)] UNSPEC_XORD))]
  ""
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)

(define_expand "lvx_fractdwp_"
  [(match_operand:V2SI 0 "register_operand")
   (match_operand:V2DI 1 "register_operand")]
  ""
  {
    rtx op0_l = gen_reg_rtx (V2SImode);
    rtx op0_m = gen_reg_rtx (V2SImode);
    rtx op1_l = simplify_gen_subreg (DImode, operands[1], V2DImode, 0);
    rtx op1_m = simplify_gen_subreg (DImode, operands[1], V2DImode, 8);
    emit_insn (gen_lvx_fractldw (op0_l, op1_l));
    emit_insn (gen_lvx_fractmdw (op0_m, op1_m));
    rtx xord = gen_rtx_UNSPEC (V2SImode, gen_rtvec (2, op0_l, op0_m), UNSPEC_XORD);
    emit_insn (gen_rtx_SET (operands[0], xord));
    DONE;
  }
)

(define_insn "lvx_fractldw"
  [(set (match_operand:V2SI 0 "register_operand" "=r")
        (unspec:V2SI [(match_operand:DI 1 "register_operand" "r") (const_int 32)] UNSPEC_SRLD))]
  ""
  "srld %0 = %1, 32"
  [(set_attr "type" "alu_tiny")]
)

(define_expand "lvx_fractmdw"
  [(match_operand:V2SI 0 "register_operand")
   (match_operand:DI 1 "register_operand")]
  ""
  {
    rtx mask = gen_reg_rtx (DImode);
    emit_insn (gen_rtx_SET (mask, GEN_INT (0xFFFFFFFF00000000)));
    rtx opnd1 = simplify_gen_subreg (V2SImode, operands[1], DImode, 0);
    rtx andd = gen_rtx_UNSPEC (V2SImode, gen_rtvec (2, opnd1, mask), UNSPEC_ANDD);
    emit_insn (gen_rtx_SET (operands[0], andd));
    DONE;
  }
)

(define_expand "lvx_sat<truncx>"
  [(match_operand:WIDENI 0 "register_operand")
   (match_operand:<WIDE> 1 "register_operand")]
  ""
  {
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += UNITS_PER_WORD)
      {
        rtx op1_i = simplify_gen_subreg (<WCHUNK>mode, operands[1], <WIDE>mode, offset * 2);
        rtx op0_i = simplify_gen_subreg (<CHUNK>mode, operands[0], <MODE>mode, offset);
        emit_insn (gen_lvx_sat<nchunkx>_ (op0_i, op1_i));
      }
    DONE;
  }
)

(define_expand "lvx_sat<truncx>_"
  [(match_operand:S64M 0 "register_operand")
   (match_operand:<WIDE> 1 "register_operand")]
  ""
  {
    rtx saturated = gen_reg_rtx (<WIDE>mode);
    rtx lshift = GEN_INT (GET_MODE_UNIT_BITSIZE (<MODE>mode));
    emit_insn (gen_ssashl<wide>3 (saturated, operands[1], lshift));
    emit_insn (gen_lvx_fract<truncx> (operands[0], saturated));
    DONE;
  }
)

(define_expand "lvx_satdwp_"
  [(match_operand:V2SI 0 "register_operand")
   (match_operand:V2DI 1 "register_operand")]
  ""
  {
    rtx op0_l = gen_reg_rtx (V2SImode);
    rtx op0_m = gen_reg_rtx (V2SImode);
    rtx op1_l = simplify_gen_subreg (DImode, operands[1], V2DImode, 0);
    rtx op1_m = simplify_gen_subreg (DImode, operands[1], V2DImode, 8);
    emit_insn (gen_lvx_satldw (op0_l, op1_l));
    emit_insn (gen_lvx_satmdw (op0_m, op1_m));
    rtx xord = gen_rtx_UNSPEC (V2SImode, gen_rtvec (2, op0_l, op0_m), UNSPEC_XORD);
    emit_insn (gen_rtx_SET (operands[0], xord));
    DONE;
  }
)

(define_expand "lvx_satldw"
  [(match_operand:V2SI 0 "register_operand" "=r")
   (match_operand:DI 1 "register_operand" "r")]
  ""
  {
    rtx saturated = gen_reg_rtx (DImode);
    emit_insn (gen_ssashldi3 (saturated, operands[1], GEN_INT (32)));
    emit_insn (gen_lvx_fractldw (operands[0], saturated));
    DONE;
  }
)

(define_expand "lvx_satmdw"
  [(match_operand:V2SI 0 "register_operand" "=r")
   (match_operand:DI 1 "register_operand" "r")]
  ""
  {
    rtx saturated = gen_rtx_SUBREG (DImode, operands[0], 0);
    emit_insn (gen_ssashldi3 (saturated, operands[1], GEN_INT (32)));
    DONE;
  }
)

(define_expand "lvx_satu<truncx>"
  [(match_operand:WIDENI 0 "register_operand")
   (match_operand:<WIDE> 1 "register_operand")]
  ""
  {
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += UNITS_PER_WORD)
      {
        rtx op1_i = simplify_gen_subreg (<WCHUNK>mode, operands[1], <WIDE>mode, offset * 2);
        rtx op0_i = simplify_gen_subreg (<CHUNK>mode, operands[0], <MODE>mode, offset);
        emit_insn (gen_lvx_satu<nchunkx>_ (op0_i, op1_i));
      }
    DONE;
  }
)

(define_expand "lvx_satu<truncx>_"
  [(match_operand:S64M 0 "register_operand")
   (match_operand:<WIDE> 1 "register_operand")]
  ""
  {
    rtx zero = gen_reg_rtx (<HWIDE>mode);
    rtx lower = gen_reg_rtx (<WIDE>mode);
    rtx upper = gen_reg_rtx (<WIDE>mode);
    rtx maxvalv4hi ATTRIBUTE_UNUSED = gen_rtx_CONST_VECTOR (V4HImode,
                                           gen_rtvec (4, GEN_INT (0xFF), GEN_INT (0xFF),
                                                         GEN_INT (0xFF), GEN_INT (0xFF)));
    rtx maxvalv2si ATTRIBUTE_UNUSED = gen_rtx_CONST_VECTOR (V2SImode,
                                           gen_rtvec (2, GEN_INT (0xFFFF), GEN_INT (0xFFFF)));
    rtx zero_chunk = gen_rtx_VEC_DUPLICATE (<WIDE>mode, zero);
    rtx maxval_chunk = gen_rtx_VEC_DUPLICATE (<WIDE>mode, maxval<hwide>);
    emit_insn (gen_rtx_SET (zero, CONST0_RTX (<HWIDE>mode)));
    emit_insn (gen_rtx_SET (lower, gen_rtx_SMAX (<WIDE>mode, operands[1], zero_chunk)));
    emit_insn (gen_rtx_SET (upper, gen_rtx_SMIN (<WIDE>mode, lower, maxval_chunk)));
    emit_insn (gen_lvx_trunc<truncx> (operands[0], upper));
    DONE;
  }
)

(define_expand "lvx_satudwp_"
  [(match_operand:V2SI 0 "register_operand")
   (match_operand:V2DI 1 "register_operand")]
  ""
  {
    rtx op0_l = gen_reg_rtx (V2SImode);
    rtx op0_m = gen_reg_rtx (V2SImode);
    rtx op1_l = simplify_gen_subreg (DImode, operands[1], V2DImode, 0);
    rtx op1_m = simplify_gen_subreg (DImode, operands[1], V2DImode, 8);
    emit_insn (gen_lvx_satuldw (op0_l, op1_l));
    emit_insn (gen_lvx_satumdw (op0_m, op1_m));
    rtx xord = gen_rtx_UNSPEC (V2SImode, gen_rtvec (2, op0_l, op0_m), UNSPEC_XORD);
    emit_insn (gen_rtx_SET (operands[0], xord));
    DONE;
  }
)

(define_expand "lvx_satuldw"
  [(match_operand:V2SI 0 "register_operand" "=r")
   (match_operand:DI 1 "register_operand" "r")]
  ""
  {
    rtx maxval = gen_reg_rtx (DImode);
    emit_insn (gen_rtx_SET (maxval, GEN_INT (0xFFFFFFFF)));
    rtx lower = gen_reg_rtx (DImode), upper = gen_rtx_SUBREG (DImode, operands[0], 0);
    emit_insn (gen_smaxdi3 (lower, operands[1], const0_rtx));
    emit_insn (gen_smindi3 (upper, lower, maxval));
    DONE;
  }
)

(define_expand "lvx_satumdw"
  [(match_operand:V2SI 0 "register_operand" "=r")
   (match_operand:DI 1 "register_operand" "r")]
  ""
  {
    rtx maxval = gen_reg_rtx (DImode);
    emit_insn (gen_rtx_SET (maxval, GEN_INT (0xFFFFFFFF)));
    rtx lower = gen_reg_rtx (DImode), upper = gen_reg_rtx (DImode);
    emit_insn (gen_smaxdi3 (lower, operands[1], const0_rtx));
    emit_insn (gen_smindi3 (upper, lower, maxval));
    emit_insn (gen_lvx_truncmdw (operands[0], upper));
    DONE;
  }
)


;; WIDENE*, SXE*, ZXE*, QXE*

(define_expand "lvx_widene<hwidenx>"
  [(match_operand:<HWIDE> 0 "register_operand" "")
   (match_operand:WIDENI 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[2], 0);
    if (!*xstr)
      {
        scalar_mode inner_mode = GET_MODE_INNER (<MODE>mode);
        unsigned inner_mode_size = GET_MODE_SIZE (inner_mode);
        if (inner_mode_size * 2 < UNITS_PER_WORD)
          {
            rtx op1 = simplify_gen_subreg (<HWIDE>mode, operands[1], <MODE>mode, 0);
            rtx bits = GEN_INT (inner_mode_size * BITS_PER_UNIT);
            emit_insn (gen_ashl<hwide>3 (operands[0], op1, bits));
            emit_insn (gen_ashr<hwide>3 (operands[0], operands[0], bits));
          }
        else if (inner_mode == SImode)
          {
            unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
            for (unsigned offset = 0; offset < mode_size; offset += UNITS_PER_WORD)
              {
                rtx op1_i = simplify_gen_subreg (SImode, operands[1], <MODE>mode, offset);
                rtx op0_i = simplify_gen_subreg (DImode, operands[0], <HWIDE>mode, offset);
                emit_insn (gen_extendsidi2 (op0_i, op1_i));
              }
          }
        else
          gcc_unreachable ();
      }
    else if (xstr[1] == 'z')
      emit_insn (gen_lvx_zxe<hwidenx> (operands[0], operands[1]));
    else if (xstr[1] == 'q')
      emit_insn (gen_lvx_qxe<hwidenx> (operands[0], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_insn "lvx_zxebhq"
  [(set (match_operand:V4HI 0 "register_operand" "=r")
        (unspec:V4HI [(match_operand:V8QI 1 "register_operand" "r")] UNSPEC_ZXE))]
  ""
  "andd %0 = %1, 0x00FF00FF.@"
  [(set_attr "type" "alu_tiny_x")
   (set_attr "length"        "8")]
)

(define_insn "lvx_zxebho"
  [(set (match_operand:V8HI 0 "register_operand" "=r")
        (unspec:V8HI [(match_operand:V16QI 1 "register_operand" "r")] UNSPEC_ZXE))]
  ""
  "andd %x0 = %x1, 0x00FF00FF.@\n\tandd %y0 = %y1, 0x00FF00FF.@"
  [(set_attr "type" "alu_tiny_x2_x")
   (set_attr "length"          "16")]
)

(define_insn "lvx_zxebhx"
  [(set (match_operand:V16HI 0 "register_operand" "=r")
        (unspec:V16HI [(match_operand:V32QI 1 "register_operand" "r")] UNSPEC_ZXE))]
  ""
  {
    return "andd %x0 = %x1, 0x00FF00FF.@\n\tandd %y0 = %y1, 0x00FF00FF.@\n\t"
           "andd %z0 = %z1, 0x00FF00FF.@\n\tandd %t0 = %t1, 0x00FF00FF.@";
  }
  [(set_attr "type" "alu_tiny_x4_x")
   (set_attr "length"          "32")]
)

(define_insn_and_split "lvx_zxebhv"
  [(set (match_operand:V32HI 0 "register_operand" "=r")
        (unspec:V32HI [(match_operand:V64QI 1 "register_operand" "r")] UNSPEC_ZXE))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V16HI (match_dup 0) 0)
        (unspec:V16HI [(subreg:V32QI (match_dup 1) 0)] UNSPEC_ZXE))
   (set (subreg:V16HI (match_dup 0) 32)
        (unspec:V16HI [(subreg:V32QI (match_dup 1) 32)] UNSPEC_ZXE))]
  ""
)

(define_insn "lvx_zxehwp"
  [(set (match_operand:V2SI 0 "register_operand" "=r")
        (unspec:V2SI [(match_operand:V4HI 1 "register_operand" "r")] UNSPEC_ZXE))]
  ""
  "andd %0 = %1, 0x0000FFFF.@"
  [(set_attr "type" "alu_tiny_x")
   (set_attr "length"        "8")]
)

(define_insn "lvx_zxehwq"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V8HI 1 "register_operand" "r")] UNSPEC_ZXE))]
  ""
  "andd %x0 = %x1, 0x0000FFFF.@\n\tandd %y0 = %y1, 0x0000FFFF.@"
  [(set_attr "type" "alu_tiny_x2_x")
   (set_attr "length"          "16")]
)

(define_insn "lvx_zxehwo"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (unspec:V8SI [(match_operand:V16HI 1 "register_operand" "r")] UNSPEC_ZXE))]
  ""
  {
    return "andd %x0 = %x1, 0x0000FFFF.@\n\tandd %y0 = %y1, 0x0000FFFF.@\n\t"
           "andd %z0 = %z1, 0x0000FFFF.@\n\tandd %t0 = %t1, 0x0000FFFF.@";
  }
  [(set_attr "type" "alu_tiny_x4_x")
   (set_attr "length"          "32")]
)

(define_insn "lvx_zxewd"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unspec:DI [(match_operand:V2SI 1 "register_operand" "r")] UNSPEC_ZXE))]
  ""
  "zxwd %0 = %1"
  [(set_attr "type" "alu_tiny")
   (set_attr "length"      "4")]
)

(define_insn "lvx_zxewdp"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V4SI 1 "register_operand" "r")] UNSPEC_ZXE))]
  ""
  "zxwd %x0 = %x1\n\tzxwd %y0 = %y1"
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)

(define_insn "lvx_zxewdq"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (unspec:V4DI [(match_operand:V8SI 1 "register_operand" "r")] UNSPEC_ZXE))]
  ""
  {
    return "zxwd %x0 = %x1\n\tzxwd %y0 = %y1\n\t"
           "zxwd %z0 = %z1\n\tzxwd %t0 = %t1";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn "lvx_qxebhq"
  [(set (match_operand:V4HI 0 "register_operand" "=r")
        (unspec:V4HI [(match_operand:V8QI 1 "register_operand" "r")] UNSPEC_QXE))]
  ""
  "sllhq %0 = %1, 8"
  [(set_attr "type" "alu_thin")]
)

(define_insn "lvx_qxebho"
  [(set (match_operand:V8HI 0 "register_operand" "=r")
        (unspec:V8HI [(match_operand:V16QI 1 "register_operand" "r")] UNSPEC_QXE))]
  ""
  "sllhq %x0 = %x1, 8\n\tsllhq %y0 = %y1, 8"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn_and_split "lvx_qxebhx"
  [(set (match_operand:V16HI 0 "register_operand" "=r")
        (unspec:V16HI [(match_operand:V32QI 1 "register_operand" "r")] UNSPEC_QXE))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V8HI (match_dup 0) 0)
        (unspec:V8HI [(subreg:V16QI (match_dup 1) 0)] UNSPEC_QXE))
   (set (subreg:V8HI (match_dup 0) 16)
        (unspec:V8HI [(subreg:V16QI (match_dup 1) 16)] UNSPEC_QXE))]
  ""
)

(define_insn_and_split "lvx_qxebhv"
  [(set (match_operand:V32HI 0 "register_operand" "=r")
        (unspec:V32HI [(match_operand:V64QI 1 "register_operand" "r")] UNSPEC_QXE))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V8HI (match_dup 0) 0)
        (unspec:V8HI [(subreg:V16QI (match_dup 1) 0)] UNSPEC_QXE))
   (set (subreg:V8HI (match_dup 0) 16)
        (unspec:V8HI [(subreg:V16QI (match_dup 1) 16)] UNSPEC_QXE))
   (set (subreg:V8HI (match_dup 0) 32)
        (unspec:V8HI [(subreg:V16QI (match_dup 1) 32)] UNSPEC_QXE))
   (set (subreg:V8HI (match_dup 0) 48)
        (unspec:V8HI [(subreg:V16QI (match_dup 1) 48)] UNSPEC_QXE))]
  ""
)

(define_insn "lvx_qxehwp"
  [(set (match_operand:V2SI 0 "register_operand" "=r")
        (unspec:V2SI [(match_operand:V4HI 1 "register_operand" "r")] UNSPEC_QXE))]
  ""
  "sllwp %0 = %1, 16"
  [(set_attr "type" "alu_thin")]
)

(define_insn "lvx_qxehwq"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V8HI 1 "register_operand" "r")] UNSPEC_QXE))]
  ""
  "sllwp %x0 = %x1, 16\n\tsllwp %y0 = %y1, 16"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn_and_split "lvx_qxehwo"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (unspec:V8SI [(match_operand:V16HI 1 "register_operand" "r")] UNSPEC_QXE))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V4SI (match_dup 0) 0)
        (unspec:V4SI [(subreg:V8HI (match_dup 1) 0)] UNSPEC_QXE))
   (set (subreg:V4SI (match_dup 0) 16)
        (unspec:V4SI [(subreg:V8HI (match_dup 1) 16)] UNSPEC_QXE))]
  ""
)

(define_insn "lvx_qxewd"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unspec:DI [(match_operand:V2SI 1 "register_operand" "r")] UNSPEC_QXE))]
  ""
  "slld %0 = %1, 32"
  [(set_attr "type" "alu_tiny")]
)

(define_insn "lvx_qxewdp"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V4SI 1 "register_operand" "r")] UNSPEC_QXE))]
  ""
  "slld %x0 = %x1, 32\n\tslld %y0 = %y1, 32"
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)

(define_insn "lvx_qxewdq"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (unspec:V4DI [(match_operand:V8SI 1 "register_operand" "r")] UNSPEC_QXE))]
  ""
  {
    return "slld %x0 = %x1, 32\n\tslld %y0 = %y1, 32\n\t"
           "slld %z0 = %z1, 32\n\tslld %t0 = %t1, 32";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)


;; WIDENO*, SXO*, ZXO*, QXO*

(define_expand "lvx_wideno<hwidenx>"
  [(match_operand:<HWIDE> 0 "register_operand" "")
   (match_operand:WIDENI 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[2], 0);
    if (!*xstr)
      {
        rtx op1 = simplify_gen_subreg (<HWIDE>mode, operands[1], <MODE>mode, 0);
        scalar_mode inner_mode = GET_MODE_INNER (<MODE>mode);
        unsigned inner_mode_size = GET_MODE_SIZE (inner_mode);
        rtx bits = GEN_INT (inner_mode_size * BITS_PER_UNIT);
        emit_insn (gen_ashr<hwide>3 (operands[0], op1, bits));
      }
    else if (xstr[1] == 'z')
      emit_insn (gen_lvx_zxo<hwidenx> (operands[0], operands[1]));
    else if (xstr[1] == 'q')
      emit_insn (gen_lvx_qxo<hwidenx> (operands[0], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_insn "lvx_zxobhq"
  [(set (match_operand:V4HI 0 "register_operand" "=r")
        (unspec:V4HI [(match_operand:V8QI 1 "register_operand" "r")] UNSPEC_ZXO))]
  ""
  "srlhq %0 = %1, 8"
  [(set_attr "type" "alu_thin")]
)

(define_insn "lvx_zxobho"
  [(set (match_operand:V8HI 0 "register_operand" "=r")
        (unspec:V8HI [(match_operand:V16QI 1 "register_operand" "r")] UNSPEC_ZXO))]
  ""
  "srlhq %x0 = %x1, 8\n\tsrlhq %y0 = %y1, 8"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn_and_split "lvx_zxobhx"
  [(set (match_operand:V16HI 0 "register_operand" "=r")
        (unspec:V16HI [(match_operand:V32QI 1 "register_operand" "r")] UNSPEC_ZXO))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V8HI (match_dup 0) 0)
        (unspec:V8HI [(subreg:V16QI (match_dup 1) 0)] UNSPEC_ZXO))
   (set (subreg:V8HI (match_dup 0) 16)
        (unspec:V8HI [(subreg:V16QI (match_dup 1) 16)] UNSPEC_ZXO))]
  ""
)

(define_insn_and_split "lvx_zxobhv"
  [(set (match_operand:V32HI 0 "register_operand" "=r")
        (unspec:V32HI [(match_operand:V64QI 1 "register_operand" "r")] UNSPEC_ZXO))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V8HI (match_dup 0) 0)
        (unspec:V8HI [(subreg:V16QI (match_dup 1) 0)] UNSPEC_ZXO))
   (set (subreg:V8HI (match_dup 0) 16)
        (unspec:V8HI [(subreg:V16QI (match_dup 1) 16)] UNSPEC_ZXO))
   (set (subreg:V8HI (match_dup 0) 32)
        (unspec:V8HI [(subreg:V16QI (match_dup 1) 32)] UNSPEC_ZXO))
   (set (subreg:V8HI (match_dup 0) 48)
        (unspec:V8HI [(subreg:V16QI (match_dup 1) 48)] UNSPEC_ZXO))]
  ""
)

(define_insn "lvx_zxohwp"
  [(set (match_operand:V2SI 0 "register_operand" "=r")
        (unspec:V2SI [(match_operand:V4HI 1 "register_operand" "r")] UNSPEC_ZXO))]
  ""
  "srlwp %0 = %1, 16"
  [(set_attr "type" "alu_thin")]
)

(define_insn "lvx_zxohwq"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V8HI 1 "register_operand" "r")] UNSPEC_ZXO))]
  ""
  "srlwp %x0 = %x1, 16\n\tsrlwp %y0 = %y1, 16"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn_and_split "lvx_zxohwo"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (unspec:V8SI [(match_operand:V16HI 1 "register_operand" "r")] UNSPEC_ZXO))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V4SI (match_dup 0) 0)
        (unspec:V4SI [(subreg:V8HI (match_dup 1) 0)] UNSPEC_ZXO))
   (set (subreg:V4SI (match_dup 0) 16)
        (unspec:V4SI [(subreg:V8HI (match_dup 1) 16)] UNSPEC_ZXO))]
  ""
)

(define_insn "lvx_zxowd"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unspec:DI [(match_operand:V2SI 1 "register_operand" "r")] UNSPEC_ZXO))]
  ""
  "srld %0 = %1, 32"
  [(set_attr "type" "alu_tiny")]
)

(define_insn "lvx_zxowdp"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V4SI 1 "register_operand" "r")] UNSPEC_ZXO))]
  ""
  "srld %x0 = %x1, 32\n\tsrld %y0 = %y1, 32"
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)

(define_insn "lvx_zxowdq"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (unspec:V4DI [(match_operand:V8SI 1 "register_operand" "r")] UNSPEC_ZXO))]
  ""
  {
    return "srld %x0 = %x1, 32\n\tsrld %y0 = %y1, 32\n\t"
           "srld %z0 = %z1, 32\n\tsrld %t0 = %t1, 32";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn "lvx_qxobhq"
  [(set (match_operand:V4HI 0 "register_operand" "=r")
        (unspec:V4HI [(match_operand:V8QI 1 "register_operand" "r")] UNSPEC_QXO))]
  ""
  "andd %0 = %1, 0xFF00FF00.@"
  [(set_attr "type" "alu_tiny_x")
   (set_attr "length"        "8")]
)

(define_insn "lvx_qxobho"
  [(set (match_operand:V8HI 0 "register_operand" "=r")
        (unspec:V8HI [(match_operand:V16QI 1 "register_operand" "r")] UNSPEC_QXO))]
  ""
  "andd %x0 = %x1, 0xFF00FF00.@\n\tandd %y0 = %y1, 0xFF00FF00.@"
  [(set_attr "type" "alu_tiny_x2_x")
   (set_attr "length"          "16")]
)

(define_insn "lvx_qxobhx"
  [(set (match_operand:V16HI 0 "register_operand" "=r")
        (unspec:V16HI [(match_operand:V32QI 1 "register_operand" "r")] UNSPEC_QXO))]
  ""
  {
    return "andd %x0 = %x1, 0xFF00FF00.@\n\tandd %y0 = %y1, 0xFF00FF00.@\n\t"
           "andd %z0 = %z1, 0xFF00FF00.@\n\tandd %t0 = %t1, 0xFF00FF00.@";
  }
  [(set_attr "type" "alu_tiny_x4_x")
   (set_attr "length"          "32")]
)

(define_insn_and_split "lvx_qxobhv"
  [(set (match_operand:V32HI 0 "register_operand" "=r")
        (unspec:V32HI [(match_operand:V64QI 1 "register_operand" "r")] UNSPEC_QXO))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V16HI (match_dup 0) 0)
        (unspec:V16HI [(subreg:V32QI (match_dup 1) 0)] UNSPEC_QXO))
   (set (subreg:V16HI (match_dup 0) 32)
        (unspec:V16HI [(subreg:V32QI (match_dup 1) 32)] UNSPEC_QXO))]
  ""
)

(define_insn "lvx_qxohwp"
  [(set (match_operand:V2SI 0 "register_operand" "=r")
        (unspec:V2SI [(match_operand:V4HI 1 "register_operand" "r")] UNSPEC_QXO))]
  ""
  "andd %0 = %1, 0xFFFF0000.@"
  [(set_attr "type" "alu_tiny_x")
   (set_attr "length"        "8")]
)

(define_insn "lvx_qxohwq"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V8HI 1 "register_operand" "r")] UNSPEC_QXO))]
  ""
  "andd %x0 = %x1, 0xFFFF0000.@\n\tandd %y0 = %y1, 0xFFFF0000.@"
  [(set_attr "type" "alu_tiny_x2_x")
   (set_attr "length"          "16")]
)

(define_insn "lvx_qxohwo"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (unspec:V8SI [(match_operand:V16HI 1 "register_operand" "r")] UNSPEC_QXO))]
  ""
  {
    return "andd %x0 = %x1, 0xFFFF0000.@\n\tandd %y0 = %y1, 0xFFFF0000.@\n\t"
           "andd %z0 = %z1, 0xFFFF0000.@\n\tandd %t0 = %t1, 0xFFFF0000.@";
  }
  [(set_attr "type" "alu_tiny_x4_x")
   (set_attr "length"          "32")]
)

(define_insn "lvx_qxowd"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unspec:DI [(match_operand:V2SI 1 "register_operand" "r")] UNSPEC_QXO))]
  ""
  "andd %0 = %1, 0xFFFFFFFF00000000"
  [(set_attr "type" "alu_tiny_y")
   (set_attr "length"       "12")]
)

(define_expand "lvx_qxowdp"
  [(set (match_operand:V2DI 0 "register_operand" "")
        (unspec:V2DI [(match_operand:V4SI 1 "register_operand" "")] UNSPEC_QXO))]
  ""
  {
    rtx mask = gen_reg_rtx (DImode);
    emit_insn (gen_rtx_SET (mask, GEN_INT (0xFFFFFFFF00000000)));
    emit_insn (gen_rtx_SET (operands[0],
                            gen_rtx_UNSPEC (V2DImode, gen_rtvec (2, operands[1], mask), UNSPEC_ANDD)));
    DONE;
  }
)

(define_expand "lvx_qxowdq"
  [(set (match_operand:V4DI 0 "register_operand" "")
        (unspec:V4DI [(match_operand:V8SI 1 "register_operand" "")] UNSPEC_QXO))]
  ""
  {
    rtx mask = gen_reg_rtx (DImode);
    emit_insn (gen_rtx_SET (mask, GEN_INT (0xFFFFFFFF00000000)));
    emit_insn (gen_rtx_SET (operands[0],
                            gen_rtx_UNSPEC (V4DImode, gen_rtvec (2, operands[1], mask), UNSPEC_ANDD)));
    DONE;
  }
)


;; SHIFT*, CAT*, LOW*, HIGH*

(define_expand "lvx_shift<lsvs>"
  [(match_operand:SIMDALL 0 "register_operand" "")
   (match_operand:SIMDALL 1 "register_operand" "")
   (match_operand 2 "const_int_operand" "")
   (match_operand:<INNER> 3 "nonmemory_operand" "")]
  ""
  {
    HOST_WIDE_INT value = INTVAL (operands[2]);
    HOST_WIDE_INT bytes = (value >= 0 ? value : -value) * GET_MODE_SIZE (<INNER>mode);
    bytes %= GET_MODE_SIZE (<MODE>mode);
    if (!bytes)
      emit_insn (gen_rtx_SET (operands[0], operands[1]));
    else
      {
        rtx chunk = const0_rtx;
        unsigned bits = bytes * BITS_PER_UNIT;
        if (operands[3] != CONST0_RTX (<INNER>mode))
          {
            if (bytes == GET_MODE_SIZE (<INNER>mode))
              chunk = force_reg (<INNER>mode, operands[3]);
            else
              {
                chunk = gen_reg_rtx (<CHUNK>mode);
                lvx_expand_chunk_splat (chunk, operands[3], <INNER>mode);
              }
          }
        lvx_expand_vector_shift (operands[0], operands[1], chunk, bits, value < 0);
      }
    DONE;
  }
)

(define_insn "lvx_catwp"
  [(set (match_operand:V2SI 0 "register_operand" "=r")
        (vec_concat:V2SI (match_operand:SI 1 "register_operand" "0")
                         (match_operand:SI 2 "register_operand" "r")))]
  ""
  "insf %0 = %2, 63, 32"
  [(set_attr "type" "alu_thin")]
)

(define_insn "lvx_catfwp"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (vec_concat:V2SF (match_operand:SF 1 "register_operand" "0")
                         (match_operand:SF 2 "register_operand" "r")))]
  ""
  "insf %0 = %2, 63, 32"
  [(set_attr "type" "alu_thin")]
)

(define_insn_and_split "lvx_cat<lsvs>"
  [(set (match_operand:S128F 0 "register_operand" "=r")
        (vec_concat:S128F (match_operand:<HALF> 1 "register_operand" "0")
                          (match_operand:<HALF> 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 8)
        (match_dup 2))]
  ""
)

(define_insn_and_split "lvx_catfdp"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (vec_concat:V2DF (match_operand:DF 1 "register_operand" "0")
                         (match_operand:DF 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 8)
        (match_dup 2))]
  ""
)

(define_insn_and_split "lvx_cat128"
  [(set (match_operand:V128 0 "register_operand" "=r")
        (vec_concat:V128 (match_operand:V64 1 "register_operand" "0")
                         (match_operand:V64 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V64 (match_dup 0) 8)
        (match_dup 2))]
  ""
)

(define_insn_and_split "lvx_cat256"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (vec_concat:V256 (match_operand:V128 1 "register_operand" "0")
                         (match_operand:V128 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V128 (match_dup 0) 16)
        (match_dup 2))]
  ""
)

(define_insn_and_split "lvx_cat512"
  [(set (match_operand:V512 0 "register_operand" "=r")
        (vec_concat:V512 (match_operand:V256 1 "register_operand" "0")
                         (match_operand:V256 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V256 (match_dup 0) 32)
        (match_dup 2))]
  ""
)

(define_insn_and_split "lvx_low64"
  [(set (match_operand:V64 0 "register_operand" "=r")
        (subreg:V64 (match_operand:V128 1 "register_operand" "r") 0))]
  ""
  "#"
  ""
  [(set (match_dup 0) (subreg:V64 (match_dup 1) 0))]
  ""
)

(define_insn_and_split "lvx_low128"
  [(set (match_operand:V128 0 "register_operand" "=r")
        (subreg:V128 (match_operand:V256 1 "register_operand" "r") 0))]
  ""
  "#"
  ""
  [(set (match_dup 0) (subreg:V128 (match_dup 1) 0))]
  ""
)

(define_insn_and_split "lvx_low256"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (subreg:V256 (match_operand:V512 1 "register_operand" "r") 0))]
  ""
  "#"
  ""
  [(set (match_dup 0) (subreg:V256 (match_dup 1) 0))]
  ""
)

(define_insn_and_split "lvx_high64"
  [(set (match_operand:V64 0 "register_operand" "=r")
        (subreg:V64 (match_operand:V128 1 "register_operand" "r") 8))]
  ""
  "#"
  ""
  [(set (match_dup 0) (subreg:V64 (match_dup 1) 8))]
  ""
)

(define_insn_and_split "lvx_high128"
  [(set (match_operand:V128 0 "register_operand" "=r")
        (subreg:V128 (match_operand:V256 1 "register_operand" "r") 16))]
  ""
  "#"
  ""
  [(set (match_dup 0) (subreg:V128 (match_dup 1) 16))]
  ""
)

(define_insn_and_split "lvx_high256"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (subreg:V256 (match_operand:V512 1 "register_operand" "r") 32))]
  ""
  "#"
  ""
  [(set (match_dup 0) (subreg:V256 (match_dup 1) 32))]
  ""
)


;; SPLAT*

(define_insn "lvx_splatd128"
  [(set (match_operand:V128 0 "register_operand" "=r")
        (unspec:V128 [(match_operand:V64 1 "register_operand" "r")] UNSPEC_SPLAT))]
  ""
  "copyd %x0 = %1\n\tcopyd %y0 = %1"
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)

(define_insn "lvx_splatd256"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (unspec:V256 [(match_operand:V64 1 "register_operand" "r")] UNSPEC_SPLAT))]
  ""
  "copyd %x0 = %1\n\tcopyd %y0 = %1\n\tcopyd %z0 = %1\n\tcopyd %t0 = %1"
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn "lvx_splatq256"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (unspec:V256 [(match_operand:V128 1 "register_operand" "r")] UNSPEC_SPLAT))]
  ""
  "copyd %x0 = %x1\n\tcopyd %y0 = %y1\n\tcopyd %z0 = %x1\n\tcopyd %t0 = %y1"
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)


;; ANY*

(define_expand "lvx_any<suffix>"
  [(match_operand:DI 0 "register_operand" "")
   (match_operand:LVXI 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[2], 0);
    gcc_assert (xstr[0] == '.');
    if (xstr[1] == 'n')
      emit_insn (gen_lvx_any<suffix>_nez (operands[0], operands[1]));
    else if (xstr[1] == 'e')
      emit_insn (gen_lvx_any<suffix>_eqz (operands[0], operands[1]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_insn "lvx_any<suffix>_nez"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (ne:DI (subreg:DI (match_operand:S64L 1 "register_operand" "r") 0)
               (const_int 0)))]
  ""
  "compd.ne %0 = %1, 0"
  [(set_attr "type" "alu_tiny")
   (set_attr "length"      "4")]
)

(define_expand "lvx_any<suffix>_eqz"
  [(match_operand:DI 0 "register_operand" "")
   (match_operand:S64L 1 "register_operand" "")]
  ""
  {
    lvx_expand_any64_eqz (operands[0], operands[1], <MODE>mode);
    emit_insn (gen_lvx_anyd_nez (operands[0], operands[0]));
    DONE;
  }
)

(define_insn "lvx_anyd_nez"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (ne:DI (match_operand:DI 1 "register_operand" "r")
               (const_int 0)))]
  ""
  "compd.ne %0 = %1, 0"
  [(set_attr "type" "alu_tiny")
   (set_attr "length"      "4")]
)

(define_expand "lvx_anyd_eqz"
  [(set (match_operand:DI 0 "register_operand" "")
        (eq:DI (match_operand:DI 1 "register_operand" "")
               (const_int 0)))]
  ""
  ""
)

(define_expand "lvx_any<suffix>_nez"
  [(match_operand:DI 0 "register_operand" "")
   (match_operand:V128L 1 "register_operand" "")]
  ""
  {
    rtx op1_0 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, 0);
    rtx op1_1 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, 8);
    rtx any_0 = gen_reg_rtx (DImode);
    rtx any_1 = gen_reg_rtx (DImode);
    emit_insn (gen_lvx_any<chunkx>_nez (any_0, op1_0));
    emit_insn (gen_lvx_any<chunkx>_nez (any_1, op1_1));
    emit_insn (gen_iordi3 (operands[0], any_0, any_1));
    DONE;
  }
)

(define_expand "lvx_any<suffix>_eqz"
  [(match_operand:DI 0 "register_operand" "")
   (match_operand:V128L 1 "register_operand" "")]
  ""
  {
    rtx op1_0 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, 0);
    rtx op1_1 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, 8);
    rtx any_0 = gen_reg_rtx (DImode);
    rtx any_1 = gen_reg_rtx (DImode);
    lvx_expand_any64_eqz (any_0, op1_0, <CHUNK>mode);
    lvx_expand_any64_eqz (any_1, op1_1, <CHUNK>mode);
    emit_insn (gen_lvx_liord (operands[0], any_0, any_1));
    DONE;
  }
)

(define_expand "lvx_any<suffix>_nez"
  [(match_operand:DI 0 "register_operand" "")
   (match_operand:V256L 1 "register_operand" "")]
  ""
  {
    rtx op1_0 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, 0);
    rtx op1_1 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, 8);
    rtx op1_2 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, 16);
    rtx op1_3 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, 24);
    rtx any_0 = gen_reg_rtx (DImode);
    rtx any_1 = gen_reg_rtx (DImode);
    rtx any_2 = gen_reg_rtx (DImode);
    rtx any_3 = gen_reg_rtx (DImode);
    rtx temp1 = gen_reg_rtx (DImode);
    rtx temp2 = gen_reg_rtx (DImode);
    emit_insn (gen_lvx_any<chunkx>_nez (any_0, op1_0));
    emit_insn (gen_lvx_any<chunkx>_nez (any_1, op1_1));
    emit_insn (gen_lvx_any<chunkx>_nez (any_2, op1_2));
    emit_insn (gen_lvx_any<chunkx>_nez (any_3, op1_3));
    emit_insn (gen_iordi3 (temp1, any_0, any_1));
    emit_insn (gen_iordi3 (temp2, any_2, any_3));
    emit_insn (gen_iordi3 (operands[0], temp1, temp2));
    DONE;
  }
)

(define_expand "lvx_any<suffix>_eqz"
  [(match_operand:DI 0 "register_operand" "")
   (match_operand:V256L 1 "register_operand" "")]
  ""
  {
    rtx op1_0 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, 0);
    rtx op1_1 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, 8);
    rtx op1_2 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, 16);
    rtx op1_3 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, 24);
    rtx any_0 = gen_reg_rtx (DImode);
    rtx any_1 = gen_reg_rtx (DImode);
    rtx any_2 = gen_reg_rtx (DImode);
    rtx any_3 = gen_reg_rtx (DImode);
    rtx temp1 = gen_reg_rtx (DImode);
    rtx temp2 = gen_reg_rtx (DImode);
    lvx_expand_any64_eqz (any_0, op1_0, <CHUNK>mode);
    lvx_expand_any64_eqz (any_1, op1_1, <CHUNK>mode);
    lvx_expand_any64_eqz (any_2, op1_2, <CHUNK>mode);
    lvx_expand_any64_eqz (any_3, op1_3, <CHUNK>mode);
    emit_insn (gen_lvx_liord (temp1, any_0, any_1));
    emit_insn (gen_lvx_liord (temp2, any_2, any_3));
    emit_insn (gen_iordi3 (operands[0], temp1, temp2));
    DONE;
  }
)


;; SELECT*

(define_insn "lvx_selectw"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (unspec:SI [(match_operand:SI 1 "register_operand" "r")
                    (match_operand:SI 2 "register_operand" "0")
                    (match_operand:DI 3 "register_operand" "r")
                    (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  "cmoved%4 %3? %0 = %1"
  [(set_attr "type" "alu_thin")
   (set_attr "length"      "4")]
)

(define_insn "lvx_selectd"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unspec:DI [(match_operand:DI 1 "register_operand" "r")
                    (match_operand:DI 2 "register_operand" "0")
                    (match_operand:DI 3 "register_operand" "r")
                    (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  "cmoved%4 %3? %0 = %1"
  [(set_attr "type" "alu_thin")
   (set_attr "length"      "4")]
)

(define_insn "lvx_select64s"
  [(set (match_operand:V64 0 "register_operand" "=r")
        (unspec:V64 [(match_operand:V64 1 "register_operand" "r")
                     (match_operand:V64 2 "register_operand" "0")
                     (match_operand:DI 3 "register_operand" "r")
                     (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  "cmoved%4 %3? %0 = %1"
  [(set_attr "type" "alu_thin")
   (set_attr "length"      "4")]
)

(define_insn "lvx_select128s"
  [(set (match_operand:V128 0 "register_operand" "=r")
        (unspec:V128 [(match_operand:V128 1 "register_operand" "r")
                      (match_operand:V128 2 "register_operand" "0")
                      (match_operand:DI 3 "register_operand" "r")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  "cmoved%4 %3? %x0 = %x1\n\tcmoved%4 %3? %y0 = %y1"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn "*lvx_select128s1"
  [(set (match_operand:V128 0 "register_operand" "=r")
        (unspec:V128 [(vec_duplicate:V128 (match_operand:ALL64 1 "register_operand" "r"))
                      (match_operand:V128 2 "register_operand" "0")
                      (match_operand:DI 3 "register_operand" "r")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  "cmoved%4 %3? %x0 = %1\n\tcmoved%4 %3? %y0 = %1"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn "*lvx_select128c1"
  [(set (match_operand:V128 0 "register_operand" "=r")
        (unspec:V128 [(vec_concat:V128
                        (match_operand:ALL64 1 "register_operand" "r")
                        (match_dup 1))
                      (match_operand:V128 2 "register_operand" "0")
                      (match_operand:DI 3 "register_operand" "r")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  "cmoved%4 %3? %x0 = %1\n\tcmoved%4 %3? %y0 = %1"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_expand "lvx_select256s"
  [(set (match_operand:V256 0 "register_operand" "")
        (unspec:V256 [(match_operand:V256 1 "register_operand" "")
                      (match_operand:V256 2 "register_operand" "")
                      (match_operand:DI 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  ""
)

(define_insn_and_split "lvx_select256s_1"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (unspec:V256 [(match_operand:V256 1 "register_operand" "r")
                      (match_operand:V256 2 "register_operand" "0")
                      (match_operand:DI 3 "register_operand" "r")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  "!HAVE_LVX_SELECT_<MODE>"
  "#"
  "!HAVE_LVX_SELECT_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)
                        (match_dup 3)
                        (match_dup 4)] UNSPEC_SELECT))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)
                        (match_dup 3)
                        (match_dup 4)] UNSPEC_SELECT))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "lvx_select256s_2"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (unspec:V256 [(match_operand:V256 1 "register_operand" "r")
                      (match_operand:V256 2 "register_operand" "0")
                      (match_operand:DI 3 "register_operand" "r")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  "HAVE_LVX_SELECT_<MODE>"
  {
    return "cmoved%4 %3? %x0 = %x1\n\tcmoved%4 %3? %y0 = %y1\n\t"
           "cmoved%4 %3? %z0 = %z1\n\tcmoved%4 %3? %t0 = %t1";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn "*lvx_select256s_2s1"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (unspec:V256 [(vec_duplicate:V256 (match_operand:ALL64 1 "register_operand" "r"))
                      (match_operand:V256 2 "register_operand" "0")
                      (match_operand:DI 3 "register_operand" "r")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  "HAVE_LVX_SELECT_<V256:MODE>"
  {
    return "cmoved%4 %3? %x0 = %1\n\tcmoved%4 %3? %y0 = %1\n\t"
           "cmoved%4 %3? %z0 = %1\n\tcmoved%4 %3? %t0 = %1";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn "*lvx_select256s_2c1"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (unspec:V256 [(vec_concat:V256
                        (vec_concat:V128
                          (match_operand:ALL64 1 "register_operand" "r")
                          (match_dup 1))
                        (vec_concat:V128
                          (match_dup 1)
                          (match_dup 1)))
                      (match_operand:V256 2 "register_operand" "0")
                      (match_operand:DI 3 "register_operand" "r")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  "HAVE_LVX_SELECT_<V256:MODE>"
  {
    return "cmoved%4 %3? %x0 = %1\n\tcmoved%4 %3? %y0 = %1\n\t"
           "cmoved%4 %3? %z0 = %1\n\tcmoved%4 %3? %t0 = %1";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_expand "lvx_select512s"
  [(set (match_operand:V512 0 "register_operand" "")
        (unspec:V512 [(match_operand:V512 1 "register_operand" "")
                      (match_operand:V512 2 "register_operand" "")
                      (match_operand:DI 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  ""
)

(define_insn_and_split "lvx_select512s_1"
  [(set (match_operand:V512 0 "register_operand" "=&r")
        (unspec:V512 [(match_operand:V512 1 "register_operand" "r")
                      (match_operand:V512 2 "register_operand" "0")
                      (match_operand:DI 3 "register_operand" "r")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  "!HAVE_LVX_SELECT_<HALF>"
  "#"
  "!HAVE_LVX_SELECT_<HALF> && reload_completed"
  [(set (subreg:<QUART> (match_dup 0) 0)
        (unspec:<QUART> [(subreg:<QUART> (match_dup 1) 0)
                         (subreg:<QUART> (match_dup 2) 0)
                         (match_dup 3)
                         (match_dup 4)] UNSPEC_SELECT))
   (set (subreg:<QUART> (match_dup 0) 16)
        (unspec:<QUART> [(subreg:<QUART> (match_dup 1) 16)
                         (subreg:<QUART> (match_dup 2) 16)
                         (match_dup 3)
                         (match_dup 4)] UNSPEC_SELECT))
   (set (subreg:<QUART> (match_dup 0) 32)
        (unspec:<QUART> [(subreg:<QUART> (match_dup 1) 32)
                         (subreg:<QUART> (match_dup 2) 32)
                         (match_dup 3)
                         (match_dup 4)] UNSPEC_SELECT))
   (set (subreg:<QUART> (match_dup 0) 48)
        (unspec:<QUART> [(subreg:<QUART> (match_dup 1) 48)
                         (subreg:<QUART> (match_dup 2) 48)
                         (match_dup 3)
                         (match_dup 4)] UNSPEC_SELECT))]
  ""
)

(define_insn_and_split "lvx_select512s_2"
  [(set (match_operand:V512 0 "register_operand" "=&r")
        (unspec:V512 [(match_operand:V512 1 "register_operand" "r")
                      (match_operand:V512 2 "register_operand" "0")
                      (match_operand:DI 3 "register_operand" "r")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  "HAVE_LVX_SELECT_<HALF>"
  "#"
  "HAVE_LVX_SELECT_<HALF> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)
                        (match_dup 3)
                        (match_dup 4)] UNSPEC_SELECT))
   (set (subreg:<HALF> (match_dup 0) 32)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 32)
                        (subreg:<HALF> (match_dup 2) 32)
                        (match_dup 3)
                        (match_dup 4)] UNSPEC_SELECT))]
  ""
)


;; SELECT*

(define_expand "lvx_select<suffix>"
  [(match_operand:VYQI 0 "register_operand" "")
   (match_operand:VYQI 1 "register_operand" "")
   (match_operand:VYQI 2 "register_operand" "")
   (match_operand:<MASK> 3 "register_operand" "")
   (match_operand 4 "" "")]
  ""
  {
    if (!HAVE_LVX_SELECT_<MODE>)
      {
        const char *modifier = XSTR (operands[4], 0);
        bool oddeven = !strcmp(modifier, ".odd") || !strcmp(modifier, ".even");
        rtx op3o = gen_reg_rtx (<HWIDE>mode), op3e = gen_reg_rtx (<HWIDE>mode);
        if (oddeven)
          {
            emit_insn (gen_lvx_zxo<hwidenx> (op3o, operands[3]));
            emit_insn (gen_lvx_zxe<hwidenx> (op3e, operands[3]));
          }
        else
          {
            emit_insn (gen_lvx_qxo<hwidenx> (op3o, operands[3]));
            emit_insn (gen_lvx_qxe<hwidenx> (op3e, operands[3]));
          }
        rtx op2 = simplify_gen_subreg (<HWIDE>mode, operands[2], <MODE>mode, 0);
        rtx op1 = simplify_gen_subreg (<HWIDE>mode, operands[1], <MODE>mode, 0);
        rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
        rtvec veco = gen_rtvec (4, op1, op2, op3o, operands[4]);
        rtvec vece = gen_rtvec (4, op1, op2, op3e, operands[4]);
        rtx selecto = gen_rtx_UNSPEC (<HWIDE>mode, veco, UNSPEC_SELECT);
        rtx selecte = gen_rtx_UNSPEC (<HWIDE>mode, vece, UNSPEC_SELECT);
        emit_insn (gen_rtx_SET (op0o, selecto));
        emit_insn (gen_rtx_SET (op0e, selecte));
        rtx opto = gen_reg_rtx (<MODE>mode), opte = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (opto, gen_rtx_SUBREG (<MODE>mode, op0o, 0)));
        emit_insn (gen_lvx_qxo<hwidenx> (op0o, opto));
        emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
        emit_insn (gen_lvx_zxe<hwidenx> (op0e, opte));
        emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));
      }
    else
      {
        rtvec vec = gen_rtvec (4, operands[1], operands[2], operands[3], operands[4]);
        rtx select = gen_rtx_UNSPEC (<MODE>mode, vec, UNSPEC_SELECT);
        emit_insn (gen_rtx_SET (operands[0], select));
      }
    DONE;
  }
)
(define_insn "*lvx_selectbo_2"
  [(set (match_operand:V8QI 0 "register_operand" "=r")
        (unspec:V8QI [(match_operand:V8QI 1 "register_operand" "r")
                      (match_operand:V8QI 2 "register_operand" "0")
                      (match_operand:V8QI 3 "register_operand" "r")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  "HAVE_LVX_SELECT_V8QI"
  "cmovebo%4 %3? %0 = %1"
  [(set_attr "type" "alu_tiny")]
)
(define_insn "*lvx_selectbx_2"
  [(set (match_operand:V16QI 0 "register_operand" "=r")
        (unspec:V16QI [(match_operand:V16QI 1 "register_operand" "r")
                       (match_operand:V16QI 2 "register_operand" "0")
                       (match_operand:V16QI 3 "register_operand" "r")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  "HAVE_LVX_SELECT_V16QI"
  "cmovebo%4 %x3? %x0 = %x1\n\tcmovebo%4 %y3? %y0 = %y1"
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)
(define_insn "*lvx_selectbv_2"
  [(set (match_operand:V32QI 0 "register_operand" "=r")
        (unspec:V32QI [(match_operand:V32QI 1 "register_operand" "r")
                       (match_operand:V32QI 2 "register_operand" "0")
                       (match_operand:V32QI 3 "register_operand" "r")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  "HAVE_LVX_SELECT_V32QI"
  {
    return "cmovebo%4 %x3? %x0 = %x1\n\tcmovebo%4 %y3? %y0 = %y1\n\t"
           "cmovebo%4 %z3? %z0 = %z1\n\tcmovebo%4 %t3? %t0 = %t1";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)
(define_insn_and_split "*lvx_selectbt_2"
  [(set (match_operand:V64QI 0 "register_operand" "=&r")
        (unspec:V64QI [(match_operand:V64QI 1 "register_operand" "r")
                       (match_operand:V64QI 2 "register_operand" "0")
                       (match_operand:V64QI 3 "register_operand" "r")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  "HAVE_LVX_SELECT_V32QI"
  "#"
  "HAVE_LVX_SELECT_V32QI && reload_completed"
  [(set (subreg:V32QI (match_dup 0) 0)
        (unspec:V32QI [(subreg:V32QI (match_dup 1) 0)
                        (subreg:V32QI (match_dup 2) 0)
                        (subreg:V32QI (match_dup 3) 0)
                        (match_dup 4)] UNSPEC_SELECT))
   (set (subreg:V32QI (match_dup 0) 32)
        (unspec:V32QI [(subreg:V32QI (match_dup 1) 32)
                        (subreg:V32QI (match_dup 2) 32)
                        (subreg:V32QI (match_dup 3) 32)
                        (match_dup 4)] UNSPEC_SELECT))]
  ""
)

(define_insn "lvx_select<suffix>"
  [(set (match_operand:S64I 0 "register_operand" "=r")
        (unspec:S64I [(match_operand:S64I 1 "register_operand" "r")
                      (match_operand:S64I 2 "register_operand" "0")
                      (match_operand:<MASK> 3 "register_operand" "r")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  "cmove<suffix>%4 %3? %0 = %1"
  [(set_attr "type" "alu_thin")]
)

(define_insn "lvx_select<suffix>"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (unspec:V128J [(match_operand:V128J 1 "register_operand" "r")
                       (match_operand:V128J 2 "register_operand" "0")
                       (match_operand:<MASK> 3 "register_operand" "r")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  "cmove<chunkx>%4 %x3? %x0 = %x1\n\tcmove<chunkx>%4 %y3? %y0 = %y1"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_expand "lvx_select<suffix>"
  [(set (match_operand:V256J 0 "register_operand" "")
        (unspec:V256J [(match_operand:V256J 1 "register_operand" "")
                       (match_operand:V256J 2 "register_operand" "")
                       (match_operand:V256J 3 "register_operand" "")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  ""
)

(define_insn_and_split "lvx_select<suffix>_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (unspec:V256J [(match_operand:V256J 1 "register_operand" "r")
                       (match_operand:V256J 2 "register_operand" "0")
                       (match_operand:V256J 3 "register_operand" "r")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  "!HAVE_LVX_SELECT_<MODE>"
  "#"
  "!HAVE_LVX_SELECT_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)
                        (subreg:<HALF> (match_dup 3) 0)
                        (match_dup 4)] UNSPEC_SELECT))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)
                        (subreg:<HALF> (match_dup 3) 16)
                        (match_dup 4)] UNSPEC_SELECT))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "lvx_select<suffix>_2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (unspec:V256J [(match_operand:V256J 1 "register_operand" "r")
                       (match_operand:V256J 2 "register_operand" "0")
                       (match_operand:V256J 3 "register_operand" "r")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  "HAVE_LVX_SELECT_<MODE>"
  {
    return "cmove<chunkx>%4 %x3? %x0 = %x1\n\tcmove<chunkx>%4 %y3? %y0 = %y1\n\t"
           "cmove<chunkx>%4 %z3? %z0 = %z1\n\tcmove<chunkx>%4 %t3? %t0 = %t1";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_expand "lvx_select<suffix>"
  [(set (match_operand:V512J 0 "register_operand" "")
        (unspec:V512J [(match_operand:V512J 1 "register_operand" "")
                       (match_operand:V512J 2 "register_operand" "")
                       (match_operand:V512J 3 "register_operand" "")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  ""
)

(define_insn_and_split "lvx_select<suffix>_1"
  [(set (match_operand:V512J 0 "register_operand" "=&r")
        (unspec:V512J [(match_operand:V512J 1 "register_operand" "r")
                       (match_operand:V512J 2 "register_operand" "0")
                       (match_operand:V512J 3 "register_operand" "r")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  "!HAVE_LVX_SELECT_<HALF>"
  "#"
  "!HAVE_LVX_SELECT_<HALF> && reload_completed"
  [(set (subreg:<QUART> (match_dup 0) 0)
        (unspec:<QUART> [(subreg:<QUART> (match_dup 1) 0)
                         (subreg:<QUART> (match_dup 2) 0)
                         (subreg:<QUART> (match_dup 3) 0)
                         (match_dup 4)] UNSPEC_SELECT))
   (set (subreg:<QUART> (match_dup 0) 16)
        (unspec:<QUART> [(subreg:<QUART> (match_dup 1) 16)
                         (subreg:<QUART> (match_dup 2) 16)
                         (subreg:<QUART> (match_dup 3) 16)
                         (match_dup 4)] UNSPEC_SELECT))
   (set (subreg:<QUART> (match_dup 0) 32)
        (unspec:<QUART> [(subreg:<QUART> (match_dup 1) 32)
                         (subreg:<QUART> (match_dup 2) 32)
                         (subreg:<QUART> (match_dup 3) 32)
                         (match_dup 4)] UNSPEC_SELECT))
   (set (subreg:<QUART> (match_dup 0) 48)
        (unspec:<QUART> [(subreg:<QUART> (match_dup 1) 48)
                         (subreg:<QUART> (match_dup 2) 48)
                         (subreg:<QUART> (match_dup 3) 48)
                         (match_dup 4)] UNSPEC_SELECT))]
  ""
)

(define_insn_and_split "lvx_select<suffix>_2"
  [(set (match_operand:V512J 0 "register_operand" "=&r")
        (unspec:V512J [(match_operand:V512J 1 "register_operand" "r")
                       (match_operand:V512J 2 "register_operand" "0")
                       (match_operand:V512J 3 "register_operand" "r")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  "HAVE_LVX_SELECT_<HALF>"
  "#"
  "HAVE_LVX_SELECT_<HALF> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)
                        (subreg:<HALF> (match_dup 3) 0)
                        (match_dup 4)] UNSPEC_SELECT))
   (set (subreg:<HALF> (match_dup 0) 32)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 32)
                        (subreg:<HALF> (match_dup 2) 32)
                        (subreg:<HALF> (match_dup 3) 32)
                        (match_dup 4)] UNSPEC_SELECT))]
  ""
)


;; SELECTF*

(define_insn "lvx_selectfw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand:SF 2 "register_operand" "0")
                    (match_operand:DI 3 "register_operand" "r")
                    (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  "cmoved%4 %3? %0 = %1"
  [(set_attr "type" "alu_thin")
   (set_attr "length"      "4")]
)

(define_insn "lvx_selectfd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:DF 1 "register_operand" "r")
                    (match_operand:DF 2 "register_operand" "0")
                    (match_operand:DI 3 "register_operand" "r")
                    (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  "cmoved%4 %3? %0 = %1"
  [(set_attr "type" "alu_thin")
   (set_attr "length"      "4")]
)

(define_insn "lvx_selectf<suffix>"
  [(set (match_operand:S64F 0 "register_operand" "=r")
        (unspec:S64F [(match_operand:S64F 1 "register_operand" "r")
                      (match_operand:S64F 2 "register_operand" "0")
                      (match_operand:<MASK> 3 "register_operand" "r")
                      (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  "cmove<suffix>%4 %3? %0 = %1"
  [(set_attr "type" "alu_thin")]
)

(define_insn "lvx_selectf<suffix>"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (unspec:V128F [(match_operand:V128F 1 "register_operand" "r")
                       (match_operand:V128F 2 "register_operand" "0")
                       (match_operand:<MASK> 3 "register_operand" "r")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  "cmove<chunkx>%4 %x3? %x0 = %x1\n\tcmove<chunkx>%4 %y3? %y0 = %y1"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_expand "lvx_selectf<suffix>"
  [(set (match_operand:V256F 0 "register_operand" "")
        (unspec:V256F [(match_operand:V256F 1 "register_operand" "")
                       (match_operand:V256F 2 "register_operand" "")
                       (match_operand:<MASK> 3 "register_operand" "")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  ""
  ""
)

(define_insn_and_split "lvx_selectf<suffix>_1"
  [(set (match_operand:V256F 0 "register_operand" "=r")
        (unspec:V256F [(match_operand:V256F 1 "register_operand" "r")
                       (match_operand:V256F 2 "register_operand" "0")
                       (match_operand:<MASK> 3 "register_operand" "r")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  "!HAVE_LVX_SELECT_<MODE>"
  "#"
  "!HAVE_LVX_SELECT_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)
                        (subreg:<HMASK> (match_dup 3) 0)
                        (match_dup 4)] UNSPEC_SELECT))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)
                        (subreg:<HMASK> (match_dup 3) 16)
                        (match_dup 4)] UNSPEC_SELECT))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "lvx_selectf<suffix>_2"
  [(set (match_operand:V256F 0 "register_operand" "=r")
        (unspec:V256F [(match_operand:V256F 1 "register_operand" "r")
                       (match_operand:V256F 2 "register_operand" "0")
                       (match_operand:<MASK> 3 "register_operand" "r")
                       (match_operand 4 "" "")] UNSPEC_SELECT))]
  "HAVE_LVX_SELECT_<MODE>"
  {
    return "cmove<chunkx>%4 %x3? %x0 = %x1\n\tcmove<chunkx>%4 %y3? %y0 = %y1\n\t"
           "cmove<chunkx>%4 %z3? %z0 = %z1\n\tcmove<chunkx>%4 %t3? %t0 = %t1";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)


;; STSU*

(define_insn "lvx_stsuhq"
  [(set (match_operand:V4HI 0 "register_operand" "=r")
        (unspec:V4HI [(match_operand:V4HI 1 "register_operand" "r")
                      (match_operand:V4HI 2 "register_operand" "r")] UNSPEC_STSU))]
  "HAVE_LVX_STSU_V4HI"
  "stsuhq %0 = %1, %2"
  [(set_attr "type" "alu_tiny")]
)

(define_insn "lvx_stsuho"
  [(set (match_operand:V8HI 0 "register_operand" "=r")
        (unspec:V8HI [(match_operand:V8HI 1 "register_operand" "r")
                      (match_operand:V8HI 2 "register_operand" "r")] UNSPEC_STSU))]
  "HAVE_LVX_STSU_V4HI"
  "stsuhq %x0 = %x1, %x2\n\tstsuhq %y0 = %y1, %y2"
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)

(define_insn "lvx_stsuhx"
  [(set (match_operand:V16HI 0 "register_operand" "=r")
        (unspec:V16HI [(match_operand:V16HI 1 "register_operand" "r")
                       (match_operand:V16HI 2 "register_operand" "r")] UNSPEC_STSU))]
  "HAVE_LVX_STSU_V4HI"
  {
    return "stsuhq %x0 = %x1, %x2\n\tstsuhq %y0 = %y1, %y2\n\t"
           "stsuhq %z0 = %z1, %z2\n\tstsuhq %t0 = %t1, %t2";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn_and_split "lvx_stsuhv"
  [(set (match_operand:V32HI 0 "register_operand" "=r")
        (unspec:V32HI [(match_operand:V32HI 1 "register_operand" "r")
                       (match_operand:V32HI 2 "register_operand" "r")] UNSPEC_STSU))]
  "HAVE_LVX_STSU_V4HI"
  "#"
  "HAVE_LVX_STSU_V4HI && reload_completed"
  [(set (subreg:V16HI (match_dup 0) 0)
        (unspec:V16HI [(subreg:V16HI (match_dup 1) 0)
                       (subreg:V16HI (match_dup 2) 0)] UNSPEC_STSU))
   (set (subreg:V16HI (match_dup 0) 32)
        (unspec:V16HI [(subreg:V16HI (match_dup 1) 32)
                       (subreg:V16HI (match_dup 2) 32)] UNSPEC_STSU))]
  ""
  [(set_attr "type" "alu_full")]
)

(define_insn "lvx_stsuwp"
  [(set (match_operand:V2SI 0 "register_operand" "=r")
        (unspec:V2SI [(match_operand:V2SI 1 "register_operand" "r")
                      (match_operand:V2SI 2 "register_operand" "r")] UNSPEC_STSU))]
  "HAVE_LVX_STSU_V2SI"
  "stsuwp %0 = %1, %2"
  [(set_attr "type" "alu_tiny")]
)

(define_insn "lvx_stsuwq"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V4SI 1 "register_operand" "r")
                      (match_operand:V4SI 2 "register_operand" "r")] UNSPEC_STSU))]
  "HAVE_LVX_STSU_V2SI"
  "stsuwp %x0 = %x1, %x2\n\tstsuwp %y0 = %y1, %y2"
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)

(define_insn "lvx_stsuwo"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (unspec:V8SI [(match_operand:V8SI 1 "register_operand" "r")
                      (match_operand:V8SI 2 "register_operand" "r")] UNSPEC_STSU))]
  "HAVE_LVX_STSU_V2SI"
  {
    return "stsuwp %x0 = %x1, %x2\n\tstsuwp %y0 = %y1, %y2\n\t"
           "stsuwp %z0 = %z1, %z2\n\tstsuwp %t0 = %t1, %t2";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn_and_split "lvx_stsuwx"
  [(set (match_operand:V16SI 0 "register_operand" "=r")
        (unspec:V16SI [(match_operand:V16SI 1 "register_operand" "r")
                       (match_operand:V16SI 2 "register_operand" "r")] UNSPEC_STSU))]
  "HAVE_LVX_STSU_V2SI"
  "#"
  "HAVE_LVX_STSU_V2SI && reload_completed"
  [(set (subreg:V8SI (match_dup 0) 0)
        (unspec:V8SI [(subreg:V8SI (match_dup 1) 0)
                      (subreg:V8SI (match_dup 2) 0)] UNSPEC_STSU))
   (set (subreg:V8SI (match_dup 0) 32)
        (unspec:V8SI [(subreg:V8SI (match_dup 1) 32)
                      (subreg:V8SI (match_dup 2) 32)] UNSPEC_STSU))]
  ""
  [(set_attr "type" "alu_full")]
)

(define_insn "lvx_stsud"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unspec:DI [(match_operand:DI 1 "register_operand" "r")
                    (match_operand:DI 2 "register_operand" "r")] UNSPEC_STSU))]
  "HAVE_LVX_STSU_DI"
  "stsud %0 = %1, %2"
  [(set_attr "type" "alu_thin")
   (set_attr "length" "4")]
)

(define_insn "lvx_stsudp"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V2DI 1 "register_operand" "r")
                      (match_operand:V2DI 2 "register_operand" "r")] UNSPEC_STSU))]
  "HAVE_LVX_STSU_V2DI"
  "stsud %x0 = %x1, %x2\n\tstsud %y0 = %y1, %y2"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_expand "lvx_stsudq"
  [(set (match_operand:V4DI 0 "register_operand" "")
        (unspec:V4DI [(match_operand:V4DI 1 "register_operand" "")
                      (match_operand:V4DI 2 "register_operand" "")] UNSPEC_STSU))]
  ""
  ""
)

(define_insn_and_split "lvx_stsudq_1"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (unspec:V4DI [(match_operand:V4DI 1 "register_operand" "r")
                      (match_operand:V4DI 2 "register_operand" "r")] UNSPEC_STSU))]
  "!HAVE_LVX_STSU_V4DI"
  "#"
  "!HAVE_LVX_STSU_V4DI && reload_completed"
  [(set (subreg:V2DI (match_dup 0) 0)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 0)
                      (subreg:V2DI (match_dup 2) 0)] UNSPEC_STSU))
   (set (subreg:V2DI (match_dup 0) 16)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 16)
                      (subreg:V2DI (match_dup 2) 16)] UNSPEC_STSU))]
  ""
  [(set_attr "type" "alu_thin_x2")]
)

(define_insn "lvx_stsudq_2"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (unspec:V4DI [(match_operand:V4DI 1 "register_operand" "r")
                      (match_operand:V4DI 2 "register_operand" "r")] UNSPEC_STSU))]
  "HAVE_LVX_STSU_V4DI"
  {
    return "stsud %x0 = %x1, %x2\n\tstsud %y0 = %y1, %y2\n\t"
           "stsud %z0 = %z1, %z2\n\tstsud %t0 = %t1, %t2";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn_and_split "lvx_stsudo"
  [(set (match_operand:V8DI 0 "register_operand" "=r")
        (unspec:V8DI [(match_operand:V8DI 1 "register_operand" "r")
                      (match_operand:V8DI 2 "register_operand" "r")] UNSPEC_STSU))]
  ""
  "#"
  ""
  [(set (subreg:V4DI (match_dup 0) 0)
        (unspec:V4DI [(subreg:V4DI (match_dup 1) 0)
                      (subreg:V4DI (match_dup 2) 0)] UNSPEC_STSU))
   (set (subreg:V4DI (match_dup 0) 32)
        (unspec:V4DI [(subreg:V4DI (match_dup 1) 32)
                      (subreg:V4DI (match_dup 2) 32)] UNSPEC_STSU))]
  ""
  [(set_attr "type" "alu_full")]
)


;; SBMM8D*, SBMMT8D*

(define_insn "lvx_sbmm8d"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (unspec:DI [(match_operand:DI 1 "register_operand" "r,r,r,r")
                    (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")] UNSPEC_SBMM8D))]
  "HAVE_LVX_SBMM8_DI"
  "sbmm8d %0 = %1, %2"
  [(set_attr "type" "alu_thin,alu_thin,alu_thin_x,alu_thin_y")
   (set_attr "length" "4,4,8,12")]
)

(define_insn "*sbmm8d"
  [(set (match_operand:ALL64 0 "register_operand" "=r")
        (unspec:ALL64 [(match_operand:FITGPR 1 "register_operand" "r")
                       (match_operand:DI 2 "register_operand" "r")] UNSPEC_SBMM8D))]
  "HAVE_LVX_SBMM8_<ALL64:MODE>"
  "sbmm8d %0 = %1, %2"
  [(set_attr "type" "alu_thin")]
)

(define_insn "*sbmm8d_s2"
  [(set (match_operand:ALL64 0 "register_operand" "=r")
        (unspec:ALL64 [(match_operand:FITGPR 1 "register_operand" "r")
                       (match_operand:DI 2 "register_operand" "r")] UNSPEC_SBMM8DS))]
  "HAVE_LVX_SBMM8_<ALL64:MODE>"
  "sbmm8d %0 = %1, %2.@"
  [(set_attr "type" "alu_thin_x")
   (set_attr "length"        "8")]
)

(define_insn "lvx_sbmm8dp"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V2DI 1 "register_operand" "r")
                      (match_operand:V2DI 2 "register_operand" "r")] UNSPEC_SBMM8D))]
  "HAVE_LVX_SBMM8_V2DI"
  "sbmm8d %x0 = %x1, %x2\n\tsbmm8d %y0 = %y1, %y2"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn "*sbmm8dp_s1"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (unspec:ALL128 [(vec_duplicate:V2DI (match_operand:DI 1 "register_operand" "r"))
                        (match_operand:SIMD128 2 "register_operand" "r")] UNSPEC_SBMM8D))]
  "HAVE_LVX_SBMM8_<ALL128:MODE>"
  "sbmm8d %x0 = %1, %x2\n\tsbmm8d %y0 = %1, %y2"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn "*sbmm8dp_s2"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (unspec:ALL128 [(match_operand:SIMD128 1 "register_operand" "r")
                        (vec_duplicate:V2DI (match_operand:DI 2 "register_operand" "r"))] UNSPEC_SBMM8D))]
  "HAVE_LVX_SBMM8_<ALL128:MODE>"
  "sbmm8d %x0 = %x1, %2\n\tsbmm8d %y0 = %y1, %2"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn "*sbmm8dp_xy"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (unspec:ALL128 [(match_operand:SIMD128 1 "register_operand" "r")
                        (match_operand:DI 2 "register_operand" "r")
                        (match_operand:DI 3 "register_operand" "r")] UNSPEC_SBMM8DXY))]
  "HAVE_LVX_SBMM8_<ALL128:MODE>"
  "sbmm8d %x0 = %x1, %2\n\tsbmm8d %y0 = %y1, %3"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_expand "lvx_sbmm8dq"
  [(set (match_operand:V4DI 0 "register_operand" "")
        (unspec:V4DI [(match_operand:V4DI 1 "register_operand" "")
                      (match_operand:V4DI 2 "register_operand" "")] UNSPEC_SBMM8D))]
  ""
  ""
)

(define_insn_and_split "lvx_sbmm8dq_1"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (unspec:V4DI [(match_operand:V4DI 1 "register_operand" "r")
                      (match_operand:V4DI 2 "register_operand" "r")] UNSPEC_SBMM8D))]
  "!HAVE_LVX_SBMM8_V4DI"
  "#"
  "!HAVE_LVX_SBMM8_V4DI && reload_completed"
  [(set (subreg:V2DI (match_dup 0) 0)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 0)
                      (subreg:V2DI (match_dup 2) 0)] UNSPEC_SBMM8D))
   (set (subreg:V2DI (match_dup 0) 16)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 16)
                      (subreg:V2DI (match_dup 2) 16)] UNSPEC_SBMM8D))]
  ""
)

(define_insn_and_split "*sbmm8dq_s1"
  [(set (match_operand:ALL256 0 "register_operand" "=&r")
        (unspec:ALL256 [(vec_duplicate:V4DI (match_operand:DI 1 "register_operand" "r"))
                        (match_operand:SIMD256 2 "register_operand" "r")] UNSPEC_SBMM8D))]
  "!HAVE_LVX_SBMM8_<ALL256:MODE>"
  "#"
  "!HAVE_LVX_SBMM8_<ALL256:MODE> && reload_completed"
  [(set (subreg:<ALL256:HALF> (match_dup 0) 0)
        (unspec:<ALL256:HALF> [(vec_duplicate:V2DI (match_dup 1))
                               (subreg:<SIMD256:HALF> (match_dup 2) 0)] UNSPEC_SBMM8D))
   (set (subreg:<ALL256:HALF> (match_dup 0) 16)
        (unspec:<ALL256:HALF> [(vec_duplicate:V2DI (match_dup 1))
                               (subreg:<SIMD256:HALF> (match_dup 2) 16)] UNSPEC_SBMM8D))]
  ""
)

(define_insn_and_split "*sbmm8dq_s2"
  [(set (match_operand:ALL256 0 "register_operand" "=&r")
        (unspec:ALL256 [(match_operand:SIMD256 1 "register_operand" "r")
                        (vec_duplicate:V4DI (match_operand:DI 2 "register_operand" "r"))] UNSPEC_SBMM8D))]
  "!HAVE_LVX_SBMM8_<ALL256:MODE>"
  "#"
  "!HAVE_LVX_SBMM8_<ALL256:MODE> && reload_completed"
  [(set (subreg:<ALL256:HALF> (match_dup 0) 0)
        (unspec:<ALL256:HALF> [(subreg:<SIMD256:HALF> (match_dup 1) 0)
                               (vec_duplicate:V2DI (match_dup 2))] UNSPEC_SBMM8D))
   (set (subreg:<ALL256:HALF> (match_dup 0) 16)
        (unspec:<ALL256:HALF> [(subreg:<SIMD256:HALF> (match_dup 1) 16)
                               (vec_duplicate:V2DI (match_dup 2))] UNSPEC_SBMM8D))]
  ""
)

(define_insn "lvx_sbmm8dq_2"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (unspec:V4DI [(match_operand:V4DI 1 "register_operand" "r")
                      (match_operand:V4DI 2 "register_operand" "r")] UNSPEC_SBMM8D))]
  "HAVE_LVX_SBMM8_V4DI"
  {
    return "sbmm8d %x0 = %x1, %x2\n\tsbmm8d %y0 = %y1, %y2\n\t"
           "sbmm8d %z0 = %z1, %z2\n\tsbmm8d %t0 = %t1, %t2";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn "*sbmm8dq_s1"
  [(set (match_operand:ALL256 0 "register_operand" "=r")
        (unspec:ALL256 [(vec_duplicate:V4DI (match_operand:DI 1 "register_operand" "r"))
                        (match_operand:SIMD256 2 "register_operand" "r")] UNSPEC_SBMM8D))]
  "HAVE_LVX_SBMM8_V4DI"
  {
    return "sbmm8d %x0 = %1, %x2\n\tsbmm8d %y0 = %1, %y2\n\t"
           "sbmm8d %z0 = %1, %z2\n\tsbmm8d %t0 = %1, %t2";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn "*sbmm8dq_s2"
  [(set (match_operand:ALL256 0 "register_operand" "=r")
        (unspec:ALL256 [(match_operand:SIMD256 1 "register_operand" "r")
                        (vec_duplicate:V4DI (match_operand:DI 2 "register_operand" "r"))] UNSPEC_SBMM8D))]
  "HAVE_LVX_SBMM8_V4DI"
  {
    return "sbmm8d %x0 = %x1, %2\n\tsbmm8d %y0 = %y1, %2\n\t"
           "sbmm8d %z0 = %z1, %2\n\tsbmm8d %t0 = %t1, %2";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_expand "lvx_sbmm8do"
  [(set (match_operand:V8DI 0 "register_operand" "")
        (unspec:V8DI [(match_operand:V8DI 1 "register_operand" "")
                      (match_operand:V8DI 2 "register_operand" "")] UNSPEC_SBMM8D))]
  ""
  ""
)

(define_insn_and_split "lvx_sbmm8do_1"
  [(set (match_operand:V8DI 0 "register_operand" "=r")
        (unspec:V8DI [(match_operand:V8DI 1 "register_operand" "r")
                      (match_operand:V8DI 2 "register_operand" "r")] UNSPEC_SBMM8D))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V2DI (match_dup 0) 0)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 0)
                      (subreg:V2DI (match_dup 2) 0)] UNSPEC_SBMM8D))
   (set (subreg:V2DI (match_dup 0) 16)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 16)
                      (subreg:V2DI (match_dup 2) 16)] UNSPEC_SBMM8D))
   (set (subreg:V2DI (match_dup 0) 32)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 32)
                      (subreg:V2DI (match_dup 2) 32)] UNSPEC_SBMM8D))
   (set (subreg:V2DI (match_dup 0) 48)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 48)
                      (subreg:V2DI (match_dup 2) 48)] UNSPEC_SBMM8D))]
  ""
)

(define_insn_and_split "lvx_sbmm8do_2"
  [(set (match_operand:V8DI 0 "register_operand" "=r")
        (unspec:V8DI [(match_operand:V8DI 1 "register_operand" "r")
                      (match_operand:V8DI 2 "register_operand" "r")] UNSPEC_SBMM8D))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V4DI (match_dup 0) 0)
        (unspec:V4DI [(subreg:V4DI (match_dup 1) 0)
                      (subreg:V4DI (match_dup 2) 0)] UNSPEC_SBMM8D))
   (set (subreg:V4DI (match_dup 0) 32)
        (unspec:V4DI [(subreg:V4DI (match_dup 1) 32)
                      (subreg:V4DI (match_dup 2) 32)] UNSPEC_SBMM8D))]
  ""
)


(define_insn "lvx_sbmmt8d"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r")
        (unspec:DI [(match_operand:DI 1 "register_operand" "r,r,r,r")
                    (match_operand:DI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")] UNSPEC_SBMMT8D))]
  "HAVE_LVX_TSBMM8_DI"
  "sbmmt8 %0 = %1, %2"
  [(set_attr "type" "alu_thin,alu_thin,alu_thin_x,alu_thin_y")
   (set_attr "length" "4,4,8,12")]
)

(define_insn "lvx_sbmmt8dp"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V2DI 1 "register_operand" "r")
                      (match_operand:V2DI 2 "register_operand" "r")] UNSPEC_SBMMT8D))]
  "HAVE_LVX_TSBMM8_V2DI"
  "sbmmt8 %x0 = %x1, %x2\n\tsbmmt8 %y0 = %y1, %y2"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn "*sbmmt8dp_s1"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (unspec:ALL128 [(vec_duplicate:V2DI (match_operand:DI 1 "register_operand" "r"))
                        (match_operand:SIMD128 2 "register_operand" "r")] UNSPEC_SBMMT8D))]
  "HAVE_LVX_TSBMM8_<ALL128:MODE>"
  "sbmmt8 %x0 = %1, %x2\n\tsbmmt8 %y0 = %1, %y2"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn "*sbmmt8dp_s2"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (unspec:ALL128 [(match_operand:SIMD128 1 "register_operand" "r")
                        (vec_duplicate:V2DI (match_operand:DI 2 "register_operand" "r"))] UNSPEC_SBMMT8D))]
  "HAVE_LVX_TSBMM8_<ALL128:MODE>"
  "sbmmt8 %x0 = %x1, %2\n\tsbmmt8 %y0 = %y1, %2"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_expand "lvx_sbmmt8dq"
  [(set (match_operand:V4DI 0 "register_operand" "")
        (unspec:V4DI [(match_operand:V4DI 1 "register_operand" "")
                      (match_operand:V4DI 2 "register_operand" "")] UNSPEC_SBMMT8D))]
  ""
  ""
)

(define_insn_and_split "lvx_sbmmt8dq_1"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (unspec:V4DI [(match_operand:V4DI 1 "register_operand" "r")
                      (match_operand:V4DI 2 "register_operand" "r")] UNSPEC_SBMMT8D))]
  "!HAVE_LVX_TSBMM8_V4DI"
  "#"
  "!HAVE_LVX_TSBMM8_V4DI && reload_completed"
  [(set (subreg:V2DI (match_dup 0) 0)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 0)
                      (subreg:V2DI (match_dup 2) 0)] UNSPEC_SBMMT8D))
   (set (subreg:V2DI (match_dup 0) 16)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 16)
                      (subreg:V2DI (match_dup 2) 16)] UNSPEC_SBMMT8D))]
  ""
)

(define_insn_and_split "*sbmmt8dq_s1"
  [(set (match_operand:ALL256 0 "register_operand" "=&r")
        (unspec:ALL256 [(vec_duplicate:V4DI (match_operand:DI 1 "register_operand" "r"))
                        (match_operand:SIMD256 2 "register_operand" "r")] UNSPEC_SBMMT8D))]
  "!HAVE_LVX_TSBMM8_V4DI"
  "#"
  "!HAVE_LVX_TSBMM8_V4DI && reload_completed"
  [(set (subreg:<ALL256:HALF> (match_dup 0) 0)
        (unspec:<ALL256:HALF> [(vec_duplicate:V2DI (match_dup 1))
                               (subreg:<SIMD256:HALF> (match_dup 2) 0)] UNSPEC_SBMMT8D))
   (set (subreg:<ALL256:HALF> (match_dup 0) 16)
        (unspec:<ALL256:HALF> [(vec_duplicate:V2DI (match_dup 1))
                               (subreg:<SIMD256:HALF> (match_dup 2) 16)] UNSPEC_SBMMT8D))]
  ""
)

(define_insn_and_split "*sbmmt8dq_s2"
  [(set (match_operand:ALL256 0 "register_operand" "=&r")
        (unspec:ALL256 [(match_operand:SIMD256 1 "register_operand" "r")
                        (vec_duplicate:V4DI (match_operand:DI 2 "register_operand" "r"))] UNSPEC_SBMMT8D))]
  "!HAVE_LVX_TSBMM8_V4DI"
  "#"
  "!HAVE_LVX_TSBMM8_V4DI && reload_completed"
  [(set (subreg:<ALL256:HALF> (match_dup 0) 0)
        (unspec:<ALL256:HALF> [(subreg:<SIMD256:HALF> (match_dup 1) 0)
                               (vec_duplicate:V2DI (match_dup 2))] UNSPEC_SBMMT8D))
   (set (subreg:<ALL256:HALF> (match_dup 0) 16)
        (unspec:<ALL256:HALF> [(subreg:<SIMD256:HALF> (match_dup 1) 16)
                               (vec_duplicate:V2DI (match_dup 2))] UNSPEC_SBMMT8D))]
  ""
)

(define_insn "lvx_sbmmt8dq_2"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (unspec:V4DI [(match_operand:V4DI 1 "register_operand" "r")
                      (match_operand:V4DI 2 "register_operand" "r")] UNSPEC_SBMMT8D))]
  "HAVE_LVX_TSBMM8_V4DI"
  {
    return "sbmmt8 %x0 = %x1, %x2\n\tsbmmt8 %y0 = %y1, %y2\n\t"
           "sbmmt8 %z0 = %z1, %z2\n\tsbmmt8 %t0 = %t1, %t2";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn "*sbmmt8dq_s1"
  [(set (match_operand:ALL256 0 "register_operand" "=r")
        (unspec:ALL256 [(vec_duplicate:V4DI (match_operand:DI 1 "register_operand" "r"))
                        (match_operand:SIMD256 2 "register_operand" "r")] UNSPEC_SBMMT8D))]
  "HAVE_LVX_TSBMM8_V4DI"
  {
    return "sbmmt8 %x0 = %1, %x2\n\tsbmmt8 %y0 = %1, %y2\n\t"
           "sbmmt8 %z0 = %1, %z2\n\tsbmmt8 %t0 = %1, %t2";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn "*sbmmt8dq_s2"
  [(set (match_operand:ALL256 0 "register_operand" "=r")
        (unspec:ALL256 [(match_operand:SIMD256 1 "register_operand" "r")
                        (vec_duplicate:V4DI (match_operand:DI 2 "register_operand" "r"))] UNSPEC_SBMMT8D))]
  "HAVE_LVX_TSBMM8_V4DI"
  {
    return "sbmmt8 %x0 = %x1, %2\n\tsbmmt8 %y0 = %y1, %2\n\t"
           "sbmmt8 %z0 = %z1, %2\n\tsbmmt8 %t0 = %t1, %2";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_expand "lvx_sbmmt8do"
  [(set (match_operand:V8DI 0 "register_operand" "")
        (unspec:V8DI [(match_operand:V8DI 1 "register_operand" "")
                      (match_operand:V8DI 2 "register_operand" "")] UNSPEC_SBMMT8D))]
  ""
  ""
)

(define_insn_and_split "lvx_sbmmt8do_1"
  [(set (match_operand:V8DI 0 "register_operand" "=r")
        (unspec:V8DI [(match_operand:V8DI 1 "register_operand" "r")
                      (match_operand:V8DI 2 "register_operand" "r")] UNSPEC_SBMMT8D))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V2DI (match_dup 0) 0)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 0)
                      (subreg:V2DI (match_dup 2) 0)] UNSPEC_SBMMT8D))
   (set (subreg:V2DI (match_dup 0) 16)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 16)
                      (subreg:V2DI (match_dup 2) 16)] UNSPEC_SBMMT8D))
   (set (subreg:V2DI (match_dup 0) 32)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 32)
                      (subreg:V2DI (match_dup 2) 32)] UNSPEC_SBMMT8D))
   (set (subreg:V2DI (match_dup 0) 48)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 48)
                      (subreg:V2DI (match_dup 2) 48)] UNSPEC_SBMMT8D))]
  ""
)

(define_insn_and_split "lvx_sbmmt8do_2"
  [(set (match_operand:V8DI 0 "register_operand" "=r")
        (unspec:V8DI [(match_operand:V8DI 1 "register_operand" "r")
                      (match_operand:V8DI 2 "register_operand" "r")] UNSPEC_SBMMT8D))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V4DI (match_dup 0) 0)
        (unspec:V4DI [(subreg:V4DI (match_dup 1) 0)
                      (subreg:V4DI (match_dup 2) 0)] UNSPEC_SBMMT8D))
   (set (subreg:V4DI (match_dup 0) 32)
        (unspec:V4DI [(subreg:V4DI (match_dup 1) 32)
                      (subreg:V4DI (match_dup 2) 32)] UNSPEC_SBMMT8D))]
  ""
)


;; FREC* — LVX has no FRECW instruction; implement reciprocal via FDIVW.

(define_expand "lvx_frecw"
  [(match_operand:SF 0 "register_operand" "")
   (match_operand:SF 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    rtx one = force_reg (SFmode, CONST1_RTX (SFmode));
    emit_insn (gen_lvx_fdivw (operands[0], one, operands[1], operands[2]));
    DONE;
  }
)

(define_expand "lvx_frechf"
  [(match_operand:HF 0 "register_operand" "")
   (match_operand:HF 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    rtx sf_tmp = gen_reg_rtx (SFmode);
    emit_insn (gen_extendhfsf2 (sf_tmp, operands[1]));
    emit_insn (gen_lvx_frecw (sf_tmp, sf_tmp, operands[2]));
    emit_insn (gen_truncsfhf2 (operands[0], sf_tmp));
    DONE;
  }
)

(define_expand "lvx_fhighw"
  [(match_operand:SF 0 "register_operand" "")
   (match_operand:V2SF 1 "register_operand" "")]
  ""
  {
    rtx op1 = simplify_gen_subreg (V2SFmode, operands[1], V2SFmode, 0);
    emit_insn (gen_rtx_SET (operands[0], gen_rtx_UNSPEC (SFmode, gen_rtvec (2, op1, GEN_INT (32)), UNSPEC_SRLD)));
    DONE;
  }
)

;; TODO: FIXME
;; UNSPEC to get the low part of a V2SF
;; It should be possible to use simplify_gen_subreg (SFmode, op, V2SFmode, 0)
;; but, due to a bug in the inliner triggered by simde_mm512_recip_ps (in svml.c
;; in simde), the types at the boundary of simde_mm512_loadu_ps (which does a
;; memcpy to convert types) are partially lost when it is inlined, resulting in
;; a (subreg:SF (subreg:V2SF (reg:V64QI ...))) which can't be processed.
;; This is currently only used by lvx_frecwp, but might be needed at other
;; places such as lvx_frsrwp. I let them untouched because I want to know when
;; it will break there as well.

(define_insn_and_split "lvx_floww"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:V2SF 1 "register_operand" "0")] UNSPEC_FLOW))]
  ""
  "#"
  "reload_completed"
  [(use (const_int 0))]
)

(define_expand "lvx_frecwp"
  [(match_operand:V2SF 0 "register_operand" "")
   (match_operand:V2SF 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    rtx op0x = gen_reg_rtx (SFmode);
    rtx op0y = gen_reg_rtx (SFmode);
    rtx op1x = gen_reg_rtx (SFmode);
    rtx op1y = gen_reg_rtx (SFmode);
    emit_insn (gen_lvx_floww (op1x, operands[1]));
    emit_insn (gen_lvx_fhighw (op1y, operands[1]));
    emit_insn (gen_lvx_frecw (op0x, op1x, operands[2]));
    emit_insn (gen_lvx_frecw (op0y, op1y, operands[2]));
    emit_insn (gen_lvx_catfwp (operands[0], op0x, op0y));
    DONE;
  }
)

(define_expand "lvx_frecwq"
  [(match_operand:V4SF 0 "register_operand" "")
   (match_operand:V4SF 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    for (int i = 0; i < 2; i++)
      {
        rtx opnd0 = simplify_gen_subreg (V2SFmode, operands[0], V4SFmode, i*8);
        rtx opnd1 = simplify_gen_subreg (V2SFmode, operands[1], V4SFmode, i*8);
        emit_insn (gen_lvx_frecwp (opnd0, opnd1, operands[2]));
      }
    DONE;
  }
)

(define_expand "lvx_frecwo"
  [(match_operand:V8SF 0 "register_operand" "")
   (match_operand:V8SF 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    for (int i = 0; i < 4; i++)
      {
        rtx opnd0 = simplify_gen_subreg (V2SFmode, operands[0], V8SFmode, i*8);
        rtx opnd1 = simplify_gen_subreg (V2SFmode, operands[1], V8SFmode, i*8);
        emit_insn (gen_lvx_frecwp (opnd0, opnd1, operands[2]));
      }
    DONE;
  }
)


;; FRSR*

(define_insn "*lvx_frsrw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(subreg:SF (match_operand:V2SF 1 "register_operand" "r") 0)
                    (match_operand 2 "" "")] UNSPEC_FRSR))]
  ""
  {
    if (GET_CODE (operands[2]) == CONST_STRING)
      return "frsrw%2 %0 = %1";
    return "frsrw %0 = %1";
  }
  [(set_attr "type" "alu_full_sfu")]
)

(define_insn "lvx_frsrw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FRSR))]
  ""
  {
    if (GET_CODE (operands[2]) == CONST_STRING)
      return "frsrw%2 %0 = %1";
    return "frsrw %0 = %1";
  }
  [(set_attr "type" "alu_full_sfu")]
)

(define_insn_and_split "lvx_frsrhf"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (unspec:HF [(match_operand:HF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FRSR))
   (match_scratch:SF 3 "=r")]
  "" "#" ""
  [(set (match_dup 3) (float_extend:SF (match_dup 1)))
   (set (match_dup 3) (unspec:SF [(match_dup 3) (match_dup 2)] UNSPEC_FRSR))
   (set (match_dup 0) (float_truncate:HF (match_dup 3)))]
  {
    operands[3] = gen_reg_rtx (SFmode);
  }
)

(define_expand "lvx_frsrwp"
  [(match_operand:V2SF 0 "register_operand" "")
   (match_operand:V2SF 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    rtx op0x = gen_reg_rtx (SFmode);
    rtx op0y = gen_reg_rtx (SFmode);
    rtx op1x = simplify_gen_subreg (SFmode, operands[1], V2SFmode, 0);
    rtx op1y = gen_reg_rtx (SFmode);
    emit_insn (gen_lvx_fhighw (op1y, operands[1]));
    emit_insn (gen_lvx_frsrw (op0x, op1x, operands[2]));
    emit_insn (gen_lvx_frsrw (op0y, op1y, operands[2]));
    emit_insn (gen_lvx_catfwp (operands[0], op0x, op0y));
    DONE;
  }
)

(define_expand "lvx_frsrwq"
  [(match_operand:V4SF 0 "register_operand" "")
   (match_operand:V4SF 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    for (int i = 0; i < 2; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (V2SFmode, operands[0], i*8);
        rtx opnd1 = gen_rtx_SUBREG (V2SFmode, operands[1], i*8);
        emit_insn (gen_lvx_frsrwp (opnd0, opnd1, operands[2]));
      }
    DONE;
  }
)

(define_expand "lvx_frsrwo"
  [(match_operand:V8SF 0 "register_operand" "")
   (match_operand:V8SF 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    for (int i = 0; i < 4; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (V2SFmode, operands[0], i*8);
        rtx opnd1 = gen_rtx_SUBREG (V2SFmode, operands[1], i*8);
        emit_insn (gen_lvx_frsrwp (opnd0, opnd1, operands[2]));
      }
    DONE;
  }
)

;; FADD*C

(define_insn "lvx_faddwc"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")
                      (match_operand:V2SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "HAVE_LVX_CPLX_PLUS_V2SF"
  "faddwc%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn "lvx_faddwcp"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "HAVE_LVX_CPLX_PLUS_V4SF"
  "faddwcp%3 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_fadddc"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand" "r")
                      (match_operand:V2DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "HAVE_LVX_CPLX_PLUS_V2DF"
  "fadddc%3 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_fadddcp"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "r")
                      (match_operand:V4DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "HAVE_LVX_CPLX_PLUS_V2DF"
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "")
                      (match_operand:V4DF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "HAVE_LVX_CPLX_PLUS_V2DF && reload_completed"
  [(set (subreg:V2DF (match_dup 0) 0)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 0)
                      (subreg:V2DF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V2DF (match_dup 0) 16)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 16)
                      (subreg:V2DF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FADD))]
  ""
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "")
                      (match_operand:V4DF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "!HAVE_LVX_CPLX_PLUS_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DF (match_dup 1) 0)
                    (subreg:DF (match_dup 2) 0)
                    (match_dup 3)] UNSPEC_FADD))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DF (match_dup 1) 8)
                    (subreg:DF (match_dup 2) 8)
                    (match_dup 3)] UNSPEC_FADD))
   (set (subreg:DF (match_dup 0) 16)
        (unspec:DF [(subreg:DF (match_dup 1) 16)
                    (subreg:DF (match_dup 2) 16)
                    (match_dup 3)] UNSPEC_FADD))
   (set (subreg:DF (match_dup 0) 24)
        (unspec:DF [(subreg:DF (match_dup 1) 24)
                    (subreg:DF (match_dup 2) 24)
                    (match_dup 3)] UNSPEC_FADD))]
  ""
)

(define_insn_and_split "lvx_fadddcq"
  [(set (match_operand:V8DF 0 "register_operand" "=r")
        (unspec:V8DF [(match_operand:V8DF 1 "register_operand" "r")
                      (match_operand:V8DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "HAVE_LVX_CPLX_PLUS_V2DF"
  "#"
  "HAVE_LVX_CPLX_PLUS_V2DF && reload_completed"
  [(set (subreg:V2DF (match_dup 0) 0)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 0)
                      (subreg:V2DF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V2DF (match_dup 0) 16)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 16)
                      (subreg:V2DF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V2DF (match_dup 0) 32)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 32)
                      (subreg:V2DF (match_dup 2) 32)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V2DF (match_dup 0) 48)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 48)
                      (subreg:V2DF (match_dup 2) 48)
                      (match_dup 3)] UNSPEC_FADD))]
  ""
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_faddwcq"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "HAVE_LVX_CPLX_PLUS_V4SF"
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "HAVE_LVX_CPLX_PLUS_V4SF && reload_completed"
  [(set (subreg:V4SF (match_dup 0) 0)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 0)
                      (subreg:V4SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V4SF (match_dup 0) 16)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 16)
                      (subreg:V4SF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FADD))]
  ""
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 0)
                      (subreg:V2SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 8)
                      (subreg:V2SF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V2SF (match_dup 0) 16)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 16)
                      (subreg:V2SF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V2SF (match_dup 0) 24)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 24)
                      (subreg:V2SF (match_dup 2) 24)
                      (match_dup 3)] UNSPEC_FADD))]
  ""
)

;; FADD*

(define_insn "lvx_faddh"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (unspec:HF [(match_operand:HF 1 "register_operand" "r")
                    (match_operand:HF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FADD))]
  ""
  "faddh%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp3")]
)

(define_insn "lvx_faddw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand:SF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FADD))]
  ""
  "faddw%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn "lvx_faddd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:DF 1 "register_operand" "r")
                    (match_operand:DF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FADD))]
  ""
  "faddd%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn "lvx_fadd<suffix>"
  [(set (match_operand:S64F 0 "register_operand" "=R,Q")
        (unspec:S64F [(match_operand:S64F 1 "register_operand" "R,Q")
                      (match_operand:S64F 2 "register_operand" "R,Q")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  ""
  "fadd<suffix>%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_faddho"
  [(set (match_operand:V8HF 0 "register_operand" "=r")
        (unspec:V8HF [(match_operand:V8HF 1 "register_operand" "r")
                      (match_operand:V8HF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  ""
  "faddho%3 %0 = %1, %2"
  "!HAVE_LVX_PLUS_V8HF && reload_completed"
  [(set (subreg:V4HF (match_dup 0) 0)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 0)
                      (subreg:V4HF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V4HF (match_dup 0) 8)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 8)
                      (subreg:V4HF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FADD))]
  ""
  [(set_attr "type" "madd_fp3")]
)

(define_insn_and_split "lvx_faddwq"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  ""
  "faddwq%3 %0 = %1, %2"
  "!HAVE_LVX_PLUS_V4SF && reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 0)
                      (subreg:V2SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 8)
                      (subreg:V2SF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FADD))]
  ""
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn_and_split "lvx_fadddp"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand" "r")
                      (match_operand:V2DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  ""
  "fadddp%3 %0 = %1, %2"
  "!HAVE_LVX_PLUS_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DF (match_dup 1) 0)
                    (subreg:DF (match_dup 2) 0)
                    (match_dup 3)] UNSPEC_FADD))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DF (match_dup 1) 8)
                    (subreg:DF (match_dup 2) 8)
                    (match_dup 3)] UNSPEC_FADD))]
  ""
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn "lvx_faddhx"
  [(set (match_operand:V16HF 0 "register_operand" "=r")
        (unspec:V16HF [(match_operand:V16HF 1 "register_operand" "r")
                      (match_operand:V16HF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V16HF 0 "register_operand" "")
        (unspec:V16HF [(match_operand:V16HF 1 "register_operand" "")
                      (match_operand:V16HF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "!HAVE_LVX_PLUS_V8HF && reload_completed"
  [(set (subreg:V4HF (match_dup 0) 0)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 0)
                      (subreg:V4HF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V4HF (match_dup 0) 8)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 8)
                      (subreg:V4HF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V4HF (match_dup 0) 16)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 16)
                      (subreg:V4HF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V4HF (match_dup 0) 24)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 24)
                      (subreg:V4HF (match_dup 2) 24)
                      (match_dup 3)] UNSPEC_FADD))]
  ""
)

(define_split
  [(set (match_operand:V16HF 0 "register_operand" "")
        (unspec:V16HF [(match_operand:V16HF 1 "register_operand" "")
                      (match_operand:V16HF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "HAVE_LVX_PLUS_V8HF && reload_completed"
  [(set (subreg:V8HF (match_dup 0) 0)
        (unspec:V8HF [(subreg:V8HF (match_dup 1) 0)
                      (subreg:V8HF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V8HF (match_dup 0) 16)
        (unspec:V8HF [(subreg:V8HF (match_dup 1) 16)
                      (subreg:V8HF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FADD))]
  ""
)

(define_insn "lvx_faddwo"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "HAVE_LVX_PLUS_V4SF && reload_completed"
  [(set (subreg:V4SF (match_dup 0) 0)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 0)
                      (subreg:V4SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V4SF (match_dup 0) 16)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 16)
                      (subreg:V4SF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FADD))]
  ""
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "!HAVE_LVX_PLUS_V4SF && reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 0)
                      (subreg:V2SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 8)
                      (subreg:V2SF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V2SF (match_dup 0) 16)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 16)
                      (subreg:V2SF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V2SF (match_dup 0) 24)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 24)
                      (subreg:V2SF (match_dup 2) 24)
                      (match_dup 3)] UNSPEC_FADD))]
  ""
)

(define_insn "lvx_fadddq"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "r")
                      (match_operand:V4DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "")
                      (match_operand:V4DF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "HAVE_LVX_PLUS_V2DF && reload_completed"
  [(set (subreg:V2DF (match_dup 0) 0)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 0)
                      (subreg:V2DF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FADD))
   (set (subreg:V2DF (match_dup 0) 16)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 16)
                      (subreg:V2DF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FADD))]
  ""
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "")
                      (match_operand:V4DF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "!HAVE_LVX_PLUS_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DF (match_dup 1) 0)
                    (subreg:DF (match_dup 2) 0)
                    (match_dup 3)] UNSPEC_FADD))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DF (match_dup 1) 8)
                    (subreg:DF (match_dup 2) 8)
                    (match_dup 3)] UNSPEC_FADD))
   (set (subreg:DF (match_dup 0) 16)
        (unspec:DF [(subreg:DF (match_dup 1) 16)
                    (subreg:DF (match_dup 2) 16)
                    (match_dup 3)] UNSPEC_FADD))
   (set (subreg:DF (match_dup 0) 24)
        (unspec:DF [(subreg:DF (match_dup 1) 24)
                    (subreg:DF (match_dup 2) 24)
                    (match_dup 3)] UNSPEC_FADD))]
  ""
)

;; FSBF*C

(define_insn "lvx_fsbfwc"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")
                      (match_operand:V2SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "fsbfwc%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn "lvx_fsbfwcp"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "fsbfwcp%3 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_fsbfdc"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand" "r")
                      (match_operand:V2DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "fsbfdc%3 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_fsbfdcp"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "r")
                      (match_operand:V4DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "")
                      (match_operand:V4DF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  "HAVE_LVX_MINUS_V2DF && reload_completed"
  [(set (subreg:V2DF (match_dup 0) 0)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 0)
                      (subreg:V2DF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V2DF (match_dup 0) 16)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 16)
                      (subreg:V2DF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FSBF))]
  ""
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "")
                      (match_operand:V4DF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  "!HAVE_LVX_MINUS_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DF (match_dup 1) 0)
                    (subreg:DF (match_dup 2) 0)
                    (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DF (match_dup 1) 8)
                    (subreg:DF (match_dup 2) 8)
                    (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:DF (match_dup 0) 16)
        (unspec:DF [(subreg:DF (match_dup 1) 16)
                    (subreg:DF (match_dup 2) 16)
                    (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:DF (match_dup 0) 24)
        (unspec:DF [(subreg:DF (match_dup 1) 24)
                    (subreg:DF (match_dup 2) 24)
                    (match_dup 3)] UNSPEC_FSBF))]
  ""
)

(define_insn_and_split "lvx_fsbfdcq"
  [(set (match_operand:V8DF 0 "register_operand" "=r")
        (unspec:V8DF [(match_operand:V8DF 1 "register_operand" "r")
                      (match_operand:V8DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V2DF (match_dup 0) 0)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 0)
                      (subreg:V2DF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V2DF (match_dup 0) 16)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 16)
                      (subreg:V2DF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V2DF (match_dup 0) 32)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 32)
                      (subreg:V2DF (match_dup 2) 32)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V2DF (match_dup 0) 48)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 48)
                      (subreg:V2DF (match_dup 2) 48)
                      (match_dup 3)] UNSPEC_FSBF))]
  ""
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_fsbfwcq"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  "HAVE_LVX_MINUS_V4SF && reload_completed"
  [(set (subreg:V4SF (match_dup 0) 0)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 0)
                      (subreg:V4SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V4SF (match_dup 0) 16)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 16)
                      (subreg:V4SF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FSBF))]
  ""
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  "!HAVE_LVX_MINUS_V4SF && reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 0)
                      (subreg:V2SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 8)
                      (subreg:V2SF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V2SF (match_dup 0) 16)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 16)
                      (subreg:V2SF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V2SF (match_dup 0) 24)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 24)
                      (subreg:V2SF (match_dup 2) 24)
                      (match_dup 3)] UNSPEC_FSBF))]
  ""
)

;; FSBF*

(define_insn "lvx_fsbfh"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (unspec:HF [(match_operand:HF 1 "register_operand" "r")
                    (match_operand:HF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "fsbfh%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp3")]
)

(define_insn "lvx_fsbfw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand:SF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "fsbfw%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn "lvx_fsbfd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:DF 1 "register_operand" "r")
                    (match_operand:DF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "fsbfd%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn "lvx_fsbf<suffix>"
  [(set (match_operand:S64F 0 "register_operand" "=R,Q")
        (unspec:S64F [(match_operand:S64F 1 "register_operand" "R,Q")
                      (match_operand:S64F 2 "register_operand" "R,Q")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "fsbf<suffix>%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_fsbfho"
  [(set (match_operand:V8HF 0 "register_operand" "=r")
        (unspec:V8HF [(match_operand:V8HF 1 "register_operand" "r")
                      (match_operand:V8HF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "fsbfho%3 %0 = %2, %1"
  "!HAVE_LVX_MINUS_V8HF && reload_completed"
  [(set (subreg:V4HF (match_dup 0) 0)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 0)
                      (subreg:V4HF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V4HF (match_dup 0) 8)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 8)
                      (subreg:V4HF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FSBF))]
  ""
  [(set_attr "type" "mult_fp3")]
)

(define_insn_and_split "lvx_fsbfwq"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "fsbfwq%3 %0 = %1, %2"
  "!HAVE_LVX_MINUS_V4SF && reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 0)
                      (subreg:V2SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 8)
                      (subreg:V2SF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FSBF))]
  ""
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn_and_split "lvx_fsbfdp"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand" "r")
                      (match_operand:V2DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "fsbfdp%3 %0 = %1, %2"
  "!HAVE_LVX_MINUS_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DF (match_dup 1) 0)
                    (subreg:DF (match_dup 2) 0)
                    (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DF (match_dup 1) 8)
                    (subreg:DF (match_dup 2) 8)
                    (match_dup 3)] UNSPEC_FSBF))]
  ""
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn "lvx_fsbfhx"
  [(set (match_operand:V16HF 0 "register_operand" "=r")
        (unspec:V16HF [(match_operand:V16HF 1 "register_operand" "r")
                      (match_operand:V16HF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V16HF 0 "register_operand" "")
        (unspec:V16HF [(match_operand:V16HF 1 "register_operand" "")
                      (match_operand:V16HF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  "!HAVE_LVX_MINUS_V8HF && reload_completed"
  [(set (subreg:V4HF (match_dup 0) 0)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 0)
                      (subreg:V4HF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V4HF (match_dup 0) 8)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 8)
                      (subreg:V4HF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V4HF (match_dup 0) 16)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 16)
                      (subreg:V4HF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V4HF (match_dup 0) 24)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 24)
                      (subreg:V4HF (match_dup 2) 24)
                      (match_dup 3)] UNSPEC_FSBF))]
  ""
)

(define_split
  [(set (match_operand:V16HF 0 "register_operand" "")
        (unspec:V16HF [(match_operand:V16HF 1 "register_operand" "")
                      (match_operand:V16HF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  "HAVE_LVX_MINUS_V8HF && reload_completed"
  [(set (subreg:V8HF (match_dup 0) 0)
        (unspec:V8HF [(subreg:V8HF (match_dup 1) 0)
                      (subreg:V8HF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V8HF (match_dup 0) 16)
        (unspec:V8HF [(subreg:V8HF (match_dup 1) 16)
                      (subreg:V8HF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FSBF))]
  ""
)

(define_insn "lvx_fsbfwo"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  "HAVE_LVX_MINUS_V4SF && reload_completed"
  [(set (subreg:V4SF (match_dup 0) 0)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 0)
                      (subreg:V4SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V4SF (match_dup 0) 16)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 16)
                      (subreg:V4SF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FSBF))]
  ""
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  "!HAVE_LVX_MINUS_V4SF && reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 0)
                      (subreg:V2SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 8)
                      (subreg:V2SF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V2SF (match_dup 0) 16)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 16)
                      (subreg:V2SF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V2SF (match_dup 0) 24)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 24)
                      (subreg:V2SF (match_dup 2) 24)
                      (match_dup 3)] UNSPEC_FSBF))]
  ""
)

(define_insn "lvx_fsbfdq"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "r")
                      (match_operand:V4DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "")
                      (match_operand:V4DF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  "HAVE_LVX_MINUS_V2DF && reload_completed"
  [(set (subreg:V2DF (match_dup 0) 0)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 0)
                      (subreg:V2DF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:V2DF (match_dup 0) 16)
        (unspec:V2DF [(subreg:V2DF (match_dup 1) 16)
                      (subreg:V2DF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FSBF))]
  ""
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "")
                      (match_operand:V4DF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  "!HAVE_LVX_MINUS_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DF (match_dup 1) 0)
                    (subreg:DF (match_dup 2) 0)
                    (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DF (match_dup 1) 8)
                    (subreg:DF (match_dup 2) 8)
                    (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:DF (match_dup 0) 16)
        (unspec:DF [(subreg:DF (match_dup 1) 16)
                    (subreg:DF (match_dup 2) 16)
                    (match_dup 3)] UNSPEC_FSBF))
   (set (subreg:DF (match_dup 0) 24)
        (unspec:DF [(subreg:DF (match_dup 1) 24)
                    (subreg:DF (match_dup 2) 24)
                    (match_dup 3)] UNSPEC_FSBF))]
  ""
)

;; FMUL*

(define_insn "lvx_fmulh"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (unspec:HF [(match_operand:HF 1 "register_operand" "r")
                    (match_operand:HF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FMUL))]
  ""
  "fmulh%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp3")]
)

(define_insn "lvx_fmulw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand:SF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FMUL))]
  ""
  "fmulw%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn "lvx_fmuld"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:DF 1 "register_operand" "r")
                    (match_operand:DF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FMUL))]
  ""
  "fmuld%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn "lvx_fdivw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand:SF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FDIV))]
  ""
  "fdivw%3 %0 = %1, %2"
  [(set_attr "type" "alu_full_sfu")]
)

(define_insn "lvx_fdivd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:DF 1 "register_operand" "r")
                    (match_operand:DF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FDIV))]
  ""
  "fdivd%3 %0 = %1, %2"
  [(set_attr "type" "alu_full_sfu")]
)

(define_insn "lvx_fmul<suffix>"
  [(set (match_operand:S64F 0 "register_operand" "=R,Q")
        (unspec:S64F [(match_operand:S64F 1 "register_operand" "R,Q")
                      (match_operand:S64F 2 "register_operand" "R,Q")
                      (match_operand 3 "" "")] UNSPEC_FMUL))]
  ""
  "fmul<suffix>%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_fmulho"
  [(set (match_operand:V8HF 0 "register_operand" "=r")
        (unspec:V8HF [(match_operand:V8HF 1 "register_operand" "r")
                      (match_operand:V8HF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMUL))]
  ""
  "fmulho%3 %0 = %1, %2"
  "!HAVE_LVX_MULT_V8HF && reload_completed"
  [(set (subreg:V4HF (match_dup 0) 0)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 0)
                      (subreg:V4HF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:V4HF (match_dup 0) 8)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 8)
                      (subreg:V4HF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FMUL))]
  ""
  [(set_attr "type" "mult_fp3")]
)

(define_insn_and_split "lvx_fmulwq"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMUL))]
  ""
  "fmulwq%3 %0 = %1, %2"
  "!HAVE_LVX_MULT_V4SF && reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 0)
                      (subreg:V2SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 8)
                      (subreg:V2SF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FMUL))]
  ""
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn_and_split "lvx_fmuldp"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand" "r")
                      (match_operand:V2DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMUL))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DF (match_dup 1) 0)
                    (subreg:DF (match_dup 2) 0)
                    (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DF (match_dup 1) 8)
                    (subreg:DF (match_dup 2) 8)
                    (match_dup 3)] UNSPEC_FMUL))]
  ""
  [(set_attr "type" "madd_fp4")]
)

(define_insn"lvx_fmulhx"
  [(set (match_operand:V16HF 0 "register_operand" "=r")
        (unspec:V16HF [(match_operand:V16HF 1 "register_operand" "r")
                      (match_operand:V16HF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMUL))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V16HF 0 "register_operand" "")
        (unspec:V16HF [(match_operand:V16HF 1 "register_operand" "")
                      (match_operand:V16HF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FMUL))]
  "!HAVE_LVX_MULT_V8HF && reload_completed"
  [(set (subreg:V4HF (match_dup 0) 0)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 0)
                      (subreg:V4HF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:V4HF (match_dup 0) 8)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 8)
                      (subreg:V4HF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:V4HF (match_dup 0) 16)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 16)
                      (subreg:V4HF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:V4HF (match_dup 0) 24)
        (unspec:V4HF [(subreg:V4HF (match_dup 1) 24)
                      (subreg:V4HF (match_dup 2) 24)
                      (match_dup 3)] UNSPEC_FMUL))]
  ""
)

(define_split
  [(set (match_operand:V16HF 0 "register_operand" "")
        (unspec:V16HF [(match_operand:V16HF 1 "register_operand" "")
                      (match_operand:V16HF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FMUL))]
  "HAVE_LVX_MULT_V8HF && reload_completed"
  [(set (subreg:V8HF (match_dup 0) 0)
        (unspec:V8HF [(subreg:V8HF (match_dup 1) 0)
                      (subreg:V8HF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:V8HF (match_dup 0) 16)
        (unspec:V8HF [(subreg:V8HF (match_dup 1) 16)
                      (subreg:V8HF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FMUL))]
  ""
)

(define_insn "lvx_fmulwo"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMUL))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FMUL))]
  "HAVE_LVX_MULT_V4SF && reload_completed"
  [(set (subreg:V4SF (match_dup 0) 0)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 0)
                      (subreg:V4SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:V4SF (match_dup 0) 16)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 16)
                      (subreg:V4SF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FMUL))]
  ""
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FMUL))]
  "!HAVE_LVX_MULT_V4SF && reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 0)
                      (subreg:V2SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 8)
                      (subreg:V2SF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:V2SF (match_dup 0) 16)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 16)
                      (subreg:V2SF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:V2SF (match_dup 0) 24)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 24)
                      (subreg:V2SF (match_dup 2) 24)
                      (match_dup 3)] UNSPEC_FMUL))]
  ""
)

(define_insn_and_split "lvx_fmuldq"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "r")
                      (match_operand:V4DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMUL))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DF (match_dup 1) 0)
                    (subreg:DF (match_dup 2) 0)
                    (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DF (match_dup 1) 8)
                    (subreg:DF (match_dup 2) 8)
                    (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:DF (match_dup 0) 16)
        (unspec:DF [(subreg:DF (match_dup 1) 16)
                    (subreg:DF (match_dup 2) 16)
                    (match_dup 3)] UNSPEC_FMUL))
   (set (subreg:DF (match_dup 0) 24)
        (unspec:DF [(subreg:DF (match_dup 1) 24)
                    (subreg:DF (match_dup 2) 24)
                    (match_dup 3)] UNSPEC_FMUL))]
  ""
  [(set_attr "type" "madd_fp4")]
)


;; FMUL*C

(define_insn "lvx_fmulwc"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")
                      (match_operand:V2SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMULC))]
  "HAVE_LVX_CPLX_MULT_V2SF"
  "fmulwc%3 %0 = %1, %2"
  [(set_attr "type" "dotp_fp4")]
)

(define_insn_and_split "lvx_fmulwcp"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMULC))]
  ""
  "fmulwcp%3 %0 = %1, %2"
  "!HAVE_LVX_CPLX_MULT_V4SF && reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 0)
                      (subreg:V2SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FMULC))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 8)
                      (subreg:V2SF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FMULC))]
  ""
  [(set_attr "type" "dmda_fp4")]
)

(define_insn "lvx_fmulwcq"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMULC))]
  ""
  "#"
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FMULC))]
  "!HAVE_LVX_CPLX_MULT_V4SF && reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 0)
                      (subreg:V2SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FMULC))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 8)
                      (subreg:V2SF (match_dup 2) 8)
                      (match_dup 3)] UNSPEC_FMULC))
   (set (subreg:V2SF (match_dup 0) 16)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 16)
                      (subreg:V2SF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FMULC))
   (set (subreg:V2SF (match_dup 0) 24)
        (unspec:V2SF [(subreg:V2SF (match_dup 1) 24)
                      (subreg:V2SF (match_dup 2) 24)
                      (match_dup 3)] UNSPEC_FMULC))]
  ""
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FMULC))]
  "HAVE_LVX_CPLX_MULT_V4SF && reload_completed"
  [(set (subreg:V4SF (match_dup 0) 0)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 0)
                      (subreg:V4SF (match_dup 2) 0)
                      (match_dup 3)] UNSPEC_FMULC))
   (set (subreg:V4SF (match_dup 0) 16)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 16)
                      (subreg:V4SF (match_dup 2) 16)
                      (match_dup 3)] UNSPEC_FMULC))]
  ""
)

(define_expand "lvx_fmuldc"
  [(set (match_operand:V2DF 0 "register_operand")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand")
                      (match_operand:V2DF 2 "register_operand")
                      (match_operand 3 "" "")] UNSPEC_FMULC))]
  ""
  {
    lvx_expand_builtin_fmuldc (operands, 1);
    DONE;
  }
)

(define_expand "lvx_fmuldcp"
  [(set (match_operand:V4DF 0 "register_operand" "")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "")
                      (match_operand:V4DF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FMULC))]
  ""
  {
    lvx_expand_builtin_fmuldc (operands, 2);
    DONE;
  }
)

(define_expand "lvx_fmuldcq"
  [(set (match_operand:V8DF 0 "register_operand" "")
        (unspec:V8DF [(match_operand:V8DF 1 "register_operand" "")
                      (match_operand:V8DF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FMULC))]
  ""
  {
    lvx_expand_builtin_fmuldc (operands, 4);
    DONE;
  }
)


;; FMULX*

(define_insn "lvx_fmulxhw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:HF 1 "register_operand" "r")
                    (match_operand:HF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FMULX))]
  "HAVE_LVX_MULT_SF_HF_HF"
  "fmulhw%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp3")]
)

(define_insn "lvx_fmulxwd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand:SF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FMULX))]
  "HAVE_LVX_MULT_DF_SF_SF"
  "fmulwd%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_insn "lvx_fmulx<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (unspec:<WIDE> [(match_operand:S64F 1 "register_operand" "r")
                        (match_operand:S64F 2 "register_operand" "r")
                        (match_operand 3 "" "")] UNSPEC_FMULX))]
  "HAVE_LVX_MULT_<WIDE>_<MODE>_<MODE>"
  "fmul<widenx>%3 %0 = %1, %2"
  [(set (attr "type")
     (if_then_else (match_operand 1 "float16_inner_mode") (const_string "mult_fp3") (const_string "mult_fp4")))]
)

(define_insn_and_split "lvx_fmulx<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=&r")
        (unspec:<WIDE> [(match_operand:S128F 1 "register_operand" "r")
                        (match_operand:S128F 2 "register_operand" "r")
                        (match_operand 3 "" "")] UNSPEC_FMULX))]
  "HAVE_LVX_MULT_<HWIDE>_<CHUNK>_<CHUNK>"
  "#"
  "reload_completed"
  [(set (subreg:<HWIDE> (match_dup 0) 0)
        (unspec:<HWIDE> [(subreg:<CHUNK> (match_dup 1) 0)
                         (subreg:<CHUNK> (match_dup 2) 0)
                         (match_dup 3)] UNSPEC_FMULX))
   (set (subreg:<HWIDE> (match_dup 0) 16)
        (unspec:<HWIDE> [(subreg:<CHUNK> (match_dup 1) 8)
                         (subreg:<CHUNK> (match_dup 2) 8)
                         (match_dup 3)] UNSPEC_FMULX))]
  ""
)

;; FDIV*C

(define_expand "lvx_fdivwc"
  [(match_operand:V2SF 0 "register_operand")
   (match_operand:V2SF 1 "register_operand")
   (match_operand:V2SF 2 "register_operand")
   (match_operand 3 "" "")]
  ""
  {
     bool conjugate = lvx_modifier_enabled_p (".c", operands[3]);
     rtx rnd = lvx_modifier_rounding (operands[3]);
     rtx conj = gen_rtx_CONST_STRING (VOIDmode, ".c");
     rtx conj_rnd = gen_rtx_CONCAT (VOIDmode, conj, rnd);
     rtx bmag = gen_reg_rtx (SFmode);
     rtx bmag_inv = gen_reg_rtx (SFmode);
     rtx bmag_inv_splat = gen_reg_rtx (V2SFmode);
     rtx a_conjb = gen_reg_rtx (V2SFmode);
     if (conjugate)
       {
         rtx cplx1_rtx = gen_reg_rtx (V2SFmode);
         rtx const0f_reg = gen_reg_rtx (SFmode);
         rtx const1f_reg = gen_reg_rtx (SFmode);
         emit_insn (gen_rtx_SET (const0f_reg, CONST0_RTX (SFmode)));
         emit_insn (gen_rtx_SET (const1f_reg, CONST1_RTX (SFmode)));
         emit_insn (gen_lvx_catfwp (cplx1_rtx, const1f_reg, const0f_reg));
         emit_insn (gen_lvx_fmulwc (operands[1], operands[1], cplx1_rtx, conj_rnd));
       }
     if (!HAVE_LVX_FDMA_SF_V2SF_V2SF)
       emit_insn (gen_lvx_ffdmaw (bmag, operands[2], operands[2], rnd));
     else
       emit_insn (gen_lvx_ffdmaw_2 (bmag, operands[2], operands[2], rnd));

     emit_insn (gen_lvx_frecw (bmag_inv, bmag, rnd));
     emit_insn (gen_lvx_catfwp (bmag_inv_splat, bmag_inv, bmag_inv));
     emit_insn (gen_lvx_fmulwc (a_conjb, operands[2], operands[1], conj_rnd));
     emit_insn (gen_lvx_fmulwp (operands[0], a_conjb, bmag_inv_splat, rnd));
     DONE;
  })

(define_expand "lvx_fdivwcp"
  [(match_operand:V4SF 0 "register_operand")
   (match_operand:V4SF 1 "register_operand")
   (match_operand:V4SF 2 "register_operand")
   (match_operand 3 "" "")]
  ""
  {
    if (!HAVE_LVX_CPLX_MULT_V4SF)
      emit_insn (gen_lvx_fdivwcp_1 (operands[0], operands[1], operands[2], operands[3]));
    if (HAVE_LVX_CPLX_MULT_V4SF)
      emit_insn (gen_lvx_fdivwcp_2 (operands[0], operands[1], operands[2], operands[3]));
    DONE;
  })

(define_expand "lvx_fdivwcp_1"
  [(match_operand:V4SF 0 "register_operand")
   (match_operand:V4SF 1 "register_operand")
   (match_operand:V4SF 2 "register_operand")
   (match_operand 3 "" "")]
  "!HAVE_LVX_CPLX_MULT_V4SF"
  {
    rtx high1 = simplify_gen_subreg (V2SFmode, operands[1], V4SFmode, 0);
    rtx high2 = simplify_gen_subreg (V2SFmode, operands[2], V4SFmode, 0);
    rtx high = gen_reg_rtx (V2SFmode);
    emit_insn (gen_lvx_fdivwc (high, high1, high2, operands[3]));
    rtx low1 = simplify_gen_subreg (V2SFmode, operands[1], V4SFmode, 8);
    rtx low2 = simplify_gen_subreg (V2SFmode, operands[2], V4SFmode, 8);
    rtx low = gen_reg_rtx (V2SFmode);
    emit_insn (gen_lvx_fdivwc (low, low1, low2, operands[3]));
    emit_insn (gen_lvx_catfwq (operands[0], high, low));
    DONE;
  })

(define_expand "lvx_fdivwcp_2"
  [(match_operand:V4SF 0 "register_operand")
   (match_operand:V4SF 1 "register_operand")
   (match_operand:V4SF 2 "register_operand")
   (match_operand 3 "" "")]
  "HAVE_LVX_CPLX_MULT_V4SF"
  {
     bool conjugate = lvx_modifier_enabled_p (".c", operands[3]);
     rtx rnd = lvx_modifier_rounding (operands[3]);
     rtx conj = gen_rtx_CONST_STRING (VOIDmode, ".c");
     rtx conj_rnd = gen_rtx_CONCAT (VOIDmode, conj, rnd);
     rtx b = simplify_gen_subreg (V2SFmode, operands[2], V4SFmode, 0);
     rtx d = simplify_gen_subreg (V2SFmode, operands[2], V4SFmode, 8);
     rtx bmag = gen_reg_rtx (SFmode);
     rtx dmag = gen_reg_rtx (SFmode);
     rtx bdmag = gen_reg_rtx (V2SFmode);
     rtx bdmag_inv = gen_reg_rtx (V2SFmode);
     rtx bdmag_inv_splat = gen_reg_rtx (V4SFmode);
     rtx ac_conjbd = gen_reg_rtx (V4SFmode);
     if (conjugate)
       {
         rtx cplx1_rtx = gen_reg_rtx (V2SFmode);
         rtx cplx1p_rtx = gen_reg_rtx (V4SFmode);
         rtx const0f_reg = gen_reg_rtx (SFmode);
         rtx const1f_reg = gen_reg_rtx (SFmode);
         emit_insn (gen_rtx_SET (const0f_reg, CONST0_RTX (SFmode)));
         emit_insn (gen_rtx_SET (const1f_reg, CONST1_RTX (SFmode)));
         emit_insn (gen_lvx_catfwp (cplx1_rtx, const1f_reg, const0f_reg));
         emit_insn (gen_lvx_catfwq (cplx1p_rtx, cplx1_rtx, cplx1_rtx));
         emit_insn (gen_lvx_fmulwcp (operands[1], operands[1], cplx1p_rtx, conj_rnd));
       }
     emit_insn (gen_lvx_ffdmaw_2 (bmag, b, b, rnd));
     emit_insn (gen_lvx_ffdmaw_2 (dmag, d, d, rnd));
     emit_insn (gen_lvx_catfwp (bdmag, bmag, dmag));

     emit_insn (gen_lvx_frecwp (bdmag_inv, bdmag, rnd));
     emit_insn (gen_lvx_catfwq (bdmag_inv_splat, bdmag_inv, bdmag_inv));
     emit_insn (gen_lvx_fmt22w (bdmag_inv_splat, bdmag_inv_splat));
     emit_insn (gen_lvx_fmulwcp (ac_conjbd, operands[2], operands[1], conj_rnd));
     emit_insn (gen_lvx_fmulwq (operands[0], ac_conjbd, bdmag_inv_splat, rnd));
     DONE;
  })

(define_expand "lvx_fdivwcq"
  [(match_operand:V8SF 0 "register_operand")
   (match_operand:V8SF 1 "register_operand")
   (match_operand:V8SF 2 "register_operand")
   (match_operand 3 "" "")]

  ""
  {
     rtx high1 = simplify_gen_subreg (V4SFmode, operands[1], V8SFmode, 0);
     rtx high2 = simplify_gen_subreg (V4SFmode, operands[2], V8SFmode, 0);
     rtx high = gen_reg_rtx (V4SFmode);
     emit_insn (gen_lvx_fdivwcp (high, high1, high2, operands[3]));
     rtx low1 = simplify_gen_subreg (V4SFmode, operands[1], V8SFmode, 16);
     rtx low2 = simplify_gen_subreg (V4SFmode, operands[2], V8SFmode, 16);
     rtx low = gen_reg_rtx (V4SFmode);
     emit_insn (gen_lvx_fdivwcp (low, low1, low2, operands[3]));
     rtx high0 = simplify_gen_subreg (V4SFmode, operands[0], V8SFmode, 0);
     rtx low0 = simplify_gen_subreg (V4SFmode, operands[0], V8SFmode, 16);
     emit_insn (gen_rtx_SET (high0, high));
     emit_insn (gen_rtx_SET (low0, low));
     DONE;
  })

(define_expand "lvx_fdivwco"
  [(match_operand:V16SF 0 "register_operand")
   (match_operand:V16SF 1 "register_operand")
   (match_operand:V16SF 2 "register_operand")
   (match_operand 3 "" "")]
  ""
  {
     rtx high1 = simplify_gen_subreg (V8SFmode, operands[1], V16SFmode, 0);
     rtx high2 = simplify_gen_subreg (V8SFmode, operands[2], V16SFmode, 0);
     rtx high = gen_reg_rtx (V8SFmode);
     emit_insn (gen_lvx_fdivwcq (high, high1, high2, operands[3]));
     rtx low1 = simplify_gen_subreg (V8SFmode, operands[1], V16SFmode, 32);
     rtx low2 = simplify_gen_subreg (V8SFmode, operands[2], V16SFmode, 32);
     rtx low = gen_reg_rtx (V8SFmode);
     emit_insn (gen_lvx_fdivwcq (low, low1, low2, operands[3]));
     rtx high0 = simplify_gen_subreg (V8SFmode, operands[0], V16SFmode, 0);
     rtx low0 = simplify_gen_subreg (V8SFmode, operands[0], V16SFmode, 32);
     emit_insn (gen_rtx_SET (high0, high));
     emit_insn (gen_rtx_SET (low0, low));
     DONE;
  })

(define_expand "lvx_fdivdc"
  [(match_operand:V2DF 0 "register_operand")
   (match_operand:V2DF 1 "register_operand")
   (match_operand:V2DF 2 "register_operand")
   (match_operand 3 "" "")]
  ""
  {
     bool conjugate = lvx_modifier_enabled_p (".c", operands[3]);
     rtx rnd = lvx_modifier_rounding (operands[3]);
     rtx conj = gen_rtx_CONST_STRING (VOIDmode, ".c");
     rtx conj_rnd = gen_rtx_CONCAT (VOIDmode, conj, rnd);
     rtx cplx1_rtx = gen_reg_rtx (V2DFmode);
     rtx const0f_reg = gen_reg_rtx (DFmode);
     rtx const1f_reg = gen_reg_rtx (DFmode);
     emit_insn (gen_rtx_SET (const0f_reg, CONST0_RTX (DFmode)));
     emit_insn (gen_rtx_SET (const1f_reg, CONST1_RTX (DFmode)));
     rtx real_2 = simplify_gen_subreg (DFmode, operands[2], V2DFmode, 0);
     rtx imag_2 = simplify_gen_subreg (DFmode, operands[2], V2DFmode, 8);
     rtx bmag_inv = gen_reg_rtx (DFmode);
     rtx bmag_inv_splat = gen_reg_rtx (V2DFmode);
     rtx a_conjb = gen_reg_rtx (V2DFmode);
     if (conjugate)
       {
         emit_insn (gen_lvx_catfdp (cplx1_rtx, const1f_reg, const0f_reg));
         emit_insn (gen_lvx_fmuldc (operands[1], operands[1], cplx1_rtx, conj_rnd));
       }
     emit_insn (gen_lvx_fmuld (bmag_inv, real_2, real_2, rnd));
     emit_insn (gen_lvx_ffmad (bmag_inv, imag_2, imag_2, bmag_inv, rnd));
     emit_insn (gen_divdf3 (bmag_inv, const1f_reg, bmag_inv));
     emit_insn (gen_lvx_catfdp (bmag_inv_splat, bmag_inv, bmag_inv));
     emit_insn (gen_lvx_fmuldc (a_conjb, operands[2], operands[1], conj_rnd));
     emit_insn (gen_lvx_fmuldp (operands[0], a_conjb, bmag_inv_splat, rnd));
     DONE;
  })

(define_expand "lvx_fdivdcp"
  [(match_operand:V4DF 0 "register_operand")
   (match_operand:V4DF 1 "register_operand")
   (match_operand:V4DF 2 "register_operand")
   (match_operand 3 "" "")]
  ""
  {
     rtx high1 = simplify_gen_subreg (V2DFmode, operands[1], V4DFmode, 0);
     rtx high2 = simplify_gen_subreg (V2DFmode, operands[2], V4DFmode, 0);
     rtx high = gen_reg_rtx (V2DFmode);
     emit_insn (gen_lvx_fdivdc (high, high1, high2, operands[3]));
     rtx low1 = simplify_gen_subreg (V2DFmode, operands[1], V4DFmode, 16);
     rtx low2 = simplify_gen_subreg (V2DFmode, operands[2], V4DFmode, 16);
     rtx low = gen_reg_rtx (V2DFmode);
     emit_insn (gen_lvx_fdivdc (low, low1, low2, operands[3]));
     rtx high0 = simplify_gen_subreg (V2DFmode, operands[0], V4DFmode, 0);
     rtx low0 = simplify_gen_subreg (V2DFmode, operands[0], V4DFmode, 16);
     emit_insn (gen_rtx_SET (high0, high));
     emit_insn (gen_rtx_SET (low0, low));
     DONE;
  })

(define_expand "lvx_fdivdcq"
  [(match_operand:V8DF 0 "register_operand")
   (match_operand:V8DF 1 "register_operand")
   (match_operand:V8DF 2 "register_operand")
   (match_operand 3 "" "")]
  ""
  {
     rtx high1 = simplify_gen_subreg (V4DFmode, operands[1], V8DFmode, 0);
     rtx high2 = simplify_gen_subreg (V4DFmode, operands[2], V8DFmode, 0);
     rtx high = gen_reg_rtx (V4DFmode);
     emit_insn (gen_lvx_fdivdcp (high, high1, high2, operands[3]));
     rtx low1 = simplify_gen_subreg (V4DFmode, operands[1], V8DFmode, 32);
     rtx low2 = simplify_gen_subreg (V4DFmode, operands[2], V8DFmode, 32);
     rtx low = gen_reg_rtx (V4DFmode);
     emit_insn (gen_lvx_fdivdcp (low, low1, low2, operands[3]));
     rtx high0 = simplify_gen_subreg (V4DFmode, operands[0], V8DFmode, 0);
     rtx low0 = simplify_gen_subreg (V4DFmode, operands[0], V8DFmode, 32);
     emit_insn (gen_rtx_SET (high0, high));
     emit_insn (gen_rtx_SET (low0, low));
     DONE;
  })

;; FFMA*

(define_insn "lvx_ffmah"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (unspec:HF [(match_operand:HF 1 "register_operand" "r")
                    (match_operand:HF 2 "register_operand" "r")
                    (match_operand:HF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFMA))]
  ""
  "ffmah%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp3")]
)

(define_insn "lvx_ffmaw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand:SF 2 "register_operand" "r")
                    (match_operand:SF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFMA))]
  ""
  "ffmaw%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_ffmad"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:DF 1 "register_operand" "r")
                    (match_operand:DF 2 "register_operand" "r")
                    (match_operand:DF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFMA))]
  ""
  "ffmad%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_ffma<suffix>"
  [(set (match_operand:S64F 0 "register_operand" "=R,Q")
        (unspec:S64F [(match_operand:S64F 1 "register_operand" "R,Q")
                      (match_operand:S64F 2 "register_operand" "R,Q")
                      (match_operand:S64F 3 "register_operand" "0,0")
                      (match_operand 4 "" "")] UNSPEC_FFMA))]
  ""
  "ffma<suffix>%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_expand "lvx_ffma<suffix>"
  [(set (match_operand:S128F 0 "register_operand" "")
        (unspec:S128F [(match_operand:S128F 1 "register_operand" "")
                       (match_operand:S128F 2 "register_operand" "")
                       (match_operand:S128F 3 "register_operand" "")
                       (match_operand 4 "" "")] UNSPEC_FFMA))]
  ""
  ""
)

(define_insn_and_split "lvx_ffma<suffix>_1"
  [(set (match_operand:S128F 0 "register_operand" "=r")
        (unspec:S128F [(match_operand:S128F 1 "register_operand" "r")
                       (match_operand:S128F 2 "register_operand" "r")
                       (match_operand:S128F 3 "register_operand" "0")
                       (match_operand 4 "" "")] UNSPEC_FFMA))]
  "!HAVE_LVX_FMA_<MODE>_<MODE>_<MODE>"
  "#"
  "&& reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 0)
                         (subreg:<CHUNK> (match_dup 2) 0)
                         (subreg:<CHUNK> (match_dup 3) 0)
                         (match_dup 4)] UNSPEC_FFMA))
   (set (subreg:<CHUNK> (match_dup 0) 8)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 8)
                         (subreg:<CHUNK> (match_dup 2) 8)
                         (subreg:<CHUNK> (match_dup 3) 8)
                         (match_dup 4)] UNSPEC_FFMA))]
  ""
)

(define_insn "lvx_ffma<suffix>_2"
  [(set (match_operand:S128F 0 "register_operand" "=r")
        (unspec:S128F [(match_operand:S128F 1 "register_operand" "r")
                       (match_operand:S128F 2 "register_operand" "r")
                       (match_operand:S128F 3 "register_operand" "0")
                       (match_operand 4 "" "")] UNSPEC_FFMA))]
  "HAVE_LVX_FMA_<MODE>_<MODE>_<MODE>"
  "ffma<suffix>%4 %0 = %1, %2"
  [(set (attr "type")
     (if_then_else (match_operand 1 "float16_inner_mode") (const_string "madd_fp3") (const_string "madd_fp4")))]
)

(define_insn_and_split "lvx_ffmadp"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand" "r")
                      (match_operand:V2DF 2 "register_operand" "r")
                      (match_operand:V2DF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFMA))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DF (match_dup 1) 0)
                    (subreg:DF (match_dup 2) 0)
                    (subreg:DF (match_dup 3) 0)
                    (match_dup 4)] UNSPEC_FFMA))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DF (match_dup 1) 8)
                    (subreg:DF (match_dup 2) 8)
                    (subreg:DF (match_dup 3) 8)
                    (match_dup 4)] UNSPEC_FFMA))]
  ""
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_ffma<suffix>"
  [(set (match_operand:S256F 0 "register_operand" "=r")
        (unspec:S256F [(match_operand:S256F 1 "register_operand" "r")
                       (match_operand:S256F 2 "register_operand" "r")
                       (match_operand:S256F 3 "register_operand" "0")
                       (match_operand 4 "" "")] UNSPEC_FFMA))]
  ""
  "#"
)

(define_split
  [(set (match_operand:S256F 0 "register_operand" "")
        (unspec:S256F [(match_operand:S256F 1 "register_operand" "")
                       (match_operand:S256F 2 "register_operand" "")
                       (match_operand:S256F 3 "register_operand" "")
                       (match_operand 4 "" "")] UNSPEC_FFMA))]
  "!HAVE_LVX_FMA_<HALF>_<HALF>_<HALF> && reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 0)
                         (subreg:<CHUNK> (match_dup 2) 0)
                         (subreg:<CHUNK> (match_dup 3) 0)
                         (match_dup 4)] UNSPEC_FFMA))
   (set (subreg:<CHUNK> (match_dup 0) 8)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 8)
                         (subreg:<CHUNK> (match_dup 2) 8)
                         (subreg:<CHUNK> (match_dup 3) 8)
                         (match_dup 4)] UNSPEC_FFMA))
   (set (subreg:<CHUNK> (match_dup 0) 16)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 16)
                         (subreg:<CHUNK> (match_dup 2) 16)
                         (subreg:<CHUNK> (match_dup 3) 16)
                         (match_dup 4)] UNSPEC_FFMA))
   (set (subreg:<CHUNK> (match_dup 0) 24)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 24)
                         (subreg:<CHUNK> (match_dup 2) 24)
                         (subreg:<CHUNK> (match_dup 3) 24)
                         (match_dup 4)] UNSPEC_FFMA))]
  ""
)

(define_split
  [(set (match_operand:S256F 0 "register_operand" "")
        (unspec:S256F [(match_operand:S256F 1 "register_operand" "")
                       (match_operand:S256F 2 "register_operand" "")
                       (match_operand:S256F 3 "register_operand" "")
                       (match_operand 4 "" "")] UNSPEC_FFMA))]
  "HAVE_LVX_FMA_<HALF>_<HALF>_<HALF> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)
                        (subreg:<HALF> (match_dup 3) 0)
                        (match_dup 4)] UNSPEC_FFMA))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)
                        (subreg:<HALF> (match_dup 3) 16)
                        (match_dup 4)] UNSPEC_FFMA))]
  ""
)

(define_insn_and_split "lvx_ffmadq"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "r")
                      (match_operand:V4DF 2 "register_operand" "r")
                      (match_operand:V4DF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFMA))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DF (match_dup 1) 0)
                    (subreg:DF (match_dup 2) 0)
                    (subreg:DF (match_dup 3) 0)
                    (match_dup 4)] UNSPEC_FFMA))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DF (match_dup 1) 8)
                    (subreg:DF (match_dup 2) 8)
                    (subreg:DF (match_dup 3) 8)
                    (match_dup 4)] UNSPEC_FFMA))
   (set (subreg:DF (match_dup 0) 16)
        (unspec:DF [(subreg:DF (match_dup 1) 16)
                    (subreg:DF (match_dup 2) 16)
                    (subreg:DF (match_dup 3) 16)
                    (match_dup 4)] UNSPEC_FFMA))
   (set (subreg:DF (match_dup 0) 24)
        (unspec:DF [(subreg:DF (match_dup 1) 24)
                    (subreg:DF (match_dup 2) 24)
                    (subreg:DF (match_dup 3) 24)
                    (match_dup 4)] UNSPEC_FFMA))]
  ""
  [(set_attr "type" "madd_fp4")]
)


;; FFMA*C

(define_expand "lvx_ffmawc"
  [(set (match_operand:V2SF 0 "register_operand" "")
        (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "")
                      (match_operand:V2SF 2 "register_operand" "")
                      (match_operand:V2SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFMAC))]
  ""
  {
    if (!HAVE_LVX_CPLX_FMA_V2SF_V2SF_V2SF)
      {
        rtx product = gen_reg_rtx (V2SFmode);
        emit_insn (gen_lvx_fmulwc (product, operands[2], operands[1], operands[4]));
        emit_insn (gen_lvx_faddwc (operands[0], product, operands[3], operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffmawc_2"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")
                      (match_operand:V2SF 2 "register_operand" "r")
                      (match_operand:V2SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFMAC))]
  "HAVE_LVX_CPLX_FMA_V2SF_V2SF_V2SF"
  "ffmawc%4 %0 = %1, %2"
  [(set_attr "type" "dmda_fp4")]
)

(define_expand "lvx_ffmawcp"
  [(set (match_operand:V4SF 0 "register_operand" "")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "")
                      (match_operand:V4SF 2 "register_operand" "")
                      (match_operand:V4SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFMAC))]
  ""
  {
    if (!HAVE_LVX_CPLX_FMA_V4SF_V4SF_V4SF)
      {
        for (int i = 0; i < 2; i++)
          {
            rtx product = gen_reg_rtx (V2SFmode);
            rtx opnd0 = gen_rtx_SUBREG (V2SFmode, operands[0], i*8);
            rtx opnd1 = gen_rtx_SUBREG (V2SFmode, operands[1], i*8);
            rtx opnd2 = gen_rtx_SUBREG (V2SFmode, operands[2], i*8);
            rtx opnd3 = gen_rtx_SUBREG (V2SFmode, operands[3], i*8);
            emit_insn (gen_lvx_fmulwc (product, opnd2, opnd1, operands[4]));
            emit_insn (gen_lvx_faddwc (opnd0, product, opnd3, operands[4]));
          }
        DONE;
      }
  }
)

(define_insn "lvx_ffmawcp_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand:V4SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFMAC))]
  "HAVE_LVX_CPLX_FMA_V4SF_V4SF_V4SF"
  "ffmawcp%4 %0 = %1, %2"
  [(set_attr "type" "dmda_fp4")]
)

(define_expand "lvx_ffmadc"
  [(set (match_operand:V2DF 0 "register_operand")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand")
                      (match_operand:V2DF 2 "register_operand")
                      (match_operand:V2DF 3 "register_operand")
                      (match_operand 4 "" "")] UNSPEC_FFMAC))]
  ""
  {
    rtx product = gen_reg_rtx (V2DFmode);
    emit_insn (gen_lvx_fmuldc (product, operands[2], operands[1], operands[4]));
    emit_insn (gen_lvx_fadddp (operands[0], product, operands[3], operands[4]));
    DONE;
  }
)

(define_expand "lvx_ffmawcq"
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand:V8SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFMAC))]
  ""
  {
    if (!HAVE_LVX_CPLX_FMA_V4SF_V4SF_V4SF)
      {
        for (int i = 0; i < 4; i++)
          {
            rtx product = gen_reg_rtx (V2SFmode);
            rtx opnd0 = gen_rtx_SUBREG (V2SFmode, operands[0], i*8);
            rtx opnd1 = gen_rtx_SUBREG (V2SFmode, operands[1], i*8);
            rtx opnd2 = gen_rtx_SUBREG (V2SFmode, operands[2], i*8);
            rtx opnd3 = gen_rtx_SUBREG (V2SFmode, operands[3], i*8);
            emit_insn (gen_lvx_fmulwc (product, opnd2, opnd1, operands[4]));
            emit_insn (gen_lvx_faddwc (opnd0, product, opnd3, operands[4]));
          }
        DONE;
      }
  }
)

(define_insn_and_split "lvx_ffmawcq_2"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand:V8SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFMAC))]
  "HAVE_LVX_CPLX_FMA_V4SF_V4SF_V4SF"
  "#"
  "&& reload_completed"
  [(set (subreg:V4SF (match_dup 0) 0)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 0)
                      (subreg:V4SF (match_dup 2) 0)
                      (subreg:V4SF (match_dup 3) 0)
                      (match_dup 4)] UNSPEC_FFMAC))
   (set (subreg:V4SF (match_dup 0) 16)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 16)
                      (subreg:V4SF (match_dup 2) 16)
                      (subreg:V4SF (match_dup 3) 16)
                      (match_dup 4)] UNSPEC_FFMAC))]
  ""
  [(set_attr "type" "dmda_fp4")]
)

(define_expand "lvx_ffmadcp"
  [(set (match_operand:V4DF 0 "register_operand")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand")
                      (match_operand:V4DF 2 "register_operand")
                      (match_operand:V4DF 3 "register_operand")
                      (match_operand 4 "" "")] UNSPEC_FFMAC))]
  ""
  {
    rtx product = gen_reg_rtx (V4DFmode);
    emit_insn (gen_lvx_fmuldcp (product, operands[2], operands[1], operands[4]));
    emit_insn (gen_lvx_fadddcp (operands[0], product, operands[3], operands[4]));
    DONE;
  }
)

(define_expand "lvx_ffmadcq"
  [(set (match_operand:V8DF 0 "register_operand")
        (unspec:V8DF [(match_operand:V8DF 1 "register_operand")
                      (match_operand:V8DF 2 "register_operand")
                      (match_operand:V8DF 3 "register_operand")
                      (match_operand 4 "" "")] UNSPEC_FFMAC))]
  ""
  {
    rtx product = gen_reg_rtx (V8DFmode);
    emit_insn (gen_lvx_fmuldcq (product, operands[2], operands[1], operands[4]));
    emit_insn (gen_lvx_fadddcq (operands[0], product, operands[3], operands[4]));
    DONE;
  }
)


;; FFMAX*

(define_insn "lvx_ffmaxhw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:HF 1 "register_operand" "r")
                    (match_operand:HF 2 "register_operand" "r")
                    (match_operand:SF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFMAX))]
  "HAVE_LVX_FMA_SF_HF_HF"
  "ffmahw%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp3")]
)

(define_insn "lvx_ffmaxwd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand:SF 2 "register_operand" "r")
                    (match_operand:DF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFMAX))]
  "HAVE_LVX_FMA_DF_SF_SF"
  "ffmawd%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_ffmax<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (unspec:<WIDE> [(match_operand:S64F 1 "register_operand" "r")
                        (match_operand:S64F 2 "register_operand" "r")
                        (match_operand:<WIDE> 3 "register_operand" "0")
                        (match_operand 4 "" "")] UNSPEC_FFMAX))]
  "HAVE_LVX_FMA_<WIDE>_<S64F:MODE>_<S64F:MODE>"
  "ffma<widenx>%4 %0 = %1, %2"
  [(set (attr "type")
     (if_then_else (match_operand 1 "float16_inner_mode") (const_string "madd_fp3") (const_string "madd_fp4")))]
)

(define_insn_and_split "lvx_ffmax<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=&r")
        (unspec:<WIDE> [(match_operand:S128F 1 "register_operand" "r")
                        (match_operand:S128F 2 "register_operand" "r")
                        (match_operand:<WIDE> 3 "register_operand" "0")
                        (match_operand 4 "" "")] UNSPEC_FFMAX))]
  "HAVE_LVX_FMA_<HWIDE>_<CHUNK>_<CHUNK>"
  "#"
  "reload_completed"
  [(set (subreg:<HWIDE> (match_dup 0) 0)
        (unspec:<HWIDE> [(subreg:<CHUNK> (match_dup 1) 0)
                         (subreg:<CHUNK> (match_dup 2) 0)
                         (subreg:<HWIDE> (match_dup 3) 0)
                         (match_dup 4)] UNSPEC_FFMAX))
   (set (subreg:<HWIDE> (match_dup 0) 16)
        (unspec:<HWIDE> [(subreg:<CHUNK> (match_dup 1) 8)
                         (subreg:<CHUNK> (match_dup 2) 8)
                         (subreg:<HWIDE> (match_dup 3) 16)
                         (match_dup 4)] UNSPEC_FFMAX))]
  ""
)


;; FFMS*

(define_insn "lvx_ffmsh"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (unspec:HF [(match_operand:HF 1 "register_operand" "r")
                    (match_operand:HF 2 "register_operand" "r")
                    (match_operand:HF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFMS))]
  ""
  "ffmsh%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp3")]
)

(define_insn "lvx_ffmsw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand:SF 2 "register_operand" "r")
                    (match_operand:SF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFMS))]
  ""
  "ffmsw%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_ffmsd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:DF 1 "register_operand" "r")
                    (match_operand:DF 2 "register_operand" "r")
                    (match_operand:DF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFMS))]
  ""
  "ffmsd%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_ffms<suffix>"
  [(set (match_operand:S64F 0 "register_operand" "=R,Q")
        (unspec:S64F [(match_operand:S64F 1 "register_operand" "R,Q")
                      (match_operand:S64F 2 "register_operand" "R,Q")
                      (match_operand:S64F 3 "register_operand" "0,0")
                      (match_operand 4 "" "")] UNSPEC_FFMS))]
  ""
  "ffms<suffix>%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_expand "lvx_ffms<suffix>"
  [(set (match_operand:S128F 0 "register_operand" "")
        (unspec:S128F [(match_operand:S128F 1 "register_operand" "")
                       (match_operand:S128F 2 "register_operand" "")
                       (match_operand:S128F 3 "register_operand" "")
                       (match_operand 4 "" "")] UNSPEC_FFMS))]
  ""
  ""
)

(define_insn_and_split "lvx_ffms<suffix>_1"
  [(set (match_operand:S128F 0 "register_operand" "=r")
        (unspec:S128F [(match_operand:S128F 1 "register_operand" "r")
                       (match_operand:S128F 2 "register_operand" "r")
                       (match_operand:S128F 3 "register_operand" "0")
                       (match_operand 4 "" "")] UNSPEC_FFMS))]
  "!HAVE_LVX_FMS_<MODE>_<MODE>_<MODE>"
  "#"
  "!HAVE_LVX_FMS_<MODE>_<MODE>_<MODE> && reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 0)
                         (subreg:<CHUNK> (match_dup 2) 0)
                         (subreg:<CHUNK> (match_dup 3) 0)
                         (match_dup 4)] UNSPEC_FFMS))
   (set (subreg:<CHUNK> (match_dup 0) 8)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 8)
                         (subreg:<CHUNK> (match_dup 2) 8)
                         (subreg:<CHUNK> (match_dup 3) 8)
                         (match_dup 4)] UNSPEC_FFMS))]
  ""
)

(define_insn "lvx_ffms<suffix>_2"
  [(set (match_operand:S128F 0 "register_operand" "=r")
        (unspec:S128F [(match_operand:S128F 1 "register_operand" "r")
                       (match_operand:S128F 2 "register_operand" "r")
                       (match_operand:S128F 3 "register_operand" "0")
                       (match_operand 4 "" "")] UNSPEC_FFMS))]
  "HAVE_LVX_FMS_<MODE>_<MODE>_<MODE>"
  "ffms<suffix>%4 %0 = %1, %2"
  [(set (attr "type")
     (if_then_else (match_operand 1 "float16_inner_mode") (const_string "madd_fp3") (const_string "madd_fp4")))]
)

(define_insn_and_split "lvx_ffmsdp"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand" "r")
                      (match_operand:V2DF 2 "register_operand" "r")
                      (match_operand:V2DF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFMS))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DF (match_dup 1) 0)
                    (subreg:DF (match_dup 2) 0)
                    (subreg:DF (match_dup 3) 0)
                    (match_dup 4)] UNSPEC_FFMS))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DF (match_dup 1) 8)
                    (subreg:DF (match_dup 2) 8)
                    (subreg:DF (match_dup 3) 8)
                    (match_dup 4)] UNSPEC_FFMS))]
  ""
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_ffms<suffix>"
  [(set (match_operand:S256F 0 "register_operand" "=r")
        (unspec:S256F [(match_operand:S256F 1 "register_operand" "r")
                       (match_operand:S256F 2 "register_operand" "r")
                       (match_operand:S256F 3 "register_operand" "0")
                       (match_operand 4 "" "")] UNSPEC_FFMS))]
  ""
  "#"
)

(define_split
  [(set (match_operand:S256F 0 "register_operand" "")
        (unspec:S256F [(match_operand:S256F 1 "register_operand" "")
                       (match_operand:S256F 2 "register_operand" "")
                       (match_operand:S256F 3 "register_operand" "")
                       (match_operand 4 "" "")] UNSPEC_FFMS))]
  "!HAVE_LVX_FMS_<HALF>_<HALF>_<HALF> && reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 0)
                         (subreg:<CHUNK> (match_dup 2) 0)
                         (subreg:<CHUNK> (match_dup 3) 0)
                         (match_dup 4)] UNSPEC_FFMS))
   (set (subreg:<CHUNK> (match_dup 0) 8)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 8)
                         (subreg:<CHUNK> (match_dup 2) 8)
                         (subreg:<CHUNK> (match_dup 3) 8)
                         (match_dup 4)] UNSPEC_FFMS))
   (set (subreg:<CHUNK> (match_dup 0) 16)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 16)
                         (subreg:<CHUNK> (match_dup 2) 16)
                         (subreg:<CHUNK> (match_dup 3) 16)
                         (match_dup 4)] UNSPEC_FFMS))
   (set (subreg:<CHUNK> (match_dup 0) 24)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 24)
                         (subreg:<CHUNK> (match_dup 2) 24)
                         (subreg:<CHUNK> (match_dup 3) 24)
                         (match_dup 4)] UNSPEC_FFMS))]
  ""
)

(define_split
  [(set (match_operand:S256F 0 "register_operand" "")
        (unspec:S256F [(match_operand:S256F 1 "register_operand" "")
                       (match_operand:S256F 2 "register_operand" "")
                       (match_operand:S256F 3 "register_operand" "")
                       (match_operand 4 "" "")] UNSPEC_FFMS))]
  "HAVE_LVX_FMS_<HALF>_<HALF>_<HALF> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)
                        (subreg:<HALF> (match_dup 3) 0)
                        (match_dup 4)] UNSPEC_FFMS))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)
                        (subreg:<HALF> (match_dup 3) 16)
                        (match_dup 4)] UNSPEC_FFMS))]
  ""
)

(define_insn_and_split "lvx_ffmsdq"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "r")
                      (match_operand:V4DF 2 "register_operand" "r")
                      (match_operand:V4DF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFMS))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DF (match_dup 1) 0)
                    (subreg:DF (match_dup 2) 0)
                    (subreg:DF (match_dup 3) 0)
                    (match_dup 4)] UNSPEC_FFMS))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DF (match_dup 1) 8)
                    (subreg:DF (match_dup 2) 8)
                    (subreg:DF (match_dup 3) 8)
                    (match_dup 4)] UNSPEC_FFMS))
   (set (subreg:DF (match_dup 0) 16)
        (unspec:DF [(subreg:DF (match_dup 1) 16)
                    (subreg:DF (match_dup 2) 16)
                    (subreg:DF (match_dup 3) 16)
                    (match_dup 4)] UNSPEC_FFMS))
   (set (subreg:DF (match_dup 0) 24)
        (unspec:DF [(subreg:DF (match_dup 1) 24)
                    (subreg:DF (match_dup 2) 24)
                    (subreg:DF (match_dup 3) 24)
                    (match_dup 4)] UNSPEC_FFMS))]
  ""
  [(set_attr "type" "madd_fp4")]
)


;; FFMS*C

(define_expand "lvx_ffmswc"
  [(set (match_operand:V2SF 0 "register_operand" "")
        (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "")
                      (match_operand:V2SF 2 "register_operand" "")
                      (match_operand:V2SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFMSC))]
  ""
  {
    if (!HAVE_LVX_CPLX_FMS_V2SF_V2SF_V2SF)
      {
        rtx product = gen_reg_rtx (V2SFmode);
        emit_insn (gen_lvx_fmulwc (product, operands[2], operands[1], operands[4]));
        emit_insn (gen_lvx_fsbfwc (operands[0], product, operands[3], operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffmswc_2"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")
                      (match_operand:V2SF 2 "register_operand" "r")
                      (match_operand:V2SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFMSC))]
  "HAVE_LVX_CPLX_FMS_V2SF_V2SF_V2SF"
  "ffmswc%4 %0 = %1, %2"
  [(set_attr "type" "dmda_fp4")]
)

(define_expand "lvx_ffmswcp"
  [(set (match_operand:V4SF 0 "register_operand" "")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "")
                      (match_operand:V4SF 2 "register_operand" "")
                      (match_operand:V4SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFMSC))]
  ""
  {
    if (!HAVE_LVX_CPLX_FMS_V4SF_V4SF_V4SF)
      {
        for (int i = 0; i < 2; i++)
          {
            rtx product = gen_reg_rtx (V2SFmode);
            rtx opnd0 = gen_rtx_SUBREG (V2SFmode, operands[0], i*8);
            rtx opnd1 = gen_rtx_SUBREG (V2SFmode, operands[1], i*8);
            rtx opnd2 = gen_rtx_SUBREG (V2SFmode, operands[2], i*8);
            rtx opnd3 = gen_rtx_SUBREG (V2SFmode, operands[3], i*8);
            emit_insn (gen_lvx_fmulwc (product, opnd2, opnd1, operands[4]));
            emit_insn (gen_lvx_fsbfwc (opnd0, product, opnd3, operands[4]));
          }
        DONE;
      }
  }
)

(define_insn "lvx_ffmswcp_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand:V4SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFMSC))]
  "HAVE_LVX_CPLX_FMS_V4SF_V4SF_V4SF"
  "ffmswcp%4 %0 = %1, %2"
  [(set_attr "type" "dmda_fp4")]
)

(define_expand "lvx_ffmsdc"
  [(set (match_operand:V2DF 0 "register_operand")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand")
                      (match_operand:V2DF 2 "register_operand")
                      (match_operand:V2DF 3 "register_operand")
                      (match_operand 4 "" "")] UNSPEC_FFMSC))]
  ""
  {
    rtx product = gen_reg_rtx (V2DFmode);
    emit_insn (gen_lvx_fmuldc (product, operands[2], operands[1], operands[4]));
    emit_insn (gen_lvx_fsbfdp (operands[0], product, operands[3], operands[4]));
    DONE;
  }
)

(define_expand "lvx_ffmswcq"
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand:V8SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFMSC))]
  ""
  {
    if (!HAVE_LVX_CPLX_FMS_V4SF_V4SF_V4SF)
      {
        for (int i = 0; i < 4; i++)
          {
            rtx product = gen_reg_rtx (V2SFmode);
            rtx opnd0 = gen_rtx_SUBREG (V2SFmode, operands[0], i*8);
            rtx opnd1 = gen_rtx_SUBREG (V2SFmode, operands[1], i*8);
            rtx opnd2 = gen_rtx_SUBREG (V2SFmode, operands[2], i*8);
            rtx opnd3 = gen_rtx_SUBREG (V2SFmode, operands[3], i*8);
            emit_insn (gen_lvx_fmulwc (product, opnd2, opnd1, operands[4]));
            emit_insn (gen_lvx_fsbfwc (opnd0, product, opnd3, operands[4]));
          }
        DONE;
      }
  }
)

(define_insn_and_split "lvx_ffmswcq_2"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand:V8SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFMSC))]
  "HAVE_LVX_CPLX_FMS_V4SF_V4SF_V4SF"
  "#"
  "HAVE_LVX_CPLX_FMS_V4SF_V4SF_V4SF && reload_completed"
  [(set (subreg:V4SF (match_dup 0) 0)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 0)
                      (subreg:V4SF (match_dup 2) 0)
                      (subreg:V4SF (match_dup 3) 0)
                      (match_dup 4)] UNSPEC_FFMSC))
   (set (subreg:V4SF (match_dup 0) 16)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 16)
                      (subreg:V4SF (match_dup 2) 16)
                      (subreg:V4SF (match_dup 3) 16)
                      (match_dup 4)] UNSPEC_FFMSC))]
  ""
  [(set_attr "type" "dmda_fp4")]
)

(define_expand "lvx_ffmsdcp"
  [(set (match_operand:V4DF 0 "register_operand")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand")
                      (match_operand:V4DF 2 "register_operand")
                      (match_operand:V4DF 3 "register_operand")
                      (match_operand 4 "" "")] UNSPEC_FFMSC))]
  ""
  {
    rtx product = gen_reg_rtx (V4DFmode);
    emit_insn (gen_lvx_fmuldcp (product, operands[2], operands[1], operands[4]));
    emit_insn (gen_lvx_fsbfdcp (operands[0], product, operands[3], operands[4]));
    DONE;
  }
)

(define_expand "lvx_ffmsdcq"
  [(set (match_operand:V8DF 0 "register_operand")
        (unspec:V8DF [(match_operand:V8DF 1 "register_operand")
                      (match_operand:V8DF 2 "register_operand")
                      (match_operand:V8DF 3 "register_operand")
                      (match_operand 4 "" "")] UNSPEC_FFMSC))]
  ""
  {
    rtx product = gen_reg_rtx (V8DFmode);
    emit_insn (gen_lvx_fmuldcq (product, operands[2], operands[1], operands[4]));
    emit_insn (gen_lvx_fsbfdcq (operands[0], product, operands[3], operands[4]));
    DONE;
  }
)


;; FFMSX*

(define_insn "lvx_ffmsxhw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:HF 1 "register_operand" "r")
                    (match_operand:HF 2 "register_operand" "r")
                    (match_operand:SF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFMSX))]
  "HAVE_LVX_FMS_SF_HF_HF"
  "ffmshw%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp3")]
)

(define_insn "lvx_ffmsxwd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand:SF 2 "register_operand" "r")
                    (match_operand:DF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFMSX))]
  "HAVE_LVX_FMS_DF_SF_SF"
  "ffmswd%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_insn "lvx_ffmsx<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (unspec:<WIDE> [(match_operand:S64F 1 "register_operand" "r")
                        (match_operand:S64F 2 "register_operand" "r")
                        (match_operand:<WIDE> 3 "register_operand" "0")
                        (match_operand 4 "" "")] UNSPEC_FFMSX))]
  "HAVE_LVX_FMS_<WIDE>_<S64F:MODE>_<S64F:MODE>"
  "ffms<widenx>%4 %0 = %1, %2"
  [(set (attr "type")
     (if_then_else (match_operand 1 "float16_inner_mode") (const_string "madd_fp3") (const_string "madd_fp4")))]
)

(define_insn_and_split "lvx_ffmsx<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=&r")
        (unspec:<WIDE> [(match_operand:S128F 1 "register_operand" "r")
                        (match_operand:S128F 2 "register_operand" "r")
                        (match_operand:<WIDE> 3 "register_operand" "0")
                        (match_operand 4 "" "")] UNSPEC_FFMSX))]
  "HAVE_LVX_FMS_<HWIDE>_<CHUNK>_<CHUNK>"
  "#"
  "reload_completed"
  [(set (subreg:<HWIDE> (match_dup 0) 0)
        (unspec:<HWIDE> [(subreg:<CHUNK> (match_dup 1) 0)
                         (subreg:<CHUNK> (match_dup 2) 0)
                         (subreg:<HWIDE> (match_dup 3) 0)
                         (match_dup 4)] UNSPEC_FFMSX))
   (set (subreg:<HWIDE> (match_dup 0) 16)
        (unspec:<HWIDE> [(subreg:<CHUNK> (match_dup 1) 8)
                         (subreg:<CHUNK> (match_dup 2) 8)
                         (subreg:<HWIDE> (match_dup 3) 16)
                         (match_dup 4)] UNSPEC_FFMSX))]
  ""
)


;; FMM*

(define_insn "lvx_fmm212w"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V2SF 1 "register_operand" "r")
                      (match_operand:V2SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMM))]
  ""
  "fmm212w%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_expand "lvx_fmm222w"
  [(set (match_operand:V4SF 0 "register_operand" "")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "")
                      (match_operand:V4SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FMM))]
  ""
  {
    if (!HAVE_LVX_MULT_M2x2_SF_M2x2_SF_M2x2_SF)
      {
        rtx modifiers = operands[3];
        const char *xstr = XSTR (modifiers, 0);
        bool matlayout = xstr && xstr[0] == '.' &&
          (xstr[1] == 'n' || xstr[1] == 't') &&
          (xstr[2] == 'n' || xstr[2] == 't');
        if (matlayout)
          {
            if (xstr[1] == 'n')
              {
                rtx operand_1 = gen_reg_rtx (V4SFmode);
                emit_insn (gen_lvx_fmt22w (operand_1, operands[1]));
                operands[1] = operand_1;
              }
            if (xstr[2] == 't')
              {
                rtx operand_2 = gen_reg_rtx (V4SFmode);
                emit_insn (gen_lvx_fmt22w (operand_2, operands[2]));
                operands[2] = operand_2;
              }
            modifiers = gen_rtx_CONST_STRING (VOIDmode, xstr + 3);
          }
        else
          {
            rtx operand_1 = gen_reg_rtx (V4SFmode);
            emit_insn (gen_lvx_fmt22w (operand_1, operands[1]));
            operands[1] = operand_1;
          }
        rtx accum = gen_reg_rtx (V4SFmode);
        rtx opnd1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 0);
        rtx opnd2_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 0);
        emit_insn (gen_lvx_fmm212w (accum, opnd1_0, opnd2_0, modifiers));
        rtx opnd1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8);
        rtx opnd2_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 8);
        emit_insn (gen_lvx_fmma212w (operands[0], opnd1_1, opnd2_1, accum, modifiers));
        DONE;
      }
  }
)

(define_insn "lvx_fmm222w_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMM))]
  "HAVE_LVX_MULT_M2x2_SF_M2x2_SF_M2x2_SF"
  "fmm222w%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)


;; FMMA*

(define_insn "lvx_fmma212w"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V2SF 1 "register_operand" "r")
                      (match_operand:V2SF 2 "register_operand" "r")
                      (match_operand:V4SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FMMA))]
  ""
  "fmma212w%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_expand "lvx_fmma222w"
  [(set (match_operand:V4SF 0 "register_operand" "")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "")
                      (match_operand:V4SF 2 "register_operand" "")
                      (match_operand:V4SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FMMA))]
  ""
  {
    if (!HAVE_LVX_MA_M2x2_SF_M2x2_SF_M2x2_SF)
      {
        rtx modifiers = operands[4];
        const char *xstr = XSTR (modifiers, 0);
        bool matlayout = xstr && xstr[0] == '.' &&
          (xstr[1] == 'n' || xstr[1] == 't') &&
          (xstr[2] == 'n' || xstr[2] == 't');
        if (matlayout)
          {
            if (xstr[1] == 'n')
              {
                rtx operand_1 = gen_reg_rtx (V4SFmode);
                emit_insn (gen_lvx_fmt22w (operand_1, operands[1]));
                operands[1] = operand_1;
              }
            if (xstr[2] == 't')
              {
                rtx operand_2 = gen_reg_rtx (V4SFmode);
                emit_insn (gen_lvx_fmt22w (operand_2, operands[2]));
                operands[2] = operand_2;
              }
            modifiers = gen_rtx_CONST_STRING (VOIDmode, xstr + 3);
          }
        else
          {
            rtx operand_1 = gen_reg_rtx (V4SFmode);
            emit_insn (gen_lvx_fmt22w (operand_1, operands[1]));
            operands[1] = operand_1;
          }
        rtx accum = gen_reg_rtx (V4SFmode);
        rtx opnd1_0 = simplify_gen_subreg (V2SFmode, operands[1], V4SFmode, 0);
        rtx opnd2_0 = simplify_gen_subreg (V2SFmode, operands[2], V4SFmode, 0);
        emit_insn (gen_lvx_fmma212w (accum, opnd1_0, opnd2_0, operands[3], modifiers));
        rtx opnd1_1 = simplify_gen_subreg (V2SFmode, operands[1], V4SFmode, 8);
        rtx opnd2_1 = simplify_gen_subreg (V2SFmode, operands[2], V4SFmode, 8);
        emit_insn (gen_lvx_fmma212w (operands[0], opnd1_1, opnd2_1, accum, modifiers));
        DONE;
      }
  }
)

(define_insn "lvx_fmma222w_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand:V4SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FMMA))]
  "HAVE_LVX_MA_M2x2_SF_M2x2_SF_M2x2_SF"
  "fmma222w%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)


;; FMMS*

(define_insn "lvx_fmms212w"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V2SF 1 "register_operand" "r")
                      (match_operand:V2SF 2 "register_operand" "r")
                      (match_operand:V4SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FMMS))]
  ""
  "fmms212w%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_expand "lvx_fmms222w"
  [(set (match_operand:V4SF 0 "register_operand" "")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "")
                      (match_operand:V4SF 2 "register_operand" "")
                      (match_operand:V4SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FMMS))]
  ""
  {
    if (!HAVE_LVX_MS_M2x2_SF_M2x2_SF_M2x2_SF)
      {
        rtx modifiers = operands[4];
        const char *xstr = XSTR (modifiers, 0);
        bool matlayout = xstr && xstr[0] == '.' &&
          (xstr[1] == 'n' || xstr[1] == 't') &&
          (xstr[2] == 'n' || xstr[2] == 't');
        if (matlayout)
          {
            if (xstr[1] == 'n')
              {
                rtx operand_1 = gen_reg_rtx (V4SFmode);
                emit_insn (gen_lvx_fmt22w (operand_1, operands[1]));
                operands[1] = operand_1;
              }
            if (xstr[2] == 't')
              {
                rtx operand_2 = gen_reg_rtx (V4SFmode);
                emit_insn (gen_lvx_fmt22w (operand_2, operands[2]));
                operands[2] = operand_2;
              }
            modifiers = gen_rtx_CONST_STRING (VOIDmode, xstr + 3);
          }
        else
          {
            rtx operand_1 = gen_reg_rtx (V4SFmode);
            emit_insn (gen_lvx_fmt22w (operand_1, operands[1]));
            operands[1] = operand_1;
          }
        rtx accum = gen_reg_rtx (V4SFmode);
        rtx opnd1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 0);
        rtx opnd2_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 0);
        emit_insn (gen_lvx_fmms212w (accum, opnd1_0, opnd2_0, operands[3], modifiers));
        rtx opnd1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8);
        rtx opnd2_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 8);
        emit_insn (gen_lvx_fmms212w (operands[0], opnd1_1, opnd2_1, accum, modifiers));
        DONE;
      }
  }
)

(define_insn "lvx_fmms222w_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand:V4SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FMMS))]
  "HAVE_LVX_MS_M2x2_SF_M2x2_SF_M2x2_SF"
  "fmms222w%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)


;; FFDMA*

(define_expand "lvx_ffdmaw"
  [(set (match_operand:SF 0 "register_operand" "")
        (unspec:SF [(match_operand:V2SF 1 "register_operand" "")
                    (match_operand:V2SF 2 "register_operand" "")
                    (match_operand 3 "" "")] UNSPEC_FFDMA))]
  ""
  {
    if (!HAVE_LVX_FDMA_SF_V2SF_V2SF)
      {
        emit_insn (gen_lvx_fdot2w (operands[0], operands[1], operands[2], operands[3]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmaw_2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:V2SF 1 "register_operand" "r")
                    (match_operand:V2SF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FFDMA))]
  "HAVE_LVX_FDMA_SF_V2SF_V2SF"
  "ffdmaw%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_expand "lvx_ffdmawp"
  [(set (match_operand:V2SF 0 "register_operand" "")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "")
                      (match_operand:V4SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FFDMA))]
  ""
  {
    if (!HAVE_LVX_FDMA_V2SF_V4SF_V4SF)
      {
        rtx accum = gen_reg_rtx (V2SFmode);
        rtx opnd1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 0);
        rtx opnd2_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 0);
        emit_insn (gen_lvx_fmulwp (accum, opnd1_0, opnd2_0, operands[3]));
        rtx opnd1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8);
        rtx opnd2_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 8);
        emit_insn (gen_lvx_ffmawp (operands[0], opnd1_1, opnd2_1, accum, operands[3]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmawp_2"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FFDMA))]
  "HAVE_LVX_FDMA_V2SF_V4SF_V4SF"
  "ffdmawp%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_expand "lvx_ffdmawq"
  [(set (match_operand:V4SF 0 "register_operand" "")
        (unspec:V4SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FFDMA))]
  ""
  {
    if (!HAVE_LVX_FDMA_V4SF_V8SF_V8SF)
      {
        rtx accum = gen_reg_rtx (V4SFmode);
        rtx opnd1_0 = gen_rtx_SUBREG (V4SFmode, operands[1], 0);
        rtx opnd2_0 = gen_rtx_SUBREG (V4SFmode, operands[2], 0);
        emit_insn (gen_lvx_fmulwq (accum, opnd1_0, opnd2_0, operands[3]));
        rtx opnd0_0 = gen_rtx_SUBREG (V2SFmode, operands[0], 0);
        rtx opnd0_1 = gen_rtx_SUBREG (V2SFmode, operands[0], 8);
        rtx opnd1_1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 16);
        rtx opnd1_1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 24);
        rtx opnd2_1_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 16);
        rtx opnd2_1_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 24);
        rtx accum_0 = gen_rtx_SUBREG (V2SFmode, accum, 0);
        rtx accum_1 = gen_rtx_SUBREG (V2SFmode, accum, 8);
        emit_insn (gen_lvx_ffmawp (opnd0_0, opnd1_1_0, opnd2_1_0, accum_0, operands[3]));
        emit_insn (gen_lvx_ffmawp (opnd0_1, opnd1_1_1, opnd2_1_1, accum_1, operands[3]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmawq_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FFDMA))]
  "HAVE_LVX_FDMA_V4SF_V8SF_V8SF"
  "ffdmawq%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)


;; FFDMS*

(define_expand "lvx_ffdmsw"
  [(set (match_operand:SF 0 "register_operand" "")
        (unspec:SF [(match_operand:V2SF 1 "register_operand" "")
                    (match_operand:V2SF 2 "register_operand" "")
                    (match_operand 3 "" "")] UNSPEC_FFDMS))]
  ""
  {
    if (!HAVE_LVX_FDMS_SF_V2SF_V2SF)
      {
        rtx fconj = gen_reg_rtx (V2SFmode);
        emit_insn (gen_lvx_fconjwc (fconj, operands[1]));
        emit_insn (gen_lvx_ffdmaw (operands[0], fconj, operands[2], operands[3]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmsw_2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:V2SF 1 "register_operand" "r")
                    (match_operand:V2SF 2 "register_operand" "r")
                    (match_operand 3 "" "")] UNSPEC_FFDMS))]
  "HAVE_LVX_FDMS_SF_V2SF_V2SF"
  "ffdmsw%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_expand "lvx_ffdmswp"
  [(set (match_operand:V2SF 0 "register_operand" "")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "")
                      (match_operand:V4SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FFDMS))]
  ""
  {
    if (!HAVE_LVX_FDMS_V2SF_V4SF_V4SF)
      {
        rtx accum = gen_reg_rtx (V2SFmode);
        rtx opnd1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 0);
        rtx opnd2_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 0);
        emit_insn (gen_lvx_fmulwp (accum, opnd1_0, opnd2_0, operands[3]));
        rtx opnd1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8);
        rtx opnd2_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 8);
        emit_insn (gen_lvx_ffmswp (operands[0], opnd1_1, opnd2_1, accum, operands[3]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmswp_2"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FFDMS))]
  "HAVE_LVX_FDMS_V2SF_V4SF_V4SF"
  "ffdmswp%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)

(define_expand "lvx_ffdmswq"
  [(set (match_operand:V4SF 0 "register_operand" "")
        (unspec:V4SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand 3 "" "")] UNSPEC_FFDMS))]
  ""
  {
    if (!HAVE_LVX_FDMS_V4SF_V8SF_V8SF)
      {
        rtx accum = gen_reg_rtx (V4SFmode);
        rtx opnd1_0 = gen_rtx_SUBREG (V4SFmode, operands[1], 0);
        rtx opnd2_0 = gen_rtx_SUBREG (V4SFmode, operands[2], 0);
        emit_insn (gen_lvx_fmulwq (accum, opnd1_0, opnd2_0, operands[3]));
        rtx opnd0_0 = gen_rtx_SUBREG (V2SFmode, operands[0], 0);
        rtx opnd0_1 = gen_rtx_SUBREG (V2SFmode, operands[0], 8);
        rtx opnd1_1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 16);
        rtx opnd1_1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 24);
        rtx opnd2_1_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 16);
        rtx opnd2_1_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 24);
        rtx accum_0 = gen_rtx_SUBREG (V2SFmode, accum, 0);
        rtx accum_1 = gen_rtx_SUBREG (V2SFmode, accum, 8);
        emit_insn (gen_lvx_ffmswp (opnd0_0, opnd1_1_0, opnd2_1_0, accum_0, operands[3]));
        emit_insn (gen_lvx_ffmswp (opnd0_1, opnd1_1_1, opnd2_1_1, accum_1, operands[3]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmswq_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FFDMS))]
  "HAVE_LVX_FDMS_V4SF_V8SF_V8SF"
  "ffdmswq%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
)


;; FFDMDA*

(define_expand "lvx_ffdmdaw"
  [(set (match_operand:SF 0 "register_operand" "")
        (unspec:SF [(match_operand:V2SF 1 "register_operand" "")
                    (match_operand:V2SF 2 "register_operand" "")
                    (match_operand:SF 3 "register_operand" "")
                    (match_operand 4 "" "")] UNSPEC_FFDMDA))]
  ""
  {
    if (!HAVE_LVX_FDMDA_SF_V2SF_V2SF)
      {
        rtx ffdma = gen_reg_rtx (SFmode);
        emit_insn (gen_lvx_ffdmaw (ffdma, operands[1], operands[2], operands[4]));
        emit_insn (gen_lvx_faddw (operands[0], ffdma, operands[3], operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmdaw_2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:V2SF 1 "register_operand" "r")
                    (match_operand:V2SF 2 "register_operand" "r")
                    (match_operand:SF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFDMDA))]
  "HAVE_LVX_FDMDA_SF_V2SF_V2SF"
  "ffdmdaw%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_expand "lvx_ffdmdawp"
  [(set (match_operand:V2SF 0 "register_operand" "")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "")
                      (match_operand:V4SF 2 "register_operand" "")
                      (match_operand:V2SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFDMDA))]
  ""
  {
    if (!HAVE_LVX_FDMDA_V2SF_V4SF_V4SF)
      {
        rtx accum = gen_reg_rtx (V2SFmode);
        rtx opnd1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 0);
        rtx opnd2_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 0);
        emit_insn (gen_lvx_ffmawp (accum, opnd1_0, opnd2_0, operands[3], operands[4]));
        rtx opnd1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8);
        rtx opnd2_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 8);
        emit_insn (gen_lvx_ffmawp (operands[0], opnd1_1, opnd2_1, accum, operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmdawp_2"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand:V2SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFDMDA))]
  "HAVE_LVX_FDMDA_V2SF_V4SF_V4SF"
  "ffdmdawp%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_expand "lvx_ffdmdawq"
  [(set (match_operand:V4SF 0 "register_operand" "")
        (unspec:V4SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand:V4SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFDMDA))]
  ""
  {
    if (!HAVE_LVX_FDMDA_V4SF_V8SF_V8SF)
      {
        rtx accum_0 = gen_reg_rtx (V2SFmode);
        rtx accum_1 = gen_reg_rtx (V2SFmode);
        rtx opnd1_0_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 0);
        rtx opnd1_0_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8);
        rtx opnd2_0_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 0);
        rtx opnd2_0_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 8);
        rtx opnd3_0 = gen_rtx_SUBREG (V2SFmode, operands[3], 0);
        rtx opnd3_1 = gen_rtx_SUBREG (V2SFmode, operands[3], 8);
        emit_insn (gen_lvx_ffmawp (accum_0, opnd1_0_0, opnd2_0_0, opnd3_0, operands[4]));
        emit_insn (gen_lvx_ffmawp (accum_1, opnd1_0_1, opnd2_0_1, opnd3_1, operands[4]));
        rtx opnd0_0 = gen_rtx_SUBREG (V2SFmode, operands[0], 0);
        rtx opnd0_1 = gen_rtx_SUBREG (V2SFmode, operands[0], 8);
        rtx opnd1_1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 16);
        rtx opnd1_1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 24);
        rtx opnd2_1_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 16);
        rtx opnd2_1_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 24);
        emit_insn (gen_lvx_ffmawp (opnd0_0, opnd1_1_0, opnd2_1_0, accum_0, operands[4]));
        emit_insn (gen_lvx_ffmawp (opnd0_1, opnd1_1_1, opnd2_1_1, accum_1, operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmdawq_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand:V4SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFDMDA))]
  "HAVE_LVX_FDMDA_V4SF_V8SF_V8SF"
  "ffdmdawq%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)


;; FFDMSA*

(define_expand "lvx_ffdmsaw"
  [(set (match_operand:SF 0 "register_operand" "")
        (unspec:SF [(match_operand:V2SF 1 "register_operand" "")
                    (match_operand:V2SF 2 "register_operand" "")
                    (match_operand:SF 3 "register_operand" "")
                    (match_operand 4 "" "")] UNSPEC_FFDMSA))]
  ""
  {
    if (!HAVE_LVX_FDMSA_SF_V2SF_V2SF)
      {
        rtx ffdmaw = gen_reg_rtx (SFmode);
        rtx fconj = gen_reg_rtx (V2SFmode);
        emit_insn (gen_lvx_fconjwc (fconj, operands[1]));
        emit_insn (gen_lvx_ffdmaw (ffdmaw, fconj, operands[2], operands[4]));
        emit_insn (gen_lvx_fsbfw (operands[0], ffdmaw, operands[3], operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmsaw_2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:V2SF 1 "register_operand" "r")
                    (match_operand:V2SF 2 "register_operand" "r")
                    (match_operand:SF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFDMSA))]
  "HAVE_LVX_FDMSA_SF_V2SF_V2SF"
  "ffdmsaw%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_expand "lvx_ffdmsawp"
  [(set (match_operand:V2SF 0 "register_operand" "")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "")
                      (match_operand:V4SF 2 "register_operand" "")
                      (match_operand:V2SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFDMSA))]
  ""
  {
    if (!HAVE_LVX_FDMSA_V2SF_V4SF_V4SF)
      {
        rtx accum = gen_reg_rtx (V2SFmode);
        rtx opnd1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 0);
        rtx opnd2_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 0);
        emit_insn (gen_lvx_ffmswp (accum, opnd1_0, opnd2_0, operands[3], operands[4]));
        rtx opnd1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8);
        rtx opnd2_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 8);
        emit_insn (gen_lvx_ffmawp (operands[0], opnd1_1, opnd2_1, accum, operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmsawp_2"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand:V2SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFDMSA))]
  "HAVE_LVX_FDMSA_V2SF_V4SF_V4SF"
  "ffdmsawp%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_expand "lvx_ffdmsawq"
  [(set (match_operand:V4SF 0 "register_operand" "")
        (unspec:V4SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand:V4SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFDMSA))]
  ""
  {
    if (!HAVE_LVX_FDMSA_V4SF_V8SF_V8SF)
      {
        rtx accum_0 = gen_reg_rtx (V2SFmode);
        rtx accum_1 = gen_reg_rtx (V2SFmode);
        rtx opnd1_0_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 0);
        rtx opnd1_0_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8);
        rtx opnd2_0_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 0);
        rtx opnd2_0_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 8);
        rtx opnd3_0 = gen_rtx_SUBREG (V2SFmode, operands[3], 0);
        rtx opnd3_1 = gen_rtx_SUBREG (V2SFmode, operands[3], 8);
        emit_insn (gen_lvx_ffmswp (accum_0, opnd1_0_0, opnd2_0_0, opnd3_0, operands[4]));
        emit_insn (gen_lvx_ffmswp (accum_1, opnd1_0_1, opnd2_0_1, opnd3_1, operands[4]));
        rtx opnd0_0 = gen_rtx_SUBREG (V2SFmode, operands[0], 0);
        rtx opnd0_1 = gen_rtx_SUBREG (V2SFmode, operands[0], 8);
        rtx opnd1_1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 16);
        rtx opnd1_1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 24);
        rtx opnd2_1_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 16);
        rtx opnd2_1_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 24);
        emit_insn (gen_lvx_ffmawp (opnd0_0, opnd1_1_0, opnd2_1_0, accum_0, operands[4]));
        emit_insn (gen_lvx_ffmawp (opnd0_1, opnd1_1_1, opnd2_1_1, accum_1, operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmsawq_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand:V4SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFDMSA))]
  "HAVE_LVX_FDMSA_V4SF_V8SF_V8SF"
  "ffdmsawq%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)


;; FFDMDS*

(define_expand "lvx_ffdmdsw"
  [(set (match_operand:SF 0 "register_operand" "")
        (unspec:SF [(match_operand:V2SF 1 "register_operand" "")
                    (match_operand:V2SF 2 "register_operand" "")
                    (match_operand:SF 3 "register_operand" "")
                    (match_operand 4 "" "")] UNSPEC_FFDMDS))]
  ""
  {
    if (!HAVE_LVX_FDMDS_SF_V2SF_V2SF)
      {
        rtx ffdma = gen_reg_rtx (SFmode);
        emit_insn (gen_lvx_ffdmaw (ffdma, operands[1], operands[2], operands[4]));
        emit_insn (gen_lvx_fsbfw (operands[0], ffdma, operands[3], operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmdsw_2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:V2SF 1 "register_operand" "r")
                    (match_operand:V2SF 2 "register_operand" "r")
                    (match_operand:SF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFDMDS))]
  "HAVE_LVX_FDMDS_SF_V2SF_V2SF"
  "ffdmdsw%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_expand "lvx_ffdmdswp"
  [(set (match_operand:V2SF 0 "register_operand" "")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "")
                      (match_operand:V4SF 2 "register_operand" "")
                      (match_operand:V2SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFDMDS))]
  ""
  {
    if (!HAVE_LVX_FDMDS_V2SF_V4SF_V4SF)
      {
        rtx accum = gen_reg_rtx (V2SFmode);
        rtx opnd1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 0);
        rtx opnd2_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 0);
        emit_insn (gen_lvx_ffmswp (accum, opnd1_0, opnd2_0, operands[3], operands[4]));
        rtx opnd1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8);
        rtx opnd2_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 8);
        emit_insn (gen_lvx_ffmswp (operands[0], opnd1_1, opnd2_1, accum, operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmdswp_2"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand:V2SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFDMDS))]
  "HAVE_LVX_FDMDS_V2SF_V4SF_V4SF"
  "ffdmdswp%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_expand "lvx_ffdmdswq"
  [(set (match_operand:V4SF 0 "register_operand" "")
        (unspec:V4SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand:V4SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFDMDS))]
  ""
  {
    if (!HAVE_LVX_FDMDS_V4SF_V8SF_V8SF)
      {
        rtx accum_0 = gen_reg_rtx (V2SFmode);
        rtx accum_1 = gen_reg_rtx (V2SFmode);
        rtx opnd1_0_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 0);
        rtx opnd1_0_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8);
        rtx opnd2_0_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 0);
        rtx opnd2_0_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 8);
        rtx opnd3_0 = gen_rtx_SUBREG (V2SFmode, operands[3], 0);
        rtx opnd3_1 = gen_rtx_SUBREG (V2SFmode, operands[3], 8);
        emit_insn (gen_lvx_ffmswp (accum_0, opnd1_0_0, opnd2_0_0, opnd3_0, operands[4]));
        emit_insn (gen_lvx_ffmswp (accum_1, opnd1_0_1, opnd2_0_1, opnd3_1, operands[4]));
        rtx opnd0_0 = gen_rtx_SUBREG (V2SFmode, operands[0], 0);
        rtx opnd0_1 = gen_rtx_SUBREG (V2SFmode, operands[0], 8);
        rtx opnd1_1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 16);
        rtx opnd1_1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 24);
        rtx opnd2_1_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 16);
        rtx opnd2_1_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 24);
        emit_insn (gen_lvx_ffmswp (opnd0_0, opnd1_1_0, opnd2_1_0, accum_0, operands[4]));
        emit_insn (gen_lvx_ffmswp (opnd0_1, opnd1_1_1, opnd2_1_1, accum_1, operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmdswq_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand:V4SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFDMDS))]
  "HAVE_LVX_FDMDS_V4SF_V8SF_V8SF"
  "ffdmdswq%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)


;; FFDMAS*

(define_expand "lvx_ffdmasw"
  [(set (match_operand:SF 0 "register_operand" "")
        (unspec:SF [(match_operand:V2SF 1 "register_operand" "")
                    (match_operand:V2SF 2 "register_operand" "")
                    (match_operand:SF 3 "register_operand" "")
                    (match_operand 4 "" "")] UNSPEC_FFDMAS))]
  ""
  {
    if (!HAVE_LVX_FDMAS_SF_V2SF_V2SF)
      {
        rtx ffdmaw = gen_reg_rtx (SFmode);
        rtx fconj = gen_reg_rtx (V2SFmode);
        emit_insn (gen_lvx_fconjwc (fconj, operands[1]));
        emit_insn (gen_lvx_ffdmaw (ffdmaw, fconj, operands[2], operands[4]));
        emit_insn (gen_lvx_faddw (operands[0], ffdmaw, operands[3], operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmasw_2"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:V2SF 1 "register_operand" "r")
                    (match_operand:V2SF 2 "register_operand" "r")
                    (match_operand:SF 3 "register_operand" "0")
                    (match_operand 4 "" "")] UNSPEC_FFDMAS))]
  "HAVE_LVX_FDMAS_SF_V2SF_V2SF"
  "ffdmasw%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_expand "lvx_ffdmaswp"
  [(set (match_operand:V2SF 0 "register_operand" "")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "")
                      (match_operand:V4SF 2 "register_operand" "")
                      (match_operand:V2SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFDMAS))]
  ""
  {
    if (!HAVE_LVX_FDMAS_V2SF_V4SF_V4SF)
      {
        rtx accum = gen_reg_rtx (V2SFmode);
        rtx opnd1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 0);
        rtx opnd2_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 0);
        emit_insn (gen_lvx_ffmawp (accum, opnd1_0, opnd2_0, operands[3], operands[4]));
        rtx opnd1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8);
        rtx opnd2_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 8);
        emit_insn (gen_lvx_ffmswp (operands[0], opnd1_1, opnd2_1, accum, operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmaswp_2"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand:V2SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFDMAS))]
  "HAVE_LVX_FDMAS_V2SF_V4SF_V4SF"
  "ffdmaswp%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)

(define_expand "lvx_ffdmaswq"
  [(set (match_operand:V4SF 0 "register_operand" "")
        (unspec:V4SF [(match_operand:V8SF 1 "register_operand" "")
                      (match_operand:V8SF 2 "register_operand" "")
                      (match_operand:V4SF 3 "register_operand" "")
                      (match_operand 4 "" "")] UNSPEC_FFDMAS))]
  ""
  {
    if (!HAVE_LVX_FDMAS_V4SF_V8SF_V8SF)
      {
        rtx accum_0 = gen_reg_rtx (V2SFmode);
        rtx accum_1 = gen_reg_rtx (V2SFmode);
        rtx opnd1_0_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 0);
        rtx opnd1_0_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8);
        rtx opnd2_0_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 0);
        rtx opnd2_0_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 8);
        rtx opnd3_0 = gen_rtx_SUBREG (V2SFmode, operands[3], 0);
        rtx opnd3_1 = gen_rtx_SUBREG (V2SFmode, operands[3], 8);
        emit_insn (gen_lvx_ffmawp (accum_0, opnd1_0_0, opnd2_0_0, opnd3_0, operands[4]));
        emit_insn (gen_lvx_ffmawp (accum_1, opnd1_0_1, opnd2_0_1, opnd3_1, operands[4]));
        rtx opnd0_0 = gen_rtx_SUBREG (V2SFmode, operands[0], 0);
        rtx opnd0_1 = gen_rtx_SUBREG (V2SFmode, operands[0], 8);
        rtx opnd1_1_0 = gen_rtx_SUBREG (V2SFmode, operands[1], 16);
        rtx opnd1_1_1 = gen_rtx_SUBREG (V2SFmode, operands[1], 24);
        rtx opnd2_1_0 = gen_rtx_SUBREG (V2SFmode, operands[2], 16);
        rtx opnd2_1_1 = gen_rtx_SUBREG (V2SFmode, operands[2], 24);
        emit_insn (gen_lvx_ffmswp (opnd0_0, opnd1_1_0, opnd2_1_0, accum_0, operands[4]));
        emit_insn (gen_lvx_ffmswp (opnd0_1, opnd1_1_1, opnd2_1_1, accum_1, operands[4]));
        DONE;
      }
  }
)

(define_insn "lvx_ffdmaswq_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand:V4SF 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_FFDMAS))]
  "HAVE_LVX_FDMAS_V4SF_V8SF_V8SF"
  "ffdmaswq%4 %0 = %1, %2"
  [(set_attr "type" "madd_fp4")]
)


;; FLOAT*

(define_insn "lvx_floatw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SI 1 "register_operand" "r")
                    (match_operand 2 "sixbits_unsigned_operand" "i")
                    (match_operand 3 "" "")] UNSPEC_FLOAT))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "floatw%3 %0 = %1, %2";
    return "floatw%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)

(define_insn "lvx_floatd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:DI 1 "register_operand" "r")
                    (match_operand 2 "sixbits_unsigned_operand" "i")
                    (match_operand 3 "" "")] UNSPEC_FLOAT))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "floatd%3 %0 = %1, %2";
    return "floatd%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)

(define_insn "lvx_floatwp"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V2SI 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FLOAT))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "floatwp%3 %0 = %1, %2";
    return "floatwp%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)

(define_insn_and_split "lvx_floatwq"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SI 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FLOAT))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SI (match_dup 1) 0)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FLOAT))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SI (match_dup 1) 8)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FLOAT))]
  ""
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_floatdp"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V2DI 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FLOAT))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DI (match_dup 1) 0)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FLOAT))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DI (match_dup 1) 8)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FLOAT))]
  ""
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_floatwo"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (unspec:V8SF [(match_operand:V8SI 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FLOAT))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SI (match_dup 1) 0)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FLOAT))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SI (match_dup 1) 8)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FLOAT))
   (set (subreg:V2SF (match_dup 0) 16)
        (unspec:V2SF [(subreg:V2SI (match_dup 1) 16)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FLOAT))
   (set (subreg:V2SF (match_dup 0) 24)
        (unspec:V2SF [(subreg:V2SI (match_dup 1) 24)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FLOAT))]
  ""
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_floatdq"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (unspec:V4DF [(match_operand:V4DI 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FLOAT))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DI (match_dup 1) 0)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FLOAT))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DI (match_dup 1) 8)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FLOAT))
   (set (subreg:DF (match_dup 0) 16)
        (unspec:DF [(subreg:DI (match_dup 1) 16)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FLOAT))
   (set (subreg:DF (match_dup 0) 24)
        (unspec:DF [(subreg:DI (match_dup 1) 24)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FLOAT))]
  ""
  [(set_attr "type" "mult_fp4")]
)


;; FLOATU*

(define_insn "lvx_floatuw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SI 1 "register_operand" "r")
                    (match_operand 2 "sixbits_unsigned_operand" "i")
                    (match_operand 3 "" "")] UNSPEC_FLOATU))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "floatuw%3 %0 = %1, %2";
    return "floatuw%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)

(define_insn "lvx_floatud"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:DI 1 "register_operand" "r")
                    (match_operand 2 "sixbits_unsigned_operand" "i")
                    (match_operand 3 "" "")] UNSPEC_FLOATU))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "floatud%3 %0 = %1, %2";
    return "floatud%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)

(define_insn "lvx_floatuwp"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V2SI 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FLOATU))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "floatuwp%3 %0 = %1, %2";
    return "floatuwp%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)

(define_insn_and_split "lvx_floatuwq"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SI 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FLOATU))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SI (match_dup 1) 0)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FLOATU))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SI (match_dup 1) 8)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FLOATU))]
  ""
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_floatudp"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V2DI 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FLOATU))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DI (match_dup 1) 0)
                    (match_dup 2)
                    (match_operand 3 "" "")] UNSPEC_FLOATU))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DI (match_dup 1) 8)
                    (match_dup 2)
                    (match_operand 3 "" "")] UNSPEC_FLOATU))]
  ""
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_floatuwo"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (unspec:V8SF [(match_operand:V8SI 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FLOATU))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V2SF (match_dup 0) 0)
        (unspec:V2SF [(subreg:V2SI (match_dup 1) 0)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FLOATU))
   (set (subreg:V2SF (match_dup 0) 8)
        (unspec:V2SF [(subreg:V2SI (match_dup 1) 8)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FLOATU))
   (set (subreg:V2SF (match_dup 0) 16)
        (unspec:V2SF [(subreg:V2SI (match_dup 1) 16)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FLOATU))
   (set (subreg:V2SF (match_dup 0) 24)
        (unspec:V2SF [(subreg:V2SI (match_dup 1) 24)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FLOATU))]
  ""
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_floatudq"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (unspec:V4DF [(match_operand:V4DI 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FLOATU))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (unspec:DF [(subreg:DI (match_dup 1) 0)
                    (match_dup 2)
                    (match_operand 3 "" "")] UNSPEC_FLOATU))
   (set (subreg:DF (match_dup 0) 8)
        (unspec:DF [(subreg:DI (match_dup 1) 8)
                    (match_dup 2)
                    (match_operand 3 "" "")] UNSPEC_FLOATU))
   (set (subreg:DF (match_dup 0) 16)
        (unspec:DF [(subreg:DI (match_dup 1) 16)
                    (match_dup 2)
                    (match_operand 3 "" "")] UNSPEC_FLOATU))
   (set (subreg:DF (match_dup 0) 24)
        (unspec:DF [(subreg:DI (match_dup 1) 24)
                    (match_dup 2)
                    (match_operand 3 "" "")] UNSPEC_FLOATU))]
  ""
  [(set_attr "type" "mult_fp4")]
)


;; FIXED*

(define_insn "lvx_fixedw"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (unspec:SI [(match_operand:SF 1 "register_operand" "r")
                    (match_operand 2 "sixbits_unsigned_operand" "i")
                    (match_operand 3 "" "")] UNSPEC_FIXED))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixedw%3 %0 = %1, %2";
    return "fixedw%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)
;; zero-extend version of lvx_fixedw
(define_insn "*lvx_fixedw_zext"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (zero_extend:DI (unspec:SI [(match_operand:SF 1 "register_operand" "r")
                                    (match_operand 2 "sixbits_unsigned_operand" "i")
                                    (match_operand 3 "" "")] UNSPEC_FIXED)))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixedw%3 %0 = %1, %2";
    return "fixedw%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)

(define_insn "lvx_fixedd"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unspec:DI [(match_operand:DF 1 "register_operand" "r")
                    (match_operand 2 "sixbits_unsigned_operand" "i")
                    (match_operand 3 "" "")] UNSPEC_FIXED))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixedd%3 %0 = %1, %2";
    return "fixedd%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)

(define_insn "lvx_fixedwp"
  [(set (match_operand:V2SI 0 "register_operand" "=r")
        (unspec:V2SI [(match_operand:V2SF 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FIXED))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixedwp%3 %0 = %1, %2";
    return "fixedwp%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)

(define_insn_and_split "lvx_fixedwq"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FIXED))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V2SI (match_dup 0) 0)
        (unspec:V2SI [(subreg:V2SF (match_dup 1) 0)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FIXED))
   (set (subreg:V2SI (match_dup 0) 8)
        (unspec:V2SI [(subreg:V2SF (match_dup 1) 8)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FIXED))]
  ""
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_fixeddp"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V2DF 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FIXED))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DI (match_dup 0) 0)
        (unspec:DI [(subreg:DF (match_dup 1) 0)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FIXED))
   (set (subreg:DI (match_dup 0) 8)
        (unspec:DI [(subreg:DF (match_dup 1) 8)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FIXED))]
  ""
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_fixedwo"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (unspec:V8SI [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FIXED))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V2SI (match_dup 0) 0)
        (unspec:V2SI [(subreg:V2SF (match_dup 1) 0)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FIXED))
   (set (subreg:V2SI (match_dup 0) 8)
        (unspec:V2SI [(subreg:V2SF (match_dup 1) 8)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FIXED))
   (set (subreg:V2SI (match_dup 0) 16)
        (unspec:V2SI [(subreg:V2SF (match_dup 1) 16)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FIXED))
   (set (subreg:V2SI (match_dup 0) 24)
        (unspec:V2SI [(subreg:V2SF (match_dup 1) 24)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FIXED))]
  ""
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_fixeddq"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (unspec:V4DI [(match_operand:V4DF 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FIXED))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DI (match_dup 0) 0)
        (unspec:DI [(subreg:DF (match_dup 1) 0)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FIXED))
   (set (subreg:DI (match_dup 0) 8)
        (unspec:DI [(subreg:DF (match_dup 1) 8)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FIXED))
   (set (subreg:DI (match_dup 0) 16)
        (unspec:DI [(subreg:DF (match_dup 1) 16)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FIXED))
   (set (subreg:DI (match_dup 0) 24)
        (unspec:DI [(subreg:DF (match_dup 1) 24)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FIXED))]
  ""
  [(set_attr "type" "mult_fp4")]
)


;; FIXEDU*

(define_insn "lvx_fixeduw"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (unspec:SI [(match_operand:SF 1 "register_operand" "r")
                    (match_operand 2 "sixbits_unsigned_operand" "i")
                    (match_operand 3 "" "")] UNSPEC_FIXEDU))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixeduw%3 %0 = %1, %2";
    return "fixeduw%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)
;; zero-extend version of lvx_fixeduw
(define_insn "*lvx_fixeduw_zext"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (zero_extend:DI (unspec:SI [(match_operand:SF 1 "register_operand" "r")
                                    (match_operand 2 "sixbits_unsigned_operand" "i")
                                    (match_operand 3 "" "")] UNSPEC_FIXEDU)))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixeduw%3 %0 = %1, %2";
    return "fixeduw%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)

(define_insn "lvx_fixedud"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unspec:DI [(match_operand:DF 1 "register_operand" "r")
                    (match_operand 2 "sixbits_unsigned_operand" "i")
                    (match_operand 3 "" "")] UNSPEC_FIXEDU))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixedud%3 %0 = %1, %2";
    return "fixedud%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)

(define_insn "lvx_fixeduwp"
  [(set (match_operand:V2SI 0 "register_operand" "=r")
        (unspec:V2SI [(match_operand:V2SF 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FIXEDU))]
  ""
  {
    if (HAVE_LVX_FP_CONV_WITH_SHIFT)
      return "fixeduwp%3 %0 = %1, %2";
    return "fixeduwp%3 %0 = %1";
  }
  [(set_attr "type" "conv_fp4")]
)

(define_insn_and_split "lvx_fixeduwq"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FIXEDU))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V2SI (match_dup 0) 0)
        (unspec:V2SI [(subreg:V2SF (match_dup 1) 0)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FIXEDU))
   (set (subreg:V2SI (match_dup 0) 8)
        (unspec:V2SI [(subreg:V2SF (match_dup 1) 8)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FIXEDU))]
  ""
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_fixedudp"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V2DF 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FIXEDU))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DI (match_dup 0) 0)
        (unspec:DI [(subreg:DF (match_dup 1) 0)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FIXEDU))
   (set (subreg:DI (match_dup 0) 8)
        (unspec:DI [(subreg:DF (match_dup 1) 8)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FIXEDU))]
  ""
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_fixeduwo"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (unspec:V8SI [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FIXEDU))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V2SI (match_dup 0) 0)
        (unspec:V2SI [(subreg:V2SF (match_dup 1) 0)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FIXEDU))
   (set (subreg:V2SI (match_dup 0) 8)
        (unspec:V2SI [(subreg:V2SF (match_dup 1) 8)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FIXEDU))
   (set (subreg:V2SI (match_dup 0) 16)
        (unspec:V2SI [(subreg:V2SF (match_dup 1) 16)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FIXEDU))
   (set (subreg:V2SI (match_dup 0) 24)
        (unspec:V2SI [(subreg:V2SF (match_dup 1) 24)
                      (match_dup 2)
                      (match_dup 3)] UNSPEC_FIXEDU))]
  ""
  [(set_attr "type" "mult_fp4")]
)

(define_insn_and_split "lvx_fixedudq"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (unspec:V4DI [(match_operand:V4DF 1 "register_operand" "r")
                      (match_operand 2 "sixbits_unsigned_operand" "i")
                      (match_operand 3 "" "")] UNSPEC_FIXEDU))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DI (match_dup 0) 0)
        (unspec:DI [(subreg:DF (match_dup 1) 0)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FIXEDU))
   (set (subreg:DI (match_dup 0) 8)
        (unspec:DI [(subreg:DF (match_dup 1) 8)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FIXEDU))
   (set (subreg:DI (match_dup 0) 16)
        (unspec:DI [(subreg:DF (match_dup 1) 16)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FIXEDU))
   (set (subreg:DI (match_dup 0) 24)
        (unspec:DI [(subreg:DF (match_dup 1) 24)
                    (match_dup 2)
                    (match_dup 3)] UNSPEC_FIXEDU))]
  ""
  [(set_attr "type" "mult_fp4")]
)


;; FWIDEN*

(define_insn "lvx_fwidenhw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:HF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FWIDEN))]
  ""
  "fwidenhw%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_fwidenwd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FWIDEN))]
  ""
  "fwidenwd%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_fwiden<widenx>"
  [(set (match_operand:<WIDE> 0 "register_operand" "=r")
        (unspec:<WIDE> [(match_operand:S64F 1 "register_operand" "r")
                        (match_operand 2 "" "")] UNSPEC_FWIDEN))]
  ""
  "fwidenl<hwidenx>%2 %x0 = %1\n\tfwidenm<hwidenx>%2 %y0 = %1"
  [(set_attr "type" "alu_lite_x2")
   (set_attr "length"         "8")]
)

(define_expand "lvx_fwiden<widenx>"
  [(match_operand:<WIDE> 0 "register_operand")
   (match_operand:S128F 1 "register_operand")
   (match_operand 2 "")]
  ""
  {
    rtx op1_l = simplify_gen_subreg (<HALF>mode, operands[1], <MODE>mode, 0);
    rtx op1_m = simplify_gen_subreg (<HALF>mode, operands[1], <MODE>mode, 8);
    rtx op0_l = simplify_gen_subreg (<HWIDE>mode, operands[0], <WIDE>mode, 0);
    rtx op0_m = simplify_gen_subreg (<HWIDE>mode, operands[0], <WIDE>mode, 16);
    emit_insn (gen_lvx_fwiden<hwidenx> (op0_l, op1_l, operands[2]));
    emit_insn (gen_lvx_fwiden<hwidenx> (op0_m, op1_m, operands[2]));
    DONE;
  }
)


;; FNARROW*

(define_insn "lvx_fnarrowwh"
  [(set (match_operand:HF 0 "register_operand" "=r")
        (unspec:HF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FNARROW))]
  ""
  "fnarrowwh%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_fnarrowdw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:DF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FNARROW))]
  ""
  "fnarrowdw%2 %0 = %1"
  [(set_attr "type" "alu_full")]
)

(define_insn "lvx_fnarrow<truncx>"
  [(set (match_operand:S64F 0 "register_operand" "=r")
        (unspec:S64F [(match_operand:<WIDE> 1 "register_operand" "r")
                      (match_operand 2 "" "")] UNSPEC_FNARROW))]
  ""
  "fnarrow<truncx>%2 %0 = %1"
  [(set_attr "type" "<fnarrowt>")]
)

(define_expand "lvx_fnarrow<truncx>"
  [(match_operand:S128F 0 "register_operand")
   (match_operand:<WIDE> 1 "register_operand")
   (match_operand 2 "")]
  ""
  {
    rtx op1_l = simplify_gen_subreg (<HWIDE>mode, operands[1], <WIDE>mode, 0);
    rtx op1_m = simplify_gen_subreg (<HWIDE>mode, operands[1], <WIDE>mode, 16);
    rtx op0_l = simplify_gen_subreg (<HALF>mode, operands[0], <MODE>mode, 0);
    rtx op0_m = simplify_gen_subreg (<HALF>mode, operands[0], <MODE>mode, 8);
    emit_insn (gen_lvx_fnarrow<htruncx> (op0_l, op1_l, operands[2]));
    emit_insn (gen_lvx_fnarrow<htruncx> (op0_m, op1_m, operands[2]));
    DONE;
  }
)


;; FCONJ*

(define_insn "lvx_fconjwc"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")] UNSPEC_FCONJ))]
  ""
  "fnegd %0 = %1"
  [(set_attr "type" "alu_thin")]
)

(define_insn "lvx_fconjwcp"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")] UNSPEC_FCONJ))]
  ""
  "fnegd %x0 = %x1\n\tfnegd %y0 = %y1"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn "lvx_fconjdc"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand" "r")] UNSPEC_FCONJ))]
  ""
  "copyd %x0 = %x1\n\tfnegd %y0 = %y1"
  [(set_attr "type" "alu_thin_x2")
   (set_attr "length"         "8")]
)

(define_insn_and_split "lvx_fconjwcq"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (unspec:V8SF [(match_operand:V8SF 1 "register_operand" "r")] UNSPEC_FCONJ))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V4SF (match_dup 0) 0)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 0)] UNSPEC_FCONJ))
   (set (subreg:V4SF (match_dup 0) 16)
        (unspec:V4SF [(subreg:V4SF (match_dup 1) 16)] UNSPEC_FCONJ))]
  ""
  [(set_attr "type" "alu_thin_x2")]
)

(define_insn "lvx_fconjdcp"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (unspec:V4DF [(match_operand:V4DF 1 "register_operand" "r")] UNSPEC_FCONJ))]
  ""
  "copyd %x0 = %x1\n\tfnegd %y0 = %y1\n\tcopyd %z0 = %z1\n\tfnegd %t0 = %t1"
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn_and_split "lvx_fconjdcq"
  [(set (match_operand:V8DF 0 "register_operand" "=r")
        (unspec:V8DF [(match_operand:V8DF 1 "register_operand" "r")] UNSPEC_FCONJ))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V4DF (match_dup 0) 0)
        (unspec:V4DF [(subreg:V4DF (match_dup 1) 0)] UNSPEC_FCONJ))
   (set (subreg:V4DF (match_dup 0) 32)
        (unspec:V4DF [(subreg:V4DF (match_dup 1) 32)] UNSPEC_FCONJ))]
  ""
  [(set_attr "type" "alu_thin_x2")]
)


;; FCDIV*

(define_expand "lvx_fcdivw"
  [(match_operand:SF 0 "register_operand" "")
   (match_operand:SF 1 "register_operand" "")
   (match_operand:SF 2 "register_operand" "")
   (match_operand 3 "" "")]
 "HAVE_LVX_FCDIV_SF"
 {
    rtx regpair = gen_reg_rtx (V4SFmode);
    emit_insn (gen_rtx_SET (gen_rtx_SUBREG (SFmode, regpair, 0), operands[1]));
    emit_insn (gen_rtx_SET (gen_rtx_SUBREG (SFmode, regpair, 8), operands[2]));
    emit_insn (gen_lvx_fcdivw_1 (operands[0], regpair, operands[3]));
    DONE;
 }
)

(define_insn "lvx_fcdivw_1"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:V4SF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FCDIV))]
  "HAVE_LVX_FCDIV_SF"
  "fcdivw%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_expand "lvx_fcdivd"
  [(match_operand:DF 0 "register_operand" "")
   (match_operand:DF 1 "register_operand" "")
   (match_operand:DF 2 "register_operand" "")
   (match_operand 3 "" "")]
 "HAVE_LVX_FCDIV_DF"
 {
    rtx regpair = gen_reg_rtx (V2DFmode);
    emit_insn (gen_lvx_catfdp (regpair, operands[1], operands[2]));
    emit_insn (gen_lvx_fcdivd_1 (operands[0], regpair, operands[3]));
    DONE;
 }
)

(define_insn "lvx_fcdivd_1"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:V2DF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FCDIV))]
  "HAVE_LVX_FCDIV_DF"
  "fcdivd%2 %0 = %1"
  [(set_attr "type" "alu_lite")
])


;; FSDIV*

(define_expand "lvx_fsdivw"
  [(match_operand:SF 0 "register_operand" "")
   (match_operand:SF 1 "register_operand" "")
   (match_operand:SF 2 "register_operand" "")
   (match_operand 3 "" "")]
 "HAVE_LVX_FSDIV_SF"
 {
    rtx regpair = gen_reg_rtx (V4SFmode);
    emit_insn (gen_rtx_SET (gen_rtx_SUBREG (SFmode, regpair, 0), operands[1]));
    emit_insn (gen_rtx_SET (gen_rtx_SUBREG (SFmode, regpair, 8), operands[2]));
    emit_insn (gen_lvx_fsdivw_1 (operands[0], regpair, operands[3]));
    DONE;
 }
)

(define_insn "lvx_fsdivw_1"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:V4SF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FSDIV))]
  "HAVE_LVX_FSDIV_SF"
  "fsdivw%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_expand "lvx_fsdivd"
  [(match_operand:DF 0 "register_operand" "")
   (match_operand:DF 1 "register_operand" "")
   (match_operand:DF 2 "register_operand" "")
   (match_operand 3 "" "")]
 "HAVE_LVX_FSDIV_DF"
 {
    rtx regpair = gen_reg_rtx (V2DFmode);
    emit_insn (gen_lvx_catfdp (regpair, operands[1], operands[2]));
    emit_insn (gen_lvx_fsdivd_1 (operands[0], regpair, operands[3]));
    DONE;
 }
)

(define_insn "lvx_fsdivd_1"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:V2DF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FSDIV))]
  "HAVE_LVX_FSDIV_DF"
  "fsdivd%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_expand "lvx_fsdivwp"
  [(match_operand:V2SF 0 "register_operand" "")
   (match_operand:V2SF 1 "register_operand" "")
   (match_operand:V2SF 2 "register_operand" "")
   (match_operand 3 "" "")]
  "HAVE_LVX_FSDIV_V2SF"
  {
    rtx regpair = gen_reg_rtx (V4SFmode);
    emit_insn (gen_lvx_catfwq (regpair, operands[1], operands[2]));
    emit_insn (gen_lvx_fsdivwp_1 (operands[0], regpair, operands[3]));
    DONE;
  }
)

(define_insn "lvx_fsdivwp_1"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand 2 "" "")] UNSPEC_FSDIV))]
  "HAVE_LVX_FSDIV_V2SF"
  "fsdivwp%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_expand "lvx_fsdivwq"
  [(match_operand:V4SF 0 "register_operand" "")
   (match_operand:V4SF 1 "register_operand" "")
   (match_operand:V4SF 2 "register_operand" "")
   (match_operand 3 "" "")]
  "HAVE_LVX_FSDIV_V2SF"
  {
    for (int i = 0; i < 2; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (V2SFmode, operands[0], i*8);
        rtx opnd1 = gen_rtx_SUBREG (V2SFmode, operands[1], i*8);
        rtx opnd2 = gen_rtx_SUBREG (V2SFmode, operands[2], i*8);
        emit_insn (gen_lvx_fsdivwp (opnd0, opnd1, opnd2, operands[3]));
      }
    DONE;
  }
)

(define_expand "lvx_fsdivdp"
  [(match_operand:V2DF 0 "register_operand" "")
   (match_operand:V2DF 1 "register_operand" "")
   (match_operand:V2DF 2 "register_operand" "")
   (match_operand 3 "" "")]
  "HAVE_LVX_FSDIV_DF"
  {
    for (int i = 0; i < 2; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (DFmode, operands[0], i*8);
        rtx opnd1 = gen_rtx_SUBREG (DFmode, operands[1], i*8);
        rtx opnd2 = gen_rtx_SUBREG (DFmode, operands[2], i*8);
        emit_insn (gen_lvx_fsdivd (opnd0, opnd1, opnd2, operands[3]));
      }
    DONE;
  }
)

(define_expand "lvx_fsdivwo"
  [(match_operand:V8SF 0 "register_operand" "")
   (match_operand:V8SF 1 "register_operand" "")
   (match_operand:V8SF 2 "register_operand" "")
   (match_operand 3 "" "")]
  "HAVE_LVX_FSDIV_V2SF"
  {
    for (int i = 0; i < 4; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (V2SFmode, operands[0], i*8);
        rtx opnd1 = gen_rtx_SUBREG (V2SFmode, operands[1], i*8);
        rtx opnd2 = gen_rtx_SUBREG (V2SFmode, operands[2], i*8);
        emit_insn (gen_lvx_fsdivwp (opnd0, opnd1, opnd2, operands[3]));
      }
    DONE;
  }
)

(define_expand "lvx_fsdivdq"
  [(match_operand:V4DF 0 "register_operand" "")
   (match_operand:V4DF 1 "register_operand" "")
   (match_operand:V4DF 2 "register_operand" "")
   (match_operand 3 "" "")]
  "HAVE_LVX_FSDIV_DF"
  {
    for (int i = 0; i < 4; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (DFmode, operands[0], i*8);
        rtx opnd1 = gen_rtx_SUBREG (DFmode, operands[1], i*8);
        rtx opnd2 = gen_rtx_SUBREG (DFmode, operands[2], i*8);
        emit_insn (gen_lvx_fsdivd (opnd0, opnd1, opnd2, operands[3]));
      }
    DONE;
  }
)


;; FCDIV*

(define_expand "lvx_fcdivwp"
  [(match_operand:V2SF 0 "register_operand" "")
   (match_operand:V2SF 1 "register_operand" "")
   (match_operand:V2SF 2 "register_operand" "")
   (match_operand 3 "" "")]
  "HAVE_LVX_FCDIV_V2SF"
  {
    rtx regpair = gen_reg_rtx (V4SFmode);
    emit_insn (gen_lvx_catfwq (regpair, operands[1], operands[2]));
    emit_insn (gen_lvx_fcdivwp_insn (operands[0], regpair, operands[3]));
    DONE;
  }
)

(define_insn "lvx_fcdivwp_insn"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand 2 "" "")] UNSPEC_FCDIV))]
  "HAVE_LVX_FCDIV_V2SF"
  "fcdivwp%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_expand "lvx_fcdivwq"
  [(match_operand:V4SF 0 "register_operand" "")
   (match_operand:V4SF 1 "register_operand" "")
   (match_operand:V4SF 2 "register_operand" "")
   (match_operand 3 "" "")]
  "HAVE_LVX_FCDIV_V2SF"
  {
    for (int i = 0; i < 2; i++)
      {
        rtx opnd0 = simplify_gen_subreg (V2SFmode, operands[0], V4SFmode, i*8);
        rtx opnd1 = simplify_gen_subreg (V2SFmode, operands[1], V4SFmode, i*8);
        rtx opnd2 = simplify_gen_subreg (V2SFmode, operands[2], V4SFmode, i*8);
        emit_insn (gen_lvx_fcdivwp (opnd0, opnd1, opnd2, operands[3]));
      }
    DONE;
  }
)

(define_expand "lvx_fcdivwo"
  [(match_operand:V8SF 0 "register_operand" "")
   (match_operand:V8SF 1 "register_operand" "")
   (match_operand:V8SF 2 "register_operand" "")
   (match_operand 3 "" "")]
  "HAVE_LVX_FCDIV_V2SF"
  {
    for (int i = 0; i < 4; i++)
      {
        rtx opnd0 = simplify_gen_subreg (V2SFmode, operands[0], V8SFmode, i*8);
        rtx opnd1 = simplify_gen_subreg (V2SFmode, operands[1], V8SFmode, i*8);
        rtx opnd2 = simplify_gen_subreg (V2SFmode, operands[2], V8SFmode, i*8);
        emit_insn (gen_lvx_fcdivwp (opnd0, opnd1, opnd2, operands[3]));
      }
    DONE;
  }
)

(define_expand "lvx_fcdivdp"
  [(match_operand:V2DF 0 "register_operand" "")
   (match_operand:V2DF 1 "register_operand" "")
   (match_operand:V2DF 2 "register_operand" "")
   (match_operand 3 "" "")]
  "HAVE_LVX_FCDIV_DF"
  {
    for (int i = 0; i < 2; i++)
      {
        rtx opnd0 = simplify_gen_subreg (DFmode, operands[0], V2DFmode, i*8);
        rtx opnd1 = simplify_gen_subreg (DFmode, operands[1], V2DFmode, i*8);
        rtx opnd2 = simplify_gen_subreg (DFmode, operands[2], V2DFmode, i*8);
        emit_insn (gen_lvx_fcdivd (opnd0, opnd1, opnd2, operands[3]));
      }
    DONE;
  }
)

(define_expand "lvx_fcdivdq"
  [(match_operand:V4DF 0 "register_operand" "")
   (match_operand:V4DF 1 "register_operand" "")
   (match_operand:V4DF 2 "register_operand" "")
   (match_operand 3 "" "")]
  "HAVE_LVX_FCDIV_DF"
  {
    for (int i = 0; i < 4; i++)
      {
        rtx opnd0 = simplify_gen_subreg (DFmode, operands[0], V4DFmode, i*8);
        rtx opnd1 = simplify_gen_subreg (DFmode, operands[1], V4DFmode, i*8);
        rtx opnd2 = simplify_gen_subreg (DFmode, operands[2], V4DFmode, i*8);
        emit_insn (gen_lvx_fcdivd (opnd0, opnd1, opnd2, operands[3]));
      }
    DONE;
  }
)


;; FSREC*

(define_insn "lvx_fsrecw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FSREC))]
  ""
  "fsrecw%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_fsrecd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:DF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FSREC))]
  ""
  "fsrecd%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_fsrecwp"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")
                      (match_operand 2 "" "")] UNSPEC_FSREC))]
  ""
  "fsrecwp%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_expand "lvx_fsrecwq"
  [(match_operand:V4SF 0 "register_operand" "")
   (match_operand:V4SF 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    for (int i = 0; i < 2; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (V2SFmode, operands[0], 8*i);
        rtx opnd1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8*i);
        emit_insn (gen_lvx_fsrecwp (opnd0, opnd1, operands[2]));
      }
    DONE;
  }
)

(define_expand "lvx_fsrecdp"
  [(match_operand:V2DF 0 "register_operand" "")
   (match_operand:V2DF 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    for (int i = 0; i < 2; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (DFmode, operands[0], i*8);
        rtx opnd1 = gen_rtx_SUBREG (DFmode, operands[1], i*8);
        emit_insn (gen_lvx_fsrecd (opnd0, opnd1, operands[2]));
      }
    DONE;
  }
)

(define_expand "lvx_fsrecwo"
  [(match_operand:V8SF 0 "register_operand" "")
   (match_operand:V8SF 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    for (int i = 0; i < 4; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (V2SFmode, operands[0], 8*i);
        rtx opnd1 = gen_rtx_SUBREG (V2SFmode, operands[1], 8*i);
        emit_insn (gen_lvx_fsrecwp (opnd0, opnd1, operands[2]));
      }
    DONE;
  }
)

(define_expand "lvx_fsrecdq"
  [(match_operand:V4DF 0 "register_operand" "")
   (match_operand:V4DF 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    for (int i = 0; i < 4; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (DFmode, operands[0], i*8);
        rtx opnd1 = gen_rtx_SUBREG (DFmode, operands[1], i*8);
        emit_insn (gen_lvx_fsrecd (opnd0, opnd1, operands[2]));
      }
    DONE;
  }
)


;; FSRSR*

(define_insn "lvx_fsrsrw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SF 1 "register_operand" "r")] UNSPEC_FSRSR))]
  ""
  "fsrsrw %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_fsrsrd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:DF 1 "register_operand" "r")] UNSPEC_FSRSR))]
  ""
  "fsrsrd %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_fsrsrwp"
  [(set (match_operand:V2SF 0 "register_operand" "=r")
        (unspec:V2SF [(match_operand:V2SF 1 "register_operand" "r")] UNSPEC_FSRSR))]
  ""
  "fsrsrwp %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_expand "lvx_fsrsrwq"
  [(match_operand:V4SF 0 "register_operand" "")
   (match_operand:V4SF 1 "register_operand" "")]
  ""
  {
    for (int i = 0; i < 2; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (V2SFmode, operands[0], i*8);
        rtx opnd1 = gen_rtx_SUBREG (V2SFmode, operands[1], i*8);
        emit_insn (gen_lvx_fsrsrwp (opnd0, opnd1));
      }
    DONE;
  }
)

(define_expand "lvx_fsrsrdp"
  [(match_operand:V2DF 0 "register_operand" "")
   (match_operand:V2DF 1 "register_operand" "")]
  ""
  {
    for (int i = 0; i < 2; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (DFmode, operands[0], i*8);
        rtx opnd1 = gen_rtx_SUBREG (DFmode, operands[1], i*8);
        emit_insn (gen_lvx_fsrsrd (opnd0, opnd1));
      }
    DONE;
  }
)

(define_expand "lvx_fsrsrwo"
  [(match_operand:V8SF 0 "register_operand" "")
   (match_operand:V8SF 1 "register_operand" "")]
  ""
  {
    for (int i = 0; i < 4; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (V2SFmode, operands[0], i*8);
        rtx opnd1 = gen_rtx_SUBREG (V2SFmode, operands[1], i*8);
        emit_insn (gen_lvx_fsrsrwp (opnd0, opnd1));
      }
    DONE;
  }
)

(define_expand "lvx_fsrsrdq"
  [(match_operand:V4DF 0 "register_operand" "")
   (match_operand:V4DF 1 "register_operand" "")]
  ""
  {
    for (int i = 0; i < 4; i++)
      {
        rtx opnd0 = gen_rtx_SUBREG (DFmode, operands[0], i*8);
        rtx opnd1 = gen_rtx_SUBREG (DFmode, operands[1], i*8);
        emit_insn (gen_lvx_fsrsrd (opnd0, opnd1));
      }
    DONE;
  }
)


;; READY

(define_insn "*lvx_ready"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (unspec:SI [(match_operand:DI 1 "register_operand" "r")] UNSPEC_READY))]
  ""
  "iord %0 = %1, %1"
  [(set_attr "type" "alu_tiny_use")
   (set_attr "length"          "4")]
)

(define_insn "*lvx_ready"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (unspec:SI [(match_operand:DI 1 "register_operand" "r")
                    (match_operand:DI 2 "register_operand" "r")] UNSPEC_READY))]
  ""
  "iord %0 = %1, %2"
  [(set_attr "type" "alu_tiny_use")
   (set_attr "length"          "4")]
)

(define_insn "*lvx_ready"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (unspec:SI [(match_operand:DI 1 "register_operand" "r")
                    (match_operand:DI 2 "register_operand" "r")
                    (match_operand:DI 3 "register_operand" "r")] UNSPEC_READY))]
  ""
  "iord %0 = %1, %2\n\tiord %0 = %3, %3"
  [(set_attr "type" "alu_tiny_use_x2")
   (set_attr "length"             "8")]
)

(define_insn "*lvx_ready"
  [(set (match_operand:SI 0 "register_operand" "=r")
        (unspec:SI [(match_operand:DI 1 "register_operand" "r")
                    (match_operand:DI 2 "register_operand" "r")
                    (match_operand:DI 3 "register_operand" "r")
                    (match_operand:DI 4 "register_operand" "r")] UNSPEC_READY))]
  ""
  "iord %0 = %1, %2\n\tiord %0 = %3, %4"
  [(set_attr "type" "alu_tiny_use_x2")
   (set_attr "length"             "8")]
)


;; LVX_LD, LVX_LO, LVX_LDF

(define_insn "lvx_ld"
 [(set (match_operand:DI 0 "register_operand" "=r,r,r")
       (unspec:DI [(match_operand:DI 1 "memory_operand" "a,b,m")
                   (match_operand 2 "" "")] UNSPEC_LOAD))]
  ""
  "ld%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_ldf"
 [(set (match_operand:DF 0 "register_operand" "=r,r,r")
       (unspec:DF [(match_operand:DF 1 "memory_operand" "a,b,m")
                   (match_operand 2 "" "")] UNSPEC_LOAD))]
  ""
  "ld%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_lq"
 [(set (match_operand:TI 0 "register_operand" "=r,r,r")
       (unspec:TI [(match_operand:TI 1 "memory_operand" "a,b,m")
                   (match_operand 2 "" "")] UNSPEC_LOAD))]
  ""
  "lq%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)


;; LVX_L*

(define_insn "lvx_l<SIMD64:lsvs>"
  [(set (match_operand:SIMD64 0 "register_operand" "=r,r,r")
        (unspec:SIMD64 [(match_operand:SIMD64 1 "memory_operand" "a,b,m")
                        (match_operand 2 "" "")] UNSPEC_LOAD))]
  ""
  "ld%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_l<SIMD128:lsvs>"
  [(set (match_operand:SIMD128 0 "register_operand" "=r,r,r")
        (unspec:SIMD128 [(match_operand:SIMD128 1 "memory_operand" "a,b,m")
                         (match_operand 2 "" "")] UNSPEC_LOAD))]
  ""
  "lq%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_l<SIMD256:lsvs>"
  [(set (match_operand:SIMD256 0 "register_operand" "=r,r,r")
        (unspec:SIMD256 [(match_operand:SIMD256 1 "memory_operand" "a,b,m")
                         (match_operand 2 "" "")] UNSPEC_LOAD))]
  ""
  "lo%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)


;; LVX_LBX, LVX_LHX, LVX_LWX

(define_insn "lvx_lbz"
 [(set (match_operand:DI 0 "register_operand" "=r,r,r")
       (unspec:DI [(match_operand:QI 1 "memory_operand" "a,b,m")
                   (match_operand 2 "" "")] UNSPEC_LOADZX))
   (use (match_dup 1))]
  ""
  "lbz%2%X1 %0 = %1"
  [(set_attr "type" "load_core_uncached,load_core_uncached_x,load_core_uncached_y")
   (set_attr "length"                    "4,                       8,                      12")]
)

(define_insn "lvx_lbs"
 [(set (match_operand:DI 0 "register_operand" "=r,r,r")
       (unspec:DI [(match_operand:QI 1 "memory_operand" "a,b,m")
                   (match_operand 2 "" "")] UNSPEC_LOADSX))
   (use (match_dup 1))]
  ""
  "lbs%2%X1 %0 = %1"
  [(set_attr "type" "load_core_uncached,load_core_uncached_x,load_core_uncached_y")
   (set_attr "length"                    "4,                       8,                      12")]
)

(define_insn "lvx_lhz"
 [(set (match_operand:DI 0 "register_operand" "=r,r,r")
       (unspec:DI [(match_operand:HI 1 "memory_operand" "a,b,m")
                   (match_operand 2 "" "")] UNSPEC_LOADZX))
   (use (match_dup 1))]
  ""
  "lhz%2%X1 %0 = %1"
  [(set_attr "type" "load_core_uncached,load_core_uncached_x,load_core_uncached_y")
   (set_attr "length"                    "4,                       8,                      12")]
)

(define_insn "lvx_lhs"
 [(set (match_operand:DI 0 "register_operand" "=r,r,r")
       (unspec:DI [(match_operand:HI 1 "memory_operand" "a,b,m")
                   (match_operand 2 "" "")] UNSPEC_LOADSX))
   (use (match_dup 1))]
  ""
  "lhs%2%X1 %0 = %1"
  [(set_attr "type" "load_core_uncached,load_core_uncached_x,load_core_uncached_y")
   (set_attr "length"                    "4,                       8,                      12")]
)

(define_insn "lvx_lwz"
 [(set (match_operand:DI 0 "register_operand" "=r,r,r")
       (unspec:DI [(match_operand:SI 1 "memory_operand" "a,b,m")
                   (match_operand 2 "" "")] UNSPEC_LOADZX))
   (use (match_dup 1))]
  ""
  "lwz%2%X1 %0 = %1"
  [(set_attr "type" "load_core_uncached,load_core_uncached_x,load_core_uncached_y")
   (set_attr "length"                    "4,                       8,                      12")]
)

(define_insn "lvx_lws"
 [(set (match_operand:DI 0 "register_operand" "=r,r,r")
       (unspec:DI [(match_operand:SI 1 "memory_operand" "a,b,m")
                   (match_operand 2 "" "")] UNSPEC_LOADSX))
   (use (match_dup 1))]
  ""
  "lws%2%X1 %0 = %1"
  [(set_attr "type" "load_core_uncached,load_core_uncached_x,load_core_uncached_y")
   (set_attr "length"                    "4,                       8,                      12")]
)

(define_insn "lvx_lhf"
 [(set (match_operand:HF 0 "register_operand" "=r,r,r")
       (unspec:HF [(match_operand:HF 1 "memory_operand" "a,b,m")
                   (match_operand 2 "" "")] UNSPEC_LOADZX))
   (use (match_dup 1))]
  ""
  "lhz%2%X1 %0 = %1"
  [(set_attr "type" "load_core_uncached,load_core_uncached_x,load_core_uncached_y")
   (set_attr "length"                    "4,                       8,                      12")]
)

(define_insn "lvx_lwf"
 [(set (match_operand:SF 0 "register_operand" "=r,r,r")
       (unspec:SF [(match_operand:SF 1 "memory_operand" "a,b,m")
                   (match_operand 2 "" "")] UNSPEC_LOADZX))
   (use (match_dup 1))]
  ""
  "lwz%2%X1 %0 = %1"
  [(set_attr "type" "load_core_uncached,load_core_uncached_x,load_core_uncached_y")
   (set_attr "length"                    "4,                       8,                      12")]
)


;; LVX_LOAD*

(define_insn "lvx_loadbz"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r")
        (zero_extend:DI (unspec:QI [(match_operand:QI 1 "memory_operand" "a,b,m")
                                    (match_operand 2 "" "")] UNSPEC_LOADZX)))]
  ""
  "lbz%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadhz"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r")
        (zero_extend:DI (unspec:HI [(match_operand:HI 1 "memory_operand" "a,b,m")
                                    (match_operand 2 "" "")] UNSPEC_LOADZX)))]
  ""
  "lhz%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadwz"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r")
        (zero_extend:DI (unspec:SI [(match_operand:SI 1 "memory_operand" "a,b,m")
                                    (match_operand 2 "" "")] UNSPEC_LOADZX)))]
  ""
  "lwz%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadd"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r")
        (unspec:DI [(match_operand:DI 1 "memory_operand" "a,b,m")
                    (match_operand 2 "" "")] UNSPEC_LOAD))]
  ""
  "ld%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadq"
  [(set (match_operand:TI 0 "register_operand" "=r,r,r")
        (unspec:TI [(match_operand:TI 1 "memory_operand" "a,b,m")
                    (match_operand 2 "" "")] UNSPEC_LOAD))]
  ""
  "lq%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadhf"
  [(set (match_operand:HF 0 "register_operand" "=r,r,r")
        (unspec:HF [(match_operand:HF 1 "memory_operand" "a,b,m")
                    (match_operand 2 "" "")] UNSPEC_LOADZX))]
  ""
  "lhz%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadwf"
  [(set (match_operand:SF 0 "register_operand" "=r,r,r")
        (unspec:SF [(match_operand:SF 1 "memory_operand" "a,b,m")
                    (match_operand 2 "" "")] UNSPEC_LOADZX))]
  ""
  "lwz%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loaddf"
  [(set (match_operand:DF 0 "register_operand" "=r,r,r")
        (unspec:DF [(match_operand:DF 1 "memory_operand" "a,b,m")
                    (match_operand 2 "" "")] UNSPEC_LOAD))]
  ""
  "ld%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_load64"
  [(set (match_operand:V64 0 "register_operand" "=r,r,r")
        (unspec:V64 [(match_operand:V64 1 "memory_operand" "a,b,m")
                     (match_operand 2 "" "")] UNSPEC_LOAD))]
  ""
  "ld%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_load128"
  [(set (match_operand:V128 0 "register_operand" "=r,r,r")
        (unspec:V128 [(match_operand:V128 1 "memory_operand" "a,b,m")
                      (match_operand 2 "" "")] UNSPEC_LOAD))]
  ""
  "lq%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_load256"
  [(set (match_operand:V256 0 "register_operand" "=r,r,r")
        (unspec:V256 [(match_operand:V256 1 "memory_operand" "a,b,m")
                      (match_operand 2 "" "")] UNSPEC_LOAD))]
  ""
  "lo%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn_and_split "lvx_load512"
  [(set (match_operand:V512 0 "register_operand" "=&r,&r,&r")
        (unspec:V512 [(match_operand:V512 1 "memory_operand" "a,b,m")
                      (match_operand 2 "" "")] UNSPEC_LOAD))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V256 (match_dup 0) 0)
        (unspec:V256 [(subreg:V256 (match_dup 1) 0)
                      (match_dup 2)] UNSPEC_LOAD))
   (set (subreg:V256 (match_dup 0) 32)
        (unspec:V256 [(subreg:V256 (match_dup 1) 32)
                      (match_dup 2)] UNSPEC_LOAD))]
  ""
)


;; LVX_LOADC*

(define_insn "lvx_loadcbz"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r")
        (zero_extend:DI (unspec:QI [(match_operand:DI 1 "register_operand" "0,0,0")
                                    (match_operand:QI 2 "memsimple_operand" "c,d,e")
                                    (match_operand:DI 3 "register_operand" "r,r,r")
                                    (match_operand 4 "" "")
                                    (match_operand 5 "" "")] UNSPEC_LOADC)))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2 %3? lbz%4 %0 = %2"
      : "lbz%4%5%X2 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadchz"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r")
        (zero_extend:DI (unspec:HI [(match_operand:DI 1 "register_operand" "0,0,0")
                                    (match_operand:HI 2 "memsimple_operand" "c,d,e")
                                    (match_operand:DI 3 "register_operand" "r,r,r")
                                    (match_operand 4 "" "")
                                    (match_operand 5 "" "")] UNSPEC_LOADC)))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2 %3? lhz%4 %0 = %2"
      : "lhz%4%5%X2 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadcwz"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r")
        (zero_extend:DI (unspec:SI [(match_operand:DI 1 "register_operand" "0,0,0")
                                    (match_operand:SI 2 "memsimple_operand" "c,d,e")
                                    (match_operand:DI 3 "register_operand" "r,r,r")
                                    (match_operand 4 "" "")
                                    (match_operand 5 "" "")] UNSPEC_LOADC)))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2 %3? lwz%4 %0 = %2"
      : "lwz%4%5%X2 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadcd"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r")
        (unspec:DI [(match_operand:DI 1 "register_operand" "0,0,0")
                    (match_operand:DI 2 "memsimple_operand" "c,d,e")
                    (match_operand:DI 3 "register_operand" "r,r,r")
                    (match_operand 4 "" "")
                    (match_operand 5 "" "")] UNSPEC_LOADC))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2 %3? ld%4 %0 = %2"
      : "ld%4%5%X2 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadcq"
  [(set (match_operand:TI 0 "register_operand" "=r,r,r")
        (unspec:TI [(match_operand:TI 1 "register_operand" "0,0,0")
                    (match_operand:TI 2 "memsimple_operand" "c,d,e")
                    (match_operand:DI 3 "register_operand" "r,r,r")
                    (match_operand 4 "" "")
                    (match_operand 5 "" "")] UNSPEC_LOADC))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2 %3? lq%4 %0 = %2"
      : "lq%4%5%X2 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadchf"
  [(set (match_operand:HF 0 "register_operand" "=r,r,r")
        (unspec:HF [(match_operand:HF 1 "register_operand" "0,0,0")
                    (match_operand:HF 2 "memsimple_operand" "c,d,e")
                    (match_operand:DI 3 "register_operand" "r,r,r")
                    (match_operand 4 "" "")
                    (match_operand 5 "" "")] UNSPEC_LOADC))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2 %3? lhz%4 %0 = %2"
      : "lhz%4%5%X2 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadcwf"
  [(set (match_operand:SF 0 "register_operand" "=r,r,r")
        (unspec:SF [(match_operand:SF 1 "register_operand" "0,0,0")
                    (match_operand:SF 2 "memsimple_operand" "c,d,e")
                    (match_operand:DI 3 "register_operand" "r,r,r")
                    (match_operand 4 "" "")
                    (match_operand 5 "" "")] UNSPEC_LOADC))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2 %3? lwz%4 %0 = %2"
      : "lwz%4%5%X2 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadcdf"
  [(set (match_operand:DF 0 "register_operand" "=r,r,r")
        (unspec:DF [(match_operand:DF 1 "register_operand" "0,0,0")
                    (match_operand:DF 2 "memsimple_operand" "c,d,e")
                    (match_operand:DI 3 "register_operand" "r,r,r")
                    (match_operand 4 "" "")
                    (match_operand 5 "" "")] UNSPEC_LOADC))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2 %3? ld%4 %0 = %2"
      : "ld%4%5%X2 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_expand "lvx_loadc64"
  [(match_operand:V64 0 "register_operand" "")
   (match_operand:V64 1 "reg_zero_mone_operand" "")
   (match_operand:V64 2 "memsimple_operand" "")
   (match_operand:DI 3 "register_operand" "")
   (match_operand 4 "" "")
   (match_operand 5 "" "")]
  ""
  {
    if (masked_modifier (operands[4], VOIDmode))
      {
        rtx loaded = gen_reg_rtx (<TMODE>mode);
        rtx address = XEXP (operands[2], 0);
        rtx memory = gen_rtx_MEM (<TMODE>mode, address);
        MEM_COPY_ATTRIBUTES (memory, operands[2]);
        if (!const_zero_operand (operands[1], <MODE>mode))
          {
            emit_insn (gen_rtx_SET (gen_rtx_SUBREG (<MODE>mode, loaded, 0), operands[1]));
            emit_insn (gen_lvx_loadc_ (loaded, loaded, memory, operands[3], operands[4], operands[5]));
          }
        else
          emit_insn (gen_lvx_loadc__ (loaded, memory, operands[3], operands[4], operands[5]));
        emit_insn (gen_rtx_SET (operands[0], gen_rtx_SUBREG (<MODE>mode, loaded, 0)));
      }
    else
      emit_insn (gen_lvx_loadc64_ (operands[0], operands[1], operands[2], operands[3], operands[4], operands[5]));
    DONE;
  }
)

(define_insn "lvx_loadc64_"
  [(set (match_operand:V64 0 "register_operand" "=r,r,r")
        (unspec:V64 [(match_operand:V64 1 "register_operand" "0,0,0")
                     (match_operand:V64 2 "memsimple_operand" "c,d,e")
                     (match_operand:DI 3 "register_operand" "r,r,r")
                     (match_operand 4 "" "")
                     (match_operand 5 "" "")] UNSPEC_LOADC))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2 %3? ld%4 %0 = %2"
      : "ld%4%5%X2 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_expand "lvx_loadc128"
  [(match_operand:V128 0 "register_operand" "")
   (match_operand:V128 1 "reg_zero_mone_operand" "")
   (match_operand:V128 2 "memsimple_operand" "")
   (match_operand:DI 3 "register_operand" "")
   (match_operand 4 "" "")
   (match_operand 5 "" "")]
  ""
  {
    if (masked_modifier (operands[4], VOIDmode))
      {
        rtx loaded = gen_reg_rtx (<DMODE>mode);
        rtx address = XEXP (operands[2], 0);
        rtx memory = gen_rtx_MEM (<DMODE>mode, address);
        MEM_COPY_ATTRIBUTES (memory, operands[2]);
        if (!const_zero_operand (operands[1], <MODE>mode))
          {
            emit_insn (gen_rtx_SET (gen_rtx_SUBREG (<MODE>mode, loaded, 0), operands[1]));
            emit_insn (gen_lvx_loadc_ (loaded, loaded, memory, operands[3], operands[4], operands[5]));
          }
        else
          emit_insn (gen_lvx_loadc__ (loaded, memory, operands[3], operands[4], operands[5]));
        emit_insn (gen_rtx_SET (operands[0], gen_rtx_SUBREG (<MODE>mode, loaded, 0)));
      }
    else
      emit_insn (gen_lvx_loadc128_ (operands[0], operands[1], operands[2], operands[3], operands[4], operands[5]));
    DONE;
  }
)

(define_insn "lvx_loadc128_"
  [(set (match_operand:V128 0 "register_operand" "=r,r,r")
        (unspec:V128 [(match_operand:V128 1 "register_operand" "0,0,0")
                      (match_operand:V128 2 "memsimple_operand" "c,d,e")
                      (match_operand:DI 3 "register_operand" "r,r,r")
                      (match_operand 4 "" "")
                      (match_operand 5 "" "")] UNSPEC_LOADC))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2 %3? lq%4 %0 = %2"
      : "lq%4%5%X2 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_expand "lvx_loadc256"
  [(match_operand:V256 0 "register_operand" "")
   (match_operand:V256 1 "reg_zero_mone_operand" "")
   (match_operand:V256 2 "memsimple_operand" "")
   (match_operand:DI 3 "register_operand" "")
   (match_operand 4 "" "")
   (match_operand 5 "" "")]
  ""
  {
    if (!const_zero_operand (operands[1], <MODE>mode))
      emit_insn (gen_lvx_loadc_ (operands[0], operands[1], operands[2], operands[3], operands[4], operands[5]));
    else
      emit_insn (gen_lvx_loadc__ (operands[0], operands[2], operands[3], operands[4], operands[5]));
    DONE;
  }
)

(define_insn "lvx_loadc_"
  [(set (match_operand:V256 0 "register_operand" "=r,r,r")
        (unspec:V256 [(match_operand:V256 1 "reg_zero_mone_operand" "0,0,0")
                      (match_operand:V256 2 "memsimple_operand" "c,d,e")
                      (match_operand:DI 3 "register_operand" "r,r,r")
                      (match_operand 4 "" "")
                      (match_operand 5 "" "")] UNSPEC_LOADC))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2 %3? lo%4 %0 = %2"
      : "lo%4%5%X2 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_loadc__"
  [(set (match_operand:V256 0 "register_operand" "=r,r,r")
        (unspec:V256 [(match_operand:V256 1 "memsimple_operand" "c,d,e")
                      (match_operand:DI 2 "register_operand" "r,r,r")
                      (match_operand 3 "" "")
                      (match_operand 4 "" "")] UNSPEC_LOADC))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%4%X1 %2? lo%3 %0 = %1"
      : "lo%3%4%X1 %2? %0 = %O1";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 3 "uncached_modifier") (const_string "load_core_uncached") (const_string "load_core"))
     (if_then_else (match_operand 3 "uncached_modifier") (const_string "load_core_uncached_x") (const_string "load_core_x"))
     (if_then_else (match_operand 3 "uncached_modifier") (const_string "load_core_uncached_y") (const_string "load_core_y"))])
   (set_attr "length" "4, 8, 12")]
)


;; LVX_STORE*

(define_insn "lvx_storeb"
  [(set (match_operand:QI 1 "memory_operand"  "=a,b,m")
        (unspec:QI [(match_operand:DI 0 "register_operand" "r,r,r")] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "sb%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)
(define_insn "*lvx_storeb"
  [(set (match_operand:QI 1 "memory_operand"  "=a,b,m")
        (unspec:QI [(ANY_EXTEND:DI (match_operand:QI 0 "register_operand" "r,r,r"))] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "sb%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storeh"
  [(set (match_operand:HI 1 "memory_operand"  "=a,b,m")
        (unspec:HI [(match_operand:DI 0 "register_operand" "r,r,r")] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "sh%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)
(define_insn "*lvx_storeh"
  [(set (match_operand:HI 1 "memory_operand"  "=a,b,m")
        (unspec:HI [(ANY_EXTEND:DI (match_operand:HI 0 "register_operand" "r,r,r"))] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "sh%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storew"
  [(set (match_operand:SI 1 "memory_operand"  "=a,b,m")
        (unspec:SI [(match_operand:DI 0 "register_operand" "r,r,r")] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "sw%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)
(define_insn "*lvx_storew"
  [(set (match_operand:SI 1 "memory_operand"  "=a,b,m")
        (unspec:SI [(ANY_EXTEND:DI (match_operand:SI 0 "register_operand" "r,r,r"))] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "sw%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_stored"
  [(set (match_operand:DI 1 "memory_operand"  "=a,b,m")
        (unspec:DI [(match_operand:DI 0 "register_operand" "r,r,r")] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "sd%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storeq"
  [(set (match_operand:TI 1 "memory_operand"  "=a,b,m")
        (unspec:TI [(match_operand:TI 0 "register_operand" "r,r,r")] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "sq%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storehf"
  [(set (match_operand:HF 1 "memory_operand"  "=a,b,m")
        (unspec:HF [(match_operand:HF 0 "register_operand" "r,r,r")] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "sh%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storewf"
  [(set (match_operand:SF 1 "memory_operand"  "=a,b,m")
        (unspec:SF [(match_operand:SF 0 "register_operand" "r,r,r")] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "sw%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storedf"
  [(set (match_operand:DF 1 "memory_operand"  "=a,b,m")
        (unspec:DF [(match_operand:DF 0 "register_operand" "r,r,r")] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "sd%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_store64"
  [(set (match_operand:V64 1 "memory_operand"  "=a,b,m")
        (unspec:V64 [(match_operand:V64 0 "register_operand" "r,r,r")] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "sd%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_store128"
  [(set (match_operand:V128 1 "memory_operand"  "=a,b,m")
        (unspec:V128 [(match_operand:V128 0 "register_operand" "r,r,r")] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "sq%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_store256"
  [(set (match_operand:V256 1 "memory_operand"  "=a,b,m")
        (unspec:V256 [(match_operand:V256 0 "register_operand" "r,r,r")] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "so%X1 %1 = %0"
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn_and_split "lvx_store512"
  [(set (match_operand:V512 1 "memory_operand"  "=a,b,m")
        (unspec:V512 [(match_operand:V512 0 "register_operand" "r,r,r")] UNSPEC_STORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "#"
  "reload_completed"
  [(parallel
    [(set (subreg:V256 (match_dup 1) 0)
          (unspec:V256 [(subreg:V256 (match_dup 0) 0)] UNSPEC_STORE))
     (use (match_operand:SI 2 "nonmemory_operand" ""))])
   (parallel
    [(set (subreg:V256 (match_dup 1) 32)
          (unspec:V256 [(subreg:V256 (match_dup 0) 32)] UNSPEC_STORE))
     (use (match_operand:SI 2 "nonmemory_operand" ""))])]
  ""
)


;; LVX_STOREC*

(define_insn "lvx_storecb"
  [(set (match_operand:QI 1 "memsimple_operand"  "=c,d,e")
        (unspec:QI [(match_operand:DI 0 "register_operand" "r,r,r")
                    (match_operand:DI 2 "register_operand" "r,r,r")
                    (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
      return HAVE_LVX_PREDICATION
        ? "guard%3%X1 %2? sb %1 = %0"
        : "sb%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)
(define_insn "*lvx_storecb"
  [(set (match_operand:QI 1 "memsimple_operand"  "=c,d,e")
        (unspec:QI [(ANY_EXTEND:DI (match_operand:QI 0 "register_operand" "r,r,r"))
                    (match_operand:DI 2 "register_operand" "r,r,r")
                    (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%3%X1 %2? sb %1 = %0"
      : "sb%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storech"
  [(set (match_operand:HI 1 "memsimple_operand"  "=c,d,e")
        (unspec:HI [(match_operand:DI 0 "register_operand" "r,r,r")
                    (match_operand:DI 2 "register_operand" "r,r,r")
                    (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%3%X1 %2? sh %1 = %0"
      : "sh%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)
(define_insn "*lvx_storech"
  [(set (match_operand:HI 1 "memsimple_operand"  "=c,d,e")
        (unspec:HI [(ANY_EXTEND:DI (match_operand:HI 0 "register_operand" "r,r,r"))
                    (match_operand:DI 2 "register_operand" "r,r,r")
                    (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%3%X1 %2? sh %1 = %0"
      : "sh%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storecw"
  [(set (match_operand:SI 1 "memsimple_operand"  "=c,d,e")
        (unspec:SI [(match_operand:DI 0 "register_operand" "r,r,r")
                    (match_operand:DI 2 "register_operand" "r,r,r")
                    (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%3%X1 %2? sw %1 = %0"
      : "sw%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)
(define_insn "*lvx_storecw"
  [(set (match_operand:SI 1 "memsimple_operand"  "=c,d,e")
        (unspec:SI [(ANY_EXTEND:DI (match_operand:SI 0 "register_operand" "r,r,r"))
                    (match_operand:DI 2 "register_operand" "r,r,r")
                    (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%3%X1 %2? sw %1 = %0"
      : "sw%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storecd"
  [(set (match_operand:DI 1 "memsimple_operand"  "=c,d,e")
        (unspec:DI [(match_operand:DI 0 "register_operand" "r,r,r")
                    (match_operand:DI 2 "register_operand" "r,r,r")
                    (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%3%X1 %2? sd %1 = %0"
      : "sd%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storecq"
  [(set (match_operand:TI 1 "memsimple_operand"  "=c,d,e")
        (unspec:TI [(match_operand:TI 0 "register_operand" "r,r,r")
                    (match_operand:DI 2 "register_operand" "r,r,r")
                    (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
    return HAVE_LVX_PREDICATION
     ? "guard%3%X1 %2? sq %1 = %0"
     : "sq%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storechf"
  [(set (match_operand:HF 1 "memsimple_operand"  "=c,d,e")
        (unspec:HF [(match_operand:HF 0 "register_operand" "r,r,r")
                    (match_operand:DI 2 "register_operand" "r,r,r")
                    (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
    return HAVE_LVX_PREDICATION
     ? "guard%3%X1 %2? sh %1 = %0"
     : "sh%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storecwf"
  [(set (match_operand:SF 1 "memsimple_operand"  "=c,d,e")
        (unspec:SF [(match_operand:SF 0 "register_operand" "r,r,r")
                    (match_operand:DI 2 "register_operand" "r,r,r")
                    (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%3%X1 %2? sw %1 = %0"
      : "sw%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storecdf"
  [(set (match_operand:DF 1 "memsimple_operand"  "=c,d,e")
        (unspec:DF [(match_operand:DF 0 "register_operand" "r,r,r")
                    (match_operand:DI 2 "register_operand" "r,r,r")
                    (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%3%X1 %2? sd %1 = %0"
      : "sd%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_expand "lvx_storec64"
  [(match_operand:V64 1 "memsimple_operand"  "")
   (match_operand:V64 0 "register_operand" "")
   (match_operand:DI 2 "register_operand" "")
   (match_operand 3 "" "")
   (match_operand:SI 4 "nonmemory_operand" "")]
  ""
  {
    const char *xstr = XSTR (operands[3], 0);
    if (*xstr && xstr[1] == 'm')
      {
        rtx stored = gen_reg_rtx (<TMODE>mode);
        rtx address = XEXP (operands[1], 0);
        rtx memory = gen_rtx_MEM (<TMODE>mode, address);
        MEM_COPY_ATTRIBUTES (memory, operands[1]);
        rtx mask = gen_reg_rtx (DImode);
        if (xstr[2] == 't')
          emit_insn (gen_andsi3 (gen_rtx_SUBREG (SImode, mask, 0),
                                 gen_rtx_SUBREG (SImode, operands[2], 0), GEN_INT (0xFF)));
        else if (xstr[2] == 'f')
          emit_insn (gen_iorsi3 (gen_rtx_SUBREG (SImode, mask, 0),
                                 gen_rtx_SUBREG (SImode, operands[2], 0), GEN_INT (0xFFFFFF00)));
        else
          gcc_unreachable ();
        emit_insn (gen_rtx_SET (gen_rtx_SUBREG (<MODE>mode, stored, 0), operands[0]));
        emit_insn (gen_lvx_storec256 (stored, memory, mask, operands[3], operands[4]));
      }
    else
      emit_insn (gen_lvx_storec64_ (operands[0], operands[1], operands[2], operands[3], operands[4]));
    DONE;
  }
)

(define_insn "lvx_storec64_"
  [(set (match_operand:V64 1 "memsimple_operand"  "=c,d,e")
        (unspec:V64 [(match_operand:V64 0 "register_operand" "r,r,r")
                     (match_operand:DI 2 "register_operand" "r,r,r")
                     (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%3%X1 %2? sd %1 = %0"
      : "sd%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_expand "lvx_storec128"
  [(match_operand:V128 1 "memsimple_operand"  "")
   (match_operand:V128 0 "register_operand" "")
   (match_operand:DI 2 "register_operand" "")
   (match_operand 3 "" "")
   (match_operand:SI 4 "nonmemory_operand" "")]
  ""
  {
    const char *xstr = XSTR (operands[3], 0);
    if (*xstr && xstr[1] == 'm')
      {
        rtx stored = gen_reg_rtx (<DMODE>mode);
        rtx address = XEXP (operands[1], 0);
        rtx memory = gen_rtx_MEM (<DMODE>mode, address);
        MEM_COPY_ATTRIBUTES (memory, operands[1]);
        rtx mask = gen_reg_rtx (DImode);
        if (xstr[2] == 't')
          emit_insn (gen_andsi3 (gen_rtx_SUBREG (SImode, mask, 0),
                                 gen_rtx_SUBREG (SImode, operands[2], 0), GEN_INT (0xFFFF)));
        else if (xstr[2] == 'f')
          emit_insn (gen_iorsi3 (gen_rtx_SUBREG (SImode, mask, 0),
                                 gen_rtx_SUBREG (SImode, operands[2], 0), GEN_INT (0xFFFF0000)));
        else
          gcc_unreachable ();
        emit_insn (gen_rtx_SET (gen_rtx_SUBREG (<MODE>mode, stored, 0), operands[0]));
        emit_insn (gen_lvx_storec256 (stored, memory, mask, operands[3], operands[4]));
      }
    else
      emit_insn (gen_lvx_storec128_ (operands[0], operands[1], operands[2], operands[3], operands[4]));
    DONE;
  }
)

(define_insn "lvx_storec128_"
  [(set (match_operand:V128 1 "memsimple_operand"  "=c,d,e")
        (unspec:V128 [(match_operand:V128 0 "register_operand" "r,r,r")
                      (match_operand:DI 2 "register_operand" "r,r,r")
                      (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%3%X1 %2? sq %1 = %0"
      : "sq%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

(define_insn "lvx_storec256"
  [(set (match_operand:V256 1 "memsimple_operand"  "=c,d,e")
        (unspec:V256 [(match_operand:V256 0 "register_operand" "r,r,r")
                      (match_operand:DI 2 "register_operand" "r,r,r")
                      (match_operand 3 "" "")] UNSPEC_STOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%3%X1 %2? so %1 = %0"
      : "so%3%X1 %2? %O1 = %0";
  }
  [(set_attr "type" "store_core,store_core_x,store_core_y")
   (set_attr "length"            "4,               8,              12")]
)

;; UNDEF

(define_insn_and_split "lvx_undef64"
  [(set (match_operand:V64 0 "register_operand" "=r")
        (unspec:V64 [(match_operand 1 "" "")] UNSPEC_DEF))]
  ""
  "#"
  "reload_completed"
  [(use (const_int 0))]
)

(define_insn_and_split "lvx_undef128"
  [(set (match_operand:V128 0 "register_operand" "=r")
        (unspec:V128 [(match_operand 1 "" "")] UNSPEC_DEF))]
  ""
  "#"
  "reload_completed"
  [(use (const_int 0))]
)

(define_insn_and_split "lvx_undef256"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (unspec:V256 [(match_operand 1 "" "")] UNSPEC_DEF))]
  ""
  "#"
  "reload_completed"
  [(use (const_int 0))]
)

(define_insn_and_split "lvx_undef512"
  [(set (match_operand:V512 0 "register_operand" "=r")
        (unspec:V512 [(match_operand 1 "" "")] UNSPEC_DEF))]
  ""
  "#"
  "reload_completed"
  [(use (const_int 0))]
)

