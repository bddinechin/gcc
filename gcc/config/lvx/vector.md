


;; 128-bit Vector Moves

(define_expand "mov<mode>"
  [(set (match_operand:ALL128 0 "nonimmediate_operand" "")
        (match_operand:ALL128 1 "general_operand" ""))]
  ""
  {
    bool misaligned_0 = !lvx_hardreg_aligned_p (operands[0], <MODE>mode);
    bool misaligned_1 = !lvx_hardreg_aligned_p (operands[1], <MODE>mode);
    if ((misaligned_0 && MEM_P (operands[1])) || (misaligned_1 && MEM_P (operands[0])))
      {
        rtx temp = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (temp, operands[1]));
        emit_insn (gen_rtx_SET (operands[0], temp));
        DONE;
      }

    if (MEM_P (operands[0]))
      operands[1] = force_reg (<MODE>mode, operands[1]);
  }
)

(define_insn_and_split "*copy<mode>"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (match_operand:ALL128 1 "register_operand" "r"))]
  "!lvx_hardreg_aligned_p (operands[0], <MODE>mode) || !lvx_hardreg_aligned_p (operands[1], <MODE>mode)"
  "#"
  "&& reload_completed"
  [(use (const_int 0))]
  {
    lvx_split_128bits_move (operands[0], operands[1]);
    DONE;
  }
)

(define_insn "*mov<mode>"
  [(set (match_operand:ALL128 0 "nonimmediate_operand" "=r, r, r, r, r, r, r,a,b,m")
        (match_operand:ALL128 1 "nonimmediate_operand"  "r,Ca,Cb,Cm,Za,Zb,Zm,r,r,r"))]
  "lvx_hardreg_aligned_p (operands[0], <MODE>mode) && lvx_hardreg_aligned_p (operands[1], <MODE>mode)"
  {
    switch (which_alternative)
      {
      case 0:
        return "#";
      case 1: case 2: case 3: case 4: case 5: case 6:
        return "lq%V1 %0 = %1";
      case 7: case 8: case 9:
        return "sq%X0 %0 = %1";
      default:
        gcc_unreachable ();
      }
  }
  [(set_attr "type" "alu_tiny_x2,load_core,load_core_x,load_core_y,load_core_uncached,load_core_uncached_x,load_core_uncached_y,store_core,store_core_x,store_core_y")
   (set_attr "length"         "8,             4,             8,             12,                     4,                       8,                      12,             4,               8,              12")]
)

(define_split
  [(set (match_operand:ALL128 0 "register_operand" "")
        (match_operand:ALL128 1 "register_operand" ""))]
  "reload_completed"
  [(use (const_int 0))]
  {
    lvx_split_128bits_move (operands[0], operands[1]);
    DONE;
  }
)

(define_insn_and_split "*make<mode>"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (match_operand:ALL128 1 "immediate_operand" "i"))]
  ""
  "#"
  "reload_completed"
  [(use (const_int 0))]
  {
    lvx_make_128bit_const (operands[0], operands[1]);
    DONE;
  }
)


;; 256-bit Vector Moves

(define_expand "mov<mode>"
  [(set (match_operand:ALL256 0 "nonimmediate_operand" "")
        (match_operand:ALL256 1 "general_operand" ""))]
  ""
  {
    bool misaligned_0 = !lvx_hardreg_aligned_p (operands[0], <MODE>mode);
    bool misaligned_1 = !lvx_hardreg_aligned_p (operands[1], <MODE>mode);
    if ((misaligned_0 && MEM_P (operands[1])) || (misaligned_1 && MEM_P (operands[0])))
      {
        rtx temp = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (temp, operands[1]));
        emit_insn (gen_rtx_SET (operands[0], temp));
        DONE;
      }

    if (MEM_P (operands[0]))
      operands[1] = force_reg (<MODE>mode, operands[1]);
  }
)

(define_insn_and_split "*copy<mode>"
  [(set (match_operand:ALL256 0 "register_operand" "=r")
        (match_operand:ALL256 1 "register_operand" "r"))]
  "!lvx_hardreg_aligned_p (operands[0], <MODE>mode) || !lvx_hardreg_aligned_p (operands[1], <MODE>mode)"
  "#"
  "&& reload_completed"
  [(use (const_int 0))]
  {
    lvx_split_256bits_move (operands[0], operands[1]);
    DONE;
  }
)

(define_insn "*mov<mode>"
  [(set (match_operand:ALL256 0 "nonimmediate_operand" "=r, r, r, r, r, r, r,a,b,m")
        (match_operand:ALL256 1 "nonimmediate_operand"  "r,Ca,Cb,Cm,Za,Zb,Zm,r,r,r"))]
  "lvx_hardreg_aligned_p (operands[0], <MODE>mode) && lvx_hardreg_aligned_p (operands[1], <MODE>mode)"
  {
    switch (which_alternative)
      {
      case 0:
        return "#";
      case 1: case 2: case 3: case 4: case 5: case 6:
        return "lo%V1 %0 = %1";
      case 7: case 8: case 9:
        return "so%X0 %0 = %1";
      default:
        gcc_unreachable ();
      }
  }
  [(set_attr "type" "alu_tiny_x4,load_core,load_core_x,load_core_y,load_core_uncached,load_core_uncached_x,load_core_uncached_y,store_core,store_core_x,store_core_y")
   (set_attr "length"        "16,            4,              8,             12,                     4,                       8,                      12,             4,               8,              12")]
)

(define_split
  [(set (match_operand:ALL256 0 "register_operand" "")
        (match_operand:ALL256 1 "register_operand" ""))]
  "reload_completed"
  [(use (const_int 0))]
  {
    lvx_split_256bits_move (operands[0], operands[1]);
    DONE;
  }
)

(define_insn_and_split "*make<mode>"
    [(set (match_operand:ALL256 0 "register_operand" "=r")
          (match_operand:ALL256 1 "immediate_operand" "i"))]
  ""
  "#"
  "reload_completed"
  [(use (const_int 0))]
  {
    lvx_make_256bit_const (operands[0], operands[1]);
    DONE;
  }
)


;; 512-bit Vector Moves

(define_expand "mov<mode>"
  [(set (match_operand:ALL512 0 "nonimmediate_operand" "")
        (match_operand:ALL512 1 "general_operand" ""))]
  ""
  {
    if (MEM_P (operands[0]))
      operands[1] = force_reg (<MODE>mode, operands[1]);
  }
)

(define_insn_and_split "*mov<mode>"
  [(set (match_operand:ALL512 0 "nonimmediate_operand" "=&r,&r,&r,&r,a,b,m")
        (match_operand:ALL512 1 "nonimmediate_operand"   "r, a, b, m,r,r,r"))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0) (subreg:<HALF> (match_dup 1) 0))
   (set (subreg:<HALF> (match_dup 0) 32) (subreg:<HALF> (match_dup 1) 32))]
  {
  }
)

(define_insn_and_split "*make<mode>"
    [(set (match_operand:ALL512 0 "register_operand" "=r")
          (match_operand:ALL512 1 "immediate_operand" "i"))]
  ""
  "#"
  "reload_completed"
  [(use (const_int 0))]
  {
    lvx_make_512bit_const (operands[0], operands[1]);
    DONE;
  }
)


;; Vector Set/Extract/Init/Perm/Shr

(define_expand "vec_set<mode>"
  [(match_operand:SIMDALL 0 "register_operand" "")
   (match_operand:<INNER> 1 "register_operand" "")
   (match_operand 2 "const_int_operand" "")]
  ""
  {
    rtx target = operands[0];
    rtx source = operands[1];
    rtx where = operands[2];
    lvx_expand_vector_insert (target, source, where);
    DONE;
  }
)

(define_expand "vec_extract<mode><inner>"
  [(match_operand:<INNER> 0 "register_operand" "")
   (match_operand:SIMDALL 1 "register_operand" "")
   (match_operand 2 "const_int_operand" "")]
  ""
  {
    rtx target = operands[0];
    rtx source = operands[1];
    rtx where = operands[2];
    lvx_expand_vector_extract (target, source, where);
    DONE;
  }
)

(define_expand "vec_init<mode><inner>"
  [(match_operand:SIMDALL 0 "register_operand" "")
   (match_operand 1 "" "")]
  ""
  {
    rtx target = operands[0];
    rtx source = operands[1];
    lvx_expand_vector_init (target, source);
    DONE;
  }
)

(define_expand "vec_duplicate<mode>"
  [(match_operand:SIMDALL 0 "register_operand" "")
   (match_operand 1 "" "")]
  ""
  {
    rtx target = operands[0];
    rtx source = operands[1];
    lvx_expand_vector_duplicate (target, source);
    DONE;
  }
)

(define_expand "vec_cmp<mode><mask>"
  [(set (match_operand:<MASK> 0 "register_operand")
        (match_operator 1 "comparison_operator"
         [(match_operand:SIMDCMP 2 "register_operand")
          (match_operand:SIMDCMP 3 "reg_zero_mone_operand")]))]
  ""
  {
    rtx mask = operands[0];
    rtx comp = operands[1];
    lvx_lower_comparison (mask, comp, <MODE>mode);
    DONE;
  }
)

(define_expand "vec_cmpu<mode><mask>"
  [(set (match_operand:<MASK> 0 "register_operand")
        (match_operator 1 "comparison_operator"
         [(match_operand:SIMDCMP 2 "register_operand")
          (match_operand:SIMDCMP 3 "reg_zero_mone_operand")]))]
  ""
  {
    rtx mask = operands[0];
    rtx comp = operands[1];
    lvx_lower_comparison (mask, comp, <MODE>mode);
    DONE;
  }
)

(define_expand "vcond<SIMDALL:mode><SIMDCMP:mode>"
  [(match_operand:SIMDALL 0 "register_operand")
   (match_operand:SIMDALL 1 "nonmemory_operand")
   (match_operand:SIMDALL 2 "nonmemory_operand")
   (match_operator 3 "comparison_operator"
    [(match_operand:SIMDCMP 4 "register_operand")
     (match_operand:SIMDCMP 5 "reg_zero_mone_operand")])]
  "(GET_MODE_NUNITS (<SIMDCMP:MODE>mode) == GET_MODE_NUNITS (<SIMDALL:MODE>mode))"
  {
    rtx target = operands[0];
    rtx select1 = operands[1];
    rtx select2 = operands[2];
    lvx_expand_conditional_move (target, select1, select2, operands[3]);
    DONE;
  }
)

(define_expand "vcondu<SIMDALL:mode><SIMDCMP:mode>"
  [(match_operand:SIMDALL 0 "register_operand")
   (match_operand:SIMDALL 1 "nonmemory_operand")
   (match_operand:SIMDALL 2 "nonmemory_operand")
   (match_operator 3 "comparison_operator"
    [(match_operand:SIMDCMP 4 "register_operand")
     (match_operand:SIMDCMP 5 "reg_zero_mone_operand")])]
  "(GET_MODE_NUNITS (<SIMDCMP:MODE>mode) == GET_MODE_NUNITS (<SIMDALL:MODE>mode))"
  {
    rtx target = operands[0];
    rtx select1 = operands[1];
    rtx select2 = operands[2];
    lvx_expand_conditional_move (target, select1, select2, operands[3]);
    DONE;
  }
)

(define_expand "vcond_mask_<mode><mask>"
  [(match_operand:SIMDALL 0 "register_operand")
   (match_operand:SIMDALL 1 "nonmemory_operand")
   (match_operand:SIMDALL 2 "nonmemory_operand")
   (match_operand:<MASK> 3 "register_operand")]
  ""
  {
    rtx target = operands[0];
    rtx select1 = operands[1];
    rtx select2 = operands[2];
    rtx mask = operands[3];
    lvx_expand_masked_move (target, select1, select2, mask);
    DONE;
  }
)

(define_expand "vec_shl_insert_<mode>"
  [(match_operand:SIMDALL 0 "register_operand" "")
   (match_operand:SIMDALL 1 "register_operand" "")
   (match_operand:<INNER> 2 "register_operand" "")]
  ""
  {
    HOST_WIDE_INT bits = GET_MODE_SIZE (<INNER>mode) * BITS_PER_UNIT;
    lvx_expand_vector_shift (operands[0], operands[1], operands[2], bits, 1);
    DONE;
  }
)

(define_expand "vec_shl_<mode>"
  [(match_operand:SIMDALL 0 "register_operand" "")
   (match_operand:SIMDALL 1 "register_operand" "")
   (match_operand:SI 2 "const_pos32_operand" "")]
  ""
  {
    HOST_WIDE_INT value = INTVAL (operands[2]);
    lvx_expand_vector_shift (operands[0], operands[1], const0_rtx, value, 1);
    DONE;
  }
)

(define_expand "vec_shr_<mode>"
  [(match_operand:SIMDALL 0 "register_operand" "")
   (match_operand:SIMDALL 1 "register_operand" "")
   (match_operand:SI 2 "const_pos32_operand" "")]
  ""
  {
    HOST_WIDE_INT value = INTVAL (operands[2]);
    lvx_expand_vector_shift (operands[0], operands[1], const0_rtx, value, 0);
    DONE;
  }
)

(define_insn "*adddp"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (unspec:ALL128 [(match_operand:SIMD128 1 "register_operand" "r")
                        (match_operand:DI 2 "register_operand" "r")] UNSPEC_ADDD))]
  ""
  "addd %x0 = %x1, %2\n\taddd %y0 = %y1, %2"
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)

(define_insn "*adddq"
  [(set (match_operand:ALL256 0 "register_operand" "=r")
        (unspec:ALL256 [(match_operand:SIMD256 1 "register_operand" "r")
                        (match_operand:DI 2 "register_operand" "r")] UNSPEC_ADDD))]
  "LVX_2"
  {
    return "adddp %L0 = %L1, %2\n\tadddp %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*andd"
  [(set (match_operand:FITGPR 0 "register_operand" "=r,r,r,r")
        (unspec:FITGPR [(match_operand:SCALAR 1 "register_operand" "r,r,r,r")
                        (match_operand:WI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")] UNSPEC_ANDD))]
  ""
  "andd %0 = %1, %2"
  [(set_attr "type" "alu_tiny,alu_tiny,alu_tiny_x,alu_tiny_y")
   (set_attr "length" "4,4,8,12")]
)

(define_insn "*anddp"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (unspec:ALL128 [(match_operand:SIMD128 1 "register_operand" "r")
                        (match_operand:DI 2 "register_operand" "r")] UNSPEC_ANDD))]
  ""
  "andq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*anddq"
  [(set (match_operand:ALL256 0 "register_operand" "=r")
        (unspec:ALL256 [(match_operand:SIMD256 1 "register_operand" "r")
                        (match_operand:DI 2 "register_operand" "r")] UNSPEC_ANDD))]
  "LVX_2"
  {
    return "andq %L0 = %L1, %2\n\tandq %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*anddp"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (unspec:ALL128 [(match_operand:ALL128 1 "register_operand" "r")
                        (match_operand:ALL128 2 "register_operand" "r")] UNSPEC_ANDD))]
  ""
  "andq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*anddq"
  [(set (match_operand:ALL256 0 "register_operand" "=r")
        (unspec:ALL256 [(match_operand:ALL256 1 "register_operand" "r")
                        (match_operand:ALL256 2 "register_operand" "r")] UNSPEC_ANDD))]
  "LVX_2"
  {
    return "andq %L0 = %L1, %L2\n\tandq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*xord"
  [(set (match_operand:FITGPR 0 "register_operand" "=r,r,r,r")
        (unspec:FITGPR [(match_operand:FITGPR 1 "register_operand" "r,r,r,r")
                        (match_operand:SCALAR 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")] UNSPEC_XORD))]
  ""
  "eord %0 = %1, %2"
  [(set_attr "type" "alu_tiny")]
)

(define_insn "*xordp"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (unspec:ALL128 [(match_operand:ALL128 1 "register_operand" "r")
                        (match_operand:ALL128 2 "register_operand" "r")] UNSPEC_XORD))]
  ""
  "eorq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*xordq"
  [(set (match_operand:ALL256 0 "register_operand" "=r")
        (unspec:ALL256 [(match_operand:ALL256 1 "register_operand" "r")
                        (match_operand:ALL256 2 "register_operand" "r")] UNSPEC_XORD))]
  "LVX_2"
  {
    return "eorq %L0 = %L1, %L2\n\teorq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*dup128"
  [(set (match_operand:SIMD128 0 "register_operand" "=r")
        (vec_duplicate:SIMD128 (match_operand:<CHUNK> 1 "nonmemory_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 8) (match_dup 1))]
  ""
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"         "8")]
)

(define_insn_and_split "*dup256"
  [(set (match_operand:SIMD256 0 "register_operand" "=r")
        (vec_duplicate:SIMD256 (match_operand:<CHUNK> 1 "nonmemory_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 8) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 16) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 24) (match_dup 1))]
  ""
  [(set_attr "type" "alu_tiny_x4")
   (set_attr "length"        "16")]
)

(define_insn_and_split "*dup512"
  [(set (match_operand:SIMD512 0 "register_operand" "=r")
        (vec_duplicate:SIMD512 (match_operand:<CHUNK> 1 "nonmemory_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 8) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 16) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 24) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 32) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 40) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 48) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 56) (match_dup 1))]
  ""
)

(define_insn "lvx_oroebx"
  [(set (match_operand:V16QI 0 "register_operand" "=r")
        (unspec:V16QI [(match_operand:V8HI 1 "register_operand" "r")
                       (match_operand:V8HI 2 "register_operand" "r")] UNSPEC_OROE))]
  ""
  "iorq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "lvx_oroebv"
  [(set (match_operand:V32QI 0 "register_operand" "=r")
        (unspec:V32QI [(match_operand:V16HI 1 "register_operand" "r")
                       (match_operand:V16HI 2 "register_operand" "r")] UNSPEC_OROE))]
  "LVX_2"
  {
    return "iorq %L0 = %L1, %L2\n\tiorq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "lvx_oroebt"
  [(set (match_operand:V64QI 0 "register_operand" "=r")
        (unspec:V64QI [(match_operand:V32HI 1 "register_operand" "r")
                       (match_operand:V32HI 2 "register_operand" "r")] UNSPEC_OROE))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V32QI (match_dup 0) 0)
        (unspec:V32QI [(subreg:V16HI (match_dup 1) 0)
                       (subreg:V16HI (match_dup 2) 0)] UNSPEC_OROE))
   (set (subreg:V32QI (match_dup 0) 32)
        (unspec:V32QI [(subreg:V16HI (match_dup 1) 32)
                       (subreg:V16HI (match_dup 2) 32)] UNSPEC_OROE))]
  ""
)

(define_insn_and_split "*zxe<hwidenx>_oroe<suffix>_2"
  [(set (match_operand:<HWIDE> 0 "register_operand" "=r")
        (unspec:<HWIDE> [(unspec:VXQI [(match_operand:<HWIDE> 1 "register_operand" "r")
                                       (match_operand:<HWIDE> 2 "register_operand" "r")] UNSPEC_OROE)] UNSPEC_ZXE))]
  ""
  "#"
  ""
  [(set (match_dup 0) (match_dup 2))]
  ""
  [(set_attr "type" "alu_tiny")]
)

(define_insn_and_split "*qxo<hwidenx>_oroe<suffix>_1"
  [(set (match_operand:<HWIDE> 0 "register_operand" "=r")
        (unspec:<HWIDE> [(unspec:VXQI [(match_operand:<HWIDE> 1 "register_operand" "r")
                                       (match_operand:<HWIDE> 2 "register_operand" "r")] UNSPEC_OROE)] UNSPEC_QXO))]
  ""
  "#"
  ""
  [(set (match_dup 0) (match_dup 1))]
  ""
  [(set_attr "type" "alu_tiny")]
)

(define_insn_and_split "*zxo<hwidenx>_oroe<suffix>_1"
  [(set (match_operand:<HWIDE> 0 "register_operand" "=r")
        (unspec:<HWIDE> [(unspec:VXQI [(match_operand:<HWIDE> 1 "register_operand" "r")
                                       (match_operand:<HWIDE> 2 "register_operand" "r")] UNSPEC_OROE)] UNSPEC_ZXO))]
  ""
  "#"
  ""
  [(set (match_dup 0)
        (unspec:<HWIDE> [(match_dup 1)] UNSPEC_ZXO))]
  {
    rtx op1 = gen_reg_rtx (<MODE>mode);
    emit_insn (gen_rtx_SET (op1, simplify_gen_subreg (<MODE>mode, operands[1], <HWIDE>mode, 0)));
    operands[1] = op1;
  }
  [(set_attr "type" "alu_tiny")]
)

(define_insn_and_split "*qxe<hwidenx>_oroe<suffix>_2"
  [(set (match_operand:<HWIDE> 0 "register_operand" "=r")
        (unspec:<HWIDE> [(unspec:VXQI [(match_operand:<HWIDE> 1 "register_operand" "r")
                                       (match_operand:<HWIDE> 2 "register_operand" "r")] UNSPEC_OROE)] UNSPEC_QXE))]
  ""
  "#"
  ""
  [(set (match_dup 0)
        (unspec:<HWIDE> [(match_dup 2)] UNSPEC_QXE))]
  {
    rtx op2 = gen_reg_rtx (<MODE>mode);
    emit_insn (gen_rtx_SET (op2, simplify_gen_subreg (<MODE>mode, operands[2], <HWIDE>mode, 0)));
    operands[2] = op2;
  }
  [(set_attr "type" "alu_tiny")]
)

;; neg<m>2 ssneg<m>2 abs<m>2 ssabs<m>2
(define_expand "<prefix><mode>2"
  [(set (match_operand:VXQI 0 "register_operand" "")
        (UNARITH:VXQI (match_operand:VXQI 1 "register_operand" "")))]
  ""
  {
    if (!HAVE_LVX_<unarith>_V8QI)
      {
        rtx op1o = gen_reg_rtx (<HWIDE>mode), op1e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op1o, operands[1]));
        emit_insn (gen_lvx_qxe<hwidenx> (op1e, operands[1]));
        rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_<prefix><hwide>2 (op0e, op1e));
        emit_insn (gen_<prefix><hwide>2 (op0o, op1o));
        rtx opto = gen_reg_rtx (<MODE>mode), opte = gen_reg_rtx (<MODE>mode);
        if (<set8lsb>)
          {
            emit_insn (gen_rtx_SET (opto, gen_rtx_SUBREG (<MODE>mode, op0o, 0)));
            emit_insn (gen_lvx_qxo<hwidenx> (op0o, opto));
          }
        emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
        emit_insn (gen_lvx_zxo<hwidenx> (op0e, opte));
        emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));
        DONE;
      }
  }
)
(define_insn "*<prefix>v16qi2_2"
  [(set (match_operand:V16QI 0 "register_operand" "=r")
        (UNARITH:V16QI (match_operand:V16QI 1 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_<unarith>_V16QI)"
  "<stem>bx %0 = %1"
  [(set_attr "type" "alu_lite_x2_x")
   (set_attr "length"          "16")]
)
(define_insn "*<prefix>v32qi2_2"
  [(set (match_operand:V32QI 0 "register_operand" "=r")
        (UNARITH:V32QI (match_operand:V32QI 1 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_<unarith>_V32QI)"
  {
    return "<stem>bx %L0 = %L1\n\t<stem>bx %M0 = %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"          "32")]
)

;; add<m>3 ssadd<m>3 usass<m>3 sub<m>3 sssub<m>3 ussub<m>3 smin<m>3 smax<m>3 umin<m>3 umax<m>3
(define_expand "<prefix><mode>3"
  [(set (match_operand:VXQI 0 "register_operand" "")
        (BINARITH:VXQI (match_operand:VXQI 1 "register_operand" "")
                       (match_operand:VXQI 2 "register_operand" "")))]
  ""
  {
    if (!HAVE_LVX_<binarith>_<MODE>)
      {
        rtx op2o = gen_reg_rtx (<HWIDE>mode), op2e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op2o, operands[2]));
        emit_insn (gen_lvx_qxe<hwidenx> (op2e, operands[2]));
        rtx op1o = gen_reg_rtx (<HWIDE>mode), op1e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op1o, operands[1]));
        emit_insn (gen_lvx_qxe<hwidenx> (op1e, operands[1]));
        rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_<prefix><hwide>3 (op0o, op1o, op2o));
        emit_insn (gen_<prefix><hwide>3 (op0e, op1e, op2e));
        rtx opto = gen_reg_rtx (<MODE>mode), opte = gen_reg_rtx (<MODE>mode);
        if (<set8lsb>)
          {
            emit_insn (gen_rtx_SET (opto, gen_rtx_SUBREG (<MODE>mode, op0o, 0)));
            emit_insn (gen_lvx_qxo<hwidenx> (op0o, opto));
          }
        emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
        emit_insn (gen_lvx_zxo<hwidenx> (op0e, opte));
        emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));

        DONE;
      }
  }
)
(define_insn "*<prefix>v16qi3_2"
  [(set (match_operand:V16QI 0 "register_operand" "=r")
        (BINARITHC:V16QI (match_operand:V16QI 1 "register_operand" "r")
                         (match_operand:V16QI 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_<binarithc>_V16QI)"
  "<stem>bx %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)
(define_insn "*<prefix>v32qi3_2"
  [(set (match_operand:V32QI 0 "register_operand" "=r")
        (BINARITHC:V32QI (match_operand:V32QI 1 "register_operand" "r")
                         (match_operand:V32QI 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_<binarithc>_V32QI)"
  {
    return "<stem>bx %L0 = %L1, %L2\n\t<stem>bx %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)
(define_insn "*<prefix>v16qi3_2"
  [(set (match_operand:V16QI 0 "register_operand" "=r")
        (BINMINUS:V16QI (match_operand:V16QI 1 "register_operand" "r")
                        (match_operand:V16QI 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_<binminus>_V16QI)"
  "<stem>bx %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)
(define_insn "*<prefix>v32qi3_2"
  [(set (match_operand:V32QI 0 "register_operand" "=r")
        (BINMINUS:V32QI (match_operand:V32QI 1 "register_operand" "r")
                        (match_operand:V32QI 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_<binminus>_V32QI)"
  {
    return "<stem>bx %L0 = %L2, %L1\n\t<stem>bx %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "mul<mode>3"
  [(set (match_operand:VXQI 0 "register_operand" "")
        (mult:VXQI (match_operand:VXQI 1 "register_operand" "")
                   (match_operand:VXQI 2 "register_operand" "")))]
  ""
  {
    rtx op2o = gen_reg_rtx (<HWIDE>mode), op2e = gen_reg_rtx (<HWIDE>mode);
    emit_insn (gen_rtx_SET (op2e, simplify_gen_subreg (<HWIDE>mode, operands[2], <MODE>mode, 0)));
    emit_insn (gen_lvx_zxo<hwidenx> (op2o, operands[2]));
    rtx op1o = gen_reg_rtx (<HWIDE>mode), op1e = gen_reg_rtx (<HWIDE>mode);
    emit_insn (gen_rtx_SET (op1e, simplify_gen_subreg (<HWIDE>mode, operands[1], <MODE>mode, 0)));
    emit_insn (gen_lvx_qxo<hwidenx> (op1o, operands[1]));
    rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
    emit_insn (gen_mul<hwide>3 (op0o, op1o, op2o));
    emit_insn (gen_mul<hwide>3 (op0e, op1e, op2e));
    rtx opte = gen_reg_rtx (<MODE>mode);
    emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
    emit_insn (gen_lvx_zxe<hwidenx> (op0e, opte));
    emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));
    DONE;
  }
)

(define_expand "mulv64qi3"
  [(set (match_operand:V64QI 0 "register_operand" "")
        (mult:V64QI (match_operand:V64QI 1 "register_operand" "")
                    (match_operand:V64QI 2 "register_operand" "")))]
  ""
  {
    unsigned mode_size = GET_MODE_SIZE (V64QImode);
    unsigned half_size = GET_MODE_SIZE (V32QImode);
    for (unsigned offset = 0; offset < mode_size; offset += half_size)
      {
        rtx operand0 = gen_reg_rtx (V32QImode);
        rtx operand1 = gen_reg_rtx (V32QImode);
        rtx operand2 = gen_reg_rtx (V32QImode);
        emit_move_insn (operand1, simplify_gen_subreg(V32QImode, operands[1], V64QImode, offset));
        emit_move_insn (operand2, simplify_gen_subreg(V32QImode, operands[2], V64QImode, offset));
        emit_insn (gen_mulv32qi3 (operand0, operand1, operand2));
        emit_move_insn (simplify_gen_subreg(V32QImode, operands[0], V64QImode, offset), operand0);
      }
    DONE;
  }
)

(define_expand "<prefix><mode>3"
  [(set (match_operand:VXQI 0 "register_operand" "")
        (BINDIV:VXQI (match_operand:VXQI 1 "register_operand" "")
                       (match_operand:VXQI 2 "register_operand" "")))]
  ""
  {
    if (HAVE_LVX_<bindiv>_<MODE>)
      {
        rtx dest = emit_library_call_value (gen_rtx_SYMBOL_REF (Pmode, "__<prefix><mode>3"),
                                            operands[0], LCT_CONST, <MODE>mode,
                                            operands[1], <MODE>mode, operands[2], <MODE>mode);
        if (dest != operands[0])
          emit_move_insn (operands[0], dest);
      }
    else
      {
        /* Recursively try to use operations on wider lanes.  */
        rtx op2o = gen_reg_rtx (<HWIDE>mode), op2e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op2o, operands[2]));
        emit_insn (gen_lvx_qxe<hwidenx> (op2e, operands[2]));
        rtx op1o = gen_reg_rtx (<HWIDE>mode), op1e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op1o, operands[1]));
        emit_insn (gen_lvx_qxe<hwidenx> (op1e, operands[1]));
        rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_<prefix><hwide>3 (op0o, op1o, op2o));
        emit_insn (gen_<prefix><hwide>3 (op0e, op1e, op2e));
        rtx opto = gen_reg_rtx (<MODE>mode), opte = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (opto, gen_rtx_SUBREG (<MODE>mode, op0o, 0)));
        emit_insn (gen_lvx_qxe<hwidenx> (op0o, opto));
        if (<set8msb>)
          {
            emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
            emit_insn (gen_lvx_zxe<hwidenx> (op0e, opte));
          }
        emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));
      }
    DONE;
  }
)

(define_expand "<prefix><mode>3"
  [(set (match_operand:VXQI 0 "register_operand" "")
        (BINMOD:VXQI (match_operand:VXQI 1 "register_operand" "")
                       (match_operand:VXQI 2 "register_operand" "")))]
  ""
  {
    if (HAVE_LVX_<binmod>_<MODE>)
      {
        rtx dest = emit_library_call_value (gen_rtx_SYMBOL_REF (Pmode, "__<prefix><mode>3"),
                                            operands[0], LCT_CONST, <MODE>mode,
                                            operands[1], <MODE>mode, operands[2], <MODE>mode);
        if (dest != operands[0])
          emit_move_insn (operands[0], dest);
      }
    else
      {
        /* Recursively try to use operations on wider lanes.  */
        rtx op2o = gen_reg_rtx (<HWIDE>mode), op2e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op2o, operands[2]));
        emit_insn (gen_lvx_qxe<hwidenx> (op2e, operands[2]));
        rtx op1o = gen_reg_rtx (<HWIDE>mode), op1e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op1o, operands[1]));
        emit_insn (gen_lvx_qxe<hwidenx> (op1e, operands[1]));
        rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_<prefix><hwide>3 (op0o, op1o, op2o));
        emit_insn (gen_<prefix><hwide>3 (op0e, op1e, op2e));
        rtx opte = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
        emit_insn (gen_lvx_zxo<hwidenx> (op0e, opte));
        emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));
      }
    DONE;
  }
)

;; abd<m>3 abds<m>3
(define_expand "<MINUS:abdm><mode>3_1"
  [(set (match_operand:VXQI 0 "register_operand" "")
        (MINUS:VXQI (smax:VXQI (match_operand:VXQI 1 "register_operand" "")
                               (match_operand:VXQI 2 "register_operand" ""))
                    (smin:VXQI (match_dup 1) (match_dup 2))))]
  ""
  {
    if (!HAVE_LVX_ABD_V8QI)
      {
        rtx op2o = gen_reg_rtx (<HWIDE>mode), op2e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op2o, operands[2]));
        emit_insn (gen_lvx_qxe<hwidenx> (op2e, operands[2]));
        rtx op1o = gen_reg_rtx (<HWIDE>mode), op1e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op1o, operands[1]));
        emit_insn (gen_lvx_qxe<hwidenx> (op1e, operands[1]));
        rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_<MINUS:abdm><hwide>3 (op0o, op1o, op2o));
        emit_insn (gen_<MINUS:abdm><hwide>3 (op0e, op1e, op2e));
        rtx opto = gen_reg_rtx (<MODE>mode), opte = gen_reg_rtx (<MODE>mode);
        if (<set8lsb>)
          {
            emit_insn (gen_rtx_SET (opto, gen_rtx_SUBREG (<MODE>mode, op0o, 0)));
            emit_insn (gen_lvx_qxo<hwidenx> (op0o, opto));
          }
        emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
        emit_insn (gen_lvx_zxo<hwidenx> (op0e, opte));
        emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));
        DONE;
      }
  }
)

;; abdu<m>3
(define_expand "abdu<mode>3_1"
  [(set (match_operand:VXQI 0 "register_operand" "")
        (minus:VXQI (umax:VXQI (match_operand:VXQI 1 "register_operand" "")
                               (match_operand:VXQI 2 "register_operand" ""))
                    (umin:VXQI (match_dup 1) (match_dup 2))))]
  ""
  {
    if (!HAVE_LVX_UABD_V8QI)
      {
        rtx op2o = gen_reg_rtx (<HWIDE>mode), op2e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op2o, operands[2]));
        emit_insn (gen_lvx_qxe<hwidenx> (op2e, operands[2]));
        rtx op1o = gen_reg_rtx (<HWIDE>mode), op1e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op1o, operands[1]));
        emit_insn (gen_lvx_qxe<hwidenx> (op1e, operands[1]));
        rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_abdu<hwide>3 (op0o, op1o, op2o));
        emit_insn (gen_abdu<hwide>3 (op0e, op1e, op2e));
        rtx opte = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
        emit_insn (gen_lvx_zxo<hwidenx> (op0e, opte));
        emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));
        DONE;
      }
  }
)

;; avg<m>3_floor uavg<m>3_floor avg<m>3_ceil uavg<m>3_ceil
(define_expand "<avgpre><mode><avgpost>"
  [(set (match_operand:VXQI 0 "register_operand" "")
        (unspec:VXQI [(match_operand:VXQI 1 "register_operand" "")
                      (match_operand:VXQI 2 "register_operand" "")] UNSPEC_AVGI))]
  ""
  {
    if (!HAVE_LVX_<AVGPRE>_V8QI)
      {
        rtx op2o = gen_reg_rtx (<HWIDE>mode), op2e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op2o, operands[2]));
        emit_insn (gen_lvx_qxe<hwidenx> (op2e, operands[2]));
        if (<avground>)
          {
            // Add 0xFF to the low bytes so the rounding propagtes to the high bytes.
            rtx bias =  gen_reg_rtx (DImode);
            emit_insn (gen_rtx_SET (bias, GEN_INT (0x00FF00FF00FF00FF)));
            emit_insn (gen_rtx_SET (op2o, gen_rtx_UNSPEC (<HWIDE>mode, gen_rtvec (2, op2o, bias), UNSPEC_ADDD)));
            emit_insn (gen_rtx_SET (op2e, gen_rtx_UNSPEC (<HWIDE>mode, gen_rtvec (2, op2e, bias), UNSPEC_ADDD)));
          }
        rtx op1o = gen_reg_rtx (<HWIDE>mode), op1e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op1o, operands[1]));
        emit_insn (gen_lvx_qxe<hwidenx> (op1e, operands[1]));
        rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_<avgpre><hwide><avgpost> (op0o, op1o, op2o));
        emit_insn (gen_<avgpre><hwide><avgpost> (op0e, op1e, op2e));
        rtx opto = gen_reg_rtx (<MODE>mode), opte = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (opto, gen_rtx_SUBREG (<MODE>mode, op0o, 0)));
        emit_insn (gen_lvx_qxo<hwidenx> (op0o, opto));
        emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
        emit_insn (gen_lvx_zxo<hwidenx> (op0e, opte));
        emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));
        DONE;
      }
  }
)
;; ashl<m>3 ssashl<m>3 usashl<m>3
(define_expand "<prefix><mode>3"
  [(set (match_operand:VXQI 0 "register_operand" "")
        (BINSHL:VXQI (match_operand:VXQI 1 "register_operand" "")
                     (match_operand:SI 2 "reg_shift_operand" "")))]
  ""
  {
    if (!HAVE_LVX_<binshl>_V8QI)
      {
        rtx op2 = NULL_RTX;
        if (CONST_INT_P (operands[2]))
          op2 = GEN_INT (INTVAL (operands[2]) & 0x7);
        else
          {
            op2 = gen_reg_rtx (SImode);
            emit_insn (gen_andsi3 (op2, operands[2], GEN_INT (0x7)));
          }
        rtx op1o = gen_reg_rtx (<HWIDE>mode), op1e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op1o, operands[1]));
        emit_insn (gen_lvx_qxe<hwidenx> (op1e, operands[1]));
        rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_<prefix><hwide>3 (op0o, op1o, op2));
        emit_insn (gen_<prefix><hwide>3 (op0e, op1e, op2));
        rtx opto = gen_reg_rtx (<MODE>mode), opte = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
        emit_insn (gen_lvx_zxo<hwidenx> (op0e, opte));
        if (<set8lsb>)
          {
            emit_insn (gen_rtx_SET (opto, gen_rtx_SUBREG (<MODE>mode, op0o, 0)));
            emit_insn (gen_lvx_qxo<hwidenx> (op0o, opto));
          }
        emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));
        DONE;
      }
  }
)
(define_insn "*<prefix>v16qi3_2"
  [(set (match_operand:V16QI 0 "register_operand" "=r")
        (BINSHLRT:V16QI (match_operand:V16QI 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2 && (HAVE_LVX_<binshlrt>_V16QI)"
  "<stem>bx %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)
(define_insn "*<prefix>v32qi3_2"
  [(set (match_operand:V32QI 0 "register_operand" "=r")
        (BINSHLRT:V32QI (match_operand:V32QI 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2 && (HAVE_LVX_<binshlrt>_V32QI)"
  {
    return "<stem>bx %L0 = %L1, %2\n\t<stem>bx %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)
(define_insn "*<prefix>v16qi3_2"
  [(set (match_operand:V16QI 0 "register_operand" "=r")
        (BINSHLRL:V16QI (match_operand:V16QI 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2 && (HAVE_LVX_<binshlrl>_V16QI)"
  "<stem>bx %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)
(define_insn_and_split "*<prefix>v32qi3_2"
  [(set (match_operand:V32QI 0 "register_operand" "=&r,r")
        (BINSHLRL:V32QI (match_operand:V32QI 1 "register_operand" "r,r")
                        (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  "HAVE_LVX_<binshlrl>_V32QI"
  "#"
  "HAVE_LVX_<binshlrl>_V32QI && reload_completed"
  [(set (subreg:V16QI (match_dup 0) 0)
        (BINSHLRL:V16QI (subreg:V16QI (match_dup 1) 0)
                        (match_dup 2)))
   (set (subreg:V16QI (match_dup 0) 16)
        (BINSHLRL:V16QI (subreg:V16QI (match_dup 1) 16)
                        (match_dup 2)))]
  ""
  [(set_attr "type" "alu_lite_x2,alu_lite_x2")]
)

;; lshr<m>3 ashr<m>3
(define_expand "<prefix><mode>3"
  [(set (match_operand:VXQI 0 "register_operand" "")
        (BINSHR:VXQI (match_operand:VXQI 1 "register_operand" "")
                     (match_operand:SI 2 "reg_shift_operand" "")))]
  ""
  {
    if (!HAVE_LVX_<binshr>_V8QI)
      {
        rtx op2 = NULL_RTX, op2p8 = NULL_RTX;
        if (CONST_INT_P (operands[2]))
          {
            op2 = GEN_INT (INTVAL (operands[2]) & 0x7);
            op2p8 = GEN_INT ((INTVAL (operands[2]) & 0x7) + 8);
          }
        else
          {
            op2 = gen_reg_rtx (SImode), op2p8 = gen_reg_rtx (SImode);
            emit_insn (gen_andsi3 (op2, operands[2], GEN_INT (0x7)));
            emit_insn (gen_addsi3 (op2p8, op2, GEN_INT (8)));
          }
        rtx op1o = gen_reg_rtx (<HWIDE>mode), op1e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op1o, operands[1]));
        emit_insn (gen_lvx_qxe<hwidenx> (op1e, operands[1]));
        rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_<prefix><hwide>3 (op0o, op1o, op2));
        emit_insn (gen_<prefix><hwide>3 (op0e, op1e, op2p8));
        rtx opto = gen_reg_rtx (<MODE>mode), opte = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (opto, gen_rtx_SUBREG (<MODE>mode, op0o, 0)));
        emit_insn (gen_lvx_qxo<hwidenx> (op0o, opto));
        if (<set8msb>)
          {
            emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
            emit_insn (gen_lvx_zxe<hwidenx> (op0e, opte));
          }
        emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));
        DONE;
      }
  }
)

;; sshr<m>3
(define_expand "sshr<mode>3"
  [(match_operand:VXQI 0 "register_operand" "")
   (match_operand:VXQI 1 "register_operand" "")
   (match_operand:SI 2 "register_operand" "")]
  ""
  {
    if (!HAVE_LVX_SSHR_V8QI)
      {
        rtx const8 = GEN_INT (8);
        rtx op2 = gen_reg_rtx (SImode), op2p8 = gen_reg_rtx (SImode);
        emit_insn (gen_andsi3 (op2, operands[2], GEN_INT (0x7)));
        emit_insn (gen_addsi3 (op2p8, op2, const8));
        rtx op1o = gen_reg_rtx (<HWIDE>mode), op1e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lvx_qxo<hwidenx> (op1o, operands[1]));
        emit_insn (gen_lvx_qxe<hwidenx> (op1e, operands[1]));
        rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_sshr<hwide>3 (op0o, op1o, op2p8));
        emit_insn (gen_sshr<hwide>3 (op0e, op1e, op2p8));
        rtx opto = gen_reg_rtx (<MODE>mode), opte = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (opto, gen_rtx_SUBREG (<MODE>mode, op0o, 0)));
        emit_insn (gen_lvx_qxe<hwidenx> (op0o, opto));
        emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
        emit_insn (gen_lvx_zxe<hwidenx> (op0e, opte));
        emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));
      }
    else
      {
        rtvec vec = gen_rtvec (2, operands[1], operands[2]);
        rtx select = gen_rtx_UNSPEC (<MODE>mode, vec, UNSPEC_SRS);
        emit_insn (gen_rtx_SET (operands[0], select));
      }
    DONE;
  }
)
(define_insn_and_split "*sshrv32qi_2"
  [(set (match_operand:V32QI 0 "register_operand" "=&r,r")
        (unspec:V32QI [(match_operand:V32QI 1 "register_operand" "r,r")
                       (match_operand:SI 2 "reg_shift_operand" "r,U06")] UNSPEC_SRS))]
  "HAVE_LVX_SSHR_V32QI"
  "#"
  "HAVE_LVX_SSHR_V32QI && reload_completed"
  [(set (subreg:V16QI (match_dup 0) 0)
        (unspec:V16QI [(subreg:V16QI (match_dup 1) 0)
                       (match_dup 2)] UNSPEC_SRS))
   (set (subreg:V16QI (match_dup 0) 16)
        (unspec:V16QI [(subreg:V16QI (match_dup 1) 16)
                       (match_dup 2)] UNSPEC_SRS))]
  ""
  [(set_attr "type" "alu_lite_x2,alu_lite_x2")]
)


;; rotl<m>3
(define_expand "rotl<mode>3"
  [(set (match_operand:VXQI 0 "register_operand" "")
        (rotate:VXQI (match_operand:VXQI 1 "register_operand" "")
                     (match_operand:SI 2 "register_operand" "")))
   (clobber (match_scratch:SI 3 ""))
   (clobber (match_scratch:VXQI 4 ""))
   (clobber (match_scratch:VXQI 5 ""))]
  ""
  {
    if (!HAVE_LVX_NEG_<MODE> || !HAVE_LVX_ASHIFT_<MODE>
     || !HAVE_LVX_LSHIFTRT_<MODE> || !HAVE_LVX_IOR_<MODE>)
      {
        rtx evenbmm = gen_reg_rtx (DImode), oddbmm = gen_reg_rtx (DImode);
        emit_insn (gen_rtx_SET (evenbmm, GEN_INT (0x4040101004040101)));
        emit_insn (gen_rtx_SET (oddbmm, GEN_INT (0x8080202008080202)));
        rtx vevenbmm = evenbmm, voddbmm = oddbmm;
        unsigned nwords = GET_MODE_SIZE (<MODE>mode) / UNITS_PER_WORD;
        if (nwords > 1)
          {
            machine_mode vmode = mode_for_vector (DImode, nwords).require ();
            vevenbmm = gen_rtx_VEC_DUPLICATE (vmode, evenbmm);
            voddbmm = gen_rtx_VEC_DUPLICATE (vmode, oddbmm);
          }
        rtx op2 = gen_reg_rtx (SImode);
        emit_insn (gen_andsi3 (op2, operands[2], GEN_INT (0x7)));
        rtx op1o = gen_reg_rtx (<HWIDE>mode), op1e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_rtx_SET (op1o, gen_rtx_UNSPEC (<HWIDE>mode, gen_rtvec (2, operands[1], voddbmm), UNSPEC_SBMM8D)));
        emit_insn (gen_rtx_SET (op1e, gen_rtx_UNSPEC (<HWIDE>mode, gen_rtvec (2, operands[1], vevenbmm), UNSPEC_SBMM8D)));
        rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_ashl<hwide>3 (op0o, op1o, op2));
        emit_insn (gen_ashl<hwide>3 (op0e, op1e, op2));
        rtx opto = gen_reg_rtx (<MODE>mode), opte = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (opto, gen_rtx_SUBREG (<MODE>mode, op0o, 0)));
        emit_insn (gen_lvx_qxo<hwidenx> (op0o, opto));
        emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
        emit_insn (gen_lvx_zxo<hwidenx> (op0e, opte));
        emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));
      }
    else
      {
        rtx operands_3 = gen_reg_rtx (SImode);
        rtx operands_4 = gen_reg_rtx (<MODE>mode);
        rtx operands_5 = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (operands_3, gen_rtx_NEG (SImode, operands[2])));
        emit_insn (gen_rtx_SET (operands_4, gen_rtx_ASHIFT (<MODE>mode, operands[1], operands[2])));
        emit_insn (gen_rtx_SET (operands_5, gen_rtx_LSHIFTRT (<MODE>mode, operands[1], operands_3)));
        emit_insn (gen_rtx_SET (operands[0], gen_rtx_IOR (<MODE>mode, operands_4, operands_5)));
      }
    DONE;
  }
)

;; rotr<m>3
(define_expand "rotr<mode>3"
  [(set (match_operand:VXQI 0 "register_operand" "")
        (rotatert:VXQI (match_operand:VXQI 1 "register_operand" "")
                       (match_operand:SI 2 "register_operand" "")))
   (clobber (match_scratch:SI 3 ""))
   (clobber (match_scratch:VXQI 4 ""))
   (clobber (match_scratch:VXQI 5 ""))]
  ""
  {
    if (!HAVE_LVX_LSHIFTRT_<MODE> || !HAVE_LVX_ASHIFT_<MODE> || !HAVE_LVX_IOR_<MODE>)
      {
        rtx evenbmm = gen_reg_rtx (DImode), oddbmm = gen_reg_rtx (DImode);
        emit_insn (gen_rtx_SET (evenbmm, GEN_INT (0x4040101004040101)));
        emit_insn (gen_rtx_SET (oddbmm, GEN_INT (0x8080202008080202)));
        rtx vevenbmm = evenbmm, voddbmm = oddbmm;
        unsigned nwords = GET_MODE_SIZE (<MODE>mode) / UNITS_PER_WORD;
        if (nwords > 1)
          {
            machine_mode vmode = mode_for_vector (DImode, nwords).require ();
            vevenbmm = gen_rtx_VEC_DUPLICATE (vmode, evenbmm);
            voddbmm = gen_rtx_VEC_DUPLICATE (vmode, oddbmm);
          }
        rtx op2 = gen_reg_rtx (SImode);
        emit_insn (gen_andsi3 (op2, operands[2], GEN_INT (0x7)));
        rtx op1o = gen_reg_rtx (<HWIDE>mode), op1e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_rtx_SET (op1o, gen_rtx_UNSPEC (<HWIDE>mode, gen_rtvec (2, operands[1], voddbmm), UNSPEC_SBMM8D)));
        emit_insn (gen_rtx_SET (op1e, gen_rtx_UNSPEC (<HWIDE>mode, gen_rtvec (2, operands[1], vevenbmm), UNSPEC_SBMM8D)));
        rtx op0o = gen_reg_rtx (<HWIDE>mode), op0e = gen_reg_rtx (<HWIDE>mode);
        emit_insn (gen_lshr<hwide>3 (op0o, op1o, op2));
        emit_insn (gen_lshr<hwide>3 (op0e, op1e, op2));
        rtx opto = gen_reg_rtx (<MODE>mode), opte = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (opto, gen_rtx_SUBREG (<MODE>mode, op0o, 0)));
        emit_insn (gen_lvx_qxe<hwidenx> (op0o, opto));
        emit_insn (gen_rtx_SET (opte, gen_rtx_SUBREG (<MODE>mode, op0e, 0)));
        emit_insn (gen_lvx_zxe<hwidenx> (op0e, opte));
        emit_insn (gen_lvx_oroe<suffix> (operands[0], op0o, op0e));
      }
    else
      {
        rtx operands_3 = gen_reg_rtx (SImode);
        rtx operands_4 = gen_reg_rtx (<MODE>mode);
        rtx operands_5 = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_rtx_SET (operands_3, gen_rtx_NEG (SImode, operands[2])));
        emit_insn (gen_rtx_SET (operands_4, gen_rtx_LSHIFTRT (<MODE>mode, operands[1], operands[2])));
        emit_insn (gen_rtx_SET (operands_5, gen_rtx_ASHIFT (<MODE>mode, operands[1], operands_3)));
        emit_insn (gen_rtx_SET (operands[0], gen_rtx_IOR (<MODE>mode, operands_4, operands_5)));
      }
    DONE;
  }
)

;; S128I (V8HI V4SI)

(define_insn "ashl<mode>3"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (ashift:S128I (match_operand:S128I 1 "register_operand" "r")
                      (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "sll<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "ssashl<mode>3"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (ss_ashift:S128I (match_operand:S128I 1 "register_operand" "r")
                         (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "sls<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_expand "usashl<mode>3"
  [(match_operand:S128I 0 "register_operand" "")
   (match_operand:S128I 1 "register_operand" "")
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
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (us_ashift:S128I (match_operand:S128I 1 "register_operand" "r")
                         (match_operand:SI 2 "reg_shift_operand" "rU06")))
   (clobber (match_scratch:S128I 3 "=&r"))
   (clobber (match_scratch:S128I 4 "=&r"))
   (clobber (match_scratch:S128I 5 "=&r"))]
  "!HAVE_LVX_US_ASHIFT_<MODE>"
  "#"
  "!HAVE_LVX_US_ASHIFT_<MODE>"
  [(set (match_dup 3)
        (ashift:S128I (match_dup 1) (match_dup 2)))
   (set (match_dup 4)
        (lshiftrt:S128I (match_dup 3) (match_dup 2)))
   (set (match_dup 5)
        (ne:S128I (match_dup 4) (match_dup 1)))
   (set (match_dup 0)
        (ior:S128I (match_dup 3) (match_dup 5)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[5]) == SCRATCH)
      operands[5] = gen_reg_rtx (<MODE>mode);
  }
)

(define_insn "usashl<mode>3_2"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (us_ashift:S128I (match_operand:S128I 1 "register_operand" "r")
                         (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2 && (HAVE_LVX_US_ASHIFT_<MODE>)"
  "slus<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "ashr<mode>3"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (ashiftrt:S128I (match_operand:S128I 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "sra<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "lshr<mode>3"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (lshiftrt:S128I (match_operand:S128I 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "srl<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "sshr<mode>3"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (unspec:S128I [(match_operand:S128I 1 "register_operand" "r")
                       (match_operand:SI 2 "reg_shift_operand" "rU06")] UNSPEC_SRS))]
  "LVX_2"
  "srs<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

;; S128I is the non-byte 128-bit integer modes, so V16QI needs its own -- SRSBX
;; is a real instruction, it just falls outside that iterator.
(define_insn "*sshrv16qi_2"
  [(set (match_operand:V16QI 0 "register_operand" "=r")
        (unspec:V16QI [(match_operand:V16QI 1 "register_operand" "r")
                       (match_operand:SI 2 "reg_shift_operand" "rU06")] UNSPEC_SRS))]
  "LVX_2 && (HAVE_LVX_SSHR_V16QI)"
  "srsbx %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "avg<mode>3_floor"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (unspec:S128I [(match_operand:S128I 1 "register_operand" "r")
                       (match_operand:S128I 2 "register_operand" "r")] UNSPEC_AVG))]
  "LVX_2"
  "avg<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "avg<mode>3_ceil"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (unspec:S128I [(match_operand:S128I 1 "register_operand" "r")
                       (match_operand:S128I 2 "register_operand" "r")] UNSPEC_AVGR))]
  "LVX_2"
  "avgr<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "uavg<mode>3_floor"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (unspec:S128I [(match_operand:S128I 1 "register_operand" "r")
                       (match_operand:S128I 2 "register_operand" "r")] UNSPEC_AVGU))]
  "LVX_2"
  "avgu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "uavg<mode>3_ceil"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (unspec:S128I [(match_operand:S128I 1 "register_operand" "r")
                       (match_operand:S128I 2 "register_operand" "r")] UNSPEC_AVGRU))]
  "LVX_2"
  "avgru<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_expand "extend<mode><wide>2"
  [(set (match_operand:<WIDE> 0 "register_operand" "")
        (sign_extend:<WIDE> (match_operand:S128L 1 "register_operand" "")))]
  ""
  {
    emit_insn (gen_lvx_sx<widenx> (operands[0], operands[1]));
    DONE;
  }
)

(define_expand "zero_extend<mode><wide>2"
  [(set (match_operand:<WIDE> 0 "register_operand" "")
        (zero_extend:<WIDE> (match_operand:S128L 1 "register_operand" "")))]
  ""
  {
    emit_insn (gen_lvx_zx<widenx> (operands[0], operands[1]));
    DONE;
  }
)


;; V128I (V8HI V2DI)

(define_insn_and_split "mul<mode>3"
  [(set (match_operand:V128I 0 "register_operand" "=r")
        (mult:V128I (match_operand:V128I 1 "register_operand" "r")
                    (match_operand:V128I 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (mult:<HALF> (subreg:<HALF> (match_dup 1) 0)
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 8)
        (mult:<HALF> (subreg:<HALF> (match_dup 1) 8)
                     (subreg:<HALF> (match_dup 2) 8)))]
  ""
  [(set_attr "type" "madd_int")]
)

(define_insn_and_split "*mul<mode>3_s1"
  [(set (match_operand:V128I 0 "register_operand" "=&r")
        (mult:V128I (vec_duplicate:V128I (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128I 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (mult:<HALF> (match_dup 1)
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 8)
        (mult:<HALF> (match_dup 1)
                     (subreg:<HALF> (match_dup 2) 8)))]
  ""
  [(set_attr "type" "madd_int")]
)

(define_insn_and_split "*mul<mode>3_s2"
  [(set (match_operand:V128I 0 "register_operand" "=&r")
        (mult:V128I (match_operand:V128I 1 "register_operand" "r")
                    (vec_duplicate:V128I (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (mult:<HALF> (subreg:<HALF> (match_dup 1) 0)
                     (match_dup 2)))
   (set (subreg:<HALF> (match_dup 0) 8)
        (mult:<HALF> (subreg:<HALF> (match_dup 1) 8)
                     (match_dup 2)))]
  ""
  [(set_attr "type" "madd_int")]
)

(define_insn_and_split "rotl<mode>3"
  [(set (match_operand:V128I 0 "register_operand" "=r")
        (rotate:V128I (match_operand:V128I 1 "register_operand" "r")
                      (match_operand:SI 2 "register_operand" "r")))
   (clobber (match_scratch:SI 3 "=&r"))
   (clobber (match_scratch:V128I 4 "=&r"))
   (clobber (match_scratch:V128I 5 "=&r"))]
  ""
  "#"
  ""
  [(set (match_dup 3) (neg:SI (match_dup 2)))
   (set (match_dup 4) (ashift:V128I (match_dup 1) (match_dup 2)))
   (set (match_dup 5) (lshiftrt:V128I (match_dup 1) (match_dup 3)))
   (set (match_dup 0) (ior:V128I (match_dup 4) (match_dup 5)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (SImode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[5]) == SCRATCH)
      operands[5] = gen_reg_rtx (<MODE>mode);
  }
)

(define_insn_and_split "rotr<mode>3"
  [(set (match_operand:V128I 0 "register_operand" "=r")
        (rotatert:V128I (match_operand:V128I 1 "register_operand" "r")
                        (match_operand:SI 2 "register_operand" "r")))
   (clobber (match_scratch:SI 3 "=&r"))
   (clobber (match_scratch:V128I 4 "=&r"))
   (clobber (match_scratch:V128I 5 "=&r"))]
  ""
  "#"
  ""
  [(set (match_dup 3) (neg:SI (match_dup 2)))
   (set (match_dup 4) (lshiftrt:V128I (match_dup 1) (match_dup 2)))
   (set (match_dup 5) (ashift:V128I (match_dup 1) (match_dup 3)))
   (set (match_dup 0) (ior:V128I (match_dup 4) (match_dup 5)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (SImode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[5]) == SCRATCH)
      operands[5] = gen_reg_rtx (<MODE>mode);
  }
)


;; V128J (V8HI V4SI V2DI)

(define_insn "add<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (plus:V128J (match_operand:V128J 1 "register_operand" "r")
                    (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "add<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*add<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (plus:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "add<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*add<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (plus:V128J (match_operand:V128J 1 "register_operand" "r")
                    (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "add<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "ssadd<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (ss_plus:V128J (match_operand:V128J 1 "register_operand" "r")
                       (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "adds<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*ssadd<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (ss_plus:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                       (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "adds<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*ssadd<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (ss_plus:V128J (match_operand:V128J 1 "register_operand" "r")
                       (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "adds<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_expand "usadd<mode>3"
  [(match_operand:V128J 0 "register_operand" "")
   (match_operand:V128J 1 "register_operand" "")
   (match_operand:V128J 2 "register_operand" "")]
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
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (us_plus:V128J (match_operand:V128J 1 "register_operand" "r")
                       (match_operand:V128J 2 "register_operand" "r")))
   (clobber (match_scratch:V128J 3 "=&r"))
   (clobber (match_scratch:V128J 4 "=&r"))]
  "!HAVE_LVX_US_PLUS_<MODE>"
  "#"
  "!HAVE_LVX_US_PLUS_<MODE>"
  [(set (match_dup 3)
        (plus:V128J (match_dup 1) (match_dup 2)))
   (set (match_dup 4)
        (ltu:V128J (match_dup 3) (match_dup 1)))
   (set (match_dup 0)
        (ior:V128J (match_dup 3) (match_dup 4)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
  }
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "*usadd<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=&r")
        (us_plus:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                       (match_operand:V128J 2 "register_operand" "r")))
   (clobber (match_scratch:V128J 3 "=&r"))
   (clobber (match_scratch:V128J 4 "=&r"))]
  "!HAVE_LVX_US_PLUS_<MODE>"
  "#"
  "!HAVE_LVX_US_PLUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (plus:V128J (vec_duplicate:V128J (match_dup 1)) (match_dup 2)))
   (set (match_dup 4)
        (ltu:V128J (match_dup 3) (match_dup 2)))
   (set (match_dup 0)
        (ior:V128J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "*usadd<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=&r")
        (us_plus:V128J (match_operand:V128J 1 "register_operand" "r")
                       (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))
   (clobber (match_scratch:V128J 3 "=&r"))
   (clobber (match_scratch:V128J 4 "=&r"))]
  "!HAVE_LVX_US_PLUS_<MODE>"
  "#"
  "!HAVE_LVX_US_PLUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (plus:V128J (match_dup 1) (vec_duplicate:V128J (match_dup 2))))
   (set (match_dup 4)
        (ltu:V128J (match_dup 3) (match_dup 1)))
   (set (match_dup 0)
        (ior:V128J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "usadd<mode>3_2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (us_plus:V128J (match_operand:V128J 1 "register_operand" "r")
                       (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_US_PLUS_<MODE>)"
  "addus<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*usadd<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (us_plus:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                       (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_US_PLUS_<MODE>)"
  "addus<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*usadd<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (us_plus:V128J (match_operand:V128J 1 "register_operand" "r")
                       (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_US_PLUS_<MODE>)"
  "addus<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*addx2<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r")
        (plus:V128K (ashift:V128K (match_operand:V128K 1 "register_operand" "r")
                                  (const_int 1))
                    (match_operand:V128K 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MUL02_ADD_<MODE>)"
  "addx2<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*addx4<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r")
        (plus:V128K (ashift:V128K (match_operand:V128K 1 "register_operand" "r")
                                  (const_int 2))
                    (match_operand:V128K 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MUL04_ADD_<MODE>)"
  "addx4<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*addx8<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r")
        (plus:V128K (ashift:V128K (match_operand:V128K 1 "register_operand" "r")
                                  (const_int 3))
                    (match_operand:V128K 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MUL08_ADD_<MODE>)"
  "addx8<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*addx16<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r")
        (plus:V128K (ashift:V128K (match_operand:V128K 1 "register_operand" "r")
                                  (const_int 4))
                    (match_operand:V128K 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MUL16_ADD_<MODE>)"
  "addx16<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "sub<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (minus:V128J (match_operand:V128J 1 "register_operand" "r")
                     (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "sbf<suffix> %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*sub<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (minus:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                     (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "sbf<suffix> %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*sub<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (minus:V128J (match_operand:V128J 1 "register_operand" "r")
                     (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "sbf<suffix> %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "sssub<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (ss_minus:V128J (match_operand:V128J 1 "register_operand" "r")
                        (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "sbfs<suffix> %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*sssub<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (ss_minus:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                        (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "sbfs<suffix> %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*sssub<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (ss_minus:V128J (match_operand:V128J 1 "register_operand" "r")
                        (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "sbfs<suffix> %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_expand "ussub<mode>3"
  [(match_operand:V128J 0 "register_operand" "")
   (match_operand:V128J 1 "register_operand" "")
   (match_operand:V128J 2 "register_operand" "")]
  ""
  {
    if (!HAVE_LVX_US_MINUS_<MODE>)
      emit_insn (gen_ussub<mode>3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_ussub<mode>3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn_and_split "ussub<mode>3_1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (us_minus:V128J (match_operand:V128J 1 "register_operand" "r")
                        (match_operand:V128J 2 "register_operand" "r")))
   (clobber (match_scratch:V128J 3 "=&r"))]
  "!HAVE_LVX_US_MINUS_<MODE>"
  "#"
  "!HAVE_LVX_US_MINUS_<MODE>"
  [(set (match_dup 3)
        (umin:V128J (match_dup 1) (match_dup 2)))
   (set (match_dup 0)
        (minus:V128J (match_dup 1) (match_dup 3)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
  }
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "*ussub<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=&r")
        (us_minus:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                        (match_operand:V128J 2 "register_operand" "r")))
   (clobber (match_scratch:V128J 3 "=&r"))
   (clobber (match_scratch:V128J 4 "=&r"))]
  "!HAVE_LVX_US_MINUS_<MODE>"
  "#"
  "!HAVE_LVX_US_MINUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (minus:V128J (vec_duplicate:V128J (match_dup 1)) (match_dup 2)))
   (set (match_dup 4)
        (geu:V128J (vec_duplicate:V128J (match_dup 1)) (match_dup 3)))
   (set (match_dup 0)
        (and:V128J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "*ussub<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=&r")
        (us_minus:V128J (match_operand:V128J 1 "register_operand" "r")
                        (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))
   (clobber (match_scratch:V128J 3 "=&r"))
   (clobber (match_scratch:V128J 4 "=&r"))]
  "!HAVE_LVX_US_MINUS_<MODE>"
  "#"
  "!HAVE_LVX_US_MINUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (minus:V128J (match_dup 1) (vec_duplicate:V128J (match_dup 2))))
   (set (match_dup 4)
        (leu:V128J (match_dup 3) (match_dup 1)))
   (set (match_dup 0)
        (and:V128J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "ussub<mode>3_2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (us_minus:V128J (match_operand:V128J 1 "register_operand" "r")
                        (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_US_MINUS_<MODE>)"
  "sbfus<suffix> %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*ussub<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (us_minus:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                        (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_US_MINUS_<MODE>)"
  "sbfus<suffix> %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*ussub<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (us_minus:V128J (match_operand:V128J 1 "register_operand" "r")
                        (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_US_MINUS_<MODE>)"
  "sbfus<suffix> %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*sbfx2<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r")
        (minus:V128K (match_operand:V128K 1 "register_operand" "r")
                     (ashift:V128K (match_operand:V128K 2 "register_operand" "r")
                                   (const_int 1))))]
  "LVX_2 && (HAVE_LVX_MUL02_SUB_<MODE>)"
  "sbfx2<suffix> %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*sbfx4<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r")
        (minus:V128K (match_operand:V128K 1 "register_operand" "r")
                     (ashift:V128K (match_operand:V128K 2 "register_operand" "r")
                                   (const_int 2))))]
  "LVX_2 && (HAVE_LVX_MUL04_SUB_<MODE>)"
  "sbfx4<suffix> %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*sbfx8<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r")
        (minus:V128K (match_operand:V128K 1 "register_operand" "r")
                     (ashift:V128K (match_operand:V128K 2 "register_operand" "r")
                                   (const_int 3))))]
  "LVX_2 && (HAVE_LVX_MUL08_SUB_<MODE>)"
  "sbfx8<suffix> %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*sbfx16<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r")
        (minus:V128K (match_operand:V128K 1 "register_operand" "r")
                     (ashift:V128K (match_operand:V128K 2 "register_operand" "r")
                                   (const_int 4))))]
  "LVX_2 && (HAVE_LVX_MUL16_SUB_<MODE>)"
  "sbfx16<suffix> %0 = %2, %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_expand "div<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "")
        (div:V128J (match_operand:V128J 1 "register_operand" "")
                   (match_operand:V128J 2 "register_operand" "")))]
  ""
  {
    rtx dest = emit_library_call_value (gen_rtx_SYMBOL_REF (Pmode, "__div<mode>3"),
                                        operands[0], LCT_CONST, <MODE>mode,
                                        operands[1], <MODE>mode, operands[2], <MODE>mode);
    if (dest != operands[0])
      emit_move_insn (operands[0], dest);
    DONE;
  }
)

(define_expand "mod<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "")
        (mod:V128J (match_operand:V128J 1 "register_operand" "")
                   (match_operand:V128J 2 "register_operand" "")))]
  ""
  {
    rtx dest = emit_library_call_value (gen_rtx_SYMBOL_REF (Pmode, "__mod<mode>3"),
                                        operands[0], LCT_CONST, <MODE>mode,
                                        operands[1], <MODE>mode, operands[2], <MODE>mode);
    if (dest != operands[0])
      emit_move_insn (operands[0], dest);
    DONE;
  }
)

(define_expand "udiv<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "")
        (udiv:V128J (match_operand:V128J 1 "register_operand" "")
                    (match_operand:V128J 2 "register_operand" "")))]
  ""
  {
    rtx dest = emit_library_call_value (gen_rtx_SYMBOL_REF (Pmode, "__udiv<mode>3"),
                                        operands[0], LCT_CONST, <MODE>mode,
                                        operands[1], <MODE>mode, operands[2], <MODE>mode);
    if (dest != operands[0])
      emit_move_insn (operands[0], dest);
    DONE;
  }
)

(define_expand "umod<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "")
        (umod:V128J (match_operand:V128J 1 "register_operand" "")
                    (match_operand:V128J 2 "register_operand" "")))]
  ""
  {
    rtx dest = emit_library_call_value (gen_rtx_SYMBOL_REF (Pmode, "__umod<mode>3"),
                                        operands[0], LCT_CONST, <MODE>mode,
                                        operands[1], <MODE>mode, operands[2], <MODE>mode);
    if (dest != operands[0])
      emit_move_insn (operands[0], dest);
    DONE;
  }
)

(define_insn "smin<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (smin:V128J (match_operand:V128J 1 "register_operand" "r")
                    (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "min<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*smin<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (smin:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "min<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*smin<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (smin:V128J (match_operand:V128J 1 "register_operand" "r")
                    (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "min<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "smax<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (smax:V128J (match_operand:V128J 1 "register_operand" "r")
                    (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "max<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*smax<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (smax:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "max<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*smax<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (smax:V128J (match_operand:V128J 1 "register_operand" "r")
                    (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "max<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "umin<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (umin:V128J (match_operand:V128J 1 "register_operand" "r")
                    (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "minu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*umin<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (umin:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "minu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*umin<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (umin:V128J (match_operand:V128J 1 "register_operand" "r")
                    (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "minu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "umax<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (umax:V128J (match_operand:V128J 1 "register_operand" "r")
                    (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "maxu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*umax<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (umax:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128J 2 "register_operand" "r")))]
  "LVX_2"
  "maxu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*umax<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (umax:V128J (match_operand:V128J 1 "register_operand" "r")
                    (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "maxu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn_and_split "madd<mode><mode>4"
  [(set (match_operand:V128M 0 "register_operand" "=r")
        (plus:V128M (mult:V128M (match_operand:V128M 1 "register_operand" "r")
                                (match_operand:V128M 2 "register_operand" "r"))
                    (match_operand:V128M 3 "register_operand" "0")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (plus:<HALF> (mult:<HALF> (subreg:<HALF> (match_dup 1) 0)
                                  (subreg:<HALF> (match_dup 2) 0))
                     (subreg:<HALF> (match_dup 3) 0)))
   (set (subreg:<HALF> (match_dup 0) 8)
        (plus:<HALF> (mult:<HALF> (subreg:<HALF> (match_dup 1) 8)
                                  (subreg:<HALF> (match_dup 2) 8))
                     (subreg:<HALF> (match_dup 3) 8)))]
  ""
  [(set_attr "type" "madd_int")
   (set_attr "length"      "8")]
)

(define_insn_and_split "msub<mode><mode>4"
  [(set (match_operand:V128M 0 "register_operand" "=r")
        (minus:V128M (match_operand:V128M 3 "register_operand" "0")
                    (mult:V128M (match_operand:V128M 1 "register_operand" "r")
                                (match_operand:V128M 2 "register_operand" "r"))))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (minus:<HALF> (subreg:<HALF> (match_dup 3) 0)
                      (mult:<HALF> (subreg:<HALF> (match_dup 1) 0)
                                   (subreg:<HALF> (match_dup 2) 0))))
   (set (subreg:<HALF> (match_dup 0) 8)
        (minus:<HALF> (subreg:<HALF> (match_dup 3) 8)
                      (mult:<HALF> (subreg:<HALF> (match_dup 1) 8)
                                   (subreg:<HALF> (match_dup 2) 8))))]
  ""
  [(set_attr "type" "madd_int")
   (set_attr "length"      "8")]
)


;; V128J (V8HI V4SI V2DI)

(define_insn "neg<mode>2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (neg:V128J (match_operand:V128J 1 "register_operand" "r")))]
  "LVX_2"
  "neg<suffix> %0 = %1"
  [(set_attr "type" "alu_lite_x2_x")
   (set_attr "length"          "16")]
)

(define_insn "ssneg<mode>2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (ss_neg:V128J (match_operand:V128J 1 "register_operand" "r")))]
  "LVX_2"
  "sbfs<suffix> %0 = %1, 0"
  [(set_attr "type" "alu_lite_x2_x")
   (set_attr "length"          "16")]
)

(define_insn "abs<mode>2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (abs:V128J (match_operand:V128J 1 "register_operand" "r")))]
  "LVX_2"
  "abs<suffix> %0 = %1"
  [(set_attr "type" "alu_lite_x2_x")
   (set_attr "length"          "16")]
)

(define_expand "ssabs<mode>2"
  [(set (match_operand:V128J 0 "register_operand" "")
        (ss_abs:V128J (match_operand:V128J 1 "register_operand" "")))]
  ""
  ""
)

(define_insn_and_split "ssabs<mode>2_1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (ss_abs:V128J (match_operand:V128J 1 "register_operand" "r")))]
  "!HAVE_LVX_SS_ABS_<MODE>"
  "#"
  "!HAVE_LVX_SS_ABS_<MODE>"
  [(set (match_dup 0)
        (ss_neg:V128J (match_dup 1)))
   (set (match_dup 0)
        (abs:V128J (match_dup 0)))]
  ""
)

(define_insn "ssabs<mode>2_2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (ss_abs:V128J (match_operand:V128J 1 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_SS_ABS_<MODE>)"
  "abss<suffix> %0 = %1"
  [(set_attr "type" "alu_lite_x2_x")
   (set_attr "length"          "16")]
)

(define_insn "clrsb<mode>2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (clrsb:V128J (match_operand:V128J 1 "register_operand" "r")))]
  "LVX_2"
  "cls<suffix> %0 = %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "clz<mode>2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (clz:V128J (match_operand:V128J 1 "register_operand" "r")))]
  "LVX_2"
  "clz<suffix> %0 = %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "ctz<mode>2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (ctz:V128J (match_operand:V128J 1 "register_operand" "r")))]
  "LVX_2"
  "ctz<suffix> %0 = %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "popcount<mode>2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (popcount:V128J (match_operand:V128J 1 "register_operand" "r")))]
  "LVX_2"
  "cbs<suffix> %0 = %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)


;; V128L

(define_insn "and<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (and:V128L (match_operand:V128L 1 "register_operand" "r")
                   (match_operand:V128L 2 "register_operand" "r")))]
  ""
  "andq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*nand<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (ior:V128L (not:V128L (match_operand:V128L 1 "register_operand" "r"))
                   (not:V128L (match_operand:V128L 2 "register_operand" "r"))))]
  ""
  "nandq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*andn<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (and:V128L (not:V128L (match_operand:V128L 1 "register_operand" "r"))
                   (match_operand:V128L 2 "register_operand" "r")))]
  ""
  "andnq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "ior<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (ior:V128L (match_operand:V128L 1 "register_operand" "r")
                   (match_operand:V128L 2 "register_operand" "r")))]
  ""
  "iorq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*nior<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (and:V128L (not:V128L (match_operand:V128L 1 "register_operand" "r"))
                   (not:V128L (match_operand:V128L 2 "register_operand" "r"))))]
  ""
  "niorq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*iorn<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (ior:V128L (not:V128L (match_operand:V128L 1 "register_operand" "r"))
                   (match_operand:V128L 2 "register_operand" "r")))]
  ""
  "iornq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "xor<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (xor:V128L (match_operand:V128L 1 "register_operand" "r")
                   (match_operand:V128L 2 "register_operand" "r")))]
  ""
  "eorq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*nxor<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (not:V128L (xor:V128L (match_operand:V128L 1 "register_operand" "r")
                              (match_operand:V128L 2 "register_operand" "r"))))]
  ""
  "neorq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "one_cmpl<mode>2"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (not:V128L (match_operand:V128L 1 "register_operand" "r")))]
  ""
  "notq %0 = %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

;; fixme: same, should not exist
(define_expand "abd<mode>3"
  [(match_operand:V128L 0 "register_operand" "")
   (match_operand:V128L 1 "register_operand" "")
   (match_operand:V128L 2 "register_operand" "")]
  ""
  {
    if (!HAVE_LVX_ABD_<MODE>)
      emit_insn (gen_abd<mode>3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_abd<mode>3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn "abd<mode>3_1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (minus:V128J (smax:V128J (match_operand:V128J 1 "register_operand" "r")
                                 (match_operand:V128J 2 "register_operand" "r"))
                     (smin:V128J (match_dup 1) (match_dup 2))))]
  "LVX_2"
  "abd<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "abd<mode>3_2"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (minus:V128L (smax:V128L (match_operand:V128L 1 "register_operand" "r")
                                 (match_operand:V128L 2 "register_operand" "r"))
                     (smin:V128L (match_dup 1) (match_dup 2))))]
  "LVX_2"
  "abd<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*abd<suffix>_s1"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (minus:V128L (smax:V128L (vec_duplicate:V128L (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                                 (match_operand:V128L 2 "register_operand" "r"))
                     (smin:V128L (vec_duplicate:V128L (match_dup 1)) (match_dup 2))))]
  "LVX_2"
  "abd<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*abd<suffix>_s2"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (minus:V128L (smax:V128L (match_operand:V128L 1 "register_operand" "r")
                                 (vec_duplicate:V128L (match_operand:<CHUNK> 2 "nonmemory_operand" "r")))
                     (smin:V128L (match_dup 1) (vec_duplicate:V128L (match_dup 2)))))]
  "LVX_2"
  "abd<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_expand "abds<mode>3"
  [(match_operand:V128L 0 "register_operand" "")
   (match_operand:V128L 1 "register_operand" "")
   (match_operand:V128L 2 "register_operand" "")]
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
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (ss_minus:V128J (smax:V128J (match_operand:V128J 1 "register_operand" "r")
                                    (match_operand:V128J 2 "register_operand" "r"))
                        (smin:V128J (match_dup 1) (match_dup 2))))
   (clobber (match_scratch:V128J 3 "=&r"))
   (clobber (match_scratch:V128J 4 "=&r"))]
  "!HAVE_LVX_SS_ABD_<MODE>"
  "#"
  "!HAVE_LVX_SS_ABD_<MODE>"
  [(set (match_dup 3)
        (smax:V128J (match_dup 1) (match_dup 2)))
   (set (match_dup 4)
        (smin:V128J (match_dup 1) (match_dup 2)))
   (set (match_dup 0)
        (ss_minus:V128J (match_dup 3) (match_dup 4)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
  }
)

(define_insn "abds<mode>3_2"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (ss_minus:V128L (smax:V128L (match_operand:V128L 1 "register_operand" "r")
                                    (match_operand:V128L 2 "register_operand" "r"))
                        (smin:V128L (match_dup 1) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_SS_ABD_<MODE>)"
  "abds<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*abds<suffix>_s1"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (ss_minus:V128L (smax:V128L (vec_duplicate:V128L (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                                    (match_operand:V128L 2 "register_operand" "r"))
                        (smin:V128L (vec_duplicate:V128L (match_dup 1)) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_SS_ABD_<MODE>)"
  "abds<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*abds<suffix>_s2"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (ss_minus:V128L (smax:V128L (match_operand:V128L 1 "register_operand" "r")
                                    (vec_duplicate:V128L (match_operand:<CHUNK> 2 "nonmemory_operand" "r")))
                        (smin:V128L (match_dup 1) (vec_duplicate:V128L (match_dup 2)))))]
  "LVX_2 && (HAVE_LVX_SS_ABD_<MODE>)"
  "abds<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_expand "abdu<mode>3"
  [(match_operand:V128L 0 "register_operand" "")
   (match_operand:V128L 1 "register_operand" "")
   (match_operand:V128L 2 "register_operand" "")]
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
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (minus:V128J (umax:V128J (match_operand:V128J 1 "register_operand" "r")
                                 (match_operand:V128J 2 "register_operand" "r"))
                     (umin:V128J (match_dup 1) (match_dup 2))))
   (clobber (match_scratch:V128J 3 "=&r"))
   (clobber (match_scratch:V128J 4 "=&r"))]
  "!HAVE_LVX_UABD_<MODE>"
  "#"
  "!HAVE_LVX_UABD_<MODE>"
  [(set (match_dup 3)
        (umax:V128J (match_dup 1) (match_dup 2)))
   (set (match_dup 4)
        (umin:V128J (match_dup 1) (match_dup 2)))
   (set (match_dup 0)
        (minus:V128J (match_dup 3) (match_dup 4)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
  }
)

(define_insn "abdu<mode>3_2"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (minus:V128L (umax:V128L (match_operand:V128L 1 "register_operand" "r")
                                 (match_operand:V128L 2 "register_operand" "r"))
                     (umin:V128L (match_dup 1) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_UABD_<MODE>)"
  "abdu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*abdu<suffix>_s1"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (minus:V128L (umax:V128L (vec_duplicate:V128L (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                                 (match_operand:V128L 2 "register_operand" "r"))
                     (umin:V128L (vec_duplicate:V128L (match_dup 1)) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_UABD_<MODE>)"
  "abdu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*abdu<suffix>_s2"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (minus:V128L (umax:V128L (match_operand:V128L 1 "register_operand" "r")
                                 (vec_duplicate:V128L (match_operand:<CHUNK> 2 "nonmemory_operand" "r")))
                     (umin:V128L (match_dup 1) (vec_duplicate:V128L (match_dup 2)))))]
  "LVX_2 && (HAVE_LVX_UABD_<MODE>)"
  "abdu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)


;; V4SI

(define_insn "mulv4si3"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (mult:V4SI (match_operand:V4SI 1 "register_operand" "r")
                   (match_operand:V4SI 2 "register_operand" "r")))]
  "LVX_2"
  "mulwq %0 = %1, %2"
  [(set_attr "type" "mulwq_int")]
)

(define_insn "rotlv4si3"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (rotate:V4SI (match_operand:V4SI 1 "register_operand" "r")
                     (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "rolwq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "rotrv4si3"
  [(set (match_operand:V4SI 0 "register_operand" "=r")
        (rotatert:V4SI (match_operand:V4SI 1 "register_operand" "r")
                       (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "rorwq %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)


;; V2DI

(define_insn "ashlv2di3"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (ashift:V2DI (match_operand:V2DI 1 "register_operand" "r")
                     (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "slldp %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "ssashlv2di3"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (ss_ashift:V2DI (match_operand:V2DI 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "slsdp %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_expand "usashlv2di3"
  [(match_operand:V2DI 0 "register_operand" "")
   (match_operand:V2DI 1 "register_operand" "")
   (match_operand:SI 2 "reg_shift_operand" "")]
  ""
  {
    if (!HAVE_LVX_US_ASHIFT_V2DI)
      emit_insn (gen_usashlv2di3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_usashlv2di3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn_and_split "usashlv2di3_1"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (us_ashift:V2DI (match_operand:V2DI 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))
   (clobber (match_scratch:V2DI 3 "=&r"))
   (clobber (match_scratch:V2DI 4 "=&r"))
   (clobber (match_scratch:V2DI 5 "=&r"))]
  "!HAVE_LVX_US_ASHIFT_V2DI"
  "#"
  "!HAVE_LVX_US_ASHIFT_V2DI"
  [(set (match_dup 3)
        (ashift:V2DI (match_dup 1) (match_dup 2)))
   (set (match_dup 4)
        (lshiftrt:V2DI (match_dup 3) (match_dup 2)))
   (set (match_dup 5)
        (ne:V2DI (match_dup 4) (match_dup 1)))
   (set (match_dup 0)
        (ior:V2DI (match_dup 3) (match_dup 5)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (V2DImode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (V2DImode);
    if (GET_CODE (operands[5]) == SCRATCH)
      operands[5] = gen_reg_rtx (V2DImode);
  }
)

(define_insn "usashlv2di3_2"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (us_ashift:V2DI (match_operand:V2DI 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2 && (HAVE_LVX_US_ASHIFT_V2DI)"
  "slusdp %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "ashrv2di3"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (ashiftrt:V2DI (match_operand:V2DI 1 "register_operand" "r")
                       (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "sradp %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "lshrv2di3"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (lshiftrt:V2DI (match_operand:V2DI 1 "register_operand" "r")
                       (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "srldp %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "sshrv2di3"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V2DI 1 "register_operand" "r")
                      (match_operand:SI 2 "reg_shift_operand" "rU06")] UNSPEC_SRS))]
  "LVX_2"
  "srsdp %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)


;; S256I (V16HI V8SI)

(define_expand "ashl<mode>3"
  [(set (match_operand:S256I 0 "register_operand" "")
        (ashift:S256I (match_operand:S256I 1 "register_operand" "")
                      (match_operand:SI 2 "reg_shift_operand" "")))]
  ""
  ""
)

(define_insn_and_split "ashl<mode>3_1"
  [(set (match_operand:S256I 0 "register_operand" "=&r,r")
        (ashift:S256I (match_operand:S256I 1 "register_operand" "r,r")
                      (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  "!HAVE_LVX_ASHIFT_<MODE> && HAVE_LVX_ASHIFT_<HALF>"
  "#"
  "!HAVE_LVX_ASHIFT_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ashift:<HALF> (subreg:<HALF> (match_dup 1) 0)
                       (match_dup 2)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ashift:<HALF> (subreg:<HALF> (match_dup 1) 16)
                       (match_dup 2)))]
  ""
  [(set_attr "type" "alu_lite_x2,alu_lite_x2")]
)

(define_insn "ashl<mode>3_2"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (ashift:S256I (match_operand:S256I 1 "register_operand" "r")
                      (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2 && (HAVE_LVX_ASHIFT_<MODE>)"
  {
    return "sll<hsuffix> %L0 = %L1, %2\n\tsll<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "ssashl<mode>3"
  [(set (match_operand:S256I 0 "register_operand" "=&r,r")
        (ss_ashift:S256I (match_operand:S256I 1 "register_operand" "r,r")
                         (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_ashift:<HALF> (subreg:<HALF> (match_dup 1) 0)
                          (match_dup 2)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_ashift:<HALF> (subreg:<HALF> (match_dup 1) 16)
                          (match_dup 2)))]
  ""
  [(set_attr "type" "alu_lite_x2,alu_lite_x2")]
)

(define_expand "usashl<mode>3"
  [(match_operand:S256I 0 "register_operand" "")
   (match_operand:S256I 1 "register_operand" "")
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
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (us_ashift:S256I (match_operand:S256I 1 "register_operand" "r")
                         (match_operand:SI 2 "reg_shift_operand" "rU06")))
   (clobber (match_scratch:S256I 3 "=&r"))
   (clobber (match_scratch:S256I 4 "=&r"))
   (clobber (match_scratch:S256I 5 "=&r"))]
  "!HAVE_LVX_US_ASHIFT_<MODE>"
  "#"
  "!HAVE_LVX_US_ASHIFT_<MODE>"
  [(set (match_dup 3)
        (ashift:S256I (match_dup 1) (match_dup 2)))
   (set (match_dup 4)
        (lshiftrt:S256I (match_dup 3) (match_dup 2)))
   (set (match_dup 5)
        (ne:S256I (match_dup 4) (match_dup 1)))
   (set (match_dup 0)
        (ior:S256I (match_dup 3) (match_dup 5)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[5]) == SCRATCH)
      operands[5] = gen_reg_rtx (<MODE>mode);
  }
)

(define_insn_and_split "usashl<mode>3_2"
  [(set (match_operand:S256I 0 "register_operand" "=&r,r")
        (us_ashift:S256I (match_operand:S256I 1 "register_operand" "r,r")
                         (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  "HAVE_LVX_US_ASHIFT_<MODE>"
  "#"
  "HAVE_LVX_US_ASHIFT_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (us_ashift:<HALF> (subreg:<HALF> (match_dup 1) 0)
                          (match_dup 2)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (us_ashift:<HALF> (subreg:<HALF> (match_dup 1) 16)
                          (match_dup 2)))]
  ""
  [(set_attr "type" "alu_lite_x2,alu_lite_x2")]
)

(define_expand "ashr<mode>3"
  [(set (match_operand:S256I 0 "register_operand" "")
        (ashiftrt:S256I (match_operand:S256I 1 "register_operand" "")
                        (match_operand:SI 2 "reg_shift_operand" "")))]
  ""
  ""
)

(define_insn_and_split "ashr<mode>3_1"
  [(set (match_operand:S256I 0 "register_operand" "=&r,r")
        (ashiftrt:S256I (match_operand:S256I 1 "register_operand" "r,r")
                        (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  "!HAVE_LVX_ASHIFTRT_<MODE> && HAVE_LVX_ASHIFTRT_<HALF>"
  "#"
  "!HAVE_LVX_ASHIFTRT_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ashiftrt:<HALF> (subreg:<HALF> (match_dup 1) 0)
                         (match_dup 2)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ashiftrt:<HALF> (subreg:<HALF> (match_dup 1) 16)
                         (match_dup 2)))]
  ""
  [(set_attr "type" "alu_lite_x2,alu_lite_x2")]
)

(define_insn "ashr<mode>3_2"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (ashiftrt:S256I (match_operand:S256I 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2 && (HAVE_LVX_ASHIFTRT_<MODE>)"
  {
    return "sra<hsuffix> %L0 = %L1, %2\n\tsra<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "lshr<mode>3"
  [(set (match_operand:S256I 0 "register_operand" "")
        (lshiftrt:S256I (match_operand:S256I 1 "register_operand" "")
                        (match_operand:SI 2 "reg_shift_operand" "")))]
  ""
  ""
)

(define_insn_and_split "lshr<mode>3_1"
  [(set (match_operand:S256I 0 "register_operand" "=&r,r")
        (lshiftrt:S256I (match_operand:S256I 1 "register_operand" "r,r")
                        (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  "!HAVE_LVX_LSHIFTRT_<MODE> && HAVE_LVX_LSHIFTRT_<HALF>"
  "#"
  "!HAVE_LVX_LSHIFTRT_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (lshiftrt:<HALF> (subreg:<HALF> (match_dup 1) 0)
                         (match_dup 2)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (lshiftrt:<HALF> (subreg:<HALF> (match_dup 1) 16)
                         (match_dup 2)))]
  ""
  [(set_attr "type" "alu_lite_x2,alu_lite_x2")]
)

(define_insn "lshr<mode>3_2"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (lshiftrt:S256I (match_operand:S256I 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2 && (HAVE_LVX_LSHIFTRT_<MODE>)"
  {
    return "srl<hsuffix> %L0 = %L1, %2\n\tsrl<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "sshr<mode>3"
  [(set (match_operand:S256I 0 "register_operand" "=&r,r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r,r")
                       (match_operand:SI 2 "reg_shift_operand" "r,U06")] UNSPEC_SRS))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (match_dup 2)] UNSPEC_SRS))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (match_dup 2)] UNSPEC_SRS))]
  ""
  [(set_attr "type" "alu_lite_x2,alu_lite_x2")]
)

(define_expand "avg<mode>3_floor"
  [(set (match_operand:S256I 0 "register_operand" "")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "")
                       (match_operand:S256I 2 "register_operand" "")] UNSPEC_AVG))]
  ""
  ""
)

(define_insn_and_split "avg<mode>3_floor_1"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r")
                       (match_operand:S256I 2 "register_operand" "r")] UNSPEC_AVG))]
  "!HAVE_LVX_AVG_<MODE> && HAVE_LVX_AVG_<HALF>"
  "#"
  "!HAVE_LVX_AVG_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)] UNSPEC_AVG))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)] UNSPEC_AVG))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "avg<mode>3_floor_2"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r")
                       (match_operand:S256I 2 "register_operand" "r")] UNSPEC_AVG))]
  "LVX_2 && (HAVE_LVX_AVG_<MODE>)"
  {
    return "avg<hsuffix> %L0 = %L1, %L2\n\tavg<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "avg<mode>3_ceil"
  [(set (match_operand:S256I 0 "register_operand" "")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "")
                       (match_operand:S256I 2 "register_operand" "")] UNSPEC_AVGR))]
  ""
  ""
)

(define_insn_and_split "avg<mode>3_ceil_1"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r")
                       (match_operand:S256I 2 "register_operand" "r")] UNSPEC_AVGR))]
  "!HAVE_LVX_CEIL_AVG_<MODE> && HAVE_LVX_CEIL_AVG_<HALF>"
  "#"
  "!HAVE_LVX_CEIL_AVG_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)] UNSPEC_AVGR))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)] UNSPEC_AVGR))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "avg<mode>3_ceil_2"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r")
                       (match_operand:S256I 2 "register_operand" "r")] UNSPEC_AVGR))]
  "LVX_2 && (HAVE_LVX_CEIL_AVG_<MODE>)"
  {
    return "avgr<hsuffix> %L0 = %L1, %L2\n\tavgr<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "uavg<mode>3_floor"
  [(set (match_operand:S256I 0 "register_operand" "")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "")
                       (match_operand:S256I 2 "register_operand" "")] UNSPEC_AVGU))]
  ""
  ""
)

(define_insn_and_split "uavg<mode>3_floor_1"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r")
                       (match_operand:S256I 2 "register_operand" "r")] UNSPEC_AVGU))]
  "!HAVE_LVX_UAVG_<MODE> && HAVE_LVX_UAVG_<HALF>"
  "#"
  "!HAVE_LVX_UAVG_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)] UNSPEC_AVGU))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)] UNSPEC_AVGU))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "uavg<mode>3_floor_2"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r")
                       (match_operand:S256I 2 "register_operand" "r")] UNSPEC_AVGU))]
  "LVX_2 && (HAVE_LVX_UAVG_<MODE>)"
  {
    return "avgu<hsuffix> %L0 = %L1, %L2\n\tavgu<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "uavg<mode>3_ceil"
  [(set (match_operand:S256I 0 "register_operand" "")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "")
                       (match_operand:S256I 2 "register_operand" "")] UNSPEC_AVGRU))]
  ""
  ""
)

(define_insn_and_split "uavg<mode>3_ceil_1"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r")
                       (match_operand:S256I 2 "register_operand" "r")] UNSPEC_AVGRU))]
  "!HAVE_LVX_CEIL_UAVG_<MODE> && HAVE_LVX_CEIL_UAVG_<HALF>"
  "#"
  "!HAVE_LVX_CEIL_UAVG_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)] UNSPEC_AVGRU))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)] UNSPEC_AVGRU))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "uavg<mode>3_ceil_2"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r")
                       (match_operand:S256I 2 "register_operand" "r")] UNSPEC_AVGRU))]
  "LVX_2 && (HAVE_LVX_CEIL_UAVG_<MODE>)"
  {
    return "avgru<hsuffix> %L0 = %L1, %L2\n\tavgru<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "extend<mode><wide>2"
  [(set (match_operand:<WIDE> 0 "register_operand" "")
        (sign_extend:<WIDE> (match_operand:S256L 1 "register_operand" "")))]
  ""
  {
    emit_insn (gen_lvx_sx<widenx> (operands[0], operands[1]));
    DONE;
  }
)

(define_expand "zero_extend<mode><wide>2"
  [(set (match_operand:<WIDE> 0 "register_operand" "")
        (zero_extend:<WIDE> (match_operand:S256L 1 "register_operand" "")))]
  ""
  {
    emit_insn (gen_lvx_zx<widenx> (operands[0], operands[1]));
    DONE;
  }
)


;; V256I (V16HI V4DI)

(define_insn_and_split "mul<mode>3"
  [(set (match_operand:V256I 0 "register_operand" "=r")
        (mult:V256I (match_operand:V256I 1 "register_operand" "r")
                    (match_operand:V256I 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 0)
                      (subreg:<CHUNK> (match_dup 2) 0)))
   (set (subreg:<CHUNK> (match_dup 0) 8)
        (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 8)
                      (subreg:<CHUNK> (match_dup 2) 8)))
   (set (subreg:<CHUNK> (match_dup 0) 16)
        (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 16)
                      (subreg:<CHUNK> (match_dup 2) 16)))
   (set (subreg:<CHUNK> (match_dup 0) 24)
        (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 24)
                      (subreg:<CHUNK> (match_dup 2) 24)))]
  ""
  [(set_attr "type" "madd_int")]
)

(define_insn_and_split "*mul<mode>3_s1"
  [(set (match_operand:V256I 0 "register_operand" "=&r")
        (mult:V256I (vec_duplicate:V256I (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256I 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (mult:<CHUNK> (match_dup 1)
                      (subreg:<CHUNK> (match_dup 2) 0)))
   (set (subreg:<CHUNK> (match_dup 0) 8)
        (mult:<CHUNK> (match_dup 1)
                      (subreg:<CHUNK> (match_dup 2) 8)))
   (set (subreg:<CHUNK> (match_dup 0) 16)
        (mult:<CHUNK> (match_dup 1)
                      (subreg:<CHUNK> (match_dup 2) 16)))
   (set (subreg:<CHUNK> (match_dup 0) 24)
        (mult:<CHUNK> (match_dup 1)
                      (subreg:<CHUNK> (match_dup 2) 24)))]
  ""
  [(set_attr "type" "madd_int")]
)

(define_insn_and_split "*mul<mode>3_s2"
  [(set (match_operand:V256I 0 "register_operand" "=&r")
        (mult:V256I (match_operand:V256I 1 "register_operand" "r")
                    (vec_duplicate:V256I (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 0)
                      (match_dup 2)))
   (set (subreg:<CHUNK> (match_dup 0) 8)
        (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 8)
                      (match_dup 2)))
   (set (subreg:<CHUNK> (match_dup 0) 16)
        (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 16)
                      (match_dup 2)))
   (set (subreg:<CHUNK> (match_dup 0) 24)
        (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 24)
                      (match_dup 2)))]
  ""
  [(set_attr "type" "madd_int")]
)


;; V256J (V16HI V8SI V4DI)

(define_insn "add<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (plus:V256J (match_operand:V256J 1 "register_operand" "r")
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "add<hsuffix> %L0 = %L1, %L2\n\tadd<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*add<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (plus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "add<hsuffix> %L0 = %1, %L2\n\tadd<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*add<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (plus:V256J (match_operand:V256J 1 "register_operand" "r")
                    (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  {
    return "add<hsuffix> %L0 = %L1, %2\n\tadd<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "ssadd<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "")
        (ss_plus:V256J (match_operand:V256J 1 "register_operand" "")
                       (match_operand:V256J 2 "register_operand" "")))]
  ""
  ""
)

(define_insn_and_split "ssadd<mode>3_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_plus:V256J (match_operand:V256J 1 "register_operand" "r")
                       (match_operand:V256J 2 "register_operand" "r")))]
  "!HAVE_LVX_SS_PLUS_<MODE>"
  "#"
  "!HAVE_LVX_SS_PLUS_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_plus:<HALF> (subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_plus:<HALF> (subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "ssadd<mode>3_2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_plus:V256J (match_operand:V256J 1 "register_operand" "r")
                       (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_SS_PLUS_<MODE>)"
  {
    return "adds<hsuffix> %L0 = %L1, %L2\n\tadds<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*ssadd<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (ss_plus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                       (match_operand:V256J 2 "register_operand" "r")))]
  "!HAVE_LVX_SS_PLUS_<MODE> && HAVE_LVX_SS_PLUS_<HALF>"
  "#"
  "!HAVE_LVX_SS_PLUS_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_plus:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                        (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_plus:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                        (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*ssadd<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_plus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_HAVE_SS_PLUS_<MODE>)"
  {
    return "adds<hsuffix> %L0 = %1, %L2\n\tadds<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*ssadd<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (ss_plus:V256J (match_operand:V256J 1 "register_operand" "r")
                       (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "!HAVE_LVX_SS_PLUS_<MODE> && HAVE_LVX_SS_PLUS_<HALF>"
  "#"
  "!HAVE_LVX_SS_PLUS_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_plus:<HALF> (subreg:<HALF> (match_dup 1) 0)
                        (vec_duplicate:<HALF> (match_dup 2))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_plus:<HALF> (subreg:<HALF> (match_dup 1) 16)
                        (vec_duplicate:<HALF> (match_dup 2))))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*ssadd<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_plus:V256J (match_operand:V256J 1 "register_operand" "r")
                    (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_SS_PLUS_<MODE>)"
  {
    return "adds<hsuffix> %L0 = %L1, %2\n\tadds<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "usadd<mode>3"
  [(match_operand:V256J 0 "register_operand" "")
   (match_operand:V256J 1 "register_operand" "")
   (match_operand:V256J 2 "register_operand" "")]
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
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (us_plus:V256J (match_operand:V256J 1 "register_operand" "r")
                       (match_operand:V256J 2 "register_operand" "r")))
   (clobber (match_scratch:V256J 3 "=&r"))
   (clobber (match_scratch:V256J 4 "=&r"))]
  "!HAVE_LVX_US_PLUS_<MODE>"
  "#"
  "!HAVE_LVX_US_PLUS_<MODE>"
  [(set (match_dup 3)
        (plus:V256J (match_dup 1) (match_dup 2)))
   (set (match_dup 4)
        (ltu:V256J (match_dup 3) (match_dup 1)))
   (set (match_dup 0)
        (ior:V256J (match_dup 3) (match_dup 4)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
  }
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "usadd<mode>3_2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (us_plus:V256J (match_operand:V256J 1 "register_operand" "r")
                       (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_US_PLUS_<MODE>)"
  {
    return "addus<hsuffix> %L0 = %L1, %L2\n\taddus<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*usadd<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (us_plus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                       (match_operand:V256J 2 "register_operand" "r")))
   (clobber (match_scratch:V256J 3 "=&r"))
   (clobber (match_scratch:V256J 4 "=&r"))]
  "!HAVE_LVX_US_PLUS_<MODE>"
  "#"
  "!HAVE_LVX_US_PLUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (plus:V256J (vec_duplicate:V256J (match_dup 1)) (match_dup 2)))
   (set (match_dup 4)
        (ltu:V256J (match_dup 3) (match_dup 2)))
   (set (match_dup 0)
        (ior:V256J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*usadd<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (us_plus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                       (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_US_PLUS_<MODE>)"
  {
    return "addus<hsuffix> %L0 = %1, %L2\n\taddus<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*usadd<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (us_plus:V256J (match_operand:V256J 1 "register_operand" "r")
                       (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))
   (clobber (match_scratch:V256J 3 "=&r"))
   (clobber (match_scratch:V256J 4 "=&r"))]
  "!HAVE_LVX_US_PLUS_<MODE>"
  "#"
  "!HAVE_LVX_US_PLUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (plus:V256J (match_dup 1) (vec_duplicate:V256J (match_dup 2))))
   (set (match_dup 4)
        (ltu:V256J (match_dup 3) (match_dup 1)))
   (set (match_dup 0)
        (ior:V256J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*usadd<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (us_plus:V256J (match_operand:V256J 1 "register_operand" "r")
                       (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_US_PLUS_<MODE>)"
  {
    return "addus<hsuffix> %L0 = %L1, %2\n\taddus<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*addx2<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r")
                                  (const_int 1))
                    (match_operand:V256K 2 "register_operand" "r")))]
  "!HAVE_LVX_MUL02_ADD_<MODE> && HAVE_LVX_MUL02_ADD_<HALF>"
  "#"
  "!HAVE_LVX_MUL02_ADD_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (plus:<HALF> (ashift:<HALF> (subreg:<HALF> (match_dup 1) 0)
                                    (const_int 1))
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (plus:<HALF> (ashift:<HALF> (subreg:<HALF> (match_dup 1) 16)
                                    (const_int 1))
                     (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*addx2<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r")
                                  (const_int 1))
                    (match_operand:V256K 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MUL02_ADD_<MODE>)"
  {
    return "addx2<hsuffix> %L0 = %L1, %L2\n\taddx2<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*addx4<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r")
                                  (const_int 2))
                    (match_operand:V256K 2 "register_operand" "r")))]
  "!HAVE_LVX_MUL04_ADD_<MODE> && HAVE_LVX_MUL04_ADD_<HALF>"
  "#"
  "!HAVE_LVX_MUL04_ADD_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (plus:<HALF> (ashift:<HALF> (subreg:<HALF> (match_dup 1) 0)
                                    (const_int 2))
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (plus:<HALF> (ashift:<HALF> (subreg:<HALF> (match_dup 1) 16)
                                    (const_int 2))
                     (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*addx4<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r")
                                  (const_int 2))
                    (match_operand:V256K 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MUL04_ADD_<MODE>)"
  {
    return "addx4<hsuffix> %L0 = %L1, %L2\n\taddx4<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*addx8<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r")
                                  (const_int 3))
                    (match_operand:V256K 2 "register_operand" "r")))]
  "!HAVE_LVX_MUL08_ADD_<MODE> && HAVE_LVX_MUL08_ADD_<HALF>"
  "#"
  "!HAVE_LVX_MUL08_ADD_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (plus:<HALF> (ashift:<HALF> (subreg:<HALF> (match_dup 1) 0)
                                    (const_int 3))
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (plus:<HALF> (ashift:<HALF> (subreg:<HALF> (match_dup 1) 16)
                                    (const_int 3))
                     (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*addx8<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r")
                                  (const_int 3))
                    (match_operand:V256K 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MUL08_ADD_<MODE>)"
  {
    return "addx8<hsuffix> %L0 = %L1, %L2\n\taddx8<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*addx16<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r")
                                  (const_int 4))
                    (match_operand:V256K 2 "register_operand" "r")))]
  "!HAVE_LVX_MUL16_ADD_<MODE> && HAVE_LVX_MUL16_ADD_<HALF>"
  "#"
  "!HAVE_LVX_MUL16_ADD_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (plus:<HALF> (ashift:<HALF> (subreg:<HALF> (match_dup 1) 0)
                                    (const_int 4))
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (plus:<HALF> (ashift:<HALF> (subreg:<HALF> (match_dup 1) 16)
                                    (const_int 4))
                     (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*addx16<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r")
                                  (const_int 4))
                    (match_operand:V256K 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MUL16_ADD_<MODE>)"
  {
    return "addx16<hsuffix> %L0 = %L1, %L2\n\taddx16<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "sub<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (minus:V256J (match_operand:V256J 1 "register_operand" "r")
                     (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "sbf<hsuffix> %L0 = %L2, %L1\n\tsbf<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*sub<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (minus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                     (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "sbf<hsuffix> %L0 = %L2, %1\n\tsbf<hsuffix> %M0 = %M2, %1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*sub<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (minus:V256J (match_operand:V256J 1 "register_operand" "r")
                     (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  {
    return "sbf<hsuffix> %L0 = %2, %L1\n\tsbf<hsuffix> %M0 = %2, %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "sssub<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "")
        (ss_minus:V256J (match_operand:V256J 1 "register_operand" "")
                        (match_operand:V256J 2 "register_operand" "")))]
  ""
  ""
)

(define_insn_and_split "sssub<mode>3_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_minus:V256J (match_operand:V256J 1 "register_operand" "r")
                        (match_operand:V256J 2 "register_operand" "r")))]
  "!HAVE_LVX_SS_MINUS_<MODE> && HAVE_LVX_SS_MINUS_<HALF>"
  "#"
  "!HAVE_LVX_SS_MINUS_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_minus:<HALF> (subreg:<HALF> (match_dup 1) 0)
                         (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_minus:<HALF> (subreg:<HALF> (match_dup 1) 16)
                         (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "sssub<mode>3_2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_minus:V256J (match_operand:V256J 1 "register_operand" "r")
                        (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_SS_MINUS_<MODE>)"
  {
    return "sbfs<hsuffix> %L0 = %L2, %L1\n\tsbfs<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*sssub<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (ss_minus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                        (match_operand:V256J 2 "register_operand" "r")))]
  "!HAVE_LVX_SS_MINUS_<MODE> && HAVE_LVX_SS_MINUS_<HALF>"
  "#"
  "!HAVE_LVX_SS_MINUS_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_minus:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                         (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_minus:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                         (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*sssub<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_minus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                     (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_SS_MINUS_<MODE>)"
  {
    return "sbfs<hsuffix> %L0 = %L2, %1\n\tsbfs<hsuffix> %M0 = %M2, %1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*sssub<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (ss_minus:V256J (match_operand:V256J 1 "register_operand" "r")
                        (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "!HAVE_LVX_SS_MINUS_<MODE> && HAVE_LVX_SS_MINUS_<HALF>"
  "#"
  "!HAVE_LVX_SS_MINUS_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_minus:<HALF> (subreg:<HALF> (match_dup 1) 0)
                         (vec_duplicate:<HALF> (match_dup 2))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_minus:<HALF> (subreg:<HALF> (match_dup 1) 16)
                         (vec_duplicate:<HALF> (match_dup 2))))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*sssub<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_minus:V256J (match_operand:V256J 1 "register_operand" "r")
                     (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_SS_MINUS_<MODE>)"
  {
    return "sbfs<hsuffix> %L0 = %2, %L1\n\tsbfs<hsuffix> %M0 = %2, %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "ussub<mode>3"
  [(match_operand:V256J 0 "register_operand" "")
   (match_operand:V256J 1 "register_operand" "")
   (match_operand:V256J 2 "register_operand" "")]
  ""
  {
    if (!HAVE_LVX_US_MINUS_<MODE>)
      emit_insn (gen_ussub<mode>3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_ussub<mode>3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn_and_split "ussub<mode>3_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (us_minus:V256J (match_operand:V256J 1 "register_operand" "r")
                        (match_operand:V256J 2 "register_operand" "r")))
   (clobber (match_scratch:V256J 3 "=&r"))]
  "!HAVE_LVX_US_MINUS_<MODE>"
  "#"
  "!HAVE_LVX_US_MINUS_<MODE>"
  [(set (match_dup 3)
        (umin:V256J (match_dup 1) (match_dup 2)))
   (set (match_dup 0)
        (minus:V256J (match_dup 1) (match_dup 3)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
  }
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "ussub<mode>3_2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (us_minus:V256J (match_operand:V256J 1 "register_operand" "r")
                        (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_US_MINUS_<MODE>)"
  {
    return "sbfus<hsuffix> %L0 = %L2, %L1\n\tsbfus<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*ussub<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (us_minus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                        (match_operand:V256J 2 "register_operand" "r")))
   (clobber (match_scratch:V256J 3 "=&r"))
   (clobber (match_scratch:V256J 4 "=&r"))]
  "!HAVE_LVX_US_MINUS_<MODE>"
  "#"
  "!HAVE_LVX_US_MINUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (minus:V256J (vec_duplicate:V256J (match_dup 1)) (match_dup 2)))
   (set (match_dup 4)
        (geu:V256J (vec_duplicate:V256J (match_dup 1)) (match_dup 3)))
   (set (match_dup 0)
        (and:V256J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*ussub<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (us_minus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                        (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_US_MINUS_<MODE>)"
  {
    return "sbfus<hsuffix> %L0 = %L2, %1\n\tsbfus<hsuffix> %M0 = %M2, %1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*ussub<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (us_minus:V256J (match_operand:V256J 1 "register_operand" "r")
                        (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))
   (clobber (match_scratch:V256J 3 "=&r"))
   (clobber (match_scratch:V256J 4 "=&r"))]
  "!HAVE_LVX_US_MINUS_<MODE>"
  "#"
  "!HAVE_LVX_US_MINUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (minus:V256J (match_dup 1) (vec_duplicate:V256J (match_dup 2))))
   (set (match_dup 4)
        (leu:V256J (match_dup 3) (match_dup 1)))
   (set (match_dup 0)
        (and:V256J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*ussub<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (us_minus:V256J (match_operand:V256J 1 "register_operand" "r")
                        (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_US_MINUS_<MODE>)"
  {
    return "sbfus<hsuffix> %L0 = %2, %L1\n\tsbfus<hsuffix> %M0 = %2, %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*sbfx2<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (minus:V256K (match_operand:V256K 1 "register_operand" "r")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r")
                                   (const_int 1))))]
  "!HAVE_LVX_MUL02_SUB_<MODE> && HAVE_LVX_MUL02_SUB_<HALF>"
  "#"
  "!HAVE_LVX_MUL02_SUB_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (minus:<HALF> (subreg:<HALF> (match_dup 1) 0)
                      (ashift:<HALF> (subreg:<HALF> (match_dup 2) 0)
                                     (const_int 1))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (minus:<HALF> (subreg:<HALF> (match_dup 1) 16)
                      (ashift:<HALF> (subreg:<HALF> (match_dup 2) 16)
                                     (const_int 1))))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*sbfx2<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (minus:V256K (match_operand:V256K 1 "register_operand" "r")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r")
                                   (const_int 1))))]
  "LVX_2 && (HAVE_LVX_MUL02_SUB_<MODE>)"
  {
    return "sbfx2<hsuffix> %L0 = %L2, %L1\n\tsbfx2<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*sbfx4<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (minus:V256K (match_operand:V256K 1 "register_operand" "r")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r")
                                   (const_int 2))))]
  "!HAVE_LVX_MUL04_SUB_<MODE> && HAVE_LVX_MUL04_SUB_<HALF>"
  "#"
  "!HAVE_LVX_MUL04_SUB_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (minus:<HALF> (subreg:<HALF> (match_dup 1) 0)
                      (ashift:<HALF> (subreg:<HALF> (match_dup 2) 0)
                                     (const_int 2))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (minus:<HALF> (subreg:<HALF> (match_dup 1) 16)
                      (ashift:<HALF> (subreg:<HALF> (match_dup 2) 16)
                                     (const_int 2))))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*sbfx4<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (minus:V256K (match_operand:V256K 1 "register_operand" "r")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r")
                                   (const_int 2))))]
  "LVX_2 && (HAVE_LVX_MUL04_SUB_<MODE>)"
  {
    return "sbfx4<hsuffix> %L0 = %L2, %L1\n\tsbfx4<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*sbfx8<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (minus:V256K (match_operand:V256K 1 "register_operand" "r")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r")
                                   (const_int 3))))]
  "!HAVE_LVX_MUL08_SUB_<MODE> && HAVE_LVX_MUL08_SUB_<HALF>"
  "#"
  "!HAVE_LVX_MUL08_SUB_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (minus:<HALF> (subreg:<HALF> (match_dup 1) 0)
                      (ashift:<HALF> (subreg:<HALF> (match_dup 2) 0)
                                     (const_int 3))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (minus:<HALF> (subreg:<HALF> (match_dup 1) 16)
                      (ashift:<HALF> (subreg:<HALF> (match_dup 2) 16)
                                     (const_int 3))))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*sbfx8<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (minus:V256K (match_operand:V256K 1 "register_operand" "r")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r")
                                   (const_int 3))))]
  "LVX_2 && (HAVE_LVX_MUL08_SUB_<MODE>)"
  {
    return "sbfx8<hsuffix> %L0 = %L2, %L1\n\tsbfx8<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*sbfx16<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (minus:V256K (match_operand:V256K 1 "register_operand" "r")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r")
                                   (const_int 4))))]
  "!HAVE_LVX_MUL16_SUB_<MODE> && HAVE_LVX_MUL16_SUB_<HALF>"
  "#"
  "!HAVE_LVX_MUL16_SUB_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (minus:<HALF> (subreg:<HALF> (match_dup 1) 0)
                      (ashift:<HALF> (subreg:<HALF> (match_dup 2) 0)
                                     (const_int 4))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (minus:<HALF> (subreg:<HALF> (match_dup 1) 16)
                      (ashift:<HALF> (subreg:<HALF> (match_dup 2) 16)
                                     (const_int 4))))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*sbfx16<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (minus:V256K (match_operand:V256K 1 "register_operand" "r")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r")
                                   (const_int 4))))]
  "LVX_2 && (HAVE_LVX_MUL16_SUB_<MODE>)"
  {
    return "sbfx16<hsuffix> %L0 = %L2, %L1\n\tsbfx16<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "div<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "")
        (div:V256J (match_operand:V256J 1 "register_operand" "")
                   (match_operand:V256J 2 "register_operand" "")))]
  ""
  {
    rtx dest = emit_library_call_value (gen_rtx_SYMBOL_REF (Pmode, "__div<mode>3"),
                                        operands[0], LCT_CONST, <MODE>mode,
                                        operands[1], <MODE>mode, operands[2], <MODE>mode);
    if (dest != operands[0])
      emit_move_insn (operands[0], dest);
    DONE;
  }
)

(define_expand "mod<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "")
        (mod:V256J (match_operand:V256J 1 "register_operand" "")
                   (match_operand:V256J 2 "register_operand" "")))]
  ""
  {
    rtx dest = emit_library_call_value (gen_rtx_SYMBOL_REF (Pmode, "__mod<mode>3"),
                                        operands[0], LCT_CONST, <MODE>mode,
                                        operands[1], <MODE>mode, operands[2], <MODE>mode);
    if (dest != operands[0])
      emit_move_insn (operands[0], dest);
    DONE;
  }
)

(define_expand "udiv<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "")
        (udiv:V256J (match_operand:V256J 1 "register_operand" "")
                    (match_operand:V256J 2 "register_operand" "")))]
  ""
  {
    rtx dest = emit_library_call_value (gen_rtx_SYMBOL_REF (Pmode, "__udiv<mode>3"),
                                        operands[0], LCT_CONST, <MODE>mode,
                                        operands[1], <MODE>mode, operands[2], <MODE>mode);
    if (dest != operands[0])
      emit_move_insn (operands[0], dest);
    DONE;
  }
)

(define_expand "umod<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "")
        (umod:V256J (match_operand:V256J 1 "register_operand" "")
                    (match_operand:V256J 2 "register_operand" "")))]
  ""
  {
    rtx dest = emit_library_call_value (gen_rtx_SYMBOL_REF (Pmode, "__umod<mode>3"),
                                        operands[0], LCT_CONST, <MODE>mode,
                                        operands[1], <MODE>mode, operands[2], <MODE>mode);
    if (dest != operands[0])
      emit_move_insn (operands[0], dest);
    DONE;
  }
)

(define_insn "smin<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (smin:V256J (match_operand:V256J 1 "register_operand" "r")
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "min<hsuffix> %L0 = %L1, %L2\n\tmin<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*smin<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (smin:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "min<hsuffix> %L0 = %1, %L2\n\tmin<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*smin<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (smin:V256J (match_operand:V256J 1 "register_operand" "r")
                    (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  {
    return "min<hsuffix> %L0 = %L1, %2\n\tmin<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "smax<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (smax:V256J (match_operand:V256J 1 "register_operand" "r")
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "max<hsuffix> %L0 = %L1, %L2\n\tmax<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*smax<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (smax:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "max<hsuffix> %L0 = %1, %L2\n\tmax<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*smax<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (smax:V256J (match_operand:V256J 1 "register_operand" "r")
                    (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  {
    return "max<hsuffix> %L0 = %L1, %2\n\tmax<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "umin<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (umin:V256J (match_operand:V256J 1 "register_operand" "r")
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "minu<hsuffix> %L0 = %L1, %L2\n\tminu<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*umin<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (umin:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "minu<hsuffix> %L0 = %1, %L2\n\tminu<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*umin<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (umin:V256J (match_operand:V256J 1 "register_operand" "r")
                    (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  {
    return "minu<hsuffix> %L0 = %L1, %2\n\tminu<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "umax<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (umax:V256J (match_operand:V256J 1 "register_operand" "r")
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "maxu<hsuffix> %L0 = %L1, %L2\n\tmaxu<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*umax<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (umax:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "maxu<hsuffix> %L0 = %1, %L2\n\tmaxu<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*umax<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (umax:V256J (match_operand:V256J 1 "register_operand" "r")
                    (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  {
    return "maxu<hsuffix> %L0 = %L1, %2\n\tmaxu<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)


;; V256L

(define_insn "and<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (and:V256L (match_operand:V256L 1 "register_operand" "r")
                   (match_operand:V256L 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "andq %L0 = %L1, %L2\n\tandq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*nand<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (ior:V256L (not:V256L (match_operand:V256L 1 "register_operand" "r"))
                   (not:V256L (match_operand:V256L 2 "register_operand" "r"))))]
  "LVX_2"
  {
    return "nandq %L0 = %L1, %L2\n\tnandq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*andn<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (and:V256L (not:V256L (match_operand:V256L 1 "register_operand" "r"))
                   (match_operand:V256L 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "andnq %L0 = %L1, %L2\n\tandnq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "ior<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (ior:V256L (match_operand:V256L 1 "register_operand" "r")
                   (match_operand:V256L 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "iorq %L0 = %L1, %L2\n\tiorq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*nior<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (and:V256L (not:V256L (match_operand:V256L 1 "register_operand" "r"))
                   (not:V256L (match_operand:V256L 2 "register_operand" "r"))))]
  "LVX_2"
  {
    return "niorq %L0 = %L1, %L2\n\tniorq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*iorn<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (ior:V256L (not:V256L (match_operand:V256L 1 "register_operand" "r"))
                   (match_operand:V256L 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "iornq %L0 = %L1, %L2\n\tiornq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "xor<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (xor:V256L (match_operand:V256L 1 "register_operand" "r")
                   (match_operand:V256L 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "eorq %L0 = %L1, %L2\n\teorq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*nxor<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (not:V256L (xor:V256L (match_operand:V256L 1 "register_operand" "r")
                              (match_operand:V256L 2 "register_operand" "r"))))]
  "LVX_2"
  {
    return "neorq %L0 = %L1, %L2\n\tneorq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)


;; V256M

(define_insn_and_split "madd<mode><mode>4"
  [(set (match_operand:V256M 0 "register_operand" "=r")
        (plus:V256M (mult:V256M (match_operand:V256M 1 "register_operand" "r")
                                (match_operand:V256M 2 "register_operand" "r"))
                    (match_operand:V256M 3 "register_operand" "0")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (plus:<CHUNK> (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 0)
                                    (subreg:<CHUNK> (match_dup 2) 0))
                      (subreg:<CHUNK> (match_dup 3) 0)))
   (set (subreg:<CHUNK> (match_dup 0) 8)
        (plus:<CHUNK> (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 8)
                                    (subreg:<CHUNK> (match_dup 2) 8))
                      (subreg:<CHUNK> (match_dup 3) 8)))
   (set (subreg:<CHUNK> (match_dup 0) 16)
        (plus:<CHUNK> (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 16)
                                    (subreg:<CHUNK> (match_dup 2) 16))
                      (subreg:<CHUNK> (match_dup 3) 16)))
   (set (subreg:<CHUNK> (match_dup 0) 24)
        (plus:<CHUNK> (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 24)
                                    (subreg:<CHUNK> (match_dup 2) 24))
                      (subreg:<CHUNK> (match_dup 3) 24)))]
  ""
  [(set_attr "type" "madd_int")]
)

(define_insn_and_split "msub<mode><mode>4"
  [(set (match_operand:V256M 0 "register_operand" "=r")
        (minus:V256M (match_operand:V256M 3 "register_operand" "0")
                     (mult:V256M (match_operand:V256M 1 "register_operand" "r")
                                 (match_operand:V256M 2 "register_operand" "r"))))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (minus:<CHUNK> (subreg:<CHUNK> (match_dup 3) 0)
                       (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 0)
                                     (subreg:<CHUNK> (match_dup 2) 0))))
   (set (subreg:<CHUNK> (match_dup 0) 8)
        (minus:<CHUNK> (subreg:<CHUNK> (match_dup 3) 8)
                       (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 8)
                                     (subreg:<CHUNK> (match_dup 2) 8))))
   (set (subreg:<CHUNK> (match_dup 0) 16)
        (minus:<CHUNK> (subreg:<CHUNK> (match_dup 3) 16)
                       (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 16)
                                     (subreg:<CHUNK> (match_dup 2) 16))))
   (set (subreg:<CHUNK> (match_dup 0) 24)
        (minus:<CHUNK> (subreg:<CHUNK> (match_dup 3) 24)
                       (mult:<CHUNK> (subreg:<CHUNK> (match_dup 1) 24)
                                     (subreg:<CHUNK> (match_dup 2) 24))))]
  ""
  [(set_attr "type" "madd_int")
   (set_attr "length"      "8")]
)


;; V256J (V16HI V8SI V4DI)

(define_insn_and_split "rotl<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (rotate:V256J (match_operand:V256J 1 "register_operand" "r")
                      (match_operand:SI 2 "register_operand" "r")))
   (clobber (match_scratch:SI 3 "=&r"))
   (clobber (match_scratch:V256J 4 "=&r"))
   (clobber (match_scratch:V256J 5 "=&r"))]
  ""
  "#"
  ""
  [(set (match_dup 3) (neg:SI (match_dup 2)))
   (set (match_dup 4) (ashift:V256J (match_dup 1) (match_dup 2)))
   (set (match_dup 5) (lshiftrt:V256J (match_dup 1) (match_dup 3)))
   (set (match_dup 0) (ior:V256J (match_dup 4) (match_dup 5)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (SImode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[5]) == SCRATCH)
      operands[5] = gen_reg_rtx (<MODE>mode);
  }
)

(define_insn_and_split "rotr<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (rotatert:V256J (match_operand:V256J 1 "register_operand" "r")
                        (match_operand:SI 2 "register_operand" "r")))
   (clobber (match_scratch:SI 3 "=&r"))
   (clobber (match_scratch:V256J 4 "=&r"))
   (clobber (match_scratch:V256J 5 "=&r"))]
  ""
  "#"
  ""
  [(set (match_dup 3) (neg:SI (match_dup 2)))
   (set (match_dup 4) (lshiftrt:V256J (match_dup 1) (match_dup 2)))
   (set (match_dup 5) (ashift:V256J (match_dup 1) (match_dup 3)))
   (set (match_dup 0) (ior:V256J (match_dup 4) (match_dup 5)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (SImode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[5]) == SCRATCH)
      operands[5] = gen_reg_rtx (<MODE>mode);
  }
)

(define_insn "neg<mode>2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (neg:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "LVX_2"
  {
    return "neg<hsuffix> %L0 = %L1\n\tneg<hsuffix> %M0 = %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"          "32")]
)

(define_expand "ssneg<mode>2"
  [(set (match_operand:V256J 0 "register_operand" "")
        (ss_neg:V256J (match_operand:V256J 1 "register_operand" "")))]
  ""
  ""
)

(define_insn_and_split "ssneg<mode>2_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_neg:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "!HAVE_LVX_SS_NEG_<MODE> && HAVE_LVX_SS_NEG_<HALF>"
  "#"
  "!HAVE_LVX_SS_NEG_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_neg:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_neg:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2_x")]
)

(define_insn "ssneg<mode>2_2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_neg:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_SS_NEG_<MODE>)"
  {
    return "negs<hsuffix> %L0 = %L1\n\tnegs<hsuffix> %M0 = %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"          "32")]
)

(define_expand "abs<mode>2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (abs:V256J (match_operand:V256J 1 "register_operand" "r")))]
  ""
  ""
)

(define_insn_and_split "abs<mode>2_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (abs:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "!HAVE_LVX_ABS_<MODE>"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (abs:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (abs:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2_x")]
)

(define_insn "abs<mode>2_2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (abs:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_ABS_<MODE>)"
  {
    return "abs<hsuffix> %L0 = %L1\n\tabs<hsuffix> %M0 = %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"          "32")]
)

(define_expand "ssabs<mode>2"
  [(set (match_operand:V256J 0 "register_operand" "")
        (ss_abs:V256J (match_operand:V256J 1 "register_operand" "")))]
  ""
  ""
)

(define_insn_and_split "ssabs<mode>2_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_abs:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "!HAVE_LVX_SS_ABS_<MODE>"
  "#"
  "!HAVE_LVX_SS_ABS_<MODE>"
  [(set (match_dup 0)
        (ss_neg:V256J (match_dup 1)))
   (set (match_dup 0)
        (abs:V256J (match_dup 0)))]
  ""
)

(define_insn "ssabs<mode>2_2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_abs:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_SS_ABS_<MODE>)"
  {
    return "abss<hsuffix> %L0 = %L1\n\tabss<hsuffix> %M0 = %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length"          "32")]
)

(define_insn_and_split "clrsb<mode>2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (clrsb:V256J (match_operand:V256J 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (clrsb:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (clrsb:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "clz<mode>2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (clz:V256J (match_operand:V256J 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (clz:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (clz:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "ctz<mode>2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ctz:V256J (match_operand:V256J 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ctz:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ctz:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "popcount<mode>2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (popcount:V256J (match_operand:V256J 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (popcount:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (popcount:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "one_cmpl<mode>2"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (not:V256L (match_operand:V256L 1 "register_operand" "r")))]
  "LVX_2"
  {
    return "notq %L0 = %L1\n\tnotq %M0 = %M1";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "abd<mode>3"
  [(match_operand:V256L 0 "register_operand" "")
   (match_operand:V256L 1 "register_operand" "")
   (match_operand:V256L 2 "register_operand" "")]
  ""
  {
    if (!HAVE_LVX_ABD_<MODE>)
      emit_insn (gen_abd<mode>3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_abd<mode>3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn_and_split "abd<mode>3_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (minus:V256J (smax:V256J (match_operand:V256J 1 "register_operand" "r")
                                 (match_operand:V256J 2 "register_operand" "r"))
                     (smin:V256J (match_dup 1) (match_dup 2))))]
  "!HAVE_LVX_ABD_<MODE> && HAVE_LVX_ABD_<HALF>"
  "#"
  "!HAVE_LVX_ABD_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (minus:<HALF> (smax:<HALF> (subreg:<HALF> (match_dup 1) 0)
                                   (subreg:<HALF> (match_dup 2) 0))
                      (smin:<HALF> (subreg:<HALF> (match_dup 1) 0)
                                   (subreg:<HALF> (match_dup 2) 0))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (minus:<HALF> (smax:<HALF> (subreg:<HALF> (match_dup 1) 16)
                                   (subreg:<HALF> (match_dup 2) 16))
                      (smin:<HALF> (subreg:<HALF> (match_dup 1) 16)
                                   (subreg:<HALF> (match_dup 2) 16))))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "abd<mode>3_2"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (minus:V256L (smax:V256L (match_operand:V256L 1 "register_operand" "r")
                                 (match_operand:V256L 2 "register_operand" "r"))
                     (smin:V256L (match_dup 1) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_ABD_<MODE>)"
  {
    return "abd<hsuffix> %L0 = %L1, %L2\n\tabd<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*abd<mode>_s1"
  [(set (match_operand:V256L 0 "register_operand" "=&r")
        (minus:V256L (smax:V256L (vec_duplicate:V256L (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                                 (match_operand:V256L 2 "register_operand" "r"))
                     (smin:V256L (vec_duplicate:V256L (match_dup 1)) (match_dup 2))))]
  "!HAVE_LVX_ABD_<MODE> && HAVE_LVX_ABD_<HALF>"
  "#"
  "!HAVE_LVX_ABD_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (minus:<HALF> (smax:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                                   (subreg:<HALF> (match_dup 2) 0))
                      (smin:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                                   (subreg:<HALF> (match_dup 2) 0))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (minus:<HALF> (smax:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                                   (subreg:<HALF> (match_dup 2) 16))
                      (smin:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                                   (subreg:<HALF> (match_dup 2) 16))))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*abd<mode>_s1"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (minus:V256L (smax:V256L (vec_duplicate:V256L (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                                 (match_operand:V256L 2 "register_operand" "r"))
                     (smin:V256L (vec_duplicate:V256L (match_dup 1)) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_ABD_<MODE>)"
  {
    return "abd<hsuffix> %L0 = %1, %L2\n\tabd<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*abd<mode>_s2"
  [(set (match_operand:V256L 0 "register_operand" "=&r")
        (minus:V256L (smax:V256L (match_operand:V256L 1 "register_operand" "r")
                                 (vec_duplicate:V256L (match_operand:<CHUNK> 2 "nonmemory_operand" "r")))
                     (smin:V256L (match_dup 1) (vec_duplicate:V256L (match_dup 2)))))]
  "!HAVE_LVX_ABD_<MODE> && HAVE_LVX_ABD_<HALF>"
  "#"
  "!HAVE_LVX_ABD_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (minus:<HALF> (smax:<HALF> (subreg:<HALF> (match_dup 1) 0)
                                   (vec_duplicate:<HALF> (match_dup 2)))
                      (smin:<HALF> (subreg:<HALF> (match_dup 1) 0)
                                   (vec_duplicate:<HALF> (match_dup 2)))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (minus:<HALF> (smax:<HALF> (subreg:<HALF> (match_dup 1) 16)
                                   (vec_duplicate:<HALF> (match_dup 2)))
                      (smin:<HALF> (subreg:<HALF> (match_dup 1) 16)
                                   (vec_duplicate:<HALF> (match_dup 2)))))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn "*abd<mode>_s2"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (minus:V256L (smax:V256L (match_operand:V256L 1 "register_operand" "r")
                                 (vec_duplicate:V256L (match_operand:<CHUNK> 2 "nonmemory_operand" "r")))
                     (smin:V256L (match_dup 1) (vec_duplicate:V256L (match_dup 2)))))]
  "LVX_2 && (HAVE_LVX_ABD_<MODE>)"
  {
    return "abd<hsuffix> %L0 = %L1, %2\n\tabd<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "abds<mode>3"
  [(match_operand:V256L 0 "register_operand" "")
   (match_operand:V256L 1 "register_operand" "")
   (match_operand:V256L 2 "register_operand" "")]
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
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_minus:V256J (smax:V256J (match_operand:V256J 1 "register_operand" "r")
                                    (match_operand:V256J 2 "register_operand" "r"))
                        (smin:V256J (match_dup 1) (match_dup 2))))
   (clobber (match_scratch:V256J 3 "=&r"))
   (clobber (match_scratch:V256J 4 "=&r"))]
  "!HAVE_LVX_SS_ABD_<MODE>"
  "#"
  "!HAVE_LVX_SS_ABD_<MODE>"
  [(set (match_dup 3)
        (smax:V256J (match_dup 1) (match_dup 2)))
   (set (match_dup 4)
        (smin:V256J (match_dup 1) (match_dup 2)))
   (set (match_dup 0)
        (ss_minus:V256J (match_dup 3) (match_dup 4)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
  }
)

(define_insn "abds<mode>3_2"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (ss_minus:V256L (smax:V256L (match_operand:V256L 1 "register_operand" "r")
                                    (match_operand:V256L 2 "register_operand" "r"))
                        (smin:V256L (match_dup 1) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_SS_ABD_<MODE>)"
  {
    return "abds<hsuffix> %L0 = %L1, %L2\n\tabds<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*abds<mode>_s1"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (ss_minus:V256L (smax:V256L (vec_duplicate:V256L (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                                    (match_operand:V256L 2 "register_operand" "r"))
                        (smin:V256L (vec_duplicate:V256L (match_dup 1)) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_SS_ABD_<MODE>)"
  {
    return "abds<hsuffix> %L0 = %1, %L2\n\tabds<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*abds<mode>_s2"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (ss_minus:V256L (smax:V256L (match_operand:V256L 1 "register_operand" "r")
                                    (vec_duplicate:V256L (match_operand:<CHUNK> 2 "nonmemory_operand" "r")))
                        (smin:V256L (match_dup 1) (vec_duplicate:V256L (match_dup 2)))))]
  "LVX_2 && (HAVE_LVX_SS_ABD_<MODE>)"
  {
    return "abds<hsuffix> %L0 = %L1, %2\n\tabds<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_expand "abdu<mode>3"
  [(match_operand:V256L 0 "register_operand" "")
   (match_operand:V256L 1 "register_operand" "")
   (match_operand:V256L 2 "register_operand" "")]
  ""
  {
    if (!HAVE_LVX_US_ABD_<MODE>)
      emit_insn (gen_abdu<mode>3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_abdu<mode>3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn_and_split "abdu<mode>3_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (minus:V256J (umax:V256J (match_operand:V256J 1 "register_operand" "r")
                                 (match_operand:V256J 2 "register_operand" "r"))
                     (umin:V256J (match_dup 1) (match_dup 2))))
   (clobber (match_scratch:V256J 3 "=&r"))
   (clobber (match_scratch:V256J 4 "=&r"))]
  "!HAVE_LVX_US_ABD_<MODE>"
  "#"
  "!HAVE_LVX_US_ABD_<MODE>"
  [(set (match_dup 3)
        (umax:V256J (match_dup 1) (match_dup 2)))
   (set (match_dup 4)
        (umin:V256J (match_dup 1) (match_dup 2)))
   (set (match_dup 0)
        (minus:V256J (match_dup 3) (match_dup 4)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (<MODE>mode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (<MODE>mode);
  }
)

(define_insn "abdu<mode>3_2"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (minus:V256L (umax:V256L (match_operand:V256L 1 "register_operand" "r")
                                 (match_operand:V256L 2 "register_operand" "r"))
                     (umin:V256L (match_dup 1) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_US_ABD_<MODE>)"
  {
    return "abdu<hsuffix> %L0 = %L1, %L2\n\tabdu<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*abdu<mode>_s1"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (minus:V256L (umax:V256L (vec_duplicate:V256L (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                                 (match_operand:V256L 2 "register_operand" "r"))
                     (umin:V256L (vec_duplicate:V256L (match_dup 1)) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_US_ABD_<MODE>)"
  {
    return "abdu<hsuffix> %L0 = %1, %L2\n\tabdu<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "*abdu<mode>_s2"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (minus:V256L (umax:V256L (match_operand:V256L 1 "register_operand" "r")
                                 (vec_duplicate:V256L (match_operand:<CHUNK> 2 "nonmemory_operand" "r")))
                     (umin:V256L (match_dup 1) (vec_duplicate:V256L (match_dup 2)))))]
  "LVX_2 && (HAVE_LVX_US_ABD_<MODE>)"
  {
    return "abdu<hsuffix> %L0 = %L1, %2\n\tabdu<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)


;; V8SI

(define_insn_and_split "mulv8si3"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (mult:V8SI (match_operand:V8SI 1 "register_operand" "r")
                    (match_operand:V8SI 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V4SI (match_dup 0) 0)
        (mult:V4SI (subreg:V4SI (match_dup 1) 0)
                   (subreg:V4SI (match_dup 2) 0)))
   (set (subreg:V4SI (match_dup 0) 16)
        (mult:V4SI (subreg:V4SI (match_dup 1) 16)
                   (subreg:V4SI (match_dup 2) 16)))]
  ""
  [(set_attr "type" "madd_int")]
)

(define_insn "maddv8siv8si4_2"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (plus:V8SI (mult:V8SI (match_operand:V8SI 1 "register_operand" "r")
                              (match_operand:V8SI 2 "register_operand" "r"))
                   (match_operand:V8SI 3 "register_operand" "0")))]
  "HAVE_LVX_FMA_V4SI || HAVE_LVX_FMA_V2SI"
  "#"
  [(set_attr "type" "madd_int")]
)

(define_split
  [(set (match_operand:V8SI 0 "register_operand" "")
        (plus:V8SI (mult:V8SI (match_operand:V8SI 1 "register_operand" "")
                              (match_operand:V8SI 2 "register_operand" ""))
                   (match_operand:V8SI 3 "register_operand" "")))]
  "HAVE_LVX_FMA_V4SI && reload_completed"
  [(set (subreg:V4SI (match_dup 0) 0)
        (plus:V4SI (mult:V4SI (subreg:V4SI (match_dup 1) 0)
                              (subreg:V4SI (match_dup 2) 0))
                   (subreg:V4SI (match_dup 3) 0)))
   (set (subreg:V4SI (match_dup 0) 16)
        (plus:V4SI (mult:V4SI (subreg:V4SI (match_dup 1) 16)
                              (subreg:V4SI (match_dup 2) 16))
                   (subreg:V4SI (match_dup 3) 16)))]
  ""
)

(define_insn "msubv8siv8si4_2"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (minus:V8SI (match_operand:V8SI 3 "register_operand" "0")
                    (mult:V8SI (match_operand:V8SI 1 "register_operand" "r")
                               (match_operand:V8SI 2 "register_operand" "r"))))]
  "HAVE_LVX_FMS_V4SI || HAVE_LVX_FMS_V2SI"
  "#"
  [(set_attr "type" "madd_int")]
)

(define_split
  [(set (match_operand:V8SI 0 "register_operand" "")
        (minus:V8SI (match_operand:V8SI 3 "register_operand" "")
                    (mult:V8SI (match_operand:V8SI 1 "register_operand" "")
                               (match_operand:V8SI 2 "register_operand" ""))))]
  "HAVE_LVX_FMS_V4SI && reload_completed"
  [(set (subreg:V4SI (match_dup 0) 0)
        (minus:V4SI (subreg:V4SI (match_dup 3) 0)
                    (mult:V4SI (subreg:V4SI (match_dup 1) 0)
                               (subreg:V4SI (match_dup 2) 0))))
   (set (subreg:V4SI (match_dup 0) 16)
        (minus:V4SI (subreg:V4SI (match_dup 3) 16)
                    (mult:V4SI (subreg:V4SI (match_dup 1) 16)
                               (subreg:V4SI (match_dup 2) 16))))]
  ""
)


;; V4DI

(define_insn "ashlv4di3"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (ashift:V4DI (match_operand:V4DI 1 "register_operand" "r")
                     (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  {
    return "slldp %L0 = %L1, %2\n\tslldp %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "ssashlv4di3"
  [(set (match_operand:V4DI 0 "register_operand" "=&r,r")
        (ss_ashift:V4DI (match_operand:V4DI 1 "register_operand" "r,r")
                        (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V2DI (match_dup 0) 0)
        (ss_ashift:V2DI (subreg:V2DI (match_dup 1) 0)
                        (match_dup 2)))
   (set (subreg:V2DI (match_dup 0) 16)
        (ss_ashift:V2DI (subreg:V2DI (match_dup 1) 16)
                        (match_dup 2)))]
  ""
  [(set_attr "type" "alu_lite_x2,alu_lite_x2")]
)

(define_expand "usashlv4di3"
  [(match_operand:V4DI 0 "register_operand" "")
   (match_operand:V4DI 1 "register_operand" "")
   (match_operand:SI 2 "reg_shift_operand" "")]
  ""
  {
    if (!HAVE_LVX_US_ASHIFT_V4DI)
      emit_insn (gen_usashlv4di3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_usashlv4di3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn_and_split "usashlv4di3_1"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (us_ashift:V4DI (match_operand:V4DI 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))
   (clobber (match_scratch:V4DI 3 "=&r"))
   (clobber (match_scratch:V4DI 4 "=&r"))
   (clobber (match_scratch:V4DI 5 "=&r"))]
  "!HAVE_LVX_US_ASHIFT_V4DI"
  "#"
  "!HAVE_LVX_US_ASHIFT_V4DI"
  [(set (match_dup 3)
        (ashift:V4DI (match_dup 1) (match_dup 2)))
   (set (match_dup 4)
        (lshiftrt:V4DI (match_dup 3) (match_dup 2)))
   (set (match_dup 5)
        (ne:V4DI (match_dup 4) (match_dup 1)))
   (set (match_dup 0)
        (ior:V4DI (match_dup 3) (match_dup 5)))]
  {
    if (GET_CODE (operands[3]) == SCRATCH)
      operands[3] = gen_reg_rtx (V4DImode);
    if (GET_CODE (operands[4]) == SCRATCH)
      operands[4] = gen_reg_rtx (V4DImode);
    if (GET_CODE (operands[5]) == SCRATCH)
      operands[5] = gen_reg_rtx (V4DImode);
  }
)

(define_insn_and_split "usashlv4di3_2"
  [(set (match_operand:V4DI 0 "register_operand" "=&r,r")
        (us_ashift:V4DI (match_operand:V4DI 1 "register_operand" "r,r")
                        (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  "HAVE_LVX_US_ASHIFT_V4DI"
  "#"
  "HAVE_LVX_US_ASHIFT_V4DI && reload_completed"
  [(set (subreg:V2DI (match_dup 0) 0)
        (us_ashift:V2DI (subreg:V2DI (match_dup 1) 0)
                        (match_dup 2)))
   (set (subreg:V2DI (match_dup 0) 16)
        (us_ashift:V2DI (subreg:V2DI (match_dup 1) 16)
                        (match_dup 2)))]
  ""
  [(set_attr "type" "alu_lite_x2,alu_lite_x2")]
)

(define_insn "ashrv4di3"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (ashiftrt:V4DI (match_operand:V4DI 1 "register_operand" "r")
                       (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  {
    return "sradp %L0 = %L1, %2\n\tsradp %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn "lshrv4di3"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (lshiftrt:V4DI (match_operand:V4DI 1 "register_operand" "r")
                       (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  {
    return "srldp %L0 = %L1, %2\n\tsrldp %M0 = %M1, %2";
  }
  [(set_attr "type" "alu_tiny_x2")
   (set_attr "length" "8")]
)

(define_insn_and_split "sshrv4di3"
  [(set (match_operand:V4DI 0 "register_operand" "=&r,r")
        (unspec:V4DI [(match_operand:V4DI 1 "register_operand" "r,r")
                      (match_operand:SI 2 "reg_shift_operand" "r,U06")] UNSPEC_SRS))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:V2DI (match_dup 0) 0)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 0)
                      (match_dup 2)] UNSPEC_SRS))
   (set (subreg:V2DI (match_dup 0) 16)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 16)
                      (match_dup 2)] UNSPEC_SRS))]
  ""
  [(set_attr "type" "alu_lite_x2,alu_lite_x2")]
)


;; VXHF

(define_expand "div<mode>3"
  [(set (match_operand:VXHF 0 "register_operand" "")
        (div:VXHF (match_operand:VXHF 1 "register_float1_operand" "")
                  (match_operand:VXHF 2 "register_operand" "")))]
  ""
  {
    /* 128-bit chunks, not 64-bit: the V4HF/V4HI half-register views this
       used to step through are gone with 64-bit SIMD, so each iteration now
       handles a V8HF chunk promoted to V8SF -- half as many iterations.  */
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += 16)
      {
        rtx temp0 = gen_reg_rtx (V8SFmode);
        rtx temp1 = gen_reg_rtx (V8SFmode);
        rtx temp2 = gen_reg_rtx (V8SFmode);
        rtx op0 = simplify_gen_subreg (V8HFmode, operands[0], <MODE>mode, offset);
        rtx op1 = simplify_gen_subreg (V8HFmode, operands[1], <MODE>mode, offset);
        rtx op2 = simplify_gen_subreg (V8HFmode, operands[2], <MODE>mode, offset);
        if (op1 == CONST1_RTX(V8HFmode))
          {
            emit_insn (gen_extendv8hfv8sf2 (temp2, op2));
            emit_insn (gen_divv8sf3 (temp0, CONST1_RTX (V8SFmode), temp2));
          }
        else
          {
            emit_insn (gen_extendv8hfv8sf2 (temp1, op1));
            emit_insn (gen_extendv8hfv8sf2 (temp2, op2));
            emit_insn (gen_divv8sf3 (temp0, temp1, temp2));
          }
        emit_insn (gen_truncv8sfv8hf2 (op0, temp0));
      }
    DONE;
  }
)

(define_expand "float<mask><mode>2"
  [(set (match_operand:VXHF 0 "register_operand" "")
        (float:VXHF (match_operand:<MASK> 1 "register_operand" "")))]
  ""
  {
    /* 128-bit chunks, not 64-bit: the V4HF/V4HI half-register views this
       used to step through are gone with 64-bit SIMD, so each iteration now
       handles a V8HF chunk promoted to V8SF -- half as many iterations.  */
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += 16)
      {
        rtx temp0 = gen_reg_rtx (V8SFmode);
        rtx temp1 = gen_reg_rtx (V8SImode);
        rtx op0 = simplify_gen_subreg (V8HFmode, operands[0], <MODE>mode, offset);
        rtx op1 = simplify_gen_subreg (V8HImode, operands[1], <MASK>mode, offset);
        emit_insn (gen_extendv8hiv8si2 (temp1, op1));
        emit_insn (gen_floatv8siv8sf2 (temp0, temp1));
        emit_insn (gen_truncv8sfv8hf2 (op0, temp0));
      }
    DONE;
  }
)

(define_expand "floatuns<mask><mode>2"
  [(set (match_operand:VXHF 0 "register_operand" "")
        (unsigned_float:VXHF (match_operand:<MASK> 1 "register_operand" "")))]
  ""
  {
    /* 128-bit chunks, not 64-bit: the V4HF/V4HI half-register views this
       used to step through are gone with 64-bit SIMD, so each iteration now
       handles a V8HF chunk promoted to V8SF -- half as many iterations.  */
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += 16)
      {
        rtx temp0 = gen_reg_rtx (V8SFmode);
        rtx temp1 = gen_reg_rtx (V8SImode);
        rtx op0 = simplify_gen_subreg (V8HFmode, operands[0], <MODE>mode, offset);
        rtx op1 = simplify_gen_subreg (V8HImode, operands[1], <MASK>mode, offset);
        emit_insn (gen_zero_extendv8hiv8si2 (temp1, op1));
        emit_insn (gen_floatunsv8siv8sf2 (temp0, temp1));
        emit_insn (gen_truncv8sfv8hf2 (op0, temp0));
      }
    DONE;
  }
)

(define_expand "fix_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "")
        (fix:<MASK> (match_operand:VXHF 1 "register_operand" "")))]
  ""
  {
    /* 128-bit chunks, not 64-bit: the V4HF/V4HI half-register views this
       used to step through are gone with 64-bit SIMD, so each iteration now
       handles a V8HF chunk promoted to V8SF -- half as many iterations.  */
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += 16)
      {
        rtx temp0 = gen_reg_rtx (V8SImode);
        rtx temp1 = gen_reg_rtx (V8SFmode);
        rtx op0 = simplify_gen_subreg (V8HImode, operands[0], <MASK>mode, offset);
        rtx op1 = simplify_gen_subreg (V8HFmode, operands[1], <MODE>mode, offset);
        emit_insn (gen_extendv8hfv8sf2 (temp1, op1));
        emit_insn (gen_fix_truncv8sfv8si2 (temp0, temp1));
        emit_insn (gen_truncv8siv8hi2 (op0, temp0));
      }
    DONE;
  }
)

(define_expand "fixuns_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "")
        (unsigned_fix:<MASK> (match_operand:VXHF 1 "register_operand" "")))]
  ""
  {
    /* 128-bit chunks, not 64-bit: the V4HF/V4HI half-register views this
       used to step through are gone with 64-bit SIMD, so each iteration now
       handles a V8HF chunk promoted to V8SF -- half as many iterations.  */
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += 16)
      {
        rtx temp0 = gen_reg_rtx (V8SImode);
        rtx temp1 = gen_reg_rtx (V8SFmode);
        rtx op0 = simplify_gen_subreg (V8HImode, operands[0], <MASK>mode, offset);
        rtx op1 = simplify_gen_subreg (V8HFmode, operands[1], <MODE>mode, offset);
        emit_insn (gen_extendv8hfv8sf2 (temp1, op1));
        emit_insn (gen_fixuns_truncv8sfv8si2 (temp0, temp1));
        emit_insn (gen_truncv8siv8hi2 (op0, temp0));
      }
    DONE;
  }
)


;; VXSF

(define_expand "div<mode>3"
  [(set (match_operand:VXSF 0 "register_operand" "")
        (div:VXSF (match_operand:VXSF 1 "register_float1_operand" "")
                  (match_operand:VXSF 2 "register_operand" "")))]
  ""
  {
    rtx rm = gen_rtx_CONST_STRING (VOIDmode, "");
    rtx rn = gen_rtx_CONST_STRING (VOIDmode, ".rn");
    rtx a = operands[1], b = operands[2];
    /* No unguarded 1.0/b shortcut.  KVX had frec*, an exact reciprocal, and
       LVX generalises that to the fdiv* instructions -- but only at scalar
       width, so there is nothing exact to use here.  fsrec* is a *seed*, so
       using it unconditionally would silently approximate a division the
       user did not ask to have approximated.  The flag-guarded paths below
       are where an approximation is legitimate.  */
    if (flag_reciprocal_math)
      {
        rtx t = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_lvx_fsrec<suffix> (t, b, rm));
        emit_insn (gen_lvx_fmul<suffix> (operands[0], a, t, rm));
      }
    else if (flag_unsafe_math_optimizations)
      {
        rtx re = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_lvx_fsrec<suffix> (re, b, rn));
        rtx y0 = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_lvx_fmul<suffix> (y0, a, re, rn));
        rtx e0 = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_lvx_ffms<suffix> (e0, b, y0, a, rn));
        rtx y1 = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_lvx_ffma<suffix> (y1, e0, re, y0, rn));
        rtx e1 = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_lvx_ffms<suffix> (e1, b, y1, a, rn));
        rtx y2 = operands[0];
        emit_insn (gen_lvx_ffma<suffix> (y2, e1, re, y1, rm));
      }
     else
       {
         emit_library_call_value
           (gen_rtx_SYMBOL_REF (Pmode, "__div<mode>3"),
            operands[0], LCT_CONST, <MODE>mode,
            operands[1], <MODE>mode, operands[2], <MODE>mode);
       }
    DONE;
  }
)

(define_expand "sqrt<mode>2"
  [(match_operand:VXSF 0 "register_operand" "")
   (match_operand:VXSF 1 "register_operand" "")]
  "flag_reciprocal_math"
  {
    rtx temp = gen_reg_rtx (<MODE>mode);
    rtx rm = gen_rtx_CONST_STRING (VOIDmode, "");
    emit_insn (gen_lvx_fsrsr<suffix> (temp, operands[1]));
    emit_insn (gen_mul<mode>3 (operands[0], operands[1], temp));
    DONE;
  }
)

(define_expand "rsqrt<mode>2"
  [(match_operand:VXSF 0 "register_operand" "")
   (match_operand:VXSF 1 "register_operand" "")]
  ""
  {
    rtx rm = gen_rtx_CONST_STRING (VOIDmode, "");
    emit_insn (gen_lvx_fsrsr<suffix> (operands[0], operands[1]));
    DONE;
  }
)


;; VXDF

(define_expand "div<mode>3"
  [(set (match_operand:VXDF 0 "register_operand" "")
        (div:VXDF (match_operand:VXDF 1 "register_operand" "")
                  (match_operand:VXDF 2 "register_operand" "")))]
  ""
  {
    emit_library_call_value
      (gen_rtx_SYMBOL_REF (Pmode, "__div<mode>3"),
       operands[0], LCT_CONST, <MODE>mode,
       operands[1], <MODE>mode, operands[2], <MODE>mode);
    DONE;
  }
)


;; S128F (V8HF V4SF)

(define_insn_and_split "fma<mode>4"
  [(set (match_operand:S128F 0 "register_operand" "=r")
        (fma:S128F (match_operand:S128F 1 "register_operand" "r")
                   (match_operand:S128F 2 "register_operand" "r")
                   (match_operand:S128F 3 "register_operand" "0")))]
  ""
  "ffma<suffix> %0 = %1, %2"
  "!HAVE_LVX_FMA_<MODE>_<MODE>_<MODE> && reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (fma:<CHUNK> (subreg:<CHUNK> (match_dup 1) 0)
                     (subreg:<CHUNK> (match_dup 2) 0)
                     (subreg:<CHUNK> (match_dup 3) 0)))
   (set (subreg:<CHUNK> (match_dup 0) 8)
        (fma:<CHUNK> (subreg:<CHUNK> (match_dup 1) 8)
                     (subreg:<CHUNK> (match_dup 2) 8)
                     (subreg:<CHUNK> (match_dup 3) 8)))]
  ""
  [(set (attr "type")
     (if_then_else (match_operand 1 "float16_inner_mode") (const_string "madd_fp3") (const_string "madd_fp4")))]
)

(define_insn_and_split "fnma<mode>4"
  [(set (match_operand:S128F 0 "register_operand" "=r")
        (fma:S128F (neg:S128F (match_operand:S128F 1 "register_operand" "r"))
                   (match_operand:S128F 2 "register_operand" "r")
                   (match_operand:S128F 3 "register_operand" "0")))]
  ""
  "ffms<suffix> %0 = %1, %2"
  "!HAVE_LVX_FMS_<MODE>_<MODE>_<MODE> && reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (fma:<CHUNK> (neg:<CHUNK> (subreg:<CHUNK> (match_dup 1) 0))
                     (subreg:<CHUNK> (match_dup 2) 0)
                     (subreg:<CHUNK> (match_dup 3) 0)))
   (set (subreg:<CHUNK> (match_dup 0) 8)
        (fma:<CHUNK> (neg:<CHUNK> (subreg:<CHUNK> (match_dup 1) 8))
                     (subreg:<CHUNK> (match_dup 2) 8)
                     (subreg:<CHUNK> (match_dup 3) 8)))]
  ""
  [(set (attr "type")
     (if_then_else (match_operand 1 "float16_inner_mode") (const_string "madd_fp3") (const_string "madd_fp4")))]
)

(define_insn_and_split "trunc<wide><mode>2"
  [(set (match_operand:S128F 0 "register_operand" "=&r")
        (float_truncate:S128F (match_operand:<WIDE> 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (float_truncate:<HALF> (subreg:<HWIDE> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 8)
        (float_truncate:<HALF> (subreg:<HWIDE> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu_full")]
)

(define_expand "extend<mode><wide>2"
  [(set (match_operand:<WIDE> 0 "register_operand" "")
        (float_extend:<WIDE> (match_operand:S128F 1 "register_operand" "")))]
  "LVX_2"
  {
    /* One FWIDEN per 128-bit half of the result: the mostsig modifier selects
       the least or most significant lanes of the 128-bit source.  This used to
       split into float_extends of 64-bit <HALF>/<QUART> chunks, whose
       sub-patterns went with 64-bit SIMD.  */
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
        emit_insn (gen_lvx_fwiden<wchunkx> (d0, src, lo));
        emit_insn (gen_lvx_fwiden<wchunkx> (d1, src, hi));
      }
    DONE;
  }
)


;; V128F (V8HF V4SF V2DF)

(define_insn "fmin<mode>3"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (smin:V128F (match_operand:V128F 1 "register_operand" "r")
                    (match_operand:V128F 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MIN_<MODE> && !(HAVE_LVX_BUG_FMIN && flag_signaling_nans))"
  "fmin<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*fmin<mode>3_s1"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (smin:V128F (vec_duplicate:V128F (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128F 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MIN_<MODE> && !(HAVE_LVX_BUG_FMIN && flag_signaling_nans))"
  "fmin<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*fmin<mode>3_s2"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (smin:V128F (match_operand:V128F 1 "register_operand" "r")
                    (vec_duplicate:V128F (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_MIN_<MODE> && !(HAVE_LVX_BUG_FMIN && flag_signaling_nans))"
  "fmin<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "fmax<mode>3"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (smax:V128F (match_operand:V128F 1 "register_operand" "r")
                    (match_operand:V128F 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MAX_<MODE> && !(HAVE_LVX_BUG_FMAX && flag_signaling_nans))"
  "fmax<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*fmax<mode>3_s1"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (smax:V128F (vec_duplicate:V128F (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128F 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MAX_<MODE> && !(HAVE_LVX_BUG_FMAX && flag_signaling_nans))"
  "fmax<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "*fmax<mode>3_s2"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (smax:V128F (match_operand:V128F 1 "register_operand" "r")
                    (vec_duplicate:V128F (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_MAX_<MODE> && !(HAVE_LVX_BUG_FMAX && flag_signaling_nans))"
  "fmax<suffix> %0 = %1, %2"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "neg<mode>2"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (neg:V128F (match_operand:V128F 1 "register_operand" "r")))]
  "LVX_2"
  "fneg<suffix> %0 = %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_insn "abs<mode>2"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (abs:V128F (match_operand:V128F 1 "register_operand" "r")))]
  "LVX_2"
  "fabs<suffix> %0 = %1"
  [(set_attr "type" "alu_lite")
   (set_attr "length" "4")]
)

(define_expand "copysign<mode>3"
  [(match_operand:V128F 0 "register_operand")
   (match_operand:V128F 1 "register_operand")
   (match_operand:V128F 2 "register_operand")]
  ""
  {
    rtx fabs1 = gen_reg_rtx (<MODE>mode);
    emit_insn (gen_abs<mode>2 (fabs1, operands[1]));
    rtx fneg1 = gen_reg_rtx (<MODE>mode);
    emit_insn (gen_neg<mode>2 (fneg1, fabs1));
    rtx sign2 = gen_reg_rtx (<MASK>mode);
    convert_move (sign2, operands[2], 0);
    scalar_mode inner_mode = GET_MODE_INNER (<MODE>mode);
    rtx ltz = GET_MODE_SIZE (inner_mode) == UNITS_PER_WORD
            ? gen_rtx_CONST_STRING (VOIDmode, ".dltz")
            : gen_rtx_CONST_STRING (VOIDmode, ".ltz");
    emit_insn (gen_lvx_selectf<suffix> (operands[0], fneg1, fabs1, sign2, ltz));
    DONE;
  }
)

(define_expand "xorsign<mode>3"
  [(match_operand:V128F 0 "register_operand")
   (match_operand:V128F 1 "register_operand")
   (match_operand:V128F 2 "register_operand")]
  ""
  {
    rtx maskv8hf __attribute__((unused)) = GEN_INT (0x8000800080008000);
    rtx maskv4sf __attribute__((unused)) = GEN_INT (0x8000000080000000);
    rtx maskv2df __attribute__((unused)) = GEN_INT (0x8000000000000000);
    rtx mask = gen_reg_rtx (DImode);
    emit_move_insn (mask, mask<mode>);
    rtx sign2 = gen_reg_rtx (<MODE>mode);
    emit_insn (gen_rtx_SET (sign2, gen_rtx_UNSPEC (<MODE>mode, gen_rtvec (2, operands[2], mask), UNSPEC_ANDD)));
    emit_insn (gen_rtx_SET (operands[0], gen_rtx_UNSPEC (<MODE>mode, gen_rtvec (2, operands[1], sign2), UNSPEC_XORD)));
    DONE;
  }
)


;; V128G (V4SF V2DF)

(define_insn_and_split "float<mask><mode>2"
  [(set (match_operand:V128G 0 "register_operand" "=r")
        (float:V128G (match_operand:<MASK> 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (float:<HALF> (subreg:<HMASK> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 8)
        (float:<HALF> (subreg:<HMASK> (match_dup 1) 8)))]
  ""
)

(define_insn_and_split "floatuns<mask><mode>2"
  [(set (match_operand:V128G 0 "register_operand" "=r")
        (unsigned_float:V128G (match_operand:<MASK> 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unsigned_float:<HALF> (subreg:<HMASK> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 8)
        (unsigned_float:<HALF> (subreg:<HMASK> (match_dup 1) 8)))]
  ""
)

(define_insn_and_split "fix_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "=r")
        (fix:<MASK> (match_operand:V128G 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HMASK> (match_dup 0) 0)
        (fix:<HMASK> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HMASK> (match_dup 0) 8)
        (fix:<HMASK> (subreg:<HALF> (match_dup 1) 8)))]
  ""
)

(define_insn_and_split "fixuns_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "=r")
        (unsigned_fix:<MASK> (match_operand:V128G 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HMASK> (match_dup 0) 0)
        (unsigned_fix:<HMASK> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HMASK> (match_dup 0) 8)
        (unsigned_fix:<HMASK> (subreg:<HALF> (match_dup 1) 8)))]
  ""
)

;;(define_insn "truncv8sfv8hf2"
;;  [(set (match_operand:V8HF 0 "register_operand" "=r")
;;        (float_truncate:V8HF (match_operand:V8SF 1 "register_operand" "r")))]
;;  ""
;;  "fnarrowwhq %x0 = %x1\n\tfnarrowwhq %y0 = %y1"
;;  [(set_attr "type" "alu_lite_x2")
;;   (set_attr "length"         "8")]
;;)

;;(define_insn_and_split "extendv8hfv8sf2"
;;  [(set (match_operand:V8SF 0 "register_operand" "=r")
;;        (float_extend:V8SF (match_operand:V8HF 1 "register_operand" "r")))]
;;  ""
;;  "#"
;;  "reload_completed"
;;  [(set (subreg:V4SF (match_dup 0) 0)
;;        (float_extend:V4SF (subreg:V4HF (match_dup 1) 0)))
;;   (set (subreg:V4SF (match_dup 0) 16)
;;        (float_extend:V4SF (subreg:V4HF (match_dup 1) 8)))]
;;  ""
;;)

(define_insn_and_split "floatv8siv8hf2"
  [(set (match_operand:V8HF 0 "register_operand" "=r")
        (float:V8HF (match_operand:V8SI 1 "register_operand" "r")))
   (clobber (match_scratch:V8SF 2 "=&r"))]
  ""
  "#"
  ""
  [(set (match_dup 2)
        (float:V8SF (match_dup 1)))
   (set (match_dup 0)
        (float_truncate:V8HF (match_dup 2)))]
  {
    if (GET_CODE (operands[2]) == SCRATCH)
      operands[2] = gen_reg_rtx (V8SFmode);
  }
)

(define_insn_and_split "floatunsv8siv8hf2"
  [(set (match_operand:V8HF 0 "register_operand" "=r")
        (unsigned_float:V8HF (match_operand:V8SI 1 "register_operand" "r")))
   (clobber (match_scratch:V8SF 2 "=&r"))]
  ""
  "#"
  ""
  [(set (match_dup 2)
        (unsigned_float:V8SF (match_dup 1)))
   (set (match_dup 0)
        (float_truncate:V8HF (match_dup 2)))]
  {
    if (GET_CODE (operands[2]) == SCRATCH)
      operands[2] = gen_reg_rtx (V8SFmode);
  }
)

(define_insn_and_split "fix_truncv8hfv8si2"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (fix:V8SI (match_operand:V8HF 1 "register_operand" "r")))
   (clobber (match_scratch:V8SF 2 "=&r"))]
  ""
  "#"
  ""
  [(set (match_dup 2)
        (float_extend:V8SF (match_dup 1)))
   (set (match_dup 0)
        (fix:V8SI (match_dup 2)))]
  {
    if (GET_CODE (operands[2]) == SCRATCH)
      operands[2] = gen_reg_rtx (V8SFmode);
  }
)

(define_insn_and_split "fixuns_truncv8hfv8si2"
  [(set (match_operand:V8SI 0 "register_operand" "=r")
        (unsigned_fix:V8SI (match_operand:V8HF 1 "register_operand" "r")))
   (clobber (match_scratch:V8SF 2 "=&r"))]
  ""
  "#"
  ""
  [(set (match_dup 2)
        (float_extend:V8SF (match_dup 1)))
   (set (match_dup 0)
        (unsigned_fix:V8SI (match_dup 2)))]
  {
    if (GET_CODE (operands[2]) == SCRATCH)
      operands[2] = gen_reg_rtx (V8SFmode);
  }
)


;; V4SF

(define_expand  "lvx_fmt22w"
  [(match_operand:V4SF 0 "register_operand" "")
   (match_operand:V4SF 1 "register_operand" "")]
  ""
  {
    int table[4] = {0,2,1,3};
    rtvec values = rtvec_alloc (4);
    for (int i = 0; i < 4; i++)
      RTVEC_ELT (values, i) = GEN_INT (table[i]);
    rtx selector = gen_rtx_CONST_VECTOR (V4SImode, values);
    lvx_expand_vec_perm_const (operands[0], operands[1], operands[1], selector);
    DONE;
  }
)


;; V2DF

(define_insn_and_split "addv2df3"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (plus:V2DF (match_operand:V2DF 1 "register_operand" "r")
                   (match_operand:V2DF 2 "register_operand" "r")))]
  "LVX_2"
  "fadddp %0 = %1, %2"
  "!HAVE_LVX_PLUS_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (plus:DF (subreg:DF (match_dup 1) 0)
                 (subreg:DF (match_dup 2) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (plus:DF (subreg:DF (match_dup 1) 8)
                 (subreg:DF (match_dup 2) 8)))]
  ""
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn_and_split "subv2df3"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (minus:V2DF (match_operand:V2DF 1 "register_operand" "r")
                    (match_operand:V2DF 2 "register_operand" "r")))]
  "LVX_2"
  "fsbfdp %0 = %2, %1"
  "!HAVE_LVX_MINUS_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (minus:DF (subreg:DF (match_dup 1) 0)
                  (subreg:DF (match_dup 2) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (minus:DF (subreg:DF (match_dup 1) 8)
                  (subreg:DF (match_dup 2) 8)))]
  ""
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn_and_split "mulv2df3"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (mult:V2DF (match_operand:V2DF 1 "register_operand" "r")
                   (match_operand:V2DF 2 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (mult:DF (subreg:DF (match_dup 1) 0)
                 (subreg:DF (match_dup 2) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (mult:DF (subreg:DF (match_dup 1) 8)
                 (subreg:DF (match_dup 2) 8)))]
  ""
  [(set_attr "type" "madd_fp4")]
)

(define_insn_and_split "fmav2df4"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (fma:V2DF (match_operand:V2DF 1 "register_operand" "r")
                  (match_operand:V2DF 2 "register_operand" "r")
                  (match_operand:V2DF 3 "register_operand" "0")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (fma:DF  (subreg:DF (match_dup 1) 0)
                 (subreg:DF (match_dup 2) 0)
                 (subreg:DF (match_dup 3) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (fma:DF  (subreg:DF (match_dup 1) 8)
                 (subreg:DF (match_dup 2) 8)
                 (subreg:DF (match_dup 3) 8)))]
  ""
  [(set_attr "type" "madd_fp4")]
)

(define_insn_and_split "fnmav2df4"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (fma:V2DF (neg:V2DF (match_operand:V2DF 1 "register_operand" "r"))
                  (match_operand:V2DF 2 "register_operand" "r")
                  (match_operand:V2DF 3 "register_operand" "0")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (fma:DF  (neg:DF (subreg:DF (match_dup 1) 0))
                 (subreg:DF (match_dup 2) 0)
                 (subreg:DF (match_dup 3) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (fma:DF  (neg:DF (subreg:DF (match_dup 1) 8))
                 (subreg:DF (match_dup 2) 8)
                 (subreg:DF (match_dup 3) 8)))]
  ""
  [(set_attr "type" "madd_fp4")]
)

;; S256F (V16HF V8SF)

(define_insn "fma<mode>4"
  [(set (match_operand:S256F 0 "register_operand" "=r")
        (fma:S256F (match_operand:S256F 1 "register_operand" "r")
                   (match_operand:S256F 2 "register_operand" "r")
                   (match_operand:S256F 3 "register_operand" "0")))]
  ""
  "#"
)

(define_split
  [(set (match_operand:S256F 0 "register_operand" "")
        (fma:S256F (match_operand:S256F 1 "register_operand" "")
                   (match_operand:S256F 2 "register_operand" "")
                   (match_operand:S256F 3 "register_operand" "")))]
  "!HAVE_LVX_FMA_<HALF>_<HALF>_<HALF> && reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (fma:<CHUNK> (subreg:<CHUNK> (match_dup 1) 0)
                     (subreg:<CHUNK> (match_dup 2) 0)
                     (subreg:<CHUNK> (match_dup 3) 0)))
   (set (subreg:<CHUNK> (match_dup 0) 8)
        (fma:<CHUNK> (subreg:<CHUNK> (match_dup 1) 8)
                     (subreg:<CHUNK> (match_dup 2) 8)
                     (subreg:<CHUNK> (match_dup 3) 8)))
   (set (subreg:<CHUNK> (match_dup 0) 16)
        (fma:<CHUNK> (subreg:<CHUNK> (match_dup 1) 16)
                     (subreg:<CHUNK> (match_dup 2) 16)
                     (subreg:<CHUNK> (match_dup 3) 16)))
   (set (subreg:<CHUNK> (match_dup 0) 24)
        (fma:<CHUNK> (subreg:<CHUNK> (match_dup 1) 24)
                     (subreg:<CHUNK> (match_dup 2) 24)
                     (subreg:<CHUNK> (match_dup 3) 24)))]
  ""
)

(define_split
  [(set (match_operand:S256F 0 "register_operand" "")
        (fma:S256F (match_operand:S256F 1 "register_operand" "")
                   (match_operand:S256F 2 "register_operand" "")
                   (match_operand:S256F 3 "register_operand" "")))]
  "HAVE_LVX_FMA_<HALF>_<HALF>_<HALF> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (fma:<HALF> (subreg:<HALF> (match_dup 1) 0)
                    (subreg:<HALF> (match_dup 2) 0)
                    (subreg:<HALF> (match_dup 3) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (fma:<HALF> (subreg:<HALF> (match_dup 1) 16)
                    (subreg:<HALF> (match_dup 2) 16)
                    (subreg:<HALF> (match_dup 3) 16)))]
  ""
)

(define_insn "fnma<mode>4"
  [(set (match_operand:S256F 0 "register_operand" "=r")
        (fma:S256F (neg:S256F (match_operand:S256F 1 "register_operand" "r"))
                   (match_operand:S256F 2 "register_operand" "r")
                   (match_operand:S256F 3 "register_operand" "0")))]
  ""
  "#"
)

(define_split
  [(set (match_operand:S256F 0 "register_operand" "")
        (fma:S256F (neg:S256F (match_operand:S256F 1 "register_operand" ""))
                   (match_operand:S256F 2 "register_operand" "")
                   (match_operand:S256F 3 "register_operand" "")))]
  "!HAVE_LVX_FMS_<HALF>_<HALF>_<HALF> && reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0)
        (fma:<CHUNK> (neg:<CHUNK> (subreg:<CHUNK> (match_dup 1) 0))
                     (subreg:<CHUNK> (match_dup 2) 0)
                     (subreg:<CHUNK> (match_dup 3) 0)))
   (set (subreg:<CHUNK> (match_dup 0) 8)
        (fma:<CHUNK> (neg:<CHUNK> (subreg:<CHUNK> (match_dup 1) 8))
                     (subreg:<CHUNK> (match_dup 2) 8)
                     (subreg:<CHUNK> (match_dup 3) 8)))
   (set (subreg:<CHUNK> (match_dup 0) 16)
        (fma:<CHUNK> (neg:<CHUNK> (subreg:<CHUNK> (match_dup 1) 16))
                     (subreg:<CHUNK> (match_dup 2) 16)
                     (subreg:<CHUNK> (match_dup 3) 16)))
   (set (subreg:<CHUNK> (match_dup 0) 24)
        (fma:<CHUNK> (neg:<CHUNK> (subreg:<CHUNK> (match_dup 1) 24))
                     (subreg:<CHUNK> (match_dup 2) 24)
                     (subreg:<CHUNK> (match_dup 3) 24)))]
  ""
)

(define_split
  [(set (match_operand:S256F 0 "register_operand" "")
        (fma:S256F (neg:S256F (match_operand:S256F 1 "register_operand" ""))
                   (match_operand:S256F 2 "register_operand" "")
                   (match_operand:S256F 3 "register_operand" "")))]
  "HAVE_LVX_FMS_<HALF>_<HALF>_<HALF> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (fma:<HALF> (neg:<HALF> (subreg:<HALF> (match_dup 1) 0))
                    (subreg:<HALF> (match_dup 2) 0)
                    (subreg:<HALF> (match_dup 3) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (fma:<HALF> (neg:<HALF> (subreg:<HALF> (match_dup 1) 16))
                    (subreg:<HALF> (match_dup 2) 16)
                    (subreg:<HALF> (match_dup 3) 16)))]
  ""
)

(define_insn_and_split "trunc<wide><mode>2"
  [(set (match_operand:S256F 0 "register_operand" "=&r")
        (float_truncate:S256F (match_operand:<WIDE> 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<QUART> (match_dup 0) 0)
        (float_truncate:<QUART> (subreg:<QWIDE> (match_dup 1) 0)))
   (set (subreg:<QUART> (match_dup 0) 8)
        (float_truncate:<QUART> (subreg:<QWIDE> (match_dup 1) 16)))
   (set (subreg:<QUART> (match_dup 0) 16)
        (float_truncate:<QUART> (subreg:<QWIDE> (match_dup 1) 32)))
   (set (subreg:<QUART> (match_dup 0) 24)
        (float_truncate:<QUART> (subreg:<QWIDE> (match_dup 1) 48)))]
  ""
  [(set_attr "type" "alu_full")]
)

(define_expand "extend<mode><wide>2"
  [(set (match_operand:<WIDE> 0 "register_operand" "")
        (float_extend:<WIDE> (match_operand:S256F 1 "register_operand" "")))]
  ""
  {
    /* One FWIDEN per 128-bit half of the result: the mostsig modifier selects
       the least or most significant lanes of the 128-bit source.  This used to
       split into float_extends of 64-bit <HALF>/<QUART> chunks, whose
       sub-patterns went with 64-bit SIMD.  */
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
        emit_insn (gen_lvx_fwiden<wchunkx> (d0, src, lo));
        emit_insn (gen_lvx_fwiden<wchunkx> (d1, src, hi));
      }
    DONE;
  }
)


;; V256F (V16HF V8SF V4D)

(define_insn_and_split "fmin<mode>3"
  [(set (match_operand:V256F 0 "register_operand" "=r")
        (smin:V256F (match_operand:V256F 1 "register_operand" "r")
                    (match_operand:V256F 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (smin:<HALF> (subreg:<HALF> (match_dup 1) 0)
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (smin:<HALF> (subreg:<HALF> (match_dup 1) 16)
                     (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "*fmin<mode>3_s1"
  [(set (match_operand:V256F 0 "register_operand" "=&r")
        (smin:V256F (vec_duplicate:V256F (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256F 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (smin:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (smin:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                     (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "*fmin<mode>3_s2"
  [(set (match_operand:V256F 0 "register_operand" "=&r")
        (smin:V256F (match_operand:V256F 1 "register_operand" "r")
                    (vec_duplicate:V256F (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (smin:<HALF> (subreg:<HALF> (match_dup 1) 0)
                     (vec_duplicate:<HALF> (match_dup 2))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (smin:<HALF> (subreg:<HALF> (match_dup 1) 16)
                     (vec_duplicate:<HALF> (match_dup 2))))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "fmax<mode>3"
  [(set (match_operand:V256F 0 "register_operand" "=r")
        (smax:V256F (match_operand:V256F 1 "register_operand" "r")
                    (match_operand:V256F 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (smax:<HALF> (subreg:<HALF> (match_dup 1) 0)
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (smax:<HALF> (subreg:<HALF> (match_dup 1) 16)
                     (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "*fmax<mode>3_s1"
  [(set (match_operand:V256F 0 "register_operand" "=&r")
        (smax:V256F (vec_duplicate:V256F (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256F 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (smax:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (smax:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                     (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "*fmax<mode>3_s2"
  [(set (match_operand:V256F 0 "register_operand" "=&r")
        (smax:V256F (match_operand:V256F 1 "register_operand" "r")
                    (vec_duplicate:V256F (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (smax:<HALF> (subreg:<HALF> (match_dup 1) 0)
                     (vec_duplicate:<HALF> (match_dup 2))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (smax:<HALF> (subreg:<HALF> (match_dup 1) 16)
                     (vec_duplicate:<HALF> (match_dup 2))))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "neg<mode>2"
  [(set (match_operand:V256F 0 "register_operand" "=r")
        (neg:V256F (match_operand:V256F 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (neg:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (neg:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_insn_and_split "abs<mode>2"
  [(set (match_operand:V256F 0 "register_operand" "=r")
        (abs:V256F (match_operand:V256F 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (abs:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (abs:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu_lite_x2")]
)

(define_expand "copysign<mode>3"
  [(match_operand:V256F 0 "register_operand")
   (match_operand:V256F 1 "register_operand")
   (match_operand:V256F 2 "register_operand")]
  ""
  {
    for (int i = 0; i < 2; i++)
      {
        rtx opnd0 = simplify_gen_subreg (<HALF>mode, operands[0], <MODE>mode, i*16);
        rtx opnd1 = simplify_gen_subreg (<HALF>mode, operands[1], <MODE>mode, i*16);
        rtx opnd2 = simplify_gen_subreg (<HALF>mode, operands[2], <MODE>mode, i*16);
        emit_insn (gen_copysign<half>3 (opnd0, opnd1, opnd2));
      }
    DONE;
  }
)

(define_expand "xorsign<mode>3"
  [(match_operand:V256F 0 "register_operand")
   (match_operand:V256F 1 "register_operand")
   (match_operand:V256F 2 "register_operand")]
  ""
  {
    rtx maskv16hf __attribute__((unused)) = GEN_INT (0x8000800080008000);
    rtx maskv8sf __attribute__((unused)) = GEN_INT (0x8000000080000000);
    rtx maskv4df __attribute__((unused)) = GEN_INT (0x8000000000000000);
    rtx mask = gen_reg_rtx (DImode);
    emit_move_insn (mask, mask<mode>);
    rtx sign2 = gen_reg_rtx (<MODE>mode);
    emit_insn (gen_rtx_SET (sign2, gen_rtx_UNSPEC (<MODE>mode, gen_rtvec (2, operands[2], mask), UNSPEC_ANDD)));
    emit_insn (gen_rtx_SET (operands[0], gen_rtx_UNSPEC (<MODE>mode, gen_rtvec (2, operands[1], sign2), UNSPEC_XORD)));
    DONE;
  }
)


;; V256G (V8SF V4DF)

(define_insn_and_split "float<mask><mode>2"
  [(set (match_operand:V256G 0 "register_operand" "=r")
        (float:V256G (match_operand:<MASK> 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<QUART> (match_dup 0) 0)
        (float:<QUART> (subreg:<QMASK> (match_dup 1) 0)))
   (set (subreg:<QUART> (match_dup 0) 8)
        (float:<QUART> (subreg:<QMASK> (match_dup 1) 8)))
   (set (subreg:<QUART> (match_dup 0) 16)
        (float:<QUART> (subreg:<QMASK> (match_dup 1) 16)))
   (set (subreg:<QUART> (match_dup 0) 24)
        (float:<QUART> (subreg:<QMASK> (match_dup 1) 24)))]
  ""
)

(define_insn_and_split "floatuns<mask><mode>2"
  [(set (match_operand:V256G 0 "register_operand" "=r")
        (unsigned_float:V256G (match_operand:<MASK> 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<QUART> (match_dup 0) 0)
        (unsigned_float:<QUART> (subreg:<QMASK> (match_dup 1) 0)))
   (set (subreg:<QUART> (match_dup 0) 8)
        (unsigned_float:<QUART> (subreg:<QMASK> (match_dup 1) 8)))
   (set (subreg:<QUART> (match_dup 0) 16)
        (unsigned_float:<QUART> (subreg:<QMASK> (match_dup 1) 16)))
   (set (subreg:<QUART> (match_dup 0) 24)
        (unsigned_float:<QUART> (subreg:<QMASK> (match_dup 1) 24)))]
  ""
)

(define_insn_and_split "fix_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "=r")
        (fix:<MASK> (match_operand:V256G 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<QMASK> (match_dup 0) 0)
        (fix:<QMASK> (subreg:<QUART> (match_dup 1) 0)))
   (set (subreg:<QMASK> (match_dup 0) 8)
        (fix:<QMASK> (subreg:<QUART> (match_dup 1) 8)))
   (set (subreg:<QMASK> (match_dup 0) 16)
        (fix:<QMASK> (subreg:<QUART> (match_dup 1) 16)))
   (set (subreg:<QMASK> (match_dup 0) 24)
        (fix:<QMASK> (subreg:<QUART> (match_dup 1) 24)))]
  ""
)

(define_insn_and_split "fixuns_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "=r")
        (unsigned_fix:<MASK> (match_operand:V256G 1 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:<QMASK> (match_dup 0) 0)
        (unsigned_fix:<QMASK> (subreg:<QUART> (match_dup 1) 0)))
   (set (subreg:<QMASK> (match_dup 0) 8)
        (unsigned_fix:<QMASK> (subreg:<QUART> (match_dup 1) 8)))
   (set (subreg:<QMASK> (match_dup 0) 16)
        (unsigned_fix:<QMASK> (subreg:<QUART> (match_dup 1) 16)))
   (set (subreg:<QMASK> (match_dup 0) 24)
        (unsigned_fix:<QMASK> (subreg:<QUART> (match_dup 1) 24)))]
  ""
)


;; V16HF

(define_insn "addv16hf3"
  [(set (match_operand:V16HF 0 "register_operand" "=r")
        (plus:V16HF (match_operand:V16HF 1 "register_operand" "r")
                   (match_operand:V16HF 2 "register_operand" "r")))]
  ""
  "#"
)

(define_split
  [(set (match_operand:V16HF 0 "register_operand" "")
        (plus:V16HF (match_operand:V16HF 1 "register_operand" "")
                   (match_operand:V16HF 2 "register_operand" "")))]
  "HAVE_LVX_PLUS_V8HF && reload_completed"
  [(set (subreg:V8HF (match_dup 0) 0)
        (plus:V8HF (subreg:V8HF (match_dup 1) 0)
                   (subreg:V8HF (match_dup 2) 0)))
   (set (subreg:V8HF (match_dup 0) 16)
        (plus:V8HF (subreg:V8HF (match_dup 1) 16)
                   (subreg:V8HF (match_dup 2) 16)))]
  ""
)

(define_insn "subv16hf3"
  [(set (match_operand:V16HF 0 "register_operand" "=r")
        (minus:V16HF (match_operand:V16HF 1 "register_operand" "r")
                    (match_operand:V16HF 2 "register_operand" "r")))]
  ""
  "#"
)

(define_split
  [(set (match_operand:V16HF 0 "register_operand" "")
        (minus:V16HF (match_operand:V16HF 1 "register_operand" "")
                    (match_operand:V16HF 2 "register_operand" "")))]
  "HAVE_LVX_MINUS_V8HF && reload_completed"
  [(set (subreg:V8HF (match_dup 0) 0)
        (minus:V8HF (subreg:V8HF (match_dup 1) 0)
                    (subreg:V8HF (match_dup 2) 0)))
   (set (subreg:V8HF (match_dup 0) 16)
        (minus:V8HF (subreg:V8HF (match_dup 1) 16)
                    (subreg:V8HF (match_dup 2) 16)))]
  ""
)

(define_insn "mulv16hf3"
  [(set (match_operand:V16HF 0 "register_operand" "=r")
        (mult:V16HF (match_operand:V16HF 1 "register_operand" "r")
                   (match_operand:V16HF 2 "register_operand" "r")))]
  ""
  "#"
)

(define_split
  [(set (match_operand:V16HF 0 "register_operand" "")
        (mult:V16HF (match_operand:V16HF 1 "register_operand" "")
                   (match_operand:V16HF 2 "register_operand" "")))]
  "HAVE_LVX_MULT_V8HF && reload_completed"
  [(set (subreg:V8HF (match_dup 0) 0)
        (mult:V8HF (subreg:V8HF (match_dup 1) 0)
                   (subreg:V8HF (match_dup 2) 0)))
   (set (subreg:V8HF (match_dup 0) 16)
        (mult:V8HF (subreg:V8HF (match_dup 1) 16)
                   (subreg:V8HF (match_dup 2) 16)))]
  ""
)


;; V8SF

(define_insn "addv8sf3"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (plus:V8SF (match_operand:V8SF 1 "register_operand" "r")
                   (match_operand:V8SF 2 "register_operand" "r")))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (plus:V8SF (match_operand:V8SF 1 "register_operand" "")
                   (match_operand:V8SF 2 "register_operand" "")))]
  "HAVE_LVX_PLUS_V4SF && reload_completed"
  [(set (subreg:V4SF (match_dup 0) 0)
        (plus:V4SF (subreg:V4SF (match_dup 1) 0)
                   (subreg:V4SF (match_dup 2) 0)))
   (set (subreg:V4SF (match_dup 0) 16)
        (plus:V4SF (subreg:V4SF (match_dup 1) 16)
                   (subreg:V4SF (match_dup 2) 16)))]
  ""
)

(define_insn "subv8sf3"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (minus:V8SF (match_operand:V8SF 1 "register_operand" "r")
                    (match_operand:V8SF 2 "register_operand" "r")))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (minus:V8SF (match_operand:V8SF 1 "register_operand" "")
                    (match_operand:V8SF 2 "register_operand" "")))]
  "HAVE_LVX_MINUS_V4SF && reload_completed"
  [(set (subreg:V4SF (match_dup 0) 0)
        (minus:V4SF (subreg:V4SF (match_dup 1) 0)
                    (subreg:V4SF (match_dup 2) 0)))
   (set (subreg:V4SF (match_dup 0) 16)
        (minus:V4SF (subreg:V4SF (match_dup 1) 16)
                    (subreg:V4SF (match_dup 2) 16)))]
  ""
)

(define_insn "mulv8sf3"
  [(set (match_operand:V8SF 0 "register_operand" "=r")
        (mult:V8SF (match_operand:V8SF 1 "register_operand" "r")
                   (match_operand:V8SF 2 "register_operand" "r")))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (mult:V8SF (match_operand:V8SF 1 "register_operand" "")
                   (match_operand:V8SF 2 "register_operand" "")))]
  "HAVE_LVX_MULT_V4SF && reload_completed"
  [(set (subreg:V4SF (match_dup 0) 0)
        (mult:V4SF (subreg:V4SF (match_dup 1) 0)
                   (subreg:V4SF (match_dup 2) 0)))
   (set (subreg:V4SF (match_dup 0) 16)
        (mult:V4SF (subreg:V4SF (match_dup 1) 16)
                   (subreg:V4SF (match_dup 2) 16)))]
  ""
)

(define_expand "floatv8hiv8sf2"
  [(set (match_operand:V8SF 0 "register_operand" "")
        (float:V8SF (match_operand:V8HI 1 "register_operand" "")))
   (clobber (match_dup 2))]
  ""
  {
    operands[2] = gen_reg_rtx (V8SImode);
    emit_insn (gen_lvx_sxhwo (operands[2], operands[1]));
    emit_insn (gen_floatv8siv8sf2 (operands[0], operands[2]));
    DONE;
  }
)

(define_expand "floatunsv8hiv8sf2"
  [(set (match_operand:V8SF 0 "register_operand" "")
        (unsigned_float:V8SF (match_operand:V8HI 1 "register_operand" "")))
   (clobber (match_dup 2))]
  ""
  {
    operands[2] = gen_reg_rtx (V8SImode);
    emit_insn (gen_lvx_zxhwo (operands[2], operands[1]));
    emit_insn (gen_floatunsv8siv8sf2 (operands[0], operands[2]));
    DONE;
  }
)

(define_expand "fix_truncv8sfv8hi2"
  [(set (match_operand:V8HI 0 "register_operand" "")
        (truncate:V8HI (fix:V8SI (match_operand:V8SF 1 "register_operand" ""))))
   (clobber (match_dup 2))]
  ""
  {
    operands[2] = gen_reg_rtx (V8SImode);
    emit_insn (gen_fix_truncv8sfv8si2 (operands[2], operands[1]));
    emit_insn (gen_lvx_truncwho (operands[0], operands[2]));
    DONE;
  }
)

(define_expand "fixuns_truncv8sfv8hi2"
  [(set (match_operand:V8HI 0 "register_operand" "")
        (truncate:V8HI (unsigned_fix:V8SI (match_operand:V8SF 1 "register_operand" ""))))
   (clobber (match_dup 2))]
  ""
  {
    operands[2] = gen_reg_rtx (V8SImode);
    emit_insn (gen_fixuns_truncv8sfv8si2 (operands[2], operands[1]));
    emit_insn (gen_lvx_truncwho (operands[0], operands[2]));
    DONE;
  }
)


;; V4DF

(define_insn "addv4df3"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (plus:V4DF (match_operand:V4DF 1 "register_operand" "r")
                   (match_operand:V4DF 2 "register_operand" "r")))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (plus:V4DF (match_operand:V4DF 1 "register_operand" "")
                   (match_operand:V4DF 2 "register_operand" "")))]
  "HAVE_LVX_PLUS_V2DF && reload_completed"
  [(set (subreg:V2DF (match_dup 0) 0)
        (plus:V2DF (subreg:V2DF (match_dup 1) 0)
                   (subreg:V2DF (match_dup 2) 0)))
   (set (subreg:V2DF (match_dup 0) 16)
        (plus:V2DF (subreg:V2DF (match_dup 1) 16)
                   (subreg:V2DF (match_dup 2) 16)))]
  ""
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (plus:V4DF (match_operand:V4DF 1 "register_operand" "")
                   (match_operand:V4DF 2 "register_operand" "")))]
  "!HAVE_LVX_PLUS_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (plus:DF (subreg:DF (match_dup 1) 0)
                 (subreg:DF (match_dup 2) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (plus:DF (subreg:DF (match_dup 1) 8)
                 (subreg:DF (match_dup 2) 8)))
   (set (subreg:DF (match_dup 0) 16)
        (plus:DF (subreg:DF (match_dup 1) 16)
                 (subreg:DF (match_dup 2) 16)))
   (set (subreg:DF (match_dup 0) 24)
        (plus:DF (subreg:DF (match_dup 1) 24)
                 (subreg:DF (match_dup 2) 24)))]
  ""
)

(define_insn "subv4df3"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (minus:V4DF (match_operand:V4DF 1 "register_operand" "r")
                    (match_operand:V4DF 2 "register_operand" "r")))]
  ""
  "#"
  [(set_attr "type" "madd_fp4")]
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (minus:V4DF (match_operand:V4DF 1 "register_operand" "")
                    (match_operand:V4DF 2 "register_operand" "")))]
  "HAVE_LVX_MINUS_V2DF && reload_completed"
  [(set (subreg:V2DF (match_dup 0) 0)
        (minus:V2DF (subreg:V2DF (match_dup 1) 0)
                    (subreg:V2DF (match_dup 2) 0)))
   (set (subreg:V2DF (match_dup 0) 16)
        (minus:V2DF (subreg:V2DF (match_dup 1) 16)
                    (subreg:V2DF (match_dup 2) 16)))]
  ""
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (minus:V4DF (match_operand:V4DF 1 "register_operand" "")
                    (match_operand:V4DF 2 "register_operand" "")))]
  "!HAVE_LVX_MINUS_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (minus:DF (subreg:DF (match_dup 1) 0)
                  (subreg:DF (match_dup 2) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (minus:DF (subreg:DF (match_dup 1) 8)
                  (subreg:DF (match_dup 2) 8)))
   (set (subreg:DF (match_dup 0) 16)
        (minus:DF (subreg:DF (match_dup 1) 16)
                  (subreg:DF (match_dup 2) 16)))
   (set (subreg:DF (match_dup 0) 24)
        (minus:DF (subreg:DF (match_dup 1) 24)
                  (subreg:DF (match_dup 2) 24)))]
  ""
)

(define_insn_and_split "mulv4df3"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (mult:V4DF (match_operand:V4DF 1 "register_operand" "r")
                   (match_operand:V4DF 2 "register_operand" "r")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (mult:DF (subreg:DF (match_dup 1) 0)
                 (subreg:DF (match_dup 2) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (mult:DF (subreg:DF (match_dup 1) 8)
                 (subreg:DF (match_dup 2) 8)))
   (set (subreg:DF (match_dup 0) 16)
        (mult:DF (subreg:DF (match_dup 1) 16)
                 (subreg:DF (match_dup 2) 16)))
   (set (subreg:DF (match_dup 0) 24)
        (mult:DF (subreg:DF (match_dup 1) 24)
                 (subreg:DF (match_dup 2) 24)))]
  ""
  [(set_attr "type" "madd_fp4")]
)

(define_insn_and_split "fmav4df4"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (fma:V4DF (match_operand:V4DF 1 "register_operand" "r")
                  (match_operand:V4DF 2 "register_operand" "r")
                  (match_operand:V4DF 3 "register_operand" "0")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (fma:DF  (subreg:DF (match_dup 1) 0)
                 (subreg:DF (match_dup 2) 0)
                 (subreg:DF (match_dup 3) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (fma:DF  (subreg:DF (match_dup 1) 8)
                 (subreg:DF (match_dup 2) 8)
                 (subreg:DF (match_dup 3) 8)))
   (set (subreg:DF (match_dup 0) 16)
        (fma:DF  (subreg:DF (match_dup 1) 16)
                 (subreg:DF (match_dup 2) 16)
                 (subreg:DF (match_dup 3) 16)))
   (set (subreg:DF (match_dup 0) 24)
        (fma:DF  (subreg:DF (match_dup 1) 24)
                 (subreg:DF (match_dup 2) 24)
                 (subreg:DF (match_dup 3) 24)))]
  ""
  [(set_attr "type" "madd_fp4")]
)

(define_insn_and_split "fnmav4df4"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (fma:V4DF (neg:V4DF (match_operand:V4DF 1 "register_operand" "r"))
                  (match_operand:V4DF 2 "register_operand" "r")
                  (match_operand:V4DF 3 "register_operand" "0")))]
  ""
  "#"
  "reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (fma:DF  (neg:DF (subreg:DF (match_dup 1) 0))
                 (subreg:DF (match_dup 2) 0)
                 (subreg:DF (match_dup 3) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (fma:DF  (neg:DF (subreg:DF (match_dup 1) 8))
                 (subreg:DF (match_dup 2) 8)
                 (subreg:DF (match_dup 3) 8)))
   (set (subreg:DF (match_dup 0) 16)
        (fma:DF  (neg:DF (subreg:DF (match_dup 1) 16))
                 (subreg:DF (match_dup 2) 16)
                 (subreg:DF (match_dup 3) 16)))
   (set (subreg:DF (match_dup 0) 24)
        (fma:DF  (neg:DF (subreg:DF (match_dup 1) 24))
                 (subreg:DF (match_dup 2) 24)
                 (subreg:DF (match_dup 3) 24)))]
  ""
  [(set_attr "type" "madd_fp4")]
)

(define_expand "vec_unpacks_hi_<packi>"
  [(set (match_operand:UNPACKI 0 "register_operand")
        (match_operand:<PACKI> 1 "register_operand"))]
  ""
  {
    lvx_expand_unpack (operands[0], operands[1], /*signed_p*/1, /*hi_p*/1);
    DONE;
  }
)

(define_expand "vec_unpacks_lo_<packi>"
  [(set (match_operand:UNPACKI 0 "register_operand")
        (match_operand:<PACKI> 1 "register_operand"))]
  ""
  {
    lvx_expand_unpack (operands[0], operands[1], /*signed_p*/1, /*hi_p*/0);
    DONE;
  }
)

(define_expand "vec_unpacku_hi_<packi>"
  [(match_operand:UNPACKI 0 "register_operand")
   (match_operand:<PACKI> 1 "register_operand")]
  ""
  {
    lvx_expand_unpack (operands[0], operands[1], /*signed_p*/0, /*hi_p*/1);
    DONE;
  }
)

(define_expand "vec_unpacku_lo_<packi>"
  [(match_operand:UNPACKI 0 "register_operand")
   (match_operand:<PACKI> 1 "register_operand")]
  ""
  {
    lvx_expand_unpack (operands[0], operands[1], /*signed_p*/0, /*hi_p*/0);
    DONE;
  }
)


;; V512G

(define_expand "float<mask><mode>2"
  [(set (match_operand:V512G 0 "register_operand" "")
        (float:V512G (match_operand:<MASK> 1 "register_operand" "")))]
  ""
  {
    rtx operand1 = gen_reg_rtx (<MASK>mode);
    rtx operand0 = gen_reg_rtx (<MODE>mode);
    emit_move_insn (operand1, operands[1]);
    emit_insn (gen_float<hmask><half>2 (gen_rtx_SUBREG (<HALF>mode, operand0, 0),
                                        gen_rtx_SUBREG (<HMASK>mode, operand1, 0)));
    emit_insn (gen_float<hmask><half>2 (gen_rtx_SUBREG (<HALF>mode, operand0, 32),
                                        gen_rtx_SUBREG (<HMASK>mode, operand1, 32)));
    emit_move_insn (operands[0], operand0);
    DONE;
  }
)

(define_expand "floatuns<mask><mode>2"
  [(set (match_operand:V512G 0 "register_operand" "")
        (float:V512G (match_operand:<MASK> 1 "register_operand" "")))]
  ""
  {
    rtx operand1 = gen_reg_rtx (<MASK>mode);
    rtx operand0 = gen_reg_rtx (<MODE>mode);
    emit_move_insn (operand1, operands[1]);
    emit_insn (gen_floatuns<hmask><half>2 (gen_rtx_SUBREG (<HALF>mode, operand0, 0),
                                           gen_rtx_SUBREG (<HMASK>mode, operand1, 0)));
    emit_insn (gen_floatuns<hmask><half>2 (gen_rtx_SUBREG (<HALF>mode, operand0, 32),
                                           gen_rtx_SUBREG (<HMASK>mode, operand1, 32)));
    emit_move_insn (operands[0], operand0);
    DONE;
  }
)

(define_expand "fix_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "")
        (fix:<MASK> (match_operand:V512G 1 "register_operand" "")))]
  ""
  {
    rtx operand1 = gen_reg_rtx (<MODE>mode);
    rtx operand0 = gen_reg_rtx (<MASK>mode);
    emit_move_insn (operand1, operands[1]);
    emit_insn (gen_fix_trunc<half><hmask>2 (gen_rtx_SUBREG (<HMASK>mode, operand0, 0),
                                            gen_rtx_SUBREG (<HALF>mode, operand1, 0)));
    emit_insn (gen_fix_trunc<half><hmask>2 (gen_rtx_SUBREG (<HMASK>mode, operand0, 32),
                                            gen_rtx_SUBREG (<HALF>mode, operand1, 32)));
    emit_move_insn (operands[0], operand0);
    DONE;
  }
)

(define_expand "fixuns_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "")
        (fix:<MASK> (match_operand:V512G 1 "register_operand" "")))]
  ""
  {
    rtx operand1 = gen_reg_rtx (<MODE>mode);
    rtx operand0 = gen_reg_rtx (<MASK>mode);
    emit_move_insn (operand1, operands[1]);
    emit_insn (gen_fixuns_trunc<half><hmask>2 (gen_rtx_SUBREG (<HMASK>mode, operand0, 0),
                                               gen_rtx_SUBREG (<HALF>mode, operand1, 0)));
    emit_insn (gen_fixuns_trunc<half><hmask>2 (gen_rtx_SUBREG (<HMASK>mode, operand0, 32),
                                               gen_rtx_SUBREG (<HALF>mode, operand1, 32)));
    emit_move_insn (operands[0], operand0);
    DONE;
  }
)

;; ---- Restored: native wide insns whose only 64-bit content was a
;; ---- split fallback for a HAVE_LVX_* capability macro frozen at (0).

(define_insn "addv8hf3"
  [(set (match_operand:V8HF 0 "register_operand" "=r")
        (plus:V8HF (match_operand:V8HF 1 "register_operand" "r")
                   (match_operand:V8HF 2 "register_operand" "r")))]
  "LVX_2"
  "faddho %0 = %1, %2"
  [(set_attr "type" "madd_fp3")]
)

(define_insn "subv8hf3"
  [(set (match_operand:V8HF 0 "register_operand" "=r")
        (minus:V8HF (match_operand:V8HF 1 "register_operand" "r")
                    (match_operand:V8HF 2 "register_operand" "r")))]
  "LVX_2"
  "fsbfho %0 = %2, %1"
  [(set_attr "type" "mult_fp3")]
)

(define_insn "mulv8hf3"
  [(set (match_operand:V8HF 0 "register_operand" "=r")
        (mult:V8HF (match_operand:V8HF 1 "register_operand" "r")
                   (match_operand:V8HF 2 "register_operand" "r")))]
  "LVX_2"
  "fmulho %0 = %1, %2"
  [(set_attr "type" "mult_fp3")]
)

(define_insn "addv4sf3"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (plus:V4SF (match_operand:V4SF 1 "register_operand" "r")
                   (match_operand:V4SF 2 "register_operand" "r")))]
  "LVX_2"
  "faddwq %0 = %1, %2"
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn "subv4sf3"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (minus:V4SF (match_operand:V4SF 1 "register_operand" "r")
                    (match_operand:V4SF 2 "register_operand" "r")))]
  "LVX_2"
  "fsbfwq %0 = %2, %1"
  [(set_attr "type" "mulwq_fp4")]
)

(define_insn "mulv4sf3"
  [(set (match_operand:V4SF 0 "register_operand" "=r")
        (mult:V4SF (match_operand:V4SF 1 "register_operand" "r")
                   (match_operand:V4SF 2 "register_operand" "r")))]
  "LVX_2"
  "fmulwq %0 = %1, %2"
  [(set_attr "type" "mulwq_fp4")]
)
