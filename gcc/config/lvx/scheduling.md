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
;; A load also carries a "variant" (types.md), because an uncached load is
;; LD.U rather than some "LDU" of its own.  The variant changes nothing but
;; the latency, so this is the one file that reads it.
;;
;; Do not gate these on LVX_1/LVX_2.  mulwq_int and mulwq_fp4 used to carry a
;; (match_test "LVX_1") guard, which was backwards: mulwq does not exist on
;; lvx-1 at all and is ALU_LITE on lvx-2, so the guard removed the reservation
;; on precisely the core that has the instruction.

(define_insn_reservation "lvx_all_all" 5 (and (eq_attr "type" "all") (eq_attr "issue" "all")) "lvx_all_r")
(define_insn_reservation "lvx_alu_full" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "full")) "lvx_full_r")
;; The integer divider.  divmodw/divmoduw/divmodd/divmodud write their result
;; at stage SF (12) against a read at RR (1), so latency 11 -- an order of
;; magnitude off the 1 they had while typed plain alu.  ALU_FULL, so one per
;; bundle: N of them cost N bundles of issue plus 11, which is why four
;; native divides beat a 32-step STSU chain and sixteen do not.
(define_insn_reservation "lvx_idiv_full" 11 (and (eq_attr "type" "idiv") (eq_attr "issue" "full")) "lvx_full_r")
(define_insn_reservation "lvx_alu_full_x" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "full_x")) "lvx_full_x_r")
;; The FP divider: fdiv{h,w,d} and fsqrt{h,w,d} commit at FD (stage 14)
;; against a read at RR, so latency 13.  They are the only instructions this
;; type covers -- the fsrec*/fsrsr* seeds are latency 1 and typed alu.
(define_insn_reservation "lvx_fdiv_full" 13 (and (eq_attr "type" "fdiv") (eq_attr "issue" "full")) "lvx_full_r")
(define_insn_reservation "lvx_alu_lite" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "lite")) "lvx_lite_r")
(define_insn_reservation "lvx_alu_lite_x" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "lite_x")) "lvx_lite_x_r")
(define_insn_reservation "lvx_xmoveto_lite_misc" 1 (and (eq_attr "type" "xmoveto") (eq_attr "issue" "lite_misc")) "lvx_lite_misc_r")
(define_insn_reservation "lvx_fdiv_lite" 13 (and (eq_attr "type" "fdiv") (eq_attr "issue" "lite")) "lvx_lite_r")
(define_insn_reservation "lvx_alu_lite_x2" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "lite_x2")) "lvx_lite_x2_r")
(define_insn_reservation "lvx_alu_lite2" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "lite2")) "lvx_lite2_r")
(define_insn_reservation "lvx_alu_lite2_x2" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "lite2_x2")) "lvx_lite2_x2_r")
(define_insn_reservation "lvx_alu_tiny" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "tiny")) "lvx_tiny_r")
(define_insn_reservation "lvx_ghost_tiny" 0 (and (eq_attr "type" "ghost") (eq_attr "issue" "tiny")) "lvx_tiny_r")
(define_insn_reservation "lvx_alu_tiny_x" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "tiny_x")) "lvx_tiny_x_r")
(define_insn_reservation "lvx_alu_tiny_x2" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "tiny_x2")) "lvx_tiny_x2_r")
(define_insn_reservation "lvx_alu_tiny2" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "tiny2")) "lvx_tiny2_r")
(define_insn_reservation "lvx_ghost_tiny2" 0 (and (eq_attr "type" "ghost") (eq_attr "issue" "tiny2")) "lvx_tiny2_r")
(define_insn_reservation "lvx_alu_tiny4" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "tiny4")) "lvx_tiny4_r")
(define_insn_reservation "lvx_alu_tiny4_x4" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "tiny4_x4")) "lvx_tiny4_x4_r")
(define_insn_reservation "lvx_xmoveto_lite2_misc" 1 (and (eq_attr "type" "xmoveto") (eq_attr "issue" "lite2_misc")) "lvx_lite2_misc_r")
(define_insn_reservation "lvx_alu_tiny__lite" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "tiny__lite")) "lvx_tiny__lite_r")
(define_insn_reservation "lvx_alu_tiny2__lite2" 1 (and (eq_attr "type" "alu") (eq_attr "issue" "tiny2__lite2")) "lvx_tiny2__lite2_r")
(define_insn_reservation "lvx_cache_lsu" 1 (and (eq_attr "type" "cache") (eq_attr "issue" "lsu")) "lvx_lsu_r")
(define_insn_reservation "lvx_cache_lsu_x" 1 (and (eq_attr "type" "cache") (eq_attr "issue" "lsu_x")) "lvx_lsu_x_r")
(define_insn_reservation "lvx_cache_lsu_x2" 1 (and (eq_attr "type" "cache") (eq_attr "issue" "lsu_x2")) "lvx_lsu_x2_r")
(define_insn_reservation "lvx_cache_lsu2_memw" 1 (and (eq_attr "type" "cache") (eq_attr "issue" "lsu2_memw")) "lvx_lsu2_memw_r")
(define_insn_reservation "lvx_cache_lsu2_memw_x" 1 (and (eq_attr "type" "cache") (eq_attr "issue" "lsu2_memw_x")) "lvx_lsu2_memw_x_r")
(define_insn_reservation "lvx_cache_lsu2_memw_x2" 1 (and (eq_attr "type" "cache") (eq_attr "issue" "lsu2_memw_x2")) "lvx_lsu2_memw_x2_r")
(define_insn_reservation "lvx_prefetch_lsu" 1 (and (eq_attr "type" "prefetch") (eq_attr "issue" "lsu")) "lvx_lsu_r")
(define_insn_reservation "lvx_prefetch_lsu_x" 1 (and (eq_attr "type" "prefetch") (eq_attr "issue" "lsu_x")) "lvx_lsu_x_r")
(define_insn_reservation "lvx_prefetch_lsu_x2" 1 (and (eq_attr "type" "prefetch") (eq_attr "issue" "lsu_x2")) "lvx_lsu_x2_r")
(define_insn_reservation "lvx_xload_lsu" 4 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu") (eq_attr "variant" "cached,speculate") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_r")
(define_insn_reservation "lvx_xload_lsu_x" 4 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu_x") (eq_attr "variant" "cached,speculate") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_x_r")
(define_insn_reservation "lvx_xload_lsu_x2" 4 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu_x2") (eq_attr "variant" "cached,speculate") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_x2_r")
(define_insn_reservation "lvx_xload_lsu_nd" 4 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu") (eq_attr "variant" "cached,speculate") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_xload_lsu_x_nd" 4 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu_x") (eq_attr "variant" "cached,speculate") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_x_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_xload_lsu_y_nd" 4 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu_x2") (eq_attr "variant" "cached,speculate") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_x2_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_load_lsu_auxw" 3 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw") (eq_attr "variant" "cached,speculate") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_r")
(define_insn_reservation "lvx_load_lsu_auxw_x" 3 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw_x") (eq_attr "variant" "cached,speculate") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_x_r")
(define_insn_reservation "lvx_load_lsu_auxw_x2" 3 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw_x2") (eq_attr "variant" "cached,speculate") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_x2_r")
(define_insn_reservation "lvx_load_lsu_auxw_nd" 3 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw") (eq_attr "variant" "cached,speculate") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_load_lsu_auxw_x_nd" 3 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw_x") (eq_attr "variant" "cached,speculate") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_x_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_load_lsu_auxw_y_nd" 3 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw_x2") (eq_attr "variant" "cached,speculate") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_x2_r + lvx_lsu_x2_u")
;; The same loads again, uncached.  LD.U is the same LD with the "variant"
;; modifier set, so it keeps the "type" and the "issue" of its cached self and
;; differs only here, in what it costs.
;;
;; 15, because an uncached load commits at SR -- "stream return", the last
;; stage in Processor.yml's pipeline, at cycle 16 -- and a latency is
;; (write stage - RR), so 16 - 1.  The same arithmetic gives FDIV* its 13 from
;; FD=14 and divmod* its 11 from SF=12, both of which this file already has
;; right.  It said 24 until 2026-10-16, which is nobody's stage: over-modelling,
;; so never a wrong answer, but nine cycles of imaginary latency on the one
;; instruction a software pipeliner exists to cover.  MDS does not yet declare
;; it per variant -- Opcode@timing comes from the Behavior's `stage` annotations
;; and a `.U` load shares its Opcode record with the cached form -- which is why
;; check-scheduling.py did not catch this: it compares against the cached
;; figure and only flags latencies that are too *low*.
(define_insn_reservation "lvx_load_uncached_lsu_auxw" 15 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw") (eq_attr "variant" "uncached,preload") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_r")
(define_insn_reservation "lvx_load_uncached_lsu_auxw_x" 15 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw_x") (eq_attr "variant" "uncached,preload") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_x_r")
(define_insn_reservation "lvx_load_uncached_lsu_auxw_x2" 15 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw_x2") (eq_attr "variant" "uncached,preload") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_x2_r")
(define_insn_reservation "lvx_load_uncached_lsu_auxw_nd" 15 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw") (eq_attr "variant" "uncached,preload") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_load_uncached_lsu_auxw_x_nd" 15 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw_x") (eq_attr "variant" "uncached,preload") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_x_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_load_uncached_lsu_auxw_y_nd" 15 (and (eq_attr "type" "load") (eq_attr "issue" "lsu_auxw_x2") (eq_attr "variant" "uncached,preload") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_x2_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_xload_uncached_lsu" 15 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu") (eq_attr "variant" "uncached,preload") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_r")
(define_insn_reservation "lvx_xload_uncached_lsu_x" 15 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu_x") (eq_attr "variant" "uncached,preload") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_x_r")
(define_insn_reservation "lvx_xload_uncached_lsu_x2" 15 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu_x2") (eq_attr "variant" "uncached,preload") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_x2_r")
(define_insn_reservation "lvx_xload_uncached_lsu_nd" 15 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu") (eq_attr "variant" "uncached,preload") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_xload_uncached_lsu_x_nd" 15 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu_x") (eq_attr "variant" "uncached,preload") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_x_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_xload_uncached_lsu_y_nd" 15 (and (eq_attr "type" "xload") (eq_attr "issue" "lsu_x2") (eq_attr "variant" "uncached,preload") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_x2_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_store_lsu_memw_auxr" 1 (and (eq_attr "type" "store") (eq_attr "issue" "lsu_memw_auxr")) "lvx_lsu_memw_auxr_r")
(define_insn_reservation "lvx_store_lsu_memw_auxr_x" 1 (and (eq_attr "type" "store") (eq_attr "issue" "lsu_memw_auxr_x")) "lvx_lsu_memw_auxr_x_r")
(define_insn_reservation "lvx_store_lsu_memw_auxr_x2" 1 (and (eq_attr "type" "store") (eq_attr "issue" "lsu_memw_auxr_x2")) "lvx_lsu_memw_auxr_x2_r")
(define_insn_reservation "lvx_xstore_lsu_memw_accr" 1 (and (eq_attr "type" "xstore") (eq_attr "issue" "lsu_memw_accr")) "lvx_lsu_memw_accr_r")
(define_insn_reservation "lvx_xstore_lsu_memw_accr_x" 1 (and (eq_attr "type" "xstore") (eq_attr "issue" "lsu_memw_accr_x")) "lvx_lsu_memw_accr_x_r")
(define_insn_reservation "lvx_xstore_lsu_memw_accr_x2" 1 (and (eq_attr "type" "xstore") (eq_attr "issue" "lsu_memw_accr_x2")) "lvx_lsu_memw_accr_x2_r")
(define_insn_reservation "lvx_aload_lsu_memw_auxw" 24 (and (eq_attr "type" "aload") (eq_attr "issue" "lsu_memw_auxw")) "lvx_lsu_memw_auxw_r")
(define_insn_reservation "lvx_aload_lsu_memw_auxw_x" 24 (and (eq_attr "type" "aload") (eq_attr "issue" "lsu_memw_auxw_x")) "lvx_lsu_memw_auxw_x_r")
(define_insn_reservation "lvx_aload_lsu_memw_auxw_x2" 24 (and (eq_attr "type" "aload") (eq_attr "issue" "lsu_memw_auxw_x2")) "lvx_lsu_memw_auxw_x2_r")
(define_insn_reservation "lvx_aloadc_lsu2_memw_auxw" 24 (and (eq_attr "type" "aloadc") (eq_attr "issue" "lsu2_memw_auxw")) "lvx_lsu2_memw_auxw_r")
(define_insn_reservation "lvx_aloadc_lsu2_memw_auxw_x" 24 (and (eq_attr "type" "aloadc") (eq_attr "issue" "lsu2_memw_auxw_x")) "lvx_lsu2_memw_auxw_x_r")
(define_insn_reservation "lvx_aloadc_lsu2_memw_auxw_x2" 24 (and (eq_attr "type" "aloadc") (eq_attr "issue" "lsu2_memw_auxw_x2")) "lvx_lsu2_memw_auxw_x2_r")
;; The non-returning atomics: as<op> and the min/max/dus store forms write no
;; register, so MDS gives them LSU2_MEMW_AUXR with no AUXW.  Reserving AUXW as
;; well -- which they did when they first reused the al<op> issue value -- is
;; over-reserving, and check-scheduling reports it as lost bundle density.
;; These three must be written by hand: md-scheduling.pl generates the
;; automaton units and the issue attribute, never which `type' an issue class
;; pairs with.
(define_insn_reservation "lvx_atomic_lsu2_memw_auxr" 24 (and (eq_attr "type" "atomic") (eq_attr "issue" "lsu2_memw_auxr")) "lvx_lsu2_memw_auxr_r")
(define_insn_reservation "lvx_atomic_lsu2_memw_auxr_x" 24 (and (eq_attr "type" "atomic") (eq_attr "issue" "lsu2_memw_auxr_x")) "lvx_lsu2_memw_auxr_x_r")
(define_insn_reservation "lvx_atomic_lsu2_memw_auxr_x2" 24 (and (eq_attr "type" "atomic") (eq_attr "issue" "lsu2_memw_auxr_x2")) "lvx_lsu2_memw_auxr_x2_r")
(define_insn_reservation "lvx_atomic_lsu2_memw_auxr_auxw" 24 (and (eq_attr "type" "atomic") (eq_attr "issue" "lsu2_memw_auxr_auxw")) "lvx_lsu2_memw_auxr_auxw_r")
(define_insn_reservation "lvx_atomic_lsu2_memw_auxr_auxw_x" 24 (and (eq_attr "type" "atomic") (eq_attr "issue" "lsu2_memw_auxr_auxw_x")) "lvx_lsu2_memw_auxr_auxw_x_r")
(define_insn_reservation "lvx_atomic_lsu2_memw_auxr_auxw_x2" 24 (and (eq_attr "type" "atomic") (eq_attr "issue" "lsu2_memw_auxr_auxw_x2")) "lvx_lsu2_memw_auxr_auxw_x2_r")
(define_insn_reservation "lvx_copy_lsu_auxr_auxw" 3 (and (eq_attr "type" "copy") (eq_attr "issue" "lsu_auxr_auxw")) "lvx_lsu_auxr_auxw_r")
(define_insn_reservation "lvx_imul_lite" 2 (and (eq_attr "type" "imul") (eq_attr "issue" "lite")) "lvx_lite_r")
(define_insn_reservation "lvx_imul_lite_x" 2 (and (eq_attr "type" "imul") (eq_attr "issue" "lite_x")) "lvx_lite_x_r")
(define_insn_reservation "lvx_imul_lite2" 2 (and (eq_attr "type" "imul") (eq_attr "issue" "lite2")) "lvx_lite2_r")
(define_insn_reservation "lvx_fmuls_lite" 4 (and (eq_attr "type" "fmuls") (eq_attr "issue" "lite")) "lvx_lite_r")
(define_insn_reservation "lvx_fmuls_lite_x" 4 (and (eq_attr "type" "fmuls") (eq_attr "issue" "lite_x")) "lvx_lite_x_r")
(define_insn_reservation "lvx_fmuld_lite" 4 (and (eq_attr "type" "fmuld") (eq_attr "issue" "lite")) "lvx_lite_r")
(define_insn_reservation "lvx_fmuld_lite_x" 4 (and (eq_attr "type" "fmuld") (eq_attr "issue" "lite_x")) "lvx_lite_x_r")
(define_insn_reservation "lvx_fcvt_lite" 4 (and (eq_attr "type" "fcvt") (eq_attr "issue" "lite")) "lvx_lite_r")
(define_insn_reservation "lvx_imadd_lite" 2 (and (eq_attr "type" "imadd") (eq_attr "issue" "lite")) "lvx_lite_r")
(define_insn_reservation "lvx_imadd_lite_x" 2 (and (eq_attr "type" "imadd") (eq_attr "issue" "lite_x")) "lvx_lite_x_r")
(define_insn_reservation "lvx_imadd_lite2" 2 (and (eq_attr "type" "imadd") (eq_attr "issue" "lite2")) "lvx_lite2_r")
(define_insn_reservation "lvx_fmadds_lite" 4 (and (eq_attr "type" "fmadds") (eq_attr "issue" "lite")) "lvx_lite_r")
(define_insn_reservation "lvx_fmadds_lite_x" 4 (and (eq_attr "type" "fmadds") (eq_attr "issue" "lite_x")) "lvx_lite_x_r")
(define_insn_reservation "lvx_fmaddd_lite" 4 (and (eq_attr "type" "fmaddd") (eq_attr "issue" "lite")) "lvx_lite_r")
(define_insn_reservation "lvx_fmaddd_lite_x" 4 (and (eq_attr "type" "fmaddd") (eq_attr "issue" "lite_x")) "lvx_lite_x_r")
(define_insn_reservation "lvx_branch_bcu_brrp" 1 (and (eq_attr "type" "branch") (eq_attr "issue" "bcu_brrp") (match_test "TARGET_DUAL_BCU")) "lvx_bcu_brrp_r")
(define_insn_reservation "lvx_branch_bcu_brrp2" 1 (and (eq_attr "type" "branch") (eq_attr "issue" "bcu_brrp2") (match_test "TARGET_DUAL_BCU")) "lvx_bcu_brrp2_r")
(define_insn_reservation "lvx_jump_bcu_xfer" 1 (and (eq_attr "type" "jump") (eq_attr "issue" "bcu_xfer") (match_test "TARGET_DUAL_BCU")) "lvx_bcu_xfer_r")
(define_insn_reservation "lvx_ijump_bcu_xfer_brrp" 1 (and (eq_attr "type" "ijump") (eq_attr "issue" "bcu_xfer_brrp") (match_test "TARGET_DUAL_BCU")) "lvx_bcu_xfer_brrp_r")
(define_insn_reservation "lvx_branch_bcu2_x" 1 (and (eq_attr "type" "branch") (eq_attr "issue" "bcu2_x")) "lvx_bcu2_x_r")
(define_insn_reservation "lvx_jump_bcu2_x" 1 (and (eq_attr "type" "jump") (eq_attr "issue" "bcu2_x")) "lvx_bcu2_x_r")
(define_insn_reservation "lvx_branch2_bcu2" 5 (and (eq_attr "type" "branch2") (eq_attr "issue" "bcu2")) "lvx_bcu2_r")
(define_insn_reservation "lvx_sysget_bcu2_tiny_lsu" 1 (and (eq_attr "type" "sysget") (eq_attr "issue" "bcu2_tiny_lsu")) "lvx_bcu2_tiny_lsu_r")
(define_insn_reservation "lvx_branch_bcu_brrp_nd" 1 (and (eq_attr "type" "branch") (eq_attr "issue" "bcu_brrp") (match_test "!TARGET_DUAL_BCU")) "lvx_bcu_brrp_r + lvx_bcu_x2_u")
(define_insn_reservation "lvx_branch_bcu_brrp2_nd" 1 (and (eq_attr "type" "branch") (eq_attr "issue" "bcu_brrp2") (match_test "!TARGET_DUAL_BCU")) "lvx_bcu_brrp2_r + lvx_bcu_x2_u")
(define_insn_reservation "lvx_jump_bcu_xfer_nd" 1 (and (eq_attr "type" "jump") (eq_attr "issue" "bcu_xfer") (match_test "!TARGET_DUAL_BCU")) "lvx_bcu_xfer_r + lvx_bcu_x2_u")
(define_insn_reservation "lvx_ijump_bcu_xfer_brrp_nd" 1 (and (eq_attr "type" "ijump") (eq_attr "issue" "bcu_xfer_brrp") (match_test "!TARGET_DUAL_BCU")) "lvx_bcu_xfer_brrp_r + lvx_bcu_x2_u")
(define_insn_reservation "lvx_xmovef_ext_misc_auxw" 3 (and (eq_attr "type" "xmovef") (eq_attr "issue" "ext_misc_auxw")) "lvx_ext_misc_auxw_r")
(define_insn_reservation "lvx_xcopy_ext_misc_auxw" 3 (and (eq_attr "type" "xcopy") (eq_attr "issue" "ext_misc_auxw")) "lvx_ext_misc_auxw_r")
(define_insn_reservation "lvx_ext_int_ext" 3 (and (eq_attr "type" "ext_int") (eq_attr "issue" "ext")) "lvx_ext_r")

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
(define_bypass 3 "lvx_fmuld_*,lvx_fmaddd_*,lvx_fcvt_*"
                 "lvx_store_*,lvx_xstore_*"
                 "lvx_stored_value_bypass_p")
(define_bypass 2 "lvx_load_*,lvx_xload_*"
                 "lvx_store_*,lvx_xstore_*"
                 "lvx_stored_value_bypass_p")

;; The integer MAC accumulator reads its input a cycle later than other units.
(define_bypass 1 "lvx_imul_*,lvx_imadd_*"
                 "lvx_imadd_*"
                 "lvx_accumulator_bypass_p")
(define_bypass 2 "lvx_load_*,lvx_xload_*"
                 "lvx_imadd_*"
                 "lvx_accumulator_bypass_p")
