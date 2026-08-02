
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
    /* One WIDENZ per 128-bit half of the result: the low half of each 128-bit
       source chunk, then its high half via the mostsig modifier.  This used to
       step 64-bit chunks and emit a two-instruction sxl/sxm partial bundle.  */
    rtx lo = gen_rtx_CONST_STRING (VOIDmode, "");
    rtx hi = gen_rtx_CONST_STRING (VOIDmode, ".m");
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += 16)
      {
        rtx src = simplify_gen_subreg (<S128CHUNK>mode, operands[1],
                                       <MODE>mode, offset);
        rtx d0 = simplify_gen_subreg (<WCHUNK>mode, operands[0],
                                      <WIDE>mode, offset * 2);
        rtx d1 = simplify_gen_subreg (<WCHUNK>mode, operands[0],
                                      <WIDE>mode, offset * 2 + 16);
        emit_insn (gen_lvx_widenz<wchunkx> (d0, src, lo));
        emit_insn (gen_lvx_widenz<wchunkx> (d1, src, hi));
      }
    DONE;
  }
)

;; ---- WIDEN*: LVX SIMD lane extension ---------------------------------------
;;
;; These replace the KVX sxl/sxm and zxl/zxm pairs.  Each reads a 128-bit
;; vector, selects its least or most significant half via the mostsig modifier
;; (operand 2: "" or ".m"), widens those lanes to the next element size and
;; writes a 128-bit result -- one instruction, no partial bundle.  The EXTL*
;; family is the other lane-extension form in the ISA, but it selects even/odd
;; lanes rather than the low/high half, so it does not express a sequential
;; widening the way these do.

(define_insn "lvx_widens<wchunkx>"
  [(set (match_operand:<WCHUNK> 0 "register_operand" "=r")
        (unspec:<WCHUNK> [(match_operand:S128L 1 "register_operand" "r")
                          (match_operand 2 "" "")] UNSPEC_WIDENS))]
  ""
  "widens<wchunkx>%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_widenz<wchunkx>"
  [(set (match_operand:<WCHUNK> 0 "register_operand" "=r")
        (unspec:<WCHUNK> [(match_operand:S128L 1 "register_operand" "r")
                          (match_operand 2 "" "")] UNSPEC_WIDENZ))]
  "LVX_2"
  "widenz<wchunkx>%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_widenq<wchunkx>"
  [(set (match_operand:<WCHUNK> 0 "register_operand" "=r")
        (unspec:<WCHUNK> [(match_operand:S128L 1 "register_operand" "r")
                          (match_operand 2 "" "")] UNSPEC_WIDENQ))]
  "LVX_2"
  "widenq<wchunkx>%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_expand "lvx_sx<widenx>"
  [(match_operand:<WIDE> 0 "register_operand")
   (match_operand:WIDENI 1 "register_operand")]
  "LVX_2"
  {
    /* One WIDENS per 128-bit half of the result: the low half of each 128-bit
       source chunk, then its high half via the mostsig modifier.  This used to
       step 64-bit chunks and emit a two-instruction sxl/sxm partial bundle.  */
    rtx lo = gen_rtx_CONST_STRING (VOIDmode, "");
    rtx hi = gen_rtx_CONST_STRING (VOIDmode, ".m");
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += 16)
      {
        rtx src = simplify_gen_subreg (<S128CHUNK>mode, operands[1],
                                       <MODE>mode, offset);
        rtx d0 = simplify_gen_subreg (<WCHUNK>mode, operands[0],
                                      <WIDE>mode, offset * 2);
        rtx d1 = simplify_gen_subreg (<WCHUNK>mode, operands[0],
                                      <WIDE>mode, offset * 2 + 16);
        emit_insn (gen_lvx_widens<wchunkx> (d0, src, lo));
        emit_insn (gen_lvx_widens<wchunkx> (d1, src, hi));
      }
    DONE;
  }
)

(define_expand "lvx_qx<widenx>"
  [(match_operand:<WIDE> 0 "register_operand")
   (match_operand:WIDENI 1 "register_operand")]
  ""
  {
    /* One WIDENQ per 128-bit half of the result: the low half of each 128-bit
       source chunk, then its high half via the mostsig modifier.  This used to
       step 64-bit chunks and emit a two-instruction sxl/sxm partial bundle.  */
    rtx lo = gen_rtx_CONST_STRING (VOIDmode, "");
    rtx hi = gen_rtx_CONST_STRING (VOIDmode, ".m");
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += 16)
      {
        rtx src = simplify_gen_subreg (<S128CHUNK>mode, operands[1],
                                       <MODE>mode, offset);
        rtx d0 = simplify_gen_subreg (<WCHUNK>mode, operands[0],
                                      <WIDE>mode, offset * 2);
        rtx d1 = simplify_gen_subreg (<WCHUNK>mode, operands[0],
                                      <WIDE>mode, offset * 2 + 16);
        emit_insn (gen_lvx_widenq<wchunkx> (d0, src, lo));
        emit_insn (gen_lvx_widenq<wchunkx> (d1, src, hi));
      }
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

(define_insn "lvx_zxebho"
  [(set (match_operand:V8HI 0 "register_operand" "=r")
        (unspec:V8HI [(match_operand:V16QI 1 "register_operand" "r")] UNSPEC_ZXE))]
  ""
  "andq %0 = %1, 0x00FF00FF.@"
  [(set_attr "type" "alu_tiny_x2_x")
   (set_attr "length"          "16")]
)

(define_insn "lvx_zxebhx"
  [(set (match_operand:V16HI 0 "register_operand" "=r")
        (unspec:V16HI [(match_operand:V32QI 1 "register_operand" "r")] UNSPEC_ZXE))]
  "LVX_2"
  {
    return "andq %L0 = %L1, 0x00FF00FF.@\n\tandq %M0 = %M1, 0x00FF00FF.@";
  }
  [(set_attr "type" "alu_tiny_x2")
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

(define_insn "lvx_zxehwq"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V8HI 1 "register_operand" "r")] UNSPEC_ZXE))]
  ""
  "andq %0 = %1, 0x0000FFFF.@"
  [(set_attr "type" "alu_tiny_x2_x")
   (set_attr "length"          "16")]
)

(define_insn "lvx_zxehwo"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (unspec:V8SI [(match_operand:V16HI 1 "register_operand" "r")] UNSPEC_ZXE))]
  "LVX_2"
  {
    return "andq %L0 = %L1, 0x0000FFFF.@\n\tandq %M0 = %M1, 0x0000FFFF.@";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"          "32")]
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
  "LVX_2"
  {
    return "zxwd %x0 = %x1\n\tzxwd %y0 = %y1\n\t"
           "zxwd %z0 = %z1\n\tzxwd %t0 = %t1";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn "lvx_qxebho"
  [(set (match_operand:V8HI 0 "register_operand" "=r")
        (unspec:V8HI [(match_operand:V16QI 1 "register_operand" "r")] UNSPEC_QXE))]
  "LVX_2"
  "sllho %0 = %1, 8"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
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

(define_insn "lvx_qxehwq"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V8HI 1 "register_operand" "r")] UNSPEC_QXE))]
  "LVX_2"
  "sllwq %0 = %1, 16"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
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

(define_insn "lvx_qxewdp"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V4SI 1 "register_operand" "r")] UNSPEC_QXE))]
  "LVX_2"
  "slldp %0 = %1, 32"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "lvx_qxewdq"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (unspec:V4DI [(match_operand:V8SI 1 "register_operand" "r")] UNSPEC_QXE))]
  "LVX_2"
  {
    return "slldp %L0 = %L1, 32\n\tslldp %M0 = %M1, 32";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
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

(define_insn "lvx_zxobho"
  [(set (match_operand:V8HI 0 "register_operand" "=r")
        (unspec:V8HI [(match_operand:V16QI 1 "register_operand" "r")] UNSPEC_ZXO))]
  "LVX_2"
  "srlho %0 = %1, 8"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
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

(define_insn "lvx_zxohwq"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V8HI 1 "register_operand" "r")] UNSPEC_ZXO))]
  "LVX_2"
  "srlwq %0 = %1, 16"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
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

(define_insn "lvx_zxowdp"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V4SI 1 "register_operand" "r")] UNSPEC_ZXO))]
  "LVX_2"
  "srldp %0 = %1, 32"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "lvx_zxowdq"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (unspec:V4DI [(match_operand:V8SI 1 "register_operand" "r")] UNSPEC_ZXO))]
  "LVX_2"
  {
    return "srldp %L0 = %L1, 32\n\tsrldp %M0 = %M1, 32";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "lvx_qxobho"
  [(set (match_operand:V8HI 0 "register_operand" "=r")
        (unspec:V8HI [(match_operand:V16QI 1 "register_operand" "r")] UNSPEC_QXO))]
  ""
  "andq %0 = %1, 0xFF00FF00.@"
  [(set_attr "type" "alu_tiny_x2_x")
   (set_attr "length"          "16")]
)

(define_insn "lvx_qxobhx"
  [(set (match_operand:V16HI 0 "register_operand" "=r")
        (unspec:V16HI [(match_operand:V32QI 1 "register_operand" "r")] UNSPEC_QXO))]
  "LVX_2"
  {
    return "andq %L0 = %L1, 0xFF00FF00.@\n\tandq %M0 = %M1, 0xFF00FF00.@";
  }
  [(set_attr "type" "alu_tiny_x2")
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

(define_insn "lvx_qxohwq"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V8HI 1 "register_operand" "r")] UNSPEC_QXO))]
  ""
  "andq %0 = %1, 0xFFFF0000.@"
  [(set_attr "type" "alu_tiny_x2_x")
   (set_attr "length"          "16")]
)

(define_insn "lvx_qxohwo"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (unspec:V8SI [(match_operand:V16HI 1 "register_operand" "r")] UNSPEC_QXO))]
  "LVX_2"
  {
    return "andq %L0 = %L1, 0xFFFF0000.@\n\tandq %M0 = %M1, 0xFFFF0000.@";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"          "32")]
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

(define_insn "lvx_splatq256"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (unspec:V256 [(match_operand:V128 1 "register_operand" "r")] UNSPEC_SPLAT))]
  ""
  "copyd %x0 = %x1\n\tcopyd %y0 = %y1\n\tcopyd %z0 = %x1\n\tcopyd %t0 = %y1"
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
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
  "LVX_2 && (HAVE_LVX_SELECT_<MODE>)"
  {
    return "cmoved%4 %3? %x0 = %x1\n\tcmoved%4 %3? %y0 = %y1\n\t"
           "cmoved%4 %3? %z0 = %z1\n\tcmoved%4 %3? %t0 = %t1";
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
  "LVX_2 && (HAVE_LVX_SELECT_V32QI)"
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
  "LVX_2 && (HAVE_LVX_SELECT_<MODE>)"
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
  "LVX_2 && (HAVE_LVX_SELECT_<MODE>)"
  {
    return "cmove<chunkx>%4 %x3? %x0 = %x1\n\tcmove<chunkx>%4 %y3? %y0 = %y1\n\t"
           "cmove<chunkx>%4 %z3? %z0 = %z1\n\tcmove<chunkx>%4 %t3? %t0 = %t1";
  }
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn "lvx_stsuho"
  [(set (match_operand:V8HI 0 "register_operand" "=r")
        (unspec:V8HI [(match_operand:V8HI 1 "register_operand" "r")
                      (match_operand:V8HI 2 "register_operand" "r")] UNSPEC_STSU))]
  "LVX_2 && (HAVE_LVX_STSU_V4HI)"
  "stsuho %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "lvx_stsuhx"
  [(set (match_operand:V16HI 0 "register_operand" "=r")
        (unspec:V16HI [(match_operand:V16HI 1 "register_operand" "r")
                       (match_operand:V16HI 2 "register_operand" "r")] UNSPEC_STSU))]
  "LVX_2 && (HAVE_LVX_STSU_V4HI)"
  {
    return "stsuho %L0 = %L1, %L2\n\tstsuho %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
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

(define_insn "lvx_stsuwq"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (unspec:V4SI [(match_operand:V4SI 1 "register_operand" "r")
                      (match_operand:V4SI 2 "register_operand" "r")] UNSPEC_STSU))]
  "LVX_2 && (HAVE_LVX_STSU_V2SI)"
  "stsuwq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "lvx_stsuwo"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (unspec:V8SI [(match_operand:V8SI 1 "register_operand" "r")
                      (match_operand:V8SI 2 "register_operand" "r")] UNSPEC_STSU))]
  "LVX_2 && (HAVE_LVX_STSU_V2SI)"
  {
    return "stsuwq %L0 = %L1, %L2\n\tstsuwq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
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
  "LVX_2 && (HAVE_LVX_STSU_V2DI)"
  "stsudp %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
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
  "LVX_2 && (HAVE_LVX_STSU_V4DI)"
  {
    return "stsudp %L0 = %L1, %L2\n\tstsudp %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
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

(define_insn "lvx_sbmm8dp"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V2DI 1 "register_operand" "r")
                      (match_operand:V2DI 2 "register_operand" "r")] UNSPEC_SBMM8D))]
  "LVX_2 && (HAVE_LVX_SBMM8_V2DI)"
  "sbmm8dp %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
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
  "LVX_2 && (HAVE_LVX_SBMM8_V4DI)"
  {
    return "sbmm8dp %L0 = %L1, %L2\n\tsbmm8dp %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*sbmm8dq_s1"
  [(set (match_operand:ALL256 0 "register_operand" "=r")
        (unspec:ALL256 [(vec_duplicate:V4DI (match_operand:DI 1 "register_operand" "r"))
                        (match_operand:SIMD256 2 "register_operand" "r")] UNSPEC_SBMM8D))]
  "LVX_2 && (HAVE_LVX_SBMM8_V4DI)"
  {
    return "sbmm8dp %L0 = %1, %L2\n\tsbmm8dp %M0 = %1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*sbmm8dq_s2"
  [(set (match_operand:ALL256 0 "register_operand" "=r")
        (unspec:ALL256 [(match_operand:SIMD256 1 "register_operand" "r")
                        (vec_duplicate:V4DI (match_operand:DI 2 "register_operand" "r"))] UNSPEC_SBMM8D))]
  "LVX_2 && (HAVE_LVX_SBMM8_V4DI)"
  {
    return "sbmm8dp %L0 = %L1, %2\n\tsbmm8dp %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
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
  "LVX_2 && (HAVE_LVX_TSBMM8_V4DI)"
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
  "LVX_2 && (HAVE_LVX_TSBMM8_V4DI)"
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
  "LVX_2 && (HAVE_LVX_TSBMM8_V4DI)"
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

(define_insn_and_split "lvx_fadddp"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand" "r")
                      (match_operand:V2DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "LVX_2"
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
  "LVX_2"
  "#"
  [(set_attr "type" "madd_fp4")]
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

(define_insn_and_split "lvx_fsbfdp"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V2DF 1 "register_operand" "r")
                      (match_operand:V2DF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  "LVX_2"
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
  "LVX_2"
  "#"
  [(set_attr "type" "madd_fp4")]
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
  "LVX_2"
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
  "LVX_2"
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

(define_insn "lvx_fmm222w_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMM))]
  "HAVE_LVX_MULT_M2x2_SF_M2x2_SF_M2x2_SF"
  "fmm222w%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
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

(define_insn "lvx_ffdmawq_2"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V8SF 1 "register_operand" "r")
                      (match_operand:V8SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FFDMA))]
  "HAVE_LVX_FDMA_V4SF_V8SF_V8SF"
  "ffdmawq%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp4")]
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

(define_insn "lvx_fwidenhwq"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V8HF 1 "register_operand" "r")
                      (match_operand 2 "" "")] UNSPEC_FWIDEN))]
  "LVX_2"
  "fwidenhwq%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_fwidenwdp"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (unspec:V2DF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand 2 "" "")] UNSPEC_FWIDEN))]
  "LVX_2"
  "fwidenwdp%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_fwidenwd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FWIDEN))]
  ;; not LVX_2: fwidenwd is a scalar SF->DF widen and is on lvx_v1 too.
  ""
  "fwidenwd%2 %0 = %1"
  [(set_attr "type" "alu_lite")]
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

(define_insn "lvx_fconjwcp"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")] UNSPEC_FCONJ))]
  "LVX_2"
  "fnegdp %0 = %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
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
  "LVX_2"
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

(define_expand "lvx_fsrecwo"
  [(match_operand:V8SF 0 "register_operand" "")
   (match_operand:V8SF 1 "register_operand" "")
   (match_operand 2 "" "")]
  ""
  {
    /* No fsrecwo in the ISA: 256 bits is two 128-bit fsrecwq.  */
    for (int i = 0; i < 2; i++)
      emit_insn (gen_lvx_fsrecwq (simplify_gen_subreg (V4SFmode, operands[0],
                                                       V8SFmode, i * 16),
                                  simplify_gen_subreg (V4SFmode, operands[1],
                                                       V8SFmode, i * 16),
                                  operands[2]));
    DONE;
  }
)

(define_expand "lvx_fsrsrwo"
  [(match_operand:V8SF 0 "register_operand" "")
   (match_operand:V8SF 1 "register_operand" "")]
  ""
  {
    /* No fsrsrwo in the ISA: 256 bits is two 128-bit fsrsrwq.  */
    for (int i = 0; i < 2; i++)
      emit_insn (gen_lvx_fsrsrwq (simplify_gen_subreg (V4SFmode, operands[0],
                                                       V8SFmode, i * 16),
                                  simplify_gen_subreg (V4SFmode, operands[1],
                                                       V8SFmode, i * 16)));
    DONE;
  }
)

(define_insn "lvx_fsrecw"
  [(set (match_operand:SF 0 "register_operand" "=r")
        (unspec:SF [(match_operand:SF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FSREC))]
  ""
  "fsrecw %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_fsrecd"
  [(set (match_operand:DF 0 "register_operand" "=r")
        (unspec:DF [(match_operand:DF 1 "register_operand" "r")
                    (match_operand 2 "" "")] UNSPEC_FSREC))]
  ""
  "fsrecd %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_fsrecwq"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand 2 "" "")] UNSPEC_FSREC))]
  "LVX_2"
  "fsrecwq %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_expand "lvx_fsrecdp"
  [(match_operand:V2DF 0 "register_operand" "")
   (match_operand:V2DF 1 "register_operand" "")
   (match_operand 2 "" "")]
  "LVX_2"
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

(define_insn "lvx_fsrsrwq"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")] UNSPEC_FSRSR))]
  "LVX_2"
  "fsrsrwq %0 = %1"
  [(set_attr "type" "alu_lite")]
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

;; ---- Restored: native wide insns whose only 64-bit content was a
;; ---- split fallback for a HAVE_LVX_* capability macro frozen at (0).

;; ---- Restored: native wide insns whose only 64-bit content was a
;; ---- split fallback for a HAVE_LVX_* capability macro frozen at (0).

(define_insn "lvx_faddho"
  [(set (match_operand:V8HF 0 "register_operand" "=r")
        (unspec:V8HF [(match_operand:V8HF 1 "register_operand" "r")
                      (match_operand:V8HF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "LVX_2"
  "faddho%3 %0 = %1, %2"
  [(set_attr "type" "madd_fp3")]
)

(define_insn "lvx_faddwq"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FADD))]
  "LVX_2"
  "faddwq%3 %0 = %1, %2"
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn "lvx_fsbfho"
  [(set (match_operand:V8HF 0 "register_operand" "=r")
        (unspec:V8HF [(match_operand:V8HF 1 "register_operand" "r")
                      (match_operand:V8HF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  "LVX_2"
  "fsbfho%3 %0 = %2, %1"
  [(set_attr "type" "mult_fp3")]
)

(define_insn "lvx_fsbfwq"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FSBF))]
  "LVX_2"
  "fsbfwq%3 %0 = %1, %2"
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn "lvx_fmulho"
  [(set (match_operand:V8HF 0 "register_operand" "=r")
        (unspec:V8HF [(match_operand:V8HF 1 "register_operand" "r")
                      (match_operand:V8HF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMUL))]
  "LVX_2"
  "fmulho%3 %0 = %1, %2"
  [(set_attr "type" "mult_fp3")]
)

(define_insn "lvx_fmulwq"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (unspec:V4SF [(match_operand:V4SF 1 "register_operand" "r")
                      (match_operand:V4SF 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_FMUL))]
  "LVX_2"
  "fmulwq%3 %0 = %1, %2"
  [(set_attr "type" "mulwq_fp4")]
)

;; ---- Narrowing: LVX's trunc/fract, 128-bit destination ---------------------
;; <truncx> on the S128L modes gives hbx / who / dwq, all of which exist; the
;; wider spellings (hbv, whx, dwo) and the whole sat*/satu* family do not.

(define_insn "lvx_trunc<truncx>"
  [(set (match_operand:S128L 0 "register_operand" "=r")
        (truncate:S128L (match_operand:<WIDE> 1 "register_operand" "r")))]
  "LVX_2"
  "trunc<truncx> %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_insn "lvx_fract<truncx>"
  [(set (match_operand:S128L 0 "register_operand" "=r")
        (unspec:S128L [(match_operand:<WIDE> 1 "register_operand" "r")]
                      UNSPEC_FRACT))]
  "LVX_2"
  "fract<truncx> %0 = %1"
  [(set_attr "type" "alu_lite")]
)

(define_expand "trunc<wide><mode>2"
  [(set (match_operand:S128L 0 "register_operand" "")
        (truncate:S128L (match_operand:<WIDE> 1 "register_operand" "")))]
  "LVX_2"
  {
    emit_insn (gen_lvx_trunc<truncx> (operands[0], operands[1]));
    DONE;
  }
)
