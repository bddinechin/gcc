;; -*- scheme -*-
;; What an instruction IS.  Semantic, hand-written, and deliberately saying
;; nothing about which slot it occupies or how long it takes: the slot is the
;; "issue" attribute, generated from the machine description, and the latency
;; is in scheduling.md.  Keeping the three apart is what lets a second LVX
;; core cost one file of latencies instead of a thousand edits.

(define_attr "type"
 "all,
  nop,
  ghost,
  alu,
  imul,
  imadd,
  fmuls,
  fmadds,
  fmuld,
  fmaddd,
  fcvt,
  fdotp,
  fdmda,
  fdiv,
  load,
  xload,
  store,
  xstore,
  aload,
  aloadc,
  atomic,
  copy,
  cache,
  prefetch,
  branch,
  branch2,
  jump,
  ijump,
  sysget,
  xmoveto,
  xmovef,
  xcopy,
  ext,
  ext_int,
  ext_float"
  (const_string "alu"))

;; Which variant of a memory access this is, in the MDS sense: LD, LD.S, LD.U
;; and LD.US are one instruction carrying the "variant" modifier, not four
;; instructions -- which is why the variant is an attribute of its own and not
;; two more values of "type" above.
;;
;; The default reads the variant off the RTL, from the address space of the
;; memory touched or from the modifier string a builtin passed, so no pattern
;; has to state it.  A pattern whose instruction has no cached form at all
;; overrides it.
;;
;; Nothing but the load latency in scheduling.md keys on this.

(define_attr "variant" "cached,speculate,uncached,preload"
  (cond [(match_test "lvx_insn_variant (insn) == LVX_VARIANT_SPECULATE")
	   (const_string "speculate")
	 (match_test "lvx_insn_variant (insn) == LVX_VARIANT_UNCACHED")
	   (const_string "uncached")
	 (match_test "lvx_insn_variant (insn) == LVX_VARIANT_PRELOAD")
	   (const_string "preload")]
	(const_string "cached")))
