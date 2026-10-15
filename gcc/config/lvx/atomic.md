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
  [(match_operand:AMO 0 "register_operand" "") ;; val output (memory content)
   (match_operand:AMO 1 "mematomic_operand" "");; memory
   (match_operand:AMO 2 "register_operand" "") ;; new value
   (match_operand:SI 3 "const_int_operand" "")] ;; model
  ""
  {
    if (!lvx_expand_atomic_op (SET, operands[0], false, operands[1], operands[2], operands[3]))
      FAIL;
    DONE;
  }
)

;; Atomic operation (add, sub, or, and, xor, nand) on memory with memory
;; model semantics.
(define_expand "atomic_<atomic_optab><mode>"
  [(match_operand:AMO 0 "register_operand" "")    ;; op result
   (atomic_op:AMO (match_dup 0)
     (match_operand:AMO 1 "mematomic_operand" ""));; op1, op2
   (match_operand:SI 2 "const_int_operand" "")]    ;; model
  ""
  {
    if (!lvx_expand_atomic_op (<CODE>, NULL_RTX, false, operands[0], operands[1], operands[2]))
      FAIL;
    DONE;
  }
)

;; Atomic operation (add, sub, or, and, xor, nand) on memory with memory
;; model semantics, return the original value.
(define_expand "atomic_fetch_<atomic_optab><mode>"
 [(match_operand:AMO 0 "register_operand" "")    ;; output (memory content before op)
  (atomic_op:AMO
    (match_operand:AMO 1 "mematomic_operand" "") ;; op1, op result
    (match_operand:AMO 2 "register_operand" "")) ;; op2
   (match_operand:SI 3 "const_int_operand")]      ;; model
  ""
  {
    if (!lvx_expand_atomic_op (<CODE>, operands[0], false, operands[1], operands[2], operands[3]))
      FAIL;
    DONE;
  }
)

;; Atomic operation (add, sub, or, and, xor, nand) on memory with memory
;; model semantics, perform the operation then return the result.
(define_expand "atomic_<atomic_optab>_fetch<mode>"
 [(match_operand:AMO 0 "register_operand" "")    ;; output (op result)
  (atomic_op:AMO
    (match_operand:AMO 1 "mematomic_operand" "") ;; op1, op result
    (match_operand:AMO 2 "register_operand" "")) ;; op2
  (match_operand:SI 3 "const_int_operand" "")]    ;; model
  ""
  {
    if (!lvx_expand_atomic_op (<CODE>, operands[0], true, operands[1], operands[2], operands[3]))
      FAIL;
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
    /* No hardware fence: a thread needs none to synchronise with itself, which
       is all a signal fence is for.  But a COMPILER blockage is the whole
       point, and defining this optab suppresses the asm memory blockage GCC
       would otherwise emit for __atomic_signal_fence -- riscv defines no
       mem_signal_fence at all and gets that fallback.  The IL-level barrier
       already stops the tree passes (both stores survive either way), so this
       is belt and braces against an RTL pass moving memory across it; it costs
       no instruction.  */
    emit_insn (gen_lvx_mem_clobber ());
    DONE;
  }
)

;; Clobber memory contents to prevent moving around load and stores.
(define_expand "lvx_mem_clobber"
  [(clobber (mem:BLK (scratch)))]
  ""
)

;; Atomic Load
;; al exists at b, h, w and d.  The condition was HAVE_LVX_ATOMIC_LOAD_<MODE>,
;; defined for SI and DI only, which is the KV3 restriction again -- not a
;; property of this ISA.
(define_insn "lvx_al<lsusize>"
  [(set (match_operand:AMO 0 "register_operand" "=r,r,r")
     (unspec_volatile:AMO [(match_operand:AMO 1 "mematomic_operand" "c,d,e")
                            (match_operand 2 "" "")] UNSPEC_ALOAD))
   (use (match_dup 1))]
  ""
  "al<lsusize>%2%X1 %0 = %O1"
  [(set_attr "type" "aload, aload, aload")
   (set_attr "issue" "lsu_memw_auxw, lsu_memw_auxw_x, lsu_memw_auxw_x2")
   (set_attr "length"             "4,                8,               12")
   (set_attr "predicable" "no")]
)

;; Atomic Store
(define_insn "lvx_as<lsusize>"
  [(set (match_operand:AMO 1 "mematomic_operand"  "=c,d,e")
        (unspec_volatile:AMO [(match_operand:AMO 0 "register_operand" "r,r,r")
                               (match_operand 2 "" "")] UNSPEC_ASTORE))
   (clobber (match_dup 1))]
  ""
  "as<lsusize>%2%X1 %O1 = %0"
  [(set_attr "type" "store, store, store")
   (set_attr "issue" "lsu_memw_auxr, lsu_memw_auxr_x, lsu_memw_auxr_x2")
   (set_attr "length"            "4,               8,              12")]
)


;; Atomic Load and Clear
(define_expand "lvx_alclr<lsusize>"
  [(set (match_operand:AMO 0 "register_operand" "")
        (unspec_volatile:AMO [(match_operand:AMO 1 "mematomic_operand" "")
                               (match_operand 2 "" "")] UNSPEC_ALCLR))]
  ""
  ""
)



(define_insn "alclr<lsusize>_2"
  [(set (match_operand:AMO 0 "register_operand" "=r,r,r")
        (unspec_volatile:AMO [(match_operand:AMO 1 "mematomic_operand" "c,d,e")
                               (match_operand 2 "" "")] UNSPEC_ALCLR))]
  ""
  "alclr<lsusize>%2%X1 %0 = %O1"
  [(set_attr "type" "aloadc, aloadc, aloadc")
   (set_attr "issue" "lsu2_memw_auxw, lsu2_memw_auxw_x, lsu2_memw_auxw_x2")
   (set_attr "length"             "4,                8,               12")
   (set_attr "predicable" "no")]
)

;; Atomic Load and Add
(define_expand "lvx_aladd<lsusize>"
  [(parallel
    [(set (match_operand:AMO 0 "register_operand" "")
          (unspec_volatile:AMO [(match_operand:AMO 1 "mematomic_operand" "")
                                 (match_operand 3 "" "")] UNSPEC_ALADD))
     (set (match_dup 1)
          (plus:AMO (match_dup 1)
                     (match_operand:AMO 2 "nonmemory_operand" "")))]
  )]
  ""
  ""
)



;; ---------------------------------------------------------------------------
;; The native read-modify-write instructions.
;;
;; al<op> returns the old value and as<op> does not, which is exactly GCC's
;; distinction between atomic_fetch_<op> and atomic_<op>.  Both exist at b, h,
;; w and d, so a byte or half-word atomic is one instruction -- where before
;; this the port had SI/DI only and everything narrower went through the middle
;; end's mask-shift-and-word-CAS emulation, about 35 instructions for one
;; __atomic_fetch_add on a char.
;;
;; The old value comes back IN THE VALUE REGISTER: the ISA gives these two
;; register operands, an address and a value, and ALADDW's behaviour writes
;; `result2', the value operand.  Hence the "0" tie on operand 2.
;;
;; The coherency modifier is operand 3, and the standard atomics pass "" --
;; the Local domain.  Nothing here chooses that; it is what the port has always
;; passed (lvx_expand_atomic_op), and the place to revisit if C11 atomics must
;; be coherent beyond the local domain.
(define_insn "al<amo_mnemonic><lsusize>_amo"
  [(set (match_operand:AMO 0 "register_operand" "=r,r,r")
        (unspec_volatile:AMO [(match_operand:AMO 1 "mematomic_operand" "+c,d,e")
                              (match_operand 3 "" "")] UNSPEC_AMO_FETCH))
   (set (match_dup 1)
        (amo_op:AMO (match_dup 1)
                    (match_operand:AMO 2 "register_operand" "0,0,0")))]
  ""
  "al<amo_mnemonic><lsusize>%3%X1 %O1 = %0"
  [(set_attr "type" "atomic, atomic, atomic")
   (set_attr "issue" "lsu2_memw_auxr_auxw, lsu2_memw_auxr_auxw_x, lsu2_memw_auxr_auxw_x2")
   (set_attr "length"                  "4,                     8,                    12")
   (set_attr "predicable" "no")]
)

;; The same operation with the old value discarded: one as<op>, no result
;; register.  C cannot ask for this -- GCC always expands an atomic
;; read-modify-write through the FETCH optab and lets dead-code elimination
;; drop the unused result, so the atomic_<op><mode> optab is never chosen and a
;; pattern reachable only from it would be dead.  These therefore exist for the
;; builtins, whose whole purpose is the operations C has no syntax for: an
;; atomic add that does not report the old value frees the register and the
;; dependency on it.
;;
;; Operand order follows lvx_as<lsusize>, because LVX_EXPAND_BUILTIN_ASTORE
;; calls gen_<name> (value, memory, modifier).
(define_insn "lvx_as<amo_mnemonic><lsusize>"
  [(set (match_operand:AMO 1 "mematomic_operand" "=c,d,e")
        (unspec_volatile:AMO [(amo_op:AMO (match_dup 1)
                                          (match_operand:AMO 0 "register_operand" "r,r,r"))
                              (match_operand 2 "" "")] UNSPEC_AMO_NOFETCH))
   (clobber (match_dup 1))]
  ""
  "as<amo_mnemonic><lsusize>%2%X1 %O1 = %0"
  [(set_attr "type" "atomic, atomic, atomic")
   (set_attr "issue" "lsu2_memw_auxr_auxw, lsu2_memw_auxr_auxw_x, lsu2_memw_auxr_auxw_x2")
   (set_attr "length"                  "4,                     8,                    12")
   (set_attr "predicable" "no")]
)

;; The atomic swap, store-only: aswap writes memory and returns nothing, so it
;; cannot serve atomic_exchange, which must yield the old value -- that keeps
;; the compare-and-swap loop, the one place these atomics are behind RISC-V's
;; amoswap.  As a builtin it is exactly the "swap and do not tell me" operation.
(define_insn "lvx_aswap<lsusize>"
  [(set (match_operand:AMO 1 "mematomic_operand" "=c,d,e")
        (unspec_volatile:AMO [(match_operand:AMO 0 "register_operand" "r,r,r")
                              (match_operand 2 "" "")] UNSPEC_AMO_NOFETCH))
   (clobber (match_dup 1))]
  ""
  "aswap<lsusize>%2%X1 %O1 = %0"
  [(set_attr "type" "atomic, atomic, atomic")
   (set_attr "issue" "lsu2_memw_auxr_auxw, lsu2_memw_auxr_auxw_x, lsu2_memw_auxr_auxw_x2")
   (set_attr "length"                  "4,                     8,                    12")
   (set_attr "predicable" "no")]
)

(define_insn "aladd<lsusize>_2"
  [(set (match_operand:AMO 0 "register_operand" "=r,r,r")
        (unspec_volatile:AMO [(match_operand:AMO 1 "mematomic_operand" "+c,d,e")
                               (match_operand 3 "" "")] UNSPEC_ALADD))
   (set (match_dup 1)
        (plus:AMO (match_dup 1)
                   (match_operand:AMO 2 "nonmemory_operand" "0,0,0")))]
  ""
  "aladd<lsusize>%3%X1 %O1 = %0"
  [(set_attr "type" "atomic, atomic, atomic")
   (set_attr "issue" "lsu2_memw_auxr_auxw, lsu2_memw_auxr_auxw_x, lsu2_memw_auxr_auxw_x2")
   (set_attr "length"                  "4,                     8,                    12")
   (set_attr "predicable" "no")]
)

;; Atomic Compare and Swap

;; SI, DI and TI.  acswap exists at b, h, w, d and q, so the 128-bit form is a
;; real instruction rather than an emulation.  TI used to be gated on
;; HAVE_LVX_ATOMIC_VALUED_CSWAP_TI, one of three macros recording whether a
;; width had the VALUED compare-and-swap: on KV3 a CAS in local memory returned
;; the old value while one in DDR returned only a success flag, so the same
;; instruction had two shapes depending on where the address pointed.  On LVX
;; every width has the valued ENCODING -- a result register of its own -- so all
;; three macros were (1) and the pattern they gated, which had no result
;; register, was unreachable.
;;
;; Encoding and flavour are two different things, and the port currently mixes
;; them: it uses the valued-encoding pattern but passes `boolcas' = `.', so the
;; result register receives a success FLAG and not the old value, and
;; lvx_expand_compare_and_swap reads it as a boolean and loops on failure.
;; Matching RISC-V, whose CAS yields the old value, means passing `.V' and
;; deriving success from a comparison -- which also removes the loop, a valued
;; CAS having no spurious failure.  That is a semantic change to the atomics
;; and is not done here.
(define_mode_iterator ACSWAP_IN [QI HI SI DI TI])
;; The register pair the instruction reads, and the quad at 128 bits.  A byte
;; and a half word use a pair like a word does: only the low `size' bytes of
;; each half are compared and written (the byteMask), so the bits above are
;; immaterial and need no masking.
(define_mode_attr ACSWAP_EX [(QI "TI") (HI "TI") (SI "TI") (DI "TI") (TI "OI")])
(define_mode_attr ACSWAP_RET [(QI "QI") (HI "HI") (SI "SI") (DI "DI") (TI "TI")])
(define_mode_attr acswap_reg [(QI "x") (HI "x") (SI "x") (DI "x") (TI "q")])

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

;; The boolean-CAS pattern that stood here is gone with the macros that gated
;; it.  It spelled `acswap %1 = %0' -- memory and a success flag, no old value
;; -- which is the DDR shape of the KV3 instruction; LVX returns the old value
;; at every width, so only the valued form below is ever emitted.

(define_insn "acswap<lsusize>_2"
  [(set (match_operand:<ACSWAP_EX> 0 "register_operand" "+r,r,r")
        (unspec_volatile:<ACSWAP_EX> [(match_operand:ACSWAP_IN 1 "mematomic_operand" "+c,d,e")
                             (match_operand 2 "" "")] UNSPEC_ACSWAP))
   (set (match_dup 1)
        (unspec:ACSWAP_IN [(match_dup 1) (match_dup 0)] UNSPEC_ACSWAP))
   (use (match_operand:SI 3 "nonmemory_operand" ""))]
  ""
  "acswap<lsusize>%2%X1 %<acswap_reg>0, %O1 = %0"
  [(set_attr "type" "atomic, atomic, atomic")
   (set_attr "issue" "lsu2_memw_auxr_auxw, lsu2_memw_auxr_auxw_x, lsu2_memw_auxr_auxw_x2")
   (set_attr "length"                  "4,                     8,                    12")
   (set_attr "predicable" "no")]
)

