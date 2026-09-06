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
  load_ext,
  load_uncached,
  load_ext_uncached,
  store,
  store_ext,
  aload,
  aloadclear,
  atomic,
  copy,
  cache,
  prefetch,
  branch,
  branch2,
  jump,
  ijump,
  sysget,
  move_to_ext,
  move_from_ext,
  copy_ext,
  ext,
  ext_int,
  ext_float,
  ext_send,
  ext_recv,
  ext_sendrecv"
  (const_string "alu"))
