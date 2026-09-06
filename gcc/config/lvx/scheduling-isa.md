;; -*- scheme -*-
;; The LVX processor resources, and the vector of them each scheduling class
;; requires.  Generated from the Machine Description System by MDS/BE/GCC;
;; DO NOT EDIT, your changes will be overwritten.  Change lvx-mds's description
;; or MDS/BE/GCC/BIN/md-scheduling.pl instead.
;;
;; Latency, the bypasses and each pattern's issue class are deliberately not
;; here: the first two are microarchitecture and belong in
;; define_insn_reservation keyed on the semantic "type", and the third is a
;; property of the instructions a template emits, which only GCC knows.

(automata_option "ndfa")
(automata_option "v")

;; One automaton per resource.  Every alternation below stays inside a
;; single unit bank and no exclusion/presence/absence set is emitted, so
;; the partition is exact and each automaton minimises to its counter.
(define_automaton "lvx_issue")
(define_automaton "lvx_tiny")
(define_automaton "lvx_lite")
(define_automaton "lvx_full")
(define_automaton "lvx_lsu")
(define_automaton "lvx_bcu")
(define_automaton "lvx_ext")
(define_automaton "lvx_brrp")
(define_automaton "lvx_auxr")
(define_automaton "lvx_auxw")
(define_automaton "lvx_xfer")
(define_automaton "lvx_memw")
(define_automaton "lvx_misc")
(define_automaton "lvx_accr")

(define_cpu_unit "lvx_issue0_u, lvx_issue1_u, lvx_issue2_u, lvx_issue3_u, lvx_issue4_u, lvx_issue5_u, lvx_issue6_u, lvx_issue7_u" "lvx_issue")
(define_cpu_unit "lvx_tiny0_u, lvx_tiny1_u, lvx_tiny2_u, lvx_tiny3_u" "lvx_tiny")
(define_cpu_unit "lvx_lite0_u, lvx_lite1_u" "lvx_lite")
(define_cpu_unit "lvx_full0_u" "lvx_full")
(define_cpu_unit "lvx_lsu0_u, lvx_lsu1_u" "lvx_lsu")
(define_cpu_unit "lvx_bcu0_u, lvx_bcu1_u" "lvx_bcu")
(define_cpu_unit "lvx_ext0_u, lvx_ext1_u, lvx_ext2_u, lvx_ext3_u" "lvx_ext")
(define_cpu_unit "lvx_brrp0_u, lvx_brrp1_u" "lvx_brrp")
(define_cpu_unit "lvx_auxr0_u, lvx_auxr1_u" "lvx_auxr")
(define_cpu_unit "lvx_auxw0_u, lvx_auxw1_u" "lvx_auxw")
(define_cpu_unit "lvx_xfer0_u" "lvx_xfer")
(define_cpu_unit "lvx_memw0_u" "lvx_memw")
(define_cpu_unit "lvx_misc0_u, lvx_misc1_u" "lvx_misc")
(define_cpu_unit "lvx_accr0_u" "lvx_accr")

(define_reservation "lvx_issue_u" "(lvx_issue0_u|lvx_issue1_u|lvx_issue2_u|lvx_issue3_u|lvx_issue4_u|lvx_issue5_u|lvx_issue6_u|lvx_issue7_u)")
(define_reservation "lvx_issue_x2_u" "(lvx_issue0_u+lvx_issue1_u|lvx_issue1_u+lvx_issue2_u|lvx_issue2_u+lvx_issue3_u|lvx_issue3_u+lvx_issue4_u|lvx_issue4_u+lvx_issue5_u|lvx_issue5_u+lvx_issue6_u|lvx_issue6_u+lvx_issue7_u)")
(define_reservation "lvx_issue_x3_u" "(lvx_issue0_u+lvx_issue1_u+lvx_issue2_u|lvx_issue1_u+lvx_issue2_u+lvx_issue3_u|lvx_issue2_u+lvx_issue3_u+lvx_issue4_u|lvx_issue3_u+lvx_issue4_u+lvx_issue5_u|lvx_issue4_u+lvx_issue5_u+lvx_issue6_u|lvx_issue5_u+lvx_issue6_u+lvx_issue7_u)")
(define_reservation "lvx_issue_x4_u" "(lvx_issue0_u+lvx_issue1_u+lvx_issue2_u+lvx_issue3_u|lvx_issue1_u+lvx_issue2_u+lvx_issue3_u+lvx_issue4_u|lvx_issue2_u+lvx_issue3_u+lvx_issue4_u+lvx_issue5_u|lvx_issue3_u+lvx_issue4_u+lvx_issue5_u+lvx_issue6_u|lvx_issue4_u+lvx_issue5_u+lvx_issue6_u+lvx_issue7_u)")
(define_reservation "lvx_issue_x8_u" "lvx_issue0_u+lvx_issue1_u+lvx_issue2_u+lvx_issue3_u+lvx_issue4_u+lvx_issue5_u+lvx_issue6_u+lvx_issue7_u")
(define_reservation "lvx_tiny_u" "(lvx_tiny0_u|lvx_tiny1_u|lvx_tiny2_u|lvx_tiny3_u)")
(define_reservation "lvx_tiny_x2_u" "(lvx_tiny0_u+lvx_tiny1_u|lvx_tiny1_u+lvx_tiny2_u|lvx_tiny2_u+lvx_tiny3_u)")
(define_reservation "lvx_tiny_x4_u" "lvx_tiny0_u+lvx_tiny1_u+lvx_tiny2_u+lvx_tiny3_u")
(define_reservation "lvx_lite_u" "(lvx_lite0_u|lvx_lite1_u)")
(define_reservation "lvx_lite_x2_u" "lvx_lite0_u+lvx_lite1_u")
(define_reservation "lvx_full_u" "lvx_full0_u")
(define_reservation "lvx_lsu_u" "(lvx_lsu0_u|lvx_lsu1_u)")
(define_reservation "lvx_lsu_x2_u" "lvx_lsu0_u+lvx_lsu1_u")
(define_reservation "lvx_bcu_u" "(lvx_bcu0_u|lvx_bcu1_u)")
(define_reservation "lvx_bcu_x2_u" "lvx_bcu0_u+lvx_bcu1_u")
(define_reservation "lvx_ext_u" "(lvx_ext0_u|lvx_ext1_u|lvx_ext2_u|lvx_ext3_u)")
(define_reservation "lvx_brrp_u" "(lvx_brrp0_u|lvx_brrp1_u)")
(define_reservation "lvx_auxr_u" "(lvx_auxr0_u|lvx_auxr1_u)")
(define_reservation "lvx_auxw_u" "(lvx_auxw0_u|lvx_auxw1_u)")
(define_reservation "lvx_xfer_u" "lvx_xfer0_u")
(define_reservation "lvx_memw_u" "lvx_memw0_u")
(define_reservation "lvx_misc_u" "(lvx_misc0_u|lvx_misc1_u)")
(define_reservation "lvx_misc_x2_u" "lvx_misc0_u+lvx_misc1_u")
(define_reservation "lvx_accr_u" "lvx_accr0_u")

(define_reservation "lvx_all_r" "lvx_issue_x8_u")
(define_reservation "lvx_alu_tiny_r" "lvx_issue_u + lvx_tiny_u")
(define_reservation "lvx_alu_tiny_x_r" "lvx_issue_x2_u + lvx_tiny_u")
(define_reservation "lvx_alu_tiny_y_r" "lvx_issue_x3_u + lvx_tiny_u")
(define_reservation "lvx_alu_lite_r" "lvx_issue_u + lvx_tiny_u + lvx_lite_u")
(define_reservation "lvx_alu_lite_x_r" "lvx_issue_x2_u + lvx_tiny_u + lvx_lite_u")
(define_reservation "lvx_alu_lite_y_r" "lvx_issue_x3_u + lvx_tiny_u + lvx_lite_u")
(define_reservation "lvx_alu_lite_misc_r" "lvx_issue_u + lvx_tiny_u + lvx_lite_u + lvx_misc_u")
(define_reservation "lvx_alu_full_r" "lvx_issue_u + lvx_tiny_u + lvx_lite_u + lvx_full_u")
(define_reservation "lvx_alu_full_x_r" "lvx_issue_x2_u + lvx_tiny_u + lvx_lite_u + lvx_full_u")
(define_reservation "lvx_bcu_brrp_r" "lvx_issue_u + lvx_bcu_u + lvx_brrp_u")
(define_reservation "lvx_bcu2_r" "lvx_issue_u + lvx_bcu_x2_u")
(define_reservation "lvx_bcu_xfer_r" "lvx_issue_u + lvx_bcu_u + lvx_xfer_u")
(define_reservation "lvx_bcu_xfer_brrp_r" "lvx_issue_u + lvx_bcu_u + lvx_brrp_u + lvx_xfer_u")
(define_reservation "lvx_bcu2_tiny_lsu_r" "lvx_issue_u + lvx_tiny_u + lvx_lsu_u + lvx_bcu_x2_u")
(define_reservation "lvx_lsu_r" "lvx_issue_u + lvx_tiny_u + lvx_lsu_u")
(define_reservation "lvx_lsu_x_r" "lvx_issue_x2_u + lvx_tiny_u + lvx_lsu_u")
(define_reservation "lvx_lsu_y_r" "lvx_issue_x3_u + lvx_tiny_u + lvx_lsu_u")
(define_reservation "lvx_lsu_memw_accr_r" "lvx_issue_u + lvx_tiny_u + lvx_lsu_u + lvx_memw_u + lvx_accr_u")
(define_reservation "lvx_lsu_memw_accr_x_r" "lvx_issue_x2_u + lvx_tiny_u + lvx_lsu_u + lvx_memw_u + lvx_accr_u")
(define_reservation "lvx_lsu_memw_accr_y_r" "lvx_issue_x3_u + lvx_tiny_u + lvx_lsu_u + lvx_memw_u + lvx_accr_u")
(define_reservation "lvx_lsu2_memw_r" "lvx_issue_u + lvx_tiny_u + lvx_lsu_x2_u + lvx_memw_u")
(define_reservation "lvx_lsu2_memw_x_r" "lvx_issue_x2_u + lvx_tiny_u + lvx_lsu_x2_u + lvx_memw_u")
(define_reservation "lvx_lsu2_memw_y_r" "lvx_issue_x3_u + lvx_tiny_u + lvx_lsu_x2_u + lvx_memw_u")
(define_reservation "lvx_lsu_memw_auxr_r" "lvx_issue_u + lvx_tiny_u + lvx_lsu_u + lvx_auxr_u + lvx_memw_u")
(define_reservation "lvx_lsu_memw_auxr_x_r" "lvx_issue_x2_u + lvx_tiny_u + lvx_lsu_u + lvx_auxr_u + lvx_memw_u")
(define_reservation "lvx_lsu_memw_auxr_y_r" "lvx_issue_x3_u + lvx_tiny_u + lvx_lsu_u + lvx_auxr_u + lvx_memw_u")
(define_reservation "lvx_lsu_memw_auxw_r" "lvx_issue_u + lvx_tiny_u + lvx_lsu_u + lvx_auxw_u + lvx_memw_u")
(define_reservation "lvx_lsu_memw_auxw_x_r" "lvx_issue_x2_u + lvx_tiny_u + lvx_lsu_u + lvx_auxw_u + lvx_memw_u")
(define_reservation "lvx_lsu_memw_auxw_y_r" "lvx_issue_x3_u + lvx_tiny_u + lvx_lsu_u + lvx_auxw_u + lvx_memw_u")
(define_reservation "lvx_lsu2_memw_auxw_r" "lvx_issue_u + lvx_tiny_u + lvx_lsu_x2_u + lvx_auxw_u + lvx_memw_u")
(define_reservation "lvx_lsu2_memw_auxw_x_r" "lvx_issue_x2_u + lvx_tiny_u + lvx_lsu_x2_u + lvx_auxw_u + lvx_memw_u")
(define_reservation "lvx_lsu2_memw_auxw_y_r" "lvx_issue_x3_u + lvx_tiny_u + lvx_lsu_x2_u + lvx_auxw_u + lvx_memw_u")
(define_reservation "lvx_lsu_auxw_r" "lvx_issue_u + lvx_tiny_u + lvx_lsu_u + lvx_auxw_u")
(define_reservation "lvx_lsu_auxw_x_r" "lvx_issue_x2_u + lvx_tiny_u + lvx_lsu_u + lvx_auxw_u")
(define_reservation "lvx_lsu_auxw_y_r" "lvx_issue_x3_u + lvx_tiny_u + lvx_lsu_u + lvx_auxw_u")
(define_reservation "lvx_lsu_auxr_auxw_r" "lvx_issue_u + lvx_tiny_u + lvx_lsu_u + lvx_auxr_u + lvx_auxw_u")
(define_reservation "lvx_lsu2_memw_auxr_auxw_r" "lvx_issue_u + lvx_tiny_u + lvx_lsu_x2_u + lvx_auxr_u + lvx_auxw_u + lvx_memw_u")
(define_reservation "lvx_lsu2_memw_auxr_auxw_x_r" "lvx_issue_x2_u + lvx_tiny_u + lvx_lsu_x2_u + lvx_auxr_u + lvx_auxw_u + lvx_memw_u")
(define_reservation "lvx_lsu2_memw_auxr_auxw_y_r" "lvx_issue_x3_u + lvx_tiny_u + lvx_lsu_x2_u + lvx_auxr_u + lvx_auxw_u + lvx_memw_u")
(define_reservation "lvx_ext_r" "lvx_issue_u + lvx_ext_u")
(define_reservation "lvx_ext_misc_auxw_r" "lvx_issue_u + lvx_ext_u + lvx_auxw_u + lvx_misc_u")

(define_reservation "lvx_alu_tiny__alu_lite_r" "lvx_issue_x2_u + lvx_tiny_x2_u + lvx_lite_u")
(define_reservation "lvx_x2_alu_lite_r" "lvx_issue_x2_u + lvx_tiny_x2_u + lvx_lite_x2_u")
(define_reservation "lvx_x2_alu_lite_misc_r" "lvx_issue_x2_u + lvx_tiny_x2_u + lvx_lite_x2_u + lvx_misc_x2_u")
(define_reservation "lvx_x2_alu_lite_x_r" "lvx_issue_x4_u + lvx_tiny_x2_u + lvx_lite_x2_u")
(define_reservation "lvx_x2_alu_tiny_r" "lvx_issue_x2_u + lvx_tiny_x2_u")
(define_reservation "lvx_x2_alu_tiny__x2_alu_lite_r" "lvx_issue_x4_u + lvx_tiny_x4_u + lvx_lite_x2_u")
(define_reservation "lvx_x4_alu_tiny_r" "lvx_issue_x4_u + lvx_tiny_x4_u")
(define_reservation "lvx_x4_alu_tiny_x_r" "lvx_issue_x8_u + lvx_tiny_x4_u")

;; Which slot class a pattern's instructions occupy, summed when its
;; template emits more than one.  A pattern states this; the reservation
;; each value stands for is above.
(define_attr "issue"
  "all,
   alu_tiny,
   alu_tiny_x,
   alu_tiny_y,
   alu_lite,
   alu_lite_x,
   alu_lite_y,
   alu_lite_misc,
   alu_full,
   alu_full_x,
   bcu_brrp,
   bcu2,
   bcu_xfer,
   bcu_xfer_brrp,
   bcu2_tiny_lsu,
   lsu,
   lsu_x,
   lsu_y,
   lsu_memw_accr,
   lsu_memw_accr_x,
   lsu_memw_accr_y,
   lsu2_memw,
   lsu2_memw_x,
   lsu2_memw_y,
   lsu_memw_auxr,
   lsu_memw_auxr_x,
   lsu_memw_auxr_y,
   lsu_memw_auxw,
   lsu_memw_auxw_x,
   lsu_memw_auxw_y,
   lsu2_memw_auxw,
   lsu2_memw_auxw_x,
   lsu2_memw_auxw_y,
   lsu_auxw,
   lsu_auxw_x,
   lsu_auxw_y,
   lsu_auxr_auxw,
   lsu2_memw_auxr_auxw,
   lsu2_memw_auxr_auxw_x,
   lsu2_memw_auxr_auxw_y,
   ext,
   ext_misc_auxw,
   alu_tiny__alu_lite,
   x2_alu_lite,
   x2_alu_lite_misc,
   x2_alu_lite_x,
   x2_alu_tiny,
   x2_alu_tiny__x2_alu_lite,
   x4_alu_tiny,
   x4_alu_tiny_x"
  (const_string "all"))
