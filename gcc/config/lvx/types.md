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
  loadu,
  xloadu,
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
