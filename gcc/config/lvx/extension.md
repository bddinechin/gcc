;; XUNDEF

(define_insn_and_split "lvx_xundef256"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand 1 "" "")] UNSPEC_DEF))]
  ""
  "#"
  "reload_completed"
  [(use (const_int 0))]
)

(define_insn_and_split "lvx_xundef<bitsize>"
  [(set (match_operand:XBUFF 0 "register_operand" "=x")
        (unspec:XBUFF [(match_operand 1 "" "")] UNSPEC_DEF))]
  ""
  "#"
  "reload_completed"
  [(use (const_int 0))]
)


;; XZERO

(define_insn "lvx_xzero256"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (match_operand:X256 1 "const_zero_operand" ""))]
  ""
  {
    if (!HAVE_LVX_EXT_EOR_<MODE>)
      return "#";
    return "xeoro %0 = %0, %0";
  }
  [(set_attr "type" "ext_int")]
)

(define_split
  [(set (match_operand:X256 0 "register_operand" "")
        (match_operand:X256 1 "const_zero_operand" ""))]
  "!HAVE_LVX_EXT_EOR_<MODE>"
  [(set (match_dup 1) (const_int 0))
   (set (match_dup 0)
        (unspec:X256 [(match_dup 1)] UNSPEC_XSPLATD))]
  {
    operands[1] = gen_reg_rtx (DImode);
  }
)

(define_insn "lvx_xzero<bitsize>"
  [(set (match_operand:XBUFF 0 "register_operand" "=x")
        (match_operand:XBUFF 1 "const_zero_operand" ""))]
  ""
  "#"
)

(define_split
  [(set (match_operand:XBUFF 0 "register_operand" "")
        (match_operand:XBUFF 1 "const_zero_operand" ""))]
  "!HAVE_LVX_EXT_SPLAT_<MODE>"
  [(set (match_dup 1) (const_int 0))
   (set (match_dup 0)
        (unspec:XBUFF [(match_dup 1)] UNSPEC_XSPLATD))]
  {
    operands[1] = gen_reg_rtx (DImode);
  }
)

(define_split
  [(set (match_operand:XBUFF 0 "register_operand" "")
        (match_operand:XBUFF 1 "const_zero_operand" ""))]
  "HAVE_LVX_EXT_SPLAT_<MODE>"
  [(use (const_int 0))]
  {
    emit_insn (gen_lvx_xzero256 (gen_rtx_SUBREG (<CHUNK>mode, operands[0], 0), CONST0_RTX (<CHUNK>mode)));
    emit_insn (gen_lvx_xsplato<bitsize> (operands[0], gen_rtx_SUBREG (<CHUNK>mode, operands[0], 0)));
  }
)


;; 256-bit Extension Moves

(define_insn "lvx_xmovefo"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (unspec:V256 [(match_operand:X256 1 "register_operand" "x")]
                     UNSPEC_XMOVEF))]
   "LVX_2 && (HAVE_LVX_MOV_FROM_CORE_V4DI_TO_EXT_V1OI)"
   "xmovefo %0 = %1"
   [(set_attr "type" "movef_ext")]
)

(define_insn "lvx_xmovefq"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:X256 1 "register_operand" "x")
                      (match_operand 2 "" "")]
                     UNSPEC_XMOVEF))]
   "LVX_2 && (HAVE_LVX_MOV_FROM_CORE_V2DI_TO_EXT_V1OI)"
   "xmovefq %0 = %1%2"
   [(set_attr "type" "movef_ext")]
)

(define_insn "lvx_xmovefd"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unspec:DI [(match_operand:X256 1 "register_operand" "x")
                    (match_operand 2 "" "")]
                   UNSPEC_XMOVEF))]
   "LVX_2 && (HAVE_LVX_MOV_FROM_CORE_V1DI_TO_EXT_V1OI)"
   "xmovefd %0 = %1%2"
   [(set_attr "type" "movef_ext")]
)

(define_insn_and_split "lvx_xmoveto"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:V256 1 "register_operand" "r")]
                     UNSPEC_XMOVET))]
  "LVX_2 && (HAVE_LVX_MOV_FROM_EXT_V1OI_TO_CORE_V2DI)"
  "#"
  "reload_completed"
  [(const_int 0)]
  {
    rtx op0_lo = simplify_gen_subreg (<X256:HALF>mode, operands[0], <X256:MODE>mode, 0);
    rtx op1_lo = simplify_gen_subreg (<X256:HALF>mode, operands[1], <V256:MODE>mode, 0);
    rtx op0_hi = simplify_gen_subreg (<X256:HALF>mode, operands[0], <X256:MODE>mode, 16);
    rtx op1_hi = simplify_gen_subreg (<X256:HALF>mode, operands[1], <V256:MODE>mode, 16);

    emit_insn (gen_rtx_SET (op0_lo, gen_rtx_UNSPEC (<X256:HALF>mode, gen_rtvec (1, op1_lo), UNSPEC_XMOVET_LO)));
    emit_insn (gen_rtx_SET (op0_hi, gen_rtx_UNSPEC (<X256:HALF>mode, gen_rtvec (1, op1_hi), UNSPEC_XMOVET_HI)));
    DONE;
  }
  [(set_attr "type" "movet_ext")
   (set_attr "length" "8")]
)

(define_insn "lvx_xmovetq"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "0")
                      (match_operand:V2DI 2 "register_operand" "r")
                      (match_operand 3 "" "")]
                     UNSPEC_XMOVET))]
  "HAVE_LVX_MOV_FROM_EXT_V1OI_TO_CORE_V2DI"
  "xputdq %0%3 = %x2, %y2"
  [(set_attr "type" "alu_tiny_recv")
   (set_attr "length" "4")]
)

(define_insn "lvx_xmovetd"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "0")
                      (match_operand:DI 2 "register_operand" "r")
                      (match_operand 3 "" "")]
                     UNSPEC_XMOVET))]
  "LVX_2 && (HAVE_LVX_MOV_FROM_EXT_V1OI_TO_CORE_V1DI)"
  "xmovetd %0%3 = %2"
  [(set_attr "type" "alu_lite_recv")
   (set_attr "length" "4")]
)

(define_expand "mov<mode>"
  [(set (match_operand:X256 0 "nonimmediate_operand" "")
        (match_operand:X256 1 "general_operand" ""))]
  ""
  {
    if (MEM_P(operands[0]))
      operands[1] = force_reg (<MODE>mode, operands[1]);
    if (CONSTANT_P (operands[1]))
      {
        rtx temp = gen_reg_rtx (OImode);
        emit_insn (gen_rtx_SET (temp, CONST_VECTOR_ELT (operands[1], 0)));
        rtx subreg = simplify_gen_subreg (<MODE>mode, temp, OImode, 0);
        emit_insn (gen_rtx_SET (operands[0], subreg));
        DONE;
      }
  }
)

(define_insn "*mov<mode>"
  [(set (match_operand:X256 0 "nonimmediate_operand" "=x,x,x,x,a,b,m,r,x,r")
        (match_operand:X256 1 "nonimmediate_operand"  "x,a,b,m,x,x,x,x,r,r"))]
  "LVX_2 && (!HAVE_LVX_EXT_CACHED_LOAD)"
  {
    switch (which_alternative)
      {
      case 0:
        return "xcopyo %0 = %1";
      case 1: case 2: case 3:
        return "xlo.u%X1 %0 = %1";
      case 4: case 5: case 6:
        return "xso%X0 %0 = %1";
      case 7:
        return "xmovefo %0 = %1";
      case 8:
        return "#";
      case 9:
        return "copyo %0 = %1";
      default:
        gcc_unreachable ();
      }
  }
  [(set_attr "type" "copy_ext,load_ext_uncached,load_ext_uncached_x,load_ext_uncached_y,store_ext,store_ext_x,store_ext_y,movef_ext,movet_ext,copy_core")
   (set_attr "length"                "4,                4,                  8,                 12,             4,               8,              12,                 4,                    8,            4")]
)

(define_insn "*mov<mode>"
  [(set (match_operand:X256 0 "nonimmediate_operand" "=x, x, x, x, x, x, x,a,b,m,r,x,r")
        (match_operand:X256 1 "nonimmediate_operand"  "x,Ca,Cb,Cm,Za,Zb,Zm,x,x,x,x,r,r"))]
  "LVX_2 && (HAVE_LVX_EXT_CACHED_LOAD)"
  {
    switch (which_alternative)
      {
      case 0:
        return "xcopyo %0 = %1";
      case 1: case 2: case 3: case 4: case 5: case 6:
        return "xlo%V1 %0 = %1";
      case 7: case 8: case 9:
        return "xso%X0 %0 = %1";
      case 10:
        return "xmovefo %0 = %1";
      case 11:
        return "#";
      case 12:
        return "copyo %0 = %1";
      default:
        gcc_unreachable ();
      }
  }
  [(set_attr "type" "copy_ext,load_ext,load_ext_x,load_ext_y,load_ext_uncached,load_ext_uncached_x,load_ext_uncached_y,store_ext,store_ext_x,store_ext_y,movef_ext,movet_ext,copy_core")
   (set_attr "length"                "4,       4,         8,        12,                4,                  8,                 12,             4,               8,              12,                 4,                    8,            4")]
)

(define_insn "*xmovef<mode>"
  [(set (match_operand:ALL256X 0 "register_operand" "=r")
        (match_operand:ALL256X 1 "register_operand" "x"))]
  "LVX_2"
  "xmovefo %0 = %1"
  [(set_attr "type" "movef_ext")
   (set_attr "length" "4")]
)

(define_insn "*xmovet<mode>_lo"
  [(set (match_operand:ALL128 0 "register_operand" "=x")
        (unspec:ALL128 [(match_operand:ALL128 1 "register_operand" "r")]
                       UNSPEC_XMOVET_LO))]
  ""
  "xputdq %0.lo = %x1, %y1"
  [(set_attr "type" "movet_ext_hi")
   (set_attr "length" "4")]
)

(define_insn "*xmovet<mode>_hi"
  [(set (match_operand:ALL128 0 "register_operand" "=x")
        (unspec:ALL128 [(match_operand:ALL128 1 "register_operand" "r")]
                       UNSPEC_XMOVET_HI))]
  ""
  "xputdq %0.hi = %x1, %y1"
  [(set_attr "type" "movet_ext_lo")
   (set_attr "length" "4")]
)

(define_split
  [(set (match_operand:ALL256X 0 "register_operand" "")
        (match_operand:ALL256X 1 "register_operand" ""))]
  "reload_completed &&
  (extension_register_operand (operands[0], VOIDmode)
    && !extension_register_operand (operands[1], VOIDmode))"
  [(const_int 0)]
  {
    rtx op0_lo = simplify_gen_subreg (<HALF>mode, operands[0], <MODE>mode, 0);
    rtx op1_lo = simplify_gen_subreg (<HALF>mode, operands[1], <MODE>mode, 0);
    rtx op0_hi = simplify_gen_subreg (<HALF>mode, operands[0], <MODE>mode, 16);
    rtx op1_hi = simplify_gen_subreg (<HALF>mode, operands[1], <MODE>mode, 16);

    emit_insn (gen_rtx_SET (op0_lo, gen_rtx_UNSPEC (<HALF>mode, gen_rtvec (1, op1_lo), UNSPEC_XMOVET_LO)));
    emit_insn (gen_rtx_SET (op0_hi, gen_rtx_UNSPEC (<HALF>mode, gen_rtvec (1, op1_hi), UNSPEC_XMOVET_HI)));
    DONE;
  }
)

;; {512, 1024, 2048, 4096, 8192}-bit Extension Moves

(define_expand "mov<mode>"
  [(set (match_operand:XMOVM 0 "nonimmediate_operand" "")
        (match_operand:XMOVM 1 "general_operand" ""))]
  "LVX_2"
  {
    if (MEM_P(operands[0]))
      operands[1] = force_reg (<MODE>mode, operands[1]);
  }
)

(define_insn_and_split "*mov<mode>"
  [(set (match_operand:XMOVM 0 "nonimmediate_operand" "=x,x,x,x,a,b,m,r,x")
        (match_operand:XMOVM 1 "nonimmediate_operand"  "x,a,b,m,x,x,x,x,r"))]
  "" "#" "reload_completed"
  [(const_int 0)]
  {
     lvx_split_tca_moves (operands[0], operands[1]);
     DONE;
  }
)

;; XCAT*

(define_insn_and_split "lvx_xcat<bitsize>"
  [(set (match_operand:XCATM 0 "register_operand" "=x")
        (vec_concat:XCATM (match_operand:<HALF> 1 "register_operand" "0")
                          (match_operand:<HALF> 2 "register_operand" "x")))]
  "" "#" "reload_completed"
  [(const_int 0)]
  {
    lvx_split_tca_vec_concat (operands[0], operands[2]);
    DONE;
  }
)

;; XLOW*, XHIGH*

(define_insn "lvx_xlow<hbitsize>"
  [(set (match_operand:<HALF> 0 "register_operand" "=x")
        (subreg:<HALF> (match_operand:XCATM 1 "register_operand" "x") 0))]
  ""
  "#"
)

(define_insn "lvx_xhigh<hbitsize>"
  [(set (match_operand:<HALF> 0 "register_operand" "=x")
        (subreg:<HALF> (match_operand:XCATM 1 "register_operand" "x") <hbytesize>))]
  ""
  "#"
)

;; XLOAD256, XLOAD512, XLOAD1024

(define_insn "lvx_xload256"
  [(set (match_operand:X256 0 "register_operand" "=x,x,x")
        (unspec:X256 [(match_operand:X256 1 "memory_operand" "a,b,m")
                        (match_operand 2 "" "")] UNSPEC_XLOAD))]
  ""
  "xlo%2%X1 %0 = %1"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 2 "uncached_modifier") (const_string "load_ext_uncached") (const_string "load_ext"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_ext_uncached_x") (const_string "load_ext_x"))
     (if_then_else (match_operand 2 "uncached_modifier") (const_string "load_ext_uncached_y") (const_string "load_ext_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn_and_split "lvx_xload512"
  [(set (match_operand:X512 0 "register_operand" "=x,x,x")
        (unspec:X512 [(match_operand:X512 1 "memsimple_operand" "c,d,e")
                        (match_operand 2 "" "")] UNSPEC_XLOAD))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 0)
                         (match_dup 2)] UNSPEC_XLOAD))
   (set (subreg:<CHUNK> (match_dup 0) 32)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 32)
                         (match_dup 2)] UNSPEC_XLOAD))]
  ""
)

(define_insn_and_split "lvx_xload1024"
  [(set (match_operand:X1024 0 "register_operand" "=x,x,x")
        (unspec:X1024 [(match_operand:X1024 1 "memsimple_operand" "c,d,e")
                         (match_operand 2 "" "")] UNSPEC_XLOAD))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 0)
                         (match_dup 2)] UNSPEC_XLOAD))
   (set (subreg:<CHUNK> (match_dup 0) 32)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 32)
                         (match_dup 2)] UNSPEC_XLOAD))
   (set (subreg:<CHUNK> (match_dup 0) 64)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 64)
                         (match_dup 2)] UNSPEC_XLOAD))
   (set (subreg:<CHUNK> (match_dup 0) 96)
        (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 1) 96)
                         (match_dup 2)] UNSPEC_XLOAD))]
  ""
)


;; XLOAD1024Q*, XLOADC1024Q*

(define_insn "lvx_xload1024q0"
  [(set (match_operand:X1024 0 "register_operand" "=x,x,x")
        (unspec:X1024 [(match_operand:X1024 1 "register_operand" "0,0,0")
                       (match_operand:<CHUNK> 2 "memory_operand" "a,b,m")
                       (match_operand 3 "" "")] UNSPEC_XLOADQ0))]
  ""
  "xlo%3%X2.q0 %0 = %2"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached") (const_string "load_ext"))
     (if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached_x") (const_string "load_ext_x"))
     (if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached_y") (const_string "load_ext_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_xload1024q1"
  [(set (match_operand:X1024 0 "register_operand" "=x,x,x")
        (unspec:X1024 [(match_operand:X1024 1 "register_operand" "0,0,0")
                       (match_operand:<CHUNK> 2 "memory_operand" "a,b,m")
                       (match_operand 3 "" "")] UNSPEC_XLOADQ1))]
  ""
  "xlo%3%X2.q1 %0 = %2"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached") (const_string "load_ext"))
     (if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached_x") (const_string "load_ext_x"))
     (if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached_y") (const_string "load_ext_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_xload1024q2"
  [(set (match_operand:X1024 0 "register_operand" "=x,x,x")
        (unspec:X1024 [(match_operand:X1024 1 "register_operand" "0,0,0")
                       (match_operand:<CHUNK> 2 "memory_operand" "a,b,m")
                       (match_operand 3 "" "")] UNSPEC_XLOADQ2))]
  ""
  "xlo%3%X2.q2 %0 = %2"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached") (const_string "load_ext"))
     (if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached_x") (const_string "load_ext_x"))
     (if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached_y") (const_string "load_ext_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_xload1024q3"
  [(set (match_operand:X1024 0 "register_operand" "=x,x,x")
        (unspec:X1024 [(match_operand:X1024 1 "register_operand" "0,0,0")
                       (match_operand:<CHUNK> 2 "memory_operand" "a,b,m")
                       (match_operand 3 "" "")] UNSPEC_XLOADQ3))]
  ""
  "xlo%3%X2.q3 %0 = %2"
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached") (const_string "load_ext"))
     (if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached_x") (const_string "load_ext_x"))
     (if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached_y") (const_string "load_ext_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_xloadc1024q0"
  [(set (match_operand:X1024 0 "register_operand" "=x,x,x")
        (unspec:X1024 [(match_operand:X1024 1 "register_operand" "0,0,0")
                       (match_operand:<CHUNK> 2 "memsimple_operand" "c,d,e")
                       (match_operand:DI 3 "register_operand" "r,r,r")
                       (match_operand 4 "" "")
                       (match_operand 5 "" "")] UNSPEC_XLOADCQ0))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2.q0 %3? xlo%4 %0 = %2"
      : "xlo%4%5%X2.q0 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached") (const_string "load_ext"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached_x") (const_string "load_ext_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached_y") (const_string "load_ext_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_xloadc1024q1"
  [(set (match_operand:X1024 0 "register_operand" "=x,x,x")
        (unspec:X1024 [(match_operand:X1024 1 "register_operand" "0,0,0")
                       (match_operand:<CHUNK> 2 "memsimple_operand" "c,d,e")
                       (match_operand:DI 3 "register_operand" "r,r,r")
                       (match_operand 4 "" "")
                       (match_operand 5 "" "")] UNSPEC_XLOADCQ1))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2.q1 %3? xlo%4 %0 = %2"
      : "xlo%4%5%X2.q1 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached") (const_string "load_ext"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached_x") (const_string "load_ext_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached_y") (const_string "load_ext_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_xloadc1024q2"
  [(set (match_operand:X1024 0 "register_operand" "=x,x,x")
        (unspec:X1024 [(match_operand:X1024 1 "register_operand" "0,0,0")
                       (match_operand:<CHUNK> 2 "memsimple_operand" "c,d,e")
                       (match_operand:DI 3 "register_operand" "r,r,r")
                       (match_operand 4 "" "")
                       (match_operand 5 "" "")] UNSPEC_XLOADCQ2))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2.q2 %3? xlo%4 %0 = %2"
      : "xlo%4%5%X2.q2 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached") (const_string "load_ext"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached_x") (const_string "load_ext_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached_y") (const_string "load_ext_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_xloadc1024q3"
  [(set (match_operand:X1024 0 "register_operand" "=x,x,x")
        (unspec:X1024 [(match_operand:X1024 1 "register_operand" "0,0,0")
                       (match_operand:<CHUNK> 2 "memsimple_operand" "c,d,e")
                       (match_operand:DI 3 "register_operand" "r,r,r")
                       (match_operand 4 "" "")
                       (match_operand 5 "" "")] UNSPEC_XLOADCQ3))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2.q3 %3? xlo%4 %0 = %2"
      : "xlo%4%5%X2.q3 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached") (const_string "load_ext"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached_x") (const_string "load_ext_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached_y") (const_string "load_ext_y"))])
   (set_attr "length" "4, 8, 12")]
)


;; XSTORE1024Q*, XSTOREC1024Q*

(define_insn "lvx_xstore1024q0"
  [(set (match_operand:X256 1 "memory_operand"  "=a,b,m")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")] UNSPEC_XSTOREQ0))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "xso.q0%X1 %1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "length"       "4,          8,         12")]
)

(define_insn "lvx_xstore1024q1"
  [(set (match_operand:X256 1 "memory_operand"  "=a,b,m")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")] UNSPEC_XSTOREQ1))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "xso.q1%X1 %1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "length"       "4,          8,         12")]
)

(define_insn "lvx_xstore1024q2"
  [(set (match_operand:X256 1 "memory_operand"  "=a,b,m")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")] UNSPEC_XSTOREQ2))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "xso.q2%X1 %1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "length"       "4,          8,         12")]
)

(define_insn "lvx_xstore1024q3"
  [(set (match_operand:X256 1 "memory_operand"  "=a,b,m")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")] UNSPEC_XSTOREQ3))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "xso.q3%X1 %1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "length"       "4,          8,         12")]
)

(define_insn "lvx_xstorec1024q0"
  [(set (match_operand:X256 1 "memsimple_operand"  "=c,d,e")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")
                      (match_operand:DI 2 "register_operand" "r,r,r")
                      (match_operand 3 "" "")] UNSPEC_XSTORECQ0))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  "xso.q0%3%X1 %2? %O1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "length"       "4,          8,         12")]
)

(define_insn "lvx_xstorec1024q1"
  [(set (match_operand:X256 1 "memsimple_operand"  "=c,d,e")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")
                      (match_operand:DI 2 "register_operand" "r,r,r")
                      (match_operand 3 "" "")] UNSPEC_XSTORECQ1))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  "xso.q1%3%X1 %2? %O1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "length"       "4,          8,         12")]
)

(define_insn "lvx_xstorec1024q2"
  [(set (match_operand:X256 1 "memsimple_operand"  "=c,d,e")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")
                      (match_operand:DI 2 "register_operand" "r,r,r")
                      (match_operand 3 "" "")] UNSPEC_XSTORECQ2))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  "xso.q2%3%X1 %2? %O1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "length"       "4,          8,         12")]
)

(define_insn "lvx_xstorec1024q3"
  [(set (match_operand:X256 1 "memsimple_operand"  "=c,d,e")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")
                      (match_operand:DI 2 "register_operand" "r,r,r")
                      (match_operand 3 "" "")] UNSPEC_XSTORECQ3))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  "xso.q3%3%X1 %2? %O1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "length"       "4,          8,         12")]
)

;; XSPLATD

(define_insn "lvx_xsplatd256"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:DI 1 "register_operand" "r")] UNSPEC_XSPLATD))]
  ""
  "xputdq %0.lo = %1, %1\n\txputdq %0.hi = %1, %1";
  [(set_attr "type" "movet_ext")
   (set_attr "length"                   "8")]
)

(define_insn_and_split "lvx_xsplatd<bitsize>"
  [(set (match_operand:XBUFF 0 "register_operand" "=x")
        (unspec:XBUFF [(match_operand:DI 1 "register_operand" "r")] UNSPEC_XSPLATD))]
  "" "#" "reload_completed"
  [(const_int 0)]
  {
     lvx_split_tca_xsplatd (operands[0], operands[1]);
     DONE;
  }
)

;; XSPLATO

(define_insn_and_split "lvx_xsplato<bitsize>"
  [(set (match_operand:XBUFF 0 "register_operand" "=x")
        (unspec:XBUFF [(match_operand:<CHUNK> 1 "register_operand" "x")] UNSPEC_XSPLATO))]
  "" "#" "reload_completed"
  [(const_int 0)]
  {
    lvx_split_tca_xsplato (operands[0], operands[1]);
    DONE;
  }
)

;; XLOADC256, XLOADC512, XLOADC1024

(define_expand "lvx_xloadc256"
  [(match_operand:X256 0 "register_operand" "")
   (match_operand:X256 1 "reg_zero_mone_operand" "")
   (match_operand:X256 2 "memsimple_operand" "")
   (match_operand:DI 3 "register_operand" "")
   (match_operand 4 "" "")
   (match_operand 5 "" "")]
  ""
  {
    if (!const_zero_operand (operands[1], <MODE>mode))
      emit_insn (gen_lvx_xloadc_ (operands[0], operands[1], operands[2], operands[3], operands[4], operands[5]));
    else
      emit_insn (gen_lvx_xloadc__ (operands[0], operands[2], operands[3], operands[4], operands[5]));
    DONE;
  }
)

(define_insn "lvx_xloadc_"
  [(set (match_operand:X256 0 "register_operand" "=x,x,x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "0,0,0")
                      (match_operand:X256 2 "memsimple_operand" "c,d,e")
                      (match_operand:DI 3 "register_operand" "r,r,r")
                      (match_operand 4 "" "")
                      (match_operand 5 "" "")] UNSPEC_XLOADC))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%5%X2 %3? xlo%4 %0 = %2"
      : "xlo%4%5%X2 %3? %0 = %O2";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached") (const_string "load_ext"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached_x") (const_string "load_ext_x"))
     (if_then_else (match_operand 4 "uncached_modifier") (const_string "load_ext_uncached_y") (const_string "load_ext_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_insn "lvx_xloadc__"
  [(set (match_operand:X256 0 "register_operand" "=x,x,x")
        (unspec:X256 [(match_operand:X256 1 "memsimple_operand" "c,d,e")
                      (match_operand:DI 2 "register_operand" "r,r,r")
                      (match_operand 3 "" "")
                      (match_operand 4 "" "")] UNSPEC_XLOADC))]
  ""
  {
    return HAVE_LVX_PREDICATION
      ? "guard%4%X1 %2? xlo%3 %0 = %1"
      : "xlo%3%5%X1 %2? %0 = %O1";
  }
  [(set_attr_alternative "type"
    [(if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached") (const_string "load_ext"))
     (if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached_x") (const_string "load_ext_x"))
     (if_then_else (match_operand 3 "uncached_modifier") (const_string "load_ext_uncached_y") (const_string "load_ext_y"))])
   (set_attr "length" "4, 8, 12")]
)

(define_expand "lvx_xloadc512"
  [(match_operand:X512 0 "register_operand" "")
   (match_operand:X512 1 "reg_zero_mone_operand" "")
   (match_operand:X512 2 "memsimple_operand" "")
   (match_operand:DI 3 "register_operand" "")
   (match_operand 4 "" "")
   (match_operand 5 "" "")]
  ""
  {
    rtx masks[2];
    masks[0] = masks[1] = operands[3];
    if (masked_modifier (operands[4], VOIDmode))
      {
        masks[1] = gen_reg_rtx (DImode);
        emit_move_insn (masks[1], (gen_rtx_LSHIFTRT (DImode, operands[3], GEN_INT (32))));
      }
    if (!const_zero_operand (operands[1], <MODE>mode))
      for (int i = 0; i < 2; i++)
        {
          rtx opnd0 = simplify_gen_subreg (<CHUNK>mode, operands[0], <MODE>mode, i*32);
          rtx opnd1 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, i*32);
          rtx opnd2 = simplify_gen_subreg (<CHUNK>mode, operands[2], <MODE>mode, i*32);
          emit_insn (gen_lvx_xloadc_ (opnd0, opnd1, opnd2, masks[i], operands[4], operands[5]));
        }
    else
      for (int i = 0; i < 2; i++)
        {
          rtx opnd0 = simplify_gen_subreg (<CHUNK>mode, operands[0], <MODE>mode, i*32);
          rtx opnd2 = simplify_gen_subreg (<CHUNK>mode, operands[2], <MODE>mode, i*32);
          emit_insn (gen_lvx_xloadc__ (opnd0, opnd2, masks[i], operands[4], operands[5]));
        }
    DONE;
  }
)

(define_expand "lvx_xloadc1024"
  [(match_operand:X1024 0 "register_operand" "")
   (match_operand:X1024 1 "reg_zero_mone_operand" "")
   (match_operand:X1024 2 "memsimple_operand" "")
   (match_operand:TI 3 "register_operand" "")
   (match_operand 4 "" "")
   (match_operand 5 "" "")]
  ""
  {
    rtx masks[4];
    rtx mask = simplify_gen_subreg (DImode, operands[3], TImode, 0);
    masks[0] = masks[1] = masks[2] = masks[3] = mask;
    if (masked_modifier (operands[4], VOIDmode))
      {
        masks[1] = gen_reg_rtx (DImode);
        emit_move_insn (masks[1], (gen_rtx_LSHIFTRT (DImode, masks[0], GEN_INT (32))));
        masks[2] = simplify_gen_subreg (DImode, operands[3], TImode, 8);
        masks[3] = gen_reg_rtx (DImode);
        emit_move_insn (masks[3], (gen_rtx_LSHIFTRT (DImode, masks[2], GEN_INT (32))));
      }
    if (!const_zero_operand (operands[1], <MODE>mode))
      for (int i = 0; i < 4; i++)
        {
          rtx opnd0 = simplify_gen_subreg (<CHUNK>mode, operands[0], <MODE>mode, i*32);
          rtx opnd1 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, i*32);
          rtx opnd2 = simplify_gen_subreg (<CHUNK>mode, operands[2], <MODE>mode, i*32);
          emit_insn (gen_lvx_xloadc_ (opnd0, opnd1, opnd2, masks[i], operands[4], operands[5]));
        }
    else
      for (int i = 0; i < 4; i++)
        {
          rtx opnd0 = simplify_gen_subreg (<CHUNK>mode, operands[0], <MODE>mode, i*32);
          rtx opnd2 = simplify_gen_subreg (<CHUNK>mode, operands[2], <MODE>mode, i*32);
          emit_insn (gen_lvx_xloadc__ (opnd0, opnd2, masks[i], operands[4], operands[5]));
        }
    DONE;
  }
)


;; XSTORE256, XSTORE512, XSTORE1024

(define_insn "lvx_xstore256"
  [(set (match_operand:X256 1 "memory_operand"  "=a,b,m")
        (unspec:X256 [(match_operand:X256 0 "register_operand" "x,x,x")] UNSPEC_XSTORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "xso%X1 %1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "length"       "4,          8,         12")]
)

(define_insn_and_split "lvx_xstore512"
  [(set (match_operand:X512 1 "memsimple_operand" "=c,d,e")
        (unspec:X512 [(match_operand:X512 0 "register_operand" "x,x,x")] UNSPEC_XSTORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "#"
  "reload_completed"
  [(parallel
    [(set (subreg:<CHUNK> (match_dup 1) 0)
          (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 0) 0)] UNSPEC_XSTORE))
     (use (match_operand:SI 2 "nonmemory_operand" ""))])
   (parallel
    [(set (subreg:<CHUNK> (match_dup 1) 32)
          (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 0) 32)] UNSPEC_XSTORE))
     (use (match_operand:SI 2 "nonmemory_operand" ""))])]
  ""
)

(define_insn_and_split "lvx_xstore1024"
  [(set (match_operand:X1024 1 "memsimple_operand" "=c,d,e")
        (unspec:X1024 [(match_operand:X1024 0 "register_operand" "x,x,x")] UNSPEC_XSTORE))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "#"
  "reload_completed"
  [(parallel
    [(set (subreg:<CHUNK> (match_dup 1) 0)
          (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 0) 0)] UNSPEC_XSTORE))
     (use (match_operand:SI 2 "nonmemory_operand" ""))])
   (parallel
    [(set (subreg:<CHUNK> (match_dup 1) 32)
          (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 0) 32)] UNSPEC_XSTORE))
     (use (match_operand:SI 2 "nonmemory_operand" ""))])
   (parallel
    [(set (subreg:<CHUNK> (match_dup 1) 64)
          (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 0) 64)] UNSPEC_XSTORE))
     (use (match_operand:SI 2 "nonmemory_operand" ""))])
   (parallel
    [(set (subreg:<CHUNK> (match_dup 1) 96)
          (unspec:<CHUNK> [(subreg:<CHUNK> (match_dup 0) 96)] UNSPEC_XSTORE))
     (use (match_operand:SI 2 "nonmemory_operand" ""))])]
  ""
)


;; XSTOREC256, XSTOREC512, XSTOREC1024

(define_insn "lvx_xstorec256"
  [(set (match_operand:X256 1 "memsimple_operand"  "=c,d,e")
        (unspec:X256 [(match_operand:X256 0 "register_operand" "x,x,x")
                      (match_operand:DI 2 "register_operand" "r,r,r")
                      (match_operand 3 "" "")] UNSPEC_XSTOREC))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  "xso%3%X1 %2? %O1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "length"       "4,          8,         12")]
)

(define_expand "lvx_xstorec512"
  [(match_operand:X512 0 "register_operand" "")
   (match_operand:X512 1 "memsimple_operand" "")
   (match_operand:DI 2 "register_operand" "")
   (match_operand 3 "" "")
   (match_operand:SI 4 "nonmemory_operand" "")]
  ""
  {
    rtx masks[2];
    masks[0] = masks[1] = operands[2];
    if (masked_modifier (operands[3], VOIDmode))
      {
        masks[1] = gen_reg_rtx (DImode);
        emit_move_insn (masks[1], (gen_rtx_LSHIFTRT (DImode, operands[2], GEN_INT (32))));
      }
    for (int i = 0; i < 2; i++)
      {
        rtx opnd0 = simplify_gen_subreg (<CHUNK>mode, operands[0], <MODE>mode, i*32);
        rtx opnd1 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, i*32);
        emit_insn (gen_lvx_xstorec256 (opnd0, opnd1, masks[i], operands[3], operands[4]));
      }
    DONE;
  }
)

(define_expand "lvx_xstorec1024"
  [(match_operand:X1024 0 "register_operand" "")
   (match_operand:X1024 1 "memsimple_operand" "")
   (match_operand:TI 2 "register_operand" "")
   (match_operand 3 "" "")
   (match_operand:SI 4 "nonmemory_operand" "")]
  ""
  {
    rtx masks[4];
    rtx mask = simplify_gen_subreg (DImode, operands[2], TImode, 0);
    masks[0] = masks[1] = masks[2] = masks[3] = mask;
    if (masked_modifier (operands[3], VOIDmode))
      {
        masks[1] = gen_reg_rtx (DImode);
        emit_move_insn (masks[1], (gen_rtx_LSHIFTRT (DImode, masks[0], GEN_INT (32))));
        masks[2] = simplify_gen_subreg (DImode, operands[2], TImode, 8);
        masks[3] = gen_reg_rtx (DImode);
        emit_move_insn (masks[3], (gen_rtx_LSHIFTRT (DImode, masks[2], GEN_INT (32))));
      }
    for (int i = 0; i < 4; i++)
      {
        rtx opnd0 = simplify_gen_subreg (<CHUNK>mode, operands[0], <MODE>mode, i*32);
        rtx opnd1 = simplify_gen_subreg (<CHUNK>mode, operands[1], <MODE>mode, i*32);
        emit_insn (gen_lvx_xstorec256 (opnd0, opnd1, masks[i], operands[3], operands[4]));
      }
    DONE;
  }
)


;; XPRELOAD*

(define_insn "lvx_xpreloado<XBUFF:bitsize>"
  [(set (match_operand:XBUFF 0 "register_operand" "=x,x,x")
        (unspec:XBUFF [(match_operand:XBUFF 1 "register_operand" "0,0,0")
                       (match_operand:OI 2 "memsimple_operand" "c,d,e")
                       (match_operand:DI 3 "register_operand" "r,r,r")
                       (match_operand 4 "" "")] UNSPEC_XPRELOAD))]
  "HAVE_LVX_EXT_BUFFER_PRELOAD_<MODE>"
  "xplo%4%X2 %b0, %3 = %O2"
  [(set_attr "type" "preload,preload_x,preload_y")
   (set_attr "length" "4,   8,   12")]
)

(define_insn "lvx_xpreload<AI:lsusize><XBUFF:bitsize>"
  [(set (match_operand:XBUFF 0 "register_operand" "=x,x,x")
        (unspec:XBUFF [(match_operand:XBUFF 1 "register_operand" "0,0,0")
                       (match_operand:AI 2 "memsimple_operand" "c,d,e")
                       (match_operand:DI 3 "register_operand" "r,r,r")
                       (match_operand 4 "" "")] UNSPEC_XPRELOAD))]
  "HAVE_LVX_EXT_BUFFER_PRELOAD_<XBUFF:MODE>"
  "xpl<AI:lsusize>%4%X2 %b0, %3 = %O2"
  [(set_attr "type" "preload,preload_x,preload_y")
   (set_attr "length" "4,   8,   12")]
)


;; XALIGN*, XACCESS*

(define_insn "lvx_xaligno<XBUFF:bitsize>"
  [(set (match_operand:<CHUNK> 0 "register_operand" "=x")
        (unspec:<CHUNK> [(match_operand:XBUFF 1 "register_operand" "x")
                         (match_operand:DI 2 "register_operand" "r")] UNSPEC_XALIGN256))]
  "LVX_2 && (HAVE_LVX_EXT_BUFFER_MOV_EXT_<CHUNK>_<XBUFF:MODE>)"
  "xaligno %0 = %b1, %2"
  [(set_attr "type" "copy_ext")]
)

(define_insn "lvx_xaccesso<XBUFF:bitsize>"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (unspec:V256 [(match_operand:XBUFF 1 "register_operand" "x")
                      (match_operand:DI 2 "register_operand" "r")] UNSPEC_XACCESS256))]
  "LVX_2 && (HAVE_LVX_EXT_BUFFER_MOV_CORE_<V256:MODE>_<XBUFF:MODE>)"
  "xaccesso %0 = %b1, %2"
  [(set_attr "type" "movef_ext")]
)

;; XFSCALEWO

(define_insn "lvx_xfscalewo"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:DI 2 "register_operand" "r")
                      (match_operand 3 "" "")] UNSPEC_XFSCALEWO))]
  "LVX_2 && (HAVE_LVX_EXT_SCALE_V1OI)"
  "xfscalewo%3 %0 = %1, %2"
  [(set_attr "type" "copy_ext")]
)


;; XMMA484BW

(define_expand "lvx_xmma484bw"
  [(match_operand:X512 0 "register_operand" "")
   (match_operand:X256 1 "register_operand" "")
   (match_operand:X256 2 "register_operand" "")
   (match_operand:X512 3 "register_operand" "")
   (match_operand 4 "" "")]
  ""
  {
    const char *xstr = XSTR (operands[4], 0);
  if (HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x4_DI_M4x8_QI_TM8x4_QI)
      {
        if (!*xstr)
          emit_insn (gen_lvx_xmma484bw_1 (operands[0], operands[1], operands[2], operands[3]));
        else if (xstr[1] == 'u' && xstr[2] == 0)
          emit_insn (gen_lvx_xmmau484bw_1 (operands[0], operands[1], operands[2], operands[3]));
        else if (xstr[1] == 'u' && xstr[2] == 's')
          emit_insn (gen_lvx_xmmaus484bw_1 (operands[0], operands[1], operands[2], operands[3]));
        else if (xstr[1] == 's' && xstr[2] == 'u')
          emit_insn (gen_lvx_xmmasu484bw_1 (operands[0], operands[1], operands[2], operands[3]));
        else
          gcc_unreachable ();
      }
  else if (HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x8_QI_TM8x4_QI)
      {
        if (!*xstr)
          emit_insn (gen_lvx_xmma484bw_2 (operands[0], operands[1], operands[2], operands[3]));
        else if (xstr[1] == 'u' && xstr[2] == 0)
          emit_insn (gen_lvx_xmmau484bw_2 (operands[0], operands[1], operands[2], operands[3]));
        else if (xstr[1] == 'u' && xstr[2] == 's')
          emit_insn (gen_lvx_xmmaus484bw_2 (operands[0], operands[1], operands[2], operands[3]));
        else if (xstr[1] == 's' && xstr[2] == 'u')
          emit_insn (gen_lvx_xmmasu484bw_2 (operands[0], operands[1], operands[2], operands[3]));
        else
          gcc_unreachable ();
      }
    DONE;
  }
)

(define_insn "lvx_xmma484bw_1"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "x")] UNSPEC_XMMA484BW))]
  "HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x4_DI_M4x8_QI_TM8x4_QI"
  "xmma484bw %0 = %3, %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmmau484bw_1"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "x")] UNSPEC_XMMAU484BW))]
  "HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x4_DI_M4x8_UQI_TM8x4_UQI"
  "xmma484ubw %0 = %3, %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmmasu484bw_1"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "x")] UNSPEC_XMMASU484BW))]
  "HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x4_DI_M4x8_QI_TM8x4_UQI"
  "xmma484subw %0 = %3, %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmmaus484bw_1"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "x")] UNSPEC_XMMAUS484BW))]
  "HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x4_DI_M4x8_UQI_TM8x4_QI"
  "xmma484usbw %0 = %3, %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmma484bw_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMMA484BW))]
  "HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x8_QI_TM8x4_QI"
  "xmma484bw %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmmau484bw_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMMAU484BW))]
  "HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x8_UQI_TM8x4_UQI"
  "xmmau484bw %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmmasu484bw_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMMASU484BW))]
  "HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x8_QI_TM8x4_UQI"
  "xmmasu484bw %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmmaus484bw_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMMAUS484BW))]
  "HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x8_UQI_TM8x4_QI"
  "xmmaus484bw %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)


;; XMMA4164BW

(define_expand "lvx_xmma4164bw"
  [(match_operand:X512 0 "register_operand" "")
   (match_operand:X512 1 "register_operand" "")
   (match_operand:X512 2 "register_operand" "")
   (match_operand:X512 3 "register_operand" "")
   (match_operand 4 "" "")]
  "HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x16_QI_TM16x4_QI
  && HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x16_QI_TM16x4_UQI
  && HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x16_UQI_TM16x4_QI
  && HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x16_UQI_TM16x4_UQI"
  {
    const char *xstr = XSTR (operands[4], 0);
    if (!*xstr)
      emit_insn (gen_lvx_xmma4164bw_2 (operands[0], operands[1], operands[2], operands[3]));
    else if (xstr[1] == 'u' && xstr[2] == 0)
      emit_insn (gen_lvx_xmmau4164bw_2 (operands[0], operands[1], operands[2], operands[3]));
    else if (xstr[1] == 'u' && xstr[2] == 's')
      emit_insn (gen_lvx_xmmaus4164bw_2 (operands[0], operands[1], operands[2], operands[3]));
    else if (xstr[1] == 's' && xstr[2] == 'u')
      emit_insn (gen_lvx_xmmasu4164bw_2 (operands[0], operands[1], operands[2], operands[3]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_insn "lvx_xmma4164bw_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X512 1 "register_operand" "x")
                      (match_operand:X512 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMMA4164BW))]
  "HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x16_QI_TM16x4_QI"
  "xmma4164bw %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmmau4164bw_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X512 1 "register_operand" "x")
                      (match_operand:X512 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMMAU4164BW))]
  "HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x16_UQI_TM16x4_UQI"
  "xmmau4164bw %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmmasu4164bw_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X512 1 "register_operand" "x")
                      (match_operand:X512 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMMASU4164BW))]
  "HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x16_QI_TM16x4_UQI"
  "xmmasu4164bw %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmmaus4164bw_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X512 1 "register_operand" "x")
                      (match_operand:X512 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMMAUS4164BW))]
  "HAVE_LVX_EXT_MATRIX_FMA_M4x4_DI_M4x16_UQI_TM16x4_QI"
  "xmmaus4164bw %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)


;; XMADD44BW0, XMADD44BW1

(define_expand "lvx_xmadd44bw0"
  [(match_operand:X512 0 "register_operand" "")
   (match_operand:X256 1 "register_operand" "")
   (match_operand:X256 2 "register_operand" "")
   (match_operand:X512 3 "register_operand" "")
   (match_operand 4 "" "")]
  "HAVE_LVX_EXT_MATRIX_LO_HPA_M4x4_DI_M4x8_QI_M4x8_QI
  && HAVE_LVX_EXT_MATRIX_LO_HPA_M4x4_DI_M4x8_QI_M4x8_UQI
  && HAVE_LVX_EXT_MATRIX_LO_HPA_M4x4_DI_M4x8_UQI_M4x8_UQI"
  {
    const char *xstr = XSTR (operands[4], 0);
    if (!*xstr)
      emit_insn (gen_lvx_xmadd44bw0_2 (operands[0], operands[1], operands[2], operands[3]));
    else if (xstr[1] == 'u' && xstr[2] == 0)
      emit_insn (gen_lvx_xmaddu44bw0_2 (operands[0], operands[1], operands[2], operands[3]));
    else if (xstr[1] == 's' && xstr[2] == 'u')
      emit_insn (gen_lvx_xmaddsu44bw0_2 (operands[0], operands[1], operands[2], operands[3]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_insn "lvx_xmadd44bw0_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMADD44BW0))]
  "HAVE_LVX_EXT_MATRIX_LO_HPA_M4x4_DI_M4x8_QI_M4x8_QI"
  "xmadd44bw0 %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmaddu44bw0_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMADDU44BW0))]
  "HAVE_LVX_EXT_MATRIX_LO_HPA_M4x4_DI_M4x8_UQI_M4x8_UQI"
  "xmaddu44bw0 %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmaddsu44bw0_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMADDSU44BW0))]
  "HAVE_LVX_EXT_MATRIX_LO_HPA_M4x4_DI_M4x8_QI_M4x8_UQI"
  "xmaddsu44bw0 %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_expand "lvx_xmadd44bw1"
  [(match_operand:X512 0 "register_operand" "")
   (match_operand:X256 1 "register_operand" "")
   (match_operand:X256 2 "register_operand" "")
   (match_operand:X512 3 "register_operand" "")
   (match_operand 4 "" "")]
  "HAVE_LVX_EXT_MATRIX_HI_HPA_M4x4_DI_M4x8_QI_M4x8_QI
  && HAVE_LVX_EXT_MATRIX_HI_HPA_M4x4_DI_M4x8_QI_M4x8_UQI
  && HAVE_LVX_EXT_MATRIX_HI_HPA_M4x4_DI_M4x8_UQI_M4x8_UQI"
  {
    const char *xstr = XSTR (operands[4], 0);
    if (!*xstr)
      emit_insn (gen_lvx_xmadd44bw1_2 (operands[0], operands[1], operands[2], operands[3]));
    else if (xstr[1] == 'u' && xstr[2] == 0)
      emit_insn (gen_lvx_xmaddu44bw1_2 (operands[0], operands[1], operands[2], operands[3]));
    else if (xstr[1] == 's' && xstr[2] == 'u')
      emit_insn (gen_lvx_xmaddsu44bw1_2 (operands[0], operands[1], operands[2], operands[3]));
    else
      gcc_unreachable ();
    DONE;
  }
)

(define_insn "lvx_xmadd44bw1_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMADD44BW1))]
  "HAVE_LVX_EXT_MATRIX_HI_HPA_M4x4_DI_M4x8_QI_M4x8_QI"
  "xmadd44bw1 %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmaddu44bw1_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMADDU44BW1))]
  "HAVE_LVX_EXT_MATRIX_HI_HPA_M4x4_DI_M4x8_UQI_M4x8_UQI"
  "xmaddu44bw1 %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmaddsu44bw1_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")] UNSPEC_XMADDSU44BW1))]
  "HAVE_LVX_EXT_MATRIX_HI_HPA_M4x4_DI_M4x8_QI_M4x8_UQI"
  "xmaddsu44bw1 %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)


;; XMADDIFWO, XMSBFIFWO, XFFMA44HW

(define_insn "lvx_xmaddifwo"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X256 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_XMADDIFWO))]
  "HAVE_LVX_EXT_SATURATED_FMA_V8SI_V8SI_V8SF"
  "xmaddifwo%4 %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xmsbfifwo"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X256 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_XMSBFIFWO))]
  "HAVE_LVX_EXT_SATURATED_FMS_V8SI_V8SI_V8SF"
  "xmsbfifwo%4 %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xffma44hw"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_XFFMA44HW))]
  "HAVE_LVX_EXT_HPA_LO_M4x4_SF_M4x4_QF_M4x4_QF"
  "xffma44hw%4 %0 = %1, %2"
  [(set_attr "type" "ext_float")]
)


;; XFMMA444HW, XFMMA484HW

(define_insn "lvx_xfmma242hw0"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "x")] UNSPEC_XFMMA242HW0))]
  "HAVE_LVX_EXT_MATRIX_LO_LO_M2x2_DI_M2x2_DI_M2x4_HI_TM2x4_HI"
  "fmma242hw0 %0.lo = %3, %1, %2"
  [(set_attr "type" "ext_float")
   (set_attr "length" "6")])

(define_insn "lvx_xfmma242hw1"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "x")] UNSPEC_XFMMA242HW1))]
  "HAVE_LVX_EXT_MATRIX_LO_HI_M2x2_DI_M2x2_DI_M2x4_HI_TM2x4_HI"
  "fmma242hw1 %0.hi = %3, %1, %2"
  [(set_attr "type" "ext_float")
   (set_attr "length" "6")])

(define_insn "lvx_xfmma242hw2"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "x")] UNSPEC_XFMMA242HW2))]
  "HAVE_LVX_EXT_MATRIX_HI_LO_M2x2_DI_M2x2_DI_M2x4_HI_TM2x4_HI"
  "fmma242hw2 %0.lo = %3, %1, %2"
  [(set_attr "type" "ext_float")
   (set_attr "length" "12")])

(define_insn "lvx_xfmma242hw3"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "x")] UNSPEC_XFMMA242HW3))]
  "HAVE_LVX_EXT_MATRIX_HI_HI_M2x2_DI_M2x2_DI_M2x4_HI_TM2x4_HI"
  "fmma242hw3 %0.hi = %3, %1, %2"
  [(set_attr "type" "ext_float")
   (set_attr "length" "12")])

(define_insn_and_split "lvx_xfmma444hw"
  [(set (match_operand:X512 0 "register_operand" "=&x")
   (unspec:X512 [(match_operand:X256 1 "register_operand" "x")
                 (match_operand:X256 2 "register_operand" "x")
                 (match_operand:X512 3 "register_operand" "0")
                 (match_operand 4 "" "")] UNSPEC_XFMMA444HW))]
  "HAVE_LVX_EXT_MATRIX_M2x2_DI_M2x2_DI_M2x4_HI_TM2x4_HI"
  "#"
  "reload_completed"
  [(set (subreg:X256 (match_dup 0) 0)
        (unspec:X256 [(match_dup 1) (match_dup 2) (match_dup 3)] UNSPEC_XFMMA242HW0))
   (use (subreg:X256 (match_dup 0) 0))
   (set (subreg:X256 (match_dup 0) 0)
        (unspec:X256 [(match_dup 1) (match_dup 2) (match_dup 3)] UNSPEC_XFMMA242HW1))
   (set (subreg:X256 (match_dup 0) 32)
        (unspec:X256 [(match_dup 1) (match_dup 2) (match_dup 3)] UNSPEC_XFMMA242HW2))
   (use (subreg:X256 (match_dup 0) 32))
   (set (subreg:X256 (match_dup 0) 32)
        (unspec:X256 [(match_dup 1) (match_dup 2) (match_dup 3)] UNSPEC_XFMMA242HW3))]
  {
     operands[0] = simplify_subreg (V2OImode, operands[0], V2OImode, 0);
  })

(define_insn "lvx_xfmma484hw_2"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X512 1 "register_operand" "x")
                      (match_operand:X512 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_XFMMA484HW))]
  "HAVE_LVX_EXT_FMA_M4x4_DI_M4x8_HI_TM4x8_HI"
  "xfmma484hw%4 %0 = %1, %2"
  [(set_attr "type" "ext_float")])

(define_expand "lvx_xfmma484hw"
  [(match_operand:X512 0 "register_operand")
   (match_operand:X512 1 "register_operand")
   (match_operand:X512 2 "register_operand")
   (match_operand:X512 3 "register_operand")
   (match_operand 4 "" "")]
  ""
  {
    if (!HAVE_LVX_EXT_FMA_M4x4_DI_M4x8_HI_TM4x8_HI
        && (HAVE_LVX_EXT_MATRIX_M2x2_DI_M2x2_DI_M2x4_HI_TM2x4_HI))
      {
        rtx lo256_1 = gen_reg_rtx (<CHUNK>mode);
        rtx lo256_2 = gen_reg_rtx (<CHUNK>mode);
        rtx hi256_1 = gen_reg_rtx (<CHUNK>mode);
        rtx hi256_2 = gen_reg_rtx (<CHUNK>mode);
        emit_insn (gen_lvx_xlow256 (lo256_1, operands[1]));
        emit_insn (gen_lvx_xlow256 (lo256_2, operands[2]));
        emit_insn (gen_lvx_xhigh256 (hi256_1, operands[1]));
        emit_insn (gen_lvx_xhigh256 (hi256_2, operands[2]));
        rtx tmp = gen_reg_rtx (V2OImode);
        emit_insn (gen_lvx_xfmma444hw (tmp, lo256_1, lo256_2, operands[3], operands[4]));
        emit_insn (gen_lvx_xfmma444hw (operands[0], hi256_1, hi256_2, tmp, operands[4]));
      }
    else if (HAVE_LVX_EXT_FMA_M4x4_DI_M4x8_HI_TM4x8_HI)
      emit_insn (gen_lvx_xfmma484hw_2 (operands[0], operands[1], operands[2], operands[3], operands[4])) ;
    else
      gcc_unreachable ();

    DONE;
  })


;; XFNARROW44WH, XCLAMPWO

(define_insn "lvx_xfnarrow44wh"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X512 1 "register_operand" "x")
                      (match_operand 2 "" "")] UNSPEC_XFNARROW44WH))]
  "HAVE_LVX_EXT_CONVERT_M4x4_HF_M4x4_SF"
  "xfnarrow44wh%2 %0 = %1"
  [(set_attr "type" "ext_float")]
)

(define_insn "lvx_xclampwo"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")
                      (match_operand:X256 3 "register_operand" "0")] UNSPEC_XCLAMPWO))]
  "HAVE_LVX_EXT_CLAMP_V8SI_V8SI_V8SI"
  "xclampwo %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)


;; XTRUNC48WB, XSX48BW, XZX48BW

(define_insn "lvx_xtrunc48wb"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X1024 1 "register_operand" "x")] UNSPEC_XTRUNC48WB))]
  "HAVE_LVX_EXT_MATRIX_TRUNC_M4x8_QI_M4x8_DI"
  "xtrunc48wb %0 = %1"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xsx48bw"
  [(set (match_operand:X1024 0 "register_operand" "=x")
        (unspec:X1024 [(match_operand:X256 1 "register_operand" "x")] UNSPEC_XSX48BW))]
  "HAVE_LVX_EXT_MATRIX_SIGN_EXTEND_M4x8_DI_M4x8_QI"
  "xsx48bw %0 = %1"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xzx48bw"
  [(set (match_operand:X1024 0 "register_operand" "=x")
        (unspec:X1024 [(match_operand:X256 1 "register_operand" "x")] UNSPEC_XZX48BW))]
  "HAVE_LVX_EXT_MATRIX_ZERO_EXTEND_M4x8_DI_M4x8_QI"
  "xzx48bw %0 = %1"
  [(set_attr "type" "ext_int")]
)


;; XSENDO, XRECVO, XSENDRECVO

(define_insn "lvx_xsendo"
  [(unspec_volatile [(match_operand:X256 0 "register_operand" "x")
                     (match_operand 1 "" "")] UNSPEC_XSENDO)]
  "HAVE_LVX_EXT_SEND_V1OI"
  "xsendo%1 %0"
  [(set_attr "type" "alu_tiny_send")]
)

(define_insn "lvx_xrecvo"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec_volatile:X256 [(match_operand 1 "" "")] UNSPEC_XRECVO))]
  "HAVE_LVX_EXT_RECV_V1OI"
  "xrecvo%1 %0"
  [(set_attr "type" "alu_tiny_recv")]
)

(define_insn "lvx_xsendrecvo"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec_volatile:X256 [(match_operand:X256 1 "register_operand" "x")
                               (match_operand 2 "" "")] UNSPEC_XSENDRECVO))]
  "HAVE_LVX_EXT_SEND_RECV_V1OI"
  "xsendrecvo%2 %1, %0"
  [(set_attr "type" "alu_tiny_sendrecv")]
)


;; XSPLATOX, XSPLATOV

(define_insn "lvx_xsplato<suffix>"
  [(set (match_operand:XSPLATOM 0 "register_operand" "=x")
        (unspec:XSPLATOM [(match_operand:V1OI 1 "register_operand" "x")
                         (match_operand 2 "" "")] UNSPEC_XSPLATO))]
  "HAVE_LVX_EXT_SPLAT_<MODE>"
  "xsplato<suffix>%2 %0 = %1"
  [(set_attr "type" "ext_int")]
)

;; XCOPYO, XCOPYX, XCOPYV

(define_insn "lvx_xcopy<suffix>"
  [(set (match_operand:XCOPYM 0 "register_operand" "=x")
        (unspec:XCOPYM [(match_operand:XCOPYM 1 "register_operand" "x")
                        (match_operand 2 "" "")] UNSPEC_XCOPY))]
  "HAVE_LVX_EXT_COPY_<MODE>"
  "xcopy<suffix>%2 %0 = %1"
  [(set (attr "type")
        (if_then_else (match_test "GET_MODE (operands[0]) == V1OImode")
        (const_string "copy_ext")
        (const_string "ext_int")))]
)

;; XMT44D

(define_insn "lvx_xmt44d"
  [(set (match_operand:X1024 0 "register_operand" "=x")
        (unspec:X1024 [(match_operand:X1024 1 "register_operand" "x")] UNSPEC_XMT44D))]
  "LVX_2 && (HAVE_LVX_EXT_MATRIX_TRANSPOSE_M4x4_DI_M4x4_DI)"
  "xmt44d %0 = %1"
  [(set_attr "type" "ext_int")]
)


;; XANDO, XNANDO, XANDNO, XIORO, XNIORO, XIORNO, XEORO, XNEORO, XSBMM8DQ, XSBMMT8DQ

(define_insn "lvx_xando"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (and:X256 (match_operand:X256 1 "register_operand" "x")
                  (match_operand:X256 2 "register_operand" "x")))]
  "HAVE_LVX_EXT_AND_<MODE>"
  "xando %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xnando"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (ior:X256 (not:X256 (match_operand:X256 1 "register_operand" "x"))
                  (not:X256 (match_operand:X256 2 "register_operand" "x"))))]
  "HAVE_LVX_EXT_NAND_<MODE>"
  "xnando %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xandno"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (and:X256 (not:X256 (match_operand:X256 1 "register_operand" "x"))
                  (match_operand:X256 2 "register_operand" "x")))]
  "HAVE_LVX_EXT_ANDN_<MODE>"
  "xandno %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xioro"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (ior:X256 (match_operand:X256 1 "register_operand" "x")
                  (match_operand:X256 2 "register_operand" "x")))]
  "HAVE_LVX_EXT_IOR_<MODE>"
  "xioro %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xnioro"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (and:X256 (not:X256 (match_operand:X256 1 "register_operand" "x"))
                  (not:X256 (match_operand:X256 2 "register_operand" "x"))))]
  "HAVE_LVX_EXT_NIOR_<MODE>"
  "xnioro %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xiorno"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (ior:X256 (not:X256 (match_operand:X256 1 "register_operand" "x"))
                  (match_operand:X256 2 "register_operand" "x")))]
  "HAVE_LVX_EXT_IORN_<MODE>"
  "xiorno %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xeoro"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (xor:X256 (match_operand:X256 1 "register_operand" "x")
                  (match_operand:X256 2 "register_operand" "x")))]
  "HAVE_LVX_EXT_EOR_<MODE>"
  "xeoro %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xneoro"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (not:X256 (xor:X256 (match_operand:X256 1 "register_operand" "x")
                            (match_operand:X256 2 "register_operand" "x"))))]
  "HAVE_LVX_EXT_NEOR_<MODE>"
  "xneoro %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xsbmm8dq"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")] UNSPEC_SBMM8D))]
  "HAVE_LVX_EXT_SBMM8_V1OI"
  "xsbmm8dq %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)

(define_insn "lvx_xsbmmt8dq"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")] UNSPEC_SBMMT8D))]
  "HAVE_LVX_EXT_SBMMT8_V1OI"
  "xsbmmt8dq %0 = %1, %2"
  [(set_attr "type" "ext_int")]
)


;; XSWAP256
;; Those can't be done with a define_insn_and_split because the bundle
;; semantics is not understood by gcc.

(define_expand "lvx_xswapo256"
  [(match_operand:V256 0 "register_operand" "")
   (match_operand:X256 1 "memory_operand" "")
   (match_operand:V256 2 "register_operand" "")]
  ""
  {
    rtx swapped = force_reg (<X256:MODE>mode, operands[1]);
    emit_insn (gen_lvx_xswapo256v4di_ (operands[0], swapped, operands[2]));
    emit_move_insn (operands[1], swapped);
    DONE;
  }
)

(define_expand "lvx_xswapq256"
  [(match_operand:V2DI 0 "register_operand" "")
   (match_operand:X256 1 "memory_operand" "")
   (match_operand:V2DI 2 "register_operand" "")
   (match_operand 3 "" "")]
  ""
  {
    rtx swapped = force_reg (<X256:MODE>mode, operands[1]);
    emit_insn (gen_lvx_xswapq256v2di_ (operands[0], swapped, operands[2], operands[3]));
    emit_move_insn (operands[1], swapped);
    DONE;
  }
)

(define_insn "lvx_xswapo256<ALL256:mode>_"
  [(set (match_operand:ALL256 0 "register_operand" "=r")
        (unspec:ALL256 [(match_operand:X256 1 "register_operand" "+x")] UNSPEC_XSWAP256))
   (set (match_dup 1)
        (unspec:X256 [(match_operand:ALL256 2 "register_operand" "0")] UNSPEC_XSWAP256))]
  "LVX_2"
  "xmovefo %0 = %1\n\txputdq %1.lo = %x2, %y2\n\txputdq %1.hi = %z2, %t2"
  [(set_attr "type" "all")
   (set_attr "length" "12")]
)

(define_insn "lvx_xswapq256<ALL128:mode>_"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (unspec:ALL128 [(match_operand:X256 1 "register_operand" "+x")] UNSPEC_XSWAP256))
   (set (match_dup 1)
        (unspec:X256 [(match_operand:ALL128 2 "register_operand" "0")
                      (match_operand 3 "" "")] UNSPEC_XSWAP256))]
  "LVX_2 && (HAVE_LVX_MOV_FROM_CORE_V2DI_TO_EXT_V1OI)"
  "xmovefq %0 = %1%3\n\txputdq %1%3 = %x2, %y2"
  [(set_attr "type" "all")
   (set_attr "length" "8")]
)


;; XFMMA444W_0, XFMMA444W_1, XFMMA424D_0, XFMMA424D_1

(define_insn "lvx_xfmma444w_0"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X512 1 "register_operand" "x")
                      (match_operand:X512 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_XFMMA444W_0))]
  "HAVE_LVX_EXT_MATRIX444_FMA_V2OI"
  "xfmma444w.0%4 %0 = %1, %2"
  [(set_attr "type" "ext_float")]
)

(define_insn "lvx_xfmma444w_1"
  [(set (match_operand:X512 0 "register_operand" "=x")
        (unspec:X512 [(match_operand:X512 1 "register_operand" "x")
                      (match_operand:X512 2 "register_operand" "x")
                      (match_operand:X512 3 "register_operand" "0")
                      (match_operand 4 "" "")] UNSPEC_XFMMA444W_1))]
  "LVX_2 && (HAVE_LVX_EXT_MATRIX444_FMA_V2OI)"
  "xfmma444w.1%4 %0 = %1, %2"
  [(set_attr "type" "ext_float")]
)

(define_insn "lvx_xfmma424d_0"
  [(set (match_operand:X1024 0 "register_operand" "=x")
        (unspec:X1024 [(match_operand:X512 1 "register_operand" "x")
                       (match_operand:X512 2 "register_operand" "x")
                       (match_operand:X1024 3 "register_operand" "0")
                       (match_operand 4 "" "")] UNSPEC_XFMMA424D_0))]
  "HAVE_LVX_EXT_MATRIX424_FMA_V2OI"
  "xfmma424d.0%4 %0 = %1, %2"
  [(set_attr "type" "ext_float")]
)

(define_insn "lvx_xfmma424d_1"
  [(set (match_operand:X1024 0 "register_operand" "=x")
        (unspec:X1024 [(match_operand:X512 1 "register_operand" "x")
                       (match_operand:X512 2 "register_operand" "x")
                       (match_operand:X1024 3 "register_operand" "0")
                       (match_operand 4 "" "")] UNSPEC_XFMMA424D_1))]
  "HAVE_LVX_EXT_MATRIX424_FMA_V2OI"
  "xfmma424d.1%4 %0 = %1, %2"
  [(set_attr "type" "ext_float")]
)

;; XFMIN/XFMAX

(define_insn "lvx_xfminhx"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (smin:X256 (match_operand:X256 1 "register_operand" "x")
                  (match_operand:X256 2 "register_operand" "x")))]
  "HAVE_LVX_EXT_MIN_X256"
  "xfminhx %0 = %1, %2"
  [(set_attr "type" "ext_float")]
)

(define_insn "lvx_xfmaxhx"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (smax:X256 (match_operand:X256 1 "register_operand" "x")
                  (match_operand:X256 2 "register_operand" "x")))]
  "HAVE_LVX_EXT_MAX_X256"
  "xfmaxhx %0 = %1, %2"
  [(set_attr "type" "ext_float")]
)
