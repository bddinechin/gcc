


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
  [(set (match_operand:ALL128 0 "nonimmediate_operand" "=r,r,r,r,a,b,m")
        (match_operand:ALL128 1 "nonimmediate_operand"  "r,a,b,m,r,r,r"))]
  "lvx_hardreg_aligned_p (operands[0], <MODE>mode) && lvx_hardreg_aligned_p (operands[1], <MODE>mode)"
  {
    switch (which_alternative)
      {
      case 0:
        return "#";
      case 1: case 2: case 3:
        return "lq%V1 %0 = %1";
      case 4: case 5: case 6:
        return "sq%X0 %0 = %1";
      default:
        gcc_unreachable ();
      }
  }
  [(set_attr "type" "alu, load, load, load, store, store, store")
   (set_attr "issue" "tiny2, lsu_auxw, lsu_auxw_x, lsu_auxw_x2, lsu_memw_auxr, lsu_memw_auxr_x, lsu_memw_auxr_x2")
   (set_attr "length"         "8,             4,             8,             12,             4,               8,              12")]
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
  [(set (match_operand:ALL256 0 "nonimmediate_operand" "=r,r,r,r,a,b,m")
        (match_operand:ALL256 1 "nonimmediate_operand"  "r,a,b,m,r,r,r"))]
  "lvx_hardreg_aligned_p (operands[0], <MODE>mode) && lvx_hardreg_aligned_p (operands[1], <MODE>mode)"
  {
    switch (which_alternative)
      {
      case 0:
        return "#";
      case 1: case 2: case 3:
        return "lo%V1 %0 = %1";
      case 4: case 5: case 6:
        return "so%X0 %0 = %1";
      default:
        gcc_unreachable ();
      }
  }
  [(set_attr "type" "alu, load, load, load, store, store, store")
   (set_attr "issue" "tiny4, lsu_auxw, lsu_auxw_x, lsu_auxw_x2, lsu_memw_auxr, lsu_memw_auxr_x, lsu_memw_auxr_x2")
   (set_attr "length"        "16,            4,              8,             12,             4,               8,              12")]
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
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2 && ((GET_MODE_NUNITS (<SIMDCMP:MODE>mode) == GET_MODE_NUNITS (<SIMDALL:MODE>mode)))"
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
  "LVX_2 && ((GET_MODE_NUNITS (<SIMDCMP:MODE>mode) == GET_MODE_NUNITS (<SIMDALL:MODE>mode)))"
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
  "LVX_2"
  {
    rtx target = operands[0];
    rtx select1 = operands[1];
    rtx select2 = operands[2];
    rtx mask = operands[3];
    lvx_expand_masked_move (target, select1, select2, mask);
    DONE;
  }
)

;; -------------------------------------------------------------------------
;; Bit-per-lane mask predication for the 128-bit integer SIMD modes.  COMP*
;; compares two vectors and packs one bit per lane into a GPR (the <LANEMASK>
;; integer mode); BLEND* selects lanes from a source into the destination by
;; that GPR bit-mask.  lvx_get_mask_mode makes the vectorizer request these
;; instead of full-width 0/-1 vector masks -- one COMP and one BLEND for a
;; select, against the full-width AND/ANDC/IOR.  compbx/ho/wq/dp and their
;; blend siblings cover byte/half/word/double lanes.
;; -------------------------------------------------------------------------

(define_insn "vec_cmp<mode><lanemask>"
  [(set (match_operand:<LANEMASK> 0 "register_operand" "=r")
        (unspec:<LANEMASK>
          [(match_operator:<LANEMASK> 1 "comparison_operator"
            [(match_operand:SIMD128I 2 "register_operand" "r")
             (match_operand:SIMD128I 3 "register_operand" "r")])]
          UNSPEC_PACKCMP))]
  "LVX_2"
  "comp<compx>.%1 %0 = %2, %3"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")])

(define_insn "vec_cmpu<mode><lanemask>"
  [(set (match_operand:<LANEMASK> 0 "register_operand" "=r")
        (unspec:<LANEMASK>
          [(match_operator:<LANEMASK> 1 "comparison_operator"
            [(match_operand:SIMD128I 2 "register_operand" "r")
             (match_operand:SIMD128I 3 "register_operand" "r")])]
          UNSPEC_PACKCMP))]
  "LVX_2"
  "comp<compx>.%1 %0 = %2, %3"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")])

(define_insn "lvx_blend<compx>"
  [(set (match_operand:SIMD128I 0 "register_operand" "=r")
        (unspec:SIMD128I
          [(match_operand:SIMD128I 1 "register_operand" "0")
           (match_operand:SIMD128I 2 "register_operand" "r")
           (match_operand:<LANEMASK> 3 "register_operand" "r")]
          UNSPEC_LVX_BLEND))]
  "LVX_2"
  "blend<compx> %0 = %2, %3"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")])

(define_expand "vcond_mask_<mode><lanemask>"
  [(match_operand:SIMD128I 0 "register_operand")
   (match_operand:SIMD128I 1 "register_operand")
   (match_operand:SIMD128I 2 "register_operand")
   (match_operand:<LANEMASK> 3 "register_operand")]
  "LVX_2"
  {
    /* vcond_mask: dst = mask ? op1 : op2.  BLEND* does dst = mask ? src : dst,
       so seed dst with the false value (op2) then blend in op1 where set.  */
    rtx dst = operands[0];
    if (!rtx_equal_p (dst, operands[2]))
      emit_move_insn (dst, operands[2]);
    emit_insn (gen_lvx_blend<compx> (dst, dst, operands[1], operands[3]));
    DONE;
  })

;; The float 128-bit modes take the same bit-per-lane GPR mask, from FCOMP*.
;; vec_cmp goes through lvx_lower_comparison, which canonicalises the FP
;; condition to what FCOMP* prints (float_comparison_operator) -- swapping
;; operands or splitting UNORDERED/ORDERED into two compares joined by an
;; AND/IOR of the packed masks -- and wraps each compare in UNSPEC_PACKCMP.
(define_expand "vec_cmp<mode><lanemask>"
  [(set (match_operand:<LANEMASK> 0 "register_operand")
        (match_operator:<LANEMASK> 1 "comparison_operator"
          [(match_operand:SIMD128F 2 "register_operand")
           (match_operand:SIMD128F 3 "register_operand")]))]
  "LVX_2"
  {
    lvx_lower_comparison (operands[0], operands[1], <MODE>mode);
    DONE;
  })

(define_insn "*fcomp<mode><lanemask>"
  [(set (match_operand:<LANEMASK> 0 "register_operand" "=r")
        (unspec:<LANEMASK>
          [(match_operator:<LANEMASK> 1 "float_comparison_operator"
            [(match_operand:SIMD128F 2 "register_operand" "r")
             (match_operand:SIMD128F 3 "register_operand" "r")])]
          UNSPEC_PACKCMP))]
  "LVX_2"
  "fcomp<compx>.%F1 %0 = %2, %3"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")])

(define_expand "vcond_mask_<mode><lanemask>"
  [(match_operand:SIMD128F 0 "register_operand")
   (match_operand:SIMD128F 1 "register_operand")
   (match_operand:SIMD128F 2 "register_operand")
   (match_operand:<LANEMASK> 3 "register_operand")]
  "LVX_2"
  {
    /* BLEND* is a lane bit-select on the register bits, type-agnostic, so view
       the float operands as their integer sibling and reuse the integer blend:
       seed dst with the false value, blend in the true one where the mask set.  */
    machine_mode im = <vintmode>mode;
    rtx dst = simplify_gen_subreg (im, operands[0], <MODE>mode, 0);
    rtx op1 = simplify_gen_subreg (im, operands[1], <MODE>mode, 0);
    rtx op2 = simplify_gen_subreg (im, operands[2], <MODE>mode, 0);
    gcc_assert (dst && op1 && op2);
    if (!rtx_equal_p (dst, op2))
      emit_move_insn (dst, op2);
    emit_insn (gen_lvx_blend<compx> (dst, dst, op1, operands[3]));
    DONE;
  })

;; -------------------------------------------------------------------------
;; Masked load/store (MASKM).  MASKM is a BCU prefix over a plain lq/sq whose
;; byte-enable mask says which bytes to touch.  The vectorizer's mask is a lane
;; bit-mask (lvx_get_mask_mode); EXTB{2,4,8}D expand each lane bit into its
;; lane's bytes, so a 128-bit access always feeds MASKM 16 byte enables.  A byte
;; vector needs no expansion -- its lane mask already is the byte mask.
;; -------------------------------------------------------------------------

;; EXTB{2,4,8}D issue as TINY, not LITE: the ISA gives all three
;; Scheduling-ALU_TINY, and claiming a LITE slot they do not need only costs
;; bundle density (MDS/BE/GCC/BIN/check-scheduling.py reported it).
(define_insn "lvx_extb2d"
  [(set (match_operand:HI 0 "register_operand" "=r")
        (unspec:HI [(match_operand:QI 1 "register_operand" "r")] UNSPEC_EXTB2))]
  "LVX_2"
  "extb2d %0 = %1"
  [(set_attr "type" "alu") (set_attr "issue" "tiny") (set_attr "length" "4")])

(define_insn "lvx_extb4d"
  [(set (match_operand:HI 0 "register_operand" "=r")
        (unspec:HI [(match_operand:QI 1 "register_operand" "r")] UNSPEC_EXTB4))]
  "LVX_2"
  "extb4d %0 = %1"
  [(set_attr "type" "alu") (set_attr "issue" "tiny") (set_attr "length" "4")])

(define_insn "lvx_extb8d"
  [(set (match_operand:HI 0 "register_operand" "=r")
        (unspec:HI [(match_operand:QI 1 "register_operand" "r")] UNSPEC_EXTB8))]
  "LVX_2"
  "extb8d %0 = %1"
  [(set_attr "type" "alu") (set_attr "issue" "tiny") (set_attr "length" "4")])

;; The MASKM-prefixed lq/sq.  The mask (byte enables) is a real operand so the
;; allocator sees it and the scheduler the edge from EXTB*D; the UNSPEC_MASKM
;; use is the marker lvx_sched_dfa_new_cycle keys BCU-slot sharing on.  Length
;; is the lq/sq's own size, as for a guarded op -- the prefix syllable is
;; accounted by the scheduler (masked attr -> bcu_used), not counted here.
(define_insn "lvx_maskload<mode>"
  [(set (match_operand:SIMD128I 0 "register_operand" "=r,r,r")
        (unspec:SIMD128I
          [(match_operand:SIMD128I 1 "memory_operand" "a,b,m")
           (match_operand:HI 2 "register_operand" "r,r,r")]
          UNSPEC_MASKM_LOAD))
   (use (unspec:HI [(match_dup 2) (const_int 1)] UNSPEC_MASKM))]
  "LVX_2"
  "maskm.mt %2? lq%V1 %0 = %1"
  [(set_attr "masked" "yes")
   (set_attr "predicable" "no")
   (set_attr "type" "load,load,load")
   (set_attr "issue" "lsu_auxw,lsu_auxw_x,lsu_auxw_x2")
   (set_attr "length" "4,8,12")])

(define_insn "lvx_maskstore<mode>"
  [(set (match_operand:SIMD128I 0 "memory_operand" "=a,b,m")
        (unspec:SIMD128I
          [(match_operand:SIMD128I 1 "register_operand" "r,r,r")
           (match_operand:HI 2 "register_operand" "r,r,r")]
          UNSPEC_MASKM_STORE))
   (use (unspec:HI [(match_dup 2) (const_int 1)] UNSPEC_MASKM))]
  "LVX_2"
  "maskm.mt %2? sq%X0 %0 = %1"
  [(set_attr "masked" "yes")
   (set_attr "predicable" "no")
   (set_attr "type" "store,store,store")
   (set_attr "issue" "lsu_memw_auxr,lsu_memw_auxr_x,lsu_memw_auxr_x2")
   (set_attr "length" "4,8,12")])

;; The optabs the vectorizer requests.  Bridge the lane mask to byte enables
;; (EXTB*D, or identity for a byte vector) then emit the prefixed lq/sq.
(define_expand "maskload<mode><lanemask>"
  [(match_operand:SIMD128I 0 "register_operand")
   (match_operand:SIMD128I 1 "memory_operand")
   (match_operand:<LANEMASK> 2 "register_operand")
   (match_operand:SIMD128I 3 "maskload_else_operand")]
  "LVX_2"
  {
    /* operands[3] is the else value; LVX supports only the undefined case
       (see maskload_else_operand), so there is nothing to materialise.  */
    unsigned elt = GET_MODE_UNIT_SIZE (<MODE>mode);
    rtx bytes;
    if (elt == 1)
      bytes = operands[2];
    else
      {
        bytes = gen_reg_rtx (HImode);
        if (elt == 2)      emit_insn (gen_lvx_extb2d (bytes, operands[2]));
        else if (elt == 4) emit_insn (gen_lvx_extb4d (bytes, operands[2]));
        else               emit_insn (gen_lvx_extb8d (bytes, operands[2]));
      }
    emit_insn (gen_lvx_maskload<mode> (operands[0], operands[1], bytes));
    DONE;
  })

(define_expand "maskstore<mode><lanemask>"
  [(match_operand:SIMD128I 0 "memory_operand")
   (match_operand:SIMD128I 1 "register_operand")
   (match_operand:<LANEMASK> 2 "register_operand")]
  "LVX_2"
  {
    unsigned elt = GET_MODE_UNIT_SIZE (<MODE>mode);
    rtx bytes;
    if (elt == 1)
      bytes = operands[2];
    else
      {
        bytes = gen_reg_rtx (HImode);
        if (elt == 2)      emit_insn (gen_lvx_extb2d (bytes, operands[2]));
        else if (elt == 4) emit_insn (gen_lvx_extb4d (bytes, operands[2]));
        else               emit_insn (gen_lvx_extb8d (bytes, operands[2]));
      }
    emit_insn (gen_lvx_maskstore<mode> (operands[0], operands[1], bytes));
    DONE;
  })

(define_expand "vec_shl_insert_<mode>"
  [(match_operand:SIMDALL 0 "register_operand" "")
   (match_operand:SIMDALL 1 "register_operand" "")
   (match_operand:<INNER> 2 "register_operand" "")]
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
  "addd %x0 = %x1, %2\n\taddd %y0 = %y1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny2")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn "*andd"
  [(set (match_operand:FITGPR 0 "register_operand" "=r,r,r,r")
        (unspec:FITGPR [(match_operand:SCALAR 1 "register_operand" "r,r,r,r")
                        (match_operand:WI 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")] UNSPEC_ANDD))]
  "LVX_2"
  "andd %0 = %1, %2"
  [(set_attr "type" "alu, alu, alu, alu")
   (set_attr "issue" "tiny, tiny, tiny_x, tiny_x2")
   (set_attr "length" "4,4,8,12")]
)

(define_insn "*anddp"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (unspec:ALL128 [(match_operand:SIMD128 1 "register_operand" "r")
                        (match_operand:DI 2 "register_operand" "r")] UNSPEC_ANDD))]
  "LVX_2"
  "andq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn "*anddp"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (unspec:ALL128 [(match_operand:ALL128 1 "register_operand" "r")
                        (match_operand:ALL128 2 "register_operand" "r")] UNSPEC_ANDD))]
  "LVX_2"
  "andq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn "*xord"
  [(set (match_operand:FITGPR 0 "register_operand" "=r,r,r,r")
        (unspec:FITGPR [(match_operand:FITGPR 1 "register_operand" "r,r,r,r")
                        (match_operand:SCALAR 2 "lvx_r_s10_s37_s64_operand" "r,I10,I37,i")] UNSPEC_XORD))]
  "LVX_2"
  "eord %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn "*xordp"
  [(set (match_operand:ALL128 0 "register_operand" "=r")
        (unspec:ALL128 [(match_operand:ALL128 1 "register_operand" "r")
                        (match_operand:ALL128 2 "register_operand" "r")] UNSPEC_XORD))]
  "LVX_2"
  "eorq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

;; A scalar broadcast into a 128-bit vector in ONE instruction: SPLAT{B,H,W}Q
;; write the low element of a GPR across every lane of the pair.  This is the
;; canonical vec_duplicate-of-an-element RTL, so it matches whatever the middle
;; end produces for `v = {x,x,x,x}` and for the scalar operand of `v * s`.
;;
;; Before it, a splat went through lvx_expand_chunk_splat: materialise a magic
;; constant (LVX_SBMM8D_SPLATW0D), sbmm8d it against the value to fill a 64-bit
;; chunk, then *dup128 to copy the chunk into both halves -- three instructions
;; and a register, and in fact none of it, since UNSPEC_SBMM8D had no pattern
;; and every splat was an unrecognizable insn.
;;
;; V2DI and V2DF are deliberately absent: their duplicate is *dup128 below, a
;; single copyd that is `tiny` where splatdq is `lite`, so the ISA's D form
;; buys nothing here.
(define_mode_iterator SPLAT128 [V16QI V8HI V8HF V4SI V4SF])
(define_mode_iterator SPLAT256 [V32QI V16HI V16HF V8SI V8SF])
(define_mode_iterator SPLAT512 [V64QI V32HI V32HF V16SI V16SF])
(define_mode_attr splat [(V16QI "b") (V8HI "h") (V8HF "h") (V4SI "w") (V4SF "w")])

(define_insn "*splat128<mode>"
  [(set (match_operand:SPLAT128 0 "register_operand" "=r")
        (vec_duplicate:SPLAT128 (match_operand:<INNER> 1 "register_operand" "r")))]
  "LVX_2"
  "splat<splat>q %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

;; A 256-bit vector of sub-word lanes: SPLAT{B,H,W}Q fills the low pair, and
;; the high pair is a copy of it -- three syllables, the last two tiny.
;; Split after reload so the splat is the *splat128 above and the copy is
;; the pair move, and the bundler can place them; a single template would
;; put the copies in the splat's own bundle, where they read the pair's
;; old value.  This replaces the chunk route (*dup256 on a 64-bit vector
;; chunk), whose split needed a move in a 64-bit vector mode that has no
;; mov pattern: (v8si){k,k,k,k,k,k,k,k} was an unrecognizable insn.
;; The 512-bit form splats the low half and copies it up; its first insn
;; is a *splat256 and splits again.
(define_insn_and_split "*splat256<mode>"
  [(set (match_operand:SPLAT256 0 "register_operand" "=r")
        (vec_duplicate:SPLAT256 (match_operand:<INNER> 1 "register_operand" "r")))]
  "LVX_2"
  "#"
  "&& reload_completed"
  [(set (match_dup 2) (vec_duplicate:<HALF> (match_dup 1)))
   (set (match_dup 3) (match_dup 2))]
  {
    operands[2] = simplify_gen_subreg (<HALF>mode, operands[0], <MODE>mode, 0);
    operands[3] = simplify_gen_subreg (<HALF>mode, operands[0], <MODE>mode, 16);
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "12")]
)

(define_insn_and_split "*splat512<mode>"
  [(set (match_operand:SPLAT512 0 "register_operand" "=r")
        (vec_duplicate:SPLAT512 (match_operand:<INNER> 1 "register_operand" "r")))]
  "LVX_2"
  "#"
  "&& reload_completed"
  [(set (match_dup 2) (vec_duplicate:<HALF> (match_dup 1)))
   (set (match_dup 3) (match_dup 2))]
  {
    operands[2] = simplify_gen_subreg (<HALF>mode, operands[0], <MODE>mode, 0);
    operands[3] = simplify_gen_subreg (<HALF>mode, operands[0], <MODE>mode, 32);
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "28")]
)

;; A lane in memory splatted across a quad is one L{B,H,W,D}SO, whatever the
;; lane type: it loads and splats in the LSU, with no ALU op after the load.
;; This is the plain form; lvx_lso<mode> in builtin.md is the builtin's,
;; whose UNSPEC_LOAD carries the variant string and which generic RTL never
;; matches.  Reached from vec_duplicate<mode> with a memory source, and by
;; combine when a scalar load feeds a vec_duplicate.
(define_insn "*lso<mode>"
  [(set (match_operand:SIMD256 0 "register_operand" "=r,r,r")
        (vec_duplicate:SIMD256 (match_operand:<INNER> 1 "memory_operand" "a,b,m")))]
  "LVX_2"
  "l<lsplat>so%X1 %0 = %1"
  [(set_attr "type" "load")
   (set_attr_alternative "issue"
    [(const_string "lsu_auxw")
     (const_string "lsu_auxw_x")
     (const_string "lsu_auxw_x2")])
   (set_attr "length" "4, 8, 12")]
)

;; The same instruction for a 128-bit vector: L*SO writes a quad, and the
;; pair wanted is its low half.  The middle end cannot say so -- the lane is
;; loaded into a scalar and only that scalar reaches vec_duplicate -- but
;; combine can fuse the load into the splat given an insn with the pair as
;; destination, which is this one.  It exists only before allocation: the
;; split gives it a fresh quad to load into and copies the low pair out,
;; and the allocator, seeing a lowpart copy, puts the pair on the quad's
;; low half and the copy disappears (a copyd pair at worst).
(define_insn_and_split "*lso128<mode>"
  [(set (match_operand:SIMD128 0 "register_operand" "=r")
        (vec_duplicate:SIMD128 (match_operand:<INNER> 1 "memory_operand" "m")))]
  "LVX_2 && can_create_pseudo_p ()"
  "#"
  "&& 1"
  [(set (match_dup 2) (vec_duplicate:<QUAD> (match_dup 1)))
   (set (match_dup 0) (match_dup 3))]
  {
    operands[2] = gen_reg_rtx (<QUAD>mode);
    operands[3] = gen_lowpart (<MODE>mode, operands[2]);
  }
)

;; The lane shuffles of a quad, after ARM's UZP1/UZP2/ZIP1/ZIP2.
;; EVEN%1 = %2, %3: the even lanes of %2 then the even lanes of %3 (UZP1);
;; ODD the odd ones (UZP2).  ZIP%1 = %2, %3 interleaves the lanes of two
;; double words: ZIP1 on the low halves of two vectors, ZIP2 on the high
;; halves.  The permutation expander (lvx_expand_vec_perm_shuffle) matches
;; the selectors; vec_pack_trunc is EVEN, since a little-endian lane's low
;; half is its even half.
(define_mode_iterator SHUF128 [V16QI V8HI V4SI V2DI])
(define_mode_iterator ZIP128 [V16QI V8HI V4SI])

(define_insn "lvx_even<lane>q"
  [(set (match_operand:SHUF128 0 "register_operand" "=r")
        (unspec:SHUF128 [(match_operand:SHUF128 1 "register_operand" "r")
                         (match_operand:SHUF128 2 "register_operand" "r")] UNSPEC_EVEN))]
  "LVX_2"
  "even<lane>q %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "lvx_odd<lane>q"
  [(set (match_operand:SHUF128 0 "register_operand" "=r")
        (unspec:SHUF128 [(match_operand:SHUF128 1 "register_operand" "r")
                         (match_operand:SHUF128 2 "register_operand" "r")] UNSPEC_ODD))]
  "LVX_2"
  "odd<lane>q %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "lvx_zip<lane>dq"
  [(set (match_operand:ZIP128 0 "register_operand" "=r")
        (unspec:ZIP128 [(match_operand:DI 1 "register_operand" "r")
                        (match_operand:DI 2 "register_operand" "r")] UNSPEC_ZIP))]
  "LVX_2"
  "zip<lane>dq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

;; CATDQ: a quad from two double words, the vec_concat.
(define_insn "lvx_catdq"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (vec_concat:V2DI (match_operand:DI 1 "register_operand" "r")
                         (match_operand:DI 2 "register_operand" "r")))]
  "LVX_2"
  "catdq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

;; Narrowing pack: the low half of each lane, which is its even half.
(define_mode_iterator PACK128 [V8HI V4SI V2DI])
(define_mode_attr packed [(V8HI "V16QI") (V4SI "V8HI") (V2DI "V4SI")])
(define_mode_attr packedlane [(V8HI "b") (V4SI "h") (V2DI "w")])

(define_expand "vec_pack_trunc_<mode>"
  [(match_operand:<packed> 0 "register_operand")
   (match_operand:PACK128 1 "register_operand")
   (match_operand:PACK128 2 "register_operand")]
  "LVX_2"
  {
    emit_insn (gen_lvx_even<packedlane>q (operands[0],
					   gen_lowpart (<packed>mode, operands[1]),
					   gen_lowpart (<packed>mode, operands[2])));
    DONE;
  }
)

(define_insn_and_split "*dup128"
  [(set (match_operand:SIMD128 0 "register_operand" "=r")
        (vec_duplicate:SIMD128 (match_operand:<CHUNK> 1 "nonmemory_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 8) (match_dup 1))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny2")
   (set_attr "length"         "8")]
)

(define_insn_and_split "*dup256"
  [(set (match_operand:SIMD256 0 "register_operand" "=r")
        (vec_duplicate:SIMD256 (match_operand:<CHUNK> 1 "nonmemory_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<CHUNK> (match_dup 0) 0) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 8) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 16) (match_dup 1))
   (set (subreg:<CHUNK> (match_dup 0) 24) (match_dup 1))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny4")
   (set_attr "length"        "16")]
)

(define_insn_and_split "*dup512"
  [(set (match_operand:SIMD512 0 "register_operand" "=r")
        (vec_duplicate:SIMD512 (match_operand:<CHUNK> 1 "nonmemory_operand" "r")))]
  "LVX_2"
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
  "LVX_2"
  "iorq %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn_and_split "lvx_oroebt"
  [(set (match_operand:V64QI 0 "register_operand" "=r")
        (unspec:V64QI [(match_operand:V32HI 1 "register_operand" "r")
                       (match_operand:V32HI 2 "register_operand" "r")] UNSPEC_OROE))]
  "LVX_2"
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
  "LVX_2"
  "#"
  ""
  [(set (match_dup 0) (match_dup 2))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn_and_split "*qxo<hwidenx>_oroe<suffix>_1"
  [(set (match_operand:<HWIDE> 0 "register_operand" "=r")
        (unspec:<HWIDE> [(unspec:VXQI [(match_operand:<HWIDE> 1 "register_operand" "r")
                                       (match_operand:<HWIDE> 2 "register_operand" "r")] UNSPEC_OROE)] UNSPEC_QXO))]
  "LVX_2"
  "#"
  ""
  [(set (match_dup 0) (match_dup 1))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn_and_split "*zxo<hwidenx>_oroe<suffix>_1"
  [(set (match_operand:<HWIDE> 0 "register_operand" "=r")
        (unspec:<HWIDE> [(unspec:VXQI [(match_operand:<HWIDE> 1 "register_operand" "r")
                                       (match_operand:<HWIDE> 2 "register_operand" "r")] UNSPEC_OROE)] UNSPEC_ZXO))]
  "LVX_2"
  "#"
  ""
  [(set (match_dup 0)
        (unspec:<HWIDE> [(match_dup 1)] UNSPEC_ZXO))]
  {
    rtx op1 = gen_reg_rtx (<MODE>mode);
    emit_insn (gen_rtx_SET (op1, simplify_gen_subreg (<MODE>mode, operands[1], <HWIDE>mode, 0)));
    operands[1] = op1;
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

(define_insn_and_split "*qxe<hwidenx>_oroe<suffix>_2"
  [(set (match_operand:<HWIDE> 0 "register_operand" "=r")
        (unspec:<HWIDE> [(unspec:VXQI [(match_operand:<HWIDE> 1 "register_operand" "r")
                                       (match_operand:<HWIDE> 2 "register_operand" "r")] UNSPEC_OROE)] UNSPEC_QXE))]
  "LVX_2"
  "#"
  ""
  [(set (match_dup 0)
        (unspec:<HWIDE> [(match_dup 2)] UNSPEC_QXE))]
  {
    rtx op2 = gen_reg_rtx (<MODE>mode);
    emit_insn (gen_rtx_SET (op2, simplify_gen_subreg (<MODE>mode, operands[2], <HWIDE>mode, 0)));
    operands[2] = op2;
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "tiny")]
)

;; neg<m>2 ssneg<m>2 abs<m>2 ssabs<m>2

;; add<m>3 ssadd<m>3 usass<m>3 sub<m>3 sssub<m>3 ussub<m>3 smin<m>3 smax<m>3 umin<m>3 umax<m>3

;; ROL and ROR have no byte-lane form in the ISA -- the packed rotates are
;; ROLDP/ROLWQ and their RORs -- so a V16QI or V32QI rotate is still built
;; from half-word rotates the way this port has always done it.  The rest of
;; the VXQI synthesis went when the byte lanes joined the general iterators.

(define_expand "mulv64qi3"
  [(set (match_operand:V64QI 0 "register_operand" "")
        (mult:V64QI (match_operand:V64QI 1 "register_operand" "")
                    (match_operand:V64QI 2 "register_operand" "")))]
  "LVX_2"
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




;; abdu<m>3

;; avg<m>3_floor uavg<m>3_floor avg<m>3_ceil uavg<m>3_ceil

;; The byte averages the expander above falls through to.  S128I is the
;; non-byte 128-bit integer modes, so the AVG*BX forms need their own patterns;
;; <avgm>bx covers all four variants (avgbx, avgubx, avgrbx, avgrubx).  These
;; used to be a pair -- and at V32QI a quad -- of 64-bit AVG*BO over the
;; register halves, which the ISA no longer has.
(define_insn "*<avgpre>v16qi<avgpost>_2"
  [(set (match_operand:V16QI 0 "register_operand" "=r")
        (unspec:V16QI [(match_operand:V16QI 1 "register_operand" "r")
                       (match_operand:V16QI 2 "register_operand" "r")] UNSPEC_AVGI))]
  "LVX_2 && (HAVE_LVX_<AVGPRE>_V16QI)"
  "<avgm>bx %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length"      "4")]
)

(define_insn "*<avgpre>v32qi<avgpost>_2"
  [(set (match_operand:V32QI 0 "register_operand" "=r")
        (unspec:V32QI [(match_operand:V32QI 1 "register_operand" "r")
                       (match_operand:V32QI 2 "register_operand" "r")] UNSPEC_AVGI))]
  "LVX_2 && (HAVE_LVX_<AVGPRE>_V32QI)"
  {
    return "<avgm>bx %L0 = %L1, %L2\n\t<avgm>bx %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length"         "8")]
)

;; ashl<m>3 ssashl<m>3 usashl<m>3
(define_insn_and_split "*<prefix>v32qi3_2"
  [(set (match_operand:V32QI 0 "register_operand" "=&r,r")
        (BINSHLRL:V32QI (match_operand:V32QI 1 "register_operand" "r,r")
                        (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  "LVX_2 && (HAVE_LVX_<binshlrl>_V32QI)"
  "#"
  "HAVE_LVX_<binshlrl>_V32QI && reload_completed"
  [(set (subreg:V16QI (match_dup 0) 0)
        (BINSHLRL:V16QI (subreg:V16QI (match_dup 1) 0)
                        (match_dup 2)))
   (set (subreg:V16QI (match_dup 0) 16)
        (BINSHLRL:V16QI (subreg:V16QI (match_dup 1) 16)
                        (match_dup 2)))]
  ""
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "lite2, lite2")]
)

;; lshr<m>3 ashr<m>3

(define_insn_and_split "*sshrv32qi_2"
  [(set (match_operand:V32QI 0 "register_operand" "=&r,r")
        (unspec:V32QI [(match_operand:V32QI 1 "register_operand" "r,r")
                       (match_operand:SI 2 "reg_shift_operand" "r,U06")] UNSPEC_SRS))]
  "LVX_2 && (HAVE_LVX_SSHR_V32QI)"
  "#"
  "HAVE_LVX_SSHR_V32QI && reload_completed"
  [(set (subreg:V16QI (match_dup 0) 0)
        (unspec:V16QI [(subreg:V16QI (match_dup 1) 0)
                       (match_dup 2)] UNSPEC_SRS))
   (set (subreg:V16QI (match_dup 0) 16)
        (unspec:V16QI [(subreg:V16QI (match_dup 1) 16)
                       (match_dup 2)] UNSPEC_SRS))]
  ""
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "lite2, lite2")]
)



;; Every 128-bit lane shape has a native rotate: ROLBX/RORBX and ROLHO/RORHO
;; joined ROLWQ/RORWQ and ROLDP/RORDP in the LVX-2 ISA, so V16QI no longer goes
;; through the VXQI half-word synthesis and V8HI is no longer lowered to
;; shift-left / shift-right / or.

(define_insn "rot<rotm><mode>3"
  [(set (match_operand:V128R 0 "register_operand" "=r")
        (ROTCODE:V128R (match_operand:V128R 1 "register_operand" "r")
                       (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "ro<rotm><suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

;; There is deliberately no vrotl/vrotr here, and that is not an omission.  The
;; two optabs are different contracts: rotl<mode>3 takes a SCALAR count (one
;; count for every lane), vrotl<mode>3 takes a VECTOR of per-lane counts
;; (md.texi: "take vectors as operand 2 instead of a scalar type").  The LVX
;; rotates are the scalar kind -- ALU_BXSWRR and its siblings give registerY as
;; a singleReg -- and so is every packed shift, so there is nothing to lower a
;; per-lane rotate to.
;;
;; A vrot<rotm><mode>3 expander used to sit here, declaring operand 2 as SI
;; while claiming the vector-count optab.  It made the vectorizer commit to a
;; per-lane rotate it then could not expand, and the operand-mode mismatch
;; ICEd in convert_move (expr.cc:305, through expand_variable_shift ->
;; expand_binop -> convert_modes).  Constant-count loops never needed it: an
;; invariant count reaches the scalar optab above by itself, exactly as
;; `a[i] << 3` reaches SLLBX, and a genuinely per-lane count now simply does
;; not vectorize -- the same answer the shifts already give.
;; `validation/tests/micro/rotlane-var.c` is the regression test.

;; The same at 256 bits, one native rotate per 128-bit half.
(define_insn "rot<rotm><mode>3"
  [(set (match_operand:V256R 0 "register_operand" "=r")
        (ROTCODE:V256R (match_operand:V256R 1 "register_operand" "r")
                       (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  {
    return "ro<rotm><hsuffix> %L0 = %L1, %2\n\tro<rotm><hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

;; S128I (V8HI V4SI)

(define_insn "ashl<mode>3"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (ashift:S128I (match_operand:S128I 1 "register_operand" "r")
                      (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "sll<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "ssashl<mode>3"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (ss_ashift:S128I (match_operand:S128I 1 "register_operand" "r")
                         (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "sls<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_expand "usashl<mode>3"
  [(match_operand:S128I 0 "register_operand" "")
   (match_operand:S128I 1 "register_operand" "")
   (match_operand:SI 2 "reg_shift_operand" "")]
  "LVX_2"
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
  "LVX_2 && (!HAVE_LVX_US_ASHIFT_<MODE>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "ashr<mode>3"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (ashiftrt:S128I (match_operand:S128I 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "sra<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "lshr<mode>3"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (lshiftrt:S128I (match_operand:S128I 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "srl<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "sshr<mode>3"
  [(set (match_operand:S128I 0 "register_operand" "=r")
        (unspec:S128I [(match_operand:S128I 1 "register_operand" "r")
                       (match_operand:SI 2 "reg_shift_operand" "rU06")] UNSPEC_SRS))]
  "LVX_2"
  "srs<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "avg<mode>3_floor"
  [(set (match_operand:S128I 0 "register_operand" "=r,r")
        (unspec:S128I [(match_operand:S128I 1 "register_operand" "r,r")
                       (match_operand:S128I 2 "reg_or_splat32_operand" "r,SXW")] UNSPEC_AVG))]
  "LVX_2"
  "@
   avg<suffix> %0 = %1, %2
   avg<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "avg<mode>3_ceil"
  [(set (match_operand:S128I 0 "register_operand" "=r,r")
        (unspec:S128I [(match_operand:S128I 1 "register_operand" "r,r")
                       (match_operand:S128I 2 "reg_or_splat32_operand" "r,SXW")] UNSPEC_AVGR))]
  "LVX_2"
  "@
   avgr<suffix> %0 = %1, %2
   avgr<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "uavg<mode>3_floor"
  [(set (match_operand:S128I 0 "register_operand" "=r,r")
        (unspec:S128I [(match_operand:S128I 1 "register_operand" "r,r")
                       (match_operand:S128I 2 "reg_or_splat32_operand" "r,SXW")] UNSPEC_AVGU))]
  "LVX_2"
  "@
   avgu<suffix> %0 = %1, %2
   avgu<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "uavg<mode>3_ceil"
  [(set (match_operand:S128I 0 "register_operand" "=r,r")
        (unspec:S128I [(match_operand:S128I 1 "register_operand" "r,r")
                       (match_operand:S128I 2 "reg_or_splat32_operand" "r,SXW")] UNSPEC_AVGRU))]
  "LVX_2"
  "@
   avgru<suffix> %0 = %1, %2
   avgru<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_expand "extend<mode><wide>2"
  [(set (match_operand:<WIDE> 0 "register_operand" "")
        (sign_extend:<WIDE> (match_operand:S128L 1 "register_operand" "")))]
  "LVX_2"
  {
    emit_insn (gen_lvx_sx<widenx> (operands[0], operands[1]));
    DONE;
  }
)

(define_expand "zero_extend<mode><wide>2"
  [(set (match_operand:<WIDE> 0 "register_operand" "")
        (zero_extend:<WIDE> (match_operand:S128L 1 "register_operand" "")))]
  "LVX_2"
  {
    emit_insn (gen_lvx_zx<widenx> (operands[0], operands[1]));
    DONE;
  }
)


;; V128I -- the multiply is native at every 128-bit lane shape: MULBX, MULHO,
;; MULWQ, MULDP.

(define_insn "mul<mode>3"
  [(set (match_operand:V128I 0 "register_operand" "=r,r")
        (mult:V128I (match_operand:V128I 1 "register_operand" "r,r")
                    (match_operand:V128I 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   mul<suffix> %0 = %1, %2
   mul<suffix> %0 = %1, %W2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

;; ... and the negating twin at every lane shape: MULNBX, MULNHO, MULNWQ,
;; MULNDP, so `-(a * b)` on a vector is one instruction instead of a multiply
;; and a NEG*.  Written as (mult (neg A) B) because that is the canonical form
;; -- md.texi moves a neg inside a mult -- and register-register only, since a
;; constant operand absorbs the negation itself (`-(a * K)` becomes `a * -K`)
;; and reaches the plain MUL<lane> with the splat of -K.
(define_insn "mulneg<mode>3"
  [(set (match_operand:V128I 0 "register_operand" "=r")
        (mult:V128I (neg:V128I (match_operand:V128I 1 "register_operand" "r"))
                    (match_operand:V128I 2 "register_operand" "r")))]
  "LVX_2"
  "muln<suffix> %0 = %1, %2"
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)


;; V128J (V8HI V4SI V2DI)

;; The .M formats take one 32-bit word and the machine replicates it over the
;; whole 128-bit operand, so a vector constant that is that word four times --
;; which any uniform byte, half-word or word constant is -- needs no register.
(define_insn "add<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r,r")
        (plus:V128J (match_operand:V128J 1 "register_operand" "r,r")
                    (match_operand:V128J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   add<suffix> %0 = %1, %2
   add<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*add<mode>3_s1"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (plus:V128JB (vec_duplicate:V128JB (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128JB 2 "register_operand" "r")))]
  "LVX_2"
  "add<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*add<mode>3_s2"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (plus:V128JB (match_operand:V128JB 1 "register_operand" "r")
                    (vec_duplicate:V128JB (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "add<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "ssadd<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r,r")
        (ss_plus:V128J (match_operand:V128J 1 "register_operand" "r,r")
                       (match_operand:V128J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   adds<suffix> %0 = %1, %2
   adds<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*ssadd<mode>3_s1"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (ss_plus:V128JB (vec_duplicate:V128JB (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                       (match_operand:V128JB 2 "register_operand" "r")))]
  "LVX_2"
  "adds<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*ssadd<mode>3_s2"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (ss_plus:V128JB (match_operand:V128JB 1 "register_operand" "r")
                       (vec_duplicate:V128JB (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "adds<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_expand "usadd<mode>3"
  [(match_operand:V128J 0 "register_operand" "")
   (match_operand:V128J 1 "register_operand" "")
   (match_operand:V128J 2 "register_operand" "")]
  "LVX_2"
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
  "LVX_2 && (!HAVE_LVX_US_PLUS_<MODE>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "*usadd<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=&r")
        (us_plus:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                       (match_operand:V128J 2 "register_operand" "r")))
   (clobber (match_scratch:V128J 3 "=&r"))
   (clobber (match_scratch:V128J 4 "=&r"))]
  "LVX_2 && (!HAVE_LVX_US_PLUS_<MODE>)"
  "#"
  "!HAVE_LVX_US_PLUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (plus:V128J (vec_duplicate:V128J (match_dup 1)) (match_dup 2)))
   (set (match_dup 4)
        (ltu:V128J (match_dup 3) (match_dup 2)))
   (set (match_dup 0)
        (ior:V128J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "*usadd<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=&r")
        (us_plus:V128J (match_operand:V128J 1 "register_operand" "r")
                       (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))
   (clobber (match_scratch:V128J 3 "=&r"))
   (clobber (match_scratch:V128J 4 "=&r"))]
  "LVX_2 && (!HAVE_LVX_US_PLUS_<MODE>)"
  "#"
  "!HAVE_LVX_US_PLUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (plus:V128J (match_dup 1) (vec_duplicate:V128J (match_dup 2))))
   (set (match_dup 4)
        (ltu:V128J (match_dup 3) (match_dup 1)))
   (set (match_dup 0)
        (ior:V128J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "usadd<mode>3_2"
  [(set (match_operand:V128J 0 "register_operand" "=r,r")
        (us_plus:V128J (match_operand:V128J 1 "register_operand" "r,r")
                       (match_operand:V128J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2 && (HAVE_LVX_US_PLUS_<MODE>)"
  "@
   addus<suffix> %0 = %1, %2
   addus<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*usadd<mode>3_s1"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (us_plus:V128JB (vec_duplicate:V128JB (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                       (match_operand:V128JB 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_US_PLUS_<MODE>)"
  "addus<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*usadd<mode>3_s2"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (us_plus:V128JB (match_operand:V128JB 1 "register_operand" "r")
                       (vec_duplicate:V128JB (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_US_PLUS_<MODE>)"
  "addus<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*addx2<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r,r")
        (plus:V128K (ashift:V128K (match_operand:V128K 1 "register_operand" "r,r")
                                  (const_int 1))
                    (match_operand:V128K 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2 && (HAVE_LVX_MUL02_ADD_<MODE>)"
  "@
   addx2<suffix> %0 = %1, %2
   addx2<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*addx4<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r,r")
        (plus:V128K (ashift:V128K (match_operand:V128K 1 "register_operand" "r,r")
                                  (const_int 2))
                    (match_operand:V128K 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2 && (HAVE_LVX_MUL04_ADD_<MODE>)"
  "@
   addx4<suffix> %0 = %1, %2
   addx4<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*addx8<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r,r")
        (plus:V128K (ashift:V128K (match_operand:V128K 1 "register_operand" "r,r")
                                  (const_int 3))
                    (match_operand:V128K 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2 && (HAVE_LVX_MUL08_ADD_<MODE>)"
  "@
   addx8<suffix> %0 = %1, %2
   addx8<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*addx16<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r,r")
        (plus:V128K (ashift:V128K (match_operand:V128K 1 "register_operand" "r,r")
                                  (const_int 4))
                    (match_operand:V128K 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2 && (HAVE_LVX_MUL16_ADD_<MODE>)"
  "@
   addx16<suffix> %0 = %1, %2
   addx16<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "sub<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r,r")
        (minus:V128J (match_operand:V128J 1 "reg_or_splat32_operand" "r,SXW")
                     (match_operand:V128J 2 "register_operand" "r,r")))]
  "LVX_2"
  "@
   sbf<suffix> %0 = %2, %1
   sbf<suffix> %0 = %2, %W1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*sub<mode>3_s1"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (minus:V128JB (vec_duplicate:V128JB (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                     (match_operand:V128JB 2 "register_operand" "r")))]
  "LVX_2"
  "sbf<suffix> %0 = %2, %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*sub<mode>3_s2"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (minus:V128JB (match_operand:V128JB 1 "register_operand" "r")
                     (vec_duplicate:V128JB (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "sbf<suffix> %0 = %2, %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "sssub<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r,r")
        (ss_minus:V128J (match_operand:V128J 1 "reg_or_splat32_operand" "r,SXW")
                        (match_operand:V128J 2 "register_operand" "r,r")))]
  "LVX_2"
  "@
   sbfs<suffix> %0 = %2, %1
   sbfs<suffix> %0 = %2, %W1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*sssub<mode>3_s1"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (ss_minus:V128JB (vec_duplicate:V128JB (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                        (match_operand:V128JB 2 "register_operand" "r")))]
  "LVX_2"
  "sbfs<suffix> %0 = %2, %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*sssub<mode>3_s2"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (ss_minus:V128JB (match_operand:V128JB 1 "register_operand" "r")
                        (vec_duplicate:V128JB (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "sbfs<suffix> %0 = %2, %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_expand "ussub<mode>3"
  [(match_operand:V128J 0 "register_operand" "")
   (match_operand:V128J 1 "register_operand" "")
   (match_operand:V128J 2 "register_operand" "")]
  "LVX_2"
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
  "LVX_2 && (!HAVE_LVX_US_MINUS_<MODE>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "*ussub<mode>3_s1"
  [(set (match_operand:V128J 0 "register_operand" "=&r")
        (us_minus:V128J (vec_duplicate:V128J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                        (match_operand:V128J 2 "register_operand" "r")))
   (clobber (match_scratch:V128J 3 "=&r"))
   (clobber (match_scratch:V128J 4 "=&r"))]
  "LVX_2 && (!HAVE_LVX_US_MINUS_<MODE>)"
  "#"
  "!HAVE_LVX_US_MINUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (minus:V128J (vec_duplicate:V128J (match_dup 1)) (match_dup 2)))
   (set (match_dup 4)
        (geu:V128J (vec_duplicate:V128J (match_dup 1)) (match_dup 3)))
   (set (match_dup 0)
        (and:V128J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "*ussub<mode>3_s2"
  [(set (match_operand:V128J 0 "register_operand" "=&r")
        (us_minus:V128J (match_operand:V128J 1 "register_operand" "r")
                        (vec_duplicate:V128J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))
   (clobber (match_scratch:V128J 3 "=&r"))
   (clobber (match_scratch:V128J 4 "=&r"))]
  "LVX_2 && (!HAVE_LVX_US_MINUS_<MODE>)"
  "#"
  "!HAVE_LVX_US_MINUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (minus:V128J (match_dup 1) (vec_duplicate:V128J (match_dup 2))))
   (set (match_dup 4)
        (leu:V128J (match_dup 3) (match_dup 1)))
   (set (match_dup 0)
        (and:V128J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "ussub<mode>3_2"
  [(set (match_operand:V128J 0 "register_operand" "=r,r")
        (us_minus:V128J (match_operand:V128J 1 "reg_or_splat32_operand" "r,SXW")
                        (match_operand:V128J 2 "register_operand" "r,r")))]
  "LVX_2 && (HAVE_LVX_US_MINUS_<MODE>)"
  "@
   sbfus<suffix> %0 = %2, %1
   sbfus<suffix> %0 = %2, %W1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*ussub<mode>3_s1"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (us_minus:V128JB (vec_duplicate:V128JB (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                        (match_operand:V128JB 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_US_MINUS_<MODE>)"
  "sbfus<suffix> %0 = %2, %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*ussub<mode>3_s2"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (us_minus:V128JB (match_operand:V128JB 1 "register_operand" "r")
                        (vec_duplicate:V128JB (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_US_MINUS_<MODE>)"
  "sbfus<suffix> %0 = %2, %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*sbfx2<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r,r")
        (minus:V128K (match_operand:V128K 1 "reg_or_splat32_operand" "r,SXW")
                     (ashift:V128K (match_operand:V128K 2 "register_operand" "r,r")
                                   (const_int 1))))]
  "LVX_2 && (HAVE_LVX_MUL02_SUB_<MODE>)"
  "@
   sbfx2<suffix> %0 = %2, %1
   sbfx2<suffix> %0 = %2, %W1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*sbfx4<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r,r")
        (minus:V128K (match_operand:V128K 1 "reg_or_splat32_operand" "r,SXW")
                     (ashift:V128K (match_operand:V128K 2 "register_operand" "r,r")
                                   (const_int 2))))]
  "LVX_2 && (HAVE_LVX_MUL04_SUB_<MODE>)"
  "@
   sbfx4<suffix> %0 = %2, %1
   sbfx4<suffix> %0 = %2, %W1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*sbfx8<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r,r")
        (minus:V128K (match_operand:V128K 1 "reg_or_splat32_operand" "r,SXW")
                     (ashift:V128K (match_operand:V128K 2 "register_operand" "r,r")
                                   (const_int 3))))]
  "LVX_2 && (HAVE_LVX_MUL08_SUB_<MODE>)"
  "@
   sbfx8<suffix> %0 = %2, %1
   sbfx8<suffix> %0 = %2, %W1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*sbfx16<suffix>"
  [(set (match_operand:V128K 0 "register_operand" "=r,r")
        (minus:V128K (match_operand:V128K 1 "reg_or_splat32_operand" "r,SXW")
                     (ashift:V128K (match_operand:V128K 2 "register_operand" "r,r")
                                   (const_int 4))))]
  "LVX_2 && (HAVE_LVX_MUL16_SUB_<MODE>)"
  "@
   sbfx16<suffix> %0 = %2, %1
   sbfx16<suffix> %0 = %2, %W1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_expand "div<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "")
        (div:V128J (match_operand:V128J 1 "register_operand" "")
                   (match_operand:V128J 2 "register_operand" "")))]
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  [(set (match_operand:V128J 0 "register_operand" "=r,r")
        (smin:V128J (match_operand:V128J 1 "register_operand" "r,r")
                    (match_operand:V128J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   min<suffix> %0 = %1, %2
   min<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*smin<mode>3_s1"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (smin:V128JB (vec_duplicate:V128JB (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128JB 2 "register_operand" "r")))]
  "LVX_2"
  "min<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*smin<mode>3_s2"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (smin:V128JB (match_operand:V128JB 1 "register_operand" "r")
                    (vec_duplicate:V128JB (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "min<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "smax<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r,r")
        (smax:V128J (match_operand:V128J 1 "register_operand" "r,r")
                    (match_operand:V128J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   max<suffix> %0 = %1, %2
   max<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*smax<mode>3_s1"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (smax:V128JB (vec_duplicate:V128JB (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128JB 2 "register_operand" "r")))]
  "LVX_2"
  "max<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*smax<mode>3_s2"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (smax:V128JB (match_operand:V128JB 1 "register_operand" "r")
                    (vec_duplicate:V128JB (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "max<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "umin<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r,r")
        (umin:V128J (match_operand:V128J 1 "register_operand" "r,r")
                    (match_operand:V128J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   minu<suffix> %0 = %1, %2
   minu<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*umin<mode>3_s1"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (umin:V128JB (vec_duplicate:V128JB (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128JB 2 "register_operand" "r")))]
  "LVX_2"
  "minu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*umin<mode>3_s2"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (umin:V128JB (match_operand:V128JB 1 "register_operand" "r")
                    (vec_duplicate:V128JB (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "minu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "umax<mode>3"
  [(set (match_operand:V128J 0 "register_operand" "=r,r")
        (umax:V128J (match_operand:V128J 1 "register_operand" "r,r")
                    (match_operand:V128J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   maxu<suffix> %0 = %1, %2
   maxu<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*umax<mode>3_s1"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (umax:V128JB (vec_duplicate:V128JB (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128JB 2 "register_operand" "r")))]
  "LVX_2"
  "maxu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*umax<mode>3_s2"
  [(set (match_operand:V128JB 0 "register_operand" "=r")
        (umax:V128JB (match_operand:V128JB 1 "register_operand" "r")
                    (vec_duplicate:V128JB (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "maxu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "madd<mode><mode>4"
  [(set (match_operand:V128M 0 "register_operand" "=r,r")
        (plus:V128M (mult:V128M (match_operand:V128M 1 "register_operand" "r,r")
                                (match_operand:V128M 2 "reg_or_splat32_operand" "r,SXW"))
                    (match_operand:V128M 3 "register_operand" "0,0")))]
  "LVX_2"
  "@
   madd<suffix> %0 = %1, %2
   madd<suffix> %0 = %1, %W2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "msub<mode><mode>4"
  [(set (match_operand:V128M 0 "register_operand" "=r,r")
        (minus:V128M (match_operand:V128M 3 "register_operand" "0,0")
                     (mult:V128M (match_operand:V128M 1 "register_operand" "r,r")
                                 (match_operand:V128M 2 "reg_or_splat32_operand" "r,SXW"))))]
  "LVX_2"
  "@
   msbf<suffix> %0 = %1, %2
   msbf<suffix> %0 = %1, %W2"
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)



;; V128J (V8HI V4SI V2DI)

(define_insn "neg<mode>2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (neg:V128J (match_operand:V128J 1 "register_operand" "r")))]
  "LVX_2"
  "neg<suffix> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "ssneg<mode>2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (ss_neg:V128J (match_operand:V128J 1 "register_operand" "r")))]
  "LVX_2"
  "sbfs<suffix> %0 = %1, 0"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "abs<mode>2"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (abs:V128J (match_operand:V128J 1 "register_operand" "r")))]
  "LVX_2"
  "abs<suffix> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_expand "ssabs<mode>2"
  [(set (match_operand:V128J 0 "register_operand" "")
        (ss_abs:V128J (match_operand:V128J 1 "register_operand" "")))]
  "LVX_2"
  ""
)

(define_insn_and_split "ssabs<mode>2_1"
  [(set (match_operand:V128J 0 "register_operand" "=r")
        (ss_abs:V128J (match_operand:V128J 1 "register_operand" "r")))]
  "LVX_2 && (!HAVE_LVX_SS_ABS_<MODE>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_insn "clrsb<mode>2"
  [(set (match_operand:V128CZ 0 "register_operand" "=r")
        (clrsb:V128CZ (match_operand:V128CZ 1 "register_operand" "r")))]
  "LVX_2"
  "cls<suffix> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "clz<mode>2"
  [(set (match_operand:V128CZ 0 "register_operand" "=r")
        (clz:V128CZ (match_operand:V128CZ 1 "register_operand" "r")))]
  "LVX_2"
  "clz<suffix> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "ctz<mode>2"
  [(set (match_operand:V128CZ 0 "register_operand" "=r")
        (ctz:V128CZ (match_operand:V128CZ 1 "register_operand" "r")))]
  "LVX_2"
  "ctz<suffix> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "popcount<mode>2"
  [(set (match_operand:V128CZ 0 "register_operand" "=r")
        (popcount:V128CZ (match_operand:V128CZ 1 "register_operand" "r")))]
  "LVX_2"
  "cbs<suffix> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)


;; V128L

(define_insn "and<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r,r")
        (and:V128L (match_operand:V128L 1 "register_operand" "r,r")
                   (match_operand:V128L 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   andq %0 = %1, %2
   andq %0 = %1, %W2.@"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*nand<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r,r")
        (ior:V128L (not:V128L (match_operand:V128L 1 "register_operand" "r,r"))
                   (not:V128L (match_operand:V128L 2 "reg_or_splat32_operand" "r,SXW"))))]
  "LVX_2"
  "@
   nandq %0 = %1, %2
   nandq %0 = %1, %W2.@"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*andn<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r,r")
        (and:V128L (not:V128L (match_operand:V128L 1 "register_operand" "r,r"))
                   (match_operand:V128L 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   andnq %0 = %1, %2
   andnq %0 = %1, %W2.@"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

;; Bitwise select over a full-width mask -- one BSELQ where the lowering is
;; eorq + andq + eorq.  See *bselq in scalar.md for why the RTL is the XOR form
;; and why the base is a match_dup with a "0" constraint.  This is the lane
;; select lvx-1 lacks in the ISA sense, but GCC cannot reach it there: vector
;; modes exist only under LVX_2 (lvx_vector_mode_supported_p), so the gate is
;; required and the lvx-1 path is the TImode pattern.
(define_insn "*bsel<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (xor:V128L
          (and:V128L (xor:V128L (match_operand:V128L 1 "register_operand" "0")
                                (match_operand:V128L 2 "register_operand" "r"))
                     (match_operand:V128L 3 "register_operand" "r"))
          (match_dup 1)))]
  "LVX_2"
  "bselq %0 = %2, %3"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*bsel<mode>3_v"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (xor:V128L
          (and:V128L (xor:V128L (match_operand:V128L 2 "register_operand" "r")
                                (match_operand:V128L 1 "register_operand" "0"))
                     (match_operand:V128L 3 "register_operand" "r"))
          (match_dup 1)))]
  "LVX_2"
  "bselq %0 = %2, %3"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "ior<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r,r")
        (ior:V128L (match_operand:V128L 1 "register_operand" "r,r")
                   (match_operand:V128L 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   iorq %0 = %1, %2
   iorq %0 = %1, %W2.@"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*nior<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r,r")
        (and:V128L (not:V128L (match_operand:V128L 1 "register_operand" "r,r"))
                   (not:V128L (match_operand:V128L 2 "reg_or_splat32_operand" "r,SXW"))))]
  "LVX_2"
  "@
   niorq %0 = %1, %2
   niorq %0 = %1, %W2.@"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*iorn<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r,r")
        (ior:V128L (not:V128L (match_operand:V128L 1 "register_operand" "r,r"))
                   (match_operand:V128L 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   iornq %0 = %1, %2
   iornq %0 = %1, %W2.@"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "xor<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r,r")
        (xor:V128L (match_operand:V128L 1 "register_operand" "r,r")
                   (match_operand:V128L 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   eorq %0 = %1, %2
   eorq %0 = %1, %W2.@"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*nxor<mode>3"
  [(set (match_operand:V128L 0 "register_operand" "=r,r")
        (not:V128L (xor:V128L (match_operand:V128L 1 "register_operand" "r,r")
                              (match_operand:V128L 2 "reg_or_splat32_operand" "r,SXW"))))]
  "LVX_2"
  "@
   neorq %0 = %1, %2
   neorq %0 = %1, %W2.@"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "one_cmpl<mode>2"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (not:V128L (match_operand:V128L 1 "register_operand" "r")))]
  "LVX_2"
  "notq %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

;; fixme: same, should not exist
(define_expand "abd<mode>3"
  [(match_operand:V128L 0 "register_operand" "")
   (match_operand:V128L 1 "register_operand" "")
   (match_operand:V128L 2 "register_operand" "")]
  "LVX_2"
  {
    if (!HAVE_LVX_ABD_<MODE>)
      emit_insn (gen_abd<mode>3_1 (operands[0], operands[1], operands[2]));
    else
      emit_insn (gen_abd<mode>3_2 (operands[0], operands[1], operands[2]));
    DONE;
  }
)

(define_insn "abd<mode>3_1"
  [(set (match_operand:V128J 0 "register_operand" "=r,r")
        (minus:V128J (smax:V128J (match_operand:V128J 1 "register_operand" "r,r")
                                 (match_operand:V128J 2 "reg_or_splat32_operand" "r,SXW"))
                     (smin:V128J (match_dup 1) (match_dup 2))))]
  "LVX_2"
  "@
   abd<suffix> %0 = %1, %2
   abd<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "abd<mode>3_2"
  [(set (match_operand:V128L 0 "register_operand" "=r,r")
        (minus:V128L (smax:V128L (match_operand:V128L 1 "register_operand" "r,r")
                                 (match_operand:V128L 2 "reg_or_splat32_operand" "r,SXW"))
                     (smin:V128L (match_dup 1) (match_dup 2))))]
  "LVX_2"
  "@
   abd<suffix> %0 = %1, %2
   abd<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*abd<suffix>_s1"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (minus:V128L (smax:V128L (vec_duplicate:V128L (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                                 (match_operand:V128L 2 "register_operand" "r"))
                     (smin:V128L (vec_duplicate:V128L (match_dup 1)) (match_dup 2))))]
  "LVX_2"
  "abd<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*abd<suffix>_s2"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (minus:V128L (smax:V128L (match_operand:V128L 1 "register_operand" "r")
                                 (vec_duplicate:V128L (match_operand:<CHUNK> 2 "nonmemory_operand" "r")))
                     (smin:V128L (match_dup 1) (vec_duplicate:V128L (match_dup 2)))))]
  "LVX_2"
  "abd<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_expand "abds<mode>3"
  [(match_operand:V128L 0 "register_operand" "")
   (match_operand:V128L 1 "register_operand" "")
   (match_operand:V128L 2 "register_operand" "")]
  "LVX_2"
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
  "LVX_2 && (!HAVE_LVX_SS_ABD_<MODE>)"
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
  [(set (match_operand:V128L 0 "register_operand" "=r,r")
        (ss_minus:V128L (smax:V128L (match_operand:V128L 1 "register_operand" "r,r")
                                    (match_operand:V128L 2 "reg_or_splat32_operand" "r,SXW"))
                        (smin:V128L (match_dup 1) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_SS_ABD_<MODE>)"
  "@
   abds<suffix> %0 = %1, %2
   abds<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*abds<suffix>_s1"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (ss_minus:V128L (smax:V128L (vec_duplicate:V128L (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                                    (match_operand:V128L 2 "register_operand" "r"))
                        (smin:V128L (vec_duplicate:V128L (match_dup 1)) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_SS_ABD_<MODE>)"
  "abds<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*abds<suffix>_s2"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (ss_minus:V128L (smax:V128L (match_operand:V128L 1 "register_operand" "r")
                                    (vec_duplicate:V128L (match_operand:<CHUNK> 2 "nonmemory_operand" "r")))
                        (smin:V128L (match_dup 1) (vec_duplicate:V128L (match_dup 2)))))]
  "LVX_2 && (HAVE_LVX_SS_ABD_<MODE>)"
  "abds<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_expand "abdu<mode>3"
  [(match_operand:V128L 0 "register_operand" "")
   (match_operand:V128L 1 "register_operand" "")
   (match_operand:V128L 2 "register_operand" "")]
  "LVX_2"
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
  "LVX_2 && (!HAVE_LVX_UABD_<MODE>)"
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
  [(set (match_operand:V128L 0 "register_operand" "=r,r")
        (minus:V128L (umax:V128L (match_operand:V128L 1 "register_operand" "r,r")
                                 (match_operand:V128L 2 "reg_or_splat32_operand" "r,SXW"))
                     (umin:V128L (match_dup 1) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_UABD_<MODE>)"
  "@
   abdu<suffix> %0 = %1, %2
   abdu<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*abdu<suffix>_s1"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (minus:V128L (umax:V128L (vec_duplicate:V128L (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                                 (match_operand:V128L 2 "register_operand" "r"))
                     (umin:V128L (vec_duplicate:V128L (match_dup 1)) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_UABD_<MODE>)"
  "abdu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*abdu<suffix>_s2"
  [(set (match_operand:V128L 0 "register_operand" "=r")
        (minus:V128L (umax:V128L (match_operand:V128L 1 "register_operand" "r")
                                 (vec_duplicate:V128L (match_operand:<CHUNK> 2 "nonmemory_operand" "r")))
                     (umin:V128L (match_dup 1) (vec_duplicate:V128L (match_dup 2)))))]
  "LVX_2 && (HAVE_LVX_UABD_<MODE>)"
  "abdu<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)


;; V4SI

;; V2DI

(define_insn "ashlv2di3"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (ashift:V2DI (match_operand:V2DI 1 "register_operand" "r")
                     (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "slldp %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "ssashlv2di3"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (ss_ashift:V2DI (match_operand:V2DI 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "slsdp %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_expand "usashlv2di3"
  [(match_operand:V2DI 0 "register_operand" "")
   (match_operand:V2DI 1 "register_operand" "")
   (match_operand:SI 2 "reg_shift_operand" "")]
  "LVX_2"
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
  "LVX_2 && (!HAVE_LVX_US_ASHIFT_V2DI)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "ashrv2di3"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (ashiftrt:V2DI (match_operand:V2DI 1 "register_operand" "r")
                       (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "sradp %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "lshrv2di3"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (lshiftrt:V2DI (match_operand:V2DI 1 "register_operand" "r")
                       (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  "srldp %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "sshrv2di3"
  [(set (match_operand:V2DI 0 "register_operand" "=r")
        (unspec:V2DI [(match_operand:V2DI 1 "register_operand" "r")
                      (match_operand:SI 2 "reg_shift_operand" "rU06")] UNSPEC_SRS))]
  "LVX_2"
  "srsdp %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)


;; S256I (V16HI V8SI)

(define_expand "ashl<mode>3"
  [(set (match_operand:S256I 0 "register_operand" "")
        (ashift:S256I (match_operand:S256I 1 "register_operand" "")
                      (match_operand:SI 2 "reg_shift_operand" "")))]
  "LVX_2"
  ""
)

(define_insn_and_split "ashl<mode>3_1"
  [(set (match_operand:S256I 0 "register_operand" "=&r,r")
        (ashift:S256I (match_operand:S256I 1 "register_operand" "r,r")
                      (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  "LVX_2 && (!HAVE_LVX_ASHIFT_<MODE> && HAVE_LVX_ASHIFT_<HALF>)"
  "#"
  "!HAVE_LVX_ASHIFT_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ashift:<HALF> (subreg:<HALF> (match_dup 1) 0)
                       (match_dup 2)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ashift:<HALF> (subreg:<HALF> (match_dup 1) 16)
                       (match_dup 2)))]
  ""
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "lite2, lite2")]
)

(define_insn "ashl<mode>3_2"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (ashift:S256I (match_operand:S256I 1 "register_operand" "r")
                      (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2 && (HAVE_LVX_ASHIFT_<MODE>)"
  {
    return "sll<hsuffix> %L0 = %L1, %2\n\tsll<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn_and_split "ssashl<mode>3"
  [(set (match_operand:S256I 0 "register_operand" "=&r,r")
        (ss_ashift:S256I (match_operand:S256I 1 "register_operand" "r,r")
                         (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_ashift:<HALF> (subreg:<HALF> (match_dup 1) 0)
                          (match_dup 2)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_ashift:<HALF> (subreg:<HALF> (match_dup 1) 16)
                          (match_dup 2)))]
  ""
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "lite2, lite2")]
)

(define_expand "usashl<mode>3"
  [(match_operand:S256I 0 "register_operand" "")
   (match_operand:S256I 1 "register_operand" "")
   (match_operand:SI 2 "reg_shift_operand" "")]
  "LVX_2"
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
  "LVX_2 && (!HAVE_LVX_US_ASHIFT_<MODE>)"
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
  "LVX_2 && (HAVE_LVX_US_ASHIFT_<MODE>)"
  "#"
  "HAVE_LVX_US_ASHIFT_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (us_ashift:<HALF> (subreg:<HALF> (match_dup 1) 0)
                          (match_dup 2)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (us_ashift:<HALF> (subreg:<HALF> (match_dup 1) 16)
                          (match_dup 2)))]
  ""
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "lite2, lite2")]
)

(define_expand "ashr<mode>3"
  [(set (match_operand:S256I 0 "register_operand" "")
        (ashiftrt:S256I (match_operand:S256I 1 "register_operand" "")
                        (match_operand:SI 2 "reg_shift_operand" "")))]
  "LVX_2"
  ""
)

(define_insn_and_split "ashr<mode>3_1"
  [(set (match_operand:S256I 0 "register_operand" "=&r,r")
        (ashiftrt:S256I (match_operand:S256I 1 "register_operand" "r,r")
                        (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  "LVX_2 && (!HAVE_LVX_ASHIFTRT_<MODE> && HAVE_LVX_ASHIFTRT_<HALF>)"
  "#"
  "!HAVE_LVX_ASHIFTRT_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ashiftrt:<HALF> (subreg:<HALF> (match_dup 1) 0)
                         (match_dup 2)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ashiftrt:<HALF> (subreg:<HALF> (match_dup 1) 16)
                         (match_dup 2)))]
  ""
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "lite2, lite2")]
)

(define_insn "ashr<mode>3_2"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (ashiftrt:S256I (match_operand:S256I 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2 && (HAVE_LVX_ASHIFTRT_<MODE>)"
  {
    return "sra<hsuffix> %L0 = %L1, %2\n\tsra<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_expand "lshr<mode>3"
  [(set (match_operand:S256I 0 "register_operand" "")
        (lshiftrt:S256I (match_operand:S256I 1 "register_operand" "")
                        (match_operand:SI 2 "reg_shift_operand" "")))]
  "LVX_2"
  ""
)

(define_insn_and_split "lshr<mode>3_1"
  [(set (match_operand:S256I 0 "register_operand" "=&r,r")
        (lshiftrt:S256I (match_operand:S256I 1 "register_operand" "r,r")
                        (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  "LVX_2 && (!HAVE_LVX_LSHIFTRT_<MODE> && HAVE_LVX_LSHIFTRT_<HALF>)"
  "#"
  "!HAVE_LVX_LSHIFTRT_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (lshiftrt:<HALF> (subreg:<HALF> (match_dup 1) 0)
                         (match_dup 2)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (lshiftrt:<HALF> (subreg:<HALF> (match_dup 1) 16)
                         (match_dup 2)))]
  ""
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "lite2, lite2")]
)

(define_insn "lshr<mode>3_2"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (lshiftrt:S256I (match_operand:S256I 1 "register_operand" "r")
                        (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2 && (HAVE_LVX_LSHIFTRT_<MODE>)"
  {
    return "srl<hsuffix> %L0 = %L1, %2\n\tsrl<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn_and_split "sshr<mode>3"
  [(set (match_operand:S256I 0 "register_operand" "=&r,r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r,r")
                       (match_operand:SI 2 "reg_shift_operand" "r,U06")] UNSPEC_SRS))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (match_dup 2)] UNSPEC_SRS))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (match_dup 2)] UNSPEC_SRS))]
  ""
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "lite2, lite2")]
)

(define_expand "avg<mode>3_floor"
  [(set (match_operand:S256I 0 "register_operand" "")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "")
                       (match_operand:S256I 2 "register_operand" "")] UNSPEC_AVG))]
  "LVX_2"
  ""
)

(define_insn_and_split "avg<mode>3_floor_1"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r")
                       (match_operand:S256I 2 "register_operand" "r")] UNSPEC_AVG))]
  "LVX_2 && (!HAVE_LVX_AVG_<MODE> && HAVE_LVX_AVG_<HALF>)"
  "#"
  "!HAVE_LVX_AVG_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)] UNSPEC_AVG))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)] UNSPEC_AVG))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "avg<mode>3_floor_2"
  [(set (match_operand:S256I 0 "register_operand" "=r,r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r,r")
                       (match_operand:S256I 2 "reg_or_splat32_operand" "r,SXW")] UNSPEC_AVG))]
  "LVX_2 && (HAVE_LVX_AVG_<MODE>)"
  {
    if (which_alternative == 1)
      return "avg<hsuffix> %L0 = %L1, %W2\n\tavg<hsuffix> %M0 = %M1, %W2";
    return "avg<hsuffix> %L0 = %L1, %L2\n\tavg<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_expand "avg<mode>3_ceil"
  [(set (match_operand:S256I 0 "register_operand" "")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "")
                       (match_operand:S256I 2 "register_operand" "")] UNSPEC_AVGR))]
  "LVX_2"
  ""
)

(define_insn_and_split "avg<mode>3_ceil_1"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r")
                       (match_operand:S256I 2 "register_operand" "r")] UNSPEC_AVGR))]
  "LVX_2 && (!HAVE_LVX_CEIL_AVG_<MODE> && HAVE_LVX_CEIL_AVG_<HALF>)"
  "#"
  "!HAVE_LVX_CEIL_AVG_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)] UNSPEC_AVGR))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)] UNSPEC_AVGR))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "avg<mode>3_ceil_2"
  [(set (match_operand:S256I 0 "register_operand" "=r,r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r,r")
                       (match_operand:S256I 2 "reg_or_splat32_operand" "r,SXW")] UNSPEC_AVGR))]
  "LVX_2 && (HAVE_LVX_CEIL_AVG_<MODE>)"
  {
    if (which_alternative == 1)
      return "avgr<hsuffix> %L0 = %L1, %W2\n\tavgr<hsuffix> %M0 = %M1, %W2";
    return "avgr<hsuffix> %L0 = %L1, %L2\n\tavgr<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_expand "uavg<mode>3_floor"
  [(set (match_operand:S256I 0 "register_operand" "")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "")
                       (match_operand:S256I 2 "register_operand" "")] UNSPEC_AVGU))]
  "LVX_2"
  ""
)

(define_insn_and_split "uavg<mode>3_floor_1"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r")
                       (match_operand:S256I 2 "register_operand" "r")] UNSPEC_AVGU))]
  "LVX_2 && (!HAVE_LVX_UAVG_<MODE> && HAVE_LVX_UAVG_<HALF>)"
  "#"
  "!HAVE_LVX_UAVG_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)] UNSPEC_AVGU))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)] UNSPEC_AVGU))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "uavg<mode>3_floor_2"
  [(set (match_operand:S256I 0 "register_operand" "=r,r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r,r")
                       (match_operand:S256I 2 "reg_or_splat32_operand" "r,SXW")] UNSPEC_AVGU))]
  "LVX_2 && (HAVE_LVX_UAVG_<MODE>)"
  {
    if (which_alternative == 1)
      return "avgu<hsuffix> %L0 = %L1, %W2\n\tavgu<hsuffix> %M0 = %M1, %W2";
    return "avgu<hsuffix> %L0 = %L1, %L2\n\tavgu<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_expand "uavg<mode>3_ceil"
  [(set (match_operand:S256I 0 "register_operand" "")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "")
                       (match_operand:S256I 2 "register_operand" "")] UNSPEC_AVGRU))]
  "LVX_2"
  ""
)

(define_insn_and_split "uavg<mode>3_ceil_1"
  [(set (match_operand:S256I 0 "register_operand" "=r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r")
                       (match_operand:S256I 2 "register_operand" "r")] UNSPEC_AVGRU))]
  "LVX_2 && (!HAVE_LVX_CEIL_UAVG_<MODE> && HAVE_LVX_CEIL_UAVG_<HALF>)"
  "#"
  "!HAVE_LVX_CEIL_UAVG_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)] UNSPEC_AVGRU))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unspec:<HALF> [(subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)] UNSPEC_AVGRU))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "uavg<mode>3_ceil_2"
  [(set (match_operand:S256I 0 "register_operand" "=r,r")
        (unspec:S256I [(match_operand:S256I 1 "register_operand" "r,r")
                       (match_operand:S256I 2 "reg_or_splat32_operand" "r,SXW")] UNSPEC_AVGRU))]
  "LVX_2 && (HAVE_LVX_CEIL_UAVG_<MODE>)"
  {
    if (which_alternative == 1)
      return "avgru<hsuffix> %L0 = %L1, %W2\n\tavgru<hsuffix> %M0 = %M1, %W2";
    return "avgru<hsuffix> %L0 = %L1, %L2\n\tavgru<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_expand "extend<mode><wide>2"
  [(set (match_operand:<WIDE> 0 "register_operand" "")
        (sign_extend:<WIDE> (match_operand:S256L 1 "register_operand" "")))]
  "LVX_2"
  {
    emit_insn (gen_lvx_sx<widenx> (operands[0], operands[1]));
    DONE;
  }
)

(define_expand "zero_extend<mode><wide>2"
  [(set (match_operand:<WIDE> 0 "register_operand" "")
        (zero_extend:<WIDE> (match_operand:S256L 1 "register_operand" "")))]
  "LVX_2"
  {
    emit_insn (gen_lvx_zx<widenx> (operands[0], operands[1]));
    DONE;
  }
)


;; V256I (V16HI V4DI)



;; V256J (V16HI V8SI V4DI)

(define_insn "add<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r,r")
        (plus:V256J (match_operand:V256J 1 "register_operand" "r,r")
                    (match_operand:V256J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "add<hsuffix> %L0 = %L1, %W2\n\tadd<hsuffix> %M0 = %M1, %W2";
    return "add<hsuffix> %L0 = %L1, %L2\n\tadd<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "*add<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (plus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "add<hsuffix> %L0 = %1, %L2\n\tadd<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_expand "ssadd<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "")
        (ss_plus:V256J (match_operand:V256J 1 "register_operand" "")
                       (match_operand:V256J 2 "register_operand" "")))]
  "LVX_2"
  ""
)

(define_insn_and_split "ssadd<mode>3_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_plus:V256J (match_operand:V256J 1 "register_operand" "r")
                       (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (!HAVE_LVX_SS_PLUS_<MODE>)"
  "#"
  "!HAVE_LVX_SS_PLUS_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_plus:<HALF> (subreg:<HALF> (match_dup 1) 0)
                        (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_plus:<HALF> (subreg:<HALF> (match_dup 1) 16)
                        (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "ssadd<mode>3_2"
  [(set (match_operand:V256J 0 "register_operand" "=r,r")
        (ss_plus:V256J (match_operand:V256J 1 "register_operand" "r,r")
                       (match_operand:V256J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2 && (HAVE_LVX_SS_PLUS_<MODE>)"
  {
    if (which_alternative == 1)
      return "adds<hsuffix> %L0 = %L1, %W2\n\tadds<hsuffix> %M0 = %M1, %W2";
    return "adds<hsuffix> %L0 = %L1, %L2\n\tadds<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn_and_split "*ssadd<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (ss_plus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                       (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (!HAVE_LVX_SS_PLUS_<MODE> && HAVE_LVX_SS_PLUS_<HALF>)"
  "#"
  "!HAVE_LVX_SS_PLUS_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_plus:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                        (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_plus:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                        (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*ssadd<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_plus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_SS_PLUS_<MODE>)"
  {
    return "adds<hsuffix> %L0 = %1, %L2\n\tadds<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*ssadd<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (ss_plus:V256J (match_operand:V256J 1 "register_operand" "r")
                       (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (!HAVE_LVX_SS_PLUS_<MODE> && HAVE_LVX_SS_PLUS_<HALF>)"
  "#"
  "!HAVE_LVX_SS_PLUS_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_plus:<HALF> (subreg:<HALF> (match_dup 1) 0)
                        (vec_duplicate:<HALF> (match_dup 2))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_plus:<HALF> (subreg:<HALF> (match_dup 1) 16)
                        (vec_duplicate:<HALF> (match_dup 2))))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*ssadd<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_plus:V256J (match_operand:V256J 1 "register_operand" "r")
                    (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_SS_PLUS_<MODE>)"
  {
    return "adds<hsuffix> %L0 = %L1, %2\n\tadds<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_expand "usadd<mode>3"
  [(match_operand:V256J 0 "register_operand" "")
   (match_operand:V256J 1 "register_operand" "")
   (match_operand:V256J 2 "register_operand" "")]
  "LVX_2"
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
  "LVX_2 && (!HAVE_LVX_US_PLUS_<MODE>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "usadd<mode>3_2"
  [(set (match_operand:V256J 0 "register_operand" "=r,r")
        (us_plus:V256J (match_operand:V256J 1 "register_operand" "r,r")
                       (match_operand:V256J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2 && (HAVE_LVX_US_PLUS_<MODE>)"
  {
    if (which_alternative == 1)
      return "addus<hsuffix> %L0 = %L1, %W2\n\taddus<hsuffix> %M0 = %M1, %W2";
    return "addus<hsuffix> %L0 = %L1, %L2\n\taddus<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn_and_split "*usadd<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (us_plus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                       (match_operand:V256J 2 "register_operand" "r")))
   (clobber (match_scratch:V256J 3 "=&r"))
   (clobber (match_scratch:V256J 4 "=&r"))]
  "LVX_2 && (!HAVE_LVX_US_PLUS_<MODE>)"
  "#"
  "!HAVE_LVX_US_PLUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (plus:V256J (vec_duplicate:V256J (match_dup 1)) (match_dup 2)))
   (set (match_dup 4)
        (ltu:V256J (match_dup 3) (match_dup 2)))
   (set (match_dup 0)
        (ior:V256J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*usadd<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (us_plus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                       (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_US_PLUS_<MODE>)"
  {
    return "addus<hsuffix> %L0 = %1, %L2\n\taddus<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*usadd<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (us_plus:V256J (match_operand:V256J 1 "register_operand" "r")
                       (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))
   (clobber (match_scratch:V256J 3 "=&r"))
   (clobber (match_scratch:V256J 4 "=&r"))]
  "LVX_2 && (!HAVE_LVX_US_PLUS_<MODE>)"
  "#"
  "!HAVE_LVX_US_PLUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (plus:V256J (match_dup 1) (vec_duplicate:V256J (match_dup 2))))
   (set (match_dup 4)
        (ltu:V256J (match_dup 3) (match_dup 1)))
   (set (match_dup 0)
        (ior:V256J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*usadd<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (us_plus:V256J (match_operand:V256J 1 "register_operand" "r")
                       (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_US_PLUS_<MODE>)"
  {
    return "addus<hsuffix> %L0 = %L1, %2\n\taddus<hsuffix> %M0 = %M1, %2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*addx2<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r")
                                  (const_int 1))
                    (match_operand:V256K 2 "register_operand" "r")))]
  "LVX_2 && (!HAVE_LVX_MUL02_ADD_<MODE> && HAVE_LVX_MUL02_ADD_<HALF>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*addx2<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r,r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r,r")
                                  (const_int 1))
                    (match_operand:V256K 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2 && (HAVE_LVX_MUL02_ADD_<MODE>)"
  {
    if (which_alternative == 1)
      return "addx2<hsuffix> %L0 = %L1, %W2\n\taddx2<hsuffix> %M0 = %M1, %W2";
    return "addx2<hsuffix> %L0 = %L1, %L2\n\taddx2<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn_and_split "*addx4<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r")
                                  (const_int 2))
                    (match_operand:V256K 2 "register_operand" "r")))]
  "LVX_2 && (!HAVE_LVX_MUL04_ADD_<MODE> && HAVE_LVX_MUL04_ADD_<HALF>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*addx4<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r,r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r,r")
                                  (const_int 2))
                    (match_operand:V256K 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2 && (HAVE_LVX_MUL04_ADD_<MODE>)"
  {
    if (which_alternative == 1)
      return "addx4<hsuffix> %L0 = %L1, %W2\n\taddx4<hsuffix> %M0 = %M1, %W2";
    return "addx4<hsuffix> %L0 = %L1, %L2\n\taddx4<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn_and_split "*addx8<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r")
                                  (const_int 3))
                    (match_operand:V256K 2 "register_operand" "r")))]
  "LVX_2 && (!HAVE_LVX_MUL08_ADD_<MODE> && HAVE_LVX_MUL08_ADD_<HALF>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*addx8<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r,r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r,r")
                                  (const_int 3))
                    (match_operand:V256K 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2 && (HAVE_LVX_MUL08_ADD_<MODE>)"
  {
    if (which_alternative == 1)
      return "addx8<hsuffix> %L0 = %L1, %W2\n\taddx8<hsuffix> %M0 = %M1, %W2";
    return "addx8<hsuffix> %L0 = %L1, %L2\n\taddx8<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn_and_split "*addx16<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r")
                                  (const_int 4))
                    (match_operand:V256K 2 "register_operand" "r")))]
  "LVX_2 && (!HAVE_LVX_MUL16_ADD_<MODE> && HAVE_LVX_MUL16_ADD_<HALF>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*addx16<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r,r")
        (plus:V256K (ashift:V256K (match_operand:V256K 1 "register_operand" "r,r")
                                  (const_int 4))
                    (match_operand:V256K 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2 && (HAVE_LVX_MUL16_ADD_<MODE>)"
  {
    if (which_alternative == 1)
      return "addx16<hsuffix> %L0 = %L1, %W2\n\taddx16<hsuffix> %M0 = %M1, %W2";
    return "addx16<hsuffix> %L0 = %L1, %L2\n\taddx16<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "sub<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r,r")
        (minus:V256J (match_operand:V256J 1 "reg_or_splat32_operand" "r,SXW")
                     (match_operand:V256J 2 "register_operand" "r,r")))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "sbf<hsuffix> %L0 = %L2, %W1\n\tsbf<hsuffix> %M0 = %M2, %W1";
    return "sbf<hsuffix> %L0 = %L2, %L1\n\tsbf<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "*sub<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (minus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                     (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "sbf<hsuffix> %L0 = %L2, %1\n\tsbf<hsuffix> %M0 = %M2, %1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_expand "sssub<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "")
        (ss_minus:V256J (match_operand:V256J 1 "register_operand" "")
                        (match_operand:V256J 2 "register_operand" "")))]
  "LVX_2"
  ""
)

(define_insn_and_split "sssub<mode>3_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_minus:V256J (match_operand:V256J 1 "register_operand" "r")
                        (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (!HAVE_LVX_SS_MINUS_<MODE> && HAVE_LVX_SS_MINUS_<HALF>)"
  "#"
  "!HAVE_LVX_SS_MINUS_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_minus:<HALF> (subreg:<HALF> (match_dup 1) 0)
                         (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_minus:<HALF> (subreg:<HALF> (match_dup 1) 16)
                         (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "sssub<mode>3_2"
  [(set (match_operand:V256J 0 "register_operand" "=r,r")
        (ss_minus:V256J (match_operand:V256J 1 "reg_or_splat32_operand" "r,SXW")
                        (match_operand:V256J 2 "register_operand" "r,r")))]
  "LVX_2 && (HAVE_LVX_SS_MINUS_<MODE>)"
  {
    if (which_alternative == 1)
      return "sbfs<hsuffix> %L0 = %L2, %W1\n\tsbfs<hsuffix> %M0 = %M2, %W1";
    return "sbfs<hsuffix> %L0 = %L2, %L1\n\tsbfs<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn_and_split "*sssub<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (ss_minus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                        (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (!HAVE_LVX_SS_MINUS_<MODE> && HAVE_LVX_SS_MINUS_<HALF>)"
  "#"
  "!HAVE_LVX_SS_MINUS_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_minus:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                         (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_minus:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                         (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*sssub<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_minus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                     (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_SS_MINUS_<MODE>)"
  {
    return "sbfs<hsuffix> %L0 = %L2, %1\n\tsbfs<hsuffix> %M0 = %M2, %1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*sssub<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (ss_minus:V256J (match_operand:V256J 1 "register_operand" "r")
                        (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (!HAVE_LVX_SS_MINUS_<MODE> && HAVE_LVX_SS_MINUS_<HALF>)"
  "#"
  "!HAVE_LVX_SS_MINUS_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_minus:<HALF> (subreg:<HALF> (match_dup 1) 0)
                         (vec_duplicate:<HALF> (match_dup 2))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_minus:<HALF> (subreg:<HALF> (match_dup 1) 16)
                         (vec_duplicate:<HALF> (match_dup 2))))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*sssub<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_minus:V256J (match_operand:V256J 1 "register_operand" "r")
                     (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_SS_MINUS_<MODE>)"
  {
    return "sbfs<hsuffix> %L0 = %2, %L1\n\tsbfs<hsuffix> %M0 = %2, %M1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_expand "ussub<mode>3"
  [(match_operand:V256J 0 "register_operand" "")
   (match_operand:V256J 1 "register_operand" "")
   (match_operand:V256J 2 "register_operand" "")]
  "LVX_2"
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
  "LVX_2 && (!HAVE_LVX_US_MINUS_<MODE>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "ussub<mode>3_2"
  [(set (match_operand:V256J 0 "register_operand" "=r,r")
        (us_minus:V256J (match_operand:V256J 1 "reg_or_splat32_operand" "r,SXW")
                        (match_operand:V256J 2 "register_operand" "r,r")))]
  "LVX_2 && (HAVE_LVX_US_MINUS_<MODE>)"
  {
    if (which_alternative == 1)
      return "sbfus<hsuffix> %L0 = %L2, %W1\n\tsbfus<hsuffix> %M0 = %M2, %W1";
    return "sbfus<hsuffix> %L0 = %L2, %L1\n\tsbfus<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn_and_split "*ussub<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (us_minus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                        (match_operand:V256J 2 "register_operand" "r")))
   (clobber (match_scratch:V256J 3 "=&r"))
   (clobber (match_scratch:V256J 4 "=&r"))]
  "LVX_2 && (!HAVE_LVX_US_MINUS_<MODE>)"
  "#"
  "!HAVE_LVX_US_MINUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (minus:V256J (vec_duplicate:V256J (match_dup 1)) (match_dup 2)))
   (set (match_dup 4)
        (geu:V256J (vec_duplicate:V256J (match_dup 1)) (match_dup 3)))
   (set (match_dup 0)
        (and:V256J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*ussub<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (us_minus:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                        (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_US_MINUS_<MODE>)"
  {
    return "sbfus<hsuffix> %L0 = %L2, %1\n\tsbfus<hsuffix> %M0 = %M2, %1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*ussub<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=&r")
        (us_minus:V256J (match_operand:V256J 1 "register_operand" "r")
                        (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))
   (clobber (match_scratch:V256J 3 "=&r"))
   (clobber (match_scratch:V256J 4 "=&r"))]
  "LVX_2 && (!HAVE_LVX_US_MINUS_<MODE>)"
  "#"
  "!HAVE_LVX_US_MINUS_<MODE> && reload_completed"
  [(set (match_dup 3)
        (minus:V256J (match_dup 1) (vec_duplicate:V256J (match_dup 2))))
   (set (match_dup 4)
        (leu:V256J (match_dup 3) (match_dup 1)))
   (set (match_dup 0)
        (and:V256J (match_dup 3) (match_dup 4)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*ussub<mode>3_s2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (us_minus:V256J (match_operand:V256J 1 "register_operand" "r")
                        (vec_duplicate:V256J (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_US_MINUS_<MODE>)"
  {
    return "sbfus<hsuffix> %L0 = %2, %L1\n\tsbfus<hsuffix> %M0 = %2, %M1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*sbfx2<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (minus:V256K (match_operand:V256K 1 "register_operand" "r")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r")
                                   (const_int 1))))]
  "LVX_2 && (!HAVE_LVX_MUL02_SUB_<MODE> && HAVE_LVX_MUL02_SUB_<HALF>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*sbfx2<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r,r")
        (minus:V256K (match_operand:V256K 1 "reg_or_splat32_operand" "r,SXW")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r,r")
                                   (const_int 1))))]
  "LVX_2 && (HAVE_LVX_MUL02_SUB_<MODE>)"
  {
    if (which_alternative == 1)
      return "sbfx2<hsuffix> %L0 = %L2, %W1\n\tsbfx2<hsuffix> %M0 = %M2, %W1";
    return "sbfx2<hsuffix> %L0 = %L2, %L1\n\tsbfx2<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn_and_split "*sbfx4<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (minus:V256K (match_operand:V256K 1 "register_operand" "r")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r")
                                   (const_int 2))))]
  "LVX_2 && (!HAVE_LVX_MUL04_SUB_<MODE> && HAVE_LVX_MUL04_SUB_<HALF>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*sbfx4<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r,r")
        (minus:V256K (match_operand:V256K 1 "reg_or_splat32_operand" "r,SXW")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r,r")
                                   (const_int 2))))]
  "LVX_2 && (HAVE_LVX_MUL04_SUB_<MODE>)"
  {
    if (which_alternative == 1)
      return "sbfx4<hsuffix> %L0 = %L2, %W1\n\tsbfx4<hsuffix> %M0 = %M2, %W1";
    return "sbfx4<hsuffix> %L0 = %L2, %L1\n\tsbfx4<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn_and_split "*sbfx8<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (minus:V256K (match_operand:V256K 1 "register_operand" "r")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r")
                                   (const_int 3))))]
  "LVX_2 && (!HAVE_LVX_MUL08_SUB_<MODE> && HAVE_LVX_MUL08_SUB_<HALF>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*sbfx8<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r,r")
        (minus:V256K (match_operand:V256K 1 "reg_or_splat32_operand" "r,SXW")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r,r")
                                   (const_int 3))))]
  "LVX_2 && (HAVE_LVX_MUL08_SUB_<MODE>)"
  {
    if (which_alternative == 1)
      return "sbfx8<hsuffix> %L0 = %L2, %W1\n\tsbfx8<hsuffix> %M0 = %M2, %W1";
    return "sbfx8<hsuffix> %L0 = %L2, %L1\n\tsbfx8<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn_and_split "*sbfx16<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r")
        (minus:V256K (match_operand:V256K 1 "register_operand" "r")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r")
                                   (const_int 4))))]
  "LVX_2 && (!HAVE_LVX_MUL16_SUB_<MODE> && HAVE_LVX_MUL16_SUB_<HALF>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "*sbfx16<suffix>"
  [(set (match_operand:V256K 0 "register_operand" "=r,r")
        (minus:V256K (match_operand:V256K 1 "reg_or_splat32_operand" "r,SXW")
                     (ashift:V256K (match_operand:V256K 2 "register_operand" "r,r")
                                   (const_int 4))))]
  "LVX_2 && (HAVE_LVX_MUL16_SUB_<MODE>)"
  {
    if (which_alternative == 1)
      return "sbfx16<hsuffix> %L0 = %L2, %W1\n\tsbfx16<hsuffix> %M0 = %M2, %W1";
    return "sbfx16<hsuffix> %L0 = %L2, %L1\n\tsbfx16<hsuffix> %M0 = %M2, %M1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_expand "div<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "")
        (div:V256J (match_operand:V256J 1 "register_operand" "")
                   (match_operand:V256J 2 "register_operand" "")))]
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  [(set (match_operand:V256J 0 "register_operand" "=r,r")
        (smin:V256J (match_operand:V256J 1 "register_operand" "r,r")
                    (match_operand:V256J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "min<hsuffix> %L0 = %L1, %W2\n\tmin<hsuffix> %M0 = %M1, %W2";
    return "min<hsuffix> %L0 = %L1, %L2\n\tmin<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "*smin<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (smin:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "min<hsuffix> %L0 = %1, %L2\n\tmin<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn "smax<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r,r")
        (smax:V256J (match_operand:V256J 1 "register_operand" "r,r")
                    (match_operand:V256J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "max<hsuffix> %L0 = %L1, %W2\n\tmax<hsuffix> %M0 = %M1, %W2";
    return "max<hsuffix> %L0 = %L1, %L2\n\tmax<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "*smax<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (smax:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "max<hsuffix> %L0 = %1, %L2\n\tmax<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn "umin<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r,r")
        (umin:V256J (match_operand:V256J 1 "register_operand" "r,r")
                    (match_operand:V256J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "minu<hsuffix> %L0 = %L1, %W2\n\tminu<hsuffix> %M0 = %M1, %W2";
    return "minu<hsuffix> %L0 = %L1, %L2\n\tminu<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "*umin<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (umin:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "minu<hsuffix> %L0 = %1, %L2\n\tminu<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn "umax<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r,r")
        (umax:V256J (match_operand:V256J 1 "register_operand" "r,r")
                    (match_operand:V256J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "maxu<hsuffix> %L0 = %L1, %W2\n\tmaxu<hsuffix> %M0 = %M1, %W2";
    return "maxu<hsuffix> %L0 = %L1, %L2\n\tmaxu<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "*umax<mode>3_s1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (umax:V256J (vec_duplicate:V256J (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  {
    return "maxu<hsuffix> %L0 = %1, %L2\n\tmaxu<hsuffix> %M0 = %1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)


;; V256L

(define_insn "and<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r,r")
        (and:V256L (match_operand:V256L 1 "register_operand" "r,r")
                   (match_operand:V256L 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "andq %L0 = %L1, %W2.@\n\tandq %M0 = %M1, %W2.@";
    return "andq %L0 = %L1, %L2\n\tandq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "*nand<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r,r")
        (ior:V256L (not:V256L (match_operand:V256L 1 "register_operand" "r,r"))
                   (not:V256L (match_operand:V256L 2 "reg_or_splat32_operand" "r,SXW"))))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "nandq %L0 = %L1, %W2.@\n\tnandq %M0 = %M1, %W2.@";
    return "nandq %L0 = %L1, %L2\n\tnandq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "*andn<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r,r")
        (and:V256L (not:V256L (match_operand:V256L 1 "register_operand" "r,r"))
                   (match_operand:V256L 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "andnq %L0 = %L1, %W2.@\n\tandnq %M0 = %M1, %W2.@";
    return "andnq %L0 = %L1, %L2\n\tandnq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "ior<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r,r")
        (ior:V256L (match_operand:V256L 1 "register_operand" "r,r")
                   (match_operand:V256L 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "iorq %L0 = %L1, %W2.@\n\tiorq %M0 = %M1, %W2.@";
    return "iorq %L0 = %L1, %L2\n\tiorq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "*nior<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r,r")
        (and:V256L (not:V256L (match_operand:V256L 1 "register_operand" "r,r"))
                   (not:V256L (match_operand:V256L 2 "reg_or_splat32_operand" "r,SXW"))))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "niorq %L0 = %L1, %W2.@\n\tniorq %M0 = %M1, %W2.@";
    return "niorq %L0 = %L1, %L2\n\tniorq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "*iorn<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r,r")
        (ior:V256L (not:V256L (match_operand:V256L 1 "register_operand" "r,r"))
                   (match_operand:V256L 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "iornq %L0 = %L1, %W2.@\n\tiornq %M0 = %M1, %W2.@";
    return "iornq %L0 = %L1, %L2\n\tiornq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "xor<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r,r")
        (xor:V256L (match_operand:V256L 1 "register_operand" "r,r")
                   (match_operand:V256L 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "eorq %L0 = %L1, %W2.@\n\teorq %M0 = %M1, %W2.@";
    return "eorq %L0 = %L1, %L2\n\teorq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn "*nxor<mode>3"
  [(set (match_operand:V256L 0 "register_operand" "=r,r")
        (not:V256L (xor:V256L (match_operand:V256L 1 "register_operand" "r,r")
                              (match_operand:V256L 2 "reg_or_splat32_operand" "r,SXW"))))]
  "LVX_2"
  {
    if (which_alternative == 1)
      return "neorq %L0 = %L1, %W2.@\n\tneorq %M0 = %M1, %W2.@";
    return "neorq %L0 = %L1, %L2\n\tneorq %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)


;; V256M

(define_insn_and_split "madd<mode><mode>4"
  [(set (match_operand:V256M 0 "register_operand" "=r,r")
        (plus:V256M (mult:V256M (match_operand:V256M 1 "register_operand" "r,r")
                                (match_operand:V256M 2 "reg_or_splat32_operand" "r,SXW"))
                    (match_operand:V256M 3 "register_operand" "0,0")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (plus:<HALF> (mult:<HALF> (subreg:<HALF> (match_dup 1) 0)
                                  (match_dup 4))
                     (subreg:<HALF> (match_dup 3) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (plus:<HALF> (mult:<HALF> (subreg:<HALF> (match_dup 1) 16)
                                  (match_dup 5))
                     (subreg:<HALF> (match_dup 3) 16)))]
  {
    operands[4] = simplify_gen_subreg (<HALF>mode, operands[2], <MODE>mode, 0);
    operands[5] = simplify_gen_subreg (<HALF>mode, operands[2], <MODE>mode, 16);
    if (!operands[4] || !operands[5])
      FAIL;
  }
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)

(define_insn_and_split "msub<mode><mode>4"
  [(set (match_operand:V256M 0 "register_operand" "=r,r")
        (minus:V256M (match_operand:V256M 3 "register_operand" "0,0")
                     (mult:V256M (match_operand:V256M 1 "register_operand" "r,r")
                                 (match_operand:V256M 2 "reg_or_splat32_operand" "r,SXW"))))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (minus:<HALF> (subreg:<HALF> (match_dup 3) 0)
                      (mult:<HALF> (subreg:<HALF> (match_dup 1) 0)
                                   (match_dup 4))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (minus:<HALF> (subreg:<HALF> (match_dup 3) 16)
                      (mult:<HALF> (subreg:<HALF> (match_dup 1) 16)
                                   (match_dup 5))))]
  {
    operands[4] = simplify_gen_subreg (<HALF>mode, operands[2], <MODE>mode, 0);
    operands[5] = simplify_gen_subreg (<HALF>mode, operands[2], <MODE>mode, 16);
    if (!operands[4] || !operands[5])
      FAIL;
  }
  [(set_attr "type" "imadd")
   (set_attr "issue" "lite")]
)



;; V256J (V16HI V8SI V4DI)

(define_insn "neg<mode>2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (neg:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "LVX_2"
  {
    return "neg<hsuffix> %L0 = %L1\n\tneg<hsuffix> %M0 = %M1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length"          "32")]
)

(define_expand "ssneg<mode>2"
  [(set (match_operand:V256J 0 "register_operand" "")
        (ss_neg:V256J (match_operand:V256J 1 "register_operand" "")))]
  "LVX_2"
  ""
)

(define_insn_and_split "ssneg<mode>2_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_neg:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "LVX_2 && (!HAVE_LVX_SS_NEG_<MODE> && HAVE_LVX_SS_NEG_<HALF>)"
  "#"
  "!HAVE_LVX_SS_NEG_<MODE> && reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ss_neg:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ss_neg:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2_x2")]
)

(define_insn "ssneg<mode>2_2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_neg:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_SS_NEG_<MODE>)"
  {
    return "negs<hsuffix> %L0 = %L1\n\tnegs<hsuffix> %M0 = %M1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length"          "32")]
)

(define_expand "abs<mode>2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (abs:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "LVX_2"
  ""
)

(define_insn_and_split "abs<mode>2_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (abs:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "LVX_2 && (!HAVE_LVX_ABS_<MODE>)"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (abs:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (abs:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2_x2")]
)

(define_insn "abs<mode>2_2"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (abs:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_ABS_<MODE>)"
  {
    return "abs<hsuffix> %L0 = %L1\n\tabs<hsuffix> %M0 = %M1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length"          "32")]
)

(define_expand "ssabs<mode>2"
  [(set (match_operand:V256J 0 "register_operand" "")
        (ss_abs:V256J (match_operand:V256J 1 "register_operand" "")))]
  "LVX_2"
  ""
)

(define_insn_and_split "ssabs<mode>2_1"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (ss_abs:V256J (match_operand:V256J 1 "register_operand" "r")))]
  "LVX_2 && (!HAVE_LVX_SS_ABS_<MODE>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length"          "32")]
)

(define_insn_and_split "clrsb<mode>2"
  [(set (match_operand:V256CZ 0 "register_operand" "=r")
        (clrsb:V256CZ (match_operand:V256CZ 1 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (clrsb:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (clrsb:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "clz<mode>2"
  [(set (match_operand:V256CZ 0 "register_operand" "=r")
        (clz:V256CZ (match_operand:V256CZ 1 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (clz:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (clz:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "ctz<mode>2"
  [(set (match_operand:V256CZ 0 "register_operand" "=r")
        (ctz:V256CZ (match_operand:V256CZ 1 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (ctz:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (ctz:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "popcount<mode>2"
  [(set (match_operand:V256CZ 0 "register_operand" "=r")
        (popcount:V256CZ (match_operand:V256CZ 1 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (popcount:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (popcount:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "one_cmpl<mode>2"
  [(set (match_operand:V256L 0 "register_operand" "=r")
        (not:V256L (match_operand:V256L 1 "register_operand" "r")))]
  "LVX_2"
  {
    return "notq %L0 = %L1\n\tnotq %M0 = %M1";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_expand "abd<mode>3"
  [(match_operand:V256L 0 "register_operand" "")
   (match_operand:V256L 1 "register_operand" "")
   (match_operand:V256L 2 "register_operand" "")]
  "LVX_2"
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
  "LVX_2 && (!HAVE_LVX_ABD_<MODE> && HAVE_LVX_ABD_<HALF>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn "abd<mode>3_2"
  [(set (match_operand:V256L 0 "register_operand" "=r,r")
        (minus:V256L (smax:V256L (match_operand:V256L 1 "register_operand" "r,r")
                                 (match_operand:V256L 2 "reg_or_splat32_operand" "r,SXW"))
                     (smin:V256L (match_dup 1) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_ABD_<MODE>)"
  {
    if (which_alternative == 1)
      return "abd<hsuffix> %L0 = %L1, %W2\n\tabd<hsuffix> %M0 = %M1, %W2";
    return "abd<hsuffix> %L0 = %L1, %L2\n\tabd<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
)

(define_insn_and_split "*abd<mode>_s1"
  [(set (match_operand:V256L 0 "register_operand" "=&r")
        (minus:V256L (smax:V256L (vec_duplicate:V256L (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                                 (match_operand:V256L 2 "register_operand" "r"))
                     (smin:V256L (vec_duplicate:V256L (match_dup 1)) (match_dup 2))))]
  "LVX_2 && (!HAVE_LVX_ABD_<MODE> && HAVE_LVX_ABD_<HALF>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn_and_split "*abd<mode>_s2"
  [(set (match_operand:V256L 0 "register_operand" "=&r")
        (minus:V256L (smax:V256L (match_operand:V256L 1 "register_operand" "r")
                                 (vec_duplicate:V256L (match_operand:<CHUNK> 2 "nonmemory_operand" "r")))
                     (smin:V256L (match_dup 1) (vec_duplicate:V256L (match_dup 2)))))]
  "LVX_2 && (!HAVE_LVX_ABD_<MODE> && HAVE_LVX_ABD_<HALF>)"
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_expand "abds<mode>3"
  [(match_operand:V256L 0 "register_operand" "")
   (match_operand:V256L 1 "register_operand" "")
   (match_operand:V256L 2 "register_operand" "")]
  "LVX_2"
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
  "LVX_2 && (!HAVE_LVX_SS_ABD_<MODE>)"
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
  [(set (match_operand:V256L 0 "register_operand" "=r,r")
        (ss_minus:V256L (smax:V256L (match_operand:V256L 1 "register_operand" "r,r")
                                    (match_operand:V256L 2 "reg_or_splat32_operand" "r,SXW"))
                        (smin:V256L (match_dup 1) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_SS_ABD_<MODE>)"
  {
    if (which_alternative == 1)
      return "abds<hsuffix> %L0 = %L1, %W2\n\tabds<hsuffix> %M0 = %M1, %W2";
    return "abds<hsuffix> %L0 = %L1, %L2\n\tabds<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_expand "abdu<mode>3"
  [(match_operand:V256L 0 "register_operand" "")
   (match_operand:V256L 1 "register_operand" "")
   (match_operand:V256L 2 "register_operand" "")]
  "LVX_2"
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
  "LVX_2 && (!HAVE_LVX_US_ABD_<MODE>)"
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
  [(set (match_operand:V256L 0 "register_operand" "=r,r")
        (minus:V256L (umax:V256L (match_operand:V256L 1 "register_operand" "r,r")
                                 (match_operand:V256L 2 "reg_or_splat32_operand" "r,SXW"))
                     (umin:V256L (match_dup 1) (match_dup 2))))]
  "LVX_2 && (HAVE_LVX_US_ABD_<MODE>)"
  {
    if (which_alternative == 1)
      return "abdu<hsuffix> %L0 = %L1, %W2\n\tabdu<hsuffix> %M0 = %M1, %W2";
    return "abdu<hsuffix> %L0 = %L1, %L2\n\tabdu<hsuffix> %M0 = %M1, %M2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2,lite2_x2")
   (set_attr "length" "8,16")]
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)


;; V8SI

(define_insn_and_split "mul<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r,r")
        (mult:V256J (match_operand:V256J 1 "register_operand" "r,r")
                    (match_operand:V256J 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (mult:<HALF> (subreg:<HALF> (match_dup 1) 0)
                     (match_dup 3)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (mult:<HALF> (subreg:<HALF> (match_dup 1) 16)
                     (match_dup 4)))]
  {
    operands[3] = simplify_gen_subreg (<HALF>mode, operands[2], <MODE>mode, 0);
    operands[4] = simplify_gen_subreg (<HALF>mode, operands[2], <MODE>mode, 16);
    if (!operands[3] || !operands[4])
      FAIL;
  }
  [(set_attr "type" "imul")
   (set_attr "issue" "lite")]
)

;; The negating twin at 256 bits, two MULN<lane> halves.  This one earns its
;; place: the vectorizer's preferred width for these loops is 256-bit, so
;; without it a `-(a[i] * b[i])` loop keeps a MUL + NEG pair per half and the
;; 128-bit `mulneg<mode>3` below never sees the body at all -- which is exactly
;; what happened when only the 128-bit pattern existed.
(define_insn_and_split "mulneg<mode>3"
  [(set (match_operand:V256J 0 "register_operand" "=r")
        (mult:V256J (neg:V256J (match_operand:V256J 1 "register_operand" "r"))
                    (match_operand:V256J 2 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (mult:<HALF> (neg:<HALF> (subreg:<HALF> (match_dup 1) 0))
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (mult:<HALF> (neg:<HALF> (subreg:<HALF> (match_dup 1) 16))
                     (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "imul")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn_and_split "ssashlv4di3"
  [(set (match_operand:V4DI 0 "register_operand" "=&r,r")
        (ss_ashift:V4DI (match_operand:V4DI 1 "register_operand" "r,r")
                        (match_operand:SI 2 "reg_shift_operand" "r,U06")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:V2DI (match_dup 0) 0)
        (ss_ashift:V2DI (subreg:V2DI (match_dup 1) 0)
                        (match_dup 2)))
   (set (subreg:V2DI (match_dup 0) 16)
        (ss_ashift:V2DI (subreg:V2DI (match_dup 1) 16)
                        (match_dup 2)))]
  ""
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "lite2, lite2")]
)

(define_expand "usashlv4di3"
  [(match_operand:V4DI 0 "register_operand" "")
   (match_operand:V4DI 1 "register_operand" "")
   (match_operand:SI 2 "reg_shift_operand" "")]
  "LVX_2"
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
  "LVX_2 && (!HAVE_LVX_US_ASHIFT_V4DI)"
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
  "LVX_2 && (HAVE_LVX_US_ASHIFT_V4DI)"
  "#"
  "HAVE_LVX_US_ASHIFT_V4DI && reload_completed"
  [(set (subreg:V2DI (match_dup 0) 0)
        (us_ashift:V2DI (subreg:V2DI (match_dup 1) 0)
                        (match_dup 2)))
   (set (subreg:V2DI (match_dup 0) 16)
        (us_ashift:V2DI (subreg:V2DI (match_dup 1) 16)
                        (match_dup 2)))]
  ""
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "lite2, lite2")]
)

(define_insn "ashrv4di3"
  [(set (match_operand:V4DI 0 "register_operand" "=r")
        (ashiftrt:V4DI (match_operand:V4DI 1 "register_operand" "r")
                       (match_operand:SI 2 "reg_shift_operand" "rU06")))]
  "LVX_2"
  {
    return "sradp %L0 = %L1, %2\n\tsradp %M0 = %M1, %2";
  }
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
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
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")
   (set_attr "length" "8")]
)

(define_insn_and_split "sshrv4di3"
  [(set (match_operand:V4DI 0 "register_operand" "=&r,r")
        (unspec:V4DI [(match_operand:V4DI 1 "register_operand" "r,r")
                      (match_operand:SI 2 "reg_shift_operand" "r,U06")] UNSPEC_SRS))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:V2DI (match_dup 0) 0)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 0)
                      (match_dup 2)] UNSPEC_SRS))
   (set (subreg:V2DI (match_dup 0) 16)
        (unspec:V2DI [(subreg:V2DI (match_dup 1) 16)
                      (match_dup 2)] UNSPEC_SRS))]
  ""
  [(set_attr "type" "alu, alu")
   (set_attr "issue" "lite2, lite2")]
)


;; VXHF

(define_expand "div<mode>3"
  [(set (match_operand:VXHF 0 "register_operand" "")
        (div:VXHF (match_operand:VXHF 1 "register_float1_operand" "")
                  (match_operand:VXHF 2 "register_operand" "")))]
  "LVX_2"
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

; V8HF <-> V8HI, 128 bits, in one instruction: FIXEDHO/FLOATHO convert eight
;; f16 <-> i16 lanes at once, where this used to promote each 128-bit chunk to
;; V8SF and go f16->f32->i32->i16 (three instructions per chunk).  The "octuple"
;; is eight 16-bit lanes in 128 bits, not a 256-bit datum.
(define_insn "floatv8hiv8hf2"
  [(set (match_operand:V8HF 0 "register_operand" "=r")
        (float:V8HF (match_operand:V8HI 1 "register_operand" "r")))]
  "LVX_2"
  "floatho.rn %0 = %1"
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "floatunsv8hiv8hf2"
  [(set (match_operand:V8HF 0 "register_operand" "=r")
        (unsigned_float:V8HF (match_operand:V8HI 1 "register_operand" "r")))]
  "LVX_2"
  "floatuho.rn %0 = %1"
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "fix_truncv8hfv8hi2"
  [(set (match_operand:V8HI 0 "register_operand" "=r")
        (fix:V8HI (match_operand:V8HF 1 "register_operand" "r")))]
  "LVX_2"
  "fixedho.rz %0 = %1"
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "fixuns_truncv8hfv8hi2"
  [(set (match_operand:V8HI 0 "register_operand" "=r")
        (unsigned_fix:V8HI (match_operand:V8HF 1 "register_operand" "r")))]
  "LVX_2"
  "fixeduho.rz %0 = %1"
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

;; Wider HF conversions (V16HF, V32HF) split into 128-bit V8HF chunks -- one
;; fixedho/floatho each, dual-issued by the VLIW.
(define_expand "float<mask><mode>2"
  [(set (match_operand:VXHFW 0 "register_operand" "")
        (float:VXHFW (match_operand:<MASK> 1 "register_operand" "")))]
  "LVX_2"
  {
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += 16)
      {
        rtx op0 = simplify_gen_subreg (V8HFmode, operands[0], <MODE>mode, offset);
        rtx op1 = simplify_gen_subreg (V8HImode, operands[1], <MASK>mode, offset);
        emit_insn (gen_floatv8hiv8hf2 (op0, op1));
      }
    DONE;
  }
)

(define_expand "floatuns<mask><mode>2"
  [(set (match_operand:VXHFW 0 "register_operand" "")
        (unsigned_float:VXHFW (match_operand:<MASK> 1 "register_operand" "")))]
  "LVX_2"
  {
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += 16)
      {
        rtx op0 = simplify_gen_subreg (V8HFmode, operands[0], <MODE>mode, offset);
        rtx op1 = simplify_gen_subreg (V8HImode, operands[1], <MASK>mode, offset);
        emit_insn (gen_floatunsv8hiv8hf2 (op0, op1));
      }
    DONE;
  }
)

(define_expand "fix_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "")
        (fix:<MASK> (match_operand:VXHFW 1 "register_operand" "")))]
  "LVX_2"
  {
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += 16)
      {
        rtx op0 = simplify_gen_subreg (V8HImode, operands[0], <MASK>mode, offset);
        rtx op1 = simplify_gen_subreg (V8HFmode, operands[1], <MODE>mode, offset);
        emit_insn (gen_fix_truncv8hfv8hi2 (op0, op1));
      }
    DONE;
  }
)

(define_expand "fixuns_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "")
        (unsigned_fix:<MASK> (match_operand:VXHFW 1 "register_operand" "")))]
  "LVX_2"
  {
    unsigned mode_size = GET_MODE_SIZE (<MODE>mode);
    for (unsigned offset = 0; offset < mode_size; offset += 16)
      {
        rtx op0 = simplify_gen_subreg (V8HImode, operands[0], <MASK>mode, offset);
        rtx op1 = simplify_gen_subreg (V8HFmode, operands[1], <MODE>mode, offset);
        emit_insn (gen_fixuns_truncv8hfv8hi2 (op0, op1));
      }
    DONE;
  }
)


;; VXSF

(define_expand "div<mode>3"
  [(set (match_operand:VXSF 0 "register_operand" "")
        (div:VXSF (match_operand:VXSF 1 "register_float1_operand" "")
                  (match_operand:VXSF 2 "register_operand" "")))]
  "LVX_2"
  {
    rtx rm = gen_rtx_CONST_STRING (VOIDmode, "");
    rtx rn = gen_rtx_CONST_STRING (VOIDmode, ".rn");
    rtx a = operands[1], b = operands[2];
    /* No unguarded 1.0/b shortcut.  frec*, an exact reciprocal, is not an LVX
       instruction -- the fdiv* instructions generalise it -- but only at scalar
       width, so there is nothing exact to use here.  fsrec* is a *seed*, so
       using it unconditionally would silently approximate a division the
       user did not ask to have approximated.  The flag-guarded paths below
       are where an approximation is legitimate.  */
    if (flag_reciprocal_math)
      {
        rtx t = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_lvx_fsrec<suffix> (t, b));
        emit_insn (gen_lvx_fmul<suffix> (operands[0], a, t, rm));
      }
    else if (flag_unsafe_math_optimizations)
      {
        rtx re = gen_reg_rtx (<MODE>mode);
        emit_insn (gen_lvx_fsrec<suffix> (re, b));
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
  "LVX_2 && (flag_reciprocal_math)"
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
  "LVX_2"
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
  "LVX_2"
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
  [(set (match_operand:S128F 0 "register_operand" "=r,r")
        (fma:S128F (match_operand:S128F 1 "register_operand" "r,r")
                   (match_operand:S128F 2 "reg_or_splat32_operand" "r,SXW")
                   (match_operand:S128F 3 "register_operand" "0,0")))]
  "LVX_2"
  "@
   ffma<suffix> %0 = %1, %2
   ffma<suffix> %0 = %1, %W2"
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
     (if_then_else (match_operand 1 "float16_inner_mode") (const_string "fmadds") (const_string "fmaddd")))
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn_and_split "fnma<mode>4"
  [(set (match_operand:S128F 0 "register_operand" "=r,r")
        (fma:S128F (neg:S128F (match_operand:S128F 1 "register_operand" "r,r"))
                   (match_operand:S128F 2 "reg_or_splat32_operand" "r,SXW")
                   (match_operand:S128F 3 "register_operand" "0,0")))]
  "LVX_2"
  "@
   ffms<suffix> %0 = %1, %2
   ffms<suffix> %0 = %1, %W2"
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
     (if_then_else (match_operand 1 "float16_inner_mode") (const_string "fmadds") (const_string "fmaddd")))
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "lvx_fnarrow<narrowx>"
  [(set (match_operand:S128F 0 "register_operand" "=r")
        (float_truncate:S128F (match_operand:<WIDE> 1 "register_operand" "r")))]
  "LVX_2 && HAVE_LVX_FP_NARROW_VECTOR"
  "fnarrow<narrowx> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")]
)

(define_expand "trunc<wide><mode>2"
  [(set (match_operand:S128F 0 "register_operand" "")
        (float_truncate:S128F (match_operand:<WIDE> 1 "register_operand" "")))]
  "LVX_2"
  {
    if (HAVE_LVX_FP_NARROW_VECTOR)
      {
	emit_insn (gen_lvx_fnarrow<narrowx> (operands[0], operands[1]));
	DONE;
      }

    /* One scalar FNARROW per lane, through a stack slot.  The lanes cannot be
       reached with subregs -- a 2-byte HF at offset 2 of a register pair is
       not an addressable subreg and simplify_gen_subreg returns NULL -- and
       the 64-bit chunk patterns this used to split into went with 64-bit
       SIMD, which is what made a v8hf divide ICE on a subreg:V4HF.

       Do NOT "simplify" this by dropping the optab and letting the middle end
       lower the conversion: with no trunc<wide><mode>2 the result folds to a
       zero vector at -O1 and above.  A missing optab miscompiles here, it does
       not fall back.  */
    machine_mode dm = GET_MODE_INNER (<MODE>mode);
    machine_mode sm = GET_MODE_INNER (<WIDE>mode);
    rtx smem = assign_stack_temp (<WIDE>mode, GET_MODE_SIZE (<WIDE>mode));
    rtx dmem = assign_stack_temp (<MODE>mode, GET_MODE_SIZE (<MODE>mode));

    emit_move_insn (smem, force_reg (<WIDE>mode, operands[1]));
    for (int i = 0; i < GET_MODE_NUNITS (<MODE>mode); i++)
      {
	rtx t = gen_reg_rtx (sm);
	rtx r = gen_reg_rtx (dm);
	emit_move_insn (t, adjust_address (smem, sm, i * GET_MODE_SIZE (sm)));
	emit_insn (gen_rtx_SET (r, gen_rtx_FLOAT_TRUNCATE (dm, t)));
	emit_move_insn (adjust_address (dmem, dm, i * GET_MODE_SIZE (dm)), r);
      }
    emit_move_insn (operands[0], dmem);
    DONE;
  }
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

;; fmin<mode>3 and fmax<mode>3 are GCC's standard names for C's fmin and fmax,
;; which are 754-2008 minNum: they return the numeric operand when one side is
;; a NaN.  That is fminn/fmaxn, not fmin/fmax -- the same pairing scalar.md got
;; wrong until a412f7a9c65, still wrong here until 2026-09-06.  The mnemonics
;; without the n propagate the NaN and belong to fminimum/fmaximum and to
;; __builtin_lvx_fmin*, which builtin.md now emits directly.
;;
;; Nothing but a NaN operand separates the two, so this cannot be caught by a
;; test that does not feed one; validation/tests/ir/minmax-nan.ll is the check.

(define_insn "smin<mode>3"
  [(set (match_operand:V128F 0 "register_operand" "=r,r")
        (smin:V128F (match_operand:V128F 1 "register_operand" "r,r")
                    (match_operand:V128F 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2 && (HAVE_LVX_MIN_<MODE> && !(HAVE_LVX_BUG_FMIN && flag_signaling_nans))"
  "@
   fminn<suffix> %0 = %1, %2
   fminn<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*smin<mode>3_s1"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (smin:V128F (vec_duplicate:V128F (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128F 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MIN_<MODE> && !(HAVE_LVX_BUG_FMIN && flag_signaling_nans))"
  "fminn<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*smin<mode>3_s2"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (smin:V128F (match_operand:V128F 1 "register_operand" "r")
                    (vec_duplicate:V128F (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_MIN_<MODE> && !(HAVE_LVX_BUG_FMIN && flag_signaling_nans))"
  "fminn<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "smax<mode>3"
  [(set (match_operand:V128F 0 "register_operand" "=r,r")
        (smax:V128F (match_operand:V128F 1 "register_operand" "r,r")
                    (match_operand:V128F 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2 && (HAVE_LVX_MAX_<MODE> && !(HAVE_LVX_BUG_FMAX && flag_signaling_nans))"
  "@
   fmaxn<suffix> %0 = %1, %2
   fmaxn<suffix> %0 = %1, %W2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "*smax<mode>3_s1"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (smax:V128F (vec_duplicate:V128F (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V128F 2 "register_operand" "r")))]
  "LVX_2 && (HAVE_LVX_MAX_<MODE> && !(HAVE_LVX_BUG_FMAX && flag_signaling_nans))"
  "fmaxn<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "*smax<mode>3_s2"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (smax:V128F (match_operand:V128F 1 "register_operand" "r")
                    (vec_duplicate:V128F (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2 && (HAVE_LVX_MAX_<MODE> && !(HAVE_LVX_BUG_FMAX && flag_signaling_nans))"
  "fmaxn<suffix> %0 = %1, %2"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "neg<mode>2"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (neg:V128F (match_operand:V128F 1 "register_operand" "r")))]
  "LVX_2"
  "fneg<suffix> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)

(define_insn "abs<mode>2"
  [(set (match_operand:V128F 0 "register_operand" "=r")
        (abs:V128F (match_operand:V128F 1 "register_operand" "r")))]
  "LVX_2"
  "fabs<suffix> %0 = %1"
  [(set_attr "type" "alu")
   (set_attr "issue" "lite")
   (set_attr "length" "4")]
)


;; V128G (V4SF V2DF)

; The 128-bit float<->int conversions in one instruction each: FIXEDWQ/FLOATWQ
;; for V4SF<->V4SI, FIXEDDP/FLOATDP for V2DF<->V2DI.  These used to split into
;; two 64-bit halves (a pair of fixedwp), before the ISA's word-quadruple and
;; double-pair forms were used; the wider 256/512-bit conversions below still
;; split, but now into these single 128-bit instructions, which the VLIW can
;; dual-issue in one bundle.
(define_mode_attr fcvt128 [(V4SF "wq") (V2DF "dp")])

(define_insn "float<mask><mode>2"
  [(set (match_operand:V128G 0 "register_operand" "=r")
        (float:V128G (match_operand:<MASK> 1 "register_operand" "r")))]
  "LVX_2"
  "float<fcvt128>.rn %0 = %1"
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "floatuns<mask><mode>2"
  [(set (match_operand:V128G 0 "register_operand" "=r")
        (unsigned_float:V128G (match_operand:<MASK> 1 "register_operand" "r")))]
  "LVX_2"
  "floatu<fcvt128>.rn %0 = %1"
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "fix_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "=r")
        (fix:<MASK> (match_operand:V128G 1 "register_operand" "r")))]
  "LVX_2"
  "fixed<fcvt128>.rz %0 = %1"
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

(define_insn "fixuns_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "=r")
        (unsigned_fix:<MASK> (match_operand:V128G 1 "register_operand" "r")))]
  "LVX_2"
  "fixedu<fcvt128>.rz %0 = %1"
  [(set_attr "type" "fcvt")
   (set_attr "issue" "lite")]
)

;;(define_insn "truncv8sfv8hf2"
;;  [(set (match_operand:V8HF 0 "register_operand" "=r")
;;        (float_truncate:V8HF (match_operand:V8SF 1 "register_operand" "r")))]
;;  ""
;;  "fnarrowwhq %x0 = %x1\n\tfnarrowwhq %y0 = %y1"
;;  [(set_attr "type" "alu")
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
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  [(set (match_operand:V2DF 0 "register_operand" "=r,r")
        (plus:V2DF (match_operand:V2DF 1 "register_operand" "r,r")
                   (match_operand:V2DF 2 "reg_or_const_zero_operand" "r,SZ0")))]
  "LVX_2"
  "@
   fadddp %0 = %1, %2
   fadddp %0 = %1, 0"
  "!HAVE_LVX_PLUS_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (plus:DF (subreg:DF (match_dup 1) 0)
                 (subreg:DF (match_dup 2) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (plus:DF (subreg:DF (match_dup 1) 8)
                 (subreg:DF (match_dup 2) 8)))]
  ""
  [(set_attr "type" "fmuld")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn_and_split "subv2df3"
  [(set (match_operand:V2DF 0 "register_operand" "=r,r")
        (minus:V2DF (match_operand:V2DF 1 "reg_or_const_zero_operand" "r,SZ0")
                    (match_operand:V2DF 2 "register_operand" "r,r")))]
  "LVX_2"
  "@
   fsbfdp %0 = %2, %1
   fsbfdp %0 = %2, 0"
  "!HAVE_LVX_MINUS_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (minus:DF (subreg:DF (match_dup 1) 0)
                  (subreg:DF (match_dup 2) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (minus:DF (subreg:DF (match_dup 1) 8)
                  (subreg:DF (match_dup 2) 8)))]
  ""
  [(set_attr "type" "fmuld")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn_and_split "mulv2df3"
  [(set (match_operand:V2DF 0 "register_operand" "=r,r")
        (mult:V2DF (match_operand:V2DF 1 "register_operand" "r,r")
                   (match_operand:V2DF 2 "reg_or_const_zero_operand" "r,SZ0")))]
  "LVX_2"
  "@
   fmuldp %0 = %1, %2
   fmuldp %0 = %1, 0"
  "!HAVE_LVX_MULT_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (mult:DF (subreg:DF (match_dup 1) 0)
                 (subreg:DF (match_dup 2) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (mult:DF (subreg:DF (match_dup 1) 8)
                 (subreg:DF (match_dup 2) 8)))]
  ""
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn_and_split "fmav2df4"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (fma:V2DF (match_operand:V2DF 1 "register_operand" "r")
                  (match_operand:V2DF 2 "register_operand" "r")
                  (match_operand:V2DF 3 "register_operand" "0")))]
  "LVX_2"
  "ffmadp %0 = %1, %2"
  "!HAVE_LVX_FMA_V2DF_V2DF_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (fma:DF  (subreg:DF (match_dup 1) 0)
                 (subreg:DF (match_dup 2) 0)
                 (subreg:DF (match_dup 3) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (fma:DF  (subreg:DF (match_dup 1) 8)
                 (subreg:DF (match_dup 2) 8)
                 (subreg:DF (match_dup 3) 8)))]
  ""
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_insn_and_split "fnmav2df4"
  [(set (match_operand:V2DF 0 "register_operand" "=r")
        (fma:V2DF (neg:V2DF (match_operand:V2DF 1 "register_operand" "r"))
                  (match_operand:V2DF 2 "register_operand" "r")
                  (match_operand:V2DF 3 "register_operand" "0")))]
  "LVX_2"
  "ffmsdp %0 = %1, %2"
  "!HAVE_LVX_FMS_V2DF_V2DF_V2DF && reload_completed"
  [(set (subreg:DF (match_dup 0) 0)
        (fma:DF  (neg:DF (subreg:DF (match_dup 1) 0))
                 (subreg:DF (match_dup 2) 0)
                 (subreg:DF (match_dup 3) 0)))
   (set (subreg:DF (match_dup 0) 8)
        (fma:DF  (neg:DF (subreg:DF (match_dup 1) 8))
                 (subreg:DF (match_dup 2) 8)
                 (subreg:DF (match_dup 3) 8)))]
  ""
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

;; S256F (V16HF V8SF)

(define_insn "fma<mode>4"
  [(set (match_operand:S256F 0 "register_operand" "=r")
        (fma:S256F (match_operand:S256F 1 "register_operand" "r")
                   (match_operand:S256F 2 "register_operand" "r")
                   (match_operand:S256F 3 "register_operand" "0")))]
  "LVX_2"
  "#"
)

(define_split
  [(set (match_operand:S256F 0 "register_operand" "")
        (fma:S256F (match_operand:S256F 1 "register_operand" "")
                   (match_operand:S256F 2 "register_operand" "")
                   (match_operand:S256F 3 "register_operand" "")))]
  "LVX_2 && (!HAVE_LVX_FMA_<HALF>_<HALF>_<HALF> && reload_completed)"
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
  "LVX_2 && (HAVE_LVX_FMA_<HALF>_<HALF>_<HALF> && reload_completed)"
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
  "LVX_2"
  "#"
)

(define_split
  [(set (match_operand:S256F 0 "register_operand" "")
        (fma:S256F (neg:S256F (match_operand:S256F 1 "register_operand" ""))
                   (match_operand:S256F 2 "register_operand" "")
                   (match_operand:S256F 3 "register_operand" "")))]
  "LVX_2 && (!HAVE_LVX_FMS_<HALF>_<HALF>_<HALF> && reload_completed)"
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
  "LVX_2 && (HAVE_LVX_FMS_<HALF>_<HALF>_<HALF> && reload_completed)"
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

;; FNARROWWHO and FNARROWDWQ write 128 bits, which is the widest narrowing
;; the ISA has, so a 256-bit result takes two of them -- one per 128-bit half
;; of the destination, reading the corresponding 256-bit half of the source.
;; This used to split into 64-bit <QUART> chunks, whose sub-patterns went with
;; 64-bit SIMD, exactly as the S128F case above did.

(define_expand "trunc<wide><mode>2"
  [(set (match_operand:S256F 0 "register_operand" "")
        (float_truncate:S256F (match_operand:<WIDE> 1 "register_operand" "")))]
  "LVX_2"
  {
    /* 128 bits is the widest narrowing the ISA has, so a 256-bit result is
       two of them -- one per 128-bit half of the destination, reading the
       corresponding 256-bit half of the source.  With the vector form gated
       off it is one scalar FNARROW per lane, as in the S128F case above, and
       for the same reason: leaving the optab out miscompiles rather than
       falling back.  */
    machine_mode dm = GET_MODE_INNER (<MODE>mode);
    machine_mode sm = GET_MODE_INNER (<WIDE>mode);
    rtx dst = gen_reg_rtx (<MODE>mode);
    rtx src = force_reg (<WIDE>mode, operands[1]);

    if (HAVE_LVX_FP_NARROW_VECTOR)
      {
	for (int i = 0; i < 2; i++)
	  {
	    rtx d = simplify_gen_subreg (<HALF>mode, dst, <MODE>mode, i * 16);
	    rtx s = simplify_gen_subreg (<HWIDE>mode, src, <WIDE>mode, i * 32);
	    gcc_assert (d && s);
	    emit_insn (gen_lvx_fnarrow<narrowx> (d, s));
	  }
	emit_move_insn (operands[0], dst);
	DONE;
      }

    rtx smem = assign_stack_temp (<WIDE>mode, GET_MODE_SIZE (<WIDE>mode));
    rtx dmem = assign_stack_temp (<MODE>mode, GET_MODE_SIZE (<MODE>mode));

    emit_move_insn (smem, src);
    for (int i = 0; i < GET_MODE_NUNITS (<MODE>mode); i++)
      {
	rtx t = gen_reg_rtx (sm);
	rtx r = gen_reg_rtx (dm);
	emit_move_insn (t, adjust_address (smem, sm, i * GET_MODE_SIZE (sm)));
	emit_insn (gen_rtx_SET (r, gen_rtx_FLOAT_TRUNCATE (dm, t)));
	emit_move_insn (adjust_address (dmem, dm, i * GET_MODE_SIZE (dm)), r);
      }
    emit_move_insn (operands[0], dmem);
    DONE;
  }
)

(define_expand "extend<mode><wide>2"
  [(set (match_operand:<WIDE> 0 "register_operand" "")
        (float_extend:<WIDE> (match_operand:S256F 1 "register_operand" "")))]
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


;; V256F (V16HF V8SF V4D)

(define_insn_and_split "smin<mode>3"
  [(set (match_operand:V256F 0 "register_operand" "=r")
        (smin:V256F (match_operand:V256F 1 "register_operand" "r")
                    (match_operand:V256F 2 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (smin:<HALF> (subreg:<HALF> (match_dup 1) 0)
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (smin:<HALF> (subreg:<HALF> (match_dup 1) 16)
                     (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "*smin<mode>3_s1"
  [(set (match_operand:V256F 0 "register_operand" "=&r")
        (smin:V256F (vec_duplicate:V256F (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256F 2 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (smin:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (smin:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                     (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "*smin<mode>3_s2"
  [(set (match_operand:V256F 0 "register_operand" "=&r")
        (smin:V256F (match_operand:V256F 1 "register_operand" "r")
                    (vec_duplicate:V256F (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (smin:<HALF> (subreg:<HALF> (match_dup 1) 0)
                     (vec_duplicate:<HALF> (match_dup 2))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (smin:<HALF> (subreg:<HALF> (match_dup 1) 16)
                     (vec_duplicate:<HALF> (match_dup 2))))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "smax<mode>3"
  [(set (match_operand:V256F 0 "register_operand" "=r")
        (smax:V256F (match_operand:V256F 1 "register_operand" "r")
                    (match_operand:V256F 2 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (smax:<HALF> (subreg:<HALF> (match_dup 1) 0)
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (smax:<HALF> (subreg:<HALF> (match_dup 1) 16)
                     (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "*smax<mode>3_s1"
  [(set (match_operand:V256F 0 "register_operand" "=&r")
        (smax:V256F (vec_duplicate:V256F (match_operand:<CHUNK> 1 "nonmemory_operand" "r"))
                    (match_operand:V256F 2 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (smax:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                     (subreg:<HALF> (match_dup 2) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (smax:<HALF> (vec_duplicate:<HALF> (match_dup 1))
                     (subreg:<HALF> (match_dup 2) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "*smax<mode>3_s2"
  [(set (match_operand:V256F 0 "register_operand" "=&r")
        (smax:V256F (match_operand:V256F 1 "register_operand" "r")
                    (vec_duplicate:V256F (match_operand:<CHUNK> 2 "nonmemory_operand" "r"))))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (smax:<HALF> (subreg:<HALF> (match_dup 1) 0)
                     (vec_duplicate:<HALF> (match_dup 2))))
   (set (subreg:<HALF> (match_dup 0) 16)
        (smax:<HALF> (subreg:<HALF> (match_dup 1) 16)
                     (vec_duplicate:<HALF> (match_dup 2))))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "neg<mode>2"
  [(set (match_operand:V256F 0 "register_operand" "=r")
        (neg:V256F (match_operand:V256F 1 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (neg:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (neg:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)

(define_insn_and_split "abs<mode>2"
  [(set (match_operand:V256F 0 "register_operand" "=r")
        (abs:V256F (match_operand:V256F 1 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (abs:<HALF> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (abs:<HALF> (subreg:<HALF> (match_dup 1) 16)))]
  ""
  [(set_attr "type" "alu")
   (set_attr "issue" "lite2")]
)


;; V256G (V8SF V4DF)

;; The 256-bit conversions split into two 128-bit halves -- one fixedwq/
;; floatwq (V4SF) or fixeddp/floatdp (V2DF) per half -- which the VLIW dual-
;; issues.  They used to split into four 64-bit pieces (fixedwp), before the
;; 128-bit word-quadruple and double-pair forms were used.
(define_insn_and_split "float<mask><mode>2"
  [(set (match_operand:V256G 0 "register_operand" "=r")
        (float:V256G (match_operand:<MASK> 1 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (float:<HALF> (subreg:<HMASK> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (float:<HALF> (subreg:<HMASK> (match_dup 1) 16)))]
  ""
)
(define_insn_and_split "floatuns<mask><mode>2"
  [(set (match_operand:V256G 0 "register_operand" "=r")
        (unsigned_float:V256G (match_operand:<MASK> 1 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HALF> (match_dup 0) 0)
        (unsigned_float:<HALF> (subreg:<HMASK> (match_dup 1) 0)))
   (set (subreg:<HALF> (match_dup 0) 16)
        (unsigned_float:<HALF> (subreg:<HMASK> (match_dup 1) 16)))]
  ""
)
(define_insn_and_split "fix_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "=r")
        (fix:<MASK> (match_operand:V256G 1 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HMASK> (match_dup 0) 0)
        (fix:<HMASK> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HMASK> (match_dup 0) 16)
        (fix:<HMASK> (subreg:<HALF> (match_dup 1) 16)))]
  ""
)
(define_insn_and_split "fixuns_trunc<mode><mask>2"
  [(set (match_operand:<MASK> 0 "register_operand" "=r")
        (unsigned_fix:<MASK> (match_operand:V256G 1 "register_operand" "r")))]
  "LVX_2"
  "#"
  "reload_completed"
  [(set (subreg:<HMASK> (match_dup 0) 0)
        (unsigned_fix:<HMASK> (subreg:<HALF> (match_dup 1) 0)))
   (set (subreg:<HMASK> (match_dup 0) 16)
        (unsigned_fix:<HMASK> (subreg:<HALF> (match_dup 1) 16)))]
  ""
)


;; V16HF

(define_insn "addv16hf3"
  [(set (match_operand:V16HF 0 "register_operand" "=r,r")
        (plus:V16HF (match_operand:V16HF 1 "register_operand" "r,r")
                   (match_operand:V16HF 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "#"
)

(define_split
  [(set (match_operand:V16HF 0 "register_operand" "")
        (plus:V16HF (match_operand:V16HF 1 "register_operand" "")
                    (match_operand:V16HF 2 "reg_or_splat32_operand" "")))]
  "LVX_2 && (HAVE_LVX_PLUS_V8HF && reload_completed)"
  [(set (subreg:V8HF (match_dup 0) 0)
        (plus:V8HF (match_dup 3) (match_dup 5)))
   (set (subreg:V8HF (match_dup 0) 16)
        (plus:V8HF (match_dup 4) (match_dup 6)))]
  {
    /* A splatted constant narrows to the half's own constant, which is
       what the 128-bit immediate form takes; a subreg written into the
       replacement RTL would not fold and would not be recognised.  */
    for (int i = 0; i < 2; i++)
      {
        operands[3 + i] = simplify_gen_subreg (V8HFmode, operands[1],
                                               V16HFmode, 16 * i);
        operands[5 + i] = simplify_gen_subreg (V8HFmode, operands[2],
                                               V16HFmode, 16 * i);
        if (!operands[3 + i] || !operands[5 + i])
          FAIL;
      }
  }
)

(define_insn "subv16hf3"
  [(set (match_operand:V16HF 0 "register_operand" "=r,r")
        (minus:V16HF (match_operand:V16HF 1 "reg_or_splat32_operand" "r,SXW")
                    (match_operand:V16HF 2 "register_operand" "r,r")))]
  "LVX_2"
  "#"
)

(define_split
  [(set (match_operand:V16HF 0 "register_operand" "")
        (minus:V16HF (match_operand:V16HF 1 "reg_or_splat32_operand" "")
                    (match_operand:V16HF 2 "register_operand" "")))]
  "LVX_2 && (HAVE_LVX_MINUS_V8HF && reload_completed)"
  [(set (subreg:V8HF (match_dup 0) 0)
        (minus:V8HF (match_dup 3) (match_dup 5)))
   (set (subreg:V8HF (match_dup 0) 16)
        (minus:V8HF (match_dup 4) (match_dup 6)))]
  {
    /* A splatted constant narrows to the half's own constant, which is
       what the 128-bit immediate form takes; a subreg written into the
       replacement RTL would not fold and would not be recognised.  */
    for (int i = 0; i < 2; i++)
      {
        operands[3 + i] = simplify_gen_subreg (V8HFmode, operands[1],
                                               V16HFmode, 16 * i);
        operands[5 + i] = simplify_gen_subreg (V8HFmode, operands[2],
                                               V16HFmode, 16 * i);
        if (!operands[3 + i] || !operands[5 + i])
          FAIL;
      }
  }
)

(define_insn "mulv16hf3"
  [(set (match_operand:V16HF 0 "register_operand" "=r,r")
        (mult:V16HF (match_operand:V16HF 1 "register_operand" "r,r")
                   (match_operand:V16HF 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "#"
)

(define_split
  [(set (match_operand:V16HF 0 "register_operand" "")
        (mult:V16HF (match_operand:V16HF 1 "register_operand" "")
                    (match_operand:V16HF 2 "reg_or_splat32_operand" "")))]
  "LVX_2 && (HAVE_LVX_MULT_V8HF && reload_completed)"
  [(set (subreg:V8HF (match_dup 0) 0)
        (mult:V8HF (match_dup 3) (match_dup 5)))
   (set (subreg:V8HF (match_dup 0) 16)
        (mult:V8HF (match_dup 4) (match_dup 6)))]
  {
    /* A splatted constant narrows to the half's own constant, which is
       what the 128-bit immediate form takes; a subreg written into the
       replacement RTL would not fold and would not be recognised.  */
    for (int i = 0; i < 2; i++)
      {
        operands[3 + i] = simplify_gen_subreg (V8HFmode, operands[1],
                                               V16HFmode, 16 * i);
        operands[5 + i] = simplify_gen_subreg (V8HFmode, operands[2],
                                               V16HFmode, 16 * i);
        if (!operands[3 + i] || !operands[5 + i])
          FAIL;
      }
  }
)


;; V8SF

(define_insn "addv8sf3"
  [(set (match_operand:V8SF 0 "register_operand" "=r,r")
        (plus:V8SF (match_operand:V8SF 1 "register_operand" "r,r")
                   (match_operand:V8SF 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "#"
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (plus:V8SF (match_operand:V8SF 1 "register_operand" "")
                    (match_operand:V8SF 2 "reg_or_splat32_operand" "")))]
  "LVX_2 && (HAVE_LVX_PLUS_V4SF && reload_completed)"
  [(set (subreg:V4SF (match_dup 0) 0)
        (plus:V4SF (match_dup 3) (match_dup 5)))
   (set (subreg:V4SF (match_dup 0) 16)
        (plus:V4SF (match_dup 4) (match_dup 6)))]
  {
    /* A splatted constant narrows to the half's own constant, which is
       what the 128-bit immediate form takes; a subreg written into the
       replacement RTL would not fold and would not be recognised.  */
    for (int i = 0; i < 2; i++)
      {
        operands[3 + i] = simplify_gen_subreg (V4SFmode, operands[1],
                                               V8SFmode, 16 * i);
        operands[5 + i] = simplify_gen_subreg (V4SFmode, operands[2],
                                               V8SFmode, 16 * i);
        if (!operands[3 + i] || !operands[5 + i])
          FAIL;
      }
  }
)

(define_insn "subv8sf3"
  [(set (match_operand:V8SF 0 "register_operand" "=r,r")
        (minus:V8SF (match_operand:V8SF 1 "reg_or_splat32_operand" "r,SXW")
                    (match_operand:V8SF 2 "register_operand" "r,r")))]
  "LVX_2"
  "#"
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (minus:V8SF (match_operand:V8SF 1 "reg_or_splat32_operand" "")
                    (match_operand:V8SF 2 "register_operand" "")))]
  "LVX_2 && (HAVE_LVX_MINUS_V4SF && reload_completed)"
  [(set (subreg:V4SF (match_dup 0) 0)
        (minus:V4SF (match_dup 3) (match_dup 5)))
   (set (subreg:V4SF (match_dup 0) 16)
        (minus:V4SF (match_dup 4) (match_dup 6)))]
  {
    /* A splatted constant narrows to the half's own constant, which is
       what the 128-bit immediate form takes; a subreg written into the
       replacement RTL would not fold and would not be recognised.  */
    for (int i = 0; i < 2; i++)
      {
        operands[3 + i] = simplify_gen_subreg (V4SFmode, operands[1],
                                               V8SFmode, 16 * i);
        operands[5 + i] = simplify_gen_subreg (V4SFmode, operands[2],
                                               V8SFmode, 16 * i);
        if (!operands[3 + i] || !operands[5 + i])
          FAIL;
      }
  }
)

(define_insn "mulv8sf3"
  [(set (match_operand:V8SF 0 "register_operand" "=r,r")
        (mult:V8SF (match_operand:V8SF 1 "register_operand" "r,r")
                   (match_operand:V8SF 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "#"
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_split
  [(set (match_operand:V8SF 0 "register_operand" "")
        (mult:V8SF (match_operand:V8SF 1 "register_operand" "")
                    (match_operand:V8SF 2 "reg_or_splat32_operand" "")))]
  "LVX_2 && (HAVE_LVX_MULT_V4SF && reload_completed)"
  [(set (subreg:V4SF (match_dup 0) 0)
        (mult:V4SF (match_dup 3) (match_dup 5)))
   (set (subreg:V4SF (match_dup 0) 16)
        (mult:V4SF (match_dup 4) (match_dup 6)))]
  {
    /* A splatted constant narrows to the half's own constant, which is
       what the 128-bit immediate form takes; a subreg written into the
       replacement RTL would not fold and would not be recognised.  */
    for (int i = 0; i < 2; i++)
      {
        operands[3 + i] = simplify_gen_subreg (V4SFmode, operands[1],
                                               V8SFmode, 16 * i);
        operands[5 + i] = simplify_gen_subreg (V4SFmode, operands[2],
                                               V8SFmode, 16 * i);
        if (!operands[3 + i] || !operands[5 + i])
          FAIL;
      }
  }
)

(define_expand "floatv8hiv8sf2"
  [(set (match_operand:V8SF 0 "register_operand" "")
        (float:V8SF (match_operand:V8HI 1 "register_operand" "")))
   (clobber (match_dup 2))]
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
  {
    operands[2] = gen_reg_rtx (V8SImode);
    emit_insn (gen_fixuns_truncv8sfv8si2 (operands[2], operands[1]));
    emit_insn (gen_lvx_truncwho (operands[0], operands[2]));
    DONE;
  }
)


;; V4DF

(define_insn "addv4df3"
  [(set (match_operand:V4DF 0 "register_operand" "=r,r")
        (plus:V4DF (match_operand:V4DF 1 "register_operand" "r,r")
                   (match_operand:V4DF 2 "reg_or_const_zero_operand" "r,SZ0")))]
  "LVX_2"
  "#"
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (plus:V4DF (match_operand:V4DF 1 "register_operand" "")
                   (match_operand:V4DF 2 "reg_or_const_zero_operand" "")))]
  "LVX_2 && (HAVE_LVX_PLUS_V2DF && reload_completed)"
  [(set (subreg:V2DF (match_dup 0) 0)
        (plus:V2DF (match_dup 3) (match_dup 5)))
   (set (subreg:V2DF (match_dup 0) 16)
        (plus:V2DF (match_dup 4) (match_dup 6)))]
  {
    /* Each half explicitly: a subreg of a CONST_VECTOR written into the
       replacement RTL does not fold, and is not recognised.  */
    for (int i = 0; i < 2; i++)
      {
        operands[3 + i] = simplify_gen_subreg (V2DFmode, operands[1],
                                               V4DFmode, 16 * i);
        operands[5 + i] = simplify_gen_subreg (V2DFmode, operands[2],
                                               V4DFmode, 16 * i);
        if (!operands[3 + i] || !operands[5 + i])
          FAIL;
      }
  }
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (plus:V4DF (match_operand:V4DF 1 "register_operand" "")
                   (match_operand:V4DF 2 "register_operand" "")))]
  "LVX_2 && (!HAVE_LVX_PLUS_V2DF && reload_completed)"
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
  [(set (match_operand:V4DF 0 "register_operand" "=r,r")
        (minus:V4DF (match_operand:V4DF 1 "reg_or_const_zero_operand" "r,SZ0")
                    (match_operand:V4DF 2 "register_operand" "r,r")))]
  "LVX_2"
  "#"
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (minus:V4DF (match_operand:V4DF 1 "reg_or_const_zero_operand" "")
                    (match_operand:V4DF 2 "register_operand" "")))]
  "LVX_2 && (HAVE_LVX_MINUS_V2DF && reload_completed)"
  [(set (subreg:V2DF (match_dup 0) 0)
        (minus:V2DF (match_dup 3) (match_dup 5)))
   (set (subreg:V2DF (match_dup 0) 16)
        (minus:V2DF (match_dup 4) (match_dup 6)))]
  {
    /* Each half explicitly: a subreg of a CONST_VECTOR written into the
       replacement RTL does not fold, and is not recognised.  */
    for (int i = 0; i < 2; i++)
      {
        operands[3 + i] = simplify_gen_subreg (V2DFmode, operands[1],
                                               V4DFmode, 16 * i);
        operands[5 + i] = simplify_gen_subreg (V2DFmode, operands[2],
                                               V4DFmode, 16 * i);
        if (!operands[3 + i] || !operands[5 + i])
          FAIL;
      }
  }
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (minus:V4DF (match_operand:V4DF 1 "register_operand" "")
                    (match_operand:V4DF 2 "register_operand" "")))]
  "LVX_2 && (!HAVE_LVX_MINUS_V2DF && reload_completed)"
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
  "LVX_2"
  "#"
  "!HAVE_LVX_MULT_V2DF && reload_completed"
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
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (mult:V4DF (match_operand:V4DF 1 "register_operand" "")
                   (match_operand:V4DF 2 "register_operand" "")))]
  "LVX_2 && (HAVE_LVX_MULT_V2DF && reload_completed)"
  [(set (subreg:V2DF (match_dup 0) 0)
        (mult:V2DF (subreg:V2DF (match_dup 1) 0)
                   (subreg:V2DF (match_dup 2) 0)))
   (set (subreg:V2DF (match_dup 0) 16)
        (mult:V2DF (subreg:V2DF (match_dup 1) 16)
                   (subreg:V2DF (match_dup 2) 16)))]
  ""
)

(define_insn_and_split "fmav4df4"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (fma:V4DF (match_operand:V4DF 1 "register_operand" "r")
                  (match_operand:V4DF 2 "register_operand" "r")
                  (match_operand:V4DF 3 "register_operand" "0")))]
  "LVX_2"
  "#"
  "!HAVE_LVX_FMA_V2DF_V2DF_V2DF && reload_completed"
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
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (fma:V4DF (match_operand:V4DF 1 "register_operand" "")
                  (match_operand:V4DF 2 "register_operand" "")
                  (match_operand:V4DF 3 "register_operand" "")))]
  "LVX_2 && (HAVE_LVX_FMA_V2DF_V2DF_V2DF && reload_completed)"
  [(set (subreg:V2DF (match_dup 0) 0)
        (fma:V2DF (subreg:V2DF (match_dup 1) 0)
                  (subreg:V2DF (match_dup 2) 0)
                  (subreg:V2DF (match_dup 3) 0)))
   (set (subreg:V2DF (match_dup 0) 16)
        (fma:V2DF (subreg:V2DF (match_dup 1) 16)
                  (subreg:V2DF (match_dup 2) 16)
                  (subreg:V2DF (match_dup 3) 16)))]
  ""
)

(define_insn_and_split "fnmav4df4"
  [(set (match_operand:V4DF 0 "register_operand" "=r")
        (fma:V4DF (neg:V4DF (match_operand:V4DF 1 "register_operand" "r"))
                  (match_operand:V4DF 2 "register_operand" "r")
                  (match_operand:V4DF 3 "register_operand" "0")))]
  "LVX_2"
  "#"
  "!HAVE_LVX_FMS_V2DF_V2DF_V2DF && reload_completed"
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
  [(set_attr "type" "fmaddd")
   (set_attr "issue" "lite")]
)

(define_split
  [(set (match_operand:V4DF 0 "register_operand" "")
        (fma:V4DF (neg:V4DF (match_operand:V4DF 1 "register_operand" ""))
                  (match_operand:V4DF 2 "register_operand" "")
                  (match_operand:V4DF 3 "register_operand" "")))]
  "LVX_2 && (HAVE_LVX_FMS_V2DF_V2DF_V2DF && reload_completed)"
  [(set (subreg:V2DF (match_dup 0) 0)
        (fma:V2DF (neg:V2DF (subreg:V2DF (match_dup 1) 0))
                  (subreg:V2DF (match_dup 2) 0)
                  (subreg:V2DF (match_dup 3) 0)))
   (set (subreg:V2DF (match_dup 0) 16)
        (fma:V2DF (neg:V2DF (subreg:V2DF (match_dup 1) 16))
                  (subreg:V2DF (match_dup 2) 16)
                  (subreg:V2DF (match_dup 3) 16)))]
  ""
)

(define_expand "vec_unpacks_hi_<packi>"
  [(set (match_operand:UNPACKI 0 "register_operand")
        (match_operand:<PACKI> 1 "register_operand"))]
  "LVX_2"
  {
    lvx_expand_unpack (operands[0], operands[1], /*signed_p*/1, /*hi_p*/1);
    DONE;
  }
)

(define_expand "vec_unpacks_lo_<packi>"
  [(set (match_operand:UNPACKI 0 "register_operand")
        (match_operand:<PACKI> 1 "register_operand"))]
  "LVX_2"
  {
    lvx_expand_unpack (operands[0], operands[1], /*signed_p*/1, /*hi_p*/0);
    DONE;
  }
)

(define_expand "vec_unpacku_hi_<packi>"
  [(match_operand:UNPACKI 0 "register_operand")
   (match_operand:<PACKI> 1 "register_operand")]
  "LVX_2"
  {
    lvx_expand_unpack (operands[0], operands[1], /*signed_p*/0, /*hi_p*/1);
    DONE;
  }
)

(define_expand "vec_unpacku_lo_<packi>"
  [(match_operand:UNPACKI 0 "register_operand")
   (match_operand:<PACKI> 1 "register_operand")]
  "LVX_2"
  {
    lvx_expand_unpack (operands[0], operands[1], /*signed_p*/0, /*hi_p*/0);
    DONE;
  }
)


;; V512G

(define_expand "float<mask><mode>2"
  [(set (match_operand:V512G 0 "register_operand" "")
        (float:V512G (match_operand:<MASK> 1 "register_operand" "")))]
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  "LVX_2"
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
  [(set (match_operand:V8HF 0 "register_operand" "=r,r")
        (plus:V8HF (match_operand:V8HF 1 "register_operand" "r,r")
                   (match_operand:V8HF 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   faddho %0 = %1, %2
   faddho %0 = %1, %W2"
  [(set_attr "type" "fmadds")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "subv8hf3"
  [(set (match_operand:V8HF 0 "register_operand" "=r,r")
        (minus:V8HF (match_operand:V8HF 1 "reg_or_splat32_operand" "r,SXW")
                    (match_operand:V8HF 2 "register_operand" "r,r")))]
  "LVX_2"
  "@
   fsbfho %0 = %2, %1
   fsbfho %0 = %2, %W1"
  [(set_attr "type" "fmuls")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "mulv8hf3"
  [(set (match_operand:V8HF 0 "register_operand" "=r,r")
        (mult:V8HF (match_operand:V8HF 1 "register_operand" "r,r")
                   (match_operand:V8HF 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   fmulho %0 = %1, %2
   fmulho %0 = %1, %W2"
  [(set_attr "type" "fmuls")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

;; -------------------------------------------------------------------------
;; V4HI and V2SI arithmetic, done on the 128-bit unit
;;
;; The narrowest packed instruction is 128-bit, so a 64-bit vector has no
;; instruction of its own and the middle end lowers it.  What the lowering
;; costs depends entirely on the operation: AND, IOR and XOR are exactly the
;; 64-bit container's own ANDD/IORD/EORD, one instruction and nothing to
;; improve; ADD and NEG on 16-bit lanes get GCC's carry-suppression trick,
;; five to seven instructions; but MUL, the shifts and the comparisons are
;; lowered lane by lane -- seventeen instructions for a V4HI multiply,
;; fourteen for a shift, twenty-three for a compare.
;;
;; Those are the ones widened here: the operand goes in the low half of a
;; 128-bit pair, the packed instruction runs on all four or eight lanes, and
;; the low half is the result.  The upper lanes are left undefined on
;; purpose -- integer arithmetic raises nothing, so what they compute cannot
;; be observed, and zeroing them (as the V2SF patterns must, floating point
;; writing the CS flags) would cost two instructions for nothing.
;;
;; An operation NOT widened here keeps whatever the middle end does with it,
;; which for the bitwise ops is already one instruction: adding a pattern is
;; a strict improvement and leaving one out costs nothing.  That is why this
;; list is the measured-bad operations and not the whole surface.
;; -------------------------------------------------------------------------

(define_mode_iterator V64I [V8QI V4HI V2SI])


(define_code_iterator V64I_SHIFT [ashift ashiftrt lshiftrt])
(define_code_attr v64i_shift [(ashift "ashl") (ashiftrt "ashr") (lshiftrt "lshr")])

;; The shifts take their count as one SImode value, exactly as the 128-bit
;; sll<suffix>/sra<suffix>/srl<suffix> patterns do -- it is a shift amount,
;; not a lane.  A pattern's condition must stay a compile-time expression, so
;; the iterator says statically what is available: every widened mode here
;; has its 128-bit shift (SLLBX/SLLHO/SLLWQ and the arithmetic and logical
;; right shifts beside them).
;; (Asking optab_handler in the condition does not work -- it would query the
;; table init_all_optabs is filling when it evaluates the condition, and the
;; answer comes back no, disabling the pattern for good.)
(define_expand "<v64i_shift><mode>3"
  [(set (match_operand:V64I 0 "register_operand")
        (V64I_SHIFT:V64I (match_operand:V64I 1 "register_operand")
                         (match_operand:SI 2 "reg_shift_operand")))]
  "LVX_2"
  {
    lvx_expand_widen64 (<CODE>, <DMODE>mode, operands);
    DONE;
  }
)

(define_code_iterator V64I_MINMAX [plus minus smin smax umin umax])
(define_code_attr v64i_minmax [(plus "add") (minus "sub")
                               (smin "smin") (smax "smax")
                               (umin "umin") (umax "umax")])

;; add/sub and min/max are all written over V128J (V8HI V4SI V2DI), which
;; covers both widened modes.  Widening an add is worth it at 32-bit lanes,
;; where the middle end extracts lanes; at 16-bit and 8-bit it has a
;; carry-suppression trick that is already about this cheap, and taking the
;; pattern means taking it everywhere -- measured below.
(define_expand "<v64i_minmax><mode>3"
  [(set (match_operand:V64I 0 "register_operand")
        (V64I_MINMAX:V64I (match_operand:V64I 1 "register_operand")
                          (match_operand:V64I 2 "register_operand")))]
  "LVX_2"
  {
    lvx_expand_widen64 (<CODE>, <DMODE>mode, operands);
    DONE;
  }
)

;; MUL reaches every width, but not equally.  V4HI and V2SI widen into one
;; packed multiply each; V8QI widens into the V16QI one.  (Until 2026-09-28
;; there was no MULBX at all and the V16QI multiply was itself synthesised from
;; two MULHO and a mask -- MULBX exists now, so that synthesis is gone and this
;; path is one instruction shorter.)  That is still worth doing: eleven
;; instructions against the thirty-three the middle end's lowering takes.
(define_expand "mul<mode>3"
  [(set (match_operand:V64I 0 "register_operand")
        (mult:V64I (match_operand:V64I 1 "register_operand")
                   (match_operand:V64I 2 "register_operand")))]
  "LVX_2"
  {
    lvx_expand_widen64 (MULT, <DMODE>mode, operands);
    DONE;
  }
)

;; -------------------------------------------------------------------------
;; 64-bit FP arithmetic (V2SF, V4HF), done on the 128-bit unit
;;
;; V2SF is two floats in one GPR -- 64 bits, the `float complex' shape the
;; FMULWC family works on, and the one sub-128-bit vector mode
;; lvx_vector_mode_supported_p claims on lvx-1 (see the 128-bit lower bound
;; there).  V4HF is the same shape one lane size down, four halves in a GPR.
;; Neither has a packed instruction of its own: the narrowest FP lanes are
;; FADDWQ/FSBFWQ/FMULWQ on a 4x32 pair and FADDHO/FSBFHO/FMULHO on an 8x16
;; one.  Without a pattern the middle end lowers lane by lane -- for V2SF
;; four EXTFZD, two scalar ops, a MAKED and two INSFD, ten instructions for
;; what one FADDWQ does; for V4HF, measured, seventeen (eight LHZ, four
;; FADDH, four SH) against the seven this expander produces.
;;
;; So widen: put each operand in the low half of a pair, run the 128-bit
;; instruction, take the low half of the result.  The upper lanes are
;; deliberately zeroed rather than left undefined, because LVX floating point
;; writes the CS exception flags: garbage in the upper half of the pair would
;; raise IX or IO from lanes the program never asked about.  Zero is exact in
;; every one of these operations (0+0, 0-0, 0*0), so it adds no flag of its
;; own.  That costs two MAKED, and the whole sequence is still half what the
;; lane-by-lane lowering costs.
;;
;; The widening is done at expand time, not in a reload_completed split: a
;; late split cannot invent the 128-bit register pair, where a pseudo asked
;; for before allocation is placed by the allocator like any other.
;; -------------------------------------------------------------------------

;; The two 64-bit FP shapes and their 128-bit counterparts, via DMODE:
;; V2SF -> V4SF (FADDWQ/FSBFWQ/FMULWQ), V4HF -> V8HF (FADDHO/FSBFHO/FMULHO).
(define_mode_iterator V64F [V2SF V4HF])

(define_code_iterator V64F_ARITH [plus minus mult])
(define_code_attr v64f_arith [(plus "add") (minus "sub") (mult "mul")])

(define_expand "<v64f_arith><mode>3"
  [(set (match_operand:V64F 0 "register_operand")
        (V64F_ARITH:V64F (match_operand:V64F 1 "register_operand")
                         (match_operand:V64F 2 "register_operand")))]
  "LVX_2"
  {
    rtx wide[3];
    for (int i = 1; i <= 2; i++)
      {
        wide[i] = gen_reg_rtx (<DMODE>mode);
        /* The pair is zero but for its low half, which is the operand.  */
        emit_move_insn (wide[i], CONST0_RTX (<DMODE>mode));
        emit_move_insn (simplify_gen_subreg (<MODE>mode, wide[i],
                                             <DMODE>mode, 0),
                        operands[i]);
      }
    wide[0] = gen_reg_rtx (<DMODE>mode);
    emit_insn (gen_rtx_SET (wide[0],
                            gen_rtx_fmt_ee (<CODE>, <DMODE>mode,
                                            wide[1], wide[2])));
    emit_move_insn (operands[0],
                    simplify_gen_subreg (<MODE>mode, wide[0], <DMODE>mode, 0));
    DONE;
  }
)

(define_insn "addv4sf3"
  [(set (match_operand:V4SF 0 "register_operand" "=r,r")
        (plus:V4SF (match_operand:V4SF 1 "register_operand" "r,r")
                   (match_operand:V4SF 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   faddwq %0 = %1, %2
   faddwq %0 = %1, %W2"
  [(set_attr "type" "fmuld")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "subv4sf3"
  [(set (match_operand:V4SF 0 "register_operand" "=r,r")
        (minus:V4SF (match_operand:V4SF 1 "reg_or_splat32_operand" "r,SXW")
                    (match_operand:V4SF 2 "register_operand" "r,r")))]
  "LVX_2"
  "@
   fsbfwq %0 = %2, %1
   fsbfwq %0 = %2, %W1"
  [(set_attr "type" "fmuld")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)

(define_insn "mulv4sf3"
  [(set (match_operand:V4SF 0 "register_operand" "=r,r")
        (mult:V4SF (match_operand:V4SF 1 "register_operand" "r,r")
                   (match_operand:V4SF 2 "reg_or_splat32_operand" "r,SXW")))]
  "LVX_2"
  "@
   fmulwq %0 = %1, %2
   fmulwq %0 = %1, %W2"
  [(set_attr "type" "fmuld")
   (set_attr "issue" "lite,lite_x")
   (set_attr "length" "4,8")]
)
