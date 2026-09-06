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


;; 256-bit Extension Moves

(define_insn "lvx_xmovefo"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (unspec:V256 [(match_operand:X256 1 "register_operand" "x")]
                     UNSPEC_XMOVEF))]
   "LVX_2 && (HAVE_LVX_MOV_FROM_CORE_V4DI_TO_EXT_V1OI)"
   "xmovefo %0 = %1"
   [(set_attr "type" "movef_ext")
    (set_attr "issue" "ext_misc_auxw")]
)

(define_insn "lvx_xmovefq"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:X256 1 "register_operand" "x")
                      (match_operand 2 "" "")]
                     UNSPEC_XMOVEF))]
   "LVX_2 && (HAVE_LVX_MOV_FROM_CORE_V2DI_TO_EXT_V1OI)"
   "xmovefq %0 = %1%2"
   [(set_attr "type" "movef_ext")
    (set_attr "issue" "ext_misc_auxw")]
)

(define_insn "lvx_xmovefd"
  [(set (match_operand:DI 0 "register_operand" "=r")
        (unspec:DI [(match_operand:X256 1 "register_operand" "x")
                    (match_operand 2 "" "")]
                   UNSPEC_XMOVEF))]
   "LVX_2 && (HAVE_LVX_MOV_FROM_CORE_V1DI_TO_EXT_V1OI)"
   "xmovefd %0 = %1%2"
   [(set_attr "type" "movef_ext")
    (set_attr "issue" "ext_misc_auxw")]
)

(define_insn_and_split "lvx_xputqo"
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
   (set_attr "issue" "x2_alu_lite_misc")
   (set_attr "length" "8")]
)

(define_insn "lvx_xputdq"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "0")
                      (match_operand:V2DI 2 "register_operand" "r")
                      (match_operand 3 "" "")]
                     UNSPEC_XMOVET))]
  "HAVE_LVX_MOV_FROM_EXT_V1OI_TO_CORE_V2DI"
  "xputdq %0%3 = %x2, %y2"
  [(set_attr "type" "alu_lite_recv")
   (set_attr "issue" "alu_lite_misc")
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
   (set_attr "issue" "alu_lite_misc")
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
   (set_attr "issue" "ext_misc_auxw, lsu, lsu_x, lsu_y, lsu_memw_accr, lsu_memw_accr_x, lsu_memw_accr_y, ext_misc_auxw, x2_alu_lite_misc, lsu_auxr_auxw")
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
   (set_attr "issue" "ext_misc_auxw, lsu, lsu_x, lsu_y, lsu, lsu_x, lsu_y, lsu_memw_accr, lsu_memw_accr_x, lsu_memw_accr_y, ext_misc_auxw, x2_alu_lite_misc, lsu_auxr_auxw")
   (set_attr "length"                "4,       4,         8,        12,                4,                  8,                 12,             4,               8,              12,                 4,                    8,            4")]
)

(define_insn "*xmovef<mode>"
  [(set (match_operand:ALL256X 0 "register_operand" "=r")
        (match_operand:ALL256X 1 "register_operand" "x"))]
  "LVX_2"
  "xmovefo %0 = %1"
  [(set_attr "type" "movef_ext")
   (set_attr "issue" "ext_misc_auxw")
   (set_attr "length" "4")]
)

(define_insn "*xmovet<mode>_lo"
  [(set (match_operand:ALL128 0 "register_operand" "=x")
        (unspec:ALL128 [(match_operand:ALL128 1 "register_operand" "r")]
                       UNSPEC_XMOVET_LO))]
  ""
  "xputdq %0.lo = %x1, %y1"
  [(set_attr "type" "movet_ext_hi")
   (set_attr "issue" "alu_lite_misc")
   (set_attr "length" "4")]
)

(define_insn "*xmovet<mode>_hi"
  [(set (match_operand:ALL128 0 "register_operand" "=x")
        (unspec:ALL128 [(match_operand:ALL128 1 "register_operand" "r")]
                       UNSPEC_XMOVET_HI))]
  ""
  "xputdq %0.hi = %x1, %y1"
  [(set_attr "type" "movet_ext_lo")
   (set_attr "issue" "alu_lite_misc")
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
  "guard%5%X2.q0 %3? xlo%4 %0 = %2"
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
  "guard%5%X2.q1 %3? xlo%4 %0 = %2"
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
  "guard%5%X2.q2 %3? xlo%4 %0 = %2"
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
  "guard%5%X2.q3 %3? xlo%4 %0 = %2"
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
   (set_attr "issue" "lsu_memw_accr, lsu_memw_accr_x, lsu_memw_accr_y")
   (set_attr "length"       "4,          8,         12")]
)

(define_insn "lvx_xstore1024q1"
  [(set (match_operand:X256 1 "memory_operand"  "=a,b,m")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")] UNSPEC_XSTOREQ1))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "xso.q1%X1 %1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "issue" "lsu_memw_accr, lsu_memw_accr_x, lsu_memw_accr_y")
   (set_attr "length"       "4,          8,         12")]
)

(define_insn "lvx_xstore1024q2"
  [(set (match_operand:X256 1 "memory_operand"  "=a,b,m")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")] UNSPEC_XSTOREQ2))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "xso.q2%X1 %1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "issue" "lsu_memw_accr, lsu_memw_accr_x, lsu_memw_accr_y")
   (set_attr "length"       "4,          8,         12")]
)

(define_insn "lvx_xstore1024q3"
  [(set (match_operand:X256 1 "memory_operand"  "=a,b,m")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")] UNSPEC_XSTOREQ3))
   (use (match_operand:SI 2 "nonmemory_operand" ""))]
  ""
  "xso.q3%X1 %1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "issue" "lsu_memw_accr, lsu_memw_accr_x, lsu_memw_accr_y")
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
  "guard%3 %2? xso.q0%X1 %1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "issue" "lsu_memw_accr, lsu_memw_accr_x, lsu_memw_accr_y")
   (set_attr "bcu_used" "yes")
   (set_attr "length"       "8,         12,         16")]
)

(define_insn "lvx_xstorec1024q1"
  [(set (match_operand:X256 1 "memsimple_operand"  "=c,d,e")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")
                      (match_operand:DI 2 "register_operand" "r,r,r")
                      (match_operand 3 "" "")] UNSPEC_XSTORECQ1))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  "guard%3 %2? xso.q1%X1 %1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "issue" "lsu_memw_accr, lsu_memw_accr_x, lsu_memw_accr_y")
   (set_attr "bcu_used" "yes")
   (set_attr "length"       "8,         12,         16")]
)

(define_insn "lvx_xstorec1024q2"
  [(set (match_operand:X256 1 "memsimple_operand"  "=c,d,e")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")
                      (match_operand:DI 2 "register_operand" "r,r,r")
                      (match_operand 3 "" "")] UNSPEC_XSTORECQ2))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  "guard%3 %2? xso.q2%X1 %1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "issue" "lsu_memw_accr, lsu_memw_accr_x, lsu_memw_accr_y")
   (set_attr "bcu_used" "yes")
   (set_attr "length"       "8,         12,         16")]
)

(define_insn "lvx_xstorec1024q3"
  [(set (match_operand:X256 1 "memsimple_operand"  "=c,d,e")
        (unspec:X256 [(match_operand:X1024 0 "register_operand" "x,x,x")
                      (match_operand:DI 2 "register_operand" "r,r,r")
                      (match_operand 3 "" "")] UNSPEC_XSTORECQ3))
   (use (match_operand:SI 4 "nonmemory_operand" ""))
   (clobber (match_dup 1))]
  ""
  "guard%3 %2? xso.q3%X1 %1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "issue" "lsu_memw_accr, lsu_memw_accr_x, lsu_memw_accr_y")
   (set_attr "bcu_used" "yes")
   (set_attr "length"       "8,         12,         16")]
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
  "guard%5%X2 %3? xlo%4 %0 = %2"
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
  "guard%4%X1 %2? xlo%3 %0 = %1"
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
   (set_attr "issue" "lsu_memw_accr, lsu_memw_accr_x, lsu_memw_accr_y")
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
  "guard%3 %2? xso%X1 %1 = %0"
  [(set_attr "type" "store_ext,store_ext_x,store_ext_y")
   (set_attr "issue" "lsu_memw_accr, lsu_memw_accr_x, lsu_memw_accr_y")
   (set_attr "bcu_used" "yes")
   (set_attr "length"       "8,         12,         16")]
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
   (set_attr "issue" "lsu, lsu_x, lsu_y")
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
   (set_attr "issue" "lsu, lsu_x, lsu_y")
   (set_attr "length" "4,   8,   12")]
)


;; XALIGN*, XACCESS*

(define_insn "lvx_xaligno<XBUFF:bitsize>"
  [(set (match_operand:<CHUNK> 0 "register_operand" "=x")
        (unspec:<CHUNK> [(match_operand:XBUFF 1 "register_operand" "x")
                         (match_operand:DI 2 "register_operand" "r")] UNSPEC_XALIGN256))]
  "LVX_2 && (HAVE_LVX_EXT_BUFFER_MOV_EXT_<CHUNK>_<XBUFF:MODE>)"
  "xaligno %0 = %b1, %2"
  [(set_attr "type" "copy_ext")
   (set_attr "issue" "ext_misc_auxw")]
)

(define_insn "lvx_xaccesso<XBUFF:bitsize>"
  [(set (match_operand:V256 0 "register_operand" "=r")
        (unspec:V256 [(match_operand:XBUFF 1 "register_operand" "x")
                      (match_operand:DI 2 "register_operand" "r")] UNSPEC_XACCESS256))]
  "LVX_2 && (HAVE_LVX_EXT_BUFFER_MOV_CORE_<V256:MODE>_<XBUFF:MODE>)"
  "xaccesso %0 = %b1, %2"
  [(set_attr "type" "movef_ext")
   (set_attr "issue" "ext_misc_auxw")]
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

;; XANDO, XNANDO, XANDNO, XIORO, XNIORO, XIORNO, XEORO, XNEORO, XSBMM8DQ, XSBMMT8DQ

(define_insn "lvx_xando"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (and:X256 (match_operand:X256 1 "register_operand" "x")
                  (match_operand:X256 2 "register_operand" "x")))]
  "HAVE_LVX_EXT_AND_<MODE>"
  "xando %0 = %1, %2"
  [(set_attr "type" "ext_int")
   (set_attr "issue" "ext")]
)

(define_insn "lvx_xnando"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (ior:X256 (not:X256 (match_operand:X256 1 "register_operand" "x"))
                  (not:X256 (match_operand:X256 2 "register_operand" "x"))))]
  "HAVE_LVX_EXT_NAND_<MODE>"
  "xnando %0 = %1, %2"
  [(set_attr "type" "ext_int")
   (set_attr "issue" "ext")]
)

(define_insn "lvx_xandno"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (and:X256 (not:X256 (match_operand:X256 1 "register_operand" "x"))
                  (match_operand:X256 2 "register_operand" "x")))]
  "HAVE_LVX_EXT_ANDN_<MODE>"
  "xandno %0 = %1, %2"
  [(set_attr "type" "ext_int")
   (set_attr "issue" "ext")]
)

(define_insn "lvx_xioro"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (ior:X256 (match_operand:X256 1 "register_operand" "x")
                  (match_operand:X256 2 "register_operand" "x")))]
  "HAVE_LVX_EXT_IOR_<MODE>"
  "xioro %0 = %1, %2"
  [(set_attr "type" "ext_int")
   (set_attr "issue" "ext")]
)

(define_insn "lvx_xnioro"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (and:X256 (not:X256 (match_operand:X256 1 "register_operand" "x"))
                  (not:X256 (match_operand:X256 2 "register_operand" "x"))))]
  "HAVE_LVX_EXT_NIOR_<MODE>"
  "xnioro %0 = %1, %2"
  [(set_attr "type" "ext_int")
   (set_attr "issue" "ext")]
)

(define_insn "lvx_xiorno"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (ior:X256 (not:X256 (match_operand:X256 1 "register_operand" "x"))
                  (match_operand:X256 2 "register_operand" "x")))]
  "HAVE_LVX_EXT_IORN_<MODE>"
  "xiorno %0 = %1, %2"
  [(set_attr "type" "ext_int")
   (set_attr "issue" "ext")]
)

(define_insn "lvx_xeoro"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (xor:X256 (match_operand:X256 1 "register_operand" "x")
                  (match_operand:X256 2 "register_operand" "x")))]
  "HAVE_LVX_EXT_EOR_<MODE>"
  "xeoro %0 = %1, %2"
  [(set_attr "type" "ext_int")
   (set_attr "issue" "ext")]
)

(define_insn "lvx_xneoro"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (not:X256 (xor:X256 (match_operand:X256 1 "register_operand" "x")
                            (match_operand:X256 2 "register_operand" "x"))))]
  "HAVE_LVX_EXT_NEOR_<MODE>"
  "xneoro %0 = %1, %2"
  [(set_attr "type" "ext_int")
   (set_attr "issue" "ext")]
)

(define_insn "lvx_xsbmm8dq"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")] UNSPEC_SBMM8D))]
  "HAVE_LVX_EXT_SBMM8_V1OI"
  "xsbmm8dq %0 = %1, %2"
  [(set_attr "type" "ext_int")
   (set_attr "issue" "ext")]
)

(define_insn "lvx_xsbmmt8dq"
  [(set (match_operand:X256 0 "register_operand" "=x")
        (unspec:X256 [(match_operand:X256 1 "register_operand" "x")
                      (match_operand:X256 2 "register_operand" "x")] UNSPEC_SBMMT8D))]
  "HAVE_LVX_EXT_SBMMT8_V1OI"
  "xsbmmt8dq %0 = %1, %2"
  [(set_attr "type" "ext_int")
   (set_attr "issue" "ext")]
)
