(define_attr "length" "" (const_int 4))

(define_attr "predicable" "no,yes"
  (const_string "no"))

(define_attr "arch" "lvx_1,lvx_2" (const (symbol_ref "lvx_arch_schedule")))

;; Unspec numbers
(include "unspec.md")

;; Register numbers
(include "lvx-registers.md")

;; Iterators
(include "iterators.md")

;; Instruction types
(include "types.md")

;; Whether the pattern occupies a BCU slot, which lvx_sched_dfa_new_cycle uses
;; to share one guard syllable between the instructions a bundle guards on the
;; same condition.  A predicated pattern overrides it to yes.
(define_attr "bcu_used" "no,yes"
  (if_then_else
    (ior (eq_attr "type" "branch")
         (eq_attr "type" "sysget")
         (eq_attr "type" "jump")
         (eq_attr "type" "ijump"))
     (const_string "yes")
     (const_string "no")))

;; Processor resources and the reservation each issue class stands for,
;; generated from the machine description.
(include "scheduling-isa.md")

;; Latencies and bypasses -- the microarchitecture half.
(include "scheduling.md")

;; Constraints
(include "constraints.md")

;; Predicates
(include "predicates.md")

;; Dummy insn for BCU slot used by conditional instructions.
;; Defined to (const_int -1) as (const_int 0) matches "nop".
(define_insn "cond_insn"
  [(const_int -1)]
  ""
  ""
  [(set_attr "type" "branch")
   (set_attr "issue" "bcu_brrp")]
)

(define_expand "store_multiple"
  [(match_par_dup 3 [(set (match_operand:DI 0 "" "")
                          (match_operand:DI 1 "" ""))
                     (use (match_operand:DI 2 "" ""))])]
  ""
  "
{
  if (! lvx_expand_store_multiple (operands))
    FAIL;
}")

;; Simple store packing

;; sd+sd+sd+sd -> so
(define_peephole2
 [(set (match_operand:DI 0 "memory_operand" "")
       (match_operand:DI 1 "register_operand" ""))
  (set (match_operand:DI 2 "memory_operand" "")
       (match_operand:DI 3 "register_operand" ""))
  (set (match_operand:DI 4 "memory_operand" "")
       (match_operand:DI 5 "register_operand" ""))
  (set (match_operand:DI 6 "memory_operand" "")
       (match_operand:DI 7 "register_operand" ""))]
 ""
 [(use (const_int 0))]
 {
   if (lvx_pack_load_store (operands, 4))
     DONE;
   else
     FAIL;
 })

;; sd+sd -> sq
(define_peephole2
 [(set (match_operand:DI 0 "memory_operand" "")
       (match_operand:DI 1 "register_operand" ""))
  (set (match_operand:DI 2 "memory_operand" "")
       (match_operand:DI 3 "register_operand" ""))]
 ""
 [(use (const_int 0))]
 {
   if (lvx_pack_load_store (operands, 2))
     DONE;
   else
     FAIL;
 })

(define_expand "load_multiple"
  [(match_par_dup 3 [(set (match_operand:DI 0 "" "")
                          (match_operand:DI 1 "" ""))
                     (use (match_operand:DI 2 "" ""))])]
  ""
  "
{
  if (! lvx_expand_load_multiple (operands))
    FAIL;
}")

;; Simple load packing

;; ld+ld+ld+ld -> lo
(define_peephole2
 [(set (match_operand:DI 0 "register_operand" "")
       (match_operand:DI 1 "memory_operand" ""))
  (set (match_operand:DI 2 "register_operand" "")
       (match_operand:DI 3 "memory_operand" ""))
  (set (match_operand:DI 4 "register_operand" "")
       (match_operand:DI 5 "memory_operand" ""))
  (set (match_operand:DI 6 "register_operand" "")
       (match_operand:DI 7 "memory_operand" ""))]
 ""
 [(use (const_int 0))]
 {
   if (lvx_pack_load_store (operands, 4))
     DONE;
   else
     FAIL;
 })

;; ld+ld -> lq
(define_peephole2
 [(set (match_operand:DI 0 "register_operand" "")
       (match_operand:DI 1 "memory_operand" ""))
  (set (match_operand:DI 2 "register_operand" "")
       (match_operand:DI 3 "memory_operand" ""))]
 ""
 [(use (const_int 0))]
 {
   if (lvx_pack_load_store (operands, 2))
     DONE;
   else
     FAIL;
 })

(define_insn "*lo_multiple_cached"
  [(match_parallel 0 "load_multiple_operation"
    [(set (match_operand:DI 1 "register_operand" "=r,r,r")
          (mem:DI (plus:P (match_operand:P 2 "register_operand" "r,r,r")
                          (match_operand 3 "const_int_operand" "I10,I37,i"))))
     (set (match_operand:DI 4 "register_operand" "=r,r,r")
          (mem:DI (plus:P (match_dup 2) (match_operand 5 "const_int_operand" "I10,I37,i"))))
     (set (match_operand:DI 6 "register_operand" "=r,r,r")
          (mem:DI (plus:P (match_dup 2) (match_operand 7 "const_int_operand" "I10,I37,i"))))
     (set (match_operand:DI 8 "register_operand" "=r,r,r")
          (mem:DI (plus:P (match_dup 2) (match_operand 9 "const_int_operand" "I10,I37,i"))))])]
  "(XVECLEN (operands[0], 0) == 4)"
  "lo %o1 = %3[%2]"
  [(set_attr "type" "load, load, load")
   (set_attr "issue" "lsu_auxw, lsu_auxw_x, lsu_auxw_y")
   (set_attr "length" "4,8,12")])

(define_insn "*lo_multiple_uncached"
  [(match_parallel 0 "load_multiple_operation_uncached"
    [(set (match_operand:DI 1 "register_operand" "=r,r,r")
          (mem:DI (plus:P (match_operand:P 2 "register_operand" "r,r,r")
                          (match_operand 3 "const_int_operand" ""))))
     (set (match_operand:DI 4 "register_operand" "=r,r,r")
          (mem:DI (plus:P (match_dup 2) (match_operand 5 "const_int_operand" "I10,I37,i"))))
     (set (match_operand:DI 6 "register_operand" "=r,r,r")
          (mem:DI (plus:P (match_dup 2) (match_operand 7 "const_int_operand" "I10,I37,i"))))
     (set (match_operand:DI 8 "register_operand" "=r,r,r")
          (mem:DI (plus:P (match_dup 2) (match_operand 9 "const_int_operand" "I10,I37,i"))))])]
  "(XVECLEN (operands[0], 0) == 4)"
  "lo.u %o1 = %3[%2]"
  [(set_attr "type" "loadu, loadu, loadu")
   (set_attr "issue" "lsu_auxw, lsu_auxw_x, lsu_auxw_y")
   (set_attr "length" "4,8,12")])

(define_insn "*lq_multiple_cached"
  [(match_parallel 0 "load_multiple_operation"
    [(set (match_operand:DI 1 "register_operand" "=r,r,r")
          (mem:DI (plus:P (match_operand:P 2 "register_operand" "r,r,r")
                          (match_operand 3 "const_int_operand" "I10,I37,i"))))
     (set (match_operand:DI 4 "register_operand" "=r,r,r")
          (mem:DI (plus:P (match_dup 2) (match_operand 5 "const_int_operand" "I10,I37,i"))))])]
  "(XVECLEN (operands[0], 0) == 2)"
  "lq %q1 = %3[%2]"
  [(set_attr "type" "load, load, load")
   (set_attr "issue" "lsu_auxw, lsu_auxw_x, lsu_auxw_y")
   (set_attr "length" "4,8,12")])

(define_insn "*lq_multiple_uncached"
  [(match_parallel 0 "load_multiple_operation_uncached"
    [(set (match_operand:DI 1 "register_operand" "=r,r,r")
          (mem:DI (plus:P (match_operand:P 2 "register_operand" "r,r,r")
                          (match_operand 3 "const_int_operand" "I10,I37,i"))))
     (set (match_operand:DI 4 "register_operand" "=r,r,r")
          (mem:DI (plus:P (match_dup 2) (match_operand 5 "const_int_operand" "I10,I37,i"))))])]
  "(XVECLEN (operands[0], 0) == 2)"
  "lq.u %q1 = %3[%2]"
  [(set_attr "type" "loadu, loadu, loadu")
   (set_attr "issue" "lsu_auxw, lsu_auxw_x, lsu_auxw_y")
   (set_attr "length" "4,8,12")])

(define_insn "*sq_multiple"
  [(match_parallel 0 "store_multiple_operation"
    [(set (mem:DI (plus:P (match_operand:P 1 "register_operand" "r,r,r")
                          (match_operand 2 "const_int_operand" "I10,I37,i")))
          (match_operand:DI 3 "register_operand" "r,r,r"))

     (set (mem:DI (plus:P (match_dup 1) (match_operand 4 "const_int_operand" "I10,I37,i")))
          (match_operand:DI 5 "register_operand" "r,r,r"))])]
  "(XVECLEN (operands[0], 0) == 2)"
  "sq %2[%1] = %q3"
  [(set_attr "type" "store, store, store")
   (set_attr "issue" "lsu_memw_auxr, lsu_memw_auxr_x, lsu_memw_auxr_y")
   (set_attr "length" "4,8,12")])

(define_insn "*so_multiple"
  [(match_parallel 0 "store_multiple_operation"
    [(set (mem:DI (plus:P (match_operand:P 1 "register_operand" "r,r,r")
                          (match_operand 2 "const_int_operand" "I10,I37,i")))
          (match_operand:DI 3 "register_operand" "r,r,r"))

     (set (mem:DI (plus:P (match_dup 1) (match_operand 4 "const_int_operand" "I10,I37,i")))
          (match_operand:DI 5 "register_operand" "r,r,r"))

     (set (mem:DI (plus:P (match_dup 1) (match_operand 6 "const_int_operand" "I10,I37,i")))
          (match_operand:DI 7 "register_operand" "r,r,r"))

     (set (mem:DI (plus:P (match_dup 1) (match_operand 8 "const_int_operand" "I10,I37,i")))
          (match_operand:DI 9 "register_operand" "r,r,r"))])]
  "(XVECLEN (operands[0], 0) == 4)"
  "so %2[%1] = %o3"
  [(set_attr "type" "store, store, store")
   (set_attr "issue" "lsu_memw_auxr, lsu_memw_auxr_x, lsu_memw_auxr_y")
   (set_attr "length" "4,8,12")])


;; ========================= move ========================
;;

;; FIXME AUTO: add size info for 'reg[reg]' addressing (currently falling back to lsu.x)

(define_expand "mov<mode>"
   [(set (match_operand:ALLIF 0 "nonimmediate_operand" "")
         (match_operand:ALLIF 1 "lvx_mov_operand"      ""))]
   ""
   "
    if (MEM_P (operands[0]))
        operands[1] = force_reg (<MODE>mode, operands[1]);

    if (CONSTANT_P (operands[1]))
      {
        lvx_expand_mov_constant (operands);
        DONE;
      }
   "
)

;; FIXME AUTO: refine set insn to bundle it when possible. T7808
(define_insn "*mov<mode>_all"
    [(set (match_operand:ALLIF 0 "nonimmediate_operand" "=r,      r,      r,  r, a, b, m,  r,  r,  r,  r,  r,  r, r, SFR,   r,   r")
          (match_operand:ALLIF 1 "general_operand"       "r, I16H16, I43H43, nF, r, r, r, Ca, Cb, Cm, Za, Zb, Zm, SFR, r,  Cp, SYM"))]
  "register_operand (operands[0], <MODE>mode) || register_operand (operands[1], <MODE>mode)"
{
  switch (which_alternative)
    {
    case 0:
      return "copy<ALLIF:copyx> %0 = %1";
    case 1: case 2: case 3: case 16:
      return "maked %0 = %1";
    case 4: case 5: case 6:
      return "s<ALLIF:lsusize>%X0 %0 = %1";
    case 7: case 8: case 9: case 10: case 11: case 12:
      return "l<lsusizezx>%V1 %0 = %1";
    case 13:
      return "get %0 = %1";
    case 14:
      return "set %0 = %1";
    case 15:
      return "pcrel %0 = %T1";
    default:
      gcc_unreachable ();
    }
}
  [(set_attr "type" "alu, alu, alu, alu, store, store, store, load, load, load, loadu, loadu, loadu, sysget, all, alu, alu")
   (set_attr "issue" "alu_tiny, alu_tiny, alu_tiny_x, alu_tiny_y, lsu_memw_auxr, lsu_memw_auxr_x, lsu_memw_auxr_y, lsu_auxw, lsu_auxw_x, lsu_auxw_y, lsu_auxw, lsu_auxw_x, lsu_auxw_y, bcu2_tiny_lsu, all, alu_full_x, alu_tiny_y")
   (set_attr "length"      "4,        4,          8,         12,          4,            8,           12,         4,           8,          12,                  4,                    8,                   12,       4,   4,          8,         12")]
)

(define_insn "add_pcrel_<mode>"
  [(set (match_operand:P 0 "register_operand" "=r")
        (const:P (plus:P (pc)
                         (unspec:P [(match_operand 1 "symbolic_operand" "")
                                    (pc)] UNSPEC_PCREL))))
]
  ""
  "pcrel %0 = %T1"
  [(set_attr "type" "alu")
   (set_attr "issue" "alu_tiny_y")
   (set_attr "length" "12")]
)

;; ========================= jump ========================
;;

(define_insn "jump"
  [(set (pc) (label_ref (match_operand 0)))]
  ""
  "goto %0"
  [(set_attr "type" "jump")
   (set_attr "issue" "bcu_xfer")]
)

(define_expand "indirect_jump"
  [(set (pc) (match_operand 0 "address_operand"))])

(define_insn "*indirect_jump_<mode>"
  [(set (pc) (match_operand:P 0 "address_operand" "r"))]
  ""
  "igoto %0"
  [(set_attr "type" "ijump")
   (set_attr "issue" "bcu_xfer_brrp")]
)

;; Restore the frame and jump, for __builtin_longjmp and nonlocal goto.
;;
;; This is expand_builtin_longjmp's generic fallback (builtins.cc), with the
;; clobber of FRAME_POINTER_RTX left out.  We must define it, rather than let
;; that fallback run, because LVX makes ARG_POINTER_REGNUM an alias of
;; FRAME_POINTER_REGNUM: var-tracking picks arg_pointer_rtx as its CFA base
;; anchor (it only prefers frame_pointer_rtx under FRAME_POINTER_CFA_OFFSET,
;; which LVX does not define), so on LVX that clobber writes cfa_base_rtx and
;; trips the "loc != cfa_base_rtx" assertion in var-tracking's add_stores.
;; Targets with a distinct arg pointer never notice; sparc, the other target
;; that aliases the two, defines nonlocal_goto for the same reason.
;;
;; The clobber only exists to keep the hard-frame-pointer move below from
;; being optimized away, and the clobber of HARD_FRAME_POINTER_RTX that we do
;; keep already does that.  Dropping it costs nothing: the virtual frame
;; pointer is eliminated long before this point and never names a real
;; machine register.
(define_expand "nonlocal_goto"
  [(use (match_operand 0 "general_operand"))
   (use (match_operand 1 "general_operand"))
   (use (match_operand 2 "general_operand"))
   (use (match_operand 3 "general_operand"))]
  ""
{
  rtx lab = operands[1];
  rtx stack = operands[2];
  rtx fp = operands[3];

  emit_clobber (gen_rtx_MEM (BLKmode, gen_rtx_SCRATCH (VOIDmode)));
  emit_clobber (gen_rtx_MEM (BLKmode, hard_frame_pointer_rtx));

  /* Both must be copied out before the stack is restored, since the setjmp
     buffer may itself live in the frame we are about to abandon.  */
  lab = copy_to_reg (lab);
  fp = copy_to_reg (fp);

  emit_stack_restore (SAVE_NONLOCAL, stack);

  /* Ensure the frame pointer move is not optimized.  */
  emit_insn (gen_blockage ());
  emit_clobber (hard_frame_pointer_rtx);
  emit_move_insn (hard_frame_pointer_rtx, fp);

  emit_use (hard_frame_pointer_rtx);
  emit_use (stack_pointer_rtx);

  emit_indirect_jump (lab);
  DONE;
})

(define_expand "tablejump"
  [(set (pc)
        (match_operand:SI 0 "register_operand"))
   (use (label_ref (match_operand 1 "")))]
  "can_create_pseudo_p ()"
{
  lvx_expand_tablejump (operands[0], operands[1]);
  DONE;
})

(define_insn "tablejump_real_<mode>"
  [(parallel [(set (pc)
                   (match_operand:P 0 "register_operand" "r"))
              (use (label_ref (match_operand 1 "")))]
  )]
  "<MODE>mode == Pmode"
  "igoto %0"
  [(set_attr "type" "ijump")
   (set_attr "issue" "bcu_xfer_brrp")]
)

(define_insn "nop"
  [(const_int 0)]
  ""
  "nop\n\t;;"
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

(define_insn "nop_volatile"
  [(unspec_volatile [(const_int 0)] UNSPECV_NOP)]
  ""
  "nop"
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

;; Provide a 37bits offset for 32bits and 64bits for 64bits.
;; This may be too much in most of the cases but having smaller
;; values may not work in some cases.
;; Changing this requires changes in Assembler @gotaddr() handling.
(define_insn "set_gotp_<mode>"
   [(set (match_operand 0 "pmode_register_operand" "=r")
         (unspec:P [(const_int 0)] UNSPEC_PIC))]
   ""
   "pcrel %0 = @gotaddr()"
  [(set_attr "type" "alu")
   (set_attr "issue" "alu_tiny<symlen1>")
   (set_attr "length" "<symlen2>")]
)

(define_insn "*get_<mode>"
   [(set (match_operand:ALLP 0 "register_operand" "=r")
         (match_operand:ALLP 1 "system_register_operand" "SFR"))]
   ""
   "get %0 = %1"
  [(set_attr "type" "sysget")
   (set_attr "issue" "bcu2_tiny_lsu")]
)

(define_insn "lvx_get"
   [(set (match_operand:DI 0 "register_operand" "=r,r")
         (unspec_volatile:DI [(match_operand:DI 1 "register_operand" "SFR,0")] UNSPEC_GET))]
   ""
   "@
    get %0 = %1
    iget %0"
  [(set_attr "type" "sysget")
   (set_attr "issue" "bcu2_tiny_lsu")]
)

(define_insn "*set_<mode>"
   [(set (match_operand:ALLP 0 "system_register_operand" "=SAB,SFR")
         (match_operand:ALLP 1 "register_operand" "r,r"))]
   ""
   "set %0 = %1"
  [(set_attr "type" "all, branch2")
   (set_attr "issue" "all, bcu2")]
)

(define_insn "lvx_set"
   [(set (match_operand:DI 0 "system_register_operand" "=SAB,SFR")
         (unspec_volatile:DI [(match_operand:DI 1 "register_operand" "r,r")] UNSPEC_SET))]
   ""
   "set %0 = %1"
  [(set_attr "type" "all, branch2")
   (set_attr "issue" "all, bcu2")]
)

(define_insn "lvx_scall"
  [(unspec_volatile:DI [(match_operand:SI 0 "nonmemory_operand" "=r,i")] UNSPEC_SCALL)]
  ""
  "scall %0"
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

(define_insn "lvx_wfxl"
   [(set (match_operand:DI 0 "system_register_operand" "=SAB,SFR")
         (unspec_volatile:DI [(match_operand:DI 1 "register_operand" "r,r")] UNSPEC_WFXL))]
   ""
   "wfxl %0, %1"
  [(set_attr "type" "all, branch2")
   (set_attr "issue" "all, bcu2")]
)

(define_insn "lvx_wfxm"
   [(set (match_operand:DI 0 "system_register_operand" "=SAB,SFR")
         (unspec_volatile:DI [(match_operand:DI 1 "register_operand" "r,r")] UNSPEC_WFXM))]
   ""
   "wfxm %0, %1"
  [(set_attr "type" "all, branch2")
   (set_attr "issue" "all, bcu2")]
)

(define_insn "lvx_syncgroup"
  [(unspec [(match_operand:DI 0 "register_operand" "r")] UNSPEC_SYNCGROUP)
   (clobber (mem:BLK (scratch)))
   (set (reg:DI LVX_IPE_REGNO)
        (unspec:DI [(reg:DI LVX_IPE_REGNO) (match_dup 0)] UNSPEC_EFFECT))]
  ""
  "syncgroup %0"
  [(set_attr "type" "branch2")
   (set_attr "issue" "bcu2")]
)

(define_insn "lvx_await"
   [(unspec_volatile [(const_int 0)] UNSPEC_AWAIT)]
   ""
   "await"
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

(define_insn "lvx_barrier"
   [(unspec_volatile [(const_int 0)] UNSPEC_BARRIER)]
   ""
   "barrier"
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

(define_insn "lvx_sleep"
   [(unspec_volatile [(const_int 0)] UNSPEC_SLEEP)]
   ""
   "sleep"
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

(define_insn "lvx_stop"
   [(unspec_volatile [(const_int 0)] UNSPEC_STOP)]
   ""
   "stop"
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

(define_insn "lvx_waitit"
   [(set (match_operand:DI 0 "register_operand" "=r")
         (unspec_volatile:DI [(match_operand:DI 1 "register_operand" "0")] UNSPEC_WAITIT))]
   ""
   "waitit %0"
  [(set_attr "type" "sysget")
   (set_attr "issue" "bcu2_tiny_lsu")]
)

(define_insn "trap"
  [(trap_if (const_int 1) (const_int 0))]
  ""
  "errop"
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

(define_insn "lvx_fence"
  [(unspec [(match_operand 0 "" "")] UNSPEC_FENCE)
   (clobber (mem:BLK (scratch)))]
  ""
  "fence%0"
  [(set_attr "type" "cache")
   (set_attr "issue" "lsu2_memw")]
)

(define_insn "lvx_d1inval"
  [(unspec [(const_int 0)] UNSPEC_D1INVAL)
   (clobber (mem:BLK (scratch)))]
  ""
  "d1inval"
  [(set_attr "type" "cache")
   (set_attr "issue" "lsu2_memw")]
)

(define_insn "lvx_i1inval"
  [(unspec_volatile [(const_int 0)] UNSPEC_I1INVAL)
   (clobber (mem:BLK (scratch)))]
  ""
  "i1inval"
  [(set_attr "type" "cache")
   (set_attr "issue" "lsu2_memw")]
)

(define_insn "lvx_dinvall"
  [(unspec [(match_operand 0 "noxsaddr_operand" "Aa,Ab,p")] UNSPEC_DINVALL)
   (clobber (mem:BLK (scratch)))]
  ""
  "dinvall%X0 %A0"
  [(set_attr "length" "4,     8,    12")
   (set_attr "type" "cache, cache, cache")
   (set_attr "issue" "lsu, lsu_x, lsu_y")]
)

(define_insn "lvx_dtouchl"
  [(prefetch (match_operand 0 "noxsaddr_operand" "Aa,Ab,p")
             (const_int 0)
             (const_int 0))]
  ""
  "dtouchl%X0 %A0"
  [(set_attr "length" "4,     8,    12")
   (set_attr "type" "cache, cache, cache")
   (set_attr "issue" "lsu, lsu_x, lsu_y")]
)

(define_insn "lvx_dpurgel"
  [(unspec [(match_operand 0 "noxsaddr_operand" "Aa,Ab,p")] UNSPEC_DPURGEL)
   (clobber (mem:BLK (scratch)))]
  ""
  "dpurgel%X0 %A0"
  [(set_attr "length" "4,     8,    12")
   (set_attr "type" "cache, cache, cache")
   (set_attr "issue" "lsu, lsu_x, lsu_y")]
)

(define_insn "lvx_dflushl"
  [(unspec [(match_operand 0 "noxsaddr_operand" "Aa,Ab,p")] UNSPEC_DFLUSHL)
   (clobber (mem:BLK (scratch)))]
  ""
  "dflushl%X0 %A0"
  [(set_attr "length" "4,     8,    12")
   (set_attr "type" "cache, cache, cache")
   (set_attr "issue" "lsu, lsu_x, lsu_y")]
)

(define_insn "lvx_i1invals"
  [(unspec_volatile [(match_operand 0 "noxsaddr_operand" "Aa,Ab,p")] UNSPEC_I1INVALS)
   (clobber (mem:BLK (scratch)))]
  ""
  "i1invals%X0 %A0"
  [(set_attr "length" "4,     8,    12")
   (set_attr "type" "cache, cache, cache")
   (set_attr "issue" "lsu2_memw, lsu2_memw_x, lsu2_memw_y")]
)

(define_insn "lvx_dinvalsw"
  [(unspec [(match_operand:DI 0 "register_operand" "r")
            (match_operand:DI 1 "register_operand" "r")
            (match_operand 2 "" "")] UNSPEC_DINVALSW)
   (clobber (mem:BLK (scratch)))]
  ""
  "dinvalsw%2 %0, %1"
  [(set_attr "type" "cache")
   (set_attr "issue" "lsu2_memw")]
)

(define_insn "lvx_dpurgesw"
  [(unspec [(match_operand:DI 0 "register_operand" "r")
            (match_operand:DI 1 "register_operand" "r")
            (match_operand 2 "" "")] UNSPEC_DPURGESW)
   (clobber (mem:BLK (scratch)))]
  ""
  "dpurgesw%2 %0, %1"
  [(set_attr "type" "cache")
   (set_attr "issue" "lsu2_memw")]
)

(define_insn "lvx_dflushsw"
  [(unspec [(match_operand:DI 0 "register_operand" "r")
            (match_operand:DI 1 "register_operand" "r")
            (match_operand 2 "" "")] UNSPEC_DFLUSHSW)
   (clobber (mem:BLK (scratch)))]
  ""
  "dflushsw%2 %0, %1"
  [(set_attr "type" "cache")
   (set_attr "issue" "lsu2_memw")]
)

(define_insn "prefetch"
  [(prefetch (match_operand 0 "noxsaddr_operand" "Aa,Ab,p")
             (match_operand 1 "const_int_operand" "")
             (match_operand 2 "const_int_operand" ""))]
  ""
  "dtouchl%X0 %A0"
  [(set_attr "length" "4,     8,    12")
   (set_attr "type" "cache, cache, cache")
   (set_attr "issue" "lsu, lsu_x, lsu_y")]
)

(define_insn "lvx_tlbdinval"
  [(unspec_volatile [(const_int 0)] UNSPEC_TLBDINVAL)
   (clobber (mem:BLK (scratch)))]
  ""
  "tlbdinval"
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

(define_insn "lvx_tlbiinval"
  [(unspec_volatile [(const_int 0)] UNSPEC_TLBIINVAL)
   (clobber (mem:BLK (scratch)))]
  ""
  "tlbiinval"
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

(define_insn "lvx_tlbprobe"
  [(unspec_volatile [(const_int 0)] UNSPEC_TLBPROBE)]
  ""
  "tlbprobe"
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

(define_insn "lvx_tlbread"
  [(unspec_volatile [(const_int 0)] UNSPEC_TLBREAD)]
  ""
  "tlbread"
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

(define_insn "lvx_tlbwrite"
  [(unspec_volatile [(const_int 0)] UNSPEC_TLBWRITE)
   (clobber (mem:BLK (scratch)))]
  ""
  "tlbwrite"
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

(define_insn "memory_barrier"
  [(clobber (mem:BLK (scratch)))]
  ""
  "fence"
  [(set_attr "type" "cache")
   (set_attr "issue" "lsu2_memw")]
)

;; Uncached Loads (Deprecated)

(define_insn "lvx_lbzu"
   [(set (match_operand:QI 0 "register_operand" "=r,r,r")
         (unspec:QI [(match_operand:QI 1 "memory_operand" "a,b,m")] UNSPEC_LOADU))
    (clobber (mem:BLK (scratch)))
   ]
   ""
   "lbz.u%X1 %0 = %1"
  [(set_attr "length" "4,8,12")
   (set_attr "type" "loadu, loadu, loadu")
   (set_attr "issue" "lsu_auxw, lsu_auxw_x, lsu_auxw_y")]
)

(define_insn "lvx_lbsu"
   [(set (match_operand:QI 0 "register_operand"           "=r, r, r")
         (unspec:QI [(match_operand:QI 1 "memory_operand" " a, b, m")] UNSPEC_LOADU))
    (clobber (mem:BLK (scratch)))
   ]
   ""
   "lbs.u%X1 %0 = %1"
  [(set_attr "length" "4,8,12")
   (set_attr "type" "loadu, loadu, loadu")
   (set_attr "issue" "lsu_auxw, lsu_auxw_x, lsu_auxw_y")]
)

(define_insn "lvx_lhzu"
   [(set (match_operand:HI 0 "register_operand" "=r,r,r")
         (unspec:HI [(match_operand:HI 1 "memory_operand" "a,b,m")] UNSPEC_LOADU))
    (clobber (mem:BLK (scratch)))
   ]
   ""
   "lhz.u%X1 %0 = %1"
  [(set_attr "length" "4, 8, 12")
   (set_attr "type" "loadu, loadu, loadu")
   (set_attr "issue" "lsu_auxw, lsu_auxw_x, lsu_auxw_y")]
)

(define_insn "lvx_lhsu"
   [(set (match_operand:HI 0 "register_operand" "=r,r,r")
         (unspec:HI [(match_operand:HI 1 "memory_operand" "a,b,m")] UNSPEC_LOADU))
    (clobber (mem:BLK (scratch)))
   ]
   ""
   "lhs.u%X1 %0 = %1"
  [(set_attr "length" "4, 8, 12")
   (set_attr "type" "loadu, loadu, loadu")
   (set_attr "issue" "lsu_auxw, lsu_auxw_x, lsu_auxw_y")]
)

(define_insn "lvx_lwzu"
   [(set (match_operand:SI 0 "register_operand" "=r,r,r")
         (unspec:SI [(match_operand:SI 1 "memory_operand" "a,b,m")] UNSPEC_LOADU))
    (clobber (mem:BLK (scratch)))
   ]
   ""
   "lwz.u%X1 %0 = %1"
  [(set_attr "length" "4,8,12")
   (set_attr "type" "loadu, loadu, loadu")
   (set_attr "issue" "lsu_auxw, lsu_auxw_x, lsu_auxw_y")]
)

(define_insn "lvx_lwsu"
   [(set (match_operand:SI 0 "register_operand" "=r,r,r")
         (unspec:SI [(match_operand:SI 1 "memory_operand" "a,b,m")] UNSPEC_LOADU))
    (clobber (mem:BLK (scratch)))
   ]
   ""
   "lws.u%X1 %0 = %1"
  [(set_attr "length" "4,8,12")
   (set_attr "type" "loadu, loadu, loadu")
   (set_attr "issue" "lsu_auxw, lsu_auxw_x, lsu_auxw_y")]
)

(define_insn "lvx_ldu"
   [(set (match_operand:DI 0 "register_operand" "=r,r,r")
         (unspec:DI [(match_operand:DI 1 "memory_operand" "a,b,m")] UNSPEC_LOADU))
    (clobber (mem:BLK (scratch)))
   ]
   ""
   "ld.u%X1 %0 = %1"
  [(set_attr "length" "4, 8, 12")
   (set_attr "type" "loadu, loadu, loadu")
   (set_attr "issue" "lsu_auxw, lsu_auxw_x, lsu_auxw_y")]
)

(define_insn "lvx_lqu"
   [(set (match_operand:TI 0 "register_operand" "=r,r,r")
         (unspec:TI [(match_operand:TI 1 "memory_operand" "a,b,m")] UNSPEC_LOADU))
    (clobber (mem:BLK (scratch)))
   ]
   ""
   "lq.u%X1 %0 = %1"
  [(set_attr "length" "4, 8, 12")
   (set_attr "type"   "loadu, loadu, loadu")
   (set_attr "issue" "lsu_auxw, lsu_auxw_x, lsu_auxw_y")]
)

;; FIXME AUTO: add size info for 'reg[reg]' addressing (currently falling back to lsu.x)
(define_insn "*l<mode><ANY_EXTEND:lsux>"
   [(set (match_operand:DI 0 "register_operand"                 "=r,  r,  r,  r,  r,  r")
         (ANY_EXTEND:DI (match_operand:SHORT 1 "memory_operand" "Ca, Cb, Cm, Za, Zb, Zm")))]
   ""
   "l<SHORT:lsusize><ANY_EXTEND:lsux>%V1 %0 = %1"
  [(set_attr "length" "            4,               8,              12,                      4,                        8,                       12")
   (set_attr "type"   "load, load, load, loadu, loadu, loadu")
   (set_attr "issue" "lsu_auxw, lsu_auxw_x, lsu_auxw_y, lsu_auxw, lsu_auxw_x, lsu_auxw_y")]
)

;; FIXME AUTO: add size info for 'reg[reg]' addressing (currently falling back to lsu.x)
(define_insn "extend<mode>di2"
  [(set (match_operand:DI 0 "register_operand"                        "=r,  r,  r,  r,  r,  r,  r")
        (sign_extend:DI (match_operand:SHORT 1 "nonimmediate_operand" " r, Ca, Cb, Cm, Za, Zb, Zm")))]
  ""
{
 switch (which_alternative)
   {
   case 0:
     return "sx<lsusize>d %0 = %1";
   case 1: case 2: case 3:
     return "l<lsusize>s%X1 %0 = %1";
   case 4: case 5: case 6:
     return "l<lsusize>s%X1.u %0 = %1";
   default:
     gcc_unreachable ();
   }
}
  [(set_attr "type"   "alu, load, load, load, loadu, loadu, loadu")
   (set_attr "issue" "alu_lite, lsu_auxw, lsu_auxw_x, lsu_auxw_y, lsu_auxw, lsu_auxw_x, lsu_auxw_y")
   (set_attr "length" "       4,             4,               8,              12,                      4,                        8,                       12")])

(define_insn "zero_extend<mode>di2"
  [(set (match_operand:DI 0 "register_operand" "=r,r,r,r,r,r,r")
        (zero_extend:DI (match_operand:SHORT 1 "nonimmediate_operand" "r,Ca,Cb,Cm,Za,Zb,Zm")))]
  ""
{
 switch (which_alternative)
   {
     case 0:
       return "zx<lsusize>d %0 = %1";
     case 1: case 2: case 3:
       return "l<lsusize>z%X1 %0 = %1";
     case 4: case 5: case 6:
       return "l<lsusize>z%X1.u %0 = %1";
     default:
       gcc_unreachable ();
   }
}
  [(set_attr "type"   "alu, load, load, load, loadu, loadu, loadu")
   (set_attr "issue" "alu_lite, lsu_auxw, lsu_auxw_x, lsu_auxw_y, lsu_auxw, lsu_auxw_x, lsu_auxw_y")
   (set_attr "length" "       4,             4,               8,              12,                      4,                        8,                       12")])

(define_insn "*icall_<mode>"
  [(call (mem:P (match_operand:P 0 "register_operand" "r"))
         (match_operand 1 "general_operand" ""))
   (clobber (reg:DI LVX_RA_REGNO))]
  ""
  "icall %0"
  [(set_attr "type" "ijump")
   (set_attr "issue" "bcu_xfer_brrp")]
)

(define_expand "call"
  [(parallel [(call (match_operand 0 "memory_operand" "")
                    (match_operand 1 "general_operand" ""))
              (clobber (reg:DI LVX_RA_REGNO))])]
  ""
{
  rtx callee = XEXP (operands[0], 0);
  if (!jump_operand (operands[0], Pmode))
      XEXP (operands[0], 0) = force_reg (Pmode, callee);
})

(define_insn "*call_<mode>"
  [(call (match_operand:P 0 "jump_operand" "")
         (match_operand 1 "" ""))
   (clobber (reg:DI LVX_RA_REGNO))]
  ""
  "call %0"
  [(set_attr "type" "jump")
   (set_attr "issue" "bcu_xfer")]
)

(define_expand "call_value"
  [(parallel [(set (match_operand 0 "" "")
                   (call (match_operand 1 "memory_operand" "")
                         (match_operand 2 "general_operand" "")))
              (clobber (reg:DI LVX_RA_REGNO))])]
  ""
{
  rtx callee = XEXP (operands[1], 0);
  if (!jump_operand (operands[1], Pmode))
      XEXP (operands[1], 0) = force_reg (Pmode, callee);
})

(define_insn "*scall_value_<mode>"
  [(set (match_operand 0 "" "")
        (call (match_operand:P 1 "syscall_operand" "")
              (match_operand 2 "" "")))
   (clobber (reg:DI LVX_RA_REGNO))]
  ""
  {
    if (GET_CODE (XEXP (operands[1], 0)) == REG)
      operands[1] = XEXP (operands[1], 0);
    return "scall %1";
  }
  [(set_attr "type" "all")
   (set_attr "issue" "all")]
)

(define_insn "*call_value_<mode>"
  [(set (match_operand 0 "" "")
        (call (match_operand:P 1 "jump_operand" "")
              (match_operand 2 "" "")))
   (clobber (reg:DI LVX_RA_REGNO))]
  ""
  "call %1"
  [(set_attr "type" "jump")
   (set_attr "issue" "bcu_xfer")]
)

(define_expand "sibcall_value"
  [(parallel
    [(set (match_operand 0 "" "")
          (call (match_operand 1 "memory_operand" "")
                (match_operand 2 "general_operand" "")))
     (return)])]
  ""
{
  rtx callee = XEXP (operands[1], 0);
  if (!jump_operand (operands[1], Pmode))
      XEXP (operands[1], 0) = force_reg (Pmode, callee);
})

(define_insn "*sibcall_value_<mode>"
  [(set (match_operand 0 "" "")
        (call (match_operand:P 1 "jump_operand" "")
              (match_operand 2 "" "")))
   (return)]
  ""
  "goto %1"
  [(set_attr "type" "jump")
   (set_attr "issue" "bcu_xfer")]
)

(define_expand "sibcall"
  [(parallel
    [(call (match_operand 0 "memory_operand" "")
           (match_operand 1 "general_operand" ""))
     (return)])]
  ""
{
  rtx callee = XEXP (operands[0], 0);
  if (!jump_operand (operands[0], Pmode))
      XEXP (operands[0], 0) = force_reg (Pmode, callee);
})

(define_insn "*sibcall_<mode>"
  [(call (match_operand:P 0 "jump_operand" "")
         (match_operand 1 "" ""))
   (return)]
  ""
  "goto %0"
  [(set_attr "type" "jump")
   (set_attr "issue" "bcu_xfer")]
)

;;
;; These pattern were used for indirect sibcall
;; but a bug prevent their correct usage
;; and leads to incorrect code (jump to random address)
;; See T1118

;; (define_insn "*isibcall_value_real_<mode>"
;;     [(set (match_operand 0 "" "")
;;           (call (mem:P (match_operand:P 1 "register_operand" "Cs"))
;;                 (match_operand 2 "" "")))
;;      (return)]
;;   "igoto<P:suffix> %1"
;; [(set_attr "type" "jump")]
;; )

;; (define_insn "*isibcall_real_<mode>"
;;     [(call (mem:P (match_operand:P 0 "register_operand" "Cs"))
;;            (match_operand 1 "" ""))
;;      (return)]
;;   "igoto<P:suffix> %0"
;; [(set_attr "type" "jump")]
;; )

(define_insn "*icall_value_<mode>"
  [(set (match_operand 0 "" "")
        (call (mem:P (match_operand:P 1 "register_operand" "r"))
              (match_operand 2 "general_operand" "")))
   (clobber (reg:DI LVX_RA_REGNO))]
  ""
  "icall %1"
  [(set_attr "type" "ijump")
   (set_attr "issue" "bcu_xfer_brrp")]
)

(define_code_iterator gt_comp [gt gtu])
(define_code_iterator lt_comp [lt ltu])
(define_code_iterator ge_comp [ge geu])
(define_code_iterator le_comp [le leu])

(define_insn_and_split "*comp_<code>_incr"
   [(set (match_operand:SI 0 "register_operand" "=r")
         (le_comp:SI (plus:SI (match_operand:SI 1 "register_operand" "r")
                              (const_int 1))
                     (match_operand:SI 2 "register_operand" "r")))
    (set (match_operand:SI 3 "register_operand" "=r")
         (plus:SI (match_dup 1) (const_int 1)))
   ]
   ""
   "comp_<code>_incr not spilt"
   ""
  [(set (match_dup 0) (match_op_dup:SI 4 [(match_dup 1) (match_dup 2)]))
 (set (match_dup 3) (plus:SI (match_dup 1) (const_int 1)))]
{
     operands[4] = gen_rtx_fmt_ee(lvx_strict_to_nonstrict_comparison_operator (<CODE>),
                                  SImode, operands[1], operands[2]);
}
)

(define_insn_and_split "*comp_<code>_decr"
   [(set (match_operand:SI 0 "register_operand" "=r")
         (lt_comp:SI (plus:SI (match_operand:SI 1 "register_operand" "r")
                              (const_int -1))
                     (match_operand:SI 2 "register_operand" "r")))
    (set (match_operand:SI 3 "register_operand" "=r")
         (plus:SI (match_dup 1) (const_int -1)))
   ]
   ""
   "comp_<code>_decr not spilt"
   ""
  [(set (match_dup 0) (match_op_dup:SI 4 [(match_dup 1) (match_dup 2)]))
 (set (match_dup 3) (plus:SI (match_dup 1) (const_int -1)))]
{
     operands[4] = gen_rtx_fmt_ee(lvx_strict_to_nonstrict_comparison_operator (<CODE>),
                                  SImode, operands[1], operands[2]);
}
)

(define_insn_and_split "*comp_<code>_decr"
   [(set (match_operand:SI 0 "register_operand" "=r")
         (ge_comp:SI (plus:SI (match_operand:SI 1 "register_operand" "r")
                              (const_int -1))
                     (match_operand:SI 2 "register_operand" "r")))
    (set (match_operand:SI 3 "register_operand" "=r")
         (plus:SI (match_dup 1) (const_int -1)))
   ]
   ""
   "comp_<code>_decr not split"
   ""
  [(set (match_dup 0) (match_op_dup:SI 4 [(match_dup 1) (match_dup 2)]))
 (set (match_dup 3) (plus:SI (match_dup 1) (const_int -1)))]
{
     operands[4] = gen_rtx_fmt_ee(lvx_strict_to_nonstrict_comparison_operator (<CODE>),
                                  SImode, operands[1], operands[2]);
}
)

(define_insn_and_split "*comp_<code>_incr"
   [(set (match_operand:SI 0 "register_operand" "=r")
         (gt_comp:SI (plus:SI (match_operand:SI 1 "register_operand" "r")
                              (const_int 1))
                     (match_operand:SI 2 "register_operand" "r")))
    (set (match_operand:SI 3 "register_operand" "=r")
         (plus:SI (match_dup 1) (const_int 1)))
   ]
   ""
   "comp_<code>_incr not split"
   ""
  [(set (match_dup 0) (match_op_dup:SI 4 [(match_dup 1) (match_dup 2)]))
 (set (match_dup 3) (plus:SI (match_dup 1) (const_int 1)))]
{
     operands[4] = gen_rtx_fmt_ee(lvx_strict_to_nonstrict_comparison_operator (<CODE>),
                                  SImode, operands[1], operands[2]);
}
)

(define_expand "prologue"
  [(const_int 1)]
  ""
  "
{
  lvx_expand_prologue ();
  DONE;
}")

(define_expand "epilogue"
  [(parallel [(return)
              (use (reg:DI LVX_RA_REGNO))])]
  ""
  "
{
        lvx_expand_epilogue ();
}")

(define_expand "sibcall_epilogue"
  [(parallel [(return)
              (use (reg:DI LVX_RA_REGNO))])]
  ""
  "
{
  emit_use (gen_rtx_REG (DImode, LVX_RA_REGNO));
  lvx_expand_epilogue ();
  DONE; /* DO NOT generate the ret in this case! */
}")

(define_insn "ret"
  [(return)
   (use (reg:DI LVX_RA_REGNO))]
  ""
  "ret"
  [(set_attr "type" "jump")
   (set_attr "issue" "bcu_xfer")]
)

(define_expand "untyped_call"
  [(parallel [(call (match_operand 0 "" "")
                    (const_int 0))
              (match_operand 1 "" "")
              (match_operand 2 "" "")])]
  ""
  {
    int i;
    rtx reg = gen_rtx_REG (OImode, LVX_ARGUMENT_POINTER_REGNO);

    /* We need to use call_value so the return value registers don't get
     * clobbered. */
    emit_call_insn (gen_call_value (reg, operands[0], const0_rtx));

    for (i = 0; i < XVECLEN (operands[2], 0); i++)
      {
        rtx set = XVECEXP (operands[2], 0, i);
        emit_move_insn (SET_DEST (set), SET_SRC (set));
      }

    DONE;
  })


/********** Hardware loops **************/

;; Here, we let a symbol even if there are only 17bits of immediate value.
;; FIXME AUTO: we should let gcc know operand 0 is unused after insn.
(define_insn "lvx_loopdo"
  [(unspec_volatile [(match_operand 0 "register_operand" "r")
                     (match_operand 1 "" "")] UNSPEC_DOLOOP)]
  ""
  "loopdo %0, %1"
  [(set_attr "type" "all")
   (set_attr "issue" "all")
   (set_attr "length" "4")])

;; operand 0 is the loop count pseudo register
;; operand 1 is the label to jump to at the top of the loop
(define_expand "doloop_end"
  [(match_operand 0 "" "")
   (match_operand 1 "" "")]
  "TARGET_HWLOOP"
  {
    if (GET_MODE (operands[0]) == SImode)
      emit_jump_insn (gen_doloop_end_si (operands[0], operands[1]));
    else if (GET_MODE (operands[0]) == DImode)
      emit_jump_insn (gen_doloop_end_di (operands[0], operands[1]));
    else
      FAIL;
    DONE;
  }
)

(define_insn_and_split "doloop_end_<mode>"
  [(set (pc)
        (if_then_else
          (ne (match_operand:SIDI 0 "shouldbe_register_operand" "+r,*m")
              (const_int 1))
          (label_ref (match_operand 1 "" ""))
          (pc)))
   (set (match_dup 0)
        (plus:SIDI (match_dup 0)
                   (const_int -1)))
   (unspec [(const_int 0)] UNSPEC_ENDLOOP)
   (clobber (match_scratch:SIDI 2 "=X,&r"))]
  ""
  {
    if (which_alternative != 0)
      return "#";
    return "# loopdo end";
  }
  "reload_completed && memory_operand(operands[0], VOIDmode)"
  [(set (match_dup 2) (match_dup 0))
   (set (match_dup 2)
        (plus:SIDI (match_dup 2)
                   (const_int -1)))
   (set (match_dup 0) (match_dup 2))
   (set (pc)
        (if_then_else
          (ne (match_dup 2)
              (const_int 0))
          (label_ref (match_dup 1))
          (pc)))]
  ""
)

/* ====================================================================== */
/*                            Reload stuff                                */

(define_expand "reload_in_gotoff_<mode>"
  [(parallel [(set (match_operand:P 0 "register_operand" "=&r")
                   (match_operand:P 1 "immediate_operand" "i"))
              (clobber (match_operand:P 2 "register_operand" "=&r"))])]
  "flag_pic"
    {
        emit_insn (gen_set_gotp_<mode>(operands[0]));
        emit_insn (gen_add<mode>3 (operands[0], operands[2], operands[0]));
        emit_insn (gen_add<mode>3 (operands[0], operands[0], gen_rtx_CONST (<MODE>mode,gen_rtx_UNSPEC (<MODE>mode, gen_rtvec (1, operands[1]), UNSPEC_GOTOFF))));
  DONE;
})

(include "atomic.md")

(include "scalar.md")

(include "vector.md")


(include "control.md")

(include "builtin.md")

(include "extension.md")

(include "if-convert.md")

