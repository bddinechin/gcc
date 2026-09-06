;; -*- scheme -*-
;; The LVX microarchitecture: how long each kind of instruction takes, and
;; where a consumer reads an operand late enough to shorten a dependence.
;;
;; The resources, the automata and the reservation each issue class stands for
;; are NOT here -- they are the ISA, generated into scheduling-isa.md from the
;; machine description and included before this file.  What is left is what a
;; second LVX core would have to restate and nothing else: latency, keyed on
;; the semantic "type" attribute, and the bypasses.  That is the split every
;; other GCC port makes; RISC-V has one type vocabulary and seventeen files of
;; define_insn_reservation.
;;
;; Do not gate these on LVX_1/LVX_2.  mulwq_int and mulwq_fp4 used to carry a
;; (match_test "LVX_1") guard, which was backwards: mulwq does not exist on
;; lvx-1 at all and is ALU_LITE on lvx-2, so the guard removed the reservation
;; on precisely the core that has the instruction.

(define_insn_reservation "lvx_nop_alu_tiny" 1 (and (eq_attr "type" "nop") (eq_attr "issue" "alu_tiny")) "lvx_alu_tiny_r")
(define_insn_reservation "lvx_all_all" 1 (and (eq_attr "type" "all") (eq_attr "issue" "all")) "lvx_all_r")
(define_insn_reservation "lvx_alu_alu_full" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "alu_full")) "lvx_alu_full_r")
(define_insn_reservation "lvx_alu_alu_full_x" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "alu_full_x")) "lvx_alu_full_x_r")
(define_insn_reservation "lvx_fdiv_alu_full" 15 (and (eq_attr "type" "fdiv") (eq_attr "issue" "alu_full")) "lvx_alu_full_r")
(define_insn_reservation "lvx_alu_alu_lite" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "alu_lite")) "lvx_alu_lite_r")
(define_insn_reservation "lvx_alu_alu_lite_x" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "alu_lite_x")) "lvx_alu_lite_x_r")
(define_insn_reservation "lvx_xmoveto_alu_lite_misc" 1 (and (eq_attr "type" "xmoveto") (eq_attr "issue" "alu_lite_misc")) "lvx_alu_lite_misc_r")
(define_insn_reservation "lvx_fdiv_alu_lite" 15 (and (eq_attr "type" "fdiv") (eq_attr "issue" "alu_lite")) "lvx_alu_lite_r")
(define_insn_reservation "lvx_alu_alu_lite_y" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "alu_lite_y")) "lvx_alu_lite_y_r")
(define_insn_reservation "lvx_alu_x2_alu_lite" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "x2_alu_lite")) "lvx_x2_alu_lite_r")
(define_insn_reservation "lvx_alu_x2_alu_lite_x" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "x2_alu_lite_x")) "lvx_x2_alu_lite_x_r")
(define_insn_reservation "lvx_alu_alu_tiny" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "alu_tiny")) "lvx_alu_tiny_r")
(define_insn_reservation "lvx_ghost_alu_tiny" 0 (and (eq_attr "type" "ghost") (eq_attr "issue" "alu_tiny")) "lvx_alu_tiny_r")
(define_insn_reservation "lvx_alu_alu_tiny_x" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "alu_tiny_x")) "lvx_alu_tiny_x_r")
(define_insn_reservation "lvx_alu_alu_tiny_y" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "alu_tiny_y")) "lvx_alu_tiny_y_r")
(define_insn_reservation "lvx_alu_x2_alu_tiny" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "x2_alu_tiny")) "lvx_x2_alu_tiny_r")
(define_insn_reservation "lvx_ghost_x2_alu_tiny" 0 (and (eq_attr "type" "ghost") (eq_attr "issue" "x2_alu_tiny")) "lvx_x2_alu_tiny_r")
(define_insn_reservation "lvx_alu_x4_alu_tiny" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "x4_alu_tiny")) "lvx_x4_alu_tiny_r")
(define_insn_reservation "lvx_alu_x4_alu_tiny_x" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "x4_alu_tiny_x")) "lvx_x4_alu_tiny_x_r")
(define_insn_reservation "lvx_xmoveto_x2_alu_lite_misc" 1 (and (eq_attr "type" "xmoveto") (eq_attr "issue" "x2_alu_lite_misc")) "lvx_x2_alu_lite_misc_r")
(define_insn_reservation "lvx_alu_alu_tiny__alu_lite" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "alu_tiny__alu_lite")) "lvx_alu_tiny__alu_lite_r")
(define_insn_reservation "lvx_alu_x2_alu_tiny__x2_alu_lite" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "x2_alu_tiny__x2_alu_lite")) "lvx_x2_alu_tiny__x2_alu_lite_r")
(define_insn_reservation "lvx_cache_lsu" 1 (and (eq_attr "type" "cache") (eq_attr "issue" "lsu")) "lvx_lsu_r")
(define_insn_reservation "lvx_cache_lsu_x" 1 (and (eq_attr "type" "cache") (eq_attr "issue" "lsu_x")) "lvx_lsu_x_r")
(define_insn_reservation "lvx_cache_lsu_y" 1 (and (eq_attr "type" "cache") (eq_attr "issue" "lsu_y")) "lvx_lsu_y_r")
(define_insn_reservation "lvx_cache_lsu2_memw" 1 (and (eq_attr "type" "cache") (eq_attr "issue" "lsu2_memw")) "lvx_lsu2_memw_r")
(define_insn_reservation "lvx_cache_lsu2_memw_x" 1 (and (eq_attr "type" "cache") (eq_attr "issue" "lsu2_memw_x")) "lvx_lsu2_memw_x_r")
(define_insn_reservation "lvx_cache_lsu2_memw_y" 1 (and (eq_attr "type" "cache") (eq_attr "issue" "lsu2_memw_y")) "lvx_lsu2_memw_y_r")
(define_insn_reservation "lvx_prefetch_lsu" 1 (and (eq_attr "type" "prefetch") (eq_attr "issue" "lsu")) "lvx_lsu_r")
(define_insn_reservation "lvx_prefetch_lsu_x" 1 (and (eq_attr "type" "prefetch") (eq_attr "issue" "lsu_x")) "lvx_lsu_x_r")
(define_insn_reservation "lvx_prefetch_lsu_y" 1 (and (eq_attr "type" "prefetch") (eq_attr "issue" "lsu_y")) "lvx_lsu_y_r")
(define_insn_reservation "lvx_xload_lsu" 4 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_r")
(define_insn_reservation "lvx_xload_lsu_x" 4 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu_x") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_x_r")
(define_insn_reservation "lvx_xload_lsu_y" 4 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu_y") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_y_r")
(define_insn_reservation "lvx_xload_lsu_nd" 4 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_xload_lsu_x_nd" 4 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu_x") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_x_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_xload_lsu_y_nd" 4 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu_y") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_y_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_load_lsu_auxw" 3 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_r")
(define_insn_reservation "lvx_load_lsu_auxw_x" 3 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw_x") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_x_r")
(define_insn_reservation "lvx_load_lsu_auxw_y" 3 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw_y") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_y_r")
(define_insn_reservation "lvx_load_lsu_auxw_nd" 3 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_load_lsu_auxw_x_nd" 3 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw_x") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_x_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_load_lsu_auxw_y_nd" 3 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw_y") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_y_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_loadu_lsu_auxw" 24 (and (eq_attr "type" "loadu") (eq_attr "issue" "lsu_auxw") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_r")
(define_insn_reservation "lvx_loadu_lsu_auxw_x" 24 (and (eq_attr "type" "loadu") (eq_attr "issue" "lsu_auxw_x") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_x_r")
(define_insn_reservation "lvx_loadu_lsu_auxw_y" 24 (and (eq_attr "type" "loadu") (eq_attr "issue" "lsu_auxw_y") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_y_r")
(define_insn_reservation "lvx_loadu_lsu_auxw_nd" 24 (and (eq_attr "type" "loadu") (eq_attr "issue" "lsu_auxw") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_loadu_lsu_auxw_x_nd" 24 (and (eq_attr "type" "loadu") (eq_attr "issue" "lsu_auxw_x") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_x_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_loadu_lsu_auxw_y_nd" 24 (and (eq_attr "type" "loadu") (eq_attr "issue" "lsu_auxw_y") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_y_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_xloadu_lsu" 24 (and (eq_attr "type" "xloadu") (eq_attr "issue" "lsu") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_r")
(define_insn_reservation "lvx_xloadu_lsu_x" 24 (and (eq_attr "type" "xloadu") (eq_attr "issue" "lsu_x") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_x_r")
(define_insn_reservation "lvx_xloadu_lsu_y" 24 (and (eq_attr "type" "xloadu") (eq_attr "issue" "lsu_y") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_y_r")
(define_insn_reservation "lvx_xloadu_lsu_nd" 24 (and (eq_attr "type" "xloadu") (eq_attr "issue" "lsu") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_xloadu_lsu_x_nd" 24 (and (eq_attr "type" "xloadu") (eq_attr "issue" "lsu_x") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_x_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_xloadu_lsu_y_nd" 24 (and (eq_attr "type" "xloadu") (eq_attr "issue" "lsu_y") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_y_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_store_lsu_memw_auxr" 1 (and (eq_attr "type" "store") (eq_attr "issue" "lsu_memw_auxr")) "lvx_lsu_memw_auxr_r")
(define_insn_reservation "lvx_store_lsu_memw_auxr_x" 1 (and (eq_attr "type" "store") (eq_attr "issue" "lsu_memw_auxr_x")) "lvx_lsu_memw_auxr_x_r")
(define_insn_reservation "lvx_store_lsu_memw_auxr_y" 1 (and (eq_attr "type" "store") (eq_attr "issue" "lsu_memw_auxr_y")) "lvx_lsu_memw_auxr_y_r")
(define_insn_reservation "lvx_xstore_lsu_memw_accr" 1 (and (eq_attr "type" "xstore") (eq_attr "issue" "lsu_memw_accr")) "lvx_lsu_memw_accr_r")
(define_insn_reservation "lvx_xstore_lsu_memw_accr_x" 1 (and (eq_attr "type" "xstore") (eq_attr "issue" "lsu_memw_accr_x")) "lvx_lsu_memw_accr_x_r")
(define_insn_reservation "lvx_xstore_lsu_memw_accr_y" 1 (and (eq_attr "type" "xstore") (eq_attr "issue" "lsu_memw_accr_y")) "lvx_lsu_memw_accr_y_r")
(define_insn_reservation "lvx_aload_lsu_memw_auxw" 24 (and (eq_attr "type" "aload") (eq_attr "issue" "lsu_memw_auxw")) "lvx_lsu_memw_auxw_r")
(define_insn_reservation "lvx_aload_lsu_memw_auxw_x" 24 (and (eq_attr "type" "aload") (eq_attr "issue" "lsu_memw_auxw_x")) "lvx_lsu_memw_auxw_x_r")
(define_insn_reservation "lvx_aload_lsu_memw_auxw_y" 24 (and (eq_attr "type" "aload") (eq_attr "issue" "lsu_memw_auxw_y")) "lvx_lsu_memw_auxw_y_r")
(define_insn_reservation "lvx_aloadc_lsu2_memw_auxw" 24 (and (eq_attr "type" "aloadc") (eq_attr "issue" "lsu2_memw_auxw")) "lvx_lsu2_memw_auxw_r")
(define_insn_reservation "lvx_aloadc_lsu2_memw_auxw_x" 24 (and (eq_attr "type" "aloadc") (eq_attr "issue" "lsu2_memw_auxw_x")) "lvx_lsu2_memw_auxw_x_r")
(define_insn_reservation "lvx_aloadc_lsu2_memw_auxw_y" 24 (and (eq_attr "type" "aloadc") (eq_attr "issue" "lsu2_memw_auxw_y")) "lvx_lsu2_memw_auxw_y_r")
(define_insn_reservation "lvx_atomic_lsu2_memw_auxr_auxw" 24 (and (eq_attr "type" "atomic") (eq_attr "issue" "lsu2_memw_auxr_auxw")) "lvx_lsu2_memw_auxr_auxw_r")
(define_insn_reservation "lvx_atomic_lsu2_memw_auxr_auxw_x" 24 (and (eq_attr "type" "atomic") (eq_attr "issue" "lsu2_memw_auxr_auxw_x")) "lvx_lsu2_memw_auxr_auxw_x_r")
(define_insn_reservation "lvx_atomic_lsu2_memw_auxr_auxw_y" 24 (and (eq_attr "type" "atomic") (eq_attr "issue" "lsu2_memw_auxr_auxw_y")) "lvx_lsu2_memw_auxr_auxw_y_r")
(define_insn_reservation "lvx_copy_lsu_auxr_auxw" 3 (and (eq_attr "type" "copy") (eq_attr "issue" "lsu_auxr_auxw")) "lvx_lsu_auxr_auxw_r")
(define_insn_reservation "lvx_imul_alu_lite" 2 (and (eq_attr "type" "imul") (eq_attr "issue" "alu_lite")) "lvx_alu_lite_r")
(define_insn_reservation "lvx_imul_alu_lite_x" 2 (and (eq_attr "type" "imul") (eq_attr "issue" "alu_lite_x")) "lvx_alu_lite_x_r")
(define_insn_reservation "lvx_imul_alu_lite_y" 2 (and (eq_attr "type" "imul") (eq_attr "issue" "alu_lite_y")) "lvx_alu_lite_y_r")
(define_insn_reservation "lvx_fmuls_alu_lite" 3 (and (eq_attr "type" "fmuls") (eq_attr "issue" "alu_lite")) "lvx_alu_lite_r")
(define_insn_reservation "lvx_fmuld_alu_lite" 4 (and (eq_attr "type" "fmuld") (eq_attr "issue" "alu_lite")) "lvx_alu_lite_r")
(define_insn_reservation "lvx_fdotp_alu_full" 4 (and (eq_attr "type" "fdotp") (eq_attr "issue" "alu_full")) "lvx_alu_full_r")
(define_insn_reservation "lvx_fcvt_alu_lite" 4 (and (eq_attr "type" "fcvt") (eq_attr "issue" "alu_lite")) "lvx_alu_lite_r")
(define_insn_reservation "lvx_imadd_alu_lite" 2 (and (eq_attr "type" "imadd") (eq_attr "issue" "alu_lite")) "lvx_alu_lite_r")
(define_insn_reservation "lvx_imadd_alu_lite_x" 2 (and (eq_attr "type" "imadd") (eq_attr "issue" "alu_lite_x")) "lvx_alu_lite_x_r")
(define_insn_reservation "lvx_imadd_alu_lite_y" 2 (and (eq_attr "type" "imadd") (eq_attr "issue" "alu_lite_y")) "lvx_alu_lite_y_r")
(define_insn_reservation "lvx_fmadds_alu_lite" 3 (and (eq_attr "type" "fmadds") (eq_attr "issue" "alu_lite")) "lvx_alu_lite_r")
(define_insn_reservation "lvx_fmaddd_alu_lite" 4 (and (eq_attr "type" "fmaddd") (eq_attr "issue" "alu_lite")) "lvx_alu_lite_r")
(define_insn_reservation "lvx_fdmda_alu_full" 4 (and (eq_attr "type" "fdmda") (eq_attr "issue" "alu_full")) "lvx_alu_full_r")
(define_insn_reservation "lvx_branch_bcu_brrp" 1 (and (eq_attr "type" "branch") (eq_attr "issue" "bcu_brrp") (match_test "TARGET_DUAL_BCU")) "lvx_bcu_brrp_r")
(define_insn_reservation "lvx_jump_bcu_xfer" 1 (and (eq_attr "type" "jump") (eq_attr "issue" "bcu_xfer") (match_test "TARGET_DUAL_BCU")) "lvx_bcu_xfer_r")
(define_insn_reservation "lvx_ijump_bcu_xfer_brrp" 1 (and (eq_attr "type" "ijump") (eq_attr "issue" "bcu_xfer_brrp") (match_test "TARGET_DUAL_BCU")) "lvx_bcu_xfer_brrp_r")
(define_insn_reservation "lvx_branch2_bcu2" 1 (and (eq_attr "type" "branch2") (eq_attr "issue" "bcu2")) "lvx_bcu2_r")
(define_insn_reservation "lvx_sysget_bcu2_tiny_lsu" 1 (and (eq_attr "type" "sysget") (eq_attr "issue" "bcu2_tiny_lsu")) "lvx_bcu2_tiny_lsu_r")
(define_insn_reservation "lvx_branch_bcu_brrp_nd" 1 (and (eq_attr "type" "branch") (eq_attr "issue" "bcu_brrp") (match_test "!TARGET_DUAL_BCU")) "lvx_bcu_brrp_r + lvx_bcu_x2_u")
(define_insn_reservation "lvx_jump_bcu_xfer_nd" 1 (and (eq_attr "type" "jump") (eq_attr "issue" "bcu_xfer") (match_test "!TARGET_DUAL_BCU")) "lvx_bcu_xfer_r + lvx_bcu_x2_u")
(define_insn_reservation "lvx_ijump_bcu_xfer_brrp_nd" 1 (and (eq_attr "type" "ijump") (eq_attr "issue" "bcu_xfer_brrp") (match_test "!TARGET_DUAL_BCU")) "lvx_bcu_xfer_brrp_r + lvx_bcu_x2_u")
(define_insn_reservation "lvx_xmovef_ext_misc_auxw" 3 (and (eq_attr "type" "xmovef") (eq_attr "issue" "ext_misc_auxw")) "lvx_ext_misc_auxw_r")
(define_insn_reservation "lvx_xcopy_ext_misc_auxw" 1 (and (eq_attr "type" "xcopy") (eq_attr "issue" "ext_misc_auxw")) "lvx_ext_misc_auxw_r")
(define_insn_reservation "lvx_ext_ext" 1 (and (eq_attr "type" "ext") (eq_attr "issue" "ext")) "lvx_ext_r")
(define_insn_reservation "lvx_ext_int_ext" 3 (and (eq_attr "type" "ext_int") (eq_attr "issue" "ext")) "lvx_ext_r")
(define_insn_reservation "lvx_ext_float_ext" 4 (and (eq_attr "type" "ext_float") (eq_attr "issue" "ext")) "lvx_ext_r")

;; Bypasses.
;;
;; Both of these are facts the machine description states per operand: an
;; operand read at a stage later than RR shortens the dependence into it by the
;; difference.  A store reads its value one cycle late, and the integer MAC
;; reads its accumulator one cycle late.  The C predicates are still needed --
;; they answer which operand of the consumer is the one reading the produced
;; value, which no attribute can say.
;;
;; The producer sets are grouped by latency, which is why they name several
;; types: what matters is when the result lands, not what computed it.

;; The stores read their input one cycle later than other execution units:
;; sd is LOAD.1 on its address and LOAD.2 on its value, and xso the same, so
;; both store families are consumers here.  Every load form is a producer.
(define_bypass 1 "lvx_imul_*,lvx_imadd_*"
                 "lvx_store_*,lvx_xstore_*"
                 "lvx_stored_value_bypass_p")
(define_bypass 2 "lvx_fmuls_*,lvx_fmadds_*"
                 "lvx_store_*,lvx_xstore_*"
                 "lvx_stored_value_bypass_p")
(define_bypass 3 "lvx_fmuld_*,lvx_fmaddd_*,lvx_fcvt_*,lvx_fdotp_*,lvx_fdmda_*"
                 "lvx_store_*,lvx_xstore_*"
                 "lvx_stored_value_bypass_p")
(define_bypass 2 "lvx_load_*,lvx_loadu_*,lvx_xload_*,lvx_xloadu_*"
                 "lvx_store_*,lvx_xstore_*"
                 "lvx_stored_value_bypass_p")

;; The integer MAC accumulator reads its input a cycle later than other units.
(define_bypass 1 "lvx_imul_*,lvx_imadd_*"
                 "lvx_imadd_*"
                 "lvx_accumulator_bypass_p")
(define_bypass 2 "lvx_load_*,lvx_loadu_*,lvx_xload_*,lvx_xloadu_*"
                 "lvx_imadd_*"
                 "lvx_accumulator_bypass_p")
