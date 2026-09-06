
(define_c_enum "unspec"
  [
        UNSPEC_EFFECT
        UNSPEC_AWAIT
        UNSPEC_BARRIER
        UNSPEC_SLEEP
        UNSPEC_STOP

        UNSPEC_SXLBHQ
        UNSPEC_SXMBHQ
        UNSPEC_SXLHWP
        UNSPEC_SXMHWP
        UNSPEC_ZXLBHQ
        UNSPEC_ZXMBHQ
        UNSPEC_ZXLHWP
        UNSPEC_ZXMHWP

        UNSPEC_ALOAD
        UNSPEC_ASTORE
        UNSPEC_ALCLR
        UNSPEC_ALADD
        UNSPEC_ACSWAP
        UNSPEC_D1INVAL
        UNSPEC_I1INVAL
        UNSPEC_DINVALL
        UNSPEC_DPURGEL
        UNSPEC_DFLUSHL
        UNSPEC_I1INVALS
        UNSPEC_DINVALSW
        UNSPEC_DPURGESW
        UNSPEC_DFLUSHSW
        UNSPEC_FENCE
        UNSPEC_READY

        UNSPEC_DEF
        UNSPEC_GET
        UNSPEC_SET
        UNSPEC_SCALL
        UNSPEC_STSU
        UNSPEC_SBMM8D
        UNSPEC_SBMMT8D
        UNSPEC_WAITIT
        UNSPEC_WFXL
        UNSPEC_WFXM
        UNSPEC_SYNCGROUP
        UNSPEC_DOLOOP
        UNSPEC_ENDLOOP

        UNSPEC_AVG
        UNSPEC_AVGU
        UNSPEC_AVGR
        UNSPEC_AVGRU

        UNSPEC_ZX64
        UNSPEC_SX64
        UNSPEC_QX64
        UNSPEC_ZXWDP
        UNSPEC_SXWDP
        UNSPEC_QXWDP
        UNSPEC_SPLAT
        UNSPEC_SBMM8DS
        UNSPEC_SBMM8DXY
        UNSPEC_TRUNC
        UNSPEC_TRUNCL
        UNSPEC_TRUNCM
        UNSPEC_FRACT
        UNSPEC_FWIDEN
        UNSPEC_FNARROW
        UNSPEC_ADDD
        UNSPEC_ANDD
        UNSPEC_XORD
        UNSPEC_SLLD
        UNSPEC_SRLD
        UNSPEC_SRAD
        UNSPEC_SRS
        UNSPEC_ZXO
        UNSPEC_ZXE
        UNSPEC_QXO
        UNSPEC_QXE
        UNSPEC_OROE
        UNSPEC_MM
        UNSPEC_MMA
        UNSPEC_MMS

        UNSPEC_INITX2A
        UNSPEC_INITX2B
        UNSPEC_INITX4A
        UNSPEC_INITX4B
        UNSPEC_INITX4C
        UNSPEC_INITX4D
        UNSPEC_INITX8A
        UNSPEC_INITX8B
        UNSPEC_INITX8C
        UNSPEC_INITX8D
        UNSPEC_INITX8E
        UNSPEC_INITX8F
        UNSPEC_INITX8G
        UNSPEC_INITX8H

        UNSPEC_SELECT

        UNSPEC_GETSIGN
        UNSPEC_SETSIGN

        UNSPEC_FREC
        UNSPEC_FLOW
        UNSPEC_FRSR
        UNSPEC_FDOT2

        UNSPEC_FADD
        UNSPEC_FSBF
        UNSPEC_FDIV
        UNSPEC_FMUL
        UNSPEC_FFMA
        UNSPEC_FFMS
        UNSPEC_FMULC
        UNSPEC_FFMAC
        UNSPEC_FFMSC
        UNSPEC_FMIN
        UNSPEC_FMINN
        UNSPEC_FMAX
        UNSPEC_FMAXN
        UNSPEC_FMULX
        UNSPEC_FFMAX
        UNSPEC_FFMSX

        UNSPEC_FMM
        UNSPEC_FMMA
        UNSPEC_FMMS
        UNSPEC_FFDMA
        UNSPEC_FFDMS
        UNSPEC_FFDMDA
        UNSPEC_FFDMSA
        UNSPEC_FFDMDS
        UNSPEC_FFDMAS

        UNSPEC_FLOAT
        UNSPEC_FLOATU
        UNSPEC_FIXED
        UNSPEC_FIXEDU

        UNSPEC_CONJ
        UNSPEC_CMULT
        UNSPEC_CFMA
        UNSPEC_FCONJ
        UNSPEC_FCDIV
        UNSPEC_FSDIV
        UNSPEC_FSREC
        UNSPEC_FSRSR

        UNSPEC_TLBDINVAL
        UNSPEC_TLBIINVAL
        UNSPEC_TLBPROBE
        UNSPEC_TLBREAD
        UNSPEC_TLBWRITE
        UNSPEC_PIC
        UNSPEC_GOT
        UNSPEC_GOTOFF
        UNSPEC_TLS_LD_CALL

        ;; Keep this part in sync with lvx_unspec_tls_asm_op in lvx.c
        ;; The order must match
        UNSPEC_TLS_GD
        UNSPEC_TLS_LD
        UNSPEC_TLS_LE
        UNSPEC_TLS_DTPOFF
        UNSPEC_TLS_IE
        ;;

        UNSPEC_PCREL
        UNSPEC_LOAD
        UNSPEC_LOADC
        UNSPEC_LOADSX
        UNSPEC_LOADZX
        UNSPEC_XLOAD
        UNSPEC_XLOADC
        UNSPEC_XLOADQ0
        UNSPEC_XLOADQ1
        UNSPEC_XLOADQ2
        UNSPEC_XLOADQ3
        UNSPEC_XLOADCQ0
        UNSPEC_XLOADCQ1
        UNSPEC_XLOADCQ2
        UNSPEC_XLOADCQ3
        UNSPEC_STORE
        UNSPEC_STOREC
        UNSPEC_XSTORE
        UNSPEC_XSTOREC
        UNSPEC_XSTOREQ0
        UNSPEC_XSTOREQ1
        UNSPEC_XSTOREQ2
        UNSPEC_XSTOREQ3
        UNSPEC_XSTORECQ0
        UNSPEC_XSTORECQ1
        UNSPEC_XSTORECQ2
        UNSPEC_XSTORECQ3
        UNSPEC_XPRELOAD
        UNSPEC_XMOVEF
        UNSPEC_XMOVET
        UNSPEC_XMOVET_LO
        UNSPEC_XMOVET_HI
        UNSPEC_XALIGN256
        UNSPEC_XACCESS256
        UNSPEC_XCOPY

        ;; WIDEN* -- LVX's SIMD lane extension.  Each takes a 128-bit source
        ;; and the mostsig modifier selecting its least ("") or most (".m")
        ;; significant half, widening those lanes into a 128-bit result.
        ;; DIVMOD*: the quotient and remainder packed into a 128-bit pair,
        ;; quotient in the low 64 bits, remainder in the high 64.  Signed and
        ;; unsigned need distinct unspecs: with the same one their patterns
        ;; have identical RTL, recog matches whichever comes first, and both
        ;; emit the signed instruction.
        UNSPEC_FSQRT      ;; fsqrth / fsqrtw / fsqrtd
        UNSPEC_FRINT      ;; frinth / frintw / frintd

        UNSPEC_DIVMOD     ;; divmodw  / divmodd
        UNSPEC_DIVMODU    ;; divmoduw / divmodud

        UNSPEC_WIDENS
        UNSPEC_WIDENZ
        UNSPEC_WIDENQ

        UNSPEC_COMPND ;; todo: get rid of this unspec
        UNSPECV_NOP
   ]
)

