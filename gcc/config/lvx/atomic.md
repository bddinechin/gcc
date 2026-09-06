;; Machine description for MPPA LVX processor synchronization primitives.
;; Copyright (C) 2019 Kalray Inc.
;;
;; This file is part of GCC.
;;
;; GCC is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.
;;
;; GCC is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;; General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.


;; GCC's builtins as described in gccint:
;;   6.53 Built-in Functions for Memory Model Aware Atomic Operations

;; Atomic compare-and-swap operation with memory model semantics.
(define_expand "atomic_compare_and_swap<mode>"
  [(match_operand:SI 0 "register_operand")   ;; bool output
   (match_operand:ACSI 1 "register_operand") ;; val output before CAS
   (match_operand:ACSI 2 "mematomic_operand");; memory
   (match_operand:ACSI 3 "register_operand") ;; expected
   (match_operand:ACSI 4 "register_operand") ;; desired
   (match_operand:SI 5 "const_int_operand")     ;; is_weak
   (match_operand:SI 6 "const_int_operand")     ;; model success
   (match_operand:SI 7 "const_int_operand")]    ;; model failure
  ""
  {
    lvx_expand_compare_and_swap (operands[0], operands[1], operands[2],
    operands[3], operands[4], operands[5], operands[6], operands[7]);
    DONE;
  }
)

;; Atomic load operation with memory model semantics.
(define_expand "atomic_load<mode>"
  [(match_operand:AI 0 "register_operand" "")   ;; val output
   (match_operand:AI 1 "mematomic_operand" "")  ;; memory
   (match_operand:SI 2 "const_int_operand" "")] ;; model
  ""
  {
    lvx_emit_pre_barrier (operands[2]);
    /* An atomic load bypasses the cache, but say so on a copy rather than on
       the caller's rtx.  set_mem_addr_space repoints the MEM it is given at
       fresh attributes, so mutating operands[1] in place changes every insn
       already emitted that still holds that same MEM.

       lvx_expand_compare_and_swap is exactly such a caller: it emits the
       acswap and only afterwards calls this expander on the same MEM, to
       reload the value on a failed weak compare.  The acswap pattern matches
       the memory once as an operand and once as a match_dup -- the dup is a
       copy -- so mutating the original retroactively left the two differing
       only in address space, the match_dup stopped matching, and every
       __atomic_compare_exchange_n ICEd with "unrecognizable insn".  */
    rtx mem = copy_rtx (operands[1]);
    set_mem_addr_space (mem, LVX_ADDR_SPACE_BYPASS);
    emit_move_insn (operands[0], mem);
    lvx_emit_post_barrier (operands[2]);
    DONE;
  }
)

;; Atomic store operation with memory model semantics.
(define_expand "atomic_store<mode>"
  [(match_operand:AI 0 "mematomic_operand" "")  ;; memory
   (match_operand:AI 1 "register_operand" "")   ;; val to write
   (match_operand:SI 2 "const_int_operand" "")] ;; model
  ""
  {
    lvx_emit_pre_barrier (operands[2]);
    emit_move_insn (operands[0], operands[1]);
    lvx_emit_post_barrier (operands[2]);
    DONE;
  }
)

;; Atomic exchange operation with memory model semantics.
(define_expand "atomic_exchange<mode>"
  [(match_operand:SIDI 0 "register_operand" "") ;; val output (memory content)
   (match_operand:SIDI 1 "mematomic_operand" "");; memory
   (match_operand:SIDI 2 "register_operand" "") ;; new value
   (match_operand:SI 3 "const_int_operand" "")] ;; model
  ""
  {
    lvx_expand_atomic_op (SET, operands[0], false, operands[1], operands[2], operands[3]);
    DONE;
  }
)

;; Atomic operation (add, sub, or, and, xor, nand) on memory with memory
;; model semantics.
(define_expand "atomic_<atomic_optab><mode>"
  [(match_operand:SIDI 0 "register_operand" "")    ;; op result
   (atomic_op:SIDI (match_dup 0)
     (match_operand:SIDI 1 "mematomic_operand" ""));; op1, op2
   (match_operand:SI 2 "const_int_operand" "")]    ;; model
  ""
  {
    lvx_expand_atomic_op (<CODE>, NULL_RTX, false, operands[0], operands[1], operands[2]);
    DONE;
  }
)

;; Atomic operation (add, sub, or, and, xor, nand) on memory with memory
;; model semantics, return the original value.
(define_expand "atomic_fetch_<atomic_optab><mode>"
 [(match_operand:SIDI 0 "register_operand" "")    ;; output (memory content before op)
  (atomic_op:SIDI
    (match_operand:SIDI 1 "mematomic_operand" "") ;; op1, op result
    (match_operand:SIDI 2 "register_operand" "")) ;; op2
   (match_operand:SI 3 "const_int_operand")]      ;; model
  ""
  {
    lvx_expand_atomic_op (<CODE>, operands[0], false, operands[1], operands[2], operands[3]);
    DONE;
  }
)

;; Atomic operation (add, sub, or, and, xor, nand) on memory with memory
;; model semantics, perform the operation then return the result.
(define_expand "atomic_<atomic_optab>_fetch<mode>"
 [(match_operand:SIDI 0 "register_operand" "")    ;; output (op result)
  (atomic_op:SIDI
    (match_operand:SIDI 1 "mematomic_operand" "") ;; op1, op result
    (match_operand:SIDI 2 "register_operand" "")) ;; op2
  (match_operand:SI 3 "const_int_operand" "")]    ;; model
  ""
  {
    lvx_expand_atomic_op (<CODE>, operands[0], true, operands[1], operands[2], operands[3]);
    DONE;
  }
)

;; TO GO FURTHER: atomic_exchange<mode> and
;; atomic_*<atomic_optab>*<mode> patterns above can also be
;; implemented for QI HI and TI modes by using a compare-and-swap loop
;; (with lvx_expand_atomic_op for example).

;; Atomic test-and-set operation on memory byte with memory model
;; semantics.
(define_expand "atomic_test_and_set"
 [(match_operand:QI 0 "register_operand" "")   ;; output (memory content)
  (match_operand:QI 1 "mematomic_operand" "")  ;; memory
  (match_operand:SI 2 "const_int_operand" "")] ;; model
  ""
  {
    lvx_expand_atomic_test_and_set (operands);
    DONE;
  }
)

;; Atomic bitwise operation on memory with memory model semantics,
;; return the original value of the specified bit.
;; - atomic_bit_test_and_set<mode>
;; - atomic_bit_test_and_complement<mode>
;; - atomic_bit_test_and_reset<mode>
;; These builtins are not implemented: atomic_fetch_[or,xor,and]<mode>
;; will be used instead.

;; TO GO FURTHER: atomic_*test_and_* patterns above can be implemented
;; by using a compare-and-swap loop (with lvx_expand_atomic_op for
;; example).

;; Thread fence with memory model semantics.
(define_expand "mem_thread_fence"
  [(match_operand:SI 0 "const_int_operand" "")]
  ""
  {
    enum memmodel model = memmodel_base (INTVAL (operands[0]));
    if (model != MEMMODEL_RELAXED)
      {
        rtx modifier = gen_rtx_CONST_STRING (VOIDmode, "");
        emit_insn (gen_lvx_fence (modifier));
      }
    DONE;
  }
)

;; Signal fence with memory model semantics.
(define_expand "mem_signal_fence"
  [(match_operand:SI 0 "const_int_operand" "")] ;; model
  ""
  {
    /* LVX memory model is strong enough not to require any
       barrier in order to synchronize a thread with itself. */
    DONE;
  }
)

;; Clobber memory contents to prevent moving around load and stores.
(define_expand "lvx_mem_clobber"
  [(clobber (mem:BLK (scratch)))]
  ""
)

;; Atomic Load
(define_insn "lvx_al<lsusize>"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r,r")
     (unspec_volatile:SIDI [(match_operand:SIDI 1 "mematomic_operand" "c,d,e")
                            (match_operand 2 "" "")] UNSPEC_ALOAD))
   (use (match_dup 1))]
  "HAVE_LVX_ATOMIC_LOAD_<MODE>"
  "al<lsusize>%2%X1 %0 = %O1"
  [(set_attr "type" "aload, aload, aload")
   (set_attr "issue" "lsu_memw_auxw, lsu_memw_auxw_x, lsu_memw_auxw_y")
   (set_attr "length"             "4,                8,               12")]
)

;; Atomic Store
(define_insn "lvx_as<lsusize>"
  [(set (match_operand:SIDI 1 "mematomic_operand"  "=c,d,e")
        (unspec_volatile:SIDI [(match_operand:SIDI 0 "register_operand" "r,r,r")
                               (match_operand 2 "" "")] UNSPEC_ASTORE))
   (clobber (match_dup 1))]
  "HAVE_LVX_ATOMIC_STORE_<MODE>"
  "as<lsusize>%2%X1 %O1 = %0"
  [(set_attr "type" "store, store, store")
   (set_attr "issue" "lsu_memw_auxr, lsu_memw_auxr_x, lsu_memw_auxr_y")
   (set_attr "length"            "4,               8,              12")]
)


;; Atomic Load and Clear
(define_expand "lvx_alclr<lsusize>"
  [(set (match_operand:SIDI 0 "register_operand" "")
        (unspec_volatile:SIDI [(match_operand:SIDI 1 "mematomic_operand" "")
                               (match_operand 2 "" "")] UNSPEC_ALCLR))]
  ""
  ""
)

(define_insn "alclr<lsusize>_1"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r,r")
         (unspec_volatile:SIDI [(match_operand:SIDI 1 "mematomic_operand" "a,b,m")
                                (match_operand 2 "" "")] UNSPEC_ALCLR))]
  "HAVE_LVX_ATOMIC_CV1_LIKE_LOAD_CLEAR_<MODE>"
  "alclr<lsusize>%2%X1 %0 = %1"
  [(set_attr "type" "aloadclear, aloadclear, aloadclear")
   (set_attr "issue" "lsu2_memw_auxw, lsu2_memw_auxw_x, lsu2_memw_auxw_y")
   (set_attr "length"             "4,                8,               12")]
)

(define_insn "alclr<lsusize>_2"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r,r")
        (unspec_volatile:SIDI [(match_operand:SIDI 1 "mematomic_operand" "c,d,e")
                               (match_operand 2 "" "")] UNSPEC_ALCLR))]
  "HAVE_LVX_ATOMIC_CV2_LIKE_LOAD_CLEAR_<MODE>"
  "alclr<lsusize>%2%X1 %0 = %O1"
  [(set_attr "type" "aloadclear, aloadclear, aloadclear")
   (set_attr "issue" "lsu2_memw_auxw, lsu2_memw_auxw_x, lsu2_memw_auxw_y")
   (set_attr "length"             "4,                8,               12")]
)

;; Atomic Load and Add
(define_expand "lvx_aladd<lsusize>"
  [(parallel
    [(set (match_operand:SIDI 0 "register_operand" "")
          (unspec_volatile:SIDI [(match_operand:SIDI 1 "mematomic_operand" "")
                                 (match_operand 3 "" "")] UNSPEC_ALADD))
     (set (match_dup 1)
          (plus:SIDI (match_dup 1)
                     (match_operand:SIDI 2 "nonmemory_operand" "")))]
  )]
  ""
  ""
)

(define_insn "aladd<lsusize>_1"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r,r")
        (unspec_volatile:SIDI [(match_operand:SIDI 1 "mematomic_operand" "+a,b,m")
                               (match_operand 3 "" "")] UNSPEC_ALADD))
   (set (match_dup 1)
        (plus:SIDI (match_dup 1)
                   (match_operand:SIDI 2 "nonmemory_operand" "0,0,0")))]
  "HAVE_LVX_ATOMIC_CV1_LIKE_LOAD_PLUS_<MODE>"
  "aladd<lsusize>%3%X1 %1 = %0"
  [(set_attr "type" "atomic, atomic, atomic")
   (set_attr "issue" "lsu2_memw_auxr_auxw, lsu2_memw_auxr_auxw_x, lsu2_memw_auxr_auxw_y")
   (set_attr "length"                  "4,                     8,                    12")]
)

(define_insn "aladd<lsusize>_2"
  [(set (match_operand:SIDI 0 "register_operand" "=r,r,r")
        (unspec_volatile:SIDI [(match_operand:SIDI 1 "mematomic_operand" "+c,d,e")
                               (match_operand 3 "" "")] UNSPEC_ALADD))
   (set (match_dup 1)
        (plus:SIDI (match_dup 1)
                   (match_operand:SIDI 2 "nonmemory_operand" "0,0,0")))]
  "HAVE_LVX_ATOMIC_CV2_LIKE_LOAD_PLUS_<MODE>"
  "aladd<lsusize>%3%X1 %O1 = %0"
  [(set_attr "type" "atomic, atomic, atomic")
   (set_attr "issue" "lsu2_memw_auxr_auxw, lsu2_memw_auxr_auxw_x, lsu2_memw_auxr_auxw_y")
   (set_attr "length"                  "4,                     8,                    12")]
)

;; Atomic Compare and Swap

(define_mode_iterator ACSWAP_IN [SI DI (TI "HAVE_LVX_ATOMIC_VALUED_CSWAP_TI")])
(define_mode_attr ACSWAP_EX [(SI "TI") (DI "TI") (TI "OI")])
(define_mode_attr ACSWAP_RET [(SI "SI") (DI "DI") (TI "TI")])
(define_mode_attr acswap_reg [(SI "x") (DI "x") (TI "q")])

(define_expand "lvx_acswap<lsusize>"
  [(parallel
    [(set (match_operand:<ACSWAP_EX> 0 "register_operand")
          (unspec_volatile:<ACSWAP_EX> [(match_operand:ACSWAP_IN 1 "mematomic_operand")
                               (match_operand 2)] UNSPEC_ACSWAP))
     (set (match_dup 1)
          (unspec:ACSWAP_IN [(match_dup 1) (match_dup 0)] UNSPEC_ACSWAP))
     (use (match_operand:SI 3 "nonmemory_operand"))]
  )]
  ""
  ""
)

(define_insn "acswap<lsusize>_1"
  [(set (match_operand:TI 0 "register_operand" "+r,r,r")
        (unspec_volatile:TI [(match_operand:SIDI 1 "mematomic_operand" "+a,b,m")
                             (match_operand 2 "" "")] UNSPEC_ACSWAP))
   (set (match_dup 1)
        (unspec:SIDI [(match_dup 1) (match_dup 0)] UNSPEC_ACSWAP))
   (use (match_operand:SI 3 "nonmemory_operand" ""))]
  "!HAVE_LVX_ATOMIC_VALUED_CSWAP_<MODE>"
  "acswap<lsusize>%2%X1 %1 = %0"
  [(set_attr "type" "atomic, atomic, atomic")
   (set_attr "issue" "lsu2_memw_auxr_auxw, lsu2_memw_auxr_auxw_x, lsu2_memw_auxr_auxw_y")
   (set_attr "length"                  "4,                     8,                    12")]
)


(define_insn "acswap<lsusize>_2"
  [(set (match_operand:<ACSWAP_EX> 0 "register_operand" "+r,r,r")
        (unspec_volatile:<ACSWAP_EX> [(match_operand:ACSWAP_IN 1 "mematomic_operand" "+c,d,e")
                             (match_operand 2 "" "")] UNSPEC_ACSWAP))
   (set (match_dup 1)
        (unspec:ACSWAP_IN [(match_dup 1) (match_dup 0)] UNSPEC_ACSWAP))
   (use (match_operand:SI 3 "nonmemory_operand" ""))]
  "HAVE_LVX_ATOMIC_VALUED_CSWAP_<MODE>"
  "acswap<lsusize>%2%X1 %<acswap_reg>0, %O1 = %0"
  [(set_attr "type" "atomic, atomic, atomic")
   (set_attr "issue" "lsu2_memw_auxr_auxw, lsu2_memw_auxr_auxw_x, lsu2_memw_auxr_auxw_y")
   (set_attr "length"                  "4,                     8,                    12")]
)

