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

(define_insn_reservation "lvx_v1_nop" 1 (eq_attr "type" "nop") "lvx_alu_tiny_r")
(define_insn_reservation "lvx_v1_all" 1 (eq_attr "type" "all") "lvx_all_r")
(define_insn_reservation "lvx_v1_alu_full" 1 (eq_attr "type" "alu_full") "lvx_alu_full_r")
(define_insn_reservation "lvx_v1_alu_full_x" 1 (eq_attr "type" "alu_full_x") "lvx_alu_full_x_r")
(define_insn_reservation "lvx_v1_alu_full_sfu" 15 (eq_attr "type" "alu_full_sfu") "lvx_alu_full_r")
(define_insn_reservation "lvx_v1_alu_lite" 1 (eq_attr "type" "alu_lite") "lvx_alu_lite_r")
(define_insn_reservation "lvx_v1_alu_lite_w" 1 (eq_attr "type" "alu_lite_w") "lvx_alu_lite_x_r")
;; LITE counterparts of alu_tiny_recv / alu_full_sfu, for mnemonics the MDS
;; schedules as ALU_LITE (xmovetd, fsrsrw).
(define_insn_reservation "lvx_v1_alu_lite_recv" 1 (eq_attr "type" "alu_lite_recv") "lvx_alu_lite_misc_r")
(define_insn_reservation "lvx_v1_alu_lite_sfu" 15 (eq_attr "type" "alu_lite_sfu") "lvx_alu_lite_r")
(define_insn_reservation "lvx_v1_alu_lite_x" 1 (eq_attr "type" "alu_lite_x") "lvx_alu_lite_x_r")
(define_insn_reservation "lvx_v1_alu_lite_y" 1 (eq_attr "type" "alu_lite_y") "lvx_alu_lite_y_r")
(define_insn_reservation "lvx_v1_alu_lite_x2" 1 (eq_attr "type" "alu_lite_x2") "lvx_x2_alu_lite_r")
(define_insn_reservation "lvx_v1_alu_lite_x2_x" 1 (eq_attr "type" "alu_lite_x2_x") "lvx_x2_alu_lite_x_r")
(define_insn_reservation "lvx_v1_alu_tiny" 1 (eq_attr "type" "alu_tiny") "lvx_alu_tiny_r")
(define_insn_reservation "lvx_v1_alu_tiny_send" 1 (eq_attr "type" "alu_tiny_send") "lvx_alu_tiny_r")
(define_insn_reservation "lvx_v1_alu_tiny_recv" 1 (eq_attr "type" "alu_tiny_recv") "lvx_alu_tiny_r")
(define_insn_reservation "lvx_v1_alu_tiny_sendrecv" 1 (eq_attr "type" "alu_tiny_sendrecv") "lvx_alu_tiny_r")
(define_insn_reservation "lvx_v1_alu_tiny_use" 0 (eq_attr "type" "alu_tiny_use") "lvx_alu_tiny_r")
(define_insn_reservation "lvx_v1_alu_tiny_x" 1 (eq_attr "type" "alu_tiny_x") "lvx_alu_tiny_x_r")
(define_insn_reservation "lvx_v1_alu_tiny_y" 1 (eq_attr "type" "alu_tiny_y") "lvx_alu_tiny_y_r")
(define_insn_reservation "lvx_v1_alu_tiny_x2" 1 (eq_attr "type" "alu_tiny_x2") "lvx_x2_alu_tiny_r")
(define_insn_reservation "lvx_v1_alu_tiny_use_x2" 0 (eq_attr "type" "alu_tiny_use_x2") "lvx_x2_alu_tiny_r")
(define_insn_reservation "lvx_v1_alu_tiny_x4" 1 (eq_attr "type" "alu_tiny_x4") "lvx_x4_alu_tiny_r")
(define_insn_reservation "lvx_v1_alu_tiny_x4_x" 1 (eq_attr "type" "alu_tiny_x4_x") "lvx_x4_alu_tiny_x_r")
(define_insn_reservation "lvx_v1_alu_tiny_w" 1 (eq_attr "type" "alu_tiny_w") "lvx_alu_tiny_x_r")
;; movet_ext* emit xputdq, which the MDS schedules as ALU_LITE_MISC at
;; latency 1 -- one LITE unit, and no auxiliary read port.
(define_insn_reservation "lvx_v1_movet_ext_v2" 1 (eq_attr "type" "movet_ext") "lvx_x2_alu_lite_misc_r")
(define_insn_reservation "lvx_v1_movet_ext_lo_v2" 1 (eq_attr "type" "movet_ext_lo") "lvx_alu_lite_misc_r")
(define_insn_reservation "lvx_v1_movet_ext_hi_v2" 1 (eq_attr "type" "movet_ext_hi") "lvx_alu_lite_misc_r")
;; A template mixing a TINY and a LITE mnemonic (copyd + fnegd, for the complex
;; conjugate) needs the SUM of the two, and alu_tiny_x2 is not it: that reserves
;; any two TINY units, which may be tiny2+tiny3, leaving both LITE units free
;; for another insn -- three LITE-needing instructions on two LITE units, which
;; gas rejects.  Naming a LITE unit explicitly is what makes the pair sound.
;;
;; The four-mnemonic form (copyd + fnegd + copyd + fnegd) needs its own type
;; under this encoding: it is TINY4 + LITE2, where lvx_splatq256's four copyd
;; are TINY4 alone.  The absence_sets used to conflate the two by making
;; tiny0..tiny3 exclude both LITE units; with those gone the counters differ
;; and so must the types.
(define_insn_reservation "lvx_v1_alu_tiny_lite_x2" 1 (eq_attr "type" "alu_tiny_lite_x2") "lvx_alu_tiny__alu_lite_r")
(define_insn_reservation "lvx_v1_alu_tiny_lite_x4" 1 (eq_attr "type" "alu_tiny_lite_x4") "lvx_x2_alu_tiny__x2_alu_lite_r")
(define_insn_reservation "lvx_v1_cache" 1 (eq_attr "type" "cache") "lvx_lsu_r")
(define_insn_reservation "lvx_v1_cache_x" 1 (eq_attr "type" "cache_x") "lvx_lsu_x_r")
(define_insn_reservation "lvx_v1_cache_y" 1 (eq_attr "type" "cache_y") "lvx_lsu_y_r")
(define_insn_reservation "lvx_v1_cache2" 1 (eq_attr "type" "cache2") "lvx_lsu2_memw_r")
(define_insn_reservation "lvx_v1_cache2_x" 1 (eq_attr "type" "cache2_x") "lvx_lsu2_memw_x_r")
(define_insn_reservation "lvx_v1_cache2_y" 1 (eq_attr "type" "cache2_y") "lvx_lsu2_memw_y_r")
(define_insn_reservation "lvx_v1_preload" 1 (eq_attr "type" "preload") "lvx_lsu_r")
(define_insn_reservation "lvx_v1_preload_x" 1 (eq_attr "type" "preload_x") "lvx_lsu_x_r")
(define_insn_reservation "lvx_v1_preload_y" 1 (eq_attr "type" "preload_y") "lvx_lsu_y_r")
(define_insn_reservation "lvx_v1_load_ext" 4 (and (eq_attr "type" "load_ext") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_r")
(define_insn_reservation "lvx_v1_load_ext_x" 4 (and (eq_attr "type" "load_ext_x") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_x_r")
(define_insn_reservation "lvx_v1_load_ext_y" 4 (and (eq_attr "type" "load_ext_y") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_y_r")
(define_insn_reservation "lvx_v1_load_ext_" 4 (and (eq_attr "type" "load_ext") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_v1_load_ext_x_" 4 (and (eq_attr "type" "load_ext_x") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_x_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_v1_load_ext_y_" 4 (and (eq_attr "type" "load_ext_y") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_y_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_v1_load_core" 3 (and (eq_attr "type" "load_core") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_r")
(define_insn_reservation "lvx_v1_load_core_x" 3 (and (eq_attr "type" "load_core_x") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_x_r")
(define_insn_reservation "lvx_v1_load_core_y" 3 (and (eq_attr "type" "load_core_y") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_y_r")
(define_insn_reservation "lvx_v1_load_core_" 3 (and (eq_attr "type" "load_core") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_v1_load_core_x_" 3 (and (eq_attr "type" "load_core_x") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_x_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_v1_load_core_y_" 3 (and (eq_attr "type" "load_core_y") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_y_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_v1_load_core_uncached" 24 (and (eq_attr "type" "load_core_uncached") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_r")
(define_insn_reservation "lvx_v1_load_core_uncached_x" 24 (and (eq_attr "type" "load_core_uncached_x") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_x_r")
(define_insn_reservation "lvx_v1_load_core_uncached_y" 24 (and (eq_attr "type" "load_core_uncached_y") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_auxw_y_r")
(define_insn_reservation "lvx_v1_load_core_uncached_" 24 (and (eq_attr "type" "load_core_uncached") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_v1_load_core_uncached_x_" 24 (and (eq_attr "type" "load_core_uncached_x") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_x_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_v1_load_core_uncached_y_" 24 (and (eq_attr "type" "load_core_uncached_y") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_auxw_y_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_v1_load_ext_uncached" 24 (and (eq_attr "type" "load_ext_uncached") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_r")
(define_insn_reservation "lvx_v1_load_ext_uncached_x" 24 (and (eq_attr "type" "load_ext_uncached_x") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_x_r")
(define_insn_reservation "lvx_v1_load_ext_uncached_y" 24 (and (eq_attr "type" "load_ext_uncached_y") (match_test "TARGET_DUAL_LSU")) "lvx_lsu_y_r")
(define_insn_reservation "lvx_v1_load_ext_uncached_" 24 (and (eq_attr "type" "load_ext_uncached") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_v1_load_ext_uncached_x_" 24 (and (eq_attr "type" "load_ext_uncached_x") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_x_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_v1_load_ext_uncached_y_" 24 (and (eq_attr "type" "load_ext_uncached_y") (match_test "!TARGET_DUAL_LSU")) "lvx_lsu_y_r + lvx_lsu_x2_u")
(define_insn_reservation "lvx_v1_store_core" 1 (eq_attr "type" "store_core") "lvx_lsu_memw_auxr_r")
(define_insn_reservation "lvx_v1_store_core_x" 1 (eq_attr "type" "store_core_x") "lvx_lsu_memw_auxr_x_r")
(define_insn_reservation "lvx_v1_store_core_y" 1 (eq_attr "type" "store_core_y") "lvx_lsu_memw_auxr_y_r")
(define_insn_reservation "lvx_v1_store_ext" 1 (eq_attr "type" "store_ext") "lvx_lsu_memw_accr_r")
(define_insn_reservation "lvx_v1_store_ext_x" 1 (eq_attr "type" "store_ext_x") "lvx_lsu_memw_accr_x_r")
(define_insn_reservation "lvx_v1_store_ext_y" 1 (eq_attr "type" "store_ext_y") "lvx_lsu_memw_accr_y_r")
(define_insn_reservation "lvx_v1_aload_core" 24 (eq_attr "type" "aload_core") "lvx_lsu_memw_auxw_r")
(define_insn_reservation "lvx_v1_aload_core_x" 24 (eq_attr "type" "aload_core_x") "lvx_lsu_memw_auxw_x_r")
(define_insn_reservation "lvx_v1_aload_core_y" 24 (eq_attr "type" "aload_core_y") "lvx_lsu_memw_auxw_y_r")
(define_insn_reservation "lvx_v1_aloadclear_core" 24 (eq_attr "type" "aloadclear_core") "lvx_lsu2_memw_auxw_r")
(define_insn_reservation "lvx_v1_aloadclear_core_x" 24 (eq_attr "type" "aloadclear_core_x") "lvx_lsu2_memw_auxw_x_r")
(define_insn_reservation "lvx_v1_aloadclear_core_y" 24 (eq_attr "type" "aloadclear_core_y") "lvx_lsu2_memw_auxw_y_r")
(define_insn_reservation "lvx_v1_atomic_core" 24 (eq_attr "type" "atomic_core") "lvx_lsu2_memw_auxr_auxw_r")
(define_insn_reservation "lvx_v1_atomic_core_x" 24 (eq_attr "type" "atomic_core_x") "lvx_lsu2_memw_auxr_auxw_x_r")
(define_insn_reservation "lvx_v1_atomic_core_y" 24 (eq_attr "type" "atomic_core_y") "lvx_lsu2_memw_auxr_auxw_y_r")
(define_insn_reservation "lvx_v1_copy_core" 3 (eq_attr "type" "copy_core") "lvx_lsu_auxr_auxw_r")
(define_insn_reservation "lvx_v1_mult_int" 2 (eq_attr "type" "mult_int") "lvx_alu_lite_r")
(define_insn_reservation "lvx_v1_mult_int_x" 2 (eq_attr "type" "mult_int_x") "lvx_alu_lite_x_r")
(define_insn_reservation "lvx_v1_mult_int_y" 2 (eq_attr "type" "mult_int_y") "lvx_alu_lite_y_r")
;; "lvx_v1_mult_int_y"
(define_insn_reservation "lvx_v1_mult_fp3" 3 (eq_attr "type" "mult_fp3") "lvx_alu_lite_r")
(define_insn_reservation "lvx_v1_mult_fp4" 4 (eq_attr "type" "mult_fp4") "lvx_alu_lite_r")
(define_insn_reservation "lvx_v1_dotp_fp4" 4 (eq_attr "type" "dotp_fp4") "lvx_alu_full_r")
(define_insn_reservation "lvx_v1_conv_fp4" 4 (eq_attr "type" "conv_fp4") "lvx_alu_lite_r")
(define_insn_reservation "lvx_v1_madd_int" 2 (eq_attr "type" "madd_int") "lvx_alu_lite_r")
(define_insn_reservation "lvx_v1_madd_int_x" 2 (eq_attr "type" "madd_int_x") "lvx_alu_lite_x_r")
(define_insn_reservation "lvx_v1_madd_int_y" 2 (eq_attr "type" "madd_int_y") "lvx_alu_lite_y_r")
(define_insn_reservation "lvx_v1_madd_fp3" 3 (eq_attr "type" "madd_fp3") "lvx_alu_lite_r")
(define_insn_reservation "lvx_v1_madd_fp4" 4 (eq_attr "type" "madd_fp4") "lvx_alu_lite_r")
(define_insn_reservation "lvx_v1_dmda_fp4" 4 (eq_attr "type" "dmda_fp4") "lvx_alu_full_r")
(define_insn_reservation "lvx_v1_mulwq_int" 2 (eq_attr "type" "mulwq_int") "lvx_alu_lite_r")
(define_insn_reservation "lvx_v1_mulwq_fp4" 4 (eq_attr "type" "mulwq_fp4") "lvx_alu_lite_r")

(define_insn_reservation "lvx_v1_bcu" 1 (and (eq_attr "type" "bcu") (match_test "TARGET_DUAL_BCU")) "lvx_bcu_brrp_r")
(define_insn_reservation "lvx_v1_bcu_xfer" 1 (and (eq_attr "type" "bcu_xfer") (match_test "TARGET_DUAL_BCU")) "lvx_bcu_xfer_r")
(define_insn_reservation "lvx_v1_bcu_xfer_brrp" 1 (and (eq_attr "type" "bcu_xfer_brrp") (match_test "TARGET_DUAL_BCU")) "lvx_bcu_xfer_brrp_r")
(define_insn_reservation "lvx_v1_bcu2" 1 (eq_attr "type" "bcu2") "lvx_bcu2_r")
(define_insn_reservation "lvx_v1_bcu_get" 1 (eq_attr "type" "bcu_get") "lvx_bcu2_tiny_lsu_r")
(define_insn_reservation "lvx_v1_bcu_" 1 (and (eq_attr "type" "bcu") (match_test "!TARGET_DUAL_BCU")) "lvx_bcu_brrp_r + lvx_bcu_x2_u")
(define_insn_reservation "lvx_v1_bcu_xfer_" 1 (and (eq_attr "type" "bcu_xfer") (match_test "!TARGET_DUAL_BCU")) "lvx_bcu_xfer_r + lvx_bcu_x2_u")
(define_insn_reservation "lvx_v1_bcu_xfer_brrp_" 1 (and (eq_attr "type" "bcu_xfer_brrp") (match_test "!TARGET_DUAL_BCU")) "lvx_bcu_xfer_brrp_r + lvx_bcu_x2_u")

(define_insn_reservation "lvx_v1_movef_ext" 3 (eq_attr "type" "movef_ext") "lvx_ext_misc_auxw_r")
(define_insn_reservation "lvx_v1_copy_ext" 1 (eq_attr "type" "copy_ext") "lvx_ext_misc_auxw_r")
(define_insn_reservation "lvx_v1_ext" 1 (eq_attr "type" "ext") "lvx_ext_r")
(define_insn_reservation "lvx_v1_ext_int" 3 (eq_attr "type" "ext_int") "lvx_ext_r")
(define_insn_reservation "lvx_v1_ext_float" 4 (eq_attr "type" "ext_float") "lvx_ext_r")

;; Bypasses
;; The stores read their input one cycle later than other execution units.
(define_bypass 1 "lvx_v1_mult_int*,lvx_v1_mulwq_int*,lvx_v1_madd_int*"
                 "lvx_v1_store_core*"
                 "lvx_stored_value_bypass_p")
(define_bypass 2 "lvx_v1_mult_fp3*,lvx_v1_madd_fp3*"
                 "lvx_v1_store_core*"
                 "lvx_stored_value_bypass_p")
(define_bypass 3 "lvx_v1_mult_fp4*,lvx_v1_mulwq_fp4*,lvx_v1_dotp_fp4*,lvx_v1_conv_fp4*,lvx_v1_madd_fp4*,lvx_v1_dmda_fp4*"
                 "lvx_v1_store_core*"
                 "lvx_stored_value_bypass_p")
(define_bypass 2 "lvx_v1_load_core*"
                 "lvx_v1_store_core*"
                 "lvx_stored_value_bypass_p")
;; The integer MAC accumulator reads its input cycle later than other execution units.
(define_bypass 1 "lvx_v1_mult_int*,lvx_v1_mulwq_int*,lvx_v1_madd_int*"
                 "lvx_v1_madd_int*"
                 "lvx_accumulator_bypass_p")
(define_bypass 2 "lvx_v1_load_core*"
                 "lvx_v1_madd_int*"
                 "lvx_accumulator_bypass_p")

